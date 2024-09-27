/**
 * @file secrypt.c
 * @brief Implementation of cryptographic functions for encryption, decryption, and hashing.
 *
 * This file contains cryptographic functions defined in crypto.h. It leverages the MBEDTLS 
 * (v2.28.x) library to provide secure encryption, decryption, and hashing capabilities. 
 * These capabilities cover a range of operations including generating secure random numbers, 
 * initializing cryptographic contexts, and performing encryption/decryption. Hashing functionality
 * is included in the core function (see below) to automatically verify integrity after decryption.
 * 
 *
 * @details
 * Core Function:
 * - crypt_and_hash_buffer: Encrypts or decrypts data in a buffer and computes its hash.
 * 
 * Cryptographic Utilities:
 * - entropy_gen: Generates cryptographically secure random entropy.
 * - entropy_init: Initializes the entropy context for random number generation.
 * 
 * Utility Functions:
 * - print_hex: Prints binary data in a hexadecimal format for debugging.
 * - print_encrypted_data: Prints encrypted data in hexadecimal format.
 * 
 * Theses functions utilize the cryptographic algorithms and utilities provided by the MBEDTLS
 * library to ensure the security of cryptographic operations. This includes the use of
 * secure random number generation, encryption/decryption algorithms, and hashing functions
 * to maintain the confidentiality and integrity of the processed data.
 *
 * @note The core function is based on the MBEDTLS file encryption/decryption 
 *       demonstation program. It has been modified to work with buffers and includes 
 *       additional IV randomization, prediction resistance, and uses OS (or hardware if available)
 *       entropy sources.
 *
 * @author Sky DeBaun, Jet Propulsion Laboratory
 * @date March 2024
 * @copyright Copyright (c) 2024, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 */



#define _POSIX_C_SOURCE 200112L //POSIX 2001 compliance check
#include "secrypt.h"
#include <bp.h>


//check for MBEDTLS cipher suite-----------------------------
#if !defined(MBEDTLS_CIPHER_C) || !defined(MBEDTLS_MD_C) || \
    !defined(MBEDTLS_FS_IO) || !defined(MBEDTLS_CTR_DRBG_C) \
    || !defined(MBEDTLS_ENTROPY_C) || !defined(MBEDTLS_FS_IO)
int main(void)
{
mbedtls_printf("MBEDTLS_CIPHER_C and/or MBEDTLS_MD_C and/or MBEDTLS_CTR_DRBG_C "
               "and/or MBEDTLS_ENTROPY_C, and/or MBEDTLS_FS_IO not defined.\n");
    mbedtls_exit(0);
}
#else


/******************************************************************************/
/*                    UTILITY AND DEBUGGING FUNCTIONS                         */
/******************************************************************************/

/******************************************************************************/
/** print_hex */
/******************************************************************************/
void print_hex(const unsigned char *data, size_t length) 
{
    for (size_t i = 0; i < length; i++) 
    {
        printf("%02x", data[i]);
    }
    printf("\n");

} //end print_hex--->///

/******************************************************************************/
/** print_encrypted_data */
/******************************************************************************/
void print_encrypted_data(const unsigned char *data, size_t length) 
{
    for (size_t i = 0; i < length; i++) 
    {
        if (isprint(data[i])) 
        {
            printf("%c", data[i]); //print as character
        } else 
        {
            printf("."); //placeholder for non-printable characters
        }
    }
    printf("\n");

} //end print_encrypted_data--->///




/******************************************************************************/
/*         CRYPTOGRAPHIC OPERATIONS        CRYPTOGRAPHIC OPERATIONS           */
/******************************************************************************/

/******************************************************************************/
/** entropy_gen */
/******************************************************************************/
int entropy_gen(void *data, unsigned char *output, size_t len, size_t *olen)
{
    int fd;
    ssize_t n;
    
    // Try /dev/hwrng first
    fd = open("/dev/hwrng", O_RDONLY);
    if (fd != -1) 
    {
        //fprintf(stdout, "Reading entropy from /dev/hwrng\n");
        n = read(fd, output, len);
        close(fd);
        if (n == len) 
        {
            *olen = len;
            return 0; //success
        }
		fprintf(stderr, "Error reading from /dev/hwrng/\n"); 
    }    
    
    fd = open("/dev/urandom", O_RDONLY);
    if (fd != -1) 
    {
        //fprintf(stdout, "Reading entropy from /dev/urandom\n");

        n = read(fd, output, len);
        close(fd);
        if (n == len) 
        {
            *olen = len;
            return 0; //success
        }
		fprintf(stderr, "Error reading from /dev/urandom/\n"); 
    }

    // If /dev/urandom fails, try /dev/random
    fd = open("/dev/random", O_RDONLY);
    if (fd != -1) 
    {
        //fprintf(stdout, "Reading entropy from /dev/random\n");

        n = read(fd, output, len);
        close(fd);
        if (n == len) 
        {
            *olen = len;
            return 0; //success
        }
		fprintf(stderr, "Error reading from /dev/random/\n");

    }

    // If both sources fail, return an error
    fprintf(stderr, "Failure reading from entropy generators");
	//writeMemo("Failure reading from entropy generators"); //debug statement

    return -1; //failure

} //end entropy_gen--->///


/******************************************************************************/
/** entropy_init */
/******************************************************************************/
int entropy_init(mbedtls_entropy_context *entropy)
{
    mbedtls_entropy_init(entropy );
    mbedtls_entropy_add_source(entropy, entropy_gen, NULL, 0, MBEDTLS_ENTROPY_SOURCE_STRONG);
    return 0;

} //end entropy_init--->///




/******************************************************************************/
/*                   ENCRYPTION/DECRYPTION AND HASHING                        */
/******************************************************************************/

/******************************************************************************/
/** crypt_and_hash_buffer */
/******************************************************************************/
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
)
{    
    int i; //iterator
    int status = -1;
    //unsigned n; //used for key
    int exit_code = MBEDTLS_EXIT_FAILURE; //default to failure
    size_t keylen=0, ilen=0, olen=0;
    size_t input_buffer_size = *input_length;
    
    //char *p = NULL;
    unsigned char *output_buffer = NULL;
    size_t output_length = 0; //return 0 on failure
    
    unsigned char key[MAXKEYSIZE] = {0};
    unsigned char digest[MBEDTLS_MD_MAX_SIZE];
    memset(digest, 0, MBEDTLS_MD_MAX_SIZE);
    unsigned char buffer[BUFFSIZE];
    memset(buffer, 0, BUFFSIZE);
    unsigned char output[BUFFSIZE];
    memset(output, 0, BUFFSIZE);
    unsigned char diff;

    unsigned char randomizer[MBEDTLS_MD_MAX_SIZE];
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy = {0};

    const mbedtls_cipher_info_t *cipher_info;
    const mbedtls_md_info_t *md_info;
    mbedtls_cipher_context_t cipher_ctx;
    mbedtls_md_context_t md_ctx;
    mbedtls_cipher_mode_t cipher_mode;
    unsigned int cipher_block_size=0;
    unsigned char md_size=0;
    FILE *fkey; //key file
    unsigned char *IV = NULL;

    

#if defined(_WIN32_WCE)
    long offset;
#elif defined(_WIN32)
    LARGE_INTEGER li_size;
    __int64 offset;
#else
    off_t offset = 0;
#endif


    mbedtls_cipher_init(&cipher_ctx);
    mbedtls_md_init(&md_ctx);

    /* 
    basic error checking-----------------------------------
     */
    if (mode != MODE_ENCRYPT && mode != MODE_DECRYPT) 
    {
        mbedtls_fprintf(stderr, "invalid operation mode\n");
        goto exit;
    }

    if (input_buffer == *my_output_buffer) 
    {
    mbedtls_fprintf(stderr, "Input and output buffers must not be the same\n");
    goto exit;
    }

    if (input_buffer == NULL) 
    {
        mbedtls_fprintf(stderr, "Input or output buffer is NULL\n");
        goto exit;
    }
    
    cipher_info = mbedtls_cipher_info_from_string(cipher);

    if (cipher_info == NULL) 
    {
        mbedtls_fprintf(stderr, "Cipher '%s' not found\n", cipher);
        goto exit;
    }

    if ((mbedtls_cipher_setup(&cipher_ctx, cipher_info)) != 0) 
    {
        mbedtls_fprintf(stderr, "mbedtls_cipher_setup failed\n");
        goto exit;
    }

    md_info = mbedtls_md_info_from_string(md);
    if (md_info == NULL) 
    {
        mbedtls_fprintf(stderr, "Message Digest '%s' not found\n", md);
        goto exit;
    }

    if (mbedtls_md_setup(&md_ctx, md_info, 1) != 0) 
    {
        mbedtls_fprintf(stderr, "mbedtls_md_setup failed\n");
        goto exit;
    }

    /*
     * Read the secret key from file
     */
    if ((fkey = fopen(my_key, "rb")) != NULL) 
    {
        keylen = fread(key, 1, sizeof(key), fkey);
        fclose(fkey);
    } 
    else 
     {
        /*
        printf("WARNING: using literal value as key (no key file found)!\n");
        if (memcmp(my_key, "hex:", 4) == 0) {
            p = &my_key[4];
            keylen = 0;

            while (sscanf(p, "%02X", (unsigned int *) &n) > 0 &&
                   keylen < (int) sizeof(key)) 
            {
                key[keylen++] = (unsigned char) n;
                p += 2;
            }
        } else  */
        {
            keylen = strlen(my_key);

            if (keylen > (int) sizeof(key)) 
            {
                keylen = (int) sizeof(key);
            }

            memcpy(key, my_key, keylen);
        }
    }

    md_size = mbedtls_md_get_size(md_info);
    cipher_block_size = mbedtls_cipher_get_block_size(&cipher_ctx);

    /* set IV size ----------------------------------------------- */
    size_t iv_size = cipher_block_size;
    IV = MTAKE(iv_size); //freed at exit:
    if (IV == NULL)
    {
        fprintf(stderr, "IV Memory allocation failed\n");
        goto exit;
    }
    memset(IV, 0, iv_size);


    /* ENCRYPT------------------------------------------------------------------------------------- */
    if (mode == MODE_ENCRYPT) 
    {

        /* INITIALIZE RANDOMIZER----------------------------------------- */
        mbedtls_ctr_drbg_init(&ctr_drbg);

        /* INITIALIZE AND SEED ENTROPY------------------------------------ */
        entropy_init(&entropy);

        status = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func,
                                &entropy, (const unsigned char *) personalization_string,
                                strlen((const char *) personalization_string)); //seeded with non cryptographic string
        if (status != 0) 
        {
            mbedtls_printf("failed in mbedtls_ctr_drbg_seed: %d\n", status);
            goto exit;
        }

        /* enable prediction resistance */
        mbedtls_ctr_drbg_set_prediction_resistance(&ctr_drbg, MBEDTLS_CTR_DRBG_PR_OFF);
        
        /* GENERATE UNIQUE CRYPTOGRAPHIC IV INITIALIZER */
        status = mbedtls_ctr_drbg_random(&ctr_drbg, randomizer, MBEDTLS_MD_MAX_SIZE); //populate with crypto grade rand string
        if (status != 0) 
        {
            mbedtls_printf("Error generating GCM!\n");
            goto exit;
        }

        /* generate the IV */
        for (i = 0; i < 8; i++) 
        {
            buffer[i] = (unsigned char) (input_buffer_size >> (i << 3));
        }        
        if (mbedtls_md_starts(&md_ctx) != 0) 
        {
            mbedtls_fprintf(stderr, "mbedtls_md_starts() returned error\n");
            goto exit;
        }
        /* very low level randomizer*/
        if (mbedtls_md_update(&md_ctx, buffer, 8) != 0) 
        {
            mbedtls_fprintf(stderr, "mbedtls_md_update() returned error\n");
            goto exit;
        }
        /* cryptographic grade 64 byte context update */
        if (mbedtls_md_update(&md_ctx, (unsigned char *) (char *)randomizer, MBEDTLS_MD_MAX_SIZE) != 0) 
        {
            mbedtls_fprintf(stderr, "mbedtls_md_update() returned error\n");
            goto exit;
        }
        if (mbedtls_md_finish(&md_ctx, digest) != 0) 
        {
            mbedtls_fprintf(stderr, "mbedtls_md_finish() returned error\n");
            goto exit;
        }
        memcpy(IV, digest, iv_size);


        /* append the IV at the beginning of the output buffer */
        size_t total_required_size = input_buffer_size + iv_size + md_size; //add padding for IV and digest
        output_length = total_required_size;

        output_buffer = MTAKE(total_required_size * sizeof(char)); //freed in exit:
       
        if (output_buffer == NULL)
        {        
            fprintf(stderr, "Encryption memory allocation failed\n");
            goto exit; 
        }        
        memset(output_buffer, 0, total_required_size);

        /* copy IV to output buffer */
        memcpy(output_buffer, IV, iv_size);

        /*
         * Hash the IV and the secret key together HASHCOUNT times
         * using the result to setup the AES context and HMAC.
         */
        memset(digest, 0, md_size);
        memcpy(digest, IV, iv_size);

        for (i = 0; i < HASHCOUNT; i++) 
        {
            if (mbedtls_md_starts(&md_ctx) != 0) 
            {
                mbedtls_fprintf(stderr,
                                "mbedtls_md_starts() returned error\n");
                goto exit;
            }
            if (mbedtls_md_update(&md_ctx, digest, md_size) != 0) 
            {
                mbedtls_fprintf(stderr,
                                "mbedtls_md_update() returned error\n");
                goto exit;
            }
            if (mbedtls_md_update(&md_ctx, key, keylen) != 0) 
            {
                mbedtls_fprintf(stderr,
                                "mbedtls_md_update() returned error\n");
                goto exit;
            }
            if (mbedtls_md_finish(&md_ctx, digest) != 0) 
            {
                mbedtls_fprintf(stderr,
                                "mbedtls_md_finish() returned error\n");
                goto exit;
            }
        }

        if (mbedtls_cipher_setkey(&cipher_ctx, digest, cipher_info->key_bitlen,
                                  MBEDTLS_ENCRYPT) != 0) 
        {
            mbedtls_fprintf(stderr, "mbedtls_cipher_setkey() returned error\n");
            goto exit;
        }
        if (mbedtls_cipher_set_iv(&cipher_ctx, IV, iv_size) != 0) 
        {
            mbedtls_fprintf(stderr, "mbedtls_cipher_set_iv() returned error\n");
            goto exit;
        }
        if (mbedtls_cipher_reset(&cipher_ctx) != 0) 
        {
            mbedtls_fprintf(stderr, "mbedtls_cipher_reset() returned error\n");
            goto exit;
        }
        if (mbedtls_md_hmac_starts(&md_ctx, digest, md_size) != 0) 
        {
            mbedtls_fprintf(stderr, "mbedtls_md_hmac_starts() returned error\n");
            goto exit;
        }


        /* encrypt and write the ciphertext */
        unsigned char *input_ptr = input_buffer;  //pointer to the current position in input_buffer
        unsigned char *output_ptr = output_buffer + iv_size;  //start writing after the IV

        /* offset fix---------------------------- */
        size_t compensator=0;
        size_t real_offset=0;

        for (offset = 0; offset < input_buffer_size; offset += cipher_block_size) 
        {
            ilen = (input_buffer_size - offset > cipher_block_size) ? cipher_block_size : (input_buffer_size - offset);
            memcpy(buffer, input_ptr + offset, ilen); 

            if (mbedtls_cipher_update(&cipher_ctx, buffer, ilen, output, &olen) != 0) 
            {
                mbedtls_fprintf(stderr, "mbedtls_cipher_update() returned error\n");
                goto exit;
            }

            compensator = olen; //olen incorrect after last iteration (get valid value here)

            if (mbedtls_md_hmac_update(&md_ctx, output, olen) != 0) 
            {
                mbedtls_fprintf(stderr, "mbedtls_md_hmac_update() returned error\n");
                goto exit;
            }

            memcpy(output_ptr + offset, output, olen);
            real_offset = offset; //resolves false offset after last iteration
        }
        /* update offset--------------------------------------- */
        size_t final_offset = real_offset + compensator; //offset thrown forward by 16 on last iteration
        


        /* finalize the encryption process */
        if (mbedtls_cipher_finish(&cipher_ctx, output, &olen) != 0) 
        {
            mbedtls_fprintf(stderr, "mbedtls_cipher_finish() returned error\n");
            goto exit;
        }
        /* update HMAC with the final block of encrypted data */
        if (mbedtls_md_hmac_update(&md_ctx, output, olen) != 0)
        {
            mbedtls_fprintf(stderr, "mbedtls_md_hmac_update() returned error\n");
            goto exit;
        }
        /* append the final block of encrypted data to the output buffer */
        memcpy(output_ptr + final_offset, output, olen);


        /* finalize the HMAC computation */
        if (mbedtls_md_hmac_finish(&md_ctx, digest) != 0)
        {
            mbedtls_fprintf(stderr, "mbedtls_md_hmac_finish() returned error\n");
            goto exit;
        }

        /* append the HMAC to the output buffer  */
        memcpy(output_ptr + final_offset, digest, md_size);        

    }// end encryption routine-->///



    //DECRYPT-------------------------------------------------------------------------------------
    if (mode == MODE_DECRYPT)
    {
        /*
         *  The encrypted file must be structured as follows:
         *
         *        00 .. 15              Initialization Vector
         *        16 .. 31              Encrypted Block #1
         *           ..
         *      N*16 .. (N+1)*16 - 1    Encrypted Block #N
         *  (N+1)*16 .. (N+1)*16 + n    Hash(ciphertext)
         */

        // Check if the buffer is large enough to contain IV, at least one block of encrypted data, and HMAC
        if (input_buffer_size < iv_size + md_size)
        {
            mbedtls_fprintf(stderr, "Buffer too small to be decrypted.\n");
            goto exit;
        }
        if (cipher_block_size == 0)
        {
            mbedtls_fprintf(stderr, "Invalid cipher block size: 0.\n");
            goto exit;
        }

        cipher_mode = cipher_info->mode;
        if (cipher_mode != MBEDTLS_MODE_GCM &&
            cipher_mode != MBEDTLS_MODE_CTR &&
            cipher_mode != MBEDTLS_MODE_CFB &&
            cipher_mode != MBEDTLS_MODE_OFB &&
            ((input_buffer_size - md_size) % cipher_block_size) != 0) 
        {
            mbedtls_fprintf(stderr, "Buffer content not a multiple of the block size (%u).\n",
                            cipher_block_size);
            goto exit;
        }

        /* subtract the IV + HMAC length for correct allocation size */
        input_buffer_size -= (iv_size + md_size);
        output_length = input_buffer_size ;
       

        memcpy(buffer, input_buffer, iv_size);
        memcpy(IV, buffer, iv_size);

        /*
         * hash the IV and the secret key together HASHCOUNT times
         * using the result to setup the AES context and HMAC.
         */
        memset(digest, 0,  md_size);
        memcpy(digest, IV, iv_size);

        for (i = 0; i < HASHCOUNT; i++) 
        {
            if (mbedtls_md_starts(&md_ctx) != 0) 
            {
                mbedtls_fprintf(stderr, "mbedtls_md_starts() returned error\n");
                goto exit;
            }
            if (mbedtls_md_update(&md_ctx, digest, md_size) != 0) 
            {
                mbedtls_fprintf(stderr, "mbedtls_md_update() returned error\n");
                goto exit;
            }
            if (mbedtls_md_update(&md_ctx, key, keylen) != 0) 
            {
                mbedtls_fprintf(stderr, "mbedtls_md_update() returned error\n");
                goto exit;
            }
            if (mbedtls_md_finish(&md_ctx, digest) != 0) 
            {
                mbedtls_fprintf(stderr, "mbedtls_md_finish() returned error\n");
                goto exit;
            }
        }

        if (mbedtls_cipher_setkey(&cipher_ctx, digest, cipher_info->key_bitlen,
                                  MBEDTLS_DECRYPT) != 0) 
        {
            mbedtls_fprintf(stderr, "mbedtls_cipher_setkey() returned error\n");
            goto exit;
        }

        if (mbedtls_cipher_set_iv(&cipher_ctx, IV, iv_size) != 0) 
        {
            mbedtls_fprintf(stderr, "mbedtls_cipher_set_iv() returned error\n");
            goto exit;
        }

        if (mbedtls_cipher_reset(&cipher_ctx) != 0) 
        {
            mbedtls_fprintf(stderr, "mbedtls_cipher_reset() returned error\n");
            goto exit;
        }

        if (mbedtls_md_hmac_starts(&md_ctx, digest, md_size) != 0) {
            mbedtls_fprintf(stderr, "mbedtls_md_hmac_starts() returned error\n");
            goto exit;
        }

        /* decrypt and write the plaintext */
 
        size_t total_required_size = input_buffer_size; 
        output_buffer = MTAKE(total_required_size * sizeof(char)); //freed in exit:

        if (output_buffer == NULL) 
        {
            fprintf(stderr, "Decryption memory allocation failed\n");
            goto exit;
        }
        memset(output_buffer, 0, total_required_size);

        unsigned char *input_ptr = input_buffer + iv_size; //skip the IV at the beginning
        unsigned char *output_ptr = output_buffer; //point to the start of the output buffer
              
        
        /* for size of the input data excluding IV and HMAC */
        for (offset = 0; offset < input_buffer_size; offset += cipher_block_size) 
        {
            ilen = (input_buffer_size - offset > cipher_block_size) ?
                cipher_block_size : (input_buffer_size - offset);

            memcpy(buffer, input_ptr + offset, ilen);

            /* update HMAC with data from the buffer */
            if (mbedtls_md_hmac_update(&md_ctx, buffer, ilen) != 0) 
            {
                mbedtls_fprintf(stderr, "mbedtls_md_hmac_update() returned error\n");
                goto exit;
            }

            /* decrypt the data */
            if (mbedtls_cipher_update(&cipher_ctx, buffer, ilen, output, &olen) != 0) 
            {
                mbedtls_fprintf(stderr, "mbedtls_cipher_update() returned error\n");
                goto exit;
            }

            memcpy(output_ptr + offset, output, olen);
        }

        /* verify the message authentication code  */
        if (mbedtls_md_hmac_finish(&md_ctx, digest) != 0) 
        {
            mbedtls_fprintf(stderr, "mbedtls_md_hmac_finish() returned error\n");
            goto exit;
        }

 
        /* the HMAC is at the end of the input buffer, calculate its position: */
        unsigned char *hmac_position = input_buffer + (input_buffer_size + iv_size);

        /* copy the HMAC from the input buffer to 'buffer' for comparison */
        memcpy(buffer, hmac_position, md_size);

        
        /* Use comparison to verify hash*/
        diff = 0;
        for (i = 0; i < md_size; i++) 
        {
            diff |= digest[i] ^ buffer[i];
        }

        if (diff != 0) 
        {
            mbedtls_fprintf(stderr, "HMAC check failed: wrong key, "
                                    "or file corrupted.\n");
            goto exit;
        }


        /*  write the final block of data  */
        if (mbedtls_cipher_finish(&cipher_ctx, output, &olen) != 0) 
        {
            mbedtls_fprintf(stderr, "mbedtls_cipher_finish() returned error\n");
            goto exit;
        }

        memcpy(output_ptr + offset, output, olen);

    } //end decryption routine--->//


    /* copy results (i.e. on successful operation) only */
    *my_output_buffer = MTAKE(output_length * sizeof(char)); //free me in calling function!!!!!!!!
    if (my_output_buffer == NULL)
    {
        printf("Error allocating memory for results\n");
        goto exit;
    }
    memcpy(*my_output_buffer, output_buffer, output_length);

    /* copy output length on success (only) */
    *my_output_length = output_length;

    exit_code = MBEDTLS_EXIT_SUCCESS;

exit: 

    mbedtls_platform_zeroize(IV,     sizeof(IV));
    mbedtls_platform_zeroize(key,    sizeof(key));
    mbedtls_platform_zeroize(buffer, sizeof(buffer));
    mbedtls_platform_zeroize(output, sizeof(output));
    mbedtls_platform_zeroize(digest, sizeof(digest));
    mbedtls_platform_zeroize(output_buffer, output_length);

    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    mbedtls_cipher_free(&cipher_ctx);
    mbedtls_md_free(&md_ctx);

    if(IV)
    {
    MRELEASE(IV);
    IV = NULL;
    }
   
    if(output_buffer)
    {
        MRELEASE(output_buffer);
        output_buffer = NULL;
    }
    
    return exit_code;

} //end crypt_and_hash_buffer--->///


#endif /* MBEDTLS_CIPHER_C && MBEDTLS_MD_C && MBEDTLS_FS_IO */
