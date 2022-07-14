// Framework
#include "unity.h"
#include "lyst.h"
#include "radix_pt.h"

RadixTestStats gStats;
RadixTestConfig gConfig;


/*
 * Prototypes
 */






int radixpt_lyst_search(Lyst eids, char *key)
{
	LystElt elt;

	int len = strlen(key);
	for(elt = lyst_first(eids); elt; elt = lyst_next(elt))
	{
		LystTestData *value = (LystTestData *) lyst_data(elt);

		int key_len = strlen(value->key);
		if((key_len == len) && (radixpt_gen_eidsMatch(value->key, key_len, key, len)))
		{
			if(gConfig.verbose)
			{
			  printf("Lyst matched %s and %s\n", value->key, key);
			}
			return 1;
		}
	}

	return 0;
}

Lyst radixpt_lyst_populate()
{
	Lyst eids = lyst_create();

	srand(gConfig.seed1);
	int i = 0;
	for(i = 0; i < gConfig.inserts; i++)
	{
		char *tmp = radixpt_gen_make_addr();

		if(radixpt_lyst_search(eids, tmp) == 0)
		{
			LystTestData *test_data = (LystTestData*) malloc(sizeof(LystTestData));
			memset(test_data, 0, sizeof(LystTestData));
			strncpy(&(test_data->key[0]), tmp, RADIX_MAX_SUBSTR-1);
			test_data->data = strndup(tmp,RADIX_MAX_SUBSTR);
			if(gConfig.verbose)
			{
				printf("ADDING: %s\n", tmp);
			}
			lyst_insert(eids, test_data);
		}
		else
		{
			if(gConfig.verbose)
			{
				printf("duplicate: %s\n", tmp);
			}
		}
		free(tmp);
	}

	gStats.nodes = lyst_length(eids);
	return eids;
}

void radixpt_lyst_query(Lyst eids)
{
	LystElt elt;

	CHKVOID(eids);

	srand(gConfig.seed2);
	int i = 0;
	for(i = 0; i < gConfig.queries; i++)
	{
		char *tmp = radixpt_gen_make_addr();
		int len = strlen(tmp);
		for(elt = lyst_first(eids); elt; elt = lyst_next(elt))
		{
			int key_len;
			LystTestData *test_data = (LystTestData*) lyst_data(elt);

			key_len = strlen(test_data->key);
			if((key_len == len) && (radixpt_gen_eidsMatch(test_data->key, strlen(test_data->key), tmp, len)))
			{
				gStats.matches++;
				if(gConfig.verbose)
				{
					printf("%s matches %s\n", test_data->key, tmp);
				}
			}
		}

		free(tmp);
	}
}


void radixpt_lyst_cleanup(Lyst eids)
{
	LystElt elt;

	for(elt = lyst_first(eids); elt; elt = lyst_next(elt))
	{
		LystTestData *test_data = (LystTestData*) lyst_data(elt);
		free(test_data->data);
		free(test_data);
	}
	lyst_destroy(eids);
}



void radixpt_lyst_runtest()
{
	Lyst eids;

	eids = radixpt_lyst_populate();

	radixpt_lyst_query(eids);
	radixpt_lyst_cleanup(eids);
}



static int radixpt_rbt_key_comp(PsmPartition wm, PsmAddress refData, void *dataBuffer)
{
	char *key;
	char *argKey;

	key = (char *) psp(wm, refData);
	argKey = (char *) dataBuffer;

	return strcmp(key, argKey);
}


void radixpt_rbt_populate(PsmAddress rbt)
{
	PsmAddress node;
	PsmAddress succ;

	srand(gConfig.seed1);
	int i = 0;
	for(i = 0; i < gConfig.inserts; i++)
	{
		char *tmp = radixpt_gen_make_addr();

		/* See if it is already there */

		node = sm_rbt_search(getIonwm(), rbt, radixpt_rbt_key_comp, tmp, &succ);

		if(node != 0)
		{
			if(gConfig.verbose)
			{
				printf("duplicate: %s\n", tmp);
			}
			free(tmp);
		}
		else
		{
			PsmAddress addr;
			char *entry;

			if(gConfig.verbose)
			{
				printf("ADDING: %s\n", tmp);
			}

			addr = psm_zalloc(getIonwm(), strlen(tmp) + 1);

			if(addr)
			{
				char *entry = (char *) psp(getIonwm(), addr);
				strncpy(entry,tmp,RADIX_MAX_SUBSTR);
				sm_rbt_insert(getIonwm(), rbt, addr, radixpt_rbt_key_comp, entry);
			}
			free(tmp);
		}
	}

	gStats.nodes = 0;

}



void radixpt_rbt_query(PsmAddress rbt)
{
	PsmAddress node;
	PsmAddress succ;


	srand(gConfig.seed2);
	int i = 0;
	for(i = 0; i < gConfig.queries; i++)
	{
		char *tmp = radixpt_gen_make_addr();

		/* See if it is already there */
		node = sm_rbt_search(getIonwm(), rbt, radixpt_rbt_key_comp, tmp, &succ);

		if(node != 0)
		{
			gStats.matches++;
			if(gConfig.verbose)
			{
				PsmAddress refAddr = sm_rbt_data(getIonwm(), node);
				char *data = (char *) psp(getIonwm(), refAddr);
				printf("%s matches %s\n", data, tmp);
			}
		}
		free(tmp);
	}
}

static void	radixpt_rbt_key_del(PsmPartition wm, PsmAddress refData, void *arg)
{
	psm_free(wm, refData);
}


void radixpt_rbt_runtest()
{
	PsmAddress rbt = sm_rbt_create(getIonwm());

	radixpt_rbt_populate(rbt);

	radixpt_rbt_query(rbt);

	sm_rbt_destroy(getIonwm(), rbt, radixpt_rbt_key_del, NULL);
}



void radixpt_ui_print_usage(char *name)
{

	printf("Usage: %s -h -t# -i# -q# -m# -v -s1# -s2# -w#\n", name);
	printf("-h (Help) Print usage and exit.\n");
	printf("-t (Type) 0 = RADIX, 1 = LYST 2 = RBT. Default = %d\n", gConfig.type);
	printf("-i (# Random Inserts). Default = %d\n", gConfig.inserts);
	printf("-q (# Random Queries). Default = %d\n", gConfig.queries);
	printf("-m (Max EID #) e.g., ipn:max.max. Default = %d\n", gConfig.maxNum);
	printf("-v (Verbose Mode). Default = off\n");
	printf("-s1 (Insert Seed) For randomized inserts. Default = %d\n", gConfig.seed1);
	printf("-s2 (Query Seed) For randomized queries. Default = %d\n", gConfig.seed2);
	printf("-w (Wildcard Pct) %% inserts with wildcard. Default = %d\n", gConfig.wildcard_pct);
}

void radixpt_ui_parse_args(int argc, char *argv[])
{
	int i = 0;
	uint value;
	int error = 0;

	for(i = 1; i < argc; i++)
	{
		if (sscanf(argv[i], "-t%d", &value) == 1)
		{
			gConfig.type = value;
			if(value > 2)
			{
				printf("Bad type: %s\n", argv[i]);
				error = 1;
				break;
			}
		}
		else if(sscanf(argv[i], "-i%d", &value) == 1)
		{
			gConfig.inserts = value;
		}
		else if(sscanf(argv[i], "-q%d", &value) == 1)
		{
			gConfig.queries = value;
		}
		else if(sscanf(argv[i], "-m%d", &value) == 1)
		{
			gConfig.maxNum = value;
		}
		else if(strcmp(argv[i], "-v") == 0)
		{
			gConfig.verbose = 1;
		}
		else if(strcmp(argv[i], "-h") == 0)
		{
			radixpt_ui_print_usage(argv[0]);
			exit(1);
		}
		else if(sscanf(argv[i], "-s1%d", &value) == 1)
		{
			gConfig.seed1 = value;
		}
		else if(sscanf(argv[i], "-s2%d", &value) == 1)
		{
			gConfig.seed2 = value;
		}
		else if(sscanf(argv[i], "-w%d", &value) == 1)
		{
			gConfig.wildcard_pct = value;
			if(gConfig.wildcard_pct > 100)
			{
				printf("Bad wilcard: %s\n", argv[i]);
				error = 1;
				break;
			}
		}
	}

	if(error)
	{
		radixpt_ui_print_usage(argv[0]);
		exit(1);
	}
}

int main(int argc, char *argv[])
{

	radixpt_gen_initconfig();
	radixpt_ui_parse_args(argc, argv);

	radixpc_gen_printconfig();

	if(gConfig.type == RADIX_TEST)
	{
		ionAttach();
		radixpt_radix_runtest();
	}
	else if(gConfig.type == LYST_TEST)
	{
		radixpt_lyst_runtest();
	}
	else
	{
		ionAttach();
		radixpt_rbt_runtest();
	}

	radixpt_gen_printstats();

	printf("Finishing\n");

	return(0);
}
