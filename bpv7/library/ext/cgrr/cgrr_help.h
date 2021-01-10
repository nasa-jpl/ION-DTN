/** \file cgrr_help.h
 *
 *	\brief       This file provides helper functions that use "cgr.h" library.
 *
 *  \details     We need this separated header file just to avoid some compilation error in ION 4.0.0.
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
 *	\author Lorenzo Persampieri, lorenzo.persampieri@studio.unibo.it
 *
 *	\par Supervisor
 *	     Carlo Caini, carlo.caini@unibo.it
 */
#ifndef BPV7_LIBRARY_EXT_CGRR_CGRR_HELP_H_
#define BPV7_LIBRARY_EXT_CGRR_CGRR_HELP_H_

#include "cgrr.h"
#include "cgr.h"

typedef struct
{
	unsigned int fragmOffset;
	unsigned int fragmLength;
	uvast evc;
	int readLock; /*  0 if you should not read the values, 1 otherwise */
	int cloneLevel; /* 1 if this CGRR Ext. Block has been cloned from
	                      another CGRR Ext. Block (i.e. due bundle's fragmentation).
	                   2 if the bundle has been cloned
	                   0 otherwise
	               */
} CGRRObject;

#ifdef __cplusplus
extern "C" {
#endif

extern int getCGRRoute(CgrRoute *in_route, CGRRoute * out_route);

#ifdef __cplusplus
}
#endif

#endif /* BPV7_LIBRARY_EXT_CGRR_CGRR_HELP_H_ */
