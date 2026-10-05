#include "RenderBucket_WriterRegisters.h"

#include <common.h>
#include <gpu.h>

#include "RenderBucket_ExecuteScratch.h"

// NOTE(aalhendi): Normal keeps its local G3 skip and textured fallthrough.
// Architectural zero makes the relative G3 skip compiler-generated.
#define RB_PACKET_FINISH_UNTEXTURED()        \
	CTR_PSX_CAPTURE_REGISTER(branchZero, 0); \
	normalWork0 = 0x06000000u;               \
	if (branchZero >= 0)                     \
	goto link
#define RB_PACKET_FINISH_TEXTURED() normalWork0 = 0x09000000u

#ifndef CTR_NATIVE
u8 *RenderBucket_DrawInstPrim_Normal(void)
#else
static s32 RenderBucket_DrawInstPrim_NormalAtOTEntry(struct RenderBucketDrawContext *ctx, const struct TextureLayout *texture, u32 *otEntry)
#endif
{
#ifdef CTR_NATIVE
	struct RenderBucketExecuteScratch *normalScratch = RenderBucket_Scratch();
	struct PrimMem *primMem = ctx->primMem;
	u8 *normalCursor = (u8 *)primMem->cursor;
	u32 normalOtAddress;
	u32 normalColor0 = (u32)ctx->tempColor[1];
	u32 normalColor1 = (u32)ctx->tempColor[2];
	u32 normalColor2 = (u32)ctx->tempColor[3];
#else
	register const struct TextureLayout *texture CTR_PSX_REGISTER("$6");
	register u32 otBase CTR_PSX_REGISTER("$5");
#endif
	register u32 word;
	register u32 normalWork0 CTR_PSX_REGISTER("$8");
	register u32 normalWork1 CTR_PSX_REGISTER("$9");
	register s32 work2 CTR_PSX_REGISTER("$10");
	register s32 branchZero CTR_PSX_REGISTER("$0");

#ifdef CTR_NATIVE
	// NOTE(aalhendi): Keep the conservative GT3 capacity check even for G3,
	// before NULL-OT handling or any GTE side effect.
	if (normalCursor + sizeof(POLY_GT3) >= (u8 *)primMem->guardEnd)
		return -1;
	if (otEntry == 0)
		return 0;
#else
	word = normalScratch->primaryOTPtr32;
	normalWork0 = normalScratch->secondaryOTPtr32;
	normalWork1 = normalOtAddress << 6;
	if ((s32)normalWork1 <= 0)
		word = normalWork0;
	normalScratch->activeOTPtr32 = word;

	// NOTE(aalhendi): The real +28 entry bypasses range selection. No prefix
	// scalar survives here; the memory clobber forces the supplied activeOT reload.
	__asm__ volatile(".globl .Lrb_normal_inner\n.Lrb_normal_inner:" : : : "memory");
	normalOtAddress = normalScratch->depthMac0;
	otBase = normalScratch->activeOTPtr32;
	normalOtAddress >>= 17;
	normalOtAddress <<= 2;
	normalOtAddress += otBase;
#endif
#include "RenderBucket_DrawInstPrim_Packet.inc"
link:
#ifndef CTR_NATIVE
	// NOTE(aalhendi): Clamp enters with cursor/OT/tag/size in V0/T3/T0/T1.
	// Reacquire that complete tuple; a CODE label alone cannot describe the entry.
	__asm__ volatile(".globl .Lrb_normal_link\n.Lrb_normal_link:"
	                 : "+r"(normalCursor), "+r"(normalOtAddress), "+r"(normalWork0), "+r"(normalWork1)
	                 :
	                 : "memory");
#endif
#ifdef CTR_NATIVE
	// NOTE(aalhendi): Native strips stale length bits; retail trusts and ORs the whole tag.
	word = CTR_ReadU32AlignedLE(otEntry) & 0x00ffffffu;
#else
	word = CTR_ReadU32AlignedLE((const void *)normalOtAddress);
	work2 = (s32)((u32)normalCursor << 8);
#endif
	word |= normalWork0;
	CTR_WriteU32AlignedLE(normalCursor, word);
#ifdef CTR_NATIVE
	work2 = (s32)CtrGpu_PrimToOTLink24(normalCursor);
	CTR_WriteU32AlignedLE(otEntry, (u32)work2);
#else
	work2 = (s32)((u32)work2 >> 8);
	CTR_WriteU32AlignedLE((void *)normalOtAddress, (u32)work2);
#endif
	normalCursor += normalWork1;
	// NOTE(aalhendi): Normal and Clamp share this advance/JR/NOP tail.
	CTR_PSX_OBSERVE_VALUE(normalCursor);
#ifdef CTR_NATIVE
	primMem->cursor = normalCursor;
	return 0;
#else
	return normalCursor;
#endif
}

#undef RB_PACKET_FINISH_UNTEXTURED
#undef RB_PACKET_FINISH_TEXTURED
