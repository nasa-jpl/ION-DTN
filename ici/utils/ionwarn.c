/*

	ionwarn.c:	utility for computing congestion forecast
			at an ION node, based on contact plan.

	Author:	Scott Burleigh, JPL

	Optimization of original congestion forecasting code was
	performed by Greg Menke of Goddard Space Flight Center
	in 2011.

	Copyright (c) 2012, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

									*/
#include "zco.h"
#include "rfx.h"
#include "lyst.h"
#include <limits.h>

typedef struct
{
	time_t	time;
	int	prevXmitRate;
	int	xmitRate;
	int	fromNeighbor;		/*	Boolean.		*/
} RateChange;

static char	*_cannotForecast()
{
	return "Can't complete congestion forecast.";
}

static IonNeighbor	*retrieveNeighbor(uvast nodeNbr, Lyst neighbors)
{
	LystElt		elt3;
	IonNeighbor	*np = NULL;

	for (elt3 = lyst_first(neighbors); elt3; elt3 = lyst_next(elt3))
	{
		np = (IonNeighbor *) lyst_data(elt3);
		if (np->nodeNbr == nodeNbr)
		{
			break;
		}
	}

	if (elt3 == NULL)		/*	This is a new neighbor.	*/
	{
		np = (IonNeighbor *) MTAKE(sizeof(IonNeighbor));
		if (np == NULL)
		{
			putErrmsg(_cannotForecast() , NULL);
			return NULL;
		}

		memset((char *) np, 0, sizeof(IonNeighbor));
		np->nodeNbr = nodeNbr;
		if (lyst_insert_last(neighbors, np) == NULL)
		{
			putErrmsg(_cannotForecast() , NULL);
			return NULL;
		}
	}

	return np;
}

static int	insertRateChange(time_t time, unsigned int xmitRate,
			int fromNeighbor, unsigned int prevXmitRate,
			Lyst changes)
{
	RateChange	*newChange;
	LystElt		elt4;
	RateChange	*change;

	newChange = (RateChange *) MTAKE(sizeof(RateChange));
	if (newChange == NULL)
	{
		putErrmsg(_cannotForecast() , NULL);
		return -1;			/*	Out of memory.	*/
	}

	newChange->time = time;
	newChange->xmitRate = xmitRate;
	newChange->fromNeighbor = fromNeighbor;	/*	Boolean.	*/
	newChange->prevXmitRate = prevXmitRate;
	for (elt4 = lyst_last(changes); elt4; elt4 = lyst_prev(elt4))
	{
		change = (RateChange *) lyst_data(elt4);
		if (change->time <= newChange->time)
		{
			break;
		}
	}

	if (elt4)
	{
		elt4 = lyst_insert_after(elt4, newChange);
	}
	else
	{
		elt4 = lyst_insert_first(changes, newChange);
	}

	if (elt4 == NULL)
	{
		putErrmsg(_cannotForecast() , NULL);
		return -1;			/*	Out of memory.	*/
	}

	return 0;
}

int	checkForCongestion()
{
	time_t		forecastTime;
	Sdr		sdr;
	Object		iondbObj;
	IonDB		iondb;
	PsmPartition	ionwm;
	int		ionMemIdx;
	Lyst		neighbors;
	Lyst		changes;
	double		fileOccupancy;
	double		heapOccupancy;
	double		currentOccupancy;
	double		maxOccupancy;
	double		forecastOccupancy;
	double		netDomesticGrowth;
	double		netInTransitGrowth;
	IonVdb		*ionvdb;
	double		netGrowth;
	PsmAddress	elt1;
	IonNeighbor	*neighbor;
	PsmAddress	elt2;
	PsmAddress	addr;
	IonEvent	*event;
	IonCXref	*contact;
	LystElt		elt3;
	IonNeighbor	*np = NULL;
	LystElt		elt4;
	RateChange	*change;
	unsigned long	secInEpoch;
	double		spaceRemaining;
	double		secUntilOutOfSpace;
	unsigned long	secAdvanced;
	double		increment;
	time_t		alarmTime = 0;
	double		delta;
	char		timestampBuffer[TIMESTAMPBUFSZ];
	char		alarmBuffer[40 + TIMESTAMPBUFSZ];
	int		result;

	forecastTime = getCtime();
	sdr = getIonsdr();
	CHKZERO(sdr_begin_xn(sdr));
	iondbObj = getIonDbObject();
	sdr_stage(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	if (iondb.horizon != 0 && iondb.horizon <= forecastTime)
	{
		writeMemo("[i] No congestion forecast; horizon reached.");
		sdr_exit_xn(sdr);
		return 0;
	}

	ionwm = getIonwm();
	ionMemIdx = getIonMemoryMgr();
	neighbors = lyst_create_using(ionMemIdx);
	changes = lyst_create_using(ionMemIdx);
	if (neighbors == NULL || changes == NULL)
	{
		putErrmsg(_cannotForecast() , NULL);
		return -1;		/*	Out of memory.		*/
	}

	/*	First get current occupancy (both file space and heap).	*/

	fileOccupancy = zco_get_file_occupancy(sdr, ZcoOutbound);
	heapOccupancy = zco_get_heap_occupancy(sdr, ZcoOutbound);
	currentOccupancy = fileOccupancy + heapOccupancy;
 	forecastOccupancy = maxOccupancy = currentOccupancy;

	/*	Get net domestic contribution to congestion.		*/

	netDomesticGrowth = 0;		/*	Default: no activity.	*/
	if (iondb.productionRate > 0)
	{
		netDomesticGrowth += iondb.productionRate;
	}

	if (iondb.consumptionRate > 0)
	{
		netDomesticGrowth -= iondb.consumptionRate;
	}

	/*	Get current net in-transit contribution to congestion.	*/

	netInTransitGrowth = 0.0;
	ionvdb = getIonVdb();
	for (elt1 = sm_rbt_first(ionwm, ionvdb->neighbors); elt1;
			elt1 = sm_rbt_next(ionwm, elt1))
	{
		neighbor = (IonNeighbor*) psp(ionwm, sm_rbt_data(ionwm, elt1));
		if (neighbor == NULL)
		{
			putErrmsg("Corrupted neighbors list.", NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}

		netInTransitGrowth += neighbor->recvRate;
		netInTransitGrowth -= neighbor->xmitRate;
		np = (IonNeighbor *) MTAKE(sizeof(IonNeighbor));
		if (np == NULL)
		{
			putErrmsg(_cannotForecast() , NULL);
			sdr_cancel_xn(sdr);
			return -1;	/*	Out of memory.		*/
		}

		memcpy((char *) np, (char *) neighbor, sizeof(IonNeighbor));
		if (lyst_insert_last(neighbors, np) == NULL)
		{
			putErrmsg(_cannotForecast() , NULL);
			sdr_cancel_xn(sdr);
			return -1;	/*	Out of memory.		*/
		}
	}

	/*	Have now got *current* occupancy and growth rate.
	 *	Next, extract list of all relevant growth rate changes.	*/

	netGrowth = netDomesticGrowth + netInTransitGrowth;
	for (elt2 = sm_rbt_first(ionwm, ionvdb->timeline); elt2;
			elt2 = sm_rbt_next(ionwm, elt2))
	{
		addr = sm_rbt_data(ionwm, elt2);
		event = (IonEvent *) psp(ionwm, addr);
		if (event->time < forecastTime)
		{
			continue;	/*	Happened in the past.	*/
		}

		/*	Start of transmission?				*/

		if (event->type == IonStartXmit)
		{
			contact = (IonCXref *) psp(ionwm, event->ref);
			if (contact->fromNode == contact->toNode)
			{
				/*	This is a loopback contact,
				 *	which has no net effect on
				 *	congestion.  Ignore it.		*/

				continue;
			}
			
			if (contact->fromNode != iondb.ownNodeNbr)
			{
				continue;	/*	Not relevant.	*/
			}

			/*	Find affected neighbor; add if nec.	*/

			np = retrieveNeighbor(contact->toNode, neighbors);
			if (np == NULL)
			{
				sdr_cancel_xn(sdr);
				return -1;	/*	Out of memory.	*/
			}

			if (insertRateChange(event->time, contact->xmitRate,
					0, np->xmitRate, changes) < 0)
			{
				sdr_cancel_xn(sdr);
				return -1;	/*	Out of memory.	*/
			}

			np->xmitRate = contact->xmitRate;
			continue;
		}

		/*	End of transmission?				*/

		if (event->type == IonStopXmit)
		{
			contact = (IonCXref *) psp(ionwm, event->ref);
			if (contact->fromNode == contact->toNode)
			{
				/*	This is a loopback contact,
				 *	which has no net effect on
				 *	congestion.  Ignore it.		*/

				continue;
			}
			
			if (contact->fromNode != iondb.ownNodeNbr)
			{
				continue;	/*	Not relevant.	*/
			}

			/*	Find affected neighbor; add if nec.	*/

			np = retrieveNeighbor(contact->toNode, neighbors);
			if (np == NULL)
			{
				sdr_cancel_xn(sdr);
				return -1;	/*	Out of memory.	*/
			}

			if (insertRateChange(event->time, 0,
					0, np->xmitRate, changes) < 0)
			{
				sdr_cancel_xn(sdr);
				return -1;	/*	Out of memory.	*/
			}

			np->xmitRate = 0;
			continue;
		}

		/*	Start of reception?				*/

		if (event->type == IonStartRecv)
		{
			contact = (IonCXref *) psp(ionwm, event->ref);
			if (contact->fromNode == contact->toNode)
			{
				/*	This is a loopback contact,
				 *	which has no net effect on
				 *	congestion.  Ignore it.		*/

				continue;
			}
			
			if (contact->toNode != iondb.ownNodeNbr)
			{
				continue;	/*	Not relevant.	*/
			}

			/*	Find affected neighbor; add if nec.	*/

			np = retrieveNeighbor(contact->fromNode, neighbors);
			if (np == NULL)
			{
				sdr_cancel_xn(sdr);
				return -1;	/*	Out of memory.	*/
			}

			if (insertRateChange(event->time, contact->xmitRate,
					1, np->recvRate, changes) < 0)
			{
				sdr_cancel_xn(sdr);
				return -1;	/*	Out of memory.	*/
			}

			np->recvRate = contact->xmitRate;
			continue;
		}

		/*	End of reception?				*/

		if (event->type == IonStopRecv)
		{
			contact = (IonCXref *) psp(ionwm, event->ref);
			if (contact->fromNode == contact->toNode)
			{
				/*	This is a loopback contact,
				 *	which has no net effect on
				 *	congestion.  Ignore it.		*/

				continue;
			}
			
			if (contact->toNode != iondb.ownNodeNbr)
			{
				continue;	/*	Not relevant.	*/
			}

			/*	Find affected neighbor; add if nec.	*/

			np = retrieveNeighbor(contact->fromNode, neighbors);
			if (np == NULL)
			{
				sdr_cancel_xn(sdr);
				return -1;	/*	Out of memory.	*/
			}

			if (insertRateChange(event->time, 0,
					1, np->recvRate, changes) < 0)
			{
				sdr_cancel_xn(sdr);
				return -1;	/*	Out of memory.	*/
			}

			np->recvRate = 0;
			continue;
		}
	}

	/*	Now revise occupancy level over time as growth occurs
	 *	and growth rates change.  Occupancy computation
	 *	optimized by Greg Menke 22 September 2011.		*/

	for (elt4 = lyst_first(changes); elt4; elt4 = lyst_next(elt4))
	{
		change = (RateChange *) lyst_data(elt4);

		/*	Let occupancy level change per current rate
		 *	up to time of next rate change.
		 *
		 *	NOTE: change->time can never be less than
		 *	forecastTime because contacts that started
		 *	in the past are reflected in the initial
		 *	netGrowth; the start of such a contact
		 *	is never posted to the list of RateChanges. 	*/

		if (change->time < forecastTime)
		{
			putErrmsg("Investigate congestion check error.",
					utoa(change->time));
			continue;
		}

		if (iondb.horizon > 0 && iondb.horizon < change->time) 
		{
			secInEpoch = iondb.horizon - forecastTime;
		}
		else
		{
			secInEpoch = change->time - forecastTime;
		}

		if (netGrowth > 0)
		{
			/*	net growth > 0 means we are receiving
			 *	more than transmitting.  Project the
			 *	rate out till the forecast reaches the
			 *	occupancy ceiling (rounding up to
			 *	ensure the violation is detected later)
			 *	or we reach the end of the epoch,
			 *	whichever occurs first.			*/

			spaceRemaining = iondb.occupancyCeiling
					- forecastOccupancy;
			secUntilOutOfSpace = (spaceRemaining / netGrowth)
					+ .5;
			if (secInEpoch < secUntilOutOfSpace)
			{
				secAdvanced = secInEpoch;
			}
			else
			{
				secAdvanced = secUntilOutOfSpace;
			}
		}
		else	/*	netGrowth is <= 0			*/
		{
			/*	Note: net growth < 0 means we are
			 *	transmitting more than receiving.
			 *	Although this rate of growth is the
			 *	one that is theoretically correct
			 *	throughout the epoch, the reduction
			 *	in occupancy will obviously stop
			 *	when occupancy reaches zero; from
			 *	that time until the end of the epoch
			 *	the theoretical negative net growth
			 *	in fact represents unused transmission
			 *	capacity.  We correct for this by
			 *	changing forecastOccupancy to zero
			 *	whenever it is driven negative.		*/

			secAdvanced = secInEpoch;
		}

		/*	Note change in forecast total occupancy,
		 *	adjust high-water mark as necessary.		*/

		increment = netGrowth * secAdvanced;
		forecastOccupancy += increment;
		if (forecastOccupancy < 0)
		{
			forecastOccupancy = 0;
		}

		if (forecastOccupancy > maxOccupancy)
		{
			maxOccupancy = forecastOccupancy;
		}

		/*	Advance the forecast time, by epoch or to end,
		 *	and check for congestion alarm.			*/

		forecastTime += secAdvanced;
		if (maxOccupancy >= iondb.occupancyCeiling)
		{
			alarmTime = forecastTime;
		}

		if (alarmTime != 0)	/*	Time of collapse known.	*/
		{
			break;		/*	Done forecasting.	*/
		}

		if (forecastTime > iondb.horizon)
		{
			break;		/*	Done forecasting.	*/
		}

		/*	Apply the adjustment that occurs at the time
		 *	of this change (the end of the prior epoch).	*/

		delta = change->xmitRate - change->prevXmitRate;
		if (change->fromNeighbor)
		{
			netGrowth += delta;
			netInTransitGrowth += delta;
		}
		else
		{
			netGrowth -= delta;
			netInTransitGrowth -= delta;
		}
	}

	/*	Have determined final net growth rate as of end of
	 *	last scheduled contact.					*/

	if (netGrowth > 0 && alarmTime == 0)
	{
		/*	Unconstrained growth; will max out eventually,
		 *	just need to determine when.  Final epoch.	*/

		if (iondb.horizon > 0) 
		{
			secInEpoch = iondb.horizon - forecastTime;
		}
		else
		{
			secInEpoch = LONG_MAX;
		}

		/*	net growth > 0 means we are receiving more
		 *	than transmitting.  Project the rate out till
		 *	the forecast reaches the occupancy ceiling
		 *	(rounding up to ensure the violation is
		 *	detected later) or we reach the end of the
		 *	epoch, whichever occurs first.			*/

		spaceRemaining = iondb.occupancyCeiling - forecastOccupancy;
		secUntilOutOfSpace = (spaceRemaining / netGrowth) + .5;
		if (secInEpoch < secUntilOutOfSpace)
		{
			secAdvanced = secInEpoch;
		}
		else
		{
			secAdvanced = secUntilOutOfSpace;
		}

		/*	Note change in forecast total occupancy,
		 *	adjust high-water mark as necessary.		*/

		increment = netGrowth * secAdvanced;
		forecastOccupancy += increment;
		if (forecastOccupancy > maxOccupancy)
		{
			maxOccupancy = forecastOccupancy;
		}

		/*	Advance the forecast time to end and check
		 *	for congestion alarm.				*/

		forecastTime += secAdvanced;
		if (maxOccupancy >= iondb.occupancyCeiling)
		{
			alarmTime = forecastTime;
		}
	}

	if (alarmTime == 0)
	{
		writeMemo("[i] No congestion collapse predicted.");
	}
	else
	{
		/*	Have determined time at which occupancy limit
		 *	will be reached.				*/

		writeTimestampUTC(alarmTime, timestampBuffer);
		isprintf(alarmBuffer, sizeof alarmBuffer,
				"[i] Congestion collapse forecast: %s.",
				timestampBuffer);
		writeMemo(alarmBuffer);
		if (iondb.alarmScript)
		{
			sdr_string_read(sdr, alarmBuffer, iondb.alarmScript);
			pseudoshell(alarmBuffer);
		}
	}

	/*	In any case, update maxForecastOccupancy.		*/

	iondb.maxForecastOccupancy = maxOccupancy;
	sdr_write(sdr, iondbObj, (char *) &iondb, sizeof(IonDB));
	result = sdr_end_xn(sdr);

	/*	Release memory used for neighbors and changes lists.	*/

	elt3 = lyst_first(neighbors);
	while (elt3)
	{
		np = (IonNeighbor *) lyst_data(elt3);
		MRELEASE(np);
		lyst_delete(elt3);
		elt3 = lyst_first(neighbors);
	}

	lyst_destroy(neighbors);
	elt4 = lyst_first(changes);
	while (elt4)
	{
		change = (RateChange *) lyst_data(elt4);
		MRELEASE(change);
		lyst_delete(elt4);
		elt4 = lyst_first(changes);
	}

	lyst_destroy(changes);
	if (result < 0)
	{
		putErrmsg("Failed on check for congestion.", NULL);
		return -1;
	}

	return 0;
}

#if defined (ION_LWT)
int	ionwarn(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
#else
int	main(int argc, char **argv)
{
#endif
	int	result = 0;

	if (ionAttach() < 0)
	{
		putErrmsg("ionwarn can't attach to ION.", NULL);
		result = -1;
	}
	else
	{
		if (checkForCongestion() < 0)
		{
			putErrmsg("ionwarn failed checking for congestion",
					NULL);
			result = -1;
		}
		else
		{
			writeMemo("[i] ionwarn finished.");
		}

		ionDetach();
	}

	return result;
}
