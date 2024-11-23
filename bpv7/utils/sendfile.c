/**
 * @file sendfile.c
 * @brief Bundle Protocol File Sender Implementation.
 *
 * This file implements file transmission functionality using the Bundle Protocol (BP).
 * It includes the main logic for sending files over BP, handling command-line arguments,
 * and managing the sending process. The file contains the main function (or `sendfile` in
 * the ION Lightweight Threads context), and the `run_bpsendfile` function for the actual
 * file sending operation. 
 * 
 * If compiled against a suitable security library encryption may be utilized (optional).
 *
 * @details
 * Functions included:
 * - main (or sendfile in ION LWT): The entry point for the BP file sender.
 * - run_bpsendfile: Handles the setup and sending of a file over BP.
 *
 * @note This program based on bpsendfile by Scott Burleigh, Jet Propulsion Laboratory.
 * 
 * @note Pair this utility with recvfile - see recvfile(1)
 * 
 * @warning The application relies on the correct configuration of the
 *          underlying BP infrastructure (i.e. ION DTN)
 * @warning The application's encryption features require a suitable cipher library.
 *
 * @author Sky DeBaun, Jet Propulsion Laboratory
 * @date December 2023
 * @copyright 2023, California Institute of Technology.	All rights reserved.
 */

#ifndef darwin
#define _GNU_SOURCE
/* macos/darwin handles this differently */
#define _POSIX_C_SOURCE 200112L
#endif

#include "ionsec.h"
#include <bp.h>
#include <metadata.h> 
#include <secrypt.h>



/******************************************************************************/
/* run_sendfile() */
/******************************************************************************/
/**
 * @brief run_sendfile - Sends a file using the Bundle Protocol.
 *
 * This function is responsible for sending a file over the Bundle Protocol (BP). 
 * It handles the setup of BP parameters, file preparation, and the actual sending 
 * process. The function also includes the creation of a temporary file with 
 * metadata, which is then transmitted.
 *
 * @param ownEid The endpoint ID of the sender.
 * @param destEid The endpoint ID of the receiver.
 * @param fileName The name of the file to be sent.
 * @param ttl Time-to-live for the bundle.
 * @param svcClass The service class for the bundle (e.g., priority).
 * @return Returns 0 on successful file transmission, or 0 in case of an error.
 *
 * @note The function attaches to BP, opens an endpoint, and sends the file as a 
 * bundle.  It uses ancillary data and custody options for BP transmission.
 * @warning The function assumes that BP is properly set up and that the file 
 * to be sent exists.
 */
static int	run_sendfile(char *ownEid, char *destEid, char *fileName,
			int ttl, char *aux, char *svcClass, unsigned char encryptFlag, char *keyInput)
{
	int		    priority = 0;
	BpAncillaryData	ancillaryData = { 0, 0, 0 };
	BpCustodySwitch	custodySwitch = NoCustodyRequested;
	BpSAP		sap = NULL;
	Sdr		    sdr = NULL;
	Object		fileRef = 0;
	struct stat	statbuf;
	int		    aduLength = 0;
	Object		bundleZco;
	char		progressText[300] = {0};
	Object		newBundle;
	size_t		readResult = 0;


	/*metadata vars*/
	const int randomizer_size = 21;
	uint64_t timestamp = 0;
	size_t out_contentLength=0;
	size_t nameSize = 0;
	size_t metabuffer_size = 0;
	size_t aux_length = 0;

	unsigned char *encrypted_content_buffer = NULL;
	unsigned char *input_buffer = NULL;
	unsigned char *metadata_buffer = NULL;
	unsigned char *aux_command = NULL;
	const char* filetype = NULL;
	unsigned char versionNumber = 0;

	/*random name and supplemental iv personalizer*/
	char randInitializer[randomizer_size];
	char *name = NULL;


	/*ION BP SETUP---------------------------------------------*/
	if (svcClass == NULL)
	{
		priority = BP_STD_PRIORITY;
	}
	else
	{
		if (!bp_parse_quality_of_service(svcClass, &ancillaryData,
				&custodySwitch, &priority))
		{
			putErrmsg("[!] sendfile error: invalid class of service.",
					svcClass);
			PUTS("\nClass of service usage: " BP_PARSE_QUALITY_OF_SERVICE_USAGE "\n");

			return -1;
		}
	}

	if (bp_attach() < 0)
	{
		putErrmsg("[!] sendfile error: can't attach to BP.", NULL);
		return -1;
	}

	if (ownEid)
	{
		if (bp_open(ownEid, &sap) < 0)
		{
			putErrmsg("[!] sendfile error: can't open own endpoint.", ownEid);
			return -1;
		}
	}

	writeMemo("[i] sendfile is running.");
	if (stat(fileName, &statbuf) < 0)
	{
		if (sap)
		{
			bp_close(sap);
		}

		putErrmsg("[!] sendfile error: can't stat the file.", fileName);
		return -1;
	}

	aduLength = statbuf.st_size;
	if (aduLength == 0)
	{
		putErrmsg("[!] sendfile error: can't send file of length zero.", fileName);

		if (sap)
		{
			bp_close(sap);
		}
		return -1;
	}

	sdr = bp_get_sdr();
	CHKZERO(sdr_begin_xn(sdr));
	if (sdr_heap_depleted(sdr))
	{
		sdr_exit_xn(sdr);
		if (sap)
		{
			bp_close(sap);
		}

		putErrmsg("Low on heap space, can't send file.", fileName);
		goto exit;
	}

	/*CREATE RANDOM FILENAME (AND IV PERSONALIZER)*/    
	createUniqueFile(randInitializer, sizeof(randInitializer));

	/* READ THE FILE-------------------------------------*/
	FILE *file = fopen(fileName, "rb");
	if (!file) 
	{
		char open_file_error[256] = {0};
		snprintf(open_file_error, sizeof(open_file_error), "[!] sendfile: error opening file %s.", fileName);
		writeErrMemo(open_file_error);
		goto exit;
	}

	fseek(file, 0, SEEK_END);
	long fileSize = ftell(file);
	fseek(file, 0, SEEK_SET);

	input_buffer = (unsigned char*)MTAKE(fileSize);

	if (!input_buffer)
	{
		writeErrMemo("[!] sendfile: memory allocation error (input_buffer).");
		goto exit;
	}


	readResult = fread(input_buffer, 1, fileSize, file);
	if (readResult != fileSize)
	{
		char read_failure_msg[256] = {0};
		snprintf(read_failure_msg, sizeof(read_failure_msg), "[!] sendfile: error reading from %s: expected %ld, got %zu bytes.",
				fileName, fileSize, readResult);
		writeErrMemo(read_failure_msg);

	}
	fclose(file);


	/*BEGIN METADATA CREATION----------------------------*/
	Metadata metadata = {0};

	/* encryption flag */
	metadata.eFlag = encryptFlag;
		
	/*metadata library version*/
	versionNumber = 1;
	metadata.versionNumber = versionNumber;

	/*time stamp*/
	timestamp =  getCurrentTimeMs();
	metadata.timestamp = htonll(timestamp);

	/*file type*/
	filetype = "text/plain";
	metadata.filetype = (unsigned char*)filetype;
	metadata.filetypeLength = strlen((const char*) metadata.filetype);

	/* aux command string */		
	if(aux)
	{
		aux_length = strlen(aux)+1;
		aux_command = MTAKE(aux_length+1);

		if (!aux_command)
		{
			writeErrMemo("[!] sendfile error: memory allocation (aux_command).");
			goto exit;
		}
		memset(aux_command, 0, aux_length+1);
		memcpy(aux_command, aux, aux_length);		
	}
	else
	{
		aux_command = MTAKE(aux_length+1);

		if (!aux_command)
		{
			writeErrMemo("[!] sendfile error: memory allocation (aux_command).");
			goto exit;
		}
		memset(aux_command, 0, aux_length+1);
		memcpy(aux_command, "", aux_length+1);


	}
	metadata.aux_command = aux_command;
	metadata.aux_command_length = aux_length; //always at least zero length
	
	/* FILE NAME */
	nameSize = strlen(fileName)+1;

	name = MTAKE(nameSize);
	if (!name)
	{
		writeErrMemo("[!] sendfile error: memory allocation (file name).");
		goto exit;
	}

	memset(name, 0, nameSize);		
	memcpy(name, fileName,nameSize);
	metadata.filename = (unsigned char*) name;
	metadata.fileNameLength = nameSize;


	/*ADD FILENAME AND FILE CONTENT TO METADATA-----------*/
	if(encryptFlag == 1)
	{
		/* Get the key from ionsecadmin database */
		/* char        keyBuffer[512] = {0};
		int	        keyBufferLength = sizeof keyBuffer;
		int	        keyLength = 0;

		keyLength = sec_get_key(keyInput,
				&keyBufferLength, keyBuffer);
		if (keyLength <= 0)
		{
			putErrmsg("Can't fetch symmetric key.",
					keyInput);			
			return -1;
		} */


		int result = -1; //default to failure	

		/* ENCRYPT FILE CONTENTS */ 
		result = crypt_and_hash_buffer(0, (unsigned char*) randInitializer, input_buffer, (size_t *)&fileSize, &encrypted_content_buffer, &out_contentLength, CIPHER, MD, keyInput);			
		if(result != 0)
		{				
			writeErrMemo("[!] sendfile error: encryption.");
			goto exit;
		}			
		metadata.fileContent = encrypted_content_buffer;
		metadata.fileContentLength = out_contentLength;
	}		
	else /*if no encrypt flag*/
	{
		metadata.fileContent = input_buffer;
		metadata.fileContentLength = fileSize;
	}


	/*CONVERT METADATA TO BUFFER-----------------------------*/
	metadata_buffer = createBufferFromMetadata(&metadata, &metabuffer_size);

	/* write  buffer to file */
	if (writeBufferToFile(metadata_buffer, metabuffer_size, randInitializer) != 0)		
	{			
		writeErrMemo("[!] sendfile: error writing meta data to file.");
		goto exit;
	}

	aduLength = metabuffer_size; 


    /*PREPARE FILE FOR TRANSMISSION-------------------------------
	Note: "" specifies auto-deletion of randInitializer (temp) file after delivery 	*/
	fileRef = zco_create_file_ref(sdr, randInitializer,"", ZcoOutbound);

	if (sdr_end_xn(sdr) < 0 || fileRef == 0)
	{
		if (sap)
		{
			bp_close(sap);
		}

		putErrmsg("sendfile can't create file ref.", NULL);
		goto exit;
	}	

	bundleZco = ionCreateZco(ZcoFileSource, fileRef, 0, aduLength,
			priority, ancillaryData.ordinal, ZcoOutbound, NULL);
	if (bundleZco == 0 || bundleZco == (Object) ERROR)
	{
		putErrmsg("[!] sendfile error: can't create ZCO.", NULL);
	}
	else
	{
		isprintf(progressText, sizeof progressText, "[i] sendfile is sending %s of size %d bytes (transmitted %d total bytes)", fileName, metadata.fileContentLength, aduLength);
		writeMemo(progressText);
		if (bp_send(sap, destEid, NULL, ttl, priority, custodySwitch,
			0, 0, &ancillaryData, bundleZco, &newBundle) <= 0)
		{
			putErrmsg("[!] sendfile error: can't send file in bundle.",
					itoa(aduLength));
		}
		else
		{
			isprintf(progressText, sizeof progressText,
					"[i] sendfile successfully sent %s.",
					fileName);
			writeMemo(progressText);
		}
	}


exit:
	
	/*CLEAN ALL TRACES OF ENCRYPTION-----------------------------*/
	if( metadata_buffer)
	{
		memset(metadata_buffer, 0, metabuffer_size);
		MRELEASE (metadata_buffer);
		metadata_buffer = NULL;
	}

	if(aux_command)
	{
		memset(aux_command, 0, aux_length);
		MRELEASE(aux_command);
		aux_command = NULL;
	}

	if(input_buffer)
	{
		memset(input_buffer, 0, fileSize);
		MRELEASE(input_buffer);
		input_buffer = NULL;
	}
	
	if(name != NULL)
	{
		MRELEASE(name);
		name = NULL;
	}
	

	if(encryptFlag == 1)
	{
		memset(encrypted_content_buffer, 0, out_contentLength);
		MRELEASE(encrypted_content_buffer);
		encrypted_content_buffer = NULL;
	}

	memset(&metadata, 0, sizeof(metadata));
	

    
	/*ION CLEANUP------------------------------------------------*/
	CHKZERO(sdr_begin_xn(sdr));	
	zco_destroy_file_ref(sdr, fileRef);

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("[!] sendfile error: can't destroy file reference.", NULL);
	}
	if (sap)
	{
		bp_close(sap);
	}

	PUTS("Stopping sendfile.");
	writeMemo("[i] sendfile has stopped.");
	writeErrmsgMemos();
	bp_detach();


	/*SANITIZE USER INPUT AND WORKING DATA STRUCTURES------------*/
	memset(&statbuf, 0, sizeof(statbuf));
	memset(fileName, 0, strlen(fileName));
	fileName = NULL;	

	if(keyInput)
	{
		memset(keyInput, 0, strlen(keyInput));
		keyInput = NULL;
	}	

	memset(ownEid, 0, strlen(ownEid));
	ownEid = NULL;

	memset(destEid, 0, strlen(destEid));
	destEid = NULL;
	
	memset(randInitializer, 0, randomizer_size);
	memset(progressText, 0, 300);
	
	/*ION specific*/
	memset(&sdr, 0, sizeof(sdr)); //looks crazy..
	memset(&sap, 0, sizeof(sap));
	aduLength = 0;
	bundleZco = 0;
	newBundle = 0;
	fileRef = 0;
    
	return 0; //success

} //---> end run_bpsendfile()



/******************************************************************************/
/* main() */
/******************************************************************************/
/**
 * @brief sendfile (or main) - Entry point for the Bundle Protocol file sender.
 *
 * This function serves as the entry point for a file sender application using
 * the Bundle Protocol (BP). It parses command-line arguments (or function arguments
 * in the ION Lightweight Threads context) to set up necessary parameters for sending
 * a file over BP. The function then calls `run_bpsendfile` to perform the actual file
 * sending process.
 *
 * @param argc Argument count (used in standard execution mode).
 * @param argv Argument vector (used in standard execution mode).
 * @return Returns 0 on successful execution, 0 on error or incorrect usage.
 *
 * @note In the ION LWT context, the function signature changes to use 'saddr' arguments.
 *       The function handles the parsing of parameters and invokes the file sending
 *       logic implemented in `run_bpsendfile`.
 * @warning The function assumes that the command-line arguments (or function arguments
 *          in ION LWT) are provided correctly. It checks for the validity of these
 *          arguments before proceeding with the file sending process.
 */
#if defined (ION_LWT)
int	sendfile(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	*ownEid = (char *) a1;
	char	*destEid = (char *) a2;
	char	*fileName = (char *) a3;
	char	*classOfService = (char *) a4;
	int	ttl = atoi((char *) a5);
	unsigned char encryptFlag = 0;
	char *keyInput = NULL;
	int result = -1;

	/* Assign keyInput to the first non-null additional argument
	and set the encryptFlag accordingly */
	if (a6) 
	{
		keyInput = (char *) a6;
		encryptFlag = 1;
	} else if (a7) 
	{
		keyInput = (char *) a7;
		encryptFlag = 1;
	} else if (a8) 
	{
		keyInput = (char *) a8;
		encryptFlag = 1;
	} else if (a9) 
	{
		keyInput = (char *) a9;
		encryptFlag = 1;
	} else if (a10) 
	{
		keyInput = (char *) a10;
		encryptFlag = 1;
	}
#else
int	main(int argc, char **argv)
{
	char	*ownEid = NULL;
	char	*destEid = NULL;
	char	*fileName = NULL;
	char	*classOfService = NULL;
	char 	*aux = NULL;
	int	ttl = 300; //time-to-live
	unsigned char encryptFlag = 0; //encryption not enabled
	char *keyInput = NULL;  //file path or literal key value
	int result = -1;

	/* Parse user input------------------------------------------ */
	if (argc < 4) 
	{
		PUTS("\nUsage: sendfile <own endpoint ID> <destination endpoint ID> "
     "<file name> [-c|--class <class of service>] [-t|--ttl <time to live "
     "(seconds)>] [<-a | --aux <comma delimited command string>] [<key file path | literal key value>]\n");
		
    return 0;
	}

	ownEid = argv[1];
	destEid = argv[2];
	fileName = argv[3];

	for (int i = 4; i < argc; i++) 
	{
		if (!strcmp(argv[i], "-a") || !strcmp(argv[i], "--aux")) 
		{
			if (i + 1 >= argc) 
			{
				fprintf(stderr,"Error: Missing value after aux flag.");
				return 0;
			}
			aux = argv[++i];
			continue;
		}		
		
		if (!strcmp(argv[i], "-t") || !strcmp(argv[i], "--ttl")) 
		{
			if (i + 1 >= argc) 
			{
				fprintf(stderr,"Error: Missing TTL value after TTL flag.");
				return 0;
			}
			ttl = atoi(argv[++i]);
			continue;
		}

		if (!strcmp(argv[i], "-c") || !strcmp(argv[i], "--class")) 
		{
			if (i + 1 >= argc) 
			{
				PUTS("Error: Missing class of service value after class flag.");
				PUTS("\nclass of service: " BP_PARSE_QUALITY_OF_SERVICE_USAGE "\n");
				return 0;
			}
			classOfService = argv[++i];
			continue;
		}

		/* If the argument does not match any known flag, treat it as the encryption key */
		if (argv[i][0] != '-') 
		{
			keyInput = argv[i];
			encryptFlag = 1;
		}
	}	
#endif
	
	if (strcmp(ownEid, "dtn:none") == 0)	/*	Anonymous.	*/
	{
		ownEid = NULL;
	}

	result = run_sendfile(ownEid, destEid, fileName, ttl, aux, classOfService, encryptFlag, keyInput);
	/* consider additional user input sanitization here */
	return result;

} //---> end main()
