/**
 * @file secrypt.h
 * @brief Implementation of non-cryptographic function stubs defined in crypto.c.
 *
 * @details
 * Core Functions:
 * - crypt_and_hash_buffer: non-cryptographic function stub.
 * - crypt_and_hash_file: non-cryptographic function stub.
 *
 * @author Sky DeBaun, Jet Propulsion Laboratory
 * @date March 2024
 * @copyright Copyright (c) 2024, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 */

#ifndef CRYPT_AND_HASH_H
#define CRYPT_AND_HASH_H

#define MODE_ENCRYPT    0
#define MODE_DECRYPT    1

#define CIPHER "AES-256-GCM" //default cipher
#define MD "SHA256" //default message digest


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>



/******************************************************************************/
/** crypt_and_hash_buffer */
/******************************************************************************/
/**
 * @brief Non-cryptographic function stub.
 *
 *
 * @param mode Operation mode: MODE_ENCRYPT (0) for encryption or MODE_DECRYPT (1)
 *             for decryption.
 * @param personalization_string A personalization string used to augment
 *                               entropy.
 * 
 * @param input_buffer Pointer to the input data buffer.
 * @param input_length Pointer to the size of the input data buffer. On
 *                     return, this may be updated to reflect the size of
 *                     the processed data.
 * @param my_output_buffer Pointer to a pointer that will be allocated and
 *                         filled with the output data (non-encrypted). The 
 *                         caller is responsible for freeing this memory.
 * @param my_output_length Pointer to a variable that will store the length
 *                         of the output data.
 * @param cipher String identifying the encryption algorithm to be used.
 * @param md String identifying the hash algorithm to be used.
 * @param my_key The encryption key for the cryptographic operation. This may 
 *               be the path to a key, or a literal value (i.e. string).
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


/******************************************************************************/
/** crypt_and_hash_file */
/******************************************************************************/
/**
 * @brief Non-cryptographic function stub.
 *
 *
 * @param mode Operation mode: MODE_ENCRYPT (0) for encryption or MODE_DECRYPT (1)
 *             for decryption.
* @param personalization_string A personalization string used to augment
 *                               entropy.
 * 
 * @param file_in Path to the input file.
 * @param file_out Path to the output file where the processed data will be
 *                 written.
 * @param cipher String identifying the encryption algorithm to be used.
 * @param md String identifying the hash algorithm to be used.
 * @param my_key The encryption key for the cryptographic operation. This may 
 *               be the path to a symetric HMAC key, or a literal value (i.e. string).
 * 
 * @return 0 on success, or a non-zero error code on failure.
 */

int crypt_and_hash_file(
    int mode, 
    unsigned char *personalization_string, 
    char *file_in, 
    char *file_out, 
    char *cipher, 
    char *md, 
    char *key
);

#endif // CRYPT_AND_HASH_H
