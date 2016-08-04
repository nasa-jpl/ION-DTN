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

	if (subj->symmetricKeyName)
	{
		MRELEASE(subj->symmetricKeyName);
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

	if (role->publicKeyName)
	{
		MRELEASE(role->publicKeyName);
	}

	if (role->privateKeyName)
	{
		MRELEASE(role->privateKeyName);
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

	if (msgspace->symmetricKeyName)
	{
		MRELEASE(msgspace->symmetricKeyName);
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

		lyst_destroy(module->amsEndpoints);
	}

	if (module->subjects)
	{
		for (elt = lyst_first(module->subjects); elt; elt = nextElt)
		{
			nextElt = lyst_next(elt);
			lyst_delete(elt);
		}

		lyst_destroy(module->subjects);
	}

	clearMamsEndpoint(&module->mamsEndpoint);
	module->role = NULL;
	MRELEASE(module);
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
		else
		{
			subunit->inclusionElt = NULL;
		}

		subunit->superunit = superunit;
	}

	lyst_destroy(unit->subunits);

	/*	Detach erased unit from its own superunit.		*/

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

	lockMib();
	if (venture->authorityName)
	{
		MRELEASE(venture->authorityName);
	}

	for (i = 0; i <= MAX_UNIT_NBR; i++)
	{
		eraseUnit(venture, venture->units[i]);
	}

	for (i = 1; i <= MAX_ROLE_NBR; i++)
	{
		if (venture->roles[i])
		{
			eraseRole(venture, venture->roles[i]);
		}
	}

	for (i = 0; i <= MAX_SUBJ_NBR; i++)
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

	for (i = 1; i <= MAX_CONTIN_NBR; i++)
	{
		if (venture->msgspaces[i])
		{
			eraseMsgspace(venture, venture->msgspaces[i]);
		}
	}

	(_mib(NULL))->ventures[venture->nbr] = NULL;
	MRELEASE(venture);
	unlockMib();
}

static void	eraseMib(AmsMib *mib)
{
	int	i;

	/*	Note: no need to lockMib() because this function
	 *	is called only from inside _mib(), which has
	 *	already called lockMib().				*/

	if (mib->csPublicKeyName)
	{
		MRELEASE(mib->csPublicKeyName);
	}

	if (mib->csPrivateKeyName)
	{
		MRELEASE(mib->csPrivateKeyName);
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

static void	eraseAmsEpspec(AmsEpspec *amses)
{
	MRELEASE(amses);
}

static void	destroyAmsEpspec(LystElt elt, void *userdata)
{
	AmsEpspec	*amses = (AmsEpspec *) lyst_data(elt);

	eraseAmsEpspec(amses);
}

void	eraseApp(AmsApp *app)
{
	if (app == NULL)
	{
		return;
	}

	if (app->name)
	{
		MRELEASE(app->name);
	}

	if (app->publicKeyName)
	{
		MRELEASE(app->publicKeyName);
	}

	if (app->privateKeyName)
	{
		MRELEASE(app->privateKeyName);
	}

	MRELEASE(app);
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

	/*	Note: no need to lockMib() because this function
	 *	is called only from inside initializeMib(), which
	 *	is only called from inside _mib(), which has
	 *	already called lockMib().				*/

	idx = mib->transportServiceCount;
	ts = &(mib->transportServices[idx]);
	loadTs(ts);	/*	Execute the transport service loader.	*/
	mib->transportServiceCount++;
}

static int	initializeMib(AmsMib *mib, int continuumNbr, char *ptsName,
			char *pubkeyname, char *privkeyname)
{
	int			amsMemory = getIonMemoryMgr();
	int			i;
	int			length;

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

	/*	Note: no need to lockMib() because this function
	 *	is called only from inside _mib(), which has already
	 *	called lockMib().					*/

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
	if (pubkeyname)
	{
		length = strlen(pubkeyname) + 1;
		mib->csPublicKeyName = MTAKE(length);
		CHKERR(mib->csPublicKeyName);
		memcpy(mib->csPublicKeyName, pubkeyname, length);
	}

	if (privkeyname)
	{
		length = strlen(privkeyname) + 1;
		mib->csPrivateKeyName = MTAKE(length);
		CHKERR(mib->csPrivateKeyName);
		memcpy(mib->csPrivateKeyName, privkeyname, length);
	}

	if (createContinuum(continuumNbr, "local", "this continuum") == NULL)
	{
		putErrmsg("Can't create local continuum object.", NULL);
		return -1;
	}

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
		ionDetach();
		return -1;
	}

	if (ionInitialize(&ionParms, continuumNbr) < 0)
	{
		putErrmsg("AMS can't start ION.", itoa(continuumNbr));
		ionDetach();
		return -1;
	}

	return 0;
}

static void	_mibLock(int lock)
{
	static ResourceLock	mibLock;

	/*	The MIB is shared among threads, so access to it must
	 *	be mutexed.						*/

	if (initResourceLock(&mibLock) == 0)
	{
		switch (lock)
		{
		case -1:
			killResourceLock(&mibLock);
			break;

		case 0:
			unlockResource(&mibLock);
			break;

		default:
			lockResource(&mibLock);
		}
	}
}

void	lockMib()
{
	_mibLock(1);
}

void	unlockMib()
{
	_mibLock(0);
}

AmsMib	*_mib(AmsMibParameters *parms)
{
	static AmsMib	*mib = NULL;

	if (parms)
	{
		lockMib();
		if (parms->continuumNbr == 0)	/*	Terminating.	*/
		{
			if (mib)
			{
				eraseMib(mib);
				mib = NULL;
			}

			_mibLock(-1);
			return mib;
		}
		else				/*	Initializing.	*/
		{
			if (mib)
			{
				writeMemo("[i] AMS MIB already created.");
			}
			else
			{
				if (parms->continuumNbr < 1
				|| parms->continuumNbr > MAX_CONTIN_NBR
				|| parms->ptsName == NULL)
				{
					putErrmsg("Invalid MIB parms.", NULL);
					_mibLock(-1);
					return mib;
				}

				if (initializeMemMgt(parms->continuumNbr) < 0)
				{
					putErrmsg("Can't attach to ION.", NULL);
					_mibLock(-1);
					return mib;
				}

				mib = (AmsMib *) MTAKE(sizeof(AmsMib));
				if (mib == NULL)
				{
					putErrmsg("Can't create MIB.",
							parms->ptsName);
					ionDetach();
					_mibLock(-1);
					return mib;
				}

				memset((char *) mib, 0, sizeof(AmsMib));
				if (initializeMib(mib, parms->continuumNbr,
						parms->ptsName,
						parms->publicKeyName,
						parms->privateKeyName) < 0)
				{
					putErrmsg("Can't initialize MIB.",
							parms->ptsName);
					eraseMib(mib);
					ionDetach();
					mib = NULL;
					_mibLock(-1);
					return mib;
				}
			}
		}

		unlockMib();
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

static LystElt	findApplication(char *appName)
{
	LystElt	elt;
	AmsApp	*app;

	lockMib();
	for (elt = lyst_first((_mib(NULL))->applications); elt;
			elt = lyst_next(elt))
	{
		app = (AmsApp *) lyst_data(elt);
		if (strcmp(app->name, appName) == 0)
		{
			unlockMib();
			return elt;
		}
	}

	unlockMib();
	return NULL;
}

AmsApp	*lookUpApplication(char *appName)
{
	LystElt	elt;

	elt = findApplication(appName);
	if (elt)
	{
		return (AmsApp *) lyst_data(elt);
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

Venture	*lookUpVenture(char *appName, char *authName)
{
	int	i;
	Venture	*venture;

	if (appName == NULL || authName == NULL)
	{
		return NULL;
	}

	lockMib();
	for (i = 1; i <= MAX_VENTURE_NBR; i++)
	{
		venture = (_mib(NULL))->ventures[i];
		if (venture == NULL)	/*	Number not in use.	*/
		{
			continue;
		}

		if (strcmp(venture->app->name, appName) == 0
		&& strcmp(venture->authorityName, authName) == 0)
		{
			unlockMib();
			return venture;
		}
	}

	unlockMib();
	return NULL;
}

Unit	*lookUpUnit(Venture *venture, char *unitName)
{
	int	i;
	Unit	*unit;

	for (i = 0; i <= MAX_UNIT_NBR; i++)
	{
		unit = venture->units[i];
		if (unit == NULL)	/*	Number not in use.	*/
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

	lockMib();
	for (i = 1; i <= MAX_CONTIN_NBR; i++)
	{
		continuum = (_mib(NULL))->continua[i];
		if (continuum == NULL)
		{
			continue;	/*	Undefined continuum.	*/
		}

		if (strcmp(continuum->name, contName) == 0)
		{
			unlockMib();
			return i;
		}
	}

	unlockMib();
	return -1;
}

static int	getKey(char *keyName, int *authKeyLen, char *authKey)
{
	int	keyLen;

	keyLen = sec_get_key(keyName, authKeyLen, authKey);
	if (keyLen <= 0)
	{
		return -1;
	}

	*authKeyLen = keyLen;
	return 0;
}

static int	getAuthenticationParms(int ventureNbr, int unitNbr, int roleNbr,
			Venture **venture, Unit **unit, int sending,
			char **authName, char *authKey, int *authKeyLen)
{
	AmsMib	*mib = _mib(NULL);
	AppRole	*role = NULL;
	char	*keyName;

	*venture = NULL;
	*unit = NULL;
	*authName = NULL;
	lockMib();
	if (ventureNbr > 0)
	{
		if (ventureNbr > MAX_VENTURE_NBR
		|| (*venture = mib->ventures[ventureNbr]) == NULL)
		{
			writeMemoNote("[?] MAMS msg from unknown msgspace",
					itoa(ventureNbr));
			unlockMib();
			return 0;
		}

		if (unitNbr > MAX_UNIT_NBR
		|| (*unit = (*venture)->units[unitNbr]) == NULL)
		{
			writeMemoNote("[?] MAMS msg from unknown cell",
					itoa(unitNbr));
			unlockMib();
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
			unlockMib();
			return 0;
		}
	}

	if (role)		/*	Sender is module.		*/
	{
		*authName = role->name;
		if (sending)
		{
			keyName = role->privateKeyName;
			if (keyName == NULL)
			{
				*authKeyLen = 0;
			}
			else if (getKey(keyName, authKeyLen, authKey) < 0)
			{
				writeMemoNote("[?] Can't get role private key", 
					role->privateKeyName);
			}
		}
		else
		{
			keyName = role->publicKeyName;
			if (keyName == NULL)
			{
				*authKeyLen = 0;
			}
			else if (getKey(keyName, authKeyLen, authKey) < 0)
			{
				writeMemoNote("[?] Can't get role public key", 
					role->publicKeyName);
			}
		}
	}
	else if (*venture)	/*	Sender is registrar.		*/
	{
		*authName = (*venture)->app->name;
		if (sending)
		{
			keyName = (*venture)->app->privateKeyName;
			if (keyName == NULL)
			{
				*authKeyLen = 0;
			}
			else if (getKey(keyName, authKeyLen, authKey) < 0)
			{
				writeMemoNote("[?] Can't get app private key", 
					(*venture)->app->privateKeyName);
			}
		}
		else
		{
			keyName = (*venture)->app->publicKeyName;
			if (keyName == NULL)
			{
				*authKeyLen = 0;
			}
			else if (getKey(keyName, authKeyLen, authKey) < 0)
			{
				writeMemoNote("[?] Can't get app public key", 
					(*venture)->app->publicKeyName);
			}
		}
	}
	else			/*	Sender is CS.		*/
	{
		*authName = mib->continua[mib->localContinuumNbr]->name;
		if (sending)
		{
			keyName = mib->csPrivateKeyName;
			if (keyName == NULL)
			{
				*authKeyLen = 0;
			}
			else if (getKey(keyName, authKeyLen, authKey) < 0)
			{
				writeMemoNote("[?] Can't get CS private key", 
					mib->csPrivateKeyName);
			}
		}
		else
		{
			keyName = mib->csPublicKeyName;
			if (keyName == NULL)
			{
				*authKeyLen = 0;
			}
			else if (getKey(keyName, authKeyLen, authKey) < 0)
			{
				writeMemoNote("[?] Can't get CS public key", 
					mib->csPublicKeyName);
			}
		}
	}

	unlockMib();
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

static void	destroyFanModule(LystElt elt, void *userdata)
{
	FanModule	*fan = (FanModule *) lyst_data(elt);

	MRELEASE(fan);
}

Subject	*createSubject(Venture *venture, int nbr, char *name,
		char *description, char *symmetricKeyName,
		char *marshalFnName, char *unmarshalFnName)
{
	int	amsMemory = getIonMemoryMgr();
	int	length;
	Subject	*subj;
	int	idx;

	CHKNULL(venture);
	CHKNULL(nbr >= 0);
	CHKNULL(nbr <= MAX_SUBJ_NBR);
	CHKNULL(venture->subjects[nbr] == NULL);
	CHKNULL(name);
	length = strlen(name);
	CHKNULL(length <= MAX_SUBJ_NAME);
	subj = (Subject *) MTAKE(sizeof(Subject));
	CHKNULL(subj);
	memset((char *) subj, 0, sizeof(Subject));
	subj->nbr = nbr;
	length += 1;
	subj->name = MTAKE(length);
	CHKNULL(subj->name);
	istrcpy(subj->name, name, length);
	if (description)
	{
		length = strlen(description) + 1;
		subj->description = MTAKE(length);
		CHKNULL(subj->description);
		istrcpy(subj->description, description, length);
	}

	if (symmetricKeyName)
	{
		length = strlen(symmetricKeyName) + 1;
		subj->symmetricKeyName = MTAKE(length);
		CHKNULL(subj->symmetricKeyName);
		memcpy(subj->symmetricKeyName, symmetricKeyName, length);
	}

	if (marshalFnName)
	{
		length = strlen(marshalFnName) + 1;
		subj->marshalFnName = MTAKE(length);
		CHKNULL(subj->marshalFnName);
		memcpy(subj->marshalFnName, marshalFnName, length);
	}

	if (unmarshalFnName)
	{
		length = strlen(unmarshalFnName) + 1;
		subj->unmarshalFnName = MTAKE(length);
		CHKNULL(subj->unmarshalFnName);
		memcpy(subj->unmarshalFnName, unmarshalFnName, length);
	}

	subj->authorizedSenders = NULL;
	subj->authorizedReceivers = NULL;
	subj->modules = lyst_create_using(amsMemory);
	CHKNULL(subj->modules);
	lyst_delete_set(subj->modules, destroyFanModule, NULL);
	venture->subjects[nbr] = subj;
	idx = hashSubjectName(name);
	subj->elt = lyst_insert_last(venture->subjLysts[idx], subj);
       	CHKNULL(subj->elt);
	return subj;
}

AppRole	*createRole(Venture *venture, int nbr, char *name, char *publicKeyName,
		char *privateKeyName)
{
	int	length;
	AppRole	*role;

	CHKNULL(venture);
	CHKNULL(nbr > 0);
	CHKNULL(nbr <= MAX_ROLE_NBR);
	CHKNULL(venture->roles[nbr] == NULL);
	CHKNULL(name);
	length = strlen(name);
	CHKNULL(length <= MAX_ROLE_NAME);
	role = (AppRole *) MTAKE(sizeof(AppRole));
	CHKNULL(role);
	memset((char *) role, 0, sizeof(AppRole));
	role->nbr = nbr;
	length += 1;
	role->name = MTAKE(length);
	CHKNULL(role->name);
	istrcpy(role->name, name, length);
	if (publicKeyName)
	{
		length = strlen(publicKeyName) + 1;
		role->publicKeyName = MTAKE(length);
		CHKNULL(role->publicKeyName);
		memcpy(role->publicKeyName, publicKeyName, length);
	}

	if (privateKeyName)
	{
		length = strlen(privateKeyName) + 1;
		role->privateKeyName = MTAKE(length);
		CHKNULL(role->privateKeyName);
		memcpy(role->privateKeyName, privateKeyName, length);
	}

	venture->roles[nbr] = role;
	return role;
}

static void	deleteAuthorization(char *roleName, Lyst *authorizations)
{
	LystElt	elt;
	char	*name;
	int	result;

	CHKVOID(roleName);
	for (elt = lyst_first(*authorizations); elt; elt = lyst_next(elt))
	{
		name = (char *) lyst_data(elt);
		result = strcmp(name, roleName);
		if (result < 0)		/*	Try the next one.	*/
		{
			continue;
		}

		if (result == 0)
		{
			break;
		}

		return;			/*	Already deleted.	*/
	}

	name = (char *) lyst_data(elt);
	MRELEASE(name);
	lyst_delete(elt);
	if (lyst_length(*authorizations) == 0)
	{
		lyst_destroy(*authorizations);
		*authorizations = NULL;
	}
}

void	deleteAuthorizedSender(Subject *subj, char *senderRoleName)
{
	CHKVOID(subj);
	deleteAuthorization(senderRoleName, &subj->authorizedSenders);
}

void	deleteAuthorizedReceiver(Subject *subj, char *receiverRoleName)
{
	CHKVOID(subj);
	deleteAuthorization(receiverRoleName, &subj->authorizedReceivers);
}

static void	destroyAuthorization(LystElt elt, void *userdata)
{
	char	*roleName = (char *) lyst_data(elt);

	MRELEASE(roleName);
}

static int	addAuthorization(Venture *venture, char *roleName,
			Lyst *authorizations)
{
	LystElt	elt;
	char	*name;
	int	result;
	int	length;

	CHKERR(venture);
	CHKERR(roleName);
	if (lookUpRole(venture, roleName) == NULL)
	{
		return -1;
	}

	if (*authorizations == NULL)
	{
		*authorizations = lyst_create_using(getIonMemoryMgr());
		CHKERR(*authorizations);
		lyst_delete_set(*authorizations, destroyAuthorization,
				NULL);
	}

	for (elt = lyst_first(*authorizations); elt; elt = lyst_next(elt))
	{
		name = (char *) lyst_data(elt);
		result = strcmp(name, roleName);
		if (result < 0)		/*	Try the next one.	*/
		{
			continue;
		}

		if (result == 0)
		{
			return 0;	/*	Already in list.	*/
		}

		break;			/*	Insert before this one.	*/
	}

	length = strlen(roleName);
	name = MTAKE(length + 1);
	CHKERR(name);
	istrcpy(name, roleName, length + 1);
	if (elt)
	{
		elt = lyst_insert_before(elt, name);
	}
	else
	{
		elt = lyst_insert_last(*authorizations, name);
	}

	if (elt == NULL)
	{
		MRELEASE(name);
		return -1;
	}

	return 0;
}

int	addAuthorizedSender(Venture *venture, Subject *subj,
		char *senderRoleName)
{
	CHKERR(subj);
	return addAuthorization(venture, senderRoleName,
			&(subj->authorizedSenders));
}

int	addAuthorizedReceiver(Venture *venture, Subject *subj,
		char *receiverRoleName)
{
	CHKERR(subj);
	return addAuthorization(venture, receiverRoleName,
			&(subj->authorizedReceivers));
}

LystElt	createApp(char *name, char *publicKeyName, char *privateKeyName)
{
	int	length;
	AmsApp	*app;
	LystElt	elt;

	CHKNULL(name);
	length = strlen(name);
	CHKNULL(length <= MAX_APP_NAME);
	app = (AmsApp *) MTAKE(sizeof(AmsApp));
	CHKNULL(app);
	memset((char *) app, 0, sizeof(AmsApp));
	length += 1;
	app->name = MTAKE(length);
	CHKNULL(app->name);
	istrcpy(app->name, name, length);
	if (publicKeyName)
	{
		length = strlen(publicKeyName) + 1;
		app->publicKeyName = MTAKE(length);
		CHKNULL(app->publicKeyName);
		memcpy(app->publicKeyName, publicKeyName, length);
	}

	if (privateKeyName)
	{
		length = strlen(privateKeyName) + 1;
		app->privateKeyName = MTAKE(length);
		CHKNULL(app->privateKeyName);
		memcpy(app->privateKeyName, privateKeyName, length);
	}

	lockMib();
	elt = lyst_insert_last((_mib(NULL))->applications, app);
	unlockMib();
	CHKNULL(elt);
	return elt;
}

Subject	*createMsgspace(Venture *venture, int continNbr, int isNeighbor,
		char *gwEidString, char *symmetricKeyName)
{
	int		amsMemory = getIonMemoryMgr();
	Subject		*msgspace;
	RamsNetProtocol	ramsProtocol;
	char		*gwEid;
	char		gwEidBuffer[MAX_GW_EID + 1];
	int		length;

	CHKNULL(venture);
	CHKNULL(continNbr > 0);
	CHKNULL(continNbr <= MAX_CONTIN_NBR);
	CHKNULL((_mib(NULL))->continua[continNbr] != NULL);
	CHKNULL(venture->msgspaces[continNbr] == NULL);
	msgspace = (Subject *) MTAKE(sizeof(Subject));
	CHKNULL(msgspace);
	memset((char *) msgspace, 0, sizeof(Subject));
	msgspace->nbr = 0 - continNbr;	/*	Negative subj number.	*/
	msgspace->isNeighbor = 1 - (isNeighbor == 0);
	ramsProtocol = parseGwEid(gwEidString, &gwEid, gwEidBuffer);
	if (ramsProtocol == RamsNoProtocol)
	{
		ramsProtocol = RamsBp;
		isprintf(gwEidBuffer, sizeof gwEidBuffer, "ipn:%d.%d",
				continNbr, venture->nbr);
		gwEid = gwEidBuffer;
	}

	length = strlen(gwEid) + 1;
	msgspace->gwEid = MTAKE(length);
	CHKNULL(msgspace->gwEid);
	istrcpy(msgspace->gwEid, gwEid, length);
	if (symmetricKeyName)
	{
		length = strlen(symmetricKeyName) + 1;
		msgspace->symmetricKeyName = MTAKE(length);
		CHKNULL(msgspace->symmetricKeyName);
		memcpy(msgspace->symmetricKeyName, symmetricKeyName, length);
	}

	msgspace->authorizedSenders = NULL;
	msgspace->authorizedReceivers = NULL;
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
		if (createModule(cell, i) == NULL)
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

static Venture	*createVenture2(int nbr, char *appname, char *authname,
			char *gwEidString, int ramsNetIsTree,
			int rootCellResyncPeriod)
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
	AppRole		*shutdownRole;
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

	gatewayRole = createRole(venture, 1, "RAMS", NULL, NULL);
	shutdownRole = createRole(venture, MAX_ROLE_NBR, "stop", NULL, NULL);
	if (gatewayRole == NULL || shutdownRole == NULL)
	{
		eraseVenture(venture);
		putErrmsg("Can't create standard roles for venture.",
				appname);
		return NULL;
	}

	/*	Automatically create venture's "all subjects" subject.	*/

	allSubjects = createSubject(venture, 0, "everything",
			"messages on all subjects", NULL, NULL, NULL);
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

	msgspace = createMsgspace(venture, mib->localContinuumNbr, 0,
			gwEidString, NULL);
	if (msgspace == NULL)
	{
		putErrmsg("Can't create local msgspace for venture.", appname);
		eraseVenture(venture);
		return NULL;
	}

	return venture;
}

Venture	*createVenture(int nbr, char *appname, char *authname,
		char *gwEidString, int ramsNetIsTree, int rootCellResyncPeriod)
{
	Venture	*result;

	lockMib();
	result = createVenture2(nbr, appname, authname, gwEidString,
			ramsNetIsTree, rootCellResyncPeriod);
	unlockMib();
	return result;
}

static Continuum	*createContinuum2(int nbr, char *name,
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

Continuum	*createContinuum(int nbr, char *name, char *description)
{
	Continuum	*result;

	lockMib();
	result = createContinuum2(nbr, name, description);
	unlockMib();
	return result;
}

LystElt	createCsEndpoint(char *endpointSpec, LystElt nextElt)
{
	AmsMib		*mib = _mib(NULL);
	int		len;
	char		endpointName[MAX_EP_NAME + 1];
	MamsEndpoint	*ep;
	LystElt		elt;

	/*	Note: no need to lockMib() because this function
	 *	is called only from inside loadMib, which has
	 *	already called lockMib().				*/

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

	/*	Note: no need to lockMib() because this function
	 *	is called only from inside loadMib, which has
	 *	already called lockMib().				*/

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
	int	result;

	endpoint->ept = MTAKE(eptLength + 1);
	CHKERR(endpoint->ept);
	memcpy(endpoint->ept, ept, eptLength);
	endpoint->ept[eptLength] = '\0';

	/*	The primary transport service's endpoint parsing
	 *	function examines the endpoint name text and fills
	 *	in the tsep of the endpoint.				*/

	lockMib();
	result = ((_mib(NULL))->pts->parseMamsEndpointFn)(endpoint); 
	unlockMib();
	if (result < 0)
	{
		writeMemoNote("[?] Can't parse endpoint name", ept);
		return -1;
	}

	return 0;
}

void	clearMamsEndpoint(MamsEndpoint *ep)
{
	lockMib();
	(_mib(NULL))->pts->clearMamsEndpointFn(ep);
	unlockMib();
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
	char		authKey[32];
	int		authKeyLen = sizeof authKey; 
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
			tsif->roleNbr, &venture, &unit, 1, &authName, authKey,
			&authKeyLen))
	{
		putErrmsg("Can't get authentication parameters.", NULL);
		return -1;
	}

	if (authName == NULL)		/*	Invalid parameters.	*/
	{
		return 0;		/*	Don't send message.	*/
	}
	
	if (authKeyLen > 0)
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
				&encryptLength, authKey, authKeyLen,
				(char *) (authenticator + 4), encryptLength);
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

	lockMib();
	result = (_mib(NULL))->pts->sendMamsFn(endpoint, tsif, (char *) msg,
			msgLength);
	unlockMib();
	MRELEASE(msg);
	return result;
}

/*	*	*	Event queueing functions	*	*	*/

int	enqueueMamsEvent(Llcv eventsQueue, AmsEvt *evt, char *ancillaryBlock,
		int responseNbr)
{
	saddr	temp;
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

	temp = (saddr) lyst_compare_get(eventsQueue->list);
	queryNbr = temp;
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
	char		authKey[32];
	int		authKeyLen = sizeof authKey;
	int		authNameLen;
	unsigned char	nonce[4];
	int		decryptLength;
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
			&venture, &unit, 0, &authName, authKey, &authKeyLen))
	{
		putErrmsg("Can't get authentication parameters.", NULL);
		return -1;
	}

	if (authName == NULL)		/*	Invalid parameters.	*/
	{
		return 0;		/*	Don't deliver message.	*/
	}
	
	if (authKeyLen > 0)
	{
		/*	Authentication is required.			*/

		authNameLen = strlen(authName);
		CHKERR(authNameLen > 0);
		CHKERR(authNameLen <= (AUTHENTICAT_LEN - 9));
		if (authenticatorLength != authNameLen + 8
		|| authenticator == NULL)
		{
			writeMemo("[?] MAMS msg discarded; bad authenticator.");
			return -1;
		}

		memcpy(nonce, authenticator, 4);
		decryptUsingPublicKey((char *) (authenticator + 4),
				&decryptLength, authKey, authKeyLen,
				(char *) (authenticator + 4),
				authenticatorLength - 4);
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
		if (msg.supplement)
		{
			MRELEASE(msg.supplement);
		}

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
