/********************************************************
 **  Authors: Michele Rodolfi, michele.rodolfi@studio.unibo.it
 **           Anna d'Amico, anna.damico@studio.unibo.it
 **           Carlo Caini (DTNperf_3 project supervisor), carlo.caini@unibo.it
 **
 **
 **  Copyright (c) 2013, Alma Mater Studiorum, University of Bologna
 **  All rights reserved.
 ** In this file are defined general types, such as synonyms of number types. E.g. s8_t is "signed char"
 ********************************************************/

#ifndef TYPES_H_
#define TYPES_H_

#ifndef NULL
#define NULL      ((void*)0)
#endif

#include <stdint.h>

typedef char char_t;
typedef char * string_t;
typedef const char* cstring_t;

#ifndef TRUE
#define TRUE        (1 == 1)
#endif
#ifndef true
#define true        TRUE
#endif
#ifndef FALSE
#define FALSE       (0 == 1)
#endif
#ifndef false
#define false       FALSE
#endif

typedef char boolean_t;

typedef int8_t s8_t;

typedef float float32_t;

typedef int32_t s32_t;

typedef long double float128_t;

typedef uint32_t u32_t;

typedef double float64_t;

typedef uint8_t u8_t;

typedef uint16_t u16_t;

typedef uint64_t u64_t;

typedef int16_t s16_t;

#endif //TYPES_H_
