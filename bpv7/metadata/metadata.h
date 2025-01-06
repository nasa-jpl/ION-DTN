/**
 * @file metadata.h
 * @brief File handling and metadata processing functions for "embedding" metadata
 * during file transfers: includes encryption flags, time, versioning, file name, 
 * and aux command strings
 *
 * This file contains function prototypes and structures for handling file metadata, including
 * the serialization and deserialization of metadata, generating unique filenames,
 * checking file existence, and writing data to files. It provides comprehensive
 * support for file metadata operations, from creating and parsing metadata buffers
 * to file content processing and utility functions for string manipulation and time
 * calculation. The primary data structure used for handling metadata is the Metadata
 * struct.
 *
 * @details
 * Structs:
 * - Metadata: Stores information about a file, including name, type, content, and
 *   size.
 * 
 * Core Functions:
 * - createBufferFromMetadata: Serializes metadata into a buffer.
 * - extractMetadataFromFile: Extracts metadata from a file into a Metadata structure.
 * - writeMetaDataContentToFile: Writes metadata content to a file.
 * - writeBufferToFile: Writes a buffer to a file.
 * 
 * Metadata and Buffer Processing:
 * - htonll: Converts a 64-bit value from host to network byte order.
 * - ntohll: Converts a 64-bit value from network to host byte order.
 * - getCurrentTimeMs: Retrieves the current system time in milliseconds.
 * - calculateTimeDifference: Calculates the time difference in milliseconds.
 *
 * File and String Utilities:
 * - fileExists: Checks if a specified file exists.
 * - generateNewFilename: Generates a new filename to avoid name clashes.
 * - generateRandomString: Generates a random string of a specified size.
 * - createUniqueFile: Creates uniquely named file.
 * - appendNullChar: Appends a null character to an array.
 * - stripLeadingWhiteSpace: Removes leading whitespace from a string.
 * - isOnlyWhitespace: Checks if a string contains only whitespace.
 * - parseCommandString: Parses a command string into an array of tokens.
 * - executeAndFreeCommands: Prints commands and frees the memory allocated for them.
 * - myStrdup: _POSIX_C_SOURCE 200112L compatible version of strdup().
 *
 *
 * @note This file is essential for handling file metadata within the context of file
 *       transfers, providing a robust set of functionalities for various file and
 *       metadata operations.
 * 
 * @warning Proper error handling and memory management are crucial in these functions
 *          to ensure data integrity and avoid potential memory leaks or data corruption.
 *
 * @author Sky DeBaun, Jet Propulsion Laboratory
 * @date January 2024 
 * @copyright 2024, California Institute of Technology. All rights reserved.
 */


#ifndef METADATA_H
#define METADATA_H

#define TIMEOUT_SECONDS 5 //safeguard against infinite loop in generateNewFilename

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <ctype.h>
#include <time.h>

#ifdef _WIN32
#include <winsock2.h>
#include <Windows.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>  //htonl, ntohl
#endif


/******************************************************************************/
/* DATA STRUCTURES    DATA STRUCTURES     DATA STRUCTURES     DATA STRUCTURES */
/******************************************************************************/

/**
 * @struct Metadata
 * @brief Structure for storing metadata about a file.
 *
 * Encapsulates file metadata, including encryption flags, version number,
 * timestamp, and file details like commands, type, name, and content. Designed
 * for handling file metadata in transfer operations, ensuring all information
 * is structured together.
 *
 * @var eFlag
 * Encryption flag indicating file content encryption status.
 * 1 indicates encrypted, other values indicate not encrypted.
 *
 * @var versionNumber
 * Version number of the metadata format for versioning control and backward
 * compatibility.
 *
 * @var timestamp
 * Timestamp of file creation or last modification, as a 64-bit unsigned
 * integer in milliseconds since the Unix Epoch.
 *
 * @var aux_command_length
 * Length of the auxiliary command in this metadata, indicating the `aux_command`
 * array size. Size is a size_t value and may vary by operating system.
 *
 * @var aux_command
 * Pointer to an array of unsigned characters for an auxiliary command
 * associated with the file, length determined by `aux_command_length`.
 *
 * @var filetypeLength
 * Length of the file type information in this metadata, indicating the
 * `filetype` array size. Allows for variable-length file type descriptions.
 *
 * @var filetype
 * Pointer to an array of unsigned characters for the file type (e.g.,
 * "text/plain" or "image/jpeg"), length determined by `filetypeLength`.
 *
 * @var fileNameLength
 * Length of the file name in this metadata, indicating the `filename`
 * array size. Supports variable-length file names.
 *
 * @var filename
 * Pointer to an array of unsigned characters for the file name, including
 * directory path if necessary, length determined by `fileNameLength`.
 *
 * @var fileContentLength
 * Length of the file content in this metadata, indicating the `fileContent`
 * array size. Supports variable-length file contents.
 *
 * @var fileContent
 * Pointer to an array of unsigned characters for the file content, length
 * determined by `fileContentLength`.
 *
 * @note Designed to be flexible to support a wide range of file types and
 *       metadata requirements.
 * @warning Proper initialization and memory management of pointers in this
 *          structure are crucial to avoid memory leaks and ensure data integrity.
 */
typedef struct
{
    unsigned char eFlag;            // Encrypt flag, 1 for encrypted
    unsigned char versionNumber;    // Metadata format version number
    uint64_t timestamp;             // File creation or modification timestamp

    size_t aux_command_length;      // Auxiliary command length
    unsigned char* aux_command;     // Auxiliary command

    size_t filetypeLength;          // File type information length
    unsigned char* filetype;        // File type

    size_t fileNameLength;          // File name length
    unsigned char* filename;        // File name

    size_t fileContentLength;       // File content length
    unsigned char* fileContent;     // File content

} Metadata;




/******************************************************************************/
/*    CORE FUNCTIONS    CORE FUNCTIONS    CORE FUNCTIONS    CORE FUNCTIONS    */
/******************************************************************************/

/******************************************************************************/
/** createBufferFromMetadata */
/******************************************************************************/
/**
 * @brief Serializes metadata into a buffer for storage or transmission.
 *
 * Takes a Metadata structure and serializes its contents into a single buffer.
 * Includes encryption flag, version number, timestamp, and variable-length fields
 * like aux_command, filetype, filename, and file content. Calculates the buffer's
 * total size, allocates memory, and copies each field into the buffer. Returns
 * the buffer size through `offset`.
 *
 * @param meta Pointer to the Metadata structure containing data to serialize.
 * @param offset Pointer to a size_t variable where the buffer's total size will
 *        be stored.
 * @return A pointer to the allocated buffer with serialized metadata, or NULL
 *         if memory allocation fails.
 *
 * @note The caller is responsible for freeing the returned buffer to avoid
 *       memory leaks. Ensure all pointers in the Metadata structure (aux_command,
 *       filetype, filename, fileContent) are valid or NULL.
 * @warning Memory allocation failure returns NULL. Ensure proper error handling.
 */
unsigned char *createBufferFromMetadata(Metadata *meta, size_t *offset);


/******************************************************************************/
/** extractMetadataFromFile */
/******************************************************************************/
/**
 * @brief Extracts metadata from a file and populates a Metadata structure.
 *
 * Opens a file in binary read mode and sequentially reads metadata, including
 * encryption flags, version number, timestamp, auxiliary command length and
 * content, filetype length and content, filename length and content, and file
 * content length and content. Reads each piece into the corresponding field of
 * a provided Metadata structure. Dynamically allocates memory for variable-length
 * fields in Metadata (aux_command, filetype, filename, fileContent), which must
 * be freed by the caller.
 *
 * @param filename The path to the file from which metadata is to be extracted.
 * @param meta Pointer to a Metadata structure where extracted metadata will be
 *        stored.
 * @return The total number of bytes read from the file on success, or -1 on failure.
 *
 * @note The caller is responsible for initializing the Metadata structure and
 *       freeing dynamically allocated fields after use. Assumes the file format
 *       adheres to expected metadata structuring.
 * @warning Handles file operations and dynamic memory allocation errors. Returns
 *          -1 on error (e.g., file opening failure, memory allocation failure).
 *          Check return value before using the populated Metadata structure.
 */
int extractMetadataFromFile(const char *filename, Metadata *meta);


/******************************************************************************/
/** writeMetaDataContentToFile */
/******************************************************************************/
/**
 * @brief Writes metadata content to a specified file.
 *
 * Attempts to write the content of a `Metadata` structure to a file identified
 * by `tempFilename`. Validates the `Metadata` structure and its content before
 * writing. Opens the file in binary write mode and writes the `Metadata` content.
 * Reports errors and returns -1 for any operation failure.
 *
 * @param metaData Pointer to a `Metadata` structure with content to write.
 * @param tempFilename Name of the file where metadata content will be written.
 * @return 0 on successful write operation, -1 if an error occurs (invalid
 *         parameters, failure to open file, incomplete write operations).
 *
 * @note Opens the file in binary mode to ensure content is written exactly as it
 *       appears in memory, suitable for binary data.
 * @warning Checks the validity of the `Metadata` structure and content but does
 *          not provide detailed error codes for failure causes. Ensure metadata
 *          content and filename are valid before calling.
 */
int writeMetaDataContentToFile(const Metadata* metaData, const char* tempFilename);


/******************************************************************************/
/** writeBufferToFile */
/******************************************************************************/
/**
 * @brief Writes a buffer to a file.
 *
 * Takes a buffer and writes its contents to a file specified by `filename` in
 * binary mode. Writes the entire buffer content, ensuring accurate data transfer
 * for binary files.
 *
 * @param buffer Pointer to the data buffer to be written.
 * @param bufferSize Size of the buffer, in bytes.
 * @param filename Name of the file where the buffer will be written.
 * @return 0 on successful buffer write to file, -1 if file cannot be opened for
 *         writing or if not all data could be written.
 *
 * @note Opens the file in binary write mode ("wb") to maintain data integrity.
 * @warning If the file cannot be opened or an error occurs during writing, -1 is
 *          returned. Ensure error handling in such cases.
 */
int writeBufferToFile(unsigned char* buffer, size_t bufferSize, const char* filename);





/******************************************************************************/
/*                  METADATA AND BUFFER PROCESSING FUNCTIONS                  */
/******************************************************************************/

/******************************************************************************/
/** htonll */
/******************************************************************************/
/**
 * @brief Converts a 64-bit unsigned integer from host to network byte order.
 *
 * Checks the endianness of the host machine and swaps the byte order of the
 * provided 64-bit unsigned integer if necessary, to convert it from host byte
 * order to network byte order (big-endian). This ensures consistency across
 * different platforms for network protocol communications.
 *
 * @param val The 64-bit unsigned integer to convert to network byte order.
 * @return The value in network byte order.
 *
 * @note Uses `htonl` for 32-bit conversions as part of the process, assuming
 *       it handles conversions correctly according to the host's endianness.
 * @warning Ensure the system's `htonl` function behaves as expected, especially
 *          in environments or compilers where its implementation might vary.
 */
/* macos/darwin includes htonll, but as a nested macro, so we turn that off
   and use the ION version for compatibility */
#undef htonll
uint64_t htonll(uint64_t val);


/******************************************************************************/
/** ntohll */
/******************************************************************************/
/**
 * @brief Converts a 64-bit unsigned integer from network to host byte order.
 *
 * Leverages the `htonll` function to convert a 64-bit unsigned integer from
 * network byte order (big-endian) to host byte order. This ensures that data
 * received from the network is correctly interpreted on the host machine,
 * regardless of its native endianness.
 *
 * @param val The 64-bit unsigned integer to convert to host byte order.
 * @return The value in host byte order.
 *
 * @note Assumes `htonll` correctly converts between host and network byte orders
 *       by reversing the byte order if necessary, depending on the system's
 *       endianness. This makes `ntohll` directly applicable without additional
 *       logic.
 * @warning Relies on correct implementation of `htonll`. Incorrect behavior in
 *          `htonll` affects `ntohll`.
 */
 /* macos/darwin includes ntohll, but as a nested macro, so we turn that off
   and use the ION version for compatibility */
#undef ntohll
uint64_t ntohll(uint64_t val);


/******************************************************************************/
/** getCurrentTimeMs */
/******************************************************************************/
/**
 * @brief Retrieves the current system time in milliseconds since the Unix Epoch.
 *
 * Provides a cross-platform method to obtain the current system time with
 * millisecond precision. Utilizes platform-specific APIs to ensure accurate
 * time retrieval on Windows and POSIX-compliant systems (Linux, UNIX, macOS).
 *
 * @return The current system time in milliseconds since the Unix Epoch.
 *
 * @note On Windows, uses `GetSystemTimeAsFileTime`, converting FILETIME from
 *       Windows epoch to Unix Epoch, then to milliseconds. On POSIX systems,
 *       employs `clock_gettime` with `CLOCK_REALTIME` to get the current time.
 * @warning Precision and accuracy of returned time may depend on system
 *          capabilities and implementation of underlying OS APIs.
 */
uint64_t getCurrentTimeMs(void);


/******************************************************************************/
/** calculateTimeDifference */
/******************************************************************************/
/**
 * @brief Calculates the time difference in milliseconds.
 *
 * Computes the difference in milliseconds between the current time and a
 * previously transmitted time. If the current time is after or at the transmitted
 * time, it calculates the difference. Otherwise, it reports an error.
 *
 * @param transmitted_time_ms The transmitted time in milliseconds.
 * @return The time difference in milliseconds if current time is after or at the
 *         transmitted time, -1 to indicate an error if the current time is before.
 *
 * @note Assumes current and transmitted times are in milliseconds. Utilizes
 *       `getCurrentTimeMs()` to obtain the current time for comparison.
 * @warning Returns an unsigned 64-bit integer. Using -1 to indicate an error
 *          relies on underflow, representing -1 as the maximum unsigned value.
 */
uint64_t calculateTimeDifference(uint64_t transmitted_time_ms);





/******************************************************************************/
/*                         FILE AND STRING UTILITIES                          */
/******************************************************************************/

/******************************************************************************/
/** fileExists */
/******************************************************************************/
/**
 * @brief Checks if a specified file exists in the filesystem.
 *
 * Determines the existence of a file specified by `filename`. Uses `stat()`
 * to retrieve the file status. If the operation succeeds, it indicates the
 * file exists; otherwise, it does not. Efficient for validating file existence
 * before performing file operations.
 *
 * @param filename The path to the file whose existence needs to be checked.
 * @return Returns 1 if the file exists, 0 otherwise.
 *
 * @note Only checks for the existence, not distinguishing between file types.
 * @warning Ability to detect existence may be affected by access permissions.
 */
int fileExists(const char *filename);


/******************************************************************************/
/* generateNewFilename() */
/******************************************************************************/
/**
 * @brief Generates a unique filename by appending a numeric suffix to avoid naming conflicts.
 *
 * This function checks if the filename within the provided Metadata structure conflicts
 * with any existing file. If so, it appends a numeric suffix to the base name of the file,
 * increments the suffix until a unique filename is found, and updates the Metadata structure
 * with this new unique filename. The original filename is freed, and new memory is allocated
 * for the unique filename.
 *
 * @param metadata Pointer to the Metadata structure containing the original filename.
 * @return 0 on successfully generating a unique filename and updating the Metadata structure,
 *         -1 on error due to invalid inputs, snprintf errors, memory allocation failure, or
 *         if a unique filename cannot be generated within the limit of numeric suffixes.
 *
 * @note The function directly modifies the filename within the provided Metadata structure
 *       and handles memory allocation for the new unique filename. The caller must ensure
 *       that the Metadata structure is properly initialized before calling this function.
 *       The Metadata structure should be properly cleaned up to free allocated memory when
 *       it is no longer needed.
 * @warning The function assumes that the filename and fileNameLength fields of the Metadata
 *          structure are correctly set. It performs file existence checks and may perform
 *          several iterations to find a unique filename, impacting performance for directories
 *          with many similarly named files.
 */
int generateNewFilename(Metadata *metadata);


/******************************************************************************/
/** generateRandomString */
/******************************************************************************/
/**
 * @brief Generates a random string of a specified size.
 *
 * Fills the provided buffer with a random sequence of characters, chosen from
 * a predefined set of alphanumeric characters. The size parameter determines the
 * length of the random string generated. The function does not null-terminate
 * the string; it populates exactly `size` characters.
 *
 * @param str The buffer where the random string will be stored.
 * @param size The number of random characters to generate, not including a null
 *        terminator.
 *
 * @note Caller's responsibility to ensure the buffer `str` is at least `size`
 *       characters in size. For a null-terminated string, allocate `size + 1`
 *       and add the terminator after calling this function.
 * @warning Uses `rand()`, which may not provide cryptographically secure random
 *          numbers. For cryptographic security, use a different method.
 */
void generateRandomString(char *str, size_t size);


/******************************************************************************/
/** createUniqueFile */
/******************************************************************************/
/**
 * @brief Creates a uniquely named file.
 *
 * This function generates a filename using the current UNIX timestamp concatenated
 * with a random string. It then attempts to create a file with this unique name.
 * The generated filename is stored in the provided buffer. The buffer size must
 * be large enough to hold the filename and a null terminator. The function aims
 * to ensure that the generated file does not overwrite any existing file.
 *
 * @param buffer The buffer where the unique filename will be stored.
 * @param bufferSize The size of the buffer, which must be at least 20 characters
 *        to accommodate the filename format plus a null terminator.
 *
 * @return Returns 0 if the file was successfully created, or -1 if an error occurred
 *         (e.g., buffer size too small, file creation failed).
 *
 * @note The function uses the `time(NULL)` function to obtain the current time and
 *       `generateRandomString` to append a random string to the time, ensuring
 *       uniqueness. The buffer is expected to be large enough to hold this generated
 *       filename. Ensure the buffer has at least 20 characters for the filename and
 *       null terminator.
 * @warning The function initializes the random number generator with `srand(now)`,
 *          which may not be secure for cryptographic purposes. Additionally, it
 *          relies on the file system not to have a file with the generated name,
 *          which might not be guaranteed in a concurrent environment.
 */
int createUniqueFile(char *buffer, size_t bufferSize);


/******************************************************************************/
/** appendNullChar */
/******************************************************************************/
/**
 * @brief Appends a null character to an array of unsigned characters.
 *
 * Reallocates the given array to increase its size by one and appends a null
 * character ('\0') at the new last index. Ensures strings are properly null-
 * terminated for standard C string functions. Handles memory allocation failures.
 *
 * @param array The original array of unsigned characters to be resized.
 * @param currentSize The current size of the array before appending the null char.
 * @return A pointer to the resized array with a null character appended, or NULL
 *         if memory reallocation fails.
 *
 * @note The caller is responsible for freeing the memory allocated for the array,
 *       including after it has been resized by this function.
 * @warning If memory reallocation fails, the original array is not freed. Handle
 *          the original array's memory if NULL is returned.
 */
unsigned char* appendNullChar(unsigned char* array, size_t currentSize);


/******************************************************************************/
/** stripLeadingWhiteSpace */
/******************************************************************************/
/**
 * @brief Removes leading whitespace from a string.
 *
 * Iterates over the characters of the input string to find the first non-
 * whitespace character. Then shifts all characters to the left, effectively
 * removing any leading whitespace and modifying the original string to have
 * no leading whitespace. The modified string is null-terminated.
 *
 * @param str The string from which leading whitespace will be removed. This
 *        string is modified in place.
 *
 * @note Directly modifies the input string. Relies on `isspace` to identify
 *       whitespace characters, including space, tab, and newline.
 * @warning The input string must be mutable and null-terminated. Does not
 *          allocate or free memory, so the input string should have sufficient
 *          space for the operation.
 */
void stripLeadingWhiteSpace(char *str);


/******************************************************************************/
/** isOnlyWhitespace */
/******************************************************************************/
/**
 * @brief Checks if a string consists only of whitespace characters.
 *
 * Iterates over each character in the input string, checking if every character
 * is a whitespace character (space, tab, newline, etc., as defined by the C
 * standard library's `isspace` function). Returns true (1) if all characters are
 * whitespace, or false (0) otherwise.
 *
 * @param str The null-terminated string to be checked.
 * @return Returns 1 if the string is only made up of whitespace characters,
 *         and 0 if it contains any non-whitespace character.
 *
 * @note Relies on the `isspace` function to check for standard whitespace
 *       characters.
 * @warning The input string must be null-terminated to prevent reading beyond
 *          the buffer's end.
 */
int isOnlyWhitespace(const char* str);


/******************************************************************************/
/** parseCommandString */
/******************************************************************************/
/**
 * @brief Parses a command string into an array of tokens based on comma separation.
 *
 * Splits an input string into tokens by commas, removing leading whitespace from
 * each token. Dynamically allocates an array of strings to hold the tokens and
 * returns this array. The number of tokens found is stored in `count`. Ignores
 * tokens consisting solely of whitespace.
 *
 * @param inputString The command string to be parsed. This string is not modified.
 * @param count Pointer to an integer where the number of tokens found will be stored.
 * @return A pointer to an array of string tokens dynamically allocated. Each string
 *         in the array is also dynamically allocated and must be freed by the caller.
 *         The array itself must also be freed after use.
 *
 * @note Uses `strdup` for duplicating the input string for manipulation and `strtok`
 *       for tokenization, which modifies the duplicate of the input string. Also uses
 *       the `stripLeadingWhiteSpace` function to clean up tokens.
 * @warning The caller is responsible for freeing the memory allocated for the array
 *          of tokens and for each individual token to prevent memory leaks.
 */
char** parseCommandString(const char* inputString, int* count);


/******************************************************************************/
/** executeAndFreeCommands */
/******************************************************************************/
/**
 * @brief Prints commands and frees the memory allocated for them. As implemented
 *        this function is for demonstration purposes only: no command execution
 *        occurs.
 *
 * Iterates over an array of command strings, prints each command to standard
 * output, and then frees the memory allocated for each command string. After
 * all commands have been processed, it frees the memory allocated for the array
 * itself. Commands are printed with a comma separator between them, except for
 * the last command.
 *
 * @param commands An array of string pointers, each pointing to a dynamically
 *        allocated command string.
 * @param commandCount The number of commands in the array.
 *
 * @note Intended for use with arrays of strings that have been dynamically
 *       allocated, such as those created by `parseCommandString`. Assumes both
 *       the strings and the array holding them need to be freed.
 * @warning Directly frees the memory pointed to by `commands` and the strings it
 *          contains. `commands` and the string pointers it contains should not
 *          be used after calling this function to avoid dangling pointer access.
 */
void executeAndFreeCommands(char** commands, int commandCount);


/******************************************************************************/
/** myStrdup */
/******************************************************************************/
/**
 * @brief Duplicates a string with dynamic memory allocation.
 *
 * Allocates sufficient memory to hold a duplicate of the provided string `s`,
 * including the null terminator, ensuring compliance with `_POSIX_C_SOURCE
 * 200112L`. This standard compliance ensures adherence to POSIX standards for
 * C source code.
 *
 * @param s Pointer to the string to be duplicated.
 * @return A pointer to the newly allocated string that duplicates `s`, or NULL
 *         if memory allocation fails.
 *
 * @note The caller is responsible for freeing the returned string to avoid
 *       memory leaks. This function includes a check for successful memory
 *       allocation before proceeding with the duplication.
 */
char* myStrdup(const char* s);


#endif // METADATA_H
