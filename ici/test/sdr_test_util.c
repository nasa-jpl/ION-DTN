/*
	sdr_test_util.c:	SDR transaction reversibility test utility.

	This utility provides direct control over SDR transactions for
	testing transaction reversibility mechanisms in ION 4.1.3+.

	Modes:
	  test_reversal       - Create transaction with modifications, cancel it
	  test_no_reversal    - Create transaction without modifications, cancel it
	  verify_state        - Verify SDR is in a consistent state

	Author: ION Development Team, 2024
									*/
/*									*/
/*	Copyright (c) 2024, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*									*/

#include "platform.h"
#include "ion.h"

typedef enum
{
	MODE_TEST_REVERSAL,
	MODE_TEST_NO_REVERSAL,
	MODE_VERIFY_STATE
} TestMode;

static void	printUsage(void)
{
	PUTS("Usage: sdr_test_util <mode> <sdr_name>");
	PUTS("");
	PUTS("Modes:");
	PUTS("  test_reversal      - Begin transaction, modify SDR, cancel");
	PUTS("                       (should trigger reversal)");
	PUTS("  test_no_reversal   - Begin transaction, read only, cancel");
	PUTS("                       (should NOT trigger reversal)");
	PUTS("  verify_state       - Verify SDR is in consistent state");
	PUTS("");
	PUTS("Example:");
	PUTS("  sdr_test_util test_reversal ion");
}

static int	testReversal(Sdr sdr)
{
	Object		obj;
	char		testData[5000];
	int		i;

	/* Initialize test data */
	for (i = 0; i < 5000; i++)
	{
		testData[i] = (char) (i % 256);
	}

	putErrmsg("sdr_test_util: Beginning transaction with modifications.",
			NULL);

	/* Begin transaction */
	if (sdr_begin_xn(sdr) < 0)
	{
		putErrmsg("sdr_test_util: Can't begin transaction.", NULL);
		return -1;
	}

	/* Allocate space in SDR */
	obj = sdr_malloc(sdr, 5000);
	if (obj == 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("sdr_test_util: Can't allocate SDR space.", NULL);
		return -1;
	}

	/* Write data to SDR - ensures sdr->modified flag is set */
	sdr_write(sdr, obj, testData, 5000);

	/*
	 * Cancel the transaction.
	 * Since we modified the SDR, this should trigger:
	 * - "Attempting transaction reversal..." message
	 * - Transaction reversal process
	 * - ionrestart execution (if restartCmd configured)
	 */
	putErrmsg("sdr_test_util: Cancelling transaction with modifications.",
			NULL);
	putErrmsg("sdr_test_util: Transaction aborted.", NULL);
	sdr_cancel_xn(sdr);

	putErrmsg("sdr_test_util: Transaction cancelled successfully.", NULL);

	/* Give ionrestart time to execute if triggered */
	sleep(3);

	return 0;
}

static int	testNoReversal(Sdr sdr)
{
	Object		obj;
	char		buffer[100];

	putErrmsg("sdr_test_util: Beginning transaction without modifications.",
			NULL);

	/* Begin transaction */
	if (sdr_begin_xn(sdr) < 0)
	{
		putErrmsg("sdr_test_util: Can't begin transaction.", NULL);
		return -1;
	}

	/*
	 * Perform read-only operations.
	 * Get the IonVdb object (read only, doesn't modify SDR).
	 */
	obj = getIonDbObject();
	if (obj)
	{
		/* Just read some data, don't write anything */
		sdr_read(sdr, buffer, obj, 100);
	}

	/*
	 * Cancel the transaction.
	 * Since we did NOT modify the SDR, this should:
	 * - Log "Transaction reversal not necessary." message
	 * - NOT trigger ionrestart
	 * - System continues normally
	 */
	putErrmsg("sdr_test_util: Cancelling transaction without modifications.",
			NULL);
	putErrmsg("sdr_test_util: Transaction aborted.", NULL);
	sdr_cancel_xn(sdr);

	putErrmsg("sdr_test_util: Transaction cancelled successfully.", NULL);

	return 0;
}

static int	verifyState(Sdr sdr)
{
	Object		obj;
	char		buffer[100];

	putErrmsg("sdr_test_util: Verifying SDR state.", NULL);

	/* Try to begin and end a simple transaction */
	if (sdr_begin_xn(sdr) < 0)
	{
		putErrmsg("sdr_test_util: FAILED - Can't begin transaction.", NULL);
		return -1;
	}

	/* Try a simple read operation */
	obj = getIonDbObject();
	if (obj)
	{
		sdr_read(sdr, buffer, obj, 100);
	}

	/* End transaction normally */
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("sdr_test_util: FAILED - Can't end transaction.", NULL);
		return -1;
	}

	putErrmsg("sdr_test_util: PASSED - SDR state is consistent.", NULL);
	return 0;
}

#if defined (ION_LWT)
int	sdr_test_util(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	*modeStr = (char *) a1;
	char	*sdrName = (char *) a2;

	(void)a3; (void)a4; (void)a5; (void)a6;
	(void)a7; (void)a8; (void)a9; (void)a10;
	(void)sdrName;  /* Currently unused, see note below */
#else
int	main(int argc, char *argv[])
{
	char	*modeStr;
	char	*sdrName;
#endif
	TestMode	mode;
	Sdr		sdr;
	int		result;

#if !defined (ION_LWT)
	if (argc < 3)
	{
		printUsage();
		return 1;
	}

	modeStr = argv[1];
	sdrName = argv[2];
#endif

	/*
	 * Note: sdrName parameter is currently unused as we always use
	 * the default ION SDR via getIonsdr(). The parameter is kept
	 * for consistency with other ION utilities and potential future use.
	 */
	(void)sdrName;  /* Suppress unused variable warning */

	/* Parse mode */
	if (strcmp(modeStr, "test_reversal") == 0)
	{
		mode = MODE_TEST_REVERSAL;
	}
	else if (strcmp(modeStr, "test_no_reversal") == 0)
	{
		mode = MODE_TEST_NO_REVERSAL;
	}
	else if (strcmp(modeStr, "verify_state") == 0)
	{
		mode = MODE_VERIFY_STATE;
	}
	else
	{
		putErrmsg("sdr_test_util: Invalid mode.", modeStr);
		printUsage();
		return 1;
	}

	/* Attach to ION */
	if (ionAttach() < 0)
	{
		putErrmsg("sdr_test_util: Can't attach to ION.", NULL);
		return 1;
	}

	/* Get the SDR */
	sdr = getIonsdr();
	if (sdr == NULL)
	{
		putErrmsg("sdr_test_util: Can't get SDR.", NULL);
		ionDetach();
		return 1;
	}

	/* Execute the requested test mode */
	switch (mode)
	{
		case MODE_TEST_REVERSAL:
			result = testReversal(sdr);
			break;

		case MODE_TEST_NO_REVERSAL:
			result = testNoReversal(sdr);
			break;

		case MODE_VERIFY_STATE:
			result = verifyState(sdr);
			break;

		default:
			result = -1;
			break;
	}

	writeErrmsgMemos();
	ionDetach();

	if (result < 0)
	{
		return 1;
	}

	return 0;
}
