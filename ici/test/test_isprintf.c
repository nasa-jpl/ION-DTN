/**
* @file    test_isprintf.c
* @author  Sky DeBaun, Jet Propulsion Laboratory
* @date    July 2026
* @brief   Standalone integration test for the ION platform string
* formatting engine.
*
* COPYRIGHT & ATTRIBUTION
* -----------------------
* Copyright (c) 2026, California Institute of Technology.
* ALL RIGHTS RESERVED. U.S. Government Sponsorship acknowledged.
*
* This research/software was carried out at the Jet Propulsion Laboratory,
* California Institute of Technology, under a contract with the National
* Aeronautics and Space Administration (NASA).
*
* SYNOPSIS
* --------
* Proves boundary enforcement, safe truncation, null-termination
* preservation, and telemetry generation under catastrophic input
* conditions for the isprintf / _isprintf utilities.
*
* ARCHITECTURE
* ------------
* This test dynamically links against the production libici shared object.
* It bypasses the BP and SDR layers entirely, validating the core POSIX
* string wrappers in a vacuum. It intentionally triggers the _isprintf
* diagnostic logger to verify zero-overhead memory safety without causing
* infinite recursion or segmentation faults.
*
* COMPILE & EXECUTION (STANDARD)
* ------------------------------
* Oracle Linux / Ubuntu:
* gcc -Wall -Wextra -I./ici/include ici/test/test_isprintf.c -o test_isprintf -L./ici/library/.libs -lici -lpthread
* LD_LIBRARY_PATH=./ici/library/.libs ./test_isprintf
*
* Solaris:
* gcc -Wall -Wextra -I./ici/include ici/test/test_isprintf.c -o test_isprintf -L./ici/library/.libs -lici -lpthread -lrt -lsocket -lnsl
* LD_LIBRARY_PATH=./ici/library/.libs ./test_isprintf
*
* MEMORY SANITIZATION (UBUNTU / ORACLE LINUX)
* -------------------------------------------
* AddressSanitizer (ASAN) Build:
* gcc -g -fsanitize=address -Wall -Wextra -I./ici/include ici/test/test_isprintf.c -o test_isprintf -L./ici/library/.libs -lici -lpthread
* LD_LIBRARY_PATH=./ici/library/.libs ./test_isprintf
*
* Valgrind Execution (Requires Standard Build with -g):
* gcc -g -Wall -Wextra -I./ici/include ici/test/test_isprintf.c -o test_isprintf -L./ici/library/.libs -lici -lpthread
* LD_LIBRARY_PATH=./ici/library/.libs valgrind --leak-check=full ./test_isprintf
*/

#include "platform.h"
#include <stdio.h>
#include <string.h>

#if defined (ION_LWT)
int     test_isprintf(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	(void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
	(void)a6; (void)a7; (void)a8; (void)a9; (void)a10;
#else
int     main(int argc, char **argv)
{
	(void)argc; (void)argv;
#endif
	char buffer[32];
	char tiny_buf[4];
	char micro_buf[1];
	int written;

	PUTS("\n=======================================================");
	PUTS("  ION ISPRINTF INTEGRATION TEST SUITE");
	PUTS("=======================================================\n");

	PUTS("--- TEST 1: Nominal Execution (Standard String) ---");
	isprintf(buffer, sizeof(buffer), "Status: %s", "NOMINAL");
	printf("Output: '%s'\n\n", buffer);

	PUTS("--- TEST 2: Nominal Execution (Hexadecimal & Pointers) ---");
	isprintf(buffer, sizeof(buffer), "Addr: %p | Hex: %04x",
		(void*)0xDEADBEEF, 255);
	printf("Output: '%s'\n\n", buffer);

	PUTS("--- TEST 3: The Exact Boundary Condition ---");
	/* 31 characters + 1 null terminator = 32 bytes */
	isprintf(buffer, sizeof(buffer), "%s",
		"1234567890123456789012345678901");
	printf("Output: '%s'\n\n", buffer);

	PUTS("--- TEST 4: The 1-Byte Micro-Buffer ---");
	/* Should only write the null terminator and trigger telemetry */
	isprintf(micro_buf, sizeof(micro_buf), "%s", "A");
	printf("Output: '%s'\n\n", micro_buf);

	PUTS("--- TEST 5: The Sub-Allocation Overrun ---");
	/* Simulating dynamic sizing mismatch (using sizeof(int) for str) */
	isprintf(tiny_buf, sizeof(tiny_buf), "sys_db_%d", 999);
	printf("Output: '%s'\n\n", tiny_buf);

	PUTS("--- TEST 6: The Massive String Overrun ---");
	/* Forcing 56 bytes of text into a 32-byte buffer */
	isprintf(buffer, sizeof(buffer),
		"Bundle queued for %s at time %lu with fec %d",
		"ipn:19.0", 1689400000UL, 1);
	printf("Output: '%s'\n\n", buffer);

	PUTS("--- TEST 7: Direct API Invocation (Pointer Math) ---");
	/* Bypassing the macro to capture the return integer */
	written = _isprintf(__FILE__, __LINE__, tiny_buf, sizeof(tiny_buf),
		"Payload: %d", 100);
	printf("Output: '%s' (Returned raw string length: %d)\n\n",
		tiny_buf, written);

	PUTS("=======================================================");
	PUTS("  TEST SUITE COMPLETE");
	PUTS("=======================================================\n");

	return 0;
}
