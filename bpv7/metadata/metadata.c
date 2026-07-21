/**
 * @file metadata.c
 * @brief Metadata function definitions for file transfers. Uses metadata to
 *        transfer associated file data, including encryption flags, version,
 *        time, file name, and aux command strings.
 *
 * Implements functions for handling file metadata, including serialization and
 * deserialization of metadata, generating unique filenames, checking file
 * existence, and writing data to files. Supports operations necessary for file
 * metadata management within file transfer contexts.
 *
 * Structs:
 * - Metadata: Data structure for storing metadata information.
 *
 * Core Functions:
 * - createBufferFromMetadata: Serializes metadata into a buffer.
 * - extractMetadataFromFile: Deserializes metadata from a file into a Metadata
 *   structure.
 * - writeMetaDataContentToFile: Writes metadata content to a file.
 * - writeBufferToFile: Writes a buffer to a file.
 *
 * Metadata and Buffer Processing:
 * - htonll: Host to network long long conversion.
 * - ntohll: Network to host long long conversion.
 * - getCurrentTimeMs: Gets current system time in milliseconds.
 * - calculateTimeDifference: Calculates time difference in milliseconds.
 *
 * File and String Utilities:
 * - fileExists: Checks file existence.
 * - generateNewFilename: Generates unique filename.
 * - generateRandomString: Generates a random string.
 * - createUniqueFile: Creates uniquely named file.
 * - appendNullChar: Appends null character to a string.
 * - stripLeadingWhiteSpace: Removes leading whitespace from a string.
 * - isOnlyWhitespace: Checks if string is only whitespace.
 * - parseCommandString: Parses command string into tokens.
 * - executeAndFreeCommands: Frees command strings and array.
 * - myStrdup: _POSIX_C_SOURCE 200112L compatible version of strdup().
 *
 *
 * @warning Proper error handling and memory management are emphasized throughout
 *          these implementations to prevent data loss, corruption, or leaks.
 *
 * @author Sky DeBaun, Jet Propulsion Laboratory
 * @date January 2024
 * @copyright 2024, California Institute of Technology. All rights reserved.
 */

#include <bp.h>
#include "metadata.h"

#define	MAX_METADATA_LEN	(4096)

/******************************************************************************/
/*    CORE FUNCTIONS    CORE FUNCTIONS    CORE FUNCTIONS    CORE FUNCTIONS    */
/******************************************************************************/

/******************************************************************************/
/** createBufferFromMetadata */
/******************************************************************************/
unsigned char *createBufferFromMetadata(Metadata *meta, size_t *offset)
{
	//the size/length of our Metadata members are as follows:
	size_t eFlagSize = sizeof(meta->eFlag);
	size_t versionSize = sizeof(meta->versionNumber);
	size_t timeStampSize = sizeof(meta->timestamp);

	size_t auxTypeSize = sizeof(meta->aux_command_length);
	size_t aux_command_length = meta->aux_command_length;

	size_t fileTypeSize = sizeof(meta->filetypeLength); //size of type
	size_t fileTypeLength = meta->filetypeLength; //length of array

	size_t fileNameTypeSize = sizeof(meta->fileNameLength);
	size_t fileNameLength = meta->fileNameLength;

	size_t contentTypeSize = sizeof(meta->fileContentLength);
	size_t contentLength = meta->fileContentLength;


	//total size needed
	size_t bufferSize = eFlagSize + versionSize + timeStampSize + aux_command_length +
		auxTypeSize + fileTypeLength + fileTypeSize + fileNameLength +
		fileNameTypeSize + contentLength + contentTypeSize;

	unsigned char* my_buffer = (unsigned char*)MTAKE(bufferSize);
	if (!my_buffer)
	{
		fprintf(stderr, "Buffer memory allocation failed!\n");
		return NULL;
	}
	memset(my_buffer, 0, bufferSize);


	/*ASSEMBLE THE BUFFER-----------------------------------*/

	/*offset 0*/
	int my_offset = 0;

	/*eFlag*/
	my_buffer[my_offset] = (unsigned char) meta->eFlag;
	my_offset += eFlagSize;


	/*version*/
	my_buffer[my_offset] = (unsigned char) meta->versionNumber;
	my_offset += versionSize;

	/*timestamp*/
	memcpy(my_buffer + my_offset, &(meta->timestamp), timeStampSize);
	my_offset += timeStampSize;

	/*aux command length*/
	memcpy(my_buffer + my_offset, &aux_command_length, auxTypeSize);
	my_offset += auxTypeSize; // length could be 0, but we offset for the type

	/*aux command*/
	if (meta->aux_command )
	{
		memcpy(my_buffer+my_offset, meta->aux_command, aux_command_length);
		my_offset += aux_command_length;
	}

	/*filetype length*/
	if (fileTypeLength > 0)
	{
		memcpy(my_buffer + my_offset, &fileTypeLength, fileTypeSize);
	}
	my_offset += fileTypeSize; // could be 0, but we still offset for the type

	/*filetype*/
	if (meta->filetype)
	{
		memcpy(my_buffer+my_offset, meta->filetype, fileTypeLength);
		my_offset += fileTypeLength;
	}

	/*filename length*/
	if (fileNameLength > 0)
	{
		memcpy(my_buffer + my_offset, &fileNameLength, fileNameTypeSize);
	}
	my_offset += fileNameTypeSize;

	/*filename*/
	if (meta->filename)
	{
		memcpy(my_buffer+my_offset, meta->filename, fileNameLength);
		my_offset += fileNameLength;
	}

	/*content length*/
	if (contentLength > 0)
	{
		memcpy(my_buffer + my_offset, &contentLength, contentTypeSize);
		my_offset += contentTypeSize;
	}

	/*content*/
	if (meta->fileContent)
	{
		memcpy(my_buffer+my_offset, meta->fileContent, contentLength);
		my_offset += contentLength;
	}

	*offset = my_offset; //the total length of my_buffer
	return my_buffer;

} //end createBufferFromMetadata--->///


/******************************************************************************/
/** writeMetaDataContentToFile */
/******************************************************************************/
int writeMetaDataContentToFile(const Metadata *metaData, const char *tempFilename)
{
	if (metaData == NULL || metaData->fileContent == NULL)
	{
		fprintf(stderr, "Invalid content.\n");
		return -1;
	}

	if (!metaData->filename)
	{
		fprintf(stderr, "Error: file name missing. (\n");
		return -1;
	}

	FILE *file = fopen((const char *)tempFilename, "wb");
	if (file == NULL)
	{
		perror("Error creating file.");
		return -1;
	}

	size_t written = fwrite(metaData->fileContent, 1, metaData->fileContentLength, file);
	if (written != metaData->fileContentLength)
	{
		fprintf(stderr, "Error writing to file\n");
		fclose(file);
		return -1;
	}

	fclose(file);
	return 0;

} //end writeMetaDataContentToFile--->///


/******************************************************************************/
/** writeBufferToFile */
/******************************************************************************/
int writeBufferToFile(unsigned char *buffer, size_t bufferSize, const char *filename)
{
	FILE *file = fopen(filename, "wb");
	if (!file)
	{
		perror("Error opening file for writing");
		return -1;
	}

	size_t written = fwrite(buffer, 1, bufferSize, file);
	fclose(file);

	if (written != bufferSize)
	{
		fprintf(stderr, "Error writing to file\n");
		return -1;
	}

	return 0; // Success

}// end writeBufferToFile--->///






/******************************************************************************/
/** extractMetadataFromFile */
/******************************************************************************/
int extractMetadataFromFile(const char *filename, Metadata *meta)
{
	/*Open the file*/
	FILE *file = fopen(filename, "rb");
	if (!file)
	{
		perror("Error opening file");
		return -1;
	}

	int offset = 0;
	int bytes_read = 0;

	/*Unkown offsets - TBD*/
	//size_t aux_command_length = 0;
	//size_t fileTypeLength = 0;
	//size_t fileNameLength = 0;
	//size_t contentLength = 0;

	/*Known offsets*/
	size_t eFlagSize = sizeof(meta->eFlag);
	size_t versionSize = sizeof(meta->versionNumber);
	size_t timeStampSize = sizeof(meta->timestamp);
	size_t auxTypeSize = sizeof(meta->aux_command_length);
	size_t fileTypeSize = sizeof(meta->filetypeLength);
	size_t fileNameTypeSize = sizeof(meta->fileNameLength);
	size_t contentTypeSize = sizeof(meta->fileContentLength);


	/*eFlag*/
	fseek(file, offset, SEEK_SET);
	bytes_read = fread(&meta->eFlag, 1, eFlagSize, file);
	offset += bytes_read;


	/*version*/
	fseek(file, offset, SEEK_SET);
	bytes_read = fread(&meta->versionNumber, 1, versionSize, file);
	offset += bytes_read;


	/*timestamp*/
	uint64_t my_time;
	fseek(file, offset, SEEK_SET);
	bytes_read = fread(&my_time, 1, timeStampSize, file);
	my_time = ntohll(my_time);
	meta->timestamp = my_time;
	offset += bytes_read;


	/*aux command*/
	fseek(file, offset, SEEK_SET);
	bytes_read = fread(&meta->aux_command_length, 1, auxTypeSize, file);
	offset += bytes_read;
	if (meta->aux_command_length > MAX_METADATA_LEN)
	{
		fclose(file);
		return -1;
	}

	fseek(file, offset, SEEK_SET);
	if (meta->aux_command_length > 0)
	{
		/*	Allocate one extra byte and NUL-terminate: this
		 *	field is consumed downstream as a C string
		 *	(parseCommandString/myStrdup -> strlen), so a
		 *	received, non-terminated buffer would be read past
		 *	its end.					*/
		meta->aux_command = MTAKE(meta->aux_command_length + 1);
		if(meta->aux_command == NULL)
		{
			fclose(file);
			return -1;
		}
		bytes_read = fread(meta->aux_command, 1, meta->aux_command_length, file);
		meta->aux_command[bytes_read] = '\0';
		offset += bytes_read;
	}



	/*file type*/
	fseek(file, offset, SEEK_SET);
	bytes_read = fread(&meta->filetypeLength, 1, fileTypeSize, file);
	offset += bytes_read;
	if (meta->filetypeLength > MAX_METADATA_LEN)
	{
		fclose(file);
		return -1;
	}

	fseek(file, offset, SEEK_SET);
	/*	NUL-terminate: consumed downstream as a C string.	*/
	meta->filetype = MTAKE(meta->filetypeLength + 1);
	if(meta->filetype == NULL)
	{
		fclose(file);
		return -1;
	}
	bytes_read = fread(meta->filetype, 1, meta->filetypeLength, file);
	meta->filetype[bytes_read] = '\0';
	offset += bytes_read;


	/*filename*/
	fseek(file, offset, SEEK_SET);
	bytes_read = fread(&meta->fileNameLength, 1, fileNameTypeSize, file);
	offset += bytes_read;
	if (meta->fileNameLength > MAX_METADATA_LEN)
	{
		fclose(file);
		return -1;
	}

	fseek(file, offset, SEEK_SET);
	/*	NUL-terminate: consumed downstream as a C string
	 *	(generateNewFilename -> strrchr/strncpy).		*/
	meta->filename = MTAKE(meta->fileNameLength + 1);
	if(meta->filename == NULL)
	{
		fclose(file);
		return -1;
	}
	bytes_read = fread(meta->filename, 1, meta->fileNameLength, file);
	meta->filename[bytes_read] = '\0';
	offset += bytes_read;


	/*file content*/
	fseek(file, offset, SEEK_SET);
	bytes_read = fread(&meta->fileContentLength, 1, contentTypeSize, file);
	offset += bytes_read;
	{
		Sdr		sdr = getIonsdr();
		SdrObject	iondbObj = getIonDbObject();
		IonDB		iondb;
		size_t		maxContentLen;

		sdr_read(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
		maxContentLen = iondb.parmcopy.wmSize / 10;	/*	10%	*/
		if (meta->fileContentLength > maxContentLen)
		{
			fclose(file);
			return -1;
		}
	}

	meta->fileContent = MTAKE(meta->fileContentLength); //free me
	if(meta->fileContent == NULL)
	{
		fclose(file);
		return -1;
	}
	bytes_read = fread(meta->fileContent, 1, meta->fileContentLength, file);
	offset += bytes_read;


	fclose(file);
	return offset; //total bytes read

} //end extractMetadataFromFile--->///





/******************************************************************************/
/*                  METADATA AND BUFFER PROCESSING FUNCTIONS                  */
/******************************************************************************/

/******************************************************************************/
/** htonll */
/******************************************************************************/
uint64_t htonll(uint64_t val) {
	// only swap if we're on a little-endian machine
	if (htonl(1) == 1)
	{
		//on a big-endian machine, no swap needed
		return val;
	} else
	{
		//swap bytes
		return ((uint64_t)htonl(val & 0xFFFFFFFF) << 32) | htonl(val >> 32);
	}

} //end htonll--->///


/******************************************************************************/
/** ntohll */
/******************************************************************************/
uint64_t ntohll(uint64_t val)
{
	return htonll(val); // This assumes htonll is defined as above

}// end ntohll--->///


/******************************************************************************/
/** getCurrentTimeMs */
/******************************************************************************/
uint64_t getCurrentTimeMs(void)
{
	/* Current time in ms since the Epoch */
	struct timespec spec;
	clock_gettime(CLOCK_REALTIME, &spec);
	return (uint64_t)spec.tv_sec * 1000 + (uint64_t)spec.tv_nsec / 1e6;

} //end getCurrentTimeMs--->///


/******************************************************************************/
/** calculateTimeDifference */
/******************************************************************************/
uint64_t calculateTimeDifference(uint64_t transmitted_time_ms)
{
	uint64_t current_time_ms = getCurrentTimeMs();

	if (current_time_ms >= transmitted_time_ms)
	{
		uint64_t time_difference_ms = current_time_ms - transmitted_time_ms;
		return time_difference_ms;
	} else
	{
		printf("Error: Current time is before the transmitted time.\n");
		return -1; //error condition
	}
} //end calculateTimeDifference--->///





/******************************************************************************/
/*                         FILE AND STRING UTILITIES                          */
/******************************************************************************/

/******************************************************************************/
/* fileExists() */
/******************************************************************************/
int fileExists(const char *filename)
{
	struct stat buffer;
	return (stat(filename, &buffer) == 0) ? 1 : 0;

} //end fileExists--->///


/******************************************************************************/
/* generateNewFilename() */
/******************************************************************************/
int generateNewFilename(Metadata *metadata)
{
	if (!metadata || !metadata->filename || metadata->fileNameLength <= 0)
	{
		return -1; //invalid input
	}

	char *originalFilename = (char *)metadata->filename;
	char *dot = strrchr(originalFilename, '.');
	char baseName[256], extension[256] = "", newFilename[512];
	if (dot)
	{
		size_t      diff;
		size_t      maxLen = sizeof(baseName) - 1;

		if (dot > originalFilename)
		{
			diff = dot - originalFilename;
		}
		else
		{
			diff = 0; // This implies the filename starts with '.', e.g., ".my_file"
		}

		int baseLength = (diff < maxLen) ? diff : maxLen;
		strncpy(baseName, originalFilename, baseLength);
		baseName[baseLength] = '\0';
		strncpy(extension, dot, sizeof(extension) - 1);  //copy the extension including the dot
	} else
	{
		strncpy(baseName, originalFilename, sizeof(baseName) - 1);
		baseName[sizeof(baseName) - 1] = '\0';  // Ensure null-termination
	}

	/*
	 calculate max space for baseName (avoid possible overflow in snprintf)
	 estimate max count digits (10 for int), plus 1 for '_', plus extension length, plus null terminator
	 */
	size_t maxCountDigits = 10; // max digits an int can have
	size_t spaceForBaseName = sizeof(newFilename) - maxCountDigits - strlen(extension) - 2; // -2 for '_' and null terminator

	if (strlen(baseName) > spaceForBaseName)
	{
		baseName[spaceForBaseName] = '\0'; //truncate baseName to fit
	}
	int count = 1, neededSize = 0;

	time_t startTime = time(NULL);  //as safeguard against infinite loop
	do
	{
		neededSize = snprintf(newFilename, sizeof(newFilename), "%s_%d%s", baseName, count++, extension);
		if (neededSize < 0 || (size_t)neededSize >= sizeof(newFilename))
		{
			return -1; //snprintf error or potential truncation
		}
		if (difftime(time(NULL), startTime) > TIMEOUT_SECONDS)
		{
			return -1;// exit after TIMEOUT_SECONDS (prevent infinite loop)
		}
		if (!fileExists(newFilename))
		{
			break;
		}

	} while (1);

	MRELEASE(metadata->filename);

	size_t newLength = strlen(newFilename) + 1;
	metadata->filename = (unsigned char *)MTAKE(newLength);
	if (!metadata->filename)
	{
		return -1; //allocation failed
	}

	memcpy(metadata->filename, newFilename, newLength);
	metadata->fileNameLength = newLength - 1;

	return 0; //success

} //end generateNewFilename--->///


/******************************************************************************/
/* generateRandomString */
/******************************************************************************/
void generateRandomString(char *str, size_t size)
{
	const char charset[] =
		"abcdefghijklmnopqrstuvwxyz"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"0123456789";

	if (size) {
		for (size_t n = 0; n < size; n++) {
			int key = rand() % (int) (sizeof charset - 1);
			str[n] = charset[key];
		}
	}
} //end generateRandomString--->///


/******************************************************************************/
/* createUniqueFile */
/******************************************************************************/
int createUniqueFile(char *buffer, size_t bufferSize)
{
	if (bufferSize < 20) //buffer should be long enough for our filename format
	{
		fprintf(stderr, "Buffer size is too small for the filename.\n");
		return -1;
	}

	time_t now = time(NULL);
	srand(now);
	char randomPart[10] = {0};
	generateRandomString(randomPart, sizeof(randomPart)-1); // this null-terminates

	/* Construct the filename with the time and random component */
	/* Example: "12345678abcdefgh" */
	snprintf(buffer, bufferSize, "%ld%s", (long)now, randomPart);

	FILE *file = fopen(buffer, "wb");
	if (!file)
	{
		fprintf(stderr, "Failed to create the file: %s\n", buffer);
		return -1;
	}
	fclose(file); //close the descriptor (we only need to create file)

	return 0;

} //end createUniqueFile--->///


/******************************************************************************/
/** appendNullChar */
/******************************************************************************/
unsigned char *appendNullChar(unsigned char *array, size_t currentSize)
{
	unsigned char* resizedArray = realloc(array, currentSize + 1);
	if (resizedArray == NULL)
	{
		return NULL;
	}
	resizedArray[currentSize] = '\0';

	return resizedArray;

} //end appendNullChar--->///


/******************************************************************************/
/** stripLeadingWhiteSpace */
/******************************************************************************/
void stripLeadingWhiteSpace(char *str)
{
	char *p = str;
	int offset = 0;

	while (isspace((unsigned char)*p))
	{
		p++;
		offset++;
	}

	if (offset > 0) // shift to remove leading white spaces
	{
		while (*p)
		{
			*(p - offset) = *p;
			p++;
		}
		*(p - offset) = '\0';
	}

} //end stripLeadingWhiteSpace--->///


/******************************************************************************/
/** isOnlyWhitespace */
/******************************************************************************/
int isOnlyWhitespace(const char* str)
{
	while (*str)
	{
		if (!isspace((unsigned char)*str))
			return 0;
		str++;
	}
	return 1;

} //end isOnlyWhitespace--->///


/******************************************************************************/
/** parseCommandString */
/******************************************************************************/
char **parseCommandString(const char *inputString, int *count)
{
	char   *str = myStrdup(inputString);
	size_t  capacity = 5;
	char  **result;
	*count = 0;
	if (str == NULL)
	{
		return NULL;
	}

	result = MTAKE(capacity * sizeof(char *));
	if (result == NULL)
	{
		MRELEASE(str);
		return NULL;
	}

	char *token = strtok(str, ",");

	while (token != NULL)
	{
		stripLeadingWhiteSpace(token);

		if ((size_t)*count >= capacity)
		{
			char **newResult;

			capacity *= 2;
			newResult = realloc(result, capacity * sizeof(char *));
			if (newResult == NULL)
			{
				int j;
				for (j = 0; j < *count; j++)
				{
					MRELEASE(result[j]);
				}
				MRELEASE(result);
				MRELEASE(str);
				*count = 0;
				return NULL;
			}

			result = newResult;
		}
		if (!isOnlyWhitespace(token))
		{
			result[*count] = myStrdup(token);
			(*count)++;
		}
		token = strtok(NULL, ",");
	}

	MRELEASE(str);
	return result;

} //end parseCommandString--->///


/******************************************************************************/
/** executeAndFreeCommands */
/******************************************************************************/
void executeAndFreeCommands(char **commands, int commandCount)
{
	int i;
	printf("\tAux commands: ");

	for (i = 0; i < commandCount; i++)
	{
		printf("%s", commands[i]);
		if (i < commandCount - 1)
		{
			printf(", "); //print after all but last
		}
		fflush(stdout);
		MRELEASE(commands[i]);
	}
	printf("\n");
	MRELEASE(commands);

} //end executeAndFreeCommands--->///


/******************************************************************************/
/** myStrdup */
/******************************************************************************/
char *myStrdup(const char *s)
{
	size_t len = strlen(s) + 1; //+1 for null terminator
	char* new_str = MTAKE(len);
	if (new_str == NULL) return NULL; //check malloc's return value
	return memcpy(new_str, s, len);

} // end myStrdup--->///
