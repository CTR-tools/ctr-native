/* Intentionally re-included for each literal entry; the entry owns LISTS. */
#include <ctr_types.h>

#ifndef CTR_NATIVE
// NOTE(aalhendi): Genuine argument/loop lanes; the kernel preserves all but T5.
// Keep these declarations before common.h and its inline function definitions.
register u32 waterTimer CTR_PSX_REGISTER("$4"), waterRemaining CTR_PSX_REGISTER("$5");
register u32 waterVertexAddress CTR_PSX_REGISTER("$6"), waterColor CTR_PSX_REGISTER("$7");
register u32 waterFirstOffset CTR_PSX_REGISTER("$11"), waterSecondOffset CTR_PSX_REGISTER("$12");
register u32 waterBits CTR_PSX_REGISTER("$13"), waterBitsLeft CTR_PSX_REGISTER("$14");
register const u32 *waterList0 CTR_PSX_REGISTER("$24");
#if ANIMATE_WATER_LISTS >= 2
register const u32 *waterList1 CTR_PSX_REGISTER("$25");
#endif
#endif
