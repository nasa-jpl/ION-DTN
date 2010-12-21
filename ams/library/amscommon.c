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

extern void	destroyAmsEndpoint(LystElt elt, void *userdata);

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

	if (contin->description)
	{
		MRELEASE(contin->description);
	}

	MRELEASE(contin);
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

void	eraseMsgspace(Venture *venture, Subject *msgspace)
{
	if (venture == NULL || msgspace == NULL)
	{
		return;
	}

	if (msgspace->gwEid)
	{
		MRELEASE(msgspace->gwEid);
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
	for (i = 1; i <= MAX_MODULE_NBR; i++)
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

void	eraseVenture(Venture *venture)
{
	int	i;

	if (venture == NULL)
	{
		return;
	}

	for (i = 1; i <= MAX_ROLE_NBR; i++)
	{
		if (venture->roles[i])
		{
			eraseRole(venture, venture->roles[i]);
		}
	}

	for (i = 1; i <= MAX_SUBJ_NBR; i++)
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
	for (i = 1; i <= MAX_CONTIN_NBR; i++)
	{
		if (venture->msgspaces[i])
		{
			eraseMsgspace(venture, venture->msgspaces[i]);
		}
	}

	(_mib(NULL))->ventures[venture->nbr] = NULL;
	MRELEASE(venture);
}

static void	eraseMib(AmsMib *mib)
{
	int	i;

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

	for (i = 1; i <= MAX_CONTIN_NBR; i++)
	{
		if (mib->continua[i])
		{
			eraseContinuum(mib->continua[i]);
		}
	}

	for (i = 1; i <= MAX_VENTURE_NBR; i++)
	{
		if (mib->ventures[i])
		{
			eraseVenture(mib->ventures[i]);
		}
	}

	MRELEASE(mib);
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

static void	eraseAmsEpspec(AmsEpspec *amses)
{
	MRELEASE(amses);
}

static void	destroyAmsEpspec(LystElt elt, void *userdata)
{
	AmsEpspec	*amses = (AmsEpspec *) lyst_data(elt);

	eraseAmsEpspec(amses);
}

static void	destroyApplication(LystElt elt, void *userdata)
{
	AmsApp	*app = (AmsApp *) lyst_data(elt);

	eraseApp(app);
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

static void	addTs(AmsMib *mib, TsLoadFn loadTs)
{
	int		idx;
	TransSvc	*ts;

	idx = mib->transportServiceCount;
	ts = &(mib->transportServices[idx]);
	loadTs(ts);	/*	Execute the transport service loader.	*/
	mib->transportServiceCount++;
}

static int	initializeMib(AmsMib *mib, int continuumNbr, char *ptsName,
			int pubkeylen, char *pubkey, int privkeylen,
			char *privkey)
{
	int			amsMemory = getIonMemoryMgr();
	int			i;

	/*	Note: transport service loaders in this table appear
	 *	in descending order of preference.  "Preference"
	 *	corresponds broadly to nominal throughput rate.		*/

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

	static int		transportServiceCount =
					sizeof transportServiceLoaders /
					sizeof(TsLoadFn);
       
	if (transportServiceCount > TS_INDEX_LIMIT)
	{
		putErrmsg("Transport service table overflow.", NULL);
		return -1;
	}

	for (i = 0; i < transportServiceCount; i++)
	{
		addTs(mib, transportServiceLoaders[i]);
		if (strcmp(mib->transportServices[i].name, ptsName) == 0)
		{
			mib->pts = &(mib->transportServices[i]);
		}
	}

	if (mib->pts == NULL)
	{
		putErrmsg("No loader for primary transport service.", ptsName);
		return -1;
	}

	if (pthread_mutex_init(&(mib->mutex), NULL))
	{
		putSysErrmsg("MIB mutex init failed", NULL);
		return -1;
	}

	mib->amsEndpointSpecs = lyst_create_using(amsMemory);
	CHKERR(mib->amsEndpointSpecs);
	lyst_delete_set(mib->amsEndpointSpecs, destroyAmsEpspec, NULL);
	mib->applications = lyst_create_using(amsMemory);
	CHKERR(mib->applications);
	lyst_delete_set(mib->applications, destroyApplication, NULL);
	mib->csEndpoints = lyst_create_using(amsMemory);
	CHKERR(mib->csEndpoints);
	lyst_delete_set(mib->csEndpoints, destroyCsEndpoint, NULL);
	mib->localContinuumNbr = continuumNbr;
	if (pubkey)
	{
		mib->csPublicKey = MTAKE(pubkeylen);
		CHKERR(mib->csPublicKey);
		memcpy(mib->csPublicKey, pubkey, pubkeylen);
	}

	if (privkey)
	{
		mib->csPrivateKey = MTAKE(privkeylen);
		CHKERR(mib->csPrivateKey);
		memcpy(mib->csPrivateKey, privkey, privkeylen);
	}

	if (createContinuum(continuumNbr, "local", 0, "this continuum") == NULL)
	{
		putErrmsg("Can't create local continuum object.", NULL);
		return -1;
	}

	mib->csPublicKeyLength = pubkeylen;
	mib->csPrivateKeyLength = privkeylen;
	return 0;
}

static int	initializeMemMgt(int continuumNbr)
{
	IonParms	ionParms;

	if (ionAttach() == 0)
	{
		return 0;		/*	ION is already started.	*/
	}

	writeMemo("[i] ION not started yet.  Starting ION from inside AMS.");
	if (readIonParms(NULL, &ionParms) < 0)
	{
		putErrmsg("AMS can't load ION parameters.", NULL);
		return -1;
	}

	if (ionInitialize(&ionParms, continuumNbr) < 0)
	{
		putErrmsg("AMS can't start ION.", itoa(continuumNbr));
		return -1;
	}

	return 0;
}

AmsMib	*_mib(AmsMibParameters *parms)
{
	static AmsMib	*mib = NULL;

	if (parms)
	{
		if (parms->continuumNbr == 0)	/*	Terminating.	*/
		{
			if (mib)
			{
				eraseMib(mib);
				mib = NULL;
			}
		}
		else				/*	Initializing.	*/
		{
			if (mib)
			{
				writeMemo("[?] AMS MIB already created.");
			}
			else
			{
				CHKNULL(parms->continuumNbr > 0);
				CHKNULL(parms->continuumNbr <= MAX_CONTIN_NBR);
				CHKNULL(parms->ptsName);
				CHKNULL(parms->publicKeyLength == 0
					|| (parms->publicKeyLength > 0
						&& parms->publicKey != NULL));
				CHKNULL(parms->privateKeyLength == 0
					|| (parms->privateKeyLength > 0
						&& parms->privateKey != NULL));
				if (initializeMemMgt(parms->continuumNbr) < 0)
				{
					putErrmsg("Can't attach to ION.", NULL);
					return NULL;
				}

				mib = (AmsMib *) MTAKE(sizeof(AmsMib));
				CHKNULL(mib);
				memset((char *) mib, 0, sizeof(AmsMib));
				if (initializeMib(mib, parms->continuumNbr,
						parms->ptsName,
						parms->publicKeyLength,
						parms->publicKey,
						parms->privateKeyLength,
						parms->privateKey) < 0)
				{
					putErrmsg("Can't create MIB.", NULL);
					eraseMib(mib);
					mib = NULL;
				}
			}
		}
	}

	return mib;
}

char	*_rejectionMemos(int idx)
{
	static char	*memos[] =	{
				"not rejected",
				"duplicate registrar",
				"no cell census",
				"cell is full",
				"no such unit",
				"unknown"
					};
	if (idx < 0 || idx > 4)
	{
		return memos[5];
	}

	return memos[idx];
}

/*	*	*	Checksum computation	*	*	*	*/

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

	for (elt = lyst_first((_mib(NULL))->applications); elt;
			elt = lyst_next(elt))
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

	for (i = 1; i <= MAX_ROLE_NBR; i++)
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

	for (i = 1; i <= MAX_VENTURE_NBR; i++)
	{
		venture = (_mib(NULL))->ventures[i];
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

	for (i = 0; i <= MAX_UNIT_NBR; i++)
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

	for (i = 1; i <= MAX_CONTIN_NBR; i++)
	{
		continuum = (_mib(NULL))->continua[i];
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

static int	getAuthenticationParms(int ventureNbr, int unitNbr, int roleNbr,
			Venture **venture, Unit **unit, int sending,
			char **authName, char **authKey, int *authKeyLen)
{
	AmsMib	*mib = _mib(NULL);
	AppRole	*role = NULL;

	*venture = NULL;
	*unit = NULL;
	*authName = NULL;
	if (ventureNbr > 0)
	{
		if (ventureNbr > MAX_VENTURE_NBR
		|| (*venture = mib->ventures[ventureNbr]) == NULL)
		{
			writeMemoNote("[?] MAMS msg from unknown msgspace",
					itoa(ventureNbr));
			return 0;
		}

		if (unitNbr > MAX_UNIT_NBR
		|| (*unit = (*venture)->units[unitNbr]) == NULL)
		{
			writeMemoNote("[?] MAMS msg from unknown cell",
					itoa(unitNbr));
			return 0;
		}
	}

	if (roleNbr > 0)
	{
		if (roleNbr > MAX_ROLE_NBR || *venture == NULL
		|| (role = (*venture)->roles[roleNbr]) == NULL)
		{
			writeMemoNote("[?] MAMS message dropped; unknown role",
					itoa(roleNbr));
			return 0;
		}
	}

	if (role)		/*	Sender is module.		*/
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
	if (atSign == NULL)			/*	Malformed.	*/
	{
		return protocol;
	}

	*gwEid = atSign + 1;
	if (**gwEid == '\0'	/*	Endpoint ID is a NULL string.	*/
	|| strlen(*gwEid) > MAX_GW_EID)		/*	Invalid.	*/
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

	CHKNULL(subj);
	CHKNULL(name);
	length = strlen(name);
	CHKNULL(length <= MAX_ELEM_NAME);
	CHKNULL(name);
	element = (MsgElement *) MTAKE(sizeof(MsgElement));
	CHKNULL(element);
	memset((char *) element, 0, sizeof(MsgElement));
	element->type = type;
	nameLen = length + 1;
	element->name = MTAKE(nameLen);
	CHKNULL(element->name);
	if (description)
	{
		descLen = strlen(description) + 1;
		element->description = MTAKE(descLen);
		CHKNULL(element->description);
	}

	istrcpy(element->name, name, nameLen);
	if (description && descLen > 1)
	{
		istrcpy(element->description, description, descLen);
	}

	elt = lyst_insert_last(subj->elements, element);
	CHKNULL(elt);
	return elt;
}

static void	destroyFanModule(LystElt elt, void *userdata)
{
	FanModule	*fan = (FanModule *) lyst_data(elt);

	MRELEASE(fan);
}

Subject	*createSubject(Venture *venture, int nbr, char *name, char *description,
		char *symmetricKey, int symmetricKeyLength)
{
	int	amsMemory = getIonMemoryMgr();
	int	length;
	Subject	*subj;
	int	nameLen;
	int	descLen = 0;
	int	idx;

	CHKNULL(venture);
	CHKNULL(nbr >= 0);
	CHKNULL(nbr <= MAX_SUBJ_NBR);
	CHKNULL(venture->subjects[nbr] == NULL);
	CHKNULL(name);
	length = strlen(name);
	CHKNULL(length <= MAX_SUBJ_NAME);
	CHKNULL(symmetricKeyLength == 0
		|| (symmetricKeyLength > 0 && symmetricKey != NULL));
	subj = (Subject *) MTAKE(sizeof(Subject));
	CHKNULL(subj);
	memset((char *) subj, 0, sizeof(Subject));
	subj->nbr = nbr;
	subj->isContinuum = 0;
	nameLen = length + 1;
	subj->name = MTAKE(nameLen);
	CHKNULL(subj->name);
	istrcpy(subj->name, name, nameLen);
	if (description)
	{
		descLen = strlen(description) + 1;
		subj->description = MTAKE(descLen);
		CHKNULL(subj->description);
		istrcpy(subj->description, description, descLen);
	}

	if (symmetricKey)
	{
		subj->symmetricKey = MTAKE(symmetricKeyLength);
		CHKNULL(subj->symmetricKey);
		memcpy(subj->symmetricKey, symmetricKey, symmetricKeyLength);
	}

	subj->elements = lyst_create_using(amsMemory);
	CHKNULL(subj->elements);
/*
For future use:
	subj->authorizedSenders = lyst_create_using(amsMemory);
	CHKNULL(subj->authorizedSenders);
	subj->authorizedReceivers = lyst_create_using(amsMemory);
	CHKNULL(subj->authorizedReceivers);
*/
	subj->modules = lyst_create_using(amsMemory);
	CHKNULL(subj->modules);
	subj->keyLength = symmetricKeyLength;
	lyst_delete_set(subj->elements, destroyElement, NULL);
	lyst_delete_set(subj->modules, destroyFanModule, NULL);
	venture->subjects[nbr] = subj;
	idx = hashSubjectName(name);
	subj->elt = lyst_insert_last(venture->subjLysts[idx], subj);
       	CHKNULL(subj->elt);
	return subj;
}

AppRole	*createRole(Venture *venture, int nbr, char *name, char *publicKey,
		int publicKeyLength, char *privateKey, int privateKeyLength)
{
	int	length;
	AppRole	*role;
	int	nameLen;

	CHKNULL(venture);
	CHKNULL(nbr > 0);
	CHKNULL(nbr <= MAX_ROLE_NBR);
	CHKNULL(venture->roles[nbr] == NULL);
	CHKNULL(name);
	length = strlen(name);
	CHKNULL(length <= MAX_ROLE_NAME);
	CHKNULL(publicKeyLength == 0
			|| (publicKeyLength > 0 && publicKey != NULL));
	CHKNULL(privateKeyLength == 0
			|| (privateKeyLength > 0 && privateKey != NULL));
	role = (AppRole *) MTAKE(sizeof(AppRole));
	CHKNULL(role);
	memset((char *) role, 0, sizeof(AppRole));
	role->nbr = nbr;
	nameLen = length + 1;
	role->name = MTAKE(nameLen);
	CHKNULL(role->name);
	istrcpy(role->name, name, nameLen);
	role->publicKeyLength = publicKeyLength;
	if (publicKey)
	{
		role->publicKey = MTAKE(publicKeyLength);
		CHKNULL(role->publicKey);
		memcpy(role->publicKey, publicKey, publicKeyLength);
	}

	role->privateKeyLength = privateKeyLength;
	if (privateKey)
	{
		role->privateKey = MTAKE(privateKeyLength);
		CHKNULL(role->privateKey);
		memcpy(role->privateKey, privateKey, privateKeyLength);
	}

	venture->roles[nbr] = role;
	return role;
}

LystElt	createApp(char *name, char *publicKey, int publicKeyLength,
		char *privateKey, int privateKeyLength)
{
	int	length;
	AmsApp	*app;
	int	nameLen;
	LystElt	elt;

	CHKNULL(name);
	length = strlen(name);
	CHKNULL(length <= MAX_APP_NAME);
	CHKNULL(publicKeyLength == 0
			|| (publicKeyLength > 0 && publicKey != NULL));
	CHKNULL(privateKeyLength == 0
			|| (privateKeyLength > 0 && privateKey != NULL));
	app = (AmsApp *) MTAKE(sizeof(AmsApp));
	CHKNULL(app);
	memset((char *) app, 0, sizeof(AmsApp));
	nameLen = length + 1;
	app->name = MTAKE(nameLen);
	CHKNULL(app->name);
	istrcpy(app->name, name, nameLen);
	app->publicKeyLength = publicKeyLength;
	if (publicKey)
	{
		app->publicKey = MTAKE(publicKeyLength);
		CHKNULL(app->publicKey);
		memcpy(app->publicKey, publicKey, publicKeyLength);
	}

	app->privateKeyLength = privateKeyLength;
	if (privateKey)
	{
		app->privateKey = MTAKE(privateKeyLength);
		CHKNULL(app->privateKey);
		memcpy(app->privateKey, privateKey, privateKeyLength);
	}

	elt = lyst_insert_last((_mib(NULL))->applications, app);
	CHKNULL(elt);
	return elt;
}

Subject	*createMsgspace(Venture *venture, int continNbr, char *gwEidString,
		char *symmetricKey, int symmetricKeyLength)
{
	int		amsMemory = getIonMemoryMgr();
	Subject		*msgspace;
	RamsNetProtocol	ramsProtocol;
	char		*gwEid;
	char		gwEidBuffer[MAX_GW_EID + 1];
	int		eidLen;

	CHKNULL(venture);
	CHKNULL(continNbr > 0);
	CHKNULL(continNbr <= MAX_CONTIN_NBR);
	CHKNULL((_mib(NULL))->continua[continNbr] != NULL);
	CHKNULL(venture->msgspaces[continNbr] == NULL);
	CHKNULL(symmetricKeyLength == 0
		|| (symmetricKeyLength > 0 && symmetricKey != NULL));
	msgspace = (Subject *) MTAKE(sizeof(Subject));
	CHKNULL(msgspace);
	memset((char *) msgspace, 0, sizeof(Subject));
	msgspace->nbr = 0 - continNbr;	/*	Negative subj number.	*/
	msgspace->isContinuum = 1;
	ramsProtocol = parseGwEid(gwEidString, &gwEid, gwEidBuffer);
	if (ramsProtocol == RamsNoProtocol)
	{
		ramsProtocol = RamsBp;
		isprintf(gwEidBuffer, sizeof gwEidBuffer, "ipn:%d.%d",
				continNbr, venture->nbr);
		gwEid = gwEidBuffer;
	}

	eidLen = strlen(gwEid) + 1;
	msgspace->gwEid = MTAKE(eidLen);
	CHKNULL(msgspace->gwEid);
	istrcpy(msgspace->gwEid, gwEid, eidLen);
	msgspace->keyLength = symmetricKeyLength;
	if (symmetricKey)
	{
		msgspace->symmetricKey = MTAKE(symmetricKeyLength);
		CHKNULL(msgspace->symmetricKey);
		memcpy(msgspace->symmetricKey, symmetricKey,
				symmetricKeyLength);
	}

	msgspace->elements = lyst_create_using(amsMemory);
	CHKNULL(msgspace->elements);
	lyst_delete_set(msgspace->elements, destroyElement, NULL);
#if 0
	msgspace->authorizedSenders = lyst_create_using(amsMemory);
	CHKNULL(msgspace->authorizedSenders);
	msgspace->authorizedReceivers = lyst_create_using(amsMemory);
	CHKNULL(msgspace->authorizedReceivers);
#endif
	msgspace->modules = lyst_create_using(amsMemory);
	CHKNULL(msgspace->modules);
	lyst_delete_set(msgspace->modules, destroyFanModule, NULL);
	venture->msgspaces[continNbr] = msgspace;
	return msgspace;
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
	int	amsMemory = getIonMemoryMgr();
	Module	*module;

	module = (Module *) MTAKE(sizeof(Module));
	CHKNULL(module);
	memset((char *) module, 0, sizeof(Module));
	module->unitNbr = cell->unit->nbr;
	module->nbr = moduleNbr;
	module->role = NULL;
	module->amsEndpoints = lyst_create_using(amsMemory);
	CHKNULL(module->amsEndpoints);
	lyst_delete_set(module->amsEndpoints, destroyAmsEndpoint, NULL);
	module->subjects = lyst_create_using(amsMemory);
	CHKNULL(module->subjects);
	lyst_delete_set(module->subjects, destroySubjOfInterest, NULL);
	cell->modules[moduleNbr] = module;
	return module;
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
	CHKNULL(unit);
	memset((char *) unit, 0, sizeof(Unit));
	unit->cell = &(unit->cellData);
	cell = unit->cell;
	cell->unit = unit;
	unit->nbr = nbr;
	nameLen = length + 1;
	unit->name = MTAKE(nameLen);
	CHKNULL(unit->name);
	istrcpy(unit->name, name, nameLen);

	/*	Initialize cell data of unit.				*/

	for (i = 1; i <= MAX_MODULE_NBR; i++)
	{
		if (createModule(cell, i) < 0)
		{
			eraseUnit(venture, unit);
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
		CHKNULL(unit->inclusionElt);
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

	CHKNULL(venture);
	CHKNULL(nbr > 0);
	CHKNULL(nbr <= MAX_UNIT_NBR);
	CHKNULL(venture->units[nbr] == NULL);
	CHKNULL(name);
	namelen = strlen(name);
	CHKNULL(namelen > 0);
	CHKNULL(namelen <= MAX_UNIT_NAME);
	CHKNULL(resyncPeriod >= 0);

	/*	Unit whose name is the longest prefix of the new
	 *	unit's name is the new unit's superunit.  Every
	 *	unit whose name is prefixed by the new unit's name
	 *	is a candidate subunit of the new unit.			*/

	subunits = lyst_create_using(getIonMemoryMgr());
	CHKNULL(subunits);
	bestLength = 0;
	superunit = venture->units[0];	/*	Default is root cell.	*/
	for (i = 0; i <= MAX_UNIT_NBR; i++)
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
		writeMemoNote("[?] Duplicate unit name", name);
		lyst_destroy(subunits);
		return NULL;
	}

	unit = initializeUnit(venture, nbr, name, namelen, resyncPeriod,
			superunit, subunits);
	if (unit == NULL)
	{
		lyst_destroy(subunits);
	}

	return unit;
}

Venture	*createVenture(int nbr, char *appname, char *authname,
		char *gwEidString, int ramsNetIsTree, int rootCellResyncPeriod)
{
	AmsMib		*mib = _mib(NULL);
	int		amsMemory = getIonMemoryMgr();
	int		length;
	LystElt		elt;
	AmsApp		*app;
	int		i;
	Venture		*venture;
	int		authnameLen;
	RamsNetProtocol	ramsProtocol;
	char		*gwEid;
	char		gwEidBuffer[MAX_GW_EID + 1];
	AppRole		*gatewayRole;
	Subject		*allSubjects;
	Lyst		subunits;
	Unit		*rootUnit;
	Subject		*msgspace;	/*	In local continuum.	*/

	CHKNULL(nbr > 0);
	CHKNULL(nbr <= MAX_VENTURE_NBR);
	CHKNULL(mib->ventures[nbr] == NULL);
	CHKNULL(appname);
	CHKNULL(authname);
	length = strlen(authname);
	CHKNULL(length <= MAX_AUTH_NAME);
	CHKNULL(rootCellResyncPeriod >= 0);
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
		writeMemoNote("[?] Adding venture for unknown application",
				appname);
		return NULL;
	}

	venture = (Venture *) MTAKE(sizeof(Venture));
	CHKNULL(venture);
	memset((char *) venture, 0, sizeof(Venture));
	venture->nbr = nbr;
	venture->app = app;
	authnameLen = length + 1;
	venture->authorityName = MTAKE(authnameLen);
	CHKNULL(venture->authorityName);
	istrcpy(venture->authorityName, authname, authnameLen);
	for (i = 0; i < SUBJ_LIST_CT; i++)
	{
		venture->subjLysts[i] = lyst_create_using(amsMemory);
		CHKNULL(venture->subjLysts[i]);
	}

	ramsProtocol = parseGwEid(gwEidString, &gwEid, gwEidBuffer);
	if (ramsProtocol == RamsNoProtocol)
	{
		ramsProtocol = RamsBp;
	}

	venture->gwProtocol = ramsProtocol;
	venture->ramsNetIsTree = ramsNetIsTree;
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

	allSubjects = createSubject(venture, 0, "everything",
			"messages on all subjects", NULL, 0);
	if (allSubjects == NULL)
	{
		eraseVenture(venture);
		putErrmsg("Can't create 'everything' subject for venture.",
				appname);
		return NULL;
	}

	/*	Automatically create venture's root unit.		*/

	subunits = lyst_create_using(amsMemory);
	CHKNULL(subunits);
	rootUnit = initializeUnit(venture, 0, "", 0, rootCellResyncPeriod,
			NULL, subunits);
	if (rootUnit == NULL)
	{
		putErrmsg("Can't initialize root unit for venture.", appname);
		lyst_destroy(subunits);
		eraseVenture(venture);
		return NULL;
	}

	/*	Automatically create the local message space.		*/

	msgspace = createMsgspace(venture, mib->localContinuumNbr, gwEidString,
			NULL, 0);
	if (msgspace == NULL)
	{
		putErrmsg("Can't create local msgspace for venture.", appname);
		eraseVenture(venture);
		return NULL;
	}

	return venture;
}

Continuum	*createContinuum(int nbr, char *name, int isNeighbor,
			char *description)
{
	AmsMib		*mib = _mib(NULL);
	int		length;
	Continuum	*contin;
	int		nameLen;
	int		descLen = 0;

	CHKNULL(nbr > 0);
	CHKNULL(nbr <= MAX_CONTIN_NBR);
	CHKNULL(mib->continua[nbr] == NULL);
	CHKNULL(name);
	length = strlen(name);
	CHKNULL(length <= MAX_SUBJ_NAME);
	contin = (Continuum *) MTAKE(sizeof(Continuum));
	CHKNULL(contin);
	memset((char *) contin, 0, sizeof(Continuum));
	contin->nbr = nbr;
	contin->isNeighbor = 1 - (isNeighbor == 0);
	nameLen = length + 1;
	contin->name = MTAKE(nameLen);
	CHKNULL(contin->name);
	istrcpy(contin->name, name, nameLen);
	if (description)
	{
		descLen = strlen(description) + 1;
		contin->description = MTAKE(descLen);
		CHKNULL(contin->description);
		istrcpy(contin->description, description, descLen);
	}

	mib->continua[nbr] = contin;
	return contin;
}

LystElt	createCsEndpoint(char *endpointSpec, LystElt nextElt)
{
	AmsMib		*mib = _mib(NULL);
	int		len;
	char		endpointName[MAX_EP_NAME + 1];
	MamsEndpoint	*ep;
	LystElt		elt;

	CHKNULL(endpointSpec);
	if (mib->pts->csepNameFn(endpointSpec, endpointName) < 0)
	{
		putErrmsg("Can't compute CS endpoint name.", endpointSpec);
		return NULL;
	}

	len = strlen(endpointName);
	ep = (MamsEndpoint *) MTAKE(sizeof(MamsEndpoint));
	CHKNULL(ep);
	memset((char *) ep, 0, sizeof(MamsEndpoint));
	if (constructMamsEndpoint(ep, len, endpointName) < 0)
	{
		putErrmsg("Can't construct MAMS endpoint for CS.", NULL);
		eraseCsEndpoint(ep);
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

	CHKNULL(elt);
	return elt;
}

LystElt	createAmsEpspec(char *tsname, char *epspec)
{
	AmsMib		*mib = _mib(NULL);
	int		i;
	TransSvc	*ts;
	AmsEpspec	amses;
	AmsEpspec	*amsesPtr;
	LystElt		elt;

	CHKNULL(tsname);
	CHKNULL(epspec);
	if (strlen(epspec) > MAX_EP_SPEC)
	{
		writeMemoNote("[?] AMS endpoint spec is too long", epspec);
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
		writeMemoNote("[?] Unknown transport service for AMS endpoint \
spec", tsname);
		return NULL;
	}

	amsesPtr = (AmsEpspec *) MTAKE(sizeof(AmsEpspec));
	CHKNULL(amsesPtr);
	memcpy((char *) amsesPtr, &amses, sizeof(AmsEpspec));
	elt = lyst_insert_last(mib->amsEndpointSpecs, amsesPtr);
	CHKNULL(elt);
	return elt;
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

	CHKERR(roleNbr);
	CHKERR(unitNbr);
	CHKERR(moduleNbr);
	*roleNbr = (moduleId >> 24) & 0x000000ff;
	if (*roleNbr < 1 || *roleNbr > MAX_ROLE_NBR)
	{
		writeMemoNote("[?] Role nbr invalid", itoa(*roleNbr));
		return -1;
	}

	*unitNbr = (moduleId >> 8) & 0x0000ffff;
	if (*unitNbr > MAX_UNIT_NBR)
	{
		writeMemoNote("[?] Unit nbr invalid", itoa(*unitNbr));
		return -1;
	}

	*moduleNbr = moduleId & 0x000000ff;
	if (*moduleNbr < 1 || *moduleNbr > MAX_MODULE_NBR)
	{
		writeMemoNote("[?] Module nbr invalid", itoa(*moduleNbr));
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
	CHKERR(endpoint->ept);
	memcpy(endpoint->ept, ept, eptLength);
	endpoint->ept[eptLength] = '\0';

	/*	The primary transport service's endpoint parsing
	 *	function examines the endpoint name text and fills
	 *	in the tsep of the endpoint.				*/

	if ((((_mib(NULL))->pts->parseMamsEndpointFn)(endpoint)) < 0)
	{
		writeMemoNote("[?] Can't parse endpoint name", ept);
		return -1;
	}

	return 0;
}

void	clearMamsEndpoint(MamsEndpoint *ep)
{
	(_mib(NULL))->pts->clearMamsEndpointFn(ep);
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

	CHKERR(endpoint);
	CHKERR(tsif);
	if (getAuthenticationParms(tsif->ventureNbr, tsif->unitNbr,
			tsif->roleNbr, &venture, &unit, 1, &authName, &authKey,
			&authKeyLen))
	{
		putErrmsg("Can't get authentication parameters.", NULL);
		return -1;
	}

	if (authName == NULL)		/*	Invalid parameters.	*/
	{
		return 0;		/*	Don't send message.	*/
	}
	
	if (authKey && authKeyLen > 0)
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
	CHKERR(msg);

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

	result = (_mib(NULL))->pts->sendMamsFn(endpoint, tsif, (char *) msg,
			msgLength);
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
			putErrmsg("Can't insert event.", NULL);
			return -1;
		}

		llcv_signal(eventsQueue, time_to_stop);
		return 0;
	}

	/*	A hack, which works in the absence of a Lyst user
	 *	data variable.  In order to make the current query
	 *	ID number accessible to an llcv condition function,
	 *	we stuff it into the "compare" function pointer in
	 *	the Lyst structure.					*/

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
		putErrmsg("Can't insert event.", NULL);
		MRELEASE(evt);
		if (ancillaryBlock)
		{
			MRELEASE(ancillaryBlock);
		}

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
	unsigned char	*authenticator = NULL;
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

	CHKERR(eventsQueue);
	CHKERR(length >= 0);
	CHKERR(msgBuffer);
	if (length < 14)
	{
		writeMemoNote("[?] MAMS msg header incomplete", itoa(length));
		return -1;
	}

	/*	First get time tag, since its size affects the
	 *	location of the authenticator and supplement.		*/

	preamble = *(msgBuffer + 12);		/*	"P-field."	*/
	timeCode = (preamble >> 4) & 0x07;
	if (timeCode != 1)
	{
		writeMemoNote("[?] Unknown time code in MAMS msg header",
				itoa(timeCode));
		return -1;
	}

	coarseTimeLength = ((preamble >> 2) & 0x03) + 1;
	fineTimeLength = preamble & 0x03;
	expectedLength = 12 + 1 + coarseTimeLength + fineTimeLength;
	if (length < expectedLength)
	{
		writeMemo("[?] MAMS message truncated.");
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
			writeMemo("[?] MAMS message truncated.");
			return -1;
		}

		memcpy((char *) &checksum, msgBuffer + (length - 2), 2);
		checksum = ntohs(checksum);
		if (checksum != computeAmsChecksum(msgBuffer, length - 2))
		{
			writeMemo("[?] Checksum failed, MAMS msg discarded.");
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
		writeMemoNote("[?] MAMS message length error", itoa(length));
		return -1;
	}

	if (getAuthenticationParms(msg.ventureNbr, msg.unitNbr, msg.roleNbr,
			&venture, &unit, 0, &authName, &authKey, &authKeyLen))
	{
		putErrmsg("Can't get authentication parameters.", NULL);
		return -1;
	}

	if (authName == NULL)		/*	Invalid parameters.	*/
	{
		return 0;		/*	Don't send message.	*/
	}
	
	if (authKey && authKeyLen > 0)
	{
		/*	Authentication is required.			*/

		authNameLen = strlen(authName);
		CHKERR(authNameLen > 0);
		CHKERR(authNameLen <= (AUTHENTICAT_LEN - 9));
		if (authenticatorLength != authNameLen + 8)
		{
			writeMemo("[?] MAMS msg discarded; bad authenticator.");
			return -1;
		}

		memcpy(nonce, authenticator, 4);
		decryptUsingPublicKey((char *) (authenticator + 4),
				authenticatorLength - 4, authKey, authKeyLen,
				(char *) (authenticator + 4), &encryptLength);
		if (memcmp(nonce, authenticator + 4, 4) != 0
		|| memcmp(authName, authenticator + 8, authNameLen) != 0)
		{
			writeMemo("[?] MAMS msg discarded; auth. failed.");
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
		CHKERR(msg.supplement);
		memcpy(msg.supplement, supplement, msg.supplementLength);
	}
	else
	{
		msg.supplement = NULL;
	}

	evt = MTAKE(1 + sizeof(MamsMsg));
	CHKERR(evt);
	evt->type = MAMS_MSG_EVT;
	memcpy(evt->value, (char *) &msg, sizeof(MamsMsg));
	if (enqueueMamsEvent(eventsQueue, evt, msg.supplement, msg.memo))
	{
		putErrmsg("Can't enqueue MAMS message.", NULL);
		MRELEASE(evt);
		return -1;
	}

	return 0;
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
	CHKERR(evt);
	evt->type = CRASH_EVT;
	memcpy(evt->value, text, textLength);
	evt->value[textLength] = '\0';
	if (enqueueMamsEvent(eventsQueue, evt, NULL, 0))
	{
		putErrmsg("Can't enqueue MAMS crash.", NULL);
		MRELEASE(evt);
		return -1;
	}

	return 0;
}

int	enqueueMamsStubEvent(Llcv eventsQueue, int eventType)
{
	AmsEvt	*evt;

	evt = (AmsEvt *) MTAKE(sizeof(AmsEvt));
	CHKERR(evt);
	evt->type = eventType;
	if (enqueueMamsEvent(eventsQueue, evt, NULL, 0))
	{
		putErrmsg("Can't enqueue stub event.", NULL);
		MRELEASE(evt);
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
