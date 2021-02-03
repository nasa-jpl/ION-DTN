
#include "ici/library/lystP.h"

#include "radix_gen.h"




void radixpt_ui_parse_args(int argc, char *argv[]);
void radixpt_ui_print_usage(char *name);


Lyst radixpt_lyst_populate();
void radixpt_lyst_query(Lyst eids);
void radixpt_lyst_runtest();
int radixpt_lyst_search(Lyst eids, char *key);

void radixpt_rbt_query(PsmAddress rbt);
void radixpt_rbt_populate(PsmAddress rbt);
void radixpt_rbt_runtest();


typedef struct LystTestData_s
{
	char key[RADIX_MAX_SUBSTR];
	void *data;
} LystTestData;




