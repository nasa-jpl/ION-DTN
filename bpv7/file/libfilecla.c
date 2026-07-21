/*
	libfilecla.c:	common functions for BP File-based
			convergence-layer daemons.

	Author: ION Development Team

	Copyright (c) 2025, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

								*/
#include "filecla.h"

/*	*	*	Sender functions	*	*	*	*/

int	sendBytesByFile(unsigned char *buffer, size_t length,
		struct FileCloConfig *cfg)
{
	int	bytesWritten = 0;

	/*
	 * Call the provider's file_write_bundle function.
	 * The provider handles framing (length prefix + CRC).
	 */
	bytesWritten = cfg->write_bundle(buffer, length);

	return bytesWritten;
}

int	sendBundleByFile(unsigned int bundleLength, SdrObject bundleZco,
		unsigned char *buffer, struct FileCloConfig *cfg)
{
	Sdr	  sdr;
	ZcoReader reader;
	int	  bytesToSend;
	int	  bytesSent = 0;

	if (bundleLength > FILECLA_BUFSZ)
	{
		putErrmsg("Bundle is too big for file CLA buffer.",
				itoa(bundleLength));
		return -1;
	}

	/*	Send the bundle via file write.			*/

	sdr = getIonsdr();
	zco_start_transmitting(bundleZco, &reader);
	zco_track_file_offset(&reader);
	CHKERR(sdr_begin_xn(sdr));
	bytesToSend = zco_transmit(sdr, &reader, FILECLA_BUFSZ, (char *) buffer);

	if (sdr_end_xn(sdr) < 0 || bytesToSend < 0)
	{
		putErrmsg("Can't issue from ZCO.", NULL);
		return -1;
	}

	bytesSent = sendBytesByFile(buffer, (size_t) bytesToSend, cfg);

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
