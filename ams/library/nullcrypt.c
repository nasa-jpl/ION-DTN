/*
	nullcrypt.c:	stub encryption and decryption functions
			for public distribution of AMS.

	Author: Scott Burleigh, JPL

	Copyright (c) 2005, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
#include "amscommon.h"

void	encryptUsingPublicKey(char *plaintext, int ptlen, char *key, int klen,
		char *cyphertext, int *ctlen)
{
	if (plaintext == NULL || ptlen < 0 || key == NULL || klen < 0
	|| cyphertext == NULL || ctlen == NULL)
	{
		return;
	}

	memcpy(cyphertext, plaintext, ptlen);
	*ctlen = ptlen;
}

void	decryptUsingPublicKey(char *cyphertext, int ctlen, char *key, int klen,
		char *plaintext, int *ptlen)
{
	if (plaintext == NULL || ptlen == NULL || key == NULL || klen < 0
	|| cyphertext == NULL || ctlen < 0)
	{
		return;
	}

	memcpy(plaintext, cyphertext, ctlen);
	*ptlen = ctlen;
}

void	encryptUsingPrivateKey(char *plaintext, int ptlen, char *key, int klen,
		char *cyphertext, int *ctlen)
{
	if (plaintext == NULL || ptlen < 0 || key == NULL || klen < 0
	|| cyphertext == NULL || ctlen == NULL)
	{
		return;
	}

	memcpy(cyphertext, plaintext, ptlen);
	*ctlen = ptlen;
}

void	decryptUsingPrivateKey(char *cyphertext, int ctlen, char *key, int klen,
		char *plaintext, int *ptlen)
{
	if (plaintext == NULL || ptlen == NULL || key == NULL || klen < 0
	|| cyphertext == NULL || ctlen < 0)
	{
		return;
	}

	memcpy(plaintext, cyphertext, ctlen);
	*ptlen = ctlen;
}

void	encryptUsingSymmetricKey(char *plaintext, int ptlen, char *key,
		int klen, char *cyphertext, int *ctlen)
{
	if (plaintext == NULL || ptlen < 0 || key == NULL || klen < 0
	|| cyphertext == NULL || ctlen == NULL)
	{
		return;
	}

	memcpy(cyphertext, plaintext, ptlen);
	*ctlen = ptlen;
}

void	decryptUsingSymmetricKey(char *cyphertext, int ctlen, char *key,
		int klen, char *plaintext, int *ptlen)
{
	if (plaintext == NULL || ptlen == NULL || key == NULL || klen < 0
	|| cyphertext == NULL || ctlen <  0)
	{
		return;
	}

	memcpy(plaintext, cyphertext, ctlen);
	*ptlen = ctlen;
}
