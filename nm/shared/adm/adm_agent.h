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
 ** File Name: adm_agent_public.h
 **
 ** Description: This implements the public portions of a DTNMP Agent ADM.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  07/04/13  E. Birrane     Initial Implementation
 *****************************************************************************/
#ifndef ADM_AGENT_H_
#define ADM_AGENT_H_

#include "lyst.h"

#include "shared/utils/nm_types.h"
#include "shared/adm/adm.h"

/*
 * We will invent an OID space for DTNMP AGENT ADM information, to live at:
 *
 * iso.identified-organization.dod.internet.mgmt.dtnmp.agent
 * or 1.3.6.1.2.3.3
 * or, as OID,: 2B 06 01 02 03 03
 *
 * Note: dtnmp.bp is a made-up subtree.
 */

/*
 * +--------------------------------------------------------------------------+
 * |						      ADM CONSTANTS  							  +
 * +--------------------------------------------------------------------------+
 */


/*
 * +--------------------------------------------------------------------------+
 * |					  ADM ATOMIC DATA DEFINITIONS  						  +
 * +--------------------------------------------------------------------------+
 */


/*
 * Structure:
 *
 *   ADM_AGENT_ROOT (2A0601020303)
 *           |
 *           |
 *  Data(1)  |   Ctrl(2)       Op(3)       Literals (4)
 *     +-----+------+------------+-------------+
 */



static char* ADM_AGENT_ROOT = "2B0601020303";
#define ADM_AGENT_ROOT_LEN (6)

static char* ADM_AGENT_NODE_NN = "2B060102030301";
#define  ADM_AGENT_NODE_NN_LEN  (7)

static char* ADM_AGENT_CTRL_NN = "2B060102030302";
#define ADM_AGENT_CTRL_NN_LEN  (7)

static char* ADM_AGENT_OP_NN = "2B060102030303";
#define ADM_AGENT_OP_NN_LEN (7)

static char* ADM_AGENT_LIT_NN = "2B060102030304";
#define ADM_AGENT_LIT_NN_LEN (7)

/* AGENT DATA */


/* Ops
static char *DTNMP_AGENT_OP_ADD      = "030A2A030102010101010300";
static char *DTNMP_AGENT_OP_SUB      = "030A2A030102010101010301";
static char *DTNMP_AGENT_OP_DIV      = "030A2A030102010101010302";
static char *DTNMP_AGENT_OP_MULT     = "030A2A030102010101010303";
static char *DTNMP_AGENT_OP_BIT_AND  = "030A2A030102010101010304";
static char *DTNMP_AGENT_OP_BIT_OR   = "030A2A030102010101010305";
static char *DTNMP_AGENT_OP_BIT_XOR  = "030A2A030102010101010306";
static char *DTNMP_AGENT_OP_BIT_NOT  = "030A2A030102010101010307";
static char *DTNMP_AGENT_OP_LT       = "030A2A030102010101010308";
static char *DTNMP_AGENT_OP_LTE      = "030A2A030102010101010309";
static char *DTNMP_AGENT_OP_GT       = "030A2A03010201010101030A";
static char *DTNMP_AGENT_OP_GTE      = "030A2A03010201010101030B";
static char *DTNMP_AGENT_OP_EQ       = "030A2A03010201010101030C";
static char *DTNMP_AGENT_OP_NEQ      = "030A2A03010201010101030D";
static char *DTNMP_AGENT_OP_LOG_AND  = "030A2A03010201010101030E";
static char *DTNMP_AGENT_OP_LOG_OR   = "030A2A03010201010101030F";
static char *DTNMP_AGENT_OP_LOG_NOT  = "030A2A030102010101010310";
static char *DTNMP_AGENT_OP_LSHFT    = "030A2A030102010101010311";
static char *DTNMP_AGENT_OP_RSHFT    = "030A2A030102010101010312";
static char *DTNMP_AGENT_OP_MOD      = "030A2A030102010101010313";
static char *DTNMP_AGENT_OP_NEG      = "030A2A030102010101010314";

*/


void adm_agent_init();

/* Print Functions */
char *adm_print_agent_all(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len);

/* Sizing Functions */
uint32_t adm_size_agent_all(uint8_t* buffer, uint64_t buffer_len);




#endif //ADM_AGENT_H_
