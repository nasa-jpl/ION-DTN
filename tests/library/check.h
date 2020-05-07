/*
 * check.h
 * Like libcheck (check.sf.net), except it doesn't leak memory.
 */

#include <stdlib.h>
#include <stdio.h>

#ifdef SOLARIS_COMPILER

#define fail_unless(expr) \
    _fail_unless((expr), __FILE__, __LINE__, \
    "Failure '"#expr"' occurred")

#else

#define fail_unless(expr, ...) \
    _fail_unless((expr), __FILE__, __LINE__, \
    "Failure '"#expr"' occurred", ## __VA_ARGS__, NULL)

#endif

#define CHECK_FINISH \
    return check_summary(argv[0])

#define ABORT_ON_FAIL      0
#define CONTINUE_ON_FAIL   1
#define SKIP_ON_FAIL       2
extern int failure_mode;

void _fail_unless(int result, const char *file, int line, const char *fmt, ...);

int check_summary(const char *name);
