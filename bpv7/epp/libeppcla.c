/*
	libeppcla.c:	common functions for BP EPP-based
			convergence-layer daemons.

	Author: Gregory Miles

	Copyright (c) 2025, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

									*/
#include "eppcla.h"

/*	*	*	Sender functions	*	*	*	*/

int	sendBytesByEPP(unsigned char *buffer, size_t length,
		struct EppConfig *eppcfg)
{
	int	bytesWritten = 0;

	/*
	 * Call the provider's encapsulation_request function.
	 * Unlike SPP, EPP handles its own framing so we don't need
	 * to account for header bytes here.
	 */
	bytesWritten = eppcfg->encapsulation_request(buffer, length,
			eppcfg->sdlp_channel, eppcfg->epi);

	return bytesWritten;
}

int sendBundleByEPP(unsigned int bundleLength, SdrObject bundleZco,
		unsigned char *buffer, struct EppConfig *eppcfg)
{
	Sdr	  sdr;
	ZcoReader reader;
	int	  bytesToSend;
	int	  bytesSent = 0;

	if (bundleLength > EPPCLA_BUFSZ)
	{
		putErrmsg("Bundle is too big for EPP CLA buffer.",
				itoa(bundleLength));
		return -1;
	}

	/*	Send the bundle via Encapsulation Packet.		*/

	sdr = getIonsdr();
	zco_start_transmitting(bundleZco, &reader);
	zco_track_file_offset(&reader);
	CHKERR(sdr_begin_xn(sdr));
	bytesToSend = zco_transmit(sdr, &reader, EPPCLA_BUFSZ, (char *) buffer);

	if (sdr_end_xn(sdr) < 0 || bytesToSend < 0)
	{
		putErrmsg("Can't issue from ZCO.", NULL);
		return -1;
	}

	bytesSent = sendBytesByEPP(buffer, (size_t) bytesToSend, eppcfg);

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
