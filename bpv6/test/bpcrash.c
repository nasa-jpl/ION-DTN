/*
	bpcrash.c:	a test support program that simply forces
			a transaction crash, to trigger restart.
									*/
/*									*/
/*	Copyright (c) 2019, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include <bp.h>

#if defined (ION_LWT)
int	bpcrash(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
#else
int	main(int argc, char **argv)
{
#endif
	Sdr	sdr;

	if (bp_attach() < 0)
	{
		putErrmsg("Can't attach to BP.", NULL);
		return 0;
	}

	sdr = bp_get_sdr();
	CHKZERO(sdr_begin_xn(sdr));
	sdr_cancel_xn(sdr);
	writeErrmsgMemos();
	PUTS("Stopping bpcrash.");
	bp_detach();
	return 0;
}
