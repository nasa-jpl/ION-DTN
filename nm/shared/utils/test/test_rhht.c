// Framework
#include "unity.h"

// module being tested
#include "rhht.h"
#include "utils.h"
#include "ari.h"

/** Simple Hash callback functions for testing (int type)**/
int int_cb_comp_fn(void *a, void* b)
{
   if (a==b)
   {
      return 0;
   }
   else if (a > b)
   {
      return 1;
   }
   else
   {
      return -1;
   }
}

int testMode = 0;
rh_idx_t int_cb_hash_fn(rhht_t *table, void *key)
{
   if (testMode == 0)
   {
      return 0;
   }
   else
   {
      return ( (int)key % testMode) % table->num_bkts;
   }
}

void test_simple_cnt_cb(void *value, void *tag)
{
   int *cnt = (int*)tag;
   (*cnt)++;
}



void test_create_basic_ari(void)
{
   int rtv, idx;
   rhht_t ht = rhht_create(16, ari_cb_comp_fn, ari_cb_hash, ari_cb_ht_del, &rtv);
   TEST_ASSERT_EQUAL_INT(AMP_OK, rtv);
 
   // proposed api
   //TEST_ASSERT_EQUAL_INT( AMP_OK, rhht_create_ari( 16, &ht ));

   //TEST_ASSERT_EQUAL_INT(RH_OK, rhht_insert(&ht, key, value, &idx));

   
   // NOTE: Valgrind is required to test if this works correctly.
   rhht_release(&ht, 0);

}

void test_simple_int(void)
{
   int rtv, i, x;
   rhht_t ht;
   rhht_t *htp = &ht;

   // Create HT
   ht = rhht_create(8, int_cb_comp_fn, int_cb_hash_fn, NULL, &rtv);
   TEST_ASSERT_EQUAL_INT(AMP_OK, rtv);

   // Insert 8 entries using fixed hashing function
   // Note: 0 is not a valid key value
   for(i = 1; i <= 8; i++)
   {
      //printf("Test Insert %d\n", i);
      TEST_ASSERT_EQUAL_INT(RH_OK, rhht_insert(htp, i, i, NULL));
   }

   // Insert 1 more and verify that we get RH_FULL
   TEST_ASSERT_EQUAL_INT(RH_FULL, rhht_insert(htp, i, i, NULL));

   // Insert a duplicate value and verify we get RH_DUPLICATE
   TEST_ASSERT_EQUAL_INT(RH_DUPLICATE, rhht_insert(htp, 2, 1, NULL));

   // Try to retrieve a specific value
   TEST_ASSERT_EQUAL_INT(RH_OK, rhht_find(htp, 3, &rtv) );
   TEST_ASSERT_EQUAL_INT(3, rhht_retrieve_idx(htp, rtv) );
   TEST_ASSERT_EQUAL_INT(3, rhht_retrieve_key(htp, 3) );

   // Test deletion
   rhht_del_idx(htp, rtv);
   rhht_del_key(htp, 5);

   // Verify deleted items can no longer be found
   TEST_ASSERT_EQUAL_INT(RH_NOT_FOUND, rhht_find(htp, 3, &rtv) );
   TEST_ASSERT_EQUAL_INT(RH_NOT_FOUND, rhht_find(htp, 5, &rtv) );
   
   // Verify all using foreach
   rtv = 0;
   rhht_foreach(htp, test_simple_cnt_cb, &rtv);
   TEST_ASSERT_EQUAL_INT(6, rtv);

   // Explicit/manual verification
   for(i = 1; i < 10; i++)
   {
      x = rhht_retrieve_key(htp, i);
      if (i == 3 || i == 5 || i > 8)
      {
         TEST_ASSERT_EQUAL_INT(NULL, x);
      }
      else
      {
         TEST_ASSERT_EQUAL_INT(i, x);
      }
   }
   
   rhht_release(&ht, 0);
}

int main(void)
{

   UNITY_BEGIN();
   RUN_TEST(test_create_basic_ari);

   testMode = 0; // single hash bucket
   RUN_TEST(test_simple_int);

   // every key has a unique bucket
   testMode = 1;
   RUN_TEST(test_simple_int);

   // Use just two hash buckets
   testMode = 2;
   RUN_TEST(test_simple_int);
   
   // TODO: Tweaks to rhht_create() call
   
   return UNITY_END();
}
