/********************************************************
 **  Authors: Davide Pallotti, davide.pallotti@studio.unibo.it
 **           Carlo Caini (DTNperf_3 project supervisor), carlo.caini@unibo.it
 **
 **
 **  Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 **  All rights reserved.
 ** This file contains the definitions of debug utility functions
 ** used to print the content of variables of Abstraction Layer types
 ********************************************************/

/*
 * al_bp_print.h
 *
 * Debug functions to print Abstraction Layer types defined in al_bp_types.h
 *
 */
 
#ifndef BP_PRINT_H_
#define BP_PRINT_H_

#include <stdio.h>
#include <stdint.h>

#include "../al_bp_types.h"

const char* al_bp_print_tabs(int tabs);

void al_bp_print_null(const char* Tname, const char* name, size_t indent, FILE* stream);

// template<typename T>
// void al_bp_print_array(const T* array, size_t size, void (*al_bp_print_T)(T, const char*, size_t, FILE*), const char* Tname, 
                       // const char* name, size_t indent, FILE* stream);
				  
#define al_bp_print_array(array, size, al_bp_print_T, Tname, name, indent, stream) \
	if ((size) > 0) { \
		fprintf(stream, "%s%s %s[%zu]:\n", al_bp_print_tabs(indent), Tname, name, (size_t) (size)); \
		size_t i; \
		for (i = 0; i < (size); ++i) { \
			char index[23]; sprintf(index, "[%zu]", i); \
			al_bp_print_T((array)[i], index, (indent)+1, stream); \
		} \
	} else fprintf(stream, "%s%s %s[%zu]: (empty)\n", al_bp_print_tabs(indent), Tname, name, (size_t) (size));

void al_bp_print_boolean(int boolean, const char* name, size_t indent, FILE* stream);

void al_bp_print_u32(uint32_t u32, const char* name, size_t indent, FILE* stream);

void al_bp_print_u32_hex(uint32_t u32, const char* name, size_t indent, FILE* stream);

void al_bp_print_str(const char* str, const char* name, size_t indent, FILE* stream);

void al_bp_print_str_len(const char* str, size_t len, const char* name, size_t indent, FILE* stream);

void al_bp_print_endpoint_id(al_bp_endpoint_id_t endpoint_id, const char* name, size_t indent, FILE* stream);

void al_bp_print_timeval(al_bp_timeval_t timeval, const char* name, size_t indent, FILE* stream);

void al_bp_print_timestamp(al_bp_timestamp_t timestamp, const char* name, size_t indent, FILE* stream);

void al_bp_print_reg_token(al_bp_reg_token_t reg_token, const char* name, size_t indent, FILE* stream);

void al_bp_print_reg_id(al_bp_reg_id_t reg_id, const char* name, size_t indent, FILE* stream);

void al_bp_print_reg_info(al_bp_reg_info_t reg_id, const char* name, size_t indent, FILE* stream);

void al_bp_print_reg_flags(al_bp_reg_flags_t reg_flags, const char* name, size_t indent, FILE* stream);

void al_bp_print_bundle_delivery_opts(al_bp_bundle_delivery_opts_t bundle_delivery_opts, const char* name, size_t indent, FILE* stream);

void al_bp_print_bundle_priority_enum(al_bp_bundle_priority_enum bundle_priority_enum, const char* name, size_t indent, FILE* stream);

void al_bp_print_bundle_priority(al_bp_bundle_priority_t bundle_priority, const char* name, size_t indent, FILE* stream);

void al_bp_print_extension_block(al_bp_extension_block_t extension_block, const char* name, size_t indent, FILE* stream);

void al_bp_print_bundle_spec(al_bp_bundle_spec_t bundle_spec, const char* name, size_t indent, FILE* stream);

void al_bp_print_bundle_payload_location(al_bp_bundle_payload_location_t bundle_payload_location, const char* name, size_t indent, FILE* stream);

void al_bp_print_status_report_reason(al_bp_status_report_reason_t status_report_reason, const char* name, size_t indent, FILE* stream);

void al_bp_print_status_report_flags(al_bp_status_report_flags_t status_report_flags, const char* name, size_t indent, FILE* stream);

void al_bp_print_bundle_id(al_bp_bundle_id_t bundle_id, const char* name, size_t indent, FILE* stream);

void al_bp_print_bundle_status_report(al_bp_bundle_status_report_t bundle_status_report, const char* name, size_t indent, FILE* stream);

void al_bp_print_bundle_payload(al_bp_bundle_payload_t bundle_payload, const char* name, size_t indent, FILE* stream);

void al_bp_print_bundle_object(al_bp_bundle_object_t bundle_object, const char* name, size_t indent, FILE* stream);


#endif /* BP_PRINT_H_ */
