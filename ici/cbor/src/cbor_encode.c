/*
 * Copyright (c) Antara Teknik, LLC. All rights reserved.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <math.h>
#include "bitwise.h"
#include "cbor.h"


// Internal Types ----------------------------------------------------------------------------------

#define SEM_TAG_IDENTIFIER 39

typedef enum encode_options
{
    ENCODE_OPTION_NO_KEY,
    ENCODE_OPTION_KEY,
} encode_options_t;

static const size_t encoding_text_size = 8;
static const char * encoding_text_none = "TARANONE";
static const char * encoding_text_tag = "TARATAG ";
static const char * encoding_text_name = "TARANAME";

// Internal Functions ------------------------------------------------------------------------------

#define GET_AND_ADVANCE_PTR(type) \
    (type *)(encoder->ptr += sizeof (type), \
             encoder->size += sizeof (type), \
             encoder->remaining_size -= sizeof (type), \
             encoder->ptr - sizeof (type))

#define define_encode_uint(bits) \
    static tara_errors_t encode_uint##bits( \
        tara_cbor_encoder_t * encoder, \
        uint##bits##_t value) \
    { \
        RETURN_IF(TARA_ERROR_OUT_OF_MEMORY, sizeof value > encoder->remaining_size); \
        *GET_AND_ADVANCE_PTR(uint##bits##_t) = tara_nbo##bits(value); \
        return TARA_ERROR_SUCCESS; \
    }

static inline uint8_t tara_nbo8(uint8_t value)
{
    return value;
}

define_encode_uint(8)
define_encode_uint(16)
define_encode_uint(32)
define_encode_uint(64)

static inline tara_errors_t encode_type(
    tara_cbor_encoder_t * encoder,
    tara_cbor_major_types_t major_type,
    int additional_type)
{
    RETURN_IF(TARA_ERROR_OUT_OF_MEMORY, sizeof (uint8_t) > encoder->remaining_size);
    *GET_AND_ADVANCE_PTR(uint8_t) = (uint8_t)(major_type | additional_type);
    return TARA_ERROR_SUCCESS;
}

static inline tara_errors_t encode_size_type(
    tara_cbor_encoder_t * encoder,
    tara_cbor_major_types_t major_type,
    uint64_t value)
{
    if (value <= TARA_CBOR_MAX_INT_DIRECT_ENCODE)
    {
        return encode_type(encoder, major_type, (int)value);
    }
    else if (value <= UINT8_MAX)
    {
        RETURN_IF_FAILED(encode_type(encoder, major_type, TARA_CBOR_SIZE_TYPE_8));
        return encode_uint8(encoder, (uint8_t)value);
    }
    else if (value <= UINT16_MAX)
    {
        RETURN_IF_FAILED(encode_type(encoder, major_type, TARA_CBOR_SIZE_TYPE_16));
        return encode_uint16(encoder, (uint16_t)value);
    }
    else if (value <= UINT32_MAX)
    {
        RETURN_IF_FAILED(encode_type(encoder, major_type, TARA_CBOR_SIZE_TYPE_32));
        return encode_uint32(encoder, (uint32_t)value);
    }
    else
    {
        RETURN_IF_FAILED(encode_type(encoder, major_type, TARA_CBOR_SIZE_TYPE_64));
        return encode_uint64(encoder, value);
    }

    return TARA_ERROR_NOT_IMPLEMENTED;
}

static tara_errors_t encode_raw_bytes(
    tara_cbor_encoder_t * encoder,
    const uint8_t bytes[],
    size_t size)
{
    RETURN_IF(TARA_ERROR_OUT_OF_MEMORY, size > encoder->remaining_size);

    memcpy(encoder->ptr, bytes, size);
    encoder->ptr += size;
    encoder->size += size;
    encoder->remaining_size -= size;

    return TARA_ERROR_SUCCESS;
}

static tara_errors_t try_encode_fp_simple(
    tara_cbor_encoder_t * encoder,
    tara_bits_dpfp_t dpfp)
{
    tara_bits_hpfp_t hpfp;
    switch (fpclassify(dpfp.value))
    {
    case FP_NORMAL:
        return TARA_ERROR_INVALID_PARAMETER;

    case FP_INFINITE:
        hpfp.raw_value = 0x7c00;
        hpfp.encoding.sign = dpfp.encoding.sign;
        break;

    case FP_NAN:
        hpfp.raw_value = 0x7e00;
        break;

    case FP_ZERO:
    case FP_SUBNORMAL:
        hpfp.raw_value = 0;
        hpfp.encoding.sign = dpfp.encoding.sign;
        break;

    default:
        return TARA_ERROR_UNEXPECTED;
    }

    RETURN_IF_FAILED(encode_type(
        encoder,
        TARA_CBOR_MAJOR_TYPE_PRIMITIVE,
        TARA_CBOR_PRIMITIVE_TYPE_HPFP));

    return encode_uint16(encoder, hpfp.raw_value);
}

static tara_errors_t encode_schema_element(
    tara_cbor_encoder_t * encoder,
    tara_cbor_schema_types_t type,
    void * value,
    size_t count)
{
    #define ENCODE_CASE(case_val, type, fn_enc) \
        case case_val: \
        { \
            type * tvalue = (type *)value; \
            for (; 0 != count; --count, ++tvalue) \
            { \
                RETURN_IF_FAILED(fn_enc(encoder, *tvalue)); \
            } \
        } \
        break;

    RETURN_IF(TARA_ERROR_INVALID_PARAMETER, 0 == count);

    if (1 < count && TARA_CBOR_SCHEMA_TYPE_TEXT != type)
    {
        RETURN_IF_FAILED(encode_size_type(encoder, TARA_CBOR_MAJOR_TYPE_ARRAY, count));
    }

    switch (type)
    {
    ENCODE_CASE(TARA_CBOR_SCHEMA_TYPE_BOOL, tara_bool_t, tara_cbor_encode_bool)
    ENCODE_CASE(TARA_CBOR_SCHEMA_TYPE_INT8, int8_t, tara_cbor_encode_int)
    ENCODE_CASE(TARA_CBOR_SCHEMA_TYPE_INT16, int16_t, tara_cbor_encode_int)
    ENCODE_CASE(TARA_CBOR_SCHEMA_TYPE_INT32, int32_t, tara_cbor_encode_int)
    ENCODE_CASE(TARA_CBOR_SCHEMA_TYPE_INT64, int64_t, tara_cbor_encode_int)
    ENCODE_CASE(TARA_CBOR_SCHEMA_TYPE_UINT8, uint8_t, tara_cbor_encode_int)
    ENCODE_CASE(TARA_CBOR_SCHEMA_TYPE_UINT16, uint16_t, tara_cbor_encode_int)
    ENCODE_CASE(TARA_CBOR_SCHEMA_TYPE_UINT32, uint32_t, tara_cbor_encode_int)
    ENCODE_CASE(TARA_CBOR_SCHEMA_TYPE_UINT64, uint64_t, tara_cbor_encode_int)
    ENCODE_CASE(TARA_CBOR_SCHEMA_TYPE_FLT32, float, tara_cbor_encode_fp)
    ENCODE_CASE(TARA_CBOR_SCHEMA_TYPE_FLT64, double, tara_cbor_encode_fp)

    case TARA_CBOR_SCHEMA_TYPE_TEXT:
        return tara_cbor_encode_text(encoder, (const char *)value);

    default:
        return TARA_ERROR_NOT_IMPLEMENTED;
    }

    #undef ENCODE_CASE

    return TARA_ERROR_SUCCESS;
}

static tara_errors_t encode_object_keyed(
    tara_cbor_encoder_t * encoder,
    void * object,
    const tara_cbor_schema_t schema[],
    ssize_t schema_size,
    encode_options_t encode_option,
    size_t * num_elements)
{
    RETURN_IF(TARA_ERROR_INVALID_OPERATION, 0 != encoder->var);

    uint8_t * objptr = (uint8_t *)object;

    for (; 0 < schema_size; ++schema, --schema_size)
    {
        if (ENCODE_OPTION_KEY == encode_option)
        {
            if (TARA_CBOR_ENCODED_KEY_TYPE_TAG == encoder->encoding_type)
            {
                RETURN_IF_FAILED(tara_cbor_encode_int(
                    encoder,
                    schema->key.tag));
            }
            else if (TARA_CBOR_ENCODED_KEY_TYPE_NAME == encoder->encoding_type)
            {
                RETURN_IF_FAILED(tara_cbor_encode_text(
                    encoder,
                    schema->key.name));
            }
            else
            {
                return TARA_ERROR_NOT_IMPLEMENTED;
            }
        }

        if (TARA_CBOR_SCHEMA_TYPE_TUPLE == schema->type)
        {
            if (TARA_CBOR_ENCODED_KEY_TYPE_NONE == encoder->encoding_type)
            {
                RETURN_IF_FAILED(encode_size_type(
                    encoder,
                    TARA_CBOR_MAJOR_TYPE_SEMTAG,
                    SEM_TAG_IDENTIFIER));

                RETURN_IF_FAILED(encode_size_type(
                    encoder,
                    TARA_CBOR_MAJOR_TYPE_ARRAY,
                    schema->count));
            }
            else
            {
                RETURN_IF_FAILED(encode_size_type(
                    encoder,
                    TARA_CBOR_MAJOR_TYPE_MAP,
                    schema->count));
            }

            size_t num_sub_elements = 0;

            RETURN_IF_FAILED(encode_object_keyed(
                encoder,
                objptr + schema->member_offset,
                schema + 1,
                schema->count,
                TARA_CBOR_ENCODED_KEY_TYPE_NONE == encoder->encoding_type
                    ? ENCODE_OPTION_NO_KEY
                    : ENCODE_OPTION_KEY,
                &num_sub_elements));

            schema_size -= num_sub_elements;
            schema += num_sub_elements;

            *num_elements += num_sub_elements + 1;
        }
        else if (TARA_CBOR_SCHEMA_TYPE_SEQUENCE == schema->type)
        {
            RETURN_IF_NULL(TARA_ERROR_INVALID_PARAMETER, encoder->sequence_handler);
            RETURN_IF(TARA_ERROR_INVALID_PARAMETER, 1 == schema_size);
            RETURN_IF(TARA_ERROR_INVALID_PARAMETER, 0 != schema[1].member_offset);

            RETURN_IF_FAILED(encode_type(
                encoder,
                TARA_CBOR_MAJOR_TYPE_ARRAY,
                TARA_CBOR_SIZE_TYPE_INDEFINITE));

            size_t num_sub_elements;

            void * data;
	    size_t index;
            for (index = 0;
                 NULL != (data = encoder->sequence_handler(object, schema, index));
                 ++index)
            {
                num_sub_elements = 0;

                RETURN_IF_FAILED(encode_object_keyed(
                    encoder,
                    data,
                    schema + 1,
                    1,
                    ENCODE_OPTION_NO_KEY,
                    &num_sub_elements));
            }

            RETURN_IF_FAILED(encode_type(
                encoder,
                TARA_CBOR_MAJOR_TYPE_PRIMITIVE,
                TARA_CBOR_PRIMITIVE_TYPE_BREAK));

            schema_size -= num_sub_elements;
            schema += num_sub_elements;

            *num_elements += num_sub_elements + 1;
        }
        else
        {
            RETURN_IF_FAILED(encode_schema_element(
                encoder,
                schema->type,
                objptr + schema->member_offset,
                schema->count));

            *num_elements += 1;
        }
    }

    return TARA_ERROR_SUCCESS;
}


// External Functions ------------------------------------------------------------------------------

int tara_cbor_encoder_init(
    tara_cbor_encoder_t * encoder,
    void * buffer,
    size_t buffer_size,
    fn_tara_cbor_encode_sequence_handler_t sequence_handler)
{
    RETURN_IF_NULL(TARA_ERROR_INVALID_PARAMETER, buffer);
    RETURN_IF(TARA_ERROR_INVALID_PARAMETER, 0 == buffer_size);

    tara_zero_struct(*encoder);
    encoder->ptr = (uint8_t *)buffer;
    encoder->remaining_size = buffer_size;
    encoder->sequence_handler = sequence_handler;

    return TARA_ERROR_SUCCESS;
}

int tara_cbor_encode_bool(
    tara_cbor_encoder_t * encoder,
    tara_bool_t value)
{
    RETURN_IF(TARA_ERROR_INVALID_OPERATION, 0 != encoder->var);
    return encode_type(
        encoder,
        TARA_CBOR_MAJOR_TYPE_PRIMITIVE,
        value ? TARA_CBOR_PRIMITIVE_TYPE_TRUE : TARA_CBOR_PRIMITIVE_TYPE_FALSE);
}

int tara_cbor_encode_int(
    tara_cbor_encoder_t * encoder,
    int64_t value)
{
    RETURN_IF(TARA_ERROR_INVALID_OPERATION, 0 != encoder->var);
    return value >= 0 ? encode_size_type(encoder, TARA_CBOR_MAJOR_TYPE_POSINT, value)
                      : encode_size_type(encoder, TARA_CBOR_MAJOR_TYPE_NEGINT, (uint64_t)~value);
}

int tara_cbor_encode_fp(
    tara_cbor_encoder_t * encoder,
    double value)
{
    RETURN_IF(TARA_ERROR_INVALID_OPERATION, 0 != encoder->var);

    tara_bits_dpfp_t dpfp;
    dpfp.value = value;

    tara_errors_t error = try_encode_fp_simple(encoder, dpfp);
    RETURN_IF(error, TARA_ERROR_INVALID_PARAMETER != error);

    const int32_t exponent = ((int32_t)dpfp.encoding.exponent) - TARA_DPFP_EXPONENT_MAX;
    const uint32_t mantissa_bits =
        TARA_DPFP_MANTISSA_BITS -
        TARA_MIN(TARA_DPFP_MANTISSA_BITS, tara_bit_scan_fwd64(dpfp.raw_value & TARA_DPFP_MANTISSA_MASK));

    if ((TARA_HPFP_EXPONENT_MIN <= exponent || TARA_HPFP_EXPONENT_MAX >= exponent) &&
        TARA_HPFP_MANTISSA_BITS >= mantissa_bits)
    {
        RETURN_IF_FAILED(encode_type(
            encoder,
            TARA_CBOR_MAJOR_TYPE_PRIMITIVE,
            TARA_CBOR_PRIMITIVE_TYPE_HPFP));

        tara_bits_hpfp_t hpfp;
        hpfp.encoding.sign = dpfp.encoding.sign;
        hpfp.encoding.exponent = exponent + TARA_HPFP_EXPONENT_MAX;
        hpfp.encoding.mantissa = dpfp.encoding.mantissa_high >> 10;
        return encode_uint16(encoder, hpfp.raw_value);
    }
    else if ((TARA_SPFP_EXPONENT_MIN <= exponent || TARA_SPFP_EXPONENT_MAX >= exponent) &&
             TARA_SPFP_MANTISSA_BITS >= mantissa_bits)
    {
        RETURN_IF_FAILED(encode_type(
            encoder,
            TARA_CBOR_MAJOR_TYPE_PRIMITIVE,
            TARA_CBOR_PRIMITIVE_TYPE_SPFP));

        tara_bits_spfp_t spfp;
        spfp.value = (float)value;
        return encode_uint32(encoder, spfp.raw_value);
    }
    else
    {
        RETURN_IF_FAILED(encode_type(
            encoder,
            TARA_CBOR_MAJOR_TYPE_PRIMITIVE,
            TARA_CBOR_PRIMITIVE_TYPE_DPFP));
        return encode_uint64(encoder, dpfp.raw_value);
    }

    return TARA_ERROR_NOT_IMPLEMENTED;
}

int tara_cbor_encode_bytes(
    tara_cbor_encoder_t * encoder,
    const uint8_t bytes[],
    size_t size)
{
    RETURN_IF(TARA_ERROR_INVALID_OPERATION, 0 != encoder->var);
    RETURN_IF(TARA_ERROR_INVALID_PARAMETER, NULL == bytes && 0 != size);
    RETURN_IF(TARA_ERROR_INVALID_PARAMETER, NULL != bytes && 0 == size);

    if (NULL == bytes)
    {
        return encode_type(encoder, TARA_CBOR_MAJOR_TYPE_BYTE, 0);
    }

    RETURN_IF_FAILED(encode_size_type(encoder, TARA_CBOR_MAJOR_TYPE_BYTE, size));
    return encode_raw_bytes(encoder, bytes, size);
}

int tara_cbor_encode_varbytes_begin(
    tara_cbor_encoder_t * encoder)
{
    RETURN_IF(TARA_ERROR_INVALID_OPERATION, 0 != encoder->var);
    encoder->var = TARA_CBOR_MAJOR_TYPE_BYTE;
    return encode_type(encoder, TARA_CBOR_MAJOR_TYPE_BYTE, TARA_CBOR_SIZE_TYPE_INDEFINITE);
}

int tara_cbor_encode_varbytes_end(
    tara_cbor_encoder_t * encoder)
{
    RETURN_IF(TARA_ERROR_INVALID_OPERATION, TARA_CBOR_MAJOR_TYPE_BYTE != encoder->var);
    encoder->var = 0;
    return encode_type(encoder, TARA_CBOR_MAJOR_TYPE_PRIMITIVE, TARA_CBOR_PRIMITIVE_TYPE_BREAK);
}

int tara_cbor_encode_varbytes(
    tara_cbor_encoder_t * encoder,
    const uint8_t bytes[],
    size_t size)
{
    RETURN_IF(TARA_ERROR_INVALID_OPERATION, TARA_CBOR_MAJOR_TYPE_BYTE != encoder->var);
    RETURN_IF_FAILED(encode_size_type(encoder, TARA_CBOR_MAJOR_TYPE_BYTE, size));
    return encode_raw_bytes(encoder, bytes, size);
}

int tara_cbor_encode_text(
    tara_cbor_encoder_t * encoder,
    const char * text)
{
    return tara_cbor_encode_text_size(encoder, text, NULL == text ? 0 : strlen(text));
}

int tara_cbor_encode_text_size(
    tara_cbor_encoder_t * encoder,
    const char * text,
    size_t text_size)
{
    RETURN_IF(TARA_ERROR_INVALID_OPERATION, 0 != encoder->var);
    RETURN_IF_FAILED(encode_size_type(encoder, TARA_CBOR_MAJOR_TYPE_TEXT, text_size));
    return encode_raw_bytes(encoder, (uint8_t *)text, text_size);
}

int tara_cbor_encode_vartext_begin(
    tara_cbor_encoder_t * encoder)
{
    RETURN_IF(TARA_ERROR_INVALID_OPERATION, 0 != encoder->var);
    encoder->var = TARA_CBOR_MAJOR_TYPE_TEXT;
    return encode_type(encoder, TARA_CBOR_MAJOR_TYPE_TEXT, TARA_CBOR_SIZE_TYPE_INDEFINITE);
}

int tara_cbor_encode_vartext_end(
    tara_cbor_encoder_t * encoder)
{
    RETURN_IF(TARA_ERROR_INVALID_OPERATION, TARA_CBOR_MAJOR_TYPE_TEXT != encoder->var);
    encoder->var = 0;
    return encode_type(encoder, TARA_CBOR_MAJOR_TYPE_PRIMITIVE, TARA_CBOR_PRIMITIVE_TYPE_BREAK);
}

int tara_cbor_encode_vartext(
    tara_cbor_encoder_t * encoder,
    const char * text)
{
    RETURN_IF(TARA_ERROR_INVALID_OPERATION, TARA_CBOR_MAJOR_TYPE_TEXT != encoder->var);
    size_t len = NULL == text ? 0 : strlen(text);
    RETURN_IF_FAILED(encode_size_type(encoder, TARA_CBOR_MAJOR_TYPE_TEXT, len));
    return encode_raw_bytes(encoder, (uint8_t *)text, len);
}

int tara_cbor_encode_object(
    tara_cbor_encoder_t * encoder,
    void * object,
    const tara_cbor_schema_t schema[],
    size_t schema_size)
{
    RETURN_IF(TARA_ERROR_INVALID_OPERATION, 0 != encoder->var);

    encoder->encoding_type = TARA_CBOR_ENCODED_KEY_TYPE_NONE;

    RETURN_IF_FAILED(tara_cbor_encode_text_size(
        encoder,
        encoding_text_none,
        encoding_text_size));

    size_t num_elements = 0;
    RETURN_IF_FAILED(encode_object_keyed(
        encoder,
        object,
        schema,
        schema_size,
        ENCODE_OPTION_NO_KEY,
        &num_elements));

    RETURN_IF(TARA_ERROR_UNEXPECTED, num_elements != schema_size)

    return TARA_ERROR_SUCCESS;
}

int tara_cbor_encode_object_tagged(
    tara_cbor_encoder_t * encoder,
    void * object,
    const tara_cbor_schema_t schema[],
    size_t schema_size)
{
    RETURN_IF(TARA_ERROR_INVALID_OPERATION, 0 != encoder->var);
    RETURN_IF(TARA_ERROR_INVALID_OPERATION, 0 != encoder->size);

    encoder->encoding_type = TARA_CBOR_ENCODED_KEY_TYPE_TAG;


    RETURN_IF_FAILED(tara_cbor_encode_text_size(
        encoder,
        encoding_text_tag,
        encoding_text_size));

    RETURN_IF_FAILED(encode_type(
        encoder,
        TARA_CBOR_MAJOR_TYPE_MAP,
        TARA_CBOR_SIZE_TYPE_INDEFINITE));

    size_t num_elements = 0;
    RETURN_IF_FAILED(encode_object_keyed(
        encoder,
        object,
        schema,
        schema_size,
        ENCODE_OPTION_KEY,
        &num_elements));

    RETURN_IF(TARA_ERROR_UNEXPECTED, num_elements != schema_size)

    RETURN_IF_FAILED(encode_type(
        encoder,
        TARA_CBOR_MAJOR_TYPE_PRIMITIVE,
        TARA_CBOR_PRIMITIVE_TYPE_BREAK));

    return TARA_ERROR_SUCCESS;
}

int tara_cbor_encode_object_named(
    tara_cbor_encoder_t * encoder,
    void * object,
    const tara_cbor_schema_t schema[],
    size_t schema_size)
{
    RETURN_IF(TARA_ERROR_INVALID_OPERATION, 0 != encoder->var);
    RETURN_IF(TARA_ERROR_INVALID_OPERATION, 0 != encoder->size);

    encoder->encoding_type = TARA_CBOR_ENCODED_KEY_TYPE_NAME;


    RETURN_IF_FAILED(tara_cbor_encode_text_size(
        encoder,
        encoding_text_name,
        encoding_text_size));

    RETURN_IF_FAILED(encode_type(
        encoder,
        TARA_CBOR_MAJOR_TYPE_MAP,
        TARA_CBOR_SIZE_TYPE_INDEFINITE));

    size_t num_elements = 0;
    RETURN_IF_FAILED(encode_object_keyed(
        encoder,
        object,
        schema,
        schema_size,
        ENCODE_OPTION_KEY,
        &num_elements));

    RETURN_IF(TARA_ERROR_UNEXPECTED, num_elements != schema_size)

    RETURN_IF_FAILED(encode_type(
        encoder,
        TARA_CBOR_MAJOR_TYPE_PRIMITIVE,
        TARA_CBOR_PRIMITIVE_TYPE_BREAK));

    return TARA_ERROR_SUCCESS;
}
