/*
 *	beacon.c -- DTN IP Neighbor Discovery (IPND). Beacon related functions.
 *	These functions enable management of IPND beacons.
 *
 *	Copyright (c) 2015, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *	Author: Gerard Garcia, TrePe
 *	Version 1.0 2015/05/09 Gerard Garcia
 *	Version 2.0 DTN Neighbor Discovery
 *		- ION IPND Implementation Assembly Part2
 *	Version 2.1 DTN Neighbor Discovery - ION IPND Fix Defects and Issues
 */

#include "platform.h"
#include "beacon.h"
#include "bpa.h"
#include "ipndP.h"

/**
 * Frees allocated memory and zeroes out the rest.
 * @param  beacon Beacon to clear.
 */
void	clearBeacon(Beacon *beacon)
{
	LystElt	cur, next;

	if (beacon->services)
	{
		for (cur = lyst_first(beacon->services); cur != NULL;
				cur = next)
		{
			next = lyst_next(cur);
			MRELEASE(((ServiceDefinition*)lyst_data(cur))->data);
		}
	}

	lyst_destroy(beacon->services);
	bloom_free(&beacon->bloom);
	memset(beacon, 0, sizeof(Beacon));
}

/**
 * Makes deep copy
 * @param  dest Beacon to fill.
 * @param  src Beacon to copy from.
 */
void	copyBeacon(Beacon* dest, Beacon* src)
{
	LystElt			cur, next;
	ServiceDefinition	*def, *newDef;

	/*	first clear old beacon	*/

	clearBeacon(dest);

	/*	shallow copy		*/

	memcpy(dest, src, sizeof(Beacon));
	if (src->services)
	{
		dest->services = lyst_create_using(getIonMemoryMgr());
		for (cur = lyst_first(src->services); cur != NULL; cur = next)
		{
			next = lyst_next(cur);
			def = (ServiceDefinition*) lyst_data(cur);
			newDef = (ServiceDefinition*)
					MTAKE(sizeof(ServiceDefinition));
			newDef->dataLength = def->dataLength;
			newDef->number = def->number;
			newDef->data = MTAKE(def->dataLength);
			memcpy(newDef->data, def->data, def->dataLength);
			lyst_insert(dest->services, newDef);
		}
	}

	if (src->bloom.ready)
	{
		dest->bloom.bf = MTAKE(src->bloom.bytes);
		memcpy(dest->bloom.bf, src->bloom.bf, src->bloom.bytes);
	}
}

/**
 * Compares to tag parameters by id.
 * @param  a First tag.
 * @param  b Second tag.
 */
static int	compareTagChildrenById(void *a, void *b)
{
	return ((IpndTagChild*)a)->tag->number
			- ((IpndTagChild*)b)->tag->number;
}

/**
 * Recursivelly creates human readable structure of tag from IPND bytes.
 * @param  tags Tag definitions.
 * @param  isFirst If this tag starts new branch (its name will be output).
 * @param  buffer Buffer to fill with human readable string.
 * @param  len Position in buffer.
 * @param  maxLen Buffer limit.
 * @param  data Processed bytes.
 */
static int	logBeaconTag(IpndTag *tags, char *parentName,
			char *buffer, int *len, int maxLen, unsigned char *data)
{
	unsigned char	id = *data;
	uvast		sdnvTmp;
	IpndTagChild	searchChild;
	LystElt		found;
	int		sdnvLength, readLen, i, ret = 1;
	char*		pName;

	data++;
	if (id != 0 && tags[id].number == 0)
	{
		if (id < 64) /*	we do not have primitive type	*/
		{
			isprintf(buffer + *len, maxLen - *len,
				"undefined primitive type(%d)", id);
			*len = strlen(buffer);
			return -1;
		}

		/*	skip this service	*/

		isprintf(buffer + *len, maxLen - *len, "undefined(%d)", id);
		*len = strlen(buffer);
		sdnvLength = decodeSdnv(&sdnvTmp, data);
		return 1 + sdnvLength + sdnvTmp;
	}

	/*	write name only when this tag starts new branch	*/

	if (parentName == NULL)
	{
		isprintf(buffer + *len, maxLen - *len, "%s:", tags[id].name);
		*len = strlen(buffer);
	}

	if (tags[id].lengthType == 0) // constructed type
	{
		sdnvLength = decodeSdnv(&sdnvTmp, data);
		data += sdnvLength;
		ret += sdnvLength;
		if (lyst_length(tags[id].children) > 1)
		{
			isprintf(buffer + *len, maxLen - *len, "{", NULL);
			*len += 1;
			if (*len >= maxLen) *len = maxLen - 1;
		}

		lyst_compare_set(tags[id].children, compareTagChildrenById);
		for (i = 0; i < lyst_length(tags[id].children); i++)
		{
			if (i > 0)
			{
				isprintf(buffer + *len, maxLen - *len, ",",
						NULL);
				*len += 1;
				if (*len >= maxLen)
				{
					*len = maxLen - 1;
				}
			}

			/*	 search for name of this parameter	*/

			searchChild.tag = &(tags[(unsigned char)(*data)]);
			found = lyst_search(lyst_first(tags[id].children),
					&searchChild);
			if (found == NULL)
			{
				isprintf(buffer + *len, maxLen - *len,
						"invalid tag", NULL);
				*len = strlen(buffer);
				return -1;
			}

			pName = ((IpndTagChild*)lyst_data(found))->name;
			if (lyst_length(tags[id].children) > 1)
			{
				isprintf(buffer + *len, maxLen - *len,
						"%s:", pName);
				*len = strlen(buffer);
			}

			readLen = logBeaconTag(tags, pName, buffer, len,
					maxLen, data);
			if (readLen == -1)
			{
				return -1;
			}

			data += readLen;
			ret += readLen;
		}

		if (lyst_length(tags[id].children) > 1)
		{
			isprintf(buffer + *len, maxLen - *len, "}", NULL);
			*len += 1;
			if (*len >= maxLen) *len = maxLen - 1;
		}
	}
	else	 /*	primitive type	*/
	{
		switch (id)
		{
		#define LBT_CONVERSION_PARAMS data, buffer + *len, maxLen - *len
		case 0:
			ret += bytesToBooleanString(LBT_CONVERSION_PARAMS);
			break;

		case 1:
			ret += bytesToUint64String(LBT_CONVERSION_PARAMS);
			break;

		case 2:
			ret += bytesToSint64String(LBT_CONVERSION_PARAMS);
			break;

		case 3:
			ret += bytesToFixed16String(LBT_CONVERSION_PARAMS);
			break;

		case 4:
			if (parentName && (strcmp(parentName, "IP") == 0))
			{
				ret += bytesIP4ToFixed32String
						(LBT_CONVERSION_PARAMS);
			}
			else
			{
				ret += bytesToFixed32String
						(LBT_CONVERSION_PARAMS);
			}

			break;

		case 5:
			ret += bytesToFixed64String(LBT_CONVERSION_PARAMS);
			break;

		case 6:
			ret += bytesToFloatString(LBT_CONVERSION_PARAMS);
			break;

		case 7:
			ret += bytesToDoubleString(LBT_CONVERSION_PARAMS);
			break;

		case 8:
			ret += bytesToStringString(LBT_CONVERSION_PARAMS);
			break;

		case 9:
			if (parentName && (strcmp(parentName, "IP") == 0))
			{
				ret += bytesIP6ToBytesString
						(LBT_CONVERSION_PARAMS);
			}
			else
			{
				ret += bytesToBytesString
						(LBT_CONVERSION_PARAMS);
			}

			break;
		#undef LBT_CONVERSION_PARAMS
		}

		*len = strlen(buffer);
	}

	return ret;
}

/**
 * Creates human readable structure of service from IPND bytes.
 * @param  buffer Buffer to fill with human readable string.
 * @param  maxLen Buffer limit.
 * @param  services Service definitions to process.
 */
static char	*logBeaconServices(char* buffer, int maxLen, Lyst services)
{
	static int			len;
	static	LystElt			cur, next;
	static	ServiceDefinition	*def;
	static	IPNDCtx			*ctx;

	if (!services)
	{
		istrcpy(buffer, "None", maxLen);
	}
	else
	{
		ctx = getIPNDCtx();
		lockResource(&ctx->configurationLock);
		buffer[0] = '\0';
		len = 0;
		for (cur = lyst_first(services); cur != NULL; cur = next)
		{
			next = lyst_next(cur);
			def = (ServiceDefinition*) lyst_data(cur);
			if (len > 0 && len + 1 < maxLen)
			{
				buffer[len++] = ',';
				buffer[len] = '\0';
			}

			logBeaconTag(ctx->tags, NULL, buffer, &len, maxLen,
					def->data);
			len = strlen(buffer);
		}

		unlockResource(&ctx->configurationLock);
	}

	return buffer;
}

/**
 * Stringifies the contents of a beacon.
 * @param  beacon Beacon to stringify.
 * @return        String representing the contents of the beacon.
 */
char	*logBeacon(Beacon *beacon)
{
	static char	buffer[2048];

	isprintf(buffer,
		sizeof buffer,
		"Version (%d) Flags (%04x) Seq. number (%d) "
		"Source EID (%s) Beacon period (%d) Services",
		beacon->version, beacon->flags, beacon->sequenceNumber,
		beacon->canonicalEid, beacon->period);
#if IPND_DEBUG
	printText(buffer);
#endif
	logBeaconServices(buffer, sizeof buffer, beacon->services);
#if IPND_DEBUG
	printText(buffer);
#endif
	return buffer;
}

/**
 * Checks if the contents of a beacon have changed.
 * @param  beacon Beacon to check
 * @param  period New beacon period
 * @return        0 if it has not changed, 1 if it has changed.
 */
int	beaconChanged(Beacon *beacon, const int period)
{
	IPNDCtx			*ctx = getIPNDCtx();
	LystElt			cur1, next1, cur2, next2;
	ServiceDefinition	*def1, *def2;

	CHKCTX(ctx);
	lockResource(&ctx->configurationLock);

	/* Check if flags have changed */
	if (((beacon->flags & (1 << BEAC_PERIOD_PRESENT)) == 0
			&& ctx->announcePeriod == 1)
	|| ((beacon->flags & (1 << BEAC_PERIOD_PRESENT)) > 0
			&& ctx->announcePeriod == 0))
	{
		unlockResource(&ctx->configurationLock);
		return 1;
	}

	/* Check if Canonical EID changed */
	if (strncasecmp(beacon->canonicalEid, ctx->srcEid, MAX_EID_LEN) != 0)
	{
		unlockResource(&ctx->configurationLock);
		return 1;
	}

	/* Check if services changed */
	if (lyst_length(beacon->services) != lyst_length(ctx->services))
	{
		unlockResource(&ctx->configurationLock);
		return 1;
	}

	if (lyst_length(beacon->services) > 0)
	{
		for (cur1 = lyst_first(beacon->services),
			cur2 = lyst_first(ctx->services);
			cur1 != NULL && cur2 != NULL;
			cur1 = next1, cur2 = next2)
		{
			next1 = lyst_next(cur1);
			next2 = lyst_next(cur2);
			def1 = (ServiceDefinition*)lyst_data(cur1);
			def2 = (ServiceDefinition*)lyst_data(cur2);
			if (memcmp(def1->data, def2->data, def1->dataLength)
					!= 0)
			{
				unlockResource(&ctx->configurationLock);
				return 1;
			}
		}
	}

	/* Check if period changed */
	if (beacon->period != period)
	{
		unlockResource(&ctx->configurationLock);
		return 1;
	}

	unlockResource(&ctx->configurationLock);
	return 0;
}

/**
 * Populates a beacon
 * @param  beacon Beacon to populate.
 * @param  period Beacon period.
 * @return        0 if beacon has been correctly populated.
 */
int	populateBeacon(Beacon *beacon, const int period)
{
	/*
	https://tools.ietf.org/html/draft-irtf-dtnrg-ipnd-02
	["2.6. Beacon Message Format", pages 7 - 14]
	*/
	LystElt			cur, next;
	ServiceDefinition	*def, *defNew;
	IPNDCtx			*ctx = getIPNDCtx();
	uvast			sdnvTmp, tmp;
	int			sdnvLength;

	CHKCTX(ctx);
	lockResource(&ctx->configurationLock);

	/*	clear out beacon	*/

	clearBeacon(beacon);
	beacon->sequenceNumber = 1;

	/*	IPND version	*/

	beacon->version = IPND_VERSION4;

	/*	Announce EID	*/

	if (ctx->announceEid && *ctx->srcEid != '\0')
	{
		beacon->flags |= 1 << BEAC_SOURCE_EID_PRESENT;
		istrcpy(beacon->canonicalEid, ctx->srcEid, MAX_EID_LEN);
	}

	/*	Services.	*/

	ServiceDefinition *defNbfHashes = NULL, *defNbfBits = NULL;
	if (lyst_length(ctx->services))
	{
		beacon->flags |= 1 << BEAC_SERVICE_BLOCK_PRESENT;

		/*	copy services	*/

		beacon->services = lyst_create_using(getIonMemoryMgr());
		for (cur = lyst_first(ctx->services); cur != NULL; cur = next)
		{
			next = lyst_next(cur);
			def = (ServiceDefinition*)lyst_data(cur);

			if (def->number == 126) defNbfHashes = def;
			if (def->number == 127) defNbfBits = def;

			defNew = (ServiceDefinition*)
					MTAKE(sizeof(ServiceDefinition));
			defNew->data = MTAKE(def->dataLength);
			defNew->number = def->number;
			defNew->dataLength = def->dataLength;
			memcpy(defNew->data, def->data, def->dataLength);
			lyst_insert(beacon->services, defNew);
		}

		if (defNbfHashes && defNbfBits)
		{
			/*	beacon contains NBF	*/

			beacon->flags |= 1 << BEAC_NBF_PRESENT;
			bloom_init(&beacon->bloom, NBF_DEFAULT_CAPACITY,
					NBF_DEFAULT_ERROR);
			sdnvLength = decodeSdnv(&sdnvTmp, defNbfBits->data + 1);
			tmp = 1 + sdnvLength + 1;
			sdnvLength = decodeSdnv(&sdnvTmp, defNbfBits->data
					+ tmp);
			/*	copy bits to NBF	*/

			memcpy(beacon->bloom.bf, defNbfBits->data + tmp
					+ sdnvLength, sdnvTmp);
		}
	}

	/*	Beacon period	*/

	if (ctx->announcePeriod && period > 0)
	{
		beacon->flags |= 1 << BEAC_PERIOD_PRESENT;
		beacon->period = period;
	}
	else
	{
		beacon->period = 0;
	}

	unlockResource(&ctx->configurationLock);
	return 0;
}

/**
 * Serializes a beacon
 * @param  beacon    Beacon to serialize.
 * @param  rawBeacon Serialized beacon.
 * @return           Length of serialized beacon.
 */
int	serializeBeacon(Beacon *beacon, unsigned char **rawBeacon)
{
	int			beaconLength;
	Sdnv			flagsSdnv;
	int			seqNumberN;
	int			EIDlength = istrlen(beacon->canonicalEid,
						MAX_EID_LEN);
	Sdnv			EIDLengthSdnv;
	Sdnv			beaconPeriodSdnv;
	Sdnv			svcCountSdnv;
	int			svcCount = lyst_length(beacon->services);
	unsigned char		*cursor;
	LystElt			cur, next;
	ServiceDefinition	*def;

	/*	Calculate beacon length	*/

	/*	First encode sdnv fields	*/

	encodeSdnv(&flagsSdnv, beacon->flags);
	encodeSdnv(&EIDLengthSdnv, EIDlength);
	if (beacon->period >= 0)
	{
		encodeSdnv(&beaconPeriodSdnv, beacon->period);
	}
	else
	{
		beaconPeriodSdnv.length = 0;
	}

	/*	First 3 bytes are the version and the sequence number	*/

	beaconLength = 3 +
		flagsSdnv.length +
		EIDLengthSdnv.length +
		EIDlength +
		beaconPeriodSdnv.length;
	if (svcCount)
	{
		encodeSdnv(&svcCountSdnv, svcCount);
		beaconLength += svcCountSdnv.length;
		for (cur = lyst_first(beacon->services); cur != NULL;
				cur = next)
		{
			next = lyst_next(cur);
			def = (ServiceDefinition*)lyst_data(cur);
			beaconLength += def->dataLength;
		}
	}

	/*	Serialize beacon	*/

	*rawBeacon = MTAKE(beaconLength);
	if (*rawBeacon == NULL)
	{
		putErrmsg("Can't get memory for constructing beacon.", NULL);
		return -1;
	}

	cursor = *rawBeacon;

	*cursor = beacon->version;
	cursor++;

	memcpy(cursor, flagsSdnv.text, flagsSdnv.length);
	cursor += flagsSdnv.length;

	seqNumberN = htons(beacon->sequenceNumber);
	memcpy(cursor, &seqNumberN, 2);
	cursor += 2;

	memcpy(cursor, EIDLengthSdnv.text, EIDLengthSdnv.length);
	cursor += EIDLengthSdnv.length;

	memcpy(cursor, beacon->canonicalEid, EIDlength);
	cursor += EIDlength;

	if (svcCount)
	{
		memcpy(cursor, svcCountSdnv.text, svcCountSdnv.length);
		cursor += svcCountSdnv.length;
		for (cur = lyst_first(beacon->services); cur != NULL;
				cur = next)
		{
			next = lyst_next(cur);
			def = (ServiceDefinition*) lyst_data(cur);
			memcpy(cursor, def->data, def->dataLength);
			cursor += def->dataLength;
		}
	}

	memcpy(cursor, beaconPeriodSdnv.text, beaconPeriodSdnv.length);
	cursor += beaconPeriodSdnv.length;

	return cursor - *rawBeacon;
}

/**
 * Advances a pointer checking if it has arrived at the end.
 * @param  cursor    Pointer to advance.
 * @param  length    Length to advance.
 * @param  cursorEnd Pointer end.
 * @return           0 if pointer has been advanced,
 *                   -1 if pointer has reached the end.
 */
static int	advanceCursor(unsigned char **cursor, const int length,
			const unsigned char *cursorEnd)
{
	unsigned char	*advancedCursor = *cursor + length;

	if (advancedCursor > cursorEnd)
	{
		return -1;
	}
	else
	{
		*cursor = advancedCursor;
		return 0;
	}
}

/**
 * Checks bytes of service definition for inconsistencies with specification
 * @param  tags   Tag definitions.
 * @param  data   Service definition bytes to validate.
 * @param  maxLen How many bytes remain.
 * @return        Real length on success, -1 on error.
 */
static int	validateServiceDefinition(IpndTag *tags, unsigned char *data,
			int maxLen)
{
	IpndTagChild	searchChild;
	uvast		tmp;
	int		i, id, ret, len, sdnvLength, childLen;

	if (maxLen < 2)
	{
		return -1;
	}

	id = *data;

	/*	primitive type	*/

	if (id < 64)
	{
		if (id > 0 && tags[id].number == 0)
		{
			putErrmsg("Unknown primitive type", itoa(id));
			return -1;
		}

		switch (tags[id].lengthType)
		{
			case -2: /*	variable length	*/
				return 1 + decodeSdnv(&tmp, data + 1);

			case -1: /*	explicit	*/
				sdnvLength = decodeSdnv(&tmp, data + 1);
				return 1 + sdnvLength + tmp;

			default: /*	fixed		*/
				return 1 + tags[id].lengthType;
		}
	}

	/*	constructed type (service)			*/

	sdnvLength = decodeSdnv(&tmp, data + 1);
	ret = 1 + sdnvLength + tmp;
	if (ret > maxLen)
	{
		putErrmsg("Invalid length of service", itoa(id));
		return -1;
	}

	if (tags[id].number == 0)
	{
		/*	unknown service	*/
		return ret;
	}

	data += 1 + sdnvLength;
	maxLen -= 1 + sdnvLength;
	len = tmp;

	lyst_compare_set(tags[id].children, compareTagChildrenById);
	for (i = 0; i < lyst_length(tags[id].children); i++)
	{
		/*	search for this parameter	*/

		searchChild.tag = &(tags[(unsigned char)(*data)]);
		if (lyst_length(tags[id].children) == 0
		|| lyst_search(lyst_first(tags[id].children), &searchChild)
				== NULL)
		{
			putErrmsg("Invalid tag for service", itoa(id));
			return -1;
		}

		childLen = validateServiceDefinition(tags, data, len);
		if (childLen == -1)
		{
			return -1;
		}

		data += childLen;
		len -= childLen;
		maxLen -= childLen;
		if (len < 0)
		{
			putErrmsg("Service has greater length then specified",
					itoa(id));
			return -1;
		}
	}

	return ret;
}

#define	BEACON_TRUNCATED_MANDATORY \
	{printText("Beacon truncated in mandatory part."); return -1;}
#define	BEACON_TRUNCATED_OPTIONAL \
	{printText("Beacon truncated in optional part."); ret = -2;}

/**
 * Deserializes a beacon
 * @param  rawBeacon          Beacon to deserialize.
 * @param  rawBeaconLength    Length of the beacon to deserialize.
 * @param  deserializedBeacon Deserialized beacon.
 * @return                    0 on success, -1 on error.
 */
int	deserializeBeacon(unsigned char *rawBeacon, const int rawBeaconLength,
	Beacon *deserializedBeacon)
{
	unsigned char		*cursor = rawBeacon;
	unsigned char		*cursorEnd = cursor + rawBeaconLength;
	uvast			sdnvTmp, tmp;
	int			sdnvLength, eidLength, i, j, ret = 0;
	int			numberOfServices;
	ServiceDefinition	*serviceDefinition;
	ServiceDefinition	*defNbfHashes = NULL, *defNbfBits = NULL;
	char			buffer[100];
	IPNDCtx			*ctx = getIPNDCtx();

	CHKCTX(ctx);
	clearBeacon(deserializedBeacon);
	deserializedBeacon->version = *cursor;
	if (advanceCursor(&cursor, 1, cursorEnd) < 0)
		BEACON_TRUNCATED_MANDATORY

	sdnvLength = decodeSdnv(&sdnvTmp, cursor);
	deserializedBeacon->flags = sdnvTmp;
	if (advanceCursor(&cursor, sdnvLength, cursorEnd) < 0)
		BEACON_TRUNCATED_MANDATORY

	memcpy(&deserializedBeacon->sequenceNumber, cursor, 2);
	deserializedBeacon->sequenceNumber =
			ntohs(deserializedBeacon->sequenceNumber);
	if (advanceCursor(&cursor, 2, cursorEnd) < 0)
		BEACON_TRUNCATED_MANDATORY

	if (deserializedBeacon->flags & (1 << BEAC_SOURCE_EID_PRESENT))
	{
		sdnvLength = decodeSdnv(&sdnvTmp, cursor);
		eidLength = sdnvTmp;
		if (advanceCursor(&cursor, sdnvLength, cursorEnd) < 0)
			BEACON_TRUNCATED_MANDATORY

		if (eidLength >= MAX_EID_LEN)
		{
			printText("Beacon malformed, source EID too long.");
			ret = -2;
		}
		else
		{
			memcpy(&deserializedBeacon->canonicalEid, cursor,
					eidLength);
		}

		if (advanceCursor(&cursor, eidLength, cursorEnd) < 0)
			BEACON_TRUNCATED_MANDATORY
	}

	if (deserializedBeacon->flags & (1 << BEAC_SERVICE_BLOCK_PRESENT))
	{
		sdnvLength = decodeSdnv(&sdnvTmp, cursor);
		if (advanceCursor(&cursor, sdnvLength, cursorEnd) < 0)
			BEACON_TRUNCATED_OPTIONAL

		numberOfServices = sdnvTmp;
		for (i = 0; i < numberOfServices; i++)
		{
			serviceDefinition = (ServiceDefinition*)
					MTAKE(sizeof(ServiceDefinition));
			if (serviceDefinition == NULL)
			{
				putErrmsg("No space for service definition.",
						NULL);
				BEACON_TRUNCATED_OPTIONAL;
				break;
			}

			serviceDefinition->number = *cursor;
			if (advanceCursor(&cursor, 1, cursorEnd) < 0)
			{
				BEACON_TRUNCATED_OPTIONAL;
				break;
			}

			sdnvLength = decodeSdnv(&sdnvTmp, cursor);
			if (advanceCursor(&cursor, sdnvLength, cursorEnd) < 0)
			{
				BEACON_TRUNCATED_OPTIONAL;
				break;
			}

			serviceDefinition->dataLength = 1 + sdnvLength
					+ sdnvTmp;
			/*	include also number and length	*/

			serviceDefinition->data =
					MTAKE(serviceDefinition->dataLength);
			if (serviceDefinition->data == NULL)
			{
				putErrmsg("No space for service def data.",
						NULL);
				BEACON_TRUNCATED_OPTIONAL;
				MRELEASE(serviceDefinition);
				break;
			}

			for (j = 0; j < sdnvLength + 1; j++)
			{
				serviceDefinition->data[j] = *(cursor
						- sdnvLength - 1 + j);
			}

			/*	copy rest of bytes	*/

			for (; j < sdnvLength + 1 + sdnvTmp; j++)
			{
				if (cursor >= cursorEnd)
				{
					BEACON_TRUNCATED_OPTIONAL;
					break;
				}

				serviceDefinition->data[j] = *cursor;
				cursor++;
			}

			if (validateServiceDefinition(ctx->tags,
					serviceDefinition->data,
					serviceDefinition->dataLength) == -1)
			{
				isprintf(buffer, sizeof buffer,
						"[?] Received beacon's service \
information (%s) is malformed.", ctx->tags[*serviceDefinition->data].name);
				printText(buffer);

				BEACON_TRUNCATED_OPTIONAL;
				MRELEASE(serviceDefinition->data);
				MRELEASE(serviceDefinition);
				break;
			}

			if (i == 0)
			{
				deserializedBeacon->services =
					lyst_create_using(getIonMemoryMgr());
				if (deserializedBeacon->services == NULL)
				{
					putErrmsg("No space for services list.",
							NULL);
					BEACON_TRUNCATED_OPTIONAL;
					MRELEASE(serviceDefinition->data);
					MRELEASE(serviceDefinition);
					break;
				}
			}

			lyst_insert(deserializedBeacon->services,
				(void *)serviceDefinition);

			if (serviceDefinition->number == 126)
				defNbfHashes = serviceDefinition;

			if (serviceDefinition->number == 127)
				defNbfBits = serviceDefinition;
		}

		if (defNbfHashes && defNbfBits)
		{
			/*	beacon contains NBF	*/

			bloom_init(&deserializedBeacon->bloom,
				NBF_DEFAULT_CAPACITY, NBF_DEFAULT_ERROR);
			sdnvLength = decodeSdnv(&sdnvTmp, defNbfBits->data + 1);
			tmp = 1 + sdnvLength + 1;
			sdnvLength = decodeSdnv(&sdnvTmp, defNbfBits->data
					+ tmp);
			/*	copy bits to NBF	*/

			memcpy(deserializedBeacon->bloom.bf,
				defNbfBits->data + tmp + sdnvLength, sdnvTmp);
		}
	}

	if (ret == 0
	&& (deserializedBeacon->flags & (1 << BEAC_PERIOD_PRESENT)))
	{
		sdnvLength = decodeSdnv(&sdnvTmp, cursor);
		if (advanceCursor(&cursor, sdnvLength, cursorEnd) < 0)
		{
			BEACON_TRUNCATED_OPTIONAL;
			sdnvTmp = 0;
		}

		deserializedBeacon->period = sdnvTmp;
	}

	return ret;
}
