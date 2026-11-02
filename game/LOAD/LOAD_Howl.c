#include <common.h>

int LOAD_HowlHeaderSectors(CdlFILE *cdlFileHWL, void *ptrDestination, int firstSector, int numSector)
{
	CdlLOC loc;

	CDSYS_SetMode_StreamData();

	// Return error, if reading out-of-bounds after the end of KART HWL
	int sizeOver = ((firstSector + numSector) * LOAD_CD_DATA_SECTOR_SIZE - cdlFileHWL->size);
	if (sizeOver >= LOAD_CD_DATA_SECTOR_SIZE)
	{
		return 0;
	}

	CdIntToPos(CdPosToInt(&cdlFileHWL->pos) + firstSector, &loc);

	u8 buf[8];
	CdControl(CdlSetloc, (u8 *)&loc, buf);

	if (CdRead(numSector, ptrDestination, CdlModeSpeed) == 0)
	{
		return 0;
	}

	if (CdReadSync(0, 0) != 0)
	{
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
	}
	else
	{
		sdata->howlChainState = -1;
	}
}

int LOAD_HowlSectorChainStart(CdlFILE *cdlFileHWL, void *ptrDestination, int firstSector, int numSector)
{
	CdlLOC loc;

	if (numSector == 0)
	{
		return 1;
	}
	if (CustomSca_Serve(ptrDestination, firstSector, numSector))
	{
		sdata->howlChainState = 0;
		return 1;
	}

	// backup, so chain can use it later
	sdata->howlChainParams[0] = (int)cdlFileHWL;
	sdata->howlChainParams[1] = (int)ptrDestination;
	sdata->howlChainParams[2] = (int)firstSector;
	sdata->howlChainParams[3] = (int)numSector;

	CDSYS_SetMode_StreamData();

	// Return error, if reading out-of-bounds after the end of KART HWL
	int sizeOver = ((firstSector + numSector) * LOAD_CD_DATA_SECTOR_SIZE - cdlFileHWL->size);
	if (sizeOver >= LOAD_CD_DATA_SECTOR_SIZE)
	{
		return 0;
	}

	CdIntToPos(CdPosToInt(&cdlFileHWL->pos) + firstSector, &loc);

	u8 buf[8];
	CdControl(CdlSetloc, (u8 *)&loc, buf);

	sdata->howlChainState = 1;

	CdReadCallback(LOAD_HowlCallback);
	return (CdRead(numSector, ptrDestination, CdlModeSpeed) != 0);
}

int LOAD_HowlSectorChainEnd()
{
	int *howlChainParams;
	int howlChainState = sdata->howlChainState;
	howlChainParams = sdata->howlChainParams;

	if (howlChainState == -1)
	{
		LOAD_HowlSectorChainStart((CdlFILE *)howlChainParams[0], (void *)howlChainParams[1], howlChainParams[2], howlChainParams[3]);

		return 0;
	}

	return (howlChainState == 0);
}
