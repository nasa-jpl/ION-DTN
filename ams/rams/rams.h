/*
rams.h: private definition supporting the implementation of 
Remote Asynchronous Message Service 

Author: Shin-Ywan (Cindy) Wang

Copyright (c) 2005, California Institute of Technology.
ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
*/

#ifndef  _RAMS_H
#define _RAMS_H

#include <ams.h>
#include <amscommon.h>
#include "platform.h"
#include "memmgr.h"
#include "psm.h"
#include "lyst.h"
#include "llcv.h"

#ifdef _cplusplus
extern "C" {
#endif

#define ENVELOPELENGTH 12
#define AMSMSGHEADER 16

#define	ErrMsg(text)	putErrmsg(text, NULL)

typedef enum 
{
	AMSNODE = 1, 
	RAMSNODE = 2
} AmsModuleType; 

typedef enum
{
	MESHTYPE = 1,
	TREETYPE = 2
} RAMSNetworkType;

/*	Management information base, which mainly contains the MIB
 *	of the relevant AMS venture plus RAMS neighbor information.	*/
typedef  struct
{
	AmsMib		*amsMib;
	Lyst		ramsNeighbors;	/*	expected neighbors	*/
					/*	formerly ramsGrids	*/
	int		neighborsCount;	/*	formerly gridSize	*/
	Lyst		declaredNeighbors; /*	declared neighbors	*/
					/*	formerly declaredRams	*/
	int		totalDeclared;
	RAMSNetworkType	netType;	/*	network type		*/
} RamsMib;

/*	RamsGateway is the module's RAMS Gateway operational state.	*/
typedef  struct ramsgateway
{
	AmsModule	amsModule;	/* gateway's AMS module state	*/

	Lyst		petitionSet;	/* list of (Petition *)		*/ 
	Lyst		invitationSet;	/* list of (Invitation *)	*/
	Lyst		registerSet;	/* list of registered modules	*/
	RamsMib		*ramsMib;
	
	pthread_t	primeThread;   
	pthread_t	petitionReceiveThread;
	pthread_t	amsReceiveThread;
} RamsGateway, *RamsGate;		/* RamsGate: formerly ramsNode	*/

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

/*	RamsNode structure identifies a neighbor in the RAMS network	*/
typedef struct
{
/*	int		moduleNbr;	//	Held continuum number	*/
/*	int		elementNbr;	// continuumID			*/
/*	int		serviceNbr;	// ipn:element#.service#	*/
	int		continuumNbr;
	RamsNetProtocol	protocol;
	char		*gwEid;
} RamsNode;				/*	Formerly BPEndPoint	*/

typedef struct
{
	char		*enclosureContent;
	int		enclosureHeaderLength;
	int		contentLength;
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
} EnvelopeContent;

/*	Fields that can be extracted from enclosure.			*/
typedef enum
{
	Enc_ContinuumNbr = 1,
	Enc_UnitNbr = 2,
	Enc_ModuleNbr = 3,
	Enc_SubjectNbr = 4,
	Enc_ChecksumFlag = 5
} EnclosureContent;

extern int rams_register(char *mibSource, char *tsorder, char *mName,
		char *memory, unsigned mSize, char *applicationName,
		char *authorityName, char *unitName, char *roleName,
		RamsGate *gwayPtr, int lifetime);
extern int rams_unregister(RamsGate *gwayPtr);

#ifdef __cplusplus
}
#endif

#endif	/* _RAMS_H */
