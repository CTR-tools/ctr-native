#ifndef CTR_NATIVE_NAMESPACE_GAMEPAD_H
#define CTR_NATIVE_NAMESPACE_GAMEPAD_H

// an exact copy of the PadButton enum in PSn00bSDK's psxpad header, for whatever reason
// minus raw input data for non-standard controllers
enum RawInput
{
	RAW_BTN_SELECT = 0x1,
	RAW_BTN_L3 = 0x2,
	RAW_BTN_R3 = 0x4,
	RAW_BTN_START = 0x8,
	RAW_BTN_UP = 0x10,
	RAW_BTN_RIGHT = 0x20,
	RAW_BTN_DOWN = 0x40,
	RAW_BTN_LEFT = 0x80,
	RAW_BTN_L2 = 0x100,
	RAW_BTN_R2 = 0x200,
	RAW_BTN_L1 = 0x400,
	RAW_BTN_R1 = 0x800,
	RAW_BTN_TRIANGLE = 0x1000,
	RAW_BTN_CIRCLE = 0x2000,
	RAW_BTN_CROSS = 0x4000,
	RAW_BTN_SQUARE = 0x8000,
	RAW_BTN_COUNT = 14
};

// CTR also uses a different set of values for controller inputs
// ...why?
enum Buttons
{
	BTN_UP = 0x1,
	BTN_DOWN = 0x2,
	BTN_LEFT = 0x4,
	BTN_RIGHT = 0x8,
	BTN_CROSS_one = 0x10,
	BTN_CROSS_two = RAW_BTN_CROSS,
	BTN_CROSS = BTN_CROSS_one | BTN_CROSS_two,
	BTN_SQUARE_one = 0x20,
	BTN_SQUARE_two = RAW_BTN_SQUARE,
	BTN_SQUARE = BTN_SQUARE_one | BTN_SQUARE_two,
	BTN_CIRCLE = 0x40,
	BTN_L2_one = 0x80,
	BTN_L2_two = RAW_BTN_L2,
	BTN_L2 = BTN_L2_one | BTN_L2_two,
	BTN_R2 = 0x200,
	BTN_R1 = 0x400,
	BTN_L1 = 0x800,
	BTN_START = 0x1000,
	BTN_SELECT = 0x2000,
	BTN_L3 = 0x10000,
	BTN_R3 = 0x20000,
	BTN_TRIANGLE = 0x40000
};

struct GamepadButtonMap
{
	// 0x0
	u32 rawInput;

	// 0x4
	u32 buttons;
};

enum Plug
{
	UNPLUGGED = -1,
	PLUGGED = 0
};

struct ControllerPacket
{
	// 0x0
	u8 plugged;

	// 0x1
	u8 controllerData;

	// 0x2
	struct
	{
		u8 high;
		u8 low;
	} input;

	// 0x4
	union
	{
		struct
		{
			u8 rightX, rightY; // Right stick coordinates
			u8 leftX, leftY;   // Left stick coordinates
		} analog;
		struct
		{
			s8 x_mov, y_mov; // X, Y movement of mouse
		} mouse;
		struct
		{
			u8 twist; // Controller twist
			u8 btn_1; // 1 button value
			u8 btn_2; // 2 button value
			u8 trg_l; // L trigger value
		} neGcon;
		struct
		{
			s16 jog_rot; // Jog rotation
		} jogcon;
		struct
		{
			u16 gun_x; // Gun X position in dotclocks
			u16 gun_y; // Gun Y position in scanlines
		} guncon;
	} payload;
};

struct MultitapPacket
{
	// 0x0
	u8 plugged;

	// 0x1
	u8 controllerData;

	// 0x2
	struct ControllerPacket controllers[4];
};

// NOTE(aalhendi): A physical port contains either a single-pad packet or a
// multitap header followed by four packets. Both views share the two-byte header.
union GamepadSlot
{
	struct ControllerPacket controller;
	struct MultitapPacket multitap;
};

CTR_STATIC_ASSERT(sizeof(struct ControllerPacket) == 8);
CTR_STATIC_ASSERT(sizeof(struct MultitapPacket) == 34);
CTR_STATIC_ASSERT(sizeof(union GamepadSlot) == 34);

struct GamepadBuffer
{
	// 0
	s16 unk_0;

	// stick values
	// 0 for left
	// 80 for middle
	// FF for right

	// 2
	s16 unk_1;

	// 4
	s16 stickLX;

	// 6
	s16 stickLY;

	// 8
	s16 stickLX_dontUse1;

	// A
	s16 stickLY_dontUse1;

	// C
	s16 stickRX;

	// E
	s16 stickRY;

	// 0x10
	int buttonsHeldCurrFrame;

	// 0x14
	int buttonsTapped;

	// 0x18
	int buttonsReleased;

	// 0x1C
	int buttonsHeldPrevFrame;

	// 0x20
	// For details,
	// see GamepadSystem->slotBuffer
	struct ControllerPacket *ptrControllerPacket;

	// 0x24
	s16 gamepadID; // 0 - 7

	// 0x26
	// 0 - no analog sticks
	// 2 - dual analog, or dualshock
	s16 gamepadType;

	// 0x28
	u16 framesSinceLastInput;

	// desired can be nullified before submission,
	// if power is above the 60-unit hardware budget

	// 0x2A
	u8 motorDesired[2];

	// 0x2C
	u8 motorPower[2];

	// 0x2E
	u8 motorSubmit[2];

	// === DualShock ===

	// 0x30
	int shockFrameFreq;
	int shockFrameForce1;
	int shockFrameForce2;

	// 0x3C
	int shockValFreq;
	u8 shockValForce1;
	u8 shockValForce2;

	// ==== JogCon ====

	u8 unk42;
	u8 unk43;

	// 0x44
	u8 unk44; // 2A
	u8 unk45; // 2A

	// elapsedTim timers
	s16 unk46; // vib1 2A
	s16 unk48; // vib2 2A

	s16 padding;

	// 0x4c
	struct RacingWheelData *rwd;
};

struct GamepadSystem
{
	// 0x0
	struct GamepadBuffer gamepad[8];

	// 0x280
	s16 unk;

	// 0x282
	char unkE[0xE];

	// what's 0x282?

	// 0x290, 0x294, 0x298, 0x29c,
	u32 anyoneHeldCurr;
	u32 anyoneTapped;
	u32 anyoneReleased;
	u32 anyoneHeldPrev;

	// 0x2A0
	char unk22[0x22];

	// 2C2
	s16 unk_2C2;

	// 2C4
	int unk_2C4;
	int unk_2C8;

	int unk_2CC;

	// Two physical ports, each holding one pad or four multitap pads.
	union GamepadSlot slotBuffer[2];

	// 0x314
	int numGamepadsConnected;

	// 0x318
	u32 gamepadsConnectedByFlag;

	// 0x31C
	// end of gamepad system
};

struct RacingWheelData
{
	// 0x0
	s16 gamepadCenter;

	// 0x2
	s16 deadZone;

	// 0x4
	s16 range;
};

CTR_STATIC_ASSERT(sizeof(struct GamepadBuffer) == 0x50);
CTR_STATIC_ASSERT(sizeof(struct GamepadButtonMap) == 0x8);
CTR_STATIC_ASSERT(sizeof(struct GamepadSystem) == 0x31C);
CTR_STATIC_ASSERT(sizeof(struct RacingWheelData) == 6);

#endif
