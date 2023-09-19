/* Dump CFDP Events to STDOUT
 * Author: Samuel Jero <sj323707@ohio.edu>
 * Date: December 2011
 */
#include <stdio.h>
#include <stdlib.h>
#include "cfdp.h"
#include "ion.h"


void poll_cfdp_messages();
void dbgprintf(int level, const char *fmt, ...);



/*Start Here*/
int main(int argc, char **argv)
{

	/*Initialize CFDP*/
	if (cfdp_attach() < 0) {
		fprintf(stderr, "Error: Can't initialize CFDP. Is ION running?\n");
		exit(1);
	}

	poll_cfdp_messages();

	exit(0);
}

/*CFDP Event Polling loop*/
void poll_cfdp_messages()
{
	char *eventTypes[] =	{
					"no event",
					"transaction started",
					"EOF sent",
					"transaction finished",
					"metadata received",
					"file data segment received",
					"EOF received",
					"suspended",
					"resumed",
					"transaction report",
					"fault",
					"abandoned"
				};
	CfdpEventType		type;
	time_t				time;
	int					reqNbr;
	CfdpTransactionId	transactionId;
	char				sourceFileNameBuf[256];
	char				destFileNameBuf[256];
	uvast			fileSize;
	MetadataList		messagesToUser;
	uvast			offset;
	unsigned int		length;
	unsigned int		recordBoundsRespected;
	CfdpContinuationState	continuationState;
	unsigned int		segMetadataLength;
	char			segMetadata[63];
	CfdpCondition		condition;
	uvast			progress;
	CfdpFileStatus		fileStatus;
	CfdpDeliveryCode	deliveryCode;
	CfdpTransactionId	originatingTransactionId;
	char				statusReportBuf[256];
	unsigned char		usrmsgBuf[256];
	MetadataList		filestoreResponses;
	uvast 			TID11;
	uvast			TID12;

	/*Main Event loop*/
	while (1) {

		/*Grab a CFDP event*/
		if (cfdp_get_event(&type, &time, &reqNbr, &transactionId,
						sourceFileNameBuf, destFileNameBuf,
						&fileSize, &messagesToUser, &offset, &length,
						&recordBoundsRespected, &continuationState,
						&segMetadataLength, segMetadata, &condition,
						&progress, &fileStatus, &deliveryCode,
						&originatingTransactionId, statusReportBuf,
						&filestoreResponses) < 0){
					fprintf(stderr, "Error: Failed getting CFDP event.");
					exit(1);
				}

		if (type == CfdpNoEvent){
			continue;	/*	Interrupted.		*/
		}

		/*Decompress transaction ID*/
		cfdp_decompress_number(&TID11,&transactionId.sourceEntityNbr);
		cfdp_decompress_number(&TID12,&transactionId.transactionNbr);

		/*Print Event type if debugging*/
		printf("\nEvent: type %d, '%s', From Node: " UVAST_FIELDSPEC
", Transaction ID: " UVAST_FIELDSPEC "." UVAST_FIELDSPEC ".\n", type,
				(type > 0 && type < 12) ? eventTypes[type]
				: "(unknown)", TID11, TID11, TID12);

		/*Parse Messages to User to get directory information*/
		while (messagesToUser) {

			/*Get user message*/
			memset(usrmsgBuf, 0, 256);
			if (cfdp_get_usrmsg(&messagesToUser, usrmsgBuf,
					(int *) &length) < 0) {
				putErrmsg("Failed getting user msg.", NULL);
				continue;
			}

			/*Set Null character at end of string*/
			if (length > 0) {
				usrmsgBuf[length] = '\0';
				printf("\tUser Message '%s'\n", usrmsgBuf);
			}
		}

		fflush(stdout);

	}
	return;
}
