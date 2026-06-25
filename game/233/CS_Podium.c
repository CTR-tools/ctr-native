#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800ac714-0x800ac840
void CS_DestroyPodium_StartDriving(void)
{
	struct Instance *inst;
	struct Driver *d;
	struct GameTracker *gGT = sdata->gGT;
	struct Thread *t = gGT->threadBuckets[OTHER].thread;

	// enable HUD
	gGT->hudFlags |= 1;

	// loop through all threads
	while (t != NULL)
	{
		if (t->funcThDestroy != CS_Podium_Prize_ThDestroy)
		{
			t->flags |= THREAD_FLAG_DEAD;
		}

		t = t->siblingThread;
	}

	d = gGT->drivers[0];

	// enable collisions for thread,
	// and make instance visible
	inst = d->instSelf;
	inst->thread->flags &= ~THREAD_FLAG_DISABLE_COLLISION;
	inst->flags &= ~(HIDE_MODEL);

	d->kartState = KS_ENGINE_REVVING;
	d->funcPtrs[DRIVER_FUNC_INIT] = VehPhysProc_Driving_Init;

	// if cutscene changed audio, restore backup
	if (D233.CutsceneManipulatesAudio != 0)
	{
		// restore backup of volume variables
		howl_VolumeSet(0, D233.FXVolumeBackup);
		howl_VolumeSet(1, D233.MusicVolumeBackup);
		howl_VolumeSet(2, D233.VoiceVolumeBackup);
	}

	// cam mode be zero to follow you
	gGT->cameraDC[0].cameraMode = 0;
	gGT->pushBuffer[0].distanceToScreen_PREV = 0x100;
	gGT->pushBuffer[0].distanceToScreen_CURR = 0x100;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b021c-0x800b0248
void CS_Podium_Stand_ThTick(struct Thread *t)
{
	if (D233.isCutsceneOver != 0)
	{
		t->flags |= THREAD_FLAG_DEAD;
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b0248-0x800b0300
void CS_Podium_Stand_Init(s16 *podiumData)
{
	struct Instance *inst = INSTANCE_BirthWithThread(STATIC_PODIUM, R233.s_podium, SMALL, OTHER, CS_Podium_Stand_ThTick, 0, 0);

	// if the instance was built
	if (inst == NULL)
	{
		return;
	}

	// set funcThDestroy to remove instance from instance pool
	inst->thread->funcThDestroy = PROC_DestroyInstance;

	inst->matrix.t[0] = podiumData[0];
	inst->matrix.t[1] = podiumData[1];
	inst->matrix.t[2] = podiumData[2];

	inst->depthBiasSecondary += 2;
	inst->depthBiasNormal += 2;

	podiumData[12] = podiumData[8];
	podiumData[13] = podiumData[9];
	podiumData[14] = podiumData[10];

	ConvertRotToMatrix(&inst->matrix, (const SVec3 *)&podiumData[12]);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800af7c0-0x800af994
void CS_Podium_Prize_Spin(struct Instance *inst, s16 *prize)
{
	struct GamepadSystem *gGS;
	u32 trigApprox;
	s16 prevAngle;
	int ratio;
	u32 angle;
	SVec3 lightDir;

	prize[5] += 100;
	const SVec3 *prizeRot = (const SVec3 *)&prize[4];
	ConvertRotToMatrix(&inst->matrix, prizeRot);

	gGS = sdata->gGamepads;

	if ((inst->flags & USE_SPECULAR_LIGHT) == 0)
	{
		return;
	}

	prevAngle = prize[0x10];
	prize[0x10] = prevAngle + 0x3f;

	if ((gGS->gamepad[1].buttonsHeldCurrFrame & BTN_L1) != 0)
	{
		prize[0x10] = prevAngle;
	}

	ratio = (prize[0x10] & 0xfff) - 0x800;
	if (ratio < 0)
	{
		ratio = -ratio;
	}

	angle = prize[0xc] + (((prize[0xe] - prize[0xc]) * ratio) >> 11);

	{
		s16 sine1;
		s16 cos1;

		trigApprox = *(u32 *)&data.trigApprox[angle & 0x3ff];
		if ((angle & 0x400) == 0)
		{
			cos1 = (s16)(trigApprox >> 16);
			sine1 = (s16)trigApprox;
		}
		else
		{
			cos1 = -(s16)trigApprox;
			sine1 = (s16)(trigApprox >> 16);
		}
		if ((angle & 0x800) != 0)
		{
			cos1 = -cos1;
			sine1 = -sine1;
		}
		lightDir.y = cos1;

		ratio = (prize[0x10] & 0xfff) - 0x800;
		if (ratio < 0)
		{
			ratio = -ratio;
		}

		angle = prize[0xd] + (((prize[0xf] - prize[0xd]) * ratio) >> 11);

		s16 sine2;
		s16 cos2;

		trigApprox = *(u32 *)&data.trigApprox[angle & 0x3ff];
		if ((angle & 0x400) == 0)
		{
			cos2 = (s16)(trigApprox >> 16);
			sine2 = (s16)trigApprox;
		}
		else
		{
			cos2 = -(s16)trigApprox;
			sine2 = (s16)(trigApprox >> 16);
		}
		if ((angle & 0x800) != 0)
		{
			cos2 = -cos2;
			sine2 = -sine2;
		}
		lightDir.x = (sine1 * cos2) >> 12;
		lightDir.z = (sine1 * sine2) >> 12;
	}

	Vector_SpecLightSpin3D(inst, prizeRot, &lightDir);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800af994-0x800afbc8
void CS_Podium_Prize_ThTick3(struct Thread *th)
{
	struct GameTracker *gGT;
	struct Instance *inst = th->inst;
	s16 *prize = th->object;
	s16 framesLeft;

	framesLeft = prize[0x13] - 1;
	prize[0x13] = framesLeft;

	if (framesLeft != 0)
	{
		int frameMax = prize[0x14];
		int xInterp = framesLeft * (0x100 - prize[8]);
		int yInterp = framesLeft * (0x6c - prize[9]);
		int x;
		int y;
		s16 scale;

		x = (prize[8] + xInterp / frameMax - 0x100) * -inst->matrix.t[2];
		if (x < 0)
		{
			x += 0xff;
		}

		inst->matrix.t[0] = x >> 8;

		y = (prize[9] + yInterp / frameMax - 0x6c) * inst->matrix.t[2];
		if (y < 0)
		{
			y += 0xff;
		}

		inst->matrix.t[1] = y >> 8;

		scale = inst->scale.x - 0x4b0;
		if (scale < 0x1001)
		{
			scale = 0x1000;
		}

		inst->scale.x = scale;
		inst->scale.y = scale;
		inst->scale.z = scale;

		CS_Podium_Prize_Spin(inst, prize);
		return;
	}

	if (CS_Camera_BoolGotoBoss() == 0)
	{
		u32 rewards = sdata->advProgress.hintFlags;
		s16 hintID = 0;

		if ((rewards & ADV_REWARD_HINT_MAP_INFORMATION_MASK) == 0)
		{
			hintID = ADV_MASK_HINT_ID_MAP_INFORMATION;
		}
		else if ((rewards & ADV_REWARD_HINT_WUMPA_FRUIT_MASK) == 0)
		{
			hintID = ADV_MASK_HINT_ID_WUMPA_FRUIT;
		}
		else if ((rewards & ADV_REWARD_HINT_TNT_MASK) == 0)
		{
			hintID = ADV_MASK_HINT_ID_TNT;
		}
		else if ((rewards & ADV_REWARD_HINT_HANG_TIME_TURBO_MASK) == 0)
		{
			hintID = ADV_MASK_HINT_ID_HANG_TIME_TURBO;
		}
		else if ((rewards & ADV_REWARD_HINT_POWER_SLIDE_MASK) == 0)
		{
			hintID = ADV_MASK_HINT_ID_POWER_SLIDE;
		}
		else if ((rewards & ADV_REWARD_HINT_TURBO_BOOST_MASK) == 0)
		{
			hintID = ADV_MASK_HINT_ID_TURBO_BOOST;
		}
		else if ((rewards & ADV_REWARD_HINT_BRAKE_SLIDE_MASK) == 0)
		{
			hintID = ADV_MASK_HINT_ID_BRAKE_SLIDE;
		}

		if (hintID != 0)
		{
			MainFrame_RequestMaskHint(hintID, 0);
		}
	}

	gGT = sdata->gGT;
	gGT->overlayTransition = 2;
	gGT->gameMode2 &= ~VEH_FREEZE_PODIUM;

	OtherFX_Play(0x67, 1);

	th->flags |= THREAD_FLAG_DEAD;
}

// Make the trophy bounce 3 times
// Then start ThTick3
// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800afbc8-0x800afcc4
void CS_Podium_Prize_ThTick2(struct Thread *th)
{
	int currScale;

	// get object from thread
	// should replace with struct Prize in 233
	s16 *prize = th->object;

	// get instance from thread
	struct Instance *inst = th->inst;

	s16 frameIndex = prize[0x15];

	// bouncing scale animation
	if (frameIndex < 5)
	{
		// if even frame
		if ((frameIndex & 1) == 0)
		{
			// scaleX
			currScale = inst->scale.x + 800 + frameIndex * 400;

			if ((frameIndex + 1) * 0x28a + 0x2000 < currScale)
			{
				// frame counter
				frameIndex += 1;
			}
		}
		else
		{
			// scaleX
			currScale = inst->scale.x - 800;

			if (currScale < 0x1001)
			{
				// frame counter
				frameIndex += 1;
			}
		}

		prize[0x15] = frameIndex;

		// scaleY and scaleZ
		inst->scale.x = currScale;
		inst->scale.y = currScale;
		inst->scale.z = currScale;

		CS_Podium_Prize_Spin(inst, prize);
	}
	else
	{
		ThTick_SetAndExec(th, CS_Podium_Prize_ThTick3);
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800afcc4-0x800afe58
void CS_Podium_Prize_ThTick1(struct Thread *th)
{
	struct Instance *inst = th->inst;
	s16 *prize = th->object;
	int trig;

	if (D233.PodiumInitUnk3 != 0)
	{
		if (th->modelIndex != STATIC_BIG1)
		{
			inst->flags &= ~HIDE_MODEL;
		}

		prize[0x12] = VehCalc_InterpBySpeed(prize[0x12], 0x14, 0);
		prize[0x11] = VehCalc_InterpBySpeed(prize[0x11], 1, 0);
	}

	trig = MATH_Sin(prize[5]);
	inst->matrix.t[0] = prize[0] + ((prize[0x11] * trig) >> 12);
	inst->matrix.t[1] = prize[1] + prize[0x12];

	trig = MATH_Cos(prize[5]);
	inst->matrix.t[2] = prize[2] + ((prize[0x11] * trig) >> 12);

	if (D233.isCutsceneOver == 0)
	{
		CS_Podium_Prize_Spin(inst, prize);
		return;
	}

	prize[0x14] = 0xf;
	prize[0x13] = 0xf;
	prize[0x15] = 0;

	inst->depthBiasNormal = 0x80;
	inst->depthBiasSecondary = 0x80;

	inst->scale.x = 0x1000;
	inst->scale.y = 0x1000;
	inst->scale.z = 0x1000;

	inst->matrix.t[0] = 0;
	inst->matrix.t[1] = 0;
	inst->matrix.t[2] = prize[10];

	{
		struct InstDrawPerPlayer *idpp = INST_GETIDPP(inst);
		idpp[0].pushBuffer = &sdata->gGT->pushBuffer_UI;
	}

	OtherFX_Stop2(0xaf);
	OtherFX_Stop2(0xae);
	OtherFX_Play(0x9a, 1);

	ThTick_SetAndExec(th, CS_Podium_Prize_ThTick2);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800afe58-0x800afe90
void CS_Podium_Prize_ThDestroy(struct Thread *t)
{
	// remove bits
	sdata->gGT->gameMode2 &= ~(INC_RELIC | INC_KEY | INC_TROPHY);
	PROC_DestroyInstance(t);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800afe90-0x800b021c
void CS_Podium_Prize_Init(u32 prizeModel, const char *prizeName, s16 *posOnScreen)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Instance *inst;
	s16 *prize;
	int tx;
	int ty;
	int tz;

	inst = INSTANCE_BirthWithThread(prizeModel, prizeName, MEDIUM, OTHER, CS_Podium_Prize_ThTick1, 0x2c, NULL);

	if (inst == NULL)
	{
		if (D233.cutsceneState < CS_WAIT_INPUT)
		{
			D233.cutsceneState = CS_WAIT_INPUT;
		}

		gGT->gameMode2 &= ~VEH_FREEZE_PODIUM;
		return;
	}

	inst->scale.x = 0x2000;
	inst->scale.y = 0x2000;
	inst->scale.z = 0x2000;
	inst->flags |= HIDE_MODEL;

	prize = inst->thread->object;
	inst->thread->funcThDestroy = CS_Podium_Prize_ThDestroy;

	prize[0x11] = 0x40;
	prize[0x12] = 0x200;
	prize[4] = 0;
	prize[5] = 0;
	prize[6] = 0;

	MTC2(0, 0);
	MTC2(0x40, 1);
	gte_llv0();

	tx = MFC2(25);
	ty = MFC2(26);
	tz = MFC2(27);

	prize[0] = posOnScreen[0] + (s16)tx;
	prize[1] = posOnScreen[1] + (s16)ty + 0x1c0;
	prize[2] = posOnScreen[2] + (s16)tz;
	prize[10] = -0x200;

	switch (prizeModel)
	{
	case STATIC_BIG1:
		inst->flags |= HIDE_MODEL;
		goto center_target;

	case STATIC_GEM:
	{
		s16 *gemColor = data.AdvCups[gGT->cup.cupID].color;

		inst->colorRGBA = (gemColor[0] << 20) | (gemColor[1] << 12) | (gemColor[2] << 4);
		prize[0xc] = 0x5d3;
		prize[0xd] = 0x718;
		prize[0xe] = 0x590;
		prize[0xf] = 0x609;
		inst->flags |= USE_SPECULAR_LIGHT;
		goto center_target;
	}

	default:
	center_target:
		prize[8] = 0x100;
		prize[9] = 0x6c;
		return;

	case STATIC_RELIC:
	{
		struct UiElement2D *hud = data.hudStructPtr[0];
		u32 bitIndex = gGT->prevLEV + ADV_REWARD_FIRST_PLATINUM_RELIC;
		u32 relicColor;

		prize[8] = hud[0xe].x;
		prize[9] = hud[0xe].y - 0x3c;

		if (CHECK_ADV_BIT(sdata->advProgress.rewards, bitIndex) == 0)
		{
			bitIndex = gGT->prevLEV + ADV_REWARD_FIRST_GOLD_RELIC;

			if (CHECK_ADV_BIT(sdata->advProgress.rewards, bitIndex) == 0)
			{
				relicColor = 0x20a5ff0;
			}
			else
			{
				relicColor = 0xd8d2090;
			}
		}
		else
		{
			relicColor = 0xffede90;
		}

		inst->colorRGBA = relicColor;
		prize[0xc] = 0x2ab;
		prize[0xd] = 0x436;
		prize[0xe] = 0x1eb;
		prize[0xf] = 0x670;
		inst->flags |= USE_SPECULAR_LIGHT;

		gGT->gameMode2 |= INC_RELIC;
		return;
	}

	case STATIC_TROPHY:
	{
		struct UiElement2D *hud = data.hudStructPtr[0];

		prize[8] = hud[0x10].x;
		prize[9] = hud[0x10].y - 0x3c;
		prize[10] = -200;

		inst->scale.x = 0x4000;
		inst->scale.y = 0x4000;
		inst->scale.z = 0x4000;

		gGT->gameMode2 |= INC_TROPHY;
		return;
	}

	case STATIC_KEY:
	{
		struct UiElement2D *hud = data.hudStructPtr[0];

		inst->colorRGBA = 0xdca6000;
		prize[0xc] = 0x1d9;
		prize[0xd] = 0x5db;
		prize[0xe] = 0x2da;
		prize[0xf] = 0x54b;
		inst->flags |= USE_SPECULAR_LIGHT;

		prize[8] = hud[0xf].x;
		prize[9] = hud[0xf].y - 0x3c;

		gGT->gameMode2 |= INC_KEY;
		return;
	}
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b0300-0x800b06ac
void CS_Podium_FullScene_Init(void)
{
	struct Instance *driverInstSelf;
	struct Thread *victoryCamThread;
	u32 podiumMusic;
	struct CsThreadInitData InitData = {0};

	struct SpawnPosRot *posRot;

	struct GameTracker *gGT = sdata->gGT;

	// assume cutscene did not manipulate audio
	D233.CutsceneManipulatesAudio = 0;

	// Make a backup of FX volume, clamp to 0x100
	D233.FXVolumeBackup = howl_VolumeGet(0);
	D233.FXVolumeBackup &= 0xff;

	// Make a backup of Music volume, clamp to 0x100
	D233.MusicVolumeBackup = howl_VolumeGet(1);
	D233.MusicVolumeBackup &= 0xff;

	// Make a backup of Voice volume, clamp to 0x100
	D233.VoiceVolumeBackup = howl_VolumeGet(2);
	D233.VoiceVolumeBackup &= 0xff;

	// Cutscene is now starting
	D233.isCutsceneOver = 0;
	D233.cutsceneState = CS_CAMERA_PAN;

	D233.PodiumInitUnk3 = 0;

	driverInstSelf = gGT->drivers[0]->instSelf;

	D233.PodiumInitUnk2 = 0;

	driverInstSelf->flags |= HIDE_MODEL;

	VehPhysProc_FreezeEndEvent_Init(driverInstSelf->thread, gGT->drivers[0]);

	// Number of Winners = 1
	// this means Draw Confetti on one window
	gGT->numWinners = 1;

	// Set winnerIndex[0] to 0, to draw
	// confetti on the first pushBuffer
	gGT->winnerIndex[0] = 0;

	gGT->confetti.numParticles_max = 200;
	gGT->confetti.unk2 = 200;
	gGT->hudFlags &= 0xfe;

	// Draw Confetti
	gGT->renderFlags |= 4;

	gGT->gameMode2 |= VEH_FREEZE_PODIUM;

	// position and rotation of podium scene
	// Y coordinate (podiumPos.y) has added height
	posRot = gGT->level1->ptrSpawnType2_PosRot[1].posRot;
	InitData.podiumPos.x = posRot->pos.x;
	InitData.podiumPos.y = posRot->pos.y + 0x80;
	InitData.podiumPos.z = posRot->pos.z;
	InitData.rot.x = posRot->rot.x;
	InitData.rot.y = posRot->rot.y;
	InitData.rot.z = posRot->rot.z;

	// convert 3 rotation shorts into rotation matrix
	ConvertRotToMatrix((MATRIX *)&InitData.local_30, &InitData.rot.vec);
	// Move position of trophy girl
	gte_SetLightMatrix(&InitData.local_30);

	// CameraDC, this makes the camera stop following you as it does while racing, it must be zero to follow you
	gGT->cameraDC[0].cameraMode = 3;

	// if someone placed third
	if (gGT->podium_modelIndex_Third != '\0')
	{
		InitData.characterPos.x = 299;
		InitData.characterPos.y = 0xffab;
		InitData.characterPos.z = 0;

		// create thread for "third"
		CS_Thread_Init(gGT->podium_modelIndex_Third, &R233.s_third[0], &InitData, 0x600, 0);
	}

	// if someone placed second
	if (gGT->podium_modelIndex_Second != '\0')
	{
		InitData.characterPos.x = 0xfed5;
		InitData.characterPos.y = 0xffd6;
		InitData.characterPos.z = 0;

		// create thread for "second"
		CS_Thread_Init(gGT->podium_modelIndex_Second, &R233.s_second[0], &InitData, 0x200, 0);
	}

	InitData.characterPos.x = 0;
	InitData.characterPos.y = 0;
	InitData.characterPos.z = 0;

	// create thread for "first"
	CS_Thread_Init(gGT->podium_modelIndex_First, &R233.s_first[0], &InitData, 0, 0);

	InitData.characterPos.x = 0x1a8;
	InitData.characterPos.y = 0xff80;
	InitData.characterPos.z = 0x140;

	// create thread for trophy girl (internally called "tawna")
	CS_Thread_Init(gGT->podium_modelIndex_tawna, &R233.s_tawna[0], &InitData, -0x2aa, 0);

	CS_Podium_Prize_Init(gGT->podiumRewardID, &R233.s_prize[0], (void *)&InitData);

	CS_Podium_Stand_Init((void *)&InitData);

	// PROC_BirthWithObject
	// 0x4 = size
	// 0 = no relation to param4
	// 0x300 flag = SmallStackPool
	// 0xf = camera thread bucket
	victoryCamThread = (struct Thread *)PROC_BirthWithObject(0x4030f, (void *)CS_Camera_ThTick_Podium, R233.s_victorycam, NULL);

	// if it allocated correctly
	if (victoryCamThread != 0)
	{
		// initialize first "s16" of the object to zero
		*(s16 *)victoryCamThread->object = 0;
	}

	// change victory music based on who is first in the podium
	switch (gGT->podium_modelIndex_First - 0x7e)
	{
	// Crash, Coco, Fake Crash
	case 0:
	case 3:
	case 0xE:
		// Crash's music
		podiumMusic = 10;
		break;

	// Cortex, NGin, NTrophy
	case 1:
	case 4:
	case 0xC:
		// Cortex's music
		podiumMusic = 8;
		break;

	// Polar Pura
	case 6:
	case 7:
		// Polar and Pura's music
		podiumMusic = 7;
		break;

	// pinstripe kjoe
	case 8:
	case 0xb:
		// Pinstripe's music
		podiumMusic = 0xb;
		break;

	// papu, roo, penta
	case 9:
	case 10:
	case 0xD:
		// Ripper Roo's music
		podiumMusic = 9;
		break;

	// Tiny, Dingo, Oxide
	default:
		// Default music is Tiny Tiger's
		podiumMusic = 0xc;
		break;
	}

	CDSYS_XAPlay(0, podiumMusic);

	return;
}
