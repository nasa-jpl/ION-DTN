/*

	ionstop.c:	Helper code to stop ION from C.

									*/

#include <bp.h>
#include "check.h"

int synch_killm(int maxseconds)
{
	int pid = pseudoshell("killm");
	if (pid == ERROR)
	{
		return 0;
	}
	while (sm_TaskExists(pid))
	{
		if (--maxseconds < 1)
		{
			sm_TaskDelete(pid);
			return 0;
		}
		sm_TaskDelay(1);
	}
	return 1;
}

void ionstop(void)
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
	synch_killm(60);
#endif
}
