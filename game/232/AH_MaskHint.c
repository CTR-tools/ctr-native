#include <common.h>
#include <ctr_gte_transfer.h>

void AH_MaskHint_Start(s16 hintId, b16 bool_interruptWarppad)
{
	struct AdvProgress *adv;
	int bitIndex;
	struct Driver *d;
	int offsetSlot;
	const SVec3 *offset;
	s16 i;
	// copy parameters
	AH_HINT_VISIBLE = 1;
	AH_HINT_INTERRUPTS_WARPPAD = bool_interruptWarppad;
	AH_HINT_ID = hintId;

	d = GAME_TRACKER->drivers[0];
	d->funcPtrs[DRIVER_FUNC_INIT] = VehPhysProc_FreezeEndEvent_Init;

	adv = &GAME_ADV_PROGRESS;
	bitIndex = (int)hintId + ADV_REWARD_FIRST_HINT;
	UNLOCK_ADV_BIT(adv->rewards, bitIndex);

	// If this is "welcome to adventure arena"
	if (hintId == ADV_MASK_HINT_ID_WELCOME_TO_ARENA)
	{
		// "Using a Warppad" and "Map Information"
		UNLOCK_ADV_BIT(adv->rewards, ADV_REWARD_HINT_USING_WARP_PAD);
		UNLOCK_ADV_BIT(adv->rewards, ADV_REWARD_HINT_MAP_INFORMATION);
	}

	// If Aku / Uka model pointer is nullptr
	if (AH_HINT_MODEL == NULL)
	{
		s32 pack = LOAD_GetAdvPackIndex();
		LOAD_TalkingMask(pack, !(s16)VehPickupItem_MaskBoolGoodGuy(d));

		// 3.0s to spawn mask
		AH_MASK_SPAWN_FRAME = AH_MASKHINT_LONG_SPAWN_FRAMES;
	}

	// if model is not nullptr
	else
	{
		// 0.667s to spawn mask
		AH_MASK_SPAWN_FRAME = AH_MASKHINT_SHORT_SPAWN_FRAMES;
	}

	offsetSlot = bool_interruptWarppad & AH_MASKHINT_OFFSET_WARPPAD_INTERRUPT;

	offset = &AH_MASK_HINT_POSITIONS[offsetSlot];
	AH_MASK_OFFSET_POS.x = offset->x;
	AH_MASK_OFFSET_POS.y = offset->y;
	AH_MASK_OFFSET_POS.z = offset->z;
	offset = &AH_MASK_HINT_ROTATIONS[offsetSlot];
	AH_MASK_OFFSET_ROT.x = offset->x;
	AH_MASK_OFFSET_ROT.y = offset->y;
	AH_MASK_OFFSET_ROT.z = offset->z;

	{
		for (i = 0; i < 3; i++)
		{
			// 4 bytes for 4 volumes
			AH_MASK_AUDIO_BACKUP[i] = howl_VolumeGet(i);
		}
	}

	return;
}

b32 AH_MaskHint_boolCanSpawn(void)
{
	// 0 - aku is gone,
	// 1 - aku is speaking

	// return 0, if aku is speaking -- can't spawn
	// return 1, if aku is gone -- can spawn

	return sdata->AkuAkuHintState == 0;
}

void AH_MaskHint_SetAnim(int scale)
{
	SVec3 posEnd;
	SVec3 rotEnd;
	SVec3 posCurr;
	SVec3 rotCurr;
	Vec3 posEndInt;
	int rot;
	s32 radius;
	s32 frameCurrent;
	s32 frameSpawn;
	int angle;
	int sin;
	int cos;
	struct GameTracker *gGT = GAME_TRACKER;
	const MATRIX *cameraMatrix = &gGT->pushBuffer[0].matrix_Camera;

	CTR_GteSetRotMatrix(cameraMatrix);
	CTR_GteSetTransMatrix(cameraMatrix);
	MTC2((u16)AH_MASK_OFFSET_POS.x | ((u32)AH_MASK_OFFSET_POS.y << 16), 0);
	MTC2((u16)AH_MASK_OFFSET_POS.z, 1);
	CTR_GteLoadDelay();
	gte_rt();
	{
		s32 mac;
		CTR_GteReadDataDelayed(posEndInt.x, 25);
		CTR_GteReadDataDelayed(posEndInt.y, 26);
		mac = MFC2_S(27);
		posEndInt.z = mac;
	}

	posEnd.x = posEndInt.x;
	posEnd.y = posEndInt.y;
	posEnd.z = posEndInt.z;


	rotEnd.x = gGT->pushBuffer[0].rot.x - AH_MASK_OFFSET_ROT.x;
	rotEnd.y = gGT->pushBuffer[0].rot.y + AH_MASK_OFFSET_ROT.y;
	rotEnd.z = gGT->pushBuffer[0].rot.z - AH_MASK_OFFSET_ROT.z;


	CAM_ProcessTransition(&posCurr, &rotCurr, &AH_MASK_CAM_POS_START, &AH_MASK_CAM_ROT_START, &posEnd, &rotEnd, scale);

	frameCurrent = AH_MASK_FRAME_CURR;
	frameSpawn = AH_MASK_SPAWN_FRAME;
	if (frameSpawn - AH_MASKHINT_SPAWN_RING_FRAMES < frameCurrent)
	{
		rot = ((frameSpawn - frameCurrent) * AH_MASKHINT_FULL_BLEND) / AH_MASKHINT_SPAWN_RING_FRAMES;
	}
	else
	{
		rot = AH_MASKHINT_FULL_BLEND;
	}

	angle = CTR_MipsSll(scale, 15) >> 12;
	{
		const struct TrigTable *table = AH_TRIG_TABLE;
		sin = CTR_ReadU32AlignedLE(&table[angle & 0x3ff]);
	}
	AH_MASK_ANGLE = angle;
	if (angle & 0x400)
	{
		cos = (s16)sin;
		sin >>= 16;
		if (angle & 0x800)
			sin = -sin;
		else
			cos = -cos;
	}
	else
	{
		cos = sin >> 16;
		sin = (s32)((u32)sin << 16);
		sin >>= 16;
		if (angle & 0x800)
		{
			cos = -cos;
			sin = -sin;
		}
	}
	radius = (rot * AH_MASKHINT_SPAWN_SPIRAL_RADIUS) >> 0xc;
	rotCurr.y += AH_MASK_ANGLE;
	posCurr.x += (s16)((sin * radius) >> 0xc);
	posCurr.z += (s16)((cos * radius) >> 0xc);
	ConvertRotToMatrix(&AH_HINT_MASK->matrix, &rotCurr);

	((struct MaskHint *)AH_HINT_MASK->thread->object)->scale = scale * 4 - 1;

	angle = ((s16)AH_FRAME_COUNTER + GAME_TRACKER->timer) * 0x20;
	{
		const struct TrigTable *table = AH_TRIG_TABLE;
		sin = CTR_ReadU32AlignedLE(&table[angle & 0x3ff]);
	}
	// NOTE(aalhendi): Keep sign extension in each branch; factoring it out
	// changes GCC 2.8.1's reuse of the mask pointer across this calculation.
	if (!(angle & 0x400))
	{
		sin = (s32)((u32)sin << 16);
		sin >>= 16;
	}
	else
	{
		sin >>= 16;
	}
	if (angle & 0x800)
		sin = -sin;
	posCurr.y += (s16)((CTR_MipsSll(sin, 4) >> 12) * scale >> 12);

	{
		struct Instance *mhInst = AH_HINT_MASK;
		mhInst->matrix.t[0] = posCurr.x;
		mhInst->matrix.t[1] = posCurr.y;
		mhInst->matrix.t[2] = posCurr.z;
	}
}

void AH_MaskHint_SpawnParticles(s16 numParticles, struct ParticleEmitter *emSet, int maskAnim)

{
	struct Particle *particle;
	struct Instance *maskInst;
	s16 i;
	s32 scale;
	s32 velocity;

	maskAnim = maskAnim + AH_MASKHINT_FULL_BLEND;
	if (maskAnim > 0x3fff)
	{
		maskAnim = 0x3fff;
	}

	for (i = 0; i < numParticles; i++)
	{
		particle = Particle_Init(0, GAME_TRACKER->iconGroup[0x10], emSet);
		if (particle == NULL)
		{
			continue;
		}

		scale = particle->axis[5].startVal * maskAnim;
		velocity = particle->axis[5].velocity * maskAnim;
		maskInst = AH_HINT_MASK;
		particle->axis[0].startVal += maskInst->matrix.t[0] * 0x100;
		particle->axis[1].startVal += maskInst->matrix.t[1] * 0x100;
		particle->axis[2].startVal += maskInst->matrix.t[2] * 0x100;
		particle->axis[5].startVal = scale >> 0xc;
		particle->axis[5].velocity = velocity >> 0xc;
		particle->otIndexOffset -= 5;
	}

	return;
}

void AH_MaskHint_LerpVol(int blend)
{
	int diff;
	int volume;
	s32 backup;

	{
		s16 i;
		for (i = 0; i < 3; i++)
		{
			backup = AH_MASK_AUDIO_BACKUP[i];

			diff = AH_MASK_AUDIO_TARGET[i] - backup;
			volume = backup + ((diff * blend) >> 12);

			// restore backups of Volume settings,
			// that were originally saved in AH_MaskHint_Start
			howl_VolumeSet(i, volume & 0xFF);
		}
	}
}

static inline b32 AH_MaskHint_IsListed(void)
{
	s16 i;
	b32 found = 0;
	for (i = 0; AH_MASK_HINT_INDICES[i] >= 0; i++)
	{
		if (AH_HINT_ID == (AH_MASK_HINT_INDICES[i] - AH_HINTMENU_HINT_LNG_FIRST) / 2)
		{
			found = 1;
			break;
		}
	}
	return found;
}

static inline s32 AH_MaskHint_DrawPromptText(void)
{
	char **message;
	if ((s16)VehPickupItem_MaskBoolGoodGuy(GAME_TRACKER->drivers[0]))
		message = &GAME_LANGUAGE_STRINGS[LNG_AKU_HINT_REPEAT_INSTRUCTIONS];
	else
		message = &GAME_LANGUAGE_STRINGS[LNG_TO_HEAR_THIS_HINT_AGAIN_PRESS_THE_START];
	return DecalFont_DrawMultiLine(*message, 0x100, 0xb4, 400, FONT_SMALL, (s16)(JUSTIFY_CENTER | ORANGE));
}

#if defined(CTR_NATIVE)
// NOTE(aalhendi): Native submits the prompt before DrawOTag; state and audio
// progression still run later through AH_MaskHint_Update.
static void AH_MaskHint_DrawRepeatPrompt(void)
{
	RECT r;
	s32 height;
	if (AH_HINT_STATE != AH_MASKHINT_STATE_REPEAT_PROMPT || !AH_MaskHint_IsListed())
		return;
	height = AH_MaskHint_DrawPromptText();
	r.x = -10;
	r.w = 0x214;
	r.y = 0xb0;
	r.h = height + 8;
	RECTMENU_DrawInnerRect(&r, 4, GAME_TRACKER->backBuffer->otMem.uiOT);
}
#endif

void AH_MaskHint_Update(void)
{
	struct Driver *d = GAME_TRACKER->drivers[0];
	struct CameraDC *cdc = &GAME_TRACKER->cameraDC[0];

	// NOTE(aalhendi): These states are mutually exclusive. The camera work,
	// extracted rotation and prompt rectangle reuse the same stack workspace.
	union
	{
		struct CameraScratchWork camera;
		struct
		{
			SVECTOR matrixRot;
			RECT r;
		} ui;
	} work;
	SVec3 pos;
	SVec3 rot;
	switch ((s16)(AH_HINT_STATE - 1))
	{
		s32 timer4096;
	case 0:
		if (AH_MASK_XA_STATE != 0)
		{
			return;
		}

		AH_HINT_STATE++;
		break;

	case 1:
	{
		s32 absSpeedApprox = d->speedApprox;
		if (absSpeedApprox < 0)
		{
			absSpeedApprox = -absSpeedApprox;
		}
		if (absSpeedApprox > AH_MASKHINT_MAX_START_SPEED)
		{
			return;
		}

		if ((AH_HINT_INTERRUPTS_WARPPAD & 1) == 0)
		{
			struct GameTracker *gGT = GAME_TRACKER;
			SVec3 *dst;
			dst = &gGT->cameraDC[0].driverOffset_CamEyePos;
			dst->x = AH_MASK_EYE.x;
			dst->y = AH_MASK_EYE.y;
			dst->z = AH_MASK_EYE.z;
			dst = &gGT->cameraDC[0].driverOffset_CamLookAtPos;
			dst->x = AH_MASK_LOOK.x;
			dst->y = AH_MASK_LOOK.y;
			dst->z = AH_MASK_LOOK.z;
			gGT->cameraDC[0].flags |= 8;

			// NOTE(aalhendi): Retail passes a stack work buffer here, not 0x1f800108.
			CAM_FollowDriver_AngleAxis(&gGT->cameraDC[0], gGT->drivers[0], CameraScratchWork_AsAngleAxis(&work.camera), &pos, &rot);
			CAM_SetDesiredPosRot(&GAME_TRACKER->cameraDC[0], &pos, &rot);
		}

		AH_MASK_DELAY = AH_MASKHINT_CAMERA_DELAY_FRAMES;

		AH_HINT_STATE++;
	}
	break;

	case 2:
	{
		if (((AH_HINT_INTERRUPTS_WARPPAD & 1) == 0) && ((cdc->flags & CAMERA_FLAG_TRANSITION_HOLD) == 0) &&
		    (AH_MASK_SPAWN_FRAME != AH_MASKHINT_SHORT_SPAWN_FRAMES))
		{
			return;
		}

		AH_HINT_MASK = VehTalkMask_Init();


		CTR_MatrixToRot(&work.ui.matrixRot, &d->instSelf->matrix, 0x11);

		// NOTE(aalhendi): Camera pitch and yaw use the opposite matrix axes.
		AH_MASK_CAM_ROT_START.x = work.ui.matrixRot.vy & 0xfff;
		AH_MASK_CAM_ROT_START.y = work.ui.matrixRot.vx & 0xfff;
		AH_MASK_CAM_ROT_START.z = work.ui.matrixRot.vz & 0xfff;

		AH_MASK_CAM_POS_START.x = d->instSelf->matrix.t[0];
		AH_MASK_CAM_POS_START.y = d->instSelf->matrix.t[1];
		AH_MASK_CAM_POS_START.z = d->instSelf->matrix.t[2];

		((struct MaskHint *)AH_HINT_MASK->thread->object)->scale = 0;

		AH_MaskHint_SetAnim(0);

		AH_MASK_FRAME_CURR = 0;

		AH_HINT_STATE++;
		break;
	}
	case 3:
	{
		s32 *frame = &AH_MASK_FRAME_CURR;

		// first frame "whoosh" sound
		if ((*frame) == 0)
		{
			OtherFX_Play_LowLevel(AH_MASKHINT_SFX_SPAWN, 1, HOWL_SFX_DEFAULT_FLAGS);
		}

		// if 3-second spawn, play more sounds
		if (AH_MASK_SPAWN_FRAME == AH_MASKHINT_LONG_SPAWN_FRAMES)
		{
			if ((*frame) == 10)
			{
				OtherFX_Play_LowLevel(AH_MASKHINT_SFX_SPAWN, 0, 0xd78a80);
			}
			else if ((*frame) == 20)
			{
				OtherFX_Play_LowLevel(AH_MASKHINT_SFX_SPAWN, 1, 0xaf9480);
			}
			else if ((*frame) == 25)
			{
				OtherFX_Play_LowLevel(AH_MASKHINT_SFX_SPAWN, 0, 0x879e80);
			}
			else if ((*frame) == 30)
			{
				OtherFX_Play_LowLevel(AH_MASKHINT_SFX_SPAWN, 1, 0x5fa880);
			}
		}

		timer4096 = CTR_MipsSll(AH_MASK_FRAME_CURR, 12) / AH_MASK_SPAWN_FRAME;

		AH_MaskHint_SetAnim(timer4096);

		AH_MaskHint_SpawnParticles(AH_MASKHINT_SPAWN_PARTICLES, &AH_MASK_SPAWN_EMITTERS[0], CTR_MipsSll(AH_MASK_FRAME_CURR, 12) / AH_MASK_SPAWN_FRAME);

		if (AH_HINT_MODEL != 0 && AH_MASK_FRAME_CURR >= AH_MASK_SPAWN_FRAME &&
		    ((cdc->flags & CAMERA_FLAG_TRANSITION_HOLD) != 0 || (AH_HINT_INTERRUPTS_WARPPAD & 1)))
		{
			AH_MaskHint_LerpVol(AH_MASKHINT_FULL_BLEND);
			AH_MaskHint_SpawnParticles(AH_MASKHINT_LEAVE_PARTICLES, &AH_MASK_LEAVE_EMITTERS[0], AH_MASKHINT_FULL_BLEND);
			VehTalkMask_PlayXA(AH_HINT_MASK, AH_HINT_ID);
			if ((GAME_TRACKER->gameMode1 & ADVENTURE_ARENA) && AH_HINT_ID != ADV_MASK_HINT_ID_WELCOME_TO_ARENA &&
			    AH_HINT_ID != ADV_MASK_HINT_ID_MAP_INFORMATION)
				GAME_TRACKER->hudFlags |= HUD_FLAG_HIDE_ADVENTURE_MAP;
			AH_HINT_STATE++;
			break;
		}
		if (AH_MASK_FRAME_CURR < AH_MASK_SPAWN_FRAME)
			AH_MASK_FRAME_CURR++;
		AH_MaskHint_LerpVol(CTR_MipsSll(AH_MASK_FRAME_CURR, 12) / AH_MASK_SPAWN_FRAME);
		break;
	}
	case 4:
	{
		b32 delayComplete;
		// NOTE(aalhendi): Native draws only this shared prompt earlier from
		// AH_Map_Main so synchronous DrawOTag sees it; the rest of this state
		// remains retail-timed here.
#if !defined(CTR_NATIVE)
		{
			b32 found = AH_MaskHint_IsListed();
			// NOTE(aalhendi): Keep the search result separate from its loop flag.
			CTR_PSX_KEEP_VALUE_RELAXED(found);
			if (found)
			{
				s32 height = AH_MaskHint_DrawPromptText();
				work.ui.r.x = -10;
				work.ui.r.w = 0x214;
				work.ui.r.y = 0xb0;
				work.ui.r.h = height + 8;
				RECTMENU_DrawInnerRect(&work.ui.r, 4, GAME_TRACKER->backBuffer->otMem.uiOT);
			}
		}
#endif

		AH_MaskHint_SetAnim(AH_MASKHINT_FULL_BLEND);

		delayComplete = AH_MASK_DELAY == 0;
		if (!delayComplete)
		{
			AH_MASK_DELAY--;
			delayComplete = AH_MASK_DELAY == 0;
		}

		if ((delayComplete && ((s16)VehTalkMask_boolNoXA() || ((GAMEPADS->gamepad[0].buttonsTapped & BTN_TRIANGLE) != 0))) &&
		    (AH_HINT_STATE++,

		     // If you're in Adventure Arena
		     ((GAME_TRACKER->gameMode1 & ADVENTURE_ARENA) != 0)))
		{
			// show map again
			GAME_TRACKER->hudFlags &= ~HUD_FLAG_HIDE_ADVENTURE_MAP;
		}
	}
	break;

	case 5:

		AH_MaskHint_SpawnParticles(AH_MASKHINT_VANISH_PARTICLES, &AH_MASK_LEAVE_EMITTERS[0], AH_MASKHINT_FULL_BLEND);

		// vanish sound
		OtherFX_Play(AH_MASKHINT_SFX_VANISH, 1);

		VehTalkMask_End();

		if ((AH_HINT_INTERRUPTS_WARPPAD & 1) == 0)
		{
			// transition back to player
			cdc->flags |= CAMERA_FLAG_TRANSITION_BACK;
		}

		AH_HINT_STATE++;
		break;

	case 6:

		AH_MaskHint_LerpVol(AH_MASKHINT_FULL_BLEND - GAME_TRACKER->cameraDC[0].transitionBlend);

		if (((cdc->flags & CAMERA_FLAG_TRANSITION_AWAY) == 0) || ((AH_HINT_INTERRUPTS_WARPPAD & 1) != 0))
		{
			AH_MaskHint_SetAnim(0);
			AH_MaskHint_LerpVol(0);

			{
				s32 delay = 0;
				if ((AH_HINT_INTERRUPTS_WARPPAD & 1) != 0)
					delay = AH_MASKHINT_INTERRUPT_DONE_DELAY_FRAMES;
				AH_MASK_DELAY = delay;
			}

			AH_HINT_STATE++;
		}
		break;

	case 7:

		AH_MaskHint_LerpVol(0);

		AH_MASK_DELAY--;

		if (AH_MASK_DELAY < 1)
		{
			RECTMENU_ClearInput();

			AH_HINT_STATE = 0;
			AH_HINT_VISIBLE = 0;

			GAME_TRACKER->gameMode2 &= ~(VEH_FREEZE_DOOR);
			d->funcPtrs[DRIVER_FUNC_INIT] = VehPhysProc_Driving_Init;
		}

		break;
	}
}
