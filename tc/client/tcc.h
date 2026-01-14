/*
 *	tcc.h:		public definitions supporting the
 *			implementation of Trusted Collective clients.
 *
 *	Copyright (c) 2020, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#ifndef TCC_H
#define TCC_H

#include "tc.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int	tcc_getBulletin(int blocksGroupNbr, char **bulletinContent,
			int *length);
			/*	Places in *bulletinContent a pointer
			 *	to a private working memory buffer
			 *	containing the content of the oldest
			 *	previously unhandled TC bulletin.
			 *	Returns 0 on success, -1 on any system
			 *	failure.  Buffer length 0 indicates
			 *	that the function was interrupted.
			 *	Calling function MUST MRELEASE the
			 *	bulletinContent when processing is
			 *	complete.				*/

#ifdef __cplusplus
}
#endif

#endif /* TCC_H */
