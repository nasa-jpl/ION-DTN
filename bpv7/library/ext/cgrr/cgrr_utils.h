/** \file cgrr_utils.h
 *
 *	\brief       This file provides all structures, variables, and function
 **              definitions necessary for a full implementation of CGR Route
 **              Extension Block.
 *
 ** \copyright Copyright (c) 2020, Alma Mater Studiorum, University of Bologna, All rights reserved.
 **
 ** \par License
 **
 **    This file is part of Unibo-CGR.                                            <br>
 **                                                                               <br>
 **    Unibo-CGR is free software: you can redistribute it and/or modify
 **    it under the terms of the GNU General Public License as published by
 **    the Free Software Foundation, either version 3 of the License, or
 **    (at your option) any later version.                                        <br>
 **    Unibo-CGR is distributed in the hope that it will be useful,
 **    but WITHOUT ANY WARRANTY; without even the implied warranty of
 **    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **    GNU General Public License for more details.                               <br>
 **                                                                               <br>
 **    You should have received a copy of the GNU General Public License
 **    along with Unibo-CGR.  If not, see <http://www.gnu.org/licenses/>.
 *
 *	\author Laura Mazzuca, laura.mazzuca@studio.unibo.it
 *
 *	\par Supervisor
 *	     Carlo Caini, carlo.caini@unibo.it
 */

#ifndef _CGRR_UTIL_H_
#define _CGRR_UTIL_H_

#include "bpP.h"
#include "bei.h"
#include <stdlib.h>

#ifndef CGRR_DEBUG
/**
 * \brief Enable (1) or disable (0) CGRR debug print.
 *
 * \hideinitializer
 */
#define CGRR_DEBUG 1
#endif

#if (CGRR_DEBUG == 1)
#define cgrr_debugPrint(f_, ...) debugPrint((f_), ##__VA_ARGS__)
#else
#define cgrr_debugPrint(f_, ...) do {  } while(0)
#endif

typedef struct
{
	uvast	fromNode;
	uvast	toNode;
	time_t	fromTime;
} CGRRHop;

typedef struct
{
	unsigned int  hopCount; //Number of hops (contacts)
	CGRRHop		 *hopList; //Hop (contact): identified by [from, to, fromTime]
} CGRRoute;


typedef struct
{
	unsigned int recRoutesLength; //number of recomputedRoutes
	CGRRoute originalRoute; //computed by the source
	CGRRoute *recomputedRoutes; //computed by following nodes
} CGRRouteBlock;

extern void printCGRRoute(CGRRoute *cgrRoute);
extern void printCGRRouteBlock(CGRRouteBlock *cgrrBlk);
extern unsigned char *cgrr_serializeCGRR(uvast *length, CGRRouteBlock *cgrrBlk);
extern int	cgrr_deserializeCGRR(AcqExtBlock *blk, AcqWorkArea *wk);
extern int	cgrr_getCGRRFromExtensionBlock(ExtensionBlock *blk, CGRRouteBlock *cgrrBlk);
extern int saveRouteToExtBlock(int hopCount, CGRRHop* hopList, Bundle* bundle);
extern int addRoute( Bundle *bundle, Object extBlockElt, CGRRoute *routeToAdd);
extern unsigned char *cgrr_addSdnvToStream(unsigned char *stream, Sdnv* value);
extern int processModifiedExtensionBlock(Bundle *bundle, Object blkAddr, ExtensionBlock *blk, unsigned int oldLength, unsigned int oldSize);
extern void copyCGRRouteBlock(CGRRouteBlock *dest, CGRRouteBlock *src);
extern void copyCGRRoute(CGRRoute *dest, CGRRoute *src);
/*extern unsigned int writeCGRRouteBlockToSdr(Sdr sdr, Object destAddress, CGRRouteBlock *src);
unsigned int writeCGRRouteToSdr(Sdr sdr, Object destAddress, CGRRoute *src);*/
extern void releaseCgrrBlkMemory(CGRRouteBlock *cgrrBlk); //Added by G.M. De Cola

extern unsigned int getBufferSizeForEncodedRoutes(unsigned int routesCount, unsigned int hopCount); // Added by L. Persampieri

extern int cgrr_getUsedEvc(Bundle *bundle, ExtensionBlock *cgrrExtBlk, uvast *size);
extern int cgrr_setUsedEvc(Bundle *bundle, ExtensionBlock *cgrrExtBlk, uvast evc);

#endif
