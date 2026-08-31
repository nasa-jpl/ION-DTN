#include "rfx.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	uvast		neighborFqnn;
	IonNeighbor	*neighbor;
	time_t		seconds;
	uint16_t	milliseconds;
	int		samples;
	int		i;

	if (argc != 3)
	{
		fprintf(stderr, "usage: %s neighbor-fqnn samples\n", argv[0]);
		return 2;
	}

	neighborFqnn = strtoull(argv[1], NULL, 10);
	samples = atoi(argv[2]);
	if (neighborFqnn == 0 || samples <= 0 || ionAttach() < 0)
	{
		return 1;
	}

	for (i = 0; i < samples; i++)
	{
		getCtimeMs(&seconds, &milliseconds);
		neighbor = getNeighbor(getIonVdb(), neighborFqnn);
		printf("%lld%03u,%zu\n", (long long) seconds,
				(unsigned int) milliseconds,
				neighbor ? neighbor->xmitRate : 0);
		fflush(stdout);
		microsnooze(5000);
	}

	ionDetach();
	return 0;
}
