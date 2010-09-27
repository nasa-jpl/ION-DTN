/*

	testutil.h:	Helpful utility code for tests in C.

									*/

#include <bp.h>

void ionstart(const char *ionrc, const char *ionsecrc, const char *ltprc,
		const char *bprc, const char *ipnrc, const char *dtn2rc);

void ionstop();
