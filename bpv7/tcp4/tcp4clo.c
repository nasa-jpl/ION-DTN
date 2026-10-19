/*
	tcp4clo.c:	BP TCPCLv4-based convergence-layer output daemon.

			Dequeues bundles from the BPA, extracts them
			into shared memory slots, and notifies the
			external TCPCLv4 agent via a Unix domain
			socket control channel.  The agent reports
			transfer disposition (success/failure)
			asynchronously.

	Author: iondev33

	Copyright (c) 2026, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
								*/

#include "tcp4cla.h"

static sm_SemId		tcp4cloSemaphore(sm_SemId *semid)
{
	static sm_SemId	semaphore = -1;

	if (semid)
	{
		semaphore = *semid;
	}

	return semaphore;
}

static void	shutDownClo(int signum)
{
	(void)signum;
	sm_SemEnd(tcp4cloSemaphore(NULL));
}

/*	*	*	Response thread		*	*	*	*/

typedef struct
{
	int		*running;
	int		*agent_connected;
	CtrlChannel	*ctrl_ch;
	IonTransferPool	*tx_pool;
	Tcp4InFlight	*inflight;
	int		inflight_size;
	pthread_mutex_t	*inflight_mutex;
} ResponseThreadParms;

static void	*handleResponses(void *parm)
{
	ResponseThreadParms	*rtp = (ResponseThreadParms *) parm;
	CtrlMsg			msg;
	SdrObject		bundleZco;
	int			ret;

	iblock(SIGTERM);	/*	Only main thread handles.	*/

	while (*(rtp->running))
	{
		ret = ctrl_recv(rtp->ctrl_ch, &msg, TCP4CLA_CTRL_POLL_MS);
		if (ret == 0)
		{
			/*	Timeout.  Check keepalive status.	*/

			if (ctrl_check_timeout(rtp->ctrl_ch))
			{
				writeMemo("[w] tcp4clo: agent keepalive "
						"timeout, failing all "
						"in-flight.");
				*(rtp->agent_connected) = 0;
				pthread_mutex_lock(rtp->inflight_mutex);
				tcp4_inflight_fail_all(rtp->inflight,
						rtp->inflight_size);
				pthread_mutex_unlock(rtp->inflight_mutex);
				pool_reset_all_slots(rtp->tx_pool);
				ctrl_disconnect(rtp->ctrl_ch);

				/*	Wait for agent reconnect.	*/

				writeMemo("[i] tcp4clo: waiting for agent "
						"reconnect...");
				if (ctrl_server_accept(rtp->ctrl_ch) < 0)
				{
					*(rtp->running) = 0;
				}
				else
				{
					writeMemo("[i] tcp4clo: agent "
							"reconnected.");
					*(rtp->agent_connected) = 1;
				}
			}

			/*	Send keepalive if due.			*/

			if (ctrl_keepalive_due(rtp->ctrl_ch))
			{
				ctrl_send_keepalive(rtp->ctrl_ch);
			}

			continue;
		}

		if (ret < 0)
		{
			/*	Agent disconnected.			*/

			writeMemo("[w] tcp4clo: agent disconnected, "
					"failing all in-flight.");
			*(rtp->agent_connected) = 0;
			pthread_mutex_lock(rtp->inflight_mutex);
			tcp4_inflight_fail_all(rtp->inflight,
					rtp->inflight_size);
			pthread_mutex_unlock(rtp->inflight_mutex);
			pool_reset_all_slots(rtp->tx_pool);
			ctrl_disconnect(rtp->ctrl_ch);

			writeMemo("[i] tcp4clo: waiting for agent "
					"reconnect...");
			if (ctrl_server_accept(rtp->ctrl_ch) < 0)
			{
				*(rtp->running) = 0;
			}
			else
			{
				writeMemo("[i] tcp4clo: agent reconnected.");
				*(rtp->agent_connected) = 1;
			}

			continue;
		}

		/*	Got a message.					*/

		switch (msg.type)
		{
		case CTRL_XFER_SUCCESS:
			pthread_mutex_lock(rtp->inflight_mutex);
			bundleZco = tcp4_inflight_remove(rtp->inflight,
					rtp->inflight_size, msg.xfer_id);
			pthread_mutex_unlock(rtp->inflight_mutex);
			if (bundleZco != 0)
			{
				if (bpHandleXmitSuccess(bundleZco) < 0)
				{
					putErrmsg("tcp4clo: can't handle "
						"xmit success.", NULL);
					*(rtp->running) = 0;
				}
			}

			/*	The TX slot is NOT released here: the
			 *	agent (consumer) already released it
			 *	after copying the data out, and the
			 *	main thread may have reused it for a
			 *	newer transfer.  Forcing it free here
			 *	would destroy that transfer's data.	*/

			break;

		case CTRL_XFER_FAILURE:
		case CTRL_XFER_REFUSED:
			pthread_mutex_lock(rtp->inflight_mutex);
			bundleZco = tcp4_inflight_remove(rtp->inflight,
					rtp->inflight_size, msg.xfer_id);
			pthread_mutex_unlock(rtp->inflight_mutex);
			if (bundleZco != 0)
			{
				if (bpHandleXmitFailure(bundleZco) < 0)
				{
					putErrmsg("tcp4clo: can't handle "
						"xmit failure.", NULL);
					*(rtp->running) = 0;
				}
			}

			/*	Slot release is the agent's job; see
			 *	the XFER_SUCCESS case above.		*/

			break;

		case CTRL_SESSION_UP:
			{
				char	txt[512];

				msg.payload[msg.payload_len] = '\0';
				isprintf(txt, sizeof(txt),
					"[i] tcp4clo: TCPCL session up, "
					"peer=%s", msg.payload);
				writeMemo(txt);
			}

			break;

		case CTRL_SESSION_DOWN:
			writeMemo("[w] tcp4clo: TCPCL session down.");
			break;

		case CTRL_KEEPALIVE:
			/*	Peer keepalive received; no action
			 *	needed (last_recv_time updated by
			 *	ctrl_recv).				*/
			break;

		default:
			{
				char	txt[128];

				isprintf(txt, sizeof(txt),
					"[w] tcp4clo: unknown control "
					"msg type 0x%02x", msg.type);
				writeMemo(txt);
			}

			break;
		}
	}

	return NULL;
}

/*	*	*	Main thread functions	*	*	*	*/

#if defined(ION_LWT)
int	tcp4clo(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	*ductName = (char *) a1;
#else
int	main(int argc, char *argv[])
{
	char	*ductName = (argc > 1 ? argv[1] : NULL);
#endif
	VOutduct		*vduct;
	PsmAddress		vductElt;
	Sdr			sdr;
	Outduct			outduct;
	SdrObject		planDuctList;
	SdrObject		planObj = 0;
	BpPlan			plan;
	Tcp4Config		cfg;
	IonTransferPool		tx_pool;
	CtrlChannel		tx_ctrl;
	Tcp4InFlight		inflight[TCP4CLA_MAX_INFLIGHT];
	pthread_mutex_t		inflight_mutex;
	uint64_t		xfer_id_counter = 1;
	int			running = 1;
	int			agent_connected = 1;
	ResponseThreadParms	rtp;
	pthread_t		responseThread;
	SdrObject		bundleZco;
	BpAncillaryData		ancillaryData;
	unsigned int		bundleLength;
	int			slot;
	int			slot_added;
	int			bytesExtracted;
	CtrlMsg			ctrlMsg;

	if (ductName == NULL)
	{
		PUTS("Usage: tcp4clo <duct name>");
		return 0;
	}

	if (bpAttach() < 0)
	{
		putErrmsg("tcp4clo can't attach to BP.", NULL);
		return -1;
	}

	findOutduct("tcp4", ductName, &vduct, &vductElt);
	if (vductElt == 0)
	{
		putErrmsg("No such tcp4 duct.", ductName);
		return -1;
	}

	if (vduct->cloPid != ERROR && vduct->cloPid != sm_TaskIdSelf())
	{
		putErrmsg("CLO task is already started for this duct.",
				itoa(vduct->cloPid));
		return -1;
	}

	/*	All arguments validated.				*/

	sdr = getIonsdr();
	CHKZERO(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &outduct,
			sdr_list_data(sdr, vduct->outductElt),
			sizeof(Outduct));

	if (outduct.planDuctListElt)
	{
		planDuctList = sdr_list_list(sdr, outduct.planDuctListElt);
		planObj = sdr_list_user_data(sdr, planDuctList);
		if (planObj)
		{
			sdr_read(sdr, (char *) &plan, planObj,
					sizeof(BpPlan));
		}
	}

	sdr_exit_xn(sdr);

	/*	Parse config and set up IPC.  This must precede the
	 *	maxPayloadLen auto-configuration below, which depends on
	 *	the (possibly environment-overridden) slot capacity.	*/

	if (tcp4_parse_config(ductName, &cfg) < 0)
	{
		putErrmsg("tcp4clo: can't parse config.", ductName);
		return -1;
	}

	if (tcp4_apply_pool_overrides(&cfg, getenv("TCP4_POOL_SLOTS"),
			getenv("TCP4_POOL_SLOT_BYTES")) < 0)
	{
		putErrmsg("tcp4clo: invalid pool geometry override.",
				ductName);
		return -1;
	}

	/*	Auto-configure maxPayloadLen to enable BP fragmentation.
	 *	If maxPayloadLen is 0 (unlimited), set it to the runtime
	 *	slot capacity minus BP header margin so that bpclm will
	 *	automatically fragment bundles larger than the slot size.
	 *
	 *	Note: We must subtract TCP4CLA_BP_HEADER_MARGIN because
	 *	BP bundle headers (primary block, payload headers) add
	 *	~90-300 bytes per fragment. Without this margin, fragments
	 *	would exceed slot capacity and be rejected.		*/

	if (outduct.maxPayloadLen == 0)
	{
		char	txt[256];
		unsigned int safeMaxPayload;

		safeMaxPayload = cfg.slot_data_capacity
				- TCP4CLA_BP_HEADER_MARGIN;

		CHKZERO(sdr_begin_xn(sdr));
		sdr_stage(sdr, (char *) &outduct,
				sdr_list_data(sdr, vduct->outductElt),
				sizeof(Outduct));
		outduct.maxPayloadLen = safeMaxPayload;
		sdr_write(sdr, sdr_list_data(sdr, vduct->outductElt),
				(char *) &outduct, sizeof(Outduct));
		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("tcp4clo: can't update outduct.", ductName);
			return -1;
		}

		isprintf(txt, sizeof(txt),
			"[i] tcp4clo: Auto-set maxPayloadLen=%u (slot %u B - %u B "
			"margin) to enable BP fragmentation for bundles > slot "
			"size.", safeMaxPayload, cfg.slot_data_capacity,
			TCP4CLA_BP_HEADER_MARGIN);
		writeMemo(txt);
	}

	if (pool_create(cfg.tx_pool_name, cfg.num_slots,
			cfg.slot_data_capacity, &tx_pool) < 0)
	{
		putErrmsg("tcp4clo: can't create TX pool.",
				cfg.tx_pool_name);
		return -1;
	}

	{
		char	geo[128];

		isprintf(geo, sizeof geo, "[i] tcp4clo TX pool '%s': %u slots "
				"x %u bytes", cfg.tx_pool_name, cfg.num_slots,
				cfg.slot_data_capacity);
		writeMemo(geo);
	}

	if (ctrl_server_create(&tx_ctrl, cfg.tx_ctrl_path,
			TCP4CLA_KEEPALIVE_INTERVAL,
			TCP4CLA_KEEPALIVE_TIMEOUT) < 0)
	{
		putErrmsg("tcp4clo: can't create control socket.",
				cfg.tx_ctrl_path);
		pool_destroy(&tx_pool);
		return -1;
	}

	{
		char	txt[512];

		isprintf(txt, sizeof(txt),
				"[i] tcp4clo waiting for agent on %s ...",
				cfg.tx_ctrl_path);
		writeMemo(txt);
	}

	if (ctrl_server_accept(&tx_ctrl) < 0)
	{
		putErrmsg("tcp4clo: agent connection failed.", NULL);
		ctrl_destroy(&tx_ctrl);
		pool_destroy(&tx_pool);
		return -1;
	}

	writeMemo("[i] tcp4clo: agent connected.");

	/*	Initialize in-flight table and mutex.			*/

	tcp4_inflight_init(inflight, TCP4CLA_MAX_INFLIGHT);
	pthread_mutex_init(&inflight_mutex, NULL);

	/*	Set up signal handling.  SIGTERM is shutdown signal.	*/

	oK(tcp4cloSemaphore(&(vduct->semaphore)));
	isignal(SIGTERM, shutDownClo);

	/*	Start the response thread.				*/

	rtp.running = &running;
	rtp.agent_connected = &agent_connected;
	rtp.ctrl_ch = &tx_ctrl;
	rtp.tx_pool = &tx_pool;
	rtp.inflight = inflight;
	rtp.inflight_size = TCP4CLA_MAX_INFLIGHT;
	rtp.inflight_mutex = &inflight_mutex;

	if (pthread_begin(&responseThread, NULL, handleResponses, &rtp))
	{
		putSysErrmsg("tcp4clo can't create response thread", NULL);
		ctrl_destroy(&tx_ctrl);
		pool_destroy(&tx_pool);
		pthread_mutex_destroy(&inflight_mutex);
		return -1;
	}

	/*	Main dequeue loop.					*/

	{
		char	txt[512];

		isprintf(txt, sizeof(txt),
				"[i] tcp4clo is running, duct = '%s'",
				ductName);
		writeMemo(txt);
	}

	while (!(sm_SemEnded(vduct->semaphore)))
	{
		/*	Never consume bundles while the agent is down:
		 *	a transfer started now could not be announced
		 *	to any agent, stranding the bundle.  Leaving
		 *	the bundles queued in BP costs nothing and
		 *	they flow normally on reconnect.		*/

		if (!agent_connected)
		{
			microsnooze(100000);	/*  100ms.		*/
			continue;
		}

		/*	timeoutInterval -1 marks this as a reliable
		 *	CLA (like stcpclo/tcpcli), so bpDequeue
		 *	accepts stewardship: ION retains the bundle
		 *	until we report its disposition via
		 *	bpHandleXmitSuccess or bpHandleXmitFailure
		 *	(called asynchronously from the response
		 *	thread when the agent reports the transfer
		 *	outcome), enabling reforwarding of failed
		 *	transfers.  Any non-negative value here would
		 *	silently disable stewardship and turn every
		 *	transmission failure into bundle loss.		*/

		if (bpDequeue(vduct, &bundleZco, &ancillaryData, -1) < 0)
		{
			putErrmsg("tcp4clo: can't dequeue bundle.", NULL);
			break;
		}

		if (bundleZco == 0)		/*  Outduct closed.	*/
		{
			writeMemo("[i] tcp4clo outduct closed.");
			sm_SemEnd(tcp4cloSemaphore(NULL));
			continue;
		}

		if (bundleZco == 1)		/*  Corrupt bundle.	*/
		{
			continue;
		}

		CHKZERO(sdr_begin_xn(sdr));
		bundleLength = zco_length(sdr, bundleZco);
		sdr_exit_xn(sdr);

		/*	Check if bundle fits in a slot.			*/

		if (bundleLength > cfg.slot_data_capacity)
		{
			char	txt[512];

			isprintf(txt, sizeof(txt),
				"[w] tcp4clo: bundle %u bytes exceeds "
				"slot capacity %u (%u MB). This should not happen "
				"if maxPayloadLen is set correctly. "
				"Check .bprc: 'a outduct tcp4 %s tcp4clo %u'",
				bundleLength, cfg.slot_data_capacity,
				cfg.slot_data_capacity / (1024*1024),
				ductName, cfg.slot_data_capacity);
			writeMemo(txt);
			oK(bpHandleXmitFailure(bundleZco));
			continue;
		}

		/*	Acquire a shared memory slot.  Spin with
		 *	snooze if all slots are occupied (natural
		 *	backpressure from the agent).			*/

		while ((slot = pool_slot_acquire(&tx_pool)) < 0)
		{
			if (sm_SemEnded(vduct->semaphore))
			{
				break;
			}

			microsnooze(50000);	/*  50ms backoff.	*/
		}

		if (slot < 0)
		{
			/*	Shutting down; fail this bundle.	*/

			oK(bpHandleXmitFailure(bundleZco));
			break;
		}

		/*	Extract bundle bytes: SDR -> shared memory
		 *	(one copy).					*/

		bytesExtracted = tcp4_extract_bundle(sdr, bundleZco,
				&tx_pool, slot);
		if (bytesExtracted < 0)
		{
			putErrmsg("tcp4clo: can't extract bundle.", NULL);
			pool_slot_force_free(&tx_pool, slot);
			oK(bpHandleXmitFailure(bundleZco));
			continue;
		}

		/*	Publish the slot and notify the agent.		*/

		pool_slot_publish(&tx_pool, slot,
				(uint64_t) bytesExtracted,
				xfer_id_counter);

		/*	A full in-flight table is backpressure, not a
		 *	transmission failure: the agent's dispositions
		 *	(processed by the response thread) drain it.
		 *	Wait for space; failing the bundle here would
		 *	take it out of the data flow even though the
		 *	session is healthy.				*/

		for (;;)
		{
			pthread_mutex_lock(&inflight_mutex);
			if (tcp4_inflight_add(inflight,
					TCP4CLA_MAX_INFLIGHT, bundleZco,
					xfer_id_counter) == 0)
			{
				pthread_mutex_unlock(&inflight_mutex);
				slot_added = 1;
				break;
			}

			pthread_mutex_unlock(&inflight_mutex);
			if (sm_SemEnded(vduct->semaphore))
			{
				slot_added = 0;
				break;
			}

			microsnooze(10000);	/*  10ms backoff.	*/
		}

		if (!slot_added)
		{
			/*	Shutting down; fail this bundle.	*/

			pool_slot_force_free(&tx_pool, slot);
			oK(bpHandleXmitFailure(bundleZco));
			break;
		}

		ctrl_msg_init(&ctrlMsg, CTRL_SLOT_READY, (uint16_t) slot,
				xfer_id_counter, NULL, 0);
		if (ctrl_send(&tx_ctrl, &ctrlMsg) < 0)
		{
			/*	Agent died between the connectivity
			 *	check and the send: no agent will ever
			 *	learn of this transfer, so unwind it
			 *	and let BP reforward the bundle once
			 *	the agent is back.  The bundle is
			 *	failed only if we win the race to
			 *	remove it from the in-flight table;
			 *	if the response thread's fail_all got
			 *	there first, it has already handled
			 *	the failure, and failing the same ZCO
			 *	twice would corrupt the requeued
			 *	bundle.					*/

			SdrObject unwound;

			writeMemo("[w] tcp4clo: can't send SLOT_READY; "
					"unwinding transfer.");
			pthread_mutex_lock(&inflight_mutex);
			unwound = tcp4_inflight_remove(inflight,
					TCP4CLA_MAX_INFLIGHT,
					xfer_id_counter);
			pthread_mutex_unlock(&inflight_mutex);
			pool_slot_force_free(&tx_pool, slot);
			if (unwound != 0)
			{
				oK(bpHandleXmitFailure(unwound));
			}
		}

		xfer_id_counter++;
		sm_TaskYield();
	}

	/*	Shutdown.						*/

	running = 0;
	pthread_join(responseThread, NULL);

	/*	Fail any remaining in-flight transfers.			*/

	pthread_mutex_lock(&inflight_mutex);
	tcp4_inflight_fail_all(inflight, TCP4CLA_MAX_INFLIGHT);
	pthread_mutex_unlock(&inflight_mutex);

	pthread_mutex_destroy(&inflight_mutex);
	ctrl_destroy(&tx_ctrl);
	pool_destroy(&tx_pool);

	writeErrmsgMemos();
	writeMemo("[i] tcp4clo duct has ended.");
	ionDetach();
	return 0;
}
