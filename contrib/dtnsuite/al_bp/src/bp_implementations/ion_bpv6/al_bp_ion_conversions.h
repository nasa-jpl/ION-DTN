/********************************************************
 **  Authors: Michele Rodolfi, michele.rodolfi@studio.unibo.it
 **           Anna d'Amico, anna.damico@studio.unibo.it
 **           Carlo Caini (DTNperf_3 project supervisor), carlo.caini@unibo.it
 **
 **
 **  Copyright (c) 2013, Alma Mater Studiorum, University of Bologna
 **  All rights reserved.
 **
 ** This files contains the definitions of the functions that convert bp abstract types in ION types and vice versa.
 ** See the comment below.
 **
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

#include <al_bp_types.h>

#include <bpP.h>


/*
 * These functions convert bp abstract types in ion types and viceversa
 * The prefix al_ion means the function takes a bp abstract type in and returns a ion type
 * so the conversion is bp -> ion
 * The prefix ion_al means the function takes a ion type in and returns a bp abstract type
 * so the conversion is ion -> bp
 */

/**
 * Encapsulates in a structure 2 different SAPs:
 * recv is the SAP used to receive bundles (initiated by bp_open())
 * source is the SAP used to send bundles (initiated by bp_open_source()).
 * If using ION < 3.3.0, only recv is used both to send and receive bundles
 * Introducted with ION 3.3.0
 */
struct al_ion_handle_st{
	BpSAP *recv;
	BpSAP *source;
};
typedef struct al_ion_handle_st * al_ion_handle_t;

al_ion_handle_t al_ion_handle(al_bp_handle_t handle);
al_bp_handle_t ion_al_handle(al_ion_handle_t handle);

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
Payload al_ion_bundle_payload(al_bp_bundle_payload_t bundle_payload, int  priority, BpAncillaryData extendedCOS);

/* Author: Laura Mazzuca, laura.mazzuca@studio.unibo.it
 *
 * This function converts al_bp metadata type into ion metadata type, BPAncillaryData.
 *
 */
int al_ion_metadata(u32_t metadata_len, al_bp_extension_block_t *metadata_val, BpAncillaryData* extendedCOS);

/* Author: Laura Mazzuca, laura.mazzuca@studio.unibo.it
 *
 * This function converts the metadata found in the ion bundle structure BPDelivery
 * into abstract layer metadata
 */
void ion_al_metadata(BpDelivery dlvBundle, al_bp_bundle_spec_t * spec);
#endif  /* AL_BP_DTN_CONVERSIONS_H_ */
#endif /* ION_IMPLEMENTAION */
