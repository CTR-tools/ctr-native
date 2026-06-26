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
	uint32_t *ot = gGT->backBuffer->otMem.uiOT;
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

	// Draw menu title (text first so it renders on top)
	DecalFont_DrawLineOT(sdata->lngStrings[LNG_OPTIONS],
		0x100, 0x18, FONT_BIG, JUSTIFY_CENTER | ORANGE, ot);

	// Draw section headers and config rows
	static char *s_sectionLabels[] = {
		"General",
		"Adventure",
		"Vehicle",
	};

	int labelX = 0x38;
	int valueX = 0x1DC;
	int sectionX = 0x24;

	int sectionStartY = 0x3C;
	int sectionSpacing = 0x1A;

	for (i = 0; i < 3; i++)
	{
		int sectionY = sectionStartY + i * sectionSpacing;
		int rowY = sectionY + 0x0A;

		// Section header
		DecalFont_DrawLineOT(s_sectionLabels[i], sectionX, sectionY, FONT_SMALL, PERIWINKLE, ot);

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

	// Highlight bar on selected row (drawn after text, rendered between text and separator)
	for (i = 0; i < 3; i++)
	{
		if (i == menu->rowSelected)
		{
			int rowY = sectionStartY + i * sectionSpacing + 0x0A;
			RECT sel = {0x18, rowY - 2, 0x1D0, 0x10};
			CTR_Box_DrawClearBox(&sel, &sdata->menuRowHighlight_Normal, TRANS_50_DECAL, ot);
			break;
		}
	}

	// Separator line (drawn after highlight, rendered between highlight and box)
	{
		RECT sep = {0x20, 0x2C, 0x1C0, 2};
		Color sepColor;
		sepColor.self = sdata->battleSetup_Color_UI_1;
		RECTMENU_DrawOuterRect_Edge(&sep, sepColor, 0x20, ot);
	}

	// Background box last (rendered behind everything)
	RECT bg = {0x10, 4, 0x1E0, 0xCE};
	RECTMENU_DrawInnerRect(&bg, 4, ot);
}
