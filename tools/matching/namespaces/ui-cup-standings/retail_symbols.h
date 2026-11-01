#ifndef CTR_MATCHING_UI_CUP_STANDINGS_RETAIL_SYMBOLS_H
#define CTR_MATCHING_UI_CUP_STANDINGS_RETAIL_SYMBOLS_H

#include "../../retail_bindings.h"

// NOTE(aalhendi): This format is the existing nine-byte UI_RaceStart owner,
// not a second string definition in the Cup input/draw unit.
extern char ctr_cupTrackFormat[9] asm("rdata+5896");
#define GAME_CUP_TRACK_FORMAT ctr_cupTrackFormat

extern __typeof__(data.advCupTrackIDs) ctr_data_advCupTrackIDs asm("data+14084");
extern __typeof__(data.ArcadeCups) ctr_data_ArcadeCups asm("data+14248");
extern __typeof__(data.cupPositionPerPlayer) ctr_data_cupPositionPerPlayer asm("data+25508");
extern __typeof__(data.cupPointsPerPosition) ctr_data_cupPointsPerPosition asm("data+25540");

// NOTE(aalhendi): Compatible incomplete aliases name aggregate members, not
// independent small-data objects. The actual native arrays remain four s16s.
extern char ctr_sdata_s_additionInt[] asm("sdata_static+1544");
extern __typeof__(sdata_static.UnlockBitIndex.CupCompletion_curr[0]) ctr_cupCurrentAbsolute[] asm("sdata_static+1548");
extern __typeof__(sdata_static.UnlockBitIndex.CupCompletion_prev[0]) ctr_cupPreviousAbsolute[] asm("sdata_static+1556");
extern __typeof__(sdata_static.UnlockBitIndex.UnlockBattleMap[0]) ctr_cupBattleAbsolute[] asm("sdata_static+1564");
#define GAME_CUP_COMPLETION_CURRENT  ctr_cupCurrentAbsolute
#define GAME_CUP_COMPLETION_PREVIOUS ctr_cupPreviousAbsolute
#define GAME_CUP_BATTLE_MAP_UNLOCKS  ctr_cupBattleAbsolute

// NOTE(aalhendi): Keep these actual table roots distinct through their address
// phases; native source defaults use the canonical Data members directly.
#define GAME_CUP_LOAD_CHARACTER_METADATA(result)                                                       \
	do                                                                                                 \
	{                                                                                                  \
		void *page;                                                                                    \
		CTR_PSX_WITH_RETAIL_PAGE(page, 0x80080000u, 0x6d84, GAME_CHARACTER_METADATA, (result) = page); \
	} while (0)
#define GAME_CUP_LOAD_CHARACTER_IDS(result)                                                       \
	do                                                                                            \
	{                                                                                             \
		void *page;                                                                               \
		CTR_PSX_WITH_RETAIL_PAGE(page, 0x80080000u, 0x6e84, GAME_CHARACTER_IDS, (result) = page); \
	} while (0)

// NOTE(aalhendi): Index within the canonical GameTracker representation; no
// alternate layout is introduced. Native defaults retain ptrIcons indexing.
#define GAME_CUP_SELECT_DRIVER_ICON(tracker, iconID)                                 \
	({                                                                               \
		struct GameTracker *iconTracker = (tracker);                                 \
		s16 iconValue = (iconID);                                                    \
		s32 iconByteOffset = (s32)iconValue * (s32)sizeof(iconTracker->ptrIcons[0]); \
		u8 *iconBase = (u8 *)iconTracker;                                            \
		CTR_PSX_DEPEND_VALUE(iconByteOffset, iconValue);                             \
		iconBase += iconByteOffset;                                                  \
		*(struct Icon **)(iconBase + offsetof(struct GameTracker, ptrIcons));        \
	})

#endif
