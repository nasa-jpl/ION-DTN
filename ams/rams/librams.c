/*
librams.c: functions enabling the implementation of RAMS gateway based applications

Author: Shin-Ywan (Cindy) Wang
Copyright (c) 2005, California Institute of Technology.
ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
*/


#include <amscommon.h>
#include <amsP.h>
#include <bp.h>
#include <sdr.h>
#include <zco.h>
#include <ams.h>
#include "rams.h"
#include "ramscommon.h"
int		ramsMemory = -1;	/*	Memory manager.		*/

static BpSAP	sap; 
static Sdr	sdr;
static int	ownUdpFd;
static FILE	*petitionLog = NULL;

static RamsGateway *gWay;
static int bTerminating = 0; 
static int ttl;

//static int continuumGrid[3] = {1, 2, 3};
//char *errMessage;

//static int printRAMSContent(RamsGate *gNode);
//static void printInvitationList(RamsGateway *gWay);
static int ReleaseRAMSAllocation(RamsGateway *gWay);
static int IsRetracting(RamsGateway *gWay);
static void terminateQuit();
int SendMessageToContinuum(RamsGateway *gWay, int continuumId,
		unsigned char flowLabel, char* envelope, int envelopeLength);
int SendPetitionToDeclaredRAMS(RamsGateway *gWay, char *env);
static int ReceiveRPDU(RamsNode *fromGWay, RamsGateway *gWay, char *msg);
static int AssertPetition(RamsGateway *gWay, Petition *pet);
static int CancelPetition(RamsGateway *gWay, Petition *pet);
static int ReceiveInitialDeclaration(RamsNode *fromGateway, RamsGateway *gWay);
static int ReceiveRAMSSubscribePetition(RamsNode *fromGway, RamsGateway *gWay,
		int subjectNbr, int domainContinuum, int domainUnit,
		int domainRole);
static int ReceiveRAMSCancelPetition(RamsNode *fromGWay, RamsGateway *gWay,
		int subjectNbr, int domainContinuum, int domainUnit,
		int domainRole);
static int ReceiveRAMSPublishMessage(RamsNode *fromGWay, RamsGateway *gWay,
		char *msg);
static int ReceiveRAMSPrivateMessage(RamsGateway *gWay, char *msg);
static int ReceiveRAMSAnnounceMessage(RamsNode *fromGWay, RamsGateway *gWay,
		char *msg);
static int ReceivePetition(Node *sourceNode,  RamsGateway *gWay,
		int domainContinuum, int domainRole, int domainUnit,
		int subjectNbr);
static int ReceiveCancelPetition(Node *sourceNode,  RamsGateway *gWay,
		int domainContinuum, int domainRole, int domainUnit,
		int subjectNbr);
static int ForwardPublishMessage(RamsGateway *gWay, AmsEvent amsEvent);
static int ForwardPrivateMessage(RamsGateway *gWay, unsigned char flowLabel,
		char* content, int contentLength);
static void errorAmsReceive(void *userData, AmsEvent *event);
static void subscriptionHandle(AmsNode node, void *userData, AmsEvent *eventRef,
		int unitNbr, int nodeNbr, int domainRoleNbr, 
		int domainContinuumNbr,  int domainUnitNbr, int subjectNbr, 
		int priority, unsigned char flowLabel, AmsSequence sequence,
		AmsDiligence diligence);
static void unsubscriptionHandle(AmsNode node,
					void *userData,
					AmsEvent *eventRef,
					int unitNbr,
					int nodeNbr,
					int domainRoleNbr,
					int domainContinuumNbr,
					int domainUnitNbr,
					int subjectNbr);
static void registrationHandle(AmsNode node,
					void *userData,
					AmsEvent *eventRef,
					int unitNbr,
					int nodeNbr,
					int roleNbr);

static void unregisterationHandle(AmsNode node,
					void *userData,
					AmsEvent *eventRef,
					int unitNbr,
					int nodeNbr);

static void invitationHandle(AmsNode node,
					void *userData,
					AmsEvent *eventRef,
					int unitNbr,
					int nodeNbr,
					int domainRoleNbr,
					int domainContinuumNbr,
					int domainUnitNbr,
					int subjectNbr,
					int priority,
					unsigned char flowLabel,
					AmsSequence sequence,
					AmsDiligence diligence);
static void disinvitaionHandle(AmsNode node,
					void *userData,
					AmsEvent *eventRef,
					int unitNbr,
					int nodeNbr,
					int domainRoleNbr,
					int domainContinuumNbr,
					int domainUnitNbr,
					int subjectNbr);

static void userEventHandle(AmsNode node,
					void *userData,
					AmsEvent *eventRef,
					int code,
					int dataLength,
					char *data);

static void amsReceiveProcess(AmsNode node,
					void *userData,
					AmsEvent *eventRef,
					int continuumNbr,
					int unitNbr,
					int nodeNbr,
					int subjectNbr,
					int contentLength,
					char *content,
					int context,
					AmsMsgType msgType,
					int priority,
					unsigned char flowLabel);

static int	rehandlePetition(RamsNetProtocol protocol, char *gwEid,
			int cc, int sub, int domainContinuum, int domainUnit,
			int domainRole)
{
	RamsNode	*fromGWay;

	fromGWay = Look_Up_NeighborRemoteRAMS(gWay, gwEid);
	if (fromGWay == NULL)
	{
		ErrMsg("Can't find source gateway");
		return 0;
	}

	switch (cc)
	{
	case PetitionAssertion:
		if (sub == -fromGWay->continuumNbr)
		{
			if (!ReceiveInitialDeclaration(fromGWay, gWay))
				return 0;
		}
		else
		{
			if (Look_Up_DeclaredRemoteRAMS(gWay,
					fromGWay->continuumNbr) == NULL)
			{
				ErrMsg("Can't find declared source gateway");
//PUTS("The rams gateway is not declared yet.");
				return 0;
			}
		}

		if (!ReceiveRAMSSubscribePetition(fromGWay, gWay, sub,
			domainContinuum, domainUnit, domainRole))
		{
			return 0;
		}

		break;

	case PetitionCancellation:
		if (!ReceiveRAMSCancelPetition(fromGWay, gWay, sub,
			domainContinuum, domainUnit, domainRole))
		{
			return 0;
		}

		break;

	default:
		ErrMsg("Invalid cc in petition log line.");
		return 0;
	}

	return 1;
}

static int	playbackPetitionLog()
{
	RamsNetProtocol	protocol;
	char		gwEid[256];
	unsigned int	cc;
	int		sub; 
	unsigned int	domainContinuum;
	unsigned int	domainUnit;
	unsigned int	domainRole;

	fseek(petitionLog, 0, SEEK_SET);
	while (1)
	{
		switch (fscanf(petitionLog, "%u %255s %u %d %u %u %u",
				(unsigned int *) &protocol, gwEid, &cc, &sub,
				&domainContinuum, &domainUnit, &domainRole))
		{
		case 7:
			if (!rehandlePetition(protocol, gwEid, cc, sub,
				domainContinuum, domainUnit, domainRole))
			{
				ErrMsg("Can't play back petition log.");
				return 0;
			}

			continue;

		case EOF:
			if (feof(petitionLog))
			{
				return 1;
			}

			ErrMsg("fscanf failed.");
			return 0;

		default:
			ErrMsg("Malformed petition log line.");
			return 0;
		}
	}
}

static void	handleBundle(BpDelivery *dlv, int *contentLength, char *content,
			int *running, int *runningErr)
{
	RamsNode	*fromGWay;
	ZcoReader	reader;
	char   		*contentLoc;
	int		byteCopied; 
	int		counter;

	fromGWay = Look_Up_NeighborRemoteRAMS(gWay, dlv->bundleSourceEid);
	if (fromGWay == NULL)
	{
		ErrMsg("Can't find source gateway");
		*running = 0;
		*runningErr = 1;
		return;
	}

	zco_start_receiving(sdr, dlv->adu, &reader);
	contentLoc = content;
	if ((byteCopied = zco_receive_source(sdr, &reader, *contentLength,
			contentLoc)) < 0)
	{
		sdr_cancel_xn(sdr);
		ErrMsg("Can't receive payload.");
		*running = 0;
		*runningErr = 1;
		return;
	}

/*
printf("Receive Envelope: byteCopied=%d contentLength=%d\n",
byteCopied, *contentLength);	
for (i = 0; i < *contentLength; i++)
{
	printf("%2x ", contentLoc[i]);
}
printf("\n");
printf("cc=%d, con=%d, unit=%d, srcId=%d, destId=%d, sub=%d len=%d from=%d\n",
EnvelopeHeader(content, Env_ControlCode),
EnvelopeHeader(content, Env_ContinuumNbr),
EnvelopeHeader(content, Env_UnitField),
EnvelopeHeader(content, Env_SourceIDField),
EnvelopeHeader(content, Env_DestIDField),
EnvelopeHeader(content, Env_SubjectNbr),
*contentLength, fromGWay->continuumNbr);				
*/

	*contentLength = EnvelopeHeader(content, Env_EnclosureLength);
	if (*contentLength > 0)
	{
		contentLoc = content + ENVELOPELENGTH;
//printf("before zco_receive_source for receiving payload\n");
		if ((byteCopied = zco_receive_source(sdr, &reader,
				*contentLength, contentLoc)) < 0)
		{
			sdr_cancel_xn(sdr);
			ErrMsg("Can't receive payload.");
			*running = 0;
			*runningErr = 1;
			return;
		}

//printf("after zco_receive_source\n");					
//		*(contentLoc + *contentLength) = '\0';
//mainly for "amsbenchr", "amsbenchs" test
		memcpy((char *) &counter, contentLoc + AMSMSGHEADER,
				sizeof(int));
		counter = ntohl(counter);
/*
printf("Enclosure: contentLength=%d messageCounter=%d\n",
*contentLength, counter); 
*/
	}

	*contentLength = ENVELOPELENGTH;	//	reset for next one

	// receive RAMS PDU from remote RAMS gateway

	if (ReceiveRPDU(fromGWay, gWay, content) == 0)
	{
		*running = 0; 
		*runningErr = 1;
		return;
	}

//printf("before zco_stop_receiving\n");
	zco_stop_receiving(sdr, &reader);
//printf("after zco_stop_receiving\n");
}

static void	handleDatagram(struct sockaddr_in *inetName, int *contentLength,
			char *content, int *running, int *runningErr)
{
	unsigned int	ipAddress;
	unsigned short	portNbr;
	char		gwEidBuffer[32];
	RamsNode	*fromGWay;
	char   		*contentLoc;
	int		counter;

	memcpy((char *) &ipAddress, (char *) &(inetName->sin_addr.s_addr), 4);
	ipAddress = ntohl(ipAddress);
	portNbr = inetName->sin_port;
	portNbr = ntohs(portNbr);
	isprintf(gwEidBuffer, sizeof gwEidBuffer, "%u:%hu", ipAddress, portNbr);
	fromGWay = Look_Up_NeighborRemoteRAMS(gWay, gwEidBuffer);
	if (fromGWay == NULL)
	{
		ErrMsg("Can't find source gateway");
		*running = 0;
		*runningErr = 1;
		return;
	}

/*
printf("cc=%d, con=%d, unit=%d, srcId=%d, destId=%d, sub=%d len=%d from=%d\n",
EnvelopeHeader(content, Env_ControlCode),
EnvelopeHeader(content, Env_ContinuumNbr),
EnvelopeHeader(content, Env_UnitField),
EnvelopeHeader(content, Env_SourceIDField),
EnvelopeHeader(content, Env_DestIDField),
EnvelopeHeader(content, Env_SubjectNbr),
*contentLength, fromGWay->continuumNbr);				
*/
	*contentLength = EnvelopeHeader(content, Env_EnclosureLength);
	if (*contentLength > 0)
	{
		contentLoc = content + ENVELOPELENGTH;
//		*(contentLoc + *contentLength) = '\0';
//mainly for "amsbenchr", "amsbenchs" test
		memcpy((char *) &counter, contentLoc + AMSMSGHEADER,
				sizeof(int));
		counter = ntohl(counter);
/*
printf("Enclosure: contentLength=%d messageCounter=%d\n",
*contentLength, counter); 
*/
	}

	*contentLength = ENVELOPELENGTH;	//	reset for next one

	// receive RAMS PDU from remote RAMS gateway

	if (ReceiveRPDU(fromGWay, gWay, content) == 0)
	{
		*running = 0; 
		*runningErr = 1;
		return;
	}
}

// new rams registration
int rams_register(char *mibSource, char *tsorder, char *mName, char *memory,
		unsigned mSize, char *applicationName, char *authorityName,
		char *unitName, char *roleName, RamsGate *gNode, int lifetime)
{
	LystElt			elt;
	BpDelivery		dlv;
	int			contentLength;
	char    		*content;
	unsigned char		envelope[ENVELOPELENGTH];
	RamsNode		*ramsNode;	//	formerly ionEndPoint
	char			gwEid[256];
	unsigned short		portNbr;
	unsigned int		ipAddress;
	struct sockaddr		socketName;
	struct sockaddr_in	*inetName = (struct sockaddr_in *) &socketName;
	socklen_t		nameLength;
	int			running;
	Lyst			msgspaces; 	//	formerly gridLyst
	int			cId; 
	Continuum		*continuum;
	int			runningErr;
	AmsNode			amsNode; 
	Petition		*pet;
	AmsEventMgt		rules;
	//char			buffer[256];
	//char			*cmdString;
	//int			i;

	PUTS("RAMS version 0.19");
	signal(SIGINT, terminateQuit);
	bTerminating = 0; 
	ttl = lifetime;
	if (ams_register(mibSource, tsorder, mName, memory, mSize,
			applicationName, authorityName, unitName, roleName,
			&amsNode) < 0)
	{
		putSysErrmsg("rams can't register", NULL);
		return -1;
	}

	//errMessage = (char *) MTAKE(512);
    
	gWay = (RamsGateway *) MTAKE(sizeof(RamsGateway));
	*gNode = gWay;
	gWay->amsNode = amsNode;     // add in gateway
	gWay->petitionSet = lyst_create();
	gWay->registerSet = lyst_create();
	gWay->invitationSet = lyst_create();
	gWay->primeThread = pthread_self();
	gWay->petitionReceiveThread = pthread_self();

	// mib
	gWay->ramsMib = (RamsMib *) MTAKE(sizeof(RamsMib));
	gWay->ramsMib->amsMib = mib;
	gWay->ramsMib->ramsNeighbors = lyst_create();
	gWay->ramsMib->declaredNeighbors = lyst_create();
	gWay->ramsMib->neighborsCount = 0;
	gWay->ramsMib->totalDeclared = 0; 

	// get all RAMS
	msgspaces = ams_list_msgspaces(gWay->amsNode);
  
	if (ams_rams_net_is_tree())
		gWay->ramsMib->netType = TREETYPE;
	else 
		gWay->ramsMib->netType = MESHTYPE;

	//	gWay->ramsMib->netType = TREETYPE;
	// transfer neighboring RAMS into ramsNeighbors list, not include itself
//printf("continuum lyst: ");
	for (elt = lyst_first(msgspaces); elt != NULL; elt = lyst_next(elt))
	{
		cId = (long)lyst_data(elt);
//printf("\nno %d ", cId);		
		if (cId == gWay->ramsMib->amsMib->localContinuumNbr)
		{
			continue; 	
		}

		if (!ams_continuum_is_neighbor(cId))
		{
			continue;
		} 	

		continuum = mib->continua[cId];
		ramsNode = (RamsNode *) MTAKE(sizeof(RamsNode));
		memset(ramsNode, 0, sizeof(RamsNode));
		ramsNode->continuumNbr = cId;
		ramsNode->protocol = continuum->gwProtocol;
		ramsNode->gwEid = continuum->gwEid;
		lyst_insert_last(gWay->ramsMib->ramsNeighbors, ramsNode);
		gWay->ramsMib->neighborsCount++;
//printf(" neighbor");
	}

//printf("\n");
	lyst_destroy(msgspaces);

	rules.registrationHandler = registrationHandle;
	rules.registrationHandlerUserData = gWay;
	
	rules.unregistrationHandler = unregisterationHandle;
	rules.unregistrationHandlerUserData = gWay;
	
	rules.invitationHandler = invitationHandle;
	rules.invitationHandlerUserData = gWay;
	
	rules.disinvitationHandler = disinvitaionHandle;
	rules.disinvitationHandlerUserData = gWay;
	
	rules.subscriptionHandler = subscriptionHandle;
	rules.subscriptionHandlerUserData = gWay;
	
	rules.unsubscriptionHandler = unsubscriptionHandle;
	rules.unsubscriptionHandlerUserData = gWay;
    
	rules.userEventHandler = userEventHandle;
	rules.userEventHandlerUserData = gWay;	
    
	rules.errHandler = errorAmsReceive;
	rules.errHandlerUserData = gWay;

	rules.msgHandler = amsReceiveProcess;     // ams handling process
	rules.msgHandlerUserData = gWay;          // rams gateway
	
	ams_set_event_mgr(gWay->amsNode, &rules);	

	//subscribe to message on the pseudo-subject for the local continuum
	if (ams_subscribe(gWay->amsNode, 0, 0, 0, (0-mib->localContinuumNbr),
			10, 0, AmsArrivalOrder, AmsAssured) < 0)
	{
		putErrmsg("error on subscribe to self",
				itoa(0 - mib->localContinuumNbr));
		return 0;
	}

//printf("subscribe on %d\n", (0 - mib->localContinuumNbr));

	//insert self into RAMS network

	switch (mib->localContinuumGwProtocol)
	{
	case RamsBp:
//printf("ownEid for bp_open %s\n", mib->localContinuumGwEid);
		if (bp_attach() < 0)
		{
			ErrMsg("Can't attach to BP.");
			return 0;
		}
//printf("ionAttach succeeds\n");

		if (bp_open(mib->localContinuumGwEid, &sap) < 0)
		{
			ErrMsg("Can't open own BP endpoint.");
			return 0;
		}
//printf("bp_open succeeds\n");

		sdr = bp_get_sdr();
		break;

	case RamsUdp:
		istrcpy(gwEid, mib->localContinuumGwEid, sizeof gwEid);
		parseSocketSpec(gwEid, &portNbr, &ipAddress);
		portNbr = htons(portNbr);
		ipAddress = htonl(ipAddress);
		memset((char *) &socketName, 0, sizeof socketName);
		inetName->sin_family = AF_INET;
		inetName->sin_port = portNbr;
		memcpy((char *) &(inetName->sin_addr.s_addr),
				(char *) &ipAddress, 4);
		ownUdpFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (ownUdpFd < 0)
		{
			ErrMsg("Can't create UDP socket.");
			return 0;
		}

		nameLength = sizeof(struct sockaddr);
		if (reUseAddress(ownUdpFd)
		|| bind(ownUdpFd, &socketName, nameLength) < 0
		|| getsockname(ownUdpFd, &socketName, &nameLength) < 0)
		{
			ErrMsg("Can't open own UDP endpoint.");
			return 0;
		}

		break;

	default:
		ErrMsg("No valid RAMS network protocol selected.");
		return 0;
	}

	// RAMS declare itself to all neighbor
//printf("rams declares itself to neighbors ....\n");
	constructEnvelope(envelope, 0, 0, 0, 0, (0 - mib->localContinuumNbr),
			0, NULL, 0, NULL, PetitionAssertion);
	pet = ConstructPetitionFromEnvelope((char *) envelope);
   	
	if (!SendMessageToContinuum(gWay, 0, 1, (char *) envelope,
			ENVELOPELENGTH))
	{
		ErrMsg("error send initial declaration");
		return 0;
	}

	//	When UDP is used as the RAMS network protocol (for
	//	testing purposes only), the re-issuance of the declaration
	//	petition upon reception of declaration petitions from
	//	neighbors is required because delivery of the original
	//	declaration petition is not guaranteed, because UDP
	//	is not a reliable protocol.  Otherwise, we must insert
	//	all neighbors into the SourceRamsSet to prevent that
	//	re-issuance, because it results in a registration loop.

	if (mib->localContinuumGwProtocol != RamsUdp)
	{
		for (elt = lyst_first(gWay->ramsMib->ramsNeighbors); elt;
				elt = lyst_next(elt))
		{
			ramsNode = (RamsNode *) lyst_data(elt);
			if (ramsNode->continuumNbr !=
				gWay->ramsMib->amsMib->localContinuumNbr)
			{
		    		lyst_insert_last(pet->SourceRamsSet, ramsNode);
			}
		}
	}

	// add this petition and itself to DNS 
	lyst_insert_last(gWay->petitionSet, pet);
	lyst_insert_last(pet->DistributionNodeSet,
			GetSourceNode(ams_get_unit_nbr(amsNode),
			ams_get_node_nbr(amsNode), gWay));

	// rams receive thread

	content = MTAKE(65534);
	running = 1;
	contentLength = ENVELOPELENGTH;
	runningErr = 0; 
	petitionLog = fopen("petition.log", "a+");
	if (petitionLog == NULL || playbackPetitionLog() < 0)
	{
		ErrMsg("RAMS can't playback petition log.");
		running = 0;
		runningErr = 1;
	}

	while (running)
	{
		switch (mib->localContinuumGwProtocol)
		{
		case RamsBp:
//printf("before bp_receive\n");
    			if (bp_receive(sap, &dlv, BP_BLOCKING) < 0)
			{
				ErrMsg("RAMS bundle reception failed.");
				running = 0;
				runningErr = 1;
				continue;
			}

			if (dlv.result == BpPayloadPresent)
			{
				sdr_begin_xn(sdr);
				handleBundle(&dlv, &contentLength, content,
						&running, &runningErr);
				if (sdr_end_xn(sdr) < 0)
				{
					ErrMsg("Can't receive payload.");
					running = 0;
					runningErr = 1;
				}
			}

			bp_release_delivery(&dlv, 1);
			if ((bTerminating == 1) && IsRetracting(gWay))
			{
				running = 0;
			}

			continue;

		case RamsUdp:
			nameLength = sizeof(struct sockaddr_in);
			switch (recvfrom(ownUdpFd, content, 65534, 0,
					&socketName, &nameLength))
			{
			case -1:
				if (errno == EINTR)
				{
					continue;
				}

				ErrMsg("RAMS datagram reception failed.");
				running = 0;
				runningErr = 1;
				continue;

			case 0:
				running = 0;
				continue;

			default:
				handleDatagram(inetName, &contentLength,
						content, &running, &runningErr);
			}

			if ((bTerminating == 1) && IsRetracting(gWay))
			{
				running = 0;
			}

			break;		/*	Out of switch.		*/

		default:
			ErrMsg("No RAMS network protocol.");
		}
	}

	MRELEASE(content);
	/*
	if (runningErr)
		rams_unregister(gNode);
	*/
	ReleaseRAMSAllocation(gWay);
	switch (mib->localContinuumGwProtocol)
	{
	case RamsBp:
		bp_close(sap);
		break;

	case RamsUdp:
		close(ownUdpFd);
		break;

	default:
		break;
	}

	if (petitionLog)
	{
		fclose(petitionLog);
	}

	return 1; 
}
#if 0
int printRAMSContent(RamsGate *gNode)
{
	char	buf[256];
	LystElt eltP, eltN;
	Petition *pet;
	RamsNode *ep; 
	Node *nd; 

	isprintf(buf, sizeof buf, "===== In RAMS %d ======",
			(*gNode)->ramsMib->amsMib->localContinuumNbr);
	PUTS(buf);
	for (eltP = lyst_first((*gNode)->petitionSet); eltP != NULL;
			eltP = lyst_next(eltP))
	{
		pet = (Petition *)lyst_data(eltP);
		isprintf(buf, sizeof buf,
			"Petition con = %d role = %d unit = %d node = %d \
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
		PUTS("    SRS = ");
		for (eltN = lyst_first(pet->SourceRamsSet); eltN != NULL;
				eltN = lyst_next(eltN))
		{
			ep = (RamsNode *)lyst_data(eltN);
			isprintf(buf, sizeof buf, "%d ", ep->continuumNbr);
			PUTS(buf);
		}

		PUTS("    DRS = ");
		for (eltN = lyst_first(pet->DestinationRamsSet); eltN != NULL;
				eltN = lyst_next(eltN))
		{
			ep = (RamsNode *)lyst_data(eltN);
			isprintf(buf, sizeof buf, "%d ", ep->continuumNbr);
			PUTS(buf);
		}

		PUTS("    DNS = ");
		for (eltN = lyst_first(pet->DistributionNodeSet); eltN != NULL;
				eltN = lyst_next(eltN))
		{
			nd = (Node *)lyst_data(eltN);
			isprintf(buf, sizeof buf, "%d ", nd->nbr);
			PUTS(buf);
		}
	}

	PUTS("====================");
	return 1;
}

void printInvitationList(RamsGateway *gWay)
{
	char	buf[256];
	LystElt elt, nodeElt;
	Invitation *inv;
	Node *node;

	PUTS("========= invitation list ==============");
	for (elt = lyst_first(gWay->invitationSet); elt != NULL;
			elt = lyst_next(elt))
	{
		inv = (Invitation *)lyst_data(elt);
		isprintf(buf, sizeof buf, "Invitation spec: unit=%d role=%d \
sub=%d\n", inv->inviteSpecification->domainUnitNbr,
				inv->inviteSpecification->domainRoleNbr,
				inv->inviteSpecification->subjectNbr);
		PUTS(buf);
		PUTS("    node set = ");
		for (nodeElt = lyst_first(inv->nodeSet); nodeElt != NULL;
				nodeElt = lyst_next(nodeElt))
		{
			node = (Node *)lyst_data(nodeElt);
			isprintf(buf, sizeof buf, "(unit=%d nodeId=%d) ",
					node->unitNbr, node->nbr);
			PUTS(buf);
		}
	}

	PUTS("=============================");
}
#endif
int IsRetracting(RamsGateway *gWay)
{
	Petition *pet;
	LystElt elt;
	int sub;
	int bTestDGS;
	int allEmpty;

	bTestDGS = 0; 
	sub = 0 - gWay->ramsMib->amsMib->localContinuumNbr;
//PUTS("in IsRetracting.");
	for (elt = lyst_first(gWay->petitionSet); elt != NULL;
			elt = lyst_next(elt))
	{
		pet = (Petition *)lyst_data(elt);
		if (EnvelopeHeader(pet->specification->envelope,
					Env_SubjectNbr) == sub)
		{
			if (lyst_first(pet->SourceRamsSet) == NULL)
			{
				bTestDGS = 1;
				break;
			}
//PUTS("SRS is has member.");
		}
	}
	if (bTestDGS)
	{
		allEmpty = 1; 
		for (elt = lyst_first(gWay->petitionSet); elt != NULL;
				elt =lyst_next(elt))
		{
			pet = (Petition *)lyst_data(elt);
			if (lyst_first(pet->DestinationRamsSet))
			{
				allEmpty = 0;
				break;
			}
		}

		if (allEmpty == 1)
			return 1; 
		else 
			return 0;
	}

	return 0;
}
	


void terminateQuit()
{
	LystElt elt, nextElt, conElt;
	RamsNode *pt;
	Petition *pet;
	int sub;


	bTerminating = 1; 
	sub = 0 - gWay->ramsMib->amsMib->localContinuumNbr;
//printf("<terminate RAMS> retracting %d\n", gWay->ramsMib->amsMib->localContinuumNbr);
	for (elt = lyst_first(gWay->petitionSet); elt != NULL; elt = lyst_next(elt))
	{
		pet = (Petition *) lyst_data(elt);
		if (EnvelopeHeader(pet->specification->envelope, Env_SubjectNbr) == sub)  // sent at initialization
			break;
	}

	if (elt == NULL)
	{
//printf("<terminate RAMS> can not retract continuum %d\n", gWay->ramsMib->amsMib->localContinuumNbr);
		return;
	}
   
	for (elt = lyst_first(gWay->ramsMib->declaredNeighbors); elt != NULL;
    			elt = nextElt)
	{
		pt = (RamsNode *) lyst_data(elt);
		nextElt = lyst_next(elt);
		if ((conElt = IsContinuumExist(pt, &(pet->SourceRamsSet)))
				== NULL)
		{
			lyst_delete(elt);
			continue;
		}
//printf("<terminate RAMS> send cancellation on sub %d to %d\n", sub, pt->continuumNbr); 
		if (!ConstructEnvelopeToRAMS(gWay, 1, NULL, 0, 0, 0, 0,
				PetitionCancellation, sub, pt->continuumNbr))
		{
			ErrMsg("error petition cancellation in CancelPetition");
			return;
		}

		lyst_delete(conElt);
		lyst_delete(elt);
	}

	if (lyst_length(gWay->ramsMib->declaredNeighbors) == 0)
	{	
		ReleaseRAMSAllocation(gWay);
		exit(0);
	}

//PUTS("<terminate RAMS> leaving terminate");
}

int rams_unregister(RamsGate *gNode)
{
	LystElt elt;
	RamsNode *pt;
	Petition *pet; 
	int sub;
	int allEmpty; 
/*
printf("<rams unregister> unregister %d\n", (*gNode)->ramsMib->amsMib->localContinuumNbr);
printf("<rams unregister> send cancellation to neighbor\n");
*/
	for (elt = lyst_first((*gNode)->ramsMib->declaredNeighbors);
			elt != NULL; elt = lyst_next(elt))
	{
		pt = (RamsNode *)lyst_data(elt);
		sub = (0-(*gNode)->ramsMib->amsMib->localContinuumNbr);
//printf("<rams unregister> send cancellation on sub %d to %d\n", sub, pt->continuumNbr); 
		if (!ConstructEnvelopeToRAMS((*gNode), 1, NULL, 0, 0, 0, 0,
				PetitionCancellation, sub, pt->continuumNbr))
		{
			ErrMsg("error petition cancellation in CancelPetition");
			return 0;
		}
	}

	// wait until all DGS become empty, and 

	while(1)
	{
		allEmpty = 1; 
		for (elt = lyst_first((*gNode)->petitionSet); elt != NULL;
				elt =lyst_next(elt))
		{
			pet = (Petition *)lyst_data(elt);
			if (lyst_first(pet->DestinationRamsSet))
			{
				allEmpty = 0;
				break;
			}
		}

		if (allEmpty == 1)
			break;
		else
		{
//printRAMSContent(gNode);
			continue;
		}
	}
	
//PUTS("allEmpty = 1");
//printRAMSContent(gNode);
	return 1;
}

int ReleaseRAMSAllocation(RamsGateway *gWay)
{
	LystElt elt;
	RamsNode *pt;
	Petition *pet;

//PUTS("<release rams> start release memory");
	ams_unregister(gWay->amsNode);

	for (elt = lyst_first(gWay->ramsMib->ramsNeighbors); elt != NULL;
			elt = lyst_next(elt))
	{
		pt = (RamsNode *)lyst_data(elt);
		MRELEASE(pt);
	}

//PUTS("<release rams> release neighbor");
	lyst_destroy(gWay->ramsMib->ramsNeighbors);
//PUTS("<release rams> release neighbor list");
	lyst_destroy(gWay->ramsMib->declaredNeighbors);
//PUTS("<release rams> release declared neighbor list");
	
	MRELEASE(gWay->ramsMib);
//PUTS("<release rams> release rams Mib");	
	for (elt = lyst_first(gWay->petitionSet); elt != NULL;
			elt = lyst_next(elt))
	{
		pet = (Petition *)lyst_data(elt);
        	DeletePetition(pet);
	}
//PUTS("<release rams>release petitions");
	lyst_destroy(gWay->petitionSet);
//PUTS("<release rams> release petition list");
	//ams_unregister(gWay->amsNode);
	MRELEASE(gWay);
//PUTS("<rams rams> release rams node");
//PUTS("<rams rams> finish unregister");
	return 1; 
}

static void errorAmsReceive(void *userData, AmsEvent *event)
{
	ErrMsg("Can't reveive AmsMessage");
}

static void subscriptionHandle(AmsNode node, void *userData, AmsEvent *eventRef,
		int unitNbr, int nodeNbr, int domainRoleNbr, 
		int domainContinuumNbr, int domainUnitNbr,
		int subjectNbr, int priority, unsigned char flowLabel,
		AmsSequence sequence, AmsDiligence diligence)
{
	RamsGateway *gWay;
	Node *sourceNode; 
	
//printf("<subscriptionHandle> receive subscription notice from %d subjectNbr=%d\n", nodeNbr, subjectNbr);
	gWay = (RamsGateway *)userData;
	sourceNode = GetSourceNode(unitNbr, nodeNbr, gWay);
	if (domainContinuumNbr != gWay->ramsMib->amsMib->localContinuumNbr &&
		!IsSameNode(sourceNode, gWay))
	{				
		ReceivePetition(sourceNode, gWay, domainContinuumNbr,
				domainRoleNbr, domainUnitNbr, subjectNbr);
	}
}

static void unsubscriptionHandle(AmsNode node,
					void *userData,
					AmsEvent *eventRef,
					int unitNbr,
					int nodeNbr,
					int domainRoleNbr,
					int domainContinuumNbr,
					int domainUnitNbr,
					int subjectNbr)
{
	RamsGateway *gWay;
	Node *sourceNode; 

	gWay = (RamsGateway *)userData;
//printf("<unscriptionHandle> receive unsubscription from %d subjectNbr=%d\n", nodeNbr, subjectNbr);
	sourceNode = GetSourceNode(unitNbr, nodeNbr, gWay);
	if (domainContinuumNbr != gWay->ramsMib->amsMib->localContinuumNbr &&
		!IsSameNode(sourceNode, gWay))
	{		
		ReceiveCancelPetition(sourceNode, gWay, domainContinuumNbr, domainRoleNbr, domainUnitNbr, subjectNbr); 
	}	
}

static void registrationHandle(AmsNode node,
					void *userData,
					AmsEvent *eventRef,
					int unitNbr,
					int nodeNbr,
					int roleNbr)
{
	Node *sourceNode;

//PUTS("in registrationHandle");

	sourceNode = GetSourceNode(unitNbr, nodeNbr, gWay);
	if (IsNodeExist(sourceNode, &(gWay->registerSet)) == NULL)
	{
		lyst_insert_last(gWay->registerSet, sourceNode);
//printf("<registerHandle> add node (unit=%d Id=%d)\n", unitNbr, nodeNbr);
	}
}

// if the same node as the RAMS gateway, then remove all petition
//  -- unsubscribe all petiion sent as recorded in SGS
//  -- remove all DNS
//  -- remove all DGS
//  -- remove all petition
// if  not the same node as the RAMS gateway, remove member same as unregister one from some petition's DNS
//  -- unsubscribe petition from the member same as the unregistered node

static void unregisterationHandle(AmsNode node,
					void *userData,
					AmsEvent *eventRef,
					int unitNbr,
					int nodeNbr)
{
	Node *sourceNode;
	LystElt elt; 

//PUTS("in unregistrationHandle");

	sourceNode = GetSourceNode(ams_get_unit_nbr(node),
			ams_get_node_nbr(node), gWay);
	if ((elt = IsNodeExist(sourceNode, &(gWay->registerSet))) != NULL)
	{
		lyst_delete(elt);
//printf("<unregisterHandle> remove node (unit=%d Id=%d)\n", ams_get_unit_nbr(node), ams_get_node_nbr(node));
	}
}

static void invitationHandle(AmsNode node,
					void *userData,
					AmsEvent *eventRef,
					int unitNbr,
					int nodeNbr,
					int domainRoleNbr,
					int domainContinuumNbr,
					int domainUnitNbr,
					int subjectNbr,
					int priority,
					unsigned char flowLabel,
					AmsSequence sequence,
					AmsDiligence diligence)
{
	RamsGateway *gWay = (RamsGateway *)userData;
	Invitation *invSp;
	LystElt inv;
	Node *sourceNode; 

//printf("in invitationHandle subjectNbr=%d from unit=%d node=%d to domain role=%d domain Unit=%d\n", subjectNbr, unitNbr, nodeNbr, domainRoleNbr, domainUnitNbr);

	if (subjectNbr <= 0) 
		return;
	sourceNode = GetSourceNode(unitNbr, nodeNbr, gWay);
	if (IsSameNode(sourceNode, gWay))
		return;

	if ((inv = IsInvitationExist(domainUnitNbr, domainRoleNbr, 0,
			subjectNbr, &(gWay->invitationSet))) == NULL)
	{
		invSp = (Invitation *)MTAKE(sizeof(Invitation));
		lyst_insert_last(gWay->invitationSet, invSp);
		invSp->inviteSpecification = (InvitationContent *)
				MTAKE(sizeof(InvitationContent));
		invSp->inviteSpecification->domainUnitNbr = domainUnitNbr;
		invSp->inviteSpecification->domainRoleNbr = domainRoleNbr; 
		invSp->inviteSpecification->domainContNbr = domainContinuumNbr; 
		invSp->inviteSpecification->subjectNbr = subjectNbr;
		invSp->nodeSet = lyst_create();
		lyst_insert_last(invSp->nodeSet, sourceNode);
	}
	else
	{
		invSp = (Invitation *)lyst_data(inv);
		if (IsNodeExist(sourceNode, &(invSp->nodeSet)) == NULL)
		{
			lyst_insert_last(invSp->nodeSet, sourceNode);
		}
	}

//printInvitationList(gWay);
}

static void disinvitaionHandle(AmsNode node,
					void *userData,
					AmsEvent *eventRef,
					int unitNbr,
					int nodeNbr,
					int domainRoleNbr,               
					int domainContinuumNbr,
					int domainUnitNbr,
					int subjectNbr)
{
	RamsGateway *gWay = (RamsGateway *)userData;
	LystElt inv;
	LystElt nodeElt;
	Invitation *invSp;
	Node *sourceNode;

//PUTS("in disinvitationHandle");
//printf("in disinvitationHandle subjectNbr=%d from unit=%d node=%d to domain role=%d domain cont = %d domain Unit=%d\n", subjectNbr, unitNbr, nodeNbr, domainRoleNbr, domainContinuumNbr, domainUnitNbr);

	sourceNode = GetSourceNode(unitNbr, nodeNbr, gWay);
	if (IsSameNode(sourceNode, gWay))
		return;

	// temporary put cont = 0 
	if ((inv = IsInvitationExist(domainUnitNbr, domainRoleNbr,
			domainContinuumNbr, subjectNbr, &(gWay->invitationSet)))
			!= NULL)
	{
		invSp = (Invitation *)lyst_data(inv);
		if ((nodeElt = IsNodeExist(sourceNode, &(invSp->nodeSet)))
				!= NULL)
		{
			lyst_delete(nodeElt);
			if (lyst_length(invSp->nodeSet) == 0)
			{
				lyst_delete(inv);
			}
		}
	}

//printInvitationList(gWay);
}

static void userEventHandle(AmsNode node,
					void *userData,
					AmsEvent *eventRef,
					int code,
					int dataLength,
					char *data)
{
//PUTS("in userEventnHandle");
}


// thread to receive ams from node of same message space
//source node which send message, continuumNbr, unitNbr, nodeNbr, from source
static void amsReceiveProcess(AmsNode node, void *userData, AmsEvent *eventRef,
	int continuumNbr, int unitNbr, int nodeNbr, int subjectNbr, 
	int contentLength, char *content, int context, AmsMsgType msgType,
	int priority, unsigned char flowLabel)
{
	RamsGateway *gWay = (RamsGateway *)userData;
	
//printf("in amsReceiveProcess, subject = %d\n", subjectNbr);
	
	if (subjectNbr < 0)     // private message from local node,
				// -subjectNbr is the remote continuum
	{
		ForwardPrivateMessage(gWay, flowLabel, content, contentLength);
		// already in envelope format
	} 
	else if (subjectNbr > 0)
	{
		ForwardPublishMessage(gWay, *eventRef);
		// in amsMsg (header+content) format
	}
}

// function to receive RAMS PDU from remote continuum 
static int ReceiveRPDU(RamsNode *fromGWay, RamsGateway *gWay, char *msg)
// formerly ReceiveRAMSPetition -- but is not always a petition
{
	int cc;
	int sub; 
	int domainContinuum;
	int domainUnit;
	int domainRole;
	RamsNode *declaredION;

	cc = EnvelopeHeader(msg, Env_ControlCode);
	switch(cc)
	{
		case PetitionAssertion:
			sub = EnvelopeHeader(msg, Env_SubjectNbr);
			if (sub == -fromGWay->continuumNbr)
			{
				if (!ReceiveInitialDeclaration(fromGWay, gWay))
					return 0;
			}
			else
			{
				declaredION = Look_Up_DeclaredRemoteRAMS(gWay, fromGWay->continuumNbr);
				if (declaredION == NULL)
				{
					ErrMsg("Can't find declared source \
gateway");
//PUTS("The rams gateway is not declared yet.");
					return 0;
				}
			}

			domainContinuum = EnvelopeHeader(msg, Env_ContinuumNbr);
			domainUnit = EnvelopeHeader(msg, Env_PublishUnitNbr);
			domainRole = EnvelopeHeader(msg, Env_PublishRoleNbr);
			if (!ReceiveRAMSSubscribePetition(fromGWay, gWay, sub,
				domainContinuum, domainUnit, domainRole))
			{
				return 0;
			}

			if (fprintf(petitionLog, "%u %s %u %d %u %u %u\n",
				(unsigned int) fromGWay->protocol,
				fromGWay->gwEid, cc, sub, domainContinuum,
				domainUnit, domainRole) < 1
			|| fflush(petitionLog) == EOF)
			{
				ErrMsg("Can't log petition assertion.");
				return 0;
			}

			break;

		case PetitionCancellation:
			sub = EnvelopeHeader(msg, Env_SubjectNbr);
			domainContinuum = EnvelopeHeader(msg, Env_ContinuumNbr);
			domainUnit = EnvelopeHeader(msg, Env_PublishUnitNbr);
			domainRole = EnvelopeHeader(msg, Env_PublishRoleNbr);
			if (!ReceiveRAMSCancelPetition(fromGWay, gWay, sub,
				domainContinuum, domainUnit, domainRole))
			{
				return 0;
			}

			if (fprintf(petitionLog, "%u %s %u %d %u %u %u\n",
				(unsigned int) fromGWay->protocol,
				fromGWay->gwEid, cc, sub, domainContinuum,
				domainUnit, domainRole) < 1
			|| fflush(petitionLog) == EOF)
			{
				ErrMsg("Can't log petition cancellation.");
				return 0;
			}

			break;

		case PublishOnReception:
			if (!ReceiveRAMSPublishMessage(fromGWay, gWay, msg))
			{
				return 0;
			}

			break;

		case SendOnReception:
			if (!ReceiveRAMSPrivateMessage(gWay, msg))
			{
				return 0;
			}

			break;

		case AnnounceOnReception:
			if (!ReceiveRAMSAnnounceMessage(fromGWay, gWay, msg))
			{
				return 0;
			}

			break;
	}

	return 1;
}

int AssertPetition(RamsGateway *gWay, Petition *pet)
{
	Lyst pSet;
	LystElt elt; 
	RamsNode *rCont;

	pSet = PropagationSet(gWay, pet);
	for (elt = lyst_first(pSet); elt != NULL; elt = lyst_next(elt))
	{
		rCont = (RamsNode *)lyst_data(elt);
		if (!SendMessageToContinuum(gWay, rCont->continuumNbr, 1,
				pet->specification->envelope, ENVELOPELENGTH))
		{
			ErrMsg("assertion petition error");
			return 0;
		}
	}

	RAMSSetUnion(pet->SourceRamsSet, pSet);
//printRAMSContent(&gWay);
	return 1;
}

int CancelPetition(RamsGateway *gWay, Petition *pet)
{
	LystElt sgsElt; 
	LystElt elt, nextElt;
	RamsNode *rCont; 

	//–	A cancellation of the petition shall be sent to the only
	// members of DGS which is also member of SGS
	// or all member of SGS if DGS has no member
	//

	if ((lyst_length(pet->DestinationRamsSet) == 0)
	&& (lyst_length(pet->DistributionNodeSet) == 0))
	{
		for (elt = lyst_first(pet->SourceRamsSet); elt != NULL;
				elt = lyst_next(elt))
		{
			nextElt = lyst_next(elt);
			rCont = (RamsNode *)lyst_data(elt);
			if (!ConstructEnvelopeToRAMS(gWay, 1, NULL,
				EnvelopeHeader(pet->specification->envelope,
					Env_ContinuumNbr),
				EnvelopeHeader(pet->specification->envelope,
					Env_PublishUnitNbr),
				EnvelopeHeader(pet->specification->envelope,
					Env_PublishRoleNbr),
				EnvelopeHeader(pet->specification->envelope,
					Env_DestIDField), 
				PetitionCancellation, 
				EnvelopeHeader(pet->specification->envelope,
					Env_SubjectNbr), 
				rCont->continuumNbr))
			{
				ErrMsg("error petition cancellation in \
CancelPetition");
				return 0;
			}

			lyst_delete(elt);
			elt = nextElt;
		}
	}
	else if ((lyst_length(pet->DestinationRamsSet) == 1)
			&& (lyst_length(pet->DistributionNodeSet) == 0))
	{
		rCont = (RamsNode *)
			lyst_data(lyst_first(pet->DestinationRamsSet));
		sgsElt = IsContinuumExist(rCont, &(pet->SourceRamsSet));
		if (sgsElt != NULL)
		{
			if (!ConstructEnvelopeToRAMS(gWay, 1, NULL,
				EnvelopeHeader(pet->specification->envelope,
					Env_ContinuumNbr),
				EnvelopeHeader(pet->specification->envelope,
					Env_PublishUnitNbr),
				EnvelopeHeader(pet->specification->envelope,
					Env_PublishRoleNbr),
				EnvelopeHeader(pet->specification->envelope,
					Env_DestIDField),
				PetitionCancellation,
				EnvelopeHeader(pet->specification->envelope,
					Env_SubjectNbr),
				rCont->continuumNbr))
			{
				ErrMsg("error petition cancellation in \
CancelPetition");
				return 0;
			}

			lyst_delete(sgsElt);
		}
	}
	
//printRAMSContent(&gWay);
	return 1; 
}

		
// receive initial petition from remote RAMS gateway
// 
// -- add into declared neighbor, if not in the list yet
// -- send out petition assertion, if petition cont = 0, or cont = fromGateway's Id 
//                                 for fromGateway not in the SGS & DGS
// -- send out inverse additive of local continuum 

static int ReceiveInitialDeclaration(RamsNode *fromGateway, RamsGateway *gWay)
{
//PUTS("<initial declaration> receive initial declaration");

	if (Look_Up_DeclaredRemoteRAMS(gWay, fromGateway->continuumNbr) == NULL)
	// declared gateway
	{
		lyst_insert_last(gWay->ramsMib->declaredNeighbors, fromGateway);
		gWay->ramsMib->totalDeclared++;
	}

	return 1; 
}

// receive petition from remote RAMS counterpart
// not knowing how the fromGway is obtained
static int ReceiveRAMSSubscribePetition(RamsNode *fromGateway,
		RamsGateway *gWay, int subjectNbr, int domainContinuum,
		int domainUnit, int domainRole)
{
	LystElt elt;
	Lyst propSet;
	Petition *pet, *aPet;	

//PUTS("<rams assertion> receive rams petition assertion");

	pet = NULL; 
	// add neighbor into DGS
	for (elt = lyst_first(gWay->petitionSet); elt != NULL;
			elt = lyst_next(elt))
	{
		pet = (Petition *)lyst_data(elt);
		if (IsSubscriptionPublisherIdentical(pet, domainContinuum,
				domainRole, domainUnit, subjectNbr))
		{
			if (IsContinuumExist(fromGateway,
					&(pet->DestinationRamsSet)) == NULL)
			{
			    lyst_insert_last(pet->DestinationRamsSet,
					    fromGateway);
//PUTS("<rams assertion> add into existing DestinationRamsSet");
				break;
			}
			else
			{
//PUTS("<rams assertion> already in existing DestinationRamsSet");
				return 1;
			}
		}
	}

	// no identical petition exist, msg is an envelope
	if (elt == NULL)
	{
//PUTS("<rams assertion> create new petition");
		pet = ConstructPetition(domainContinuum, domainRole,
				domainUnit, subjectNbr, PetitionAssertion);
		lyst_insert_last(pet->DestinationRamsSet, fromGateway);
		lyst_insert_last(gWay->petitionSet, pet);	
	}

	// submit to local continuum
	if (lyst_length(pet->DestinationRamsSet) == 1)
	{
		if (domainContinuum == 0
		|| domainContinuum == gWay->ramsMib->amsMib->localContinuumNbr)
		{
			if (ams_subscribe(gWay->amsNode, domainRole,
					domainContinuum, domainUnit, subjectNbr,
					10, 0, AmsArrivalOrder, AmsAssured) < 0)
			{
				ErrMsg("error on subscribe to local continuum \
in rams subscription");
//PUTS("<rams assertion> error on ams_subscribe");
				return 0;
			}
//PUTS("<rams assertion> local subscription");
		}
	}

	// If subject cited is the pseudo-subject for some continuum
	// - then all relevant petitions relationships with
	// the conduit (the neighbor) are established. 
	if (subjectNbr < 0)
	{
//printf("<rams assertion> subjectNbr = %d\n", subjectNbr); 
		for (elt = lyst_first(gWay->petitionSet); elt != NULL;
				elt = lyst_next(elt))
		{
			aPet = (Petition *)lyst_data(elt);
			if (IsPetitionPublisherIdentical(aPet, pet))
				continue;
			if (IsPetitionAssertable(gWay, aPet))
			{
				propSet = PropagationSet(gWay, aPet);
				// if neighbor is a memeber of the computed PS
				if (IsContinuumExist(fromGateway, &propSet))
				{
//printf("<rams assertion> propagate to %d  petition cId = %d unit = %d role = %d node = %d sub = %d\n",  fromGateway->continuumNbr, EnvelopeHeader(aPet->specification->envelope, Env_ContinuumNbr), EnvelopeHeader(aPet->specification->envelope, Env_PublishUnitNbr), EnvelopeHeader(aPet->specification->envelope, Env_PublishRoleNbr), EnvelopeHeader(aPet->specification->envelope, Env_DestIDField), EnvelopeHeader(aPet->specification->envelope, Env_SubjectNbr));
					if (!SendMessageToContinuum(gWay,
						fromGateway->continuumNbr, 1,
						aPet->specification->envelope,
						ENVELOPELENGTH))
					{
//PUTS("<rams assertion> propagation error");
						ErrMsg("error in propagate \
petition");
						return 0;
					}

			    		lyst_insert_last(aPet->SourceRamsSet,
							fromGateway);
				}
			}
		}
	}
		
	// assert petition to neighbors
	if (IsPetitionAssertable(gWay, pet))
	{
//PUTS("<rams assertion> assert petition");		
		if (!AssertPetition(gWay, pet))
		{
//PUTS("<rams assertion> assert petition to other neighbors error");
			ErrMsg("assert petition error in \
ReceiveRAMSSubscribePetition");
			return 0;
		}
	}

//PUTS("<rams assertion>");
//printRAMSContent(&gWay);
	return 1; 
}


// receive a RAMS PDU from counterpartRAMS gateway
// with control code == 3, cancel petition
static int ReceiveRAMSCancelPetition(RamsNode *fromGWay,
		RamsGateway *gWay, int subjectNbr, int domainContinuum,
		int domainUnit, int domainRole)
{
	LystElt elt, delWay; 
	Petition *pet, *aPet; 

//PUTS("<rams cancellation> receive rams petition cancellation");
	pet = NULL; 

	// delete this neighbor from DGS of this petition

	for (elt = lyst_first(gWay->petitionSet); elt != NULL;
			elt = lyst_next(elt))
	{
		pet = (Petition *) lyst_data(elt);
		if (IsSubscriptionPublisherIdentical(pet, domainContinuum,
				domainRole, domainUnit, subjectNbr))
		{
			if ((delWay = IsContinuumExist(fromGWay,
					&(pet->DestinationRamsSet))) != NULL)
			{
				lyst_delete(delWay);				
//printf("<rams cancellation> removed continuum %d from DGS for %d \n", fromGWay->continuumNbr, subjectNbr);
			}
			else
			{
//PUTS("<rams cancellation> already deleted from DestinationRamsSet (DGS)");
				return 1;
			}
			break;
		}
	}

	// petition not found

	if (elt == NULL)
	{
//PUTS("<rams cancellation> no matching petition");
		ErrMsg("no matching petition found");
		return 1;
	}

	// if the DGS of this petition now has zero members, unsubscribe

	if (lyst_first(pet->DestinationRamsSet) == NULL)
	{
		if (domainContinuum == 0
		|| domainContinuum == gWay->ramsMib->amsMib->localContinuumNbr)
		{
			if (ams_unsubscribe(gWay->amsNode, domainRole,
				domainContinuum, domainUnit, subjectNbr) < 0)
			{
				ErrMsg("error on unsubscribe to local \
continuum in rams petition cancellation");
//PUTS("<rams cancellation> error on ams_unsubscribe");
				return 0;
			}
//PUTS("<rams cancellation> local unsubscription okay");
		}
	}

	// if the subject cited is the pseudo-subject for the neighbor's
	// own continuum, the neighbor is retracting itself; remove it
	// from SGS and DGS of all petitions (per interop testing at APL)

#if 0
/*	Changed per interop testing at APL.	*/
	if (subjectNbr < 0)
#endif
	if (subjectNbr == (0 - fromGWay->continuumNbr))
	{
//printf("<rams cancellation> continuum nbr %d is retracting\n", fromGWay->continuumNbr);
//printf("<rams cancellation> subjectNbr = %d\n", subjectNbr); 
		for (elt = lyst_first(gWay->petitionSet); elt != NULL;
				elt = lyst_next(elt))
		{
			aPet = (Petition *)lyst_data(elt);
#if 0
/*	Changed per interop testing at APL.	*/
	if (subjectNbr < 0)
			if (IsPetitionPublisherIdentical(aPet, pet))
				continue;
			if (((delWay = IsContinuumExist(fromGWay, &(aPet->SourceRamsSet)))!= NULL) &&
				((EnvelopeHeader(aPet->specification->envelope, Env_ContinuumNbr) == -subjectNbr)||(-subjectNbr == fromGWay->continuumNbr)))
#endif
			if (((delWay = IsContinuumExist(fromGWay,
					&(aPet->SourceRamsSet))) != NULL))
			{
//printf("<rams cancellation> deleting %d from SGS of petition cId = %d unit = %d role = %d node = %d sub = %d\n",  fromGWay->continuumNbr, EnvelopeHeader(aPet->specification->envelope, Env_ContinuumNbr), EnvelopeHeader(aPet->specification->envelope, Env_PublishUnitNbr), EnvelopeHeader(aPet->specification->envelope, Env_PublishRoleNbr), EnvelopeHeader(aPet->specification->envelope, Env_DestIDField), EnvelopeHeader(aPet->specification->envelope, Env_SubjectNbr));
				if (!ConstructEnvelopeToRAMS(gWay, 1, NULL,
				EnvelopeHeader(aPet->specification->envelope,
					Env_ContinuumNbr),
				EnvelopeHeader(aPet->specification->envelope,
					Env_PublishUnitNbr),
				EnvelopeHeader(aPet->specification->envelope,
					Env_PublishRoleNbr),
				EnvelopeHeader(aPet->specification->envelope,
					Env_DestIDField),
				PetitionCancellation,
				EnvelopeHeader(aPet->specification->envelope,
					Env_SubjectNbr),
				fromGWay->continuumNbr))
				{
					ErrMsg("error petition cancellation \
in ReceiveRAMSCancelPetition");
//PUTS("<rams cancellation> delete fails");
					return 0;
				}

				lyst_delete(delWay);
			}

/*	Add per interop testing at APL.		*/

			if (((delWay = IsContinuumExist(fromGWay,
				&(aPet->DestinationRamsSet))) != NULL))
			{
//printf("<rams cancellation> deleting %d from DGS of petition cId = %d unit = %d role = %d node = %d sub = %d\n",  fromGWay->continuumNbr, EnvelopeHeader(aPet->specification->envelope, Env_ContinuumNbr), EnvelopeHeader(aPet->specification->envelope, Env_PublishUnitNbr), EnvelopeHeader(aPet->specification->envelope, Env_PublishRoleNbr), EnvelopeHeader(aPet->specification->envelope, Env_DestIDField), EnvelopeHeader(aPet->specification->envelope, Env_SubjectNbr));
				if (!ConstructEnvelopeToRAMS(gWay, 1, NULL,
				EnvelopeHeader(aPet->specification->envelope,
					Env_ContinuumNbr),
				EnvelopeHeader(aPet->specification->envelope,
					Env_PublishUnitNbr),
				EnvelopeHeader(aPet->specification->envelope,
					Env_PublishRoleNbr),
				EnvelopeHeader(aPet->specification->envelope,
					Env_DestIDField),
				PetitionCancellation,
				EnvelopeHeader(aPet->specification->envelope,
					Env_SubjectNbr),
				fromGWay->continuumNbr))
				{
					ErrMsg("error petition cancellation \
in ReceiveRAMSCancelPetition");
//PUTS("<rams cancellation> delete fails");
					return 0;
				}

				lyst_delete(delWay);
			}

	// cancel the petition if
	// - DNS is empty
	// - DGS is empty or the only member of DGS is inside member of SGS
//printf("<rams cancellation> before test cancel petition cId = %d unit = %d role = %d node = %d sub = %d\n",  EnvelopeHeader(aPet->specification->envelope, Env_ContinuumNbr), EnvelopeHeader(aPet->specification->envelope, Env_PublishUnitNbr), EnvelopeHeader(aPet->specification->envelope, Env_PublishRoleNbr), EnvelopeHeader(aPet->specification->envelope, Env_DestIDField), EnvelopeHeader(aPet->specification->envelope, Env_SubjectNbr));

			if (IsToCancelPetition(aPet))
			{
//PUTS("<rams cancellation> cancelling this petition"); 
				if (!CancelPetition(gWay, aPet))
				{
					ErrMsg("error in propagating petition \
cancel in ReceiveRAMSCancelPetition");
//PUTS("<rams cancellation> error in cancelling the petition");
					return 0;
				}
			}
		}
	}

#if 0
/*	Superseded by scan through all petitions, above, per interop
	testing at APL.							*/

	// cancel the petition if
	// - DNS is empty
	// - DGS is empty or the only member of DGS is inside member of SGS
//printf("<rams cancellation> before test cancel petition cId = %d unit = %d role = %d node = %d sub = %d\n",  EnvelopeHeader(pet->specification->envelope, Env_ContinuumNbr), EnvelopeHeader(pet->specification->envelope, Env_PublishUnitNbr), EnvelopeHeader(pet->specification->envelope, Env_PublishRoleNbr), EnvelopeHeader(pet->specification->envelope, Env_DestIDField), EnvelopeHeader(pet->specification->envelope, Env_SubjectNbr));

//printRAMSContent(&gWay);

	if (IsToCancelPetition(pet))
	{
//PUTS("<rams cancellation> cancelling this petition"); 
		if (!CancelPetition(gWay, pet))
		{
			ErrMsg("error in propagating petition cancel in \
ReceiveRAMSCancelPetition");
//PUTS("<rams cancellation> error in cancelling the petition");
			return 0;
		}
	}
#endif

	if (subjectNbr < 0 && fromGWay->continuumNbr == -subjectNbr)
	{
		if ((elt = IsContinuumExist(fromGWay,
				&(gWay->ramsMib->declaredNeighbors))) != NULL)
		{
//printf("<rams cancellation> remove continuum %d from list of declared continua\n", fromGWay->continuumNbr);
			lyst_delete(elt);
		}
	}

//PUTS("<rams cancellation>");
//printRAMSContent(&gWay);
	return 1; 
}

// receive a RAMS PDU from a counterpart RAMS gateway 
//with control code greater than 3
//control code = 4 case
static int	ReceiveRAMSPublishMessage(RamsNode* fromGWay, RamsGateway *gWay,
			char *msg)
{
	LystElt elt, eltBp, eltNode;
	Lyst newRSet, newNSet;
	Node *node;
	RamsNode *ramsNode;	//	formerly bpP
	Petition *pet;
	int localcn = gWay->ramsMib->amsMib->localContinuumNbr;

//PUTS("<rams publish> receive rams publish message");
//printf("<rams publish> from contId=%d unit=%d nodeId=%d\n", EnclosureHeader(EnvelopeText(msg, -1), Enc_ContinuumNbr), EnclosureHeader(EnvelopeText(msg, -1), Enc_UnitNbr), EnclosureHeader(EnvelopeText(msg, -1), Enc_NodeNbr));

	newRSet = lyst_create();
	newNSet = lyst_create(); 
	for (elt = lyst_first(gWay->petitionSet); elt != NULL;
			elt = lyst_next(elt))
	{
		pet = (Petition *)lyst_data(elt);
		if (IsRAMSPDUSatisfyPetition(gWay->amsNode, msg, pet))
			// check petition and enclosure
		{
			for (eltBp = lyst_first(pet->DestinationRamsSet);
					eltBp != NULL; eltBp = lyst_next(eltBp))
			{
				ramsNode = (RamsNode *)lyst_data(eltBp);
				if (ramsNode->continuumNbr
						== fromGWay->continuumNbr)
					continue;
				if (!IsContinuumExist(ramsNode, &newRSet))
				{
					lyst_insert_last(newRSet, ramsNode);
				}
			}

			for (eltNode = lyst_first(pet->DistributionNodeSet);
				eltNode != NULL; eltNode = lyst_next(eltNode))
			{
				node = (Node *)lyst_data(eltNode);
				if (!IsNodeExist(node, &newNSet))
				{
					lyst_insert_last(newNSet, node);
				}
			}
		}
	}			

	for (eltBp = lyst_first(newRSet); eltBp != NULL;
			eltBp = lyst_next(eltBp))
	{
		ramsNode = (RamsNode *)lyst_data(eltBp);
//printf("<rams publish> send message to rams %d\n", ramsNode->continuumNbr); 
		if (!SendMessageToContinuum(gWay, ramsNode->continuumNbr, 1,
				msg, ENVELOPELENGTH + EnvelopeHeader(msg,
				Env_EnclosureLength)))
			// send to neighbor RAMS, only one
		{
//printf("<rams publish> forward rams publish message to %d error\n", ramsNode->continuumNbr);
		   ErrMsg("error in Forward RAMS publish Message");
		   return 0;
		}
	}
    
	for (eltNode = lyst_first(newNSet); eltNode != NULL; eltNode = lyst_next(eltNode))
	{
		node = (Node *)lyst_data(eltNode);
//printf("<rams publish> send message to unit=%d node=%d\n", node->unitNbr, node->nbr);
		if (ams_send(gWay->amsNode, localcn, node->unitNbr, node->nbr,
				(0 - localcn), 8, 0, EnvelopeHeader(msg,
				Env_EnclosureLength), EnvelopeText(msg, -1), 0)
				< 0)   // send enclosure on subject 0
		{
//PUTS("<rams publish> ams_send error");
			ErrMsg("error in sending message of \
ReceiveRAMSPublishMessage");
			return 0;
		}
	}
	return 1;
}

// control code = 5 (send on reception)

static int ReceiveRAMSPrivateMessage(RamsGateway *gWay, char *msg)
{
	int con;
	RamsNode *ramsNode;		//	formerly bpP

//PUTS("<rams private> receive rams private message");
	con =  EnvelopeHeader(msg, Env_ContinuumNbr);
//printf("<rams private> message is for conId = %d local conId = %d\n", con, gWay->ramsMib->amsMib->localContinuumNbr);
//printf("<rams private> message is %s\n", msg+ENVELOPELENGTH+AMSMSGHEADER);
	if (con != gWay->ramsMib->amsMib->localContinuumNbr)
	{
		ramsNode = GetConduitForContinuum(con, gWay);
//printf("<rams private> forward to continuum %d\n", ramsNode->continuumNbr);
		if (!SendMessageToContinuum(gWay, ramsNode->continuumNbr,
				1, msg, ENVELOPELENGTH + EnvelopeHeader(msg,
				Env_EnclosureLength)))
			// send to neighbor RAMS, only one
		{
//PUTS("<rams private> forward error"); 
		   ErrMsg("error in Forward Private Message");
		   return 0;
		}

		return 1;
		// should have only destination node from only one cOrder
	}
	else
	{
		if (IsPrivateReceiverExist(gWay, msg) != NULL)
		{
//printf("<rams private> send message to con=%d unit=%d node=%d sub = 0\n", con, EnvelopeHeader(msg, Env_DestUnitNbr), EnvelopeHeader(msg, Env_DestNodeNbr));
			if (ams_send(gWay->amsNode, con,
				EnvelopeHeader(msg, Env_DestUnitNbr),
				EnvelopeHeader(msg, Env_DestNodeNbr),
				(0 - con), 8, 0,
				EnvelopeHeader(msg, Env_EnclosureLength),
				EnvelopeText(msg, -1), 0) < 0)
			{
//PUTS("<rams private> ams_send error");
				ErrMsg("error in Receive RAMS Private Message");
				return 0;
			}
		}
		else
		{
//printf("<rams private> no valid invitor exist for con=%d unit=%d node=%d sub=%d\n", con, EnvelopeHeader(msg, Env_DestUnitNbr), EnvelopeHeader(msg, Env_DestNodeNbr), EnvelopeHeader(msg, Env_SubjectNbr));
		}
	}
	return 1; 
}

//  control code = 6 (announce on reception)
static int	ReceiveRAMSAnnounceMessage(RamsNode* fromGWay,
			RamsGateway *gWay, char *msg)
{

	int con, domainRole, domainUnit;
	LystElt elt, nodeElt;
	Lyst newSet;
	Invitation *inv;
	Node *amsNode; 
	RamsNode *ramsNode;		//	formerly bpP
	//int subNo;
	int localcn = gWay->ramsMib->amsMib->localContinuumNbr;

//printf("<rams announce> receive rams private message %s\n", msg+ENVELOPELENGTH+AMSMSGHEADER);
	con =  EnvelopeHeader(msg, Env_ContinuumNbr);
//printf("<rams announce> domain cont = %d  local cont = %d\n", con, gWay->ramsMib->amsMib->localContinuumNbr);

	// continuum is 0, and network is tree, forward message to neighbor
	if (con == 0)
	{
		if (gWay->ramsMib->netType == TREETYPE)
		{
			for (elt = lyst_first(gWay->ramsMib->declaredNeighbors);
					elt != NULL; elt = lyst_next(elt))
			{
				ramsNode = (RamsNode *)lyst_data(elt);
				if (ramsNode->continuumNbr
						== fromGWay->continuumNbr)
					continue;
//printf("<rams announce> forward announce to conId = %d \n", ramsNode->continuumNbr);
				if (!SendMessageToContinuum(gWay,
					ramsNode->continuumNbr, 1, msg,
					ENVELOPELENGTH + EnvelopeHeader(msg,
						Env_EnclosureLength)))
				{
//PUTS("<rams announce> forward error");
					ErrMsg("error in forward RAMS \
announce message");
					return 0;
				}
			}
		}
	}

	// continuum is not zero, not local Id, forward to condiut
	else if (con != localcn)
	{
		ramsNode = GetConduitForContinuum(con, gWay);
		if (ramsNode)
		{
//printf("<rams announce> forward announce to %d\n", ramsNode->continuumNbr);
			if (!SendMessageToContinuum(gWay,
				ramsNode->continuumNbr, 1, msg,
				ENVELOPELENGTH + EnvelopeHeader(msg,
					Env_EnclosureLength)))
			{
//PUTS("<rams announce> forward error");
				ErrMsg("error in fwd RAMS announce message");
				return 0;
			}
		}
		else
		{
//PUTS("<rams announce> cannot find out conduit");
		}
	}
  
	// con is 0, or local Id, local announce.

	newSet = lyst_create();

	domainUnit = EnvelopeHeader(msg, Env_DestUnitNbr);
	domainRole = EnvelopeHeader(msg, Env_DestRoleNbr);
	if (con == 0 || con == localcn)
	{
		//get all eligible node invitor
		for (elt = lyst_first(gWay->invitationSet); elt != NULL;
				elt = lyst_next(elt))
		{
			inv = (Invitation *)lyst_data(elt);
			if (IsRAMSPDUSatisfyInvitation(gWay, msg, inv))
			// check if source satify invitaion requirement
			{
				for (nodeElt = lyst_first(inv->nodeSet);
						nodeElt != NULL;
						nodeElt = lyst_next(nodeElt))
				{
					amsNode = (Node *)lyst_data(nodeElt);
					if (IsValidAnnounceReceiver(gWay,
							amsNode, domainUnit,
							domainRole))
						// check if node satisfy 
						// announce destination
					{
						if (!IsNodeExist(amsNode,
								&newSet))
						{
							lyst_insert_last(newSet,
								amsNode);
						}
					}
				}
			}
		}

//PUTS("<rams announce> sent announce to");
		for (elt = lyst_first(newSet); elt != NULL;
				elt = lyst_next(elt))
		{
			amsNode = (Node *)lyst_data(elt);
//printf("<rams announce>        unit=%d node=%d\n", amsNode->unitNbr, amsNode->nbr);
			if (ams_send(gWay->amsNode, localcn, amsNode->unitNbr,
					amsNode->nbr, (0 - localcn), 
					 //EnvelopeHeader(msg, Env_SubjectNbr), 
					 8, 0, EnvelopeHeader(msg,
						 Env_EnclosureLength),
					 EnvelopeText(msg, -1), 0) < 0)
			{
//PUTS("<rams announce> ams_send error");
			     ErrMsg("error in  Receive RAMS Announce Message");
			     return 0;
			}
			
		}

/*
printf("<rams private> local announce to con=%d unit=%d node=%d sub=%d\n", con, EnvelopeHeader(msg, Env_UnitNbr), EnvelopeHeader(msg, Env_NodeNbr), EnvelopeHeader(msg, Env_SubjectNbr)); 
	    if (ams_announce(gWay->amsNode, EnvelopeHeader(msg, Env_RoleNbr),
	    		con, EnvelopeHeader(msg, Env_UnitNbr),
		//	EnvelopeHeader(msg, Env_SubjectNbr), 
			0, 8, 0, EnvelopeHeader(msg, Env_EnclosureLength),
			EnvelopeText(msg, -1), 0) < 0)
		{
			ErrMsg("error in Receive RAMS Private Message");
			return 0;
		}
*/
	}
	return 1; 
}
	
// receive petition from node of same message space
// node????
static int	ReceivePetition(Node *sourceNode,  RamsGateway *gWay,
			int domainContinuum, int domainRole, int domainUnit,
			int subjectNbr)
{
	LystElt elt;
	Petition *pet;

//PUTS("<receive petition> receive petition");

	  // add into DNS
	 for (elt = lyst_first(gWay->petitionSet); elt != NULL;
			 elt = lyst_next(elt))
	 {
		 pet = (Petition *)lyst_data(elt);
		 if (IsSubscriptionPublisherIdentical(pet, domainContinuum,
				 domainRole, domainUnit, subjectNbr))
		 {
			 if (!IsNodeExist(sourceNode,
					 &(pet->DistributionNodeSet)))
			 {  			    
				lyst_insert_last(pet->DistributionNodeSet,
						sourceNode);
//printf("<receive petition> add node %d into existing petition DNS\n", sourceNode->nbr);
			 }

			 break;
		 }
	 }

	 // no identical petition exist yet
	 if (elt == NULL)
	 {
//PUTS("<receive petition>create new petition");
		pet = ConstructPetition(domainContinuum, domainRole,
				domainUnit, subjectNbr, PetitionAssertion);
		lyst_insert_last(pet->DistributionNodeSet, sourceNode);
		lyst_insert_last(gWay->petitionSet, pet);
	 }

	 // assert petition
	 if (!AssertPetition(gWay, pet))
	 {
		 ErrMsg("error assert petition in ReceivePetition");
		 return 0; 
	 }

//PUTS("<receive petition>");
//printRAMSContent(&gWay);
	 return 1;  
}

// receive cancel petition from node of same message space
static int ReceiveCancelPetition(Node *sourceNode,  RamsGateway *gWay, int domainContinuum, int domainRole, int domainUnit, int subjectNbr)
{
	 LystElt elt, nodeElt;
	 Petition *pet;
	 
//PUTS("<cancel petition> cancel petition");

	 for (elt = lyst_first(gWay->petitionSet); elt != NULL;
			 elt = lyst_next(elt))
	 {
		 pet = (Petition *)lyst_data(elt);
		 if (IsSubscriptionPublisherIdentical(pet, domainContinuum,
				 domainRole, domainUnit, subjectNbr))
		 {
			 if ((nodeElt = IsNodeExist(sourceNode,
					 &(pet->DistributionNodeSet))) != NULL)
			 {
				 lyst_delete(nodeElt);		
//printf("<cancel petition> delete node %d from exist distribution order\n", sourceNode->nbr);
			 }

			 break;
		 }
	 }

	 if (elt == NULL)   // not found the petition
	 {
//PUTS("<cancel petition> no matched petition found");
		ErrMsg("no matched petition found in ReceiveCancelPetition");
		return 1;
	 }

	 if (IsToCancelPetition(pet))
	 {
		 if (CancelPetition(gWay, pet) == 0)
		 {
			 ErrMsg("error in propagating petition cancel in \
 ReceiveCancelPetition");
			 return 0;
		 }
//printf("<cancel petition> cancel petition cId = %d unit = %d role = %d node = %d \n",  EnvelopeHeader(pet->specification->envelope, Env_ContinuumNbr), EnvelopeHeader(pet->specification->envelope, Env_PublishUnitNbr), EnvelopeHeader(pet->specification->envelope, Env_PublishRoleNbr), EnvelopeHeader(pet->specification->envelope, Env_DestIDField));
	 }

//PUTS("<cancel petition>");
//printRAMSContent(&gWay);
	return 1;
}

// amsEvent contain amsHeader (source node information) and envelope (envelope header and enclosure)
static int ForwardPublishMessage(RamsGateway *gWay, AmsEvent amsEvent)
{
	LystElt elt;
	Lyst legalN; 
	RamsNode *remoteGWay;
	Enclosure *enc;
	Petition *pet;
	char* content;
	AmsMsgType msgType;
	int conId, unitNbr, nodeNbr, subNbr, contentLen, context, priority;
	unsigned char flowLabel;
	int roleNbr; 

//PUTS("<forward publish> forward publish message");
	ams_parse_msg(amsEvent, &conId, &unitNbr, &nodeNbr, &subNbr,
			&contentLen, &content,    // contId: source information
			&context, &msgType, &priority, &flowLabel);
//printf("<forward publish> contentLength = %d \n", contentLen);// ?? check??

	roleNbr = RoleNumber(gWay->amsNode, unitNbr, nodeNbr);
//	if (roleNbr < 0)
//		return 1;
	
	// construct enclosure
	enc = ConstructEnclosure(conId, unitNbr, nodeNbr, subNbr, contentLen,
			content, context, msgType, priority, flowLabel);

	//	If flow label is invalid, map AMS priority to BP COS
	if ((flowLabel & 0x03) == 3)	//	Invalid BP class of service 3
	{
		flowLabel = (15 - priority) / 5;
	}

	// for each neighbor that is member of the DGS of at least one
	// petition whose sub. spec. is satisfied by received message header
	// calculation union of DGS of all satisfying the petition

	legalN = lyst_create();
	for (elt = lyst_first(gWay->petitionSet); elt != NULL;
			elt = lyst_next(elt))
	{		
		pet = (Petition *)lyst_data(elt);
		if (lyst_first(pet->DestinationRamsSet) == NULL)
			continue; 
		// check if the sourceNode of message match to the
		// petition specification in collection order
		if (!IsAmsMsgSatisfyPetition(gWay->amsNode, conId, unitNbr,
				nodeNbr, subNbr, pet))
			continue;
      
		RAMSSetUnion(legalN, pet->DestinationRamsSet);
	}

	// forward to all legal neighbor

	for (elt = lyst_first(legalN); elt != NULL; elt = lyst_next(elt))
	{
		remoteGWay = (RamsNode *)lyst_data(elt);		    
			
		// put source role number into the envelope to
		// be sent to remote continuum
//printf("<forward publish> forward message to con=%d \n", remoteGWay->continuumNbr); 
		if (!ConstructEnvelopeToRAMS(gWay, flowLabel, enc, 0, 0,
				roleNbr, 0, PublishOnReception, subNbr,
				remoteGWay->continuumNbr))
		{
//PUTS("<forward publish> forward public message error");
			ErrMsg("error in forward publish message");
			DeleteEnclosure(enc);
			return 0;
		}
	}
	
	DeleteEnclosure(enc);
	return 1; 
}


// received content is an envelope on "send on reception"
// or "announce on reception"
static int	ForwardPrivateMessage(RamsGateway *gWay,
			unsigned char flowLabel, char* content,
			int contentLength)
{
	LystElt elt; 
	RamsNode *ramsNode;		//	formerly bpP
	int contId; 

//PUTS("<forward private> forward private message");
//printf("<forward private> need to forward private to con %d \n", EnvelopeHeader(content, Env_ContinuumNbr));

	contId = EnvelopeHeader(content, Env_ContinuumNbr);  
//printf("<forward private> message is %s\n", content+ENVELOPELENGTH+AMSMSGHEADER);
	// if contId == 0, announce the message
	if (contId == 0)
	{
		for (elt = lyst_first(gWay->ramsMib->declaredNeighbors);
				elt != NULL; elt = lyst_next(elt))
		{
			ramsNode = (RamsNode *)lyst_data(elt);
//printf("<forward private> forward private to con %d \n", ramsNode->continuumNbr);
			if (!SendMessageToContinuum(gWay,
					ramsNode->continuumNbr, flowLabel,
					content, contentLength))
				// send to neighbor RAMS, only one
			{
				ErrMsg("error in Forward Private Message");
				return 0;
			 }
		}
	}
	else if (contId > 0)
	{
		ramsNode = GetConduitForContinuum(contId, gWay);
		if (ramsNode)
		{
//printf("<forward private> forward private to con %d \n", ramsNode->continuumNbr);
			if (!SendMessageToContinuum(gWay,
					ramsNode->continuumNbr, flowLabel,
					content, contentLength))
				// send to neighbor RAMS, only one
			{
//PUTS("<forward private> forward error");
				ErrMsg("error in Forward Private Message");
				return 0;
			 }
		}
		else
		{
//PUTS("<forward private> no conduit exist");
		}
	}
	return 1;
}


int SendPetitionToDeclaredRAMS(RamsGateway *gWay, char *env)
{
	LystElt elt;
	RamsNode *decRams; 
	int succeed; 

	if (env == NULL)
		return 0;

	succeed = 1; 
	for (elt = lyst_first(gWay->ramsMib->declaredNeighbors); elt != NULL;
			elt = lyst_next(elt))
	{
		decRams = (RamsNode *)lyst_data(elt);
		if (!SendMessageToContinuum(gWay, decRams->continuumNbr, 1,
				env, ENVELOPELENGTH + EnvelopeHeader(env,
				Env_EnclosureLength)))
		{
			succeed = 0;
		}
	}
	return succeed;
}

int	SendMessageViaBp(RamsGateway *gWay, RamsNode *ramsNode,
		unsigned char flowLabel, char* envelope, int envelopeLength)
{
	int		classOfService;
	BpCustodySwitch	custodySwitch = SourceCustodyRequired;
	BpExtendedCOS	ecos = { 0, 0, 0 };
	Object		extent;
	Object		bundleZco;
	Object		newBundle;
	char		errorMsg[128];

	while (sdr == NULL)
	{
		PUTS("sdr has not been set yet.");
		snooze(1);
	}

	classOfService = flowLabel & 0x03;
	ecos.flags = (flowLabel >> 2) & 0x03;
	if (ecos.flags & BP_BEST_EFFORT)
	{
		custodySwitch = NoCustodyRequested;
	}

	sdr_begin_xn(sdr);
//PUTS("after sdr_begin_xn");
	extent = sdr_insert(sdr, envelope, envelopeLength);
	if (extent == 0)
	{
		sdr_cancel_xn(sdr);
		ErrMsg("can't write msg to SDR");
		return 0;
	}

//PUTS("after sdr_insert");

	bundleZco = zco_create(sdr, ZcoSdrSource, extent, 0, envelopeLength);
//PUTS("after zco_create");
	if (sdr_end_xn(sdr) < 0 || bundleZco == 0)
	{
		ErrMsg("failed creating message");
		return 0;
	}
//PUTS("after sdr_end_xn");

	if (bp_send(sap, BP_BLOCKING, ramsNode->gwEid, NULL, ttl,
			classOfService, custodySwitch, 0, 0, &ecos,
			bundleZco, &newBundle) < 1)
	{
		isprintf(errorMsg, sizeof errorMsg, "cannot send message to %s",
				ramsNode->gwEid);
		ErrMsg(errorMsg);
		return 0;
	}
//printf("send to %s\n", destId);

	return 1;
}

int	SendMessageViaUdp(RamsGateway *gWay, RamsNode *ramsNode,
		unsigned char flowLabel, char* envelope, int envelopeLength)
{
	char			gwEid[256];
	unsigned short		portNbr;
	unsigned int		ipAddress;
	struct sockaddr		socketName;
	struct sockaddr_in	*inetName = (struct sockaddr_in *) &socketName;
	char			errorMsg[128];

	istrcpy(gwEid, ramsNode->gwEid, sizeof gwEid);
	parseSocketSpec(gwEid, &portNbr, &ipAddress);
	portNbr = htons(portNbr);
	ipAddress = htonl(ipAddress);
	memset((char *) &socketName, 0, sizeof socketName);
	inetName->sin_family = AF_INET;
	inetName->sin_port = portNbr;
	memcpy((char *) &(inetName->sin_addr.s_addr), (char *) &ipAddress, 4);
	while (1)
	{
		if (sendto(ownUdpFd, envelope, envelopeLength, 0, &socketName,
				sizeof(struct sockaddr)) < 0)
		{
			if (errno == EINTR)	/*	Interrupted.	*/
			{
				continue;	/*	Retry.		*/
			}

			isprintf(errorMsg, sizeof errorMsg,
				"cannot send message to %s", ramsNode->gwEid);
			ErrMsg(errorMsg);
			return 0;
		}

		return 1;
	}
}
		
int	SendMessageToContinuum(RamsGateway *gWay, int continuumId,
		unsigned char flowLabel, char* envelope, int envelopeLength)
{
	char		errorMsg[128];
	LystElt		elt;
	RamsNode	*ramsNode;		//	formerly bpP
	//int		i;

//printf("<SendMessageToContinuum> to %d\n", continuumId);
	if (continuumId == 0)	//	Send to all continua
	{
//PUTS("<SendMessageToContinuum> sent to the following continuum");
		for (elt = lyst_first(gWay->ramsMib->ramsNeighbors);
				elt != NULL; elt = lyst_next(elt))
		{
			ramsNode = (RamsNode *) lyst_data(elt);
			if (ramsNode->continuumNbr ==
				gWay->ramsMib->amsMib->localContinuumNbr)
			{
				continue;
			}

//printf("<SendMessageToContinuum> to %d envelopeLength=%d cc=%d\n", ramsNode->continuumNbr, envelopeLength, EnvelopeHeader(envelope, Env_ControlCode));
			switch (ramsNode->protocol)
			{
			case RamsBp:
				if (!SendMessageViaBp(gWay, ramsNode,
					flowLabel, envelope, envelopeLength))
				{
					ErrMsg("Can't send RAMS msg via BP.");
					return 0;
				}

				continue;

			case RamsUdp:
				if (!SendMessageViaUdp(gWay, ramsNode,
					flowLabel, envelope, envelopeLength))
				{
					ErrMsg("Can't send RAMS msg via UDP.");
					return 0;
				}

				continue;

			default:
				ErrMsg("No RAMS network protocol.");
				return 0;
			}
		}

		return 1;
	}

	//	This message is being sent to a single continuum.

	ramsNode = Look_Up_DeclaredRemoteRAMS(gWay, continuumId);
	if (ramsNode == NULL)
	{
		isprintf(errorMsg, sizeof errorMsg, "continuum %d does not \
exist", continuumId);
		ErrMsg(errorMsg);
//printf("<SendMessageToContinuum> continuum %d not declared yet\n", continuumId);
		return 0;
	}

	switch (ramsNode->protocol)
	{
	case RamsBp:
		return SendMessageViaBp(gWay, ramsNode, flowLabel, envelope,
				envelopeLength);

	case RamsUdp:
		return SendMessageViaUdp(gWay, ramsNode, flowLabel, envelope,
				envelopeLength);

	default:
		ErrMsg("No RAMS network protocol.");
		return 0;

	}
}
