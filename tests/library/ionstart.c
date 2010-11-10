/*

	ionstart.c:	Helper code to start ION from C.

									*/

#include <bp.h>
#include <rfx.h>
#include <ltp.h>
#include "check.h"

static void _ionstart(const char* path_prefix, const char *ionrc, 
    const char *ionsecrc, const char *ltprc, const char *bprc, 
    const char *ipnrc, const char *dtn2rc);


void ionstart_default_config(const char *ionrc, const char *ionsecrc, 
        const char *ltprc, const char *bprc, 
        const char *ipnrc, const char *dtn2rc)
{
    char	pathbuf[256];
    const char * cfgroot = getenv("CONFIGSROOT");
    const char	* path_prefix = NULL;
    if(cfgroot) {
        snprintf(pathbuf, sizeof(pathbuf), "%s/", cfgroot);
        path_prefix = pathbuf;
    }

    _ionstart(path_prefix, ionrc, ionsecrc, ltprc, bprc, ipnrc, dtn2rc);
}

void ionstart(const char *ionrc, const char *ionsecrc, const char *ltprc,
		const char *bprc, const char *ipnrc, const char *dtn2rc)
{
    _ionstart(NULL, ionrc, ionsecrc, ltprc, bprc, ipnrc, dtn2rc);
}

static void _ionstart(const char* path_prefix, const char *ionrc, 
    const char *ionsecrc, const char *ltprc, const char *bprc, 
    const char *ipnrc, const char *dtn2rc)
{
	char	cmdline[256];
	int     pid;
	int     status;

        if ( path_prefix == NULL ) {
            path_prefix = "";
        }

	if (ionrc != NULL)
	{
		snprintf(cmdline, sizeof(cmdline), "ionadmin %s%s", 
                    path_prefix, ionrc);
		pid = pseudoshell(cmdline);
		fail_unless(pid != ERROR);
		fail_unless (-1 != waitpid(pid, &status, 0),
			"Failed to wait for ionadmin pid.");
	}

	if (ionsecrc != NULL)
	{
		snprintf(cmdline, sizeof(cmdline), "ionsecadmin %s%s", 
                        path_prefix, ionsecrc);
		pid = pseudoshell(cmdline);
		fail_unless(pid != ERROR);
		fail_unless (-1 != waitpid(pid, &status, 0),
			"Failed to wait for ionsecadmin pid.");
	}

	if (ltprc != NULL)
	{
		snprintf(cmdline, sizeof(cmdline), "ltpadmin %s%s", 
                        path_prefix, ltprc);
		pid = pseudoshell(cmdline);
		fail_unless(pid != ERROR);
		fail_unless (-1 != waitpid(pid, &status, 0),
			"Failed to wait for ltpadmin pid.");
	}

	if (bprc != NULL)
	{
		snprintf(cmdline, sizeof(cmdline), "bpadmin %s%s", 
                        path_prefix, bprc);
		pid = pseudoshell(cmdline);
		fail_unless(pid != ERROR);
		fail_unless (-1 != waitpid(pid, &status, 0),
			"Failed to wait for bpadmin pid.");
	}

	if (ipnrc != NULL)
	{
		snprintf(cmdline, sizeof(cmdline), "ipnadmin %s%s", 
                        path_prefix, ipnrc);
		pid = pseudoshell(cmdline);
		fail_unless(pid != ERROR);
		fail_unless (-1 != waitpid(pid, &status, 0),
			"Failed to wait for ipnadmin pid.");
	}

	if (dtn2rc != NULL)
	{
		snprintf(cmdline, sizeof(cmdline), "dtn2admin %s%s", 
                        path_prefix, dtn2rc);
		pid = pseudoshell(cmdline);
		fail_unless(pid != ERROR);
		fail_unless (-1 != waitpid(pid, &status, 0),
			"Failed to wait for dtn2admin pid.");
	}
}


