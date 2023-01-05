// Framework
#include "unity.h"
#include "radix_ut.h"

RadixTestStats gStats;
RadixTestConfig gConfig;

void setUp(void) { }
void tearDown(void) { }




void radix_basic(void)
{
	PsmAddress radixAddr;
	PsmPartition wm = getIonwm();

	char key[20];
	int i,j;

	radixpt_gen_initconfig();
	memset(&gStats, 0, sizeof(RadixTestStats));

	radixAddr = radix_create(wm);

	for(i = 0; i < 10; i++)
	{
		for(j = 0; j < 10; j++)
		{
			memset(key,0,20);
			sprintf(key,"ipn:%d.%d", i,j);
			TEST_ASSERT_EQUAL_INT(1,radix_insert(wm, radixAddr, key, makeDataFromString(wm, key), NULL, radixpt_user_del));
		}


	}

	if(gConfig.verbose)
	{
		char tree_str[4096];
		radix_prettyprint(wm, radixAddr, tree_str, 4096);
		printf("Tree is:\n%s\n",tree_str);
	}

	for(i = 0; i < 10; i++)
	{
		for(j = 0; j < 10; j++)
		{
			sprintf(key,"ipn:%d.%d", i,j);
			radix_foreach_match(wm, radixAddr, key, RADIX_FULL_MATCH, radixpt_user_match, key);
		}
	}

	radix_destroy(wm, radixAddr, radixpt_user_del);

	TEST_ASSERT_EQUAL_INT(100, gStats.matches);
}

void radix_wildcard1(void)
{
	PsmAddress radixAddr;
	PsmPartition wm = getIonwm();
	char key[20];
	int i,j;

	radixpt_gen_initconfig();
	memset(&gStats, 0, sizeof(RadixTestStats));

	radixAddr = radix_create(wm);

	for(i = 0; i < 10; i++)
	{
		memset(key,0,20);
		sprintf(key,"ipn:%d.*", i);
		TEST_ASSERT_EQUAL_INT(1,radix_insert(wm, radixAddr, key, makeDataFromString(wm, key), NULL, radixpt_user_del));
	}

	if(gConfig.verbose)
	{
		char tree_str[4096];
		radix_prettyprint(wm, radixAddr, tree_str, 4096);
		printf("Tree is:\n%s\n",tree_str);
	}

	for(i = 0; i < 10; i++)
	{
		for(j = 0; j < 10; j++)
		{
			sprintf(key,"ipn:%d.%d", i,j);
			radix_foreach_match(wm, radixAddr, key, RADIX_FULL_MATCH, radixpt_user_match, key);
		}
	}

	radix_destroy(wm, radixAddr, radixpt_user_del);

	TEST_ASSERT_EQUAL_INT(100, gStats.matches);
}

void radix_wildcard2(void)
{
	PsmAddress radixAddr;
	PsmPartition wm = getIonwm();
	char key[20];
	int i,j;

	radixpt_gen_initconfig();
	memset(&gStats, 0, sizeof(RadixTestStats));

	radixAddr = radix_create(wm);

	memset(key,0,20);
	sprintf(key,"ipn:*");
	char *tmp = strdup(key);
	TEST_ASSERT_EQUAL_INT(1,radix_insert(wm, radixAddr, key, makeDataFromString(wm, key), NULL, radixpt_user_del));

	if(gConfig.verbose)
	{
		char tree_str[4096];
		radix_prettyprint(wm, radixAddr, tree_str, 4096);
		printf("Tree is:\n%s\n",tree_str);
	}

	for(i = 0; i < 10; i++)
	{
		for(j = 0; j < 10; j++)
		{
			sprintf(key,"ipn:%d.%d", i,j);
			radix_foreach_match(wm, radixAddr, key, RADIX_FULL_MATCH, radixpt_user_match, key);
		}
	}

	radix_destroy(wm, radixAddr, radixpt_user_del);

	TEST_ASSERT_EQUAL_INT(100, gStats.matches);
}


void shallow(void)
{
	PsmAddress radixAddr;
	PsmPartition wm = getIonwm();
	char key[20];
	int i,j;

	radixpt_gen_initconfig();
	memset(&gStats, 0, sizeof(RadixTestStats));

	radixAddr = radix_create(wm);

	for(i = 0; i < 1000; i++)
	{
		memset(key,0,20);
		sprintf(key,"ipn:1.%d", i);
		TEST_ASSERT_EQUAL_INT(1,radix_insert(wm, radixAddr, key, makeDataFromString(wm, key), NULL, radixpt_user_del));
	}

	if(gConfig.verbose)
	{
		char tree_str[9096];
		radix_prettyprint(wm, radixAddr, tree_str, 9096);
		printf("Tree is:\n%s\n",tree_str);
	}

	for(i = 0; i < 1000; i++)
	{
		memset(key,0,20);
		sprintf(key,"ipn:1.%d", i);
		radix_foreach_match(wm, radixAddr, key, RADIX_FULL_MATCH, radixpt_user_match, key);
	}

	RadixTree *radixPtr = psp(wm, radixAddr);
	int nodes = radixPtr->stats.nodes;
	radix_destroy(wm, radixAddr, radixpt_user_del);


	TEST_ASSERT_EQUAL_INT(1000, gStats.matches);
	TEST_ASSERT_EQUAL_INT(1001, nodes);
}


void deep(void)
{
	PsmAddress radixAddr;
	PsmPartition wm = getIonwm();
	PsmAddress keyAddr;
	char *key;
	int i,j;
	int num = 1000;
	int max_len = num + 8;

	radixpt_gen_initconfig();
	memset(&gStats, 0, sizeof(RadixTestStats));

	radixAddr = radix_create(wm);

	keyAddr = radix_alloc(wm, max_len);
	key = psp(wm, keyAddr);

	for(i = 0; i < num; i++)
	{
		memset(key,0,max_len);
		sprintf(key,"ipn:1.");
		for(j = 0; j <= i; j++)
		{
			key[6+j] = '1';
		}
		TEST_ASSERT_EQUAL_INT(1,radix_insert(wm, radixAddr, key, makeDataFromString(wm, key), NULL, radixpt_user_del));
	}

	if(gConfig.verbose)
	{
		char tree_str[4096];
		radix_prettyprint(wm, radixAddr, tree_str, 4096);
		printf("Tree is:\n%s\n",tree_str);
	}

	for(i = 0; i < num; i++)
	{
		memset(key,0,max_len);
		sprintf(key,"ipn:1.");
		for(j = 0; j <= i; j++)
		{
			key[6+j] = '1';
		}
		radix_foreach_match(wm, radixAddr, key, RADIX_FULL_MATCH, radixpt_user_match, key);
	}

	psm_free(wm, keyAddr);
	RadixTree *radixPtr = psp(wm, radixAddr);
	int nodes = radixPtr->stats.nodes;

	radix_destroy(wm, radixAddr, radixpt_user_del);

	TEST_ASSERT_EQUAL_INT(num, gStats.matches);
	TEST_ASSERT_EQUAL_INT(num, nodes);
}



void big_keys(void)
{
	PsmAddress radixAddr;
	PsmPartition wm = getIonwm();
	PsmAddress keyAddr;
	char *key;
	int i,j;
	int num = 10;
	int max_len = 4096;

	radixpt_gen_initconfig();
	memset(&gStats, 0, sizeof(RadixTestStats));

	radixAddr = radix_create(wm);

	keyAddr = radix_alloc(wm, max_len);
	key = psp(wm, keyAddr);


	for(i = 0; i < num; i++)
	{
		memset(key,'A',max_len);
		for(j = 0; j < 10; j++)
		{
			sprintf(key,"ipn:%d.%d", i, j);
			key[max_len-1] = '\0';
			TEST_ASSERT_EQUAL_INT(1,radix_insert(wm, radixAddr, key, makeDataFromString(wm, key), NULL, radixpt_user_del));
		}
	}

	if(gConfig.verbose)
	{
		char tree_str[4096];
		radix_prettyprint(wm, radixAddr, tree_str, 4096);
		printf("Tree is:\n%s\n",tree_str);
	}

	for(i = 0; i < num; i++)
	{
		memset(key,'A',max_len);
		for(j = 0; j < 10; j++)
		{
			sprintf(key,"ipn:%d.%d", i, j);
			key[max_len-1] = '\0';
			radix_foreach_match(wm, radixAddr, key, RADIX_FULL_MATCH, radixpt_user_match, key);
		}
	}

	psm_free(wm, keyAddr);
//	RadixTree *radixPtr = psp(wm, radixAddr);
//	int nodes = radixPtr->stats.nodes;
	radix_destroy(wm, radixAddr, radixpt_user_del);

	TEST_ASSERT_EQUAL_INT(num*num, gStats.matches);
}

void radix_large(void)
{
	PsmAddress radixAddr;
	PsmPartition wm = getIonwm();
	char key[25];
	int i,j;
	int result = 0;

	radixpt_gen_initconfig();
	memset(&gStats, 0, sizeof(RadixTestStats));

	radixAddr = radix_create(wm);

	for(i = 0; i < 1000; i++)
	{

		for(j = 0; j < 1000; j++)
		{
			memset(key,0,25);
			sprintf(key,"ipn:%d.%d", i,j);

			result = radix_insert(wm, radixAddr, key, makeDataFromString(wm, key), NULL, radixpt_user_del);

			if(result != 1)
			{
				radix_destroy(wm, radixAddr, radixpt_user_del);
				TEST_ASSERT_EQUAL_INT(1,result);
				return;
			}
		}
	}

	RadixTree *radixPtr = psp(wm, radixAddr);
	int nodes = radixPtr->stats.nodes;

	radix_destroy(wm, radixAddr, radixpt_user_del);

	TEST_ASSERT_GREATER_THAN(1000000, nodes);
}


void radix_query(void)
{
	PsmAddress radixAddr;
	PsmPartition wm = getIonwm();
	long i;

	radixpt_gen_initconfig();
	memset(&gStats, 0, sizeof(RadixTestStats));

	radixAddr = radix_create(wm);

	srand(gConfig.seed1);
	for(i = 0; i < 1000; i++)
	{
		char *tmp = radixpt_gen_make_addr();
		radix_insert(wm, radixAddr, tmp, makeDataFromString(wm, tmp), NULL, radixpt_user_del);
		free(tmp);
	}

	srand(gConfig.seed2);

	for(i = 0; i < 1000000; i++)
	{
		char *tmp = radixpt_gen_make_addr();
		radixpt_radix_search(wm, radixAddr, tmp);
		free(tmp);
	}

	RadixTree *radixPtr = psp(wm, radixAddr);
	int nodes = radixPtr->stats.nodes;

	radix_destroy(wm, radixAddr, radixpt_user_del);

	TEST_ASSERT_GREATER_THAN(1000, nodes);
	TEST_ASSERT_GREATER_THAN(90000, gStats.matches); // TODO figure out this number...?

}

int main(void)
{
   UNITY_BEGIN();

   ionAttach();

   /* Basic Test */
   RUN_TEST(radix_basic);

   /* Basic Wildcard Tests */
   RUN_TEST(radix_wildcard1);
   RUN_TEST(radix_wildcard2);

   /* Shallow Tree Test */
   RUN_TEST(shallow);

   /* Deep Tree Test */
   RUN_TEST(deep);

   /* Large keys test */
   RUN_TEST(big_keys);

   /* Large Inserts Test */
//   RUN_TEST(radix_large);

   /* Large Queries Test */
   RUN_TEST(radix_query);

   return UNITY_END();
}
