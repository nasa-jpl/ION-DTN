/**
 * @file entropy_src.h
 * @brief Cross-platform, high-reliability entropy source polling.
 *
 * @mainpage ION Entropy Source Subsystem
 *
 * @section intro_sec Introduction
 *
 * This file declares a robust, portable, and secure interface for
 * generating high-quality random bytes for cryptographic operations.
 *
 * It is designed to be self-contained and easily integrated into any
 * subsystem requiring a reliable source of entropy.
 *
 * <hr>
 *
 * @section tiers_sec Implementation Strategy
 *
 * The implementation uses a tiered strategy to ensure the best, most secure
 * entropy source is always used on a given platform. The tiers are prioritized
 * from most to least preferred:
 *
 * <b>Tier 1: Primary Platform APIs</b>
 * - The modern, recommended, high-level interfaces for acquiring entropy on
 * a given operating system.
 * - **Linux (Kernel 3.17+):** `getrandom() glibc wrapper`
 * - **macOS (10.12+), FreeBSD, OpenBSD:** `arc4random_buf()`
 * - **Windows:** `BCryptGenRandom`
 *
 * <b>Tier 2: Fallback & Legacy APIs</b>
 * - Used when the primary Tier 1 API is unavailable.
 * - **macOS (Fallback):** `SecRandomCopyBytes`
 *
 * <b>Tier 3: Universal Fallback (Device Files)</b>
 * - This is the final fallback for ensuring broad compatibility with older or
 * generic Unix-like systems where modern APIs are not available.
 * - **Primary Fallback:** Reads from `/dev/urandom`.
 * - **Safety Precaution (Linux):** On older Linux kernels that lack the
 * `getrandom()` syscall, the code first performs a **non-blocking read**
 * from `/dev/random`. This acts as an explicit poll to verify the
 * kernel's entropy pool is safely initialized before any data is taken
 * from `/dev/urandom`.
 * - **Legacy Fallback:** If `/dev/urandom` is unavailable, or if the
 * non-blocking readiness check on Linux indicates the pool is not yet
 * seeded, the code will fall back to a **blocking read** from
 * `/dev/random`. This serves as the ultimate guarantee of entropy quality.
 *
 * <hr>
 *
 * @section usage_sec Usage Example
 *
 * @code
 * #include "entropy_src.h"
 * #include <platform.h>
 * #include <stdio.h>
 *
 * int main(void) {
 * unsigned char buffer[32];
 * size_t bytes_read = 0;
 *
 * int result = poll_entropy_src(NULL, buffer, sizeof(buffer), &bytes_read);
 *
 * if (result == 0 && bytes_read == sizeof(buffer))
 *     {
 *         printf("Successfully read %zu random bytes.\n", bytes_read);
 *     } else
 *     {
 *         fprintf(stderr, "Error: %s\n", get_error_message(result));
 *         return 1;
 *     }
 *
 *     return 0;
 * }
 * @endcode
 *
 * @see poll_entropy_src()
 * @see get_error_message()
 * @see ErrorCode
 *
 * @author Sky DeBaun - Jet Propulsion Laboratory
 * @copyright Copyright (c) 2024-2025, California Institute of Technology.
 * ALL RIGHTS RESERVED. U.S. Government Sponsorship acknowledged.
 */

#ifndef ENTROPY_SRC_H
#define ENTROPY_SRC_H

#include <ion.h>
#include <platform.h>

/**
 * @enum ErrorCode
 * @brief Error codes for entropy source operations.
 *
 * This enumeration defines error codes that are returned by the entropy
 * source operations to indicate various types of failures.
 */
typedef enum
{
	INVALID_ARGUMENTS = -1,                 /**< Invalid arguments passed to a function. */
	ERROR_GETRANDOM_FAILED = -2,            /**< The getrandom() glibc wrapper call failed on Linux. */
	ERROR_BCRYPT_FAILED = -3,               /**< The BCryptGenRandom function failed on Windows. */
	ERROR_SECRANDOM_FAILED = -4,            /**< The SecRandomCopyBytes function failed on macOS. */
	ERROR_OPENING_ENTROPY_SOURCE = -5,      /**< [Fallback] Could not open an entropy device file. */
	ERROR_READING_ENTROPY_SOURCE = -6       /**< [Fallback] Could not read from an entropy device file. */
} ErrorCode;


/**
 * @brief Generates cryptographically secure random bytes.
 *
 * @details This function uses the best available entropy source on the current
 * platform, prioritizing modern syscalls over device files.
 *
 * @param data      A pointer for API compatibility. Unused. Should be NULL.
 * @param output    Pointer to the buffer to store the random bytes.
 * @param ilen      Number of bytes to generate.
 * @param olen      Pointer to a size_t where the actual number of generated
 * bytes will be stored.
 * @return          0 on success, or a negative ErrorCode on failure.
 */
int poll_entropy_src(void *data, unsigned char *output, size_t ilen, size_t *olen);


/**
 * @brief Retrieves a human-readable error message for a given ErrorCode.
 *
 * @param  code     The ErrorCode to translate.
 * @return          A constant string containing the description of the error.
 */
const char* get_error_message(ErrorCode code);

#endif /* ENTROPY_SRC_H */
