//
//  adm_bp.h
//
//  Created by Birrane, Edward J. on 10/22/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//
#ifndef ADM_ION_H_
#define ADM_ION_H_

#include "lyst.h"
#include "bpnm.h"
#include "icinm.h"


#include "shared/utils/nm_types.h"
#include "shared/adm/adm.h"

/*
 * We will invent an OID space for ICI MIB information, to live at:
 *
 * iso.identified-organization.dod.internet.mgmt.mib-2.ion
 * or 1.2.3.1.2.1.0.1
 * or, as OID,: 2A 03 01 02 01 00 01
 *
 * \todo EJB 0 here is not a valid entry. Just using something for now.
 */

/*
 * +--------------------------------------------------------------------------+
 * |						      ADM CONSTANTS  							  +
 * +--------------------------------------------------------------------------+
 */

static char* ION_ADM_NICKNAME = "2A030102010001";


/*
 * +--------------------------------------------------------------------------+
 * |					  ADM ATOMIC DATA DEFINITIONS  						  +
 * +--------------------------------------------------------------------------+
 */


/*
 * Structure:
 *
 *                   ADM_ION_ROOT (2A030102010001)
 *                        |
 *      ICI (01)          |    INDUCT (02)    OUTDUCT (03)     NODE (04)
 *           +-----------------------+--------------+--------------+
 *           |                       |              |              |
 */



/* ION ICI Data Items (2A03010201000101*) */
static char *ION_ICI_SDR_STATE_ALL    = "00092A0301020100010100";
static char *ION_ICI_SMALL_POOL_SIZE  = "00092A0301020100010101";
static char *ION_ICI_SMALL_POOL_FREE  = "00092A0301020100010102";
static char *ION_ICI_SMALL_POOL_ALLOC = "00092A0301020100010103";
static char *ION_ICI_LARGE_POOL_SIZE  = "00092A0301020100010104";
static char *ION_ICI_LARGE_POOL_FREE  = "00092A0301020100010105";
static char *ION_ICI_LARGE_POOL_ALLOC = "00092A0301020100010106";
static char *ION_ICI_UNUSED_SIZE      = "00092A0301020100010107";


/* ION INDUCT Items (2A03010201000102*) */
static char *ION_INDUCT_ALL              = "40092A0301020100010200";
static char *ION_INDUCT_NAME             = "40092A0301020100010201";
static char *ION_INDUCT_LAST_RESET       = "40092A0301020100010202";
static char *ION_INDUCT_RX_BUNDLES       = "40092A0301020100010203";
static char *ION_INDUCT_RX_BYTES         = "40092A0301020100010204";
static char *ION_INDUCT_MAL_BUNDLES      = "40092A0301020100010205";
static char *ION_INDUCT_MAL_BYTES        = "40092A0301020100010206";
static char *ION_INDUCT_INAUTH_BUNDLES   = "40092A0301020100010207";
static char *ION_INDUCT_INAUTH_BYTES     = "40092A0301020100010208";
static char *ION_INDUCT_OVERFLOW_BUNDLES = "40092A0301020100010209";
static char *ION_INDUCT_OVERFLOW_BYTES   = "40092A030102010001020A";


/* ION OUTDUCT Items (2A03010201000103*) */
static char *ION_OUTDUCT_ALL               = "40092A0301020100010300";
static char *ION_OUTDUCT_NAME              = "40092A0301020100010301";
static char *ION_OUTDUCT_CUR_QUEUE_BUNDLES = "40092A0301020100010302";
static char *ION_OUTDUCT_CUR_QUEUE_BYTES   = "40092A0301020100010303";
static char *ION_OUTDUCT_LAST_RESET        = "40092A0301020100010304";
static char *ION_OUTDUCT_ENQUEUED_BUNDLES  = "40092A0301020100010305";
static char *ION_OUTDUCT_ENQUEUED_BYTES    = "40092A0301020100010306";
static char *ION_OUTDUCT_DEQUEUED_BUNDLES  = "40092A0301020100010307";
static char *ION_OUTDUCT_DEQUEUED_BYTES    = "40092A0301020100010308";


/* ION NODE Items (2A03010201000104*) */
static char *ION_NODE_ALL      = "00092A0301020100010400";
static char *ION_NODE_INDUCTS  = "00092A0301020100010401";
static char *ION_NODE_OUTDUCTS = "00092A0301020100010402";


/* ION Controls */
static char *ION_INDUCT_RESET  = "01092A0301020100010500";
static char *ION_OUTDUCT_RESET = "01092A0301020100010501";



void initIonAdm();

/* Print Functions */
char *iciPrintSdrStateAll(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len);
char *ion_node_print_all(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len);
char *ion_induct_print_all(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len);
char *ion_outduct_print_all(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len);


/* Retrieval Functions */

/* ION ICI */
uint8_t *iciGetSdrStateAll(Lyst params, uint64_t *length);
uint8_t *iciGetSmallPoolSize(Lyst params, uint64_t *length);
uint8_t *iciGetSmallPoolFree(Lyst params, uint64_t *length);
uint8_t *iciGetSmallPoolAlloc(Lyst params, uint64_t *length);
uint8_t *iciGetLargePoolSize(Lyst params, uint64_t *length);
uint8_t *iciGetLargePoolFree(Lyst params, uint64_t *length);
uint8_t *iciGetLargePoolAlloc(Lyst params, uint64_t *length);
uint8_t *iciGetUnusedSize(Lyst params, uint64_t *length);


/* ION INDUCT */
uint8_t *ion_induct_get_all(Lyst params, uint64_t *length);
uint8_t *ion_induct_get_name(Lyst params, uint64_t *length);
uint8_t *ion_induct_get_last_reset(Lyst params, uint64_t *length);
uint8_t *ion_induct_get_rx_bndl(Lyst params, uint64_t *length);
uint8_t *ion_induct_get_rx_byte(Lyst params, uint64_t *length);
uint8_t *ion_induct_get_mal_bndl(Lyst params, uint64_t *length);
uint8_t *ion_induct_get_mal_byte(Lyst params, uint64_t *length);
uint8_t *ion_induct_get_inauth_bndl(Lyst params, uint64_t *length);
uint8_t *ion_induct_get_inauth_byte(Lyst params, uint64_t *length);
uint8_t *ion_induct_get_over_bndl(Lyst params, uint64_t *length);
uint8_t *ion_induct_get_over_byte(Lyst params, uint64_t *length);


/* ION OUTDUCT */
uint8_t *ion_outduct_get_all(Lyst params, uint64_t *length);
uint8_t *ion_outduct_get_name(Lyst params, uint64_t *length);
uint8_t *ion_outduct_get_cur_q_bdnl(Lyst params, uint64_t *length);
uint8_t *ion_outduct_get_cur_q_byte(Lyst params, uint64_t *length);
uint8_t *ion_outduct_get_last_reset(Lyst params, uint64_t *length);
uint8_t *ion_outduct_get_enq_bndl(Lyst params, uint64_t *length);
uint8_t *ion_outduct_get_enq_byte(Lyst params, uint64_t *length);
uint8_t *ion_outduct_get_deq_bndl(Lyst params, uint64_t *length);
uint8_t *ion_outduct_get_deq_byte(Lyst params, uint64_t *length);

/* ION NODE */
uint8_t *ion_node_get_all(Lyst params, uint64_t *length);
uint8_t *ion_node_get_inducts(Lyst params, uint64_t *length);
uint8_t *ion_node_get_outducts(Lyst params, uint64_t *length);





/* Sizing Functions */

/* ION ICI */
uint32_t iciSizeSdrStateAll(uint8_t* buffer, uint64_t buffer_len);
uint32_t iciSizeSmallPoolSize(uint8_t* buffer, uint64_t buffer_len);
uint32_t iciSizeSmallPoolFree(uint8_t* buffer, uint64_t buffer_len);
uint32_t iciSizeSmallPoolAlloc(uint8_t* buffer, uint64_t buffer_len);
uint32_t iciSizeLargePoolSize(uint8_t* buffer, uint64_t buffer_len);
uint32_t iciSizeLargePoolFree(uint8_t* buffer, uint64_t buffer_len);
uint32_t iciSizeLargePoolAlloc(uint8_t* buffer, uint64_t buffer_len);
uint32_t iciSizeUnusedSize(uint8_t* buffer, uint64_t buffer_len);

/* ION INDUCT */
uint32_t ion_induct_size_all(uint8_t* buffer, uint64_t buffer_len);

/* ION OUTDUCT */
uint32_t ion_outduct_size_all(uint8_t* buffer, uint64_t buffer_len);

/* ION NODE */
uint32_t ion_node_size_all(uint8_t* buffer, uint64_t buffer_len);
uint32_t ion_node_size_inducts(uint8_t* buffer, uint64_t buffer_len);
uint32_t ion_node_size_outducts(uint8_t* buffer, uint64_t buffer_len);


#endif //ADM_ION_H_
