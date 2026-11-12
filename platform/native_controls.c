#include <platform/native_controls.h>

#include <macros.h>

#include "platform/native_assets.h"
#include "platform/native_log.h"
#include "platform/native_path.h"

#include <SDL3/SDL.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// A profile holds the bindings for one device identity. Profiles outlive the
// devices they belong to, so a controller that is unplugged keeps its layout and
// picks it back up when it returns.
#define NATIVE_CONTROLS_MAX_PROFILES 8
#define NATIVE_CONTROLS_KEY_LENGTH   40
#define NATIVE_CONTROLS_GUID_LENGTH  33

#define NATIVE_CONTROLS_SECTION_ASSIGNMENT "Assignment"
#define NATIVE_CONTROLS_SECTION_VIBRATION  "Vibration"
#define NATIVE_CONTROLS_SECTION_PREFIX    "device "

#define NATIVE_CONTROLS_KEY_NAME     "name"
#define NATIVE_CONTROLS_KEY_NONE     "none"
#define NATIVE_CONTROLS_KEY_DEADZONE "deadzone"
#define NATIVE_CONTROLS_VALUE_AUTO     "auto"
#define NATIVE_CONTROLS_VALUE_DISABLED "disabled"

// Frames to wait for a press before giving up. The game runs at roughly 60Hz, so 180
// is about three seconds: long enough to read the prompt and move a hand to the
// device, short enough that an abandoned rebind does not sit there.
#define NATIVE_CONTROLS_CAPTURE_TIMEOUT_FRAMES 180

enum NativeControlsCapturePhase
{
	NATIVE_CONTROLS_CAPTURE_IDLE = 0,

	// The control that opened the capture is still held; refuse to bind it.
	NATIVE_CONTROLS_CAPTURE_WAIT_RELEASE,
	NATIVE_CONTROLS_CAPTURE_ARMED,

	// A value was taken and is pending confirmation. Held in the bindings table as a
	// proposal, not committed, so leaving this state restores the previous binding.
	NATIVE_CONTROLS_CAPTURE_CONFIRM,

	// As above, and the newly bound control is still held. Needed so the press that
	// chose the key cannot double as the confirming press.
	NATIVE_CONTROLS_CAPTURE_CONFIRM_WAIT_RELEASE,
};

struct NativeControlsBindingInfo
{
	const char *key;
};

struct NativeControlsProfile
{
	bool used;
	int kind;
	char key[NATIVE_CONTROLS_KEY_LENGTH];
	char name[NATIVE_CONTROLS_NAME_LENGTH];
	s32 deadzone;
	s32 bindings[NATIVE_CONTROLS_BIND_COUNT];
};

struct NativeControlsDevice
{
	bool used;
	int kind;
	SDL_JoystickID instanceId;
	int profile;

	// Display name, disambiguated when several devices share a profile name.
	char name[NATIVE_CONTROLS_NAME_LENGTH];
	SDL_Gamepad *gamepad;
};

struct NativeControlsPlayer
{
	int kind;
	char deviceKey[NATIVE_CONTROLS_KEY_LENGTH];
	bool vibrationEnabled;
};

global_variable struct NativeControlsProfile s_profiles[NATIVE_CONTROLS_MAX_PROFILES];
global_variable struct NativeControlsDevice s_devices[NATIVE_CONTROLS_MAX_DEVICES];
global_variable int s_deviceCount;
global_variable struct NativeControlsPlayer s_playerPref[PLATFORM_INPUT_PAD_COUNT];
global_variable int s_playerDevice[PLATFORM_INPUT_PAD_COUNT];
global_variable int s_deviceOwner[NATIVE_CONTROLS_MAX_DEVICES];
global_variable u32 s_generation;

// True once NativeControls_Refresh has enumerated at least once. Not set by
// NativeControls_Init, because the first refresh has to enumerate unconditionally:
// with no gamepad attached the connected set matches the still-empty table, and
// skipping enumeration would leave a keyboard-only machine with no devices at all.
global_variable bool s_refreshed;

global_variable int s_capturePhase;
global_variable int s_captureDevice;
global_variable int s_captureBinding;
global_variable SDL_JoystickID s_captureInstanceId;
global_variable int s_captureFrames;
global_variable s32 s_capturePreviousValue;
global_variable s32 s_capturePendingValue;

// Defaults are carried over verbatim from the hardcoded mappings this module
// replaces, so an unconfigured install feels exactly as it did before.
local_persist const struct NativeControlsBindingInfo s_bindingInfo[NATIVE_CONTROLS_BIND_COUNT] = {
	[NATIVE_CONTROLS_BIND_CROSS] = {"cross"},
	[NATIVE_CONTROLS_BIND_CIRCLE] = {"circle"},
	[NATIVE_CONTROLS_BIND_TRIANGLE] = {"triangle"},
	[NATIVE_CONTROLS_BIND_SQUARE] = {"square"},
	[NATIVE_CONTROLS_BIND_L1] = {"l1"},
	[NATIVE_CONTROLS_BIND_L2] = {"l2"},
	[NATIVE_CONTROLS_BIND_L3] = {"l3"},
	[NATIVE_CONTROLS_BIND_R1] = {"r1"},
	[NATIVE_CONTROLS_BIND_R2] = {"r2"},
	[NATIVE_CONTROLS_BIND_R3] = {"r3"},
	[NATIVE_CONTROLS_BIND_DPAD_UP] = {"dpad_up"},
	[NATIVE_CONTROLS_BIND_DPAD_DOWN] = {"dpad_down"},
	[NATIVE_CONTROLS_BIND_DPAD_LEFT] = {"dpad_left"},
	[NATIVE_CONTROLS_BIND_DPAD_RIGHT] = {"dpad_right"},
	[NATIVE_CONTROLS_BIND_START] = {"start"},
	[NATIVE_CONTROLS_BIND_SELECT] = {"select"},
	[NATIVE_CONTROLS_BIND_AXIS_LEFT_X] = {"axis_left_x"},
	[NATIVE_CONTROLS_BIND_AXIS_LEFT_Y] = {"axis_left_y"},
	[NATIVE_CONTROLS_BIND_AXIS_RIGHT_X] = {"axis_right_x"},
	[NATIVE_CONTROLS_BIND_AXIS_RIGHT_Y] = {"axis_right_y"},
};

local_persist const s32 s_defaultKeyboard[NATIVE_CONTROLS_BIND_COUNT] = {
	[NATIVE_CONTROLS_BIND_CROSS] = SDL_SCANCODE_C,
	[NATIVE_CONTROLS_BIND_CIRCLE] = SDL_SCANCODE_V,
	[NATIVE_CONTROLS_BIND_TRIANGLE] = SDL_SCANCODE_Z,
	[NATIVE_CONTROLS_BIND_SQUARE] = SDL_SCANCODE_X,
	[NATIVE_CONTROLS_BIND_L1] = SDL_SCANCODE_LSHIFT,
	[NATIVE_CONTROLS_BIND_L2] = SDL_SCANCODE_LCTRL,
	[NATIVE_CONTROLS_BIND_L3] = SDL_SCANCODE_LEFTBRACKET,
	[NATIVE_CONTROLS_BIND_R1] = SDL_SCANCODE_RSHIFT,
	[NATIVE_CONTROLS_BIND_R2] = SDL_SCANCODE_RCTRL,
	[NATIVE_CONTROLS_BIND_R3] = SDL_SCANCODE_RIGHTBRACKET,
	[NATIVE_CONTROLS_BIND_DPAD_UP] = SDL_SCANCODE_UP,
	[NATIVE_CONTROLS_BIND_DPAD_DOWN] = SDL_SCANCODE_DOWN,
	[NATIVE_CONTROLS_BIND_DPAD_LEFT] = SDL_SCANCODE_LEFT,
	[NATIVE_CONTROLS_BIND_DPAD_RIGHT] = SDL_SCANCODE_RIGHT,
	[NATIVE_CONTROLS_BIND_START] = SDL_SCANCODE_RETURN,
	[NATIVE_CONTROLS_BIND_SELECT] = SDL_SCANCODE_SPACE,
	// The keyboard has no analog axes. Nothing reads these, but leaving them
	// pointing at an arbitrary scancode would be a lie the menu could report.
	[NATIVE_CONTROLS_BIND_AXIS_LEFT_X] = NATIVE_CONTROLS_BINDING_NONE,
	[NATIVE_CONTROLS_BIND_AXIS_LEFT_Y] = NATIVE_CONTROLS_BINDING_NONE,
	[NATIVE_CONTROLS_BIND_AXIS_RIGHT_X] = NATIVE_CONTROLS_BINDING_NONE,
	[NATIVE_CONTROLS_BIND_AXIS_RIGHT_Y] = NATIVE_CONTROLS_BINDING_NONE,
};

local_persist const s32 s_defaultGamepad[NATIVE_CONTROLS_BIND_COUNT] = {
	[NATIVE_CONTROLS_BIND_CROSS] = SDL_GAMEPAD_BUTTON_SOUTH,
	[NATIVE_CONTROLS_BIND_CIRCLE] = SDL_GAMEPAD_BUTTON_EAST,
	[NATIVE_CONTROLS_BIND_TRIANGLE] = SDL_GAMEPAD_BUTTON_NORTH,
	[NATIVE_CONTROLS_BIND_SQUARE] = SDL_GAMEPAD_BUTTON_WEST,
	[NATIVE_CONTROLS_BIND_L1] = SDL_GAMEPAD_BUTTON_LEFT_SHOULDER,
	[NATIVE_CONTROLS_BIND_L2] = SDL_GAMEPAD_AXIS_LEFT_TRIGGER | NATIVE_CONTROLS_BINDING_FLAG_AXIS,
	[NATIVE_CONTROLS_BIND_L3] = SDL_GAMEPAD_BUTTON_LEFT_STICK,
	[NATIVE_CONTROLS_BIND_R1] = SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER,
	[NATIVE_CONTROLS_BIND_R2] = SDL_GAMEPAD_AXIS_RIGHT_TRIGGER | NATIVE_CONTROLS_BINDING_FLAG_AXIS,
	[NATIVE_CONTROLS_BIND_R3] = SDL_GAMEPAD_BUTTON_RIGHT_STICK,
	[NATIVE_CONTROLS_BIND_DPAD_UP] = SDL_GAMEPAD_BUTTON_DPAD_UP,
	[NATIVE_CONTROLS_BIND_DPAD_DOWN] = SDL_GAMEPAD_BUTTON_DPAD_DOWN,
	[NATIVE_CONTROLS_BIND_DPAD_LEFT] = SDL_GAMEPAD_BUTTON_DPAD_LEFT,
	[NATIVE_CONTROLS_BIND_DPAD_RIGHT] = SDL_GAMEPAD_BUTTON_DPAD_RIGHT,
	[NATIVE_CONTROLS_BIND_START] = SDL_GAMEPAD_BUTTON_START,
	[NATIVE_CONTROLS_BIND_SELECT] = SDL_GAMEPAD_BUTTON_BACK,
	[NATIVE_CONTROLS_BIND_AXIS_LEFT_X] = SDL_GAMEPAD_AXIS_LEFTX | NATIVE_CONTROLS_BINDING_FLAG_AXIS,
	[NATIVE_CONTROLS_BIND_AXIS_LEFT_Y] = SDL_GAMEPAD_AXIS_LEFTY | NATIVE_CONTROLS_BINDING_FLAG_AXIS,
	[NATIVE_CONTROLS_BIND_AXIS_RIGHT_X] = SDL_GAMEPAD_AXIS_RIGHTX | NATIVE_CONTROLS_BINDING_FLAG_AXIS,
	[NATIVE_CONTROLS_BIND_AXIS_RIGHT_Y] = SDL_GAMEPAD_AXIS_RIGHTY | NATIVE_CONTROLS_BINDING_FLAG_AXIS,
};

internal const s32 *NativeControls_DefaultsForKind(int kind)
{
	if (kind == NATIVE_CONTROLS_DEVICE_KEYBOARD)
	{
		return s_defaultKeyboard;
	}

	return s_defaultGamepad;
}

internal bool NativeControls_IsValidPlayer(int player)
{
	return (player >= 0) && (player < PLATFORM_INPUT_PAD_COUNT);
}

internal bool NativeControls_IsValidDevice(int device)
{
	return (device >= 0) && (device < s_deviceCount) && s_devices[device].used;
}

internal bool NativeControls_IsValidBinding(int binding)
{
	return (binding >= 0) && (binding < NATIVE_CONTROLS_BIND_COUNT);
}

// Shared by the file loader and the setter so a hand-edited value and a menu edit cannot
// disagree about what counts as a legal figure.
internal s32 NativeControls_ClampDeadzone(s32 percent)
{
	if (percent < NATIVE_CONTROLS_DEADZONE_MIN)
	{
		return NATIVE_CONTROLS_DEADZONE_MIN;
	}

	if (percent > NATIVE_CONTROLS_DEADZONE_MAX)
	{
		return NATIVE_CONTROLS_DEADZONE_MAX;
	}

	return percent;
}

internal bool NativeControls_StrEqual(const char *a, const char *b)
{
	return (a != NULL) && (b != NULL) && (strcmp(a, b) == 0);
}

// ---------------------------------------------------------------------------
// Profiles
// ---------------------------------------------------------------------------

internal int NativeControls_FindProfileByKey(int kind, const char *key)
{
	if ((key == NULL) || (key[0] == '\0'))
	{
		return -1;
	}

	for (int i = 0; i < NATIVE_CONTROLS_MAX_PROFILES; i++)
	{
		if (s_profiles[i].used && (s_profiles[i].kind == kind) && NativeControls_StrEqual(s_profiles[i].key, key))
		{
			return i;
		}
	}

	return -1;
}

internal int NativeControls_CreateProfile(int kind, const char *key, const char *name)
{
	for (int i = 0; i < NATIVE_CONTROLS_MAX_PROFILES; i++)
	{
		if (s_profiles[i].used)
		{
			continue;
		}

		struct NativeControlsProfile *profile = &s_profiles[i];
		profile->used = true;
		profile->kind = kind;
		snprintf(profile->key, sizeof(profile->key), "%s", key);
		snprintf(profile->name, sizeof(profile->name), "%s", name);
		profile->deadzone = NATIVE_CONTROLS_DEADZONE_DEFAULT;
		memcpy(profile->bindings, NativeControls_DefaultsForKind(kind), sizeof(profile->bindings));
		return i;
	}

	Platform_LogWarn("[Controls] No free profile slot for '%s', using defaults\n", key);
	return -1;
}

// Returns the profile index for a device identity, creating it from the defaults
// the first time that identity is seen.
internal int NativeControls_ProfileForDevice(int kind, const char *key, const char *name)
{
	int profile = NativeControls_FindProfileByKey(kind, key);

	if (profile >= 0)
	{
		// Keep a display name even when the file carried none.
		if (s_profiles[profile].name[0] == '\0')
		{
			snprintf(s_profiles[profile].name, sizeof(s_profiles[profile].name), "%s", name);
		}

		return profile;
	}

	return NativeControls_CreateProfile(kind, key, name);
}

internal s32 *NativeControls_BindingsOfDevice(int device)
{
	if (!NativeControls_IsValidDevice(device))
	{
		return NULL;
	}

	int profile = s_devices[device].profile;

	if (profile < 0)
	{
		return NULL;
	}

	return s_profiles[profile].bindings;
}

// ---------------------------------------------------------------------------
// Binding value text
// ---------------------------------------------------------------------------

internal bool NativeControls_DecodeGamepadBinding(const char *text, s32 *dst)
{
	char buffer[64];
	snprintf(buffer, sizeof(buffer), "%s", text);

	char *name = buffer;
	while ((*name == ' ') || (*name == '\t'))
	{
		name++;
	}

	// A button name and an axis name never collide in SDL's tables, so trying the
	// button table first is unambiguous.
	SDL_GamepadButton button = SDL_GetGamepadButtonFromString(name);
	if (button != SDL_GAMEPAD_BUTTON_INVALID)
	{
		*dst = (s32)button;
		return true;
	}

	SDL_GamepadAxis axis = SDL_GetGamepadAxisFromString(name);
	if (axis != SDL_GAMEPAD_AXIS_INVALID)
	{
		*dst = (s32)axis | NATIVE_CONTROLS_BINDING_FLAG_AXIS;
		return true;
	}

	return false;
}

// SDL names a printable key by the character it types, and the retail font cannot show
// most of those characters: four are repurposed as button artwork and the rest have no
// glyph at all. Spelling these out keeps the menu honest, where the bare character would
// read as the wrong icon or as nothing.
local_persist const struct NativeControlsScancodeName
{
	SDL_Scancode scancode;
	const char *name;
} s_scancodeNames[] = {
	{SDL_SCANCODE_LEFTBRACKET, "Left Bracket"},
	{SDL_SCANCODE_RIGHTBRACKET, "Right Bracket"},
	{SDL_SCANCODE_BACKSLASH, "Backslash"},
	{SDL_SCANCODE_NONUSHASH, "Hash"},
	{SDL_SCANCODE_SEMICOLON, "Semicolon"},
	{SDL_SCANCODE_GRAVE, "Grave"},
};

internal const char *NativeControls_ScancodeName(SDL_Scancode scancode)
{
	for (unsigned int i = 0; i < SDL_arraysize(s_scancodeNames); i++)
	{
		if (s_scancodeNames[i].scancode == scancode)
		{
			return s_scancodeNames[i].name;
		}
	}

	return SDL_GetScancodeName(scancode);
}

// The two ways a keyboard binding has to be spelled: what goes into controls.ini, and
// what goes on screen. The file keeps SDL's own key names, several of which the retail
// font cannot draw, so the display form spells those out.
enum NativeControlsNameStyle
{
	NATIVE_CONTROLS_NAME_FILE = 0,
	NATIVE_CONTROLS_NAME_DISPLAY,
};

// The axis flag is not part of the control index, so it comes off before the value is
// used as one.
internal void NativeControls_BindingToText(int kind, s32 value, char *dst, size_t dstSize,
                                            enum NativeControlsNameStyle style)
{
	if (kind == NATIVE_CONTROLS_DEVICE_KEYBOARD)
	{
		if (value < 0)
		{
			snprintf(dst, dstSize, "%s", NATIVE_CONTROLS_KEY_NONE);
			return;
		}

		const SDL_Scancode scancode = (SDL_Scancode)value;
		const char *name = (style == NATIVE_CONTROLS_NAME_DISPLAY) ? NativeControls_ScancodeName(scancode)
		                                                          : SDL_GetScancodeName(scancode);

		snprintf(dst, dstSize, "%s", (name != NULL) ? name : NATIVE_CONTROLS_KEY_NONE);
		return;
	}

	const s32 index = value & ~NATIVE_CONTROLS_BINDING_FLAG_MASK;
	const char *name = ((value & NATIVE_CONTROLS_BINDING_FLAG_AXIS) != 0)
	                       ? SDL_GetGamepadStringForAxis((SDL_GamepadAxis)index)
	                       : SDL_GetGamepadStringForButton((SDL_GamepadButton)index);

	if ((name == NULL) || (name[0] == '\0'))
	{
		snprintf(dst, dstSize, "%s", NATIVE_CONTROLS_KEY_NONE);
		return;
	}

	snprintf(dst, dstSize, "%s", name);
}

// ---------------------------------------------------------------------------
// File
// ---------------------------------------------------------------------------

internal bool NativeControls_FilePath(char *dst, size_t dstSize)
{
	return NativePath_Join(dst, dstSize, NativeStr8_FromCString(NativeAssets_GetBaseDir()),
	                       NativeStr8_FromCString(NATIVE_CONTROLS_FILE_NAME)) != 0;
}

internal char *NativeControls_Trim(char *text)
{
	while ((*text == ' ') || (*text == '\t') || (*text == '\r') || (*text == '\n'))
	{
		text++;
	}

	char *end = text + strlen(text);
	while (end > text)
	{
		char c = end[-1];
		if ((c != ' ') && (c != '\t') && (c != '\r') && (c != '\n'))
		{
			break;
		}
		end--;
	}
	*end = '\0';

	return text;
}

internal int NativeControls_PlayerForKey(const char *key)
{
	static const char prefix[] = "player";

	const size_t prefixLength = sizeof(prefix) - 1;
	if (strncmp(key, prefix, prefixLength) != 0)
	{
		return -1;
	}

	// One-based in the file, zero-based in the table.
	char *end = NULL;
	const char *number = key + prefixLength;
	const long player = strtol(number, &end, 10);
	if (end == number || *end != '\0' || player < 1 || player > PLATFORM_INPUT_PAD_COUNT)
	{
		return -1;
	}
	return (int)player - 1;
}

internal void NativeControls_ApplyAssignmentKey(char *key, char *value)
{
	const int player = NativeControls_PlayerForKey(key);
	if (!NativeControls_IsValidPlayer(player))
	{
		return;
	}
	struct NativeControlsPlayer *pref = &s_playerPref[player];

	if (NativeControls_StrEqual(value, NATIVE_CONTROLS_VALUE_DISABLED))
	{
		// Player 1 can never be disabled, so a file asking for it is answered
		// with auto rather than obeyed.
		pref->kind = (player == 0) ? NATIVE_CONTROLS_PREF_AUTO : NATIVE_CONTROLS_PREF_DISABLED;
		pref->deviceKey[0] = '\0';
		return;
	}

	if (NativeControls_StrEqual(value, NATIVE_CONTROLS_VALUE_AUTO))
	{
		pref->kind = NATIVE_CONTROLS_PREF_AUTO;
		pref->deviceKey[0] = '\0';
		return;
	}

	pref->kind = NATIVE_CONTROLS_PREF_PINNED;
	snprintf(pref->deviceKey, sizeof(pref->deviceKey), "%s", value);
}

internal void NativeControls_ApplyVibrationKey(const char *key, const char *value)
{
	const int player = NativeControls_PlayerForKey(key);
	if (!NativeControls_IsValidPlayer(player))
	{
		return;
	}
	if (NativeControls_StrEqual(value, "true") || NativeControls_StrEqual(value, "1"))
	{
		s_playerPref[player].vibrationEnabled = true;
	}
	else if (NativeControls_StrEqual(value, "false") || NativeControls_StrEqual(value, "0"))
	{
		s_playerPref[player].vibrationEnabled = false;
	}
}

// Applies one key from a device section. A key or value this build does not
// recognise is skipped and the binding keeps whatever it already had, so a
// hand-edited file can be wrong in one line without corrupting the rest.
internal void NativeControls_ApplyDeviceKey(int profile, char *key, char *value)
{
	if (NativeControls_StrEqual(key, NATIVE_CONTROLS_KEY_NAME))
	{
		snprintf(s_profiles[profile].name, sizeof(s_profiles[profile].name), "%s", value);
		return;
	}

	if (NativeControls_StrEqual(key, NATIVE_CONTROLS_KEY_DEADZONE))
	{
		// An unparseable figure leaves the default rather than silently trimming input.
		char *end = NULL;
		const long percent = strtol(value, &end, 10);

		if ((end == value) || (*end != '\0'))
		{
			return;
		}

		s_profiles[profile].deadzone = NativeControls_ClampDeadzone((s32)percent);
		return;
	}

	for (int binding = 0; binding < NATIVE_CONTROLS_BIND_COUNT; binding++)
	{
		if (!NativeControls_StrEqual(key, s_bindingInfo[binding].key))
		{
			continue;
		}

		s32 decoded;

		if (NativeControls_StrEqual(value, NATIVE_CONTROLS_KEY_NONE))
		{
			decoded = NATIVE_CONTROLS_BINDING_NONE;
		}
		else if (s_profiles[profile].kind == NATIVE_CONTROLS_DEVICE_KEYBOARD)
		{
			SDL_Scancode scancode = SDL_GetScancodeFromName(value);

			if (scancode == SDL_SCANCODE_UNKNOWN)
			{
				return;
			}

			decoded = (s32)scancode;
		}
		else if (!NativeControls_DecodeGamepadBinding(value, &decoded))
		{
			return;
		}

		s_profiles[profile].bindings[binding] = decoded;
		return;
	}
}

// Sections are either the assignment table or one device. Returns the profile
// index a following key applies to, or -1 for the assignment table.
internal int NativeControls_ProfileForSection(const char *section)
{
	const char *key = section;

	if (strncmp(key, NATIVE_CONTROLS_SECTION_PREFIX, strlen(NATIVE_CONTROLS_SECTION_PREFIX)) == 0)
	{
		key += strlen(NATIVE_CONTROLS_SECTION_PREFIX);
	}

	if (NativeControls_StrEqual(key, NATIVE_CONTROLS_KEYBOARD_KEY))
	{
		return NativeControls_ProfileForDevice(NATIVE_CONTROLS_DEVICE_KEYBOARD, NATIVE_CONTROLS_KEYBOARD_KEY, "Keyboard");
	}

	return NativeControls_ProfileForDevice(NATIVE_CONTROLS_DEVICE_GAMEPAD, key, key);
}

internal void NativeControls_Load(void)
{
	char path[512];

	if (!NativeControls_FilePath(path, sizeof(path)))
	{
		Platform_LogWarn("[Controls] Could not build a path for %s\n", NATIVE_CONTROLS_FILE_NAME);
		return;
	}

	FILE *file = fopen(path, "r");
	if (file == NULL)
	{
		// Missing is the normal case on a first run, not a fault.
		Platform_Log("[Controls] %s not found (%s), using defaults\n", NATIVE_CONTROLS_FILE_NAME, path);
		return;
	}

	// -1 means the assignment table, which no profile owns.
	int profile = -1;
	bool inAssignment = false;
	bool inVibration = false;
	char section[96] = "";
	char line[512];

	while (fgets(line, sizeof(line), file) != NULL)
	{
		char *cursor = NativeControls_Trim(line);

		if ((*cursor == '\0') || (*cursor == ';') || (*cursor == '#'))
		{
			continue;
		}

		if (*cursor == '[')
		{
			char *end = strchr(cursor + 1, ']');
			if (end == NULL)
			{
				continue;
			}

			*end = '\0';
			snprintf(section, sizeof(section), "%s", cursor + 1);
			inAssignment = NativeControls_StrEqual(section, NATIVE_CONTROLS_SECTION_ASSIGNMENT);
			inVibration = NativeControls_StrEqual(section, NATIVE_CONTROLS_SECTION_VIBRATION);
			profile = inAssignment || inVibration ? -1 : NativeControls_ProfileForSection(section);
			continue;
		}

		char *equals = strchr(cursor, '=');
		if (equals == NULL)
		{
			continue;
		}

		*equals = '\0';
		char *key = NativeControls_Trim(cursor);
		char *value = NativeControls_Trim(equals + 1);

		if (section[0] == '\0')
		{
			continue;
		}

		if (inAssignment)
		{
			NativeControls_ApplyAssignmentKey(key, value);
		}
		else if (inVibration)
		{
			NativeControls_ApplyVibrationKey(key, value);
		}
		else if (profile >= 0)
		{
			NativeControls_ApplyDeviceKey(profile, key, value);
		}
	}

	fclose(file);
}

void NativeControls_Save(void)
{
	char path[512];

	if (!NativeControls_FilePath(path, sizeof(path)))
	{
		Platform_LogWarn("[Controls] Could not build a path for %s\n", NATIVE_CONTROLS_FILE_NAME);
		return;
	}

	FILE *file = fopen(path, "w");
	if (file == NULL)
	{
		Platform_LogWarn("[Controls] Could not write %s\n", path);
		return;
	}

	fprintf(file, "[%s]\n", NATIVE_CONTROLS_SECTION_ASSIGNMENT);
	for (int player = 0; player < PLATFORM_INPUT_PAD_COUNT; player++)
	{
		const struct NativeControlsPlayer *pref = &s_playerPref[player];
		const char *value = NATIVE_CONTROLS_VALUE_AUTO;

		if (pref->kind == NATIVE_CONTROLS_PREF_DISABLED)
		{
			value = NATIVE_CONTROLS_VALUE_DISABLED;
		}
		else if (pref->kind == NATIVE_CONTROLS_PREF_PINNED)
		{
			value = pref->deviceKey;
		}

		fprintf(file, "player%d = %s\n", player + 1, value);
	}

	fprintf(file, "\n[%s]\n", NATIVE_CONTROLS_SECTION_VIBRATION);
	for (int player = 0; player < PLATFORM_INPUT_PAD_COUNT; player++)
	{
		fprintf(file, "player%d = %s\n", player + 1, s_playerPref[player].vibrationEnabled ? "true" : "false");
	}

	char value[64];
	for (int i = 0; i < NATIVE_CONTROLS_MAX_PROFILES; i++)
	{
		if (!s_profiles[i].used)
		{
			continue;
		}

		fprintf(file, "\n[%s%s]\n", NATIVE_CONTROLS_SECTION_PREFIX, s_profiles[i].key);
		fprintf(file, "%s = %s\n", NATIVE_CONTROLS_KEY_NAME, s_profiles[i].name);
		fprintf(file, "%s = %d\n", NATIVE_CONTROLS_KEY_DEADZONE, s_profiles[i].deadzone);

		for (int binding = 0; binding < NATIVE_CONTROLS_BIND_COUNT; binding++)
		{
			NativeControls_BindingToText(s_profiles[i].kind, s_profiles[i].bindings[binding], value, sizeof(value),
			                             NATIVE_CONTROLS_NAME_FILE);
			fprintf(file, "%s = %s\n", s_bindingInfo[binding].key, value);
		}
	}

	fclose(file);
}

// ---------------------------------------------------------------------------
// Device enumeration
// ---------------------------------------------------------------------------

internal void NativeControls_CloseAllDevices(void)
{
	for (int i = 0; i < NATIVE_CONTROLS_MAX_DEVICES; i++)
	{
		if (s_devices[i].used && (s_devices[i].gamepad != NULL))
		{
			SDL_CloseGamepad(s_devices[i].gamepad);
		}
	}

	memset(s_devices, 0, sizeof(s_devices));
	s_deviceCount = 0;
}

// Identical controllers report identical names, so number the duplicates. Without this the
// menu would list "Xbox Controller" twice with no way to tell which is which.
//
// Compared against a snapshot rather than the table, because a name is rewritten as soon
// as its own count is known: reading the table back would stop matching the pads still
// waiting to be numbered, leaving all but the first of a set unnumbered.
internal void NativeControls_NumberDuplicateGamepadNames(void)
{
	char names[NATIVE_CONTROLS_MAX_DEVICES][NATIVE_CONTROLS_NAME_LENGTH];

	for (int i = 0; i < s_deviceCount; i++)
	{
		snprintf(names[i], sizeof(names[i]), "%s", s_devices[i].name);
	}

	for (int i = 0; i < s_deviceCount; i++)
	{
		if (s_devices[i].kind != NATIVE_CONTROLS_DEVICE_GAMEPAD)
		{
			continue;
		}

		int total = 0;
		int ordinal = 0;

		for (int other = 0; other < s_deviceCount; other++)
		{
			if ((s_devices[other].kind != NATIVE_CONTROLS_DEVICE_GAMEPAD) || (strcmp(names[other], names[i]) != 0))
			{
				continue;
			}

			total++;

			if (other <= i)
			{
				ordinal++;
			}
		}

		if (total < 2)
		{
			continue;
		}

		char disambiguated[NATIVE_CONTROLS_NAME_LENGTH];
		snprintf(disambiguated, sizeof(disambiguated), "%.*s %d", (int)(sizeof(disambiguated) - 12), names[i], ordinal);
		snprintf(s_devices[i].name, sizeof(s_devices[i].name), "%s", disambiguated);
	}
}

internal void NativeControls_EnumerateDevices(void)
{
	NativeControls_CloseAllDevices();

	// SDL_OpenGamepad hands back the same wrapper for an already-open joystick, so
	// opening here does not duplicate the handle the input layer reads through.
	const int maxGamepads = NATIVE_CONTROLS_MAX_DEVICES - 1;
	int count = 0;
	SDL_JoystickID *gamepads = SDL_GetGamepads(&count);

	for (int i = 0; (i < count) && (s_deviceCount < maxGamepads); i++)
	{
		char guid[NATIVE_CONTROLS_GUID_LENGTH];
		SDL_GUIDToString(SDL_GetGamepadGUIDForID(gamepads[i]), guid, (int)sizeof(guid));

		const char *name = SDL_GetGamepadNameForID(gamepads[i]);
		if ((name == NULL) || (name[0] == '\0'))
		{
			name = guid;
		}

		struct NativeControlsDevice *device = &s_devices[s_deviceCount];
		device->used = true;
		device->kind = NATIVE_CONTROLS_DEVICE_GAMEPAD;
		device->instanceId = gamepads[i];
		device->profile = NativeControls_ProfileForDevice(NATIVE_CONTROLS_DEVICE_GAMEPAD, guid, name);
		snprintf(device->name, sizeof(device->name), "%s", name);
		device->gamepad = SDL_OpenGamepad(gamepads[i]);
		s_deviceCount++;
	}

	if (gamepads != NULL)
	{
		SDL_free(gamepads);
	}

	// The keyboard is always present and always last, so player 1 only receives it
	// once no gamepad is left.
	struct NativeControlsDevice *keyboard = &s_devices[s_deviceCount];
	keyboard->used = true;
	keyboard->kind = NATIVE_CONTROLS_DEVICE_KEYBOARD;
	keyboard->instanceId = -1;
	keyboard->profile =
	    NativeControls_ProfileForDevice(NATIVE_CONTROLS_DEVICE_KEYBOARD, NATIVE_CONTROLS_KEYBOARD_KEY, "Keyboard");
	snprintf(keyboard->name, sizeof(keyboard->name), "%s", "Keyboard");
	keyboard->gamepad = NULL;
	s_deviceCount++;

	NativeControls_NumberDuplicateGamepadNames();
}

// True when the connected gamepads are exactly the ones already in the table, in
// the same order. SDL reports gamepads in enumeration order, so this detects any
// add, remove or reorder.
internal bool NativeControls_DeviceSetUnchanged(void)
{
	int count = 0;
	SDL_JoystickID *gamepads = SDL_GetGamepads(&count);

	int haveGamepads = 0;
	for (int i = 0; i < s_deviceCount; i++)
	{
		if (s_devices[i].used && (s_devices[i].kind == NATIVE_CONTROLS_DEVICE_GAMEPAD))
		{
			haveGamepads++;
		}
	}

	bool unchanged = (haveGamepads == count);
	for (int i = 0; unchanged && (i < count); i++)
	{
		unchanged = (s_devices[i].used && (s_devices[i].kind == NATIVE_CONTROLS_DEVICE_GAMEPAD) &&
		             (s_devices[i].instanceId == gamepads[i]));
	}

	if (gamepads != NULL)
	{
		SDL_free(gamepads);
	}

	return unchanged;
}

internal int NativeControls_FindDeviceByKey(const char *key)
{
	if ((key == NULL) || (key[0] == '\0'))
	{
		return NATIVE_CONTROLS_NO_DEVICE;
	}

	for (int i = 0; i < s_deviceCount; i++)
	{
		int profile = s_devices[i].profile;

		if (s_devices[i].used && (profile >= 0) && NativeControls_StrEqual(s_profiles[profile].key, key))
		{
			return i;
		}
	}

	return NATIVE_CONTROLS_NO_DEVICE;
}

internal int NativeControls_FindKeyboardDevice(void)
{
	for (int i = 0; i < s_deviceCount; i++)
	{
		if (s_devices[i].used && (s_devices[i].kind == NATIVE_CONTROLS_DEVICE_KEYBOARD))
		{
			return i;
		}
	}

	return NATIVE_CONTROLS_NO_DEVICE;
}

// ---------------------------------------------------------------------------
// Assignment
// ---------------------------------------------------------------------------

internal void NativeControls_ResolveAssignment(void)
{
	int free[NATIVE_CONTROLS_MAX_DEVICES];
	int freeCount = 0;

	for (int i = 0; i < NATIVE_CONTROLS_MAX_DEVICES; i++)
	{
		s_deviceOwner[i] = NATIVE_CONTROLS_NO_DEVICE;
	}

	for (int i = 0; i < PLATFORM_INPUT_PAD_COUNT; i++)
	{
		s_playerDevice[i] = NATIVE_CONTROLS_NO_DEVICE;
	}

	// Pinned preferences first and in player order, so two players pinning the
	// same device resolve in favour of the lower one.
	for (int player = 0; player < PLATFORM_INPUT_PAD_COUNT; player++)
	{
		if (s_playerPref[player].kind != NATIVE_CONTROLS_PREF_PINNED)
		{
			continue;
		}

		int device = NativeControls_FindDeviceByKey(s_playerPref[player].deviceKey);
		if ((device == NATIVE_CONTROLS_NO_DEVICE) || (s_deviceOwner[device] != NATIVE_CONTROLS_NO_DEVICE))
		{
			// Gone, or already owned by a lower player. The preference is kept so
			// the device is reclaimed if it comes back.
			continue;
		}

		s_deviceOwner[device] = player;
		s_playerDevice[player] = device;
	}

	// Table order is gamepads first and the keyboard last.
	for (int device = 0; device < s_deviceCount; device++)
	{
		if (s_deviceOwner[device] == NATIVE_CONTROLS_NO_DEVICE)
		{
			free[freeCount++] = device;
		}
	}

	int next = 0;
	for (int player = 0; player < PLATFORM_INPUT_PAD_COUNT; player++)
	{
		if (s_playerPref[player].kind != NATIVE_CONTROLS_PREF_AUTO)
		{
			continue;
		}

		if (next >= freeCount)
		{
			break;
		}

		int device = free[next++];
		s_deviceOwner[device] = player;
		s_playerDevice[player] = device;
	}

	// Player 1 is never "disabled", so it should always have been filled above.
	// Reaching here means every device was claimed by someone else, which happens
	// when a higher player pinned the keyboard. Take it back rather than leave the
	// session without a way in.
	if (s_playerDevice[0] == NATIVE_CONTROLS_NO_DEVICE)
	{
		int keyboard = NativeControls_FindKeyboardDevice();

		if (keyboard != NATIVE_CONTROLS_NO_DEVICE)
		{
			int previousOwner = s_deviceOwner[keyboard];

			s_deviceOwner[keyboard] = 0;
			s_playerDevice[0] = keyboard;

			if (previousOwner > 0)
			{
				// Release the keyboard, or the old owner would keep reading it and
				// two players would share one device.
				s_playerDevice[previousOwner] = NATIVE_CONTROLS_NO_DEVICE;

				// Do not leave a stale pin behind that would steal it right back.
				s_playerPref[previousOwner].kind = NATIVE_CONTROLS_PREF_AUTO;
				s_playerPref[previousOwner].deviceKey[0] = '\0';
			}
		}
	}
}

// Re-resolves the assignment and publishes the result.
//
// Every path that can change which device a player holds must go through here. The
// input layer keys its player-to-pad-slot projection off the generation counter, so
// resolving without publishing leaves it serving the previous assignment forever and
// the change appears to do nothing.
internal void NativeControls_ResolveAndPublish(void)
{
	int previousPlayerDevice[PLATFORM_INPUT_PAD_COUNT];

	memcpy(previousPlayerDevice, s_playerDevice, sizeof(previousPlayerDevice));

	NativeControls_ResolveAssignment();

	if (memcmp(previousPlayerDevice, s_playerDevice, sizeof(previousPlayerDevice)) != 0)
	{
		s_generation++;
	}
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void NativeControls_Init(void)
{
	memset(s_profiles, 0, sizeof(s_profiles));
	memset(s_devices, 0, sizeof(s_devices));
	memset(s_deviceOwner, 0, sizeof(s_deviceOwner));
	s_deviceCount = 0;

	for (int player = 0; player < PLATFORM_INPUT_PAD_COUNT; player++)
	{
		s_playerPref[player].kind = NATIVE_CONTROLS_PREF_AUTO;
		s_playerPref[player].deviceKey[0] = '\0';
		s_playerPref[player].vibrationEnabled = true;
		s_playerDevice[player] = NATIVE_CONTROLS_NO_DEVICE;
	}

	s_generation = 0;
	s_capturePhase = NATIVE_CONTROLS_CAPTURE_IDLE;
	s_captureDevice = NATIVE_CONTROLS_NO_DEVICE;
	s_captureBinding = -1;
	s_captureInstanceId = -1;
	s_captureFrames = 0;
	s_capturePreviousValue = NATIVE_CONTROLS_BINDING_NONE;
	s_refreshed = false;

	NativeControls_Load();
}

void NativeControls_Refresh(void)
{
	// Handles are only torn down when the connected set actually differs, so a
	// spurious event cannot interrupt a rumble or a capture in progress.
	const bool devicesChanged = !s_refreshed || !NativeControls_DeviceSetUnchanged();

	if (devicesChanged)
	{
		NativeControls_EnumerateDevices();
	}

	NativeControls_ResolveAndPublish();

	// The device table changed even when no player did, so the input layer has to
	// re-bind pad slots to rebuild the bus from the new table.
	if (devicesChanged)
	{
		s_generation++;
	}

	s_refreshed = true;
}

u32 NativeControls_GetGeneration(void)
{
	return s_generation;
}

int NativeControls_GetPlayerDevice(int player)
{
	if (!NativeControls_IsValidPlayer(player))
	{
		return NATIVE_CONTROLS_NO_DEVICE;
	}

	return s_playerDevice[player];
}

int NativeControls_GetPlayerPref(int player)
{
	if (!NativeControls_IsValidPlayer(player))
	{
		return NATIVE_CONTROLS_PREF_AUTO;
	}

	return s_playerPref[player].kind;
}

int NativeControls_SetPlayerPref(int player, int pref, int device)
{
	if (!NativeControls_IsValidPlayer(player))
	{
		return 0;
	}

	if ((player == 0) && (pref == NATIVE_CONTROLS_PREF_DISABLED))
	{
		return 0;
	}

	struct NativeControlsPlayer *target = &s_playerPref[player];

	if (pref == NATIVE_CONTROLS_PREF_PINNED)
	{
		if (!NativeControls_IsValidDevice(device))
		{
			return 0;
		}

		int profile = s_devices[device].profile;
		if (profile < 0)
		{
			return 0;
		}

		target->kind = NATIVE_CONTROLS_PREF_PINNED;
		snprintf(target->deviceKey, sizeof(target->deviceKey), "%s", s_profiles[profile].key);
	}
	else
	{
		target->kind = pref;
		target->deviceKey[0] = '\0';
	}

	NativeControls_ResolveAndPublish();
	NativeControls_Save();
	return 1;
}

int NativeControls_GetAvailableDevices(int player, int *dst, int max)
{
	if ((dst == NULL) || (max <= 0) || !NativeControls_IsValidPlayer(player))
	{
		return 0;
	}

	int count = 0;
	for (int device = 0; (device < s_deviceCount) && (count < max); device++)
	{
		if (!s_devices[device].used)
		{
			continue;
		}

		int owner = s_deviceOwner[device];
		if ((owner != NATIVE_CONTROLS_NO_DEVICE) && (owner != player))
		{
			// Claimed by someone else; not offered at all.
			continue;
		}

		dst[count++] = device;
	}

	return count;
}

int NativeControls_GetDeviceKind(int device)
{
	if (!NativeControls_IsValidDevice(device))
	{
		return NATIVE_CONTROLS_DEVICE_NONE;
	}

	return s_devices[device].kind;
}

const char *NativeControls_GetDeviceName(int device)
{
	if (!NativeControls_IsValidDevice(device))
	{
		return "";
	}

	return s_devices[device].name;
}

void *NativeControls_GetGamepadHandle(int device)
{
	if (!NativeControls_IsValidDevice(device))
	{
		return NULL;
	}

	return s_devices[device].gamepad;
}

bool NativeControls_CanVibrate(int device)
{
	SDL_Gamepad *gamepad = NativeControls_GetGamepadHandle(device);
	return gamepad != NULL && SDL_GetBooleanProperty(SDL_GetGamepadProperties(gamepad),
	                                                SDL_PROP_GAMEPAD_CAP_RUMBLE_BOOLEAN, false);
}

bool NativeControls_GetPlayerVibration(int player)
{
	return NativeControls_IsValidPlayer(player) && s_playerPref[player].vibrationEnabled;
}

void NativeControls_SetPlayerVibration(int player, bool enabled)
{
	if (!NativeControls_IsValidPlayer(player) || s_playerPref[player].vibrationEnabled == enabled)
	{
		return;
	}
	s_playerPref[player].vibrationEnabled = enabled;
	NativeControls_Save();
}

s32 NativeControls_GetBinding(int device, int binding)
{
	const s32 *bindings = NativeControls_BindingsOfDevice(device);

	if ((bindings == NULL) || !NativeControls_IsValidBinding(binding))
	{
		return NATIVE_CONTROLS_BINDING_NONE;
	}

	return bindings[binding];
}

void NativeControls_GetBindingLabel(int device, int binding, char *dst, size_t dstSize)
{
	if ((dst == NULL) || (dstSize == 0))
	{
		return;
	}

	dst[0] = '\0';

	if (!NativeControls_IsValidDevice(device) || !NativeControls_IsValidBinding(binding))
	{
		return;
	}

	NativeControls_BindingToText(s_devices[device].kind, NativeControls_GetBinding(device, binding), dst, dstSize,
	                             NATIVE_CONTROLS_NAME_DISPLAY);
}

void NativeControls_SetBinding(int device, int binding, s32 value)
{
	s32 *bindings = NativeControls_BindingsOfDevice(device);

	if ((bindings == NULL) || !NativeControls_IsValidBinding(binding))
	{
		return;
	}

	bindings[binding] = value;
	NativeControls_Save();
}

void NativeControls_RestoreDeviceDefaults(int device)
{
	s32 *bindings = NativeControls_BindingsOfDevice(device);

	if (!NativeControls_IsValidDevice(device) || (bindings == NULL))
	{
		return;
	}

	memcpy(bindings, NativeControls_DefaultsForKind(s_devices[device].kind), sizeof(s_profiles[0].bindings));
	s_profiles[s_devices[device].profile].deadzone = NATIVE_CONTROLS_DEADZONE_DEFAULT;
	NativeControls_Save();
}

s32 NativeControls_GetDeadzone(int device)
{
	if (!NativeControls_IsValidDevice(device))
	{
		return NATIVE_CONTROLS_DEADZONE_DEFAULT;
	}

	int profile = s_devices[device].profile;
	if (profile < 0)
	{
		return NATIVE_CONTROLS_DEADZONE_DEFAULT;
	}

	return s_profiles[profile].deadzone;
}

void NativeControls_SetDeadzone(int device, s32 percent)
{
	if (!NativeControls_IsValidDevice(device))
	{
		return;
	}

	int profile = s_devices[device].profile;
	if (profile < 0)
	{
		return;
	}

	s_profiles[profile].deadzone = NativeControls_ClampDeadzone(percent);
}

// ---------------------------------------------------------------------------
// Capture
// ---------------------------------------------------------------------------

internal s32 NativeControls_ReadKeyboardPress(void)
{
	const bool *state = SDL_GetKeyboardState(NULL);

	if (state == NULL)
	{
		return NATIVE_CONTROLS_BINDING_NONE;
	}

	for (int i = 0; i < SDL_SCANCODE_COUNT; i++)
	{
		if (state[i])
		{
			return (s32)i;
		}
	}

	return NATIVE_CONTROLS_BINDING_NONE;
}

internal s32 NativeControls_ReadGamepadPress(SDL_Gamepad *gamepad)
{
	if (gamepad == NULL)
	{
		return NATIVE_CONTROLS_BINDING_NONE;
	}

	for (int button = 0; button < SDL_GAMEPAD_BUTTON_COUNT; button++)
	{
		if (SDL_GetGamepadButton(gamepad, (SDL_GamepadButton)button))
		{
			return (s32)button;
		}
	}

	for (int axis = 0; axis < SDL_GAMEPAD_AXIS_COUNT; axis++)
	{
		if (abs(SDL_GetGamepadAxis(gamepad, (SDL_GamepadAxis)axis)) > PLATFORM_INPUT_PRESS_THRESHOLD)
		{
			// Direction is deliberately not encoded. The game already reads a
			// stick axis as positive-is-down/right, so an axis captured in either
			// direction has to resolve to the same binding.
			return (s32)axis | NATIVE_CONTROLS_BINDING_FLAG_AXIS;
		}
	}

	return NATIVE_CONTROLS_BINDING_NONE;
}

// The first control of one device that is currently held, or NATIVE_CONTROLS_BINDING_NONE
// when the device is idle. One scan answers both "is anything held" and "what was pressed",
// so the two cannot disagree on what counts as held.
internal s32 NativeControls_ReadDevicePress(int device)
{
	struct NativeControlsDevice *entry = &s_devices[device];

	if (entry->kind == NATIVE_CONTROLS_DEVICE_KEYBOARD)
	{
		return NativeControls_ReadKeyboardPress();
	}

	return NativeControls_ReadGamepadPress(entry->gamepad);
}

int NativeControls_IsCapturing(void)
{
	return s_capturePhase != NATIVE_CONTROLS_CAPTURE_IDLE;
}

// True while a chosen key is waiting to be confirmed with Cross.
int NativeControls_IsConfirming(void)
{
	return (s_capturePhase == NATIVE_CONTROLS_CAPTURE_CONFIRM) ||
	       (s_capturePhase == NATIVE_CONTROLS_CAPTURE_CONFIRM_WAIT_RELEASE);
}

// Whether the control a binding value names is currently held. An axis counts as held
// past the press threshold, whichever way it is pushed.
internal bool NativeControls_IsValueHeld(int device, s32 value)
{
	struct NativeControlsDevice *entry = &s_devices[device];

	if (value == NATIVE_CONTROLS_BINDING_NONE)
	{
		return false;
	}

	const s32 index = value & ~NATIVE_CONTROLS_BINDING_FLAG_MASK;

	if (entry->kind == NATIVE_CONTROLS_DEVICE_KEYBOARD)
	{
		const bool *state = SDL_GetKeyboardState(NULL);

		if ((state == NULL) || (index < 0))
		{
			return false;
		}

		return state[index] != 0;
	}

	if (entry->gamepad == NULL)
	{
		return false;
	}

	if ((value & NATIVE_CONTROLS_BINDING_FLAG_AXIS) != 0)
	{
		return abs(SDL_GetGamepadAxis(entry->gamepad, (SDL_GamepadAxis)index)) >
		       PLATFORM_INPUT_PRESS_THRESHOLD;
	}

	return SDL_GetGamepadButton(entry->gamepad, (SDL_GamepadButton)index) != 0;
}

// Whether the device's Cross control is currently held. This reads whatever control
// Cross is bound to rather than a physical Cross, so a proposal that binds Cross to
// something the player cannot reach can never be confirmed and reverts instead of
// leaving the device unusable.
internal bool NativeControls_IsCrossHeld(int device)
{
	if (!NativeControls_IsValidDevice(device))
	{
		return false;
	}

	return NativeControls_IsValueHeld(device, NativeControls_GetBinding(device, NATIVE_CONTROLS_BIND_CROSS));
}

void NativeControls_BeginCapture(int device, int binding)
{
	if (!NativeControls_IsValidDevice(device) || !NativeControls_IsValidBinding(binding))
	{
		NativeControls_CancelCapture();
		return;
	}

	s_captureDevice = device;
	s_captureBinding = binding;

	// Remember which physical device the index pointed at. The table is rebuilt
	// whenever devices come and go, so a bare index would silently start listening
	// to whatever took that slot.
	s_captureInstanceId = s_devices[device].instanceId;
	s_capturePhase = NATIVE_CONTROLS_CAPTURE_WAIT_RELEASE;
	s_captureFrames = 0;

	// Show the binding as unbound while listening, so it is visible that the old
	// value is not in effect. The original is kept to restore on timeout or cancel.
	// Written directly rather than through NativeControls_SetBinding so the file is
	// not rewritten on every rebind attempt.
	s_capturePreviousValue = NativeControls_GetBinding(device, binding);

	// Stands in for the proposal until a key is chosen. Nothing treats this as a real
	// binding because the capture is live the whole time, and restoring it on cancel
	// puts the previous value back.
	s_capturePendingValue = NATIVE_CONTROLS_BINDING_NONE;

	s32 *bindings = NativeControls_BindingsOfDevice(device);

	if (bindings != NULL)
	{
		bindings[binding] = s_capturePendingValue;
	}
}

// Puts the pre-capture value back if the binding still holds the proposal. A capture
// that already committed has overwritten it, so this must not stomp that.
internal void NativeControls_RestoreCaptureValue(void)
{
	if (!NativeControls_IsValidDevice(s_captureDevice) || !NativeControls_IsValidBinding(s_captureBinding))
	{
		return;
	}

	s32 *bindings = NativeControls_BindingsOfDevice(s_captureDevice);

	if ((bindings == NULL) || (bindings[s_captureBinding] != s_capturePendingValue))
	{
		return;
	}

	bindings[s_captureBinding] = s_capturePreviousValue;
}

// Clears the capture state without touching the bindings. A commit uses this, since the
// value in the table is then the one just written to disk and must not be mistaken for
// a proposal needing a restore.
internal void NativeControls_EndCapture(void)
{
	s_capturePhase = NATIVE_CONTROLS_CAPTURE_IDLE;
	s_captureDevice = NATIVE_CONTROLS_NO_DEVICE;
	s_captureBinding = -1;
	s_captureInstanceId = -1;
	s_captureFrames = 0;
}

// Writes the proposal into the bindings table and persists it. Split out of PollCapture
// so the commit path can be exercised without a live press to confirm with.
internal int NativeControls_CommitCapture(void)
{
	const int device = s_captureDevice;
	const int binding = s_captureBinding;
	const s32 value = s_capturePendingValue;

	// End the capture before saving. The committed value now equals the proposal, so
	// leaving the capture fields set would let a later CancelCapture mistake the
	// committed value for an unconfirmed proposal and roll the binding back in memory,
	// desyncing the game from the file that was just written.
	NativeControls_EndCapture();

	// The device table can be rebuilt by a refresh, so resolve the binding through
	// the device again rather than trusting the index blindly.
	NativeControls_SetBinding(device, binding, value);
	return 1;
}

// Returns 1 when a value was captured, 2 when the wait timed out, 0 while still
// waiting. The caller uses 2 to tell a revert apart from a still-pending capture.
int NativeControls_PollCapture(void)
{
	if (s_capturePhase == NATIVE_CONTROLS_CAPTURE_IDLE)
	{
		return 0;
	}

	// A device can vanish mid-capture, e.g. unplugged while listening.
	if (!NativeControls_IsValidDevice(s_captureDevice) ||
	    (s_devices[s_captureDevice].instanceId != s_captureInstanceId))
	{
		NativeControls_RestoreCaptureValue();
		NativeControls_CancelCapture();
		return 0;
	}

	s_captureFrames++;

	if (s_captureFrames > NATIVE_CONTROLS_CAPTURE_TIMEOUT_FRAMES)
	{
		NativeControls_RestoreCaptureValue();
		NativeControls_CancelCapture();
		return 2;
	}

	if (s_capturePhase == NATIVE_CONTROLS_CAPTURE_WAIT_RELEASE)
	{
		if (NativeControls_ReadDevicePress(s_captureDevice) == NATIVE_CONTROLS_BINDING_NONE)
		{
			s_capturePhase = NATIVE_CONTROLS_CAPTURE_ARMED;
		}

		return 0;
	}

	if (s_capturePhase == NATIVE_CONTROLS_CAPTURE_CONFIRM_WAIT_RELEASE)
	{
		// Let the chosen key go before accepting the confirming press, otherwise
		// holding it would confirm itself.
		if (NativeControls_ReadDevicePress(s_captureDevice) == NATIVE_CONTROLS_BINDING_NONE)
		{
			s_capturePhase = NATIVE_CONTROLS_CAPTURE_CONFIRM;
		}

		return 0;
	}

	if (s_capturePhase == NATIVE_CONTROLS_CAPTURE_CONFIRM)
	{
		if (!NativeControls_IsCrossHeld(s_captureDevice))
		{
			return 0;
		}

		return NativeControls_CommitCapture();
	}

	s32 value = NativeControls_ReadDevicePress(s_captureDevice);
	if (value == NATIVE_CONTROLS_BINDING_NONE)
	{
		return 0;
	}

	// Propose rather than commit. The value goes in now so the menu can show what is
	// about to be bound, and RestoreCaptureValue takes it back out if the confirm step
	// is not completed.
	s_capturePendingValue = value;

	s32 *bindings = NativeControls_BindingsOfDevice(s_captureDevice);

	if (bindings != NULL)
	{
		bindings[s_captureBinding] = value;
	}

	s_capturePhase = NATIVE_CONTROLS_CAPTURE_CONFIRM_WAIT_RELEASE;
	return 0;
}

void NativeControls_CancelCapture(void)
{
	NativeControls_RestoreCaptureValue();
	NativeControls_EndCapture();
}
