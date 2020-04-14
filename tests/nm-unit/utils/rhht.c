// Framework
#include "unity.h"

// module being tested
#include "utils/db.h"
#include "utils/rhht.h"
#include "utils/utils.h"
#include "primitives/ari.h"

// Macros to appese the compiler
#define INT2VOIDP(i) (void*)(uintptr_t)(i)
#define VOIDP2INT(i) (uintptr_t)(void*)(i)

#if AMP_VERSION < 7
#define TEST_ARI_1 "0xa401410101"
#else
#define TEST_ARI_1 "0xa40141014101"
#endif

/** The following stubs are needed for compilation
 * adm_init() as a stub to avoid building nmagent.c/nm_mgr.c
 * setUp and tearDown are required by unity when not integrated with ceedling
 * They are called before and after every RUN_TEST()
 */
void adm_init() { }
void setUp(void) { }
void tearDown(void) { }

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
rh_idx_t int_cb_hash_fn(void *table_in, void *key)
{
   rhht_t *table = (rhht_t*)table_in;
   if (testMode == 0)
   {
      return 0;
   }
   else
   {
      return ( (size_t)key % testMode) % table->num_bkts;
   }
}

void test_simple_cnt_cb(void *value, void *tag)
{
   int *cnt = (int*)tag;
   (*cnt)++;
}

// TODO: Consider moving this, and test_rhht_ari, to ari test file
ari_t* test_ari_deserialize_raw(char* input)
{
   int success = AMP_FAIL;
   blob_t *buf, *buf2;
   
   // Construct input Data Blob
   buf = utils_string_to_hex(input);
   TEST_ASSERT_NOT_NULL(buf);

   ari_t *rtv = ari_deserialize_raw(buf, &success);

   TEST_ASSERT_NOT_NULL(rtv);
   TEST_ASSERT_EQUAL_INT(AMP_OK, success);

   blob_release(buf, 1); // Destroy the blob
   
   return rtv;
}

void test_rhht_ari(void)
{
   int rtv;
   rh_idx_t idx;
   rhht_t ht = rhht_create(16, ari_cb_comp_fn, ari_cb_hash, ari_cb_ht_del, &rtv);
   TEST_ASSERT_EQUAL_INT(AMP_OK, rtv);

   // Create a demo ARI
   ari_t *ari = test_ari_deserialize_raw(TEST_ARI_1);
   TEST_ASSERT_NOT_NULL(ari);
   
   // Insert ARI into RHHT
   TEST_ASSERT_EQUAL_INT(RH_OK, rhht_insert(&ht, ari, ari, &idx));
   
   // Find ARI in RHHT
   ari_t *ari2 = rhht_retrieve_key(&ht, ari);
   TEST_ASSERT_EQUAL(ari2, ari);

   // Create a new ARI from same input and verify it can be found
   ari2 = test_ari_deserialize_raw(TEST_ARI_1);
   TEST_ASSERT_NOT_NULL(ari2);

   // Validate hash calculations
   //printf("ISS values are %x, %x\n", ari->as_reg.iss_idx, ari2->as_reg.iss_idx);
   TEST_ASSERT_EQUAL(ht.hash(&ht, ari), ht.hash(&ht, ari2));

   // Retrieve key with copy
   TEST_ASSERT_EQUAL(ari, rhht_retrieve_key(&ht, ari2 ) );
   
   // NOTE: Valgrind is required to test if this works correctly.
   // All contents of the HT should be deleted with it
   rhht_release(&ht, 0);

}

void test_simple_int(void)
{
   int rtv, i, x;
   rhht_t ht;
   rhht_t *htp = &ht;
   rh_idx_t idx;

   // Create HT
   ht = rhht_create(8, int_cb_comp_fn, int_cb_hash_fn, NULL, &rtv);
   TEST_ASSERT_EQUAL_INT(AMP_OK, rtv);

   // Insert 8 entries using fixed hashing function
   // Note: 0 is not a valid key value
   for(i = 1; i <= 8; i++)
   {
      //printf("Test Insert %d\n", i);
      TEST_ASSERT_EQUAL_INT(RH_OK, rhht_insert(htp, INT2VOIDP(i), INT2VOIDP(i), NULL));
   }

   // Insert 1 more and verify that we get RH_FULL
   TEST_ASSERT_EQUAL_INT(RH_FULL, rhht_insert(htp, INT2VOIDP(i), INT2VOIDP(i), NULL));

   // Insert a duplicate value and verify we get RH_DUPLICATE
   TEST_ASSERT_EQUAL_INT(RH_DUPLICATE, rhht_insert(htp, INT2VOIDP(2), INT2VOIDP(1), NULL));

   // Try to retrieve a specific value
   TEST_ASSERT_EQUAL_INT(RH_OK, rhht_find(htp, INT2VOIDP(3), &idx) );
   TEST_ASSERT_EQUAL_INT(3, rhht_retrieve_idx(htp, idx) );
   TEST_ASSERT_EQUAL_INT(3, rhht_retrieve_key(htp, INT2VOIDP(3)) );

   // Test deletion
   rhht_del_idx(htp, idx);
   rhht_del_key(htp, INT2VOIDP(5));

   // Verify deleted items can no longer be found
   TEST_ASSERT_EQUAL_INT(RH_NOT_FOUND, rhht_find(htp, INT2VOIDP(3), &idx) );
   TEST_ASSERT_EQUAL_INT(RH_NOT_FOUND, rhht_find(htp, INT2VOIDP(5), &idx) );
   
   // Verify all using foreach
   rtv = 0;
   rhht_foreach(htp, (rh_foreach_fn)test_simple_cnt_cb, &rtv);
   TEST_ASSERT_EQUAL_INT(6, rtv);

   // Explicit/manual verification
   for(i = 1; i < 10; i++)
   {
      rtv = VOIDP2INT(rhht_retrieve_key(htp, INT2VOIDP(i)));
      if (i == 3 || i == 5 || i > 8)
      {
         TEST_ASSERT_EQUAL_INT(NULL, rtv);
      }
      else
      {
         TEST_ASSERT_EQUAL_INT(i, rtv);
      }
   }
   
   rhht_release(&ht, 0);
}

int main(void)
{
#if 0
   // Memory initialization
   if (ionAttach() < 0)
   {
      AMP_DEBUG_ERR("nm_dotest", "can't attach to ION.", NULL);
      return -1;
   }
#endif
   utils_mem_int(); // Initialize utils
   db_init("test_db", &adm_init); // Initialize global structures (used in some nested functions)

   
   UNITY_BEGIN();

   testMode = 0; // single hash bucket
   RUN_TEST(test_simple_int);

   // every key has a unique bucket
   testMode = 1;
   RUN_TEST(test_simple_int);

   // Use just two hash buckets
   testMode = 2;
   RUN_TEST(test_simple_int);

   RUN_TEST(test_rhht_ari);
   
   return UNITY_END();
}
