/*
 *	libltpnm.c:	functions that implement the LTP instrumentation
 *			API.
 *
 *	Copyright (c) 2011, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 *
 *	Author: Larry Shackleford, GSFC
 *
 */

#include "ltpP.h"
#include "ltpnm.h"

/****************************************************************************/
/* This routine will report current LTP heapBytesReserved and heapBytesOccupied
   at the local ION node.
   The calling routine should provide:
     1) a long integer to hold heapBytesReserved
     1) a long integer to hold heapBytesOccupied
     
   The routine below will modify each of these 2 parameters.
*/
void ltpnm_resources (unsigned long *heapBytesReserved,
		unsigned long *heapBytesOccupied)
{
    Sdr             sdr = getIonsdr();
    LtpDB           db;

    CHKVOID(heapBytesReserved);
    CHKVOID(heapBytesOccupied);
    sdr_read(sdr, (char *) &db, getLtpDbObject(), sizeof(LtpDB));
    *heapBytesReserved = db.heapSpaceBytesReserved;
    *heapBytesOccupied = db.heapSpaceBytesOccupied;
}

/****************************************************************************/
/* This routine will examine the current ION node and return the names of all 
   of the Span Neighboring Engine IDs currently defined.  
   The calling routine should provide:
     1) an array of integers with enough entries to hold all of the IDs.
     2) an integer that provides the length of the array and, on return,
        will provide the number of engine IDs found (up to the array length).
     
   The routine below will modify each of these 2 parameters.
*/
void ltpnm_spanEngineIds_get (unsigned int IdArray [], int * numIds)
{
    Sdr             sdr = getIonsdr();
    int             maxEngines;
    Object          sdrElt;
    Object          spanObj;
    LtpSpan         span;

    CHKVOID(numIds);
    maxEngines = *numIds;
    CHKVOID(maxEngines > 0);
    CHKVOID(IdArray);
    * numIds = 0;
    CHKVOID(sdr_begin_xn(sdr));

    if(getLtpConstants() == NULL)
    {
        sdr_exit_xn(sdr);
        return;
    }

    for (sdrElt = sdr_list_first(sdr, (getLtpConstants())->spans);
         sdrElt; 
         sdrElt = sdr_list_next(sdr, sdrElt))
    {
        spanObj = sdr_list_data(sdr, sdrElt);
        sdr_read(sdr, (char *) &span, spanObj, sizeof(LtpSpan));
        IdArray [*numIds] = span.engineId;

        (*numIds)++;
	if ((*numIds) == maxEngines)
	{
		break;
	}
    }

    sdr_exit_xn(sdr);
}

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
        if ( (loop % 4) == 3) {* (destPtr + loop) = incrementingCounter; incrementingCounter++; }
        else                  {* (destPtr + loop) = basePattern; }
    }

}   /* end of memset_IncFillPat */

/****************************************************************************/
/* This routine will examine the current ION node and return the Span
   statistics for the node requested.  The calling routine should provide:
     1) The engine ID of the span to be found.  It is 
        assumed that this ID would have been returned to the caller via a  
        previous call to the function ltpnm_spanEngineIds_get (above).
     2) A buffer large enough to hold the appropriate statistics.
     3) an integer flag that indicates whether the buffer contains data.
        (0 = data not updated, 1 = data has been updated
*/   
void ltpnm_span_get (unsigned int   engineIdWanted,  
                     NmltpSpan    * results,
                     int          * success)
{
    Sdr             sdr = getIonsdr();
    Object          sdrElt;
    int             eltLoop;
    Object          spanObj;
    LtpSpan         span;
    LtpSpanStats    stats;
    Object          elt2;
    ImportSession   isession;
    
    CHKVOID(engineIdWanted > 0);
    CHKVOID(results);
    CHKVOID(success);
    * success = 0;
    CHKVOID(sdr_begin_xn(sdr));
    if(getLtpConstants() == NULL)
    {
        sdr_exit_xn(sdr);
        return;
    }

    for (eltLoop = 0, sdrElt = sdr_list_first(sdr, (getLtpConstants())->spans);
         sdrElt; 
         eltLoop++, sdrElt = sdr_list_next(sdr, sdrElt))
    {
        spanObj = sdr_list_data(sdr, sdrElt);
        sdr_read(sdr, (char *) & span, spanObj, sizeof(LtpSpan));
        if (engineIdWanted == span.engineId)
        {
            sdr_read(sdr, (char *) & stats, span.stats, sizeof(LtpSpanStats));

            /* DEBUGGING AID ONLY:  Useful for showing which fields have yet to be assigned. */
            memset_IncFillPat ( (char *) results, LTPNM_SPAN_FILLPATT, sizeof(NmltpSpan) );

            results->remoteEngineNbr         = span.engineId;

	    results->currentExportSessions   = sdr_list_length(sdr, span.exportSessions);
	    results->currentOutboundSegments = sdr_list_length(sdr, span.segments);
	    results->currentImportSessions   = sdr_list_length(sdr, span.importSessions);
	    results->currentInboundSegments  = 0;
	    for (elt2 = sdr_list_first(sdr, span.importSessions); elt2; elt2 = sdr_list_next(sdr, elt2))
	    {
                sdr_read(sdr, (char *) & isession, sdr_list_data(sdr, elt2), sizeof(ImportSession));
		results->currentInboundSegments += sdr_list_length(sdr, isession.redSegments);
	    }
        
            sdr_exit_xn(sdr);

            results->lastResetTime           = stats.resetTime;
        
            results->outputSegQueuedCount    = stats.tallies[OUT_SEG_QUEUED].currentCount;
            results->outputSegQueuedBytes    = stats.tallies[OUT_SEG_QUEUED].currentBytes;
            results->outputSegPoppedCount    = stats.tallies[OUT_SEG_POPPED].currentCount;
            results->outputSegPoppedBytes    = stats.tallies[OUT_SEG_POPPED].currentBytes;
        
            results->outputCkptXmitCount     = stats.tallies[CKPT_XMIT         ].currentCount;
            results->outputPosAckRecvCount   = stats.tallies[POS_RPT_RECV      ].currentCount;
            results->outputNegAckRecvCount   = stats.tallies[NEG_RPT_RECV      ].currentCount;
            results->outputCancelRecvCount   = stats.tallies[EXPORT_CANCEL_RECV].currentCount;
            results->outputCkptReXmitCount   = stats.tallies[CKPT_RE_XMIT      ].currentCount;
            results->outputSegReXmitCount   = stats.tallies[SEG_RE_XMIT      ].currentCount;
            results->outputSegReXmitBytes   = stats.tallies[SEG_RE_XMIT      ].currentBytes;
            results->outputCancelXmitCount   = stats.tallies[EXPORT_CANCEL_XMIT].currentCount;
            results->outputCompleteCount     = stats.tallies[EXPORT_COMPLETE   ].currentCount;
        
            results->inputSegRecvRedCount    = stats.tallies[IN_SEG_RECV_RED   ].currentCount;
            results->inputSegRecvRedBytes    = stats.tallies[IN_SEG_RECV_RED   ].currentBytes;
            results->inputSegRecvGreenCount  = stats.tallies[IN_SEG_RECV_GREEN ].currentCount;
            results->inputSegRecvGreenBytes  = stats.tallies[IN_SEG_RECV_GREEN ].currentBytes;
            results->inputSegRedundantCount  = stats.tallies[IN_SEG_REDUNDANT  ].currentCount;
            results->inputSegRedundantBytes  = stats.tallies[IN_SEG_REDUNDANT  ].currentBytes;
            results->inputSegMalformedCount  = stats.tallies[IN_SEG_MALFORMED  ].currentCount;
            results->inputSegMalformedBytes  = stats.tallies[IN_SEG_MALFORMED  ].currentBytes;
            results->inputSegUnkSenderCount  = stats.tallies[IN_SEG_UNK_SENDER ].currentCount;
            results->inputSegUnkSenderBytes  = stats.tallies[IN_SEG_UNK_SENDER ].currentBytes;
            results->inputSegUnkClientCount  = stats.tallies[IN_SEG_UNK_CLIENT ].currentCount;
            results->inputSegUnkClientBytes  = stats.tallies[IN_SEG_UNK_CLIENT ].currentBytes;
            results->inputSegStrayCount      = stats.tallies[IN_SEG_SCREENED   ].currentCount;
            results->inputSegStrayBytes      = stats.tallies[IN_SEG_SCREENED   ].currentBytes;
            results->inputSegMiscolorCount   = stats.tallies[IN_SEG_MISCOLORED ].currentCount;
            results->inputSegMiscolorBytes   = stats.tallies[IN_SEG_MISCOLORED ].currentBytes;
            results->inputSegClosedCount     = stats.tallies[IN_SEG_SES_CLOSED ].currentCount;
            results->inputSegClosedBytes     = stats.tallies[IN_SEG_SES_CLOSED ].currentBytes;
        
            results->inputCkptRecvCount      = stats.tallies[CKPT_RECV         ].currentCount;
            results->inputPosAckXmitCount    = stats.tallies[POS_RPT_XMIT      ].currentCount;
            results->inputNegAckXmitCount    = stats.tallies[NEG_RPT_XMIT      ].currentCount;
            results->inputCancelXmitCount    = stats.tallies[IMPORT_CANCEL_RECV].currentCount;
            results->inputAckReXmitCount     = stats.tallies[RPT_RE_XMIT       ].currentCount;
            results->inputCancelRecvCount    = stats.tallies[IMPORT_CANCEL_XMIT].currentCount;
            results->inputCompleteCount      = stats.tallies[IMPORT_COMPLETE   ].currentCount;
            
            * success = 1;
            return;
        }
    }

    sdr_exit_xn(sdr);
}

/****************************************************************************/
void ltpnm_span_reset (unsigned int engineIdWanted, int * success)
{
    Sdr             sdr = getIonsdr();
    Object          sdrElt;
    int             eltLoop;
    Object          spanObj;
    LtpSpan         span;
    LtpSpanStats    stats;
    Tally         * tally;
    int             tallyLoop;

    CHKVOID(engineIdWanted > 0);
    CHKVOID(success);
    * success = 0;
    CHKVOID(sdr_begin_xn(sdr));
    if(getLtpConstants() == NULL)
    {
        sdr_exit_xn(sdr);
        return;
    }

    for (eltLoop = 0, sdrElt = sdr_list_first(sdr, (getLtpConstants())->spans);
         sdrElt; 
         eltLoop++, sdrElt = sdr_list_next(sdr, sdrElt))
    {
        spanObj = sdr_list_data(sdr, sdrElt);
        sdr_read(sdr, (char *) &span, spanObj, sizeof(LtpSpan));
        if (engineIdWanted == span.engineId)
        {
            sdr_stage(sdr, (char *) & stats, span.stats, sizeof(LtpSpanStats));
            stats.resetTime = getCtime();
            for (tallyLoop = 0; tallyLoop < LTP_SPAN_STATS; tallyLoop++)
            {
                tally = stats.tallies + tallyLoop;
                tally->currentCount = 0;
                tally->currentBytes = 0;
            }

            sdr_write(sdr, span.stats, (char *) & stats, sizeof(LtpSpanStats));
            if (sdr_end_xn(sdr) != 0)
	    {
                putErrmsg("ltpnm_span_reset: sdr_end_xn failed", NULL);
	    }
	    else
	    {
                *success = 1;
	    }

            return;
        }
    }

    sdr_exit_xn(sdr);
}
