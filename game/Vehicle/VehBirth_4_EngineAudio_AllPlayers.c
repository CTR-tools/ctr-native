#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80058ba4-0x80058c44.
void DECOMP_VehBirth_EngineAudio_AllPlayers(void)
{
	struct Thread *th;
	struct GameTracker *gGT;
	gGT = sdata->gGT;

	for (th = gGT->threadBuckets[PLAYER].thread; th != 0; th = th->siblingThread)
	{
		struct Driver *d = th->object;

		u8 driverID = d->driverID;

		int engine = data.MetaDataCharacters[data.characterIDs[driverID]].engineID;

		EngineAudio_InitOnce((engine * 4) + driverID, 0x8080);
	}
}
