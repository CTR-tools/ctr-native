#include <common.h>

#ifdef CTR_NATIVE
struct AnimateQuadTrig
{
	s32 sin;
	s32 cos;
};

static struct AnimateQuadTrig AnimateQuad_GetTrig(u32 angle)
{
	struct AnimateQuadTrig trig;
	TRIG_AngleSinCos_r19r17r18(angle, &trig.sin, &trig.cos);
	return trig;
}

void AnimateQuadVertex(s32 timer, struct SCVert *scVert, u32 *visBits)
{
	u32 bits = *visBits;
	struct LevVertex *levVert;
	u32 offsetPosZw, offsetPosXy, originalY, animatedXy, animatedZw, color;
	s32 flags;
	struct AnimateQuadTrig trig;

	*visBits = bits >> 1;
	if ((bits & 1) == 0)
		return;
	levVert = scVert->v;
	offsetPosZw = (u32)scVert->offset_pos_zw;
	flags = (s32)offsetPosZw >> 16;
	trig = AnimateQuad_GetTrig((u32)(flags & 0x3fff) + (u32)timer);
	if (flags < 0)
	{
		offsetPosXy = (u32)scVert->offset_pos_xy;
		originalY = (offsetPosXy >> 16) << 16;
		animatedXy = offsetPosXy + (u32)(trig.sin >> 7);
		animatedZw = offsetPosZw + (u32)(trig.cos >> 7);
		CTR_WriteU32LE(&levVert->pos, animatedXy | originalY);
		levVert->pos.z = (s16)animatedZw;
	}
	if ((flags & 0x4000) != 0)
	{
		MTC2((u32)((trig.sin + 0x1000) >> 2), 8);
		MTC2((u32)scVert->offset_color_rgba, 6);
		gte_dpcs();
		color = MFC2(22);
		CTR_WriteU32LE(&levVert->color_hi[0], color);
		CTR_WriteU32LE(&levVert->color_lo[0], color);
	}
}
#else

// NOTE(aalhendi): The scenery kernel passes its angle in s3 to the retail
// trig provider, receives sine/cosine in s1/s2, and keeps its return in s4.
// Packed xy addition intentionally keeps retail carry/OR behavior, and the
// GTE result writes all four color bytes, including their packet padding.
__asm__(".section .AnimateQuadVertex,\"ax\",@progbits\n"
        ".align 2\n"
        ".ent\tAnimateQuadVertex\n"
        ".set\tnoreorder\n"
        ".globl AnimateQuadVertex\n"
        "AnimateQuadVertex:\n"
        "andi $v1,$t5,1\n"
        "beqz $v1,AnimateQuad_Return\n"
        "srl $t5,$t5,1\n"
        "move $s4,$ra\n"
        "lw $s7,8($a2)\n"
        "lw $t7,0($a2)\n"
        "lw $s5,4($a2)\n"
        "sra $s0,$s7,16\n"
        "andi $s3,$s0,0x3fff\n"
        "jal TRIG_AngleSinCos_r19r17r18\n"
        "addu $s3,$s3,$a0\n"
        "bgez $s0,.LAnimateQuadVertexColor\n"
        "srl $s6,$s5,16\n"
        "sll $s6,$s6,16\n"
        "sra $v1,$s1,7\n"
        "addu $s5,$s5,$v1\n"
        "sra $v1,$s2,7\n"
        "addu $s7,$s7,$v1\n"
        "or $s5,$s5,$s6\n"
        "sw $s5,0($t7)\n"
        "sh $s7,4($t7)\n"
        ".LAnimateQuadVertexColor:\n"
        "andi $v1,$s0,0x4000\n"
        "beqz $v1,.LAnimateQuadVertexReturn\n"
        "addiu $v1,$s1,0x1000\n"
        "sra $v1,$v1,2\n"
        "mtc2 $v1,$8\n"
        "lwc2 $6,12($a2)\n"
        "nop\n"
        "nop\n"
        "cop2 0x0780010\n"
        "swc2 $22,8($t7)\n"
        "swc2 $22,12($t7)\n"
        ".LAnimateQuadVertexReturn:\n"
        "jr $s4\n"
        "nop\n"
        ".end AnimateQuadVertex\n"
        ".set\treorder\n"
        ".text\n");

#endif
