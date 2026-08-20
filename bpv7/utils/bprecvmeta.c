/*
	bprecvmeta.c:	a bundle sink that reports the content of the
			metadata extension block carried by each bundle
			it receives.

			ION delivers the content of a received metadata
			block to the receiving application in the
			metadata members of the delivery structure, but
			no other utility reports them, so a metadata
			block that crosses the network cannot otherwise
			be observed at its destination.  This is the
			counterpart of bpsendmeta.

			Each bundle produces one "metadata" line, or a
			line reporting that the bundle carried no
			metadata block, so that a test can assert on
			the content that arrived rather than only on the
			arrival of the payload.
									*/
/*									*/
/*	Copyright (c) 2026, California Institute of Technology.		*/
/*	ALL RIGHTS RESERVED.  U.S. Government Sponsorship		*/
/*	acknowledged.							*/
/*									*/

#include <bp.h>

typedef struct
{
	BpSAP	sap;
	int	running;
} BptestState;

static BptestState	*_bptestState(BptestState *newState)
{
	void		*value;
	BptestState	*state;

	if (newState)			/*	Add task variable.	*/
	{
		value = (void *) (newState);
		state = (BptestState *) sm_TaskVar(&value);
	}
	else				/*	Retrieve task variable.	*/
	{
		state = (BptestState *) sm_TaskVar(NULL);
	}

	return state;
}

static void	handleQuit(int signum)
{
	BptestState	*state;

	/* Tell the compiler that we are not using 'signum' */
	(void)signum;

	isignal(SIGINT, handleQuit);
	PUTS("BP reception interrupted.");
	fflush(NULL);
	state = _bptestState(NULL);
	bp_interrupt(state->sap);
	state->running = 0;
}

/*	The metadata content of a bundle is arbitrary bytes, so it is
 *	reported as text only when every byte of it is printable.  A
 *	test that sends text can then assert on the text it sent, while
 *	a bundle carrying binary metadata is still reported in a form
 *	that shows what arrived.					*/

static int	metadataIsPrintable(unsigned char *metadata, int metadataLen)
{
	int	i;

	for (i = 0; i < metadataLen; i++)
	{
		if (!isprint(metadata[i]))
		{
			return 0;
		}
	}

	return 1;
}

static void	reportMetadata(BpDelivery *dlv)
{
	char		line[(BP_MAX_METADATA_LEN * 2) + 80];
	char		content[BP_MAX_METADATA_LEN + 1];
	char		*cursor;
	int		i;

	if (dlv->metadataType == 0)
	{
		/*	ION reports a metadata type of zero when the
		 *	bundle carried no metadata block, since zero is
		 *	also the value that makes a sending application
		 *	decline to produce one.				*/

		PUTS("\tno metadata block.");
		fflush(NULL);
		return;
	}

	if (metadataIsPrintable(dlv->metadata, dlv->metadataLen))
	{
		memcpy(content, dlv->metadata, dlv->metadataLen);
		content[dlv->metadataLen] = '\0';
		isprintf(line, sizeof line,
				"\tmetadata type %u, length %u, '%s'",
				(unsigned int) dlv->metadataType,
				(unsigned int) dlv->metadataLen, content);
	}
	else
	{
		cursor = line;
		isprintf(cursor, sizeof line,
				"\tmetadata type %u, length %u, 0x",
				(unsigned int) dlv->metadataType,
				(unsigned int) dlv->metadataLen);
		cursor += strlen(cursor);
		for (i = 0; i < dlv->metadataLen; i++)
		{
			isprintf(cursor, 3, "%02x",
					(unsigned int) dlv->metadata[i]);
			cursor += 2;
		}
	}

	PUTS(line);
	fflush(NULL);
}

#if defined (ION_LWT)
int	bprecvmeta(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char		*ownEid = (char *) a1;
	int		maxBundles = a2 ? strtol((char *) a2, NULL, 0) : 0;
#else
int	main(int argc, char **argv)
{
	char		*ownEid = (argc > 1 ? argv[1] : NULL);
	int		maxBundles = argc > 2 ? strtol(argv[2], NULL, 0) : 0;
#endif
	static char	*deliveryTypes[] =	{
				"Payload delivered.",
				"Reception timed out.",
				"Reception interrupted.",
				"Endpoint stopped."
						};
	BptestState	state = { NULL, 1 };
	Sdr		sdr;
	BpDelivery	dlv;
	int		contentLength;
	ZcoReader	reader;
	int		len;
	int		bundlesReceived = 0;
	char		content[80];
	char		line[84];

	setlinebuf(stdout);
	if (ownEid == NULL)
	{
		PUTS("Usage: bprecvmeta <own endpoint ID> [max bundles]");
		PUTS("  Reports the metadata block carried by each bundle.");
		PUTS("  Runs until interrupted if max bundles is omitted.");
		fflush(NULL);
		return 0;
	}

	if (bp_attach() < 0)
	{
		putErrmsg("Can't attach to BP.", NULL);
		return 0;
	}

	if (bp_open(ownEid, &state.sap) < 0)
	{
		putErrmsg("Can't open own endpoint.", ownEid);
		return 0;
	}

	oK(_bptestState(&state));
	sdr = bp_get_sdr();
	isignal(SIGINT, handleQuit);
	while (state.running)
	{
		if (bp_receive(state.sap, &dlv, BP_BLOCKING) < 0)
		{
			putErrmsg("bprecvmeta bundle reception failed.", NULL);
			state.running = 0;
			continue;
		}

		PUTMEMO("ION event", deliveryTypes[dlv.result - 1]);
		fflush(NULL);
		if (dlv.result == BpReceptionInterrupted)
		{
			bp_release_delivery(&dlv, 1);
			continue;
		}

		if (dlv.result == BpEndpointStopped)
		{
			bp_release_delivery(&dlv, 1);
			state.running = 0;
			continue;
		}

		if (dlv.result == BpPayloadPresent)
		{
			CHKZERO(sdr_begin_xn(sdr));
			contentLength = zco_source_data_length(sdr, dlv.adu);
			sdr_exit_xn(sdr);
			isprintf(line, sizeof line, "\tpayload length is %d.",
					contentLength);
			PUTS(line);
			fflush(NULL);
			if (contentLength >= 0
			&& (size_t) contentLength < sizeof content)
			{
				zco_start_receiving(dlv.adu, &reader);
				CHKZERO(sdr_begin_xn(sdr));
				len = zco_receive_source(sdr, &reader,
						contentLength, content);
				if (sdr_end_xn(sdr) < 0 || len < 0)
				{
					putErrmsg("Can't handle delivery.",
							NULL);
					bp_release_delivery(&dlv, 1);
					state.running = 0;
					continue;
				}

				content[contentLength] = '\0';
				isprintf(line, sizeof line, "\t'%s'", content);
				PUTS(line);
				fflush(NULL);
			}

			reportMetadata(&dlv);
			bundlesReceived++;
		}

		bp_release_delivery(&dlv, 1);
		if (maxBundles > 0 && bundlesReceived >= maxBundles)
		{
			state.running = 0;
		}
	}

	bp_close(state.sap);
	writeErrmsgMemos();
	PUTS("Stopping bprecvmeta.");
	fflush(NULL);
	bp_detach();
	return 0;
}
