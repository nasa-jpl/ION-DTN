/*
ramscommon.h: private definition supporting the implementation of 
Remote Asynchronous Message Service 

Author: Shin-Ywan (Cindy) Wang

Copyright (c) 2005, California Institute of Technology.
ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
*/

#ifndef  _RAMSCOMMON_H
#define _RAMSCOMMON_H

#ifdef _cplusplus
extern "C" {
#endif

#include <amsP.h>

extern RamsNode		*Look_Up_Neighbor(RamsGateway *gWay,
				char *gwEid);
extern RamsNode		*Look_Up_DeclaredNeighbor(RamsGateway *gWay,
				int ramsNbr);

extern void		ConstructEnvelope(unsigned char *envelope,
				int continuumNbr, int unitNbr, int moduleNbr,
				int roleNbr, int subjectNbr,
				int enclosureHdrLength, char *enclosureHdr,
				int enclosureContentLength,
				char *enclosureContent, int controlCode);
extern int		ConstructEnvelopeToNeighbor(RamsGateway *gWay,
				unsigned char flowLabel, Enclosure *enclosure,
				int toContinuum, int toUnit, int toRole,
				int toModule, int cCode, int sNbr,
				int neighborContinuumNbr);
extern int		ConstructEnvelopeToDeclaredNeighbor(RamsGateway *gWay,
				Enclosure *enclosure, int toContinuum,
				int toUnit, int toRole, int toModule,
				int cCode, int sNbr);
extern int		EnvelopeHeader(char *envelope, EnvelopeContent conId);
extern char		*EnvelopeText(char *envelope, int contentLength);
extern void		DeleteEnvelope(Envelope *env);
extern int		SendEnvelopeToNeighbor(RamsGateway *gWay,
				char *envelope, int envelopeSize,
				int toContinuum);
extern void		GetEnvelopeSpecification(char* env, int *continuumNbr,
				int *unitNbr, int *roleNbr);

extern Enclosure	*ConstructEnclosure(int continuumNbr, int unitNbr,
				int moduleNbr, int subjectNbr,
				int contentLength, char* content, int context,
				AmsMsgType msgType, int priority,
				unsigned char flowLabel);
extern int		EnclosureHeader(char *enc, EnclosureContent encId);
extern void		DeleteEnclosure(Enclosure *enc);

extern Petition		*ConstructPetition(int domainContinuum, int domainRole,
				int domainUnit, int subNbr, int cCode);
extern Petition		*ConstructPetitionFromEnvelope(char* envelope);
extern void		GetPetitionSpecification(Petition *pet,
				int* continuumNbr, int* unitNbr, int* roleNbr);

extern int		RoleNumber(AmsModule module, int unitNbr,
				int moduleNbr);
extern int		SamePetition(Petition *pet1, Petition *pet2);
extern int		PetitionMatchesDomain(Petition *pet,
				int domainContinuum, int domainRole,
				int domainUnit, int subNbr);
extern int		EnclosureSatisfiesPetition(AmsModule module,
				char *rpdu, Petition *pet);
extern int		EnclosureSatisfiesInvitation(RamsGateway *gWay,
				char* rpdu, Invitation *inv);
extern int		MessageSatisfiesPetition(AmsModule module, int msgCont,
				int msgUnit, int msgModule, int subjectNbr,
				Petition *pet);

extern LystElt		ModuleSetMember(Module *module, Lyst lyst);
extern LystElt		InvitationSetMember(int dUnit, int dRole, int dCont,
				int sub, Lyst lyst);
extern LystElt		NodeSetMember(RamsNode *fromGWay, Lyst lyst);

extern int		ModuleIsMyself(Module* sModule, RamsGateway *gWay);
extern Module		*LookupModule(int unitNbr, int moduleNbr,
				RamsGateway *gWay);
extern void		SubtractNodeSets(Lyst set1, Lyst set2);
extern void		AddNodeSets(Lyst set1, Lyst set2);
extern RamsNode		*GetConduitForContinuum(int cId, RamsGateway *gWay);
extern Lyst		PropagationSet(RamsGateway *gWay, Petition *pet);

extern int		PetitionIsAssertable(RamsGateway *gWay, Petition *pet);
extern int		PetitionIsCancellable(Petition *pet);
extern void		DeletePetition(Petition *pet);

extern int		MessageIsInvited(RamsGateway *gWay, char* msg);
extern int		ModuleIsInAnnouncementDomain(RamsGateway* gWay,
				Module *module, int dUnit, int dRole);

#ifdef __cplusplus
}
#endif

#endif
