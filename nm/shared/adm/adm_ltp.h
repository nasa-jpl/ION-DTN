//
//  adm_bp.h
//
//  Created by Birrane, Edward J. on 10/22/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//
#ifndef ADM_LTP_H_
#define ADM_LTP_H_

#ifdef _HAVE_LTP_ADM_

#include "lyst.h"
#include "ltpnm.h"



#include "shared/utils/nm_types.h"

#include "shared/adm/adm.h"

/*
 * We will invent an OID space for LTP ADM information, to live at:
 *
 * iso.identified-organization.dod.internet.mgmt.dtnmp.ltp
 * or 1.3.6.1.2.3.2
 * or, as OID,: 2B 06 01 02 03 02
 *
 * Note: dtnmp.ltp is a made-up subtree.
 */


/*
 * Structure:
 *
 *                   ADM_LTP_ROOT (2A0601020302)
 *                        |
 *      NODE (01)         |    ENGINE (02)
 *           +-----------------------+
 *           |                       |
 */

static char* ADM_LTP_ROOT = "2B0601020302";
#define ADM_LTP_ROOT_LEN (6)

static char* ADM_LTP_NODE_NN = "2B060102030201";
#define ADM_LTP_NODE_NN_LEN (7)

static char* ADM_LTP_ENGINE_NN = "2B060102030202";
#define ADM_LTP_ENGINE_NN_LEN (7)

static char* ADM_LTP_CTRL_NN = "2B060102030203";
#define ADM_LTP_CTRL_NN_LEN  (7)


void adm_ltp_init();


/* Print Functions */
char *ltp_print_node_resources_all(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len);

char *ltp_engine_print_all(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len);

/* SIZE */
uint32_t ltp_size_node_resources_all(uint8_t* buffer, uint64_t buffer_len);
uint32_t ltp_size_heap_bytes_reserved(uint8_t* buffer, uint64_t buffer_len);
uint32_t ltp_size_heap_bytes_used(uint8_t* buffer, uint64_t buffer_len);

uint32_t ltp_engine_size(uint8_t* buffer, uint64_t buffer_len);

#endif /* _HAVE_LTP_ADM */
#endif //ADM_LTP_H_
