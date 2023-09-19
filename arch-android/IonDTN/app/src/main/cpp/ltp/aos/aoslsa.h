/*
 	aoslsa.h:	common definitions for AOS link service
			adapter modules.

*/

/* 7/6/2010, copied from udplso, as per issue 101-LTP-over-AOS-via-UDP
   Greg Menke, Raytheon, under contract METS-MR-679-0909 with NASA GSFC */


#ifndef _AOSLSA_H_
#define _AOSLSA_H_

#include "ltpP.h"
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AOSLSA_BUFSZ		((256 * 256) - 1)
#define LtpAosDefaultPortNbr	1200

#ifdef __cplusplus
}
#endif

#endif	/* _AOSLSA_H */
