/*

	paths.c:	Helper code to map from test suite paths to 
				config/executable paths.

									*/
#include <ionsec.h>
#include "check.h"
#include "testutil.h"

const char *get_configs_path_prefix()
{
	static char path_prefix[256] = "";
    const char * cfgroot = getenv("CONFIGSROOT");
	
    if(cfgroot) {
        fail_unless(snprintf(path_prefix, sizeof(path_prefix), "%s/", cfgroot)
			 < sizeof(path_prefix));
    }
	return path_prefix;
}

int sec_addKey_default_config(char *keyName, char *fileName)
{
	char prefixedFileName[PATHLENMAX];

	fail_unless(snprintf(prefixedFileName, PATHLENMAX, "%s%s",
					get_configs_path_prefix(), fileName) < PATHLENMAX);

	return sec_addKey(keyName, prefixedFileName);
}
