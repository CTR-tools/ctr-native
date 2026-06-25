#include <common.h>

enum ArcadeAdventureEndMenuConstants
{
	AA_SCREEN_DEPTH = 0x200,
	AA_BIG_NUM_TARGET_SCALE = 0x1e00,
	AA_CTR_HUD_SLOT = 0x12,
	AA_CTR_LETTER_BASE_SCALE = 0x800,
	AA_CTR_LETTER_SCALE_BIAS_LOW = 0x401,
	AA_CTR_LETTER_GROW_STEP = 0x400,
	AA_CTR_LETTER_FLYIN_FRAMES = 8,
	AA_CTR_LETTER_FLYOUT_FRAMES = 10,
	AA_TOKEN_GROW_LIMIT = 0x2001,
	AA_TOKEN_GROW_STEP = 0x200,
	AA_TOKEN_AWARD_TEXT_FLY_FRAMES = 8,
	AA_CTR_TEXT_FLYIN_START_FRAME = 140,
	AA_CTR_TEXT_FLYOUT_START_FRAME = 230,
	AA_CTR_TEXT_FLYOUT_AWARD_OFFSET = 50,
	AA_CTR_ALREADY_UNLOCKED_FLYOUT_FRAME = CTR_SECONDS_TO_FRAMES(10),
	AA_CONFIRM_BUTTON_MASK = BTN_CROSS_one | BTN_CIRCLE,
	AA_MENU_READY_FLAG = 1,
	AA_RACE_HUD_FLAG = 1,
	AA_CUP_STANDINGS_HUD_FLAG = 4,
	AA_RESULT_WAIT_FRAMES = CTR_SECONDS_TO_FRAMES(1),
	AA_RESULT_MAX_FRAMES = CTR_SECONDS_TO_FRAMES(30),
	AA_DRIVER_ICON_STAGGER_FRAMES = 10,
	AA_DRIVER_ICON_SPACING = 56,
	AA_DRIVER_ICON_EXIT_FRAME = CTR_SECONDS_TO_FRAMES(10),
	AA_DRIVER_ICON_SCALE = 0x1000,
	AA_DRIVER_ICON_GRAY_CHANNEL = 0x80,
	AA_CONTINUE_DELAY_FRAMES = 110,
	AA_CTR_LETTER_FALL_DELAY_FRAMES = 6,
	AA_CTR_LETTER_FALL_MIN_Y = -300,
	AA_CTR_LETTER_FALL_MIN_VEL_Y = -0x14,
	AA_KEY_BOSS_COUNT = 4,
	AA_OXIDE_SECOND_WIN_BOSS_ID = 5,
	AA_HUD_ELEMENTS_PER_DRIVER = 0x14,
	AA_TIME_DISPLAY_BIG_NUM_SLOT = 2,
	AA_TIME_DISPLAY_SUFFIX_SLOT = 5,
	AA_TIME_DISPLAY_SKIP_FRAME = 110,
	AA_TIME_DISPLAY_LATE_FRAME = CTR_SECONDS_TO_FRAMES(10),
	AA_TIME_DISPLAY_FLYIN_FRAMES = CTR_SECONDS_TO_FRAMES(1),
	AA_TIME_DISPLAY_FLYOUT_FRAMES = 15,
	AA_TIME_BOX_HEIGHT_7_LAPS = 0x49,
	AA_TIME_BOX_HEIGHT_5_LAPS = 0x39,
	AA_TIME_BOX_HEIGHT_DEFAULT = 0x44,
};

global_variable s32 s_driverRankString222 = 0x20; // " \0"
extern struct RectMenu menu222;
extern struct RectMenu menu222_2P;

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8009f704-0x800a06f8.
void AA_EndEvent_DrawMenu(void)
{
	SVec2 letterPos;
	SVec2 textPos;

	s16 lerpStartX;
	s16 txtStartX;
	s16 lerpStartY;
	s16 lerpEndX;
	s16 txtEndX;
	s16 lerpEndY;
	s16 lerpFrames;
	s32 rewardBit = -1;

	struct GameTracker *gGT = sdata->gGT;
	struct Driver *driver = gGT->drivers[0];
	s32 numPlayers = gGT->numPlyrCurrGame;
	s32 totalRacers = numPlayers + gGT->numBotsNextGame;
	struct AdvProgress *adv = &sdata->advProgress;
	struct Instance *hudC = sdata->ptrHudC;
	struct Instance *hudT = sdata->ptrHudT;
	struct Instance *hudR = sdata->ptrHudR;
	struct Instance *hudLetters[3] = {hudC, hudT, hudR};
	struct Instance *hudToken = sdata->ptrToken;
	struct UiElement2D *hudCTR = &data.hud_1P_P1[AA_CTR_HUD_SLOT];

	s32 elapsedFrames = sdata->framesSinceRaceEnded;

	if (elapsedFrames < AA_RESULT_MAX_FRAMES)
	{
		elapsedFrames++;
	}

	sdata->framesSinceRaceEnded = elapsedFrames;

	if (driver->instBigNum->scale.x != AA_BIG_NUM_TARGET_SCALE)
	{
		struct Instance *instFruitDisp = driver->instFruitDisp;
		instFruitDisp->scale = (SVec3){{0, 0, 0}};
	}

	// if not in Token mode, these won't be used until later;
	lerpStartY = 0;
	lerpEndY = 0;

	// For trophy race, check 1st place
	b32 didWin = (driver->driverRank == 0);
	b32 didEarnCtrToken = didWin && (driver->PickupLetterHUD.numCollected == 3);

	// If adventure mode
	if ((gGT->gameMode1 & ADVENTURE_MODE) != 0)
	{
		if (didEarnCtrToken)
		{
			// lerp C-T-R letters closer to center by 16 pixels
			// until the new-token or already-unlocked flyout phase starts
			lerpStartX = hudCTR->x;
			lerpStartY = hudCTR->y;
			lerpEndX = lerpStartX + 0x10;
			lerpEndY = lerpStartY + 0x10;
			lerpFrames = AA_CTR_LETTER_FLYIN_FRAMES;

			// If you have not unlocked this CTR Token
			rewardBit = gGT->levelID + ADV_REWARD_FIRST_CTR_TOKEN;
			letterPos.x = hudCTR->x;
			letterPos.y = hudCTR->y;
			s32 letterScaleOffset;
			b32 shouldDrawToken = false;
			b32 shouldScaleLetters = false;
			s32 tokenAwardTextFrame = -1;
			if (CHECK_ADV_BIT(adv->rewards, rewardBit) == 0)
			{
				letterScaleOffset = hudC->scale.x;
				letterScaleOffset -= (letterScaleOffset < AA_CTR_LETTER_BASE_SCALE) ? AA_CTR_LETTER_SCALE_BIAS_LOW : AA_CTR_LETTER_BASE_SCALE;
				letterScaleOffset >>= 10;
				shouldDrawToken = true;

				// lerp letters off-screen
				if (elapsedFrames > AA_CTR_TEXT_FLYOUT_START_FRAME)
				{
					// NOTE(aalhendi): Retail uses frames-50 for the awarded text, skipping most of the fly-out.
					tokenAwardTextFrame = elapsedFrames - AA_CTR_TEXT_FLYOUT_AWARD_OFFSET;
					txtStartX = 0x100;
					txtEndX = -150;
					elapsedFrames -= AA_CTR_TEXT_FLYOUT_START_FRAME;
					lerpFrames = AA_CTR_LETTER_FLYOUT_FRAMES;

					lerpStartX += 0x10;
					lerpStartY += 0x50;
					lerpEndX = -400;
					lerpEndY = lerpStartY;
				}

				// lerp letters to center
				else if (elapsedFrames > AA_CTR_TEXT_FLYIN_START_FRAME)
				{
					elapsedFrames -= AA_CTR_TEXT_FLYIN_START_FRAME;
					tokenAwardTextFrame = elapsedFrames;
					txtStartX = 0x264;
					txtEndX = 0x100;
					shouldScaleLetters = true;

					lerpStartX += 0x10;
					lerpStartY += 0x10;
					lerpEndX = hudCTR->x - 0x10;
					lerpEndY = hudCTR->y + 0x50;
				}

				UI_Lerp2D_Linear(letterPos.v, lerpStartX, lerpStartY, lerpEndX, lerpEndY, elapsedFrames, lerpFrames);

				if (shouldScaleLetters)
				{
					// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8009fc48-0x8009fc50 for CTR token unlock SFX.
					if (hudC->scale.x == AA_CTR_LETTER_BASE_SCALE)
					{
						OtherFX_Play(0x67, 1);
					}

					// NOTE(aalhendi): Retail scales until X reaches target, with no separate scale cap.
					if (letterPos.x != hudCTR->x - 0x10)
					{
						for (s32 i = 0; i < 3; i++)
						{
							hudLetters[i]->scale.x += AA_CTR_LETTER_GROW_STEP;
							hudLetters[i]->scale.y += AA_CTR_LETTER_GROW_STEP;
							hudLetters[i]->scale.z += AA_CTR_LETTER_GROW_STEP;
						}
					}
				}

				// variable reuse, frame timers
				lerpStartY = 120;
				lerpEndY = 160;
			}

			// If you already have this CTR Token unlocked
			else
			{
				if (elapsedFrames > AA_CTR_ALREADY_UNLOCKED_FLYOUT_FRAME)
				{
					elapsedFrames -= AA_CTR_ALREADY_UNLOCKED_FLYOUT_FRAME;

					lerpStartX = hudCTR->x + 0x10;
					lerpStartY = hudCTR->y + 0x10;
					lerpEndX = -400;
					lerpEndY = lerpStartY;
					lerpFrames = AA_CTR_LETTER_FLYOUT_FRAMES;
				}

				UI_Lerp2D_Linear(letterPos.v, lerpStartX, lerpStartY, lerpEndX, lerpEndY, elapsedFrames, lerpFrames);

				// variable reuse, frame timers
				lerpStartY = 0;
				lerpEndY = 0;

				letterScaleOffset = 0;
			}

			for (s32 i = 0; i < 3; i++)
			{
				hudLetters[i]->matrix.t[0] = UI_ConvertX_2(letterPos.x + (letterScaleOffset * (i * 12)) + (i * 29), AA_SCREEN_DEPTH);
				hudLetters[i]->matrix.t[1] = UI_ConvertY_2(letterPos.y - (i & 1), AA_SCREEN_DEPTH);
			}

			if (shouldDrawToken)
			{
				hudR->depthBiasNormal = 1;
				hudToken->flags &= ~HIDE_MODEL;
				hudToken->matrix.t[0] = hudT->matrix.t[0];
				hudToken->matrix.t[1] = UI_ConvertY_2(letterPos.y + 0x18, AA_SCREEN_DEPTH);

				if ((tokenAwardTextFrame >= 0) && (hudToken->scale.x < AA_TOKEN_GROW_LIMIT))
				{
					hudToken->scale.x += AA_TOKEN_GROW_STEP;
					hudToken->scale.y += AA_TOKEN_GROW_STEP;
					hudToken->scale.z += AA_TOKEN_GROW_STEP;
				}

				if (tokenAwardTextFrame >= 0)
				{
					UI_Lerp2D_Linear(textPos.v, txtStartX, 0xa6, txtEndX, 0xa6, tokenAwardTextFrame, AA_TOKEN_AWARD_TEXT_FLY_FRAMES);

					s32 textColor = (gGT->timer & 1) ? (JUSTIFY_CENTER | RED) : (JUSTIFY_CENTER | WHITE);

					DecalFont_DrawLine(sdata->lngStrings[LNG_CTR_TOKEN_AWARDED], textPos.x, textPos.y, FONT_BIG, textColor);
				}
			}
		}

		// If you did not collect all 3 letters (C, T, and R), or you lost the race.
		else
		{
			driver->PickupLetterHUD.numCollected = 0;

			// Do this for the first 30 seconds.
			if (elapsedFrames < AA_RESULT_MAX_FRAMES)
			{
				for (s32 i = 0; i < 3; i++)
				{
					if (
					    // letter is visible
					    ((hudLetters[i]->flags & HIDE_MODEL) == 0) &&

					    // delay letter (6 frames apart)
					    (elapsedFrames > AA_CTR_LETTER_FALL_DELAY_FRAMES * i) &&

					    // letter not fully off-screen
					    (AA_CTR_LETTER_FALL_MIN_Y < hudLetters[i]->matrix.t[1]))
					{
						struct UiElement3D *letter = hudLetters[i]->thread->object;

						// move X position (yes, C-Letter only, Naughty Dog bug?)
						hudLetters[0]->matrix.t[0] += letter->vel[0];

						// make the letter fall off the screen
						hudLetters[i]->matrix.t[1] -= letter->vel[1];

						if (AA_CTR_LETTER_FALL_MIN_VEL_Y < letter->vel[1])
						{
							letter->vel[1] -= 2;
						}
					}
				}
			}
		}
	}

	// If C-T-R token race, add requirement of C-T-R letters.
	if ((gGT->gameMode2 & TOKEN_RACE) != 0)
	{
		didWin = didEarnCtrToken;
	}

	for (s32 i = 0; i < numPlayers; i++)
	{
		// Draw how much time it took to finish laps and race
		AA_EndEvent_DisplayTime(i, lerpEndY);
	}

	elapsedFrames = sdata->framesSinceRaceEnded;

	// If it hasn't been 1 second from race ended
	if (elapsedFrames < AA_RESULT_WAIT_FRAMES)
	{
		return;
	}

	// If there is one player
	if (numPlayers == 1)
	{
		// start counting time 1 second after race ends
		s32 driverIconFrame = (elapsedFrames & 0xffff) - AA_RESULT_WAIT_FRAMES;

		if (
		    // Every 10 frames
		    ((driverIconFrame % AA_DRIVER_ICON_STAGGER_FRAMES & 0xffff) == 0) &&

		    // sdata->numIconsEOR is the number of icons being
		    // drawn on the end-of-race menu in 1P mode

		    // If you have not drawn all drivers yet
		    (sdata->numIconsEOR < totalRacers))
		{
			// add an icon to draw
			sdata->numIconsEOR++;
		}

		// loop through all the driver icons
		for (s32 i = 0; i < sdata->numIconsEOR; i++)
		{
			s32 driverIconTargetX =
			    gGT->pushBuffer[0].rect.x + (gGT->pushBuffer[0].rect.w - totalRacers * AA_DRIVER_ICON_SPACING + 12) / 2 + (i * AA_DRIVER_ICON_SPACING);
			s32 currFrame;

			if (elapsedFrames + lerpEndY > AA_DRIVER_ICON_EXIT_FRAME)
			{
				lerpStartX = driverIconTargetX;
				lerpEndX = -100;
				currFrame = elapsedFrames + lerpEndY - AA_DRIVER_ICON_EXIT_FRAME;
			}
			else
			{
				lerpStartX = 0x218;
				lerpEndX = driverIconTargetX;
				currFrame = driverIconFrame;
			}

			driverIconFrame -= AA_DRIVER_ICON_STAGGER_FRAMES;

			// interpolate fly-in
			UI_Lerp2D_Linear(letterPos.v, lerpStartX, 0x60, lerpEndX, 0x60, currFrame, AA_DRIVER_ICON_STAGGER_FRAMES);

			s_driverRankString222 = (char)i + '1';

			// print a single character, a number 1-8,
			DecalFont_DrawLine((char *)&s_driverRankString222, letterPos.x + 0x20, 0x5f, FONT_SMALL, WHITE);

			// Draw the driver's character icon
			UI_DrawDriverIcon(

			    gGT->ptrIcons[data.MetaDataCharacters[data.characterIDs[gGT->driversInRaceOrder[i]->driverID]].iconID],

			    letterPos.x, 0x60, &gGT->backBuffer->primMem,

			    // pointer to OT mem
			    gGT->pushBuffer_UI.ptrOT,

			    1, AA_DRIVER_ICON_SCALE, MakeColor(AA_DRIVER_ICON_GRAY_CHANNEL, AA_DRIVER_ICON_GRAY_CHANNEL, AA_DRIVER_ICON_GRAY_CHANNEL).self);
		}
	}

	// 0x78 + 0x6e = 0xe6 (230) frames waited for Token Race
	if ((elapsedFrames - lerpStartY) < AA_CONTINUE_DELAY_FRAMES)
	{
		return;
	}

	if (
	    // If you are in Adventure cup
	    ((gGT->gameMode1 & ADVENTURE_CUP) != 0) ||

	    // If you are in Arcade or VS cup
	    ((gGT->gameMode2 & CUP_ANY_KIND) != 0))
	{
		// but text near middle of screen
		s16 pressContinueY = (numPlayers == 2) ? 100 : 0xbe;

		DecalFont_DrawLine(sdata->lngStrings[LNG_PRESS_TO_CONTINUE], 0x100, pressContinueY, FONT_BIG, (JUSTIFY_CENTER | ORANGE));

		// If you do not "Press X to continue"
		if ((sdata->AnyPlayerTap & AA_CONFIRM_BUTTON_MASK) == 0)
		{
			return;
		}

		// If you are here, it means you pressed X to continue

		// clear gamepad input
		RECTMENU_ClearInput();

		sdata->menuReadyToPass = 0;
		sdata->framesSinceRaceEnded = 0;
		sdata->numIconsEOR = 1;

		// Disable HUD
		gGT->hudFlags &= ~AA_RACE_HUD_FLAG;

		// Enable Cup Standings
		gGT->hudFlags |= AA_CUP_STANDINGS_HUD_FLAG;
		return;
	}

	// if the menu is already drawing
	if (sdata->menuReadyToPass & AA_MENU_READY_FLAG)
	{
		return;
	}

	// If you're in Arcade mode
	if ((gGT->gameMode1 & ARCADE_MODE) != 0)
	{
		RECTMENU_Show((numPlayers == 1) ? &menu222 : &menu222_2P);

		// record that the menu is drawing
		sdata->menuReadyToPass |= AA_MENU_READY_FLAG;
		return;
	}

	// If you are in adventure mode
	if ((gGT->gameMode1 & ADVENTURE_MODE) == 0)
	{
		return;
	}

	DecalFont_DrawLine(sdata->lngStrings[LNG_PRESS_TO_CONTINUE], 0x100, 0xbe, FONT_BIG, (JUSTIFY_CENTER | ORANGE));

	// If you have not pressed X
	if ((sdata->AnyPlayerTap & AA_CONFIRM_BUTTON_MASK) == 0)
	{
		return;
	}

	// === If Pressed X ===

	RECTMENU_ClearInput();

	sdata->Loading.OnBegin.AddBitsConfig0 |= ADVENTURE_ARENA;
	sdata->Loading.OnBegin.RemBitsConfig0 |= ADVENTURE_BOSS;
	sdata->Loading.OnBegin.RemBitsConfig8 |= TOKEN_RACE;

	if (IS_BOSS_RACE(gGT->gameMode1))
	{
		sdata->Loading.OnBegin.AddBitsConfig8 |= SPAWN_AT_BOSS;
	}

	if (!didWin)
	{
		RECTMENU_Show(&data.menuRetryExit);
		sdata->menuReadyToPass |= AA_MENU_READY_FLAG;
		return;
	}

	// === If you won the race ===

	sdata->framesSinceRaceEnded = 0;
	sdata->numIconsEOR = 1;

	// Load the levelID for Adventure Hub that you came from
	s16 levSpawn = gGT->prevLEV;

	if (IS_BOSS_RACE(gGT->gameMode1))
	{
		// Reward bit of key unlocked, and boss beaten.
		rewardBit = gGT->bossID + ADV_REWARD_FIRST_BOSS_KEY;

		// If this is one of the four key-awarding bosses.
		if (gGT->bossID < AA_KEY_BOSS_COUNT)
		{
			// only if first time beating boss
			if (CHECK_ADV_BIT(adv->rewards, rewardBit) == 0)
			{
				// Go to Podium after returning to Adventure Hub
				gGT->podiumRewardID = STATIC_KEY; // key

				// hot air skyway
				if (gGT->levelID == HOT_AIR_SKYWAY)
				{
					// If you just beat Pinstripe
					// Load gemstone valley
					levSpawn = GEM_STONE_VALLEY;
				}
			}
		}

		// If you have 4 keys (only here if you beat oxide)
		else
		{
			// Always go to podium after oxide,
			// with no key (0x38 = empty)
			gGT->podiumRewardID = STATIC_BIG1;

			adv->storyFlags |= ADV_REWARD_OXIDE_FIRST_WIN_FLAGS;

			if (gGT->bossID == AA_OXIDE_SECOND_WIN_BOSS_ID)
			{
				adv->storyFlags |= ADV_REWARD_OXIDE_SECOND_WIN_FLAGS;
			}
		}
	}

	// if something needs unlocking
	if (rewardBit > 0)
	{
		// Unlock reward
		UNLOCK_ADV_BIT(adv->rewards, rewardBit);
	}

	if (IS_BOSS_RACE(gGT->gameMode1))
	{
		MainRaceTrack_RequestLoad(levSpawn);
		return;
	}

	// if trophy is not won,
	// Dingo Bingo needs to win trophy and token in the same race
	rewardBit = gGT->levelID + ADV_REWARD_FIRST_TROPHY;
	if (CHECK_ADV_BIT(adv->rewards, rewardBit) == 0)
	{
		// unlock tropy
		UNLOCK_ADV_BIT(adv->rewards, rewardBit);

		// go to podium with trophy
		gGT->podiumRewardID = STATIC_TROPHY;
	}

	MainRaceTrack_RequestLoad(levSpawn);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800a06f8-0x800a0b38.
void AA_EndEvent_DisplayTime(s16 driverId, s16 timeOffsetFrames)
{
	s16 lerpStartY;
	s16 lerpEndY;
	s16 lerpStartX;
	s16 lerpEndX;
	s32 currFrame;
	s32 endFrame;
	SVec2 pos;

	struct GameTracker *gGT = sdata->gGT;
	struct Driver *driver = gGT->drivers[driverId];

	s32 numPlayers = gGT->numPlyrCurrGame;
	struct UiElement2D *hudArray = data.hudStructPtr[numPlayers - 1];
	struct UiElement2D *hud = &hudArray[driverId * AA_HUD_ELEMENTS_PER_DRIVER];
	struct Instance *bigNum = driver->instBigNum;

	// Lap time box height
	RECT timeBoxRect;
	switch (gGT->numLaps)
	{
	// based on number of laps
	case 7:
		timeBoxRect.h = AA_TIME_BOX_HEIGHT_7_LAPS;
		break;
	case 5:
		timeBoxRect.h = AA_TIME_BOX_HEIGHT_5_LAPS;
		break;
	default:
		// default height for 1/3 laps.
		timeBoxRect.h = AA_TIME_BOX_HEIGHT_DEFAULT;
		break;
	}

	// increment counter for number of frames since the player ended the race
	driver->framesSinceRaceEnded_forThisDriver++;
	s32 framesElapsed = driver->framesSinceRaceEnded_forThisDriver;

	if (
	    // if player ended race less than 110 frames ago
	    (framesElapsed < AA_TIME_DISPLAY_SKIP_FRAME) &&

	    // If you press Cross or Circle
	    ((sdata->AnyPlayerTap & AA_CONFIRM_BUTTON_MASK) != 0) &&

	    // only one player
	    (numPlayers == 1))
	{
		// Assume race ended 110 frames ago
		framesElapsed = AA_TIME_DISPLAY_SKIP_FRAME;
		sdata->framesSinceRaceEnded = framesElapsed;
		driver->framesSinceRaceEnded_forThisDriver = framesElapsed;

		sdata->numIconsEOR = numPlayers + gGT->numBotsNextGame;

		// clear gamepad input (for menus)
		RECTMENU_ClearInput();
	}

	b32 isLateDisplay = (framesElapsed + timeOffsetFrames > AA_TIME_DISPLAY_LATE_FRAME);

	// === Draw BigNum ===

	// Player 2
	lerpEndY = 0x41;

	if (driverId == 0)
	{
		lerpEndY = -0x3d;
	}

	// If race ended more than 10 seconds ago.
	if (isLateDisplay)
	{
		currFrame = framesElapsed + timeOffsetFrames - AA_TIME_DISPLAY_LATE_FRAME;
		endFrame = AA_TIME_DISPLAY_FLYOUT_FRAMES;

		lerpStartX = -0xae;
		lerpEndX = UI_ConvertX_2(-100, hud[AA_TIME_DISPLAY_BIG_NUM_SLOT].z);
		lerpStartY = lerpEndY;
	}

	// If not
	else
	{
		currFrame = framesElapsed;
		endFrame = AA_TIME_DISPLAY_FLYIN_FRAMES;

		lerpStartX = UI_ConvertX_2(hud[AA_TIME_DISPLAY_BIG_NUM_SLOT].x, hud[AA_TIME_DISPLAY_BIG_NUM_SLOT].z);
		lerpStartY = UI_ConvertY_2(hud[AA_TIME_DISPLAY_BIG_NUM_SLOT].y, hud[AA_TIME_DISPLAY_BIG_NUM_SLOT].z);
		lerpEndX = -0xae;
	}

	// interpolate fly-in positionXY
	UI_Lerp2D_Linear(pos.v, lerpStartX, lerpStartY, lerpEndX, lerpEndY, currFrame, endFrame);

	bigNum->matrix.t[0] = pos.x;
	bigNum->matrix.t[1] = pos.y;

	// interpolate scale to the target big-number size
	UI_Lerp2D_Linear(pos.v, hud[AA_TIME_DISPLAY_BIG_NUM_SLOT].scale, 0, AA_BIG_NUM_TARGET_SCALE, 0, framesElapsed, AA_TIME_DISPLAY_FLYIN_FRAMES);
	s16 bigNumScale = pos.x;

	bigNum->scale = (SVec3){{bigNumScale, bigNumScale, bigNumScale}};

	// === Draw Suffix ===

	// Player 2
	lerpEndY = 0x89;

	if (driverId == 0)
	{
		lerpEndY = 9;
	}

	if (isLateDisplay)
	{
		lerpStartX = 0x78;
		lerpStartY = lerpEndY;
		lerpEndX = -0x3c;
	}
	else
	{
		lerpStartX = hud[AA_TIME_DISPLAY_SUFFIX_SLOT].x;
		lerpStartY = hud[AA_TIME_DISPLAY_SUFFIX_SLOT].y;
		lerpEndX = 0x78;
	}

	UI_Lerp2D_Linear(pos.v, lerpStartX, lerpStartY, lerpEndX, lerpEndY, currFrame, endFrame);

	UI_DrawPosSuffix(pos.x, pos.y, driver, 0);

	// === DrawRaceClock ===

	lerpEndY = 0xc3;
	if (driverId == 0)
	{
		lerpEndY = 0x3e;
	}

	if (isLateDisplay)
	{
		lerpStartX = 0x150;
		lerpEndX = 0x27c;
	}

	else
	{
		lerpStartX = 0x218;
		lerpEndX = 0x150;
	}

	UI_Lerp2D_Linear(pos.v, lerpStartX, lerpEndY, lerpEndX, lerpEndY, currFrame, endFrame);

	UI_DrawRaceClock(pos.x, pos.y, 1, driver);

	s16 totalTextWidth = DecalFont_GetLineWidth(sdata->lngStrings[LNG_TOTAL], FONT_BIG);

	timeBoxRect.x = (pos.x - totalTextWidth) + -6;
	timeBoxRect.y = (pos.y - timeBoxRect.h) + 0xd;
	timeBoxRect.w = totalTextWidth + 0x94;
	timeBoxRect.h += 6;

	// Draw 2D Menu rectangle background
	RECTMENU_DrawInnerRect(&timeBoxRect, 4, gGT->backBuffer->otMem.uiOT);
	return;
}

struct MenuRow rows222[5] = {
    // Retry
    {
        .stringIndex = LNG_RETRY,
        .rowOnPressUp = 0,
        .rowOnPressDown = 1,
        .rowOnPressLeft = 0,
        .rowOnPressRight = 0,
    },
    // Change Level
    {
        .stringIndex = LNG_CHANGE_LEVEL,
        .rowOnPressUp = 0,
        .rowOnPressDown = 2,
        .rowOnPressLeft = 1,
        .rowOnPressRight = 1,
    },
    // Change Character
    {
        .stringIndex = LNG_CHANGE_CHARACTER,
        .rowOnPressUp = 1,
        .rowOnPressDown = 3,
        .rowOnPressLeft = 2,
        .rowOnPressRight = 2,
    },
    // Quit
    {
        .stringIndex = LNG_QUIT,
        .rowOnPressUp = 2,
        .rowOnPressDown = 3,
        .rowOnPressLeft = 3,
        .rowOnPressRight = 3,
    },
    // NULL, end of menu
    {
        .stringIndex = 0xFFFF,
        .rowOnPressUp = 0,
        .rowOnPressDown = 0,
        .rowOnPressLeft = 0,
        .rowOnPressRight = 0,
    }};

struct RectMenu menu222 = {
    .stringIndexTitle = 0xFFFF,
    .posX_curr = 256,
    .posY_curr = 170,
    .unk1 = 0,
    .state = (0x800 | USE_SMALL_FONT | CENTER_ON_COORDS), // 0x883
    .rows = rows222,
    .funcPtr = UI_RaceEnd_MenuProc,
    .drawStyle = 4,
    // rest of variables all default zero
};

struct RectMenu menu222_2P = {
    .stringIndexTitle = 0xFFFF,
    .posX_curr = 256,
    .posY_curr = 108,
    .unk1 = 0,
    .state = (0x800 | USE_SMALL_FONT | CENTER_ON_COORDS), // 0x883
    .rows = rows222,
    .funcPtr = UI_RaceEnd_MenuProc,
    .drawStyle = 4,
    // rest of variables all default zero
};
