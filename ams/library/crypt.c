/*
	default.crypt.c:	stub encryption and decryption functions
				for public distribution of AMS.

	Author: Scott Burleigh, JPL

	Copyright (c) 2005, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
#include "amscommon.h"

void	encryptUsingPublicKey(char *cyphertext, int *ctlen, char *key, int klen,
		char *plaintext, int ptlen)
{
	if (cyphertext == NULL || ctlen == NULL || key == NULL || klen < 0
	|| plaintext == NULL || ptlen < 0)
	{
		return;
	}

	memcpy(cyphertext, plaintext, ptlen);
	*ctlen = ptlen;
}

void	decryptUsingPublicKey(char *plaintext, int *ptlen, char *key, int klen,
		char *cyphertext, int ctlen)
{
	if (plaintext == NULL || ptlen == NULL || key == NULL || klen < 0
	|| cyphertext == NULL || ctlen < 0)
	{
		return;
	}

	memcpy(plaintext, cyphertext, ctlen);
	*ptlen = ctlen;
}

void	encryptUsingPrivateKey(char *cyphertext, int *ctlen, char *key,
		int klen, char *plaintext, int ptlen)
{
	if (cyphertext == NULL || ctlen == NULL || key == NULL || klen < 0
	|| plaintext == NULL || ptlen < 0)
	{
		return;
	}

	memcpy(cyphertext, plaintext, ptlen);
	*ctlen = ptlen;
}

void	decryptUsingPrivateKey(char *plaintext, int *ptlen, char *key, int klen,
		char *cyphertext, int ctlen)
{
	if (plaintext == NULL || ptlen == NULL || key == NULL || klen < 0
	|| cyphertext == NULL || ctlen < 0)
	{
		return;
	}

	memcpy(plaintext, cyphertext, ctlen);
	*ptlen = ctlen;
}

int	encryptUsingSymmetricKey(char **cyphertext, char *key,
		int klen, char *plaintext, int ptlen)
{
	if (cyphertext == NULL || key == NULL || klen < 0
	|| plaintext == NULL || ptlen < 0)
	{
		return 0;
	}

	*cyphertext = plaintext;
	return ptlen;
}

int	decryptUsingSymmetricKey(char **plaintext, char *key,
		int klen, char *cyphertext, int ctlen)
{
	if (plaintext == NULL || key == NULL || klen < 0
	|| cyphertext == NULL || ctlen <  0)
	{
		return 0;
	}

	*plaintext = TAKE_CONTENT_SPACE(ctlen);
	if (*plaintext == NULL)
	{
		return 0;
	}

	memcpy(*plaintext, cyphertext, ctlen);
	return ctlen;
}
