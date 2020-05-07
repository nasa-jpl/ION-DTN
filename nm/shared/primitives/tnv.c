/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2018 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/
/*****************************************************************************
 **
 ** File Name: tdc.c
 **
 ** Description: This implements a Type-Name-Value instance and multiple
 **              encodings of TNV collections.
 **
 ** Notes:
 **  1. A TNV may not have as its type another TNV.
 **  2. ION AMP will never generate names associated with a TNV.
 **
 ** Assumptions:
 ** 1. A TNVC will have less than 256 elements.
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  06/24/15  J. P. Mayer    Initial Implementation.
 **  06/27/15  E. Birrane     Migrate from datalist to TDC. (Secure DTN - NASA: NNX14CS58P)
 **  08/28/18  E. Birrane     Update to latest spec and TNVs (JHU/APL)
 *****************************************************************************/

#include "platform.h"

#include "ion.h"
#include "../utils/utils.h"

#include "tnv.h"
#include "ari.h"
#include "ctrl.h"
#include "edd_var.h"
#include "expr.h"
#include "report.h"
#include "rules.h"
#include "table.h"


/* Local functions. */
static tnvc_t tnvc_deserialize_tvc(QCBORDecodeContext *it, size_t array_len, int *success);
static int tnvc_serialize_tvc(QCBOREncodeContext *encoder, tnvc_t *tnvc);



int  tnv_cb_comp(void *i1, void *i2)
{
	return tnv_compare((tnv_t*)i1, (tnv_t*)i2);
}

void *tnv_cb_copy(void *item)
{
	tnv_t *tnv = (tnv_t*) item;
	return tnv_copy_ptr(tnv);
}


void tnv_cb_del(void *item)
{
	tnv_release((tnv_t*)item, 1);
}



/******************************************************************************
 * Create a TNV that is the conversion of another TNV to a new type.
 *
 * \returns The created TNV, or NULL.
 *
 * \param[in] tnv  The TNC being converted.
 * \param[in] type The type to convert the TNV to.
 *
 * \note
 *	- Result is a deep copy of the source TNV and must be freed by the caller.
 *	- You can only cast numeric types.
 *****************************************************************************/

tnv_t* tnv_cast(tnv_t *tnv, amp_type_e type)
{
	tnv_t *result = NULL;
	int success = AMP_FAIL;

	/* Cannot cast non-numeric types or mapped parameters. */
	if( (type_is_numeric(tnv->type) == 0) ||
		(type_is_numeric(type) == 0) ||
		(TNV_IS_MAP(tnv->flags)))
	{
		AMP_DEBUG_ERR("tnv_cast","Bad parms.", NULL);
		return NULL;
	}

	switch(type)
	{
		case AMP_TYPE_INT:   result = tnv_from_int(tnv_to_int(*tnv, &success)); break;
		case AMP_TYPE_UINT:  result = tnv_from_uint(tnv_to_uint(*tnv, &success)); break;
		case AMP_TYPE_VAST:  result = tnv_from_vast(tnv_to_vast(*tnv, &success)); break;
		case AMP_TYPE_TV:    result = tnv_from_tv(tnv_to_uvast(*tnv, &success)); break;
		case AMP_TYPE_TS:
		case AMP_TYPE_UVAST:  result = tnv_from_uvast(tnv_to_uvast(*tnv, &success)); break;
		case AMP_TYPE_REAL32:  result = tnv_from_real32(tnv_to_real32(*tnv, &success)); break;
		case AMP_TYPE_REAL64:  result = tnv_from_real64(tnv_to_real64(*tnv, &success)); break;
		default:
			success = AMP_FAIL; break;
	}

	if(success != AMP_OK)
	{
		AMP_DEBUG_ERR("tnv_cast","Cannot cast from type %d to type %d.", tnv->type, type);
		result = NULL;
	}

	return result;
}



/******************************************************************************
 * Perform a value comparison between two TNVs.
 *
 * \returns -1 on error. 0 if equivalent. 1 if not equivalent.
 *
 * \param[in]   t1	The first TNV to compare
 * \param[in]   t2  The second TNV to compare.
 *
 * \note
 *   - This is only for similarity, not sorting. No concept of < or >.
 *****************************************************************************/

int tnv_compare(tnv_t *v1, tnv_t *v2)
{
	int diff = 0;

	/* Cannot compare NULL values. */
	if((v1 == NULL) || (v2 == NULL))
	{
		return -1;
	}

	/* Same object? */
	if(v1 == v2)
	{
		return 0;
	}

	/* Different types. */
	if(v1->type != v2->type)
	{
		return 1;
	}

	/* semantic comparison. */
	switch(v1->type)
	{
		case AMP_TYPE_CNST:
		case AMP_TYPE_EDD:
		case AMP_TYPE_ARI:
		case AMP_TYPE_LIT:     diff = ari_compare((ari_t*)v1->value.as_ptr, (ari_t*)v2->value.as_ptr, 0);       break;
		case AMP_TYPE_CTRL:    diff = ctrl_cb_comp_fn((ctrl_t*)v1->value.as_ptr, (ctrl_t*)v2->value.as_ptr); break;
		case AMP_TYPE_OPER:    diff = op_cb_comp_fn((op_t*)v1->value.as_ptr, (op_t*)v2->value.as_ptr);       break;
		case AMP_TYPE_RPT:     diff = rpt_cb_comp_fn((rpt_t*)v1->value.as_ptr, (rpt_t*)v2->value.as_ptr);    break;
		case AMP_TYPE_RPTTPL:  diff = rpttpl_cb_comp_fn((rpttpl_t*)v1->value.as_ptr, (rpttpl_t*)v2->value.as_ptr); break;
		case AMP_TYPE_TBR:
		case AMP_TYPE_SBR:     diff = rule_cb_comp_fn(v1->value.as_ptr, v2->value.as_ptr);                   break;
		case AMP_TYPE_VAR:     diff = var_cb_comp_fn((var_t*)v1->value.as_ptr, (var_t*)v2->value.as_ptr);    break;
		case AMP_TYPE_BYTESTR: diff = blob_compare((blob_t*)v1->value.as_ptr, (blob_t*)v2->value.as_ptr);    break;
		case AMP_TYPE_STR:     diff = strcmp((char*)v1->value.as_ptr, (char*)v2->value.as_ptr);              break;
		case AMP_TYPE_BOOL:
		case AMP_TYPE_BYTE:    diff = (v1->value.as_byte == v2->value.as_byte) ? 0 : 1;                      break;
		case AMP_TYPE_INT:     diff = (v1->value.as_int == v2->value.as_int) ? 0 : 1;                        break;
		case AMP_TYPE_UINT:    diff = (v1->value.as_uint == v2->value.as_uint) ? 0 : 1;                      break;
		case AMP_TYPE_VAST:    diff = (v1->value.as_vast == v2->value.as_vast) ? 0 : 1;                      break;
		case AMP_TYPE_TV:
		case AMP_TYPE_TS:
		case AMP_TYPE_UVAST:   diff = (v1->value.as_uvast == v2->value.as_uvast) ? 0 : 1;                    break;
		case AMP_TYPE_REAL32:  diff = (v1->value.as_real32 == v2->value.as_real32) ? 0 : 1;                  break;
		case AMP_TYPE_REAL64:  diff = (v1->value.as_real64 == v2->value.as_real64) ? 0 : 1;                  break;

		case AMP_TYPE_MAC:     diff = macdef_cb_comp_fn(v1->value.as_ptr, v2->value.as_ptr);                 break;
		case AMP_TYPE_TNVC:    diff = tnvc_cb_comp(v1->value.as_ptr, v2->value.as_ptr);                      break;

		case AMP_TYPE_EXPR:
		case AMP_TYPE_TBL:
		case AMP_TYPE_TBLT:
		case AMP_TYPE_AC:     // diff = ac_compare((ac_t*)v1->value.as_ptr, (ac_t*)v2->value.as_ptr);          break;
		default:
			AMP_DEBUG_WARN("tnv_compare", "Unsupported compare for type %d", v1->type);
			diff =  AMP_SYSERR; break;
	}

	return diff;
}



/******************************************************************************
 * Copies a TNV value.
 *
 * \returns A deep copy of the TNV.
 *
 * \param[in]   val   	 The TNV being copied.
 * \param[out]  success  AMP status code for the copy.
 *
 * \note
 *	- Result must be freed via tnv_release(<item>,0);
 *****************************************************************************/

tnv_t tnv_copy(tnv_t val, int *success)
{
	tnv_t result;

	*success = AMP_OK;

	/* Shallow copy everything over. */
	memcpy(&result, &val, sizeof(val));

	/* Assume we are going to ALLOC. */
	TNV_SET_ALLOC(result.flags);

	/* If this is not a basic type, we need to deep copy. */
	switch(result.type)
	{
		case AMP_TYPE_CNST:
		case AMP_TYPE_EDD:
		case AMP_TYPE_LIT:
		case AMP_TYPE_ARI:     result.value.as_ptr = ari_copy_ptr((ari_t*)result.value.as_ptr);  		break;
		case AMP_TYPE_CTRL:    result.value.as_ptr = ctrl_copy_ptr((ctrl_t*) result.value.as_ptr);   	break;
		case AMP_TYPE_MAC:     result.value.as_ptr = macdef_copy_ptr((macdef_t*) result.value.as_ptr);  break;
		case AMP_TYPE_AC:      result.value.as_ptr = ac_copy_ptr((ac_t*) result.value.as_ptr);   		break;
		case AMP_TYPE_OPER:    result.value.as_ptr = op_copy_ptr((op_t*) result.value.as_ptr);     	    break;
		case AMP_TYPE_RPT:     result.value.as_ptr = rpt_copy_ptr((rpt_t*) result.value.as_ptr);    	break;
		case AMP_TYPE_RPTTPL:  result.value.as_ptr = rpttpl_copy_ptr((rpttpl_t*) result.value.as_ptr);  break;
		case AMP_TYPE_TBR:
		case AMP_TYPE_SBR:     result.value.as_ptr = rule_copy_ptr((rule_t*) result.value.as_ptr);      break;
		case AMP_TYPE_TBL:     result.value.as_ptr = tbl_copy_ptr((tbl_t*) result.value.as_ptr);        break;
		case AMP_TYPE_TBLT:    result.value.as_ptr = tblt_copy_ptr((tblt_t*) result.value.as_ptr);      break;
		case AMP_TYPE_VAR:     result.value.as_ptr = var_copy_ptr((var_t*) result.value.as_ptr);        break;
		case AMP_TYPE_EXPR:    result.value.as_ptr = expr_copy_ptr((expr_t*) result.value.as_ptr);      break;
		case AMP_TYPE_BYTESTR: result.value.as_ptr = blob_copy_ptr((blob_t*) result.value.as_ptr);      break;
		case AMP_TYPE_TNVC:    result.value.as_ptr = tnvc_copy((tnvc_t*)result.value.as_ptr);           break;

		case AMP_TYPE_STR:
		{
			if(result.value.as_ptr != NULL)
			{
				char *tmp = NULL;
				size_t len = strlen((char*)result.value.as_ptr);
				if((tmp = STAKE(len+1)) != NULL)
				{
					memcpy(tmp, result.value.as_ptr, len);
					tmp[len]=0; // Ensure NULL-termination
					result.value.as_ptr = tmp;
				}
			}
		};
		break;

		/* Note failure on unknown or invalid type. */
		case AMP_TYPE_TNV:
		case AMP_TYPE_UNK:
			*success = AMP_FAIL;
			TNV_CLEAR_ALLOC(result.flags);
			break;

		/* If this wasn't a deep copy, clear ALLOC flag. */
		default:
			TNV_CLEAR_ALLOC(result.flags);
			break;
	};

	return result;
}



/******************************************************************************
 * Create a TNV pointer that is the conversion of another TNV to a new type.
 *
 * \returns The created TNV, or NULL.
 *
 * \param[in] tnv  The TNC being copied.
 *
 * \note
 *	- Result must be freed via tnv_release(<item>,1);
 *****************************************************************************/

tnv_t *tnv_copy_ptr(tnv_t *val)
{
	tnv_t *result = NULL;
	int success = AMP_FAIL;

	if(val == NULL)
	{
		return NULL;
	}

	result = tnv_create();
	CHKNULL(result);

	*result = tnv_copy(*val, &success);
	if(success != AMP_OK)
	{
		SRELEASE(result);
		result = NULL;
	}

	return result;
}



/******************************************************************************
 * Allocate new, empty TNV.
 *
 * \returns The created TNV, or NULL.
 *****************************************************************************/

tnv_t *tnv_create()
{
	tnv_t *result = STAKE(sizeof(tnv_t));
	CHKNULL(result);

	tnv_init(result, AMP_TYPE_UNK);
	return result;
}



/******************************************************************************
 * Deserializes a TNV value from a CBOR value.
 *
 * \returns The created TNV, or error code.
 *
 * \param[in,out] it       The current CBOR value.
 * \param[out]    success  AMP status code.
 *
 * \note
 *	- The resultant TNV will have a type of AMP_TYPE_UNK on error.
 *****************************************************************************/

tnv_t tnv_deserialize(QCBORDecodeContext *it, int *success)
{
    uint8_t type = 0;
    size_t array_len = 0;
    QCBORItem item;
    QCBORError err;
    tnv_t result;

    AMP_DEBUG_ENTRY("tnv_deserialize","("ADDR_FIELDSPEC","ADDR_FIELDSPEC")", (uaddr)it, (uaddr)success);

    tnv_init(&result, AMP_TYPE_UNK);

    CHKUSR(it, result);
    CHKUSR(success, result);

    /* This should be an array... */
    err = QCBORDecode_GetNext(it, &item);
    if (err != QCBOR_SUCCESS || item.uDataType != QCBOR_TYPE_ARRAY)
    {
    	AMP_DEBUG_ERR("tnv_deserialize","Invalid Array", NULL);
    	*success = AMP_FAIL;
        return result;
	}
    
    /* Step 1: Grab the TNV and Flags byte. */
    cut_get_cbor_numeric(it, AMP_TYPE_BYTE, &type);

    result.type = (type & 0x7F);
    array_len--;

    /* Step 2: Deserialize name, if we have a name. */
    if(type & 0x80)
    {
    	char *name = cut_get_cbor_str(it, success);
    	if(name == NULL)
    	{
    		AMP_DEBUG_ERR("tnv_deserialize","Error getting name.", NULL);
    		result.type = AMP_TYPE_UNK;
    		*success = AMP_FAIL;
    		return result;
    	}
    	array_len--;
    }

    if(array_len > 0)
    {
    	if(tnv_deserialize_val_by_type(it, &result) != AMP_OK)
    	{
    		AMP_DEBUG_ERR("tnv_deserialize","Deserialize error: AMP %d", *success);
    		tnv_release(&result,0);
    		result.type = AMP_TYPE_UNK;
    		return result;
		}
    }

    *success = AMP_OK;
	return result;
}



/******************************************************************************
 * Deserializes a TNV pointer value from a CBOR value.
 *
 * \returns The created TNV, or NULL
 *
 * \param[in,out] it       The current CBOR value.
 * \param[out]    success  AMP status code.
 *****************************************************************************/

tnv_t *tnv_deserialize_ptr(QCBORDecodeContext *it, int *success)
{
	tnv_t *result = tnv_create();
	*success = AMP_FAIL;

	CHKNULL(result);

	*result = tnv_deserialize(it, success);
	if(*success != AMP_OK)
	{
		SRELEASE(result);
		result = NULL;
	}

	return result;
}

tnv_t* tnv_deserialize_raw(blob_t *data, int *success)
{
	QCBORDecodeContext it;

	if((data == NULL) || (success == NULL))
	{
		return NULL;
	}
    *success = AMP_FAIL;

    QCBORDecode_Init(&it,
                     (UsefulBufC){data->value,data->length},
                     QCBOR_DECODE_MODE_NORMAL);

	tnv_t *tmp = tnv_deserialize_ptr(&it, success);

    // Verify Decoding Completed Successfully
    cut_decode_finish(&it);

    return tmp;
}


/******************************************************************************
 * Deserializes only a TNV value from a CBOR value.
 *
 * \returns AMP Status Code.
 *
 * \param[in,out]  it       The current CBOR value.
 * \param[in,out]  result   The TNV updated with the deserialized value.
 *
 * \note
 *   1. It is assumed that that result has been allocated and type set.
 *   2. This function only updates the TNV's value, not name or type.
 *****************************************************************************/

int tnv_deserialize_val_by_type(QCBORDecodeContext *it, tnv_t *result)
{
	int success = AMP_FAIL;
	int can_alloc = 0;

	if((it == NULL) || (result == NULL))
	{
		return AMP_FAIL;
	}

	TNV_CLEAR_ALLOC(result->flags);

	switch(result->type)
	{
		/* AMM Objects and Compound Objects.*/
	    case AMP_TYPE_EDD:
	    case AMP_TYPE_CNST:
	    case AMP_TYPE_ARI:
	    case AMP_TYPE_LIT:    result->value.as_ptr = ari_deserialize_ptr(it, &success);     can_alloc = 1; break;
	    case AMP_TYPE_CTRL:   result->value.as_ptr = ctrl_deserialize_ptr(it, &success);    can_alloc = 1; break;
	   	case AMP_TYPE_MAC:    result->value.as_ptr = macdef_deserialize_ptr(it, &success);  can_alloc = 1; break;
	   	case AMP_TYPE_RPT:    result->value.as_ptr = rpt_deserialize_ptr(it, &success);     can_alloc = 1; break;
	   	case AMP_TYPE_RPTTPL: result->value.as_ptr = rpttpl_deserialize_ptr(it, &success);  can_alloc = 1; break;
	   	case AMP_TYPE_TBL:    result->value.as_ptr = tbl_deserialize_ptr(it, &success);     can_alloc = 1; break;
	   	case AMP_TYPE_TBR:
	   	case AMP_TYPE_SBR:    result->value.as_ptr = rule_deserialize_ptr(it, &success);    can_alloc = 1; break;
	   	case AMP_TYPE_VAR:    result->value.as_ptr = var_deserialize_ptr(it, &success);     can_alloc = 1; break;
	   	case AMP_TYPE_AC:     result->value.as_ptr = ac_deserialize_ptr(it, &success);      can_alloc = 1; break;
	   	case AMP_TYPE_EXPR:   result->value.as_ptr = expr_deserialize_ptr(it, &success);    can_alloc = 1; break;
	   	case AMP_TYPE_BYTESTR:result->value.as_ptr = blob_deserialize_ptr(it, &success);    can_alloc = 1; break;
	   	case AMP_TYPE_TNVC:   result->value.as_ptr = tnvc_deserialize_ptr(it, &success);    can_alloc = 1; break;

	   	/* Primitive Types and Derived Types */
	   	case AMP_TYPE_STR:    result->value.as_ptr = cut_get_cbor_str(it, &success);        can_alloc = 1; break;
	   	case AMP_TYPE_BOOL:
	   	case AMP_TYPE_BYTE:   success = cut_get_cbor_numeric(it, result->type, (uint8_t*) &(result->value.as_byte));  break;
	   	case AMP_TYPE_INT:    success = cut_get_cbor_numeric(it, result->type, (int32_t*) &(result->value.as_int));   break;
	   	case AMP_TYPE_UINT:   success = cut_get_cbor_numeric(it, result->type, (uint32_t*) &(result->value.as_uint)); break;
	   	case AMP_TYPE_VAST:   success = cut_get_cbor_numeric(it, result->type, (vast*) &(result->value.as_vast));     break;
	   	case AMP_TYPE_TV:
	   	case AMP_TYPE_TS:
	   	case AMP_TYPE_UVAST:  success = cut_get_cbor_numeric(it, result->type, (uvast*) &(result->value.as_uvast));   break;
	   	case AMP_TYPE_REAL32: success = cut_get_cbor_numeric(it, result->type, (float*) &(result->value.as_real32));  break;
	   	case AMP_TYPE_REAL64: success = cut_get_cbor_numeric(it, result->type, (double*) &(result->value.as_real64)); break;

	   	case AMP_TYPE_TBLT:
	   	case AMP_TYPE_OPER:
	   	case AMP_TYPE_TNV:
	   	case AMP_TYPE_UNK:
	   	default:
	   		AMP_DEBUG_ERR("tnv_deserialize_val_by_type","Cannot deserialize TNV of type %d", result->type);
	   		break;
	}

	if((success == AMP_OK) && (can_alloc == 1))
	{
		TNV_SET_ALLOC(result->flags);
	}

	return success;
}

int tnv_deserialize_val_raw(blob_t *data, tnv_t *result)
{
	QCBORDecodeContext it;

	if((data == NULL) || (result == NULL))
	{
		return AMP_FAIL;
	}

	QCBORDecode_Init(&it,
					 (UsefulBufC){data->value,data->length},
					 QCBOR_DECODE_MODE_NORMAL);

	int tmp = tnv_deserialize_val_by_type(&it, result);

	// Verify Decoding Completed Successfully
	cut_decode_finish(&it);

	return tmp;
}



/******************************************************************************
 * Create new TNV from Boolean value.
 *
 * \returns New TNV, or NULL
 *
 * \param[in]  val  The value for the new TNV.
 *
 * \note
 *   1. Result must be freed with tnv_release(<item>, 1);
 *****************************************************************************/

tnv_t*  tnv_from_bool(uint8_t val)
{
	tnv_t *result = tnv_create();
	CHKNULL(result);
	tnv_init(result, AMP_TYPE_BOOL);
	result->value.as_byte = val;
	return result;
}



/******************************************************************************
 * Create new TNV from BLOB value.
 *
 * \returns New TNV, or NULL
 *
 * \param[in]  val  The value for the new TNV.
 *
 * \note
 *   1. Result must be freed with tnv_release(<item>, 1);
 *****************************************************************************/

tnv_t*  tnv_from_blob(blob_t *val)
{
	tnv_t *result = NULL;
	CHKNULL(val);
	result = tnv_create();
	CHKNULL(result);
	tnv_init(result, AMP_TYPE_BYTESTR);
	result->value.as_ptr = val;
	TNV_SET_ALLOC(result->flags);
	return result;
}



/******************************************************************************
 * Create new TNV from Byte value.
 *
 * \returns New TNV, or NULL
 *
 * \param[in]  val  The value for the new TNV.
 *
 * \note
 *   1. Result must be freed with tnv_release(<item>, 1);
 *****************************************************************************/

tnv_t*  tnv_from_byte(uint8_t val)
{
	tnv_t *result = tnv_create();
	CHKNULL(result);
	tnv_init(result, AMP_TYPE_BYTE);
	result->value.as_byte = val;
	return result;

}



/******************************************************************************
 * Create new TNV from INT value.
 *
 * \returns New TNV, or NULL
 *
 * \param[in]  val  The value for the new TNV.
 *
 * \note
 *   1. Result must be freed with tnv_release(<item>, 1);
 *****************************************************************************/

tnv_t* tnv_from_int(int32_t val)
{
	tnv_t *result = tnv_create();
	CHKNULL(result);
	tnv_init(result, AMP_TYPE_INT);
	result->value.as_int = val;
	return result;
}




/******************************************************************************
 * Create new TNV that is a mapped value.
 *
 * \returns New TNV, or NULL
 *
 * \param[in]  type    The type of the mapped value.
 * \param[in]  map_idx The index of the container's mapped value.
 *
 * \note
 *   1. Result must be freed with tnv_release(<item>, 1);
 *****************************************************************************/

tnv_t* tnv_from_map(amp_type_e type, uint8_t map_idx)
{
	tnv_t *result = tnv_create();
	CHKNULL(result);

	tnv_init(result, type);
	TNV_SET_MAP(result->flags);

	result->value.as_uint = map_idx;
	return result;
}



/******************************************************************************
 * Create new TNV that holds an allocated object.
 *
 * \returns New TNV, or NULL
 *
 * \param[in]  type    The type of the object.
 * \param[in]  item    The allocated object.
 *
 * \note
 *   1. Result must be freed with tnv_release(<item>, 1);
 *   2. The caller MUST NOT release the given object after this call.
 *****************************************************************************/

tnv_t* tnv_from_obj(amp_type_e type, void *item)
{
	tnv_t *result = tnv_create();
	CHKNULL(result);

	tnv_init(result, type);
	TNV_SET_ALLOC(result->flags);

	result->value.as_ptr = item;
	return result;
}


/******************************************************************************
 * Create new TNV from REAL32 value.
 *
 * \returns New TNV, or NULL
 *
 * \param[in]  val  The value for the new TNV.
 *
 * \note
 *   1. Result must be freed with tnv_release(<item>, 1);
 *****************************************************************************/

tnv_t* tnv_from_real32(float val)
{
	tnv_t *result = tnv_create();
	CHKNULL(result);
	tnv_init(result, AMP_TYPE_REAL32);
	result->value.as_real32 = val;
	return result;
}



/******************************************************************************
 * Create new TNV from REAL64 value.
 *
 * \returns New TNV, or NULL
 *
 * \param[in]  val  The value for the new TNV.
 *
 * \note
 *   1. Result must be freed with tnv_release(<item>, 1);
 *****************************************************************************/

tnv_t* tnv_from_real64(double val)
{
	tnv_t *result = tnv_create();
	CHKNULL(result);
	tnv_init(result, AMP_TYPE_REAL64);
	result->value.as_real64 = val;
	return result;
}



/******************************************************************************
 * Create new TNV from STR value.
 *
 * \returns New TNV, or NULL
 *
 * \param[in]  str  The value for the new TNV.
 *
 * \note
 *   1. Result must be freed with tnv_release(<item>, 1);
 *****************************************************************************/

tnv_t*  tnv_from_str(char *str)
{
	tnv_t* result = NULL;
	uint32_t len = 0;

	CHKNULL(str);
	result = tnv_create();
	CHKNULL(result);

	result->type = AMP_TYPE_STR;
	len = strlen(str);

	if((result->value.as_ptr = STAKE(len+1)) == NULL)
	{
		tnv_release(result, 1);
		result = NULL;
	}
	else
	{
		TNV_SET_ALLOC(result->flags);
		memcpy(result->value.as_ptr, str, len);
	}

	return result;
}



/******************************************************************************
 * Create new TNV from UINT value.
 *
 * \returns New TNV, or NULL
 *
 * \param[in]  str  The value for the new TNV.
 *
 * \note
 *   1. Result must be freed with tnv_release(<item>, 1);
 *****************************************************************************/

tnv_t*  tnv_from_uint(uint32_t val)
{
	tnv_t *result = tnv_create();
	CHKNULL(result);
	tnv_init(result, AMP_TYPE_UINT);
	result->value.as_uint = val;
	return result;
}



/******************************************************************************
 * Create new TNV from UVAST value.
 *
 * \returns New TNV, or NULL
 *
 * \param[in]  str  The value for the new TNV.
 *
 * \note
 *   1. Result must be freed with tnv_release(<item>, 1);
 *****************************************************************************/

tnv_t*  tnv_from_uvast(uvast val)
{
	tnv_t *result = tnv_create();
	CHKNULL(result);
	tnv_init(result, AMP_TYPE_UVAST);
	result->value.as_uvast = val;
	return result;
}



/******************************************************************************
 * Create new TNV from time_t value.
 *
 * \returns New TNV, or NULL
 *
 * \param[in]  str  The value for the new TNV.
 *
 * \note
 *   1. Result must be freed with tnv_release(<item>, 1);
 *****************************************************************************/

tnv_t*  tnv_from_tv(time_t val)
{
	tnv_t *result = tnv_create();
	CHKNULL(result);
	tnv_init(result, AMP_TYPE_TV);
	result->value.as_uvast = val;
	return result;
}



/******************************************************************************
 * Create new TNV from VAST value.
 *
 * \returns New TNV, or NULL
 *
 * \param[in]  str  The value for the new TNV.
 *
 * \note
 *   1. Result must be freed with tnv_release(<item>, 1);
 *****************************************************************************/

tnv_t*  tnv_from_vast(vast val)
{
	tnv_t *result = tnv_create();
	CHKNULL(result);
	tnv_init(result, AMP_TYPE_VAST);
	result->value.as_vast = val;
	return result;
}



/******************************************************************************
 * Initialize a TNV value with a given type.
 *
 * \param[out]  val   The TNV being initialized.
 * \param[in]   type  The type for this TNV.
 *
 * \note
 *   1. It is assumed that val exists and will be zeroed out.
 *****************************************************************************/

void tnv_init(tnv_t *val, amp_type_e type)
{
	if(val != NULL)
	{
		memset(val, 0, sizeof(tnv_t));
		val->type = type;
	}
}



/******************************************************************************
 * Write a TNV value to a CBOR encoder.
 *
 * \param[in,out] encoder  The CBOR Encoder.
 * \param[in]     item     The item being encoded.
 *
 * \note
 *   1. This may be called with an empty encoder to determine the space needed
 *      to hold the serialied TNV. Therefore, CborErrorOutOfMemory must not
 *      be considered an error.
 *   2. This implementation will never serialize TNVs with names.
 *   3. This implementation will always generate both types and values.
 *   4. The item pointer must be of type tnv_t *.
 *****************************************************************************/

int tnv_serialize(QCBOREncodeContext *encoder, void *item)
{
	tnv_t *tnv = (tnv_t *) item;
	int err;

	CHKUSR(encoder, AMP_FAIL);
	CHKUSR(tnv, AMP_FAIL);

	/* Step 1: Create the Array Encoder. */
	QCBOREncode_OpenArray(encoder);

	/* Step 2: Write the Type/Flag Byte. */
	err = cut_enc_byte(encoder, (tnv->type & 0x7F));
	if(err != AMP_OK)
	{
		AMP_DEBUG_ERR("tnv_serialize_helper","Cbor Error: %d", err);
		return err;
	}

	err = tnv_serialize_value(encoder,tnv);
	if(err != AMP_OK)
	{
		AMP_DEBUG_ERR("tnv_serialize_helper","Error: %d", err);
		return err;
	}

	QCBOREncode_CloseArray(encoder);
	return AMP_OK;
}



/******************************************************************************
 * Encode only the value of a TNV to a CBOR encoder.
 *
 * \param[in,out] encoder  The CBOR Encoder.
 * \param[in]     item     The item being encoded.
 *
 * \note
 *   1. This may be called with an empty encoder to determine the space needed
 *      to hold the serialied TNV. Therefore, CborErrorOutOfMemory must not
 *      be considered an error.
 *   2. The item pointer must be of type tnv_t *.
 *****************************************************************************/

int tnv_serialize_value(QCBOREncodeContext *encoder, void *item)
{
	int err = AMP_OK;
	tnv_t *tnv = (tnv_t *) item;

	CHKUSR(tnv, AMP_FAIL);

	/* Step 3: Encode the value. */
	switch(tnv->type)
	{
	    /* AMM Object Types. */
		case AMP_TYPE_EDD:
	    case AMP_TYPE_CNST:
	    case AMP_TYPE_ARI:
	    case AMP_TYPE_LIT:    err = ari_serialize(encoder, (ari_t*) tnv->value.as_ptr);       break;
	    case AMP_TYPE_CTRL:   err = ctrl_serialize(encoder, (ctrl_t*) tnv->value.as_ptr);     break;
		case AMP_TYPE_MAC:    err = macdef_serialize(encoder, (macdef_t*)tnv->value.as_ptr);  break;
		case AMP_TYPE_RPT:    err = rpt_serialize(encoder, (rpt_t*)tnv->value.as_ptr);        break;
		case AMP_TYPE_RPTTPL: err = rpttpl_serialize(encoder, (rpttpl_t*)tnv->value.as_ptr);  break;
		case AMP_TYPE_TBR:
		case AMP_TYPE_SBR:    err = rule_serialize(encoder, (rule_t*)tnv->value.as_ptr);      break;
		case AMP_TYPE_TBL:    err = tbl_serialize(encoder, (tbl_t*)tnv->value.as_ptr);        break;
		case AMP_TYPE_VAR:    err = var_serialize(encoder, (var_t*)tnv->value.as_ptr);        break;
		case AMP_TYPE_AC:     err = ac_serialize(encoder, (ac_t*)tnv->value.as_ptr);          break;
		case AMP_TYPE_EXPR:   err = expr_serialize(encoder, (expr_t*)tnv->value.as_ptr);      break;
		case AMP_TYPE_BYTESTR:err = blob_serialize(encoder, (blob_t*)tnv->value.as_ptr);      break;
		case AMP_TYPE_TNVC:   err = tnvc_serialize(encoder, (tnvc_t*)tnv->value.as_ptr);      break;
		/* Primitive Types and Derived Types */
		case AMP_TYPE_STR:    err = (tnv->value.as_ptr == NULL) ? AMP_FAIL : cut_char_serialize(encoder, (char*) tnv->value.as_ptr); break;
		case AMP_TYPE_BOOL:
		case AMP_TYPE_BYTE:   err = cut_enc_byte(encoder, tnv->value.as_byte);                break;
		case AMP_TYPE_INT:    QCBOREncode_AddInt64(encoder, (int64_t) tnv->value.as_int);     break;
		case AMP_TYPE_UINT:   QCBOREncode_AddUInt64(encoder, (uint64_t) tnv->value.as_uint);  break;
		case AMP_TYPE_VAST:   QCBOREncode_AddInt64(encoder, tnv->value.as_vast);              break;
		case AMP_TYPE_TV:
		case AMP_TYPE_TS:
		case AMP_TYPE_UVAST:  QCBOREncode_AddInt64(encoder, tnv->value.as_uvast);             break;

           /* NOTE: QCBOR Only provides one method for encoding floating point, but will automatically
            * pack it into the smallest form that does not result in a loss of precision.
            */
		case AMP_TYPE_REAL32: QCBOREncode_AddDouble(encoder, tnv->value.as_real32);           break;

		case AMP_TYPE_REAL64: QCBOREncode_AddDouble(encoder, tnv->value.as_real64);           break;
		/* Unsupported Types. */
		case AMP_TYPE_TBLT:
		case AMP_TYPE_TNV:
		case AMP_TYPE_OPER:
		default:
			/* Invalid type. */
			printf("DEBUG: Invalid AMP_TYPE: %i\n", tnv->type);
           err = AMP_FAIL;
	}

	return err;
}



/******************************************************************************
 * Encode the value of a TNV as a BLOB.
 *
 * \param[in] tnv  The TNV being encoded.
 *
 * \note
 *   1. The blob must be freed with blob_release(<item>, 1);
 *****************************************************************************/

blob_t* tnv_serialize_value_wrapper(tnv_t *tnv)
{
	return cut_serialize_wrapper(TNV_DEFAULT_ENC_SIZE, tnv, (cut_enc_fn)tnv_serialize_value);
}



/******************************************************************************
 * Encode a TNV as a BLOB.
 *
 * \param[in] tnv  The TNV being encoded.
 *
 * \note
 *   1. The blob must be freed with blob_release(<item>, 1);
 *****************************************************************************/

blob_t* tnv_serialize_wrapper(tnv_t *tnv)
{
	return cut_serialize_wrapper(TNV_DEFAULT_ENC_SIZE, tnv, (cut_enc_fn)tnv_serialize);
}



/******************************************************************************
 * Establish a TNV as a mapped value.
 *
 * \returns AMP status code.
 *
 * \param[in,out] tnv      The TNV whose value is mapped to another value.
 * \param[in]     parm_idx The index of the containing item holding the value.
 *
 * \note
 *   1. If the existing TNV is an object, it will be released before mapping.
 *****************************************************************************/

int tnv_set_map(tnv_t *tnv, uint32_t parm_idx)
{
	CHKUSR(tnv, AMP_FAIL);

	if(TNV_IS_MAP(tnv->flags) == 0)
	{
		if(TNV_IS_ALLOC(tnv->flags))
		{
			tnv_release(tnv, 0);
		}
		TNV_SET_MAP(tnv->flags);
	}

	tnv->value.as_uint = parm_idx;
	return AMP_OK;
}



/******************************************************************************
 * Release the resources associated with a TNV.
 *
 * \param[in,out] tnv      The TNV being released.
 * \param[in]     destroy  Whether to free the TNV container.
 *
 * \note
 *   1. Destroy should be set to 1 if the TNV container was allocated in a
 *      memory pool and 0 if allocated on the stack.
 *****************************************************************************/

void tnv_release(tnv_t *val, int destroy)
{
	if(val == NULL)
	{
		return;
	}

	/* If allocated and we have a pointer. */
	if(TNV_IS_ALLOC(val->flags) && (val->value.as_ptr != NULL))
	{
		switch(val->type)
		{
			case AMP_TYPE_CNST:
			case AMP_TYPE_EDD:
			case AMP_TYPE_ARI:
			case AMP_TYPE_LIT:     ari_release((ari_t*)val->value.as_ptr, 1);        break;
			case AMP_TYPE_CTRL:    ctrl_release((ctrl_t*) val->value.as_ptr, 1);     break;
			case AMP_TYPE_MAC:     macdef_release((macdef_t*) val->value.as_ptr, 1); break;
			case AMP_TYPE_AC:      ac_release((ac_t*) val->value.as_ptr, 1);         break;
			case AMP_TYPE_OPER:    op_release((op_t*) val->value.as_ptr, 1);         break;
			case AMP_TYPE_RPT:     rpt_release((rpt_t*) val->value.as_ptr, 1);       break;
			case AMP_TYPE_RPTTPL:  rpttpl_release((rpttpl_t*) val->value.as_ptr, 1); break;
			case AMP_TYPE_TBR:
			case AMP_TYPE_SBR:     rule_release((rule_t*) val->value.as_ptr, 1);     break;
			case AMP_TYPE_TBL:     tbl_release((tbl_t*) val->value.as_ptr, 1);       break;
			case AMP_TYPE_TBLT:    tblt_release((tblt_t*) val->value.as_ptr, 1);     break;
			case AMP_TYPE_VAR:     var_release((var_t*) val->value.as_ptr, 1);       break;
			case AMP_TYPE_EXPR:    expr_release((expr_t*) val->value.as_ptr, 1);     break;
			case AMP_TYPE_BYTESTR: blob_release((blob_t*) val->value.as_ptr, 1);     break;
			case AMP_TYPE_STR:	   SRELEASE((char*)val->value.as_ptr);               break;
			case AMP_TYPE_TNVC:    tnvc_release((tnvc_t*) val->value.as_ptr, 1);     break;
			default: break;
		}
	}

	if(destroy != 0)
	{
		SRELEASE(val);
	}
}



/******************************************************************************
 * Conversion function, relying on language type casting rather than trying
 * to be clever based on internal representation.
 *
 * \returns The converted type.
 *
 * \param[in]   val      The TNV being converted to a basic type.
 * \param[out]  success  Whether the conversion succeeded.
 *
 * \note
 *   1. Mapped values cannot be converted.
 *****************************************************************************/

int32_t  tnv_to_int(tnv_t val, int *success)
{
	int32_t result = 0;

	if(TNV_IS_MAP(val.flags))
	{
		*success = AMP_FAIL;
		return 0;
	}

	*success = AMP_OK;

	switch(val.type)
	{
		case AMP_TYPE_BOOL:
		case AMP_TYPE_BYTE:   result = (int32_t) val.value.as_byte;   break;
		case AMP_TYPE_INT:    result = val.value.as_int;              break;
		case AMP_TYPE_UINT:   result = (int32_t) val.value.as_uint;   break;
		case AMP_TYPE_VAST:   result = (int32_t) val.value.as_vast;   break;
		case AMP_TYPE_UVAST:  result = (int32_t) val.value.as_uvast;  break;
		case AMP_TYPE_REAL32: result = (int32_t) val.value.as_real32; break;
		case AMP_TYPE_REAL64: result = (int32_t) val.value.as_real64; break;
		default: *success = AMP_FAIL; break;
	}
	return result;
}




/******************************************************************************
 * Conversion function, relying on language type casting rather than trying
 * to be clever based on internal representation.
 *
 * \returns The converted type.
 *
 * \param[in]   val      The TNV being converted to a basic type.
 * \param[out]  success  Whether the conversion succeeded.
 *
 * \note
 *   1. Mapped values cannot be converted.
 *****************************************************************************/

float  tnv_to_real32(tnv_t val, int *success)
{
	float result = 0;

	if(TNV_IS_MAP(val.flags))
	{
		*success = AMP_FAIL;
		return 0;
	}

	*success = AMP_OK;

	switch(val.type)
	{
		case AMP_TYPE_BOOL:
		case AMP_TYPE_BYTE:  result = (float) val.value.as_byte;   break;
		case AMP_TYPE_INT:   result = (float) val.value.as_int;    break;
		case AMP_TYPE_UINT:  result = (float) val.value.as_uint;   break;
		case AMP_TYPE_VAST:  result = (float) val.value.as_vast;   break;
		case AMP_TYPE_UVAST: result = (float) val.value.as_uvast;  break;
		case AMP_TYPE_REAL32:result = (float) val.value.as_real32; break;
		case AMP_TYPE_REAL64:result = (float) val.value.as_real64; break;
		default: *success = AMP_FAIL; break;
	}
	return result;
}



/******************************************************************************
 * Conversion function, relying on language type casting rather than trying
 * to be clever based on internal representation.
 *
 * \returns The converted type.
 *
 * \param[in]   val      The TNV being converted to a basic type.
 * \param[out]  success  Whether the conversion succeeded.
 *
 * \note
 *   1. Mapped values cannot be converted.
 *****************************************************************************/

double  tnv_to_real64(tnv_t val, int *success)
{
	double result = 0;

	if(TNV_IS_MAP(val.flags))
	{
		*success = AMP_FAIL;
		return 0;
	}

	*success = AMP_OK;

	switch(val.type)
	{
		case AMP_TYPE_BOOL:
		case AMP_TYPE_BYTE:   result = (double) val.value.as_byte;  break;
		case AMP_TYPE_INT:   result = (double) val.value.as_int;    break;
		case AMP_TYPE_UINT:  result = (double) val.value.as_uint;   break;
		case AMP_TYPE_VAST:  result = (double) val.value.as_vast;   break;
		case AMP_TYPE_UVAST: result = (double) val.value.as_uvast;  break;
		case AMP_TYPE_REAL32:result = (double) val.value.as_real32; break;
		case AMP_TYPE_REAL64:result = (double) val.value.as_real64; break;
		default: *success = AMP_FAIL; break;
	}
	return result;
}



/******************************************************************************
 * Conversion function, relying on language type casting rather than trying
 * to be clever based on internal representation.
 *
 * \returns The converted type.
 *
 * \param[in]   val      The TNV being converted to a basic type.
 * \param[out]  success  Whether the conversion succeeded.
 *
 * \note
 *   1. Mapped values cannot be converted.
 *****************************************************************************/

uint32_t  tnv_to_uint(tnv_t val, int *success)
{
	uint32_t result = 0;

	if(TNV_IS_MAP(val.flags))
	{
		*success = AMP_FAIL;
		return 0;
	}

	*success = AMP_OK;

	switch(val.type)
	{
		case AMP_TYPE_BOOL:
		case AMP_TYPE_BYTE:   result = (uint32_t) val.value.as_byte;   break;
		case AMP_TYPE_INT:    result = (uint32_t) val.value.as_int;    break;
		case AMP_TYPE_UINT:   result = val.value.as_uint;              break;
		case AMP_TYPE_VAST:   result = (uint32_t) val.value.as_vast;   break;
		case AMP_TYPE_UVAST:  result = (uint32_t) val.value.as_uvast;  break;
		case AMP_TYPE_REAL32: result = (uint32_t) val.value.as_real32; break;
		case AMP_TYPE_REAL64: result = (uint32_t) val.value.as_real64; break;
		default: *success = AMP_FAIL; break;
	}

	return result;
}



/******************************************************************************
 * Conversion function, relying on language type casting rather than trying
 * to be clever based on internal representation.
 *
 * \returns The converted type.
 *
 * \param[in]   val      The TNV being converted to a basic type.
 * \param[out]  success  Whether the conversion succeeded.
 *
 * \note
 *   1. Mapped values cannot be converted.
 *****************************************************************************/

uvast  tnv_to_uvast(tnv_t val, int *success)
{
	uvast result = 0;

	if(TNV_IS_MAP(val.flags))
	{
		*success = AMP_FAIL;
		return 0;
	}

	*success = AMP_OK;

	switch(val.type)
	{
		case AMP_TYPE_BOOL:
		case AMP_TYPE_BYTE:   result = (uvast) val.value.as_byte;   break;
		case AMP_TYPE_INT:    result = (uvast) val.value.as_int;    break;
		case AMP_TYPE_UINT:   result = (uvast) val.value.as_uint;   break;
		case AMP_TYPE_VAST:   result = (uvast) val.value.as_vast;   break;
		case AMP_TYPE_UVAST:  result =  val.value.as_uvast;         break;
		case AMP_TYPE_REAL32: result = (uvast) val.value.as_real32; break;
		case AMP_TYPE_REAL64: result = (uvast) val.value.as_real64; break;
		default: *success = AMP_FAIL; break;
	}
	return result;
}



/******************************************************************************
 * Conversion function, relying on language type casting rather than trying
 * to be clever based on internal representation.
 *
 * \returns The converted type.
 *
 * \param[in]   val      The TNV being converted to a basic type.
 * \param[out]  success  Whether the conversion succeeded.
 *
 * \note
 *   1. Mapped values cannot be converted.
 *****************************************************************************/

vast  tnv_to_vast(tnv_t val, int *success)
{
	vast result = 0;

	if(TNV_IS_MAP(val.flags))
	{
		*success = AMP_FAIL;
		return 0;
	}

	*success = AMP_OK;

	switch(val.type)
	{
		case AMP_TYPE_BOOL:
		case AMP_TYPE_BYTE:   result = (vast) val.value.as_byte;   break;
		case AMP_TYPE_INT:    result = (vast) val.value.as_int;    break;
		case AMP_TYPE_UINT:   result = (vast) val.value.as_uint;   break;
		case AMP_TYPE_VAST:   result = val.value.as_vast;          break;
		case AMP_TYPE_UVAST:  result = (vast) val.value.as_uvast;  break;
		case AMP_TYPE_REAL32: result = (vast) val.value.as_real32; break;
		case AMP_TYPE_REAL64: result = (vast) val.value.as_real64; break;
		default: *success = AMP_FAIL; break;
	}
	return result;
}





// TNVC Stuff.


/******************************************************************************
 * Append one TNVC to another TNVC.
 *
 * \returns AMP status code.
 *
 * \param[in,out] dst  The TNVC being appended to.
 * \param[in]     src  The TNVC being appended.
 *
 * \note
 *   1.The dst TNVC will deep-copy the source TNVC.
 *****************************************************************************/

int tnvc_append(tnvc_t *dst, tnvc_t *src)
{
	uint8_t i;
	vec_idx_t max;
	int success = AMP_OK;

	if((dst == NULL) || (src == NULL))
	{
		AMP_DEBUG_ERR("tnvc_append","Bad parms", NULL);
		return AMP_FAIL;
	}

	/* Appending an empty list is easy... */
	if((max = vec_num_entries(src->values)) == 0)
	{
		return AMP_OK;
	}

	/* Make sure the destination TNVC has room. */
	vec_make_room(&(dst->values), max);

	/* Deep copy each item. */
	for(i = 0; i < max; i++)
	{
		tnv_t *cur_tnv = (tnv_t *)vec_at(&(src->values), i);
		tnv_t *new_tnv = NULL;

		if((cur_tnv == NULL) ||
		   ((new_tnv = tnv_copy_ptr(cur_tnv)) == NULL))
		{
			AMP_DEBUG_ERR("tnvc_append","Unable to copy item %d", i);
			break;
		}

		if(vec_insert(&(dst->values), new_tnv, NULL) != VEC_OK)
		{
			AMP_DEBUG_ERR("tnvc_append","Unable to insert item %d", i);
			tnv_release(new_tnv, 1);
			break;
		}
	}

	if (i < max)
	{
		success = AMP_FAIL;
	}

	return success;
}


void tnvc_cb_del(void *item)
{
	tnvc_release((tnvc_t*)item, 1);
}

int tnvc_cb_comp(void *i1, void *i2)
{
	return tnvc_compare((tnvc_t*)i1, (tnvc_t*) i2);
}

void* tnvc_cb_copy_fn(void *item)
{
	return tnvc_copy((tnvc_t*)item);
}



/******************************************************************************
 * Remove all entries from a TNVC, but keep the TNVC.
 *
 * \param[out] tnvc  The TNVC being cleared.
 *****************************************************************************/

void tnvc_clear(tnvc_t* tnvc)
{
	if(tnvc != NULL)
	{
		vec_clear(&(tnvc->values));
	}
}



/******************************************************************************
 * Perform a value comparison between two TNVCs.
 *
 * \returns -1 on error. 0 if equivalent. 1 if not equivalent.
 *
 * \param[in]   t1	The first TNVC to compare
 * \param[in]   t2  The second TNVC to compare.
 *
 * \note
 *   - This is only for similarity, not sorting. No concept of < or >.
 *****************************************************************************/

int tnvc_compare(tnvc_t *t1, tnvc_t *t2)
{
	uint8_t i = 0;
	int diff = 0;
	vecit_t it1, it2;

	if((t1 == NULL) || (t2 == NULL))
	{
		return -1;
	}

	if(vec_num_entries(t1->values) != vec_num_entries(t2->values))
	{
		return 1;
	}

	// Use iterator to ensure we are comparing values and not internal storage
	it1 = vecit_first(&(t1->values));
	it2 = vecit_first(&(t2->values));
	for(i = 0; i < vec_num_entries(t1->values); i++, vecit_next(it1), vecit_next(it2))
	{
		if( (diff = tnv_compare(vecit_data(it1), vecit_data(it2)) ) != 0)
		{
			return diff;
		}
	}

	return 0;
}




/******************************************************************************
 * Creates a TNVC.
 *
 * \returns The created TNVC, or NULL.
 *
 * \param[in]  num  The expected number of entries in the TNVC.
 *
 * \note
 *   - This allocated the TNVC and sets up the vector that holds its TNVs
 *****************************************************************************/

tnvc_t *tnvc_create(uint8_t num)
{
	tnvc_t *result = NULL;
	int success = AMP_FAIL;

	if((result = (tnvc_t *) STAKE(sizeof(tnvc_t))) == NULL)
	{
		AMP_DEBUG_ERR("tdc_create","Cannot allocate new TDC.", NULL);
		return NULL;
	}

	result->values = vec_create(num,tnv_cb_del,tnv_cb_comp,tnv_cb_copy, VEC_FLAG_AS_STACK, &success);

	if(success != AMP_OK)
	{
		AMP_DEBUG_ERR("tdc_create","Can't allocate vector.", NULL);
		SRELEASE(result);
		return NULL;
	}

	return result;
}




/******************************************************************************
 * Deep copies an existing TNVC
 *
 * \returns The copied TNVC, or NULL.
 *
 * \param[in] src  The TNVC being copied.
 *****************************************************************************/

tnvc_t* tnvc_copy(tnvc_t *src)
{
   tnvc_t *result = NULL;

   if((src == NULL) ||
      ((result = tnvc_create(vec_size(&(src->values)))) == NULL))
   {
	   return NULL;
   }

   if(tnvc_append(result, src) != AMP_OK)
   {
	   tnvc_release(result, 1);
	   return NULL;
   }

	return result;
}



/******************************************************************************
 * Deserializes a TNVC value from a CBOR value.
 *
 * \returns The created TNVC, or error code.
 *
 * \param[in,out] it       The current CBOR value.
 * \param[out]    success  AMP status code.
 *
 * \note
 *   1. This implementation only supports TNVCs encoded as TVCs.
 *****************************************************************************/

tnvc_t tnvc_deserialize(QCBORDecodeContext *it, int *success)
{
	uint8_t type;
	tnvc_t result;

	QCBORItem item;
	QCBORError err;
	size_t array_len = 0;

    memset(&result,0,sizeof(result));

#if AMP_VERSION < 7
    err = QCBORDecode_GetNext(it, &item);
    if ( err != QCBOR_SUCCESS || item.uDataType != QCBOR_TYPE_ARRAY)
	{
		AMP_DEBUG_ERR("tnvc_deserialize","CBOR Item Not An Array", NULL);
		*success = AMP_FAIL;
		return result;
	}
	array_len = item.val.uCount;
    
	/* Handle special case of empty TNVC. */
	if(array_len == 0)
	{
		*success = AMP_OK;
		tnvc_init(&result, 0);
		/* Skip over empty array,. */
		return result;
	}
#else
	QCBORDecode_StartOctets(it);
#endif
    
	/* Get the first byte (the flags). */
	*success = cut_get_cbor_numeric(it, AMP_TYPE_BYTE, &type);

	if(*success != AMP_OK)
	{
		AMP_DEBUG_ERR("tnvc_deserialize","Can't get TNVC form byte.",NULL);
		return result;
	}

#if AMP_VERSION >= 7
    /* Special Case: Is this an empty collection? */
    if(type == 0)
    {
		*success = AMP_OK;
		tnvc_init(&result, 0);
		/* Skip over empty array,. */
		QCBORDecode_EndOctets(it);
		return result;
	}

    /* Read Collection Length */
    *success = cut_get_cbor_numeric(it, AMP_TYPE_UINT, &array_len);
    if (*success != AMP_OK)
    {
		AMP_DEBUG_ERR("tnvc_deserialize","CBOR Item Length field not a number", NULL);
		return result;
	}
	
	// Extra Sanity Check
	if (array_len == 0)
	{
		AMP_DEBUG_WARN("tnvc_deserialize", "Illegal Collection of 0-length with non-zero flags. Treating as empty.", NULL);
		QCBORDecode_EndOctets(it);
		return result;
	}

#endif

	switch(type)
	{
		case TNVC_TVC:
			result = tnvc_deserialize_tvc(it, array_len, success);
			break;

		case TNVC_VC:
		case TNVC_NC:
		case TNVC_NVC:
		case TNVC_TC:
		case TNVC_TNC:
		case TNVC_TNVC:
			AMP_DEBUG_ERR("tnvc_deserialize","Unsupported TNVC formulation: %d", type);
			*success = AMP_FAIL;
			break;
		case TNVC_UNK:
		default:
			AMP_DEBUG_ERR("tnvc_deserialize","Unknown TNVC formulation: %d", type);
			*success = AMP_FAIL;
			break;
	}

	if(*success != AMP_OK)
	{
		AMP_DEBUG_ERR("tnvc_deserialize","Failed to get TNVC.", NULL);
	}

#if AMP_VERSION >= 7
	QCBORDecode_EndOctets(it);
#endif

	return result;
}



/******************************************************************************
 * Deserializes a TNVC pointer value from a CBOR value.
 *
 * \returns The created TNVC, or NULL
 *
 * \param[in,out] it       The current CBOR value.
 * \param[out]    success  AMP status code.
 *****************************************************************************/

tnvc_t *tnvc_deserialize_ptr(QCBORDecodeContext *it, int *success)
{
	tnvc_t *result = NULL;

	if((result = tnvc_create(0)) == NULL)
	{
		AMP_DEBUG_ERR("tnvc_deserialize_ptr","Can't allocate new struct.", NULL);
		*success = AMP_FAIL;
		return NULL;
	}

	*result = tnvc_deserialize(it, success);
	if(*success != AMP_OK)
	{
		tnvc_release(result, 1);
		result = NULL;
	}

	return result;
}



/******************************************************************************
 * Deserializes a TNVC pointer value from a BLOB
 *
 * \returns The created TNVC, or NULL
 *
 * \param[in]   data     The data holding the serialized TNVC.
 * \param[out]  success  AMP status code.
 *****************************************************************************/

tnvc_t*  tnvc_deserialize_ptr_raw(blob_t *data, int *success)
{
	QCBORDecodeContext decoder;

	CHKNULL(success);
	*success = AMP_FAIL;
	CHKNULL(data);

	QCBORDecode_Init(&decoder,
					 (UsefulBufC){data->value,data->length},
					 QCBOR_DECODE_MODE_NORMAL);

	tnvc_t *tmp = tnvc_deserialize_ptr(&decoder, success);
	
	// Verify Decoding Completed Successfully
	cut_decode_finish(&decoder);

	return tmp;
}



/******************************************************************************
 * Deserializes a TNVC value from a BLOB
 *
 * \returns The created TNVC, or NULL
 *
 * \param[in]   data     The data holding the serialized TNVC.
 * \param[out]  success  AMP status code.
 *****************************************************************************/

tnvc_t   tnvc_deserialize_raw(blob_t *data, int *success)
{
	QCBORDecodeContext decoder;
	tnvc_t result;

	*success = AMP_FAIL;
	memset(&result,0,sizeof(result));
	if(data == NULL)
	{
		return result;
	}

	QCBORDecode_Init(&decoder,
					 (UsefulBufC){data->value,data->length},
					 QCBOR_DECODE_MODE_NORMAL);
	result = tnvc_deserialize(&decoder, success);

	// Verify Decoding Completed Successfully
	cut_decode_finish(&decoder);

	return result;
}



/******************************************************************************
 * Deserializes a TNVC value from a CBOR value that is encoded as a TVC
 *
 * \returns The TNVC
 *
 * \param[in,out] it       The current CBOR value.
 * \param[out]    success  AMP status code.
 *****************************************************************************/
#if AMP_VERSION < 7
static tnvc_t tnvc_deserialize_tvc(QCBORDecodeContext *array_it, size_t array_len, int *success)
{
	tnvc_t result;
	blob_t types;
	int i;

	AMP_DEBUG_ENTRY("tnvc_deserialize_tvc","(0x"ADDR_FIELDSPEC",0x"ADDR_FIELDSPEC")", (uaddr) array_it, (uaddr) success);
	memset(&result,0,sizeof(result));
	*success = AMP_OK;

	/* Deserialize Types (array_len byte fields) */
	types = blob_deserialize(array_it, success);
	if(*success != AMP_OK)
	{
		AMP_DEBUG_ERR("tnvc_deserialize_tvc","Can't get types set.", NULL);
		return result;
	}

	/* Sanity check. If the array has N elements, the first element is the types array and it should
	 * have N-2 types identified (subtracting types bytes array and flags)
	 */
	if(types.length != (array_len - 2)) // TODO: Why is it -2 for AMPv6? array_len is of parent array...
	{
		AMP_DEBUG_ERR("tnvc_deserialize_tvc","Size mismtach: array size %ld and types length %ld.", array_len, types.length);
		*success = AMP_FAIL;
		blob_release(&types, 0);
		return result;
	}
	
	result.values = vec_create(array_len - 2, tnv_cb_del,tnv_cb_comp,tnv_cb_copy, VEC_FLAG_AS_STACK, success);

	if(*success != AMP_OK)
	{
		AMP_DEBUG_ERR("tnvc_deserialize_tvc","Can;t allocate vector.", NULL);
		blob_release(&types, 0);
		return result;
	}

	/* For each type deserialize values */
	for(i = 0; i < types.length; i++)
	{
		tnv_t *val = tnv_create();
		*success = AMP_FAIL;

		blob_t *blob = blob_deserialize_ptr(array_it, success);

		if(blob != NULL)
		{
			val->type = types.value[i];
			if((*success = tnv_deserialize_val_raw(blob, val)) == AMP_OK)
			{
				vec_insert(&(result.values), val, NULL);
			}
			blob_release(blob, 1);
		}
		
		if(*success != AMP_OK)
		{
			break;
		}
	}

	blob_release(&types, 0);

	if(*success != AMP_OK)
	{
		AMP_DEBUG_ERR("tnv_deserialize_tvc","Failed to deserialize values (last was %d).", i);
		vec_release(&(result.values), 0);
		return result;
	}

	return result;
}
#else // AMPv7
static tnvc_t tnvc_deserialize_tvc(QCBORDecodeContext *array_it, size_t array_len, int *success)
{
	tnvc_t result;
	blob_t types;
	int i;

	AMP_DEBUG_ENTRY("tnvc_deserialize_tvc","(0x"ADDR_FIELDSPEC",0x"ADDR_FIELDSPEC")", (uaddr) array_it, (uaddr) success);
	memset(&result,0,sizeof(result));
	*success = AMP_OK;

	/* Deserialize Types (array_len byte fields) */
	types = blob_deserialize_as_bytes(array_it, success, array_len);
	if(*success != AMP_OK)
	{
		AMP_DEBUG_ERR("tnvc_deserialize_tvc","Can't get types set.", NULL);
		return result;
	}

	result.values = vec_create(array_len, tnv_cb_del,tnv_cb_comp,tnv_cb_copy, VEC_FLAG_AS_STACK, success);

	if(*success != AMP_OK)
	{
		AMP_DEBUG_ERR("tnvc_deserialize_tvc","Can;t allocate vector.", NULL);
		blob_release(&types, 0);
		return result;
	}

	/* For each type deserialize values */
	for(i = 0; i < array_len; i++)
	{
		tnv_t *val = tnv_create();
		*success = AMP_FAIL;
		if (val == NULL)
		{
			AMP_DEBUG_ERR("tnv_deserialize_tvc","Error allocating TNV", NULL);
			break;
		}
		val->type = types.value[i];

		*success = tnv_deserialize_val_by_type(array_it, val);
		if (*success == AMP_OK)
		{
			vec_insert(&(result.values), val, NULL);
		}
		else
		{
			AMP_DEBUG_ERR("tnv_deserialize_tvc", "Failed to deserialize TNV %i\n", i);
			break;
		}
	}

	blob_release(&types, 0);

	if(*success != AMP_OK)
	{
		AMP_DEBUG_ERR("tnv_deserialize_tvc","Failed to deserialize values (last was %d).", i);
		vec_release(&(result.values), 0);
		return result;
	}

	return result;
}
#endif


/******************************************************************************
 * Returns the requested TNV from a TNVC.
 *
 * \returns The TNV, or NULL.
 *
 * \param[in]  tnvc   The collection holding the requested TNV.
 * \param[out] index  Which TNV is being requested.
 *****************************************************************************/

tnv_t* tnvc_get(tnvc_t* tnvc, uint8_t index)
{
	tnv_t *result = NULL;

	if(tnvc != NULL)
	{
		result = vec_at(&(tnvc->values), index);
	}
	return result;
}



/******************************************************************************
 * Returns the number of TNVs in the TNVC.
 *
 * \returns the TNV count.
 *
 * \param[in]  tnvc   The TNVC collection being queried.
 *****************************************************************************/

uint8_t tnvc_get_count(tnvc_t* tnvc)
{
	uint8_t count = 0;
	if(tnvc != NULL)
	{
		count = vec_num_entries(tnvc->values);
	}
	return count;
}



/******************************************************************************
 * Returns the way in which a TNVC should be encoded.
 *
 * \returns The TNVC encoding type.
 *
 * \param[in]  tnvc   The TNVC collection being queried.
 *****************************************************************************/

tnv_enc_e tnvc_get_encode_type(tnvc_t *tnvc)
{
	return TNVC_TVC;
}



/******************************************************************************
 * Returns the type of a given TNV in a TNVC.
 *
 * \returns The type of the requested TNV.
 *
 * \param[in]  tnvc   The TNVC collection being queried.
 * \param[in]  index  The index of the TNV being queried.
 *****************************************************************************/

amp_type_e tnvc_get_type(tnvc_t *tnvc, uint8_t index)
{
	tnv_t *tnv = NULL;

	if(tnvc != NULL)
	{
		tnv = (tnv_t *) vec_at(&(tnvc->values), index);
	}
	return ((tnv == NULL) ? AMP_TYPE_UNK : tnv->type);
}




/******************************************************************************
 * Returns a BLOB that contains the type of every TNV in the TNVC, in order,
 * with each byte of the BLOB representing 1 TNV type.
 *
 * \returns The BLOB of TNVC TNV types.
 *
 * \param[in]   tnvc     The TNVC collection being queried.
 * \param[out]  success  AMP status code.
 *****************************************************************************/

blob_t tnvc_get_types(tnvc_t *tnvc, int *success)
{
	blob_t types;
	uint8_t length = 0;
	uint8_t i = 0;

	*success = AMP_FAIL;
	memset(&types, 0, sizeof(blob_t));
	if(tnvc == NULL)
	{
		return types;
	}

	length = vec_num_entries(tnvc->values);
	blob_init(&types, NULL, 0, length);
	for(i = 0; i < length; i++)
	{
		amp_type_e cur_type = tnvc_get_type(tnvc, i);
		blob_append(&types, (uint8_t*)&cur_type, 1);
	}

	*success = AMP_OK;
	return types;
}



/******************************************************************************
 * Initializes an empty TNVC.
 *
 * \returns AMP status code.
 *
 * \param[out]  tnvc  The TNVC collection being initialized
 * \param[in]   num   The expected minimum number of entries in the TNVC.
 *****************************************************************************/

int tnvc_init(tnvc_t *tnvc, size_t num)
{
	int success = AMP_FAIL;

	if(tnvc != NULL)
	{
		tnvc->values = vec_create(num, tnv_cb_del, tnv_cb_comp, tnv_cb_copy, VEC_FLAG_AS_STACK, &success);
	}

	return success;
}



/******************************************************************************
 * Inserts a new TNV to the end of a TNVC.
 *
 * \returns AMP status code.
 *
 * \param[out]  tnvc  The TNVC receiving the new TNV.
 * \param[in]   tnv   The new TNV value.
 *
 * \note
 *   1. The new TNV is shallow-copied into the TNVC and MUST NOT be freed
 *      by the caller if this function succeeds.
 *****************************************************************************/

int tnvc_insert(tnvc_t* tnvc, tnv_t *tnv)
{
	int result;

	/* It isn't necessarily an error if TNV is NULL. */
	if((tnvc == NULL) || (tnv == NULL))
	{
		return AMP_FAIL;
	}

	if((result = vec_insert(&(tnvc->values), tnv, NULL)) != AMP_OK)
	{
		AMP_DEBUG_ERR("tnvc_insert","Error vector inserting.", NULL);
		tnv_release(tnv, 1);
	}

	return result;
}



/******************************************************************************
 * Releases resources associated with a TNVC.
 *
 * \param[out]  tnvc     The TNVC being released
 * \param[in]   destroy  Whether to release the TNVC container.
 *
 * \note
 *   1. Destroy must be set to 1 if the tnvc is in a buffer pool and 0 if it is
 *      defined on the stack.
 *****************************************************************************/

void tnvc_release(tnvc_t *tnvc, int destroy)
{
	if(tnvc != NULL)
	{
		vec_release(&(tnvc->values), 0);

		if(destroy)
		{
			SRELEASE(tnvc);
		}
	}

}


/*
 * This may be called with a NULL encoder for sizing, so only bail if we
 * get something other than CBorErrorOutOfMemory.
 *
 * \note
 *   1. ION AMP will never have a name.
 *   2. ION AMP will always have type and value.
 *   3. The current implementation only understands E(TNVC) == TVC.
 */

int tnvc_serialize(QCBOREncodeContext *encoder, void *item)
{
	tnv_enc_e type;
	tnvc_t *tnvc = (tnvc_t *) item;

	/* Step 1: Figure out what flavor of TNVC we are serializing. */
	/* Note; Write now this is always a TVC. */


	if((type = tnvc_get_encode_type(tnvc)) == TNVC_UNK)
	{
		AMP_DEBUG_ERR("tnvc_serialize","Unknown type.", NULL);
		return AMP_FAIL;
	}

	/* Step 3: Based on type, serialize the rest of the collection. */
	switch(type)
	{
		case TNVC_TVC:
		   return tnvc_serialize_tvc(encoder, tnvc);

		default:
			return AMP_FAIL;
	}
}





blob_t* tnvc_serialize_wrapper(tnvc_t *tnvc)
{

	return cut_serialize_wrapper(TNVC_DEFAULT_ENC_SIZE, tnvc, (cut_enc_fn)tnvc_serialize);
}


/*
 * This is an array of N + 1 elements, where N is the number of values and
 * the +1 is a type bytestring at the beginning.
 */

static int tnvc_serialize_tvc(QCBOREncodeContext *encoder, tnvc_t *tnvc)
{
	int err;
	blob_t types;
	int success;
	uint8_t i;
	int num;


	CHKUSR(encoder, AMP_FAIL);
	CHKUSR(tnvc, AMP_FAIL);

    /* Step 1: Setup Container Flags */
	num = vec_num_entries(tnvc->values);

	// Start an Array. (Octets Array for AMP_VERSION >=7)
	QCBOREncode_OpenArray(encoder);
	
#if AMP_VERSION < 7

	/* Special case of an empty TNVC. Just write an empty array */
	if(num == 0)
	{
	   QCBOREncode_CloseArray(encoder);
	   return AMP_OK;
	}
#else
	/* Special case of an empty TNVC. Just write a zero flag for type */
	if(num == 0)
	{
       err = cut_enc_byte(encoder, 0);
       if (err != AMP_OK)
       {
          AMP_DEBUG_ERR("tnvc_serialize","Cbor Error: %d encoding empty TNVC", err);
          return err;
       }
	   QCBOREncode_CloseArrayOctet(encoder);
	   return AMP_OK;
	}    
#endif
    
	/* Step 2: Write the type (Flags) as the first encoded byte. */
	err = cut_enc_byte(encoder, TNVC_TVC);
	if(err != AMP_OK)
	{
		AMP_DEBUG_ERR("tnvc_serialize","Cbor Error: %d. Type %d", err, TNVC_TVC);
		return err;
	}

#if AMP_VERSION >= 7
    /* Step 2a: Write the # of Item as an encoded uint */
    QCBOREncode_AddUInt64(encoder, num);
#endif

	/* Step 2: Construct and serialize the type bytestring. */
	types = tnvc_get_types(tnvc, &success);

	if(types.length != num)
	{
		AMP_DEBUG_WARN("tnvc_serialize_tvc","Mismatch: have %d types but expected %d.", types.length, num);
	}

#if AMP_VERSION < 7
	err = blob_serialize(encoder, &types);
#else
	err = blob_serialize_as_bytes(encoder, &types);
#endif
	blob_release(&types, 0);

	if(err != AMP_OK)
	{
		AMP_DEBUG_ERR("tnvc_serialize_tvc","Can't serialize types", NULL);
		return err;
	}

	/* Step 4: For each value, encode it. */
	for(i = 0; i < num; i++)
	{
		tnv_t *tnv = (tnv_t*) vec_at(&(tnvc->values),i);

#if AMP_VERSION < 7
		/* Go through the trouble of getting a serialized string because we don't
		 * want the array encoder to think parts of the serialized value are
		 * different indices in the array...
		 */
		blob_t *blob = tnv_serialize_value_wrapper(tnv);
		err = blob_serialize(encoder, blob);

		blob_release(blob, 1);
#else
		err = tnv_serialize_value(encoder, tnv);
#endif
		if(err != AMP_OK)
		{
			AMP_DEBUG_ERR("tnvc_serialize_tvc","Can't serialize TNV: %d", i);
			break;
		}
	}

	if(err != AMP_OK)
	{
		AMP_DEBUG_ERR("tnvc_serialize_tvc","Cbor Error: %d", err);
#if AMP_VERSION < 7
		QCBOREncode_CloseArray(encoder);
#else
		QCBOREncode_CloseArrayOctet(encoder);
#endif
		return err;
	}

#if AMP_VERSION < 7
	QCBOREncode_CloseArray(encoder);
#else
	QCBOREncode_CloseArrayOctet(encoder);
#endif
	return AMP_OK;
}


/* 09/30/2018) */
int  tnvc_size(tnvc_t *tnvc)
{
	CHKZERO(tnvc);
	return vec_num_entries(tnvc->values);
}


/*
 * Replace a TNV in a TNVC.
 */

int tnvc_update(tnvc_t *tnvc, uint8_t idx, tnv_t *src_tnv)
{
	int success = AMP_FAIL;
	tnv_t *old = NULL;

	CHKUSR(tnvc,AMP_FAIL);
	CHKUSR(src_tnv,AMP_FAIL);


	old = (tnv_t *) vec_set(&(tnvc->values), idx, src_tnv, &success);
	if(success != AMP_OK)
	{
		return success;
	}

	tnv_release(old, 1);
	return AMP_OK;
}
