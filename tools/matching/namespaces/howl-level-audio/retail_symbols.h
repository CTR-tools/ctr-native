#ifndef CTR_MATCHING_HOWL_LEVEL_AUDIO_RETAIL_SYMBOLS_H
#define CTR_MATCHING_HOWL_LEVEL_AUDIO_RETAIL_SYMBOLS_H

#include "../../retail_bindings.h"

#define GAME_LEVEL_AUDIO_TRACKER        ctr_gameTrackerPtr
#define GAME_LEVEL_AUDIO_FADES          ctr_levelAudioFades
#define GAME_LEVEL_AUDIO_RNG            ctr_levelAudioRng
#define GAME_LEVEL_AUDIO_AMBIENT_SOUNDS ctr_levelAudioAmbientSounds
#define GAME_LEVEL_AUDIO_INVALID_SPAWN  ctr_levelAudioInvalidSpawn

extern struct SoundFadeInput ctr_levelAudioFades[2] asm("sdata_static+37720");
extern u32 ctr_levelAudioRng asm("sdata_static+236");
extern u8 ctr_levelAudioAmbientSounds[0x84] asm("data+12268");
extern s32 ctr_levelAudioInvalidSpawn asm("2244($28)");

// Retail reloads the tracker page after scaling the selected camera index.
#define GAME_LEVEL_AUDIO_TRACKER_LOAD_AFTER(result, dependency)                                              \
	do                                                                                                       \
	{                                                                                                        \
		CTR_PSX_LOAD_SYMBOL_PAGE_AFTER(result, RETAIL_GAME_TRACKER_ASM_NAME, dependency);                    \
		CTR_PSX_LOAD_WORD_FROM_PAGE(result, result, RETAIL_GAME_TRACKER_ASM_NAME, GAME_LEVEL_AUDIO_TRACKER); \
	} while (0)
// These private MIPS address forms preserve the camera stride before the base.
// Ordinary native builds index the canonical cameraDC member in the source defaults.
#define GAME_LEVEL_AUDIO_CAMERA_ECHO(result, tracker, offset, index)                                                               \
	{                                                                                                                              \
		register struct GameTracker *indexedTracker CTR_PSX_REGISTER("$3") = (struct GameTracker *)((char *)(tracker) + (offset)); \
		(result) = indexedTracker->cameraDC[0].ptrQuadBlock->quadFlags;                                                            \
	}
#define GAME_LEVEL_AUDIO_CAMERA_QUAD_LOAD(result, tracker, offset, index)                                                          \
	{                                                                                                                              \
		register struct GameTracker *indexedTracker CTR_PSX_REGISTER("$3") = (struct GameTracker *)((char *)(tracker) + (offset)); \
		(result) = indexedTracker->cameraDC[0].ptrQuadBlock;                                                                       \
	}

#endif
