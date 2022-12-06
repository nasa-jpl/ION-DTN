/******************************************************************************
#include <sc_util.h>
 **                           COPYRIGHT NOTICE
 **      (c) 2021 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: sc_util.c
 **
 ** Namespace: bpsec_scutl
 **
 ** Description:
 **
 **     This file contains a series of generic utility functions that can
 **     be used by any security context implementation within ION for
 **     security processing.
 **
 **     These utilities focus on the creation and handling of generic data
 **     structures comprising the SCI interface.
 **
 ** Assumptions:
 **  - Assumed that state lives as long as its security context pointer. :)
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  08/05/21  E. Birrane     Initial implementation
 **
 *****************************************************************************/
#include <stddef.h>

#include "bpsec_util.h"
#include "sc_util.h"
#include "sc_value.h"


/*****************************************************************************
 *                           CONSTANTS DEFINITIONS                           *
 *****************************************************************************/



/*****************************************************************************
 *                             FUNCTION DEFINITIONS                          *
 *****************************************************************************/



/******************************************************************************
 * @brief Turn a single char representing a hex value into a nibble.
 *
 * @param[in] c  The character being converted.
 *
 * @note
 *  - For example, 'a' or 'A' should return 0x1010
 *
 * @retval -1  - Error
 * @retval !-1 - The nibble
 *****************************************************************************/

int bpsec_scutl_hexNibbleGet(char c)
{
    if((c >= '0') && (c <= '9'))
    {
        return (c - '0');
    }
    else if((c >= 'A') && (c <= 'F'))
    {
        return 10 + (c - 'A');
    }
    else if((c >= 'a') && (c <= 'f'))
    {
         return 10 + (c - 'a');
    }

    return -1;
}

char* bpsec_scutl_hexStrCvt(uint8_t *data, int len)
{
    const char xx[]= "0123456789ABCDEF";
    int i = len*2;
    CHKNULL(data);

    char *result = MTAKE(i + 1);
    CHKNULL(result);

    while (--i >= 0) result[i] = xx[(data[i>>1] >> ((1 - (i&1)) << 2)) & 0xF];
    result[len*2]=0;

    return result;
}



// TODO document function
// TODO - need to free the results of the key when done. This is a deep copy.
int   bpsec_scutl_keyGet(sc_state *state, int key_id, sc_value *key_value)
{
    sc_value *key_name = NULL;

    BPSEC_DEBUG_PROC("("ADDR_FIELDSPEC","ADDR_FIELDSPEC")",
                     (uaddr) state, (uaddr) key_value);

    if((key_name = bpsec_scv_lystFind(state->scStParms, key_id, SC_VAL_TYPE_PARM)) == NULL)
    {
        BPSEC_DEBUG_ERR("Cannot find parm.", NULL);
        return -1;
    }
    else
    {
        void *tmp = bpsec_scv_rawGet(state->scStWm, key_name);

        *key_value = bpsec_util_keyRetrieve(tmp);
        if(key_value->scValLength == 0)
        {
            BPSEC_DEBUG_ERR("Cannot find value for key %s", tmp);
            return -1;
        }

    }

    return 0;
}



// TODO document function
int   bpsec_scutl_keyUnwrap(sc_state *state, int kek_id, csi_val_t *key_value, int wrappedId, int suite, csi_cipherparms_t *csi_parms)
{
    int result = -1;

    sc_value *wrappedKey = NULL;
    sc_value ltk;
    csi_val_t csi_ltk;
    csi_val_t csi_wrappedKey;


    /* Step 1 - Grab the LTK and convert to a CSI value. */
    if(bpsec_scutl_keyGet(state, kek_id, &ltk) < 0)
    {
        BPSEC_DEBUG_ERR("Can't unwrap key because LTK not found.", NULL);
        return -1;
    }

    csi_ltk.contents = ltk.scRawValue.asPtr;
    csi_ltk.len = ltk.scValLength;


    /*
     * Step 2 - Find the wrapped key and convert it to a CSI value. If we
     *          cannot find a wrapped key, default to using the LTK.
     */
    if((wrappedKey = bpsec_scv_lystFind(state->scStParms, wrappedId, SC_VAL_TYPE_PARM)) == NULL)
    {
        BPSEC_DEBUG_ERR("Did not find wrapped key. Trying LTK.",NULL);
        *key_value = csi_ltk;
        return 1;
    }

    csi_wrappedKey.contents = wrappedKey->scRawValue.asPtr;
    csi_wrappedKey.len = wrappedKey->scValLength;


    /* Step 3 - If we found a wrapped key, unwrap it. */

    if(suite == CSTYPE_AES_KW)
    {
        result = csi_keywrap(0, csi_ltk, csi_wrappedKey, key_value);
    }
    else
    {
    	result = csi_crypt_key(suite, CSI_SVC_DECRYPT, csi_parms, csi_ltk, csi_wrappedKey, key_value);
    }

    /* Step 4 - Clean up and return.  */
    bpsec_scv_clear(0, &ltk);


    if(result == ERROR)
    {
        BPSEC_DEBUG_ERR("Could not unwrap session key.", NULL);
    }

    return result;
}





// TODO document function
/*
 * These are shallow copies. Don't delete!
 */
void  bpsec_scutl_parmsExtract(sc_state *state, csi_cipherparms_t *parms)
{
    sc_value *tmp;
    LystElt elt;

    memset(parms,0,sizeof(csi_cipherparms_t));

    for(elt = lyst_first(state->scStParms); elt; elt = lyst_next(elt))
    {
        if((tmp = lyst_data(elt)) == NULL)
        {
            continue;
        }

        switch(tmp->scValId)
        {
            case CSI_PARM_IV:
                parms->iv.contents = tmp->scRawValue.asPtr;
                parms->iv.len = tmp->scValLength;
                break;

            case CSI_PARM_SALT:
                parms->salt.contents = tmp->scRawValue.asPtr;
                parms->salt.len = tmp->scValLength;
                break;

            case CSI_PARM_ICV:
                parms->icv.contents = tmp->scRawValue.asPtr;
                parms->icv.len = tmp->scValLength;
                break;

            case CSI_PARM_INTSIG:
                parms->intsig.contents = tmp->scRawValue.asPtr;
                parms->intsig.len = tmp->scValLength;
                break;

            /*
             * CSI is broken in this regard, as there are defines for both
             * BEK and KEY_INFO with poor definition. For now, we will assume
             * that in the context where this function is called, key_info means
             * the wrapped key and we get other key info outside of the CSI API.
             */
            case CSI_PARM_BEK:
                parms->keyinfo.contents = tmp->scRawValue.asPtr;
                parms->keyinfo.len = tmp->scValLength;
                break;

            case CSI_PARM_BEKICV:
                parms->aad.contents = tmp->scRawValue.asPtr;
                parms->aad.len = tmp->scValLength;
                break;
        }
    }
}




/******************************************************************************
 * @brief Generate a string from a series of other strings.
 *
 * @param[in]  array      Series of strings being catenated.
 * @param[in]  size       Size of all strings.
 * @param[in]  num_items  Number of items in the array
 *
 * @todo
 *  1. More error checking.
 *
 * @note
 *  1. The resultant string is a comma-separated list of parameters
 *  2. The resultant string must be released by the caller.
 *
 * @retval !NULL - The string representation of the parameters.
 * @retval NULL  - Error or no parameters.
 *****************************************************************************/

char *bpsec_scutl_strFromStrsCreate(char **array, int size, int num_items)
{
    char *result = NULL;
    int idx = 0;
    int i = 0;

    CHKNULL(array);
    CHKNULL(size);

    result = (char *) MTAKE(size + num_items + 1);

    for(i = 0; i < num_items; i++)
    {
        if(array[i] != NULL)
        {
            memcpy(result + idx, array[i], strlen(array[i]));
            idx += strlen(array[i]);
            memcpy(result + idx, " ", 1);
            idx++;
            MRELEASE(array[i]);
            array[i] = NULL;
        }
    }
    memset(result + idx, 0, 1);
    MRELEASE(array);

    return result;
}














