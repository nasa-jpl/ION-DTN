/*
  Network Manager Unit Tests

Test Architecture:
- ION fail_unless() used to count sub-tests in this file. 
- Subtest functions will return 1 on success, 0 otherwise using ocal test macros

TODO:
- Split into seperate files for each unit under test, making this file a driver only

 */
#include <stdio.h>

// Test Utilities
#include "check.h"
#include "testutil.h"

#define check(bool) if (!(bool)) { printf("END: FAILED at %s:%d after %i sub-checks passed\n", __FILE__, __LINE__, group_pass); return 0; } else { group_pass++; }
#define end_group() printf("END: %i checks passed\n", group_pass); return 1;

int num_groups = 0, group_pass = 0, group_fail = 0;
void init_group(char* str) {
   printf("START: %s\n", str);
   num_groups++;
   group_pass = 0;
}



/*** nm/shared/msg.c ***/
#include "shared/utils/utils.h"
#include "shared/msg/msg.h"

ari_t* test_ari_deserialize_raw(char* input)
{
   int success = AMP_FAIL;
   blob_t *buf, *buf2;
   
   // Construct input Data Blob
   buf = utils_string_to_hex(input);
   check(buf != NULL);

   ari_t *rtv = ari_deserialize_raw(buf, &success);

   // Verify Status
   if (rtv == NULL && success == AMP_OK)
   {
      printf("ERROR: ari_deserialize_raw() returned OK but gave NULL result\n");
   } else if (success != AMP_OK && rtv != NULL) {
      printf("ERROR: ari_deserialize_raw() returned a result but indicated failure\n");
      ari_release(rtv, 1);
      blob_release(buf, 1); // Destroy the blob
      return NULL;
   }

   // Verify Ability to re-encode (set failure and return NULL on error)
   buf2 = ari_serialize_wrapper(rtv);
   if (buf2 == NULL)
   {
      printf("ERROR: ari_serialize_wrapper() failed to re-encode");
      ari_release(rtv, 1);
      blob_release(buf, 1); // Destroy the blob
      return NULL;
   }

   // Compare the blobs
   if (blob_compare(buf, buf2) != 0)
   {
      printf("ERROR: Input does not match deserialized output\n");
      ari_release(rtv, 1);
      rtv = NULL;
   }
   
   blob_release(buf, 1); // Destroy the blob
   blob_release(buf2, 1); // Destroy the blob   
   
   return rtv;
}

int test_simple_ari(char *desc, char* cbor, amp_type_e type)
{
   int rtv = 1;
   ari_t *ari = test_ari_deserialize_raw(cbor);

   // Placeholder; This should be replaced with a TAP lib fn
   if (ari == NULL || ari->type != type)
   {
      printf("not ok - %s; Unexpected type: %i != %i\n", desc, type, ari->type);
      rtv = 0;
   }
   else
   {
      printf("ok - %s\n", desc);
      rtv = 1;
   }
   
   ari_release(ari, 1);

   return rtv;
}

/* PROTOTYPE FUNCTION for initial experimentation (not final test)
 * - This is the first function called when decoding a received (CBOR) message
 * - 
 */
int test_msg_grp_deserialize() {
   msg_grp_t *grp = NULL;
   int success = AMP_FAIL;

   init_group("TEST msg_grp_deserialize");
   

   // Construct input Data Blob
   /* "Generate Agent ADM Full Report" command from amp_agent_suite.txt
    * type = AMP_TYPE_CTRL
    * ari_reg_t as_reg
    */
   ari_t *ari = test_ari_deserialize_raw("c11541054f840542252347814587182d41004180");
   check(ari != NULL);

   // Verify ARI Properties
   check(ari->type == AMP_TYPE_CTRL);
   check(ari->as_reg.flags == 0xC1);
   check(ari->as_reg.nn_idx == 0);
   // TODO: Verify remaining fields
   
   // And cleanup
   ari_release(ari, 1);
   
#if 0 // Requires a fully constructed message, not just the debug part
   // Attempt to Deserialize
   grp = msg_grp_deserialize(buf, &success);
   check(success==AMP_OK);
   check(grp!=NULL);

   // Verify Deserialized Message
   // TODO

   // Free Message
   msg_grp_release(grp, 1);
#endif
   
   end_group();

   return 1;
}

int test_msg_grp(char *desc, char* cbor) {
   vecit_t it;
   
   // Setup
   init_group("TEST test_msg_grp");   
   int success, cnt=0;
   blob_t *buf = utils_string_to_hex(cbor);

   // Decode Group
   msg_grp_t *grp = msg_grp_deserialize(buf, &success);
   check(success == AMP_OK);
   check(grp != NULL);

   // Decode Messages in group
   for(it = vecit_first(&(grp->msgs)); vecit_valid(it); it = vecit_next(it), cnt++)
   {
      vec_idx_t i = vecit_idx(it);
      blob_t *msg_data = (blob_t*) vecit_data(it);
      /* Get the message type. */
      int msg_type = msg_grp_get_type(grp, i);

      success = AMP_FAIL;
      check(msg_type == MSG_TYPE_RPT_SET);

      msg_rpt_t *rpt_msg = msg_rpt_deserialize(msg_data, &success);
      check(success == AMP_OK);
      check(rpt_msg != NULL);
      // TOOD: VERIFY

      msg_rpt_release(rpt_msg, 1);
      
   }

   // Re-encode the group
   blob_t *buf2 = msg_grp_serialize_wrapper(grp);
   
   // Verify
   check(buf2 != NULL);
   check( blob_compare(buf,buf2) == 0);

   // And Cleanup
   blob_release(buf,1);
   blob_release(buf2,1);
   msg_grp_release(grp, 1);

   printf("\tDEBUG: msgs in group=%d\n", cnt);

   end_group();
   return 1;
}


int main(int argc, char **argv)
{
   ari_t *ari;
   
   printf("Test Suite Built on %s %s\n", __DATE__, __TIME__);
   // Setup
   // TODO: Initialize Ion ICI (for now, assume ion is already running)
   
   utils_mem_int(); // Initialize utils

   // Prototype test
   fail_unless(test_msg_grp_deserialize());

   // The following tests do not include intermediate verification
   fail_unless(test_simple_ari("BP Report CTRL", "c11541054f840542252347814587182d41004180", AMP_TYPE_CTRL));

   fail_unless(test_msg_grp("BP Report CTRL Response", "8200585a01816769706e3a312e3181834587181941001a5d76ac5c5841920550121214141414141414141414141414144a69616d705f6167656e74456476332e314106411641014100410041004100421825410241014100410141014100"));

   fail_unless(test_simple_ari("Generate ADM Report and BP Report Concurrently.", "c11541055584054225234d824587181941004587182d41004180", AMP_TYPE_CTRL));

   fail_unless(test_msg_grp("ADM Report and BP Report Concurrently (response).", "8200585a01816769706e3a312e3181834587181941001a5d76ac5c5841920550121214141414141414141414141414144a69616d705f6167656e74456476332e314106411641014100410041004100421825410241014100410141014100"));
   
   CHECK_FINISH;
}
