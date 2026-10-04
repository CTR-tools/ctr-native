#include <common.h>

// NOTE(aalhendi): Retail negation traps on INT_MIN. GCC 2.8.1 emits
// wrapping SUBU for C negation, so only this checked instruction is explicit.
#ifdef CTR_NATIVE
#define COLL_NEGATE_CHECKED(value)        \
	do                                    \
	{                                     \
		if ((value) == (-2147483647 - 1)) \
			CTR_TRAP();                   \
		(value) = -(value);               \
	} while (0)
#else
#define COLL_NEGATE_CHECKED(value) __asm__("sub %0,$0,%0" : "+r"(value))
#endif

void COLL_FIXED_TRIANGL_GetNormVec(struct ScratchpadStruct *sps, struct BspSearchVertex *v1, struct BspSearchVertex *v2, struct BspSearchVertex *v3)
{
	register struct ScratchpadStruct *context CTR_PSX_REGISTER("$4") = sps;
	register struct BspSearchVertex *origin CTR_PSX_REGISTER("$5") = v1;
	register s32 v1x CTR_PSX_REGISTER("$3") = origin->pos.x;
	register s32 v1y CTR_PSX_REGISTER("$8") = origin->pos.y;
	register s32 v1z CTR_PSX_REGISTER("$9") = origin->pos.z;
	register s32 nx CTR_PSX_REGISTER("$11") = v3->pos.x;
	register s32 ny CTR_PSX_REGISTER("$12") = v3->pos.y;
	register s32 nz CTR_PSX_REGISTER("$13") = v3->pos.z;
	register s32 edgeBX CTR_PSX_REGISTER("$14") = v2->pos.x;
	register s32 edgeBY CTR_PSX_REGISTER("$15") = v2->pos.y;
	register s32 edgeBZ CTR_PSX_REGISTER("$24") = v2->pos.z;
	register u32 lodShift CTR_PSX_REGISTER("$3");
	register u32 normalShift CTR_PSX_REGISTER("$8");
	register s32 scale CTR_PSX_REGISTER("$9");
	register u32 originXY CTR_PSX_REGISTER("$14");
	register u32 originZ CTR_PSX_REGISTER("$24");
	register s32 plane CTR_PSX_REGISTER("$3");
	register s32 difference CTR_PSX_REGISTER("$3");
	register CollNormalAxis dominantAxis CTR_PSX_REGISTER("$8");
	register s32 branchZero CTR_PSX_REGISTER("$0");

	nx -= v1x;
	ny -= v1y;
	nz -= v1z;
	edgeBX -= v1x;
	edgeBY -= v1y;
	edgeBZ -= v1z;
	nx &= 0xffff;
	ny &= 0xffff;
	nz &= 0xffff;
	CTC2(nx, 0);
	CTC2(ny, 2);
	CTC2(nz, 4);
	MTC2_S(edgeBX, 9);
	MTC2_S(edgeBY, 10);
	MTC2_S(edgeBZ, 11);
	lodShift = context->collision.triNormalLodShift;
	CTR_PSX_KEEP_VALUE(lodShift);
	normalShift = context->collision.triNormalVecBitShift;
	CTR_PSX_KEEP_VALUE(normalShift);
	doCOP2(0x0170000c);
	originXY = CTR_ReadU32LE(&origin->pos.x);
	originZ = (u16)origin->pos.z;
	scale = context->collision.triNormalVecDividend;
	CTR_PSX_KEEP_VALUE(scale);
	nx = MFC2_S(25);
	CTR_PSX_KEEP_VALUE(nx);
	ny = MFC2_S(26);
	CTR_PSX_KEEP_VALUE(ny);
	nx = CTR_MipsSra(nx, lodShift);
	nx = CTR_MipsMulLo(nx, scale);
	ny = CTR_MipsSra(ny, lodShift);
	CTR_PSX_KEEP_VALUE(nx);
	nz = MFC2_S(27);
	CTR_PSX_KEEP_VALUE(nz);
	CTC2(originXY, 0);
	ny = CTR_MipsMulLo(ny, scale);
	nz = CTR_MipsSra(nz, lodShift);
	CTR_PSX_KEEP_VALUE(ny);
	CTC2(originZ, 1);
	nx = CTR_MipsSra(nx, normalShift);
	CTR_PSX_KEEP_VALUE(nx);
	nz = CTR_MipsMulLo(nz, scale);
	CTR_PSX_KEEP_VALUE(nz);
	ny = CTR_MipsSra(ny, normalShift);
	nz = CTR_MipsSra(nz, normalShift);
	MTC2_S(nx, 9);
	MTC2_S(ny, 10);
	MTC2_S(nz, 11);
	origin->plane.normal.x = (s16)nx;
	origin->plane.normal.y = (s16)ny;
	doCOP2(0x049e012);
	plane = MFC2_S(25);
	CTR_PSX_KEEP_VALUE(plane);
	// NOTE(aalhendi): Empty observations keep these stores before the
	// magnitude branches instead of moving them into delay slots.
	origin->plane.normal.z = (s16)nz;
	CTR_PSX_OBSERVE_MEMORY(origin->plane.normal.z);
	plane = CTR_MipsSra(plane, 1);
	origin->plane.halfDistance = (s16)plane;
	CTR_PSX_OBSERVE_MEMORY(origin->plane.halfDistance);

	// Compare full magnitudes; X wins ties, followed by Y and Z.
	if (nx < 0)
		COLL_NEGATE_CHECKED(nx);
	if (ny < 0)
		COLL_NEGATE_CHECKED(ny);
	if (nz < 0)
		COLL_NEGATE_CHECKED(nz);
	CTR_PSX_KEEP_VALUE(nz);
	dominantAxis = COLL_NORMAL_AXIS_Z;
	CTR_PSX_KEEP_VALUE(dominantAxis);
	difference = nx - ny;
	CTR_PSX_KEEP_VALUE(difference);
	if (difference >= 0)
	{
		difference = nx - nz;
		CTR_PSX_KEEP_VALUE(difference);
		if (difference < 0)
			goto axisReady;
		// NOTE(aalhendi): ABI32 C jumps emit J, not retail's relative B.
		// Capture immutable $zero so GCC emits BGEZ-zero without loading it.
		CTR_PSX_CAPTURE_REGISTER(branchZero, 0);
		dominantAxis = COLL_NORMAL_AXIS_X;
		if (branchZero >= 0)
			goto axisReady;
	}
	difference = ny - nz;
	CTR_PSX_KEEP_VALUE(difference);
	if (difference >= 0)
		dominantAxis = COLL_NORMAL_AXIS_Y;
axisReady:
	origin->normalAxis = dominantAxis;
}

#undef COLL_NEGATE_CHECKED
