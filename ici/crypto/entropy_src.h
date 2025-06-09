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
 * The implementation uses a tiered strategy to ensure the best possible
 * entropy source is always used on a given platform. The tiers are prioritized
 * from most to least preferred:
 *
 * <b>Tier 1: Modern System Calls</b>
 * - The preferred method on modern Unix-like systems. These are direct,
 * efficient, and safe kernel interfaces.
 * - **Linux (Kernel 3.17+):** `getrandom()`
 * - **macOS (10.12+), FreeBSD, OpenBSD:** `getentropy()`
 *
 * <b>Tier 2: High-Level Cryptographic APIs</b>
 * - Used on platforms that provide a dedicated crypto API, or as a fallback
 * on macOS.
 * - **Windows:** `BCryptGenRandom` (primary method)
 * - **macOS (Fallback):** `SecRandomCopyBytes`
 *
 * <b>Tier 3: Legacy Device Files</b>
 * - This is the universal fallback for ensuring broad compatibility with
 * older and generic Unix-like systems.
 * - **Primary Fallback:** Reads from `/dev/urandom`, the standard non-blocking
 * source available on nearly all historical and modern Unix systems.
 * - **Legacy Fallback:** If `/dev/urandom` is not found, it attempts to read
 * from `/dev/random`. This ensures support for very old legacy Unix
 * systems that may predate the introduction of `/dev/urandom`.
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
 * uvast bytes_read = 0;
 *
 * int result = poll_entropy_src(NULL, buffer, sizeof(buffer), &bytes_read);
 *
 * if (result == 0 && bytes_read == sizeof(buffer)) {
 * printf("Successfully read %llu random bytes.\n", (unsigned long long)bytes_read);
 * } else {
 * fprintf(stderr, "Error: %s\n", getErrorMessage(result));
 * return 1;
 * }
 *
 * return 0;
 * }
 * @endcode
 *
 * @see poll_entropy_src()
 * @see getErrorMessage()
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
typedef enum {
    INVALID_ARGUMENTS = -1,                 /**< Invalid arguments passed to a function. */
    ERROR_GETRANDOM_FAILED = -2,            /**< The getrandom() syscall failed on Linux. */
    ERROR_GETENTROPY_FAILED = -3,           /**< The getentropy() function failed on a BSD or macOS. */
    ERROR_BCRYPT_FAILED = -4,               /**< The BCryptGenRandom function failed on Windows. */
    ERROR_SECRANDOM_FAILED = -5,            /**< The SecRandomCopyBytes function failed on macOS. */
    ERROR_OPENING_ENTROPY_SOURCE = -6,      /**< [Fallback] Could not open an entropy device file. */
    ERROR_READING_ENTROPY_SOURCE = -7       /**< [Fallback] Could not read from an entropy device file. */
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
 * @param olen      Pointer to a uvast where the actual number of generated
 * bytes will be stored.
 * @return          0 on success, or a negative ErrorCode on failure.
 */
int poll_entropy_src(void *data, unsigned char *output, uvast ilen, uvast *olen);


/**
 * @brief Retrieves a human-readable error message for a given ErrorCode.
 *
 * @param  code     The ErrorCode to translate.
 * @return          A constant string containing the description of the error.
 */
const char* getErrorMessage(ErrorCode code);

#endif /* ENTROPY_SRC_H */