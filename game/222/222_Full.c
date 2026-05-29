#include <common.h>

static int str_number222 = 0x20; // " \0"
extern struct RectMenu menu222;
extern struct RectMenu menu222_2P;

// 3528

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8009f704-0x800a06f8.
void AA_EndEvent_DrawMenu(void)
{
	struct GameTracker *gGT;
	struct Driver *driver;
	struct UiElement2D *hudCTR;
	struct AdvProgress *adv;
	struct Instance *hudC;
	struct Instance *hudT;
	struct Instance *hudR;
	struct Instance *hudToken;
	struct UiElement3D *letter;

	char i;
	char tokenUnlock;
	char numPlyr;
	char totalPlyr;

	s16 letterPos[2];
	s16 txtPos[2];

	s16 t;
	s16 elapsedFrames;
	s16 levSpawn;

	s16 lerpStartX;
	s16 txtStartX;
	s16 lerpStartY;
	s16 lerpEndX;
	s16 txtEndX;
	s16 lerpEndY;
	s16 lerpFrames;
	s16 currFrame;
	int scaleDown;
	u32 txtColor;
	int bitIndex;

	bitIndex = -1;
	gGT = sdata->gGT;
	driver = gGT->drivers[0];
	numPlyr = gGT->numPlyrCurrGame;
	totalPlyr = numPlyr + gGT->numBotsNextGame;
	adv = &sdata->advProgress;
	hudC = sdata->ptrHudC;
	hudT = sdata->ptrHudT;
	hudR = sdata->ptrHudR;
	struct Instance *hudLetters[3] = {hudC, hudT, hudR};
	hudToken = sdata->ptrToken;
	hudCTR = &data.hud_1P_P1[0x12];

	elapsedFrames = sdata->framesSinceRaceEnded;

	// count frames if hasn't been 30 seconds
	if (elapsedFrames < 900)
		elapsedFrames++;

	sdata->framesSinceRaceEnded = elapsedFrames;

	if (driver->instBigNum->scale[0] != 0x1e00)
	{
		struct Instance *instFruitDisp = driver->instFruitDisp;
		instFruitDisp->scale[0] = 0;
		instFruitDisp->scale[1] = 0;
		instFruitDisp->scale[2] = 0;
	}

	// if not in Token mode, these won't be used until later;
	lerpStartY = 0;
	lerpEndY = 0;

	// For trophy race, check 1st place
	int boolWin = (driver->driverRank == 0);
	int boolTokenAward = boolWin && (driver->PickupLetterHUD.numCollected == 3);

	// If adventure mode
	if ((gGT->gameMode1 & ADVENTURE_MODE) != 0)
	{
		if (boolTokenAward)
		{
			// lerp C-T-R letters closer to center by 16 pixels
			// default (unlocking and frames < 140) or (already unlocked and frames < 300)
			lerpStartX = hudCTR->x;
			lerpStartY = hudCTR->y;
			lerpEndX = lerpStartX + 0x10;
			lerpEndY = lerpStartY + 0x10;
			lerpFrames = 8;


			// If you have not unlocked this CTR Token
			bitIndex = gGT->levelID + 0x4c;
			*(int *)&letterPos[0] = *(int *)&hudCTR[0];
			int updateToken = 0;
			int scaleLetters = 0;
			int awardTextFrame = -1;
			if (CHECK_ADV_BIT(adv->rewards, bitIndex) == 0)
			{
				scaleDown = hudC->scale[0];
				scaleDown -= (scaleDown < 0x800) ? 0x401 : 0x800;
				scaleDown >>= 10;
				updateToken = 1;

				// lerp letters off-screen
				if (elapsedFrames > 230)
				{
					// NOTE(aalhendi): Retail uses frames-50 for the awarded text, skipping most of the fly-out.
					awardTextFrame = elapsedFrames - 50;
					txtStartX = 0x100;
					txtEndX = -150;
					elapsedFrames -= 230;
					lerpFrames = 10;

					lerpStartX += 0x10;
					lerpStartY += 0x50;
					lerpEndX = -400;
					lerpEndY = lerpStartY;
				}

				// lerp letters to center
				else if (elapsedFrames > 140)
				{
					elapsedFrames -= 140;
					awardTextFrame = elapsedFrames;
					txtStartX = 0x264;
					txtEndX = 0x100;
					scaleLetters = 1;

					lerpStartX += 0x10;
					lerpStartY += 0x10;
					lerpEndX = hudCTR->x - 0x10;
					lerpEndY = hudCTR->y + 0x50;
				}

				UI_Lerp2D_Linear(&letterPos[0], lerpStartX, lerpStartY, lerpEndX, lerpEndY, elapsedFrames, lerpFrames);

				if (scaleLetters)
				{
					// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8009fc48-0x8009fc50 for CTR token unlock SFX.
					if (hudC->scale[0] == 0x800)
						OtherFX_Play(0x67, 1);

					// NOTE(aalhendi): Retail scales until X reaches target, with no separate scale cap.
					if (letterPos[0] != hudCTR->x - 0x10)
					{
						for (i = 0; i < 3; i++)
						{
							hudLetters[i]->scale[0] += 0x400;
							hudLetters[i]->scale[1] += 0x400;
							hudLetters[i]->scale[2] += 0x400;
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
				if (elapsedFrames > 300)
				{
					elapsedFrames -= 300;

					lerpStartX = hudCTR->x + 0x10;
					lerpStartY = hudCTR->y + 0x10;
					lerpEndX = -400;
					lerpEndY = lerpStartY;
					lerpFrames = 10;
				}

				UI_Lerp2D_Linear(&letterPos[0], lerpStartX, lerpStartY, lerpEndX, lerpEndY, elapsedFrames, lerpFrames);

				// variable reuse, frame timers
				lerpStartY = 0;
				lerpEndY = 0;

				scaleDown = 0;
			}

			for (i = 0; i < 3; i++)
			{
				hudLetters[i]->matrix.t[0] = UI_ConvertX_2(letterPos[0] + (scaleDown * (i * 12)) + (i * 29), 0x200);
				hudLetters[i]->matrix.t[1] = UI_ConvertY_2(letterPos[1] - (i & 1), 0x200);
			}

			if (updateToken)
			{
				hudR->unk50 = 1;
				hudToken->flags &= ~HIDE_MODEL;
				hudToken->matrix.t[0] = hudT->matrix.t[0];
				hudToken->matrix.t[1] = UI_ConvertY_2(letterPos[1] + 0x18, 0x200);

				if ((awardTextFrame >= 0) && (hudToken->scale[0] < 0x2001))
				{
					hudToken->scale[0] += 0x200;
					hudToken->scale[1] += 0x200;
					hudToken->scale[2] += 0x200;
				}

				if (awardTextFrame >= 0)
				{
					UI_Lerp2D_Linear(&txtPos[0], txtStartX, 0xa6, txtEndX, 0xa6, awardTextFrame, 8);

					txtColor = (gGT->timer & 1) ? 0xFFFF8003 : 0xFFFF8004;

					DecalFont_DrawLine(sdata->lngStrings[0x16F], txtPos[0], txtPos[1], 1, txtColor);
				}
			}
		}

		// If you did not collect all 3 letters (C, T, and R), or you lost the race.
		else
		{
			driver->PickupLetterHUD.numCollected = 0;

			// Do this for the first 30 seconds (900 frames).
			if (elapsedFrames < 900)
			{
				for (i = 0; i < 3; i++)
				{
					if (
					    // letter is visible
					    ((hudLetters[i]->flags & HIDE_MODEL) == 0) &&

					    // delay letter (6 frames apart)
					    (elapsedFrames > 6 * i) &&

					    // letter not fully off-screen
					    (-300 < hudLetters[i]->matrix.t[1]))
					{
						letter = hudLetters[i]->thread->object;

						// move X position (yes, C-Letter only, Naughty Dog bug?)
						hudLetters[0]->matrix.t[0] += letter->vel[0];

						// make the letter fall off the screen
						hudLetters[i]->matrix.t[1] -= letter->vel[1];

						if (-0x14 < letter->vel[1])
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
		boolWin = boolTokenAward;

	for (i = 0; i < numPlyr; i++)
	{
		// Draw how much time it took to finish laps and race
		AA_EndEvent_DisplayTime(i, lerpEndY);
	}

	elapsedFrames = sdata->framesSinceRaceEnded;

	// If it hasn't been 1 second from race ended
	if (elapsedFrames < 30)
		return;

	// If there is one player
	if (numPlyr == 1)
	{
		// start counting time 1 second after race ends
		t = (elapsedFrames & 0xffff) - 30;

		if (
		    // Every 0.5 seconds or so
		    ((t % 10 & 0xffff) == 0) &&

		    // sdata->numIconsEOR is the number of icons being
		    // drawn on the end-of-race menu in 1P mode

		    // If you have not drawn all drivers yet
		    (sdata->numIconsEOR < totalPlyr))
		{
			// add an icon to draw
			sdata->numIconsEOR++;
		}

		// loop through all the driver icons
		for (i = 0; i < sdata->numIconsEOR; i++)
		{
			int iVar11 = gGT->pushBuffer[0].rect.x + (gGT->pushBuffer[0].rect.w - totalPlyr * 56 + 12) / 2 + (i * 56);

			if (elapsedFrames + lerpEndY > 300)
			{
				lerpStartX = iVar11;
				lerpEndX = -100;
				currFrame = elapsedFrames + lerpEndY - 300;
			}
			else
			{
				lerpStartX = 0x218;
				lerpEndX = iVar11;
				currFrame = t;
			}

			t -= 10;

			// interpolate fly-in
			UI_Lerp2D_Linear(&letterPos[0], lerpStartX, 0x60, lerpEndX, 0x60, currFrame, 10);

			str_number222 = (char)i + '1';

			// print a single character, a number 1-8,
			DecalFont_DrawLine((char *)&str_number222, letterPos[0] + 0x20, 0x5f, 2, 4);

			// Draw the driver's character icon
			UI_DrawDriverIcon(

			    gGT->ptrIcons[data.MetaDataCharacters[data.characterIDs[gGT->driversInRaceOrder[i]->driverID]].iconID],

			    letterPos[0], 0x60, &gGT->backBuffer->primMem,

			    // pointer to OT mem
			    gGT->pushBuffer_UI.ptrOT,

			    1, 0x1000, MakeColor(0x80, 0x80, 0x80).self);
		}
	}

	// 0x78 + 0x6e = 0xe6 (230) frames waited for Token Race
	if ((elapsedFrames - lerpStartY) < 110)
		return;
	if (
	    // If you are in Adventure cup
	    ((gGT->gameMode1 & ADVENTURE_CUP) != 0) ||

	    // If you are in Arcade or VS cup
	    ((gGT->gameMode2 & CUP_ANY_KIND) != 0))
	{
		// but text near middle of screen
		s16 posX = (numPlyr < 2) ? 0xbe : 100;

		// PRESS * TO CONTINUE
		DecalFont_DrawLine(sdata->lngStrings[0xC9], 0x100, posX, 1, 0xffff8000);

		// If you do not "Press X to continue"
		if ((sdata->AnyPlayerTap & 0x50) == 0)
			return;

		// If you are here, it means you pressed X to continue

		// clear gamepad input
		RECTMENU_ClearInput();

		sdata->menuReadyToPass = 0;
		sdata->framesSinceRaceEnded = 0;
		sdata->numIconsEOR = 1;

		// Disable HUD
		gGT->hudFlags &= 0xfe;

		// Enable Cup Standings
		gGT->hudFlags |= 4;
		return;
	}

	// if the menu is already drawing
	if (sdata->menuReadyToPass & 1)
		return;

	// If you're in Arcade mode
	if ((gGT->gameMode1 & ARCADE_MODE) != 0)
	{
		RECTMENU_Show((numPlyr == 1) ? &menu222 : &menu222_2P);

		// record that the menu is drawing
		sdata->menuReadyToPass |= 1;
		return;
	}

	// If you are in adventure mode
	if ((gGT->gameMode1 & ADVENTURE_MODE) == 0)
		return;

	// PRESS * TO CONTINUE
	DecalFont_DrawLine(sdata->lngStrings[0xc9], 0x100, 0xbe, 1, 0xffff8000);

	// If you have not pressed X
	if ((sdata->AnyPlayerTap & 0x50) == 0)
		return;

	// === If Pressed X ===

	RECTMENU_ClearInput();

	sdata->Loading.OnBegin.AddBitsConfig0 |= ADVENTURE_ARENA;
	sdata->Loading.OnBegin.RemBitsConfig0 |= ADVENTURE_BOSS;
	sdata->Loading.OnBegin.RemBitsConfig8 |= TOKEN_RACE;

	// If you are in boss mode
	if (gGT->gameMode1 < 0)
	{
		sdata->Loading.OnBegin.AddBitsConfig8 |= SPAWN_AT_BOSS;
	}

	if (!boolWin)
	{
		RECTMENU_Show(&data.menuRetryExit);
		sdata->menuReadyToPass |= 1;
		return;
	}

	// === If you won the race ===

	sdata->framesSinceRaceEnded = 0;
	sdata->numIconsEOR = 1;

	// Load the levelID for Adventure Hub that you came from
	levSpawn = gGT->prevLEV;

	// If you are in boss mode
	if (gGT->gameMode1 < 0)
	{
		// bitIndex of keys unlocked, and boss beaten
		bitIndex = gGT->bossID + 0x5e;

		// If the number of keys you have is less than 4
		if (gGT->bossID < 4)
		{
			// only if first time beating boss
			if (CHECK_ADV_BIT(adv->rewards, bitIndex) == 0)
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

			// assume oxide beaten 1st time
			adv->rewards[3] |= 0x80004;

			// if beaten oxide 2nd time
			if (gGT->bossID == 5)
			{
				// beat 2nd time
				adv->rewards[3] |= 0x100008;
			}
		}
	}

	// if something needs unlocking
	if (bitIndex > 0)
	{
		// Unlock reward
		UNLOCK_ADV_BIT(adv->rewards, bitIndex);
	}

	if (gGT->gameMode1 < 0)
	{
		MainRaceTrack_RequestLoad(levSpawn);
		return;
	}

	// if trophy is not won,
	// Dingo Bingo needs to win trophy and token in the same race
	bitIndex = gGT->levelID + 6;
	if (CHECK_ADV_BIT(adv->rewards, bitIndex) == 0)
	{
		// unlock tropy
		UNLOCK_ADV_BIT(adv->rewards, bitIndex);

		// go to podium with trophy
		gGT->podiumRewardID = STATIC_TROPHY;
	}

	MainRaceTrack_RequestLoad(levSpawn);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800a06f8-0x800a0b38.
void AA_EndEvent_DisplayTime(s16 driverId, s16 param_2)
{
	struct GameTracker *gGT;
	struct Driver *driver;
	struct UiElement2D *hudArray;
	struct UiElement2D *hud;
	struct Instance *bigNum;
	char numPlyr, tenseconds;
	s16 framesElapsed;
	s16 lerpStartY;
	s16 lerpEndY;
	s16 lerpStartX;
	s16 lerpEndX;
	s16 currFrame;
	s16 width;
	s16 endFrame;
	s16 posXY[2];
	RECT r;

	gGT = sdata->gGT;
	driver = gGT->drivers[driverId];

	numPlyr = gGT->numPlyrCurrGame;
	hudArray = data.hudStructPtr[numPlyr - 1];
	hud = &hudArray[driverId * 0x14]; // to-do, use enum where 0x14 is number of hud
	bigNum = driver->instBigNum;

	// Lap time box height
	switch (gGT->numLaps)
	{
	// based on number of laps
	case 7:
		r.h = 0x49;
		break;
	case 5:
		r.h = 0x39;
		break;
	default:
		// default height for 1/3 laps.
		r.h = 0x44;
		break;
	}

	// increment counter for number of frames since the player ended the race
	driver->framesSinceRaceEnded_forThisDriver++;
	framesElapsed = driver->framesSinceRaceEnded_forThisDriver;

	if (
	    // if player ended race less than 110 frames ago
	    (framesElapsed < 110) &&

	    // If you press Cross or Circle
	    ((sdata->AnyPlayerTap & 0x50) != 0) &&

	    // only one player
	    (numPlyr == 1))
	{
		// Assume race ended 110 frames ago
		framesElapsed = 110;
		sdata->framesSinceRaceEnded = framesElapsed;
		driver->framesSinceRaceEnded_forThisDriver = framesElapsed;

		sdata->numIconsEOR = numPlyr + gGT->numBotsNextGame;

		// clear gamepad input (for menus)
		RECTMENU_ClearInput();
	}

	tenseconds = (framesElapsed + param_2 > 300);

	// === Draw BigNum ===

	// Player 2
	lerpEndY = 0x41;

	if (driverId == 0)
		lerpEndY = -0x3d;

	// If race ended more than 10 seconds ago.
	if (tenseconds)
	{
		currFrame = framesElapsed + param_2 - 300;
		endFrame = 0xf;

		lerpStartX = -0xae;
		lerpEndX = UI_ConvertX_2(-100, hud[2].z);
		lerpStartY = lerpEndY;
	}

	// If not
	else
	{
		currFrame = framesElapsed;
		endFrame = 0x1e;

		lerpStartX = UI_ConvertX_2(hud[2].x, hud[2].z);
		lerpStartY = UI_ConvertY_2(hud[2].y, hud[2].z);
		lerpEndX = -0xae;
	}

	// interpolate fly-in positionXY
	UI_Lerp2D_Linear(&posXY[0], lerpStartX, lerpStartY, lerpEndX, lerpEndY, currFrame, endFrame);

	bigNum->matrix.t[0] = posXY[0];
	bigNum->matrix.t[1] = posXY[1];

	// interpolate scale to 0x1e00
	UI_Lerp2D_Linear(&posXY[0], hud[2].scale, 0, 0x1e00, 0, framesElapsed, 30);

	bigNum->scale[0] = posXY[0];
	bigNum->scale[1] = posXY[0];
	bigNum->scale[2] = posXY[0];

	// === Draw Suffix ===

	// Player 2
	lerpEndY = 0x89;

	if (driverId == 0)
		lerpEndY = 9;

	if (tenseconds)
	{
		lerpStartX = 0x78;
		lerpStartY = lerpEndY;
		lerpEndX = -0x3c;
	}
	else
	{
		lerpStartX = hud[5].x;
		lerpStartY = hud[5].y;
		lerpEndX = 0x78;
	}

	UI_Lerp2D_Linear(&posXY[0], lerpStartX, lerpStartY, lerpEndX, lerpEndY, currFrame, endFrame);

	UI_DrawPosSuffix(posXY[0], posXY[1], driver, 0);

	// === DrawRaceClock ===

	lerpEndY = 0xc3;
	if (driverId == 0)
		lerpEndY = 0x3e;

	if (tenseconds)
	{
		lerpStartX = 0x150;
		lerpEndX = 0x27c;
	}

	else
	{
		lerpStartX = 0x218;
		lerpEndX = 0x150;
	}

	UI_Lerp2D_Linear(&posXY[0], lerpStartX, lerpEndY, lerpEndX, lerpEndY, currFrame, endFrame);

	UI_DrawRaceClock(posXY[0], posXY[1], 1, driver);

	// "TOTAL"
	width = DecalFont_GetLineWidth(sdata->lngStrings[0xc4], 1);

	r.x = (posXY[0] - width) + -6;
	r.y = (posXY[1] - r.h) + 0xd;
	r.w = width + 0x94;
	r.h += 6;

	// Draw 2D Menu rectangle background
	RECTMENU_DrawInnerRect(&r, 4, gGT->backBuffer->otMem.startPlusFour);
	return;
}

struct MenuRow rows222[5] = {
    // Retry
    {
        .stringIndex = 4,
        .rowOnPressUp = 0,
        .rowOnPressDown = 1,
        .rowOnPressLeft = 0,
        .rowOnPressRight = 0,
    },
    // Change Level
    {
        .stringIndex = 6,
        .rowOnPressUp = 0,
        .rowOnPressDown = 2,
        .rowOnPressLeft = 1,
        .rowOnPressRight = 1,
    },
    // Change Character
    {
        .stringIndex = 5,
        .rowOnPressUp = 1,
        .rowOnPressDown = 3,
        .rowOnPressLeft = 2,
        .rowOnPressRight = 2,
    },
    // Quit
    {
        .stringIndex = 3,
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
