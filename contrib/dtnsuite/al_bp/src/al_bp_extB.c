/********************************************************
 **  Authors: Andrea Bisacchi, andrea.bisacchi5@studio.unibo.it
 **           Carlo Caini (DTNperf_3 project supervisor), carlo.caini@unibo.it
 **
 **
 **  Copyright (c) 2013, Alma Mater Studiorum, University of Bologna
 **  All rights reserved.
 ********************************************************/

/*
 * al_bp_extB.c
 */

#include "al_bp_extB.h"
#include "includes.h"

#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <libgen.h>
#include <sys/stat.h>
#include "al_bp_api.h"

#include "list.h"

//#define EXTB_DEBUG

typedef struct
{
	al_bp_reg_info_t reginfo;					// Reginfo, used in al_bp
	al_bp_endpoint_id_t local_eid;				// My eid
	al_bp_handle_t handle;						// Handle, used in al_bp
	pthread_mutex_t mutexDTN2send_receive;		// Mutex for send/receive (not at the same time)
	al_bp_reg_id_t regid;						// Regid, used in al_bp
	al_bp_implementation_t bp_implementation; 	// Saves the implementation used
	al_bp_error_t error;						// used to "save" the last error
	al_bp_extB_registration_descriptor reg_des;	// Save the univoc register descriptor (used to be found in list)
} al_bp_extB_registration_t;

static List registration_list = empty_list;		// One node for each register

static al_bp_extB_registration_descriptor max_registration_descriptor = 1;

static boolean_t initialized = FALSE;						// Tells if initialized or not

static al_bp_scheme_t scheme; 								// scheme CBHE_SCHEME, DTN_SCHEME

static char eid_format;										// D=DTN, I=IPN

static al_bp_implementation_t bp_implementation = BP_NONE;	// Implementation active

static int ipn_node_forDTN2;								// ipn node number used only in case of registration following the ipn scheme on a DTN2 machine

// Private functions
static al_bp_error_t set_eid_scheme(char force_eid);
static al_bp_error_t build_local_eid(al_bp_extB_registration_t* registration, char* dtn_demux_string, int ipn_demux_number, int ipn_node_forDTN2);
static void remove_registration_from_list(al_bp_extB_registration_descriptor reg_des);
static void insert_registration_in_list(al_bp_extB_registration_t registration);
static al_bp_extB_registration_t* get_registration_from_reg_des(al_bp_extB_registration_descriptor reg_des);
static int compare_registration_and_reg_des(void* data1, size_t data1_size, void* data2, size_t data2_size);

// This function registers a new connection of the application to the BP.
// Output:
// 		registration_descriptor: the registration descriptor (&gt;0; 0 in case of error)
// Input:
// 		dtn_demux string: dtn demux token (string)
// 		ipn_demux_number: demux token (number )to be used in case of an ipn scheme registration (&gt;0)
// It returns, in addition to the possible error, the registration_descriptor (an integer somewhat inspired to file descriptor in sockets, but not passed by
// the OS) to be given in input to “al_bp_extB” functions that work on a specific registration.
al_bp_extB_error_t al_bp_extB_register(al_bp_extB_registration_descriptor* registration_descriptor, char* dtn_demux_string, int ipn_demux_number)
{
	al_bp_extB_registration_t registration;
	*registration_descriptor = 0;
	
	if (eid_format == 'D' && dtn_demux_string == NULL)
		return BP_EXTB_ERRNULLPTR;
	if (eid_format == 'I' && ipn_demux_number <= 0)
		return BP_EXTB_ERRIPNPARAMETERS;

	memset(&registration, 0, sizeof(al_bp_extB_registration_t)); // set all to 0

	registration.reg_des = max_registration_descriptor;

	//open
	registration.error = al_bp_open(&(registration.handle));
	if (bp_implementation == BP_DTN)
		pthread_mutex_init(&(registration.mutexDTN2send_receive), NULL);
	
	// XXX To force a specific IP use the below function
	//registration.error = al_bp_open_with_ip(ip_addr, ip_port, &(registration.handle)); 

	if (registration.error != BP_SUCCESS)
		return BP_EXTB_ERROPEN;

	//get local eid
	if (build_local_eid(&registration, dtn_demux_string, ipn_demux_number, ipn_node_forDTN2) != BP_SUCCESS)
		return BP_EXTB_ERRLOCALEID;

	//set reginfo
	al_bp_copy_eid(&(registration.reginfo.endpoint), &(registration.local_eid));
	registration.reginfo.flags = BP_REG_DEFER;
	registration.reginfo.regid = BP_REGID_NONE;
	registration.reginfo.expiration = 0;

	registration.bp_implementation = bp_implementation;

	//register
	registration.error = al_bp_register(&(registration.handle), &(registration.reginfo), &(registration.regid));
	if (	(bp_implementation == BP_DTN && registration.error != BP_SUCCESS)
			|| (bp_implementation == BP_ION && (registration.error == BP_EBUSY || registration.error == BP_EPARSEEID || registration.error == BP_EREG))
			|| (bp_implementation == BP_IBR && registration.error != BP_SUCCESS))
		return BP_EXTB_ERRREGISTER;

	// put registration in the list
	insert_registration_in_list(registration);

	*registration_descriptor = registration.reg_des;
	max_registration_descriptor++;
	
	return BP_EXTB_SUCCESS;
}

// It unregisters the registration identified by the registration descriptor.
al_bp_extB_error_t al_bp_extB_unregister(al_bp_extB_registration_descriptor registration_descriptor)
{
	al_bp_extB_error_t error;
	al_bp_extB_registration_t *registration;

	registration = get_registration_from_reg_des(registration_descriptor);
	if (registration == NULL)
		return BP_EXTB_ERRNOTREGISTRED;

	error = BP_EXTB_SUCCESS;

	if (al_bp_close(registration->handle) != BP_SUCCESS)
		error = BP_EXTB_ERRCLOSE;

	//if (registration->bp_implementation == BP_DTN && al_bp_close(registration->handle_send) != BP_SUCCESS)
	//	error = BP_EXTB_ERRCLOSE;

	if (registration->bp_implementation == BP_DTN)
		pthread_mutex_destroy(&(registration->mutexDTN2send_receive));

	if (registration->bp_implementation == BP_ION)
		if (al_bp_unregister(registration->handle, registration->regid, registration->local_eid) != BP_SUCCESS)
			error =  BP_EXTB_ERRUNREGISTER;

	// remove from list
	remove_registration_from_list(registration->reg_des);

	return error;
}

#ifdef EXTB_DEBUG
static void debug_print_timestamp(al_bp_timestamp_t ts) {
	printf("%d.%d", ts.secs,ts.seqno);
}

static void debug_print_bundle_object(al_bp_bundle_object_t b) {
	// BUNDLE ID
	printf("\tid=%p\n",b.id);
	if (b.id != NULL) {
		printf("\t\tsource=%s\n",b.id->source.uri);
		printf("\t\tcreation_ts=%d.%d\n",b.id->creation_ts.secs, b.id->creation_ts.seqno);
		printf("\t\tfrag_offset=%d\n",b.id->frag_offset);
		printf("\t\torig_length=%d\n",b.id->orig_length);
	} else {
		printf("There are no b.id.\n");
	}
	
	// BUNDLE SPEC
	printf("\tspec=%p\n",b.spec);
	if (b.spec != NULL) {
		printf("\t\tsource=%s\n", b.spec->source.uri);
		printf("\t\tdest=%s\n", b.spec->dest.uri);
		printf("\t\treplyto=%s\n", b.spec->replyto.uri);
		printf("\t\tpriority=%d.%d\n", b.spec->priority.priority, b.spec->priority.ordinal);
		printf("\t\tdopts=%d\n", b.spec->dopts);
		printf("\t\texpiration=%d\n", b.spec->expiration);
		printf("\t\tcreation_ts=%d.%d\n", b.spec->creation_ts.secs, b.spec->creation_ts.seqno);
		printf("\t\tdelivery_regid=%d\n", b.spec->delivery_regid);
		printf("\t\tblocks len=%d\n", b.spec->blocks.blocks_len);
		printf("\t\tmetadata len=%d\n", b.spec->metadata.metadata_len);
		printf("\t\tunreliable=%d\n", (int)b.spec->unreliable);
		printf("\t\tcritical=%d\n", (int)b.spec->critical);
		printf("\t\tflow_label=%d\n", b.spec->flow_label);
	} else {
		printf("There are no b.spec.\n");
	}
	
	// BUNDLE PAYLOAD
	printf("\tpayload=%p\n", b.payload);
	if (b.payload != NULL) {
		printf("\t\tpayloadLocation=%d\n", b.payload->location);
		printf("\t\tfilename:\n\t\t\tlen=%d\n\t\t\tval=%s\n", b.payload->filename.filename_len, b.payload->filename.filename_val);
		printf("\t\tbuf:\n\t\t\tlen=%d\n\t\t\tval=%s\n", b.payload->buf.buf_len, b.payload->buf.buf_val);
		printf("\t\tstatus_report=%p\n", b.payload->status_report);
		
		if (b.payload->status_report != NULL) {
			// BUNDLE PAYLOAD -> STATUS REPORT
			printf("\t\t\t\tsource=%s\n", b.payload->status_report->bundle_id.source.uri);
			printf("\t\t\t\tcreation_ts=%d.%d\n",b.payload->status_report->bundle_id.creation_ts.secs, b.payload->status_report->bundle_id.creation_ts.seqno);
			printf("\t\t\t\tfrag_offset=%d\n",b.payload->status_report->bundle_id.frag_offset);
			printf("\t\t\t\torig_length=%d\n",b.payload->status_report->bundle_id.orig_length);
			printf("\t\t\treason=%d\n", b.payload->status_report->reason);
			printf("\t\t\tflags=%d\n", b.payload->status_report->flags);
	
			printf("\t\t\treceipt_ts=");
			debug_print_timestamp(b.payload->status_report->receipt_ts);
			printf("\n");
			printf("\t\t\tcustody_ts=");
			debug_print_timestamp(b.payload->status_report->custody_ts);
			printf("\n");
			printf("\t\t\tforwarding_ts=");
			debug_print_timestamp(b.payload->status_report->forwarding_ts);
			printf("\n");
			printf("\t\t\tdelivery_ts=");
			debug_print_timestamp(b.payload->status_report->delivery_ts);
			printf("\n");
			printf("\t\t\tdeletion_ts=");
			debug_print_timestamp(b.payload->status_report->deletion_ts);
			printf("\n");
			printf("\t\t\tack_by_app_ts=");
			debug_print_timestamp(b.payload->status_report->ack_by_app_ts);
			printf("\n");
		} else {
		printf("There are no b.payload->status_report.\n");
	}
	}  else {
		printf("There are no b.payload.\n");
	}
}
#endif

// This function uses the registration_descriptor to identify the 
// registration; then it sends a bundle object. It is a wrapper of the 
// al_bp_bundle_send, but the destination and the "reply to" EIDs are 
// passed in input. For the sake of backward compatibility the 
// destination and "reply to" originally contained in the bundle object 
// are saved and restored at the end of the function.
al_bp_extB_error_t al_bp_extB_send(al_bp_extB_registration_descriptor registration_descriptor, al_bp_bundle_object_t bundle, al_bp_endpoint_id_t destination, al_bp_endpoint_id_t reply_to)
{
	al_bp_extB_registration_t* registration;
	al_bp_endpoint_id_t old_destination;
	al_bp_endpoint_id_t old_reply_to;
	al_bp_handle_t handle;

	registration = get_registration_from_reg_des(registration_descriptor);
	if (registration == NULL)
		return BP_EXTB_ERRNOTREGISTRED;

	if (strcmp(destination.uri, al_bp_get_none_endpoint_string()) == 0)
		return BP_EXTB_ERRRECEIVER;

	old_destination = bundle.spec->dest;
	// save old values
	old_reply_to = bundle.spec->replyto;
	al_bp_bundle_set_source(&bundle, registration->local_eid);
	al_bp_bundle_set_dest(&bundle, destination);
	al_bp_bundle_set_replyto(&bundle, reply_to);
	
	// This should avoid the blocking in getting handle
	/*if (registration->bp_implementation == BP_DTN)
		handle = registration->handle_send;
	else
		handle = registration->handle;*/
	
	handle = registration->handle;
	if (registration->bp_implementation == BP_DTN)
		pthread_mutex_lock(&(registration->mutexDTN2send_receive));

	#ifdef DEBUG
	debug_print_bundle_object(bundle);
	#endif

	registration->error = al_bp_bundle_send(handle, registration->regid, bundle);

	if (registration->bp_implementation == BP_DTN)
		pthread_mutex_unlock(&(registration->mutexDTN2send_receive));
	// reset old destination and old reply_to
	al_bp_bundle_set_dest(&bundle, old_destination);
	al_bp_bundle_set_replyto(&bundle, old_reply_to);
	switch (registration->error)
	{
	case BP_EXTB_SUCCESS:
		return BP_EXTB_SUCCESS;
	default:
		return BP_EXTB_ERRSEND;
	}
}

// It receives a bundle object destined to the registration_descriptor 
// passed in input. It is a wrapper of the al_bp_bundle_receive. 
// Payload_location is a structure passed to al_bp lower layers. Timeval 
// is a timeout value passed to al_bp_lower layers.
al_bp_extB_error_t al_bp_extB_receive(al_bp_extB_registration_descriptor registration_descriptor, al_bp_bundle_object_t* bundle, al_bp_bundle_payload_location_t payload_location, al_bp_timeval_t timeval)
{
	al_bp_extB_registration_t* registration;
	struct timeval begin;
	struct timeval loop;
	boolean_t negative_timeval = (timeval < 0);

	if (bundle == NULL)
                return BP_EXTB_ERRNULLPTR;

	registration = get_registration_from_reg_des(registration_descriptor);
	if (registration == NULL)
		return BP_EXTB_ERRNOTREGISTRED;

	if (registration->bp_implementation == BP_DTN)
		pthread_mutex_lock(&(registration->mutexDTN2send_receive));

	if (!negative_timeval)
		gettimeofday(&begin, NULL);

	do
	{
		registration->error = al_bp_bundle_receive(registration->handle, bundle, payload_location, timeval);
		if (!negative_timeval) // adjust the timeout
		{
			gettimeofday(&loop, NULL);
			timeval -= (loop.tv_sec - begin.tv_sec); // timeval = timeval - (time I waited)
		}
	} while ((negative_timeval || timeval > 0) && registration->bp_implementation == BP_DTN && (int)registration->error < 0); // this because DTN2 can return -1 value, I don't know exactly why

	if (registration->bp_implementation == BP_DTN)
		pthread_mutex_unlock(&(registration->mutexDTN2send_receive));

	if (registration->bp_implementation == BP_DTN && (int)registration->error < 0)
		return BP_EXTB_ERRTIMEOUT;

	switch (registration->error)
	{
	case BP_SUCCESS:
		return BP_EXTB_SUCCESS;
	case BP_ERECVINT:
		return BP_EXTB_ERRRECEPINTER;
	case BP_ETIMEOUT:
		return BP_EXTB_ERRTIMEOUT;
	default:
		return BP_EXTB_ERRRECEIVE;
	}
}

// It initializes the Abstraction Layer extension "B". The "force_eid" 
// (N|D|I) is used to specify if the default format of the registration 
// must be overridden ("N" no, i.e. use the default, 'D' for "dtn", 'I' 
// for "ipn"); it also discovers the active BP implementation and saves 
// this information in a local variable available to other extB 
// functions. If it does not find any active BP implementation, it 
// returns a specific error.
al_bp_extB_error_t al_bp_extB_init(char force_eid_scheme, int ipn_node_DTN2)
{
	al_bp_error_t error;
	if (initialized)
		return BP_EXTB_ERRINIT;

	if (force_eid_scheme != 'I' && force_eid_scheme != 'D' && force_eid_scheme != 'N')
		return BP_EXTB_ERRINIT;

	// Discovers the BP implementation
	bp_implementation = al_bp_get_implementation();
	
	ipn_node_forDTN2 = ipn_node_DTN2;
	
	if (bp_implementation == BP_NONE)
		return BP_EXTB_ERRNOIMPLEMENTATION;

	error = set_eid_scheme(force_eid_scheme);
	
	if (eid_format == 'I' && bp_implementation == BP_DTN && ipn_node_forDTN2 <= 0)
		return BP_EXTB_ERRDTN2PARAMETERS;
	
	if (error == BP_SUCCESS) {
		registration_list = empty_list;
		initialized = TRUE;
		return BP_EXTB_SUCCESS;
	} else
		return BP_EXTB_ERRINIT;
}

// It is a wrapper of al_bp_find_registration. It gives in output the 
// registration descriptor corresponding to the EID passed in input.
al_bp_error_t al_bp_extB_find_registration(al_bp_extB_registration_descriptor registration_descriptor, al_bp_endpoint_id_t* eid)
{
	al_bp_extB_registration_t* registration;

	if (eid == NULL)
		return BP_ENULLPNTR;

	registration = get_registration_from_reg_des(registration_descriptor);
	if (registration == NULL) return BP_ENULLPNTR;

	return al_bp_find_registration(registration->handle, eid, &(registration->regid));
}

// It destroys the registration list. After calling this function, the 
// programmer can no more use any al_bp_extB functions except the al_bp_extB_init.
void al_bp_extB_destroy()
{
	if (!initialized) return;
	initialized = FALSE;

	// dealloc all list
	list_destroy(&registration_list);
}

// Returns the string of the error passed
char* al_bp_extB_str_type_error(al_bp_extB_error_t error)
{
	switch (error) {
	case BP_EXTB_SUCCESS:
		return NULL;
	case BP_EXTB_ERROPEN:
		return "opening BP handle";
	case BP_EXTB_ERRLOCALEID:
		return "building local EID";
	case BP_EXTB_ERRREGISTER:
		return "registering eid";
	case BP_EXTB_ERRRECEPINTER:
		return "bundle reception interrupted";
	case BP_EXTB_ERRTIMEOUT:
		return "bundle reception timeout expired";
	case BP_EXTB_ERRRECEIVE:
		return "getting server ack";
	default:
		return "unknown error";
	}
}

/***************************************************
 *        BACKWARD COMPATIBILITY FUNCTIONS         *
 ***************************************************/

// It returns the "reg_info" structure associated to the registration 
// descriptor given in input.
al_bp_reg_info_t al_bp_extB_get_reginfo(al_bp_extB_registration_descriptor registration_descriptor)
{
	al_bp_extB_registration_t* registration;
	registration = get_registration_from_reg_des(registration_descriptor);
	if (registration == NULL) { al_bp_reg_info_t result; memset(&result, 0, sizeof(al_bp_reg_info_t)); return result; }
	return registration->reginfo;
}

// It returns the "reg_id" structure associated to the registration 
// descriptor given in input.
al_bp_reg_id_t al_bp_extB_get_regid(al_bp_extB_registration_descriptor registration_descriptor)
{
	al_bp_extB_registration_t* registration;
	registration = get_registration_from_reg_des(registration_descriptor);
	if (registration == NULL) { al_bp_reg_id_t result; memset(&result, 0, sizeof(al_bp_reg_id_t)); return result; }
	return registration->regid;
}

// It returns the EID format used in all registrations, found and saved 
// by the al_bp_extB_init ("I" for "ipn" and "D" for "dtn").
char al_bp_extB_get_eid_format()
{
	return eid_format;
}

// It returns the local EID structure associated to the registration 
// descriptor given in input.
al_bp_endpoint_id_t al_bp_extB_get_local_eid(al_bp_extB_registration_descriptor registration_descriptor)
{
	al_bp_extB_registration_t* registration;
	registration = get_registration_from_reg_des(registration_descriptor);
	if (registration == NULL) { al_bp_endpoint_id_t none; al_bp_get_none_endpoint(&none); return none; }
	return registration->local_eid;
}

// It sets the local EID for the corresponding registration descriptor 
// (both in input).
void al_bp_extB_set_local_eid(al_bp_extB_registration_descriptor registration_descriptor, al_bp_endpoint_id_t local_eid)
{
	al_bp_extB_registration_t* registration;

	if (!initialized) return;

	registration = get_registration_from_reg_des(registration_descriptor);
	if (registration == NULL) return;

	registration->local_eid = local_eid;
}

// It returns (DTN2, IBR-DTN) or prints (ION) the last "errno" given by 
// the BP implementation, corresponding to the registration descriptor 
// passed in input.
al_bp_error_t al_bp_extB_errno(al_bp_extB_registration_descriptor registration_descriptor)
{
	al_bp_extB_registration_t* registration;

	if (!initialized) return BP_ERRBASE;

	registration = get_registration_from_reg_des(registration_descriptor);
	if (registration == NULL) return BP_ENULLPNTR;

	return al_bp_errno(registration->handle);
}

// It returns the last error (in al_bp format) that occurred in the 
// previous al_bp (not al_bp_extB) function corresponding to the 
// registration descriptor passed in input.
al_bp_error_t al_bp_extB_get_error(al_bp_extB_registration_descriptor registration_descriptor)
{
	al_bp_extB_registration_t* registration;

	if (!initialized) return BP_ERRBASE;

	registration = get_registration_from_reg_des(registration_descriptor);
	if (registration == NULL) return BP_ENULLPNTR;

	return registration->error;
}

// It returns the "handle" structure associated to the registration 
// descriptor given in input.
al_bp_handle_t al_bp_extB_get_handle(al_bp_extB_registration_descriptor registration_descriptor)
{
	al_bp_extB_registration_t* registration;
	registration = get_registration_from_reg_des(registration_descriptor);
	if (registration == NULL) return NULL;
	return registration->handle;
}

// It returns the last error (as a string) that occurred in the previous 
// al_bp (not al_bp_extB) function corresponding to the registration 
// descriptor passed in input.
char* al_bp_extB_strerror(al_bp_extB_registration_descriptor registration_descriptor)
{
	al_bp_extB_registration_t* registration;
	registration = get_registration_from_reg_des(registration_descriptor);
	if (registration == NULL) return NULL;
	return al_bp_strerror(registration->error);
}

/***************************************************
 *               PRIVATE FUNCTIONS                 *
 ***************************************************/
static al_bp_error_t set_eid_scheme(char force_eid_scheme)
{
	if (force_eid_scheme != 'N')
	{
		eid_format = force_eid_scheme;
		switch (eid_format) {
			case 'I':
				scheme = CBHE_SCHEME;
				break;
			case 'D':
				scheme = DTN_SCHEME;
				break;
			default:
				return BP_EINVAL;
				break;
		}
	}
	else
	{ // eid is NOT forces, use de defaults
		switch (bp_implementation)
		{
		case BP_ION:
			eid_format = 'I';
			scheme = CBHE_SCHEME;
			break;
		case BP_DTN:
		case BP_IBR:
			eid_format = 'D';
			scheme = DTN_SCHEME;
			break;
		default:
			return BP_EINVAL;
		}
	}
	return BP_SUCCESS;
}

// demux_string is double pointer because the function makes the malloc, demux_string will point to an area
static al_bp_error_t build_local_eid(al_bp_extB_registration_t* registration, char* dtn_demux_string, int ipn_demux_number, int ipn_node_forDTN2)
{
	char demux_string_temp[256];
	
	switch (eid_format) {
	case 'I':
		//if using a DTN2 implementation, al_bp_build_local_eid() wants ipn_node_forDTN2.service_number
		if (bp_implementation == BP_DTN)
			sprintf(demux_string_temp, "%d.%lu", ipn_node_forDTN2, (unsigned long) ipn_demux_number);
		else
			sprintf(demux_string_temp, "%lu", (unsigned long) ipn_demux_number);
		break;
	case 'D':
		// client will register with DTN scheme
		// append process id to the client demux string
		//demux_string = (char*) malloc(sizeof(char) * (strlen(dtn2_str) + 10));
		//sprintf(demux_string, "%s_%d", dtn2_str, ipn_demux_number);
		//strcpy(demux_string_temp, demux_string);
		if (dtn_demux_string == NULL)
			return BP_EXTB_ERRNULLPTR;
		strcpy(demux_string_temp, dtn_demux_string);
		break;
	default:
		return BP_EXTB_ERRLOCALEID;
	}
	return al_bp_build_local_eid((*registration).handle, &((*registration).local_eid), demux_string_temp, scheme);
}

static void remove_registration_from_list(al_bp_extB_registration_descriptor registration_descriptor)
{
	list_remove_data(&registration_list, &registration_descriptor, sizeof(registration_descriptor), &compare_registration_and_reg_des);
}

static void insert_registration_in_list(al_bp_extB_registration_t registration)
{
	// insert in the list
	list_push_back(&registration_list, &registration, sizeof(registration));
}

static al_bp_extB_registration_t* get_registration_from_reg_des(al_bp_extB_registration_descriptor registration_descriptor)
{
	al_bp_extB_registration_t* registration_pointer;

	registration_pointer = (al_bp_extB_registration_t*) list_get_pointer_data(registration_list, &registration_descriptor, sizeof(al_bp_extB_registration_descriptor), &compare_registration_and_reg_des);

	return registration_pointer;
}

static int compare_registration_and_reg_des(void* data1, size_t data1_size, void* data2, size_t data2_size)
{
	al_bp_extB_registration_t registration;
	al_bp_extB_registration_descriptor reg_des;

	registration.reg_des = reg_des = (al_bp_extB_registration_descriptor) -1; // may be uninitialized

	if (data1 == NULL) return -1;
	if (data2 == NULL) return 1;

	if (data1_size == sizeof(al_bp_extB_registration_t))
		registration = * ((al_bp_extB_registration_t*) data1);
	else if (data1_size == sizeof(al_bp_extB_registration_descriptor))
		reg_des = * ((al_bp_extB_registration_descriptor*) data1);
	else
		return -1;

	if (data2_size == sizeof(al_bp_extB_registration_t))
		registration = * ((al_bp_extB_registration_t*) data2);
	else if (data2_size == sizeof(al_bp_extB_registration_descriptor))
		reg_des = * ((al_bp_extB_registration_descriptor*) data2);
	else
		return 1;

	if (reg_des == -1 || registration.reg_des == -1)
		return -1;

	return (reg_des - registration.reg_des);
}
