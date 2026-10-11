#ifndef CTR_TEST_H
#define CTR_TEST_H

// Shared harness for the suites under tests/.

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