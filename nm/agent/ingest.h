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

#ifndef INGEST_H
#define INGEST_H

#include "../shared/msg/msg.h"

#ifdef __cplusplus
extern "C" {
#endif

void *rx_thread(void *arg);
void rx_handle_perf_ctrl(msg_metadata_t *meta, blob_t *contents);

#ifdef __cplusplus
}
#endif

#endif /* INGEST_H */
