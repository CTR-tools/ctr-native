#include <common.h>

#ifdef CTR_NATIVE

#define RENDER_WEATHER_XY_MASK   0xfffeffffu
#define RENDER_WEATHER_WRAP_MASK 0x07fe07ffu

struct RenderWeatherScratch
{
	u32 colorTop;
	u32 colorBottom;
	u32 packedCenterXY;
	u32 centerZ;
};

CTR_STATIC_ASSERT(sizeof(struct RenderWeatherScratch) == 0x10);
CTR_STATIC_ASSERT(offsetof(struct RenderWeatherScratch, colorTop) == 0x00);
CTR_STATIC_ASSERT(offsetof(struct RenderWeatherScratch, colorBottom) == 0x04);
CTR_STATIC_ASSERT(offsetof(struct RenderWeatherScratch, packedCenterXY) == 0x08);
CTR_STATIC_ASSERT(offsetof(struct RenderWeatherScratch, centerZ) == 0x0C);

CTR_STATIC_ASSERT(sizeof(LINE_G2) == 0x14);
CTR_STATIC_ASSERT(offsetof(LINE_G2, tag) == 0x00);
CTR_STATIC_ASSERT(offsetof(LINE_G2, r0) == 0x04);
CTR_STATIC_ASSERT(offsetof(LINE_G2, x0) == 0x08);
CTR_STATIC_ASSERT(offsetof(LINE_G2, r1) == 0x0C);
CTR_STATIC_ASSERT(offsetof(LINE_G2, x1) == 0x10);

static u32 RenderWeather_ReadWord(const void *base, int offset)
{
	return CTR_ReadU32LE((const u8 *)base + offset);
}

static struct TrigPair RenderWeather_TrigAngleSinCos(int angle)
{
	struct TrigPair pair;

	TRIG_AngleSinCos_r16r17r18(angle, &pair.sin, &pair.cos);
	return pair;
}

static u32 RenderWeather_NextRngByte(u32 *state0, u32 *state1)
{
	u32 state1Shifted = *state1 >> 8;
	u32 mixed = *state0 + state1Shifted;

	*state0 = (*state0 >> 8) | (*state1 << 24);
	mixed += *state0 >> 8;
	mixed <<= 24;
	*state1 = state1Shifted | mixed;

	return mixed;
}

static void RenderWeather_NextRng(u32 *state0, u32 *state1, u32 *xy, u32 *z)
{
	u32 xSeed = RenderWeather_NextRngByte(state0, state1);
	u32 ySeed = RenderWeather_NextRngByte(state0, state1);
	u32 zSeed = RenderWeather_NextRngByte(state0, state1);

	*xy = (u32)(((s32)xSeed >> 21) & 0xffff) | (u32)((s32)ySeed >> 5);
	*z = zSeed >> 21;
}

static int RenderWeather_IsVisible(u32 gteFlag, u32 sxy0, u32 sxy1, u32 screenBounds)
{
	u32 overlap;
	u32 bounds;

	if ((s32)(gteFlag << 14) < 0)
	{
		return 0;
	}

	overlap = sxy0 & sxy1;
	bounds = ~((sxy0 - screenBounds) | (sxy1 - screenBounds)) | overlap;
	if ((s32)bounds < 0)
	{
		return 0;
	}

	return (s32)(bounds << 16) >= 0;
}

void RenderWeather(struct PushBuffer *pb, struct PrimMem *primMem, struct RainBuffer *rainBuffer, u8 numPlyr, int gameMode1)
{
	u32 *prim = (u32 *)primMem->cursor;
	u32 *rainWords = (u32 *)(void *)rainBuffer;
	// NOTE(aalhendi): Default scratchpad 0x00..0x2f holds saved MIPS registers.
	// Native C relies on its host ABI and owns only the shared weather fields.
	struct RenderWeatherScratch *scratch = CTR_SCRATCHPAD_PTR(struct RenderWeatherScratch, 0x30);
	struct TrigPair trig;
	u32 centerX;
	u32 screenBounds;
	u32 *ot;
	s32 currentParticles;
	u32 particleCount;
	u32 scrollXY;
	u32 scrollZ;
	u32 velocityXY;
	u32 velocityZ;
	u32 scrollXYEnd;
	u32 scrollZEnd;
	u32 scrollXYStart;
	u32 scrollZStart;
	u32 cameraXY;
	s32 cameraZ;
	u32 prevCameraXY;
	s32 prevCameraZ;
	u32 cameraDeltaXY;
	u32 cameraCorrectionXY;
	u32 smoothedCameraXY;
	s32 smoothedCameraZ;
	u32 startXY;
	u32 endXY;
	u32 startZ;
	u32 endZ;
	u32 spanXY;
	u32 spanZ;
	u32 fillMode;
	u32 state0;
	u32 state1;
	u32 rngXY;
	u32 rngZ;

	(void)numPlyr;

	CTC2(0, 5);
	CTC2(0, 6);
	CTC2(0, 7);

	CTC2(RenderWeather_ReadWord(&pb->matrix_ViewProj, 0x00), 0);
	CTC2(RenderWeather_ReadWord(&pb->matrix_ViewProj, 0x04), 1);
	CTC2(RenderWeather_ReadWord(&pb->matrix_ViewProj, 0x08), 2);
	CTC2(RenderWeather_ReadWord(&pb->matrix_ViewProj, 0x0c), 3);
	CTC2(RenderWeather_ReadWord(&pb->matrix_ViewProj, 0x10), 4);

	trig = RenderWeather_TrigAngleSinCos(pb->rot.y);
	centerX = (u32)((trig.sin >> 2) + 0x400);
	scratch->packedCenterXY = 0x04000000u | centerX;
	scratch->centerZ = (u32)((trig.cos >> 2) + 0x400);

	screenBounds = RenderWeather_ReadWord(pb, 0x20);
	ot = &pb->ptrOT[rainBuffer->offsetOT];

	CTC2((u32)(s32)pb->rect.w << 15, 24);
	CTC2((u32)(s32)pb->rect.h << 15, 25);
	CTC2((u32)pb->distanceToScreen_PREV, 26);

	currentParticles = rainBuffer->numParticles_curr;
	if (gameMode1 == 0)
	{
		s32 maxParticles = (u16)rainBuffer->numParticles_max;
		s32 vanishRate = (u16)rainBuffer->vanishRate;
		s32 diff = maxParticles - currentParticles;

		if (maxParticles != currentParticles)
		{
			if (diff < 0)
			{
				currentParticles -= vanishRate;
				if (diff + vanishRate > 0)
				{
					currentParticles = maxParticles;
				}
			}
			else
			{
				currentParticles += vanishRate;
				if (diff - vanishRate < 0)
				{
					currentParticles = maxParticles;
				}
			}

			rainBuffer->numParticles_curr = currentParticles;
		}
	}

	particleCount = (u32)currentParticles;

	scrollXY = rainWords[2];
	scrollZ = rainWords[3];
	velocityXY = rainWords[4] & RENDER_WEATHER_XY_MASK;
	velocityZ = rainWords[5];

	scrollXYEnd = (scrollXY + velocityXY) & RENDER_WEATHER_XY_MASK;
	// NOTE(aalhendi): Scroll and endpoint Z use retail ADDU/SUBU wrapping,
	// including inputs whose signed sum exceeds the native s32 range.
	scrollZEnd = scrollZ + velocityZ;
	if (gameMode1 == 0)
	{
		rainWords[2] = scrollXYEnd;
		rainWords[3] = (u32)scrollZEnd;
	}

	scrollXYStart = (scrollXY + ((u32)((s32)velocityXY >> 1) & RENDER_WEATHER_XY_MASK)) & RENDER_WEATHER_XY_MASK;
	scrollZStart = scrollZ + (u32)CTR_MipsSra((s32)velocityZ, 1);

	cameraXY = RenderWeather_ReadWord(pb, 0x00) & RENDER_WEATHER_XY_MASK;
	cameraZ = pb->pos.z;
	prevCameraXY = rainWords[6];
	prevCameraZ = *(s16 *)(void *)((char *)rainBuffer + 0x1c);
	if (gameMode1 == 0)
	{
		rainWords[6] = cameraXY;
		*(s16 *)(void *)((char *)rainBuffer + 0x1c) = pb->pos.z;
	}

	cameraDeltaXY = cameraXY - prevCameraXY;
	// NOTE(aalhendi): Retail SRA/SLL wraps the packed negative camera delta;
	// shifting a negative C value left is undefined on the native target.
	cameraCorrectionXY = ((u32)CTR_MipsSra((s32)cameraDeltaXY, 20) << 17) | ((u32)CTR_MipsSra((s32)(cameraDeltaXY << 16), 19) & 0xffffu);
	smoothedCameraXY = (cameraXY - cameraCorrectionXY) & RENDER_WEATHER_XY_MASK;
	smoothedCameraZ = cameraZ - ((cameraZ - prevCameraZ) >> 3);

	startXY = ((scrollXYStart - smoothedCameraXY) & RENDER_WEATHER_XY_MASK) + centerX;
	startXY &= RENDER_WEATHER_XY_MASK;
	endXY = ((scrollXYEnd - cameraXY) & RENDER_WEATHER_XY_MASK) + centerX;
	endXY &= RENDER_WEATHER_XY_MASK;
	startZ = (scrollZStart - (u32)smoothedCameraZ) + scratch->centerZ;
	endZ = (scrollZEnd - (u32)cameraZ) + scratch->centerZ;
	spanXY = endXY - startXY;
	spanZ = (u32)(endZ - startZ);

	scratch->colorTop = rainBuffer->colorRGBA_top | 0x52000000u;
	scratch->colorBottom = rainBuffer->colorRGBA_bottom;
	fillMode = rainBuffer->fillMode;

	state0 = 0x30125400;
	state1 = 0x493583fe;
	RenderWeather_NextRng(&state0, &state1, &rngXY, &rngZ);

	while (particleCount != 0)
	{
		u32 xy0;
		u32 xy1;
		u32 z0;
		u32 z1;

		particleCount--;

		xy0 = ((rngXY + startXY) & RENDER_WEATHER_WRAP_MASK) - scratch->packedCenterXY;
		xy1 = ((rngXY + endXY) & RENDER_WEATHER_WRAP_MASK) - scratch->packedCenterXY;

		if ((u32)(xy1 - xy0) == spanXY)
		{
			xy0 &= RENDER_WEATHER_XY_MASK;
			xy1 &= RENDER_WEATHER_XY_MASK;

			MTC2(xy0, 0);
			MTC2(xy1, 2);
			MTC2(xy1, 4);

			z0 = ((rngZ + (u32)startZ) & 0x7ff);
			z1 = ((rngZ + (u32)endZ) & 0x7ff);
			if ((u32)(z1 - z0) == spanZ)
			{
				u32 sxy0;
				u32 sxy1;
				u32 gteFlag;

				z0 -= scratch->centerZ;
				z1 -= scratch->centerZ;

				MTC2(z0, 1);
				MTC2(z1, 3);
				MTC2(z1, 5);

				gte_rtpt_b();
				RenderWeather_NextRng(&state0, &state1, &rngXY, &rngZ);

				sxy0 = MFC2(12);
				gteFlag = CFC2(31);
				sxy1 = MFC2(13);

				if (RenderWeather_IsVisible(gteFlag, sxy0, sxy1, screenBounds))
				{
					LINE_G2 *line = (LINE_G2 *)prim;

					CtrGpu_WriteColorCode(&line->r0, scratch->colorTop);
					CtrGpu_WritePackedXY(&line->x0, sxy0);
					CtrGpu_WriteColorCode(&line->r1, scratch->colorBottom);
					CtrGpu_WritePackedXY(&line->x1, sxy1);
					CtrGpu_LinkPacketFullOTTag(ot, &line->tag, line, 0x04000000);
					prim = (u32 *)(line + 1);
				}

				continue;
			}
		}

		RenderWeather_NextRng(&state0, &state1, &rngXY, &rngZ);
	}

	struct CtrGpuDrawModePacket *drawMode = (struct CtrGpuDrawModePacket *)prim;
	drawMode->drawMode = fillMode;
	drawMode->terminator = 0;
	CtrGpu_LinkPacketFullOTTag(ot, &drawMode->tag, drawMode, 0x02000000);
	prim = (u32 *)(drawMode + 1);

	primMem->cursor = prim;
}
#else
// NOTE(aalhendi): This entry has ordinary stack arguments, but owns scratch
// 0x00..0x3f and repurposes sp/gp for RNG state between its register saves.
// Its signed trig call uses s0 -> s1/s2, not an ordinary C call ABI.
// The repeated RNG text includes the observed RTPT scheduling point.
// clang-format off
#define RW_RNG_BYTE(reg, pipe) \
	"sll $v1,$s1,24\n" \
	"srl $s1,$s1,8\n" pipe \
	"addu " reg ",$s0,$s1\n" \
	"srl $s0,$s0,8\n" \
	"or $s0,$s0,$v1\n" \
	"srl $v1,$s0,8\n" \
	"addu " reg "," reg ",$v1\n" \
	"sll " reg "," reg ",24\n" \
	"or $s1,$s1," reg "\n"

#define RW_NEXT_RNG(pipe) \
	RW_RNG_BYTE("$t9", pipe) RW_RNG_BYTE("$gp", "") RW_RNG_BYTE("$sp", "") \
	"sra $t9,$t9,21\n" "sra $gp,$gp,5\n" "andi $t9,$t9,0xffff\n"

__asm__(".section .RenderWeather,\"ax\",@progbits\n"
	".align 2\n" ".ent\tRenderWeather\n" ".set\tnoreorder\n" ".set\tnoat\n"
	".globl RenderWeather\n" "RenderWeather:\n"
	"lui $at,0x1f80\n"
	"sw $s0,0($at)\n" "sw $s1,4($at)\n" "sw $s2,8($at)\n"
	"sw $s3,12($at)\n" "sw $s4,16($at)\n" "sw $s5,20($at)\n"
	"sw $s6,24($at)\n" "sw $s7,28($at)\n" "sw $gp,32($at)\n"
	"sw $sp,36($at)\n" "sw $s8,40($at)\n" "sw $ra,44($at)\n"
	"lw $s8,16($sp)\n" "ctc2 $zero,$5\n" "ctc2 $zero,$6\n" "ctc2 $zero,$7\n"
	"addiu $a3,$a3,-1\n" "lw $v0,12($a1)\n"
	"lw $t3,40($a0)\n" "lw $t4,44($a0)\n" "lw $t5,48($a0)\n"
	"lw $t6,52($a0)\n" "lw $t7,56($a0)\n"
	"ctc2 $t3,$0\n" "lh $s0,8($a0)\n" "ctc2 $t4,$1\n"
	"ctc2 $t5,$2\n" "ctc2 $t6,$3\n"
	"jal TRIG_AngleSinCos_r16r17r18\n" "ctc2 $t7,$4\n"
	"sra $s2,$s2,2\n" "sra $s1,$s1,2\n"
	"addiu $s2,$s2,1024\n" "addiu $s1,$s1,1024\n"
	"lui $v1,0x400\n" "or $v1,$v1,$s1\n"
	"sw $v1,56($at)\n" "sw $s2,60($at)\n"
	"lh $v1,32($a0)\n" "lh $t0,34($a0)\n" "lw $t1,24($a0)\n"
	"lw $t4,32($a0)\n" "lw $t3,244($a0)\n"
	"sll $v1,$v1,15\n" "sll $t0,$t0,15\n"
	"ctc2 $v1,$24\n" "ctc2 $t0,$25\n" "ctc2 $t1,$26\n"
	"lw $t7,0($a2)\n" "bne $s8,$zero,.Lrw_count_done\n" "lhu $v1,4($a2)\n"
	"lhu $t0,6($a2)\n" "beq $v1,$t7,.Lrw_count_done\n" "subu $t1,$v1,$t7\n"
	"bgez $t1,.Lrw_count_up\n" "addu $t2,$t1,$t0\n"
	"blez $t2,.Lrw_count_store\n" "subu $t7,$t7,$t0\n"
	"bgez $zero,.Lrw_count_store\n" "move $t7,$v1\n"
	".Lrw_count_up:\n" "subu $t2,$t1,$t0\n"
	"bgez $t2,.Lrw_count_store\n" "addu $t7,$t7,$t0\n" "move $t7,$v1\n"
	".Lrw_count_store:\n" "sw $t7,0($a2)\n"
	".Lrw_count_done:\n" "lw $s3,8($a2)\n" "lw $s4,12($a2)\n"
	"lui $t5,0xfffe\n" "ori $t5,$t5,0xffff\n"
	"lui $t6,0x7fe\n" "ori $t6,$t6,0x7ff\n"
	"lw $s7,16($a2)\n" "lw $t8,20($a2)\n" "and $s7,$s7,$t5\n"
	"addu $s5,$s3,$s7\n" "addu $s6,$s4,$t8\n"
	"bne $s8,$zero,.Lrw_scroll_done\n" "and $s5,$s5,$t5\n"
	"sw $s5,8($a2)\n" "sw $s6,12($a2)\n"
	".Lrw_scroll_done:\n" "sra $s7,$s7,1\n" "sra $t8,$t8,1\n" "and $s7,$s7,$t5\n"
	"addu $s3,$s3,$s7\n" "and $s3,$s3,$t5\n" "addu $s4,$s4,$t8\n"
	"lw $v1,0($a0)\n" "lh $t0,4($a0)\n" "lw $t1,24($a2)\n" "lh $t2,28($a2)\n"
	"bne $s8,$zero,.Lrw_camera_done\n" "and $v1,$v1,$t5\n"
	"sw $v1,24($a2)\n" "sh $t0,28($a2)\n"
	".Lrw_camera_done:\n" "subu $t1,$v1,$t1\n" "subu $t2,$t0,$t2\n"
	"sra $s7,$t1,20\n" "sll $s7,$s7,17\n" "sll $t1,$t1,16\n"
	"sra $t1,$t1,19\n" "andi $t1,$t1,0xffff\n" "or $t1,$t1,$s7\n"
	"sra $t2,$t2,3\n" "subu $t1,$v1,$t1\n" "subu $t2,$t0,$t2\n" "and $t1,$t1,$t5\n"
	"subu $s3,$s3,$t1\n" "subu $s5,$s5,$v1\n" "and $s3,$s3,$t5\n" "and $s5,$s5,$t5\n"
	"addu $s3,$s3,$s1\n" "addu $s5,$s5,$s1\n" "and $s3,$s3,$t5\n" "and $s5,$s5,$t5\n"
	"subu $s4,$s4,$t2\n" "subu $s6,$s6,$t0\n" "addu $s4,$s4,$s2\n" "addu $s6,$s6,$s2\n"
	"subu $s7,$s5,$s3\n" "subu $t8,$s6,$s4\n"
	"lw $v1,32($a2)\n" "lw $t0,36($a2)\n" "lw $t1,44($a2)\n" "lw $s2,40($a2)\n"
	"sll $t1,$t1,2\n" "addu $t3,$t3,$t1\n" "lui $t1,0x5200\n" "or $v1,$v1,$t1\n"
	"sw $v1,48($at)\n" "sw $t0,52($at)\n"
	"lui $s0,0x3012\n" "ori $s0,$s0,0x5400\n" "lui $s1,0x4935\n"
	"bgez $zero,.Lrw_rng\n" "ori $s1,$s1,0x83fe\n"
	".Lrw_particle:\n" "or $t0,$t9,$gp\n" "lw $t1,56($at)\n"
	"addu $v1,$t0,$s3\n" "addu $t0,$t0,$s5\n" "and $v1,$v1,$t6\n" "and $t0,$t0,$t6\n"
	"subu $v1,$v1,$t1\n" "subu $t0,$t0,$t1\n" "subu $t1,$t0,$v1\n"
	"bne $t1,$s7,.Lrw_rng\n" "and $v1,$v1,$t5\n" "and $t0,$t0,$t5\n"
	"mtc2 $v1,$0\n" "mtc2 $t0,$2\n" "mtc2 $t0,$4\n"
	"srl $t2,$sp,21\n" "addu $t0,$t2,$s4\n" "addu $t1,$t2,$s6\n"
	"andi $t0,$t0,0x7ff\n" "andi $t1,$t1,0x7ff\n" "subu $t2,$t1,$t0\n"
	"lw $v1,60($at)\n" "bne $t2,$t8,.Lrw_rng\n" "subu $t0,$t0,$v1\n"
	"subu $t1,$t1,$v1\n" "mtc2 $t0,$1\n" "mtc2 $t1,$3\n" "mtc2 $t1,$5\n"
	RW_NEXT_RNG("cop2 0x280030\n")
	"mfc2 $v1,$12\n" "cfc2 $t1,$31\n" "mfc2 $t0,$13\n" "sll $t1,$t1,14\n"
	"bltz $t1,.Lrw_next\n" "and $t1,$v1,$t0\n"
	"subu $v1,$v1,$t4\n" "subu $t0,$t0,$t4\n" "nor $v1,$v1,$t0\n" "or $v1,$v1,$t1\n"
	"bltz $v1,.Lrw_next\n" "sll $v1,$v1,16\n" "bltz $v1,.Lrw_next\n" "lw $v1,48($at)\n"
	"lw $t0,52($at)\n" "sw $v1,4($v0)\n" "sw $t0,12($v0)\n" "lw $t2,0($t3)\n"
	"sll $v1,$v0,8\n" "srl $v1,$v1,8\n" "lui $t1,0x400\n" "or $t2,$t2,$t1\n"
	"sw $t2,0($v0)\n" "swc2 $12,8($v0)\n" "swc2 $13,16($v0)\n" "sw $v1,0($t3)\n"
	"bgez $zero,.Lrw_next\n" "addiu $v0,$v0,20\n"
	".Lrw_rng:\n" RW_NEXT_RNG("")
	".Lrw_next:\n" "bne $t7,$zero,.Lrw_particle\n" "addiu $t7,$t7,-1\n"
	"sw $s2,4($v0)\n" "sw $zero,8($v0)\n" "lw $t2,0($t3)\n"
	"sll $v1,$v0,8\n" "lui $t1,0x200\n" "or $t2,$t2,$t1\n"
	"sw $t2,0($v0)\n" "srl $v1,$v1,8\n" "sw $v1,0($t3)\n"
	"addiu $v0,$v0,12\n" "sw $v0,12($a1)\n"
	"lw $ra,44($at)\n" "lw $s8,40($at)\n" "lw $sp,36($at)\n" "lw $gp,32($at)\n"
	"lw $s7,28($at)\n" "lw $s6,24($at)\n" "lw $s5,20($at)\n" "lw $s4,16($at)\n"
	"lw $s3,12($at)\n" "lw $s2,8($at)\n" "lw $s1,4($at)\n" "lw $s0,0($at)\n"
	"jr $ra\n" "nop\n"
	".end RenderWeather\n" ".set\tat\n" ".set\treorder\n" ".text\n");
#undef RW_RNG_BYTE
#undef RW_NEXT_RNG
// clang-format on
#endif
