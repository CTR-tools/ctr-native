#include <common.h>
#include <platform/config/native_config.h>

void MM_Cheat_MaxWumpa(void)
{
	sdata_static.gGT->gameMode2 |= CHEAT_WUMPA;
	OtherFX_Play(MM_CHEAT_SUCCESS_SFX, 1);
}

void MM_Cheat_UnlockRoo(void)
{
	GAME_PROGRESS.unlocks[0] |= UNLOCK_ROO;
	OtherFX_Play(MM_CHEAT_SUCCESS_SFX, 1);
}

void MM_Cheat_UnlockPapu(void)
{
	GAME_PROGRESS.unlocks[0] |= UNLOCK_PAPU;
	OtherFX_Play(MM_CHEAT_SUCCESS_SFX, 1);
}

void MM_Cheat_UnlockJoe(void)
{
	GAME_PROGRESS.unlocks[0] |= UNLOCK_JOE;
	OtherFX_Play(MM_CHEAT_SUCCESS_SFX, 1);
}

void MM_Cheat_UnlockPinstripe(void)
{
	GAME_PROGRESS.unlocks[0] |= UNLOCK_PINSTRIPE;
	OtherFX_Play(MM_CHEAT_SUCCESS_SFX, 1);
}

void MM_Cheat_UnlockFakeCrash(void)
{
	GAME_PROGRESS.unlocks[0] |= UNLOCK_FAKE_CRASH;
	OtherFX_Play(MM_CHEAT_SUCCESS_SFX, 1);
}

void MM_Cheat_UnlockPenta(void)
{
	GAME_PROGRESS.unlocks[0] |= UNLOCK_PENTA;
	OtherFX_Play(MM_CHEAT_SUCCESS_SFX, 1);
}

void MM_Cheat_UnlockTropy(void)
{
	GAME_PROGRESS.unlocks[0] |= UNLOCK_TROPY;
	OtherFX_Play(MM_CHEAT_SUCCESS_SFX, 1);
}

void MM_Cheat_UnlockScrapbook(void)
{
	UNLOCK_ADV_BIT(GAME_PROGRESS.unlocks, GAME_UNLOCK_BIT_SCRAPBOOK);
	OtherFX_Play(MM_CHEAT_SUCCESS_SFX, 1);
}

void MM_Cheat_UnlockTracks(void)
{
	GAME_PROGRESS.unlocks[0] |= GAME_UNLOCK_TRACKS_MASK;
	OtherFX_Play(MM_CHEAT_SUCCESS_SFX, 1);
}

void MM_Cheat_InfiniteMasks(void)
{
	sdata_static.gGT->gameMode2 |= CHEAT_MASK;
	OtherFX_Play(MM_CHEAT_SUCCESS_SFX, 1);
}

void MM_Cheat_MaxTurbos(void)
{
	sdata_static.gGT->gameMode2 |= CHEAT_TURBO;
	OtherFX_Play(MM_CHEAT_SUCCESS_SFX, 1);
}

void MM_Cheat_MaxInvisibility(void)
{
	sdata_static.gGT->gameMode2 |= CHEAT_INVISIBLE;
	OtherFX_Play(MM_CHEAT_SUCCESS_SFX, 1);
}

void MM_Cheat_MaxEngine(void)
{
	sdata_static.gGT->gameMode2 |= CHEAT_ENGINE;
	OtherFX_Play(MM_CHEAT_SUCCESS_SFX, 1);
}

void MM_Cheat_MaxBombs(void)
{
	sdata_static.gGT->gameMode2 |= CHEAT_BOMBS;
	OtherFX_Play(MM_CHEAT_SUCCESS_SFX, 1);
}

void MM_Cheat_AdvDifficulty(void)
{
	sdata_static.gGT->gameMode2 |= CHEAT_ADV;
	OtherFX_Play(MM_CHEAT_SUCCESS_SFX, 1);
}

void MM_Cheat_SuperHard(void)
{
	sdata_static.gGT->gameMode2 |= CHEAT_SUPERHARD;
	OtherFX_Play(MM_CHEAT_SUCCESS_SFX, 1);
}

void MM_Cheat_IcyTracks(void)
{
	sdata_static.gGT->gameMode2 |= CHEAT_ICY;
	OtherFX_Play(MM_CHEAT_SUCCESS_SFX, 1);
}

void MM_Cheat_SuperTurboPads(void)
{
	sdata_static.gGT->gameMode2 |= CHEAT_TURBOPAD;
	OtherFX_Play(MM_CHEAT_SUCCESS_SFX, 1);
}

void MM_Cheat_OneLap(void)
{
	sdata_static.gGT->gameMode2 |= CHEAT_ONELAP;
	OtherFX_Play(MM_CHEAT_SUCCESS_SFX, 1);
}

void MM_Cheat_TurboCounter(void)
{
	sdata_static.gGT->gameMode2 |= CHEAT_TURBOCOUNT;
	OtherFX_Play(MM_CHEAT_SUCCESS_SFX, 1);
}

void MM_ParseCheatCodes(void)
{
	struct GamepadBuffer *gpad;
	struct MainMenuCheatCode *cheat;
	struct MainMenuCheatCode *cheats;
	u32 *buttonHistory;
	u32 cheatOffset;
	s32 expectedButtonIndex;
	s32 historyIndex;
	b32 cheatMatches;

	// if not holding L1 and R1
	if (((GAMEPADS->gamepad[0].buttonsHeldCurrFrame & (BTN_L1 | BTN_R1)) ^ (BTN_L1 | BTN_R1)) != 0)
	{
		// skip function
		return;
	}

	if (GAMEPADS->gamepad[0].buttonsTapped == 0)
	{
		return;
	}

	// at this point, must be holding L1 and R1,
	// and also must have tapped a buttons

	// shift the loop
	for (historyIndex = MM_CHEAT_BUTTON_HISTORY_COUNT - 1; historyIndex > 0; historyIndex--)
	{
		MM_CHEAT_BUTTON_HISTORY[historyIndex] = MM_CHEAT_BUTTON_HISTORY[historyIndex - 1];
	}

	buttonHistory = MM_CHEAT_BUTTON_HISTORY;
	gpad = &GAMEPADS->gamepad[0];
	cheats = MM_CHEATS;

	// add to input
	MM_CHEAT_BUTTON_HISTORY[0] = (u32)gpad->buttonsTapped;

	// loop through all cheats
	cheat = cheats;
	cheatOffset = 0;
	do
	{
		cheatMatches = true;
		expectedButtonIndex = cheat->buttonCount - 1;
		historyIndex = 0;

		// check if buttons match this cheat
		while ((expectedButtonIndex >= 0) && cheatMatches)
		{
			// remember, inputButtons is backward
			if ((buttonHistory[historyIndex] &
			     *(u32 *)(cheatOffset + (expectedButtonIndex * sizeof(u32)) + (u32)cheats + OFFSETOF(struct MainMenuCheatCode, buttons))) == 0)
			{
				cheatMatches = false;
			}

			expectedButtonIndex--;
			historyIndex++;
		}

		// skip to next cheat if needed
		if (cheatMatches && (cheat->handler != NULL))
		{
			cheat->handler();
		}

		cheat++;
		cheatOffset += sizeof(*cheat);
	} while (cheat < &cheats[MM_CHEAT_COUNT]);

	return;
}

void MM_Cheat_EnableDevHacks(void)
{
	g_config.developerHacksEnabled = true;
	OtherFX_Play(MM_CHEAT_SUCCESS_SFX, 1);
}
