/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 ******************************************************************************/


/*****************************************************************************
 **
 ** File Name: adm_agent.h
 **
 **
 ** Description: This file implements the public aspects of an AMP Agent ADM.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  07/04/13  E. Birrane     Initial Implementation (JHU/APL)
 **  07/03/16  E. Birrane     Updated to Agent ADM v0.2 (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

#ifndef ADM_AGENT_H_
#define ADM_AGENT_H_

#include "lyst.h"

#include "../utils/nm_types.h"
#include "../adm/adm.h"
#include "../primitives/lit.h"



/*
 * +--------------------------------------------------------------------------+
 * |				     ADM TEMPLATE DOCUMENTATION  						  +
 * +--------------------------------------------------------------------------+
 *
 * ADM ROOT STRING    : iso.identified-organization.dod.internet.mgmt.dtnmp.agent
 * ADM ROOT ID STRING : 1.3.6.1.2.3.3
 * ADM ROOT OID       : 2B 06 01 02 03 03
 * ADM NICKNAMES      : 0 -> 0x2B0601020303
 *
 *
 *                             AGENT ADM ROOT
 *                             (1.3.6.1.2.3.3)
 *                                   |
 *                                   |
 *   Meta-   Atomic  Computed        |
 *   Data    Data      Data    Rpts  |  Ctrls  Literals  Macros   Ops
 *    (.0)   (.1)      (.2)    (.3)  |  (.4)    (.5)      (.6)    (.7)
 *      +-------+---------+------+------+--------+----------+---------+
 *
 */


/*
 * +--------------------------------------------------------------------------+
 * |					    AGENT NICKNAME DEFINITIONS  					  +
 * +--------------------------------------------------------------------------+
 *
   +--------+----------------------------+-----------------------------+
   | Unique |           Label            |       OID as ASN.1 BER      |
   |   ID   |                            |                             |
   +--------+----------------------------+-----------------------------+
   |   0    |       Agent Metadata       |       0x2B060102030300      |
   |        |                            |                             |
   |   1    |   Agent Primitive Values   |       0x2B060102030301      |
   |        |                            |                             |
   |   2    |   Agent Computed Values    |       0x2B060102030302      |
   |        |                            |                             |
   |   3    |       Agent Reports        |       0x2B060102030303      |
   |        |                            |                             |
   |   4    |       Agent Controls       |       0x2B060102030304      |
   |        |                            |                             |
   |   5    |       Agent Literals       |       0x2B060102030305      |
   |        |                            |                             |
   |   6    |        Agent Macros        |       0x2B060102030306      |
   |        |                            |                             |
   |   7    |      Agent Operators       |       0x2B060102030307      |
   |        |                            |                             |
   |   8    |         Agent Root         |        0x2B0601020303       |
   +--------+----------------------------+-----------------------------+
 */

#define AGENT_ADM_MD_NN_IDX 0
#define AGENT_ADM_MD_NN_STR "2B060102030300"

#define AGENT_ADM_AD_NN_IDX 1
#define AGENT_ADM_AD_NN_STR "2B060102030301"

#define AGENT_ADM_CD_NN_IDX 2
#define AGENT_ADM_CD_NN_STR "2B060102030302"

#define AGENT_ADM_RPT_NN_IDX 3
#define AGENT_ADM_RPT_NN_STR "2B060102030303"

#define AGENT_ADM_CTRL_NN_IDX 4
#define AGENT_ADM_CTRL_NN_STR "2B060102030304"

#define AGENT_ADM_LTRL_NN_IDX 5
#define AGENT_ADM_LTRL_NN_STR "2B060102030305"

#define AGENT_ADM_MAC_NN_IDX 6
#define AGENT_ADM_MAC_NN_STR "2B060102030306"

#define AGENT_ADM_OP_NN_IDX 7
#define AGENT_ADM_OP_NN_STR "2B060102030307"

#define AGENT_ADM_ROOT_NN_IDX 8
#define AGENT_ADM_ROOT_NN_STR "2B0601020303"

/*
 * +--------------------------------------------------------------------------+
 * |					  AGENT META-DATA DEFINITIONS  						  +
 * +--------------------------------------------------------------------------+

   +---------+------------+--------+------------------+------+---------+
   |   Item  | MID (Hex)  |  OID   |   Description    | Type |  Value  |
   |         |            | (Str)  |                  |      |         |
   +---------+------------+--------+------------------+------+---------+
   |   Name  | 0x80000100 | [0].0  |    The human-    | STR  |   AMP   |
   |         |            |        |   readable ADM   |      |  Agent  |
   |         |            |        |      name.       |      |   ADM   |
   +---------+------------+--------+------------------+------+---------+
   | Version | 0x80000101 | [0].1  | The ADM version. | STR  |   v0.2  |
   +---------+------------+--------+------------------+------+---------+
 */

// "DTNMP AGENT ADM"
#define ADM_AGENT_MD_NAME_MID	"80000100"

// "v0.2"
#define ADM_AGENT_MD_VER_MID    "80000101"


/*
 * +--------------------------------------------------------------------------+
 * |					  AGENT ATOMIC DATA DEFINITIONS  					  +
 * +--------------------------------------------------------------------------+

+-----------+------------+-------+---------------------------+------+
   |    Name   |    MID     |  OID  |        Description        | Type |
   +-----------+------------+-------+---------------------------+------+
   |    Num    | 0x80010100 | [1].0 |   # Reports known to the  | UINT |
   |  Reports  |            |       |           Agent.          |      |
   +-----------+------------+-------+---------------------------+------+
   |    Sent   | 0x80010101 | [1].1 |   # Reports sent by this  | UINT |
   |  Reports  |            |       |  Agent since last reset.  |      |
   +-----------+------------+-------+---------------------------+------+
   |  Num TRL  | 0x80010102 | [1].2 | # Time-Based Rules (TRLs) | UINT |
   |           |            |       |   running on the Agent.   |      |
   +-----------+------------+-------+---------------------------+------+
   |  Run TRL  | 0x80010103 | [1].3 | # Time-Based Rules (TRLs) | UINT |
   |           |            |       |   run by the Agent since  |      |
   |           |            |       |        last reset.        |      |
   +-----------+------------+-------+---------------------------+------+
   |  Num SRL  | 0x80010104 | [1].4 |    # State-Based Rules    | UINT |
   |           |            |       |   (SRLs) running on the   |      |
   |           |            |       |           Agent.          |      |
   +-----------+------------+-------+---------------------------+------+
   |  Run SRL  | 0x80010105 | [1].5 |    # State-Based Rules    | UINT |
   |           |            |       |  (SRLs) run by the Agent  |      |
   |           |            |       |     since last reset.     |      |
   +-----------+------------+-------+---------------------------+------+
   |  Num Lit  | 0x80010106 | [1].6 |   # Literal definitions   | UINT |
   |           |            |       |    known to the Agent.    |      |
   +-----------+------------+-------+---------------------------+------+
   |    Num    | 0x80010107 | [1].7 |  # Variables known to the | UINT |
   | Variables |            |       |           Agent.          |      |
   +-----------+------------+-------+---------------------------+------+
   |    Num    | 0x80010108 | [1].8 |    # Macro definitions    | UINT |
   |   Macros  |            |       |  configured on the Agent. |      |
   +-----------+------------+-------+---------------------------+------+
   |    Run    | 0x80010109 | [1].9 | # Macros run by the Agent | UINT |
   |   Macros  |            |       |   since the last reset.   |      |
   +-----------+------------+-------+---------------------------+------+
   |    Num    | 0x8001010A | [1].A |  # Controls known by the  | UINT |
   |  Controls |            |       |           Agent.          |      |
   +-----------+------------+-------+---------------------------+------+
   |    Run    | 0x8001010B | [1].B |   # Controls run by the   | UINT |
   |  Controls |            |       |    Agent since the last   |      |
   |           |            |       |           reset.          |      |
   +-----------+------------+-------+---------------------------+------+
   |  Current  | 0x8001010C | [1].C |       Current time.       |  TS  |
   |    Time   |            |       |                           |      |
   +-----------+------------+-------+---------------------------+------+
 */

#define ADM_AGENT_AD_NUMRPT_MID   "80010100"
#define ADM_AGENT_AD_SENTRPT_MID  "80010101"
#define ADM_AGENT_AD_NUMTRL_MID   "80010102"
#define ADM_AGENT_AD_RUNTRL_MID   "80010103"
#define ADM_AGENT_AD_NUMSRL_MID   "80010104"
#define ADM_AGENT_AD_RUNSRL_MID   "80010105"
#define ADM_AGENT_AD_NUMLIT_MID   "80010106"
#define ADM_AGENT_AD_NUMCUST_MID  "80010107"
#define ADM_AGENT_AD_NUMMAC_MID   "80010108"
#define ADM_AGENT_AD_RUNMAC_MID   "80010109"
#define ADM_AGENT_AD_NUMCTRL_MID  "8001010A"
#define ADM_AGENT_AD_RUNCTRL_MID  "8001010B"
#define ADM_AGENT_AD_CURTIME_MID  "8001010C"

/*
 * +--------------------------------------------------------------------------+
 * |				    AGENT COMPUTED DATA DEFINITIONS 					  +
 * +--------------------------------------------------------------------------+

   +----------+------------+-------+----------------------------+------+
   |   Name   |    MID     |  OID  |        Description         | Type |
   +----------+------------+-------+----------------------------+------+
   |   Num    | 0x81020100 | [2].0 |  # Rules know to the Agent | UINT |
   |   Rules  |            |       |      (# TRL + # SRL).      |      |
   +----------+------------+-------+----------------------------+------+
   |                            Definition                             |
   +-------------------------------------------------------------------+
   | 0x03 0x80010102 0x80010104 0x83070100                             |
   +-------------------------------------------------------------------+
 */

#define ADM_AGENT_CD_NUMRULE_MID    "81020100"



/*
 * +--------------------------------------------------------------------------+
 * |				    	AGENT REPORT DEFINITIONS						  +
 * +--------------------------------------------------------------------------+

   +------------+----------+---------+------------------+--------------+
   |    Name    |   MID    |   OID   |   Description    |     Type     |
   +------------+----------+---------+------------------+--------------+
   | FullReport | 82030100 | [3].0   |  Report of all   |      DC      |
   |            |          |         |   atomic data    |              |
   |            |          |         |      items       |              |
   +------------+----------+---------+------------------+--------------+
 */

#define ADM_AGENT_RPT_FULL_MID  "82030100"



/*
 * +--------------------------------------------------------------------------+
 * |				    AGENT CONTROL DEFINITIONS CONSTANTS  				  +
 * +--------------------------------------------------------------------------+

   +--------------+-------------+--------+------+----------------------+
   |     Name     |     MID     |  OID   |  #   |         Prms         |
   |              |             |        | Prms |                      |
   +--------------+-------------+--------+------+----------------------+
   |   ListADMs   |  0x83040100 | [4].0  |  0   |                      |
   +--------------+-------------+--------+------+----------------------+
   |    AddVar    |  0xC3040101 | [4].1  |  4   |  MID Id, EXPR Def,   |
   |              |             |        |      | BYTE Type, BYTE Flg  |
   +--------------+-------------+--------+------+----------------------+
   |    DelVar    |  0xC3040102 | [4].2  |  1   |        MC IDs        |
   +--------------+-------------+--------+------+----------------------+
   |   ListVars   |  0x83040103 | [4].3  |  0   |                      |
   +--------------+-------------+--------+------+----------------------+
   |   DescVars   |  0xC3040104 | [4].4  |  1   |        MC IDs        |
   +--------------+-------------+--------+------+----------------------+
   |  AddRptTpl   |  0xC3040105 | [4].5  |  2   | MID ID, MC Template  |
   +--------------+-------------+--------+------+----------------------+
   |  DelRptTpl   |  0xC3040106 | [4].6  |  1   |        MC IDs        |
   +--------------+-------------+--------+------+----------------------+
   | ListRptTpls  |  0x83040107 | [4].7  |  0   |                      |
   +--------------+-------------+--------+------+----------------------+
   | DescRptTpls  |  0xC3040108 | [4].8  |  1   |        MC IDs        |
   +--------------+-------------+--------+------+----------------------+
   | GenerateRpts |  0xC3040109 | [4].9  |  2   |  MC IDs, DC RxMgrs   |
   +--------------+-------------+--------+------+----------------------+
   |   AddMacro   |  0xC304010A | [4].A  |  3   | STR Name, MID ID, MC |
   |              |             |        |      |         Def          |
   +--------------+-------------+--------+------+----------------------+
   |   DelMacro   |  0xC304010B | [4].B  |  1   |        MC IDs        |
   +--------------+-------------+--------+------+----------------------+
   |  ListMacros  |  0x8304010C | [4].C  |  0   |                      |
   +--------------+-------------+--------+------+----------------------+
   |  DescMacros  |  0xC304010D | [4].D  |  1   |        MC IDs        |
   +--------------+-------------+--------+------+----------------------+
   |    AddTRL    |  0xC304010E | [4].E  |  5   |  MID ID, TS Start,   |
   |              |             |        |      |  SDNV Period, SDNV   |
   |              |             |        |      |    Cnt, MC Action    |
   +--------------+-------------+--------+------+----------------------+
   |    DelTRL    |  0xC304010F | [4].F  |  1   |        MC IDs        |
   +--------------+-------------+--------+------+----------------------+
   |   ListTRLs   |  0x83040110 | [4].10 |  0   |                      |
   +--------------+-------------+--------+------+----------------------+
   |   DescTRLs   |  0xC3040111 | [4].11 |  1   |        MC IDs        |
   +--------------+-------------+--------+------+----------------------+
   |    AddSRL    | 0xC30401012 | [4].12 |  5   |  MID ID, TS Start,   |
   |              |             |        |      |   PRED State, SDNV   |
   |              |             |        |      |    Cnt, MC Action    |
   +--------------+-------------+--------+------+----------------------+
   |    DelSRL    |  0xC3040113 | [4].13 |  1   |        MC IDs        |
   +--------------+-------------+--------+------+----------------------+
   |   ListSRLs   |  0x83040114 | [4].14 |  0   |                      |
   +--------------+-------------+--------+------+----------------------+
   |   DescSRLs   |  0xC3040115 | [4].15 |  1   |        MC IDs        |
   +--------------+-------------+--------+------+----------------------+
   |   StoreVar   |  0xC3040116 | [4].16 |  1   |        MC IDs        |
   +--------------+-------------+--------+------+----------------------+
   | ResetCounts  |  0x83040117 | [4].17 |  0   |                      |
   +--------------+-------------+--------+------+----------------------+
 */

#define ADM_AGENT_CTL_LSTADM_MID    "83040100"
#define ADM_AGENT_CTL_ADDCD_MID     "C3040101"
#define ADM_AGENT_CTL_DELCD_MID     "C3040102"
#define ADM_AGENT_CTL_LSTCD_MID     "83040103"
#define ADM_AGENT_CTL_DSCCD_MID     "C3040104"
#define ADM_AGENT_CTL_ADDRPT_MID    "C3040105"
#define ADM_AGENT_CTL_DELRPT_MID    "C3040106"
#define ADM_AGENT_CTL_LSTRPT_MID    "83040107"
#define ADM_AGENT_CTL_DSCRPT_MID    "C3040108"
#define ADM_AGENT_CTL_GENRPT_MID    "C3040109"
#define ADM_AGENT_CTL_ADDMAC_MID    "C304010A"
#define ADM_AGENT_CTL_DELMAC_MID    "C304010B"
#define ADM_AGENT_CTL_LSTMAC_MID    "8304010C"
#define ADM_AGENT_CTL_DSCMAC_MID    "C304010D"
#define ADM_AGENT_CTL_ADDTRL_MID    "C304010E"
#define ADM_AGENT_CTL_DELTRL_MID    "C304010F"
#define ADM_AGENT_CTL_LSTTRL_MID    "83040110"
#define ADM_AGENT_CTL_DSCTRL_MID    "C3040111"
#define ADM_AGENT_CTL_ADDSRL_MID    "C3040112"
#define ADM_AGENT_CTL_DELSRL_MID    "C3040113"
#define ADM_AGENT_CTL_LSTSRL_MID    "83040114"
#define ADM_AGENT_CTL_DSCSRL_MID    "C3040115"
#define ADM_AGENT_CTL_STOR_MID      "C3040116"
#define ADM_AGENT_CTL_RESET_CNTS    "83040117"

/*
 * +--------------------------------------------------------------------------+
 * |					  AGENT LITERAL DEFINTIONS  						  +
 * +--------------------------------------------------------------------------+

   +------------+-----------+-------+------------+--------+------------+
   |    Name    | MID (Hex) |  OID  |   Value    |  Type  |   Params   |
   +------------+-----------+-------+------------+--------+------------+
   | AMP Epoch  |  87050100 | [5].0 | 1347148800 |  UINT  |    None    |
   +------------+-----------+-------+------------+--------+------------+
   | User VAST  |  C7050101 | [5].1 |   Varies   | VAST   | VAST Value |
   +------------+-----------+-------+------------+--------+------------+
   | User UVAST |  C7050102 | [5].2 |   Varies   | UVAST  | SDNV Value |
   +------------+-----------+-------+------------+--------+------------+
   | User Float |  C7050103 | [5].3 |   Varies   | REAL32 | BLOB Value |
   +------------+-----------+-------+------------+--------+------------+
   |    User    |  C7050104 | [5].4 |   Varies   | REAL64 | BLOB Value |
   |   Double   |           |       |            |        |            |
   +------------+-----------+-------+------------+--------+------------+
   |    User    |  C7050105 | [5].5 |   Varies   |  STR   | STR Value  |
   |   String   |           |       |            |        |            |
   +------------+-----------+-------+------------+--------+------------+
   | User BLOB  |  C7050106 | [5].6 |   Varies   |  BLOB  | BLOB Value |
   +------------+-----------+-------+------------+--------+------------+

 */

#define ADM_AGENT_LIT_EPOCH_MID    "87050100"
#define ADM_AGENT_LIT_USRVAST_MID  "C7050101"
#define ADM_AGENT_LIT_USRUVAST_MID "C7050102"
#define ADM_AGENT_LIT_USRFLT_MID   "C7050103"
#define ADM_AGENT_LIT_USRDBL_MID   "C7050104"
#define ADM_AGENT_LIT_USRSTR_MID   "C7050105"
#define ADM_AGENT_LIT_USRBLOB_MID  "C7050106"


/*
 * +--------------------------------------------------------------------------+
 * |					  AGENT MACRO DEFINTIONS  						  +
 * +--------------------------------------------------------------------------+

  +----------+------------+-------+-----------------------------+
   |   Name   |    MID     |  OID  |        Description          |
   +----------+------------+-------+-----------------------------+
   |   User   | 0xC6060100 | [6].0 | List all user-defined data  |
   |   List   |            |       |                             |
   +----------+------------+-------+-----------------------------+
   |                       Definition                            |
   +-------------------------------------------------------------+
   | 0x04 0x83040103 0x83040107 0x8304010C 0x83040110 0x83040114 |
   +-------------------------------------------------------------+
 */

#define ADM_AGENT_MAC_FULL_MID    "C6060100"


/*
 * +--------------------------------------------------------------------------+
 * |				    	AGENT OPERATOR DEFINITIONS						  +
 * +--------------------------------------------------------------------------+

   +------+------------+--------+--------------------------+-----------+
   | Name |    MID     |  OID   |       Description        |     #     |
   |      |            |        |                          |  Operands |
   +------+------------+--------+--------------------------+-----------+
   |  +   | 0x88070100 | [7].0  |         Addition         |     2     |
   +------+------------+--------+--------------------------+-----------+
   |  -   | 0x88070101 | [7].1  |       Subtraction        |     2     |
   +------+------------+--------+--------------------------+-----------+
   |  *   | 0x88070102 | [7].2  |      Multiplication      |     2     |
   +------+------------+--------+--------------------------+-----------+
   |  /   | 0x88070103 | [7].3  |         Division         |     2     |
   +------+------------+--------+--------------------------+-----------+
   |  %   | 0x88070104 | [7].4  |          Modulo          |     2     |
   +------+------------+--------+--------------------------+-----------+
   |  ^   | 0x88070105 | [7].5  |      Exponentiation      |     2     |
   +------+------------+--------+--------------------------+-----------+
   |  &   | 0x88070106 | [7].6  |       Bitwise AND        |     2     |
   +------+------------+--------+--------------------------+-----------+
   |  |   | 0x88070107 | [7].7  |        Bitwise OR        |     2     |
   +------+------------+--------+--------------------------+-----------+
   |  #   | 0x88070108 | [7].8  |       Bitwise XOR        |     2     |
   +------+------------+--------+--------------------------+-----------+
   |  ~   | 0x88070109 | [7].9  |       Bitwise NOT        |     1     |
   +------+------------+--------+--------------------------+-----------+
   |  &&  | 0x8807010A | [7].A  |       Logical AND        |     2     |
   +------+------------+--------+--------------------------+-----------+
   |  ||  | 0x8807010B | [7].B  |        Logical OR        |     2     |
   +------+------------+--------+--------------------------+-----------+
   |  !   | 0x8807010C | [7].C  |       Logical NOT        |     1     |
   +------+------------+--------+--------------------------+-----------+
   | abs  | 0x8807010D | [7].D  |      Absolute Value      |     1     |
   +------+------------+--------+--------------------------+-----------+
   |  <   | 0x8807010E | [7].E  |        Less than         |     2     |
   +------+------------+--------+--------------------------+-----------+
   |  >   | 0x8807010F | [7].F  |       Greater than       |     2     |
   +------+------------+--------+--------------------------+-----------+
   |  <=  | 0x88070110 | [7].10 |  Less than or equal to   |     2     |
   +------+------------+--------+--------------------------+-----------+
   |  >=  | 0x88070111 | [7].11 | Greater than or equal to |     2     |
   +------+------------+--------+--------------------------+-----------+
   |  !=  | 0x88070112 | [7].12 |        Not equal         |     2     |
   +------+------------+--------+--------------------------+-----------+
   |  ==  | 0x88070113 | [7].13 |         Equal to         |     2     |
   +------+------------+--------+--------------------------+-----------+
   |  <<  | 0x88070114 | [7].14 |    Bitwise Left Shift    |     2     |
   +------+------------+--------+--------------------------+-----------+
   |  >>  | 0x88070115 | [7].15 |   Bitwise Right Shift    |     2     |
   +------+------------+--------+--------------------------+-----------+
   | STOR | 0x88070116 | [7].16 | Store value of Parm 2 in |     2     |
   |      |            |        |          Parm 1          |           |
   +------+------------+--------+--------------------------+-----------+
 */


#define ADM_AGENT_OP_PLUS_MID   "88070100"
#define ADM_AGENT_OP_MINUS_MID  "88070101"
#define ADM_AGENT_OP_MULT_MID   "88070102"
#define ADM_AGENT_OP_DIV_MID    "88070103"
#define ADM_AGENT_OP_MOD_MID    "88070104"
#define ADM_AGENT_OP_EXP_MID    "88070105"
#define ADM_AGENT_OP_BITAND_MID "88070106"
#define ADM_AGENT_OP_BITOR_MID  "88070107"
#define ADM_AGENT_OP_BITXOR_MID "88070108"
#define ADM_AGENT_OP_BITNOT_MID "88070109"
#define ADM_AGENT_OP_LOGAND_MID "8807010A"
#define ADM_AGENT_OP_LOGOR_MID  "8807010B"
#define ADM_AGENT_OP_LOGNOT_MID "8807010C"
#define ADM_AGENT_OP_ABS_MID    "8807010D"
#define ADM_AGENT_OP_LT_MID     "8807010E"
#define ADM_AGENT_OP_GT_MID     "8807010F"
#define ADM_AGENT_OP_LTE_MID    "88070110"
#define ADM_AGENT_OP_GTE_MID    "88070111"
#define ADM_AGENT_OP_NEQ_MID    "88070112"
#define ADM_AGENT_OP_EQ_MID     "88070113"
#define ADM_AGENT_OP_LSFHT_MID  "88070114"
#define ADM_AGENT_OP_RSHFT_MID  "88070115"
#define ADM_AGENT_OP_ASSGN_MID  "88070116"


void adm_agent_init();
void adm_agent_init_atomic();
void adm_agent_init_computed();
void adm_agent_init_controls();
void adm_agent_init_literals();
void adm_agent_init_macros();
void adm_agent_init_metadata();
void adm_agent_init_ops();
void adm_agent_init_reports();

/* Literal Functions */

value_t adm_agent_user_vast(mid_t *id);
value_t adm_agent_user_uvast(mid_t *id);
value_t adm_agent_user_float(mid_t *id);
value_t adm_agent_user_double(mid_t *id);
value_t adm_agent_user_string(mid_t *id);
value_t adm_agent_user_blob(mid_t *id);


uint32_t adm_agent_send_dc(eid_t *rx, mid_t *mid, Lyst dc);

#endif //ADM_AGENT_H_
