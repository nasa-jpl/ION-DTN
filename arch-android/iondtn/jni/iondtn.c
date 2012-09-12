/*
 *  ION JNI interface.
 */

#include <jni.h>
#include "gov_nasa_jpl_iondtn_NodeAdministrator.h"
#include "platform.h"
#include "ion.h"
#include "rfx.h"
#include "bp.h"

#define	ION_NODE_NBR	19

static void	createIonConfigFiles()
{
	int	nodenbr = ION_NODE_NBR;
	char	filenamebuf[80];
	int	fd;
	char	*ionconfigLines[] =	{
"wmSize 300000\n",
"configFlags 1\n",
"heapWords 75000\n",
"pathName /ion\n",
					};
	int	ionconfigLineCount = sizeof ionconfigLines / sizeof (char *);
	char	*globalLines[] =	{
"a contact +0 +7200 19 19 100000\n"
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

	isprintf(filenamebuf, sizeof filenamebuf, "/ion/node%d.ionconfig",
			nodenbr);
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

	isprintf(filenamebuf, sizeof filenamebuf, "/ion/node%d.ionrc", nodenbr);
	fd = iopen(filenamebuf, O_WRONLY | O_CREAT | O_TRUNC, 0777);
	if (fd < 0)
	{
		printf("Can't create .ionrc file '%s'.\n", filenamebuf);
		return;
	}

	isprintf(linebuf, sizeof linebuf, "1 %d /ion/node%d.ionconfig\ns\n",
			nodenbr, nodenbr);
	oK(iputs(fd, linebuf));
	close(fd);

	/*	Create global.ionrc file.				*/

	isprintf(filenamebuf, sizeof filenamebuf, "/ion/global.ionrc",
			nodenbr);
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

	isprintf(filenamebuf, sizeof filenamebuf, "/ion/node%d.ionsecrc",
			nodenbr);
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

	isprintf(filenamebuf, sizeof filenamebuf, "/ion/node%d.ltprc",
			nodenbr);
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

	isprintf(filenamebuf, sizeof filenamebuf, "/ion/node%d.ipnrc",
			nodenbr);
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

	isprintf(filenamebuf, sizeof filenamebuf, "/ion/node%d.bprc",
			nodenbr);
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

	isprintf(linebuf, sizeof linebuf, "r 'ipnadmin /ion/node%d.ipnrc'\ns\n",
			nodenbr);
	oK(iputs(fd, linebuf));
	close(fd);

#ifndef NASA_PROTECTED_FLIGHT_CODE
	/*	Create cfdprc file.					*/

	isprintf(filenamebuf, sizeof filenamebuf, "/ion/node%d.cfdprc",
			nodenbr);
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

JNIEXPORT jstring JNICALL Java_gov_nasa_jpl_iondtn_NodeAdministrator_init(JNIEnv *env, jobject this)
{
	int	nodenbr = ION_NODE_NBR;
	char	cmd[80];
	int	count;
	char	*result = "ION node started.";

	sm_ipc_init();
	createIonConfigFiles();
	isprintf(cmd, sizeof cmd, "ionadmin /ion/node%d.ionrc", nodenbr);
	pseudoshell(cmd);
	count = 5;
	while (rfx_system_is_started() == 0)
	{
		snooze(1);
		count--;
		if (count == 0)
		{
			result = "IonDTN not started: RFX start hung up, \
abandoned.";
			return (*env)->NewStringUTF(env, result);
		}
	}

	pseudoshell("ionadmin /ion/global.ionrc");
	snooze(1);
	isprintf(cmd, sizeof cmd, "ionsecadmin /ion/node%d.ionsecrc", nodenbr);
	pseudoshell(cmd);
	snooze(1);

	/*	Now start the Bundle Protocol agent.			*/

	isprintf(cmd, sizeof cmd, "bpadmin /ion/node%d.bprc", nodenbr);
	pseudoshell(cmd);
	count = 5;
	while (bp_agent_is_started() == 0)
	{
		snooze(1);
		count--;
		if (count == 0)
		{
			result = "IonDTN not started: BP start hung up, \
abandoned.";
			return (*env)->NewStringUTF(env, result);
		}
	}

	isprintf(cmd, sizeof cmd, "lgagent ipn:%d.127", nodenbr);
	pseudoshell(cmd);
	snooze(1);
	return (*env)->NewStringUTF(env, result);
}
