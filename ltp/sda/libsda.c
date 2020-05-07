/*
 *	libsda.c:	functions enabling the implementation of
 *			LTP-SDA applications.
 *
 *	Copyright (c) 2013, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 *
 */

#include "platform.h"
#include "zco.h"
#include "ion.h"
#include "sda.h"

#define	SdaLtpClientId	(2)

static int	_running(int *newState)
{
	static int	state = 0;

	if (newState)
	{
		state = *newState;
	}

	return state;
}

static void	handleQuit(int signum)
{
	int	stop = 0;

	oK(_running(&stop));
	ltp_interrupt(SdaLtpClientId);
}

int	sda_send(uvast destinationEngineId, unsigned int clientSvcId,
		Object clientServiceData)
{
	Sdr		sdr = getIonsdr();
	Sdnv		sdnvBuf;
	Object		sdaZco;
	LtpSessionId	sessionId;

	CHKERR(destinationEngineId);
	CHKERR(clientSvcId);
	CHKERR(clientServiceData);
	encodeSdnv(&sdnvBuf, clientSvcId);
	CHKERR(sdr_begin_xn(sdr));
	sdaZco = zco_clone(sdr, clientServiceData, 0,
			zco_source_data_length(sdr, clientServiceData));
	if (sdaZco != (Object) ERROR && sdaZco != 0)
	{
		oK(zco_prepend_header(sdr, sdaZco, (char *) sdnvBuf.text,
				sdnvBuf.length));
		oK(zco_bond(sdr, sdaZco));
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't prepend client service ID.", NULL);
		return -1;
	}

	switch (ltp_send(destinationEngineId, SdaLtpClientId, sdaZco,
			LTP_ALL_RED, &sessionId))
	{
	case 0:
		putErrmsg("Unable to send service data item via LTP.", NULL);
		break;

	case -1:
		putErrmsg("sd_send failed.", NULL);
		return -1;
	}

	/*	Note: sdaZco is destroyed later, when LTP's
	 *	ExportSession is closed following transmission of
	 *	SDA ZCOs as aggregated into a block.			*/

	return 0;
}

static int	receiveSdaItems(SdaDelimiterFn delimiter, SdaHandlerFn handler,
			Object zco, uvast senderEngineNbr)
{
	Sdr		sdr = getIonsdr();
	uvast		bytesHandled = 0;
	ZcoReader	reader;
	vast		bytesReceived;
	unsigned char	buffer[2048];
	int		offset;
	uvast		clientId;
	vast		itemLength;
	Object		itemZco;

	zco_start_receiving(zco, &reader);
	while (1)
	{ 
		/*	Get the first (up to) 2048 bytes of the
		 *	unprocessed remainder of the LTP service
		 *	data item.					*/

		bytesReceived = zco_receive_source(sdr, &reader, sizeof buffer,
				(char *) buffer);
		switch (bytesReceived)
		{
		case -1:
			putErrmsg("Can't begin acquisition of SDA item.", NULL);
			return -1;

		case 0:
			return 0;	/*	No more to acquire.	*/
		}

		/*	Get the client ID of the client data unit at
		 *	the start of the buffer.			*/

		offset = decodeSdnv(&clientId, buffer);
		if (offset == 0)
		{
			writeMemo("[?] No SDA item at start of LTP block.");
			return 0;	/*	No more to acquire.	*/
		}

		/*	Skip over the client ID, then call a user
		 *	function to get the length of the client data
		 *	unit.						*/

		bytesHandled += offset;
		itemLength = delimiter(clientId, buffer + offset,
				bytesReceived - offset);
		switch (itemLength)
		{
		case -1:
			putErrmsg("Failure calculating SDA item length.", NULL);
			return -1;

		case 0:
			writeMemo("[?] Invalid SDA item in LTP block.");
			return 0;	/*	No more to acquire.	*/
		}

		/*	Clone the client data unit from the LTP
		 *	service data item.				*/

		itemZco = zco_clone(sdr, zco, bytesHandled, itemLength);
		if (itemZco == 0)
		{
			putErrmsg("Failure extracting SDA item.", NULL);
			return -1;
		}

		/*	Call a user function to handle the client
		 *	data unit.					*/

		if (handler(senderEngineNbr, clientId, itemZco) < 0)
		{
			putErrmsg("Failure handling SDA item.", NULL);
			return -1;
		}

		zco_destroy(sdr, itemZco);
		bytesHandled += itemLength;

		/*	To extract next item, first skip over all
		 *	bytes of the LTP service data item that have
		 *	already been handled.				*/

		zco_start_receiving(zco, &reader);
		switch (zco_receive_source(sdr, &reader, bytesHandled, NULL))
		{
		case -1:
			putErrmsg("Can't skip over handled items.", NULL);
			return -1;

		case 0:
			putSysErrmsg("LTP-SDA block file access error.", NULL);
			return 0;	/*	No more to acquire.	*/
		}
	}
}

int	sda_run(SdaDelimiterFn delimiter, SdaHandlerFn handler)
{
	Sdr		sdr;
	int		state = 1;
	LtpNoticeType	type;
	LtpSessionId	sessionId;
	unsigned char	reasonCode;
	unsigned char	endOfBlock;
	unsigned int	dataOffset;
	unsigned int	dataLength;
	Object		data;

	if (ltp_attach() < 0)
	{
		putErrmsg("SDA can't initialize LTP.", NULL);
		return -1;
	}

	if (ltp_open(SdaLtpClientId) < 0)
	{
		putErrmsg("SDA can't open client access.",
				itoa(SdaLtpClientId));
		return -1;
	}

	sdr = getIonsdr();

	/*	Can now start receiving notices.  On failure,
	 *	terminate SDA.						*/

	isignal(SIGINT, handleQuit);
	oK((_running(&state)));
	state = 0;	/*	Prepare for stop.			*/
	while (_running(NULL))
	{
		if (ltp_get_notice(SdaLtpClientId, &type, &sessionId,
				&reasonCode, &endOfBlock, &dataOffset,
				&dataLength, &data) < 0)
		{
			writeMemo("[?] SDA failed getting LTP notice.");
			oK((_running(&state)));
			continue;
		}

		switch (type)
		{
		case LtpExportSessionComplete:	/*	Xmit success.	*/
		case LtpExportSessionCanceled:	/*	Xmit failure.	*/
			if (data == 0)		/*	Ignore it.	*/
			{
				break;		/*	Out of switch.	*/
			}

			CHKERR(sdr_begin_xn(sdr));
			zco_destroy(sdr, data);
			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Crashed on data cleanup.", NULL);
				oK((_running(&state)));
			}

			break;		/*	Out of switch.		*/

		case LtpImportSessionCanceled:
			/*	None of the red data for the import
			 *	session (if any) have been received
			 *	yet, so nothing to discard.		*/

			break;		/*	Out of switch.		*/

		case LtpRecvRedPart:
			if (!endOfBlock)
			{
				/*	Block is partially red and
				 *	partially green, an error.	*/

				writeMemo("[?] SDA block partially green.");
				CHKERR(sdr_begin_xn(sdr));
				zco_destroy(sdr, data);
				if (sdr_end_xn(sdr) < 0)
				{
					putErrmsg("Can't destroy block.", NULL);
					oK((_running(&state)));
				}

				break;		/*	Out of switch.	*/
			}

			CHKERR(sdr_begin_xn(sdr));
			if (receiveSdaItems(delimiter, handler, data,
					sessionId.sourceEngineId) < 0)
			{
				putErrmsg("Can't acquire SDA item(s).", NULL);
				oK((_running(&state)));
			}

			zco_destroy(sdr, data);
			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Can't release block.", NULL);
				oK((_running(&state)));
			}

			break;		/*	Out of switch.		*/

		case LtpRecvGreenSegment:
			writeMemo("[?] SDA received a green segment.");
			CHKERR(sdr_begin_xn(sdr));
			zco_destroy(sdr, data);
			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Can't destroy item.", NULL);
				oK((_running(&state)));
			}

			break;		/*	Out of switch.		*/

		default:
			break;		/*	Out of switch.		*/
		}

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	writeErrmsgMemos();
	writeMemo("[i] SDA reception has ended.");

	/*	Free resources.						*/

	ltp_close(SdaLtpClientId);
	return 0;
}

void	sda_interrupt()
{
	ltp_interrupt(SdaLtpClientId);
}
