/*
	ltpcounter.c:	a test block counter.  Hard-coded for client
			ID = 1, i.e., bundle protocol.
									*/
/*									*/
/*	Copyright (c) 2007, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include "platform.h"
#include "ltpP.h"
#include "rfx.h"

static int	_clientId(int *newId)
{
	static int	id = 0;

	if (newId)
	{
		id = *newId;
	}

	return id;
}


static int	_running(int *newState)
{
	static int	state = 1;

	if (newState)
	{
		state = *newState;
	}

	return state;
}

static int	_sessionsCanceled(int increment)
{
	static int	count = 0;
	
	if (increment)
	{
		count += increment;
	}
	
	return count;
}

static int	_blocksReceived(int increment)
{
	static int	count = 0;
	
	if (increment)
	{
		count += increment;
	}
	
	return count;

}
static int	_bytesReceived(int increment)
{
	static int	count = 0;
	
	if (increment)
	{
		count += increment;
	}
	
	return count;
}

static void	*showProgress(void *parm)
{
	PsmAddress	alarm = (PsmAddress) parm;

	while (1)
	{
		if (rfx_alarm_raised(alarm) == 0)
		{
			return NULL;
		}

		PUTS("v");
		fflush(stdout);
	}
}

static void	handleQuit(int signum)
{
	int	stop = 0;

	oK(_running(&stop));
	ltp_interrupt(_clientId(NULL));
}

static void	printCounts()
{
	PUTMEMO("Sessions canceled", itoa(_sessionsCanceled(0)));
	PUTMEMO("Blocks received", itoa(_blocksReceived(0)));
	PUTMEMO("Bytes received", itoa(_bytesReceived(0)));
	fflush(stdout);
}

#if defined (ION_LWT)
int	ltpcounter(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	int		clientId = a1;
	int		maxBytes = a2;
#else
int	main(int argc, char **argv)
{
	int		clientId = (argc > 1 ? strtol(argv[1], NULL, 0) : 0);
	int		maxBytes = (argc > 2 ? strtol(argv[2], NULL, 0) : 0);
#endif
	PsmAddress	alarm;
	pthread_t	progressThread;
	int		state = 1;
	LtpNoticeType	type;
	LtpSessionId	sessionId;
	unsigned char	reasonCode;
	unsigned char	endOfBlock;
	unsigned int	dataOffset;
	unsigned int	dataLength;
	Object		data;
	char		buffer[255];

	if (clientId < 1)
	{
		PUTS("Usage: ltpcounter <client ID> [<max nbr of bytes>]");
		PUTS("  Max nbr of bytes defaults to 2 billion.");
		return 0;
	}

	oK(_clientId(&clientId));
	if (maxBytes < 1)
	{
		maxBytes = 2000000000;
	}

	if (ltp_attach() < 0)
	{
		putErrmsg("ltpcounter can't initialize LTP.", NULL);
		return 1;
	}

	if (ltp_open(_clientId(NULL)) < 0)
	{
		putErrmsg("ltpcounter can't open client access.",
				itoa(_clientId(NULL)));
		return 1;
	}

	alarm = rfx_insert_alarm(5, 0);
	if (pthread_begin(&progressThread, NULL, showProgress,
		(void *) alarm, "ltpcounter_progress"))
	{
		putErrmsg("Can't start progress thread.", NULL);
		rfx_remove_alarm(alarm);
		ltp_close(_clientId(NULL));
		ltp_detach();
		return 0;
	}

	isignal(SIGINT, handleQuit);
	oK((_running(&state)));
	while (_running(NULL))
	{
		if (ltp_get_notice(_clientId(NULL), &type, &sessionId,
				&reasonCode, &endOfBlock, &dataOffset,
				&dataLength, &data) < 0)
		{
			putErrmsg("Can't get LTP notice.", NULL);
			state = 0;
			oK((_running(&state)));
			continue;
		}

		switch (type)
		{
		case LtpExportSessionCanceled:
			isprintf(buffer, sizeof buffer, "Transmission \
canceled: source engine " UVAST_FIELDSPEC ", session %u, reason code %d.",
					sessionId.sourceEngineId,
					sessionId.sessionNbr, reasonCode);
			writeMemo(buffer);
			if (data)
			{
				ltp_release_data(data);
			}

			break;

		case LtpImportSessionCanceled:
			oK(_sessionsCanceled(1));
			isprintf(buffer, sizeof buffer, "Reception canceled: \
source engine " UVAST_FIELDSPEC ", session %u, reason code %d.",
					sessionId.sourceEngineId,
					sessionId.sessionNbr, reasonCode);
			writeMemo(buffer);
			break;

		case LtpRecvGreenSegment:
			isprintf(buffer, sizeof buffer, "Green segment \
received, discarded: source engine " UVAST_FIELDSPEC ", session %u, \
offset %u, length %u, eob=%d.", sessionId.sourceEngineId, sessionId.sessionNbr,
					dataOffset, dataLength, endOfBlock);
			writeMemo(buffer);
			ltp_release_data(data);
			break;

		case LtpRecvRedPart:
			oK(_blocksReceived(1));
			oK(_bytesReceived(dataLength));
			ltp_release_data(data);
			break;

		default:
			break;
		}

		if (_bytesReceived(0) >= maxBytes)
		{
			state = 0;
			oK((_running(&state)));
		}
	}

	rfx_remove_alarm(alarm);
	pthread_join(progressThread, NULL);
	writeErrmsgMemos();
	printCounts();
	PUTS("Stopping ltpcounter.");
	ltp_close(_clientId(NULL));
	ltp_detach();
	return 0;
}
