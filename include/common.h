#ifndef COMMON_H
#define COMMON_H

// Project base types and helpers.
#include <macros.h>
#include <psx/psx_prelude.h>

// Project-owned helpers layered on top of the PSX-shaped SDK headers.
#include <ctr_math.h>
#include <ctr_gte.h>
#include <ctr_scratchpad.h>
#include <prim.h>

#include <game_layouts.h>

// NOTE(aalhendi): Native and unbound PSX units use the canonical runtime
// aggregates. Matching supplies direct resident bindings before this header.
#ifndef GAME_TRACKER
#define GAME_TRACKER                 (sdata->gGT)
#define GAME_LOADING_STAGE           (sdata->Loading.stage)
#define GAME_LOAD_IN_PROGRESS        (sdata->load_inProgress)
#define GAME_LOAD_QUEUE_READY        (sdata->queueReady)
#define GAME_LOAD_VRAM_FINISHED      (sdata->frameFinishedVRAM)
#define GAME_LOAD_FILE_LABEL         (sdata->s_FILE)
#define GAME_LANGUAGE_FILE           (sdata->lngFile)
#define GAME_LANGUAGE_BUFFER_SIZE    (sdata->langBufferSize)
#define GAME_LANGUAGE_STRING_COUNT   (sdata->numLngStrings)
#define GAME_LANGUAGE_STRINGS        (sdata->lngStrings)
#define GAME_POSITION_SUFFIX_INDICES (data.stringIndexSuffix)
#define GAME_BATTLE_SCORE_COLORS     (data.battleScoreColor)
#define GAME_CHARACTER_METADATA      (data.MetaDataCharacters)
#define GAME_CHARACTER_IDS           (data.characterIDs)
#define GAME_2P_AI_SETS              (data.characterIDs_2P_AIs)
#define GAME_OVERLAY_CALLBACKS       (data.overlayCallbackFuncs)
#define GAME_LEVEL_BIG_LOD_INDEX     (sdata->levBigLodIndex)
#define GAME_LEVEL_METADATA          (data.metaDataLEV)
#define GAME_LEVEL_PREFIX_NDI        (sdata->s_ndi)
#define GAME_LEVEL_PREFIX_ENDING     (sdata->s_ending)
#define GAME_LEVEL_PREFIX_INTRO      (sdata->s_intro)
#define GAME_LEVEL_PREFIX_SCREEN     (sdata->s_screen)
#define GAME_LEVEL_PREFIX_GARAGE     (sdata->s_garage)
#define GAME_LEVEL_PREFIX_HUB        (sdata->s_hub)
#define GAME_LEVEL_PREFIX_CREDIT     (sdata->s_credit)
#define GAME_RDATA_NAME(name)        (rdata.name)
#define GAME_SDATA_NAME(name)        (sdata->name)
#define GAME_PRIM_MEM_SIZES_1P       (data.primMem_SizePerLEV_1P)
#define GAME_PRIM_MEM_SIZES_2P       (data.primMem_SizePerLEV_2P)
#define GAME_PRIM_MEM_SIZES_4P       (data.primMem_SizePerLEV_4P)
#define GAME_CLIP_BUFFERS            (data.PtrClipBuffer)
#define GAME_LEVEL_CLEAR_RED         (sdata->LevClearColorRGB[0])
#define GAME_LEVEL_CLEAR_GREEN       (sdata->LevClearColorRGB[1])
#define GAME_LEVEL_CLEAR_BLUE        (sdata->LevClearColorRGB[2])
#define GAME_XA_STATE                (sdata->XA_State)
#define GAME_BOSS_WEAPON_METADATA    (data.bossWeaponMetaPtr)
#define GAME_FRAMES_SINCE_RACE_ENDED (sdata->framesSinceRaceEnded)
#define GAME_MENU_READY              (sdata->menuReadyToPass)
#define GAME_ANY_PLAYER_TAP          (sdata->AnyPlayerTap)
#define GAME_ADV_PROGRESS            (sdata->advProgress)
#define GAME_ADV_RNG                 (sdata->advRng)
#define GAME_SAVE                    (sdata->gameSave)
#define GAME_PROGRESS                (GAME_SAVE.progress)
#define GAMEPADS                     (sdata->gGamepads)
#define GAME_MENU_HIGHLIGHT          (sdata->menuRowHighlight_Normal)
#define GAME_TOKEN                   (sdata->ptrToken)
#define GAME_ADD_CONFIG_0            (sdata->Loading.OnBegin.AddBitsConfig0)
#define GAME_REMOVE_CONFIG_0         (sdata->Loading.OnBegin.RemBitsConfig0)
#define GAME_DOOR_ACCESS_FLAGS       (sdata->doorAccessFlags)
#define GAME_DRIVER_MODEL_EXTRAS     (data.driverModelExtras)
#define GAME_PLAYER_OBJECT_LIST      (sdata->PLYROBJECTLIST)
#define GAME_SONG_SEQUENCES          (sdata->songSeq)
#define GAME_SONG_POOL               (sdata->songPool)
#define GAME_NOTE_FREQUENCY          (data.noteFrequency)
#define GAME_DISTORT_CONST_MUSIC     (data.distortConst_Music)
#define GAME_CHANNEL_UPDATE_FLAGS    (sdata->ChannelUpdateFlags)
#define GAME_CHANNEL_ATTR_NEW        (sdata->channelAttrNew)
#define GAME_VOLUME_LR               (data.volumeLR)
#define GAME_WRONG_WAY_DIRECTION     (sdata->WrongWayDirection_bool)
#define GAME_SAME_DIRECTION_FRAMES   (sdata->framesDrivingSameDirection)
#define GAME_DISTORT_CONST_OTHER_FX  (data.distortConst_OtherFX)
#define GAME_HOWL_REVERB_PARAMS      (data.reverbParams)
#define GAME_CHANNEL_TAKEN           (sdata->channelTaken)
#define GAME_CHANNEL_FREE            (sdata->channelFree)
#define GAME_HOWL_CD_FILE            (sdata->KartHWL_CdFile)
#define GAME_AUDIO_ENABLED           (sdata->boolAudioEnabled)
#define GAME_HOWL_BANK_OFFSETS       (sdata->howl_bankOffsets)
#define GAME_HOWL_HEADER             (sdata->ptrHowlHeader)
#define GAME_CSEQ_HEADER             (sdata->ptrCseqHeader)
#define GAME_HOWL_SPU_ADDRS          (sdata->howl_spuAddrs)
#define GAME_GARAGE_SOUND_POOL       (sdata->garageSoundPool)
#define GAME_MEMCARD_STATE           (sdata->memcard)
#define GAME_MEMCARD_DIR_HEADER      (sdata->s_memcardDirHeader)
#define GAME_MEMCARD_SW_IOE          (sdata->SwCARD_EvSpIOE)
#define GAME_MEMCARD_HW_IOE          (sdata->HwCARD_EvSpIOE)
#define GAME_MEMCARD_ICON_CRASH      (data.memcardIcon_CrashHead)
#define GAME_MEMCARD_ICON_GHOST      (data.memcardIcon_Ghost)
#define GAME_MEMCARD_ICON_HAND       (data.memcardIcon_PsyqHand)
#define GAME_AUDIO_BANKS             (sdata->bank)
#define GAME_AUDIO_BANK_COUNT        (sdata->numAudioBanks)
#define GAME_HOWL_SAMPLE_BLOCK_NAME  (rdata.s_LoadSampleBlock)
#endif

#ifndef GAME_MATRIX_TABLE_BAKED
#define GAME_MATRIX_TABLE_BAKED (sdata->matrixTableBaked)
#endif

#ifndef GAME_TRAFFIC_LIGHT_PREVIOUS_TIMER
#define GAME_TRAFFIC_LIGHT_PREVIOUS_TIMER (sdata->trafficLightsTimer_prevFrame)
#endif

#ifndef GAME_HUD_LAYOUTS
#define GAME_HUD_LAYOUTS (data.hudStructPtr)
#endif

#ifndef GAME_LAP_MINUTE_UNITS
#define GAME_LAP_MINUTE_UNITS      (sdata->LapTimes.minuteUnits)
#define GAME_LAP_CENTISECOND_TENS  (sdata->LapTimes.centisecondTens)
#define GAME_LAP_CENTISECOND_UNITS (sdata->LapTimes.centisecondUnits)
#define GAME_LAP_SECOND_TENS       (sdata->LapTimes.secondTens)
#define GAME_LAP_SECOND_UNITS      (sdata->LapTimes.secondUnits)
#endif

#ifndef GAME_WUMPA_HUD_VIEWPORTS
#define GAME_WUMPA_HUD_VIEWPORTS (sdata->multiplayerWumpaHudData)
#endif

#ifndef GAME_MUSIC_PLAYING_INITIAL
#define GAME_MUSIC_PLAYING_INITIAL (sdata->cseqBoolPlay)
#endif

#ifndef GAME_MUSIC_LOAD_STAGE_INITIAL
#define GAME_MUSIC_LOAD_STAGE_INITIAL (((u8 *)&sdata->audioDefaults[7])[1])
#endif

#ifndef GAME_LEVEL_FX_BANKS
#define GAME_LEVEL_FX_BANKS (data.levBank_FX)
#endif
#ifndef GAME_LEVEL_SONG_BANKS
#define GAME_LEVEL_SONG_BANKS (data.levBank_Song)
#endif
#ifndef GAME_BOSS_FX_BANKS
#define GAME_BOSS_FX_BANKS (sdata->songBankBossID)
#endif

#ifndef GAME_DESIRED_MENU
#define GAME_DESIRED_MENU (sdata->ptrDesiredMenu)
#endif
#ifndef GAME_SPU_DECODED_BUFFER
#define GAME_SPU_DECODED_BUFFER (sdata->SpuDecodedBuf)
#endif
#ifndef GAME_CD_READY_POSITION
#define GAME_CD_READY_POSITION (sdata->cdlFile_CdReady)
#endif
#ifndef GAME_XA_MAX_SAMPLE_HISTORY
#define GAME_XA_MAX_SAMPLE_HISTORY (sdata->XA_MaxSampleValArr)
#endif
#ifndef GAME_XA_CURRENT_OFFSET
#define GAME_XA_CURRENT_OFFSET (sdata->XA_CurrOffset)
#endif
#ifndef GAME_XA_IRQ_ADDRESS
#define GAME_XA_IRQ_ADDRESS (sdata->irqAddr)
#endif
#ifndef GAME_XA_MAX_SAMPLE
#define GAME_XA_MAX_SAMPLE (sdata->XA_MaxSampleVal)
#endif
#ifndef GAME_XA_HISTORY_MAX
#define GAME_XA_HISTORY_MAX (sdata->XA_MaxSampleValInArr)
#endif
#ifndef GAME_XA_MAX_SAMPLE_INDEX
#define GAME_XA_MAX_SAMPLE_INDEX (sdata->XA_MaxSampleIndex)
#endif
#ifndef GAME_XA_SAMPLE_COUNT
#define GAME_XA_SAMPLE_COUNT (sdata->XA_MaxSampleNumSaved)
#endif
#ifndef GAME_MAIN_MENU_STATE
#define GAME_MAIN_MENU_STATE (sdata->mainMenuState)
#endif
#ifndef GAME_FRAME_COUNTER_LOW
#define GAME_FRAME_COUNTER_LOW ((u16)sdata->frameCounter)
#endif
#ifndef GAME_RCNT_TOTAL_UNITS
#define GAME_RCNT_TOTAL_UNITS (sdata->rcntTotalUnits)
#endif
#ifndef GAME_BANK_LOAD_STAGE_INITIAL
#define GAME_BANK_LOAD_STAGE_INITIAL (sdata->bankLoadStage)
#endif

#ifndef GAME_BATTLE_CAMERA_HEIGHTS
#define GAME_BATTLE_CAMERA_HEIGHTS (data.Spin360_heightOffset_cameraPos)
#endif

#ifndef GAME_SPIN360_DRIVER_HEIGHTS
#define GAME_SPIN360_DRIVER_HEIGHTS (data.Spin360_heightOffset_driverPos)
#endif

#ifndef GAME_CAMERA_THREAD_NAME
#define GAME_CAMERA_THREAD_NAME (sdata->s_camera)
#endif

// NOTE(aalhendi): Collision work lives in resident RAM, not PSX hardware scratchpad.
#ifndef GAME_COLLISION_WORK
#define GAME_COLLISION_WORK (sdata->scratchpadStruct)
#endif

#ifndef GAME_AUDIO_BANK_COUNT_LOAD_AFTER
#define GAME_AUDIO_BANK_COUNT_LOAD_AFTER(result, dependency) \
	do                                                       \
	{                                                        \
		(result) = GAME_AUDIO_BANK_COUNT;                    \
		(void)(dependency);                                  \
	} while (0)
#endif

// NOTE(aalhendi): Retail sometimes rereads the pointer slot rather than reusing
// a cached tracker. Qualify the access, not the shared declaration.
#define GAME_TRACKER_RELOAD() (*(struct GameTracker *volatile *)&GAME_TRACKER)

#if defined(CTR_NATIVE)
#include <platform.h>
#endif

// Game declarations and GPU helpers that depend on the layout headers above.
#include <functions.h>
#include <gpu.h>

#if defined(CTR_NATIVE)
static inline void *CTR_PsyqMemmove(void *dest, const void *src, s32 count)
{
	// NOTE(aalhendi): Retail PSYQ memmove at 0x80077e38 returns
	// immediately for signed lengths <= 0. Host libc takes u32, so native
	// must preserve the signed PSYQ contract for retail-shaped game code.
	if (count <= 0)
	{
		return dest;
	}

	return memmove(dest, src, (u32)count);
}

#define memmove(dest, src, count) CTR_PsyqMemmove((dest), (src), (s32)(count))
#endif

#endif
