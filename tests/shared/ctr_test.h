#ifndef CTR_TEST_H
#define CTR_TEST_H

// Shared harness for the suites under tests/.

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static int ctrTestChecks;
static int ctrTestFailures;

static void CtrTest_Fail(const char *what)
{
	ctrTestFailures++;
	printf("  FAIL: %s\n", what);
}

// Counts a check whether or not it held, so the summary reports the real size
// of the suite rather than only its successes.
#define CTR_TEST_CHECK(condition, what) \
	do \
	{ \
		ctrTestChecks++; \
		if (!(condition)) \
		{ \
			CtrTest_Fail(what); \
		} \
	} while (0)

// A value comparison reports what it got as well as what it wanted. A failing
// CTR_TEST_CHECK prints only its label, which leaves the reader guessing which
// of two similarly-worded assertions fired and whether the answer was high or
// low.
//
// inline because a suite that never compares values must still compile clean:
// a plain static function here would draw -Wunused-function from every suite
// that does not use it.
static inline void CtrTest_FailEq(int actual, int expected, const char *what)
{
	ctrTestFailures++;
	printf("  FAIL: %s (expected %d, got %d)\n", what, expected, actual);
}

// Each side is evaluated once, so an expression with a side effect is safe here.
#define CTR_TEST_CHECK_EQ(actual, expected, what) \
	do \
	{ \
		ctrTestChecks++; \
		if ((actual) != (expected)) \
		{ \
			CtrTest_FailEq((int)(actual), (int)(expected), (what)); \
		} \
	} while (0)

// The floating-point counterpart, for geometry. Reports what it got the same
// way CHECK_EQ does, so a failing suite names the quantity that moved.
// inline for the same reason CtrTest_FailEq is: a suite that never compares
// floating-point values must still compile clean.
static inline void CtrTest_FailEqNear(double actual, double expected, const char *what)
{
	ctrTestFailures++;
	printf("  FAIL: %s (expected %f, got %f)\n", what, expected, actual);
}

#define CTR_TEST_CHECK_NEAR(actual, expected, what) \
	do \
	{ \
		ctrTestChecks++; \
		const double ctrTestActual = (actual); \
		const double ctrTestExpected = (expected); \
		if (!(fabs(ctrTestActual - ctrTestExpected) < 0.000001)) \
		{ \
			CtrTest_FailEqNear(ctrTestActual, ctrTestExpected, (what)); \
		} \
	} while (0)

#define CTR_TEST_SECTION(title) printf("%s\n", (title))

// Prints the run's totals and returns the process exit code.
static int CtrTest_Summary(void)
{
	printf("\n%d checks, %d failures\n", ctrTestChecks, ctrTestFailures);
	return (ctrTestFailures == 0) ? 0 : 1;
}

// Ends the run when the harness cannot set itself up. A suite uses this only
// where continuing would report defects the code under test does not have.
static inline void CtrTest_Bail(const char *why)
{
	fflush(stdout);
	fprintf(stderr, "test harness: %s\n", why);
	exit(1);
}

#endif