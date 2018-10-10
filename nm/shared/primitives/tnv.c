/*****************************************************************************
 **
 ** File Name: tdc.c
 **
 ** Description: This implements a Type-Name-Value instance and multiple
 **              encodings of TNV collections.
 **
 ** Notes:
 **  1. A TNV may not have as its type another TNV or a TNVC.
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
static int tnv_copy_helper(tnv_t *val);
static int tnv_deserialize_val_by_type(CborValue *it, tnv_t *result);

//static int tnvc_deserialize_vc(CborValue *it);
//static int tnvc_deserialize_nc(CborValue *it);
//static int tnvc_deserialize_nvc(CborValue *it);
//static int tnvc_deserialize_tc(CborValue *it);
static tnvc_t tnvc_deserialize_tvc(CborValue *it, int *success);
//static int tnvc_deserialize_tnc(CborValue *it);
//static int tnvc_deserialize_tnv(CborValue *it);

static tnv_enc_e tnvc_get_encode_type(tnvc_t *tnvc);

static CborError tnvc_serialize_tvc(CborEncoder *encoder, tnvc_t *tnvc);

static void tnv_del_fn(void *item)
{
	return tnv_release((tnv_t*)item, 1);
}

static int tnv_comp_fn(void *i1, void *i2)
{
	return tnv_compare((tnv_t *)i1, (tnv_t *)i2);
}

void *tnv_cb_copy(void *item)
{
	tnv_t *tnv = (tnv_t*) item;
	CHKNULL(tnv);
	return tnv_copy_ptr(*tnv);
}

tnv_t* tnv_cast(tnv_t *tnv, amp_type_e type)
{
	tnv_t *result = NULL;
	int success;

	/* Cannot cast non-numeric types. Also
	 * Cannot cast a mapped parameter.
	 */
	if( (type_is_numeric(tnv->type) == 0) ||
		(type_is_numeric(type) == 0) ||
		(TNV_IS_MAP(tnv->flags)))
	{
		return NULL;
	}

	/* cast is just a copy. */
	if(tnv->type == type)
	{
		return NULL;
	}

	switch(type)
	{
	case AMP_TYPE_INT:   result = tnv_from_int(tnv_to_int(*tnv, &success)); break;
	case AMP_TYPE_UINT:  result = tnv_from_uint(tnv_to_uint(*tnv, &success)); break;
	case AMP_TYPE_VAST:  result = tnv_from_vast(tnv_to_vast(*tnv, &success)); break;
	case AMP_TYPE_TV:
	case AMP_TYPE_TS:
	case AMP_TYPE_UVAST:  result = tnv_from_uvast(tnv_to_uvast(*tnv, &success)); break;
	case AMP_TYPE_REAL32:  result = tnv_from_real32(tnv_to_real32(*tnv, &success)); break;
	case AMP_TYPE_REAL64:  result = tnv_from_real64(tnv_to_real64(*tnv, &success)); break;
	default:
		success = AMP_FAIL; break;
	}

	if(success != AMP_OK)
	{
		AMP_DEBUG_ERR("tnv_cast","Cannot cast from type %d top type %d.", tnv->type, type);
		result = NULL;
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name: tnv_compare
 *
 * \par Purpose: Perform a value comparison between two TNVs.
 *
 * \return -1 System Error
 *          0 Values equal
 *          1 Values differ
 *
 * \param[in]   t1	The first TNV to compare
 * \param[in]   t2  The second TNV to compare.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  09/7/18  E. Birrane     Initial implementation,
 *****************************************************************************/
int tnv_compare(tnv_t *v1, tnv_t *v2)
{
	int diff = 0;

	CHKERR(v1);
	CHKERR(v2);

	if(v1 == v2)
	{
		return 0;
	}

	if(v1->type != v2->type)
	{
		return 1;
	}

	switch(v1->type)
	{
		case AMP_TYPE_CNST:
		case AMP_TYPE_EDD:
		case AMP_TYPE_ARI:
		case AMP_TYPE_LIT:
			diff = ari_compare((ari_t*)v1->value.as_ptr, (ari_t*)v2->value.as_ptr);
			break;

		case AMP_TYPE_CTRL: diff = ctrl_cb_comp_fn((ctrl_t*)v1->value.as_ptr, (ctrl_t*)v2->value.as_ptr); break;


		case AMP_TYPE_OPER: diff = op_cb_comp_fn((op_t*)v1->value.as_ptr, (op_t*)v2->value.as_ptr);       break;
		case AMP_TYPE_RPT:  diff = rpt_cb_comp_fn((rpt_t*)v1->value.as_ptr, (rpt_t*)v2->value.as_ptr);    break;
		case AMP_TYPE_RPTTPL: diff = rpttpl_cb_comp_fn((rpttpl_t*)v1->value.as_ptr, (rpttpl_t*)v2->value.as_ptr); break;

		case AMP_TYPE_TBR:
		case AMP_TYPE_SBR:  diff = rule_cb_comp_fn(v1->value.as_ptr, v2->value.as_ptr);    break;


		case AMP_TYPE_VAR:  diff = var_cb_comp_fn((var_t*)v1->value.as_ptr, (var_t*)v2->value.as_ptr);    break;

		case AMP_TYPE_BYTESTR: diff = blob_compare((blob_t*)v1->value.as_ptr, (blob_t*)v2->value.as_ptr); break;

		case AMP_TYPE_STR: diff = strcmp((char*)v1->value.as_ptr, (char*)v2->value.as_ptr); break;

		case AMP_TYPE_BOOL:
		case AMP_TYPE_BYTE:  diff = (v1->value.as_byte == v2->value.as_byte) ? 0 : 1; break;


		case AMP_TYPE_INT:  diff = (v1->value.as_int == v2->value.as_int) ? 0 : 1; break;
		case AMP_TYPE_UINT:  diff = (v1->value.as_uint == v2->value.as_uint) ? 0 : 1; break;
		case AMP_TYPE_VAST:  diff = (v1->value.as_vast == v2->value.as_vast) ? 0 : 1; break;
		case AMP_TYPE_TV:
		case AMP_TYPE_TS:
		case AMP_TYPE_UVAST:  diff = (v1->value.as_uvast == v2->value.as_uvast) ? 0 : 1; break;
		case AMP_TYPE_REAL32:  diff = (v1->value.as_real32 == v2->value.as_real32) ? 0 : 1; break;
		case AMP_TYPE_REAL64:  diff = (v1->value.as_real64 == v2->value.as_real64) ? 0 : 1; break;

		case AMP_TYPE_EXPR:
		case AMP_TYPE_TBL:
		case AMP_TYPE_TBLT:
		case AMP_TYPE_MAC:
		case AMP_TYPE_AC:
		default:
			AMP_DEBUG_WARN("tnv_compare", "Unsupported compare for type %d", v1->type);
			diff =  AMP_SYSERR; break;
	}

	return diff;
}



void tnv_cb_del(void *item)
{
	tnv_release((tnv_t*)item, 1);
}

int  tnv_cb_comp(void *i1, void *i2)
{
	return tnv_compare((tnv_t*)i1, (tnv_t*)i2);
}


tnv_t  tnv_copy(tnv_t val, int *success)
{
	tnv_t result;

	memcpy(&result, &val, sizeof(tnv_t));

	/* If this is not a basic type, we need to deep copy. */
	switch(val.type)
	{
		case AMP_TYPE_CNST:
		case AMP_TYPE_CTRL:
		case AMP_TYPE_EDD:
		case AMP_TYPE_LIT:
		case AMP_TYPE_MAC:
		case AMP_TYPE_OPER:
		case AMP_TYPE_RPT:
		case AMP_TYPE_RPTTPL:
		case AMP_TYPE_SBR:
		case AMP_TYPE_TBL:
		case AMP_TYPE_TBLT:
		case AMP_TYPE_TBR:
		case AMP_TYPE_VAR:
		case AMP_TYPE_STR:
		case AMP_TYPE_ARI:
		case AMP_TYPE_AC:
		case AMP_TYPE_EXPR:
		case AMP_TYPE_BYTESTR:
		{
			if((*success = tnv_copy_helper(&result)) != AMP_OK)
			{
				result.type = AMP_TYPE_UNK;
			}
		}
		break;

		case AMP_TYPE_TNV:
		case AMP_TYPE_TNVC:
			result.type = AMP_TYPE_UNK;
			break;

		default:
			break;
	};

	return result;
}


static int  tnv_copy_helper(tnv_t *val)
{
	CHKERR(val);
	CHKERR(val->value.as_ptr);

	switch(val->type)
	{
		case AMP_TYPE_CNST:
		case AMP_TYPE_EDD:
		case AMP_TYPE_LIT:
		case AMP_TYPE_ARI:  val->value.as_ptr = ari_copy_ptr(*((ari_t*)val->value.as_ptr));  break;

		case AMP_TYPE_CTRL: val->value.as_ptr = ctrl_copy_ptr((ctrl_t*) val->value.as_ptr);   break;

		case AMP_TYPE_MAC:
		case AMP_TYPE_AC:   val->value.as_ptr = ac_copy_ptr((ac_t*) val->value.as_ptr);   break;

		case AMP_TYPE_OPER: val->value.as_ptr = op_copy_ptr((op_t*) val->value.as_ptr);      break;
		case AMP_TYPE_RPT:  val->value.as_ptr = rpt_copy_ptr((rpt_t*) val->value.as_ptr);    break;
		case AMP_TYPE_RPTTPL: val->value.as_ptr = rpttpl_copy_ptr((rpttpl_t*) val->value.as_ptr);  break;

		case AMP_TYPE_TBR:
		case AMP_TYPE_SBR:  val->value.as_ptr = rule_copy_ptr((rule_t*) val->value.as_ptr);    break;


		case AMP_TYPE_TBL:  val->value.as_ptr = tbl_copy_ptr((tbl_t*) val->value.as_ptr);    break;
		case AMP_TYPE_TBLT: val->value.as_ptr = tblt_copy_ptr((tblt_t*) val->value.as_ptr);  break;

		case AMP_TYPE_VAR:  val->value.as_ptr = var_copy_ptr((var_t*) val->value.as_ptr);    break;
		case AMP_TYPE_EXPR: val->value.as_ptr = expr_copy_ptr((expr_t*) val->value.as_ptr);  break;
		case AMP_TYPE_BYTESTR: val->value.as_ptr = blob_copy_ptr((blob_t*) val->value.as_ptr);  break;

		case AMP_TYPE_STR:
			{
				char *tmp = NULL;
				size_t len = strlen((char*)val->value.as_ptr);
				if((tmp = STAKE(len + 1)) != NULL)
				{
					strncpy(tmp, val->value.as_ptr, len);
					val->value.as_ptr = tmp;
				}
			};
			break;

		case AMP_TYPE_TNV:
		case AMP_TYPE_TNVC:
		default:
			val->value.as_ptr = NULL;
			break;
	}

	if(val->value.as_ptr == NULL)
	{
		TNV_CLEAR_ALLOC(val->flags);
		return AMP_FAIL;
	}

	TNV_SET_ALLOC(val->flags);

	return AMP_OK;
}


tnv_t *tnv_copy_ptr(tnv_t val)
{
	tnv_t *result = NULL;
	int success;

	result = STAKE(sizeof(tnv_t));
	CHKNULL(result);

	*result = tnv_copy(val, &success);
	if(success != AMP_OK)
	{
		SRELEASE(result);
		result = NULL;
	}

	return result;
}

tnv_t *tnv_create()
{
	tnv_t *result = NULL;

	if((result = STAKE(sizeof(tnv_t))) != NULL)
	{
		memset(result,0,sizeof(tnv_t));
	}

	tnv_init(result, AMP_TYPE_UNK);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: tnv_deserialize
 *
 * \par Builds a TNV from an input byte stream formatted in accordance with the
 *      AMP CBOR encoding of objects.
 *
 *                             +---------+
 *                             |   TNV   |
 *                             | [ARRAY] |
 *                             +----++---+
 *                                  ||
 *                                  ||
 *                  _______________/  \________________
 *                 /                                   \
 *                 +------------+-----------+----------+
 *                 | Flags/Type |    Name   |   Value  |
 *                 |   [BYTE]   | [TXT STR] | [Varies] |
 *                 |            |   (opt)   |   (opt)  |
 *                 +------------+-----------+----------+
 *
 *
 * \retval The deserialized structure.
 *
 * \param[in|out]  it       The CBOR value iterator being read/advanced
 * \param[out]     success  Whether the deserialization succeeded.
 *
 * \par Notes:
 *   - A deserialization error will return  a TNV with unknown type.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
  * 07/10/15  E. Birrane     Updat to support all DTNMP types.
 *  09/02/18  E. Birrane     Update CBOR and to latest spec. (JHU/APL)
 *****************************************************************************/

tnv_t tnv_deserialize(CborValue *it, int *success)
{
    uint8_t type = 0;
    size_t array_len = 0;
    CborValue array_it;
    CborError err = CborNoError;
    tnv_t result;

    AMP_DEBUG_ENTRY("tnv_deserialize","("ADDR_FIELDSPEC","ADDR_FIELDSPEC")", (uaddr)it, (uaddr)success);

    tnv_init(&result, AMP_TYPE_UNK);

    CHKUSR(it, result);
    CHKUSR(success, result);

    /* This should be an array... */
    if((*success = cut_get_array_len(it, &array_len)) != AMP_OK)
    {
    	return result;
    }

    if((err = cbor_value_enter_container(it, &array_it)) != CborNoError)
    {
    	AMP_DEBUG_ERR("tnv_deserialize","Cbor Error: %d", err);
    	*success = AMP_FAIL;
    	return result;
    }


    /* Step 1: Grab the TNV and Flags byte. */
    cut_get_cbor_numeric(&array_it, AMP_TYPE_BYTE, &type);

    result.type = (type & 0x7F);
    array_len--;

    /* Step 2: Deserialize name, if we have a name. */
    if(type & 0x80)
    {
    	char *name = cut_get_cbor_str(&array_it, success);
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
    	err = CborNoError;

    	if(tnv_deserialize_val_by_type(&array_it, &result) != AMP_OK)
    	{
    		AMP_DEBUG_ERR("tnv_deserialize","Deserialize error: Cbor %d AMP %d", err, *success);
    		tnv_release(&result,0);
    		result.type = AMP_TYPE_UNK;
    		cbor_value_leave_container(it, &array_it);
    		return result;
		}
    }

    if((err = cbor_value_leave_container(it, &array_it)) != CborNoError)
    {
    	AMP_DEBUG_ERR("tnv_deserialize","Deserialize error: Cbor %d", err);
    	tnv_release(&result,0);
    	result.type = AMP_TYPE_UNK;
    	*success = AMP_FAIL;
    }

    *success = AMP_OK;
	return result;
}



/*
 * TODO: Document this API pattern.
 */

tnv_t *tnv_deserialize_ptr(CborValue *it, int *success)
{
	tnv_t *result = NULL;

	if((result = (tnv_t*)STAKE(sizeof(tnv_t))) == NULL)
	{
		AMP_DEBUG_ERR("tnv_deserialize_ptr","Can't allocate new struct.", NULL);
		*success = AMP_FAIL;
	}

	*result = tnv_deserialize(it, success);
	if(*success != AMP_OK)
	{
		SRELEASE(result);
		result = NULL;
	}

	return result;
}


static int tnv_deserialize_val_by_type(CborValue *it, tnv_t *result)
{
	int success = AMP_FAIL;
	int can_alloc = 0;

	TNV_CLEAR_ALLOC(result->flags);

	switch(result->type)
	{
		/* AMM Objects and Compound Objects.*/
	    case AMP_TYPE_EDD:
	    case AMP_TYPE_CNST:
	    case AMP_TYPE_ARI:
	    case AMP_TYPE_LIT: result->value.as_ptr = ari_deserialize_ptr(it, &success);  can_alloc = 1; break;

	    case AMP_TYPE_CTRL: result->value.as_ptr = ctrl_deserialize_ptr(it, &success); can_alloc = 1; break;
	   	case AMP_TYPE_MAC:  result->value.as_ptr = macdef_deserialize_ptr(it, &success);  can_alloc = 1; break;

	   	case AMP_TYPE_RPT:  result->value.as_ptr = rpt_deserialize_ptr(it, &success);  can_alloc = 1; break;
	   	case AMP_TYPE_RPTTPL: result->value.as_ptr = rpttpl_deserialize_ptr(it, &success); can_alloc = 1; break;

	   	case AMP_TYPE_TBL:  result->value.as_ptr = tbl_deserialize_ptr(it, &success);  can_alloc = 1; break;
	   	case AMP_TYPE_TBLT: result->value.as_ptr = tblt_deserialize_ptr(it, &success); can_alloc = 1; break;

	   	case AMP_TYPE_TBR:
	   	case AMP_TYPE_SBR:  result->value.as_ptr = rule_deserialize_ptr(it, &success);  can_alloc = 1; break;

	   	case AMP_TYPE_VAR:  result->value.as_ptr = var_deserialize_ptr(it, &success);  can_alloc = 1; break;
	   	case AMP_TYPE_AC:   result->value.as_ptr = ac_deserialize_ptr(it, &success);   can_alloc = 1; break;
	   	case AMP_TYPE_EXPR: result->value.as_ptr = expr_deserialize_ptr(it, &success); can_alloc = 1; break;
	   	case AMP_TYPE_BYTESTR: result->value.as_ptr = blob_deserialize_ptr(it, &success);  can_alloc = 1; break;

	       /* Primitive Types and Derived Types */
	   	case AMP_TYPE_STR:  result->value.as_ptr = cut_get_cbor_str(it, &success); can_alloc = 1; break;
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

	   	case AMP_TYPE_OPER:
	   	default:
	   		break;
	}

	if((success == AMP_OK) && (can_alloc == 1))
	{
		TNV_SET_ALLOC(result->flags);
	}

	return success;
}

tnv_t*  tnv_from_bool(uint8_t val)
{
	tnv_t *result = tnv_create();
	CHKNULL(result);
	tnv_init(result, AMP_TYPE_BOOL);
	result->value.as_byte = val;
	return result;
}

tnv_t*  tnv_from_byte(uint8_t val)
{
	tnv_t *result = tnv_create();
	CHKNULL(result);
	tnv_init(result, AMP_TYPE_BYTE);
	result->value.as_byte = val;
	return result;

}

tnv_t* tnv_from_int(int32_t val)
{
	tnv_t *result = tnv_create();
	CHKNULL(result);
	tnv_init(result, AMP_TYPE_INT);
	result->value.as_int = val;
	return result;
}


tnv_t* tnv_from_real32(float val)
{
	tnv_t *result = tnv_create();
	CHKNULL(result);
	tnv_init(result, AMP_TYPE_REAL32);
	result->value.as_real32 = val;
	return result;
}

tnv_t* tnv_from_real64(double val)
{
	tnv_t *result = tnv_create();
	CHKNULL(result);
	tnv_init(result, AMP_TYPE_REAL64);
	result->value.as_real64 = val;
	return result;
}



/*
 * Must release result...
 */
tnv_t*  tnv_from_str(char *str)
{
	tnv_t* result = NULL;
	uint32_t len;

	AMP_DEBUG_ENTRY("tnv_from_str","(0x"ADDR_FIELDSPEC")", (uaddr) str);


	CHKNULL(str);
	result = tnv_create();
	CHKNULL(result);


	tnv_init(result, AMP_TYPE_STR);

	/* Step 2 - Find out length of string. */
	len = strlen(str) + 1;

	if((result->value.as_ptr = STAKE(len)) == NULL)
	{
		tnv_release(result, 1);
		result = NULL;
	}
	else
	{

		/* Step 4 - Populate the return value. */
		TNV_SET_ALLOC(result->flags);

		memcpy(result->value.as_ptr,str, len);
	}

	AMP_DEBUG_EXIT("tnv_from_str","-> %s", str);
	return result;
}

tnv_t*  tnv_from_uint(uint32_t val)
{
	tnv_t *result = tnv_create();
	CHKNULL(result);
	tnv_init(result, AMP_TYPE_UINT);
	result->value.as_uint = val;
	return result;
}

tnv_t*  tnv_from_uvast(uvast val)
{
	tnv_t *result = tnv_create();
	CHKNULL(result);
	tnv_init(result, AMP_TYPE_UVAST);
	result->value.as_uvast = val;
	return result;
}

tnv_t*  tnv_from_vast(vast val)
{
	tnv_t *result = tnv_create();
	CHKNULL(result);
	tnv_init(result, AMP_TYPE_VAST);
	result->value.as_vast = val;
	return result;
}


void tnv_init(tnv_t *val, amp_type_e type)
{
	CHKVOID(val);

	memset(val, 0, sizeof(tnv_t));
	val->type = type;
}


/*
 * This may be called with a NULL encoder for sizing, so only bail if we
 * get something other than CBorErrorOutOfMemory.
 *
 * Note: ION AMP will never have a name.
 * Note: ION AMP will always have type and value.
 */
CborError tnv_serialize(CborEncoder *encoder, void *item)
{
	CborError err;
	CborEncoder array_enc;
	tnv_t *tnv = (tnv_t *) item;

	CHKUSR(encoder, CborErrorIO);
	CHKUSR(tnv, CborErrorIO);

	/* Step 1: Create the Array Encoder. */
	err = cbor_encoder_create_array(encoder, &array_enc, 2);
	if((err != CborNoError) && (err != CborErrorOutOfMemory))
	{
		AMP_DEBUG_ERR("tnv_serialize_helper","Cbor Error: %d", err);
		return err;
	}

	/* Step 2: Write the Type/Flag Byte. */
	err = cut_enc_byte(&array_enc, (tnv->type & 0x7F));
	if((err != CborNoError) && (err != CborErrorOutOfMemory))
	{
		AMP_DEBUG_ERR("tnv_serialize_helper","Cbor Error: %d", err);
		return err;
	}

	err = tnv_serialize_value(&array_enc,tnv);
	if((err != CborNoError) && (err != CborErrorOutOfMemory))
	{
		AMP_DEBUG_ERR("tnv_serialize_helper","Cbor Error: %d", err);
		return err;
	}

	return cbor_encoder_close_container(encoder, &array_enc);
}


CborError tnv_serialize_value(CborEncoder *encoder, void *item)
{
	CborError err;
	tnv_t *tnv = (tnv_t *) item;

	CHKUSR(tnv,CborErrorIO);

	/* Step 3: Encode the value. */
	switch(tnv->type)
	{
		case AMP_TYPE_EDD:
	    case AMP_TYPE_CNST:
	    case AMP_TYPE_ARI:
	    case AMP_TYPE_LIT:  err = ari_serialize(encoder, (ari_t*) tnv->value.as_ptr);  break;

	    case AMP_TYPE_CTRL: err = ctrl_serialize(encoder, (ctrl_t*) tnv->value.as_ptr);break;
		case AMP_TYPE_MAC:  err = macdef_serialize(encoder, (macdef_t*)tnv->value.as_ptr);   break;

		case AMP_TYPE_RPT:  err = rpt_serialize(encoder, (rpt_t*)tnv->value.as_ptr);   break;
		case AMP_TYPE_RPTTPL: err = rpttpl_serialize(encoder, (rpttpl_t*)tnv->value.as_ptr); break;

		case AMP_TYPE_TBR:
		case AMP_TYPE_SBR:  err = rule_serialize(encoder, (rule_t*)tnv->value.as_ptr);   break;

		case AMP_TYPE_TBL:  err = tbl_serialize(encoder, (tbl_t*)tnv->value.as_ptr);   break;
		case AMP_TYPE_TBLT: err = tblt_serialize(encoder,(tblt_t*)tnv->value.as_ptr);  break;

		case AMP_TYPE_VAR:  err = var_serialize(encoder, (var_t*)tnv->value.as_ptr);   break;
		case AMP_TYPE_AC:   err = ac_serialize(encoder, (ac_t*)tnv->value.as_ptr);     break;
		case AMP_TYPE_EXPR: err = expr_serialize(encoder, (expr_t*)tnv->value.as_ptr); break;
		case AMP_TYPE_BYTESTR: err = blob_serialize(encoder, (blob_t*)tnv->value.as_ptr); break;

		/* Primitive Types and Derived Types */
		case AMP_TYPE_STR: err = cbor_encode_text_string(encoder, (char*) tnv->value.as_ptr, strlen((char*)tnv->value.as_ptr)); break;

		case AMP_TYPE_BOOL:
		case AMP_TYPE_BYTE:  err = cut_enc_byte(encoder, tnv->value.as_byte); break;

		case AMP_TYPE_INT:   err = cbor_encode_int(encoder, (int64_t) tnv->value.as_int); break;
		case AMP_TYPE_UINT:  err = cbor_encode_uint(encoder, (uint64_t) tnv->value.as_uint); break;
		case AMP_TYPE_VAST:  err = cbor_encode_int(encoder, tnv->value.as_vast); break;
		case AMP_TYPE_TV:
		case AMP_TYPE_TS:
		case AMP_TYPE_UVAST:  err = cbor_encode_int(encoder, tnv->value.as_uvast); break;
		case AMP_TYPE_REAL32: err = cbor_encode_float(encoder, tnv->value.as_real32); break;
		case AMP_TYPE_REAL64: err = cbor_encode_double(encoder, tnv->value.as_real64); break;

		case AMP_TYPE_TNV:
		case AMP_TYPE_TNVC:
		case AMP_TYPE_OPER:
		default:
			/* Invalid type. */
			err = CborErrorIllegalType;
	}

	return err;
}

blob_t* tnv_serialize_value_wrapper(tnv_t *tnv)
{
	return cut_serialize_wrapper(TNV_DEFAULT_ENC_SIZE, tnv, tnv_serialize_value);
}


blob_t* tnv_serialize_wrapper(tnv_t *tnv)
{
	return cut_serialize_wrapper(TNV_DEFAULT_ENC_SIZE, tnv, tnv_serialize);
}

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

void tnv_release(tnv_t *val, int destroy)
{
	CHKVOID(val);

	if(TNV_IS_ALLOC(val->flags) && (val->value.as_ptr != NULL))
	{
		switch(val->type)
		{
			case AMP_TYPE_CNST:
			case AMP_TYPE_EDD:
			case AMP_TYPE_ARI:
			case AMP_TYPE_LIT: ari_release((ari_t*)val->value.as_ptr, 1);     break;

			case AMP_TYPE_CTRL: ctrl_release((ctrl_t*) val->value.as_ptr, 1);  break;

			case AMP_TYPE_MAC:
			case AMP_TYPE_AC:   ac_release((ac_t*) val->value.as_ptr, 1);      break;

			case AMP_TYPE_OPER: op_release((op_t*) val->value.as_ptr, 1);      break;
			case AMP_TYPE_RPT:  rpt_release((rpt_t*) val->value.as_ptr, 1);    break;
			case AMP_TYPE_RPTTPL: rpttpl_release((rpttpl_t*) val->value.as_ptr, 1);  break;

			case AMP_TYPE_TBR:
			case AMP_TYPE_SBR:  rule_release((rule_t*) val->value.as_ptr, 1);    break;

			case AMP_TYPE_TBL:  tbl_release((tbl_t*) val->value.as_ptr, 1);    break;
			case AMP_TYPE_TBLT: tblt_release((tblt_t*) val->value.as_ptr, 1);  break;

			case AMP_TYPE_VAR:  var_release((var_t*) val->value.as_ptr, 1);    break;
			case AMP_TYPE_EXPR: expr_release((expr_t*) val->value.as_ptr, 1);  break;
			case AMP_TYPE_BYTESTR: blob_release((blob_t*) val->value.as_ptr, 1);  break;

			case AMP_TYPE_STR:	SRELEASE((char*)val->value.as_ptr); break;
			default: break;
		}
	}

	if(destroy != 0)
	{
		SRELEASE(val);
	}
}


/******************************************************************************
 *
 * \par Function Name: tnv_to_int
 *
 * \par Conversion function, relying on language type casting
 *      rather than trying to be clever based on internal
 *      representation.
 *
 * \retval The converted value of the value type.
 *
 * \param[in]  val 	    The value type to be converted.
 * \param[out] success  Whether the conversion is a valid one.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  09/02/18  E. Birrane     Update to latest spec. (JHU/APL)
 *****************************************************************************/

int32_t  tnv_to_int(tnv_t val, int *success)
{
	int32_t result = 0;

	*success = AMP_OK;

	if(TNV_IS_MAP(val.flags))
	{
		*success = AMP_FAIL;
		return 0;
	}

	switch(val.type)
	{
	case AMP_TYPE_BOOL:
	case AMP_TYPE_BYTE:   result = (int32_t) val.value.as_byte; break;
	case AMP_TYPE_INT:    result = val.value.as_int; break;
	case AMP_TYPE_UINT:   result = (int32_t) val.value.as_uint; break;
	case AMP_TYPE_VAST:   result = (int32_t) val.value.as_vast; break;
	case AMP_TYPE_UVAST:  result = (int32_t) val.value.as_uvast; break;
	case AMP_TYPE_REAL32: result = (int32_t) val.value.as_real32; break;
	case AMP_TYPE_REAL64: result = (int32_t) val.value.as_real64; break;
	default: *success = AMP_FAIL; break;
	}
	return result;
}




/******************************************************************************
 *
 * \par Function Name: tnv_to_real32
 *
 * \par Conversion function, relying on language type casting
 *      rather than trying to be clever based on internal
 *      representation.
 *
 * \retval The converted value of the value type.
 *
 * \param[in]  val 	The value type to be converted.
 * \param[out] success  Whether the conversion is a valid one.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  09/02/18  E. Birrane     Update to latest spec. (JHU/APL)
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
	case AMP_TYPE_BYTE:  result = (float) val.value.as_byte; break;
	case AMP_TYPE_INT:   result = (float) val.value.as_int; break;
	case AMP_TYPE_UINT:  result = (float) val.value.as_uint; break;
	case AMP_TYPE_VAST:  result = (float) val.value.as_vast; break;
	case AMP_TYPE_UVAST: result = (float) val.value.as_uvast; break;
	case AMP_TYPE_REAL32:result = (float) val.value.as_real32; break;
	case AMP_TYPE_REAL64:result = (float) val.value.as_real64; break;
	default: *success = AMP_FAIL; break;
	}
	return result;
}



/******************************************************************************
 *
 * \par Function Name: tnv_to_real64
 *
 * \par Conversion function, relying on language type casting
 *      rather than trying to be clever based on internal
 *      representation.
 *
 * \retval The converted value of the value type.
 *
 * \param[in]  val 	The value type to be converted.
 * \param[out] success  Whether the conversion is a valid one.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  09/02/18  E. Birrane     Update to latest spec. (JHU/APL)
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
	case AMP_TYPE_BYTE:   result = (double) val.value.as_byte; break;

	case AMP_TYPE_INT:   result = (double) val.value.as_int; break;
	case AMP_TYPE_UINT:  result = (double) val.value.as_uint; break;
	case AMP_TYPE_VAST:  result = (double) val.value.as_vast; break;
	case AMP_TYPE_UVAST: result = (double) val.value.as_uvast; break;
	case AMP_TYPE_REAL32:result = (double) val.value.as_real32; break;
	case AMP_TYPE_REAL64:result = (double) val.value.as_real64; break;
	default: *success = AMP_FAIL; break;
	}

	return result;
}




/******************************************************************************
 *
 * \par Function Name: tnv_to_type
 *
 * \par Conversion function, relying on language type casting
 *      rather than trying to be clever based on internal
 *      representation.
 *
 * \retval Whether the conversion is valid.
 *
 * \param[in] val 	The tnv to be converted.
 * \param[in] type  The type to convert to.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  09/02/18  E. Birrane     Update to latest spec. (JHU/APL)
 *****************************************************************************/
int tnv_to_type(tnv_t *val, amp_type_e type)
{
	int success = AMP_FAIL;

	CHKZERO(val);

	if(val->type == type)
	{
		return AMP_OK;
	}

	if((type_is_numeric(type) == 0) ||
	   (type_is_numeric(val->type) == 0) ||
	   (TNV_IS_MAP(val->flags)))
	{
		AMP_DEBUG_ERR("val_cvt_type","Can't cvt from %d to %d.", val->type, type);
		return AMP_FAIL;
	}

	val->type = type;
	switch(val->type)
	{
	// TODO: Eval byte/bool from int semantics.
		case AMP_TYPE_BOOL:   val->value.as_byte   = tnv_to_uint(*val, &success);   break;
		case AMP_TYPE_BYTE:   val->value.as_byte   = tnv_to_uint(*val, &success);   break;
		case AMP_TYPE_INT:    val->value.as_int    = tnv_to_int(*val, &success);    break;
		case AMP_TYPE_UINT:   val->value.as_uint   = tnv_to_uint(*val, &success);   break;
		case AMP_TYPE_VAST:	  val->value.as_vast   = tnv_to_vast(*val, &success);   break;
		case AMP_TYPE_UVAST:  val->value.as_uvast  = tnv_to_uvast(*val, &success);  break;
		case AMP_TYPE_REAL32: val->value.as_real32 = tnv_to_real32(*val, &success); break;
		case AMP_TYPE_REAL64: val->value.as_real64 = tnv_to_real64(*val, &success); break;
		default: success = AMP_FAIL; break;
	}

	return success;
}



/******************************************************************************
 *
 * \par Function Name: tnv_to_uint
 *
 * \par Conversion function, relying on language type casting
 *      rather than trying to be clever based on internal
 *      representation.
 *
 * \retval The converted value of the value type.
 *
 * \param[in]  val 	The value type to be converted.
 * \param[out] success  Whether the conversion is a valid one.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  09/02/18  E. Birrane     Update to latest spec. (JHU/APL)
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
 *
 * \par Function Name: tnv_to_uvast
 *
 * \par Conversion function, relying on language type casting
 *      rather than trying to be clever based on internal
 *      representation.
 *
 * \retval The converted value of the value type.
 *
 * \param[in]  val 	The value type to be converted.
 * \param[out] success  Whether the conversion is a valid one.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  09/02/18  E. Birrane     Update to latest spec. (JHU/APL)
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
		case AMP_TYPE_BYTE:   result = (uvast) val.value.as_byte; break;
		case AMP_TYPE_INT:    result = (uvast) val.value.as_int; break;
		case AMP_TYPE_UINT:   result = (uvast) val.value.as_uint; break;
		case AMP_TYPE_VAST:   result = (uvast) val.value.as_vast; break;
		case AMP_TYPE_UVAST:  result =  val.value.as_uvast; break;
		case AMP_TYPE_REAL32: result = (uvast) val.value.as_real32; break;
		case AMP_TYPE_REAL64: result = (uvast) val.value.as_real64; break;
		default: *success = AMP_FAIL; break;
	}
	return result;
}



/******************************************************************************
 *
 * \par Function Name: tnv_to_vast
 *
 * \par Conversion function, relying on language type casting
 *      rather than trying to be clever based on internal
 *      representation.
 *
 * \retval The converted value of the value type.
 *
 * \param[in]  val 	The value type to be converted.
 * \param[out] success  Whether the conversion is a valid one.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  09/02/18  E. Birrane     Update to latest spec. (JHU/APL)
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
	case AMP_TYPE_BYTE:   result = (vast) val.value.as_byte; break;
	case AMP_TYPE_INT:    result = (vast) val.value.as_int; break;
	case AMP_TYPE_UINT:   result = (vast) val.value.as_uint; break;
	case AMP_TYPE_VAST:   result = val.value.as_vast; break;
	case AMP_TYPE_UVAST:  result = (vast) val.value.as_uvast; break;
	case AMP_TYPE_REAL32: result = (vast) val.value.as_real32; break;
	case AMP_TYPE_REAL64: result = (vast) val.value.as_real64; break;
	default: *success = AMP_FAIL; break;
	}
	return result;
}





// TNVC Stuff.




int tnvc_append(tnvc_t *dst, tnvc_t *src)
{
	uint8_t i;
	vec_idx_t max;
	int success = AMP_FAIL;



	AMP_DEBUG_ENTRY("tnvc_append","("ADDR_FIELDSPEC","ADDR_FIELDSPEC")", (uaddr) dst, (uaddr) src);

	CHKUSR(dst, AMP_FAIL);
	CHKUSR(src, AMP_FAIL);

	max = vec_size(src->values);
	/* Appending an empty list is easy... */
	if(max == 0)
	{
		return AMP_OK;
	}

	vec_make_room(&(dst->values), max);

	for(i = 0; i < max; i++)
	{
		tnv_t *cur_tnv = (tnv_t *)vec_at(src->values, i);
		CHKUSR(cur_tnv != NULL, AMP_FAIL);
		tnv_t *new_tnv = tnv_copy_ptr(*cur_tnv);
		if(new_tnv == NULL)
		{
			return AMP_FAIL;
		}
		success = vec_insert(&(dst->values), new_tnv, NULL);
		CHKUSR(success == AMP_OK, success);
	}

	return AMP_OK;
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
 *
 * \par Function Name: tnvc_clear
 *
 * \par Purpose: Clears the TNVC, including all contents within.
 *
 * \return N/A
 *
 * \param[in]   tdc  A pointer to the TDC
 *
 * \par Notes:
 *  - This function does not destroy the tdc itself, it merely clears
 *    out the contents of a TDC. THerefore, it is suitable to clear both
 *    dynamically and statically scoped TDCs.
 *
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  03/13/15  J.P Mayer      Initial implementation,
 *  06/27/15  E. Birrane     Ported from datalist to TDC.
 *****************************************************************************/

void tnvc_clear(tnvc_t* tnvc)
{
	CHKVOID(tnvc);
	vec_clear(&(tnvc->values));
}



/******************************************************************************
 *
 * \par Function Name: tnvc_compare
 *
 * \par Purpose: Compares two TDCs. This is a logical comparison of values,
 *               not a comparison of addresses.
 *
 * \return -1 System Error
 *          0 TDCs equal
 *          1 TDCs do not match
 *
 * \param[in]   t1	The first TDC to compare
 * \param[in]   t2  The second TDC to compare.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  06/11/16  E. Birrane     Initial implementation,
 *****************************************************************************/

int tnvc_compare(tnvc_t *t1, tnvc_t *t2)
{
	uint8_t i = 0;
	int diff = 0;
	int success;

	CHKERR(t1);
	CHKERR(t2);

	if(vec_num_entries(t1->values) != vec_num_entries(t2->values))
	{
		return 1;
	}

	for(i = 0; i < vec_num_entries(t1->values); i++)
	{
		if((diff = tnv_compare(vec_at(t1->values, i), vec_at(t2->values, i))) != 0)
		{
			return diff;
		}
	}

	return 0;
}




/******************************************************************************
 *
 * \par Function Name: tnvc_create
 *
 * \par Purpose: Creates a typed data collection.
 *
 * \return The allocated TDC.
 *
 * \param[in]   dc	      The datacol to deserialize from (optional)
 * \param[in]   types     Array of dc entry types.
 * \param[in]   type_cnt  # types.
 *
 * \par Notes:
 *
 *  - It is possible to create a typed data collection from an existing
 *    data collection, if the types of each datum comprising the collection
 *    are provided. When constructing a TDC from a DC, the original DC and
 *    type array are preserved in the resulting TDC and MUST NOT be deallocated
 *    prior to deallocating the encapsulating TDC (after a tdc_clear()).
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  03/13/15  J.P Mayer      Initial implementation,
 *  06/27/15  E. Birrane     Migrated from datalist to TDC.
 *****************************************************************************/

tnvc_t *tnvc_create(uint8_t num)
{
	tnvc_t *result = NULL;
	int success = AMP_FAIL;

	/* Step 0: Allocate the new TDC. */
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

	/* Step 2: Return the created TDC. */
	return result;
}



/******************************************************************************
 *
 * \par Function Name: tdc_copy
 *
 * \par Purpose: Deep-copies an existing TDC.
 *
 * \return The TDC, or NULL if something failed.
 *
 * \param[in]   tdc		The TDC being copied.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/04/15  E. Birrane     Initial Implementation.
 *****************************************************************************/

tnvc_t* tnvc_copy(tnvc_t *src)
{
	tnvc_t *result = tnvc_create(vec_size(src->values));

	CHKNULL(result);
	CHKNULL(src);

	if(tnvc_append(result, src) != AMP_OK)
	{
		tnvc_release(result, 1);
		return NULL;
	}

	return result;
}





/******************************************************************************
 *
 * \par Function Name: tdc_deserialize
 *
 * \par Purpose: Deserializes a tdc from a byte array
 *
 * \return The datalist, or NULL if something failed.
 *
 * \param[in]   buffer		the byte array
 * \param[in]   buffer_size	The length of buffer
 * \param[out]  bytes_used	The length of buffer which was deserialized.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  03/13/15  J.P Mayer      Initial implementation,
 *  06/27/15  E. Birrane     Ported from datalist to TDC.
 *  09/09/15  E. Birrane     Updated TDC to match AMP-01 spec
 *  04/15/16  E. Birrane     Updated to blob_t
 *****************************************************************************/

tnvc_t tnvc_deserialize(CborValue *it, int *success)
{
	uint8_t type;
	tnvc_t result;


	/* Get the first byte. */

	*success = cut_get_cbor_numeric(it, AMP_TYPE_BYTE, &type);

	if(*success != AMP_OK)
	{
		AMP_DEBUG_ERR("tnvc_deserialize","Can't get TNVC form byte.",NULL);
		return result;
	}

	switch(type)
	{
		case TNVC_TVC:
			result = tnvc_deserialize_tvc(it, success);
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

	return result;
}


/*
 * TODO: Document this API pattern.
 */

tnvc_t *tnvc_deserialize_ptr(CborValue *it, int *success)
{
	tnvc_t *result = NULL;

	if((result = tnvc_create(0)) == NULL)
	{
		AMP_DEBUG_ERR("tnvc_deserialize_ptr","Can't allocate new struct.", NULL);
		*success = AMP_FAIL;
	}

	*result = tnvc_deserialize(it, success);
	if(*success != AMP_OK)
	{
		tnvc_release(result, 1);
		result = NULL;
	}

	return result;
}

tnvc_t*  tnvc_deserialize_raw(blob_t *data, int *success)
{
	CborParser parser;
	CborValue it;

	CHKNULL(success);
	*success = AMP_FAIL;
	CHKNULL(data);

	if(cbor_parser_init(data->value, data->length, 0, &parser, &it) != CborNoError)
	{
		return NULL;
	}

	return tnvc_deserialize_ptr(&it, success);
}

/*
 * This should look like [bytestring,E(v1),E(v2)...]
 */

static tnvc_t tnvc_deserialize_tvc(CborValue *it, int *success)
{
	tnvc_t result;
	blob_t types;
	int i;

	CborError err;
	CborValue array_it;
	size_t array_len = 0;


	AMP_DEBUG_ENTRY("tnvc_deserialize_tvc","(0x"ADDR_FIELDSPEC",0x"ADDR_FIELDSPEC")", (uaddr) it, (uaddr) success);

	*success = AMP_OK;

	if(((err = cbor_value_is_array(it)) != CborNoError) ||
	   ((err = cbor_value_get_array_length(it, &array_len)) != CborNoError) ||
	   (array_len <= 1))
	{
		AMP_DEBUG_ERR("tnvs_deserialize_tvc","CBOR Array Error %d with length %ld", err, array_len);
		*success = AMP_FAIL;
		return result;
	}

	if((err = cbor_value_enter_container(it, &array_it)) != CborNoError)
	{
		AMP_DEBUG_ERR("tnvs_deserialize_tvc","Unable to enter array. Error %d.", err);
		*success = AMP_FAIL;
		return result;
	}



	types = blob_deserialize(&array_it, success);
	if(*success != AMP_OK)
	{
		AMP_DEBUG_ERR("tnvs_deserialize_tvc","Can't get types set.", NULL);
		cbor_value_leave_container(it, &array_it);
		return result;
	}

	/* Sanity check. If the array has N elements, the first element is the types array and it should
	 * have N-1 types identified.
	 */
	if(types.length != (array_len - 1))
	{
		AMP_DEBUG_ERR("tnvc_deserialize_tvc","Size mismtach: array size %ld and types length %ld.", array_len, types.length);
		*success = AMP_FAIL;
		blob_release(&types, 0);
		cbor_value_leave_container(it, &array_it);
		return result;
	}

	result.values = vec_create(array_len - 1, tnv_cb_del,tnv_cb_comp,tnv_cb_copy, VEC_FLAG_AS_STACK, success);

	if(*success != AMP_OK)
	{
		AMP_DEBUG_ERR("tnvc_deserialize_tvc","Can;t allocate vector.", NULL);
		blob_release(&types, 0);
		cbor_value_leave_container(it, &array_it);
		return result;
	}

	/* For each type... */
	for(i = 0; i < types.length; i++)
	{
		tnv_t *val = tnv_create();
		*success = AMP_FAIL;

		if(val != NULL)
		{
			val->type = types.value[i];
			if((*success = tnv_deserialize_val_by_type(&array_it, val)) == AMP_OK)
			{
				vec_insert(&(result.values), val, NULL);
			}
		}

		if(*success != AMP_OK)
		{
			break;
		}
	}

	if(*success != AMP_OK)
	{
		AMP_DEBUG_ERR("tnv_deserialize_tvc","Failed to deserialize values (last was %d).", i);
		blob_release(&types, 0);
		vec_release(&(result.values), 0);
		cbor_value_leave_container(it, &array_it);
		return result;
	}

	cbor_value_leave_container(it, &array_it);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: tnvc_get
 *
 * \par Purpose: Returns the data collection entry for a given index
 *
 * \return The datacol_entry_t, or NULL if something failed.
 *
 *
 * \param[in]   tdc		The typed data collection
 * \param[in]   index	The zero-based index index
 *
 * \par Notes:
 *  - The returned entry is NOT allocated and must NOT be released by the caller.
 *
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  03/13/15  J.P Mayer      Initial implementation,
 *  06/27/15  E. Birrane     Ported from datalist to TDC.
 *  04/15/16  E. Birrane     Updated to blob_t
 *****************************************************************************/

tnv_t* tnvc_get(tnvc_t* tnvc, uint8_t index)
{
	tnv_t *result = NULL;

	CHKNULL(tnvc);
	result =  vec_at(tnvc->values, index);
	return result;
}


/******************************************************************************
 *
 * \par Function Name: tnvc_get_count
 *
 * \par Purpose: Returns the number of items in a typed data collection
 *
 * \return  The number of elements
 *
 * \param[in]   tdc		The typed data collection
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  03/13/15  J.P Mayer      Initial implementation,
 *  06/27/15  E. Birrane     Ported from datalist to TDC.
 *****************************************************************************/

uint8_t tnvc_get_count(tnvc_t* tnvc)
{
	CHKZERO(tnvc);
	return vec_num_entries(tnvc->values);
}


tnv_enc_e tnvc_get_encode_type(tnvc_t *tnvc)
{
	CHKUSR(tnvc,TNVC_UNK);
	return TNVC_TVC;
}

tnv_enc_e tnvc_get_type(tnvc_t *tnvc, uint8_t index)
{
	tnv_t *tnv;
	int success = AMP_OK;

	CHKUSR(tnvc, TNVC_UNK);

	tnv = (tnv_t *) vec_at(tnvc->values, index);

	return (tnv == NULL) ? TNVC_UNK : tnv->type;
}

blob_t    tnvc_get_types(tnvc_t *tnvc, int *success)
{
	blob_t types;
	uint8_t length = 0;
	uint8_t i = 0;

	*success = AMP_FAIL;
	CHKUSR(tnvc, types);

	length = vec_num_entries(tnvc->values);
	blob_init(&types, NULL, 0, length);
	for(i = 0; i < length; i++)
	{
		tnv_enc_e cur_type = tnvc_get_type(tnvc, i);
		blob_append(&types, (uint8_t*)&cur_type, 1);
	}

	*success = AMP_OK;
	return types;
}


int tnvc_init(tnvc_t *tnvc, size_t num)
{
	int success;
	CHKERR(tnvc);

	tnvc->values = vec_create(num, tnv_del_fn, tnv_comp_fn, tnv_cb_copy, VEC_FLAG_AS_STACK, &success);

	return AMP_OK;
}

/******************************************************************************
 *
 * \par Function Name: tdc_insert
 *
 * \par Purpose: Inserts an item to the end of the TDC, taking into account
 *               its type. If the type is variable length (blob, string),
 *               the size field must be filled out... memory is also
 *				 allocated and the parameter copied, so remember to free
 *				 your list!
 *
 * \return       The type inserted, or AMP_TYPE_UNK if something failed.
 *
 *
 * \param[in]   tdc     The typed data collection.
 * \param[in]   data	A pointer to the data which shall be inserted.
 * \param[in]   size	The size of the variable pointed to by data,
 *                      required for variable length stuff and 0 otherwise
 *
 * \par Notes:
 *  - The data entry is deep-copied into the TDC, so the data parameter
 *    MUST be de-allocated by the calling function.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  03/13/15  J.P Mayer      Initial implementation,
 *  06/27/15  E. Birrane     Ported from datalist to TDC.
 *  06/11/16  E. Birrane     Update to handle inserting to empty TDC.
 *****************************************************************************/

int tnvc_insert(tnvc_t* tnvc, tnv_t *tnv)
{
	tnv_t *new_tnv = NULL;
	int result;

	CHKUSR(tnvc,AMP_FAIL);
	CHKUSR(tnv,AMP_FAIL);

	if((new_tnv = tnv_copy_ptr(*tnv)) == NULL)
	{
		AMP_DEBUG_ERR("tnvc_insert","Can't copy TNV.", NULL);
		return AMP_SYSERR;
	}

	if((result = vec_insert(&(tnvc->values), new_tnv, NULL)) != AMP_OK)
	{
		AMP_DEBUG_ERR("tnvc_insert","Error vector inserting.", NULL);
		tnv_release(new_tnv, 1);
	}

	return result;
}



void tnvc_release(tnvc_t *tnvc, int destroy)
{
	CHKVOID(tnvc);

	if(destroy)
	{
		vec_release(&(tnvc->values), 0);
		SRELEASE(tnvc);
	}
	else
	{
		vec_clear(&(tnvc->values));
	}
}


/*
 * This may be called with a NULL encoder for sizing, so only bail if we
 * get something other than CBorErrorOutOfMemory.
 *
 * Note: ION AMP will never have a name.
 * Note: ION AMP will always have type and value.
 */

CborError tnvc_serialize(CborEncoder *encoder, void *item)
{
	CborError err;
	tnv_enc_e type;
	tnvc_t *tnvc = (tnvc_t *) item;

	/* Step 1: Figure out what flavor of TNVC we are serializing. */
	/* Note; Write now this is always a TVC. */

	if((type = tnvc_get_encode_type(tnvc)) == TNVC_UNK)
	{
		AMP_DEBUG_ERR("tnvc_serialize","Unknown type.", NULL);
		return CborErrorIO;
	}

	/* Step 2: Write the type as the first encoded byte. */
	err = cut_enc_byte(encoder, type);
	if((err != CborNoError) && (err != CborErrorOutOfMemory))
	{
		AMP_DEBUG_ERR("tnvc_serialize","Cbor Error: %d. Type %d", err, type);
		return err;
	}

	/* Step 3: Based on type, serialize the rest of the collection. */
	switch(type)
	{
		case TNVC_TVC:
			err = tnvc_serialize_tvc(encoder, tnvc);
			break;

		default:
			err = CborErrorIO;
			break;
	}


	return err;
}




/******************************************************************************
 *
 * \par Function Name: tdc_serialize
 *
 * \par Purpose: Generate full, serialized version of a Typed Data Collection.
 *               A serialized Typed Data Collection is of the form:
 *
 * \par +---------+-----------+-------------+     +-------------+
 *      | # BLOBs | TYPE BLOB | DATA BLOB 1 | ... | DATA BLOB N |
 *      |  [SDNV] |   [BLOB]  |   [BLOB]    |     |    [BLOB]   |
 *      +---------+-----------+-------------+     +-------------+
 *
 * \retval NULL - Failure serializing
 * 		   !NULL - Serialized collection.
 *
 * \param[in]  tdc    The Typed Data Collection to be serialized.
 * \param[out] size   The size of the resulting serialized Collection.
 *
 * \par Notes:
 *		1. The result is allocated on the memory pool and must be released when
 *         no longer needed.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  03/13/15  J.P Mayer      Initial implementation,
 *  06/27/15  E. Birrane     Ported from datalist to TDC.
 *  09/09/15  E. Birrane     Updated to latest AMP spec.
 *****************************************************************************/

blob_t* tnvc_serialize_wrapper(tnvc_t *tnvc)
{

	return cut_serialize_wrapper(TNVC_DEFAULT_ENC_SIZE, tnvc, tnvc_serialize);
}


/*
 * This is an array of N + 1 elements, where N is the number of values and
 * the +1 is a type bytestring at the beginning.
 */

CborError tnvc_serialize_tvc(CborEncoder *encoder, tnvc_t *tnvc)
{
	CborError err;
	CborEncoder array_enc;
	blob_t types;
	int success;
	uint8_t i;

	CHKUSR(encoder, CborErrorIO);
	CHKUSR(tnvc, CborErrorIO);


	/* Step 1: Create the Array Encoder. */
	err = cbor_encoder_create_array(encoder, &array_enc, vec_num_entries(tnvc->values)+1);
	if((err != CborNoError) && (err != CborErrorOutOfMemory))
	{
		AMP_DEBUG_ERR("tnvc_serialize_tvc","Cbor Error: %d", err);
		return err;
	}

	/* Step 2: Construct and serialize the type bytestring. */
	types = tnvc_get_types(tnvc, &success);

	err = blob_serialize(&array_enc, &types);
	blob_release(&types, 0);

	if((err != CborNoError) && (err != CborErrorOutOfMemory))
	{
		AMP_DEBUG_ERR("tnvc_serialize_tvc","Can't serialize types", NULL);
		return err;
	}

	/* Step 4: For each value, encode it. */
	for(i = 0; i < types.length; i++)
	{
		blob_t *data = NULL;

		tnv_t *tnv = (tnv_t*) vec_at(tnvc->values,i);

		/* Go through the trouble of getting a serialized string because we don't
		 * want the array encoder to think parts of the serialized value are
		 * different indices in the array...
		 */
		if((data = tnv_serialize_wrapper(tnv)) == NULL)
		{
			AMP_DEBUG_ERR("tnvc_serialize_tvc","Can't serialize TNV: %d", i);
			err = CborErrorIO;
			break;
		}

		/* Serialize as a bytestring for this array element. */
		err = cbor_encode_byte_string(&array_enc, data->value, data->length);
		blob_release(data, 1);
		if((err != CborNoError) && (err != CborErrorOutOfMemory))
		{
			AMP_DEBUG_ERR("tnvc_serialize_tvc","Can't serialize TNV: %d", i);
			break;
		}
	}

	if((err != CborNoError) && (err != CborErrorOutOfMemory))
	{
		AMP_DEBUG_ERR("tnvc_serialize_tvc","Cbor Error: %d", err);
		cbor_encoder_close_container(encoder, &array_enc);
		return err;
	}

	return cbor_encoder_close_container(encoder, &array_enc);
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
