/*

	issue-260-teach-valgrind-mtake/domtake.c:	MTAKE-in-valgrind test.
	
	This test does some MTAKEing and MRELEASEing; you should run it from
	inside of valgrind and observe some leaks.

									*/

#include <ion.h>
#include "check.h"
#include "testutil.h"

int main(int argc, char **argv)
{
	char *stringToLeak = NULL;
	char *stringToNotLeak = NULL;

    ionAttach();

	stringToLeak = MTAKE(128);
	fail_unless(stringToLeak != NULL);
	istrcpy(stringToLeak, "This is a string we will leak on purpose.", 128);

	stringToNotLeak = MTAKE(129);
	fail_unless(stringToNotLeak != NULL);
	istrcpy(stringToNotLeak, "This is a string we won't leak.", 129);
	MRELEASE(stringToNotLeak);

	CHECK_FINISH;
}
