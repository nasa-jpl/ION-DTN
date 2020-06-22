/*

	bibe.c:	API for bundle-in-bundle encapsulation.

	Author:	Scott Burleigh, JPL

	Copyright (c) 2019, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

									*/
#include "bibeP.h"

static void	getSchemeName(char *eid, char *schemeNameBuf)
{
	MetaEid		meid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;

	if (parseEidString(eid, &meid, &vscheme, &vschemeElt) == 0)
	{
		*schemeNameBuf = '\0';
	}
	else
	{
		istrcpy(schemeNameBuf, meid.schemeName, MAX_SCHEME_NAME_LEN);
		restoreEidString(&meid);
	}
}

void	bibeAdd(char *peerEid, int lifespan, unsigned char priority,
		unsigned char ordinal, unsigned int label, unsigned char flags)
{
	Sdr		sdr = getIonsdr();
	Object		bclaAddr;
	Object		bclaElt;
	char		schemeName[MAX_SCHEME_NAME_LEN + 1];
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	Scheme		scheme;
	Bcla		bcla;

	bibeFind(peerEid, &bclaAddr, &bclaElt);
	if (bclaElt)
	{
		writeMemoNote("[?] Duplicate BIBE CLA", peerEid);
		return;
	}

	getSchemeName(peerEid, schemeName);
	if (schemeName[0] == '\0')
	{
		writeMemoNote("[?] No scheme name in bcla ID", peerEid);
		return;
	}

	findScheme(schemeName, &vscheme, &vschemeElt);
	if (vscheme == NULL)
	{
		writeMemoNote("[?] bcla ID scheme name unknown", peerEid);
		return;
	}

	sdr_read(sdr, (char *) &scheme, sdr_list_data(sdr, vscheme->schemeElt),
			sizeof(Scheme));
	CHKVOID(sdr_begin_xn(sdr));
	memset((char *) &bcla, 0, sizeof(Bcla));
	bcla.source = sdr_string_create(sdr, vscheme->adminEid);
	bcla.dest = sdr_string_create(sdr, peerEid);
	bcla.lifespan = lifespan;
	bcla.classOfService = priority;
	if (flags & BP_DATA_LABEL_PRESENT)
	{
		bcla.ancillaryData.dataLabel = label;
	}

	bcla.ancillaryData.flags = flags;
	bcla.ancillaryData.ordinal = ordinal;
	bclaAddr = sdr_malloc(sdr, sizeof(Bcla));
	if (bclaAddr)
	{
		sdr_write(sdr, bclaAddr, (char *) &bcla, sizeof(Bcla));
		oK(sdr_list_insert_last(sdr, scheme.bclas, bclaAddr));
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Failed adding bcla.", peerEid);
	}
}

void	bibeChange(char *peerEid, int lifespan, unsigned char priority,
		unsigned char ordinal, unsigned int label, unsigned char flags)
{
	Sdr		sdr = getIonsdr();
	Object		bclaAddr;
	Object		bclaElt;
	Bcla		bcla;

	bibeFind(peerEid, &bclaAddr, &bclaElt);
	if (bclaElt == 0)
	{
		writeMemoNote("[?] Can't find BIBE CLA to change", peerEid);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &bcla, bclaAddr, sizeof(Bcla));
	bcla.lifespan = lifespan;
	bcla.classOfService = priority;
	if (flags & BP_DATA_LABEL_PRESENT)
	{
		bcla.ancillaryData.dataLabel = label;
	}

	bcla.ancillaryData.flags = flags;
	bcla.ancillaryData.ordinal = ordinal;
	sdr_write(sdr, bclaAddr, (char *) &bcla, sizeof(Bcla));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Failed changing bcla.", peerEid);
	}
}

void	bibeDelete(char *peerEid)
{
	Sdr	sdr = getIonsdr();
	Object	bclaAddr;
	Object	bclaElt;
	Bcla	bcla;

	bibeFind(peerEid, &bclaAddr, &bclaElt);
	if (bclaElt == 0)
	{
		writeMemoNote("[?] Can't find BIBE CLA to delete", peerEid);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &bcla, bclaAddr, sizeof(Bcla));
	sdr_free(sdr, bcla.dest);
	sdr_free(sdr, bclaAddr);
	sdr_list_delete(sdr, bclaElt, NULL, NULL);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Failed deleting bcla.", peerEid);
	}
}

void	bibeFind(char *peerEid, Object *bclaAddr, Object *bclaElt)
{
	Sdr		sdr = getIonsdr();
	char		schemeName[MAX_SCHEME_NAME_LEN + 1];
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	Scheme		scheme;
	Object		elt;
	Object		addr;
			OBJ_POINTER(Bcla, bcla);
	char		dest[SDRSTRING_BUFSZ];

	CHKVOID(peerEid && bclaAddr && bclaElt);
	*bclaAddr = 0;
	*bclaElt = 0;
	getSchemeName(peerEid, schemeName);
	findScheme(schemeName, &vscheme, &vschemeElt);
	if (vscheme == NULL)
	{
		return;
	}

	sdr_read(sdr, (char *) &scheme, sdr_list_data(sdr, vscheme->schemeElt),
			sizeof(Scheme));
	if (scheme.bclas == 0)
	{
		return;
	}

	oK(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, scheme.bclas); elt;
			elt = sdr_list_next(sdr, elt))
	{
		addr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, Bcla, bcla, addr);
		CHKVOID(bcla);
		if (sdr_string_read(sdr, dest, bcla->dest) < 0)
		{
			continue;
		}

		if (strcmp(dest, peerEid) == 0)
		{
			*bclaAddr = addr;
			*bclaElt = elt;
			break;
		}

	}

	sdr_exit_xn(sdr);
}
