/*
HSLTP_def.h

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
 Co-author of HSLTP extensions: Azzurra Ciliberti (azzurra.ciliberti@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

This file contains the structures of the eclso daemon

 * */
#ifndef _HSLTP_DEF_H_

#define _HSLTP_DEF_H_


#ifdef __cplusplus
extern "C" {
#endif


#define SPECIAL_PROC_TIMER 100 //milliseconds

/* HSLTP processing type*/
typedef enum
	{
	ERROR_PROC				= -1 ,
	DEFAULT_PROC			= 0  ,
	END_OF_MATRIX			= 1  ,
	SPECIAL_PROC			= 2  ,
	} HSLTPProcessingType;

/* HSLTP matrix type*/
typedef enum
	{
	ONLY_DATA 					= 0  ,
	ONLY_SIGNALING				= 1  ,
	SIGNALING_AND_DATA 			= 2  ,
	} HSLTPMatrixType;


#ifdef __cplusplus
}
#endif

#endif
