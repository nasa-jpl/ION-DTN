/*
 *
 * 	file2udp.h:	definitions used for the file2tcp benchmark
 * 			system.
 *
 * 									*/

#include "platform.h"

#define	TEST_PORT_NBR	2101
#define EOF_LINE_TEXT	"*** End of the file ***"

/*
 * Structure to contain everything needed for RTT timing.
 * One of these required per socket being timed.
 * The caller allocates this structure, then passes its address to
 * all the rtt_XXX() functions.
 */

struct rtt_struct {
  float	rtt_rtt;	/* most recent round-trip time (RTT), seconds */
  float	rtt_srtt;	/* smoothed round-trip time (SRTT), seconds */
  float	rtt_rttdev;	/* smoothed mean deviation, seconds */
  short	rtt_nrexmt;	/* #times retransmitted: 0, 1, 2, ... */
  short	rtt_currto;	/* current retransmit timeout (RTO), seconds */
  short	rtt_nxtrto;	/* retransmit timeout for next packet, if nonzero */
  struct timeval	time_start;	/* for elapsed time */
  struct timeval	time_stop;	/* for elapsed time */
};
#if 0
#define	RTT_RXTMIN      2	/* min retransmit timeout value, seconds */
#define	RTT_RXTMAX    120	/* max retransmit timeout value, seconds */
#endif

#define	RTT_RXTMIN      1	/* min retransmit timeout value, seconds */
#define	RTT_RXTMAX	3	/* max retransmit timeout value, seconds */
#define	RTT_MAXNREXMT 	4	/* max #times to retransmit: must also
				   change exp_backoff[] if this changes */
