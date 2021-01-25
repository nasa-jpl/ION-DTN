/** \file rgr_utils.h
 *
 *	\brief       This file provides all structures, variables, and function
 *               definitions necessary for a full implementation of Real Route
 *               Extension Block.
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

#ifndef _RGR_UTILS_H_
#define _RGR_UTILS_H_

#include "bpP.h"
#include "bei.h"
#include "platform.h"
#include "cbor.h"
#include <stdlib.h>

#ifndef RGR_DEBUG
/**
 * \brief Enable (1) or disable (0) RGR debug print.
 *
 * \hideinitializer
 */
#define RGR_DEBUG 1
#endif

#if (RGR_DEBUG == 1)
#define rgr_debugPrint(f_, ...) debugPrint((f_), ##__VA_ARGS__)
#else
#define rgr_debugPrint(f_, ...) do {  } while(0)
#endif

typedef struct
{
	unsigned int	length;
	char			*nodes;
} GeoRoute;

extern void printRoute(GeoRoute *route);
extern void copyRoute(GeoRoute *dest, GeoRoute *src);
extern char* parseAdminEID(Sdr sdr, Bundle *bundle);
int parseAndSaveRoute(GeoRoute *route);
//Added by A.Stramiglio 21/11/19
extern int rgr_read(ExtensionBlock *blk, GeoRoute *rgrBlk);

//added by A.Stramiglio 29/02/20
extern uvast findLoopEntryNode(GeoRoute *route, uvast nodeNum);

//Added by L. Persampieri 26/03/20
extern int get_geo_route_lyst(GeoRoute *route, Lyst resultLyst);
//Added by L. Persampieri 04/04/20
extern int decode_rgr(unsigned int bytesBuffered, unsigned char *bufferEncoded, GeoRoute *geoRouteToDecode, int TODO_MTAKE_nodes);
//Added by L. Persampieri 04/04/20
extern unsigned int encode_rgr(GeoRoute *geoRouteToEncode, unsigned char ** bufferEncoded);
#endif /* _RGR_UTILS_H_ */
