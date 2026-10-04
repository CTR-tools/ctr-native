#include <common.h>

enum
{
	UI_POS_SUFFIX_BIG_NUM_Z_BASE = 0x100,
	UI_LAP_COUNT_SPLIT_PLAYER_COUNT = 3,
	UI_LAP_COUNT_TEXT_Y_OFFSET = 8,
	UI_LAP_COUNT_MESSAGE_BUFFER_SIZE = 24,
	UI_BATTLE_SCORE_STRING_BUFFER_SIZE = 32,
	UI_BATTLE_SCORE_LIFE_ICON = 0x84,
	UI_BATTLE_SCORE_POINTS_ICON = 0x85,
	UI_BATTLE_SCORE_TEXT_X_OFFSET = 37,
	UI_BATTLE_SCORE_TEXT_Y_OFFSET = 4,
	UI_BATTLE_SCORE_ICON_SCALE = 0x1000,
};

void UI_DrawPosSuffix(s16 posX, s16 posY, struct Driver *d, s16 flags)
{
	// NOTE(aalhendi): Keep the tracker base distinct from the indexed team address.
	register struct GameTracker *gGT CTR_PSX_REGISTER("$8") = GAME_TRACKER;
	s32 suffixIndex;
	char *str;

	if ((gGT->gameMode1 & BATTLE_MODE) != 0)
	{
		// Get the battle team's finishing position.
		suffixIndex = gGT->battleSetup.finishedRankOfEachTeam[d->BattleHUD.teamID];
		CTR_PSX_OBSERVE_VALUE(gGT);
		suffixIndex = GAME_POSITION_SUFFIX_INDICES[suffixIndex];
	}
	else
	{
		suffixIndex = GAME_POSITION_SUFFIX_INDICES[d->driverRank];
	}

	// NOTE(aalhendi): Finish the string lookup before preparing the narrow font arguments.
	str = GAME_LANGUAGE_STRINGS[suffixIndex];
	CTR_PSX_OBSERVE_VALUE(str);
	DecalFont_DrawLine(str, posX, posY, FONT_BIG, flags);

	// Instance depth selects the driver's position number, even in a team battle.
	if (d->instBigNum != 0)
	{
		d->instBigNum->matrix.t[2] = d->driverRank + UI_POS_SUFFIX_BIG_NUM_Z_BASE;
	}
}

void UI_DrawLapCount(s16 posX, s32 posY, s32 unusedScale, struct Driver *d)
{
	s16 currLap;
	char message[UI_LAP_COUNT_MESSAGE_BUFFER_SIZE];
	struct GameTracker *gGT;

	(void)unusedScale;
	gGT = GAME_TRACKER;
	currLap = d->lapIndex + 1;
	if ((s32)d->lapIndex + 1 > gGT->numLaps)
	{
		currLap = gGT->numLaps;
	}

	// 1P or 2P
	if ((u32)gGT->numPlyrCurrGame < UI_LAP_COUNT_SPLIT_PLAYER_COUNT)
	{
		s32 flags = JUSTIFY_RIGHT | PERIWINKLE;
		DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_LAP], posX, posY, FONT_SMALL, flags);
		// Reload the tracker after drawing the label before formatting the lap total.
		sprintf(message, GAME_SDATA_NAME(s_intDividing), currLap, GAME_TRACKER->numLaps);
		DecalFont_DrawLine(message, posX, CTR_MipsAddLo(posY, UI_LAP_COUNT_TEXT_Y_OFFSET), FONT_BIG, flags);
	}
	else
	{
		// Split-screen uses the existing shared single-digit buffer.
		char *str = GAME_SDATA_NAME(s_printDividing);
		str[0] = currLap + '0';
		str[2] = (u8)gGT->numLaps + '0';
		DecalFont_DrawLine(str, posX, CTR_MipsAddLo(posY, UI_LAP_COUNT_TEXT_Y_OFFSET), FONT_SMALL, PERIWINKLE);
	}
}

// Draw the player's remaining lives or team points in battle.
void UI_DrawBattleScores(s32 posX, s32 posY, struct Driver *d)
{
	char string[UI_BATTLE_SCORE_STRING_BUFFER_SIZE];
	// NOTE(aalhendi): The packed halfword palette uses offset-first address addition.
	register s16 *colors CTR_PSX_REGISTER("$4") = (s16 *)GAME_BATTLE_SCORE_COLORS;
	struct GameTracker *gGT = GAME_TRACKER;
	register u32 colorOffset CTR_PSX_REGISTER("$3") = d->driverID * sizeof(s16);
	register s16 *colorEntry CTR_PSX_REGISTER("$3");
	register u32 color CTR_PSX_REGISTER("$18");
	u32 mode;
	colorOffset += (gGT->numPlyrCurrGame - 1) * sizeof(GAME_BATTLE_SCORE_COLORS[0]);
	CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(colorEntry, colors, colorOffset);
	color = (u16)*colorEntry;
	mode = gGT->gameMode1;

	// NOTE(aalhendi): Keep the unsigned color snapshot across text calls and take
	// each icon/tracker snapshot afterward; joining the branches earlier changes codegen.
	if ((mode & POINT_LIMIT) != 0)
	{
		struct GameTracker *nextGT;
		s32 value = gGT->battleSetup.pointsPerTeam[d->BattleHUD.teamID];
		sprintf(string, GAME_SDATA_NAME(s_longInt), CTR_PRINTF_PSX_LONG(value));
		DecalFont_DrawLine(string, CTR_MipsAddLo(posX, UI_BATTLE_SCORE_TEXT_X_OFFSET), CTR_MipsAddLo(posY, UI_BATTLE_SCORE_TEXT_Y_OFFSET), FONT_SMALL, color);
		CTR_PSX_OBSERVE_VALUE(color);
		nextGT = GAME_TRACKER;
		// NOTE(aalhendi): Retail narrows word-valued HUD coordinates before packet packing.
		DecalHUD_DrawPolyFT4(nextGT->ptrIcons[UI_BATTLE_SCORE_POINTS_ICON], (s16)posX, (s16)posY, &nextGT->backBuffer->primMem, nextGT->pushBuffer_UI.ptrOT, 1,
		                     UI_BATTLE_SCORE_ICON_SCALE);
	}
	else
	{
		struct GameTracker *nextGT;
		if ((mode & LIFE_LIMIT) == 0)
		{
			return;
		}
		sprintf(string, GAME_SDATA_NAME(s_longInt), CTR_PRINTF_PSX_LONG(d->BattleHUD.numLives));
		DecalFont_DrawLine(string, CTR_MipsAddLo(posX, UI_BATTLE_SCORE_TEXT_X_OFFSET), CTR_MipsAddLo(posY, UI_BATTLE_SCORE_TEXT_Y_OFFSET), FONT_SMALL, color);
		CTR_PSX_OBSERVE_VALUE(color);
		nextGT = GAME_TRACKER;
		DecalHUD_DrawPolyFT4(nextGT->ptrIcons[UI_BATTLE_SCORE_LIFE_ICON], (s16)posX, (s16)posY, &nextGT->backBuffer->primMem, nextGT->pushBuffer_UI.ptrOT, 1,
		                     UI_BATTLE_SCORE_ICON_SCALE);
	}
}
