#ifndef CTR_MATCHING_RETAIL_BINDINGS_H
#define CTR_MATCHING_RETAIL_BINDINGS_H

// NOTE(aalhendi): Shared resident addresses for private matching bindings.
// Access widths and addressing forms may still differ by compilation unit.
#define RETAIL_GAME_TRACKER_ASM_NAME       "sdata_static+832"
#define RETAIL_LANGUAGE_STRINGS_ASM_NAME   "sdata_static+2316"
#define RETAIL_CHARACTER_METADATA_ASM_NAME "data+25572"
#define RETAIL_CHARACTER_IDS_ASM_NAME      "data+25828"
#define RETAIL_LEVEL_METADATA_ASM_NAME     "data+12512"
#define RETAIL_BOSS_WEAPON_META_ASM_NAME   "data+20528"
#define RETAIL_GAME_SAVE_ASM_NAME          "sdata_static+6012"
#define RETAIL_ADD_CONFIG_0_ASM_NAME       "sdata_static+404"

#define GAME_TRACKER                       ctr_gameTrackerPtr
#define GAME_LANGUAGE_STRINGS              ctr_languageStrings
#define GAME_CHARACTER_METADATA            ctr_characterMetadata
#define GAME_CHARACTER_IDS                 ctr_characterIDs
#define GAME_LEVEL_METADATA                ctr_levelMetadata
#define GAME_BOSS_WEAPON_METADATA           ctr_bossWeaponMetaPtr
#define GAME_FRAMES_SINCE_RACE_ENDED       ctr_framesSinceRaceEnded
#define GAME_MENU_READY                    ctr_menuReady
#define GAME_ANY_PLAYER_TAP                ctr_anyPlayerTap
#define GAME_ADV_PROGRESS                  ctr_advProgress
#define GAME_ADV_RNG                       ctr_advRng
#define GAME_SAVE                          ctr_gameSave
#define GAME_PROGRESS                      (GAME_SAVE.progress)
#define GAMEPADS                           ctr_gamepads
#define GAME_MENU_HIGHLIGHT                ctr_menuHighlight
#define GAME_TOKEN                         ctr_token
#define GAME_ADD_CONFIG_0                  ctr_addConfig0
#define GAME_REMOVE_CONFIG_0               ctr_removeConfig0
#define GAME_DOOR_ACCESS_FLAGS             ctr_doorAccessFlags

#include <common.h>

// NOTE(aalhendi): These declarations name existing resident storage. Native
// accesses the same fields through the canonical sData and Data aggregates.
extern struct GameTracker *ctr_gameTrackerPtr asm(RETAIL_GAME_TRACKER_ASM_NAME);
extern char **ctr_languageStrings asm(RETAIL_LANGUAGE_STRINGS_ASM_NAME);
extern struct MetaDataCHAR ctr_characterMetadata[16] asm(RETAIL_CHARACTER_METADATA_ASM_NAME);
extern s16 ctr_characterIDs[8] asm(RETAIL_CHARACTER_IDS_ASM_NAME);
extern struct MetaDataLEV ctr_levelMetadata[0x41] asm(RETAIL_LEVEL_METADATA_ASM_NAME);
extern struct MetaDataBOSS *ctr_bossWeaponMetaPtr[5] asm(RETAIL_BOSS_WEAPON_META_ASM_NAME);

extern s32 ctr_framesSinceRaceEnded asm("sdata_static+1472");
extern s32 ctr_menuReady asm("sdata_static+1360");
extern s32 ctr_anyPlayerTap asm("sdata_static+2532");
extern struct AdvProgress ctr_advProgress asm("sdata_static+11320");
extern struct RngDeadCoedState ctr_advRng asm("sdata_static+1788");
extern struct GameSave ctr_gameSave asm(RETAIL_GAME_SAVE_ASM_NAME);
extern struct GamepadSystem *ctr_gamepads asm("sdata_static+836");
extern Color ctr_menuHighlight asm("sdata_static+2528");
extern struct Instance *ctr_token asm("sdata_static+2660");
extern u32 ctr_addConfig0 asm(RETAIL_ADD_CONFIG_0_ASM_NAME);
extern u32 ctr_removeConfig0 asm("sdata_static+408");
extern u32 ctr_doorAccessFlags asm("sdata_static+1980");

#endif
