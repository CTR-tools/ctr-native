#include <common.h>

#ifdef CTR_NATIVE

#include "COLL_FixedTriangleShared.h"

internal void COLL_FIXED_TRIANGL_TestPoint_Body(struct ScratchpadStruct *sps, struct BspSearchVertex *v1, struct BspSearchVertex *v2,
                                                struct BspSearchVertex *v3, s32 normalZW);

void COLL_FIXED_TRIANGL_UNUSED(struct ScratchpadStruct *sps, struct BspSearchVertex *v1, struct BspSearchVertex *v2, struct BspSearchVertex *v3)
{
	// NOTE(aalhendi): Retail skips the TestPoint setup and jumps into the
	// shared body with t2 preloaded from sps+0x58. Native makes a0-a3 and t2
	// explicit.
	COLL_FIXED_TRIANGL_TestPoint_Body(sps, v1, v2, v3, (s32)CTR_PackS16Pair(sps->candidate.plane.normal.z, sps->candidate.plane.halfDistance));
}


internal void COLL_FIXED_TRIANGL_TestPoint_Body(struct ScratchpadStruct *sps, struct BspSearchVertex *v1, struct BspSearchVertex *v2,
                                                struct BspSearchVertex *v3, s32 normalZW)
{
	struct CollTriangleProjection projection;
	struct BspSearchVertex *baryVertex1;
	struct BspSearchVertex *baryVertex2;
	struct QuadBlock *quad;
	CollNormalAxis normalAxis;
	s32 startX;
	s32 startY;
	s32 startZ;
	s32 deltaX;
	s32 deltaY;
	s32 deltaZ;
	s32 lineDot;
	s32 planeDot;
	s32 factor;
	s32 hitX;
	s32 hitY;
	s32 hitZ;
	s32 baryA;
	s32 baryB;

	startX = sps->Union.QuadBlockColl.pos.x;
	startY = sps->Union.QuadBlockColl.pos.y;
	startZ = sps->Union.QuadBlockColl.pos.z;
	deltaX = sps->Union.QuadBlockColl.hitPos.x - startX;
	deltaY = sps->Union.QuadBlockColl.hitPos.y - startY;
	deltaZ = sps->Union.QuadBlockColl.hitPos.z - startZ;

	CollFixed_GteLoadR11R12(CTR_PackS16Pair(startX, startY));
	CollFixed_GteLoadR13R21(CTR_PackS16Pair(startZ, deltaX));
	CollFixed_GteLoadR22R23(CTR_PackS16Pair(deltaY, deltaZ));
	CollFixed_GteLoadVXY0(CTR_PackS16Pair(sps->candidate.plane.normal.x, sps->candidate.plane.normal.y));
	CollFixed_GteLoadVZ0(normalZW);

	normalZW = CTR_MipsSll(CTR_MipsSra(normalZW, 16), 13);

	CollFixed_GteMVMVA();
	lineDot = CollFixed_GteReadMAC2();
	planeDot = CTR_MipsSubLo(CollFixed_GteReadMAC1(), normalZW);

	if (lineDot >= 0)
	{
		return;
	}

	factor = CTR_MipsDiv(CTR_MipsNegLo(planeDot), CTR_MipsSra(lineDot, 12));

	CollFixed_GteLoadMAC(startX, startY, startZ);
	CollFixed_GteLoadIR(deltaX, deltaY, deltaZ);
	CollFixed_GteLoadIR0(factor);

	if ((factor < 0) || (factor > COLL_FRACTION_ONE))
	{
		return;
	}

	CollFixed_GteGPL12();
	hitX = CollFixed_GteReadMAC1();
	hitY = CollFixed_GteReadMAC2();
	hitZ = CollFixed_GteReadMAC3();

	CTR_SET_VEC3(CTR_VECTOR_DATA(&(sps->candidate.hitPos)), (s16)hitX, (s16)hitY, (s16)hitZ);

	baryVertex1 = v2;
	baryVertex2 = v3;
	normalAxis = sps->candidate.normalAxis;

	if (normalAxis == COLL_NORMAL_AXIS_Y)
	{
		s32 origin = v1->pos.z;
		projection.firstA = v2->pos.z - origin;
		projection.firstB = v3->pos.z - origin;
		projection.firstHit = hitZ - origin;

		if (Coll_MipsAbsS32(projection.firstA) < Coll_MipsAbsS32(projection.firstB))
		{
			s32 tmp = projection.firstA;
			projection.firstA = projection.firstB;
			projection.firstB = tmp;
			baryVertex1 = v3;
			baryVertex2 = v2;
		}

		origin = v1->pos.x;
		projection.secondA = baryVertex1->pos.x - origin;
		projection.secondB = baryVertex2->pos.x - origin;
		projection.secondHit = hitX - origin;
	}
	else if (normalAxis == COLL_NORMAL_AXIS_Z)
	{
		s32 origin = v1->pos.x;
		projection.firstA = v2->pos.x - origin;
		projection.firstB = v3->pos.x - origin;
		projection.firstHit = hitX - origin;

		if (Coll_MipsAbsS32(projection.firstA) < Coll_MipsAbsS32(projection.firstB))
		{
			s32 tmp = projection.firstA;
			projection.firstA = projection.firstB;
			projection.firstB = tmp;
			baryVertex1 = v3;
			baryVertex2 = v2;
		}

		origin = v1->pos.y;
		projection.secondA = baryVertex1->pos.y - origin;
		projection.secondB = baryVertex2->pos.y - origin;
		projection.secondHit = hitY - origin;
	}
	else
	{
		s32 origin = v1->pos.y;
		projection.firstA = v2->pos.y - origin;
		projection.firstB = v3->pos.y - origin;
		projection.firstHit = hitY - origin;

		if (Coll_MipsAbsS32(projection.firstA) < Coll_MipsAbsS32(projection.firstB))
		{
			s32 tmp = projection.firstA;
			projection.firstA = projection.firstB;
			projection.firstB = tmp;
			baryVertex1 = v3;
			baryVertex2 = v2;
		}

		origin = v1->pos.z;
		projection.secondA = baryVertex1->pos.z - origin;
		projection.secondB = baryVertex2->pos.z - origin;
		projection.secondHit = hitZ - origin;
	}

	baryA = COLL_FRACTION_INVALID;
	baryB = COLL_FRACTION_INVALID;

	if (projection.firstA != 0)
	{
		s32 denom = CTR_MipsSra(CTR_MipsSubLo(CTR_MipsMulLo(projection.secondB, projection.firstA), CTR_MipsMulLo(projection.firstB, projection.secondA)), 6);

		if (denom != 0)
		{
			baryB = CTR_MipsDiv(
			    CTR_MipsSll(CTR_MipsSubLo(CTR_MipsMulLo(projection.secondHit, projection.firstA), CTR_MipsMulLo(projection.firstHit, projection.secondA)), 6),
			    denom);

			if ((baryB >= 0) && (baryB <= COLL_FRACTION_ONE))
			{
				baryA = CTR_MipsDiv(CTR_MipsSubLo(CTR_MipsSll(projection.firstHit, 12), CTR_MipsMulLo(baryB, projection.firstB)), projection.firstA);
			}
		}
	}
	else
	{
		if (projection.firstB == 0)
		{
			return;
		}

		baryB = CTR_MipsDiv(CTR_MipsSll(projection.firstHit, 12), projection.firstB);

		if ((baryB >= 0) && (baryB <= COLL_FRACTION_ONE))
		{
			baryA = CTR_MipsDiv(CTR_MipsSubLo(CTR_MipsSll(projection.secondHit, 12), CTR_MipsMulLo(baryB, projection.secondB)), projection.secondA);
		}
	}

	quad = sps->candidate.ptrQuadblock;

	if ((baryA < 0) || (CTR_MipsSubLo(CTR_MipsAddLo(baryA, baryB), COLL_FRACTION_ONE) > 0))
	{
		return;
	}

	if ((quad->quadFlags & QUADBLOCK_FLAG_TRIGGER) != 0)
	{
		sps->collision.stepFlags |= (u8)quad->terrain_type;
		return;
	}

	sps->hit.ptrQuadblock = quad;
	sps->hitBarycentrics.v1 = (s16)baryA;
	sps->hitBarycentrics.v2 = (s16)baryB;
	sps->hitLevelTriangle.v0 = v1->pLevelVertex;
	sps->hitLevelTriangle.v1 = baryVertex1->pLevelVertex;
	sps->hitLevelTriangle.v2 = baryVertex2->pLevelVertex;
	sps->boolDidTouchQuadblock = (s16)CTR_MipsAddLo(sps->boolDidTouchQuadblock, 1);
	sps->hit.hitPos = sps->candidate.hitPos;
	sps->Union.QuadBlockColl.hitPos = sps->candidate.hitPos;
	sps->hit.plane = sps->candidate.plane;
}

void COLL_FIXED_TRIANGL_TestPoint(struct ScratchpadStruct *sps, struct BspSearchVertex *v1, struct BspSearchVertex *v2, struct BspSearchVertex *v3)
{
	s32 normalZW = (s32)CTR_PackS16Pair(v1->plane.normal.z, v1->plane.halfDistance);

	sps->numTrianglesTested = (s16)CTR_MipsAddLo(sps->numTrianglesTested, 1);
	sps->candidate.normalAxis = v1->normalAxis;
	sps->candidate.plane = v1->plane;

	COLL_FIXED_TRIANGL_TestPoint_Body(sps, v1, v2, v3, normalZW);
}

#else

// NOTE(aalhendi): These entries share a frame-free GTE kernel. UNUSED supplies
// packed normalZW in t2 through its call delay slot; TestPoint falls through
// after its nine-instruction setup. The kernel preserves a0/t9/ra/sp, clobbers
// s0-s2 and HI/LO, and retains ordered GTE/scratch effects on rejected paths.
// Keep this custom-entry ABI in ASM, with s0-s2 saved by the outward callers.
// Native uses the explicit C body and bounded wrap/divide behavior above;
// physical register images, overflow/divide exceptions and timing are not modeled.
// clang-format off
__asm__(".section .COLL_FIXED_TRIANGL_UNUSED,\"ax\",@progbits\n"
	".align 2\n"
	".globl COLL_FIXED_TRIANGL_UNUSED\n"
	".ent\tCOLL_FIXED_TRIANGL_UNUSED\n"
	".set\tnoreorder\n"
	"COLL_FIXED_TRIANGL_UNUSED:\n"
	// Save the inner entry's callee-saved clobbers; supply its implicit t2.
	"addiu $sp,$sp,-32\n"
	"sw $s0,16($sp)\n"
	"sw $s1,20($sp)\n"
	"sw $ra,28($sp)\n"
	"sw $s2,24($sp)\n"
	"jal COLL_FIXED_TRIANGL_TestPoint_Inner\n"
	"lw $t2,88($a0)\n"
	"lw $ra,28($sp)\n"
	"lw $s2,24($sp)\n"
	"lw $s1,20($sp)\n"
	"lw $s0,16($sp)\n"
	"jr $ra\n"
	"addiu $sp,$sp,32\n"
	".end COLL_FIXED_TRIANGL_UNUSED\n"
	".section .COLL_FIXED_TRIANGL_TestPoint,\"ax\",@progbits\n"
	".align 2\n"
	".globl COLL_FIXED_TRIANGL_TestPoint\n"
	".ent\tCOLL_FIXED_TRIANGL_TestPoint\n"
	"COLL_FIXED_TRIANGL_TestPoint:\n"
	// TestPoint alone counts the triangle and caches the first vertex's plane.
	"lh $v1,60($a0)\n"
	"lh $t0,6($a1)\n"
	"lw $t1,12($a1)\n"
	"lw $t2,16($a1)\n"
	"addiu $v1,$v1,1\n"
	"sh $v1,60($a0)\n"
	"sh $t0,82($a0)\n"
	"sw $t1,84($a0)\n"
	"sw $t2,88($a0)\n"
	"COLL_FIXED_TRIANGL_TestPoint_Inner:\n"
	// Matrix rows are start and truncated delta; retain the untouched third row.
	"lw $v1,16($a0)\n"
	"lh $t6,28($a0)\n"
	"lh $t7,30($a0)\n"
	"lh $t8,32($a0)\n"
	"lh $t3,16($a0)\n"
	"lh $t4,18($a0)\n"
	"lh $t5,20($a0)\n"
	"ctc2 $v1,$0\n"
	"subu $t6,$t6,$t3\n"
	"subu $t7,$t7,$t4\n"
	"subu $t8,$t8,$t5\n"
	"sll $v1,$t6,16\n"
	"andi $t0,$t5,0xffff\n"
	"or $v1,$v1,$t0\n"
	"andi $t0,$t7,0xffff\n"
	"sll $t1,$t8,16\n"
	"or $t0,$t0,$t1\n"
	"ctc2 $v1,$1\n"
	"ctc2 $t0,$2\n"
	"lwc2 $0,84($a0)\n"
	"mtc2 $t2,$1\n"
	"sra $t2,$t2,16\n"
	"sll $t2,$t2,13\n"
	"cop2 0x0406012\n"
	"mfc2 $t0,$26\n"
	"mfc2 $v1,$25\n"
	"bgez $t0,.Lctp_return\n"
	"subu $v1,$v1,$t2\n"
	"sra $t0,$t0,12\n"
	"sub $v1,$zero,$v1\n"
	"div $zero,$v1,$t0\n"
	// These MAC/IR writes precede the factor-range rejection.
	"mtc2 $t3,$25\n"
	"mtc2 $t4,$26\n"
	"mtc2 $t5,$27\n"
	"mtc2 $t6,$9\n"
	"mtc2 $t7,$10\n"
	"mtc2 $t8,$11\n"
	"mflo $v1\n"
	"mtc2 $v1,$8\n"
	"bltz $v1,.Lctp_return\n"
	"addiu $t0,$v1,-4096\n"
	"bgtz $t0,.Lctp_return\n"
	"nop\n"
	"cop2 0x01a8003e\n"
	"mfc2 $t6,$25\n"
	"mfc2 $t7,$26\n"
	"mfc2 $t8,$27\n"
	"lh $v1,82($a0)\n"
	// Candidate coordinates are visible even when barycentrics later reject.
	"sh $t6,76($a0)\n"
	"sh $t7,78($a0)\n"
	"sh $t8,80($a0)\n"
	"addiu $t0,$zero,3\n"
	"bne $v1,$t0,.Lctp_axis_z\n"
	"addiu $t0,$zero,1\n"
	// Y normal projects to Z/X. Swap only when the second first-edge is larger.
	"lh $t3,4($a1)\n"
	"lh $t5,4($a2)\n"
	"lh $s1,4($a3)\n"
	"subu $t5,$t5,$t3\n"
	"subu $s1,$s1,$t3\n"
	"subu $t3,$t8,$t3\n"
	"bgez $t5,.Lctp_y_abs_a\n"
	"addu $v1,$t5,$zero\n"
	"sub $v1,$zero,$t5\n"
	".Lctp_y_abs_a:\n"
	"bgez $s1,.Lctp_y_abs_b\n"
	"addu $t0,$s1,$zero\n"
	"sub $t0,$zero,$s1\n"
	".Lctp_y_abs_b:\n"
	"subu $t1,$v1,$t0\n"
	"bgez $t1,.Lctp_y_ordered\n"
	"addu $v1,$t5,$zero\n"
	"addu $t5,$s1,$zero\n"
	"addu $s1,$v1,$zero\n"
	"addu $v1,$a2,$zero\n"
	"addu $a2,$a3,$zero\n"
	"addu $a3,$v1,$zero\n"
	".Lctp_y_ordered:\n"
	"lh $t4,0($a1)\n"
	"lh $s0,0($a2)\n"
	"lh $s2,0($a3)\n"
	"subu $s0,$s0,$t4\n"
	"subu $s2,$s2,$t4\n"
	"bgez $zero,.Lctp_barycentrics\n"
	"subu $t4,$t6,$t4\n"
	// Z normal projects to X/Y, with the same strict magnitude tie rule.
	".Lctp_axis_z:\n"
	"bne $v1,$t0,.Lctp_axis_x\n"
	"lh $t3,0($a1)\n"
	"lh $t5,0($a2)\n"
	"lh $s1,0($a3)\n"
	"subu $t5,$t5,$t3\n"
	"subu $s1,$s1,$t3\n"
	"subu $t3,$t6,$t3\n"
	"bgez $t5,.Lctp_z_abs_a\n"
	"addu $v1,$t5,$zero\n"
	"sub $v1,$zero,$t5\n"
	".Lctp_z_abs_a:\n"
	"bgez $s1,.Lctp_z_abs_b\n"
	"addu $t0,$s1,$zero\n"
	"sub $t0,$zero,$s1\n"
	".Lctp_z_abs_b:\n"
	"subu $t1,$v1,$t0\n"
	"bgez $t1,.Lctp_z_ordered\n"
	"addu $v1,$t5,$zero\n"
	"addu $t5,$s1,$zero\n"
	"addu $s1,$v1,$zero\n"
	"addu $v1,$a2,$zero\n"
	"addu $a2,$a3,$zero\n"
	"addu $a3,$v1,$zero\n"
	".Lctp_z_ordered:\n"
	"lh $t4,2($a1)\n"
	"lh $s0,2($a2)\n"
	"lh $s2,2($a3)\n"
	"subu $s0,$s0,$t4\n"
	"subu $s2,$s2,$t4\n"
	"bgez $zero,.Lctp_barycentrics\n"
	"subu $t4,$t7,$t4\n"
	// Other axes project to Y/Z; a2/a3 follow the retained vertex ordering.
	".Lctp_axis_x:\n"
	"lh $t3,2($a1)\n"
	"lh $t5,2($a2)\n"
	"lh $s1,2($a3)\n"
	"subu $t5,$t5,$t3\n"
	"subu $s1,$s1,$t3\n"
	"subu $t3,$t7,$t3\n"
	"bgez $t5,.Lctp_x_abs_a\n"
	"addu $v1,$t5,$zero\n"
	"sub $v1,$zero,$t5\n"
	".Lctp_x_abs_a:\n"
	"bgez $s1,.Lctp_x_abs_b\n"
	"addu $t0,$s1,$zero\n"
	"sub $t0,$zero,$s1\n"
	".Lctp_x_abs_b:\n"
	"subu $t1,$v1,$t0\n"
	"bgez $t1,.Lctp_x_ordered\n"
	"addu $v1,$t5,$zero\n"
	"addu $t5,$s1,$zero\n"
	"addu $s1,$v1,$zero\n"
	"addu $v1,$a2,$zero\n"
	"addu $a2,$a3,$zero\n"
	"addu $a3,$v1,$zero\n"
	".Lctp_x_ordered:\n"
	"lh $t4,4($a1)\n"
	"lh $s0,4($a2)\n"
	"lh $s2,4($a3)\n"
	"subu $s0,$s0,$t4\n"
	"subu $s2,$s2,$t4\n"
	"subu $t4,$t8,$t4\n"
	// Fixed-point determinants and raw DIVs retain their actual delay effects.
	".Lctp_barycentrics:\n"
	"addiu $t6,$zero,-4096\n"
	"addiu $t7,$zero,-4096\n"
	"beq $t5,$zero,.Lctp_fallback\n"
	"mult $s2,$t5\n"
	"mflo $v1\n"
	"nop\n"
	"nop\n"
	"mult $s1,$s0\n"
	"mflo $t0\n"
	"subu $v1,$v1,$t0\n"
	"sra $v1,$v1,6\n"
	"beq $v1,$zero,.Lctp_publish\n"
	"mult $t4,$t5\n"
	"mflo $t0\n"
	"nop\n"
	"nop\n"
	"mult $t3,$s0\n"
	"mflo $t1\n"
	"subu $t0,$t0,$t1\n"
	"sll $t0,$t0,6\n"
	"div $zero,$t0,$v1\n"
	"mflo $t7\n"
	"bltz $t7,.Lctp_publish\n"
	"addiu $t0,$t7,-4096\n"
	"bgtz $t0,.Lctp_publish\n"
	"mult $t7,$s1\n"
	"mflo $t0\n"
	"sll $v1,$t3,12\n"
	"subu $v1,$v1,$t0\n"
	"div $zero,$v1,$t5\n"
	"bgez $zero,.Lctp_publish\n"
	"mflo $t6\n"
	".Lctp_fallback:\n"
	"beq $s1,$zero,.Lctp_return\n"
	"sll $v1,$t3,12\n"
	"div $zero,$v1,$s1\n"
	"mflo $t7\n"
	"bltz $t7,.Lctp_publish\n"
	"addiu $t0,$t7,-4096\n"
	"bgtz $t0,.Lctp_publish\n"
	"mult $t7,$s2\n"
	"mflo $t0\n"
	"sll $v1,$t4,12\n"
	"subu $v1,$v1,$t0\n"
	"div $zero,$v1,$s0\n"
	"mflo $t6\n"
	".Lctp_publish:\n"
	"lw $t0,100($a0)\n"
	"bltz $t6,.Lctp_return\n"
	"addu $v1,$t6,$t7\n"
	"addiu $v1,$v1,-4096\n"
	"lh $t1,18($t0)\n"
	"bgtz $v1,.Lctp_return\n"
	"andi $t1,$t1,0x40\n"
	"bne $t1,$zero,.Lctp_trigger\n"
	"lh $t1,62($a0)\n"
	// Ordinary hits commit ordered pointers, halfword count and cached plane.
	"sw $t0,128($a0)\n"
	"sh $t6,200($a0)\n"
	"sh $t7,202($a0)\n"
	"lw $a1,8($a1)\n"
	"lw $a2,8($a2)\n"
	"lw $a3,8($a3)\n"
	"sw $a1,204($a0)\n"
	"sw $a2,208($a0)\n"
	"sw $a3,212($a0)\n"
	"addiu $t1,$t1,1\n"
	"sh $t1,62($a0)\n"
	"lw $v1,76($a0)\n"
	"lh $t0,80($a0)\n"
	"lw $t1,84($a0)\n"
	"lw $t2,88($a0)\n"
	"sw $v1,104($a0)\n"
	"sw $v1,28($a0)\n"
	"sh $t0,108($a0)\n"
	"sh $t0,32($a0)\n"
	"sw $t1,112($a0)\n"
	"sw $t2,116($a0)\n"
	".Lctp_return:\n"
	"jr $ra\n"
	"nop\n"
	// Trigger terrain bytes update the full step-flags word, not ordinary hits.
	".Lctp_trigger:\n"
	"lw $v1,420($a0)\n"
	"lbu $t0,56($t0)\n"
	"nop\n"
	"or $v1,$v1,$t0\n"
	"jr $ra\n"
	"sw $v1,420($a0)\n"
	".end COLL_FIXED_TRIANGL_TestPoint\n"
	".set\treorder\n"
	".text\n");
// clang-format on

#endif
