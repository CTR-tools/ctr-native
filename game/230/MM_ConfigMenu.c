#include <common.h>
#include <stdio.h>

// Row arrays with CONFIG entry at bottom, used by MM_MenuProc_Main
struct MenuRow s_rowsMainMenuBasicConfig[] = {
	{0x4C, 0, 1, 0, 0},
	{0x4D, 0, 2, 1, 1},
	{0x4E, 1, 3, 2, 2},
	{0x4F, 2, 4, 3, 3},
	{0x50, 3, 5, 4, 4},
	{0x51, 4, 6, 5, 5},
	{0x0E, 5, 6, 6, 6},
	{-1},
};

struct MenuRow s_rowsMainMenuWithSBConfig[] = {
	{0x4C, 0, 1, 0, 0},
	{0x4D, 0, 2, 1, 1},
	{0x4E, 1, 3, 2, 2},
	{0x4F, 2, 4, 3, 3},
	{0x50, 3, 5, 4, 4},
	{0x51, 4, 6, 5, 5},
	{0x234, 5, 7, 6, 6},
	{0x0E, 6, 7, 7, 7},
	{-1},
};

static void MM_MenuProc_Config(struct RectMenu *menu);

struct RectMenu g_configMenu = {
	.stringIndexTitle = -1,
	.state = EXECUTE_FUNCPTR | DISABLE_INPUT_ALLOW_FUNCPTRS,
	.funcPtr = MM_MenuProc_Config,
};

static void MM_MenuProc_Config(struct RectMenu *menu)
{
	struct GameTracker *gGT = sdata->gGT;
	uint32_t *ot = &gGT->backBuffer->otMem.uiOT[3];
	struct GamepadBuffer *pad = &sdata->gGamepads->gamepad[0];
	char buf[32];
	int i;

	// Exit (Triangle / Start)
	if ((pad->buttonsTapped & (BTN_TRIANGLE | BTN_START)) != 0)
	{
		OtherFX_Play(2, 1);
		NativeConfig_Save();
		sdata->ptrDesiredMenu = &D230.menuMainMenu;
		return;
	}

	// Navigation (D-pad up/down, wrap around)
	if ((pad->buttonsTapped & BTN_UP) != 0)
	{
		menu->rowSelected = (menu->rowSelected > 0) ? menu->rowSelected - 1 : 2;
		OtherFX_Play(0, 1);
	}
	if ((pad->buttonsTapped & BTN_DOWN) != 0)
	{
		menu->rowSelected = (menu->rowSelected < 2) ? menu->rowSelected + 1 : 0;
		OtherFX_Play(0, 1);
	}

	// Row-specific actions
	if ((pad->buttonsTapped & (BTN_CROSS | BTN_CIRCLE)) != 0)
	{
		OtherFX_Play(1, 1);
		switch (menu->rowSelected)
		{
			case 0: g_config.skipIntro ^= 1; break;
			case 1: g_config.skipHints ^= 1; break;
			default: break;
		}
	}

	if (menu->rowSelected == 2)
	{
		int held = pad->buttonsHeldCurrFrame;

		if ((held & BTN_LEFT) != 0 && (sdata->frameCounter % 3) == 0)
		{
			g_config.speedMultiplier -= 10;
			if (g_config.speedMultiplier < 10)
				g_config.speedMultiplier = 10;
		}
		if ((held & BTN_RIGHT) != 0 && (sdata->frameCounter % 3) == 0)
		{
			g_config.speedMultiplier += 10;
			if (g_config.speedMultiplier > 200)
				g_config.speedMultiplier = 200;
		}
	}

	// Draw background box
	RECT bg = {0x46, 0x28, 0x1D4, 0x9C};
	RECTMENU_DrawInnerRect(&bg, 4, ot);

	// Draw menu title
	DecalFont_DrawLineOT(sdata->lngStrings[LNG_OPTIONS],
		0x100, 0x3C, FONT_BIG, JUSTIFY_CENTER | ORANGE, ot);

	// Draw separator line
	{
		RECT sep = {0x64, 0x4E, 0x198, 2};
		Color sepColor;
		sepColor.self = sdata->battleSetup_Color_UI_1;
		RECTMENU_DrawOuterRect_Edge(&sep, sepColor, 0x20, ot);
	}

	// Draw section headers and config rows
	static char *s_sectionLabels[] = {
		"General",
		"Adventure",
		"Vehicle",
	};

	int labelX = 0x72;
	int valueX = 0x1E0;
	int sectionX = 0x5C;

	for (i = 0; i < 3; i++)
	{
		int sectionY = 0x54 + i * 0x24;
		int rowY = sectionY + 0x0C;

		// Section header
		DecalFont_DrawLineOT(s_sectionLabels[i], sectionX, sectionY, FONT_SMALL, PERIWINKLE, ot);

		// Highlight bar on selected row
		if (i == menu->rowSelected)
		{
			RECT sel = {0x50, rowY - 2, 0x1C0, 0x10};
			CTR_Box_DrawClearBox(&sel, &sdata->menuRowHighlight_Normal, TRANS_50_DECAL, ot);
		}

		switch (i)
		{
			case 0:
				DecalFont_DrawLineOT("Skip Intros", labelX, rowY, FONT_SMALL, ORANGE, ot);
				DecalFont_DrawLineOT(g_config.skipIntro ? "ON" : "OFF",
					valueX, rowY, FONT_SMALL, JUSTIFY_RIGHT | WHITE, ot);
				break;
			case 1:
				DecalFont_DrawLineOT("Skip Mask Hints", labelX, rowY, FONT_SMALL, ORANGE, ot);
				DecalFont_DrawLineOT(g_config.skipHints ? "ON" : "OFF",
					valueX, rowY, FONT_SMALL, JUSTIFY_RIGHT | WHITE, ot);
				break;
			case 2:
				DecalFont_DrawLineOT("Kart Speed Multiplier", labelX, rowY, FONT_SMALL, ORANGE, ot);
				sprintf(buf, "%d%%", g_config.speedMultiplier);
				DecalFont_DrawLineOT(buf,
					valueX, rowY, FONT_SMALL, JUSTIFY_RIGHT | WHITE, ot);
				break;
		}
	}
}
