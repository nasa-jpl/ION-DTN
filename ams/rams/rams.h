/*
	rams.h:		public definitions supporting the
			implementation of Remote Asynchronous
			Message Service.

	Author: Shin-Ywan (Cindy) Wang
	Copyright (c) 2005, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
*/

#ifndef	_RAMS_H
#define	_RAMS_H

#include "ams.h"
#include "amscommon.h"
#include "platform.h"
#include "memmgr.h"
#include "psm.h"
#include "lyst.h"
#include "llcv.h"
#include "bp.h"
#include "sdr.h"
#include "zco.h"

#ifdef _cplusplus
extern "C" {
#endif

#define ENVELOPELENGTH	12
#define AMSMSGHEADER	16

#define	ErrMsg(text)	putErrmsg(text, NULL)

typedef enum
{
	MESHTYPE = 1,
	TREETYPE = 2
} RAMSNetworkType;

typedef struct
{
	int		continuumNbr;
	RamsNetProtocol	protocol;
	char		*gwEid;
} RamsNode;

typedef struct decl_s
{
	time_t		checkTime;
	RamsNode	*neighbor;
	unsigned char	flowLabel;
	char		*envelope;
	int		envelopeLength;
} UdpRpdu;

/*	RamsGateway is the module's RAMS Gateway operational state.	*/
typedef  struct ramsgateway
{
	AmsMib		*amsMib;
	AmsModule	amsModule;	/* gateway's AMS module state	*/

	Lyst		petitionSet;	/* list of (Petition *)		*/ 
	Lyst		invitationSet;	/* list of (Invitation *)	*/
	Lyst		registerSet;	/* list of registered modules	*/
	
	pthread_t	primeThread;   
	pthread_t	petitionReceiveThread;
	pthread_t	amsReceiveThread;

	Lyst		ramsNeighbors;	/*	expected neighbors	*/
	int		neighborsCount;
	Lyst		declaredNeighbors;
	int		declaredNeighborsCount;

	int		stopping;

	RAMSNetworkType	netType;
	RamsNetProtocol	netProtocol;

	/*	For netProtocol == RamsUdp....				*/

	int		ownUdpFd;
	Lyst		udpRpdus;	/*	(UdpRpdu *)		*/

	/*	For netProtocol == RamsBp....				*/

	BpSAP		sap;
	int		ttl;
} RamsGateway, *RamsGate;

typedef struct
{
	int		controlCode;
	int		continuumNbr;
	int		unitNbr;
	int		moduleNbr;
	int		roleNbr;
	int		subjectNbr;
	int		enclosureLength;
	char		*enclosure;
} Envelope;

typedef struct
{
	int		length;
	char		*text;
} Enclosure;

typedef struct
{
	char		*envelope;
	int		envelopeLength;
	int		toContinuumNbr;
} PetitionSpec;

typedef struct
{
	PetitionSpec	*specification;
	Lyst		DistributionModuleSet;
	Lyst		DestinationNodeSet;
	Lyst		SourceNodeSet;
	int		stateIsFromPlayback;
} Petition;	

typedef struct 
{
	int		domainContNbr;
	int		domainUnitNbr;
	int		domainRoleNbr;
	int		subjectNbr;
} InvitationSpec;

typedef struct
{
	InvitationSpec	*inviteSpecification;
	Lyst		moduleSet;	/*	Lyst of (Module *)	*/
} Invitation;

/*	 Envelope control code list.					*/
typedef enum 
{
	InitialDeclaration = 1,
	PetitionAssertion = 2,
	PetitionCancellation = 3,
	PublishOnReception = 4,
	SendOnReception = 5,
	AnnounceOnReception = 6
} EnvelopeControlCode;

/*	Fields that can be extracted from envelope.			*/
typedef enum 
{
	Env_ControlCode = 1,
	Env_ContinuumNbr = 2,
	Env_PublishUnitNbr = 3,		/* unit				*/
	Env_PublishRoleNbr = 4,		/* sourceID field cc=2&3	*/
	Env_SourceRoleNbr = 5,		/* sourceID field cc=4&5&6	*/
	Env_DestUnitNbr = 6,		/* unit 			*/
	Env_DestModuleNbr = 7,		/* destID field cc=5		*/
	Env_DestRoleNbr = 8,		/* destID field cc=6		*/
	Env_SubjectNbr = 9,
	Env_EnclosureLength = 10,
	Env_UnitField = 11,
	Env_SourceIDField = 12,
	Env_DestIDField = 13
} EnvelopeField;

/*	Fields that can be extracted from enclosure.			*/
typedef enum
{
	Enc_ContinuumNbr = 1,
	Enc_UnitNbr = 2,
	Enc_ModuleNbr = 3,
	Enc_SubjectNbr = 4,
	Enc_ChecksumFlag = 5
} EnclosureField;

extern int	rams_run(char *mibSource, char *tsorder, char *applicationName,
			char *authorityName, char *unitName, char *roleName,
			long lifetime);
#ifdef __cplusplus
}
#endif

#endif	/* _RAMS_H */
