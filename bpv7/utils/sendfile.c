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

 /*
 * CRITICAL: bp.h must be included FIRST to ensure platform.h
 * feature test macros are processed before any system headers.
 * This provides proper POSIX compliance for all platforms.
 */
 #include <bp.h>

#include "ionsec.h"
#include <metadata.h>
#include <secrypt.h>

#define RANDOMIZER_SIZE 21



/******************************************************************************/
/* secure_wipe()                                                          */
/******************************************************************************/
/**
 * @brief secure_wipe - Securely zeroes out memory.
 * * Uses a volatile pointer to ensure the compiler does not optimize
 * away the zeroing operation (Dead Store Elimination), which often
 * happens with standard memset() at the end of a function.
 * * @param v Pointer to memory to wipe.
 * @param n Number of bytes to wipe.
 */
static void secure_wipe(void *v, size_t n)
{
	volatile unsigned char *p = (volatile unsigned char *)v;
	while (n--)
	{
		*p++ = 0;
	}
}

/******************************************************************************/
/* extractBasename()                                                          */
/******************************************************************************/
/**
 * @brief extractBasename - Returns pointer to the basename (skipping directories).
 *        Handles both forward slash '/' and backslash '\' for cross-platform paths.
 *        If there is no slash, returns the original path string.
 *
 * @param path file name with (or without) path.
 */
static const char* extractBasename(const char *path)
{
	/* find the last occurrence of forward slash and backslash */
	const char *slashPosForward  = strrchr(path, '/');
	const char *slashPosBackward = strrchr(path, '\\');
	const char *slashPos         = NULL;

	/* pick whichever is furthest to the right */
	if (slashPosForward == NULL)
	{
		slashPos = slashPosBackward;
	}
	else if (slashPosBackward == NULL)
	{
		slashPos = slashPosForward;
	}
	else
	{
		/* both non-null; pick the one with the greater pointer value */
		slashPos = (slashPosForward > slashPosBackward)
				? slashPosForward
				: slashPosBackward;
	}

	/* if no slash found, return the original string */
	if (slashPos == NULL)
	{
		return path;
	}

	/* otherwise, skip beyond the slash and return pointer to the basename */
	return slashPos + 1;
}


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
static int run_sendfile(char *ownEid, char *destEid, char *fileName,
			int ttl, char *aux, char *svcClass, unsigned char encryptFlag, char *keyInput)
{
	int         priority = 0;
	BpAncillaryData ancillaryData = {0};
	BpCustodySwitch custodySwitch = NoCustodyRequested;
	BpSAP       sap = NULL;
	Sdr         sdr = NULL;
	SdrObject   fileRef = 0;
	struct stat statbuf;
	size_t      aduLength = 0;
	SdrObject   bundleZco;
	char        progressText[300] = {0};
	SdrObject   newBundle;
	size_t      readResult = 0;

	/* Initialize file handle early for safe cleanup */
	FILE        *file = NULL;

	/* Safe local copy of key to prevent crashes during wipe */
	char        *localKey = NULL;

	/*metadata vars*/
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
	char randInitializer[RANDOMIZER_SIZE];
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

	/* SAFE KEY HANDLING:
	* We make a local copy of the key. This ensures we can safely
	* wipe 'localKey' at the end without worrying if 'keyInput'
	* is a read-only string literal (which would crash if wiped).
	*/
	if (keyInput && encryptFlag == 1)
	{
		size_t keyLen = strlen(keyInput) + 1;
		localKey = MTAKE(keyLen);
		if (!localKey)
		{
			writeErrMemo("[!] sendfile: memory allocation error (key copy).");
			goto exit;
		}
		memcpy(localKey, keyInput, keyLen);
	}

	/* OPEN FILE FIRST (Fixes TOCTOU Race Condition)
	* We open the file now to acquire a file descriptor.
	* All subsequent checks use this descriptor, ensuring we
	* operate on the exact same resource.
	*/
	file = fopen(fileName, "rb");
	if (!file)
	{
		char open_file_error[256] = {0};
		snprintf(open_file_error, sizeof(open_file_error), "[!] sendfile: error opening file %s.", fileName);
		putErrmsg(open_file_error, NULL);
		goto exit;
	}

	if (fstat(fileno(file), &statbuf) < 0)
	{
		putErrmsg("[!] sendfile error: can't stat the file.", fileName);
		goto exit; // Goto exit ensures file is closed properly
	}

	/* Assign st_size to size_t. Safe as file is valid/open. */
	aduLength = (size_t)statbuf.st_size;

	if (aduLength == 0)
	{
		putErrmsg("[!] sendfile error: can't send file of length zero.", fileName);
		goto exit;
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

	/*CREATE RANDOM FILENAME (AND IV PERSONALIZER string)*/
	createUniqueFile(randInitializer, sizeof(randInitializer));

	/* READ THE FILE-------------------------------------*/
	size_t fileSize = aduLength;

	input_buffer = (unsigned char*)MTAKE(fileSize);

	if (!input_buffer)
	{
		writeErrMemo("[!] sendfile: memory allocation error (input_buffer).");
		goto exit;
	}

	readResult = fread(input_buffer, 1, fileSize, file);

	/* We don't need the file handle anymore, just the data in the buffer */
	fclose(file);
	file = NULL; // set NULL so the exit block doesn't try to close it again.

	if (readResult != (size_t)fileSize)
	{
		char read_failure_msg[256] = {0};
		snprintf(read_failure_msg, sizeof(read_failure_msg), "[!] sendfile: error reading from %s: expected %ld, got %zu bytes.",
				fileName, fileSize, readResult);
		writeErrMemo(read_failure_msg);

		/* Abort if the read failed. */
		goto exit;
	}

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
	metadata.filetype = (unsigned char *)(uintptr_t)filetype;
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
	/* Use only the basename for metadata. */
	const char *baseName = extractBasename(fileName);
	nameSize = strlen(baseName) + 1;

	name = MTAKE(nameSize);
	if (name == NULL)
	{
		writeErrMemo("[!] sendfile error: memory allocation (file name).");
		goto exit;
	}

	memset(name, 0, nameSize);
	memcpy(name, baseName, nameSize);

	metadata.filename = (unsigned char*)name;
	metadata.fileNameLength = nameSize;


	/*ADD FILENAME AND FILE CONTENT TO METADATA-----------*/
	if(encryptFlag == 1)
	{
		int result = -1; //default to failure

		/* ENCRYPT FILE CONTENTS */
		result = crypt_and_hash_buffer(0, (unsigned char*) randInitializer, input_buffer, &fileSize, &encrypted_content_buffer, &out_contentLength, CIPHER, MD, localKey);
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
	if (bundleZco == 0 || bundleZco == (SdrObject) ERROR)
	{
		putErrmsg("[!] sendfile error: can't create ZCO.", NULL);
	}
	else
	{
		/* Using %zu for size_t logging */
		isprintf(progressText, sizeof progressText, "[i] sendfile is sending %s of size %zu bytes (transmitted %zu total bytes)", fileName, (size_t)metadata.fileContentLength, aduLength);
		writeMemo(progressText);
		if (bp_send(sap, destEid, NULL, ttl, priority, custodySwitch,
			0, 0, &ancillaryData, bundleZco, &newBundle) <= 0)
		{
			/* Replaced non-standard itoa with snprintf */
			char sizeStr[32];
			snprintf(sizeStr, sizeof(sizeStr), "%zu", aduLength);
			putErrmsg("[!] sendfile error: can't send file in bundle.", sizeStr);
			fprintf(stderr, "sendfile: failed to send '%s'.\n",
					fileName);
			CHKZERO(sdr_begin_xn(sdr));
			zco_destroy(sdr, bundleZco);
			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Can't destroy ZCO.", NULL);
			}
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

	/* Check if file is still open (e.g. if we jumped here on error) */
	if (file) {
		fclose(file);
		file = NULL;
	}

	if(metadata_buffer)
	{
		secure_wipe(metadata_buffer, metabuffer_size);
		MRELEASE(metadata_buffer);
		metadata_buffer = NULL;
	}

	if(aux_command)
	{
		secure_wipe(aux_command, aux_length);
		MRELEASE(aux_command);
		aux_command = NULL;
	}

	if(input_buffer)
	{
		secure_wipe(input_buffer, fileSize);
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
		secure_wipe(encrypted_content_buffer, out_contentLength);
		MRELEASE(encrypted_content_buffer);
		encrypted_content_buffer = NULL;
	}

	/* Wipe the LOCAL copy of the key */
	if (localKey)
	{
		secure_wipe(localKey, strlen(localKey));
		MRELEASE(localKey);
		localKey = NULL;
	}

	secure_wipe(&metadata, sizeof(metadata));


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
	secure_wipe(&statbuf, sizeof(statbuf));

	fileName = NULL;
	ownEid = NULL;
	destEid = NULL;
	keyInput = NULL;

	secure_wipe(randInitializer, RANDOMIZER_SIZE);
	secure_wipe(progressText, sizeof(progressText));

	/*ION specific handles */
	/* Wiping local handle variables ensures no stale data on stack,
	and satisfies CodeQL that the wipe was intentional. */
	secure_wipe(&sdr, sizeof(sdr));
	secure_wipe(&sap, sizeof(sap));

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
	char    *ownEid = (char *) a1;
	char    *destEid = (char *) a2;
	char    *fileName = (char *) a3;
	char    *classOfService = (char *) a4;
	int    ttl = atoi((char *) a5);
	unsigned char    encryptFlag = 0;
	char    *keyInput = NULL;
	int    result = -1;
	char    *aux = NULL;

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
	char    *ownEid = NULL;
	char    *destEid = NULL;
	char    *fileName = NULL;
	char    *classOfService = NULL;
	char    *aux = NULL;
	int    ttl = 300; //time-to-live
	unsigned char     encryptFlag = 0; //encryption not enabled
	char    *keyInput = NULL;  //file path or literal key value
	int    result = -1;

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
		keyInput = argv[i];
		encryptFlag = 1;
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
