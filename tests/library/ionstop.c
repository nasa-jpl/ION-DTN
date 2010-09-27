/*

	ionstop.c:	Helper code to stop ION from C.

									*/

#include <bp.h>
#include "check.h"

void ionstop()
{
#if ! defined (VXWORKS) && ! defined (RTEMS)
	int killm_pid;
#endif

	fail_unless(pseudoshell("bpadmin .") != ERROR);
	fail_unless(pseudoshell("ltpadmin .") != ERROR);
	fail_unless(pseudoshell("ionadmin .") != ERROR);
#if ! defined (VXWORKS) && ! defined (RTEMS)
	killm_pid = pseudoshell("killm");
	fail_unless(killm_pid != ERROR);
#endif
}

