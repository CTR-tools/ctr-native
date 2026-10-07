#include <common.h>

struct LngFile
{
	s32 numStrings;
	s32 offsetToPtrArr;
};

// Language index: 0 ja, 1 en, 2 en2, 3 fr, 4 de, 5 it, 6 es, 7 ne.
void LOAD_LangFile(s32 bigfilePtr, s32 lang)
{
	register struct LngFile *lngFile CTR_PSX_REGISTER("$6");
	u32 size;
	s16 i;
	s32 numStrings;
	char **strArray;

	if (GAME_LANGUAGE_FILE == NULL)
	{
		GAME_LANGUAGE_FILE = MEMPACK_AllocMem(GAME_LANGUAGE_BUFFER_SIZE, rdata.s_lang_buffer);
	}

	{
		register struct BigHeader *header CTR_PSX_REGISTER("$4") = (struct BigHeader *)bigfilePtr;
		register s32 loadType CTR_PSX_REGISTER("$5") = LT_SETADDR;
		register u32 *sizeOut CTR_PSX_REGISTER("$2") = &size;

		// NOTE(aalhendi): Finish call-input lifetimes before a2 becomes the returned file pointer.
		CTR_PSX_ORDER_VALUES(header, loadType);
		CTR_PSX_OBSERVE_VALUE(sizeOut);
		lngFile = LOAD_ReadFile_ex(header, loadType, BI_LANGUAGEFILE + lang, GAME_LANGUAGE_FILE, sizeOut, NULL);
	}
	if (lngFile == NULL)
	{
		return;
	}

	{
		register char **loadedStrings CTR_PSX_REGISTER("$2");
		register s32 loadedCount CTR_PSX_REGISTER("$3");

		// NOTE(aalhendi): Header temporaries end here so v0/v1 can hold the loop address and increment.
		CTR_PSX_KEEP_VALUE(lngFile);
		loadedStrings = (char **)((u32)lngFile + lngFile->offsetToPtrArr);
		loadedCount = lngFile->numStrings;
		GAME_LANGUAGE_STRING_COUNT = loadedCount;
		GAME_LANGUAGE_STRINGS = loadedStrings;
		i = 0;
		if (loadedCount <= 0)
		{
			return;
		}
		strArray = loadedStrings;
		numStrings = loadedCount;
	}

	// NOTE(aalhendi): Retail sign-extends the 16-bit index on both addressing and comparison.
	do
	{
		strArray[i] = (char *)((u32)strArray[i] + (u32)lngFile);
		i++;
	} while (i < numStrings);
}
