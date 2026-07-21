/*
	sdrcancel.c:	deterministic test trigger for SDR transaction
	reversibility recovery.

	Attaches to a running ION instance, opens an SDR transaction,
	performs a single small write to mark the transaction as
	modified, then cancels the transaction.  This forces the
	cancel-with-modifications code path in terminateXn(), which
	must invoke reverseTransaction() and then ionrestart.

	NOT a recovery tool.  This exists only as a test trigger and
	should never be installed in a production environment.

	Author: redesigned 2026 to replace bpcrash_hard for the SDR
	reversibility recovery test.
									*/
/*									*/
/*	Copyright (c) 2026, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*									*/

#include "platform.h"
#include "ion.h"

#define	SCRATCH_SIZE	64

#if defined (ION_LWT)
int	sdrcancel(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	(void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
	(void)a6; (void)a7; (void)a8; (void)a9; (void)a10;
#else
int	main(int argc, char **argv)
{
	(void)argc; (void)argv;
#endif
	Sdr	sdr;
	SdrObject scratch;
	char	buf[SCRATCH_SIZE];

	if (ionAttach() < 0)
	{
		putErrmsg("sdrcancel: can't attach to ION.", NULL);
		writeErrmsgMemos();
		return 1;
	}

	sdr = getIonsdr();
	if (sdr == NULL)
	{
		putErrmsg("sdrcancel: can't get ION SDR.", NULL);
		writeErrmsgMemos();
		ionDetach();
		return 1;
	}

	if (sdr_begin_xn(sdr) == 0)
	{
		putErrmsg("sdrcancel: can't begin transaction.", NULL);
		writeErrmsgMemos();
		ionDetach();
		return 1;
	}

	scratch = sdr_malloc(sdr, SCRATCH_SIZE);
	if (scratch == 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("sdrcancel: malloc failed.", NULL);
		writeErrmsgMemos();
		ionDetach();
		return 1;
	}

	memset(buf, 0xA5, SCRATCH_SIZE);
	sdr_write(sdr, scratch, buf, SCRATCH_SIZE);

	putErrmsg("sdrcancel: cancelling modified transaction.", NULL);
	sdr_cancel_xn(sdr);

	writeErrmsgMemos();
	ionDetach();
	return 0;
}
