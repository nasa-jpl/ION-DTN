#include "bpP.h"

#include <stdio.h>
#include <stdlib.h>

static long long timevalToMilliseconds(const struct timeval *value)
{
	return (((long long) value->tv_sec) * 1000)
			+ (value->tv_usec / 1000);
}

int main(int argc, char **argv)
{
	IonVdb		*vdb;
	IonDB		db;
	Sdr		sdr;
	struct timeval	before;
	struct timeval	after;
	time_t		ctimeSeconds;
	uint16_t	ctimeMilliseconds;
	long long	ctimeTotal;
	long long	observedDelta;
	long long	volatileDelta;
	long long	persistentDelta;
	DtnTime		dtnTime;
	long long	expectedDtnTime;
	long long	expectedDelta;
	long long	tolerance;

	if (argc != 3)
	{
		fprintf(stderr, "usage: %s expected-delta-ms tolerance-ms\n",
				argv[0]);
		return 2;
	}

	expectedDelta = strtoll(argv[1], NULL, 10);
	tolerance = strtoll(argv[2], NULL, 10);
	if (tolerance < 0 || ionAttach() < 0)
	{
		return 1;
	}

	vdb = getIonVdb();
	sdr = getIonsdr();
	sdr_read(sdr, (char *) &db, getIonDbObject(), sizeof(IonDB));
	getCurrentTime(&before);
	getCtimeMs(&ctimeSeconds, &ctimeMilliseconds);
	getCurrentTime(&after);
	ctimeTotal = (((long long) ctimeSeconds) * 1000)
			+ ctimeMilliseconds;
	getCurrentDtnTime(&dtnTime);
	expectedDtnTime = ctimeTotal - (((long long) EPOCH_2000_SEC) * 1000);
	observedDelta = ((timevalToMilliseconds(&before)
			+ timevalToMilliseconds(&after)) / 2) - ctimeTotal;
	volatileDelta = (((long long) vdb->deltaFromUTC) * 1000)
			+ vdb->deltaFromUTCMillis;
	persistentDelta = (((long long) db.deltaFromUTC) * 1000)
			+ db.deltaFromUTCMillis;

	printf("persistent=%lld,volatile=%lld,observed=%lld,expected=%lld,"
			"dtn-error=%lld\n",
			persistentDelta, volatileDelta, observedDelta,
			expectedDelta, ((long long) dtnTime) - expectedDtnTime);
	ionDetach();
	if (persistentDelta != expectedDelta || volatileDelta != expectedDelta
	|| llabs(observedDelta - expectedDelta) > tolerance
	|| llabs(((long long) dtnTime) - expectedDtnTime) > tolerance)
	{
		return 1;
	}

	return 0;
}
