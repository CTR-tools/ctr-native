#include <common.h>

#include "MM_FullScreenMenu.c"

enum AudioConfigConstants
{
	AUDIO_CONFIG_ROW_MODE = GAME_OPTIONS_VOLUME_COUNT,
	AUDIO_CONFIG_ROW_COUNT,
	AUDIO_CONFIG_VOLUME_MIN = 0,
	AUDIO_CONFIG_VOLUME_MAX = 0xff,
	AUDIO_CONFIG_VOLUME_STEP = 4,
	AUDIO_CONFIG_SLIDER_X = FULLSCREEN_MENU_TITLE_X,
	AUDIO_CONFIG_SLIDER_HEIGHT = 10,
};

static const int s_audioConfigLabels[AUDIO_CONFIG_ROW_COUNT] = {
	LNG_FX, LNG_MUSIC, LNG_VOICE, LNG_MODE,
};

static void AudioConfig_DrawRow(const void *context, int row, int y, bool selected, uint32_t *ot)
{
	(void)context;
	(void)selected;
	char label[128];
	snprintf(label, sizeof(label), "%s", sdata->lngStrings[s_audioConfigLabels[row]]);
	size_t length = strlen(label);
	while (length > 0 && (label[length - 1] == ':' || label[length - 1] == ' '))
	{
		label[--length] = '\0';
	}
	FullScreenMenu_DrawRowLabel(label, y, ot);
	if (row == AUDIO_CONFIG_ROW_MODE)
	{
		FullScreenMenu_DrawValueText(sdata->lngStrings[howl_ModeGet() ? LNG_STEREO : LNG_MONO], y, ot);
	}
	else
	{
		const int volume = howl_VolumeGet(row) & AUDIO_CONFIG_VOLUME_MAX;
		RECT slider = {AUDIO_CONFIG_SLIDER_X, y - 2,
		               FULLSCREEN_MENU_VALUE_X - AUDIO_CONFIG_SLIDER_X, AUDIO_CONFIG_SLIDER_HEIGHT};
		UI_DrawVolumeSlider(&slider, volume, ot);
	}
}

void MM_AudioConfig_Leave(void)
{
	OptionsMenu_TestSound(HOWL_VOLUME_TYPE_FX, false);
	RaceConfig_SaveGameOptions();
}

void MM_MenuProc_Audio(struct RectMenu *menu)
{
	struct GameTracker *gGT = sdata->gGT;
	struct GamepadBuffer *pad = &sdata->gGamepads->gamepad[0];
	struct FullScreenMenu_Scroll scroll = {0};
	FullScreenMenu_NavScroll(pad, &scroll, &menu->rowSelected, AUDIO_CONFIG_ROW_COUNT);

	const int row = menu->rowSelected;
	if (row == AUDIO_CONFIG_ROW_MODE)
	{
		OptionsMenu_TestSound(HOWL_VOLUME_TYPE_FX, false);
		if ((pad->buttonsTapped & (BTN_CROSS | BTN_CIRCLE)) != 0)
		{
			OtherFX_Play(1, 1);
			howl_ModeSet(howl_ModeGet() == 0);
		}
	}
	else
	{
		if (row == HOWL_VOLUME_TYPE_MUSIC && (gGT->gameMode1 & MAIN_MENU) != 0)
		{
			OptionsMenu_TestSound(HOWL_VOLUME_TYPE_FX, false);
		}
		else
		{
			OptionsMenu_TestSound(row, true);
		}
		// Match the original audio sliders: four units per held frame, Left takes priority.
		const int held = pad->buttonsHeldCurrFrame;
		if ((held & (BTN_LEFT | BTN_RIGHT)) != 0)
		{
			int volume = howl_VolumeGet(row) & AUDIO_CONFIG_VOLUME_MAX;
			const int direction = (held & BTN_LEFT) != 0 ? -1 : 1;
			if (FullScreenMenu_OptionStep(&volume, direction, AUDIO_CONFIG_VOLUME_MIN,
			                            AUDIO_CONFIG_VOLUME_MAX, AUDIO_CONFIG_VOLUME_STEP))
			{
				howl_VolumeSet(row, (u8)volume);
			}
		}
	}

	FullScreenMenu_DrawScreen(LANG_GetString(LNG_EXPANDED_CONFIG_AUDIO), 0, &scroll, AUDIO_CONFIG_ROW_COUNT, menu->rowSelected,
	                          AudioConfig_DrawRow, NULL, gGT, gGT->backBuffer->otMem.uiOT);
}
