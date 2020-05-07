/*
	loadmib.c:	initial implementation of MIB loading
			function for AMS.

	Author: Scott Burleigh, JPL

	Copyright (c) 2005, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
#include "amscommon.h"
#ifndef NOEXPAT
#include "expat.h"
#endif

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
#ifdef NOEXPAT
	int		lineNbr;
#else
	XML_Parser	parser;
#endif
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
	AmsMibParameters	parms = { 1, "dgr", NULL, NULL };
	AmsMib			*mib;
	char			ownHostName[MAXHOSTNAMELEN + 1];
	char			eps[MAXHOSTNAMELEN + 5 + 1];
	LystElt			elt;
	Venture			*venture;
	AppRole			*role;
	Subject			*subject;

	mib = _mib(&parms);
	if (mib == NULL)
	{
		return crash();
	}

	getNameOfHost(ownHostName, sizeof ownHostName);
	isprintf(eps, sizeof eps, "%s:2357", ownHostName);
	elt = createCsEndpoint(eps, NULL);
       	if (elt == NULL)
	{
		return crash();
	}

	elt = createApp("amsdemo", NULL, NULL);
       	if (elt == NULL)
	{
		return crash();
	}

	venture = createVenture(1, "amsdemo", "test", NULL, 0, 0);
	if (venture == NULL)
	{
		return crash();
	}

	role = createRole(venture, 2, "shell", NULL, NULL);
	if (role == NULL)
	{
		return crash();
	}

	role = createRole(venture, 3, "log", NULL, NULL);
	if (role == NULL)
	{
		return crash();
	}

	role = createRole(venture, 4, "pitch", NULL, NULL);
	if (role == NULL)
	{
		return crash();
	}

	role = createRole(venture, 5, "catch", NULL, NULL);
	if (role == NULL)
	{
		return crash();
	}

	role = createRole(venture, 96, "amsd", NULL, NULL);
	if (role == NULL)
	{
		return crash();
	}

	role = createRole(venture, 97, "amsstop", NULL, NULL);
	if (role == NULL)
	{
		return crash();
	}

	role = createRole(venture, 98, "amsmib", NULL, NULL);
	if (role == NULL)
	{
		return crash();
	}

	subject = createSubject(venture, 1, "text",
			"Arbitrary variable-length text.", NULL, NULL, NULL);
	if (subject == NULL)
	{
		return crash();
	}

	subject = createSubject(venture, 97, "amsstop",
			"Message space shutdown command.", NULL, NULL, NULL);
	if (subject == NULL)
	{
		return crash();
	}

	subject = createSubject(venture, 98, "amsmib",
			"Runtime MIB updates.", NULL, NULL, NULL);
	if (subject == NULL)
	{
		return crash();
	}

	return 0;
}

static void	noteLoadError(LoadMibState *state, char *text)
{
	char		buf[256];
#ifdef NOEXPAT
	isprintf(buf, sizeof buf, "[?] MIB load error at line %d of file: %s",
			state->lineNbr, text);
#else
	XML_Parser	parser = state->parser;

	isprintf(buf, sizeof buf, "[?] MIB load error at line %d of file: %s",
			(int) XML_GetCurrentLineNumber(parser), text);
#endif
	writeMemo(buf);
	state->abandoned = 1;
}

static int	noMibYet(LoadMibState *state)
{
	if (_mib(NULL) == NULL)
	{
		noteLoadError(state, "MIB not initialized.");
		return 1;
	}

	return 0;
}

static void	handle_load_start(LoadMibState *state, const char **atts)
{
	return;
}

static void	handle_init_start(LoadMibState *state, const char **atts)
{
	AmsMib			*mib;
	int			cnbr = 0;
	char			*ptsname = NULL;
	char			*pubkeyname = NULL;
	char			*privkeyname = NULL;
	char			**att;
	char			*name;
	char			*value;
	AmsMibParameters	parms;

	if (state->currentOperation != LoadDormant)
	{
		noteLoadError(state, "Already in an operation.");
		return;
	}

	mib = _mib(NULL);
	if (mib)
	{
		noteLoadError(state, "Already initialized.");
		return;
	}

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
		else if (strcmp(name, "ptsname") == 0)
		{
			ptsname = value;
		}
		else if (strcmp(name, "pubkey") == 0)
		{
			pubkeyname = value;
		}
		else if (strcmp(name, "privkey") == 0)
		{
			privkeyname = value;
		}
		else
		{
			noteLoadError(state, "Unknown attribute.");
			return;
		}
	}

	parms.continuumNbr = cnbr;
	parms.ptsName = ptsname;
	parms.publicKeyName = pubkeyname;
	parms.privateKeyName = privkeyname;
	mib = _mib(&parms);
	if (mib == NULL)
	{
		return putErrmsg("Couldn't create MIB.", NULL);
	}
}

static void	handle_op_start(LoadMibState *state, LoadMibOp op)
{
	if (state->currentOperation != LoadDormant)
	{
		noteLoadError(state, "Already in an operation.");
		return;
	}

	state->currentOperation = op;
	state->target = NULL;
}

static void	handle_continuum_start(LoadMibState *state, const char **atts)
{
	int		contnbr = 0;
	char		*contname = NULL;
	char		*desc = NULL;
	char		**att;
	char		*name;
	char		*value;
	int		idx;
	Continuum	*contin;

	if (noMibYet(state)) return;
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
		else if (strcmp(name, "desc") == 0)
		{
			desc = value;
		}
		else
		{	
			noteLoadError(state, "Unknown attribute.");
			return;
		}
	}

	if (contname == NULL)
	{
		noteLoadError(state, "Need name of continuum.");
		return;
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
			contin = (_mib(NULL))->continua[idx];
		}
		else
		{
			noteLoadError(state, "Continuum name/nbr mismatch.");
			return;
		}
	}

	switch (state->currentOperation)
	{
	case LoadAdding:
		if (contin == NULL)
		{
			contin = createContinuum(contnbr, contname, desc);
			if (contin == NULL)
			{
				return putErrmsg("Couldn't add continuum.",
						contname);
			}
		}

		break;

	case LoadChanging:
		noteLoadError(state, "'Change' not yet implemented.");
		return;

	case LoadDeleting:
		if (contin == NULL)
		{
			noteLoadError(state, "No such continuum.");
			return;
		}

		noteLoadError(state, "'Delete' not yet implemented.");
		return;

	default:
		noteLoadError(state, "Not in an operation.");
		return;
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

	if (noMibYet(state)) return;
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
				noteLoadError(state, "'after' illegal");
				return;
			}

			count = after;
			for (elt = lyst_first((_mib(NULL))->csEndpoints); elt;
					elt = lyst_next(elt))
			{
				if (count == 0) break;
				count--;
			}

			if (count > 0)
			{
				noteLoadError(state, "'after' invalid");
				return;
			}
		}
		else if (strcmp(name, "epspec") == 0)
		{
			epspec = value;
		}
		else
		{
			noteLoadError(state, "Unknown attribute.");
			return;
		}
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
		noteLoadError(state, "CS endpoints can only be added \
and deleted.");
		return;

	case LoadDeleting:
       		if (elt == NULL)
		{
			return putErrmsg("Couldn't delete CS endpoint.", NULL);
		}

		lyst_delete(elt);
		break;

	default:
		noteLoadError(state, "Not in an operation.");
		return;
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

	if (noMibYet(state)) return;
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
		else
		{
			noteLoadError(state, "Unknown attribute.");
			return;
		}
	}

	if (tsname == NULL)
	{
		noteLoadError(state, "Need name of transport service.");
		return;
	}

	if (epspec == NULL)
	{
		noteLoadError(state, "Need AMS endpoint spec.");
		return;
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
		noteLoadError(state, "'Change' not yet implemented.");
		return;

	case LoadDeleting:
		noteLoadError(state, "'Delete' not yet implemented.");
		return;

	default:
		noteLoadError(state, "Not in an operation.");
		return;
	}
}

static void	handle_application_start(LoadMibState *state, const char **atts)
{
	char	*appname = NULL;
	char	*pubkeyname = NULL;
	char	*privkeyname = NULL;
	char	**att;
	char	*name;
	char	*value;
	LystElt	elt;

	if (noMibYet(state)) return;
	if (state->app)
	{
		noteLoadError(state, "Already working on an app.");
		return;
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
			pubkeyname = value;
		}
		else if (strcmp(name, "privkey") == 0)
		{
			privkeyname = value;
		}
		else
		{
			noteLoadError(state, "Unknown attribute.");
			return;
		}
	}

	if (appname == NULL)
	{
		noteLoadError(state, "Need name of application.");
		return;
	}

	state->app = lookUpApplication(appname);
	switch (state->currentOperation)
	{
	case LoadAdding:
		if (state->app == NULL)
		{
			elt = createApp(appname, pubkeyname, privkeyname);
			if (elt == NULL)
			{
				return putErrmsg("Couldn't add application.",
						appname);
			}

			state->app = (AmsApp *) lyst_data(elt);
		}

		break;

	case LoadChanging:
		noteLoadError(state, "'Change' not yet implemented.");
		return;

	case LoadDeleting:
		if (state->app == NULL)
		{
			noteLoadError(state, "No such application.");
			return;
		}

		state->target = state->app;	/*	May be target.	*/
		break;

	default:
		break;				/*	Just context.	*/
	}
}

static void	handle_venture_start(LoadMibState *state, const char **atts)
{
	int	vnbr = 0;
	char	*appname = NULL;
	char	*authname = NULL;
	char	*gwEid = NULL;
	int	ramsNetIsTree = 0;
	int	rzrsp = 0;
	char	**att;
	char	*name;
	char	*value;

	if (noMibYet(state)) return;
	if (state->venture)
	{
		noteLoadError(state, "Already working on a venture.");
		return;
	}

	for (att = (char **) atts; *att; att++)
	{
		name = *att;
		att++;
		value = *att;
		if (strcmp(name, "nbr") == 0)
		{
			vnbr = atoi(value);
		}
		else if (strcmp(name, "appname") == 0)
		{
			appname = value;
		}
		else if (strcmp(name, "authname") == 0)
		{
			authname = value;
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
		else if (strcmp(name, "root_cell_resync_period") == 0)
		{
			rzrsp = atoi(value);
		}
		else
		{
			noteLoadError(state, "Unknown attribute.");
			return;
		}
	}

	if (appname == NULL)
	{
		return writeMemo("[?] Need app name for venture.");
	}

	if (authname == NULL)
	{
		return writeMemo("[?] Need auth name for venture.");
	}

	state->venture = lookUpVenture(appname, authname);
	switch (state->currentOperation)
	{
	case LoadAdding:
		if (state->venture == NULL)
		{
			state->venture = createVenture(vnbr, appname,
					authname, gwEid, ramsNetIsTree, rzrsp);
			if (state->venture == NULL)
			{
				return putErrmsg("Couldn't add venture.",
						appname);
			}
		}

		break;

	case LoadChanging:
		noteLoadError(state, "'Change' not yet implemented.");
		return;

	case LoadDeleting:
		if (state->venture == NULL)
		{
			noteLoadError(state, "No such venture.");
			return;
		}

		state->target = state->venture;/*	May be target.	*/
		break;

	default:
		break;				/*	Just context.	*/
	}
}

static void	handle_role_start(LoadMibState *state, const char **atts)
{
	int	rolenbr = 0;
	char	*rolename = NULL;
	char	*pubkeyname = NULL;
	char	*privkeyname = NULL;
	char	**att;
	char	*name;
	char	*value;
	AppRole	*role;

	if (noMibYet(state)) return;
	if (state->venture == NULL)
	{
		noteLoadError(state, "Venture not specified.");
		return;
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
			pubkeyname = value;
		}
		else if (strcmp(name, "privkey") == 0)
		{
			privkeyname = value;
		}
		else
		{
			noteLoadError(state, "Unknown attribute.");
			return;
		}
	}

	if (rolename == NULL)
	{
		noteLoadError(state, "Need name of role.");
		return;
	}

	role = lookUpRole(state->venture, rolename);
	switch (state->currentOperation)
	{
	case LoadAdding:
		if (role == NULL)
		{
			role = createRole(state->venture, rolenbr, rolename,
					pubkeyname, privkeyname);
			if (role == NULL)
			{
				return putErrmsg("Couldn't add role.",
						rolename);
			}
		}
		else
		{
			noteLoadError(state, "Role already in MIB.");
			return;
		}

		break;

	case LoadChanging:
		noteLoadError(state, "'Change' not yet implemented.");
		return;

	case LoadDeleting:
		if (role == NULL)
		{
			noteLoadError(state, "No such role.");
			return;
		}

		state->target = role;	/*	May be deletion target.	*/
		break;

	default:
		noteLoadError(state, "Not in an operation.");
		return;
	}
}

static void	handle_subject_start(LoadMibState *state, const char **atts)
{
	int	subjnbr = 0;
	char	*subjname = NULL;
	char	*desc = NULL;
	char	*symkeyname = NULL;
	char	*marshalfnname = NULL;
	char	*unmarshalfnname = NULL;
	char	**att;
	char	*name;
	char	*value;

	if (noMibYet(state)) return;
	if (state->venture == NULL)
	{
		noteLoadError(state, "Venture not specified.");
		return;
	}

	if (state->subject)
	{
		noteLoadError(state, "Already working on a subject.");
		return;
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
			symkeyname = value;
		}
		else if (strcmp(name, "marshal") == 0)
		{
			marshalfnname = value;
		}
		else if (strcmp(name, "unmarshal") == 0)
		{
			unmarshalfnname = value;
		}
		else
		{
			noteLoadError(state, "Unknown attribute.");
			return;
		}
	}

	if (subjname == NULL)
	{
		noteLoadError(state, "Need name of subject.");
		return;
	}

	state->subject = lookUpSubject(state->venture, subjname);
	switch (state->currentOperation)
	{
	case LoadAdding:
		if (state->subject == NULL)
		{
			state->subject = createSubject(state->venture, subjnbr,
					subjname, desc, symkeyname,
					marshalfnname, unmarshalfnname);
			if (state->subject == NULL)
			{
				return putErrmsg("Couldn't add subject.",
						subjname);
			}
		}

		break;

	case LoadChanging:
		noteLoadError(state, "'Change' not yet implemented.");
		return;

	case LoadDeleting:
		if (state->subject == NULL)
		{
			noteLoadError(state, "No such subject.");
			return;
		}

		state->target = state->subject;	/*	May be target.	*/
		break;

	default:
		break;				/*	Just context.	*/
	}
}

static void	handle_sender_start(LoadMibState *state, const char **atts)
{
	char	*rolename = NULL;
	char	**att;
	char	*name;
	char	*value;

	if (noMibYet(state)) return;
	if (state->subject == NULL)
	{
		noteLoadError(state, "Subject not specified.");
		return;
	}

	for (att = (char **) atts; *att; att++)
	{
		name = *att;
		att++;
		value = *att;
		if (strcmp(name, "name") == 0)
		{
			rolename = value;
		}
		else
		{
			noteLoadError(state, "Unknown attribute.");
			return;
		}
	}

	if (rolename == NULL)
	{
		noteLoadError(state, "Need role name of sender.");
		return;
	}

	switch (state->currentOperation)
	{
	case LoadAdding:
		if (addAuthorizedSender(state->venture, state->subject,
				rolename) < 0)
		{
			return putErrmsg("Couldn't add authorized sender.",
						rolename);
		}

		break;

	case LoadChanging:
		noteLoadError(state, "'Change' not applicable.");
		return;

	case LoadDeleting:
		state->target = rolename;	/*	May be target.	*/
		break;

	default:
		noteLoadError(state, "Not in an operation.");
		return;
	}
}

static void	handle_receiver_start(LoadMibState *state, const char **atts)
{
	char	*rolename = NULL;
	char	**att;
	char	*name;
	char	*value;

	if (noMibYet(state)) return;
	if (state->subject == NULL)
	{
		noteLoadError(state, "Subject not specified.");
		return;
	}

	for (att = (char **) atts; *att; att++)
	{
		name = *att;
		att++;
		value = *att;
		if (strcmp(name, "name") == 0)
		{
			rolename = value;
		}
		else
		{
			noteLoadError(state, "Unknown attribute.");
			return;
		}
	}

	if (rolename == NULL)
	{
		noteLoadError(state, "Need role name of receiver.");
		return;
	}

	switch (state->currentOperation)
	{
	case LoadAdding:
		if (addAuthorizedReceiver(state->venture, state->subject,
				rolename) < 0)
		{
			return putErrmsg("Couldn't add authorized receiver.",
						rolename);
		}

		break;

	case LoadChanging:
		noteLoadError(state, "'Change' not applicable.");
		return;

	case LoadDeleting:
		state->target = rolename;	/*	May be target.	*/
		break;

	default:
		noteLoadError(state, "Not in an operation.");
		return;
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

	if (noMibYet(state)) return;
	if (state->venture == NULL)
	{
		noteLoadError(state, "Venture not specified.");
		return;
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
		else
		{
			noteLoadError(state, "Unknown attribute.");
			return;
		}
	}

	if (zname == NULL)
	{
		noteLoadError(state, "Need name of unit.");
		return;
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
			noteLoadError(state, "Unit already in MIB.");
			return;
		}

		break;

	case LoadChanging:
		noteLoadError(state, "'Change' not yet implemented.");
		return;

	case LoadDeleting:
		if (unit == NULL)
		{
			noteLoadError(state, "No such unit.");
			return;
		}

		state->target = unit;	/*	May be deletion target.	*/
		break;

	default:
		noteLoadError(state, "Not in an operation.");
		return;
	}
}

static void	handle_msgspace_start(LoadMibState *state, const char **atts)
{
	int		contnbr = 0;
	char		*gwEid = NULL;
	char		*symkeyname = NULL;
	int		isNeighbor = 1;
	char		**att;
	char		*name;
	char		*value;
	Continuum	*contin;
	Subject		*msgspace;

	if (noMibYet(state)) return;
	if (state->venture == NULL)
	{
		noteLoadError(state, "Venture not specified.");
		return;
	}

	if (state->subject)
	{
		noteLoadError(state, "Already working on a subject.");
		return;
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
		else if (strcmp(name, "neighbor") == 0)
		{
			isNeighbor = 1 - (0 == atoi(value));
		}
		else if (strcmp(name, "gweid") == 0)
		{
			gwEid = value;
		}
		else if (strcmp(name, "symkey") == 0)
		{
			symkeyname = value;
		}
		else
		{
			noteLoadError(state, "Unknown attribute.");
			return;
		}
	}

	if (contnbr < 1 || contnbr > MAX_CONTIN_NBR)
	{
		noteLoadError(state, "Need number of continuum.");
		return;
	}

	contin = (_mib(NULL))->continua[contnbr];
	if (contin == NULL)
	{
		noteLoadError(state, "Unknown continuum.");
		return;
	}

	msgspace = state->venture->msgspaces[contnbr];
	switch (state->currentOperation)
	{
	case LoadAdding:
		if (msgspace == NULL)
		{
			msgspace = createMsgspace(state->venture, contnbr,
					isNeighbor, gwEid, symkeyname);
			if (msgspace == NULL)
			{
				return putErrmsg("Couldn't add msgspace.",
						contin->name);
			}
		}

		break;

	case LoadChanging:
		noteLoadError(state, "'Change' not yet implemented.");
		return;

	case LoadDeleting:
		if (msgspace == NULL)
		{
			noteLoadError(state, "No such continuum.");
			return;
		}

		state->target = msgspace;	/*	May be target.	*/
		break;

	default:
		noteLoadError(state, "Not in an operation.");
		return;
	}
}

#ifdef NOEXPAT
static void		startElement(LoadMibState *state, const char *name,
				const char **atts)
{
#else
static void XMLCALL	startElement(void *userData, const char *name,
				const char **atts)
{
	LoadMibState	*state = (LoadMibState *) userData;
#endif
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

	if (strcmp(name, "sender") == 0)
	{
		return handle_sender_start(state, atts);
	}

	if (strcmp(name, "receiver") == 0)
	{
		return handle_receiver_start(state, atts);
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
	return;
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
		eraseApp((AmsApp *) (state->target));
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

static void	handle_sender_end(LoadMibState *state)
{
	if (state->target)	/*	Sender is deletion target.	*/
	{
		deleteAuthorizedSender(state->subject,
				(char *) (state->target));
		state->target = NULL;
	}
}

static void	handle_receiver_end(LoadMibState *state)
{
	if (state->target)	/*	Receiver is deletion target.	*/
	{
		deleteAuthorizedReceiver(state->subject,
				(char *) (state->target));
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

#ifdef NOEXPAT
static void		endElement(LoadMibState	*state, const char *name)
{
#else
static void XMLCALL	endElement(void *userData, const char *name)
{
	LoadMibState	*state = (LoadMibState *) userData;
#endif
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

	if (strcmp(name, "sender") == 0)
	{
		return handle_sender_end(state);
	}

	if (strcmp(name, "receiver") == 0)
	{
		return handle_receiver_end(state);
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
	return;
}

#ifdef NOEXPAT
#define MAX_ATTRIBUTES		20

static int	rcParse(LoadMibState *state, char *buf, size_t length)
{
	char	*elementName;
	char	*atts[MAX_ATTRIBUTES * 2];
	int	attNameIdx = 0;
	int	attValueIdx = 1;
	char	*cursor;
	char	*attStart;
	int	bytesRemaining;
	char	*delimiter;
	char	*token;

	if (length < 2)
	{
		writeMemoNote("[?] No element name in rc line", buf);
		return -1;
	}

	cursor = buf + 1;
	findToken(&cursor, &token);
	if (token == NULL)
	{
		writeMemo("Element name omitted.");
		return -1;
	}

	elementName = token;
	memset(atts, 0, sizeof atts);
	bytesRemaining = length - (cursor - buf);
	while (bytesRemaining > 0)
	{
		if (bytesRemaining < 2)
		{
			writeMemoNote("[?] Incomplete rc line attribute",
					cursor);
			return -1;
		}

		attStart = cursor;
		delimiter = strchr(cursor, '=');
		if (delimiter == NULL)
		{
			writeMemoNote("[?] Attribute name not terminated",
					cursor);
			return -1;
		}

		if (attValueIdx > MAX_ATTRIBUTES)
		{
			writeMemoNote("[?] Too many attributes", cursor);
			return -1;
		}

		atts[attNameIdx] = cursor;
		*delimiter = 0;
		cursor = delimiter + 1;
		findToken(&cursor, &token);
		if (token == NULL)
		{
			writeMemoNote("[?] Attribute value omitted",
					atts[attNameIdx]);
			return -1;
		}

		atts[attValueIdx] = token;
		attNameIdx += 2;
		attValueIdx += 2;
		bytesRemaining -= (cursor - attStart);
	}

	switch (buf[0])
	{
	case '+':
		startElement(state, elementName, (const char **) atts);
		break;

	case '-':
		endElement(state, elementName);
		break;

	case '*':
		startElement(state, elementName, (const char **) atts);
		endElement(state, elementName);
		break;

	default:
		writeMemoNote("[?] Invalid rc line control character", buf);
		return -1;
	}

	return 0;
}

static int	loadMibFromRcSource(char *mibSource)
{
	int		sourceFile;
	LoadMibState	state;
	char		buf[256];
	int		length;
	int		result = 0;

	if (*mibSource == '\0')		/*	Use default file name.	*/
	{
		mibSource = "mib.amsrc";
	}

	sourceFile = iopen(mibSource, O_RDONLY, 0777);
	if (sourceFile < 0)
	{
		putSysErrmsg("Can't open MIB source file", mibSource);
		return -1;
	}

	memset((char *) &state, 0, sizeof state);
	state.abandoned = 0;
	state.currentOperation = LoadDormant;
	state.lineNbr = 0;
	while (1)
	{
		if (igets(sourceFile, buf, sizeof(buf), &length) == NULL)
		{
			if (length == 0)	/*	End of file.	*/
			{
				break;		/*	Out of loop.	*/
			}

			putErrmsg("Failed reading MIB.", mibSource);
			break;			/*	Out of loop.	*/
		}

		state.lineNbr++;
		if (rcParse(&state, buf, length) < 0)
		{
			isprintf(buf, sizeof buf, "amsrc error at line %d.",
					state.lineNbr);
			writeMemo(buf);
			result = -1;
			break;			/*	Out of loop.	*/
		}

		if (state.abandoned)
		{
			writeMemo("[?] Abandoning MIB load.");
			result = -1;
			break;			/*	Out of loop.	*/
		}
	}

	close(sourceFile);
	return result;
}
#else
static int	loadMibFromXmlSource(char *mibSource)
{
	int		sourceFile;
	LoadMibState	state;
	char		buf[256];
	int		done = 0;
	size_t		length;
	int		result = 0;

	if (*mibSource == '\0')		/*	Use default file name.	*/
	{
		mibSource = "amsmib.xml";
	}

	sourceFile = iopen(mibSource, O_RDONLY, 0777);
	if (sourceFile < 0)
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
		close(sourceFile);
		return -1;
	}

	XML_SetElementHandler(state.parser, startElement, endElement);
	XML_SetUserData(state.parser, &state);
	while (!done)
	{
		length = read(sourceFile, buf, sizeof(buf));
		switch (length)
		{
		case -1:
			putSysErrmsg("Failed reading MIB", mibSource);

			/*	Intentional fall-through to next case.	*/

		case 0:			/*	End of file.		*/
			done = 1;
			break;

		default:
			done = (length < sizeof buf);
		}

		if (XML_Parse(state.parser, buf, length, done)
				== XML_STATUS_ERROR)
		{
			isprintf(buf, sizeof buf, "XML error at line %d.", (int)
					XML_GetCurrentLineNumber(state.parser));
			putSysErrmsg(buf, XML_ErrorString
					(XML_GetErrorCode(state.parser)));
			result = -1;
			break;		/*	Out of loop.		*/
		}

		if (state.abandoned)
		{
			writeMemo("[?] Abandoning MIB load.");
			result = -1;
			break;		/*	Out of loop.		*/
		}
	}

	XML_ParserFree(state.parser);
	close(sourceFile);
	return result;
}
#endif

int	updateMib(char *mibSource)
{
	int	result;

	if (mibSource == NULL)
	{
		return 0;		/*	Nothing to do.		*/
	}

	lockMib();
#ifdef NOEXPAT
	result = loadMibFromRcSource(mibSource);
#else
	result = loadMibFromXmlSource(mibSource);
#endif
	unlockMib();
	return result;
}

AmsMib	*loadMib(char *mibSource)
{
	AmsMib			*mib;
	int			result;
	int			i;
	TransSvc		*ts;
	AmsMibParameters	parms = { 0, NULL, NULL, NULL };

	lockMib();
	mib = _mib(NULL);
	if (mib)
	{
		mib->users += 1;
		unlockMib();
		return mib;	/*	MIB is already loaded.		*/
	}

	if (mibSource == NULL)
	{
		result = loadTestMib();
	}
	else
	{
#ifdef NOEXPAT
		result = loadMibFromRcSource(mibSource);
#else
		result = loadMibFromXmlSource(mibSource);
#endif
	}

	if (result < 0)
	{
		oK(_mib(&parms));	/*	Erase.			*/
		mib = NULL;
	}
	else
	{
		mib = _mib(NULL);
	}

	if (mib == NULL)
	{
		putErrmsg("Failed loading AMS MIB.", NULL);
		unlockMib();
		return NULL;
	}

	if (lyst_length(mib->amsEndpointSpecs) == 0)
	{
		for (i = 0, ts = mib->transportServices;
				i < mib->transportServiceCount; i++, ts++)
		{
			if (createAmsEpspec(ts->name, "@") == NULL)
			{
				putErrmsg("Can't load default AMS endpoint \
specs.", NULL);
				oK(_mib(&parms));	/*	Erase.	*/
				unlockMib();
				return NULL;
			}
		}
	}

	mib->users = 1;
	unlockMib();
	return mib;
}

void	unloadMib()
{
	AmsMib			*mib;
	AmsMibParameters	parms = { 0, NULL, NULL, NULL };

	lockMib();
	mib = _mib(NULL);
	if (mib)
	{
		mib->users -= 1;
		if (mib->users <= 0)
		{
			oK(_mib(&parms));		/*	Erase.	*/
		}
	}

	unlockMib();
}
