#ifndef CTR_TEST_SCRATCH_H
#define CTR_TEST_SCRATCH_H

// A disposable directory for a suite that writes files, built on the SDL pref
// path so nothing depends on the working directory the test was started in.
//
// A module whose paths run through NativeAssets_GetBaseDir writes into that
// directory. Left pointing at the working directory it would overwrite whatever
// the last run left there, so a suite that exercises real file I/O points its
// base directory at one of these instead. Suites that only read should not
// need this header.

#include <SDL3/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ctr_test.h"

#define CTR_TEST_SCRATCH_ORG "ctr-native"

// Removes the scratch directory and everything in it. Built from the pref path
// rather than from the caller's buffer, so it also works before the directory
// has been resolved into one.
static void CtrTestScratch_Remove(const char *suite, const char *fileName)
{
	char *path = SDL_GetPrefPath(CTR_TEST_SCRATCH_ORG, suite);

	if (path == NULL)
	{
		return;
	}

	// SDL_RemovePath only removes an empty directory, so the file goes first. The pref
	// path already ends in a separator.
	if (fileName != NULL)
	{
		char file[1024];

		snprintf(file, sizeof(file), "%s%s", path, fileName);
		SDL_RemovePath(file);
	}

	SDL_RemovePath(path);
	SDL_free(path);
}

// Resolves the scratch directory for `suite` into dst and creates it, clearing
// out a directory a run killed part way through would have left behind.
static void CtrTestScratch_Open(char *dst, size_t dstSize, const char *suite, const char *fileName)
{
	char *path = SDL_GetPrefPath(CTR_TEST_SCRATCH_ORG, suite);

	if (path == NULL)
	{
		CtrTest_Bail("could not resolve a scratch directory path");
	}

	if (strlen(path) >= dstSize)
	{
		SDL_free(path);
		CtrTest_Bail("the scratch directory path does not fit the caller's buffer");
	}

	// SDL_CreateDirectory fails on a directory that is already there.
	CtrTestScratch_Remove(suite, fileName);

	if (!SDL_CreateDirectory(path))
	{
		CtrTest_Bail("could not create the scratch directory");
	}

	snprintf(dst, dstSize, "%s", path);
	SDL_free(path);
}

#endif