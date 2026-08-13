/*
	tests/ltp-budget-unit/dotest.c:	Unit tests for the LTP session
					budget model in getMaxReports.

	These exercise the model arithmetic only; they attach to no ION
	node.  getMaxReports reads its inputs from an LtpVspan and is
	otherwise a pure function, so a stack-resident span is enough.

	The property that matters most here is the one a naive test
	would miss: a test at a single block size passes on the
	unparameterised algorithm too.  Sweeping across the thresholds
	at which the budget steps IS the test.
									*/

#include "platform.h"
#include <math.h>
#include "ltpP.h"
#include "check.h"
#include "testutil.h"

/*	The algorithm as it stood before the session failure target was
 *	introduced, transcribed so that equivalence at a target of 1.0
 *	can be asserted rather than asserted about.  This is frozen
 *	reference behaviour; it must not be "fixed" to track changes in
 *	getMaxReports, since that is precisely what it exists to
 *	detect.								*/

static int	shippingBudget(int redPartLength, unsigned int maxSegmentSize,
			float segmentLossRate)
{
	int	dataGapsPerReport = MAX_CLAIMS_PER_RS - 1;
	int	maxReportSegments = 2;
	int	xmitBytes = redPartLength;
	int	xmitSegments;
	int	dataGaps;
	int	reportsIssued;
	float	lostSegments;

	while (1)
	{
		xmitSegments = xmitBytes / maxSegmentSize;
		lostSegments = xmitSegments * segmentLossRate;
		if (lostSegments < 1.0)
		{
			break;
		}

		dataGaps = (int) lostSegments;
		reportsIssued = dataGaps / dataGapsPerReport;
		if (dataGaps % dataGapsPerReport > 0)
		{
			reportsIssued += 1;
		}

		maxReportSegments += reportsIssued;
		xmitBytes = (int) (lostSegments * maxSegmentSize);
	}

	return maxReportSegments;
}

/*	Computes a budget for a block of n0 segments of the given size.	*/

static int	budget(int n0, unsigned int maxSegmentSize, float lossRate,
			unsigned int rounds, int asReceiver)
{
	LtpVspan	vspan;

	memset((char *) &vspan, 0, sizeof vspan);
	if (asReceiver)
	{
		vspan.maxRecvSegSize = maxSegmentSize;
		vspan.recvSegLossRate = lossRate;
	}
	else
	{
		vspan.maxXmitSegSize = maxSegmentSize;
		vspan.xmitSegLossRate = lossRate;
	}

	return getMaxReports(n0 * ((int) maxSegmentSize), &vspan, asReceiver,
			rounds);
}

/*	Deterministic PRNG.  rand() is not portable across platforms in
 *	its sequence, and these tests assert on measured rates, so the
 *	generator is written out here to keep results reproducible.	*/

static unsigned long long	rngState = 88172645463325252ULL;

static void	rngSeed(unsigned long long seed)
{
	rngState = seed ? seed : 88172645463325252ULL;
}

static double	rngNext(void)
{
	rngState ^= rngState << 13;
	rngState ^= rngState >> 7;
	rngState ^= rngState << 17;
	return (double) (rngState >> 11) / 9007199254740992.0;
}

/*	Simulates the session model: n0 segments, independent per-
 *	segment loss at rate p, one repair round per report, session
 *	cancelled once the rounds available are used up.  Returns the
 *	measured fraction of sessions cancelled.			*/

static double	measureCancelRate(int n0, double p, int maxRounds, int trials)
{
	int	cancels = 0;
	int	trial;
	int	round;
	int	outstanding;
	int	stillLost;
	int	i;

	for (trial = 0; trial < trials; trial++)
	{
		outstanding = 0;
		for (i = 0; i < n0; i++)
		{
			if (rngNext() < p)
			{
				outstanding++;
			}
		}

		round = 0;
		while (outstanding > 0)
		{
			if (round >= maxRounds)
			{
				cancels++;
				break;
			}

			stillLost = 0;
			for (i = 0; i < outstanding; i++)
			{
				if (rngNext() < p)
				{
					stillLost++;
				}
			}

			outstanding = stillLost;
			round++;
		}
	}

	return (double) cancels / (double) trials;
}

int	main(int argc, char **argv)
{
	static int	sweepN0[] = {6, 45, 69, 181, 724, 2500, 4344, 13032};
	static float	sweepL[] = {0.01, 0.02, 0.03, 0.05, 0.10};
	static unsigned int sweepR[] = {1, 2, 4, 8, 16, 32};
	unsigned int	M = 1448;
	unsigned int	R;
	int		i;
	int		j;
	int		k;
	int		n0;
	int		value;
	int		previous;
	int		senderBudget;
	int		receiverBudget;
	double		measured;
	double		predicted;
	double		residual;

	(void) argc;
	failure_mode = CONTINUE_ON_FAIL;

	/*	1.  The threshold effect is gone.  The budget used to
	 *	step by a whole repair round as the block size crossed
	 *	a multiple of 1/L, so two spans with identical
	 *	configuration could differ by a factor of fifty in
	 *	achieved reliability purely because of block geometry.
	 *	The round limit is now stated rather than derived, so
	 *	the authorised rounds no longer depend on where the
	 *	block falls between thresholds.				*/

	fail_unless(shippingBudget(49 * ((int) M), M, 0.02) == 2);
	fail_unless(shippingBudget(51 * ((int) M), M, 0.02) == 3);

	previous = budget(47, M, 0.02, 8, 0);
	for (n0 = 47; n0 <= 53; n0++)
	{
		value = budget(n0, M, 0.02, 8, 0);
		if (value != previous)
		{
			printf("n0 %d: budget %d, was %d\n", n0, value,
					previous);
		}

		fail_unless(value == previous,
			"the budget still steps across the old threshold");
	}

	/*	2.  Monotonicity, in the round limit and in the block
	 *	size.  Neither may ever reduce the budget.		*/

	for (i = 0; i < 8; i++)
	{
		previous = 0;
		for (k = 0; k < 6; k++)
		{
			value = budget(sweepN0[i], M, 0.02, sweepR[k], 0);
			if (value < previous)
			{
				printf("n0 %d, rounds %u: %d, was %d\n",
					sweepN0[i], sweepR[k], value,
					previous);
			}

			fail_unless(value >= previous,
				"budget shrank as the round limit rose");
			previous = value;
		}
	}

	for (j = 0; j < 5; j++)
	{
		previous = 0;
		for (n0 = 10; n0 <= 5000; n0++)
		{
			value = budget(n0, M, sweepL[j], 8, 0);
			if (value < previous)
			{
				printf("n0 %d, loss rate %f: %d, was %d\n",
					n0, sweepL[j], value, previous);
			}

			fail_unless(value >= previous,
				"budget shrank as the block grew");
			previous = value;
		}
	}

	/*	3.  The budget at the default round limit.  Small
	 *	blocks need only the rounds themselves; large ones
	 *	need extra reports to enumerate their gaps, which is
	 *	arithmetic rather than a reliability judgement.		*/

	fail_unless(budget(6, M, 0.02, 8, 0) == 10);
	fail_unless(budget(724, M, 0.02, 8, 0) == 10);
	fail_unless(budget(724, M, 0.05, 8, 0) == 11);
	fail_unless(budget(13032, M, 0.02, 8, 0) == 23);
	fail_unless(budget(13032, M, 0.05, 8, 0) == 45);
	fail_unless(budget(72000, M, 0.05, 8, 0) == 208);

	/*	A block smaller than one segment still gets each
	 *	authorised round, one report apiece.			*/

	fail_unless(budget(0, M, 0.02, 8, 0) == 2 + 8,
		"a sub-segment block does not get its authorised rounds");

	/*	4.  The design premise.  Against a ceiling of 5%
	 *	segment loss, the default round limit must leave a
	 *	negligible residual across the whole realistic range
	 *	of block sizes.  This is what fails if the default is
	 *	lowered without revisiting the ceiling.			*/

	{
		static int	premiseN0[] = {6, 69, 724, 4344, 13032, 72000};

		for (i = 0; i < 6; i++)
		{
			residual = (double) premiseN0[i]
				* pow(0.05, (double) LTP_DEFAULT_REPAIR_ROUNDS);
			printf("residual at the 5%% ceiling, n0 %d: %.1e\n",
					premiseN0[i], residual);
			fail_unless(residual < 1.0e-5,
				"the default round limit leaves too large a \
residual at the design ceiling of 5% segment loss");
		}
	}

	/*	5.  The default must not be less generous than the
	 *	budgets that shipped, for any geometry in normal use;
	 *	an upgrade should never make cancellation more likely.	*/

	for (i = 0; i < 8; i++)
	{
		for (j = 0; j < 5; j++)
		{
			n0 = sweepN0[i];
			value = budget(n0, M, sweepL[j],
					LTP_DEFAULT_REPAIR_ROUNDS, 0);
			previous = shippingBudget(n0 * ((int) M), M,
					sweepL[j]);
			if (value < previous)
			{
				printf("n0 %d, loss rate %f: new %d, \
shipping %d\n", n0, sweepL[j], value, previous);
			}

			fail_unless(value >= previous,
				"the default budget is smaller than the one \
that shipped");
		}
	}

	/*	6.  Headroom dominance.  The receiver is allowed extra
	 *	rounds, so its budget must cover the sender's for every
	 *	geometry and every round limit.  Expressed in rounds
	 *	this holds unconditionally, where a target-based
	 *	headroom held only over a bounded range of mismatch.	*/

	for (i = 0; i < 8; i++)
	{
		for (j = 0; j < 5; j++)
		{
			for (k = 0; k < 6; k++)
			{
				R = sweepR[k];
				senderBudget = budget(sweepN0[i], M,
						sweepL[j], R, 0);
				receiverBudget = budget(sweepN0[i], M,
						sweepL[j],
						R + LTP_RECV_BUDGET_HEADROOM,
						1);
				if (receiverBudget < senderBudget)
				{
					printf("n0 %d, loss rate %f, rounds \
%u: receiver %d, sender %d\n", sweepN0[i], sweepL[j], R, receiverBudget,
						senderBudget);
				}

				fail_unless(receiverBudget >= senderBudget,
					"receiver headroom no longer covers \
the sender's budget");
			}
		}
	}

	/*	7.  The configured limit is honoured, and bounded.	*/

	fail_unless(budget(724, M, 0.02, 1, 0) == 3,
		"a single authorised round");
	fail_unless(LTP_DEFAULT_REPAIR_ROUNDS >= 1
			&& LTP_DEFAULT_REPAIR_ROUNDS <= LTP_MAX_REPAIR_ROUNDS,
		"the default round limit is outside the permitted range");

	/*	8.  Monte Carlo: does the authorised number of rounds
	 *	actually produce the reliability the arithmetic
	 *	predicts?  With R repair rounds a segment is sent R+1
	 *	times, so a session of n0 segments should be cancelled
	 *	with probability 1 - (1 - p^(R+1))^n0.
	 *
	 *	Small round limits are used deliberately: at the
	 *	default the predicted rate is far below what a
	 *	simulation of this size could resolve.			*/

	rngSeed(20260813);

	for (R = 1; R <= 3; R++)
	{
		predicted = 1.0 - pow(1.0 - pow(0.02, (double) (R + 1)), 49.0);
		measured = measureCancelRate(49, 0.02, (int) R, 400000);
		printf("rounds %u: predicted %.6f, measured %.6f\n", R,
				predicted, measured);
		fail_unless(measured < predicted * 1.25 + 0.00002
				&& measured > predicted * 0.75 - 0.00002,
			"the measured cancellation rate does not match what \
the authorised rounds predict");
	}

	/*	And the geometry where the unparameterised algorithm
	 *	was at its worst: just below a threshold, where it
	 *	authorised the fewest rounds.  It cancelled about one
	 *	session in fifty there; the default authorises eight
	 *	rounds regardless of where the block falls.		*/

	value = shippingBudget(49 * ((int) M), M, 0.02);
	measured = measureCancelRate(49, 0.02, value - 1, 400000);
	printf("shipping budget %d at n0 49: measured %.6f\n", value,
			measured);
	fail_unless(measured > 0.005,
		"the unparameterised algorithm no longer cancels sessions on \
the geometry where its threshold effect was worst; this check has stopped \
demonstrating anything");

	return check_summary(argv[0]);
}
