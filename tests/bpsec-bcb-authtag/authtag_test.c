/*
 * authtag_test.c -- regression for the BCB AES-GCM authentication-tag bypass.
 *
 * ION's native BPSec decrypts a BCB-protected payload block through the
 * streaming cipher-suite interface: csi_crypt_start(), csi_crypt_update() for
 * each chunk, then csi_crypt_finish().  On decryption the AES-GCM
 * authentication tag carried on the wire is an input to be verified.  The
 * finish step used mbedtls_gcm_finish(), whose tag argument is an OUTPUT: it
 * overwrote the received tag with the freshly computed one and never compared
 * them, so any tampered ciphertext "decrypted" successfully and the corrupted
 * plaintext was accepted.  (The extension-block path uses a different,
 * one-shot call that does verify; only the payload path was affected.)
 *
 * This harness drives the same csi_crypt_start/update/finish calls the payload
 * BCB path uses.  For AES-128-GCM and AES-256-GCM it:
 *   1. encrypts a known plaintext, capturing ciphertext and tag;
 *   2. decrypts the untampered ciphertext -- this must succeed AND recover the
 *      original plaintext (so the fix does not break legitimate decryption);
 *   3. decrypts with one ciphertext byte flipped -- this must FAIL;
 *   4. decrypts with one tag byte flipped -- this must FAIL.
 *
 * On the vulnerable code the tampered cases (3) and (4) "succeed", so this test
 * fails.  On the fixed code they are rejected.
 *
 * csi_crypt_update() allocates from ION working memory, so the harness attaches
 * to a running node; the dotest starts a minimal one.  Built with
 * --enable-crypto-mbedtls -- a default (NULL_SUITES) build makes the cipher
 * suite a no-op, so the dotest skips unless real crypto is present.
 */

#include <ion.h>
#include <platform.h>
#include <csi.h>

#include <stdio.h>
#include <string.h>

#define TAG_LEN		(16)

static const unsigned char KEY16[16] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
};
static const unsigned char KEY32[32] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
};
static const unsigned char IV12[12] = {
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5,
	0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab
};
static const unsigned char PLAINTEXT[] =
	"The quick brown fox jumps over the lazy dog -- twice.";
#define PT_LEN		((int) (sizeof(PLAINTEXT) - 1))

/*	Encrypt PLAINTEXT into ct[PT_LEN] and tag[TAG_LEN].  Returns 0 on
 *	success, -1 on any failure.					*/
static int gcm_encrypt(csi_csid_t suite, csi_val_t key,
		unsigned char *ct, unsigned char *tag)
{
	uint8_t			*ctx;
	csi_cipherparms_t	parms;
	csi_val_t		in;
	csi_val_t		out;

	memset(&parms, 0, sizeof parms);
	parms.iv.contents = (uint8_t *) IV12;
	parms.iv.len = sizeof IV12;

	ctx = csi_ctx_init(suite, key, CSI_SVC_ENCRYPT);
	if (ctx == NULL)
	{
		return -1;
	}

	if (csi_crypt_start(suite, ctx, parms) != 1)
	{
		csi_ctx_free(suite, ctx);
		return -1;
	}

	in.contents = (uint8_t *) PLAINTEXT;
	in.len = PT_LEN;
	out = csi_crypt_update(suite, ctx, CSI_SVC_ENCRYPT, in);
	if (out.contents == NULL || out.len != PT_LEN)
	{
		csi_ctx_free(suite, ctx);
		return -1;
	}

	memcpy(ct, out.contents, PT_LEN);
	MRELEASE(out.contents);

	if (csi_crypt_finish(suite, ctx, CSI_SVC_ENCRYPT, &parms) != 1
			|| parms.icv.contents == NULL
			|| parms.icv.len != TAG_LEN)
	{
		if (parms.icv.contents)
		{
			MRELEASE(parms.icv.contents);
		}

		csi_ctx_free(suite, ctx);
		return -1;
	}

	memcpy(tag, parms.icv.contents, TAG_LEN);
	MRELEASE(parms.icv.contents);
	csi_ctx_free(suite, ctx);
	return 0;
}

/*	Decrypt ct[PT_LEN] with the given tag.  Returns the result of
 *	csi_crypt_finish() (1 on a verified tag, <= 0 on rejection or error).
 *	When the tag verifies and pt is non-NULL, the recovered plaintext is
 *	written there.							*/
static int gcm_decrypt(csi_csid_t suite, csi_val_t key,
		const unsigned char *ct, const unsigned char *tag,
		unsigned char *pt)
{
	uint8_t			*ctx;
	csi_cipherparms_t	parms;
	csi_val_t		in;
	csi_val_t		out;
	unsigned char		tagbuf[TAG_LEN];
	int			rc;

	memset(&parms, 0, sizeof parms);
	parms.iv.contents = (uint8_t *) IV12;
	parms.iv.len = sizeof IV12;
	memcpy(tagbuf, tag, TAG_LEN);
	parms.icv.contents = tagbuf;		/*	Tag from the wire.	*/
	parms.icv.len = TAG_LEN;

	ctx = csi_ctx_init(suite, key, CSI_SVC_DECRYPT);
	if (ctx == NULL)
	{
		return -1;
	}

	if (csi_crypt_start(suite, ctx, parms) != 1)
	{
		csi_ctx_free(suite, ctx);
		return -1;
	}

	in.contents = (uint8_t *) ct;
	in.len = PT_LEN;
	out = csi_crypt_update(suite, ctx, CSI_SVC_DECRYPT, in);
	if (out.contents == NULL)
	{
		csi_ctx_free(suite, ctx);
		return -1;
	}

	if (pt != NULL && out.len == PT_LEN)
	{
		memcpy(pt, out.contents, PT_LEN);
	}

	MRELEASE(out.contents);
	rc = csi_crypt_finish(suite, ctx, CSI_SVC_DECRYPT, &parms);
	csi_ctx_free(suite, ctx);
	return rc;
}

/*	Run the four checks for one suite.  Returns 0 on success, 1 on any
 *	failure (printing the reason).					*/
static int run_suite(const char *name, csi_csid_t suite, csi_val_t key)
{
	unsigned char	ct[PT_LEN];
	unsigned char	tag[TAG_LEN];
	unsigned char	recovered[PT_LEN];
	unsigned char	ct_bad[PT_LEN];
	unsigned char	tag_bad[TAG_LEN];
	int		rc;

	if (gcm_encrypt(suite, key, ct, tag) != 0)
	{
		fprintf(stderr, "FAIL[%s]: encryption failed.\n", name);
		return 1;
	}

	/*	1. Untampered ciphertext must verify and round-trip.	*/

	memset(recovered, 0, sizeof recovered);
	rc = gcm_decrypt(suite, key, ct, tag, recovered);
	if (rc != 1)
	{
		fprintf(stderr, "FAIL[%s]: valid ciphertext was rejected "
				"(rc=%d).\n", name, rc);
		return 1;
	}

	if (memcmp(recovered, PLAINTEXT, PT_LEN) != 0)
	{
		fprintf(stderr, "FAIL[%s]: decryption did not recover the "
				"plaintext.\n", name);
		return 1;
	}

	/*	2. A single flipped ciphertext byte must be rejected.	*/

	memcpy(ct_bad, ct, sizeof ct);
	ct_bad[0] ^= 0x01;
	rc = gcm_decrypt(suite, key, ct_bad, tag, NULL);
	if (rc == 1)
	{
		fprintf(stderr, "FAIL[%s]: tampered CIPHERTEXT was accepted "
				"-- the authentication tag was not verified.\n",
				name);
		return 1;
	}

	/*	3. A single flipped tag byte must be rejected.		*/

	memcpy(tag_bad, tag, sizeof tag);
	tag_bad[0] ^= 0x01;
	rc = gcm_decrypt(suite, key, ct, tag_bad, NULL);
	if (rc == 1)
	{
		fprintf(stderr, "FAIL[%s]: tampered TAG was accepted "
				"-- the authentication tag was not verified.\n",
				name);
		return 1;
	}

	fprintf(stderr, "PASS[%s]: valid ciphertext verified; tampered "
			"ciphertext and tag both rejected.\n", name);
	return 0;
}

int main(int argc, char **argv)
{
	csi_val_t	key16;
	csi_val_t	key32;
	int		failures = 0;

	(void) argc;
	(void) argv;

	if (ionAttach() < 0)
	{
		fprintf(stderr, "SKIP: can't attach to ION.\n");
		return 2;
	}

	key16.contents = (uint8_t *) KEY16;
	key16.len = sizeof KEY16;
	key32.contents = (uint8_t *) KEY32;
	key32.len = sizeof KEY32;

	failures += run_suite("AES-128-GCM", CSTYPE_AES128_GCM, key16);
	failures += run_suite("AES-256-GCM", CSTYPE_AES256_GCM, key32);

	ionDetach();

	if (failures == 0)
	{
		fprintf(stderr, "SUCCESS: BCB AES-GCM decryption verifies the "
				"authentication tag.\n");
		return 0;
	}

	return 1;
}
