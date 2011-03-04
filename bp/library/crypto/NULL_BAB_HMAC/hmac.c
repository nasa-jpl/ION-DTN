/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2009 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 **     This material may only be used, modified, or reproduced by or for the
 **       U.S. Government pursuant to the license rights granted under
 **          FAR clause 52.227-14 or DFARS clauses 252.227-7013/7014
 **
 **     For any other permissions, please contact the Legal Office at JHU/APL.
 ******************************************************************************/

/*****************************************************************************
 ** \file hmac.c
 **
 ** File Name: hmac.c
 **
 **
 ** Subsystem:
 **          BSP-ION, but is project-independent.
 **
 ** Description: This file is an implementation of RFC 2104.  It has been
 **              valided with NIST test vectors.  This code is intended
 **              for the BSP-ION effort, but can be used on any project.
 **              RFC 2104 can be found at
 **              http://tools.ietf.org/html/rfc2104
 **
 **
 ** Notes:       This file contains a number of functions which are used as a 
 **              wrapper for an underlying hash function.  Currently it uses 
 **              the SHA1 implementation provided by Git.  To use a different 
 **              hash function, simply add a pre-compiler #ifdef block and 
 **              write your own wrapper for it in the hash_* functions.  Make 
 **              sure the change the proper constants, too.
 **
 ** Author:      Bill Van Besien
 **              William.Van.Besien@jhuapl.edu
 **              x89826
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 **  --------  ------------  -----------------------------------------------
 **  06/24/09  W. Van Besien        Tidy up for initial release (this added)
 **                                 Changed hmac_authenticate to return -1
 **                                 on error instead of 0.
 *****************************************************************************/

#include "hmac.h"


/******************************************************************************
 *
 * \par Function Name: demo_print
 *
 * \par Purpose: This utility function prints the contents of a buffer.  It is
 *               not currently used.
 *
 * \param[in,out]  name     label for this buffer
 *                 buffer   start address of buffer
 *                 length   length of this buffer in bytes
 *
 * \par Notes:
 *      1. This function currently does nothing.
 *****************************************************************************/

void demo_print(char * name, char * buffer, int len)
{
	return;
}


/*****************************************************************************
 *                         PUBLIC HMAC FUNCTIONS                             *
 *****************************************************************************/

/******************************************************************************
 *
 * \par Function Name: hmac_init
 *
 * \par Purpose: This utility funnction will initialize the hmac structure.
 *
 * \retval int - -1 - hmac structure is null.
 *                0 - OK; hmac structure initialized
 *                1 - hmac initialized, but the key length is shorter than
 *                    minimum length recommended by NIST.
 *
 * \param[in]  hmac        pointer to hmac structure
 *             key         pointer to key buffer
 *             key_length  length of key in bytes
 *
 * \par Notes:
 *      1. Key should be at least 20 bytes, if shorter than this will work,
 *         but it does not provide substantial security to brute-force
 *         cryptanalysis.
 *****************************************************************************/
int hmac_init(struct hmac_st * hmac, char * key, int key_length)
{
	return 0;
}

/******************************************************************************
 *
 * \par Function Name: hmac_hash
 *
 * \par Purpose: This function digests the data.   
 *
 * \retval int - -1 indicates hmac is null
 *                0 indicated OK, else error (always OK)
 *
 * \param[in]  hmac                pointer to hmac structure
 *             message             pointer to start of data to be digested
 *             message_length      size of message in bytes
 *
 * \par Notes:
 *      1. Future version will check to make sure message is no null and 
 *         that message length is greater than 0.
 *****************************************************************************/
int hmac_hash(struct hmac_st * hmac, char * message, int message_length)
{
	return 0;
}

/******************************************************************************
 *
 * \par Function Name: hmac_final
 *
 * \par Purpose: This function completes an hmac authentication.
 *
 * \retval int - 0 indicated OK, else error (always OK)
 *
 * \param[in]  hmac        pointer to hmac structure
 *
 * \par Notes:
 *      1. This function is a placeholder.  In later versions it may do
 *         something useful.
 *****************************************************************************/
int hmac_final(struct hmac_st * hmac)
{
	return 0;
}


/**
 * This is the entire HMAC algorithm wrapped into one function.  This returns 
 * the number of bytes in the MAC.
 */
/******************************************************************************
 *
 * \par Function Name: hmac_authenticate
 *
 * \par Purpose: This function does an entire hmac authentication.
 *
 * \retval int - -1 - Error: No hmac performed.  Check buffer sizes.
 *               >0 - Size of digest.  Compare this with mac_size
 *
 * \param[in]  mac_buffer       pointer to buffer containing output hmac 
 *                              security result (secure digest)
 *             mac_length       length of buffer in bytes
 *             key              pointer to buffer containing key
 *             key_length       length of key in bytes (should be at least 
                                20 bytes)
 *             message          pointer to data to be authenticated
 *             message_length   length in bytes
 *
 * \par Notes:
 *      1. Either this version can be used, or you can invoke the specific
 *         hash functions.
 *****************************************************************************/
int hmac_authenticate(char * mac_buffer, const int mac_size, 
                      const char * key, const int key_length, 
                      const char * message, const int message_length)
{
	memset(mac_buffer, 0, mac_size);
	return mac_size;
}
