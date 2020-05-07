/****************************************************************************
 **
 ** File Name: adm_amp_agent.h
 **
 ** Description: TODO
 **
 ** Notes: TODO
 **
 ** Assumptions: TODO
 **
 ** Modification History: 
 **  YYYY-MM-DD  AUTHOR           DESCRIPTION
 **  ----------  --------------   --------------------------------------------
 **  2020-04-16  AUTO             Auto-generated header file 
 **
 ****************************************************************************/


#ifndef ADM_AMP_AGENT_H_
#define ADM_AMP_AGENT_H_
#define _HAVE_AMP_AGENT_ADM_
#ifdef _HAVE_AMP_AGENT_ADM_

#include "shared/utils/nm_types.h"
#include "shared/adm/adm.h"

extern vec_idx_t g_amp_agent_idx[11];

/*
 * +---------------------------------------------------------------------------------------------+
 * |                                 ADM TEMPLATE DOCUMENTATION                                  +
 * +---------------------------------------------------------------------------------------------+
 *
 * ADM ROOT STRING:Amp/Agent
 */
#define ADM_ENUM_AMP_AGENT 1
/*
 * +---------------------------------------------------------------------------------------------+
 * |                                 AGENT NICKNAME DEFINITIONS                                  +
 * +---------------------------------------------------------------------------------------------+
 */

/*
 * +---------------------------------------------------------------------------------------------+
 * |                               AMP_AGENT META-DATA DEFINITIONS                               +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |         VALUE          |
 * +---------------------+--------------------------------------+-------+------------------------+
 * |name                 |The human-readable name of the ADM.   |STR    |amp_agent               |
 * +---------------------+--------------------------------------+-------+------------------------+
 * |namespace            |The namespace of the ADM.             |STR    |Amp/Agent               |
 * +---------------------+--------------------------------------+-------+------------------------+
 * |version              |The version of the ADM.               |STR    |v3.1                    |
 * +---------------------+--------------------------------------+-------+------------------------+
 * |organization         |The name of the issuing organization o|       |                        |
 * |                     |f the ADM.                            |STR    |JHUAPL                  |
 * +---------------------+--------------------------------------+-------+------------------------+
 */
// "name"
#define AMP_AGENT_META_NAME 0x00
// "namespace"
#define AMP_AGENT_META_NAMESPACE 0x01
// "version"
#define AMP_AGENT_META_VERSION 0x02
// "organization"
#define AMP_AGENT_META_ORGANIZATION 0x03


/*
 * +---------------------------------------------------------------------------------------------+
 * |                        AMP_AGENT EXTERNALLY DEFINED DATA DEFINITIONS                        +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 * |num_rpt_tpls         |This is the number of report templates|       |
 * |                     | known to the Agent.                  |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_tbl_tpls         |This is the number of table templates |       |
 * |                     |known to the Agent.                   |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |sent_reports         |This is the number of reports sent by |       |
 * |                     |the agent.                            |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_tbr              |This is the number of time-based rules|       |
 * |                     | running on the agent.                |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |run_tbr              |This is the number of time-based rules|       |
 * |                     | run by the agent since the last reset|       |
 * |                     |.                                     |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_sbr              |This is the number of state-based rule|       |
 * |                     |s running on the agent.               |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |run_sbr              |This is the number of state-based rule|       |
 * |                     |s run by the agent since the last rese|       |
 * |                     |t.                                    |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_const            |This is the number of constants known |       |
 * |                     |by the agent.                         |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_var              |This is the number of variables known |       |
 * |                     |by the agent.                         |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_macros           |This is the number of macros known by |       |
 * |                     |the agent.                            |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |run_macros           |This is the number of macros run by th|       |
 * |                     |e agent since the last reset.         |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_controls         |This is the number of controls known b|       |
 * |                     |y the agent.                          |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |run_controls         |This is the number of controls run by |       |
 * |                     |the agent since the last reset.       |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |cur_time             |This is the current system time.      |TV     |
 * +---------------------+--------------------------------------+-------+
 */
#define AMP_AGENT_EDD_NUM_RPT_TPLS 0x00
#define AMP_AGENT_EDD_NUM_TBL_TPLS 0x01
#define AMP_AGENT_EDD_SENT_REPORTS 0x02
#define AMP_AGENT_EDD_NUM_TBR 0x03
#define AMP_AGENT_EDD_RUN_TBR 0x04
#define AMP_AGENT_EDD_NUM_SBR 0x05
#define AMP_AGENT_EDD_RUN_SBR 0x06
#define AMP_AGENT_EDD_NUM_CONST 0x07
#define AMP_AGENT_EDD_NUM_VAR 0x08
#define AMP_AGENT_EDD_NUM_MACROS 0x09
#define AMP_AGENT_EDD_RUN_MACROS 0x0a
#define AMP_AGENT_EDD_NUM_CONTROLS 0x0b
#define AMP_AGENT_EDD_RUN_CONTROLS 0x0c
#define AMP_AGENT_EDD_CUR_TIME 0x0d


/*
 * +---------------------------------------------------------------------------------------------+
 * |                               AMP_AGENT VARIABLE DEFINITIONS                                +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 * |num_rules            |This is the number of rules known to t|       |
 * |                     |he Agent (#TBR + #SBR).               |UINT   |
 * +---------------------+--------------------------------------+-------+
 */
#define AMP_AGENT_VAR_NUM_RULES 0x00


/*
 * +---------------------------------------------------------------------------------------------+
 * |                                AMP_AGENT REPORT DEFINITIONS                                 +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 * |full_report          |This is all known meta-data, EDD, and |       |
 * |                     |VAR values known by the agent.        |TNVC   |
 * +---------------------+--------------------------------------+-------+
 */
#define AMP_AGENT_RPTTPL_FULL_REPORT 0x00


/*
 * +---------------------------------------------------------------------------------------------+
 * |                                 AMP_AGENT TABLE DEFINITIONS                                 +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 * |adms                 |This table lists all the adms that are|       |
 * |                     | supported by the agent.              |       |
 * +---------------------+--------------------------------------+-------+
 * |variables            |This table lists the ARI for every var|       |
 * |                     |iable that is known to the agent.     |       |
 * +---------------------+--------------------------------------+-------+
 * |rptts                |This table lists the ARI for every rep|       |
 * |                     |ort template that is known to the agen|       |
 * |                     |t.                                    |       |
 * +---------------------+--------------------------------------+-------+
 * |macros               |This table lists the ARI for every mac|       |
 * |                     |ro that is known to the agent.        |       |
 * +---------------------+--------------------------------------+-------+
 * |rules                |This table lists the ARI for every rul|       |
 * |                     |e that is known to the agent.         |       |
 * +---------------------+--------------------------------------+-------+
 * |tblts                |This table lists the ARI for every tab|       |
 * |                     |le template that is known to the agent|       |
 * |                     |.                                     |       |
 * +---------------------+--------------------------------------+-------+
 */
#define AMP_AGENT_TBLT_ADMS 0x00
#define AMP_AGENT_TBLT_VARIABLES 0x01
#define AMP_AGENT_TBLT_RPTTS 0x02
#define AMP_AGENT_TBLT_MACROS 0x03
#define AMP_AGENT_TBLT_RULES 0x04
#define AMP_AGENT_TBLT_TBLTS 0x05


/*
 * +---------------------------------------------------------------------------------------------+
 * |                                AMP_AGENT CONTROL DEFINITIONS                                +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 * |add_var              |This control configures a new variable|       |
 * |                     | definition on the Agent.             |       |
 * +---------------------+--------------------------------------+-------+
 * |del_var              |This control removes one or more varia|       |
 * |                     |ble definitions from the Agent.       |       |
 * +---------------------+--------------------------------------+-------+
 * |add_rptt             |This control configures a new report t|       |
 * |                     |emplate definition on the Agent.      |       |
 * +---------------------+--------------------------------------+-------+
 * |del_rptt             |This control removes one or more repor|       |
 * |                     |t template definitions from the Agent.|       |
 * +---------------------+--------------------------------------+-------+
 * |desc_rptt            |This control produces a detailed descr|       |
 * |                     |iption of one or more report template |       |
 * |                     | identifier(ARI) known to the Agent.  |       |
 * +---------------------+--------------------------------------+-------+
 * |gen_rpts             |This control causes the Agent to produ|       |
 * |                     |ce a report entry for each identified |       |
 * |                     |report templates and send them to one |       |
 * |                     |or more identified managers(ARIs).    |       |
 * +---------------------+--------------------------------------+-------+
 * |gen_tbls             |This control causes the Agent to produ|       |
 * |                     |ce a table for each identified table t|       |
 * |                     |emplates and send them to one or more |       |
 * |                     |identified managers(ARIs).            |       |
 * +---------------------+--------------------------------------+-------+
 * |add_macro            |This control configures a new macro de|       |
 * |                     |finition on the Agent.                |       |
 * +---------------------+--------------------------------------+-------+
 * |del_macro            |This control removes one or more macro|       |
 * |                     | definitions from the Agent.          |       |
 * +---------------------+--------------------------------------+-------+
 * |desc_macro           |This control produces a detailed descr|       |
 * |                     |iption of one or more macro identifier|       |
 * |                     |(ARI) known to the Agent.             |       |
 * +---------------------+--------------------------------------+-------+
 * |add_tbr              |This control configures a new time-bas|       |
 * |                     |ed rule(TBR) definition on the Agent. |       |
 * +---------------------+--------------------------------------+-------+
 * |add_sbr              |This control configures a new state-ba|       |
 * |                     |sed rule(SBR) definition on the Agent.|       |
 * +---------------------+--------------------------------------+-------+
 * |del_rule             |This control removes one or more rule |       |
 * |                     |definitions from the Agent.           |       |
 * +---------------------+--------------------------------------+-------+
 * |desc_rule            |This control produces a detailed descr|       |
 * |                     |iption of one or more rules known to t|       |
 * |                     |he Agent.                             |       |
 * +---------------------+--------------------------------------+-------+
 * |store_var            |This control stores variables.        |       |
 * +---------------------+--------------------------------------+-------+
 * |reset_counts         |This control resets all Agent ADM stat|       |
 * |                     |istics reported in the Agent ADM repor|       |
 * |                     |t.                                    |       |
 * +---------------------+--------------------------------------+-------+
 */
#define AMP_AGENT_CTRL_ADD_VAR 0x00
#define AMP_AGENT_CTRL_DEL_VAR 0x01
#define AMP_AGENT_CTRL_ADD_RPTT 0x02
#define AMP_AGENT_CTRL_DEL_RPTT 0x03
#define AMP_AGENT_CTRL_DESC_RPTT 0x04
#define AMP_AGENT_CTRL_GEN_RPTS 0x05
#define AMP_AGENT_CTRL_GEN_TBLS 0x06
#define AMP_AGENT_CTRL_ADD_MACRO 0x07
#define AMP_AGENT_CTRL_DEL_MACRO 0x08
#define AMP_AGENT_CTRL_DESC_MACRO 0x09
#define AMP_AGENT_CTRL_ADD_TBR 0x0a
#define AMP_AGENT_CTRL_ADD_SBR 0x0b
#define AMP_AGENT_CTRL_DEL_RULE 0x0c
#define AMP_AGENT_CTRL_DESC_RULE 0x0d
#define AMP_AGENT_CTRL_STORE_VAR 0x0e
#define AMP_AGENT_CTRL_RESET_COUNTS 0x0f


/*
 * +---------------------------------------------------------------------------------------------+
 * |                               AMP_AGENT CONSTANT DEFINITIONS                                +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |         VALUE          |
 * +---------------------+--------------------------------------+-------+------------------------+
 * |amp_epoch            |This constant is the time epoch for th|       |                        |
 * |                     |e Agent.                              |TS     |1504915200              |
 * +---------------------+--------------------------------------+-------+------------------------+
 */
#define AMP_AGENT_CNST_AMP_EPOCH 0x00


/*
 * +---------------------------------------------------------------------------------------------+
 * |                                 AMP_AGENT MACRO DEFINITIONS                                 +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 */


/*
 * +---------------------------------------------------------------------------------------------+
 * |                               AMP_AGENT OPERATOR DEFINITIONS                                +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 * |plusINT              |Int32 addition                        |INT    |
 * +---------------------+--------------------------------------+-------+
 * |plusUINT             |Unsigned Int32 addition               |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |plusVAST             |Int64 addition                        |VAST   |
 * +---------------------+--------------------------------------+-------+
 * |plusUVAST            |Unsigned Int64 addition               |UVAST  |
 * +---------------------+--------------------------------------+-------+
 * |plusREAL32           |Real32 addition                       |REAL32 |
 * +---------------------+--------------------------------------+-------+
 * |plusREAL64           |Real64 addition                       |REAL64 |
 * +---------------------+--------------------------------------+-------+
 * |minusINT             |Int32 subtraction                     |INT    |
 * +---------------------+--------------------------------------+-------+
 * |minusUINT            |Unsigned Int32 subtraction            |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |minusVAST            |Int64 subtraction                     |VAST   |
 * +---------------------+--------------------------------------+-------+
 * |minusUVAST           |Unsigned Int64 subtraction            |UVAST  |
 * +---------------------+--------------------------------------+-------+
 * |minusREAL32          |Real32 subtraction                    |REAL32 |
 * +---------------------+--------------------------------------+-------+
 * |minusREAL64          |Real64 subtraction                    |REAL64 |
 * +---------------------+--------------------------------------+-------+
 * |multINT              |Int32 multiplication                  |INT    |
 * +---------------------+--------------------------------------+-------+
 * |multUINT             |Unsigned Int32 multiplication         |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |multVAST             |Int64 multiplication                  |VAST   |
 * +---------------------+--------------------------------------+-------+
 * |multUVAST            |Unsigned Int64 multiplication         |UVAST  |
 * +---------------------+--------------------------------------+-------+
 * |multREAL32           |Real32 multiplication                 |REAL32 |
 * +---------------------+--------------------------------------+-------+
 * |multREAL64           |Real64 multiplication                 |REAL64 |
 * +---------------------+--------------------------------------+-------+
 * |divINT               |Int32 division                        |INT    |
 * +---------------------+--------------------------------------+-------+
 * |divUINT              |Unsigned Int32 division               |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |divVAST              |Int64 division                        |VAST   |
 * +---------------------+--------------------------------------+-------+
 * |divUVAST             |Unsigned Int64 division               |UVAST  |
 * +---------------------+--------------------------------------+-------+
 * |divREAL32            |Real32 division                       |REAL32 |
 * +---------------------+--------------------------------------+-------+
 * |divREAL64            |Real64 division                       |REAL64 |
 * +---------------------+--------------------------------------+-------+
 * |modINT               |Int32 modulus division                |INT    |
 * +---------------------+--------------------------------------+-------+
 * |modUINT              |Unsigned Int32 modulus division       |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |modVAST              |Int64 modulus division                |VAST   |
 * +---------------------+--------------------------------------+-------+
 * |modUVAST             |Unsigned Int64 modulus division       |UVAST  |
 * +---------------------+--------------------------------------+-------+
 * |modREAL32            |Real32 modulus division               |REAL32 |
 * +---------------------+--------------------------------------+-------+
 * |modREAL64            |Real64 modulus division               |REAL64 |
 * +---------------------+--------------------------------------+-------+
 * |expINT               |Int32 exponentiation                  |INT    |
 * +---------------------+--------------------------------------+-------+
 * |expUINT              |Unsigned int32 exponentiation         |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |expVAST              |Int64 exponentiation                  |VAST   |
 * +---------------------+--------------------------------------+-------+
 * |expUVAST             |Unsigned Int64 exponentiation         |UVAST  |
 * +---------------------+--------------------------------------+-------+
 * |expREAL32            |Real32 exponentiation                 |REAL32 |
 * +---------------------+--------------------------------------+-------+
 * |expREAL64            |Real64 exponentiation                 |REAL64 |
 * +---------------------+--------------------------------------+-------+
 * |bitAND               |Bitwise and                           |UVAST  |
 * +---------------------+--------------------------------------+-------+
 * |bitOR                |Bitwise or                            |UVAST  |
 * +---------------------+--------------------------------------+-------+
 * |bitXOR               |Bitwise xor                           |UVAST  |
 * +---------------------+--------------------------------------+-------+
 * |bitNOT               |Bitwise not                           |UVAST  |
 * +---------------------+--------------------------------------+-------+
 * |logAND               |Logical and                           |BOOL   |
 * +---------------------+--------------------------------------+-------+
 * |logOR                |Logical or                            |BOOL   |
 * +---------------------+--------------------------------------+-------+
 * |logNOT               |Logical not                           |BOOL   |
 * +---------------------+--------------------------------------+-------+
 * |abs                  |absolute value                        |UVAST  |
 * +---------------------+--------------------------------------+-------+
 * |lessThan             |<                                     |BOOL   |
 * +---------------------+--------------------------------------+-------+
 * |greaterThan          |>                                     |BOOL   |
 * +---------------------+--------------------------------------+-------+
 * |lessEqual            |<=                                    |BOOL   |
 * +---------------------+--------------------------------------+-------+
 * |greaterEqual         |>=                                    |BOOL   |
 * +---------------------+--------------------------------------+-------+
 * |notEqual             |!=                                    |BOOL   |
 * +---------------------+--------------------------------------+-------+
 * |Equal                |==                                    |BOOL   |
 * +---------------------+--------------------------------------+-------+
 * |bitShiftLeft         |<<                                    |UVAST  |
 * +---------------------+--------------------------------------+-------+
 * |bitShiftRight        |>>                                    |UVAST  |
 * +---------------------+--------------------------------------+-------+
 * |STOR                 |Store value of parm 2 in parm 1       |UNK    |
 * +---------------------+--------------------------------------+-------+
 */
#define AMP_AGENT_OP_PLUSINT 0x00
#define AMP_AGENT_OP_PLUSUINT 0x01
#define AMP_AGENT_OP_PLUSVAST 0x02
#define AMP_AGENT_OP_PLUSUVAST 0x03
#define AMP_AGENT_OP_PLUSREAL32 0x04
#define AMP_AGENT_OP_PLUSREAL64 0x05
#define AMP_AGENT_OP_MINUSINT 0x06
#define AMP_AGENT_OP_MINUSUINT 0x07
#define AMP_AGENT_OP_MINUSVAST 0x08
#define AMP_AGENT_OP_MINUSUVAST 0x09
#define AMP_AGENT_OP_MINUSREAL32 0x0a
#define AMP_AGENT_OP_MINUSREAL64 0x0b
#define AMP_AGENT_OP_MULTINT 0x0c
#define AMP_AGENT_OP_MULTUINT 0x0d
#define AMP_AGENT_OP_MULTVAST 0x0e
#define AMP_AGENT_OP_MULTUVAST 0x0f
#define AMP_AGENT_OP_MULTREAL32 0x10
#define AMP_AGENT_OP_MULTREAL64 0x11
#define AMP_AGENT_OP_DIVINT 0x12
#define AMP_AGENT_OP_DIVUINT 0x13
#define AMP_AGENT_OP_DIVVAST 0x14
#define AMP_AGENT_OP_DIVUVAST 0x15
#define AMP_AGENT_OP_DIVREAL32 0x16
#define AMP_AGENT_OP_DIVREAL64 0x17
#define AMP_AGENT_OP_MODINT 0x18
#define AMP_AGENT_OP_MODUINT 0x19
#define AMP_AGENT_OP_MODVAST 0x1a
#define AMP_AGENT_OP_MODUVAST 0x1b
#define AMP_AGENT_OP_MODREAL32 0x1c
#define AMP_AGENT_OP_MODREAL64 0x1d
#define AMP_AGENT_OP_EXPINT 0x1e
#define AMP_AGENT_OP_EXPUINT 0x1f
#define AMP_AGENT_OP_EXPVAST 0x20
#define AMP_AGENT_OP_EXPUVAST 0x21
#define AMP_AGENT_OP_EXPREAL32 0x22
#define AMP_AGENT_OP_EXPREAL64 0x23
#define AMP_AGENT_OP_BITAND 0x24
#define AMP_AGENT_OP_BITOR 0x25
#define AMP_AGENT_OP_BITXOR 0x26
#define AMP_AGENT_OP_BITNOT 0x27
#define AMP_AGENT_OP_LOGAND 0x28
#define AMP_AGENT_OP_LOGOR 0x29
#define AMP_AGENT_OP_LOGNOT 0x2a
#define AMP_AGENT_OP_ABS 0x2b
#define AMP_AGENT_OP_LESSTHAN 0x2c
#define AMP_AGENT_OP_GREATERTHAN 0x2d
#define AMP_AGENT_OP_LESSEQUAL 0x2e
#define AMP_AGENT_OP_GREATEREQUAL 0x2f
#define AMP_AGENT_OP_NOTEQUAL 0x30
#define AMP_AGENT_OP_EQUAL 0x31
#define AMP_AGENT_OP_BITSHIFTLEFT 0x32
#define AMP_AGENT_OP_BITSHIFTRIGHT 0x33
#define AMP_AGENT_OP_STOR 0x34

/* Initialization functions. */
void amp_agent_init();
void amp_agent_init_meta();
void amp_agent_init_cnst();
void amp_agent_init_edd();
void amp_agent_init_op();
void amp_agent_init_var();
void amp_agent_init_ctrl();
void amp_agent_init_mac();
void amp_agent_init_rpttpl();
void amp_agent_init_tblt();
#endif /* _HAVE_AMP_AGENT_ADM_ */
#endif //ADM_AMP_AGENT_H_
