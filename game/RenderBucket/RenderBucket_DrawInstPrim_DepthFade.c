#include <ctr_types.h>

#ifndef CTR_NATIVE
// NOTE(aalhendi): These are inherited renderer lanes, not ordinary arguments.
// S0/T6/T7 and the supplied scratch pointer are read-only inputs; T3 is work,
// not an incoming command, and only V0 publishes the advanced packet cursor.
register struct RenderBucketExecuteScratch *const depthScratch CTR_PSX_REGISTER("$1");
register u8 *depthCursor CTR_PSX_REGISTER("$2");
register u32 depthOtAddress CTR_PSX_REGISTER("$11");
register const u32 depthColor0 CTR_PSX_REGISTER("$14");
register const u32 depthColor1 CTR_PSX_REGISTER("$15");
register const u32 depthColor2 CTR_PSX_REGISTER("$16");
#endif

#include <common.h>
#include <gpu.h>

#include "RenderBucket_ExecuteScratch.h"

#ifdef CTR_NATIVE
#define DEPTH_FADE_STORE_DATA_WORD(base, byteOffset, reg) CTR_WriteU32AlignedLE((u8 *)(base) + (byteOffset), MFC2(reg))
#else
#define DEPTH_FADE_STORE_DATA_WORD(base, byteOffset, reg) __asm__ volatile("swc2 $" CTR_GTE_STRINGIFY(reg) ",%1(%0)" : : "r"(base), "I"(byteOffset) : "memory")
#endif

// Near rows become black; far rows keep their saved color; the transition uses DPCS.
// NOTE(aalhendi): Only row zero primes RGB2, including its far-depth path.
// Use these labeled rows as standalone blocks. Architectural
// zero keeps their skips relative without a compiler branch policy; literal
// row offsets preserve SWC2 stores and the genuine renderer work lanes.
#define RENDER_BUCKET_FADE_ROW(scratch, row, depth, color, limit, firstRow)                                                                      \
	{                                                                                                                                            \
		if ((depth) <= 0)                                                                                                                        \
		{                                                                                                                                        \
			(scratch)->primitiveState.split.depthFadeColors[row] = 0;                                                                            \
			if (branchZero >= 0)                                                                                                                 \
				goto depth_fade_row_end_##row;                                                                                                   \
		}                                                                                                                                        \
		{                                                                                                                                        \
			if (firstRow)                                                                                                                        \
			{                                                                                                                                    \
				MTC2(color, 22);                                                                                                                 \
			}                                                                                                                                    \
			else                                                                                                                                 \
			{                                                                                                                                    \
				CTR_PSX_OBSERVE_VALUE(depth);                                                                                                    \
			}                                                                                                                                    \
			(depth) = (s32)(limit) - (depth);                                                                                                    \
			if ((depth) >= 0)                                                                                                                    \
			{                                                                                                                                    \
				(depth) = (s32)((u32)(depth) << 1);                                                                                              \
				MTC2(color, 6);                                                                                                                  \
				MTC2(depth, 8);                                                                                                                  \
				CTR_PSX_GTE_PIPELINE_DELAY();                                                                                                    \
				gte_dpcs();                                                                                                                      \
				DEPTH_FADE_STORE_DATA_WORD(scratch, offsetof(struct RenderBucketExecuteScratch, primitiveState.split.depthFadeColors[row]), 22); \
			}                                                                                                                                    \
		}                                                                                                                                        \
		depth_fade_row_end_##row :;                                                                                                              \
	}

#ifdef CTR_NATIVE
static s32 RenderBucket_DrawInstPrim_DepthFadeAtOTEntry(struct RenderBucketDrawContext *ctx, const struct TextureLayout *texture, u32 *otEntry)
#else
u8 *RenderBucket_DrawInstPrim_DepthFade(void)
#endif
{
#ifdef CTR_NATIVE
	struct RenderBucketExecuteScratch *depthScratch = RenderBucket_Scratch();
	struct PrimMem *primMem = ctx->primMem;
	u8 *depthCursor = (u8 *)primMem->cursor;
	u32 depthColor0 = (u32)ctx->tempColor[1];
	u32 depthColor1 = (u32)ctx->tempColor[2];
	u32 depthColor2 = (u32)ctx->tempColor[3];
#else
	register u32 otBase CTR_PSX_REGISTER("$5");
	register const struct TextureLayout *texture CTR_PSX_REGISTER("$6");
#endif
	register u32 word CTR_PSX_REGISTER("$3");
	register u32 work0 CTR_PSX_REGISTER("$8");
	register s32 work1 CTR_PSX_REGISTER("$9");
	register s32 work2 CTR_PSX_REGISTER("$10");
	register s32 branchZero CTR_PSX_REGISTER("$0");

	CTR_PSX_CAPTURE_REGISTER(branchZero, 0);
#ifndef CTR_NATIVE
	depthOtAddress = depthScratch->depthMac0;
	otBase = depthScratch->activeOTPtr32;
	depthOtAddress >>= 17;
	depthOtAddress <<= 2;
	depthOtAddress += otBase;
	texture = (const struct TextureLayout *)depthScratch->texturePtr32;
#endif
	// Save all rolling colors before overwriting the individual faded rows.
	depthScratch->primitiveState.split.depthFadeColors[0] = depthColor0;
	depthScratch->primitiveState.split.depthFadeColors[1] = depthColor1;
	depthScratch->primitiveState.split.depthFadeColors[2] = depthColor2;
	work0 = 0x800;
	work1 = (s32)MFC2(17);
	work2 = (s32)MFC2(18);
	work1 -= 0xc00;
	RENDER_BUCKET_FADE_ROW(depthScratch, 0, work1, depthColor0, work0, 1);
	// NOTE(aalhendi): Sample SZ3 after row zero to preserve GTE chronology.
	work1 = (s32)MFC2(19);
	work2 -= 0xc00;
	RENDER_BUCKET_FADE_ROW(depthScratch, 1, work2, depthColor1, work0, 0);
	work1 -= 0xc00;
	RENDER_BUCKET_FADE_ROW(depthScratch, 2, work1, depthColor2, work0, 0);

	work0 = depthScratch->primitiveState.split.depthFadeColors[0];
	work1 = (s32)depthScratch->primitiveState.split.depthFadeColors[1];
	work2 = (s32)depthScratch->primitiveState.split.depthFadeColors[2];
	word = work0 | (u32)work1;
	word |= (u32)work2;
	if (word == 0)
	{
#ifdef CTR_NATIVE
		return 0;
#else
		return depthCursor;
#endif
	}

#ifdef CTR_NATIVE
	// NOTE(aalhendi): Reject native arena exhaustion after fade/black handling.
	if (depthCursor + sizeof(POLY_GT3) >= (u8 *)primMem->guardEnd)
	{
		return -1;
	}
#endif

	work0 <<= 8;
	work0 >>= 8;
	word = 0x36000000u;
	// NOTE(aalhendi): Keep the command in V1 for the packet-color merge.
	CTR_PSX_KEEP_VALUE(word);
	word |= work0;
	CTR_WriteU32AlignedLE(depthCursor + 4, word);
	CTR_WriteU32AlignedLE(depthCursor + 16, (u32)work1);
	CTR_WriteU32AlignedLE(depthCursor + 28, (u32)work2);
	word = CTR_ReadU32LE(&texture->u0);
	work0 = CTR_ReadU32LE(&texture->u1);
	work1 = (s32)CTR_ReadU32LE(&texture->u2);
	DEPTH_FADE_STORE_DATA_WORD(depthCursor, 8, 12);
	DEPTH_FADE_STORE_DATA_WORD(depthCursor, 20, 13);
	DEPTH_FADE_STORE_DATA_WORD(depthCursor, 32, 14);
	CTR_WriteU32AlignedLE(depthCursor + 12, word);
	CTR_WriteU32AlignedLE(depthCursor + 24, work0);
	CTR_WriteU32AlignedLE(depthCursor + 36, (u32)work1);
	work1 = 40;
	// NOTE(aalhendi): Retain the byte count in its work lane for the return-slot advance.
	CTR_PSX_KEEP_VALUE(work1);
	work0 = 0x09000000u;
#ifdef CTR_NATIVE
	// NOTE(aalhendi): Native strips stale length bits; retail ORs the whole tag.
	word = CTR_ReadU32AlignedLE(otEntry) & 0x00ffffffu;
#else
	word = CTR_ReadU32AlignedLE((const void *)depthOtAddress);
	work2 = (s32)((u32)depthCursor << 8);
#endif
	word |= work0;
	CTR_WriteU32AlignedLE(depthCursor, word);
#ifdef CTR_NATIVE
	work2 = (s32)CtrGpu_PrimToOTLink24(depthCursor);
	CTR_WriteU32AlignedLE(otEntry, (u32)work2);
#else
	work2 = (s32)((u32)work2 >> 8);
	CTR_WriteU32AlignedLE((void *)depthOtAddress, (u32)work2);
#endif
	depthCursor += (u32)work1;
#ifdef CTR_NATIVE
	primMem->cursor = depthCursor;
	return 0;
#else
	return depthCursor;
#endif
}

#ifdef CTR_NATIVE
static void RenderBucket_DepthFadeColors(u32 color0, u32 color1, u32 color2, s32 sz1, s32 sz2, s32 sz3)
{
	struct RenderBucketExecuteScratch *scratch = RenderBucket_Scratch();
	s32 depth;
	u32 limit = 0x800;
	register s32 branchZero CTR_PSX_REGISTER("$0");

	// Generated triangles supply explicit SZ values, not the live GTE depth FIFO.
	CTR_PSX_CAPTURE_REGISTER(branchZero, 0);
	scratch->primitiveState.split.depthFadeColors[0] = color0;
	scratch->primitiveState.split.depthFadeColors[1] = color1;
	scratch->primitiveState.split.depthFadeColors[2] = color2;
	depth = sz1 - 0xc00;
	RENDER_BUCKET_FADE_ROW(scratch, 0, depth, color0, limit, 1);
	depth = sz2 - 0xc00;
	RENDER_BUCKET_FADE_ROW(scratch, 1, depth, color1, limit, 0);
	depth = sz3 - 0xc00;
	RENDER_BUCKET_FADE_ROW(scratch, 2, depth, color2, limit, 0);
}
#endif

#undef RENDER_BUCKET_FADE_ROW
#undef DEPTH_FADE_STORE_DATA_WORD
