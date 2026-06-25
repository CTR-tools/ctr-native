#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800aed48-0x800aedf8
u8 CS_Camera_BoolGotoBoss(void)
{
	struct GameTracker *gGT = sdata->gGT;

	// If just got 18th relic
	if ((gGT->podiumRewardID == STATIC_RELIC) && (gGT->currAdvProfile.numRelics >= 18))
	{
		// If Oxide was not beaten twice yet
		if (CHECK_ADV_BIT(sdata->advProgress.rewards, ADV_REWARD_BEAT_OXIDE_SECOND) == 0)
		{
			return 1;
		}
	}

	// If just unlocked Key
	if (gGT->podiumRewardID == STATIC_KEY)
	{
		return 1;
	}

	struct Instance *inst = gGT->drivers[0]->instSelf;
	const SVec3 *podiumPos = &gGT->level1->ptrSpawnType2_PosRot[1].posRot->pos;

	// TRUE if TeleportSelf did NOT spawn on podium (goto boss door)
	return (inst->matrix.t[0] != podiumPos->x) || (inst->matrix.t[2] != podiumPos->z);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800ae9a8-0x800aed48
void CS_Camera_ThTick_Boss(struct Thread *t)
{
	char i;

	int cutsceneID;
	s16 levID;

	struct Instance *inst;
	struct CutsceneObj *cs;
	struct GameTracker *gGT;

	gGT = sdata->gGT;
	levID = gGT->levelID;

	// for undecided cutscene:
	//	hub*2+0 - intro (after winning trophy)
	//	hub*2+1 - outro (after winning key)
	if (D233.bossCutsceneIndex < 0)
	{
		cutsceneID = (levID - GEM_STONE_VALLEY) * 2;

		if (gGT->podiumRewardID == STATIC_KEY)
		{
			cutsceneID++;
		}
	}

	else
	{
		cutsceneID = D233.bossCutsceneIndex;
	}

	const struct BossCutsceneData *bcd = &R233.bossCS[cutsceneID];

	switch (D233.cutsceneState)
	{
	// Start Fade-to-black
	case CS_CAMERA_PAN:
	case CS_WAIT_INPUT:
		gGT->pushBuffer_UI.fadeFromBlack_desiredResult = 0;
		gGT->pushBuffer_UI.fade_step = -0x400;
		D233.cutsceneState = CS_FADE_OUT;
		break;

	// Wait for fade-to-black
	// Start loading process
	case CS_FADE_OUT:

		// wait for fade
		if (gGT->pushBuffer_UI.fadeFromBlack_currentValue != 0)
		{
			break;
		}

		// kill all podium "other" threads
		t = gGT->threadBuckets[OTHER].thread;
		while (t != 0)
		{
			t->flags |= THREAD_FLAG_DEAD;
			t = t->siblingThread;
		}

		// wait one frame, for the thread recycler to finish
		if (gGT->threadBuckets[OTHER].thread != 0)
		{
			break;
		}

		CS_LoadBoss(bcd);
		D233.cutsceneState = CS_LOADING;
		break;

	// Wait for loading callback,
	// start thread for head+body
	// start fade-to-normal
	case CS_LOADING:

		// NULLPTR checks if load finished,
		// because CS_LoadBossCallback writes this last
		if (D233.ptrModelBossHead == 0)
		{
			break;
		}

		struct Model **mArr = &D233.ptrModelBossHead;

		for (i = 0; i < 2; i++)
		{
			if (mArr[i] != NULL)
			{
				if (i != 0)
				{
					mArr[i] = (struct Model *)((char *)mArr[i] + 4);
				}

				gGT->modelPtr[mArr[i]->id] = mArr[i];
			}
		}

		MEMPACK_SwapPacks(gGT->activeMempackIndex);

		struct CsThreadInitData initData = {0};
		initData.podiumPos.x = bcd->bossPos.x;
		initData.podiumPos.y = bcd->bossPos.y;
		initData.podiumPos.z = bcd->bossPos.z;
		initData.rot.x = bcd->bossRot.x;
		initData.rot.y = bcd->bossRot.y;
		initData.rot.z = bcd->bossRot.z;
		initData.characterPos.x = 0;
		initData.characterPos.y = 0;
		initData.characterPos.z = 0;

		// MUST go backwards,
		// Body first, sibling = 0
		// Head next, sibling = body
		t = 0;
		for (i = 1; i >= 0; i--)
		{
			if (mArr[i] == NULL)
			{
				continue;
			}

			t = CS_Thread_Init(mArr[i]->id, mArr[i]->name, &initData, 0, t);
			if (t == NULL)
			{
				continue;
			}

			inst = t->inst;
			cs = t->object;

			// head
			if (i == 0)
			{
				CS_ScriptCmd_OpcodeAt(cs, bcd->opcode);
				cs->opcodeDuration = 0;
				continue;
			}

			// body
			inst->scale.x = 0x1000;
			inst->scale.y = 0x1000;
			inst->scale.z = 0x1000;
			cs->desiredScale = 0x1000;
		}

		// set camera position and rotation for cutscene
		gGT->pushBuffer[0].pos.x = bcd->camPos.x;
		gGT->pushBuffer[0].pos.y = bcd->camPos.y;
		gGT->pushBuffer[0].pos.z = bcd->camPos.z;

		gGT->pushBuffer[0].rot.x = bcd->camRot.x + 0x800;
		gGT->pushBuffer[0].rot.y = bcd->camRot.y;
		gGT->pushBuffer[0].rot.z = bcd->camRot.z;

		// fade back in
		gGT->pushBuffer_UI.fadeFromBlack_desiredResult = 0x1000;
		gGT->pushBuffer_UI.fade_step = 0x400;
		D233.cutsceneState = CS_FADE_IN;
		break;

	case CS_FADE_IN:

		// wait for fade
		if (gGT->pushBuffer_UI.fadeFromBlack_currentValue != 0x1000)
		{
			break;
		}

		D233.cutsceneState = CS_WAIT_END;
		break;

	case CS_WAIT_END:

		// wait for cutscene to end
		if (D233.isCutsceneOver != 1)
		{
			break;
		}

		gGT->podiumRewardID = NOFUNC; // 0
		t->flags |= THREAD_FLAG_DEAD;
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800aedf8-0x800af328
void CS_Camera_ThTick_Podium(struct Thread *th)
{
	struct GameTracker *gGT = sdata->gGT;
	u16 *podium = th->object;

	if (podium[0] == 0)
	{
		gGT->drivers[0]->funcPtrs[DRIVER_FUNC_INIT] = VehStuckProc_RIP_Init;
	}

	if (gGT->cameraDC[0].cameraMode != 3)
	{
		if (D233.cutsceneState < CS_WAIT_INPUT)
		{
			D233.cutsceneState = CS_WAIT_INPUT;
		}

		D233.PodiumInitUnk3 = 1;
	}

	if (((D233.cutsceneState != CS_CAMERA_PAN || D233.boolStartToSkip != 0) && ((gGT->gameMode2 & CUP_NEW_WIN) != 0)) && sdata->ptrActiveMenu == NULL)
	{
		s16 stringIndex = 0x236;

		if ((gGT->gameMode2 & CUP_NEW_BATTLE) != 0)
		{
			stringIndex = 0x237;
		}

		TakeCupProgress_Activate(stringIndex);
		gGT->gameMode2 &= ~(CUP_NEW_WIN | CUP_NEW_BATTLE);
	}

	if (D233.cutsceneState == CS_CAMERA_PAN || sdata->ptrActiveMenu != NULL)
	{
		int numPoints = CAM_Path_GetNumPoints();
		int maxFrame = (numPoints << 0x15) >> 0x10;

		if (maxFrame != 0)
		{
			u16 frameTime = podium[0] + gGT->elapsedTimeMS;
			int frameTimeSigned = (s16)frameTime;
			SVec3 pos;
			SVec3 rot;
			s16 camPath[4];
			int frame;

			if (maxFrame - 0x12c0 < frameTimeSigned)
			{
				D233.PodiumInitUnk3 = 1;
			}

			if (maxFrame <= frameTimeSigned)
			{
				frameTime = numPoints * 0x20 - 1;

				if (D233.cutsceneState < CS_WAIT_INPUT)
				{
					D233.cutsceneState = CS_WAIT_INPUT;
				}
			}

			frame = ((int)frameTime << 16) >> 21;
			D233.PodiumInitUnk2 = frame;
			podium[0] = frameTime;

			CAM_Path_Move(frame, pos.v, rot.v, camPath);

			gGT->pushBuffer[0].pos = pos;
			gGT->pushBuffer[0].rot = rot;
		}
	}
	else
	{
		if ((gGT->gameMode2 & CUP_NEW_WIN) != 0)
		{
			goto check_skip_button;
		}

		DecalFont_DrawLine(sdata->lngStrings[LNG_PRESS_TO_CONTINUE], 0x100, 0xbe, FONT_BIG, JUSTIFY_CENTER | ORANGE);
	}

	if (((gGT->gameMode2 & CUP_NEW_WIN) == 0) && sdata->ptrActiveMenu == NULL)
	{
		u32 tapped = sdata->gGamepads->gamepad[0].buttonsTapped;
		s16 rewardId;

		if (((tapped & BTN_START) == 0) && ((D233.cutsceneState == CS_CAMERA_PAN || (tapped & (BTN_START | BTN_CROSS_one)) == 0)) &&
		    ((gGT->gameMode2 & VEH_FREEZE_PODIUM) != 0))
		{
			return;
		}

		if ((gGT->gameMode1 & ADVENTURE_MODE) == 0)
		{
			sdata->mainMenuState = MAIN_MENU_TITLE;
			gGT->gameMode1 = (gGT->gameMode1 & ~ADVENTURE_ARENA) | MAIN_MENU;
			gGT->podiumRewardID = NOFUNC;
			gGT->gameMode2 &= ~VEH_FREEZE_PODIUM;

			RaceFlag_SetDrawOrder(0);
			MainRaceTrack_RequestLoad(MAIN_MENU_LEVEL);
			return;
		}

		D233.PodiumInitUnk3 = 1;
		rewardId = gGT->podiumRewardID;
		gGT->numWinners = 0;
		gGT->renderFlags &= ~4;

		if (rewardId != STATIC_BIG1)
		{
			if (CS_Camera_BoolGotoBoss() == 0)
			{
				s16 hintID;

				D233.isCutsceneOver = 1;
				th->flags |= THREAD_FLAG_DEAD;

				CS_DestroyPodium_StartDriving();

				switch (rewardId)
				{
				case STATIC_TROPHY:
					hintID = 0xc;
					break;
				case STATIC_RELIC:
					hintID = 0x13;
					break;
				case STATIC_KEY:
					hintID = 0xd;
					break;
				case STATIC_TOKEN:
					hintID = 0x14;
					break;
				default:
					hintID = 0x15;
					break;
				}

				if ((VehPickupItem_MaskBoolGoodGuy(gGT->drivers[0]) & 0xffff) == 0)
				{
					hintID += 0x1f;
				}

				CDSYS_XAPauseForce();
				CDSYS_XAPlay(1, hintID);

				gGT->podiumRewardID = NOFUNC;
				return;
			}

			th->funcThTick = CS_Camera_ThTick_Boss;

			if (gGT->podiumRewardID != STATIC_RELIC)
			{
				D233.bossCutsceneIndex = -1;
				return;
			}

			if (gGT->currAdvProfile.numRelics < 18)
			{
				D233.bossCutsceneIndex = -1;
				return;
			}

			D233.bossCutsceneIndex = gGT->levelID - GEM_STONE_VALLEY + OXIDE_RELICS_GEMSTONE;
			return;
		}

		gGT->podiumRewardID = NOFUNC;
		gGT->gameMode1 &= ~ADVENTURE_ARENA;
		gGT->gameMode2 &= ~VEH_FREEZE_PODIUM;

		MainRaceTrack_RequestLoad(CHECK_ADV_BIT(sdata->advProgress.rewards, ADV_REWARD_BEAT_OXIDE_SECOND) ? OXIDE_TRUE_ENDING : OXIDE_ENDING);

#if defined(CTR_NATIVE)
		// NOTE(aalhendi): The STATIC_BIG1 path does not kill the podium thread
		// (unlike the normal-exit path). On the next tick the podium re-enters
		// with podiumRewardID == NOFUNC and falls into the boss-camera
		// path. On retail PSX this is invisible: the checkered flag
		// transition covers the screen, and CD-ROM model loading in CS_LoadBoss
		// is slow enough that LOADING engages before boss threads can emit audio
		// or visuals. Native's faster I/O lets the boss camera
		// spawn threads and play audio before the checkered flag finishes, so kill
		// the thread here to prevent the re-entry entirely.
		D233.isCutsceneOver = 1;
		th->flags |= THREAD_FLAG_DEAD;
#endif
		return;
	}

check_skip_button:
	if ((sdata->gGamepads->gamepad[0].buttonsTapped & BTN_START) != 0)
	{
		D233.boolStartToSkip = 1;
	}
}
