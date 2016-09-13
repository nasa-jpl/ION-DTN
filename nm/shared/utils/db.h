/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: db.h
 **
 ** Description: This file contains the definitions, prototypes, constants, and
 **              other information necessary for DTNMP actors to interact with
 **              SDRs to persistently store information.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  06/29/13  E. Birrane Initial Implementation (JHU/APL)
 **  08/21/16  E. Birrane     Update to AMP v02 (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/
#ifndef DB_H_
#define DB_H_

#include "ion_if.h"
#include "nm_types.h"

int  db_forget(Object *primitiveObj, Object *descObj, Object list);

int  db_persist(uint8_t  *item,
		        uint32_t  item_len,
		        Object   *itemObj,
		        void     *desc,
		        uint32_t  desc_len,
		        Object   *descObj,
		        Object    list);



#endif /* DB_H_ */
