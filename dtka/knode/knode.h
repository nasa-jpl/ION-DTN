/*
 *	knode.h:	private definitions supporting the use of
 *			DTKA at ION nodes.
 *
 *	Copyright (c) 2013, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#include "tcc.h"
#include "dtka.h"

#ifndef _KNODE_H_
#define _KNODE_H_

#ifdef __cplusplus
extern "C" {
#endif

#define	DTKA_DELCARE	201

/*	*	*	Database structure	*	*	*	*/

typedef struct
{
	time_t		nextKeyGenTime;
	unsigned int	keyGenInterval;		/*	At least 60.	*/
	unsigned int	effectiveLeadTime;	/*	At least 20.	*/
} DtkaNodeDB;

extern int		knodeInit();
extern int		knodeAttach();
extern Object		getKnodeDbObject();
extern DtkaNodeDB	*getKnodeConstants();

#ifdef __cplusplus
}
#endif

#endif	/*	_KNODE_H	*/
