/*
 *  ION runtime image initialization and driver.
 */

#include <bsp.h>
#include <rtems.h>
#include <rtems/rtems_bsdnet.h>
#include <rtems/error.h>
#include <rtems/shell.h>
#include "platform.h"
#include "ion.h"
#include "rfx.h"
#include "ltp.h"
#include "bp.h"

/*	When CFDP is included in the build, the test fails due to
 *	insufficient memory resources.  Need to figure out how to
 *	get RTEMS to allocate enough memory for the test, but for
 *	now we just exclude CFDP from the build (since it is not
 *	involved in sending the test bundle).				*/

#ifndef NASA_PROTECTED_FLIGHT_CODE
#include "cfdp.h"
#endif

#define	ION_NODE_NBR	19

static void	createIonConfigFiles()
{
	uvast	nodenbr = ION_NODE_NBR;
	char	filenamebuf[80];
	int	fd;
	char	*ionconfigLines[] =	{
"wmSize 100000\n",
"configFlags 1\n",
"heapWords 75000\n",
"pathName /ion\n",
					};
	int	ionconfigLineCount = sizeof ionconfigLines / sizeof (char *);
	char	*globalLines[] =	{
"a contact +0 +7200 19 19 100000\n",
"a range +0 +7200 19 19 0\n"
					};
	int	globalLineCount = sizeof globalLines / sizeof (char *);
	char	*ionsecrcLines[] =	{
"1\n",
"a bspbabrule ipn:19.* ipn:19.* '' ''\n"
					};
	int	ionsecrcLineCount = sizeof ionsecrcLines / sizeof (char *);
	char	*ltprcLines[] =		{
"1 20 64000\n",
"a span 19 4 4000 4 4000 1084 2048 1 'pmqlso /ionpmq.19'\n",
"w 1\n",
"s 'pmqlsi /ionpmq.19'\n"
					};
	int	ltprcLineCount = sizeof ltprcLines / sizeof (char *);
	char	*bprcLines[] =		{
"1\n",
"a scheme ipn 'ipnfw' 'ipnadminep'\n",
"a endpoint ipn:19.0 x\n",
"a endpoint ipn:19.1 x\n",
"a endpoint ipn:19.2 x\n",
"a endpoint ipn:19.64 x\n",
"a endpoint ipn:19.65 x\n",
"a endpoint ipn:19.126 x\n",
"a endpoint ipn:19.127 x\n",
"a protocol ltp 1400 100\n",
"a induct ltp 19 ltpcli\n",
"a outduct ltp 19 ltpclo\n",
"w 1\n"
					};
	int	bprcLineCount = sizeof bprcLines / sizeof (char *);
	char	*ipnrcLines[] =		{
"a plan 19 ltp/19\n"
					};
	int	ipnrcLineCount = sizeof ipnrcLines / sizeof (char *);
	char	linebuf[255];
	char	**line;
	int	i;

	/*	Keep all ION configuration files in one directory.	*/

	if (mkdir("/ion", 0777) < 0)
	{
		perror("Can't create directory for config files");
		return;
	}

	/*	Create ionconfig file.					*/

	isprintf(filenamebuf, sizeof filenamebuf, "/ion/node" UVAST_FIELDSPEC
			".ionconfig", nodenbr);
	fd = iopen(filenamebuf, O_WRONLY | O_CREAT | O_TRUNC, 0777);
	if (fd < 0)
	{
		printf("Can't create .ionconfig file '%s'.\n", filenamebuf);
		return;
	}

	for (i = 0, line = ionconfigLines; i < ionconfigLineCount; line++, i++)
	{
		oK(iputs(fd, *line));
	}

	close(fd);

	/*	Create ionrc file.					*/

	isprintf(filenamebuf, sizeof filenamebuf, "/ion/node" UVAST_FIELDSPEC
			".ionrc", nodenbr);
	fd = iopen(filenamebuf, O_WRONLY | O_CREAT | O_TRUNC, 0777);
	if (fd < 0)
	{
		printf("Can't create .ionrc file '%s'.\n", filenamebuf);
		return;
	}

	isprintf(linebuf, sizeof linebuf, "1 " UVAST_FIELDSPEC " /ion/node"
			UVAST_FIELDSPEC ".ionconfig\ns\n", nodenbr, nodenbr);
	oK(iputs(fd, linebuf));
	close(fd);

	/*	Create global.ionrc file.				*/

	istrcpy(filenamebuf, "/ion/global.ionrc", sizeof filenamebuf);
	fd = iopen(filenamebuf, O_WRONLY | O_CREAT | O_TRUNC, 0777);
	if (fd < 0)
	{
		printf("Can't create global.ionrc file '%s'.\n", filenamebuf);
		return;
	}

	for (i = 0, line = globalLines; i < globalLineCount; line++, i++)
	{
		oK(iputs(fd, *line));
	}

	close(fd);

	/*	Create ionsecrc file.					*/

	isprintf(filenamebuf, sizeof filenamebuf, "/ion/node" UVAST_FIELDSPEC
			".ionsecrc", nodenbr);
	fd = iopen(filenamebuf, O_WRONLY | O_CREAT | O_TRUNC, 0777);
	if (fd < 0)
	{
		printf("Can't create .ionsecrc file '%s'.\n", filenamebuf);
		return;
	}

	for (i = 0, line = ionsecrcLines; i < ionsecrcLineCount; line++, i++)
	{
		oK(iputs(fd, *line));
	}

	close(fd);

	/*	Create ltprc file.					*/

	isprintf(filenamebuf, sizeof filenamebuf, "/ion/node" UVAST_FIELDSPEC
			".ltprc", nodenbr);
	fd = iopen(filenamebuf, O_WRONLY | O_CREAT | O_TRUNC, 0777);
	if (fd < 0)
	{
		printf("Can't create .ltprc file '%s'.\n", filenamebuf);
		return;
	}

	for (i = 0, line = ltprcLines; i < ltprcLineCount; line++, i++)
	{
		oK(iputs(fd, *line));
	}

	close(fd);

	/*	Create ipnrc file.					*/

	isprintf(filenamebuf, sizeof filenamebuf, "/ion/node" UVAST_FIELDSPEC
			".ipnrc", nodenbr);
	fd = iopen(filenamebuf, O_WRONLY | O_CREAT | O_TRUNC, 0777);
	if (fd < 0)
	{
		printf("Can't create .ipnrc file '%s'.\n", filenamebuf);
		return;
	}

	for (i = 0, line = ipnrcLines; i < ipnrcLineCount; line++, i++)
	{
		oK(iputs(fd, *line));
	}

	close(fd);

	/*	Create bprc file.					*/

	isprintf(filenamebuf, sizeof filenamebuf, "/ion/node" UVAST_FIELDSPEC
			".bprc", nodenbr);
	fd = iopen(filenamebuf, O_WRONLY | O_CREAT | O_TRUNC, 0777);
	if (fd < 0)
	{
		printf("Can't create .bprc file '%s'.\n", filenamebuf);
		return;
	}

	for (i = 0, line = bprcLines; i < bprcLineCount; line++, i++)
	{
		oK(iputs(fd, *line));
	}

	isprintf(linebuf, sizeof linebuf, "r 'ipnadmin /ion/node"
			UVAST_FIELDSPEC ".ipnrc'\ns\n", nodenbr);
	oK(iputs(fd, linebuf));
	close(fd);

#ifndef NASA_PROTECTED_FLIGHT_CODE
	/*	Create cfdprc file.					*/

	isprintf(filenamebuf, sizeof filenamebuf, "/ion/node" UVAST_FIELDSPEC
			".cfdprc", nodenbr);
	fd = iopen(filenamebuf, O_WRONLY | O_CREAT | O_TRUNC, 0777);
	if (fd < 0)
	{
		printf("Can't create .cfdprc file '%s'.\n", filenamebuf);
		return;
	}

	oK(iputs(fd, "1\ns bputa\n"));
	close(fd);
#endif
}

static int	startDTN()
{
	uvast	nodenbr = ION_NODE_NBR;
	char	cmd[80];
	int	count;

	sm_ipc_init();
	isprintf(cmd, sizeof cmd, "ionadmin /ion/node" UVAST_FIELDSPEC
			".ionrc", nodenbr);
	pseudoshell(cmd);
	count = 5;
	while (rfx_system_is_started() == 0)
	{
		snooze(1);
		count--;
		if (count == 0)
		{
			writeMemo("[?] RFX start hung up, abandoned.");
			return -1;
		}
	}

	pseudoshell("ionadmin /ion/global.ionrc");
	snooze(1);
	isprintf(cmd, sizeof cmd, "ionsecadmin /ion/node" UVAST_FIELDSPEC
			".ionsecrc", nodenbr);
	pseudoshell(cmd);
	snooze(1);

	/*	Now start the higher layers of the DTN stack.		*/

	isprintf(cmd, sizeof cmd, "ltpadmin /ion/node" UVAST_FIELDSPEC
			".ltprc", nodenbr);
	pseudoshell(cmd);
	count = 5;
	while (ltp_engine_is_started() == 0)
	{
		snooze(1);
		count--;
		if (count == 0)
		{
			writeMemo("[?] LTP start hung up, abandoned.");
			return -1;
		}
	}

	isprintf(cmd, sizeof cmd, "bpadmin /ion/node" UVAST_FIELDSPEC
			".bprc", nodenbr);
	pseudoshell(cmd);
	count = 5;
	while (bp_agent_is_started() == 0)
	{
		snooze(1);
		count--;
		if (count == 0)
		{
			writeMemo("[?] BP start hung up, abandoned.");
			return -1;
		}
	}

	isprintf(cmd, sizeof cmd, "lgagent ipn:" UVAST_FIELDSPEC ".127",
			nodenbr);
	pseudoshell(cmd);
	snooze(1);

#ifndef NASA_PROTECTED_FLIGHT_CODE
	/*	Now start CFDP.						*/

	isprintf(cmd, sizeof cmd, "cfdpadmin /ion/node" UVAST_FIELDSPEC
			".cfdprc", nodenbr);
	pseudoshell(cmd);
	count = 5;
	while (cfdp_entity_is_started() == 0)
	{
		snooze(1);
		count--;
		if (count == 0)
		{
			writeMemo("[?] CFDP start hung up, abandoned.");
			return -1;
		}
	}
#endif
	return 0;
}

static void	testLoopback()
{
	char	cmd[80];

	puts("Starting loopback test.");
	isprintf(cmd, sizeof cmd, "bpsink ipn:" UVAST_FIELDSPEC ".1",
			ION_NODE_NBR);
	pseudoshell(cmd);
	snooze(1);
	isprintf(cmd, sizeof cmd, "bpsource ipn:" UVAST_FIELDSPEC
			".1 'Hello, world.'", ION_NODE_NBR);
	pseudoshell(cmd);
	snooze(1);
	puts("Loopback test ended.");
}

static int	stopDTN(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
			saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
#ifndef NASA_PROTECTED_FLIGHT_CODE
	/*	Stop CFDP.						*/

	pseudoshell("cfdpadmin .");
	while (cfdp_entity_is_started())
	{
		snooze(1);
	}
#endif

	/*	Stop BP.						*/

	pseudoshell("bpadmin .");
	while (bp_agent_is_started())
	{
		snooze(1);
	}

	/*	Stop LTP.					*/

	pseudoshell("ltpadmin .");
	while (ltp_engine_is_started())
	{
		snooze(1);
	}

	/*	Stop rfxclock.						*/

	pseudoshell("ionadmin .");
	while (rfx_system_is_started())
	{
		snooze(1);
	}

	/*	Erase all ION data in DRAM.				*/

	ionTerminate();
	return 0;
}

rtems_task	Init(rtems_task_argument ignored)
{
	puts("Inside Init(), creating configuration files.");
	createIonConfigFiles();
	puts("Inside Init(), spawning ION startup tasks.");
	if (startDTN() < 0)
	{
		writeMemo("[?] Can't start ION.");
	}

	testLoopback();
	snooze(1);
	puts("Stopping ION.");
	oK(stopDTN(0, 0, 0, 0, 0, 0, 0, 0, 0, 0));
	puts("ION stopped.");
	exit(0);
}

void	inferUtcDelta(char *correctUtcTimeStamp)
{
	IonVdb	*ionvdb = getIonVdb();
	time_t	correctUtcTime = readTimestampUTC(correctUtcTimeStamp, 0);
	time_t	clocktime = getUTCTime() + ionvdb->deltaFromUTC;
	int	delta = clocktime - correctUtcTime;
	char	buffer[80];

	CHKVOID(setDeltaFromUTC(delta) == 0);
	sprintf(buffer, "[i] Delta from UTC revised, is now %d.", delta);
	writeMemo(buffer);
}

void	showUtcDelta()
{
	IonVdb	*ionvdb = getIonVdb();
	char	buffer[80];

	sprintf(buffer, "[i] Delta from UTC is %d.", ionvdb->deltaFromUTC);
	writeMemo(buffer);
}

/*	*	*	RTEMS configuration	*	*	*	*/

#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER

#define	CONFIGURE_RTEMS_INIT_TASKS_TABLE

#define	CONFIGURE_MAXIMUM_SEMAPHORES				20
#define	CONFIGURE_MAXIMUM_MESSAGE_QUEUES			10
#define	CONFIGURE_MAXIMUM_TASKS					40

#ifndef CONFIGURE_MICROSECONDS_PER_TICK
#define	CONFIGURE_MICROSECONDS_PER_TICK				10000
#endif
#ifndef CONFIGURE_TICKS_PER_TIMESLICE
#define	CONFIGURE_TICKS_PER_TIMESLICE				10
#endif
#define CONFIGURE_LIBIO_MAXIMUM_FILE_DESCRIPTORS		40
#define CONFIGURE_USE_IMFS_AS_BASE_FILESYSTEM

#define CONFIGURE_MAXIMUM_POSIX_THREADS				40
#define CONFIGURE_MAXIMUM_POSIX_MUTEXES				10
#define CONFIGURE_MAXIMUM_POSIX_CONDITION_VARIABLES		10
#define CONFIGURE_MAXIMUM_POSIX_SEMAPHORES			100
#define CONFIGURE_MAXIMUM_POSIX_MESSAGE_QUEUES			10

#define	CONFIGURE_STACK_CHECKER_ON
#define	CONFIGURE_ZERO_WORKSPACE_AUTOMATICALLY			TRUE

#define	CONFIGURE_DISABLE_CLASSIC_NOTEPADS

#define	CONFIGURE_INIT

#undef Object
#include <rtems/confdefs.h>

/* Loopback Network Configuration needed to prevent linking with dummy.o */

extern int rtems_bsdnet_loopattach(struct rtems_bsdnet_ifconfig *, int);

static struct rtems_bsdnet_ifconfig	loopback_config =
{
	"lo0",				/* name */
	rtems_bsdnet_loopattach,	/* attach function */
	NULL,				/* link to next interface */
	"127.0.0.1",			/* IP address */
	"255.0.0.0",			/* IP net mask */
};

struct rtems_bsdnet_config		rtems_bsdnet_config =
{
	&loopback_config,		/* Network interface */
	NULL,				/* Use fixed network configuration */
	0,				/* Default network task priority */
	0,				/* Default mbuf capacity */
	0,				/* Default mbuf cluster capacity */
	"127.0.0.1",			/* Host name */
	"localdomain",			/* Domain name */
	"127.0.0.1",			/* Gateway */
	"127.0.0.1",			/* Log host */
	{"127.0.0.1" },			/* Name server(s) */
	{"127.0.0.1" },			/* NTP server(s) */
	2,				/* sb_efficiency */
	8192,				/* udp_tx_buf_size */
	8192,				/* udp_rx_buf_size */
	8192,				/* tcp_tx_buf_size */
	8192				/* tcp_rx_buf_size */
};
