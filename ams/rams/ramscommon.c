/*
	ramscommon.c:	utility functions supporting the
			implementation of RAMS gateway nodes.

	Author: Shin-Ywan (Cindy) Wang
	Copyright (c) 2005, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
*/

#include "ramscommon.h"

RamsGateway	*_gWay(RamsGateway *currentGateway)
{
	static RamsGateway	*gWay = NULL;

	if (currentGateway)		/*	Shut down the gateway.	*/
	{
		if (gWay)
		{
			MRELEASE(gWay);
			gWay = NULL;
		}
	}
	else
	{
		if (gWay == NULL)	/*	Create gateway.		*/
		{
			gWay = (RamsGateway *) MTAKE(sizeof(RamsGateway));
			if (gWay)
			{
				memset((char *) gWay, 0, sizeof(RamsGateway));
			}
		}
	}

	return gWay;
}

void	ConstructEnvelope(unsigned char *envelope, int destContinuumNbr,
		int unitNbr, int sourceID, int destID, int subjectNbr,
		int enclosureLength, char *enclosure, int controlCode)
{
	short	i2;

	CHKVOID(envelope);
	envelope[0] = controlCode & 0x0000000f;
	envelope[1] = 0;		/*	Reserved.		*/
	envelope[2] = (destContinuumNbr >> 8) & 0x0000007f;
	envelope[3] = destContinuumNbr & 0x000000ff;
	envelope[4] = (unitNbr >> 8) & 0x000000ff;
	envelope[5] = unitNbr & 0x000000ff;
	envelope[6] = sourceID;
	envelope[7] = destID;
	i2 = subjectNbr;
	i2 = htons(i2);			/*	Might be negative.	*/
	memcpy(envelope + 8, (unsigned char *) &i2, 2);
	envelope[10] = (enclosureLength >> 8) & 0x00ff;
	envelope[11] = enclosureLength & 0x00ff;
	if (enclosureLength > 0)
	{
		CHKVOID(enclosure);
		memcpy(envelope + 12, enclosure, enclosureLength);
	}
}

int	EnclosureHeader(char *enclosure, EnclosureField encId)
{
	unsigned char	*enc = (unsigned char *) enclosure;
	int		num = 0;	/*	Init. to avoid warning.	*/
	short		subj;

	CHKZERO(enclosure);
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

int	EnvelopeHeader(char *envelope, EnvelopeField conId)
{
	unsigned char	*envl = (unsigned char *) envelope;
	int		num = 0;
	short		subj;

	CHKZERO(envelope);
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
		memcpy((char *) &subj, envl + 8, 2);
		subj = ntohs(subj);
		num = subj;
		break;

	case Env_EnclosureLength:
		num = ((envl[10] << 8) & 0x0000ff00) + (envl[11] & 0x000000ff);
		break;
	}

	return num; 
}

char	*EnvelopeContent(char *envelope, int contentLength)
{
	/*	Returns pointer to content of envelope, if any.		*/

	unsigned char	*envl = (unsigned char *) envelope;

	CHKNULL(envelope);
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

	CHKNULL(gWay);
	CHKNULL(gwEid);
	for (elt = lyst_first(gWay->ramsNeighbors); elt != NULL;
			elt = lyst_next(elt))
	{
		node = (RamsNode *) lyst_data(elt);
		if (strcmp(node->gwEid, gwEid) == 0)
		{
			return node;
		}
	}

	return NULL;
}

RamsNode	*Look_Up_DeclaredNeighbor(RamsGateway *gWay, int ramsNbr)
{
	LystElt		elt;
	RamsNode	*node;

	CHKNULL(gWay);
	for (elt = lyst_first(gWay->declaredNeighbors); elt != NULL;
			elt = lyst_next(elt))
	{
		node = (RamsNode *) lyst_data(elt);
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

	CHKVOID(envelope);
	CHKVOID(continuumNbr);
	CHKVOID(unitNbr);
	CHKVOID(roleNbr);
	*continuumNbr = ((env[2] << 8) & 0x00007f00) + env[3];
	*continuumNbr = ((env[2] << 8) & 0x00007f00) + env[3];
	*continuumNbr = ((env[2] << 8) & 0x00007f00) + env[3];
	*continuumNbr = ((env[2] << 8) & 0x00007f00) + env[3];
	*unitNbr = ((env[4] << 8) & 0x0000ff00) + env[5];
	*roleNbr = env[7];
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
	CHKNULL(enc);
	memset((char *) enc, 0, sizeof(Enclosure));
	enc->length = contentLength + AMSMSGHEADER;
	enc->text = (char *) MTAKE(enc->length);
	CHKNULL(enc->text);
	header = enc->text;
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
		CHKNULL(content);
		memcpy(enc->text + AMSMSGHEADER, content, contentLength);
	}
	
	return enc;
}

void	DeleteEnclosure(Enclosure *enc)
{
	CHKVOID(enc);
	if (enc->text)
	{
		MRELEASE((char *)(enc->text));
	}

	MRELEASE(enc);
}

Petition	*ConstructPetition(int domainContinuum, int domainRole,
			int domainUnit, int subNbr, int cCode)
{
	int		amsMemory = getIonMemoryMgr();
	Petition	*pet;

	pet = MTAKE(sizeof(Petition));
	CHKNULL(pet);
	memset((char *) pet, 0, sizeof(Petition));
	pet->DistributionModuleSet = lyst_create_using(amsMemory);
	CHKNULL(pet->DistributionModuleSet);
	pet->DestinationNodeSet = lyst_create_using(amsMemory);
	CHKNULL(pet->DestinationNodeSet);
	pet->SourceNodeSet = lyst_create_using(amsMemory);
	CHKNULL(pet->SourceNodeSet);
	pet->specification = (PetitionSpec *) MTAKE(sizeof(PetitionSpec));
	CHKNULL(pet->specification);
	pet->specification->envelope = (char *) MTAKE(ENVELOPELENGTH);
	CHKNULL(pet->specification->envelope);
	pet->specification->envelopeLength = ENVELOPELENGTH;
	pet->specification->toContinuumNbr = domainContinuum;
	ConstructEnvelope((unsigned char *) (pet->specification->envelope),
			domainContinuum, domainUnit, domainRole, 0, subNbr,
			0, NULL, cCode);
	return pet;
}

Petition	*ConstructPetitionFromEnvelope(char* envelope)
{
	int		amsMemory = getIonMemoryMgr();
	Petition	*pet;

	CHKNULL(envelope);
	pet = MTAKE(sizeof(Petition));
	CHKNULL(pet);
	memset((char *) pet, 0, sizeof(Petition));
	pet->DistributionModuleSet = lyst_create_using(amsMemory);
	CHKNULL(pet->DistributionModuleSet);
	pet->DestinationNodeSet = lyst_create_using(amsMemory);
	CHKNULL(pet->DestinationNodeSet);
	pet->SourceNodeSet = lyst_create_using(amsMemory);
	CHKNULL(pet->SourceNodeSet);
	pet->specification = (PetitionSpec *) MTAKE(sizeof(PetitionSpec));
	CHKNULL(pet->specification);
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
		CHKNULL(pet->specification->envelope);
		memcpy(pet->specification->envelope, envelope,
				pet->specification->envelopeLength);
	}

	return pet;
}

int	SamePetition(Petition *pet1, Petition *pet2)
{
	char	*env1;
	char	*env2;

	CHKZERO(pet1);
	CHKZERO(pet2);
	CHKZERO(pet1->specification);
	CHKZERO(pet2->specification);
	env1 = pet1->specification->envelope;
	env2 = pet2->specification->envelope;
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
	char	*env;

	CHKZERO(pet);
	CHKZERO(pet->specification);
	env = pet->specification->envelope;
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

	CHKZERO(rpdu);
	CHKZERO(pet);
	enc = EnvelopeContent(rpdu, -1);
	if (enc == NULL)
	{
		return 0;
	}

	CHKZERO(pet->specification);
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
	InvitationSpec	*spec;
	char		*enc;

	CHKZERO(gWay);
	CHKZERO(rpdu);
	CHKZERO(inv);
	spec = inv->inviteSpecification;
	CHKZERO(spec);
	enc = EnvelopeContent(rpdu, -1);
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
	char	*env;
	int	petCont;
	int	petUnit;
	int	petRole;

	CHKZERO(pet);
	CHKZERO(pet->specification);
	env = pet->specification->envelope;
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

	CHKNULL(module);
	CHKNULL(lyst);
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

	CHKNULL(lyst);
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

	CHKNULL(fromNode);
	CHKNULL(lyst);
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
	CHKZERO(sModule);
	CHKZERO(gWay);
	if (sModule->unitNbr == ams_get_unit_nbr(gWay->amsModule)
	&& sModule->nbr == ams_get_module_nbr(gWay->amsModule))
	{
		return 1;
	}

	return 0;
}

Module	*LookupModule(int unitNbr, int moduleNbr, RamsGateway *rg)
{
	CHKNULL(rg);
	return rg->amsModule->venture->units[unitNbr]->cell->modules[moduleNbr];
}

void	SubtractNodeSets(Lyst set1, Lyst set2)
{
	LystElt		elt;
	LystElt		nextElt;
	RamsNode	*node;

	CHKVOID(set1);
	CHKVOID(set2);
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
	LystElt		elt;
	RamsNode	*node;

	CHKVOID(set1);
	CHKVOID(set2);
	for (elt = lyst_first(set2); elt != NULL; elt = lyst_next(elt))
	{
		node = (RamsNode *)lyst_data(elt);
		if (!NodeSetMember(node, set1))
		{
			if (lyst_insert_last(set1, node) == NULL)
			{
				ErrMsg("Failed adding node to set.");
				return;
			}
		}
	}
}

RamsNode	*GetConduitForContinuum(int continuumNbr, RamsGateway *gWay)
{
	LystElt		elt;
	Petition	*pet; 
	int		subN;
	LystElt		nodeElt;

	CHKNULL(gWay);
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

int	PetitionIsAssertable(RamsGateway *gWay, Petition *pet)
{
	CHKZERO(gWay);
	CHKZERO(pet);
	if (lyst_length(pet->DistributionModuleSet) > 0)
	{
		return 1;
	}

	if (gWay->netType == TREETYPE
	&& lyst_length(pet->DestinationNodeSet) > 0)
	{
		return 1;
	}

	return 0;
}

Lyst	AssertionSet(RamsGateway *gWay, Petition *pet)
{
	Lyst		assertionSet; 
	int		domainCont;
	LystElt		elt;
	RamsNode	*node;

	CHKNULL(gWay);
	CHKNULL(pet);
	assertionSet = lyst_create();
	CHKNULL(assertionSet);

	/*	First populate the assertion set with all RAMS
	 *	nodes in the domain of the petition.			*/

	domainCont = EnvelopeHeader(pet->specification->envelope,
			Env_ContinuumNbr);
	if (domainCont == 0)
	{
		/*	Domain of petition is "all continua".		*/

		for (elt = lyst_first(gWay->declaredNeighbors);
				elt != NULL; elt = lyst_next(elt))
		{
			node = (RamsNode *) lyst_data(elt);
			if (lyst_insert_last(assertionSet, node) == NULL)
			{
				ErrMsg("Failed adding node to set.");
				return NULL;
			}
		}
	}
	else   
	{ 
		/*	Domain of petition is a single remote
		 *	continuum; find conduit to that continuum.	*/

		node = GetConduitForContinuum(domainCont, gWay);
		if (node != NULL)
		{
			if (lyst_insert_last(assertionSet, node) == NULL)
			{
				ErrMsg("Failed adding node to set.");
				return NULL;
			}
		}
	}
#if RAMSDEBUG
printf("<assertion set> initial set size is %lu\n", lyst_length(assertionSet));
#endif
		
	/*	Now REMOVE from the assertion set all members of
	 *	the source gateway set of this petition.		*/

	SubtractNodeSets(assertionSet, pet->SourceNodeSet);
#if RAMSDEBUG
printf("<assertion set> set size is %lu after subtraction of SGS\n",
lyst_length(assertionSet));
#endif

	/*	Also REMOVE from the assertion set the sole member
	 *	of the DGS if the petition's DMS is empty and its DGS
	 *	contains only one member.				*/
	
	if (lyst_length(pet->DistributionModuleSet) == 0
	&& lyst_length(pet->DestinationNodeSet) == 1)
	{
		SubtractNodeSets(assertionSet, pet->DestinationNodeSet);
	}

#if RAMSDEBUG
printf("<assertion set> final set size is %lu\n", lyst_length(assertionSet));
#endif
	return assertionSet;
}

void	DeletePetition(Petition *pet)
{
	CHKVOID(pet);
	if (pet->SourceNodeSet)
	{
		lyst_destroy(pet->SourceNodeSet);
	}

	if (pet->DestinationNodeSet)
	{
		lyst_destroy(pet->DestinationNodeSet);
	}

	if (pet->DistributionModuleSet)
	{
		lyst_destroy(pet->DistributionModuleSet);
	}

	if (pet->specification)
	{
		if (pet->specification->envelope)
		{
			MRELEASE(pet->specification->envelope);
		}

		MRELEASE(pet->specification);
	}

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

	CHKZERO(gWay);
	CHKZERO(msg);
	enc = EnvelopeContent(msg, -1);
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

	CHKZERO(gWay);
	CHKZERO(module);
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

static int	SendRPDUviaBp(RamsGateway *gWay, RamsNode *ramsNode,
			unsigned char flowLabel, char* envelope,
			int envelopeLength)
{
	Sdr		sdr = getIonsdr();
	int		classOfService;
	BpAncillaryData	ancillaryData = { 0, 0, 0 };
	Object		extent;
	Object		bundleZco;
	Object		newBundle;
	char		errorMsg[128];

	while (gWay->sap == NULL)
	{
		PUTS("Gateway not registered in network yet.");
		snooze(1);
	}

	classOfService = flowLabel & 0x03;
	ancillaryData.flags = (flowLabel >> 2) & 0x03;
	CHKERR(sdr_begin_xn(sdr));
	extent = sdr_insert(sdr, envelope, envelopeLength);
	if (extent == 0)
	{
		sdr_cancel_xn(sdr);
		ErrMsg("Can't write msg to SDR.");
		return -1;
	}

	bundleZco = ionCreateZco(ZcoSdrSource, extent, 0, envelopeLength,
		classOfService, ancillaryData.ordinal, ZcoOutbound, NULL);
	if (sdr_end_xn(sdr) < 0 || bundleZco == (Object) ERROR
	|| bundleZco == 0)
	{
		ErrMsg("Failed creating message.");
		return -1;
	}

	if (bp_send(gWay->sap, ramsNode->gwEid, "dtn:none", gWay->ttl,
		classOfService, SourceCustodyRequired, 0, 0, &ancillaryData,
		bundleZco, &newBundle) < 1)
	{
		isprintf(errorMsg, sizeof errorMsg,
				"Cannot send message to %s.", ramsNode->gwEid);
		ErrMsg(errorMsg);
		return -1;
	}

	return 0;
}

static int	SendRPDUviaUdp(RamsGateway *gWay, RamsNode *ramsNode,
			unsigned char flowLabel, char *envelope,
			int envelopeLength)
{
	char			gwEid[256];
	unsigned short		portNbr;
	unsigned int		ipAddress;
	struct sockaddr		socketName;
	struct sockaddr_in	*inetName = (struct sockaddr_in *) &socketName;
	char			errorMsg[128];
	UdpRpdu			*rpdu;
	LystElt			elt;

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
		if (sendto(gWay->ownUdpFd, envelope, envelopeLength, 0,
				&socketName, sizeof(struct sockaddr)) < 0)
		{
			if (errno == EINTR)	/*	Interrupted.	*/
			{
				continue;	/*	Retry.		*/
			}

			isprintf(errorMsg, sizeof errorMsg,
				"Cannot send message to %s", ramsNode->gwEid);
			putSysErrmsg(errorMsg, NULL);
			return -1;
		}

		if (NodeSetMember(ramsNode, gWay->declaredNeighbors))
		{
		/*	This neighbor has declared itself, so it has
		 *	been reachable, so we assume that this RPDU
		 *	will reach it.  So no need to try again later.	*/

			return 0;
		}

		/*	This neighbor is not yet declared, so we don't
		 *	know if it's reachable or not.  So let's send
		 *	this RPDU again in another 10 seconds.		*/

		rpdu = (UdpRpdu *) MTAKE(sizeof(UdpRpdu));
		CHKERR(rpdu);
		rpdu->checkTime = time(NULL) + 10;
		rpdu->neighbor = ramsNode;
		rpdu->flowLabel = flowLabel;
		rpdu->envelope = MTAKE(envelopeLength);
		CHKERR(rpdu->envelope);
		memcpy(rpdu->envelope, envelope, envelopeLength);
		rpdu->envelopeLength = envelopeLength;
		elt = lyst_insert(gWay->udpRpdus, rpdu);
		CHKERR(elt);
		return 0;
	}
}

int	SendRPDU(RamsGateway *gWay, int destContinuumNbr,
		unsigned char flowLabel, char *envelope, int envelopeLength)
{
	char		errorMsg[128];
	LystElt		elt;
	RamsNode	*ramsNode;

#if RAMSDEBUG
printf("<SendRPDU> to %d\n", destContinuumNbr);
#endif
	CHKERR(gWay);
	CHKERR(envelope);
	CHKERR(envelopeLength > 0);
	if (destContinuumNbr == 0)	/*	Send to all continua.	*/
	{
#if RAMSDEBUG
PUTS("<SendRPDU> sent to the following continua:");
#endif
		for (elt = lyst_first(gWay->ramsNeighbors);
				elt != NULL; elt = lyst_next(elt))
		{
			ramsNode = (RamsNode *) lyst_data(elt);
			if (ramsNode->continuumNbr ==
					gWay->amsMib->localContinuumNbr)
			{
				continue;
			}
#if RAMSDEBUG
printf("<SendRPDU> to %d envelopeLength=%d cc=%d\n",
ramsNode->continuumNbr, envelopeLength, EnvelopeHeader(envelope,
Env_ControlCode));
#endif
			switch (ramsNode->protocol)
			{
			case RamsBp:
				if (SendRPDUviaBp(gWay, ramsNode,
					flowLabel, envelope, envelopeLength))
				{
					ErrMsg("Can't send RAMS msg via BP.");
					return -1;
				}

				continue;

			case RamsUdp:
				if (SendRPDUviaUdp(gWay, ramsNode,
					flowLabel, envelope, envelopeLength))
				{
					ErrMsg("Can't send RAMS msg via UDP.");
					return -1;
				}

				continue;

			default:
				putErrmsg("Can't send to RAMS node: no network \
protocol.", itoa(ramsNode->continuumNbr));
				return -1;
			}
		}

		return 0;
	}

	/*	This message is being sent to a single continuum.	*/

	ramsNode = Look_Up_DeclaredNeighbor(gWay, destContinuumNbr);
	if (ramsNode == NULL)
	{
		isprintf(errorMsg, sizeof errorMsg, "Continuum %d has not \
declared itself.", destContinuumNbr);
		ErrMsg(errorMsg);
#if RAMSDEBUG
printf("<SendRPDU> continuum %d not declared.\n", destContinuumNbr);
#endif
		return -1;
	}

	switch (ramsNode->protocol)
	{
	case RamsBp:
		return SendRPDUviaBp(gWay, ramsNode, flowLabel, envelope,
				envelopeLength);

	case RamsUdp:
		return SendRPDUviaUdp(gWay, ramsNode, flowLabel, envelope,
				envelopeLength);

	default:
		ErrMsg("No RAMS network protocol.");
		return -1;
	}
}

int	SendNewRPDU(RamsGateway *gWay, int destContinuumNbr,
		unsigned char flowLabel, Enclosure *enclosure,
		int continuumNbr, int unitNbr, int sourceID, int destID,
		int controlCode, int subjectNbr)
{
	char	*envelope;
	int	encLength;
	int	result;

	CHKERR(gWay);
	envelope = NULL; 
	if (enclosure)
	{
		encLength = enclosure->length;
		envelope = (char *) MTAKE(ENVELOPELENGTH + encLength);
		CHKERR(envelope);
		ConstructEnvelope((unsigned char *) envelope, continuumNbr,
				unitNbr, sourceID, destID, subjectNbr,
				enclosure->length, enclosure->text,
				controlCode);
	}
	else 
	{
		encLength = 0;
		envelope = (char *) MTAKE(ENVELOPELENGTH);
		CHKERR(envelope);
		ConstructEnvelope((unsigned char *) envelope, continuumNbr,
				unitNbr, sourceID, destID, subjectNbr, 0, NULL,
				controlCode);
	}

	result = SendRPDU(gWay, destContinuumNbr, flowLabel, envelope,
			ENVELOPELENGTH + encLength);
	MRELEASE(envelope);
	if (result < 0)
	{
		ErrMsg("Failed sending newly constructed RPDU.");
		return -1;
	}

	return 0;
}

void	*CheckUdpRpdus(void *parm)
{
	RamsGateway	*gWay = _gWay(NULL);
	time_t		currentTime;
	LystElt		elt;
	LystElt		nextElt;
	UdpRpdu		*rpdu;

	while (1)
	{
		snooze(1);
		currentTime = time(NULL);
		for (elt = lyst_first(gWay->udpRpdus); elt; elt = nextElt)
		{
			nextElt = lyst_next(elt);
			rpdu = (UdpRpdu	 *) lyst_data(elt);
			if (rpdu->checkTime > currentTime)
			{
				break;	/*	Out of inner loop.	*/
			}

			/*	Time to re-send this RPDU via UDP.	*/

			if (SendRPDUviaUdp(gWay, rpdu->neighbor,
					rpdu->flowLabel, rpdu->envelope,
					rpdu->envelopeLength) < 0)
			{
				ErrMsg("Failed re-sending UDP RPDU.");
				return NULL;
			}

			lyst_delete(elt);
		}
	}
}
