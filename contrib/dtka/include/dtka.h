/*
 *	dtka.h:	common definitions supporting DTKA implementation.
 *
 *	Copyright (c) 2013, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#include "lyst.h"
#include "zco.h"
#include "ionsec.h"
#include "fec.h"
#include "crypto.h"

#ifndef DTKA_DEBUG
#define	DTKA_DEBUG	(0)
#endif

#ifndef _DTKA_H_
#define _DTKA_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DTKA_MAX_DATLEN
#define	DTKA_MAX_DATLEN	1024
#endif
#define	DTKA_MAX_REC	(22 + DTKA_MAX_DATLEN)

#define	DTKA_DECLARE	124
#define	DTKA_CONFER	125
#define	DTKA_ANNOUNCE	126

/*	DTKA erasure coding parameters.  Notes:
 *		20% redundancy in coding.
 *		50% overlap in code block publication:
 *			For each share of each encoded bulletin,
 *			one authority node is assigned prime
 *			responsibility for publishing that share
 *			and some other node is 	assigned backup
 *			responsibility for publishing the same share.
 *		N is total number of blocks transmitted per bulletin.
 *		Q is number of blocks per bulletin per authority.
 *									*/
#define	DTKA_FEC_K	(50)
#define	DTKA_FEC_M	((DTKA_FEC_K /5) * 6)
#define	DTKA_FEC_N	(DTKA_FEC_M * 2)
#ifndef DTKA_NUM_AUTHS
#define	DTKA_NUM_AUTHS	(6)
#endif
#define	DTKA_FEC_Q	(DTKA_FEC_N / DTKA_NUM_AUTHS)
#define DTKA_PRIMARY	(0)
#define DTKA_BACKUP	(1)

extern int	dtka_serialize(unsigned char *buffer, unsigned int buflen,
			uvast nodeNbr, time_t effectiveTime,
			time_t assertionTime, unsigned short datLength,
			unsigned char *datValue);

extern int	dtka_deserialize(unsigned char **buffer, int *buflen,
			unsigned short maxDatLength, uvast *nodeNbr,
			time_t *effectiveTime, time_t *assertionTime,
			unsigned short *datLength, unsigned char *datValue);

#ifdef __cplusplus
}
#endif

#endif
