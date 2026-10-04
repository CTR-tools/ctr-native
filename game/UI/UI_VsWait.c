#include <common.h>

void UI_VsWaitForPressX(void)
{
	SVec2 battleStatsPos3P4P[4];
	SVec2 battleStatsPos2P[2];
	union
	{
		char statText[8];
		Color clearColor;
	} scratch;
	SVec2 position;
	RECT clearRect;
	s16 readyCount = 0;
	s32 playerIndex;
	s32 statPlayerIndex;
	u32 entryPlayerCount;
	char *statPressState;
	SVec2 *ownedPos3P4P = rdata.uiVsWait.positions3P4P;
	Color *clearColor;
	struct GameTracker *tracker = GAME_TRACKER;

	// NOTE(aalhendi): Empty operands retain retail lifetimes and scheduling phases;
	// the local register owners below are matching aids, not a caller ABI.
	CTR_PSX_OBSERVE_VALUE(tracker);
	// Snapshot both layouts before any drawing callbacks.
	memcpy(battleStatsPos3P4P, ownedPos3P4P, sizeof(battleStatsPos3P4P));
	{
		SVec2 *ownedPos2P = sdata_static.Battle_EndOfRace.textPos2P;
		memcpy(battleStatsPos2P, ownedPos2P, sizeof(battleStatsPos2P));
	}
	entryPlayerCount = tracker->numPlyrCurrGame;
	if (entryPlayerCount != 0)
	{
		playerIndex = 0;
		CTR_PSX_OBSERVE_VALUE(entryPlayerCount);
		clearColor = &scratch.clearColor;
		do
		{
			char *pressState;
			struct Driver *driver = tracker->drivers[playerIndex];
			struct PushBuffer *pushBuffer = &tracker->pushBuffer[playerIndex];
			pressState = &sdata_static.Battle_EndOfRace.boolPressX[playerIndex];

			if ((*pressState & 2) == 0)
			{
				// Left/Right switches the displayed attack direction.
				if ((GAMEPADS->gamepad[playerIndex].buttonsTapped & BTN_LEFT) != 0 || (GAMEPADS->gamepad[playerIndex].buttonsTapped & BTN_RIGHT) != 0)
				{
					*pressState ^= 1;
				}
				if ((GAMEPADS->gamepad[playerIndex].buttonsTapped & (BTN_CROSS_one | BTN_START)) != 0 && GAME_TRACKER->timerEndOfRaceVS < 0x78)
				{
					char *readyPress;
					CTR_PSX_OBSERVE_VALUE(pressState);
					readyPress = &sdata_static.Battle_EndOfRace.boolPressX[playerIndex];
					*readyPress ^= 2;
				}
				CTR_PSX_OBSERVE_VALUE(pressState);
				{
					register char *promptPressState CTR_PSX_REGISTER("$17") = sdata_static.Battle_EndOfRace.boolPressX + playerIndex;
					s32 promptLng;
					CTR_PSX_KEEP_VALUE_RELAXED(promptPressState);
					if ((*promptPressState & 1) != 0)
						promptLng = LNG_HIT_YOU;
					else
						promptLng = LNG_YOU_HIT;

					if ((GAME_TRACKER->gameMode1 & BATTLE_MODE) != 0)
					{
						DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[promptLng], pushBuffer->rect.x + (pushBuffer->rect.w >> 1), pushBuffer->rect.y + 0x23,
						                   FONT_CREDITS, JUSTIFY_CENTER | WHITE);
						{
							struct GameTracker *rowTracker = GAME_TRACKER;
							if (rowTracker->numPlyrCurrGame != 0)
							{
								statPlayerIndex = 0;
								CTR_PSX_KEEP_VALUE_RELAXED(promptPressState);
								statPressState = promptPressState;
								do
								{
									// NOTE(aalhendi): Carry the byte offset across table selection so GCC
									// shares the row scale instead of creating another loop counter.
									u32 positionByteOffset = (u32)statPlayerIndex * sizeof(SVec2);
									s32 teamColor = (u16)rowTracker->drivers[statPlayerIndex]->BattleHUD.teamID + (u32)PLAYER_BLUE;
									s32 textX, rowY;
									register s32 textY CTR_PSX_REGISTER("$19");
									register u8 *positionBytes CTR_PSX_REGISTER("$2");
									if (rowTracker->numPlyrCurrGame == 2)
										positionBytes = (u8 *)&battleStatsPos2P;
									else
										positionBytes = (u8 *)&battleStatsPos3P4P;
									CTR_PSX_OBSERVE_VALUE(statPlayerIndex);
									positionBytes += positionByteOffset;
									position.x = ((SVec2 *)positionBytes)->x;
									position.y = ((SVec2 *)positionBytes)->y;
									CTR_PSX_RELOAD(position);
									textX = pushBuffer->rect.x + position.x;
									rowY = pushBuffer->rect.y + position.y;
									if ((*statPressState & 1) != 0)
										sprintf(scratch.statText, rdata.uiVsWait.statFormat, statPlayerIndex + 1,
										        driver->numTimesAttackedByPlayer[statPlayerIndex]);
									else
										sprintf(scratch.statText, rdata.uiVsWait.statFormat, statPlayerIndex + 1,
										        driver->numTimesAttackingPlayer[statPlayerIndex]);
									textY = rowY;
									CTR_PSX_KEEP_VALUE_RELAXED(textY);
									DecalFont_DrawLine(scratch.statText, textX, textY, FONT_SMALL, teamColor | JUSTIFY_CENTER);
									++statPlayerIndex;
									rowTracker = GAME_TRACKER;
								} while (statPlayerIndex < rowTracker->numPlyrCurrGame);
							}
						}
					}
				}
			}
			else
			{
				// Keep a ready viewport clear; Square reopens its statistics.
				memset(clearColor, 0, sizeof(scratch.clearColor));
				clearRect.x = pushBuffer->rect.x;
				clearRect.y = pushBuffer->rect.y;
				clearRect.w = pushBuffer->rect.w;
				clearRect.h = pushBuffer->rect.h;
				CTR_Box_DrawClearBox(&clearRect, clearColor, 0, GAME_TRACKER->backBuffer->otMem.uiOT, &GAME_TRACKER->backBuffer->primMem);
				if ((GAMEPADS->gamepad[playerIndex].buttonsTapped & BTN_SQUARE_two) != 0)
					*pressState ^= 2;
				readyCount++;
			}
			++playerIndex;
			tracker = GAME_TRACKER;
		} while (playerIndex < tracker->numPlyrCurrGame);
	}
	// All ready: return to the shared results menu.
	if (readyCount == GAME_TRACKER->numPlyrCurrGame)
	{
		GAME_TRACKER->timerEndOfRaceVS = 0;
		{
			// NOTE(aalhendi): A whole-sdata byte view keeps the unused final step in bounds.
			char *pressX = (char *)&sdata_static + offsetof(struct sData, Battle_EndOfRace.boolPressX) + 3;
			for (playerIndex = 3; playerIndex >= 0; playerIndex--)
				*pressX-- = 0;
		}
	}
}
