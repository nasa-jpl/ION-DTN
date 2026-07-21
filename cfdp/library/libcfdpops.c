/*
 *	libcfdpops.c:	functions implementing CFDP standard user
 *			operations.
 *
 *	Copyright (c) 2009, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 *
 */

#include "cfdpP.h"

extern int		createFDU(CfdpNumber *destinationEntityNbr,
				unsigned int utParmsLength,
				unsigned char *utParms,
				char *sourceFileName,
				char *destFileName,
				CfdpReaderFn readerFn,
				CfdpMetadataFn metadataFn,
				CfdpHandler *faultHandlers,
				int flowLabelLength,
				unsigned char *flowLabel,
				unsigned int closureLatency,
				MetadataList messagesToUser,
				MetadataList filestoreRequests,
				CfdpTransactionId *originatingTransactionId,
				CfdpTransactionId *transactionId);

#ifndef NO_PROXY

void	parseProxyPutRequest(char *text, int bytesRemaining,
		CfdpUserOpsData *opsData)
{
	int	length;
	int	pad;

	if (bytesRemaining < 1)
	{
		return;
	}

	/*	Get destination entity ID.				*/

	length = (unsigned char) *text;
	text++;
	bytesRemaining--;
	if (length > bytesRemaining)
	{
		return;
	}

	pad = 8 - length;
	opsData->proxyDestinationEntityNbr.length = length;
	memset(opsData->proxyDestinationEntityNbr.buffer, 0, pad);
	memcpy(opsData->proxyDestinationEntityNbr.buffer + pad, text, length);
	text += length;
	bytesRemaining -= length;

	/*	Get source file name.					*/

	if (bytesRemaining < 1)
	{
		return;
	}

	length = (unsigned char) *text;
	text++;
	bytesRemaining--;
	if (length > bytesRemaining)
	{
		return;
	}

	memcpy(opsData->proxySourceFileName, text, length);
	(opsData->proxySourceFileName)[length] = '\0';
	text += length;
	bytesRemaining -= length;

	/*	Get destination file name.				*/

	if (bytesRemaining < 1)
	{
		return;
	}

	length = (unsigned char) *text;
	text++;
	bytesRemaining--;
	if (length > bytesRemaining)
	{
		return;
	}

	memcpy(opsData->proxyDestFileName, text, length);
	(opsData->proxyDestFileName)[length] = '\0';
}

void	parseProxyMsgToUser(char *text, int bytesRemaining,
		CfdpUserOpsData *opsData)
{
	Sdr		sdr = getIonsdr();
	MsgToUser	msg;
	SdrObject	msgObj;

	/*	Get message.						*/

	if (bytesRemaining < 1)
	{
		return;
	}

	memset((char *) &msg, 0, sizeof(MsgToUser));
	msg.length = (unsigned char) *text;
	text++;
	bytesRemaining--;
	if (msg.length > bytesRemaining)
	{
		return;
	}

	if (msg.length > 0)
	{
		msg.text = sdr_malloc(sdr, msg.length);
		if (msg.text !=  0)
		{
			sdr_write(sdr, msg.text, text, msg.length);
		}
	}

	/*	Write to non-volatile storage.				*/

	if (opsData->proxyMsgsToUser == 0)
	{
		opsData->proxyMsgsToUser = cfdp_create_usrmsg_list();
		if (opsData->proxyMsgsToUser == 0)
		{
			return;
		}
	}

	msgObj = sdr_malloc(sdr, sizeof(MsgToUser));
	if (msgObj)
	{
		sdr_write(sdr, msgObj, (char *) &msg, sizeof(MsgToUser));
		sdr_list_insert_last(sdr, opsData->proxyMsgsToUser, msgObj);
	}
}

void	parseProxyFilestoreRequest(char *text, int bytesRemaining,
		CfdpUserOpsData *opsData)
{
	Sdr			sdr = getIonsdr();
	int			length;
	FilestoreRequest	req;
	SdrObject		reqObj;
	char			nameBuffer[256];

	/*	Get (and ignore) total length of request.		*/

	if (bytesRemaining < 1)
	{
		return;
	}

	length = (unsigned char) *text;
	text++;
	bytesRemaining--;
	if (length > bytesRemaining)
	{
		return;
	}

	/*	Get filestore request action code.			*/

	if (bytesRemaining < 1)
	{
		return;
	}

	memset((char *) &req, 0, sizeof(FilestoreRequest));
	req.action = (((unsigned char) *text) >> 4) & 0x0f;
	text++;
	bytesRemaining--;

	/*	Get first file name.  (LV)				*/

	if (bytesRemaining < 1)
	{
		return;
	}

	length = (unsigned char) *text;
	text++;
	bytesRemaining--;
	if (length > bytesRemaining)
	{
		return;
	}

	if (length > 0)
	{
		memcpy(nameBuffer, text, length);
		nameBuffer[length] = '\0';
		text += length;
		bytesRemaining -= length;
		req.firstFileName = sdr_string_create(sdr, nameBuffer);
	}

	/*	Get second file name.  (LV)				*/

	if (bytesRemaining < 1)
	{
		return;
	}

	length = (unsigned char) *text;
	text++;
	bytesRemaining--;
	if (length > bytesRemaining)
	{
		return;
	}

	if (length > 0)
	{
		memcpy(nameBuffer, text, length);
		nameBuffer[length] = '\0';
		req.secondFileName = sdr_string_create(sdr, nameBuffer);
	}

	/*	Write to non-volatile storage.				*/

	if (opsData->proxyFilestoreRequests == 0)
	{
		opsData->proxyFilestoreRequests = cfdp_create_fsreq_list();
		if (opsData->proxyFilestoreRequests == 0)
		{
			return;
		}
	}

	reqObj = sdr_malloc(sdr, sizeof(FilestoreRequest));
	if (reqObj)
	{
		sdr_write(sdr, reqObj, (char *) &req,
				sizeof(FilestoreRequest));
		sdr_list_insert_last(sdr, opsData->proxyFilestoreRequests,
				reqObj);
	}
}

void	parseProxyFaultHandlerOverride(char *text, int bytesRemaining,
		CfdpUserOpsData *opsData)
{
	unsigned int	override;
	CfdpCondition	condition;
	CfdpHandler	handler;

	if (bytesRemaining < 1)
	{
		return;
	}

	override = (unsigned char) *text;
	condition = (override >> 4) & 0x0f;
	handler = override & 0x0f;
	opsData->proxyFaultHandlers[condition] = handler;
}

void	parseProxyTransmissionMode(char *text, int bytesRemaining,
		CfdpUserOpsData *opsData)
{
	if (bytesRemaining < 1)
	{
		return;
	}

	opsData->proxyUnacknowledged = ((unsigned char) *text) & 0x01;
}

void	parseProxyFlowLabel(char *text, int bytesRemaining,
		CfdpUserOpsData *opsData)
{
	int	length;

	if (bytesRemaining < 1)
	{
		return;
	}

	length = (unsigned char) *text;
	text++;
	bytesRemaining--;
	if (length > bytesRemaining)
	{
		return;
	}

	opsData->proxyFlowLabelLength = length;
	memcpy(opsData->proxyFlowLabel, text, length);
}

void	parseProxySegmentationControl(char *text, int bytesRemaining,
		CfdpUserOpsData *opsData)
{
	if (bytesRemaining < 1)
	{
		return;
	}

	opsData->proxyRecordBoundsRespected = ((unsigned char) *text) & 0x01;
}

void	parseProxyClosureRequest(char *text, int bytesRemaining,
		CfdpUserOpsData *opsData)
{
	if (bytesRemaining < 1)
	{
		return;
	}

	opsData->proxyClosureRequested = ((unsigned char) *text) & 0x01;
}

void	parseProxyPutResponse(char *text, int bytesRemaining,
		CfdpUserOpsData *opsData)
{
	unsigned int	response;

	if (bytesRemaining < 1)
	{
		return;
	}

	response = (unsigned char) *text;
	opsData->proxyCondition = (response >> 4) & 0x0f;
	opsData->proxyDeliveryCode = (response >> 2) & 0x01;
	opsData->proxyFileStatus = response & 0x03;
}

void	parseProxyFilestoreResponse(char *text, int bytesRemaining,
		CfdpUserOpsData *opsData)
{
	Sdr			sdr = getIonsdr();
	int			length;
	FilestoreResponse	resp;
	SdrObject		respObj;
	char			nameBuffer[256];

	/*	Get (and ignore) total length of response.		*/

	if (bytesRemaining == 0)
	{
		return;
	}

	length = (unsigned char) *text;
	text++;
	bytesRemaining--;
	if (length > bytesRemaining)
	{
		return;
	}

	/*	Get filestore response action code.			*/

	if (bytesRemaining == 0)
	{
		return;
	}

	memset((char *) &resp, 0, sizeof(FilestoreResponse));
	resp.action = (((unsigned char) *text) >> 4) & 0x0f;
	text++;
	bytesRemaining--;

	/*	Get first file name.  (LV)				*/

	if (bytesRemaining == 0)
	{
		return;
	}

	length = (unsigned char) *text;
	text++;
	bytesRemaining--;
	if (length > bytesRemaining)
	{
		return;
	}

	if (length > 0)
	{
		memcpy(nameBuffer, text, length);
		nameBuffer[length] = '\0';
		text += length;
		bytesRemaining -= length;
		resp.firstFileName = sdr_string_create(sdr, nameBuffer);
	}

	/*	Get second file name.  (LV)				*/

	if (bytesRemaining == 0)
	{
		return;
	}

	length = (unsigned char) *text;
	text++;
	bytesRemaining--;
	if (length > bytesRemaining)
	{
		return;
	}

	if (length > 0)
	{
		memcpy(nameBuffer, text, length);
		nameBuffer[length] = '\0';
		text += length;
		bytesRemaining -= length;
		resp.secondFileName = sdr_string_create(sdr, nameBuffer);
	}

	/*	Get message text.  (LV)					*/

	if (bytesRemaining == 0)
	{
		return;
	}

	length = (unsigned char) *text;
	text++;
	bytesRemaining--;
	if (length > bytesRemaining)
	{
		return;
	}

	if (length > 0)
	{
		memcpy(nameBuffer, text, length);
		nameBuffer[length] = '\0';
		resp.message = sdr_string_create(sdr, nameBuffer);
	}

	/*	Write to non-volatile storage.				*/

	if (opsData->proxyFilestoreResponses == 0)
	{
		opsData->proxyFilestoreResponses = cfdp_create_fsreq_list();
		if (opsData->proxyFilestoreResponses == 0)
		{
			return;
		}
	}

	respObj = sdr_malloc(sdr, sizeof(FilestoreResponse));
	if (respObj)
	{
		sdr_write(sdr, respObj, (char *) &resp,
				sizeof(FilestoreResponse));
		sdr_list_insert_last(sdr, opsData->proxyFilestoreResponses,
				respObj);
	}
}

#endif

void	parseOriginatingTransactionId(char *text, int bytesRemaining,
		CfdpUserOpsData *opsData)
{
	unsigned int	lengths;
	int		sourceEntityNbrLength;
	int		sourceEntityNbrPad;
	int		transactionNbrLength;
	int		transactionNbrPad;

	if (bytesRemaining < 1)
	{
		return;
	}

	lengths = (unsigned char) *text;
	sourceEntityNbrLength = 1 + ((lengths >> 4) & 0x07);
	sourceEntityNbrPad = 8 - sourceEntityNbrLength;
	transactionNbrLength = 1 + (lengths & 0x07);
	transactionNbrPad = 8 - transactionNbrLength;
	text++;
	bytesRemaining--;

	/*	Get destination entity ID.				*/

	if ((sourceEntityNbrLength + transactionNbrLength) > bytesRemaining)
	{
		return;
	}

	opsData->originatingTransactionId.sourceEntityNbr.length
			= sourceEntityNbrLength;
	memset(opsData->originatingTransactionId.sourceEntityNbr.buffer, 0,
			sourceEntityNbrPad);
	memcpy(opsData->originatingTransactionId.sourceEntityNbr.buffer
			+ sourceEntityNbrPad, text, sourceEntityNbrLength);
	text += sourceEntityNbrLength;
	bytesRemaining -= sourceEntityNbrLength;
	opsData->originatingTransactionId.transactionNbr.length
			= transactionNbrLength;
	memset(opsData->originatingTransactionId.transactionNbr.buffer, 0,
			transactionNbrPad);
	memcpy(opsData->originatingTransactionId.transactionNbr.buffer
			+ transactionNbrPad, text, transactionNbrLength);
}

#ifndef NO_PROXY

static int	reportOnProxyPut(CfdpUserOpsData *opsData,
			CfdpCondition condition, CfdpDeliveryCode deliveryCode,
			CfdpFileStatus fileStatus)
{
	Sdr			sdr = getIonsdr();
	SdrObject		msgs = cfdp_create_usrmsg_list();
	MsgToUser		msg;
	unsigned char		textBuffer[6];
	SdrObject		msgObj;
	CfdpTransactionId	transactionId;

	msg.length = 6;
	if (msgs == 0 || (msg.text = sdr_malloc(sdr, msg.length)) == 0
	|| (msgObj = sdr_malloc(sdr, sizeof(MsgToUser))) == 0
	|| sdr_list_insert_last(sdr, msgs, msgObj) == 0)
	{
		putErrmsg("Can't report on proxy put.", NULL);
		return -1;
	}

	memcpy(textBuffer, "cfdp", 4);
	textBuffer[4] = CfdpProxyPutResponse;
	textBuffer[5] = (((int) condition) << 4) + (((int) deliveryCode) << 2)
			+ (int) fileStatus;
	sdr_write(sdr, msg.text, (char *) textBuffer, msg.length);
	sdr_write(sdr, msgObj, (char *) &msg, sizeof(MsgToUser));
	return createFDU(&opsData->originatingTransactionId.sourceEntityNbr,
			0, NULL, NULL, NULL, NULL, NULL, NULL, 0, NULL, 0, msgs,
			0, &opsData->originatingTransactionId, &transactionId);
}

int	handleProxyPutRequest(CfdpUserOpsData *opsData)
{
	static CfdpReaderFn	stdReaderFn = CFDP_STD_READER;
	Entity			dest;
	uvast			destinationEntityId;
	unsigned int		closureLatency;
	CfdpTransactionId	transactionId;
	int			result;

	if (!opsData->proxyUnacknowledged)
	{
		return reportOnProxyPut(opsData, CfdpInvalidTransmissionMode,
				CfdpDataIncomplete, CfdpFileStatusUnreported);
	}

	if (opsData->proxyClosureRequested)
	{
		cfdp_decompress_number(&destinationEntityId,
				&(opsData->proxyDestinationEntityNbr));
		if (findEntity(destinationEntityId, &dest) == 0)
		{
			closureLatency = 0;
		}
		else
		{
			closureLatency = dest.ackTimerInterval;
		}
	}
	else
	{
		closureLatency = 0;
	}

	result = createFDU(&opsData->proxyDestinationEntityNbr, 0, NULL,
			opsData->proxySourceFileName,
			opsData->proxyDestFileName,
			(opsData->proxyRecordBoundsRespected ?
			 		stdReaderFn : NULL),
			NULL,
			opsData->proxyFaultHandlers,
			opsData->proxyFlowLabelLength,
			(opsData->proxyFlowLabelLength>0 ?
					opsData->proxyFlowLabel : NULL),
			closureLatency,
			opsData->proxyMsgsToUser,
			opsData->proxyFilestoreRequests,
			&opsData->originatingTransactionId,
			&transactionId);
	if (result < 0)
	{
		putErrmsg("Can't perform proxy put.", NULL);
		return -1;
	}

	if (transactionId.transactionNbr.length == 0)
	{
		/*	createFDU failed silently (e.g., source file not
		 *	found). Send error response back to originator.	*/
		return reportOnProxyPut(opsData, CfdpFilestoreRejection,
				CfdpDataIncomplete, CfdpFileStatusUnreported);
	}

	return reportOnProxyPut(opsData, CfdpNoError, CfdpDataComplete,
			CfdpFileStatusUnreported);
}

int	handleProxyPutCancel(CfdpUserOpsData *opsData)
{
	Sdr	sdr = getIonsdr();
	CfdpDB	*db = getCfdpConstants();
	SdrObject elt;
		OBJ_POINTER(OutFdu, fdu);

	for (elt = sdr_list_first(sdr, db->outboundFdus); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, OutFdu, fdu, sdr_list_data(sdr, elt));
		if (memcmp((char *) &fdu->originatingTransactionId,
				(char *) &opsData->originatingTransactionId,
				sizeof(CfdpTransactionId)) == 0)
		{
			if (cfdp_cancel(&fdu->transactionId) < 0)
			{
				putErrmsg("CFDP failed on remote put cancel.",
						NULL);
				return -1;
			}

			return reportOnProxyPut(opsData, CfdpCancelRequested,
				CfdpDataIncomplete, CfdpFileStatusUnreported);
		}
	}

	return reportOnProxyPut(opsData, CfdpCancelRequested, CfdpDataComplete,
			CfdpFileStatusUnreported);
}

int	cfdp_rput(CfdpNumber *respondentEntityNbr, unsigned int utParmsLength,
		unsigned char *utParms, char *sourceFileName,
		char *destFileName, CfdpReaderFn readerFn,
		CfdpHandler *faultHandlers, unsigned int flowLabelLength,
		unsigned char *flowLabel, unsigned int closureLatency,
		SdrObject messagesToUser, SdrObject filestoreRequests,
		CfdpNumber *beneficiaryEntityNbr, CfdpProxyTask *task,
		CfdpTransactionId *transactionId)
{
	Sdr		sdr = getIonsdr();
	int		sourceFileNameLen;
	int		destFileNameLen;
	SdrObject	msgs;
	unsigned char	textBuffer[600];
	SdrObject	elt;
	SdrObject	nextElt;
	SdrObject	obj;
	int		length;
	int		pad;
			OBJ_POINTER(MsgToUser, proxyMsg);
			OBJ_POINTER(FilestoreRequest, req);
	int		i;
	CfdpHandler	*override;

	CHKERR(respondentEntityNbr);
	CHKERR(beneficiaryEntityNbr);
	CHKERR(transactionId);
	CHKERR(task);
	if (task->sourceFileName == NULL)
	{
		CHKERR(task->destFileName == NULL);
		sourceFileNameLen = 0;
		destFileNameLen = 0;
	}
	else
	{
		sourceFileNameLen = strlen(task->sourceFileName);
		CHKERR(sourceFileNameLen < 256);
		if (task->destFileName == NULL)
		{
			destFileNameLen = 0;
		}
		else
		{
			destFileNameLen = strlen(task->destFileName);
			CHKERR(destFileNameLen < 256);
		}
	}

	if (task->flowLabel)
	{
		CHKERR(task->flowLabelLength > 0
				&& task->flowLabelLength < 256);
	}
	else
	{
		CHKERR(task->flowLabelLength == 0);
	}

	CHKERR(sdr_begin_xn(sdr));

	/*	Append proxy messages to messagesToUser if provided,
	 *	else create new sdrlist for messages to user.		*/

	if (messagesToUser == 0)
	{
		if ((msgs = cfdp_create_usrmsg_list()) == 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("Can't create user messages list.", NULL);
			return -1;
		}
	}
	else
	{
		msgs = messagesToUser;
	}

	memcpy(textBuffer, "cfdp", 4);

	/*	Add proxy put request message to msgs list.		*/

	textBuffer[4] = CfdpProxyPutRequest;
	length = 5;
	textBuffer[length] = beneficiaryEntityNbr->length;
	length++;
	pad = 8 - beneficiaryEntityNbr->length;
	memcpy(textBuffer + length, beneficiaryEntityNbr->buffer + pad,
			beneficiaryEntityNbr->length);
	length += beneficiaryEntityNbr->length;
	textBuffer[length] = sourceFileNameLen;
	length++;
	memcpy(textBuffer + length, task->sourceFileName,
			sourceFileNameLen);
	length += sourceFileNameLen;
	textBuffer[length] = destFileNameLen;
	length++;
	memcpy(textBuffer + length, task->destFileName, destFileNameLen);
	length += destFileNameLen;
	if (length > 255)
	{
		sdr_list_destroy(sdr, msgs, NULL, NULL);
		sdr_end_xn(sdr);
		putErrmsg("Message to User Too Long.", itoa(length));
		return -1;
	}
	if (cfdp_add_usrmsg(msgs, textBuffer, length) < 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't insert proxy put request msg.", NULL);
		return -1;
	}

	/*	Add all proxy user messages to msgs list.		*/

	if (task->messagesToUser)
	{
		textBuffer[4] = CfdpProxyMsgToUser;
		for (elt = sdr_list_first(sdr, task->messagesToUser); elt;
				elt = nextElt)
		{
			length = 5;
			nextElt = sdr_list_next(sdr, elt);
			obj = sdr_list_data(sdr, elt);
			sdr_list_delete(sdr, elt, NULL, NULL);
			GET_OBJ_POINTER(sdr, MsgToUser, proxyMsg, obj);
			if (proxyMsg->text)
			{
				textBuffer[length] = proxyMsg->length;
				length++;
				sdr_read(sdr, (char *) textBuffer + length,
					proxyMsg->text, proxyMsg->length);
				sdr_free(sdr, proxyMsg->text);
				length += proxyMsg->length;
				if (length > 255)
				{
					sdr_list_destroy(sdr, msgs, NULL, NULL);
					sdr_list_destroy(sdr,
							task->messagesToUser,
							NULL, NULL);
					sdr_end_xn(sdr);
					putErrmsg("Message to user too long.",
							itoa(length));
					return -1;
				}

				if (cfdp_add_usrmsg(msgs, textBuffer, length)
						< 0)
				{
					sdr_cancel_xn(sdr);
					putErrmsg("Can't copy proxy user msg.",
							NULL);
					return -1;
				}
			}

			sdr_free(sdr, obj);
		}

		sdr_list_destroy(sdr, task->messagesToUser, NULL, NULL);
	}

	/*	Add all proxy filestore requests to msgs list.		*/

	if (task->filestoreRequests)
	{
		textBuffer[4] = CfdpProxyFilestoreRequest;
		for (elt = sdr_list_first(sdr, task->filestoreRequests); elt;
				elt = nextElt)
		{
			length = 5;
			nextElt = sdr_list_next(sdr, elt);
			obj = sdr_list_data(sdr, elt);
			sdr_list_delete(sdr, elt, NULL, NULL);
			GET_OBJ_POINTER(sdr, FilestoreRequest, req, obj);
			textBuffer[length]
					= (((int) req->action) << 4) & 0xff;
			length++;
			if (req->firstFileName == 0)
			{
				textBuffer[length] = 0;
				length++;
			}
			else
			{
				length = sdr_string_length(sdr,
						req->firstFileName);
				textBuffer[length] = length;
				length++;
				sdr_string_read(sdr, (char *) textBuffer
						+ length, req->firstFileName);
				sdr_free(sdr, req->firstFileName);
				length += length;
			}

			if (req->secondFileName == 0)
			{
				textBuffer[length] = 0;
				length++;
			}
			else
			{
				length = sdr_string_length(sdr,
						req->secondFileName);
				textBuffer[length] = length;
				length++;
				sdr_string_read(sdr, (char *) textBuffer
						+ length, req->secondFileName);
				sdr_free(sdr, req->secondFileName);
				length += length;
			}

			if (length > 255)
			{
				sdr_list_destroy(sdr, msgs, NULL, NULL);
				sdr_list_destroy(sdr, task->filestoreRequests,
						NULL, NULL);
				sdr_end_xn(sdr);
				putErrmsg("Message to User Too Long.",
						itoa(length));
				return -1;
			}

			if (cfdp_add_usrmsg(msgs, textBuffer, length) < 0)
			{
				sdr_cancel_xn(sdr);
				putErrmsg("Can't copy proxy filestore request.",
						NULL);
				return -1;
			}

			sdr_free(sdr, obj);
		}

		sdr_list_destroy(sdr, task->filestoreRequests, NULL, NULL);
	}

	/*	Add all proxy fault handler overrides to msgs list.	*/

	if (task->faultHandlers)
	{
		textBuffer[4] = CfdpProxyFaultHandlerOverride;
		for (i = 0, override = task->faultHandlers; i < 16;
				i++, override++)
		{
			length = 5;
			if (override != CfdpNoHandler)
			{
				textBuffer[length] = (i << 4)
						+ (*override & 0x0f);
				length++;
				if (cfdp_add_usrmsg(msgs, textBuffer, length)
						< 0)
				{
					sdr_cancel_xn(sdr);
					putErrmsg("Can't copy proxy fault \
handler override.", NULL);
					return -1;
				}
			}
		}
	}

	/*	Add proxy transmission mode message to msgs list.	*/

	textBuffer[4] = CfdpProxyTransmissionMode;
	length = 5;
	textBuffer[length] = task->unacknowledged & 0x01;
	length++;
	if (cfdp_add_usrmsg(msgs, textBuffer, length) < 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't insert proxy transmission mode message.",
				NULL);
		return -1;
	}

	/*	Add proxy flow label message to msgs list.		*/

	if (task->flowLabelLength > 0)
	{
		textBuffer[4] = CfdpProxyFlowLabel;
		length = 5;
		textBuffer[length] = task->flowLabelLength;
		length++;
		memcpy(textBuffer + length, task->flowLabel,
				task->flowLabelLength);
		length += task->flowLabelLength;
		if (length > 255)
		{
			sdr_list_destroy(sdr, msgs, NULL, NULL);
			sdr_end_xn(sdr);
			putErrmsg("Message to User Too Long.", itoa(length));
			return -1;
		}
		if (cfdp_add_usrmsg(msgs, textBuffer, length) < 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("Can't copy proxy flow label.", NULL);
			return -1;
		}
	}

	/*	Add proxy segmentation control message to msgs list.	*/

	textBuffer[4] = CfdpProxySegmentationControl;
	length = 5;
	textBuffer[length] = task->recordBoundsRespected & 0x01;
	length++;
	if (cfdp_add_usrmsg(msgs, textBuffer, length) < 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't insert proxy segmentation control message.",
				NULL);
		return -1;
	}

	/*	Add proxy closure request message to msgs list.		*/

	if (task->closureRequested)
	{
		textBuffer[4] = CfdpProxyClosureRequest;
		length = 5;
		textBuffer[length] = 0x01;
		length++;
		if (cfdp_add_usrmsg(msgs, textBuffer, length) < 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("Can't insert proxy closure request message.",
			       	NULL);
			return -1;
		}
	}

	/*	Send the proxy put request FDU.				*/

	if (createFDU(respondentEntityNbr, utParmsLength, utParms,
			sourceFileName, destFileName, readerFn, NULL,
			faultHandlers, flowLabelLength, flowLabel,
			closureLatency, msgs, filestoreRequests, NULL,
			transactionId) < 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't send proxy put request.", NULL);
		return -1;
	}

	if (sdr_end_xn(sdr))
	{
		putErrmsg("CFDP failed in proxy put request.", NULL);
		return -1;
	}

	return 0;
}

int	cfdp_rput_cancel(CfdpNumber *respondentEntityNbr,
		unsigned int utParmsLength, unsigned char *utParms,
		char *sourceFileName, char *destFileName, CfdpReaderFn readerFn,
		CfdpHandler *faultHandlers, unsigned int flowLabelLength,
		unsigned char *flowLabel, unsigned int closureLatency,
		SdrObject messagesToUser, SdrObject filestoreRequests,
		CfdpTransactionId *rputTransactionId,
		CfdpTransactionId *transactionId)
{
	Sdr		sdr = getIonsdr();
	int		length;
	unsigned char	textBuffer[5];

	CHKERR(respondentEntityNbr);
	CHKERR(rputTransactionId);
	CHKERR(transactionId);
	CHKERR(sdr_begin_xn(sdr));

	/*	Append cancel messages to messagesToUser if provided,
	 *	else create new sdrlist for messages to user.		*/

	if (messagesToUser == 0)
	{
		if ((messagesToUser = cfdp_create_usrmsg_list()) == 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("Can't create user messages list.", NULL);
			return -1;
		}
	}

	memcpy(textBuffer, "cfdp", 4);

	/*	Append proxy put cancel message to msgs list.		*/

	textBuffer[4] = CfdpProxyPutCancel;
	length = 5;
	if (cfdp_add_usrmsg(messagesToUser, textBuffer, length) < 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't insert proxy put request msg.", NULL);
		return -1;
	}

	/*	Send the proxy put cancel FDU.				*/

	if (createFDU(respondentEntityNbr, utParmsLength, utParms,
			sourceFileName, destFileName, readerFn, NULL,
			faultHandlers, flowLabelLength, flowLabel,
			closureLatency, messagesToUser, filestoreRequests,
			rputTransactionId, transactionId) < 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't send proxy put cancel.", NULL);
		return -1;
	}

	if (sdr_end_xn(sdr))
	{
		putErrmsg("CFDP failed in proxy put cancel.", NULL);
		return -1;
	}

	return 0;
}

int	cfdp_get(CfdpNumber *respondentEntityNbr, unsigned int utParmsLength,
		unsigned char *utParms, char *sourceFileName,
		char *destFileName, CfdpReaderFn readerFn,
		CfdpHandler *faultHandlers, unsigned int flowLabelLength,
		unsigned char *flowLabel, unsigned int closureLatency,
		SdrObject messagesToUser, SdrObject filestoreRequests,
		CfdpProxyTask *task, CfdpTransactionId *transactionId)
{
	CfdpDB	*db = getCfdpConstants();

	return cfdp_rput(respondentEntityNbr, utParmsLength, utParms,
			sourceFileName, destFileName, readerFn, faultHandlers,
			flowLabelLength, flowLabel, closureLatency,
			messagesToUser, filestoreRequests,
			&db->ownEntityNbr, task, transactionId);
}

#endif

#ifndef NO_DIRLIST

/*	Backward-compatible wire format for directory listing:
 *
 *	Request (CfdpDirectoryListingRequest = 16):
 *	  [dirNameLen][dirName...][destFileNameLen][destFileName...]
 *	  [options byte] -- optional, v2 only.  Old parsers stop
 *	  after destFileName and never see it; new parsers detect
 *	  it as a trailing byte and use it to enable v2 behavior.
 *
 *	Response (CfdpDirectoryListingResponse = 17, legacy):
 *	  [responseCode][dirNameLen][dirName][destFileNameLen][destFileName]
 *
 *	Response (CfdpDirectoryListingResponseV2 = 19, v2):
 *	  [responseCode][incomplete byte][dirNameLen][dirName]
 *	  [destFileNameLen][destFileName]
 *
 *	Manifest file content (legacy):
 *	  Stream of NUL-terminated entry names.
 *
 *	Manifest file content (v2):
 *	  Stream of records [status byte][type byte][name][NUL].
 *	  status: bitfield, currently only CFDP_DIRENT_NAME_TRUNCATED.
 *	  type:   one of CFDP_DIRENT_REGULAR/DIRECTORY/SYMLINK/OTHER/UNKNOWN.
 */

void	parseDirectoryListingRequest(char *text, int bytesRemaining,
		CfdpUserOpsData *opsData)
{
	int	length;

	opsData->directoryListingOptions = 0;

	/*	Get directory name.					*/

	if (bytesRemaining < 1)
	{
		return;
	}

	length = (unsigned char) *text;
	text++;
	bytesRemaining--;
	if (length > bytesRemaining)
	{
		return;
	}

	memcpy(opsData->directoryName, text, length);
	(opsData->directoryName)[length] = '\0';
	text += length;
	bytesRemaining -= length;

	/*	Get destination file name.				*/

	if (bytesRemaining < 1)
	{
		return;
	}

	length = (unsigned char) *text;
	text++;
	bytesRemaining--;
	if (length > bytesRemaining)
	{
		return;
	}

	memcpy(opsData->directoryDestFileName, text, length);
	(opsData->directoryDestFileName)[length] = '\0';
	text += length;
	bytesRemaining -= length;

	/*	v2 request: optional trailing options byte.  Old
	 *	requestors do not send this; old responders never
	 *	read it (they stop after destFileName).			*/

	if (bytesRemaining >= 1)
	{
		opsData->directoryListingOptions = (unsigned char) *text;
	}
}

void	parseDirectoryListingResponse(char *text, int bytesRemaining,
		CfdpUserOpsData *opsData)
{
	int	length;

	opsData->directoryListingIncomplete = 0;

	if (bytesRemaining < 1)
	{
		return;
	}

	opsData->directoryListingResponseCode = *text;
	text++;
	bytesRemaining--;

	/*	Get directory name.					*/

	if (bytesRemaining < 1)
	{
		return;
	}

	length = (unsigned char) *text;
	text++;
	bytesRemaining--;
	if (length > bytesRemaining)
	{
		return;
	}

	memcpy(opsData->directoryName, text, length);
	(opsData->directoryName)[length] = '\0';
	text += length;
	bytesRemaining -= length;

	/*	Get destination file name.				*/

	if (bytesRemaining < 1)
	{
		return;
	}

	length = (unsigned char) *text;
	text++;
	bytesRemaining--;
	if (length > bytesRemaining)
	{
		return;
	}

	memcpy(opsData->directoryDestFileName, text, length);
	(opsData->directoryDestFileName)[length] = '\0';
}

void	parseDirectoryListingResponseV2(char *text, int bytesRemaining,
		CfdpUserOpsData *opsData)
{
	int	length;

	opsData->directoryListingIncomplete = 0;

	if (bytesRemaining < 1)
	{
		return;
	}

	opsData->directoryListingResponseCode = *text;
	text++;
	bytesRemaining--;

	/*	v2-specific: incomplete byte.				*/

	if (bytesRemaining < 1)
	{
		return;
	}

	opsData->directoryListingIncomplete = (unsigned char) *text;
	text++;
	bytesRemaining--;

	/*	Get directory name.					*/

	if (bytesRemaining < 1)
	{
		return;
	}

	length = (unsigned char) *text;
	text++;
	bytesRemaining--;
	if (length > bytesRemaining)
	{
		return;
	}

	memcpy(opsData->directoryName, text, length);
	(opsData->directoryName)[length] = '\0';
	text += length;
	bytesRemaining -= length;

	/*	Get destination file name.				*/

	if (bytesRemaining < 1)
	{
		return;
	}

	length = (unsigned char) *text;
	text++;
	bytesRemaining--;
	if (length > bytesRemaining)
	{
		return;
	}

	memcpy(opsData->directoryDestFileName, text, length);
	(opsData->directoryDestFileName)[length] = '\0';
}

static int	sendDirectoryListingResponse(CfdpUserOpsData *opsData,
			int responseCode, char *listingFileName,
			int incomplete)
{
	Sdr			sdr = getIonsdr();
	SdrObject		msgs = cfdp_create_usrmsg_list();
	int			dirNameLen = strlen(opsData->directoryName);
	int			destFileNameLen =
					strlen(opsData->directoryDestFileName);
	int			useV2 = ((opsData->directoryListingOptions
					& CFDP_DIRLIST_OPTION_STATUS_BYTES) != 0);
	int			headerOverhead = useV2 ? 8 : 7;
				/*	"cfdp" (4) + msgType (1) + responseCode
				 *	(1) + [incomplete (1) if v2] +
				 *	dirNameLen byte (1).		*/
	MsgToUser		msg;
	unsigned char		textBuffer[600];
	int			pos;
	SdrObject		msgObj;
	CfdpTransactionId	transactionId;

	if (headerOverhead + 1 + dirNameLen + destFileNameLen > 255)
	{
		putErrmsg("CFDP: User Message too long.",  NULL);
		return -1;
	}

	msg.length = headerOverhead + 1 + dirNameLen + destFileNameLen;
	if (msgs == 0 || (msg.text = sdr_malloc(sdr, msg.length)) == 0
	|| (msgObj = sdr_malloc(sdr, sizeof(MsgToUser))) == 0
	|| sdr_list_insert_last(sdr, msgs, msgObj) == 0)
	{
		putErrmsg("Can't respond to directory listing request.", NULL);
		return -1;
	}

	memcpy(textBuffer, "cfdp", 4);
	if (useV2)
	{
		textBuffer[4] = CfdpDirectoryListingResponseV2;
		textBuffer[5] = (unsigned char) responseCode;
		textBuffer[6] = (unsigned char) (incomplete ? 1 : 0);
		pos = 7;
	}
	else
	{
		textBuffer[4] = CfdpDirectoryListingResponse;
		textBuffer[5] = (unsigned char) responseCode;
		pos = 6;
	}

	textBuffer[pos] = dirNameLen;
	pos++;
	memcpy(textBuffer + pos, opsData->directoryName, dirNameLen);
	pos += dirNameLen;
	textBuffer[pos] = destFileNameLen;
	pos++;
	memcpy(textBuffer + pos, opsData->directoryDestFileName,
			destFileNameLen);
	pos += destFileNameLen;
	sdr_write(sdr, msg.text, (char *) textBuffer, msg.length);
	sdr_write(sdr, msgObj, (char *) &msg, sizeof(MsgToUser));
	return createFDU(&opsData->originatingTransactionId.sourceEntityNbr, 0,
			NULL, listingFileName, (listingFileName != NULL ?
			opsData->directoryDestFileName : NULL),
			NULL, NULL, NULL, 0, NULL, 0, msgs, 0,
			&opsData->originatingTransactionId, &transactionId);
}

#if defined(mingw)
#include <windows.h>
#endif

static unsigned char	classifyDirEntry(const char *fullPath)
{
#if defined(mingw)
	DWORD	attributes;

	attributes = GetFileAttributesA(fullPath);
	if (attributes == INVALID_FILE_ATTRIBUTES)
	{
		return CFDP_DIRENT_UNKNOWN;
	}

	if (attributes & FILE_ATTRIBUTE_REPARSE_POINT)
	{
		/*	Treat reparse points (Windows symlink moral
		 *	equivalent) like symlinks.			*/
		return CFDP_DIRENT_SYMLINK;
	}

	if (attributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		return CFDP_DIRENT_DIRECTORY;
	}

	return CFDP_DIRENT_REGULAR;
#else
	struct stat	st;

	if (lstat(fullPath, &st) < 0)
	{
		return CFDP_DIRENT_UNKNOWN;
	}

	if (S_ISLNK(st.st_mode))	return CFDP_DIRENT_SYMLINK;
	if (S_ISDIR(st.st_mode))	return CFDP_DIRENT_DIRECTORY;
	if (S_ISREG(st.st_mode))	return CFDP_DIRENT_REGULAR;
	return CFDP_DIRENT_OTHER;
#endif
}

static int	writeListingRecordV2(int listing, unsigned char status,
			unsigned char type, const char *name, int nameLen)
{
	unsigned char	header[2];

	header[0] = status;
	header[1] = type;
	if (write(listing, header, 2) != 2)
	{
		return -1;
	}

	/*	Write name plus its terminating NUL.			*/

	if (write(listing, name, nameLen + 1) != (nameLen + 1))
	{
		return -1;
	}

	return 0;
}

int	handleDirectoryListingRequest(CfdpUserOpsData *opsData)
{
	DIR		*dir;
	char		listingFileName[256];
	int		listing;
	struct dirent	*entry;
	int		nameLen;
	char		fullPath[CFDP_DIRLIST_MAX_NAME + 256 + 2];
	int		incomplete = 0;
	int		useV2 = ((opsData->directoryListingOptions
				& CFDP_DIRLIST_OPTION_STATUS_BYTES) != 0);
	unsigned char	type;

	if (strlen(opsData->directoryName) == 0
	|| strlen(opsData->directoryDestFileName) == 0)
	{
		return sendDirectoryListingResponse(opsData, -1, NULL, 0);
	}

	dir = opendir(opsData->directoryName);
	if (dir == NULL)
	{
		putSysErrmsg("Can't list requested directory",
				opsData->directoryName);
		return sendDirectoryListingResponse(opsData, -1, NULL, 0);
	}

	isprintf(listingFileName, sizeof listingFileName, "dirlist_%lu",
			(unsigned long) time(NULL));
	listing = ifopen(listingFileName, O_WRONLY | O_CREAT | O_TRUNC, 0777);
	if (listing < 0)
	{
		putSysErrmsg("Can't create directory listing file",
				listingFileName);
		closedir(dir);
		return -1;
	}

	while ((entry = readdir(dir)) != NULL)
	{
		/*	Skip "." and ".." in both formats.		*/

		if (strcmp(entry->d_name, ".") == 0
		|| strcmp(entry->d_name, "..") == 0)
		{
			continue;
		}

		nameLen = strlen(entry->d_name);

		if (useV2)
		{
			/*	Drop entries we cannot fully convey.
			 *	Truncating a name produces something
			 *	that looks like a real fetchable
			 *	filename but is not -- silent
			 *	corruption.  Drop and signal via
			 *	the listing-level incomplete flag
			 *	instead.				*/

			if (nameLen > CFDP_DIRLIST_MAX_NAME)
			{
				putErrmsg("Skipping over-length entry name",
						entry->d_name);
				incomplete = 1;
				continue;
			}

			/*	Build full path for stat.  Length is
			 *	already bounded: directoryName <= 255
			 *	(per cfdp_rls_extended check) and
			 *	nameLen <= CFDP_DIRLIST_MAX_NAME (511).
			 *	fullPath is sized to hold both plus
			 *	separator and NUL.			*/

			isprintf(fullPath, sizeof fullPath, "%s/%s",
					opsData->directoryName,
					entry->d_name);

			type = classifyDirEntry(fullPath);

			if (writeListingRecordV2(listing, 0, type,
					entry->d_name, nameLen) < 0)
			{
				putSysErrmsg("Listing write failed",
						listingFileName);
				incomplete = 1;
				continue;
			}
		}
		else
		{
			/*	Legacy format: stream of NUL-terminated
			 *	names.  Drop over-length names rather
			 *	than silently truncating them; truncation
			 *	produces a name that looks fetchable but
			 *	does not match any real file.		*/

			if (nameLen > 255)
			{
				putErrmsg("Skipping over-length entry name "
						"(legacy format cannot signal)",
						entry->d_name);
				incomplete = 1;
				continue;
			}

			if (write(listing, entry->d_name, nameLen + 1)
					!= (nameLen + 1))
			{
				putSysErrmsg("Can't write directory listing file",
						listingFileName);
				incomplete = 1;
				continue;
			}
		}
	}

	close(listing);
	closedir(dir);
	return sendDirectoryListingResponse(opsData, 0, listingFileName,
			incomplete);
}

int	cfdp_rls(CfdpNumber *respondentEntityNbr, unsigned int utParmsLength,
		unsigned char *utParms, char *sourceFileName,
		char *destFileName, CfdpReaderFn readerFn,
		CfdpHandler *faultHandlers, unsigned int flowLabelLength,
		unsigned char *flowLabel, unsigned int closureLatency,
		SdrObject messagesToUser, SdrObject filestoreRequests,
		CfdpDirListTask *task, CfdpTransactionId *transactionId)
{
	return cfdp_rls_extended(respondentEntityNbr, utParmsLength, utParms,
			sourceFileName, destFileName, readerFn, faultHandlers,
			flowLabelLength, flowLabel, closureLatency,
			messagesToUser, filestoreRequests, task, 0,
			transactionId);
}

int	cfdp_rls_extended(CfdpNumber *respondentEntityNbr,
		unsigned int utParmsLength, unsigned char *utParms,
		char *sourceFileName, char *destFileName,
		CfdpReaderFn readerFn, CfdpHandler *faultHandlers,
		unsigned int flowLabelLength, unsigned char *flowLabel,
		unsigned int closureLatency, SdrObject messagesToUser,
		SdrObject filestoreRequests, CfdpDirListTask *task,
		unsigned int listingOptions,
		CfdpTransactionId *transactionId)
{
	Sdr		sdr = getIonsdr();
	int		directoryNameLen;
	int		destFileNameLen;
	int		length;
	unsigned char	textBuffer[600];

	CHKERR(respondentEntityNbr);
	CHKERR(task && task->directoryName && task->destFileName);
	directoryNameLen = strlen(task->directoryName);
	CHKERR(directoryNameLen > 0 && directoryNameLen < 256);
	destFileNameLen = strlen(task->destFileName);
	CHKERR(destFileNameLen > 0 && destFileNameLen < 256);
	CHKERR((listingOptions & ~CFDP_DIRLIST_OPTION_STATUS_BYTES) == 0);
	CHKERR(transactionId);
	CHKERR(sdr_begin_xn(sdr));

	/*	Append proxy messages to messagesToUser if provided,
	 *	else create new sdrlist for messages to user.		*/

	if (messagesToUser == 0)
	{
		if ((messagesToUser = cfdp_create_usrmsg_list()) == 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("Can't create user messages list.", NULL);
			return -1;
		}
	}

	memcpy(textBuffer, "cfdp", 4);

	/*	Add directory listing request message to msgs list.	*/

	textBuffer[4] = CfdpDirectoryListingRequest;
	length = 5;
	textBuffer[length] = directoryNameLen;
	length++;
	memcpy(textBuffer + length, task->directoryName, directoryNameLen);
	length += directoryNameLen;
	textBuffer[length] = destFileNameLen;
	length++;
	memcpy(textBuffer + length, task->destFileName, destFileNameLen);
	length += destFileNameLen;

	/*	v2: append optional options byte.  Old responders
	 *	stop parsing after destFileName and never read it.	*/

	if (listingOptions != 0)
	{
		textBuffer[length] = (unsigned char) listingOptions;
		length++;
	}

	if (length > 255)
	{
		sdr_list_destroy(sdr, messagesToUser, NULL, NULL);
		sdr_end_xn(sdr);
		putErrmsg("Message to User Too Long.", itoa(length));
		return -1;
	}
	if (cfdp_add_usrmsg(messagesToUser, textBuffer, length) < 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't insert proxy put request msg.", NULL);
		return -1;
	}

	/*	Send the directory listing request FDU.			*/

	if (createFDU(respondentEntityNbr, utParmsLength, utParms,
			sourceFileName, destFileName, readerFn, NULL,
			faultHandlers, flowLabelLength, flowLabel,
			closureLatency, messagesToUser, filestoreRequests,
			NULL, transactionId) < 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't send directory listing request.", NULL);
		return -1;
	}

	if (sdr_end_xn(sdr))
	{
		putErrmsg("CFDP failed in directory listing request.", NULL);
		return -1;
	}

	return 0;
}

#endif
