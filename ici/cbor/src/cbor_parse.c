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

typedef struct parser
{
    const uint8_t * ptr;
    size_t remaining_size;
    void * private_data;
    fn_tara_cbor_parse_handler_t handler;
    tara_cbor_encoded_key_types_t encoding_type;
} parser_t;

static const size_t encoding_text_size = 8;
static const char * encoding_text_none = "TARANONE";
static const char * encoding_text_tag = "TARATAG ";
static const char * encoding_text_name = "TARANAME";

// Internal Functions ------------------------------------------------------------------------------

#define ADVANCE_PTR(size) \
    (parser->ptr += size, \
     parser->remaining_size -= size, \
     size)

#define GET_AND_ADVANCE_PTR(type) \
    (type *)(parser->ptr - ADVANCE_PTR(sizeof (type)))

static tara_errors_t parse_type(
    parser_t * parser,
    tara_cbor_major_types_t * major_type,
    tara_cbor_additional_type_t * additional_type)
{
    RETURN_IF(TARA_ERROR_OUT_OF_MEMORY, sizeof (uint8_t) > parser->remaining_size);

    uint8_t type = *GET_AND_ADVANCE_PTR(uint8_t);
    *major_type = type & TARA_CBOR_MAJOR_TYPE_MASK;
    additional_type->asInt = type & TARA_CBOR_ADDITIONAL_TYPE_MASK;

    return TARA_ERROR_SUCCESS;
}

static tara_errors_t parse_size_type(
    parser_t * parser,
    tara_cbor_size_types_t size_type,
    uint64_t * size)
{
    if (TARA_CBOR_SIZE_TYPE_8 > size_type)
    {
        *size = size_type;
        return TARA_ERROR_SUCCESS;
    }

    switch (size_type)
    {
    case TARA_CBOR_SIZE_TYPE_8:
        RETURN_IF(TARA_ERROR_OUT_OF_MEMORY, sizeof (uint8_t) > parser->remaining_size);
        *size = *GET_AND_ADVANCE_PTR(uint8_t);
        break;

    case TARA_CBOR_SIZE_TYPE_16:
        RETURN_IF(TARA_ERROR_OUT_OF_MEMORY, sizeof (uint16_t) > parser->remaining_size);
        *size = tara_nbo16(*GET_AND_ADVANCE_PTR(uint16_t));
        break;

    case TARA_CBOR_SIZE_TYPE_32:
        RETURN_IF(TARA_ERROR_OUT_OF_MEMORY, sizeof (uint32_t) > parser->remaining_size);
        *size = tara_nbo32(*GET_AND_ADVANCE_PTR(uint32_t));
        break;

    case TARA_CBOR_SIZE_TYPE_64:
        RETURN_IF(TARA_ERROR_OUT_OF_MEMORY, sizeof (uint64_t) > parser->remaining_size);
        *size = tara_nbo64(*GET_AND_ADVANCE_PTR(uint64_t));
        break;

    case TARA_CBOR_SIZE_TYPE_INDEFINITE:
        return TARA_ERROR_NO_DATA;

    default:
        return TARA_ERROR_INVALID_PARAMETER;
    }

    return TARA_ERROR_SUCCESS;
}

static tara_errors_t parse_posint(
    parser_t * parser,
    tara_cbor_size_types_t size_type,
    tara_cbor_element_data_t * element_data)
{
    uint64_t value;
    RETURN_IF_FAILED(parse_size_type(parser, size_type, &value));

    element_data->value.uint64 = value;

    if (UINT8_MAX >= value)
    {
        element_data->type = TARA_CBOR_ELEMENT_TYPE_POSINT8;
    }
    else if (UINT16_MAX >= value)
    {
        element_data->type = TARA_CBOR_ELEMENT_TYPE_POSINT16;
    }
    else if (UINT32_MAX >= value)
    {
        element_data->type = TARA_CBOR_ELEMENT_TYPE_POSINT32;
    }
    else
    {
        element_data->type = TARA_CBOR_ELEMENT_TYPE_POSINT64;
    }

    return TARA_ERROR_SUCCESS;
}

static tara_errors_t parse_negint(
    parser_t * parser,
    tara_cbor_size_types_t size_type,
    tara_cbor_element_data_t * element_data)
{
    int64_t value;
    RETURN_IF_FAILED(parse_size_type(parser, size_type, (uint64_t *)&value));

    value = ~value;
    element_data->value.int64 = value;

    if (INT8_MIN <= value)
    {
        element_data->type = TARA_CBOR_ELEMENT_TYPE_NEGINT8;
    }
    else if (INT16_MIN <= value)
    {
        element_data->type = TARA_CBOR_ELEMENT_TYPE_NEGINT16;
    }
    else if (INT32_MIN <= value)
    {
        element_data->type = TARA_CBOR_ELEMENT_TYPE_NEGINT32;
    }
    else
    {
        element_data->type = TARA_CBOR_ELEMENT_TYPE_NEGINT64;
    }

    return TARA_ERROR_SUCCESS;
}

static tara_errors_t parse_bytes(
    parser_t * parser,
    tara_cbor_size_types_t size_type,
    tara_cbor_element_data_t * element_data)
{
    size_t size;
    tara_errors_t error = parse_size_type(parser, size_type, &size);
    RETURN_IF(error, FAILED(error) && TARA_ERROR_NO_DATA != error);

    if (SUCCEEDED(error))
    {
        element_data->type = TARA_CBOR_ELEMENT_TYPE_BYTES;
        element_data->value.bytes.ptr = parser->ptr;
        element_data->value.bytes.size = size;

        (void)ADVANCE_PTR(size);
    }
    else
    {
        element_data->type = TARA_CBOR_ELEMENT_TYPE_VARBYTES;
        element_data->value.bytes.ptr = NULL;
        element_data->value.bytes.size = 0;
    }

    return TARA_ERROR_SUCCESS;
}

static tara_errors_t parse_text(
    parser_t * parser,
    tara_cbor_size_types_t size_type,
    tara_cbor_element_data_t * element_data)
{
    size_t size;
    tara_errors_t error = parse_size_type(parser, size_type, &size);
    RETURN_IF(error, FAILED(error) && TARA_ERROR_NO_DATA != error);

    if (SUCCEEDED(error))
    {
        element_data->type = TARA_CBOR_ELEMENT_TYPE_TEXT;
        element_data->value.text.ptr = (const char *)parser->ptr;
        element_data->value.text.size = size;

        (void)ADVANCE_PTR(size);
    }
    else
    {
        element_data->type = TARA_CBOR_ELEMENT_TYPE_VARTEXT;
        element_data->value.text.ptr = NULL;
        element_data->value.text.size = 0;
    }

    return TARA_ERROR_SUCCESS;
}

static tara_errors_t parse_array(
    parser_t * parser,
    tara_cbor_size_types_t size_type,
    tara_cbor_element_data_t * element_data)
{
    size_t size;
    tara_errors_t error = parse_size_type(parser, size_type, &size);
    RETURN_IF(error, FAILED(error) && TARA_ERROR_NO_DATA != error);

    if (SUCCEEDED(error))
    {
        element_data->type = TARA_CBOR_ELEMENT_TYPE_ARRAY;
        element_data->value.array_size = size;
    }
    else
    {
        element_data->type = TARA_CBOR_ELEMENT_TYPE_VARARRAY;
        element_data->value.array_size = 0;
    }

    return TARA_ERROR_SUCCESS;
}

static tara_errors_t parse_map(
    parser_t * parser,
    tara_cbor_size_types_t size_type,
    tara_cbor_element_data_t * element_data)
{
    size_t size;
    tara_errors_t error = parse_size_type(parser, size_type, &size);
    RETURN_IF(error, FAILED(error) && TARA_ERROR_NO_DATA != error);

    if (SUCCEEDED(error))
    {
        element_data->type = TARA_CBOR_ELEMENT_TYPE_MAP;
        element_data->value.map_size = size;
    }
    else
    {
        element_data->type = TARA_CBOR_ELEMENT_TYPE_VARMAP;
        element_data->value.map_size = 0;
    }

    return TARA_ERROR_SUCCESS;
}

static tara_errors_t parse_semtag(
    parser_t * parser,
    tara_cbor_size_types_t size_type,
    tara_cbor_element_data_t * element_data)
{
    element_data->has_sem_tag = TRUE;
    return parse_size_type(parser, size_type, &element_data->sem_tag);
}

static tara_errors_t parse_float(
    parser_t * parser,
    tara_cbor_primitive_types_t prim_type,
    tara_cbor_element_data_t * element_data)
{
    switch (prim_type)
    {
    case TARA_CBOR_PRIMITIVE_TYPE_HPFP:
    {
        element_data->type = TARA_CBOR_ELEMENT_TYPE_FLT16;

        RETURN_IF(TARA_ERROR_OUT_OF_MEMORY, sizeof (uint16_t) > parser->remaining_size);
        tara_bits_hpfp_t hpfp;
        hpfp.raw_value = tara_nbo16(*GET_AND_ADVANCE_PTR(uint16_t));

        tara_bits_dpfp_t dpfp;
        dpfp.encoding.mantissa_low = 0;
        dpfp.encoding.mantissa_high = hpfp.encoding.mantissa << 10;
        if (0 == hpfp.encoding.exponent)
        {
            dpfp.encoding.exponent = 0;
        }
        else if (TARA_HPFP_EXPONENT_MASK == hpfp.encoding.exponent)
        {
            dpfp.encoding.exponent = TARA_DPFP_EXPONENT_MASK;
        }
        else
        {
            dpfp.encoding.exponent =
                hpfp.encoding.exponent + (TARA_DPFP_EXPONENT_MAX - TARA_HPFP_EXPONENT_MAX);
        }
        dpfp.encoding.sign = hpfp.encoding.sign;

        element_data->value.flt64 = dpfp.value;
        break;
    }

    case TARA_CBOR_PRIMITIVE_TYPE_SPFP:
    {
        element_data->type = TARA_CBOR_ELEMENT_TYPE_FLT32;

        tara_bits_spfp_t spfp;
        spfp.raw_value = tara_nbo32(*GET_AND_ADVANCE_PTR(uint32_t));
        element_data->value.flt64 = spfp.value;
        break;
    }

    case TARA_CBOR_PRIMITIVE_TYPE_DPFP:
    {
        element_data->type = TARA_CBOR_ELEMENT_TYPE_FLT64;

        tara_bits_dpfp_t dpfp;
        dpfp.raw_value = tara_nbo64(*GET_AND_ADVANCE_PTR(uint64_t));
        element_data->value.flt64 = dpfp.value;
        break;
    }

    default:
        return TARA_ERROR_INVALID_PARAMETER;
    }

    return TARA_ERROR_SUCCESS;
}

static tara_errors_t parse_primitive(
    parser_t * parser,
    tara_cbor_primitive_types_t prim_type,
    tara_cbor_element_data_t * element_data)
{
    switch (prim_type)
    {
    case TARA_CBOR_PRIMITIVE_TYPE_FALSE:
        element_data->type = TARA_CBOR_ELEMENT_TYPE_BOOL;
        element_data->value.boolean = FALSE;
        break;

    case TARA_CBOR_PRIMITIVE_TYPE_TRUE:
        element_data->type = TARA_CBOR_ELEMENT_TYPE_BOOL;
        element_data->value.boolean = TRUE;
        break;

    case TARA_CBOR_PRIMITIVE_TYPE_NULL:
        element_data->type = TARA_CBOR_ELEMENT_TYPE_NULL;
        break;

    case TARA_CBOR_PRIMITIVE_TYPE_UNDEFINED:
        element_data->type = TARA_CBOR_ELEMENT_TYPE_UNDEFINED;
        break;

    case TARA_CBOR_PRIMITIVE_TYPE_UINT8:
        element_data->type = TARA_CBOR_ELEMENT_TYPE_POSINT8;
        element_data->value.boolean = *GET_AND_ADVANCE_PTR(uint8_t);
        break;

    case TARA_CBOR_PRIMITIVE_TYPE_HPFP:
    case TARA_CBOR_PRIMITIVE_TYPE_SPFP:
    case TARA_CBOR_PRIMITIVE_TYPE_DPFP:
        RETURN_IF_FAILED(parse_float(parser, prim_type, element_data));
        break;

    case TARA_CBOR_PRIMITIVE_TYPE_BREAK:
        element_data->type = TARA_CBOR_ELEMENT_TYPE_VARBREAK;
        break;

    default:
        return TARA_ERROR_UNEXPECTED;
    }

    return TARA_ERROR_SUCCESS;
}

typedef enum parse_behaviors
{
    PARSE_BEHAVIOR_NONE          = 0x00,
    PARSE_BEHAVIOR_SIZE_FIXED    = 0x01,
    PARSE_BEHAVIOR_SIZE_VARIABLE = 0x02,
    PARSE_BEHAVIOR_SIZE_MASK     = 0x0F,
    PARSE_BEHAVIOR_KEY_SKIP      = 0x10,
    PARSE_BEHAVIOR_KEY_MASK      = 0xF0,
} parse_behaviors_t;

static tara_errors_t parse(
    parser_t * parser,
    parse_behaviors_t behavior,
    size_t count)
{
    tara_cbor_element_data_t element_data;
    tara_zero_struct(element_data);

    while (0 < parser->remaining_size)
    {
        tara_cbor_major_types_t major_type;
        tara_cbor_additional_type_t additional_type;

        tara_bool_t exit = FALSE;
        tara_bool_t recurse = FALSE;
        parse_behaviors_t next_behavior = PARSE_BEHAVIOR_NONE;
        size_t next_count = 0;

        RETURN_IF_FAILED(parse_type(parser, &major_type, &additional_type));

        // Do a pre-check for BREAK
        if (TARA_CBOR_MAJOR_TYPE_PRIMITIVE == major_type &&
            TARA_CBOR_PRIMITIVE_TYPE_BREAK == additional_type.primitive)
        {
            RETURN_IF(TARA_ERROR_UNEXPECTED,
                      PARSE_BEHAVIOR_SIZE_VARIABLE != (behavior & PARSE_BEHAVIOR_SIZE_MASK));
        }
        else if (PARSE_BEHAVIOR_KEY_SKIP != (behavior & PARSE_BEHAVIOR_KEY_MASK))
        {
            switch (parser->encoding_type)
            {
            case TARA_CBOR_ENCODED_KEY_TYPE_TAG:
                RETURN_IF(TARA_ERROR_UNEXPECTED, TARA_CBOR_MAJOR_TYPE_POSINT != major_type);
                RETURN_IF_FAILED(parse_size_type(parser, additional_type.size, &element_data.key.tag));
                RETURN_IF_FAILED(parse_type(parser, &major_type, &additional_type));
                element_data.has_key = TRUE;
                break;

            case TARA_CBOR_ENCODED_KEY_TYPE_NAME:
                RETURN_IF(TARA_ERROR_UNEXPECTED, TARA_CBOR_MAJOR_TYPE_TEXT != major_type);
                RETURN_IF_FAILED(parse_text(parser, additional_type.size, &element_data));
                RETURN_IF(TARA_ERROR_UNEXPECTED, TARA_CBOR_ELEMENT_TYPE_TEXT != element_data.type);
                element_data.key.name.ptr = element_data.value.text.ptr;
                element_data.key.name.size = element_data.value.text.size;
                RETURN_IF_FAILED(parse_type(parser, &major_type, &additional_type));
                element_data.has_key = TRUE;
                break;

            default:
                // Do nothing
                break;
            }
        }

        switch (major_type)
        {
        case TARA_CBOR_MAJOR_TYPE_POSINT:
            RETURN_IF_FAILED(parse_posint(parser, additional_type.size, &element_data));
            break;

        case TARA_CBOR_MAJOR_TYPE_NEGINT:
            RETURN_IF_FAILED(parse_negint(parser, additional_type.size, &element_data));
            break;

        case TARA_CBOR_MAJOR_TYPE_BYTE:
            RETURN_IF_FAILED(parse_bytes(parser, additional_type.size, &element_data));
            if (TARA_CBOR_SIZE_TYPE_INDEFINITE == additional_type.size)
            {
                recurse = TRUE;
                next_behavior = PARSE_BEHAVIOR_SIZE_VARIABLE | PARSE_BEHAVIOR_KEY_SKIP;
            }
            break;

        case TARA_CBOR_MAJOR_TYPE_TEXT:
            RETURN_IF_FAILED(parse_text(parser, additional_type.size, &element_data));
            if (TARA_CBOR_SIZE_TYPE_INDEFINITE == additional_type.size)
            {
                recurse = TRUE;
                next_behavior = PARSE_BEHAVIOR_SIZE_VARIABLE | PARSE_BEHAVIOR_KEY_SKIP;
            }
            break;

        case TARA_CBOR_MAJOR_TYPE_ARRAY:
            RETURN_IF_FAILED(parse_array(parser, additional_type.size, &element_data));
            recurse = TRUE;
            if (TARA_CBOR_SIZE_TYPE_INDEFINITE == additional_type.size)
            {
                next_behavior = PARSE_BEHAVIOR_SIZE_VARIABLE | PARSE_BEHAVIOR_KEY_SKIP;
            }
            else
            {
                next_behavior = PARSE_BEHAVIOR_SIZE_FIXED | PARSE_BEHAVIOR_KEY_SKIP;
                next_count = element_data.value.array_size;
            }
            break;

        case TARA_CBOR_MAJOR_TYPE_MAP:
            RETURN_IF_FAILED(parse_map(parser, additional_type.size, &element_data));
            recurse = TRUE;
            if (TARA_CBOR_SIZE_TYPE_INDEFINITE == additional_type.size)
            {
                next_behavior = PARSE_BEHAVIOR_SIZE_VARIABLE;
            }
            else
            {
                next_behavior = PARSE_BEHAVIOR_SIZE_FIXED;
                next_count = element_data.value.map_size;
            }
            break;

        case TARA_CBOR_MAJOR_TYPE_SEMTAG:
            RETURN_IF(TARA_ERROR_INVALID_PARAMETER, element_data.has_sem_tag);
            RETURN_IF_FAILED(parse_semtag(parser, additional_type.size, &element_data));
            continue;

        case TARA_CBOR_MAJOR_TYPE_PRIMITIVE:
            RETURN_IF_FAILED(parse_primitive(parser, additional_type.size, &element_data));
            exit = TARA_CBOR_PRIMITIVE_TYPE_BREAK == additional_type.primitive;
            break;

        default:
            return TARA_ERROR_NOT_IMPLEMENTED;
        }

        RETURN_IF_FAILED(parser->handler(
            parser->encoding_type,
            &element_data,
            parser->private_data));

        if (recurse)
        {
            RETURN_IF_FAILED(parse(parser, next_behavior, next_count));
        }

        if (PARSE_BEHAVIOR_SIZE_FIXED == (behavior & PARSE_BEHAVIOR_SIZE_MASK) &&
            --count == 0)
        {
            exit = TRUE;
        }

        RETURN_IF(TARA_ERROR_SUCCESS, exit);

        element_data.has_key = FALSE;
        element_data.has_sem_tag = FALSE;
    }

    return TARA_ERROR_SUCCESS;
}

static tara_errors_t parse_encoding_type(
    parser_t * parser)
{
    parser->encoding_type = TARA_CBOR_ENCODED_KEY_TYPE_UNKNOWN;

    uint8_t type = *parser->ptr;
    RETURN_IF(TARA_ERROR_OUT_OF_MEMORY, sizeof type > parser->remaining_size);

    tara_cbor_major_types_t major_type = type & TARA_CBOR_MAJOR_TYPE_MASK;
    RETURN_IF(TARA_ERROR_SUCCESS, TARA_CBOR_MAJOR_TYPE_TEXT != major_type);

    tara_cbor_additional_type_t additional_type = { .size = type & TARA_CBOR_ADDITIONAL_TYPE_MASK };
    RETURN_IF(TARA_ERROR_SUCCESS, encoding_text_size != additional_type.size);

    RETURN_IF(TARA_ERROR_OUT_OF_MEMORY, sizeof (type) + encoding_text_size > parser->remaining_size);
    const char * encoding_text = (const char *)parser->ptr + 1;
    if (0 == strncmp(encoding_text_none, encoding_text, encoding_text_size))
    {
        parser->encoding_type = TARA_CBOR_ENCODED_KEY_TYPE_NONE;
    }
    else if (0 == strncmp(encoding_text_tag, encoding_text, encoding_text_size))
    {
        parser->encoding_type = TARA_CBOR_ENCODED_KEY_TYPE_TAG;
    }
    else if (0 == strncmp(encoding_text_name, encoding_text, encoding_text_size))
    {
        parser->encoding_type = TARA_CBOR_ENCODED_KEY_TYPE_NAME;
    }
    else
    {
        return TARA_ERROR_SUCCESS;
    }

    parser->ptr += sizeof (type) + encoding_text_size;
    parser->remaining_size -= sizeof (type) + encoding_text_size;

    return TARA_ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// External Functions ------------------------------------------------------------------------------

int tara_cbor_parse(
    const void * cbor_data,
    size_t cbor_data_size,
    void * private_data,
    fn_tara_cbor_parse_handler_t parse_handler)
{
    RETURN_IF_NULL(TARA_ERROR_INVALID_PARAMETER, cbor_data);
    RETURN_IF(TARA_ERROR_INVALID_PARAMETER, 0 == cbor_data_size);
    RETURN_IF_NULL(TARA_ERROR_INVALID_PARAMETER, parse_handler);

    // Initialize parser
    parser_t parser;
    tara_zero_struct(parser);
    parser.ptr = (uint8_t *)cbor_data;
    parser.remaining_size = cbor_data_size;
    parser.private_data = private_data;
    parser.handler = parse_handler;
    RETURN_IF_FAILED(parse_encoding_type(&parser));

    return parse(&parser, PARSE_BEHAVIOR_KEY_SKIP, 0);
}
