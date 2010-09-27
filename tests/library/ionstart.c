/*

	ionstart.c:	Helper code to start ION from C.

									*/

#include <bp.h>
#include "check.h"

void ionstart(const char *ionrc, const char *ionsecrc, const char *ltprc,
		const char *bprc, const char *ipnrc, const char *dtn2rc)
{
	char	cmdline[256];

	if (ionrc != NULL)
	{
		snprintf(cmdline, sizeof(cmdline), "ionadmin %s", ionrc);
		fail_unless(pseudoshell(cmdline) != ERROR);
		sleep(1);
	}

	if (ionsecrc != NULL)
	{
		snprintf(cmdline, sizeof(cmdline), "ionsecadmin %s", ionsecrc);
		fail_unless(pseudoshell(cmdline) != ERROR);
		sleep(1);
	}

	if (ltprc != NULL)
	{
		snprintf(cmdline, sizeof(cmdline), "ltpadmin %s", ltprc);
		fail_unless(pseudoshell(cmdline) != ERROR);
		sleep(1);
	}

	if (bprc != NULL)
	{
		snprintf(cmdline, sizeof(cmdline), "bpadmin %s", bprc);
		fail_unless(pseudoshell(cmdline) != ERROR);
		sleep(1);
	}

	if (ipnrc != NULL)
	{
		snprintf(cmdline, sizeof(cmdline), "ipnadmin %s", ipnrc);
		fail_unless(pseudoshell(cmdline) != ERROR);
		sleep(1);
	}

	if (dtn2rc != NULL)
	{
		snprintf(cmdline, sizeof(cmdline), "dtn2admin %s", dtn2rc);
		fail_unless(pseudoshell(cmdline) != ERROR);
		sleep(1);
	}
}


