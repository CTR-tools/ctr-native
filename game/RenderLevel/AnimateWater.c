#include <common.h>
#include <ctr_gte_transfer.h>

#ifdef CTR_NATIVE
void AnimateWaterVertex(struct WaterVert *waterVert, u16 colorOffset, s32 firstOffset, s32 secondOffset, u32 *visBits)
{
	u32 bits = *visBits;
	struct LevVertex *levVert;
	struct OVert *waterColor;
	u16 first, second;
	u32 rgb, farR, farG, farB, color;

	*visBits = bits >> 1;
	if ((bits & 1) == 0)
		return;
	levVert = waterVert->v;
	waterColor = waterVert->w;
	first = CTR_ReadU16LE((char *)waterColor + firstOffset);
	second = CTR_ReadU16LE((char *)waterColor + secondOffset);
	rgb = first & 0x3f;
	farR = (second & 0x3f) << 4;
	farG = (second & 0xfc0) >> 2;
	farB = (second & 0xf000) >> 4;
	rgb |= (first & 0xfc0) << 2;
	rgb |= (first & 0xf000) << 8;
	rgb |= (first & 0xf000) << 4;
	farB |= (second & 0xf000) >> 8;
	MTC2(rgb, 6);
	CTC2(farR, 21);
	CTC2(farG, 22);
	CTC2(farB, 23);
	gte_dpcs();
	color = MFC2(22);
	CTR_WriteU16LE(&levVert->color_lo[0], (u16)(color + colorOffset));
	color = (color >> 16) & 0xff;
	CTR_WriteU32LE(&levVert->color_hi[0], (color << 16) | (color << 8) | color);
}

#define ANIMATE_WATER_LISTS 3
#include "Animate_ScratchFrame.h"
void AnimateWater3P(s32 timer, s32 numWaterVertices, struct WaterVert *waterVert, const struct TextureLayout *waterEnvMap, s32 *visOVertList0,
                    s32 *visOVertList1, s32 *visOVertList2)
{
#include "AnimateWater_Body.inc"
}
#undef ANIMATE_WATER_LISTS
#undef ANIMATE_SCRATCH_DECLARE
#undef ANIMATE_SCRATCH_SAVE
#undef ANIMATE_SCRATCH_RESTORE

#define ANIMATE_WATER_LISTS 4
#include "Animate_ScratchFrame.h"
void AnimateWater4P(s32 timer, s32 numWaterVertices, struct WaterVert *waterVert, const struct TextureLayout *waterEnvMap, s32 *visOVertList0,
                    s32 *visOVertList1, s32 *visOVertList2, s32 *visOVertList3)
{
#include "AnimateWater_Body.inc"
}
#undef ANIMATE_WATER_LISTS
#undef ANIMATE_SCRATCH_DECLARE
#undef ANIMATE_SCRATCH_SAVE
#undef ANIMATE_SCRATCH_RESTORE

// Native unity includes each shared leaf once; MIPS compiles these as real TUs.
#include "AnimateQuadVertex.c"
#include "AnimateQuad.c"
#include "AnimateWater1P.c"
#include "AnimateWater2P.c"
#else

// NOTE(aalhendi): These retail adapters save S0-S7/RA at scratch 0x00..0x28,
// not in a C stack frame. Three players borrow GP as list 2; four also borrow
// SP as list 3, only after all original-stack arguments have been read.
// Kernels preserve A0-A3, T3/T4, T6 and list cursors; T5 is shifted. Ordinary
// setup/visibility logic remains ASM here; native entries 1-4 share the C body.
// WaterVertex's invisible path uses 4P's final JR/NOP without its restores.
#define ANIMATE_SAVE_REGISTERS \
	"lui $at,0x1f80\n"         \
	"sw $s0,0($at)\n"          \
	"sw $s1,4($at)\n"          \
	"sw $s2,8($at)\n"          \
	"sw $s3,12($at)\n"         \
	"sw $s4,16($at)\n"         \
	"sw $s5,20($at)\n"         \
	"sw $s6,24($at)\n"         \
	"sw $s7,28($at)\n"

#define ANIMATE_RESTORE_REGISTERS(returnLabel) \
	"lw $s7,28($at)\n"                         \
	"lw $s6,24($at)\n"                         \
	"lw $s5,20($at)\n"                         \
	"lw $s4,16($at)\n"                         \
	"lw $s3,12($at)\n"                         \
	"lw $s2,8($at)\n"                          \
	"lw $s1,4($at)\n"                          \
	"lw $s0,0($at)\n" returnLabel "jr $ra\n"   \
	"nop\n"

#define ANIMATE_WATER_FRAME_SETUP \
	"addiu $a2,$a2,-8\n"          \
	"addiu $v1,$zero,28\n"        \
	"srl $t0,$a0,3\n"             \
	"divu $zero,$t0,$v1\n"        \
	"lhu $a3,0($a3)\n"            \
	"mfhi $t0\n"                  \
	"andi $t1,$a0,7\n"            \
	"sll $t1,$t1,9\n"             \
	"mtc2 $t1,$8\n"               \
	"addiu $t1,$t0,1\n"           \
	"divu $zero,$t1,$v1\n"

#define ANIMATE_WATER_READ_LIST0 \
	"lw $t5,0($t8)\n"            \
	"addiu $t8,$t8,4\n"

#define ANIMATE_WATER_READ_LIST1 \
	"lw $v1,0($t9)\n"            \
	"addiu $t9,$t9,4\n"          \
	"or $t5,$t5,$v1\n"

#define ANIMATE_WATER_READ_LIST2 \
	"lw $v1,0($gp)\n"            \
	"addiu $gp,$gp,4\n"          \
	"or $t5,$t5,$v1\n"

#define ANIMATE_WATER_READ_LIST3 \
	"lw $v1,0($sp)\n"            \
	"addiu $sp,$sp,4\n"          \
	"or $t5,$t5,$v1\n"

__asm__(".section .AnimateWater3P,\"ax\",@progbits\n"
        ".align 2\n"
        ".ent\tAnimateWater3P\n"
        ".set\tnoreorder\n"
        ".set\tnoat\n"
        ".globl AnimateWater3P\n"
        "AnimateWater3P:\n" ANIMATE_SAVE_REGISTERS "sw $gp,32($at)\n"
        "sw $ra,40($at)\n"
        "lw $t8,16($sp)\n"
        "lw $t9,20($sp)\n"
        "lw $gp,24($sp)\n" ANIMATE_WATER_FRAME_SETUP ANIMATE_WATER_READ_LIST0 ANIMATE_WATER_READ_LIST1 ANIMATE_WATER_READ_LIST2 "sll $t3,$t0,1\n"
        "mfhi $t1\n"
        "addiu $t6,$zero,32\n"
        "bgez $zero,.LAnimateWater3PTest\n"
        "sll $t4,$t1,1\n"
        ".LAnimateWater3PVertexCall:\n"
        "jal AnimateWaterVertex\n"
        "addiu $a2,$a2,8\n"
        ".LAnimateWater3PTest:\n"
        "beqz $a1,.LAnimateWater3PRestore\n"
        "addiu $a1,$a1,-1\n"
        "bnez $t6,.LAnimateWater3PVertexCall\n"
        "addiu $t6,$t6,-1\n" ANIMATE_WATER_READ_LIST0 ANIMATE_WATER_READ_LIST1 ANIMATE_WATER_READ_LIST2 "bgez $zero,.LAnimateWater3PVertexCall\n"
        "addiu $t6,$zero,31\n"
        ".LAnimateWater3PRestore:\n"
        "lw $ra,40($at)\n"
        "lw $gp,32($at)\n" ANIMATE_RESTORE_REGISTERS(".LAnimateWater3PReturn:\n") ".end AnimateWater3P\n"
                                                                                  ".set\tat\n"
                                                                                  ".set\treorder\n"
                                                                                  ".text\n");

__asm__(".section .AnimateWater4P,\"ax\",@progbits\n"
        ".align 2\n"
        ".ent\tAnimateWater4P\n"
        ".set\tnoreorder\n"
        ".set\tnoat\n"
        ".globl AnimateWater4P\n"
        "AnimateWater4P:\n" ANIMATE_SAVE_REGISTERS "sw $gp,32($at)\n"
        "sw $sp,36($at)\n"
        "sw $ra,40($at)\n"
        "lw $t8,16($sp)\n"
        "lw $t9,20($sp)\n"
        "lw $gp,24($sp)\n"
        "lw $sp,28($sp)\n" ANIMATE_WATER_FRAME_SETUP ANIMATE_WATER_READ_LIST0 ANIMATE_WATER_READ_LIST1 ANIMATE_WATER_READ_LIST2 ANIMATE_WATER_READ_LIST3
        "sll $t3,$t0,1\n"
        "mfhi $t1\n"
        "addiu $t6,$zero,32\n"
        "bgez $zero,.LAnimateWater4PTest\n"
        "sll $t4,$t1,1\n"
        ".LAnimateWater4PVertexCall:\n"
        "jal AnimateWaterVertex\n"
        "addiu $a2,$a2,8\n"
        ".LAnimateWater4PTest:\n"
        "beqz $a1,.LAnimateWater4PRestore\n"
        "addiu $a1,$a1,-1\n"
        "bnez $t6,.LAnimateWater4PVertexCall\n"
        "addiu $t6,$t6,-1\n" ANIMATE_WATER_READ_LIST0 ANIMATE_WATER_READ_LIST1 ANIMATE_WATER_READ_LIST2 ANIMATE_WATER_READ_LIST3
        "bgez $zero,.LAnimateWater4PVertexCall\n"
        "addiu $t6,$zero,31\n"
        ".LAnimateWater4PRestore:\n"
        "lw $ra,40($at)\n"
        "lw $sp,36($at)\n"
        "lw $gp,32($at)\n" ANIMATE_RESTORE_REGISTERS(".LAnimateWater4PReturn:\n") ".end AnimateWater4P\n"
                                                                                  ".set\tat\n"
                                                                                  ".set\treorder\n"
                                                                                  ".text\n");

#undef ANIMATE_SAVE_REGISTERS
#undef ANIMATE_RESTORE_REGISTERS
#undef ANIMATE_WATER_FRAME_SETUP
#undef ANIMATE_WATER_READ_LIST0
#undef ANIMATE_WATER_READ_LIST1
#undef ANIMATE_WATER_READ_LIST2
#undef ANIMATE_WATER_READ_LIST3

// NOTE(aalhendi): These retail kernels have a custom register ABI, not the
// native C argument ABI: a2 is the vertex, t5 the shifting visibility word,
// a3 the water color offset, and t3/t4 the two water-frame byte offsets.
// Their invisible path shares the final wrapper's jr ra instead of creating
// another return. Preserve the explicit GTE pipeline and read delays.
__asm__(".section .AnimateWaterVertex,\"ax\",@progbits\n"
        ".align 2\n"
        ".ent\tAnimateWaterVertex\n"
        ".set\tnoreorder\n"
        ".globl AnimateWaterVertex\n"
        "AnimateWaterVertex:\n"
        "andi $v1,$t5,1\n"
        "beqz $v1,AnimateWater4P+(.LAnimateWater4PReturn-AnimateWater4P)\n"
        "srl $t5,$t5,1\n"
        "lw $t0,4($a2)\n"
        "lw $t7,0($a2)\n"
        "addu $v1,$t0,$t3\n"
        "addu $t0,$t0,$t4\n"
        "lhu $v1,0($v1)\n"
        "lhu $t2,0($t0)\n"
        "andi $t0,$v1,0x0fc0\n"
        "andi $t1,$v1,0xf000\n"
        "andi $v1,$v1,0x003f\n"
        "sll $t0,$t0,2\n"
        "or $v1,$v1,$t0\n"
        "sll $t1,$t1,8\n"
        "or $v1,$v1,$t1\n"
        "srl $t1,$t1,4\n"
        "or $v1,$v1,$t1\n"
        "mtc2 $v1,$6\n"
        "andi $t0,$t2,0x0fc0\n"
        "andi $t1,$t2,0xf000\n"
        "andi $v1,$t2,0x003f\n"
        "sll $v1,$v1,4\n"
        "srl $t0,$t0,2\n"
        "ctc2 $v1,$21\n"
        "ctc2 $t0,$22\n"
        "srl $t1,$t1,4\n"
        "srl $t0,$t1,4\n"
        "or $t1,$t1,$t0\n"
        "ctc2 $t1,$23\n"
        "nop\n"
        "nop\n"
        "cop2 0x0780010\n"
        "mfc2 $t0,$22\n"
        "nop\n"
        "addu $v1,$t0,$a3\n"
        "sh $v1,12($t7)\n"
        "srl $t0,$t0,16\n"
        "andi $t0,$t0,0xff\n"
        "sll $v1,$t0,8\n"
        "or $v1,$v1,$t0\n"
        "sll $v1,$v1,8\n"
        "or $v1,$v1,$t0\n"
        "jr $ra\n"
        "sw $v1,8($t7)\n"
        ".end AnimateWaterVertex\n"
        ".set\treorder\n"
        ".text\n");

#endif
