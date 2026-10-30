#include <common.h>

#ifdef CTR_NATIVE

// NOTE(aalhendi): Retain the maintained portable body at its COLL.c seam,
// after the existing CollFixed and CollMoved GTE helper definitions.
void COLL_MOVED_TRIANGL_TestPoint(struct ScratchpadStruct *sps, struct BspSearchVertex *v1, struct BspSearchVertex *v2, struct BspSearchVertex *v3)
{
	s32 usedSegmentProjection;

	sps->numTrianglesTested = (s16)CTR_MipsAddLo(sps->numTrianglesTested, 1);
	sps->candidate.normalAxis = v1->normalAxis;
	sps->candidate.plane = v1->plane;

	struct QuadBlock *quad = sps->candidate.ptrQuadblock;
	s32 normalZW = (s32)CTR_PackS16Pair(sps->candidate.plane.normal.z, sps->candidate.plane.halfDistance);

	if (((quad->quadFlags & QUADBLOCK_FLAG_DOOR) != 0) && (((s32)(s8)quad->terrain_type & sdata->doorAccessFlags) != 0))
	{
		return;
	}

	CollFixed_GteLoadR11R12(CTR_PackS16Pair(sps->Input1.pos.x, sps->Input1.pos.y));
	CollFixed_GteLoadR13R21(CTR_PackS16Pair(sps->Input1.pos.z, sps->Union.QuadBlockColl.pos.x));
	CollFixed_GteLoadR22R23(CTR_PackS16Pair(sps->Union.QuadBlockColl.pos.y, sps->Union.QuadBlockColl.pos.z));
	CollFixed_GteLoadVXY0(CTR_PackS16Pair(sps->candidate.plane.normal.x, sps->candidate.plane.normal.y));
	CollFixed_GteLoadVZ0(normalZW);
	CollMoved_GteRTV0();

	s32 halfDistance = sps->candidate.plane.halfDistance;
	s32 planeOffset = CTR_MipsSll(halfDistance, 1);
	s32 planeNear = CTR_MipsSubLo(CollFixed_GteReadMAC1(), planeOffset);
	s32 planeFar = CTR_MipsSubLo(CollFixed_GteReadMAC2(), planeOffset);

	if (planeFar < 0)
	{
		if (((quad->quadFlags & QUADBLOCK_FLAG_TRIGGER) == 0) && ((quad->draw_order_low & QUADBLOCK_DRAW_ORDER_LOW_DOUBLE_SIDED) == 0))
		{
			goto KeepNormal;
		}

		planeNear = CTR_MipsNegLo(planeNear);
		planeFar = CTR_MipsNegLo(planeFar);
		sps->candidate.plane.normal.x = (s16)CTR_MipsNegLo(sps->candidate.plane.normal.x);
		sps->candidate.plane.normal.y = (s16)CTR_MipsNegLo(sps->candidate.plane.normal.y);
		sps->candidate.plane.normal.z = (s16)CTR_MipsNegLo(sps->candidate.plane.normal.z);
		sps->candidate.plane.halfDistance = (s16)CTR_MipsNegLo(sps->candidate.plane.halfDistance);
	}

KeepNormal:;
	u16 quadFlags = quad->quadFlags;
	sps->numTrianglesTested = (s16)CTR_MipsAddLo(sps->numTrianglesTested, 1);

	if (CTR_MipsSubLo(planeNear, sps->Input1.hitRadius) >= 0)
	{
		return;
	}

	if (planeFar < 0)
	{
		return;
	}

	if (((quadFlags & QUADBLOCK_FLAG_TRIGGER) == 0) && (CTR_MipsSubLo(planeNear, planeFar) > 0))
	{
		return;
	}

	if (planeNear >= 0)
	{
		CollFixed_GteLoadIR0(planeNear);
		CollFixed_GteLoadIR(sps->candidate.plane.normal.x, sps->candidate.plane.normal.y, sps->candidate.plane.normal.z);
		usedSegmentProjection = 0;
	}
	else
	{
		CollFixed_GteLoadIR(CTR_MipsSubLo(sps->Input1.pos.x, sps->Union.QuadBlockColl.pos.x), CTR_MipsSubLo(sps->Input1.pos.y, sps->Union.QuadBlockColl.pos.y),
		                    CTR_MipsSubLo(sps->Input1.pos.z, sps->Union.QuadBlockColl.pos.z));
		CollFixed_GteLoadIR0(CTR_MipsDiv(CTR_MipsSll(CTR_MipsNegLo(planeNear), 12), CTR_MipsSubLo(planeFar, planeNear)));
		usedSegmentProjection = 1;
	}

	CollMoved_GteGPF12();
	s32 hitX = CollFixed_GteReadMAC1();
	s32 hitY = CollFixed_GteReadMAC2();
	s32 hitZ = CollFixed_GteReadMAC3();

	CTR_SET_VEC3(CTR_VECTOR_DATA(&(sps->candidate.pushOut)), (s16)CTR_MipsSubLo(sps->Input1.pos.x, hitX), (s16)CTR_MipsSubLo(sps->Input1.pos.y, hitY),
	             (s16)CTR_MipsSubLo(sps->Input1.pos.z, hitZ));

	sps->hitBspSearchTriangle.v0 = v1;
	sps->hitBspSearchTriangle.v1 = v2;
	sps->hitBspSearchTriangle.v2 = v3;

	s32 reorderResult = COLL_MOVED_TRIANGL_ReorderNormals(&sps->candidate, v1, v2, v3);
	if (reorderResult < 0)
	{
		return;
	}

	if (usedSegmentProjection != 0)
	{
		sps->candidateDelta.x = (s16)CTR_MipsSubLo(sps->candidate.pushOut.x, sps->candidate.hitPos.x);
		sps->candidateDelta.y = (s16)CTR_MipsSubLo(sps->candidate.pushOut.y, sps->candidate.hitPos.y);
		sps->candidateDelta.z = (s16)CTR_MipsSubLo(sps->candidate.pushOut.z, sps->candidate.hitPos.z);
	}
	else
	{
		sps->candidateDelta.x = (s16)CTR_MipsSubLo(sps->Input1.pos.x, sps->candidate.hitPos.x);
		sps->candidateDelta.y = (s16)CTR_MipsSubLo(sps->Input1.pos.y, sps->candidate.hitPos.y);
		sps->candidateDelta.z = (s16)CTR_MipsSubLo(sps->Input1.pos.z, sps->candidate.hitPos.z);
	}

	CollFixed_GteLoadR11R12(CTR_PackS16Pair(sps->candidateDelta.x, sps->candidateDelta.y));
	CollFixed_GteLoadR13R21(sps->candidateDelta.z);
	CollFixed_GteLoadVXY0(CTR_PackS16Pair(sps->candidateDelta.x, sps->candidateDelta.y));
	// NOTE(aalhendi): Retail's LWC2 retains the adjacent padding halfword too.
	CollFixed_GteLoadVZ0((s32)CTR_PackS16Pair(sps->candidateDelta.z, sps->padEA));
	CollFixed_GteMVMVA();
	s32 distanceSq = CollFixed_GteReadMAC1();

	if (CTR_MipsSubLo(distanceSq, sps->Input1.hitRadiusSquared) > 0)
	{
		return;
	}

	if ((quadFlags & QUADBLOCK_FLAG_TRIGGER) != 0)
	{
		if ((planeNear < 0) || ((CTR_MipsSubLo(planeNear, sps->Input1.hitRadius) | CTR_MipsSubLo(planeFar, sps->Input1.hitRadius)) < 0))
		{
			sps->collision.stepFlags |= (u8)quad->terrain_type;
			return;
		}
	}

	s32 distance = CTR_MipsSubLo(planeFar, planeNear);
	if (distance != 0)
	{
		distance = CTR_MipsSubLo(COLL_FRACTION_ONE, CTR_MipsDiv(CTR_MipsSll(CTR_MipsSubLo(sps->Input1.hitRadius, planeNear), 12), distance));
	}

	if (CTR_MipsSubLo(distance, sps->hitFraction) >= 0)
	{
		return;
	}

	if ((quadFlags & QUADBLOCK_FLAG_NO_COLLISION_RESPONSE) != 0)
	{
		if ((quadFlags & QUADBLOCK_FLAG_KILL_PLANE) != 0)
		{
			sps->collision.stepFlags |= COLL_STEP_FLAG_KILL_PLANE;
		}

		return;
	}

	sps->hitFraction = distance;
	sps->hitLevelTriangle.v0 = v1->pLevelVertex;
	sps->hitLevelTriangle.v1 = v2->pLevelVertex;
	sps->hitLevelTriangle.v2 = v3->pLevelVertex;

	sps->hit.hitPos = sps->candidate.hitPos;
	sps->hit.normalAxis = sps->candidate.normalAxis;
	sps->hit.plane = sps->candidate.plane;
	sps->hit.pushOut = sps->candidate.pushOut;
	sps->hit.ptrQuadblock = quad;

	sps->hit.triangleID = sps->candidate.triangleID;
	sps->hit.reorderResult = (CollTriangleClipResult)reorderResult;

	if (distance <= 0)
	{
		sps->Union.QuadBlockColl.hitPos = sps->Union.QuadBlockColl.pos;
	}
	else
	{
		CollFixed_GteLoadIR0(distance);
		CollFixed_GteLoadIR(CTR_MipsSubLo(sps->Input1.pos.x, sps->Union.QuadBlockColl.pos.x), CTR_MipsSubLo(sps->Input1.pos.y, sps->Union.QuadBlockColl.pos.y),
		                    CTR_MipsSubLo(sps->Input1.pos.z, sps->Union.QuadBlockColl.pos.z));
		CollMoved_GteGPF12();

		sps->Union.QuadBlockColl.hitPos.x = (s16)CTR_MipsAddLo(sps->Union.QuadBlockColl.pos.x, CollFixed_GteReadMAC1());
		sps->Union.QuadBlockColl.hitPos.y = (s16)CTR_MipsAddLo(sps->Union.QuadBlockColl.pos.y, CollFixed_GteReadMAC2());
		sps->Union.QuadBlockColl.hitPos.z = (s16)CTR_MipsAddLo(sps->Union.QuadBlockColl.pos.z, CollFixed_GteReadMAC3());
	}

	sps->boolDidTouchQuadblock = (s16)CTR_MipsAddLo(sps->boolDidTouchQuadblock, 1);
}

#else

// NOTE(aalhendi): This bounded retail entry deliberately clobbers s0-s6.
// s6 restores the scratch argument after ReorderNormals; a negative result
// returns before the positive-path t9 reload. Keep every load/GTE/divide
// delay and checked SUB explicit. The door owner is sdata_static+1980.
// clang-format off
__asm__(".section .COLL_MOVED_TRIANGL_TestPoint,\"ax\",@progbits\n"
	".align 2\n"
	".globl COLL_MOVED_TRIANGL_TestPoint\n"
	".ent\tCOLL_MOVED_TRIANGL_TestPoint\n"
	".set\tnoreorder\n"
	".set\tnoat\n"
	"COLL_MOVED_TRIANGL_TestPoint:\n"
	// Count and candidate plane writes precede the door rejection.
	"lh $v1,60($a0)\n"
	"lh $t0,6($a1)\n"
	"lw $t1,12($a1)\n"
	"lw $t2,16($a1)\n"
	"addiu $v1,$v1,1\n"
	"sh $v1,60($a0)\n"
	"sh $t0,82($a0)\n"
	"sw $t1,84($a0)\n"
	"sw $t2,88($a0)\n"
	"addiu $sp,$sp,-20\n"
	"sw $ra,16($sp)\n"
	"lw $t9,100($a0)\n"
	"lui $t1,%hi(sdata_static+1980)\n"
	"addiu $t1,$t1,%lo(sdata_static+1980)\n"
	"lh $v1,18($t9)\n"
	"lb $t0,56($t9)\n"
	"lw $t1,0($t1)\n"
	"andi $v1,$v1,0x400\n"
	"beq $v1,$zero,.Lcmpt_project_inputs\n"
	"and $v1,$t0,$t1\n"
	"bne $v1,$zero,.Lcmpt_return\n"
	// Ordered endpoint matrix transfer and SF=1 plane projection.
	".Lcmpt_project_inputs:\n"
	"lw $v1,0($a0)\n"
	"lhu $t0,4($a0)\n"
	"lw $t1,16($a0)\n"
	"lhu $t2,20($a0)\n"
	"ctc2 $v1,$0\n"
	"sll $v1,$t1,16\n"
	"or $v1,$v1,$t0\n"
	"ctc2 $v1,$1\n"
	"srl $v1,$t1,16\n"
	"sll $t0,$t2,16\n"
	"or $v1,$v1,$t0\n"
	"ctc2 $v1,$2\n"
	"lwc2 $0,84($a0)\n"
	"lwc2 $1,88($a0)\n"
	"lh $t2,90($a0)\n"
	"lh $t1,88($a0)\n"
	"cop2 0x0486012\n"
	"sll $t2,$t2,1\n"
	"mfc2 $s3,$25\n"
	"mfc2 $s4,$26\n"
	"subu $s3,$s3,$t2\n"
	"subu $s4,$s4,$t2\n"
	"lh $t2,18($t9)\n"
	"bgez $s4,.Lcmpt_keep_normal\n"
	"andi $t2,$t2,0x40\n"
	"lw $v1,20($t9)\n"
	"bne $t2,$zero,.Lcmpt_reverse\n"
	"lh $t0,86($a0)\n"
	"bgez $v1,.Lcmpt_keep_normal\n"
	".Lcmpt_reverse:\n"
	"lh $v1,84($a0)\n"
	"lh $t2,90($a0)\n"
	"sub $s3,$zero,$s3\n"
	"sub $s4,$zero,$s4\n"
	"sub $v1,$zero,$v1\n"
	"sub $t0,$zero,$t0\n"
	"sub $t1,$zero,$t1\n"
	"sub $t2,$zero,$t2\n"
	"sh $v1,84($a0)\n"
	"sh $t0,86($a0)\n"
	"sh $t1,88($a0)\n"
	"sh $t2,90($a0)\n"
	// Second count increment remains in the first rejection branch slot.
	".Lcmpt_keep_normal:\n"
	"lh $v1,60($a0)\n"
	"lh $t0,6($a0)\n"
	"addiu $v1,$v1,1\n"
	"subu $t0,$s3,$t0\n"
	"lh $t2,18($t9)\n"
	"bgez $t0,.Lcmpt_return\n"
	"sh $v1,60($a0)\n"
	"bltz $s4,.Lcmpt_return\n"
	"andi $t2,$t2,0x40\n"
	"subu $t0,$s3,$s4\n"
	"bne $t2,$zero,.Lcmpt_projection\n"
	"sub $v1,$zero,$s3\n"
	"bgtz $t0,.Lcmpt_return\n"
	".Lcmpt_projection:\n"
	"sll $v1,$v1,12\n"
	"bgez $s3,.Lcmpt_normal_projection\n"
	"subu $t0,$s4,$s3\n"
	// Raw DIV: do not request a destination-form ASPSX expansion.
	"div $zero,$v1,$t0\n"
	"lh $v1,0($a0)\n"
	"lh $t0,16($a0)\n"
	"lh $t1,2($a0)\n"
	"lh $t2,18($a0)\n"
	"subu $v1,$v1,$t0\n"
	"mtc2 $v1,$9\n"
	"lh $v1,4($a0)\n"
	"lh $t0,20($a0)\n"
	"subu $t1,$t1,$t2\n"
	"mtc2 $t1,$10\n"
	"subu $v1,$v1,$t0\n"
	"mtc2 $v1,$11\n"
	"mflo $v1\n"
	"mtc2 $v1,$8\n"
	"bgez $zero,.Lcmpt_project_gte\n"
	"addiu $s5,$zero,1\n"
	".Lcmpt_normal_projection:\n"
	"lh $v1,84($a0)\n"
	"lh $t0,86($a0)\n"
	"lh $t1,88($a0)\n"
	"mtc2 $s3,$8\n"
	"mtc2 $v1,$9\n"
	"mtc2 $t0,$10\n"
	"mtc2 $t1,$11\n"
	"addiu $s5,$zero,0\n"
	"nop\n"
	".Lcmpt_project_gte:\n"
	"cop2 0x0198003d\n"
	"lh $v1,0($a0)\n"
	"lh $t0,2($a0)\n"
	"mfc2 $t2,$25\n"
	"lh $t1,4($a0)\n"
	"subu $v1,$v1,$t2\n"
	"mfc2 $t2,$26\n"
	"sh $v1,92($a0)\n"
	"subu $t0,$t0,$t2\n"
	"mfc2 $t2,$27\n"
	"sh $t0,94($a0)\n"
	"subu $t1,$t1,$t2\n"
	"sh $t1,96($a0)\n"
	"addu $s6,$a0,$zero\n"
	"sw $a1,216($a0)\n"
	"sw $a2,220($a0)\n"
	"sw $a3,224($a0)\n"
	"jal COLL_MOVED_TRIANGL_ReorderNormals\n"
	"addiu $a0,$a0,76\n"
	"addu $a0,$s6,$zero\n"
	"lw $a1,216($a0)\n"
	"lw $a2,220($a0)\n"
	"bltz $v0,.Lcmpt_return\n"
	"lw $a3,224($a0)\n"
	"lh $t0,76($a0)\n"
	"beq $s5,$zero,.Lcmpt_input_delta\n"
	"lh $t2,78($a0)\n"
	"lh $v1,92($a0)\n"
	"lh $t1,94($a0)\n"
	"subu $v1,$v1,$t0\n"
	"sh $v1,228($a0)\n"
	"lh $v1,96($a0)\n"
	"lh $t0,80($a0)\n"
	"subu $t1,$t1,$t2\n"
	"sh $t1,230($a0)\n"
	"subu $v1,$v1,$t0\n"
	"bgez $zero,.Lcmpt_distance\n"
	"sh $v1,232($a0)\n"
	".Lcmpt_input_delta:\n"
	"lh $v1,0($a0)\n"
	"lh $t1,2($a0)\n"
	"subu $v1,$v1,$t0\n"
	"sh $v1,228($a0)\n"
	"lh $v1,4($a0)\n"
	"lh $t0,80($a0)\n"
	"subu $t1,$t1,$t2\n"
	"sh $t1,230($a0)\n"
	"subu $v1,$v1,$t0\n"
	"sh $v1,232($a0)\n"
	".Lcmpt_distance:\n"
	"lw $v1,228($a0)\n"
	"lh $t0,232($a0)\n"
	"ctc2 $v1,$0\n"
	"ctc2 $t0,$1\n"
	"lwc2 $0,228($a0)\n"
	"lwc2 $1,232($a0)\n"
	"nop\n"
	"nop\n"
	"cop2 0x0406012\n"
	"lw $t0,8($a0)\n"
	"mfc2 $v1,$25\n"
	"lw $t9,100($a0)\n"
	"subu $t0,$v1,$t0\n"
	"lh $s5,18($t9)\n"
	"bgtz $t0,.Lcmpt_return\n"
	"andi $t1,$s5,0x40\n"
	"beq $t1,$zero,.Lcmpt_fraction\n"
	"lh $t1,6($a0)\n"
	"bltz $s3,.Lcmpt_trigger\n"
	"subu $v1,$s3,$t1\n"
	"subu $t0,$s4,$t1\n"
	"or $v1,$v1,$t0\n"
	"bgez $v1,.Lcmpt_fraction\n"
	".Lcmpt_trigger:\n"
	"lw $v1,420($a0)\n"
	"lbu $t1,56($t9)\n"
	"nop\n"
	"or $v1,$v1,$t1\n"
	"bgez $zero,.Lcmpt_return\n"
	"sw $v1,420($a0)\n"
	".Lcmpt_fraction:\n"
	"lh $t1,6($a0)\n"
	"lw $t2,132($a0)\n"
	"subu $v1,$s4,$s3\n"
	"beq $v1,$zero,.Lcmpt_earliest\n"
	"subu $t1,$t1,$s3\n"
	"sll $t1,$t1,12\n"
	"div $zero,$t1,$v1\n"
	"addiu $t0,$zero,4096\n"
	"mflo $v1\n"
	"subu $v1,$t0,$v1\n"
	".Lcmpt_earliest:\n"
	"subu $t0,$v1,$t2\n"
	"bgez $t0,.Lcmpt_return\n"
	"andi $t0,$s5,0x10\n"
	"beq $t0,$zero,.Lcmpt_accept\n"
	"andi $t0,$s5,0x200\n"
	"beq $t0,$zero,.Lcmpt_return\n"
	"lw $t0,420($a0)\n"
	"nop\n"
	"ori $t0,$t0,0x4000\n"
	"bgez $zero,.Lcmpt_return\n"
	"sw $t0,420($a0)\n"
	// Accepted hit copies, including the current retail dispatcher ID.
	".Lcmpt_accept:\n"
	"sw $v1,132($a0)\n"
	"lw $a1,8($a1)\n"
	"lw $a2,8($a2)\n"
	"lw $a3,8($a3)\n"
	"sw $a1,204($a0)\n"
	"sw $a2,208($a0)\n"
	"sw $a3,212($a0)\n"
	"lw $t0,76($a0)\n"
	"lw $t1,80($a0)\n"
	"sw $t0,104($a0)\n"
	"sw $t1,108($a0)\n"
	"lw $t0,84($a0)\n"
	"lw $t1,88($a0)\n"
	"sw $t0,112($a0)\n"
	"sw $t1,116($a0)\n"
	"lw $t0,92($a0)\n"
	"lw $t1,96($a0)\n"
	"sw $t0,120($a0)\n"
	"sw $t1,124($a0)\n"
	"lb $t0,99($a0)\n"
	"sw $t9,128($a0)\n"
	"sb $t0,127($a0)\n"
	"sb $v0,126($a0)\n"
	"bgtz $v1,.Lcmpt_segment_hit\n"
	"lw $t0,16($a0)\n"
	"lh $t1,20($a0)\n"
	"sw $t0,28($a0)\n"
	"bgez $zero,.Lcmpt_touch\n"
	"sh $t1,32($a0)\n"
	".Lcmpt_segment_hit:\n"
	"mtc2 $v1,$8\n"
	"lh $v1,0($a0)\n"
	"lh $t0,16($a0)\n"
	"lh $t1,2($a0)\n"
	"lh $t2,18($a0)\n"
	"subu $v1,$v1,$t0\n"
	"mtc2 $v1,$9\n"
	"lh $v1,4($a0)\n"
	"lh $t0,20($a0)\n"
	"subu $t1,$t1,$t2\n"
	"mtc2 $t1,$10\n"
	"subu $v1,$v1,$t0\n"
	"mtc2 $v1,$11\n"
	"lh $v1,16($a0)\n"
	"lh $t0,18($a0)\n"
	"cop2 0x0198003d\n"
	"mfc2 $t2,$25\n"
	"lh $t1,20($a0)\n"
	"addu $v1,$v1,$t2\n"
	"mfc2 $t2,$26\n"
	"sh $v1,28($a0)\n"
	"addu $t0,$t0,$t2\n"
	"mfc2 $t2,$27\n"
	"sh $t0,30($a0)\n"
	"addu $t1,$t1,$t2\n"
	"sh $t1,32($a0)\n"
	".Lcmpt_touch:\n"
	"lh $t0,62($a0)\n"
	"nop\n"
	"addiu $t0,$t0,1\n"
	"sh $t0,62($a0)\n"
	".Lcmpt_return:\n"
	"lw $ra,16($sp)\n"
	"nop\n"
	"jr $ra\n"
	"addiu $sp,$sp,20\n"
	".end COLL_MOVED_TRIANGL_TestPoint\n"
	".set\tat\n"
	".set\treorder\n"
	".text\n");
// clang-format on

#endif
