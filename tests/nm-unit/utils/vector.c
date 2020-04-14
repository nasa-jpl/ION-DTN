// Framework
#include "unity.h"

// module being tested
#include "nm/shared/utils/vector.h"

/** The following stubs are needed for compilation
 * adm_init() as a stub to avoid building nmagent.c/nm_mgr.c
 * setUp and tearDown are required by unity when not integrated with ceedling
 */
void adm_init() { }
void setUp(void) { }
void tearDown(void) { }


/* Test minimum vector creation and cleanup functions
 *  This test does not allocate any resources that need to be cleaned up.
 */
void test_create_basic(void)
{
   int rtv;
   vector_t result = vec_create(0,
                                NULL, NULL, NULL, VEC_FLAG_AS_STACK, &rtv);
   TEST_ASSERT_EQUAL_INT(VEC_OK, rtv);

   // @TODO: is there anything we should verify here?
   vec_release(&result, 0);
}

void test_vecstring(void)
{
   vector_t vec, vec2;
   char* val1 = "Hello World!";
   char* valX = "String %d";
   char valI[32];
   char *ptr, *ptr2;
   vec_idx_t idx;
   int rtv, i, num;
   vecit_t it;
   
   TEST_ASSERT_EQUAL_INT(VEC_OK, vec_str_init(&vec, 0));

   // Add to the vector
   TEST_ASSERT_EQUAL_INT(VEC_OK, vec_insert_copy(&vec, val1, &idx) );

   // Verify that vector size is now one (or more) and num_entires is 1
   TEST_ASSERT_EQUAL_INT(1, vec_num_entries(vec));
   TEST_ASSERT_GREATER_THAN(1, vec_size(&vec));

   // Verify we can retrieve value by index
   TEST_ASSERT_EQUAL_STRING(val1, vec_at(&vec, idx));
   
   // Pop from the vector and verify contents
   ptr = vec_pop(&vec, &rtv);
   TEST_ASSERT_EQUAL_STRING(val1, ptr);
   TEST_ASSERT_EQUAL_INT(rtv, VEC_OK);
   vec_free_value(&vec, ptr);


   // Insert several entries
   for(i = 0; i < 10; i++)
   {
      sprintf(valI, valX, i);
      
      // Add to the vector
      TEST_ASSERT_EQUAL_INT(VEC_OK, vec_insert_copy(&vec, valI, &idx) );
      //printf("Inserted %s at i=%d, idx=%d\n", valI, i, idx);
      TEST_ASSERT_EQUAL_INT(i, idx);
   }
   TEST_ASSERT_EQUAL_INT(10, vec_num_entries(vec));
   TEST_ASSERT_GREATER_THAN(9, vec_size(&vec));


   // Verify values
   for(i = 0; i < 10; i++)
   {
      sprintf(valI, valX, i);

      // Verify we can retrieve value by index
      TEST_ASSERT_EQUAL_STRING(valI, vec_at(&vec, i));
      //printf("Val %i is %s\n", i, vec_at(&vec, i) );
   }
   
   // Pop the last one and compare
   ptr = vec_pop(&vec, &rtv);
   TEST_ASSERT_EQUAL_STRING(valI, ptr);
   TEST_ASSERT_EQUAL_INT(rtv, VEC_OK);
   vec_free_value(&vec, ptr);

   // Let's look for the last one and verify it's been popped
   vec_find(&vec, valI, &rtv);
   TEST_ASSERT_EQUAL_INT(VEC_FAIL, rtv);

   // Let's find one and verify it's index
   sprintf(valI, valX, 4);
   TEST_ASSERT_EQUAL_INT(4, vec_find(&vec, valI, &rtv));

   // Let's delete the entry
   TEST_ASSERT_EQUAL_INT(VEC_OK, vec_del(&vec, 4));
   TEST_ASSERT_EQUAL_INT(8, vec_num_entries(vec));

   // and verify it can no longer be found
   vec_find(&vec, valI, &rtv);
   TEST_ASSERT_EQUAL_INT(VEC_FAIL, rtv);
   
   
   // Test vec_copy
   vec2 = vec_copy(&vec, &rtv);
   TEST_ASSERT_EQUAL_INT(VEC_OK, rtv);
   num = vec_num_entries(vec);
   TEST_ASSERT_EQUAL_INT(num, vec_num_entries(vec2));
   TEST_ASSERT_EQUAL_INT(vec_size(&vec), vec_size(&vec2));
   
   vec_release(&vec, 0);

   // Parallel test of iterators:
   it = vecit_first(&vec2);
   
   // Verify vec_copy after original has been released
   for(i = 0; i < 16; i++)
   {
      sprintf(valI, valX, i);
     
      // Verify we can retrieve value by index
      if (i == 4 || i > 8)
      {
         // Except for the ones we deleted or are otherwise empty
         TEST_ASSERT_EQUAL_STRING(NULL, vec_at(&vec2, i));
      }
      else
      {
         TEST_ASSERT_EQUAL_STRING(valI, vec_at(&vec2, i));
      }

      // Vector verification
      if (i > 8)
      {
         TEST_ASSERT_EQUAL_INT(0, vecit_valid(it));
      }
      else if (i != 4)
      {
         TEST_ASSERT_EQUAL_INT(1, vecit_valid(it));
         TEST_ASSERT_EQUAL_INT(i, vecit_idx(it));
         TEST_ASSERT_EQUAL_INT(vec_at(&vec2, i), vecit_data(it) );
         it = vecit_next(it);
      }

      //printf("Val %i is %s\n", i, vec_at(&vec2, i) );


   }

   // Let's reinsert idx 4. Then insert it a second time and verify results
   sprintf(valI, valX, 4);
   TEST_ASSERT_EQUAL_INT(NULL, vec_set(&vec2, 4, val1, &rtv));
   TEST_ASSERT_EQUAL_INT(VEC_OK, rtv);
   TEST_ASSERT_EQUAL_INT(num+1, vec_num_entries(vec2));

   // second insert should return the previous value at this location (we'll just compare the pointers)
   TEST_ASSERT_EQUAL_INT(val1, vec_set(&vec2, 4, valI, &rtv));
   TEST_ASSERT_EQUAL_INT(VEC_OK, rtv);
   TEST_ASSERT_EQUAL_INT(num+1, vec_num_entries(vec2));
   
   // lastly delete this entry to avoid potential errors since it was not allocated with malloc/pools
   TEST_ASSERT_EQUAL_INT(valI, vec_remove(&vec2, 4, &rtv));
   TEST_ASSERT_EQUAL_INT(VEC_OK, rtv);
   TEST_ASSERT_EQUAL_INT(num, vec_num_entries(vec2));

   vec_release(&vec2, 0);

   // NOTE: We depend on valgrind analysis to confirm that all resources have really been freed.
}

void test_vecuvast(void)
{
   // Create a vector

   // Add an item
   // Get vector number of item
   // Test Vector find idx
   // Add item a second time
   // Verify vector still has the same number of items -- vector should not allow duplicates

   // Test Vector get
   // Test Vector comp
   // Test Vector copy
   
   // Cleanup
   // vector
}

void test_vecblob(void)
{
   // TODO
}

int main(void)
{
   UNITY_BEGIN();

   RUN_TEST(test_create_basic);
   RUN_TEST(test_vecstring);
   RUN_TEST(test_vecuvast);
   // TODO: Test a stack VECTOR (str is no longer a stack).  Including vec_del, vec_set
   /*
     vec_lock / vec_unlock
     vec_push - simple wrapper, we don't need to test it
     vec_set

     vecit_*
     vec_blob_*
     vec_uvast*

    */

   
   return UNITY_END();
}
