/********************************************************
 **  Authors: Michele Rodolfi, michele.rodolfi@studio.unibo.it
 **           Anna d'Amico, anna.damico@studio.unibo.it
 **           Carlo Caini (DTNperf_3 project supervisor), carlo.caini@unibo.it
 **
 **
 **  Copyright (c) 2013, Alma Mater Studiorum, University of Bologna
 **  All rights reserved.
 ** This file contains the definitions of functions interfacing ION
 ** (i.e. that will actually call the ION APIs).
 ********************************************************/

/*
 * bp_ion.h
 *
 * Functions interfacing the ion api
 *
 */

#ifndef BP_ION_H_
#define BP_ION_H_

#include <al_bp_types.h>

#define CBHESCHEMENAME "ipn"
#define DTN2SCHEMENAME "dtn"

al_bp_error_t bp_ion_attach();

al_bp_error_t bp_ion_open_with_IP(char * daemon_api_IP,
								int daemon_api_port,
								al_bp_handle_t * handle);

al_bp_error_t bp_ion_errno(al_bp_handle_t handle);

/* The local eid is built with different rule according to the type:
 * Client :
 * 		if the eid_destination is ipn:nn.ns then the local eid is:
 * 			ipn:ownNodeNbr.ownPid
 * 		if the eid_destination is dtn://name.dtn then the local eid is:
 * 			dtn://ownNodeNbr.dtn/service_tag
 * 	Server-CBHE or Monitor-CBHE :
 * 		the service tag is converted to long unsigned integer and the local eid is:
 * 			ipn:ownNodeNbr.service_tag
 * 	Server-DTN or Monitor-DTN:
 * 		the local eid is :
 * 			dtn://ownNodeNbr.dtn/service_tag
 * */
al_bp_error_t bp_ion_build_local_eid(al_bp_endpoint_id_t* local_eid,
								const char* service_tag,
								al_bp_scheme_t type);

/* This API register the eid and open the connection initializing the handle*/
al_bp_error_t bp_ion_register(al_bp_handle_t * handle,
                        al_bp_reg_info_t* reginfo,
                        al_bp_reg_id_t* newregid);

al_bp_error_t bp_ion_find_registration(al_bp_handle_t handle,
						al_bp_endpoint_id_t * eid,
						al_bp_reg_id_t * newregid);

al_bp_error_t bp_ion_unregister(al_bp_endpoint_id_t eid);

al_bp_error_t bp_ion_send(al_bp_handle_t handle,
                    al_bp_reg_id_t regid,
                    al_bp_bundle_spec_t* spec,
                    al_bp_bundle_payload_t* payload,
                    al_bp_bundle_id_t* id);

al_bp_error_t bp_ion_recv(al_bp_handle_t handle,
                    al_bp_bundle_spec_t* spec,
                    al_bp_bundle_payload_location_t location,
                    al_bp_bundle_payload_t* payload,
                    al_bp_timeval_t timeout);

al_bp_error_t bp_ion_close(al_bp_handle_t handle);

void bp_ion_copy_eid(al_bp_endpoint_id_t* dst, al_bp_endpoint_id_t* src);

al_bp_error_t bp_ion_parse_eid_string(al_bp_endpoint_id_t* eid, const char* str);

al_bp_error_t bp_ion_set_payload(al_bp_bundle_payload_t* payload,
                           al_bp_bundle_payload_location_t location,
                           char* val, int len);

void bp_ion_free_payload(al_bp_bundle_payload_t* payload);

void bp_ion_free_extension_blocks(al_bp_bundle_spec_t* spec);

void bp_ion_free_metadata_blocks(al_bp_bundle_spec_t* spec);

/**
 * converts DTN errors in the corresponding al_bp_error_t values
 */
al_bp_error_t bp_ion_error(int err);

#endif /* BP_ION_H_ */
