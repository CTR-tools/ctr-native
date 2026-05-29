#include <common.h>

extern struct RectMenu menuVS;
extern struct RectMenu menuBattle;

enum VsPosY
{
	VsPosY_TITLE = 0,
	VsPosY_P1,
	VsPosY_P2,
	VsPosY_P3,
	VsPosY_P4,
	VsPosY_NUM
};

// 2P, 3P, 4P
s16 VsPosY_Config[3 * VsPosY_NUM] = {
    0x32, 0x5a, 0x82, 0,    0,   // 2P
    0x1e, 0x46, 0x6e, 0x96, 0,   // 3P
    0xa,  0x35, 0x5b, 0x81, 0xa7 // 4P
};

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8009f718-0x800a0208.
void VB_EndEvent_DrawMenu(void)
{
	struct GameTracker *gGT;
	struct Instance *bigNum;
	struct PushBuffer *view;
	struct RectMenu *endMenu;
	char numPlyr;
	char text[24];
	s16 pos[2];
	RECT box;

	s16 sVar1;
	int iVar2;
	u32 uVar3;
	s16 sVar5;
	int iVar6;
	int uVar7;
	s16 *psVar8;
	s16 sVar9;
	int iVar10;
	int iVar11;
	int iVar14;
	u16 uStack112;
	s16 sStack104;
	u16 uStack96;
	u16 uStack88;
	s16 sStack80;
	s16 sStack72;
	int iStack60;
	u32 uStack52;
	int iStack48;
	int VsConfigIndex;

	sStack104 = 0;
	uStack88 = 1;
	sStack80 = 0;

	s16 asStack128[4];
	*(int *)&asStack128[0] = 0;
	*(int *)&asStack128[2] = 0;

	sStack72 = 0;
	gGT = sdata->gGT;
	numPlyr = gGT->numPlyrCurrGame;
	VsConfigIndex = numPlyr - 2;

	if (sdata->framesSinceRaceEnded < 0xf0)
	{
		sdata->framesSinceRaceEnded++;
	}

	// lng index for BATTLE
	iVar10 = 0x50;

	s16 yCoord;
	// if you are not in battle mode
	if ((gGT->gameMode1 & 0x20) == 0)
	{
		// lng index for VERSUS
		iVar10 = 0x14f;

		// number of battle teams
		uStack96 = numPlyr;

		yCoord = VsPosY_Config[VsPosY_NUM * VsConfigIndex + VsPosY_TITLE];
	}
	// if you are in battle mode
	else
	{
		// number of battle teams
		uStack96 = gGT->battleSetup.numTeams;

		// if you don't have zero players
		for (iVar11 = 0; iVar11 < numPlyr; iVar11++)
		{
			// increment something based on which team each player is on
			asStack128[gGT->drivers[iVar11]->BattleHUD.teamID]++;
		}

		yCoord = 0xd8 - ((gGT->battleSetup.numTeams + -1) * 10 + numPlyr * 0x1a + 0x28) >> 1;
	}

	// Disable drawing lines between multiplayer screens
	gGT->renderFlags &= ~(0x8000);

	RaceFlag_SetFullyOnScreen();

	if (sdata->framesSinceRaceEnded <= 25)
	{
		uVar7 = 0x296;
		iVar11 = sdata->framesSinceRaceEnded;
	}

	else
	{
		uVar7 = 0x180;
		iVar11 = sdata->framesSinceRaceEnded - 25;
	}

	// fly-in interpolation
	UI_Lerp2D_Linear(&pos[0], 0x296, yCoord, uVar7, yCoord, iVar11, 5);

	iVar14 = yCoord + 0x28;

	// "Versus" or "Battle"
	DecalFont_DrawLine(sdata->lngStrings[iVar10], pos[0], pos[1], 1, 0xffff8000);

	// STANDINGS
	DecalFont_DrawLine(sdata->lngStrings[0xCA], pos[0], (pos[1] + 0x11), 1, 0xffff8000);

	iVar10 = uStack96;
	iVar11 = 0;

	{
		iStack60 = iVar10;
		uStack52 = (u32)(iVar10 < 3);

		iStack48 = 30;

		for (iVar11 = 0; iVar11 < iStack60; iVar11++)
		{
			sVar1 = asStack128[gGT->battleSetup.unk1dc8[iVar11]];
			sVar5 = (s16)iVar14;

			if (sdata->framesSinceRaceEnded > iStack48)
			{
				iVar10 = sdata->framesSinceRaceEnded - iStack48;

				uVar7 = 0x165;
			}

			else
			{
				iVar10 = sdata->framesSinceRaceEnded;

				uVar7 = 0x296;
			}

			// fly-in interpolation
			UI_Lerp2D_Linear(&pos[0], 0x296, sVar5, uVar7, sVar5, iVar10, 5);

			sVar9 = 0;

			// If you are not in battle mode
			if ((gGT->gameMode1 & 0x20) == 0)
			{
				uStack112 = VsPosY_Config[VsPosY_NUM * VsConfigIndex + uStack88];

				// Draw character icon
				DecalHUD_DrawPolyFT4(gGT->ptrIcons[data.MetaDataCharacters[data.characterIDs[gGT->drivers[gGT->battleSetup.unk1dc8[iVar11]]->driverID]].iconID],

				                     // Position X and Position Y
				                     pos[0], uStack112,

				                     // pointer to PrimMem struct
				                     &gGT->backBuffer->primMem,

				                     // pointer to OT mem
				                     gGT->pushBuffer_UI.ptrOT,

				                     1, 0x1000);
			}

			// if battle mode
			else
			{
				iVar10 = sVar1 * 0x1b0000;
				uStack112 = (sVar5 + (s16)((iVar10 >> 0x10) - (iVar10 >> 0x1f) >> 1)) - 0xd;

				// loop through all players
				for (iVar10 = 0; iVar10 < numPlyr; iVar10++)
				{
					// player -> battleTeam == (teamID)
					if (gGT->drivers[iVar10]->BattleHUD.teamID == gGT->battleSetup.unk1dc8[iVar11])
					{
						iVar6 = sVar9;
						sVar9 = sVar9 + 1;

						DecalHUD_DrawPolyFT4(gGT->ptrIcons[data.MetaDataCharacters[data.characterIDs[gGT->drivers[iVar10]->driverID]].iconID], pos[0],
						                     sVar5 + iVar6 * 0x1b,

						                     // pointer to PrimMem struct
						                     &gGT->backBuffer->primMem,

						                     // pointer to OT mem
						                     gGT->pushBuffer_UI.ptrOT,

						                     1, 0x1000);
					}
				}

				iVar14 = sVar1 * 0x1b + iVar14 + 10;
			}

			uStack88 = uStack88 + 1;
			sVar1 = 2;
			if (uStack52 == 0)
			{
				sVar1 = uStack96 - 1;
			}
			iVar6 = sVar1;
			if (0 < iVar6)
			{
				for (iVar10 = 0; iVar10 < iVar6; iVar10++)
				{
					// If you are not in Battle Mode
					if ((gGT->gameMode1 & 0x20) == 0)
					{
						// get position player finished in race
						iVar2 = gGT->drivers[gGT->battleSetup.unk1dc8[iVar11]]->driverRank;
					}
					// If you are in Battle Mode
					else
					{
						// get position of the team in battle
						iVar2 = gGT->battleSetup.finishedRankOfEachTeam[gGT->battleSetup.unk1dc8[iVar11]];
					}

					uVar7 = 0x4003;
					if ((iVar10 == iVar2) && (uVar7 = 0x4004,
					                          // odd number frames
					                          (gGT->timer & 1) != 0))
					{
						uVar7 = 0x4003;
					}

					iVar2 = iVar10 + 1;

					// string for each player rank and count from standings (0x1e80)
					sprintf(text, "%d%s-%2.02ld", iVar2, sdata->lngStrings[0x19 + iVar10],
					        (gGT->standingsPoints[gGT->battleSetup.unk1dc8[iVar11] * 3 + iVar10]));

					// Draw string
					DecalFont_DrawLine(text, (pos[0] + 0x79), ((uStack112 - (iVar6 * 4 + -0xd)) + iVar10 * 8), 2, uVar7);
				}
			}

			if (gGT->battleSetup.unk_afterTeams[gGT->battleSetup.unk1dc8[iVar11]] == sStack80)
			{
				sStack72 = sStack72 + 1;
			}
			else
			{
				sStack72 = 0;
			}

			sVar1 = (s16)iVar11 - sStack72;
			if (sStack72 == 0)
			{
				sVar1 = (s16)iVar11;
			}

			iStack48 = iStack48 + 5;

			sStack80 = gGT->battleSetup.unk_afterTeams[gGT->battleSetup.unk1dc8[iVar11]];
			sprintf(text, "%d%s", sVar1 + 1, sdata->lngStrings[0x19 + sVar1]);

			// Draw String
			DecalFont_DrawLine(text, (pos[0] - 0x24), (uStack112 + 5), 1, 0xffff8000);
		}
	}

	for (u8 plCount = 0; plCount < (u8)numPlyr; plCount++)
	{
		// get pointer to instance of Big Number in HUD
		bigNum = gGT->drivers[plCount]->instBigNum;
		view = &gGT->pushBuffer[plCount];

		// if the pointer is valid
		if (bigNum != NULL)
		{
			// clear the scale
			bigNum->scale[0] = 0;
			bigNum->scale[1] = 0;
			bigNum->scale[2] = 0;
		}

		// Winner not found
		if (sStack104 == 0)
		{
			// if this is not battle mode, get first place racer, else get battle winner
			uVar3 = ((gGT->gameMode1 & 0x20) == 0) ? gGT->driversInRaceOrder[0]->driverID : gGT->winnerIndex[0];

			if (uVar3 != plCount)
				goto LAB_8009ff4c;

			// === PushBuffer is the winner ===

			sStack104 = 1;

			if (
			    // If there are two players
			    (numPlyr == 2) &&

			    (0x100 < view->rect.w))
			{
				view->rect.w -= 0xc;
				view->distanceToScreen_CURR = 0x80;
			}

			// fly-in interpolation
			UI_Lerp2D_Linear(&pos[0], view->rect.x, view->rect.y, 0x14, 0xc, sdata->framesSinceRaceEnded, 25);

			box.x = pos[0] - 3;
			box.y = pos[1] - 2;
			box.w = view->rect.w + 6;
			box.h = view->rect.h + 4;

			Color color;
			color.self = sdata->battleSetup_Color_UI_1;
			RECTMENU_DrawOuterRect_HighLevel(&box, color, 0, gGT->backBuffer->otMem.startPlusFour);

			view->rect.x = pos[0];
			view->rect.y = pos[1];
		}

		// === Winner already found ===
		else
		{
		// === PushBuffer is not winner ===
		LAB_8009ff4c:

			if (view->rect.w > 0)
			{
				view->rect.x += 5;
				view->rect.y += 3;
				view->rect.w -= 10;
				view->rect.h -= 6;
			}
		}
	}

	if (((sdata->menuReadyToPass & 1) == 0) && (25 < sdata->framesSinceRaceEnded))
	{
		// if you're in battle mode.
		endMenu = ((gGT->gameMode1 & 0x20) == 0) ? &menuVS : &menuBattle;

		// Make Menu Box appear based on the game mode
		RECTMENU_Show(endMenu);

		sdata->menuReadyToPass |= 1;
	}
}

struct MenuRow rowsVS[5] = {
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

struct RectMenu menuVS = {
    .stringIndexTitle = 0xFFFF,
    .posX_curr = 143,
    .posY_curr = 162,
    .unk1 = 0,
    .state = (0x800 | EXECUTE_FUNCPTR | USE_SMALL_FONT | CENTER_ON_COORDS), // 0xC83
    .rows = rowsVS,
    .funcPtr = UI_RaceEnd_MenuProc,
    .drawStyle = 4,
    // rest of variables all default zero
};

struct MenuRow rowsBattle[6] = {
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
    // Change Setup
    {
        .stringIndex = 10,
        .rowOnPressUp = 1,
        .rowOnPressDown = 3,
        .rowOnPressLeft = 2,
        .rowOnPressRight = 2,
    },
    // Change Character
    {
        .stringIndex = 5,
        .rowOnPressUp = 2,
        .rowOnPressDown = 4,
        .rowOnPressLeft = 3,
        .rowOnPressRight = 3,
    },
    // Quit
    {
        .stringIndex = 3,
        .rowOnPressUp = 3,
        .rowOnPressDown = 4,
        .rowOnPressLeft = 4,
        .rowOnPressRight = 4,
    },
    // NULL, end of menu
    {
        .stringIndex = 0xFFFF,
        .rowOnPressUp = 0,
        .rowOnPressDown = 0,
        .rowOnPressLeft = 0,
        .rowOnPressRight = 0,
    }};

struct RectMenu menuBattle = {
    .stringIndexTitle = 0xFFFF,
    .posX_curr = 143,
    .posY_curr = 166,
    .unk1 = 0,
    .state = (0x800 | EXECUTE_FUNCPTR | USE_SMALL_FONT | CENTER_ON_COORDS), // 0xC83
    .rows = rowsBattle,
    .funcPtr = UI_RaceEnd_MenuProc,
    .drawStyle = 4,
    // rest of variables all default zero
};
