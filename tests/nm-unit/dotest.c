/*
  Network Manager Unit Tests

Test Architecture:
- ION fail_unless() used to count sub-tests in this file. 
- Subtest functions will return 1 on success, 0 otherwise using ocal test macros

TODO:
- Split into seperate files for each unit under test, making this file a driver only

 */
#include <stdio.h>

// Headers under test
#include "shared/utils/utils.h"
#include "shared/msg/msg.h"

// Test Utilities
#include "check.h"
#include "testutil.h"
#include "unity.h"

#define check(bool) if (!(bool)) { printf("END: FAILED at %s:%d after %i sub-checks passed\n", __FILE__, __LINE__, group_pass); return 0; } else { group_pass++; }
#define end_group() printf("END: %i checks passed\n", group_pass); return 1;

void adm_init_stub() { }

int num_groups = 0, group_pass = 0, group_fail = 0;
void init_group(char* str) {
   printf("START: %s\n", str);
   num_groups++;
   group_pass = 0;
}


int test_cnt = 0;

ari_t* test_ari_deserialize_raw(char* input)
{
   int success = AMP_FAIL;
   blob_t *buf, *buf2;
   test_cnt++;
   
   // Construct input Data Blob
   buf = utils_string_to_hex(input);
   check(buf != NULL);

   ari_t *rtv = ari_deserialize_raw(buf, &success);

   // Verify Status
   if (rtv == NULL && success == AMP_OK)
   {
      printf("ERROR in test %d: ari_deserialize_raw() returned OK but gave NULL result\n", test_cnt);
   } else if (success != AMP_OK && rtv != NULL) {
      printf("ERRORin test %d: ari_deserialize_raw() returned a result but indicated failure\n", test_cnt);
      ari_release(rtv, 1);
      blob_release(buf, 1); // Destroy the blob
      return NULL;
   }

   // Verify Ability to re-encode (set failure and return NULL on error)
   buf2 = ari_serialize_wrapper(rtv);
   if (buf2 == NULL)
   {
      printf("ERROR in test %d: ari_serialize_wrapper() failed to re-encode", test_cnt);
      ari_release(rtv, 1);
      blob_release(buf, 1); // Destroy the blob
      return NULL;
   }

   // Compare the blobs
   if (blob_compare(buf, buf2) != 0)
   {
      printf("ERROR in test %d: Input does not match deserialized output\n", test_cnt);
      char *msg_str = utils_hex_to_string(buf2->value, buf2->length);
      printf("Input:  0x%s\nOutput: %s\n", input, msg_str);
         SRELEASE(msg_str);
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
int test_msg_grp_deserialize() { // TODO: Remove or finish this function
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

void msg_encoding_tests()
{
#if 0 // AMPv6 Tests
   
   // Prototype test
   fail_unless(test_msg_grp_deserialize());

   // The following tests do not include intermediate verification
   fail_unless(test_simple_ari("BP Report CTRL", "c11541054f840542252347814587182d41004180", AMP_TYPE_CTRL));

   fail_unless(test_msg_grp("BP Report CTRL Response", "8200585a01816769706e3a312e3181834587181941001a5d76ac5c5841920550121214141414141414141414141414144a69616d705f6167656e74456476332e314106411641014100410041004100421825410241014100410141014100"));

   fail_unless(test_simple_ari("Generate ADM Report and BP Report Concurrently.", "c11541055584054225234d824587181941004587182d41004180", AMP_TYPE_CTRL));

   fail_unless(test_msg_grp("ADM Report and BP Report Concurrently (response).", "8200585a01816769706e3a312e3181834587181941001a5d76ac5c5841920550121214141414141414141414141414144a69616d705f6167656e74456476332e314106411641014100410041004100421825410241014100410141014100"));

   // Step 2.1
   fail_unless(test_simple_ari("2.1: Generated Endpoint report for endpoint", "c1154105581d8405422523558153c7182d41014d83054112486769706e3a312e314180", AMP_TYPE_CTRL));
   fail_unless(test_msg_grp("2.2: Verify endpoint report received for selected endpoint", "8200582701816769706e3a312e31818353c7182d41014d83054112486769706e3a312e311a5d79808c4180"));

#else // New AMPv7 Tests
   fail_unless(test_simple_ari("ari:/IANA:Amp.Agent/Ctrl.gen_rpts([ari:/IANA:DTN.bpsec/Rptt.source_report(ipn:1.1)],[])", "c11541050502252381c7185541010501126769706e3a312e3100", AMP_TYPE_CTRL));

   fail_unless(test_simple_ari("ari:/IANA:DTN.bp_agent/CTRL.reset_all_counts()", "8118294100", AMP_TYPE_CTRL));
   
   fail_unless(test_simple_ari("ari:/IANA:Amp.agent/CTRL.gen_rpts([ari:/ADM:dtn.bp_agent/rptt.full_report],[])",
                               "c11541050502252381374b66756c6c5f7265706f72744341444d4c64746e2e62705f6167656e7400",
                             AMP_TYPE_CTRL));


   fail_unless(test_simple_ari("ari:/IANA:Amp.Agent/Ctrl.gen_rpts([ari:/IANA:dtn.bp_agent/rptt.endpoint_report(\"ipn:1.1\"), ari:/IANA:dtn.bp_agent/rptt.endpoint_report(\"ipn:1.2\")], [])",
                               "c11541050502252382c7182d4101050112692269706e3a312e3122c7182d4101050112692269706e3a312e322200",
                               AMP_TYPE_CTRL));

   fail_unless(test_simple_ari("ari:/IANA:Amp.Agent/Ctrl.add_tbr(ari:/test:/TBR.t1, 0, 10, 20, [ari:/IANA:amp.agent/ctrl.gen_rpts([ari:/IANA:DTN.bp_agent/rptt.full_report],[])], TBR1)",
                               "c115410a05062420201625122b4274314474657374000a1481c1154105050225238187182d4100006454425231",
                               AMP_TYPE_CTRL));

   

   fail_unless(test_simple_ari("ari:/IANA:Amp.Agent/Ctrl.add_sbr(ari:/test:/SBR.s1, 0, (BOOL)[ari:/test:/VAR.y, UINT.4, ari:/IANA/Amp.Agent/Op.greaterEqual], 20, 10, [ari:/IANA:amp.agent/ctrl.gen_rpts([ari:/IANA:DTN.bp_agent/rptt.full_report],[])], STR.SBR1)",
                               "c115410b0507242026161625122842733144746573740010832c417944746573744304851818412f140A81c1154105050225238187182D4100006453425231",
                               AMP_TYPE_CTRL));

   fail_unless(test_simple_ari("add_rptt", "c115410205022425a4014101410182821041018c181d4100", AMP_TYPE_CTRL));
#endif

}

int test_msgs_raw(char* input) {
   int success = AMP_FAIL;
   int rtv = 1;
   blob_t *buf, *buf2;
   msg_grp_t *grp;

   // Construct input Data Blob
   buf = utils_string_to_hex(input);
   check(buf != NULL);

   grp = msg_grp_deserialize(buf, &success);

   buf2 = msg_grp_serialize_wrapper(grp);
  
   // Compare the blobs
   if (blob_compare(buf, buf2) != 0)
   {
      printf("ERROR in test %d: Input does not match deserialized output\n", test_cnt);
      char *msg_str = utils_hex_to_string(buf2->value, buf2->length);
      printf("Input:  0x%s\nOutput: %s\n", input, msg_str);
      SRELEASE(msg_str);
      rtv = 0;
   }
   msg_grp_release(grp, 1);
   blob_release(buf, 1);
   blob_release(buf2, 1);

   return rtv;
}
void msgs_encoding_tests() {
   fail_unless(test_msgs_raw("0x8200587801816869706e3a322e36358383c115410405012581a701410141011a5e83f8b805012300838a181b41021a5e83f8b8050109878a181b4102050124c7182d410100050124c718414100000501248718cd410005012487182d41000501248718194100050124c718cd41010083a701410141011a5e83f8b800"));
}


int main(int argc, char **argv)
{
   printf("Test Suite Built on %s %s\n", __DATE__, __TIME__);
#if 0
   // Setup
   if (ionAttach() < 0)
   {
      AMP_DEBUG_ERR("nm_dotest", "can't attach to ION.", NULL);
      return -1;
   }
#endif
   
   utils_mem_int(); // Initialize utils
   db_init("test_db", &adm_init_stub); // Initialize global structures (used in some nested functions)
   
   msg_encoding_tests();

   msgs_encoding_tests();
   
   CHECK_FINISH;
}
