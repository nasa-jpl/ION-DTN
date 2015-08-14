/*****************************************************************************
 **
 ** File Name: adm_agent.h
 **
 ** Description: This implements the public portions of a DTNMP Agent ADM.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 *****************************************************************************/
#ifndef ADM_AGENT_H_
#define ADM_AGENT_H_

#include "lyst.h"

#include "shared/utils/nm_types.h"
#include "shared/adm/adm.h"
#include "shared/primitives/lit.h"



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
 * 0 -> 0x2B060102030300
 * 1 -> 0x2B060102030301
 * 2 -> 0x2B060102030302
 * 3 -> 0x2B060102030303
 * 4 -> 0x2B060102030304
 * 5 -> 0x2B060102030305
 * 6 -> 0x2B060102030306
 * 7 -> 0x2B060102030307
 * 8 -> 0x2B0601020303
 */

#define AGENT_ADM_MD_NN_IDX 0
#define AGENT_ADM_MD_NN_STR "0x2B060102030300"

#define AGENT_ADM_AD_NN_IDX 1
#define AGENT_ADM_AD_NN_STR "0x2B060102030301"

#define AGENT_ADM_CD_NN_IDX 2
#define AGENT_ADM_CD_NN_STR "0x2B060102030302"

#define AGENT_ADM_RPT_NN_IDX 3
#define AGENT_ADM_RPT_NN_STR "0x2B060102030303"

#define AGENT_ADM_CTRL_NN_IDX 4
#define AGENT_ADM_CTRL_NN_STR "0x2B060102030304"

#define AGENT_ADM_LTRL_NN_IDX 5
#define AGENT_ADM_LTRL_NN_STR "0x2B060102030305"

#define AGENT_ADM_MAC_NN_IDX 6
#define AGENT_ADM_MAC_NN_STR "0x2B060102030306"

#define AGENT_ADM_OP_NN_IDX 7
#define AGENT_ADM_OP_NN_STR "0x2B060102030307"

#define AGENT_ADM_ROOT_NN_IDX 8
#define AGENT_ADM_ROOT_NN_STR "0x2B0601020303"

/*
 * +--------------------------------------------------------------------------+
 * |					  AGENT META-DATA DEFINITIONS  						  +
 * +--------------------------------------------------------------------------+
   +------------------+----------+---------+----------------+----------+
   |       Name       |   MID    |   OID   |  Description   |   Type   |
   +------------------+----------+---------+----------------+----------+
   |     Name         | 80000100 |  [0].0  |   ADM Name     |   STR    |
   +------------------+----------+---------+----------------+----------+
   |     Version      | 80000101 |  [0].1  |  ADM Version   |   STR    |
   +------------------+----------+---------+----------------+----------+
 */

// "DTNMP AGENT ADM"
#define ADM_AGENT_MD_NAME_MID	"80000100"

// "2014_12_31"
#define ADM_AGENT_MD_VER_MID    "80000101"


/*
 * +--------------------------------------------------------------------------+
 * |					  AGENT ATOMIC DATA DEFINITIONS  					  +
 * +--------------------------------------------------------------------------+

   +------------------+----------+---------+----------------+----------+
   |       Name       |   MID    |   OID   |  Description   |   Type   |
   +------------------+----------+---------+----------------+----------+
   |  DefinedReports  | 80010100 | [1].0   |   # Reports    |   INT    |
   |                  |          |         |    Defined     |          |
   |                  |          |         |                |          |
   |   SentReports    | 80010101 | [1].1   | # Reports Sent |   INT    |
   |                  |          |         |                |          |
   |   NumTimeRules   | 80010102 | [1].2   | # Active TRL   |   INT    |
   |                  |          |         |                |          |
   |   RunTimeRules   | 80010103 | [1].3   |  # Run TRLs    |   INT    |
   |                  |          |         |                |          |
   |  NumStateRules   | 80010102 | [1].4   | # Active SRL   |   INT    |
   |                  |          |         |                |          |
   |   RunStateRules  | 80010103 | [1].5   |  # Run SRLs    |   INT    |
   |                  |          |         |                |          |
   |    NumConsts     | 80010104 | [1].6   |  # Constants   |   INT    |
   |                  |          |         |    Defined     |          |
   |                  |          |         |                |          |
   |    NumCustom     | 80010105 | [1].7   |   # Computed   |   INT    |
   |                  |          |         |      Data      |          |
   |                  |          |         |  Definitions   |          |
   |                  |          |         |                |          |
   |     NumMacros    | 80010106 | [1].8   |    # Macros    |   INT    |
   |                  |          |         |    Defined     |          |
   |                  |          |         |                |          |
   |    RunMacros     | 80010107 | [1].9   |  # Macros Run  |   INT    |
   |                  |          |         |                |          |
   |     NumCtrls     | 80010108 | [1].A   |   # Controls   |   INT    |
   |                  |          |         |    Defined     |          |
   |                  |          |         |                |          |
   |     RunCtrls     | 80010109 | [1].B   |   # Controls   |   INT    |
   |                  |          |         |    Defined     |          |
   +------------------+----------+---------+----------------+----------+

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

/*
 * +--------------------------------------------------------------------------+
 * |				    AGENT COMPUTED DATA DEFINITIONS 					  +
 * +--------------------------------------------------------------------------+

   +------------------+----------+---------+----------------+----------+
   |       Name       |   MID    |   OID   |  Description   |   Type   |
   +------------------+----------+---------+----------------+----------+
   |   # Rules        | 84020100 | [2].0   | # TRL + # SRL  |   INT    |
   +------------------+----------+---------+----------------+----------+
 */

#define ADM_AGENT_CD_NUMRULE_MID    "84020100"



/*
 * +--------------------------------------------------------------------------+
 * |				    	AGENT REPORT DEFINITIONS						  +
 * +--------------------------------------------------------------------------+

   +------------+----------+---------+------------------+--------------+
   |    Name    |   MID    |   OID   |   Description    |     Type     |
   +------------+----------+---------+------------------+--------------+
   | FullReport | 88030100 | [3].0   |  Report of all   |      DC      |
   |            |          |         |   atomic data    |              |
   |            |          |         |      items       |              |
   +------------+----------+---------+------------------+--------------+
 */

#define ADM_AGENT_RPT_FULL_MID  "88030100"



/*
 * +--------------------------------------------------------------------------+
 * |				    AGENT CONTROL DEFINITIONS CONSTANTS  				  +
 * +--------------------------------------------------------------------------+

   +----------------+-----------+----------+---------------------------+
   |      Name      |    MID    |   OID    |        Description        |
   +----------------+-----------+----------+---------------------------+
   |    ListADMs    |  81040100 |  [4].0   |   List all ADMs known to  |
   |                |           |          |         the agent.        |
   |                |           |          |                           |
   | DescAtomicData |  C1040101 |  [4].1   |    Dump information for   |
   |                |           |          |     given Atomic MIDs.    |
   |                |           |          |                           |
   |  AddCompData   |  C1040102 |  [4].2   |   Define a computed data  |
   |                |           |          |  definition on the agent. |
   |                |           |          |                           |
   |  DelCompData   |  C1040103 |  [4].3   |   Remove a computed data  |
   |                |           |          |    definition from the    |
   |                |           |          |           agent.          |
   |                |           |          |                           |
   |  ListCompData  |  81040104 |  [4].4   |  List known computed data |
   |                |           |          |           MIDs.           |
   |                |           |          |                           |
   |  DescCompData  |  C1040105 |  [4].5   |    Dump information for   |
   |                |           |          | given computed data MIDs. |
   |                |           |          |                           |
   |   AddRptDef    |  C1040106 |  [4].6   |  Define a custom report.  |
   |                |           |          |                           |
   |   DelRptDef    |  C1040107 |  [4].7   |  Forget a custom report.  |
   |                |           |          |                           |
   |    ListRpts    |  C1040108 |  [4].8   |  List known custom report |
   |                |           |          |           MIDs.           |
   |                |           |          |                           |
   |    DescRpts    |  C1040109 |  [4].9   |    Dump information for   |
   |                |           |          | given custom report MIDs. |
   |                |           |          |                           |
   |     GenRpt     |  C104010A |  [4].A   | Build and send reports.   |
   |                |           |          |                           |
   |  AddMacroDef   |  C104010B |  [4].B   |   Define a custom macro.  |
   |                |           |          |                           |
   |  DelMacroDef   |  C104010C |  [4].C   |   Forget a custom macro.  |
   |                |           |          |                           |
   |   ListMacros   |  8104010D |  [4].D   |  List known custom macro  |
   |                |           |          |           MIDs.           |
   |                |           |          |                           |
   |   DescMacros   |  C104010E |  [4].E   |    Dump information for   |
   |                |           |          |  given custom macro MIDs. |
   |                |           |          |                           |
   |  AddTimeRule   |  C104010F |  [4].F   |  Define a time-based prod |
   |                |           |          |           rule.           |
   |                |           |          |                           |
   |  DelTimeRule   |  C1040110 |  [4].10  |  Forget a time-based prod |
   |                |           |          |           rule.           |
   |                |           |          |                           |
   | ListTimeRules  |  81040111 |  [4].11  |   List known time-based   |
   |                |           |          |           rules.          |
   |                |           |          |                           |
   | DescTimeRules  |  C1040112 |  [4].12  |    Dump information for   |
   |                |           |          |  given time-based rules.  |
   |                |           |          |                           |
   |  AddStateRule  |  C1040113 |  [4].13  | Define a state-based prod |
   |                |           |          |           rule.           |
   |                |           |          |                           |
   |  DelStateRule  |  C1040114 |  [4].14  | Forget a state-based prod |
   |                |           |          |           rule.           |
   |                |           |          |                           |
   | ListStateRules |  81040115 |  [4].15  |   List known state-based  |
   |                |           |          |           rules.          |
   |                |           |          |                           |
   | DescStateRules |  C1040116 |  [4].16  |    Dump information for   |
   |                |           |          |  given state-based rules. |
   +----------------+-----------+----------+---------------------------+
 */

#define ADM_AGENT_CTL_LSTADM_MID    "81040100"
#define ADM_AGENT_CTL_DSCAD_MID     "C1040101"
#define ADM_AGENT_CTL_ADDCD_MID     "C1040102"
#define ADM_AGENT_CTL_DELCD_MID     "C1040103"
#define ADM_AGENT_CTL_LSTCD_MID     "81040104"
#define ADM_AGENT_CTL_DSCCD_MID     "C1040105"
#define ADM_AGENT_CTL_ADDRPT_MID    "C1040106"
#define ADM_AGENT_CTL_DELRPT_MID    "C1040107"
#define ADM_AGENT_CTL_LSTRPT_MID    "81040108"
#define ADM_AGENT_CTL_DSCRPT_MID    "C1040109"
#define ADM_AGENT_CTL_GENRPT_MID    "C104010A"
#define ADM_AGENT_CTL_ADDMAC_MID    "C104010B"
#define ADM_AGENT_CTL_DELMAC_MID    "C104010C"
#define ADM_AGENT_CTL_LSTMAC_MID    "8104010D"
#define ADM_AGENT_CTL_DSCMAC_MID    "C104010E"
#define ADM_AGENT_CTL_ADDTRL_MID    "C104010F"
#define ADM_AGENT_CTL_DELTRL_MID    "C1040110"
#define ADM_AGENT_CTL_LSTTRL_MID    "81040111"
#define ADM_AGENT_CTL_DSCTRL_MID    "C1040112"
#define ADM_AGENT_CTL_ADDSRL_MID    "C1040113"
#define ADM_AGENT_CTL_DELSRL_MID    "C1040114"
#define ADM_AGENT_CTL_LSTSRL_MID    "81040115"
#define ADM_AGENT_CTL_DSCSRL_MID    "C1040116"


/*
 * +--------------------------------------------------------------------------+
 * |					  AGENT LITERAL DEFINTIONS  						  +
 * +--------------------------------------------------------------------------+

   +----------------+-----------+----------+---------------------------------+
   |      Name      |    MID    |   OID    |           Description           |
   +----------------+-----------+----------+---------------------------------+
   |      Epoch     |  82050100 |  [5].0   |       DTNMP Time Epoch          |
   +----------------+-----------+----------+---------------------------------+
   |  User Integer  |  C2050101 |  [5].1   | User-defined signed integer.    |
   +----------------+-----------+----------+---------------------------------+
   |  User Integer  |  C2050102 |  [5].2   | User-defined unsigned integer.  |
   +----------------+-----------+----------+---------------------------------+
   |   User Float   |  C2050103 |  [5].3   | Arbitrary floating-point # in   |
   |                |           |          | IEEE754 format.                 |
   +----------------+-----------+----------+---------------------------------+
   |   User Double  |  C2050104 |  [5].4   | 64 bit version of float.        |
   +----------------+-----------+----------+---------------------------------+
   |   User String  |  C2050105 |  [5].5   | Arbitrary user-defined string.  |
   +----------------+-----------+----------+---------------------------------+
   |   User BLOB    |  C2050106 |  [5].6   | Arbitrary user-defined blob.    |
   +----------------+-----------+----------+---------------------------------+

 */

#define ADM_AGENT_LIT_EPOCH_MID    "82050100"
#define ADM_AGENT_LIT_USRINT_MID   "C2050101"
#define ADM_AGENT_LIT_USRUINT_MID  "C2050102"
#define ADM_AGENT_LIT_USRFLT_MID   "C2050103"
#define ADM_AGENT_LIT_USRDBL_MID   "C2050104"
#define ADM_AGENT_LIT_USRSTR_MID   "C2050105"
#define ADM_AGENT_LIT_USRBLOB_MID  "C2050106"


/*
 * +--------------------------------------------------------------------------+
 * |					  AGENT MACRO DEFINTIONS  						  +
 * +--------------------------------------------------------------------------+

   +----------------+-----------+----------+---------------------------+
   |      Name      |    MID    |   OID    |        Description        |
   +----------------+-----------+----------+---------------------------+
   |   Full List    |  C9060100 |  [6].0   |     List All ADM Data     |
   +----------------+-----------+----------+---------------------------+

 */

#define ADM_AGENT_MAC_FULL_MID    "C9060100"


/*
 * +--------------------------------------------------------------------------+
 * |				    	AGENT OPERATOR DEFINITIONS						  +
 * +--------------------------------------------------------------------------+

   +------------+-----------+----------+-------------------------------+
   |    Name    |    MID    |   OID    |          Description          |
   +------------+-----------+----------+-------------------------------+
   |     +      |  83070100 | [7].0    |            Addition           |
   |            |           |          |                               |
   |     -      |  83070101 | [7].1    |          Subtraction          |
   |            |           |          |                               |
   |     *      |  83070102 | [7].2    |         Multiplication        |
   |            |           |          |                               |
   |     /      |  83070103 | [7].3    |            Division           |
   |            |           |          |                               |
   |     %      |  83070104 | [7].4    |             Modulo            |
   |            |           |          |                               |
   |     ^      |  83070105 | [7].5    |         Exponentiation        |
   |            |           |          |                               |
   |     &      |  83070106 | [7].6    |          Bitwise AND          |
   |            |           |          |                               |
   |     |      |  83070107 | [7].7    |           Bitwise OR          |
   |            |           |          |                               |
   |     #      |  83070108 | [7].8    |          Bitwise XOR          |
   |            |           |          |                               |
   |     ~      |  83070109 | [7].9    |          Bitwise NOT          |
   |            |           |          |                               |
   |     &&     |  8307010A | [7].A    |          Logical AND          |
   |            |           |          |                               |
   |     ||     |  8307010B | [7].B    |           Logical OR          |
   |            |           |          |                               |
   |     !      |  8307010C | [7].C    |          Logical NOT          |
   |            |           |          |                               |
   |    abs     |  8307010D | [7].D    |         Absolute Value        |
   |            |           |          |                               |
   |     <      |  8307010E | [7].E    |           Less than           |
   |            |           |          |                               |
   |     >      |  8307010F | [7].F    |          Greater than         |
   |            |           |          |                               |
   |     <=     |  83070110 | [7].10   |     Less than or equal to     |
   |            |           |          |                               |
   |     >=     |  83070111 | [7].11   |    Greater than or equal to   |
   |            |           |          |                               |
   |     !=     |  83070112 | [7].12   |           Not equal           |
   |            |           |          |                               |
   |     ==     |  83070113 | [7].13   |            Equal to           |
   +------------+-----------+----------+-------------------------------+

 */


#define ADM_AGENT_OP_PLUS_MID   "83070100"
#define ADM_AGENT_OP_MINUS_MID  "83070101"
#define ADM_AGENT_OP_MULT_MID   "83070102"
#define ADM_AGENT_OP_DIV_MID    "83070103"
#define ADM_AGENT_OP_MOD_MID    "83070104"
#define ADM_AGENT_OP_EXP_MID    "83070105"
#define ADM_AGENT_OP_BITAND_MID "83070106"
#define ADM_AGENT_OP_BITOR_MID  "83070107"
#define ADM_AGENT_OP_BITXOR_MID "83070108"
#define ADM_AGENT_OP_BITNOT_MID "83070109"
#define ADM_AGENT_OP_LOGAND_MID "8307010A"
#define ADM_AGENT_OP_LOGOR_MID  "8307010B"
#define ADM_AGENT_OP_LOGNOT_MID "8307010C"
#define ADM_AGENT_OP_ABS_MID    "8307010D"
#define ADM_AGENT_OP_LT_MID     "8307010E"
#define ADM_AGENT_OP_GT_MID     "8307010F"
#define ADM_AGENT_OP_LTE_MID    "83070110"
#define ADM_AGENT_OP_GTE_MID    "83070111"
#define ADM_AGENT_OP_NEQ_MID    "83070112"
#define ADM_AGENT_OP_EQ_MID     "83070113"

/*
 * \todo add bitshifts << and >>
 */

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

value_t adm_agent_user_int(mid_t *id);
value_t adm_agent_user_uint(mid_t *id);
value_t adm_agent_user_float(mid_t *id);
value_t adm_agent_user_double(mid_t *id);
value_t adm_agent_user_string(mid_t *id);
value_t adm_agent_user_blob(mid_t *id);


uint32_t adm_agent_send_dc(eid_t *rx, mid_t *mid, Lyst dc);

#endif //ADM_AGENT_H_
