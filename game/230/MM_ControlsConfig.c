#include <common.h>
#include <text_format.h>

#include <platform/native_controls.h>

#include <stdio.h>
#include <string.h>

#include "MM_ChoicePicker.c"

enum ControlsRowKind
{
	CONTROLS_ROW_DEVICE = 0,
	CONTROLS_ROW_VIBRATION,
	CONTROLS_ROW_BINDING,
	CONTROLS_ROW_DEADZONE,
	CONTROLS_ROW_RESTORE,
};

struct ControlsRow
{
	int kind;
	int binding;
};

// One entry in the device picker: a device to pin, plus the two preference states
// that are not a device. Automatic is what leaves a player resolving itself, and
// Disabled is only offered from player 2 onwards.
struct ControlsChoice
{
	int device;
	int pref;
};

// Rebuilt when the picker opens, so hotplug cannot leave it offering a stale device.
static struct ControlsChoice s_choices[NATIVE_CONTROLS_MAX_DEVICES + 2];
static int s_choiceCount;
static int s_choiceSelected;
static bool s_pickerOpen;

// Which player the list belongs to. Persistent so the menu reopens where it was.
static int s_player;

// Row cursor is menu->rowSelected; the scroll origin is ours.
static struct FullScreenMenu_Scroll s_scroll = {0};

// Rebuilt every frame from the selected player's device, because the device can
// change under the cursor at any time via hotplug or the row above.
static struct ControlsRow s_rows[NATIVE_CONTROLS_BIND_COUNT + 4];
static int s_rowCount;

static char s_valueBuffer[80];
static char s_titleBuffer[32];
// Filtering never grows a string, but a device name is driver-supplied and up to
// NATIVE_CONTROLS_NAME_LENGTH, so this is sized well past the longest label it carries.
static char s_safeBuffer[2 * NATIVE_CONTROLS_NAME_LENGTH];

// The keyboard has no analog axes, so those rows are hidden for it rather than
// shown as unbound. Listed explicitly instead of testing the enum range, so
// reordering the binding enum cannot silently change what is hidden.
static const int s_axisBindings[] = {
    NATIVE_CONTROLS_BIND_AXIS_LEFT_X,
    NATIVE_CONTROLS_BIND_AXIS_LEFT_Y,
    NATIVE_CONTROLS_BIND_AXIS_RIGHT_X,
    NATIVE_CONTROLS_BIND_AXIS_RIGHT_Y,
};

static bool Controls_BindingIsAxis(int binding)
{
	for (unsigned int i = 0; i < sizeof(s_axisBindings) / sizeof(s_axisBindings[0]); i++)
	{
		if (s_axisBindings[i] == binding)
		{
			return true;
		}
	}

	return false;
}

static bool Controls_DeviceHasAnalog(int device)
{
	return (device != NATIVE_CONTROLS_NO_DEVICE) && (NativeControls_GetDeviceKind(device) == NATIVE_CONTROLS_DEVICE_GAMEPAD);
}

static void Controls_BuildRows(void)
{
	const int device = NativeControls_GetPlayerDevice(s_player);
	const bool hasAnalog = Controls_DeviceHasAnalog(device);

	s_rowCount = 0;

	s_rows[s_rowCount].kind = CONTROLS_ROW_DEVICE;
	s_rows[s_rowCount].binding = -1;
	s_rowCount++;

	s_rows[s_rowCount].kind = CONTROLS_ROW_VIBRATION;
	s_rows[s_rowCount].binding = -1;
	s_rowCount++;

	for (int binding = 0; binding < NATIVE_CONTROLS_BIND_COUNT; binding++)
	{
		if (!hasAnalog && Controls_BindingIsAxis(binding))
		{
			continue;
		}

		s_rows[s_rowCount].kind = CONTROLS_ROW_BINDING;
		s_rows[s_rowCount].binding = binding;
		s_rowCount++;
	}

	// Only for a device with axes to trim: the keyboard's stick rows are hidden above, so
	// a deadzone there would be a figure that changes nothing.
	if (hasAnalog)
	{
		s_rows[s_rowCount].kind = CONTROLS_ROW_DEADZONE;
		s_rows[s_rowCount].binding = -1;
		s_rowCount++;
	}

	s_rows[s_rowCount].kind = CONTROLS_ROW_RESTORE;
	s_rows[s_rowCount].binding = -1;
	s_rowCount++;
}

// Fills the picker with what the player could switch to: every device nobody else
// owns, then Automatic, then Disabled for players 2 onwards. Player 1 has no
// Disabled entry, matching NativeControls_SetPlayerPref refusing it.
//
// The cursor starts on whatever the player currently holds, so opening and closing
// without moving leaves things as they were.
static void Controls_BuildChoices(void)
{
	int available[NATIVE_CONTROLS_MAX_DEVICES];
	const int count = NativeControls_GetAvailableDevices(s_player, available, NATIVE_CONTROLS_MAX_DEVICES);
	const int currentDevice = NativeControls_GetPlayerDevice(s_player);
	const int currentPref = NativeControls_GetPlayerPref(s_player);

	s_choiceCount = 0;
	s_choiceSelected = -1;

	for (int i = 0; i < count; i++)
	{
		s_choices[s_choiceCount].device = available[i];
		s_choices[s_choiceCount].pref = NATIVE_CONTROLS_PREF_PINNED;

		if ((currentPref == NATIVE_CONTROLS_PREF_PINNED) && (available[i] == currentDevice))
		{
			s_choiceSelected = s_choiceCount;
		}

		s_choiceCount++;
	}

	// Automatic resolves the player against whatever is free, which is how a disabled
	// player gets a device back when every device is currently owned by someone else.
	s_choices[s_choiceCount].device = NATIVE_CONTROLS_NO_DEVICE;
	s_choices[s_choiceCount].pref = NATIVE_CONTROLS_PREF_AUTO;

	if (currentPref == NATIVE_CONTROLS_PREF_AUTO)
	{
		s_choiceSelected = s_choiceCount;
	}

	s_choiceCount++;

	if (s_player != 0)
	{
		s_choices[s_choiceCount].device = NATIVE_CONTROLS_NO_DEVICE;
		s_choices[s_choiceCount].pref = NATIVE_CONTROLS_PREF_DISABLED;

		if (currentPref == NATIVE_CONTROLS_PREF_DISABLED)
		{
			s_choiceSelected = s_choiceCount;
		}

		s_choiceCount++;
	}

	if (s_choiceSelected < 0)
	{
		s_choiceSelected = 0;
	}
}

static const char *Controls_ChoiceName(const struct ControlsChoice *choice)
{
	if (choice->pref == NATIVE_CONTROLS_PREF_AUTO)
	{
		return LANG_GetString(LNG_EXPANDED_AUTOMATIC);
	}

	if (choice->pref == NATIVE_CONTROLS_PREF_DISABLED)
	{
		return LANG_GetString(LNG_EXPANDED_DISABLED);
	}

	return NativeControls_GetDeviceKind(choice->device) == NATIVE_CONTROLS_DEVICE_KEYBOARD
	           ? LANG_GetString(LNG_EXPANDED_CONTROLS_KEYBOARD) : NativeControls_GetDeviceName(choice->device);
}

// The retail font has no glyph for many ASCII characters and repurposes four of them
// (@ [ ^ *) as button artwork, so free-form text reaching DecalFont with one of those
// either comes out blank or turns into a button nobody asked for. Device names come from
// the driver and can hold anything, so every string on this screen is filtered down to
// characters the font can actually show. The four button characters stay reachable
// through s_bindingGlyphs, which is the only path meant to draw them.
static bool Controls_IsSafeChar(char c)
{
	const u8 code = (u8)c;

	// Space, colon and period are handled outside the glyph table.
	if ((code == ' ') || (code == ':') || (code == '.'))
	{
		return true;
	}

	// Below 0x21 is the zero-advance accent run, and 0x21 upwards is the glyph table.
	if (code < 0x21)
	{
		return true;
	}

	switch (code)
	{
	case '@':
	case '[':
	case '^':
	case '*':
		return false;
	default:
		break;
	}

	return data.font_characterIconID[code - 0x21] != 0xFF;
}

// Copies text through the filter. Never grows the string, so dstSize only has to match
// the source.
static void Controls_FilterText(const char *text, char *dst, size_t dstSize)
{
	size_t out = 0;

	for (const char *c = text; (*c != '\0') && (out + 1 < dstSize); c++)
	{
		if (Controls_IsSafeChar(*c))
		{
			dst[out++] = *c;
		}
	}

	dst[out] = '\0';
}

static void Controls_OpenPicker(void)
{
	Controls_BuildChoices();
	s_pickerOpen = true;
}

static void Controls_ClosePicker(void)
{
	s_pickerOpen = false;
}

static bool Controls_ApplyChoice(const void *context, int selected)
{
	const struct ControlsChoice *choice = &s_choices[selected];

	(void)context;
	NativeControls_SetPlayerPref(s_player, choice->pref, choice->device);
	return true;
}

// One picker entry, filtered into the shared buffer. Measured after filtering, or the box
// would be sized to characters that never reach the screen. Neither caller draws through
// Controls_DrawSafeText, so the buffer can be shared.
static const char *Controls_FilteredChoiceName(const void *context, int index)
{
	(void)context;
	Controls_FilterText(Controls_ChoiceName(&s_choices[index]), s_safeBuffer, sizeof(s_safeBuffer));
	return s_safeBuffer;
}

static void Controls_DrawPicker(struct GameTracker *gGT, uint32_t *ot)
{
	ChoicePicker_Draw(LANG_GetString(LNG_EXPANDED_SELECT_DEVICE), s_choiceCount, s_choiceSelected, Controls_FilteredChoiceName, NULL, gGT, ot);
}

// Union of player 1's presses and the selected player's own, so the screen stays reachable
// when the selected player is disabled or has not touched their pad yet. A disabled player
// reads as no device and contributes nothing.
u32 MM_Controls_MenuTapped(void)
{
	u32 tapped = (u32)sdata->gGamepads->gamepad[0].buttonsTapped;

	if (s_player != 0)
	{
		tapped |= (u32)sdata->gGamepads->gamepad[s_player].buttonsTapped;
	}

	return tapped;
}

// The held counterpart of MM_Controls_MenuTapped, for the option rows: a numeric option
// reads a direction being held rather than the edge that opens a rebind.
u32 MM_Controls_MenuHeld(void)
{
	u32 held = (u32)sdata->gGamepads->gamepad[0].buttonsHeldCurrFrame;

	if (s_player != 0)
	{
		held |= (u32)sdata->gGamepads->gamepad[s_player].buttonsHeldCurrFrame;
	}

	return held;
}

static void Controls_SelectPlayer(int player)
{
	if ((player < 0) || (player >= PLATFORM_INPUT_PAD_COUNT))
	{
		return;
	}

	s_player = player;
}

// Nudges the deadzone like a config slider: same cadence for a held direction, same
// clamping at the ends.
static void Controls_AdjustDeadzone(int device, int held)
{
	if (device == NATIVE_CONTROLS_NO_DEVICE)
	{
		OtherFX_Play(2, 1);
		return;
	}

	for (int direction = -1; direction <= 1; direction += 2)
	{
		if (!FullScreenMenu_OptionStepDue(held, direction))
		{
			continue;
		}

		int value = NativeControls_GetDeadzone(device);

		if (FullScreenMenu_OptionStep(&value, direction, NATIVE_CONTROLS_DEADZONE_MIN, NATIVE_CONTROLS_DEADZONE_MAX,
		                      NATIVE_CONTROLS_DEADZONE_STEP))
		{
			NativeControls_SetDeadzone(device, value);
			OtherFX_Play(1, 1);
		}
	}
}

static void Controls_HandleConfirm(int device, const struct ControlsRow *row)
{
	switch (row->kind)
	{
	case CONTROLS_ROW_DEVICE:
		Controls_OpenPicker();
		OtherFX_Play(0, 1);
		break;

	case CONTROLS_ROW_DEADZONE:
		// Left and Right are the editing gesture here; Cross has nothing to confirm.
		OtherFX_Play(2, 1);
		break;

	case CONTROLS_ROW_VIBRATION:
		if (!NativeControls_CanVibrate(device))
		{
			OtherFX_Play(2, 1);
			break;
		}
		NativeControls_SetPlayerVibration(s_player, !NativeControls_GetPlayerVibration(s_player));
		OtherFX_Play(1, 1);
		break;

	case CONTROLS_ROW_BINDING:
		if (device == NATIVE_CONTROLS_NO_DEVICE)
		{
			// Nothing to rebind without a device.
			OtherFX_Play(2, 1);
			break;
		}

		NativeControls_BeginCapture(device, row->binding);
		OtherFX_Play(1, 1);
		break;

	case CONTROLS_ROW_RESTORE:
		if (device == NATIVE_CONTROLS_NO_DEVICE)
		{
			OtherFX_Play(2, 1);
			break;
		}

		NativeControls_RestoreDeviceDefaults(device);
		OtherFX_Play(1, 1);
		break;

	default:
		break;
	}
}

// Draws a shoulder-button hint on the title row. rightJustify anchors the text's
// right edge to x, matching how the value column is laid out.
//
// Dropped by half the difference in glyph height so the smaller text sits centred
// against the big title rather than hanging off its top edge.
static void Controls_DrawHint(const char *text, int x, bool rightJustify, uint32_t *ot)
{
	const int textWidth = DecalFont_GetLineWidth((char *)text, FONT_SMALL);
	const int textX = rightJustify ? (x - textWidth) : x;
	const int textY = FullScreenMenu_TitleY(s_rowCount) + FULLSCREEN_MENU_TITLE_BASELINE_NUDGE_Y;

	DecalFont_DrawLineOT((char *)text, textX, textY, FONT_SMALL, ORANGE, ot);
}

// Button artwork for the four face buttons, indexed by binding. The retail font has no
// glyph slots for them: DecalFont recognises four ordinary ASCII characters and
// substitutes the button artwork, so these rows show the same icons the rest of the game
// uses instead of button names, and the glyph is the whole label. NULL for everything
// else, since the shoulders, d-pad, start/select and sticks have no glyph.
static const char *const s_bindingGlyphs[NATIVE_CONTROLS_BIND_COUNT] = {
    [NATIVE_CONTROLS_BIND_CROSS] = "*",
    [NATIVE_CONTROLS_BIND_CIRCLE] = "@",
    [NATIVE_CONTROLS_BIND_TRIANGLE] = "^",
    [NATIVE_CONTROLS_BIND_SQUARE] = "[",
};

static const int s_bindingLabels[NATIVE_CONTROLS_BIND_COUNT] = {
	[NATIVE_CONTROLS_BIND_L1] = LNG_EXPANDED_CONTROLS_L1,
	[NATIVE_CONTROLS_BIND_L2] = LNG_EXPANDED_CONTROLS_L2,
	[NATIVE_CONTROLS_BIND_L3] = LNG_EXPANDED_CONTROLS_L3,
	[NATIVE_CONTROLS_BIND_R1] = LNG_EXPANDED_CONTROLS_R1,
	[NATIVE_CONTROLS_BIND_R2] = LNG_EXPANDED_CONTROLS_R2,
	[NATIVE_CONTROLS_BIND_R3] = LNG_EXPANDED_CONTROLS_R3,
	[NATIVE_CONTROLS_BIND_DPAD_UP] = LNG_EXPANDED_CONTROLS_UP,
	[NATIVE_CONTROLS_BIND_DPAD_DOWN] = LNG_EXPANDED_CONTROLS_DOWN,
	[NATIVE_CONTROLS_BIND_DPAD_LEFT] = LNG_EXPANDED_CONTROLS_LEFT,
	[NATIVE_CONTROLS_BIND_DPAD_RIGHT] = LNG_EXPANDED_CONTROLS_RIGHT,
	[NATIVE_CONTROLS_BIND_START] = LNG_EXPANDED_CONTROLS_START,
	[NATIVE_CONTROLS_BIND_SELECT] = LNG_EXPANDED_CONTROLS_SELECT,
	[NATIVE_CONTROLS_BIND_AXIS_LEFT_X] = LNG_EXPANDED_CONTROLS_LEFT_STICK_X,
	[NATIVE_CONTROLS_BIND_AXIS_LEFT_Y] = LNG_EXPANDED_CONTROLS_LEFT_STICK_Y,
	[NATIVE_CONTROLS_BIND_AXIS_RIGHT_X] = LNG_EXPANDED_CONTROLS_RIGHT_STICK_X,
	[NATIVE_CONTROLS_BIND_AXIS_RIGHT_Y] = LNG_EXPANDED_CONTROLS_RIGHT_STICK_Y,
};

static void Controls_DrawSafeText(const char *text, int x, int y, int flags, uint32_t *ot)
{
	Controls_FilterText(text, s_safeBuffer, sizeof(s_safeBuffer));
	DecalFont_DrawLineOT(s_safeBuffer, x, y, FONT_SMALL, flags, ot);
}

static void Controls_DrawValue(int device, const struct ControlsRow *row, int y, uint32_t *ot)
{
	if (row->kind == CONTROLS_ROW_RESTORE)
	{
		return;
	}

	if (row->kind == CONTROLS_ROW_VIBRATION)
	{
		const bool enabled = NativeControls_GetPlayerVibration(s_player);
		FullScreenMenu_DrawOnOffColored(enabled, y, NativeControls_CanVibrate(device) ? WHITE : GRAY, ot);
		return;
	}

	if ((row->kind == CONTROLS_ROW_DEADZONE) && (device != NATIVE_CONTROLS_NO_DEVICE))
	{
		FullScreenMenu_DrawPercent(NativeControls_GetDeadzone(device), y, ot);
		return;
	}

	// Grey means there is nothing here to rebind, because the player has no device.
	const char *text;
	int colour;

	if (device == NATIVE_CONTROLS_NO_DEVICE)
	{
		text = (row->kind == CONTROLS_ROW_DEVICE) ? LANG_GetString(LNG_EXPANDED_DISABLED) : "-";
		colour = GRAY;
	}
	else if (row->kind == CONTROLS_ROW_DEVICE)
	{
		text = NativeControls_GetDeviceKind(device) == NATIVE_CONTROLS_DEVICE_KEYBOARD
		           ? LANG_GetString(LNG_EXPANDED_CONTROLS_KEYBOARD) : NativeControls_GetDeviceName(device);
		colour = WHITE;
	}
	else
	{
		NativeControls_GetBindingLabel(device, row->binding, s_valueBuffer, sizeof(s_valueBuffer));
		text = (s_valueBuffer[0] != '\0') ? s_valueBuffer : LANG_GetString(LNG_EXPANDED_NONE);
		colour = WHITE;
	}

	Controls_DrawSafeText(text, FULLSCREEN_MENU_VALUE_X, y, JUSTIFY_RIGHT | colour, ot);
}

static void Controls_DrawRow(const void *context, int index, int y, bool selected, uint32_t *ot)
{
	const int device = *(const int *)context;
	const struct ControlsRow *row = &s_rows[index];
	const bool capturing = NativeControls_IsCapturing();
	const char *label = "";

	switch (row->kind)
	{
	case CONTROLS_ROW_DEVICE:
		label = LANG_GetString(LNG_EXPANDED_DEVICE);
		break;
	case CONTROLS_ROW_VIBRATION:
		label = LANG_GetString(LNG_EXPANDED_VIBRATION);
		break;
	case CONTROLS_ROW_BINDING:
		label = s_bindingGlyphs[row->binding];
		if (label == NULL)
		{
			label = LANG_GetString(s_bindingLabels[row->binding]);
		}
		break;
	case CONTROLS_ROW_DEADZONE:
		label = LANG_GetString(LNG_EXPANDED_DEADZONE);
		break;
	case CONTROLS_ROW_RESTORE:
		label = LANG_GetString(LNG_EXPANDED_RESTORE_DEFAULTS);
		break;
	default:
		break;
	}

	// Binding glyphs must reach the font without filtering.
	FullScreenMenu_DrawRowLabel(label, y, ot);

	if (capturing && row->kind == CONTROLS_ROW_BINDING && selected)
	{
		const char *prompt = NativeControls_IsConfirming() ? LANG_GetString(LNG_EXPANDED_CONTROLS_CONFIRM_BINDING) : "...";
		DecalFont_DrawLineOT((char *)prompt, FULLSCREEN_MENU_VALUE_X, y, FONT_SMALL, JUSTIFY_RIGHT | RED, ot);
	}
	else
	{
		Controls_DrawValue(device, row, y, ot);
	}
}

b32 MM_Controls_IsPickerOpen(void)
{
	return s_pickerOpen;
}

void MM_Controls_ResetNavigation(void)
{
	Controls_ClosePicker();
	s_scroll.offset = 0;
	NativeControls_CancelCapture();
}

// Moves the cursor with the same minimal scrolling as the other full-screen menus, so the
// list only scrolls once the cursor reaches the edge of the visible rows.
static void Controls_MoveSelection(struct RectMenu *menu, int delta)
{
	menu->rowSelected = (s16)FullScreenMenu_MoveRow(&s_scroll, menu->rowSelected, delta, s_rowCount);
	OtherFX_Play(0, 1);
}

static void Controls_ClampSelection(struct RectMenu *menu)
{
	FullScreenMenu_FollowScroll(&s_scroll, &menu->rowSelected, s_rowCount);
}

// Player switching stays on player 1's shoulders only. Reading it from the selected player
// too would mean their own L1/R1 changed who is selected, so a second press moved them
// twice.
static void Controls_HandlePlayerSwitch(struct RectMenu *menu, u32 playerOneTapped)
{
	int delta = 0;

	if ((playerOneTapped & BTN_R1) != 0)
	{
		delta = 1;
	}
	else if ((playerOneTapped & BTN_L1) != 0)
	{
		delta = -1;
	}

	if (delta == 0)
	{
		return;
	}

	Controls_SelectPlayer(s_player + delta);
	menu->rowSelected = 0;
	s_scroll.offset = 0;
	OtherFX_Play(0, 1);
}

// The picker owns input while it is up, and a rebind owns it while listening. Either one
// suppresses the screen underneath so their presses are unambiguous.
static void Controls_HandleInput(struct RectMenu *menu, u32 tapped, u32 held, int device)
{
	if (s_pickerOpen)
	{
		ChoicePicker_UpdateAndApply(&s_pickerOpen, &s_choiceSelected, s_choiceCount, tapped, Controls_ApplyChoice, NULL);
		return;
	}

	if (NativeControls_IsCapturing())
	{
		return;
	}

	const u32 playerOneTapped = (u32)sdata->gGamepads->gamepad[0].buttonsTapped;
	Controls_HandlePlayerSwitch(menu, playerOneTapped);

	if ((tapped & BTN_UP) != 0)
	{
		Controls_MoveSelection(menu, -1);
	}
	else if ((tapped & BTN_DOWN) != 0)
	{
		Controls_MoveSelection(menu, 1);
	}

	// Left/Right and Cross all open the picker on the device row, so the choices
	// are visible before anything changes.
	if ((s_rows[menu->rowSelected].kind == CONTROLS_ROW_DEVICE) && ((tapped & (BTN_LEFT | BTN_RIGHT)) != 0))
	{
		Controls_OpenPicker();
		OtherFX_Play(0, 1);
	}
	else if (s_rows[menu->rowSelected].kind == CONTROLS_ROW_DEADZONE)
	{
		Controls_AdjustDeadzone(device, held);
	}

	if ((tapped & BTN_CROSS) != 0)
	{
		Controls_HandleConfirm(device, &s_rows[menu->rowSelected]);
	}
}

// Distinct sound from a commit, so an abandoned rebind is not mistaken for a confirmed one.
static void Controls_PollCapture(void)
{
	const int result = NativeControls_PollCapture();

	if (result == 1)
	{
		OtherFX_Play(1, 1);
	}
	else if (result == 2)
	{
		OtherFX_Play(2, 1);
	}
}

static void Controls_DrawTitle(uint32_t *ot)
{
	// Uppercase here, unlike the section name used in the options list: the retail
	// titles this shares a font with are all caps.
	char player[16];
	snprintf(player, sizeof(player), "%d", s_player + 1);
	const struct TextFormatValue values[] = {{"player", player}};
	if (!TextFormat_Apply(s_titleBuffer, sizeof(s_titleBuffer), LANG_GetString(LNG_EXPANDED_CONTROLS_PLAYER_TITLE), values, len(values)))
	{
		s_titleBuffer[0] = '\0';
	}
	FullScreenMenu_DrawTitle(s_titleBuffer, s_rowCount, 0, ot);
}

void MM_MenuProc_Controls(struct RectMenu *menu)
{
	struct GameTracker *gGT = sdata->gGT;
	uint32_t *ot = gGT->backBuffer->otMem.uiOT;

	Controls_BuildRows();
	Controls_ClampSelection(menu);

	// Read before the input handling, which can move the selection to another player.
	const int device = NativeControls_GetPlayerDevice(s_player);
	Controls_HandleInput(menu, MM_Controls_MenuTapped(), MM_Controls_MenuHeld(), device);
	Controls_PollCapture();

	// This OT draws back to front: a primitive added later sits behind the ones already in
	// the list, which is why FullScreenMenu_DrawFrame can draw the opaque menu background after
	// every row and still leave the text visible. The picker therefore has to be submitted
	// before the screen it covers, while inside Controls_DrawPicker the order is the mirror
	// image and the popup's own background goes last.
	if (s_pickerOpen)
	{
		Controls_DrawPicker(gGT, ot);
	}

	Controls_DrawTitle(ot);
	Controls_DrawHint("L1", FULLSCREEN_MENU_LABEL_X, false, ot);
	Controls_DrawHint("R1", FULLSCREEN_MENU_VALUE_X, true, ot);
	const int drawDevice = NativeControls_GetPlayerDevice(s_player);
	FullScreenMenu_DrawRows(&s_scroll, s_rowCount, menu->rowSelected, Controls_DrawRow, &drawDevice, gGT, ot);
	FullScreenMenu_DrawFrame(s_rowCount);
}
