/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** \file utils.c
 **
 ** Subsystem:
 **          Network Management Utilities
 **
 ** Description:
 **
 ** Notes:
 **   1. TODO: more work needed to serialize/deserialize floating point. Approach
 **      is to captur ein a string. Some evident that an IEEE 754 double loses
 **      significant precision after ~16 digits.
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  10/29/12  E. Birrane     Initial Implementation. (JHU/APL)
 **  ??/??/16  E. Birrane     Added Serialize/Deserialize Functions.
 **                           Added "safe" memory functions.
 **                           Document Updates (Secure DTN - NASA: NNX14CS58P)
 **  07/04/16  E. Birrane     Added limited support for serialize/deserialize
 **                           floats and doubles. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

#include "platform.h"
#include "ion.h"

#include "../utils/debug.h"
#include "../utils/utils.h"

static ResourceLock gMemMutex;

#if AMP_DEBUGGING == 1
char gAmpMsg[AMP_GMSG_BUFLEN];
#endif

int8_t utils_mem_int()
{
	if(initResourceLock(&gMemMutex))
	{
		AMP_DEBUG_ERR("utils_mem_int", "Cannot allocate memory mutex.", NULL);
		return ERROR;

	}
	return SUCCESS;
}

void utils_mem_teardown()
{
	killResourceLock(&gMemMutex);
}


void* utils_safe_take(size_t size)
{
	void *result;

	lockResource(&gMemMutex);
	result = MTAKE(size);
	//result = malloc(size);
	if(result != NULL)
	{
		memset(result,0,size);
	}
	unlockResource(&gMemMutex);
	return result;
}

void utils_safe_release(void* ptr)
{
	lockResource(&gMemMutex);
	MRELEASE(ptr);
	//free(ptr);
	unlockResource(&gMemMutex);
}

/******************************************************************************
 *
 * \par Function Name: atox
 *
 * \par Initializes an unsigned long with a value from a string. For example,
 * 		return an unsigned long with the value 0x01020304 from the string
 * 		"01020304".
 *
 * \param[in]  s        The string to be converted to a hex array.
 * \param[out] success  Whether the conversion was a success.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/25/12  E. Birrane     Initial implementation. (JHU/APL)
 *  12/16/12  E. Birrane     Added success return, error checks, logging (JHU/APL)
 *****************************************************************************/

unsigned long utils_atox(char *s, int *success)
{
	unsigned long result = 0;
	int i = 0;
	int mult = 0;
	int j = 0;
	int temp = 0;

	AMP_DEBUG_ENTRY("utils_atox","(%#llx, %#llx)", s, success);

	/* Step 0 - Sanity Check. */
	if (success == NULL)
	{
		AMP_DEBUG_ERR("utils_atox","Bad Args.",NULL);
		AMP_DEBUG_ENTRY("utils_atox","->0.",NULL);
		return 0;
	}

	*success = 0;
	if(s == NULL)
	{
		AMP_DEBUG_ERR("utils_atox","Bad Args.",NULL);
		AMP_DEBUG_ENTRY("utils_atox","->0.",NULL);
		return 0;
	}

	/*
	 * Step 0.5 Handle case where string starts with "0x" by simply
	 * advancing s to skip over it. This won't modify s from the
	 * caller point of view.
	 */
	if((s[0]=='0') && (s[1]=='x'))
	{
		s = s + 2;
	}

	*success = 1;

	/* Step 1 - Make sure string isn't too long. Since every character in the
	 *          string represents a nibble, 2 characters are a byte, making
	 *          the longest valid length sizeof(unsigned long) * 2.
	 */
	if(strlen(s) > (sizeof(unsigned long) * 2))
	{
		AMP_DEBUG_ERR("utils_atox","x UI: String %s too long to convert to hex unsigned long.", s);
		*success = 0;
		AMP_DEBUG_ENTRY("utils_atox","->0.",NULL);
		return 0;
	}

	/* Step 2 - Walk through the string building the result. */
	for(i = strlen(s)-1; i >= 0; i--)
	{
		mult = j * 16;
		if(mult == 0)
		{
			mult = 1;
		}

		switch(s[i])
		{
		case '0': case '1': case '2': case '3': case '4': case '5': case '6':
		case '7': case '8': case '9':
				  temp = s[i] - '0';
				  result += temp * mult;
				  break;
		case 'A': case 'a': result += 10 * mult; break;
		case 'B': case 'b': result += 11 * mult; break;
		case 'C': case 'c': result += 12 * mult; break;
		case 'D': case 'd': result += 13 * mult; break;
		case 'E': case 'e': result += 14 * mult; break;
		case 'F': case 'f': result += 15 * mult; break;
		default:
			AMP_DEBUG_ERR("utils_atox","x Non-hex character: %c", s[i]);
			*success = 0;
			j--;
			break;
		}
		j++;
	}

//	DTNMP_DEBUG_INFO("utils_atox","i UI: Turned string %s to number %x.", s, result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: utils_grab_byte
 *
 * \par Purpose: extracts a byte from a sized buffer.
 *
 * \return 0 - Failure.
 * 		   >0 - # bytes consumed from the buffer.
 *
 * \param[in,out]  cursor      Pointer into current buffer.
 * \param[in]      size        Remaining size of the buffer.
 * \param[out]     result      The extracted byte.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/14/12  E. Birrane     Initial implementation, (JHU/APL)
 *****************************************************************************/

int8_t utils_grab_byte(unsigned char *cursor,
		  		       uint32_t size,
				       uint8_t *result)
{
	AMP_DEBUG_ENTRY("utils_grab_byte","(%x,%d,%x)",
			          (unsigned long) cursor, size,
			          (unsigned long) result);

	/* Do we have a byte to grab? */
	if(size < 1)
	{
        AMP_DEBUG_ERR("utils_grab_byte","Bounds overrun. Size %d Used %d.",
        				size, 1);
        AMP_DEBUG_EXIT("utils_grab_byte","-> 0", NULL);
        return 0;
	}

    *result = *cursor;

    AMP_DEBUG_EXIT("utils_grab_byte","-> 1", NULL);

    return 1;
}



/******************************************************************************
 *
 * \par Function Name: utils_grab_sdnv
 *
 * \par Purpose: extracts an SDNV value from a sized buffer.
 *
 * \return 0 - Failure.
 * 		   >0 - # bytes consumed from the buffer.
 *
 * \param[in,out]  cursor      Pointer into current buffer.
 * \param[in]      size        Remaining size of the buffer.
 * \param[out]     result      The extracted value.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/14/12  E. Birrane     Initial implementation, (JHU/APL)
 *  06/17/13  E. Birrane     Updated to ION 3.1.3, added uvast type. (JHU/APL)
 *****************************************************************************/

uint32_t utils_grab_sdnv(unsigned char *cursor,
		                 uint32_t size,
		                 uvast *result)
{
	int result_len = 0;

	AMP_DEBUG_ENTRY("utils_grab_sdnv","(%x,%d,%x)",
			          (unsigned long) cursor,
			          (unsigned long) size,
			          (unsigned long) result);

    if((result_len = decodeSdnv(result, cursor)) == 0)
    {
        AMP_DEBUG_ERR("utils_grab_sdnv","Bad SDNV extract.", NULL);
		AMP_DEBUG_EXIT("utils_grab_sdnv","-> 0", NULL);
        return 0;
    }

    /* Did we go too far? */
	if (result_len > size)
	{
		AMP_DEBUG_ERR("utils_grab_sdnv","Bounds overrun. Size %d Used %d.",
						size, result_len);

		AMP_DEBUG_EXIT("utils_grab_sdnv","-> 0", NULL);
		return 0;
	}

	AMP_DEBUG_EXIT("utils_grab_sdnv","-> %d", result_len);
	return result_len;
}



/******************************************************************************
 *
 * \par Function Name: utils_hex_to_string
 *
 * \par Purpose: Constructs a character string of values from a buffer.
 *
 * \return NULL - Failure.
 * 		   !NULL - Desired string
 *
 * \param[in]  buffer  The buffer whose string representation is desired.
 * \param[in]  size    Size of the buffer, in bytes,
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/14/12  E. Birrane     Initial implementation (JHU/APL)
 *****************************************************************************/

char *utils_hex_to_string(uint8_t *buffer, uint32_t size)
{
    char *result = NULL;
    uint32_t char_size = 0;

    char temp[3];
    int i = 0;
    int r = 0;

    AMP_DEBUG_ENTRY("utils_hex_to_string","(%x,%d)",
    		          (unsigned long) buffer, size);

    /* Each byte requires 2 characters to represent in HEX. Also, require
     * three additional bytes to capture '0x' and NULL terminator.
     */
    char_size = (2 * size) + 3;
    result = (char *) STAKE(char_size);

    if(result == NULL)
    {
        AMP_DEBUG_ERR("utils_hex_to_string", "Cannot allocate %d bytes.",
        		        char_size);
        AMP_DEBUG_EXIT("utils_hex_to_string", "-> NULL.", NULL);
        return NULL;
    }

    result[0] = '0';
    result[1] = 'x';
    r = 2;

    for(i = 0; i < size; i++)
    {
        sprintf(temp, "%.2x", (unsigned int)buffer[i]);
        result[r++] = temp[0];
        result[r++] = temp[1];
    }

    result[r] = '\0';

    AMP_DEBUG_EXIT("mid_to_string","->%s.", result);

    return result;
}



/******************************************************************************
 *
 * \par Function Name: utils_print_hex
 *
 * \par Purpose: Prints a string as a series of hex characters.
 *
 * \param[in]  s     The string to be printed.
 * \param[in]  len   The length of the string.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/14/12  E. Birrane     Initial implementation (JHU/APL)
 *****************************************************************************/

void utils_print_hex(unsigned char *s, uint32_t len)
{
	int i;

	printf("0x");
	for(i = 0; i < len; i++)
	{
		printf("%.2x", s[i]);
	}
	printf("\n");
}



/******************************************************************************
 *
 * \par Function Name: utils_string_to_hex
 *
 * \par Purpose: Converts an ASCII string representing hex values to a byte
 *               array of those hex values.
 *
 * \return NULL - Failure.
 * 		   !NULL - The desired byte array.
 *
 * \param[in]   value  The string to be converted to hex.
 * \param[out]  size   The length of the converted byte array.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/14/12  E. Birrane     Initial implementation (JHU/APL)
 *****************************************************************************/

uint8_t getNibble(char c)
{
	if(c >= '0' && c <= '9') return c - '0';
	if(c >= 'a' && c <= 'f') return c - 'a' + 10;
	if( c>= 'A' && c <= 'F') return c - 'A' + 10;

	return 255;
}



uint8_t *utils_string_to_hex(char *value, uint32_t *size)
{
	uint8_t *result = NULL;
	char tmp_s[3];
	int len = 0;
	int success = 0;
	int pad = 0; 

	AMP_DEBUG_ENTRY("utils_string_to_hex","(%#llx, %#llx)", value, size);

	/* Step 0 - Sanity Checks. */
	if((value == NULL) || (size == NULL))
	{
		AMP_DEBUG_ERR("utils_string_to_hex", "Bad Args.", NULL);
		AMP_DEBUG_EXIT("utils_string_to_hex", "->NULL.", NULL);
		return NULL;
	}

	/*
	 * Step 0.5 Handle case where string starts with "0x" by simply
	 * advancing s to skip over it. This won't modify s from the
	 * caller point of view.
	 */
	if((value[0]=='0') && (value[1]=='x'))
	{
		value = value + 2;
	}


	/* Step 1 - Figure out the size of the byte array. Since each ASCII
	 *          character represents a nibble, the size of the byte array is
	 *          half the size of the string (accounting for odd values).
	 */
	len = strlen((char*)value);

	if((len%2) == 0)
	{
	  *size = len/2;
	}
	else
	{
  	*size = (len/2) + 1;
       pad = 1;
	}

	if((result = (uint8_t *) STAKE(*size+1)) == NULL)
	{
		AMP_DEBUG_ERR("utils_string_to_hex","Can't Alloc %d bytes.", *size);
		*size = 0;

		AMP_DEBUG_EXIT("utils_string_to_hex", "->NULL.", NULL);
		return NULL;
	}

	/* Step 2 - For each byte, copy in the nibbles. */
	tmp_s[2] = '\0';
	int incr = 1;
    int base = 0;
    int i = 0;

	for(i = 0; i < len;)
	{
		if(pad == 1)
		{
			tmp_s[0] = '0';
			tmp_s[1] = value[i];
			pad = 0;
			incr = 1;
			base = 1;
		}
		else
		{
			memcpy(tmp_s, &(value[i]), 2);
			incr = 2;
		}

		result[(i+base)/2] = utils_atox(tmp_s, &success);
	
		i += incr;
		if(success == 0)
		{
			AMP_DEBUG_ERR("utils_string_to_hex","Can't AtoX %s.", tmp_s);
			SRELEASE(result);
			*size = 0;

			AMP_DEBUG_EXIT("utils_string_to_hex", "->NULL.", NULL);
			return NULL;
		}
	}

	AMP_DEBUG_EXIT("utils_string_to_hex", "->%#llx.", result);
	return result;
}




/*
 * THis software adapted from:
 * http://www.gnu.org/software/libc/manual/html_node/Elapsed-Time.html
 *
 * performs: result = t1 - t2.
 */
int utils_time_delta(struct timeval *result, struct timeval *t1, struct timeval *t2)
{
	/* Perform the carry for the later subtraction by updating t2. */
	if (t1->tv_usec < t2->tv_usec) {
		int nsec = (t2->tv_usec - t1->tv_usec) / 1000000 + 1;
		t2->tv_usec -= 1000000 * nsec;
		t2->tv_sec += nsec;
	}
	if (t1->tv_usec - t2->tv_usec > 1000000) {
		int nsec = (t1->tv_usec - t2->tv_usec) / 1000000;
		t2->tv_usec += 1000000 * nsec;
		t2->tv_sec -= nsec;
	}

	/* Compute the time remaining to wait.
	          tv_usec is certainly positive. */
	result->tv_sec = t1->tv_sec - t2->tv_sec;
	result->tv_usec = t1->tv_usec - t2->tv_usec;

	/* Return 1 if result is negative. */
	return t1->tv_sec < t2->tv_sec;
}



/* Return number of micro-seconds that have elapsed since the passed-in time.*/
vast    utils_time_cur_delta(struct timeval *t1)
{
	vast result = 0;

	struct timeval cur;
	struct timeval delta;
	int neg = 0;

	getCurrentTime(&cur);
	neg = utils_time_delta(&delta, &cur, t1);

	result = delta.tv_usec;
	result += delta.tv_sec * 1000000;

	if(neg)
	{
		result *= -1;
	}

	return result;
}



int32_t  utils_deserialize_int(uint8_t *buffer, uint32_t bytes_left, uint32_t *bytes_used)
{
	return (int32_t) utils_deserialize_uint(buffer, bytes_left, bytes_used);
}




float    utils_deserialize_real32(uint8_t *buffer, uint32_t bytes_left, uint32_t *bytes_used)
{
	float result = 0;
	char *endstr = NULL;

	errno = 0;
	result = strtof((char *)buffer, &endstr);

	if(errno != 0)
	{
		AMP_DEBUG_ERR("utils_deserialize_real32","Error deserializing. Errno %d", errno);
		*bytes_used = 0;
		result = 0;
	}
	else
	{
		*bytes_used = endstr - (char *) buffer;
	}

	return result;
}

double   utils_deserialize_real64(uint8_t *buffer, uint32_t bytes_left, uint32_t *bytes_used)
{
	double result = 0;
	char *endstr = NULL;

	errno = 0;
	result = strtod((char *)buffer, &endstr);

	if(errno != 0)
	{
		AMP_DEBUG_ERR("utils_deserialize_real32","Error deserializing. Errno %d", errno);
		*bytes_used = 0;
		result = 0;
	}
	else
	{
		*bytes_used = endstr - (char *)buffer;
	}

	return result;
}

char*    utils_deserialize_string(uint8_t *buffer, uint32_t bytes_left, uint32_t *bytes_used)
{
	uint32_t blob_len = 0;
	uint8_t *blob = NULL;
	char *result = NULL;

	/* Step 0: Sanity Check. */
	if((buffer == NULL) || (bytes_used == NULL))
	{
		AMP_DEBUG_ERR("utils_deserialize_string","Bad Args.", NULL);
		return NULL;
	}

	/* Step 1: Strings are null-terminated, so calculate length. */
	blob_len = strlen((char*)buffer);

	/* Step 2: Allocate string which is 1 extra character (null terminator). */
	if((result = (char *) STAKE(blob_len + 1)) == NULL)
	{
		AMP_DEBUG_ERR("utils_deserialize_string","Can't allocate %d bytes.", blob_len + 1);
		*bytes_used = 0;
		return NULL;
	}

	memcpy(result, buffer, blob_len);
	result[blob_len] = '\0';

	return result;
}

uint32_t utils_deserialize_uint(uint8_t *buffer, uint32_t bytes_left, uint32_t *bytes_used)
{
	uint32_t result = 0;

	/* Step 0: Sanity check. */
	if((buffer == NULL) || (bytes_used == NULL))
	{
		AMP_DEBUG_ERR("utils_deserialize_uint","Bad Args.", NULL);
		if(bytes_used != NULL)
		{
			*bytes_used = 0;
		}
		return 0;
	}

	// + 1 for length byte.
	if(bytes_left < sizeof(uint32_t))
	{
		AMP_DEBUG_ERR("utils_deserialize_uint","Buffer size %d too small for %d.", bytes_left, sizeof(uint32_t));
		*bytes_used = 0;
		return 0;
	}

	/* Step 1: Populate the uint32_t. */

	result |= buffer[0] << 24;
	result |= buffer[1] << 16;
	result |= buffer[2] << 8;
	result |= buffer[3];

	*bytes_used = 4;

	/* Step 2: Check byte order. */
	result = ntohl(result);

	return result;
}

uvast    utils_deserialize_uvast(uint8_t *buffer, uint32_t bytes_left, uint32_t *bytes_used)
{

	uvast result = 0;

	/* Step 0: Sanity check. */
	if((buffer == NULL) || (bytes_used == NULL))
	{
		AMP_DEBUG_ERR("utils_deserialize_uvast","Bad Args.", NULL);
		if(bytes_used != NULL)
		{
			*bytes_used = 0;
		}
		return 0;
	}

	if(bytes_left < sizeof(uvast))
	{
		AMP_DEBUG_ERR("utils_deserialize_uvast","Buffer size %d too small for %d.", bytes_left, sizeof(uvast));
		*bytes_used = 0;
		return 0;
	}

	/* Step 1: Populate the uvast. */
	uvast tmp;

#if LONG_LONG_OKAY
	tmp = buffer[0]; tmp <<= 56;
	result |= tmp;

	tmp = buffer[1]; tmp <<= 48;
	result |= tmp;

	tmp = buffer[2]; tmp <<= 40;
	result |= tmp;

	tmp = buffer[3]; tmp <<= 32;
	result |= tmp;

	tmp = buffer[4]; tmp <<= 24;
	result |= tmp;

	tmp = buffer[5]; tmp <<= 16;
	result |= tmp;

	tmp = buffer[6]; tmp <<= 8;
	result |= tmp;

	result |= buffer[7];
#else
	tmp = buffer[0]; tmp <<= 24;
	result |= tmp;

	tmp = buffer[1]; tmp <<= 16;
	result |= tmp;

	tmp = buffer[2]; tmp <<= 8;
	result |= tmp;

	result |= buffer[3];
#endif

	*bytes_used = 8;

	/* Step 2: Check byte order. */
	result = ntohv(result);

	return result;
}

vast     utils_deserialize_vast(uint8_t *buffer, uint32_t bytes_left, uint32_t *bytes_used)
{
	return (vast) utils_deserialize_uvast(buffer, bytes_left, bytes_used);
}



uint8_t *utils_serialize_byte(uint8_t value, uint32_t *size)
{
	uint8_t *result = NULL;

	*size = 1;
	if((result = (uint8_t *) STAKE(*size)) == NULL)
	{
		AMP_DEBUG_ERR("utils_serialize_byte","Cannot grab memory.", NULL);
		return NULL;
	}

	result[0] = value;

	return result;
}

uint8_t *utils_serialize_int(int32_t value, uint32_t *size)
{
	return utils_serialize_uint((uint32_t) value, size);
}


/*
 *
 */
uint8_t *utils_serialize_real32(float value, uint32_t *size)
{
	char too_small[1];
	uint8_t *result = NULL;

	*size = snprintf(too_small, sizeof too_small, "%f", value) + 1;

	if((result = STAKE(*size)) == NULL)
	{
		*size = 0;
		return NULL;
	}

	snprintf((char *) result, *size, "%f", value);

	return result;
}

uint8_t *utils_serialize_real64(double value, uint32_t *size)
{
	char too_small[1];
	uint8_t *result = NULL;

	*size = snprintf(too_small, sizeof too_small, "%f", value) + 1;

	if((result = STAKE(*size)) == NULL)
	{
		*size = 0;
		return NULL;
	}

	snprintf((char *)result, *size, "%f", value);

	return result;
}

uint8_t *utils_serialize_string(char *value, uint32_t *size)
{
	uint8_t *result = NULL;

	if((value == NULL) || (size == NULL))
	{
		AMP_DEBUG_ERR("utils_serialize_string","Bad Args", NULL);
		return NULL;
	}

	*size = strlen(value) + 1;
	if((result = (uint8_t *) STAKE(*size)) == NULL)
	{
		AMP_DEBUG_ERR("utils_serialize_uint", "Can't allocate %d bytes", *size);
		return NULL;
	}
	memcpy(result, value, *size);
	return result;
}

uint8_t *utils_serialize_uint(uint32_t value, uint32_t *size)
{
	uint8_t *result = NULL;
	uint32_t tmp;

	if(size == NULL)
	{
		AMP_DEBUG_ERR("utils_serialize_uint", "Bad Args.", NULL);
		return NULL;
	}

	if((result = (uint8_t *) STAKE(sizeof(uint32_t))) == NULL)
	{
		AMP_DEBUG_ERR("utils_serialize_uint", "Can't allocate %d bytes", sizeof(uint32_t));
		return NULL;
	}

	tmp = htonl(value);
	result[0] = (tmp >> 24) & (0xFF);
	result[1] = (tmp >> 16) & (0xFF);
	result[2] = (tmp >> 8) & (0xFF);
	result[3] = tmp & (0xFF);

	*size = 4;

	return result;
}


uint8_t *utils_serialize_uvast(uvast value, uint32_t *size)
{
	uint8_t *result = NULL;
	uvast tmp;

	if(size == NULL)
	{
		AMP_DEBUG_ERR("utils_serialize_uvast", "Bad Args.", NULL);
		return NULL;
	}

	if(sizeof(uvast) != 8)
	{
		AMP_DEBUG_ERR("utils_serialize_uvast","uvast isn't size 8?", NULL);
		return NULL;
	}

	if((result = (uint8_t *) STAKE(sizeof(uvast))) == NULL)
	{
		AMP_DEBUG_ERR("utils_serialize_uvast", "Can't allocate %d bytes", sizeof(uvast));
		return NULL;
	}

	tmp = htonv(value);

#if LONG_LONG_OKAY
	result[0] = (tmp >> 56) & (0xFF);
	result[1] = (tmp >> 48) & (0xFF);
	result[2] = (tmp >> 40) & (0xFF);
	result[3] = (tmp >> 32) & (0xFF);
	result[4] = (tmp >> 24) & (0xFF);
	result[5] = (tmp >> 16) & (0xFF);
	result[6] = (tmp >> 8) & (0xFF);
	result[7] = tmp & (0xFF);
#else
	result[0] = (tmp >> 24) & (0xFF);
	result[1] = (tmp >> 16) & (0xFF);
	result[2] = (tmp >> 8) & (0xFF);
	result[3] = tmp & (0xFF);
#endif

	*size = 8;

	return result;
}


uint8_t *utils_serialize_vast(vast value, uint32_t *size)
{
	return utils_serialize_uvast((uvast) value, size);
}
