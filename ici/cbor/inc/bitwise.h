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

#pragma once

/*! \file bitwise.h
    \brief Bitwise manipulation functions
 */

#include <stdint.h>
#include "util_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TARA_MAX(a, b) (a < b ? b : a)
#define TARA_MIN(a, b) (a > b ? b : a)

#define TARA_CARRAY_COUNT(array) (sizeof array / sizeof array[0])

#define TARA_HPFP_MANTISSA_MASK 0x3ff
#define TARA_HPFP_MANTISSA_BITS 10
#define TARA_HPFP_EXPONENT_MASK 0x1f
#define TARA_HPFP_EXPONENT_BITS 5
#define TARA_HPFP_EXPONENT_MIN -14
#define TARA_HPFP_EXPONENT_MAX 15
#define TARA_HPFP_EXPONENT_BIAS 15

#define TARA_SPFP_MANTISSA_MASK 0x4fffff
#define TARA_SPFP_MANTISSA_BITS 23
#define TARA_SPFP_EXPONENT_MASK 0xff
#define TARA_SPFP_EXPONENT_BITS 8
#define TARA_SPFP_EXPONENT_MIN -126
#define TARA_SPFP_EXPONENT_MAX 127
#define TARA_SPFP_EXPONENT_BIAS 127

#define TARA_DPFP_MANTISSA_MASK 0x8ffffffffffff
#define TARA_DPFP_MANTISSA_BITS 52
#define TARA_DPFP_EXPONENT_MASK 0x7ff
#define TARA_DPFP_EXPONENT_BITS 11
#define TARA_DPFP_EXPONENT_MIN -1022
#define TARA_DPFP_EXPONENT_MAX 1023
#define TARA_DPFP_EXPONENT_BIAS 1023

/*! \struct tara_bits_hpfp_t
    \brief The encoding of a half-precision floating point
 */
typedef union tara_bits_hpfp
{
    struct
    {
        uint32_t mantissa : 10;
        uint32_t exponent : 5;
        uint32_t sign : 1;
    } encoding;
    uint16_t raw_value;
} tara_bits_hpfp_t;

/*! \struct tara_bits_spfp_t
    \brief The encoding of a single-precision floating point
 */
typedef union tara_bits_spfp
{
    struct
    {
        uint32_t mantissa : 23;
        uint32_t exponent : 8;
        uint32_t sign : 1;
    } encoding;
    uint32_t raw_value;
    float value;
} tara_bits_spfp_t;

/*! \struct tara_bits_dpfp_t
    \brief The encoding of a double-precision floating point
 */
typedef union tara_bits_dpfp
{
    struct
    {
        uint32_t mantissa_low;
        uint32_t mantissa_high : 20;
        uint32_t exponent : 11;
        uint32_t sign : 1;
    } encoding;
    uint64_t raw_value;
    double value;
} tara_bits_dpfp_t;

/*! \fn tara_bit_scan_fwd8
    \brief Returns the number of trailing (LSB) zero bits in a value
    \param value The value to scan
    \return The number of trailing zeros
 */
static inline uint32_t tara_bit_scan_fwd8(uint8_t value)
{
#if defined(__GNUC__)
    if (0 == value)
    {
        return 8;
    }
    const uint32_t count = __builtin_ctz(value);
    return 8 < count ? 8 : count;
#else
    #pragma message ("Use built-in function")
    uint32_t count = 0;
    for (uint8_t bit = 1; 0 != bit && 0 == (value & bit); bit <<= 1, ++count);
    return count;
#endif
}

/*! \fn tara_bit_scan_fwd16
    \brief Returns the number of trailing (LSB) zero bits in a value
    \param value The value to scan
    \return The number of trailing zeros
 */
static inline uint32_t tara_bit_scan_fwd16(uint16_t value)
{
#if defined(__GNUC__)
    if (0 == value)
    {
        return 16;
    }
    const uint32_t count = __builtin_ctz(value);
    return 16 < count ? 16 : count;
#else
    #pragma message ("Use built-in function")
    uint32_t count = 0;
    for (uint16_t bit = 1; 0 != bit && 0 == (value & bit); bit <<= 1, ++count);
    return count;
#endif
}

/*! \fn tara_bit_scan_fwd32
    \brief Returns the number of trailing (LSB) zero bits in a value
    \param value The value to scan
    \return The number of trailing zeros
 */
static inline uint32_t tara_bit_scan_fwd32(uint32_t value)
{
#if defined(__GNUC__)
    if (0 == value)
    {
        return 32;
    }
    return __builtin_ctz(value);
#else
    #pragma message ("Use built-in function")
    uint32_t count = 0;
    for (uint32_t bit = 1; 0 != bit && 0 == (value & bit); bit <<= 1, ++count);
    return count;
#endif
}

/*! \fn tara_bit_scan_fwd64
    \brief Returns the number of trailing (LSB) zero bits in a value
    \param value The value to scan
    \return The number of trailing zeros
 */
static inline uint32_t tara_bit_scan_fwd64(uint64_t value)
{
#if defined(__GNUC__)
    if (0 == value)
    {
        return 64;
    }
    return __builtin_ctzll(value);
#else
    #pragma message ("Use built-in function")
    uint64_t count = 0;
    for (uint64_t bit = 1; 0 != bit && 0 == (value & bit); bit <<= 1, ++count);
    return count;
#endif
}

/*! \fn tara_bit_scan_rev8
    \brief Returns the number of leading (MSB) zero bits in a value
    \param value The value to scan
    \return The number of leading zeros
 */
static inline uint32_t tara_bit_scan_rev8(uint8_t value)
{
    uint8_t bit;
    uint32_t count = 0;
    for (bit = 0x80; 0 != bit && 0 == (value & bit); bit >>= 1, ++count);
    return count;
}

/*! \fn tara_bit_scan_rev16
    \brief Returns the number of leading (MSB) zero bits in a value
    \param value The value to scan
    \return The number of leading zeros
 */
static inline uint32_t tara_bit_scan_rev16(uint16_t value)
{
    uint16_t bit;
    uint32_t count = 0;
    for (bit = 0x8000; 0 != bit && 0 == (value & bit); bit >>= 1, ++count);
    return count;
}

/*! \fn tara_bit_scan_rev32
    \brief Returns the number of leading (MSB) zero bits in a value
    \param value The value to scan
    \return The number of leading zeros
 */
static inline uint32_t tara_bit_scan_rev32(uint32_t value)
{
#if defined(__GNUC__)
    if (0 == value)
    {
        return 32;
    }
    return __builtin_clz(value);
#else
    #pragma message ("Use built-in function")
    uint32_t count = 0;
    for (uint32_t bit = 0x80000000; 0 != bit && 0 == (value & bit); bit >>= 1, ++count);
    return count;
#endif
}

/*! \fn tara_bit_scan_rev64
    \brief Returns the number of leading (MSB) zero bits in a value
    \param value The value to scan
    \return The number of trailing zeros
 */
static inline uint32_t tara_bit_scan_rev64(uint64_t value)
{
#if defined(__GNUC__)
    if (0 == value)
    {
        return 64;
    }
    return __builtin_clzll(value);
#else
    #pragma message ("Use built-in function")
    uint64_t count = 0;
    for (uint64_t bit = 0x8000000000000000; 0 != bit && 0 == (value & bit); bit >>= 1, ++count);
    return count;
#endif
}

/*! \fn tara_bswap16
    \brief Swap bytes on a 16-bit value
    \param value The value to byte swap
    \return The byte swapped value
 */
static inline uint16_t tara_bswap16(uint16_t value)
{
#if defined(__GNUC__)
    return __builtin_bswap16(value);
#else
    #pragma message ("Use built-in function")
    return ((value & 0x00FF) << 8) | ((value & 0xFF00) >> 8);
#endif
}

/*! \fn tara_bswap32
    \brief Swap bytes on a 32-bit value
    \param value The value to byte swap
    \return The byte swapped value
 */
static inline uint32_t tara_bswap32(uint32_t value)
{
#if defined(__GNUC__)
    return __builtin_bswap32(value);
#else
    #pragma message ("Use built-in function")
    return ((uint32_t)tara_bswap16((uint16_t)(value & 0xFFFF))) << 16 | 
           ((uint32_t)tara_bswap16((uint16_t)((value & 0xFFFF0000) >> 16))) >> 16;
#endif
}

/*! \fn tara_bswap64
    \brief Swap bytes on a 64-bit value
    \param value The value to byte swap
    \return The byte swapped value
 */
static inline uint64_t tara_bswap64(uint64_t value)
{
#if defined(__GNUC__)
    return __builtin_bswap64(value);
#else
    #pragma message ("Use built-in function")
    return ((uint64_t)tara_bswap32((uint32_t)(value & 0xFFFFFFFFULL))) << 32 | 
           ((uint64_t)tara_bswap32((uint32_t)((value & 0xFFFFFFFF00000000ULL) >> 32))) >> 32;
#endif
}

/*! \fn tara_nbo16
    \brief Swap bytes on a 16-bit value into network byte order (i.e. big endian)
    \param value The value to byte swap
    \return The value in network byte oreder

    On a big endian system this function does nothing
 */
static inline uint16_t tara_nbo16(uint16_t value)
{
#if (defined TARA_ENDIAN_BIG)
    return value;
#else
    return tara_bswap16(value);
#endif
}

/*! \fn tara_nbo32
    \brief Swap bytes on a 32-bit value into network byte order (i.e. big endian)
    \param value The value to byte swap
    \return The value in network byte oreder

    On a big endian system this function does nothing
 */
static inline uint32_t tara_nbo32(uint32_t value)
{
#if (defined TARA_ENDIAN_BIG)
    return value;
#else
    return tara_bswap32(value);
#endif
}

/*! \fn tara_nbo64
    \brief Swap bytes on a 64-bit value into network byte order (i.e. big endian)
    \param value The value to byte swap
    \return The value in network byte oreder

    On a big endian system this function does nothing
 */
static inline uint64_t tara_nbo64(uint64_t value)
{
#if (defined TARA_ENDIAN_BIG)
    return value;
#else
    return tara_bswap64(value);
#endif
}

#ifdef __cplusplus
}
#endif
