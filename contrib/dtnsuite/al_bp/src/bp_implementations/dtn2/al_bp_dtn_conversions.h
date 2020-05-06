/********************************************************
 **  Authors: Michele Rodolfi, michele.rodolfi@studio.unibo.it
 **           Anna d'Amico, anna.damico@studio.unibo.it
 **           Carlo Caini (DTNperf_3 project supervisor), carlo.caini@unibo.it
 **
 **
 **  Copyright (c) 2013, Alma Mater Studiorum, University of Bologna
 **  All rights reserved.
 ********************************************************/

/*
 * al_bp_dtn_conversions.h
 *
 * Conversions from bp abstract types to dtn types and viceversa
 *
 */

#ifndef AL_BP_DTN_CONVERSIONS_H_
#define AL_BP_DTN_CONVERSIONS_H_

#ifdef DTN2_IMPLEMENTATION

#include <al_bp_types.h>

#include <dtn_types.h>
#include <dtn_api.h>

/*
 * These functions convert bp abstract types in dtn types and viceversa
 * The prefix bp_dtn means the function takes a bp abstract type in and returns a dtn type
 * so the conversion is bp -> dtn
 * The prefix dtn_bp means the function takes a dtn type in and returns a bp abstract type
 * so the conversion is dtn -> bp
 */

dtn_handle_t al_dtn_handle(al_bp_handle_t handle);
al_bp_handle_t dtn_al_handle(dtn_handle_t handle);

dtn_endpoint_id_t al_dtn_endpoint_id(al_bp_endpoint_id_t endpoint_id);
al_bp_endpoint_id_t dtn_al_endpoint_id(dtn_endpoint_id_t endpoint_id);

dtn_timeval_t al_dtn_timeval(al_bp_timeval_t timeval);
al_bp_timeval_t dtn_al_timeval(dtn_timeval_t);

dtn_timestamp_t al_dtn_timestamp(al_bp_timestamp_t timestamp);
al_bp_timestamp_t dtn_al_timestamp(dtn_timestamp_t timestamp);

dtn_reg_token_t al_dtn_reg_token(al_bp_reg_token_t reg_token);
al_bp_reg_token_t dtn_al_reg_token(dtn_reg_token_t reg_token);

dtn_reg_id_t al_dtn_reg_id(al_bp_reg_id_t reg_id);
al_bp_reg_id_t dtn_al_reg_id(dtn_reg_id_t reg_id);

dtn_reg_info_t al_dtn_reg_info(al_bp_reg_info_t reg_info);
al_bp_reg_info_t dtn_al_reg_info(dtn_reg_info_t reg_info);

dtn_reg_flags_t al_dtn_reg_flags(al_bp_reg_flags_t reg_flags);
al_bp_reg_flags_t dtn_al_reg_flags(dtn_reg_flags_t reg_flags);

dtn_bundle_delivery_opts_t al_dtn_bundle_delivery_opts(al_bp_bundle_delivery_opts_t bundle_delivery_opts);
al_bp_bundle_delivery_opts_t dtn_al_bundle_delivery_opts(dtn_bundle_delivery_opts_t bundle_delivery_opts);

dtn_bundle_priority_t al_dtn_bundle_priority(al_bp_bundle_priority_t bundle_priority);
al_bp_bundle_priority_t dtn_al_bunlde_priority(dtn_bundle_priority_t bundle_priority);

dtn_bundle_spec_t al_dtn_bundle_spec(al_bp_bundle_spec_t bundle_spec);
al_bp_bundle_spec_t dtn_al_bundle_spec(dtn_bundle_spec_t bundle_spec);

dtn_bundle_payload_location_t al_dtn_bundle_payload_location(al_bp_bundle_payload_location_t bundle_payload_location);
al_bp_bundle_payload_location_t dtn_al_bundle_payload_location(dtn_bundle_payload_location_t bundle_payload_location);

dtn_status_report_reason_t al_dtn_status_report_reason(al_bp_status_report_reason_t status_report_reason);
al_bp_status_report_reason_t dtn_al_status_report_reason(dtn_status_report_reason_t status_report_reason);

dtn_status_report_flags_t al_dtn_status_report_flags(al_bp_status_report_flags_t status_repot_flags);
al_bp_status_report_flags_t dtn_al_status_report_flags(dtn_status_report_flags_t status_repot_flags);

dtn_bundle_id_t al_dtn_bundle_id(al_bp_bundle_id_t bundle_id);
al_bp_bundle_id_t dtn_al_bundle_id(dtn_bundle_id_t bundle_id);

dtn_bundle_status_report_t al_dtn_bundle_status_report(al_bp_bundle_status_report_t bundle_status_report);
al_bp_bundle_status_report_t * dtn_al_bundle_status_report(dtn_bundle_status_report_t bundle_status_report);

al_bp_bundle_payload_t dtn_al_bundle_payload(dtn_bundle_payload_t bundle_payload);
dtn_bundle_payload_t al_dtn_bundle_payload(al_bp_bundle_payload_t bundle_payload);

#endif /* AL_BP_DTN_CONVERSIONS_H_ */
#endif /* DTN2_IMPLEMENTATION*/
