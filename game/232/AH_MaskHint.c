#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b3dd8-0x800b3f88.
void AH_MaskHint_Start(s16 hintId, u16 bool_interruptWarppad)
{
	int iVar3;
	int bitIndex;
	struct Driver *d;

	// copy parameters
	D232.maskWarppadBoolInterrupt = bool_interruptWarppad;
	D232.maskHintID = hintId;

	sdata->boolDraw3D_AdvMask = 1;

	struct AdvProgress *adv = &sdata->advProgress;
	bitIndex = (int)hintId + ADV_REWARD_FIRST_HINT;
	UNLOCK_ADV_BIT(adv->rewards, bitIndex);

	// If this is "welcome to adventure arena"
	if (hintId == ADV_MASK_HINT_ID_WELCOME_TO_ARENA)
	{
		// "Using a Warppad" and "Map Information"
		UNLOCK_ADV_BIT(adv->rewards, ADV_REWARD_HINT_USING_WARP_PAD);
		UNLOCK_ADV_BIT(adv->rewards, ADV_REWARD_HINT_MAP_INFORMATION);
	}

	d = sdata->gGT->drivers[0];
	d->funcPtrs[DRIVER_FUNC_INIT] = VehPhysProc_FreezeEndEvent_Init;

	// If Aku / Uka model pointer is nullptr
	if (sdata->modelMaskHints3D == NULL)
	{
		LOAD_TalkingMask(LOAD_GetAdvPackIndex(), (VehPickupItem_MaskBoolGoodGuy(d) & 0xffff) == 0);

		// 3.0s to spawn mask
		D232.maskSpawnFrame = 90;
	}

	// if model is not nullptr
	else
	{
		// 0.667s to spawn mask
		D232.maskSpawnFrame = 20;
	}

	iVar3 = (bool_interruptWarppad & 1) * 3;

	s16 *input = &D232.maskVars[0];

	D232.maskOffsetPos.x = input[iVar3 + 0];
	D232.maskOffsetPos.y = input[iVar3 + 1];
	D232.maskOffsetPos.z = input[iVar3 + 2];

	D232.maskOffsetRot.x = input[iVar3 + 6];
	D232.maskOffsetRot.y = input[iVar3 + 7];
	D232.maskOffsetRot.z = input[iVar3 + 8];

	for (int i = 0; i < 3; i++)
	{
		// 4 bytes for 4 volumes
		// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b3f3c-0x800b3f54 for mask-hint volume backup.
		D232.audioBackup[i] = howl_VolumeGet(i);
	}

	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b3f88-0x800b3f98.
int AH_MaskHint_boolCanSpawn()
{
	// 0 - aku is gone,
	// 1 - aku is speaking

	// return 0, if aku is speaking -- can't spawn
	// return 1, if aku is gone -- can spawn

	return sdata->AkuAkuHintState == 0;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b3f98-0x800b42b4.
void AH_MaskHint_SetAnim(int scale)
{
	MATRIX *m;
	struct GameTracker *gGT = sdata->gGT;
	struct PushBuffer *pb = &gGT->pushBuffer[0];

	m = &pb->matrix_Camera;
	gte_SetRotMatrix(m);
	gte_SetTransMatrix(m);

	CTR_GteLoadSVec3V0(&D232.maskOffsetPos);
	gte_rt();

	int posEndINT[3];
	SVec3 posEnd;

	CTR_GteStoreMAC(&posEndINT[0]);

	posEnd.x = posEndINT[0];
	posEnd.y = posEndINT[1];
	posEnd.z = posEndINT[2];

	SVec3 rotEnd;
	rotEnd.x = pb->rot.x - D232.maskOffsetRot.x;
	rotEnd.y = pb->rot.y + D232.maskOffsetRot.y;
	rotEnd.z = pb->rot.z - D232.maskOffsetRot.z;

	SVec3 posCurr;
	SVec3 rotCurr;
	CAM_ProcessTransition(&posCurr, &rotCurr, &D232.maskCamPosStart, &D232.maskCamRotStart, &posEnd, &rotEnd, scale);

	int rot = 0x1000;
	if (D232.maskSpawnFrame - 20 < D232.maskFrameCurr)
	{
		rot = ((D232.maskSpawnFrame - D232.maskFrameCurr) * rot) / 20;
	}

	// 4096->50
	rot = (rot * 50) >> 0xc;

	int angle = (scale << 0xf) >> 0xc;
	D232.maskAngle = angle;

	int sin = MATH_Sin(angle);
	int cos = MATH_Cos(angle);

	struct Instance *mhInst = sdata->instMaskHints3D;
	posCurr.x += (s16)((sin * rot) >> 0xc);
	posCurr.z += (s16)((cos * rot) >> 0xc);

	rotCurr.y += angle;
	ConvertRotToMatrix(&mhInst->matrix, &rotCurr);

	((struct MaskHint *)mhInst->thread->object)->scale = scale * 4 - 1;

	angle = (sdata->frameCounter + gGT->timer) * 0x20;
	sin = MATH_Sin(angle);
	posCurr.y += (s16)(((sin << 4) >> 0xc) * scale >> 0xc);

	mhInst->matrix.t[0] = posCurr.x;
	mhInst->matrix.t[1] = posCurr.y;
	mhInst->matrix.t[2] = posCurr.z;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b42b4-0x800b43cc.
void AH_MaskHint_SpawnParticles(s16 numParticles, struct ParticleEmitter *emSet, int maskAnim)

{
	struct Particle *particle;
	struct Instance *maskInst;
	int i, j;

	maskAnim = maskAnim + 0x1000;
	if (maskAnim > 0x3fff)
	{
		maskAnim = 0x3fff;
	}

	// "hubdustpuff"
	struct IconGroup *ig = sdata->gGT->iconGroup[0x10];

	// talking mask instance
	maskInst = sdata->instMaskHints3D;

	for (i = 0; i < numParticles; i++)
	{
		particle = Particle_Init(0, ig, emSet);
		if (particle == NULL)
		{
			continue;
		}

		for (j = 0; j < 3; j++)
		{
			particle->axis[j].startVal += maskInst->matrix.t[j] * 0x100;
		}

		particle->axis[5].startVal = (particle->axis[5].startVal * maskAnim) >> 0xc;
		particle->axis[5].velocity = (particle->axis[5].velocity * maskAnim) >> 0xc;

		particle->otIndexOffset -= 5;
	}

	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b43cc-0x800b4470.
void AH_MaskHint_LerpVol(int param_1)
{
	int diff;
	int volume;
	u8 backup;

	for (char i = 0; i < 3; i++)
	{
		backup = D232.audioBackup[i];

		diff = D232.maskAudioSettings[i] - backup;
		volume = backup + ((diff * param_1) >> 12);

		// restore backups of Volume settings,
		// that were originally saved in AH_MaskHint_Start
		howl_VolumeSet(i, volume & 0xFF);
	}
}

force_inline void AH_MaskHint_DrawRepeatPrompt(void)
{
	int lngIndex = 0;
	b32 boolFound = false;

	if (sdata->AkuAkuHintState != 5)
	{
		return;
	}

	s16 *ptrLngID = &D232.hintMenu_lngIndexArr[0];
	struct GameTracker *gGT = sdata->gGT;
	struct Driver *d = gGT->drivers[0];

	for (/**/; *ptrLngID > -1; ptrLngID++)
	{
		if (D232.maskHintID == (ptrLngID[0] - 0x17b) / 2)
		{
			boolFound = true;
			break;
		}
	}

	if (!boolFound)
	{
		return;
	}

	// Retail finds the hint subtitle entry above, but the shipped path draws a
	// generic "press start to repeat" prompt instead of that hint text.
	if (VehPickupItem_MaskBoolGoodGuy(d))
	{
		lngIndex = 0x177;
	}
	else
	{
		lngIndex = 0x232;
	}

	RECT r;
	r.x = -10;
	r.y = 0xb0;
	r.w = 0x214;
	r.h = 8 + DecalFont_DrawMultiLine(sdata->lngStrings[lngIndex], 0x100, 0xb4, 400, 2, 0xffff8000);

	RECTMENU_DrawInnerRect(&r, 4, gGT->backBuffer->otMem.uiOT);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 overlay 232 0x800b4470-0x800b4c80.
void AH_MaskHint_Update()
{
	struct GameTracker *gGT = sdata->gGT;
	struct Driver *d = gGT->drivers[0];
	struct CameraAngleAxisScratch angleAxisWork;
	SVec3 pos;
	SVec3 rot;

	switch (sdata->AkuAkuHintState - 1)
	{
	case 0:
		if (sdata->XA_State != 0)
		{
			return;
		}

		sdata->AkuAkuHintState++;
		break;

	case 1:
	{
		int absSpeedApprox = d->speedApprox;
		if (absSpeedApprox < 0)
		{
			absSpeedApprox = -absSpeedApprox;
		}
		if (absSpeedApprox > 0x31)
		{
			return;
		}

		if ((D232.maskWarppadBoolInterrupt & 1) == 0)
		{
			struct CameraDC *cdc = &gGT->cameraDC[0];

			CTR_COPY_VEC3(cdc->driverOffset_CamEyePos.v, D232.eyePos.v);

			CTR_COPY_VEC3(cdc->driverOffset_CamLookAtPos.v, D232.lookAtPos.v);

			cdc->flags |= 8;

			// NOTE(aalhendi): Retail passes a stack work buffer here, not 0x1f800108.
			CAM_FollowDriver_AngleAxis(cdc, d, &angleAxisWork, &pos, &rot);
			CAM_SetDesiredPosRot(cdc, &pos, &rot);
		}

		D232.maskWarppadDelayFrames = 60;

		sdata->AkuAkuHintState++;
	}
	break;

	case 2:

		if (((D232.maskWarppadBoolInterrupt & 1) == 0) && ((gGT->cameraDC[0].flags & CAMERA_FLAG_TRANSITION_HOLD) == 0) && (D232.maskSpawnFrame != 20))
		{
			return;
		}

		struct Instance *dInst = d->instSelf;
		sdata->instMaskHints3D = VehTalkMask_Init();
		struct Instance *mhInst = sdata->instMaskHints3D;

		CTR_MatrixToRot((SVECTOR *)&rot, &dInst->matrix, 0x11);

		// not a typo
		D232.maskCamRotStart.x = rot.y & 0xfff;
		D232.maskCamRotStart.z = rot.z & 0xfff;
		D232.maskCamRotStart.y = rot.x & 0xfff;

		CTR_COPY_VEC3(D232.maskCamPosStart.v, dInst->matrix.t);

		((struct MaskHint *)mhInst->thread->object)->scale = 0;

		AH_MaskHint_SetAnim(0);

		D232.maskFrameCurr = 0;

		sdata->AkuAkuHintState++;
		break;

	case 3:

		// first frame "whoosh" sound
		if (D232.maskFrameCurr == 0)
		{
			// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b46d4-0x800b46e4 for mask spawn-start SFX.
			OtherFX_Play_LowLevel(0x100, 1, HOWL_SFX_DEFAULT_FLAGS);
		}

		// if 3-second spawn, play more sounds
		if (D232.maskSpawnFrame == 0x5a)
		{
			if (D232.maskFrameCurr == 10)
			{
				// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b470c-0x800b4774 for mask spawn pulse 10 SFX.
				OtherFX_Play_LowLevel(0x100, 0, 0xd78a80);
			}
			else if (D232.maskFrameCurr == 20)
			{
				// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b4728-0x800b4774 for mask spawn pulse 20 SFX.
				OtherFX_Play_LowLevel(0x100, 1, 0xaf9480);
			}
			else if (D232.maskFrameCurr == 25)
			{
				// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b4744-0x800b4774 for mask spawn pulse 25 SFX.
				OtherFX_Play_LowLevel(0x100, 0, 0x879e80);
			}
			else if (D232.maskFrameCurr == 30)
			{
				// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b4760-0x800b4774 for mask spawn pulse 30 SFX.
				OtherFX_Play_LowLevel(0x100, 1, 0x5fa880);
			}
		}

		int timer4096 = (D232.maskFrameCurr << 0xc) / D232.maskSpawnFrame;

		AH_MaskHint_SetAnim(timer4096);

		AH_MaskHint_SpawnParticles(3, &D232.emSet_maskSpawn[0], timer4096);

		// if not finished spawning
		if (D232.maskFrameCurr < D232.maskSpawnFrame)
		{
			D232.maskFrameCurr++;

			timer4096 = (D232.maskFrameCurr << 0xc) / D232.maskSpawnFrame;

			AH_MaskHint_LerpVol(timer4096);
			break;
		}

		// NOTE(aalhendi): Retail only waits for the mask model pointer.
		if (sdata->modelMaskHints3D == 0)
		{
			AH_MaskHint_LerpVol(0x1000);
			break;
		}

		if (((D232.maskWarppadBoolInterrupt & 1) != 0) || ((gGT->cameraDC[0].flags & CAMERA_FLAG_TRANSITION_HOLD) != 0))
		{
			AH_MaskHint_LerpVol(0x1000);

			AH_MaskHint_SpawnParticles(0x18, &D232.emSet_maskLeave[0], 0x1000);

			VehTalkMask_PlayXA(sdata->instMaskHints3D, D232.maskHintID);

			if (((gGT->gameMode1 & ADVENTURE_ARENA) != 0) &&

			    // Not "Welcome to Adventure" or "You need a Boss Key"
			    (D232.maskHintID != 0) && (D232.maskHintID != 0x18))
			{
				// hide UI map
				gGT->hudFlags |= 0x10;
			}

			sdata->AkuAkuHintState++;
			break;
		}
		break;

	case 4:
	{
		// NOTE(aalhendi): Native draws only this shared prompt earlier from
		// AH_Map_Main so synchronous DrawOTag sees it; the rest of this state
		// remains retail-timed here.
#if !defined(CTR_NATIVE)
		AH_MaskHint_DrawRepeatPrompt();
#endif

		AH_MaskHint_SetAnim(0x1000);

		int bVar8;
		int uVar3 = D232.maskWarppadDelayFrames - 1;
		if ((((D232.maskWarppadDelayFrames == 0) || (bVar8 = D232.maskWarppadDelayFrames == 1, D232.maskWarppadDelayFrames = uVar3, bVar8)) &&
		     (((VehTalkMask_boolNoXA() != 0) || ((sdata->gGamepads->gamepad[0].buttonsTapped & BTN_TRIANGLE) != 0)))) &&
		    (sdata->AkuAkuHintState++,

		     // If you're in Adventure Arena
		     ((gGT->gameMode1 & ADVENTURE_ARENA) != 0)))
		{
			// show map again
			gGT->hudFlags &= ~(0x10);
		}
	}
	break;

	case 5:

		AH_MaskHint_SpawnParticles(20, &D232.emSet_maskLeave[0], 0x1000);

		// vanish sound
		// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b4b24-0x800b4b2c for mask vanish SFX.
		OtherFX_Play(0x101, 1);

		VehTalkMask_End();

		if ((D232.maskWarppadBoolInterrupt & 1) == 0)
		{
			// transition back to player
			gGT->cameraDC[0].flags |= CAMERA_FLAG_TRANSITION_BACK;
		}

		sdata->AkuAkuHintState++;
		break;

	case 6:

		AH_MaskHint_LerpVol(0x1000 - gGT->cameraDC[0].transitionBlend);

		if (((gGT->cameraDC[0].flags & CAMERA_FLAG_TRANSITION_AWAY) == 0) || ((D232.maskWarppadBoolInterrupt & 1) != 0))
		{
			AH_MaskHint_SetAnim(0);
			AH_MaskHint_LerpVol(0);

			D232.maskWarppadDelayFrames = 0;
			if ((D232.maskWarppadBoolInterrupt & 1) != 0)
			{
				D232.maskWarppadDelayFrames = 30;
			}

			sdata->AkuAkuHintState++;
		}
		break;

	case 7:

		AH_MaskHint_LerpVol(0);

		D232.maskWarppadDelayFrames--;

		if (D232.maskWarppadDelayFrames < 1)
		{
			RECTMENU_ClearInput();

			sdata->AkuAkuHintState = 0;
			sdata->boolDraw3D_AdvMask = 0;

			gGT->gameMode2 &= ~(VEH_FREEZE_DOOR);
			d->funcPtrs[DRIVER_FUNC_INIT] = VehPhysProc_Driving_Init;
		}

		break;
	}
}
