/*
 * entropy_test.c
 * 
 * A utility program to generate random bytes using an entropy source and write them to stdout.
 * This program is designed to generate a specific amount of random data in binary format.
 * 
 * Compilation:
 * Use the following command to compile the program:
 * gcc -o entropy_test entropy_test.c -lici
 * 
 * - `entropy_src.h` is required and provides the `poll_entropy_src` function to fetch entropy bytes.
 * - Ensure that the `entropy_src` library is correctly installed and linked using `-lentropy_src`.
 * 
 * Execution:
 * ./entropy_test > random_data.bin
 * The above command generates approximately 10 MB of random data (1 KB * 10,000 iterations)
 * and writes it to the file `random_data.bin` in binary format.
 * 
 * Parameters:
 * - BUFFERSIZE: The size of the buffer used to store random bytes (1 KB).
 * - ITERATIONS: The number of iterations to poll the entropy source (~10 MB total output).
 */

#include <stdio.h>       // For standard I/O functions
#include <unistd.h>      // For the write and close system calls
#include <string.h>      // For memset
#include "entropy_src.h" // Custom header for entropy source

#define BUFFERSIZE 1024  // 1 KB buffer size for each read
#define ITERATIONS 10000 // Generate ~10 MB of random data (1 KB * 10,000 iterations)

int main(void) {
    unsigned char entropy_buffer[BUFFERSIZE] = {0}; // Buffer to hold random bytes
    size_t output_size = 0;                         // Number of bytes generated in each iteration
    int poll_result = 0;                            // Return value from `poll_entropy_src`

    // Generate random bytes and write them to stdout
    for (size_t counter = 0; counter < ITERATIONS; ++counter) {
        // Poll the entropy source to fill the buffer
        poll_result = poll_entropy_src(NULL, entropy_buffer, BUFFERSIZE, &output_size);
        if (poll_result < 0) {
            // Handle errors from the entropy source
            fprintf(stderr, "Error: %s\n", getErrorMessage(poll_result));
            return poll_result;
        } else {
            // Write the entropy bytes to stdout in binary format
            fwrite(entropy_buffer, 1, output_size, stdout);
        }
        // Clear the buffer to avoid residual data
        memset(entropy_buffer, 0, BUFFERSIZE);
    }

    return 0; // Indicate successful execution
}
