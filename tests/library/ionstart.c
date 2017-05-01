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
	}
}

void _ionstart(const char* path_prefix, const char *ionrc, 
    const char *ionsecrc, const char *ltprc, const char *bprc, 
    const char *ipnrc, const char *dtn2rc)
{
    _ionadmin(path_prefix, ionrc);
    sleep(2);
    _ionsecadmin(path_prefix, ionsecrc);
    sleep(2);
    _ltpadmin(path_prefix, ltprc);
    sleep(2);
    _bpadmin(path_prefix, bprc);
    sleep(2);
    _ipnadmin(path_prefix, ipnrc);
    sleep(2);
    _dtn2admin(path_prefix, dtn2rc);
}
