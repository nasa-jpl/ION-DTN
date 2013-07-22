/********************************************************
 **  Authors: Michele Rodolfi, michele.rodolfi@studio.unibo.it
 **           Anna d'Amico, anna.damico@studio.unibo.it
 **           Carlo Caini (DTNperf_3 project supervisor), carlo.caini@unibo.it
 **
 **
 **  Copyright (c) 2013, Alma Mater Studiorum, University of Bologna
 **  All rights reserved.
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

typedef signed char s8_t;

typedef float float32_t;

typedef signed long s32_t;

typedef long double float128_t;

typedef unsigned long u32_t;

typedef double float64_t;

typedef unsigned char u8_t;

typedef unsigned short u16_t;

typedef unsigned long long u64_t;

typedef signed short s16_t;

#endif //TYPES_H_
