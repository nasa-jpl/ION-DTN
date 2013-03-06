/* Program to repeatedly initiate/terminate a transaction
 * and ignore any SIGINT's sent to it.
 * Samuel Jero <sj323707@ohio.edu>
 * Ohio University
 * March 5, 2013*/

#include <stdlib.h>
#include <stdio.h>
#include "bp.h"
#include "sdrxn.h"
#include "platform.h"
#include "platform_sm.h"

void sighandler(){
	isignal(SIGINT, sighandler);
	printf("Got SIGINT\n");
}

int main()
{

	if (bp_attach() < 0)
	{
		printf("Can't Attached to BP!\n");
		return 1;
	}

	isignal(SIGINT, sighandler);

	while(1){
		if(!sdr_begin_xn(bp_get_sdr()))
		{
			printf("Begin Transaction Failed\n");
			return 1;
		}
		sm_TaskYield();
		sdr_end_xn(bp_get_sdr());
		sm_TaskYield();
	}

	bp_detach();
	return 0;
}
