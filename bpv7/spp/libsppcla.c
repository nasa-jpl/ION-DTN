/*
	libsppcla.c:	common functions for BP SPP-based
			convergence-layer daemons.

	Author: Gregory Miles

	Copyright (c) 2025, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

									*/
#include "sppcla.h"

/*	*	*	Sender functions	*	*	*	*/

int	sendBytesBySPP(int length,unsigned char *buffer, struct SppConfig *sppcfg, size_t toSendBytes)
{
	size_t bytesWritten = 0;
	size_t sppheadersize = 6;

	/* Parameter intentionally unused. */
	(void) length;

	toSendBytes += sppheadersize;
	bytesWritten = sppcfg->packet_request(buffer, sppcfg->apid,
			sppcfg->seq_count, sppcfg->packet_type, sppcfg->sec_header_flag, toSendBytes);

	return bytesWritten;
}

int sendBundleBySPP(unsigned int bundleLength, SdrObject bundleZco,
		unsigned char *buffer, struct SppConfig *sppcfg)
{
	Sdr	  sdr;
	ZcoReader reader;
	int	  bytesToSend;
	int	  bytesSent = 0;
	int	  sppHeaderBytes = 6;

	if (bundleLength > SPPCLA_BUFSZ)
	{
		putErrmsg("Bundle is too big for UDP CLA.", itoa(bundleLength));
		return -1;
	}

	/*	Send the bundle via Space Packet.		*/

	sdr = getIonsdr();
	zco_start_transmitting(bundleZco, &reader);
	zco_track_file_offset(&reader);
	CHKERR(sdr_begin_xn(sdr));
	bytesToSend = zco_transmit(sdr, &reader, SPPCLA_BUFSZ, (char *) buffer);

	if (sdr_end_xn(sdr) < 0 || bytesToSend < 0)
	{
		putErrmsg("Can't issue from ZCO.", NULL);
		return -1;
	}

	bytesSent = sendBytesBySPP(bundleLength, buffer, sppcfg, (size_t) bytesToSend);
	bytesSent -= sppHeaderBytes;

	if (bytesSent < 0)
	{
		if (bpHandleXmitFailure(bundleZco) < 0)
		{
			putErrmsg("Can't handle xmit failure.", NULL);
			return -1;
		}
	}
	else
	{
		if (bpHandleXmitSuccess(bundleZco) < 0)
		{
			putErrmsg("Can't handle xmit success.", NULL);
			return -1;
		}
	}

	return bytesSent;
}
