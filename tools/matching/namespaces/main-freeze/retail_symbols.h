#ifndef CTR_MATCHING_MAIN_FREEZE_RETAIL_SYMBOLS_H
#define CTR_MATCHING_MAIN_FREEZE_RETAIL_SYMBOLS_H

// NOTE(aalhendi): The linker places this scalar at _gp+832 so GCC can emit
// and schedule the retail gp-relative tracker load before the call frame.
#define GAME_TRACKER                      ctr_mainFreezeGameTracker
// NOTE(aalhendi): The menu stores and low-half frame-counter read use absolute
// pages in retail, not gp-relative offsets.
#define GAME_DESIRED_MENU                 ctr_mainFreezeDesiredMenu
#define GAME_MAIN_MENU_STATE              ctr_mainFreezeMainMenuState
#define GAME_FRAME_COUNTER_LOW            ctr_mainFreezeFrameCounterLow

// NOTE(aalhendi): The resident EXE links the shared trig table inside data.
#define MAIN_FREEZE_TRIG_ASM_NAME         "data+15360"
#define MAIN_FREEZE_TRIG_TABLE            ctr_mainFreezeTrigTable
#define MAIN_FREEZE_WHEEL_DATA            ctr_mainFreezeWheelData
#define MAIN_FREEZE_DEAD_ZONE_OPTIONS     ctr_mainFreezeDeadZoneOptions
#define MAIN_FREEZE_RANGE_OPTIONS         ctr_mainFreezeRangeOptions
#define MAIN_FREEZE_OPTION_COUNTS         ctr_mainFreezeOptionCounts
#define MAIN_FREEZE_ARROW_COLORS          ctr_mainFreezeArrowColors
#define MAIN_FREEZE_BLUE_RECT_COLORS      ctr_mainFreezeBlueRectColors
#define MAIN_FREEZE_NAMCO_TRIANGLE_COLORS ctr_mainFreezeNamcoTriangleColors
#define MAIN_FREEZE_NAMCO_GEOMETRY        ctr_mainFreezeNamcoGeometry
#define MAIN_FREEZE_NAMCO_ICON_OFFSETS    ctr_mainFreezeNamcoIconOffsets
#define MAIN_FREEZE_NAMCO_ROWS            ctr_mainFreezeNamcoRows
#define MAIN_FREEZE_NAMCO_ICON_COLORS     ctr_mainFreezeNamcoIconColors
#define MAIN_FREEZE_BUTTON_TAPS           ctr_mainFreezeButtonTaps
#define MAIN_FREEZE_MULTILINE_Y           ctr_mainFreezeMultilineY
#define MAIN_FREEZE_ARROW_Y               ctr_mainFreezeArrowY
#define MAIN_FREEZE_LOAD_TRIG_BASE(page, base, offset)                                            \
	do                                                                                            \
	{                                                                                             \
		CTR_PSX_LOAD_SYMBOL_PAGE_AFTER((page), MAIN_FREEZE_TRIG_ASM_NAME, (offset));              \
		CTR_PSX_ADD_SYMBOL_LOW((base), (page), MAIN_FREEZE_TRIG_ASM_NAME, (u8 *)data.trigApprox); \
	} while (0)
#define MAIN_FREEZE_LOAD_TRIG_PAGE(page)      CTR_PSX_LOAD_SYMBOL_PAGE((page), MAIN_FREEZE_TRIG_ASM_NAME)
#define MAIN_FREEZE_ADD_TRIG_LOW(base, page)  CTR_PSX_ADD_SYMBOL_LOW((base), (page), MAIN_FREEZE_TRIG_ASM_NAME, data.trigApprox)
#define MAIN_FREEZE_TRIANGLE_OPTIONS_ASM_NAME "data+14580"
#define MAIN_FREEZE_LOAD_TRIANGLE_OPTIONS(page, base, dependency)                                                       \
	do                                                                                                                  \
	{                                                                                                                   \
		CTR_PSX_LOAD_SYMBOL_PAGE_AFTER((page), MAIN_FREEZE_TRIANGLE_OPTIONS_ASM_NAME, (dependency));                    \
		CTR_PSX_ADD_SYMBOL_LOW((base), (page), MAIN_FREEZE_TRIANGLE_OPTIONS_ASM_NAME, &data.raceConfig_unk80084290[2]); \
	} while (0)

// NOTE(aalhendi): These real gp-relative scalar symbols are disjoint from the
// outgoing argument stack. Aggregate gp-pointer accesses create false alias
// dependencies in GCC 2.8.1 and change retail argument scheduling.
#define MAIN_FREEZE_WHEEL_CENTER_Y   ctr_mainFreezeWheelCenterY
#define MAIN_FREEZE_DISPLAY_VALUE    ctr_mainFreezeDisplayValue
#define MAIN_FREEZE_LANGUAGE_STRINGS ctr_mainFreezeLanguageStrings
#define MAIN_FREEZE_NAMCO_CENTER_Y   ctr_mainFreezeNamcoCenterY
#include <common.h>
extern char **ctr_mainFreezeLanguageStrings;
extern u16 ctr_mainFreezeNamcoCenterY;
extern s32 ctr_mainFreezeWheelCenterY;
extern s32 ctr_mainFreezeDisplayValue;

#undef GHOST_REPLAY_HUMAN
#define GHOST_REPLAY_HUMAN ctr_mainFreezeReplayHuman
#undef GHOST_PLAYING
#define GHOST_PLAYING ctr_mainFreezeGhostPlaying
#undef GAME_ANY_PLAYER_TAP
#define GAME_ANY_PLAYER_TAP ctr_mainFreezeAnyPlayerTap

extern struct GameTracker *ctr_mainFreezeGameTracker;
extern struct RectMenu *ctr_mainFreezeDesiredMenu asm("sdata_static+2488");
extern MainMenuState ctr_mainFreezeMainMenuState asm("sdata_static+2576");
extern u16 ctr_mainFreezeFrameCounterLow asm("sdata_static+2564");
extern b16 ctr_mainFreezeReplayHuman asm("sdata_static+2540");
extern struct GhostHeader *ctr_mainFreezeGhostPlaying asm("sdata_static+2024");
extern u32 ctr_mainFreezeAnyPlayerTap asm("sdata_static+2532");
extern struct RacingWheelData ctr_mainFreezeWheelData[4] asm("data+14428");
extern struct RaceWheelConfigOption ctr_mainFreezeDeadZoneOptions[4] asm("data+14520");
extern struct RaceWheelConfigOption ctr_mainFreezeRangeOptions[5] asm("data+14544");
extern u16 ctr_mainFreezeOptionCounts[14] asm("data+14576");
extern u8 ctr_mainFreezeArrowColors[0xc] asm("data+14604");
extern u8 ctr_mainFreezeBlueRectColors[0x18] asm("data+14616");
extern u8 ctr_mainFreezeNamcoTriangleColors[0xc] asm("data+14640");
extern s16 ctr_mainFreezeNamcoGeometry[0x18] asm("data+14652");
extern s16 ctr_mainFreezeNamcoIconOffsets[6] asm("data+14688");
extern s16 ctr_mainFreezeNamcoRows[6] asm("data+14664");
extern s16 ctr_mainFreezeNamcoIconColors[6] asm("data+14676");
extern u32 ctr_mainFreezeButtonTaps[4] asm("sdata_static+55844");
extern s16 ctr_mainFreezeMultilineY[4] asm("sdata_static+884");
extern s16 ctr_mainFreezeArrowY[4] asm("sdata_static+892");
extern struct TrigTable ctr_mainFreezeTrigTable[0x400] asm("data+15360");

#endif
