#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80032498-0x8003254c
int LOAD_HowlHeaderSectors(CdlFILE *cdlFileHWL, void *ptrDestination, int firstSector, int numSector)
{
	CdlLOC loc;

	CDSYS_SetMode_StreamData();

	// Return error, if reading out-of-bounds after the end of KART HWL
	int sizeOver = ((firstSector + numSector) * 0x800 - cdlFileHWL->size);
	if (sizeOver >= 0x800)
	{
		return 0;
	}

	CdIntToPos(CdPosToInt(&cdlFileHWL->pos) + firstSector, &loc);

	char buf[8];
	CdControl(CdlSetloc, (u8 *)&loc, buf);

	if (CdRead(numSector, ptrDestination, 0x80) == 0)
	{
		return 0;
	}

	if (CdReadSync(0, 0) != 0)
	{
		return 0;
	}

	return 1;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003254c-0x80032594.
void LOAD_HowlCallback(u8 result, u8 *unk)
{
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

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80032594-0x8003266c
int LOAD_HowlSectorChainStart(CdlFILE *cdlFileHWL, void *ptrDestination, int firstSector, int numSector)
{
	CdlLOC loc;

	if (numSector == 0)
	{
		return 1;
	}

	// backup, so chain can use it later
	sdata->howlChainParams[0] = (int)cdlFileHWL;
	sdata->howlChainParams[1] = (int)ptrDestination;
	sdata->howlChainParams[2] = (int)firstSector;
	sdata->howlChainParams[3] = (int)numSector;

	CDSYS_SetMode_StreamData();

	// Return error, if reading out-of-bounds after the end of KART HWL
	int sizeOver = ((firstSector + numSector) * 0x800 - cdlFileHWL->size);
	if (sizeOver >= 0x800)
	{
		return 0;
	}

	CdIntToPos(CdPosToInt(&cdlFileHWL->pos) + firstSector, &loc);

	char buf[8];
	CdControl(CdlSetloc, (u8 *)&loc, buf);

	sdata->howlChainState = 1;

	CdReadCallback(LOAD_HowlCallback);
	return (CdRead(numSector, ptrDestination, 0x80) != 0);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003266c-0x800326b4.
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
