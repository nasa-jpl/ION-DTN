/********************************************************
  ** Authors: Nicolò Castellazzi, nicolo.castellazzi@studio.unibo.it
  **          Marcello Ballanti, marcello.ballanti@studio.unibo.it
  **          Carlo Caini (DTNbox project supervisor), carlo.caini@unibo.it
  **
  ** Copyright 2018 University of Bologna
  ** Released under GPLv3. See LICENSE.txt for details.
  **
  ********************************************************/

/*
 * message.h
 */

#ifndef MESSAGE_H_
#define MESSAGE_H_

#include "definitions.h"

//messaggio inviato o ricevuto.
typedef struct message{
	unsigned long long nextTxTimestamp;
	int txLeft;
	dtnNode source;
	dtnNode destination;
} message;


#endif /* MESSAGE_H_ */
