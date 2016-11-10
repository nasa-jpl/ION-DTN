/*

	ionstop.c:	Helper code to stop ION from C.

									*/

#include <bp.h>
#include "check.h"

void ionstop()
{
	int pid;

	pid = pseudoshell("bpadmin .");
	fail_unless(pid != ERROR);
	sleep(1);

	pid = pseudoshell("ltpadmin .");
	fail_unless(pid != ERROR);
	sleep(1);

	pid = pseudoshell("ionadmin .");
	fail_unless(pid != ERROR);
	sleep(1);

#if ! defined (VXWORKS) && ! defined (RTEMS)
	pid = pseudoshell("killm");
	fail_unless(pid != ERROR);
#endif
}
