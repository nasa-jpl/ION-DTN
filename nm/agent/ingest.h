/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: ingest.h
 **
 ** Description: This implements the data ingest thread to receive DTNMP msgs.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  01/10/13  E. Birrane     Initial Implementation (JHU/APL)
 *****************************************************************************/

#ifndef _INGEST_H_
#define _INGEST_H_

#include "../shared/msg/msg.h"

void *rx_thread(int *running);
void rx_handle_perf_ctrl(msg_metadata_t *meta, blob_t *contents);

#endif /* _INGEST_H_ */
