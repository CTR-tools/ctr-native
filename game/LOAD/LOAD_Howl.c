#include <common.h>

int LOAD_HowlHeaderSectors(CdlFILE *cdlFileHWL, void *ptrDestination, int firstSector, int numSector)
{
	CdlLOC loc;
	int sizeOver;
	u8 buf[8];

	CDSYS_SetMode_StreamData();

	// Return error, if reading out-of-bounds after the end of KART HWL
	sizeOver = ((firstSector + numSector) * LOAD_CD_DATA_SECTOR_SIZE - cdlFileHWL->size);
	if (sizeOver >= LOAD_CD_DATA_SECTOR_SIZE)
	{
		return 0;
	}

	CdIntToPos(CdPosToInt(&cdlFileHWL->pos) + firstSector, &loc);

	CdControl(CdlSetloc, (u8 *)&loc, buf);

	if (CdRead(numSector, ptrDestination, CdlModeSpeed) == 0)
	{
		return 0;
	}

	if (CdReadSync(0, 0) != 0)
	{
		// NOTE(aalhendi): Retail branches past the zero return instead of folding the sync result into a boolean.
		CTR_PSX_MEMORY_BARRIER();
		return 0;
	}

	return 1;
}

void LOAD_HowlCallback(u8 result, u8 *unk)
{
	(void)unk;
	// disable callback
	CdReadCallback(0);

	if (result == CdlComplete)
	{
		sdata->howlChainState = 0;
		// NOTE(aalhendi): Retail does not move this store into the following jump's delay slot.
		CTR_PSX_MEMORY_BARRIER();
	}
	else
	{
		sdata->howlChainState = -1;
	}
}

int LOAD_HowlSectorChainStart(CdlFILE *cdlFileHWL, void *ptrDestination, int firstSector, int numSector)
{
	CdlLOC loc;
	int sizeOver;
	u8 buf[8];

	if (numSector == 0)
	{
		return 1;
	}

	// backup, so chain can use it later
	sdata->howlChainParams[0] = (int)cdlFileHWL;
	sdata->howlChainParams[1] = (int)ptrDestination;
	sdata->howlChainParams[2] = (int)firstSector;
	sdata->howlChainParams[3] = (int)numSector;
	// NOTE(aalhendi): Retail does not move the final parameter store into the mode-setting call's delay slot.
	CTR_PSX_MEMORY_BARRIER();

	CDSYS_SetMode_StreamData();

	// Return error, if reading out-of-bounds after the end of KART HWL
	sizeOver = ((firstSector + numSector) * LOAD_CD_DATA_SECTOR_SIZE - cdlFileHWL->size);
	if (sizeOver >= LOAD_CD_DATA_SECTOR_SIZE)
	{
		return 0;
	}

	CdIntToPos(CdPosToInt(&cdlFileHWL->pos) + firstSector, &loc);

	CdControl(CdlSetloc, (u8 *)&loc, buf);

	sdata->howlChainState = 1;

	CdReadCallback(LOAD_HowlCallback);
	return (CdRead(numSector, ptrDestination, CdlModeSpeed) != 0);
}

#ifndef LOAD_HOWL_CHAIN_STATE_INITIAL
#define LOAD_HOWL_CHAIN_STATE_INITIAL (sdata->howlChainState)
#endif

int LOAD_HowlSectorChainEnd(void)
{
	int howlChainState = LOAD_HOWL_CHAIN_STATE_INITIAL;

	if (howlChainState == -1)
	{
		LOAD_HowlSectorChainStart((CdlFILE *)sdata->howlChainParams[0], (void *)sdata->howlChainParams[1], sdata->howlChainParams[2],
		                          sdata->howlChainParams[3]);
		return 0;
	}

	return (howlChainState == 0);
}
