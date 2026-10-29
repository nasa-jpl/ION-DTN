// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026, California Institute of Technology. ALL RIGHTS RESERVED. U.S. Government sponsorship acknowledged.
/*
 * tests/bpsec/bsl-reattach.bsl/dotest.c: regression test for BSL state across
 *     bp_detach() and bp_attach().
 *
 * The node's BSL policy applies a BCB to the payload of every bundle sent from
 * ipn:1.1 to ipn:1.2 and has no acceptor, so a payload delivered as ciphertext
 * shows that BSL processed the bundle in the sending process.
 *
 * Case 1: a bundle sent after the first bp_attach() is encrypted.
 * Case 2: a bundle sent after bp_detach() and a second bp_attach() in the same
 * process is encrypted.
 * Case 3: threads that each send one bundle and exit do not grow ION working
 * memory.
 */

#include "bp.h"
#include "ion.h"
#include "platform.h"
#include "psm.h"
#include "sdrmgt.h"
#include "sdrxn.h"
#include "zco.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "check.h"
#include "testutil.h"

#define SENDER_THREADS (50)
#define RECEIVE_WAIT   (30)

/*
 * Working memory growth allowed for SENDER_THREADS sender threads; each leaked
 * primary block buffer occupies about 64 bytes.
 */
#define MAX_GROWTH     (SENDER_THREADS * 40)

static char txEid[] = "ipn:1.1";
static char rxEid[] = "ipn:1.2";
static char payload[] = "BSL re-attach regression test payload";

static void sendOne(BpSAP sap)
{
	Sdr	  sdr = bp_get_sdr();
	SdrObject extent;
	SdrObject zco;
	SdrObject newBundle;

	fail_unless(sdr_begin_xn(sdr));
	extent = sdr_malloc(sdr, sizeof(payload));
	fail_unless(extent != 0);
	sdr_write(sdr, extent, payload, sizeof(payload));
	zco = ionCreateZco(ZcoSdrSource, extent, 0, sizeof(payload), 0, 0, 0,
			NULL);
	fail_unless(sdr_end_xn(sdr) >= 0 && zco != 0);
	fail_unless(bp_send(sap, rxEid, NULL, 300, BP_STD_PRIORITY,
				    NoCustodyRequested, 0, 0, NULL, zco,
				    &newBundle) > 0);
}

/*
 * Receives one bundle and returns 1 if its payload differs from the plaintext
 * that was sent.
 */
static int receiveEncrypted(BpSAP sap)
{
	Sdr	   sdr = bp_get_sdr();
	BpDelivery dlv;
	ZcoReader  reader;
	int	   length;
	char	   content[sizeof(payload)];

	do
	{
		fail_unless(bp_receive(sap, &dlv, RECEIVE_WAIT) >= 0);
	} while (dlv.result == BpReceptionInterrupted);

	fail_unless(dlv.result == BpPayloadPresent, "No bundle delivered.");
	fail_unless(sdr_begin_xn(sdr));
	length = zco_source_data_length(sdr, dlv.adu);
	fail_unless(length == sizeof(payload));
	zco_start_receiving(dlv.adu, &reader);
	fail_unless(zco_receive_source(sdr, &reader, sizeof(content), content) ==
			sizeof(content));
	fail_unless(sdr_end_xn(sdr) >= 0);
	bp_release_delivery(&dlv, 1);
	return memcmp(content, payload, sizeof(payload)) != 0;
}

static void *sendFromThread(void *arg)
{
	sendOne((BpSAP) arg);
	return NULL;
}

static size_t wmAllocated(void)
{
	PsmUsageSummary usage;

	psm_usage(getIonwm(), &usage);
	return usage.smallPoolAllocated + usage.largePoolAllocated;
}

int main(int argc, char **argv)
{
	BpSAP	  txSap;
	BpSAP	  rxSap;
	pthread_t sender;
	size_t	  before;
	size_t	  after;
	int	  i;

	(void) argc;

	synch_killm(60);
	ionstart("node.ionrc", NULL, "node.ltprc", "node.bprc", "node.ipnrc",
			NULL);

	fail_unless(bp_attach() >= 0);
	fail_unless(bp_open(txEid, &txSap) == 0);
	fail_unless(bp_open(rxEid, &rxSap) == 0);

	fprintf(stderr, "[case 1] send after first attach\n");
	sendOne(txSap);
	fail_unless(receiveEncrypted(rxSap),
			"Bundle sent after first attach is not encrypted.");

	fprintf(stderr, "[case 2] send after detach and re-attach\n");
	bp_close(txSap);
	bp_close(rxSap);
	bp_detach();
	fail_unless(bp_attach() >= 0);
	fail_unless(bp_open(txEid, &txSap) == 0);
	fail_unless(bp_open(rxEid, &rxSap) == 0);
	sendOne(txSap);
	fail_unless(receiveEncrypted(rxSap),
			"Bundle sent after re-attach is not encrypted.");

	fprintf(stderr, "[case 3] %d sender threads\n", SENDER_THREADS);
	snooze(2);
	before = wmAllocated();
	for (i = 0; i < SENDER_THREADS; i++)
	{
		fail_unless(pthread_create(&sender, NULL, sendFromThread,
					    txSap) == 0);
		fail_unless(pthread_join(sender, NULL) == 0);
	}

	for (i = 0; i < SENDER_THREADS; i++)
	{
		fail_unless(receiveEncrypted(rxSap),
				"Bundle sent from thread is not encrypted.");
	}

	snooze(2);
	after = wmAllocated();
	fprintf(stderr, "[case 3] working memory %zu -> %zu\n", before, after);
	fail_unless(after < before + MAX_GROWTH,
			"Working memory grew with sender threads.");

	bp_close(txSap);
	bp_close(rxSap);
	writeErrmsgMemos();
	bp_detach();

	ionstop();

	return check_summary(argv[0]);
}
