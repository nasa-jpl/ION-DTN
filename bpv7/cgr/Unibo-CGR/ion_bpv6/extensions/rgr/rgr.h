/** \file rgr.h
 *
 *	\brief  definitions supporting implementation of
 *			the Register Route extension block.
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
#ifndef _RGR_H_
#define _RGR_H_

#include "rgr_utils.h"

#define	EXTENSION_TYPE_RGR	22

extern int	rgr_offer(ExtensionBlock *, Bundle *);
extern void	rgr_release(ExtensionBlock *);
extern int	rgr_record(ExtensionBlock *, AcqExtBlock *);
extern int	rgr_copy(ExtensionBlock *, ExtensionBlock *);
//extern int	rgr_processOnTransmit(ExtensionBlock *, Bundle *, void *);
extern int	rgr_processOnDequeue(ExtensionBlock *, Bundle *, void *);
extern int	rgr_acquire(AcqExtBlock *, AcqWorkArea *);
extern void	rgr_clear(AcqExtBlock *);
extern int  rgr_parse(AcqExtBlock *, AcqWorkArea *);

int rgr_attach(Bundle *bundle, ExtensionBlock *blk, GeoRoute *rgrBlk);

#endif /* _RGR_H_ */
