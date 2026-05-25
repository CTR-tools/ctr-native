#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800123e0-0x80012440
void BOTS_SetGlobalNavData(u16 index)
{
	sdata->lastPathIndex = index;

	sdata->nav_NumPointsOnPath = sdata->NavPath_ptrHeader[index]->numPoints;

	sdata->nav_ptrFirstPoint = sdata->NavPath_ptrNavFrameArray[index];

	sdata->nav_ptrLastPoint = &sdata->nav_ptrFirstPoint[sdata->nav_NumPointsOnPath];

	return;
}
