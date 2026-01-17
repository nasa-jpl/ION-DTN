#include "ion.h"
#include "ici/include/radix.h"
#include "ici/library/radixP.h"

#define RADIX_TEST 0
#define LYST_TEST 1

#define RADIX_MAX_SUBSTR 200

PsmAddress makeDataFromString(PsmPartition wm, char *str);

int  radixpt_gen_eidsMatch(char *firstEid, int firstEidLen, char *secondEid, int secondEidLen);
char *radixpt_gen_make_addr(void);
void radixpc_gen_printconfig(void);
void radixpt_gen_printstats(void);
void radixpt_gen_initconfig(void);

void radixpt_user_del(PsmPartition partition, PsmAddress user_data);

int radixpt_user_match(PsmPartition partition, PsmAddress user_data, void *tag);

void radixpt_radix_search(PsmPartition partition, PsmAddress radixAddr, char *value);

void radixpt_radix_populate(PsmPartition partition, PsmAddress radixAddr);
void radixpt_radix_query(PsmPartition partition, PsmAddress raddixAddr);
void radixpt_radix_runtest(void);

typedef struct
{
	int matches;
	int duplicates;
	int nodes;
} RadixTestStats;

typedef struct
{
	int type;
	int inserts;
	int queries;
	int maxNum;
	int verbose;
	int seed1;
	int seed2;
	int wildcard_pct;
} RadixTestConfig;

extern RadixTestStats gStats;
extern RadixTestConfig gConfig;

extern PsmAddress radix_alloc(PsmPartition partition, int size);
