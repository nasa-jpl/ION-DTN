/*Program to send a CFDP FDU with an originating transaction ID
 * Samuel Jero
 * December 2011*/
#include <cfdp.h>
#include <stdlib.h>
#include <stdio.h>

extern int createFDU(CfdpNumber *destinationEntityNbr, unsigned int utParmsLength,
		unsigned char *utParms, char *sourceFileName,
		char *destFileName, CfdpReaderFn readerFn,
		CfdpHandler *faultHandlers, int flowLabelLength,
		unsigned char *flowLabel, MetadataList messagesToUser,
		MetadataList filestoreRequests,
		CfdpTransactionId *originatingTransactionId,
		CfdpTransactionId *transactionId);

typedef struct
{
	CfdpHandler		faultHandlers[16];
	CfdpNumber		destinationEntityNbr;
	char			sourceFileNameBuf[256];
	char			*sourceFileName;
	char			destFileNameBuf[256];
	char			*destFileName;
	BpUtParms		utParms;
	MetadataList		msgsToUser;
	MetadataList		fsRequests;
	CfdpTransactionId	transactionId;
	CfdpTransactionId	OrigtransactionId;
} CfdpReqParms;


int main()
{
	CfdpReqParms	parms;

	if (cfdp_attach() < 0)
	{
		printf("Error: Can't initialize CFDP\n");
		return -1;
	}

	/*Setup parameters*/
	memset((char *) &parms, 0, sizeof(CfdpReqParms));
	parms.utParms.lifespan = 86400;
	parms.utParms.classOfService = BP_STD_PRIORITY;
	parms.utParms.custodySwitch = NoCustodyRequested;
	strcpy(parms.destFileNameBuf, "../rcvfile");
	parms.destFileName=parms.destFileNameBuf;
	strcpy(parms.sourceFileNameBuf, "../dotest");
	parms.sourceFileName=parms.sourceFileNameBuf;
	cfdp_compress_number(&parms.destinationEntityNbr, 1);
	cfdp_compress_number(&parms.OrigtransactionId.sourceEntityNbr, 2);
	cfdp_compress_number(&parms.OrigtransactionId.transactionNbr, 1);

	/*Make call*/
	if (createFDU(&(parms.destinationEntityNbr),
			sizeof(BpUtParms),
			(unsigned char *) &(parms.utParms),
			parms.sourceFileName,
			parms.destFileName, NULL,
			parms.faultHandlers, 0, NULL,
			parms.msgsToUser,
			parms.fsRequests,
			&(parms.OrigtransactionId),
			&(parms.transactionId)) < 0)
	{
		printf("Error: Can't send FDU\n");
		return -1;
	}

return 0;
}
