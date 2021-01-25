/** \file cgrr.h
 *
 *	\brief		definitions supporting implementation of
 *				the CGR Route extension block. All functions defined
 *				here are requested and used by libbpP, except cgrr_attach.
 *				Other functions specific to the use of this extension block
 *				can be found in cgrr_util.
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

#ifndef _CGRR_H_

#define _CGRR_H_


#include "cgrr_utils.h"

#define	EXTENSION_TYPE_CGRR	23 //Unused in BPv7

extern int	cgrr_offer(ExtensionBlock *, Bundle *);
extern void	cgrr_release(ExtensionBlock *);
extern int	cgrr_record(ExtensionBlock *, AcqExtBlock *);
extern int	cgrr_copy(ExtensionBlock *, ExtensionBlock *);
extern int	cgrr_acquire(AcqExtBlock *, AcqWorkArea *);
extern int	cgrr_check(AcqExtBlock *, AcqWorkArea *);
extern void	cgrr_clear(AcqExtBlock *);

extern int cgrr_attach(ExtensionBlock *blk, CGRRouteBlock *cgrrBlk);
#endif
