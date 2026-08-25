/*
 * check.h
 * Like libcheck (check.sf.net), except it doesn't leak memory.
 */

#include <stdlib.h>
#include <stdio.h>

/*
 * C99-compliant, pedantic-safe macro overloading for fail_unless.
 * This supports both fail_unless(expr) and fail_unless(expr, "message", ...).
 */

/* Helper for fail_unless(expr) - provides a default message. */
#define fail_unless__1(expr) \
	_fail_unless((expr), __FILE__, __LINE__, "Failure '"#expr"' occurred", NULL)

/* Helper for fail_unless(expr, ...) - passes the custom message. */
#define fail_unless__N(expr, ...) \
	_fail_unless((expr), __FILE__, __LINE__, __VA_ARGS__, NULL)

/* Argument-counting dispatcher macro. */
#define fail_unless__GET_MACRO(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, NAME, ...) NAME

/* The main fail_unless macro. */
#define fail_unless(...) \
	fail_unless__GET_MACRO(__VA_ARGS__, \
			fail_unless__N, fail_unless__N, fail_unless__N, fail_unless__N, \
			fail_unless__N, fail_unless__N, fail_unless__N, fail_unless__N, \
			fail_unless__N, fail_unless__1, 0) \
	(__VA_ARGS__)

#define ABORT_ON_FAIL      0
#define CONTINUE_ON_FAIL   1
#define SKIP_ON_FAIL       2
extern int failure_mode;

void _fail_unless(int result, const char *file, int line, const char *fmt, ...);

int check_summary(const char *name);
