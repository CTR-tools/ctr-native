#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003e6d4-0x8003e740.
// called by MC_ACTION_Erase
u8 MEMCARD_EraseFile(int slotIdx, char *srcString)
{
	if (sdata->memcard_stage != 0)
		return MC_RETURN_TIMEOUT;

	char name[64];
	MEMCARD_StringSet(name, slotIdx, srcString);

	sdata->memcard_stage = (erase(name)) ? MC_STAGE_ERASE_PASS : MC_STAGE_ERASE_FAIL;

	MEMCARD_GetFreeBytes(slotIdx);
	return MC_RETURN_PENDING;
}
