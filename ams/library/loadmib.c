/*
	loadmib.c:	initial implementation of MIB loading
			function for AMS.

	Author: Scott Burleigh, JPL

	Copyright (c) 2005, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
#include "amscommon.h"
#include "expat.h"

typedef enum
{
	LoadDormant = 0,
	LoadInitializing,
	LoadAdding,
	LoadChanging,
	LoadDeleting
} LoadMibOp;

typedef struct
{
	XML_Parser	parser;
	LoadMibOp	currentOperation;
	int		abandoned;
	AmsApp		*app;
	Subject		*subject;
	Venture		*venture;
	void		*target;	/*	For deletion.		*/
} LoadMibState;

static int	crash()
{
	putErrmsg("Loading of test MIB failed.", NULL);
	return -1;
}

static int	loadTestMib()
{
	int	result;
	LystElt	elt;
	AppRole	*role;
	Subject	*subject;
	Venture	*venture;

	result = createMib(1, NULL, 0, "dgr", NULL, 0, NULL, 0);
	if (result < 0)
	{
		return crash();
	}

	elt = createCsEndpoint(NULL, NULL);
       	if (elt == NULL)
	{
		return crash();
	}

	elt = createApp("amsdemo", NULL, 0, NULL, 0);
       	if (elt == NULL)
	{
		return crash();
	}

	venture = createVenture(1, "amsdemo", "test", 0);
	if (venture == NULL)
	{
		return crash();
	}

	role = createRole(venture, 2, "shell", NULL, 0, NULL, 0);
	if (role == NULL)
	{
		return crash();
	}

	role = createRole(venture, 3, "log", NULL, 0, NULL, 0);
	if (role == NULL)
	{
		return crash();
	}

	role = createRole(venture, 4, "pitch", NULL, 0, NULL, 0);
	if (role == NULL)
	{
		return crash();
	}

	role = createRole(venture, 5, "catch", NULL, 0, NULL, 0);
	if (role == NULL)
	{
		return crash();
	}

	subject = createSubject(venture, 1, "text",
			"Arbitrary variable-length text.", NULL, 0);
	if (subject == NULL)
	{
		return crash();
	}

	return 0;
}

static void	noteLoadError(LoadMibState *state, char *text)
{
	char		buf[256];
	XML_Parser	parser = state->parser;

	isprintf(buf, sizeof buf, "[?] MIB load error at line %d of file: %s",
			(int) XML_GetCurrentLineNumber(parser), text);
	writeMemo(buf);
	state->abandoned = 1;
}

static void	handle_load_start(LoadMibState *state, const char **atts)
{
	return;
}

static void	handle_init_start(LoadMibState *state, const char **atts)
{
	int	cnbr = 0;
	char	*gwEid = NULL;
	int	ramsNetIsTree = 0;
	char	*ptsname = NULL;
	char	*pubkey = NULL;
	int	pubkeylen = 0;
	char	*privkey = NULL;
	int	privkeylen = 0;
	char	**att;
	char	*name;
	char	*value;
	int	result;

	if (state->currentOperation != LoadDormant)
	{
		return noteLoadError(state, "Already in an operation.");
	}

	if (mib) return noteLoadError(state, "Already initialized.");
	state->currentOperation = LoadInitializing;
	for (att = (char **) atts; *att; att++)
	{
		name = *att;
		att++;
		value = *att;
		if (strcmp(name, "continuum_nbr") == 0)
		{
			cnbr = atoi(value);
		}
		else if (strcmp(name, "gweid") == 0)
		{
			gwEid = value;
		}
		else if (strcmp(name, "net_config") == 0)
		{
			if (strcmp(value, "tree") == 0)
			{
				ramsNetIsTree = 1;
			}
			else	/*	Only other valid value is mesh.	*/
			{
				ramsNetIsTree = 0;
			}
		}
		else if (strcmp(name, "ptsname") == 0)
		{
			ptsname = value;
		}
		else if (strcmp(name, "pubkey") == 0)
		{
			pubkey = value;
			pubkeylen = strlen(pubkey);
		}
		else if (strcmp(name, "privkey") == 0)
		{
			privkey = value;
			privkeylen = strlen(privkey);
		}
		else return noteLoadError(state, "Unknown attribute.");
	}

	result = createMib(cnbr, gwEid, ramsNetIsTree, ptsname,
			pubkey, pubkeylen, privkey, privkeylen);
	if (result < 0)
	{
		return putErrmsg("Couldn't create MIB.", NULL);
	}
}

static void	handle_op_start(LoadMibState *state, LoadMibOp op)
{
	if (state->currentOperation != LoadDormant)
	{
		return noteLoadError(state, "Already in an operation.");
	}

	state->currentOperation = op;
	state->target = NULL;
}

static void	handle_continuum_start(LoadMibState *state, const char **atts)
{
	int		contnbr = 0;
	char		*contname = NULL;
	char		*gwEid = NULL;
	int		isNeighbor = 1;
	char		*desc = NULL;
	char		**att;
	char		*name;
	char		*value;
	int		idx;
	Continuum	*contin;

	if (mib == NULL) return noteLoadError(state, "MIB not initialized.");
	for (att = (char **) atts; *att; att++)
	{
		name = *att;
		att++;
		value = *att;
		if (strcmp(name, "nbr") == 0)
		{
			contnbr = atoi(value);
		}
		else if (strcmp(name, "name") == 0)
		{
			contname = value;
		}
		else if (strcmp(name, "gweid") == 0)
		{
			gwEid = value;
		}
		else if (strcmp(name, "neighbor") == 0)
		{
			isNeighbor = 1 - (0 == atoi(value));
		}
		else if (strcmp(name, "desc") == 0)
		{
			desc = value;
		}
		else return noteLoadError(state, "Unknown attribute.");
	}

	if (contname == NULL)
	{
		return noteLoadError(state, "Need name of continuum.");
	}

	idx = lookUpContinuum(contname);
	if (idx < 0)
	{
		contin = NULL;
	}
	else
	{
		if (contnbr == 0 || contnbr == idx)
		{
			contin = mib->continua[idx];
		}
		else
		{
			return noteLoadError(state, "Continuum name/nbr \
mismatch.");
		}
	}

	switch (state->currentOperation)
	{
	case LoadAdding:
		if (contin == NULL)
		{
			contin = createContinuum(contnbr, contname, gwEid,
					isNeighbor, desc);
			if (contin == NULL)
			{
				return putErrmsg("Couldn't add continuum.",
						contname);
			}
		}

		break;

	case LoadChanging:
		return noteLoadError(state, "'Change' not yet implemented.");

	case LoadDeleting:
		if (contin == NULL)
		{
			return noteLoadError(state, "No such continuum.");
		}

		return noteLoadError(state, "'Delete' not yet implemented.");

	default:
		return noteLoadError(state, "Not in an operation.");
	}
}

static void	handle_csendpoint_start(LoadMibState *state, const char **atts)
{
	int	after = -1;
	LystElt	elt = NULL;
	int	count;
	char	*epspec = NULL;
	char	**att;
	char	*name;
	char	*value;

	if (mib == NULL) return noteLoadError(state, "MIB not initialized.");
	for (att = (char **) atts; *att; att++)
	{
		name = *att;
		att++;
		value = *att;
		if (strcmp(name, "after") == 0)
		{
			after = atoi(value);
			if (after < 0)
			{
				return noteLoadError(state, "'after' illegal");
			}

			count = after;
			for (elt = lyst_first(mib->csEndpoints); elt;
					elt = lyst_next(elt))
			{
				if (count == 0) break;
				count--;
			}

			if (count > 0)
			{
				return noteLoadError(state, "'after' invalid");
			}
		}
		else if (strcmp(name, "epspec") == 0)
		{
			epspec = value;
		}
		else return noteLoadError(state, "Unknown attribute.");
	}

	switch (state->currentOperation)
	{
	case LoadAdding:
		if (createCsEndpoint(epspec, elt) == NULL)
		{
			return putErrmsg("Couldn't add CS endpoint.", NULL);
		}

		break;

	case LoadChanging:
		return noteLoadError(state, "CS endpoints can only be added \
and deleted.");

	case LoadDeleting:
       		if (elt == NULL)
		{
			return putErrmsg("Couldn't delete CS endpoint.", NULL);
		}

		lyst_delete(elt);
		break;

	default:
		return noteLoadError(state, "Not in an operation.");
	}
}

static void	handle_amsendpoint_start(LoadMibState *state, const char **atts)
{
	char		*tsname = NULL;
	char		*epspec = NULL;
	char		**att;
	char		*name;
	char		*value;
	LystElt		elt;

	if (mib == NULL) return noteLoadError(state, "MIB not initialized.");
	for (att = (char **) atts; *att; att++)
	{
		name = *att;
		att++;
		value = *att;
		if (strcmp(name, "tsname") == 0)
		{
			tsname = value;
		}
		else if (strcmp(name, "epspec") == 0)
		{
			epspec = value;
		}
		else return noteLoadError(state, "Unknown attribute.");
	}

	if (tsname == NULL)
	{
		return noteLoadError(state, "Need name of transport service.");
	}

	if (epspec == NULL)
	{
		return noteLoadError(state, "Need AMS endpoint spec.");
	}

	switch (state->currentOperation)
	{
	case LoadAdding:
		elt = createAmsEpspec(tsname, epspec);
		if (elt == NULL)
		{
			return putErrmsg("Couldn't add AMS endpoint spec.",
					NULL);
		}

		break;

	case LoadChanging:
		return noteLoadError(state, "'Change' not yet implemented.");

	case LoadDeleting:
		return noteLoadError(state, "'Delete' not yet implemented.");

	default:
		return noteLoadError(state, "Not in an operation.");
	}
}

static void	handle_application_start(LoadMibState *state, const char **atts)
{
	char	*appname = NULL;
	char	*pubkey = NULL;
	int	pubkeylen = 0;
	char	*privkey = NULL;
	int	privkeylen = 0;
	char	**att;
	char	*name;
	char	*value;
	LystElt	elt;

	if (mib == NULL) return noteLoadError(state, "MIB not initialized.");
	if (state->app)
	{
		return noteLoadError(state, "Already working on an app.");
	}

	for (att = (char **) atts; *att; att++)
	{
		name = *att;
		att++;
		value = *att;
		if (strcmp(name, "name") == 0)
		{
			appname = value;
		}
		else if (strcmp(name, "pubkey") == 0)
		{
			pubkey = value;
			pubkeylen = strlen(pubkey);
		}
		else if (strcmp(name, "privkey") == 0)
		{
			privkey = value;
			privkeylen = strlen(privkey);
		}
		else return noteLoadError(state, "Unknown attribute.");
	}

	if (appname == NULL)
	{
		return noteLoadError(state, "Need name of application.");
	}

	elt = findApplication(appname);
	if (elt)
	{
		state->app = (AmsApp *) lyst_data(elt);
	}

	switch (state->currentOperation)
	{
	case LoadAdding:
		if (state->app == NULL)
		{
			elt = createApp(appname, pubkey, pubkeylen, privkey,
					privkeylen);
			if (elt == NULL)
			{
				return putErrmsg("Couldn't add application.",
						appname);
			}

			state->app = (AmsApp *) lyst_data(elt);
		}

		break;

	case LoadChanging:
		return noteLoadError(state, "'Change' not yet implemented.");

	case LoadDeleting:
		if (state->app == NULL)
		{
			return noteLoadError(state, "No such application.");
		}

		state->target = elt;	/*	May be deletion target.	*/
		break;

	default:
		return noteLoadError(state, "Not in an operation.");
	}
}

static void	handle_venture_start(LoadMibState *state, const char **atts)
{
	int	msnbr = 0;
	char	*appname = NULL;
	char	*authname = NULL;
	int	rzrsp = 0;
	char	**att;
	char	*name;
	char	*value;

	if (mib == NULL) return noteLoadError(state, "MIB not initialized.");
	if (state->venture)
	{
		return noteLoadError(state, "Already working on a venture.");
	}

	for (att = (char **) atts; *att; att++)
	{
		name = *att;
		att++;
		value = *att;
		if (strcmp(name, "nbr") == 0)
		{
			msnbr = atoi(value);
		}
		else if (strcmp(name, "appname") == 0)
		{
			appname = value;
		}
		else if (strcmp(name, "authname") == 0)
		{
			authname = value;
		}
		else if (strcmp(name, "root_cell_resync_period") == 0)
		{
			rzrsp = atoi(value);
		}
		else return noteLoadError(state, "Unknown attribute.");
	}

	if (appname == NULL)
	{
		return putErrmsg("Need app name for venture.", NULL);
	}


	if (authname == NULL)
	{
		return putErrmsg("Need auth name for venture.", NULL);
	}

	state->venture = lookUpVenture(appname, authname);
	switch (state->currentOperation)
	{
	case LoadAdding:
		if (state->venture == NULL)
		{
			state->venture = createVenture(msnbr, appname,
					authname, rzrsp);
			if (state->venture == NULL)
			{
				return putErrmsg("Couldn't add venture.",
						appname);
			}
		}

		break;

	case LoadChanging:
		return noteLoadError(state, "'Change' not yet implemented.");

	case LoadDeleting:
		if (state->venture == NULL)
		{
			return noteLoadError(state, "No such venture.");
		}

		state->target = state->venture;/*	May be target.	*/
		break;

	default:
		return noteLoadError(state, "Not in an operation.");
	}
}

static void	handle_role_start(LoadMibState *state, const char **atts)
{
	int	rolenbr = 0;
	char	*rolename = NULL;
	char	*pubkey = NULL;
	int	pubkeylen = 0;
	char	*privkey = NULL;
	int	privkeylen = 0;
	char	**att;
	char	*name;
	char	*value;
	AppRole	*role;

	if (mib == NULL) return noteLoadError(state, "MIB not initialized.");
	if (state->venture == NULL)
	{
		return noteLoadError(state, "Venture not specified.");
	}

	for (att = (char **) atts; *att; att++)
	{
		name = *att;
		att++;
		value = *att;
		if (strcmp(name, "nbr") == 0)
		{
			rolenbr = atoi(value);
		}
		else if (strcmp(name, "name") == 0)
		{
			rolename = value;
		}
		else if (strcmp(name, "pubkey") == 0)
		{
			pubkey = value;
			pubkeylen = strlen(pubkey);
		}
		else if (strcmp(name, "privkey") == 0)
		{
			privkey = value;
			privkeylen = strlen(privkey);
		}
		else return noteLoadError(state, "Unknown attribute.");
	}

	if (rolename == NULL)
	{
		return noteLoadError(state, "Need name of role.");
	}

	role = lookUpRole(state->venture, rolename);
	switch (state->currentOperation)
	{
	case LoadAdding:
		if (role == NULL)
		{
			role = createRole(state->venture, rolenbr, rolename,
					pubkey, pubkeylen, privkey, privkeylen);
			if (role == NULL)
			{
				return putErrmsg("Couldn't add role.",
						rolename);
			}
		}
		else
		{
			return noteLoadError(state, "Role already in MIB.");
		}

		break;

	case LoadChanging:
		return noteLoadError(state, "'Change' not yet implemented.");

	case LoadDeleting:
		if (role == NULL)
		{
			return noteLoadError(state, "No such role.");
		}

		state->target = role;	/*	May be deletion target.	*/
		break;

	default:
		return noteLoadError(state, "Not in an operation.");
	}
}

static void	handle_subject_start(LoadMibState *state, const char **atts)
{
	int	subjnbr = 0;
	char	*subjname = NULL;
	char	*desc = NULL;
	char	*symkey = NULL;
	int	symkeylen = 0;
	char	**att;
	char	*name;
	char	*value;

	if (mib == NULL) return noteLoadError(state, "MIB not initialized.");
	if (state->venture == NULL)
	{
		return noteLoadError(state, "Venture not specified.");
	}

	if (state->subject)
	{
		return noteLoadError(state, "Already working on a subject.");
	}

	for (att = (char **) atts; *att; att++)
	{
		name = *att;
		att++;
		value = *att;
		if (strcmp(name, "nbr") == 0)
		{
			subjnbr = atoi(value);
		}
		else if (strcmp(name, "name") == 0)
		{
			subjname = value;
		}
		else if (strcmp(name, "desc") == 0)
		{
			desc = value;
		}
		else if (strcmp(name, "symkey") == 0)
		{
			symkey = value;
			symkeylen = strlen(symkey);
		}
		else return noteLoadError(state, "Unknown attribute.");
	}

	if (subjname == NULL)
	{
		return noteLoadError(state, "Need name of role.");
	}

	state->subject = lookUpSubject(state->venture, subjname);
	switch (state->currentOperation)
	{
	case LoadAdding:
		if (state->subject == NULL)
		{
			state->subject = createSubject(state->venture, subjnbr,
					subjname, desc, symkey, symkeylen);
			if (state->subject == NULL)
			{
				return putErrmsg("Couldn't add subject.",
						subjname);
			}
		}

		break;

	case LoadChanging:
		return noteLoadError(state, "'Change' not yet implemented.");

	case LoadDeleting:
		if (state->subject == NULL)
		{
			return noteLoadError(state, "No such subject.");
		}

		state->target = state->subject;	/*	May be target.	*/
		break;

	default:
		return noteLoadError(state, "Not in an operation.");
	}
}

static void	handle_element_start(LoadMibState *state, const char **atts)
{
	ElementType	type = AmsNoElement;
	char		*ename = NULL;
	char		*desc = NULL;
	char		**att;
	char		*name;
	char		*value;
	LystElt		elt;

	if (mib == NULL) return noteLoadError(state, "MIB not initialized.");
	if (state->subject == NULL)
	{
		return noteLoadError(state, "Subject not specified.");
	}

	for (att = (char **) atts; *att; att++)
	{
		name = *att;
		att++;
		value = *att;
		if (strcmp(name, "type") == 0)
		{
			type = (ElementType) atoi(value);
		}
		else if (strcmp(name, "name") == 0)
		{
			ename = value;
		}
		else if (strcmp(name, "desc") == 0)
		{
			desc = value;
		}
		else return noteLoadError(state, "Unknown attribute.");
	}

	if (ename == NULL)
	{
		return noteLoadError(state, "Need name of element.");
	}

	elt = findElement(state->subject, ename);
	switch (state->currentOperation)
	{
	case LoadAdding:
		if (elt == NULL)
		{
			elt = createElement(state->subject, ename, type, desc);
			if (elt == NULL)
			{
				return putErrmsg("Couldn't add element.",
						ename);
			}
		}
		else
		{
			return noteLoadError(state, "Element already in MIB.");
		}

		break;

	case LoadChanging:
		return noteLoadError(state, "'Change' not yet implemented.");

	case LoadDeleting:
		if (elt == NULL)
		{
			return noteLoadError(state, "No such element.");
		}

		state->target = elt;	/*	May be deletion target.	*/
		break;

	default:
		return noteLoadError(state, "Not in an operation.");
	}
}

static void	handle_unit_start(LoadMibState *state, const char **atts)
{
	int	znbr = 0;
	char	*zname = NULL;
	int	rsp = 0;
	char	**att;
	char	*name;
	char	*value;
	Unit	*unit;

	if (mib == NULL) return noteLoadError(state, "MIB not initialized.");
	if (state->venture == NULL)
	{
		return noteLoadError(state, "Venture not specified.");
	}

	for (att = (char **) atts; *att; att++)
	{
		name = *att;
		att++;
		value = *att;
		if (strcmp(name, "nbr") == 0)
		{
			znbr = atoi(value);
		}
		else if (strcmp(name, "name") == 0)
		{
			zname = value;
		}
		else if (strcmp(name, "resync_period") == 0)
		{
			rsp = atoi(value);
		}
		else return noteLoadError(state, "Unknown attribute.");
	}

	if (zname == NULL)
	{
		return noteLoadError(state, "Need name of unit.");
	}

	unit = lookUpUnit(state->venture, zname);
	switch (state->currentOperation)
	{
	case LoadAdding:
		if (unit == NULL)
		{
			unit = createUnit(state->venture, znbr, zname, rsp);
			if (unit == NULL)
			{
				return putErrmsg("Couldn't add unit.", zname);
			}
		}
		else
		{
			return noteLoadError(state, "Unit already in MIB.");
		}

		break;

	case LoadChanging:
		return noteLoadError(state, "'Change' not yet implemented.");

	case LoadDeleting:
		if (unit == NULL)
		{
			return noteLoadError(state, "No such unit.");
		}

		state->target = unit;	/*	May be deletion target.	*/
		break;

	default:
		return noteLoadError(state, "Not in an operation.");
	}
}

static void	handle_msgspace_start(LoadMibState *state, const char **atts)
{
	int		contnbr = 0;
	char		*symkey = NULL;
	int		symkeylen = 0;
	char		**att;
	char		*name;
	char		*value;
	Continuum	*contin;
	Subject		*msgspace;

	if (mib == NULL) return noteLoadError(state, "MIB not initialized.");
	if (state->venture == NULL)
	{
		return noteLoadError(state, "Venture not specified.");
	}

	if (state->subject)
	{
		return noteLoadError(state, "Already working on a msgspace.");
	}

	for (att = (char **) atts; *att; att++)
	{
		name = *att;
		att++;
		value = *att;
		if (strcmp(name, "nbr") == 0)
		{
			contnbr = atoi(value);
		}
		else if (strcmp(name, "symkey") == 0)
		{
			symkey = value;
			symkeylen = strlen(symkey);
		}
		else return noteLoadError(state, "Unknown attribute.");
	}

	if (contnbr < 1 || contnbr > MaxContinNbr)
	{
		return noteLoadError(state, "Need number of continuum.");
	}

	contin = mib->continua[contnbr];
	if (contin == NULL)
	{
		return noteLoadError(state, "Unknown continuum.");
	}

	msgspace = state->venture->msgspaces[contnbr];
	switch (state->currentOperation)
	{
	case LoadAdding:
		if (msgspace == NULL)
		{
			msgspace = createMsgspace(state->venture, contnbr,
					symkey, symkeylen);
			if (msgspace == NULL)
			{
				return putErrmsg("Couldn't add msgspace.",
						contin->name);
			}
		}

		break;

	case LoadChanging:
		return noteLoadError(state, "'Change' not yet implemented.");

	case LoadDeleting:
		if (msgspace == NULL)
		{
			return noteLoadError(state, "No such continuum.");
		}

		state->target = msgspace;	/*	May be target.	*/
		break;

	default:
		return noteLoadError(state, "Not in an operation.");
	}
}

static void XMLCALL	startElement(void *userData, const char *name,
				const char **atts)
{
	LoadMibState	*state = (LoadMibState *) userData;

	if (strcmp(name, "ams_mib_load") == 0)
	{
		return handle_load_start(state, atts);
	}

	if (strcmp(name, "ams_mib_init") == 0)
	{
		return handle_init_start(state, atts);
	}

	if (strcmp(name, "ams_mib_add") == 0)
	{
		return handle_op_start(state, LoadAdding);
	}

	if (strcmp(name, "ams_mib_change") == 0)
	{
		return handle_op_start(state, LoadChanging);
	}

	if (strcmp(name, "ams_mib_delete") == 0)
	{
		return handle_op_start(state, LoadDeleting);
	}

	if (strcmp(name, "continuum") == 0)
	{
		return handle_continuum_start(state, atts);
	}

	if (strcmp(name, "csendpoint") == 0)
	{
		return handle_csendpoint_start(state, atts);
	}

	if (strcmp(name, "amsendpoint") == 0)
	{
		return handle_amsendpoint_start(state, atts);
	}

	if (strcmp(name, "application") == 0)
	{
		return handle_application_start(state, atts);
	}

	if (strcmp(name, "venture") == 0)
	{
		return handle_venture_start(state, atts);
	}

	if (strcmp(name, "role") == 0)
	{
		return handle_role_start(state, atts);
	}

	if (strcmp(name, "subject") == 0)
	{
		return handle_subject_start(state, atts);
	}

	if (strcmp(name, "element") == 0)
	{
		return handle_element_start(state, atts);
	}

	if (strcmp(name, "unit") == 0)
	{
		return handle_unit_start(state, atts);
	}

	if (strcmp(name, "msgspace") == 0)
	{
		return handle_msgspace_start(state, atts);
	}

	noteLoadError(state, "Unknown element name.");
}

static void	handle_load_end(LoadMibState *state)
{
	return;
}

static void	handle_init_end(LoadMibState *state)
{
	state->currentOperation = LoadDormant;
}

static void	handle_op_end(LoadMibState *state, LoadMibOp op)
{
	state->currentOperation = LoadDormant;
}

static void	handle_continuum_end(LoadMibState *state)
{
	return;
}

static void	handle_csendpoint_end(LoadMibState *state)
{
	return;
}

static void	handle_amsendpoint_end(LoadMibState *state)
{
	return;
}

static void	handle_application_end(LoadMibState *state)
{
	if (state->target)	/*	Application is deletion target.	*/
	{
		lyst_delete((LystElt) (state->target));
		state->target = NULL;
	}

	state->app = NULL;
}

static void	handle_role_end(LoadMibState *state)
{
	if (state->target)	/*	Role is deletion target.	*/
	{
		eraseRole(state->venture, (AppRole *) (state->target));
		state->target = NULL;
	}
}

static void	handle_subject_end(LoadMibState *state)
{
	if (state->target)	/*	Subject is deletion target.	*/
	{
		eraseSubject(state->venture, (Subject *) (state->target));
		state->target = NULL;
	}

	state->subject = NULL;
}

static void	handle_element_end(LoadMibState *state)
{
	if (state->target)	/*	Element is deletion target.	*/
	{
		lyst_delete((LystElt) (state->target));
		state->target = NULL;
	}
}

static void	handle_venture_end(LoadMibState *state)
{
	if (state->target)	/*	Venture is deletion target.	*/
	{
		eraseVenture((Venture *) (state->target));
		state->target = NULL;
	}

	state->venture = NULL;
}

static void	handle_unit_end(LoadMibState *state)
{
	if (state->target)	/*	Unit is deletion target.	*/
	{
		eraseUnit(state->venture, (Unit *) (state->target));
		state->target = NULL;
	}
}

static void	handle_msgspace_end(LoadMibState *state)
{
	if (state->target)	/*	Msgspace is deletion target.	*/
	{
		eraseMsgspace(state->venture, (Subject *) (state->target));
		state->target = NULL;
	}
}

static void XMLCALL	endElement(void *userData, const char *name)
{
	LoadMibState	*state = (LoadMibState *) userData;

	if (strcmp(name, "ams_mib_load") == 0)
	{
		return handle_load_end(state);
	}

	if (strcmp(name, "ams_mib_init") == 0)
	{
		return handle_init_end(state);
	}

	if (strcmp(name, "ams_mib_add") == 0)
	{
		return handle_op_end(state, LoadAdding);
	}

	if (strcmp(name, "ams_mib_change") == 0)
	{
		return handle_op_end(state, LoadChanging);
	}

	if (strcmp(name, "ams_mib_delete") == 0)
	{
		return handle_op_end(state, LoadDeleting);
	}

	if (strcmp(name, "continuum") == 0)
	{
		return handle_continuum_end(state);
	}

	if (strcmp(name, "csendpoint") == 0)
	{
		return handle_csendpoint_end(state);
	}

	if (strcmp(name, "amsendpoint") == 0)
	{
		return handle_amsendpoint_end(state);
	}

	if (strcmp(name, "application") == 0)
	{
		return handle_application_end(state);
	}

	if (strcmp(name, "role") == 0)
	{
		return handle_role_end(state);
	}

	if (strcmp(name, "subject") == 0)
	{
		return handle_subject_end(state);
	}

	if (strcmp(name, "element") == 0)
	{
		return handle_element_end(state);
	}

	if (strcmp(name, "venture") == 0)
	{
		return handle_venture_end(state);
	}

	if (strcmp(name, "unit") == 0)
	{
		return handle_unit_end(state);
	}

	if (strcmp(name, "msgspace") == 0)
	{
		return handle_msgspace_end(state);
	}

	noteLoadError(state, "Unknown element name.");
}

static int	loadMibFromSource(char *mibSource)
{
	FILE		*sourceFile;
	LoadMibState	state;
	char		buf[256];
	int		done = 0;
	size_t		length;
	int		result = 0;

	sourceFile = fopen(mibSource, "r");
	if (sourceFile == NULL)
	{
		putSysErrmsg("Can't open MIB source file", mibSource);
		return -1;
	}

	memset((char *) &state, 0, sizeof state);
	state.abandoned = 0;
	state.currentOperation = LoadDormant;
	state.parser = XML_ParserCreate(NULL);
	if (state.parser == NULL)
	{
		putSysErrmsg("Can't open XML parser", NULL);
		fclose(sourceFile);
		return -1;
	}

	XML_SetElementHandler(state.parser, startElement, endElement);
	XML_SetUserData(state.parser, &state);
	while (!done)
	{
		length = fread(buf, 1, sizeof(buf), sourceFile);
		done = length < sizeof buf;
		if (XML_Parse(state.parser, buf, length, done)
				== XML_STATUS_ERROR)
		{
			isprintf(buf, sizeof buf, "XML error at line %d.", (int)
					XML_GetCurrentLineNumber(state.parser));
			putSysErrmsg(buf, XML_ErrorString
					(XML_GetErrorCode(state.parser)));
			result = -1;
			break;	/*	Out of loop.			*/
		}

		if (state.abandoned)
		{
			writeMemo("[?] Abandoning MIB load.");
			result = -1;
			break;	/*	Out of loop.			*/
		}
	}

	XML_ParserFree(state.parser);
	fclose(sourceFile);
	return result;
}

int	loadMib(char *mibSource)
{
	int		result;
	int		i;
	TransSvc	*ts;

	if (mibSource == NULL)
	{
		result = loadTestMib();
	}
	else
	{
		result = loadMibFromSource(mibSource);
	}

	if (result < 0)
	{
		return result;
	}

	if (lyst_length(mib->amsEndpointSpecs) == 0)
	{
		for (i = 0, ts = mib->transportServices;
				i < mib->transportServiceCount; i++, ts++)
		{
			if (createAmsEpspec(ts->name, "@") < 0)
			{
				putErrmsg("Can't load default AMS endpoint \
specs.", NULL);
				return -1;
			}
		}
	}

	return 0;
}
