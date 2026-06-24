/*
	sdr_logmode_test.c:	Regression test for heap-tier-coupled default
	selection of the SDR reversibility log location.

	Black-box: the log MODE is observed by the side effect that only the
	file tier creates "<pathName>/<name>.sdrlog".  Reversal parity is
	exercised in-process (sdr_cancel_xn drives reverseTransaction against
	the shm log).  All cases run deterministically on every build config;
	no PMEM, robust-mutex build, or special hardware required.

	Case A  auto => memory for DRAM-only : no .sdrlog created.
	Case B  auto => file for SDR_IN_FILE : .sdrlog created.
	Case C  explicit honored             : logSize 0 => file; logSize N => memory.
	Case D  memory-log reversal parity   : a committed object overwritten
	                                       inside a transaction is restored
	                                       from the shm log on reversal.
	Case E  overflow fails loud, no corrupt : undersized logSize => the
	                                       transaction fails and is fully
	                                       reversed (free space intact).

	Prints "PASS"/"FAIL: <reason>"; exits 0/1.  Exits 2 (skip) when the
	AUTO-default support is not present in the build.

	Copyright (c) 2026, California Institute of Technology.
	All rights reserved.  U.S. Government Sponsorship acknowledged.
									*/

#include "platform.h"
#include "sdr.h"

#define WM_SIZE		(2 * 1024 * 1024)	/* SDR working memory	*/
#define HEAP_WORDS	(262144)	/* ~2 MiB heap on 64-bit		*/
#define PATTERN_LEN	(64)
#define CAT_NAME	"logobj"

#ifndef ION_LOGSIZE_AUTO
int	main(int argc, char **argv)
{
	PUTS("SKIP: build lacks heap-tier-coupled log default (ION_LOGSIZE_AUTO).");
	return 2;
}
#else

static char	gWorkDir[256];

static void	logFilePath(const char *sdrName, char *out, size_t outlen)
{
	isprintf(out, outlen, "%s%c%s.sdrlog", gWorkDir,
			ION_PATH_DELIMITER, sdrName);
}

static int	logFileExists(const char *sdrName)
{
	char		path[300];
	struct stat	sb;

	logFilePath(sdrName, path, sizeof path);
	return (stat(path, &sb) == 0);
}

/*	Load a profile.  logSize == ION_LOGSIZE_AUTO exercises the new
 *	normalization; an explicit value (including 0) overrides it.		*/
static int	load(char *name, int configFlags, size_t logSize)
{
	return sdr_load_profile(name, configFlags, HEAP_WORDS, SM_NO_KEY,
			logSize, SM_NO_KEY, gWorkDir, NULL);
}

static void	fillPattern(char *buf, int seed)
{
	int	i;

	for (i = 0; i < PATTERN_LEN; i++) buf[i] = (char) (seed + i);
}

static int	patternMatches(char *buf, int seed)
{
	char	expect[PATTERN_LEN];

	fillPattern(expect, seed);
	return (memcmp(buf, expect, PATTERN_LEN) == 0);
}

static SdrObject	writeCommitted(Sdr sdr, int seed)
{
	char	pattern[PATTERN_LEN];
	SdrObject	obj;

	fillPattern(pattern, seed);
	CHKZERO(sdr_begin_xn(sdr));
	obj = sdr_malloc(sdr, PATTERN_LEN);
	if (obj == 0) { sdr_cancel_xn(sdr); return 0; }
	sdr_write(sdr, obj, pattern, PATTERN_LEN);
	sdr_catlg(sdr, CAT_NAME, 0, obj);	/*	void; can't fail	*/
	if (sdr_end_xn(sdr) < 0) return 0;
	return obj;
}

/*	NB: there is no per-case teardown.  sdr_stop_using()/sdr_destroy()
 *	both DETACH this process from the shared SDR working memory (and
 *	reset the _sdrwm handle), which would break every subsequent case.
 *	The cases use distinct SDR names and coexist in one working memory;
 *	it is released exactly once, by sdr_shutdown() at the end of main().
 *	Stray dataspace/log shm segments are reclaimed by 'killm', as with
 *	the other ICI unit tests.					*/

/* ---- Case A: auto => memory for DRAM-only --------------------------- */
static int	caseAutoMemory(void)
{
	char		*name = "logA";
	Sdr		sdr;

	if (load(name, SDR_IN_DRAM | SDR_REVERSIBLE, ION_LOGSIZE_AUTO) < 0)
		{ PUTS("FAIL: load (A)."); return -1; }
	sdr = sdr_start_using(name);
	if (sdr == NULL) { PUTS("FAIL: start_using (A)."); return -1; }

	if (writeCommitted(sdr, 0x10) == 0) { PUTS("FAIL: commit (A)."); return -1; }

	if (logFileExists(name)) {
		PUTS("FAIL: .sdrlog created for DRAM-only auto (expected memory log).");
		return -1;
	}
	PUTS("Case A (auto=>memory, DRAM-only): PASS");
	return 0;
}

/* ---- Case B: auto => file for SDR_IN_FILE --------------------------- */
static int	caseAutoFile(void)
{
	char		*name = "logB";
	Sdr		sdr;

	if (load(name, SDR_IN_DRAM | SDR_IN_FILE | SDR_REVERSIBLE,
			ION_LOGSIZE_AUTO) < 0)
		{ PUTS("FAIL: load (B)."); return -1; }
	sdr = sdr_start_using(name);
	if (sdr == NULL) { PUTS("FAIL: start_using (B)."); return -1; }

	if (writeCommitted(sdr, 0x20) == 0) { PUTS("FAIL: commit (B)."); return -1; }

	if (!logFileExists(name)) {
		PUTS("FAIL: no .sdrlog for SDR_IN_FILE auto (expected file log).");
		return -1;
	}
	PUTS("Case B (auto=>file, SDR_IN_FILE): PASS");
	return 0;
}

/* ---- Case C: explicit logSize honored ------------------------------- */
static int	caseExplicit(void)
{
	char		*nameF = "logCf";
	char		*nameM = "logCm";
	Sdr		sdr;

	/* explicit 0 with DRAM-only => file */
	if (load(nameF, SDR_IN_DRAM | SDR_REVERSIBLE, 0) < 0)
		{ PUTS("FAIL: load (C/file)."); return -1; }
	sdr = sdr_start_using(nameF);
	if (sdr == NULL || writeCommitted(sdr, 0x30) == 0)
		{ PUTS("FAIL: commit (C/file)."); return -1; }
	if (!logFileExists(nameF)) {
		PUTS("FAIL: explicit logSize 0 did not select file log.");
		return -1;
	}

	/* explicit N>0 => memory */
	if (load(nameM, SDR_IN_DRAM | SDR_REVERSIBLE, 1024 * 1024) < 0)
		{ PUTS("FAIL: load (C/mem)."); return -1; }
	sdr = sdr_start_using(nameM);
	if (sdr == NULL || writeCommitted(sdr, 0x31) == 0)
		{ PUTS("FAIL: commit (C/mem)."); return -1; }
	if (logFileExists(nameM)) {
		PUTS("FAIL: explicit logSize>0 created a file log.");
		return -1;
	}
	PUTS("Case C (explicit honored): PASS");
	return 0;
}

/* ---- Case D: memory-log reversal parity ---------------------------- */
/*	The log-LOCATION change moves undo records from a file into shm.
 *	The property that must be preserved is that reverseTransaction
 *	reconstructs a committed object from those shm-resident undo
 *	records.  This case proves it directly and deterministically:
 *	commit an object, overwrite it inside a transaction, then reverse
 *	(sdr_cancel_xn) and confirm the committed value is restored from
 *	the memory log.
 *
 *	NB: this is intentionally NOT a fork()/SIGKILL test.  Genuine
 *	cross-process rollback of an orphaned transaction runs the SAME
 *	memory-branch reverseTransaction code, but only fires via the
 *	robust-mutex EOWNERDEAD path (recoverOrphanedXn) -- which is
 *	compiled in only when ION_HAVE_ROBUST_MUTEX is configured, so a
 *	fork-based assertion is not portable across build configs.  And
 *	sdr_reload_profile cannot stand in for it on a DRAM-only heap:
 *	reload re-initializes the dataspace (initSdrMap), wiping the very
 *	object the rollback would restore.				*/
static int	caseMemoryRecovery(void)
{
	char		*name = "logD";
	Sdr		sdr;
	SdrObject		obj;
	char		buf[PATTERN_LEN];
	char		overwrite[PATTERN_LEN];

	if (load(name, SDR_IN_DRAM | SDR_REVERSIBLE, ION_LOGSIZE_AUTO) < 0)
		{ PUTS("FAIL: load (D)."); return -1; }
	sdr = sdr_start_using(name);
	if (sdr == NULL) { PUTS("FAIL: start_using (D)."); return -1; }

	obj = writeCommitted(sdr, 0x10);
	if (obj == 0) { PUTS("FAIL: seed (D)."); return -1; }

	fillPattern(overwrite, 0xA0);
	CHKERR(sdr_begin_xn(sdr));
	sdr_write(sdr, obj, overwrite, PATTERN_LEN);
	sdr_cancel_xn(sdr);		/* reverse from the shm log */

	CHKERR(sdr_begin_xn(sdr));
	sdr_read(sdr, buf, obj, PATTERN_LEN);
	sdr_end_xn(sdr);
	if (!patternMatches(buf, 0x10)) {
		PUTS("FAIL: memory-log did not roll back uncommitted write.");
		return -1;
	}
	PUTS("Case D (memory-log reversal parity): PASS");
	return 0;
}

/* ---- Case E: overflow fails loud, no corruption --------------------- */
/*	A memory log is fixed-size and fails (does not grow) on overflow.
 *	Drive a transaction past a small log and assert (a) it fails loud
 *	rather than silently truncating the undo trail, and (b) the failed
 *	transaction is fully reversed -- no heap leaked -- so the SDR stays
 *	usable.  Free-space-before == free-space-after is the free-list
 *	invariant check; we deliberately avoid sdr_reload_profile here,
 *	which on a DRAM-only heap re-inits the dataspace and would mask any
 *	corruption with a trivially-fresh map.				*/
#define OVERFLOW_LOG_SIZE	(512)
#define OVERFLOW_OBJ_LEN	(4096)	/* undo bytes >> log size */
static int	caseOverflow(void)
{
	static char	big[OVERFLOW_OBJ_LEN];
	char		*name = "logE";
	Sdr		sdr;
	SdrObject		obj;
	size_t		freeBefore;
	size_t		freeAfter;
	int		rc;

	if (load(name, SDR_IN_DRAM | SDR_REVERSIBLE, OVERFLOW_LOG_SIZE) < 0)
		{ PUTS("FAIL: load (E)."); return -1; }
	sdr = sdr_start_using(name);
	if (sdr == NULL) { PUTS("FAIL: start_using (E)."); return -1; }

	CHKERR(sdr_begin_xn(sdr));
	freeBefore = sdr_unused(sdr);
	sdr_end_xn(sdr);			/* read-only: nothing logged */

	CHKERR(sdr_begin_xn(sdr));
	obj = sdr_malloc(sdr, OVERFLOW_OBJ_LEN);
	if (obj == 0) { sdr_cancel_xn(sdr); PUTS("FAIL: malloc (E)."); return -1; }
	memset(big, 0xE0, sizeof big);
	sdr_write(sdr, obj, big, OVERFLOW_OBJ_LEN);	/* undo trail > log */
	rc = sdr_end_xn(sdr);			/* expect failure via crashXn */

	if (rc == 0) {
		PUTS("FAIL: oversized transaction did not fail on small log.");
		return -1;
	}

	/* The failed transaction must have been fully reversed. */
	CHKERR(sdr_begin_xn(sdr));
	freeAfter = sdr_unused(sdr);
	sdr_end_xn(sdr);
	if (freeAfter != freeBefore) {
		PUTS("FAIL: overflow left the heap altered (not fully reversed).");
		return -1;
	}
	PUTS("Case E (overflow fails loud, no corruption): PASS");
	return 0;
}

int	main(int argc, char **argv)
{
	istrcpy(gWorkDir, (argc > 1) ? argv[1] : "/tmp", sizeof gWorkDir);

	if (sdr_initialize(WM_SIZE, NULL, SM_NO_KEY, NULL) < 0) {
		PUTS("FAIL: sdr_initialize."); return 1;
	}

	if (caseAutoMemory() < 0)     { sdr_shutdown(); return 1; }
	if (caseAutoFile() < 0)       { sdr_shutdown(); return 1; }
	if (caseExplicit() < 0)       { sdr_shutdown(); return 1; }
	if (caseMemoryRecovery() < 0) { sdr_shutdown(); return 1; }
	if (caseOverflow() < 0)       { sdr_shutdown(); return 1; }

	sdr_shutdown();
	PUTS("PASS");
	return 0;
}

#endif	/* ION_LOGSIZE_AUTO */
