#include <common.h>

// Retail 0x800116ec: Battle end stat positions for 3P/4P.
static const SVec2 s_battleStatsPos3P4P[4] = {
    {0x55, 0x35},
    {0xaa, 0x35},
    {0x55, 0x43},
    {0xaa, 0x43},
};

enum
{
	UI_VS_WAIT_STAT_MODE_HIT_YOU = 1,
	UI_VS_WAIT_PLAYER_READY = 2,
	UI_VS_WAIT_READY_COOLDOWN_FRAMES = 0x78,
	UI_VS_WAIT_PROMPT_Y_OFFSET = 0x23,
	UI_VS_WAIT_STAT_TEXT_SIZE = 8,
	UI_VS_WAIT_TEAM_COLOR_BASE = PLAYER_BLUE,
	UI_VS_WAIT_TEAM_TEXT_FLAGS = JUSTIFY_CENTER,
};

CTR_STATIC_ASSERT(LNG_HIT_YOU == LNG_YOU_HIT + 1);

void UI_VsWaitForPressX(void)
{
	char statText[UI_VS_WAIT_STAT_TEXT_SIZE];
	Color clearColor;
	RECT clearRect;

	struct GameTracker *gGT = sdata->gGT;
	u8 numPlayers = gGT->numPlyrCurrGame;

	int readyCount = 0;

	for (s32 playerIndex = 0; playerIndex < numPlayers; playerIndex++)
	{
		// flags, for which players have pressed X to continue
		char *pressState = &sdata->Battle_EndOfRace.boolPressX[playerIndex];

		struct Driver *driver = gGT->drivers[playerIndex];
		RECT *viewport = &gGT->pushBuffer[playerIndex].rect;
		int buttonsTapped = sdata->gGamepads->gamepad[playerIndex].buttonsTapped;

		// If Player has not pressed X to continue
		// Draw comment, and battle stats
		if ((*pressState & UI_VS_WAIT_PLAYER_READY) == 0)
		{
			// If you hit left or right on the D-Pad, or Analog Stick
			if ((buttonsTapped & (BTN_LEFT | BTN_RIGHT)) != 0)
			{
				// Invert &1 bit
				*pressState = *pressState ^ UI_VS_WAIT_STAT_MODE_HIT_YOU;
			}

			if (
			    // ready to continue, after cooldown
			    (gGT->timerEndOfRaceVS < UI_VS_WAIT_READY_COOLDOWN_FRAMES) && ((buttonsTapped & (BTN_CROSS_one | BTN_START)) != 0))
			{
				// invert &2 bit
				*pressState = *pressState ^ UI_VS_WAIT_PLAYER_READY;
			}

			// If you're in Battle Mode
			if ((gGT->gameMode1 & BATTLE_MODE) != 0)
			{
				int promptString = LNG_YOU_HIT + (*pressState & UI_VS_WAIT_STAT_MODE_HIT_YOU);

				DecalFont_DrawLine(sdata->lngStrings[promptString],

				                   // Midpoint between pushBuffer Start X and End X
				                   viewport->x + (viewport->w >> 1),

				                   (viewport->y + UI_VS_WAIT_PROMPT_Y_OFFSET),

				                   FONT_CREDITS, JUSTIFY_CENTER | WHITE);

				// If you have 3 screens, you need 9 prints
				// If you have 4 screens, you need 16 prints

				// for (a in players)
				// for (b in players)
				// print a hit b
				// print b hit a

				const SVec2 *battleStatsPos = s_battleStatsPos3P4P;
				if (numPlayers == 2)
				{
					battleStatsPos = sdata->Battle_EndOfRace.textPos2P;
				}

				for (s32 statPlayerIndex = 0; statPlayerIndex < numPlayers; statPlayerIndex++)
				{
					// YOU HIT THEM
					u8 numAttacked;
					if ((*pressState & UI_VS_WAIT_STAT_MODE_HIT_YOU) == 0)
					{
						numAttacked = driver->numTimesAttackingPlayer[statPlayerIndex];
					}

					// HIT YOU
					else
					{
						numAttacked = driver->numTimesAttackedByPlayer[statPlayerIndex];
					}

					sprintf(statText, "p%d:%2.02d",

					        // basically, j + 1
					        // which is (1, 2, 3, 4)
					        (statPlayerIndex + 1),

					        // Amount of times this player hit you,
					        // or amount of times you hit them
					        numAttacked);


					// Get font color based on battle team
					s16 teamID = (s16)gGT->drivers[statPlayerIndex]->BattleHUD.teamID;
					int statTextFlags = ((teamID + (u32)UI_VS_WAIT_TEAM_COLOR_BASE) | UI_VS_WAIT_TEAM_TEXT_FLAGS);


					DecalFont_DrawLine(statText,

					                   // midpoint between Start X and Size X
					                   (viewport->x + battleStatsPos[statPlayerIndex].x),

					                   // midpoint between Start Y and Size Y
					                   (viewport->y + battleStatsPos[statPlayerIndex].y),

					                   FONT_SMALL, statTextFlags);
				}
			}
		}

		// If Player has pressed X to continue
		else
		{
			// Stop drawing comment + battle stats

			memset(&clearColor, 0, sizeof(clearColor));
			clearRect = *viewport;
			CTR_Box_DrawClearBox(&clearRect, &clearColor, 0, gGT->backBuffer->otMem.uiOT, &gGT->backBuffer->primMem);

			// Allow Go-Back option to YouHit/HitYou
			if ((buttonsTapped & BTN_SQUARE_two) != 0)
			{
				// invert &2 bit
				*pressState = *pressState ^ UI_VS_WAIT_PLAYER_READY;
			}

			// increment counter of players ready to continue
			readyCount++;
		}
	}

	// If all players press X to continue
	if (readyCount == numPlayers)
	{
		// Stop drawing 4 screens, draw 1 screen and options
		gGT->timerEndOfRaceVS = 0;
		*(int *)&sdata->Battle_EndOfRace.boolPressX[0] = 0;
	}
}
