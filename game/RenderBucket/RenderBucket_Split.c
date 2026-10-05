#include <common.h>

#ifdef CTR_NATIVE
static void RenderBucket_DrawFunc_Split(struct RenderBucketDrawContext *ctx)
{
	u32 *pCmd = (u32 *)ctx->idpp->ptrCommandList;

	// NOTE(aalhendi): Split strips use the shared water-clipping family.
	pCmd++;

	while (*pCmd != 0xffffffff)
	{
		u32 command = *pCmd++;
		u16 flags = (command >> 24) & 0xff;
		u16 stackIndex = (command >> 16) & 0xff;
		int startsNewStrip;
		int useRtps;
		int reuseFirstVertex;
		u32 drawCommand;
		int color;
		struct RenderBucketUncompressResult decoded;

		if ((command >> 16) == 0)
		{
			RenderBucket_ApplyColorOnlyCommand(ctx, command);
			continue;
		}

		decoded = RenderBucket_DispatchUncompressAnimationFrame(ctx, command, stackIndex);
		color = decoded.color;
		if ((flags & 4) == 0)
		{
			ctx->vertexIndex++;
		}

		ctx->tempCoords[0] = ctx->tempCoords[1];
		ctx->tempCoords[1] = ctx->tempCoords[2];
		ctx->tempCoords[2] = ctx->tempCoords[3];
		ctx->tempCoords[3] = ctx->stack[stackIndex];
		ctx->tempPacked[0] = ctx->tempPacked[1];
		ctx->tempPacked[1] = ctx->tempPacked[2];
		ctx->tempPacked[2] = ctx->tempPacked[3];
		ctx->tempPacked[3] = decoded.packed;
		ctx->tempSplit[0] = ctx->tempSplit[1];
		ctx->tempSplit[1] = ctx->tempSplit[2];
		ctx->tempSplit[2] = ctx->tempSplit[3];
		RenderBucket_InitWaterSplitVertex(ctx, 3, ctx->tempPacked[3].xy, ctx->tempPacked[3].z, color);

		ctx->tempColor[0] = ctx->tempColor[1];
		ctx->tempColor[1] = ctx->tempColor[2];
		ctx->tempColor[2] = ctx->tempColor[3];
		ctx->tempColor[3] = color;

		startsNewStrip = (flags & 0x80) != 0;
		if ((startsNewStrip != 0) || (ctx->stripLength == 0))
		{
			ctx->primCommand = command;
			RenderBucket_WritePackedWord((u8 *)RenderBucket_Scratch() + 0x10c, command);
		}

		if (startsNewStrip != 0)
		{
			ctx->stripLength = 0;
		}

		useRtps = ctx->stripLength > 2;
		reuseFirstVertex = (useRtps != 0) && ((flags & 0x40) != 0);

		if (reuseFirstVertex != 0)
		{
			ctx->tempCoords[1] = ctx->tempCoords[0];
			ctx->tempPacked[1] = ctx->tempPacked[0];
			ctx->tempSplit[1] = ctx->tempSplit[0];
			ctx->tempColor[1] = ctx->tempColor[0];
		}

		{
			u8 *raw = (u8 *)RenderBucket_Scratch();
			int offset = ctx->stripLength < 2 ? 0x90 + ctx->stripLength * 0x18 : 0xc0;
			u16 distance = (u16)((s16)RenderBucket_ReadPackedWord(raw + 0x44) - (s16)(decoded.packed.xy >> 16));
			if (useRtps != 0)
			{
				if (reuseFirstVertex == 0)
				{
					RenderBucket_WritePackedWord(raw + 0x90, RenderBucket_ReadPackedWord(raw + 0xa8));
					RenderBucket_WritePackedWord(raw + 0x94, RenderBucket_ReadPackedWord(raw + 0xac));
					memcpy(raw + 0x9e, raw + 0xb6, 2);
				}
				RenderBucket_WritePackedWord(raw + 0xa8, RenderBucket_ReadPackedWord(raw + 0xc0));
				RenderBucket_WritePackedWord(raw + 0xac, RenderBucket_ReadPackedWord(raw + 0xc4));
				memcpy(raw + 0xb6, raw + 0xce, 2);
			}
			RenderBucket_WritePackedWord(raw + offset, decoded.packed.xy);
			RenderBucket_WritePackedWord(raw + offset + 4, decoded.packed.z);
			memcpy(raw + offset + 14, &distance, sizeof(distance));
		}

		if (ctx->stripLength < 2)
		{
			ctx->stripLength++;
			continue;
		}

		drawCommand = (ctx->stripLength == 2) ? ctx->primCommand : command;

		{
			struct TextureLayout *tex;
			int depthMac0;
			int isValidTexture;
			int shouldDraw;

			shouldDraw = RenderBucket_ProjectNormalTriangle(ctx, drawCommand, useRtps, reuseFirstVertex, &depthMac0);
			if (shouldDraw != 0)
				RenderBucket_StoreSplitProjectedRegs(ctx);
			if (shouldDraw == 0)
			{
				ctx->stripLength++;
				continue;
			}

			tex = RenderBucket_GetCommandTexture(ctx, drawCommand, &isValidTexture);
			if (isValidTexture == 0)
			{
				ctx->stripLength++;
				continue;
			}

			RenderBucket_WritePackedWord((u8 *)RenderBucket_Scratch() + 0x50, (u32)tex);
			if (RenderBucket_DrawWaterSplitClipped(ctx, drawCommand, tex, depthMac0) < 0)
			{
				return;
			}
		}

		ctx->stripLength++;
	}
}

#else
// NOTE(aalhendi): Split consumes Execute's live s6 decoder/s5 writer ABI,
// packed streams and scratch records. sp is the texture table; strip completion
// transfers to the real Execute NextInstance CODE entry. The water family is
// an external application-ABI call, not owned by this unit.
// clang-format off
__asm__(".section .RenderBucket_DrawFunc_Split,\"ax\",@progbits\n"
        ".align 2\n"
        ".ent\tRenderBucket_DrawFunc_Split\n"
        ".globl RenderBucket_DrawFunc_Split\n"
        ".set\tnoreorder\n"
        ".set\tnoat\n"
        "RenderBucket_DrawFunc_Split:\n"
        ".Lrb_split_b030:\n"
        "beqz\t$t0,RenderBucket_Execute_NextInstance\n"
        "addiu\t$t9,$t9,4\n"
        "jalr\t$s6\n"
        "sw\t$t3,268($at)\n"
        "lw\t$t3,-4($t9)\n"
        "sw\t$t0,144($at)\n"
        "sw\t$v1,148($at)\n"
        "lh\t$v1,68($at)\n"
        "sra\t$t0,$t0,0x10\n"
        "subu\t$t0,$v1,$t0\n"
        "sh\t$t0,158($at)\n"
        "jalr\t$s6\n"
        "move\t$t6,$t1\n"
        "lw\t$t3,0($t9)\n"
        "sw\t$t0,168($at)\n"
        "sw\t$v1,172($at)\n"
        "lh\t$v1,68($at)\n"
        "sra\t$t0,$t0,0x10\n"
        "subu\t$t0,$v1,$t0\n"
        "sh\t$t0,182($at)\n"
        "jalr\t$s6\n"
        "move\t$t7,$t1\n"
        "lwc2\t$0,144($at)\n"
        "lwc2\t$1,148($at)\n"
        "lwc2\t$2,168($at)\n"
        "lwc2\t$3,172($at)\n"
        "mtc2\t$t0,$4\n"
        "mtc2\t$v1,$5\n"
        "move\t$s0,$t1\n"
        "lw\t$t3,268($at)\n"
        "c2\t0x280030\n"
        "sw\t$t0,192($at)\n"
        "sw\t$v1,196($at)\n"
        "lh\t$v1,68($at)\n"
        "sra\t$t0,$t0,0x10\n"
        "subu\t$t0,$v1,$t0\n"
        "sh\t$t0,206($at)\n"
        "addiu\t$t9,$t9,4\n"
        ".Lrb_split_b0c8:\n"
        "andi\t$a2,$t3,0x1ff\n"
        "beqz\t$a2,.Lrb_split_b0dc\n"
        "sll\t$a2,$a2,0x2\n"
        "addu\t$a2,$a2,$sp\n"
        "lw\t$a2,-4($a2)\n"
        ".Lrb_split_b0dc:\n"
        "lw\t$v1,28($at)\n"
        "mfc2\t$t2,$14\n"
        "cfc2\t$a1,$31\n"
        "sll\t$t0,$t3,0x3\n"
        "sll\t$a1,$a1,0xd\n"
        "bltz\t$a1,.Lrb_split_b15c\n"
        "sll\t$t1,$t3,0x2\n"
        "bgez\t$t0,.Lrb_split_b118\n"
        "lh\t$a1,36($at)\n"
        "c2\t0x1400006\n"
        "mfc2\t$t0,$24\n"
        "xor\t$t1,$a1,$t1\n"
        "beqz\t$t0,.Lrb_split_b15c\n"
        "xor\t$t0,$t0,$t1\n"
        "blez\t$t0,.Lrb_split_b15c\n"
        ".Lrb_split_b118:\n"
        "mfc2\t$t0,$12\n"
        "mfc2\t$t1,$13\n"
        "and\t$a1,$t0,$t2\n"
        "and\t$a1,$a1,$t1\n"
        "c2\t0x158002d\n"
        "subu\t$t0,$t0,$v1\n"
        "subu\t$t1,$t1,$v1\n"
        "subu\t$t2,$t2,$v1\n"
        "or\t$v1,$t0,$t1\n"
        "nor\t$v1,$v1,$t2\n"
        "or\t$v1,$v1,$a1\n"
        "bltz\t$v1,.Lrb_split_b15c\n"
        "sll\t$v1,$v1,0x10\n"
        "bltz\t$v1,.Lrb_split_b15c\n"
        "swc2\t$24,44($at)\n"
        "jal\tRenderBucket_WaterSplit_Import\n"
        "sw\t$a2,80($at)\n"
        ".Lrb_split_b15c:\n"
        "lw\t$t3,0($t9)\n"
        "addiu\t$t9,$t9,4\n"
        "bltz\t$t3,.Lrb_split_b030\n"
        "addiu\t$t0,$t3,1\n"
        "srl\t$t0,$t3,0x10\n"
        "bnez\t$t0,.Lrb_split_b1c4\n"
        "sll\t$t1,$t3,0x1\n"
        "sll\t$t0,$t3,0x1f\n"
        "srl\t$v1,$t3,0x7\n"
        "bltz\t$t0,.Lrb_split_b194\n"
        "andi\t$v1,$v1,0x1fc\n"
        "addu\t$v1,$v1,$gp\n"
        "bgez\t$zero,.Lrb_split_b19c\n"
        "lw\t$t7,0($v1)\n"
        ".Lrb_split_b194:\n"
        "addu\t$v1,$v1,$at\n"
        "lw\t$t7,320($v1)\n"
        ".Lrb_split_b19c:\n"
        "sll\t$t0,$t3,0x1e\n"
        "bltz\t$t0,.Lrb_split_b1b4\n"
        "andi\t$v1,$t3,0x1fc\n"
        "addu\t$v1,$v1,$gp\n"
        "bgez\t$zero,.Lrb_split_b1bc\n"
        "lw\t$s0,0($v1)\n"
        ".Lrb_split_b1b4:\n"
        "addu\t$v1,$v1,$at\n"
        "lw\t$s0,320($v1)\n"
        ".Lrb_split_b1bc:\n"
        "bgez\t$zero,.Lrb_split_b15c\n"
        "move\t$t6,$t7\n"
        ".Lrb_split_b1c4:\n"
        "bgez\t$t1,.Lrb_split_b1e4\n"
        "nop\n"
        "mfc2\t$t1,$12\n"
        "mfc2\t$t2,$17\n"
        "mtc2\t$t1,$13\n"
        "mtc2\t$t2,$18\n"
        "bgez\t$zero,.Lrb_split_b1fc\n"
        "move\t$t7,$t6\n"
        ".Lrb_split_b1e4:\n"
        "lw\t$t1,168($at)\n"
        "lw\t$t2,172($at)\n"
        "sw\t$t1,144($at)\n"
        "lh\t$t1,182($at)\n"
        "sw\t$t2,148($at)\n"
        "sh\t$t1,158($at)\n"
        ".Lrb_split_b1fc:\n"
        "lw\t$t1,192($at)\n"
        "lw\t$t2,196($at)\n"
        "sw\t$t1,168($at)\n"
        "lh\t$t1,206($at)\n"
        "sw\t$t2,172($at)\n"
        "sh\t$t1,182($at)\n"
        "jalr\t$s6\n"
        "move\t$t6,$t7\n"
        "mtc2\t$t0,$0\n"
        "mtc2\t$v1,$1\n"
        "move\t$t7,$s0\n"
        "move\t$s0,$t1\n"
        "c2\t0x180001\n"
        "sw\t$t0,192($at)\n"
        "sw\t$v1,196($at)\n"
        "lh\t$v1,68($at)\n"
        "sra\t$t0,$t0,0x10\n"
        "subu\t$t0,$v1,$t0\n"
        "bgez\t$zero,.Lrb_split_b0c8\n"
        "sh\t$t0,206($at)\n"
        ".set\tat\n"
        ".set\treorder\n"
        ".end\tRenderBucket_DrawFunc_Split\n");
// clang-format on
#endif
