#include <shared/ctr_test.h>
#include <common.h>
#include <platform/native_controls.h>

struct sData sdata_static;
struct Data data;
static struct GameTracker s_tracker;
static struct DB s_buffer;
static struct RectMenu s_menu;
static int s_devices[PLATFORM_INPUT_PAD_COUNT];
static bool s_rumble[PLATFORM_INPUT_PAD_COUNT];
static bool s_vibration[PLATFORM_INPUT_PAD_COUNT];
static bool s_capturing;
static bool s_drawingVibration;
static int s_vibrationColor;
static char s_vibrationValue[2 * NATIVE_CONTROLS_NAME_LENGTH];
static int s_restores;
static int s_preferenceChanges;

#include <game/230/MM_FullScreenMenu.c>
#include <game/230/MM_ControlsConfig.c>
#include <game/RaceConfig.c>

enum { TEST_KEYBOARD_DEVICE = PLATFORM_INPUT_PAD_COUNT };

int NativeControls_GetPlayerDevice(int player) { return s_devices[player]; }
int NativeControls_GetPlayerPref(int player) { return s_devices[player] < 0 ? NATIVE_CONTROLS_PREF_DISABLED : NATIVE_CONTROLS_PREF_PINNED; }
int NativeControls_GetDeviceKind(int device)
{
	return device < 0 ? NATIVE_CONTROLS_DEVICE_NONE :
	       device == TEST_KEYBOARD_DEVICE ? NATIVE_CONTROLS_DEVICE_KEYBOARD : NATIVE_CONTROLS_DEVICE_GAMEPAD;
}
bool NativeControls_CanVibrate(int device)
{
	return device >= 0 && device < PLATFORM_INPUT_PAD_COUNT && s_rumble[device];
}
bool NativeControls_GetPlayerVibration(int player) { return s_vibration[player]; }
void NativeControls_SetPlayerVibration(int player, bool enabled) { s_vibration[player] = enabled; }
const char *NativeControls_GetDeviceName(int device) { return device == TEST_KEYBOARD_DEVICE ? "Keyboard" : "Gamepad"; }
int NativeControls_GetAvailableDevices(int player, int *dst, int max)
{
	int count = 0;
	for (int device = 0; device <= TEST_KEYBOARD_DEVICE && count < max; device++)
	{
		bool owned = false;
		for (int other = 0; other < PLATFORM_INPUT_PAD_COUNT; other++)
		{
			owned |= other != player && s_devices[other] == device;
		}
		if (!owned)
		{
			dst[count++] = device;
		}
	}
	return count;
}
int NativeControls_SetPlayerPref(int player, int pref, int device)
{
	s_devices[player] = pref == NATIVE_CONTROLS_PREF_DISABLED ? NATIVE_CONTROLS_NO_DEVICE : device;
	s_preferenceChanges++;
	return 1;
}
const char *NativeControls_GetBindingName(int binding) { (void)binding; return "Binding"; }
void NativeControls_GetBindingLabel(int device, int binding, char *dst, size_t size)
{
	(void)device; (void)binding;
	snprintf(dst, size, "A");
}
s32 NativeControls_GetDeadzone(int device) { (void)device; return 0; }
void NativeControls_SetDeadzone(int device, s32 value) { (void)device; (void)value; }
void NativeControls_RestoreDeviceDefaults(int device) { (void)device; s_restores++; }
void NativeControls_BeginCapture(int device, int binding) { (void)device; (void)binding; s_capturing = true; }
int NativeControls_IsCapturing(void) { return s_capturing; }
int NativeControls_IsConfirming(void) { return 0; }
int NativeControls_PollCapture(void) { return 0; }
void NativeControls_CancelCapture(void) { s_capturing = false; }
s32 howl_VolumeGet(s32 type) { (void)type; return 255; }
void howl_VolumeSet(s32 type, u8 volume) { (void)type; (void)volume; }
s32 howl_ModeGet(void) { return 1; }
void howl_ModeSet(s32 mode) { (void)mode; }
int OtherFX_Play(u32 id, int flags) { (void)id; (void)flags; return 0; }
s32 DecalFont_GetLineWidth(char *text, s16 font) { (void)font; return (int)strlen(text) * 8; }
void DecalFont_DrawLineOT(char *text, s16 x, s16 y, s16 font, s16 flags, u32 *ot)
{
	(void)y; (void)font; (void)ot;
	if (x == FULLSCREEN_MENU_LABEL_X)
	{
		s_drawingVibration = strcmp(text, "Vibration") == 0;
	}
	else if (x == FULLSCREEN_MENU_VALUE_X && s_drawingVibration)
	{
		s_vibrationColor = flags;
		snprintf(s_vibrationValue, sizeof(s_vibrationValue), "%s", text);
	}
}
void RECTMENU_DrawInnerRect(RECT *rect, int inset, u32 *ot) { (void)rect; (void)inset; (void)ot; }
void RECTMENU_DrawOuterRect_Edge(RECT *rect, const Color *color, u32 flags, u32 *ot)
{
	(void)rect; (void)color; (void)flags; (void)ot;
}
void RECTMENU_DrawFullRect(struct RectMenu *menu, RECT *rect) { (void)menu; (void)rect; }
void CTR_Box_DrawClearBox(const RECT *rect, const Color *color, s32 transparency, u32 *ot, struct PrimMem *prims)
{
	(void)rect; (void)color; (void)transparency; (void)ot; (void)prims;
}

static void Press(int buttons)
{
	for (int player = 0; player < PLATFORM_INPUT_PAD_COUNT; player++)
	{
		sdata->gGamepads->gamepad[player].buttonsTapped = player == 0 ? buttons : 0;
		sdata->gGamepads->gamepad[player].buttonsHeldCurrFrame = 0;
	}
	MM_MenuProc_Controls(&s_menu);
}

static void TestPlayers(int mode)
{
	s_player = 0; // No public reset of the persistent selected player.
	s_menu.rowSelected = 0;
	s_scroll.offset = 0;
	s_tracker.gameMode1 = mode | P2_VIBRATE | P4_VIBRATE;
	for (int player = 0; player < PLATFORM_INPUT_PAD_COUNT; player++)
	{
		s_devices[player] = player;
		s_rumble[player] = true;
		s_vibration[player] = player % 2 == 0;
	}
	for (int player = 0; player < PLATFORM_INPUT_PAD_COUNT; player++)
	{
		if (player != 0)
		{
			Press(BTN_R1);
		}
		Press(BTN_DOWN);
		CTR_TEST_CHECK(s_rows[s_menu.rowSelected].kind == CONTROLS_ROW_VIBRATION,
		               "Vibration is directly below Device for each selected player");
		const int before = s_tracker.gameMode1;
		const bool enabled = s_vibration[player];
		CTR_TEST_CHECK(strcmp(s_vibrationValue, enabled ? "ON" : "OFF") == 0,
		               "the displayed value follows the player's Controls preference");
		CTR_TEST_CHECK(s_vibrationColor == (JUSTIFY_RIGHT | WHITE), "rumble-capable devices can edit Vibration");
		Press(BTN_CROSS);
		for (int other = 0; other < PLATFORM_INPUT_PAD_COUNT; other++)
		{
			CTR_TEST_CHECK(s_vibration[other] == (other == player ? !enabled : other % 2 == 0),
			               "confirm changes only the selected player's Controls preference");
		}
		CTR_TEST_CHECK(s_tracker.gameMode1 == before, "vibration editing is independent of legacy game-mode bits");
		CTR_TEST_CHECK(strcmp(s_vibrationValue, enabled ? "OFF" : "ON") == 0,
		               "the displayed value updates immediately after toggling");
		Press(BTN_CROSS);
		CTR_TEST_CHECK(s_vibration[player] == enabled, "toggling back restores the initial Controls preference");
	}
}

static void TestUnavailableDevices(void)
{
	s_player = 0;
	s_menu.rowSelected = 1;
	const int unavailable[] = {NATIVE_CONTROLS_NO_DEVICE, TEST_KEYBOARD_DEVICE, 0};
	s_rumble[0] = false;
	for (int i = 0; i < (int)len(unavailable); i++)
	{
		s_devices[0] = unavailable[i];
		Press(0);
		const bool before = s_vibration[0];
		CTR_TEST_CHECK(s_rows[s_menu.rowSelected].kind == CONTROLS_ROW_VIBRATION,
		               "Vibration stays visible for missing, keyboard and non-rumble devices");
		CTR_TEST_CHECK(s_vibrationColor == (JUSTIFY_RIGHT | GRAY), "unavailable vibration values are gray");
		Press(BTN_CROSS);
		CTR_TEST_CHECK(s_vibration[0] == before, "unsupported devices cannot change the player's vibration preference");
	}
}

static void TestDeviceChanges(void)
{
	s_player = 0;
	s_menu.rowSelected = 0;
	for (int player = 0; player < PLATFORM_INPUT_PAD_COUNT; player++)
	{
		s_devices[player] = player == 0 ? 0 : NATIVE_CONTROLS_NO_DEVICE;
		s_rumble[player] = true;
	}
	s_vibration[0] = false;
	Press(BTN_CROSS);
	Press(BTN_DOWN);
	Press(BTN_CROSS);
	CTR_TEST_CHECK(s_preferenceChanges == 1 && s_devices[0] == 1, "the picker changes player 1 to another available device");
	Press(BTN_DOWN);
	CTR_TEST_CHECK(!s_vibration[0] && strcmp(s_vibrationValue, "OFF") == 0,
	               "changing devices retains the player's disabled vibration preference");
	for (int row = 0; row < s_rowCount; row++)
	{
		if (s_rows[row].kind == CONTROLS_ROW_RESTORE)
		{
			s_menu.rowSelected = (s16)row;
			break;
		}
	}
	Press(BTN_CROSS);
	CTR_TEST_CHECK(s_restores == 1 && !s_vibration[0],
	               "Restore Defaults resets device bindings without changing player vibration");
	s_menu.rowSelected = 1;
	s_devices[0] = TEST_KEYBOARD_DEVICE;
	Press(0);
	s_devices[0] = 1;
	Press(0);
	CTR_TEST_CHECK(s_vibrationColor == (JUSTIFY_RIGHT | WHITE) && strcmp(s_vibrationValue, "OFF") == 0,
	               "hotplug restores editability without resetting the vibration preference");
}

static void TestLegacyGameOptions(void)
{
	GAME_SAVE.options.gameMode1_vibrationFlags = GAME_MODE_VIBRATION_MASK;
	sdata->boolHasLoadedOptions = 0;
	s_vibration[0] = true;
	s_vibration[1] = false;
	RaceConfig_LoadGameOptions();
	RaceConfig_SaveGameOptions();
	CTR_TEST_CHECK(s_vibration[0] && !s_vibration[1],
	               "loading and saving memory-card options cannot overwrite Controls vibration preferences");
}

// The list must not scroll on the first steps: it only moves once the cursor leaves the
// visible window, so the highlight does not run away from the rows while browsing.
static void TestScroll(void)
{
	s_player = 0;
	s_menu.rowSelected = 0;
	s_scroll.offset = 0;
	for (int player = 0; player < PLATFORM_INPUT_PAD_COUNT; player++)
	{
		s_devices[player] = player == 0 ? 0 : NATIVE_CONTROLS_NO_DEVICE;
		s_rumble[player] = true;
	}
	Press(0);
	CTR_TEST_CHECK(s_scroll.offset == 0, "Controls opens with the list on the first row");
	Press(BTN_DOWN);
	CTR_TEST_CHECK(s_menu.rowSelected == 1 && s_scroll.offset == 0,
	               "the first step keeps the list still instead of scrolling it");
	for (int step = 1; step < FULLSCREEN_MENU_MAX_VISIBLE_ROWS - 1; step++)
	{
		Press(BTN_DOWN);
	}
	CTR_TEST_CHECK(s_menu.rowSelected == FULLSCREEN_MENU_MAX_VISIBLE_ROWS - 1 && s_scroll.offset == 0,
	               "the list stays put until the cursor reaches the last visible row");
	Press(BTN_DOWN);
	CTR_TEST_CHECK(s_menu.rowSelected == FULLSCREEN_MENU_MAX_VISIBLE_ROWS && s_scroll.offset == 1,
	               "one more step scrolls the list by a single row");

	s_menu.rowSelected = (s16)(s_rowCount - 1);
	Press(0);
	CTR_TEST_CHECK(s_scroll.offset == s_rowCount - FULLSCREEN_MENU_MAX_VISIBLE_ROWS,
	               "the last row clamps the scroll so the list stays full");
}

int main(void)
{
	(void)FullScreenMenu_RowActivate;
	(void)FullScreenMenu_RowStep;
	(void)FullScreenMenu_DrawRowValue;
	(void)FullScreenMenu_DrawScreen;
	(void)FullScreenMenu_NavScroll;
	(void)FullScreenMenu_SetShadedFrame;
	sdata->gGT = &s_tracker;
	sdata->gGamepads = &sdata->gamepadSystem;
	s_tracker.backBuffer = &s_buffer;
	CTR_TEST_SECTION("Per-player vibration in the main menu");
	TestPlayers(MAIN_MENU);
	CTR_TEST_SECTION("Per-player vibration while paused");
	TestPlayers(ARCADE_MODE | PAUSE_1);
	CTR_TEST_SECTION("Unavailable vibration devices");
	TestUnavailableDevices();
	CTR_TEST_SECTION("Device changes and restoring defaults");
	TestDeviceChanges();
	CTR_TEST_SECTION("Minimal list scrolling");
	TestScroll();
	TestLegacyGameOptions();
	return CtrTest_Summary();
}
