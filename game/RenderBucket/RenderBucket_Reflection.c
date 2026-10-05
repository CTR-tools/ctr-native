#include <common.h>

#ifdef CTR_NATIVE
static void RenderBucket_DrawFunc_Reflection(struct RenderBucketDrawContext *ctx)
{
	u32 *pCmd = (u32 *)ctx->idpp->ptrCommandList;

	// Native uses the explicit RenderBucketDrawContext split/FIFO ABI.
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

		if (ctx->stripLength < 2)
		{
			ctx->stripLength++;
			continue;
		}

		drawCommand = (ctx->stripLength == 2) ? ctx->primCommand : command;

		{
			struct TextureLayout *tex;
			int isValidTexture;

			tex = RenderBucket_GetCommandTexture(ctx, drawCommand, &isValidTexture);
			if (isValidTexture == 0)
			{
				ctx->stripLength++;
				continue;
			}

			if (RenderBucket_DrawReflectionPrimitive(ctx, drawCommand, useRtps, reuseFirstVertex, tex) < 0)
			{
				return;
			}
		}

		ctx->stripLength++;
	}
}

#else
// NOTE(aalhendi): Reflection consumes the live renderer register/scratch ABI.
// Native keeps the context body above; default retains both pass transitions.
// Entry keeps at=scratchpad, gp=palette, sp=texture window and s6=decoder.
// t9 is the command cursor; t3/t0 carry the command/restart state. The sentinel
// joins Execute's next-instance interior, so this is not a C-callable entry.
#include "RenderBucket_ReflectionGate.inc"
// clang-format off
__asm__(".section .RenderBucket_DrawFunc_Reflection,\"ax\",@progbits\n"
        ".align 2\n"
        ".ent\tRenderBucket_DrawFunc_Reflection\n"
        ".globl RenderBucket_DrawFunc_Reflection\n"
        ".set\tnoreorder\n"
        ".set\tnoat\n"
        "RenderBucket_DrawFunc_Reflection:\n"
        ".Lrb_reflection_strip_start:\n"
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
        "addu\t$t6,$t1,$zero\n"
        "lw\t$t3,0($t9)\n"
        "sw\t$t0,168($at)\n"
        "sw\t$v1,172($at)\n"
        "lh\t$v1,68($at)\n"
        "sra\t$t0,$t0,0x10\n"
        "subu\t$t0,$v1,$t0\n"
        "sh\t$t0,182($at)\n"
        "jalr\t$s6\n"
        "addu\t$t7,$t1,$zero\n"
        "lwc2\t$0,144($at)\n"
        "lwc2\t$1,148($at)\n"
        "lwc2\t$2,168($at)\n"
        "lwc2\t$3,172($at)\n"
        "mtc2\t$t0,$4\n"
        "mtc2\t$v1,$5\n"
        "addu\t$s0,$t1,$zero\n"
        "lw\t$t3,268($at)\n"
        "cop2\t0x280030\n"
        "sw\t$t0,192($at)\n"
        "sw\t$v1,196($at)\n"
        "lh\t$v1,68($at)\n"
        "sra\t$t0,$t0,0x10\n"
        "subu\t$t0,$v1,$t0\n"
        "sh\t$t0,206($at)\n"
        "addiu\t$t9,$t9,4\n"
        "swc2\t$12,108($at)\n"
        "swc2\t$17,112($at)\n"
        "swc2\t$13,116($at)\n"
        "swc2\t$18,120($at)\n"
        "swc2\t$14,124($at)\n"
        "swc2\t$19,128($at)\n"
        "lw\t$v1,284($at)\n"
        "mfc2\t$t0,$0\n"
        "mfc2\t$t1,$2\n"
        "mfc2\t$t2,$4\n"
        "lui\t$a1,0xffff\n"
        "andi\t$a2,$t0,0xffff\n"
        "and\t$t0,$t0,$a1\n"
        "subu\t$t0,$v1,$t0\n"
        "or\t$t0,$t0,$a2\n"
        "andi\t$a2,$t1,0xffff\n"
        "and\t$t1,$t1,$a1\n"
        "subu\t$t1,$v1,$t1\n"
        "or\t$t1,$t1,$a2\n"
        "andi\t$a2,$t2,0xffff\n"
        "and\t$t2,$t2,$a1\n"
        "subu\t$t2,$v1,$t2\n"
        "or\t$t2,$t2,$a2\n"
        "mtc2\t$t0,$0\n"
        "mtc2\t$t1,$2\n"
        "mtc2\t$t2,$4\n"
        "nop\n"
        "nop\n"
        "cop2\t0x280030\n"
        ".Lrb_reflection_prepare_pass:\n"
        "lh\t$t0,36($at)\n"
        "andi\t$a2,$t3,0x1ff\n"
        "beqz\t$a2,.Lrb_reflection_texture_ready\n"
        "sll\t$a2,$a2,0x2\n"
        "addu\t$a2,$a2,$sp\n"
        "lw\t$a2,-4($a2)\n"
        ".Lrb_reflection_texture_ready:\n"
        "xori\t$t0,$t0,0x8000\n"
        "sh\t$t0,36($at)\n"
        "sw\t$a2,80($at)\n"
        "lw\t$t0,64($at)\n"
        "lw\t$v1,60($at)\n"
        "sw\t$t0,60($at)\n"
        "sw\t$v1,64($at)\n"
        "sw\t$zero,264($at)\n"
        CTR_RB_REFLECTION_GATE(".Lrb_reflection_mirrored_")
        "mfc2\t$v1,$12\n"
        "mfc2\t$t0,$17\n"
        "mfc2\t$t1,$13\n"
        "mfc2\t$t2,$18\n"
        "lwc2\t$12,108($at)\n"
        "lwc2\t$17,112($at)\n"
        "lwc2\t$13,116($at)\n"
        "lwc2\t$18,120($at)\n"
        "sw\t$v1,108($at)\n"
        "sw\t$t0,112($at)\n"
        "sw\t$t1,116($at)\n"
        "sw\t$t2,120($at)\n"
        "mfc2\t$v1,$14\n"
        "mfc2\t$t0,$19\n"
        "lwc2\t$14,124($at)\n"
        "lwc2\t$19,128($at)\n"
        "sw\t$v1,124($at)\n"
        "lh\t$v1,36($at)\n"
        "sw\t$t0,128($at)\n"
        "xori\t$v1,$v1,0x8000\n"
        "sh\t$v1,36($at)\n"
        "lw\t$v1,60($at)\n"
        "lw\t$t0,64($at)\n"
        "sw\t$v1,64($at)\n"
        "sw\t$t0,60($at)\n"
        "addiu\t$t0,$zero,-1\n"
        "sw\t$t0,264($at)\n"
        CTR_RB_REFLECTION_GATE(".Lrb_reflection_original_")
        ".Lrb_reflection_next_command:\n"
        "lw\t$t3,0($t9)\n"
        "addiu\t$t9,$t9,4\n"
        "bltz\t$t3,.Lrb_reflection_strip_start\n"
        "addiu\t$t0,$t3,1\n"
        "srl\t$t0,$t3,0x10\n"
        "bnez\t$t0,.Lrb_reflection_continue_strip\n"
        "sll\t$t1,$t3,0x1\n"
        "sll\t$t0,$t3,0x1f\n"
        "srl\t$v1,$t3,0x7\n"
        "bltz\t$t0,.Lrb_reflection_first_scratch_color\n"
        "andi\t$v1,$v1,0x1fc\n"
        "addu\t$v1,$v1,$gp\n"
        "bgez\t$zero,.Lrb_reflection_first_color_ready\n"
        "lw\t$t7,0($v1)\n"
        ".Lrb_reflection_first_scratch_color:\n"
        "addu\t$v1,$v1,$at\n"
        "lw\t$t7,320($v1)\n"
        ".Lrb_reflection_first_color_ready:\n"
        "sll\t$t0,$t3,0x1e\n"
        "bltz\t$t0,.Lrb_reflection_second_scratch_color\n"
        "andi\t$v1,$t3,0x1fc\n"
        "addu\t$v1,$v1,$gp\n"
        "bgez\t$zero,.Lrb_reflection_second_color_ready\n"
        "lw\t$s0,0($v1)\n"
        ".Lrb_reflection_second_scratch_color:\n"
        "addu\t$v1,$v1,$at\n"
        "lw\t$s0,320($v1)\n"
        ".Lrb_reflection_second_color_ready:\n"
        "bgez\t$zero,.Lrb_reflection_next_command\n"
        "addu\t$t6,$t7,$zero\n"
        ".Lrb_reflection_continue_strip:\n"
        "bgez\t$t1,.Lrb_reflection_no_reuse\n"
        "nop\n"
        "mfc2\t$t1,$12\n"
        "mfc2\t$t2,$17\n"
        "mtc2\t$t1,$13\n"
        "mtc2\t$t2,$18\n"
        "lw\t$t1,108($at)\n"
        "lw\t$t2,112($at)\n"
        "sw\t$t1,116($at)\n"
        "sw\t$t2,120($at)\n"
        "bgez\t$zero,.Lrb_reflection_slide_third\n"
        "addu\t$t7,$t6,$zero\n"
        ".Lrb_reflection_no_reuse:\n"
        "lw\t$t1,168($at)\n"
        "lw\t$t2,172($at)\n"
        "sw\t$t1,144($at)\n"
        "lh\t$t1,182($at)\n"
        "sw\t$t2,148($at)\n"
        "sh\t$t1,158($at)\n"
        ".Lrb_reflection_slide_third:\n"
        "lw\t$t1,192($at)\n"
        "lw\t$t2,196($at)\n"
        "sw\t$t1,168($at)\n"
        "lh\t$t1,206($at)\n"
        "sw\t$t2,172($at)\n"
        "sh\t$t1,182($at)\n"
        "jalr\t$s6\n"
        "addu\t$t6,$t7,$zero\n"
        "mtc2\t$t0,$0\n"
        "mtc2\t$v1,$1\n"
        "addu\t$t7,$s0,$zero\n"
        "addu\t$s0,$t1,$zero\n"
        "cop2\t0x180001\n"
        "sw\t$t0,192($at)\n"
        "sw\t$v1,196($at)\n"
        "lh\t$v1,68($at)\n"
        "sra\t$t0,$t0,0x10\n"
        "subu\t$t0,$v1,$t0\n"
        "sh\t$t0,206($at)\n"
        "mfc2\t$v1,$12\n"
        "mfc2\t$t0,$17\n"
        "mfc2\t$t1,$13\n"
        "mfc2\t$t2,$18\n"
        "lwc2\t$12,108($at)\n"
        "lwc2\t$17,112($at)\n"
        "lwc2\t$13,116($at)\n"
        "lwc2\t$18,120($at)\n"
        "sw\t$v1,108($at)\n"
        "sw\t$t0,112($at)\n"
        "sw\t$t1,116($at)\n"
        "sw\t$t2,120($at)\n"
        "mfc2\t$v1,$14\n"
        "mfc2\t$t0,$19\n"
        "lwc2\t$14,124($at)\n"
        "lwc2\t$19,128($at)\n"
        "sw\t$v1,124($at)\n"
        "sw\t$t0,128($at)\n"
        "lw\t$v1,284($at)\n"
        "mfc2\t$t0,$0\n"
        "lui\t$a1,0xffff\n"
        "andi\t$a2,$t0,0xffff\n"
        "and\t$t0,$t0,$a1\n"
        "subu\t$t0,$v1,$t0\n"
        "or\t$t0,$t0,$a2\n"
        "mtc2\t$t0,$0\n"
        "nop\n"
        "nop\n"
        "cop2\t0x180001\n"
        "bgez\t$zero,.Lrb_reflection_prepare_pass\n"
        "nop\n"
        ".end\tRenderBucket_DrawFunc_Reflection\n"
        ".size RenderBucket_DrawFunc_Reflection,.-RenderBucket_DrawFunc_Reflection\n"
        ".set\tat\n"
        ".set\treorder\n"
        ".text\n");
// clang-format on
#undef CTR_RB_REFLECTION_GATE
#endif
