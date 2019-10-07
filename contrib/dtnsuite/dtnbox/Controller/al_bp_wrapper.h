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
 * al_bp_wrapper.h
 */

#ifndef AL_BP_WRAPPER_H_
#define AL_BP_WRAPPER_H_

#include "al_bp_types.h"
#include "../../al_bp/src/al_bp_extB.h"
#include "al_bp_api.h"
#include "../Model/definitions.h"

//creazione connessione
int dtnbox_openConnection(al_bp_extB_registration_descriptor* register_descriptor, char* demux_token_ipn, char* demux_token_dtn);
//invio messaggio
int dtnbox_send(al_bp_extB_registration_descriptor register_descriptor, char* tarName, dtnNode dest);
//ricezione messaggio. tempo 0 = aspetta all'infinito.
int dtnbox_receive(al_bp_extB_registration_descriptor register_descriptor, char* tarName, char* sourceEID);
//chiusura connessione
int dtnbox_closeConnection(al_bp_extB_registration_descriptor register_descriptor);

#endif /* AL_BP_WRAPPER_H_ */
