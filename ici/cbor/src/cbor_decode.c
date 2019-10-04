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


// Internal Functions ------------------------------------------------------------------------------

#define GET_OBJECT_MEMBER(type) \
    (type *)(((uint8_t *)object) + \
             current_schema->member_offset + \
             (sizeof (type) * element_index))

static tara_errors_t decode_element(
    tara_cbor_decoder_t * decoder,
    tara_cbor_encoded_key_types_t encoding_type,
    const tara_cbor_element_data_t * element_data,
    size_t schema_index,
    size_t element_index,
    void * object)
{
    const tara_cbor_schema_t * current_schema = decoder->schema + schema_index;
    const tara_cbor_element_types_t element_type = element_data->type;
    const tara_cbor_element_types_t element_category = element_type &
                                                       TARA_CBOR_ELEMENT_TYPE_CATEGORY_MASK;

    // Process a fixed array of the same element type
    // This corresponds to one of the schema value types (e.g. int, float) with a count
    if (TARA_CBOR_ELEMENT_TYPE_ARRAY == element_type &&
        !element_data->has_sem_tag)
    {
        RETURN_IF(TARA_ERROR_UNEXPECTED, element_data->value.array_size != current_schema->count);

        tara_cbor_decoder_indexer_t * indexer = tara_array_append(decoder->indexers);
        RETURN_IF_NULL(TARA_ERROR_OUT_OF_MEMORY, indexer);

        indexer->type = TARA_CBOR_DECODER_INDEXER_TYPE_ARRAY;
        indexer->count = element_data->value.array_size;
        indexer->schema_index = schema_index;
        indexer->element_index = 0;
        indexer->object = object;

        return TARA_ERROR_SUCCESS;
    }

    #define DECODE_ELEMENT_INT(bits) \
        case TARA_CBOR_SCHEMA_TYPE_INT##bits: \
            if (TARA_CBOR_ELEMENT_TYPE_POS == element_category) \
            { \
                RETURN_IF(TARA_ERROR_UNEXPECTED, INT##bits##_MAX < element_data->value.uint64); \
                *GET_OBJECT_MEMBER(int##bits##_t) = (int##bits##_t)element_data->value.uint64; \
            } \
            else if (TARA_CBOR_ELEMENT_TYPE_NEG == element_category) \
            { \
                RETURN_IF(TARA_ERROR_UNEXPECTED, INT##bits##_MIN > element_data->value.int64); \
                *GET_OBJECT_MEMBER(int##bits##_t) = (int##bits##_t)element_data->value.int64; \
            } \
            else \
            { \
                return TARA_ERROR_UNEXPECTED; \
            } \
            break

    #define DECODE_ELEMENT_UINT(bits) \
        case TARA_CBOR_SCHEMA_TYPE_UINT##bits: \
            RETURN_IF(TARA_ERROR_UNEXPECTED, TARA_CBOR_ELEMENT_TYPE_POS != element_category); \
            RETURN_IF(TARA_ERROR_UNEXPECTED, UINT##bits##_MAX < element_data->value.uint64); \
            *GET_OBJECT_MEMBER(uint##bits##_t) = (uint##bits##_t)element_data->value.uint64; \
            break

    switch (current_schema->type)
    {
    case TARA_CBOR_SCHEMA_TYPE_BOOL:
        RETURN_IF(TARA_ERROR_UNEXPECTED, TARA_CBOR_ELEMENT_TYPE_BOOL != element_type);
        *GET_OBJECT_MEMBER(tara_bool_t) = element_data->value.boolean;
        break;

    DECODE_ELEMENT_INT(8);
    DECODE_ELEMENT_INT(16);
    DECODE_ELEMENT_INT(32);
    DECODE_ELEMENT_INT(64);
    DECODE_ELEMENT_UINT(8);
    DECODE_ELEMENT_UINT(16);
    DECODE_ELEMENT_UINT(32);
    DECODE_ELEMENT_UINT(64);

    case TARA_CBOR_SCHEMA_TYPE_FLT32:
        RETURN_IF(TARA_ERROR_UNEXPECTED, TARA_CBOR_ELEMENT_TYPE_FLT != element_category);
        RETURN_IF(TARA_ERROR_UNEXPECTED, TARA_CBOR_ELEMENT_TYPE_FLT64 == element_type);
        *GET_OBJECT_MEMBER(float) = (float)element_data->value.flt64;
        break;

    case TARA_CBOR_SCHEMA_TYPE_FLT64:
        RETURN_IF(TARA_ERROR_UNEXPECTED, TARA_CBOR_ELEMENT_TYPE_FLT != element_category);
        *GET_OBJECT_MEMBER(double) = element_data->value.flt64;
        break;

    case TARA_CBOR_SCHEMA_TYPE_TEXT:
    {
        RETURN_IF(TARA_ERROR_UNEXPECTED, TARA_CBOR_ELEMENT_TYPE_TEXT != element_type);
        char * str = GET_OBJECT_MEMBER(char);
        size_t size = TARA_MIN(current_schema->count - 1, element_data->value.text.size);
        strncpy(str, element_data->value.text.ptr, size);
        str[size] = '\0';
        break;
    }

    case TARA_CBOR_SCHEMA_TYPE_TUPLE:
    {
        tara_cbor_decoder_indexer_t * indexer = tara_array_append(decoder->indexers);
        RETURN_IF_NULL(TARA_ERROR_OUT_OF_MEMORY, indexer);

        switch (encoding_type)
        {
        case TARA_CBOR_ENCODED_KEY_TYPE_NONE:
            RETURN_IF(TARA_ERROR_UNEXPECTED, !(element_data->has_sem_tag &&
                                               SEM_TAG_IDENTIFIER == element_data->sem_tag));
            RETURN_IF(TARA_ERROR_UNEXPECTED, TARA_CBOR_ELEMENT_TYPE_ARRAY != element_type);
            RETURN_IF(TARA_ERROR_UNEXPECTED,
                      element_data->value.array_size != current_schema->count);
            indexer->count = element_data->value.array_size;
            break;

        case TARA_CBOR_ENCODED_KEY_TYPE_TAG:
        case TARA_CBOR_ENCODED_KEY_TYPE_NAME:
            RETURN_IF(TARA_ERROR_UNEXPECTED, TARA_CBOR_ELEMENT_TYPE_MAP != element_type);
            // Keyed schema objects can skip tuple elements they are not interested in,
            // so the schema count is used instead of the encoded element count
            RETURN_IF(TARA_ERROR_UNEXPECTED,
                      element_data->value.map_size < current_schema->count);
            indexer->count = current_schema->count;
            break;

        default:
            return TARA_ERROR_UNEXPECTED;
        }

        indexer->type = TARA_CBOR_DECODER_INDEXER_TYPE_TUPLE;
        indexer->schema_index = schema_index + 1;
        indexer->element_index = 0;
        indexer->object = (uint8_t *)object + current_schema->member_offset;

        break;
    }

    case TARA_CBOR_SCHEMA_TYPE_SEQUENCE:
    {
        RETURN_IF(TARA_ERROR_UNEXPECTED, TARA_CBOR_ELEMENT_TYPE_VARARRAY != element_type);

        tara_cbor_decoder_indexer_t * indexer = tara_array_append(decoder->indexers);
        RETURN_IF_NULL(TARA_ERROR_OUT_OF_MEMORY, indexer);

        indexer->type = TARA_CBOR_DECODER_INDEXER_TYPE_SEQUENCE;
        indexer->schema_index = schema_index + 1;
        indexer->element_index = 0;
        indexer->object = object;

        break;
    }

    default:
        return TARA_ERROR_NOT_IMPLEMENTED;
    }

    #undef DECODE_ELEMENT_INT
    #undef DECODE_ELEMENT_UINT

    return TARA_ERROR_SUCCESS;
}

static tara_errors_t preprocess_indexer(
    tara_cbor_decoder_t * decoder,
    const tara_cbor_element_data_t * element_data,
    tara_cbor_decoder_indexer_t ** out_indexer,
    size_t * out_schema_index,
    size_t * out_element_index,
    void ** out_object)
{
    size_t schema_offset = 0;
    while (TRUE)
    {
        RETURN_IF(TARA_ERROR_UNEXPECTED, tara_array_empty(decoder->indexers));
        tara_cbor_decoder_indexer_t * indexer = tara_array_end(decoder->indexers) - 1;

        *out_indexer = indexer;
        *out_schema_index = indexer->schema_index;
        *out_element_index = indexer->element_index;
        *out_object = indexer->object;

        switch (indexer->type)
        {
        case TARA_CBOR_DECODER_INDEXER_TYPE_ROOT:
            RETURN_IF(TARA_ERROR_UNEXPECTED, 1 != tara_array_size(decoder->indexers));
            indexer->schema_index += schema_offset;
            *out_schema_index = indexer->schema_index;
            return TARA_ERROR_SUCCESS;

        case TARA_CBOR_DECODER_INDEXER_TYPE_ARRAY:
            RETURN_IF(TARA_ERROR_SUCCESS, indexer->count != indexer->element_index);
            tara_array_remove(decoder->indexers, indexer);
            break;

        case TARA_CBOR_DECODER_INDEXER_TYPE_TUPLE:
            if (indexer->count != indexer->element_index)
            {
                *out_schema_index = indexer->schema_index + indexer->element_index;
                *out_element_index = 0;
                return TARA_ERROR_SUCCESS;
            }
            else
            {
                schema_offset += indexer->count;
                tara_array_remove(decoder->indexers, indexer);
            }
            break;

        case TARA_CBOR_DECODER_INDEXER_TYPE_SEQUENCE:
            if (TARA_CBOR_ELEMENT_TYPE_VARBREAK == element_data->type)
            {
                schema_offset += 1;
                tara_array_remove(decoder->indexers, indexer);
            }
            else
            {
                RETURN_IF_NULL(TARA_ERROR_INVALID_PARAMETER, decoder->sequence_handler);
                *out_element_index = 0;
                *out_object = decoder->sequence_handler(
                    indexer->object,
                    decoder->schema + indexer->schema_index - 1);
                RETURN_IF_NULL(TARA_ERROR_OUT_OF_MEMORY, *out_object);
                return TARA_ERROR_SUCCESS;
            }
            break;

        default:
            return TARA_ERROR_UNEXPECTED;
        }
    }

    return TARA_ERROR_FAILURE;
}

static tara_errors_t postprocess_indexer(
    tara_cbor_decoder_indexer_t * indexer)
{
    switch (indexer->type)
    {
    case TARA_CBOR_DECODER_INDEXER_TYPE_ROOT:
        ++indexer->schema_index;
        break;

    case TARA_CBOR_DECODER_INDEXER_TYPE_ARRAY:
    case TARA_CBOR_DECODER_INDEXER_TYPE_TUPLE:
    case TARA_CBOR_DECODER_INDEXER_TYPE_SEQUENCE:
        ++indexer->element_index;
        break;

    default:
        return TARA_ERROR_UNEXPECTED;
    }

    return TARA_ERROR_SUCCESS;
}

static tara_errors_t find_schema_index(
    tara_cbor_decoder_t * decoder,
    tara_cbor_encoded_key_types_t key_type,
    const tara_cbor_element_data_t * element_data,
    tara_cbor_decoder_indexer_t * indexer,
    size_t * out_schema_index)
{
    if (!(TARA_CBOR_DECODER_INDEXER_TYPE_ROOT == indexer->type ||
          TARA_CBOR_DECODER_INDEXER_TYPE_TUPLE == indexer->type))
    {
        return TARA_ERROR_SUCCESS;
    }

    RETURN_IF(TARA_ERROR_UNEXPECTED, !element_data->has_key);

    size_t scm_index = 0;
    size_t scm_size = decoder->schema_size;
    if (TARA_CBOR_DECODER_INDEXER_TYPE_TUPLE == indexer->type)
    {
        RETURN_IF(TARA_ERROR_UNEXPECTED, 1 == tara_array_size(decoder->indexers));
        scm_index = indexer->schema_index;
        scm_size = scm_index + indexer->count;
    }

    const tara_cbor_schema_t * scm = decoder->schema;
    for (; scm_index < scm_size; ++scm_index)
    {
        switch (key_type)
        {
        case TARA_CBOR_ENCODED_KEY_TYPE_TAG:
            if (element_data->key.tag == scm[scm_index].key.tag)
            {
                *out_schema_index = scm_index;
                return TARA_ERROR_SUCCESS;
            }
            break;

        case TARA_CBOR_ENCODED_KEY_TYPE_NAME:
            if (strlen(scm[scm_index].key.name) == element_data->key.name.size &&
                0 == strncmp(element_data->key.name.ptr,
                             scm[scm_index].key.name,
                             element_data->key.name.size))
            {
                *out_schema_index = scm_index;
                return TARA_ERROR_SUCCESS;
            }
            break;

        default:
            return TARA_ERROR_INVALID_PARAMETER;
        }

        if (TARA_CBOR_SCHEMA_TYPE_SEQUENCE == scm[scm_index].type)
        {
            ++scm_index;
            RETURN_IF(TARA_ERROR_UNEXPECTED, scm_index >= decoder->schema_size);
        }

        if (TARA_CBOR_SCHEMA_TYPE_TUPLE == scm[scm_index].type)
        {
            scm_index += scm[scm_index].count;
            RETURN_IF(TARA_ERROR_UNEXPECTED, scm_index >= decoder->schema_size);
        }
    }

    return TARA_ERROR_NOT_FOUND;
}

static tara_errors_t no_key_parse_handler(
    tara_cbor_decoder_t * decoder,
    const tara_cbor_element_data_t * element_data)
{
    tara_cbor_decoder_indexer_t * indexer;
    if (tara_array_empty(decoder->indexers))
    {
        indexer = tara_array_append(decoder->indexers);
        indexer->type = TARA_CBOR_DECODER_INDEXER_TYPE_ROOT;
        indexer->count = 0;
        indexer->schema_index = 0;
        indexer->element_index = 0;
        indexer->object = decoder->object;
    }

    size_t schema_index = 0;
    size_t element_index = 0;
    void * object = NULL;
    RETURN_IF_FAILED(preprocess_indexer(
        decoder,
        element_data,
        &indexer,
        &schema_index,
        &element_index,
        &object));
    RETURN_IF(TARA_ERROR_SUCCESS, TARA_CBOR_ELEMENT_TYPE_VARBREAK == element_data->type);

    RETURN_IF_FAILED(decode_element(
        decoder,
        TARA_CBOR_ENCODED_KEY_TYPE_NONE,
        element_data,
        schema_index,
        element_index,
        object));

    RETURN_IF_FAILED(postprocess_indexer(indexer));

    return TARA_ERROR_SUCCESS;
}

static tara_errors_t key_parse_handler(
    tara_cbor_decoder_t * decoder,
    tara_cbor_encoded_key_types_t encoding_type,
    const tara_cbor_element_data_t * element_data)
{
    tara_cbor_decoder_indexer_t * indexer;
    if (tara_array_empty(decoder->indexers))
    {
        RETURN_IF(TARA_ERROR_UNEXPECTED, element_data->has_key);
        RETURN_IF(TARA_ERROR_UNEXPECTED, TARA_CBOR_ELEMENT_TYPE_VARMAP != element_data->type);

        indexer = tara_array_append(decoder->indexers);
        RETURN_IF_NULL(TARA_ERROR_OUT_OF_MEMORY, indexer);

        indexer->type = TARA_CBOR_DECODER_INDEXER_TYPE_ROOT;
        indexer->schema_index = 0;
        indexer->element_index = 0;
        indexer->object = decoder->object;

        return TARA_ERROR_SUCCESS;
    }

    size_t schema_index = 0;
    size_t element_index = 0;
    void * object = NULL;
    RETURN_IF_FAILED(preprocess_indexer(
        decoder,
        element_data,
        &indexer,
        &schema_index,
        &element_index,
        &object));
    RETURN_IF(TARA_ERROR_SUCCESS, TARA_CBOR_ELEMENT_TYPE_VARBREAK == element_data->type);

    tara_errors_t error = find_schema_index(
        decoder,
        encoding_type,
        element_data,
        indexer,
        &schema_index);
    RETURN_IF(TARA_ERROR_SUCCESS, TARA_ERROR_NOT_FOUND == error);
    RETURN_IF_FAILED(error);

    RETURN_IF_FAILED(decode_element(
        decoder,
        encoding_type,
        element_data,
        schema_index,
        element_index,
        object));

    RETURN_IF_FAILED(postprocess_indexer(indexer));

    return TARA_ERROR_SUCCESS;
}

static tara_errors_t parse_handler(
    tara_cbor_encoded_key_types_t encoding_type,
    const tara_cbor_element_data_t * element_data,
    void * private_data)
{
    tara_cbor_decoder_t * decoder = (tara_cbor_decoder_t *)private_data;

    switch (encoding_type)
    {
    case TARA_CBOR_ENCODED_KEY_TYPE_NONE:
        return no_key_parse_handler(decoder, element_data);

    case TARA_CBOR_ENCODED_KEY_TYPE_TAG:
    case TARA_CBOR_ENCODED_KEY_TYPE_NAME:
        return key_parse_handler(decoder, encoding_type, element_data);

    default:
        return TARA_ERROR_UNEXPECTED;
    }
}


// External Functions ------------------------------------------------------------------------------

int tara_cbor_decoder_init(
    tara_cbor_decoder_t * decoder,
    const void * data,
    size_t data_size,
    fn_tara_cbor_decode_sequence_handler_t sequence_handler)
{
    RETURN_IF(TARA_ERROR_INVALID_PARAMETER, NULL == data || 0 == data_size);
    tara_zero_struct(*decoder);
    decoder->ptr = (uint8_t *)data;
    decoder->remaining_size = data_size;
    decoder->sequence_handler = sequence_handler;
    return TARA_ERROR_SUCCESS;
}

int tara_cbor_decode_object(
    tara_cbor_decoder_t * decoder,
    void * object,
    const tara_cbor_schema_t schema[],
    size_t schema_size)
{
    decoder->object = object;
    decoder->schema = schema;
    decoder->schema_size = schema_size;

    return tara_cbor_parse(
        decoder->ptr,
        decoder->remaining_size,
        decoder,
        parse_handler);
}
