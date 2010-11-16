/*
	amscommon.c:	common functions used by libams, amsd,
			and the transport service adapters.

	Author: Scott Burleigh, JPL

	Copyright (c) 2005, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
#include "amscommon.h"

#define EPOCH_OFFSET_1958	(378691200)
#define	MAX_GW_EID		(255)

AmsMib			*mib = NULL;

char			*rejectionMemos[] =	{
				"not rejected",
				"duplicate registrar",
				"no cell census",
				"cell is full",
				"no such unit"
						};

extern void		destroyAmsEndpoint(LystElt elt, void *userdata);

/*	*	*	Globals used by AMS services.	*	*	*/

char			*BadParmsMemo = "AMS app error: bad input parms(s).";
char			*NoMemoryMemo = "AMS failure: out of memory.";
int			MaxContinNbr = MAX_CONTIN_NBR;
int			MaxVentureNbr = MAX_VENTURE_NBR;
int			MaxUnitNbr = MAX_UNIT_NBR;
int			MaxModuleNbr = MAX_NODE_NBR;
int			MaxRoleNbr = MAX_ROLE_NBR;
int			MaxSubjNbr = MAX_SUBJ_NBR;

/*	*	*	Memory management globals.	*	*	*/

static PsmPartition	amspartition = NULL;
int			amsMemory = -1;	/*	Memory manager.		*/
MemAllocator		amsmtake;
MemDeallocator		amsmrelease;
MemAtoPConverter	amsmatop;
MemPtoAConverter	amsmptoa;

/*	*	*	Transport service globals.	*	*	*/

#ifdef UDPTS
extern void		udptsLoadTs(TransSvc *ts);
#endif
#ifdef DGRTS
extern void		dgrtsLoadTs(TransSvc *ts);
#endif
#ifdef VMQTS
extern void		vmqtsLoadTs(TransSvc *ts);
#endif
#ifdef TCPTS
extern void		tcptsLoadTs(TransSvc *ts);
#endif

/*	Note: transport service loaders in this table appear in
 *	descending order of preference.  "Preference" corresponds
 *	broadly to nominal throughput rate.				*/

static TsLoadFn		transportServiceLoaders[] =
			{
#ifdef UDPTS
				udptsLoadTs,
#endif
#ifdef DGRTS
				dgrtsLoadTs,
#endif
#ifdef VMQTS
				vmqtsLoadTs,
#endif
#ifdef TCPTS
				tcptsLoadTs
#endif
			};

static int		transportServiceCount = sizeof transportServiceLoaders /
					sizeof(TsLoadFn);

unsigned short	computeAmsChecksum(unsigned char *cursor, int pduLength)
{
	unsigned int	sum = 0;
	unsigned short	addend;

	while (pduLength > 0)
	{
		addend = *cursor;
		addend <<= 8;	/*	Low-order byte is now zero pad.	*/
		cursor++;
		pduLength--;
		if (pduLength > 0)	/*	Replace pad with byte.	*/
		{
			addend += *cursor;
			cursor++;
			pduLength--;
		}

		sum += addend;		/*	Okay if it overflows.	*/
	}

	return sum & 0x0000ffff;
}

/*	*	*	Memory management functions	*	*	*/

static void	*amsmtake_fn(char *file, int line, size_t length)
{
	void	*block;

	block = psp(amspartition, Psm_zalloc(file, line, amspartition, length));
	if (block)
	{
		memset((char *) block, 0, length);
	}

	return block;
}

static void	amsmrelease_fn(char *file, int line, void *block)
{
	if (block)
	{
		Psm_free(file, line, amspartition, psa(amspartition, block));
	}
}

static void	*amsmatop_fn(unsigned long address)
{
	return (psp(amspartition, address));
}

static unsigned long amsmptoa_fn(void *pointer)
{
	return (psa(amspartition, pointer));
}

static int	initAmsPsm(char *name, char *allocation, unsigned size)
{
	PsmMgtOutcome	outcome;

	if (sm_ipc_init() < 0)
	{
		return -1;
	}

	if (allocation == NULL)
	{
		allocation = calloc(1, size);
		if (allocation == NULL)
		{
			return -1;
		}
	}

	if (psm_manage(allocation, size, name, &amspartition, &outcome) < 0
	|| outcome != Okay)
	{
		amspartition = NULL;
		return -1;	/*	PSM partition unmanageable.	*/
	}

	return memmgr_add(name, amsmtake_fn, amsmrelease_fn, amsmatop_fn,
			amsmptoa_fn);
}

int	initMemoryMgt(char *mName, char *memory, unsigned mSize)
{
	int	mmid;

	/*	Configure memory management as necessary.		*/

	if (mName == NULL)
	{
		mmid = 0;	/*	Default to malloc/free.		*/
	}
	else
	{
		mmid = memmgr_find(mName);
	}

	if (amsMemory < 0)	/*	memmgr not selected yet.	*/
	{
		if (mmid < 0)	/*	No such memmgr yet.		*/
		{
			mmid = initAmsPsm(mName, memory, mSize);
			if (mmid < 0)
			{
				putErrmsg("AMS can't initialize DRAM mgr",
						mName);
				return -1;
			}
		}

		/*	Use the selected (possibly new) memmgr.		*/

		amsMemory = mmid;
		amsmtake = memmgr_take(amsMemory);
		amsmrelease = memmgr_release(amsMemory);
		amsmatop = memmgr_AtoP(amsMemory);
		amsmptoa = memmgr_PtoA(amsMemory);
	}
	else	/*	Ensure no AMS memory manager conflict.		*/
	{
		if (mmid != amsMemory)
		{
			putErrmsg("Memory manager selections conflict.", mName);
			errno = EINVAL;
			return -1;
		}
	}

	return 0;
}

/*	*	*	Conditions	*	*	*	*	*/

int	time_to_stop(Llcv llcv)
{
	return 1;
}

int     llcv_reply_received(Llcv llcv)
{
	return (lyst_compare_get(llcv->list) == NULL ? 1 : 0);
}

/*	*	*	MIB lookup functions	*	*	*	*/

LystElt	findApplication(char *appName)
{
	LystElt	elt;
	AmsApp	*app;

	for (elt = lyst_first(mib->applications); elt; elt = lyst_next(elt))
	{
		app = (AmsApp *) lyst_data(elt);
		if (strcmp(app->name, appName) == 0)
		{
			return elt;
		}
	}

	return NULL;
}

static int	hashSubjectName(char *name)
{
	/*	Hash function adapted from Dr. Dobbs, April 1996.	*/

	int		length = strlen(name);
	int		i = 0;
	unsigned int	h = 0;
	unsigned int	g = 0;

	for (i = 0; i < length; i++, name++)
	{
		h = (h << 4) + *name;
		g = h & 0xf0000000;
		if (g)
		{
			h ^= g >> 24;
		}

		h &= ~g;
	}
	
	return h % SUBJ_LIST_CT;
}

Subject *lookUpSubject(Venture *venture, char *subjectName)
{
	int	idx;
	LystElt	elt;
	Subject	*subject;

	idx = hashSubjectName(subjectName);
	for (elt = lyst_first(venture->subjLysts[idx]); elt;
			elt = lyst_next(elt))
	{
		subject = (Subject *) lyst_data(elt);
		if (strcmp(subject->name, subjectName) == 0)
		{
			return subject;
		}
	}

	return NULL;
}

AppRole	*lookUpRole(Venture *venture, char *roleName)
{
	int	i;
	AppRole	*role;

	for (i = 1; i <= MaxRoleNbr; i++)
	{
		role = venture->roles[i];
		if (role == NULL)
		{
			continue;
		}

		if (strcmp(role->name, roleName) == 0)
		{
			return role;
		}
	}

	return NULL;
}

LystElt	findElement(Subject *subject, char *elementName)
{
	LystElt		elt;
	MsgElement	*element;

	for (elt = lyst_first(subject->elements); elt; elt = lyst_next(elt))
	{
		element = (MsgElement *) lyst_data(elt);
		if (strcmp(element->name, elementName) == 0)
		{
			return elt;
		}
	}

	return NULL;
}

Venture	*lookUpVenture(char *appName, char *authName)
{
	int	i;
	Venture	*venture;

	if (appName == NULL || authName == NULL)
	{
		return NULL;
	}

	for (i = 1; i <= MaxVentureNbr; i++)
	{
		venture = mib->ventures[i];
		if (venture == NULL)
		{
			continue;
		}

		if (strcmp(venture->app->name, appName) == 0
		&& strcmp(venture->authorityName, authName) == 0)
		{
			return venture;
		}
	}

	return NULL;
}

Unit	*lookUpUnit(Venture *venture, char *unitName)
{
	int	i;
	Unit	*unit;

	for (i = 0; i <= MaxUnitNbr; i++)
	{
		unit = venture->units[i];
		if (unit == NULL)
		{
			continue;
		}

		if (strcmp(unit->name, unitName) == 0)
		{
			return unit;
		}
	}

	return NULL;
}

int	lookUpContinuum(char *contName)
{
	int		i;
	Continuum	*continuum;

	for (i = 1; i <= MaxContinNbr; i++)
	{
		continuum = mib->continua[i];
		if (continuum == NULL)
		{
			continue;	/*	Undefined continuum.	*/
		}

		if (strcmp(continuum->name, contName) == 0)
		{
			return i;
		}
	}

	return -1;
}

static int	getAuthenticationParms(unsigned char ventureNbr,
			unsigned short unitNbr, unsigned char roleNbr,
			Venture **venture, Unit **unit, int sending,
			char **authName, char **authKey, int *authKeyLen)
{
	AppRole	*role = NULL;

	*venture = NULL;
	*unit = NULL;
	if (ventureNbr > 0)
	{
		if (ventureNbr > MaxVentureNbr
		|| (*venture = mib->ventures[ventureNbr]) == NULL)
		{
			putErrmsg("MAMS msg from unknown message space.",
					itoa(ventureNbr));
			errno = EINVAL;
			return -1;
		}

		if (unitNbr > MaxUnitNbr
		|| (*unit = (*venture)->units[unitNbr]) == NULL)
		{
			putErrmsg("MAMS msg from unknown cell.",
					itoa(unitNbr));
			errno = EINVAL;
			return -1;
		}
	}

	if (roleNbr > 0)
	{
		if (roleNbr > MaxRoleNbr || *venture == NULL
		|| (role = (*venture)->roles[roleNbr]) == NULL)
		{
			putErrmsg("MAMS message discarded; unknown role.",
					itoa(roleNbr));
			errno = EINVAL;
			return -1;
		}
	}

	if (role)		/*	Sender is module.			*/
	{
		*authName = role->name;
		if (sending)
		{
			*authKey = role->privateKey;
			*authKeyLen = role->privateKeyLength;
		}
		else
		{
			*authKey = role->publicKey;
			*authKeyLen = role->publicKeyLength;
		}
	}
	else if (*venture)	/*	Sender is registrar.		*/
	{
		*authName = (*venture)->app->name;
		if (sending)
		{
			*authKey = (*venture)->app->privateKey;
			*authKeyLen = (*venture)->app->privateKeyLength;
		}
		else
		{
			*authKey = (*venture)->app->publicKey;
			*authKeyLen = (*venture)->app->publicKeyLength;
		}
	}
	else			/*	Sender is CS.		*/
	{
		*authName = mib->continua[mib->localContinuumNbr]->name;
		if (sending)
		{
			*authKey = mib->csPrivateKey;
			*authKeyLen = mib->csPrivateKeyLength;
		}
		else
		{
			*authKey = mib->csPublicKey;
			*authKeyLen = mib->csPublicKeyLength;
		}
	}

	return 0;
}


/*	*	*	MIB loading functions	*	*	*	*/

static RamsNetProtocol	parseGwEid(char *gwEidString, char **gwEid,
				char *gwEidBuffer)
{
	char		*atSign = NULL;
	RamsNetProtocol	protocol = RamsNoProtocol;	/*	default	*/
	unsigned short	portNbr;
	unsigned int	ipAddress;

	if (gwEidString == NULL || gwEid == NULL)
	{
		return protocol;
	}

	*gwEid = NULL;					/*	default	*/
	atSign = strchr(gwEidString, '@');
	if (atSign == NULL)
	{
		return protocol;
	}

	*gwEid = atSign + 1;
	if (**gwEid == '\0'	/*	Endpoint ID is a NULL string.	*/
	|| strlen(*gwEid) > MAX_GW_EID)
	{
		return protocol;
	}

	*atSign = '\0';
	if (strcmp(gwEidString, "bp") == 0)
	{
		protocol = RamsBp;
	}
	else if (strcmp(gwEidString, "udp") == 0)
	{
		protocol = RamsUdp;

		/*	Must convert to canonical hostnbr:portnbr
		 *	form to enable recvfrom lookup to succeed.	*/

		parseSocketSpec(*gwEid, &portNbr, &ipAddress);
		isprintf(gwEidBuffer, MAX_GW_EID + 1, "%u:%hu", ipAddress,
				portNbr);
		*gwEid = gwEidBuffer;
	}

	*atSign = '@';		/*	Restore original string.	*/
	return protocol;
}

static void	eraseElement(MsgElement *element)
{
	if (element->name)
	{
		MRELEASE(element->name);
	}

	if (element->description)
	{
		MRELEASE(element->description);
	}

	MRELEASE(element);
}

static void	destroyElement(LystElt elt, void *userdata)
{
	MsgElement	*element = (MsgElement *) lyst_data(elt);

	eraseElement(element);
}

LystElt	createElement(Subject *subj, char *name, ElementType type,
		char *description)
{
	int		length;
	MsgElement	*element;
	int		nameLen;
	int		descLen = 0;
	LystElt		elt;

	if (subj == NULL || name == NULL
	|| (length = strlen(name)) > MAX_ELEM_NAME)
	{
		putErrmsg(BadParmsMemo, NULL);
		errno = EINVAL;
		return NULL;
	}

	element = (MsgElement *) MTAKE(sizeof(MsgElement));
	if (element == NULL)
	{
		putSysErrmsg(NoMemoryMemo, NULL);
		return NULL;
	}

	memset((char *) element, 0, sizeof(MsgElement));
	element->type = type;
	nameLen = length + 1;
	element->name = MTAKE(nameLen);
	if (description)
	{
		descLen = strlen(description) + 1;
		element->description = MTAKE(descLen);
	}

	if (element->name == NULL
	|| (description && element->description == NULL))
	{
		eraseElement(element);
		putSysErrmsg(NoMemoryMemo, NULL);
		return NULL;
	}

	istrcpy(element->name, name, nameLen);
	if (description && descLen > 1)
	{
		istrcpy(element->description, description, descLen);
	}

	elt = lyst_insert_last(subj->elements, element);
	if (elt == NULL)
	{
		eraseElement(element);
		putSysErrmsg(NoMemoryMemo, NULL);
		return NULL;
	}

	return elt;
}

static void	destroyFanModule(LystElt elt, void *userdata)
{
	FanModule	*fan = (FanModule *) lyst_data(elt);

	MRELEASE(fan);
}

void	eraseSubject(Venture *venture, Subject *subj)
{
	if (venture == NULL || subj == NULL)
	{
		return;
	}

	if (subj->elt)		/*	Must remove from hashtable.	*/
	{
		lyst_delete(subj->elt);
	}

	if (subj->name)
	{
		MRELEASE(subj->name);
	}

	if (subj->description)
	{
		MRELEASE(subj->description);
	}

	if (subj->symmetricKey)
	{
		MRELEASE(subj->symmetricKey);
	}

	if (subj->elements)
	{
		lyst_destroy(subj->elements);
	}

	if (subj->authorizedSenders)
	{
		lyst_destroy(subj->authorizedSenders);
	}

	if (subj->authorizedReceivers)
	{
		lyst_destroy(subj->authorizedReceivers);
	}

	if (subj->modules)
	{
		lyst_destroy(subj->modules);
	}

	venture->subjects[subj->nbr] = NULL;
	MRELEASE(subj);
}

Subject	*createSubject(Venture *venture, int nbr, char *name, char *description,
		char *symmetricKey, int symmetricKeyLength)
{
	int	length;
	Subject	*subj;
	int	nameLen;
	int	descLen = 0;
	int	idx;

	if (venture == NULL || nbr < 0 || nbr > MaxSubjNbr
	|| venture->subjects[nbr] != NULL || name == NULL
	|| (length = strlen(name)) > MAX_SUBJ_NAME
	|| symmetricKeyLength < 0
	|| (symmetricKeyLength > 0 && symmetricKey == NULL))
	{
		putErrmsg(BadParmsMemo, NULL);
		errno = EINVAL;
		return NULL;
	}

	subj = (Subject *) MTAKE(sizeof(Subject));
	if (subj == NULL)
	{
		putSysErrmsg(NoMemoryMemo, NULL);
		return NULL;
	}

	memset((char *) subj, 0, sizeof(Subject));
	subj->nbr = nbr;
	subj->isContinuum = 0;
	nameLen = length + 1;
	subj->name = MTAKE(nameLen);
	if (description)
	{
		descLen = strlen(description) + 1;
		subj->description = MTAKE(descLen);
	}

	if (symmetricKey)
	{
		subj->symmetricKey = MTAKE(symmetricKeyLength);
	}

	subj->elements = lyst_create_using(amsMemory);
/*
For future use:
	subj->authorizedSenders = lyst_create_using(amsMemory);
	subj->authorizedReceivers = lyst_create_using(amsMemory);
*/
	subj->modules = lyst_create_using(amsMemory);
	if (subj->name == NULL
	|| (description && subj->description == NULL)
	|| (symmetricKey && subj->symmetricKey == NULL)
	|| subj->elements == NULL
/*
For future use:
	|| subj->authorizedSenders == NULL
	|| subj->authorizedReceivers == NULL
*/
	|| subj->modules == NULL)
	{
		eraseSubject(venture, subj);
		putSysErrmsg(NoMemoryMemo, NULL);
		return NULL;
	}

	istrcpy(subj->name, name, nameLen);
	if (description && descLen > 1)
	{
		istrcpy(subj->description, description, descLen);
	}

	if (symmetricKey)
	{
		memcpy(subj->symmetricKey, symmetricKey, symmetricKeyLength);
	}

	subj->keyLength = symmetricKeyLength;
	lyst_delete_set(subj->elements, destroyElement, NULL);
	lyst_delete_set(subj->modules, destroyFanModule, NULL);
	venture->subjects[nbr] = subj;
	idx = hashSubjectName(name);
	subj->elt = lyst_insert_last(venture->subjLysts[idx], subj);
       	if (subj->elt == NULL)
	{
		eraseSubject(venture, subj);
		putSysErrmsg(NoMemoryMemo, NULL);
		return NULL;
	}

	return subj;
}

void	eraseRole(Venture *venture, AppRole *role)
{
	if (venture == NULL || role == NULL)
	{
		return;
	}

	if (role->name)
	{
		MRELEASE(role->name);
	}

	if (role->publicKey)
	{
		MRELEASE(role->publicKey);
	}

	if (role->privateKey)
	{
		MRELEASE(role->privateKey);
	}

	venture->roles[role->nbr] = NULL;
	MRELEASE(role);
}

AppRole	*createRole(Venture *venture, int nbr, char *name, char *publicKey,
		int publicKeyLength, char *privateKey, int privateKeyLength)
{
	int	length;
	AppRole	*role;
	int	nameLen;

	if (venture == NULL || nbr < 1 || nbr > MaxRoleNbr
	|| venture->roles[nbr] != NULL || name == NULL
	|| (length = strlen(name)) > MAX_ROLE_NAME
	|| publicKeyLength < 0 || (publicKeyLength > 0 && publicKey == NULL)
	|| privateKeyLength < 0 || (privateKeyLength > 0 && privateKey == NULL))
	{
		putErrmsg(BadParmsMemo, NULL);
		errno = EINVAL;
		return NULL;
	}

	role = (AppRole *) MTAKE(sizeof(AppRole));
	if (role == NULL)
	{
		putSysErrmsg(NoMemoryMemo, NULL);
		return NULL;
	}

	memset((char *) role, 0, sizeof(AppRole));
	role->nbr = nbr;
	nameLen = length + 1;
	role->name = MTAKE(nameLen);
	if (publicKey)
	{
		role->publicKey = MTAKE(publicKeyLength);
	}

	if (privateKey)
	{
		role->privateKey = MTAKE(privateKeyLength);
	}

	if (role->name == NULL
	|| (publicKey && role->publicKey == NULL)
	|| (privateKey && role->privateKey == NULL))
	{
		eraseRole(venture, role);
		putSysErrmsg(NoMemoryMemo, NULL);
		return NULL;
	}

	istrcpy(role->name, name, nameLen);
	if (publicKey)
	{
		memcpy(role->publicKey, publicKey, publicKeyLength);
	}

	role->publicKeyLength = publicKeyLength;
	if (privateKey)
	{
		memcpy(role->privateKey, privateKey, privateKeyLength);
	}

	role->privateKeyLength = privateKeyLength;
	venture->roles[nbr] = role;
	return role;
}

static void	eraseApp(AmsApp *app)
{
	if (app == NULL)
	{
		return;
	}

	if (app->name)
	{
		MRELEASE(app->name);
	}

	if (app->publicKey)
	{
		MRELEASE(app->publicKey);
	}

	if (app->privateKey)
	{
		MRELEASE(app->privateKey);
	}

	MRELEASE(app);
}

static void	destroyApplication(LystElt elt, void *userdata)
{
	AmsApp	*app = (AmsApp *) lyst_data(elt);

	eraseApp(app);
}

LystElt	createApp(char *name, char *publicKey, int publicKeyLength,
		char *privateKey, int privateKeyLength)
{
	int	length;
	AmsApp	*app;
	int	nameLen;
	LystElt	elt;

	if (name == NULL
	|| (length = strlen(name)) > MAX_APP_NAME
	|| publicKeyLength < 0 || (publicKeyLength > 0 && publicKey == NULL)
	|| privateKeyLength < 0 || (privateKeyLength > 0 && privateKey == NULL))
	{
		putErrmsg(BadParmsMemo, NULL);
		errno = EINVAL;
		return NULL;
	}

	app = (AmsApp *) MTAKE(sizeof(AmsApp));
	if (app == NULL)
	{
		putSysErrmsg(NoMemoryMemo, NULL);
		return NULL;
	}

	memset((char *) app, 0, sizeof(AmsApp));
	nameLen = length + 1;
	app->name = MTAKE(nameLen);
	if (publicKey)
	{
		app->publicKey = MTAKE(publicKeyLength);
	}

	if (privateKey)
	{
		app->privateKey = MTAKE(privateKeyLength);
	}

	if (app->name == NULL
	|| (publicKey && app->publicKey == NULL)
	|| (privateKey && app->privateKey == NULL))
	{
		eraseApp(app);
		putSysErrmsg(NoMemoryMemo, NULL);
		return NULL;
	}

	istrcpy(app->name, name, nameLen);
	if (publicKey)
	{
		memcpy(app->publicKey, publicKey, publicKeyLength);
	}

	app->publicKeyLength = publicKeyLength;
	if (privateKey)
	{
		memcpy(app->privateKey, privateKey, privateKeyLength);
	}

	app->privateKeyLength = privateKeyLength;
	elt = lyst_insert_last(mib->applications, app);
	if (elt == NULL)
	{
		eraseApp(app);
		putSysErrmsg(NoMemoryMemo, NULL);
		return NULL;
	}

	return elt;
}

void	eraseMsgspace(Venture *venture, Subject *msgspace)
{
	if (venture == NULL || msgspace == NULL)
	{
		return;
	}

	if (msgspace->symmetricKey)
	{
		MRELEASE(msgspace->symmetricKey);
	}

	if (msgspace->elements)
	{
		lyst_destroy(msgspace->elements);
	}

	if (msgspace->authorizedSenders)
	{
		lyst_destroy(msgspace->authorizedSenders);
	}

	if (msgspace->authorizedReceivers)
	{
		lyst_destroy(msgspace->authorizedReceivers);
	}

	if (msgspace->modules)
	{
		lyst_destroy(msgspace->modules);
	}

	venture->msgspaces[0 - msgspace->nbr] = NULL;
	MRELEASE(msgspace);
}

Subject	*createMsgspace(Venture *venture, int continNbr, char *symmetricKey,
		int symmetricKeyLength)
{
	Subject	*msgspace;

	if (venture == NULL || continNbr < 1 || continNbr > MaxContinNbr
	|| mib->continua[continNbr] == NULL
	|| venture->msgspaces[continNbr] != NULL
	|| symmetricKeyLength < 0
	|| (symmetricKeyLength > 0 && symmetricKey == NULL))
	{
		putErrmsg(BadParmsMemo, NULL);
		errno = EINVAL;
		return NULL;
	}

	msgspace = (Subject *) MTAKE(sizeof(Subject));
	if (msgspace == NULL)
	{
		putSysErrmsg(NoMemoryMemo, NULL);
		return NULL;
	}

	memset((char *) msgspace, 0, sizeof(Subject));
	msgspace->nbr = 0 - continNbr;	/*	Negative subj number.	*/
	msgspace->isContinuum = 1;
	if (symmetricKey)
	{
		msgspace->symmetricKey = MTAKE(symmetricKeyLength);
	}

	msgspace->elements = lyst_create_using(amsMemory);
#if 0
	msgspace->authorizedSenders = lyst_create_using(amsMemory);
	msgspace->authorizedReceivers = lyst_create_using(amsMemory);
#endif
	msgspace->modules = lyst_create_using(amsMemory);
	if ((symmetricKey && msgspace->symmetricKey == NULL)
	|| msgspace->elements == NULL
#if 0
	|| msgspace->authorizedSenders == NULL
	|| msgspace->authorizedReceivers == NULL
#endif
	|| msgspace->modules == NULL)
	{
		eraseMsgspace(venture, msgspace);
		putSysErrmsg(NoMemoryMemo, NULL);
		return NULL;
	}

	if (symmetricKey)
	{
		memcpy(msgspace->symmetricKey, symmetricKey,
				symmetricKeyLength);
	}

	msgspace->keyLength = symmetricKeyLength;
	lyst_delete_set(msgspace->elements, destroyElement, NULL);
	lyst_delete_set(msgspace->modules, destroyFanModule, NULL);
	venture->msgspaces[continNbr] = msgspace;
	return msgspace;
}

static void	eraseModule(Module *module)
{
	LystElt	elt;
	LystElt	nextElt;

	if (module->amsEndpoints)
	{
		for (elt = lyst_first(module->amsEndpoints); elt; elt = nextElt)
		{
			nextElt = lyst_next(elt);
			lyst_delete(elt);
		}
	}

	if (module->subjects)
	{
		for (elt = lyst_first(module->subjects); elt; elt = nextElt)
		{
			nextElt = lyst_next(elt);
			lyst_delete(elt);
		}
	}

	clearMamsEndpoint(&module->mamsEndpoint);
	module->role = NULL;
}

static void	destroySubjOfInterest(LystElt elt, void *userdata)
{
	SubjOfInterest	*subj = (SubjOfInterest *) lyst_data(elt);

	lyst_destroy(subj->subscriptions);
	lyst_destroy(subj->invitations);
	lyst_delete(subj->fanElt);
	MRELEASE(subj);
}

static Module	*createModule(Cell *cell, int moduleNbr)
{
	Module	*module;

	module = (Module *) MTAKE(sizeof(Module));
	if (module == NULL)
	{
		putSysErrmsg(NoMemoryMemo, NULL);
		return NULL;
	}

	memset((char *) module, 0, sizeof(Module));
	module->unitNbr = cell->unit->nbr;
	module->nbr = moduleNbr;
	module->role = NULL;
	module->amsEndpoints = lyst_create_using(amsMemory);
	module->subjects = lyst_create_using(amsMemory);
	if (module->amsEndpoints == NULL || module->subjects == NULL)
	{
		eraseModule(module);
		putSysErrmsg(NoMemoryMemo, NULL);
		return NULL;
	}

	lyst_delete_set(module->amsEndpoints, destroyAmsEndpoint, NULL);
	lyst_delete_set(module->subjects, destroySubjOfInterest, NULL);
	cell->modules[moduleNbr] = module;
	return module;
}

void	eraseUnit(Venture *venture, Unit *unit)
{
	Cell	*cell;
	int	i;
	Unit	*superunit;
	LystElt	elt;
	LystElt	nextElt;
	Unit	*subunit;

	if (venture == NULL || unit == NULL)
	{
		return;
	}

	if (unit->name)
	{
		MRELEASE(unit->name);
	}

	/*	Erase all local cell data.				*/

	cell = unit->cell;
	clearMamsEndpoint(&cell->mamsEndpoint);
	for (i = 1; i <= MaxModuleNbr; i++)
	{
		if (cell->modules[i])
		{
			eraseModule(cell->modules[i]);
		}
	}

	/*	Reattach erased unit's subunits directly to its
	 *	superunit, if any.					*/

	superunit = unit->superunit;
	for (elt = lyst_first(unit->subunits); elt; elt = nextElt)
	{
		nextElt = lyst_next(elt);
		subunit = (Unit *) lyst_data(elt);
		lyst_delete(subunit->inclusionElt);
		if (superunit)
		{
			subunit->inclusionElt =
				lyst_insert_last(superunit->subunits, subunit);
		}

		subunit->superunit = superunit;
	}

	/*	Detach erased unit from its superunit.			*/

	if (unit->inclusionElt)
	{
		lyst_delete(unit->inclusionElt);
	}

	unit->superunit = NULL;

	/*	Erase the unit object itself.				*/

	venture->units[unit->nbr] = NULL;
	MRELEASE(unit);
}

static Unit	*initializeUnit(Venture *venture, int nbr, char *name,
			int length, int resyncPeriod, Unit *superunit,
			Lyst subunits)
{
	Unit	*unit;
	int	nameLen;
	Cell	*cell;
	int	i;
	LystElt	elt;
	LystElt	nextElt;
	Unit	*subunit;

	unit = (Unit *) MTAKE(sizeof(Unit));
	if (unit == NULL)
	{
		lyst_destroy(subunits);
		putSysErrmsg(NoMemoryMemo, NULL);
		return NULL;
	}

	memset((char *) unit, 0, sizeof(Unit));
	unit->cell = &(unit->cellData);
	cell = unit->cell;
	cell->unit = unit;
	unit->nbr = nbr;
	nameLen = length + 1;
	unit->name = MTAKE(nameLen);
	if (unit->name == NULL)
	{
		eraseUnit(venture, unit);
		lyst_destroy(subunits);
		putSysErrmsg(NoMemoryMemo, NULL);
		return NULL;
	}

	istrcpy(unit->name, name, nameLen);

	/*	Initialize cell data of unit.				*/

	for (i = 1; i <= MaxModuleNbr; i++)
	{
		if (createModule(cell, i) < 0)
		{
			eraseUnit(venture, unit);
			lyst_destroy(subunits);
			putSysErrmsg(NoMemoryMemo, NULL);
			return NULL;
		}
	}

	cell->resyncPeriod = resyncPeriod;

	/*	Insert new unit as subunit of its superunit.		*/

	unit->superunit = superunit;
	if (superunit != NULL)
	{
		unit->inclusionElt = lyst_insert_last(superunit->subunits,
				unit);
		if (unit->inclusionElt == NULL)
		{
			eraseUnit(venture, unit);
			lyst_destroy(subunits);
			putSysErrmsg(NoMemoryMemo, NULL);
			return NULL;
		}
	}

	/*	Assert new unit as superunit of its subunits.		*/

	for (elt = lyst_first(subunits); elt; elt = nextElt)
	{
		nextElt = lyst_next(elt);
		subunit = (Unit *) lyst_data(elt);
		if (subunit->superunit != superunit)
		{
			/*	This is a subunit of some subunit of
			 *	the new unit, not of the new unit
			 *	directly.				*/

			lyst_delete(elt);
			continue;
		}

		/*	Detach this unit from its current superunit
		 *	(the new module's superunit) and insert it as
		 *	a subunit of the new unit.			*/

		lyst_delete(subunit->inclusionElt);
		subunit->inclusionElt = elt;
		subunit->superunit = unit;
	}

	unit->subunits = subunits;
	venture->units[nbr] = unit;
	return unit;
}

Unit	*createUnit(Venture *venture, int nbr, char *name, int resyncPeriod)
{
	int	namelen;
	int	i;
	Lyst	subunits;
	int	bestLength;
	Unit	*superunit;
	Unit	*unit;
	int	j;

	if (venture == NULL || nbr < 1 || nbr > MaxUnitNbr
	|| venture->units[nbr] != NULL || name == NULL || resyncPeriod < 0
	|| (namelen = strlen(name)) == 0 || namelen > MAX_UNIT_NAME)
	{
		putErrmsg(BadParmsMemo, NULL);
		errno = EINVAL;
		return NULL;
	}

	/*	Unit whose name is the longest prefix of the new
	 *	unit's name is the new unit's superunit.  Every
	 *	unit whose name is prefixed by the new unit's name
	 *	is a candidate subunit of the new unit.			*/

	subunits = lyst_create_using(amsMemory);
	if (subunits == NULL)
	{
		putSysErrmsg(NoMemoryMemo, NULL);
		return NULL;
	}

	bestLength = 0;
	superunit = venture->units[0];	/*	Default is root cell.	*/
	for (i = 0; i <= MaxUnitNbr; i++)
	{
		if ((unit = venture->units[i]) == NULL) continue;
		for (j= 0;; j++)
		{
			if (unit->name[j] == '\0')
			{
				if (j > bestLength)
				{
					/*	May be the superunit.	*/

					bestLength = j;
					superunit = unit;
				}

				break;
			}

			if (name[j] == '\0')
			{
				/*	May be a subunit.		*/

				if (lyst_insert_last(subunits, unit) == NULL)
				{
					lyst_destroy(subunits);
					putSysErrmsg(NoMemoryMemo, NULL);
					return NULL;
				}

				break;
			}

			if (unit->name[j] != name[j])
			{
				break;
			}
		}
	}

	if (bestLength == namelen)	/*	Perfect match for name.	*/
	{
		putErrmsg("Duplicate unit name.", NULL);
		errno = EINVAL;
		return NULL;
	}

	return initializeUnit(venture, nbr, name, namelen, resyncPeriod,
			superunit, subunits);
}

void	eraseVenture(Venture *venture)
{
	int	i;

	if (venture == NULL)
	{
		return;
	}

	for (i = 1; i <= MaxRoleNbr; i++)
	{
		if (venture->roles[i])
		{
			eraseRole(venture, venture->roles[i]);
		}
	}

	for (i = 1; i <= MaxSubjNbr; i++)
	{
		if (venture->subjects[i])
		{
			eraseSubject(venture, venture->subjects[i]);
		}
	}

	for (i = 0; i < SUBJ_LIST_CT; i++)
	{
		if (venture->subjLysts[i])
		{
			lyst_destroy(venture->subjLysts[i]);
		}
	}

	eraseUnit(venture, venture->units[0]);		/*	Root.	*/
	for (i = 1; i <= MaxContinNbr; i++)
	{
		if (venture->msgspaces[i])
		{
			eraseMsgspace(venture, venture->msgspaces[i]);
		}
	}

	mib->ventures[venture->nbr] = NULL;
	MRELEASE(venture);
}

Venture	*createVenture(int nbr, char *appname, char *authname,
			int rootCellResyncPeriod)
{
	int	length;
	LystElt	elt;
	AmsApp	*app;
	int	i;
	Venture	*venture;
	int	authnameLen;
	AppRole	*gatewayRole;
	Subject	*allSubjects;
	Lyst	subunits;
	Unit	*rootUnit;
	Subject	*msgspace;	/*	Msgspace in local continuum.	*/

	if (nbr < 1 || nbr > MaxVentureNbr || mib->ventures[nbr] != NULL
	|| appname == NULL || authname == NULL
	|| (length = strlen(authname)) > MAX_AUTH_NAME
	|| rootCellResyncPeriod < 0)
	{
		putErrmsg(BadParmsMemo, NULL);
		errno = EINVAL;
		return NULL;
	}

	for (elt = lyst_first(mib->applications); elt; elt = lyst_next(elt))
	{
		app = (AmsApp *) lyst_data(elt);
		if (strcmp(app->name, appname) == 0)
		{
			break;
		}
	}

	if (elt == NULL)
	{
		putErrmsg("Adding venture for unknown application.",
				appname);
		errno = EINVAL;
		return NULL;
	}

	venture = (Venture *) MTAKE(sizeof(Venture));
	if (venture == NULL)
	{
		putSysErrmsg(NoMemoryMemo, NULL);
		return NULL;
	}

	memset((char *) venture, 0, sizeof(Venture));
	venture->nbr = nbr;
	venture->app = app;
	authnameLen = length + 1;
	venture->authorityName = MTAKE(authnameLen);
	if (venture->authorityName == NULL)
	{
		eraseVenture(venture);
		putSysErrmsg(NoMemoryMemo, NULL);
		return NULL;
	}

	istrcpy(venture->authorityName, authname, authnameLen);
	for (i = 0; i < SUBJ_LIST_CT; i++)
	{
		venture->subjLysts[i] = lyst_create_using(amsMemory);
		if (venture->subjLysts[i] == NULL)
		{
			eraseVenture(venture);
			putSysErrmsg(NoMemoryMemo, NULL);
			return NULL;
		}
	}

	mib->ventures[nbr] = venture;

	/*	Automatically create venture's RAMS gateway role.	*/

	gatewayRole = createRole(venture, 1, "RAMS", NULL, 0, NULL, 0);
	if (gatewayRole == NULL)
	{
		eraseVenture(venture);
		putErrmsg("Can't create RAMS gateway role for venture.",
				appname);
		return NULL;
	}

	/*	Automatically create venture's "all subjects" subject.	*/

	allSubjects = createSubject(venture, 0, "everything", "messages on \
all subjects", NULL, 0);
	if (allSubjects == NULL)
	{
		eraseVenture(venture);
		putErrmsg("Can't create 'everything' subject for venture.",
				appname);
		return NULL;
	}

	/*	Automatically create venture's root unit.		*/

	subunits = lyst_create_using(amsMemory);
	if (subunits == NULL)
	{
		eraseVenture(venture);
		putSysErrmsg(NoMemoryMemo, NULL);
		return NULL;
	}

	rootUnit = initializeUnit(venture, 0, "", 0, rootCellResyncPeriod,
			NULL, subunits);
	if (rootUnit == NULL)
	{
		eraseVenture(venture);
		putErrmsg(NoMemoryMemo, NULL);
		return NULL;
	}

	/*	Automatically create the local message space.		*/

	msgspace = createMsgspace(venture, mib->localContinuumNbr, NULL, 0);
	if (msgspace == NULL)
	{
		eraseVenture(venture);
		putErrmsg(NoMemoryMemo, NULL);
		return NULL;
	}

	return venture;
}

static void	eraseContinuum(Continuum *contin)
{
	if (contin == NULL)
	{
		return;
	}

	if (contin->name)
	{
		MRELEASE(contin->name);
	}

	if (contin->gwEid)
	{
		MRELEASE(contin->gwEid);
	}

	if (contin->description)
	{
		MRELEASE(contin->description);
	}

	MRELEASE(contin);
}

Continuum	*createContinuum(int nbr, char *name, char *gwEidString,
			int isNeighbor, char *description)
{
	int		length;
	RamsNetProtocol	ramsProtocol;
	char		*gwEid;
	char		gwEidBuffer[MAX_GW_EID + 1];
	Continuum	*contin;
	int		gwEidLen;
	int		nameLen;
	int		descLen = 0;

	if (nbr < 1 || nbr > MaxContinNbr || mib->continua[nbr] != NULL
	|| name == NULL || (length = strlen(name)) > MAX_SUBJ_NAME)
	{
		putErrmsg(BadParmsMemo, NULL);
		errno = EINVAL;
		return NULL;
	}

	ramsProtocol = parseGwEid(gwEidString, &gwEid, gwEidBuffer);
	if (ramsProtocol == RamsNoProtocol)
	{
		ramsProtocol = RamsBp;
		isprintf(gwEidBuffer, sizeof gwEidBuffer, "ipn:%d.9", nbr);
		gwEid = gwEidBuffer;
	}

	contin = (Continuum *) MTAKE(sizeof(Continuum));
	if (contin == NULL)
	{
		putSysErrmsg(NoMemoryMemo, NULL);
		return NULL;
	}

	memset((char *) contin, 0, sizeof(Continuum));
	contin->nbr = nbr;
	contin->gwProtocol = ramsProtocol;
	gwEidLen = strlen(gwEid) + 1;
	contin->gwEid = MTAKE(gwEidLen);
	contin->isNeighbor = 1 - (isNeighbor == 0);
	nameLen = length + 1;
	contin->name = MTAKE(nameLen);
	if (description)
	{
		descLen = strlen(description) + 1;
		contin->description = MTAKE(descLen);
	}

	if (contin->name == NULL
	|| contin->gwEid == NULL
	|| (description && contin->description == NULL))
	{
		eraseContinuum(contin);
		putSysErrmsg(NoMemoryMemo, NULL);
		return NULL;
	}

	istrcpy(contin->name, name, nameLen);
	istrcpy(contin->gwEid, gwEid, gwEidLen);
	if (description && descLen > 1)
	{
		istrcpy(contin->description, description, descLen);
	}

	mib->continua[nbr] = contin;
	return contin;
}

static void	eraseCsEndpoint(MamsEndpoint *ep)
{
	clearMamsEndpoint(ep);
	MRELEASE(ep);
}

static void	destroyCsEndpoint(LystElt elt, void *userdata)
{
	MamsEndpoint	*ep = (MamsEndpoint *) lyst_data(elt);

	eraseCsEndpoint(ep);
}

LystElt	createCsEndpoint(char *endpointSpec, LystElt nextElt)
{
	int		len;
	char		endpointName[MAX_EP_NAME + 1];
	MamsEndpoint	*ep;
	LystElt		elt;

	if (mib->pts->csepNameFn(endpointSpec, endpointName) < 0)
	{
		putErrmsg("Can't compute CS endpoint name.", endpointSpec);
		errno = EINVAL;
		return NULL;
	}

	len = strlen(endpointName);
	ep = (MamsEndpoint *) MTAKE(sizeof(MamsEndpoint));
	if (ep == NULL)
	{
		putSysErrmsg(NoMemoryMemo, NULL);
		return NULL;
	}

	memset((char *) ep, 0, sizeof(MamsEndpoint));
	if (constructMamsEndpoint(ep, len, endpointName) < 0)
	{
		eraseCsEndpoint(ep);
		putErrmsg("Can't construct CS endpoint.", NULL);
		return NULL;
	}

	if (nextElt)
	{
		elt = lyst_insert_before(nextElt, ep);
	}
	else
	{
		elt = lyst_insert_last(mib->csEndpoints, ep);
	}

	if (elt == NULL)
	{
		eraseCsEndpoint(ep);
		putSysErrmsg(NoMemoryMemo, NULL);
		return NULL;
	}

	return elt;
}

static void	eraseAmsEpspec(AmsEpspec *amses)
{
	MRELEASE(amses);
}

static void	destroyAmsEpspec(LystElt elt, void *userdata)
{
	AmsEpspec	*amses = (AmsEpspec *) lyst_data(elt);

	eraseAmsEpspec(amses);
}

LystElt	createAmsEpspec(char *tsname, char *epspec)
{
	int		i;
	TransSvc	*ts;
	AmsEpspec	amses;
	AmsEpspec	*amsesPtr;
	LystElt		elt;

	if (epspec == NULL || tsname == NULL)
	{
		putErrmsg("NULL parameter(s) for AMS endpoint spec.", NULL);
		errno = EINVAL;
		return NULL;
	}
	
	if (strlen(epspec) > MAX_EP_SPEC)
	{
		putErrmsg("AMS endpoint spec is too long.", epspec);
		errno = EINVAL;
		return NULL;
	}

	istrcpy(amses.epspec, epspec, sizeof amses.epspec);
	amses.ts = NULL;
	for (i = 0, ts = mib->transportServices; i < mib->transportServiceCount;
			i++, ts++)
	{
		if (strcmp(tsname, ts->name) == 0)
		{
			amses.ts = ts;
			break;
		}
	}

	if (amses.ts == NULL)
	{
		putErrmsg("Unknown transport service for AMS endpoint spec.",
				tsname);
		errno = EINVAL;
		return NULL;
	}

	amsesPtr = (AmsEpspec *) MTAKE(sizeof(AmsEpspec));
	if (amsesPtr == NULL)
	{
		putSysErrmsg(NoMemoryMemo, NULL);
		return NULL;
	}

	memcpy((char *) amsesPtr, &amses, sizeof(AmsEpspec));
	elt = lyst_insert_last(mib->amsEndpointSpecs, amsesPtr);
	if (elt == NULL)
	{
		eraseAmsEpspec(amsesPtr);
		putSysErrmsg(NoMemoryMemo, NULL);
		return NULL;
	}

	return elt;
}

void	eraseMib()
{
	int	i;

	if (mib->localContinuumGwEid)
	{
		MRELEASE(mib->localContinuumGwEid);
	}

	pthread_mutex_destroy(&(mib->mutex));
	if (mib->csPublicKey)
	{
		MRELEASE(mib->csPublicKey);
	}

	if (mib->csPrivateKey)
	{
		MRELEASE(mib->csPrivateKey);
	}

	if (mib->amsEndpointSpecs)
	{
		lyst_destroy(mib->amsEndpointSpecs);
	}

	if (mib->applications)
	{
		lyst_destroy(mib->applications);
	}

	if (mib->csEndpoints)
	{
		lyst_destroy(mib->csEndpoints);
	}

	for (i = 1; i <= MaxContinNbr; i++)
	{
		if (mib->continua[i])
		{
			eraseContinuum(mib->continua[i]);
		}
	}

	for (i = 1; i <= MaxVentureNbr; i++)
	{
		if (mib->ventures[i])
		{
			eraseVenture(mib->ventures[i]);
		}
	}

	MRELEASE(mib);
	mib = NULL;
}

static int	addTs(int idx, TsLoadFn loadTs)
{
	TransSvc	*ts;
       
	if (idx < 0)	/*	Adding services in order.		*/
	{
		if (mib->transportServiceCount > TS_INDEX_LIMIT)
		{
			putErrmsg("Transport service table is full.", NULL);
			errno = EINVAL;
			return -1;
		}

		idx = mib->transportServiceCount;
	}

	ts = &(mib->transportServices[idx]);
	loadTs(ts);	/*	Execute the transport service loader.	*/
	mib->transportServiceCount++;
	return 0;
}

static int	loadTransportService(int i, char *ptsName)
{
	if (addTs(-1, transportServiceLoaders[i]) < 0)
	{
		return -1;
	}

	if (strcmp(mib->transportServices[i].name, ptsName) == 0)
	{
		mib->pts = &(mib->transportServices[i]);
	}

	return 0;
}

int	createMib(int nbr, char *gwEidString, int ramsNetIsTree, char *ptsName,
		char *pubkey, int pubkeylen, char *privkey, int privkeylen)
{
	int		i;
	RamsNetProtocol	ramsProtocol;
	char		*gwEid;
	char		gwEidBuffer[MAX_GW_EID + 1];
	int		eidLen;

	if (mib != NULL || nbr < 1 || nbr > MaxContinNbr || ptsName == NULL
	|| pubkeylen < 0 || (pubkeylen > 0 && pubkey == NULL)
	|| privkeylen < 0 || (privkeylen > 0 && privkey == NULL))
	{
		putErrmsg(BadParmsMemo, NULL);
		errno = EINVAL;
		return -1;
	}

	ramsProtocol = parseGwEid(gwEidString, &gwEid, gwEidBuffer);
	if (ramsProtocol == RamsNoProtocol)
	{
		ramsProtocol = RamsBp;
		isprintf(gwEidBuffer, sizeof gwEidBuffer, "ipn:%d.9", nbr);
		gwEid = gwEidBuffer;
	}

	mib = (AmsMib *) MTAKE(sizeof(AmsMib));
	if (mib == NULL)
	{
		putSysErrmsg(NoMemoryMemo, NULL);
		return -1;
	}

	memset((char *) mib, 0, sizeof(AmsMib));
	for (i = 0; i < transportServiceCount; i++)
	{
		if (loadTransportService(i, ptsName) < 0)
		{
			eraseMib();
			return -1;
		}
	}

	if (mib->pts == NULL)
	{
		eraseMib();
		errno = EINVAL;
		putErrmsg("No loader for primary transport service.", ptsName);
		return -1;
	}

	if (pthread_mutex_init(&(mib->mutex), NULL))
	{
		eraseMib();
		putSysErrmsg("MIB mutex init failed", NULL);
		return -1;
	}

	mib->amsEndpointSpecs = lyst_create_using(amsMemory);
	mib->applications = lyst_create_using(amsMemory);
	mib->csEndpoints = lyst_create_using(amsMemory);
	mib->localContinuumNbr = nbr;
	mib->localContinuumGwProtocol = ramsProtocol;
	eidLen = strlen(gwEid) + 1;
	mib->localContinuumGwEid = MTAKE(eidLen);
	mib->ramsNetIsTree = ramsNetIsTree;
	if (pubkey)
	{
		mib->csPublicKey = MTAKE(pubkeylen);
	}

	if (privkey)
	{
		mib->csPrivateKey = MTAKE(privkeylen);
	}

	if (mib->amsEndpointSpecs == NULL
	|| mib->applications == NULL
	|| mib->csEndpoints == NULL
	|| mib->localContinuumGwEid == NULL
	|| (pubkey && mib->csPublicKey == NULL)
	|| (privkey && mib->csPrivateKey == NULL)
	|| createContinuum(nbr, "local", NULL, 0, "local continuum") == NULL)
	{
		eraseMib();
		putSysErrmsg(NoMemoryMemo, NULL);
		return -1;
	}

	istrcpy(mib->localContinuumGwEid, gwEid, eidLen);
	lyst_delete_set(mib->amsEndpointSpecs, destroyAmsEpspec, NULL);
	lyst_delete_set(mib->applications, destroyApplication, NULL);
	lyst_delete_set(mib->csEndpoints, destroyCsEndpoint, NULL);
	if (pubkey)
	{
		memcpy(mib->csPublicKey, pubkey, pubkeylen);
	}

	mib->csPublicKeyLength = pubkeylen;
	if (privkey)
	{
		memcpy(mib->csPrivateKey, privkey, privkeylen);
	}

	mib->csPrivateKeyLength = privkeylen;
	return 0;
}

/*	*	*	MIB management functions	*	*	*/

int	computeModuleId(int roleNbr, int unitNbr, int moduleNbr)
{
	unsigned int	moduleId;

	moduleId = (roleNbr << 24) + (unitNbr << 8) + moduleNbr;
	return (int) moduleId;
}

int	parseModuleId(int memo, int *roleNbr, int *unitNbr, int *moduleNbr)
{
	unsigned int	moduleId = (unsigned int) memo;

	*roleNbr = (moduleId >> 24) & 0x000000ff;
	if (*roleNbr < 1 || *roleNbr > MaxRoleNbr)
	{
		putErrmsg("Role nbr invalid.", itoa(*roleNbr));
		return -1;
	}

	*unitNbr = (moduleId >> 8) & 0x0000ffff;
	if (*unitNbr > MaxUnitNbr)
	{
		putErrmsg("Unit nbr invalid.", itoa(*unitNbr));
		return -1;
	}

	*moduleNbr = moduleId & 0x000000ff;
	if (*moduleNbr < 1 || *moduleNbr > MaxModuleNbr)
	{
		putErrmsg("Module nbr invalid.", itoa(*moduleNbr));
		return -1;
	}

	return 0;
}

char	*parseString(char **cursor, int *bytesRemaining, int *len)
{
	char	*string = *cursor;

	*len = 0;
	while (*bytesRemaining > 0)
	{
		if (**cursor == '\0')	/*	End of string.		*/
		{
			(*cursor)++;	/*	Skip over the NULL.	*/
			(*bytesRemaining)--;
			return string;
		}

		/*	This is one of the characters of the string.	*/

		(*cursor)++;
		(*bytesRemaining)--;
		(*len)++;
	}

	return NULL;		/*	Not a NULL-terminated string.	*/
}

int	constructMamsEndpoint(MamsEndpoint *endpoint, int eptLength, char *ept)
{
	endpoint->ept = MTAKE(eptLength + 1);
	if (endpoint->ept == NULL)
	{
		putErrmsg(NoMemoryMemo, NULL);
		return -1;
	}

	memcpy(endpoint->ept, ept, eptLength);
	endpoint->ept[eptLength] = '\0';
//PUTMEMO("Constructing MAMS endpoint", endpoint->ept);

	/*	The primary transport service's endpoint parsing
	 *	function examines the endpoint name text and fills
	 *	in the tsep of the endpoint.				*/

	if (((mib->pts->parseMamsEndpointFn)(endpoint)) < 0)
	{
		putErrmsg("Can't parse endpoint name.", ept);
		return -1;
	}

	return 0;
}

void	clearMamsEndpoint(MamsEndpoint *ep)
{
//PUTS("...in clearMamsEndpoint...");
	mib->pts->clearMamsEndpointFn(ep);
	ep->tsep = NULL;
	if (ep->ept)
	{
		MRELEASE(ep->ept);
		ep->ept = NULL;
	}
}

int	rememberModule(Module *module, AppRole *role, int eptLength, char *ept)
{
	if (constructMamsEndpoint(&(module->mamsEndpoint), eptLength, ept))
	{
		putErrmsg("Can't store module's MAMS endpoint.", NULL);
		return -1;
	}

	module->role = role;
	return 0;
}

void	forgetModule(Module *module)
{
	LystElt	elt;
	LystElt	nextElt;

	for (elt = lyst_first(module->amsEndpoints); elt; elt = nextElt)
	{
		nextElt = lyst_next(elt);
		lyst_delete(elt);
	}

	for (elt = lyst_first(module->subjects); elt; elt = nextElt)
	{
		nextElt = lyst_next(elt);
		lyst_delete(elt);
	}

	clearMamsEndpoint(&module->mamsEndpoint);
	module->role = NULL;
}

/*	*	*	MAMS message transmission *	*	*	*/

int	sendMamsMsg(MamsEndpoint *endpoint, MamsInterface *tsif,
		MamsPduType type, unsigned int memo,
		unsigned short supplementLength, char *supplement)
{
	Venture		*venture;
	Unit		*unit;
	char		*authName;
	char		*authKey;
	int		authKeyLen; 
	int		authNameLen;
	int		nonce;
	unsigned char	authenticator[AUTHENTICAT_LEN];
	int		encryptLength;
	int		authenticatorLength = 0;
	time_t		unixTime;
	unsigned int	u4;
	unsigned char	timeTag[5];
	int		msgLength;
	unsigned char	*msg;
	unsigned short	u2;
	unsigned short	checksum;
	int		result;

	if (endpoint == NULL || tsif == NULL)
	{
		putErrmsg(BadParmsMemo, NULL);
		errno = EINVAL;
		return -1;
	}

	if (getAuthenticationParms(tsif->ventureNbr, tsif->unitNbr,
			tsif->roleNbr, &venture, &unit, 1, &authName, &authKey,
			&authKeyLen))
	{
		putErrmsg("Can't get authentication parameters.", NULL);
		return -1;
	}

	if (authName && authKey && authKeyLen > 0)
	{
		authNameLen = strlen(authName);
		if (authNameLen == 0 || authNameLen > (AUTHENTICAT_LEN - 9))
		{
			putErrmsg("Bad authentication name.", authName);
			return -1;
		}

		encryptLength = authNameLen + 4;
		nonce = rand();
		memcpy(authenticator, (char *) &nonce, 4);/*	Clear.	*/
		memcpy(authenticator + 4, (char *) &nonce, 4);
		istrcpy((char *) (authenticator + 8), authName,
				sizeof authenticator - 8);
		encryptUsingPrivateKey((char *) (authenticator + 4),
				encryptLength, authKey, authKeyLen,
				(char *) (authenticator + 4), &encryptLength);
		authenticatorLength = encryptLength + 4;
	}

	/*	Time tag is CCSDS Unsegmented Time (CUC) with preamble.
	 *	The preamble (P-field) is always a single octet whose
	 *	bits have the following significance:
	 *		0	no extension
	 *		001	time code ID (Epoch 1958 TAI)
	 *		11	indicates 4 octets of coarse time (sec)
	 *		00	indicates 0 octets of fine time
	 *	Note that the CCSDS spec actually requires TAI time for
	 *	time code 001, but the operating system only gives us
	 *	UTC, so the time tags inserted here will be in error by
	 *	some number of leap seconds.  For the purposes of AMS
	 *	this error is not significant.				*/

	unixTime = time(NULL);			/*	Epoch 1970.	*/
	u4 = unixTime + EPOCH_OFFSET_1958;	/*	Now epoch 1958.	*/
	u4 = htonl(u4);
	timeTag[0] = 28;			/*	Preamble.	*/
	memcpy(timeTag + 1, (char *) &u4, 4);	/*	Coarse time.	*/

	/*	12 bytes of message length are fixed by specification.
	 *	5 bytes are time tag, which in the JPL implementation
	 *	is always of length 5.  2 bytes are checksum, which
	 *	in the JPL implementation is always present.		*/

	msgLength = 12 + 5 + authenticatorLength + supplementLength + 2;
	msg = MTAKE(msgLength);
	if (msg == NULL)
	{
		putSysErrmsg(NoMemoryMemo, NULL);
		return -1;
	}

	/*	Construct the message.	First octet is two bits of
	 *	version number (which is always 00 for now) followed
	 *	by checksum flag (always 1 in this implementation of
	 *	MAMS) followed by 5 bits of MPDU type.			*/

	u4 = type;
	*msg = 0x20 + (u4 & 0x0000001f);
	*(msg + 1) = tsif->ventureNbr;
	u2 = tsif->unitNbr;
	u2 = htons(u2);
	memcpy(msg + 2, (char *) &u2, 2);
	*(msg + 4) = tsif->roleNbr;
	*(msg + 5) = authenticatorLength;
	u2 = supplementLength;
	u2 = htons(u2);
	memcpy(msg + 6, (char *) &u2, 2);
	u4 = htonl(memo);
	memcpy(msg + 8, (char *) &u4, 4);
	memcpy(msg + 12, timeTag, 5);

	/*	Optionally append digital signature to header.		*/

	if (authenticatorLength > 0)
	{
		memcpy(msg + 17, authenticator, authenticatorLength);
	}

	/*	Append supplementary data to message.			*/

	if (supplementLength > 0)
	{
		memcpy(msg + 17 + authenticatorLength, supplement,
				supplementLength);
	}

	/*	For this implementation, always append checksum to
	 *	message.						*/

	checksum = computeAmsChecksum(msg, msgLength - 2);
	checksum = htons(checksum);
	memcpy(msg + (msgLength -2), (char *) &checksum, 2);

	/*	Send the message.					*/

	result = mib->pts->sendMamsFn(endpoint, tsif, (char *) msg, msgLength);
	MRELEASE(msg);
	return result;
}

/*	*	*	Event queueing functions	*	*	*/

int	enqueueMamsEvent(Llcv eventsQueue, AmsEvt *evt, char *ancillaryBlock,
		int responseNbr)
{
	long	queryNbr;
	LystElt	elt;

	llcv_lock(eventsQueue);

	/*	Events that shut down event loops are inserted at
	 *	the start of the events queue, for immediate handling.	*/

	if (evt->type == CRASH_EVT)
	{
		elt = lyst_insert_first(eventsQueue->list, evt);
		llcv_unlock(eventsQueue);
		if (elt == NULL)
		{
			MRELEASE(evt);
			putSysErrmsg(NoMemoryMemo, NULL);
			return -1;
		}

		llcv_signal(eventsQueue, time_to_stop);
		return 0;
	}

	/*	A hack, which works in the absence of a Lyst user
	 *	data variable.  In order to make the current query
	 *	ID number accessible to an llcv condition function,
	 *	we stuff it into the "compare" function pointer in
	 *	the Lyst structure.*/

	queryNbr = (long) lyst_compare_get(eventsQueue->list);
	if (queryNbr != 0)	/*	Need response to specfic msg.	*/
	{
		if (responseNbr == queryNbr)	/*	This is it.	*/
		{
			elt = lyst_insert_first(eventsQueue->list, evt);
			if (elt)	/*	Must erase query nbr.	*/
			{
				lyst_compare_set(eventsQueue->list, NULL);
				llcv_signal_while_locked(eventsQueue,
						llcv_reply_received);
			}
		}
		else	/*	This isn't it; deal with it later.	*/
		{
			elt = lyst_insert_last(eventsQueue->list, evt);
		}
	}
	else	/*	Any event is worth waking up the thread for.	*/
	{
		elt = lyst_insert_last(eventsQueue->list, evt);
		if (elt)
		{
			llcv_signal_while_locked(eventsQueue,
					llcv_lyst_not_empty);
		}
	}

	llcv_unlock(eventsQueue);
	if (elt == NULL)
	{
		MRELEASE(evt);
		if (ancillaryBlock)
		{
			MRELEASE(ancillaryBlock);
		}

		putSysErrmsg(NoMemoryMemo, NULL);
		return -1;
	}

	return 0;
}

int	enqueueMamsMsg(Llcv eventsQueue, int length, unsigned char *msgBuffer)
{
	unsigned int	preamble;	/*	Describes time tag.	*/
	int		timeCode;
	int		coarseTimeLength;
	int		fineTimeLength;
	int		expectedLength;
	unsigned int	u4;
	unsigned char	*cursor;
	MamsMsg		msg;
	unsigned short	checksum;
	unsigned short	u2;
	int		authenticatorLength;
	unsigned char	*authenticator;
	unsigned char	*supplement;
	Venture		*venture;
	Unit		*unit;
	char		*authName;
	char		*authKey;
	int		authKeyLen;
	int		authNameLen;
	unsigned char	nonce[4];
	int		encryptLength;
	AmsEvt		*evt;

	authenticator = NULL;  //initialized to avoid warning

	if (eventsQueue == NULL || msgBuffer == NULL || length < 0)
	{
		errno = EINVAL;
		putSysErrmsg(BadParmsMemo, NULL);
		return -1;
	}

	if (length < 14)
	{
		putErrmsg("MAMS msg header incomplete.", itoa(length));
		return -1;
	}

	/*	First get time tag, since its size affects the
	 *	location of the authenticator and supplement.		*/

	preamble = *(msgBuffer + 12);		/*	"P-field."	*/
	timeCode = (preamble >> 4) & 0x07;
	if (timeCode != 1)
	{
		putErrmsg("Unknown time code in MAMS msg header.",
				itoa(timeCode));
		return -1;
	}

	coarseTimeLength = ((preamble >> 2) & 0x03) + 1;
	fineTimeLength = preamble & 0x03;
	expectedLength = 12 + 1 + coarseTimeLength + fineTimeLength;
	if (length < expectedLength)
	{
		putErrmsg("MAMS message truncated.", NULL);
		return -1;
	}

	u4 = 0;
	cursor = msgBuffer + 13;		/*	After preamble.	*/
	while (coarseTimeLength > 0)
	{
		u4 += *cursor;
		u4 <<= 1;
		cursor++;
		coarseTimeLength--;
	}

	u4 = ntohl(u4);				/*	Epoch 1958.	*/
	msg.timeTag = u4 - EPOCH_OFFSET_1958;	/*	Now epoch 1970.	*/
	while (fineTimeLength > 0)
	{
		cursor++;		/*	Ignore fine time.	*/
		fineTimeLength--;
	}

	/*	Now look for checksum.					*/

	if (*msgBuffer & 0x20)		/*	Checksum present.	*/
	{
		expectedLength += 2;
		if (length < expectedLength)
		{
			putErrmsg("MAMS message truncated.", NULL);
			return -1;
		}

		memcpy((char *) &checksum, msgBuffer + (length - 2), 2);
		checksum = ntohs(checksum);
		if (checksum != computeAmsChecksum(msgBuffer, length - 2))
		{
			putErrmsg("Checksum failed, MAMS message discarded.",
					NULL);
			return -1;
		}
	}

	/*	Now extract the rest of the MAMS message.		*/

	msg.type = *msgBuffer & 0x1f;
	msg.ventureNbr = *(msgBuffer + 1);
	memcpy((char *) &u2, msgBuffer + 2, 2);
	u2 = ntohs(u2);
	msg.unitNbr = u2;
	msg.roleNbr = *(msgBuffer + 4);
	authenticatorLength = *(msgBuffer + 5);
	expectedLength += authenticatorLength;
	if (authenticatorLength > 0)
	{
		authenticator = cursor;	/*	Starts after time tag.	*/
		cursor += authenticatorLength;
	}

	memcpy((char *) &u2, msgBuffer + 6, 2);
	u2 = ntohs(u2);
	msg.supplementLength = u2;
	expectedLength += msg.supplementLength;
	supplement = cursor;	/*	After time tag, authenticator.	*/
	if (length != expectedLength)
	{
		length -= expectedLength;
		putErrmsg("MAMS message length error.", itoa(length));
		return -1;
	}

	if (getAuthenticationParms(msg.ventureNbr, msg.unitNbr, msg.roleNbr,
			&venture, &unit, 0, &authName, &authKey, &authKeyLen))
	{
		putErrmsg("Can't get authentication parameters.", NULL);
		return -1;
	}

	if (authName && authKey && authKeyLen > 0)
	{
		/*	Authentication is required.			*/

		authNameLen = strlen(authName);
		if (authNameLen == 0 || authNameLen > (AUTHENTICAT_LEN - 9))
		{
			putErrmsg(BadParmsMemo, NULL);
			errno = EINVAL;
			return -1;
		}

		if (authenticatorLength != authNameLen + 8)
		{
//PUTMEMO("authenticatorLength", utoa(authenticatorLength));
//PUTMEMO("authNameLen", utoa(authNameLen));
//PUTMEMO("authName", authName);
			putErrmsg("MAMS msg discarded; bad authenticator.",
					NULL);
			errno = EINVAL;
			return -1;
		}

		memcpy(nonce, authenticator, 4);
		decryptUsingPublicKey((char *) (authenticator + 4),
				authenticatorLength - 4, authKey, authKeyLen,
				(char *) (authenticator + 4), &encryptLength);
		if (memcmp(nonce, authenticator + 4, 4) != 0
		|| memcmp(authName, authenticator + 8, authNameLen) != 0)
		{
			putErrmsg("MAMS msg discarded; authentication failed.",
					NULL);
			errno = EINVAL;
			return -1;
		}
	}

	/*	Construct message event, enqueue, and signal arrival.	*/

	memcpy((char *) &u4, msgBuffer + 8, 4);
	u4 = ntohl(u4);
	msg.memo = u4;
	if (msg.supplementLength > 0)
	{
		msg.supplement = MTAKE(msg.supplementLength);
		if (msg.supplement == NULL)
		{
			putSysErrmsg(NoMemoryMemo, NULL);
			return -1;
		}

		memcpy(msg.supplement, supplement, msg.supplementLength);
	}
	else
	{
		msg.supplement = NULL;
	}

	evt = MTAKE(1 + sizeof(MamsMsg));
	if (evt == NULL)
	{
		MRELEASE(msg.supplement);
		putSysErrmsg(NoMemoryMemo, NULL);
		return -1;
	}

	evt->type = MAMS_MSG_EVT;
	memcpy(evt->value, (char *) &msg, sizeof(MamsMsg));
	return enqueueMamsEvent(eventsQueue, evt, msg.supplement, msg.memo);
}

int	enqueueMamsCrash(Llcv eventsQueue, char *text)
{
	int	textLength;
	char	*silence = "";
	AmsEvt	*evt;

	if (text == NULL)
	{
		textLength = 0;
		text = silence;
	}
	else
	{
		textLength = strlen(text);
	}

	evt = (AmsEvt *) MTAKE(1 + textLength + 1);
	if (evt == NULL)
	{
		putSysErrmsg(NoMemoryMemo, NULL);
		return -1;
	}

	evt->type = CRASH_EVT;
	memcpy(evt->value, text, textLength);
	evt->value[textLength] = '\0';
	if (enqueueMamsEvent(eventsQueue, evt, NULL, 0))
	{
		MRELEASE(evt);
		putErrmsg(NoMemoryMemo, NULL);
		return -1;
	}

	return 0;
}

int	enqueueMamsStubEvent(Llcv eventsQueue, int eventType)
{
	AmsEvt	*evt;

	evt = (AmsEvt *) MTAKE(sizeof(AmsEvt));
	if (evt == NULL)
	{
		putSysErrmsg(NoMemoryMemo, NULL);
		return -1;
	}

	evt->type = eventType;
	if (enqueueMamsEvent(eventsQueue, evt, NULL, 0))
	{
		MRELEASE(evt);
		putErrmsg(NoMemoryMemo, NULL);
		return -1;
	}

	return 0;
}

void	recycleEvent(AmsEvt *evt)
{
	MamsMsg	*msg;

	if (evt == NULL) return;
	if (evt->type == MAMS_MSG_EVT)
	{
		msg = (MamsMsg *) (evt->value);
		if (msg->supplement)
		{
			MRELEASE(msg->supplement);
		}
	}

	MRELEASE(evt);
}

void	destroyEvent(LystElt elt, void *userdata)
{
	AmsEvt	*evt = (AmsEvt *) lyst_data(elt);

	recycleEvent(evt);
}
