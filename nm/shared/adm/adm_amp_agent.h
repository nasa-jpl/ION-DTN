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
 **  2018-10-16  AUTO             Auto-generated header file 
 **
 ****************************************************************************/


#ifndef ADM_AMP_AGENT_H_
#define ADM_AMP_AGENT_H_
#define _HAVE_AMP_AGENT_ADM_
#ifdef _HAVE_AMP_AGENT_ADM_

#include "../utils/nm_types.h"
#include "adm.h"


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |                                        ADM TEMPLATE DOCUMENTATION                                        +
 * +-----------------------------------------------------------------------------------------------------------+
 *
 * ADM ROOT STRING:Amp/Agent
 */

#define ADM_ENUM_AMP_AGENT 1

/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |                                        AGENT NICKNAME DEFINITIONS                                        +
 * +-----------------------------------------------------------------------------------------------------------+
 */

/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |                                      AMP_AGENT META-DATA DEFINITIONS                                      +
 * +-----------------------------------------------------------------------------------------------------------+
 * |            NAME             |    mid     |                   DESCRIPTION                    |    TYPE     |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |name                         |80000100    |The human-readable name of the ADM.               |STR          |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |namespace                    |80000101    |The namespace of the ADM.                         |STR          |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |version                      |80000102    |The version of the ADM.                           |STR          |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |organization                 |80000103    |The name of the issuing organization of the ADM.  |STR          |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 */
#define AMP_AGENT_NAME         0x00
#define AMP_AGENT_NAMESPACE    0x01
#define AMP_AGENT_VERSION      0x02
#define AMP_AGENT_ORGANIZATION 0x03


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |                               AMP_AGENT EXTERNALLY DEFINED DATA DEFINITIONS                               +
 * +-----------------------------------------------------------------------------------------------------------+
 * |            NAME             |    mid     |                   DESCRIPTION                    |    TYPE     |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |num_rpt_tpls                 |80010100    |This is the number of report templates known to th|             |
 * |                             |            |e Agent.                                          |UINT         |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |num_tbl_tpls                 |80010101    |This is the number of table templates known to the|             |
 * |                             |            | Agent.                                           |UINT         |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |sent_reports                 |80010102    |This is the number of reports sent by the agent.  |UINT         |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |num_tbr                      |80010103    |This is the number of time-based rules running on |             |
 * |                             |            |the agent.                                        |UINT         |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |run_tbr                      |80010104    |This is the number of time-based rules run by the |             |
 * |                             |            |agent since the last reset.                       |UINT         |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |num_sbr                      |80010105    |This is the number of state-based rules running on|             |
 * |                             |            | the agent.                                       |UINT         |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |run_sbr                      |80010106    |This is the number of state-based rules run by the|             |
 * |                             |            | agent since the last reset.                      |UINT         |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |num_const                    |80010107    |This is the number of constants known by the agent|             |
 * |                             |            |.                                                 |UINT         |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |num_var                      |80010108    |This is the number of variables known by the agent|             |
 * |                             |            |.                                                 |UINT         |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |num_macros                   |80010109    |This is the number of macros known by the agent.  |UINT         |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |run_macros                   |8001010a    |This is the number of macros run by the agent sinc|             |
 * |                             |            |e the last reset.                                 |UINT         |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |num_controls                 |8001010b    |This is the number of controls known by the agent.|UINT         |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |run_controls                 |8001010c    |This is the number of controls run by the agent si|             |
 * |                             |            |nce the last reset.                               |UINT         |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |cur_time                     |8001010d    |This is the current system time.                  |TV           |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 */
#define AMP_AGENT_NUM_RPT_TPLS 0x00
#define AMP_AGENT_NUM_TBL_TPLS 0x01
#define AMP_AGENT_SENT_REPORTS 0x02
#define AMP_AGENT_NUM_TBR      0x03
#define AMP_AGENT_RUN_TBR      0x04
#define AMP_AGENT_NUM_SBR      0x05
#define AMP_AGENT_RUN_SBR      0x06
#define AMP_AGENT_NUM_CONST    0x07
#define AMP_AGENT_NUM_VAR      0x08
#define AMP_AGENT_NUM_MACROS   0x09
#define AMP_AGENT_RUN_MACROS   0x0A
#define AMP_AGENT_NUM_CONTROLS 0x0B
#define AMP_AGENT_RUN_CONTROLS 0x0C
#define AMP_AGENT_CUR_TIME     0x0D


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |                                      AMP_AGENT VARIABLE DEFINITIONS                                      +
 * +-----------------------------------------------------------------------------------------------------------+
 * |            NAME             |    mid     |                   DESCRIPTION                    |    TYPE     |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |num_rules                    |81020100    |This is the number of rules known to the Agent (#T|             |
 * |                             |            |BR + #SBR).                                       |UINT         |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 */
#define AMP_AGENT_NUM_RULES 0x00


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |                                       AMP_AGENT REPORT DEFINITIONS                                       +
 * +-----------------------------------------------------------------------------------------------------------+
 * |            NAME             |    mid     |                   DESCRIPTION                    |    TYPE     |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |full_report                  |82030100    |This is all known meta-data, EDD, and VAR values k|             |
 * |                             |            |nown by the agent.                                |?            |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 */
#define AMP_AGENT_FULL_REPORT 0x00


/* Table Definitions .*/

#define AMP_AGENT_TBLT_ADM 0x00
#define AMP_AGENT_TBLT_VARIABLE 0x01
#define AMP_AGENT_TBLT_RPTT 0x02
#define AMP_AGENT_TBLT_MACRO 0x03
#define AMP_AGENT_TBLT_RULE 0x04
#define AMP_AGENT_TBLT_TBLT 0x05

/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |                                       AMP_AGENT CONTROL DEFINITIONS                                       +
 * +-----------------------------------------------------------------------------------------------------------+
 * |            NAME             |    mid     |                   DESCRIPTION                    |    TYPE     |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |add_var                      |c3040100    |This control configures a new variable definition |             |
 * |                             |            |on the Agent.                                     |             |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |del_var                      |c3040101    |This control removes one or more variable definiti|             |
 * |                             |            |ons from the Agent.                               |             |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |desc_var                     |c3040102    |This control produces a detailed description of on|             |
 * |                             |            |e or more variable identifier(ARI)s known to the A|             |
 * |                             |            |gent.                                             |             |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |add_rptt                     |c3040103    |This control configures a new report template defi|             |
 * |                             |            |nition on the Agent.                              |             |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |del_rptt                     |c3040104    |This control removes one or more report template d|             |
 * |                             |            |efinitions from the Agent.                        |             |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |desc_rptt                    |c3040105    |This control produces a detailed description of on|             |
 * |                             |            |e or more report template  identifier(ARI) known t|             |
 * |                             |            |o the Agent.                                      |             |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |gen_rpts                     |c3040106    |This control causes the Agent to produce a report |             |
 * |                             |            |entry for each identified report templates and sen|             |
 * |                             |            |d them to one or more identified managers(ARIs).  |             |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |gen_tbls                     |c3040107    |This control generates tables for each identified |             |
 * |                             |            |table template.                                   |             |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |add_macro                    |c3040108    |This control configures a new macro definition on |             |
 * |                             |            |the Agent.                                        |             |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |del_macro                    |c3040109    |This control removes one or more macro definitions|             |
 * |                             |            | from the Agent.                                  |             |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |desc_macro                   |c304010a    |This control produces a detailed description of on|             |
 * |                             |            |e or more macro identifier(ARI) known to the Agent|             |
 * |                             |            |.                                                 |             |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |add_tbr                      |c304010b    |This control configures a new time-based rule(TBR)|             |
 * |                             |            | definition on the Agent.                         |             |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |del_tbr                      |c304010c    |This control removes one or more TBR definitions f|             |
 * |                             |            |rom the Agent.                                    |             |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |desc_tbr                     |c304010d    |This control produces a detailed description of on|             |
 * |                             |            |e or more TRL identifier(ARI)s known to the Agent.|             |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |add_sbr                      |c304010e    |This control configures a new state-based rule(SBR|             |
 * |                             |            |) definition on the Agent.                        |             |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |del_sbr                      |c304010f    |This control removes one or more SBR definitions f|             |
 * |                             |            |rom the Agent.                                    |             |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |desc_sbr                     |c3040110    |This control produces a detailed description of on|             |
 * |                             |            |e or more SBR identifier(ARI)s known to the Agent.|             |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |store_var                    |c3040111    |This control stores variables.                    |             |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |reset_counts                 |83040112    |This control resets all Agent ADM statistics repor|             |
 * |                             |            |ted in the Agent ADM report.                      |             |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 */
#define AMP_AGENT_ADD_VAR      0x00
#define AMP_AGENT_DEL_VAR      0x01
#define AMP_AGENT_ADD_RPTT     0x03
#define AMP_AGENT_DEL_RPTT     0x04
#define AMP_AGENT_DESC_RPTT    0x05
#define AMP_AGENT_GEN_RPTS     0x06
#define AMP_AGENT_GEN_TBLS     0x07
#define AMP_AGENT_ADD_MACRO    0x08
#define AMP_AGENT_DEL_MACRO    0x09
#define AMP_AGENT_DESC_MACRO   0x0A
#define AMP_AGENT_ADD_TBR      0x0B
#define AMP_AGENT_DEL_TBR      0x0C
#define AMP_AGENT_DESC_TBR     0x0D
#define AMP_AGENT_ADD_SBR      0x0E
#define AMP_AGENT_DEL_SBR      0x0F
#define AMP_AGENT_DESC_SBR     0x10
#define AMP_AGENT_STORE_VAR    0x11
#define AMP_AGENT_RESET_COUNTS 0x12


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |                                      AMP_AGENT CONSTANT DEFINITIONS                                      +
 * +-----------------------------------------------------------------------------------------------------------+
 * |            NAME             |    mid     |                   DESCRIPTION                    |    TYPE     |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |amp_epoch                    |87050100    |This constant is the time epoch for the Agent.    |TS           |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 */
#define AMP_AGENT_AMP_EPOCH 0x00


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |                                        AMP_AGENT MACRO DEFINITIONS                                        +
 * +-----------------------------------------------------------------------------------------------------------+
 * |            NAME             |    mid     |                   DESCRIPTION                    |    TYPE     |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |user_desc                    |86060100    |This macro lists all of the user defined data.    |mc           |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 */
#define AMP_AGENT_USER_DESC 0x00


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |                                      AMP_AGENT OPERATOR DEFINITIONS                                      +
 * +-----------------------------------------------------------------------------------------------------------+
 * |            NAME             |    mid     |                   DESCRIPTION                    |    TYPE     |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |plusINT                      |88070100    |Int32 addition                                    |INT          |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |plusUINT                     |88070101    |Unsigned Int32 addition                           |UINT         |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |plusVAST                     |88070102    |Int64 addition                                    |VAST         |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |plusUVAST                    |88070103    |Unsigned Int64 addition                           |UVAST        |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |plusREAL32                   |88070104    |Real32 addition                                   |REAL32       |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |plusREAL64                   |88070105    |Real64 addition                                   |REAL64       |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |minusINT                     |88070106    |Int32 subtraction                                 |INT          |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |minusUINT                    |88070107    |Unsigned Int32 subtraction                        |UINT         |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |minusVAST                    |88070108    |Int64 subtraction                                 |VAST         |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |minusUVAST                   |88070109    |Unsigned Int64 subtraction                        |UVAST        |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |minusREAL32                  |8807010a    |Real32 subtraction                                |REAL32       |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |minusREAL64                  |8807010b    |Real64 subtraction                                |REAL64       |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |multINT                      |8807010c    |Int32 multiplication                              |INT          |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |multUINT                     |8807010d    |Unsigned Int32 multiplication                     |UINT         |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |multVAST                     |8807010e    |Int64 multiplication                              |VAST         |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |multUVAST                    |8807010f    |Unsigned Int64 multiplication                     |UVAST        |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |multREAL32                   |88070110    |Real32 multiplication                             |REAL32       |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |multREAL64                   |88070111    |Real64 multiplication                             |REAL64       |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |divINT                       |88070112    |Int32 division                                    |INT          |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |divUINT                      |88070113    |Unsigned Int32 division                           |UINT         |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |divVAST                      |88070114    |Int64 division                                    |VAST         |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |divUVAST                     |88070115    |Unsigned Int64 division                           |UVAST        |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |divREAL32                    |88070116    |Real32 division                                   |REAL32       |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |divREAL64                    |88070117    |Real64 division                                   |REAL64       |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |modINT                       |88070118    |Int32 modulus division                            |INT          |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |modUINT                      |88070119    |Unsigned Int32 modulus division                   |UINT         |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |modVAST                      |8807011a    |Int64 modulus division                            |VAST         |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |modUVAST                     |8807011b    |Unsigned Int64 modulus division                   |UVAST        |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |modREAL32                    |8807011c    |Real32 modulus division                           |REAL32       |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |modREAL64                    |8807011d    |Real64 modulus division                           |REAL64       |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |expINT                       |8807011e    |Int32 exponentiation                              |INT          |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |expUINT                      |8807011f    |Unsigned int32 exponentiation                     |UINT         |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |expVAST                      |88070120    |Int64 exponentiation                              |VAST         |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |expUVAST                     |88070121    |Unsigned Int64 exponentiation                     |UVAST        |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |expREAL32                    |88070122    |Real32 exponentiation                             |REAL32       |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |expREAL64                    |88070123    |Real64 exponentiation                             |REAL64       |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |bitAND                       |88070124    |Bitwise and                                       |BYTESTR      |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |bitOR                        |88070125    |Bitwise or                                        |BYTESTR      |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |bitXOR                       |88070126    |Bitwise xor                                       |BYTESTR      |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |bitNOT                       |88070127    |Bitwise not                                       |BYTESTR      |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |logAND                       |88070128    |Logical and                                       |BOOL         |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |logOR                        |88070129    |Logical or                                        |BOOL         |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |logNOT                       |8807012a    |Logical not                                       |BOOL         |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |abs                          |8807012b    |absolute value                                    |INT          |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |lessTHAN                     |8807012c    |Less than                                         |BOOL         |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |greaterTHAN                  |8807012d    |Greater than                                      |BOOL         |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |lessEQUAL                    |8807012e    |Less than or equal to                             |BOOL         |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |greaterEQUAL                 |8807012f    |Greater than or equal to                          |BOOL         |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |notEQUAL                     |88070130    |Not equal                                         |BOOL         |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |EQUAL                        |88070131    |Equal to                                          |BOOL         |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |bitShiftLeft                 |88070132    |Bitwise left shift                                |BYTESTR      |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |bitShiftRight                |88070133    |Bitwise right shift                               |BYTESTR      |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 * |STOR                         |88070134    |Store value of parm 2 in parm 1                   |BYTESTR      |
 * +-----------------------------+------------+--------------------------------------------------+-------------+
 */
#define AMP_AGENT_PLUSINT       0x00
#define AMP_AGENT_PLUSUINT      0x01
#define AMP_AGENT_PLUSVAST      0x02
#define AMP_AGENT_PLUSUVAST     0x03
#define AMP_AGENT_PLUSREAL32    0x04
#define AMP_AGENT_PLUSREAL64    0x05
#define AMP_AGENT_MINUSINT      0x06
#define AMP_AGENT_MINUSUINT     0x07
#define AMP_AGENT_MINUSVAST     0x08
#define AMP_AGENT_MINUSUVAST    0x09
#define AMP_AGENT_MINUSREAL32   0x0a
#define AMP_AGENT_MINUSREAL64   0x0b
#define AMP_AGENT_MULTINT       0x0c
#define AMP_AGENT_MULTUINT      0x0d
#define AMP_AGENT_MULTVAST      0x0e
#define AMP_AGENT_MULTUVAST     0x0f
#define AMP_AGENT_MULTREAL32    0x10
#define AMP_AGENT_MULTREAL64    0x11
#define AMP_AGENT_DIVINT        0x12
#define AMP_AGENT_DIVUINT       0x13
#define AMP_AGENT_DIVVAST       0x14
#define AMP_AGENT_DIVUVAST      0x15
#define AMP_AGENT_DIVREAL32     0x16
#define AMP_AGENT_DIVREAL64     0x17
#define AMP_AGENT_MODINT        0x18
#define AMP_AGENT_MODUINT       0x19
#define AMP_AGENT_MODVAST       0x1a
#define AMP_AGENT_MODUVAST      0x1b
#define AMP_AGENT_MODREAL32     0x1c
#define AMP_AGENT_MODREAL64     0x1d
#define AMP_AGENT_EXPINT        0x1e
#define AMP_AGENT_EXPUINT       0x1f
#define AMP_AGENT_EXPVAST       0x20
#define AMP_AGENT_EXPUVAST      0x21
#define AMP_AGENT_EXPREAL32     0x22
#define AMP_AGENT_EXPREAL64     0x23
#define AMP_AGENT_BITAND        0x24
#define AMP_AGENT_BITOR         0x25
#define AMP_AGENT_BITXOR        0x26
#define AMP_AGENT_BITNOT        0x27
#define AMP_AGENT_LOGAND        0x28
#define AMP_AGENT_LOGOR         0x29
#define AMP_AGENT_LOGNOT        0x2a
#define AMP_AGENT_ABS           0x2b
#define AMP_AGENT_LESSTHAN      0x2c
#define AMP_AGENT_GREATERTHAN   0x2d
#define AMP_AGENT_LESSEQUAL     0x2e
#define AMP_AGENT_GREATEREQUAL  0x2f
#define AMP_AGENT_NOTEQUAL      0x30
#define AMP_AGENT_EQUAL         0x31
#define AMP_AGENT_BITSHIFTLEFT  0x32
#define AMP_AGENT_BITSHIFTRIGHT 0x33
#define AMP_AGENT_STOR          0x34

/* Initialization functions. */
void amp_agent_init();
void amp_agent_init_meta();
void amp_agent_init_cnst();
void amp_agent_init_edd();
void amp_agent_init_op();
void amp_agent_init_var();
void amp_agent_init_ctrl();
void amp_agent_init_macro();
void amp_agent_init_rptt();
void amp_agent_init_tblt();


#endif /* _HAVE_AMP_AGENT_ADM_ */
#endif //ADM_AMP_AGENT_H_
