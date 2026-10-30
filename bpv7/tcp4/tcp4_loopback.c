/*
	tcp4_loopback.c:	Loopback bridge for TCP4 CLA testing.

			Connects to both the TX and RX control
			channels and shared memory pools.  When
			a SLOT_READY arrives on the TX channel,
			copies the data from the TX pool to the
			RX pool and sends BUNDLE_RECV on the RX
			channel, then sends XFER_SUCCESS on the
			TX channel.

			This enables end-to-end testing of the
			TCP4 CLA without the Python TCPCLv4 agent.

	Author: iondev33

	Copyright (c) 2026, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
								*/

#include "ion_transfer_pool.h"
#include "ion_ctrl_channel.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

static volatile int	running = 1;

static void	handleSignal(int signum)
{
	(void)signum;
	running = 0;
}

static void	usage(void)
{
	fprintf(stderr, "Usage: tcp4_loopback <duct_name> "
			"[max_retries]\n");
}

int	main(int argc, char *argv[])
{
	char		*ductName;
	int		maxRetries = 30;
	char		txPoolName[256];
	char		rxPoolName[256];
	char		txCtrlPath[256];
	char		rxCtrlPath[256];
	IonTransferPool	txPool;
	IonTransferPool	rxPool;
	CtrlChannel	txCtrl;
	CtrlChannel	rxCtrl;
	int		retries;
	CtrlMsg		msg;
	CtrlMsg		reply;
	int		ret;

	if (argc < 2)
	{
		usage();
		return 1;
	}

	ductName = argv[1];
	if (argc > 2)
	{
		maxRetries = atoi(argv[2]);
	}

	/*	Build the same names that tcp4clo/tcp4cli use.		*/

	snprintf(txPoolName, sizeof(txPoolName), "/ion_tcp4_tx_%s",
			ductName);
	snprintf(rxPoolName, sizeof(rxPoolName), "/ion_tcp4_rx_%s",
			ductName);
	snprintf(txCtrlPath, sizeof(txCtrlPath), "/tmp/ion_tcp4/tx_%s.sock",
			ductName);
	snprintf(rxCtrlPath, sizeof(rxCtrlPath), "/tmp/ion_tcp4/rx_%s.sock",
			ductName);

	signal(SIGTERM, handleSignal);
	signal(SIGINT, handleSignal);

	/*	Wait for the pools to become available, then attach.
	 *	The CLA daemons create them; we just attach.		*/

	fprintf(stdout, "[i] tcp4_loopback: attaching to TX pool %s ...\n",
			txPoolName);
	for (retries = 0; retries < maxRetries; retries++)
	{
		if (pool_attach(txPoolName, &txPool) == 0)
		{
			break;
		}

		sleep(1);
	}

	if (retries >= maxRetries)
	{
		fprintf(stderr, "tcp4_loopback: can't attach to TX pool.\n");
		return 1;
	}

	fprintf(stdout, "[i] tcp4_loopback: attaching to RX pool %s ...\n",
			rxPoolName);
	for (retries = 0; retries < maxRetries; retries++)
	{
		if (pool_attach(rxPoolName, &rxPool) == 0)
		{
			break;
		}

		sleep(1);
	}

	if (retries >= maxRetries)
	{
		fprintf(stderr, "tcp4_loopback: can't attach to RX pool.\n");
		pool_detach(&txPool);
		return 1;
	}

	/*	Connect to control channels as a client.		*/

	fprintf(stdout, "[i] tcp4_loopback: connecting to TX ctrl %s ...\n",
			txCtrlPath);
	for (retries = 0; retries < maxRetries; retries++)
	{
		if (ctrl_client_connect(&txCtrl, txCtrlPath, 15, 45) == 0)
		{
			break;
		}

		sleep(1);
	}

	if (retries >= maxRetries)
	{
		fprintf(stderr, "tcp4_loopback: can't connect to TX ctrl.\n");
		pool_detach(&txPool);
		pool_detach(&rxPool);
		return 1;
	}

	fprintf(stdout, "[i] tcp4_loopback: connecting to RX ctrl %s ...\n",
			rxCtrlPath);
	for (retries = 0; retries < maxRetries; retries++)
	{
		if (ctrl_client_connect(&rxCtrl, rxCtrlPath, 15, 45) == 0)
		{
			break;
		}

		sleep(1);
	}

	if (retries >= maxRetries)
	{
		fprintf(stderr, "tcp4_loopback: can't connect to RX ctrl.\n");
		ctrl_disconnect(&txCtrl);
		pool_detach(&txPool);
		pool_detach(&rxPool);
		return 1;
	}

	fprintf(stdout, "[i] tcp4_loopback: running, duct='%s'\n", ductName);

	/*	Main loop: relay bundles from TX pool to RX pool.	*/

	while (running)
	{
		/*	Check for messages on either control channel.	*/

		ret = ctrl_recv(&txCtrl, &msg, 500);
		if (ret < 0)
		{
			fprintf(stderr, "tcp4_loopback: TX ctrl "
					"disconnected.\n");
			break;
		}

		if (ret > 0 && msg.type == CTRL_SLOT_READY)
		{
			/*	Read the bundle from the TX slot.	*/

			char		*txData;
			uint64_t	dataLen;
			uint64_t	xferId;
			int		rxSlot;
			char		*rxData;

			txData = pool_slot_read(&txPool, msg.slot_index,
					&dataLen, &xferId);
			if (txData == NULL)
			{
				fprintf(stderr, "tcp4_loopback: TX slot "
						"%d not READY.\n",
						msg.slot_index);
				continue;
			}

			if (dataLen > pool_slot_capacity(&rxPool))
			{
				fprintf(stderr, "tcp4_loopback: bundle "
						"exceeds RX slot capacity.\n");
				pool_slot_release(&txPool, msg.slot_index);
				ctrl_msg_init(&reply, CTRL_XFER_FAILURE,
						msg.slot_index, xferId,
						NULL, 0);
				ctrl_send(&txCtrl, &reply);
				continue;
			}

			/*	Acquire an RX slot and copy data.	*/

			rxSlot = pool_slot_acquire(&rxPool);
			if (rxSlot < 0)
			{
				fprintf(stderr, "tcp4_loopback: no free "
						"RX slots.\n");

				/*	Release TX slot, fail xfer.	*/

				pool_slot_release(&txPool, msg.slot_index);
				ctrl_msg_init(&reply, CTRL_XFER_FAILURE,
						msg.slot_index, xferId,
						NULL, 0);
				ctrl_send(&txCtrl, &reply);
				continue;
			}

			rxData = pool_slot_data(&rxPool, rxSlot);
			memcpy(rxData, txData, (size_t) dataLen);

			/*	Release the TX slot (we've copied).	*/

			pool_slot_release(&txPool, msg.slot_index);

			/*	Publish the RX slot and notify the
			 *	induct daemon.				*/

			pool_slot_publish(&rxPool, rxSlot, dataLen,
					xferId);
			ctrl_msg_init(&reply, CTRL_BUNDLE_RECV,
					(uint16_t) rxSlot, xferId,
					NULL, 0);
			if (ctrl_send(&rxCtrl, &reply) < 0)
			{
				fprintf(stderr, "tcp4_loopback: can't "
						"send BUNDLE_RECV.\n");
				pool_slot_force_free(&rxPool, rxSlot);
				ctrl_msg_init(&reply, CTRL_XFER_FAILURE,
						msg.slot_index, xferId,
						NULL, 0);
				ctrl_send(&txCtrl, &reply);
				continue;
			}

			/*	Report transfer success to the
			 *	outduct daemon.				*/

			ctrl_msg_init(&reply, CTRL_XFER_SUCCESS,
					msg.slot_index, xferId, NULL, 0);
			ctrl_send(&txCtrl, &reply);
		}
		else if (ret > 0 && msg.type == CTRL_KEEPALIVE)
		{
			ctrl_send_keepalive(&txCtrl);
		}

		/*	Check for SLOT_RELEASE from the RX side.	*/

		ret = ctrl_recv(&rxCtrl, &msg, 100);
		if (ret > 0 && msg.type == CTRL_SLOT_RELEASE)
		{
			pool_slot_release(&rxPool, msg.slot_index);
		}
		else if (ret > 0 && msg.type == CTRL_KEEPALIVE)
		{
			ctrl_send_keepalive(&rxCtrl);
		}
		else if (ret < 0)
		{
			fprintf(stderr, "tcp4_loopback: RX ctrl "
					"disconnected.\n");
			break;
		}
	}

	fprintf(stdout, "[i] tcp4_loopback: shutting down.\n");

	ctrl_disconnect(&txCtrl);
	ctrl_disconnect(&rxCtrl);
	pool_detach(&txPool);
	pool_detach(&rxPool);
	return 0;
}
