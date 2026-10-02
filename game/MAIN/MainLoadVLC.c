#include <common.h>

#ifndef MAIN_LOAD_VLC_BIGFILE
#define MAIN_LOAD_VLC_BIGFILE (sdata->ptrBigfileCdPos_2)
#endif

void MainLoadVLC_Callback(struct LoadQueueSlot *param_1)
{
	// VLC is now loaded
	sdata->bool_IsLoaded_VlcTable = 1;
	// NOTE(aalhendi): Retail commits the flag before the return delay slot.
	CTR_PSX_MEMORY_BARRIER();

#ifdef CTR_NATIVE
	// NOTE(aalhendi): Native keeps the loaded VLC pointer in host-visible state.
	sdata->ptrVlcTable = param_1->ptrDestination;
#endif
}

void MainLoadVLC(void)
{
	struct BigHeader *bigfile;
	struct BigEntry *entry;
	s32 size;
	void *destination;

	bigfile = MAIN_LOAD_VLC_BIGFILE;
	entry = BIG_GETENTRY(bigfile);
	size = entry[BI_VLCTABLE].size;
	// VLC is not loaded
	sdata->bool_IsLoaded_VlcTable = 0;
	// NOTE(aalhendi): Retail commits the reset before the signed-rounding branch.
	CTR_PSX_MEMORY_BARRIER();
	size = ((size + 0x7ff) / 0x800) * 0x800;
	// NOTE(aalhendi): Keep the allocation result live through publication and queue setup.
	destination = MEMPACK_AllocMem(size, GAME_RDATA_NAME(s_VlcTable));
	sdata->ptrVlcTable = destination;

	// This table is passed as parameter to DecDCTvlc2
	LOAD_AppendQueue(MAIN_LOAD_VLC_BIGFILE, LT_SETADDR, BI_VLCTABLE, destination, MainLoadVLC_Callback);
}
