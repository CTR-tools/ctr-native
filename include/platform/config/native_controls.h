#ifndef PLATFORM_NATIVE_CONTROLS_H
#define PLATFORM_NATIVE_CONTROLS_H

#include <macros.h>

#include <platform/native_input.h>

// The PSX pad bus exposes PLATFORM_INPUT_PAD_COUNT player slots, and player N drives
// slot N-1, so that is also how many players there are.

// Room for every player slot's gamepad plus the keyboard.
#define NATIVE_CONTROLS_MAX_DEVICES (PLATFORM_INPUT_PAD_COUNT + 1)

// Length of a device name, for callers sizing a buffer to render one.
#define NATIVE_CONTROLS_NAME_LENGTH 64

// Stick deadzone as a percentage of full deflection. Zero is the console's own behaviour:
// nothing is trimmed, and a stick that drifts steers. The figure is per device, since it
// answers how that particular stick rests rather than how the game reads input.
//
// The maximum stops short of the whole range on purpose: past it there is no travel left to
// give back, so a deadzone that trimmed everything would only ever report centre.
#define NATIVE_CONTROLS_DEADZONE_MIN     0
#define NATIVE_CONTROLS_DEADZONE_MAX     90
#define NATIVE_CONTROLS_DEADZONE_STEP    5
#define NATIVE_CONTROLS_DEADZONE_DEFAULT 0

// Figures are percentages of the full axis travel, so the scale stays 100 whatever the
// maximum is allowed to be. Scaling by the maximum instead would quietly make every
// figure mean a larger share of the stick than it says.
#define NATIVE_CONTROLS_DEADZONE_SCALE 100

// Returned by NativeControls_GetPlayerDevice when a player has no device. Also a
// valid index into the device table, so callers can use one sentinel for both.
#define NATIVE_CONTROLS_NO_DEVICE (-1)

enum NativeControlsDeviceKind
{
	NATIVE_CONTROLS_DEVICE_NONE = 0,
	NATIVE_CONTROLS_DEVICE_KEYBOARD,
	NATIVE_CONTROLS_DEVICE_GAMEPAD,
};

enum NativeControlsPlayerPref
{
	// Take the next free device, gamepads before the keyboard.
	NATIVE_CONTROLS_PREF_AUTO = 0,

	// Deliberately have no device. Refused for player 1.
	NATIVE_CONTROLS_PREF_DISABLED,

	// Hold a specific device for as long as it stays connected.
	NATIVE_CONTROLS_PREF_PINNED,
};

enum NativeControlsBinding
{
	NATIVE_CONTROLS_BIND_CROSS = 0,
	NATIVE_CONTROLS_BIND_CIRCLE,
	NATIVE_CONTROLS_BIND_TRIANGLE,
	NATIVE_CONTROLS_BIND_SQUARE,
	NATIVE_CONTROLS_BIND_L1,
	NATIVE_CONTROLS_BIND_L2,
	NATIVE_CONTROLS_BIND_L3,
	NATIVE_CONTROLS_BIND_R1,
	NATIVE_CONTROLS_BIND_R2,
	NATIVE_CONTROLS_BIND_R3,
	NATIVE_CONTROLS_BIND_DPAD_UP,
	NATIVE_CONTROLS_BIND_DPAD_DOWN,
	NATIVE_CONTROLS_BIND_DPAD_LEFT,
	NATIVE_CONTROLS_BIND_DPAD_RIGHT,
	NATIVE_CONTROLS_BIND_START,
	NATIVE_CONTROLS_BIND_SELECT,
	NATIVE_CONTROLS_BIND_AXIS_LEFT_X,
	NATIVE_CONTROLS_BIND_AXIS_LEFT_Y,
	NATIVE_CONTROLS_BIND_AXIS_RIGHT_X,
	NATIVE_CONTROLS_BIND_AXIS_RIGHT_Y,
	NATIVE_CONTROLS_BIND_COUNT,
};

// Bit flag packed into a gamepad binding value by the platform layer, to tell a
// gamepad axis apart from a button.
#define NATIVE_CONTROLS_BINDING_FLAG_AXIS 0x4000
#define NATIVE_CONTROLS_BINDING_FLAG_MASK NATIVE_CONTROLS_BINDING_FLAG_AXIS

// Binding value meaning "nothing is bound here". Must be negative, because the
// flag bit above would otherwise alias it into a valid gamepad axis.
#define NATIVE_CONTROLS_BINDING_NONE (-2)

// File name resolved against the executable base directory.
#define NATIVE_CONTROLS_FILE_NAME "controls.ini"

// Section name used for the host keyboard in controls.ini.
#define NATIVE_CONTROLS_KEYBOARD_KEY "keyboard"

// Loads controls.ini and resets every preference to automatic. Does not touch
// SDL devices; call NativeControls_Refresh once the gamepad subsystem is up.
void NativeControls_Init(void);

// Re-enumerates devices and re-resolves the player assignment. Cheap to call
// repeatedly; bumps the generation counter only when something actually changed.
void NativeControls_Refresh(void);

void NativeControls_Save(void);

// Bumped whenever the device set or the player assignment changes, so the input
// layer can decide when to re-sync pad slots to devices.
u32 NativeControls_GetGeneration(void);

// Player index is 0-based. The result indexes the device table, or
// NATIVE_CONTROLS_NO_DEVICE when the player is disabled.
int NativeControls_GetPlayerDevice(int player);
int NativeControls_GetPlayerPref(int player);

// Changes a player's preference, re-resolves, and persists. `device` is only read
// for NATIVE_CONTROLS_PREF_PINNED. Returns 0 when the request was refused, which
// only happens for a disabled player 1.
int NativeControls_SetPlayerPref(int player, int pref, int device);

// Devices the player may pick: every connected device that no other player owns.
// NATIVE_CONTROLS_NO_DEVICE is never in the list, so player 1 cannot be offered
// "disabled" and the caller does not need a special case. Gamepads come first and
// the keyboard last. Returns the number written to dst.
int NativeControls_GetAvailableDevices(int player, int *dst, int max);

int NativeControls_GetDeviceKind(int device);
const char *NativeControls_GetDeviceName(int device);

// Open SDL handle for a gamepad device, NULL for the keyboard. Owned by this
// module; valid until the device set changes.
void *NativeControls_GetGamepadHandle(int device);

// False for missing devices, keyboards, and gamepads without rumble support.
bool NativeControls_CanVibrate(int device);

// Vibration belongs to the player, defaults to on, and is persisted in controls.ini.
bool NativeControls_GetPlayerVibration(int player);
void NativeControls_SetPlayerVibration(int player, bool enabled);

// Binding value in the platform encoding: an SDL_Scancode for the keyboard, or a
// button/axis with NATIVE_CONTROLS_BINDING_FLAG_* flags for a gamepad.
s32 NativeControls_GetBinding(int device, int binding);

// Row name for a binding, e.g. "Cross" or "Left Stick Y".

// Display name of what a device currently has bound to a binding, e.g. "A" or
// "Left Shift". Empty when nothing is bound. Writes into caller storage.
void NativeControls_GetBindingLabel(int device, int binding, char *dst, size_t dstSize);

// Sets a binding and persists the file.
void NativeControls_SetBinding(int device, int binding, s32 value);

// Replaces every binding on one device with the shipped defaults, then persists.
void NativeControls_RestoreDeviceDefaults(int device);

// Stick deadzone for one device, as a percentage of full deflection clamped to
// NATIVE_CONTROLS_DEADZONE_MIN..MAX, or NATIVE_CONTROLS_DEADZONE_DEFAULT without a device.
s32 NativeControls_GetDeadzone(int device);

// Sets the deadzone in memory. The value is clamped, not refused.
void NativeControls_SetDeadzone(int device, s32 percent);

// Waits for the device to go neutral, then takes the next press on that device alone and
// applies it. The binding reads as unbound for the duration; if the capture is cancelled
// or times out, the previous value comes back and controls.ini is left untouched.
void NativeControls_BeginCapture(int device, int binding);

// True while a capture is waiting for a press, so callers can ignore their own
// input handling for the device being listened to.
int NativeControls_IsCapturing(void);

// True once a key has been chosen and is waiting for the confirming Cross press. The
// binding holds the proposed value during this step.
int NativeControls_IsConfirming(void);

// Polls the pending capture, advancing its timeout by one call. A capture is two steps:
// take a press, then confirm it with whatever Cross is bound to on the same device.
//
// Returns 1 when a value was confirmed and committed, 2 when the wait timed out or was
// abandoned and the binding was reverted, and 0 while still waiting.
int NativeControls_PollCapture(void);

// Abandons a pending capture and restores the binding it was holding.
void NativeControls_CancelCapture(void);

#endif
