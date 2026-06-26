#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80059100-0x80059344.
struct Particle *VehEmitter_Exhaust(struct Driver *d, VECTOR *exhaustPos, VECTOR *exhaustVel)
{
	int exhaustType;

	struct ParticleEmitter *emSet;
	struct Particle *p = NULL;
	struct GameTracker *gGT = sdata->gGT;
	struct Instance *dInst = d->instSelf;

	if (d->invisibleTimer != 0)
	{
		return 0;
	}

	if ((dInst->flags & HIDE_MODEL) != 0)
	{
		return 0;
	}

	// low LOD exhaust (4p or ai car)
	exhaustType = 1;
	emSet = &data.emSet_Exhaust_Low[0];

	char numPlyr = gGT->numPlyrCurrGame;

	// equivalent of (d->driverID < numPlyr),
	// because modelIndex is not set to DYNAMIC_ROBOT_CAR
	// for human players after BOTS_Driver_Convert is called
	if (dInst->thread->modelIndex != DYNAMIC_ROBOT_CAR)
	{
		switch (numPlyr)
		{
		case 1:
			// 1P mode, high LOD exhaust
			emSet = &data.emSet_Exhaust_High[0];
			break;
		case 2:
			// 2P mode, med LOD exhaust
			emSet = &data.emSet_Exhaust_Med[0];
			break;
		}
	}

	if (((dInst->flags & SPLIT_LINE) != 0) && ((exhaustPos->vy - exhaustVel->vy) + d->posCurr.y < 256))
	{
		// bubble texture
		exhaustType = 7;
		emSet = &data.emSet_Exhaust_Water[0];
	}

	p = Particle_Init(0, gGT->iconGroup[exhaustType], emSet);

	if (p == NULL)
	{
		return p;
	}

	p->axis[0].startVal += exhaustPos->vx - exhaustVel->vx;
	p->axis[0].velocity = (s16)exhaustVel->vx;
	p->axis[1].startVal += exhaustPos->vy - exhaustVel->vy;
	p->axis[1].velocity = (s16)exhaustVel->vy;
	p->axis[2].startVal += exhaustPos->vz - exhaustVel->vz;
	p->axis[2].velocity = (s16)exhaustVel->vz;

	p->driverInst = dInst;
	p->otIndexOffset = dInst->depthBiasNormal;

	if (exhaustType == 7)
	{
		p->funcPtr = Particle_FuncPtr_ExhaustUnderwater;
	}

	// if engine revving
	if (d->kartState == KS_ENGINE_REVVING)
	{
		if (d->revEngineState != 1)
		{
			return p;
		}
	}

	// if not engine revving
	else
	{
		s16 meterLeft = d->turbo_MeterRoomLeft;
		if ((meterLeft < 129) || (((d->const_turboLowRoomWarning + 2) * 32) < meterLeft))
		{
			return p;
		}
	}

	p->flagsSetColor &= ~(0x60);
	p->flagsSetColor |= 0x40;

	return p;
}

static const SVECTOR sparkGround_inX = {0x1800, 0, 0, 0};
static const SVECTOR sparkGround_inZ = {0, 0, -0x1800, 0};
static const SVECTOR sparkGround_inZ2 = {0, 0, -0x200, 0};

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80059344-0x80059558.
void VehEmitter_Sparks_Ground(struct Driver *d, struct ParticleEmitter *emSet)
{
	struct GameTracker *gGT = sdata->gGT;

	Vec3 outX;
	Vec3 outZ;
	Vec3 outZ2;

	CTR_GteLoadSV0(&sparkGround_inX);
	gte_rtv0();
	CTR_GteStoreMAC(outX.v);

	CTR_GteLoadSV0(&sparkGround_inZ);
	gte_rtv0();
	CTR_GteStoreMAC(outZ.v);

	CTR_GteLoadSV0(&sparkGround_inZ2);
	gte_rtv0();
	CTR_GteStoreMAC(outZ2.v);

	for (int i = 0; i < 10; i++)
	{
		// Create instance in particle pool
		struct Particle *p = Particle_Init(0, gGT->iconGroup[0], emSet);

		if (p == NULL)
		{
			continue;
		}

		u32 rng = (u32)(RngDeadCoed(&gGT->deadcoed_struct.unk1) & 0x7ff);

		if ((rng & 1) != 0)
		{
			rng = -rng;
		}

		for (int j = 0; j < 3; j++)
		{
			p->axis[j].velocity += (s16)outZ2.v[j] + (s16)((rng * outX.v[j]) >> 12);
			p->axis[j].startVal += outZ.v[j] + p->axis[j].velocity;
		}

		p->driverInst = d->instSelf;
		p->otIndexOffset = d->instSelf->depthBiasNormal;
	}
}

static const SVECTOR terrainEmitterPos[4] = {
    {0x1E, 0xA, -0x14, 0},
    {-0x1E, 0xA, -0x14, 0},
    {0x1E, 0xA, 0x28, 0},
    {-0x1E, 0xA, 0x28, 0},
};

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80059558-0x80059780.
void VehEmitter_Terrain_Ground(struct Driver *d, struct ParticleEmitter *emSet)
{
	int speed;
	char numTires;
	Vec3 pos;
	Vec3 vel;

	int flags = d->actionsFlagSet;

	// not touching quadblock
	if ((flags & 1) == 0)
	{
		return;
	}

	// in accel prevention (holding square)
	if ((flags & 8) != 0)
	{
		return;
	}

	// abs fireSpeed < 0x300
	speed = d->fireSpeed;
	if (speed < 0)
	{
		speed = -speed;
	}
	if (speed < 0x300)
	{
		// abs speedApprox < 0x300
		speed = d->speedApprox;
		if (speed < 0)
		{
			speed = -speed;
		}
		if (speed < 0x300)
		{
			return;
		}
	}

	// if sliding, spawn on 4 tires, otherwise just 2
	numTires = (d->kartState == KS_DRIFTING) ? 4 : 2;

	struct Instance *dInst = d->instSelf;
	struct IconGroup *ig = sdata->gGT->iconGroup[0];

	// spawn particles on wheels
	for (; numTires != 0; numTires--)
	{
		CTR_GteLoadSV0(&terrainEmitterPos[numTires - 1]);
		gte_rtv0();
		CTR_GteStoreMAC(pos.v);

		struct Particle *p = Particle_Init(0, ig, emSet);

		if (p == NULL)
		{
			continue;
		}

		SVECTOR velInput = {p->axis[0].velocity, p->axis[1].velocity, p->axis[2].velocity, 0};

		CTR_GteLoadSV0(&velInput);
		gte_rtv0();
		CTR_GteStoreMAC(vel.v);

		for (int i = 0; i < 3; i++)
		{
			p->axis[i].startVal += pos.v[i] * 0x100;
			p->axis[i].velocity = (s16)vel.v[i];
		}

		p->driverInst = dInst;
		p->otIndexOffset = dInst->depthBiasNormal;
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80059780-0x80059a18.
void VehEmitter_Sparks_Wall(struct Driver *d, struct ParticleEmitter *emSet)
{
	int speedAbs = d->speedApprox;
	if (speedAbs < 0)
	{
		speedAbs = -speedAbs;
	}

	// must have speed, or gas pedal, for vibration
	if (((d->fireSpeed != 0) || (speedAbs > 0x200)) && (d->frameAgainstWall < 450))
	{
		// both gamepad vibration
		GAMEPAD_ShockFreq(d, 8, 0);
		GAMEPAD_ShockForce1(d, 8, 0x7f);

		d->frameAgainstWall++;
	}
	else
	{
		d->frameAgainstWall = 0;
	}

	// must reach minimum speed for sparks
	if (speedAbs <= 0x200)
	{
		return;
	}

	union VehEmitterWallScratch *scratch = CTR_SCRATCHPAD_PTR(union VehEmitterWallScratch, 0);
	s32 *tireLeftOutWord = &scratch->word[0];
	s32 *tireRightOutWord = &scratch->word[3];
	s16 *tireLeftOutHalf = &scratch->half[0];
	s16 *tireRightOutHalf = &scratch->half[3];
	s16 *distIn4 = &scratch->half[6];
	s32 *distOut4 = &scratch->word[3];

	// s16[3] array
	tireLeftOutWord[0] = 0xa00de00;
	tireRightOutWord[0] = 0xa002200;

	int valZ = -0x1400;
	if (d->speedApprox > 0)
	{
		valZ = 0x2800;
	}

	// s16[3] array
	tireLeftOutWord[1] = valZ;
	tireRightOutWord[1] = valZ;

	CTR_GteLoadS16TripletV0(tireLeftOutHalf);
	gte_rtv0();
	CTR_GteStoreMAC(&tireLeftOutWord[0]);

	CTR_GteLoadS16TripletV0(tireRightOutHalf);
	gte_rtv0();
	CTR_GteStoreMAC(&tireRightOutWord[0]);

	// this compresses TireLeft and TireRight from int to s16,
	// which then doubles in usage as a matrix (3x2)
	for (int i = 0; i < 6; i++)
	{
		tireLeftOutHalf[i] = (u16)scratch->word[i];
	}

#ifdef CTR_NATIVE

#define gte_SetLightMatrix3x2(r0)                  \
	{                                              \
		CTC2(*(uint32_t *)((char *)(r0)), 8);      \
		CTC2(*(uint32_t *)((char *)(r0) + 4), 9);  \
		CTC2(*(uint32_t *)((char *)(r0) + 8), 10); \
	}

#else

#define gte_SetLightMatrix3x2(r0)        \
	__asm__ volatile("lw		$t0, 0( %0 );" \
	                 "lw		$t1, 4( %0 );" \
	                 "lw		$t2, 8( %0 );" \
	                 "ctc2	$t0, $8;"     \
	                 "ctc2	$t1, $9;"     \
	                 "ctc2	$t2, $10;"    \
	                 :                   \
	                 : "r"(r0)           \
	                 : "$t2")

#endif

	gte_SetLightMatrix3x2(&scratch->half[0]);

	// dist4 is actual distance
	distIn4[0] = (d->posWallColl.x * 0x100) - d->posCurr.x;
	distIn4[1] = (d->posWallColl.y * 0x100) - d->posCurr.y;
	distIn4[2] = (d->posWallColl.z * 0x100) - d->posCurr.z;

	CTR_GteLoadS16TripletV0(&distIn4[0]);
	gte_llv0();
	CTR_GteStoreMAC(&distOut4[0]);
	if (distOut4[0] < distOut4[1])
	{
		tireLeftOutHalf = tireRightOutHalf;
	}

	// Create instance in particle pool
	struct Particle *p = Particle_Init(0, sdata->gGT->iconGroup[0], emSet);

	if (p == NULL)
	{
		return;
	}

	for (int i = 0; i < 3; i++)
	{
		p->axis[i].startVal += tireLeftOutHalf[i];
		distIn4[i] = p->axis[i].velocity;
	}

	// dist4 now determines velocity
	CTR_GteLoadS16TripletV0(&distIn4[0]);
	gte_rtv0();
	CTR_GteStoreMAC(&distOut4[0]);

	p->axis[0].velocity = (s16)distOut4[0];
	p->axis[1].velocity = (s16)distOut4[1];
	p->axis[2].velocity = (s16)distOut4[2];

	p->driverInst = d->instSelf;
}

static void VehEmitter_SetRotTransMatrix(MATRIX *m)
{
	gte_SetRotMatrix(m);
	gte_SetTransMatrix(m);
}

static void VehEmitter_RotVec(const SVECTOR *in, VECTOR *out)
{
	CTR_GteLoadSV0(in);
	gte_rtv0();
	CTR_GteStoreMAC(&out->vx);
}

static void VehEmitter_RotTransVec(const SVECTOR *in, VECTOR *out)
{
	CTR_GteLoadSV0(in);
	gte_rt();
	CTR_GteStoreMAC(&out->vx);
}

static union VehEmitterSkidmark *VehEmitter_GetSkidmark(struct Driver *d, u8 frameIndex, int tireIndex)
{
	return &d->skidmarks[frameIndex & (DRIVER_SKIDMARK_FRAME_COUNT - 1)][tireIndex];
}

static void VehEmitter_WriteSkidmark(struct Driver *d, u8 frameIndex, int tireIndex, int x, int y, int z, int widthX, int widthZ, u8 color, u8 flags)
{
	union VehEmitterSkidmark *mark = VehEmitter_GetSkidmark(d, frameIndex, tireIndex);

	mark->edge0.x = (s16)(x + widthX);
	mark->edge0.y = (s16)y;
	mark->edge0.z = (s16)(z + widthZ);
	mark->color = color;
	mark->flags = flags;
	mark->edge1.x = (s16)(x - widthX);
	mark->edge1.y = (s16)y;
	mark->edge1.z = (s16)(z - widthZ);
}

static void VehEmitter_WriteSkidmarkPair(struct Driver *d, int tireIndex, int x, int y, int z, int lateralX, int lateralZ, int widthX, int widthZ, u8 color,
                                         u8 flags)
{
	u8 frame = (u8)d->skidmarkFrameIndex;

	VehEmitter_WriteSkidmark(d, frame, tireIndex, x, y, z, widthX, widthZ, color, flags);

	x += lateralX;
	z += lateralZ;
	VehEmitter_WriteSkidmark(d, (u8)(frame - 1), tireIndex, x, y, z, widthX >> 1, widthZ >> 1, color, flags);
}

static void VehEmitter_Skidmarks(struct Thread *thread, struct Driver *d, TerrainFlags terrainFlags)
{
	struct Instance *inst = thread->inst;
	MATRIX *m = &inst->matrix;
	u8 color = ((inst->flags & SPLIT_LINE) == 0) ? inst->depthBiasNormal : inst->depthBiasSecondary;
	u8 flags = ((terrainFlags & TERRAIN_FLAG_FORCE_SKIDMARKS) == 0) ? 0 : 1;
	int sin = MATH_Sin(d->axisRotationX);
	int cos = MATH_Cos(d->axisRotationX);
	int lateralX = (sin * 15) >> 12;
	int lateralZ = (cos * 15) >> 12;
	int widthX = (cos * 10) >> 12;
	int widthZ = (-sin * 10) >> 12;
	VECTOR pos;

	color += 2;

	VehEmitter_SetRotTransMatrix(m);

	if ((d->actionsFlagSet & ACTION_BACK_SKID) != 0)
	{
		d->skidmarkEnableFlags |= 1;

		SVECTOR local = {-0x1e, 0, -0x14, 0};
		VehEmitter_RotTransVec(&local, &pos);
		VehEmitter_WriteSkidmarkPair(d, 0, pos.vx - (lateralX >> 1), pos.vy, pos.vz - (lateralZ >> 1), lateralX, lateralZ, widthX, widthZ, color, flags);

		d->skidmarkEnableFlags |= 2;

		local.vx = 0x1e;
		VehEmitter_RotTransVec(&local, &pos);
		VehEmitter_WriteSkidmarkPair(d, 1, pos.vx - (lateralX >> 1), pos.vy, pos.vz - (lateralZ >> 1), lateralX, lateralZ, widthX, widthZ, color, flags);
	}

	if ((d->actionsFlagSet & ACTION_FRONT_SKID) != 0)
	{
		d->skidmarkEnableFlags |= 4;

		SVECTOR local = {-0x1e, 0, 0x28, 0};
		VehEmitter_RotTransVec(&local, &pos);
		VehEmitter_WriteSkidmarkPair(d, 2, pos.vx - (lateralX >> 1), pos.vy, pos.vz - (lateralZ >> 1), lateralX, lateralZ, widthX, widthZ, color, flags);

		d->skidmarkEnableFlags |= 8;

		local.vx = 0x1e;
		VehEmitter_RotTransVec(&local, &pos);
		VehEmitter_WriteSkidmarkPair(d, 3, pos.vx - (lateralX >> 1), pos.vy, pos.vz - (lateralZ >> 1), lateralX, lateralZ, widthX, widthZ, color, flags);
	}
}

static void VehEmitter_MudSplash(struct Driver *d)
{
	struct GameTracker *gGT = sdata->gGT;
	int count = ((d->actionsFlagSet & ACTION_STARTED_TOUCH_GROUND) == 0) ? 1 : 10;

	for (; count != 0; count--)
	{
		struct Particle *p = Particle_Init(0, gGT->iconGroup[0xd], &data.emSet_MudSplash[0]);

		if (p == NULL)
		{
			continue;
		}

		p->otIndexOffset = d->instSelf->depthBiasNormal;
		p->driverInst = d->instSelf;
		p->driverID = d->driverID;

		p->axis[0].startVal += (int)p->axis[0].velocity * 0x10;
		p->axis[2].startVal += (int)p->axis[2].velocity * 0x10;
		p->axis[0].accel -= p->axis[0].velocity >> 4;
		p->axis[2].accel -= p->axis[2].velocity >> 4;
	}
}

static void VehEmitter_TerrainEffects(struct Thread *thread, struct Driver *d, struct Terrain *terrain, TerrainFlags terrainFlags, int absSpeedApprox)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Instance *inst = thread->inst;
	MATRIX *m = &inst->matrix;

	if (gGT->numPlyrCurrGame >= 2)
	{
		return;
	}

	int absSpeed = d->speed;
	if (absSpeed < 0)
	{
		absSpeed = -absSpeed;
	}

	if ((absSpeed > 0x500) && (d->currentTerrain == TERRAIN_MUD))
	{
		VehEmitter_MudSplash(d);
	}

	if (((terrainFlags & TERRAIN_FLAG_LANDING_SPARKS) != 0) && ((d->actionsFlagSet & ACTION_STARTED_TOUCH_GROUND) != 0) && (absSpeedApprox > 0x600))
	{
		int absJump = d->jumpHeightPrev;
		if (absJump < 0)
		{
			absJump = -absJump;
		}

		if (absJump > 0x1600)
		{
			VehEmitter_SetRotTransMatrix(m);
			VehEmitter_Sparks_Ground(d, &data.emSet_GroundSparks[0]);
		}
	}

	if (terrain->em_OddFrame != NULL)
	{
		struct ParticleEmitter *emSet = terrain->em_OddFrame;

		if ((terrain->em_EvenFrame != NULL) && ((gGT->timer & 1) != 0))
		{
			emSet = terrain->em_EvenFrame;
		}

		VehEmitter_SetRotTransMatrix(m);
		VehEmitter_Terrain_Ground(d, emSet);
	}

	int wallSound = 0x14;
	s32 engineVol = (s16)d->engineVol;

	if ((d->wallRubTimer == 0xf0) && (d->kartState != KS_MASK_GRABBED))
	{
		VehEmitter_SetRotTransMatrix(m);
		VehEmitter_Sparks_Wall(d, &data.emSet_WallSparks[0]);

		engineVol += 0x14;
		if (engineVol > 0xff)
		{
			engineVol = 0xff;
		}
	}
	else
	{
		if (d->wallRubTimer == 0)
		{
			d->frameAgainstWall = 0;
		}

		engineVol -= 0x14;
		if (engineVol < 0)
		{
			engineVol = 0;
		}

		if (engineVol == 0)
		{
			wallSound = -1;
		}
	}

	d->engineVol = (u16)engineVol;

	if (thread->modelIndex == DYNAMIC_PLAYER)
	{
		u32 echo = ((d->actionsFlagSet & ACTION_ENGINE_ECHO) != 0);
		u32 flags = HowlSfx_Pack(HOWL_SFX_LR_CENTER, HOWL_SFX_DISTORTION_NONE, (u32)(s16)d->engineVol, echo);

		OtherFX_RecycleNew((u32 *)&d->driverAudioPtrs[2], wallSound, flags);
	}
}

static void VehEmitter_TerrainAudioAndFeedback(struct Thread *thread, struct Driver *d, struct Terrain *terrain, TerrainFlags terrainFlags, int absSpeedApprox)
{
	if (thread->modelIndex != DYNAMIC_PLAYER)
	{
		return;
	}

	int soundID = -1;

	if (((d->actionsFlagSet & ACTION_TOUCH_GROUND) != 0) && ((terrainFlags & TERRAIN_FLAG_ONESHOT_GROUND_SOUND) == 0))
	{
		soundID = terrain->sound;
	}

	int vol = VehCalc_MapToRange(absSpeedApprox, 0, 5000, 0, 200);
	int distort = VehCalc_MapToRange(absSpeedApprox, 0, 12000, 0x6c, 0xd2);
	u32 echo = ((d->actionsFlagSet & ACTION_ENGINE_ECHO) != 0);
	OtherFX_RecycleNew((u32 *)&d->driverAudioPtrs[1], soundID, HowlSfx_Pack(HOWL_SFX_LR_CENTER, (u32)distort, (u32)vol, echo));

	if ((d->actionsFlagSet & ACTION_BOT) == 0)
	{
		if (absSpeedApprox > 0x200)
		{
			GAMEPAD_ShockFreq(d, terrain->vibrationData[0], terrain->vibrationData[1]);
			GAMEPAD_ShockForce2(d, terrain->vibrationData[2], terrain->vibrationData[3]);
		}

		if ((d->actionsFlagSet & ACTION_STARTED_TOUCH_GROUND) != 0)
		{
			int absJump = d->jumpHeightPrev;
			if (absJump < 0)
			{
				absJump = -absJump;
			}

			if (absJump > 0x1600)
			{
				GAMEPAD_ShockForce1(d, 3, 0xff);
			}
		}
	}
}

static void VehEmitter_SkidmarkAudio(struct Thread *thread, struct Driver *d, struct Terrain *terrain, TerrainFlags terrainFlags, int absSpeedApprox)
{
	if (((d->actionsFlagSet & ACTION_TOUCH_GROUND) == 0) || ((d->actionsFlagSet & (ACTION_BACK_SKID | ACTION_FRONT_SKID)) == 0) || (absSpeedApprox < 0x201))
	{
		if (d->driverAudioPtrs[0] != NULL)
		{
			OtherFX_Stop1((int)d->driverAudioPtrs[0]);
			d->driverAudioPtrs[0] = NULL;
		}
		return;
	}

	if (thread->modelIndex == DYNAMIC_PLAYER)
	{
		int absTurn = d->simpTurnState;
		if (absTurn < 0)
		{
			absTurn = -absTurn;
		}

		int vol = VehCalc_MapToRange(absSpeedApprox, 2000, 12000, 0x14, 0xaa);
		int distort = VehCalc_MapToRange(absSpeedApprox, 2000, 12000, 0x92, 0x78);

		if (d->kartState == KS_DRIFTING)
		{
			int drift = d->turnWobbleAngle;
			if (drift < 0)
			{
				drift = -drift;
			}

			distort -= drift;
			if (distort < 0)
			{
				distort = 0;
			}
		}

		distort += absTurn;
		if (distort > 0x92)
		{
			distort = 0x92;
		}

		u32 lr = 0x80u - (((u32)(u8)d->simpTurnState << 24) >> 26);
		u32 echo = ((d->actionsFlagSet & ACTION_ENGINE_ECHO) != 0);
		u32 flags = HowlSfx_Pack(lr, (u32)distort, (u32)(vol + (absTurn >> 1)), echo);

		OtherFX_RecycleNew((u32 *)&d->driverAudioPtrs[0], terrain->skidSound, flags);
	}

	VehEmitter_Skidmarks(thread, d, terrainFlags);
}

static int VehEmitter_ShouldSkipExhaust(struct Thread *thread, struct Driver *d)
{
	struct GameTracker *gGT = sdata->gGT;

	if (thread->modelIndex == DYNAMIC_ROBOT_CAR)
	{
		if ((gGT->timer & 3) != (d->driverID & 3))
		{
			return 1;
		}
	}
	else
	{
		if (d->revEngineState == 2)
		{
			return 1;
		}

		u32 numPlyr = gGT->numPlyrCurrGame;

		if (numPlyr > 1)
		{
			if (((numPlyr != 2) || ((gGT->timer & 1) != d->driverID)) && ((gGT->timer & 3) != d->driverID))
			{
				return 1;
			}
		}

		if (d->failedBoostExhaustTimer == 0)
		{
			int meterLeft = d->turbo_MeterRoomLeft;

			if ((meterLeft < 0x81) || (((d->const_turboLowRoomWarning + 2) * 0x20) < meterLeft))
			{
				if (PROC_SearchForModel(thread->childThread, STATIC_TURBO_EFFECT) != NULL)
				{
					return 1;
				}
			}
		}
	}

	if (d->failedBoostExhaustTimer != 0)
	{
		d->failedBoostExhaustTimer--;
	}

	return 0;
}

static void VehEmitter_ExhaustPair(struct Thread *thread, struct Driver *d)
{
	struct Instance *inst = thread->inst;
	MATRIX *m = &inst->matrix;
	SVECTOR local;
	VECTOR exhaustPos;
	VECTOR exhaustVel;

	gte_SetRotMatrix(m);

	local.vx = 0;
	local.vy = 0x400;
	local.vz = -0x400;
	local.pad = 0;
	VehEmitter_RotVec(&local, &exhaustVel);

	local.vx = (s16)((inst->scale.x * 9) >> 3);
	local.vy = (s16)((inst->scale.y * 7) >> 1);
	local.vz = (s16)((inst->scale.z * -0x38) >> 4);
	VehEmitter_RotVec(&local, &exhaustPos);
	VehEmitter_Exhaust(d, &exhaustPos, &exhaustVel);

	local.vx = (s16)((inst->scale.x * -0x12) >> 4);
	VehEmitter_RotVec(&local, &exhaustPos);
	VehEmitter_Exhaust(d, &exhaustPos, &exhaustVel);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80059a18-0x8005ab24
void VehEmitter_DriverMain(struct Thread *thread, struct Driver *d)
{
	struct Terrain *terrain = d->terrainMeta1;
	TerrainFlags terrainFlags = terrain->flags;
	int absSpeedApprox = d->speedApprox;

	d->skidmarkEnableFlags = (d->skidmarkEnableFlags & 0xfffff) << 4;
	d->skidmarkFrameIndex = (d->skidmarkFrameIndex - 1) & 7;

	if (absSpeedApprox < 0)
	{
		absSpeedApprox = -absSpeedApprox;
	}

	VehEmitter_TerrainAudioAndFeedback(thread, d, terrain, terrainFlags, absSpeedApprox);
	VehEmitter_TerrainEffects(thread, d, terrain, terrainFlags, absSpeedApprox);

	if ((terrainFlags & TERRAIN_FLAG_FORCE_SKIDMARKS) != 0)
	{
		d->actionsFlagSet |= ACTION_BACK_SKID | ACTION_FRONT_SKID;
	}

	if ((d->matrixArray != 0) && (d->matrixArray < 4))
	{
		if (d->matrixArray == 1)
		{
			d->actionsFlagSet |= ACTION_BACK_SKID;
		}

		d->actionsFlagSet &= ~ACTION_FRONT_SKID;
	}

	VehEmitter_SkidmarkAudio(thread, d, terrain, terrainFlags, absSpeedApprox);

	if (!VehEmitter_ShouldSkipExhaust(thread, d))
	{
		VehEmitter_ExhaustPair(thread, d);
	}

	if (d->burnTimer != 0)
	{
		d->alphaScaleBackup = 0x1000;
		d->instSelf->alphaScale = 0x1000;
	}

	if (d->invisibleTimer != 0)
	{
		thread->inst->alphaScale = 0x1000;
	}

	if ((d->kartState != KS_NORMAL) && (d->kartState != KS_DRIFTING))
	{
		d->actionsFlagSet &= ~ACTION_AIRBORNE;
	}

	if ((d->kartState == KS_ENGINE_REVVING) || (d->kartState == KS_MASK_GRABBED) || ((d->actionsFlagSet & ACTION_TOUCH_GROUND) != 0))
	{
		GAMEPAD_JogCon2(d, 0x27, 0);

		if (d->turnWobbleAngle == 0)
		{
			return;
		}

		int jogValue = ((sdata->gGT->timer & 3) == 0) ? 0x27 : 0xf0;
		GAMEPAD_JogCon2(d, jogValue, 0x100);
		return;
	}

	if (d->jump_LandingBoost < 0x80)
	{
		int jogValue = 0x12;

		if ((d->simpTurnState < 0) || ((jogValue = 0x22), (d->simpTurnState > 0)))
		{
			GAMEPAD_JogCon1(d, jogValue, 0x20);
		}
	}

	GAMEPAD_JogCon2(d, 0, 0);
}
