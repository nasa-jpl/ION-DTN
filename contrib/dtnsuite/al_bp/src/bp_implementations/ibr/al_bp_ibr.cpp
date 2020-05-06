/********************************************************
 **  Authors: Davide Pallotti, davide.pallotti@studio.unibo.it
 **           Carlo Caini (DTNperf_3 project supervisor), carlo.caini@unibo.it
 **
 **
 **  Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 **  All rights reserved.
 ** This file contains the implementation of the functions interfacing IBR-DTN
 ** (i.e. that will actually call the IBR-DTN API).
 ********************************************************/

/*
 * al_bp_ibr.cpp
 *
 * Implementation of the functions interfacing the IBR-DTN API
 * Needs compiling with g++
 *
 */

#include "al_bp_ibr.h"

/*
 * if there is the IBR-DTN implementation on the
 * machine the functions are actually implemented; 
 * otherwise they are just dummy functions
 * to avoid compilation errors
 */
#ifdef IBRDTN_IMPLEMENTATION

#include "ibrdtn/api/Client.h"
#include "ibrdtn/data/StatusReportBlock.h" //AdministrativeBlock, StatusReportBlock
#include "ibrdtn/utils/Clock.h" //Clock::getTime();

#include <cstdlib>
#include <cstring>
#include <sstream> //istringstream, ostringstream
#include <unistd.h> //access, unlink
#include <ctime> //time
#include <cctype> //isdigit

#define PRINT_EXCEPTIONS

//private structure holding objects for an open-register-unregister-close session
struct IbrHandle {
	char* daemonIP;
	int daemonPort;
	ibrcommon::socketstream* stream;
	dtn::api::Client* client;
	al_bp_error_t error;
};


al_bp_error_t bp_ibr_errno(al_bp_handle_t handle)
{
	IbrHandle* ibrHandle = (IbrHandle*) handle;
	if (ibrHandle == NULL)
		return BP_ENULLPNTR;
	
	return ibrHandle->error;
}

al_bp_error_t bp_ibr_open(al_bp_handle_t* handle_p) //nonnull: parameters marked "nonnull" are checked for NULL in al_bp_api.c
{
	return bp_ibr_open_with_IP("localhost", 4550, handle_p);
}

al_bp_error_t bp_ibr_open_with_IP(const char* daemon_api_IP, 
                                  int daemon_api_port, 
                                  al_bp_handle_t* handle_p) //nonnull
{
	if (daemon_api_IP == NULL)
		return BP_ENULLPNTR;
	
	ibrcommon::vaddress vaddress(daemon_api_IP, daemon_api_port);
	
	ibrcommon::tcpsocket* socket_p = new ibrcommon::tcpsocket(vaddress);
	//opened when socketstream is constructed,
	//closed when socketstream is closed,
	//deleted when socketstream is deleted (by ibrcommon::vsocket)
	
	ibrcommon::socketstream* stream;
	try {
		stream = new ibrcommon::socketstream(socket_p);
		
	} catch (const ibrcommon::socket_exception& e) {
#ifdef PRINT_EXCEPTIONS
		std::cerr << e.what() << std::endl;
		//"111: Connection refused" / "getaddrinfo(): Name or service not known"
#endif
		
		//because socketstream's constructor threw, its destructor was not called
		socket_p->down(); //unnecessary, called by the destructor
		delete socket_p;
		
		*handle_p = NULL;
		return BP_ECONNECT;
	}
	
	IbrHandle* ibrHandle = new IbrHandle();
	
	ibrHandle->daemonIP = new char[strlen(daemon_api_IP) + 1];
	strcpy(ibrHandle->daemonIP, daemon_api_IP);
	ibrHandle->daemonPort = daemon_api_port;
	
	ibrHandle->stream = stream;
	
	//remember that al_bp_handle_t is just a pointer: typedef int* al_bp_handle_t;
	//then from now on we will use al_bp_handle_t as IbrHandle* instead of int*
	*handle_p = (al_bp_handle_t) ibrHandle;
	
	return ibrHandle->error = BP_SUCCESS;
}

char* get_local_node(IbrHandle* ibrHandle) //nonnull
{
	if (ibrHandle->daemonIP == NULL || ibrHandle->daemonPort <= 0)
		return NULL;
	
	static char local_node[256]; //initialized to zero because it's static
	
	if (strlen(local_node) > 0)
		return local_node;
	
	ibrcommon::vaddress vaddress1(ibrHandle->daemonIP, ibrHandle->daemonPort);
	ibrcommon::tcpsocket* socket_p1 = new ibrcommon::tcpsocket(vaddress1); 
	ibrcommon::socketstream* stream_p1;
	try {
		stream_p1 = new ibrcommon::socketstream(socket_p1);
	} catch (ibrcommon::socket_exception& e) {
#ifdef PRINT_EXCEPTIONS
		std::cerr << e.what() << std::endl;
#endif
		delete socket_p1;
		return NULL;
	}
	
	static const char* const sender = "1073741823"; //a numeric demux token works for both the DTN scheme and the CBHE scheme
	static const char* const receiver = "1073741824";
	static const char* const group = "dtn://bp_ibr_build_local_eid_group_node/bp_ibr_build_local_eid_group";
	
	dtn::api::Client client1(sender, *stream_p1);
	client1.connect();
	
	dtn::data::Bundle bundle;
	bundle.destination = EID(group);
	bundle.lifetime = 1;
	
	ibrcommon::BLOB::Reference blob = ibrcommon::BLOB::create();
	(*blob.iostream()) << ""; //dummy payload, needed
	bundle.push_back(blob);
	
	client1 << bundle;
	
	client1.close();
	stream_p1->close();
	
	
	ibrcommon::vaddress vaddress2(ibrHandle->daemonIP, ibrHandle->daemonPort);
	ibrcommon::tcpsocket* socket_p2 = new ibrcommon::tcpsocket(vaddress2); 
	ibrcommon::socketstream* stream_p2;
	try {
		stream_p2 = new ibrcommon::socketstream(socket_p2);
	} catch (ibrcommon::socket_exception& e) {
#ifdef PRINT_EXCEPTIONS
		std::cerr << e.what() << std::endl;
#endif
		delete socket_p2;
		return NULL;
	}
	
	dtn::api::Client client2(receiver, EID(group), *stream_p2);
	client2.connect();
	
	try {
		bundle = client2.getBundle(1);
	} catch (dtn::api::ConnectionException& e) {
#ifdef PRINT_EXCEPTIONS
		std::cerr << e.what() << std::endl;
#endif
		delete socket_p2;
		return NULL;
	}
	
	client2.close();
	stream_p2->close();
	
	if (bundle.source.getString().length() + 1 > 256)
		return NULL;
	
	char source[bundle.source.getString().length() + 1];
	strcpy(source, bundle.source.getString().c_str());
	
	char* demuxPtr = strstr(source, sender);
	if (demuxPtr == NULL)
		return NULL;
	--demuxPtr; //. or /
	strncpy(local_node, source, demuxPtr - source);
	local_node[demuxPtr - source] = '\0';
	
	return local_node;
}

al_bp_error_t bp_ibr_build_local_eid(al_bp_handle_t handle,
                                     al_bp_endpoint_id_t* local_eid, //nonnull
                                     const char* service_tag,
                                     al_bp_scheme_t type)
{
	IbrHandle* ibrHandle = (IbrHandle*) handle;
	if (ibrHandle == NULL)
		return BP_ENULLPNTR;
	
	const char* const local_node = get_local_node(ibrHandle);
	if (local_node == NULL)
		return ibrHandle->error = BP_EBUILDEID;
	
	if (type == DTN_SCHEME) {
		
		strcpy(local_eid->uri, local_node);
		
		if (service_tag != NULL && strlen(service_tag) > 0) {
			if (service_tag[0] != '/')
				strcat(local_eid->uri, "/");
			
			strncat(local_eid->uri, service_tag, AL_BP_MAX_ENDPOINT_ID - strlen(local_eid->uri) - 1);
		
		} else { //random service_tag
			strcat(local_eid->uri, "/");
		
			static bool srand_called = false;
			if (!srand_called) {
				srand(time(NULL));
				srand_called = true;
			}
		
			uint32_t initial_len = strlen(local_eid->uri);
			for (uint32_t i = 0; i < 16; ++i)
				local_eid->uri[initial_len + i] = "abcdefghijklmnopqrstuvwxyz"[rand() % 26];
			local_eid->uri[initial_len + 16] = '\0';
			
			//NOTE if we left local_eid->uri empty, with "local_eid->uri[0] = '\0';",
			//upon registration the daemon would assign a random 16-letter long tag
			//with both capital and lower case letters;
			//since capital letters could cause incompatibilities with DTN2,
			//we choose to generate a random tag ourselves
		}
		
		return ibrHandle->error = BP_SUCCESS;
		
	} else if (type == CBHE_SCHEME) {
		if (strcmp(service_tag, ".") == 0)
			return ibrHandle->error = BP_EBUILDEID;
		
		for (uint32_t i = 0; i < strlen(service_tag) && i < AL_BP_MAX_ENDPOINT_ID; ++i)
			if (!isdigit(service_tag[i]) && !(i == 0 && service_tag[i] == '.'))
				return ibrHandle->error = BP_EBUILDEID;		
		
		strcpy(local_eid->uri, local_node);
		if (service_tag[0] != '.')
			strcat(local_eid->uri, ".");
		
		strncat(local_eid->uri, service_tag, AL_BP_MAX_ENDPOINT_ID - strlen(local_eid->uri) - 1);
		
		return ibrHandle->error = BP_SUCCESS;
		
	} else 
		return ibrHandle->error = BP_EINVAL;
}

al_bp_error_t bp_ibr_register(al_bp_handle_t handle,
                              al_bp_reg_info_t* reginfo, //nonnull
                              al_bp_reg_id_t* newregid) //nonnull
{
	IbrHandle* ibrHandle = (IbrHandle*) handle;
	if (ibrHandle == NULL)
		return BP_ENULLPNTR;
	if (ibrHandle->stream == NULL)
		return ibrHandle->error = BP_ENULLPNTR;	

	const char* uri = reginfo->endpoint.uri;
	
	const char* const local_node = get_local_node(ibrHandle);
	if (local_node != NULL && strstr(uri, local_node) == uri)
		uri += strlen(local_node);
	
	if (uri[0] == '/' || uri[0] == '.')
		++uri;
	
	dtn::api::Client* client = new dtn::api::Client(std::string(uri), *(ibrHandle->stream));
	//NOTE the other constructor, Client(const std::string &app, const dtn::data::EID &group, ibrcommon::socketstream &stream),
	//allows to receive bundles addressed to both the local EID in the form dtn://<hostname>/<app> (also used as source EID) 
	//and the group EID, as long as it contains a colon ":" and the first colon is not the first or the last character (tested)
	
	client->connect();
	
	ibrHandle->client = client;
	
	if (!reginfo->init_passive) {
		//if needed, implement callback by overriding dtn::api::Client::received(const dtn::data::Bundle &b)
	}
	
	*newregid = 0; //don't know what this is
	
	return ibrHandle->error = BP_SUCCESS;
}

al_bp_error_t bp_ibr_find_registration(al_bp_handle_t handle,
                                       al_bp_endpoint_id_t* eid, //nonnull
                                       al_bp_reg_id_t* newregid)
{
	IbrHandle* ibrHandle = (IbrHandle*) handle;
	if (ibrHandle == NULL)
		return BP_ENULLPNTR;
	
	//NOTE this function cannot be implemented using IBR-DTN's APIs,
	//but since it is used to check if a registration already exists, 
	//make it return false (error) as IBR allows multiple registrations of the same EID
	return ibrHandle->error = BP_ENOTFOUND;
}

al_bp_error_t bp_ibr_send(al_bp_handle_t handle,
                          al_bp_reg_id_t regid, //unused
                          al_bp_bundle_spec_t* spec, //nonnull
                          al_bp_bundle_payload_t* payload, //nonnull
                          al_bp_bundle_id_t* id) //nonnull
{
	IbrHandle* ibrHandle = (IbrHandle*) handle;
	if (ibrHandle == NULL)
		return BP_ENULLPNTR;
	if (ibrHandle->client == NULL)
		return ibrHandle->error = BP_ENULLPNTR;
	
#if 0
	clock_t start = clock();
#endif
	dtn::data::Bundle bundle;
	//bundle.timestamp and bundle.sequencenumber are set now by the constructor
	
	bundle.source = EID(std::string(spec->source.uri));
	bundle.destination = EID(std::string(spec->dest.uri));	
	//I assume spec->replyto is the report-to EID described in RFC5050
	bundle.reportto = EID(std::string(spec->replyto.uri));
	
	bundle.setPriority((dtn::data::PrimaryBlock::PRIORITY) spec->priority.priority);
	
	if (spec->dopts & BP_DOPTS_CUSTODY)
		bundle.set(dtn::data::PrimaryBlock::CUSTODY_REQUESTED, true);
	if (spec->dopts & BP_DOPTS_DELIVERY_RCPT)
		bundle.set(dtn::data::PrimaryBlock::REQUEST_REPORT_OF_BUNDLE_DELIVERY, true);
	if (spec->dopts & BP_DOPTS_RECEIVE_RCPT)
		bundle.set(dtn::data::PrimaryBlock::REQUEST_REPORT_OF_BUNDLE_RECEPTION, true);
	if (spec->dopts & BP_DOPTS_FORWARD_RCPT)
		bundle.set(dtn::data::PrimaryBlock::REQUEST_REPORT_OF_BUNDLE_FORWARDING, true);
	if (spec->dopts & BP_DOPTS_CUSTODY_RCPT)
		bundle.set(dtn::data::PrimaryBlock::REQUEST_REPORT_OF_CUSTODY_ACCEPTANCE, true);
	if (spec->dopts & BP_DOPTS_DELETE_RCPT)
		bundle.set(dtn::data::PrimaryBlock::REQUEST_REPORT_OF_BUNDLE_DELETION, true);
	if (spec->dopts & BP_DOPTS_SINGLETON_DEST)
		bundle.set(dtn::data::PrimaryBlock::DESTINATION_IS_SINGLETON, true);
	if (spec->dopts & BP_DOPTS_MULTINODE_DEST)
		; //no equivalent in RFC5050 or IBR-DTN's APIs
	if (spec->dopts & BP_DOPTS_DO_NOT_FRAGMENT)
		bundle.set(dtn::data::PrimaryBlock::DONT_FRAGMENT, true);
	
	bundle.lifetime = spec->expiration;
	
	//return the bundle's "identity" to the caller
	spec->creation_ts.secs = bundle.timestamp.get(); //bundle.timestamp = spec->creation_ts.secs;
	spec->creation_ts.seqno = bundle.sequencenumber.get(); //bundle.sequencenumber = spec->creation_ts.seqno;
	//no need for spec->delivery_regid in IBR-DTN's API
	
	memset(id, 0, sizeof(*id));
	id->source.uri[0] = '\0'; //no way to retrieve the source EID
	id->creation_ts.secs = bundle.timestamp.get(); //NOTE could differ from the actual timestamp of the bundle sent
	id->creation_ts.seqno = bundle.sequencenumber.get(); //NOTE will almost surely differ from the actual sequence number
	//id->frag_offset and id->orig_length are set later, even though they are not needed
	
	//-------- spec->blocks --------
	
	//dtn::data::ExtensionBlock::Factory has a protected constructor and 
	//dtn::data::ExtensionBlock::Factory::get((uint8_t) spec->blocks.blocks_val[block_index].type);
	//always produces a "Factory not available" ibrcommon::Exception
	class ExtensionBlockFactory : public dtn::data::ExtensionBlock::Factory
	{
	public:
		ExtensionBlockFactory(block_t type)
			: dtn::data::ExtensionBlock::Factory(type), _type(type) {}
			
		virtual dtn::data::Block* create() {
			dtn::data::ExtensionBlock* extensionBlock = new dtn::data::ExtensionBlock();
			extensionBlock->setType(_type);
			return extensionBlock;
		}
	private:
		block_t _type;
	};
	
	for (uint32_t block_index = 0; block_index < spec->blocks.blocks_len; ++block_index) {
		dtn::data::ExtensionBlock::Factory* extblockFactory;
		//pointer needed because ExtensionBlock::Factory is abstract
		
		extblockFactory = new ExtensionBlockFactory((uint8_t) spec->blocks.blocks_val[block_index].type);
		
		// try {
			// extblockFactory = &dtn::data::ExtensionBlock::Factory::get(
				// (uint8_t) spec->blocks.blocks_val[block_index].type);
			
		// } catch (ibrcommon::Exception& e) { //always thrown
// #ifdef PRINT_EXCEPTIONS
			// std::cerr << e.what() << std::endl;
			// //"Factory not available"
// #endif
			// return BP_ESEND;
		// }
		
		dtn::data::Block* extblock = &bundle.push_back(*extblockFactory); 
		//pointer needed because Block is abstract
			
		if (spec->blocks.blocks_val[block_index].flags & (1 << 0))
			extblock->set(dtn::data::Block::REPLICATE_IN_EVERY_FRAGMENT, true);
		if (spec->blocks.blocks_val[block_index].flags & (1 << 1))
			extblock->set(dtn::data::Block::TRANSMIT_STATUSREPORT_IF_NOT_PROCESSED, true);
		if (spec->blocks.blocks_val[block_index].flags & (1 << 2))
			extblock->set(dtn::data::Block::DELETE_BUNDLE_IF_NOT_PROCESSED, true);
		if (spec->blocks.blocks_val[block_index].flags & (1 << 3)) //set automatically if needed
			; //extblock->set(dtn::data::Block::LAST_BLOCK, true); 
		if (spec->blocks.blocks_val[block_index].flags & (1 << 4))
			extblock->set(dtn::data::Block::DISCARD_IF_NOT_PROCESSED, true);
		if (spec->blocks.blocks_val[block_index].flags & (1 << 5))
			extblock->set(dtn::data::Block::FORWARDED_WITHOUT_PROCESSED, true);
		if (spec->blocks.blocks_val[block_index].flags & (1 << 6))
			extblock->set(dtn::data::Block::BLOCK_CONTAINS_EIDS, true);
		
		std::istringstream memin;
		memin.rdbuf()->pubsetbuf(spec->blocks.blocks_val[block_index].data.data_val,
		                         spec->blocks.blocks_val[block_index].data.data_len);
		
		extblock->deserialize(memin, spec->blocks.blocks_val[block_index].data.data_len);
	}
	
	for (uint32_t block_index = 0; block_index < spec->metadata.metadata_len; ++block_index) {
		dtn::data::ExtensionBlock::Factory* extblockFactory;
		
		extblockFactory = new ExtensionBlockFactory((uint8_t) spec->metadata.metadata_val[block_index].type);
		
		// try {
			// extblockFactory = &dtn::data::ExtensionBlock::Factory::get(
					// (uint8_t) spec->metadata.metadata_val[block_index].type); 
			
		// } catch (ibrcommon::Exception& e) { //always thrown
// #ifdef PRINT_EXCEPTIONS
			// std::cerr << e.what() << std::endl;
			// //"Factory not available"
// #endif
			// return ibrHandle->error = BP_ESEND;
		// }
		
		dtn::data::Block* extblock = &bundle.push_back(*extblockFactory);
			
		if (spec->metadata.metadata_val[block_index].flags & (1 << 0))
			extblock->set(dtn::data::Block::REPLICATE_IN_EVERY_FRAGMENT, true);
		if (spec->metadata.metadata_val[block_index].flags & (1 << 1))
			extblock->set(dtn::data::Block::TRANSMIT_STATUSREPORT_IF_NOT_PROCESSED, true);
		if (spec->metadata.metadata_val[block_index].flags & (1 << 2))
			extblock->set(dtn::data::Block::DELETE_BUNDLE_IF_NOT_PROCESSED, true);
		if (spec->metadata.metadata_val[block_index].flags & (1 << 3))
			; //extblock->set(dtn::data::Block::LAST_BLOCK, true);
		if (spec->metadata.metadata_val[block_index].flags & (1 << 4))
			extblock->set(dtn::data::Block::DISCARD_IF_NOT_PROCESSED, true);
		if (spec->metadata.metadata_val[block_index].flags & (1 << 5))
			extblock->set(dtn::data::Block::FORWARDED_WITHOUT_PROCESSED, true);
		if (spec->metadata.metadata_val[block_index].flags & (1 << 6))
			extblock->set(dtn::data::Block::BLOCK_CONTAINS_EIDS, true);
		
		std::istringstream memin;
		memin.rdbuf()->pubsetbuf(spec->metadata.metadata_val[block_index].data.data_val,
		                         spec->metadata.metadata_val[block_index].data.data_len);
		
		extblock->deserialize(memin, spec->metadata.metadata_val[block_index].data.data_len);
	}
	
	//-------- al_bp_bundle_payload_t* payload --------
	
	if (payload->location == BP_PAYLOAD_MEM) {
		if (payload->buf.buf_val == NULL)
			return ibrHandle->error = BP_ENULLPNTR;
		
		ibrcommon::BLOB::Reference blobRef = ibrcommon::BLOB::create();
		
		blobRef.iostream()->rdbuf()->pubsetbuf(payload->buf.buf_val, payload->buf.buf_len);
		
		bundle.push_back(blobRef);
		
		//actually, these fields are "present only if the Fragment flag 
		//in the block’s processing flags byte is set to 1" (RFC5050)
		id->frag_offset = 0;
		id->orig_length = payload->buf.buf_len;
		
	} else if (payload->location == BP_PAYLOAD_FILE || payload->location == BP_PAYLOAD_TEMP_FILE) {
		if (payload->filename.filename_val == NULL)
			return ibrHandle->error = BP_ENULLPNTR;
		
		try {
			ibrcommon::BLOB::Reference blobRef = ibrcommon::BLOB::open(
					ibrcommon::File(std::string(payload->filename.filename_val)));
			
			bundle.push_back(blobRef);
			
			id->frag_offset = 0;
			id->orig_length = blobRef.size();
		
		} catch (ibrcommon::CanNotOpenFileException& e) {
#ifdef PRINT_EXCEPTIONS
			std::cerr << e.what() << std::endl;
			//filename
#endif
			return ibrHandle->error = BP_ESEND;
		}
		
	} else 
		return ibrHandle->error = BP_EINVAL;
	
	//NOTE al_bp_bundle_status_report_t* payload->status_report is not used because it should reflect
	//the content of the payload itself, since "Administrative records are standard application data units" (RFC5050)
	
	ibrHandle->client->operator<<(bundle);	
#if 0
	clock_t end = clock();
	double elapsed = ((double) (end - start)) / CLOCKS_PER_SEC;
	std::cerr << "Elapsed time between bundle creation and sending: " << elapsed << std::endl;
	//up to 0.2 seconds for 50 MB
#endif

	//return an almost certainly correct timestamp and a fake yet consistent sequence number
	// id->creation_ts.secs = dtn::utils::Clock::getTime().get();
	dtn::data::Bundle bundle2;
	id->creation_ts.secs = bundle2.timestamp.get();
	id->creation_ts.seqno = bundle2.sequencenumber.get();
	
	return ibrHandle->error = BP_SUCCESS;
}

al_bp_error_t bp_ibr_recv(al_bp_handle_t handle,
                          al_bp_bundle_spec_t* spec, //nonnull
                          al_bp_bundle_payload_location_t location,
                          al_bp_bundle_payload_t* payload, //nonnull
                          al_bp_timeval_t timeout) //milliseconds
{
	IbrHandle* ibrHandle = (IbrHandle*) handle;
	if (ibrHandle == NULL)
		return BP_ENULLPNTR;
	if (ibrHandle->client == NULL)
		return ibrHandle->error = BP_ENULLPNTR;
	
	dtn::data::Bundle bundle;
	
	try {
		//NOTE unfortunately, dtn::api::Client::getBundle only accepts a timeout in seconds
		//and a zero timeout means an infinite wait, so the shortest possible timeout is one second!
		
		if (timeout == 0) //no wait
			timeout = 1; //(1+999)/1000 = 1s
		else if (timeout == (uint32_t)-1) //infinite wait
			timeout = 0; //(0+999)/1000 = 0 -> inf
		bundle = ibrHandle->client->getBundle((timeout + 999) / 1000); //to seconds and ceiling
	
	} catch (dtn::api::ConnectionTimeoutException& e) {
#ifdef PRINT_EXCEPTIONS
		std::cerr << e.what() << std::endl;
		//"Timeout."
#endif
		return ibrHandle->error = BP_ETIMEOUT;
		
	} catch (dtn::api::ConnectionAbortedException& e) { // not experienced yet
#ifdef PRINT_EXCEPTIONS
		std::cerr << e.what() << std::endl;
		//"Aborted."
#endif
		return ibrHandle->error = BP_ERECVINT;
		
	} catch (dtn::api::ConnectionException& e) { // not experienced yet
#ifdef PRINT_EXCEPTIONS
		std::cerr << e.what() << std::endl;
		//"A connection error occurred."
#endif
		return ibrHandle->error = BP_ERECV;
	}
	
	//-------- al_bp_bundle_spec_t* spec --------
	
	memset(spec, 0, sizeof(*spec));
	
	memcpy(spec->source.uri, bundle.source.getString().c_str(), AL_BP_MAX_ENDPOINT_ID);
	spec->source.uri[AL_BP_MAX_ENDPOINT_ID - 1] = '\0'; //ensures the string terminator
	
	memcpy(spec->dest.uri, bundle.destination.getString().c_str(), AL_BP_MAX_ENDPOINT_ID);
	spec->dest.uri[AL_BP_MAX_ENDPOINT_ID - 1] = '\0';
	
	//I assume spec->replyto is the report-to EID described in RFC5050...
	memcpy(spec->replyto.uri, bundle.reportto.getString().c_str(), AL_BP_MAX_ENDPOINT_ID);
	spec->replyto.uri[AL_BP_MAX_ENDPOINT_ID - 1] = '\0';
	//...otherwise use
	// strcpy(spec->replyto.uri, "dtn:none"); //as per al_bp_ion.c	
	
	spec->priority.priority = (al_bp_bundle_priority_enum) bundle.getPriority();
	spec->priority.ordinal = 0; //as per al_bp_dtn_conversions.c, ION-related
	
	uint32_t dopts = BP_DOPTS_NONE;
	if (bundle.get(dtn::data::PrimaryBlock::CUSTODY_REQUESTED))
		dopts |= BP_DOPTS_CUSTODY;
	if (bundle.get(dtn::data::PrimaryBlock::REQUEST_REPORT_OF_BUNDLE_DELIVERY))
		dopts |= BP_DOPTS_DELIVERY_RCPT;
	if (bundle.get(dtn::data::PrimaryBlock::REQUEST_REPORT_OF_BUNDLE_RECEPTION))
		dopts |= BP_DOPTS_RECEIVE_RCPT;
	if (bundle.get(dtn::data::PrimaryBlock::REQUEST_REPORT_OF_BUNDLE_FORWARDING))
		dopts |= BP_DOPTS_FORWARD_RCPT;
	if (bundle.get(dtn::data::PrimaryBlock::REQUEST_REPORT_OF_CUSTODY_ACCEPTANCE))
		dopts |= BP_DOPTS_CUSTODY_RCPT;
	if (bundle.get(dtn::data::PrimaryBlock::REQUEST_REPORT_OF_BUNDLE_DELETION))
		dopts |= BP_DOPTS_DELETE_RCPT;
	if (bundle.get(dtn::data::PrimaryBlock::DESTINATION_IS_SINGLETON))
		dopts |= BP_DOPTS_SINGLETON_DEST;
	if (false) //no equivalent in RFC5050 or IBR-DTN's APIs
		dopts |= BP_DOPTS_MULTINODE_DEST;
	if (bundle.get(dtn::data::PrimaryBlock::DONT_FRAGMENT))
		dopts |= BP_DOPTS_DO_NOT_FRAGMENT;
	//al_bp_bundle_delivery_opts_t is missing some RFC5050's Bundle Processing Control Flags	
	spec->dopts = (al_bp_bundle_delivery_opts_t) dopts;
	
	spec->expiration = bundle.lifetime.get();
	
	spec->creation_ts.secs = bundle.timestamp.get();
	spec->creation_ts.seqno = bundle.sequencenumber.get();
	
	spec->delivery_regid = 0; //again, no regid support in IBR-DTN's API
	
	spec->unreliable = 0; //ION-related
	spec->critical = 0; //ION-related
	spec->flow_label = 0; //ION-related
	
	//-------- spec->blocks --------
	
	spec->blocks.blocks_len = 0;
	for (dtn::data::Bundle::iterator block_ptr_it = bundle.begin(); block_ptr_it != bundle.end(); ++block_ptr_it) {
		// *block_ptr_it is a refcnt_ptr
		if ((*block_ptr_it)->getType() != PayloadBlock::BLOCK_TYPE) {			
			++ spec->blocks.blocks_len;			
		}
	}
	
	spec->blocks.blocks_val = new al_bp_extension_block_t[spec->blocks.blocks_len];
	
	//NOTE blocks with type 2, 3 and 4 give different kinds of problems,
	//probably because they have special meanings and thus are treated unusually
	
	uint32_t block_index = 0;
	for (dtn::data::Bundle::iterator block_ptr_it = bundle.begin(); block_ptr_it != bundle.end(); ++block_ptr_it) {
		if ((*block_ptr_it)->getType() != PayloadBlock::BLOCK_TYPE) {
		
			spec->blocks.blocks_val[block_index].type = (*block_ptr_it)->getType();
			spec->blocks.blocks_val[block_index].flags = (*block_ptr_it)->getProcessingFlags().get();
			
			spec->blocks.blocks_val[block_index].data.data_len = (*block_ptr_it)->getLength();			
			spec->blocks.blocks_val[block_index].data.data_val = new char[spec->blocks.blocks_val[block_index].data.data_len];
			
			std::ostringstream memout;
			memout.rdbuf()->pubsetbuf(spec->blocks.blocks_val[block_index].data.data_val,
			                          spec->blocks.blocks_val[block_index].data.data_len);
			
			size_t length = 0;
			(*block_ptr_it)->serialize(memout, length); 
			//might (PayloadBlock) or might not (ExtensionBlock) increase the second arg (length)
			
			++block_index;
		}
	}
	
	spec->metadata.metadata_len = 0;
	spec->metadata.metadata_val = NULL;
	
	//-------- al_bp_bundle_payload_t* payload --------
	
	memset(payload, 0, sizeof(*payload));
	
	for (dtn::data::Bundle::iterator block_ptr_it = bundle.begin(); block_ptr_it != bundle.end(); ++block_ptr_it) {
		if ((*block_ptr_it)->getType() == PayloadBlock::BLOCK_TYPE) {
			
			
			if (location == BP_PAYLOAD_MEM) {
				payload->location = BP_PAYLOAD_MEM;
				
				memset(&payload->filename, 0, sizeof(payload->filename));
				
				payload->buf.buf_len = (*block_ptr_it)->getLength();
				payload->buf.buf_val = new char[payload->buf.buf_len];
				
				std::ostringstream memout;
				memout.rdbuf()->pubsetbuf(payload->buf.buf_val,
				                          payload->buf.buf_len);
				
				size_t length;
				(*block_ptr_it)->serialize(memout, length);
			
				payload->buf.buf_crc = 0;
				//NOTE is payload->buf.buf_crc actually wanted? besides, it was clearly added later
				
			} else if (location == BP_PAYLOAD_FILE || location == BP_PAYLOAD_TEMP_FILE) {
				payload->location = BP_PAYLOAD_FILE;
				
				memset(&payload->buf, 0, sizeof(payload->buf));
				
				std::stringstream filename;
				uint32_t i = 0;
				while (true) {
					filename.str("");
					filename << "/tmp/ibrdtn_payload_" << i;
					if (access(filename.str().c_str(), F_OK) != 0)
						break;
					++i;
				}
				payload->filename.filename_len = filename.str().length();
				payload->filename.filename_val = new char[filename.str().length() + 1];
				strcpy(payload->filename.filename_val, filename.str().c_str());
				
				ofstream fileout(payload->filename.filename_val);
				
				size_t length = 0;
				(*block_ptr_it)->serialize(fileout, length);
				
				fileout.close();		
				
			} else {
				bp_ibr_free_extension_blocks(spec);
				bp_ibr_free_metadata_blocks(spec);
				return ibrHandle->error = BP_EINVAL;
			}
			
			//-------- al_bp_bundle_status_report_t* payload->status_report --------
			
			if (bundle.get(dtn::data::PrimaryBlock::APPDATA_IS_ADMRECORD)) {
				StatusReportBlock srblock;
				
				try {
					srblock.read(*((dtn::data::PayloadBlock*) &(**block_ptr_it))); //downcast from Block to PayloadBlock (trust me)
					
					payload->status_report = new al_bp_bundle_status_report_t;
					memset(payload->status_report, 0, sizeof(*payload->status_report));
					
					memcpy(payload->status_report->bundle_id.source.uri, srblock.bundleid.source.getString().c_str(), AL_BP_MAX_ENDPOINT_ID);
					payload->status_report->bundle_id.source.uri[AL_BP_MAX_ENDPOINT_ID - 1] = '\0';
					
					payload->status_report->bundle_id.creation_ts.secs = srblock.bundleid.timestamp.get();
					payload->status_report->bundle_id.creation_ts.seqno = srblock.bundleid.sequencenumber.get();
					
					if (srblock.bundleid.isFragment()) {
						payload->status_report->bundle_id.frag_offset = srblock.bundleid.fragmentoffset.get();
						
						//it would be the original length if we were in the Primary Block;
						//in an Administrative Record it's the fragment length
						payload->status_report->bundle_id.orig_length = srblock.bundleid.getPayloadLength();
					}
					
					payload->status_report->flags = (al_bp_status_report_flags_t) srblock.status;
					payload->status_report->reason = (al_bp_status_report_reason_t) srblock.reasoncode;
					
					if (payload->status_report->flags & BP_STATUS_RECEIVED)
						payload->status_report->receipt_ts.secs = srblock.timeof_receipt.getTimestamp().get();
					if (payload->status_report->flags & BP_STATUS_CUSTODY_ACCEPTED)
						payload->status_report->custody_ts.secs = srblock.timeof_custodyaccept.getTimestamp().get();
					if (payload->status_report->flags & BP_STATUS_FORWARDED)
						payload->status_report->forwarding_ts.secs = srblock.timeof_forwarding.getTimestamp().get();
					if (payload->status_report->flags & BP_STATUS_DELIVERED)
						payload->status_report->delivery_ts.secs = srblock.timeof_delivery.getTimestamp().get();
					if (payload->status_report->flags & BP_STATUS_DELETED)
						payload->status_report->deletion_ts.secs = srblock.timeof_deletion.getTimestamp().get();
					if (payload->status_report->flags & BP_STATUS_ACKED_BY_APP)
						payload->status_report->ack_by_app_ts.secs = 0; //not RFC5050 compliant and not supported by IBR-DTN
					
				} catch (dtn::data::AdministrativeBlock::WrongRecordException& e) { //this AdministrativeBlock is a CustodySignalBlock
#ifdef PRINT_EXCEPTIONS
					std::cerr << e.what() << std::endl;
					//"This administrative block is not of the expected type."
#endif
				}
				
			} else {
				payload->status_report = NULL;
			}
			
			
			//"At most one of the blocks in the sequence may be a payload block" (RFC5050)
			break;
		}
	}
	
	return ibrHandle->error = BP_SUCCESS;
}

//it isn't clear whether al_bp_unregister is always called before or after al_bp_close, 
//and sometimes it appears not to be called at all,
//even though one would expect the former to be always called and always before the latter, 
//and the bp_ibr code follows that thought;
//thus, we modified bp_ibr_close to call bp_ibr_unregister if this wasn't called
//(and apparently bp_ibr_unregister doesn't throw any error if called (immediately?) after bp_ibr_close,
//but keep in mind that in such case dereferencing ibrHandle with 'ibrHandle->client' is undefined behavior,
//because ibrHandle's pointed data has already been deleted by bp_ibr_close with 'delete ibrHandle')

al_bp_error_t bp_ibr_unregister(al_bp_handle_t handle, 
                                al_bp_reg_id_t regid, //unused
                                al_bp_endpoint_id_t eid) //unused
{
	IbrHandle* ibrHandle = (IbrHandle*) handle;
	if (ibrHandle == NULL)
		return BP_ENULLPNTR;
	
	if (ibrHandle->client == NULL)
		return ibrHandle->error = BP_SUCCESS;
	
	dtn::api::Client* client = ibrHandle->client;

	// client->abort(); //Aborts blocking calls of getBundle()
	client->close();
	
	// client->abort();
	//if Client's destructor is called while it is waiting for a bundle, 
	//for instance through a signal handler, it blocks on _receiver.join();
	//abort() should "Aborts blocking calls of getBundle()"
	
	delete client;
	
	// operator delete (client);
	//NOTE it turns out that abort() isn't enough: let's deallocate client without calling the constructor;
	//it's a really bad practice, but it works for now
	
	ibrHandle->client = NULL;
	
	return ibrHandle->error = BP_SUCCESS;
}

al_bp_error_t bp_ibr_close(al_bp_handle_t handle)
{
	IbrHandle* ibrHandle = (IbrHandle*) handle;
	if (ibrHandle == NULL)
		return BP_ENULLPNTR;
	
	if (ibrHandle->client != NULL)
		bp_ibr_unregister(handle, al_bp_reg_id_t(), al_bp_endpoint_id_t());
	
	if (ibrHandle->stream == NULL)
		return ibrHandle->error = BP_SUCCESS;
	
	ibrcommon::socketstream* stream = ibrHandle->stream;
	stream->close();
	
	delete stream;
	
	// delete stream;
	//NOTE *sometimes*, when bp_ibr_close is called from a signal handler, it causes the error
	//"pure virtual method called   terminate called without an active exception   Aborted (core dumped)"
	//after all, it's not so strange for the asynchronicity of a signal handler to cause problems
	
	ibrHandle->stream = NULL;
	
	delete ibrHandle->daemonIP;
	ibrHandle->daemonIP = NULL;
	ibrHandle->daemonPort = 0;
	
	delete ibrHandle;
	
	return BP_SUCCESS;
}

/* Utility Functions */

al_bp_error_t bp_ibr_parse_eid_string(al_bp_endpoint_id_t* eid, 
                                      const char* str)
{
	if (str == NULL)
		return BP_ENULLPNTR;
	if (eid == NULL)
		return BP_ENULLPNTR;
	
	EID ibreid(str);
	//if str is malformed, a "dtn:none" EID is constructed
	
	//copy readonly str and trim it to compare with "dtn:none"
	char str_copy[strlen(str) + 1];
	strcpy(str_copy, str);
	char* trimmed_str = str_copy;
	while (*trimmed_str != '\0' && *trimmed_str <= ' ')
		++trimmed_str;
	while (*trimmed_str != '\0' && trimmed_str[strlen(trimmed_str) - 1] <= ' ')
		trimmed_str[strlen(trimmed_str) - 1] = '\0';
	
	if (strcmp(trimmed_str, "dtn:none") != 0 && strcmp(ibreid.getString().c_str(), "dtn:none") == 0)
		return BP_EPARSEEID;
	
	if (ibreid.getString().length() >= AL_BP_MAX_ENDPOINT_ID)
		return BP_ESIZE;
	
	strcpy(eid->uri, ibreid.getString().c_str());
	
	return BP_SUCCESS;
}

void bp_ibr_copy_eid(al_bp_endpoint_id_t* dst, 
                     al_bp_endpoint_id_t* src) 
{
	//I don't understand why this function is duplicated for each DTN implementation,
	//nor the need for the conversions in the other two implementations
	
	if (dst == NULL || src == NULL)
		return;
	
	strncpy(dst->uri, src->uri, AL_BP_MAX_ENDPOINT_ID);
	dst->uri[AL_BP_MAX_ENDPOINT_ID - 1] = '\0'; //src->uri might not be terminated and be longer than AL_BP_MAX_ENDPOINT_ID - 1
}

void bp_ibr_free_extension_blocks(al_bp_bundle_spec_t* spec) 
{
	if (spec == NULL)
		return;
	if (spec->blocks.blocks_val == NULL) {
		spec->blocks.blocks_len = 0;
		return;
	}
	
	for (uint32_t i = 0; i < spec->blocks.blocks_len; ++i) {
		delete spec->blocks.blocks_val[i].data.data_val;
	}
	
	delete spec->blocks.blocks_val;
    spec->blocks.blocks_val = NULL;
    spec->blocks.blocks_len = 0;
}

void bp_ibr_free_metadata_blocks(al_bp_bundle_spec_t* spec) 
{
	if (spec == NULL)
		return;
	if (spec->metadata.metadata_val == NULL) {
		spec->metadata.metadata_len = 0;
		return;
	}
	
	for (uint32_t i = 0; i < spec->metadata.metadata_len; ++i) {
		delete spec->metadata.metadata_val[i].data.data_val;
	}
	
	delete spec->metadata.metadata_val;
    spec->metadata.metadata_val = NULL;
    spec->metadata.metadata_len = 0;
}

al_bp_error_t bp_ibr_set_payload(al_bp_bundle_payload_t* payload, //nonnull
                                 al_bp_bundle_payload_location_t location,
                                 char* val, int len)
{
	memset(payload, 0, sizeof(*payload));
	
	payload->location = location;
	
	// if (val == NULL)
		// return BP_ENULLPNTR;
	
	if (location == BP_PAYLOAD_MEM) {
		payload->buf.buf_len = len;
		payload->buf.buf_val = val;
		
	} else if (location == BP_PAYLOAD_FILE || location == BP_PAYLOAD_TEMP_FILE) {
		payload->filename.filename_len = len;
		payload->filename.filename_val = val;
		
	} else
		return BP_EINVAL;
		
	return BP_SUCCESS;
}

void bp_ibr_free_payload(al_bp_bundle_payload_t* payload) {
	if (payload == NULL)
		return;
	
	if (payload->filename.filename_val != NULL) {
		if (payload->location == BP_PAYLOAD_FILE || payload->location == BP_PAYLOAD_TEMP_FILE) {
			unlink(payload->filename.filename_val);
		}
		
		// delete payload->filename.filename_val; //causes "Error in `./dtnperf_vIBRDTN': double free or corruption (fasttop)"
		payload->filename.filename_val = NULL;
		payload->filename.filename_len = 0;
	}
	
	if (payload->buf.buf_val != NULL) {
		// delete payload->buf.buf_val; //already freed by DTNperf
		payload->buf.buf_val = NULL;
		payload->buf.buf_len = 0;
		payload->buf.buf_crc = 0;
	}
	
	if (payload->status_report != NULL) {
		delete payload->status_report;
		payload->status_report = NULL;
	}
}

#undef PRINT_EXCEPTIONS

#else /* IBRDTN_IMPLEMENTATION */

al_bp_error_t bp_ibr_errno(al_bp_handle_t handle)
{
	return BP_ENOTIMPL;
}

al_bp_error_t bp_ibr_open(al_bp_handle_t* handle_p)
{
	return BP_ENOTIMPL;
}

al_bp_error_t bp_ibr_open_with_IP(const char* daemon_api_IP, 
                                  int daemon_api_port, 
                                  al_bp_handle_t* handle_p)
{
	return BP_ENOTIMPL;
}

al_bp_error_t bp_ibr_build_local_eid(al_bp_handle_t handle,
                                     al_bp_endpoint_id_t* local_eid,
                                     const char* service_tag,
                                     al_bp_scheme_t type)
{
	return BP_ENOTIMPL;
}

al_bp_error_t bp_ibr_register(al_bp_handle_t handle,
                              al_bp_reg_info_t* reginfo,
                              al_bp_reg_id_t* newregid)
{
	return BP_ENOTIMPL;
}

al_bp_error_t bp_ibr_find_registration(al_bp_handle_t handle,
                                       al_bp_endpoint_id_t* eid,
                                       al_bp_reg_id_t* newregid)
{
	return BP_ENOTIMPL;
}

al_bp_error_t bp_ibr_send(al_bp_handle_t handle,
                          al_bp_reg_id_t regid,
                          al_bp_bundle_spec_t* spec,
                          al_bp_bundle_payload_t* payload,
                          al_bp_bundle_id_t* id)
{
	return BP_ENOTIMPL;
}

al_bp_error_t bp_ibr_recv(al_bp_handle_t handle,
                          al_bp_bundle_spec_t* spec,
                          al_bp_bundle_payload_location_t location,
                          al_bp_bundle_payload_t* payload,
                          al_bp_timeval_t timeout)
{
	return BP_ENOTIMPL;
}

al_bp_error_t bp_ibr_unregister(al_bp_handle_t handle, 
                                al_bp_reg_id_t regid,
                                al_bp_endpoint_id_t eid)
{
	return BP_ENOTIMPL;
}

al_bp_error_t bp_ibr_close(al_bp_handle_t handle)
{
	return BP_ENOTIMPL;
}

al_bp_error_t bp_ibr_parse_eid_string(al_bp_endpoint_id_t* eid, 
                                      const char* str)
{
	return BP_ENOTIMPL;
}

void bp_ibr_copy_eid(al_bp_endpoint_id_t* dst, 
                     al_bp_endpoint_id_t* src) {}

void bp_ibr_free_extension_blocks(al_bp_bundle_spec_t* spec) {}

void bp_ibr_free_metadata_blocks(al_bp_bundle_spec_t* spec) {}

al_bp_error_t bp_ibr_set_payload(al_bp_bundle_payload_t* payload,
                                 al_bp_bundle_payload_location_t location,
                                 char* val, int len)
{
	return BP_ENOTIMPL;
}

void bp_ibr_free_payload(al_bp_bundle_payload_t* payload) {}

#endif /* IBRDTN_IMPLEMENTATION */
