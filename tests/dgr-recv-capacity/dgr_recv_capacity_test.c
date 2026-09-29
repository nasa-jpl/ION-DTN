/*
 * dgr_recv_capacity_test.c -- regression for the DGR delivery-side buffer
 * over-write.
 *
 * dgr_receive() copies a received record's content into the caller's buffer.
 * Historically it trusted the caller to supply a buffer of at least 65535
 * bytes and copied the full content length regardless, so a valid record
 * larger than a smaller caller buffer overran that buffer (an unauthenticated
 * remote memory-safety defect; see the DGR service-data-length advisories).
 *
 * The fix gives dgr_receive() an explicit destination-capacity argument and
 * discards any record whose content exceeds it, rather than copying past the
 * buffer.  This harness exercises the real dgr_receive() over a loopback DGR
 * exchange:
 *
 *   Case 1: a valid record larger than the caller's buffer must NOT be copied
 *           into that buffer.  The buffer is followed by a guard region filled
 *           with a known byte; on vulnerable code the over-write clobbers the
 *           guard (detected deterministically, no AddressSanitizer required).
 *   Case 2: a record that fits is delivered intact.
 *
 * The test SKIPs (exit 2) when a DGR loopback SAP cannot be established here.
 */

#include <ion.h>
#include <platform.h>
#include <dgr.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define	CAP		(256)		/*	Caller's buffer size.	*/
#define	GUARD		(1024)		/*	Trailing canary region.	*/
#define	GUARD_BYTE	(0xAA)
#define	BIG_LEN		(1000)		/*	> CAP, < 65535.		*/
#define	SMALL_LEN	(100)		/*	< CAP.			*/
#define	RECV_TIMEOUT	(5)		/*	Seconds.		*/

static int	guardIntact(const unsigned char *region)
{
	int	i;

	for (i = CAP; i < CAP + GUARD; i++)
	{
		if (region[i] != GUARD_BYTE)
		{
			return 0;
		}
	}

	return 1;
}

int	main(void)
{
	unsigned int	ownIp;
	Dgr		rx;
	Dgr		tx;
	DgrRC		rc;
	unsigned short	rxPort;
	unsigned int	rxIp;
	unsigned char	*region;
	char		*bigPayload;
	char		smallPayload[SMALL_LEN];
	unsigned short	fromPort;
	unsigned int	fromIp;
	int		length;
	int		errnbr;
	int		i;

	ownIp = getAddressOfHost();
	if (ownIp == 0)
	{
		printf("SKIP: no host IP address available for DGR loopback.\n");
		return 2;
	}

	if (dgr_open(1, 2, 0, ownIp, NULL, &rx, &rc) < 0 || rc != DgrOpened)
	{
		printf("SKIP: cannot open DGR receive access point.\n");
		return 2;
	}

	if (dgr_open(2, 2, 0, ownIp, NULL, &tx, &rc) < 0 || rc != DgrOpened)
	{
		printf("SKIP: cannot open DGR send access point.\n");
		dgr_close(rx);
		return 2;
	}

	dgr_getsockname(rx, &rxPort, &rxIp);

	region = malloc(CAP + GUARD);
	bigPayload = malloc(BIG_LEN);
	if (region == NULL || bigPayload == NULL)
	{
		printf("SKIP: out of memory.\n");
		dgr_close(tx);
		dgr_close(rx);
		return 2;
	}

	memset(bigPayload, 'B', BIG_LEN);
	memset(smallPayload, 'S', SMALL_LEN);

	/*	Case 1: oversized valid record must not overrun the buffer.  */

	memset(region, GUARD_BYTE, CAP + GUARD);
	if (dgr_send(tx, rxPort, rxIp, DGR_NOTE_NONE, bigPayload, BIG_LEN, &rc)
			< 0)
	{
		printf("SKIP: dgr_send of the oversized record failed.\n");
		free(bigPayload);
		free(region);
		dgr_close(tx);
		dgr_close(rx);
		return 2;
	}

	rc = DgrFailed;
	length = 0;
	errnbr = 0;
	if (dgr_receive(rx, &fromPort, &fromIp, (char *) region, CAP, &length,
			&errnbr, RECV_TIMEOUT, &rc) < 0)
	{
		printf("FAIL: dgr_receive failed on the oversized case.\n");
		return 1;
	}

	if (!guardIntact(region))
	{
		printf("FAIL: dgr_receive wrote past a %d-byte buffer "
				"(delivery buffer overflow).\n", CAP);
		return 1;
	}

	if (rc == DgrDatagramReceived)
	{
		printf("FAIL: a %d-byte record was delivered into a %d-byte "
				"buffer (length=%d).\n", BIG_LEN, CAP, length);
		return 1;
	}

	/*	Case 2: a record that fits is delivered intact.		*/

	memset(region, GUARD_BYTE, CAP + GUARD);
	if (dgr_send(tx, rxPort, rxIp, DGR_NOTE_NONE, smallPayload, SMALL_LEN,
			&rc) < 0)
	{
		printf("SKIP: dgr_send of the fitting record failed.\n");
		free(bigPayload);
		free(region);
		dgr_close(tx);
		dgr_close(rx);
		return 2;
	}

	rc = DgrFailed;
	length = 0;
	errnbr = 0;
	if (dgr_receive(rx, &fromPort, &fromIp, (char *) region, CAP, &length,
			&errnbr, RECV_TIMEOUT, &rc) < 0)
	{
		printf("FAIL: dgr_receive failed on the fitting case.\n");
		return 1;
	}

	if (rc != DgrDatagramReceived || length != SMALL_LEN)
	{
		printf("FAIL: fitting record not delivered (rc=%d, length=%d).\n",
				(int) rc, length);
		return 1;
	}

	for (i = 0; i < SMALL_LEN; i++)
	{
		if (region[i] != 'S')
		{
			printf("FAIL: delivered content does not match.\n");
			return 1;
		}
	}

	if (!guardIntact(region))
	{
		printf("FAIL: dgr_receive wrote past the buffer on a fitting "
				"record.\n");
		return 1;
	}

	free(bigPayload);
	free(region);
	dgr_close(tx);
	dgr_close(rx);

	printf("PASS: dgr_receive bounds delivery to the caller's buffer.\n");
	return 0;
}
