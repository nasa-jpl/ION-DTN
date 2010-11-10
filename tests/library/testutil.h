/*

	testutil.h:	Helpful utility code for tests in C.

									*/

#include <bp.h>

/*
 * Same as ionstart(...), but prefixes all paths with value of CONFIGSDIR
 * from environment
 */
void ionstart_default_config(const char *ionrc, const char *ionsecrc, const
        char *ltprc, const char *bprc, const char *ipnrc, const char *dtn2rc);

void ionstart(const char *ionrc, const char *ionsecrc, const char *ltprc,
		const char *bprc, const char *ipnrc, const char *dtn2rc);

void ionstop();
