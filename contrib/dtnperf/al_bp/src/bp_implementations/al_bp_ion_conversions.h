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
************
 *
 * al_ion_conversion.h
 *
 * Conversions bp abstract types to ion types and viceversa
 *
 ************
*/
#ifndef AL_al_ion_CONVERSIONS_H_
#define AL_al_ion_CONVERSIONS_H_

#ifdef ION_IMPLEMENTATION

#include "../al_bp_types.h"

#include <bpP.h>


/*
 * These functions convert bp abstract types in ion types and viceversa
 * The prefix al_ion means the function takes a bp abstract type in and returns a ion type
 * so the conversion is bp -> ion
 * The prefix ion_al means the function takes a ion type in and returns a bp abstract type
 * so the conversion is ion -> bp
 */

BpSAP al_ion_handle(al_bp_handle_t handle);
al_bp_handle_t ion_al_handle(BpSAP handle);

char * al_ion_endpoint_id(al_bp_endpoint_id_t endpoint_id);
al_bp_endpoint_id_t ion_al_endpoint_id(char * endpoint_id);

BpTimestamp al_ion_timestamp(al_bp_timestamp_t timestamp);
al_bp_timestamp_t ion_al_timestamp(BpTimestamp timestamp);

DtnTime al_ion_timeval(al_bp_timeval_t timeval);
al_bp_timeval_t ion_al_timeval(DtnTime timeval);

unsigned char al_ion_bundle_srrFlags(al_bp_bundle_delivery_opts_t bundle_delivery_opts);
al_bp_bundle_delivery_opts_t ion_al_bundle_srrFlags(unsigned char ssrFlag);

/* *
 * This conversion convert only the al_bundle_priority_enum
 * The ordinal number is setted in bp_ion_send()
 * */

int al_ion_bundle_priority(al_bp_bundle_priority_t bundle_priority);
al_bp_bundle_priority_t ion_al_bunlde_priority(int bundle_priority);

BpSrReason al_ion_status_report_reason(al_bp_status_report_reason_t status_report_reason);
al_bp_status_report_reason_t ion_al_status_report_reason(BpSrReason status_report_reason);

int al_ion_status_report_flags(al_bp_status_report_flags_t status_repot_flags);
al_bp_status_report_flags_t ion_al_status_report_flags(int status_repot_flags);

BpStatusRpt al_ion_bundle_status_report(al_bp_bundle_status_report_t bundle_status_report);
al_bp_bundle_status_report_t ion_al_bundle_status_report(BpStatusRpt bundle_status_report);

al_bp_bundle_payload_t ion_al_bundle_payload(Payload bundle_payload,al_bp_bundle_payload_location_t location,char * filename);
Payload al_ion_bundle_payload(al_bp_bundle_payload_t bundle_payload, int  priority, BpExtendedCOS extendedCOS);
#endif  /* AL_BP_DTN_CONVERSIONS_H_ */
#endif /* ION_IMPLEMENTAION */
