#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800aedf8-0x800af328
void CS_Camera_ThTick_Podium(struct Thread *th)
{
	struct GameTracker *gGT = sdata->gGT;
	u16 *podium = th->object;

	if (podium[0] == 0)
		gGT->drivers[0]->funcPtrs[DRIVER_FUNC_INIT] = VehStuckProc_RIP_Init;

	if (gGT->cameraDC[0].cameraMode != 3)
	{
		if (D233.cutsceneState < CS_WAIT_INPUT)
			D233.cutsceneState = CS_WAIT_INPUT;

		D233.PodiumInitUnk3 = 1;
	}

	if (((D233.cutsceneState != CS_CAMERA_PAN || D233.boolStartToSkip != 0) && ((gGT->gameMode2 & CUP_NEW_WIN) != 0)) && sdata->ptrActiveMenu == NULL)
	{
		s16 stringIndex = 0x236;

		if ((gGT->gameMode2 & CUP_NEW_BATTLE) != 0)
			stringIndex = 0x237;

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
			s16 pos[3];
			s16 rot[3];
			s16 camPath[4];
			int frame;

			if (maxFrame - 0x12c0 < frameTimeSigned)
				D233.PodiumInitUnk3 = 1;

			if (maxFrame <= frameTimeSigned)
			{
				frameTime = numPoints * 0x20 - 1;

				if (D233.cutsceneState < CS_WAIT_INPUT)
					D233.cutsceneState = CS_WAIT_INPUT;
			}

			frame = ((int)frameTime << 16) >> 21;
			D233.PodiumInitUnk2 = frame;
			podium[0] = frameTime;

			CAM_Path_Move(frame, pos, rot, camPath);

			gGT->pushBuffer[0].pos.x = pos[0];
			gGT->pushBuffer[0].pos.y = pos[1];
			gGT->pushBuffer[0].pos.z = pos[2];
			gGT->pushBuffer[0].rot.x = rot[0];
			gGT->pushBuffer[0].rot.y = rot[1];
			gGT->pushBuffer[0].rot.z = rot[2];
		}
	}
	else
	{
		if ((gGT->gameMode2 & CUP_NEW_WIN) != 0)
			goto check_skip_button;

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
					hintID += 0x1f;

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
		D233.boolStartToSkip = 1;
}
