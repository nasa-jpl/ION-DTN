/*
ramscommon.c: functions enabling the implementation of RAMS gateway based applications

Author: Shin-Ywan (Cindy) Wang
Copyright (c) 2005, California Institute of Technology.
ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
*/

#include "rams.h"
#include <ams.h>
#include <amsP.h>


//extern char *errMessage;
extern int SendMessageToContinuum(RamsGateway *gWay, int continuumId, unsigned char flowLabel, char* envelope, int envelopeLength);
extern int SendPetitionToDeclaredRAMS(RamsGateway *gWay, char *env);

#if 0
void ErrMsg(char *err)
{
	putErrmsg(err, NULL);
}
#endif

void	constructEnvelope(unsigned char *envelope, int continuumNbr,
			int unitNbr, int sourceID, int destID, int subjectNbr,
			int enclosureHdrLength, char *enclosureHdr,
			int enclosureContentLength, char *enclosureContent,
			int controlCode)
{
	short		i2;
	unsigned short	enclosureLength;

	envelope[0] = controlCode & 0x0000000f;
	envelope[1] = 0;	/*	Reserved.			*/
	envelope[2] = (continuumNbr >> 8) & 0x0000007f;
	envelope[3] = continuumNbr & 0x000000ff;
	envelope[4] = (unitNbr >> 8) & 0x000000ff;
	envelope[5] = unitNbr & 0x000000ff;
	envelope[6] = sourceID;
	envelope[7] = destID;
	i2 = subjectNbr;
	i2 = htons(i2);     // possible to be negative
	memcpy(envelope + 8, (unsigned char *) &i2, 2);
	enclosureLength = enclosureHdrLength + enclosureContentLength;
	envelope[10] = (enclosureLength >> 8) & 0x00ff;
	envelope[11] = enclosureLength & 0x00ff;
	if (enclosureLength > 0)
	{
		if (enclosureHdrLength > 0)
		   memcpy(envelope + 12, enclosureHdr, enclosureHdrLength);
		memcpy(envelope + 12 + enclosureHdrLength, enclosureContent,
				enclosureContentLength);
	}
}

int EnclosureHeader(unsigned char *enc, EnclosureContent encId)
{
	int num = 0;		/*	Initialized to avoid warning.	*/
	short subj;

	switch(encId)
	{
	case Enc_ChecksumFlag:
		num = enc[2] & 0x00000080;
		break;
	case Enc_ContinuumNbr:
		num = ((enc[2] << 8) & 0x00007f00) + enc[3];
		break;
	case Enc_UnitNbr:
		num = ((enc[4] << 8) & 0x0000ff00) + enc[5];
		break;
	case Enc_NodeNbr:
		num = enc[6];
		break;
	case Enc_SubjectNbr:
		memcpy((char *) & subj, enc + 12, 2);
		subj = ntohs(subj);
		num = subj;
		break;
	default:
		return 0;
	}
	return num;
}

int EnvelopeHeader(unsigned char *envelope, EnvelopeContent conId)
{
	int num = 0;		/*	Initialized to avoid warning.	*/
	short subj;

	switch (conId)
	{
	case Env_ControlCode: 
		num = envelope[0] & 0x0f;
		break;
	case Env_ContinuumNbr:
		num = ((envelope[2] << 8) & 0x00007f00) + envelope[3];
		break;
	case Env_PublishUnitNbr:
	case Env_DestUnitNbr:
	case Env_UnitField:
		num = ((envelope[4] << 8) & 0x0000ff00) + envelope[5];
		break;
	case Env_PublishRoleNbr:
	case Env_SourceRoleNbr:
	case Env_SourceIDField:
		num = envelope[6];
		break;
	case Env_DestNodeNbr:
	case Env_DestRoleNbr:
	case Env_DestIDField:
		num = envelope[7];
		break;
	case Env_SubjectNbr:
		memcpy((char *)&subj, envelope+8, 2);
		subj = ntohs(subj);
		num = subj;
		break;
	case Env_EnclosureLength:
		num = ((envelope[10] << 8) & 0x0000ff00) + (envelope[11]&0x000000ff);
		break;
	}
	return num; 
}

// contentLength = -1: unknown 
// contentLength >= 0 knownsize
char* EnvelopeText(unsigned char *envelope, int contentLength)
{
	if (contentLength < 0)
	{
		contentLength = ((envelope[10] << 8)&0x0000ff00) + (envelope[11]&0x000000ff);
	}

	if (contentLength > 0)
		return envelope+ENVELOPELENGTH;
	else
		return NULL;
}

// find the RAMS gateway from MIB rams grid
RamsNode* Look_Up_NeighborRemoteRAMS(RamsGateway *gWay, char *gwEid)
{
	LystElt elt;
	RamsNode *ramsNode;

	for (elt = lyst_first(gWay->ramsMib->ramsNeighbors); elt != NULL;
			elt = lyst_next(elt))
	{
		
		ramsNode = (RamsNode *)lyst_data(elt);
		if (strcmp(ramsNode->gwEid, gwEid) == 0)
		{
			return ramsNode;
		}
	}

	return NULL;
}

RamsNode* Look_Up_DeclaredRemoteRAMS(RamsGateway *gWay, int ramsNbr)
{
	LystElt elt;
	RamsNode *bpPoint;


	for (elt = lyst_first(gWay->ramsMib->declaredNeighbors); elt != NULL; elt = lyst_next(elt))
	{
		
		bpPoint = (RamsNode *)lyst_data(elt);
		if (bpPoint->continuumNbr == ramsNbr)
			return bpPoint;
	}
    return NULL;
}

void GetEnvelopeSpecification(unsigned char* env, int* continuumNbr, int* unitNbr, int* roleNbr)
{
	*continuumNbr = ((env[2] << 8) & 0x00007f00) + env[3];
	*unitNbr = ((env[4] << 8) & 0x0000ff00) + env[5];
	*roleNbr = env[7];
}

void GetPetitionSpecification(PetitionContent *pet, int* continuumNbr, int* unitNbr, int* roleNbr)
{

	GetEnvelopeSpecification(pet->envelope, continuumNbr, unitNbr, roleNbr);
}

int EnvelopeToRAMS(RamsGateway *gWay, unsigned char flowLabel, char *envelope, int envelopeSize, int toContinuum)
{
	if (!SendMessageToContinuum(gWay, toContinuum, flowLabel, envelope, envelopeSize))
	{
		ErrMsg("error in construct envelope sent to RAMS");
		return 0;
	}
	return 1; 
}

int ConstructEnvelopeToDeclaredRAMS(RamsGateway *gWay, Enclosure *enclosure, 
	int toContinuum, int toUnit, int sourceID, int destID, int cCode, int sNbr)
{
	char* envelope;
	int subjectNbr;
	int succeed; 

	envelope = NULL; 
	if (enclosure != NULL)
	{
		envelope = (char *)MTAKE(ENVELOPELENGTH + enclosure->contentLength);
		CHKZERO(envelope);
		subjectNbr = EnclosureHeader(enclosure->enclosureContent, Enc_SubjectNbr);
		constructEnvelope((unsigned char *) envelope, toContinuum, toUnit, sourceID, destID, subjectNbr,0, NULL, enclosure->contentLength,
					enclosure->enclosureContent, cCode);
	}
	else 
	{
		envelope = (char *)MTAKE(ENVELOPELENGTH);
		CHKZERO(envelope);
		subjectNbr = sNbr;
		constructEnvelope((unsigned char *) envelope, toContinuum, toUnit, sourceID, destID, subjectNbr, 0, NULL, 0, NULL, cCode);
	}	

	succeed = SendPetitionToDeclaredRAMS(gWay, envelope);
	MRELEASE(envelope);
	return succeed; 
}

int ConstructEnvelopeToRAMS(RamsGateway *gWay, unsigned char flowLabel, Enclosure *enclosure, 
							int toContinuum, int toUnit,  int sourceID, int destID, int cCode, 
							int sNbr, int toRAMS)
{
	char* envelope;
	//int subjectNbr;
	int encLength;
	int succeed; 

	envelope = NULL; 
	if (enclosure != NULL)
	{
		envelope = (char *)MTAKE(ENVELOPELENGTH + enclosure->contentLength);
		CHKZERO(envelope);
		//subjectNbr = EnclosureHeader( enclosure->enclosureContent, Enc_SubjectNbr);
		constructEnvelope((unsigned char *) envelope, toContinuum, toUnit, sourceID, destID, sNbr, 0, NULL, enclosure->contentLength,
						  enclosure->enclosureContent, cCode);
		encLength = enclosure->contentLength;
	}
	else 
	{
		envelope = (char *)MTAKE(ENVELOPELENGTH);
		CHKZERO(envelope);
		//subjectNbr = sNbr;
		constructEnvelope((unsigned char *) envelope, toContinuum, toUnit, sourceID, destID, sNbr, 0, NULL, 0, NULL, cCode);
	    encLength = 0; 
	}	

	succeed = EnvelopeToRAMS(gWay, flowLabel, envelope, ENVELOPELENGTH+encLength, toRAMS);
	MRELEASE(envelope);
	return succeed; 

}

// construct ams msg (enclosure: source node info + content)
Enclosure* ConstructEnclosure(int continuumNbr, int unitNbr, int nodeNbr, int subjectNbr, int contentLength, char* content, int context,
		AmsMsgType msgType, int priority, unsigned char flowLabel)
{
	Enclosure* enc;
	char* header;
	unsigned long u8;
	unsigned char u1;
	short i2;

	enc = (Enclosure *)MTAKE(sizeof(Enclosure));
	enc->contentLength = contentLength + AMSMSGHEADER;
	enc->enclosureHeaderLength = AMSMSGHEADER;	
	enc->enclosureContent = NULL; 
	enc->enclosureContent = (char *)MTAKE(enc->contentLength);
	
	header = enc->enclosureContent;

	u8 = msgType;
	u1 = ((u8 << 4) & 0x30) + (priority & 0x0f);
	*header = u1;
	*(header + 1) = flowLabel;
	/*	First bit of 3rd octet is checksum flag, always zero.	*/
	*(header + 2) = (continuumNbr >> 8) & 0x0000007f;
	*(header + 3) = continuumNbr & 0x000000ff;
	*(header + 4) = (unitNbr>> 8) & 0x000000ff;
	*(header + 5) = unitNbr & 0x000000ff;
	*(header + 6) = nodeNbr & 0x000000ff;
	*(header + 7) = 0;	/*	Reserved.			*/
	context = htonl(context);
	memcpy(header + 8, (char *) &context, 4);
	i2 = subjectNbr;
	i2 = htons(i2);
	memcpy(header + 12, (char *) &i2, 2);
	*(header + 14) = (contentLength >> 8) & 0x000000ff;
	*(header + 15) = (contentLength) & 0x000000ff;

	if (contentLength > 0)
	{
		memcpy(enc->enclosureContent+AMSMSGHEADER, content, contentLength);
	}
	
	return enc;
}

void DeleteEnclosure(Enclosure *enc)
{
	if (enc->enclosureContent)
		MRELEASE((char *)enc->enclosureContent);
	MRELEASE(enc);
}

Petition* ConstructPetition(int domainContinuum, int domainRole, int domainUnit, int subNbr, int cCode)
{
	Petition *pet;

	pet = MTAKE(sizeof(Petition));
	pet->DistributionNodeSet = lyst_create();
	pet->DestinationRamsSet = lyst_create();
	pet->SourceRamsSet = lyst_create();
	
	pet->specification = (PetitionContent *)MTAKE(sizeof(PetitionContent));
	pet->specification->envelope = (char *)MTAKE(ENVELOPELENGTH);
	pet->specification->envelopeLength = ENVELOPELENGTH;
	pet->specification->toContinuumNbr = domainContinuum;

	constructEnvelope((unsigned char *) (pet->specification->envelope), domainContinuum, domainUnit, domainRole, 0, subNbr, 0, NULL, 0, NULL, cCode);
	return pet;
}

Petition* ConstructPetitionFromEnvelope(char* envelope)
{

	Petition *pet;

	pet = MTAKE(sizeof(Petition));
	pet->DistributionNodeSet = lyst_create();
	pet->DestinationRamsSet = lyst_create();
	pet->SourceRamsSet = lyst_create();
	pet->specification = (PetitionContent *)MTAKE(sizeof(PetitionContent));
	pet->specification->envelope = NULL;
	pet->specification->envelopeLength = 0;
	pet->specification->toContinuumNbr = -1; 
	if (envelope != NULL)
	{
		pet->specification->envelopeLength = ENVELOPELENGTH + EnvelopeHeader(envelope, Env_EnclosureLength);
		pet->specification->toContinuumNbr = EnvelopeHeader(envelope, Env_ContinuumNbr);
		pet->specification->envelope = (char *)MTAKE(pet->specification->envelopeLength);
		memcpy(pet->specification->envelope, envelope, pet->specification->envelopeLength); // envelope space will be reused for next message
	}
	return pet;
}

void DeleteEnvelope(Envelope *env)
{
	if (env->enclosure)
	   MRELEASE(env->enclosure);
	MRELEASE(env);
}

int IsPetitionPublisherIdentical(Petition *pet1, Petition *pet2)
{
	if ((EnvelopeHeader(pet1->specification->envelope, Env_ContinuumNbr) == EnvelopeHeader(pet2->specification->envelope, Env_ContinuumNbr))&&
		(EnvelopeHeader(pet1->specification->envelope, Env_PublishUnitNbr) == EnvelopeHeader(pet2->specification->envelope, Env_PublishUnitNbr)) &&
		(EnvelopeHeader(pet1->specification->envelope, Env_PublishRoleNbr) == EnvelopeHeader(pet2->specification->envelope, Env_PublishRoleNbr)) &&
		(EnvelopeHeader(pet1->specification->envelope, Env_SubjectNbr) == EnvelopeHeader(pet2->specification->envelope, Env_SubjectNbr)))
	{
	
		return 1;
	}
	else
	{
		return 0;
	}
}

int IsSubscriptionPublisherIdentical(Petition *pet, int domainContinuum, int domainRole, int domainUnit, int subNbr)
{
	if ((EnvelopeHeader(pet->specification->envelope, Env_ContinuumNbr) == domainContinuum)&&
		(EnvelopeHeader(pet->specification->envelope, Env_PublishUnitNbr) == domainUnit) &&
		(EnvelopeHeader(pet->specification->envelope, Env_PublishRoleNbr) == domainRole) &&
		(EnvelopeHeader(pet->specification->envelope, Env_SubjectNbr) == subNbr))
	{	
		return 1;
	}
	else
	{
		return 0;
	}
}

// petition subsume source node in msg (envelope+enclosure, enclosure: source node +msg)

int IsRAMSPDUSatisfyPetition(AmsNode node, char *msg, Petition *pet)
{
	char* enc;
	int encCon, petCon;
	int encRole, petRole;

	enc = EnvelopeText(msg, -1);
	if (enc == NULL)
		return 0;

	if (EnclosureHeader(enc, Enc_SubjectNbr) != EnvelopeHeader(pet->specification->envelope, Env_SubjectNbr))
		return 0;

	encCon = EnclosureHeader(enc, Enc_ContinuumNbr);
	petCon = EnvelopeHeader(pet->specification->envelope, Env_ContinuumNbr);
	
	// check continuum ID
	if (petCon != encCon && petCon != 0)
       return 0;

	// check if unit is satisfied, since all unit structure is the same in all continuum, so
	// using local node to check the unit of publisher from counterpart continuum
	if (EnvelopeHeader(pet->specification->envelope, Env_PublishUnitNbr) != 0)
	{
		if (!ams_subunit_of(node, EnclosureHeader(enc, Enc_UnitNbr), EnvelopeHeader(pet->specification->envelope, Env_PublishUnitNbr)))
			return 0;
	}

	// check the role ID, msg roleNbr field means the source node role.
	encRole = EnvelopeHeader(msg, Env_PublishRoleNbr);
	petRole = EnvelopeHeader(pet->specification->envelope, Env_PublishRoleNbr);
	if (encRole != petRole && petRole != 0)
		return 0;
	return 1;
	// check unit number
}

int IsRAMSPDUSatisfyInvitation(RamsGateway *gWay, char* msg, Invitation *inv)
{
	char *enc;
	int srcUnit, srcRole, srcCon;

	enc = EnvelopeText(msg, -1);
	if (enc == NULL)
		return 0;

	if (inv->inviteSpecification->subjectNbr != 0 && EnclosureHeader(enc, Enc_SubjectNbr) != inv->inviteSpecification->subjectNbr)
		return 0;

	srcCon = EnclosureHeader(enc, Enc_ContinuumNbr);

	if (inv->inviteSpecification->domainContNbr != 0 && 
		inv->inviteSpecification->domainContNbr != srcCon)
		return 0;

	srcUnit = EnclosureHeader(enc, Enc_UnitNbr);		// source unit set in enclosure
	srcRole = EnvelopeHeader(msg, Env_SourceRoleNbr);  // source role set in envelope

	if (inv->inviteSpecification->domainUnitNbr != 0)
	{
		if (!ams_subunit_of(gWay->amsNode, srcUnit, inv->inviteSpecification->domainUnitNbr))
			return 0;
	}

	if (inv->inviteSpecification->domainRoleNbr != 0 && srcRole != inv->inviteSpecification->domainRoleNbr)
	{
		return 0;
	}
	return 1; 
}

int RoleNumber(AmsNode node, int unitNbr, int nodeNbr)
{
	char *msgRoleName;
	int msgRole;

	msgRoleName = ams_get_role_name(node, unitNbr, nodeNbr);
	if (msgRoleName == NULL)
		return -1; 
   	msgRole = ams_lookup_role_nbr(node, msgRoleName);
	return msgRole;
}

int IsAmsMsgSatisfyPetition(AmsNode node, int msgCon, int msgUnit, int msgNode, int subjectNbr, Petition* pet)
{
	int petCon;
	int msgRole, petRole;

	if (subjectNbr != EnvelopeHeader(pet->specification->envelope, Env_SubjectNbr))
		return 0;

	petCon = EnvelopeHeader(pet->specification->envelope, Env_ContinuumNbr);
	// check continuum ID
	if (petCon != msgCon && petCon != 0)
       return 0;

	// check if unit is satisfied	
	if (EnvelopeHeader(pet->specification->envelope, Env_PublishUnitNbr) != 0)
	{	
		if (!ams_subunit_of(node, msgUnit, EnvelopeHeader(pet->specification->envelope, Env_PublishUnitNbr)))
			return 0;
	}

	// check the role ID

	msgRole = RoleNumber(node, msgUnit, msgNode);
	petRole = EnvelopeHeader(pet->specification->envelope, Env_PublishRoleNbr);
	if (msgRole != petRole && petRole != 0)
		return 0;
	return 1;
}

// check if the node exist int the distribution order destination node
LystElt IsNodeExist(Node *node, Lyst *lyst)
{
	LystElt elt;
	Node *nodeExist;
	// check node no and unit no. 
	for (elt = lyst_first(*lyst); elt != NULL; elt = lyst_next(elt))
	{
		nodeExist = (Node *)lyst_data(elt);
		if (node->unitNbr == nodeExist->unitNbr &&
		//	node->role->nbr == nodeExist->role->nbr &&
			node->nbr == nodeExist->nbr)
        return elt;
	}
	return NULL;
}

LystElt IsInvitationExist(int dUnit, int dRole, int dCont, int sub, Lyst *lyst)
{
	LystElt elt;
	Invitation *inv; 

	for (elt = lyst_first(*lyst); elt != NULL; elt = lyst_next(elt))
	{
		inv = (Invitation *)lyst_data(elt);
		if (inv->inviteSpecification->domainContNbr == dCont &&
			inv->inviteSpecification->domainUnitNbr == dUnit && 
			inv->inviteSpecification->domainRoleNbr == dRole && 
			inv->inviteSpecification->subjectNbr == sub)
		{
			return elt;
		}
	}
	return NULL;
}

int IsSameRamsGateway(RamsNode *gWay1, RamsNode *gWay2)
{
	/*
	if (gWay1->ramsMib->amsMib->localContinuumNbr == gWay2->ramsMib->amsMib->localContinuumNbr &&
		gWay1->amsNode->msgspace->nbr == gWay2->amsNode->msgspace->nbr &&
		gWay1->amsNode->unit->nbr = gWay2->amdNode->unit->nbr &&
		gWay1->amsNode->role->nbr = gWay2->amsNode->role->nbr &&
		gWay1->amdNode->nodeNbr == gWay2->amsNode->nodeNbr)
	*/
//	if (gWay1->elementNbr == gWay2->elementNbr &&
//		gWay1->serviceNbr == gWay2->serviceNbr &&
		//gWay1->unitNbr == gWay2->unitNbr &&
//		gWay1->nodeNbr == gWay2->nodeNbr)
	if (gWay1->continuumNbr == gWay2->continuumNbr
	&& gWay1->protocol == gWay2->protocol
	&& strcmp(gWay1->gwEid, gWay2->gwEid) == 0)
		return 1;
	else
		return 0;
}

// check if the continuum node exists in a continuum set 
LystElt IsContinuumExist(RamsNode *fromGWay, Lyst *lyst)
{
	LystElt elt;
	for (elt = lyst_first(*lyst); elt != NULL; elt = lyst_next(elt))
	{
		RamsNode *gateWay;
		gateWay = (RamsNode *)lyst_data(elt);
		if (IsSameRamsGateway(fromGWay, gateWay))
			return elt;
	}
	return NULL;
}

int IsSameNode(Node* sNode, RamsGateway *gWay)
{
	if (sNode->unitNbr == ams_get_unit_nbr(gWay->amsNode) &&
		sNode->nbr == ams_get_node_nbr(gWay->amsNode))
		return 1;
	else
		return 0;
}

int IsSameAMSNode(AmsNode node1, AmsNode node2)
{
	if (ams_get_unit_nbr(node1) == ams_get_unit_nbr(node2) &&
		ams_get_node_nbr(node1) == ams_get_node_nbr(node2))
		return 1;
	else
		return 0;
}

Node *GetSourceNode(int unitNbr, int nodeNbr, RamsGateway *gWay)
{
	Node *node;

	node = gWay->amsNode->venture->units[unitNbr]->cell->nodes[nodeNbr];
	return node;
}

/*
RamsNode* FindNeighborGateway(RamsGateway *gWay, char *sourceId)
{
	char *s1, *s2;
    char* sId;
    int srcCId; 

    s1 = strchr(sourceId, ':');
    sId = s1 + 1;
    s2 = strchr(sId, '.');
    *s2 = '\0';
    srcCId = atoi(sId);
    *s2 = '.';
    return Look_Up_NeighborRemoteRAMS(gWay, srcCId);
}
*/

RamsNode* FindDeclaredGateway(RamsGateway *gWay, char *sourceId)
{
	char *s1, *s2;
    char* sId;
    int srcCId; 

    s1 = strchr(sourceId, ':');
    sId = s1 + 1;
    s2 = strchr(sId, '.');
    *s2 = '\0';
    srcCId = atoi(sId);
    *s2 = '.';
    return Look_Up_DeclaredRemoteRAMS(gWay, srcCId);
}

// set exclusion
void RAMSSetSubtraction(Lyst set1, Lyst set2)
{
	LystElt elt, nextElt;
	RamsNode *rams;

	for (elt = lyst_first(set1); elt != NULL; )
	{
		rams = (RamsNode *)lyst_data(elt);
		nextElt = lyst_next(elt);
		if (IsContinuumExist(rams, &set2))
		{
			lyst_delete(elt);
		}
		elt = nextElt;
	}
}

// set union
void RAMSSetUnion(Lyst set1, Lyst set2)
{
	LystElt elt;
	RamsNode *rams;

	for (elt = lyst_first(set2); elt != NULL; elt = lyst_next(elt))
	{
		rams = (RamsNode *)lyst_data(elt);
		if (!IsContinuumExist(rams, &set1))
		{
			lyst_insert_last(set1, rams);
		}
	}
}

// == NULL, not exist
// != NULL, exist
RamsNode* GetConduitForContinuum(int cId, RamsGateway *gWay)
{
	LystElt elt, eltNode;
	Petition *pet; 
	int subN;
	RamsNode *rGWay; 

	for (elt = lyst_first(gWay->petitionSet); elt != NULL; elt = lyst_next(elt))
	{
		pet = (Petition *)lyst_data(elt);
		subN = EnvelopeHeader(pet->specification->envelope, Env_SubjectNbr);
		if (subN == -cId)
		{

			eltNode = lyst_first(pet->DestinationRamsSet);
			if (eltNode == NULL)
				continue;
			rGWay = (RamsNode *)lyst_data(eltNode);
			return rGWay; 
		}
	}
	return NULL;  // not exist 
}


Lyst PropagationSet(RamsGateway *gWay, Petition *pet)
{
	int domainCont;
	LystElt elt;
	Lyst    PS; 
	RamsNode *rams;

	PS = lyst_create();
	domainCont = EnvelopeHeader(pet->specification->envelope, Env_ContinuumNbr);

	// domain of peition subscription is "all continua"
	if (domainCont == 0)
	{
		for (elt = lyst_first(gWay->ramsMib->declaredNeighbors); elt != NULL; elt = lyst_next(elt))
		{
			rams = (RamsNode *)lyst_data(elt);
			lyst_insert_last(PS, rams);
		}
	}
	// domain of petition subscription is a single remote continuum 
	else   
	{ 
		// get conduit for domain continuum
		rams = GetConduitForContinuum(domainCont, gWay);
		if (rams != NULL)
			lyst_insert_last(PS, rams);
	}
		
	// remove from PS all members of the source gateway sets of this petition
	RAMSSetSubtraction(PS, pet->SourceRamsSet);
	
	// remove from the PS the sole member of the DGS if petition's DNS is empty 
	// and DGS continas only one member
	
	if (lyst_first(pet->DistributionNodeSet) == NULL &&
		lyst_length(pet->DestinationRamsSet) == 1)
	{
		RAMSSetSubtraction(PS, pet->DestinationRamsSet);
	}
	
	return PS;
}

int IsPetitionAssertable(RamsGateway *gWay, Petition *pet)
{
	if (gWay->ramsMib->netType == MESHTYPE)
	{
		if (lyst_first(pet->DistributionNodeSet) != NULL)
			return 1;
	}
	else if (gWay->ramsMib->netType == TREETYPE)
	{
		if (lyst_first(pet->DistributionNodeSet) != NULL ||
			lyst_first(pet->DestinationRamsSet) != NULL)
			return 1;
	}
	
	return 0;
}

// if cancel petition
// - DNS = empty
// - DGS = empty
//   or member of DGS == member of SGS

int IsToCancelPetition(Petition *pet)
{
	RamsNode *pet1;
	//RamsNode *pet2;

	if (lyst_first(pet->DistributionNodeSet) == NULL)
	{
		if (lyst_first(pet->DestinationRamsSet) == NULL)
			return 1;
		
		if (lyst_length(pet->DestinationRamsSet) == 1)
		{
			pet1 = (RamsNode *)lyst_data(lyst_first(pet->DestinationRamsSet));
			if (IsContinuumExist(pet1, &(pet->SourceRamsSet)) != NULL )
				return 1;
			else
				return 0;
		}
		
		/*
		if (lyst_length(pet->DestinationRamsSet) == 1 &&
			lyst_length(pet->SourceRamsSet) == 1)
		{
			pet1 = (RamsNode *)lyst_data(lyst_first(pet->DestinationRamsSet));
			pet2 = (RamsNode *)lyst_data(lyst_first(pet->SourceRamsSet));
			if (pet1->continuumNbr == pet2->continuumNbr)
				return 1;
		}
		*/
	}
	return 0;
}
	
void DeleteSourceRamsSet(Petition *pet)
{
	Lyst sourceSet;

	sourceSet = pet->SourceRamsSet;
	lyst_destroy(sourceSet);
}

void DeleteCollectionOrders(Petition *pet)
{
	Lyst cOrders;
		
	cOrders = pet->DestinationRamsSet;
    lyst_destroy(cOrders);
}

void DeleteDistributionOrders(Petition *pet)
{
	Lyst dOrders; 
	
	dOrders = pet->DistributionNodeSet;
	lyst_destroy(dOrders);
}

void DeletePetition(Petition *pet)
{
//printf("  petition(%d)\n", EnvelopeHeader(pet->specification->envelope, Env_SubjectNbr));
	DeleteSourceRamsSet(pet);
//PUTS("release SRS");
	DeleteCollectionOrders(pet);
//PUTS("release DRS");
	DeleteDistributionOrders(pet);
//PUTS("release DNS");
	MRELEASE(pet->specification->envelope);
	MRELEASE(pet->specification);
	MRELEASE(pet);
}

LystElt IsPrivateReceiverExist(RamsGateway *gWay, char* msg)
{
	int srcUnit, srcRole, srcSubject; 
	int destUnit, destNode; 
	char* enc;
	LystElt elt, nodeElt;
	Invitation *inv; 
	Node *sourceNode; 

	enc = EnvelopeText(msg, -1);
	if (enc == NULL)
		return NULL;
	
	srcUnit = EnclosureHeader(enc, Enc_UnitNbr);
	srcRole = EnvelopeHeader(msg, Env_SourceRoleNbr);
	srcSubject = EnclosureHeader(enc, Enc_SubjectNbr);

	destUnit = EnvelopeHeader(msg, Env_DestUnitNbr);
	destNode = EnvelopeHeader(msg, Env_DestNodeNbr);

	sourceNode = GetSourceNode(destUnit, destNode, gWay);
	if (sourceNode == NULL)
		return NULL;  

	nodeElt = NULL; 

	for (elt = lyst_first(gWay->invitationSet); elt != NULL; elt = lyst_next(elt))
	{
		inv = (Invitation *)lyst_data(elt);
		if (IsRAMSPDUSatisfyInvitation(gWay, msg, inv))
		{
			if ((nodeElt = IsNodeExist(sourceNode, &(inv->nodeSet))) != NULL)
				break;
		}
	}
	return nodeElt;
}

int IsValidAnnounceReceiver(RamsGateway* gWay, Node *node, int dUnit, int dRole)
{
	int rUnit, rRole;

	rUnit = node->unitNbr;
	rRole = RoleNumber(gWay->amsNode, rUnit, node->nbr);
	if (dUnit != 0 && dUnit != rUnit)
	{
        if (!ams_subunit_of(gWay->amsNode, rUnit, dUnit)) 
			return 0;
	}
	if (dRole != 0 && dRole != rRole)
		return 0;
	return 1;
}
