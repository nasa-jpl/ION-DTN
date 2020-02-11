/*
 *  auth.c:  This is the C module that implements callback functions to handle
 *           LTP Authentication Extension.
 *
 *	Copyright (c) 2014, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *									
 *  Author: TCSASSEMBLER, TopCoder
 *
 *  Modification History:
 *  Date       Who     What
 *  02-04-14    TC      - Added this file
 *  02-19-14    TC      - Fixed several issues to integrate the extension
 *                        assembly.
 */
#include "auth.h"
#include "crypto.h"

#define	SHA1_HASH_LENGTH	(10)

static unsigned char	null_key[20] =
				{0xc3, 0x7b, 0x7e, 0x64, 0x92, 0x58, 0x43, 0x40,
				0xbe, 0xd1, 0x22, 0x07, 0x80, 0x89, 0x41, 0x15,
				0x50, 0x68, 0xf7, 0x38};

static void	getSymmetricKeyForRule(char *keyName, int *keyLength,
			char **keyValue)
{
	*keyLength = 0;
	*keyValue = MTAKE(1);
	oK(sec_get_key(keyName, keyLength, *keyValue));
	MRELEASE(keyValue);
	if (*keyLength <= 0)
	{
		/*	No such key in database, so we can't apply
		 *	this rule.					*/

		writeMemoNote("[!] LTP auth key not found", keyName);
		return;
	}

	*keyValue = MTAKE(*keyLength);
	oK(sec_get_key(keyName, keyLength, *keyValue));
}

static void	getPublicKeyForNode(uvast nodeNbr, int *keyLength,
			char **keyValue)
{
	time_t	epoch = 0;

	*keyLength = 0;
	*keyValue = MTAKE(1);
	sec_get_public_key(nodeNbr, epoch, keyLength,
			(unsigned char *) *keyValue);
	MRELEASE(keyValue);
	if (*keyLength <= 0)
	{
		/*	No such key in database, so we can't apply
		 *	this rule.					*/

		writeMemo("[!] LTP auth public key not found.");
		return;
	}

	*keyValue = MTAKE(*keyLength);
	sec_get_public_key(nodeNbr, epoch, keyLength,
			(unsigned char *) *keyValue);
}

static void	getPrivateKey(int *keyLength, char **keyValue)
{
	time_t	epoch = 0;

	*keyLength = 0;
	*keyValue = MTAKE(1);
	sec_get_private_key(epoch, keyLength, (unsigned char *) *keyValue);
	MRELEASE(keyValue);
	if (*keyLength <= 0)
	{
		/*	No such key in database, so we can't apply
		 *	this rule.					*/

		writeMemo("[!] LTP auth own private key not found.");
		return;
	}

	*keyValue = MTAKE(*keyLength);
	sec_get_private_key(epoch, keyLength, (unsigned char *) *keyValue);
}

static int	verify_sha1(LtpExtensionInbound *trailerExt,
			char *segmentRawData, int keyLength, char *keyValue)
{
	Sdnv		sdnv;
	int		hashOffset;
	unsigned char	hashValue[32];          
	char		authVal[20];

	if (trailerExt->length == 0 || trailerExt->length > sizeof authVal)
	{
		return 0;	/*	Invalid hash in trailer field.	*/
	}

	encodeSdnv(&sdnv, SHA1_HASH_LENGTH);
	hashOffset = trailerExt->offset + 1 + sdnv.length;

	/*	Compute hash over entire segment up to hash value.	*/

	hmac_sha1_sign((unsigned char *) keyValue, keyLength,
			(unsigned char *) segmentRawData, hashOffset,
			hashValue);

	/*	Compare computed hash to hash value in trailer field.	*/

	memcpy(authVal, segmentRawData + hashOffset, trailerExt->length);
	if (memcmp(authVal, hashValue, trailerExt->length) == 0)
	{
		return 1;	/*	Segment has been verified.	*/
	}               

	return 0;
}

static int	verify_sha256(LtpExtensionInbound *trailerExt,
			char *segmentRawData, int keyLength, char *keyValue)
{
	void		*ctx = NULL;
	Sdnv		sdnv;
	int		hashOffset;
	unsigned char	hashValue[32];          
	int		result = 0;

	if (rsa_sha256_verify_init(&ctx, keyValue, keyLength))
	{
		putErrmsg("Can't init sha256 verify context.", NULL);
		return -1;
	}

	encodeSdnv(&sdnv, rsa_sha256_sign_context_length(ctx));
	hashOffset = trailerExt->offset + 1 + sdnv.length;

	/*	Compute hash over entire segment up to hash value.	*/

	sha256_hash((unsigned char*) segmentRawData, hashOffset,
			hashValue, sizeof hashValue);

	/*	Verify computed hash against hash in trailer field.	*/

	if (rsa_sha256_verify(ctx, sizeof hashValue, hashValue, 512,
			(unsigned char *) segmentRawData + hashOffset) == 0)
	{
		result = 1;
	}

	if (ctx)
	{
		MRELEASE(ctx);	//	Does rsa_sha256_verify_init use MTAKE?
	}

	return result;
}

static int	tryAuthHeader(LtpRecvSeg* segment, Lyst trailerExtensions,
			char *segmentRawData, LtpVspan* vspan,
			LtpRecvAuthRule *rule, LtpExtensionInbound *headerExt)
{
	LystElt			trailerElt;
	LtpExtensionInbound	*trailerExt;
	int			result;
	char			*keyValue;
	int			keyLength;

	for (trailerElt = lyst_first(trailerExtensions); trailerElt;
			trailerElt = lyst_next(trailerElt))
	{
		trailerExt = (LtpExtensionInbound *) lyst_data(trailerElt);
		if (trailerExt->tag != 0x00)
		{
			/*	Skip trailer extension fields other
			 *	than authentication extension.		*/

			continue;
		}

		/*	Found an authentication extension trailer.
		 *	Does its value work with this header?		*/

		if (rule->ciphersuiteNbr == 255)	/*	No key.	*/
		{
			keyLength = sizeof null_key;
			keyValue = (char *) null_key;
			result = verify_sha1(trailerExt, segmentRawData,
					keyLength, keyValue);
			if (result == 0)
			{
				continue;
			}

			/*	Either segment is authentic (1) or
			 *	system failure (-1).			*/

			return result;
		}

		/*	Must retrieve key required by rule.		*/

		if (rule->ciphersuiteNbr == 0)
		{
			getSymmetricKeyForRule(rule->keyName, &keyLength,
					&keyValue);
			if (keyLength <= 0)
			{
				/*	No such key in database, so
				 *	we can't verify this segment.	*/

				return 0;
			}

			result = verify_sha1(trailerExt, segmentRawData,
					keyLength, keyValue);
			MRELEASE(keyValue);
			if (result == 0)
			{
				continue;
			}

			/*	Either segment is authentic (1) or
			 *	system failure (-1).			*/

			return result;
		}

		if (rule->ciphersuiteNbr == 1)
		{
			getPublicKeyForNode(vspan->engineId, &keyLength,
					&keyValue);
			if (keyLength <= 0)
			{
				/*	No such key in database, so
				 *	we can't verify this segment.	*/

				return 0;
			}

			result = verify_sha256(trailerExt, segmentRawData,
					keyLength, keyValue);
			MRELEASE(keyValue);
			if (result == 0)
			{
				continue;
			}

			/*	Either segment is authentic (1) or
			 *	system failure (-1).			*/

			return result;
		}
	} 

	return 0;	/*	Not authenticated.			*/
}

int	verifyAuthExtensionField(LtpRecvSeg* segment, Lyst headerExtensions,
		Lyst trailerExtensions, char *segmentRawData, LtpVspan* vspan)
{
	Sdr			sdr = getIonsdr();
				OBJ_POINTER(SecDB, secdb);
	Object			elt;    
	Object			ruleAddr;
				OBJ_POINTER(LtpRecvAuthRule, rule);
	LystElt			headerElt;
	LtpExtensionInbound	*headerExt;     
	int			result;

	if (secAttach() != 0)
	{
		putErrmsg("No ION security.", NULL);
		return -1;
	}

	GET_OBJ_POINTER(sdr, SecDB, secdb, getSecDbObject());
	if (secdb == NULL)	/*	No security database declared.	*/
	{
		putErrmsg("No security database.", NULL);
		return -1;
	}

	/*	Find authentication rule whose remote (sending)
	 *	engine ID matches the sender of this segment.		*/

	CHKERR(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, secdb->ltpRecvAuthRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		ruleAddr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, LtpRecvAuthRule, rule, ruleAddr);      
		if (rule->ltpEngineId != vspan->engineId)  
		{
			continue;
		}       

		/*	Found the matching rule.  Now process each
		 *	LTP authentication extension header field
		 *	in the context of this rule.			*/

		for (headerElt = lyst_first(headerExtensions);
				headerElt; headerElt = lyst_next(headerElt))
		{
			headerExt = (LtpExtensionInbound *)
					lyst_data(headerElt);
			if (headerExt->tag != 0x00 || headerExt->value == NULL
			|| *(headerExt->value) != rule->ciphersuiteNbr)
			{
				/*	Skip header extension fields
				 *	other than authentication
				 *	extension and those for
				 *	ciphersuites other than the
				 *	ciphersuite for this rule.	*/

				continue;
			}

			/*	Found an authentication extension
			 *	header field that fits the rule.
			 *	(Note that we ignore any key name
			 *	that is carried in the extension
			 *	header field; this key name is
			 *	optional, and we only care about
			 *	the key name in the rule.)  Now
			 *	look for a matching trailer field.	*/

			result = tryAuthHeader(segment, trailerExtensions,
					segmentRawData, vspan, rule, headerExt);
			if (result != 0)
			{
				/*	Either segment is authentic
				 *	(1) or system failure (-1).	*/

				sdr_exit_xn(sdr);
				return result;
			}

			/*	No verification; try next header field.	*/
		}

		/*	No authenticating header/trailer pair.		*/

		sdr_exit_xn(sdr);
		return 0;	/*	Segment is not authentic.	*/
	}

	/*	No rule, so we assume the segment is authentic.		*/

	sdr_exit_xn(sdr);
	return 1;
}

int	addAuthHeaderExtensionField(LtpXmitSeg *segment)
{
	Sdr	sdr = getIonsdr();
		OBJ_POINTER(SecDB, secdb);
	Object	elt;
	Object	ruleAddr;
		OBJ_POINTER(LtpXmitAuthRule, rule);
	char	value[1];
	int	result = 0;

	if (secAttach() != 0)
	{
		putErrmsg("No ION security.", NULL);
		return -1;
	}

	GET_OBJ_POINTER(sdr, SecDB, secdb, getSecDbObject());
	if (secdb == NULL)	/*	No security database declared.	*/
	{
		putErrmsg("No security database.", NULL);
		return -1;
	}

	CHKERR(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, secdb->ltpXmitAuthRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		ruleAddr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, LtpXmitAuthRule, rule, ruleAddr);
		if (rule->ltpEngineId != segment->remoteEngineId)
		{
			continue;
		}

		/*	Have rule, must add authentication header
		 *	field.						*/

		value[0] = rule->ciphersuiteNbr;
		if (ltpei_add_xmit_header_extension(segment,
				0x00, sizeof value, value) < 0)
		{
			result = -1;
		}

		break;
	}

	if (sdr_end_xn(sdr) < 0)
	{
		result = -1;
	}

	return result;
}

int	addAuthTrailerExtensionField(LtpXmitSeg *segment)
{
	Sdr	sdr = getIonsdr();
		OBJ_POINTER(SecDB, secdb);
	Object	elt;
	Object	ruleAddr;
		OBJ_POINTER(LtpXmitAuthRule, rule);
	char	*keyValue;
	int	keyLength;
	int	valueLength = 0;
	void	*ctx = NULL;
	char	value[32] = "";      
	int	result = 0;

	if (secAttach() != 0)
	{
		putErrmsg("No ION security.", NULL);
		return -1;
	}

	GET_OBJ_POINTER(sdr, SecDB, secdb, getSecDbObject());
	if (secdb == NULL)	/*	No security database declared.	*/
	{
		putErrmsg("No security database.", NULL);
		return -1;
	}

	CHKERR(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, secdb->ltpXmitAuthRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		ruleAddr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, LtpXmitAuthRule, rule, ruleAddr);
		if (rule->ltpEngineId != segment->remoteEngineId)
		{
			continue;
		}

		/*	Have rule, must add authentication trailer
		 *	field with dummy AuthVal but correct value
		 *	length.						*/

		switch (rule->ciphersuiteNbr)
		{
		case 0:
		case 255:
			valueLength = SHA1_HASH_LENGTH;
			break;

		case 1:
			getSymmetricKeyForRule(rule->keyName, &keyLength,
					&keyValue);
			if (keyLength <= 0)
			{
				break;
			}

			if (rsa_sha256_sign_init(&ctx, keyValue, keyLength))
			{
				putErrmsg("Can't init sha256 sign context.",
						NULL);
			}
			else
			{
				valueLength = rsa_sha256_sign_context_length
						(ctx);
			}

			if (ctx)
			{
				MRELEASE(ctx);	//	MTAKE?
			}

			MRELEASE(keyValue);
			break;

		default:	/*	Ciphersuite is not supported.	*/
			break;	/*	Don't add a trailer field.	*/
		}

		if (valueLength > 0)
		{
			if (ltpei_add_xmit_trailer_extension(segment, 0x00,
					valueLength, value) < 0)
			{
				result = -1;
			}
		}

		break;	/*	Don't look for more rules.	*/
	}

	if (sdr_end_xn(sdr) < 0)
	{
		result = -1;
	}

	return result;
}

static void	sign_using_sha256(char *keyValue, int keyLength, char *buf,
			int lengthToSign, unsigned char *authVal)
{
	void		*ctx = NULL;
	unsigned char	hashValue[32];

	if (rsa_sha256_sign_init(&ctx, keyValue, keyLength))
	{
		putErrmsg("Can't init sha256 sign context.", NULL);
	}
	else
	{
		oK(rsa_sha256_sign_context_length(ctx));

		/*	First calculate the SHA256 hash value.		*/

		sha256_hash((unsigned char *) buf, lengthToSign, hashValue, 32);

		/*	Now sign the hash; signature is the AuthVal.	*/

		if (rsa_sha256_sign(ctx, 32, hashValue, 512, authVal) != 0)
		{
			putErrmsg("Can't sign  hash.", NULL);
		}
	}

	if (ctx)
	{
		MRELEASE(ctx);	//	MTAKE?
	}
}

int	serializeAuthTrailerExtensionField(Object fieldObj, LtpXmitSeg *segment,
		char **cursor)
{
	Sdr		sdr = getIonsdr();
			OBJ_POINTER(SecDB, secdb);
			OBJ_POINTER(LtpExtensionOutbound, field);
	char		*buf;
	int		authLength;
	Sdnv		sdnv;
	int		extensionLength;
	Object		elt;
	Object		ruleAddr;
			OBJ_POINTER(LtpXmitAuthRule, rule);
	char		*keyValue;
	int		keyLength;      
	unsigned char	authVal[512];      

	if (secAttach() != 0)
	{
		putErrmsg("No ION security.", NULL);
		return -1;
	}

	GET_OBJ_POINTER(sdr, SecDB, secdb, getSecDbObject());
	if (secdb == NULL)	/*	No security database declared.	*/
	{
		putErrmsg("No security database.", NULL);
		return -1;
	}

	GET_OBJ_POINTER(sdr, LtpExtensionOutbound, field, fieldObj);
	if (field->tag != 0x00)	/*	Not an authentication trailer.	*/
	{
		return 0;	/*	Do nothing.			*/
	}

	/*	Note: authentication trailer MUST be the last
	 *	extension trailer field of the segment.  It has not
	 *	yet been serialized, but its serialized length has
	 *	already been added to the segment's trailer length.
	 *	So we can point at the start of the segment buffer by
	 *	subtracting the entire length of the segment from the
	 *	buffer offset that we'll be at after this final
	 *	extension field has been serialized into the buffer.	*/

	encodeSdnv(&sdnv, field->length);
	extensionLength = 1 + sdnv.length + field->length;
	buf = (*cursor + extensionLength) - (segment->pdu.headerLength
		+ segment->pdu.contentLength + segment->pdu.trailerLength);
	authLength = (*cursor - buf);

	/*	Begin serialization.					*/

	**cursor = field->tag;
	(*cursor)++;
	authLength++;
	encodeSdnv(&sdnv, field->length);
	memcpy(*cursor, sdnv.text, sdnv.length);
	(*cursor) += sdnv.length;
	authLength += sdnv.length;

	/*	Now retrieve the applicable rule and compute AuthVal.
	 *	(Note: field->value is a dummy value, inserted as a
	 *	placeholder, and is now ignored.)			*/

	memset(authVal, 0, sizeof authVal);	/*	Default.	*/
	CHKERR(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, secdb->ltpXmitAuthRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		ruleAddr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, LtpXmitAuthRule, rule, ruleAddr);
		if (rule->ltpEngineId != segment->remoteEngineId)
		{
			continue;
		}

		/*	Have rule, can compute authentication value
		 *	and serialize the trailer field.		*/

		if (rule->ciphersuiteNbr == 255)	/*	No key.	*/
		{
			hmac_sha1_sign((unsigned char *) null_key,
					sizeof null_key, (unsigned char *) buf,
					authLength, authVal);
			break;			/*	Out of loop.	*/
		}

		if (rule->ciphersuiteNbr == 0)
		{
			getSymmetricKeyForRule(rule->keyName, &keyLength,
					&keyValue);
			if (keyLength > 0)
			{
				hmac_sha1_sign((unsigned char *) keyValue,
						keyLength,
						(unsigned char *) buf,
						authLength, authVal);
				MRELEASE(keyValue);
			}

			break;			/*	Out of loop.	*/
		}

		if (rule->ciphersuiteNbr == 1)
		{
			getPrivateKey(&keyLength, &keyValue);
			if (keyLength > 0)
			{
				sign_using_sha256(keyValue, keyLength, buf,
						authLength, authVal);
				MRELEASE(keyValue);
			}

			break;			/*	Out of loop.	*/
		}

		break;	/*	Don't look for any other rule.		*/
	}

	/*	(Note: if we were unable to compute signature, for any
	 *	reason, a signature of all zeroes - guaranteed to fail
	 *	authentication - is attached to the segment.)		*/

	memcpy(*cursor, authVal, field->length);
	(*cursor) += field->length;
	return 0;
}
