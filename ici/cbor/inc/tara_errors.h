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

/*! \file errors.h
    \brief Error enumeration and helper macros
 */

/*! \enum tara_errors_t
    \brief Tara API errors returned
 */
typedef enum tara_errors
{
    TARA_ERROR_SUCCESS = 0,
    TARA_ERROR_FAILURE = -1,
    TARA_ERROR_UNEXPECTED = -2,
    TARA_ERROR_ALREADY_CREATED = -3,
    TARA_ERROR_NOT_FOUND = -4,
    TARA_ERROR_INVALID_HANDLE = -5,
    TARA_ERROR_INVALID_MODULE = -6,
    TARA_ERROR_INVALID_PARAMETER = -7,
    TARA_ERROR_INVALID_OPERATION = -8,
    TARA_ERROR_BAD_ALLOC = -9,
    TARA_ERROR_NO_DATA = -10,
    TARA_ERROR_RETRY = -11,
    TARA_ERROR_TIMEOUT = -12,
    TARA_ERROR_NOT_IMPLEMENTED = -13,
    TARA_ERROR_OUT_OF_MEMORY = -14,
} tara_errors_t;

#define SUCCEEDED(x) (TARA_ERROR_SUCCESS == (x))
#define FAILED(x) (TARA_ERROR_SUCCESS != (x))

#define RETURN_IF(e, c) if (c) { return (e); }
#define RETURN_IF_NULL(e, p) if (NULL == (p)) { return (e); }

#define RETURN_VOID_IF_NULL(p) if (NULL == (p)) { return ; }

#define RETURN_IF_FAILED(x) \
    { \
        tara_errors_t error = (x); \
        if (error != TARA_ERROR_SUCCESS) \
        { \
            return error; \
        } \
    }
    
#define RETURN_NULL_IF_FAILED(x) \
    { \
        tara_errors_t error = (x); \
        if (error != TARA_ERROR_SUCCESS) \
        { \
            return NULL; \
        } \
    }
    
#define GOTO_IF(e, c, l) if (c) { error = (e); goto l; }
#define GOTO_IF_NULL(e, p, l) if (NULL == (p)) { error = (e); goto l; }
#define GOTO_IF_FAILED(e, l) if (FAILED(error = (e))) { goto l; }

#define CLEANUP_IF(e, c) GOTO_IF(e, c, cleanup)
#define CLEANUP_IF_NULL(e, p) GOTO_IF_NULL(e, p, cleanup)
#define CLEANUP_IF_FAILED(c) GOTO_IF_FAILED(c, cleanup)
