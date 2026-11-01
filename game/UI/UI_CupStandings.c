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
	UI_CUP_STANDINGS_ICON_SLOT_X = 0x60,
	UI_CUP_STANDINGS_ICON_SLOT_W = 0x5a,
	UI_CUP_STANDINGS_ICON_ROW_ICON_W = 0x4e,
	UI_CUP_STANDINGS_ICON_ROW_GAP = 0xc,
	UI_CUP_STANDINGS_ICON_TOP_ROW_Y = 0x42,
	UI_CUP_STANDINGS_ICON_BOTTOM_ROW_Y = 0x79,
	UI_CUP_STANDINGS_ICON_PURPLE_GEM_ROW_Y = 0x60,
	UI_CUP_STANDINGS_ICON_VS_ROW_Y = 0x6c,
	UI_CUP_STANDINGS_ICON_LEFT_MARGIN = 0x20,
	UI_CUP_STANDINGS_ICON_RIGHT_MARGIN = 0x10,
	UI_CUP_STANDINGS_PLACE_TEXT_X_OFFSET = 0x20,
	UI_CUP_STANDINGS_PLACE_TEXT_Y_OFFSET = -1,
	UI_CUP_STANDINGS_POINTS_TEXT_X_OFFSET = 0x2d,
	UI_CUP_STANDINGS_POINTS_TEXT_Y_OFFSET = 6,
	UI_CUP_STANDINGS_ICON_SCALE = 0x1000,
	UI_CUP_STANDINGS_ICON_NEUTRAL_CHANNEL = 0x80,
	UI_CUP_STANDINGS_TRACKS_PER_CUP = 4,
	UI_CUP_STANDINGS_DRIVER_SLOTS = 8,
	UI_CUP_STANDINGS_MAX_CUP_LOSSES = 10,
};


#ifndef GAME_CUP_TRACK_FORMAT
#define GAME_CUP_TRACK_FORMAT "%s %ld/4"
#endif

#ifndef GAME_CUP_COMPLETION_CURRENT
#define GAME_CUP_COMPLETION_CURRENT  (GAME_SDATA_NAME(UnlockBitIndex).CupCompletion_curr)
#define GAME_CUP_COMPLETION_PREVIOUS (GAME_SDATA_NAME(UnlockBitIndex).CupCompletion_prev)
#define GAME_CUP_BATTLE_MAP_UNLOCKS  (GAME_SDATA_NAME(UnlockBitIndex).UnlockBattleMap)
#endif

#ifndef GAME_CUP_LOAD_CHARACTER_METADATA
#define GAME_CUP_LOAD_CHARACTER_METADATA(result) ((result) = GAME_CHARACTER_METADATA)
#define GAME_CUP_LOAD_CHARACTER_IDS(result)      ((result) = GAME_CHARACTER_IDS)
#endif

#ifndef GAME_CUP_SELECT_DRIVER_ICON
#define GAME_CUP_SELECT_DRIVER_ICON(tracker, iconID) ((tracker)->ptrIcons[(iconID)])
#endif

static inline void UI_CupStandings_CopyUnlockIndexes(s16 *destination, const s16 *source)
{
	memcpy(destination, source, 3 * sizeof(s16));
}

static inline s8 *UI_CupStandings_CupLossCounter(struct GameTracker *tracker)
{
	struct AdvProgress *progress = &GAME_ADV_PROGRESS;
	s32 lossIndex;
	// NOTE(aalhendi): The Cup bank belongs to the complete progress object, not the level-loss array.
	CTR_PSX_KEEP_VALUE(progress);
	lossIndex = tracker->cup.trackIndex + sizeof(GAME_ADV_PROGRESS.timesLostRacePerLev);
	CTR_PSX_KEEP_VALUE_RELAXED(lossIndex);
	return (s8 *)((size_t)lossIndex + (size_t)progress + offsetof(struct AdvProgress, timesLostRacePerLev));
}

static inline u32 *UI_CupStandings_ProgressWord(struct GameProgress *progress, s32 bit)
{
	size_t byteOffset = (bit >> 5) * sizeof(u32);
	return (u32 *)(byteOffset + (size_t)progress + offsetof(struct GameProgress, unlocks));
}

#define UI_CupStandings_PublishRanks(loopIndex)                            \
	{                                                                      \
		struct GameTracker *tracker = GAME_TRACKER;                        \
		s32 *ranks = &GAME_DATA_NAME(cupPositionPerPlayer)[0];             \
		for (; (loopIndex) < UI_CUP_STANDINGS_DRIVER_SLOTS; (loopIndex)++) \
		{                                                                  \
			struct Driver *driver = tracker->drivers[ranks[(loopIndex)]];  \
			if (driver != NULL)                                            \
				driver->driverRank = (s16)(loopIndex);                     \
			tracker->cup.points[(loopIndex)] = 0;                          \
		}                                                                  \
	}

void UI_CupStandings_InputAndDraw(void)
{
	struct GameTracker *gGT = GAME_TRACKER;
	struct PushBuffer *viewport;
	s32 iconColumnOffset;
	s32 i;
	s16 pageEnterY;
	s16 pageExitY;
	u32 nextMenuReadyFlags;
	s16 iconTargetX;
	u32 wasTrackPointsPage;
	s32 numDrivers = gGT->numPlyrCurrGame + gGT->numBotsNextGame;
	s32 lastDriverIndex;
	struct MetaDataCHAR *characterMetadata;
	s16 *characterIDs;
	RECT r;
	char pointsText[8];
	char *ordinalText;
	char trackText[24];

	s16 drawPos[2];

	viewport = &gGT->pushBuffer_UI;

	// NOTE(aalhendi): Keep RaceFlag's three transition-state observations separate.
	if ((gGT->gameMode2 & CUP_ANY_KIND) != 0 && gGT->numPlyrCurrGame != 1 && RaceFlag_IsFullyOffScreen() == 1)
	{
		RaceFlag_BeginTransition(1);
	}
	if (RaceFlag_IsFullyOnScreen() != 0 && GAME_TRACKER->numPlyrCurrGame != 1)
	{
		RaceFlag_SetCanDraw(1);
	}
	if (RaceFlag_IsFullyOnScreen() == 0 && GAME_TRACKER->numPlyrCurrGame != 1)
	{
		return;
	}
	if (RaceFlag_IsFullyOnScreen() != 0 || GAME_TRACKER->numPlyrCurrGame == 1)
	{
		if (GAME_TRACKER->numPlyrCurrGame != 1)
		{
			GAME_TRACKER->confetti.numParticles_max = 0;
			GAME_TRACKER->confetti.vanishRate = 0;
			GAME_TRACKER->renderFlags &= RENDER_FLAG_CHECKERED_FLAG;
		}
		{
			s32 currentFrame = sdata->framesSinceRaceEnded;
			if (currentFrame < UI_CUP_STANDINGS_PAGE_TRANSITION_FRAME ||
			    ((sdata->menuReadyToPass & UI_CUP_STANDINGS_PAGE_OVERALL_POINTS) != 0 && currentFrame < UI_CUP_STANDINGS_OVERALL_HOLD_FRAME))
			{
				sdata->framesSinceRaceEnded = currentFrame + 1;
			}
		}
	}

	if ((sdata->framesSinceRaceEnded < UI_CUP_STANDINGS_QUICK_SKIP_FRAME) &&

	    // If you press Cross or Circle
	    ((GAME_ANY_PLAYER_TAP & UI_CUP_STANDINGS_CONFIRM_BUTTONS) != 0))
	{
		sdata->framesSinceRaceEnded = UI_CUP_STANDINGS_QUICK_SKIP_FRAME;
		sdata->numIconsEOR = GAME_TRACKER->numPlyrCurrGame + GAME_TRACKER->numBotsNextGame;
		CTR_PSX_OBSERVE_MEMORY(sdata->numIconsEOR);
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


	{
		s32 currentFrame = sdata->framesSinceRaceEnded;
		if (currentFrame > UI_CUP_STANDINGS_PAGE_TRANSITION_FRAME)
		{
			UI_Lerp2D_Linear(drawPos, UI_CUP_STANDINGS_TITLE_CENTER_X, UI_CUP_STANDINGS_TITLE_Y, UI_CUP_STANDINGS_TITLE_END_X, pageExitY,
			                 currentFrame - UI_CUP_STANDINGS_PAGE_TRANSITION_FRAME, UI_CUP_STANDINGS_LERP_FRAMES);
		}
		else
		{
			UI_Lerp2D_Linear(drawPos, UI_CUP_STANDINGS_TITLE_START_X, pageEnterY, UI_CUP_STANDINGS_TITLE_CENTER_X, UI_CUP_STANDINGS_TITLE_Y, currentFrame,
			                 UI_CUP_STANDINGS_LERP_FRAMES);
		}
	}

	if ((sdata->menuReadyToPass & UI_CUP_STANDINGS_PAGE_TRACK_POINTS) == 0)
	{
		DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[GAME_LEVEL_METADATA[GAME_TRACKER->levelID].name_LNG], drawPos[0], drawPos[1] + UI_CUP_STANDINGS_TITLE_Y_OFFSET,
		                   FONT_BIG, JUSTIFY_CENTER | ORANGE);
	}
	else if (GAME_TRACKER->cup.trackIndex == UI_CUP_STANDINGS_TRACKS_PER_CUP - 1)
	{
		DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_FINAL], drawPos[0], drawPos[1] + UI_CUP_STANDINGS_TITLE_Y_OFFSET, FONT_BIG, JUSTIFY_CENTER | ORANGE);
	}
	else if ((GAME_TRACKER->gameMode2 & CUP_ANY_KIND) != 0)
	{
		DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[GAME_DATA_NAME(ArcadeCups)[GAME_TRACKER->cup.cupID].lngIndex_CupName], drawPos[0],
		                   drawPos[1] + UI_CUP_STANDINGS_TITLE_Y_OFFSET, FONT_BIG, JUSTIFY_CENTER | ORANGE);
	}
	else
	{
		DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[GAME_DATA_NAME(AdvCups)[GAME_TRACKER->cup.cupID].lngIndex_CupName], drawPos[0],
		                   drawPos[1] + UI_CUP_STANDINGS_TITLE_Y_OFFSET, FONT_BIG, JUSTIFY_CENTER | ORANGE);
	}

	DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_STANDINGS], drawPos[0], drawPos[1], FONT_BIG, JUSTIFY_CENTER | ORANGE);

	// 24 characters, in case of other
	// languages with longer text

	// TRACK 1/4, 2/4, 3/4, 4/4
	sprintf(trackText, GAME_CUP_TRACK_FORMAT,

	        GAME_LANGUAGE_STRINGS[LNG_TRACK],

	        // Track Index (0, 1, 2, 3) + 1
	        CTR_PRINTF_PSX_LONG(GAME_TRACKER->cup.trackIndex + 1));

	DecalFont_DrawLine(trackText, drawPos[0], drawPos[1] + UI_CUP_STANDINGS_TRACK_TEXT_Y_OFFSET, FONT_SMALL, JUSTIFY_CENTER | ORANGE);

	{
		s32 currentFrame = sdata->framesSinceRaceEnded;
		if (currentFrame == (currentFrame / UI_CUP_STANDINGS_ICON_REVEAL_STEP_FRAMES) * UI_CUP_STANDINGS_ICON_REVEAL_STEP_FRAMES)
		{
			s32 iconCount = sdata->numIconsEOR;
			u16 previousIconCount = sdata->numIconsEOR;
			if (iconCount < numDrivers)
				sdata->numIconsEOR = previousIconCount + 1;
		}
	}


	CTR_PSX_OBSERVE_VALUE(numDrivers);
	{
		s32 initialIconCount = sdata->numIconsEOR;
		i = 0;
		if (initialIconCount > 0)
		{
			lastDriverIndex = numDrivers - 1;
			ordinalText = pointsText;
			GAME_CUP_LOAD_CHARACTER_METADATA(characterMetadata);
			GAME_CUP_LOAD_CHARACTER_IDS(characterIDs);
			do
			{
				s16 iconTargetY;
				struct Driver *d;
				s32 iconRowWidth;
				s32 iconColumn;
				register s16 iconID CTR_PSX_REGISTER("$9");
				if (GAME_TRACKER->cup.cupID == UI_CUP_STANDINGS_PURPLE_GEM_CUP_ID)
				{
					if (i < UI_CUP_STANDINGS_PURPLE_GEM_VISIBLE_DRIVERS)
					{
						iconTargetY = UI_CUP_STANDINGS_ICON_PURPLE_GEM_ROW_Y;
						iconColumnOffset = ((viewport->rect.w - UI_CUP_STANDINGS_ICON_LEFT_MARGIN) / UI_CUP_STANDINGS_PURPLE_GEM_VISIBLE_DRIVERS) * i +
						                   UI_CUP_STANDINGS_ICON_RIGHT_MARGIN;
						iconTargetX = (u16)viewport->rect.x;
						goto AddIconColumnOffset;
					}
					iconTargetX = 0;
					iconTargetY = 0;
					goto HaveIconTargetPos;
				}
				if (GAME_TRACKER->numPlyrCurrGame + GAME_TRACKER->numBotsNextGame == UI_CUP_STANDINGS_TWO_PLAYER_ARCADE_DRIVER_COUNT)
				{
					iconTargetY = UI_CUP_STANDINGS_ICON_TOP_ROW_Y;
					if (i < 3)
					{
						s32 rowStep = (viewport->rect.w - 128) >> 2;
						s32 rowProduct = rowStep * i;
						iconTargetX = (u16)viewport->rect.x;
						CTR_PSX_OBSERVE_LO_GR(rowProduct);
						iconColumnOffset = rowProduct + UI_CUP_STANDINGS_ICON_LEFT_MARGIN;
						goto AddIconColumnOffset;
					}
					iconRowWidth = viewport->rect.w;
					iconColumn = i - 2;
					goto IconBottomRow;
				}
				if (GAME_TRACKER->numBotsNextGame == 0)
				{
					iconTargetY = UI_CUP_STANDINGS_ICON_VS_ROW_Y;
					iconTargetX = (u16)viewport->rect.x +
					              (viewport->rect.w - (numDrivers * UI_CUP_STANDINGS_ICON_ROW_ICON_W + lastDriverIndex * UI_CUP_STANDINGS_ICON_ROW_GAP)) / 2 +
					              i * UI_CUP_STANDINGS_ICON_SLOT_W;
					goto HaveIconTargetPos;
				}
				iconTargetY = UI_CUP_STANDINGS_ICON_TOP_ROW_Y;
				if (i < 4)
				{
					iconColumnOffset = ((viewport->rect.w - 128) >> 2) * i + UI_CUP_STANDINGS_ICON_LEFT_MARGIN;
					iconTargetX = (u16)viewport->rect.x;
					goto AddIconColumnOffset;
				}
				iconRowWidth = viewport->rect.w;
				iconColumn = i - 4;
			IconBottomRow:
				iconColumnOffset = ((iconRowWidth - 128) >> 2) * iconColumn + UI_CUP_STANDINGS_ICON_SLOT_X;
				iconTargetY = UI_CUP_STANDINGS_ICON_BOTTOM_ROW_Y;
				iconTargetX = (u16)viewport->rect.x;
			AddIconColumnOffset:
				iconTargetX = iconTargetX + iconColumnOffset;

			HaveIconTargetPos:

			{
				s32 currentFrame = sdata->framesSinceRaceEnded;
				if (currentFrame > UI_CUP_STANDINGS_PAGE_TRANSITION_FRAME)
				{
					UI_Lerp2D_Linear(drawPos, iconTargetX, iconTargetY, UI_CUP_STANDINGS_TITLE_START_X, iconTargetY,
					                 currentFrame - UI_CUP_STANDINGS_PAGE_TRANSITION_FRAME, UI_CUP_STANDINGS_LERP_FRAMES);
				}
				else
				{
					UI_Lerp2D_Linear(drawPos, UI_CUP_STANDINGS_TITLE_END_X, iconTargetY, iconTargetX, iconTargetY,
					                 currentFrame - i * UI_CUP_STANDINGS_ICON_REVEAL_STEP_FRAMES, UI_CUP_STANDINGS_LERP_FRAMES);
				}
			}

				// %d
				sprintf(ordinalText, GAME_SDATA_NAME(s_int), i + 1);

				DecalFont_DrawLine(ordinalText, drawPos[0] + UI_CUP_STANDINGS_PLACE_TEXT_X_OFFSET, drawPos[1] + UI_CUP_STANDINGS_PLACE_TEXT_Y_OFFSET,
				                   FONT_SMALL, RED);

				// If this is the first screen of cup standings,
				// where you see just amount of points added
				if ((sdata->menuReadyToPass & UI_CUP_STANDINGS_PAGE_TRACK_POINTS) == 0)
				{
					// get driver in order of race rank (for one track)
					d = GAME_TRACKER->driversInRaceOrder[i];
				}

				// if this is not the first page,
				// so now you see total points of drivers
				else
				{
					// get driver in order of cup rank (for collection of tracks)
					s32 *cupRanks = GAME_DATA_NAME(cupPositionPerPlayer);
					CTR_PSX_KEEP_VALUE(cupRanks);
					d = GAME_TRACKER->drivers[cupRanks[i]];
				}

				// Draw character icon
				iconID = characterMetadata[characterIDs[d->driverID]].iconID;
				CTR_PSX_KEEP_VALUE(iconID);
				UI_DrawDriverIcon(
				    GAME_CUP_SELECT_DRIVER_ICON(GAME_TRACKER, iconID),

				    drawPos[0], drawPos[1], &GAME_TRACKER->backBuffer->primMem,

				    GAME_TRACKER->pushBuffer_UI.ptrOT, TRANS_50_DECAL, UI_CUP_STANDINGS_ICON_SCALE,
				    MakeColorPacked(UI_CUP_STANDINGS_ICON_NEUTRAL_CHANNEL, UI_CUP_STANDINGS_ICON_NEUTRAL_CHANNEL, UI_CUP_STANDINGS_ICON_NEUTRAL_CHANNEL));

				// If this is the first screen of cup standings,
				// where you see just amount of points added
				{
					char *pointTextOut = pointsText;
					if ((sdata->menuReadyToPass & UI_CUP_STANDINGS_PAGE_TRACK_POINTS) == 0)
					{
						s32 awardedPoints;
						CTR_PSX_KEEP_VALUE_RELAXED(pointTextOut);

						if (GAME_TRACKER->numBotsNextGame == 0)
						{
							s32 playerCount = GAME_TRACKER->numPlyrCurrGame;
							s32 nextPlace;
							CTR_PSX_OBSERVE_VALUE(playerCount);
							nextPlace = i + 1;
							CTR_PSX_KEEP_VALUE_RELAXED(nextPlace);
							awardedPoints = playerCount - nextPlace;
						}
						else
						{
							awardedPoints = GAME_DATA_NAME(cupPointsPerPosition)[i];
							pointTextOut = pointsText;
						}

						sprintf(pointTextOut, GAME_SDATA_NAME(s_additionInt), awardedPoints);
					}

					// if this is not the first page,
					// so now you see total points of drivers
					else
					{
						// The amount of points that each player has, in a cup
						s32 *cupRanks = GAME_DATA_NAME(cupPositionPerPlayer);
						CTR_PSX_KEEP_VALUE(cupRanks);
						sprintf(pointTextOut, GAME_SDATA_NAME(s_longInt), CTR_PRINTF_PSX_LONG(GAME_TRACKER->cup.points[cupRanks[i]]));
					}
				}


				// draw string for number of points
				DecalFont_DrawLine(pointsText, drawPos[0] + UI_CUP_STANDINGS_POINTS_TEXT_X_OFFSET, drawPos[1] + UI_CUP_STANDINGS_POINTS_TEXT_Y_OFFSET, FONT_BIG,
				                   PERIWINKLE);
				i++;
			} while (i < sdata->numIconsEOR);
		}
	}

	// If this is the first screen of cup standings,
	// where you see just amount of points added
	if ((sdata->menuReadyToPass & UI_CUP_STANDINGS_PAGE_TRACK_POINTS) == 0)
	{
		s32 currentFrame = sdata->framesSinceRaceEnded;
		// fly-in interpolation
		UI_Lerp2D_Linear(drawPos, UI_CUP_STANDINGS_PANEL_X, pageEnterY, UI_CUP_STANDINGS_PANEL_X, UI_CUP_STANDINGS_PANEL_Y, currentFrame,
		                 UI_CUP_STANDINGS_LERP_FRAMES);
	}

	// if it's not...
	else
	{
		s32 currentFrame = sdata->framesSinceRaceEnded;
		if (currentFrame > UI_CUP_STANDINGS_PAGE_TRANSITION_FRAME)
		{
			UI_Lerp2D_Linear(drawPos, UI_CUP_STANDINGS_PANEL_X, UI_CUP_STANDINGS_PANEL_Y, UI_CUP_STANDINGS_PANEL_X, pageExitY,
			                 currentFrame - UI_CUP_STANDINGS_PAGE_TRANSITION_FRAME, UI_CUP_STANDINGS_LERP_FRAMES);
		}
		else
		{
			drawPos[0] = UI_CUP_STANDINGS_PANEL_X;
			drawPos[1] = UI_CUP_STANDINGS_PANEL_Y;
		}
	}

	r.x = drawPos[0];
	r.y = drawPos[1];
	r.w = UI_CUP_STANDINGS_PANEL_W;
	r.h = UI_CUP_STANDINGS_PANEL_H;

	// Draw 2D Menu rectangle background
	RECTMENU_DrawInnerRect(&r, 4, GAME_TRACKER->backBuffer->otMem.uiOT);

	// Timer
	{
		s32 currentFrame = sdata->framesSinceRaceEnded;
		if (currentFrame >= UI_CUP_STANDINGS_DONE_FRAME)
		{
			sdata->numIconsEOR = 1;
			sdata->framesSinceRaceEnded = 0;
			wasTrackPointsPage = sdata->menuReadyToPass;
			{
				u32 clearedFlags = wasTrackPointsPage;
				clearedFlags &= ~UI_CUP_STANDINGS_MENU_READY_END_OPTIONS;
				clearedFlags &= ~UI_CUP_STANDINGS_PAGE_OVERALL_POINTS;
				nextMenuReadyFlags = clearedFlags;
			}
			wasTrackPointsPage &= UI_CUP_STANDINGS_PAGE_TRACK_POINTS;

			// Proceed from end-of-race menu to Track Standings,
			// where you see how many points are added just for this race
			sdata->menuReadyToPass = nextMenuReadyFlags;
			CTR_PSX_KEEP_VALUE(nextMenuReadyFlags);

			// If the "4" flag was not enabled till just now,
			// If this is the first frame of Track Standings
			if (wasTrackPointsPage == 0)
			{
				struct GameTracker *awardTracker;
				nextMenuReadyFlags |= UI_CUP_STANDINGS_PAGE_TRACK_POINTS;
				sdata->menuReadyToPass = nextMenuReadyFlags;
				awardTracker = GAME_TRACKER;
				if (awardTracker->numBotsNextGame == 0)
				{
					s32 initialPlayerCount = awardTracker->numPlyrCurrGame;
					i = 0;
					if (initialPlayerCount != 0)
					{
						do
						{
							struct Driver *awardDriver = awardTracker->driversInRaceOrder[i];
							s32 nextRank = i + 1;
							s32 awardDriverID = awardDriver->driverID;
							s32 playerCount = awardTracker->numPlyrCurrGame;
							awardTracker->cup.points[awardDriverID] += playerCount - nextRank;
							i = nextRank;
						} while (i < awardTracker->numPlyrCurrGame);
					}
				}
				else
				{
					struct GameTracker *botAwardInput = awardTracker;
					s32 *botPoints;
					struct GameTracker *botAwardTracker;
					// NOTE(aalhendi): Retain the tracker input before establishing the scoring-table cursor.
					CTR_PSX_KEEP_VALUE(botAwardInput);
					i = 0;
					botPoints = GAME_DATA_NAME(cupPointsPerPosition);
					botAwardTracker = botAwardInput;
					for (; i < UI_CUP_STANDINGS_DRIVER_SLOTS; i++)
					{
#ifdef CTR_NATIVE
						// NOTE(aalhendi): Two-player Arcade can leave the last two slots empty.
						if (botAwardTracker->driversInRaceOrder[i] == NULL)
							continue;
#endif
						botAwardTracker->cup.points[botAwardTracker->driversInRaceOrder[i]->driverID] += botPoints[i];
					}
				}

				// update cup rank of all drivers
				UI_CupStandings_UpdateCupRanks();
			}

			// If this is not the first frame of track standings
			else
			{
				s32 cupTrack;
				s32 previousCupTrack;
				struct GameTracker *nextTrackTracker;

				// enable drawing HUD
				GAME_TRACKER->hudFlags |= HUD_FLAG_RACE_HUD;

				// Disable types of HUD that are not needed for gameplay,
				// This includes Cup rankings, which is a flag in this byte
				GAME_TRACKER->hudFlags &= HUD_FLAG_CLEAR_CUP_STANDINGS_MASK;

				nextTrackTracker = GAME_TRACKER;
				previousCupTrack = nextTrackTracker->cup.trackIndex;
				sdata->menuReadyToPass &= ~UI_CUP_STANDINGS_PAGE_TRACK_POINTS;

				// Increment Track Number by 1 (0-3 in the cup)
				cupTrack = previousCupTrack + 1;
				nextTrackTracker->cup.trackIndex = cupTrack;

				// If this is not the last race in the cup
				if (cupTrack < UI_CUP_STANDINGS_TRACKS_PER_CUP)
				{
					if ((GAME_TRACKER->gameMode2 & CUP_ANY_KIND) != 0)
					{
						MainRaceTrack_RequestLoad(GAME_DATA_NAME(ArcadeCups)[GAME_TRACKER->cup.cupID].CupTrack[cupTrack].trackID);
					}
					else
					{
						const s32 *trackIDs = GAME_DATA_NAME(advCupTrackIDs);
						size_t trackOffset = cupTrack * sizeof(trackIDs[0]);
						size_t cupOffset = GAME_TRACKER->cup.cupID * (UI_CUP_STANDINGS_TRACKS_PER_CUP * sizeof(trackIDs[0]));
						MainRaceTrack_RequestLoad((s16) * (const s32 *)((const u8 *)trackIDs + (trackOffset + cupOffset)));
					}
				}

				// If this was the last race in the cup
				// If the cup is over
				else
				{
					if ((GAME_TRACKER->gameMode2 & CUP_ANY_KIND) != 0)
					{
						GAME_TRACKER->cup.trackIndex = 0;
						UI_CupStandings_FinalizeCupRanks();
						i = 0;
						UI_CupStandings_PublishRanks(i);
						Podium_InitModels(GAME_TRACKER);
						// === Naughty Dog Bug ===
						// If driver[1] is an AI in a 1P cup,
						// then you still get completion credit

						// If Player 1 or Player 2 won the cup
						if (((GAME_TRACKER->drivers[0]->driverRank == 0) || (GAME_TRACKER->drivers[1]->driverRank == 0)) &&

						    // If you're in Arcade Mode
						    ((GAME_TRACKER->gameMode1 & ARCADE_MODE) != 0))
						{
							struct GameProgress *gameProgress;
							const s16 *completionBase;
							const s16 *currentCompletion;
							s32 currentCupBit;
							s32 previousCupBit;
							s32 scanBase;
							s32 difficulty;
							b32 boolUnlockMap = true;
							s16 completionCurrent[3];
							s16 completionPrevious[3];
							s16 battleMap[3];
							if (GAME_TRACKER->arcadeDifficulty != 80)
							{
								if (GAME_TRACKER->arcadeDifficulty == 160)
									difficulty = 1;
								else
									difficulty = 2;
							}
							else
								difficulty = 0;
							UI_CupStandings_CopyUnlockIndexes(completionCurrent, GAME_CUP_COMPLETION_CURRENT);
							UI_CupStandings_CopyUnlockIndexes(completionPrevious, GAME_CUP_COMPLETION_PREVIOUS);
							UI_CupStandings_CopyUnlockIndexes(battleMap, GAME_CUP_BATTLE_MAP_UNLOCKS);
							CTR_PSX_MEMORY_BARRIER();

							completionBase = completionCurrent;
							gameProgress = &GAME_SAVE.progress;
							currentCompletion = &completionBase[difficulty];

							currentCupBit = *currentCompletion + GAME_TRACKER->cup.cupID;
							UNLOCK_ADV_BIT(gameProgress->unlocks, currentCupBit);


							// if track was not unlocked "previously",
							// this writes when TakeCupProgress is saved
							previousCupBit = completionPrevious[difficulty] + GAME_TRACKER->cup.cupID;
							if (!CHECK_ADV_BIT(gameProgress->unlocks, previousCupBit))
							{
								struct GameProgress *scanProgress;
								u32 previousMode;
								i = 0;
								previousMode = GAME_TRACKER->gameMode2;
								scanProgress = gameProgress;
								// lets 233 know to prompt the Save Game box
								GAME_TRACKER->gameMode2 = previousMode | CUP_NEW_WIN;

								scanBase = *currentCompletion;

								for (; i < UI_CUP_STANDINGS_TRACKS_PER_CUP; i++)
								{
									s32 scanBit = scanBase + i;
									// if any of four cups on this difficulty was not won
									if (!((*UI_CupStandings_ProgressWord(scanProgress, scanBit) >> (scanBit & 0x1f)) & 1))
									{
										// you dont deserve to unlock a battle map
										boolUnlockMap = false;
										break;
									}
								}

								// If new Battle unlocked
								if ((b8)boolUnlockMap)
								{
									struct GameProgress *battleProgress = &GAME_SAVE.progress;
									s16 battleMapBit = battleMap[difficulty];
									UNLOCK_ADV_BIT(battleProgress->unlocks, battleMapBit);

									// battle map is now unlocked (233 overlay)
									GAME_TRACKER->gameMode2 |= CUP_NEW_BATTLE;
								}
							}
						}

						// Set podium's reward model to "Empty"
						GAME_TRACKER->podiumRewardID = STATIC_BIG1;
						MainRaceTrack_RequestLoad(GEM_STONE_VALLEY);
					}
					else
					{
						i = 0;
						GAME_TRACKER->levelID = GAME_TRACKER->cup.cupID + ADVENTURE_CUP_SYNTHETIC_LEVEL_ID_BASE;
						GAME_TRACKER->cup.trackIndex = 0;
						GAME_REMOVE_CONFIG_0 |= ADVENTURE_CUP;
						UI_CupStandings_PublishRanks(i);
						Podium_InitModels(GAME_TRACKER);

						// If player 1 won the cup
						if (GAME_DATA_NAME(cupPositionPerPlayer)[0] == GAME_TRACKER->drivers[0]->driverID)
						{
							s32 currentGemBit = ADV_REWARD_FIRST_GEM + GAME_TRACKER->cup.cupID;
							u32 *rewardsSet = GAME_ADV_PROGRESS.rewards;

							if (!CHECK_ADV_BIT(rewardsSet, currentGemBit))
							{
								s32 bossBit;
								s32 awardedGemBit;
								struct GameProgress *characterProgress = &GAME_SAVE.progress;
								// Set podium reward model to Gem
								GAME_TRACKER->podiumRewardID = STATIC_GEM;
								// unlock Roo, Papu, Joe, Pinstripe, FCrash
								bossBit = GAME_UNLOCK_BIT_BOSS_CHARACTER_FIRST + GAME_TRACKER->cup.cupID;
								UNLOCK_ADV_BIT(characterProgress->unlocks, bossBit);
								awardedGemBit = ADV_REWARD_FIRST_GEM + GAME_TRACKER->cup.cupID;
								UNLOCK_ADV_BIT(rewardsSet, awardedGemBit);
							}

							// reset counter for number of times you lost cup, to zero
							GAME_ADV_PROGRESS.timesLostCupRace[GAME_TRACKER->cup.trackIndex] = 0;
						}

						// If player 1 did not win the cup
						else
						{
							s8 *lossCounter = UI_CupStandings_CupLossCounter(GAME_TRACKER);
							if (*lossCounter < UI_CUP_STANDINGS_MAX_CUP_LOSSES)
							{
								(*lossCounter)++;
							}
						}
						MainRaceTrack_RequestLoad(GEM_STONE_VALLEY);
					}
				}
			}
		}
		else
		{
			if ((
			        // Timer
			        (UI_CUP_STANDINGS_PRESS_PROMPT_FRAME < currentFrame) &&

			        // If you are not in overall Cup standings
			        ((sdata->menuReadyToPass & UI_CUP_STANDINGS_PAGE_OVERALL_POINTS) == 0)) &&
			    (DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_PRESS_TO_CONTINUE], UI_CUP_STANDINGS_PRESS_X, UI_CUP_STANDINGS_PRESS_Y, FONT_BIG,
			                        JUSTIFY_CENTER | ORANGE),

			     // If you press Cross or Circle
			     (GAME_ANY_PLAYER_TAP & UI_CUP_STANDINGS_CONFIRM_BUTTONS) != 0))
			{
				// Timer
				sdata->framesSinceRaceEnded = UI_CUP_STANDINGS_PAGE_TRANSITION_FRAME;

				// Proceed from Track standings to overall Cup standings,
				// where you see how many points each driver has overall
				sdata->menuReadyToPass = sdata->menuReadyToPass | UI_CUP_STANDINGS_PAGE_OVERALL_POINTS;

				// clear gamepad input (for menus)
				CTR_PSX_OBSERVE_MEMORY(sdata->menuReadyToPass);
				RECTMENU_ClearInput();
			}
		}
	}
}
