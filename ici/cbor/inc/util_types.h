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

/*! \file types.h
    \brief tara types
*/

/*! \typedef tara_bool_t
    \brief tara boolean type
*/
typedef unsigned char tara_bool_t;

#ifndef FALSE
#define FALSE 0
#endif 

#ifndef TRUE
#define TRUE !FALSE
#endif

/*! \def TARA_ID_TYPE
    \brief Declares an id type and its functionality
    \param name The name of the id
    \param underlying_type The underlying type for the id

    The underlying type must be an integral value.

    The declared id type will be <name>_id_t

    Defines the following functions:

        tara_bool_t <name>_ids_eq(<name>_id_t, <name>_id_t)
        <name>_id_t <name>_id_incrment(<name>_id_t)
*/
#define TARA_ID_TYPE(name, underlying_type) \
    typedef struct name##_id { underlying_type value; } name##_id_t; \
    static inline tara_bool_t name##_ids_eq(name##_id_t lhs, name##_id_t rhs) \
    { return lhs.value == rhs.value; } \
    static inline name##_id_t name##_id_increment(name##_id_t id) \
    { name##_id_t nid = { (underlying_type)(id.value + 1) }; return nid; }

/*! \def TARA_CONTINUE_IF
    \brief Helper macro for continuing a loop if the condition is met
    \param condition The condition
*/
#define TARA_CONTINUE_IF(condition) if (condition) continue
