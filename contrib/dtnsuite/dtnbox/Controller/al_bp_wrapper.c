/********************************************************
  ** Authors: Nicolò Castellazzi, nicolo.castellazzi@studio.unibo.it
  **          Marcello Ballanti, marcello.ballanti@studio.unibo.it
  **          Marco Bertolazzi, marco.bertolazzi3@studio.unibo.it
  **          Carlo Caini (DTNbox project supervisor), carlo.caini@unibo.it
  **
  ** Copyright 2018 University of Bologna
  ** Released under GPLv3. See LICENSE.txt for details.
  **
  ********************************************************/

/*
 * al_bp_wrapper.c
 * Modificato da Caini per supportare lo schema ipn e l'implementazione IBR
 * -modifica con Andrea Bisacchi per usare l'al_bp_extB
 * -modificato provvisoriamente path del tar da inviare nel bundle per presunto bug (percorso < 31) in caso di ION.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "al_bp_extB.h"
#include "al_bp_api.h"
#include "al_bp_wrapper.h"
#include "utils.h"
#include <libgen.h>

#include "../Model/definitions.h"
#include "debugger.h"
// 0 -> tutto ok
int dtnbox_openConnection(
		al_bp_extB_registration_descriptor* register_descriptor,
		char* demux_token_ipn, char* demux_token_dtn) {

	al_bp_extB_error_t error_extB;
	error_extB = al_bp_extB_register(register_descriptor, demux_token_dtn, atoi(demux_token_ipn));

	switch (error_extB) {

	case (BP_EXTB_SUCCESS): {
		debug_print(DEBUG_OFF, "dtnbox_openConnection: DTN connection successfully opened\n");
		break;
	}
	case (BP_EXTB_ERRNULLPTR): {
		error_print("dtnbox_openConnection: error in parameters\n");
		break;
	}
	case (BP_EXTB_ERROPEN): {
		error_print(
				"dtnbox_openConnection: error while opening connection\n");
		break;
	}
	case (BP_EXTB_ERRLOCALEID): {
		error_print(
				"dtnbox_openConnection: error while building local EID\n");
		break;
	}
	case (BP_EXTB_ERRREGISTER): {
		error_print("dtnbox_openConnection: error while registering\n");
		break;
	}
	default: {
		error_print(
				"dtnbox_openConnection: unknown error while opening connection\n");
		break;
	}
	}
	return error_extB;
}

int dtnbox_closeConnection(
		al_bp_extB_registration_descriptor register_descriptor) {

	al_bp_extB_error_t extB_error;

	extB_error = al_bp_extB_unregister(register_descriptor);
	if (extB_error != BP_EXTB_SUCCESS) {
		error_print(
				"dtnbox_closeConnection: error in al_bp_extB_unregister() (%d)\n",
				extB_error);
		return ERROR_VALUE;
	}
	return SUCCESS_VALUE;
}

int dtnbox_send(al_bp_extB_registration_descriptor register_descriptor,
		char* tarName, dtnNode dest) {

	al_bp_error_t error;
	al_bp_bundle_object_t bundle;
	al_bp_bundle_payload_location_t location = BP_PAYLOAD_FILE;
	al_bp_bundle_priority_t bundle_priority;
	al_bp_timeval_t bundle_expiration;

	al_bp_endpoint_id_t to;
	al_bp_endpoint_id_t reply_to;
	al_bp_extB_error_t extB_error;

	char cpCommand[SYSTEM_COMMAND_LENGTH + 2*DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH];
	char newPath[DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH];


	getHomeDir(newPath);
	strcat(newPath, basename(tarName));
	strcpy(cpCommand, "cp \"");
	strcat(cpCommand, tarName);
	strcat(cpCommand, "\" \"");
	strcat(cpCommand, newPath);
	strcat(cpCommand, "\"");

	error = system(cpCommand);
	if (error) {
		error_print("dtnbox_send: error in system(%s)\n",
				cpCommand);
		return ERROR_VALUE;
	}

	bundle_priority.priority = BP_PRIORITY_NORMAL;
	bundle_priority.ordinal = 0;
	bundle_expiration = (al_bp_timeval_t) dest.lifetime;

	//creo bundle
	error = al_bp_bundle_create(&bundle);
	if (error != BP_SUCCESS) {
		error_print("dtnbox_send: error in al_bp_bundle_create(): %s\n",
				al_bp_strerror(error));
		return ERROR_VALUE;
	}
	error = al_bp_bundle_set_payload_location(&bundle, location);
	if (error != BP_SUCCESS) {
		error_print(
				"dtnbox_send: error in al_bp_bundle_set_payload_location(): %s\n",
				al_bp_strerror(error));
		al_bp_bundle_free(&bundle);
		return ERROR_VALUE;
	}

	error = al_bp_bundle_set_payload_file(&bundle, newPath, strlen(newPath));
	//error = al_bp_bundle_set_payload_file(&bundle, tarName, strlen(tarName));
	if (error != BP_SUCCESS) {
		error_print(
				"dtnbox_send: error in al_bp_bundle_set_payload_file(): %s\n",
				al_bp_strerror(error));
		al_bp_bundle_free(&bundle);
		return ERROR_VALUE;
	}

	error = al_bp_bundle_set_priority(&bundle, bundle_priority);
	if (error != BP_SUCCESS) {
		error_print(
				"dtnbox_send: error in al_bp_bundle_set_priority(): %s\n",
				al_bp_strerror(error));
		al_bp_bundle_free(&bundle);
		return ERROR_VALUE;
	}
	bundle.spec->priority.ordinal = 0;
	bundle.spec->critical = FALSE;
	bundle.spec->flow_label = 0;
	bundle.spec->unreliable = FALSE;

	error = al_bp_bundle_set_expiration(&bundle, bundle_expiration);
	if (error != BP_SUCCESS) {
		error_print(
				"dtnbox_send: error in al_bp_bundle_set_expiration(): %s\n",
				al_bp_strerror(error));
		al_bp_bundle_free(&bundle);
		return ERROR_VALUE;
	}
	error = al_bp_bundle_set_delivery_opts(&bundle,
			(al_bp_bundle_delivery_opts_t) 0);
	if (error != BP_SUCCESS) {
		error_print(
				"dtnbox_send: error in al_bp_bundle_set_delivery_opts(): %s\n",
				al_bp_strerror(error));
		al_bp_bundle_free(&bundle);
		return ERROR_VALUE;
	}

	al_bp_parse_eid_string(&to, dest.EID);
	al_bp_get_none_endpoint(&reply_to);
	extB_error = al_bp_extB_send(register_descriptor, bundle, to,
			reply_to);
	if (extB_error != BP_EXTB_SUCCESS) {
		error_print("dtnbox_send: error in al_bp_extB_send_bundle(): %s\n",
				al_bp_strerror(error));
		al_bp_bundle_free(&bundle);
		return ERROR_VALUE;
	}

	//mandato il bundle, lo dealloco
	error = al_bp_bundle_free(&bundle);
	if (error != BP_SUCCESS) {
		error_print("dtnbox_send: error in al_bp_bundle_free(): %s\n",
				al_bp_strerror(error));
		al_bp_bundle_free(&bundle);
		return ERROR_VALUE;
	}
	return SUCCESS_VALUE;
}

int dtnbox_receive(al_bp_extB_registration_descriptor register_descriptor,
		char* tarName, char* sourceEID) {
	al_bp_error_t error;
	u32_t fileNameLen;
	al_bp_bundle_object_t bundle;
	al_bp_bundle_payload_location_t location = BP_PAYLOAD_FILE;
	al_bp_endpoint_id_t sourceEIDStruct;
	char* fileName;

	al_bp_extB_error_t extB_error;

	//creo l'oggetto bundle
	error = al_bp_bundle_create(&bundle);
	if (error != BP_SUCCESS) {
		error_print("dtnbox_receive: error in al_bp_bundle_create(): %s\n",
				al_bp_strerror(error));
		return ERROR_VALUE;
	}

	//aspetto di riceverlo
	debug_print(DEBUG_L1,"dtnbox_receive: waiting for a bundle...\n");
	extB_error = al_bp_extB_receive(register_descriptor, &bundle,
			location, -1);
	if (extB_error != BP_EXTB_SUCCESS) {
		switch(extB_error){
		case BP_EXTB_ERRNOTREGISTRED:{
			//ERROR
			error_print("dtnbox_receive: error BP_EXTB_ERRNOTREGISTRED in al_bp_extB_receive_bundle()\n");
			al_bp_bundle_free(&bundle);
			return ERROR_VALUE;
		}
		case BP_EXTB_ERRRECEIVE:{
			//ERROR
			error_print("dtnbox_receive: error BP_EXTB_ERRRECEIVE in al_bp_extB_receive_bundle()\n");
			al_bp_bundle_free(&bundle);
			return ERROR_VALUE;
		}
		case BP_EXTB_ERRTIMEOUT:{
			//WARNING
			error_print("dtnbox_receive: warning BP_EXTB_ERRTIMEOUT in al_bp_extB_receive_bundle()\n");
			al_bp_bundle_free(&bundle);
			return WARNING_VALUE;
		}
		case BP_EXTB_ERRRECEPINTER:{
			//WARNING
			error_print("dtnbox_receive: warning BP_EXTB_ERRRECEPINTER in al_bp_extB_receive_bundle()\n");
			al_bp_bundle_free(&bundle);
			return WARNING_VALUE;
		}
		default:{
			//ERROR
			error_print("dtnbox_receive: error in al_bp_extB_receive_bundle()\n");
			al_bp_bundle_free(&bundle);
			return ERROR_VALUE;
		}
		}//switch(extB_error)
	}//if(error)

	//dal bundle costruisco il tar e ricavo il mittente
	error = al_bp_bundle_get_payload_file(bundle, &fileName, &fileNameLen);
	if (error != BP_SUCCESS) {
		error_print(
				"dtnbox_receive: error in al_bp_bundle_get_payload_file(): %s\n",
				al_bp_strerror(error));
		al_bp_bundle_free(&bundle);
		return ERROR_VALUE;
	}
	strcpy(tarName, fileName);

	//stampa per capire dov'e' il file con il contenuto del bundle
	//debug_print(DEBUG_L1, "dtnbox_receive: received bundle, associated file=%s\n", fileName);

	error = al_bp_bundle_get_source(bundle, &sourceEIDStruct);
	if (error != BP_SUCCESS) {
		error_print(
				"dtnbox_receive: error in al_bp_bundle_get_source(): %s\n",
				al_bp_strerror(error));
		al_bp_bundle_free(&bundle);
		return ERROR_VALUE;
	}
	strcpy(sourceEID, sourceEIDStruct.uri);

	//libero memoria dal nome file. Suppongo sia necessario per come viene usata la funzione al_bp_bundle_get_payload_file().
	free(fileName);

	//distruggo il bundle
	error = al_bp_bundle_free(&bundle);
	if (error != BP_SUCCESS) {
		error_print("dtnbox_receive: error in al_bp_bundle_free(): %s\n",
				al_bp_strerror(error));
		al_bp_bundle_free(&bundle);
		return ERROR_VALUE;
	}
	return SUCCESS_VALUE;
}
