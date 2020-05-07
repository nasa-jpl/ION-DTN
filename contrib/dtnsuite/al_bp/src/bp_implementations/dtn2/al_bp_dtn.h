/********************************************************
 **  Authors: Michele Rodolfi, michele.rodolfi@studio.unibo.it
 **           Anna d'Amico, anna.damico@studio.unibo.it
 **           Carlo Caini (DTNperf_3 project supervisor), carlo.caini@unibo.it
 **
 **
 **  Copyright (c) 2013, Alma Mater Studiorum, University of Bologna
 **  All rights reserved.
 ** This file contains the definitions of functions interfacing dtn2
 ** (i.e. that will actually call the dtn2 APIs).
 ********************************************************/

/*
 * bp_dtn.h
 *
 * Functions interfacing the dtn2 api
 *
 */

#ifndef BP_DTN_H_
#define BP_DTN_H_

#include <al_bp_types.h>

#ifdef DTN2_IMPLEMENTATION
#ifdef HAVE_CONFIG_H
#  include <dtn-config.h>
#endif
#endif
/* al_bp_error_t is the type of the value returned by bp_dtn_open(). Analogously for other ones*/
al_bp_error_t bp_dtn_open(al_bp_handle_t* handle);

al_bp_error_t bp_dtn_open_with_IP(char * daemon_api_IP, int daemon_api_port, al_bp_handle_t * handle);

al_bp_error_t bp_dtn_errno(al_bp_handle_t handle);


al_bp_error_t bp_dtn_build_local_eid(al_bp_handle_t handle,
								al_bp_endpoint_id_t* local_eid,
								const char* service_tag,
								al_bp_scheme_t type);


al_bp_error_t bp_dtn_register(al_bp_handle_t handle,
						al_bp_reg_info_t* reginfo,
						al_bp_reg_id_t* newregid);

al_bp_error_t bp_dtn_find_registration(al_bp_handle_t handle,
						al_bp_endpoint_id_t * eid,
						al_bp_reg_id_t * newregid);

al_bp_error_t bp_dtn_unregister(al_bp_handle_t handle,al_bp_reg_id_t regid);

al_bp_error_t bp_dtn_send(al_bp_handle_t handle,
					al_bp_reg_id_t regid,
					al_bp_bundle_spec_t* spec,
					al_bp_bundle_payload_t* payload,
					al_bp_bundle_id_t* id);

al_bp_error_t bp_dtn_recv(al_bp_handle_t handle,
					al_bp_bundle_spec_t* spec,
					al_bp_bundle_payload_location_t location,
					al_bp_bundle_payload_t* payload,
					al_bp_timeval_t timeout);

al_bp_error_t bp_dtn_close(al_bp_handle_t handle);

void bp_dtn_copy_eid(al_bp_endpoint_id_t* dst, al_bp_endpoint_id_t* src);


al_bp_error_t bp_dtn_parse_eid_string(al_bp_endpoint_id_t* eid, const char* str);

al_bp_error_t bp_dtn_set_payload(al_bp_bundle_payload_t* payload,
							al_bp_bundle_payload_location_t location,
							char* val, int len);

void bp_dtn_free_payload(al_bp_bundle_payload_t* payload);

void bp_dtn_free_extension_blocks(al_bp_bundle_spec_t* spec);

void bp_dtn_free_metadata_blocks(al_bp_bundle_spec_t* spec);


/**
 * converts DTN errors in the corresponding al_bp_error_t values
 */
al_bp_error_t bp_dtn_error(int err);

#endif /* BP_DTN_H_ */
