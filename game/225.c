#include <common.h>

global_variable struct RectMenu menuVS;
global_variable struct RectMenu menuBattle;

enum VersusBattleStandingsPosY
{
	VB_POSY_TITLE = 0,
	VB_POSY_P1,
	VB_POSY_P2,
	VB_POSY_P3,
	VB_POSY_P4,
	VB_POSY_NUM
};

enum VersusBattleEndMenuConstants
{
	VB_MAX_PLAYERS = 4,
	VB_MIN_PLAYERS = 2,
	VB_RESULT_MAX_FRAMES = CTR_SECONDS_TO_FRAMES(8),
	VB_MENU_SHOW_DELAY_FRAMES = 25,
	VB_ROW_INITIAL_DELAY_FRAMES = 30,
	VB_ROW_STAGGER_FRAMES = 5,
	VB_LERP_FRAMES = 5,

	VB_TITLE_ENTRY_X = 0x296,
	VB_TITLE_TARGET_X = 0x180,
	VB_ROW_TARGET_X = 0x165,
	VB_TITLE_TO_ROWS_Y = 0x28,
	VB_STANDINGS_SUBTITLE_Y = 0x11,

	VB_BATTLE_BLOCK_BOTTOM_Y = 0xd8,
	VB_BATTLE_TEAM_SCORE_GAP = 10,
	VB_BATTLE_TITLE_PLAYER_HEIGHT = 0x1a,
	VB_BATTLE_PLAYER_ICON_SPACING = 0x1b,
	VB_BATTLE_BLOCK_HEADER_HEIGHT = 0x28,
	VB_BATTLE_RANK_TEXT_CENTER_Y = 0xd,

	VB_STANDINGS_VISIBLE_PLACES_MIN = 2,
	VB_STANDINGS_POINTS_PER_ENTRY = 3,
	VB_STANDINGS_TEXT_X_OFFSET = 0x79,
	VB_STANDINGS_RANK_X_OFFSET = -0x24,
	VB_STANDINGS_RANK_Y_OFFSET = 5,
	VB_STANDINGS_ROW_HALF_HEIGHT = 4,
	VB_STANDINGS_ROW_BASELINE_BIAS = 0xd,
	VB_STANDINGS_SUFFIX_FIRST = LNG_ST,

	VB_ICON_TRANSPARENCY = 1,
	VB_ICON_SCALE = 0x1000,

	VB_WINNER_TARGET_X = 0x14,
	VB_WINNER_TARGET_Y = 0xc,
	VB_WINNER_LERP_FRAMES = 25,
	VB_WINNER_2P_MIN_WIDE_RECT = 0x100,
	VB_WINNER_2P_WIDTH_STEP = 0xc,
	VB_WINNER_DISTANCE_TO_SCREEN = 0x80,
	VB_WINNER_BOX_X_PAD = 3,
	VB_WINNER_BOX_Y_PAD = 2,

	VB_LOSER_RECT_STEP_X = 5,
	VB_LOSER_RECT_STEP_Y = 3,
	VB_LOSER_RECT_STEP_W = 10,
	VB_LOSER_RECT_STEP_H = 6,

	VB_MENU_READY_SHOW_MENU = 1,
	VB_RECTMENU_STATE = RECTMENU_UNKNOWN_0x800 | EXECUTE_FUNCPTR | USE_SMALL_FONT | CENTER_ON_COORDS,
};

// 2P, 3P, 4P
global_variable s16 s_vsStandingsYByPlayerCount[3][VB_POSY_NUM] = {
    {0x32, 0x5a, 0x82, 0, 0},     // 2P
    {0x1e, 0x46, 0x6e, 0x96, 0},  // 3P
    {0xa, 0x35, 0x5b, 0x81, 0xa7} // 4P
};

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8009f718-0x800a0208.
void VB_EndEvent_DrawMenu(void)
{
	char text[24];
	SVec2 pos;

	struct GameTracker *gGT = sdata->gGT;
	s32 numPlayers = gGT->numPlyrCurrGame;
	s32 playerCountIndex = numPlayers - VB_MIN_PLAYERS;
	b32 isBattleMode = (gGT->gameMode1 & BATTLE_MODE) != 0;

	if (sdata->framesSinceRaceEnded < VB_RESULT_MAX_FRAMES)
	{
		sdata->framesSinceRaceEnded++;
	}

	s32 titleString = LNG_BATTLE;
	s32 standingsEntryCount;
	s16 titleY;
	s16 teamPlayerCount[VB_MAX_PLAYERS] = {0, 0, 0, 0};
	if (!isBattleMode)
	{
		titleString = LNG_VERSUS;
		standingsEntryCount = numPlayers;
		titleY = s_vsStandingsYByPlayerCount[playerCountIndex][VB_POSY_TITLE];
	}
	else
	{
		standingsEntryCount = gGT->battleSetup.numTeams;

		for (s32 player = 0; player < numPlayers; player++)
		{
			teamPlayerCount[gGT->drivers[player]->BattleHUD.teamID]++;
		}

		titleY = (VB_BATTLE_BLOCK_BOTTOM_Y -
		          ((gGT->battleSetup.numTeams - 1) * VB_BATTLE_TEAM_SCORE_GAP + numPlayers * VB_BATTLE_TITLE_PLAYER_HEIGHT + VB_BATTLE_BLOCK_HEADER_HEIGHT)) >>
		         1;
	}

	// Disable drawing lines between multiplayer screens
	gGT->renderFlags &= ~RENDER_FLAG_SPLIT_SCREEN_LINES;

	RaceFlag_SetFullyOnScreen();

	s32 titleTargetX;
	s32 titleFrame;
	if (sdata->framesSinceRaceEnded <= VB_MENU_SHOW_DELAY_FRAMES)
	{
		titleTargetX = VB_TITLE_ENTRY_X;
		titleFrame = sdata->framesSinceRaceEnded;
	}
	else
	{
		titleTargetX = VB_TITLE_TARGET_X;
		titleFrame = sdata->framesSinceRaceEnded - VB_MENU_SHOW_DELAY_FRAMES;
	}

	// fly-in interpolation
	UI_Lerp2D_Linear(pos.v, VB_TITLE_ENTRY_X, titleY, titleTargetX, titleY, titleFrame, VB_LERP_FRAMES);

	s32 rowY = titleY + VB_TITLE_TO_ROWS_Y;

	// "Versus" or "Battle"
	DecalFont_DrawLine(sdata->lngStrings[titleString], pos.x, pos.y, FONT_BIG, (JUSTIFY_CENTER | ORANGE));

	DecalFont_DrawLine(sdata->lngStrings[LNG_STANDINGS], pos.x, pos.y + VB_STANDINGS_SUBTITLE_Y, FONT_BIG, (JUSTIFY_CENTER | ORANGE));

	s32 visiblePlaces = VB_STANDINGS_VISIBLE_PLACES_MIN;
	if (standingsEntryCount >= 3)
	{
		visiblePlaces = standingsEntryCount - 1;
	}

	s32 rowDelay = VB_ROW_INITIAL_DELAY_FRAMES;
	s16 displayedRankOffset = 0;
	s16 previousStandingsScore = 0;
	for (s32 standingsIndex = 0; standingsIndex < standingsEntryCount; standingsIndex++)
	{
		s32 entityID = gGT->battleSetup.standingsOrder[standingsIndex];
		s16 currRowY = (s16)rowY;

		s32 rowTargetX;
		s32 rowFrame;
		if (sdata->framesSinceRaceEnded > rowDelay)
		{
			rowFrame = sdata->framesSinceRaceEnded - rowDelay;
			rowTargetX = VB_ROW_TARGET_X;
		}
		else
		{
			rowFrame = sdata->framesSinceRaceEnded;
			rowTargetX = VB_TITLE_ENTRY_X;
		}

		// fly-in interpolation
		UI_Lerp2D_Linear(pos.v, VB_TITLE_ENTRY_X, currRowY, rowTargetX, currRowY, rowFrame, VB_LERP_FRAMES);

		s16 rankTextY;
		if (!isBattleMode)
		{
			rankTextY = s_vsStandingsYByPlayerCount[playerCountIndex][VB_POSY_P1 + standingsIndex];

			struct Driver *driver = gGT->drivers[entityID];
			struct Icon *icon = gGT->ptrIcons[data.MetaDataCharacters[data.characterIDs[driver->driverID]].iconID];

			DecalHUD_DrawPolyFT4(icon, pos.x, rankTextY, &gGT->backBuffer->primMem, gGT->pushBuffer_UI.ptrOT, VB_ICON_TRANSPARENCY, VB_ICON_SCALE);
		}
		else
		{
			s16 numPlayersOnTeam = teamPlayerCount[entityID];
			rankTextY = currRowY + (numPlayersOnTeam * VB_BATTLE_PLAYER_ICON_SPACING) / 2 - VB_BATTLE_RANK_TEXT_CENTER_Y;

			s16 iconSlot = 0;
			for (s32 player = 0; player < numPlayers; player++)
			{
				struct Driver *driver = gGT->drivers[player];

				if (driver->BattleHUD.teamID != entityID)
				{
					continue;
				}

				struct Icon *icon = gGT->ptrIcons[data.MetaDataCharacters[data.characterIDs[driver->driverID]].iconID];
				DecalHUD_DrawPolyFT4(icon, pos.x, currRowY + iconSlot * VB_BATTLE_PLAYER_ICON_SPACING, &gGT->backBuffer->primMem, gGT->pushBuffer_UI.ptrOT,
				                     VB_ICON_TRANSPARENCY, VB_ICON_SCALE);
				iconSlot++;
			}

			rowY += numPlayersOnTeam * VB_BATTLE_PLAYER_ICON_SPACING + VB_BATTLE_TEAM_SCORE_GAP;
		}

		for (s32 place = 0; place < visiblePlaces; place++)
		{
			s32 entityRank = isBattleMode ? gGT->battleSetup.finishedRankOfEachTeam[entityID] : gGT->drivers[entityID]->driverRank;

			s32 placeTextColor = JUSTIFY_RIGHT | RED;
			if (place == entityRank)
			{
				placeTextColor = (gGT->timer & 1) ? (JUSTIFY_RIGHT | RED) : (JUSTIFY_RIGHT | WHITE);
			}

			sprintf(text, "%d%s-%2.02ld", place + 1, sdata->lngStrings[VB_STANDINGS_SUFFIX_FIRST + place],
			        CTR_PRINTF_PSX_LONG(gGT->standingsPoints[entityID * VB_STANDINGS_POINTS_PER_ENTRY + place]));

			DecalFont_DrawLine(text, pos.x + VB_STANDINGS_TEXT_X_OFFSET,
			                   rankTextY - (visiblePlaces * VB_STANDINGS_ROW_HALF_HEIGHT - VB_STANDINGS_ROW_BASELINE_BIAS) + place * 8, FONT_SMALL,
			                   placeTextColor);
		}

		if (gGT->battleSetup.standingsScore[entityID] == previousStandingsScore)
		{
			displayedRankOffset++;
		}
		else
		{
			displayedRankOffset = 0;
		}

		s32 displayedRank = standingsIndex - displayedRankOffset;

		rowDelay += VB_ROW_STAGGER_FRAMES;

		previousStandingsScore = (s16)gGT->battleSetup.standingsScore[entityID];
		sprintf(text, "%d%s", displayedRank + 1, sdata->lngStrings[VB_STANDINGS_SUFFIX_FIRST + displayedRank]);

		DecalFont_DrawLine(text, pos.x + VB_STANDINGS_RANK_X_OFFSET, rankTextY + VB_STANDINGS_RANK_Y_OFFSET, FONT_BIG, (JUSTIFY_CENTER | ORANGE));
	}

	b32 winnerViewportFound = false;
	for (s32 player = 0; player < numPlayers; player++)
	{
		struct Instance *bigNum = gGT->drivers[player]->instBigNum;
		struct PushBuffer *view = &gGT->pushBuffer[player];

		if (bigNum != NULL)
		{
			bigNum->scale = (SVec3){{0, 0, 0}};
		}

		s32 winnerDriverID = isBattleMode ? gGT->winnerIndex[0] : gGT->driversInRaceOrder[0]->driverID;
		b32 isWinnerViewport = !winnerViewportFound && (winnerDriverID == player);

		if (isWinnerViewport)
		{
			winnerViewportFound = true;

			if ((numPlayers == 2) && (view->rect.w > VB_WINNER_2P_MIN_WIDE_RECT))
			{
				view->rect.w -= VB_WINNER_2P_WIDTH_STEP;
				view->distanceToScreen_CURR = VB_WINNER_DISTANCE_TO_SCREEN;
			}

			// fly-in interpolation
			UI_Lerp2D_Linear(pos.v, view->rect.x, view->rect.y, VB_WINNER_TARGET_X, VB_WINNER_TARGET_Y, sdata->framesSinceRaceEnded, VB_WINNER_LERP_FRAMES);

			RECT box;
			box.x = pos.x - VB_WINNER_BOX_X_PAD;
			box.y = pos.y - VB_WINNER_BOX_Y_PAD;
			box.w = view->rect.w + 2 * VB_WINNER_BOX_X_PAD;
			box.h = view->rect.h + 2 * VB_WINNER_BOX_Y_PAD;

			Color color;
			color.self = sdata->battleSetup_Color_UI_1;
			RECTMENU_DrawOuterRect_HighLevel(&box, color, 0, gGT->backBuffer->otMem.uiOT);

			view->rect.x = pos.x;
			view->rect.y = pos.y;
		}
		else
		{
			if (view->rect.w > 0)
			{
				view->rect.x += VB_LOSER_RECT_STEP_X;
				view->rect.y += VB_LOSER_RECT_STEP_Y;
				view->rect.w -= VB_LOSER_RECT_STEP_W;
				view->rect.h -= VB_LOSER_RECT_STEP_H;
			}
		}
	}

	if (((sdata->menuReadyToPass & VB_MENU_READY_SHOW_MENU) == 0) && (VB_MENU_SHOW_DELAY_FRAMES < sdata->framesSinceRaceEnded))
	{
		struct RectMenu *endMenu = isBattleMode ? &menuBattle : &menuVS;

		// Make Menu Box appear based on the game mode
		RECTMENU_Show(endMenu);

		sdata->menuReadyToPass |= VB_MENU_READY_SHOW_MENU;
	}
}

global_variable struct MenuRow rowsVS[5] = {
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
        .stringIndex = RECTMENU_STRING_NONE,
        .rowOnPressUp = 0,
        .rowOnPressDown = 0,
        .rowOnPressLeft = 0,
        .rowOnPressRight = 0,
    }};

global_variable struct RectMenu menuVS = {
    .stringIndexTitle = RECTMENU_STRING_NONE,
    .posX_curr = 143,
    .posY_curr = 162,
    .unk1 = 0,
    .state = VB_RECTMENU_STATE,
    .rows = rowsVS,
    .funcPtr = UI_RaceEnd_MenuProc,
    .drawStyle = 4,
    // rest of variables all default zero
};

global_variable struct MenuRow rowsBattle[6] = {
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
    // Change Setup
    {
        .stringIndex = LNG_CHANGE_SETUP,
        .rowOnPressUp = 1,
        .rowOnPressDown = 3,
        .rowOnPressLeft = 2,
        .rowOnPressRight = 2,
    },
    // Change Character
    {
        .stringIndex = LNG_CHANGE_CHARACTER,
        .rowOnPressUp = 2,
        .rowOnPressDown = 4,
        .rowOnPressLeft = 3,
        .rowOnPressRight = 3,
    },
    // Quit
    {
        .stringIndex = LNG_QUIT,
        .rowOnPressUp = 3,
        .rowOnPressDown = 4,
        .rowOnPressLeft = 4,
        .rowOnPressRight = 4,
    },
    // NULL, end of menu
    {
        .stringIndex = RECTMENU_STRING_NONE,
        .rowOnPressUp = 0,
        .rowOnPressDown = 0,
        .rowOnPressLeft = 0,
        .rowOnPressRight = 0,
    }};

global_variable struct RectMenu menuBattle = {
    .stringIndexTitle = RECTMENU_STRING_NONE,
    .posX_curr = 143,
    .posY_curr = 166,
    .unk1 = 0,
    .state = VB_RECTMENU_STATE,
    .rows = rowsBattle,
    .funcPtr = UI_RaceEnd_MenuProc,
    .drawStyle = 4,
    // rest of variables all default zero
};
