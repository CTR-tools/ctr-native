#include <common.h>

enum UIRaceStartConstants
{
	UI_RACE_START_TITLE_EXIT_FRAME = 0x1f,
	UI_RACE_START_TITLE_EXIT_BASE_FRAME = 0x1e,
	UI_RACE_START_TITLE_CENTER_X = 0x100,
	UI_RACE_START_TITLE_TOP_Y_BIAS = 7,
	UI_RACE_START_CUP_TITLE_Y_OFFSET = -6,
	UI_RACE_START_CUP_TRACK_Y_OFFSET = 0xb,
	UI_RACE_START_LEVEL_TITLE_Y_OFFSET = -0x17,
	UI_RACE_START_DIVIDER_TOP_Y_OFFSET = 0x1c,
	UI_RACE_START_DIVIDER_BOTTOM_Y_OFFSET = -0x1e,
	UI_RACE_START_DIVIDER_HEIGHT = 2,
	UI_RACE_START_BAR_HEIGHT = 0x1e,
};

// One-player fly-in titles and their four viewport-aligned bars.
void UI_RaceStart_IntroText1P(void)
{
	struct GameTracker *gGT;
	struct PushBuffer *pushBuffer;
	s32 gameMode;
	s16 textID;
	s32 titleTransition = 0;
	s16 compareFrame;
	u16 transitionFrame;
	s32 topOffset;
	s32 bottomOffset;
	s32 levelTitleOffset;
	s32 dividerTopOffset;
	char trackText[24];
	RECT rect;
	Color color;

	gGT = GAME_TRACKER;
	gameMode = gGT->gameMode1;
	pushBuffer = &gGT->pushBuffer[0];
	if ((gameMode & RELIC_RACE) != 0)
	{
		textID = LNG_RELIC_RACE;
		goto title_selected;
	}
	if ((gameMode & CRYSTAL_CHALLENGE) != 0)
	{
		textID = LNG_BONUS_ROUND;
		goto title_selected;
	}
	// NOTE(aalhendi): Preserve the halfword bit pattern here; the signed title
	// index is interpreted when selecting its language string below.
	if ((gameMode & ADVENTURE_CUP) != 0)
	{
		textID = ((u16 *)data.advCupStringIndex)[gGT->cup.cupID];
		goto title_selected;
	}
	if ((gGT->gameMode2 & CUP_ANY_KIND) != 0)
	{
		textID = ((u16 *)data.arcadeVsCupStringIndex)[gGT->cup.cupID];
		goto title_selected;
	}
	textID = LNG_ARCADE;
	if ((gameMode & ARCADE_MODE) != 0)
		goto title_selected;
	textID = LNG_TIME_TRIAL;
	if ((gameMode & TIME_TRIAL) != 0)
		goto title_selected;
	if (gameMode < 0)
	{
		textID = ((u16 *)data.lng_challenge)[gGT->bossID];
		goto title_selected;
	}
	textID = LNG_TROPHY_RACE;
	if ((gGT->gameMode2 & TOKEN_RACE) != 0)
		textID = LNG_CTR_CHALLENGE_TITLE;
title_selected:
	compareFrame = GAME_TRACKER->cameraDC->transitionFrame;
	transitionFrame = (u16)GAME_TRACKER->cameraDC->transitionFrame;
	if (compareFrame < UI_RACE_START_TITLE_EXIT_FRAME)
		titleTransition = UI_RACE_START_TITLE_EXIT_BASE_FRAME - transitionFrame;

	if (RaceFlag_IsFullyOnScreen() != 0)
		return;

	if ((GAME_TRACKER->gameMode1 & ADVENTURE_CUP) != 0 || (GAME_TRACKER->gameMode2 & CUP_ANY_KIND) != 0)
	{
		topOffset = titleTransition - UI_RACE_START_TITLE_TOP_Y_BIAS;
		DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[textID], (s16)((u16)pushBuffer->rect.x + (((s32)((u32)(u16)pushBuffer->rect.w << 16)) >> 17)),
		                   (s16)((u16)pushBuffer->rect.y - topOffset + UI_RACE_START_CUP_TITLE_Y_OFFSET), FONT_BIG, JUSTIFY_CENTER | ORANGE);
		sprintf(trackText, "%s %ld/4", GAME_LANGUAGE_STRINGS[LNG_TRACK], CTR_PRINTF_PSX_LONG(GAME_TRACKER->cup.trackIndex + 1));
		DecalFont_DrawLine(trackText, UI_RACE_START_TITLE_CENTER_X, (s16)((u16)pushBuffer->rect.y - topOffset + UI_RACE_START_CUP_TRACK_Y_OFFSET), FONT_SMALL,
		                   JUSTIFY_CENTER | ORANGE);
	}
	else
	{
		s32 nonCupOffset = titleTransition - UI_RACE_START_TITLE_TOP_Y_BIAS;

		DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[textID], (s16)((u16)pushBuffer->rect.x + (((s32)((u32)(u16)pushBuffer->rect.w << 16)) >> 17)),
		                   (s16)((u16)pushBuffer->rect.y - nonCupOffset), FONT_BIG, JUSTIFY_CENTER | ORANGE);
	}

	// NOTE(aalhendi): Keep each offset as a value; folding it into a viewport
	// expression changes GCC 2.8.1's arithmetic grouping and draw schedule.
	levelTitleOffset = titleTransition + UI_RACE_START_LEVEL_TITLE_Y_OFFSET;
	DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[GAME_LEVEL_METADATA[GAME_TRACKER->levelID].name_LNG],
	                   (s16)((u16)pushBuffer->rect.x + (((s32)((u32)(u16)pushBuffer->rect.w << 16)) >> 17)),
	                   (s16)((u16)pushBuffer->rect.y + (u16)pushBuffer->rect.h + levelTitleOffset), FONT_BIG, JUSTIFY_CENTER | ORANGE);

	color = ColorCode_Load(&GAME_SDATA_NAME(battleSetup_Color_UI_1));
	dividerTopOffset = titleTransition - UI_RACE_START_DIVIDER_TOP_Y_OFFSET;
	rect.x = pushBuffer->rect.x;
	rect.y = (u16)pushBuffer->rect.y - dividerTopOffset;
	rect.w = pushBuffer->rect.w;
	rect.h = UI_RACE_START_DIVIDER_HEIGHT;
	CTR_Box_DrawSolidBox(&rect, &color, GAME_TRACKER->backBuffer->otMem.uiOT, &GAME_TRACKER->backBuffer->primMem);

	rect.x = pushBuffer->rect.x;
	bottomOffset = titleTransition + UI_RACE_START_DIVIDER_BOTTOM_Y_OFFSET;
	rect.y = (u16)pushBuffer->rect.y + (u16)pushBuffer->rect.h + bottomOffset;
	rect.w = pushBuffer->rect.w;
	rect.h = UI_RACE_START_DIVIDER_HEIGHT;
	CTR_Box_DrawSolidBox(&rect, &color, GAME_TRACKER->backBuffer->otMem.uiOT, &GAME_TRACKER->backBuffer->primMem);

	// Keep the packed color's command/alpha byte while clearing the bar RGB.
	color.r = 0;
	color.g = 0;
	color.b = 0;
	rect.x = pushBuffer->rect.x;
	rect.y = (u16)pushBuffer->rect.y - titleTransition;
	rect.w = pushBuffer->rect.w;
	rect.h = UI_RACE_START_BAR_HEIGHT;
	CTR_Box_DrawSolidBox(&rect, &color, GAME_TRACKER->backBuffer->otMem.uiOT, &GAME_TRACKER->backBuffer->primMem);

	rect.x = pushBuffer->rect.x;
	rect.y = (u16)pushBuffer->rect.y + (u16)pushBuffer->rect.h + bottomOffset;
	rect.w = pushBuffer->rect.w;
	rect.h = UI_RACE_START_BAR_HEIGHT;
	CTR_Box_DrawSolidBox(&rect, &color, GAME_TRACKER->backBuffer->otMem.uiOT, &GAME_TRACKER->backBuffer->primMem);
}
