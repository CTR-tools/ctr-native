#include <platform/native_input.h>

#include <macros.h>

#include <platform/native_config.h>
#include <platform/native_controls.h>
#include "platform/native_log.h"
#include "psx/libpad.h"

#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NATIVE_INPUT_MAX_CONTROLLERS       PLATFORM_INPUT_PAD_COUNT
#define NATIVE_INPUT_PHYSICAL_SLOT_COUNT   2
#define NATIVE_INPUT_PAD_PACKET_BYTES      8
#define NATIVE_INPUT_MULTITAP_HEADER       2
#define NATIVE_INPUT_PAD_ANALOG            0x73
#define NATIVE_INPUT_PAD_MULTITAP          0x80
#define NATIVE_INPUT_PAD_DISCONNECT        0xff
// NOTE(aalhendi): Little-endian tag `CTRI` = CTR native Input snapshot.
#define NATIVE_INPUT_STATE_MAGIC           0x49525443

#define NATIVE_INPUT_STATE_VERSION         3

// NOTE(aalhendi): Native input preserves behavior from PsyCross's
// MIT-licensed pad implementation while moving host ownership into ctr-native.
// See THIRD_PARTY_NOTICES.md.

// PSX pad packet bits, one per bindable control. Axes contribute no bit: they are
// carried in the packet's analog bytes instead.
local_persist const u16 s_bindingButtonMask[NATIVE_CONTROLS_BIND_COUNT] = {
	[NATIVE_CONTROLS_BIND_CROSS] = 0x4000,
	[NATIVE_CONTROLS_BIND_CIRCLE] = 0x2000,
	[NATIVE_CONTROLS_BIND_TRIANGLE] = 0x1000,
	[NATIVE_CONTROLS_BIND_SQUARE] = 0x8000,
	[NATIVE_CONTROLS_BIND_L1] = 0x400,
	[NATIVE_CONTROLS_BIND_L2] = 0x100,
	[NATIVE_CONTROLS_BIND_L3] = 0x2,
	[NATIVE_CONTROLS_BIND_R1] = 0x800,
	[NATIVE_CONTROLS_BIND_R2] = 0x200,
	[NATIVE_CONTROLS_BIND_R3] = 0x4,
	[NATIVE_CONTROLS_BIND_DPAD_UP] = 0x10,
	[NATIVE_CONTROLS_BIND_DPAD_DOWN] = 0x40,
	[NATIVE_CONTROLS_BIND_DPAD_LEFT] = 0x80,
	[NATIVE_CONTROLS_BIND_DPAD_RIGHT] = 0x20,
	[NATIVE_CONTROLS_BIND_START] = 0x8,
	[NATIVE_CONTROLS_BIND_SELECT] = 0x1,
	[NATIVE_CONTROLS_BIND_AXIS_LEFT_X] = 0,
	[NATIVE_CONTROLS_BIND_AXIS_LEFT_Y] = 0,
	[NATIVE_CONTROLS_BIND_AXIS_RIGHT_X] = 0,
	[NATIVE_CONTROLS_BIND_AXIS_RIGHT_Y] = 0,
};

// One pad slot per player: slot N is driven by whatever device player N owns.
struct NativeInputController
{
	struct PlatformInputPadSnapshot snapshot;
	int device;
};

struct NativeInputControllerStateSnapshot
{
	struct PlatformInputPadSnapshot snapshot;
};

struct NativeInputStateSnapshot
{
	u32 magic;
	u32 version;
	u32 size;
	s32 installedSnapshotsActive;
	struct PlatformInputPadSnapshot installedSnapshots[NATIVE_INPUT_MAX_CONTROLLERS];
	struct NativeInputControllerStateSnapshot controllers[NATIVE_INPUT_MAX_CONTROLLERS];
};

global_variable struct NativeInputController s_controllers[NATIVE_INPUT_MAX_CONTROLLERS];
global_variable struct PlatformInputPadSnapshot s_installedSnapshots[NATIVE_INPUT_MAX_CONTROLLERS];
global_variable u8 *s_padSlotData[NATIVE_INPUT_PHYSICAL_SLOT_COUNT];
global_variable const bool *s_keyboardState;
global_variable s32 s_inputInitialized;
global_variable s32 s_installedSnapshotsActive;
global_variable u32 s_syncedGeneration;

extern s32 g_padCommEnable;

internal u16 NativeInput_GetSnapshotButtons(const struct PlatformInputPadSnapshot *snapshot)
{
	return (u16)(snapshot->buttons[0] | (snapshot->buttons[1] << 8));
}

internal void NativeInput_SetSnapshotButtons(struct PlatformInputPadSnapshot *snapshot, u16 buttons)
{
	snapshot->buttons[0] = (u8)(buttons & 0xff);
	snapshot->buttons[1] = (u8)(buttons >> 8);
}

internal void NativeInput_ResetSnapshot(s32 slot)
{
	struct PlatformInputPadSnapshot *snapshot = &s_controllers[slot].snapshot;

	snapshot->connected = 0;
	snapshot->status = NATIVE_INPUT_PAD_DISCONNECT;
	snapshot->id = NATIVE_INPUT_PAD_DISCONNECT;
	NativeInput_SetSnapshotButtons(snapshot, 0xffff);
	snapshot->analog[0] = 0x80;
	snapshot->analog[1] = 0x80;
	snapshot->analog[2] = 0x80;
	snapshot->analog[3] = 0x80;
	memset(snapshot->reserved, 0, sizeof(snapshot->reserved));
}

internal void NativeInput_ResetController(s32 slot)
{
	s_controllers[slot].device = NATIVE_CONTROLS_NO_DEVICE;
	NativeInput_ResetSnapshot(slot);
	s_installedSnapshots[slot] = s_controllers[slot].snapshot;
}

internal s32 NativeInput_IsValidControllerSlot(s32 slot)
{
	return (slot >= 0) && (slot < NATIVE_INPUT_MAX_CONTROLLERS);
}

internal void NativeInput_MakeDisconnectedSnapshot(struct PlatformInputPadSnapshot *snapshot)
{
	if (snapshot == NULL)
	{
		return;
	}

	snapshot->connected = 0;
	snapshot->status = NATIVE_INPUT_PAD_DISCONNECT;
	snapshot->id = NATIVE_INPUT_PAD_DISCONNECT;
	NativeInput_SetSnapshotButtons(snapshot, 0xffff);
	snapshot->analog[0] = 0x80;
	snapshot->analog[1] = 0x80;
	snapshot->analog[2] = 0x80;
	snapshot->analog[3] = 0x80;
	memset(snapshot->reserved, 0, sizeof(snapshot->reserved));
}

internal void NativeInput_WritePadPacket(u8 *dst, const struct PlatformInputPadSnapshot *snapshot)
{
	if ((dst == NULL) || (snapshot == NULL))
	{
		return;
	}

	dst[0] = snapshot->status;
	dst[1] = snapshot->id;
	dst[2] = snapshot->buttons[0];
	dst[3] = snapshot->buttons[1];
	dst[4] = snapshot->analog[0];
	dst[5] = snapshot->analog[1];
	dst[6] = snapshot->analog[2];
	dst[7] = snapshot->analog[3];
}

internal void NativeInput_WritePadBus(void)
{
	u8 *slot0 = s_padSlotData[0];
	u8 *slot1 = s_padSlotData[1];
	s32 slot;

	// The host has no notion of one or two physical PS1 ports, so the distinction only
	// made sense on retail hardware. Always presenting a multitap keeps the mapping
	// between player and pad slot stable: the game then walks four pads on port 0 and
	// player N always reads slot N-1, whatever devices happen to be connected.
	if (slot0 != NULL)
	{
		// Multitap header followed by four pad packets on the single port.
		slot0[0] = 0;
		slot0[1] = NATIVE_INPUT_PAD_MULTITAP;
		for (slot = 0; slot < NATIVE_INPUT_MAX_CONTROLLERS; slot++)
		{
			NativeInput_WritePadPacket(&slot0[NATIVE_INPUT_MULTITAP_HEADER + (slot * NATIVE_INPUT_PAD_PACKET_BYTES)], &s_controllers[slot].snapshot);
		}
	}

	if (slot1 != NULL)
	{
		// Nothing is ever reported on the second port while the multitap is in use.
		struct PlatformInputPadSnapshot disconnected;

		NativeInput_MakeDisconnectedSnapshot(&disconnected);
		NativeInput_WritePadPacket(slot1, &disconnected);
	}
}

internal void NativeInput_WriteInstalledSnapshots(void)
{
	for (s32 slot = 0; slot < NATIVE_INPUT_MAX_CONTROLLERS; slot++)
	{
		s_controllers[slot].snapshot = s_installedSnapshots[slot];
	}

	NativeInput_WritePadBus();
}

// Slot N follows the device the controls module gave player N.
//
// The controls module publishes a generation counter that moves whenever the
// assignment changes, including changes made from the options menu, and this is the
// only thing that re-projects the slots. `s_controllers[].device` must not be cached
// anywhere else: holding a device index across an assignment change would keep
// serving the old device.
internal void NativeInput_SyncDevices(void)
{
	const u32 generation = NativeControls_GetGeneration();

	if (generation == s_syncedGeneration)
	{
		return;
	}

	s_syncedGeneration = generation;

	for (s32 slot = 0; slot < NATIVE_INPUT_MAX_CONTROLLERS; slot++)
	{
		s_controllers[slot].device = NativeControls_GetPlayerDevice(slot);
	}
}

internal SDL_Gamepad *NativeInput_GamepadForSlot(s32 slot)
{
	if (!NativeInput_IsValidControllerSlot(slot))
	{
		return NULL;
	}

	int device = s_controllers[slot].device;
	if (device == NATIVE_CONTROLS_NO_DEVICE)
	{
		return NULL;
	}

	return (SDL_Gamepad *)NativeControls_GetGamepadHandle(device);
}

internal s32 NativeInput_ControllerButtonState(SDL_Gamepad *gamepad, s32 bindingValue)
{
	// Checked before the axis flag: an unbound value is negative, and the flag
	// would otherwise alias it into a valid axis index.
	if ((gamepad == NULL) || (bindingValue < 0))
	{
		return 0;
	}

	const s32 index = bindingValue & ~NATIVE_CONTROLS_BINDING_FLAG_MASK;

	if ((bindingValue & NATIVE_CONTROLS_BINDING_FLAG_AXIS) != 0)
	{
		return SDL_GetGamepadAxis(gamepad, (SDL_GamepadAxis)index);
	}

	return SDL_GetGamepadButton(gamepad, (SDL_GamepadButton)index) * 32767;
}

internal u8 NativeInput_AxisToByte(s32 axis)
{
	s32 value = (axis / 256) + 128;

	if (value < 0)
	{
		return 0;
	}

	if (value > 0xff)
	{
		return 0xff;
	}

	return (u8)value;
}

// Reads one device into the pad packet bits for every control bound to a button.
// A binding with no packet bit is skipped rather than tested, which keeps the
// analog bindings out of the digital half of the packet.
internal u16 NativeInput_ReadDeviceButtons(int device, SDL_Gamepad *gamepad)
{
	u16 buttons = 0xffff;

	for (s32 binding = 0; binding < NATIVE_CONTROLS_BIND_COUNT; binding++)
	{
		u16 mask = s_bindingButtonMask[binding];

		if (mask == 0)
		{
			continue;
		}

		s32 value = NativeControls_GetBinding(device, binding);
		if (value == NATIVE_CONTROLS_BINDING_NONE)
		{
			continue;
		}

		if (NativeInput_ControllerButtonState(gamepad, value) > PLATFORM_INPUT_PRESS_THRESHOLD)
		{
			buttons &= (u16)~mask;
		}
	}

	return buttons;
}

// Pulls the axis in toward the centre until it clears the deadzone, then stretches what is
// left of the travel back across the full range. Without that stretch a stick with any
// deadzone could never report full deflection, and steering would quietly top out short.
internal s32 NativeInput_ApplyDeadzone(s32 axis, s32 percent)
{
	if ((percent <= NATIVE_CONTROLS_DEADZONE_MIN) || (axis == 0))
	{
		return axis;
	}

	// An SDL axis runs one unit further negative than positive, so the negative end's
	// magnitude is the full scale. Measuring against the shorter positive end instead would
	// push the far negative end back outside the axis range.
	const s32 full = -SDL_JOYSTICK_AXIS_MIN;
	const s32 dead = percent * full / NATIVE_CONTROLS_DEADZONE_SCALE;
	const s32 magnitude = (axis < 0) ? -axis : axis;

	// A deadzone wide enough to cover the whole axis trims everything, which leaves nothing
	// to stretch and a zero denominator. That end is reachable whatever maximum is
	// configured, so the guard lives here rather than in the clamp on the setter.
	if ((dead >= full) || (magnitude <= dead))
	{
		return 0;
	}

	// Rescaled rather than truncated: the travel above the deadzone is stretched back over
	// the full scale, so trimming the centre does not cost the ends. What it costs instead
	// is sensitivity just past the deadzone, which is the price of still reaching full
	// deflection.
	const s32 reach = full - dead;
	const s32 stretched = (s32)(((long long)(magnitude - dead) * full) / reach);

	return (axis < 0) ? -stretched : stretched;
}

internal void NativeInput_ApplyController(s32 slot)
{
	struct NativeInputController *nativeController = &s_controllers[slot];
	struct PlatformInputPadSnapshot *snapshot = &nativeController->snapshot;
	SDL_Gamepad *gamepad = NativeInput_GamepadForSlot(slot);

	if ((gamepad == NULL) || (SDL_GamepadConnected(gamepad) == 0))
	{
		return;
	}

	int device = nativeController->device;
	u16 buttons = NativeInput_ReadDeviceButtons(device, gamepad);
	const s32 deadzone = NativeControls_GetDeadzone(device);

	snapshot->connected = 1;
	snapshot->status = 0;
	snapshot->id = NATIVE_INPUT_PAD_ANALOG;

	s32 rightX = NativeInput_ControllerButtonState(gamepad, NativeControls_GetBinding(device, NATIVE_CONTROLS_BIND_AXIS_RIGHT_X));
	s32 rightY = NativeInput_ControllerButtonState(gamepad, NativeControls_GetBinding(device, NATIVE_CONTROLS_BIND_AXIS_RIGHT_Y));
	s32 leftX = NativeInput_ControllerButtonState(gamepad, NativeControls_GetBinding(device, NATIVE_CONTROLS_BIND_AXIS_LEFT_X));
	s32 leftY = NativeInput_ControllerButtonState(gamepad, NativeControls_GetBinding(device, NATIVE_CONTROLS_BIND_AXIS_LEFT_Y));

	NativeInput_SetSnapshotButtons(snapshot, buttons);
	snapshot->analog[0] = NativeInput_AxisToByte(NativeInput_ApplyDeadzone(rightX, deadzone));
	snapshot->analog[1] = NativeInput_AxisToByte(NativeInput_ApplyDeadzone(rightY, deadzone));
	snapshot->analog[2] = NativeInput_AxisToByte(NativeInput_ApplyDeadzone(leftX, deadzone));
	snapshot->analog[3] = NativeInput_AxisToByte(NativeInput_ApplyDeadzone(leftY, deadzone));
}

internal u16 NativeInput_ReadKeyboard(int device)
{
	u16 buttons = 0xffff;

	if (s_keyboardState == NULL)
	{
		return buttons;
	}

	for (s32 binding = 0; binding < NATIVE_CONTROLS_BIND_COUNT; binding++)
	{
		u16 mask = s_bindingButtonMask[binding];

		if (mask == 0)
		{
			continue;
		}

		s32 scancode = NativeControls_GetBinding(device, binding);
		if (scancode < 0)
		{
			continue;
		}

		if (s_keyboardState[scancode])
		{
			buttons &= (u16)~mask;
		}
	}

	return buttons;
}

// Alt belongs to the host window manager: Alt+Tab, Alt+F4 and Alt+Enter must never
// reach the game. This only answers whether Alt is held; what it does about it is
// up to the caller.
internal s32 NativeInput_KeyboardSuppressed(void)
{
	if (s_keyboardState == NULL)
	{
		return 0;
	}

	return s_keyboardState[SDL_SCANCODE_RALT] || s_keyboardState[SDL_SCANCODE_LALT];
}

// The keyboard is a device like any other: it is merged into whichever slot owns it, which
// is not necessarily slot 0, and it reports an analog pad like every other device. That is
// safe: unbound axes sit at the centre value the game reads as neutral, and the controls
// screen keeps the stick rows off the keyboard.
//
// The Alt suppression applies to the buttons only. Skipping the keyboard's whole
// contribution also skipped its connection flag, so holding Alt made the game report
// the pad as unplugged. The buttons are already released by the frame's reset, so
// leaving the merge out leaves every button up.
internal void NativeInput_ApplyKeyboard(void)
{
	const s32 suppressed = NativeInput_KeyboardSuppressed();

	for (s32 slot = 0; slot < NATIVE_INPUT_MAX_CONTROLLERS; slot++)
	{
		int device = s_controllers[slot].device;

		if ((device == NATIVE_CONTROLS_NO_DEVICE) ||
		    (NativeControls_GetDeviceKind(device) != NATIVE_CONTROLS_DEVICE_KEYBOARD))
		{
			continue;
		}

		struct PlatformInputPadSnapshot *snapshot = &s_controllers[slot].snapshot;

		if (snapshot->connected == 0)
		{
			snapshot->connected = 1;
			snapshot->status = 0;
			snapshot->id = NATIVE_INPUT_PAD_ANALOG;
		}

		if (suppressed != 0)
		{
			continue;
		}

		u16 buttons = NativeInput_GetSnapshotButtons(snapshot);
		NativeInput_SetSnapshotButtons(snapshot, buttons & NativeInput_ReadKeyboard(device));
	}
}

int Platform_InputInit(void)
{
	if (s_inputInitialized != 0)
	{
		return 1;
	}

	memset(s_controllers, 0, sizeof(s_controllers));
	memset(s_padSlotData, 0, sizeof(s_padSlotData));
	for (s32 slot = 0; slot < NATIVE_INPUT_MAX_CONTROLLERS; slot++)
	{
		NativeInput_ResetController(slot);
	}

	s_installedSnapshotsActive = 0;
	s_syncedGeneration = 0;
	s_keyboardState = SDL_GetKeyboardState(NULL);

	if (SDL_InitSubSystem(SDL_INIT_GAMEPAD | SDL_INIT_HAPTIC) == 0)
	{
		fprintf(stderr, "[CTR Native Expanded] Failed to initialise SDL input subsystem: %s\n", SDL_GetError());
		return 0;
	}

	SDL_AddGamepadMappingsFromFile("gamecontrollerdb.txt");

	// Requires the gamepad subsystem, so it cannot happen in NativeControls_Init.
	NativeControls_Refresh();
	NativeInput_SyncDevices();

	s_inputInitialized = 1;
	return 1;
}

void Platform_InputShutdown(void)
{
	if (s_inputInitialized != 0)
	{
		SDL_QuitSubSystem(SDL_INIT_GAMEPAD | SDL_INIT_HAPTIC);
	}

	for (s32 slot = 0; slot < NATIVE_INPUT_MAX_CONTROLLERS; slot++)
	{
		s_controllers[slot].device = NATIVE_CONTROLS_NO_DEVICE;
	}

	s_inputInitialized = 0;
	s_installedSnapshotsActive = 0;
	s_syncedGeneration = 0;
	memset(s_padSlotData, 0, sizeof(s_padSlotData));
	s_keyboardState = NULL;
}

void Platform_InputUpdate(void)
{
	if (s_inputInitialized == 0)
	{
		return;
	}

	if (s_installedSnapshotsActive != 0)
	{
		// NOTE(aalhendi): replay/state installs PSX-shaped pad bytes here;
		// SDL host state is not serialized.
		NativeInput_WriteInstalledSnapshots();
		return;
	}

	if (g_padCommEnable == 0)
	{
		return;
	}

	SDL_PumpEvents();

	// Hotplug hooks already re-resolved the assignment, so this only picks up the
	// result. It is one comparison when nothing changed.
	NativeInput_SyncDevices();

	for (s32 slot = 0; slot < NATIVE_INPUT_MAX_CONTROLLERS; slot++)
	{
		NativeInput_ResetSnapshot(slot);
		NativeInput_ApplyController(slot);
	}

	NativeInput_ApplyKeyboard();

	if (g_config.omniController)
	{
		// TEST: map all 4 controllers to port 1 input.
		for (s32 slot = 1; slot < NATIVE_INPUT_MAX_CONTROLLERS; slot++)
		{
			s_controllers[slot].snapshot = s_controllers[0].snapshot;
		}
	}

	NativeInput_WritePadBus();
}

void Platform_InputControllerAdded(int deviceIndex)
{
	// The controls module re-enumerates and re-assigns from SDL state, so there is
	// nothing to do here beyond picking the change up promptly.
	(void)deviceIndex;
	NativeControls_Refresh();
}

void Platform_InputControllerRemoved(int instanceId)
{
	(void)instanceId;
	NativeControls_Refresh();
}

// Reports the current assignment. The old F4/F6 keys overrode the assignment at
// runtime, which the controls menu now owns outright, so they only report.
void Platform_InputLogAssignment(void)
{
	NativeControls_Refresh();

	for (s32 player = 0; player < NATIVE_INPUT_MAX_CONTROLLERS; player++)
	{
		int device = NativeControls_GetPlayerDevice(player);
		const char *name = (device == NATIVE_CONTROLS_NO_DEVICE) ? "disabled" : NativeControls_GetDeviceName(device);

		Platform_LogWarn("[CTR Native Expanded] Player %d device: %s\n", player + 1, name);
	}
}

void Platform_InputPadInit(int slot, unsigned char *padData)
{
	if ((slot < 0) || (slot >= NATIVE_INPUT_PHYSICAL_SLOT_COUNT))
	{
		return;
	}

	s_padSlotData[slot] = padData;
	NativeInput_WritePadBus();
}

int Platform_InputPadGetState(int port)
{
	// On the multitap bus the four pads all sit on port 0 and the low bits of the
	// address select one of them, so port 1 never reports a pad.
	const s32 physicalSlot = (port >> 4) & 1;
	const s32 tap = port & 3;

	if (physicalSlot != 0)
	{
		return PadStateDiscon;
	}

	if (!NativeInput_IsValidControllerSlot(tap))
	{
		return PadStateDiscon;
	}

	return s_controllers[tap].snapshot.connected ? PadStateStable : PadStateDiscon;
}

int Platform_InputCapturePadSnapshots(struct PlatformInputPadSnapshot *dst, int count)
{
	if ((dst == NULL) || (count < NATIVE_INPUT_MAX_CONTROLLERS))
	{
		return 0;
	}

	for (s32 slot = 0; slot < NATIVE_INPUT_MAX_CONTROLLERS; slot++)
	{
		dst[slot] = s_controllers[slot].snapshot;
	}

	return NATIVE_INPUT_MAX_CONTROLLERS;
}

int Platform_InputInstallPadSnapshots(const struct PlatformInputPadSnapshot *src, int count)
{
	if ((src == NULL) || (count < NATIVE_INPUT_MAX_CONTROLLERS))
	{
		return 0;
	}

	for (s32 slot = 0; slot < NATIVE_INPUT_MAX_CONTROLLERS; slot++)
	{
		s_installedSnapshots[slot] = src[slot];
	}

	s_installedSnapshotsActive = 1;
	NativeInput_WriteInstalledSnapshots();
	return NATIVE_INPUT_MAX_CONTROLLERS;
}

void Platform_InputClearInstalledPadSnapshots(void)
{
	s_installedSnapshotsActive = 0;
}

int Platform_InputGetStateSize(void)
{
	return (int)sizeof(struct NativeInputStateSnapshot);
}

int Platform_InputCaptureState(void *dst, int dstSize)
{
	struct NativeInputStateSnapshot *snapshot = (struct NativeInputStateSnapshot *)dst;

	if ((dst == NULL) || (dstSize < (int)sizeof(*snapshot)))
	{
		return 0;
	}

	memset(snapshot, 0, sizeof(*snapshot));
	snapshot->magic = NATIVE_INPUT_STATE_MAGIC;
	snapshot->version = NATIVE_INPUT_STATE_VERSION;
	snapshot->size = sizeof(*snapshot);
	snapshot->installedSnapshotsActive = s_installedSnapshotsActive;

	for (s32 slot = 0; slot < NATIVE_INPUT_MAX_CONTROLLERS; slot++)
	{
		snapshot->installedSnapshots[slot] = s_installedSnapshots[slot];
		snapshot->controllers[slot].snapshot = s_controllers[slot].snapshot;
	}

	return 1;
}

int Platform_InputRestoreState(const void *src, int srcSize)
{
	const struct NativeInputStateSnapshot *snapshot = (const struct NativeInputStateSnapshot *)src;
	s32 slot;

	if ((src == NULL) || (srcSize < (int)sizeof(*snapshot)))
	{
		return 0;
	}
	if ((snapshot->magic != NATIVE_INPUT_STATE_MAGIC) || (snapshot->version != NATIVE_INPUT_STATE_VERSION) ||
	    (snapshot->size != sizeof(*snapshot)))
	{
		return 0;
	}
	if ((snapshot->installedSnapshotsActive < 0) || (snapshot->installedSnapshotsActive > 1))
	{
		return 0;
	}

	s_installedSnapshotsActive = snapshot->installedSnapshotsActive != 0;

	for (slot = 0; slot < NATIVE_INPUT_MAX_CONTROLLERS; slot++)
	{
		s_installedSnapshots[slot] = snapshot->installedSnapshots[slot];
		s_controllers[slot].snapshot = snapshot->controllers[slot].snapshot;
	}
	NativeInput_WritePadBus();

	return 1;
}

void Platform_InputPadVibrate(int port, unsigned char *table, int len)
{
	// On the multitap bus the four pads all sit on port 0 and the low bits of the
	// address select one of them, so a rumble for port 1 has no pad to land on.
	const s32 physicalSlot = (port >> 4) & 1;

	if (physicalSlot != 0)
	{
		return;
	}

	if ((table == NULL) || (len <= 0))
	{
		return;
	}

	SDL_Gamepad *gamepad = NativeInput_GamepadForSlot(port & 3);
	if (gamepad == NULL)
	{
		return;
	}

	u16 freqHigh = table[0] * 255;
	u16 freqLow = len > 1 ? table[1] * 255 : 0;

	if ((freqLow != 0) && (freqLow < 4096))
	{
		freqLow = 4096;
	}

	if ((freqHigh != 0) && (freqHigh < 4096))
	{
		freqHigh = 4096;
	}

	SDL_RumbleGamepad(gamepad, freqLow, freqHigh, 200);
}
