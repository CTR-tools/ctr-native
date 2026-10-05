#include <common.h>

CTR_STATIC_ASSERT(sizeof(TILE_1) == 0x0C);
CTR_STATIC_ASSERT(offsetof(TILE_1, tag) == 0x00);
CTR_STATIC_ASSERT(offsetof(TILE_1, r0) == 0x04);
CTR_STATIC_ASSERT(offsetof(TILE_1, x0) == 0x08);

#ifdef CTR_NATIVE

static s32 RenderStars_MulLowShift(s32 value, s32 scale)
{
	return (s32)(u32)((s64)value * (s64)scale) >> 12;
}

static u32 RenderStars_NextRngSelfOr(u32 *state0, u32 *state1)
{
	u32 shifted1 = *state1 >> 8;
	u32 mixed = *state0 + shifted1;

	*state0 = (*state0 >> 8) | mixed;
	mixed += *state0 >> 8;
	mixed <<= 24;
	*state1 = shifted1 | mixed;
	return mixed;
}

static u32 RenderStars_NextRngWithOr(u32 *state0, u32 *state1, u32 seedOr)
{
	u32 shifted1 = *state1 >> 8;
	u32 mixed = *state0 + shifted1;

	*state0 = (*state0 >> 8) | seedOr;
	mixed += *state0 >> 8;
	mixed <<= 24;
	*state1 = shifted1 | mixed;
	return mixed;
}

static int RenderStars_IsVisible(u32 gteFlag, u32 sxy)
{
	s32 clip = (s32)(gteFlag << 13);
	s32 bounds;

	if (clip < 0)
	{
		return 0;
	}

	bounds = (s32) ~((sxy - 0x00d90200) | sxy);
	if (bounds < 0)
	{
		return 0;
	}

	return ((s32)((u32)bounds << 16) >= 0);
}

void RenderStars(struct PushBuffer *pb, struct PrimMem *primMem, struct Stars *stars, u8 numPlyr)
{
	u32 *prim = (u32 *)primMem->cursor;
	int playerCount = numPlyr;

	// NOTE(aalhendi): Native relies on the host ABI instead of retail's GPR
	// saves at scratchpad 0x00-0x2c; no data temporary lives in that span.
	CTC2(0, 5);
	CTC2(0, 6);
	CTC2(0, 7);

	for (int playerIndex = playerCount - 1; playerIndex >= 0; playerIndex--, pb++)
	{
		s32 starIndex;
		u32 state0;
		u32 state1;
		u32 seedX;
		u32 seedY;
		u32 seedZ;
		int seedShift;
		int spread;
		u32 *ot;

		CTC2(CTR_ReadU32LE((char *)&pb->matrix_ViewProj + 0x00), 0);
		CTC2(CTR_ReadU32LE((char *)&pb->matrix_ViewProj + 0x04), 1);
		CTC2(CTR_ReadU32LE((char *)&pb->matrix_ViewProj + 0x08), 2);
		CTC2(CTR_ReadU32LE((char *)&pb->matrix_ViewProj + 0x0c), 3);
		CTC2(CTR_ReadU32LE((char *)&pb->matrix_ViewProj + 0x10), 4);

		CTC2((u32)(s32)pb->rect.w << 15, 24);
		CTC2((u32)(s32)pb->rect.h << 15, 25);
		CTC2((u32)pb->distanceToScreen_PREV, 26);
		CTC2(0, 5);
		CTC2(0, 6);
		CTC2(0, 7);

		starIndex = (u16)stars->numStars;
		spread = (u16)stars->spread;
		seedShift = (u16)stars->seed + 20;
		ot = &pb->ptrOT[(u16)stars->distance];
		state0 = 0x30125400;
		state1 = 0x493583fe;

		while (starIndex >= 0)
		{
			s32 starX;
			s32 starY;
			s32 starZ;
			s32 absX;
			s32 absY;
			s32 absZ;
			s32 scale;
			u32 packedXY;
			u32 colorSeed;
			u32 colorByte;
			u32 color;
			u32 gteFlag;
			u32 sxy;

			seedX = RenderStars_NextRngSelfOr(&state0, &state1);
			seedY = RenderStars_NextRngWithOr(&state0, &state1, seedX);
			seedZ = RenderStars_NextRngWithOr(&state0, &state1, seedX);

			starX = (s32)seedX >> 20;
			starY = (s32)seedY >> (seedShift & 0x1f);
			starZ = (s32)seedZ >> 20;

			absX = starX;
			if (absX < 0)
			{
				absX = -absX;
			}

			absY = starY;
			if (absY < 0)
			{
				absY = -absY;
				if (spread == 0)
				{
					starY = absY;
				}
			}

			absZ = starZ;
			if (absZ < 0)
			{
				absZ = -absZ;
			}

			if (absX - absY <= 0)
			{
				s32 oldAbsX = absX;
				absX = absY;
				absY = oldAbsX;
			}

			if (absX - absZ <= 0)
			{
				s32 oldAbsX = absX;
				absX = absZ;
				absZ = oldAbsX;
			}

			scale = 0x01000000 / (absX + (absY >> 2) + (absZ >> 2));

			packedXY = (u32)(RenderStars_MulLowShift(starX, scale) & 0xffff);
			packedXY |= (u32)RenderStars_MulLowShift(starY, scale) << 16;

			MTC2(packedXY, 0);
			MTC2((u32)RenderStars_MulLowShift(starZ, scale), 1);

			colorSeed = RenderStars_NextRngWithOr(&state0, &state1, packedXY);
			gte_rtps_b();

			colorByte = ((colorSeed >> 24) | 0x40) & 0xff;
			color = 0x68000000 | colorByte | (colorByte << 8) | (colorByte << 16);

			gteFlag = CFC2(31);
			sxy = MFC2(14);

			if (RenderStars_IsVisible(gteFlag, sxy))
			{
				TILE_1 *star = (TILE_1 *)prim;

				CtrGpu_WriteColorCode(&star->r0, color);
				CtrGpu_WritePackedXY(&star->x0, sxy);
				CtrGpu_LinkPacketFullOTTag(ot, &star->tag, star, 0x02000000);
				prim = (u32 *)(star + 1);
			}

			starIndex--;
		}

		// Retail emits a length-2 draw-env packet: E1 tpage followed by a
		// zero terminator word.
		struct CtrGpuDrawModePacket *drawMode = (struct CtrGpuDrawModePacket *)prim;
		drawMode->drawMode = 0xe1000a20;
		drawMode->terminator = 0;
		CtrGpu_LinkPacketFullOTTag(ot, &drawMode->tag, drawMode, 0x02000000);
		prim = (u32 *)(drawMode + 1);
	}

	primMem->cursor = prim;
}

#else

// NOTE(aalhendi): Retail saves the caller's GPRs through at in scratchpad,
// then uses gp/sp as seed/color lanes without an ordinary stack frame.
// There are no external calls. Preserve the division padding, GTE hazards,
// signed loop delay slots and original one-star minimum for legal 1P..4P.
#define RENDER_STARS_RNG(lane, seedOr) \
	"sll $t6,$t8,24\n"                 \
	"srl $t8,$t8,8\n"                  \
	"addu " lane ",$s7,$t8\n"          \
	"srl $s7,$s7,8\n"                  \
	"or $s7,$s7," seedOr "\n"          \
	"srl $t6,$s7,8\n"                  \
	"addu " lane "," lane ",$t6\n"     \
	"sll " lane "," lane ",24\n"       \
	"or $t8,$t8," lane "\n"

// NOTE(aalhendi): Keep the adjacent RNG fragments aligned as instructions;
// clang-format otherwise reads them as nested expressions. Native C is formatted.
// clang-format off
__asm__(".section .RenderStars,\"ax\",@progbits\n"
        ".align 2\n"
        ".ent\tRenderStars\n"
        ".set\tnoreorder\n"
        ".set\tnoat\n"
        ".globl RenderStars\n"
        "RenderStars:\n"
        "lui $at,0x1f80\n"
        "sw $s0,0($at)\n"
        "sw $s1,4($at)\n"
        "sw $s2,8($at)\n"
        "sw $s3,12($at)\n"
        "sw $s4,16($at)\n"
        "sw $s5,20($at)\n"
        "sw $s6,24($at)\n"
        "sw $s7,28($at)\n"
        "sw $gp,32($at)\n"
        "sw $sp,36($at)\n"
        "sw $s8,40($at)\n"
        "sw $ra,44($at)\n"
        "ctc2 $zero,$5\n"
        "ctc2 $zero,$6\n"
        "ctc2 $zero,$7\n"
        "addiu $a3,$a3,-1\n"
        "lw $v0,12($a1)\n"
        ".Lstars_player:\n"
        "lw $t7,40($a0)\n"
        "lw $s0,44($a0)\n"
        "lw $s1,48($a0)\n"
        "lw $s2,52($a0)\n"
        "lw $s3,56($a0)\n"
        "ctc2 $t7,$0\n"
        "ctc2 $s0,$1\n"
        "ctc2 $s1,$2\n"
        "ctc2 $s2,$3\n"
        "ctc2 $s3,$4\n"
        "lh $v1,32($a0)\n"
        "lh $t0,34($a0)\n"
        "lw $t1,24($a0)\n"
        "lw $s5,32($a0)\n"
        "lw $s4,244($a0)\n"
        "sll $v1,$v1,15\n"
        "sll $t0,$t0,15\n"
        "ctc2 $v1,$24\n"
        "ctc2 $t0,$25\n"
        "ctc2 $t1,$26\n"
        "ctc2 $zero,$5\n"
        "ctc2 $zero,$6\n"
        "ctc2 $zero,$7\n"
        "lhu $s6,0($a2)\n"
        "lhu $t9,2($a2)\n"
        "lhu $gp,4($a2)\n"
        "lhu $v1,6($a2)\n"
        "addi $gp,$gp,20\n"
        "sll $v1,$v1,2\n"
        "addu $s4,$s4,$v1\n"
        "lui $s7,0x3012\n"
        "ori $s7,$s7,0x5400\n"
        "lui $t8,0x4935\n"
        "bgez $zero,.Lstars_next\n"
        "ori $t8,$t8,0x83fe\n"
        ".Lstars_star:\n"
        RENDER_STARS_RNG("$v1", "$v1")
        RENDER_STARS_RNG("$t0", "$v1")
        RENDER_STARS_RNG("$t1", "$v1")
        "sra $v1,$v1,20\n"
        "srav $t0,$t0,$gp\n"
        "sra $t1,$t1,20\n"
        "bgez $v1,.Lstars_abs_y\n"
        "move $t2,$v1\n"
        "sub $t2,$zero,$v1\n"
        ".Lstars_abs_y:\n"
        "bgez $t0,.Lstars_abs_z\n"
        "move $t3,$t0\n"
        "bne $t9,$zero,.Lstars_abs_z\n"
        "sub $t3,$zero,$t0\n"
        "move $t0,$t3\n"
        ".Lstars_abs_z:\n"
        "bgez $t1,.Lstars_sort_y\n"
        "move $t4,$t1\n"
        "sub $t4,$zero,$t1\n"
        ".Lstars_sort_y:\n"
        "subu $t5,$t2,$t3\n"
        "bgtz $t5,.Lstars_sort_z\n"
        "move $t5,$t2\n"
        "move $t2,$t3\n"
        "move $t3,$t5\n"
        ".Lstars_sort_z:\n"
        "subu $t5,$t2,$t4\n"
        "bgtz $t5,.Lstars_scale\n"
        "move $t5,$t2\n"
        "move $t2,$t4\n"
        "move $t4,$t5\n"
        ".Lstars_scale:\n"
        "sra $t3,$t3,2\n"
        "sra $t4,$t4,2\n"
        "addu $t2,$t2,$t3\n"
        "addu $t2,$t2,$t4\n"
        "lui $t3,0x100\n"
        "div $zero,$t3,$t2\n"
        "mflo $t2\n"
        "nop\n"
        "nop\n"
        "mult $v1,$t2\n"
        "mflo $v1\n"
        "sra $v1,$v1,12\n"
        "andi $v1,$v1,0xffff\n"
        "mult $t0,$t2\n"
        "mflo $t0\n"
        "sra $t0,$t0,12\n"
        "sll $t0,$t0,16\n"
        "mult $t1,$t2\n"
        "or $v1,$v1,$t0\n"
        "mflo $t1\n"
        "sra $t1,$t1,12\n"
        "mtc2 $v1,$0\n"
        "mtc2 $t1,$1\n"
        RENDER_STARS_RNG("$sp", "$v1")
        "cop2 0x180001\n"
        "srl $sp,$sp,24\n"
        "ori $sp,$sp,0x40\n"
        "andi $sp,$sp,0xff\n"
        "sll $v1,$sp,8\n"
        "or $sp,$sp,$v1\n"
        "sll $v1,$v1,8\n"
        "or $sp,$sp,$v1\n"
        "lui $v1,0x6800\n"
        "or $sp,$sp,$v1\n"
        "lui $t4,0xd9\n"
        "ori $t4,$t4,0x200\n"
        "cfc2 $v1,$31\n"
        "mfc2 $t2,$14\n"
        "sll $v1,$v1,13\n"
        "bltz $v1,.Lstars_next\n"
        "subu $v1,$t2,$t4\n"
        "nor $v1,$v1,$t2\n"
        "bltz $v1,.Lstars_next\n"
        "sll $v1,$v1,16\n"
        "bltz $v1,.Lstars_next\n"
        "lui $t0,0x200\n"
        "lw $v1,0($s4)\n"
        "sw $sp,4($v0)\n"
        "or $v1,$v1,$t0\n"
        "sw $v1,0($v0)\n"
        "sll $t3,$v0,8\n"
        "srl $t3,$t3,8\n"
        "sw $t2,8($v0)\n"
        "sw $t3,0($s4)\n"
        "addiu $v0,$v0,12\n"
        ".Lstars_next:\n"
        "bgez $s6,.Lstars_star\n"
        "addiu $s6,$s6,-1\n"
        "lui $t0,0x200\n"
        "lui $v1,0xe100\n"
        "ori $v1,$v1,0xa20\n"
        "sw $v1,4($v0)\n"
        "sw $zero,8($v0)\n"
        "lw $v1,0($s4)\n"
        "sll $t2,$v0,8\n"
        "or $v1,$v1,$t0\n"
        "sw $v1,0($v0)\n"
        "srl $t2,$t2,8\n"
        "sw $t2,0($s4)\n"
        "addiu $v0,$v0,12\n"
        "addiu $a0,$a0,272\n"
        "bne $a3,$zero,.Lstars_player\n"
        "addiu $a3,$a3,-1\n"
        "sw $v0,12($a1)\n"
        "lw $ra,44($at)\n"
        "lw $s8,40($at)\n"
        "lw $sp,36($at)\n"
        "lw $gp,32($at)\n"
        "lw $s7,28($at)\n"
        "lw $s6,24($at)\n"
        "lw $s5,20($at)\n"
        "lw $s4,16($at)\n"
        "lw $s3,12($at)\n"
        "lw $s2,8($at)\n"
        "lw $s1,4($at)\n"
        "lw $s0,0($at)\n"
        "jr $ra\n"
        "nop\n"
        ".end RenderStars\n"
        ".set\tat\n"
        ".set\treorder\n"
        ".text\n");
// clang-format on

#undef RENDER_STARS_RNG

#endif
