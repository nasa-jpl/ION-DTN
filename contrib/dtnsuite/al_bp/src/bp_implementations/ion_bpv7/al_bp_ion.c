/********************************************************
 **  Authors: Michele Rodolfi, michele.rodolfi@studio.unibo.it
 **           Anna d'Amico, anna.damico@studio.unibo.it
 **           Andrea Bisacchi, andrea.bisacchi5@studio.unibo.it
 **           Carlo Caini (DTNperf_3 project supervisor), carlo.caini@unibo.it
 **
 **
 **  Copyright (c) 2013, Alma Mater Studiorum, University of Bologna
 **  All rights reserved.
 ** This file contains the functions interfacing ION
 ** (i.e. that will actually call the ION APIs).
 ********************************************************/

/*
 * al_bp_ion.c
 *
 */

#include "al_bp_ion.h"

#include <bpP.h>

/**
 * bp_open_source() is present only in ION >= 3.3.0
 * If using ION < 3.3.0, please comment the following line.
 */
#define BP_OPEN_SOURCE

/*
 * if there is the ION implementation on the
 * machine the API are implemented
 */
#ifdef ION_IMPLEMENTATION

#include "al_bp_ion_conversions.h"

/* It's for private function */
#include <ion.h>
int albp_parseAdminRecord(int *adminRecordType, BpStatusRpt *rpt, void **otherPtr, Object payload);
int	albp_parseStatusRpt(BpStatusRpt *rpt, unsigned char *cursor,int unparsedBytes, int isFragment);
/*********************************/

al_bp_error_t bp_ion_attach(){
	int result;
	result = bp_attach();
	if(result == -1)
	{
		return BP_EATTACH;
	}
	return BP_SUCCESS;
}

al_bp_error_t bp_ion_open_with_IP(char * daemon_api_IP, int daemon_api_port, al_bp_handle_t * handle)
{
	return BP_ENOTIMPL;
}

al_bp_error_t bp_ion_errno(al_bp_handle_t handle)
{
	printf("%s\n",system_error_msg());
	return BP_SUCCESS;
}

al_bp_error_t bp_ion_build_local_eid(al_bp_endpoint_id_t* local_eid,
								const char* service_tag,
								al_bp_scheme_t  type)
{
	char * eidString, * hostname;
	int result;
	VScheme * scheme;
	PsmAddress psmAddress;
	eidString = (char *)malloc(sizeof(char)*AL_BP_MAX_ENDPOINT_ID);

	if(type == CBHE_SCHEME)
	{
		findScheme(CBHESCHEMENAME,&scheme,&psmAddress);
		if(psmAddress == 0)
		{
			/*Unknow scheme*/
			result = addScheme(CBHESCHEMENAME,"ipnfw","ipnadmin");
			if(result == 0) {
				free(eidString);
				return BP_EBUILDEID;
			}
		}
		long int service_num = strtol(service_tag,NULL,10);
		sprintf(eidString, "%s:%lu.%lu", CBHESCHEMENAME,(unsigned long int) getOwnNodeNbr(), service_num);
		(*local_eid) = ion_al_endpoint_id(eidString);
	}
	else if(type == DTN_SCHEME)
	{
		findScheme(DTN2SCHEMENAME,&scheme,&psmAddress);
		if(psmAddress == 0)
		{
			/*Unknow scheme*/
			result = addScheme(DTN2SCHEMENAME,"dtn2fw","dtn2admin");
			if(result == 0) {
				free(eidString);
				return BP_EBUILDEID;
			}
		}

		hostname = (char *)malloc(sizeof(char)*100);
		result = gethostname(hostname, 100);
		if(result == -1) {
			free(eidString);
			free(hostname);
			return BP_EBUILDEID;
		}
		sprintf(eidString,"%s://%s.dtn%s",DTN2SCHEMENAME,hostname,service_tag);
		(*local_eid) = ion_al_endpoint_id(eidString);
		free(hostname);
	}
	else {
		free(eidString);
		return BP_EBUILDEID;
	}
	//Free resource
	free(eidString);
	return BP_SUCCESS;
}

al_bp_error_t bp_ion_register(al_bp_handle_t * handle,
						al_bp_reg_info_t* reginfo,
						al_bp_reg_id_t* newregid)
{
	int result;
	char * eid;
	al_ion_handle_t ion_handle;

	//ion_handle = al_ion_handle(*handle); //this is useless because *handle is meaningless
	//initialization of ion_handle
	ion_handle = (al_ion_handle_t) malloc(sizeof(al_ion_handle_t));
	ion_handle->recv = (BpSAP *) malloc(sizeof(BpSAP));
	ion_handle->source = (BpSAP *) malloc(sizeof(BpSAP));
	//end of initialization

	eid = al_ion_endpoint_id(reginfo->endpoint);
/*	switch(reginfo->flags)
	{
		case BP_REG_DEFER: rule = EnqueueBundle;break;
		case BP_REG_DROP: rule = DiscardBundle;break;
		default: return BP_EINVAL;
	}*/
	//If the eid is not registrated then it will be registrated
	if(bp_ion_find_registration(*handle,&(reginfo->endpoint),newregid) ==  BP_ENOTFOUND)
	{
		result = addEndpoint(eid, DiscardBundle ,NULL);
		if(result == 0) {
			free(eid);
			return BP_EREG;
		}
	}
#ifdef BP_OPEN_SOURCE
	//result = bp_open(eid, (ion_handle->source));
	result = bp_open_source(eid, (ion_handle->source), 1);
	if(result == -1) {
		free(eid);
		return BP_EREG;
	}
#endif
	result = bp_open(eid, (ion_handle->recv));
	if(result == -1) {
		free(eid);
		return BP_EREG;
	}
	//Free resource
	free(eid);
	(*handle) = ion_al_handle(ion_handle);
	return BP_SUCCESS;
}

al_bp_error_t bp_ion_find_registration(al_bp_handle_t handle,
						al_bp_endpoint_id_t * eid,
						al_bp_reg_id_t * newregid)
{
	char * schemeName, * endpoint;
	VEndpoint * veid;
	PsmAddress psmAddress;
	MetaEid metaEid;
	VScheme * vscheme;
	endpoint = al_ion_endpoint_id((*eid));
	schemeName = (char *)malloc(sizeof(char)*4);
	if(strncmp(endpoint,"ipn",3) == 0)
		strncpy(schemeName,"ipn",4);
	else
		strncpy(schemeName,"dtn",4);
	if(parseEidString(endpoint,&metaEid,&vscheme,&psmAddress) == 0)
	{
		free(schemeName);
		free(endpoint);
		return BP_EPARSEEID;
	}
	findEndpoint(schemeName,metaEid.nss,vscheme,&veid,&psmAddress);
	if(psmAddress == 0)
	{
		free(schemeName);
		free(endpoint);
		return BP_ENOTFOUND;
	}
	if (sm_TaskExists(veid->appPid))
	{
		if (veid->appPid != -1) {
			free(schemeName);
			free(endpoint);
			return BP_EBUSY;
		}
	}
	//Free resource
	free(schemeName);
	free(endpoint);

	return BP_SUCCESS;
}

al_bp_error_t bp_ion_unregister(al_bp_endpoint_id_t eid)
{
	char * ion_eid = al_ion_endpoint_id(eid);
	int result = removeEndpoint(ion_eid);
	free(ion_eid);
	if(result != 1)
	{
		return BP_EUNREG;
	}
	return BP_SUCCESS;

}

al_bp_error_t bp_ion_send(al_bp_handle_t handle,
					al_bp_reg_id_t regid,
					al_bp_bundle_spec_t* spec,
					al_bp_bundle_payload_t* payload,
					al_bp_bundle_id_t* id)
{
	al_ion_handle_t ion_handle = al_ion_handle(handle);
#ifdef BP_OPEN_SOURCE
	// using SAP obtained exclusively for sending bundles
	BpSAP * bpSap = ion_handle->source;
#else
	// using unique SAP for sending/receiving bundles
	BpSAP * bpSap = ion_handle->recv;
#endif
	char * destEid = al_ion_endpoint_id(spec->dest);
	char * reportEid = NULL;
	char * tokenClassOfService;
	int result, tmpCustody, tmpPriority, lifespan, ackRequested;
	unsigned char srrFlags;
	BpCustodySwitch custodySwitch;
	BpAncillaryData extendedCOS = { 0, 0, 0 };
	/* Set option bundle */
	reportEid = al_ion_endpoint_id(spec->replyto);
	lifespan = (int) spec->expiration;
	custodySwitch = NoCustodyRequested;
	srrFlags = al_ion_bundle_srrFlags(spec->dopts);
	ackRequested = 0;
	/* Create String for parse class of service */	
	if(spec->dopts & BP_DOPTS_CUSTODY)
	{
			tmpCustody = 1;
	}
	else
	{
			tmpCustody = 0;
	}
	tmpPriority = al_ion_bundle_priority(spec->priority);
	if(tmpPriority == -1)
		return BP_EINVAL;
	tokenClassOfService = (char *)malloc(sizeof(char)*255);
	sprintf(tokenClassOfService,"%1u.%1u.%lu.%1u.%1u.%lu", tmpCustody, tmpPriority, (unsigned long) spec->priority.ordinal, 
			spec->unreliable==TRUE?1:0, spec->critical==TRUE?1:0, (unsigned long) spec->flow_label);
	
	//printf("COS is: %s\n", tokenClassOfService);
	if (spec->metadata.metadata_len > 0)
	{
		result = al_ion_metadata(spec->metadata.metadata_len, spec->metadata.metadata_val, &extendedCOS);
		if(result == 0) {
			free(destEid);
			free(reportEid);
			free(tokenClassOfService);
			return BP_EINVAL;
		}
	}

	result = bp_parse_quality_of_service(tokenClassOfService,&extendedCOS,&custodySwitch,&tmpPriority);
	if(result == 0) {
		free(destEid);
		free(reportEid);
		free(tokenClassOfService);
		return BP_EINVAL;
	}
	Payload ion_payload = al_ion_bundle_payload((*payload), tmpPriority, extendedCOS);
	Object adu = ion_payload.content;
	Object newBundleObj;

	/* Send Bundle*/
	result = bp_send(*bpSap,destEid,reportEid,lifespan,tmpPriority,
			custodySwitch,srrFlags,ackRequested,&extendedCOS,adu,&newBundleObj);
	if(result == 0) {
		free(destEid);
		free(reportEid);
		free(tokenClassOfService);
		return BP_ENOSPACE;
	}
	if(result == -1) {
		free(destEid);
		free(reportEid);
		free(tokenClassOfService);
		return BP_ESEND;
	}

	/* Set Id Bundle Sent*/
//dz debug --- It is not safe to use the newBundleObj "address" to access SDR because it may
//dz debug --- have been released (or worse: reallocated) before bp_send returns. ION 3.3 should 
//dz debug --- have a new feature bp_open_source() which will preserve the sent bundle until
//dz debug --- it is released or expires. I am commenting out retrieving the dictionary because
//dz debug --- that can lead to allocating 4GB of memory if the newBundleObj data was overwritten.
//dz debug --- This should be updated after the new feature is available. 
//dz debug
	Bundle bundleION;
	Sdr bpSdr = bp_get_sdr();
	sdr_begin_xn(bpSdr);
	sdr_read(bpSdr,(char*)&bundleION,(SdrAddress) newBundleObj,sizeof(Bundle));
	sdr_end_xn(bpSdr);
	char * tmpEidSource;
	readEid(&(bundleION.id.source),&tmpEidSource);
#ifdef BP_OPEN_SOURCE
	id->source = ion_al_endpoint_id(tmpEidSource);
	bp_release((SdrAddress) newBundleObj);
#else
	id->source = ion_al_endpoint_id("<TBD>");  //dz debug
#endif

//dz debug --- Note that the following values may not always be valid by the time they are read here
//dz debug --- but most of the time they should be okay and reading invalid values will not cause 
//dz debug --- a crash. Just setting these values to zeroes prevents the client Window option from working
//dz debug --- because ACKs from the server would never match. This compromise should work until invalid
//dz debug --- values fill up the window.
	id->creation_ts = ion_al_timestamp(bundleION.id.creationTime);
	id->frag_offset = bundleION.id.fragmentOffset;
	id->orig_length = bundleION.totalAduLength;

	handle = ion_al_handle(ion_handle);
	free(destEid);
	free(reportEid);
	free(tokenClassOfService);
	return BP_SUCCESS;
}

al_bp_error_t bp_ion_recv(al_bp_handle_t handle,
					al_bp_bundle_spec_t* spec,
					al_bp_bundle_payload_location_t location,
					al_bp_bundle_payload_t* payload,
					al_bp_timeval_t timeout)
{
	al_ion_handle_t ion_handle = al_ion_handle(handle);
	// using SAP obtained exclusively for receiving bundles
	// or unique SAP if BP_OPEN_SOURCE is undefined
	BpSAP * bpSap = ion_handle->recv;
	BpDelivery dlv;
	int second_timeout = (int) timeout;
	int result;
	result = bp_receive(*bpSap ,&dlv, second_timeout);
	if(result < 0)
	{
		return BP_ERECV;
	}
	if(dlv.result == BpReceptionTimedOut)
	{
		//printf("\nAL-BP: Result Timeout\n");
		return BP_ETIMEOUT;
	}
	if(dlv.result == BpReceptionInterrupted)
	{
		//printf("\nAL-BP: Reception Interrupted\n");
		return BP_ERECVINT;
	}
	if (dlv.result == BpEndpointStopped) {
		return BP_ERECV;
	}
	/* Set Bundle Spec */
	spec->creation_ts = ion_al_timestamp(dlv.bundleCreationTime);
	spec->source = ion_al_endpoint_id(dlv.bundleSourceEid);
	char * tmp = "dtn:none";
	spec->replyto = ion_al_endpoint_id(tmp);
	//Laura Mazzuca: added support to metadata read
	//printf("--------------metada len: %u\n", dlv.metadataLen);
	if (dlv.metadataType) {
		//spec->metadata.metadata_len = 1;
		ion_al_metadata(dlv, spec);
	}
	/* Payload */
	Sdr bpSdr = bp_get_sdr();
	Payload ion_payload;
	ion_payload.content = dlv.adu;
	ion_payload.length = zco_source_data_length(bpSdr, dlv.adu);
	/* File Name if payload is saved in a file */
	char * filename_base = "/tmp/ion_";
	char * filename = (char *) malloc(sizeof(char)*256);
	//check for existing files
	int i = 0;
	boolean_t stop = FALSE;
	while (!stop)
	{
		sprintf(filename, "%s%d_%d", filename_base, getpid(), i);
		if (access(filename,F_OK) != 0)
			// if filename doesn't exist, exit
			stop = TRUE;
		i++;
	}
	(*payload)  = ion_al_bundle_payload(ion_payload,location,filename);
	free(filename);
	/* Status Report */
	BpStatusRpt statusRpt;
	void * acsptr;
	if(albp_parseAdminRecord(&dlv.adminRecord,&statusRpt,&acsptr,dlv.adu) == 1)
	{
		al_bp_bundle_status_report_t bp_statusRpt = ion_al_bundle_status_report(statusRpt);
		if(payload->status_report == NULL)
		{
			payload->status_report = (al_bp_bundle_status_report_t *) malloc(sizeof(al_bp_bundle_status_report_t));
		}
		(*payload->status_report) = bp_statusRpt;
	}

	/* Release Delivery */
	bp_release_delivery(&dlv, 1);

	handle = ion_al_handle(ion_handle);

	return BP_SUCCESS;
}

al_bp_error_t bp_ion_close(al_bp_handle_t handle)
{
	al_ion_handle_t ion_handle = al_ion_handle(handle);
	bp_close(*ion_handle->recv);
	bp_close(*ion_handle->source);
	//free ion_handle
	free(ion_handle->recv);
#ifdef BP_OPEN_SOURCE
	free(ion_handle->source);
#endif
	free(ion_handle);
	handle = ion_al_handle(ion_handle);
	return BP_SUCCESS;
}

void bp_ion_copy_eid(al_bp_endpoint_id_t* dst, al_bp_endpoint_id_t* src)
{
	char * ion_dst;
	char * ion_src;
	int length;
	ion_src = al_ion_endpoint_id((*src));
	length = strlen(ion_src)+1;
	ion_dst = (char *)malloc(sizeof(char)*length);
	memcpy(ion_dst,ion_src,length);
	(*dst) = ion_al_endpoint_id(ion_dst);
	free(ion_dst);
	free(ion_src);
}

al_bp_error_t bp_ion_parse_eid_string(al_bp_endpoint_id_t* eid, const char* str)
{
	char * endpoint;
	PsmAddress psmAddress;
	MetaEid metaEid;
	VScheme * vscheme;
	endpoint = (char *) malloc(sizeof(char)*AL_BP_MAX_ENDPOINT_ID);
	memcpy(endpoint,str,strlen(str)+1);
	if(parseEidString(endpoint,&metaEid,&vscheme,&psmAddress) == 0) {
		free(endpoint);
		return BP_EPARSEEID;
	}
	(*eid) = ion_al_endpoint_id((char *)str);
	free(endpoint);
	return BP_SUCCESS;
}

al_bp_error_t bp_ion_set_payload(al_bp_bundle_payload_t* payload,
							al_bp_bundle_payload_location_t location,
							char* val, int len)
{
	memset(payload,0,sizeof(al_bp_bundle_payload_t));
	payload->location = location;
	if(location == BP_PAYLOAD_MEM)
	{
		payload->buf.buf_len = len;
		payload->buf.buf_val= val;
	}
	else
	{
		payload->filename.filename_len = len;
		payload->filename.filename_val = val;
	}
	return BP_SUCCESS;
}

void bp_ion_free_payload(al_bp_bundle_payload_t* payload)
{
	if(payload->status_report != NULL)
	{
		free(payload->status_report);
	}
	if(payload->location != BP_PAYLOAD_MEM && payload->filename.filename_val != NULL)
	{
		int type = 0;
		Sdr bpSdr = bp_get_sdr();
		sdr_begin_xn(bpSdr);
		Object fileRef = sdr_find(bpSdr, payload->filename.filename_val, &type);
		if(fileRef != 0)
		{
			zco_destroy_file_ref(bpSdr, fileRef);
//by David Zoller remove filename from catalog 
                        sdr_uncatlg(bpSdr, payload->filename.filename_val);
		}
		else
		//delete the payload file
			unlink(payload->filename.filename_val);
		sdr_end_xn(bpSdr);
	}
}

void bp_ion_free_extension_blocks(al_bp_bundle_spec_t* spec)
{
	 int i;
	for ( i=0; i<spec->blocks.blocks_len; i++ ) {
//		printf("Freeing extension block [%d].data at 0x%08X\n",
//						   i, (unsigned int) *(spec->blocks.blocks_val[i].data.data_val));
		if (spec->blocks.blocks_val[i].data.data_len > 0)
			free(spec->blocks.blocks_val[i].data.data_val);
	}
	free(spec->blocks.blocks_val);
	spec->blocks.blocks_val = NULL;
	spec->blocks.blocks_len = 0;
}

void bp_ion_free_metadata_blocks(al_bp_bundle_spec_t* spec)
{
	int i;
	for ( i=0; i< spec->metadata.metadata_len; i++ ) {
//		printf("Freeing metadata block [%d].data at 0x%08X\n",
//						   i, (unsigned int) *(spec->metadata.metadata_val[i].data.data_val));
		if (spec->metadata.metadata_val[i].data.data_len > 0)
			free(spec->metadata.metadata_val[i].data.data_val);
	}
	free(spec->metadata.metadata_val);
	spec->metadata.metadata_val = NULL;
	spec->metadata.metadata_len = 0;
}

al_bp_error_t bp_ion_error(int err)
{
	return BP_ENOTIMPL;
}
/***************** PRIVATE FUNCTION ******************
 * There are 2 private function to parse the payload
 * and have the status report.
 * This function are a copy of private function of ION
 *****************************************************/

/* *
 * Parse the admin record to have a status report.
 * Return 1 on success
 * */
int albp_parseAdminRecord(int *adminRecordType, BpStatusRpt *rpt, void **otherPtr, Object payload)
{
	Sdr			bpSdr = bp_get_sdr();
	unsigned int		buflen;
	unsigned char		*buffer;
	ZcoReader		reader;
	unsigned char		*cursor;
	int			bytesToParse;
	unsigned int		unparsedBytes;
	int			result;
	uvast 			uvtemp;

	/*****
	 ***** This code is partially copied from ION files. libbpP.c file in "bp/library" folder.
	 ***** In case of future errors, please, check it out if the original one si modified and try to copy the differences.
	 *****/

	sdr_begin_xn(bpSdr);
	buflen = zco_source_data_length(bpSdr, payload);
	buffer = (unsigned char *) malloc(sizeof(unsigned char)*buflen);
	if ( buffer == NULL )
	{
		sdr_end_xn(bpSdr);
		return -1;
	}
	zco_start_receiving(payload, &reader);
	bytesToParse = zco_receive_source(bpSdr, &reader, buflen, (char*)buffer);
	if (bytesToParse < 0)
	{
		sdr_end_xn(bpSdr);
		free(buffer);
		return -1;
	}
	cursor = buffer;
	unparsedBytes = bytesToParse;

	if (unparsedBytes < 1)
        {
                sdr_end_xn(bpSdr);
                free(buffer);
                return -1;
        }

	uvtemp = 2; /* Decode array of size 2. */
	if (cbor_decode_array_open(&uvtemp, &cursor, &unparsedBytes) < 1)
	{
		sdr_end_xn(bpSdr);
		free(buffer);
		return -1;
	}

	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		sdr_end_xn(bpSdr);
		free(buffer);
		return -1;
	}

	*adminRecordType = uvtemp;

	switch (*adminRecordType)
	{
		case BP_STATUS_REPORT:
		result = parseStatusRpt(rpt, (unsigned char*) cursor, unparsedBytes);
		break;
		/*case BP_CUSTODY_SIGNAL:
		result = 0; break;*/
		default: result = 0; break;
	}
	sdr_end_xn(bpSdr);
	free(buffer);
	return result;
}

/* *
 * Parse cursor to have a status report
 * Return 1 on success
 * *//*
int	albp_parseStatusRpt(BpStatusRpt *rpt, unsigned char *cursor,int unparsedBytes, int isFragment)
{
	unsigned int	eidLength;
	memset((char *) rpt, 0, sizeof(BpStatusRpt));
	rpt->isFragment = isFragment;
	if (unparsedBytes < 1)
	{
		return 0;
	}
	rpt->flags = *cursor;
	cursor++;
	rpt->reasonCode = *cursor;
	cursor++;
	unparsedBytes -= 2;
	if (isFragment)
	{
		extractSmallSdnv(&(rpt->fragmentOffset), &cursor, &unparsedBytes);
		extractSmallSdnv(&(rpt->fragmentLength), &cursor, &unparsedBytes);
	}

	if (rpt->flags & BP_RECEIVED_RPT)
	{
		extractSmallSdnv(&(rpt->receiptTime.seconds), &cursor,&unparsedBytes);
		extractSmallSdnv(&(rpt->receiptTime.nanosec), &cursor,&unparsedBytes);
	}

	if (rpt->flags & BP_CUSTODY_RPT)
	{
		extractSmallSdnv(&(rpt->acceptanceTime.seconds), &cursor,&unparsedBytes);
		extractSmallSdnv(&(rpt->acceptanceTime.nanosec), &cursor,&unparsedBytes);
	}

	if (rpt->flags & BP_FORWARDED_RPT)
	{
		extractSmallSdnv(&(rpt->forwardTime.seconds), &cursor,&unparsedBytes);
		extractSmallSdnv(&(rpt->forwardTime.nanosec), &cursor,&unparsedBytes);
	}

	if (rpt->flags & BP_DELIVERED_RPT)
	{
		extractSmallSdnv(&(rpt->deliveryTime.seconds), &cursor,&unparsedBytes);
		extractSmallSdnv(&(rpt->deliveryTime.nanosec), &cursor,&unparsedBytes);
	}

	if (rpt->flags & BP_DELETED_RPT)
	{

		extractSmallSdnv(&(rpt->deletionTime.seconds), &cursor,&unparsedBytes);
		extractSmallSdnv(&(rpt->deletionTime.nanosec), &cursor,&unparsedBytes);
	}

	extractSmallSdnv(&(rpt->creationTime.seconds), &cursor, &unparsedBytes);
	extractSmallSdnv(&(rpt->creationTime.count), &cursor, &unparsedBytes);
	extractSmallSdnv(&eidLength, &cursor, &unparsedBytes);
	if (unparsedBytes != eidLength)
	{
		return 0;
	}
	rpt->sourceEid = MTAKE(eidLength + 1);
	if (rpt->sourceEid == NULL)
	{
		return -1;
	}
	memcpy(rpt->sourceEid, cursor, eidLength);
	rpt->sourceEid[eidLength] = '\0';
	return 1;
}*/

/*****************************************************/
/*
 * If there isn't the ION implementation
 * the API there are not implementation
 */
#else
al_bp_error_t bp_ion_attach()
{
	return BP_ENOTIMPL;
}

al_bp_error_t bp_ion_open_with_IP(char * daemon_api_IP, int daemon_api_port, al_bp_handle_t * handle)
{
	return BP_ENOTIMPL;
}

al_bp_error_t bp_ion_errno(al_bp_handle_t handle)
{
	return BP_ENOTIMPL;
}

al_bp_error_t bp_ion_build_local_eid(al_bp_endpoint_id_t* local_eid,
								const char* service_tag,
								al_bp_scheme_t type)
{
	return BP_ENOTIMPL;
}

al_bp_error_t bp_ion_register(al_bp_handle_t * handle,
						al_bp_reg_info_t* reginfo,
						al_bp_reg_id_t* newregid)
{
	return BP_ENOTIMPL;
}

al_bp_error_t bp_ion_find_registration(al_bp_handle_t handle,
						al_bp_endpoint_id_t * eid,
						al_bp_reg_id_t * newregid)
{
	return BP_ENOTIMPL;
}

al_bp_error_t bp_ion_unregister(al_bp_endpoint_id_t eid)
{
	return BP_ENOTIMPL;
}

al_bp_error_t bp_ion_send(al_bp_handle_t handle,
					al_bp_reg_id_t regid,
					al_bp_bundle_spec_t* spec,
					al_bp_bundle_payload_t* payload,
					al_bp_bundle_id_t* id)
{
	return BP_ENOTIMPL;
}

al_bp_error_t bp_ion_recv(al_bp_handle_t handle,
					al_bp_bundle_spec_t* spec,
					al_bp_bundle_payload_location_t location,
					al_bp_bundle_payload_t* payload,
					al_bp_timeval_t timeout)
{
	return BP_ENOTIMPL;
}

al_bp_error_t bp_ion_close(al_bp_handle_t handle)
{
	return BP_ENOTIMPL;
}

void bp_ion_copy_eid(al_bp_endpoint_id_t* dst, al_bp_endpoint_id_t* src)
{

}

al_bp_error_t bp_ion_parse_eid_string(al_bp_endpoint_id_t* eid, const char* str)
{
	return BP_ENOTIMPL;
}

al_bp_error_t bp_ion_set_payload(al_bp_bundle_payload_t* payload,
							al_bp_bundle_payload_location_t location,
							char* val, int len)
{
	return BP_ENOTIMPL;
}

void bp_ion_free_payload(al_bp_bundle_payload_t* payload)
{

}

void bp_ion_free_extension_blocks(al_bp_bundle_spec_t* spec)
{

}

void bp_ion_free_metadata_blocks(al_bp_bundle_spec_t* 	spec)
{

}

al_bp_error_t bp_ion_error(int err)
{
	return BP_ENOTIMPL;
}
#endif /* ION_IMPLEMENTATION */
