/*
 *	dtka.h:	definitions supporting the use of DTKA at ION nodes.
 *
 *	Copyright (c) 2020, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#include "tcc.h"

#ifndef _DTKA_H_
#define _DTKA_H_

#ifdef __cplusplus
extern "C" {
#endif

#define	DTKA_DECLARE	201
#define	DTKA_ANNOUNCE	203

/*	*	*	Database structure	*	*	*	*/

typedef struct
{
	time_t		nextKeyGenTime;
	unsigned int	keyGenInterval;		/*	At least 60.	*/
	unsigned int	effectiveLeadTime;	/*	At least 20.	*/
} DtkaDB;

extern int	dtkaInit();
extern int	dtkaAttach();
extern Object	getDtkaDbObject();
extern DtkaDB	*getDtkaConstants();

#ifdef __cplusplus
}
#endif

#endif	/*	_DTKA_H	*/
