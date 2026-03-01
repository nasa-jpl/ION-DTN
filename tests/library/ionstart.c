/*

	ionstart.c:	Helper code to start ION from C.

*/

#include <bp.h>
#include <rfx.h>
#include <ltp.h>
#include <unistd.h>
#include <libgen.h>
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
	char	saved_cwd[512];
	char	config_dir[512];
	char	ionrc_path[512];
	char	rc_copy[256];
	char	*dir;

	/*	Save current working directory.				*/
	if (getcwd(saved_cwd, sizeof(saved_cwd)) == NULL)
	{
		saved_cwd[0] = '\0';
	}

	/*	Change to config directory so ionconfig paths resolve
	 *	correctly (ionconfig is referenced by filename only
	 *	in ionrc files).					*/
	if (ionrc != NULL)
	{
		if (path_prefix == NULL)
		{
			path_prefix = get_configs_path_prefix();
		}

		snprintf(ionrc_path, sizeof(ionrc_path), "%s%s",
				path_prefix ? path_prefix : "", ionrc);
		snprintf(config_dir, sizeof(config_dir), "%s", ionrc_path);
		dir = dirname(config_dir);
		if (dir != NULL && chdir(dir) == 0)
		{
			/*	Now run admin programs with just
			 *	filenames since we're in config dir.	*/
			snprintf(rc_copy, sizeof(rc_copy), "%s", ionrc);
			_ionadmin("", basename(rc_copy));
			sleep(2);
			if (ionsecrc != NULL)
			{
				snprintf(rc_copy, sizeof(rc_copy), "%s", ionsecrc);
				_ionsecadmin("", basename(rc_copy));
			}
			sleep(2);
			if (ltprc != NULL)
			{
				snprintf(rc_copy, sizeof(rc_copy), "%s", ltprc);
				_ltpadmin("", basename(rc_copy));
			}
			sleep(2);
			if (bprc != NULL)
			{
				snprintf(rc_copy, sizeof(rc_copy), "%s", bprc);
				_bpadmin("", basename(rc_copy));
			}
			sleep(2);
			if (ipnrc != NULL)
			{
				snprintf(rc_copy, sizeof(rc_copy), "%s", ipnrc);
				_ipnadmin("", basename(rc_copy));
			}
			sleep(2);
			if (dtn2rc != NULL)
			{
				snprintf(rc_copy, sizeof(rc_copy), "%s", dtn2rc);
				_dtn2admin("", basename(rc_copy));
			}

			/*	Restore original working directory.	*/
			if (saved_cwd[0] != '\0')
			{
				if (chdir(saved_cwd) < 0)
				{
					perror("chdir restore failed");
				}
			}

			return;
		}
	}

	/*	Fallback to original behavior if chdir fails.		*/
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
