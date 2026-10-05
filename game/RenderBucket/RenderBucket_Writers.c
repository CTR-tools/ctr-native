#include <common.h>

#ifndef CTR_NATIVE
extern u8 RenderBucket_DrawInstPrim_Normal[];
extern u8 RenderBucket_DrawInstPrim_DepthFade[];
extern u8 RenderBucket_DrawInstPrim_ClampDepth[];
extern u8 RenderBucket_NormalInner[] __asm__(".Lrb_normal_inner");

static u32 RenderBucket_RunWriter(u32 cursor, u32 command, u32 color0, u32 color1, u32 color2, const u8 *entry)
{
	register u32 result CTR_PSX_REGISTER("$2") = cursor;
	register u32 t3 CTR_PSX_REGISTER("$11") = command;
	register u32 t6 CTR_PSX_REGISTER("$14") = color0;
	register u32 t7 CTR_PSX_REGISTER("$15") = color1;
	register u32 s0 CTR_PSX_REGISTER("$16") = color2;

	// NOTE(aalhendi): Snapshot rolling colors into the genuine renderer ABI.
	// Only the packet cursor is returned; the kernel preserves the inputs and
	// persistent renderer registers. GCC owns this ordinary wrapper's RA/S0 saves.
	__asm__ volatile(".set\tnoreorder\n\t.set\tnoat\n\tlui $at,0x1f80\n\tjalr %5\n\tnop\n\t.set\tat\n\t.set\treorder"
	                 : "+r"(result), "+r"(t3)
	                 : "r"(t6), "r"(t7), "r"(s0), "r"(entry)
	                 : "$1", "$3", "$5", "$6", "$8", "$9", "$10", "$31", "memory");
	return result;
}

#endif
