/*
ramscommon.c: functions enabling the implementation of RAMS gateway based applications

Author: Shin-Ywan (Cindy) Wang
Copyright (c) 2005, California Institute of Technology.
ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
*/

#include "rams.h"
#include <ams.h>
#include <amsP.h>

extern int	SendMessageToContinuum(RamsGateway *gWay, int continuumId,
			unsigned char flowLabel, char* envelope,
			int envelopeLength);
extern int	SendPetitionToDeclaredNeighbor(RamsGateway *gWay, char *env);

void	ConstructEnvelope(unsigned char *envelope, int continuumNbr,
		int unitNbr, int sourceID, int destID, int subjectNbr,
		int enclosureHdrLength, char *enclosureHdr,
		int enclosureContentLength, char *enclosureContent,
		int controlCode)
{
	short		i2;
	unsigned short	enclosureLength;

	envelope[0] = controlCode & 0x0000000f;
	envelope[1] = 0;		/*	Reserved.		*/
	envelope[2] = (continuumNbr >> 8) & 0x0000007f;
	envelope[3] = continuumNbr & 0x000000ff;
	envelope[4] = (unitNbr >> 8) & 0x000000ff;
	envelope[5] = unitNbr & 0x000000ff;
	envelope[6] = sourceID;
	envelope[7] = destID;
	i2 = subjectNbr;
	i2 = htons(i2);			/*	Might be negative.	*/
	memcpy(envelope + 8, (unsigned char *) &i2, 2);
	enclosureLength = enclosureHdrLength + enclosureContentLength;
	envelope[10] = (enclosureLength >> 8) & 0x00ff;
	envelope[11] = enclosureLength & 0x00ff;
	if (enclosureLength > 0)
	{
		if (enclosureHdrLength > 0)
		{
			memcpy(envelope + 12, enclosureHdr, enclosureHdrLength);
		}

		memcpy(envelope + 12 + enclosureHdrLength, enclosureContent,
				enclosureContentLength);
	}
}

int	EnclosureHeader(char *enclosure, EnclosureContent encId)
{
	unsigned char	*enc = (unsigned char *) enclosure;
	int		num = 0;	/*	Init. to avoid warning.	*/
	short		subj;

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

	case Enc_ModuleNbr:
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

int	EnvelopeHeader(char *envelope, EnvelopeContent conId)
{
	unsigned char	*envl = (unsigned char *) envelope;
	int		num = 0;	/*	Init. to avoid warning.	*/
	short		subj;

	switch (conId)
	{
	case Env_ControlCode: 
		num = envl[0] & 0x0f;
		break;

	case Env_ContinuumNbr:
		num = ((envl[2] << 8) & 0x00007f00) + envelope[3];
		break;

	case Env_PublishUnitNbr:
	case Env_DestUnitNbr:
	case Env_UnitField:
		num = ((envl[4] << 8) & 0x0000ff00) + envelope[5];
		break;

	case Env_PublishRoleNbr:
	case Env_SourceRoleNbr:
	case Env_SourceIDField:
		num = envl[6];
		break;

	case Env_DestModuleNbr:
	case Env_DestRoleNbr:
	case Env_DestIDField:
		num = envl[7];
		break;

	case Env_SubjectNbr:
		memcpy((char *)&subj, envl + 8, 2);
		subj = ntohs(subj);
		num = subj;
		break;

	case Env_EnclosureLength:
		num = ((envl[10] << 8) & 0x0000ff00) + (envl[11] & 0x000000ff);
		break;
	}

	return num; 
}

char	*EnvelopeText(char *envelope, int contentLength)
{
	/*	Returns pointer to content of envelope, if any.		*/

	unsigned char	*envl = (unsigned char *) envelope;

	if (contentLength < 0)	/*	Content length not yet known.	*/
	{
		contentLength = ((envl[10] << 8) & 0x0000ff00)
				+ (envl[11] & 0x000000ff);
	}

	if (contentLength > 0)
	{
		return envelope + ENVELOPELENGTH;
	}

	/*	Envelope has no content.				*/

	return NULL;
}

RamsNode	*Look_Up_Neighbor(RamsGateway *gWay, char *gwEid)
{
	LystElt		elt;
	RamsNode	*node;

	for (elt = lyst_first(gWay->ramsMib->ramsNeighbors); elt != NULL;
			elt = lyst_next(elt))
	{
		node = (RamsNode *)lyst_data(elt);
		if (strcmp(node->gwEid, gwEid) == 0)
		{
			return node;
		}
	}

	return NULL;
}

RamsNode* Look_Up_DeclaredNeighbor(RamsGateway *gWay, int ramsNbr)
{
	LystElt		elt;
	RamsNode	*node;

	for (elt = lyst_first(gWay->ramsMib->declaredNeighbors); elt != NULL;
			elt = lyst_next(elt))
	{
		node = (RamsNode *)lyst_data(elt);
		if (node->continuumNbr == ramsNbr)
		{
			return node;
		}
	}

	return NULL;
}

void	GetEnvelopeSpecification(char *envelope, int *continuumNbr,
		int *unitNbr, int *roleNbr)
{
	unsigned char	*env = (unsigned char *) envelope;

	*continuumNbr = ((env[2] << 8) & 0x00007f00) + env[3];
	*unitNbr = ((env[4] << 8) & 0x0000ff00) + env[5];
	*roleNbr = env[7];
}

void	GetPetitionSpecification(PetitionSpec *pet, int *continuumNbr,
		int *unitNbr, int *roleNbr)
{
	GetEnvelopeSpecification(pet->envelope, continuumNbr, unitNbr, roleNbr);
}

int	SendEnvelopeToNeighbor(RamsGateway *gWay, unsigned char flowLabel,
		char *envelope, int envelopeSize, int toContinuum)
{
	if (!SendMessageToContinuum(gWay, toContinuum, flowLabel, envelope,
			envelopeSize))
	{
		ErrMsg("Failed sending envelope to neighbor");
		return 0;
	}

	return 1; 
}

int	ConstructEnvelopeToDeclaredNeighbor(RamsGateway *gWay,
		Enclosure *enclosure, int toContinuum, int toUnit,
		int sourceID, int destID, int cCode, int sNbr)
{
	char	*envelope;
	int	subjectNbr;
	int	succeed; 

	envelope = NULL; 
	if (enclosure != NULL)
	{
		envelope = (char *) MTAKE(ENVELOPELENGTH
				+ enclosure->contentLength);
		CHKZERO(envelope);
		subjectNbr = EnclosureHeader(enclosure->enclosureContent,
				Enc_SubjectNbr);
		ConstructEnvelope((unsigned char *) envelope, toContinuum,
				toUnit, sourceID, destID, subjectNbr, 0, NULL,
				enclosure->contentLength,
				enclosure->enclosureContent, cCode);
	}
	else 
	{
		envelope = (char *) MTAKE(ENVELOPELENGTH);
		CHKZERO(envelope);
		subjectNbr = sNbr;
		ConstructEnvelope((unsigned char *) envelope, toContinuum,
				toUnit, sourceID, destID, subjectNbr, 0, NULL,
				0, NULL, cCode);
	}

	succeed = SendPetitionToDeclaredNeighbor(gWay, envelope);
	MRELEASE(envelope);
	return succeed; 
}

int	ConstructEnvelopeToNeighbor(RamsGateway *gWay, unsigned char flowLabel,
		Enclosure *enclosure, int toContinuum, int toUnit,
		int sourceID, int destID, int cCode, int sNbr,
		int neighborContinuumNbr)
{
	char	*envelope;
	int	encLength;
	int	succeed; 

	envelope = NULL; 
	if (enclosure != NULL)
	{
		envelope = (char *) MTAKE(ENVELOPELENGTH
				+ enclosure->contentLength);
		CHKZERO(envelope);
		ConstructEnvelope((unsigned char *) envelope, toContinuum,
				toUnit, sourceID, destID, sNbr, 0, NULL,
				enclosure->contentLength,
				enclosure->enclosureContent, cCode);
		encLength = enclosure->contentLength;
	}
	else 
	{
		envelope = (char *) MTAKE(ENVELOPELENGTH);
		CHKZERO(envelope);
		ConstructEnvelope((unsigned char *) envelope, toContinuum,
				toUnit, sourceID, destID, sNbr, 0, NULL, 0,
				NULL, cCode);
		encLength = 0;
	}	

	succeed = SendEnvelopeToNeighbor(gWay, flowLabel, envelope,
			ENVELOPELENGTH + encLength, neighborContinuumNbr);
	MRELEASE(envelope);
	return succeed; 

}

Enclosure	*ConstructEnclosure(int continuumNbr, int unitNbr,
			int moduleNbr, int subjectNbr, int contentLength,
			char *content, int context, AmsMsgType msgType,
			int priority, unsigned char flowLabel)
{
	Enclosure	*enc;
	char		*header;
	unsigned long	u8;
	unsigned char	u1;
	short		i2;

	/*	Enclosure within a RAMS message is an AAMS message.	*/

	enc = (Enclosure *) MTAKE(sizeof(Enclosure));
	enc->contentLength = contentLength + AMSMSGHEADER;
	enc->enclosureHeaderLength = AMSMSGHEADER;	
	enc->enclosureContent = NULL; 
	enc->enclosureContent = (char *) MTAKE(enc->contentLength);
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
	*(header + 6) = moduleNbr & 0x000000ff;
	*(header + 7) = 0;		/*	Reserved.		*/
	context = htonl(context);
	memcpy(header + 8, (char *) &context, 4);
	i2 = subjectNbr;
	i2 = htons(i2);
	memcpy(header + 12, (char *) &i2, 2);
	*(header + 14) = (contentLength >> 8) & 0x000000ff;
	*(header + 15) = (contentLength) & 0x000000ff;
	if (contentLength > 0)
	{
		memcpy(enc->enclosureContent + AMSMSGHEADER, content,
				contentLength);
	}
	
	return enc;
}

void	DeleteEnclosure(Enclosure *enc)
{
	if (enc->enclosureContent)
	{
		MRELEASE((char *)(enc->enclosureContent));
	}

	MRELEASE(enc);
}

Petition	*ConstructPetition(int domainContinuum, int domainRole,
			int domainUnit, int subNbr, int cCode)
{
	Petition	*pet;

	pet = MTAKE(sizeof(Petition));
	pet->DistributionModuleSet = lyst_create();
	pet->DestinationNodeSet = lyst_create();
	pet->SourceNodeSet = lyst_create();
	pet->specification = (PetitionSpec *) MTAKE(sizeof(PetitionSpec));
	pet->specification->envelope = (char *) MTAKE(ENVELOPELENGTH);
	pet->specification->envelopeLength = ENVELOPELENGTH;
	pet->specification->toContinuumNbr = domainContinuum;
	ConstructEnvelope((unsigned char *) (pet->specification->envelope),
			domainContinuum, domainUnit, domainRole, 0, subNbr,
			0, NULL, 0, NULL, cCode);
	return pet;
}

Petition* ConstructPetitionFromEnvelope(char* envelope)
{
	Petition	*pet;

	pet = MTAKE(sizeof(Petition));
	pet->DistributionModuleSet = lyst_create();
	pet->DestinationNodeSet = lyst_create();
	pet->SourceNodeSet = lyst_create();
	pet->specification = (PetitionSpec *) MTAKE(sizeof(PetitionSpec));
	pet->specification->envelope = NULL;
	pet->specification->envelopeLength = 0;
	pet->specification->toContinuumNbr = -1; 
	if (envelope != NULL)
	{
		pet->specification->envelopeLength = ENVELOPELENGTH
				+ EnvelopeHeader(envelope, Env_EnclosureLength);
		pet->specification->toContinuumNbr = EnvelopeHeader(envelope,
				Env_ContinuumNbr);
		pet->specification->envelope = (char *)
				MTAKE(pet->specification->envelopeLength);

		/*	envelope space will be reused for next message	*/

		memcpy(pet->specification->envelope, envelope,
				pet->specification->envelopeLength);
	}

	return pet;
}

void DeleteEnvelope(Envelope *env)
{
	if (env->enclosure)
	{
		MRELEASE(env->enclosure);
	}

	MRELEASE(env);
}

int	SamePetition(Petition *pet1, Petition *pet2)
{
	char	*env1 = pet1->specification->envelope;
	char	*env2 = pet2->specification->envelope;

	if (EnvelopeHeader(env1, Env_ContinuumNbr)
			== EnvelopeHeader(env2, Env_ContinuumNbr)
	&& EnvelopeHeader(env1, Env_PublishUnitNbr)
			== EnvelopeHeader(env2, Env_PublishUnitNbr)
	&& EnvelopeHeader(env1, Env_PublishRoleNbr)
			== EnvelopeHeader(env2, Env_PublishRoleNbr)
	&& EnvelopeHeader(env1, Env_SubjectNbr)
			== EnvelopeHeader(env2, Env_SubjectNbr))
	{
		return 1;
	}

	return 0;
}

int	PetitionMatchesDomain(Petition *pet, int domainContinuum,
		int domainRole, int domainUnit, int subNbr)
{
	char	*env = pet->specification->envelope;

	if (EnvelopeHeader(env, Env_ContinuumNbr) == domainContinuum
	&& EnvelopeHeader(env, Env_PublishUnitNbr) == domainUnit
	&& EnvelopeHeader(env, Env_PublishRoleNbr) == domainRole
	&& EnvelopeHeader(env, Env_SubjectNbr) == subNbr)
	{	
		return 1;
	}

	return 0;
}

int	EnclosureSatisfiesPetition(AmsModule module, char *rpdu, Petition *pet)
{
	char	*enc;
	char	*env;
	int	petCont;
	int	petUnit;
	int	petRole;

	enc = EnvelopeText(rpdu, -1);
	if (enc == NULL)
	{
		return 0;
	}

	env = pet->specification->envelope;
	if (EnclosureHeader(enc, Enc_SubjectNbr)
			!= EnvelopeHeader(env, Env_SubjectNbr))
	{
		return 0;
	}

	/*	Enclosure matches petition subject.			*/

	petCont = EnvelopeHeader(env, Env_ContinuumNbr);
	if (petCont != 0)
	{
		/*	Domain continuum of petition is not "all".	*/

		if (petCont != EnclosureHeader(enc, Enc_ContinuumNbr))
		{
			return 0;
		}
	}

	/*	Enclosure matches domain continuum of petition.		*/

	petUnit = EnvelopeHeader(env, Env_PublishUnitNbr);
	if (petUnit != 0)
	{
		/*	Domain unit of petition is not the root unit.	*/

		if (!ams_subunit_of(module, EnclosureHeader(enc, Enc_UnitNbr),
					petUnit))
		{
			return 0;
		}
	}

	/*	Enclosure matches domain unit of petition.		*/

	petRole = EnvelopeHeader(env, Env_PublishRoleNbr);
	if (petRole != 0)
	{
		/*	Domain role of petition is not "all".  Note
		 *	that the role of the source of the message
		 *	is NOT carried in the enclosure; we have to
		 *	get it from the RPDU's header instead.		*/

		if (petRole != EnvelopeHeader(rpdu, Env_PublishRoleNbr))
		{
			return 0;
		}
	}

	/*	Enclosure matches domain role of petition.		*/

	return 1;
}

int	EnclosureSatisfiesInvitation(RamsGateway *gWay, char* rpdu,
		Invitation *inv)
{
	InvitationSpec	*spec = inv->inviteSpecification;
	char		*enc;

	enc = EnvelopeText(rpdu, -1);
	if (enc == NULL)
	{
		return 0;
	}

	if (spec->subjectNbr != 0)
	{
		/*	Subject of invitation is not "all".		*/

		if (spec->subjectNbr != EnclosureHeader(enc, Enc_SubjectNbr))
		{
			return 0;
		}
	}

	/*	Enclosure matches invitation subject.			*/

	if (spec->domainContNbr != 0)
	{
		/*	Domain continuum of invitation is not "all".	*/

		if (spec->domainContNbr != EnclosureHeader(enc,
					Enc_ContinuumNbr))
		{
			return 0;
		}
	}

	/*	Enclosure matches domain continuum of invitation.	*/


	if (spec->domainUnitNbr != 0)
	{
		/*	Domain unit of invitation is not "all".		*/

		if (!ams_subunit_of(gWay->amsModule,
				EnclosureHeader(enc, Enc_UnitNbr),
				spec->domainUnitNbr))
		{
			return 0;
		}
	}

	/*	Enclosure matches domain unit of invitation.		*/

	if (spec->domainRoleNbr != 0)
	{
		/*	Domain role of invitation is not "all".  Note
		 *	that the role of the source of the message
		 *	is NOT carried in the enclosure; we have to
		 *	get it from the RPDU's header instead.		*/

		if (spec->domainRoleNbr != EnvelopeHeader(rpdu,
				Env_SourceRoleNbr))
		{
			return 0;
		}
	}

	/*	Enclosure matches domain role of invitation.		*/

	return 1; 
}

int	RoleNumber(AmsModule module, int unitNbr, int moduleNbr)
{
	char	*roleName;

	roleName = ams_get_role_name(module, unitNbr, moduleNbr);
	if (roleName == NULL)
	{
		return -1; 
	}

   	return ams_lookup_role_nbr(module, roleName);
}

int	MessageSatisfiesPetition(AmsModule module, int msgCont, int msgUnit,
		int msgModule, int subjectNbr, Petition* pet)
{
	char	*env = pet->specification->envelope;
	int	petCont;
	int	petUnit;
	int	petRole;

	if (subjectNbr != EnvelopeHeader(env, Env_SubjectNbr))
	{
		return 0;
	}

	petCont = EnvelopeHeader(env, Env_ContinuumNbr);
	if (petCont != 0)
	{
		if (petCont != msgCont)
		{
			return 0;
		}
	}

	petUnit = EnvelopeHeader(env, Env_PublishUnitNbr);
	if (petUnit != 0)
	{
		if (!ams_subunit_of(module, msgUnit, petUnit))
		{
			return 0;
		}
	}

	petRole = EnvelopeHeader(env, Env_PublishRoleNbr);
	if (petRole != 0)
	{
		if (petRole != RoleNumber(module, msgUnit, msgModule))
		{
			return 0;
		}
	}

	return 1;
}

LystElt	ModuleSetMember(Module *module, Lyst lyst)
{
	LystElt	elt;
	Module	*member;

	for (elt = lyst_first(lyst); elt != NULL; elt = lyst_next(elt))
	{
		member = (Module *) lyst_data(elt);
		if (module->unitNbr == member->unitNbr
		&& module->nbr == member->nbr)
		{
        		return elt;
		}
	}

	return NULL;
}

LystElt InvitationSetMember(int dUnit, int dRole, int dCont, int sub, Lyst lyst)
{
	LystElt		elt;
	Invitation	*inv; 

	for (elt = lyst_first(lyst); elt != NULL; elt = lyst_next(elt))
	{
		inv = (Invitation *) lyst_data(elt);
		if (inv->inviteSpecification->domainContNbr == dCont
		&& inv->inviteSpecification->domainUnitNbr == dUnit
		&& inv->inviteSpecification->domainRoleNbr == dRole
		&& inv->inviteSpecification->subjectNbr == sub)
		{
			return elt;
		}
	}

	return NULL;
}

static int	SameRamsNode(RamsNode *node1, RamsNode *node2)
{
	if (node1->continuumNbr == node2->continuumNbr
	&& node1->protocol == node2->protocol
	&& strcmp(node1->gwEid, node2->gwEid) == 0)
	{
		return 1;
	}

	return 0;
}

LystElt	NodeSetMember(RamsNode *fromNode, Lyst lyst)
{
	LystElt		elt;
	RamsNode	*node;

	for (elt = lyst_first(lyst); elt != NULL; elt = lyst_next(elt))
	{
		node = (RamsNode *) lyst_data(elt);
		if (SameRamsNode(fromNode, node))
		{
			return elt;
		}
	}

	return NULL;
}

int	ModuleIsMyself(Module* sModule, RamsGateway *gWay)
{
	if (sModule->unitNbr == ams_get_unit_nbr(gWay->amsModule)
	&& sModule->nbr == ams_get_module_nbr(gWay->amsModule))
	{
		return 1;
	}

	return 0;
}

Module	*LookupModule(int unitNbr, int moduleNbr, RamsGateway *rg)
{
	return rg->amsModule->venture->units[unitNbr]->cell->modules[moduleNbr];
}

void	SubtractNodeSets(Lyst set1, Lyst set2)
{
	LystElt elt, nextElt;
	RamsNode *node;

	for (elt = lyst_first(set1); elt != NULL; )
	{
		node = (RamsNode *)lyst_data(elt);
		nextElt = lyst_next(elt);
		if (NodeSetMember(node, set2))
		{
			lyst_delete(elt);
		}
		elt = nextElt;
	}
}

void	AddNodeSets(Lyst set1, Lyst set2)
{
	LystElt elt;
	RamsNode *node;

	for (elt = lyst_first(set2); elt != NULL; elt = lyst_next(elt))
	{
		node = (RamsNode *)lyst_data(elt);
		if (!NodeSetMember(node, set1))
		{
			lyst_insert_last(set1, node);
		}
	}
}

RamsNode	*GetConduitForContinuum(int continuumNbr, RamsGateway *gWay)
{
	LystElt		elt;
	Petition	*pet; 
	int		subN;
	LystElt		nodeElt;

	for (elt = lyst_first(gWay->petitionSet); elt != NULL;
			elt = lyst_next(elt))
	{
		pet = (Petition *) lyst_data(elt);
		subN = EnvelopeHeader(pet->specification->envelope,
				Env_SubjectNbr);
		if (subN == (0 - continuumNbr))
		{
			nodeElt = lyst_first(pet->DestinationNodeSet);
			if (nodeElt == NULL)
			{
				continue;
			}

			return (RamsNode *) lyst_data(nodeElt);
		}
	}

	return NULL;	/*	No conduit to this continuum.		*/
}


Lyst	PropagationSet(RamsGateway *gWay, Petition *pet)
{
	Lyst		PS; 
	int		domainCont;
	LystElt		elt;
	RamsNode	*node;

	PS = lyst_create();

	/*	First populate the propagation set with all RAMS
	 *	nodes in the domain of the petition.			*/

	domainCont = EnvelopeHeader(pet->specification->envelope,
			Env_ContinuumNbr);
	if (domainCont == 0)
	{
		/*	Domain of petition is "all continua".		*/

		for (elt = lyst_first(gWay->ramsMib->declaredNeighbors);
				elt != NULL; elt = lyst_next(elt))
		{
			node = (RamsNode *) lyst_data(elt);
			lyst_insert_last(PS, node);
		}
	}
	else   
	{ 
		/*	Domain of petition is a single remote
		 *	continuum; find conduit to that continuum.	*/

		node = GetConduitForContinuum(domainCont, gWay);
		if (node != NULL)
		{
			lyst_insert_last(PS, node);
		}
	}
		
	/*	Now REMOVE from the propagation set all members of
	 *	the source gateway set of this petition.		*/

	SubtractNodeSets(PS, pet->SourceNodeSet);

	/*	Also REMOVE from the propagation set the sole member
	 *	of the DGS if the petition's DMS is empty and its DGS
	 *	contains only one member.				*/
	
	if (lyst_length(pet->DistributionModuleSet) == 0
	&& lyst_length(pet->DestinationNodeSet) == 1)
	{
		SubtractNodeSets(PS, pet->DestinationNodeSet);
	}
	
	return PS;
}

int	PetitionIsAssertable(RamsGateway *gWay, Petition *pet)
{
	if (gWay->ramsMib->netType == MESHTYPE)
	{
		if (lyst_first(pet->DistributionModuleSet) != NULL)
		{
			return 1;
		}
	}
	else if (gWay->ramsMib->netType == TREETYPE)
	{
		if (lyst_first(pet->DistributionModuleSet) != NULL
		|| lyst_first(pet->DestinationNodeSet) != NULL)
		{
			return 1;
		}
	}
	
	return 0;
}

int	PetitionIsCancellable(Petition *pet)
{
	RamsNode	*node1;

	/*	Petition may be canceled if and only if (a) its
	 *	DMS is empty and (b) either its DGS is empty or
	 *	its DGS contains only some member of the petition's
	 *	SGS.							*/

	if (lyst_first(pet->DistributionModuleSet) == NULL)
	{
		if (lyst_first(pet->DestinationNodeSet) == NULL)
		{
			return 1;
		}
		
		if (lyst_length(pet->DestinationNodeSet) == 1)
		{
			node1 = (RamsNode *)
				lyst_data(lyst_first(pet->DestinationNodeSet));
			if (NodeSetMember(node1, pet->SourceNodeSet) != NULL)
			{
				return 1;
			}
			else
			{
				return 0;
			}
		}
	}

	return 0;
}
	
void	DeletePetition(Petition *pet)
{
	lyst_destroy(pet->SourceNodeSet);
	lyst_destroy(pet->DestinationNodeSet);
	lyst_destroy(pet->DistributionModuleSet);
	MRELEASE(pet->specification->envelope);
	MRELEASE(pet->specification);
	MRELEASE(pet);
}

int	MessageIsInvited(RamsGateway *gWay, char* msg)
{
	char		*enc;
	int		destUnitNbr;
	int		destModuleNbr; 
	Module		*destModule; 
	LystElt		elt;
	Invitation	*inv; 

	enc = EnvelopeText(msg, -1);
	if (enc == NULL)
	{
		return 0;
	}

	destUnitNbr = EnvelopeHeader(msg, Env_DestUnitNbr);
	destModuleNbr = EnvelopeHeader(msg, Env_DestModuleNbr);
	destModule = LookupModule(destUnitNbr, destModuleNbr, gWay);
	if (destModule == NULL)
	{
		return 0;  
	}

	for (elt = lyst_first(gWay->invitationSet); elt != NULL;
			elt = lyst_next(elt))
	{
		inv = (Invitation *) lyst_data(elt);
		if (EnclosureSatisfiesInvitation(gWay, msg, inv))
		{
			if (ModuleSetMember(destModule, inv->moduleSet) != NULL)
			{
				return 1;
			}
		}
	}

	return 0;
}

int	ModuleIsInAnnouncementDomain(RamsGateway* gWay, Module *module,
		int dUnit, int dRole)
{
	int	mUnit;
	int	mRole;

	mUnit = module->unitNbr;
	mRole = RoleNumber(gWay->amsModule, mUnit, module->nbr);
	if (dUnit != 0 && dUnit != mUnit)
	{
		/*	Domain unit is not the root unit (the entire
		 *	venture), and it is not the unit in which this
		 *	module is registered.  Is it a super-unit of
		 *	the unit in which this module is registered?	*/

        	if (!ams_subunit_of(gWay->amsModule, mUnit, dUnit)) 
		{
			return 0;
		}
	}

	/*	Module is registered in a unit that is within the
	 *	domain of the announcement.				*/

	if (dRole != 0 && dRole != mRole)
	{
		/*	Domain role is not "all roles", and it is not
		 *	the role in which this module registered.	*/

		return 0;
	}

	/*	Module is registered in a role that is within the
	 *	domain of the announcement.				*/

	return 1;
}
