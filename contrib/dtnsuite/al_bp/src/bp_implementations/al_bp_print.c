/********************************************************
 **  Authors: Davide Pallotti, davide.pallotti@studio.unibo.it
 **           Carlo Caini (DTNperf_3 project supervisor), carlo.caini@unibo.it
 **
 **
 **  Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 **  All rights reserved.
 ** This file contains the implementation of debug utility functions
 ** used to print the content of variables of Abstraction Layer types
 ********************************************************/

/*
 * al_bp_print.c
 *
 * Implementation of debug functions to print Abstraction Layer types defined in al_bp_types.h
 *
 */
 
#include "al_bp_print.h"

const char* al_bp_print_tabs(int tabs) {
	switch (tabs) {
		case 0: return "";
		case 1: return "    ";
		case 2: return "        ";
		case 3: return "            ";
		case 4: return "                ";
		case 5: return "                    ";
		case 6: return "                        ";
		case 7: return "                            ";
		case 8: return "                                ";
		case 9: return "                                    ";
		default: return "                                        ";
	}
}

void al_bp_print_null(const char* Tname, const char* name, size_t indent, FILE* stream) {
	fprintf(stream, "%s%s %s: %s\n", al_bp_print_tabs(indent), Tname, name, "(null)");
}

// template<typename T>
// void al_bp_print_array(const T* array, size_t size, void (*al_bp_print_T)(T, const char*, size_t, FILE*), const char* Tname, 
                       // const char* name, size_t indent, FILE* stream) {
	// if (size > 0) {
		// fprintf(stream, "%s%s %s[%zu]:\n", al_bp_print_tabs(indent), Tname, name, size);
		// for (size_t i = 0; i < size; ++i) {
			// char name[8]; sprintf(name, "[%zu]", i);
			// al_bp_print_T(array[i], name, indent+1, stream);
		// }
	// } else fprintf(stream, "%s%s %s[%zu]: (empty)\n", al_bp_print_tabs(indent), Tname, name, size);
// }

void al_bp_print_boolean(int boolean, const char* name, size_t indent, FILE* stream) {
	fprintf(stream, "%s%s %s: %s\n", al_bp_print_tabs(indent), "boolean_t", name, boolean ? "TRUE" : "FALSE");
}

void al_bp_print_u32(uint32_t u32, const char* name, size_t indent, FILE* stream) {
	fprintf(stream, "%s%s %s: %u\n", al_bp_print_tabs(indent), "u32_t", name, u32);
}

void al_bp_print_u32_hex(uint32_t u32, const char* name, size_t indent, FILE* stream) {
	fprintf(stream, "%s%s %s: %#X\n", al_bp_print_tabs(indent), "u32_t", name, u32);
}

void al_bp_print_str(const char* str, const char* name, size_t indent, FILE* stream) {
	if (str != NULL) fprintf(stream, "%s%s %s: \"%s\"\n", al_bp_print_tabs(indent), "char*", name, str);
	else fprintf(stream, "%s%s %s: %s\n", al_bp_print_tabs(indent), "char*", name, str);
}

void al_bp_print_str_len(const char* str, size_t len, const char* name, size_t indent, FILE* stream) {
	if (str != NULL) fprintf(stream, "%s%s %s: \"%.*s\"\n", al_bp_print_tabs(indent), "char*", name, (int)len, str);
	else fprintf(stream, "%s%s %s: %s\n", al_bp_print_tabs(indent), "char*", name, str);	
}

void al_bp_print_endpoint_id(al_bp_endpoint_id_t endpoint_id, const char* name, size_t indent, FILE* stream) {
	fprintf(stream, "%s%s %s:\n", al_bp_print_tabs(indent), "al_bp_endpoint_id_t", name);
	al_bp_print_str(endpoint_id.uri, "uri", indent+1, stream);
}

void al_bp_print_timeval(al_bp_timeval_t timeval, const char* name, size_t indent, FILE* stream) {
	fprintf(stream, "%s%s %s: %u\n", al_bp_print_tabs(indent), "al_bp_timeval_t", name, timeval);
}

void al_bp_print_timestamp(al_bp_timestamp_t timestamp, const char* name, size_t indent, FILE* stream) {
	fprintf(stream, "%s%s %s:\n", al_bp_print_tabs(indent), "al_bp_timestamp_t", name);
	al_bp_print_u32(timestamp.secs, "secs", indent+1, stream);
	al_bp_print_u32(timestamp.seqno, "seqno", indent+1, stream);
}

void al_bp_print_reg_token(al_bp_reg_token_t reg_token, const char* name, size_t indent, FILE* stream) {
	fprintf(stream, "%s%s %s: %u\n", al_bp_print_tabs(indent), "al_bp_reg_token_t", name, reg_token);
}

void al_bp_print_reg_id(al_bp_reg_id_t reg_id, const char* name, size_t indent, FILE* stream) {
	fprintf(stream, "%s%s %s: %u\n", al_bp_print_tabs(indent), "al_bp_reg_id_t", name, reg_id);
}

void al_bp_print_reg_info(al_bp_reg_info_t reg_id, const char* name, size_t indent, FILE* stream) {
	fprintf(stream, "%s%s %s:\n", al_bp_print_tabs(indent), "al_bp_reg_info_t", name);
	al_bp_print_endpoint_id(reg_id.endpoint, "endpoint", indent+1, stream);
	al_bp_print_reg_id(reg_id.regid, "regid", indent+1, stream);
	al_bp_print_u32_hex(reg_id.flags, "flags", indent+1, stream);
	al_bp_print_u32_hex(reg_id.replay_flags, "replay_flags", indent+1, stream);
	al_bp_print_timeval(reg_id.expiration, "expiration", indent+1, stream);
	al_bp_print_boolean(reg_id.init_passive, "init_passive", indent+1, stream);
	al_bp_print_reg_token(reg_id.reg_token, "reg_token", indent+1, stream);
	fprintf(stream, "%s%s %s:\n", al_bp_print_tabs(indent+1), "struct", "script");
	al_bp_print_u32(reg_id.script.script_len, "script_len", indent+2, stream);
	al_bp_print_str(reg_id.script.script_val, "script_val", indent+2, stream);
}

void al_bp_print_reg_flags(al_bp_reg_flags_t reg_flags, const char* name, size_t indent, FILE* stream) {
	fprintf(stream, "%s%s %s: %#X\n", al_bp_print_tabs(indent), "al_bp_reg_flags_t", name, (uint32_t) reg_flags);
}

void al_bp_print_bundle_delivery_opts(al_bp_bundle_delivery_opts_t bundle_delivery_opts, const char* name, size_t indent, FILE* stream) {
	fprintf(stream, "%s%s %s: %#X\n", al_bp_print_tabs(indent), "al_bp_bundle_delivery_opts_t", name, (uint32_t) bundle_delivery_opts);
}

void al_bp_print_bundle_priority_enum(al_bp_bundle_priority_enum bundle_priority_enum, const char* name, size_t indent, FILE* stream) {
	fprintf(stream, "%s%s %s: %u\n", al_bp_print_tabs(indent), "al_bp_bundle_priority_enum", name, (uint32_t) bundle_priority_enum);
}

void al_bp_print_bundle_priority(al_bp_bundle_priority_t bundle_priority, const char* name, size_t indent, FILE* stream) {
	fprintf(stream, "%s%s %s:\n", al_bp_print_tabs(indent), "al_bp_bundle_priority_enum", name);
	al_bp_print_bundle_priority_enum(bundle_priority.priority, "priority", indent+1, stream);
	al_bp_print_u32(bundle_priority.ordinal, "ordinal", indent+1, stream);
}

void al_bp_print_extension_block(al_bp_extension_block_t extension_block, const char* name, size_t indent, FILE* stream) {
	fprintf(stream, "%s%s %s:\n", al_bp_print_tabs(indent), "al_bp_extension_block_t", name);
	al_bp_print_u32(extension_block.type, "type", indent+1, stream);
	al_bp_print_u32_hex(extension_block.flags, "flags", indent+1, stream);
	fprintf(stream, "%s%s %s:\n", al_bp_print_tabs(indent+1), "struct", "data");
	al_bp_print_u32(extension_block.data.data_len, "data_len", indent+2, stream);
	al_bp_print_str_len(extension_block.data.data_val, extension_block.data.data_len, "data_val", indent+2, stream);
}

void al_bp_print_bundle_spec(al_bp_bundle_spec_t bundle_spec, const char* name, size_t indent, FILE* stream) {
	fprintf(stream, "%s%s %s:\n", al_bp_print_tabs(indent), "al_bp_bundle_spec_t", name);
	al_bp_print_endpoint_id(bundle_spec.source, "source", indent+1, stream);
	al_bp_print_endpoint_id(bundle_spec.dest, "dest", indent+1, stream);
	al_bp_print_endpoint_id(bundle_spec.replyto, "replyto", indent+1, stream);
	al_bp_print_bundle_priority(bundle_spec.priority, "priority", indent+1, stream);
	al_bp_print_bundle_delivery_opts(bundle_spec.dopts, "dopts", indent+1, stream);
	al_bp_print_timeval(bundle_spec.expiration, "expiration", indent+1, stream);
	al_bp_print_timestamp(bundle_spec.creation_ts, "creation_ts", indent+1, stream);
	al_bp_print_reg_id(bundle_spec.delivery_regid, "delivery_regid", indent+1, stream);
	fprintf(stream, "%s%s %s:\n", al_bp_print_tabs(indent+1), "struct", "blocks");
	al_bp_print_u32(bundle_spec.blocks.blocks_len, "blocks_len", indent+2, stream);
	al_bp_print_array(bundle_spec.blocks.blocks_val, bundle_spec.blocks.blocks_len, al_bp_print_extension_block, "al_bp_extension_block_t", "blocks_val", indent+2, stream);
	fprintf(stream, "%s%s %s:\n", al_bp_print_tabs(indent+1), "struct", "metadata");
	al_bp_print_u32(bundle_spec.metadata.metadata_len, "metadata_len", indent+2, stream);
	al_bp_print_array(bundle_spec.metadata.metadata_val, bundle_spec.metadata.metadata_len, al_bp_print_extension_block, "al_bp_extension_block_t", "metadata_val", indent+2, stream);
	al_bp_print_boolean(bundle_spec.unreliable, "unreliable", indent+1, stream);
	al_bp_print_boolean(bundle_spec.critical, "critical", indent+1, stream);
	al_bp_print_u32(bundle_spec.flow_label, "flow_label", indent+1, stream);
}

void al_bp_print_bundle_payload_location(al_bp_bundle_payload_location_t bundle_payload_location, const char* name, size_t indent, FILE* stream) {
	fprintf(stream, "%s%s %s: %u\n", al_bp_print_tabs(indent), "al_bp_bundle_payload_location_t", name, (uint32_t) bundle_payload_location);
}

void al_bp_print_status_report_reason(al_bp_status_report_reason_t status_report_reason, const char* name, size_t indent, FILE* stream) {
	fprintf(stream, "%s%s %s: %u\n", al_bp_print_tabs(indent), "al_bp_status_report_reason_t", name, (uint32_t) status_report_reason);
}

void al_bp_print_status_report_flags(al_bp_status_report_flags_t status_report_flags, const char* name, size_t indent, FILE* stream) {
	fprintf(stream, "%s%s %s: %#X\n", al_bp_print_tabs(indent), "al_bp_status_report_flags_t", name, (uint32_t) status_report_flags);
}

void al_bp_print_bundle_id(al_bp_bundle_id_t bundle_id, const char* name, size_t indent, FILE* stream) {
	fprintf(stream, "%s%s %s:\n", al_bp_print_tabs(indent), "al_bp_bundle_id_t", name);
	al_bp_print_endpoint_id(bundle_id.source, "source", indent+1, stream);
	al_bp_print_timestamp(bundle_id.creation_ts, "creation_ts", indent+1, stream);
	al_bp_print_u32(bundle_id.frag_offset, "frag_offset", indent+1, stream);
	al_bp_print_u32(bundle_id.orig_length, "orig_length", indent+1, stream);
}

void al_bp_print_bundle_status_report(al_bp_bundle_status_report_t bundle_status_report, const char* name, size_t indent, FILE* stream) {
	fprintf(stream, "%s%s %s:\n", al_bp_print_tabs(indent), "al_bp_bundle_status_report_t", name);
	al_bp_print_bundle_id(bundle_status_report.bundle_id, "bundle_id", indent+1, stream);
	al_bp_print_status_report_reason(bundle_status_report.reason, "reason", indent+1, stream);
	al_bp_print_status_report_flags(bundle_status_report.flags, "flags", indent+1, stream);
	al_bp_print_timestamp(bundle_status_report.receipt_ts, "receipt_ts", indent+1, stream);
	al_bp_print_timestamp(bundle_status_report.custody_ts, "custody_ts", indent+1, stream);
	al_bp_print_timestamp(bundle_status_report.forwarding_ts, "forwarding_ts", indent+1, stream);
	al_bp_print_timestamp(bundle_status_report.delivery_ts, "delivery_ts", indent+1, stream);
	al_bp_print_timestamp(bundle_status_report.deletion_ts, "deletion_ts", indent+1, stream);
	al_bp_print_timestamp(bundle_status_report.ack_by_app_ts, "ack_by_app_ts", indent+1, stream);
}

void al_bp_print_bundle_payload(al_bp_bundle_payload_t bundle_payload, const char* name, size_t indent, FILE* stream) {
	fprintf(stream, "%s%s %s:\n", al_bp_print_tabs(indent), "al_bp_bundle_payload_t", name);
	al_bp_print_bundle_payload_location(bundle_payload.location, "location", indent+1, stream);
	fprintf(stream, "%s%s %s:\n", al_bp_print_tabs(indent+1), "struct", "filename");
	al_bp_print_u32(bundle_payload.filename.filename_len, "filename_len", indent+2, stream);
	al_bp_print_str(bundle_payload.filename.filename_val, "filename_val", indent+2, stream);
	fprintf(stream, "%s%s %s:\n", al_bp_print_tabs(indent+1), "struct", "buf");
	al_bp_print_u32(bundle_payload.buf.buf_crc, "buf_crc", indent+2, stream);
	al_bp_print_u32(bundle_payload.buf.buf_len, "buf_len", indent+2, stream);
	al_bp_print_str_len(bundle_payload.buf.buf_val, bundle_payload.buf.buf_len, "buf_val", indent+2, stream);
	if (bundle_payload.status_report != NULL)
		al_bp_print_bundle_status_report(*bundle_payload.status_report, "*status_report", indent+1, stream);
	else al_bp_print_null("al_bp_bundle_status_report_t", "status_report", indent+1, stream);
}

void al_bp_print_bundle_object(al_bp_bundle_object_t bundle_object, const char* name, size_t indent, FILE* stream) {
	fprintf(stream, "%s%s %s:\n", al_bp_print_tabs(indent), "al_bp_bundle_object_t", name);
	al_bp_print_bundle_id(*bundle_object.id, "*id", indent+1, stream);
	al_bp_print_bundle_spec(*bundle_object.spec, "*spec", indent+1, stream);
	al_bp_print_bundle_payload(*bundle_object.payload, "*payload", indent+1, stream);
}
