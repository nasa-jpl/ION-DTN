/**
 * @file secrypt.h
 * @brief Cryptographic utility functions for encryption, decryption, and hashing.
 *
 * This header file defines cryptographic utility functions leveraging the
 * capabilities of the MBEDTLS 2.28.x library for encryption, decryption, and hashing.
 * Functions are organized for easier navigation and integration into cryptographic
 * workflows, covering utility operations, cryptographic processes, and comprehensive
 * encryption/decryption with hashing capabilities.
 *
 * @details
 * Utility and Debugging Functions:
 * - print_hex: Print binary data as hexadecimal for debugging purposes.
 * - print_encrypted_data: Print encrypted data as hexadecimal to verify encryption.
 * 
 * Cryptographic Operations:
 * - entropy_gen: Generate cryptographically secure random entropy.
 * - entropy_init: Initialize the entropy context for secure random number generation.
 * 
 * Encryption/Decryption and Hashing:
 * - crypt_and_hash_buffer: Perform encryption/decryption and hashing on a buffer.
 * 
 * Features:
 * - Utilizes strong entropy sources and cryptographic algorithms from MBEDTLS.
 * - Supports encryption and decryption of data buffers with simultaneous hashing for
 *   integrity verification.
 * - Enables file-based encryption and decryption with hashing, using MBEDTLS's
 *   file I/O capabilities.
 * - Includes utility functions for debugging and data inspection.
 * 
 * Built upon the MBEDTLS 2.28.x security library, this header ensures adherence to
 * contemporary cryptographic standards and practices. MBEDTLS, maintained by ARM,
 * provides a reliable foundation for secure communication and data protection.
 * 
 * Detailed function descriptions, parameters, and return values are provided to
 * ensure ease of use for developers.
 * 
 * @note Proper use of cryptographic functions is crucial for maintaining data security.
 *       Familiarize with cryptographic principles and MBEDTLS specifics to ensure security.
 * 
 *       Developed and tested with MBEDTLS v2.28.5 and v2.28.7.
 * 
 * @warning Regular updates to MBEDTLS and adherence to cryptographic standards are
 *          essential for maintaining security.
 *
 * @author Sky DeBaun, Jet Propulsion Laboratory
 * @date March 2024
 * @copyright Copyright (c) 2024, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 */

#ifndef CRYPT_AND_HASH_H
#define CRYPT_AND_HASH_H

// MBEDTLS configuration and platform setup
#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#include "mbedtls/platform.h"

// Include cryptographic primitives if available
#if defined(MBEDTLS_CTR_DRBG_C) && defined(MBEDTLS_ENTROPY_C) && \
    defined(MBEDTLS_FS_IO)
    // Entropy and DRBG for random number generation
    #include "mbedtls/entropy.h"
    #include "mbedtls/ctr_drbg.h"
#endif

#if defined(MBEDTLS_CIPHER_C) && defined(MBEDTLS_MD_C) && \
    defined(MBEDTLS_FS_IO)
    // Cipher and message digest for encryption and hashing
    #include "mbedtls/cipher.h"
    #include "mbedtls/md.h"
    #include "mbedtls/platform_util.h"

    // Standard libraries for basic operations
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <ctype.h>  // For isprint
#endif

// Platform-specific includes and adjustments
#if defined(_WIN32)
    // Windows-specific headers
    #include <windows.h>
    #if !defined(_WIN32_WCE)
        #include <io.h>
    #endif
#else
    // POSIX-specific headers for Unix, Linux, and others
    #include <sys/types.h>
    #include <unistd.h>
#endif

#include <fcntl.h>

// Default cipher and message digest
#define CIPHER "AES-256-GCM"
#define MD "SHA256"
#define MAXKEYSIZE 512
#define BUFFSIZE 1024

// Macro definitions for operation modes
#define MODE_ENCRYPT    0
#define MODE_DECRYPT    1

// Predefined hash count for operation security levels
#define HASHCOUNT (8192) // See the following list for details
/* 
    CONSIDER THE FOLLOWING HASHCOUNT VALUES------------------
    8192 (2^13)     --> Basic Security
    16384 (2^14)    --> Good Security (Good balance between security and performance)
    32768 (2^15)    --> Good Security (Good balance between security and performance)
    65536 (2^16)    --> Good Security (Good balance between security and performance)
    131072 (2^17)   --> Enhanced Security (Favors security over performance)
    262144 (2^18)   --> Enhanced Security (Favors security over performance)
    524288 (2^19)   --> Enhanced Security (Favors security over performance)
    1048576 (2^20)  --> High Security (Greater performance impact) 
    2097152 (2^21)  --> High Security (Greater performance impact) 
    4194304 (2^22)  --> High Security (Greater performance impact) 
 */

#define USAGE   \
    "\n  crypt_and_hash <mode> <input filename> <output filename> <cipher> <mbedtls_md> <key>\n" \
    "\n   <mode>: 0 = encrypt, 1 = decrypt\n" \
    "\n  example: crypt_and_hash 0 'personalization_string' file file.aes AES-128-GCM SHA1 hex:E76B2413958B00E193\n" \
    "\n  example: crypt_and_hash 0 'xVc538Fa1773L5' file file.aes AES-256-GCM SHA256 ../my_key.hmk\n" \
    "\n"

//utility/debugging functions--------------------------------


/******************************************************************************/
/*                    UTILITY AND DEBUGGING FUNCTIONS                         */
/******************************************************************************/

/******************************************************************************/
/** print_hex */
/******************************************************************************/
/**
 * @brief Print a hex representation of binary data to standard output.
 *
 * This utility function is designed for debugging purposes, allowing
 * for the visual inspection of binary data in a hexadecimal format.
 * Each byte of input data is converted to its hexadecimal equivalent
 * and printed to standard output, facilitating easy debugging and
 * verification of data contents.
 *
 * @param data Pointer to the binary data to be printed.
 * @param length The length of the data in bytes.
 */
void print_hex(const unsigned char *data, size_t length);


/******************************************************************************/
/** print_encrypted_data */
/******************************************************************************/
/**
 * @brief Print encrypted data in a readable hexadecimal format.
 *
 * Similar to print_hex, this function is aimed at debugging and allows
 * for the inspection of encrypted data. It converts the binary encrypted
 * data into a hexadecimal string representation and prints it to the
 * standard output. This is particularly useful for verifying the output
 * of cryptographic operations, ensuring that encryption processes are
 * functioning as expected.
 *
 * @param data Pointer to the encrypted binary data to be printed.
 * @param length The length of the encrypted data in bytes.
 */

void print_encrypted_data(const unsigned char *data, size_t length);



/******************************************************************************/
/*         CRYPTOGRAPHIC OPERATIONS        CRYPTOGRAPHIC OPERATIONS           */
/******************************************************************************/

/******************************************************************************/
/** entropy_gen */
/******************************************************************************/
/**
 * @brief Generate random entropy.
 *
 * This function generates cryptographically secure random entropy data,
 * which can be used in various cryptographic operations such as key
 * generation, seeding random number generators, etc. The function fills
 * the provided output buffer with random data of the specified length.
 *
 * @param data Context data for the entropy generation, can be NULL if not
 *             used.
 * @param output Buffer where the generated entropy will be stored.
 * @param len The number of bytes of entropy to generate.
 * @param olen Pointer to a size_t variable where the actual number of
 *             generated bytes will be stored. This can be less than or
 *             equal to `len`.
 *
 * @return 0 on success, or a non-zero error code on failure.
 */
int entropy_gen(void *data, unsigned char *output, size_t len, size_t *olen);


/******************************************************************************/
/** entropy_init */
/******************************************************************************/
/**
 * @brief Initialize the entropy context.
 *
 * This function initializes the entropy context, using strong entropy source, 
 * in preparation for generating cryptographically secure random numbers. 
 * It initializes the context structure used by subsequent entropy generation 
 * functions. This is a crucial step in ensuring that the entropy source is ready 
 * and capable of providing high-quality random data for cryptographic operations.
 *
 * @param entropy Pointer to the mbedtls_entropy_context structure to be
 *                initialized. This structure is used to maintain the state
 *                of the entropy source.
 *
 * @return 0 on success, or a non-zero (-1) error code on failure.
 */

int entropy_init(mbedtls_entropy_context *entropy);




/******************************************************************************/
/*                   ENCRYPTION/DECRYPTION AND HASHING                        */
/******************************************************************************/

/******************************************************************************/
/** crypt_and_hash_buffer */
/******************************************************************************/
/**
 * @brief Perform encryption/decryption and hashing on a buffer.
 *
 * This function combines cryptographic operations, specifically encryption
 * or decryption, along with hashing of the input buffer. It allows for
 * seamless processing of data with both encryption (for confidentiality)
 * and hashing (for integrity verification). The mode parameter determines
 * whether to encrypt or decrypt the input data. The function also applies
 * a specified hash algorithm to the input data, producing a digest for
 * integrity checks.
 *
 * @param mode Operation mode: MODE_ENCRYPT (0) for encryption or MODE_DECRYPT (1)
 *             for decryption.
 * @param personalization_string A personalization string used to augment the
 *                               cryptographic operation, enhancing security by 
 *                               introducing additional variation. This provides
 *                               an additional layer of randomization on top of the
 *                               strong entropy source used.
 * @param input_buffer Pointer to the input data buffer.
 * @param input_length Pointer to the size of the input data buffer. On
 *                     return, this may be updated to reflect the size of
 *                     the processed data.
 * @param my_output_buffer Pointer to a pointer that will be allocated and
 *                         filled with the output data (encrypted/decrypted
 *                         and hashed). The caller is responsible for freeing
 *                         this memory.
 * @param my_output_length Pointer to a variable that will store the length
 *                         of the output data.
 * @param cipher String identifying the encryption algorithm to be used.
 * @param md String identifying the hash algorithm to be used.
 * @param my_key The encryption key for the cryptographic operation. This may 
 *               be the path to a symetric HMAC key, or a literal value (i.e. string).
 *
 * @return 0 on success, or a non-zero error code on failure.
 */
int crypt_and_hash_buffer(
    int mode, 
    unsigned char *personalization_string,
    unsigned char *input_buffer, 
    size_t *input_length, 
    unsigned char **my_output_buffer, 
    size_t *my_output_length, 
    char *cipher, 
    char *md, 
    char *my_key
);

#endif // CRYPT_AND_HASH_H