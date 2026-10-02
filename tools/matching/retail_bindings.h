#ifndef CTR_MATCHING_RETAIL_BINDINGS_H
#define CTR_MATCHING_RETAIL_BINDINGS_H

// NOTE(aalhendi): Shared resident addresses for private matching bindings.
// Access widths and addressing forms may still differ by compilation unit.
// NOTE(aalhendi): Units that use the four-byte gp slot supply a link alias.
#ifndef RETAIL_GAME_TRACKER_ASM_NAME
#define RETAIL_GAME_TRACKER_ASM_NAME "sdata_static+832"
#endif
#define RETAIL_LANGUAGE_STRINGS_ASM_NAME                     "sdata_static+2316"
#define RETAIL_CHARACTER_METADATA_ASM_NAME                   "data+25572"
#define RETAIL_CHARACTER_IDS_ASM_NAME                        "data+25828"
#define RETAIL_2P_AI_SETS_ASM_NAME                           "data+12468"
#define RETAIL_OVERLAY_CALLBACKS_ASM_NAME                    "data+12496"
#define RETAIL_CURR_SLOT_ASM_NAME                            "data+12444"
#define RETAIL_QUEUE_SLOTS_ASM_NAME                          "sdata_static+37848"
#define RETAIL_LEVEL_METADATA_ASM_NAME                       "data+12512"
#define RETAIL_BOSS_WEAPON_META_ASM_NAME                     "data+20528"
#define RETAIL_GAME_SAVE_ASM_NAME                            "sdata_static+6012"
#define RETAIL_ADD_CONFIG_0_ASM_NAME                         "sdata_static+404"
#define RETAIL_HOWL_MAGIC_ASM_NAME                           "sdata_static+244"
#define RETAIL_MEMCARD_STATE_ASM_NAME                        "sdata_static+56004"
#define RETAIL_GHOST_TEMPLATE_ASM_NAME                       "data+20624"

// NOTE(aalhendi): LOAD_TenStages materializes these link addresses in a
// particular order. Keep the retail page and signed lows in the private ABI;
// native uses the corresponding GAME_* objects instead.
#define RETAIL_LOAD_PREFIX_PAGE                              0x80090000
#define RETAIL_LOAD_PREFIX_ENDING_LOW                        -12092
#define RETAIL_LOAD_PREFIX_INTRO_LOW                         -12084
#define RETAIL_LOAD_PREFIX_SCREEN_LOW                        -12076
#define RETAIL_LOAD_PREFIX_GARAGE_LOW                        -12068
#define RETAIL_LOAD_PREFIX_HUB_LOW                           -12060
#define RETAIL_LOAD_PREFIX_CREDIT_LOW                        -12056
#define RETAIL_LOAD_PREFIX_GARAGE_ASM_NAME                   "sdata_static+368"
#define RETAIL_LOAD_DRIVER_CALLBACK_ASM_NAME                 "LOAD_Callback_DriverModels"
#define RETAIL_LOAD_DRIVER_CALLBACK_LOW                      0x1b00
#define RETAIL_LOAD_PATCH_MEMORY_ASM_NAME                    "rdata+4552"
#define RETAIL_LOAD_PATCH_MEMORY_LOW                         4552

#define GAME_TRACKER                                         ctr_gameTrackerPtr
#define GAME_TRAFFIC_LIGHT_PREVIOUS_TIMER                    ctr_trafficLightPreviousTimer
#define GAME_LOADING_STAGE                                   ctr_loadingStage
#define GAME_LOAD_IN_PROGRESS                                ctr_loadInProgress
#define GAME_LANGUAGE_STRINGS                                ctr_languageStrings
#define GAME_CHARACTER_METADATA                              ctr_characterMetadata
#define GAME_CHARACTER_IDS                                   ctr_characterIDs
#define GAME_HUD_LAYOUTS                                     ctr_hudLayouts
#define GAME_LAP_MINUTE_UNITS                                ctr_lapMinuteUnits
#define GAME_LAP_CENTISECOND_TENS                            ctr_lapCentisecondTens
#define GAME_LAP_CENTISECOND_UNITS                           ctr_lapCentisecondUnits
#define GAME_LAP_SECOND_TENS                                 ctr_lapSecondTens
#define GAME_LAP_SECOND_UNITS                                ctr_lapSecondUnits
#define GAME_BATTLE_CAMERA_HEIGHTS                           ctr_battleCameraHeights
#define GAME_SPIN360_DRIVER_HEIGHTS                          ctr_spin360DriverHeights
#define GAME_CAMERA_THREAD_NAME                              ctr_cameraThreadName
#define GAME_2P_AI_SETS                                      ctr_2pAiSets
#define GAME_OVERLAY_CALLBACKS                               ctr_overlayCallbacks
#define GAME_LEVEL_BIG_LOD_INDEX                             ctr_levelBigLodIndex
#define GAME_LEVEL_METADATA                                  ctr_levelMetadata
#define GAME_LEVEL_PREFIX_NDI                                ctr_levelPrefixNdi
#define GAME_LEVEL_PREFIX_ENDING                             ctr_levelPrefixEnding
#define GAME_LEVEL_PREFIX_INTRO                              ctr_levelPrefixIntro
#define GAME_LEVEL_PREFIX_SCREEN                             ctr_levelPrefixScreen
#define GAME_LEVEL_PREFIX_GARAGE                             ctr_levelPrefixGarage
#define GAME_LEVEL_PREFIX_HUB                                ctr_levelPrefixHub
#define GAME_LEVEL_PREFIX_CREDIT                             ctr_levelPrefixCredit
#define GAME_RDATA_NAME(name)                                ctr_retail_##name
#define GAME_SDATA_NAME(name)                                ctr_sdata_##name
#define GAME_PRIM_MEM_SIZES_1P                               ctr_primMemSizes1P
#define GAME_PRIM_MEM_SIZES_2P                               ctr_primMemSizes2P
#define GAME_PRIM_MEM_SIZES_4P                               ctr_primMemSizes4P
#define GAME_CLIP_BUFFERS                                    ctr_clipBuffers
#define GAME_LEVEL_CLEAR_RED                                 ctr_levelClearRed
#define GAME_LEVEL_CLEAR_GREEN                               ctr_levelClearGreen
#define GAME_LEVEL_CLEAR_BLUE                                ctr_levelClearBlue
#define GAME_MAIN_MENU_STATE                                 ctr_mainMenuState
#define GAME_XA_STATE                                        ctr_xaState
#define GAME_BOSS_WEAPON_METADATA                            ctr_bossWeaponMetaPtr
#define GAME_FRAMES_SINCE_RACE_ENDED                         ctr_framesSinceRaceEnded
#define GAME_MENU_READY                                      ctr_menuReady
#define GAME_ANY_PLAYER_TAP                                  ctr_anyPlayerTap
#define GAME_ADV_PROGRESS                                    ctr_advProgress
#define GAME_ADV_RNG                                         ctr_advRng
#define GAME_SAVE                                            ctr_gameSave
#define GAME_PROGRESS                                        (GAME_SAVE.progress)
#define GAMEPADS                                             ctr_gamepads
#define GAME_MENU_HIGHLIGHT                                  ctr_menuHighlight
#define GAME_TOKEN                                           ctr_token
#define GAME_ADD_CONFIG_0                                    ctr_addConfig0
#define GAME_REMOVE_CONFIG_0                                 ctr_removeConfig0
#define GAME_DOOR_ACCESS_FLAGS                               ctr_doorAccessFlags
#define GAME_DRIVER_MODEL_EXTRAS                             ctr_driverModelExtras
#define GAME_PLAYER_OBJECT_LIST                              ctr_playerObjectList
#define GAME_SONG_SEQUENCES                                  ctr_songSequences
#define GAME_SONG_POOL                                       ctr_songPool
#define GAME_NOTE_FREQUENCY                                  ctr_noteFrequency
#define GAME_DISTORT_CONST_MUSIC                             ctr_distortConstMusic
#define GAME_CHANNEL_UPDATE_FLAGS                            ctr_channelUpdateFlags
#define GAME_CHANNEL_ATTR_NEW                                ctr_channelAttrNew
#define GAME_VOLUME_LR                                       ctr_volumeLR
#define GAME_WRONG_WAY_DIRECTION                             ctr_wrongWayDirection
#define GAME_SAME_DIRECTION_FRAMES                           ctr_framesDrivingSameDirection
#define GAME_DISTORT_CONST_OTHER_FX                          ctr_distortConstOtherFX
#define GAME_HOWL_REVERB_PARAMS                              ctr_howlReverbParams
#define GAME_CHANNEL_TAKEN                                   ctr_channelTaken
#define GAME_CHANNEL_FREE                                    ctr_channelFree
#define GAME_HOWL_CD_FILE                                    ctr_howlCdFile
#define GAME_AUDIO_ENABLED                                   ctr_audioEnabled
#define GAME_HOWL_BANK_OFFSETS                               ctr_howlBankOffsets
#define GAME_HOWL_HEADER                                     ctr_howlHeader
#define GAME_CSEQ_HEADER                                     ctr_cseqHeader
#define GAME_HOWL_SPU_ADDRS                                  ctr_howlSpuAddrs
#define GAME_GARAGE_SOUND_POOL                               ctr_garageSoundPool
#define GAME_MEMCARD_STATE                                   ctr_memcardState
#define GAME_MEMCARD_DIR_HEADER                              ctr_memcardDirHeader
#define GAME_MEMCARD_SW_IOE                                  ctr_memcardSwIOE
#define GAME_MEMCARD_HW_IOE                                  ctr_memcardHwIOE
#define GAME_MEMCARD_ICON_CRASH                              ctr_memcardIconCrash
#define GAME_MEMCARD_ICON_GHOST                              ctr_memcardIconGhost
#define GAME_AUDIO_BANKS                                     ctr_audioBanks
#define GAME_AUDIO_BANK_COUNT                                ctr_audioBankCount

// NOTE(aalhendi): Count is at 2048($gp); keep that ABI displacement out of game source.
#define GAME_AUDIO_BANK_COUNT_LOAD_AFTER(result, dependency) CTR_PSX_LOAD_GP_UNSIGNED_BYTE_AFTER(result, 2048, GAME_AUDIO_BANK_COUNT, dependency)

#define GAME_HOWL_SAMPLE_BLOCK_NAME                          ctr_howlLoadSampleBlockName

#define GAME_COLLISION_WORK                                  ctr_collisionWork

#define GAME_MATRIX_TABLE_BAKED                              ctr_matrixTableBaked

#define GAME_LOAD_QUEUE_READY                                ctr_loadQueueReady
#define GAME_LOAD_VRAM_FINISHED                              ctr_loadVramFinished
#define GAME_LOAD_FILE_LABEL                                 ctr_loadFileLabel

#include <common.h>

// NOTE(aalhendi): These declarations name existing resident storage. Native
// accesses the same fields through the canonical sData and Data aggregates.
extern struct GameTracker *ctr_gameTrackerPtr asm(RETAIL_GAME_TRACKER_ASM_NAME);
extern s32 ctr_trafficLightPreviousTimer asm("168($28)");
extern u8 ctr_matrixTableBaked asm("1572($28)");
extern b8 ctr_loadQueueReady asm("308($28)");
extern s32 ctr_loadVramFinished asm("316($28)");
// NOTE(aalhendi): An unspecified extent keeps the FILE allocation label
// absolute rather than classifying its eight-byte storage as small data.
extern char ctr_loadFileLabel[] asm("sdata_static+320");
extern struct ScratchpadStruct ctr_collisionWork asm("sdata_static+2992");
extern LoadStage ctr_loadingStage asm("sdata_static+396");
extern s32 ctr_loadInProgress asm("312($28)");
extern char **ctr_languageStrings asm(RETAIL_LANGUAGE_STRINGS_ASM_NAME);
extern struct MetaDataCHAR ctr_characterMetadata[16] asm(RETAIL_CHARACTER_METADATA_ASM_NAME);
extern s16 ctr_characterIDs[8] asm(RETAIL_CHARACTER_IDS_ASM_NAME);
// NOTE(aalhendi): The owned HUD-layout pointer table is indexed by player count minus one.
extern struct UiElement2D *ctr_hudLayouts[4] asm("data+22716");
extern s32 ctr_lapMinuteUnits[14] asm("sdata_static+56468");
extern s32 ctr_lapCentisecondTens[14] asm("sdata_static+56524");
extern s32 ctr_lapCentisecondUnits[14] asm("sdata_static+56580");
extern s32 ctr_lapSecondTens[14] asm("sdata_static+56636");
extern s32 ctr_lapSecondUnits[14] asm("sdata_static+56692");
extern s32 ctr_battleCameraHeights[5] asm("data+1588");
extern s32 ctr_spin360DriverHeights[5] asm("data+1608");
// NOTE(aalhendi): An unspecified extent preserves retail absolute addressing;
// the eight-byte camera name would otherwise be classified as small data.
extern const char ctr_cameraThreadName[] asm("sdata_static+32");
extern u8 ctr_2pAiSets[LOAD_2P_AI_SET_COUNT][LOAD_2P_AI_SET_RACER_COUNT] asm(RETAIL_2P_AI_SETS_ASM_NAME);
extern void *ctr_overlayCallbacks[4] asm(RETAIL_OVERLAY_CALLBACKS_ASM_NAME);
extern u8 ctr_levelBigLodIndex[8] asm("sdata_static+328");
extern struct MetaDataLEV ctr_levelMetadata[0x41] asm(RETAIL_LEVEL_METADATA_ASM_NAME);
extern u8 ctr_primMemSizes1P[28] asm("data+15276");
extern u8 ctr_primMemSizes2P[28] asm("data+15304");
extern u8 ctr_primMemSizes4P[28] asm("data+15332");
extern void *ctr_clipBuffers[4] asm("data+14472");
// NOTE(aalhendi): Scalar views of the existing RGB array avoid array-base
// aliasing reloads in MainInit's three independent retail debug stores.
extern s32 ctr_levelClearRed asm("sdata_static+136");
extern s32 ctr_levelClearGreen asm("sdata_static+140");
extern s32 ctr_levelClearBlue asm("sdata_static+144");
// NOTE(aalhendi): Unspecified label extents retain absolute addressing under G8.
extern char ctr_retail_s_ThreadPool[] asm("rdata+5016");
extern char ctr_retail_s_InstancePool[] asm("rdata+5028");
extern char ctr_retail_s_SmallStackPool[] asm("rdata+5044");
extern char ctr_retail_s_MediumStackPool[] asm("rdata+5060");
extern char ctr_retail_s_LargeStackPool[] asm("rdata+5076");
extern char ctr_retail_s_ParticlePool[] asm("rdata+5092");
extern char ctr_retail_s_OscillatorPool[] asm("rdata+5108");
extern char ctr_retail_s_RainPool[] asm("rdata+5124");
extern char ctr_retail_s_RENDER_BUCKET_INSTANCE[] asm("rdata+5136");
extern char ctr_retail_s_Clip_Buffer[] asm("rdata+5160");
extern char ctr_retail_s_TRACKING[] asm("rdata+5172");
extern char ctr_retail_s_FOLLOWER[] asm("rdata+5184");
extern char ctr_retail_s_STARTTEXT[] asm("rdata+5196");
extern char ctr_sdata_s_ot0[] asm("sdata_static+912");
extern char ctr_sdata_s_ot1[] asm("sdata_static+920");
extern char ctr_sdata_s_PLAYER[] asm("sdata_static+928");
extern char ctr_sdata_s_PLYR[] asm("sdata_static+936");
extern char ctr_sdata_s_ROBOT[] asm("sdata_static+944");
extern char ctr_sdata_s_RBOT[] asm("sdata_static+952");
extern char ctr_sdata_s_STATIC[] asm("sdata_static+960");
extern char ctr_sdata_s_STAT[] asm("sdata_static+968");
extern char ctr_sdata_s_MINE[] asm("sdata_static+976");
extern char ctr_sdata_s_WARPPAD[] asm("sdata_static+984");
extern char ctr_sdata_s_WRPD[] asm("sdata_static+992");
extern char ctr_sdata_s_TRKG[] asm("sdata_static+1000");
extern char ctr_sdata_s_BURST[] asm("sdata_static+1008");
extern char ctr_sdata_s_BRST[] asm("sdata_static+1016");
extern char ctr_sdata_s_BLOWUP[] asm("sdata_static+1024");
extern char ctr_sdata_s_BLWP[] asm("sdata_static+1032");
extern char ctr_sdata_s_TURBO_again[] asm("sdata_static+1040");
extern char ctr_sdata_s_TRBO[] asm("sdata_static+1048");
extern char ctr_sdata_s_SPIDER[] asm("sdata_static+1056");
extern char ctr_sdata_s_SPDR[] asm("sdata_static+1064");
extern char ctr_sdata_s_FLWR[] asm("sdata_static+1072");
extern char ctr_sdata_s_STXT[] asm("sdata_static+1080");
extern char ctr_sdata_s_OTHER[] asm("sdata_static+1088");
extern char ctr_sdata_s_OTHR[] asm("sdata_static+1096");
extern char ctr_sdata_s_AKUAKU[] asm("sdata_static+1104");
extern char ctr_sdata_s_AKUU[] asm("sdata_static+1112");
extern char ctr_sdata_s_CAMERA[] asm("sdata_static+1120");
extern char ctr_sdata_s_CAMR[] asm("sdata_static+1128");
extern char ctr_sdata_s_HUD[] asm("sdata_static+1136");
extern char ctr_sdata_s_THUD[] asm("sdata_static+1140");
extern char ctr_sdata_s_PAUSE[] asm("sdata_static+1148");
extern char ctr_sdata_s_PAUS[] asm("sdata_static+1156");
extern char ctr_levelPrefixNdi[4] asm("sdata_static+340");
extern char ctr_levelPrefixEnding[8] asm("sdata_static+344");
extern char ctr_levelPrefixIntro[8] asm("sdata_static+352");
extern char ctr_levelPrefixScreen[8] asm("sdata_static+360");
extern char ctr_levelPrefixGarage[8] asm(RETAIL_LOAD_PREFIX_GARAGE_ASM_NAME);
extern char ctr_levelPrefixHub[4] asm("sdata_static+376");
extern char ctr_levelPrefixCredit[8] asm("sdata_static+380");
extern char ctr_retail_s_circle[] asm("rdata+4572");
extern char ctr_retail_s_HUB_ALLOC[] asm("rdata+4540");
extern char ctr_retail_s_Patch_Table_Memory[] asm(RETAIL_LOAD_PATCH_MEMORY_ASM_NAME);
extern char ctr_retail_s_clod[] asm("rdata+4588");
extern char ctr_retail_s_dustpuff[] asm("rdata+4604");
extern char ctr_retail_s_smokering[] asm("rdata+4620");
extern char ctr_retail_s_sparkle[] asm("rdata+4636");
extern char ctr_retail_s_lightredoff[] asm("rdata+4652");
extern char ctr_retail_s_lightredon[] asm("rdata+4680");
extern char ctr_retail_s_lightgreenoff[] asm("rdata+4708");
extern char ctr_retail_s_lightgreenon[] asm("rdata+4740");
extern s16 ctr_mainMenuState asm("sdata_static+2576");
extern XAState ctr_xaState asm("sdata_static+1948");
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
extern DriverModelExtraSlot ctr_driverModelExtras[LOAD_DRIVER_MODEL_EXTRA_COUNT] asm("data+12400");
extern struct Model **ctr_playerObjectList asm("sdata_static+2308");
extern struct SongSeq ctr_songSequences[NUM_SFX_CHANNELS] asm("sdata_static+13152");
extern struct Song ctr_songPool[2] asm("sdata_static+36376");
extern u16 ctr_noteFrequency[0x6c] asm("data+9484");
extern u16 ctr_distortConstMusic[0x40] asm("data+9700");
extern u32 ctr_channelUpdateFlags[NUM_SFX_CHANNELS] asm("sdata_static+11520");
extern struct ChannelAttr ctr_channelAttrNew[NUM_SFX_CHANNELS] asm("sdata_static+11616");
extern u8 ctr_volumeLR[0x100] asm("data+9228");
extern u8 ctr_wrongWayDirection asm("sdata_static+2672");
extern s32 ctr_framesDrivingSameDirection asm("sdata_static+2680");
extern s32 ctr_distortConstOtherFX[0x100] asm("data+8204");
extern SpuReverbAttr ctr_howlReverbParams[5] asm("data+7080");
extern struct LinkedList ctr_channelTaken asm("sdata_static+13824");
extern struct LinkedList ctr_channelFree asm("sdata_static+13836");
extern CdlFILE ctr_howlCdFile asm("sdata_static+36624");
extern u8 ctr_audioEnabled asm("sdata_static+240");
extern u16 *ctr_howlBankOffsets asm("sdata_static+2168");
extern struct HowlHeader *ctr_howlHeader asm("sdata_static+2132");
extern struct CseqHeader *ctr_cseqHeader asm("sdata_static+2116");
extern struct SpuAddrEntry *ctr_howlSpuAddrs asm("sdata_static+2160");
extern struct GarageFX ctr_garageSoundPool[8] asm("sdata_static+37752");
extern struct MemcardState ctr_memcardState asm(RETAIL_MEMCARD_STATE_ASM_NAME);
extern char ctr_memcardDirHeader[8] asm("sdata_static+1192");
// NOTE(aalhendi): Link these as plain small-data symbols so GCC emits the
// retail GP-relative event-handle loads before the stack frame is opened.
extern int ctr_memcardSwIOE;
extern int ctr_memcardHwIOE;
extern int ctr_memcardIconCrash[0x40] asm("data+19456");
extern int ctr_memcardIconGhost[0x40] asm("data+19712");
extern struct Bank ctr_audioBanks[8] asm("sdata_static+11456");
// NOTE(aalhendi): The lifecycle namespace resolves this byte and _gp together for GP-relative loads.
extern u8 ctr_audioBankCount;
extern char ctr_howlLoadSampleBlockName[] asm("rdata+4300");

#endif
