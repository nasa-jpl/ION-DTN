/*
 *	tc.h:	definitions common to both client and authority
 *		in Trusted Collective.
 *
 *	Copyright (c) 2020, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#ifndef TC_H
#define TC_H

#include "lyst.h"
#include "zco.h"
#include "bpP.h"

#ifndef TC_DEBUG
#define TC_DEBUG	(0)
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TC_MAX_DATLEN
#define	TC_MAX_DATLEN	1024
#endif

#define	TC_MAX_REC	(22 + TC_MAX_DATLEN)

extern int	tc_serialize(char *buffer, unsigned int buflen,
			uvast fqnn, time_t effectiveTime,
			time_t assertionTime, unsigned short datLength,
			unsigned char *datValue);

extern int	tc_deserialize(char **buffer, int *buflen,
			unsigned short maxDatLength, uvast *fqnn,
			time_t *effectiveTime, time_t *assertionTime,
			unsigned short *datLength, unsigned char *datValue);

#ifdef __cplusplus
}
#endif

#endif /* TC_H */
