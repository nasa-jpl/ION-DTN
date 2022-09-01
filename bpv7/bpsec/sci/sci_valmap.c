/******************************************************************************
#include <sc_util.h>
 **                           COPYRIGHT NOTICE
 **      (c) 2021 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

// TODO: Update documentation
/*****************************************************************************
 **
 ** File Name: sciP.c
 **
 ** Namespace: bpsec_scip
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
 ** Notes:
 **
 **     1. This interface does not, itself, perform any cryptographic functions.
 **
 **     2. The ION Open Source distribution ships with a NULL implementation of a
 **        security context. This NULL implementation performs no cryptographic
 **        functions and, instead, only populates "dummy" data appropriate for
 **        unit testing only.
 **
 **     3. Security context parameters are stored in shared memory using the
 **        Personal Space Manager. This is because parameters (like other policy
 **        configuration) may be accessed by multiple processes/threads within the
 **        ION ecosystem.
 **
 **     4. The security context "state" represents the state of a security context in
 **        the process of applying a security service at a node (such as when encrypting
 **        a target block).  This state is used under the auspices of the single
 **        thread within the BPA tasked with the encryption. As such, this state is
 **        stored in regular heap memory and not in shared memory.
 **
 **
 ** Assumptions:
 **
 **  - Assumed that state lives as long as its security context pointer. :)
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  08/30/21  E. Birrane     Initial implementation
 **
 *****************************************************************************/

#include "sci.h"
#include "sci_valmap.h"
#include "sc_value.h"
#include "sc_util.h"
#include "bpsec_util.h"





/******************************************************************************
 * @brief Returns a value from a key-map memory set
 *
 * @param[in]  map  - The specific map being queried
 * @param[in]  id   - The key being queried
 * @param[in]  type - Whether this is a parameter or a result
 *
 *
 * @note
 * This function serves as an associative array lookup
 *
 * Keys in this array are static strings.  Values are integers.
 *
 * @retval 0 - The value
 * @retval <0  - The value was not found.
 *****************************************************************************/

int   bpsec_scvm_byIdIdxFind(sc_value_map map[], int id, sc_val_type type)
{
    int idx = 0;

    while(map[idx].scValName != NULL)
    {
        if((type == map[idx].scValType) &&
           (id == map[idx].scValId))
        {
          return idx;
        }
        idx++;
    }

    return -1;
}



/******************************************************************************
 * @brief Returns the name of a security context paramater's type and id.
 *
 * @param[in]  map   - The specific map being queried
 * @param[in]  id    - The ID of the value
 * @param[in]  type  - Whether this is a parameter or a result
 *
 *
 * @note
 * 1. Security context values are uniquely identified by the 2-tuple of
 *    (id, type) where type is either a security context parameter or a
 *    security context result.
 *
 * 2. The returned string is a constant and MUST NOT be freed by the caller.
 *
 * @retval NULL  - The id/type was not found
 * @retval !NULL - The name of the item.
 *****************************************************************************/

char *bpsec_scvm_byIdNameFind(sc_value_map map[], int id, sc_val_type type)
{
    int idx = bpsec_scvm_byIdIdxFind(map, id, type);

    if(idx >= 0)
    {
        return map[idx].scValName;
    }

    return NULL;
}



/******************************************************************************
 * @brief Returns a value from a key-map memory set
 *
 * @param[in]  map   - The specific map being queried
 * @param[in]  key   - The key being queried
 *
 * @note
 * This function serves as an associative array lookup
 *
 * Keys in this array are static strings.  Values are integers.
 *
 * @retval >=0 - The index of the value
 * @retval <0  - The value was not found.
 *****************************************************************************/

int bpsec_scvm_byNameIdxFind(sc_value_map map[], char *name)
{
    int idx = 0;

    while(map[idx].scValName != NULL)
    {
        if(strcmp(name, map[idx].scValName) == 0)
        {
          return idx;
        }
        idx++;
    }

    return -1;
}




/******************************************************************************
 * @brief Extract hex value from CBOR.
 *
 * @param[in]  wm     The memory partition where policy parameters live
 * @param[out] val    The SCI value being represented as a string
 * @param[in]  tlv    The TLV from the security block.
 *
 *
 * @note
 *  - It is assumed that the calling function has allocated space for the
 *    SCI value and pre-set the type of value being initialized.
 *
 * @retval   1 - Success
 * @retval  <0 - Error
 *****************************************************************************/

int bpsec_scvm_hexCborDecode(PsmPartition wm, sc_value *val, unsigned int len, uint8_t *buffer)
{
	int bytesUsed = 0;
	uvast uv_temp = 0;
	unsigned int tmp_len = len;
	unsigned char *cursor = NULL;
	unsigned char *dataPtr = NULL;

	BPSEC_DEBUG_PROC("(wm,"ADDR_FIELDSPEC",%d,"ADDR_FIELDSPEC")", (uaddr)val, len, (uaddr)buffer);

    CHKERR(val);


    /**
     * For debugging...
     *
     * char *tmp_str = bpsec_scutl_hexStrCvt(buffer, len);
     * BPSEC_DEBUG_INFO("CBOR buffer is %s.", tmp_str);
     * MRELEASE(tmp_str);
     */


    // Init... decode_byte_string will do a length check before... setting this length...
    val->scValLength = len;
    cursor = buffer;

 	uv_temp = -1;
    if(cbor_decode_byte_string(NULL, (uvast*) &uv_temp, &cursor, &tmp_len) < 1)
    {
    	BPSEC_DEBUG_ERR("Cannot determine SCI value length.", NULL);
    	return -1;
    }

    val->scValLength = (int) uv_temp;
    if((dataPtr = bpsec_scv_rawAlloc(wm, val, val->scValLength)) == NULL)
    {
    	BPSEC_DEBUG_ERR("Cannot allocate %d bytes.", val->scValLength);
    	return -1;
    }

    cursor = buffer;
    tmp_len = len;

 	uv_temp = val->scValLength;
    if((bytesUsed = cbor_decode_byte_string((unsigned char *) val->scRawValue.asPtr, &uv_temp, &cursor, &tmp_len)) < 1)
    {
    	BPSEC_DEBUG_ERR("Cannot decode byte string.", NULL);
    	bpsec_scv_clear(wm, val);
    	return -1;
    }

    BPSEC_DEBUG_PROC("Returning %d:", bytesUsed);
    return bytesUsed;
}



/******************************************************************************
 * @brief Encode hex value into CBOR.
 *
 * @param[in]  wm     The memory partition where policy parameters live
 * @param[out] val    The SCI value being represented as a string
 * @param[in]  tlv    The TLV from the security block.
 *
 *
 * @note
 *  - It is assumed that the calling function has allocated space for the
 *    SCI value and pre-set the type of value being initialized.
 *
 * @retval   1 - Success
 * @retval  <0 - Error
 *****************************************************************************/

uint8_t* bpsec_scvm_hexCborEncode(PsmPartition wm, sc_value *val, unsigned int *len)
{
	unsigned char *result = NULL;
	unsigned char *rawVal = NULL;

	BPSEC_DEBUG_PROC("(wm,"ADDR_FIELDSPEC","ADDR_FIELDSPEC")", (uaddr)val, (uaddr)len);

	CHKNULL(val);
	CHKNULL(len);

	BPSEC_DEBUG_INFO("Encoding value of length %d.", val->scValLength);

	*len = 0;

	// + 9 for additional info.
	// + 1 for type byte.

	if((result = MTAKE(val->scValLength + 10)) != NULL)
	{
		if((rawVal = bpsec_scv_rawGet(wm, val)) == NULL)
		{
			MRELEASE(result);
			result = NULL;
		}
		else
		{
		    unsigned char *cursor = result;
			*len = cbor_encode_byte_string(rawVal, val->scValLength, &cursor);
		}
	}
	return (uint8_t *)result;
}




/******************************************************************************
 * @brief Initialize an SCI value from a hex string
 *
 * @param[in]  wm     The memory partition where policy parameters live
 * @param[out] val    The SCi value being initialized.
 * @param[in]  len    Length of the value string.
 * @param[in]  value  String used to initialize the SCI value.
 *
 * @note
 *  - It is assumed that the calling function has allocated space for the
 *    SCI value and pre-set the type of value being initialized.
 *
 * @retval 0  - Success
 * @retval <0 - Error
 *****************************************************************************/
int   bpsec_scvm_hexStrDecode(PsmPartition wm, sc_value *val, unsigned int len, char *value)
{
    int isOdd = 0;
    int i = 0;
    int nib1 = 0;
    int nib2 = 0;
    char *cursor = NULL;
    int value_start = 0;

    /* Step 0: Sanity checks. */
    CHKERR(value);
    CHKERR(val);

    /* Step 1: Calculate Length */
    val->scValLength = strlen(value)/2;
    if((len % 2) != 0){
        isOdd = 1;
        val->scValLength += 1;
    }

    /* Skip over any '0x" preamble in the key */
    if((value[1] == 'x') || (value[1] == 'X'))
    {
        val->scValLength-= 1;
        value_start = 2;
    }

    /* Step 2: Allocate and populate. */
    if((cursor = (char *) bpsec_scv_rawAlloc(wm, val, val->scValLength)) == NULL)
    {
        bpsec_scv_clear(wm, val);
        return -1;
    }

    for(i = 0; i < len; i++)
    {
        if((isOdd) && (i == 0))
        {
            nib1 = 0;
            nib2 = bpsec_scutl_hexNibbleGet(value[0]);
        }
        else
        {
            int idx = (i*2) + value_start - isOdd;
            nib1 = bpsec_scutl_hexNibbleGet(value[idx]);
            nib2 = bpsec_scutl_hexNibbleGet(value[idx+1]);
        }

        if((nib1 == -1) || (nib2 == -1))
        {
            bpsec_scv_clear(wm, val);
            return -1;
        }

        cursor[i] = (nib1 << 4) | nib2;
    }

    return 0;
}




/******************************************************************************
 * @brief Generate a string representation of a hex SCI value.
 *
 * @param[in]  wm     The memory partition where policy parameters live
 * @param[in]  val    The SCi value being represented as a string
 *
 * @todo
 *  1. Logging
 *  2. testing. Lots of testing.
 *
 * @note
 *   - The string representation is allocated in an ION memory pool and MUST be
 *     freed by the calling function.
 *
 * @retval !NULL - The string representation
 * @retval  NULL - Error
 *****************************************************************************/
char* bpsec_scvm_hexStrEncode(PsmPartition wm, sc_value *val)
{
    char *data = bpsec_scv_rawGet(wm, val);
    char *cursor = NULL;
    char *result = NULL;
    int i = 0;

    int value = 0;

    CHKNULL(data);

    if((cursor = result = MTAKE((val->scValLength * 2) + 1)) == NULL)
    {
        return NULL;
    }

    for(i = 0; i < val->scValLength; i++)
    {
        value = (int) data[i];
        sprintf(cursor,"%02x", value);
        cursor += 2;
    }
    cursor = NULL;

    return result;
}



/******************************************************************************
 * @brief Generate an INT SCI value from the TLV grabbed from a security block.
 *
 * @param[in]  wm     The memory partition where policy parameters live
 * @param[out] val    The SCI value being represented as a string
 * @param[in]  tlv    The TLV from the security block.
 *
 * @todo
 *  1. Logging
 *  2. testing. Lots of testing.
 *
 * @note
 *  - It is assumed that the calling function has allocated space for the
 *    SCI value and pre-set the type of value being initialized.
 *  - The PsmPartition should never be used becasue block parameters are not
 *    policy parameters. May wind up removing this.
 *
 * @retval !NULL - The string representation
 * @retval  NULL - Error
 *****************************************************************************/
int   bpsec_scvm_intCborDecode(PsmPartition wm, sc_value *val, unsigned int len, uint8_t *value)
{
	int bytesUsed = 0;
	void *cursor = NULL;

    CHKERR(val);

    val->scValLength = sizeof(uvast);
    if((cursor = bpsec_scv_rawAlloc(wm, val, val->scValLength)) == NULL)
    {
    	BPSEC_DEBUG_ERR("Cannot allocate %d bytes.", val->scValLength);
    	return -1;
    }

    if((bytesUsed = cbor_decode_integer(cursor, CborAny, &value, &len)) < 1)
    {
    	BPSEC_DEBUG_ERR("Cannot decode byte string.", NULL);
    	bpsec_scv_clear(wm, val);
    	return -1;
    }

    return bytesUsed;
}



/******************************************************************************
 * @brief Encode int value into CBOR.
 *
 * @param[in]  wm     The memory partition where policy parameters live
 * @param[out] val    The SCI value being represented as a string
 * @param[in]  tlv    The TLV from the security block.
 *
 *
 * @note
 *  - It is assumed that the calling function has allocated space for the
 *    SCI value and pre-set the type of value being initialized.
 *
 * @retval  <0 - Error
 *****************************************************************************/
uint8_t* bpsec_scvm_intCborEncode(PsmPartition wm, sc_value *val, unsigned int *len)
{
	uint8_t *result = NULL;
	unsigned char *rawVal = NULL;

	CHKNULL(val);
	CHKNULL(len);

	*len = 0;

	if((result = MTAKE(sizeof(uvast) * 2)) != NULL)
	{
		if((rawVal = bpsec_scv_rawGet(wm, val)) == NULL)
		{
			MRELEASE(result);
			result = NULL;
		}
		else
		{
   			uvast value = 0;
   			unsigned char *cursor = result;
			memcpy(&value, rawVal, sizeof(uvast));

			/*
			 * cbor_encode_integer advances cursor by len,
			 * so you need to pass in a sacrificial pointer (cursor)
			 * instead of the actual pointer (result).
			 */
			*len = cbor_encode_integer(value, &cursor);
		}
	}

	return result;
}



/******************************************************************************
 * @brief Initialize an SCI value from a string containing an integer
 *
 * @param[in]  wm     The memory partition where policy parameters live
 * @param[out] val    The SCi value being initialized.
 * @param[in]  len    Length of the value string.
 * @param[in]  value  String used to initialize the SCI value.
 *
 * @todo
 *  1. Logging
 *  2. testing. Lots of testing.
 *
 * @note
 *  - It is assumed that the calling function has allocated space for the
 *    SCI value and pre-set the type of value being initialized.
 *
 * @retval 0  - Success
 * @retval <0 - Error
 *****************************************************************************/
int   bpsec_scvm_intStrDecode(PsmPartition wm, sc_value *val, unsigned int len, char *value)
{
    uvast *cursor = NULL;

    /* Step 0: Sanity checks. */
    CHKERR(val);
    CHKERR(value);

    /* Step 1: Allocate and populate. */
    if((cursor = (uvast *) bpsec_scv_rawAlloc(wm, val, sizeof(uvast))) == NULL)
    {
        bpsec_scv_clear(wm, val);
        return -1;
    }

    if(sscanf(value,UVAST_FIELDSPEC, cursor) != 1)
    {
    	bpsec_scv_clear(wm, val);
        return -1;
    }

    return 0;
}



/******************************************************************************
 * @brief Generate a string representation of a hex SCI value.
 *
 * @param[in]  wm     The memory partition where policy parameters live
 * @param[in]  val    The SCi value being represented as a string
 *
 * @todo
 *  1. Logging
 *  2. testing. Lots of testing.
 *
 * @note
 *   - The string representation is allocated in an ION memory pool and MUST be
 *     freed by the calling function.
 *
 * @retval !NULL - The string representation
 * @retval  NULL - Error
 *****************************************************************************/
char* bpsec_scvm_intStrEncode(PsmPartition wm, sc_value *val)
{
    static int max_length = 20;
    int *value = bpsec_scv_rawGet(wm, val);
    char *result = NULL;

    if(value == NULL)
    {
        return NULL;
    }

    if((result = MTAKE(max_length)) == NULL)
    {
        return NULL;
    }

    snprintf(result, max_length, "%d", *value);

    return result;
}



/******************************************************************************
 * @brief Generate an STR SCI value from the TLV grabbed from a security block.
 *
 * @param[in]  wm     The memory partition where policy parameters live
 * @param[out] val    The SCI value being represented as a string
 * @param[in]  tlv    The TLV from the security block.
 *
 * @todo
 *  1. Logging
 *  2. testing. Lots of testing.
 *
 * @note
 *  - It is assumed that the calling function has allocated space for the
 *    SCI value and pre-set the type of value being initialized.
 *  - The PsmPartition should never be used becasue block parameters are not
 *    policy parameters. May wind up removing this.
 *
 * @retval !NULL - The string representation
 * @retval  NULL - Error
 *****************************************************************************/
int   bpsec_scvm_strCborDecode(PsmPartition wm, sc_value *val, unsigned int len, uint8_t *buffer)
{
	int bytesUsed = 0;
	void *cursor = NULL;

    CHKERR(val);

    if(cbor_decode_text_string(NULL, (uvast *) &val->scValLength, &buffer, &len) < 1)
    {
    	BPSEC_DEBUG_ERR("Cannot determine SC value length.", NULL);
    	return -1;
    }

    if((cursor = bpsec_scv_rawAlloc(wm, val, val->scValLength)) == NULL)
    {
    	BPSEC_DEBUG_ERR("Cannot allocate %d bytes.", val->scValLength);
    	return -1;
    }

    if((bytesUsed = cbor_decode_text_string(cursor, (uvast *) &val->scValLength, &buffer, &len)) < 1)
    {
    	BPSEC_DEBUG_ERR("Cannot decode text string.", NULL);
    	bpsec_scv_clear(wm, val);
    	return -1;
    }

    return bytesUsed;
}



/******************************************************************************
 * @brief Encode string value into CBOR.
 *
 * @param[in]  wm     The memory partition where policy parameters live
 * @param[out] val    The SCI value being represented as a string
 * @param[in]  tlv    The TLV from the security block.
 *
 *
 * @note
 *  - It is assumed that the calling function has allocated space for the
 *    SCI value and pre-set the type of value being initialized.
 *
 * @retval   1 - Success
 * @retval  <0 - Error
 *****************************************************************************/
uint8_t* bpsec_scvm_strCborEncode(PsmPartition wm, sc_value *val, unsigned int *len)
{
	uint8_t *result = NULL;
	char *rawVal = NULL;

	CHKNULL(val);
	CHKNULL(len);

	*len = 0;

	if((result = MTAKE(sizeof(uvast) * 2)) != NULL)
	{
		if((rawVal = bpsec_scv_rawGet(wm, val)) == NULL)
		{
			MRELEASE(result);
			result = NULL;
		}
		else
		{
		    unsigned char *cursor = result;
			*len = cbor_encode_text_string(rawVal, val->scValLength, &cursor);
		}
	}
	return result;
}




/******************************************************************************
 * @brief Initialize an SCI value from a string
 *
 * @param[in]  wm     The memory partition where policy parameters live
 * @param[out] val    The SCi value being initialized.
 * @param[in]  len    Length of the value string.
 * @param[in]  value  String used to initialize the SCI value.
 *
 * @todo
 *  1. Logging
 *  2. testing. Lots of testing.
 *
 * @note
 *  - It is assumed that the calling function has allocated space for the
 *    SCI value and pre-set the type of value being initialized.
 *
 * @retval 0  - Success
 * @retval <0 - Error
 *****************************************************************************/
int   bpsec_scvm_strStrDecode(PsmPartition wm, sc_value *val, unsigned int len, char *value)
{
    char *cursor = NULL;

    /* Step 0: Sanity checks. */
    CHKERR(val);
    CHKERR(value);

    /* Step 1: Allocate and populate. */
    if((cursor = (char *) bpsec_scv_rawAlloc(wm, val, strlen(value) + 1)) == NULL)
    {
    	bpsec_scv_clear(wm, val);
        return -1;
    }

    memcpy(cursor, value, strlen(value) + 1);

    return 0;
}



/******************************************************************************
 * @brief Generate a string representation of a hex SCI value.
 *
 * @param[in]  wm     The memory partition where policy parameters live
 * @param[in]  val    The SCi value being represented as a string
 *
 * @todo
 *  1. Logging
 *  2. testing. Lots of testing.
 *
 * @note
 *   - The string representation is allocated in an ION memory pool and MUST be
 *     freed by the calling function.
 *
 * @retval !NULL - The string representation
 * @retval  NULL - Error
 *****************************************************************************/
char* bpsec_scvm_strStrEncode(PsmPartition wm, sc_value *val)
{
    char *value = bpsec_scv_rawGet(wm, val);
    char *result = NULL;

    if(value == NULL)
    {
        return NULL;
    }

    if((result = MTAKE(strlen(value)+1)) == NULL)
    {
        return NULL;
    }

    memset(result, 0, strlen(value)+1);
    memcpy(result, value, strlen(value));

    return result;
}

