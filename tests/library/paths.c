/*

	paths.c:	Helper code to map from test suite paths to
				config/executable paths.

									*/
#include <ionsec.h>
#include "check.h"
#include "testutil.h"

const char *get_configs_path_prefix(void)
{
	static char path_prefix[256] = "";
	const char * cfgroot = getenv("CONFIGSROOT");

	if(cfgroot) {
		int neededSize;
		neededSize = snprintf(path_prefix, sizeof(path_prefix), "%s/", cfgroot);

		/*
		 * Check for snprintf errors. A return value < 0 is an error.
		 * A return value >= sizeof(buffer) means the output was truncated.
		 */
		fail_unless(neededSize >= 0 && (size_t)neededSize < sizeof(path_prefix),
					"snprintf failed or truncated the config path prefix");
	}
	return path_prefix;
}

int sec_addKey_default_config(char *keyName, char *fileName)
{
	char prefixedFileName[PATHLENMAX];

	fail_unless(snprintf(prefixedFileName, PATHLENMAX, "%s%s",
					get_configs_path_prefix(), fileName) < PATHLENMAX);

	return sec_addKey(keyName, prefixedFileName, 0);
}
