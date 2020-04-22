/********************************************************
 **  Authors: Andrea Bisacchi, andrea.bisacchi5@studio.unibo.it
 **           Carlo Caini (DTNperf_3 project supervisor), carlo.caini@unibo.it
 **
 **
 **  Copyright (c) 2013, Alma Mater Studiorum, University of Bologna
 **  All rights reserved.
 ********************************************************/

/*
 * al_bp_extB.h
 */

#ifndef DTNPERF_SRC_AL_BP_EXTB_H_
#define DTNPERF_SRC_AL_BP_EXTB_H_

#include "al_bp_api.h"

// This is an identifier of the registration, one for each registration.
typedef unsigned int al_bp_extB_registration_descriptor;

typedef enum al_bp_extB_error_t
{
	BP_EXTB_SUCCESS=0,			// Success
	BP_EXTB_ERRNULLPTR,			// Null pointer
	BP_EXTB_ERRINIT,			// Error on initalization
	BP_EXTB_ERROPEN,			// Error on opening
	BP_EXTB_ERRLOCALEID,		// Error on building local eid
	BP_EXTB_ERRREGISTER,		// Error on registering
	BP_EXTB_ERRCLOSE,			// Error on closing
	BP_EXTB_ERRUNREGISTER,		// Error on unregister
	BP_EXTB_ERRNOTREGISTRED,	// Error, not registred
	BP_EXTB_ERRSEND,			// Error on sending
	BP_EXTB_ERRRECEIVE,			// Error on receiving
	BP_EXTB_ERRRECEPINTER,		// Error, reception interrupted
	BP_EXTB_ERRTIMEOUT,			// Error, timed out
	BP_EXTB_ERRRECEIVER,		// Error, the receiver is not indicated or is dtn:none
	BP_EXTB_ERRIPNPARAMETERS,	// Error, the parameters in al_bp_extB_register are not correct. You are using IPN scheme and you passed wrong parameters for that scheme.
	BP_EXTB_ERRDTN2PARAMETERS,	// Error, the parameters in al_bp_extB_register are not correct. You are using DTN2 with IPN scheme and you passed wrong parameters.
	BP_EXTB_ERRNOIMPLEMENTATION // Error, no BP implementation found
} al_bp_extB_error_t;

// This function registers a new connection of the application to the BP.
// Output:
// 		registration_descriptor: the registration descriptor (&gt;0; 0 in case of error)
// Input:
// 		dtn_demux string: dtn demux token (string)
// 		ipn_demux_number: demux token (number )to be used in case of an ipn scheme registration (&gt;0)
// It returns, in addition to the possible error, the registration_descriptor (an integer somewhat inspired to file descriptor in sockets, but not passed by
// the OS) to be given in input to “al_bp_extB” functions that work on a specific registration.
al_bp_extB_error_t al_bp_extB_register(al_bp_extB_registration_descriptor* registration_descriptor, char* dtn_demux_string, int ipn_demux_number);

// It unregisters the registration identified by the registration descriptor.
al_bp_extB_error_t al_bp_extB_unregister(al_bp_extB_registration_descriptor registration_descriptor);

// This function uses the registration_descriptor to identify the 
// registration; then it sends a bundle object. It is a wrapper of the 
// al_bp_bundle_send, but the destination and the "reply to" EIDs are 
// passed in input. For the sake of backward compatibility the 
// destination and "reply to" originally contained in the bundle object 
// are saved and restored at the end of the function.
al_bp_extB_error_t al_bp_extB_send(al_bp_extB_registration_descriptor registration_descriptor, al_bp_bundle_object_t bundle, al_bp_endpoint_id_t destination, al_bp_endpoint_id_t reply_to);

// It receives a bundle object destined to the registration_descriptor 
// passed in input. It is a wrapper of the al_bp_bundle_receive. 
// Payload_location is a structure passed to al_bp lower layers. Timeval 
// is a timeout value passed to al_bp_lower layers.
al_bp_extB_error_t al_bp_extB_receive(al_bp_extB_registration_descriptor registration_descriptor, al_bp_bundle_object_t* bundle, al_bp_bundle_payload_location_t payload_location, al_bp_timeval_t timeval);

// It initializes the Abstraction Layer extension "B". The "force_eid" 
// (N|D|I) is used to specify if the default format of the registration 
// must be overridden ("N" no, i.e. use the default, 'D' for "dtn", 'I' 
// for "ipn"); it also discovers the active BP implementation and saves 
// this information in a local variable available to other extB 
// functions. If it does not find any active BP implementation, it 
// returns a specific error.
// ipn_node_forDTN2: ipn node number used only in case of registration following the ipn scheme on a DTN2 machine
al_bp_extB_error_t al_bp_extB_init(char force_eid_scheme, int ipn_node_forDTN2);

// It destroys the registration list. After calling this function, the 
// programmer can no more use any al_bp_extB functions except the al_bp_extB_init.
void al_bp_extB_destroy();

// It is a wrapper of al_bp_find_registration. It gives in output the 
// registration descriptor corresponding to the EID passed in input.
al_bp_error_t al_bp_extB_find_registration(al_bp_extB_registration_descriptor registration_descriptor, al_bp_endpoint_id_t* eid);

char* al_bp_extB_str_type_error(al_bp_extB_error_t error);



/***************************************************
 *        BACKWARD COMPATIBILITY FUNCTIONS         *
 ***************************************************/



// It returns the "handle" structure associated to the registration 
// descriptor given in input.
al_bp_handle_t al_bp_extB_get_handle(al_bp_extB_registration_descriptor registration_descriptor);

// It returns the "reg_info" structure associated to the registration 
// descriptor given in input.
al_bp_reg_info_t al_bp_extB_get_reginfo(al_bp_extB_registration_descriptor registration_descriptor);

// It returns the "reg_id" structure associated to the registration 
// descriptor given in input.
al_bp_reg_id_t al_bp_extB_get_regid(al_bp_extB_registration_descriptor registration_descriptor);

// It returns the EID format used in all registrations, found and saved 
// by the al_bp_extB_init ("I" for "ipn" and "D" for "dtn").
char al_bp_extB_get_eid_format();

// It returns the local EID structure associated to the registration 
// descriptor given in input.
al_bp_endpoint_id_t al_bp_extB_get_local_eid(al_bp_extB_registration_descriptor registration_descriptor);

// It sets the local EID for the corresponding registration descriptor 
// (both in input).
void al_bp_extB_set_local_eid(al_bp_extB_registration_descriptor registration_descriptor, al_bp_endpoint_id_t local_eid);

// It returns (DTN2, IBR-DTN) or prints (ION) the last "errno" given by 
// the BP implementation, corresponding to the registration descriptor 
// passed in input.
al_bp_error_t al_bp_extB_errno(al_bp_extB_registration_descriptor registration_descriptor);

// It returns the last error (in al_bp format) that occurred in the 
// previous al_bp (not al_bp_extB) function corresponding to the 
// registration descriptor passed in input.
al_bp_error_t al_bp_extB_get_error(al_bp_extB_registration_descriptor registration_descriptor);

// It returns the last error (as a string) that occurred in the previous 
// al_bp (not al_bp_extB) function corresponding to the registration 
// descriptor passed in input.
char* al_bp_extB_strerror(al_bp_extB_registration_descriptor registration_descriptor);

#endif /* DTNPERF_SRC_AL_BP_EXTB_H_ */
