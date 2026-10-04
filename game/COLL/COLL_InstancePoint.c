#include <common.h>

u32 COLL_FIXED_INSTANC_TestPoint(struct ScratchpadStruct *sps, struct BSP *node)
{
	s32 diffX;
	s32 diffY;
	s32 diffZ;
	s32 centerDiffX;
	s32 centerDiffY;
	s32 centerDiffZ;
	register s32 dotSegment CTR_PSX_REGISTER("$10");
	register s32 dotCenter CTR_PSX_REGISTER("$3");
	register s32 shift CTR_PSX_REGISTER("$7");
	register s32 divisor CTR_PSX_REGISTER("$8");
	s32 factor;
	s32 radius;
	register s32 lateRadius CTR_PSX_REGISTER("$6");
	register s32 radiusSquared CTR_PSX_REGISTER("$6");
	register s32 radiusBase CTR_PSX_REGISTER("$3");
	register s32 remaining CTR_PSX_REGISTER("$6");
	register s32 hitX CTR_PSX_REGISTER("$22");
	register s32 hitY CTR_PSX_REGISTER("$20");
	register s32 hitZ CTR_PSX_REGISTER("$23");
	s32 centerY;
	s32 normalX;
	s32 normalY;
	s32 normalZ;
	u32 acceptedCount;
	s32 len;
	register s32 rawScaledX CTR_PSX_REGISTER("$4");
	register s32 rawScaledY CTR_PSX_REGISTER("$5");
	register s32 rawScaledZ CTR_PSX_REGISTER("$6");
	register s32 scaledY CTR_PSX_REGISTER("$4");
	register s32 scaledZ CTR_PSX_REGISTER("$5");
	u32 originX;
	u32 originY;
	register u32 retailResult CTR_PSX_REGISTER("$2") = 0;
	u32 nativeResult = 0;

	// NOTE(aalhendi): The hitbox class is flag byte 1 in the retail little-endian layout.
	if (((const u8 *)&node->flag)[1] == BSP_HITBOX_CLASS_TOUCH)
	{
		sps->bspHitbox = node;
		sps->boolDidTouchHitbox = (s16)CTR_MipsAddLo((u16)sps->boolDidTouchHitbox, 1);
	}

	if ((sps->Union.QuadBlockColl.searchFlags & COLL_SEARCH_FORCE_INSTANCE_HIT) != 0)
	{
		register u32 forceX CTR_PSX_REGISTER("$4");
		register u32 forceY CTR_PSX_REGISTER("$5");
		register u32 forceZ CTR_PSX_REGISTER("$6");
		register u32 forceCount CTR_PSX_REGISTER("$3");
		register u32 forceValue CTR_PSX_REGISTER("$2");

		forceX = (u16)sps->Union.QuadBlockColl.pos.x;
		forceY = (u16)sps->Union.QuadBlockColl.pos.y;
		forceZ = (u16)sps->Union.QuadBlockColl.pos.z;
		forceCount = (u16)sps->boolDidTouchHitbox;
		forceValue = COLL_FRACTION_ONE;
		CTR_PSX_DEPEND_VALUE(forceX, forceY);
		CTR_PSX_DEPEND_VALUE(forceZ, forceCount);
		CTR_PSX_DEPEND_VALUE(forceValue, forceZ);
		sps->hit.plane.normal.y = (s16)forceValue;
		sps->hit.plane.normal.x = 0;
		sps->hit.plane.normal.z = 0;
		forceValue = COLL_TRIANGLE_CLIP_FACE;
		sps->hit.reorderResult = (u8)forceValue;
		CTR_PSX_MEMORY_BARRIER();
		sps->hitFraction = 0;
		sps->bspHitbox = node;
		CTR_PSX_MEMORY_BARRIER();
		CTR_PSX_KEEP_VALUE(forceCount);
		sps->Union.QuadBlockColl.hitPos.x = (s16)forceX;
		sps->Union.QuadBlockColl.hitPos.y = (s16)forceY;
		sps->Union.QuadBlockColl.hitPos.z = (s16)forceZ;
		sps->boolDidTouchHitbox = (s16)CTR_MipsAddLo(forceCount, 1);
		retailResult = forceValue;
		nativeResult = COLL_TRIANGLE_CLIP_FACE;
		goto returnPolicy;
	}

	sps->collision.instanceHitbox.segmentDelta.y = 0;

	diffX = sps->Input1.pos.x - sps->Union.QuadBlockColl.pos.x;
	diffZ = sps->Input1.pos.z - sps->Union.QuadBlockColl.pos.z;
	sps->collision.instanceHitbox.segmentDelta.x = diffX;
	sps->collision.instanceHitbox.segmentDelta.z = diffZ;

	CTR_PSX_DEPEND_VALUE(diffX, diffZ);
	centerDiffX = node->data.hitbox.center.x - sps->Union.QuadBlockColl.pos.x;
	sps->collision.instanceHitbox.centerDelta.y = 0;
	sps->collision.instanceHitbox.centerDelta.x = centerDiffX;
	CTR_PSX_MEMORY_BARRIER();
	centerDiffZ = node->data.hitbox.center.z - sps->Union.QuadBlockColl.pos.z;
	sps->collision.instanceHitbox.centerDelta.z = centerDiffZ;
	diffY = 0;
	centerDiffY = diffY;

	if ((*(const u8 *)&node->flag & BSP_HITBOX_USE_Y_AXIS) != 0)
	{
		diffY = sps->Input1.pos.y - sps->Union.QuadBlockColl.pos.y;
		sps->collision.instanceHitbox.segmentDelta.y = diffY;
		centerDiffY = node->data.hitbox.center.y - sps->Union.QuadBlockColl.pos.y;
		sps->collision.instanceHitbox.centerDelta.y = centerDiffY;
	}

	CTR_PSX_DEPEND_VALUE(diffX, centerDiffY);
	{
		register u32 packedSegment CTR_PSX_REGISTER("$7");
		register u32 packedSegmentCenter CTR_PSX_REGISTER("$8");
		register u32 packedCenter CTR_PSX_REGISTER("$6");
		register u32 packedLow CTR_PSX_REGISTER("$3");

		packedLow = (u32)diffX & 0xffff;
		CTR_PSX_KEEP_VALUE(packedLow);
		retailResult = (u32)CTR_MipsSll(diffY, 16);
		CTR_PSX_KEEP_VALUE(retailResult);
		packedSegment = packedLow | retailResult;
		packedLow = (u32)diffZ & 0xffff;
		CTR_PSX_KEEP_VALUE(packedLow);
		retailResult = (u32)CTR_MipsSll(centerDiffX, 16);
		CTR_PSX_KEEP_VALUE(retailResult);
		packedSegmentCenter = packedLow | retailResult;
		CTR_PSX_DEPEND_VALUE(packedSegmentCenter, packedSegment);
		packedLow = (u32)centerDiffY & 0xffff;
		CTR_PSX_KEEP_VALUE(packedLow);
		retailResult = (u32)CTR_MipsSll(centerDiffZ, 16);
		packedCenter = packedLow | retailResult;
		CTC2(packedSegment, 0);
		CTC2(packedSegmentCenter, 1);
		CTC2(packedCenter, 2);
		MTC2(packedSegment, 0);
		MTC2_S(diffZ, 1);
		CTR_PSX_GTE_PIPELINE_DELAY();
		doCOP2(0x0406012);
	}

	dotSegment = MFC2_S(25);
	CTR_PSX_KEEP_VALUE(dotSegment);
	dotCenter = MFC2_S(26);
	CTR_PSX_KEEP_VALUE(dotCenter);
	sps->collision.instanceHitbox.segmentDot = dotSegment;
	sps->collision.instanceHitbox.centerDot = dotCenter;

	if (dotCenter <= 0)
	{
		goto returnPolicy;
	}

	MTC2_S(dotCenter, 30);
	CTR_PSX_GTE_PIPELINE_DELAY();
	shift = MFC2_S(31);
	CTR_PSX_GTE_READ_DELAY();
	CTR_PSX_DEPEND_VALUE(shift, dotCenter);
	shift = CTR_MipsSubLo(shift, 2);
	if (shift < 0)
	{
		shift = 0;
	}
	if (shift > 12)
	{
		shift = 12;
	}

	retailResult = 12 - shift;
	divisor = dotSegment >> retailResult;
	CTR_PSX_KEEP_VALUE(divisor);

	if (divisor < 0)
	{
		goto returnPolicy;
	}

	shift = (s32)((u32)dotCenter << shift);
	CTR_PSX_KEEP_VALUE(shift);
	factor = 0;
	if (divisor != 0)
	{
		factor = CTR_MipsDiv(shift, divisor);
	}
	sps->collision.instanceHitbox.lineFactor = factor;

	{
		register s32 projX CTR_PSX_REGISTER("$8") = CTR_MipsSra(CTR_MipsMulLo(factor, diffX), 12);
		register s32 projY CTR_PSX_REGISTER("$6") = 0;
		s32 projZ;
		register u32 relativeLow CTR_PSX_REGISTER("$3");
		register u32 relativeHigh CTR_PSX_REGISTER("$2");
		if ((*(const u8 *)&node->flag & BSP_HITBOX_USE_Y_AXIS) != 0)
		{
			projY = CTR_MipsSra(CTR_MipsMulLo(factor, diffY), 12);
		}
		CTR_PSX_KEEP_VALUE(projX);
		projZ = CTR_MipsSra(CTR_MipsMulLo(factor, diffZ), 12);
		relativeLow = (u32)CTR_MipsSubLo(projX, centerDiffX) & 0xffff;
		CTR_PSX_KEEP_VALUE_RELAXED(relativeLow);
		relativeHigh = (u32)CTR_MipsSll(CTR_MipsSubLo(projY, centerDiffY), 16);
		CTR_PSX_KEEP_VALUE_RELAXED(relativeHigh);
		sps->collision.instanceHitbox.projectedDelta.x = projX;
		sps->collision.instanceHitbox.projectedDelta.y = projY;
		{
			register s32 relZ CTR_PSX_REGISTER("$8") = CTR_MipsSubLo(projZ, centerDiffZ);
			register u32 projectedPair CTR_PSX_REGISTER("$7");
			sps->collision.instanceHitbox.projectedDelta.z = projZ;
			projectedPair = relativeLow | relativeHigh;
			CTC2(projectedPair, 0);
			CTC2(relZ, 1);
			MTC2(projectedPair, 0);
			MTC2_S(relZ, 1);
			CTR_PSX_GTE_PIPELINE_DELAY();
			doCOP2(0x0406012);
		}
	}

	radiusBase = sps->Input1.hitRadius;
	CTR_PSX_KEEP_VALUE(radiusBase);
	retailResult = (u32)node->data.hitbox.radius;
	CTR_PSX_KEEP_VALUE(retailResult);
	radius = (s32)retailResult;
	radiusSquared = radiusBase + radius;
	CTR_PSX_KEEP_VALUE(radiusSquared);
	radiusSquared = CTR_MipsMulLo(radiusSquared, radiusSquared);
	CTR_PSX_KEEP_VALUE(radiusSquared);
	{
		register s32 distanceOwner CTR_PSX_REGISTER("$8") = MFC2_S(25);
		CTR_PSX_KEEP_VALUE(distanceOwner);
		sps->collision.instanceHitbox.radiusSquared = radiusSquared;
		CTR_PSX_MEMORY_BARRIER();
		sps->collision.instanceHitbox.distanceSquared = distanceOwner;
		remaining = CTR_MipsSubLo(radiusSquared, distanceOwner);
	}
	if (remaining < 0)
	{
		goto returnPolicy;
	}
	CTR_PSX_KEEP_VALUE(remaining);

	if (remaining != 0)
	{
		if (dotSegment != 0)
		{
			register s32 shiftedDividend CTR_PSX_REGISTER("$2") = CTR_MipsSll(remaining, 12);
			CTR_PSX_KEEP_VALUE_RELAXED(shiftedDividend);
			factor = CTR_MipsSubLo(factor, CTR_MipsDiv(shiftedDividend, dotSegment));
		}
		sps->collision.instanceHitbox.adjustedFactor = factor;
	}

	retailResult = sps->hitFraction < factor;
	if (retailResult)
	{
		goto returnPolicy;
	}

	hitX = 0;
	hitY = hitX;
	hitZ = hitX;
	if (factor > 0)
	{
		register s32 rawHitX CTR_PSX_REGISTER("$6") = CTR_MipsMulLo(diffX, factor);
		register s32 rawHitY CTR_PSX_REGISTER("$2");
		register s32 rawHitZ CTR_PSX_REGISTER("$3");
		CTR_PSX_KEEP_VALUE(rawHitX);
		hitX = rawHitX >> 12;
		rawHitY = CTR_MipsMulLo(diffY, factor);
		hitY = rawHitY >> 12;
		rawHitZ = CTR_MipsMulLo(diffZ, factor);
		hitZ = rawHitZ >> 12;
	}

	if ((*(const u8 *)&node->flag & BSP_HITBOX_CHECK_Y_RANGE) != 0)
	{
		centerY = node->data.hitbox.center.y;
		if (hitY < centerY)
		{
			retailResult = CTR_MipsAddLo(centerY, node->id) < hitY;
			if (retailResult)
			{
				goto returnPolicy;
			}
		}
	}

	sps->bspHitbox = node;
	sps->hitFraction = factor;
	acceptedCount = CTR_MipsAddLo((u16)sps->boolDidTouchHitbox, 1);
	sps->boolDidTouchHitbox = (s16)acceptedCount;
	CTR_PSX_KEEP_VALUE(acceptedCount);
	sps->collision.instanceHitbox.hitDelta.x = hitX;
	sps->collision.instanceHitbox.hitDelta.y = hitY;
	sps->collision.instanceHitbox.hitDelta.z = hitZ;

	normalX = CTR_MipsSubLo(hitX, centerDiffX);
	normalY = 0;
	if ((*(const u8 *)&node->flag & BSP_HITBOX_USE_Y_AXIS) != 0)
	{
		normalY = CTR_MipsSubLo(hitY, centerDiffY);
	}
	// NOTE(aalhendi): Direct wrapped arithmetic avoids old GCC's hard-register inline-argument copies.
	normalZ = (s32)((u32)hitZ - (u32)centerDiffZ);

	{
		register u32 normalPair CTR_PSX_REGISTER("$7");
		register u32 low CTR_PSX_REGISTER("$3") = (u32)normalX & 0xffff;
		register u32 high CTR_PSX_REGISTER("$2");
		CTR_PSX_KEEP_VALUE(low);
		high = (u32)CTR_MipsSll(normalY, 16);
		CTR_PSX_KEEP_VALUE(high);
		normalPair = low | high;
		CTC2(normalPair, 0);
		CTC2(normalZ, 1);
		MTC2(normalPair, 0);
		MTC2_S(normalZ, 1);
		CTR_PSX_GTE_PIPELINE_DELAY();
		doCOP2(0x0406012);
	}

#ifdef CTR_NATIVE
	// NOTE(aalhendi): Native keeps SDK approximation without GTE writes; the game root has different rounding/state.
	len = SquareRoot0(MFC2_S(25));
	// NOTE(aalhendi): The fixed positive inverse numerator can trap only on zero.
	if (len == 0)
	{
		CTR_TRAP();
	}
#else
	len = SquareRoot0_stub(MFC2_S(25));
#endif
	{
		register s32 inverseNumerator CTR_PSX_REGISTER("$3") = 0x1000000;
		CTR_PSX_KEEP_VALUE_RELAXED(inverseNumerator);
		remaining = inverseNumerator / len;
		CTR_PSX_KEEP_VALUE_RELAXED(remaining);
	}

	{
		register s32 rawNormalX CTR_PSX_REGISTER("$7") = (s32)((u32)normalX * (u32)remaining);
		register s32 rawNormalY CTR_PSX_REGISTER("$2");
		register s32 rawNormalZ CTR_PSX_REGISTER("$3");
		normalX = rawNormalX >> 12;
		rawNormalY = (s32)((u32)normalY * (u32)remaining);
		normalY = rawNormalY >> 12;
		rawNormalZ = (s32)((u32)normalZ * (u32)remaining);
		normalZ = rawNormalZ >> 12;
	}
	sps->collision.instanceHitbox.normal.x = normalX;
	sps->collision.instanceHitbox.normal.y = normalY;
	sps->collision.instanceHitbox.normal.z = normalZ;
	lateRadius = node->data.hitbox.radius;

	rawScaledX = (s32)((u32)normalX * (u32)lateRadius);
	rawScaledY = (s32)((u32)normalY * (u32)lateRadius);

	originX = (u16)sps->Union.QuadBlockColl.pos.x;
	originY = (u16)sps->Union.QuadBlockColl.pos.y;
	retailResult = (u32)CTR_MipsAddLo(originX, hitX);
	sps->Union.QuadBlockColl.hitPos.x = (s16)retailResult;
	retailResult = (u16)sps->Union.QuadBlockColl.pos.z;
	sps->hit.plane.normal.x = (s16)normalX;
	sps->hit.plane.normal.y = (s16)normalY;
	sps->hit.plane.normal.z = (s16)normalZ;
	originY += (u32)hitY;
	retailResult += (u32)hitZ;
	sps->Union.QuadBlockColl.hitPos.z = (s16)retailResult;
	retailResult = COLL_TRIANGLE_CLIP_FACE;
	sps->Union.QuadBlockColl.hitPos.y = (s16)originY;
	sps->hit.reorderResult = (u8)retailResult;

	// NOTE(aalhendi): Reuse the consumed endpoint-Y temporary for radial X.
	originY = (u32)(rawScaledX >> 12);
	scaledY = rawScaledY >> 12;
	rawScaledZ = (s32)((u32)normalZ * (u32)lateRadius);
	scaledZ = rawScaledZ >> 12;
	sps->collision.instanceHitbox.scaledNormal.x = (s32)originY;
	sps->collision.instanceHitbox.scaledNormal.y = scaledY;
	sps->collision.instanceHitbox.scaledNormal.z = scaledZ;

	sps->hit.pushOut.x = (s16)CTR_MipsAddLo((u16)node->data.hitbox.center.x, (s32)originY);
	sps->hit.hitPos.x = sps->hit.pushOut.x;
	sps->hit.pushOut.y = (s16)(s32)((u32)(u16)node->data.hitbox.center.y + (u32)scaledY);
	sps->hit.hitPos.y = sps->hit.pushOut.y;
	retailResult = (u32)(s32)((u32)(u16)node->data.hitbox.center.z + (u32)scaledZ);
	CTR_PSX_KEEP_VALUE(retailResult);
	sps->hit.pushOut.z = (s16)retailResult;
	sps->hit.hitPos.z = sps->hit.pushOut.z;

returnPolicy:
	// NOTE(aalhendi): Retail leaves path-dependent v0; InstanceQuery ignores it.
	// The native public API retains force=6 and all other ordinary returns=0.
#ifdef CTR_NATIVE
	return nativeResult;
#else
	return retailResult;
#endif
}
