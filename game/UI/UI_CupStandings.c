#include <common.h>

enum
{
	UI_CUP_STANDINGS_MENU_READY_END_OPTIONS = 1,
	UI_CUP_STANDINGS_PAGE_TRACK_POINTS = 0x4,
	UI_CUP_STANDINGS_PAGE_OVERALL_POINTS = 0x8,
	UI_CUP_STANDINGS_CONFIRM_BUTTONS = BTN_CROSS_one | BTN_CIRCLE,
	UI_CUP_STANDINGS_QUICK_SKIP_FRAME = 60,
	UI_CUP_STANDINGS_PAGE_TRANSITION_FRAME = 0xf0,
	UI_CUP_STANDINGS_DONE_FRAME = 0x10f,
	UI_CUP_STANDINGS_OVERALL_HOLD_FRAME = 400,
	UI_CUP_STANDINGS_ICON_REVEAL_STEP_FRAMES = 10,
	UI_CUP_STANDINGS_LERP_FRAMES = 0x14,
	UI_CUP_STANDINGS_TITLE_START_X = -0x96,
	UI_CUP_STANDINGS_TITLE_CENTER_X = 0x100,
	UI_CUP_STANDINGS_TITLE_END_X = 0x296,
	UI_CUP_STANDINGS_TITLE_Y = 0x1e,
	UI_CUP_STANDINGS_TITLE_Y_OFFSET = -0x11,
	UI_CUP_STANDINGS_TRACK_TEXT_Y_OFFSET = 0x11,
	UI_CUP_STANDINGS_PRESS_X = 0x100,
	UI_CUP_STANDINGS_PRESS_Y = 0xbe,
	UI_CUP_STANDINGS_PRESS_PROMPT_FRAME = 0x3b,
	UI_CUP_STANDINGS_PANEL_X = -10,
	UI_CUP_STANDINGS_PANEL_Y = 9,
	UI_CUP_STANDINGS_PANEL_W = 0x214,
	UI_CUP_STANDINGS_PANEL_H = 0x32,
	UI_CUP_STANDINGS_PANEL_MOVE_Y = 0x32,
	UI_CUP_STANDINGS_PURPLE_GEM_CUP_ID = 4,
	UI_CUP_STANDINGS_PURPLE_GEM_VISIBLE_DRIVERS = 5,
	UI_CUP_STANDINGS_TWO_PLAYER_ARCADE_DRIVER_COUNT = 6,
	UI_CUP_STANDINGS_MAX_VISIBLE_AWARD_DRIVERS = 4,
	UI_CUP_STANDINGS_ICON_SLOT_X = 0x60,
	UI_CUP_STANDINGS_ICON_SLOT_W = 0x5a,
	UI_CUP_STANDINGS_ICON_TOP_ROW_Y = 0x42,
	UI_CUP_STANDINGS_ICON_BOTTOM_ROW_Y = 0x79,
	UI_CUP_STANDINGS_ICON_PURPLE_GEM_ROW_Y = 0x60,
	UI_CUP_STANDINGS_ICON_VS_ROW_Y = 0x6c,
	UI_CUP_STANDINGS_ICON_LEFT_MARGIN = 0x20,
	UI_CUP_STANDINGS_ICON_RIGHT_MARGIN = 0x10,
	UI_CUP_STANDINGS_ICON_VS_CENTER_BIAS = 0xc,
	UI_CUP_STANDINGS_PLACE_TEXT_X_OFFSET = 0x20,
	UI_CUP_STANDINGS_PLACE_TEXT_Y_OFFSET = -1,
	UI_CUP_STANDINGS_POINTS_TEXT_X_OFFSET = 0x2d,
	UI_CUP_STANDINGS_POINTS_TEXT_Y_OFFSET = 6,
	UI_CUP_STANDINGS_ICON_SCALE = 0x1000,
	UI_CUP_STANDINGS_ICON_NEUTRAL_CHANNEL = 0x80,
	UI_CUP_STANDINGS_TRACKS_PER_CUP = 4,
	UI_CUP_STANDINGS_DRIVER_SLOTS = 8,
	UI_CUP_STANDINGS_ARCADE_DIFFICULTY_DIVISOR = 0x50,
	UI_CUP_STANDINGS_MAX_ARCADE_DIFFICULTY = 2,
	UI_CUP_STANDINGS_MAX_CUP_LOSSES = 10,
};

CTR_STATIC_ASSERT(UI_CUP_STANDINGS_CONFIRM_BUTTONS == 0x50);
CTR_STATIC_ASSERT(UI_CUP_STANDINGS_PAGE_TRANSITION_FRAME == 0xf0);
CTR_STATIC_ASSERT(UI_CUP_STANDINGS_DONE_FRAME == 0x10f);
CTR_STATIC_ASSERT(UI_CUP_STANDINGS_MENU_READY_END_OPTIONS == 1);
CTR_STATIC_ASSERT(UI_CUP_STANDINGS_PAGE_TRACK_POINTS == 0x4);
CTR_STATIC_ASSERT(UI_CUP_STANDINGS_PAGE_OVERALL_POINTS == 0x8);
CTR_STATIC_ASSERT(UI_CUP_STANDINGS_PURPLE_GEM_CUP_ID == 4);
CTR_STATIC_ASSERT(UI_CUP_STANDINGS_ICON_SCALE == 0x1000);
CTR_STATIC_ASSERT(UI_CUP_STANDINGS_TRACKS_PER_CUP == 4);
CTR_STATIC_ASSERT(UI_CUP_STANDINGS_DRIVER_SLOTS == 8);
CTR_STATIC_ASSERT(GEM_STONE_VALLEY == 0x19);

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8005607c-0x80056220.
void UI_CupStandings_FinalizeCupRanks(void)
{
	struct GameTracker *gGT;
	int numDrivers;
	int tiedTopCount;
	int topScore;
	int bestRank;
	int selectedRankSlot;

	bestRank = 99;
	gGT = sdata->gGT;
	selectedRankSlot = -1;

	numDrivers = gGT->numPlyrCurrGame + gGT->numBotsNextGame;
	if (numDrivers >= 5)
	{
		numDrivers = 4;
	}

	tiedTopCount = 0;
	if (1 < numDrivers)
	{
		topScore = gGT->cup.points[data.cupPositionPerPlayer[0]];

		for (int i = 1; i < numDrivers; i++)
		{
			if (gGT->cup.points[data.cupPositionPerPlayer[i]] != topScore)
			{
				break;
			}

			tiedTopCount++;
		}
	}

	for (int rankSlot = 0; rankSlot < tiedTopCount + 1; rankSlot++)
	{
		for (int i = rankSlot; i < tiedTopCount + 1; i++)
		{
			struct Driver *driver;
			driver = gGT->drivers[data.cupPositionPerPlayer[i]];

			if (driver->driverRank < (s16)bestRank)
			{
				bestRank = (u16)driver->driverRank;
				selectedRankSlot = i;
			}
		}

		int previousRankValue = (u16)data.cupPositionPerPlayer[rankSlot];
		data.cupPositionPerPlayer[rankSlot] = (s16)selectedRankSlot;
		data.cupPositionPerPlayer[selectedRankSlot] = (s16)previousRankValue;

		bestRank = 99;
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80056220-0x800562fc.
void UI_CupStandings_UpdateCupRanks(void)
{
	struct GameTracker *gGT;
	int bestScore;
	int bestIndex;
	int assignedMask;
	int numDrivers;

	bestScore = 0;
	bestIndex = -1;
	gGT = sdata->gGT;
	assignedMask = 0;

	numDrivers = gGT->numPlyrCurrGame + gGT->numBotsNextGame;
	if (numDrivers == 0)
	{
		return;
	}

	for (int rankSlot = 0; rankSlot < numDrivers; rankSlot++)
	{
		for (int driverIndex = numDrivers - 1; driverIndex >= 0; driverIndex--)
		{
			if ((gGT->cup.points[driverIndex] >= (s16)bestScore) && (((assignedMask >> driverIndex) & 1) == 0))
			{
				bestScore = (u16)gGT->cup.points[driverIndex];

				if ((s16)bestIndex != -1)
				{
					assignedMask &= ~(1 << bestIndex);
				}

				bestIndex = driverIndex;
				assignedMask |= 1 << driverIndex;
			}
		}

		data.cupPositionPerPlayer[rankSlot] = (s16)bestIndex;
		bestScore = 0;
		bestIndex = -1;
	}
}

void UI_CupStandings_InputAndDraw(void)
{
	// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800562fc-0x800572d0.
	struct GameTracker *gGT = sdata->gGT;
	s16 iconColumnOffset;
	int i;
	int animationFrame;
	int pageExitY;
	int pageEnterY;
	u32 nextMenuReadyFlags;
	s16 iconTargetX;
	u32 wasTrackPointsPage;
	int iconStartX;
	int iconEndX;
	int numDrivers = gGT->numPlyrCurrGame + gGT->numBotsNextGame;
	s16 vsCupIconOffsetX;

	SVec2 drawPos;

	// Multiplayer Cup Game
	if (gGT->numPlyrCurrGame != 1)
	{
		if (((gGT->gameMode2 & CUP_ANY_KIND) != 0) && (RaceFlag_IsFullyOffScreen()))
		{
			RaceFlag_BeginTransition(1);
		}

		if (!RaceFlag_IsFullyOnScreen())
		{
			return;
		}

		// Assume FullyOnScreen
		RaceFlag_SetCanDraw(1);

		// disable loading screen,
		// set amount of confetti to zero
		gGT->confetti.numParticles_max = 0;
		gGT->confetti.vanishRate = 0;
		gGT->renderFlags &= RENDER_FLAG_CHECKERED_FLAG;
	}

	// Conditions to increment frame counter
	if ((sdata->framesSinceRaceEnded < UI_CUP_STANDINGS_PAGE_TRANSITION_FRAME) ||
	    ((sdata->framesSinceRaceEnded < UI_CUP_STANDINGS_OVERALL_HOLD_FRAME) && ((sdata->menuReadyToPass & UI_CUP_STANDINGS_PAGE_OVERALL_POINTS) != 0)))
	{
		sdata->framesSinceRaceEnded++;
	}

	if ((sdata->framesSinceRaceEnded < UI_CUP_STANDINGS_QUICK_SKIP_FRAME) &&

	    // If you press Cross or Circle
	    ((sdata->AnyPlayerTap & UI_CUP_STANDINGS_CONFIRM_BUTTONS) != 0))
	{
		sdata->framesSinceRaceEnded = UI_CUP_STANDINGS_QUICK_SKIP_FRAME;
		sdata->numIconsEOR = numDrivers;
		RECTMENU_ClearInput();
	}

	if ((sdata->menuReadyToPass & UI_CUP_STANDINGS_PAGE_TRACK_POINTS) == 0)
	{
		pageEnterY = -UI_CUP_STANDINGS_PANEL_MOVE_Y;
		pageExitY = UI_CUP_STANDINGS_TITLE_Y;
	}

	else
	{
		pageEnterY = UI_CUP_STANDINGS_TITLE_Y;
		pageExitY = -UI_CUP_STANDINGS_PANEL_MOVE_Y;
	}

	int titleStartX;
	int titleStartY;
	int titleEndX;
	int titleEndY;

	if (sdata->framesSinceRaceEnded <= UI_CUP_STANDINGS_PAGE_TRANSITION_FRAME)
	{
		titleStartX = UI_CUP_STANDINGS_TITLE_START_X;
		titleStartY = pageEnterY;
		titleEndX = UI_CUP_STANDINGS_TITLE_CENTER_X;
		titleEndY = UI_CUP_STANDINGS_TITLE_Y;
		animationFrame = sdata->framesSinceRaceEnded;
	}

	else
	{
		titleStartX = UI_CUP_STANDINGS_TITLE_CENTER_X;
		titleStartY = UI_CUP_STANDINGS_TITLE_Y;
		titleEndX = UI_CUP_STANDINGS_TITLE_END_X;
		titleEndY = pageExitY;
		animationFrame = sdata->framesSinceRaceEnded - UI_CUP_STANDINGS_PAGE_TRANSITION_FRAME;
	}

	UI_Lerp2D_Linear(drawPos.v, titleStartX, titleStartY, titleEndX, titleEndY, animationFrame, UI_CUP_STANDINGS_LERP_FRAMES);

	// "FINAL"
	int titleString = LNG_FINAL;
	int cupID = gGT->cup.cupID;

	if ((sdata->menuReadyToPass & UI_CUP_STANDINGS_PAGE_TRACK_POINTS) == 0)
	{
		// Level ID
		titleString = data.metaDataLEV[gGT->levelID].name_LNG;
	}

	else if (gGT->cup.trackIndex != UI_CUP_STANDINGS_TRACKS_PER_CUP - 1)
	{
		// If not in Arcade or VS cup
		if ((gGT->gameMode2 & CUP_ANY_KIND) == 0)
		{
			titleString = data.AdvCups[cupID].lngIndex_CupName;
		}

		// If Arcade or VS cup
		else
		{
			titleString = data.ArcadeCups[cupID].lngIndex_CupName;
		}
	}

	// title text
	DecalFont_DrawLine(sdata->lngStrings[titleString], drawPos.x, drawPos.y + UI_CUP_STANDINGS_TITLE_Y_OFFSET, FONT_BIG, JUSTIFY_CENTER | ORANGE);

	DecalFont_DrawLine(sdata->lngStrings[LNG_STANDINGS], drawPos.x, drawPos.y, FONT_BIG, JUSTIFY_CENTER | ORANGE);

	// 24 characters, in case of other
	// languages with longer text
	char text[24];

	// TRACK 1/4, 2/4, 3/4, 4/4
	sprintf(text, "%s %ld/4",

	        sdata->lngStrings[LNG_TRACK],

	        // Track Index (0, 1, 2, 3) + 1
	        CTR_PRINTF_PSX_LONG(gGT->cup.trackIndex + 1));

	DecalFont_DrawLine(text, drawPos.x, drawPos.y + UI_CUP_STANDINGS_TRACK_TEXT_Y_OFFSET, FONT_SMALL, JUSTIFY_CENTER | ORANGE);

	if ((sdata->framesSinceRaceEnded == (sdata->framesSinceRaceEnded / UI_CUP_STANDINGS_ICON_REVEAL_STEP_FRAMES) * UI_CUP_STANDINGS_ICON_REVEAL_STEP_FRAMES) &&
	    (sdata->numIconsEOR < numDrivers))
	{
		sdata->numIconsEOR++;
	}

	int rectX = gGT->pushBuffer_UI.rect.x;
	int rectW = gGT->pushBuffer_UI.rect.w;

	int *points = &data.cupPointsPerPosition[0];

	vsCupIconOffsetX = 0;
	for (i = 0; i < sdata->numIconsEOR; i++)
	{
		s16 iconTargetY;
		iconColumnOffset = (s16)i;
		// If you are in Purple Gem Cup
		if (gGT->cup.cupID == UI_CUP_STANDINGS_PURPLE_GEM_CUP_ID)
		{
			if (i < UI_CUP_STANDINGS_PURPLE_GEM_VISIBLE_DRIVERS)
			{
				iconTargetY = UI_CUP_STANDINGS_ICON_PURPLE_GEM_ROW_Y;
				iconTargetX = rectX;
				iconColumnOffset = (s16)((rectW - UI_CUP_STANDINGS_ICON_LEFT_MARGIN) / UI_CUP_STANDINGS_PURPLE_GEM_VISIBLE_DRIVERS) * iconColumnOffset +
				                   UI_CUP_STANDINGS_ICON_RIGHT_MARGIN;
				goto AddIconColumnOffset;
			}
			iconTargetX = 0;
			iconTargetY = 0;
		}

		// If this is not Purple Gem Cup
		else
		{
			// Basically, if you're in 2P Arcade
			if (numDrivers == UI_CUP_STANDINGS_TWO_PLAYER_ARCADE_DRIVER_COUNT)
			{
				iconTargetY = UI_CUP_STANDINGS_ICON_TOP_ROW_Y;

				if (i < 3)
				{
					iconTargetX = rectX;
					iconColumnOffset = UI_CUP_STANDINGS_ICON_SLOT_X * iconColumnOffset + UI_CUP_STANDINGS_ICON_LEFT_MARGIN;
				}

				else
				{
					iconColumnOffset = iconColumnOffset + -2;

				IconBottomRow:
					iconTargetY = UI_CUP_STANDINGS_ICON_BOTTOM_ROW_Y;

					iconTargetX = rectX;
					iconColumnOffset = UI_CUP_STANDINGS_ICON_SLOT_X * iconColumnOffset + UI_CUP_STANDINGS_ICON_SLOT_X;
				}
			}

			// If you are not in 2P Arcade
			else
			{
				// If VS cup of any kind
				if (gGT->numBotsNextGame == 0)
				{
					iconTargetY = UI_CUP_STANDINGS_ICON_VS_ROW_Y;
					iconTargetX =
					    rectX + (s16)((rectW - (numDrivers * UI_CUP_STANDINGS_ICON_SLOT_W) + UI_CUP_STANDINGS_ICON_VS_CENTER_BIAS) / 2) + vsCupIconOffsetX;
					goto HaveIconTargetPos;
				}

				iconTargetY = UI_CUP_STANDINGS_ICON_TOP_ROW_Y;
				if (3 < i)
				{
					iconColumnOffset = iconColumnOffset + -4;
					goto IconBottomRow;
				}

				iconTargetX = rectX;
				iconColumnOffset = UI_CUP_STANDINGS_ICON_SLOT_X * iconColumnOffset + UI_CUP_STANDINGS_ICON_LEFT_MARGIN;
			}

		AddIconColumnOffset:
			iconTargetX = iconTargetX + iconColumnOffset;
		}

	HaveIconTargetPos:

		if (sdata->framesSinceRaceEnded <= UI_CUP_STANDINGS_PAGE_TRANSITION_FRAME)
		{
			iconStartX = UI_CUP_STANDINGS_TITLE_END_X;
			iconEndX = (int)iconTargetX;
			animationFrame = sdata->framesSinceRaceEnded - (i * UI_CUP_STANDINGS_ICON_REVEAL_STEP_FRAMES);
		}

		else
		{
			iconStartX = (int)iconTargetX;
			iconEndX = UI_CUP_STANDINGS_TITLE_START_X;
			animationFrame = sdata->framesSinceRaceEnded + -UI_CUP_STANDINGS_PAGE_TRANSITION_FRAME;
		}

		// Interpolate fly-in variables over 0x14 frames
		UI_Lerp2D_Linear(drawPos.v, iconStartX, iconTargetY, iconEndX, iconTargetY, animationFrame, UI_CUP_STANDINGS_LERP_FRAMES);

		// %d
		sprintf(text, (char *)&sdata->s_int, i + 1);

		DecalFont_DrawLine(text, drawPos.x + UI_CUP_STANDINGS_PLACE_TEXT_X_OFFSET, drawPos.y + UI_CUP_STANDINGS_PLACE_TEXT_Y_OFFSET, FONT_SMALL, RED);

		struct Driver *d;

		// If this is the first screen of cup standings,
		// where you see just amount of points added
		if ((sdata->menuReadyToPass & UI_CUP_STANDINGS_PAGE_TRACK_POINTS) == 0)
		{
			// get driver in order of race rank (for one track)
			d = gGT->driversInRaceOrder[i];
		}

		// if this is not the first page,
		// so now you see total points of drivers
		else
		{
			// get driver in order of cup rank (for collection of tracks)
			d = gGT->drivers[data.cupPositionPerPlayer[i]];
		}

		// Draw character icon
		UI_DrawDriverIcon(gGT->ptrIcons[data.MetaDataCharacters[data.characterIDs[d->driverID]].iconID],

		                  drawPos.x, drawPos.y, &gGT->backBuffer->primMem,

		                  gGT->pushBuffer_UI.ptrOT, TRANS_50_DECAL, UI_CUP_STANDINGS_ICON_SCALE,
		                  MakeColor(UI_CUP_STANDINGS_ICON_NEUTRAL_CHANNEL, UI_CUP_STANDINGS_ICON_NEUTRAL_CHANNEL, UI_CUP_STANDINGS_ICON_NEUTRAL_CHANNEL).self);

		// If this is the first screen of cup standings,
		// where you see just amount of points added
		if ((sdata->menuReadyToPass & UI_CUP_STANDINGS_PAGE_TRACK_POINTS) == 0)
		{
			int awardedPoints = 0;

			if (i < UI_CUP_STANDINGS_MAX_VISIBLE_AWARD_DRIVERS)
			{
				if (gGT->numBotsNextGame == 0)
				{
					awardedPoints = gGT->numPlyrCurrGame - (i + 1);
				}
				else
				{
					awardedPoints = points[i];
				}
			}

			*(int *)&text[0] = '+' + (('0' + awardedPoints) << 8);
		}

		// if this is not the first page,
		// so now you see total points of drivers
		else
		{
			// The amount of points that each player has, in a cup
			sprintf(text, &sdata->s_longInt[0], gGT->cup.points[data.cupPositionPerPlayer[i]]);
		}

		vsCupIconOffsetX = vsCupIconOffsetX + UI_CUP_STANDINGS_ICON_SLOT_W;

		// draw string for number of points
		DecalFont_DrawLine(text, drawPos.x + UI_CUP_STANDINGS_POINTS_TEXT_X_OFFSET, drawPos.y + UI_CUP_STANDINGS_POINTS_TEXT_Y_OFFSET, FONT_BIG, PERIWINKLE);
	}

	// If this is the first screen of cup standings,
	// where you see just amount of points added
	if ((sdata->menuReadyToPass & UI_CUP_STANDINGS_PAGE_TRACK_POINTS) == 0)
	{
		// fly-in interpolation
		UI_Lerp2D_Linear(drawPos.v, UI_CUP_STANDINGS_PANEL_X, pageEnterY, UI_CUP_STANDINGS_PANEL_X, UI_CUP_STANDINGS_PANEL_Y, sdata->framesSinceRaceEnded,
		                 UI_CUP_STANDINGS_LERP_FRAMES);
	}

	// if it's not...
	else
	{
		if (sdata->framesSinceRaceEnded <= UI_CUP_STANDINGS_PAGE_TRANSITION_FRAME)
		{
			drawPos.x = UI_CUP_STANDINGS_PANEL_X;
			drawPos.y = UI_CUP_STANDINGS_PANEL_Y;
		}

		else
		{
			// fly-in interpolation
			UI_Lerp2D_Linear(drawPos.v, UI_CUP_STANDINGS_PANEL_X, UI_CUP_STANDINGS_PANEL_Y, UI_CUP_STANDINGS_PANEL_X, pageExitY,
			                 sdata->framesSinceRaceEnded + -UI_CUP_STANDINGS_PAGE_TRANSITION_FRAME, UI_CUP_STANDINGS_LERP_FRAMES);
		}
	}

	RECT r;
	r.x = drawPos.x;
	r.y = drawPos.y;
	r.w = UI_CUP_STANDINGS_PANEL_W;
	r.h = UI_CUP_STANDINGS_PANEL_H;

	// Draw 2D Menu rectangle background
	RECTMENU_DrawInnerRect(&r, 4, gGT->backBuffer->otMem.uiOT);

	// Timer
	if (sdata->framesSinceRaceEnded < UI_CUP_STANDINGS_DONE_FRAME)
	{
		if ((
		        // Timer
		        (UI_CUP_STANDINGS_PRESS_PROMPT_FRAME < sdata->framesSinceRaceEnded) &&

		        // If you are not in overall Cup standings
		        ((sdata->menuReadyToPass & UI_CUP_STANDINGS_PAGE_OVERALL_POINTS) == 0)) &&
		    (DecalFont_DrawLine(sdata->lngStrings[LNG_PRESS_TO_CONTINUE], UI_CUP_STANDINGS_PRESS_X, UI_CUP_STANDINGS_PRESS_Y, FONT_BIG,
		                        JUSTIFY_CENTER | ORANGE),

		     // If you press Cross or Circle
		     (sdata->AnyPlayerTap & UI_CUP_STANDINGS_CONFIRM_BUTTONS) != 0))
		{
			// Timer
			sdata->framesSinceRaceEnded = UI_CUP_STANDINGS_PAGE_TRANSITION_FRAME;

			// Proceed from Track standings to overall Cup standings,
			// where you see how many points each driver has overall
			sdata->menuReadyToPass = sdata->menuReadyToPass | UI_CUP_STANDINGS_PAGE_OVERALL_POINTS;

			// clear gamepad input (for menus)
			RECTMENU_ClearInput();
		}
	}

	else
	{
		sdata->numIconsEOR = 1;
		sdata->framesSinceRaceEnded = 0;
		nextMenuReadyFlags =
		    sdata->menuReadyToPass & ~(UI_CUP_STANDINGS_MENU_READY_END_OPTIONS | UI_CUP_STANDINGS_PAGE_TRACK_POINTS | UI_CUP_STANDINGS_PAGE_OVERALL_POINTS);
		wasTrackPointsPage = sdata->menuReadyToPass & UI_CUP_STANDINGS_PAGE_TRACK_POINTS;

		// Proceed from end-of-race menu to Track Standings,
		// where you see how many points are added just for this race
		sdata->menuReadyToPass = nextMenuReadyFlags | UI_CUP_STANDINGS_PAGE_TRACK_POINTS;

		// If the "4" flag was not enabled till just now,
		// If this is the first frame of Track Standings
		if (wasTrackPointsPage == 0)
		{
			if (numDrivers > UI_CUP_STANDINGS_MAX_VISIBLE_AWARD_DRIVERS)
			{
				numDrivers = UI_CUP_STANDINGS_MAX_VISIBLE_AWARD_DRIVERS;
			}

			for (i = 0; i < numDrivers; i++)
			{
				// the amount of points this player has in the cup
				gGT->cup.points[gGT->driversInRaceOrder[i]->driverID] +=

				    // Incremented by
				    // The amount of points that should be awarded to each position
				    (gGT->numBotsNextGame == 0) ? (gGT->numPlyrCurrGame - (i + 1)) : points[i];
			}

			// update cup rank of all drivers
			UI_CupStandings_UpdateCupRanks();
		}

		// If this is not the first frame of track standings
		else
		{
			sdata->menuReadyToPass = nextMenuReadyFlags;

			// enable drawing HUD
			gGT->hudFlags |= HUD_FLAG_RACE_HUD;

			// Disable types of HUD that are not needed for gameplay,
			// This includes Cup rankings, which is a flag in this byte
			gGT->hudFlags &= HUD_FLAG_CLEAR_CUP_STANDINGS_MASK;

			sdata->menuReadyToPass &= ~UI_CUP_STANDINGS_PAGE_TRACK_POINTS;

			// Increment Track Number by 1 (0-3 in the cup)
			gGT->cup.trackIndex++;

			int cupTrack = gGT->cup.trackIndex;

			// If this is not the last race in the cup
			if (cupTrack < UI_CUP_STANDINGS_TRACKS_PER_CUP)
			{
				int nextLevelID;

				// If not in Arcade or VS cup
				if ((gGT->gameMode2 & CUP_ANY_KIND) == 0)
				{
					nextLevelID = data.advCupTrackIDs[(UI_CUP_STANDINGS_TRACKS_PER_CUP * cupID) + cupTrack];
				}

				// If Arcade or VS cup
				else
				{
					nextLevelID = data.ArcadeCups[cupID].CupTrack[cupTrack].trackID;
				}

				MainRaceTrack_RequestLoad(nextLevelID);
			}

			// If this was the last race in the cup
			// If the cup is over
			else
			{
				if ((gGT->gameMode2 & CUP_ANY_KIND) != 0)
				{
					UI_CupStandings_FinalizeCupRanks();
				}

				gGT->cup.trackIndex = 0;

				// Array with the final ranking of each player
				int *ranks = &data.cupPositionPerPlayer[0];
				for (i = 0; i < UI_CUP_STANDINGS_DRIVER_SLOTS; i++)
				{
					struct Driver *d;
					d = gGT->drivers[ranks[i]];

					// If driver is not nullptr
					if (d != 0)
					{
						// Set final ranking of each player
						d->driverRank = (s16)i;
					}

					// set driver's cup score to zero
					gGT->cup.points[i] = 0;
				}

				Podium_InitModels(gGT);

				i = gGT->cup.cupID;

				// If this is an Adventure Cup
				if ((gGT->gameMode2 & CUP_ANY_KIND) == 0)
				{
					// Array with the ranking of each player
					gGT->levelID = i + ADVENTURE_CUP_SYNTHETIC_LEVEL_ID_BASE;

					// when loading is done,
					// remove flag for adventure cup
					sdata->Loading.OnBegin.RemBitsConfig0 |= ADVENTURE_CUP;

					// If player 1 won the cup
					if (data.cupPositionPerPlayer[0] == gGT->drivers[0]->driverID)
					{
						int bitIndex = ADV_REWARD_FIRST_GEM + i;
						u32 *rewardsSet = sdata->advProgress.rewards;

						if (!CHECK_ADV_BIT(rewardsSet, bitIndex))
						{
							UNLOCK_ADV_BIT(rewardsSet, bitIndex);

							// unlock Roo, Papu, Joe, Pinstripe, FCrash
							bitIndex = GAME_UNLOCK_BIT_BOSS_CHARACTER_FIRST + i;
							UNLOCK_ADV_BIT(sdata->gameProgress.unlocks, bitIndex);

							// Set podium reward model to Gem
							gGT->podiumRewardID = STATIC_GEM;
						}

						// reset counter for number of times you lost cup, to zero
						sdata->advProgress.timesLostCupRace[gGT->cup.trackIndex] = 0;
					}

					// If player 1 did not win the cup
					else
					{
						if (sdata->advProgress.timesLostCupRace[gGT->cup.trackIndex] < UI_CUP_STANDINGS_MAX_CUP_LOSSES)
						{
							sdata->advProgress.timesLostCupRace[gGT->cup.trackIndex]++;
						}
					}
				}

				// If this is not an Adventure Cup
				else
				{
					// === Naughty Dog Bug ===
					// If driver[1] is an AI in a 1P cup,
					// then you still get completion credit

					// If Player 1 or Player 2 won the cup
					if ((gGT->drivers[0]->driverRank == 0) || ((gGT->drivers[1]->driverRank == 0) &&

					                                           // If you're in Arcade Mode
					                                           ((gGT->gameMode1 & ARCADE_MODE) != 0)))
					{
						int difficulty = (gGT->arcadeDifficulty / UI_CUP_STANDINGS_ARCADE_DIFFICULTY_DIVISOR) - 1;
						if (difficulty > UI_CUP_STANDINGS_MAX_ARCADE_DIFFICULTY)
						{
							difficulty = UI_CUP_STANDINGS_MAX_ARCADE_DIFFICULTY;
						}

						u32 *rewardsSet = &sdata->gameProgress.unlockFlags;

						int baseIndex = sdata->UnlockBitIndex.CupCompletion_prev[difficulty];

						// if track was not unlocked "previously",
						// this writes when TakeCupProgress is saved
						int bitIndex = baseIndex + gGT->cup.cupID;
						if (!CHECK_ADV_BIT(rewardsSet, bitIndex))
						{
							// lets 233 know to prompt the Save Game box
							gGT->gameMode2 |= CUP_NEW_WIN;

							baseIndex = sdata->UnlockBitIndex.CupCompletion_curr[difficulty];

							bitIndex = baseIndex + gGT->cup.cupID;
							UNLOCK_ADV_BIT(rewardsSet, bitIndex);

							b32 boolUnlockMap = true;
							for (i = 0; i < UI_CUP_STANDINGS_TRACKS_PER_CUP; i++)
							{
								// if any of four cups on this difficulty was not won
								bitIndex = baseIndex + i;
								if (!CHECK_ADV_BIT(rewardsSet, bitIndex))
								{
									// you dont deserve to unlock a battle map
									boolUnlockMap = false;
									break;
								}
							}

							// If new Battle unlocked
							if (boolUnlockMap)
							{
								bitIndex = sdata->UnlockBitIndex.UnlockBattleMap[difficulty];
								UNLOCK_ADV_BIT(rewardsSet, bitIndex);

								// battle map is now unlocked (233 overlay)
								gGT->gameMode2 |= CUP_NEW_BATTLE;
							}
						}
					}

					// Set podium's reward model to "Empty"
					gGT->podiumRewardID = STATIC_BIG1;
				}

				// Level ID for Gemstone Valley (podiums)
				MainRaceTrack_RequestLoad(GEM_STONE_VALLEY);
			}
		}
	}
}
