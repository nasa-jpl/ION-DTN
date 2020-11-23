/*
	libtcc.c:	standard common function offered to Trusted
			Collective clients.

	Nodeor: Scott Burleigh, JPL

	Copyright (c) 2020, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "tccP.h"

int	tcc_getBulletin(int blocksGroupNbr, char **bulletinContent,
	       	int *length)
{
	Sdr		sdr;
	Object		dbobj;
	TccDB		db;
	TccVdb		*vdb;
	Object		contentElt;
	Object		contentObj;
	TccContent	content;

	CHKERR(bulletinContent && length);
	*bulletinContent = NULL;	/*	Default.		*/
	*length = 0;			/*	Default.		*/
	if (tccAttach(blocksGroupNbr) < 0)
	{
		putErrmsg("Can't attach to TC client support",
				itoa(blocksGroupNbr));
		return -1;
	}

	dbobj = getTccDBObj(blocksGroupNbr);
	vdb = getTccVdb(blocksGroupNbr);
	sdr = getIonsdr();
	CHKERR(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &db, dbobj, sizeof(TccDB));
	contentElt = sdr_list_first(sdr, db.contents);
	while (contentElt == 0)
	{
		sdr_exit_xn(sdr);

		/*	Wait until bulletin is acquired.		*/

		if (sm_SemTake(vdb->contentSemaphore) < 0)
		{
			putErrmsg("Can't take content semaphore.", NULL);
			return -1;
		}

		if (sm_SemEnded(vdb->contentSemaphore))
		{
			writeMemo("[i] TCC stop has been signaled.");
			return -1;
		}

		CHKERR(sdr_begin_xn(sdr));
		contentElt = sdr_list_first(sdr, db.contents);
	}

	contentObj = sdr_list_data(sdr, contentElt);
	sdr_read(sdr, (char *) &content, contentObj, sizeof(TccContent));
	*bulletinContent = MTAKE(content.length);
	if (*bulletinContent == NULL)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't allocate buffer for bulletin content.", NULL);
		return-1;
	}

	*length = content.length;
	sdr_read(sdr, *bulletinContent, content.data, content.length);
	sdr_free(sdr, content.data);
	sdr_free(sdr, contentObj);
	sdr_list_delete(sdr, contentElt, NULL, NULL);
	return sdr_end_xn(sdr);
}
