#include <common.h>

#ifdef CTR_NATIVE
static void RenderBucket_DrawFunc_Special(struct RenderBucketDrawContext *ctx)
{
	u32 *pCmd = (u32 *)ctx->idpp->ptrCommandList;

	// Native uses the explicit RenderBucketDrawContext mirrored FIFO ABI.
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

		// NOTE(aalhendi): The retail strip-start command survives decoder calls
		// at scratch+0x10c, including restarts after a continuation.
		if ((flags & 0x80) != 0 || ctx->stripLength == 0)
		{
			*CTR_SCRATCHPAD_PTR(u32, 0x10c) = command;
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

			struct RenderBucketProjectedRegs originalRegs;
			struct RenderBucketExecuteScratch *scratch = RenderBucket_Scratch();
			u8 *cullFlagsBytes = (u8 *)scratch + offsetof(struct RenderBucketExecuteScratch, instFlags);
			u32 pass;
			u32 lane;

			// NOTE(aalhendi): Original/mirrored FIFOs are also observable scratch
			// state. Keep the existing GTE helpers and retain the retail stores.
			if (useRtps != 0)
			{
				RenderBucket_LoadPrimRTPS(ctx, reuseFirstVertex);
			}
			else
			{
				RenderBucket_LoadPrimRTPT(ctx);
			}
			RenderBucket_StoreProjectedRegs(&originalRegs);
			for (lane = 0; lane < 3; lane++)
			{
				*CTR_SCRATCHPAD_PTR(u32, 0x90 + lane * 0x18) = MFC2(12 + lane);
				*CTR_SCRATCHPAD_PTR(u32, 0x94 + lane * 0x18) = MFC2(17 + lane);
			}
			if (useRtps != 0)
			{
				RenderBucket_LoadSpecialMirroredRTPS(ctx, reuseFirstVertex);
			}
			else
			{
				RenderBucket_LoadSpecialMirroredRTPT(ctx);
			}
			scratch->texturePtr32 = (u32)tex;
			CTR_WriteU16LE(cullFlagsBytes, CTR_ReadU16LE(cullFlagsBytes) ^ 0x8000);

			for (pass = 0; pass < 2; pass++)
			{
				u32 gteFlag = CFC2(31);
				int accepts = (s32)(gteFlag << 13) >= 0;
				int depthMac0;

				if (accepts != 0 && (s32)(drawCommand << 3) < 0)
				{
					int opZ;
					u32 cullXor = (u32)(s32)CTR_ReadS16LE(cullFlagsBytes) ^ (drawCommand << 2);
					gte_nclip();
					gte_stopz(&opZ);
					accepts = opZ != 0 && (s32)((u32)opZ ^ cullXor) > 0;
				}
				if (accepts != 0)
				{
					u32 screen = RenderBucket_PackXY(ctx->pb->rect.w, ctx->pb->rect.h);
					u32 xy0 = MFC2(12);
					u32 xy1 = MFC2(13);
					u32 xy2 = MFC2(14);
					u32 reject = (xy0 & xy1 & xy2) | ~((xy0 - screen) | (xy1 - screen) | (xy2 - screen));

					gte_avsz3();
					gte_stopz(&depthMac0);
					// NOTE(aalhendi): The second screen branch stores MAC0 in its delay slot,
					// even when its low-half rejection skips the writer.
					if ((s32)reject >= 0)
					{
						scratch->depthMac0 = (u32)depthMac0;
					}
					accepts = (s32)reject >= 0 && (s32)(reject << 16) >= 0;
					if (accepts != 0)
					{
						int result;
						if (pass == 0)
						{
							for (lane = 0; lane < 3; lane++)
							{
								*CTR_SCRATCHPAD_PTR(u32, 0x98 + lane * 0x18) = (u32)ctx->tempColor[lane + 1];
							}
							result = RenderBucket_DrawSpecialMirroredPass(ctx, drawCommand, tex, depthMac0);
						}
						else
						{
							result = RenderBucket_DispatchDrawInstPrimAtRange(ctx, drawCommand, tex, ctx->idpp->otRangeNormal, depthMac0);
						}
						if (result < 0)
						{
							return;
						}
					}
				}
				if (pass == 0)
				{
					RenderBucket_StoreProjectedRegs(&ctx->specialMirrorRegs);
					for (lane = 0; lane < 3; lane++)
					{
						*CTR_SCRATCHPAD_PTR(u32, 0xa0 + lane * 0x18) = MFC2(12 + lane);
						*CTR_SCRATCHPAD_PTR(u32, 0xa4 + lane * 0x18) = MFC2(17 + lane);
					}
					RenderBucket_LoadProjectedRegs(&originalRegs);
					CTR_WriteU16LE(cullFlagsBytes, CTR_ReadU16LE(cullFlagsBytes) ^ 0x8000);
					// NOTE(aalhendi): Reloading FIFO data retains the mirrored pass's control
					// flag. The next iteration reads CFC2(31) again, as retail.
				}
			}
		}

		ctx->stripLength++;
	}
}
#else
// NOTE(aalhendi): Execute supplies the application register ABI: sp is the
// texture table, s6 decodes packed vertices and s5 writes packets. Special
// keeps original/mirrored GTE FIFOs and colors across its two OT passes,
// then transfers empty streams to the real Execute NextInstance entry.
// clang-format off
__asm__(".section .RenderBucket_DrawFunc_Special,\"ax\",@progbits\n"
        ".align 2\n"
        ".ent\tRenderBucket_DrawFunc_Special\n"
        ".globl RenderBucket_DrawFunc_Special\n"
        ".set\tnoreorder\n"
        ".set\tnoat\n"
        "RenderBucket_DrawFunc_Special:\n"
        ".Lrb_special_bbc0:\n"
        "beqz\t$t0,RenderBucket_Execute_NextInstance\n"
        "addiu\t$t9,$t9,4\n"
        "jalr\t$s6\n"
        "sw\t$t3,268($at)\n"
        "lw\t$t3,-4($t9)\n"
        "mtc2\t$t0,$0\n"
        "mtc2\t$v1,$1\n"
        "jalr\t$s6\n"
        "move\t$t6,$t1\n"
        "lw\t$t3,0($t9)\n"
        "mtc2\t$t0,$2\n"
        "mtc2\t$v1,$3\n"
        "jalr\t$s6\n"
        "move\t$t7,$t1\n"
        "mtc2\t$t0,$4\n"
        "mtc2\t$v1,$5\n"
        "move\t$s0,$t1\n"
        "lw\t$t3,268($at)\n"
        // NOTE(aalhendi): Original and mirrored RTPT use separate scratch FIFOs.
        "c2\t0x280030\n"
        "addiu\t$t9,$t9,4\n"
        "swc2\t$12,144($at)\n"
        "swc2\t$17,148($at)\n"
        "swc2\t$13,168($at)\n"
        "swc2\t$18,172($at)\n"
        "swc2\t$14,192($at)\n"
        "swc2\t$19,196($at)\n"
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
        "c2\t0x280030\n"
        ".Lrb_special_bc84:\n"
        "lh\t$t0,36($at)\n"
        "andi\t$a2,$t3,0x1ff\n"
        "beqz\t$a2,.Lrb_special_bc9c\n"
        "sll\t$a2,$a2,0x2\n"
        "addu\t$a2,$a2,$sp\n"
        "lw\t$a2,-4($a2)\n"
        ".Lrb_special_bc9c:\n"
        "xori\t$t0,$t0,0x8000\n"
        "sh\t$t0,36($at)\n"
        "sw\t$a2,80($at)\n"
        "lw\t$v1,28($at)\n"
        "mfc2\t$t2,$14\n"
        "cfc2\t$a1,$31\n"
        "sll\t$t0,$t3,0x3\n"
        "sll\t$a1,$a1,0xd\n"
        "bltz\t$a1,.Lrb_special_bd64\n"
        "sll\t$t1,$t3,0x2\n"
        "bgez\t$t0,.Lrb_special_bce4\n"
        "lh\t$a1,36($at)\n"
        "c2\t0x1400006\n"
        "mfc2\t$t0,$24\n"
        "xor\t$t1,$a1,$t1\n"
        "beqz\t$t0,.Lrb_special_bd64\n"
        "xor\t$t0,$t0,$t1\n"
        "blez\t$t0,.Lrb_special_bd64\n"
        ".Lrb_special_bce4:\n"
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
        "bltz\t$v1,.Lrb_special_bd64\n"
        "sll\t$v1,$v1,0x10\n"
        "bltz\t$v1,.Lrb_special_bd64\n"
        "swc2\t$24,44($at)\n"
        "sw\t$t6,152($at)\n"
        "sw\t$t7,176($at)\n"
        "sw\t$s0,200($at)\n"
        // NOTE(aalhendi): The secondary OT pass shifts/masks colors, then restores them.
        "lb\t$t1,72($at)\n"
        "lw\t$v1,76($at)\n"
        "srlv\t$t6,$t6,$t1\n"
        "srlv\t$t7,$t7,$t1\n"
        "srlv\t$s0,$s0,$t1\n"
        "and\t$t6,$t6,$v1\n"
        "and\t$t7,$t7,$v1\n"
        "and\t$s0,$s0,$v1\n"
        "lw\t$v1,64($at)\n"
        "jalr\t$s5\n"
        "sw\t$v1,56($at)\n"
        "lw\t$t6,152($at)\n"
        "lw\t$t7,176($at)\n"
        "lw\t$s0,200($at)\n"
        ".Lrb_special_bd64:\n"
        "swc2\t$12,160($at)\n"
        "swc2\t$17,164($at)\n"
        "swc2\t$13,184($at)\n"
        "swc2\t$18,188($at)\n"
        "swc2\t$14,208($at)\n"
        "swc2\t$19,212($at)\n"
        "lwc2\t$12,144($at)\n"
        "lwc2\t$17,148($at)\n"
        "lwc2\t$13,168($at)\n"
        "lwc2\t$18,172($at)\n"
        "lwc2\t$14,192($at)\n"
        "lh\t$t0,36($at)\n"
        "lwc2\t$19,196($at)\n"
        "xori\t$t0,$t0,0x8000\n"
        "sh\t$t0,36($at)\n"
        "lw\t$v1,28($at)\n"
        "mfc2\t$t2,$14\n"
        "cfc2\t$a1,$31\n"
        "sll\t$t0,$t3,0x3\n"
        "sll\t$a1,$a1,0xd\n"
        "bltz\t$a1,.Lrb_special_be24\n"
        "sll\t$t1,$t3,0x2\n"
        "bgez\t$t0,.Lrb_special_bddc\n"
        "lh\t$a1,36($at)\n"
        "c2\t0x1400006\n"
        "mfc2\t$t0,$24\n"
        "xor\t$t1,$a1,$t1\n"
        "beqz\t$t0,.Lrb_special_be24\n"
        "xor\t$t0,$t0,$t1\n"
        "blez\t$t0,.Lrb_special_be24\n"
        ".Lrb_special_bddc:\n"
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
        "bltz\t$v1,.Lrb_special_be24\n"
        "sll\t$v1,$v1,0x10\n"
        "bltz\t$v1,.Lrb_special_be24\n"
        "swc2\t$24,44($at)\n"
        "lw\t$v1,60($at)\n"
        "jalr\t$s5\n"
        "sw\t$v1,56($at)\n"
        // NOTE(aalhendi): Strip restart, color-only commands and RTPS continuation.
        ".Lrb_special_be24:\n"
        "lw\t$t3,0($t9)\n"
        "addiu\t$t9,$t9,4\n"
        "bltz\t$t3,.Lrb_special_bbc0\n"
        "addiu\t$t0,$t3,1\n"
        "srl\t$t0,$t3,0x10\n"
        "bnez\t$t0,.Lrb_special_be8c\n"
        "sll\t$t1,$t3,0x1\n"
        "sll\t$t0,$t3,0x1f\n"
        "srl\t$v1,$t3,0x7\n"
        "bltz\t$t0,.Lrb_special_be5c\n"
        "andi\t$v1,$v1,0x1fc\n"
        "addu\t$v1,$v1,$gp\n"
        "bgez $zero,.Lrb_special_be64\n"
        "lw\t$t7,0($v1)\n"
        ".Lrb_special_be5c:\n"
        "addu\t$v1,$v1,$at\n"
        "lw\t$t7,320($v1)\n"
        ".Lrb_special_be64:\n"
        "sll\t$t0,$t3,0x1e\n"
        "bltz\t$t0,.Lrb_special_be7c\n"
        "andi\t$v1,$t3,0x1fc\n"
        "addu\t$v1,$v1,$gp\n"
        "bgez $zero,.Lrb_special_be84\n"
        "lw\t$s0,0($v1)\n"
        ".Lrb_special_be7c:\n"
        "addu\t$v1,$v1,$at\n"
        "lw\t$s0,320($v1)\n"
        ".Lrb_special_be84:\n"
        "bgez $zero,.Lrb_special_be24\n"
        "move\t$t6,$t7\n"
        ".Lrb_special_be8c:\n"
        "bgez\t$t1,.Lrb_special_beb8\n"
        "nop\n"
        "mfc2\t$t1,$12\n"
        "mfc2\t$t2,$17\n"
        "mtc2\t$t1,$13\n"
        "mtc2\t$t2,$18\n"
        "move\t$t7,$t6\n"
        "lw\t$t1,160($at)\n"
        "lw\t$t2,164($at)\n"
        "sw\t$t1,184($at)\n"
        "sw\t$t2,188($at)\n"
        ".Lrb_special_beb8:\n"
        "jalr\t$s6\n"
        "move\t$t6,$t7\n"
        "mtc2\t$t0,$0\n"
        "mtc2\t$v1,$1\n"
        "move\t$t7,$s0\n"
        "move\t$s0,$t1\n"
        "c2\t0x180001\n"
        "swc2\t$12,144($at)\n"
        "swc2\t$17,148($at)\n"
        "swc2\t$13,168($at)\n"
        "swc2\t$18,172($at)\n"
        "swc2\t$14,192($at)\n"
        "swc2\t$19,196($at)\n"
        "lw\t$v1,284($at)\n"
        "mfc2\t$t0,$0\n"
        "lui\t$a1,0xffff\n"
        "andi\t$a2,$t0,0xffff\n"
        "and\t$t0,$t0,$a1\n"
        "subu\t$t0,$v1,$t0\n"
        "or\t$t0,$t0,$a2\n"
        "mtc2\t$t0,$0\n"
        "lwc2\t$13,184($at)\n"
        "lwc2\t$18,188($at)\n"
        "lwc2\t$14,208($at)\n"
        "lwc2\t$19,212($at)\n"
        "nop\n"
        "nop\n"
        "c2\t0x180001\n"
        "bgez $zero,.Lrb_special_bc84\n"
        "nop\n"
        ".set\tat\n"
        ".set\treorder\n"
        ".end\tRenderBucket_DrawFunc_Special\n");
// clang-format on
#endif
