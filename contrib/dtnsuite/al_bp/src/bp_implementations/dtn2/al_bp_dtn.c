/********************************************************
 **  Authors: Michele Rodolfi, michele.rodolfi@studio.unibo.it
 **           Anna d'Amico, anna.damico@studio.unibo.it
 **	      Andrea Bisacchi, andrea.bisacchi5@studio.unibo.it
 **           Carlo Caini (DTNperf_3 project supervisor), carlo.caini@unibo.it
 **
 **
 **  Copyright (c) 2013, Alma Mater Studiorum, University of Bologna
 **  All rights reserved.
 ** This file contains the functions interfacing dtn2
 ** (i.e. that will actually call the dtn2 APIs).
 ********************************************************/

/*
 * bp_dtn.c
 *
 */
#include "al_bp_dtn.h"

/*
 * if there is the DTN2 implementation on the
 * machine the API are actually implemented; otherwise they are just dummy functions
 * to avoid compilation errors.
 */
#ifdef DTN2_IMPLEMENTATION

#include "al_bp_dtn_conversions.h"

al_bp_error_t bp_dtn_open(al_bp_handle_t* handle)
{
	dtn_handle_t dtn_handle = al_dtn_handle(*handle);
	int result = dtn_open(& dtn_handle);
	* handle = dtn_al_handle(dtn_handle);
	return bp_dtn_error(result);
}

al_bp_error_t bp_dtn_open_with_IP(char * daemon_api_IP, int daemon_api_port, al_bp_handle_t * handle)
{
	dtn_handle_t dtn_handle = al_dtn_handle(*handle);
	int result = dtn_open_with_IP(daemon_api_IP, daemon_api_port, & dtn_handle);
	* handle = dtn_al_handle(dtn_handle);
	return bp_dtn_error(result);
}

al_bp_error_t bp_dtn_errno(al_bp_handle_t handle)
{
	dtn_handle_t dtn_handle = al_dtn_handle(handle);
	int result = dtn_errno(dtn_handle);
	handle = dtn_al_handle(dtn_handle);
	return bp_dtn_error(result);
}

/**
 * Build a local eid for DTN2 implementation
 * if type == CBHE_SCHEME, service_tag must be ipn_local_number.service_number
 */
al_bp_error_t bp_dtn_build_local_eid(al_bp_handle_t handle,
								al_bp_endpoint_id_t* local_eid,
								const char* service_tag,
								al_bp_scheme_t type)
{
	if (type == DTN_SCHEME)
	{
		dtn_handle_t dtn_handle = al_dtn_handle(handle);
		dtn_endpoint_id_t dtn_local_eid = al_dtn_endpoint_id(*local_eid);
		int result = dtn_build_local_eid(dtn_handle, & dtn_local_eid, service_tag);
		handle = dtn_al_handle(dtn_handle);
		* local_eid = dtn_al_endpoint_id(dtn_local_eid);
		return bp_dtn_error(result);
	}
	else if (type == CBHE_SCHEME)
	{
		//check if string is correctly formatted (it must be ipn_local_number.service_number)
		int ipn_local_number = -1, service_number = -1;
		if (sscanf(service_tag, "%d.%d", &ipn_local_number, &service_number) != 2)
			return BP_EPARSEEID;
		if (ipn_local_number < 0 || service_number < 0)
			return BP_EPARSEEID;
		sprintf(local_eid->uri, "ipn:%s", service_tag);
		return BP_SUCCESS;
	}
	else
		return BP_EINVAL;

}

al_bp_error_t bp_dtn_register(al_bp_handle_t handle,
						al_bp_reg_info_t* reginfo,
						al_bp_reg_id_t* newregid)
{
	dtn_handle_t dtn_handle = al_dtn_handle(handle);
	dtn_reg_info_t dtn_reginfo = al_dtn_reg_info(*reginfo);
	dtn_reg_id_t dtn_newregid = al_dtn_reg_id(*newregid);
	int result = dtn_register(dtn_handle, & dtn_reginfo, & dtn_newregid);
	handle = dtn_al_handle(dtn_handle);
	*reginfo = dtn_al_reg_info(dtn_reginfo);
	*newregid = dtn_al_reg_id(dtn_newregid);
	return bp_dtn_error(result);
}

al_bp_error_t bp_dtn_find_registration(al_bp_handle_t handle,
						al_bp_endpoint_id_t * eid,
						al_bp_reg_id_t * newregid)
{
	dtn_handle_t dtn_handle = al_dtn_handle(handle);
	dtn_endpoint_id_t dtn_eid = al_dtn_endpoint_id(*eid);
	dtn_reg_id_t dtn_newregid = al_dtn_reg_id(*newregid);
	int result = dtn_find_registration(dtn_handle, & dtn_eid, & dtn_newregid);
	handle = dtn_al_handle(dtn_handle);
	* eid = dtn_al_endpoint_id(dtn_eid);
	*newregid = dtn_al_reg_id(dtn_newregid);
	return bp_dtn_error(result);
}

al_bp_error_t bp_dtn_unregister(al_bp_handle_t handle,al_bp_reg_id_t regid){
	dtn_handle_t dtn_handle = al_dtn_handle(handle);
	dtn_reg_id_t dtn_regid = al_dtn_reg_id(regid);
	int result = dtn_unregister(dtn_handle,dtn_regid);
	if(result != 0)
		return BP_EUNREG;
	return BP_SUCCESS;
}

al_bp_error_t bp_dtn_send(al_bp_handle_t handle,
					al_bp_reg_id_t regid,
					al_bp_bundle_spec_t* spec,
					al_bp_bundle_payload_t* payload,
					al_bp_bundle_id_t* id)
{
	dtn_handle_t dtn_handle = al_dtn_handle(handle);
	dtn_reg_id_t dtn_regid = al_dtn_reg_id(regid);
	dtn_bundle_spec_t dtn_spec = al_dtn_bundle_spec(*spec);
	dtn_bundle_payload_t dtn_payload = al_dtn_bundle_payload(*payload);
	dtn_bundle_id_t dtn_id = al_dtn_bundle_id(*id);
	int result = dtn_send(dtn_handle, dtn_regid, & dtn_spec, & dtn_payload, & dtn_id);
	handle = dtn_al_handle(dtn_handle);
	regid = dtn_al_reg_id(dtn_regid);
	*spec = dtn_al_bundle_spec(dtn_spec);
	*payload = dtn_al_bundle_payload(dtn_payload);
	*id = dtn_al_bundle_id(dtn_id);
	return bp_dtn_error(result);
}

al_bp_error_t bp_dtn_recv(al_bp_handle_t handle,
					al_bp_bundle_spec_t* spec,
					al_bp_bundle_payload_location_t location,
					al_bp_bundle_payload_t* payload,
					al_bp_timeval_t timeout)
{
	dtn_handle_t dtn_handle = al_dtn_handle(handle);
	dtn_bundle_spec_t dtn_spec = al_dtn_bundle_spec(*spec);
	dtn_bundle_payload_location_t dtn_location = al_dtn_bundle_payload_location(location);
	dtn_bundle_payload_t dtn_payload = al_dtn_bundle_payload(*payload);
	dtn_timeval_t dtn_timeout = al_dtn_timeval(timeout);
 	int result = dtn_recv(dtn_handle, & dtn_spec, dtn_location, & dtn_payload, dtn_timeout);
	handle = dtn_al_handle(dtn_handle);
	*spec = dtn_al_bundle_spec(dtn_spec);
	location = dtn_al_bundle_payload_location(dtn_location);
	*payload = dtn_al_bundle_payload(dtn_payload);
	timeout = dtn_al_timeval(dtn_timeout);
	return bp_dtn_error(result);
}

al_bp_error_t bp_dtn_close(al_bp_handle_t handle)
{
	dtn_handle_t dtn_handle = al_dtn_handle(handle);
	int result = dtn_close(dtn_handle);
	handle = dtn_al_handle(dtn_handle);
	return bp_dtn_error(result);
}

void bp_dtn_copy_eid(al_bp_endpoint_id_t* dst, al_bp_endpoint_id_t* src)
{
	dtn_endpoint_id_t dtn_dst = al_dtn_endpoint_id(*dst);
	dtn_endpoint_id_t dtn_src = al_dtn_endpoint_id(*src);
	dtn_copy_eid(& dtn_dst, & dtn_src);
	*dst = dtn_al_endpoint_id(dtn_dst);
	*src = dtn_al_endpoint_id(dtn_src);
}

al_bp_error_t bp_dtn_parse_eid_string(al_bp_endpoint_id_t* eid, const char* str)
{
	dtn_endpoint_id_t dtn_eid = al_dtn_endpoint_id(*eid);
	int result =  dtn_parse_eid_string(& dtn_eid, str);
	*eid = dtn_al_endpoint_id(dtn_eid);
	return bp_dtn_error(result);
}

al_bp_error_t bp_dtn_set_payload(al_bp_bundle_payload_t* payload,
							al_bp_bundle_payload_location_t location,
							char* val, int len)
{
	dtn_bundle_payload_t dtn_payload = al_dtn_bundle_payload(*payload);
	dtn_bundle_payload_location_t dtn_location = al_dtn_bundle_payload_location(location);
	int result = dtn_set_payload(& dtn_payload, dtn_location, val, len);
	*payload = dtn_al_bundle_payload(dtn_payload);
	return bp_dtn_error(result);
}

void bp_dtn_free_payload(al_bp_bundle_payload_t* payload)
{
	if (payload->status_report != NULL)
	{
		free(payload->status_report);
		payload->status_report = NULL;
	}
	dtn_bundle_payload_t dtn_payload = al_dtn_bundle_payload(*payload);
	dtn_free_payload(&dtn_payload);
	*payload = dtn_al_bundle_payload(dtn_payload);
}

void bp_dtn_free_extension_blocks(al_bp_bundle_spec_t* spec)
{
        int i;
        for ( i=0; i<spec->blocks.blocks_len; i++ ) {
            //printf("Freeing extension block [%d].data at 0x%08X\n",
                               i, (unsigned int) *(spec->blocks.blocks_val[i].data.data_val));
            if (spec->blocks.blocks_val[i].data.data_val != NULL) {
		    free(spec->blocks.blocks_val[i].data.data_val);
		    spec->blocks.blocks_val[i].data.data_val = NULL;
            }
        }
        if (spec->blocks.blocks_val != NULL) {
		free(spec->blocks.blocks_val);
		spec->blocks.blocks_val = NULL;
        }
        spec->blocks.blocks_len = 0;
}
void bp_dtn_free_metadata_blocks(al_bp_bundle_spec_t* spec)
{
        int i;
        for ( i=0; i<spec->metadata.metadata_len; i++ ) {
            //printf("Freeing metadata block [%d].data at 0x%08X\n",
                               i, (unsigned int) *(spec->metadata.metadata_val[i].data.data_val));
            if (spec->metadata.metadata_val[i].data.data_val != NULL) {
            	free(spec->metadata.metadata_val[i].data.data_val);
            	spec->metadata.metadata_val[i].data.data_val = NULL;
            }
        }
        if (spec->metadata.metadata_val != NULL) {
		free(spec->metadata.metadata_val);
		spec->metadata.metadata_val = NULL;
        }
        spec->metadata.metadata_len = 0;
}

al_bp_error_t bp_dtn_error(int err)
{
	switch (err)
	{
	case DTN_SUCCESS:	return BP_SUCCESS;
	case DTN_EINVAL:	return BP_EINVAL;
	case DTN_ECONNECT:	return BP_ECONNECT;
	case DTN_ETIMEOUT:	return BP_ETIMEOUT;
	case DTN_ESIZE:		return BP_ESIZE;
	case DTN_ENOTFOUND:	return BP_ENOTFOUND;
	case DTN_EINTERNAL: return BP_EINTERNAL;
	case DTN_EBUSY:     return BP_EBUSY;
	case DTN_ENOSPACE:	return BP_ENOSPACE;
	default: 			return -1;
	}
}
/*
 * if there isn't the DTN2 implementation
 * the APIs are just dummy calls
 */
#else
al_bp_error_t bp_dtn_open(al_bp_handle_t* handle)
{
	return BP_ENOTIMPL;
}

al_bp_error_t bp_dtn_open_with_IP(char * daemon_api_IP, int daemon_api_port, al_bp_handle_t * handle)
{
	return BP_ENOTIMPL;
}

al_bp_error_t bp_dtn_errno(al_bp_handle_t handle)
{
	return BP_ENOTIMPL;
}

al_bp_error_t bp_dtn_build_local_eid(al_bp_handle_t handle,
								al_bp_endpoint_id_t* local_eid,
								const char* service_tag,
								al_bp_scheme_t type)
{
	return BP_ENOTIMPL;

}

al_bp_error_t bp_dtn_register(al_bp_handle_t handle,
						al_bp_reg_info_t* reginfo,
						al_bp_reg_id_t* newregid)
{
	return BP_ENOTIMPL;
}

al_bp_error_t bp_dtn_find_registration(al_bp_handle_t handle,
						al_bp_endpoint_id_t * eid,
						al_bp_reg_id_t * newregid)
{
	return BP_ENOTIMPL;
}

al_bp_error_t bp_dtn_unregister(al_bp_handle_t handle,al_bp_reg_id_t regid){
	return BP_ENOTIMPL;
}

al_bp_error_t bp_dtn_send(al_bp_handle_t handle,
					al_bp_reg_id_t regid,
					al_bp_bundle_spec_t* spec,
					al_bp_bundle_payload_t* payload,
					al_bp_bundle_id_t* id)
{
	return BP_ENOTIMPL;
}

al_bp_error_t bp_dtn_recv(al_bp_handle_t handle,
					al_bp_bundle_spec_t* spec,
					al_bp_bundle_payload_location_t location,
					al_bp_bundle_payload_t* payload,
					al_bp_timeval_t timeout)
{
	return BP_ENOTIMPL;
}

al_bp_error_t bp_dtn_close(al_bp_handle_t handle)
{
	return BP_ENOTIMPL;
}

void bp_dtn_copy_eid(al_bp_endpoint_id_t* dst, al_bp_endpoint_id_t* src)
{

}

al_bp_error_t bp_dtn_parse_eid_string(al_bp_endpoint_id_t* eid, const char* str)
{
	return BP_ENOTIMPL;
}

al_bp_error_t bp_dtn_set_payload(al_bp_bundle_payload_t* payload,
							al_bp_bundle_payload_location_t location,
							char* val, int len)
{
	return BP_ENOTIMPL;
}

void bp_dtn_free_payload(al_bp_bundle_payload_t* payload)
{

}

void bp_dtn_free_extension_blocks(al_bp_bundle_spec_t* spec)
{

}

void bp_dtn_free_metadata_blocks(al_bp_bundle_spec_t* spec)
{

}

al_bp_error_t bp_dtn_error(int err)
{
	return BP_ENOTIMPL;
}
#endif /* DTN2_IMPLEMENTATION */
