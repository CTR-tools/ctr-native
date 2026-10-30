#include <common.h>

#ifdef CTR_NATIVE

// The COLL unity supplies CollTriangleProjection and CollMoved_SelectProjection
// before this include; retain that helper and the maintained call chronology.
s32 COLL_MOVED_TRIANGL_ReorderNormals(struct BspSearchResult *candidate, struct BspSearchVertex *v1, struct BspSearchVertex *v2, struct BspSearchVertex *v3)
{
	struct BspSearchVertex *baryV2 = v2;
	struct BspSearchVertex *baryV3 = v3;
	struct CollTriangleProjection projection;
	s32 baryA = COLL_FRACTION_INVALID;
	s32 baryB = COLL_FRACTION_INVALID;

	CollMoved_SelectProjection(candidate->normalAxis, candidate, v1, &baryV2, &baryV3, &projection);

	if (projection.firstA == 0)
	{
		if (projection.firstB == 0)
		{
			return COLL_TRIANGLE_CLIP_MISS;
		}

		baryB = CTR_MipsDiv(CTR_MipsSll(projection.firstHit, 12), projection.firstB);

		if (projection.secondA != 0)
		{
			baryA = CTR_MipsDiv(CTR_MipsSubLo(CTR_MipsSll(projection.secondHit, 12), CTR_MipsMulLo(baryB, projection.secondB)), projection.secondA);
		}
	}
	else
	{
		s32 denom = CTR_MipsSra(CTR_MipsSubLo(CTR_MipsMulLo(projection.secondB, projection.firstA), CTR_MipsMulLo(projection.firstB, projection.secondA)), 6);

		if (denom != 0)
		{
			baryB = CTR_MipsDiv(
			    CTR_MipsSll(CTR_MipsSubLo(CTR_MipsMulLo(projection.secondHit, projection.firstA), CTR_MipsMulLo(projection.firstHit, projection.secondA)), 6),
			    denom);
			baryA = CTR_MipsDiv(CTR_MipsSubLo(CTR_MipsSll(projection.firstHit, 12), CTR_MipsMulLo(baryB, projection.firstB)), projection.firstA);
		}
	}

	if (baryA == COLL_FRACTION_INVALID)
	{
		return COLL_TRIANGLE_CLIP_MISS;
	}

	s32 sum = CTR_MipsSubLo(CTR_MipsAddLo(baryA, baryB), COLL_FRACTION_ONE);

	if (baryA < 0)
	{
		if (baryB < 0)
		{
			candidate->hitPos = v1->pos;
			return COLL_TRIANGLE_CLIP_V1;
		}

		if (sum >= 0)
		{
			candidate->hitPos = baryV3->pos;
			return COLL_TRIANGLE_CLIP_V3;
		}

		COLL_FIXED_TRIANGL_Barycentrics(&candidate->hitPos, &v1->pos, &baryV3->pos, &candidate->pushOut);
		return COLL_TRIANGLE_CLIP_EDGE_V1_V3;
	}

	if (baryB >= 0)
	{
		if (sum <= 0)
		{
			candidate->hitPos = candidate->pushOut;
			return COLL_TRIANGLE_CLIP_FACE;
		}

		COLL_FIXED_TRIANGL_Barycentrics(&candidate->hitPos, &baryV2->pos, &baryV3->pos, &candidate->pushOut);
		return COLL_TRIANGLE_CLIP_EDGE_V2_V3;
	}

	if (sum >= 0)
	{
		candidate->hitPos = baryV2->pos;
		return COLL_TRIANGLE_CLIP_V2;
	}

	COLL_FIXED_TRIANGL_Barycentrics(&candidate->hitPos, &v1->pos, &baryV2->pos, &candidate->pushOut);
	return COLL_TRIANGLE_CLIP_EDGE_V1_V2;
}

#else

// NOTE(aalhendi): This inner entry clobbers s0-s2 while preserving s3-s6.
// Only the three barycentric paths allocate a 20-byte frame. The direct miss
// path leaves t9 intact; Barycentrics clobbers it on positive-result paths,
// where MOVED TestPoint reloads the quad cursor. Preserve the checked SUBs,
// raw DIV/MFLO, HI/LO spacing, and repeated baryA sentinel test explicitly.
// Native retains the maintained projection helper and defined wrap/divide body.
// clang-format off
__asm__(".section .COLL_MOVED_TRIANGL_ReorderNormals,\"ax\",@progbits\n"
	".align 2\n"
	".globl COLL_MOVED_TRIANGL_ReorderNormals\n"
	".ent\tCOLL_MOVED_TRIANGL_ReorderNormals\n"
	".set\tnoreorder\n"
	"COLL_MOVED_TRIANGL_ReorderNormals:\n"
	// Y projection: first Z, then X; order vertices by the first edge magnitude.
	"lh $v1,6($a0)\n"
	"lh $t6,16($a0)\n"
	"lh $t7,18($a0)\n"
	"lh $t8,20($a0)\n"
	"addiu $t0,$zero,3\n"
	"bne $v1,$t0,.Lcmrn_project_z\n"
	"addiu $t0,$zero,1\n"
	"lh $t3,4($a1)\n"
	"lh $t5,4($a2)\n"
	"lh $s1,4($a3)\n"
	"subu $t5,$t5,$t3\n"
	"subu $s1,$s1,$t3\n"
	"subu $t3,$t8,$t3\n"
	"bgez $t5,.Lcmrn_y_abs_b\n"
	"addu $v1,$t5,$zero\n"
	"sub $v1,$zero,$t5\n"
	".Lcmrn_y_abs_b:\n"
	"bgez $s1,.Lcmrn_y_compare\n"
	"addu $t0,$s1,$zero\n"
	"sub $t0,$zero,$s1\n"
	".Lcmrn_y_compare:\n"
	"subu $t1,$v1,$t0\n"
	"bgez $t1,.Lcmrn_y_second\n"
	"addu $v1,$t5,$zero\n"
	"addu $t5,$s1,$zero\n"
	"addu $s1,$v1,$zero\n"
	"addu $v1,$a2,$zero\n"
	"addu $a2,$a3,$zero\n"
	"addu $a3,$v1,$zero\n"
	".Lcmrn_y_second:\n"
	"lh $t4,0($a1)\n"
	"lh $s0,0($a2)\n"
	"lh $s2,0($a3)\n"
	"subu $s0,$s0,$t4\n"
	"subu $s2,$s2,$t4\n"
	"bgez $zero,.Lcmrn_solve\n"
	"subu $t4,$t6,$t4\n"
	// Z projection: first X, then Y.
	".Lcmrn_project_z:\n"
	"bne $v1,$t0,.Lcmrn_project_x\n"
	"lh $t3,0($a1)\n"
	"lh $t5,0($a2)\n"
	"lh $s1,0($a3)\n"
	"subu $t5,$t5,$t3\n"
	"subu $s1,$s1,$t3\n"
	"subu $t3,$t6,$t3\n"
	"bgez $t5,.Lcmrn_z_abs_b\n"
	"addu $v1,$t5,$zero\n"
	"sub $v1,$zero,$t5\n"
	".Lcmrn_z_abs_b:\n"
	"bgez $s1,.Lcmrn_z_compare\n"
	"addu $t0,$s1,$zero\n"
	"sub $t0,$zero,$s1\n"
	".Lcmrn_z_compare:\n"
	"subu $t1,$v1,$t0\n"
	"bgez $t1,.Lcmrn_z_second\n"
	"addu $v1,$t5,$zero\n"
	"addu $t5,$s1,$zero\n"
	"addu $s1,$v1,$zero\n"
	"addu $v1,$a2,$zero\n"
	"addu $a2,$a3,$zero\n"
	"addu $a3,$v1,$zero\n"
	".Lcmrn_z_second:\n"
	"lh $t4,2($a1)\n"
	"lh $s0,2($a2)\n"
	"lh $s2,2($a3)\n"
	"subu $s0,$s0,$t4\n"
	"subu $s2,$s2,$t4\n"
	"bgez $zero,.Lcmrn_solve\n"
	"subu $t4,$t7,$t4\n"
	// X/default projection: first Y, then Z.
	".Lcmrn_project_x:\n"
	"lh $t3,2($a1)\n"
	"lh $t5,2($a2)\n"
	"lh $s1,2($a3)\n"
	"subu $t5,$t5,$t3\n"
	"subu $s1,$s1,$t3\n"
	"subu $t3,$t7,$t3\n"
	"bgez $t5,.Lcmrn_x_abs_b\n"
	"addu $v1,$t5,$zero\n"
	"sub $v1,$zero,$t5\n"
	".Lcmrn_x_abs_b:\n"
	"bgez $s1,.Lcmrn_x_compare\n"
	"addu $t0,$s1,$zero\n"
	"sub $t0,$zero,$s1\n"
	".Lcmrn_x_compare:\n"
	"subu $t1,$v1,$t0\n"
	"bgez $t1,.Lcmrn_x_second\n"
	"addu $v1,$t5,$zero\n"
	"addu $t5,$s1,$zero\n"
	"addu $s1,$v1,$zero\n"
	"addu $v1,$a2,$zero\n"
	"addu $a2,$a3,$zero\n"
	"addu $a3,$v1,$zero\n"
	".Lcmrn_x_second:\n"
	"lh $t4,4($a1)\n"
	"lh $s0,4($a2)\n"
	"lh $s2,4($a3)\n"
	"subu $s0,$s0,$t4\n"
	"subu $s2,$s2,$t4\n"
	"subu $t4,$t8,$t4\n"
	// Solve with signed raw divides; retain every measured HI/LO spacing slot.
	".Lcmrn_solve:\n"
	"addiu $t6,$zero,-4096\n"
	"addiu $t7,$zero,-4096\n"
	"beq $t5,$zero,.Lcmrn_first_zero\n"
	"mult $s2,$t5\n"
	"mflo $v1\n"
	"nop\n"
	"nop\n"
	"mult $s1,$s0\n"
	"mflo $t0\n"
	"subu $v1,$v1,$t0\n"
	"sra $v1,$v1,6\n"
	"beq $v1,$zero,.Lcmrn_classify\n"
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
	"beq $t5,$zero,.Lcmrn_classify\n"
	"nop\n"
	"mult $t7,$s1\n"
	"mflo $t0\n"
	"sll $v1,$t3,12\n"
	"subu $v1,$v1,$t0\n"
	"div $zero,$v1,$t5\n"
	"bgez $zero,.Lcmrn_classify\n"
	"mflo $t6\n"
	".Lcmrn_first_zero:\n"
	"beq $s1,$zero,.Lcmrn_miss\n"
	"sll $v1,$t3,12\n"
	"div $zero,$v1,$s1\n"
	"mflo $t7\n"
	"beq $s0,$zero,.Lcmrn_classify\n"
	"nop\n"
	"mult $t7,$s2\n"
	"mflo $t0\n"
	"sll $v1,$t4,12\n"
	"subu $v1,$v1,$t0\n"
	"div $zero,$v1,$s0\n"
	"mflo $t6\n"
	// The second branch deliberately tests v1 again, not the computed t0.
	".Lcmrn_classify:\n"
	"addiu $v1,$t6,4096\n"
	"beq $v1,$zero,.Lcmrn_miss\n"
	"addiu $t0,$t7,4096\n"
	"beq $v1,$zero,.Lcmrn_miss\n"
	"addu $t2,$t6,$t7\n"
	"bgez $t6,.Lcmrn_nonnegative_a\n"
	"nop\n"
	"bltz $t7,.Lcmrn_vertex1\n"
	"addiu $t2,$t2,-4096\n"
	"bltz $t2,.Lcmrn_edge13\n"
	"addiu $v0,$zero,4\n"
	"lw $v1,0($a3)\n"
	"lh $t0,4($a3)\n"
	"sw $v1,0($a0)\n"
	"jr $ra\n"
	"sh $t0,4($a0)\n"
	".Lcmrn_vertex1:\n"
	"lw $v1,0($a1)\n"
	"lh $t0,4($a1)\n"
	"sw $v1,0($a0)\n"
	"sh $t0,4($a0)\n"
	"jr $ra\n"
	"addiu $v0,$zero,0\n"
	// Edge 1/3: only this path allocates its own 20-byte frame.
	".Lcmrn_edge13:\n"
	"addiu $sp,$sp,-20\n"
	"sw $ra,16($sp)\n"
	"addu $a2,$a3,$zero\n"
	"jal COLL_FIXED_TRIANGL_Barycentrics\n"
	"addiu $a3,$a0,16\n"
	"lw $ra,16($sp)\n"
	"addiu $sp,$sp,20\n"
	"jr $ra\n"
	"addiu $v0,$zero,5\n"
	".Lcmrn_nonnegative_a:\n"
	"bgez $t7,.Lcmrn_nonnegative_b\n"
	"addiu $t2,$t2,-4096\n"
	"bltz $t2,.Lcmrn_edge12\n"
	"addiu $v0,$zero,2\n"
	"lw $v1,0($a2)\n"
	"lh $t0,4($a2)\n"
	"sw $v1,0($a0)\n"
	"jr $ra\n"
	"sh $t0,4($a0)\n"
	".Lcmrn_edge12:\n"
	"addiu $sp,$sp,-20\n"
	"sw $ra,16($sp)\n"
	"jal COLL_FIXED_TRIANGL_Barycentrics\n"
	"addiu $a3,$a0,16\n"
	"lw $ra,16($sp)\n"
	"addiu $sp,$sp,20\n"
	"jr $ra\n"
	"addiu $v0,$zero,1\n"
	".Lcmrn_nonnegative_b:\n"
	"bgtz $t2,.Lcmrn_edge23\n"
	"addiu $v0,$zero,6\n"
	"lw $v1,16($a0)\n"
	"lh $t0,20($a0)\n"
	"sw $v1,0($a0)\n"
	"jr $ra\n"
	"sh $t0,4($a0)\n"
	".Lcmrn_edge23:\n"
	"addiu $sp,$sp,-20\n"
	"sw $ra,16($sp)\n"
	"addu $a1,$a2,$zero\n"
	"addu $a2,$a3,$zero\n"
	"jal COLL_FIXED_TRIANGL_Barycentrics\n"
	"addiu $a3,$a0,16\n"
	"lw $ra,16($sp)\n"
	"addiu $sp,$sp,20\n"
	"jr $ra\n"
	"addiu $v0,$zero,3\n"
	".Lcmrn_miss:\n"
	"jr $ra\n"
	"addiu $v0,$zero,-1\n"
	".end COLL_MOVED_TRIANGL_ReorderNormals\n"
	".set\treorder\n"
	".text\n");
// clang-format on

#endif
