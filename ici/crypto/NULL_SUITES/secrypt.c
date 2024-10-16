/**
 * @file secrypt.c
 * @brief This file contains non-cryptographic function stubs.
 *
 * @details
 * Core Functions:
 * - crypt_and_hash_buffer: non-cryptographic function stub.
 * - crypt_and_hash_file: non-cryptographic function stub.
 *
 *
 * @author Sky DeBaun, Jet Propulsion Laboratory
 * @date March 2024
 * @copyright Copyright (c) 2024, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 */

#define _POSIX_C_SOURCE 200112L //POSIX 2001 compliance check
#include "secrypt.h"


/******************************************************************************/
/*                   NON-CRYPTOGRAPHIC FUNCTION STUBS                         */
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
    int exit_code = 0; //success     

    /* allocate memory for the output buffer (if not already allocated) */
    if (*my_output_buffer == NULL) 
    {
        *my_output_buffer = (unsigned char*)malloc(*input_length);
        if (*my_output_buffer == NULL) 
        {
            return -1; //memory allocation failed
        }
    }
    else 
    {
         if (input_buffer == NULL)
        {
            printf("Input buffer is NULL\n");
            return -1;
        }        
        if (input_buffer == *my_output_buffer)
        {
            printf("Input and output buffers must not be the same\n");
            return -1;
        }
    }
    
    /* copy and return input as no encryption occurs */
    memcpy(*my_output_buffer, input_buffer, *input_length);
    *my_output_length = *input_length;

    return exit_code;
}

/******************************************************************************/
/** crypt_and_hash_file */
/******************************************************************************/
int crypt_and_hash_file(
    int my_mode, 
    unsigned char *personalization_string, 
    char *file_in, 
    char *file_out, 
    char *cipher, 
    char *md, 
    char *my_key
) 
{
   int exit_code = 0; //success

   /* open input file */
    FILE *fin = fopen(file_in, "rb"); 
    if (fin == NULL) 
    {
        perror("Error opening input file");
        return -1; 
    }

    /* open output file */
    FILE *fout = fopen(file_out, "wb");
    if (fout == NULL) 
    {
        perror("Error opening output file");
        fclose(fin); 
        return -1; 
    }

    char buffer[4096];
    size_t bytes_read;

    /* loop to read from the input file and write to the output file */
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), fin)) > 0) 
    {
        if (fwrite(buffer, 1, bytes_read, fout) != bytes_read) 
        {
            perror("Error writing to output file");
            fclose(fin);
            fclose(fout);
            return -1; 
        }
    }

    fclose(fin);
    fclose(fout);

    return exit_code;
}
