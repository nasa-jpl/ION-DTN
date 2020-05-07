/*
	amsd.c:	configuration and registration services daemon for
		Asynchronous Message Service (AMS).

	Author: Scott Burleigh, JPL

	Copyright (c) 2005, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
#include "amscommon.h"
#include "ams.h"

#define	RS_STOP_EVT	21

typedef struct
{
	int		csRequired;
	int		csRunning;
	char		*csEndpointSpec;
	LystElt		startOfFailoverChain;
	pthread_t	csThread;
	pthread_t	csHeartbeatThread;
	Lyst		csEvents;
	struct llcv_str	csEventsCV_str;
	Llcv		csEventsCV;
	MamsInterface	tsif;
} CsState;

typedef struct
{
	int		rsRequired;
	int		rsRunning;
	char		*rsAppName;
	char		*rsAuthName;
	char		*rsUnitName;
	Venture		*venture;
	Cell		*cell;
	int		cellHeartbeats;
	int		undeclaredModulesCount;
	char		undeclaredModules[MAX_MODULE_NBR + 1];
	MamsEndpoint	*csEndpoint;
	LystElt		csEndpointElt;
	int		heartbeatsMissed;	/*	From CS.	*/
	pthread_t	rsThread;
	pthread_t	rsHeartbeatThread;
	Lyst		rsEvents;
	struct llcv_str	rsEventsCV_str;
	Llcv		rsEventsCV;
	MamsInterface	tsif;
} RsState;

typedef struct
{
	int		dmRequired;
	int		dmRunning;
	char		*mibSource;
	char		*dmAppName;
	char		*dmAuthName;
	char		*dmUnitName;
	AmsModule	dmModule;
	pthread_t	dmThread;
	CsState		*csState;
	RsState		*rsState;
} DmState;

static int	_amsdRunning(int *state)
{
	static int		running = 0;
#ifndef mingw
	static pthread_t	amsdThread;
#endif

	if (state)
	{
		if (*state == 0)	/*	Stopping.		*/
		{
			running = 0;
#ifdef mingw
			sm_Wakeup(GetCurrentProcessId());
#else
			if (pthread_equal(amsdThread, pthread_self()) == 0)
			{
				pthread_kill(amsdThread, SIGINT);
			}
#endif
		}
		else			/*	Starting.		*/
		{
			running = 1;
#ifndef mingw
			amsdThread = pthread_self();
#endif
		}
	}

	return running;
}

static void	shutDownAmsd(int signum)
{
	int	stop = 0;

	isignal(SIGINT, shutDownAmsd);
	oK(_amsdRunning(&stop));
}

/*	*	*	Configuration server code	*	*	*/

static void	stopOtherConfigServers(CsState *csState)
{
	LystElt		elt;
	MamsEndpoint	*ep;

	for (elt = lyst_next(csState->startOfFailoverChain); elt;
			elt = lyst_next(elt))
	{
		ep = (MamsEndpoint *) lyst_data(elt);
		if (sendMamsMsg(ep, &(csState->tsif), I_am_running, 0, 0, NULL)
				< 0)
		{
			putErrmsg("Can't send I_am_running message.", NULL);
		}
	}
}

static void	*csHeartbeat(void *parm)
{
	CsState		*csState = (CsState *) parm;
	pthread_mutex_t	mutex;
	pthread_cond_t	cv;
	int		cycleCount = 6;
	int		i;
	Venture		*venture;
	int		j;
	Unit		*unit;
	Cell		*cell;
	int		result;
	struct timeval	workTime;
	struct timespec	deadline;

	CHKNULL(csState);
	if (pthread_mutex_init(&mutex, NULL))
	{
		putSysErrmsg("Can't start heartbeat, mutex init failed", NULL);
		return NULL;
	}

	if (pthread_cond_init(&cv, NULL))
	{
		pthread_mutex_destroy(&mutex);
		putSysErrmsg("Can't start heartbeat, cond init failed", NULL);
		return NULL;
	}
#ifndef mingw
	sigset_t	signals;

	sigfillset(&signals);
	pthread_sigmask(SIG_BLOCK, &signals, NULL);
#endif
	while (1)
	{
		lockMib();
		if (cycleCount > 5)	/*	Every N5_INTERVAL sec.	*/
		{
			cycleCount = 0;
			stopOtherConfigServers(csState);
		}

		for (i = 1; i <= MAX_VENTURE_NBR; i++)
		{
			venture = (_mib(NULL))->ventures[i];
			if (venture == NULL) continue;
			for (j = 0; j <= MAX_UNIT_NBR; j++)
			{
				unit = venture->units[j];
				if (unit == NULL
				|| (cell = unit->cell)->mamsEndpoint.ept
						== NULL)
				{
					continue;
				}

				if (cell->heartbeatsMissed == 3)
				{
					clearMamsEndpoint
						(&(cell->mamsEndpoint));
				}
				else if (cell->heartbeatsMissed < 3)
				{
					if (sendMamsMsg (&cell->mamsEndpoint,
						&csState->tsif, heartbeat,
						0, 0, NULL) < 0)
					{
						putErrmsg("Can't send \
heartbeat.", NULL);
					}
				}

				cell->heartbeatsMissed++;
			}
		}

		/*	Now sleep for N3_INTERVAL seconds.		*/

		unlockMib();
		getCurrentTime(&workTime);
		deadline.tv_sec = workTime.tv_sec + N3_INTERVAL;
		deadline.tv_nsec = workTime.tv_usec * 1000;
		pthread_mutex_lock(&mutex);
		result = pthread_cond_timedwait(&cv, &mutex, &deadline);
		pthread_mutex_unlock(&mutex);
		if (result)
		{
			errno = result;
			if (errno != ETIMEDOUT)
			{
				putSysErrmsg("Heartbeat failure", NULL);
				break;
			}
		}

		cycleCount++;
	}

	writeMemo("[i] CS heartbeat thread ended.");
	return NULL;
}

static void	cleanUpCsState(CsState *csState)
{
	if (csState->tsif.ts)
	{
		csState->tsif.ts->shutdownFn(csState->tsif.sap);
	}

	if (csState->tsif.ept)
	{
		MRELEASE(csState->tsif.ept);
	}

	if (csState->csEvents)
	{
		lyst_destroy(csState->csEvents);
		csState->csEvents = NULL;
	}

	llcv_close(csState->csEventsCV);
	csState->csRunning = 0;
}

static void	reloadRsRegistrations(CsState *csState)
{
	return;		/*	Maybe do this eventually.		*/
}

static void	processMsgToCs(CsState *csState, AmsEvt *evt)
{
	AmsMib		*mib = _mib(NULL);
	MamsMsg		*msg = (MamsMsg *) (evt->value);
	char		*zeroLengthEpt = "";
	Venture		*venture;
	Unit		*unit;
	Cell		*cell;
	char		*cursor;
	int		bytesRemaining;
	char		*ept;
	int		eptLength;
	MamsEndpoint	endpoint;
	int		i;
	int		supplementLength;
	char		*supplement;
	char		reasonCode;
	int		cellspecLength;
	char		*cellspec;
	int		result;
	int		unitNbr;

#if AMSDEBUG
PUTMEMO("CS got msg of type", itoa(msg->type));
PUTMEMO("...from role", itoa(msg->roleNbr));
#endif
	if (msg->type == I_am_running)
	{
		if (enqueueMamsCrash(csState->csEventsCV, "Outranked") < 0)
		{
			putErrmsg("CS can't enqueue own Crash event", NULL);
		}

		return;
	}

	/*	All other messages to CS are from registrars and
	 *	modules, which need valid venture and unit numbers.	*/

	venture = mib->ventures[msg->ventureNbr];
	if (venture == NULL)
	{
		unit = NULL;
	}
	else
	{
		unit = venture->units[msg->unitNbr];
	}

	switch (msg->type)
	{
	case heartbeat:
		if (unit == NULL
		|| (cell = unit->cell)->mamsEndpoint.ept == NULL)
		{
			return;	/*	Ignore invalid heartbeat.	*/
		}

		/*	Legitimate heartbeat from registered RS.	*/

		cell->heartbeatsMissed = 0;
		return;

	case announce_registrar:
		cursor = msg->supplement;
		bytesRemaining = msg->supplementLength;
		ept = parseString(&cursor, &bytesRemaining, &eptLength);
		if (ept == NULL)	/*	Ignore malformed msg.	*/
		{
			return;
		}

		if (unit == NULL)
		{
			reasonCode = REJ_NO_UNIT;
			endpoint.ept = ept;
			if (((mib->pts->parseMamsEndpointFn)(&endpoint)) == 0)
			{
				if (sendMamsMsg(&endpoint, &(csState->tsif),
						rejection, msg->memo, 1,
						&reasonCode) < 0)
				{
					putErrmsg("CS can't reject registrar.",
							NULL);
				}

				(mib->pts->clearMamsEndpointFn)(&endpoint);
			}

			return;
		}

		cell = unit->cell;
		if (cell->mamsEndpoint.ept == NULL)	/*	Needed.	*/
		{
			if (constructMamsEndpoint(&(cell->mamsEndpoint),
					eptLength, ept) < 0)
			{
				putErrmsg("CS can't register new registrar.",
						itoa(msg->unitNbr));
				return;
			}
		}

		/*	(Re-announcement by current registrar is okay.)	*/

		if (strcmp(ept, cell->mamsEndpoint.ept) != 0)
		{
			/*	Already have registrar for this cell.	*/

			reasonCode = REJ_DUPLICATE;
			endpoint.ept = ept;
			if (((mib->pts->parseMamsEndpointFn)(&endpoint)) == 0)
			{
				if (sendMamsMsg(&endpoint, &(csState->tsif),
						rejection, msg->memo, 1,
						&reasonCode) < 0)
				{
					putErrmsg("CS can't reject registrar.",
							NULL);
				}

				(mib->pts->clearMamsEndpointFn)(&endpoint);
			}

			return;
		}

		/*	Accept (possibly re-announced) registration of
		 *	registrar.					*/

		cell->heartbeatsMissed = 0;
		if (sendMamsMsg(&(cell->mamsEndpoint), &(csState->tsif),
				registrar_noted, msg->memo, 0, NULL) < 0)
		{
			putErrmsg("CS can't accept registrar.", NULL);
		}

		/*	Tell all other registrars about this one.	*/

		ept = cell->mamsEndpoint.ept;
		supplementLength = 2 + strlen(ept) + 1;
		supplement = MTAKE(supplementLength);
		CHKVOID(supplement);
		supplement[0] = (char) ((msg->unitNbr >> 8) & 0xff);
		supplement[1] = (char) (msg->unitNbr & 0xff);
		istrcpy(supplement + 2, ept, supplementLength - 2);
		cellspec = NULL;
		for (i = 0; i <= MAX_UNIT_NBR; i++)
		{
			if (i == msg->unitNbr)	/*	New one itself.	*/
			{
				continue;
			}

			unit = venture->units[i];
			if (unit == NULL
			|| (cell = unit->cell)->mamsEndpoint.ept == NULL)
			{
				continue;
			}

			/*	Tell this registrar about the new one.	*/

			if (sendMamsMsg(&(cell->mamsEndpoint),
					&(csState->tsif), cell_spec, 0,
					supplementLength, supplement) < 0)
			{
				putErrmsg("CS can't send cell_spec", NULL);
				break;
			}

			/*	Tell the new registrar about this one.	*/

			ept = cell->mamsEndpoint.ept;
			cellspecLength = 2 + strlen(ept) + 1;
			cellspec = MTAKE(cellspecLength);
			CHKVOID(cellspec);
			cellspec[0] = (char) ((unit->nbr >> 8) & 0xff);
			cellspec[1] = (char) (unit->nbr & 0xff);
			istrcpy(cellspec + 2, ept, cellspecLength - 2);
			if (sendMamsMsg(&endpoint, &(csState->tsif), cell_spec,
					0, cellspecLength, cellspec) < 0)
			{
				putErrmsg("CS can't send cell_spec.", NULL);
			}

			MRELEASE(cellspec);
		}

		if (cellspec == NULL)	/*	No other registrars.	*/
		{
			/*	Notify the new registrar, by sending
			 *	it a cell_spec announcing itself.	*/

			unit = venture->units[msg->unitNbr];
			cell = unit->cell;
			if (sendMamsMsg(&(cell->mamsEndpoint),
					&(csState->tsif), cell_spec, 0,
					supplementLength, supplement) < 0)
			{
				putErrmsg("CS can't send cell_spec", NULL);
				break;
			}
		}

		MRELEASE(supplement);
		return;

	case registrar_query:
		cursor = msg->supplement;
		bytesRemaining = msg->supplementLength;
		ept = parseString(&cursor, &bytesRemaining, &eptLength);
		if (ept == NULL)	/*	Ignore malformed msg.	*/
		{
			return;
		}

		endpoint.ept = ept;
		if (((mib->pts->parseMamsEndpointFn)(&endpoint)) < 0)
		{
			return;		/*	Can't respond.		*/
		}

		if (unit == NULL)
		{
			unitNbr = 0;
			ept = zeroLengthEpt;
		}
		else
		{
			unitNbr = unit->nbr;
			ept = unit->cell->mamsEndpoint.ept;
			if (ept == NULL)	/*	No registrar.	*/
			{
				ept = zeroLengthEpt;
			}
		}

		if (ept == zeroLengthEpt)
		{
			if (sendMamsMsg(&endpoint, &(csState->tsif),
				registrar_unknown, msg->memo, 0, NULL) < 0)
			{
				putErrmsg("CS can't send registrar_unknown",
						NULL);
			}

			(mib->pts->clearMamsEndpointFn)(&endpoint);
			return;
		}

		supplementLength = 2 + strlen(ept) + 1;
		supplement = MTAKE(supplementLength);
		CHKVOID(supplement);
		supplement[0] = (char) ((unitNbr >> 8) & 0xff);
		supplement[1] = (char) (unitNbr & 0xff);
		istrcpy(supplement + 2, ept, supplementLength - 2);
		result = sendMamsMsg(&endpoint, &(csState->tsif), cell_spec,
			       	msg->memo, supplementLength, supplement);
		MRELEASE(supplement);
		if (result < 0)
		{
			putErrmsg("CS can't send cell_spec.", NULL);
		}

		(mib->pts->clearMamsEndpointFn)(&endpoint);
		return;

	default:		/*	Inapplicable message; ignore.	*/
		return;
	}
}

static void	*csMain(void *parm)
{
	CsState	*csState = (CsState *) parm;
	LystElt	elt;
	AmsEvt	*evt;

	CHKNULL(csState);
#ifndef mingw
	sigset_t	signals;

	sigfillset(&signals);
	pthread_sigmask(SIG_BLOCK, &signals, NULL);
#endif
	csState->csRunning = 1;
	writeMemo("[i] Configuration server is running.");
	while (1)
	{
		if (llcv_wait(csState->csEventsCV, llcv_lyst_not_empty,
					LLCV_BLOCKING) < 0)
		{
			putErrmsg("CS thread failed getting event.", NULL);
			break;
		}

		llcv_lock(csState->csEventsCV);
		elt = lyst_first(csState->csEvents);
		if (elt == NULL)
		{
			llcv_unlock(csState->csEventsCV);
			continue;
		}

		evt = (AmsEvt *) lyst_data_set(elt, NULL);
		lyst_delete(elt);
		llcv_unlock(csState->csEventsCV);
		switch (evt->type)
		{
		case MAMS_MSG_EVT:
			lockMib();
			processMsgToCs(csState, evt);
			unlockMib();
			recycleEvent(evt);
			continue;

		case CRASH_EVT:
			writeMemoNote("[i] CS thread terminated", evt->value);
			recycleEvent(evt);
			break;	/*	Out of switch.			*/

		default:	/*	Inapplicable event; ignore.	*/
			recycleEvent(evt);
			continue;
		}

		break;		/*	Out of loop.			*/
	}

	/*	Operation of the configuration server is terminated.	*/

	writeErrmsgMemos();
	pthread_end(csState->csHeartbeatThread);
	pthread_join(csState->csHeartbeatThread, NULL);
	cleanUpCsState(csState);
	return NULL;
}

static int	startConfigServer(CsState *csState)
{
	AmsMib		*mib = _mib(NULL);
	MamsInterface	*tsif;
	LystElt		elt;
	MamsEndpoint	*ep;

	/*	Load the necessary state data structures.		*/

	csState->startOfFailoverChain = NULL;
	csState->csEvents = lyst_create_using(getIonMemoryMgr());
	CHKERR(csState->csEvents);
	lyst_delete_set(csState->csEvents, destroyEvent, NULL);
	csState->csEventsCV = llcv_open(csState->csEvents,
			&(csState->csEventsCV_str));
	CHKERR(csState->csEventsCV);

	/*	Initialize the MAMS transport service interface.	*/

	tsif = &(csState->tsif);
	tsif->ts = mib->pts;
	tsif->endpointSpec = csState->csEndpointSpec;
	tsif->eventsQueue = csState->csEventsCV;
	if (tsif->ts->mamsInitFn(tsif) < 0)
	{
		putErrmsg("amsd can't initialize CS MAMS interface.", NULL);
		return -1;
	}

	/*	Make sure the PTS endpoint opened by the initialize
	 *	function is one of the ones that the continuum knows
	 *	about.							*/

	lockMib();
	for (elt = lyst_first(mib->csEndpoints); elt; elt = lyst_next(elt))
	{
		ep = (MamsEndpoint *) lyst_data(elt);
		if (strcmp(ep->ept, tsif->ept) == 0)
		{
			csState->startOfFailoverChain = elt;
			break;
		}
	}

	unlockMib();
	if (csState->startOfFailoverChain == NULL)
	{
		putErrmsg("Endpoint spec doesn't match any catalogued \
CS endpoint.", csState->csEndpointSpec);
		return -1;
	}

	/*	Start the MAMS transport service receiver thread.	*/

	if (pthread_begin(&(tsif->receiver), NULL, mib->pts->mamsReceiverFn,
				tsif, "amsd_cs_tsif"))
	{
		putSysErrmsg("Can't spawn CS tsif thread", NULL);
		return -1;
	}

	/*	Reload best current information about registrars'
	 *	locations.						*/

	reloadRsRegistrations(csState);

	/*	Start the configuration server heartbeat thread.	*/

	if (pthread_begin(&csState->csHeartbeatThread, NULL, csHeartbeat,
				csState, "amsd_cs_heartbeat"))
	{
		putSysErrmsg("Can't spawn CS heartbeat thread", NULL);
		return -1;
	}

	/*	Start the configuration server main thread.		*/

	if (pthread_begin(&(csState->csThread), NULL, csMain,
		csState, "amsd_cs"))
	{
		putSysErrmsg("Can't spawn configuration server thread", NULL);
		return -1;
	}

	/*	Configuration server is now running.			*/

	return 0;
}

static void	stopConfigServer(CsState *csState)
{
	if (csState->csRunning)
	{
		if (enqueueMamsCrash(csState->csEventsCV, "Stopped") < 0)
		{
			putErrmsg("Can't enqueue MAMS termination.", NULL);
			cleanUpCsState(csState);
		}
		else
		{
			pthread_join(csState->csThread, NULL);
		}
	}
}

/*	*	*	Registrar code	*	*	*	*	*/

static int	sendMsgToCS(RsState *rsState, AmsEvt *evt)
{
	AmsMib		*mib = _mib(NULL);
	MamsMsg		*msg = (MamsMsg *) (evt->value);
	MamsEndpoint	*ep;
	int		result;

	if (rsState->csEndpoint)
	{
		result = sendMamsMsg(rsState->csEndpoint, &(rsState->tsif),
				msg->type, msg->memo, msg->supplementLength,
				msg->supplement);
	}
	else	/*	Not currently in contact with config server.	*/
	{
		if (lyst_length(mib->csEndpoints) == 0)
		{
			writeMemo("[?] Config server endpoints list empty.");
			return -1;
		}

		if (rsState->csEndpointElt == NULL)
		{
			rsState->csEndpointElt = lyst_first(mib->csEndpoints);
		}
		else	/*	Let's try next csEndpoint in list.	*/
		{
			rsState->csEndpointElt =
					lyst_next(rsState->csEndpointElt);
			if (rsState->csEndpointElt == NULL)
			{
				rsState->csEndpointElt =
						lyst_first(mib->csEndpoints);
			}
		}

		ep = (MamsEndpoint *) lyst_data(rsState->csEndpointElt);
		result = sendMamsMsg(ep, &(rsState->tsif), msg->type, msg->memo,
				msg->supplementLength, msg->supplement);
	}

	if (msg->supplement)
	{
		MRELEASE(msg->supplement);
	}

	if (result < 0)
	{
		putErrmsg("RS failed sending message to CS.", NULL);
	}

	return result;
}

static int	enqueueMsgToCS(RsState *rsState, MamsPduType msgType,
			signed int memo, unsigned short supplementLength,
			char *supplement)
{
	MamsMsg	msg;
	AmsEvt	*evt;

	memset((char *) &msg, 0, sizeof msg);
	msg.type = msgType;
	msg.memo = memo;
	msg.supplementLength = supplementLength;
	msg.supplement = supplement;
	evt = (AmsEvt *) MTAKE(1 + sizeof(MamsMsg));
	CHKERR(evt);
	memcpy(evt->value, (char *) &msg, sizeof msg);
	evt->type = MSG_TO_SEND_EVT;
	if (enqueueMamsEvent(rsState->rsEventsCV, evt, NULL, 0))
	{
		MRELEASE(evt);
		putErrmsg("Can't enqueue message-to-CS event.", NULL);
		return -1;
	}

	return 0;
}

static int	forwardMsg(RsState *rsState, MamsPduType msgType,
			int roleNbr, int unitNbr, int moduleNbr,
			unsigned short supplementLength, char *supplement)
{
	signed int	moduleId;
	int		i;
	Cell		*cell;
	Module		*module;
	int		result = 0;

	moduleId = computeModuleId(roleNbr, unitNbr, moduleNbr);
	cell = rsState->cell;
	for (i = 1; i <= MAX_MODULE_NBR; i++)
	{
		if (i == moduleNbr && unitNbr == rsState->cell->unit->nbr)
		{
			continue;	/*	Don't echo to source.	*/
		}

		module = cell->modules[i];
		if (module->role == NULL)
		{
			continue;	/*	No such module.		*/
		}

		result = sendMamsMsg(&(module->mamsEndpoint), &(rsState->tsif),
			msgType, moduleId, supplementLength, supplement);
		if (result < 0)
		{
			break;
		}
	}

	return 0;
}

static int	propagateMsg(RsState *rsState, MamsPduType msgType,
			int roleNbr, int unitNbr, int moduleNbr,
			unsigned short supplementLength, char *supplement)
{
	signed int	moduleId;
	int		i;
	Unit		*unit;
	Cell		*cell;
	int		result;

	if (forwardMsg(rsState, msgType, roleNbr, unitNbr, moduleNbr,
			supplementLength, supplement))
	{
		return -1;
	}

	moduleId = computeModuleId(roleNbr, unitNbr, moduleNbr);
	for (i = 0; i <= MAX_UNIT_NBR; i++)
	{
		if (i == rsState->cell->unit->nbr)	/*	Self.	*/
		{
			continue;
		}

		unit = rsState->venture->units[i];
		if (unit == NULL
		|| (cell = unit->cell)->mamsEndpoint.ept == NULL)
		{
			continue;
		}

		result = sendMamsMsg(&(cell->mamsEndpoint), &(rsState->tsif),
			msgType, moduleId, supplementLength, supplement);
		if (result < 0)
		{
			break;
		}
	}

	return 0;
}

static int	resyncCell(RsState *rsState)
{
	int		moduleCount = 0;
	int		i;
	Module		*module;
	unsigned char	moduleLyst[MAX_MODULE_NBR + 1];
	int		moduleLystLength;

	/*	Construct list of currently registered modules.		*/

	for (i = 1; i <= MAX_MODULE_NBR; i++)
	{
		module = rsState->cell->modules[i];
		if (module->role == NULL)
		{
			continue;	/*	No such module.		*/
		}

		moduleCount++;
		moduleLyst[moduleCount] = i;
	}

	moduleLyst[0] = moduleCount;
	moduleLystLength = moduleCount + 1;

	/*	Tell everybody in venture about state of own cell.	*/

	if (propagateMsg(rsState, cell_status, 0, rsState->cell->unit->nbr, 0,
			moduleLystLength, (char *) moduleLyst))
	{
		putErrmsg("RS can't propagate cell_status.", NULL);
	}

	return 0;
}

static void	processHeartbeatCycle(RsState *rsState, int *cycleCount,
			int *beatsSinceResync)
{
	int	i;
	Module	*module;

	/*	Send heartbeats to modules as necessary.		*/

	if (*cycleCount > 1)	/*	Every 20 seconds.		*/
	{
		*cycleCount = 0;

		/*	Registrar's census clock starts upon initial
		 *	contact with configuration server.		*/

		if (rsState->csEndpoint != NULL || rsState->cellHeartbeats > 0)
		{
			rsState->cellHeartbeats++;
		}

		/*	Send heartbeats to all modules in own cell.	*/

		for (i = 1; i <= MAX_MODULE_NBR; i++)
		{
			module = rsState->cell->modules[i];
			if (module->role == NULL)
			{
				continue;
			}

			if (module->heartbeatsMissed == 3)
			{
				if (sendMamsMsg(&(module->mamsEndpoint),
					&(rsState->tsif), you_are_dead,
					0, 0, NULL))
				{
					putErrmsg("RS can't send imputed \
termination to dead module.", NULL);
				}

				if (propagateMsg(rsState, I_am_stopping,
					module->role->nbr,
					rsState->cell->unit->nbr, i, 0, NULL))
				{
					putErrmsg("RS can't send imputed \
termination to peer modules.", NULL);
				}

				forgetModule(module);
			}
			else if (module->heartbeatsMissed < 3)
			{
				if (sendMamsMsg(&(module->mamsEndpoint),
					&(rsState->tsif), heartbeat,
					0, 0, NULL) < 0)
				{
					putErrmsg("RS can't send heartbeat.",
							NULL);
				}
			}

			module->heartbeatsMissed++;
		}
	}

	/*	Resync as necessary.				*/

	if (rsState->cell->resyncPeriod > 0)
	{
		(*beatsSinceResync)++;
		if (*beatsSinceResync == rsState->cell->resyncPeriod)
		{
			resyncCell(rsState);
			*beatsSinceResync = 0;
		}
	}
}

static void	*rsHeartbeat(void *parm)
{
	RsState		*rsState = (RsState *) parm;
	int		cycleCount = 0;
	pthread_mutex_t	mutex;
	pthread_cond_t	cv;
	int		supplementLen;
	char		*ept;
	int		beatsSinceResync = -1;
	struct timeval	workTime;
	struct timespec	deadline;
	int		result;

	CHKNULL(rsState);
	if (pthread_mutex_init(&mutex, NULL))
	{
		putSysErrmsg("Can't start heartbeat, mutex init failed", NULL);
		return NULL;
	}

	if (pthread_cond_init(&cv, NULL))
	{
		pthread_mutex_destroy(&mutex);
		putSysErrmsg("Can't start heartbeat, cond init failed", NULL);
		return NULL;
	}
#ifndef mingw
	sigset_t	signals;

	sigfillset(&signals);
	pthread_sigmask(SIG_BLOCK, &signals, NULL);
#endif
	while (1)		/*	Every 10 seconds.		*/
	{
		lockMib();

		/*	Send heartbeat to configuration server.		*/

		if (rsState->heartbeatsMissed == 3)
		{
			rsState->csEndpoint = NULL;
		}

		if (rsState->csEndpoint)	/*	Send heartbeat.	*/
		{
			enqueueMsgToCS(rsState, heartbeat, 0, 0, NULL);
		}
		else	/*	Try to reconnect to config server.	*/
		{
			supplementLen = strlen(rsState->tsif.ept) + 1;
			ept = MTAKE(supplementLen);
			if (ept == NULL)
			{
				unlockMib();
				putErrmsg("Can't record endpoint.", NULL);
				return NULL;
			}

			istrcpy(ept, rsState->tsif.ept, supplementLen);
			enqueueMsgToCS(rsState, announce_registrar, 0,
					supplementLen, ept);
		}

		rsState->heartbeatsMissed++;

		/*	Send heartbeats to all modules in cell; resync.	*/

		processHeartbeatCycle(rsState, &cycleCount, &beatsSinceResync);
		unlockMib();

		/*	Sleep for N3_INTERVAL seconds and repeat.	*/

		getCurrentTime(&workTime);
		deadline.tv_sec = workTime.tv_sec + N3_INTERVAL;
		deadline.tv_nsec = workTime.tv_usec * 1000;
		pthread_mutex_lock(&mutex);
		result = pthread_cond_timedwait(&cv, &mutex, &deadline);
		pthread_mutex_unlock(&mutex);
		if (result)
		{
			errno = result;
			if (errno != ETIMEDOUT)
			{
				putSysErrmsg("Heartbeat thread failure", NULL);
				break;
			}
		}

		cycleCount++;
	}

	writeMemo("[i] RS heartbeat thread ended.");
	return NULL;
}

static void	cleanUpRsState(RsState *rsState)
{
	if (rsState->tsif.ts)
	{
		rsState->tsif.ts->shutdownFn(rsState->tsif.sap);
	}

	if (rsState->tsif.ept)
	{
		MRELEASE(rsState->tsif.ept);
	}

	if (rsState->rsEvents)
	{
		lyst_destroy(rsState->rsEvents);
		rsState->rsEvents = NULL;
	}

	llcv_close(rsState->rsEventsCV);
	rsState->rsRunning = 0;
}

static int	skipDeliveryVector(int *bytesRemaining, char **cursor)
{
	int	len;

	if (*bytesRemaining < 1)
	{
		return -1;
	}

	(*cursor)++;
	(*bytesRemaining)--;
	if (parseString(cursor, bytesRemaining, &len) == NULL)
	{
		return -1;
	}

	return 0;
}

static int	skipDeliveryVectorList(int *bytesRemaining, char **cursor)
{
	int	vectorCount;

	if (*bytesRemaining < 1)
	{
		return -1;
	}

	vectorCount = (unsigned char) **cursor;
	(*cursor)++;
	(*bytesRemaining)--;
	while (vectorCount > 0)
	{
		if (skipDeliveryVector(bytesRemaining, cursor))
		{
			return -1;
		}

		vectorCount--;
	}

	return 0;
}

static int	skipDeclaration(int *bytesRemaining, char **cursor)
{
	int	listLength;

	/*	First skip over the subscriptions list.			*/

	if (*bytesRemaining < 2)
	{
		return -1;
	}

	listLength = (((unsigned char) **cursor) << 8) & 0xff00;
	(*cursor)++;
	listLength += (unsigned char) **cursor;
	(*cursor)++;
	(*bytesRemaining) -= 2;
	while (listLength > 0)
	{
		if (*bytesRemaining < SUBSCRIBE_LEN)
		{
			return -1;
		}

		(*cursor) += SUBSCRIBE_LEN;
		(*bytesRemaining) -= SUBSCRIBE_LEN;
		listLength--;
	}

	/*	Now skip over the invitations list.			*/

	if (*bytesRemaining < 2)
	{
		return -1;
	}

	listLength = (((unsigned char) **cursor) << 8) & 0xff00;
	(*cursor)++;
	listLength += (unsigned char) **cursor;
	(*cursor)++;
	(*bytesRemaining) -= 2;
	while (listLength > 0)
	{
		if (*bytesRemaining < INVITE_LEN)
		{
			return -1;
		}

		(*cursor) += INVITE_LEN;
		(*bytesRemaining) -= INVITE_LEN;
		listLength--;
	}

	return 0;
}

static void	processMsgToRs(RsState *rsState, AmsEvt *evt)
{
	AmsMib		*mib = _mib(NULL);
	MamsMsg		*msg = (MamsMsg *) (evt->value);
	Venture		*venture;
	Unit		*unit;
	Cell		*cell;
	Module		*module;
	int		unitNbr;
	int		i;
	int		moduleNbr;
	int		roleNbr;
	AppRole		*role;
	int		moduleCount;
	char		*ept;
	int		eptLength;
	MamsEndpoint	endpoint;
	int		supplementLength;
	char		*supplement;
	char		reasonCode;
	char		*reasonString;
	int		result;
	char		*cursor;
	int		bytesRemaining;

#if AMSDEBUG
PUTMEMO("RS got msg of type", itoa(msg->type));
PUTMEMO("...from role", itoa(msg->roleNbr));
#endif
	venture = mib->ventures[msg->ventureNbr];
	if (venture == NULL)
	{
		unit = NULL;
	}
	else
	{
		unit = venture->units[msg->unitNbr];
	}

	switch (msg->type)
	{
	case heartbeat:
		if (msg->ventureNbr == 0)	/*	From CS.	*/
		{
			rsState->heartbeatsMissed = 0;
			return;
		}

		/*	Heartbeat from a module.			*/

		if (unit == NULL || msg->memo < 1 || msg->memo > MAX_MODULE_NBR
		|| (module = unit->cell->modules[msg->memo]) == NULL
		|| module->role == NULL)
		{
			return;	/*	Ignore invalid heartbeat.	*/
		}

		/*	Legitimate heartbeat from registered module.	*/

		module->heartbeatsMissed = 0;
		return;

	case rejection:

		/*	Rejected on attempt to announce self to the
		 *	configuration server.				*/

		reasonCode = *(msg->supplement);
		switch (reasonCode)
		{
		case REJ_DUPLICATE:
			reasonString = "Duplicate";
			break;

		case REJ_NO_UNIT:
			reasonString = "No such unit";
			break;

		default:
			reasonString = "Reason unknown";
		}

		if (enqueueMamsCrash(rsState->rsEventsCV, reasonString) < 0)
		{
			putErrmsg("Can't enqueue MAMS termination.", NULL);
		}

		return;

	case registrar_noted:
		rsState->heartbeatsMissed = 0;
		rsState->csEndpoint =
			(MamsEndpoint *) lyst_data(rsState->csEndpointElt);
		rsState->csEndpointElt = NULL;
		return;

	case cell_spec:
		if (msg->supplementLength < 3)
		{
			writeMemo("[?] Cell spec lacks endpoint name.");
			return;
		}
		
		unitNbr = (((unsigned char) (msg->supplement[0])) << 8) +
				((unsigned char) (msg->supplement[1]));
		if (unitNbr == rsState->cell->unit->nbr)
		{
			return;		/*	Notification of self.	*/
		}

		cursor = msg->supplement + 2;
		bytesRemaining = msg->supplementLength - 2;
		ept = parseString(&cursor, &bytesRemaining, &eptLength);
		if (ept == NULL)
		{
			writeMemo("[?] Cell spec endpoint name invalid.");
			return;
		}

		/*	Cell spec for remote registrar.			*/

		unit = rsState->venture->units[unitNbr];
		if (unit == NULL)
		{
			return;		/*	Ignore invalid sender.	*/
		}

		cell = unit->cell;
		if (cell->mamsEndpoint.ept != NULL)
		{
			if (strcmp(ept, cell->mamsEndpoint.ept) == 0)
			{
				/*	Redundant information.		*/

				return;
			}

			writeMemoNote("[i] Got revised registrar spec; \
accepting it", itoa(unitNbr));
			clearMamsEndpoint(&(cell->mamsEndpoint));
		}

		if (constructMamsEndpoint(&(cell->mamsEndpoint), eptLength,
					ept) < 0)
		{
			clearMamsEndpoint(&(cell->mamsEndpoint));
			writeMemo("[?] Can't load spec for cell.");
		}

		return;

	case module_registration:

		/*	Parse module's MAMS endpoint in case it's
		 *	needed for an echo message.			*/

		cursor = msg->supplement;
		bytesRemaining = msg->supplementLength;
		ept = parseString(&cursor, &bytesRemaining, &eptLength);
		if (ept == NULL)
		{
			return;		/*	Ignore malformed msg.	*/
		}

		if (skipDeliveryVectorList(&bytesRemaining, &cursor) < 0)
		{
			return;		/*	Ignore malformed msg.	*/
		}

		endpoint.ept = ept;
		if (((mib->pts->parseMamsEndpointFn)(&endpoint)) < 0)
		{
			return;		/*	Can't respond.		*/
		}

		if (rsState->cellHeartbeats < 4)
		{
			reasonCode = REJ_NO_CENSUS;
			if (sendMamsMsg(&endpoint, &(rsState->tsif), rejection,
					msg->memo, 1, &reasonCode) < 0)
			{
				putErrmsg("RS can't reject MAMS msg.", NULL);
			}

			(mib->pts->clearMamsEndpointFn)(&endpoint);
			return;
		}

		moduleNbr = moduleCount = 0;
		for (i = 1; i <= MAX_MODULE_NBR; i++)
		{
			module = rsState->cell->modules[i];
			if (module->role == NULL)
			{
				/*	This is an unused module #.	*/

				if (moduleNbr == 0)
				{
					moduleNbr = i;
				}
			}
			else
			{
				moduleCount++;
			}
		}

		if (moduleNbr == 0)	/*	Cell already full.	*/
		{
			reasonCode = REJ_CELL_FULL;
			if (sendMamsMsg(&endpoint, &(rsState->tsif), rejection,
					msg->memo, 1, &reasonCode) < 0)
			{
				putErrmsg("RS can't reject MAMS msg.", NULL);
			}

			(mib->pts->clearMamsEndpointFn)(&endpoint);
			return;
		}

		module = rsState->cell->modules[moduleNbr];
		roleNbr = msg->roleNbr;
		role = rsState->venture->roles[roleNbr];
		if (rememberModule(module, role, eptLength, ept))
		{
			putErrmsg("RS can't register new module.", NULL);
			(mib->pts->clearMamsEndpointFn)(&endpoint);
			return;
		}

		supplementLength = 1;
		supplement = MTAKE(supplementLength);
		if (supplement == NULL)
		{
			forgetModule(module);
			putErrmsg("RS can't send module number.", NULL);
			(mib->pts->clearMamsEndpointFn)(&endpoint);
			return;
		}

		*supplement = moduleNbr;
		result = sendMamsMsg(&endpoint, &(rsState->tsif), you_are_in,
				msg->memo, supplementLength, supplement);
		(mib->pts->clearMamsEndpointFn)(&endpoint);
		MRELEASE(supplement);
		if (result < 0)
		{
			forgetModule(module);
			putErrmsg("RS can't accept module registration.", NULL);
			(mib->pts->clearMamsEndpointFn)(&endpoint);
			return;
		}

		if (propagateMsg(rsState, I_am_starting, roleNbr,
				rsState->cell->unit->nbr, moduleNbr,
				msg->supplementLength, msg->supplement))
		{
			putErrmsg("RS can't advertise new module.", NULL);
		}

		return;

	case I_am_stopping:
		if (parseModuleId(msg->memo, &roleNbr, &unitNbr, &moduleNbr))
		{
			writeMemo("[?] RS ditching I_am_stoppng.");
			return;
		}

		if (unitNbr == rsState->cell->unit->nbr)
		{
			/*	Message from module in own cell.	*/

			module = rsState->cell->modules[moduleNbr];
			forgetModule(module);
			if (propagateMsg(rsState, I_am_stopping, roleNbr,
				unitNbr, moduleNbr, msg->supplementLength,
				msg->supplement))
			{
				putErrmsg("RS can't propagate I_am_stopping.",
						NULL);
			}
		}
		else	/*	Message from registrar of another cell.	*/
		{
			if (forwardMsg(rsState, I_am_stopping, roleNbr, unitNbr,
					moduleNbr, msg->supplementLength,
					msg->supplement))
			{
				putErrmsg("RS can't forward I_am_stopping.",
						NULL);
			}
		}

		return;

	case reconnect:
		if (msg->supplementLength < 4)
		{
			return;		/*	Ignore malformed msg.	*/
		}

		moduleNbr = (unsigned char) *(msg->supplement + 2);
		cursor = msg->supplement + 4;
		bytesRemaining = msg->supplementLength - 4;
		ept = parseString(&cursor, &bytesRemaining, &eptLength);
		if (ept == NULL)
		{
			return;		/*	Ignore malformed msg.	*/
		}

		endpoint.ept = ept;
		if (((mib->pts->parseMamsEndpointFn)(&endpoint)) < 0)
		{
			return;		/*	Can't respond.		*/
		}

		if (rsState->cellHeartbeats > 3)
		{
			if (sendMamsMsg(&endpoint, &(rsState->tsif),
					you_are_dead, 0, 0, NULL) < 0)
			{
				putErrmsg("RS can't ditch reconnect.", NULL);
			}

			(mib->pts->clearMamsEndpointFn)(&endpoint);
			return;
		}

		/*	Registrar is still new enough not to know
		 *	the complete composition of the cell.		*/

		if (skipDeliveryVectorList(&bytesRemaining, &cursor) < 0)
		{
			(mib->pts->clearMamsEndpointFn)(&endpoint);
			return;		/*	Ignore malformed msg.	*/
		}

		if (skipDeclaration(&bytesRemaining, &cursor) < 0)
		{
			(mib->pts->clearMamsEndpointFn)(&endpoint);
			return;		/*	Ignore malformed msg.	*/
		}

		if (bytesRemaining == 0)
		{
			(mib->pts->clearMamsEndpointFn)(&endpoint);
			return;		/*	Ignore malformed msg.	*/
		}

		moduleCount = (unsigned char) *cursor;
		cursor++;
		bytesRemaining--;
		if (bytesRemaining != moduleCount)
		{
			(mib->pts->clearMamsEndpointFn)(&endpoint);
			return;		/*	Ignore malformed msg.	*/
		}

		/*	Message correctly formed, reconnection okay.	*/

		if (rsState->undeclaredModulesCount > 0
		&& rsState->undeclaredModules[moduleNbr] == 0)
		{
			/*	This module was not in the census
			 *	declared by the first reconnected
			 *	module.					*/

			if (sendMamsMsg(&endpoint, &rsState->tsif,
					you_are_dead, 0, 0, NULL) < 0)
			{
				putErrmsg("RS can't ditch reconnect.", NULL);
			}

			(mib->pts->clearMamsEndpointFn)(&endpoint);
			return;
		}

		module = rsState->cell->modules[moduleNbr];
		if (module->role)
		{
			/*	Another module identifying itself by
			 *	the same module number has already
			 *	reconnected.				*/

			if (sendMamsMsg(&endpoint, &(rsState->tsif),
					you_are_dead, 0, 0, NULL) < 0)
			{
				putErrmsg("RS can't ditch reconnect.", NULL);
			}

			(mib->pts->clearMamsEndpointFn)(&endpoint);
			return;
		}

		roleNbr = msg->roleNbr;
		role = rsState->venture->roles[roleNbr];
		result = rememberModule(module, role, eptLength, ept);
		if (result < 0)
		{
			putErrmsg("RS can't reconnect module.", NULL);
			(mib->pts->clearMamsEndpointFn)(&endpoint);
			return;
		}

		if (rsState->undeclaredModulesCount == 0)
		{
			/*	This is the first reconnect sent to
			 *	the resurrected registrar; load list
			 *	of modules to expect reconnects from.	*/

			rsState->undeclaredModulesCount = moduleCount;
			while (bytesRemaining > 0)
			{
				i = *cursor;
				if (i > 0 && i <= MAX_MODULE_NBR)
				{
					rsState->undeclaredModules[i] = 1;
				}

				bytesRemaining--;
				cursor++;
			}
		}

		/*	Scratch module off the undeclared modules list.	*/

		if (rsState->undeclaredModules[moduleNbr] == 1)
		{
			rsState->undeclaredModules[moduleNbr] = 0;
			rsState->undeclaredModulesCount--;
		}

		/*	Let module go about its business.		*/

		result = sendMamsMsg(&endpoint, &(rsState->tsif), reconnected,
				msg->memo, 0, NULL);
		(mib->pts->clearMamsEndpointFn)(&endpoint);
		if (result < 0)
		{
			putErrmsg("RS can't acknowledge MAMS msg.", NULL);
		}

		return;

	case subscribe:
	case unsubscribe:
	case invite:
	case disinvite:
	case module_status:
		if (parseModuleId(msg->memo, &roleNbr, &unitNbr, &moduleNbr))
		{
			writeMemo("[i] RS ditching MAMS propagation.");
			return;
		}

		if (unitNbr == rsState->cell->unit->nbr)
		{
			/*	Message from module in own cell.	*/

			if (propagateMsg(rsState, msg->type, roleNbr, unitNbr,
					moduleNbr, msg->supplementLength,
					msg->supplement))
			{
				putErrmsg("RS can't propagate message.",
						NULL);
			}
		}
		else	/*	Message from registrar of another cell.	*/
		{
			if (rsState->cell->resyncPeriod > 0)
			{
				if (forwardMsg(rsState, msg->type, roleNbr,
					unitNbr, moduleNbr,
					msg->supplementLength, msg->supplement))
				{
					putErrmsg("RS can't forward message.",
							NULL);
				}
			}
		}

		return;

	case cell_status:	/*	From registrar of some cell.	*/
		if (rsState->cell->resyncPeriod > 0)
		{
			if (forwardMsg(rsState, cell_status, 0, msg->unitNbr,
					0, msg->supplementLength,
					msg->supplement))
			{
				putErrmsg("RS can't forward message.", NULL);
			}
		}

		return;

	default:		/*	Inapplicable message; ignore.	*/
		return;
	}
}

static void	shutDownCell(RsState *rsState)
{
	if (forwardMsg(rsState, you_are_dead, 0, rsState->cell->unit->nbr,
				0, 0, NULL) < 0)
	{
		putErrmsg("Registrar can't shut down cell.",
				itoa(rsState->cell->unit->nbr));
	}
}

static void	*rsMain(void *parm)
{
	RsState	*rsState = (RsState *) parm;
	LystElt	elt;
	AmsEvt	*evt;
	int	result;

	CHKNULL(rsState);
#ifndef mingw
	sigset_t	signals;

	sigfillset(&signals);
	pthread_sigmask(SIG_BLOCK, &signals, NULL);
#endif
	rsState->rsRunning = 1;
	writeMemo("[i] Registrar is running.");
	while (1)
	{
		if (llcv_wait(rsState->rsEventsCV, llcv_lyst_not_empty,
					LLCV_BLOCKING) < 0)
		{
			putErrmsg("RS thread failed getting event.", NULL);
			break;
		}

		llcv_lock(rsState->rsEventsCV);
		elt = lyst_first(rsState->rsEvents);
		if (elt == NULL)
		{
			llcv_unlock(rsState->rsEventsCV);
			continue;
		}

		evt = (AmsEvt *) lyst_data_set(elt, NULL);
		lyst_delete(elt);
		llcv_unlock(rsState->rsEventsCV);
		switch (evt->type)
		{
		case MAMS_MSG_EVT:
			lockMib();
			processMsgToRs(rsState, evt);
			unlockMib();
			recycleEvent(evt);
			continue;

		case MSG_TO_SEND_EVT:
			lockMib();
			result = sendMsgToCS(rsState, evt);
			unlockMib();
			if (result < 0)
			{
				putErrmsg("Registrar CS contact failed.", NULL);
			}

			recycleEvent(evt);
			continue;

		case RS_STOP_EVT:
			shutDownCell(rsState);
			writeMemoNote("[i] RS thread terminated", "Shut down");
			recycleEvent(evt);
			break;	/*	Out of switch.			*/

		case CRASH_EVT:
			writeMemoNote("[i] RS thread terminated", evt->value);
			recycleEvent(evt);
			break;	/*	Out of switch.			*/

		default:	/*	Inapplicable event; ignore.	*/
			recycleEvent(evt);
			continue;
		}

		break;		/*	Out of loop.			*/
	}

	/*	Operation of the registrar is terminated.		*/

	writeErrmsgMemos();
	pthread_end(rsState->rsHeartbeatThread);
	pthread_join(rsState->rsHeartbeatThread, NULL);
	cleanUpRsState(rsState);
	return NULL;
}

static int	startRegistrar(RsState *rsState)
{
	AmsMib		*mib = _mib(NULL);
	Venture		*venture = NULL;
	char		ventureName[MAX_APP_NAME + 2 + MAX_AUTH_NAME + 1];
	Unit		*unit = NULL;
	MamsInterface	*tsif;

	CHKERR(rsState->rsAppName);
	CHKERR(*(rsState->rsAppName));
	CHKERR(rsState->rsAuthName);
	CHKERR(*(rsState->rsAuthName));
	CHKERR(rsState->rsUnitName);
	venture = lookUpVenture(rsState->rsAppName, rsState->rsAuthName);
	if (venture == NULL)
	{
		isprintf(ventureName, sizeof ventureName, "%s(%s)",
				rsState->rsAppName, rsState->rsAuthName);
		putErrmsg("Can't start registrar: no such message space.",
				ventureName);
		return -1;
	}

	rsState->venture = venture;
	unit = lookUpUnit(venture, rsState->rsUnitName);
	if (unit == NULL)
	{
		putErrmsg("Can't start registrar: no such unit.",
				rsState->rsUnitName);
		return -1;
	}

	rsState->cell = unit->cell;

	/*	Load the necessary state data structures.		*/

	rsState->rsEvents = lyst_create_using(getIonMemoryMgr());
	CHKERR(rsState->rsEvents);
	lyst_delete_set(rsState->rsEvents, destroyEvent, NULL);
	rsState->rsEventsCV = llcv_open(rsState->rsEvents,
			&(rsState->rsEventsCV_str));
	CHKERR(rsState->rsEventsCV);

	/*	Initialize the MAMS transport service interface.	*/

	tsif = &(rsState->tsif);
	tsif->ts = mib->pts;
	tsif->ventureNbr = rsState->venture->nbr;
	tsif->unitNbr = rsState->cell->unit->nbr;
	tsif->endpointSpec = NULL;
	tsif->eventsQueue = rsState->rsEventsCV;
	if (tsif->ts->mamsInitFn(tsif) < 0)
	{
		putErrmsg("amsd can't initialize RS MAMS interface.", NULL);
		return -1;
	}

	/*	Start the MAMS transport service receiver thread.	*/

	if (pthread_begin(&(tsif->receiver), NULL, mib->pts->mamsReceiverFn,
				tsif, "amsd_rs_tsif"))
	{
		putSysErrmsg("amsd can't spawn RS tsif thread", NULL);
		return -1;
	}

	/*	Start the registrar heartbeat thread.			*/

	if (pthread_begin(&rsState->rsHeartbeatThread, NULL, rsHeartbeat,
				rsState, "amsd_rs_heartbeat"))
	{
		putSysErrmsg("Can't spawn RS heartbeat thread", NULL);
		return -1;
	}

	/*	Start the registrar main thread.			*/

	if (pthread_begin(&(rsState->rsThread), NULL, rsMain,
		rsState, "amsd_rs"))
	{
		putSysErrmsg("Can't spawn registrar thread", NULL);
		return -1;
	}

	/*	Registrar is now running.				*/

	return 0;
}

static void	stopRegistrar(RsState *rsState)
{
	if (rsState->rsRunning)
	{
		if (enqueueMamsCrash(rsState->rsEventsCV, "Stopped") < 0)
		{
			putErrmsg("Can't enqueue MAMS termination.", NULL);
			cleanUpRsState(rsState);
		}
		else
		{
			pthread_join(rsState->rsThread, NULL);
		}
	}
}

/*	*	*	Daemon module code	*	*	*	*/

static void	cleanUpDmState(DmState *dmState)
{
	dmState->dmModule = NULL;
	dmState->dmRunning = 0;
}

static void	enqueueRegistrarStop(RsState *rsState)
{
	AmsEvt	*evt;

	evt = (AmsEvt *) MTAKE(2);
	CHKVOID(evt);
	evt->type = RS_STOP_EVT;
	evt->value[0] = '\0';
	if (enqueueMamsEvent(rsState->rsEventsCV, evt, NULL, 0))
	{
		putErrmsg("Can't enqueue registrar stop.", NULL);
		MRELEASE(evt);
	}
}

static void	*dmMain(void *parm)
{
	DmState		*dmState = (DmState *) parm;
	int		amsstopSubj;
	int		amsstopRole;
	AmsEvent	event;
	int		eventType;
	RsState		*rsState;
	int		stop = 0;

	CHKNULL(dmState);
#ifndef mingw
	sigset_t	signals;

	sigfillset(&signals);
	pthread_sigmask(SIG_BLOCK, &signals, NULL);
#endif
	dmState->dmRunning = 1;
	if (ams_register(dmState->mibSource, NULL, dmState->dmAppName,
			dmState->dmAuthName, dmState->dmUnitName, "amsd",
			&(dmState->dmModule)) < 0)
	{
		writeMemo("[?] AAMS module can't register.");
		writeErrmsgMemos();
		cleanUpDmState(dmState);
		return NULL;
	}

	writeMemo("[i] Daemon AAMS module is running.");
	amsstopSubj = ams_lookup_subject_nbr(dmState->dmModule, "amsstop");
	amsstopRole = ams_lookup_role_nbr(dmState->dmModule, "amsstop");
	if (amsstopSubj < 0 || amsstopRole < 0
	|| ams_subscribe(dmState->dmModule, amsstopRole, 0, 0, amsstopSubj,
			1, 0, AmsTransmissionOrder, AmsAssured) < 0)
	{
		writeMemo("[i] AAMS module can't subscribe to 'amsstop'.");
	}

	while (1)
	{
		if (ams_get_event(dmState->dmModule, AMS_BLOCKING, &event) < 0)
		{
			ams_recycle_event(event);
			putErrmsg("AAMS module can't get event.", NULL);
			break;			/*	Out of loop.	*/
		}

		eventType = ams_get_event_type(event);
		ams_recycle_event(event);
		switch (eventType)
		{
		case USER_DEFINED_EVT:		/*	Termination.	*/
			break;			/*	Out of switch.	*/

		case AMS_MSG_EVT:		/*	Only amsstop.	*/
			stopConfigServer(dmState->csState);
			rsState = dmState->rsState;
			if (rsState->rsRequired == 1 && rsState->rsRunning == 1)
			{
				enqueueRegistrarStop(dmState->rsState);
			}

			/*	Wait for modules to be canceled, then
			 *	shut down the AMS daemon.		*/

			snooze(3);
			oK(_amsdRunning(&stop));
			break;			/*	Out of switch.	*/

		default:
			continue;
		}

		writeMemo("[i] AAMS module terminated.");
		break;				/*	Out of loop.	*/
	}

	/*	Operation of the module is terminated.		*/

	writeErrmsgMemos();
	ams_unregister(dmState->dmModule);
	cleanUpDmState(dmState);
	return NULL;
}

static int	startModule(DmState *dmState)
{
	CHKERR(dmState->dmAppName);
	CHKERR(*(dmState->dmAppName));
	CHKERR(dmState->dmAuthName);
	CHKERR(*(dmState->dmAuthName));
	CHKERR(dmState->dmUnitName);

	/*	Start the AAMS module main thread.			*/

	if (pthread_begin(&(dmState->dmThread), NULL, dmMain,
		dmState, "amsd_aams_module"))
	{
		putSysErrmsg("Can't spawn AAMS module thread", NULL);
		return -1;
	}

	/*	AAMS module is now running.				*/

	return 0;
}

static void	stopModule(DmState *dmState)
{
	if (dmState->dmRunning)
	{
		if (ams_post_user_event(dmState->dmModule, 0, 0, NULL, 0) < 0)
		{
			putErrmsg("Can't post STOP user event.", NULL);
			cleanUpDmState(dmState);
		}
		else
		{
			pthread_join(dmState->dmThread, NULL);
		}
	}
}

/*	*	*	AMSD code	*	*	*	*	*/

static int	run_amsd(char *mibSource, char *csEndpointSpec,
			char *rsAppName, char *rsAuthName, char *rsUnitName)
{
	char		ownHostName[MAXHOSTNAMELEN + 1];
	char		eps[MAXHOSTNAMELEN + 5 + 1];
	CsState		csState;
	RsState		rsState;
	DmState		dmState;
	Venture		*venture;
	int		start = 1;

#if AMSDEBUG
PUTS("...in run_amsd...");
#endif
	/*	Apply defaults as necessary.				*/

	if (strcmp(mibSource, "@") == 0)
	{
		mibSource = NULL;
	}

	if (strcmp(csEndpointSpec, "@") == 0)
	{
		getNameOfHost(ownHostName, sizeof ownHostName);
		isprintf(eps, sizeof eps, "%s:2357", ownHostName);
		csEndpointSpec = eps;
	}

	if (strcmp(csEndpointSpec, ".") == 0)
	{
		csEndpointSpec = NULL;
	}

	/*	Load Management Information Base as necessary.		*/

	if (loadMib(mibSource) == NULL)
	{
		putErrmsg("amsd can't load MIB.", mibSource);
		return -1;
	}

#if AMSDEBUG
PUTS("...amsd loaded MIB...");
#endif
	memset((char *) &csState, 0, sizeof csState);
	csState.csEndpointSpec = csEndpointSpec;
	if (csEndpointSpec)
	{
		csState.csRequired = 1;
	}

	memset((char *) &rsState, 0, sizeof rsState);
	rsState.rsAppName = rsAppName;
	rsState.rsAuthName = rsAuthName;
	rsState.rsUnitName = rsUnitName;
	if (rsUnitName)
	{
		rsState.rsRequired = 1;
	}

	memset((char *) &dmState, 0, sizeof dmState);
	dmState.dmAppName = rsAppName;
	dmState.dmAuthName = rsAuthName;
	dmState.csState = &csState;
	dmState.rsState = &rsState;
	if (rsUnitName)
	{
		dmState.dmUnitName = rsUnitName;
	}
	else
	{
		dmState.dmUnitName = "";	/*	Root unit.	*/
	}

	if (rsAppName && rsAuthName)
	{
		venture = lookUpVenture(rsAppName, rsAuthName);
		if (venture)
		{
			if (lookUpRole(venture, "amsd") != NULL)
			{
				dmState.dmRequired = 1;
			}
		}
	}

	oK(_amsdRunning(&start));
	isignal(SIGINT, shutDownAmsd);
#if AMSDEBUG
PUTS("...amsd starting main loop...");
#endif
	while (1)
	{
		if (_amsdRunning(NULL) == 0)
		{
			stopModule(&dmState);
			stopRegistrar(&rsState);
			stopConfigServer(&csState);
			unloadMib();
			return 0;
		}

		lockMib();
		if (csState.csRequired == 1 && csState.csRunning == 0)
		{
			writeMemo("[i] Starting configuration server.");
			if (startConfigServer(&csState) < 0)
			{
				cleanUpCsState(&csState);
				putErrmsg("amsd can't start CS", NULL);
			}
		}

		if (rsState.rsRequired == 1 && rsState.rsRunning == 0)
		{
			writeMemo("[i] Starting registration server.");
			if (startRegistrar(&rsState) < 0)
			{
				cleanUpRsState(&rsState);
				putErrmsg("amsd can't start RS.", NULL);
			}
		}

		if (dmState.dmRequired == 1 && dmState.dmRunning == 0)
		{
			writeMemo("[i] Starting daemon's AAMS module.");
			if (startModule(&dmState) < 0)
			{
				cleanUpDmState(&dmState);
				putErrmsg("amsd can't start DM.", NULL);
			}
		}

		unlockMib();
#ifdef mingw
		sm_WaitForWakeup(N5_INTERVAL);
#else
		snooze(N5_INTERVAL);
#endif
	}
}

#if defined (ION_LWT)
int	amsd(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char		*mibSource = (char *) a1;
	char		*csEndpointSpec = (char *) a2;
	char		*rsAppName = (char *) a3;
	char		*rsAuthName = (char *) a4;
	char		*rsUnitName = (char *) a5;
	int		result;
#else
int	main(int argc, char *argv[])
{
	char		*mibSource;
	char		*csEndpointSpec;
	char		*rsAppName = NULL;
	char		*rsAuthName = NULL;
	char		*rsUnitName = NULL;
	int		result;

	if (argc != 3 && argc != 5 && argc != 6)
	{
		PUTS("Usage:  amsd { @ | <MIB source name> }");
		PUTS("             { . | @ | <config. server endpoint spec> }");
		PUTS("             [<registrar application name>");
		PUTS("              <registrar authority name>");
		PUTS("              [<registrar unit name>]]");
		return 0;
	}

	mibSource = argv[1];
	csEndpointSpec = argv[2];
	if (argc > 3)
	{
		rsAppName = argv[3];
		rsAuthName = argv[4];
		if (argc > 5)
		{
			rsUnitName = argv[5];
		}
	}
#endif
	result = run_amsd(mibSource, csEndpointSpec, rsAppName, rsAuthName,
			rsUnitName);
	writeErrmsgMemos();
	return result;
}
