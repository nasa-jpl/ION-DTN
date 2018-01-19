/*
	amsmib.c:	an AMS utility program that announces
			MIB updates to the modules of a venture.
									*/
/*									*/
/*	Copyright (c) 2011, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include "ams.h"

#define	MAX_MIB_UPDATE_TEXT	(4096)

static void	announceMibUpdate(AmsModule me, char *roleName,
			char *continuumName, char *unitName, char *fileName)
{
	int	amsmibSubj;
	int	domainRole;
	int	domainContinuum;
	int	domainUnit;
	int	fd;
	off_t	fileLength;
	int	contentLength;
	char	content[MAX_MIB_UPDATE_TEXT];
	int	result;

	amsmibSubj = ams_lookup_subject_nbr(me, "amsmib");
	if (amsmibSubj < 0)
	{
		writeMemo("[?] amsmib subject undefined.");
		return;
	}

	if (*roleName == '\0')
	{
		domainRole = 0;			/*	All roles.	*/
	}
	else
	{
		domainRole = ams_lookup_role_nbr(me, roleName);
		if (domainRole < 0)
		{
			writeMemoNote("[?] amsmib domain role unknown",
					roleName);
			return;
		}
	}

	if (*continuumName == '\0')
	{
		domainContinuum = 0;		/*	All continua.	*/
	}
	else
	{
		domainContinuum = ams_lookup_continuum_nbr(me, continuumName);
		if (domainContinuum < 0)
		{
			writeMemoNote("[?] amsmib domain continuum unknown",
					continuumName);
			return;
		}
	}

	if (*unitName == '\0')
	{
		domainUnit = 0;			/*	All roles.	*/
	}
	else
	{
		domainUnit = ams_lookup_unit_nbr(me, unitName);
		if (domainUnit < 0)
		{
			writeMemoNote("[?] amsmib domain unit unknown",
					unitName);
			return;
		}
	}

	fd = iopen(fileName, O_RDONLY, 0777);
	if (fd < 0)
	{
		writeMemoNote("[?] amsmib can't open MIB file", fileName);
		return;
	}

	fileLength = lseek(fd, 0, SEEK_END);
	if (fileLength == (off_t) -1)
	{
		putSysErrmsg("Can't seek to end of MIB file", fileName);
		close(fd);
		return;
	}

	if (fileLength > (off_t) MAX_MIB_UPDATE_TEXT)
	{
		writeMemoNote("[?] MIB file length > 4096", fileName);
		close(fd);
		return;
	}

	contentLength = fileLength;
	oK(lseek(fd, 0, SEEK_SET));
	result = read(fd, content, contentLength);
	close(fd);
	if (result < 0)
	{
		putSysErrmsg("Can't read MIB file", fileName);
		return;
	}

	result = ams_announce(me, domainRole, domainContinuum, domainUnit,
			amsmibSubj, 1, 0, contentLength, content, 0);
	if (result < 0)
	{
		putErrmsg("amsmib can't announce 'amsmib' message.", NULL);
	}
}

#if defined (ION_LWT)
int	amsmib(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char		*applicationName = (char *) a1;
	char		*authorityName = (char *) a2;
	char		*roleName = (char *) a3;
	char		*continuumName = (char *) a4;
	char		*unitName = (char *) a5;
	char		*fileName = (char *) a6;
#else
int	main(int argc, char **argv)
{
	char		*applicationName = (argc > 1 ? argv[1] : NULL);
	char		*authorityName = (argc > 2 ? argv[2] : NULL);
	char		*roleName = (argc > 3 ? argv[3] : NULL);
	char		*continuumName = (argc > 4 ? argv[4] : NULL);
	char		*unitName = (argc > 5 ? argv[5] : NULL);
	char		*fileName = (argc > 6 ? argv[6] : NULL);
#endif
	AmsModule	me;

	if (applicationName == NULL || authorityName == NULL
	|| roleName == NULL || continuumName == NULL
	|| unitName == NULL || fileName == NULL)
	{
		PUTS("Usage: amsmib <application name> <authority name> \
<roleName or \"\"> <continuumName or \"\"> <unitName> <fileName>");
		return 0;
	}

	if (ams_register("", NULL, applicationName, authorityName, "",
			"amsmib", &me) < 0)
	{
		putErrmsg("amsmib can't register.", NULL);
		return 1;
	}

	snooze(4);
	announceMibUpdate(me, roleName, continuumName, unitName, fileName);
	snooze(1);
	ams_unregister(me);
	writeErrmsgMemos();
	return 0;
}
