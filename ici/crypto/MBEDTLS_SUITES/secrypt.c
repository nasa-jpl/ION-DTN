/*****************************************************************************
 **
 ** File Name: secrypt.c
 **
 ** Description: This file provides the implementation for the cryptographic
 ** functions defined in secrypt.h. It leverages the
 ** Mbed TLS (v2.28.x) library to perform secure authenticated
 ** encryption using an Encrypt-then-MAC scheme.
 **
 ** Key security features include:
 ** - Use of a standard HMAC-based Key Derivation Function (HKDF)
 ** to derive separate, strong keys for encryption and
 ** authentication from a single master secret.
 ** - Generation of unique, unpredictable Initialization Vectors
 ** (IVs) for each encryption operation using a secure
 ** random number generator.
 ** - Constant-time comparison of HMAC tags during decryption
 ** to prevent timing side-channel attacks.
 **
 **
 ** Modification History:
 ** MM/DD/YY  AUTHOR         DESCRIPTION
 ** --------  ------------   ---------------------------------------------
 **
 ** 03/15/24  S. DeBaun      Initial implementation.
 ** 06/08/25  S. DeBaun      Major security refactor. Replaced custom key
 **                          derivation with standard HKDF (RFC 5869),
 **                          implemented key separation, and simplified IV
 **                          generation to use CSPRNG directly.
 **
 *****************************************************************************/

#ifndef darwin
#ifndef freebsd
/* needed to compile on 32-bit Raspberry PI */
#define _GNU_SOURCE
/* #define _POSIX_C_SOURCE 200112L */
#endif
#endif
#include "secrypt.h"
#include "../entropy_src.h" /* For cross-platform entropy polling (libici) */

#include <bp.h>


/* Check for MBEDTLS cipher suite----------------------------- */
#if !defined(MBEDTLS_CIPHER_C) || !defined(MBEDTLS_MD_C)                   \
		|| !defined(MBEDTLS_FS_IO) || !defined(MBEDTLS_CTR_DRBG_C) \
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
}

/******************************************************************************/
/** print_encrypted_data */
/******************************************************************************/
void print_encrypted_data(const unsigned char *data, size_t length)
{
	for (size_t i = 0; i < length; i++)
	{
		if (isprint(data[i]))
		{
			printf("%c", data[i]); /* print as character */
		}
		else
		{
			printf("."); /* placeholder for non-printable characters */
		}
	}
	printf("\n");
}



/******************************************************************************/
/*         CRYPTOGRAPHIC OPERATIONS        CRYPTOGRAPHIC OPERATIONS           */
/******************************************************************************/

/******************************************************************************/
/** derive_keys */
/******************************************************************************/
/**
 * @brief Derives separate encryption and authentication keys from a master secret.
 *
 * This function uses the standard HMAC-based Key Derivation Function (HKDF)
 * to convert a single master key into cryptographically separate keys for
 * use in an Encrypt-then-MAC scheme.
 *
 * @param md_info           The hash algorithm to use for HKDF.
 * @param salt              The salt for the derivation (typically the IV).
 * @param salt_len          Length of the salt.
 * @param ikm               The Initial Keying Material (the master secret key).
 * @param ikm_len           Length of the master secret key.
 * @param encryption_key    Output buffer for the derived encryption key.
 * @param encryption_key_len Length of the required encryption key.
 * @param auth_key          Output buffer for the derived authentication (HMAC) key.
 * @param auth_key_len      Length of the required authentication key.
 * @return 0 on success, non-zero on failure.
 */
/******************************************************************************/
static int derive_keys(const mbedtls_md_info_t *md_info,
		const unsigned char *salt, size_t salt_len,
		const unsigned char *ikm, size_t ikm_len,
		unsigned char *encryption_key, size_t encryption_key_len,
		unsigned char *auth_key, size_t auth_key_len)
{
	int	      ret = -1;
	/* Buffer to hold the combined output from HKDF */
	unsigned char okm[encryption_key_len + auth_key_len];

	/* Use the standard HKDF to derive the Output Keying Material (okm) */
	ret = mbedtls_hkdf(md_info, salt, salt_len, ikm, ikm_len,
			NULL, 0, // Optional "info" field not used here
			okm, sizeof(okm));

	if (ret == 0)
	{
		/* HKDF success, now split the OKM into two separate keys */
		memcpy(encryption_key, okm, encryption_key_len);
		memcpy(auth_key, okm + encryption_key_len, auth_key_len);
	}

	/* IMPORTANT: Securely wipe the buffer that held the combined keys */
	mbedtls_platform_zeroize(okm, sizeof(okm));

	return ret;
}


/******************************************************************************/
/** entropy_init */
/******************************************************************************/
int entropy_init(mbedtls_entropy_context *entropy)
{
	/* initialize the entropy context object */
	mbedtls_entropy_init(entropy);

	/* add entropy source - see: ici/crypto/entropy_src.c: poll_entropy_src */
	mbedtls_entropy_add_source(entropy, poll_entropy_src, NULL, 0,
			MBEDTLS_ENTROPY_SOURCE_STRONG);
	return 0;
}


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
	int i; /* iterator */
	int status = -1;
	int exit_code = MBEDTLS_EXIT_FAILURE; /* default to failure */
	size_t keylen=0, ilen=0, olen=0;
	size_t input_buffer_size = *input_length;

	unsigned char *output_buffer = NULL;
	size_t output_length = 0; /* return 0 on failure */

	unsigned char key[MAXKEYSIZE] = {0};
	unsigned char digest[MBEDTLS_MD_MAX_SIZE];
	memset(digest, 0, MBEDTLS_MD_MAX_SIZE);
	unsigned char buffer[BUFFSIZE];
	memset(buffer, 0, BUFFSIZE);
	unsigned char output[BUFFSIZE];
	memset(output, 0, BUFFSIZE);
	unsigned char diff;

	mbedtls_ctr_drbg_context ctr_drbg;
	mbedtls_entropy_context entropy = {0};

	const mbedtls_cipher_info_t *cipher_info;
	const mbedtls_md_info_t *md_info;
	mbedtls_cipher_context_t cipher_ctx;
	mbedtls_md_context_t md_ctx;
	mbedtls_cipher_mode_t cipher_mode;
	unsigned int cipher_block_size=0;
	unsigned char md_size=0;
	FILE *fkey; /* key file */
	unsigned char *IV = NULL;


	size_t  offset = 0;


	mbedtls_cipher_init(&cipher_ctx);
	mbedtls_md_init(&md_ctx);

	/*  Basic error checking------------------------------------ */
	if (mode != MODE_ENCRYPT && mode != MODE_DECRYPT)
	{
		putErrmsg("[!] libsecrypt error:", "invalid operation mode.");
		goto exit;
	}

	if (input_buffer == *my_output_buffer)
	{
		putErrmsg("[!] libsecrypt error:",
				"input and output buffers must not be the same.");
		goto exit;
	}

	if (input_buffer == NULL)
	{
		putErrmsg("[!] libsecrypt error:", "Input or output buffer is NULL.");
		goto exit;
	}

	cipher_info = mbedtls_cipher_info_from_string(cipher);

	if (cipher_info == NULL)
	{
		putErrmsg("[!] libsecrypt error:",
				"mbedtls_cipher_info_from_string cipher argument can not be NULL.");
		goto exit;
	}

	if ((mbedtls_cipher_setup(&cipher_ctx, cipher_info)) != 0)
	{
		putErrmsg("[!] libsecrypt error:", "mbedtls_cipher_setup failed.");
		goto exit;
	}

	md_info = mbedtls_md_info_from_string(md);
	if (md_info == NULL)
	{
		putErrmsg("[!] libsecrypt Message Digest not found: ", md);
		goto exit;
	}

	if (mbedtls_md_setup(&md_ctx, md_info, 1) != 0)
	{
		putErrmsg("[!] libsecrypt error:", "mbedtls_md_setup failed.");
		goto exit;
	}

	/* TODO: READ KEY FROM IONSECRC INSTEAD*/

	/*  Read the secret key from file */
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

	/* set IV size ----------------------------------------------*/
	size_t iv_size = cipher_block_size;
	IV = MTAKE(iv_size); /* freed at exit: */
	if (IV == NULL)
	{
		putErrmsg("[!] libsecrypt error:", "IV Memory allocation failed.");
		goto exit;
	}
	memset(IV, 0, iv_size);


	/* ENCRYPT---------------------------------------------------------------- */
	if (mode == MODE_ENCRYPT)
	{
		/* INITIALIZE RANDOMIZER--------------------------------- */
		mbedtls_ctr_drbg_init(&ctr_drbg);

		/* INITIALIZE AND SEED ENTROPY--------------------------- */
		entropy_init(&entropy);

		/* Seeded with non cryptographic string */
		status = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
				(const unsigned char *) personalization_string,
				strlen((const char *) personalization_string));
		if (status != 0)
		{
			putErrmsg("[!] libsecrypt encryption error:",
					"mbedtls_ctr_drbg_seed failed.");
			goto exit;
		}

		/* enable prediction resistance */
		mbedtls_ctr_drbg_set_prediction_resistance(&ctr_drbg,
				MBEDTLS_CTR_DRBG_PR_ON);

		if (status != 0)
		{
			putErrmsg("[!] libsecrypt encryption error:",
					"GCM generation failed.");
			goto exit;
		}

		/* generate the IV */
		if (mbedtls_ctr_drbg_random(&ctr_drbg, IV, iv_size) != 0)
		{
			putErrmsg("[!] libsecrypt encryption error:",
					"ctr_drbg_random IV generation failed.");
			goto exit;
		}

		/*
		 * The output buffer is structured with the IV at the beginning, so we
		 * calculate the total required size by adding the crypto overhead (IV and
		 * message digest/tag) to the original data size.
		 */
		size_t total_required_size = input_buffer_size + iv_size + md_size;
		output_length = total_required_size; /* set final size for the caller */

		output_buffer = MTAKE(total_required_size * sizeof(char)); /* freed in exit */

		if (output_buffer == NULL)
		{
			putErrmsg("[!] libsecrypt encryption error:",
					"output buffer memory allocation failed.");
			goto exit;
		}
		memset(output_buffer, 0, total_required_size);

		/* copy IV to output buffer */
		memcpy(output_buffer, IV, iv_size);

		/*
		 * Derive separate encryption and authentication keys using the
		 * standard HKDF function.
		 */
		unsigned char encryption_key[mbedtls_cipher_get_key_bitlen(&cipher_ctx) / 8];
		unsigned char auth_key[md_size];

		if (derive_keys(md_info, IV, iv_size, key, keylen,
				encryption_key, sizeof(encryption_key),
				auth_key, sizeof(auth_key)) != 0)
		{
			putErrmsg("[!] libsecrypt error:", "Key derivation (HKDF) failed.");
			goto exit;
		}

		if (mbedtls_cipher_setkey(&cipher_ctx, encryption_key,
				cipher_info->key_bitlen,
				MBEDTLS_ENCRYPT) != 0)
		{
			putErrmsg("[!] libsecrypt encryption error:",
					"mbedtls_cipher_setkey failed.");
			goto exit;
		}
		if (mbedtls_cipher_set_iv(&cipher_ctx, IV, iv_size) != 0)
		{
			putErrmsg("[!] libsecrypt encryption error:",
					"mbedtls_cipher_set_iv failed.");
			goto exit;
		}
		if (mbedtls_cipher_reset(&cipher_ctx) != 0)
		{
			putErrmsg("[!] libsecrypt encryption error:",
					"mbedtls_cipher_reset failed.");
			goto exit;
		}
		if (mbedtls_md_hmac_starts(&md_ctx, auth_key, md_size) != 0)
		{
			putErrmsg("[!] libsecrypt encryption error:",
					"mbedtls_md_hmac_starts failed.");
			goto exit;
		}

		/* point to current pos. in input_buffer */
		unsigned char *input_ptr = input_buffer;

		/* start writing after the IV */
		unsigned char *output_ptr = output_buffer + iv_size;

		/* offset fix------------------------------------------- */
		size_t compensator=0;
		size_t real_offset=0;

		for (offset = 0; offset < input_buffer_size; offset += cipher_block_size)
		{
			ilen = (input_buffer_size - offset > cipher_block_size)
				? cipher_block_size
				: (input_buffer_size - offset);

			memcpy(buffer, input_ptr + offset, ilen);

			if (mbedtls_cipher_update(&cipher_ctx, buffer, ilen, output, &olen) != 0)
			{
				putErrmsg("[!] libsecrypt encryption error:",
						"mbedtls_cipher_update failed.");
				goto exit;
			}

			compensator = olen; /* olen incorrect after last iter. (get valid value here) */

			if (mbedtls_md_hmac_update(&md_ctx, output, olen) != 0)
			{
				putErrmsg("[!] libsecrypt encryption error:",
						"mbedtls_md_hmac_update failed.");
				goto exit;
			}

			memcpy(output_ptr + offset, output, olen);
			real_offset = offset; /* resolves false offset after last iteration */
		}
		/* update offset (offset forward 16 on last iteration) */
		size_t final_offset = real_offset + compensator;


		/* Finalize the encryption process ---------------------- */
		if (mbedtls_cipher_finish(&cipher_ctx, output, &olen) != 0)
		{
			putErrmsg("[!] libsecrypt encryption error:",
					"mbedtls_cipher_finish failed.");
			goto exit;
		}
		/* update HMAC with the final block of encrypted data */
		if (mbedtls_md_hmac_update(&md_ctx, output, olen) != 0)
		{
			putErrmsg("[!] libsecrypt encryption error:",
					"mbedtls_md_hmac_update failed.");
			goto exit;
		}
		/* append the final block of encrypted data to the output buffer */
		memcpy(output_ptr + final_offset, output, olen);


		/* finalize the HMAC computation */
		if (mbedtls_md_hmac_finish(&md_ctx, digest) != 0)
		{
			putErrmsg("[!] libsecrypt encryption error:",
					"mbedtls_md_hmac_finish failed.");
			goto exit;
		}

		/* append the HMAC to the output buffer  */
		memcpy(output_ptr + final_offset, digest, md_size);

	} /* end encryption routine */


	/* DECRYPT------------------------------------------------------------------ */
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

		/*
		    Check if the buffer is large enough to contain IV, at least one block
		    of encrypted data, and HMAC
		 */
		if (input_buffer_size < iv_size + md_size)
		{
			putErrmsg("[!] libsecrypt decryption error:",
					"input buffer too small to be decrypted.");
			goto exit;
		}
		if (cipher_block_size == 0)
		{
			putErrmsg("[!] libsecrypt decryption error:",
					"Invalid cipher block size (0).");
			goto exit;
		}

		cipher_mode = cipher_info->mode;
		if (cipher_mode != MBEDTLS_MODE_GCM &&
			cipher_mode != MBEDTLS_MODE_CTR &&
			cipher_mode != MBEDTLS_MODE_CFB &&
			cipher_mode != MBEDTLS_MODE_OFB &&
			((input_buffer_size - md_size) % cipher_block_size) != 0)
		{
			putErrmsg("[!] libsecrypt decryption error:",
					"Buffer content not a multiple of block size.");
			goto exit;
		}

		/* subtract the IV + HMAC length for correct allocation size */
		input_buffer_size -= (iv_size + md_size);
		output_length = input_buffer_size;

		memcpy(buffer, input_buffer, iv_size);
		memcpy(IV, buffer, iv_size);

		/*
		 * Derive separate encryption and authentication keys using the
		 * standard HKDF function.
		 */
		unsigned char encryption_key[mbedtls_cipher_get_key_bitlen(&cipher_ctx) / 8];
		unsigned char auth_key[md_size];

		if (derive_keys(md_info, IV, iv_size, key, keylen,
				encryption_key, sizeof(encryption_key),
				auth_key, sizeof(auth_key)) != 0)
		{
			putErrmsg("[!] libsecrypt error:", "Key derivation (HKDF) failed.");
			goto exit;
		}

		if (mbedtls_cipher_setkey(&cipher_ctx, encryption_key, cipher_info->key_bitlen,
				MBEDTLS_DECRYPT) != 0)
		{
			putErrmsg("[!] libsecrypt decryption error:",
					"mbedtls_cipher_setkey failed.");
			goto exit;
		}

		if (mbedtls_cipher_set_iv(&cipher_ctx, IV, iv_size) != 0)
		{
			putErrmsg("[!] libsecrypt decryption error:",
					"mbedtls_cipher_set_iv failed.");
			goto exit;
		}

		if (mbedtls_cipher_reset(&cipher_ctx) != 0)
		{
			putErrmsg("[!] libsecrypt decryption error:",
					"mbedtls_cipher_reset failed.");
			goto exit;
		}

		if (mbedtls_md_hmac_starts(&md_ctx, auth_key, md_size) != 0)
		{
			putErrmsg("[!] libsecrypt decryption error:",
					"mbedtls_md_hmac_starts failed.");
			goto exit;
		}

		/* Decrypt and write the plaintext */

		size_t total_required_size = input_buffer_size;
		output_buffer = MTAKE(total_required_size * sizeof(char)); /* freed in exit */

		if (output_buffer == NULL)
		{
			putErrmsg("[!] libsecrypt decryption memory allocation failed",
					"MTAKE returned NULL.");
			goto exit;
		}
		memset(output_buffer, 0, total_required_size);

		unsigned char *input_ptr = input_buffer + iv_size; /* skip the IV at beginning */
		unsigned char *output_ptr = output_buffer; /* point to start of output buffer */


		/* for size of the input data excluding IV and HMAC */
		for (offset = 0; offset < input_buffer_size; offset += cipher_block_size)
		{
			ilen = (input_buffer_size - offset > cipher_block_size) ?
				cipher_block_size : (input_buffer_size - offset);

			memcpy(buffer, input_ptr + offset, ilen);

			/* update HMAC with data from the buffer */
			if (mbedtls_md_hmac_update(&md_ctx, buffer, ilen) != 0)
			{
				putErrmsg("[!] libsecrypt decryption error:",
						"mbedtls_md_hmac_update failed.");
				goto exit;
			}

			/* decrypt the data */
			if (mbedtls_cipher_update(&cipher_ctx, buffer, ilen, output, &olen) != 0)
			{
				putErrmsg("[!] libsecrypt decryption error:",
						"mbedtls_cipher_update failed.");
				goto exit;
			}

			memcpy(output_ptr + offset, output, olen);
		}

		/* verify the message authentication code  */
		if (mbedtls_md_hmac_finish(&md_ctx, digest) != 0)
		{
			putErrmsg("[!] libsecrypt decryption error:",
					"mbedtls_md_hmac_finish failed.");
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
			putErrmsg("[!] libsecrypt decryption error:",
					"HMAC check failed - wrong key or file altered.");
			goto exit;
		}

		/*  write the final block of data  */
		if (mbedtls_cipher_finish(&cipher_ctx, output, &olen) != 0)
		{
			putErrmsg("[!] libsecrypt decryption error:",
					"mbedtls_md_hmac_finish HMAC check failed.");
			goto exit;
		}

		memcpy(output_ptr + offset, output, olen);

	} /* end decryption routine */


	/* Copy results (i.e. on successful operation) only ---------*/
	/* FREE ME IN CALLING FUNCTION !!! */
	*my_output_buffer = MTAKE(output_length * sizeof(char)); /* FREE ME!!! */

	if (my_output_buffer == NULL)
	{
		putErrmsg("[!] libsecrypt error allocating memory:",
				"outbut buffer is NULL");
		goto exit;
	}
	memcpy(*my_output_buffer, output_buffer, output_length);

	/* copy output length on success (only) */
	*my_output_length = output_length;

	exit_code = MBEDTLS_EXIT_SUCCESS;

exit:

	mbedtls_platform_zeroize(IV, sizeof(IV));
	mbedtls_platform_zeroize(key, sizeof(key));
	mbedtls_platform_zeroize(buffer, sizeof(buffer));
	mbedtls_platform_zeroize(output, sizeof(output));
	mbedtls_platform_zeroize(digest, sizeof(digest));
	mbedtls_platform_zeroize(output_buffer, output_length);

	mbedtls_ctr_drbg_free(&ctr_drbg);
	mbedtls_entropy_free(&entropy);
	mbedtls_cipher_free(&cipher_ctx);
	mbedtls_md_free(&md_ctx);

	if (IV)
	{
		MRELEASE(IV);
		IV = NULL;
	}

	if (output_buffer)
	{
		MRELEASE(output_buffer);
		output_buffer = NULL;
	}

	return exit_code;

}
#endif /* MBEDTLS_CIPHER_C && MBEDTLS_MD_C && MBEDTLS_FS_IO */
