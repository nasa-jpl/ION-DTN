/*
	tcp4cli.c:	BP TCPCLv4-based convergence-layer input daemon.

			Receives bundle data from shared memory slots
			filled by the external TCPCLv4 agent, and
			delivers them to the BPA via the standard
			acquisition pipeline.

	Author: iondev33

	Copyright (c) 2026, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
								*/

#include "tcp4cla.h"

/*	*	*	Main thread functions	*	*	*	*/

typedef struct
{
	VInduct		*vduct;
	int		running;
	CtrlChannel	*ctrl_ch;
	IonTransferPool	*rx_pool;
} ReceiverThreadParms;

static void	interruptThread(int signum)
{
	(void)signum;
	isignal(SIGTERM, interruptThread);
	ionKillMainThread("tcp4cli");
}

static void	handleAgentDisconnect(ReceiverThreadParms *rtp,
			AcqWorkArea *work, int *midAcquisition)
{
	writeMemo("[w] tcp4cli: agent disconnected.");

	/*	Cancel any in-progress bundle acquisition.		*/

	if (*midAcquisition)
	{
		bpCancelAcq(work);
		*midAcquisition = 0;
	}

	/*	Force all pool slots to FREE.				*/

	pool_reset_all_slots(rtp->rx_pool);
	ctrl_disconnect(rtp->ctrl_ch);

	/*	Wait for the agent to reconnect, but stay interruptible so
	 *	shutdown is not blocked here if it never comes back.	*/

	writeMemo("[i] tcp4cli: waiting for agent reconnect...");
	if (ctrl_server_accept_wait(rtp->ctrl_ch, &rtp->running) == 0)
	{
		writeMemo("[i] tcp4cli: agent reconnected.");
	}
	else
	{
		rtp->running = 0;
	}
}

static void	*receiveFromAgent(void *parm)
{
	ReceiverThreadParms	*rtp = (ReceiverThreadParms *) parm;
	char			*procName = "tcp4cli";
	AcqWorkArea		*work;
	CtrlMsg			msg;
	CtrlMsg			releaseMsg;
	char			*data;
	uint64_t		data_len;
	uint64_t		xfer_id;
	int			ret;
	int			midAcquisition = 0;

	snooze(1);	/*	Let main thread become interruptible.	*/
	work = bpGetAcqArea(rtp->vduct);
	if (work == NULL)
	{
		putErrmsg("tcp4cli can't get acquisition work area.", NULL);
		ionKillMainThread(procName);
		return NULL;
	}

	while (rtp->running)
	{
		ret = ctrl_recv(rtp->ctrl_ch, &msg, TCP4CLA_CTRL_POLL_MS);
		if (ret == 0)
		{
			/*	Timeout.  Check keepalive.		*/

			if (ctrl_check_timeout(rtp->ctrl_ch))
			{
				handleAgentDisconnect(rtp, work,
						&midAcquisition);
			}

			if (ctrl_keepalive_due(rtp->ctrl_ch))
			{
				ctrl_send_keepalive(rtp->ctrl_ch);
			}

			continue;
		}

		if (ret < 0)
		{
			handleAgentDisconnect(rtp, work, &midAcquisition);
			continue;
		}

		/*	Got a message.					*/

		switch (msg.type)
		{
		case CTRL_BUNDLE_RECV:
			/*	Agent has placed a bundle into the
			 *	shared memory slot.  Read it directly
			 *	and feed to the BPA.			*/

			data = pool_slot_read(rtp->rx_pool,
					msg.slot_index,
					&data_len, &xfer_id);
			if (data == NULL)
			{
				writeMemo("[w] tcp4cli: BUNDLE_RECV for "
						"non-READY slot, "
						"discarding.");
				break;
			}

			if (bpBeginAcq(work, 0, NULL) < 0
			|| bpContinueAcq(work, data, (int) data_len,
					0, 0) < 0
			|| bpEndAcq(work) < 0)
			{
				putErrmsg("tcp4cli: can't acquire "
						"bundle.", NULL);
				ionKillMainThread(procName);
				rtp->running = 0;
				pool_slot_release(rtp->rx_pool,
						msg.slot_index);
				break;
			}

			/*	Release the RX slot and notify
			 *	the agent.				*/

			pool_slot_release(rtp->rx_pool, msg.slot_index);

			ctrl_msg_init(&releaseMsg, CTRL_SLOT_RELEASE,
					msg.slot_index, 0, NULL, 0);
			if (ctrl_send(rtp->ctrl_ch, &releaseMsg) < 0)
			{
				writeMemo("[w] tcp4cli: can't send "
						"SLOT_RELEASE.");
			}

			break;

		case CTRL_SESSION_UP:
			{
				char	txt[512];

				isprintf(txt, sizeof(txt),
					"[i] tcp4cli: TCPCL session up, "
					"peer=%.*s", (int) msg.payload_len,
					msg.payload);
				writeMemo(txt);
			}

			break;

		case CTRL_SESSION_DOWN:
			writeMemo("[w] tcp4cli: TCPCL session down.");
			break;

		case CTRL_KEEPALIVE:
			break;

		default:
			{
				char	txt[128];

				isprintf(txt, sizeof(txt),
					"[w] tcp4cli: unknown control "
					"msg type 0x%02x", msg.type);
				writeMemo(txt);
			}

			break;
		}

		sm_TaskYield();
	}

	writeErrmsgMemos();
	writeMemo("[i] tcp4cli receiver thread has ended.");
	bpReleaseAcqArea(work);
	return NULL;
}

/*	*	*	Main entry point	*	*	*	*/

#if defined(ION_LWT)
int	tcp4cli(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	*ductName = (char *) a1;
#else
int	main(int argc, char *argv[])
{
	char	*ductName = (argc > 1 ? argv[1] : NULL);
#endif
	VInduct			*vduct;
	PsmAddress		vductElt;
	Sdr			sdr;
	Induct			duct;
	ClProtocol		protocol;
	Tcp4Config		cfg;
	IonTransferPool		rx_pool;
	CtrlChannel		rx_ctrl;
	ReceiverThreadParms	rtp;
	pthread_t		receiverThread;

	if (ductName == NULL)
	{
		PUTS("Usage: tcp4cli <duct name>");
		return 0;
	}

	if (bpAttach() < 0)
	{
		putErrmsg("tcp4cli can't attach to BP.", NULL);
		return -1;
	}

	findInduct("tcp4", ductName, &vduct, &vductElt);
	if (vductElt == 0)
	{
		putErrmsg("No such tcp4 duct.", ductName);
		return -1;
	}

	if (vduct->cliPid != ERROR && vduct->cliPid != sm_TaskIdSelf())
	{
		putErrmsg("CLI task is already started for this duct.",
				itoa(vduct->cliPid));
		return -1;
	}

	/*	All arguments validated.				*/

	sdr = getIonsdr();
	CHKZERO(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &duct,
			sdr_list_data(sdr, vduct->inductElt),
			sizeof(Induct));
	sdr_read(sdr, (char *) &protocol, duct.protocol,
			sizeof(ClProtocol));
	sdr_exit_xn(sdr);

	/*	Parse config and set up IPC.				*/

	if (tcp4_parse_config(ductName, &cfg) < 0)
	{
		putErrmsg("tcp4cli: can't parse config.", ductName);
		return -1;
	}

	if (tcp4_apply_pool_overrides(&cfg, getenv("TCP4_POOL_SLOTS"),
			getenv("TCP4_POOL_SLOT_BYTES")) < 0)
	{
		putErrmsg("tcp4cli: invalid pool geometry override.",
				ductName);
		return -1;
	}

	if (pool_create(cfg.rx_pool_name, cfg.num_slots,
			cfg.slot_data_capacity, &rx_pool) < 0)
	{
		putErrmsg("tcp4cli: can't create RX pool.",
				cfg.rx_pool_name);
		return -1;
	}

	{
		char	geo[128];

		isprintf(geo, sizeof geo, "[i] tcp4cli RX pool '%s': %u slots "
				"x %u bytes", cfg.rx_pool_name, cfg.num_slots,
				cfg.slot_data_capacity);
		writeMemo(geo);
	}

	if (ctrl_server_create(&rx_ctrl, cfg.rx_ctrl_path,
			TCP4CLA_KEEPALIVE_INTERVAL,
			TCP4CLA_KEEPALIVE_TIMEOUT) < 0)
	{
		putErrmsg("tcp4cli: can't create control socket.",
				cfg.rx_ctrl_path);
		pool_destroy(&rx_pool);
		return -1;
	}

	{
		char	txt[512];

		isprintf(txt, sizeof(txt),
				"[i] tcp4cli waiting for agent on %s ...",
				cfg.rx_ctrl_path);
		writeMemo(txt);
	}

	if (ctrl_server_accept(&rx_ctrl) < 0)
	{
		putErrmsg("tcp4cli: agent connection failed.", NULL);
		ctrl_destroy(&rx_ctrl);
		pool_destroy(&rx_pool);
		return -1;
	}

	writeMemo("[i] tcp4cli: agent connected.");

	/*	Set up signal handling.  SIGTERM is shutdown signal.	*/

	ionNoteMainThread("tcp4cli");
	isignal(SIGTERM, interruptThread);

	/*	Start the receiver thread.				*/

	rtp.vduct = vduct;
	rtp.running = 1;
	rtp.ctrl_ch = &rx_ctrl;
	rtp.rx_pool = &rx_pool;

	if (pthread_begin(&receiverThread, NULL, receiveFromAgent, &rtp))
	{
		putSysErrmsg("tcp4cli can't create receiver thread", NULL);
		ctrl_destroy(&rx_ctrl);
		pool_destroy(&rx_pool);
		return -1;
	}

	{
		char	txt[512];

		isprintf(txt, sizeof(txt),
				"[i] tcp4cli is running for duct '%s'.",
				ductName);
		writeMemo(txt);
	}

	ionPauseMainThread(-1);

	/*	Time to shut down.					*/

	rtp.running = 0;
	pthread_join(receiverThread, NULL);

	ctrl_destroy(&rx_ctrl);
	pool_destroy(&rx_pool);

	writeErrmsgMemos();
	writeMemo("[i] tcp4cli duct has ended.");
	ionDetach();
	return 0;
}
