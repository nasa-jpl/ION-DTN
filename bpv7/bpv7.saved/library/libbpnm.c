/*
 *	libbpnm.c:	functions that implement the BP instrumentation
 *			API.
 *
 *	Copyright (c) 2011, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Larry Shackleford, GSFC
 *	Modified by Scott Burleigh per changes in MIB definition
 */

#include "bpP.h"
#include "bpnm.h"
#include "bei.h"

/*	*	*	*	Utility		*	*	*	*/

/*****************************************************************************/
/* Fill a buffer with a unique and identifiable fill pattern. This pattern will 
   appear as unique constant (the base pattern) in the first 3 bytes and an 
   incrementing numerical pattern every fourth byte.  The pattern repeats for 
   as many bytes as requested.
          i.e. xxxxxx00 xxxxxx01 xxxxxx02 xxxxxx03 xxxxxx04 etc. 
               where xx is the unique fill (base) pattern for this instance   */

static void memset_IncFillPat (char * destPtr, char basePattern, int numBytes)
{
    int     loop = 0;
    char    incrementingCounter = 0;
    
    for (loop = 0; loop < numBytes; loop++)
    {
        if ( (loop % 4) == 3)
	{
		*(destPtr + loop) = incrementingCounter;
		incrementingCounter++;
	}
        else
	{
		*(destPtr + loop) = basePattern;
	}
    }
}   /* end of memset_IncFillPat */

/*	*	*	*	Endpoints	*	*	*	*/

/*****************************************************************************/
/* This routine will examine the current ION node and return the names of all 
   of the endpoints currently defined.  The calling routine should provide:
     1) one buffer large enough to hold all of the names (in string format).
        Each string will be stored sequentially in this buffer with the string's
        NULL terminator separating each.
     2) the size of that buffer.
     3) an array of string pointers with enough entries to point to all of the 
        strings.  The pointers returned in this array will point to locations 
        within the buffer defined in parameter #1 above.
     4) an integer that will return the number of items found.
   The routine below will modify parameters 1, 3, and 4.
*/   

void	bpnm_endpointNames_get(char * nameBuffer, int bufLen,
		char * nameArray [], int * numStrings)
{
    PsmPartition    ionwm = getIonwm();
    VScheme       * vscheme;
    VEndpoint     * vpoint;
    PsmAddress      schemeElt;
    PsmAddress      endpointElt;
    char            computedName [BPNM_ENDPOINT_EIDSTRING_LEN];
    char          * bufPtr;

    CHKVOID(nameBuffer);
    CHKVOID(nameArray);
    CHKVOID(numStrings);
    (* numStrings) = 0;
    bufPtr = nameBuffer;
    for (schemeElt = sm_list_first(ionwm, (getBpVdb())->schemes);
	schemeElt; schemeElt = sm_list_next(ionwm, schemeElt) )
    {
        vscheme = (VScheme *) psp(ionwm, sm_list_data(ionwm, schemeElt));
        for (endpointElt = sm_list_first(ionwm, vscheme->endpoints); 
             endpointElt; endpointElt = sm_list_next(ionwm, endpointElt))
        {
            vpoint = (VEndpoint *) psp(ionwm, sm_list_data(ionwm, endpointElt));

            /* form the Scheme Name and the NSS into the Endpoint ID in
	     * the form "schemename:nss" */

            isprintf(computedName, sizeof(computedName),
			BPNM_ENDPOINT_NAME_FMT_STRING, 
			vscheme->name, vpoint->nss);

            /* Move the name into the buffer supplied by the caller */
            istrcpy (bufPtr, computedName, bufLen);
            nameArray [*numStrings] = bufPtr;
            bufPtr += (istrlen (computedName, BPNM_ENDPOINT_EIDSTRING_LEN) + 1);
            bufLen -= (istrlen (computedName, BPNM_ENDPOINT_EIDSTRING_LEN) + 1);
	    if (bufLen < 0)	/*	Buffer is too small.		*/
	    {
		    putErrmsg("Buffer too small for all endpoint names.", NULL);
		    return;
	    }
            
            (*numStrings)++;
        }
    }
}   /* end of bpnm_endpointNames_get */

/*****************************************************************************/
/* This routine will examine the current ION node and return the VEndpoint
   for the endpoint requested.  The calling routine should provide:
     1) a string containing the name of the endpoint to be found.  It is 
        assumed that this name would have been returned to the caller via a  
        previous call to the function bpnm_endpointNames_get (above).
     2) A buffer large enough to hold a pointer to the VEndpoint.
     3) an integer flag indicating whether the buffer contains data when
        returned.  (0 = data not updated, 1 = data has been updated
*/   

static void getBpEndpoint(char * targetName, VEndpoint **vpoint, int  * success)
{
    PsmPartition    ionwm = getIonwm();
    VScheme       * vscheme;
    PsmAddress      schemeElt;
    PsmAddress      endpointElt;
    char            computedName [BPNM_ENDPOINT_EIDSTRING_LEN];

    * success = 0;

    for (schemeElt = sm_list_first(ionwm, (getBpVdb())->schemes); 
         schemeElt;
         schemeElt = sm_list_next(ionwm, schemeElt) )
    {
        vscheme = (VScheme *) psp(ionwm, sm_list_data(ionwm, schemeElt));
        for (endpointElt = sm_list_first(ionwm, vscheme->endpoints); 
		endpointElt; endpointElt = sm_list_next(ionwm, endpointElt))
        {
            *vpoint = (VEndpoint *) psp(ionwm, sm_list_data(ionwm,
			endpointElt));

            /* form the Scheme Name and the NSS into the Endpoint ID
	     * in the form "ipn:4.2" */

            isprintf(computedName, sizeof(computedName),
			BPNM_ENDPOINT_NAME_FMT_STRING, 
			vscheme->name, (*vpoint)->nss);
                     
            /* Is the current EID string the one that you're looking for? */

            if (strcmp (targetName, computedName) == 0)
            {
                * success = 1;
                return;
            }
        }
    }
}   /* end of getBpEndpointStats */

/*****************************************************************************/

void	bpnm_endpoint_get (char * targetName, NmbpEndpoint * results,
		int * success)
{
    Sdr             sdr = getIonsdr();
    VEndpoint     * vpoint = NULL;
    Endpoint        endpoint;
    Scheme          scheme;

    CHKVOID(targetName);
    CHKVOID(results);
    CHKVOID(success);
    getBpEndpoint(targetName, & vpoint, success);
    if ( (* success) != 0)	/*	VEndpoint was found.		*/
    {
        /* DEBUGGING AID ONLY:  Useful for showing which fields have yet
	 * to be assigned. */
        memset_IncFillPat ( (char *) results, BPNM_ENDPOINT_FILLPATT,
			sizeof(NmbpEndpoint) );
        istrcpy ( results->eid, targetName, sizeof results->eid );
	results->active = (vpoint->appPid != ERROR);
        CHKVOID(sdr_begin_xn(sdr));
        sdr_read(sdr, (char *) & endpoint, sdr_list_data(sdr,
			vpoint->endpointElt), sizeof(Endpoint));
	results->abandonOnDelivFailure = (endpoint.recvRule == DiscardBundle);
        sdr_read(sdr, (char *) & scheme, endpoint.scheme, sizeof(Scheme));
	results->singleton = scheme.unicast;
        sdr_exit_xn(sdr);
    }
}   /* end of bpnm_endpoint_get */

/*	*	*	*	Node	*	*	*	*	*/

void	bpnm_node_get(NmbpNode *buf)
{
	Sdr	sdr = getIonsdr();
	Object	bpDbObject = getBpDbObject();
	BpDB	bpdb;

	CHKVOID(buf);
	isprintf(buf->nodeID, sizeof buf->nodeID, "ipn:" UVAST_FIELDSPEC ".0",
			getOwnNodeNbr());
	isprintf(buf->bpVersionNbr, sizeof buf->bpVersionNbr, "%d", BP_VERSION);
	CHKVOID(sdr_begin_xn(sdr));
	buf->avblStorage =
		(zco_get_max_file_occupancy(sdr, ZcoOutbound)
				- zco_get_file_occupancy(sdr, ZcoOutbound))
		+ (zco_get_max_heap_occupancy(sdr, ZcoOutbound)
				- zco_get_heap_occupancy(sdr, ZcoOutbound));
	sdr_read(sdr, (char *) &bpdb, bpDbObject, sizeof(BpDB));
	buf->lastRestartTime = bpdb.startTime;
	buf->nbrOfRegistrations = bpdb.regCount;
	sdr_exit_xn(sdr);
}

/*****************************************************************************/
/* This routine will examine the current ION node and return the names of all 
   of the extensions currently supported.  The calling routine should provide:
     1) one buffer large enough to hold all of the names (in string format).
        Each string will be stored sequentially in this buffer with the string's
        NULL terminator separating each.
     2) an array of string pointers with enough entries to point to all of the 
        strings.  The pointers returned in this array will point to locations 
        within the buffer defined in parameter #1 above.
     3) an integer that will return the number of items found.
   The routine below will modify each of these 3 parameters.
*/   

void	bpnm_extensions_get(char * nameBuffer, int bufLen, char * nameArray [],
		int * numStrings)
{
	ExtensionDef	*extensions;
	ExtensionDef	*ext;
	int		extensionsCount;
	char		*bufPtr;
	int		i;

	CHKVOID(nameBuffer);
	CHKVOID(bufLen > 0);
	CHKVOID(nameArray);
	CHKVOID(numStrings);
	(* numStrings) = 0;
	getExtensionDefs(&extensions, &extensionsCount);
	bufPtr = nameBuffer;
	for (i = 0, ext = extensions; i < extensionsCount; i++, ext++)
	{
		/* Move the name into the buffer supplied by the caller */

		istrcpy(bufPtr, ext->name, bufLen);
		nameArray[*numStrings] = bufPtr;
		bufPtr += (istrlen(ext->name, 32) + 1);
		bufLen -= (istrlen(ext->name, 32) + 1);
		if (bufLen < 0)
		{
			putErrmsg("Buffer too small for all extensions.", NULL);
			return;
		}

		(*numStrings)++;
	}
}   /* end of bpnm_endpointNames_get */

void    bpnm_disposition_get(NmbpDisposition * results)
{
    Sdr             sdr = getIonsdr();
    Object          bpDbObject = getBpDbObject();
    BpDB            bpdb;
    int             i;
    Tally           *tally;
    BpCosStats      cosStats;
    Object          elt;
    Scheme          scheme;
    Object          elt2;
    Endpoint        endpoint;
    BpPlan          plan;
    BpDelStats      delStats;
    BpCtStats       ctStats;
    BpDbStats       dbStats;

    CHKVOID(results);
    CHKVOID(sdr_begin_xn(sdr));

    /*		Retention constraints					*/

    sdr_read(sdr, (char *) &bpdb, bpDbObject, sizeof(BpDB));
    results->currentForwardPending = sdr_list_length(sdr, bpdb.limboQueue);
    for (elt = sdr_list_first(sdr, bpdb.plans); elt;
                    elt = sdr_list_next(sdr, elt))
    {
        sdr_read(sdr, (char *) &plan, sdr_list_data(sdr, elt), sizeof(BpPlan));
	results->currentForwardPending += sdr_list_length(sdr,
			plan.urgentQueue);
	results->currentForwardPending += sdr_list_length(sdr, plan.stdQueue);
	results->currentForwardPending += sdr_list_length(sdr, plan.bulkQueue);
    }

    results->currentDispatchPending = 0;
    sdr_read(sdr, (char *) &ctStats, bpdb.ctStats, sizeof(BpCtStats));
    results->currentInCustody
	    = ctStats.tallies[BP_CT_CUSTODY_ACCEPTED].totalCount
	    - (ctStats.tallies[BP_CT_CUSTODY_RELEASED].totalCount
	    +  ctStats.tallies[BP_CT_CUSTODY_EXPIRED].totalCount);

    results->currentReassemblyPending = 0;
    for (elt = sdr_list_first(sdr, bpdb.schemes); elt;
            elt = sdr_list_next(sdr, elt))
    {
	    sdr_read(sdr, (char *) & scheme, sdr_list_data(sdr, elt),
                    sizeof(Scheme));
            results->currentDispatchPending
		    += sdr_list_length(sdr, scheme.forwardQueue);
	    for (elt2 = sdr_list_first(sdr, scheme.endpoints); elt2;
                    elt2 = sdr_list_next(sdr, elt2))
	    {
	        sdr_read(sdr, (char *) & endpoint, sdr_list_data(sdr, elt2),
                        sizeof(Endpoint));
                results->currentReassemblyPending
		        += sdr_list_length(sdr, endpoint.incompletes);
	    }
    }

    /*		Priority counters					*/

    for (i = 0; i < 3; i++)
    {
        results->currentResidentCount[i] = 0;
        results->currentResidentBytes[i] = 0;
    }

    sdr_read(sdr, (char *) &cosStats, bpdb.sourceStats, sizeof(BpCosStats));
    for (i = 0, tally = cosStats.tallies; i < 3; i++, tally++)
    {
    	results->bundleSourceCount[i] = tally->currentCount;
    	results->bundleSourceBytes[i] = tally->currentBytes;
        results->currentResidentCount[i] += tally->totalCount;
        results->currentResidentBytes[i] += tally->totalBytes;
    }

    sdr_read(sdr, (char *) &cosStats, bpdb.recvStats, sizeof(BpCosStats));
    for (i = 0, tally = cosStats.tallies; i < 3; i++, tally++)
    {
        results->currentResidentCount[i] += tally->totalCount;
        results->currentResidentBytes[i] += tally->totalBytes;
    }

    sdr_read(sdr, (char *) &cosStats, bpdb.discardStats, sizeof(BpCosStats));
    for (i = 0, tally = cosStats.tallies; i < 3; i++, tally++)
    {
        results->currentResidentCount[i] -= tally->totalCount;
        results->currentResidentBytes[i] -= tally->totalBytes;
    }

    /*		Fragmentation						*/

    results->bundlesFragmented = bpdb.currentBundlesFragmented;
    results->fragmentsProduced = bpdb.currentFragmentsProduced;

    /*		Bundle deletions					*/

    sdr_read(sdr, (char *) &delStats, bpdb.delStats, sizeof(BpDelStats));
    results->delNoneCount
	    = delStats.currentDelByReason[BP_REASON_NONE];
    results->delExpiredCount
	    = delStats.currentDelByReason[BP_REASON_EXPIRED];
    results->delFwdUnidirCount
	    = delStats.currentDelByReason[BP_REASON_FWD_UNIDIR];
    results->delCanceledCount
	    = delStats.currentDelByReason[BP_REASON_CANCELED];
    results->delDepletionCount
	    = delStats.currentDelByReason[BP_REASON_DEPLETION];
    results->delEidMalformedCount
	    = delStats.currentDelByReason[BP_REASON_EID_MALFORMED];
    results->delNoRouteCount
	    = delStats.currentDelByReason[BP_REASON_NO_ROUTE];
    results->delNoContactCount
	    = delStats.currentDelByReason[BP_REASON_NO_CONTACT];
    results->delBlkMalformedCount
	    = delStats.currentDelByReason[BP_REASON_BLK_MALFORMED];

    /*		Bundle processing errors				*/

    sdr_read(sdr, (char *) &ctStats, bpdb.ctStats, sizeof(BpCtStats));
    results->custodyRefusedCount = 0;
    results->custodyRefusedBytes = 0;
    results->custodyRefusedCount
	    += ctStats.tallies[BP_CT_REDUNDANT].currentCount;
    results->custodyRefusedBytes
	    += ctStats.tallies[BP_CT_REDUNDANT].currentBytes;
    results->custodyRefusedCount
	    += ctStats.tallies[BP_CT_DEPLETION].currentCount;
    results->custodyRefusedBytes
	    += ctStats.tallies[BP_CT_DEPLETION].currentBytes;
    results->custodyRefusedCount
	    += ctStats.tallies[BP_CT_EID_MALFORMED].currentCount;
    results->custodyRefusedBytes
	    += ctStats.tallies[BP_CT_EID_MALFORMED].currentBytes;
    results->custodyRefusedCount
	    += ctStats.tallies[BP_CT_NO_ROUTE].currentCount;
    results->custodyRefusedBytes
	    += ctStats.tallies[BP_CT_NO_ROUTE].currentBytes;
    results->custodyRefusedCount
	    += ctStats.tallies[BP_CT_NO_CONTACT].currentCount;
    results->custodyRefusedBytes
	    += ctStats.tallies[BP_CT_NO_CONTACT].currentBytes;
    results->custodyRefusedCount
	    += ctStats.tallies[BP_CT_BLK_MALFORMED].currentCount;
    results->custodyRefusedBytes
	    += ctStats.tallies[BP_CT_BLK_MALFORMED].currentBytes;

    sdr_read(sdr, (char *) &dbStats, bpdb.dbStats, sizeof(BpDbStats));
    results->bundleFwdFailedCount
	    = dbStats.tallies[BP_DB_FWD_FAILED].currentCount;
    results->bundleFwdFailedBytes
	    = dbStats.tallies[BP_DB_FWD_FAILED].currentBytes;
    results->bundleAbandonCount
	    = dbStats.tallies[BP_DB_ABANDON].currentCount;
    results->bundleAbandonBytes
	    = dbStats.tallies[BP_DB_ABANDON].currentBytes;
    results->bundleDiscardCount
	    = dbStats.tallies[BP_DB_DISCARD].currentCount;
    results->bundleDiscardBytes
	    = dbStats.tallies[BP_DB_DISCARD].currentBytes;

    results->bytesDeletedToDate
	    = dbStats.tallies[BP_DB_DISCARD].totalBytes;
    sdr_exit_xn(sdr);

}   /* end of bpnm_disposition_get */

/*****************************************************************************/
static void    resetSourceStats(BpDB *db)
{
    Sdr             sdr = getIonsdr();
    BpCosStats      stats;
    int             i;
    Tally         * tally;

    sdr_stage(sdr, (char *) &stats, db->sourceStats, sizeof(BpCosStats));
    for (i = 0, tally = stats.tallies; i < 3; i++, tally++)
    {
        tally->currentCount = 0;
        tally->currentBytes = 0;
    }

    sdr_write(sdr, db->sourceStats, (char *) &stats, sizeof(BpCosStats));

}   /* end of resetSourceStats */

/*****************************************************************************/
static void    resetRecvStats(BpDB *db)
{
    Sdr             sdr = getIonsdr();
    BpCosStats      stats;
    int             i;
    Tally         * tally;

    sdr_stage(sdr, (char *) &stats, db->recvStats, sizeof(BpCosStats));
    for (i = 0, tally = stats.tallies; i < 3; i++, tally++)
    {
        tally->currentCount = 0;
        tally->currentBytes = 0;
    }

    sdr_write(sdr, db->recvStats, (char *) &stats, sizeof(BpCosStats));

}   /* end of resetRecvStats */

/*****************************************************************************/
static void    resetDiscardStats(BpDB *db)
{
    Sdr             sdr = getIonsdr();
    BpCosStats      stats;
    int             i;
    Tally         * tally;

    sdr_stage(sdr, (char *) &stats, db->discardStats, sizeof(BpCosStats));
    for (i = 0, tally = stats.tallies; i < 3; i++, tally++)
    {
        tally->currentCount = 0;
        tally->currentBytes = 0;
    }

    sdr_write(sdr, db->discardStats, (char *) &stats, sizeof(BpCosStats));

}   /* end of resetDiscardStats */

/*****************************************************************************/
static void    resetXmitStats(BpDB *db)
{
    Sdr             sdr = getIonsdr();
    BpCosStats      stats;
    int             i;
    Tally         * tally;

    sdr_stage(sdr, (char *) &stats, db->xmitStats, sizeof(BpCosStats));
    for (i = 0, tally = stats.tallies; i < 3; i++, tally++)
    {
        tally->currentCount = 0;
        tally->currentBytes = 0;
    }

    sdr_write(sdr, db->xmitStats, (char *) &stats, sizeof(BpCosStats));

}   /* end of resetXmitStats */

/*****************************************************************************/
static void    resetDelStats(BpDB *db)
{
    Sdr             sdr = getIonsdr();
    BpDelStats      stats;
    int             i;

    sdr_stage(sdr, (char *) &stats, db->delStats, sizeof(BpDelStats));
    for (i = 0; i < BP_REASON_STATS; i++)
    {
        stats.currentDelByReason[i] = 0;
    }

    sdr_write(sdr, db->delStats, (char *) &stats, sizeof(BpDelStats));

}   /* end of resetDelStats */

/*****************************************************************************/
static void    resetCtStats(BpDB *db)
{
    Sdr             sdr = getIonsdr();
    BpCtStats       stats;
    int             i;
    Tally         * tally;

    sdr_stage(sdr, (char *) &stats, db->ctStats, sizeof(BpCtStats));
    for (i = 0, tally = stats.tallies; i < BP_CT_STATS; i++, tally++)
    {
        tally->currentCount = 0;
        tally->currentBytes = 0;
    }

    sdr_write(sdr, db->ctStats, (char *) &stats, sizeof(BpCtStats));

}   /* end of resetCtStats */

/*****************************************************************************/
static void    resetDbStats(BpDB *db)
{
    Sdr             sdr = getIonsdr();
    BpDbStats       stats;
    int             i;
    Tally         * tally;

    sdr_stage(sdr, (char *) &stats, db->dbStats, sizeof(BpDbStats));
    for (i = 0, tally = stats.tallies; i < BP_DB_STATS; i++, tally++)
    {
        tally->currentCount = 0;
        tally->currentBytes = 0;
    }

    sdr_write(sdr, db->dbStats, (char *) &stats, sizeof(BpDbStats));

}   /* end of bpnm_dbStats_reset */

/*****************************************************************************/
void    bpnm_disposition_reset()
{
    Sdr             sdr = getIonsdr();
    Object          dbobj;
    BpDB            db;

    dbobj = getBpDbObject();
    CHKVOID(sdr_begin_xn(sdr));
    sdr_stage(sdr, (char *) &db, dbobj, sizeof(BpDB));
    db.resetTime = getCtime();
    resetSourceStats(&db);
    resetRecvStats(&db);
    resetDiscardStats(&db);
    resetXmitStats(&db);
    resetDelStats(&db);
    resetCtStats(&db);
    resetDbStats(&db);
    db.currentBundlesFragmented = 0;
    db.currentFragmentsProduced = 0;
    sdr_write(sdr, dbobj, (char *) &db, sizeof(BpDB));
    oK(sdr_end_xn(sdr));
    
}   /* end of bpnm_disposition_reset */
