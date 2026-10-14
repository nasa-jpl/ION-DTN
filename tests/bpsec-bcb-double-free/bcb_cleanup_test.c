/*
 * bcb_cleanup_test.c -- regression for the BCB AES-GCM decryption-failure
 * cleanup defect (node-wide DoS on a rejected payload BCB).
 *
 * On a decryption failure the payload BCB acceptor path, bpsec_bagscu_zcoCompute(),
 * freed the caller's target ZCO and then cancelled its SDR transaction.  On a
 * non-reversible SDR (configFlags 1) the acceptor therefore aborts the process
 * on a single tampered or key-mismatched bundle -- the freed handle is left for
 * the acquisition cleanup to free again, and the cancel of a modified
 * non-reversible transaction is itself unrecoverable.
 *
 * This harness drives bpsec_bagscu_zcoCompute() directly with the RFC 9173
 * Appendix A.3 test vectors (public values).  For each case it checks that:
 *   - a correct key/tag decrypts (result 0) and recovers the plaintext;
 *   - a wrong key, wrong tag, or tampered ciphertext is rejected (result ERROR)
 *     WITHOUT aborting, with the caller's handle and bytes left intact, and
 *     with the SDR heap returned to its starting level (no leak, no double free).
 *
 * On the vulnerable code the negative cases abort the process, so the test
 * fails.  A default (NULL_SUITES) build has no tag to verify, so the dotest
 * skips before running this harness.
 */

#include <ion.h>
#include <csi.h>

#include <stdio.h>
#include <string.h>

extern int bpsec_bagscu_zcoCompute(uint32_t, SdrObject *, csi_val_t,
		csi_cipherparms_t *, uint8_t);

/* RFC 9173 Appendix A.3 payload BCB vectors (AES-128-GCM). */
static unsigned char ciphertext[] = {
	0x3a,0x09,0xc1,0xe6,0x3f,0xe2,0x3a,0x7f,0x66,0xa5,0x9c,0x73,0x03,0x83,0x72,0x41,
	0xe0,0x70,0xb0,0x26,0x19,0xfc,0x59,0xc5,0x21,0x4a,0x22,0xf0,0x8c,0xd7,0x07,0x95,
	0xe7,0x3e,0x9a
};
static unsigned char key[] = "qwertyuiopasdfgh";
static unsigned char iv[] = "Twelve121212";
static unsigned char tag[] = {
	0xef,0xa4,0xb5,0xac,0x01,0x08,0xe3,0x81,
	0x6c,0x56,0x06,0x47,0x98,0x01,0xbc,0x04
};
static unsigned char aad[] = {0};
static char plaintext[] = "Ready to generate a 32-byte payload";

static int one_case(Sdr sdr, const char *mode)
{
	SdrObject		bytes, object, original;
	unsigned char		data[sizeof ciphertext], k[16], t[16];
	unsigned char		output[sizeof ciphertext];
	csi_val_t		secret;
	csi_cipherparms_t	parms;
	ZcoReader		reader;
	SdrUsageSummary		before, after;
	int			result, negative = strcmp(mode, "positive") != 0;

	if (!sdr_begin_xn(sdr)) return 2;
	sdr_usage(sdr, &before);
	memcpy(data, ciphertext, sizeof data);
	memcpy(k, key, sizeof k);
	memcpy(t, tag, sizeof t);
	if (!strcmp(mode, "wrong-key")) k[0] ^= 1;
	if (!strcmp(mode, "wrong-tag")) t[0] ^= 1;
	if (!strcmp(mode, "wrong-ciphertext")) data[0] ^= 1;

	bytes = sdr_malloc(sdr, sizeof data);
	if (!bytes) { oK(sdr_end_xn(sdr)); return 3; }
	sdr_write(sdr, bytes, (char *) data, sizeof data);
	object = zco_create(sdr, ZcoSdrSource, bytes, 0, -(vast) sizeof data, ZcoInbound);
	if (!object || object == (SdrObject) ERROR) { oK(sdr_end_xn(sdr)); return 4; }
	original = object;

	memset(&parms, 0, sizeof parms);
	secret.len = sizeof k; secret.contents = k;
	parms.iv.len = 12; parms.iv.contents = iv;
	parms.icv.len = sizeof t; parms.icv.contents = t;
	parms.aad.len = sizeof aad; parms.aad.contents = aad;

	result = bpsec_bagscu_zcoCompute(CSTYPE_AES128_GCM, &object, secret,
			&parms, CSI_SVC_DECRYPT);
	printf("mode=%s conversion=%d original=%llu current=%llu\n", mode, result,
			(unsigned long long) original, (unsigned long long) object);
	fflush(stdout);

	if ((negative && result != ERROR) || (!negative && result != 0)) return 5;
	if (negative && object != original) return 6;

	zco_start_transmitting(object, &reader);
	if (zco_transmit(sdr, &reader, sizeof output, (char *) output) != sizeof output)
	{
		return 7;
	}

	if (memcmp(output, negative ? data : (unsigned char *) plaintext, sizeof output))
	{
		return 8;
	}

	zco_destroy(sdr, object);
	sdr_usage(sdr, &after);
	printf("heap allocated before=%llu after=%llu\n",
			(unsigned long long) before.largePoolAllocated,
			(unsigned long long) after.largePoolAllocated);
	if (sdr_end_xn(sdr) < 0) return 10;
	if (before.largePoolAllocated != after.largePoolAllocated) return 9;
	return 0;
}

int main(int argc, char **argv)
{
	static const char	*modes[] = {
		"positive", "wrong-key", "wrong-tag", "wrong-ciphertext"
	};
	Sdr			sdr;
	int			i, rc;

	if (ionAttach() < 0)
	{
		fprintf(stderr, "SKIP: can't attach to ION.\n");
		return 2;
	}
	sdr = getIonsdr();

	/* ZCO initializes its persistent accounting metadata on first use;
	 * do that here so the per-case heap-balance check is exact.	*/
	if (!sdr_begin_xn(sdr)) return 1;
	{
		SdrObject warmup = zco_create(sdr, ZcoSdrSource, 0, 0, 0, ZcoInbound);
		if (!warmup || warmup == (SdrObject) ERROR) { oK(sdr_end_xn(sdr)); return 1; }
		zco_destroy(sdr, warmup);
	}
	if (sdr_end_xn(sdr) < 0) return 1;

	for (i = 0; i < (int) (sizeof modes / sizeof modes[0]); i++)
	{
		rc = one_case(sdr, modes[i]);
		if (rc != 0)
		{
			fprintf(stderr, "FAIL: mode=%s rc=%d\n", modes[i], rc);
			writeErrmsgMemos();
			ionDetach();
			return 1;
		}
	}

	writeErrmsgMemos();
	ionDetach();
	return 0;
}
