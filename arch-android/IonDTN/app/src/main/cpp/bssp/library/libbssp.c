/*
 *	libbssp.c:	functions enabling the implementation of
 *			BSSP applications.
 *
 *	Copyright (c) 2013, California Institute of Technology.
 *	Copyright (c) 2013, Space Internetworking Center,
 *	Democritus University of Thrace.
 *	
 *	All rights reserved. U.S. Government and E.U. Sponsorship acknowledged.
 *				
 *	Author: Sotirios-Angelos Lenas, Space Internetworking Center
 */

#include "bsspP.h"

int	bssp_attach()
{
	return bsspAttach();
}

void	bssp_detach()
{
#if (!(defined (ION_LWT)))
	bsspDetach();
#endif
	ionDetach();
}

int	bssp_engine_is_started()
{
	BsspVdb	*vdb = getBsspVdb();

	return (vdb && vdb->clockPid != ERROR);
}

int	bssp_send(uvast destinationEngineId, unsigned int clientSvcId,
		Object clientServiceData, int inOrder, BsspSessionId *sessionId)
{
	BsspVdb		*vdb = getBsspVdb();
	Sdr		sdr = getIonsdr();
	BsspVspan	*vspan;
	PsmAddress	vspanElt;
	unsigned int	dataLength;
	Object		spanObj;
	BsspSpan	span;
	ExportSession   session;
	int		blockIssued;

	CHKERR(clientSvcId <= MAX_BSSP_CLIENT_NBR);
	CHKERR(clientServiceData);
	CHKERR(inOrder == 0 || inOrder == 1);
	CHKERR(sessionId);
	CHKERR(sdr_begin_xn(sdr));
	findBsspSpan(destinationEngineId, &vspan, &vspanElt);
	if (vspanElt == 0)
	{
		sdr_exit_xn(sdr);
		putErrmsg("Destination BSSP engine unknown.",
				utoa(destinationEngineId));
		return -1;
	}

	dataLength = zco_length(sdr, clientServiceData);
	spanObj = sdr_list_data(sdr, vspan->spanElt);
	sdr_stage(sdr, (char *) &span, spanObj, sizeof(BsspSpan));

	/*	Every service data unit is encapculated  	*
	 *	into a single block. 				*/

	while (1)
	{
		if (span.currentExportSessionObj)
		{
			/*	Span has been initialized with a
			 *	pdu buffer (block) into which
		 	*	service data can be inserted.		*/

			if (sdr_list_length(sdr, span.exportSessions)
				< span.maxExportSessions)
			{
				break; 		/*	Out of loop.	*/
			}
		
		}
		
		/*	currentExportSessionObj not initialiazed 	* 
		 *	yet or this is an overlimit export session.	* 
		 *	Must wait for one to be released.		*/

		sdr_exit_xn(sdr);
		if (sm_SemTake(vspan->bufOpenSemaphore) < 0)
		{
			putErrmsg("Can't take buffer open semaphore.",
					itoa(vspan->engineId));
			return -1;
		}

		if (sm_SemEnded(vspan->bufOpenSemaphore))
		{
			putErrmsg("Span has been stopped.",
					itoa(vspan->engineId));
			return 0;
		}
		
		CHKERR(sdr_begin_xn(sdr));
		sdr_stage(sdr, (char *) &span, spanObj, sizeof(BsspSpan));
	}

	/*	Now append the outbound SDU to the block that is	*
	 *	currently being available for this span and give 	*
	 * 	the span's be/rl Semaphore.				*/
	sdr_stage(sdr, (char *) &session, span.currentExportSessionObj,
				sizeof(ExportSession));
	session.svcDataObject = clientServiceData;
	span.clientSvcIdOfBufferedBlock = clientSvcId;
	span.lengthOfBufferedBlock += dataLength;
	sdr_write(sdr, spanObj, (char *) &span, sizeof(BsspSpan));
	session.clientSvcId = span.clientSvcIdOfBufferedBlock;
	encodeSdnv(&(session.clientSvcIdSdnv), session.clientSvcId);
	session.totalLength = span.lengthOfBufferedBlock;

	blockIssued = issueXmitBlock(sdr, &span, vspan, &session,
				span.currentExportSessionObj, inOrder);
	switch (blockIssued)
	{
	case -1:		/*	System error.		*/
		putErrmsg("Can't issue block.", NULL);
		sdr_cancel_xn(sdr);
		return -1;

	case 0:			/*	Database too full.	*/
		sdr_cancel_xn(sdr);
		return 0;
	}

	/*	xmitBlock issuance succeeded.			*/

	if (vdb->watching & WATCH_f)
	{
		putchar('f');
		fflush(stdout);
	}
	/*	Commit changes to current session to the
	 *	database.					*/

	sdr_write(sdr, span.currentExportSessionObj, (char *) &session,
			sizeof(ExportSession));

	/*	Reinitialize span's block buffer.		*/

	span.lengthOfBufferedBlock = 0;
	span.clientSvcIdOfBufferedBlock = 0;
	span.currentExportSessionObj = 0;
	sdr_write(sdr, spanObj, (char *) &span, sizeof(BsspSpan));

	/*	Start an export session for the next block.	*/

	if (startBsspExportSession(sdr, spanObj, vspan) < 0)
	{
		putErrmsg("bssp_send can't start new session.",
				utoa(destinationEngineId));
		return -1;
	}

	if (vdb->watching & WATCH_d)
	{
		putchar('d');
		fflush(stdout);
	}

	if (sdr_end_xn(sdr))
	{
		putErrmsg("Can't send data.", NULL);
		return -1;
	}

	sessionId->sourceEngineId = vdb->ownEngineId;
	sessionId->sessionNbr = session.sessionNbr;
	return 1;
}

int	bssp_open(unsigned int clientSvcId)
{
	return bsspAttachClient(clientSvcId);
}

int	bssp_get_notice(unsigned int clientSvcId, BsspNoticeType *type,
		BsspSessionId *sessionId, unsigned char *reasonCode,
		unsigned int *dataLength, Object *data)
{
	Sdr		sdr = getIonsdr();
	BsspVdb		*vdb = getBsspVdb();
	BsspVclient	*client;
	Object		elt;
	Object		noticeAddr;
	BsspNotice	notice;

	CHKERR(clientSvcId <= MAX_BSSP_CLIENT_NBR);
	CHKERR(type);
	CHKERR(sessionId);
	CHKERR(reasonCode);
	CHKERR(dataLength);
	CHKERR(data);
	*type = BsspNoNotice;	/*	Default.			*/
	CHKERR(sdr_begin_xn(sdr));
	client = vdb->clients + clientSvcId;
	if (client->pid != sm_TaskIdSelf())
	{
		sdr_exit_xn(sdr);
		putErrmsg("Can't get notice: not owner of client service.",
				itoa(client->pid));
		return -1;
	}

	elt = sdr_list_first(sdr, client->notices);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);

		/*	Wait until BSSP engine announces an event
		 *	by giving the client's semaphore.		*/

		if (sm_SemTake(client->semaphore) < 0)
		{
			putErrmsg("BSSP client can't take semaphore.", NULL);
			return -1;
		}

		if (sm_SemEnded(client->semaphore))
		{
			writeMemo("[?] Client access terminated.");

			/*	End task, but without error.		*/

			return -1;
		}

		CHKERR(sdr_begin_xn(sdr));
		elt = sdr_list_first(sdr, client->notices);
		if (elt == 0)	/*	Function was interrupted.	*/
		{
			sdr_exit_xn(sdr);
			return 0;
		}
	}

	/*	Got next inbound notice.  Remove it from the queue
	 *	for this client.					*/

	noticeAddr = sdr_list_data(sdr, elt);
	sdr_list_delete(sdr, elt, (SdrListDeleteFn) NULL, NULL);
	sdr_read(sdr, (char *) &notice, noticeAddr, sizeof(BsspNotice));
	sdr_free(sdr, noticeAddr);

	/*	Note that an ExportSessionCanceled notice may have
	 *	associated data of zero.
	 */

	if (sdr_end_xn(sdr))
	{
		putErrmsg("Can't get inbound notice.", NULL);
		return -1;
	}

	*type = notice.type;
	sessionId->sourceEngineId = notice.sessionId.sourceEngineId;
	sessionId->sessionNbr = notice.sessionId.sessionNbr;
	*reasonCode = notice.reasonCode;
	*dataLength = notice.dataLength;
	*data = notice.data;
	return 0;
}

void	bssp_interrupt(unsigned int clientSvcId)
{
	BsspVdb		*vdb;
	BsspVclient	*client;

	if (clientSvcId <= MAX_BSSP_CLIENT_NBR)
	{
		vdb = getBsspVdb();
		client = vdb->clients + clientSvcId;
		if (client->semaphore != SM_SEM_NONE)
		{
			sm_SemGive(client->semaphore);
		}
	}
}

void	bssp_release_data(Object data)
{
	Sdr	bsspSdr = getIonsdr();

	if (data)
	{
		CHKVOID(sdr_begin_xn(bsspSdr));
		zco_destroy(bsspSdr, data);
		if (sdr_end_xn(bsspSdr) < 0)
		{
			putErrmsg("Failed releasing BSSP notice object.", NULL);
		}
	}
}

void	bssp_close(unsigned int clientSvcId)
{
	bsspDetachClient(clientSvcId);
}
