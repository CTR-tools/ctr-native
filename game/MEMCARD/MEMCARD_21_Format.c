#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003e51c-0x8003e59c.
u8 MEMCARD_Format(int slotIdx)
{
	if (sdata->memcard_stage != MC_STAGE_IDLE)
		return MC_RETURN_TIMEOUT;

	if (!format(MEMCARD_StringInit(slotIdx, 0)))
		return MC_RETURN_TIMEOUT;

	// discard any previous events
	// submit a load to make sure format worked,
	// check the result of a NEW CARD
	// 8 tries to see if it worked
	sdata->memcardSlot = slotIdx;
	sdata->memcard_stage = MC_STAGE_NEWCARD;
	MEMCARD_SkipEvents();
	while (_card_load(sdata->memcardSlot) != 1)
		;
	sdata->memcard_remainingAttempts = 8;

	// The "format" has started, the result will be found
	// the next time we wait for an event result
	return MC_RETURN_PENDING;
}
