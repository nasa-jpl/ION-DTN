/********************************************************
 **  Authors: Davide Pallotti, davide.pallotti@studio.unibo.it
 **           Carlo Caini (DTNperf_3 project supervisor), carlo.caini@unibo.it
 **
 **
 **  Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 **  All rights reserved.
 ** This file contains the definitions of the functions interfacing IBR-DTN
 ** (i.e. that will actually call the IBR-DTN API).
 ********************************************************/

/*
 * al_bp_ibr.h
 *
 * Functions interfacing the IBR-DTN API
 *
 */

#ifndef BP_IBR_H_
#define BP_IBR_H_

#include <al_bp_types.h>

#ifdef __cplusplus
/* prevents the C++ compiler from performing name mangling on the functions defined in al_bp_ibr.cpp, 
   so that they can be successfully linked to the existing C code */
extern "C" {
#endif

al_bp_error_t bp_ibr_errno(al_bp_handle_t handle);

al_bp_error_t bp_ibr_open(al_bp_handle_t* handle_p);

al_bp_error_t bp_ibr_open_with_IP(const char* daemon_api_IP, 
                                  int daemon_api_port, 
                                  al_bp_handle_t* handle_p);

al_bp_error_t bp_ibr_build_local_eid(al_bp_handle_t handle,
                                     al_bp_endpoint_id_t* local_eid,
                                     const char* service_tag,
                                     al_bp_scheme_t type);

al_bp_error_t bp_ibr_register(al_bp_handle_t handle,
                              al_bp_reg_info_t* reginfo,
                              al_bp_reg_id_t* newregid);

al_bp_error_t bp_ibr_find_registration(al_bp_handle_t handle,
                                       al_bp_endpoint_id_t* eid,
                                       al_bp_reg_id_t* newregid);

al_bp_error_t bp_ibr_send(al_bp_handle_t handle,
                          al_bp_reg_id_t regid,
                          al_bp_bundle_spec_t* spec,
                          al_bp_bundle_payload_t* payload,
                          al_bp_bundle_id_t* id);

al_bp_error_t bp_ibr_recv(al_bp_handle_t handle,
                          al_bp_bundle_spec_t* spec,
                          al_bp_bundle_payload_location_t location,
                          al_bp_bundle_payload_t* payload,
                          al_bp_timeval_t timeout);

al_bp_error_t bp_ibr_unregister(al_bp_handle_t handle, 
                                al_bp_reg_id_t regid,
                                al_bp_endpoint_id_t eid);

al_bp_error_t bp_ibr_close(al_bp_handle_t handle);

/* Utility Functions */

al_bp_error_t bp_ibr_parse_eid_string(al_bp_endpoint_id_t* eid, 
                                      const char* str);

void bp_ibr_copy_eid(al_bp_endpoint_id_t* dst, 
                     al_bp_endpoint_id_t* src);

void bp_ibr_free_extension_blocks(al_bp_bundle_spec_t* spec);

void bp_ibr_free_metadata_blocks(al_bp_bundle_spec_t* spec);

al_bp_error_t bp_ibr_set_payload(al_bp_bundle_payload_t* payload,
                                 al_bp_bundle_payload_location_t location,
                                 char* val, int len);

void bp_ibr_free_payload(al_bp_bundle_payload_t* payload);

#ifdef __cplusplus
} //extern "C" {
#endif

#endif /* BP_IBR_H_ */
