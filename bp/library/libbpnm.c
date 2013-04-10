/*
 *	libbpnm.c:	functions that implement the BP instrumentation
 *			API.
 *
 *	Copyright (c) 2011, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Larry Shackleford, GSFC
 */

#include "bpP.h"
#include "bpnm.h"

void    bpnm_resources(double * occupancyCeiling,
		double * maxForecastOccupancy,
		double * currentOccupancy,
		double * maxHeapOccupancy,
		double * currentHeapOccupancy,
		double * maxFileOccupancy,
		double * currentFileOccupancy)
{
    Sdr     sdr = getIonsdr();
    IonDB   db;

    CHKVOID(occupancyCeiling);
    CHKVOID(maxForecastOccupancy);
    CHKVOID(currentOccupancy);
    CHKVOID(maxHeapOccupancy);
    CHKVOID(currentHeapOccupancy);
    CHKVOID(maxFileOccupancy);
    CHKVOID(currentFileOccupancy);
    CHKVOID(sdr_begin_xn(sdr));
    sdr_read(sdr, (char *) &db, getIonDbObject(), sizeof(IonDB));
    *occupancyCeiling = db.occupancyCeiling;
    *maxForecastOccupancy = db.maxForecastOccupancy;
    *maxHeapOccupancy = zco_get_max_heap_occupancy(sdr);
    *currentHeapOccupancy = zco_get_heap_occupancy(sdr);
    *maxFileOccupancy = zco_get_max_file_occupancy(sdr);
    *currentFileOccupancy = zco_get_file_occupancy(sdr);
    *currentOccupancy = *currentHeapOccupancy + *currentFileOccupancy;
    sdr_exit_xn(sdr);
}

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
        if ( (loop % 4) == 3) {* (destPtr + loop) = incrementingCounter; incrementingCounter++; }
        else                  {* (destPtr + loop) = basePattern; }
    }

}   /* end of memset_IncFillPat */

/*	*	*	*	Endpoints	*	*	*	*/

/*****************************************************************************/
/* This routine will examine the current ION node and return the names of all 
   of the endpoints currently defined.  The calling routine should provide:
     1) one buffer large enough to hold all of the names (in string format).
        Each string will be stored sequentially in this buffer with the string's 
        NULL terminator separating each.
     2) an array of string pointers with enough entries to point to all of the 
        strings.  The pointers returned in this array will point to locations 
        within the buffer defined in parameter #1 above.
     3) an integer that will return the number of items found.
     
   The routine below will modify each of these 3 parameters.
*/   
void	bpnm_endpointNamesGet(char * nameBuffer, char * nameArray [], int * numStrings)
{
    PsmPartition    ionwm = getIonwm();
    VScheme       * vscheme;
    VEndpoint     * vpoint;
    PsmAddress      schemeElt;
    PsmAddress      endpointElt;
    char            computedName [BPNM_ENDPOINT_EIDSTRING_LEN];
    char          * buffPtr;

    CHKVOID(nameBuffer);
    CHKVOID(nameArray);
    CHKVOID(numStrings);
    (* numStrings) = 0;

    /* The caller must supply the 2 buffers that will each return data. */
    if ( (nameBuffer == NULL) || (nameArray == NULL) )
    {
        return;
    }
    
    buffPtr = nameBuffer;
    for (schemeElt = sm_list_first(ionwm, (getBpVdb())->schemes); 
         schemeElt;
         schemeElt = sm_list_next(ionwm, schemeElt) )
    {
        vscheme = (VScheme *) psp(ionwm, sm_list_data(ionwm, schemeElt));

        for (endpointElt = sm_list_first(ionwm, vscheme->endpoints); 
             endpointElt;
             endpointElt = sm_list_next(ionwm, endpointElt))
        {
            vpoint = (VEndpoint *) psp(ionwm, sm_list_data(ionwm, endpointElt));

            /* form the Scheme Name and the NSS into the Endpoint ID in the form "ipn:4.2" */
            isprintf(computedName, sizeof(computedName), BPNM_ENDPOINT_NAME_FMT_STRING, 
                     vscheme->name, vpoint->nss);

            /* Move the name into the buffer supplied by the caller */
            memcpy (buffPtr, computedName, strlen (computedName) );
            nameArray [*numStrings] = buffPtr;
            buffPtr += strlen (computedName);
            
            /* Add the NULL char into the caller-supplied buffer. */
            * buffPtr = '\0';
            buffPtr++;
            (*numStrings)++;
        }
    }
}   /* end of bpnm_endpointNamesGet */


/*****************************************************************************/
/* This routine will examine the current ION node and return the VEndpoint
   for the endpoint requested.  The calling routine should provide:
     1) a string containing the name of the endpoint to be found.  It is 
        assumed that this name would have been returned to the caller via a  
        previous call to the function bpnm_endpointNamesGet (above).
     2) A buffer large enough to hold a pointer to the VEndpoint.
     3) an integer flag indicating whether the buffer contains data when returned.
        (0 = data not updated, 1 = data has been updated
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
             endpointElt;
             endpointElt = sm_list_next(ionwm, endpointElt))
        {
            *vpoint = (VEndpoint *) psp(ionwm, sm_list_data(ionwm, endpointElt));

            /* form the Scheme Name and the NSS into the Endpoint ID in the form "ipn:4.2" */
            isprintf(computedName, sizeof(computedName), BPNM_ENDPOINT_NAME_FMT_STRING, 
                     vscheme->name, (*vpoint)->nss);
                     
            /* Is the current EID string the one that you're looking for */
            if (strcmp (targetName, computedName) == 0)
            {
          
                * success = 1;
                return;
            }
        }
    }
}   /* end of getBpEndpointStats */

/*****************************************************************************/
void bpnm_endpoint_get (char * targetName, NmbpEndpoint * results, int * success)
{
    Sdr             sdr = getIonsdr();
    VEndpoint     * vpoint = NULL;
    Endpoint        endpoint;
    Object          elt;
    Bundle          bundle;
    EndpointStats   stats;

    CHKVOID(targetName);
    CHKVOID(results);
    CHKVOID(success);
    getBpEndpoint(targetName, & vpoint, success);
    
    if ( (* success) != 0)
    {
        /* DEBUGGING AID ONLY:  Useful for showing which fields have yet to be assigned. */
        memset_IncFillPat ( (char *) results, BPNM_ENDPOINT_FILLPATT, sizeof(NmbpEndpoint) );
        
        /* Copy extra byte to ensure that the NULL byte gets copied. */
        memcpy (results->eid, targetName, strlen (targetName)+1 );
    
        CHKVOID(sdr_begin_xn(sdr));
        sdr_read(sdr, (char *) & endpoint, sdr_list_data(sdr, vpoint->endpointElt), sizeof(Endpoint));
	results->currentQueuedBundlesCount = sdr_list_length(sdr, endpoint.deliveryQueue);
	results->currentQueuedBundlesBytes = 0;
	for (elt = sdr_list_first(sdr, endpoint.deliveryQueue); elt; elt = sdr_list_next(sdr, elt))
	{
            sdr_read(sdr, (char *) & bundle, sdr_list_data(sdr, elt), sizeof(Bundle));
	    results->currentQueuedBundlesBytes += bundle.payload.length;
	}

        sdr_read(sdr, (char *) & stats, endpoint.stats, sizeof(EndpointStats));
        sdr_exit_xn(sdr);

        results->lastResetTime             = stats.resetTime;

        results->bundleEnqueuedCount       = stats.tallies[BP_ENDPOINT_QUEUED   ].currentCount;
        results->bundleEnqueuedBytes       = stats.tallies[BP_ENDPOINT_QUEUED   ].currentBytes;
        results->bundleDequeuedCount       = stats.tallies[BP_ENDPOINT_DELIVERED].currentCount;
        results->bundleDequeuedBytes       = stats.tallies[BP_ENDPOINT_DELIVERED].currentBytes;
    }
    
}   /* end of bpnm_endpoint_get */

/*****************************************************************************/
/* This routine will examine the current ION node and zero the "current" counters
   associated with the ION INSTRUMENTATION for the indicated EndPoint.
   The calling routine should provide:
     1) a string containing the name of the endpoint to be searched.  It is 
        assumed that this name would have been returned to the caller via a  
        previous call to the function bpnm_endpointNamesGet (above).
     2) an integer flag indicating whether the counters have been zeroed.
        (0 = data not zeroed, 1 = data successfully zeroed
*/   

/*****************************************************************************/
void  bpnm_endpoint_reset (char * targetName, int * success)
{
    Sdr             sdr = getIonsdr();
    PsmPartition    ionwm = getIonwm();
    VScheme       * vscheme;
    VEndpoint     * vpoint;
    PsmAddress      schemeElt;
    PsmAddress      endpointElt;
    char            computedName [BPNM_ENDPOINT_EIDSTRING_LEN];
    EndpointStats   stats;
    Tally         * tally;
    int             tallyLoop;

    CHKVOID(targetName);
    CHKVOID(success);
    * success = 0;
    if (targetName == NULL)     /* The caller must supply the name to be found. */
    {
        return;
    }
    
    for (schemeElt = sm_list_first(ionwm, (getBpVdb())->schemes); 
         schemeElt;
         schemeElt = sm_list_next(ionwm, schemeElt) )
    {
        vscheme = (VScheme *) psp(ionwm, sm_list_data(ionwm, schemeElt));

        for (endpointElt = sm_list_first(ionwm, vscheme->endpoints); 
             endpointElt;
             endpointElt = sm_list_next(ionwm, endpointElt))
        {
            vpoint = (VEndpoint *) psp(ionwm, sm_list_data(ionwm, endpointElt));

            isprintf(computedName, sizeof(computedName), BPNM_ENDPOINT_NAME_FMT_STRING, 
                     vscheme->name, vpoint->nss);
                     
            if (strcmp (targetName, computedName) == 0)
            {
                CHKVOID(sdr_begin_xn(sdr));
                sdr_stage(sdr, (char *) & stats, vpoint->stats, sizeof(EndpointStats));

                stats.resetTime = getUTCTime();
                for (tallyLoop = 0; tallyLoop < BP_ENDPOINT_STATS; tallyLoop++)
                {
                    tally = stats.tallies + tallyLoop;
                    tally->currentCount = 0;
                    tally->currentBytes = 0;
                }

                sdr_write(sdr, vpoint->stats, (char *) & stats, sizeof(EndpointStats));
                oK(sdr_end_xn(sdr));
          
                * success = 1;
                return;
            }
        }
    }
}   /* end of bpnm_endpoint_reset */

/*	*	*	*	Inducts		*	*	*	*/

/*****************************************************************************/
/* This routine will examine the current ION node and return the names of all 
   of the inducts currently defined.  The calling routine should provide:
     1) one buffer large enough to hold all of the names (in string format).
        Each string will be stored sequentially in this buffer with the string's 
        NULL terminator separating each.
     2) an array of string pointers with enough entries to point to all of the 
        strings.  The pointers returned in this array will point to locations 
        within the buffer defined in parameter #1 above.
     3) an integer that will return the number of items found.
     
   The routine below will modify each of these 3 parameters.
*/   

/*****************************************************************************/
void bpnm_inductNames_get (char * nameBuffer, char * nameArray [], int * numStrings)
{
    Sdr             sdr = getIonsdr();
    PsmPartition    ionwm = getIonwm();
    ClProtocol      clpbuf;
    Object          protoElt;
    VInduct       * vduct;
    PsmAddress      ductElt;
    char            computedName [BPNM_INDUCT_NAME_LEN];
    char          * buffPtr;

    CHKVOID(nameBuffer);
    CHKVOID(nameArray);
    CHKVOID(numStrings);
    (* numStrings) = 0;

    /* The caller must supply the 2 buffers that will each return data. */
    if ( (nameBuffer == NULL) || (nameArray == NULL) )
    {
        return;
    }
    
    buffPtr = nameBuffer;
    for (protoElt = sdr_list_first(sdr, (getBpConstants())->protocols);
         protoElt; 
         protoElt = sdr_list_next(sdr, protoElt))
    {
        sdr_read(sdr, (char *) &clpbuf, sdr_list_data(sdr, protoElt), sizeof(ClProtocol));

        for (ductElt = sm_list_first(ionwm, (getBpVdb())->inducts); 
             ductElt; 
             ductElt = sm_list_next(ionwm, ductElt))
        {
            vduct = (VInduct *) psp(ionwm, sm_list_data(ionwm, ductElt));
            if (strcmp(vduct->protocolName, clpbuf.name) == 0)
            {
                /* form the Induct Name in the form protocol/duct_name" */
                isprintf(computedName, sizeof(computedName), BPNM_INDUCT_NAME_FMT_STRING, clpbuf.name, vduct->ductName);

                /* Move the name into the buffer supplied by the caller */
                memcpy (buffPtr, computedName, strlen (computedName) );
                nameArray [*numStrings] = buffPtr;
                buffPtr += strlen (computedName);
                
                /* Add the NULL char into the caller-supplied buffer. */
                * buffPtr = '\0';
                buffPtr++;
                (*numStrings)++;
            }
        }
    }
}   /* end of bpnm_inductNames_get */

/*****************************************************************************/
/* This routine will examine the current ION node and return the VInduct
   for the node requested.  The calling routine should provide:
     1) a string containing the name of the item to be found.  It is 
        assumed that this name would have been returned to the caller via a  
        previous call to the function bpnm_inductNames_get (above).
     2) A buffer large enough to hold a pointer to the VInduct.
     3) an integer flag that indicates whether the buffer contains data.
        (0 = data not updated, 1 = data has been updated
*/ 
static void getBpInductStats (char * targetName, VInduct ** vduct, int  * success)
{
    Sdr             sdr = getIonsdr();
    PsmPartition    ionwm = getIonwm();
    ClProtocol      clpbuf;
    Object          protoElt;
    PsmAddress      ductElt;
    char            computedName [BPNM_INDUCT_NAME_LEN];

    * success = 0;

    for (protoElt = sdr_list_first(sdr, (getBpConstants())->protocols);
         protoElt; 
         protoElt = sdr_list_next(sdr, protoElt))
    {
        sdr_read(sdr, (char *) &clpbuf, sdr_list_data(sdr, protoElt), sizeof(ClProtocol));

        for (ductElt = sm_list_first(ionwm, (getBpVdb())->inducts); 
             ductElt; 
             ductElt = sm_list_next(ionwm, ductElt))
        {
            *vduct = (VInduct *) psp(ionwm, sm_list_data(ionwm, ductElt));
            if (strcmp((*vduct)->protocolName, clpbuf.name) == 0)
            {
                /* form the Induct Name in the form protocol/duct_name" */
                isprintf(computedName, sizeof(computedName), BPNM_INDUCT_NAME_FMT_STRING, clpbuf.name, (*vduct)->ductName);

                if (strcmp (targetName, computedName) == 0)
                {
                    * success = 1;
                    return;
                }
            }
        }
    }
}   /* end of getBpInductStats */

/*****************************************************************************/
void bpnm_induct_get (char * targetName, NmbpInduct * results, int * success)
{
    Sdr             sdr = getIonsdr();
    VInduct       * vduct = NULL;
    Induct          duct;
    InductStats     stats;

    CHKVOID(targetName);
    CHKVOID(results);
    CHKVOID(success);
    getBpInductStats (targetName, & vduct, success);
    
    if ( (* success) != 0)
    {              
        /* DEBUGGING AID ONLY:  Useful for showing which fields have yet to be assigned. */
        memset_IncFillPat ( (char *) results, BPNM_INDUCT_FILLPATT, sizeof(NmbpInduct) );

        /* Copy extra byte to ensure that the NULL byte gets copied. */
        memcpy (results->inductName, targetName, strlen (targetName)+1 );
    
        CHKVOID(sdr_begin_xn(sdr));
        sdr_read(sdr, (char *) & duct, sdr_list_data(sdr, vduct->inductElt), sizeof(Induct));
        sdr_read(sdr, (char *) & stats, duct.stats, sizeof(InductStats));
        sdr_exit_xn(sdr);

        results->lastResetTime          = stats.resetTime;
        
        results->bundleRecvCount        = stats.tallies[BP_INDUCT_RECEIVED   ].currentCount;
        results->bundleRecvBytes        = stats.tallies[BP_INDUCT_RECEIVED   ].currentBytes;
        results->bundleMalformedCount   = stats.tallies[BP_INDUCT_MALFORMED  ].currentCount;
        results->bundleMalformedBytes   = stats.tallies[BP_INDUCT_MALFORMED  ].currentBytes;
        results->bundleInauthenticCount = stats.tallies[BP_INDUCT_INAUTHENTIC].currentCount;
        results->bundleInauthenticBytes = stats.tallies[BP_INDUCT_INAUTHENTIC].currentBytes;
        results->bundleOverflowCount    = stats.tallies[BP_INDUCT_CONGESTIVE ].currentCount;
        results->bundleOverflowBytes    = stats.tallies[BP_INDUCT_CONGESTIVE ].currentBytes;

    }
}   /* end of bpnm_induct_get */

/*****************************************************************************/
/* This routine will examine the current ION node and zero the "current" counters
   associated with the ION INSTRUMENTATION for the indicated Induct.
   The calling routine should provide:
     1) a string containing the name of the InDuct to be searched.  It is 
        assumed that this name would have been returned to the caller via a  
        previous call to the function bpnm_inductNames_get (above).
     2) an integer flag indicating whether the counters have been zeroed.
        (0 = data not zeroed, 1 = data successfully zeroed
*/   
void bpnm_induct_reset (char * targetName, int * success)
{
    Sdr             sdr = getIonsdr();
    PsmPartition    ionwm = getIonwm();
    ClProtocol      clpbuf;
    Object          protoElt;
    VInduct       * vduct;
    PsmAddress      ductElt;
    char            computedName [BPNM_INDUCT_NAME_LEN];
    InductStats     stats;
    Tally         * tally;
    int             tallyLoop;

    CHKVOID(targetName);
    CHKVOID(success);
    * success = 0;
    if (targetName == NULL)     /* The caller must supply the name to be found. */
    {
        return;
    }
    
    for (protoElt = sdr_list_first(sdr, (getBpConstants())->protocols);
         protoElt; 
         protoElt = sdr_list_next(sdr, protoElt))
    {
        sdr_read(sdr, (char *) &clpbuf, sdr_list_data(sdr, protoElt), sizeof(ClProtocol));

        for (ductElt = sm_list_first(ionwm, (getBpVdb())->inducts); 
             ductElt; 
             ductElt = sm_list_next(ionwm, ductElt))
        {
            vduct = (VInduct *) psp(ionwm, sm_list_data(ionwm, ductElt));
            if (strcmp(vduct->protocolName, clpbuf.name) == 0)
            {
                /* form the Induct Name in the form protocol/duct_name" */
                isprintf(computedName, sizeof(computedName), BPNM_INDUCT_NAME_FMT_STRING, clpbuf.name, vduct->ductName);

                if (strcmp (targetName, computedName) == 0)
                {
                    CHKVOID(sdr_begin_xn(sdr));
                    sdr_stage(sdr, (char *) & stats, vduct->stats, sizeof(InductStats));

                    stats.resetTime = getUTCTime();
                    for (tallyLoop = 0; tallyLoop < BP_INDUCT_STATS; tallyLoop++)
                    {
                        tally = stats.tallies + tallyLoop;
                        tally->currentCount = 0;
                        tally->currentBytes = 0;
                    }
    
                    sdr_write(sdr, vduct->stats, (char *) & stats, sizeof(InductStats));
                    oK(sdr_end_xn(sdr));
              
                    * success = 1;
                    return;
                }
            }
        }
    }
}   /* end of bpnm_induct_reset */

/*	*	*	*	Outducts	*	*	*	*/

/*****************************************************************************/
/* This routine will examine the current ION node and return the names of all 
   of the outducts currently defined.  The calling routine should provide:
     1) one buffer large enough to hold all of the names (in string format).
        Each string will be stored sequentially in this buffer with the string's 
        NULL terminator separating each.
     2) an array of string pointers with enough entries to point to all of the 
        strings.  The pointers returned in this array will point to locations 
        within the buffer defined in parameter #1 above.
     3) an integer that will return the number of items found.
     
   The routine below will modify each of these 3 parameters.
*/
void bpnm_outductNames_get (char * nameBuffer, char * nameArray [], int * numStrings)
{
    Sdr             sdr = getIonsdr();
    PsmPartition    ionwm = getIonwm();
    ClProtocol      clpbuf;
    Object          protoElt;
    VOutduct      * vduct;
    PsmAddress      ductElt;
    char            computedName [BPNM_OUTDUCT_NAME_LEN];
    char          * buffPtr;

    CHKVOID(nameBuffer);
    CHKVOID(nameArray);
    CHKVOID(numStrings);
    * numStrings = 0;

    /* The caller must supply the 2 buffers that will each return data. */
    if ( (nameBuffer == NULL) || (nameArray == NULL) )
    {
        return;
    }
    
    buffPtr = nameBuffer;
    for (protoElt = sdr_list_first(sdr, (getBpConstants())->protocols);
         protoElt; 
         protoElt = sdr_list_next(sdr, protoElt))
    {
        sdr_read(sdr, (char *) &clpbuf, sdr_list_data(sdr, protoElt), sizeof(ClProtocol));

        for (ductElt = sm_list_first(ionwm, (getBpVdb())->outducts); 
             ductElt; 
             ductElt = sm_list_next(ionwm, ductElt))
        {
            vduct = (VOutduct *) psp(ionwm, sm_list_data(ionwm, ductElt));
            if (strcmp(vduct->protocolName, clpbuf.name) == 0)
            {
                /* form the Induct Name in the form protocol/duct_name" */
                isprintf(computedName, sizeof(computedName), BPNM_OUTDUCT_NAME_FMT_STRING, clpbuf.name, vduct->ductName);

                /* Move the name into the buffer supplied by the caller */
                memcpy (buffPtr, computedName, strlen (computedName) );
                nameArray [*numStrings] = buffPtr;
                buffPtr += strlen (computedName);
                
                /* Add the NULL char into the caller-supplied buffer. */
                * buffPtr = '\0';
                buffPtr++;
                (*numStrings)++;
            }
        }
    }
}   /* end of bpnm_outductNames_get */

/*****************************************************************************/
/* This routine will examine the current ION node and return the VOutduct
   for the node requested.  The calling routine should provide:
     1) a string containing the name of the item to be found.  It is 
        assumed that this name would have been returned to the caller via a  
        previous call to the function bpnm_outductNames_get (above).
     2) A buffer large enough to hold a pointer to the VOutduct.
     3) an integer flag that indicates whether the buffer contains data.
        (0 = data not updated, 1 = data has been updated
*/   
static void getBpOutductStats (char * targetName, VOutduct ** vduct, int  * success)
{
    Sdr             sdr = getIonsdr();
    PsmPartition    ionwm = getIonwm();
    ClProtocol      clpbuf;
    Object          protoElt;
    PsmAddress      ductElt;
    char            computedName [BPNM_OUTDUCT_NAME_LEN];

    * success = 0;

    for (protoElt = sdr_list_first(sdr, (getBpConstants())->protocols);
         protoElt; 
         protoElt = sdr_list_next(sdr, protoElt))
    {
        sdr_read(sdr, (char *) &clpbuf, sdr_list_data(sdr, protoElt), sizeof(ClProtocol));

        for (ductElt = sm_list_first(ionwm, (getBpVdb())->outducts); 
             ductElt; 
             ductElt = sm_list_next(ionwm, ductElt))
        {
            *vduct = (VOutduct *) psp(ionwm, sm_list_data(ionwm, ductElt));
            if (strcmp((*vduct)->protocolName, clpbuf.name) == 0)
            {
                /* form the Induct Name in the form protocol/duct_name" */
                isprintf(computedName, sizeof(computedName), BPNM_OUTDUCT_NAME_FMT_STRING, clpbuf.name, (*vduct)->ductName);

                if (strcmp (targetName, computedName) == 0)
                {
                    * success = 1;
                    return;
                }
            }
        }
    }
}   /* end of getBpOutductStats */

/*****************************************************************************/
void bpnm_outduct_get (char * targetName, NmbpOutduct * results, int * success)
{
    Sdr             sdr = getIonsdr();
    VOutduct      * vduct = NULL;
    Outduct         duct;
    OutductStats    stats;
    
    CHKVOID(targetName);
    CHKVOID(results);
    CHKVOID(success);
    getBpOutductStats (targetName, & vduct, success);

    if ( (* success) != 0)
    {
        /* DEBUGGING AID ONLY:  Useful for showing which fields have yet to be assigned. */
        memset_IncFillPat ( (char *) results, BPNM_OUTDUCT_FILLPATT, sizeof(NmbpOutduct) );
        
        /* Copy extra byte to ensure that the NULL byte gets copied. */
        memcpy (results->outductName, targetName, strlen (targetName)+1 );

        CHKVOID(sdr_begin_xn(sdr));
        sdr_read(sdr, (char *) & duct, sdr_list_data(sdr, vduct->outductElt), sizeof(Outduct));
        results->currentQueuedBundlesCount =
                sdr_list_length(sdr, duct.urgentQueue)
                + sdr_list_length(sdr, duct.stdQueue)
		+ sdr_list_length(sdr, duct.bulkQueue);
        results->currentQueuedBundlesBytes =
		(ONE_GIG * duct.urgentBacklog.gigs) + duct.urgentBacklog.units
		+ (ONE_GIG * duct.stdBacklog.gigs) + duct.stdBacklog.units
		+ (ONE_GIG * duct.bulkBacklog.gigs) + duct.bulkBacklog.units;
        sdr_read(sdr, (char *) & stats, duct.stats, sizeof(OutductStats));
        sdr_exit_xn(sdr);

        results->lastResetTime       = stats.resetTime;

        results->bundleEnqueuedCount = stats.tallies[BP_OUTDUCT_ENQUEUED].currentCount;
        results->bundleEnqueuedBytes = stats.tallies[BP_OUTDUCT_ENQUEUED].currentBytes;
        results->bundleDequeuedCount = stats.tallies[BP_OUTDUCT_DEQUEUED].currentCount;
        results->bundleDequeuedBytes = stats.tallies[BP_OUTDUCT_DEQUEUED].currentBytes;
    }
}   /* end of bpnm_outduct_get */

/*****************************************************************************/
/* This routine will examine the current ION node and zero the "current" counters
   associated with the ION INSTRUMENTATION for the indicated Outducts.
   The calling routine should provide:
     1) a string containing the name of the outduct to be searched.  It is 
        assumed that this name would have been returned to the caller via a  
        previous call to the function bpnm_outductNames_get (above).
     2) an integer flag indicating whether the counters have been zeroed.
        (0 = data not zeroed, 1 = data successfully zeroed
*/   
void bpnm_outduct_reset (char * targetName, int * success)
{
    Sdr             sdr = getIonsdr();
    PsmPartition    ionwm = getIonwm();
    ClProtocol      clpbuf;
    Object          protoElt;
    VOutduct      * vduct;
    PsmAddress      ductElt;
    char            computedName [BPNM_OUTDUCT_NAME_LEN];
    OutductStats    stats;
    Tally         * tally;
    int             tallyLoop;

    CHKVOID(targetName);
    CHKVOID(success);
    * success = 0;
    if (targetName == NULL)     /* The caller must supply the name to be found. */
    {
        return;
    }
    
    for (protoElt = sdr_list_first(sdr, (getBpConstants())->protocols);
         protoElt; 
         protoElt = sdr_list_next(sdr, protoElt))
    {
        sdr_read(sdr, (char *) &clpbuf, sdr_list_data(sdr, protoElt), sizeof(ClProtocol));

        for (ductElt = sm_list_first(ionwm, (getBpVdb())->outducts); 
             ductElt; 
             ductElt = sm_list_next(ionwm, ductElt))
        {
            vduct = (VOutduct *) psp(ionwm, sm_list_data(ionwm, ductElt));
            if (strcmp(vduct->protocolName, clpbuf.name) == 0)
            {
                /* form the Outduct Name in the form protocol/duct_name" */
                isprintf(computedName, sizeof(computedName), BPNM_OUTDUCT_NAME_FMT_STRING, clpbuf.name, vduct->ductName);

                if (strcmp (targetName, computedName) == 0)
                {
                    CHKVOID(sdr_begin_xn(sdr));
                    sdr_stage(sdr, (char *) & stats, vduct->stats, sizeof(OutductStats));

                    stats.resetTime = getUTCTime();
                    for (tallyLoop = 0; tallyLoop < BP_OUTDUCT_STATS; tallyLoop++)
                    {
                        tally = stats.tallies + tallyLoop;
                        tally->currentCount = 0;
                        tally->currentBytes = 0;
                    }
    
                    sdr_write(sdr, vduct->stats, (char *) & stats, sizeof(OutductStats));
                    oK(sdr_end_xn(sdr));
              
                    * success = 1;
                    return;
                }
            }
        }
    }
}   /* end of bpnm_outduct_reset */

/*	*	*	Database, including limbo	*	*	*/

/*****************************************************************************/
void    bpnm_limbo_get(NmbpOutduct * results)
{
    Sdr             sdr = getIonsdr();
    Object          bpDbObject = getBpDbObject();
    BpDB            bpdb;
    Object          elt;
    Bundle          bundle;
    BpDbStats       dbStats;

    CHKVOID(results);
    istrcpy(results->outductName, "limbo", BPNM_OUTDUCT_NAME_LEN);
    CHKVOID(sdr_begin_xn(sdr));
    sdr_read(sdr, (char *) &bpdb, bpDbObject, sizeof(BpDB));
    results->currentQueuedBundlesCount = sdr_list_length(sdr, bpdb.limboQueue);
    results->currentQueuedBundlesBytes = 0;
    for (elt = sdr_list_first(sdr, bpdb.limboQueue); elt;
            elt = sdr_list_next(sdr, elt))
    {
	    sdr_read(sdr, (char *) &bundle, sdr_list_data(sdr, elt),
			    sizeof(Bundle));
	    results->currentQueuedBundlesBytes += bundle.payload.length;
    }

    results->lastResetTime = bpdb.resetTime;
    sdr_read(sdr, (char *) &dbStats, bpdb.dbStats, sizeof(BpDbStats));
    results->bundleEnqueuedCount
	    = dbStats.tallies[BP_DB_TO_LIMBO].currentCount;
    results->bundleEnqueuedBytes
	    = dbStats.tallies[BP_DB_TO_LIMBO].currentBytes;
    results->bundleDequeuedCount
	    = dbStats.tallies[BP_DB_FROM_LIMBO].currentCount;
    results->bundleDequeuedBytes
	    = dbStats.tallies[BP_DB_FROM_LIMBO].currentBytes;

    sdr_exit_xn(sdr);

}   /* end of bpnm_limbo_get */

/*****************************************************************************/
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
    ClProtocol      protocol;
    Outduct         duct;
    BpRptStats      rptStats;
    BpCtStats       ctStats;
    BpDbStats       dbStats;

    CHKVOID(results);
    CHKVOID(sdr_begin_xn(sdr));
    sdr_read(sdr, (char *) &bpdb, bpDbObject, sizeof(BpDB));
    results->lastResetTime = bpdb.resetTime;

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
    	results->bundleRecvCount[i] = tally->currentCount;
    	results->bundleRecvBytes[i] = tally->currentBytes;
        results->currentResidentCount[i] += tally->totalCount;
        results->currentResidentBytes[i] += tally->totalBytes;
    }

    sdr_read(sdr, (char *) &cosStats, bpdb.discardStats, sizeof(BpCosStats));
    for (i = 0, tally = cosStats.tallies; i < 3; i++, tally++)
    {
    	results->bundleDiscardCount[i] = tally->currentCount;
    	results->bundleDiscardBytes[i] = tally->currentBytes;
        results->currentResidentCount[i] -= tally->totalCount;
        results->currentResidentBytes[i] -= tally->totalBytes;
    }

    sdr_read(sdr, (char *) &cosStats, bpdb.xmitStats, sizeof(BpCosStats));
    for (i = 0, tally = cosStats.tallies; i < 3; i++, tally++)
    {
    	results->bundleXmitCount[i] = tally->currentCount;
    	results->bundleXmitBytes[i] = tally->currentBytes;
    }

    results->currentInLimbo = sdr_list_length(sdr, bpdb.limboQueue);
    results->currentDispatchPending = 0;
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
	        sdr_read(sdr, (char *) & endpoint, sdr_list_data(sdr, elt),
                        sizeof(Endpoint));
                results->currentReassemblyPending
		        += sdr_list_length(sdr, endpoint.incompletes);
	    }
    }

    results->currentForwardPending = results->currentInLimbo;
    for (elt = sdr_list_first(sdr, bpdb.protocols); elt;
            elt = sdr_list_next(sdr, elt))
    {
	    sdr_read(sdr, (char *) & protocol, sdr_list_data(sdr, elt),
                    sizeof(ClProtocol));
	    for (elt2 = sdr_list_first(sdr, protocol.outducts); elt2;
                    elt2 = sdr_list_next(sdr, elt2))
	    {
	        sdr_read(sdr, (char *) & duct, sdr_list_data(sdr, elt),
                        sizeof(Outduct));
                results->currentForwardPending
		        += sdr_list_length(sdr, duct.urgentQueue);
                results->currentForwardPending
		        += sdr_list_length(sdr, duct.stdQueue);
                results->currentForwardPending
		        += sdr_list_length(sdr, duct.bulkQueue);
	    }
    }

    sdr_read(sdr, (char *) &rptStats, bpdb.rptStats, sizeof(BpRptStats));

    results->rptReceiveCount = rptStats.currentRptByStatus[BP_STATUS_RECEIVE];
    results->rptAcceptCount = rptStats.currentRptByStatus[BP_STATUS_ACCEPT];
    results->rptForwardCount = rptStats.currentRptByStatus[BP_STATUS_FORWARD];
    results->rptDeliverCount = rptStats.currentRptByStatus[BP_STATUS_DELIVER];
    results->rptDeleteCount = rptStats.currentRptByStatus[BP_STATUS_DELETE];

    results->rptNoneCount
	    = rptStats.currentRptByReason[BP_REASON_NONE];
    results->rptExpiredCount
	    = rptStats.currentRptByReason[BP_REASON_EXPIRED];
    results->rptFwdUnidirCount =
	    rptStats.currentRptByReason[BP_REASON_FWD_UNIDIR];
    results->rptCanceledCount
	    = rptStats.currentRptByReason[BP_REASON_CANCELED];
    results->rptDepletionCount
	    = rptStats.currentRptByReason[BP_REASON_DEPLETION];
    results->rptEidMalformedCount
	    = rptStats.currentRptByReason[BP_REASON_EID_MALFORMED];
    results->rptNoRouteCount
	    = rptStats.currentRptByReason[BP_REASON_NO_ROUTE];
    results->rptNoContactCount
	    = rptStats.currentRptByReason[BP_REASON_NO_CONTACT];
    results->rptBlkMalformedCount
	    = rptStats.currentRptByReason[BP_REASON_BLK_MALFORMED];

    sdr_read(sdr, (char *) &ctStats, bpdb.ctStats, sizeof(BpCtStats));
    results->custodyAcceptCount
	    = ctStats.tallies[BP_CT_CUSTODY_ACCEPTED].currentCount;
    results->custodyAcceptBytes
	    = ctStats.tallies[BP_CT_CUSTODY_ACCEPTED].currentBytes;
    results->custodyReleasedCount
	    = ctStats.tallies[BP_CT_CUSTODY_RELEASED].currentCount;
    results->custodyReleasedBytes
	    = ctStats.tallies[BP_CT_CUSTODY_RELEASED].currentBytes;
    results->custodyExpiredCount
	    = ctStats.tallies[BP_CT_CUSTODY_EXPIRED].currentCount;
    results->custodyExpiredBytes
	    = ctStats.tallies[BP_CT_CUSTODY_EXPIRED].currentBytes;
    results->custodyRedundantCount
	    = ctStats.tallies[BP_CT_REDUNDANT].currentCount;
    results->custodyRedundantBytes
	    = ctStats.tallies[BP_CT_REDUNDANT].currentBytes;
    results->custodyDepletionCount
	    = ctStats.tallies[BP_CT_DEPLETION].currentCount;
    results->custodyDepletionBytes
	    = ctStats.tallies[BP_CT_DEPLETION].currentBytes;
    results->custodyEidMalformedCount
	    = ctStats.tallies[BP_CT_EID_MALFORMED].currentCount;
    results->custodyEidMalformedBytes
	    = ctStats.tallies[BP_CT_EID_MALFORMED].currentBytes;
    results->custodyNoRouteCount
	    = ctStats.tallies[BP_CT_NO_ROUTE].currentCount;
    results->custodyNoRouteBytes
	    = ctStats.tallies[BP_CT_NO_ROUTE].currentBytes;
    results->custodyNoContactCount
	    = ctStats.tallies[BP_CT_NO_CONTACT].currentCount;
    results->custodyNoContactBytes
	    = ctStats.tallies[BP_CT_NO_CONTACT].currentBytes;
    results->custodyBlkMalformedCount
	    = ctStats.tallies[BP_CT_BLK_MALFORMED].currentCount;
    results->custodyBlkMalformedBytes
	    = ctStats.tallies[BP_CT_BLK_MALFORMED].currentBytes;

    results->currentInCustody
	    = ctStats.tallies[BP_CT_CUSTODY_ACCEPTED].totalCount
	    - (ctStats.tallies[BP_CT_CUSTODY_RELEASED].totalCount
	    +  ctStats.tallies[BP_CT_CUSTODY_EXPIRED].totalCount);
    results->currentNotInCustody
	    = (results->currentResidentCount[0]
            + results->currentResidentCount[1]
	    + results->currentResidentCount[2])
	    - results->currentInCustody;

    sdr_read(sdr, (char *) &dbStats, bpdb.dbStats, sizeof(BpDbStats));
    results->bundleQueuedForFwdCount
	    = dbStats.tallies[BP_DB_QUEUED_FOR_FWD].currentCount;
    results->bundleQueuedForFwdBytes
	    = dbStats.tallies[BP_DB_QUEUED_FOR_FWD].currentBytes;
    results->bundleFwdOkayCount
	    = dbStats.tallies[BP_DB_FWD_OKAY].currentCount;
    results->bundleFwdOkayBytes
	    = dbStats.tallies[BP_DB_FWD_OKAY].currentBytes;
    results->bundleFwdFailedCount
	    = dbStats.tallies[BP_DB_FWD_FAILED].currentCount;
    results->bundleFwdFailedBytes
	    = dbStats.tallies[BP_DB_FWD_FAILED].currentBytes;
    results->bundleRequeuedForFwdCount
	    = dbStats.tallies[BP_DB_REQUEUED_FOR_FWD].currentCount;
    results->bundleRequeuedForFwdBytes
	    = dbStats.tallies[BP_DB_REQUEUED_FOR_FWD].currentBytes;
    results->bundleExpiredCount
	    = dbStats.tallies[BP_DB_EXPIRED].currentCount;
    results->bundleExpiredBytes
	    = dbStats.tallies[BP_DB_EXPIRED].currentBytes;

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
static void    resetRptStats(BpDB *db)
{
    Sdr             sdr = getIonsdr();
    BpRptStats      stats;
    int             i;

    sdr_stage(sdr, (char *) &stats, db->rptStats, sizeof(BpRptStats));
    for (i = 0; i < BP_STATUS_STATS; i++)
    {
        stats.currentRptByStatus[i] = 0;
    }

    for (i = 0; i < BP_REASON_STATS; i++)
    {
        stats.currentRptByReason[i] = 0;
    }

    sdr_write(sdr, db->rptStats, (char *) &stats, sizeof(BpRptStats));

}   /* end of resetRptStats */

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
    db.resetTime = getUTCTime();
    resetSourceStats(&db);
    resetRecvStats(&db);
    resetDiscardStats(&db);
    resetXmitStats(&db);
    resetRptStats(&db);
    resetCtStats(&db);
    resetDbStats(&db);
    sdr_write(sdr, dbobj, (char *) &db, sizeof(BpDB));
    oK(sdr_end_xn(sdr));
    
}   /* end of bpnm_disposition_reset */
