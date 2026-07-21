/*
 *	dtka.h:	definitions supporting the use of DTKA at ION nodes.
 *
 *	Copyright (c) 2020, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#ifndef DTKA_H
#define DTKA_H

#include "tcc.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * DTKA is a TC (Trusted Collective) application.  Every TC application is
 * identified by the multicast group number at which the application's TC client
 * daemon listens for "blocks" published by the TC aggregate authority.
 * The DTKA application is identified by the block multicast group number 203.
 *
 * Additionally, every TC application is configured to send new information to
 * the TC aggregate authority in bundles whose destination cites the multicast
 * group number at which the authority's reception daemon listens for "records"
 * published by the application.  The records multicast group number for DTKA is
 * 201.
 */

#define DTKA_DECLARE 201
#define DTKA_ANNOUNCE 203

	/*	*	*	Database structure	*	*	*	*/

	typedef struct
	{
		time_t nextKeyGenTime;
		unsigned int keyGenInterval;	/*	At least 60.	*/
		unsigned int effectiveLeadTime; /*	At least 20.	*/
		char keyType[6];		/* HMAC, ECDSA, AES are supported */
		unsigned int keySize;		/* In bytes*/
	} DtkaDB;

	extern int dtkaInit(void);
	extern int dtkaAttach(void);
	extern SdrObject getDtkaDbObject(void);
	extern DtkaDB *getDtkaConstants(void);

#ifdef __cplusplus
}
#endif

#endif /* DTKA_H */
