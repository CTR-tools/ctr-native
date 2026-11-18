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

// The frame ordering table is one chain of slots that the GPU walks from its last
// slot down to the first, so a higher slot index is emitted first and therefore
// sits at a lower layer. Primitives sharing one slot are emitted in reverse call
// order. The UI table holds five slots: 0 is the default layer for HUD, prompt and
// menu primitives, 1 carries secondary text behind the HUD, 3 carries boxes and bars,
// and 4 is the lowest layer, where the UI draw environment and full-screen
// backgrounds go.
enum UIHudOrderingSlot
{
	UI_HUD_ORDERING_SLOT_BEHIND_HUD = 0x1,
	UI_HUD_ORDERING_SLOT_LOWEST = 0x4,
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
	UI_RACE_CLOCK_COMPACT_RESULTS = 0x40,
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

#endif
