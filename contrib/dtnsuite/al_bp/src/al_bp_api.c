/********************************************************
 **  Authors: Michele Rodolfi, michele.rodolfi@studio.unibo.it
 **           Anna d'Amico, anna.damico2@studio.unibo.it
 **           Davide Pallotti, davide.pallotti@studio.unibo.it
 **           Andrea Bisacchi, andrea.bisacchi5@studio.unibo.it
 **           Carlo Caini (DTNperf_3 project supervisor), carlo.caini@unibo.it
 **
 **
 **  Copyright (c) 2013, Alma Mater Studiorum, University of Bologna
 **  All rights reserved.
 ** This file contains all the APIs of the al_bp (al_bp prefix).
 ** These are called directly from the application.
 ** In DTNperf_3 These are the sole al_bp_ APIs called. 
 ** Each API consists of a switch between DTN2,ION and IBR-DTN API implementations (bp prefix)
 ** For their meaning, see al_bp documentation.
 ********************************************************/

/*
 * bp_abstraction_layer.c
 *
 */

#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

#include "includes.h"
#include "al_bp_api.h"
#include "al_bp_version.h"

/* Implementations API */
#include "al_bp_dtn.h"
#include "al_bp_ion.h"
#include "al_bp_ibr.h"

#ifdef mingw
#include <windows.h>
#include <tlhelp32.h>
BOOL FindProcessId(const char *processname);
#endif

static al_bp_implementation_t bp_implementation = BP_NONE;
const char* al_bp_version = AL_BP_VERSION_STRING;

#define DTNNONE_STRING "dtn:none"
const char* dtn_none = DTNNONE_STRING;

/* This the only API of this file whose name has not the prefix al_bp*/
const char * get_al_bp_version()
{
	return al_bp_version;
}

#ifdef mingw
BOOL FindProcessId(const char *processname)
{
	HANDLE hProcessSnap;
	PROCESSENTRY32 pe32;
	BOOL result = FALSE;

	// Take a snapshot of all processes in the system.
	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (INVALID_HANDLE_VALUE == hProcessSnap) return(FALSE);

	pe32.dwSize = sizeof(PROCESSENTRY32); // <----- IMPORTANT

	// Retrieve information about the first process,
	// and exit if unsuccessful
	if (!Process32First(hProcessSnap, &pe32))
	{
		CloseHandle(hProcessSnap);          // clean the snapshot object
		//printf("!!! Failed to gather information on system processes! \n");
		return(FALSE);
	}

	do
	{
		if (0 == strcmp(processname, pe32.szExeFile))
		{
			result = TRUE;
			break;
		}
	} while (Process32Next(hProcessSnap, &pe32));

	CloseHandle(hProcessSnap);

	return result;
}
#endif

al_bp_implementation_t al_bp_get_implementation()
{
	if (bp_implementation == BP_NONE)
	{
#ifdef mingw
		if (FindProcessId("dtnd.exe"))
			bp_implementation = BP_DTN;
		else if (FindProcessId("rfxclock.exe"))
			bp_implementation = BP_ION;
		else if (FindProcessId("ibrdtnd.exe"))
			bp_implementation = BP_IBR;
#else
		char* ibr1 = "/usr/sbin/dtnd";
		char* ibr2 = "/usr/local/sbin/dtnd";
		char* ibr3 = "ibrdtnd";
		char* ion = "rfxclock";
		char* dtn2 = "dtnd";
		DIR* procs = opendir("/proc");
		struct dirent* process;
		
		while (bp_implementation == BP_NONE && (process = readdir(procs)) != NULL) {
			#define LINE_MAX_SIZE 1024
			int fd_cmdline;
			char cmdline[LINE_MAX_SIZE];
			char* path = malloc(strlen(process->d_name) + 15);
			sprintf(path, "/proc/%s/cmdline", process->d_name);
			fd_cmdline = open(path, O_RDONLY);
			// read as much as I can
			memset(cmdline, 0, LINE_MAX_SIZE);
			if (read(fd_cmdline, cmdline, LINE_MAX_SIZE)) {
				if (strstr(cmdline, ibr1) != NULL || strstr(cmdline, ibr2) != NULL || strstr(cmdline, ibr3) != NULL) { // IBR
					bp_implementation = BP_IBR;
				} else if (strstr(cmdline, dtn2) != NULL) { // DTN2
					bp_implementation = BP_DTN;
				} else if (strstr(cmdline, ion) != NULL) { // ION
					bp_implementation = BP_ION;
				}
			}
			close(fd_cmdline);
			free(path);
		} // while

		closedir(procs);
#endif
	}
	return bp_implementation;
}

al_bp_error_t al_bp_open(al_bp_handle_t* handle)
{
	if (handle == NULL)
		return BP_ENULLPNTR;

	switch (al_bp_get_implementation())
	{
	case BP_DTN:
		return bp_dtn_open(handle);

	case BP_ION:
		return bp_ion_attach();

	case BP_IBR:
		return bp_ibr_open(handle);

	default: // cannot find bundle protocol implementation
		return BP_ENOBPI;
	}
}

al_bp_error_t al_bp_open_with_ip(char *daemon_api_IP,int daemon_api_port,al_bp_handle_t* handle)
{
	if (handle == NULL)
		return BP_ENULLPNTR;

	switch (al_bp_get_implementation())
	{
	case BP_DTN:
		return bp_dtn_open_with_IP(daemon_api_IP, daemon_api_port, handle);

	case BP_ION:
		return bp_ion_open_with_IP(daemon_api_IP, daemon_api_port, handle);

	case BP_IBR:
		return bp_ibr_open_with_IP(daemon_api_IP, daemon_api_port, handle);

	default: // cannot find bundle protocol implementation
		return BP_ENOBPI;
	}
}

al_bp_error_t al_bp_errno(al_bp_handle_t handle)
{
	switch (al_bp_get_implementation())
	{
	case BP_DTN:
		return bp_dtn_errno(handle);

	case BP_ION:
		return bp_ion_errno(handle);

	case BP_IBR:
		return bp_ibr_errno(handle);

	default: // cannot find bundle protocol implementation
		return BP_ENOBPI;
	}
}

al_bp_error_t al_bp_build_local_eid(al_bp_handle_t handle,
		al_bp_endpoint_id_t* local_eid,
		const char* service_tag,
		al_bp_scheme_t type)
{
	if (local_eid == NULL)
		return BP_ENULLPNTR;

	switch (al_bp_get_implementation())
	{
	case BP_DTN:
		//if type is CBHE, service_tag must be in the form ipn_local_number.service_number
		return bp_dtn_build_local_eid(handle, local_eid, service_tag, type);

	case BP_ION:
		return bp_ion_build_local_eid(local_eid, service_tag,type);

	case BP_IBR:
		return bp_ibr_build_local_eid(handle, local_eid, service_tag, type);

	default: // cannot find bundle protocol implementation
		return BP_ENOBPI;
	}
}

al_bp_error_t al_bp_register(al_bp_handle_t * handle,
		al_bp_reg_info_t* reginfo,
		al_bp_reg_id_t* newregid)
{
	if (reginfo == NULL)
		return BP_ENULLPNTR;
	if (newregid == NULL)
		return BP_ENULLPNTR;

	switch (al_bp_get_implementation())
	{
	case BP_DTN:
		return bp_dtn_register(*handle, reginfo, newregid);

	case BP_ION:
		return bp_ion_register(handle, reginfo, newregid);

	case BP_IBR:
		return bp_ibr_register(*handle, reginfo, newregid);

	default: // cannot find bundle protocol implementation
		return BP_ENOBPI;
	}
}

al_bp_error_t al_bp_find_registration(al_bp_handle_t handle,
		al_bp_endpoint_id_t * eid,
		al_bp_reg_id_t * newregid)
{
	if (eid == NULL)
		return BP_ENULLPNTR;

	switch (al_bp_get_implementation())
	{
	case BP_DTN:
		return bp_dtn_find_registration(handle, eid, newregid);

	case BP_ION:
		return bp_ion_find_registration(handle, eid, newregid);

	case BP_IBR:
		return bp_ibr_find_registration(handle, eid, newregid);

	default: // cannot find bundle protocol implementation
		return BP_ENOBPI;
	}
}

al_bp_error_t al_bp_unregister(al_bp_handle_t handle, al_bp_reg_id_t regid,al_bp_endpoint_id_t eid){

	switch (al_bp_get_implementation())
	{
	case BP_DTN:
		return bp_dtn_unregister(handle, regid);

	case BP_ION:
		return bp_ion_unregister(eid);

	case BP_IBR:
		return bp_ibr_unregister(handle, regid, eid);

	default: // cannot find bundle protocol implementation
		return BP_ENOBPI;
	}
}

al_bp_error_t al_bp_send(al_bp_handle_t handle,
		al_bp_reg_id_t regid,
		al_bp_bundle_spec_t* spec,
		al_bp_bundle_payload_t* payload,
		al_bp_bundle_id_t* id)
{
	al_bp_endpoint_id_t none;
	if (spec == NULL)
		return BP_ENULLPNTR;
	if (payload == NULL)
		return BP_ENULLPNTR;
	if (id == NULL)
		return BP_ENULLPNTR;

	if (spec->replyto.uri == NULL || strlen(spec->replyto.uri) == 0) {
		al_bp_get_none_endpoint(&none);
		al_bp_copy_eid(&(spec->replyto), &none);
	}

	switch (al_bp_get_implementation())
	{
	case BP_DTN:
		return bp_dtn_send(handle, regid, spec, payload, id);

	case BP_ION:
		return bp_ion_send(handle, regid, spec, payload, id);

	case BP_IBR:
		return bp_ibr_send(handle, regid, spec, payload, id);

	default: // cannot find bundle protocol implementation
		return BP_ENOBPI;
	}
}

al_bp_error_t al_bp_recv(al_bp_handle_t handle,
		al_bp_bundle_spec_t* spec,
		al_bp_bundle_payload_location_t location,
		al_bp_bundle_payload_t* payload,
		al_bp_timeval_t timeout)
{
	if (spec == NULL)
		return BP_ENULLPNTR;
	if (payload == NULL)
		return BP_ENULLPNTR;

	switch (al_bp_get_implementation())
	{
	case BP_DTN:
		return bp_dtn_recv(handle, spec, location, payload, timeout);

	case BP_ION:
		return bp_ion_recv(handle, spec, location, payload, timeout);

	case BP_IBR:
		return bp_ibr_recv(handle, spec, location, payload, timeout);

	default: // cannot find bundle protocol implementation
		return BP_ENOBPI;
	}
}

al_bp_error_t al_bp_close(al_bp_handle_t handle)
{
	switch (al_bp_get_implementation())
	{
	case BP_DTN:
		return bp_dtn_close(handle);

	case BP_ION:
		return bp_ion_close(handle);

	case BP_IBR:
		return bp_ibr_close(handle);

	default: // cannot find bundle protocol implementation
		return BP_ENOBPI;
	}
}

al_bp_error_t al_bp_parse_eid_string(al_bp_endpoint_id_t* eid, const char* str)
{
	if (eid == NULL)
		return BP_ENULLPNTR;

	switch (al_bp_get_implementation())
	{
	case BP_DTN:
		return bp_dtn_parse_eid_string(eid, str);

	case BP_ION:
		return bp_ion_parse_eid_string(eid, str);

	case BP_IBR:
		return bp_ibr_parse_eid_string(eid, str);

	default: // cannot find bundle protocol implementation
		return BP_ENOBPI;
	}
}

void al_bp_copy_eid(al_bp_endpoint_id_t* dst, al_bp_endpoint_id_t* src)
{
	switch (al_bp_get_implementation())
	{
	case BP_DTN:
		bp_dtn_copy_eid(dst, src);
		break;

	case BP_ION:
		bp_ion_copy_eid(dst, src);
		break;

	case BP_IBR:
		bp_ibr_copy_eid(dst, src);
		break;

	default: // cannot find bundle protocol implementation
		return ;
	}
}

const char* al_bp_get_none_endpoint_string()
{
	return dtn_none;
}

al_bp_error_t al_bp_get_none_endpoint(al_bp_endpoint_id_t * eid_none)
{
	switch (al_bp_get_implementation())
	{
	case BP_DTN:
		return bp_dtn_parse_eid_string(eid_none, dtn_none);

	case BP_ION:
		return bp_ion_parse_eid_string(eid_none, dtn_none);

	case BP_IBR:
		return bp_ibr_parse_eid_string(eid_none, dtn_none);

	default: // cannot find bundle protocol implementation
		return BP_ENOBPI;
	}
}

al_bp_error_t al_bp_set_payload(al_bp_bundle_payload_t* payload,
		al_bp_bundle_payload_location_t location,
		char* val, int len)
{
	if (payload == NULL)
		return BP_ENULLPNTR;

	switch (al_bp_get_implementation())
	{
	case BP_DTN:
		return bp_dtn_set_payload(payload, location, val, len);

	case BP_ION:
		return bp_ion_set_payload(payload, location, val, len);

	case BP_IBR:
		return bp_ibr_set_payload(payload, location, val, len);

	default: // cannot find bundle protocol implementation
		return BP_ENOBPI;
	}
}

void al_bp_free_extension_blocks(al_bp_bundle_spec_t* spec)
{
	switch (al_bp_get_implementation())
	{
	case BP_DTN:
		bp_dtn_free_extension_blocks(spec);
		break;

	case BP_ION:
		bp_ion_free_extension_blocks(spec);
		break;

	case BP_IBR:
		bp_ibr_free_extension_blocks(spec);
		break;

	default: // cannot find bundle protocol implementation
		return ;
	}
}

void al_bp_free_metadata_blocks(al_bp_bundle_spec_t* spec)
{
	switch (al_bp_get_implementation())
	{
	case BP_DTN:
		bp_dtn_free_metadata_blocks(spec);
		break;

	case BP_ION:
		bp_ion_free_metadata_blocks(spec);
		break;

	case BP_IBR:
		bp_ibr_free_metadata_blocks(spec);
		break;

	default: // cannot find bundle protocol implementation
		return ;
	}
}

void al_bp_free_payload(al_bp_bundle_payload_t* payload)
{
	payload->status_report = NULL;
	switch (al_bp_get_implementation())
	{
	case BP_DTN:
		bp_dtn_free_payload(payload);
		break;

	case BP_ION:
		bp_ion_free_payload(payload);
		break;

	case BP_IBR:
		bp_ibr_free_payload(payload);
		break;

	default: // cannot find bundle protocol implementation
		return ;
	}
}

/**
 * Remember to free return value
 */
char* al_bp_status_report_flag_to_str(al_bp_status_report_flags_t flag)
{
	static char temp[256];
	temp[0] = '\0';
	if (flag & BP_STATUS_RECEIVED)
		strcat(temp, "RECEIVED, ");
	if (flag & BP_STATUS_CUSTODY_ACCEPTED)
		strcat(temp, "CUSTODY_ACCEPTED, ");
	if (flag & BP_STATUS_FORWARDED)
		strcat(temp, "FORWARDED, ");
	if (flag & BP_STATUS_DELIVERED)
		strcat(temp, "DELIVERED, ");
	if (flag & BP_STATUS_DELETED)
		strcat(temp, "DELETED");
	if (flag & BP_STATUS_ACKED_BY_APP)
		strcat(temp, "ACKED_BY_APP, ");
	temp[strlen(temp) - 2] = '\0';
	return temp;
}

const char* al_bp_status_report_reason_to_str(al_bp_status_report_reason_t err)
{
	switch (err) {
	case BP_SR_REASON_NO_ADDTL_INFO:
		return "no additional information";

	case BP_SR_REASON_LIFETIME_EXPIRED:
		return "lifetime expired";

	case BP_SR_REASON_FORWARDED_UNIDIR_LINK:
		return "forwarded over unidirectional link";

	case BP_SR_REASON_TRANSMISSION_CANCELLED:
		return "transmission cancelled";

	case BP_SR_REASON_DEPLETED_STORAGE:
		return "depleted storage";

	case BP_SR_REASON_ENDPOINT_ID_UNINTELLIGIBLE:
		return "endpoint id unintelligible";

	case BP_SR_REASON_NO_ROUTE_TO_DEST:
		return "no known route to destination";

	case BP_SR_REASON_NO_TIMELY_CONTACT:
		return "no timely contact";

	case BP_SR_REASON_BLOCK_UNINTELLIGIBLE:
		return "block unintelligible";

	default:
		return "(unknown reason)";
	}
}
char * al_bp_strerror(int err){
	switch(err) {
	case BP_SUCCESS: 		return "success";
	case BP_EINVAL: 		return "invalid argument";
	case BP_ENULLPNTR:		return "operation on a null pointer";
	case BP_ECONNECT: 		return "error connecting to server";
	case BP_ETIMEOUT: 		return "operation timed out";
	case BP_ESIZE: 			return "payload too large";
	case BP_ENOTFOUND: 		return "not found";
	case BP_EINTERNAL: 		return "internal error";
	case BP_EBUSY:     		return "registration already in use";
	case BP_ENOSPACE:		return "no storage space";
	case BP_ENOTIMPL:		return "function not yet implemented";
	case BP_ENOBPI:			return "cannot find bundle protocol implementation";
	case BP_EATTACH:		return "cannot attach bundle protocol";
	case BP_EBUILDEID:		return "cannot build local eid";
	case BP_EOPEN :			return "cannot open the connection whit bp";
	case BP_EREG:			return "cannot register the eid";
	case BP_EPARSEEID:		return "cannot parse the endpoint string";
	case BP_ESEND:			return "cannot send Bundle";
	case BP_EUNREG:			return "cannot unregister eid";
	case BP_ERECV:			return "cannot receive bundle";
	case BP_ERECVINT:		return "receive bundle interrupted";
	case -1:				return "(invalid error code -1)";
	default:		   		break;
	}
	// there's a small race condition here in case there are two
	// simultaneous calls that will clobber the same buffer, but this
	// should be rare and the worst that happens is that the output
	// string is garbled
	static char buf[128];
	snprintf(buf, sizeof(buf), "(unknown error %d)", err);
	return buf;
}


/********************************************************************
 *
 *             HIGHER LEVEL FUNCTIONS
 *
 ********************************************************************
 */

al_bp_error_t al_bp_bundle_send(al_bp_handle_t handle,
		al_bp_reg_id_t regid,
		al_bp_bundle_object_t bundle_object)
{
	memset(bundle_object.id, 0, sizeof(al_bp_bundle_id_t));
	return al_bp_send(handle, regid, bundle_object.spec, bundle_object.payload, bundle_object.id);
}
al_bp_error_t al_bp_bundle_receive(al_bp_handle_t handle,
		al_bp_bundle_object_t * bundle_object,
		al_bp_bundle_payload_location_t payload_location,
		al_bp_timeval_t timeout)
{
	al_bp_free_payload(bundle_object->payload);
	al_bp_free_extension_blocks(bundle_object->spec);
	al_bp_free_metadata_blocks(bundle_object->spec);
	memset(bundle_object->id, 0, sizeof(al_bp_bundle_id_t));
	memset(bundle_object->payload, 0, sizeof(al_bp_bundle_payload_t));
	memset(bundle_object->spec, 0, sizeof(al_bp_bundle_spec_t));
	return al_bp_recv(handle, bundle_object->spec, payload_location, bundle_object->payload, timeout);
}

al_bp_error_t al_bp_bundle_create(al_bp_bundle_object_t * bundle_object)
{
	if (bundle_object == NULL)
		return BP_ENULLPNTR;
	
	// ID
	bundle_object->id = (al_bp_bundle_id_t*) malloc(sizeof(al_bp_bundle_id_t));
	memset(bundle_object->id, 0, sizeof(al_bp_bundle_id_t));
	
	// SPEC
	bundle_object->spec = (al_bp_bundle_spec_t*) malloc(sizeof(al_bp_bundle_spec_t));
	memset(bundle_object->spec, 0, sizeof(al_bp_bundle_spec_t));
	bundle_object->spec->blocks.blocks_val = (al_bp_extension_block_t*) malloc(sizeof(al_bp_extension_block_t));
	bundle_object->spec->metadata.metadata_val = (al_bp_extension_block_t*) malloc(sizeof(al_bp_extension_block_t));
	memset(bundle_object->spec->blocks.blocks_val, 0, sizeof(al_bp_extension_block_t));
	memset(bundle_object->spec->metadata.metadata_val, 0, sizeof(al_bp_extension_block_t));
	
	// PAYLOAD
	bundle_object->payload = (al_bp_bundle_payload_t*) malloc(sizeof(al_bp_bundle_payload_t));
	memset(bundle_object->payload, 0, sizeof(al_bp_bundle_payload_t));
	
	return BP_SUCCESS;
}
al_bp_error_t al_bp_bundle_free(al_bp_bundle_object_t * bundle_object)
{
	if (bundle_object == NULL)
		return BP_ENULLPNTR;
	al_bp_free_payload(bundle_object->payload);
	al_bp_free_extension_blocks(bundle_object->spec);
	al_bp_free_metadata_blocks(bundle_object->spec);
	if (bundle_object->id != NULL) {
		free(bundle_object->id);
		bundle_object->id = NULL;
	}
	if (bundle_object->spec != NULL) {
		free(bundle_object->spec);
		bundle_object->spec = NULL;
	}
	if (bundle_object->payload != NULL) {
		free(bundle_object->payload);
		bundle_object->payload = NULL;
	}
	return BP_SUCCESS;
}

al_bp_error_t al_bp_bundle_get_id(al_bp_bundle_object_t bundle_object, al_bp_bundle_id_t ** bundle_id)
{
	if (bundle_object.id == NULL)
		return BP_ENULLPNTR;
	*bundle_id = bundle_object.id;
	return BP_SUCCESS;
}
al_bp_error_t al_bp_bundle_get_payload_location(al_bp_bundle_object_t bundle_object, al_bp_bundle_payload_location_t * location)
{
	if (bundle_object.payload == NULL)
		return BP_ENULLPNTR;

	* location = bundle_object.payload->location;

	return BP_SUCCESS;
}

al_bp_error_t al_bp_bundle_set_payload_location(al_bp_bundle_object_t * bundle_object, al_bp_bundle_payload_location_t location)
{
	if (bundle_object == NULL)
		return BP_ENULLPNTR;
	if (bundle_object->payload == NULL)
		return BP_ENULLPNTR;

	bundle_object->payload->location = location;
	return BP_SUCCESS;
}
al_bp_error_t al_bp_bundle_get_payload_size(al_bp_bundle_object_t bundle_object, u32_t * size)
{
	if (bundle_object.payload == NULL)
		return BP_ENULLPNTR;

	if (bundle_object.payload->location == BP_PAYLOAD_MEM)
	{
		if (bundle_object.payload->buf.buf_val != NULL)
		{
			* size = bundle_object.payload->buf.buf_len;
			return BP_SUCCESS;
		}
		else
		{ // buffer is null
			return BP_ENULLPNTR;
		}
	}
	else if (bundle_object.payload->location == BP_PAYLOAD_FILE
			|| bundle_object.payload->location == BP_PAYLOAD_TEMP_FILE)
	{
		if (bundle_object.payload->filename.filename_val == NULL) // filename is null
			return BP_ENULLPNTR;
		struct stat st;
		memset(&st, 0, sizeof(st));
		if (stat(bundle_object.payload->filename.filename_val, &st) < 0)
		{
			perror("Error in checking bundle payload file");
			return BP_EINTERNAL;
		}
		*size = st.st_size;
		return BP_SUCCESS;

	}
	else { // wrong payload location
		return BP_EINTERNAL;
	}
}

al_bp_error_t al_bp_bundle_get_payload_file(al_bp_bundle_object_t bundle_object, char_t ** filename, u32_t * filename_len)
{
	if (bundle_object.payload->location == BP_PAYLOAD_FILE
			|| bundle_object.payload->location == BP_PAYLOAD_TEMP_FILE)
	{
		if (bundle_object.payload->filename.filename_val == NULL) // filename is null
		{
			return BP_ENULLPNTR;
		}
		if (bundle_object.payload->filename.filename_len <= 0) // filename size error
			return BP_EINTERNAL;
		* filename_len = bundle_object.payload->filename.filename_len;
		(* filename) = bundle_object.payload->filename.filename_val;
		return BP_SUCCESS;
	}
	else // bundle location is not file
		return BP_EINVAL;
}
al_bp_error_t al_bp_bundle_get_payload_mem(al_bp_bundle_object_t bundle_object, char ** buf, u32_t * buf_len)
{
	if (bundle_object.payload->location == BP_PAYLOAD_MEM)
	{
		if (bundle_object.payload->buf.buf_val != NULL)
		{
			*buf_len = bundle_object.payload->buf.buf_len;
			(*buf) = bundle_object.payload->buf.buf_val;
			return BP_SUCCESS;
		}
		else
		{ // buffer is null
			return BP_ENULLPNTR;
		}
	}
	else // bundle location is not memory
		return BP_EINVAL;
}

al_bp_error_t al_bp_bundle_set_payload_file(al_bp_bundle_object_t * bundle_object, char_t * filename, u32_t filename_len)
{
	if (bundle_object == NULL)
		return BP_ENULLPNTR;
	if (filename == NULL)
		return BP_ENULLPNTR;
	if (filename_len <= 0 )
		return BP_EINVAL;

	al_bp_error_t err;

	if (bundle_object->payload == NULL){
		al_bp_bundle_payload_t bundle_payload;
		memset(&bundle_payload, 0, sizeof(bundle_payload));
		err = al_bp_set_payload(& bundle_payload, BP_PAYLOAD_FILE, filename, filename_len);
		bundle_object->payload = & bundle_payload;
	}
	else // payload not null
	{
		memset(bundle_object->payload, 0, sizeof(al_bp_bundle_payload_t));
		err = al_bp_set_payload(bundle_object->payload, BP_PAYLOAD_FILE, filename, filename_len);
	}
	return err;
}
al_bp_error_t al_bp_bundle_set_payload_mem(al_bp_bundle_object_t * bundle_object, char * buf, u32_t buf_len)
{
	if (bundle_object == NULL)
		return BP_ENULLPNTR;
	if (buf_len < 0)
		return BP_EINVAL;

	al_bp_error_t err;

	if (bundle_object->payload == NULL){
		al_bp_bundle_payload_t bundle_payload;
		memset(&bundle_payload, 0, sizeof(bundle_payload));
		err = al_bp_set_payload(& bundle_payload, BP_PAYLOAD_MEM, buf, buf_len);
		bundle_object->payload = & bundle_payload;
	}
	else //payload not null
	{
		memset(bundle_object->payload, 0, sizeof(al_bp_bundle_payload_t));
		err = al_bp_set_payload(bundle_object->payload, BP_PAYLOAD_MEM, buf, buf_len);
	}
	return err;

}

al_bp_error_t al_bp_bundle_get_source(al_bp_bundle_object_t bundle_object, al_bp_endpoint_id_t * source)
{
	if (bundle_object.spec == NULL)
		return BP_ENULLPNTR;
	* source = bundle_object.spec->source;
	return BP_SUCCESS;

}
al_bp_error_t al_bp_bundle_set_source(al_bp_bundle_object_t * bundle_object, al_bp_endpoint_id_t source)
{
	if (bundle_object == NULL)
		return BP_ENULLPNTR;
	if (bundle_object->spec == NULL)
		return BP_ENULLPNTR;
	al_bp_copy_eid(&(bundle_object->spec->source), &source);
	return BP_SUCCESS;
}

al_bp_error_t al_bp_bundle_get_dest(al_bp_bundle_object_t bundle_object, al_bp_endpoint_id_t * dest)
{
	if (bundle_object.spec == NULL)
		return BP_ENULLPNTR;
	* dest = bundle_object.spec->dest;
	return BP_SUCCESS;

}
al_bp_error_t al_bp_bundle_set_dest(al_bp_bundle_object_t * bundle_object, al_bp_endpoint_id_t dest)
{
	if (bundle_object == NULL)
		return BP_ENULLPNTR;
	if (bundle_object->spec == NULL)
		return BP_ENULLPNTR;
	al_bp_copy_eid(&(bundle_object->spec->dest), &dest);
	return BP_SUCCESS;
}

al_bp_error_t al_bp_bundle_get_replyto(al_bp_bundle_object_t bundle_object, al_bp_endpoint_id_t * replyto)
{
	if (bundle_object.spec == NULL)
		return BP_ENULLPNTR;
	* replyto = bundle_object.spec->replyto;
	return BP_SUCCESS;

}
al_bp_error_t al_bp_bundle_set_replyto(al_bp_bundle_object_t * bundle_object, al_bp_endpoint_id_t replyto)
{
	if (bundle_object == NULL)
		return BP_ENULLPNTR;
	if (bundle_object->spec == NULL)
		return BP_ENULLPNTR;
	al_bp_copy_eid(&(bundle_object->spec->replyto), &replyto);
	return BP_SUCCESS;
}

al_bp_error_t al_bp_bundle_get_priority(al_bp_bundle_object_t bundle_object,al_bp_bundle_priority_t * priority)
{
	if (bundle_object.spec == NULL)
		return BP_ENULLPNTR;
	* priority = bundle_object.spec->priority;
	return BP_SUCCESS;
}
al_bp_error_t al_bp_bundle_set_priority(al_bp_bundle_object_t * bundle_object, al_bp_bundle_priority_t priority)
{
	if (bundle_object == NULL)
		return BP_ENULLPNTR;
	if(bundle_object->spec == NULL)
		return BP_ENULLPNTR;
	bundle_object->spec->priority = priority;
	return BP_SUCCESS;
}

al_bp_error_t al_bp_bundle_set_unreliable(al_bp_bundle_object_t * bundle_object, boolean_t unreliable)
{
	if (bundle_object == NULL)
		return BP_ENULLPNTR;
	if(bundle_object->spec == NULL)
		return BP_ENULLPNTR;
	bundle_object->spec->unreliable = unreliable;
	return BP_SUCCESS;
}

al_bp_error_t al_bp_bundle_get_unreliable(al_bp_bundle_object_t bundle_object, boolean_t * unreliable)
{
	if (bundle_object.spec == NULL)
		return BP_ENULLPNTR;
	*unreliable = bundle_object.spec->unreliable;
	return BP_SUCCESS;
}

al_bp_error_t al_bp_bundle_set_critical(al_bp_bundle_object_t * bundle_object, boolean_t critical)
{
	if (bundle_object == NULL)
		return BP_ENULLPNTR;
	if(bundle_object->spec == NULL)
		return BP_ENULLPNTR;
	bundle_object->spec->critical = critical;
	return BP_SUCCESS;
}

al_bp_error_t al_bp_bundle_get_critical(al_bp_bundle_object_t bundle_object, boolean_t * critical)
{
	if (bundle_object.spec == NULL)
		return BP_ENULLPNTR;
	*critical = bundle_object.spec->critical;
	return BP_SUCCESS;
}

al_bp_error_t al_bp_bundle_set_flow_label(al_bp_bundle_object_t * bundle_object, u32_t flow_label)
{
	if (bundle_object == NULL)
		return BP_ENULLPNTR;
	if(bundle_object->spec == NULL)
		return BP_ENULLPNTR;
	bundle_object->spec->flow_label = flow_label;
	return BP_SUCCESS;
}

al_bp_error_t al_bp_bundle_get_flow_label(al_bp_bundle_object_t bundle_object, u32_t * flow_label)
{
	if (bundle_object.spec == NULL)
		return BP_ENULLPNTR;
	*flow_label = bundle_object.spec->flow_label;
	return BP_SUCCESS;
}

al_bp_error_t al_bp_bundle_get_expiration(al_bp_bundle_object_t bundle_object, al_bp_timeval_t * exp)
{
	if (bundle_object.spec == NULL)
		return BP_ENULLPNTR;
	*exp = bundle_object.spec->expiration;
	return BP_SUCCESS;
}
al_bp_error_t al_bp_bundle_set_expiration(al_bp_bundle_object_t * bundle_object, al_bp_timeval_t exp)
{
	if (bundle_object == NULL)
		return BP_ENULLPNTR;
	if(bundle_object->spec == NULL)
		return BP_ENULLPNTR;
	bundle_object->spec->expiration = exp;
	return BP_SUCCESS;
}

al_bp_error_t al_bp_bundle_get_creation_timestamp(al_bp_bundle_object_t bundle_object, al_bp_timestamp_t * ts)
{
	if (bundle_object.spec == NULL)
		return BP_ENULLPNTR;
	*ts = bundle_object.spec->creation_ts;
	return BP_SUCCESS;
}
al_bp_error_t al_bp_bundle_set_creation_timestamp(al_bp_bundle_object_t * bundle_object, al_bp_timestamp_t ts)
{
	if (bundle_object == NULL)
		return BP_ENULLPNTR;
	if(bundle_object->spec == NULL)
		return BP_ENULLPNTR;
	bundle_object->spec->creation_ts = ts;
	return BP_SUCCESS;
}

al_bp_error_t al_bp_bundle_get_delivery_opts(al_bp_bundle_object_t bundle_object, al_bp_bundle_delivery_opts_t * dopts)
{
	if (bundle_object.spec == NULL)
		return BP_ENULLPNTR;
	* dopts = bundle_object.spec->dopts;
	return BP_SUCCESS;
}
al_bp_error_t al_bp_bundle_set_delivery_opts(al_bp_bundle_object_t * bundle_object, al_bp_bundle_delivery_opts_t dopts)
{
	if (bundle_object == NULL)
		return BP_ENULLPNTR;
	if(bundle_object->spec == NULL)
		return BP_ENULLPNTR;
	bundle_object->spec->dopts = dopts;
	return BP_SUCCESS;
}

al_bp_error_t al_bp_bundle_get_status_report(al_bp_bundle_object_t bundle_object, al_bp_bundle_status_report_t ** status_report)
{
	*status_report = bundle_object.payload->status_report;
	return BP_SUCCESS;
}

