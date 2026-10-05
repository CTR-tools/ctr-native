#include "RenderBucket_WriterRegisters.h"

#include <common.h>
#include <gpu.h>
#include "RenderBucket_ExecuteScratch.h"

#ifndef CTR_NATIVE
extern u8 normalLinkCode[] __asm__(".Lrb_normal_link");

// NOTE(aalhendi): This closed pair owns the tag write and external delay slot.
// Its ready V0/T3/T1 inputs have no C outputs or deferred writeback.
#define CLAMP_LINK_TAG(upper)                                                                                         \
	do                                                                                                                \
	{                                                                                                                 \
		__asm__ volatile(".set\tnoreorder\n\t"                                                                        \
		                 "bgez $0,%0\n\t"                                                                             \
		                 "lui $8,%1\n\t"                                                                              \
		                 ".set\treorder"                                                                              \
		                 :                                                                                            \
		                 : "i"(normalLinkCode), "K"(upper), "r"(normalCursor), "r"(normalOtAddress), "r"(normalWork1) \
		                 : "$8", "memory");                                                                           \
		__builtin_unreachable();                                                                                      \
	} while (0)

#define RB_PACKET_FINISH_UNTEXTURED() CLAMP_LINK_TAG(0x0600)
#define RB_PACKET_FINISH_TEXTURED()   CLAMP_LINK_TAG(0x0900)

u8 *RenderBucket_DrawInstPrim_ClampDepth(void)
{
	// NOTE(aalhendi): These internal register choices are measured, not entry ABI.
	register const struct TextureLayout *texture CTR_PSX_REGISTER("$6");
	u32 word;
	register s32 work2 CTR_PSX_REGISTER("$10");
	register u32 otBase CTR_PSX_REGISTER("$5");
	register u32 normalWork0 CTR_PSX_REGISTER("$8");
	register u32 normalWork1 CTR_PSX_REGISTER("$9");

	normalOtAddress = normalScratch->depthMac0;
	word = (u32)(s32)normalScratch->minDepthByteOffset;
	normalWork0 = (u32)(s32)normalScratch->maxDepthByteOffset;
	otBase = normalScratch->activeOTPtr32;
#include "RenderBucket_ClampOT.inc"

#include "RenderBucket_DrawInstPrim_Packet.inc"
}

#undef RB_PACKET_FINISH_UNTEXTURED
#undef RB_PACKET_FINISH_TEXTURED
#undef CLAMP_LINK_TAG
#endif
