#include <common.h>

static const u8 sDrawConfettiColorTable8008a2a0[36] = {
    0x39, 0x39, 0x3a, 0x3b, 0x3d, 0x3f, 0x41, 0x44, 0x47, 0x4b, 0x4f, 0x53, 0x58, 0x5d, 0x63, 0x69, 0x6f, 0x76,
    0x7d, 0x85, 0x8d, 0x95, 0x9e, 0xa7, 0xb1, 0xbb, 0xc5, 0xd0, 0xdb, 0xe7, 0xf3, 0xff, 0xff, 0xff, 0xff, 0xff,
};

struct DrawConfettiCorner
{
	u32 xy;
	s32 z;
};

struct DrawConfettiScratch
{
	u32 remainingParticles;
	u32 color;
	struct DrawConfettiCorner corners[4];
	u8 colorTable[36];
};

CTR_STATIC_ASSERT(offsetof(struct DrawConfettiScratch, remainingParticles) == 0x00);
CTR_STATIC_ASSERT(offsetof(struct DrawConfettiScratch, color) == 0x04);
CTR_STATIC_ASSERT(offsetof(struct DrawConfettiScratch, corners) == 0x08);
CTR_STATIC_ASSERT(offsetof(struct DrawConfettiScratch, colorTable) == 0x28);

#ifdef CTR_NATIVE

struct DrawConfettiRng
{
	u32 x;
	u32 y;
	u32 z;
};

static u32 DrawConfetti_ReadWord(const void *base, int offset)
{
	return CTR_ReadU32LE((const u8 *)base + offset);
}

static struct TrigPair DrawConfetti_TrigAngleSinCos(int angle)
{
	struct TrigPair pair;

	TRIG_AngleSinCos_r15r16r17(angle, &pair.sin, &pair.cos);
	return pair;
}

static u32 DrawConfetti_NextRngByte(u32 *state0, u32 *state1)
{
	u32 state1Shifted = *state1 >> 8;
	u32 mixed = *state0 + state1Shifted;

	*state0 = (*state0 >> 8) | (*state1 << 24);
	mixed += *state0 >> 8;
	mixed <<= 24;
	*state1 = state1Shifted | mixed;

	return mixed;
}

static struct DrawConfettiRng DrawConfetti_NextRng(u32 *state0, u32 *state1)
{
	struct DrawConfettiRng rng;

	rng.x = DrawConfetti_NextRngByte(state0, state1);
	rng.y = DrawConfetti_NextRngByte(state0, state1);
	rng.z = DrawConfetti_NextRngByte(state0, state1);

	return rng;
}

static u32 DrawConfetti_PackXY(s32 x, s32 y)
{
	return (u32)(u16)x | ((u32)(u16)y << 16);
}

static u32 DrawConfetti_BuildColor(u32 cycle, u8 shade)
{
	u32 color = 0x28000000;

	if ((cycle & 1) != 0)
	{
		color |= shade;
	}

	if ((cycle & 2) != 0)
	{
		color |= (u32)shade << 8;
	}

	if ((cycle & 4) != 0)
	{
		color |= (u32)shade << 16;
	}

	return color;
}

static int DrawConfetti_BoundsVisible(u32 sxy0, u32 sxy1, u32 sxy2, u32 sxy3, u32 screenBounds)
{
	u32 overlap;
	u32 bounds;

	overlap = sxy0 & sxy1 & sxy2 & sxy3;
	// Retail uses the raw fourth SXY here, not `sxy3 - screenBounds`.
	bounds = ~((sxy0 - screenBounds) | (sxy1 - screenBounds) | (sxy2 - screenBounds) | sxy3) | overlap;
	if ((s32)bounds < 0)
	{
		return 0;
	}

	return (s32)(bounds << 16) >= 0;
}

static void DrawConfetti_LinkPrimitive(POLY_F4 *poly, u32 *ot)
{
	CtrGpu_LinkPacket24(ot, &poly->tag, poly, 0x05000000);
}

void DrawConfetti(struct PushBuffer *pb, struct PrimMem *primMem, struct GameTrackerConfetti *confetti, int frameTimer, int gameMode1)
{
	POLY_F4 *prim = primMem->cursor;
	struct DrawConfettiScratch *scratch = CTR_SCRATCHPAD_PTR(struct DrawConfettiScratch, 0x30);
	struct TrigPair cameraTrig;
	u32 screenBounds;
	u32 *otBase;
	u32 currentParticles;
	u32 particleCount;
	s32 centerX;
	s32 centerZ;
	s32 baseX;
	s32 baseY;
	s32 baseZ;
	u32 timer;
	u32 cycle;
	u32 state0;
	u32 state1;
	struct DrawConfettiRng rng;

	// NOTE(aalhendi): Native relies on the host ABI instead of retail's GPR
	// saves at scratchpad 0x00-0x2c. Its data scratch range starts at 0x30.
	CTC2(0, 5);
	CTC2(0, 6);
	CTC2(0, 7);

	for (int i = 0; i < (int)sizeof(sDrawConfettiColorTable8008a2a0); i++)
	{
		scratch->colorTable[i] = sDrawConfettiColorTable8008a2a0[i];
	}

	CTC2(DrawConfetti_ReadWord(&pb->matrix_ViewProj, 0x00), 0);
	CTC2(DrawConfetti_ReadWord(&pb->matrix_ViewProj, 0x04), 1);
	CTC2(DrawConfetti_ReadWord(&pb->matrix_ViewProj, 0x08), 2);
	CTC2(DrawConfetti_ReadWord(&pb->matrix_ViewProj, 0x0c), 3);
	CTC2(DrawConfetti_ReadWord(&pb->matrix_ViewProj, 0x10), 4);

	cameraTrig = DrawConfetti_TrigAngleSinCos(pb->rot.y);
	centerX = (cameraTrig.sin >> 2) + 0x400;
	centerZ = (cameraTrig.cos >> 2) + 0x400;

	CTC2((u32)(s32)pb->rect.w << 15, 24);
	CTC2((u32)(s32)pb->rect.h << 15, 25);
	CTC2((u32)pb->distanceToScreen_PREV, 26);

	currentParticles = confetti->numParticles_currWord;
	if (gameMode1 == 0)
	{
		u32 maxParticles = (u16)confetti->numParticles_max;
		u32 vanishRate = (u16)confetti->vanishRate;
		u32 diff = maxParticles - currentParticles;

		if (maxParticles != currentParticles)
		{
			if ((s32)diff < 0)
			{
				currentParticles -= vanishRate;
				if ((s32)(diff + vanishRate) > 0)
				{
					currentParticles = maxParticles;
				}
			}
			else
			{
				currentParticles += vanishRate;
				if ((s32)(diff - vanishRate) < 0)
				{
					currentParticles = maxParticles;
				}
			}

			confetti->numParticles_currWord = currentParticles;
		}
	}

	screenBounds = DrawConfetti_ReadWord(pb, 0x20);
	otBase = pb->ptrOT;
	timer = (u32)frameTimer;
	baseX = -pb->pos.x + centerX;
	baseY = (s32)((u32)confetti->velY * timer - (u32)(s32)pb->pos.y);
	baseZ = -pb->pos.z + centerZ;

	scratch->color = 0x28000000;
	cycle = 6;
	state0 = 0x30125400;
	state1 = 0x493583fe;
	rng = DrawConfetti_NextRng(&state0, &state1);
	particleCount = currentParticles;

	while (1)
	{
		s32 x;
		s32 y;
		s32 z;
		s32 angle;
		struct TrigPair spinA;
		struct TrigPair spinB;
		u8 shade;
		s32 halfY;
		s32 skewX;
		s32 halfX;
		s32 skewZ;
		s32 halfZ;
		u32 xy0;
		u32 xy1;
		u32 xy2;
		u32 xy3;
		s32 z0;
		s32 z1;
		s32 z2;
		s32 z3;
		u32 sxy0;
		u32 sxy1;
		u32 sxy2;
		u32 sxy3;
		u32 flag0;
		u32 flag1;
		u32 angleWord;

		cycle--;
		timer += 23;
		if (cycle == 0)
		{
			cycle = 6;
		}

		if (particleCount == 0)
		{
			break;
		}

		particleCount--;
		scratch->remainingParticles = particleCount;

		x = (((s32)rng.x >> 21) + baseX) & 0x7ff;
		// NOTE(aalhendi): Retail ADDU wraps before ANDI, even at timer extrema.
		y = (s32)(((u32)((s32)rng.y >> 21) + (u32)baseY) & 0x7ffu);
		z = (((s32)rng.z >> 21) + baseZ) & 0x7ff;
		x -= centerX;
		y -= 0x400;
		z -= centerZ;

		angleWord = (cycle & 1) != 0 ? 0u - timer : timer;
		angle = (s32)(angleWord << 4);
		spinA = DrawConfetti_TrigAngleSinCos(angle);
		x += spinA.sin >> 5;
		z += spinA.cos >> 5;

		angle = (s32)((u32)angle << 3);
		spinB = DrawConfetti_TrigAngleSinCos(angle);

		shade = scratch->colorTable[((spinB.cos > 0) ? spinB.cos : -spinB.cos) >> 7];
		scratch->color = DrawConfetti_BuildColor(cycle, shade);

		halfY = spinB.cos >> 8;
		skewX = (spinB.sin * spinA.sin) >> 20;
		halfX = spinA.cos >> 8;
		skewZ = (spinB.sin * spinA.cos) >> 20;
		halfZ = spinA.sin >> 8;

		xy0 = DrawConfetti_PackXY(x - halfX - skewX, y - halfY);
		z0 = z - halfZ + skewZ;
		xy1 = DrawConfetti_PackXY(x + halfX - skewX, y - halfY);
		z1 = z + halfZ + skewZ;
		xy2 = DrawConfetti_PackXY(x - halfX + skewX, y + halfY);
		z2 = z - halfZ - skewZ;
		xy3 = DrawConfetti_PackXY(x + halfX + skewX, y + halfY);
		z3 = z + halfZ - skewZ;

		// NOTE(aalhendi): Retail writes all four scratch corners before the
		// GTE loads, including for rejected particles. Preserve that image.
		scratch->corners[0].xy = xy0;
		scratch->corners[0].z = z0;
		scratch->corners[1].xy = xy1;
		scratch->corners[1].z = z1;
		scratch->corners[2].xy = xy2;
		scratch->corners[2].z = z2;
		scratch->corners[3].xy = xy3;
		scratch->corners[3].z = z3;

		MTC2(xy0, 0);
		MTC2((u32)z0, 1);
		MTC2(xy1, 2);
		MTC2((u32)z1, 3);
		MTC2(xy2, 4);
		MTC2((u32)z2, 5);

		gte_rtpt_b();
		rng = DrawConfetti_NextRng(&state0, &state1);

		sxy0 = MFC2(12);
		flag0 = CFC2(31);
		sxy1 = MFC2(13);
		sxy2 = MFC2(14);
		CtrGpu_WritePackedXY(&prim->x0, sxy0);

		MTC2(xy3, 0);
		MTC2((u32)z3, 1);

		if ((s32)(flag0 << 14) >= 0)
		{
			gte_rtps_b();
			sxy3 = MFC2(14);
			flag1 = CFC2(31);

			if ((s32)(flag1 << 14) >= 0)
			{
				gte_avsz4_b();
				if (DrawConfetti_BoundsVisible(sxy0, sxy1, sxy2, sxy3, screenBounds))
				{
					u32 depth;
					u32 *ot;

					CtrGpu_WritePackedXY(&prim->x1, MFC2(12));
					CtrGpu_WritePackedXY(&prim->x2, MFC2(13));
					CtrGpu_WritePackedXY(&prim->x3, MFC2(14));
					depth = MFC2(24);
					CtrGpu_WriteColorCode(&prim->r0, scratch->color);
					ot = (u32 *)(void *)((char *)otBase + ((depth >> 18) << 2));
					DrawConfetti_LinkPrimitive(prim, ot);
					particleCount = scratch->remainingParticles;
					prim++;
					continue;
				}
			}
		}

		particleCount = scratch->remainingParticles;
	}

	primMem->cursor = prim;
}

#else

// NOTE(aalhendi): Retail owns at as the scratchpad base and repurposes gp/sp
// for RNG/vertex lanes after saving the ordinary caller's GPRs. The fifth
// argument is loaded from the original sp before those custom-register uses.
// Trig consumes t7 and returns s0/s1 while the other live lanes survive.
// These source instructions retain the GTE/load hazards and branch delays.
#define DRAW_CONFETTI_RNG_BYTE(lane) \
	"sll $v1,$t8,24\n"               \
	"srl $t8,$t8,8\n"                \
	"addu " lane ",$s7,$t8\n"        \
	"srl $s7,$s7,8\n"                \
	"or $s7,$s7,$v1\n"               \
	"srl $v1,$s7,8\n"                \
	"addu " lane "," lane ",$v1\n"   \
	"sll " lane "," lane ",24\n"     \
	"or $t8,$t8," lane "\n"
#define DRAW_CONFETTI_RNG DRAW_CONFETTI_RNG_BYTE("$t9") DRAW_CONFETTI_RNG_BYTE("$gp") DRAW_CONFETTI_RNG_BYTE("$sp")

// NOTE(aalhendi): clang-format reads adjacent RNG fragments as nested
// expressions. Keep this instruction listing aligned; native C stays formatted.
// clang-format off
__asm__(".section .DrawConfetti,\"ax\",@progbits\n"
        ".align 2\n"
        ".ent\tDrawConfetti\n"
        ".set\tnoreorder\n"
        ".set\tnoat\n"
        ".globl DrawConfetti\n"
        "DrawConfetti:\n"
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
        "lw $v0,12($a1)\n"
        "lw $s8,16($sp)\n"
        "lui $v1,%hi(sDrawConfettiColorTable8008a2a0)\n"
        "addiu $v1,$v1,%lo(sDrawConfettiColorTable8008a2a0)\n"
        "addiu $t0,$at,88\n"
        "addiu $t1,$zero,8\n"
        ".Lconfetti_copy_shades:\n"
        "lw $t2,0($v1)\n"
        "addiu $v1,$v1,4\n"
        "sw $t2,0($t0)\n"
        "addiu $t0,$t0,4\n"
        "bgtz $t1,.Lconfetti_copy_shades\n"
        "addiu $t1,$t1,-1\n"
        "lw $v1,40($a0)\n"
        "lw $t0,44($a0)\n"
        "lw $t1,48($a0)\n"
        "lw $t2,52($a0)\n"
        "lw $t3,56($a0)\n"
        "ctc2 $v1,$0\n"
        "lh $t7,8($a0)\n"
        "ctc2 $t0,$1\n"
        "ctc2 $t1,$2\n"
        "ctc2 $t2,$3\n"
        "jal TRIG_AngleSinCos_r15r16r17\n"
        "ctc2 $t3,$4\n"
        "sra $s0,$s0,2\n"
        "addiu $s5,$s0,1024\n"
        "sra $s1,$s1,2\n"
        "addiu $s6,$s1,1024\n"
        "lw $s3,8($a2)\n"
        "addiu $s2,$zero,0\n"
        "addiu $s4,$zero,0\n"
        "mult $s3,$a3\n"
        "lh $v1,32($a0)\n"
        "lh $t0,34($a0)\n"
        "lw $t1,24($a0)\n"
        "lw $t4,32($a0)\n"
        "lw $t3,244($a0)\n"
        "sll $v1,$v1,15\n"
        "sll $t0,$t0,15\n"
        "ctc2 $v1,$24\n"
        "ctc2 $t0,$25\n"
        "ctc2 $t1,$26\n"
        "lw $t5,0($a2)\n"
        "bne $s8,$zero,.Lconfetti_particle_count_ready\n"
        "lhu $v1,4($a2)\n"
        "lhu $t0,6($a2)\n"
        "beq $v1,$t5,.Lconfetti_particle_count_ready\n"
        "subu $t1,$v1,$t5\n"
        "bgez $t1,.Lconfetti_particle_count_grow\n"
        "addu $t2,$t1,$t0\n"
        "blez $t2,.Lconfetti_store_particle_count\n"
        "subu $t5,$t5,$t0\n"
        "bgez $zero,.Lconfetti_store_particle_count\n"
        "move $t5,$v1\n"
        ".Lconfetti_particle_count_grow:\n"
        "subu $t2,$t1,$t0\n"
        "bgez $t2,.Lconfetti_store_particle_count\n"
        "addu $t5,$t5,$t0\n"
        "move $t5,$v1\n"
        ".Lconfetti_store_particle_count:\n"
        "sw $t5,0($a2)\n"
        ".Lconfetti_particle_count_ready:\n"
        "lh $v1,0($a0)\n"
        "lh $t0,2($a0)\n"
        "lh $t1,4($a0)\n"
        "mflo $s3\n"
        "subu $s2,$s2,$v1\n"
        "subu $s3,$s3,$t0\n"
        "subu $s4,$s4,$t1\n"
        "addu $s2,$s2,$s5\n"
        "addu $s4,$s4,$s6\n"
        "lui $t0,0x2800\n"
        "sw $t0,52($at)\n"
        "addiu $s8,$zero,6\n"
        "lui $s7,0x3012\n"
        "ori $s7,$s7,0x5400\n"
        "lui $t8,0x4935\n"
        "bgez $zero,.Lconfetti_initial_rng\n"
        "ori $t8,$t8,0x83fe\n"
        ".Lconfetti_particle:\n"
        "sw $t5,48($at)\n"
        "sra $t9,$t9,21\n"
        "sra $gp,$gp,21\n"
        "sra $sp,$sp,21\n"
        "addu $t9,$t9,$s2\n"
        "addu $gp,$gp,$s3\n"
        "addu $sp,$sp,$s4\n"
        "andi $t9,$t9,0x7ff\n"
        "andi $gp,$gp,0x7ff\n"
        "andi $sp,$sp,0x7ff\n"
        "subu $t9,$t9,$s5\n"
        "addiu $gp,$gp,-1024\n"
        "subu $sp,$sp,$s6\n"
        "andi $t2,$s8,1\n"
        "beq $t2,$zero,.Lconfetti_spin_angle\n"
        "move $t7,$a3\n"
        "sub $t7,$zero,$a3\n"
        ".Lconfetti_spin_angle:\n"
        "jal TRIG_AngleSinCos_r15r16r17\n"
        "sll $t7,$t7,4\n"
        "sra $t2,$s0,5\n"
        "addu $t9,$t9,$t2\n"
        "sra $t2,$s1,5\n"
        "addu $sp,$sp,$t2\n"
        "move $t1,$s0\n"
        "move $t2,$s1\n"
        "jal TRIG_AngleSinCos_r15r16r17\n"
        "sll $t7,$t7,3\n"
        "bgtz $s1,.Lconfetti_shade_index\n"
        "sra $t0,$s1,7\n"
        "sub $t0,$zero,$s1\n"
        "sra $t0,$t0,7\n"
        ".Lconfetti_shade_index:\n"
        "addu $t0,$t0,$at\n"
        "lbu $t0,88($t0)\n"
        "andi $v1,$s8,1\n"
        "beq $v1,$zero,.Lconfetti_green\n"
        "sb $zero,52($at)\n"
        "sb $t0,52($at)\n"
        ".Lconfetti_green:\n"
        "andi $v1,$s8,2\n"
        "beq $v1,$zero,.Lconfetti_blue\n"
        "sb $zero,53($at)\n"
        "sb $t0,53($at)\n"
        ".Lconfetti_blue:\n"
        "andi $v1,$s8,4\n"
        "beq $v1,$zero,.Lconfetti_corners\n"
        "sb $zero,54($at)\n"
        "sb $t0,54($at)\n"
        ".Lconfetti_corners:\n"
        "mult $s0,$t1\n"
        "sra $s1,$s1,8\n"
        "subu $t0,$gp,$s1\n"
        "sh $t0,58($at)\n"
        "sh $t0,66($at)\n"
        "addu $t0,$gp,$s1\n"
        "sh $t0,74($at)\n"
        "mflo $v1\n"
        "sh $t0,82($at)\n"
        "sra $v1,$v1,20\n"
        "mult $s0,$t2\n"
        "sra $s0,$s0,8\n"
        "sra $t2,$t2,8\n"
        "subu $t0,$t9,$t2\n"
        "subu $t0,$t0,$v1\n"
        "sh $t0,56($at)\n"
        "addu $t0,$t9,$t2\n"
        "subu $t0,$t0,$v1\n"
        "sh $t0,64($at)\n"
        "subu $t0,$t9,$t2\n"
        "addu $t0,$t0,$v1\n"
        "sh $t0,72($at)\n"
        "addu $t0,$t9,$t2\n"
        "addu $t0,$t0,$v1\n"
        "sh $t0,80($at)\n"
        "mflo $v1\n"
        "sra $t1,$t1,8\n"
        "sra $v1,$v1,20\n"
        "subu $t0,$sp,$t1\n"
        "addu $t0,$t0,$v1\n"
        "sw $t0,60($at)\n"
        "addu $t0,$sp,$t1\n"
        "addu $t0,$t0,$v1\n"
        "sw $t0,68($at)\n"
        "subu $t0,$sp,$t1\n"
        "subu $t0,$t0,$v1\n"
        "sw $t0,76($at)\n"
        "addu $t0,$sp,$t1\n"
        "subu $t0,$t0,$v1\n"
        "sw $t0,84($at)\n"
        "lwc2 $0,56($at)\n"
        "lwc2 $1,60($at)\n"
        "lwc2 $2,64($at)\n"
        "lwc2 $3,68($at)\n"
        "lwc2 $4,72($at)\n"
        "lwc2 $5,76($at)\n"
        "sll $v1,$t8,24\n"
        "srl $t8,$t8,8\n"
        "cop2 0x280030\n"
        "addu $t9,$s7,$t8\n"
        "srl $s7,$s7,8\n"
        "or $s7,$s7,$v1\n"
        "srl $v1,$s7,8\n"
        "addu $t9,$t9,$v1\n"
        "sll $t9,$t9,24\n"
        "or $t8,$t8,$t9\n"
        DRAW_CONFETTI_RNG_BYTE("$gp") DRAW_CONFETTI_RNG_BYTE("$sp")
        "mfc2 $v1,$12\n"
        "cfc2 $t2,$31\n"
        "mfc2 $t0,$13\n"
        "mfc2 $t1,$14\n"
        "swc2 $12,8($v0)\n"
        "lwc2 $0,80($at)\n"
        "lwc2 $1,84($at)\n"
        "sll $t2,$t2,14\n"
        "bltz $t2,.Lconfetti_next_particle\n"
        "and $t6,$v1,$t0\n"
        "cop2 0x180001\n"
        "and $t6,$t6,$t1\n"
        "subu $v1,$v1,$t4\n"
        "subu $t0,$t0,$t4\n"
        "subu $t1,$t1,$t4\n"
        "mfc2 $t2,$14\n"
        "or $v1,$v1,$t0\n"
        "or $v1,$v1,$t1\n"
        "nor $v1,$v1,$t2\n"
        "cfc2 $t1,$31\n"
        "and $t6,$t6,$t2\n"
        "sll $t1,$t1,14\n"
        "bltz $t1,.Lconfetti_next_particle\n"
        "or $v1,$v1,$t6\n"
        "cop2 0x168002e\n"
        "bltz $v1,.Lconfetti_next_particle\n"
        "sll $v1,$v1,16\n"
        "bltz $v1,.Lconfetti_next_particle\n"
        "lw $t0,52($at)\n"
        "swc2 $12,12($v0)\n"
        "swc2 $13,16($v0)\n"
        "swc2 $14,20($v0)\n"
        "mfc2 $t6,$24\n"
        "sw $t0,4($v0)\n"
        "srl $t6,$t6,18\n"
        "sll $t6,$t6,2\n"
        "addu $t6,$t6,$t3\n"
        "lw $t2,0($t6)\n"
        "sll $v1,$v0,8\n"
        "srl $v1,$v1,8\n"
        "lui $t1,0x500\n"
        "or $t2,$t2,$t1\n"
        "sw $t2,0($v0)\n"
        "sw $v1,0($t6)\n"
        "lw $t5,48($at)\n"
        "bgez $zero,.Lconfetti_next_particle\n"
        "addiu $v0,$v0,24\n"
        ".Lconfetti_initial_rng:\n"
        DRAW_CONFETTI_RNG
        ".Lconfetti_next_particle:\n"
        "addiu $s8,$s8,-1\n"
        "bne $s8,$zero,.Lconfetti_cycle_ready\n"
        "addiu $a3,$a3,23\n"
        "addiu $s8,$zero,6\n"
        ".Lconfetti_cycle_ready:\n"
        "bne $t5,$zero,.Lconfetti_particle\n"
        "addiu $t5,$t5,-1\n"
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
        ".end DrawConfetti\n"
        ".set\tat\n"
        ".set\treorder\n"
        ".text\n");
// clang-format on

#undef DRAW_CONFETTI_RNG
#undef DRAW_CONFETTI_RNG_BYTE

#endif
