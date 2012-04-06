/*

	ionstart.c:	Helper code to start ION from C.

									*/

#include <bp.h>
#include <rfx.h>
#include <ltp.h>
#include "check.h"
#include "testutil.h"

void _xadmin(const char *xadmin, const char *path_prefix, const char *xrc)
{
 	char	cmdline[256];
	int     pid;
	int     status;

    if (path_prefix == NULL)
    {
        path_prefix = get_configs_path_prefix();
    }

 	if (xrc != NULL)
	{
		snprintf(cmdline, sizeof(cmdline), "%s %s%s", 
                        xadmin, path_prefix ? path_prefix : "", xrc);
		pid = pseudoshell(cmdline);
		fail_unless(pid != ERROR);
		fail_unless (-1 != waitpid(pid, &status, 0),
			"Failed to wait for admin pid.");
	}
}

void _ionstart(const char* path_prefix, const char *ionrc, 
    const char *ionsecrc, const char *ltprc, const char *bprc, 
    const char *ipnrc, const char *dtn2rc)
{
    _ionadmin(path_prefix, ionrc);
    _ionsecadmin(path_prefix, ionsecrc);
    _ltpadmin(path_prefix, ltprc);
    _bpadmin(path_prefix, bprc);
    _ipnadmin(path_prefix, ipnrc);
    _dtn2admin(path_prefix, dtn2rc);
}
