/*
	librams.c:	functions enabling the implementation of RAMS
			gateway nodes.

	Author: Shin-Ywan (Cindy) Wang
	Copyright (c) 2005, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
*/

#include "ramscommon.h"

#if RAMSDEBUG
static void	PrintGatewayState(RamsGateway *gWay);
static void	PrintInvitationList(RamsGateway *gWay);
#endif
static void	KillGateway();
static void	TerminateGateway();

static int	AssertPetition(RamsGateway *gWay, Petition *pet);
static int	CancelPetition(RamsGateway *gWay, Petition *pet);

static int	NoteDeclaration(RamsNode *fromNode, RamsGateway *gWay);
static int	HandlePetitionAssertion(RamsNode *fromNode,
			RamsGateway *gWay, int subjectNbr, int domainContinuum,
			int domainUnit, int domainRole, int fromPlayback);
static int	HandlePetitionCancellation(RamsNode *fromNode,
			RamsGateway *gWay, int subjectNbr, int domainContinuum,
			int domainUnit, int domainRole, int fromPlayback);
static int	HandlePublishedMessage(RamsNode *fromNode, RamsGateway *gWay,
			char *msg);
static int	HandlePrivateMessage(RamsGateway *gWay, char *msg);
static int	HandleAnnouncedMessage(RamsNode *fromNode,
			RamsGateway *gWay, char *msg);
static int	HandleRPDU(RamsNode *fromNode, RamsGateway *gWay, char *msg);

static int	AddPetitioner(Module *sourceModule,  RamsGateway *gWay,
			int domainRole, int domainContinuum, int domainUnit,
			int subjectNbr);
static int	RemovePetitioner(Module *sourceModule,  RamsGateway *gWay,
			int domainRole, int domainContinuum, int domainUnit,
			int subjectNbr);
static int	ForwardPublishedMessage(RamsGateway *gWay, AmsEvent amsEvent);
static int	ForwardTargetedMessage(RamsGateway *gWay,
			unsigned char flowLabel, char* content,
			int contentLength);

static void	HandleAamsError(void *userData,
					AmsEvent *eventRef);
static void	HandleSubscription(AmsModule module,
					void *userData,
					AmsEvent *eventRef,
					int unitNbr,
					int moduleNbr,
					int domainRoleNbr,
					int domainContinuumNbr,
					int domainUnitNbr,
					int subjectNbr,
					int priority,
					unsigned char flowLabel,
					AmsSequence sequence,
					AmsDiligence diligence);
static void	HandleUnsubscription(AmsModule module,
					void *userData,
					AmsEvent *eventRef,
					int unitNbr,
					int moduleNbr,
					int domainRoleNbr,
					int domainContinuumNbr,
					int domainUnitNbr,
					int subjectNbr);
static void	HandleRegistration(AmsModule module,
					void *userData,
					AmsEvent *eventRef,
					int unitNbr,
					int moduleNbr,
					int roleNbr);
static void	HandleUnregistration(AmsModule module,
					void *userData,
					AmsEvent *eventRef,
					int unitNbr,
					int moduleNbr);
static void	HandleInvitation(AmsModule module,
					void *userData,
					AmsEvent *eventRef,
					int unitNbr,
					int moduleNbr,
					int domainRoleNbr,
					int domainContinuumNbr,
					int domainUnitNbr,
					int subjectNbr,
					int priority,
					unsigned char flowLabel,
					AmsSequence sequence,
					AmsDiligence diligence);
static void	HandleDisinvitation(AmsModule module,
					void *userData,
					AmsEvent *eventRef,
					int unitNbr,
					int moduleNbr,
					int domainRoleNbr,
					int domainContinuumNbr,
					int domainUnitNbr,
					int subjectNbr);
static void	HandleUserEvent(AmsModule module,
					void *userData,
					AmsEvent *eventRef,
					int code,
					int dataLength,
					char *data);
static void	HandleAamsMessage(AmsModule module,
					void *userData,
					AmsEvent *eventRef,
					int continuumNbr,
					int unitNbr,
					int moduleNbr,
					int subjectNbr,
					int contentLength,
					char *content,
					int context,
					AmsMsgType msgType,
					int priority,
					unsigned char flowLabel);

static int	RehandlePetition(RamsNetProtocol protocol, char *gwEid,
			int cc, int sub, int domainContinuum, int domainUnit,
			int domainRole)
{
	RamsGateway	*gWay = _gWay(NULL);
	RamsNode	*fromNode;

	fromNode = Look_Up_Neighbor(gWay, gwEid);
	if (fromNode == NULL)
	{
		ErrMsg("Can't find sending node.");
		return -1;
	}

	switch (cc)
	{
	case PetitionAssertion:
		if (sub == 0 - fromNode->continuumNbr)
		{
			if (NoteDeclaration(fromNode, gWay) < 0)
			{
				return -1;
			}
		}
		else
		{
			if (Look_Up_DeclaredNeighbor(gWay,
					fromNode->continuumNbr) == NULL)
			{
				ErrMsg("Can't find declared source gateway.");
				return -1;
			}
		}

		if (HandlePetitionAssertion(fromNode, gWay, sub,
				domainContinuum, domainUnit, domainRole, 1))
		{
			return -1;
		}

		break;

	case PetitionCancellation:
		if (HandlePetitionCancellation(fromNode, gWay, sub,
				domainContinuum, domainUnit, domainRole, 1))
		{
			return -1;
		}

		break;

	default:
		ErrMsg("Invalid cc in petition log line.");
		return -1;
	}

	return 0;
}

static int	PlaybackPetitionLog(int petitionLog)
{
	char		buffer[512];
	int		lineLen;
	RamsNetProtocol	protocol;
	char		gwEid[256];
	unsigned int	cc;
	int		sub;
	unsigned int	domainContinuum;
	unsigned int	domainUnit;
	unsigned int	domainRole;

	if (lseek(petitionLog, 0, SEEK_SET) < 0)
	{
		putSysErrmsg("Can't lseek to start of petition log", NULL);
		return -1;
	}

	while (1)
	{
		if (igets(petitionLog, buffer, sizeof buffer, &lineLen) == NULL)
		{
			if (lineLen == 0)
			{
				return 0;	/*	End of file.	*/
			}

			ErrMsg("igets failed.");
			return -1;
		}

		if (sscanf(buffer, "%u %255s %u %d %u %u %u",
			(unsigned int *) &protocol, gwEid, &cc, &sub,
			&domainContinuum, &domainUnit, &domainRole) == 7)
		{
			if (RehandlePetition(protocol, gwEid, cc, sub,
				domainContinuum, domainUnit, domainRole))
			{
				ErrMsg("Can't play back petition log.");
				return -1;
			}

			continue;
		}

		putErrmsg("Malformed petition log line.", buffer);
		return -1;
	}
}

static int	_petitionLog(char *logLine, int ventureNbr)
{
	static int	petitionLog = -1;
	int		len;

	/*	A NULL logLine means either "open and playback the
	 *	petition log" (if it's not currently open) or "close
	 *	the petition log" (otherwise).  A non-NULL logLine
	 *	is a line of text that must be appended to the petition
	 *	log file.						*/

	if (logLine == NULL)
	{
		if (petitionLog < 0)	/*	Log is not open.	*/
		{
			char	logName[64];

			/*	Recover all known petition activity.	*/

			isprintf(logName, sizeof logName, "%d.petition.log",
					ventureNbr);
			petitionLog = iopen(logName,
					O_RDWR | O_CREAT | O_APPEND, 0777);
			if (petitionLog < 0)
			{
				putSysErrmsg("Can't open petition log", NULL);
				return -1;
			}

			if (PlaybackPetitionLog(petitionLog) < 0)
			{
				ErrMsg("RAMS can't playback petition log.");
				close(petitionLog);
				petitionLog = -1;
				return -1;
			}
		}
		else			/*	Log is open.		*/
		{
			close(petitionLog);
			petitionLog = -1;
		}

		return 0;
	}

	/*	Printing a line to the petition log.			*/

	if (petitionLog < 0)
	{
		putErrmsg("Can't write to petition log -- not open.", logLine);
		return -1;
	}

	len = strlen(logLine);
	if (len > 0)
	{
		if (write(petitionLog, logLine, len) < 0)
		{
			putSysErrmsg("Can't write to petition log file",
					logLine);
			return -1;
		}
	}

	return 0;
}

/*	*	*	RAMS gateway main line	*	*	*	*/

#ifdef mingw
static void	KillGateway()
{
	RamsGateway	*gWay = _gWay(NULL);

	gWay->stopping = 1;
	if (gWay->netProtocol == RamsBp)
	{
		bp_interrupt(gWay->sap);
	}
	else	/*	Must make sure recvfrom is interrupted.		*/
	{
		shutdown(gWay->ownUdpFd, SD_BOTH);
	}
}
#else
static pthread_t	_mainThread()
{
	static pthread_t	mainThread;
	static int		haveMainThread = 0;

	if (haveMainThread == 0)
	{
		mainThread = pthread_self();
		haveMainThread = 1;
	}

	return mainThread;
}

static void	KillGateway()
{
	RamsGateway	*gWay = _gWay(NULL);
	pthread_t	mainThread;

	gWay->stopping = 1;
	if (gWay->netProtocol == RamsBp)
	{
		bp_interrupt(gWay->sap);
	}
	else	/*	Must make sure recvfrom is interrupted.		*/
	{
		mainThread = _mainThread();
		if (!pthread_equal(mainThread, pthread_self()))
		{
			pthread_kill(mainThread, SIGTERM);
		}
	}
}
#endif

static void	InterruptGateway(int signum)
{
	isignal(SIGTERM, InterruptGateway);
	KillGateway();
}

static int	HandleBundle(BpDelivery *dlv, char *buffer)
{
	RamsGateway	*gWay = _gWay(NULL);
	Sdr		sdr = getIonsdr();
	RamsNode	*fromNode;
	ZcoReader	reader;
	int		bytesCopied;
	int		enclosureLength;
#if RAMSDEBUG
int	i;
#endif
	fromNode = Look_Up_Neighbor(gWay, dlv->bundleSourceEid);
	if (fromNode == NULL)	/*	Stray bundle?			*/
	{
#if RAMSDEBUG
printf("Can't find source gateway '%s'.\n", dlv->bundleSourceEid);
#endif
		return 0;
	}

	zco_start_receiving(dlv->adu, &reader);
	if ((bytesCopied = zco_receive_source(sdr, &reader, ENVELOPELENGTH,
			buffer)) < ENVELOPELENGTH)
	{
		ErrMsg("Can't receive envelope.");
		return -1;
	}

	enclosureLength = EnvelopeHeader(buffer, Env_EnclosureLength);
#if RAMSDEBUG
printf("Receive RPDU:\n");
for (i = 0; i < ENVELOPELENGTH; i++)
{
	printf("%2x ", *(buffer + i));
}
printf("\n");
printf("cc=%d, con=%d, unit=%d, srcId=%d, destId=%d, sub=%d len=%d from=%d\n",
EnvelopeHeader(buffer, Env_ControlCode),
EnvelopeHeader(buffer, Env_ContinuumNbr),
EnvelopeHeader(buffer, Env_UnitField),
EnvelopeHeader(buffer, Env_SourceIDField),
EnvelopeHeader(buffer, Env_DestIDField),
EnvelopeHeader(buffer, Env_SubjectNbr),
enclosureLength, fromNode->continuumNbr);				
#endif
	if (enclosureLength > 0)
	{
		if ((bytesCopied = zco_receive_source(sdr, &reader,
				enclosureLength, buffer + ENVELOPELENGTH))
				< enclosureLength)
		{
			ErrMsg("Can't receive enclosure.");
			return -1;
		}
	}

	/*	Handle RAMS PDU from remote RAMS gateway.		*/

	if (HandleRPDU(fromNode, gWay, buffer) < 0)
	{
		ErrMsg("Can't receive RPDU.");
		return -1;
	}

	return 0;
}

static int	HandleDatagram(struct sockaddr_in *inetName,
			int datagramLength, char *buffer)
{
	RamsGateway	*gWay = _gWay(NULL);
	unsigned int	ipAddress;
	unsigned short	portNbr;
	char		gwEidBuffer[32];
	RamsNode	*fromNode;
	int		enclosureLength;
#if RAMSDEBUG
int	i;
#endif
	memcpy((char *) &ipAddress, (char *) &(inetName->sin_addr.s_addr), 4);
	ipAddress = ntohl(ipAddress);
	portNbr = inetName->sin_port;
	portNbr = ntohs(portNbr);
	isprintf(gwEidBuffer, sizeof gwEidBuffer, "%u:%hu", ipAddress, portNbr);
	fromNode = Look_Up_Neighbor(gWay, gwEidBuffer);
	if (fromNode == NULL)
	{
#if RAMSDEBUG
printf("Can't find source gateway '%s'.\n", gwEidBuffer);
#endif
		return 0;
	}

	if (datagramLength < ENVELOPELENGTH)
	{
#if RAMSDEBUG
printf("Datagram too short for envelope: %d.\n", datagramLength);
#endif
		return 0;
	}

	enclosureLength = EnvelopeHeader(buffer, Env_EnclosureLength);
#if RAMSDEBUG
printf("Receive RPDU:\n");
for (i = 0; i < ENVELOPELENGTH; i++)
{
	printf("%2x ", *(buffer + i));
}
printf("\n");
printf("cc=%d, con=%d, unit=%d, srcId=%d, destId=%d, sub=%d len=%d from=%d\n",
EnvelopeHeader(buffer, Env_ControlCode),
EnvelopeHeader(buffer, Env_ContinuumNbr),
EnvelopeHeader(buffer, Env_UnitField),
EnvelopeHeader(buffer, Env_SourceIDField),
EnvelopeHeader(buffer, Env_DestIDField),
EnvelopeHeader(buffer, Env_SubjectNbr),
enclosureLength, fromNode->continuumNbr);				
#endif
	if (enclosureLength > 0)
	{
		if (datagramLength < (ENVELOPELENGTH + enclosureLength))
		{
#if RAMSDEBUG
printf("Datagram has truncated enclosure: %d.\n", datagramLength);
#endif
			return 0;
		}
	}

	/*	 Handle RAMS PDU from remote RAMS gateway.		*/

	if (HandleRPDU(fromNode, gWay, buffer))
	{
		ErrMsg("Can't receive RPDU.");
		return -1;
	}

	return 0;
}

static int	compareCheckTimes(void *data1, void *data2)
{
	UdpRpdu	*rpdu1 = (UdpRpdu *) data1;
	UdpRpdu	*rpdu2 = (UdpRpdu *) data2;

	if (rpdu1->checkTime < rpdu2->checkTime) return -1;
	if (rpdu1->checkTime > rpdu2->checkTime) return 1;
	return 0;
}

static void	deleteDeclaration(LystElt elt, void *arg)
{
	UdpRpdu	*rpdu = (UdpRpdu *) lyst_data(elt);

	MRELEASE(rpdu->envelope);
	MRELEASE(rpdu);
}

static void	DeleteInvitation(Invitation *inv)
{
	lyst_destroy(inv->moduleSet);
	MRELEASE(inv->inviteSpecification);
	MRELEASE(inv);
}

int	rams_run(char *mibSource, char *tsorder, char *applicationName,
		char *authorityName, char *unitName, char *roleName,
		long lifetime)
{
	int			amsMemory;
	AmsModule		amsModule;
	AmsMib			*mib;
	int			ownContinuumNbr;
	Subject			*ownMsgspace;
	RamsGateway		*gWay;
	Sdr			sdr;
	LystElt			elt;
	BpDelivery		dlv;
	char    		*buffer;
	unsigned char		envelope[ENVELOPELENGTH];
	RamsNode		*ramsNode;
	char			gwEid[256];
	unsigned short		portNbr;
	unsigned int		ipAddress;
	struct sockaddr		socketName;
	struct sockaddr_in	*inetName = (struct sockaddr_in *) &socketName;
	socklen_t		nameLength;
	int			datagramLength;
	Lyst			msgspaces;
	saddr			temp;
	long			cId;
	Petition		*pet;
	AmsEventMgt		rules;
	int			ownPseudoSubject;
	pthread_t		checkThread;

	PUTS("RAMS version 1.0");

	/*	Either the gateway net protocol will be BP, and we'll
	 *	set sdr to the ION sdr, or using SDR will correctly
	 *	generate an assertion error.				*/

	sdr = 0;

	/*	Register as an AMS module.				*/

	if (ams_register(mibSource, tsorder, applicationName, authorityName,
			unitName, roleName, &amsModule) < 0)
	{
		putErrmsg("RAMS gateway can't register.", NULL);
		return -1;
	}

	mib = _mib(NULL);
	ownContinuumNbr = mib->localContinuumNbr;
	ownMsgspace = amsModule->venture->msgspaces[ownContinuumNbr];
	amsMemory = getIonMemoryMgr();

	/*	Construct RAMS gateway state.				*/

	gWay = _gWay(NULL);
	if (gWay == NULL)
	{
		putErrmsg("Can't create RamsGateway object.", NULL);
		return -1;
	}

	gWay->amsModule = amsModule;
	gWay->primeThread = pthread_self();
	gWay->petitionReceiveThread = pthread_self();
	gWay->amsMib = mib;
	gWay->neighborsCount = 0;
	gWay->declaredNeighborsCount = 0;
	gWay->petitionSet = lyst_create_using(amsMemory);
	gWay->registerSet = lyst_create_using(amsMemory);
	gWay->invitationSet = lyst_create_using(amsMemory);
	gWay->ramsNeighbors = lyst_create_using(amsMemory);
	gWay->declaredNeighbors = lyst_create_using(amsMemory);
	if (gWay->petitionSet == NULL
	|| gWay->registerSet == NULL
	|| gWay->invitationSet == NULL
	|| gWay->ramsNeighbors == NULL
	|| gWay->declaredNeighbors == NULL)
	{
		putErrmsg("Can't initialize RAMS gateway object.", NULL);
		return -1;
	}

	/*	Determine RAMS network type.				*/

	if (ams_rams_net_is_tree(amsModule))
	{
		gWay->netType = TREETYPE;
	}
	else 
	{
		gWay->netType = MESHTYPE;
	}

	/*	Load list of all neighboring nodes in the RAMS network.	*/

#if RAMSDEBUG
printf("continuum lyst:");
#endif
	msgspaces = ams_list_msgspaces(amsModule);
	for (elt = lyst_first(msgspaces); elt; elt = lyst_next(elt))
	{
		temp = (saddr) lyst_data(elt);
		cId = temp;
#if RAMSDEBUG
printf(" %ld", cId);		
#endif
		if (cId == gWay->amsMib->localContinuumNbr)
		{
			continue;
		}

		if (!ams_msgspace_is_neighbor(amsModule, cId))
		{
			continue;
		} 	

		ramsNode = (RamsNode *) MTAKE(sizeof(RamsNode));
		if (ramsNode == NULL)
		{
			putErrmsg("Can't create RAMS neighbor object.", NULL);
			return -1;
		}

		memset(ramsNode, 0, sizeof(RamsNode));
		ramsNode->continuumNbr = cId;
		ramsNode->protocol = amsModule->venture->gwProtocol;
		ramsNode->gwEid = amsModule->venture->msgspaces[cId]->gwEid;
		if (lyst_insert_last(gWay->ramsNeighbors, ramsNode)
				== NULL)
		{
			putErrmsg("Can't note RAMS neighbor.", NULL);
			return -1;
		}

		gWay->neighborsCount++;
#if RAMSDEBUG
printf("[neighbor]");
#endif
	}

#if RAMSDEBUG
printf("\n");
#endif
	lyst_destroy(msgspaces);

	/*	Load AMS event management rules and spawn AMS event
	 *	management thread.					*/

	rules.registrationHandler = HandleRegistration;
	rules.registrationHandlerUserData = gWay;
	
	rules.unregistrationHandler = HandleUnregistration;
	rules.unregistrationHandlerUserData = gWay;
	
	rules.invitationHandler = HandleInvitation;
	rules.invitationHandlerUserData = gWay;
	
	rules.disinvitationHandler = HandleDisinvitation;
	rules.disinvitationHandlerUserData = gWay;
	
	rules.subscriptionHandler = HandleSubscription;
	rules.subscriptionHandlerUserData = gWay;
	
	rules.unsubscriptionHandler = HandleUnsubscription;
	rules.unsubscriptionHandlerUserData = gWay;
    
	rules.userEventHandler = HandleUserEvent;
	rules.userEventHandlerUserData = gWay;	
    
	rules.errHandler = HandleAamsError;
	rules.errHandlerUserData = gWay;

	rules.msgHandler = HandleAamsMessage;
	rules.msgHandlerUserData = gWay;

	ams_set_event_mgr(amsModule, &rules);	

	/*	Subscribe to message on the pseudo-subject for the
	 *	local continuum.					*/

	ownPseudoSubject = 0 - gWay->amsMib->localContinuumNbr;
	if (ams_subscribe(amsModule, 0, 0, 0, ownPseudoSubject, 10, 0,
			AmsTransmissionOrder, AmsAssured) < 0)
	{
		putErrmsg("Can't subscribe to local continuum pseudo-subject.",
				itoa(ownPseudoSubject));
		return -1;
	}

#if RAMSDEBUG
printf("subscribed to %d\n", ownPseudoSubject);
#endif
	/*	Insert self into RAMS network.				*/

	gWay->netProtocol = amsModule->venture->gwProtocol;
	switch (gWay->netProtocol)
	{
	case RamsBp:
#if RAMSDEBUG
printf("ownEid for bp_open is '%s'.\n", ownMsgspace->gwEid);
#endif
		gWay->ttl = lifetime;
		if (bp_attach() < 0)
		{
			ErrMsg("Can't attach to BP.");
			return -1;
		}
#if RAMSDEBUG
printf("bp_attach succeeds.\n");
#endif
		if (bp_open(ownMsgspace->gwEid, &gWay->sap) < 0)
		{
			ErrMsg("Can't open own BP endpoint.");
			return -1;
		}
#if RAMSDEBUG
printf("bp_open succeeds.\n");
#endif
		sdr = getIonsdr();
		break;

	case RamsUdp:
		gWay->udpRpdus = lyst_create_using(amsMemory);
		CHKERR(gWay->udpRpdus);
		lyst_compare_set(gWay->udpRpdus, compareCheckTimes);
		lyst_delete_set(gWay->udpRpdus, deleteDeclaration, NULL);
		if (pthread_begin(&checkThread, NULL, CheckUdpRpdus,
			NULL, "librams_check"))
		{
			putSysErrmsg("Can't create check thread", NULL);
			return -1;
		}

		istrcpy(gwEid, ownMsgspace->gwEid, sizeof gwEid);
		parseSocketSpec(gwEid, &portNbr, &ipAddress);
		portNbr = htons(portNbr);
		ipAddress = htonl(ipAddress);
		memset((char *) &socketName, 0, sizeof socketName);
		inetName->sin_family = AF_INET;
		inetName->sin_port = portNbr;
		memcpy((char *) &(inetName->sin_addr.s_addr),
				(char *) &ipAddress, 4);
		gWay->ownUdpFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (gWay->ownUdpFd < 0)
		{
			putSysErrmsg("Can't create UDP socket", NULL);
			return -1;
		}

		nameLength = sizeof(struct sockaddr);
		if (reUseAddress(gWay->ownUdpFd)
		|| bind(gWay->ownUdpFd, &socketName, nameLength) < 0
		|| getsockname(gWay->ownUdpFd, &socketName, &nameLength) < 0)
		{
			putSysErrmsg("Can't open own UDP endpoint", NULL);
			return -1;
		}

		break;

	default:
		ErrMsg("No valid RAMS network protocol selected.");
		return -1;
	}

	/*	Declare self to all RAMS network neighbors.		*/

#if RAMSDEBUG
printf("Gateway declares itself to all RAMS network neighbors ....\n");
#endif
	ConstructEnvelope(envelope, 0, 0, 0, 0, ownPseudoSubject, 0, NULL,
			PetitionAssertion);
	pet = ConstructPetitionFromEnvelope((char *) envelope);
	CHKERR(pet);
	if (SendRPDU(gWay, 0, 1, (char *) envelope, ENVELOPELENGTH))
	{
		ErrMsg("Failed sending initial declaration.");
		return -1;
	}

	/*	Add all neighbors to SGS for declaration petition.	*/

	for (elt = lyst_first(gWay->ramsNeighbors); elt; elt = lyst_next(elt))
	{
		ramsNode = (RamsNode *) lyst_data(elt);
		if (ramsNode->continuumNbr != gWay->amsMib->localContinuumNbr)
		{
	    		if (lyst_insert_last(pet->SourceNodeSet, ramsNode)
					== NULL)
			{
				ErrMsg("Failed adding node to SGS");
				return -1;
			}
		}
	}

	/*	Add this petition, and add self to the DMS for this
	 *	petition.  Then recover all other known petitions.	*/

	if (lyst_insert_last(gWay->petitionSet, pet) == NULL
	|| lyst_insert_last(pet->DistributionModuleSet,
			LookupModule(ams_get_unit_nbr(amsModule),
			ams_get_module_nbr(amsModule), gWay)) == NULL)
	{
		ErrMsg("Failed adding petition for initial declaration.");
		return -1;
	}

	if (_petitionLog(NULL, amsModule->venture->nbr) < 0)
	{
		ErrMsg("Failed initializing the petition log.");
		return -1;
	}

	/*	This is the RPDU handling thread, the operational main
	 *	loop for the RAMS gateway module.			*/

	buffer = MTAKE(65534);
	if (buffer == NULL)
	{
		ErrMsg("Can't allocate RPDU buffer.");
		return -1;
	}

#ifndef mingw
	oK(_mainThread());
#endif
	isignal(SIGTERM, InterruptGateway);
	while (gWay->stopping == 0)
	{
		switch (gWay->netProtocol)
		{
		case RamsBp:
#if RAMSDEBUG
printf("Before bp_receive...\n");
#endif
    			if (bp_receive(gWay->sap, &dlv, BP_BLOCKING) < 0)
			{
				ErrMsg("RAMS bundle reception failed.");
				gWay->stopping = 1;
				continue;
			}

			switch (dlv.result)
			{
			case BpEndpointStopped:
				gWay->stopping = 1;
				break;

			case BpPayloadPresent:
				CHKERR(sdr_begin_xn(sdr));
				if (HandleBundle(&dlv, buffer) < 0)
				{
					sdr_cancel_xn(sdr);
					gWay->stopping = 1;
				}
				else
				{
					if (sdr_end_xn(sdr) < 0)
					{
						ErrMsg("Can't handle bundle.");
						gWay->stopping = 1;
					}
				}

				break;

			default:
				break;
			}

			bp_release_delivery(&dlv, 1);
			continue;

		case RamsUdp:
			nameLength = sizeof(struct sockaddr_in);
			datagramLength = recvfrom(gWay->ownUdpFd, buffer,
					65534, 0, &socketName, &nameLength);
			switch (datagramLength)
			{
			case -1:
				if (errno == EINTR)
				{
					continue;
				}

				putSysErrmsg("RAMS datagram reception failed",
						NULL);

				/*	Intentional fall-through.	*/

			case 0:		/*	Peer socket closed.	*/
				gWay->stopping = 1;
				continue;

			default:
				if (HandleDatagram(inetName, datagramLength,
						buffer) < 0)
				{
					ErrMsg("Can't handle datagram.");
					gWay->stopping = 1;
				}
			}

			continue;

		default:
			ErrMsg("No RAMS network protocol.");
			gWay->stopping = 1;
		}
	}

	MRELEASE(buffer);		/*	Release RPDU buffer.	*/
	if (gWay->netProtocol == RamsUdp)
	{
		pthread_end(checkThread);
		pthread_join(checkThread, NULL);
	}

	TerminateGateway(gWay);
	if (gWay->netProtocol == RamsBp)
	{
		bp_detach();
	}

	oK(_gWay(gWay));
	oK(_petitionLog(NULL, 0));	/*	Close the petition log.	*/
	writeMemo("[i] Stopping RAMS gateway.");
	return 0;
}

static void	TerminateGateway(RamsGateway *gWay)
{
	int		ownPseudoSubject;
	LystElt		elt;
	LystElt		sgsElt;
	RamsNode	*node;
	Petition	*pet;
	Invitation	*inv;

	/*	First cancel petitions as necessary.  To do so, begin
	 *	by locating the local continuum's declaration petition.	*/

	ownPseudoSubject = 0 - gWay->amsMib->localContinuumNbr;
#if RAMSDEBUG
printf("<terminate gateway> terminating %d\n", gWay->amsMib->localContinuumNbr);
#endif
	for (elt = lyst_first(gWay->petitionSet); elt; elt = lyst_next(elt))
	{
		pet = (Petition *) lyst_data(elt);
		if (EnvelopeHeader(pet->specification->envelope, Env_SubjectNbr)
				== ownPseudoSubject)
		{
			break;	/*	Found the declaration petition.	*/
		}
	}

	if (elt == NULL)	/*	Found no declaration petition.	*/
	{
		ErrMsg("Can't terminate properly: no declaration petition.");
	}
	else
	{
		/*	Now retract self from all neighbors.		*/

		for (elt = lyst_first(gWay->declaredNeighbors); elt;
    				elt = lyst_next(elt))
		{
			node = (RamsNode *) lyst_data(elt);
			if ((sgsElt = NodeSetMember(node, pet->SourceNodeSet))
					== NULL)
			{
				/*	This neighbor is not in the SGS
				 *	of the declaration petition.	*/

				continue;
			}
#if RAMSDEBUG
printf("<terminate gateway> sending cancellation on subject %d to %d\n",
ownPseudoSubject, node->continuumNbr);
#endif
			/*	Send petition cancellation.		*/

			if (SendNewRPDU(gWay, node->continuumNbr, 1, NULL,
					0, 0, 0, 0, PetitionCancellation,
					ownPseudoSubject) < 0)
			{
				ErrMsg("Can't cancel declaration petition.");
				continue;
			}

			/*	Remove node from SGS of this petition.	*/

			lyst_delete(sgsElt);
		}
	}

	/*	Wait 2 seconds for all retraction RPDUs to be sent.	*/

	snooze(2);

	/*	Now extract self from RAMS network.			*/

	switch (gWay->netProtocol)
	{
	case RamsBp:
		bp_close(gWay->sap);
		break;

	case RamsUdp:
		closesocket(gWay->ownUdpFd);
		break;

	default:
		ErrMsg("Can't detach from RAMS network: unknown protocol.");
	}

	/*	Now release gateway state resources.			*/

	ams_unregister(gWay->amsModule);
	for (elt = lyst_first(gWay->petitionSet); elt; elt = lyst_next(elt))
	{
		pet = (Petition *) lyst_data(elt);
        	DeletePetition(pet);
	}

	lyst_destroy(gWay->petitionSet);
	lyst_destroy(gWay->registerSet);
	for (elt = lyst_first(gWay->invitationSet); elt; elt = lyst_next(elt))
	{
		inv = (Invitation *) lyst_data(elt);
        	DeleteInvitation(inv);
	}

	lyst_destroy(gWay->invitationSet);
	for (elt = lyst_first(gWay->ramsNeighbors); elt; elt = lyst_next(elt))
	{
		node = (RamsNode *) lyst_data(elt);
		MRELEASE(node);
	}

	lyst_destroy(gWay->ramsNeighbors);
	lyst_destroy(gWay->declaredNeighbors);
	if (gWay->netProtocol == RamsUdp)
	{
		lyst_destroy(gWay->udpRpdus);
	}
}

#if RAMSDEBUG
/*	*	*	Debugging utilities	*	*	*	*/

static void	PrintGatewayState(RamsGateway *gWay)
{
	char		buf[256];
	LystElt		eltP;
	LystElt		eltN;
	Petition	*pet;
	RamsNode	*ep;
	Module		*nd;

	isprintf(buf, sizeof buf, "===== In Continuum %d ======",
			gWay->amsMib->localContinuumNbr);
	PUTS(buf);
	for (eltP = lyst_first(gWay->petitionSet); eltP != NULL;
			eltP = lyst_next(eltP))
	{
		pet = (Petition *) lyst_data(eltP);
		isprintf(buf, sizeof buf,
			"Petition con = %d role = %d unit = %d module = %d \
sub = %d", EnvelopeHeader(pet->specification->envelope, Env_ContinuumNbr),
			EnvelopeHeader(pet->specification->envelope,
				Env_PublishRoleNbr),
			EnvelopeHeader(pet->specification->envelope,
				Env_PublishUnitNbr),
			EnvelopeHeader(pet->specification->envelope,
				Env_DestIDField),
			EnvelopeHeader(pet->specification->envelope,
				Env_SubjectNbr));
		PUTS(buf);
		PUTS("    SGS = ");
		for (eltN = lyst_first(pet->SourceNodeSet); eltN != NULL;
				eltN = lyst_next(eltN))
		{
			ep = (RamsNode *) lyst_data(eltN);
			isprintf(buf, sizeof buf, "%d ", ep->continuumNbr);
			PUTS(buf);
		}

		PUTS("    DGS = ");
		for (eltN = lyst_first(pet->DestinationNodeSet); eltN != NULL;
				eltN = lyst_next(eltN))
		{
			ep = (RamsNode *) lyst_data(eltN);
			isprintf(buf, sizeof buf, "%d ", ep->continuumNbr);
			PUTS(buf);
		}

		PUTS("    DMS = ");
		for (eltN = lyst_first(pet->DistributionModuleSet);
				eltN != NULL; eltN = lyst_next(eltN))
		{
			nd = (Module *) lyst_data(eltN);
			isprintf(buf, sizeof buf, "%d ", nd->nbr);
			PUTS(buf);
		}
	}

	PUTS("====================");
}

static void	PrintInvitationList(RamsGateway *gWay)
{
	char		buf[256];
	LystElt		elt;
	LystElt		modulesElt;
	Invitation	*inv;
	Module		*module;

	PUTS("========= invitation list ==============");
	for (elt = lyst_first(gWay->invitationSet); elt; elt = lyst_next(elt))
	{
		inv = (Invitation *) lyst_data(elt);
		isprintf(buf, sizeof buf, "Invitation spec: unit=%d role=%d \
sub=%d\n", inv->inviteSpecification->domainUnitNbr,
				inv->inviteSpecification->domainRoleNbr,
				inv->inviteSpecification->subjectNbr);
		PUTS(buf);
		PUTS("    module set = ");
		for (modulesElt = lyst_first(inv->moduleSet); modulesElt;
				modulesElt = lyst_next(modulesElt))
		{
			module = (Module *) lyst_data(modulesElt);
			isprintf(buf, sizeof buf, "(unit=%d moduleId=%d) ",
					module->unitNbr, module->nbr);
			PUTS(buf);
		}
	}

	PUTS("=============================");
}
#endif

/*	*	*	AMS event handlers	*	*	*	*/

static void	HandleAamsError(void *userData, AmsEvent *event)
{
	ErrMsg("AAMS error.");
	KillGateway();
}

static void	HandleSubscription(AmsModule module, void *userData,
			AmsEvent *eventRef, int unitNbr, int moduleNbr,
			int domainRoleNbr, int domainContinuumNbr,
			int domainUnitNbr, int subjectNbr, int priority,
			unsigned char flowLabel, AmsSequence sequence,
			AmsDiligence diligence)
{
	RamsGateway	*gWay;
	Module		*sourceModule;

#if RAMSDEBUG
printf("<HandleSubscription> receive subscription notice from %d \
subjectNbr=%d\n", moduleNbr, subjectNbr);
#endif
	gWay = (RamsGateway *) userData;
	sourceModule = LookupModule(unitNbr, moduleNbr, gWay);
	if (domainContinuumNbr != gWay->amsMib->localContinuumNbr
	&& !ModuleIsMyself(sourceModule, gWay))
	{				
		AddPetitioner(sourceModule, gWay, domainRoleNbr,
				domainContinuumNbr, domainUnitNbr, subjectNbr);
	}
}

static void	HandleUnsubscription(AmsModule module, void *userData,
			AmsEvent *eventRef, int unitNbr, int moduleNbr,
			int domainRoleNbr, int domainContinuumNbr,
			int domainUnitNbr, int subjectNbr)
{
	RamsGateway	*gWay;
	Module		*sourceModule;

#if RAMSDEBUG
printf("<HandleUnsubscription> receive unsubscription from %d subjectNbr=%d\n",
moduleNbr, subjectNbr);
#endif
	gWay = (RamsGateway *) userData;
	sourceModule = LookupModule(unitNbr, moduleNbr, gWay);
	if (domainContinuumNbr != gWay->amsMib->localContinuumNbr
	&& !ModuleIsMyself(sourceModule, gWay))
	{		
		RemovePetitioner(sourceModule, gWay, domainRoleNbr,
				domainContinuumNbr, domainUnitNbr, subjectNbr);
	}
}

static void	HandleRegistration(AmsModule module, void *userData,
			AmsEvent *eventRef, int unitNbr, int moduleNbr,
			int roleNbr)
{
	RamsGateway	*gWay = _gWay(NULL);
	Module		*sourceModule;

#if RAMSDEBUG
PUTS("in HandleRegistration");
#endif
	sourceModule = LookupModule(unitNbr, moduleNbr, gWay);
	if (ModuleSetMember(sourceModule, gWay->registerSet) == NULL)
	{
		if (lyst_insert_last(gWay->registerSet, sourceModule) == NULL)
		{
			ErrMsg("Can't note module registration.");
		}
#if RAMSDEBUG
printf("<HandleRegistration> add module (unit=%d Id=%d)\n", unitNbr, moduleNbr);
#endif
	}
}

static void	HandleUnregistration(AmsModule module, void *userData,
			AmsEvent *eventRef, int unitNbr, int moduleNbr)
{
	RamsGateway	*gWay = _gWay(NULL);
	Module		*sourceModule;
	LystElt		elt;

#if RAMSDEBUG
PUTS("in HandleUnregistration");
#endif
	unitNbr = ams_get_unit_nbr(module);
	moduleNbr = ams_get_module_nbr(module);
	if (unitNbr < 0 || moduleNbr < 0)
	{
		return;
	}

	sourceModule = LookupModule(unitNbr, moduleNbr, gWay);
	if ((elt = ModuleSetMember(sourceModule, gWay->registerSet)) != NULL)
	{
		lyst_delete(elt);
#if RAMSDEBUG
printf("<HandlUnregistration> remove module (unit=%d Id=%d)\n",
ams_get_unit_nbr(module), ams_get_module_nbr(module));
#endif
	}
}

static void	HandleInvitation(AmsModule module, void *userData,
			AmsEvent *eventRef, int unitNbr, int moduleNbr,
			int domainRoleNbr, int domainContinuumNbr,
			int domainUnitNbr, int subjectNbr, int priority,
			unsigned char flowLabel, AmsSequence sequence,
			AmsDiligence diligence)
{
	RamsGateway	*gWay = (RamsGateway *) userData;
	Module		*sourceModule;
	LystElt		invElt;
	Invitation	*inv;
	LystElt		elt;

#if RAMSDEBUG
printf("in HandleInvitation subjectNbr=%d from unit=%d module=%d to domain \
role=%d domain unit=%d\n", subjectNbr, unitNbr, moduleNbr, domainRoleNbr,
domainUnitNbr);
#endif
	if (subjectNbr <= 0) 
	{
		return;
	}

	sourceModule = LookupModule(unitNbr, moduleNbr, gWay);
	if (ModuleIsMyself(sourceModule, gWay))
	{
		return;
	}

	if ((invElt = InvitationSetMember(domainUnitNbr, domainRoleNbr, 0,
			subjectNbr, gWay->invitationSet)) == NULL)
	{
		inv = (Invitation *) MTAKE(sizeof(Invitation));
		CHKVOID(inv);
		invElt = lyst_insert_last(gWay->invitationSet, inv);
		CHKVOID(invElt);
		inv->inviteSpecification = (InvitationSpec *)
				MTAKE(sizeof(InvitationSpec));
		CHKVOID(inv->inviteSpecification);
		inv->inviteSpecification->domainUnitNbr = domainUnitNbr;
		inv->inviteSpecification->domainRoleNbr = domainRoleNbr;
		inv->inviteSpecification->domainContNbr = domainContinuumNbr;
		inv->inviteSpecification->subjectNbr = subjectNbr;
		inv->moduleSet = lyst_create_using(getIonMemoryMgr());
		CHKVOID(inv->moduleSet);
		elt = lyst_insert_last(inv->moduleSet, sourceModule);
		CHKVOID(elt);
	}
	else
	{
		inv = (Invitation *) lyst_data(invElt);
		if (ModuleSetMember(sourceModule, inv->moduleSet) == NULL)
		{
			elt = lyst_insert_last(inv->moduleSet, sourceModule);
			CHKVOID(elt);
		}
	}
#if RAMSDEBUG
PrintInvitationList(gWay);
#endif
}

static void	HandleDisinvitation(AmsModule module, void *userData,
			AmsEvent *eventRef, int unitNbr, int moduleNbr,
			int domainRoleNbr, int domainContinuumNbr,
			int domainUnitNbr, int subjectNbr)
{
	RamsGateway	*gWay = (RamsGateway *)userData;
	LystElt		invElt;
	LystElt		modulesElt;
	Invitation	*inv;
	Module		*sourceModule;

#if RAMSDEBUG
printf("in HandleDisinvitation subjectNbr=%d from unit=%d module=%d to \
domain role=%d domain cont = %d domain Unit=%d\n", subjectNbr, unitNbr,
moduleNbr, domainRoleNbr, domainContinuumNbr, domainUnitNbr);
#endif
	sourceModule = LookupModule(unitNbr, moduleNbr, gWay);
	if (ModuleIsMyself(sourceModule, gWay))
	{
		return;
	}

	if ((invElt = InvitationSetMember(domainUnitNbr, domainRoleNbr,
			domainContinuumNbr, subjectNbr, gWay->invitationSet))
			!= NULL)
	{
		inv = (Invitation *) lyst_data(invElt);
		if ((modulesElt = ModuleSetMember(sourceModule,
				inv->moduleSet)) != NULL)
		{
			lyst_delete(modulesElt);
			if (lyst_length(inv->moduleSet) == 0)
			{
				lyst_delete(invElt);
				DeleteInvitation(inv);
			}
		}
	}
#if RAMSDEBUG
PrintInvitationList(gWay);
#endif
}

static void	HandleUserEvent(AmsModule module, void *userData,
			AmsEvent *eventRef, int code, int dataLength,
			char *data)
{
#if RAMSDEBUG
PUTS("in HandleUserEvent");
#endif
}

static void	HandleAamsMessage(AmsModule module, void *userData,
			AmsEvent *eventRef, int continuumNbr, int unitNbr,
			int moduleNbr, int subjectNbr, int contentLength,
			char *content, int context, AmsMsgType msgType,
			int priority, unsigned char flowLabel)
{
	RamsGateway	*gWay = (RamsGateway *) userData;

#if RAMSDEBUG
printf("in HandleAamsMessage, subject = %d\n", subjectNbr);
#endif
	if (subjectNbr < 0)
	{
		/*	This is a private or announced message from a
		 *	module in the local message space.  Destination
		 *	continuum is the additive inverse of the subject
		 *	number.  Message content is an envelope.	*/

		ForwardTargetedMessage(gWay, flowLabel, content, contentLength);
	} 
	else if (subjectNbr > 0)
	{
		/*	This is a message that was published by a module
		 *	in the local message space.  Pass the entire
		 *	event structure to message forwarding logic.	*/

		ForwardPublishedMessage(gWay, *eventRef);
	}
}

/*	*	*	RAMS PDU handlers	*	*	*	*/

static int	HandleRPDU(RamsNode *fromNode, RamsGateway *gWay, char *msg)
{
	int		cc;
	int		sub;
	int		domainContinuum;
	int		domainUnit;
	int		domainRole;
	RamsNode	*declaredNode;
	char		petitionLine[512];

	cc = EnvelopeHeader(msg, Env_ControlCode);
	switch (cc)
	{
		case PetitionAssertion:
			sub = EnvelopeHeader(msg, Env_SubjectNbr);
			if (sub == 0 - fromNode->continuumNbr)
			{
				if (NoteDeclaration(fromNode, gWay) < 0)
				{
					return -1;
				}
			}
			else	/*	Not a declaration petition.	*/
			{
				declaredNode = Look_Up_DeclaredNeighbor(gWay,
						fromNode->continuumNbr);
				if (declaredNode == NULL)
				{
#if RAMSDEBUG
printf("Source of RPDU is not a declared source gateway: %d.\n",
fromNode->continuumNbr);
#endif
					return 0;
				}
			}

			domainContinuum = EnvelopeHeader(msg, Env_ContinuumNbr);
			domainUnit = EnvelopeHeader(msg, Env_PublishUnitNbr);
			domainRole = EnvelopeHeader(msg, Env_PublishRoleNbr);
			if (HandlePetitionAssertion(fromNode, gWay, sub,
				domainContinuum, domainUnit, domainRole, 0))
			{
				return -1;
			}

			isprintf(petitionLine, sizeof petitionLine,
					"%u %.255s %u %d %u %u %u\n",
					(unsigned int) fromNode->protocol,
					fromNode->gwEid, cc, sub,
					domainContinuum, domainUnit,
					domainRole);
			if (_petitionLog(petitionLine, 0) < 0)
			{
				putErrmsg("Can't log petition assertion.",
						petitionLine);
				return -1;
			}

			return 0;

		case PetitionCancellation:
			sub = EnvelopeHeader(msg, Env_SubjectNbr);
			domainContinuum = EnvelopeHeader(msg, Env_ContinuumNbr);
			domainUnit = EnvelopeHeader(msg, Env_PublishUnitNbr);
			domainRole = EnvelopeHeader(msg, Env_PublishRoleNbr);
			if (HandlePetitionCancellation(fromNode, gWay, sub,
				domainContinuum, domainUnit, domainRole, 0))
			{
				return -1;
			}

			isprintf(petitionLine, sizeof petitionLine,
					"%u %.255s %u %d %u %u %u\n",
					(unsigned int) fromNode->protocol,
					fromNode->gwEid, cc, sub,
					domainContinuum, domainUnit,
					domainRole);
			if (_petitionLog(petitionLine, 0) < 0)
			{
				putErrmsg("Can't log petition cancellation.",
						petitionLine);
				return -1;
			}

			return 0;

		case PublishOnReception:
			if (HandlePublishedMessage(fromNode, gWay, msg))
			{
				return -1;
			}

			return 0;

		case SendOnReception:
			if (HandlePrivateMessage(gWay, msg))
			{
				return -1;
			}

			return 0;

		case AnnounceOnReception:
			if (HandleAnnouncedMessage(fromNode, gWay, msg))
			{
				return -1;
			}

			return 0;

		default:
#if RAMSDEBUG
printf("Unknown RPDU CC: %d.\n", cc);
#endif
			return 0;
	}
}

static int	AssertPetition(RamsGateway *gWay, Petition *pet)
{
	Lyst		assertionSet;
	LystElt		elt;
	RamsNode	*node;

	/*	The petition may or may not be assertable.
	 *
	 *	If there are modules in the local message space
	 *	that subscribe to the subject and domain of this
	 *	petition, then the petition is assertable.  If not,
	 *	then the petition is assertable only if the RAMS
	 *	network is a tree and the DGS of the petition is
	 *	non-empty (i.e., other nodes care about messages
	 *	that satisfy this petition).				*/

	if (lyst_length(pet->DistributionModuleSet) == 0)
	{
		if (gWay->netType == MESHTYPE
		|| lyst_length(pet->DestinationNodeSet) == 0)
		{
			return 0;	/*	Not assertable.		*/
		}
	}

	/*	The petition is assertable, so we send it to all
	 *	members of the petition's computed propagation set.	*/

#if RAMSDEBUG
char	*env = pet->specification->envelope;
printf("<assert petition> assert petition cId = %d unit = %d role = %d \
subject = %d \n",  EnvelopeHeader(env, Env_ContinuumNbr),
EnvelopeHeader(env, Env_UnitField),
EnvelopeHeader(env, Env_SourceIDField),
EnvelopeHeader(env, Env_SubjectNbr));
#endif
	assertionSet = AssertionSet(gWay, pet);
	CHKERR(assertionSet);
	for (elt = lyst_first(assertionSet); elt; elt = lyst_next(elt))
	{
		node = (RamsNode *) lyst_data(elt);
		if (SendRPDU(gWay, node->continuumNbr, 1,
				pet->specification->envelope, ENVELOPELENGTH))
		{
			ErrMsg("Petition assertion error.");
			return -1;
		}
	}

	AddNodeSets(pet->SourceNodeSet, assertionSet);
	lyst_destroy(assertionSet);
	return 0;
}

static int	CancelPetition(RamsGateway *gWay, Petition *pet)
{
	int		continuumNbr;
	int		unitNbr;
	int		sourceId;
	int		subjectNbr;
	LystElt		sgsElt;
	LystElt		nextElt;
	RamsNode	*node;

	continuumNbr = EnvelopeHeader(pet->specification->envelope,
		 	Env_ContinuumNbr);
	unitNbr = EnvelopeHeader(pet->specification->envelope,
			 Env_PublishUnitNbr);
	sourceId = EnvelopeHeader(pet->specification->envelope,
			 Env_PublishRoleNbr);
	subjectNbr = EnvelopeHeader(pet->specification->envelope,
			 Env_SubjectNbr);

	/*	If the DGS of the petition is empty (no other nodes
	 *	care about messages that satisfy this petition), then
	 *	the cancellation is sent to all members of the
	 *	petition's SGS, i.e., all potential sources of messages
	 *	on this subject.					*/

	if (lyst_length(pet->DestinationNodeSet) == 0)
	{
#if RAMSDEBUG
printf("<cancel petition> cancel petition cId = %d unit = %d role = %d \
subject = %d \n",  continuumNbr, unitNbr, sourceId, subjectNbr);
#endif
		/*	No further interest in these messages at all.	*/

		for (sgsElt = lyst_first(pet->SourceNodeSet); sgsElt;
				sgsElt = nextElt)
		{
			nextElt = lyst_next(sgsElt);
			node = (RamsNode *) lyst_data(sgsElt);
			if (SendNewRPDU(gWay, node->continuumNbr, 1, NULL,
					continuumNbr, unitNbr, sourceId, 0,
					PetitionCancellation, subjectNbr) < 0)
			{
				ErrMsg("Can't sent Petition Cancellation.");
				return -1;
			}

			lyst_delete(sgsElt);
		}

		return 0;
	}

	/*	Otherwise, if there is exactly one node in the DGS
	 *	and that node is in the petition's SGS, then we send
	 *	the cancellation to that node.  We want to turn off
	 *	this potential source of messages that we would
	 *	simply have to send back again.				*/

	if (lyst_length(pet->DestinationNodeSet) == 1)
	{
		node = (RamsNode *)
				lyst_data(lyst_first(pet->DestinationNodeSet));
		sgsElt = NodeSetMember(node, pet->SourceNodeSet);
		if (sgsElt)
		{
#if RAMSDEBUG
printf("<cancel petition> cancel petition cId = %d unit = %d role = %d \
subject = %d \n",  continuumNbr, unitNbr, sourceId, subjectNbr);
#endif
			if (SendNewRPDU(gWay, node->continuumNbr, 1, NULL,
					continuumNbr, unitNbr, sourceId, 0,
					PetitionCancellation, subjectNbr) < 0)
			{
				ErrMsg("Can't send Petition Cancellation.");
				return -1;
			}

			lyst_delete(sgsElt);
			return 0;
		}
	}

	/*	Otherwise, we don't cancel.				*/

	return 0;
}

static int	NoteDeclaration(RamsNode *fromNode, RamsGateway *gWay)
{
#if RAMSDEBUG
printf("<note declaration> received declaration from %d\n",
fromNode->continuumNbr);
#endif
	if (Look_Up_DeclaredNeighbor(gWay, fromNode->continuumNbr) == NULL)
	{
		if (lyst_insert_last(gWay->declaredNeighbors, fromNode) == NULL)
		{
			ErrMsg("Can't note declaration.");
			return -1;
		}

#if RAMSDEBUG
printf("<note declaration> added %d as a declared neighbor\n",
fromNode->continuumNbr);
#endif
		gWay->declaredNeighborsCount++;
	}

	return 0;
}

static int	HandlePetitionAssertion(RamsNode *fromNode, RamsGateway *gWay,
			int subjectNbr, int domainContinuum, int domainUnit,
			int domainRole, int fromPlayback)
{
	LystElt		elt;
	Petition	*pet;
	LystElt		nodeElt;
	Lyst		assertionSet;
	Petition	*aPet;	

#if RAMSDEBUG
PUTS("<handle petition assertion> receive petition assertion RPDU (2)");
printf("<handle petition assertion> petition from %d for subject %d \
continuum %d unit %d role %d\n", fromNode->continuumNbr, subjectNbr,
domainContinuum, domainUnit, domainRole);
#endif
	/*	First, insert the asserting node into the DGS of this
	 *	petition if the petition is already known.		*/

	for (elt = lyst_first(gWay->petitionSet); elt; elt = lyst_next(elt))
	{
		pet = (Petition *) lyst_data(elt);
		if (PetitionMatchesDomain(pet, domainContinuum,
				domainRole, domainUnit, subjectNbr))
		{
			/*	This petition already exists.  If
			 *	this neighbor has already asserted
			 *	it, then process an imputed prior
			 *	cancellation before proceeding.		*/

			nodeElt = NodeSetMember(fromNode,
					pet->DestinationNodeSet);
			if (nodeElt)
			{
				if (pet->stateIsFromPlayback)
				{
					/*	This is not a true
					 *	reassertion, so just
					 *	note that the petition
					 *	state is confirmed and
					 *	ignore the assertion.	*/

					pet->stateIsFromPlayback = fromPlayback;
					return 0;
				}

				if (HandlePetitionCancellation(fromNode,
						gWay, subjectNbr,
						domainContinuum, domainUnit,
						domainRole, fromPlayback) < 0)
				{
					ErrMsg("Can't handle imputed cancel");
					return -1;
				}
			}

			/*	Now proceed with the assertion.		*/

			if (lyst_insert_last(pet->DestinationNodeSet,
					fromNode) == NULL)
			{
				ErrMsg("Can't add node to DGS");
				return -1;
			}
#if RAMSDEBUG
PUTS("<handle petition assertion> add into existing DestinationNodeSet");
#endif
			break;
		}
	}

	if (elt == NULL)
	{
		/*	This is a hitherto unknown petition.		*/
#if RAMSDEBUG
PUTS("<handle petition assertion> create new petition");
#endif
		pet = ConstructPetition(domainContinuum, domainRole,
				domainUnit, subjectNbr, PetitionAssertion);
		if (pet == NULL
		|| lyst_insert_last(gWay->petitionSet, pet) == NULL
		|| lyst_insert_last(pet->DestinationNodeSet, fromNode) == NULL)
		{
			ErrMsg("Can't add new petition.");
			return -1;
		}
	}

	/*	That node is now a member of the DGS of this petition.
	 *	If it's a newly asserted petition, subscribe locally.	*/

	if (lyst_length(pet->DestinationNodeSet) == 1)
	{
		if (domainContinuum == 0
		|| domainContinuum == gWay->amsMib->localContinuumNbr)
		{
			if (ams_subscribe(gWay->amsModule, domainRole,
				domainContinuum, domainUnit, subjectNbr, 10,
				0, AmsTransmissionOrder, AmsAssured) < 0)
			{
				ErrMsg("Can't subscribe for newly asserted \
petition.");
#if RAMSDEBUG
PUTS("<handle petition assertion> error on ams_subscribe");
#endif
				return -1;
			}
#if RAMSDEBUG
PUTS("<handle petition assertion> local subscription okay");
#endif
		}
	}

	/*	If this assertion is from playback, then we're just
	 *	recovering local RAMS gateway state.  No need to
	 *	propagate to other nodes: they either received and
	 *	retained the original RPDU or else recovered (or will
	 *	recover) it on restart.					*/

	pet->stateIsFromPlayback = fromPlayback;
	if (pet->stateIsFromPlayback)
	{
#if RAMSDEBUG
PUTS("<handle petition assertion> recovering");
PrintGatewayState(gWay);
#endif
		return 0;
	}

	/*	If the subject cited is the pseudo-subject for some
	 *	continuum then all relevant petition relationships
	 *	with this node (which is the conduit to that
	 *	continuum) must be established.				*/

	if (subjectNbr < 0)
	{
#if RAMSDEBUG
printf("<handle petition assertion> node is conduit for continuum %d\n",
0 - subjectNbr);
#endif
		for (elt = lyst_first(gWay->petitionSet); elt;
				elt = lyst_next(elt))
		{
			aPet = (Petition *) lyst_data(elt);
			if (SamePetition(aPet, pet))
			{
				continue;
			}

			if (lyst_length(aPet->DistributionModuleSet) > 0
			|| (gWay->netType == TREETYPE
				&& lyst_length(aPet->DestinationNodeSet) > 0))
			{
				/*	Petition is assertable.		*/

				assertionSet = AssertionSet(gWay, aPet);
				CHKERR(assertionSet);
				nodeElt = NodeSetMember(fromNode, assertionSet);
				lyst_destroy(assertionSet);
				if (nodeElt)
				{
#if RAMSDEBUG
printf("<handle petition assertion> propagate petition to %d: petition \
cId = %d unit = %d role = %d sub = %d\n", fromNode->continuumNbr,
EnvelopeHeader(aPet->specification->envelope, Env_ContinuumNbr),
EnvelopeHeader(aPet->specification->envelope, Env_PublishUnitNbr),
EnvelopeHeader(aPet->specification->envelope, Env_PublishRoleNbr),
EnvelopeHeader(aPet->specification->envelope, Env_SubjectNbr));
#endif
					if (SendRPDU(gWay,
						fromNode->continuumNbr, 1,
						aPet->specification->envelope,
						ENVELOPELENGTH) < 0)
					{
#if RAMSDEBUG
PUTS("<handle petition assertion> propagation error");
#endif
						ErrMsg("Propagation error.");
						return -1;
					}

			    		if (lyst_insert_last
						(aPet->SourceNodeSet,
						fromNode) == NULL)
					{
						ErrMsg("SGS error.");
						return -1;
					}
				}
			}
		}
	}

	/*	Now propagate the petition assertion to neighbors as
	 *	needed.							*/
		
	if (AssertPetition(gWay, pet) < 0)
	{
#if RAMSDEBUG
PUTS("<handle petition assertion> failed propagating petition assertion");
#endif
		ErrMsg("Petition assertion propagation error.");
		return -1;
	}

#if RAMSDEBUG
PUTS("<handle petition assertion>");
PrintGatewayState(gWay);
#endif
	return 0;
}

static int	HandlePetitionCancellation(RamsNode *fromNode,
			RamsGateway *gWay, int petSubject, int domainContinuum,
			int domainUnit, int domainRole, int fromPlayback)
{
	LystElt		elt;
	LystElt		dgsElt;
	LystElt		sgsElt;
	Petition	*pet;
	Petition	*aPet;
	int		continuumNbr;
	int		unitNbr;
	int		sourceId;
	int		subjectNbr;

#if RAMSDEBUG
PUTS("<handle petition cancellation> receive petition cancellation RPDU (3)");
printf("<handle petition cancellation> petition from %d for subject %d \
continuum %d unit %d role %d\n", fromNode->continuumNbr, petSubject,
domainContinuum, domainUnit, domainRole);
#endif
	/*	Delete this neighbor from DGS of this petition.		*/

	for (elt = lyst_first(gWay->petitionSet); elt; elt = lyst_next(elt))
	{
		pet = (Petition *) lyst_data(elt);
		if (PetitionMatchesDomain(pet, domainContinuum,
				domainRole, domainUnit, petSubject))
		{
			if ((dgsElt = NodeSetMember(fromNode,
					pet->DestinationNodeSet)) != NULL)
			{
				lyst_delete(dgsElt);				
#if RAMSDEBUG
printf("<handle petition cancellation> removed continuum %d from DGS for %d \n",
fromNode->continuumNbr, petSubject);
#endif
				break;
			}
			else
			{
#if RAMSDEBUG
PUTS("<handle petition cancellation> node already deleted from DGS");
#endif
				return 0;
			}
		}
	}

	if (elt == NULL)
	{
		/*	No such petition, so there's nothing to do.	*/
#if RAMSDEBUG
PUTS("<handle petition cancellation> no matching petition");
#endif
		return 0;
	}

	/*	If the DGS of this petition now has zero members,
	 *	unsubscribe locally.					*/

	if (lyst_first(pet->DestinationNodeSet) == NULL)
	{
		if (domainContinuum == 0
		|| domainContinuum == gWay->amsMib->localContinuumNbr)
		{
			if (ams_unsubscribe(gWay->amsModule, domainRole,
				domainContinuum, domainUnit, petSubject) < 0)
			{
				ErrMsg("Can't unsubscribe canceled petition.");
#if RAMSDEBUG
PUTS("<handle petition cancellation> error on ams_unsubscribe");
#endif
				return -1;
			}
#if RAMSDEBUG
PUTS("<handle petition cancellation> local unsubscription okay");
#endif
		}
	}

	/*	If this cancellation is from playback, then we're
	 *	just recovering local RAMS gateway state.  No need
	 *	to propagate to other nodes: they either received and
	 *	retained the original RPDU or else recovered (or will
	 *	recover) it on restart.					*/

	pet->stateIsFromPlayback = fromPlayback;
	if (pet->stateIsFromPlayback)
	{
#if RAMSDEBUG
PUTS("<handle petition cancellation> recovering");
PrintGatewayState(gWay);
#endif
		return 0;
	}

	/*	If the subject cited is the pseudo-subject for the
	 *	neighbor's own continuum, the neighbor is retracting
	 *	itself.							*/

	if (petSubject != (0 - fromNode->continuumNbr))
	{
		/*	Neighbor is NOT retracting itself, so the
		 *	only thing left to do is to cancel this
		 *	petition if it is no longer assertable.
		 *	The assertability of all other petitions
		 *	is unaffected by this cancellation, so
		 *	there's no need to cancel anything else.	*/

		if (!PetitionIsAssertable(gWay, pet))
		{
			if (CancelPetition(gWay, pet) < 0)
			{
				ErrMsg("CancelPetition failed");
#if RAMSDEBUG
PUTS("<handle petition cancellation> petition cancellation fails");
#endif
				return -1;
			}
		}

#if RAMSDEBUG
PUTS("<handle petition cancellation>");
PrintGatewayState(gWay);
#endif
		return 0;
	}

	/*	The neighbor is retracting itself, so must remove it
	 *	from SGS and DGS of all petitions (per interop testing
	 *	at APL) and from the list of declared neighbors.
	 *
	 *	First, the petitions.					*/

#if RAMSDEBUG
printf("<handle petition cancellation> continuum nbr %d is retracting\n",
fromNode->continuumNbr);
printf("<handle petition cancellation> subjectNbr = %d\n", petSubject);
#endif
	for (elt = lyst_first(gWay->petitionSet); elt; elt = lyst_next(elt))
	{
		aPet = (Petition *) lyst_data(elt);
		continuumNbr = EnvelopeHeader(aPet->specification->envelope,
			 	Env_ContinuumNbr);
		unitNbr = EnvelopeHeader(aPet->specification->envelope,
				 Env_PublishUnitNbr);
		sourceId = EnvelopeHeader(aPet->specification->envelope,
				 Env_PublishRoleNbr);
		subjectNbr = EnvelopeHeader(aPet->specification->envelope,
				 Env_SubjectNbr);
		sgsElt = NodeSetMember(fromNode, aPet->SourceNodeSet);
		if (sgsElt)
		{
#if RAMSDEBUG
printf("<handle petition cancellation> deleting %d from SGS of petition \
cId = %d unit = %d role = %d sub = %d\n", fromNode->continuumNbr,
continuumNbr, unitNbr, sourceId, subjectNbr);
#endif
			lyst_delete(sgsElt);
		}

		dgsElt = NodeSetMember(fromNode, aPet->DestinationNodeSet);
		if (dgsElt)	/*	Add per APL interop testing.	*/
		{
#if RAMSDEBUG
printf("<handle petition cancellation> deleting %d from DGS of petition \
cId = %d unit = %d role = %d sub = %d\n", fromNode->continuumNbr,
continuumNbr, unitNbr, sourceId, subjectNbr);
#endif
			lyst_delete(dgsElt);
		}

		if (sgsElt || dgsElt)
		{
			/*	Send own cancellation, for this
			 *	petition, back to the retracting
			 *	node.					*/

			if (SendNewRPDU(gWay, fromNode->continuumNbr,
					1, NULL, continuumNbr, unitNbr,
					sourceId, 0, PetitionCancellation,
					subjectNbr) < 0)
			{
				ErrMsg("Can't send petition cancel");
#if RAMSDEBUG
PUTS("<handle petition cancellation> own petition cancel RPDU fails");
#endif
				return -1;
			}
		}

#if RAMSDEBUG
printf("<handle petition cancellation> canceling petition cId = %d unit = %d \
role = %d sub = %d\n", continuumNbr, unitNbr, sourceId, subjectNbr);
#endif
		/*	May now be able to cancel petition altogether.	*/

		if (!PetitionIsAssertable(gWay, aPet))
		{
			if (CancelPetition(gWay, aPet) < 0)
			{
				ErrMsg("CancelPetition failed");
#if RAMSDEBUG
PUTS("<handle petition cancellation> petition cancellation fails");
#endif
				return -1;
			}
		}
	}

	/*	Finally, remove this node from the list of declared
	 *	neighbors.						*/

	if ((elt = NodeSetMember(fromNode, gWay->declaredNeighbors)) != NULL)
	{
#if RAMSDEBUG
printf("<handle petition cancellation> remove continuum %d from list of \
declared neighbors\n", fromNode->continuumNbr);
#endif
		lyst_delete(elt);
	}

#if RAMSDEBUG
PUTS("<handle petition cancellation>");
PrintGatewayState(gWay);
#endif
	return 0;
}

static int	HandlePublishedMessage(RamsNode *fromNode, RamsGateway *gWay,
			char *msg)
{
	int		amsMemory = getIonMemoryMgr();
	LystElt		elt;
	LystElt		nodesElt;
	LystElt		modulesElt;
	Lyst		destinationNodes;
	Lyst		destinationModules;
	Module		*module;
	RamsNode	*ramsNode;
	Petition	*pet;
	int		localcn = gWay->amsMib->localContinuumNbr;

#if RAMSDEBUG
PUTS("<handle published message> receive published message RPDU (4)");
printf("<handle published message> from contId=%d unit=%d moduleId=%d\n",
EnclosureHeader(EnvelopeContent(msg, -1), Enc_ContinuumNbr),
EnclosureHeader(EnvelopeContent(msg, -1), Enc_UnitNbr),
EnclosureHeader(EnvelopeContent(msg, -1), Enc_ModuleNbr));
#endif
	destinationNodes = lyst_create_using(amsMemory);
	destinationModules = lyst_create_using(amsMemory);
	CHKERR(destinationNodes);
	CHKERR(destinationModules);

	/*	For each petition that is satisfied by this AAMS
	 *	message, add all members of petition's DGS (except
	 *	the sending node) to the destinationNodes list and
	 *	add all members of the petition's DMS to the
	 *	destinationModules list -- excluding duplicates.	*/

	for (elt = lyst_first(gWay->petitionSet); elt; elt = lyst_next(elt))
	{
		pet = (Petition *) lyst_data(elt);
		if (EnclosureSatisfiesPetition(gWay->amsModule, msg, pet))
		{
			for (nodesElt = lyst_first(pet->DestinationNodeSet);
				nodesElt; nodesElt = lyst_next(nodesElt))
			{
				ramsNode = (RamsNode *) lyst_data(nodesElt);
				if (ramsNode->continuumNbr
						== fromNode->continuumNbr)
				{
					continue;
				}

				if (!NodeSetMember(ramsNode, destinationNodes))
				{
					if (lyst_insert_last(destinationNodes,
							ramsNode) == NULL)
					{
						ErrMsg("Can't add node.");
						return -1;
					}
				}
			}

			for (modulesElt = lyst_first
				(pet->DistributionModuleSet); modulesElt;
				modulesElt = lyst_next(modulesElt))
			{
				module = (Module *) lyst_data(modulesElt);
				if (!ModuleSetMember(module,
						destinationModules))
				{
					if (lyst_insert_last(destinationModules,
							module) == NULL)
					{
						ErrMsg("Can't add module.");
						return -1;
					}
				}
			}
		}
	}			

	/*	Now send the message to all of those nodes and modules.	*/

	for (nodesElt = lyst_first(destinationNodes); nodesElt;
			nodesElt = lyst_next(nodesElt))
	{
		ramsNode = (RamsNode *) lyst_data(nodesElt);
#if RAMSDEBUG
printf("<handle published message> send message to continuum %d\n",
ramsNode->continuumNbr);
#endif
		if (SendRPDU(gWay, ramsNode->continuumNbr, 1,
				msg, ENVELOPELENGTH + EnvelopeHeader(msg,
				Env_EnclosureLength)) < 0)
		{
#if RAMSDEBUG
PUTS("<handle published message> sending to continuum failed");
#endif
			ErrMsg("Error in sending published message to node.");
			return -1;
		}
	}
    
	for (modulesElt = lyst_first(destinationModules); modulesElt;
			modulesElt = lyst_next(modulesElt))
	{
		module = (Module *) lyst_data(modulesElt);
#if RAMSDEBUG
printf("<handle published message> send message to unit %d module %d\n",
module->unitNbr, module->nbr);
#endif
		/*	Send enclosure as content; subject number is
		 *	additive inverse of the local continuum number.	*/

		if (ams_send(gWay->amsModule, localcn, module->unitNbr,
				module->nbr, (0 - localcn), 8, 0,
				EnvelopeHeader(msg, Env_EnclosureLength),
				EnvelopeContent(msg, -1), 0) < 0)
		{
#if RAMSDEBUG
PUTS("<handle published message> sending to module failed");
#endif
			ErrMsg("Error in sending published message to module.");
			return -1;
		}
	}

	lyst_destroy(destinationNodes);
	lyst_destroy(destinationModules);
	return 0;
}

static int	HandlePrivateMessage(RamsGateway *gWay, char *msg)
{
	int		destinationContinuumNbr;
	RamsNode	*ramsNode;
	int		unitNbr;
	int		moduleNbr;

	destinationContinuumNbr =  EnvelopeHeader(msg, Env_ContinuumNbr);
#if RAMSDEBUG
PUTS("<handle private message> receive private message RPDU (5)");
printf("<handle private message> message is for cId = %d, local cId = %d\n",
destinationContinuumNbr, gWay->amsMib->localContinuumNbr);
printf("<handle private message> message content is %s\n",
msg + ENVELOPELENGTH + AMSMSGHEADER);
#endif
	if (destinationContinuumNbr != gWay->amsMib->localContinuumNbr)
	{
		/*	Destination module is in another continuum.	*/

		ramsNode = GetConduitForContinuum(destinationContinuumNbr,
				gWay);
		if (ramsNode == NULL)
		{
#if RAMSDEBUG
printf("<handle private message> no conduit for continuum %d\n",
destinationContinuumNbr);
#endif
			return 0;
		}
#if RAMSDEBUG
printf("<handle private message> send message to continuum %d\n",
ramsNode->continuumNbr);
#endif
		if (SendRPDU(gWay, ramsNode->continuumNbr, 1, msg,
				ENVELOPELENGTH + EnvelopeHeader(msg,
				Env_EnclosureLength)) < 0)
		{
#if RAMSDEBUG
PUTS("<handle private message> sending to continuum failed");
#endif
			ErrMsg("Error in sending private message to node.");
			return -1;
		}

		return 0;
	}

	/*	Destination module is in the local continuum.		*/

	unitNbr = EnvelopeHeader(msg, Env_DestUnitNbr);
	moduleNbr = EnvelopeHeader(msg, Env_DestModuleNbr);
	if (MessageIsInvited(gWay, msg))
	{
#if RAMSDEBUG
printf("<handle private message> send message to con=%d unit=%d module=%d \
sub=0\n", destinationContinuumNbr, unitNbr, moduleNbr);
#endif
		if (ams_send(gWay->amsModule, destinationContinuumNbr,
				unitNbr, moduleNbr,
				(0 - destinationContinuumNbr), 8, 0,
				EnvelopeHeader(msg, Env_EnclosureLength),
				EnvelopeContent(msg, -1), 0) < 0)
		{
#if RAMSDEBUG
PUTS("<handle private message> sending to module failed");
#endif
			ErrMsg("Error in sending private message to module.");
			return -1;
		}

		return 0;
	}

	/*	Message can't be delivered to module -- no invitation.	*/

#if RAMSDEBUG
printf("<handle private message> uninvited message to con=%d unit=%d module=%d \
sub=%d\n", destinationContinuumNbr, unitNbr, moduleNbr,
EnvelopeHeader(msg, Env_SubjectNbr));
#endif
	return 0;
}

static int	HandleAnnouncedMessage(RamsNode* fromNode,
			RamsGateway *gWay, char *msg)
{
	int		destinationContinuumNbr;
	int		domainRole;
	int		domainUnit;
	LystElt		elt;
	LystElt		modulesElt;
	Lyst		moduleList;
	Invitation	*inv;
	Module		*amsModule;
	RamsNode	*ramsNode;
	int		localcn = gWay->amsMib->localContinuumNbr;

	destinationContinuumNbr =  EnvelopeHeader(msg, Env_ContinuumNbr);
#if RAMSDEBUG
PUTS("<handle announced message> receive announced message RPDU (6)");
printf("<handle announced message> message is for cId = %d, local cId = %d\n",
destinationContinuumNbr, gWay->amsMib->localContinuumNbr);
printf("<handle announced message> message content is %s\n",
msg + ENVELOPELENGTH + AMSMSGHEADER);
#endif
	/*	If the destination continuum is zero ("all continua")
	 *	*and* the network is a tree, propagate the message to
	 *	all neighboring nodes except the sending node.		*/

	if (destinationContinuumNbr == 0)
	{
		if (gWay->netType == TREETYPE)
		{
			for (elt = lyst_first(gWay->declaredNeighbors);
					elt; elt = lyst_next(elt))
			{
				ramsNode = (RamsNode *) lyst_data(elt);
				if (ramsNode->continuumNbr
						== fromNode->continuumNbr)
				{
					continue;
				}
#if RAMSDEBUG
printf("<handle announced message> send message to continuum %d\n",
ramsNode->continuumNbr);
#endif
				if (SendRPDU(gWay, ramsNode->continuumNbr, 1,
						msg, ENVELOPELENGTH
						+ EnvelopeHeader(msg,
						Env_EnclosureLength)))
				{
#if RAMSDEBUG
PUTS("<handle announced message> sending to continuum failed");
#endif
					ErrMsg("Error in sending announced \
message to node.");
					return -1;
				}
			}
		}
	}
	else
	{
		/*	The destination continuum is only a single
		 *	specified continuum.  If the network is a
		 *	mesh, then only the gateway for that continuum
		 *	has received a copy of the message; since a
		 *	copy was received, this gateway would have to
		 *	be in the destination continuum.
		 *
		 *	So if the local continuum is *not* the
		 *	destination continuum then the network must
		 *	be a tree rather than a mesh and the message
		 *	must be relayed to the conduit for the
		 *	destination continuum.				*/

		if (destinationContinuumNbr != localcn)
		{
			ramsNode = GetConduitForContinuum
					(destinationContinuumNbr, gWay);
			if (ramsNode)
			{
#if RAMSDEBUG
printf("<handle announced message> send to continuum %d\n",
ramsNode->continuumNbr);
#endif
				if (SendRPDU(gWay, ramsNode->continuumNbr, 1,
						msg, ENVELOPELENGTH
						+ EnvelopeHeader(msg,
						Env_EnclosureLength)))
				{
#if RAMSDEBUG
PUTS("<handle announced message> sending to continuum failed");
#endif
					ErrMsg("Error in sending announced \
message to node.");
					return -1;
				}
			}
#if RAMSDEBUG
else printf("<handle announced message> no conduit for continuum %d\n",
destinationContinuumNbr);
#endif
			return 0;	/*	Nothing more to do.	*/
		}
	}
  
	/*	The destination continuum is either the local continuum
	 *	or all continua, so must announce the message locally.	*/

	moduleList = lyst_create_using(getIonMemoryMgr());
	CHKERR(moduleList);
	domainUnit = EnvelopeHeader(msg, Env_DestUnitNbr);
	domainRole = EnvelopeHeader(msg, Env_DestRoleNbr);

	/*	Compile list of all eligible message recipients,
	 *	excluding all duplicates.				*/

	for (elt = lyst_first(gWay->invitationSet); elt; elt = lyst_next(elt))
	{
		inv = (Invitation *) lyst_data(elt);
		if (EnclosureSatisfiesInvitation(gWay, msg, inv))
		{
			for (modulesElt = lyst_first(inv->moduleSet);
				modulesElt; modulesElt = lyst_next(modulesElt))
			{
				amsModule = (Module *) lyst_data(modulesElt);
				if (ModuleIsInAnnouncementDomain(gWay,
						amsModule, domainUnit,
						domainRole))
				{
					if (ModuleSetMember(amsModule,
							moduleList))
					{
						/*	Already listed.	*/

						continue;
					}

					if (lyst_insert_last(moduleList,
							amsModule) == NULL)
					{
						ErrMsg("Can't add module.");
						return -1;
					}
				}
			}
		}
	}

	/*	Now send the message to all of those modules.		*/

#if RAMSDEBUG
PUTS("<handle announced message> send message to modules:");
#endif
	for (modulesElt = lyst_first(moduleList); modulesElt;
			modulesElt = lyst_next(modulesElt))
	{
		amsModule = (Module *) lyst_data(modulesElt);
#if RAMSDEBUG
printf("<handle announced message>        unit=%d module=%d\n",
amsModule->unitNbr, amsModule->nbr);
#endif
		if (ams_send(gWay->amsModule, localcn, amsModule->unitNbr,
				amsModule->nbr, (0 - localcn), 8, 0,
				EnvelopeHeader(msg, Env_EnclosureLength),
				EnvelopeContent(msg, -1), 0) < 0)
		{
#if RAMSDEBUG
PUTS("<handle announced message> sending to module failed");
#endif
			ErrMsg("Error in sending announced message to module.");
			return -1;
		}
	}

	lyst_destroy(moduleList);
	return 0;
}

/*	*	Utility functions for AMS event handlers	*	*/
	
static int	AddPetitioner(Module *sourceModule, RamsGateway *gWay,
			int domainRole, int domainContinuum, int domainUnit,
			int subjectNbr)
{
	LystElt		elt;
	Petition	*pet;

#if RAMSDEBUG
printf("<add petitioner> adding module unit %d nbr %d to petition for \
subject %d continuum %d unit %d role %d\n", sourceModule->unitNbr,
sourceModule->nbr, subjectNbr, domainContinuum, domainUnit, domainRole);
#endif
	for (elt = lyst_first(gWay->petitionSet); elt; elt = lyst_next(elt))
	{
		pet = (Petition *) lyst_data(elt);
		if (PetitionMatchesDomain(pet, domainContinuum,
				 domainRole, domainUnit, subjectNbr))
		{
			if (ModuleSetMember(sourceModule,
					 pet->DistributionModuleSet))
			{
				return 0;	/*	Already listed.	*/

				/*	Note: no need to assert the
				 *	petition.  Since this module
				 *	was already in the petition's
				 *	DMS, that DMS was already non-
				 *	empty, so the petition had
				 *	already been asserted.		*/
			}

			if (lyst_insert_last(pet->DistributionModuleSet,
					sourceModule) == NULL)
			{
				ErrMsg("Can't add module to petition.");
				return -1;
			}

			break;
		}
	}

	if (elt == NULL)
	{
#if RAMSDEBUG
PUTS("<add petitioner> must create new petition");
#endif
		pet = ConstructPetition(domainContinuum, domainRole,
				domainUnit, subjectNbr, PetitionAssertion);
		if (pet == NULL
		|| lyst_insert_last(gWay->petitionSet, pet) == NULL
		|| lyst_insert_last(pet->DistributionModuleSet, sourceModule)
				== NULL)
		{
			ErrMsg("Can't add module to petition.");
			return -1;
		}
	}

	pet->stateIsFromPlayback = 0;
	if (AssertPetition(gWay, pet) < 0)
	{
		 ErrMsg("Can't assert petition in AddPetitioner.");
		 return -1;
	}

#if RAMSDEBUG
PUTS("<add petitioner>");
PrintGatewayState(gWay);
#endif
	 return 0;
}

static int	RemovePetitioner(Module *sourceModule, RamsGateway *gWay,
			int domainRole, int domainContinuum, int domainUnit,
			int subjectNbr)
{
	 LystElt	elt;
	 LystElt	modulesElt;
	 Petition	*pet;
	 
#if RAMSDEBUG
printf("<remove petitioner> removing module unit %d nbr %d from petition for \
subject %d continuum %d unit %d role %d\n", sourceModule->unitNbr,
sourceModule->nbr, subjectNbr, domainContinuum, domainUnit, domainRole);
#endif
	for (elt = lyst_first(gWay->petitionSet); elt; elt = lyst_next(elt))
	{
		pet = (Petition *) lyst_data(elt);
		if (PetitionMatchesDomain(pet, domainContinuum,
				 domainRole, domainUnit, subjectNbr))
		{
			 if ((modulesElt = ModuleSetMember(sourceModule,
					 pet->DistributionModuleSet)) != NULL)
			 {
				 lyst_delete(modulesElt);		
			 }

			 break;
		 }
	}

	if (elt == NULL)   	/*	No such petition.		*/
	{
#if RAMSDEBUG
PUTS("<remove petitioner> no matching petition");
#endif
		return 0;
	}

	/*	May now be able to cancel the petition altogether.	*/

	if (!PetitionIsAssertable(gWay, pet))
	{
		if (CancelPetition(gWay, pet) < 0)
		{
			ErrMsg("CancelPetition failed.");
			return -1;
		}
	}

#if RAMSDEBUG
PUTS("<remove petitioner>");
PrintGatewayState(gWay);
#endif
	return 0;
}

static int	ForwardPublishedMessage(RamsGateway *gWay, AmsEvent amsEvent)
{
	LystElt		elt;
	Lyst		nodesList;
	RamsNode	*node;
	Enclosure	*enc;
	Petition	*pet;
	char		*content;
	AmsMsgType	msgType;
	int		continuumNbr;
	int		unitNbr;
	int		moduleNbr;
	int		subjectNbr;
	int		contentLen;
	int		context;
	int		priority;
	unsigned char	flowLabel;
	int		sourceRoleNbr;

	ams_parse_msg(amsEvent, &continuumNbr, &unitNbr, &moduleNbr,
			&subjectNbr, &contentLen, &content, &context,
			&msgType, &priority, &flowLabel);
#if RAMSDEBUG
PUTS("<forward published message> forward published message");
printf("<forward published message> contentLength = %d\n", contentLen);
#endif
	sourceRoleNbr = RoleNumber(gWay->amsModule, unitNbr, moduleNbr);

	/*	Package the message in an Enclosure structure so that
	 *	it can be forwarded.					*/

	enc = ConstructEnclosure(continuumNbr, unitNbr, moduleNbr, subjectNbr,
			contentLen, content, context, msgType, priority,
			flowLabel);
	CHKERR(enc);

	/*	Normally the published message's flow label is used
	 *	as the Bundle Protocol Class of Service (which includes
	 *	priority) in the event that the message is forwarded
	 *	over a BP RAMS network.  The publisher can instead
	 *	choose to let the RAMS gateway compute the BP Class
	 *	of Service (priority only) from the AMS priority and
	 *	use it as a replacement flow label, by specifying in
	 *	flow label the invalid BP COS value 3.			*/
	
	if ((flowLabel & 0x03) == 3)
	{
		flowLabel = (15 - priority) / 5;
	}

	/*	Compile a list of all RAMS nodes to forward the
	 *	published message to.  The list must include each
	 *	neighbor that is a member of the DGS of at least
	 *	one petition that is satisfied by the published
	 *	message, and no other nodes.  That is, the list
	 *	is the union of the DGSs of all petitions that
	 *	are satisfied by the published message.			*/

	nodesList = lyst_create_using(getIonMemoryMgr());
	CHKERR(nodesList);
	for (elt = lyst_first(gWay->petitionSet); elt; elt = lyst_next(elt))
	{		
		pet = (Petition *) lyst_data(elt);
		if (lyst_length(pet->DestinationNodeSet) == 0)
		{
			continue;
		}

		if (MessageSatisfiesPetition(gWay->amsModule, continuumNbr,
				unitNbr, moduleNbr, subjectNbr, pet))
		{
			AddNodeSets(nodesList, pet->DestinationNodeSet);
		}
	}

	/*	Now forward the published message to every node in
	 *	the list.						*/

	for (elt = lyst_first(nodesList); elt; elt = lyst_next(elt))
	{
		node = (RamsNode *) lyst_data(elt);		    
#if RAMSDEBUG
printf("<forward published message> send to continuum %d\n",
node->continuumNbr);
#endif
		if (SendNewRPDU(gWay, node->continuumNbr, flowLabel, enc, 0, 0,
			sourceRoleNbr, 0, PublishOnReception, subjectNbr) < 0)
		{
#if RAMSDEBUG
PUTS("<forward published message> sending to continuum failed");
#endif
			ErrMsg("Error in sending published message to node.");
			DeleteEnclosure(enc);
			return -1;
		}
	}

	lyst_destroy(nodesList);
	DeleteEnclosure(enc);
	return 0;
}

static int	ForwardTargetedMessage(RamsGateway *gWay,
			unsigned char flowLabel, char* content,
			int contentLength)
{
	int		destinationContinuumNbr;
	LystElt		elt;
	RamsNode	*ramsNode;

	destinationContinuumNbr = EnvelopeHeader(content, Env_ContinuumNbr);
#if RAMSDEBUG
printf("<forward targeted message> forward targeted message to continuum %d\n",
destinationContinuumNbr);
printf("<forward targeted message> content is '%s'\n",
content + ENVELOPELENGTH + AMSMSGHEADER);
#endif
	if (destinationContinuumNbr < 0)	/*	Invalid.	*/
	{
#if RAMSDEBUG
PUTS("<forward targeted message> destination continuum number is invalid");
#endif
		return 0;
	}

	if (destinationContinuumNbr == 0)	/*	All continua.	*/
	{
		if (EnvelopeHeader(content, Env_ControlCode)
				!= AnnounceOnReception)
		{
#if RAMSDEBUG
PUTS("<forward targeted message> 'sending' to destination continuum 0");
#endif
			return 0;		/*	Invalid.	*/
		}

		for (elt = lyst_first(gWay->declaredNeighbors); elt;
				elt = lyst_next(elt))
		{
			ramsNode = (RamsNode *) lyst_data(elt);
#if RAMSDEBUG
printf("<forward targeted message> send to continuum %d\n",
ramsNode->continuumNbr);
#endif
			if (SendRPDU(gWay, ramsNode->continuumNbr, flowLabel,
					content, contentLength) < 0)
			{
#if RAMSDEBUG
PUTS("<forward targeted message> sending to continuum failed");
#endif
				ErrMsg("Error in sending targeted message \
to node.");
				return -1;
			 }
		}

		return 0;
	}

	/*	Destination is a specific continuum.			*/

	switch (EnvelopeHeader(content, Env_ControlCode))
	{
	case SendOnReception:
	case AnnounceOnReception:
		break;

	default:
#if RAMSDEBUG
PUTS("<forward targeted message> invalid control code for targeted message");
#endif
		return 0;		/*	Invalid.	*/
	}

	ramsNode = GetConduitForContinuum(destinationContinuumNbr, gWay);
	if (ramsNode == NULL)
	{
#if RAMSDEBUG
printf("<forward targeted message> no conduit for continuum %d\n",
ramsNode->continuumNbr);
#endif
		return 0;
	}

	/*	Send message to conduit for destination continuum.	*/

#if RAMSDEBUG
printf("<forward targeted message> send to conduit continuum %d\n",
ramsNode->continuumNbr);
#endif
	if (SendRPDU(gWay, ramsNode->continuumNbr, flowLabel, content,
				contentLength) < 0)
	{
#if RAMSDEBUG
PUTS("<forward targeted message> sending to conduit continuum failed");
#endif
		ErrMsg("Error in sending targeted message to conduit.");
		return -1;
	}

	return 0;
}
