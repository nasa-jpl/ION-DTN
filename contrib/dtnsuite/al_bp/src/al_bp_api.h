/********************************************************
 **  Authors: Michele Rodolfi, michele.rodolfi@studio.unibo.it
 **           Anna d'Amico, anna.damico@studio.unibo.it
 **           Carlo Caini (DTNperf_3 project supervisor), carlo.caini@unibo.it
 **
 **
 **  Copyright (c) 2013, Alma Mater Studiorum, University of Bologna
 **  All rights reserved.
 This file contains all the API definitions of the al_bp (al_bp prefix).
 ** These are called directly from the application.
 ** In DTNperf_3 These are the sole al_bp_ APIs called. 
 ** Each API consists of a switch between DTN2 and ION API implementations (bp prefix)
 ********************************************************/

/*
 * bp_abstraction_api.h
 *
 */

#ifndef BP_ABSTRACTION_API_H_
#define BP_ABSTRACTION_API_H_

#include "al_bp_types.h"

/**
 * Get abstraction layer library version.
 */
const char * get_al_bp_version();

/**
 * Find the underlying implementation of bundle protocol
 */
al_bp_implementation_t al_bp_get_implementation();

/**
 * Open a new connection to the router.
 *
 * On success, initializes the handle parameter as a new handle to the
 * daemon and returns BP_SUCCESS. On failure, sets handle to NULL and
 * returns a bp_errno error code.
 *
 * For ION this API attach the application to the bundle protocol
 */
al_bp_error_t al_bp_open(al_bp_handle_t* handle);

/**
 * Open a new connection to the router witha given IP addr and port
 *
 * On success, initializes the handle parameter as a new handle to the
 * daemon and returns BP_SUCCESS. On failure, sets handle to NULL and
 * returns a bp_errno error code.
 */
al_bp_error_t al_bp_open_with_ip(char *daemon_api_IP,int daemon_api_port,al_bp_handle_t* handle);


/**
 * Get the error associated with the given handle.
 */
al_bp_error_t al_bp_errno(al_bp_handle_t handle);

/**
 * Build an appropriate local endpoint id by appending the specified
 * service tag to the daemon's preferred administrative endpoint id.
 *
 * @type scheme to use to create a local eid (DTN_SCHEME or CBHE_SCHEME)
 * @service_tag service tag of the local eid. If type is CBHE_SCHEME on a DTN2 implementation
 * 		service_tag must be in the form ipn_local_number.service_number because DTN2 (2.9 with patch) doesn't know his ipn number
 */
al_bp_error_t al_bp_build_local_eid(al_bp_handle_t handle,
									al_bp_endpoint_id_t* local_eid,
									const char* service_tag,
									al_bp_scheme_t type);


/**
 * Create a bp registration. If the init_passive flag in the reginfo
 * struct is true, the registration is initially in passive state,
 * requiring a call to dtn_bind to activate it. Otherwise, the call to
 * dtn_bind is unnecessary as the registration will be created in the
 * active state and bound to the handle.
 */
al_bp_error_t al_bp_register(al_bp_handle_t * handle,
						al_bp_reg_info_t* reginfo,
						al_bp_reg_id_t* newregid);

/**
 * Check for an existing registration on the given endpoint id,
 * returning BP_SUCCSS and filling in the registration id if it
 * exists, or returning ENOENT if it doesn't.
 */
al_bp_error_t al_bp_find_registration(al_bp_handle_t handle,
							al_bp_endpoint_id_t * eid,
							al_bp_reg_id_t * newregid);
/**
 * Remove a registration
 */
al_bp_error_t al_bp_unregister(al_bp_handle_t handle, al_bp_reg_id_t regid,al_bp_endpoint_id_t eid);

/**
 * Send a bundle either from memory or from a file.
 */
al_bp_error_t al_bp_send(al_bp_handle_t handle,
					al_bp_reg_id_t regid,
					al_bp_bundle_spec_t* spec,
					al_bp_bundle_payload_t* payload,
					al_bp_bundle_id_t* id);


/**
 * Blocking receive for a bundle, filling in the spec and payload
 * structures with the bundle data. The location parameter indicates
 * the manner by which the caller wants to receive payload data (i.e.
 * either in memory or in a file). The timeout parameter specifies an
 * interval in milliseconds to block on the server-side (-1 means
 * infinite wait).
 *
 * Note that it is advisable to call bp_free_payload on the returned
 * structure, otherwise the XDR routines will memory leak.
 */
al_bp_error_t al_bp_recv(al_bp_handle_t handle,
					al_bp_bundle_spec_t* spec,
					al_bp_bundle_payload_location_t location,
					al_bp_bundle_payload_t* payload,
					al_bp_timeval_t timeout);

/**
 * Close an open bp handle.  Returns BP_SUCCESS on success.
 */
al_bp_error_t al_bp_close(al_bp_handle_t handle);



/*************************************************************
 *
 *                     Utility Functions
 *
 *************************************************************/

/**
 * Copy the contents of one eid into another.
 */
void al_bp_copy_eid(al_bp_endpoint_id_t* dst, al_bp_endpoint_id_t* src);

/**
 * Parse a string into an endpoint id structure, validating that it is
 * in fact a valid endpoint id (i.e. a URI).
 */
al_bp_error_t al_bp_parse_eid_string(al_bp_endpoint_id_t* eid, const char* str);

/**
 * Returns the pointer to the well-formatted string of dtn:none
 */
const char* al_bp_get_none_endpoint_string();

/**
 * Returns the null endpoint
 */
al_bp_error_t al_bp_get_none_endpoint(al_bp_endpoint_id_t * eid_none);

/**
 * Sets the value of the given payload structure to either a memory
 * buffer or a file location.
 *
 * Returns: 0 on success, bp_ESIZE if the memory location is
 * selected and the payload is too big.
 */
al_bp_error_t al_bp_set_payload(al_bp_bundle_payload_t* payload,
							al_bp_bundle_payload_location_t location,
                           char* val, int len);

/**
 * Frees dynamic storage allocated by the xdr for a bundle extension blocks in
 * bp_recv.
 */
void al_bp_free_extension_blocks(al_bp_bundle_spec_t* spec);

/**
 * Frees dynamic storage allocated by the xdr for a bundle metadata blocks in
 * bp_recv.
 */
void al_bp_free_metadata_blocks(al_bp_bundle_spec_t* spec);

/**
 * Frees dynamic storage allocated by the xdr for a bundle payload in
 * bp_recv.
 */
void al_bp_free_payload(al_bp_bundle_payload_t* payload);

/**
 * Returns a string version of a status report reason code.
 */
const char* al_bp_status_report_reason_to_str(al_bp_status_report_reason_t err);

/**
 * Returns a string version of a status report type.
 */
char* al_bp_status_report_flag_to_str(al_bp_status_report_flags_t flag);
/**
 * Get a string value associated with the bp error code.
 */
char * al_bp_strerror(int err);

/********************************************************************
 *
 *             HIGHER LEVEL FUNCTIONS
 *
 ********************************************************************/

/**
 * Send bundle
 * Bundle source, destination and reply to eid must be valid bp endpoints
 */
al_bp_error_t al_bp_bundle_send(al_bp_handle_t handle,
							al_bp_reg_id_t regid,
							al_bp_bundle_object_t bundle_object);

/**
 * Receive bundle
 */
al_bp_error_t al_bp_bundle_receive(al_bp_handle_t handle,
							al_bp_bundle_object_t * bundle_object,
							al_bp_bundle_payload_location_t payload_location,
							al_bp_timeval_t timeout);

/**
 * Initialize a bundle object
 */
al_bp_error_t al_bp_bundle_create(al_bp_bundle_object_t * bundle_object);

/**
 * Deallocate memory allocated with bp_bundle_create()
 */
al_bp_error_t al_bp_bundle_free(al_bp_bundle_object_t * bundle_object);

/**
 * Get bundle id
 */
al_bp_error_t al_bp_bundle_get_id(al_bp_bundle_object_t bundle_object, al_bp_bundle_id_t ** bundle_id);

/**
 * Get Bundle payload location
 */

al_bp_error_t al_bp_bundle_set_payload_location(al_bp_bundle_object_t * bundle_object,
											al_bp_bundle_payload_location_t location);
/**
 * Get Bundle payload location
 */

al_bp_error_t al_bp_bundle_get_payload_location(al_bp_bundle_object_t bundle_object,
										al_bp_bundle_payload_location_t * location);

/**
 * Get payload size
 * if payload location is BP_PAYLOAD_FILE gets the size of the file
 */
al_bp_error_t al_bp_bundle_get_payload_size(al_bp_bundle_object_t bundle_object, u32_t * size);

/**
 * Get filename of payload
 */
al_bp_error_t al_bp_bundle_get_payload_file(al_bp_bundle_object_t bundle_object,
										char_t ** filename, u32_t * filename_len);

/**
 * Get pointer of payload buffer
 */
al_bp_error_t al_bp_bundle_get_payload_mem(al_bp_bundle_object_t bundle_object,
										char ** buf, u32_t * buf_len);

/**
 * Set payload from a file
 */
al_bp_error_t al_bp_bundle_set_payload_file(al_bp_bundle_object_t * bundle_object,
										char_t * filename, u32_t filename_len);

/**
 * Set payload from memory
 */
al_bp_error_t al_bp_bundle_set_payload_mem(al_bp_bundle_object_t * bundle_object,
										char * buf, u32_t buf_len);

/**
 * Get bundle source eid
 */
al_bp_error_t al_bp_bundle_get_source(al_bp_bundle_object_t bundle_object, al_bp_endpoint_id_t * source);

/**
 * Set Bundle Source eid
 */
al_bp_error_t al_bp_bundle_set_source(al_bp_bundle_object_t * bundle_object, al_bp_endpoint_id_t source);

/**
 * Get bundle destination eid
 */
al_bp_error_t al_bp_bundle_get_dest(al_bp_bundle_object_t bundle_object, al_bp_endpoint_id_t * dest);

/**
 * Set bundle destination eid
 */
al_bp_error_t al_bp_bundle_set_dest(al_bp_bundle_object_t * bundle_object, al_bp_endpoint_id_t dest);

/**
 * Get bundle reply-to eid
 */
al_bp_error_t al_bp_bundle_get_replyto(al_bp_bundle_object_t bundle_object, al_bp_endpoint_id_t * replyto);

/**
 * Set bundle reply-to eid
 */
al_bp_error_t al_bp_bundle_set_replyto(al_bp_bundle_object_t * bundle_object, al_bp_endpoint_id_t replyto);

/**
 * Get bundle priority
 */
al_bp_error_t al_bp_bundle_get_priority(al_bp_bundle_object_t bundle_object, al_bp_bundle_priority_t * priority);

/**
 * Set bundle priority
 */
al_bp_error_t al_bp_bundle_set_priority(al_bp_bundle_object_t * bundle_object, al_bp_bundle_priority_t priority);

/**
 * Set bundle unreliable
 */
al_bp_error_t al_bp_bundle_set_unreliable(al_bp_bundle_object_t * bundle_object, boolean_t unreliable);

/**
 * Get bundle unreliable
 */
al_bp_error_t al_bp_bundle_get_unreliable(al_bp_bundle_object_t bundle_object, boolean_t * unreliable);

/**
 * Set bundle critical
 */
al_bp_error_t al_bp_bundle_set_critical(al_bp_bundle_object_t * bundle_object, boolean_t critical);

/**
 * Get bundle critical
 */
al_bp_error_t al_bp_bundle_get_critical(al_bp_bundle_object_t bundle_object, boolean_t * critical);

/**
 * Set bundle flow label
 */
al_bp_error_t al_bp_bundle_set_flow_label(al_bp_bundle_object_t * bundle_object, u32_t flow_label);

/**
 * Get bundle flow label
 */
al_bp_error_t al_bp_bundle_get_flow_label(al_bp_bundle_object_t bundle_object, u32_t * flow_label);

/**
 * Get bundle expiration time
 */
al_bp_error_t al_bp_bundle_get_expiration(al_bp_bundle_object_t bundle_object, al_bp_timeval_t * exp);
/**
 * Set bundle expiration time
 */
al_bp_error_t al_bp_bundle_set_expiration(al_bp_bundle_object_t * bundle_object, al_bp_timeval_t exp);

/**
 * Get bundle creation timestamp
 */
al_bp_error_t al_bp_bundle_get_creation_timestamp(al_bp_bundle_object_t bundle_object, al_bp_timestamp_t * ts);
/**
 * Set bundle creation timestamp
 */
al_bp_error_t al_bp_bundle_set_creation_timestamp(al_bp_bundle_object_t * bundle_object, al_bp_timestamp_t ts);

/**
 * Get bundle delivery options
 */
al_bp_error_t al_bp_bundle_get_delivery_opts(al_bp_bundle_object_t bundle_object,
											al_bp_bundle_delivery_opts_t * dopts);
/**
 * Set bundle delivery options
 */
al_bp_error_t al_bp_bundle_set_delivery_opts(al_bp_bundle_object_t * bundle_object,
											al_bp_bundle_delivery_opts_t dopts);

/**
 * Get status report.
 * If bundle_object is a status report, status_report is filled with a pointer to the status report structure
 * Otherwise status_report is set to NULL.
 * Returns BP_SUCCESS.
 * Returns BP_EINTERNAL if the bundle is malformed
 */
al_bp_error_t al_bp_bundle_get_status_report(al_bp_bundle_object_t bundle_object,
											al_bp_bundle_status_report_t ** status_report);

#endif /* BP_ABSTRACTION_API_H_ */
