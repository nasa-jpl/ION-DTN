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
} AmsNodeType; 

typedef enum
{
	MESHTYPE = 1,
	TREETYPE = 2
} RAMSNetworkType;

// message information base which mainly contains the MIB of the
// the relevant AMS plus RAMS grids information
typedef  struct
{
	AmsMib *amsMib;
	Lyst   ramsNeighbors;     // expected neighbors, formerly ramsGrids
	Lyst   declaredNeighbors; // declared neighbors, formerly declaredRams
	int    neighborsCount;	  // formerly gridSize
	int    totalDeclared;
	RAMSNetworkType netType;  // network type
}RamsMib;
	
typedef  struct ramsgateway
{
	AmsNode amsNode;               // the base AMSNode
	
	Lyst   petitionSet;           // list of petition; 
	Lyst   invitationSet;         // list of Invitation;
	Lyst   registerSet;           // list of registered node, set of Node*
	RamsMib *ramsMib;             // MIB manage information base
	
	pthread_t primeThread;   
	pthread_t petitionReceiveThread;
	pthread_t amsReceiveThread;
} RamsGateway, *RamsGate;	// RamsGate was formerly ramsNode

// Envelope content
typedef struct
{
	int controlCode;
	int continuumNbr;
	int unitNbr;
	int nodeNbr;
	int roleNbr;
	int subjectNbr;
	int enclosureLength;
	char* enclosure;
} Envelope;

// Rams gateway address, continuum ID, unit no., node no.
typedef struct
{
//	int elementNbr;  // continuumID
//	int serviceNbr;  // ipn:element#.service#
	int		continuumNbr;
	RamsNetProtocol	protocol;
	char		*gwEid;
//	int		nodeNbr;	//	Held continuum number
} RamsNode;				//	Formerly BPEndPoint

// enclosure including header and content
typedef struct
{
	char* enclosureContent;
	int enclosureHeaderLength;
	int contentLength;
} Enclosure;


typedef struct
{
    char* envelope;
	int envelopeLength;
	int toContinuumNbr;
} PetitionContent;

// petition assertion
typedef struct
{
	PetitionContent *specification;

	Lyst DistributionNodeSet;           // 
	Lyst DestinationRamsSet;
	Lyst SourceRamsSet;
} Petition;	

typedef struct 
{
	int domainContNbr;
	int domainUnitNbr;
	int domainRoleNbr;
	int subjectNbr;
} InvitationContent;

typedef struct
{
	InvitationContent* inviteSpecification;
	Lyst nodeSet;    // set of Node*
} Invitation;


// envelope control code list
typedef enum 
{
	InitialDeclaration = 1,
	PetitionAssertion = 2,
	PetitionCancellation = 3,
	PublishOnReception = 4,
	SendOnReception = 5,
	AnnounceOnReception = 6
} EnvelopeControlCode;


typedef enum 
{
	Env_ControlCode = 1,
	Env_ContinuumNbr = 2,
	Env_PublishUnitNbr = 3,      // unit
	Env_PublishRoleNbr = 4,      // sourceID field cc=2&3
	Env_SourceRoleNbr = 5,		 // sourceID field cc=4&5&6
	Env_DestUnitNbr = 6,         // unit 
	Env_DestNodeNbr = 7,		 // destID field cc=5
	Env_DestRoleNbr = 8,         // destID field cc=6
	Env_SubjectNbr = 9,
	Env_EnclosureLength = 10,
	Env_UnitField = 11,
	Env_SourceIDField = 12,
	Env_DestIDField = 13
} EnvelopeContent;

typedef enum
{
	Enc_ContinuumNbr = 1,
	Enc_UnitNbr = 2,
	Enc_NodeNbr = 3,
	Enc_SubjectNbr = 4,
	Enc_ChecksumFlag = 5
} EnclosureContent;


//	typedef struct ramsgateway	*ramsNode;
extern int rams_register(char *mibSource, char *tsorder, char *mName,
		char *memory, unsigned mSize, char *applicationName,
		char *authorityName, char *unitName, char *roleName,
		RamsGate *gNode, int lifetime);
extern int rams_unregister(RamsGate *gNode);

#ifdef __cplusplus
}
#endif

#endif	/* _RAMS_H */
