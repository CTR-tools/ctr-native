#define ANIMATE_WATER_LISTS 2
#include "AnimateWater_Entry.h"
#include <common.h>
#include <ctr_gte_transfer.h>
#include "Animate_ScratchFrame.h"

void CTR_PSX_UNSAFE_MANUAL_AT44 AnimateWater2P(s32 timer, s32 numWaterVertices, struct WaterVert *waterVert, const struct TextureLayout *waterEnvMap,
                                               s32 *visOVertList0, s32 *visOVertList1)
{
#include "AnimateWater_Body.inc"
}

#undef ANIMATE_WATER_LISTS
#undef ANIMATE_SCRATCH_DECLARE
#undef ANIMATE_SCRATCH_SAVE
#undef ANIMATE_SCRATCH_RESTORE
