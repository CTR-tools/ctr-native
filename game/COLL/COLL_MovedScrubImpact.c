#include <common.h>

static inline s32 CollMoved_ScrubImpact_Abs(s16 value)
{
	return (value < 0) ? -(s32)value : value;
}

static inline void CollMoved_ScrubImpact_GteOP12(void)
{
	CTR_GteLoadDelay();
	doCOP2(0x0178000c);
}

static inline void CollMoved_ScrubImpact_LoadWallVelocity(const Vec3 *velocity)
{
#ifdef CTR_NATIVE
	MTC2(velocity->z, 11);
	MTC2(velocity->x, 9);
	MTC2(velocity->y, 10);
#else
	__asm__ volatile("lwc2 $11,8(%0)\n\tlwc2 $9,0(%0)\n\tlwc2 $10,4(%0)" : : "r"(velocity), "m"(*velocity));
#endif
}

// NOTE(aalhendi): These outer-product transfers use T4-T6 and full signed
// control words; negative halfwords also fill packed upper matrix components.
static inline void CollMoved_ScrubImpact_LoadDiagonal(const SVec3 *normal)
{
#ifdef CTR_NATIVE
	CTC2((s32)normal->x, 0);
	CTC2((s32)normal->y, 2);
	CTC2((s32)normal->z, 4);
#else
	__asm__ volatile("lh $12,0(%0)\n\tlh $13,2(%0)\n\tctc2 $12,$0\n\t"
	                 "lh $14,4(%0)\n\tctc2 $13,$2\n\tctc2 $14,$4"
	                 :
	                 : "r"(normal), "m"(*normal)
	                 : "$12", "$13", "$14");
#endif
}

static inline void CollMoved_ScrubImpact_DiagonalToIR(void)
{
#ifdef CTR_NATIVE
	u32 r11, r22, r33;
	r11 = CFC2(0);
	r22 = CFC2(2);
	r33 = CFC2(4);
	MTC2(r11, 9);
	MTC2(r22, 10);
	MTC2(r33, 11);
#else
	__asm__ volatile("cfc2 $12,$0\n\tcfc2 $13,$2\n\tcfc2 $14,$4\n\t"
	                 "mtc2 $12,$9\n\tmtc2 $13,$10\n\tmtc2 $14,$11"
	                 :
	                 :
	                 : "$12", "$13", "$14");
#endif
}

static inline void CollMoved_ScrubImpact_MACToDiagonal(void)
{
#ifdef CTR_NATIVE
	s32 crossX, crossY, crossZ;
	crossX = MFC2_S(25);
	crossY = MFC2_S(26);
	crossZ = MFC2_S(27);
	CTC2(crossX, 0);
	CTC2(crossY, 2);
	CTC2(crossZ, 4);
#else
	__asm__ volatile("mfc2 $12,$25\n\tmfc2 $13,$26\n\tmfc2 $14,$27\n\t"
	                 "ctc2 $12,$0\n\tctc2 $13,$2\n\tctc2 $14,$4"
	                 :
	                 :
	                 : "$12", "$13", "$14");
#endif
}

u32 COLL_MOVED_ScrubImpact(struct Driver *d, struct Thread *t, struct ScratchpadStruct *sps, struct Scrub *scrub, Vec3 *velocity)
{
	SVec3 normal;
	s32 dot;
	// NOTE(aalhendi): Reuse the coordinate workspace across collision phases
	// to retain the old compiler's retail value lifetimes.
	s32 workX, workZ;
	s32 workY;
	s32 scale;
	normal.x = sps->hit.plane.normal.x;
	normal.y = sps->hit.plane.normal.y;
	normal.z = sps->hit.plane.normal.z;

	if ((d->vShiftCount != 0) && (sps->boolDidTouchQuadblock != 0) && ((sps->hit.ptrQuadblock->quadFlags & QUADBLOCK_FLAG_GROUND) != 0) &&
	    ((u8)sps->hit.reorderResult != COLL_TRIANGLE_CLIP_FACE) && (sps->hit.ptrQuadblock != d->underDriver))
	{
		if ((CollMoved_ScrubImpact_Abs(d->speedApprox) < 0x300) && (CollMoved_ScrubImpact_Abs(d->jumpHeightCurr) < 0x300) && (d->fireSpeed == 0))
		{
			s32 diffY;
			workX = CTR_MipsSubLo(CTR_MipsSra(d->posCurr.x, 8), sps->hit.hitPos.x);
			workZ = CTR_MipsSubLo(CTR_MipsSra(d->posCurr.z, 8), sps->hit.hitPos.z);
			diffY = CTR_MipsSubLo(CTR_MipsSra(d->posCurr.y, 8), sps->hit.hitPos.y);

			if ((workX | diffY | workZ) != 0)
			{
				workY = VehCalc_FastSqrt((u32)CTR_MipsMulLo(workX, workX) + (u32)CTR_MipsMulLo(diffY, diffY) + (u32)CTR_MipsMulLo(workZ, workZ), 0);

				normal.x = (s16)CTR_MipsDiv(CTR_MipsSll(workX, 12), workY);
				normal.y = (s16)CTR_MipsDiv(CTR_MipsSll(diffY, 12), workY);
				normal.z = (s16)CTR_MipsDiv(CTR_MipsSll(workZ, 12), workY);
			}
		}
	}

	dot = CTR_MipsMulLo(CTR_MipsSra(velocity->x, 3), normal.x);
	dot = CTR_MipsAddLo(dot, CTR_MipsMulLo(CTR_MipsSra(velocity->y, 3), normal.y));
	dot = CTR_MipsAddLo(dot, CTR_MipsMulLo(CTR_MipsSra(velocity->z, 3), normal.z));
	dot = CTR_MipsSra(dot, 9);

	if (dot < -0xa00)
	{
		d->actionsFlagSet |= ACTION_TURBO_INPUT_LATCH;
	}

	dot = CTR_MipsSubLo(dot, sps->Input1.scrubDepth);

	if (dot < 0)
	{
		u32 scrubFlags = scrub->flags;
		s32 scrubSpeed;
		b32 speedLimitTooHigh;

		if ((scrubFlags & SCRUB_FLAG_SKIP_WALL_RUB_TIMER) == 0)
		{
			d->actionsFlagSet |= ACTION_DRIVING_AGAINST_WALL;
		}

		if ((scrubFlags & SCRUB_FLAG_KEEP_RESERVES) == 0)
		{
			d->reserves = 0;
			d->turbo_outsideTimer = 0;
		}

		scrubSpeed = scrub->speedLimit;

		if (d->wallRubTimer != 0)
		{
			speedLimitTooHigh = d->wallRubSpeedLimit < scrubSpeed;
		}
		else
		{
			speedLimitTooHigh = 0x3e7ff < scrubSpeed;
		}
		if (!speedLimitTooHigh)
		{
			d->wallRubTimer = DRIVER_WALL_RUB_TIMER_START;
			d->wallRubSpeedLimit = scrubSpeed;
			CTR_WriteU32AlignedLE(&d->posWallColl, CTR_ReadU32AlignedLE(&sps->hit.hitPos));
			d->posWallColl.z = (u16)sps->hit.hitPos.z;
		}

		if ((scrubFlags & SCRUB_FLAG_APPLY_IMPACT) != 0)
		{
			s32 impactX, impactZ;
			s32 speedSq = 0;
			s32 transformedImpactXZ;
			// NOTE(aalhendi): Reuse the Z-update word for packed Y. The soft
			// local preference retains retail's allocation; it is not caller ABI.
			s32 component CTR_PSX_LOCAL_REGISTER_HINT("$3");
			impactX = CTR_MipsSra(CTR_MipsMulLo(dot, normal.x), 12);
			workY = CTR_MipsSra(CTR_MipsMulLo(dot, normal.y), 12);
			impactZ = CTR_MipsSra(CTR_MipsMulLo(dot, normal.z), 12);

			if (scrub->impactAngle != 0)
			{
				speedSq = CTR_MipsSra(CTR_MipsAddLo(CTR_MipsAddLo(CTR_MipsMulLo(velocity->x, velocity->x), CTR_MipsMulLo(velocity->y, velocity->y)),
				                                    CTR_MipsMulLo(velocity->z, velocity->z)),
				                      15);
			}

			workX = velocity->x;
			workZ = velocity->z;
			velocity->x = CTR_MipsSubLo(workX, impactX);
			// NOTE(aalhendi): Read the live Z field for the update; the old Z
			// remains cached separately for the wall projection below.
			component = velocity->z;
			component = CTR_MipsSubLo(component, impactZ);
			velocity->z = component;
			velocity->y = CTR_MipsSubLo(velocity->y, workY);

			component = CTR_MipsSll(workY, 16);
			MTC2((u16)impactX | (u32)component, 0);
			MTC2(impactZ, 1);
			CTR_GteLoadDelay();
			doCOP2(0x04a6012);
			CTR_GteReadMAC123(impactX, workY, impactZ);

			if ((sps->boolDidTouchQuadblock != 0) && ((sps->Union.QuadBlockColl.searchFlags & COLL_SEARCH_WALL_PROJECTION_DONE) == 0) &&
			    ((d->actionsFlagSetPrevFrame & ACTION_TOUCH_GROUND) == 0) && ((sps->hit.ptrQuadblock->quadFlags & QUADBLOCK_FLAG_GROUND) != 0))
			{
				Vec3 wallVelocity;
				u32 wallSpeedSq;
				wallVelocity.x = workX;
				wallVelocity.y = 0;
				wallVelocity.z = workZ;
				CollMoved_ScrubImpact_LoadWallVelocity(&wallVelocity);
				CollMoved_ScrubImpact_LoadDiagonal(&normal);
				CollMoved_ScrubImpact_GteOP12();
				CollMoved_ScrubImpact_DiagonalToIR();
				CollMoved_ScrubImpact_MACToDiagonal();
				CollMoved_ScrubImpact_GteOP12();
				CTR_PSX_STORE_COP2_VEC3((VecElement *)&wallVelocity, 25, 26, 27);

				wallSpeedSq = (u32)CTR_MipsMulLo(wallVelocity.x, wallVelocity.x) + (u32)CTR_MipsMulLo(wallVelocity.y, wallVelocity.y) +
				              (u32)CTR_MipsMulLo(wallVelocity.z, wallVelocity.z);
				workY = VehCalc_FastSqrt(wallSpeedSq, 0x10) >> 8;
				scale = d->speedApprox;

				if ((workY != 0) && (scale > 0))
				{
					sps->Union.QuadBlockColl.searchFlags |= COLL_SEARCH_WALL_PROJECTION_DONE;
					velocity->x = CTR_MipsDiv(CTR_MipsMulLo(wallVelocity.x, scale), workY);
					velocity->y = CTR_MipsDiv(CTR_MipsMulLo(wallVelocity.y, scale), workY);
					velocity->z = CTR_MipsDiv(CTR_MipsMulLo(wallVelocity.z, scale), workY);
					velocity->x = CTR_MipsSubLo(velocity->x, CTR_MipsSra(normal.x, 1));
					velocity->y = CTR_MipsSubLo(velocity->y, CTR_MipsSra(normal.y, 1));
					velocity->z = CTR_MipsSubLo(velocity->z, CTR_MipsSra(normal.z, 1));
				}
			}

			if (((scrubFlags & SCRUB_FLAG_SLAM_ON_HARD_IMPACT) != 0) && (dot < -0x13ff))
			{
				transformedImpactXZ = CTR_MipsAddLo(CTR_MipsMulLo(impactX, impactX), CTR_MipsMulLo(impactZ, impactZ));
				if (transformedImpactXZ > 0x1900000)
				{
					if (d->kartState != KS_MASK_GRABBED)
					{
						if (d->simpTurnState > 0)
						{
							GAMEPAD_JogCon1(d, 0x2f, 0x60);
						}
						else
						{
							GAMEPAD_JogCon1(d, 0x1f, 0x60);
						}
					}

					scale = scrub->impactAngle;
					if (scale != 0)
					{
						s32 scaledSpeed, angleLimit, dotSq;
						const struct TrigTable *table = data.trigApprox;
						workY = (s32)CTR_ReadU32AlignedLE(&table[ANG_MODULO_HALF_PI(scale)]);
						if (IS_ANG_FIRST_OR_THIRD_QUADRANT(scale))
						{
							workY = CTR_MipsSll(workY, 16);
						}
						workY = CTR_MipsSra(workY, 16);
						if (IS_ANG_THIRD_OR_FOURTH_QUADRANT(scale))
						{
							workY = CTR_MipsNegLo(workY);
						}
						scaledSpeed = CTR_MipsMulLo(speedSq, workY);
						dotSq = CTR_MipsMulLo(dot, dot);
						scaledSpeed = CTR_MipsSra(scaledSpeed, 12);
						angleLimit = CTR_MipsMulLo(scaledSpeed, workY);
						dotSq = CTR_MipsSra(dotSq, 15);
						angleLimit = CTR_MipsSra(angleLimit, 12);

						if (angleLimit >= dotSq)
						{
							goto impact_result;
						}
					}

					if ((d->kartState != KS_MASK_GRABBED) && (transformedImpactXZ > 0x1900000))
					{
						u32 soundFlags = HowlSfx_Pack(HOWL_SFX_LR_CENTER, HOWL_SFX_DISTORTION_NONE, HOWL_SFX_VOLUME_MAX, 0);

						if ((d->actionsFlagSet & ACTION_ENGINE_ECHO) != 0)
						{
							soundFlags = HowlSfx_Pack(HOWL_SFX_LR_CENTER, HOWL_SFX_DISTORTION_NONE, HOWL_SFX_VOLUME_MAX, 1);
						}

						OtherFX_Play_LowLevel(6, 1, soundFlags);
						Voiceline_RequestPlay(6, GAME_CHARACTER_IDS[d->driverID], 0x10);
						GAMEPAD_ShockFreq(d, 8, 0);
						GAMEPAD_ShockForce1(d, 8, 0x7f);

						if (d->kartState == KS_DRIFTING)
						{
							scale = (u16)d->turnAngleCurr;
							d->angle = (s16)((u16)d->angle + scale);
							workY = (u16)d->turnAngleCurr;
							d->rotCurr.w = (s16)((u16)d->rotCurr.w - workY);
							d->turnAngleCurr = 0;
						}

						d->instSelf->animIndex = 2;
						d->instSelf->animFrame = 0;
						d->matrixArray = BAKED_GTE_MATRIX_CRASH_FALL;
						d->matrixIndex = 0;

						VehPhysProc_SlamWall_Init(t, d);
						return 2;
					}
				}
			}
		impact_result:
			return 1;
		}
	}

	return 0;
}
