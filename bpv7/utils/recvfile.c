/**
 * @file recvfile.c
 * @brief Bundle Protocol File Receiver Implementation.
 *
 * Implements file receiver functionality for the Bundle Protocol (BP), enabling
 * files transferred over BP to be correctly named upon reception. Includes 
 * logic for receiving BP files, handling interruptions, and managing reception
 * state. Contains the main function, state management and signal handling utilities,
 * and the `BptestState` structure definition.
 * 
 * If compiled against a suitable security library decryption may be utilized (optional).
 *
 * @details
 * Functions included:
 * - main (or recvfile in ION LWT): Entry point for BP file receiver.
 * - receiveFile: Processes received files, saving metadata and content.
 * - _bptestState: Manages the BP test process state.
 * - handleQuit: Interrupt signal handler.
 *
 * Structures included:
 * - BptestState: Represents the BP test process state.
 * 
 * @note Based on bprecvfile by Scott Burleigh, Jet Propulsion Laboratory.
 * @note Use this utility with sendfile (see sendfile(1) for synopsis)
 * 
 * @warning Depends on proper BP infrastructure setup (ION DTN).
 * @warning The application's decryption features require a suitable cipher library.
 *
 * @author Sky DeBaun, Jet Propulsion Laboratory
 * @date December 2023
 * @copyright 2023, California Institute of Technology. All rights reserved.
 */

#ifndef darwin
#define _GNU_SOURCE
/* macos/darwin handles this differently */
#define _POSIX_C_SOURCE 200112L
#endif

//#define	BPRECVBUFSZ	(65536) // 65KB
#define BPRECVBUFSZ (262144)  // 256KB

#include "ionsec.h"
#include <bp.h>
#include <metadata.h>
#include <secrypt.h>



/**
 * @struct BptestState
 * @brief State of a Bundle Protocol test process.
 *
 * This structure maintains the state of a Bundle Protocol (BP) test process.
 * It includes the Service Access Point (SAP) and a flag indicating the process's
 * running status.
 */
typedef struct
{
	BpSAP	sap;
	int	running;
} BptestState;


/******************************************************************************/
/* _bptestState() */
/******************************************************************************/
/**
 * @brief Manages the state of the BP test.
 *
 * This function sets or retrieves the state of the Bundle Protocol (BP) test. When
 * a new state is provided, it updates the current task variable to this state. If
 * called with NULL, it returns the current task variable representing the BP test
 * state.
 *
 * @param newState Pointer to BptestState structure for new state. If NULL, retrieves
 *                 the current state.
 * @return Pointer to current or newly set BptestState structure.
 *
 * @note Used to maintain state across different stages of the BP test process,
 *       particularly in multitasking environments.
 */
static BptestState	*_bptestState(BptestState *newState)
{
	void		*value;
	BptestState	*state;

	if (newState)			/*	Add task variable.	*/
	{
		value = (void *) (newState);
		state = (BptestState *) sm_TaskVar(&value);
	}
	else				/*	Retrieve task variable.	*/
	{
		state = (BptestState *) sm_TaskVar(NULL);
	}

	return state;
}


/******************************************************************************/
/* handleQuit() */
/******************************************************************************/
/**
 * @brief Signal handler for interrupt signal.
 *
 * This function acts as the signal handler for interrupt signals, like SIGINT. It
 * updates the Bundle Protocol (BP) test state for interruptions and initiates
 * cleanup and termination of the BP receiving process.
 *
 * @param signum The signal number (typically SIGINT).
 *
 * @note Typically used for SIGINT (Ctrl+C) handling to gracefully terminate the
 *       BP receiving process. It sets the 'running' flag of the BP test state to 0,
 *       indicating process cessation.
 */
static void	handleQuit(int signum)
{
	BptestState	*state;

	isignal(SIGINT, handleQuit);
	writeMemo("[i] recvfile interrupted.");
	state = _bptestState(NULL);
	bp_interrupt(state->sap);
	state->running = 0;
} //---> end handleQuit()


/******************************************************************************/
/* cleanFile() */
/******************************************************************************/
/**
 * @brief Deletes files.
 *
 * This function is used to remove temporary files used by the program during 
 * the meta-extraction and decryption routine.
 *
 * @param filename File to remove
 * @return Integer (0 on success, -1 on failure)
 *
 */
int cleanFile(char *filename)
{
	int status = remove (filename);
	if (status != 0)
	{
		writeErrMemo("[!] recvfile: error deleting temporary file.");
		return -1;
	}
	return 0;
	
} //---> end cleanFile()


/******************************************************************************/
/* rename_and_clean() */
/******************************************************************************/
int rename_and_clean(char *oldFilename, char *newFilename)
{
    if (oldFilename && newFilename)
    {
        if (rename(oldFilename, newFilename) == 0)
        {
            return 0; //success
        }
        else
        {
            cleanFile(oldFilename); //Clean if rename fails
        }
    }

    return -1; //failure to rename
}


/******************************************************************************/
/* receiveFile() */
/******************************************************************************/
/**
 * @brief receiveFile - Receives a file via Bundle Protocol and saves it.
 *
 * This function handles the reception of a file over the Bundle Protocol. It 
 * reads incoming data, processes it for metadata extraction, and writes the 
 * file content to a temporary file. Post successful reception, it renames the 
 * temporary file to its original name using extracted metadata.
 *
 * @param sdr The SDR (Source Data Record) object for BP data handling.
 * @param dlv Pointer to BpDelivery struct with received data.
 * @return Returns 0 on successful file reception and processing, -1 on error.
 *
 * @note Assumes received data includes metadata for file naming. Generates a 
 *       temporary filename for initial data storage.
 * @warning Implements error handling for data integrity and I/O errors.
 */
static int	receiveFile(Sdr sdr, BpDelivery *dlv, int overwriteFlag, char *keyInput)
{
	static char	buffer[BPRECVBUFSZ];
	int			contentLength;
	int			remainingLength;
	int			fileHandle = -1;
	ZcoReader	reader;
	int			recvLength;
	char		progressText[80];
	Metadata 	metadata = {0};
	char **commands = NULL; //metadata commands
	int commandCount = 0;		


	unsigned char decryptFlag = 0; //default to no decryption
	char randomPart[10];
	char tmpFile[256];	
	int result; //simple status flag

	int delete_on_fail = 1; //to save or not to save (encrypted file)
	int decryption_failure = 0;


	/*CREATE RANDOM FILE(NAME)*/
    createUniqueFile(tmpFile, sizeof(tmpFile)); //temp file


	/*ION ----------------------------------- */
	contentLength = zco_source_data_length(sdr, dlv->adu);

	/* isprintf(progressText, sizeof progressText, "[i] recvfile is creating '%s'\
	, size %d.", tmpFile, contentLength);
	writeMemo(progressText); */

	fileHandle = iopen(tmpFile, O_WRONLY | O_CREAT, 0666);
	if (fileHandle < 0)
	{
		putErrmsg("[!] recvfile error: can't create temp file", tmpFile);
		goto exit;
	}


	/*FILE RECEPTION---------------------------------*/
	zco_start_receiving(dlv->adu, &reader);
	remainingLength = contentLength;
	oK(sdr_begin_xn(sdr));
	
	while (remainingLength > 0)
	{
		recvLength = BPRECVBUFSZ;
		if (remainingLength < recvLength)
		{
			recvLength = remainingLength;
		}

		if (zco_receive_source(sdr, &reader, recvLength, buffer) < 0)
		{
			putErrmsg("[!] recvfile error: can't receive bundle content.", tmpFile);
			close(fileHandle);
			oK(sdr_end_xn(sdr));
			goto exit;
		}
			/* write from buffer to temp file */
			ssize_t bytesWritten = write(fileHandle, buffer, recvLength );

			if (bytesWritten < recvLength)
				{
					putSysErrmsg("[!] recvfile error: can't write to file.",
							tmpFile);
					close(fileHandle);
					oK(sdr_end_xn(sdr));
					goto exit;
				}

				remainingLength -= (recvLength);
		
	}//end while loop
	close(fileHandle);


	/*READ FILE TO METADATA STRUCT--------------------*/
	result = extractMetadataFromFile(tmpFile, &metadata);
	if(result < 0)
	{
		putErrmsg("[!] recvfile: error extracting metadata from file.", tmpFile);
		goto exit;
	}


	/* DECRYPTION------------------------------------- */
	decryptFlag = metadata.eFlag;

	if(decryptFlag && keyInput) //if decryptFlag and cipher key
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
		
		
		/* decrypt file content */
		unsigned char *decrypted_fileContent = NULL;
		size_t decrypted_fileContentLength = 0;

		result = crypt_and_hash_buffer(1, metadata.aux_command, metadata.fileContent, &metadata.fileContentLength, &decrypted_fileContent, &decrypted_fileContentLength, CIPHER, MD, keyInput);
		if(result != 0)
		{				
			decryption_failure = 1;			
		}
		else
		{			
			/* update with decrypted data*/
			if (metadata.fileContent)
			{
				memset(metadata.fileContent, 0, metadata.fileContentLength);
				MRELEASE(metadata.fileContent); //free me first (avoid memory leak)!
			}
			metadata.fileContent = decrypted_fileContent; //free in Exit;
			metadata.fileContentLength = decrypted_fileContentLength;
		}
		
	} //end decryption routine

	/* if failure to decrypt and no key */
	if(decryptFlag && !keyInput)
	{
		decryption_failure = 1;
	}
	

	/*WRITE METADATA TO FILE--------------------------*/
	result = writeMetaDataContentToFile(&metadata, tmpFile);
	if (result < 0)
	{
		writeErrMemo ("[!] recvfile: error writing file.");
		goto exit;
	}

	/* extract any aux commands */
	if(metadata.aux_command_length > 0)
	{
		commands = parseCommandString((const char*) metadata.aux_command, &commandCount);		
	}

	if (!overwriteFlag && fileExists((const char*)metadata.filename)) 
	{
		if (generateNewFilename(&metadata) != 0) 
		{
			/* failed to generate a new filename */
			writeErrMemo("[!] recvfile error: temp filename creation.");
			goto exit;
		}
}

	/* rename temp file to correct filename */	
	result = rename_and_clean(tmpFile, (char*) metadata.filename);

	if(result >= 0)
	{
		char file_receipt_msg[512] = {0};
		snprintf(file_receipt_msg, sizeof(file_receipt_msg), "File received: %s of size: " UVAST_FIELDSPEC " bytes (received %d total bytes).", (char *) metadata.filename, (uvast)metadata.fileContentLength, contentLength);
		writeMemo(file_receipt_msg);

		/* Write to application window */
		/* 
		float time_difference = (float)calculateTimeDifference(metadata.timestamp);
		printf("\nFile received: %s \n", metadata.filename);
		printf("\t" UVAST_FIELDSPEC " bytes in %f seconds\n", (uvast)metadata.fileContentLength, time_difference/1000.0);
 		*/
	}
	else
	{
		writeErrMemo("[!] recvfile error: filename NULL.");
	}
	

	/* Print Aux commands (demonstration purposes only) */
	if(metadata.aux_command_length > 0)
	{
		executeAndFreeCommands(commands, commandCount);
	}


	/* DELETE ENCRYPTED DATA ON DECRYPTION FAILURE */
	if (delete_on_fail && decryption_failure) 
	{
		char decryption_failure_msg[256] = {0};
		snprintf(decryption_failure_msg, sizeof(decryption_failure_msg), "Decryption failure: %s deleted.", (char *)metadata.filename);
		writeErrMemo(decryption_failure_msg);
		cleanFile((char *)metadata.filename); //delete me
	}


exit:

	/* MEMORY OBFUSCATION AND CLEANUP*/	
	if (metadata.filename) 
	{
		MRELEASE(metadata.filename);
		metadata.filename = NULL;
	}

	if(metadata.fileContent)
	{
		MRELEASE(metadata.fileContent);
		metadata.fileContent = NULL;
	}

	if(metadata.aux_command)
	{
		MRELEASE(metadata.aux_command);
		metadata.aux_command = NULL;
	}

	if(metadata.filetype)
	{
		MRELEASE(metadata.filetype);
		metadata.filetype = NULL;
	}

	metadata.timestamp = 0;
	metadata.eFlag = 0;
	metadata.versionNumber = 0;
	metadata.aux_command_length = 0;
	metadata.filetypeLength = 0;
	metadata.fileContentLength = 0;

	memset(buffer, 0, BPRECVBUFSZ);
	memset(progressText, 0, 80);
	memset(randomPart, 0, 10);
	memset(tmpFile, 0, 256);

	contentLength = 0;
	recvLength = 0;

	/* ION */
	if (sdr_end_xn(sdr) < 0)
	{
		return -1;
	}

	return 0;

} //---> end receiveFile()


/******************************************************************************/
/* main() */
/******************************************************************************/
/**
 * @brief Main entry for the Bundle Protocol file receiver.
 *
 * This function is the main entry point for a file receiver application using
 * the Bundle Protocol (BP). It establishes the BP environment, opens an 
 * endpoint, and continuously processes files sent to this endpoint. Supports 
 * standard execution and ION Lightweight Threads (LWT).
 *
 * @param argc Argument count (standard execution).
 * @param argv Argument vector (standard execution).
 * @return Returns 0 on normal termination, -1 on error.
 *
 * @note In ION LWT context, the signature changes to use 'saddr' arguments. 
 *       Manages BP delivery objects and employs `receiveFile` for processing 
 *       received files.
 * @warning Implements error handling for application robustness.
 */
#if defined (ION_LWT)
int recvfile(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
             saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
    char *ownEid = (char *) a1;
    unsigned char overwriteFlag = 0;
    char *keyInput = NULL;

    // Check each argument for the overwrite flag or key
    saddr args[] = {a2, a3, a4, a5, a6, a7, a8, a9, a10};
    for (int i = 0; i < sizeof(args) / sizeof(saddr); i++) 
	{
        if (args[i] == NULL) 
		{
            continue; // Skip null arguments
        }

        if (!strcmp((char *)args[i], "-o") || !strcmp((char *)args[i], "--overwrite")) 
		{
            overwriteFlag = 1;
        } else if (!keyInput) 
		{ 			
            keyInput = (char *) args[i]; // keyInput is not already set
        }
    }
	
#else
int	main(int argc, char **argv)
{
	int status = 0; //default to good return value
	char	*ownEid = NULL; 
	int 	overwriteFlag = 0; // flag for overwriting files
	char 	*keyInput = NULL;  //file path or a literal value (if no key found)


	if (argc < 2) 
	{
		PUTS("Error: Missing own endpoint ID.");
		PUTS("\nUsage: recvfile <own endpoint ID> [-o|--overwrite] [decryption <key file path | literal key value>]\n");

		return 0;
	}

	/* first argument is always ownEid */
	ownEid = argv[1];

	for (int i = 2; i < argc; i++) 
	{
		if (!strcmp(argv[i], "-o") || !strcmp(argv[i], "--overwrite")) 
		{
			overwriteFlag = 1;
			continue;
		}

		/* if keyInput is already set avoid overwriting with additional arguments */
		if (!keyInput && argv[i][0] != '-') 
		{
			keyInput = argv[i];
		}
	}	

#endif
	BptestState	state = { NULL, 1 };
	Sdr		sdr;
	BpDelivery	dlv;

	if (ownEid == NULL)
	{
		PUTS("\nUsage: recvfile <own endpoint ID> [-o|--overwrite] [<key file path | literal key value>]\n");
		return 0;
	}

	if (bp_attach() < 0)
	{
		putErrmsg("[!] recvfile error: can't attach to BP.", NULL);
		return -1;
	}

	if (bp_open(ownEid, &state.sap) < 0)
	{
		putErrmsg("Can't open own endpoint.", ownEid);
		fprintf(stderr, "[!] recvfile error: cannot open own endpoint: %s\n", ownEid);
		status = -1;
		goto exit;
	}

	oK(_bptestState(&state));
	sdr = bp_get_sdr();
	isignal(SIGINT, handleQuit);
	writeMemo("[i] recvfile is running.");
	while (state.running)
	{
		if (bp_receive(state.sap, &dlv, BP_BLOCKING) < 0)
		{
			putErrmsg("[!] recvfile error: bundle reception failed.", NULL);
			state.running = 0;
			continue;
		}

		switch (dlv.result)
		{
		case BpEndpointStopped:
			state.running = 0;
			break;

		case BpPayloadPresent:
			if (receiveFile(sdr, &dlv, overwriteFlag, keyInput) < 0)
			{
				putErrmsg("[!] recvfile error: cannot continue.", NULL);
				state.running = 0;
			}

		/*	fall-through to default */
		default:
			break;
		}		

		bp_release_delivery(&dlv, 1);
	}

exit:
	bp_close(state.sap);
	PUTS("Stopping recvfile.");
	writeMemo("[i] Stopping recvfile.");
	writeErrmsgMemos();
	bp_detach();

	ownEid = NULL; 
	overwriteFlag = 0;
	keyInput = NULL;

	return status;

} //end---> main()
