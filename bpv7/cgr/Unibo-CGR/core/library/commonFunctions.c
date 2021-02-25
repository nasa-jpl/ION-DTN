/** \file commonFunctions.c
 *
 *  \brief  The purpose of this file is to keep in one place
 *          the functions used by many (or all) files.
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
 *  \author Lorenzo Persampieri, lorenzo.persampieri@studio.unibo.it
 *
 *  \par Supervisor
 *       Carlo Caini, carlo.caini@unibo.it
 */


#include "commonDefines.h"

/******************************************************************************
 *
 * \par Function Name:
 * 		MDEPOSIT_wrapper
 *
 * \brief Call the MDEPOSIT macro, so deallocate memory pointed by addr.
 *
 * \details Use this function when you need to assign MDEPOSIT to a function pointer.
 *        For any other reason you should use MDEPOSIT.
 *
 *
 * \par Date Written:
 * 		13/05/20
 *
 * \return void
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  13/05/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
void MDEPOSIT_wrapper(void *addr)
{
    if(addr != NULL)
    {
        MDEPOSIT(addr);
    }
}
