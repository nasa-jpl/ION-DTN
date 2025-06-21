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

int	sendBytesBySPP(int fd, char *from, int length)
{
    int	bytesWritten = 0;
    return bytesWritten;
}

int	sendBundleBySPP(int fd, unsigned int bundleLength,
			Object bundleZco, unsigned char *buffer)
{
    Sdr		sdr;
    ZcoReader	reader;
    int		bytesToSend;
    int		bytesSent;

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

    bytesSent = sendBytesBySPP(fd, (char *) buffer, bytesToSend);

    if (bytesSent < 0)
    {
	if (bpHandleXmitFailure(bundleZco) < 0)
	{
	    putErrmsg("Can't handle xmit failure.", NULL);
	    return -1;
	}

	// Look for other failure possiblities

	if (bpHandleXmitSuccess(bundleZco) < 0)
	{
	    putErrmsg("Can't handle xmit success.", NULL);
	    return -1;
	}
    }
    
    return bytesSent;
}

/*	*	*	Receiver functions	*	*	*	*/

int	receiveBytesBySPP(int fd, char *into, int length)
{
    	int		bytesRead = 0;

	bytesRead = read(fd, into, length);

	// Actually here have to add the functions to de-packet from the space packets
	// space packets will be read into "into" buffer

	return bytesRead;
}
