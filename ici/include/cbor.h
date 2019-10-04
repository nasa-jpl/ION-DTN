/*
 * 	cbor.h:		encapsulation of ion_cbor.h provided
 * 			by Antara Teknik, LLC.
 *
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

#ifndef _CBOR_H_
#define _CBOR_H_

#ifdef __cplusplus
extern "C" {
#endif

#pragma once

/*! \file ion_cbor.h
    \brief Contains functionality for encoding and decoding the CBOR data format
 */

#include <stddef.h>
#include <stdint.h>
#include "array.h"
#include "tara_errors.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TARA_CBOR_MAJOR_TYPE_MASK 0xE0
#define TARA_CBOR_ADDITIONAL_TYPE_MASK 0x1F

#define TARA_CBOR_MAX_INT_DIRECT_ENCODE 23

/*! \fn tara_zero_struct
    \brief Zeros out a struct
    \param struct The struct to fill with zeros
 */
#ifndef tara_zero_struct
#define tara_zero_struct(struct) memset(&(struct), 0, sizeof (struct))
#endif

/*! \enum tara_cbor_major_types_t
    \brief The major types for a CBOR encoding
 */
typedef enum tara_cbor_major_types
{
    TARA_CBOR_MAJOR_TYPE_POSINT = (0 << 5),
    TARA_CBOR_MAJOR_TYPE_NEGINT = (1 << 5),
    TARA_CBOR_MAJOR_TYPE_BYTE = (2 << 5),
    TARA_CBOR_MAJOR_TYPE_TEXT = (3 << 5),
    TARA_CBOR_MAJOR_TYPE_ARRAY = (4 << 5),
    TARA_CBOR_MAJOR_TYPE_MAP = (5 << 5),
    TARA_CBOR_MAJOR_TYPE_SEMTAG = (6 << 5),
    TARA_CBOR_MAJOR_TYPE_PRIMITIVE = (7 << 5),
} tara_cbor_major_types_t;

/*! \enum tara_cbor_size_types_t
    \brief The size types for a CBOR additional type encoding
 */
typedef enum tara_cbor_size_types
{
    TARA_CBOR_SIZE_TYPE_8 = 24,
    TARA_CBOR_SIZE_TYPE_16 = 25,
    TARA_CBOR_SIZE_TYPE_32 = 26,
    TARA_CBOR_SIZE_TYPE_64 = 27,
    TARA_CBOR_SIZE_TYPE_INDEFINITE = 31,
} tara_cbor_size_types_t;

/*! \enum tara_cbor_primitive_types_t
    \brief The primitive types for a CBOR additional type encoding
 */
typedef enum tara_cbor_primitive_types
{
    TARA_CBOR_PRIMITIVE_TYPE_FALSE = 20,
    TARA_CBOR_PRIMITIVE_TYPE_TRUE = 21,
    TARA_CBOR_PRIMITIVE_TYPE_NULL = 22,
    TARA_CBOR_PRIMITIVE_TYPE_UNDEFINED = 23,
    TARA_CBOR_PRIMITIVE_TYPE_UINT8 = 24,
    TARA_CBOR_PRIMITIVE_TYPE_HPFP = 25,
    TARA_CBOR_PRIMITIVE_TYPE_SPFP = 26,
    TARA_CBOR_PRIMITIVE_TYPE_DPFP = 27,
    TARA_CBOR_PRIMITIVE_TYPE_BREAK = 31,
} tara_cbor_primitive_types_t;

/*! \enum tara_cbor_additional_type_t
    \brief A union of the additional types
 */
typedef union tara_cbor_additional_type
{
    tara_cbor_size_types_t size;
    tara_cbor_primitive_types_t primitive;
    int asInt;
} tara_cbor_additional_type_t;

/*! \enum tara_cbor_element_types_t
    \brief The element type in CBOR encoded data
 */
typedef enum tara_cbor_element_types
{
    TARA_CBOR_ELEMENT_TYPE_INVALID   = -1,
    TARA_CBOR_ELEMENT_TYPE_NULL      = 0x0000,
    TARA_CBOR_ELEMENT_TYPE_UNDEFINED = 0x0001,
    TARA_CBOR_ELEMENT_TYPE_BOOL      = 0x0002,
    TARA_CBOR_ELEMENT_TYPE_POS       = 0x0100,
    TARA_CBOR_ELEMENT_TYPE_POSINT8   = TARA_CBOR_ELEMENT_TYPE_POS | 8,
    TARA_CBOR_ELEMENT_TYPE_POSINT16  = TARA_CBOR_ELEMENT_TYPE_POS | 16,
    TARA_CBOR_ELEMENT_TYPE_POSINT32  = TARA_CBOR_ELEMENT_TYPE_POS | 32,
    TARA_CBOR_ELEMENT_TYPE_POSINT64  = TARA_CBOR_ELEMENT_TYPE_POS | 64,
    TARA_CBOR_ELEMENT_TYPE_NEG       = 0x0200,
    TARA_CBOR_ELEMENT_TYPE_NEGINT8   = TARA_CBOR_ELEMENT_TYPE_NEG | 8,
    TARA_CBOR_ELEMENT_TYPE_NEGINT16  = TARA_CBOR_ELEMENT_TYPE_NEG | 16,
    TARA_CBOR_ELEMENT_TYPE_NEGINT32  = TARA_CBOR_ELEMENT_TYPE_NEG | 32,
    TARA_CBOR_ELEMENT_TYPE_NEGINT64  = TARA_CBOR_ELEMENT_TYPE_NEG | 64,
    TARA_CBOR_ELEMENT_TYPE_FLT       = 0x0300,
    TARA_CBOR_ELEMENT_TYPE_FLT16     = TARA_CBOR_ELEMENT_TYPE_FLT | 16,
    TARA_CBOR_ELEMENT_TYPE_FLT32     = TARA_CBOR_ELEMENT_TYPE_FLT | 32,
    TARA_CBOR_ELEMENT_TYPE_FLT64     = TARA_CBOR_ELEMENT_TYPE_FLT | 64,
    TARA_CBOR_ELEMENT_TYPE_VAR       = 0x8000,
    TARA_CBOR_ELEMENT_TYPE_VARBREAK  = TARA_CBOR_ELEMENT_TYPE_VAR | 0x4000,
    TARA_CBOR_ELEMENT_TYPE_BYTES     = 0x0400,
    TARA_CBOR_ELEMENT_TYPE_VARBYTES  = TARA_CBOR_ELEMENT_TYPE_VAR | TARA_CBOR_ELEMENT_TYPE_BYTES,
    TARA_CBOR_ELEMENT_TYPE_TEXT      = 0x0500,
    TARA_CBOR_ELEMENT_TYPE_VARTEXT   = TARA_CBOR_ELEMENT_TYPE_VAR | TARA_CBOR_ELEMENT_TYPE_TEXT,
    TARA_CBOR_ELEMENT_TYPE_ARRAY     = 0x0600,
    TARA_CBOR_ELEMENT_TYPE_VARARRAY  = TARA_CBOR_ELEMENT_TYPE_VAR | TARA_CBOR_ELEMENT_TYPE_ARRAY,
    TARA_CBOR_ELEMENT_TYPE_MAP       = 0x0700,
    TARA_CBOR_ELEMENT_TYPE_VARMAP    = TARA_CBOR_ELEMENT_TYPE_VAR | TARA_CBOR_ELEMENT_TYPE_MAP,
    TARA_CBOR_ELEMENT_TYPE_CATEGORY_MASK = 0x0F00,
} tara_cbor_element_types_t;

/*! \def TARA_CBOR_MEMBER_OFFSET
    \brief Gets the offset of an object's member
    \param object The object (i.e. structure) variable
    \param name The member name
 */
#define TARA_CBOR_MEMBER_OFFSET(object, member) ((size_t)(uint8_t*)&object.member - (size_t)(uint8_t *)&object)

/*! \struct tara_cbor_schema_keys_t
    \brief The type of key used by the schema

    Antara specific CBOR keyed encoding mechanism
 */
typedef enum tara_cbor_encoded_key_types
{
    TARA_CBOR_ENCODED_KEY_TYPE_UNKNOWN = 0,
    TARA_CBOR_ENCODED_KEY_TYPE_NONE = 1,
    TARA_CBOR_ENCODED_KEY_TYPE_TAG = 2,
    TARA_CBOR_ENCODED_KEY_TYPE_NAME = 3,
} tara_cbor_encoded_key_types_t;

/*! \struct tara_cbor_element_key_t
    \brief The Anatara specific key type of the encoded element
 */
typedef union tara_cbor_element_key
{
    uint64_t tag;
    struct
    {
        const char * ptr;
        size_t size;
    } name;
} tara_cbor_element_key_t;

/*! \struct tara_cbor_element_data_t
    \brief The data of a decoded CBOR element

    The key member is for using the Anatara CBOR keyed encoding, whether a tag key or a name key
    The sem_tag member is the CBOR semantic tag
    The type is the paresed data type of the element
    The value is the value of the element, as is dependent on the type
 */
typedef struct tara_cbor_element_data
{
    tara_bool_t has_key;
    tara_cbor_element_key_t key;

    tara_bool_t has_sem_tag;
    uint64_t sem_tag;

    tara_cbor_element_types_t type;
    union
    {
        tara_bool_t boolean;
        int64_t int64;
        uint64_t uint64;
        double flt64;

        struct
        {
            const uint8_t * ptr;
            size_t size;
        } bytes;

        struct
        {
            const char * ptr;
            size_t size;
        } text;

        uint64_t array_size;
        uint64_t map_size;
    } value;
} tara_cbor_element_data_t;

/*! \enum tara_cbor_schema_types_t
    \brief The object member types to be encoded in Tara CBOR schema format
 */
typedef enum tara_cbor_schema_types
{
    TARA_CBOR_SCHEMA_TYPE_BOOL,
    TARA_CBOR_SCHEMA_TYPE_INT8,
    TARA_CBOR_SCHEMA_TYPE_INT16,
    TARA_CBOR_SCHEMA_TYPE_INT32,
    TARA_CBOR_SCHEMA_TYPE_INT64,
    TARA_CBOR_SCHEMA_TYPE_UINT8,
    TARA_CBOR_SCHEMA_TYPE_UINT16,
    TARA_CBOR_SCHEMA_TYPE_UINT32,
    TARA_CBOR_SCHEMA_TYPE_UINT64,
    TARA_CBOR_SCHEMA_TYPE_FLT32,
    TARA_CBOR_SCHEMA_TYPE_FLT64,
    TARA_CBOR_SCHEMA_TYPE_TEXT,
    TARA_CBOR_SCHEMA_TYPE_TUPLE,
    TARA_CBOR_SCHEMA_TYPE_SEQUENCE,
} tara_cbor_schema_types_t;

/*! \struct tara_cbor_schema_t
    \brief Schema element
    Do not directly set this structure. Use the TARA_CBOR_SCHEMA_ELEMENT macro instead.
 */
typedef struct tara_cbor_schema
{
    union
    {
        uintptr_t tag;
        const char * name;
    } key;
    size_t member_offset;
    tara_cbor_schema_types_t type;
    size_t count;
} tara_cbor_schema_t;

/*! \def TARA_CBOR_SCHEMA_ELEMENT
    \brief Defines a schema array element
    \param member_offset The offset of the member
    \param type The tara_cbor_schema_types_t of the member

    The count can be used for encoding a fixed length array or a struct with the same type repeated
    for all members.

    The TARA_CBOR_SCHEMA_TYPE_TEXT entry count is ignored for encode, and specifies the size of the
    string buffer for decode.

    The TARA_CBOR_SCHEMA_TYPE_TUPLE entry is used to define a structure with the next count
    elements following as members of the tuple. It is encoded as a fixed length array without
    key encoding and as a fixed length map for either tag or name key encoding.

    The TARA_CBOR_SCHEMA_TYPE_SEQUENCE entry is used to define a variable length sequence of
    the next schema element. It is encoded as an indefinite length array. It does not make use of
    the count parameter, and can therefore be set to any value. One use can be as an identifier in
    the callback.
    The element that defines the type of a sequence (i.e. the entry immediately after the sequence
    entry) does not use the tag parameter, and the offset parameter must be set to 0.

    Nested complex types and sequence types are not supported. The only nesting allowed is a single
    complex type used as the element of a sequence type.
 */
#define TARA_CBOR_SCHEMA_ELEMENT(member_offset, type, count) \
    { { 0 }, member_offset, type, count }

/*! \def TARA_CBOR_TAGGED_SCHEMA_ELEMENT
    \brief Defines a tagged schema array element
    \param tag The user defined tag of the entry, this gets encoded
    \param member_offset The offset of the member
    \param type The tara_cbor_schema_types_t of the member
    \param count The count of the element to encode

    \see TARA_CBOR_SCHEMA_ELEMENT for usage details.
 */
#define TARA_CBOR_TAGGED_SCHEMA_ELEMENT(tag, member_offset, type, count) \
    { { tag }, member_offset, type, count }

/*! \def TARA_CBOR_NAMED_SCHEMA_ELEMENT
    \brief Defines a named schema array element
    \param tag The user defined tag of the entry, this gets encoded
    \param member_offset The offset of the member
    \param type The tara_cbor_schema_types_t of the member
    \param count The count of the element to encode

    \see TARA_CBOR_SCHEMA_ELEMENT for usage details.
 */
#define TARA_CBOR_NAMED_SCHEMA_ELEMENT(name, member_offset, type, count) \
    { { (uintptr_t)name }, member_offset, type, count }

/*! \fn fn_tara_cbor_parse_handler_t
    \brief Callback for parsing CBOR data
    \param encoding_type The Anatara specific CBOR keyed encoding type
    \param element_data The parse element data
    \param private_data The private data passed to tara_cbor_parse
    \return TARA_ERROR_SUCCESS to continue processing, an error value to stop
 */
typedef int (*fn_tara_cbor_parse_handler_t)(
    tara_cbor_encoded_key_types_t encoding_type,
    const tara_cbor_element_data_t * element_data,
    void * private_data);

/*! \fn fn_tara_cbor_encode_sequence_handler_t
    \brief Callback for getting a pointer a sequence's element
    \param object The object containing the sequence
    \param schema The schema entry for the sequence
    \param index The index of the element to encode
    \return A pointer to the element at the index, or NULL if there is no element at that index

    Indexes will come in sequentially, starting from 0. Once the function returns NULL no more
    calls will be made.
 */
typedef void * (*fn_tara_cbor_encode_sequence_handler_t)(
    void * object,
    const tara_cbor_schema_t * schema,
    size_t index);

/*! \fn fn_tara_cbor_decode_sequence_handler_t
    \brief Callback for setting sequence elements
    \param object The object containing the sequence
    \param schema The schema entry for the sequence
    \return A pointer to the element to decode into
 */
typedef void * (*fn_tara_cbor_decode_sequence_handler_t)(
    void * object,
    const tara_cbor_schema_t * schema);

/*! \struct tara_cbor_encoder_t
    \brief Encoder data
    Do not directly modify this structure
 */
typedef struct tara_cbor_encoder
{
    uint8_t * ptr;
    size_t size;
    size_t remaining_size;
    tara_cbor_encoded_key_types_t encoding_type;
    tara_cbor_major_types_t var;
    fn_tara_cbor_encode_sequence_handler_t sequence_handler;
} tara_cbor_encoder_t;

/*! \struct tara_cbor_decoder_indexer_types_t
    \brief Decoder index types
 */
typedef enum tara_cbor_decoder_indexer_types
{
    TARA_CBOR_DECODER_INDEXER_TYPE_ROOT,
    TARA_CBOR_DECODER_INDEXER_TYPE_ARRAY,
    TARA_CBOR_DECODER_INDEXER_TYPE_TUPLE,
    TARA_CBOR_DECODER_INDEXER_TYPE_SEQUENCE,
} tara_cbor_decoder_indexer_types_t;

/*! \struct tara_cbor_decoder_indexer_t
    \brief Decoder index data
    Do not directly modify this structure
 */
typedef struct tara_cbor_decoder_indexer
{
    tara_cbor_decoder_indexer_types_t type;
    size_t count;
    size_t schema_index;
    size_t element_index;
    void * object;
} tara_cbor_decoder_indexer_t;
TARA_ARRAY_TYPE(tara_cbor_decoder_indexer_t, cbor_decoder_indexer, 4);

/*! \struct tara_cbor_decoder_t
    \brief Decoder data
    Do not directly modify this structure
 */
typedef struct tara_cbor_decoder
{
    uint8_t * ptr;
    size_t remaining_size;
    fn_tara_cbor_decode_sequence_handler_t sequence_handler;
    void * object;
    const tara_cbor_schema_t * schema;
    size_t schema_size;
    tara_cbor_decoder_indexer_array_t indexers;
} tara_cbor_decoder_t;

/*! \fn tara_cbor_parse
    \brief Parses encoded CBOR data
    \param cbor_data The CBOR encoded data to parse
    \param cbor_data_size The size of the data in bytes
    \param private_data A pointer to caller define data
    \param parse_handler A handler to receive each parsed element
    \return Tara error code
 */
extern int tara_cbor_parse(
    const void * cbor_data,
    size_t cbor_data_size,
    void * private_data,
    fn_tara_cbor_parse_handler_t parse_handler);

/*! \fn tara_cbor_encoder_init
    \brief Initializes the encoder data to begin encoding
    \param encoder The encoder
    \param buffer The buffer to store the encoded data
    \param buffer_size The size of the buffer in bytes
    \return Tara error code
 */
extern int tara_cbor_encoder_init(
    tara_cbor_encoder_t * encoder,
    void * buffer,
    size_t buffer_size,
    fn_tara_cbor_encode_sequence_handler_t sequence_handler);

/*! \fn tara_cbor_encoded_size
    \brief Gets the encoded size in bytes
    \param encoder The encoder
    \return The encoded size in bytes
 */
static inline size_t tara_cbor_encoded_size(
    tara_cbor_encoder_t * encoder)
{
    return encoder->size;
}

/*! \fn tara_cbor_encode_bool
    \brief Encodes a boolean value
    \param encoder The encoder
    \param value The value to encode
    \return Tara error code
 */
extern int tara_cbor_encode_bool(
    tara_cbor_encoder_t * encoder,
    tara_bool_t value);

/*! \fn tara_cbor_encode_int
    \brief Encodes an integer value
    \param encoder The encoder
    \param value The value to encode
    \return Tara error code
 */
extern int tara_cbor_encode_int(
    tara_cbor_encoder_t * encoder,
    int64_t value);

/*! \fn tara_cbor_encode_fp
    \brief Encodes a floating-point value
    \param encoder The encoder
    \param value The value to encode
    \return Tara error code
    Subnormal (i.e. denormal) values are flushed to zero.
 */
extern int tara_cbor_encode_fp(
    tara_cbor_encoder_t * encoder,
    double value);

/*! \fn tara_cbor_encode_fp
    \brief Encodes bytes
    \param encoder The encoder
    \param bytes The bytes to encode
    \param size The number of bytes to encode
    \return Tara error code
 */
extern int tara_cbor_encode_bytes(
    tara_cbor_encoder_t * encoder,
    const uint8_t bytes[],
    size_t size);

/*! \fn tara_cbor_encode_varbytes_begin
    \brief Begins encoding for a variable number of bytes
    \param encoder The encoder
    \return Tara error code
 */
extern int tara_cbor_encode_varbytes_begin(
    tara_cbor_encoder_t * encoder);

/*! \fn tara_cbor_encode_varbytes_end
    \brief Ends encoding for a variable number of bytes
    \param encoder The encoder
    \return Tara error code
 */
extern int tara_cbor_encode_varbytes_end(
    tara_cbor_encoder_t * encoder);

/*! \fn tara_cbor_encode_varbytes
    \brief Encodes a fixed set of bytes as part of a variable byte encoding
    \param encoder The encoder
    \param bytes The bytes to encode
    \param size The number of bytes to encode
    \return Tara error code
 */
extern int tara_cbor_encode_varbytes(
    tara_cbor_encoder_t * encoder,
    const uint8_t bytes[],
    size_t size);

/*! \fn tara_cbor_encode_text
    \brief Encodes a a null terminated string
    \param encoder The encoder
    \param text The null terminzated string to encode
    \return Tara error code
 */
extern int tara_cbor_encode_text(
    tara_cbor_encoder_t * encoder,
    const char * text);

/*! \fn tara_cbor_encode_text_len
    \brief Encodes a string
    \param encoder The encoder
    \param text The string to encode
    \param text_size The size of the string to encode in bytes
    \return Tara error code
 */
extern int tara_cbor_encode_text_size(
    tara_cbor_encoder_t * encoder,
    const char * text,
    size_t text_size);

/*! \fn tara_cbor_encode_vartext_begin
    \brief Begins encoding for a variable string length
    \param encoder The encoder
    \return Tara error code
 */
extern int tara_cbor_encode_vartext_begin(
    tara_cbor_encoder_t * encoder);

/*! \fn tara_cbor_encode_vartext_end
    \brief Ends encoding for a variable string length
    \param encoder The encoder
    \return Tara error code
 */
extern int tara_cbor_encode_vartext_end(
    tara_cbor_encoder_t * encoder);

/*! \fn tara_cbor_encode_text
    \brief Encodes a string as part of a variable length string
    \param encoder The encoder
    \param text The null terminzated string to encode
    \return Tara error code
 */
extern int tara_cbor_encode_vartext(
    tara_cbor_encoder_t * encoder,
    const char * text);

/*! \fn tara_cbor_encode_object
    \brief Encodes an object based on a schema
    \param encoder The encoder
    \param object The object to encode
    \param schema The schema to use for encoding
    \param schema_size The number of elements in the schema
    \return Tara error code
 */
extern int tara_cbor_encode_object(
    tara_cbor_encoder_t * encoder,
    void * object,
    const tara_cbor_schema_t schema[],
    size_t schema_size);

/*! \fn tara_cbor_encode_object_tagged
    \brief Encodes an object based on a schema
    \param encoder The encoder
    \param object The object to encode
    \param schema The schema to use for encoding
    \param schema_size The number of elements in the schema
    \return Tara error code

    Elements are tagged within the encoding. Tags are implemented as a CBOR map entry with a key
    having an integer type.
 */
extern int tara_cbor_encode_object_tagged(
    tara_cbor_encoder_t * encoder,
    void * object,
    const tara_cbor_schema_t schema[],
    size_t schema_size);

/*! \fn tara_cbor_encode_object_named
    \brief Encodes an object based on a schema
    \param encoder The encoder
    \param object The object to encode
    \param schema The schema to use for encoding
    \param schema_size The number of elements in the schema
    \return Tara error code

    Elements are named within the encoding. Names are implemented as a CBOR map entry with a key
    having a text type.
 */
extern int tara_cbor_encode_object_named(
    tara_cbor_encoder_t * encoder,
    void * object,
    const tara_cbor_schema_t schema[],
    size_t schema_size);

/*! \fn tara_cbor_decoder_init
    \brief Initializes the decoder data for decoding
    \param decoder The decoder
    \param data The data to decode
    \param data_size The size of the data in bytes
    \return Tara error code
 */
extern int tara_cbor_decoder_init(
    tara_cbor_decoder_t * decoder,
    const void * data,
    size_t data_size,
    fn_tara_cbor_decode_sequence_handler_t sequence_handler);

/*! \fn tara_cbor_decode_object
    \brief Decodes an object based on a schema
    \param decoder The decoder
    \param object The object to decode
    \param schema The schema to use for decoding
    \param schema_size The number of elements in the schema
    \return Tara error code
 */
extern int tara_cbor_decode_object(
    tara_cbor_decoder_t * decoder,
    void * object,
    const tara_cbor_schema_t schema[],
    size_t schema_size);

/*	Macros that slighly abbreviate the Antara CBOR function
 *	names.								*/

#define	cbor_parse(cbor_data, cbor_data_size, private_data, parse_handler) \
tara_cbor_parse(cbor_data, cbor_data_size, private_data, parse_handler)

#define cbor_encoder_init(encoder, buffer, buffer_size, sequence_handler) \
tara_cbor_encoder_init(encoder, buffer, buffer_size, sequence_handler)

#define cbor_encode_bool(encoder, value)				\
tara_cbor_encode_bool(encoder, value)

#define cbor_encode_int(encoder, value)					\
tara_cbor_encode_int(encoder, value)

#define cbor_encode_fp(encoder, value)					\
tara_cbor_encode_fp(encoder, value)

#define cbor_encode_bytes(encoder, bytes, size)				\
tara_cbor_encode_bytes(encoder, bytes, size)

#define cbor_encode_varbytes_begin(encoder)				\
tara_cbor_encode_varbytes_begin(encoder)

#define cbor_encode_varbytes(encoder, bytes, size)			\
tara_cbor_encode_varbytes(encoder, bytes, size)

#define cbor_encode_varbytes_end(encoder)				\
tara_cbor_encode_varbytes_end(encoder)

#define cbor_encode_text(encoder, text)					\
tara_cbor_encode_text(encoder, text)

#define cbor_encode_text_size(encoder, text, text_size)			\
tara_cbor_encode_text_size(encoder, text, text_size)

#define cbor_encode_vartext_begin(encoder)				\
tara_cbor_encode_vartext_begin(encoder)

#define cbor_encode_vartext_end(encoder)				\
tara_cbor_encode_vartext_end(encoder)

#define cbor_encode_vartext(encoder, text)				\
tara_cbor_encode_vartext(encoder, text)

#define cbor_encode_object(encoder, object, schema, schema_size)	\
tara_cbor_encode_object(encoder, object, schema, schema_size)

#define cbor_encode_object_tagged(encoder, object, schema, schema_size)	\
tara_cbor_encode_object_tagged(encoder, object, schema, schema_size)

#define cbor_encode_object_named(encoder, object, schema, schema_size)	\
tara_cbor_encode_object_named(encoder, object, schema, schema_size)

#define cbor_decoder_init(decoder, data, data_size, sequence_handler)	\
tara_cbor_decoder_init(decoder, data, data_size, sequence_handler)

#define cbor_decode_object(decoder, object, schema, schema_size)	\
tara_cbor_decode_object(decoder, object, schema, schema_size)

#ifdef __cplusplus
}
#endif

#endif  /* _CBOR_H_ */
