/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 **     This material may only be used, modified, or reproduced by or for the
 **       U.S. Government pursuant to the license rights granted under
 **          FAR clause 52.227-14 or DFARS clauses 252.227-7013/7014
 **
 **     For any other permissions, please contact the Legal Office at JHU/APL.
 ******************************************************************************/

/*****************************************************************************
 **
 ** \file msg_reports.h
 **
 **
 ** Description: Defines the serialization and de-serialization methods
 **              used to translate data types into byte streams associated
 **              with DTNMP messages, and vice versa.
 **
 ** Notes:
 ** 			 Not all fields of internal structures are serialized into/
 **				 deserialized from DTNMP messages.
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  09/09/11  M. Reid        Initial Implementation (in other files)
 **  11/02/12  E. Birrane     Redesign of messaging architecture.
 **  01/11/13  E. Birrane     Migrate data structures to primitives.
 *****************************************************************************/


#ifndef MSG_REPORTS_H_
#define MSG_REPORTS_H_

#include "lyst.h"

#include "shared/utils/nm_types.h"

#include "shared/primitives/mid.h"
#include "shared/primitives/report.h"
#include "shared/msg/pdu.h"

/*
 * +--------------------------------------------------------------------------+
 * |							  CONSTANTS  								  +
 * +--------------------------------------------------------------------------+
 */

/* Reporting messages */
#define MSG_TYPE_RPT_DATA_LIST    (0x10)
#define MSG_TYPE_RPT_DATA_DEFS    (0x11)
#define MSG_TYPE_RPT_DATA_RPT     (0x12)
#define MSG_TYPE_RPT_PROD_SCHED   (0x13)


/*
 * +--------------------------------------------------------------------------+
 * |							  	MACROS  								  +
 * +--------------------------------------------------------------------------+
 */


/*
 * +--------------------------------------------------------------------------+
 * |							  DATA TYPES  								  +
 * +--------------------------------------------------------------------------+
 */


/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */



/* Serialize functions. */
uint8_t *rpt_serialize_lst(rpt_items_t *msg, uint32_t *len);
uint8_t *rpt_serialize_defs(rpt_defs_t *msg, uint32_t *len);

uint8_t *rpt_serialize_data_entry(rpt_data_entry_t *entry, uint32_t *len);
uint8_t *rpt_serialize_data(rpt_data_t *msg, uint32_t *len);


uint8_t *rpt_serialize_prod(rpt_prod_t *msg, uint32_t *len);


/* Deserialize functions. */
rpt_items_t *rpt_deserialize_lst(uint8_t *cursor,
								 uint32_t size,
								 uint32_t *bytes_used);

rpt_defs_t *rpt_deserialize_defs(uint8_t *cursor,
		                       	 uint32_t size,
		                       	 uint32_t *bytes_used);

rpt_data_t *rpt_deserialize_data(uint8_t *cursor,
		                       	 uint32_t size,
		                       	 uint32_t *bytes_used);

rpt_prod_t *rpt_deserialize_prod(uint8_t *cursor,
		                       	 uint32_t size,
		                       	 uint32_t *bytes_used);


#endif // MSG_REPORTS_H_
