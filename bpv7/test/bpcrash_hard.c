/*
	bpcrash_hard.c:	a test support program that forces a transaction
			crash WITH SDR modifications, to reliably trigger
			transaction reversal and restart in ION 4.1.3+.

	This is an enhanced version of bpcrash that ensures the
	SDR is actually modified before transaction cancellation,
	guaranteeing that transaction reversal logic is exercised.
									*/
/*									*/
/*	Copyright (c) 2024, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: based on bpcrash.c by Scott Burleigh			*/
/*									*/

#include <bp.h>

#if defined (ION_LWT)
int	bpcrash_hard(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	/* Suppress unused parameter warnings for LWT build */
	(void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
	(void)a6; (void)a7; (void)a8; (void)a9; (void)a10;
#else
int	main(void)
{
#endif
	Sdr		sdr;
	SdrObject	obj;
	char		testData[10000];
	int		i;

	if (bp_attach() < 0)
	{
		putErrmsg("bpcrash_hard: Can't attach to BP.", NULL);
		return 1;
	}

	sdr = bp_get_sdr();

	/* Initialize test data */
	for (i = 0; i < 10000; i++)
	{
		testData[i] = (char) (i % 256);
	}

	/* Begin transaction and MODIFY the SDR */
	CHKZERO(sdr_begin_xn(sdr));

	/* Allocate space in SDR - this marks the transaction as modified */
	obj = sdr_malloc(sdr, 10000);
	if (obj == 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("bpcrash_hard: Can't allocate SDR space.", NULL);
		bp_detach();
		return 1;
	}

	/* Write data to SDR - ensures sdr->modified flag is set */
	sdr_write(sdr, obj, testData, 10000);

	/*
	 * Now cancel the transaction.
	 * Since we've modified the SDR, this should trigger:
	 * 1. "Attempting transaction reversal..." message
	 * 2. Transaction reversal process
	 * 3. ionrestart execution (if restartCmd is configured)
	 */
	putErrmsg("bpcrash_hard: Forcing transaction cancellation with \
modifications.", NULL);
	putErrmsg("bpcrash_hard: Transaction aborted.", NULL);
	sdr_cancel_xn(sdr);

	writeErrmsgMemos();
	PUTS("Stopping bpcrash_hard.");
	bp_detach();

	/* Give ionrestart time to execute if triggered */
	sleep(2);

	return 0;
}
