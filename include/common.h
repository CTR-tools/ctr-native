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
#define GAME_LANGUAGE_STRINGS        (sdata->lngStrings)
#define GAME_CHARACTER_METADATA      (data.MetaDataCharacters)
#define GAME_CHARACTER_IDS           (data.characterIDs)
#define GAME_LEVEL_METADATA           (data.metaDataLEV)
#define GAME_BOSS_WEAPON_METADATA      (data.bossWeaponMetaPtr)
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
