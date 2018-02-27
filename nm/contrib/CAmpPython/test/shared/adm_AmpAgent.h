/******************************************************************************
 **
 ** File Name: ./shared/adm_AmpAgent.h
 **
 ** Description: 
 **
 ** Notes: 
 **
 ** Assumptions: 
 **
 ** Modification History: 
 **  MM/DD/YY  AUTHOR           DESCRIPTION
 **  --------  --------------   ------------------------------------------------
 **
 **  2017-11-21  AUTO           Auto generated header file 
 **
*********************************************************************************/
#ifndef ADM_AMPAGENT_H_
#define ADM_AMPAGENT_H_
#define _HAVE_AMPAGENT_ADM_
#ifdef _HAVE_AMPAGENT_ADM_

#include "lyst.h"
#include "../utils/nm_types.h"
#include "adm.h"

/*
 * +----------------------------------------------------------------------------------------------------------+
 * |			              ADM TEMPLATE DOCUMENTATION                                                          +
 * +----------------------------------------------------------------------------------------------------------+
 *
 * ADM ROOT STRING:arn:DTN:AmpAgent
 */
/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    AMPAGENT META-DATA DEFINITIONS                                                          +
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |name                         |805a0100    |The human-readable name of the ADM.               |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |namespace                    |805a0101    |The namespace of the ADM.                         |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |version                      |805a0102    |The version of the ADM.                           |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |organization                 |805a0103    |The name of the issuing organization of the ADM.  |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */
// "name"
#define ADM_AMPAGENT_META_NAME_MID "805a0100"
// "namespace"
#define ADM_AMPAGENT_META_NAMESPACE_MID "805a0101"
// "version"
#define ADM_AMPAGENT_META_VERSION_MID "805a0102"
// "organization"
#define ADM_AMPAGENT_META_ORGANIZATION_MID "805a0103"

/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                 AMPAGENT EXTERNALLY DEFINED DATA DEFINITIONS                                               +
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |numReports                   |805b0100    |This is the number of reports known to the Agent. |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |sentReports                  |805b0101    |This is the number of reports sent by the agent.  |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |numTrl                       |805b0102    |This is the number of time-based rules running on |             |
   |                             |            |the agent.                                        |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |runTrl                       |805b0103    |This is the number of time-based rules run by the |             |
   |                             |            |agent since the last reset.                       |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |numSrl                       |805b0104    |This is the number of state-based rules running on|             |
   |                             |            | the agent.                                       |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |runSrl                       |805b0105    |This is the number of state-based rules run by the|             |
   |                             |            | agent since the last reset.                      |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |numConst                     |805b0106    |This is the number of constants known by the agent|             |
   |                             |            |.                                                 |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |numVar                       |805b0107    |This is the number of variables known by the agent|             |
   |                             |            |.                                                 |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |numMacros                    |805b0108    |This is the number of macros known by the agent.  |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |runMacros                    |805b0109    |This is the number of macros run by the agent sinc|             |
   |                             |            |e the last reset.                                 |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |numCTRL                      |805b010a    |This is the number of controls known by the agent.|UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |runCTRL                      |805b010b    |This is the number of controls run by the agent si|             |
   |                             |            |nce the last reset.                               |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |curTime                      |805b010c    |This is the current system time.                  |TS           |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */
#define ADM_AMPAGENT_EDD_NUMREPORTS_MID "805b0100"
#define ADM_AMPAGENT_EDD_SENTREPORTS_MID "805b0101"
#define ADM_AMPAGENT_EDD_NUMTRL_MID "805b0102"
#define ADM_AMPAGENT_EDD_RUNTRL_MID "805b0103"
#define ADM_AMPAGENT_EDD_NUMSRL_MID "805b0104"
#define ADM_AMPAGENT_EDD_RUNSRL_MID "805b0105"
#define ADM_AMPAGENT_EDD_NUMCONST_MID "805b0106"
#define ADM_AMPAGENT_EDD_NUMVAR_MID "805b0107"
#define ADM_AMPAGENT_EDD_NUMMACROS_MID "805b0108"
#define ADM_AMPAGENT_EDD_RUNMACROS_MID "805b0109"
#define ADM_AMPAGENT_EDD_NUMCTRL_MID "805b010a"
#define ADM_AMPAGENT_EDD_RUNCTRL_MID "805b010b"
#define ADM_AMPAGENT_EDD_CURTIME_MID "805b010c"

/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |			                  AMPAGENT VARIABLE DEFINITIONS                                                          +
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |numRules                     |815c0100    |This is the number of rules known to the Agent(#TR|             |
   |                             |            |L + #SRl).                                        |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */
#define ADM_AMPAGENT_VAR_NUMRULES_MID "815c0100"

/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |				                AMPAGENT REPORT DEFINITIONS                                                           +
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |fullReport                   |825d0100    |This is all known meta-data, EDD, and VAR values k|             |
   |                             |            |nown by the agent.                                |?            |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */
#define ADM_AMPAGENT_RPT_FULLREPORT_MID "825d0100"

/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |			                    AMPAGENT CONTROL DEFINITIONS                                                         +
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |listADMs                     |835e0100    |This control causes the Agent to produce a report |             |
   |                             |            |entry detailing the name of each ADM supported by |             |
   |                             |            |the Agent.                                        |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |addVar                       |c35e0101    |This control configures a new variable definition |             |
   |                             |            |on the Agent.                                     |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |delVar                       |c35e0102    |This control removes one or more variable definiti|             |
   |                             |            |ons from the Agent.                               |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |listVar                      |835e0103    |This control produces a listing of every variable |             |
   |                             |            |identifier(MID) known to the Agent.               |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |descVar                      |c35e0104    |This control produces a detailed description of on|             |
   |                             |            |e or more variable identifier(MID)s known to the A|             |
   |                             |            |gent.                                             |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |addRptTpl                    |c35e0105    |This control configures a new report template defi|             |
   |                             |            |nition on the Agent.                              |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |delRptTpl                    |c35e0106    |This control removes one or more report template d|             |
   |                             |            |efinitions from the Agent.                        |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |listRptTpl                   |835e0107    |This control produces a listing of every report te|             |
   |                             |            |mplate identifier(MID) known to the Agent.        |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |descRptTpl                   |c35e0108    |This control produces a detailed description of on|             |
   |                             |            |e or more report template  identifier(MID) known t|             |
   |                             |            |o the Agent.                                      |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |genRpts                      |c35e0109    |This control causes the Agent to produce a report |             |
   |                             |            |entry for each identified report templates and sen|             |
   |                             |            |d them to one or more identified managers(MIDs).  |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |addMacro                     |c35e010a    |This control configures a new macro definition on |             |
   |                             |            |the Agent.                                        |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |delMacro                     |c35e010b    |This control removes one or more macro definitions|             |
   |                             |            | from the Agent.                                  |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |listMacro                    |835e010c    |This control produces a listing of every macro ide|             |
   |                             |            |ntifier(MID) known to the Agent.                  |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |descMacro                    |c35e010d    |This control produces a detailed description of on|             |
   |                             |            |e or more macro identifier(MID) known to the Agent|             |
   |                             |            |.                                                 |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |addTrl                       |c35e010e    |This control configures a new time-based rule(TRL)|             |
   |                             |            | definition on the Agent.                         |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |delTrl                       |c35e010f    |This control removes one or more TRL definitions f|             |
   |                             |            |rom the Agent.                                    |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |listTrl                      |835e0110    |This control produces a listing of every TRL ident|             |
   |                             |            |ifier(MID) known to the Agent.                    |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |desCTRL                      |c35e0111    |This control produces a detailed description of on|             |
   |                             |            |e or more TRL identifier(MID)s known to the Agent.|             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |addSrl                       |c35e0112    |This control configures a new state-based rule(SRL|             |
   |                             |            |) definition on the Agent.                        |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |delSrl                       |c35e0113    |This control removes one or more SRL definitions f|             |
   |                             |            |rom the Agent.                                    |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |listSrl                      |835e0114    |This control produces a listing of every macro ide|             |
   |                             |            |ntifier(MID) known to the Agent.                  |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |descSrl                      |c35e0115    |This control produces a detailed description of on|             |
   |                             |            |e or more SRL identifier(MID)s known to the Agent.|             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |storeVar                     |c35e0116    |This control stores variables.                    |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |resetCounts                  |835e0117    |This control resets all Agent ADM statistics repor|             |
   |                             |            |ted in the Agent ADM report.                      |             |
   +-----------------------------+------------+----------------------------------------------------------------+
 */
#define ADM_AMPAGENT_CTRL_LISTADMS_MID "835e0100"
#define ADM_AMPAGENT_CTRL_ADDVAR_MID "c35e0101"
#define ADM_AMPAGENT_CTRL_DELVAR_MID "c35e0102"
#define ADM_AMPAGENT_CTRL_LISTVAR_MID "835e0103"
#define ADM_AMPAGENT_CTRL_DESCVAR_MID "c35e0104"
#define ADM_AMPAGENT_CTRL_ADDRPTTPL_MID "c35e0105"
#define ADM_AMPAGENT_CTRL_DELRPTTPL_MID "c35e0106"
#define ADM_AMPAGENT_CTRL_LISTRPTTPL_MID "835e0107"
#define ADM_AMPAGENT_CTRL_DESCRPTTPL_MID "c35e0108"
#define ADM_AMPAGENT_CTRL_GENRPTS_MID "c35e0109"
#define ADM_AMPAGENT_CTRL_ADDMACRO_MID "c35e010a"
#define ADM_AMPAGENT_CTRL_DELMACRO_MID "c35e010b"
#define ADM_AMPAGENT_CTRL_LISTMACRO_MID "835e010c"
#define ADM_AMPAGENT_CTRL_DESCMACRO_MID "c35e010d"
#define ADM_AMPAGENT_CTRL_ADDTRL_MID "c35e010e"
#define ADM_AMPAGENT_CTRL_DELTRL_MID "c35e010f"
#define ADM_AMPAGENT_CTRL_LISTTRL_MID "835e0110"
#define ADM_AMPAGENT_CTRL_DESCTRL_MID "c35e0111"
#define ADM_AMPAGENT_CTRL_ADDSRL_MID "c35e0112"
#define ADM_AMPAGENT_CTRL_DELSRL_MID "c35e0113"
#define ADM_AMPAGENT_CTRL_LISTSRL_MID "835e0114"
#define ADM_AMPAGENT_CTRL_DESCSRL_MID "c35e0115"
#define ADM_AMPAGENT_CTRL_STOREVAR_MID "c35e0116"
#define ADM_AMPAGENT_CTRL_RESETCOUNTS_MID "835e0117"

/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |				                AMPAGENT CONSTANT DEFINITIONS                                                         +
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |AMPEpoch                     |875f0100    |This constant is the time epoch for the Agent.    |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */
#define ADM_AMPAGENT_CONST_AMPEPOCH_MID "875f0100"

/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |				                AMPAGENT MACRO DEFINITIONS                                                            +
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |userList                     |86600100    |This macro lists all of the user defined data.    |mc           |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */
#define ADM_AMPAGENT_MACRO_USERLIST_MID "86600100"

/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |							      AMPAGENT OPERATOR DEFINITIONS                                                          +
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |+INT                         |88610100    |Int32 addition                                    |INT          |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |+UINT                        |88610101    |Unsigned int32 addition                           |UINT         |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |+VAST                        |88610102    |Int64 addition                                    |VAST         |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |+UVAST                       |88610103    |Unsigned int64 addition                           |UVAST        |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |+REAL32                      |88610104    |Real32 addition                                   |REAL32       |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |+REAL64                      |88610105    |Real64 addition                                   |REAL64       |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |-INT                         |88610106    |Int32 subtraction                                 |INT          |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |-UINT                        |88610107    |Unsigned int32 subtraction                        |UINT         |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |-VAST                        |88610108    |Int64 subtraction                                 |VAST         |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |-UVAST                       |88610109    |Unsigned int64 subtraction                        |UVAST        |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |-REAL32                      |8861010a    |Real32 subtraction                                |REAL32       |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |-REAL64                      |8861010b    |Real64 subtraction                                |REAL64       |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |*INT                         |8861010c    |Int multiplication                                |INT          |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |*UINT                        |8861010d    |Unsigned int32 multiplication                     |UINT         |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |*VAST                        |8861010e    |Int64 multiplication                              |VAST         |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |*UVAST                       |8861010f    |Unsigned int64 multiplication                     |UVAST        |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |*REAL32                      |88610110    |Real32 multiplication                             |REAL32       |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |*REAL64                      |88610111    |Real64 multiplication                             |REAL64       |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |/INT                         |88610112    |Int32 division                                    |INT          |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |/UINT                        |88610113    |Unsigned int32 division                           |UINT         |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |/VAST                        |88610114    |Int64 division                                    |VAST         |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |/UVAST                       |88610115    |Unsigned int64 division                           |UVAST        |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |/REAL32                      |88610116    |Real32 division                                   |REAL32       |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |/REAL64                      |88610117    |Real64 division                                   |REAL64       |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |MODINT                       |88610118    |Int32 modulus division                            |INT          |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |MODUINT                      |88610119    |Unsigned int32 modulus division                   |UINT         |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |MODVAST                      |8861011a    |Int64 modulus division                            |VAST         |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |MODUVAST                     |8861011b    |Unsigned int64 modulus division                   |UVAST        |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |MODREAL32                    |8861011c    |Real32 modulus division                           |REAL32       |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |MODREAL64                    |8861011d    |Real64 modulus division                           |REAL64       |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |^INT                         |8861011e    |Int32 exponentiation                              |INT          |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |^UINT                        |8861011f    |Unsigned int32 exponentiation                     |UINT         |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |^VAST                        |88610120    |Int64 exponentiation                              |VAST         |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |^UVAST                       |88610121    |Unsigned int64 exponentiation                     |UVAST        |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |^REAL32                      |88610122    |Real32 exponentiation                             |REAL32       |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |^REAL64                      |88610123    |Real64 exponentiation                             |REAL64       |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |&                            |88610106    |Bitwise and                                       |BLOB         |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   ||                            |88610107    |Bitwise or                                        |BLOB         |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |#                            |88610108    |Bitwise xor                                       |BLOB         |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |~                            |88610109    |Bitwise not                                       |BLOB         |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |&&                           |8861010a    |Logical and                                       |BOOL         |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |||                           |8861010b    |Logical or                                        |BOOL         |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |!                            |8861010c    |Logical not                                       |BOOL         |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |abs                          |8861010d    |absolute value                                    |INT          |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |<                            |8861010e    |Less than                                         |BOOL         |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |>                            |8861010f    |Greater than                                      |BOOL         |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |<=                           |88610110    |Less than or equal to                             |BOOL         |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |>=                           |88610111    |Greater than or equal to                          |BOOL         |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |!=                           |88610112    |Not equal                                         |BOOL         |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |==                           |88610113    |Equal to                                          |BOOL         |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |<<                           |88610114    |Bitwise left shift                                |BLOB         |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |>>                           |88610115    |Bitwise right shift                               |BLOB         |
   +-----------------------------+------------+-------------------------------------------------+-------------+
   |STOR                         |88610116    |Store value of parm 2 in parm 1                   |             |
   +-----------------------------+------------+-------------------------------------------------+-------------+
 */
#define ADM_AMPAGENT_OP_+INT_MID "88610100"
#define ADM_AMPAGENT_OP_+UINT_MID "88610101"
#define ADM_AMPAGENT_OP_+VAST_MID "88610102"
#define ADM_AMPAGENT_OP_+UVAST_MID "88610103"
#define ADM_AMPAGENT_OP_+REAL32_MID "88610104"
#define ADM_AMPAGENT_OP_+REAL64_MID "88610105"
#define ADM_AMPAGENT_OP_-INT_MID "88610106"
#define ADM_AMPAGENT_OP_-UINT_MID "88610107"
#define ADM_AMPAGENT_OP_-VAST_MID "88610108"
#define ADM_AMPAGENT_OP_-UVAST_MID "88610109"
#define ADM_AMPAGENT_OP_-REAL32_MID "8861010a"
#define ADM_AMPAGENT_OP_-REAL64_MID "8861010b"
#define ADM_AMPAGENT_OP_*INT_MID "8861010c"
#define ADM_AMPAGENT_OP_*UINT_MID "8861010d"
#define ADM_AMPAGENT_OP_*VAST_MID "8861010e"
#define ADM_AMPAGENT_OP_*UVAST_MID "8861010f"
#define ADM_AMPAGENT_OP_*REAL32_MID "88610110"
#define ADM_AMPAGENT_OP_*REAL64_MID "88610111"
#define ADM_AMPAGENT_OP_/INT_MID "88610112"
#define ADM_AMPAGENT_OP_/UINT_MID "88610113"
#define ADM_AMPAGENT_OP_/VAST_MID "88610114"
#define ADM_AMPAGENT_OP_/UVAST_MID "88610115"
#define ADM_AMPAGENT_OP_/REAL32_MID "88610116"
#define ADM_AMPAGENT_OP_/REAL64_MID "88610117"
#define ADM_AMPAGENT_OP_MODINT_MID "88610118"
#define ADM_AMPAGENT_OP_MODUINT_MID "88610119"
#define ADM_AMPAGENT_OP_MODVAST_MID "8861011a"
#define ADM_AMPAGENT_OP_MODUVAST_MID "8861011b"
#define ADM_AMPAGENT_OP_MODREAL32_MID "8861011c"
#define ADM_AMPAGENT_OP_MODREAL64_MID "8861011d"
#define ADM_AMPAGENT_OP_^INT_MID "8861011e"
#define ADM_AMPAGENT_OP_^UINT_MID "8861011f"
#define ADM_AMPAGENT_OP_^VAST_MID "88610120"
#define ADM_AMPAGENT_OP_^UVAST_MID "88610121"
#define ADM_AMPAGENT_OP_^REAL32_MID "88610122"
#define ADM_AMPAGENT_OP_^REAL64_MID "88610123"
#define ADM_AMPAGENT_OP_&_MID "88610106"
#define ADM_AMPAGENT_OP_|_MID "88610107"
#define ADM_AMPAGENT_OP_#_MID "88610108"
#define ADM_AMPAGENT_OP_~_MID "88610109"
#define ADM_AMPAGENT_OP_&&_MID "8861010a"
#define ADM_AMPAGENT_OP_||_MID "8861010b"
#define ADM_AMPAGENT_OP_!_MID "8861010c"
#define ADM_AMPAGENT_OP_ABS_MID "8861010d"
#define ADM_AMPAGENT_OP_<_MID "8861010e"
#define ADM_AMPAGENT_OP_>_MID "8861010f"
#define ADM_AMPAGENT_OP_<=_MID "88610110"
#define ADM_AMPAGENT_OP_>=_MID "88610111"
#define ADM_AMPAGENT_OP_!=_MID "88610112"
#define ADM_AMPAGENT_OP_==_MID "88610113"
#define ADM_AMPAGENT_OP_<<_MID "88610114"
#define ADM_AMPAGENT_OP_>>_MID "88610115"
#define ADM_AMPAGENT_OP_STOR_MID "88610116"

/* Initialization functions. */
void adm_AmpAgent_init();
void adm_AmpAgent_init_edd();
void adm_AmpAgent_init_variables();
void adm_AmpAgent_init_controls();
void adm_AmpAgent_init_constants();
void adm_AmpAgent_init_macros();
void adm_AmpAgent_init_metadata();
void adm_AmpAgent_init_ops();
void adm_AmpAgent_init_reports();
#endif /* _HAVE_AMPAGENT_ADM_ */
#endif //ADM_AMPAGENT_H_