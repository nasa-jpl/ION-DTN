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
extern void	constructEnvelope(unsigned char *envelope, int continuumNbr,
			int unitNbr, int nodeNbr, int roleNbr, int subjectNbr,
			int enclosureHdrLength, char *enclosureHdr,
			int enclosureContentLength, char *enclosureContent,
			int controlCode);
extern int EnclosureHeader(char *enc, EnclosureContent encId);
extern int EnvelopeHeader(char *envelope, EnvelopeContent conId);
extern char* EnvelopeText(char *envelope, int contentLength);

//extern RamsNode* Look_Up_RemoteRAMS(RamsGateway *gWay, int remoteRamsNbr);
extern RamsNode* Look_Up_NeighborRemoteRAMS(RamsGateway *gWay, char *gwEid);
extern RamsNode* Look_Up_DeclaredRemoteRAMS(RamsGateway *gWay, int ramsNbr);
extern void GetEnvelopeSpecification(char* env, int* continuumNbr, int* unitNbr, int* roleNbr);
extern void GetPetitionSpecification(Petition *pet, int* continuumNbr, int* unitNbr, int* roleNbr);
extern int EnvelopeToRAMS(RamsGateway *gWay, char *envelope, int envelopeSize, int toContinuum);
extern int ConstructEnvelopeToRAMS(RamsGateway *gWay, unsigned char flowLabel, Enclosure *enclosure,
							int toContinuum, int toUnit, int toRole, int toNode, int cCode, int sNbr, int toRAMS);
extern int ConstructEnvelopeToDeclaredRAMS(RamsGateway *gWay, Enclosure *enclosure, 
							int toContinuum, int toUnit, int toRole, int toNode, int cCode, int sNbr);
extern Enclosure* ConstructEnclosure(int continuumNbr, int unitNbr, int nodeNbr, int subjectNbr, int contentLength, char* content, int context,
							   AmsMsgType msgType, int priority, unsigned char flowLabel);
extern void DeleteEnclosure(Enclosure *enc);
extern Petition* ConstructPetition(int domainContinuum, int doaminRole, int domainUnit, int subNbr, int cCode);
extern Petition* ConstructPetitionFromEnvelope(char* envelope);
extern void DeleteEnvelope(Envelope *env);

extern int IsPetitionPublisherIdentical(Petition *pet1, Petition *pet2);
extern int IsSubscriptionPublisherIdentical(Petition *pet, int domainContinuum, int domainRole, int domainUnit, int subNbr);
extern int IsRAMSPDUSatisfyPetition(AmsNode node, char *msg, Petition *pet);
extern int IsRAMSPDUSatisfyInvitation(RamsGateway *gWay, char* msg, Invitation *inv);

extern int IsAmsMsgSatisfyPetition(AmsNode node, int msgCon, int msgUnit, int msgNode, int subjectNbr, Petition*  pet);
extern int RoleNumber(AmsNode node, int unitNbr, int nodeNbr);
extern LystElt IsNodeExist(Node *node, Lyst *lyst);
extern int IsSameRamsGateway(RamsNode *gWay1, RamsNode *gWay2);
extern LystElt IsContinuumExist(RamsNode *fromGWay, Lyst *lyst);
extern LystElt IsInvitationExist(int dUnit, int dRole, int dCnt, int sub, Lyst *lyst);

extern LystElt IsPrivateReceiverExist(RamsGateway *gWay, char* msg);
extern int IsValidAnnounceReceiver(RamsGateway* gWay, Node *node, int dUnit, int dRole);

extern int IsSameNode(Node* sNode, RamsGateway *gWay);
extern int IsSameAMSNode(AmsNode node1, AmsNode node2);
extern Node* GetSourceNode(int unitNbr, int nodeNbr, RamsGateway *gWay);
//extern RamsNode* FindGateway(RamsGateway *gWay, char *sourceId);
extern RamsNode* FindNeighborGateway(RamsGateway *gWay, char *sourceId);
extern RamsNode* FindDeclaredGateway(RamsGateway *gWay, char *sourceId);

extern void RAMSSetSubtraction(Lyst set1, Lyst set2);
extern void RAMSSetUnion(Lyst set1, Lyst set2);
extern RamsNode* GetConduitForContinuum(int cId, RamsGateway *gWay);
extern Lyst PropagationSet(RamsGateway *gWay, Petition *pet);
extern int IsPetitionAssertable(RamsGateway *gWay, Petition *pet);
extern int IsToCancelPetition(Petition *pet);
extern void DeleteSourceRamsSet(Petition *pet);
extern void DeleteCollectionOrders(Petition *pet);
extern void DeleteDistributionOrders(Petition *pet);
extern void DeletePetition(Petition *pet);
//extern void ErrMsg(char *err);

//extern Petition* CopyPetition(Petition *pet1);




#ifdef __cplusplus
}
#endif

#endif

