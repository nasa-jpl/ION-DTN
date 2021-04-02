/*
	cpmd.c:	contact plan management daemon for ION.

	Author: Scott Burleigh, JPL

	Copyright (c) 2021, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "imcfw.h"

static uaddr	_running(uaddr *newValue)
{
	void	*value;
	uaddr	state;
	
	if (newValue)			/*	Changing state.		*/
	{
		value = (void *) (*newValue);
		state = (uaddr) sm_TaskVar(&value);
	}
	else				/*	Just check.		*/
	{
		state = (uaddr) sm_TaskVar(NULL);
	}

	return state;
}

static void	shutDown(int signum)	/*	Stops cpmd.		*/
{
	uaddr	stop = 0;

	oK(_running(&stop));	/*	Terminates cpmd.		*/
}

#if defined (ION_LWT)
int	cpmd(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
#else
int	main(int argc, char *argv[])
{
#endif
	Sdr		sdr;
	Object		iondbObj;
	IonDB		iondb;
	uaddr		start = 1;
	Object		elt;
	Object		addr;
	CpmNotice	notice;
	unsigned char	buffer[128];
	unsigned char	*cursor;
	uvast		uvtemp;
	int		noticeLength;

	if (imcInit() < 0)
	{
		putErrmsg("cpmd can't attach to IMC database.", NULL);
		return -1;
	}

	sdr = getIonsdr();
	CHKERR(sdr);
	iondbObj = getIonDbObject();
	CHKERR(iondbObj);
	sdr_read(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	isignal(SIGTERM, shutDown);

	/*	Main loop: snooze 1 second, then drain queue of pending
	 *	contact plana management notices.			*/

	oK(_running(&start));
	writeMemo("[i] cpmd is running.");
	while (_running(NULL))
	{
		/*	Sleep for 1 second, then multicast all pending
		 *	CPM notices to propagate changes in contact
		 *	plans.						*/

		snooze(1);
		if (!sdr_begin_xn(sdr))
		{
			putErrmsg("cpmd failed to begin new transaction.",
					NULL);
			break;
		}

		while (1)
		{
			elt = sdr_list_first(sdr, iondb.cpmNotices);
			if (elt == 0)
			{
				break;	/*	No more to send.	*/
			}

			addr = sdr_list_data(sdr, elt);
			sdr_read(sdr, (char *) &notice, addr,
					sizeof(CpmNotice));

			/*	Use buffer to serialize CPM notice.	*/

			cursor = buffer;

			/*	Notice is an array of 8 items.		*/

			uvtemp = 8;
			oK(cbor_encode_array_open(uvtemp, &cursor));

			/*	Second item of array is dispatch type.	*/

			uvtemp = CpmDispatch;
			oK(cbor_encode_integer(uvtemp, &cursor));

			/*	Next is region number.			*/

			uvtemp = notice.regionNbr;
			oK(cbor_encode_integer(uvtemp, &cursor));

			/*	Then From time.				*/

			uvtemp = notice.fromTime;
			oK(cbor_encode_integer(uvtemp, &cursor));

			/*	Then To time.				*/

			uvtemp = notice.toTime;
			oK(cbor_encode_integer(uvtemp, &cursor));

			/*	Then From node.				*/

			uvtemp = notice.fromNode;
			oK(cbor_encode_integer(uvtemp, &cursor));

			/*	Then To node.				*/

			uvtemp = notice.toNode;
			oK(cbor_encode_integer(uvtemp, &cursor));

			/*	Then magnitude.				*/

			uvtemp = notice.magnitude;
			oK(cbor_encode_integer(uvtemp, &cursor));

			/*	Finally confidence, which we convert
			 *	from a float (max 4.0) to an integer
			 *	percentage (max 400).			*/

			uvtemp = (notice.confidence >= 4.0 ? 400 :
					(uvast) (notice.confidence * 100.0));
			oK(cbor_encode_integer(uvtemp, &cursor));

			/*	Now multicast the notice.		*/

			noticeLength = cursor - buffer;
			if (imcSendDispatch(notice.regionNbr, buffer,
					noticeLength) < 0)
			{
				putErrmsg("Failed sending CPM notice.", NULL);
				break;
			}
		}

		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't send contact plan updates.", NULL);
			break;
		}
	}

	writeErrmsgMemos();
	writeMemo("[i] cpmd has ended.");
	ionDetach();
	return 0;
}
