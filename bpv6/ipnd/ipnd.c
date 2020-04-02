/*
 *	ipnd.c -- DTN IP Neighbor Discovery (IPND). Initializes IPND context,
 *	loads configuration and launches the main threads.
 *
 *	Copyright (c) 2015, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *	Author: Gerard Garcia, TrePe
 *	Version 1.0 2015/05/09 Gerard Garcia
 *	Version 2.0 DTN Neighbor Discovery
 *		- ION IPND Implementation Assembly Part2
 *	Version 2.1 DTN Neighbor Discovery - ION IPND Fix Defects and Issues
 *	Version 2.2 Shared context ctx passed explicitely to threads to avoid shared library security change implications
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "platform.h"
#include "ion.h"
#include "bpP.h"
#include "ipndP.h"
#include "dtn2fw.h"
#include "bpa.h"

#define DEFAULT_EID			""
#define DEFAULT_ANNOUNCE_EID		1
#define DEFAULT_ANNOUNCE_PERIOD		1
#define DEFAULT_PORT			40000
#define DEFAULT_MULTICAST_TTL		255
#define DEFAULT_UNICAST_PERIOD		5
#define DEFAULT_MULTICAST_PERIOD	7
#define DEFAULT_BROADCAST_PERIOD	11

/**
 * Definition of TAGs, handled as lines from config
 */
char tagDefLines[][53] = {
	// Primitive data types
	// TAG# Definition LengthType ContentLength
	"m svcdef 0 boolean fixed	 1",
	"m svcdef 1 uint64	variable",
	"m svcdef 2 sint64	variable",
	"m svcdef 3 fixed16 fixed	 2",
	"m svcdef 4 fixed32 fixed	 4",
	"m svcdef 5 fixed64 fixed	 8",
	"m svcdef 6 float	fixed	 4",
	"m svcdef 7 double	fixed	 8",
	"m svcdef 8 string	explicit",
	"m svcdef 9 bytes	explicit",
	// Constructed data types
	// TAG# Definition ParamName1:ParamType1 ParamName2:ParamType2 ...
	"m svcdef 64  CLA-TCP-v4 IP:fixed32		 Port:fixed16",
	"m svcdef 65  CLA-UDP-v4 IP:fixed32		 Port:fixed16",
	"m svcdef 66  CLA-TCP-v6 IP:bytes		 Port:fixed16",
	"m svcdef 67  CLA-UDP-v6 IP:bytes		 Port:fixed16",
	"m svcdef 68  CLA-TCP-HN Hostname:string Port:fixed16",
	"m svcdef 69  CLA-UDP-HN Hostname:string Port:fixed16",
	"m svcdef 126 NBF-Hashes HashIDs:bytes",
	"m svcdef 127 NBF-Bits	 BitArray:bytes"
};

/**
 * Handles SIGINT and SIGTERM signals
 */
static void	interruptThread(int signum)
{
	isignal(SIGTERM, interruptThread);
	isignal(SIGINT, interruptThread);
	ionKillMainThread("ipnd");
}

/**
 * Handles global IPND context.
 * @param  newCtx New IPNDCtx pointer.
 * @return IPNDCtx context.
 */
static IPNDCtx	*_IPNDCtx(IPNDCtx *newCtx)
{
	static IPNDCtx	*ctx = NULL;

	if (newCtx)
	{
		ctx = newCtx;
	}

	return ctx;
}

/**
 * Gets IPNDCtx context.
 * @return IPND context.
 */
IPNDCtx	*getIPNDCtx()
{
	return _IPNDCtx(NULL);
}

/**
 * Sets IPNDCtx context.
 */
void setIPNDCtx(IPNDCtx *newctx)
{
	 _IPNDCtx(newctx);
}

/**
 * Prints a syntax error.
 * @param lineNbr Line number where the error occurred.
 */
static void	printSyntaxError(int lineNbr)
{
	char	buffer[80];

	isprintf(buffer, sizeof buffer,
		"Syntax error in configuration file (line %d of ipnc.c)",
		lineNbr);
	printText(buffer);
}

/* Macro for printSyntaxError funciton */
#define	SYNTAX_ERROR	printSyntaxError(__LINE__);

/**
 * Prints usage.
 */
static void	printUsage()
{
	PUTS("Valid commands are:");
	PUTS("\t1\tInitialize");
	PUTS("\t   1");
	PUTS("\tm\tManage");
	PUTS("\t   m eid <eid>");
	PUTS("\t   m announce period { 0 | 1 }");
	PUTS("\t   m announce eid { 0 | 1 }");
	PUTS("\t   m interval unicast <interval>");
	PUTS("\t   m interval multicast <interval>");
	PUTS("\t   m interval broadcast <interval>");
	PUTS("\t   m multicast ttl <ttl>");
	PUTS("\t   m svcdef <id> <name> <child1name>:<child1type> ...");
	PUTS("\ta\tAdd");
	PUTS("\t   a listen <listen address>");
	PUTS("\t   a destination <destination address>");
	PUTS("\t   a svcadv <name> <child1name>:<child1value> ...");
	PUTS("\ts\tStart");
	PUTS("\t   s");
	PUTS("\te\tEnable or disable echo of printed output to log file");
	PUTS("\t   e { 0 | 1 }");
	PUTS("\t#\tComment");
	PUTS("\t   # <comment text>");
}

/* Forward declaration */
static int	processLine(char *line, int lineLength);

/**
 * Initializes IPND context and loads its default values.
 * @return 0 on success, -1 on error.
 */
static int	initializeIpnd()
{
	IPNDCtx	*ctx = NULL;
	char	line[1024];

	if (bpAttach() < 0)
	{
		putErrmsg("IPND can't attach to BP.", NULL);
		return -1;
	}

	ctx = MTAKE(sizeof(IPNDCtx));
	CHKCTX(ctx);

	if (initResourceLock(&ctx->configurationLock) < 0)
	{
		putErrmsg("Error initializing IPND context lock.", NULL);
		return -1;
	}

	/* Set default values */
	istrcpy(ctx->srcEid, DEFAULT_EID, MAX_EID_LEN);
	ctx->port = DEFAULT_PORT;
	ctx->announceEid = DEFAULT_ANNOUNCE_EID;
	ctx->multicastTTL = DEFAULT_MULTICAST_TTL;
	ctx->announcePeriod =  DEFAULT_ANNOUNCE_PERIOD;
	ctx->announcePeriods[UNICAST] = DEFAULT_UNICAST_PERIOD;
	ctx->announcePeriods[MULTICAST] = DEFAULT_MULTICAST_PERIOD;
	ctx->announcePeriods[BROADCAST] = DEFAULT_BROADCAST_PERIOD;

	ctx->neighbors = lyst_create_using(getIonMemoryMgr());
	lyst_compare_set(ctx->neighbors, compareIpndNeighbor);
	if (initResourceLock(&ctx->neighborsLock) < 0)
	{
		putErrmsg("Error initializing IPND neighbors lock.", NULL);
		return -1;
	}

	ctx->destinations = lyst_create_using(getIonMemoryMgr());
	lyst_compare_set(ctx->destinations, compareDestination);
	ctx->destinationsCV = llcv_open(ctx->destinations,
			&(ctx->destinationsCV_str));
	if (ctx->destinationsCV == NULL)
	{
		putErrmsg("Error initializing destinations LLCV.", NULL);
		return -1;
	}

	if (bloom_init(&ctx->nbf, NBF_DEFAULT_CAPACITY, NBF_DEFAULT_ERROR) != 0)
	{
		putErrmsg("Error initializing Bloom filter.", NULL);
		return -1;
	}

	setIPNDCtx(ctx);

	/* process tagDefLines */
	int i;

	for (i = 0; i < sizeof(tagDefLines) / sizeof(tagDefLines[0]); i++)
	{
		istrcpy(line, tagDefLines[i], sizeof line);
		processLine(line, sizeof line);
	}

	if (ipnInit() < 0)
	{
		putErrmsg("Can't load IPN routing database.", NULL);
		return -1;
	}

	printText("[i] IPND initialized.");

	return 0;
}

/* Updates ctx->nbf as well as NBF-Bits service in ctx->services if present */
void updateCtxNbf(char* eid, int eidLen)
{
	IPNDCtx	*ctx = getIPNDCtx();

	if (!ctx) return;
	if (bloom_add(&ctx->nbf, eid, eidLen) < 0)
	{
		putErrmsg("Can't add EID to Bloom filter", eid);
	}

	/* find service 127 NBF-Bits and update it with bytes */

	LystElt			cur, next;
	ServiceDefinition	*def;
	uvast			len1, len2;
	int			cnt1, cnt2;
	Sdnv			sdnvTmp1, sdnvTmp2;

	if (lyst_length(ctx->services) == 0)
	{
		return;
	}

	for (cur = lyst_first(ctx->services); cur != NULL; cur = next)
	{
		next = lyst_next(cur);
		def = (ServiceDefinition*) lyst_data(cur);
		if (def->number == 127)
		{
			cnt1 = decodeSdnv(&len1, def->data + 1);
			cnt2 = decodeSdnv(&len2, def->data + 1 + cnt1 + 1);
			if (len2 == ctx->nbf.bytes)
			{
				/* just replace bytes */
				memcpy(def->data + 1 + cnt1 + 1 + cnt2,
						ctx->nbf.bf, len2);
			}
			else
			{
				/* we need to reallocate */
				len2 = ctx->nbf.bytes;
				encodeSdnv(&sdnvTmp2, len2);
				len1 = 1 + sdnvTmp2.length + len2;
				encodeSdnv(&sdnvTmp1, len1);
				MRELEASE(def->data);
				def->dataLength = 1 + sdnvTmp1.length + len1;
				def->data = MTAKE(def->dataLength);
				def->data[0] = 127; /* NBF-Bits */
				def->data[1 + sdnvTmp1.length] = 9;
				memcpy(def->data + 1, sdnvTmp1.text,
						sdnvTmp1.length);
				memcpy(def->data + 1 + sdnvTmp1.length + 1,
						sdnvTmp2.text, sdnvTmp2.length);
				memcpy(def->data + 1 + sdnvTmp1.length + 1
						+ sdnvTmp2.length, ctx->nbf.bf,
						len2);
			}

			return;
		}
	}
}

/**
 * Adds a new IPND destination
 * @param  ip Ip of the new destination.
 * @return 0 on success, -1 on error.
 */
static int	addDestination(char *ip)
{
	char		buffer[80];
	IPNDCtx		*ctx = getIPNDCtx();
	Destination	*dest;
	int		addressType = getIpv4AddressType(ip);

	CHKCTX(ctx);

	if (addressType == -1)
	{
		putErrmsg("Unsupported address.", NULL);
		return -1;
	}

	dest = (Destination *) MTAKE(sizeof(Destination));
	istrcpy(dest->addr.ip, ip, INET_ADDRSTRLEN);
	dest->addr.port = ctx->port;
	*dest->eid = '\0';
	dest->announcePeriod = ctx->announcePeriods[addressType];
	dest->nextAnnounceTimestamp = time(NULL);
	dest->fixed = 1;

	lyst_insert(ctx->destinations, dest);

	if (addressType == BROADCAST)
	{
		ctx->enabledBroadcastSending = 1;
	}

	isprintf(buffer, sizeof buffer,
		"[i] Destination %s:%d added with announcement interval %d.",
		dest->addr.ip, dest->addr.port, dest->announcePeriod);
	printText(buffer);

	return 0;
}

/**
 * Adds a new address where IPND will listen for beacons.
 * @param  address New address.
 * @return 0 on success, -1 on error.
 */
static int	addListen(char *address)
{
	char		buffer[80];
	IPNDCtx		*ctx = getIPNDCtx();
	NetAddress	*newlistenAddress;
	int		addressType = getIpv4AddressType(address);

	CHKCTX(ctx);

	if (addressType == -1)
	{
		putErrmsg("Unsupported address.", NULL);
		return -1;
	}

	if (ctx->listenAddresses == NULL)
	{
		ctx->listenAddresses = lyst_create_using(getIonMemoryMgr());
	}

	newlistenAddress = (NetAddress *)MTAKE(sizeof(NetAddress));
	istrcpy(newlistenAddress->ip, address, INET_ADDRSTRLEN);
	newlistenAddress->port = ctx->port;

	lyst_insert(ctx->listenAddresses, newlistenAddress);

	if (addressType == BROADCAST)
	{
		ctx->enabledBroadcastReceiving = 1;
	}

	isprintf(buffer, sizeof buffer, "[i] Listening on address %s:%d.",
			newlistenAddress->ip, newlistenAddress->port);
	printText(buffer);

	return 0;
}

/**
 * Parses service configure command and adds service definition to ctx->tags
 * @param  tokenCount Number of tokens in tokens.
 * @param  tokens     Command tokens
 * @return 0 on success, -1 on error.
 */
static int	configService(int tokenCount, char** tokens)
{
	IPNDCtx		*ctx = getIPNDCtx();
	int		i, j;
	char		*p;
	IpndTagChild	*tagChild;

	CHKCTX(ctx);

	/* first token contains tag id */

	unsigned char id = (unsigned char) atoi(tokens[0]);

	ctx->tags[id].number = id;

	/* second token contains tag name */

	istrcpy(ctx->tags[id].name, tokens[1], IPND_MAX_TAG_NAME_LENGTH + 1);

	/* third token contains "fixed", "variable", "explicit"
	 * or "param1name:param1type" */

	if (strcmp(tokens[2], "fixed") == 0 && tokenCount == 4)
	{
		ctx->tags[id].lengthType = atoi(tokens[3]);
	}
	else if (strcmp(tokens[2], "variable") == 0 && tokenCount == 3)
	{
		ctx->tags[id].lengthType = -2;
	}
	else if (strcmp(tokens[2], "explicit") == 0 && tokenCount == 3)
	{
		ctx->tags[id].lengthType = -1;
	}
	else if (strchr(tokens[2], ':') != NULL)
	{
		ctx->tags[id].lengthType = 0;

		/* process all "paramXname:paramXtype" */

		for (i = 2; i < tokenCount; i++)
		{
			p = strchr(tokens[i], ':');
			if (p == NULL)
			{
				return -1;
			}

			/* find tag with tag name "paramXtype" */

			for (j = 0; j < 256; j++)
			{
				if (strncmp(ctx->tags[j].name, p + 1,
					IPND_MAX_TAG_NAME_LENGTH) == 0)
				{
					break;
				}
			}

			if (j < 256) /* found such tag */
			{
				if (ctx->tags[id].children == NULL)
				{
					ctx->tags[id].children =
						lyst_create_using
						(getIonMemoryMgr());
				}

				tagChild = (IpndTagChild*)
						MTAKE(sizeof(IpndTagChild));
				tagChild->tag = ctx->tags + j;
				*p = '\0';
				istrcpy(tagChild->name, tokens[i],
						IPND_MAX_TAG_NAME_LENGTH + 1);

				lyst_insert(ctx->tags[id].children,
						(void*) tagChild);
			}
			else // tag name does not exist
			{
				return -1;
			}
		}
	}
	else
	{
		return -1;
	}

	return 0;
}

/**
 * Converts previously assigned human readable strings into IPND protocol bytes
 * @param  tags Definitions of known tags.
 * @param  child    Tag child to convert
 * @param  data     Buffer to fill with bytes
 * @param  dataLen  Current position in buffer filled
 * @param  maxLen   Buffer limit
 * @return read number of bytes, -1 on error.
 */
static int constructServiceDefinition(IpndTag *tags, IpndTagChild *child,
	char *data, int *dataLen, int maxLen)
{
	int		ret = 0;
	int		dataLenBackup = *dataLen;
	static int	len;
	static Sdnv	tmpSdnv;

	/* primitive type */
	if (child->tag->number < 64)
	{
		data[*dataLen] = child->tag->number;
		*dataLen += 1;
		if (*dataLen >= maxLen)
		{
			putErrmsg("Service definition does not fit into beacon",
					NULL);
			return -1;
		}

		switch (child->tag->number)
		{
		#define CSD_CONVERSION_PARAMS child->strVal, data + *dataLen, maxLen - *dataLen
		case 0:
			ret = stringToBooleanBytes(CSD_CONVERSION_PARAMS);
			break;

		case 1:
			ret = stringToUint64Bytes(CSD_CONVERSION_PARAMS);
			break;

		case 2:
			ret = stringToSint64Bytes(CSD_CONVERSION_PARAMS);
			break;

		case 3:
			ret = stringToFixed16Bytes(CSD_CONVERSION_PARAMS);
			break;

		case 4:
			if (strcmp(child->name, "IP") == 0)
			{
				ret = stringIP4ToFixed32Bytes
					(CSD_CONVERSION_PARAMS);
			}
			else
			{
				ret = stringToFixed32Bytes
					(CSD_CONVERSION_PARAMS);
			}

			break;

		case 5:
			ret = stringToFixed64Bytes(CSD_CONVERSION_PARAMS);
			break;

		case 6:
			ret = stringToFloatBytes(CSD_CONVERSION_PARAMS);
			break;

		case 7:
			ret = stringToDoubleBytes(CSD_CONVERSION_PARAMS);
			break;

		case 8:
			ret = stringToStringBytes(CSD_CONVERSION_PARAMS);
			break;

		case 9:
			if (strcmp(child->name, "IP") == 0)
			{
				ret = stringIP6ToBytesBytes
					(CSD_CONVERSION_PARAMS);
			}
			else
			{
				ret = stringToBytesBytes
					(CSD_CONVERSION_PARAMS);
			}
			break;

		default:
			putErrmsg("Unsupported primitive type", NULL);
			return -1;
		#undef CSD_CONVERSION_PARAMS
		}

		if (ret < 0)
		{
			putErrmsg("Primitive type conversion failed", NULL);
			return -1;
		}

		*dataLen += ret;
		return ret + 1;
	}

	/* constructed type */

	LystElt	cur, next;

	/* first byte is tag id
	   next bytes are SDNV length. we assume 1 byte,
	   we will shift the data later if not. */

	data[*dataLen] = child->tag->number;
	*dataLen += 2;
	if (*dataLen >= maxLen)
	{
		putErrmsg("Service definition does not fit into beacon", NULL);
		return -1;
	}

	/* recursively fill children */
	if (lyst_length(tags[child->tag->number].children) > 0)
	{
		for (cur = lyst_first(tags[child->tag->number].children);
				cur != NULL; cur = next)
		{
			next = lyst_next(cur);
			child = (IpndTagChild*) lyst_data(cur);
			len = constructServiceDefinition(tags, child, data,
					dataLen, maxLen);
			if (len == -1)
			{
				putErrmsg("Service definition does not fit \
into beacon", NULL);
				return -1;
			}

			ret += len;
		}
	}

	/* determine real length */
	encodeSdnv(&tmpSdnv, ret);
	if (tmpSdnv.length == 1)
	{
		/* just one byte as assumed */
		data[dataLenBackup + 1] = tmpSdnv.text[0];
		ret += 2;
	}
	else
	{
		/* we need to shift everything tmpSdnv.length - 1 bytes right */
		*dataLen += tmpSdnv.length - 1;
		if (*dataLen >= maxLen)
		{
			putErrmsg("Service definition does not fit into beacon",
					NULL);
			return -1;
		}

		/* memmove() copies from back in this case */
		memmove(data + dataLenBackup + 2 + tmpSdnv.length - 1,
			data + dataLenBackup + 2, ret);

		/* write correct length */
		memcpy(data + dataLenBackup + 1, tmpSdnv.text, tmpSdnv.length);
		ret += tmpSdnv.length + 1;
	}

	return ret;
}

/**
 * Parses service add command and adds advertized service to ctx->services
 * @param  tokenCount Number of tokens in tokens.
 * @param  tokens     Command tokens
 * @return 0 on success, -1 on error.
 */
static int	addService(int tokenCount, char** tokens)
{
	IPNDCtx		*ctx = getIPNDCtx();
	int		i, id, curId;
	char		*pFrom, *pTo;
	IpndTagChild	*tagChild = 0;
	LystElt		cur, next;

	CHKCTX(ctx);

	/* first token contains service name */
	for (id = 0; id < 256; id++)
	{
		if (strncmp(ctx->tags[id].name, tokens[0],
				IPND_MAX_TAG_NAME_LENGTH) == 0)
		{
			break;
		}
	}

	if (id >= 256)
	{
		putErrmsg("Unknown service %s.", tokens[0]);
		return -1;
	}

	/* rest of tokens contain "child1name1:child1name2:child1value" */
	for (i = 1; i < tokenCount; i++)
	{
		if ((pTo = strchr(tokens[i], ':')) != NULL)
		{
			curId = id;
			pFrom = tokens[i];
			/* go down tag tree to find the leaf to set value */
			do
			{
				*pTo = '\0';
				for (cur = lyst_first
						(ctx->tags[curId].children);
						cur != NULL; cur = next)
				{
					next = lyst_next(cur);
					tagChild = (IpndTagChild *)
						lyst_data(cur);
					if (strncmp(pFrom, tagChild->name,
						IPND_MAX_TAG_NAME_LENGTH) == 0)
					{
						break;
					}
				}

				*pTo = ':';
				if (cur == NULL)
				{
					/* this is not child name */
					if (curId == id)
					{
						/* still at top level */

						putErrmsg("Bad param name %s.",
								pFrom);
						return -1;
					}

					// it can be param value with ':' in it
					break;
				}

				pFrom = pTo + 1;
				curId = tagChild->tag->number;
			} while ((pTo = strchr(pFrom, ':')) != NULL);

			/* we need to go further down the tree of
			 * single children				*/

			while (lyst_length(ctx->tags[curId].children) == 1)
			{
				tagChild = lyst_data
					(lyst_first(ctx->tags[curId].children));
				curId = tagChild->tag->number;
			}

			if (lyst_length(ctx->tags[curId].children) != 0)
			{
				putErrmsg("Param %s has more than one child.",
						ctx->tags[curId].name);
				return -1;
			}

			/* we have tagChild to set its string value
			 * pFrom contains that value from config line */
			tagChild->strVal = pFrom;
		}
		else
		{
			putErrmsg("Wrong param format %s.", tokens[i]);
			return -1;
		}
	}

	/* service is parsed out, the tagChild->strVals are filled out
	 * now we need to create service definition, i.e. bytes */

	/* create temporary buffer with max size possible */

	char		*data = MTAKE(MAX_BEACON_SIZE);

	/* construct the buffer and get back its length */

	int		len = 0;
	IpndTagChild	dummyChild;

	dummyChild.tag = ctx->tags + id;
	if (constructServiceDefinition(ctx->tags, &dummyChild, data, &len,
			MAX_BEACON_SIZE) < 0)
	{
		MRELEASE(data);
		putErrmsg("Cannot construct service bytes of %s\n", tokens[0]);
		return -1;
	}

	/* construct service definition and copy data from temp buffer */
	ServiceDefinition	*def =
		(ServiceDefinition*) MTAKE(sizeof(ServiceDefinition));

	def->number = id;
	def->dataLength = len;
	def->data = MTAKE(len);
	memcpy((char *)def->data, (char *)data, len);

	/* release temp buffer */
	MRELEASE(data);

	/* insert service definition into list of advertized services */
	if (!ctx->services)
	{
		ctx->services = lyst_create_using(getIonMemoryMgr());
	}

	lyst_insert(ctx->services, def);

	/* NBF-Hashes (126) implies NBF-Bits init. with bloom filter bits */

	uvast	len1, len2;
	Sdnv	sdnvTmp1, sdnvTmp2;

	if (id == 126)
	{
		def = (ServiceDefinition*) MTAKE(sizeof(ServiceDefinition));
		def->number = 127;

		/* copy bytes from ctx->nbf */

		len2 = ctx->nbf.bytes;
		encodeSdnv(&sdnvTmp2, len2);
		len1 = 1 + sdnvTmp2.length + len2;
		encodeSdnv(&sdnvTmp1, len1);
		def->dataLength = 1 + sdnvTmp1.length + len1;
		def->data = MTAKE(def->dataLength);
		def->data[0] = 127; /* NBF-Bits */
		def->data[1 + sdnvTmp1.length] = 9; // byte array
		memcpy(def->data + 1, sdnvTmp1.text, sdnvTmp1.length);
		memcpy(def->data + 1 + sdnvTmp1.length + 1,
				sdnvTmp2.text, sdnvTmp2.length);
		memcpy(def->data + 1 + sdnvTmp1.length + 1
				+ sdnvTmp2.length, ctx->nbf.bf, len2);

		lyst_insert(ctx->services, def);
	}

	return 0;
}

#define TOKENCHK(n) if (tokenCount != n) {SYNTAX_ERROR; return -1;}

/**
 * Parses and add command
 * @param  tokenCount Number of tokens in tokens
 * @param  tokens     Command tokens
 * @return 0 on success, -1 on error.
 */
static int	executeAdd(int tokenCount, char **tokens)
{
	char	buffer[80];

	if (tokenCount < 2)
	{
		printText("Add what?");
		return -1;
	}

	if (strcmp(tokens[1], "destination") == 0)
	{
		TOKENCHK(3);

		return addDestination(tokens[2]);
	}

	if (strcmp(tokens[1], "listen") == 0)
	{
		TOKENCHK(3);

		return addListen(tokens[2]);
	}

	if (strcmp(tokens[1], "svcadv") == 0 && tokenCount >= 3)
	{
		if (addService(tokenCount - 2, tokens + 2) == 0)
		{
			isprintf(buffer, sizeof buffer,
				"[i] Service %s will be advertised.",
				tokens[2]);
			printText(buffer);
			return 0;
		}
	}

	SYNTAX_ERROR;
	return -1;
}

/**
 * Parses a configure command
 * @param  tokenCount Number of tokens in tokens.
 * @param  tokens     Command tokens
 * @return 0 on success, -1 on error.
 */
static int	executeConfigure(int tokenCount, char **tokens)
{
	char	buffer[80], *p;
	long	intervalValue;
	IPNDCtx	*ctx = getIPNDCtx();

	if (tokenCount < 2)
	{
		printText("Configure what?");
		return -1;
	}

	if (strcmp(tokens[1], "eid") == 0)
	{
		/* this check eliminates possibility of having white
		 * space in eid						*/

		if (tokenCount != 3)
		{
			printText("Specified EID is empty or contains white \
space.");
			return -1;
		}

		istrcpy(ctx->srcEid, tokens[2], MAX_EID_LEN);

		/* we need exactly one colon */
		p = strchr(ctx->srcEid, ':');
		if (p == NULL || strchr(p + 1, ':') != NULL)
		{
			printText("Specified EID must contain exactly one \
colon (:)");
			return -1;
		}

		/* only valid ASCII values */
		for (p = ctx->srcEid; *p; ++p)
		{
			if (*p <= 32 || *p >= 127) break;
		}

		if (*p)
		{
			printText("Specified EID must contain only valid \
ASCII values");
			return -1;
		}

		isprintf(buffer, sizeof buffer, "[i] Eid: %s.", ctx->srcEid);
		printText(buffer);

		return 0;
	}

	if (strcmp(tokens[1], "port") == 0)
	{
		TOKENCHK(3);

		ctx->port = strtol(tokens[2], NULL, 10);
		isprintf(buffer, sizeof buffer, "[i] Port: %d.", ctx->port);
		printText(buffer);

		return 0;
	}

	if (strcmp(tokens[1], "announce") == 0)
	{
		TOKENCHK(4);

		if (strcmp(tokens[2], "period") == 0)
		{
			ctx->announcePeriod = strtol(tokens[3], NULL, 10);

			printText("[i] Period announced.");

			return 0;
		}
		else if (strcmp(tokens[2], "eid") == 0)
		{
			ctx->announceEid = strtol(tokens[3], NULL, 10);

			printText("[i] Eid announced.");

			return 0;
		}

		SYNTAX_ERROR;
		return -1;
	}

	if (strcmp(tokens[1], "interval") == 0)
	{
		TOKENCHK(4);

		/* interval must be non-negative integer without
		 * extra characters, e.g. "3s" is invalid		*/

		p = buffer;
		intervalValue = strtol(tokens[3], &p, 10);
		if (*p != '\0' || intervalValue < 0)
		{
			isprintf(buffer, sizeof buffer, "Invalid interval \
(beacon period) value: %s", tokens[3]);
			printText(buffer);
			return -1;
		}

		if (strcmp(tokens[2], "unicast") == 0)
		{
			ctx->announcePeriods[UNICAST] = intervalValue;

			isprintf(buffer, sizeof buffer,
					"[i] Unicast announce interval: %d.",
					ctx->announcePeriods[UNICAST]);
			printText(buffer);

			return 0;
		}
		else if (strcmp(tokens[2], "multicast") == 0)
		{
			ctx->announcePeriods[MULTICAST] = intervalValue;

			isprintf(buffer, sizeof buffer,
					"[i] Multicast announce interval: %d.",
					ctx->announcePeriods[MULTICAST]);
			printText(buffer);

			return 0;
		}
		else if (strcmp(tokens[2], "broadcast") == 0)
		{
			ctx->announcePeriods[BROADCAST] = intervalValue;

			isprintf(buffer, sizeof buffer,
					"[i] Broadcast announce interval: %d.",
					ctx->announcePeriods[BROADCAST]);
			printText(buffer);

			return 0;
		}

		SYNTAX_ERROR;
		return -1;
	}

	if (strcmp(tokens[1], "multicast") == 0)
	{
		TOKENCHK(4);

		if (strcmp(tokens[2], "ttl") == 0)
		{
			ctx->multicastTTL = strtol(tokens[3], NULL, 10);
			isprintf(buffer, sizeof buffer,
				"[i] Multicast TTL: %d.", ctx->multicastTTL);
			printText(buffer);

			return 0;
		}

		SYNTAX_ERROR;
		return -1;
	}

	if (strcmp(tokens[1], "svcdef") == 0)
	{
		if (tokenCount >= 5
		&& configService(tokenCount - 2, tokens + 2) == 0)
		{
			isprintf(buffer, sizeof buffer,
				"[i] Service definition: %s %s.",
				tokens[2], tokens[3]);
			printText(buffer);
			return 0;
		}

		SYNTAX_ERROR;
		return -1;
	}

	SYNTAX_ERROR;
	return -1;
}

/**
 * Processes a command line from the config file.
 * @param  line       Line.
 * @param  lineLength Line length.
 * @return 0 on success, -1 on error.
 */
static int	processLine(char *line, int lineLength)
{
	IPNDCtx		*ctx = NULL;
	int		tokenCount;
	char		*cursor;
	int		i;
	const int	MAX_TOKENS = 99;
	char		*tokens[MAX_TOKENS];

	tokenCount = 0;
	for (cursor = line, i = 0; i < MAX_TOKENS; i++)
	{
		if (*cursor == '\0')
		{
			tokens[i] = NULL;
		}
		else
		{
			findToken(&cursor, &(tokens[i]));
			tokenCount++;
		}
	}

	if (tokenCount == 0 || tokens[0] == NULL)
	{
		return 0;
	}

	/*	Skip over any trailing whitespace.			*/

	while (isspace((int) *cursor))
	{
		cursor++;
	}

	/*	Make sure we've parsed everything.			*/

	if (*cursor != '\0')
	{
		printText("Too many tokens.");
		return 0;
	}

	/*	Have parsed the command.  Now execute it.		*/

	switch (*(tokens[0]))		/*	Command code.		*/
	{
	case 0:				/*	Empty line.		*/
	case '#':			/*	Comment.		*/
		break;

	case '1':
		/* Initialize command */
		return initializeIpnd();

	case 's':
		/* Start command */
		ctx = getIPNDCtx();

		if (ctx->srcEid[0] == '\0')
		{
			printText("No valid EID specified in config file.");
			return -1;
		}

		if (pthread_begin(&ctx->sendBeaconsThread, NULL, sendBeacons,
				ctx, "sendBeacons"))
		{
			putSysErrmsg("IPND can't start sendBeacons thread.",
					NULL);
			return -1;
		}

		ctx->haveSendThread = 1;
		if (pthread_begin(&ctx->receiveBeaconsThread, NULL,
				receiveBeacons, ctx, "receiveBeacons"))
		{
			putSysErrmsg("IPND can't start receiveBeacons thread.",
					NULL);
			return -1;
		}

		ctx->haveReceiveThread = 1;
		if (pthread_begin(&ctx->expireNeighborsThread, NULL,
				expireNeighbors, ctx, "expireNeighbors"))
		{
			putSysErrmsg("IPND can't start expireNeighbors thread.",
					NULL);
			return -1;
		}

		ctx->haveExpireThread = 1;
		return 0;

	case 'a':
		if (executeAdd(tokenCount, tokens) == -1)
		{
			return -1;
		}

		break;

	case 'm':
		if (executeConfigure(tokenCount, tokens) == -1)
		{
			return -1;
		}

		break;

	case 'e':
		switchEcho(tokenCount, tokens);
		break;

	default:
		printText("Invalid command.  Enter '?' for help.");
		return -1;
	}

	return 0;
}

#if defined (ION_LWT)
int	ipndadmin(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	*cmdFileName = (char *) a1;
#else
int	main(int argc, char *argv[])
{
	char	*cmdFileName = (argc > 1 ? argv[1] : NULL);
#endif
	int	cmdFile, len, processLineResult;
	char	line[1024];
	IPNDCtx	*ctx = NULL;
	int	i;

#if 0
	/* Block SIGUSR1 signals */
	iblock(SIGUSR1);	/*	Why is this necessary?		*/
#endif

	if (cmdFileName == NULL)
	{
		putErrmsg("No IPND configuration file provided.", NULL);
		printUsage();
		return 1;
	}

	cmdFile = iopen(cmdFileName, O_RDONLY, 0777);
	if (cmdFile < 0)
	{
		putErrmsg("Can't open configuration file.", NULL);
		return 1;
	}
	else
	{
		while (1)
		{
			if (igets(cmdFile, line, sizeof line, &len) == NULL)
			{
				if (len == 0)
				{
					break;	/*	Loop.		*/
				}

				putErrmsg("igets failed.", NULL);
				break;		/*	Loop.		*/
			}

			if (len == 0 || line[0] == '#')	/*	Comment.*/
			{
				continue;
			}

			processLineResult = processLine(line, len);
			if (processLineResult < 0)
			{
				putErrmsg("Error processing config file.",
						NULL);
				return 1;
			}
		}

		close(cmdFile);
	}

	ctx = getIPNDCtx();

	if (ctx == NULL || ctx->haveSendThread == 0)
	{
		printText("Configuration script did not issue initialize or \
start command.");
		return 1;
	}

	/* Notify ION that IPND is running*/
	ionNoteMainThread("ipnd");

	/*	Set up signal handling	*/
	isignal(SIGTERM, interruptThread);
	isignal(SIGINT, interruptThread);

	/* Sleep until interrupted by SIGTERM or SIGINT. */
	ionPauseMainThread(-1);

	printText("[i] IPND shutting down.");

	/* Shutdown */
	if (ctx->haveSendThread)
	{
		pthread_end(ctx->sendBeaconsThread);
		pthread_join(ctx->sendBeaconsThread, NULL);
	}

	if (ctx->haveReceiveThread)
	{
		pthread_end(ctx->receiveBeaconsThread);
		pthread_join(ctx->receiveBeaconsThread, NULL);
	}

	if (ctx->haveExpireThread)
	{
		pthread_end(ctx->expireNeighborsThread);
		pthread_join(ctx->expireNeighborsThread, NULL);
	}

	/* Free up resources */
	releaseLystElements(ctx->destinations);
	releaseLystElements(ctx->neighbors);
	releaseLystElements(ctx->listenAddresses);
	MRELEASE(ctx);

	/* Close all listening sockets */

	for (i = 0; i < ctx->numListenSockets; i++)
	{
		close(ctx->listenSockets[i]);
	}

	return 0;
}
