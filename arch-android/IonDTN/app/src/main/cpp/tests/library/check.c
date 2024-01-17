/*
 * check.c
 * Like libcheck (check.sf.net), except it doesn't leak memory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include "check.h"

static int current_failures = 0;
static int current_success  = 0;
static int current_skips    = 0;
int failure_mode     = ABORT_ON_FAIL;

void _fail_unless(int result, const char *file,
    int line, const char *expr, ...)
{
    const char *msg;

    if(!result) {
        va_list ap;

	if (errno)
	{
		perror("ION encountered system error");
	}

        va_start(ap, expr);
        msg = (const char *)va_arg(ap, char *);
        if (msg == NULL) msg = expr;
        vfprintf(stderr, msg, ap);
        va_end(ap);
        fprintf(stderr, "\n");

        if(failure_mode == SKIP_ON_FAIL) {
            fprintf(stderr, "\t...Skipping\n");
            ++current_skips;
        } else {
            ++current_failures;
        }
        if(failure_mode == ABORT_ON_FAIL) {
            fprintf(stderr, "%d PASS, %d SKIP, %d FAIL (%d TOTAL)\n",
                current_success, current_skips, current_failures,
                current_success + current_skips + current_failures);
            abort();
        }
    } else {
        ++current_success;
    }
}

int check_summary(const char *name)
{
    fprintf(stderr, "%s: %d PASS, %d SKIP, %d FAIL (%d TOTAL)\n",
        name, 
        current_success, current_skips, current_failures,
        current_success + current_skips + current_failures);
    if(current_failures == 0) return 0;
    return 1;
}
