#ifndef CTR_NATIVE_NAMESPACE_UI_H
#define CTR_NATIVE_NAMESPACE_UI_H

struct UiElement2D
{
	s16 x;
	s16 y;
	s16 z;
	s16 scale;
};

enum UIHudSlot
{
	UI_HUD_SLOT_WEAPON = 0x0,
	UI_HUD_SLOT_LAP_COUNT = 0x1,
	UI_HUD_SLOT_BIG1 = 0x2,
	UI_HUD_SLOT_FRUIT_MODEL = 0x3,
	UI_HUD_SLOT_WUMPA_COUNT = 0x4,
	UI_HUD_SLOT_RANK = 0x5,
	UI_HUD_SLOT_JUMP_METER = 0x6,
	UI_HUD_SLOT_SLIDE_METER = 0x8,
	UI_HUD_SLOT_SPEEDOMETER = 0x9,
	UI_HUD_SLOT_BATTLE_WEAPON_BG = 0xb,
	UI_HUD_SLOT_RACING_WEAPON_BG = 0xc,
	UI_HUD_SLOT_BATTLE_SCORE = 0xd,
	UI_HUD_SLOT_RELIC = 0xe,
	UI_HUD_SLOT_KEY = 0xf,
	UI_HUD_SLOT_TROPHY = 0x10,
	UI_HUD_SLOT_CRYSTAL = 0x11,
	UI_HUD_SLOT_TOKEN_OR_CTR = 0x12,
	UI_HUD_SLOT_TIMEBOX = 0x13,
	UI_HUD_SLOT_COUNT = 0x14,
};

enum UIRaceClockFlags
{
	UI_RACE_CLOCK_SHOW_CURRENT_TIME = 0x0,
	UI_RACE_CLOCK_SHOW_RESULTS = 0x1,
	UI_RACE_CLOCK_DRAW_LAPS_IN_RELIC = 0x2,
	UI_RACE_CLOCK_FLASH_TOTAL = 0x4,
	UI_RACE_CLOCK_FLASH_LAP_1 = 0x8,
	UI_RACE_CLOCK_FLASH_LAP_2 = 0x10,
	UI_RACE_CLOCK_FLASH_LAP_3 = 0x20,
};

enum UIWeaponBGConstants
{
	UI_WEAPON_BG_SHINE_THETA_STEP = 0x100,
	UI_WEAPON_BG_SHINE_SCALE_MUL = 0xd000,
	UI_WEAPON_BG_SHINE_SCALE_SHIFT = 0x10,
	UI_WEAPON_BG_SHINE_ICON_INDEX = 0x31,
	UI_WEAPON_BG_SHINE_TRANSPARENCY_BASE = 2,
};

enum UIJumpMeterConstants
{
	UI_JUMP_METER_SIGNIFICANT_THRESHOLD = 0x150,
	UI_JUMP_METER_HANG_TIME_SMALL = 640,
	UI_JUMP_METER_HANG_TIME_MEDIUM = 960,
	UI_JUMP_METER_HANG_TIME_LARGE = 1440,
	UI_JUMP_METER_HANG_TIME_RESERVES = 960,
	UI_JUMP_METER_HANG_TIME_FIRE_SMALL = 0,
	UI_JUMP_METER_HANG_TIME_FIRE_MEDIUM = 0x80,
	UI_JUMP_METER_HANG_TIME_FIRE_LARGE = 0x100,
	UI_JUMP_METER_LANDING_VOICELINE_MIN = 0x480,
	UI_JUMP_METER_LANDING_VOICELINE_CUTOFF = 0x481,
	UI_JUMP_METER_LANDING_VOICELINE_ID = 7,
	UI_JUMP_METER_LANDING_VOICELINE_FLAGS = 0x10,
	UI_JUMP_METER_MAX = 0x960,
	UI_JUMP_METER_DISPLAY_TIMER = 0x5a0,
};

struct UiElement3D
{
	// 0x0
	SVec3 rot;
	s16 scale;

	// 0x8
	MATRIX m;

	// 0x28
	SVec3 lightDir;
	s16 _pad_lightDir;

	// 0x30
	s16 vel[4];

	// 0x38 bytes
};

struct UILapTimes
{
	// NOTE(aalhendi): Each digit has seven lap slots for each of two players.
	// One array keeps player-two indexing within the declared C object.
	s32 minuteUnits[14];
	s32 centisecondTens[14];
	s32 centisecondUnits[14];
	s32 secondTens[14];
	s32 secondUnits[14];
};

CTR_STATIC_ASSERT(sizeof(struct UILapTimes) == 0x118);

struct UIMap
{
	s16 worldEndX;
	s16 worldEndY;
	s16 worldStartX;
	s16 worldStartY;
	s16 iconSizeX;
	s16 iconSizeY;
	s16 iconStartX;
	s16 iconStartY;
	s16 mode;
};

struct UIMapSpawnMetadata
{
	struct UIMap map;

	// 0x12
	s16 topHalfMode;
};

struct QuipStr
{
	s16 lngIndex;
	s16 flags;
	int priority;
};

struct QuipMeta
{
	struct QuipStr *ptrQuipStrCurr;
	struct QuipStr *ptrQuipStrNext;
	s16 conditionType;
	s16 flags;
	int threshold;
	int driverOffset;
	int dataSize;
};

// NOTE(aalhendi): Quip ranges point into these owned arrays on both targets.
struct EndRaceQuipData
{
	struct QuipStr vsStrings[46];
	struct QuipMeta vsMetadata[39];
	struct QuipStr battleStrings[67];
	struct QuipMeta battleMetadata[12];
};

struct UI_VsWaitRdata
{
	SVec2 positions3P4P[4];
	char statFormat[12];
};

CTR_STATIC_ASSERT(sizeof(struct UiElement2D) == 0x8);
CTR_STATIC_ASSERT(OFFSETOF(struct UiElement3D, rot) == 0x0);
CTR_STATIC_ASSERT(OFFSETOF(struct UiElement3D, scale) == 0x6);
CTR_STATIC_ASSERT(OFFSETOF(struct UiElement3D, m) == 0x8);
CTR_STATIC_ASSERT(sizeof(struct UiElement3D) == 0x38);
CTR_STATIC_ASSERT(sizeof(struct UIMap) == 0x12);
CTR_STATIC_ASSERT(OFFSETOF(struct UIMap, mode) == 0x10);
CTR_STATIC_ASSERT(offsetof(struct UIMapSpawnMetadata, topHalfMode) == 0x12);
CTR_STATIC_ASSERT(sizeof(struct UIMapSpawnMetadata) == 0x14);
CTR_STATIC_ASSERT(sizeof(struct QuipStr) == 0x8);
CTR_STATIC_ASSERT(sizeof(struct QuipMeta) == 0x18);
CTR_STATIC_ASSERT(offsetof(struct EndRaceQuipData, vsMetadata) == 0x170);
CTR_STATIC_ASSERT(offsetof(struct EndRaceQuipData, battleStrings) == 0x518);
CTR_STATIC_ASSERT(offsetof(struct EndRaceQuipData, battleMetadata) == 0x730);
CTR_STATIC_ASSERT(sizeof(struct EndRaceQuipData) == 0x850);
CTR_STATIC_ASSERT(offsetof(struct UI_VsWaitRdata, statFormat) == 0x10);
CTR_STATIC_ASSERT(sizeof(struct UI_VsWaitRdata) == 0x1c);

#endif
