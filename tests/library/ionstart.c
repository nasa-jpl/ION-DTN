/*

	ionstart.c:	Helper code to start ION from C.

									*/

#include <bp.h>
#include <rfx.h>
#include <ltp.h>
#include "check.h"

void ionstart(const char *ionrc, const char *ionsecrc, const char *ltprc,
		const char *bprc, const char *ipnrc, const char *dtn2rc)
{
	char	cmdline[256];
	int     pid;
	int     status;
	if (ionrc != NULL)
	{
		snprintf(cmdline, sizeof(cmdline), "ionadmin %s", ionrc);
		pid = pseudoshell(cmdline);
		fail_unless(pid != ERROR);
		fail_unless (-1 != waitpid(pid, &status, 0), "Failed to wait for ionadmin pid.");
	}

	if (ionsecrc != NULL)
	{
		snprintf(cmdline, sizeof(cmdline), "ionsecadmin %s", ionsecrc);
		pid = pseudoshell(cmdline);
		fail_unless(pid != ERROR);
		fail_unless (-1 != waitpid(pid, &status, 0), "Failed to wait for ionsecadmin pid.");
	}

	if (ltprc != NULL)
	{
		snprintf(cmdline, sizeof(cmdline), "ltpadmin %s", ltprc);
		pid = pseudoshell(cmdline);
		fail_unless(pid != ERROR);
		fail_unless (-1 != waitpid(pid, &status, 0), "Failed to wait for ltpadmin pid.");
	}

	if (bprc != NULL)
	{
		snprintf(cmdline, sizeof(cmdline), "bpadmin %s", bprc);
		pid = pseudoshell(cmdline);
		fail_unless(pid != ERROR);
		fail_unless (-1 != waitpid(pid, &status, 0), "Failed to wait for bpadmin pid.");
	}

	if (ipnrc != NULL)
	{
		snprintf(cmdline, sizeof(cmdline), "ipnadmin %s", ipnrc);
		pid = pseudoshell(cmdline);
		fail_unless(pid != ERROR);
		fail_unless (-1 != waitpid(pid, &status, 0), "Failed to wait for ipnadmin pid.");
	}

	if (dtn2rc != NULL)
	{
		snprintf(cmdline, sizeof(cmdline), "dtn2admin %s", dtn2rc);
		pid = pseudoshell(cmdline);
		fail_unless(pid != ERROR);
		fail_unless (-1 != waitpid(pid, &status, 0), "Failed to wait for dtn2admin pid.");
	}
}


