#ifndef RENDER_BUCKET_WRITER_REGISTERS_H
#define RENDER_BUCKET_WRITER_REGISTERS_H

#include <ctr_types.h>

#ifndef CTR_NATIVE
// NOTE(aalhendi): These are inherited renderer lanes, not C arguments. Declare
// them before common.h; the ordinary RunWriter wrapper still owns its RA/S0 saves.
register struct RenderBucketExecuteScratch *const normalScratch CTR_PSX_REGISTER("$1");
register u8 *normalCursor CTR_PSX_REGISTER("$2");
register u32 normalOtAddress CTR_PSX_REGISTER("$11");
register const u32 normalColor0 CTR_PSX_REGISTER("$14");
register const u32 normalColor1 CTR_PSX_REGISTER("$15");
register const u32 normalColor2 CTR_PSX_REGISTER("$16");
#endif

#endif
