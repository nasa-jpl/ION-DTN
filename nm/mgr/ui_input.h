/*****************************************************************************
 **
 ** \file ui_input.h
 **
 **
 ** Description: Functions to retrieve information from the user via a
 **              text-based interface.
 **
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  05/24/15  E. Birrane     Initial Implementation (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

#ifndef _UI_INPUT_H
#define _UI_INPUT_H

#include "../shared/utils/nm_types.h"
#include "../shared/adm/adm.h"
#include "../shared/primitives/mid.h"
#include "../shared/primitives/blob.h"

#include "mgr/nm_mgr_ui.h"

#define MAX_INPUT_BYTES 1024

int ui_input_get_line(char *prompt, char **line, int max_len);


/*
 * User input methods for basic data types.
 */
blob_t*  ui_input_blob(char *prompt, uint8_t no_file);
uint8_t  ui_input_byte(char *prompt);
double   ui_input_double(char *prompt);
blob_t*  ui_input_file_contents(char *prompt);
float    ui_input_float(char *prompt);
int32_t  ui_input_int(char *prompt);
Sdnv     ui_input_sdnv(char *prompt);
char *   ui_input_string(char *prompt);
uint32_t ui_input_uint(char *prompt);
uvast    ui_input_uvast(char *prompt);
vast     ui_input_vast(char *prompt);

/*
 * User input methods for DTNMP complex types.
 */

//adm_datadef_t*     ui_input_atomic(char *prompt);
//adm_computeddef_t* ui_input_compted_def(char *prompt);
//adm_ctrl_t*        ui_input_ctrl(char *prompt);
Lyst               ui_input_dc(char *prompt);
//lit_t*             ui_input_lit(char *prompt);
Lyst               ui_input_mc(char *prompt);
mid_t*             ui_input_mid(char *prompt, uint8_t adm_type, uint8_t id);
mid_t*             ui_input_mid_build();
int                ui_input_mid_flag(uint8_t *flag);
mid_t*             ui_input_mid_list(uint8_t adm_type, uint8_t id);
mid_t*             ui_input_mid_raw(uint8_t no_file);
oid_t              ui_input_oid(uint8_t mid_flags);
//adm_op_t*          ui_input_op(char *prompt);
tdc_t*             ui_input_parms(ui_parm_spec_t *spec);

#endif // _UI_INPUT_H
