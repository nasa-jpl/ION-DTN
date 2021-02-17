#include "radix_gen.h"



PsmAddress makeDataFromString(PsmPartition wm, char *str)
{
	PsmAddress tmpAddr = 0;
	char *tmp = NULL;

	CHKZERO(wm);
	CHKZERO(str);

	tmpAddr = radix_alloc(wm, strlen(str)+1);
	CHKZERO(tmpAddr);

	tmp = psp(wm, tmpAddr);
	memcpy(tmp, str, strlen(str));
	return tmpAddr;
}

void radixpt_user_del(PsmPartition partition, PsmAddress user_data)
{
	psm_free(partition, user_data);
}

int radixpt_user_match(PsmPartition partition, PsmAddress user_data, void *tag)
{
	char *user = (char *) psp(partition, user_data);
	char *key = (char *) tag;

	gStats.matches++;

	if(gConfig.verbose)
	{
		CHKZERO(user);
		CHKZERO(key);
		printf("%s matches %s\n", user, key);
	}
	return 0;
}


void radixpt_radix_search(PsmPartition partition, PsmAddress radixAddr, char *value)
{
	char *src_key = strndup(value, RADIX_MAX_SUBSTR);
	CHKVOID(src_key);

//	if(gConfig.verbose)
//	{
//		printf("Searching for matches to %s\n", src_key);
//		printf("----------------------------\n");
//	}

	radix_foreach_match(partition, radixAddr, value, RADIX_FULL_MATCH, radixpt_user_match, src_key);

//	if(gConfig.verbose)
//	{
//		printf("----------------------------\n");
//	}

	free(src_key);
}

char *radixpt_gen_make_addr()
{
	char *tmp = malloc(RADIX_MAX_SUBSTR);
	CHKNULL(tmp);
	if((rand()%100) < gConfig.wildcard_pct)
	{
		switch(rand()%2)
		{
		case 0:
			snprintf(tmp, RADIX_MAX_SUBSTR-1, "ipn:%d~", rand()%gConfig.maxNum);
			break;
		case 1:
		default:
			snprintf(tmp, RADIX_MAX_SUBSTR-1, "ipn:%d.%d~", rand()%gConfig.maxNum, rand()%gConfig.maxNum);
			break;
		}
	}
	else
	{
		snprintf(tmp, RADIX_MAX_SUBSTR-1, "ipn:%d.%d", rand()%gConfig.maxNum, rand()%gConfig.maxNum);
	}

	return tmp;
}

/*
 * Modified: treat ~ as +.
 */

int	radixpt_gen_eidsMatch(char *firstEid, int firstEidLen, char *secondEid, int secondEidLen)
{
	int i;

	CHKZERO(firstEid);
	CHKZERO(firstEidLen > 0);
	CHKZERO(secondEid);
	CHKZERO(secondEidLen > 0);

	for (i = 0; i < MAX(firstEidLen, secondEidLen); i++)
	{
		if ((firstEidLen < i) || (secondEidLen < i))
		{
			return 0;
		}
		else if((firstEid[i] == '~') || (secondEid[i] == '~'))
		{
			return 1;
		}
		else if (firstEid[i] != secondEid[i])
		{
			return 0;
		}
	}

	return 1;
}

void radixpc_gen_printconfig()
{
		printf("Test Options:\n-------------\n");
		printf("\tType: %s\n", (gConfig.type == RADIX_TEST) ? "radix" : "lyst");
		printf("\t#Inserts: %d\n", gConfig.inserts);
		printf("\t#Queries: %d\n", gConfig.queries);
		printf("\t#MaxNum: %d\n", gConfig.maxNum);
		printf("\t#Verbose: %d\n", gConfig.verbose);
		printf("\tInsert Seed: %d\n", gConfig.seed1);
		printf("\tQuery Seed: %d\n", gConfig.seed2);
		printf("\tWildcard: %d%%\n", gConfig.wildcard_pct);
		printf("-------------\n");
}


void radixpt_gen_printstats()
{
/*
	long est_size;

	printf("Sizes:\nTree is %d. Node is %d.\n", sizeof(struct RadixTree_s), sizeof(struct RadixNode_s));
	printf("Sizes:\nLyst is %d. Elt is %d.\n", sizeof(struct LystStruct), sizeof(struct LystEltStruct));

	est_size = (gConfig.type == RADIX_TEST) ?
			      sizeof(struct RadixTree_s) + (gStats.nodes * sizeof(struct RadixNode_s)) :
				  sizeof(struct LystStruct)  + (gStats.nodes * sizeof(struct LystEltStruct));
*/
	if(gConfig.verbose)
	{
		printf("Statistics:\n-------------\n");
		printf("\tMatches: %d\n", gStats.matches);
		printf("\tDuplications: %d\n", gStats.duplicates);
		printf("\tNodes %d\n", gStats.nodes);
//		printf("\tEst space: %ld bytes\n", est_size);
		printf("-------------\n");
	}
	else
	{
		printf("%d, %d, %d\n", gStats.matches, gStats.duplicates, gStats.nodes);
	}
}



void radixpt_radix_populate(PsmPartition partition, PsmAddress radixAddr)
{
	RadixTree *radixPtr = NULL;
	PsmAddress dataAddr = 0;

	srand(gConfig.seed1);
	int i = 0;
	for(i = 0; i < gConfig.inserts; i++)
	{
		char *tmp = radixpt_gen_make_addr();

		dataAddr = makeDataFromString(partition, tmp);
		if(radix_insert(partition, radixAddr, tmp, dataAddr, NULL, radixpt_user_del) != 1)
		{
			if(gConfig.verbose)
			{
				printf("duplicate: %s\n", tmp);
			}
		}
		else
		{
			if(gConfig.verbose)
			{
				printf("ADDING: %s with addr %ld\n", tmp, dataAddr);
			}
		}

		free(tmp);
	}
	radixPtr = (RadixTree*) psp(partition, radixAddr);
	gStats.nodes = radixPtr->stats.nodes;
}


void radixpt_radix_query(PsmPartition partition, PsmAddress radixAddr)
{
	srand(gConfig.seed2);
	int i = 0;
	for(i = 0; i < gConfig.queries; i++)
	{
		char *tmp = radixpt_gen_make_addr();
		radixpt_radix_search(partition, radixAddr, tmp);
		free(tmp);
	}
}


void radixpt_radix_runtest()
{
	PsmPartition wm = getIonwm();

	PsmAddress radixAddr = 0;


	radixAddr = radix_create(wm);

	radixpt_radix_populate(wm, radixAddr);

	if(gConfig.verbose)
	{
      char tree_str[4096];
		radix_prettyprint(wm, radixAddr, tree_str, 4096);
		printf("Tree is:\n%s\n",tree_str);
	}

	radixpt_radix_query(wm, radixAddr);

	radix_destroy(wm, radixAddr, radixpt_user_del);
}


void radixpt_gen_initconfig()
{
	gConfig.type = RADIX_TEST;
	gConfig.inserts = 100;
	gConfig.queries = 1000;
	gConfig.maxNum = 100;
	gConfig.seed1 = 1234;
	gConfig.seed2 = 1234;
	gConfig.verbose = 0;
	gConfig.wildcard_pct = 0;
}

