/*

	ionstop.c:	Helper code to stop ION from C.

									*/

#include <bp.h>
#include "check.h"

void ionstop()
{
	int pid;
	int status;

	pid = pseudoshell("bpadmin .");
	fail_unless(pid != ERROR);
	fail_unless(-1 != waitpid(pid, &status, 0),
		"Failed to wait for bpadmin to stop.");

	pid = pseudoshell("ltpadmin .");
	fail_unless(pid != ERROR);
	fail_unless(-1 != waitpid(pid, &status, 0),
		"Failed to wait for ltpadmin to stop.");

	pid = pseudoshell("ionadmin .");
	fail_unless(pid != ERROR);
	fail_unless(-1 != waitpid(pid, &status, 0), 
		"Failed to wait for ionadmin to stop.");

#if ! defined (VXWORKS) && ! defined (RTEMS)
	pid = pseudoshell("killm");
	fail_unless(pid != ERROR);
	fail_unless(-1 != waitpid(pid, &status, 0),
		"Failed to wait for killm to finish");
#endif
}
