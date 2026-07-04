#ifndef CTR_NATIVE_NAMESPACE_MEMCARD_H
#define CTR_NATIVE_NAMESPACE_MEMCARD_H

// Modified from original game
enum MC_STAGE
{
	MC_STAGE_IDLE = 0,
	MC_STAGE_GETINFO = 1,
	MC_STAGE_NEWCARD,

	MC_STAGE_LOAD_PART0_START,
	// MC_STAGE_LOAD_PART1_HEADER = 3,
	MC_STAGE_LOAD_PART2_READ,
	MC_STAGE_LOAD_PART3_CHECK,
	MC_STAGE_LOAD_PART4_READ,
	MC_STAGE_LOAD_PART5_CHECK,

	MC_STAGE_SAVE_PART0_START = 9,
	MC_STAGE_SAVE_PART1_ICON,
	MC_STAGE_SAVE_PART2_WRITE,
	MC_STAGE_SAVE_PART3_DONE,

	MC_STAGE_ERASE_FAIL = 13,
	MC_STAGE_ERASE_PASS = 14,

	MC_STAGE_GHOST_FOUND = 15,
};

enum MC_RETURN
{
	MC_RETURN_IOE = 0,
	MC_RETURN_TIMEOUT = 1,
	MC_RETURN_NOCARD = 2,
	MC_RETURN_NEWCARD = 3,
	MC_RETURN_FULL = 4,
	MC_RETURN_UNFORMATTED = 5,
	MC_RETURN_NODATA = 6,
	MC_RETURN_PENDING = 7,
};

enum MEMCARD_LOAD_FLAGS
{
	MEMCARD_LOAD_SYNC_CHECKSUM = 0x2,
};

enum MEMCARD_SAVE_FLAGS
{
	MEMCARD_SAVE_FORCE_BACKUP_COPY = 0x1,
};

enum MEMCARD_STATUS_FLAGS
{
	MEMCARD_STATUS_NO_BACKUP_COPY = 0x4,
	MEMCARD_STATUS_SYNC_CHECKSUM = 0x8,
};

enum MC_RESULT
{
	MC_RESULT_ERROR_NOCARD = 0,
	MC_RESULT_FULL = 1,
	MC_RESULT_ERROR_TIMEOUT = 2,
	MC_RESULT_NEWCARD = 3,
	MC_RESULT_READY_LOAD = 4,
	MC_RESULT_ERROR_NODATA = 5,
	MC_RESULT_ERROR_UNFORMATTED = 6,
	MC_RESULT_READY_SAVE = 7,
	MC_RESULT_PENDING = 8
};

enum MC_ACTION
{
	MC_ACTION_GetInfo = 1,
	MC_ACTION_Save = 2,
	MC_ACTION_Load = 3,
	MC_ACTION_Format = 4,
	MC_ACTION_Erase = 5,
};

enum MC_SCREEN
{
	MC_SCREEN_WARNING_NOCARD = 0,
	MC_SCREEN_WARNING_UNFORMATTED = 1,
	MC_SCREEN_FORMATTING = 2,
	MC_SCREEN_SAVING = 3,
	MC_SCREEN_LOADING = 4,
	MC_SCREEN_CHECKING = 5,
	MC_SCREEN_ERROR_FULL = 6,
	MC_SCREEN_ERROR_TIMEOUT = 7,
	MC_SCREEN_NULL = 8,
	MC_SCREEN_ERROR_NODATA = 9,
};

enum HighScoreConstants
{
	MEMCARD_HIGH_SCORE_TRACK_COUNT = 0x12,
	MEMCARD_HIGH_SCORE_MODE_COUNT = 2,
	MEMCARD_HIGH_SCORE_ENTRIES_PER_MODE = 6,
	MEMCARD_HIGH_SCORE_ENTRIES_PER_TRACK = MEMCARD_HIGH_SCORE_MODE_COUNT * MEMCARD_HIGH_SCORE_ENTRIES_PER_MODE,
	MEMCARD_HIGH_SCORE_NAME_LENGTH = 18,
	MEMCARD_HIGH_SCORE_DEFAULT_TIME = 0x8c640,
};

enum GameProgressConstants
{
	GAME_PROGRESS_UNLOCK_WORD_COUNT = 2,
	GAME_PROGRESS_CUP_COUNT = 4,
	GAME_PROGRESS_CUP_DIFFICULTY_COUNT = 3,
	GAME_PROGRESS_CUP_WIN_COUNT = GAME_PROGRESS_CUP_COUNT * GAME_PROGRESS_CUP_DIFFICULTY_COUNT,
	GAME_PROGRESS_CUP_CURRENT_WIN_FIRST_BIT = 0x0c,
	GAME_PROGRESS_CUP_PREVIOUS_WIN_OFFSET = GAME_PROGRESS_CUP_WIN_COUNT,
	ADV_OXIDE_FINAL_RELIC_COUNT = 18,
	MEMCARD_ADV_PROFILE_COUNT = 4,
	MEMCARD_PROFILE_VERSION = -18,
};

// 0x18 (24) bytes each
struct HighScoreEntry
{
	u32 time;
	char name[MEMCARD_HIGH_SCORE_NAME_LENGTH];
	u16 characterID;
};

// 0x124 bytes each
struct HighScoreTrack
{
	// Time Trial Best Lap
	// Time Trial Best Race (5)
	// Relic Race Best Lap -- unused
	// Relic Race Best Race (5)
	struct HighScoreEntry scoreEntry[MEMCARD_HIGH_SCORE_ENTRIES_PER_TRACK];

#define TT_NTROPY_OPEN   0x1
#define TT_NTROPY_BEATEN 0x2
#define TT_NOXIDE_BEATEN 0x4

	u32 timeTrialFlags;
};

struct GameProgress
{
	// FUN_80026bf0 shows a memset on a structure
	// which must be this structure here

	// 8008e6e8
	u32 unknown;

	// 8008e6ec - UsaRetail
	// 8008eaa0 - EurRetail
	// characters, tracks, cups, scrapbook
	union
	{
		u32 unlocks[GAME_PROGRESS_UNLOCK_WORD_COUNT];
		struct
		{
			u32 unlockFlags;
			u32 extendedUnlockFlags;
		};
	};

	// 8008e6f4 -- 0x1488 bytes large
	struct HighScoreTrack highScoreTracks[MEMCARD_HIGH_SCORE_TRACK_COUNT];
};

#define MEMCARD_BIT_WORD(bitIndex) ((bitIndex) >> 5)
#define MEMCARD_BIT_MASK(bitIndex) (1u << ((bitIndex) & 0x1f))

enum GameProgressUnlockBitIndex
{
	GAME_UNLOCK_BIT_TURBO_TRACK = 1,
	GAME_UNLOCK_BIT_PARKING_LOT = 2,
	GAME_UNLOCK_BIT_NORTH_BOWL = 3,
	GAME_UNLOCK_BIT_LAB_BASEMENT = 4,
	GAME_UNLOCK_BIT_BOSS_CHARACTER_FIRST = 7,
	GAME_UNLOCK_BIT_SCRAPBOOK = 36,
};

enum GameProgressUnlockMask
{
	GAME_UNLOCK_TURBO_TRACK_MASK = MEMCARD_BIT_MASK(GAME_UNLOCK_BIT_TURBO_TRACK),
	GAME_UNLOCK_TRACKS_MASK = MEMCARD_BIT_MASK(GAME_UNLOCK_BIT_TURBO_TRACK) | MEMCARD_BIT_MASK(GAME_UNLOCK_BIT_PARKING_LOT) |
	                          MEMCARD_BIT_MASK(GAME_UNLOCK_BIT_NORTH_BOWL) | MEMCARD_BIT_MASK(GAME_UNLOCK_BIT_LAB_BASEMENT),
};

enum AdvMaskHintId
{
	ADV_MASK_HINT_ID_WELCOME_TO_ARENA = 0,
	ADV_MASK_HINT_ID_USING_WARP_PAD = 1,
	ADV_MASK_HINT_ID_NEED_MORE_TROPHIES = 2,
	ADV_MASK_HINT_ID_NEED_FOUR_TROPHIES_FOR_BOSS = 3,
	ADV_MASK_HINT_ID_NEED_FOUR_KEYS_FOR_OXIDE = 4,
	ADV_MASK_HINT_ID_MUST_HAVE_ONE_BOSS_KEY = 5,
	ADV_MASK_HINT_ID_SAVE_LOAD_SCREEN = 6,
	ADV_MASK_HINT_ID_NEW_WORLD_GREETING = 7,
	ADV_MASK_HINT_ID_TROPHY_AWARDED = 0x0c,
	ADV_MASK_HINT_ID_KEY_AWARDED = 0x0d,
	ADV_MASK_HINT_ID_HANG_TIME_TURBO = 0x0e,
	ADV_MASK_HINT_ID_POWER_SLIDE = 0x0f,
	ADV_MASK_HINT_ID_TURBO_BOOST = 0x10,
	ADV_MASK_HINT_ID_BRAKE_SLIDE = 0x11,
	ADV_MASK_HINT_ID_MUST_HAVE_TWO_BOSS_KEYS = 0x12,
	ADV_MASK_HINT_ID_RELIC_AWARDED = 0x13,
	ADV_MASK_HINT_ID_CTR_TOKEN_AWARDED = 0x14,
	ADV_MASK_HINT_ID_GEM_AWARDED = 0x15,
	ADV_MASK_HINT_ID_WUMPA_FRUIT = 0x16,
	ADV_MASK_HINT_ID_TNT = 0x17,
	ADV_MASK_HINT_ID_MAP_INFORMATION = 0x18,
	ADV_MASK_HINT_ID_COLLECT_EVERY_CRYSTAL = 0x19,
	ADV_MASK_HINT_ID_CTR_TOKEN_CHALLENGE = 0x1a,
	ADV_MASK_HINT_ID_GEM_CUPS_CHALLENGE = 0x1b,
	ADV_MASK_HINT_ID_MUST_GET_10_RELICS = 0x1c,
	ADV_MASK_HINT_ID_RELIC_CHALLENGE = 0x1d,
	ADV_MASK_HINT_UKA_UKA_XA_OFFSET = 0x1f,
};

enum AdvRewardBitIndex
{
	ADV_REWARD_FIRST_TROPHY = 0x06,
	ADV_REWARD_FIRST_SAPPHIRE_RELIC = 0x16,
	ADV_REWARD_FIRST_GOLD_RELIC = 0x28,
	ADV_REWARD_FIRST_PLATINUM_RELIC = 0x3a,
	ADV_REWARD_FIRST_CTR_TOKEN = 0x4c,
	ADV_REWARD_BOSS_KEY_HUB_ID_BASE = 0x5d,
	ADV_REWARD_FIRST_BOSS_KEY = 0x5e,
	ADV_REWARD_BEAT_OXIDE_FIRST_BOSS = ADV_REWARD_FIRST_BOSS_KEY + 4,
	ADV_REWARD_BEAT_OXIDE_SECOND_BOSS = ADV_REWARD_FIRST_BOSS_KEY + 5,
	ADV_REWARD_DOOR_BEACH_TO_GEMSTONE_VALLEY = 0x64,
	ADV_REWARD_DOOR_GEMSTONE_VALLEY_TO_CUPS = 0x65,
	ADV_REWARD_DOOR_BEACH_TO_GLACIER_PARK = 0x66,
	ADV_REWARD_DOOR_LOST_RUINS_TO_GLACIER_PARK = 0x67,
	ADV_REWARD_DOOR_GLACIER_PARK_TO_CITADEL_CITY = 0x68,
	ADV_REWARD_FIRST_GEM = 0x6a,
	ADV_REWARD_PURPLE_TOKEN_HUB_ID_BASE = 0x6e,
	ADV_REWARD_FIRST_PURPLE_TOKEN = 0x6f,
	ADV_REWARD_BEAT_OXIDE_FIRST = 0x73,
	ADV_REWARD_BEAT_OXIDE_SECOND = 0x74,
	ADV_REWARD_FIRST_HINT = 0x76,
	ADV_REWARD_HINT_WELCOME_TO_ARENA = ADV_REWARD_FIRST_HINT + ADV_MASK_HINT_ID_WELCOME_TO_ARENA,
	ADV_REWARD_HINT_USING_WARP_PAD = ADV_REWARD_FIRST_HINT + ADV_MASK_HINT_ID_USING_WARP_PAD,
	ADV_REWARD_HINT_NEED_MORE_TROPHIES = ADV_REWARD_FIRST_HINT + ADV_MASK_HINT_ID_NEED_MORE_TROPHIES,
	ADV_REWARD_HINT_NEED_FOUR_TROPHIES_FOR_BOSS = ADV_REWARD_FIRST_HINT + ADV_MASK_HINT_ID_NEED_FOUR_TROPHIES_FOR_BOSS,
	ADV_REWARD_HINT_NEED_FOUR_KEYS_FOR_OXIDE = ADV_REWARD_FIRST_HINT + ADV_MASK_HINT_ID_NEED_FOUR_KEYS_FOR_OXIDE,
	ADV_REWARD_HINT_MUST_HAVE_ONE_BOSS_KEY = ADV_REWARD_FIRST_HINT + ADV_MASK_HINT_ID_MUST_HAVE_ONE_BOSS_KEY,
	ADV_REWARD_HINT_SAVE_LOAD_SCREEN = ADV_REWARD_FIRST_HINT + ADV_MASK_HINT_ID_SAVE_LOAD_SCREEN,
	ADV_REWARD_HINT_NEW_WORLD_GREETING = ADV_REWARD_FIRST_HINT + ADV_MASK_HINT_ID_NEW_WORLD_GREETING,
	ADV_REWARD_HINT_HANG_TIME_TURBO = ADV_REWARD_FIRST_HINT + ADV_MASK_HINT_ID_HANG_TIME_TURBO,
	ADV_REWARD_HINT_POWER_SLIDE = ADV_REWARD_FIRST_HINT + ADV_MASK_HINT_ID_POWER_SLIDE,
	ADV_REWARD_HINT_TURBO_BOOST = ADV_REWARD_FIRST_HINT + ADV_MASK_HINT_ID_TURBO_BOOST,
	ADV_REWARD_HINT_BRAKE_SLIDE = ADV_REWARD_FIRST_HINT + ADV_MASK_HINT_ID_BRAKE_SLIDE,
	ADV_REWARD_HINT_MUST_HAVE_TWO_BOSS_KEYS = ADV_REWARD_FIRST_HINT + ADV_MASK_HINT_ID_MUST_HAVE_TWO_BOSS_KEYS,
	ADV_REWARD_HINT_WUMPA_FRUIT = ADV_REWARD_FIRST_HINT + ADV_MASK_HINT_ID_WUMPA_FRUIT,
	ADV_REWARD_HINT_TNT = ADV_REWARD_FIRST_HINT + ADV_MASK_HINT_ID_TNT,
	ADV_REWARD_HINT_MAP_INFORMATION = ADV_REWARD_FIRST_HINT + ADV_MASK_HINT_ID_MAP_INFORMATION,
	ADV_REWARD_HINT_COLLECT_EVERY_CRYSTAL = ADV_REWARD_FIRST_HINT + ADV_MASK_HINT_ID_COLLECT_EVERY_CRYSTAL,
	ADV_REWARD_HINT_CTR_TOKEN_CHALLENGE = ADV_REWARD_FIRST_HINT + ADV_MASK_HINT_ID_CTR_TOKEN_CHALLENGE,
	ADV_REWARD_HINT_GEM_CUPS_CHALLENGE = ADV_REWARD_FIRST_HINT + ADV_MASK_HINT_ID_GEM_CUPS_CHALLENGE,
	ADV_REWARD_HINT_MUST_GET_10_RELICS = ADV_REWARD_FIRST_HINT + ADV_MASK_HINT_ID_MUST_GET_10_RELICS,
	ADV_REWARD_HINT_RELIC_CHALLENGE = ADV_REWARD_FIRST_HINT + ADV_MASK_HINT_ID_RELIC_CHALLENGE,
};

enum AdvRewardConstants
{
	ADV_REWARD_HUB_COUNT = 4,
	ADV_REWARD_RACE_TRACK_COUNT = 0x10,
	ADV_REWARD_TROPHY_TRACK_COUNT = ADV_REWARD_RACE_TRACK_COUNT,
	ADV_REWARD_CTR_TOKEN_TRACK_COUNT = ADV_REWARD_RACE_TRACK_COUNT,
	ADV_REWARD_RELIC_TRACK_COUNT = 0x12,
	ADV_REWARD_RELIC_TIER_STRIDE = 0x12,
	ADV_REWARD_BOSS_KEY_COUNT = ADV_REWARD_HUB_COUNT,
	ADV_REWARD_PURPLE_TOKEN_COUNT = ADV_REWARD_HUB_COUNT,
	ADV_REWARD_GEM_COUNT = 5,
	ADV_REWARD_OXIDE_BEAT_COUNT = 2,
};

enum AdvRewardWordMask
{
	ADV_REWARD_BEAT_OXIDE_FIRST_BOSS_MASK = MEMCARD_BIT_MASK(ADV_REWARD_BEAT_OXIDE_FIRST_BOSS),
	ADV_REWARD_BEAT_OXIDE_SECOND_BOSS_MASK = MEMCARD_BIT_MASK(ADV_REWARD_BEAT_OXIDE_SECOND_BOSS),
	ADV_REWARD_BEAT_OXIDE_FIRST_MASK = MEMCARD_BIT_MASK(ADV_REWARD_BEAT_OXIDE_FIRST),
	ADV_REWARD_BEAT_OXIDE_SECOND_MASK = MEMCARD_BIT_MASK(ADV_REWARD_BEAT_OXIDE_SECOND),
	ADV_REWARD_DOOR_BEACH_TO_GEMSTONE_VALLEY_MASK = MEMCARD_BIT_MASK(ADV_REWARD_DOOR_BEACH_TO_GEMSTONE_VALLEY),
	ADV_REWARD_DOOR_GEMSTONE_VALLEY_TO_CUPS_MASK = MEMCARD_BIT_MASK(ADV_REWARD_DOOR_GEMSTONE_VALLEY_TO_CUPS),
	ADV_REWARD_DOOR_BEACH_TO_GLACIER_PARK_MASK = MEMCARD_BIT_MASK(ADV_REWARD_DOOR_BEACH_TO_GLACIER_PARK),
	ADV_REWARD_DOOR_LOST_RUINS_TO_GLACIER_PARK_MASK = MEMCARD_BIT_MASK(ADV_REWARD_DOOR_LOST_RUINS_TO_GLACIER_PARK),
	ADV_REWARD_DOOR_GLACIER_PARK_TO_CITADEL_CITY_MASK = MEMCARD_BIT_MASK(ADV_REWARD_DOOR_GLACIER_PARK_TO_CITADEL_CITY),
	ADV_REWARD_DOORS_TO_GLACIER_PARK_MASK = ADV_REWARD_DOOR_BEACH_TO_GLACIER_PARK_MASK | ADV_REWARD_DOOR_LOST_RUINS_TO_GLACIER_PARK_MASK,
	ADV_REWARD_HINT_WELCOME_TO_ARENA_MASK = MEMCARD_BIT_MASK(ADV_REWARD_HINT_WELCOME_TO_ARENA),
	ADV_REWARD_HINT_USING_WARP_PAD_MASK = MEMCARD_BIT_MASK(ADV_REWARD_HINT_USING_WARP_PAD),
	ADV_REWARD_HINT_NEED_MORE_TROPHIES_MASK = MEMCARD_BIT_MASK(ADV_REWARD_HINT_NEED_MORE_TROPHIES),
	ADV_REWARD_HINT_NEED_FOUR_TROPHIES_FOR_BOSS_MASK = MEMCARD_BIT_MASK(ADV_REWARD_HINT_NEED_FOUR_TROPHIES_FOR_BOSS),
	ADV_REWARD_HINT_NEED_FOUR_KEYS_FOR_OXIDE_MASK = MEMCARD_BIT_MASK(ADV_REWARD_HINT_NEED_FOUR_KEYS_FOR_OXIDE),
	ADV_REWARD_HINT_MUST_HAVE_ONE_BOSS_KEY_MASK = MEMCARD_BIT_MASK(ADV_REWARD_HINT_MUST_HAVE_ONE_BOSS_KEY),
	ADV_REWARD_HINT_SAVE_LOAD_SCREEN_MASK = MEMCARD_BIT_MASK(ADV_REWARD_HINT_SAVE_LOAD_SCREEN),
	ADV_REWARD_HINT_NEW_WORLD_GREETING_MASK = MEMCARD_BIT_MASK(ADV_REWARD_HINT_NEW_WORLD_GREETING),
	ADV_REWARD_HINT_HANG_TIME_TURBO_MASK = MEMCARD_BIT_MASK(ADV_REWARD_HINT_HANG_TIME_TURBO),
	ADV_REWARD_HINT_POWER_SLIDE_MASK = MEMCARD_BIT_MASK(ADV_REWARD_HINT_POWER_SLIDE),
	ADV_REWARD_HINT_TURBO_BOOST_MASK = MEMCARD_BIT_MASK(ADV_REWARD_HINT_TURBO_BOOST),
	ADV_REWARD_HINT_BRAKE_SLIDE_MASK = MEMCARD_BIT_MASK(ADV_REWARD_HINT_BRAKE_SLIDE),
	ADV_REWARD_HINT_MUST_HAVE_TWO_BOSS_KEYS_MASK = MEMCARD_BIT_MASK(ADV_REWARD_HINT_MUST_HAVE_TWO_BOSS_KEYS),
	ADV_REWARD_HINT_WUMPA_FRUIT_MASK = MEMCARD_BIT_MASK(ADV_REWARD_HINT_WUMPA_FRUIT),
	ADV_REWARD_HINT_TNT_MASK = MEMCARD_BIT_MASK(ADV_REWARD_HINT_TNT),
	ADV_REWARD_HINT_MAP_INFORMATION_MASK = MEMCARD_BIT_MASK(ADV_REWARD_HINT_MAP_INFORMATION),
	ADV_REWARD_HINT_COLLECT_EVERY_CRYSTAL_MASK = MEMCARD_BIT_MASK(ADV_REWARD_HINT_COLLECT_EVERY_CRYSTAL),
	ADV_REWARD_HINT_CTR_TOKEN_CHALLENGE_MASK = MEMCARD_BIT_MASK(ADV_REWARD_HINT_CTR_TOKEN_CHALLENGE),
	ADV_REWARD_HINT_GEM_CUPS_CHALLENGE_MASK = MEMCARD_BIT_MASK(ADV_REWARD_HINT_GEM_CUPS_CHALLENGE),
	ADV_REWARD_HINT_MUST_GET_10_RELICS_MASK = MEMCARD_BIT_MASK(ADV_REWARD_HINT_MUST_GET_10_RELICS),
	ADV_REWARD_HINT_RELIC_CHALLENGE_MASK = MEMCARD_BIT_MASK(ADV_REWARD_HINT_RELIC_CHALLENGE),
	ADV_REWARD_OXIDE_FIRST_WIN_FLAGS = ADV_REWARD_BEAT_OXIDE_FIRST_BOSS_MASK | ADV_REWARD_BEAT_OXIDE_FIRST_MASK,
	ADV_REWARD_OXIDE_SECOND_WIN_FLAGS = ADV_REWARD_BEAT_OXIDE_SECOND_BOSS_MASK | ADV_REWARD_BEAT_OXIDE_SECOND_MASK,
};

#define CHECK_MEMCARD_BIT(words, bitIndex)  (((words)[MEMCARD_BIT_WORD(bitIndex)] >> ((bitIndex) & 0x1f)) & 1)
#define UNLOCK_MEMCARD_BIT(words, bitIndex) ((words)[MEMCARD_BIT_WORD(bitIndex)] |= MEMCARD_BIT_MASK(bitIndex))

#define CHECK_ADV_BIT(rewards, bitIndex)    CHECK_MEMCARD_BIT(rewards, bitIndex)
#define UNLOCK_ADV_BIT(rewards, bitIndex)   UNLOCK_MEMCARD_BIT(rewards, bitIndex)

struct AdvProgress
{
	// 8008fba4
	union
	{
		u32 rewards[6];
		struct
		{
			u32 trophySapphireRelicFlags;
			u32 sapphireGoldRelicFlags;
			u32 platinumCtrTokenFlags;
			u32 storyFlags;
			u32 hintFlags;
			u32 reservedRewardFlags;
		};
	};

	/*
	    // 0x00:
	        0x1: ???
	        0x2: ???
	        0x4: ???
	        0x8: ???
	        0x10: ???
	        0x20: ???
	        0x40: Dingo Canyon Trophy
	        0x80: Dragon Mines Trophy
	        0x100: Blizzard Bluff Trophy
	        0x200: Crash Cove Trophy
	        0x400: Tiger Temple Trophy
	        0x800: Papu Pyramid Trophy
	        0x1000: Roos Tubes Trophy
	        0x2000: Hot Air Skyway Trophy
	        0x4000: Sewer Speedway Trophy
	        0x8000: Mystery Caves Trophy
	        0x10000: Cortex Castle Trophy
	        0x20000: N Gin Labs Trophy
	        0x40000: Polar Pass Trophy
	        0x80000: Oxide Station Trophy
	        0x100000: Coco Park Trophy
	        0x200000: Tiny Arena Trophy
	        0x400000: Dingo Canyon Sapphire Relic
	        0x800000: Dragon Mines Sapphire Relic
	        0x1000000: Blizzard Bluff Sapphire Relic
	        0x2000000: Crash Cove Sapphire Relic
	        0x4000000: Tiger Temple Sapphire Relic
	        0x8000000: Papu Pyramid Sapphire Relic
	        0x10000000: Roo Tubes Sapphire Relic
	        0x20000000: Hot Air Skyway Sapphire Relic
	        0x40000000: Sewer Speedway Sapphire Relic
	        0x80000000: Mystery Caves Sapphire Relic

	    // 0x04:
	        0x1: Cortex Castle Sapphire Relic
	        0x2: N Gin Labs Sapphire Relic
	        0x4: Polar Pass Sapphire Relic
	        0x8: Oxide Station Sapphire Relic
	        0x10: Coco Park Sapphire Relic
	        0x20: Tiny Arena Sapphire Relic
	        0x40: Slide Coliseum Sapphire Relic
	        0x80: Turbo Track Sapphire Relic
	        0x100: Dingo Canyon Gold Relic
	        0x200: Dragon Mines Gold Relic
	        0x400: Blizzard Bluff Gold Relic
	        0x800: Crash Cove Gold Relic
	        0x1000: Tiger Temple Gold Relic
	        0x2000: Papu Pyramid Gold Relic
	        0x4000: Roo Tubes Gold Relic
	        0x8000: Hot Air Skyway Gold Relic
	        0x10000: Sewer Speedway Gold Relic
	        0x20000: Mystery Caves Gold Relic
	        0x40000: Cortex Castle Gold Relic
	        0x80000: N Gin Labs Gold Relic
	        0x100000: Polar Pass Gold Relic
	        0x200000: Oxide Station Gold Relic
	        0x400000: Coco Park Gold Relic
	        0x800000: Tiny Arena Gold Relic
	        0x1000000: Slide Coliseum Gold Relic
	        0x2000000: Turbo Track Gold Relic
	        0x4000000: Dingo Canyon Platinum Relic
	        0x8000000: Dragon Mines Platinum Relic
	        0x10000000: Blizzard Bluff Platinum Relic
	        0x20000000: Crash Cove Platinum Relic
	        0x40000000: Tiger Temple Platinum Relic
	        0x80000000: Papu Pyramid Platinum Relic

	    // 0x08:
	        0x1: Roos Tubes Platinum Relic
	        0x2: Hot Air Skyway Platinum Relic
	        0x4: Sewer Speedway Platinum Relic
	        0x8: Mystery Caves Platinum Relic
	        0x10: Cortex Castle Platinum Relic
	        0x20: N Gin Labs Platinum Relic
	        0x40: Polar Pass Platinum Relic
	        0x80: Oxide Station Platinum Relic
	        0x100: Coco Park Platinum Relic
	        0x200: Tiny Arena Platinum Relic
	        0x400: Slide Coliseum Platinum Relic
	        0x800: Turbo Track Platinum Relic
	        0x1000: Dingo Canyon Yellow CTR Token
	        0x2000: Dragon Mines Blue CTR Token
	        0x4000: Blizzard Bluff Red CTR Token
	        0x8000: Crash Cove Red CTR Token
	        0x10000: Tiger Temple Blue CTR Token
	        0x20000: Papu Pyramid Red CTR Token
	        0x40000: Roo Tubes Green CTR Token
	        0x80000: Hot Air Skyway Yellow CTR Token
	        0x100000: Sewer Speedway Blue CTR Token
	        0x200000: Mystery Caves Red CTR Token
	        0x400000: Cortex Castle Green CTR Token
	        0x800000: N Gin Labs Blue CTR Token
	        0x1000000: Polar Pass Green CTR Token
	        0x2000000: Oxide Station Yellow CTR Token
	        0x4000000: Coco Park Green CTR Token
	        0x8000000: Tiny Arena Yellow CTR Token
	        0x10000000: No known source use
	        0x20000000: No known source use
	        0x40000000: Ripper Roo Key
	        0x80000000: Lost Ruins Key

	    // 0x0C:
	        0x1: Glacier Park Key
	        0x2: Citadel Key
	        0x4: Beat Oxide first challenge boss clear
	        0x8: Beat Oxide second challenge boss clear
	        0x10: Beach to Gemstone Valley door
	        0x20: Gemstone Valley to cups
	        0x40: Beach to Glacier Park
	        0x80: Lost Ruins to Glacier Park
	        0x100: Glacier Park to Citadel City
	        0x200: No known source use
	        0x400: Red Gem
	        0x800: Green Gem
	        0x1000: Blue Gem
	        0x2000: Yellow Gem
	        0x4000: Purple Gem
	        0x8000: N Sanity Beach Purple CTR Token
	        0x10000: Lost Ruins Purple CTR Token
	        0x20000: Glacier Park Purple CTR Token
	        0x40000: Citadel City Purple CTR Token
	        0x80000: Beat Oxide first ending flag
	        0x100000: Beat Oxide second ending flag
	        0x200000: No known source use
	        0x400000: Hint - Welcome to Arena
	        0x800000: Hint - Using a Warp Pad
	        0x1000000: Hint - Need More Trophies
	        0x2000000: Hint - Need Four Trophies for Boss
	        0x4000000: Hint - Need Four Keys for Oxide
	        0x8000000: Hint - Must Have One Boss Key
	        0x10000000: Save / Load Screen
	        0x20000000: New World Greeting (congratulations on opening this new area)
	        0x40000000: No known source use
	        0x80000000: No known source use

	    // 0x10: Hints
	        0x1: No known source use
	        0x2: No known source use
	        0x4: No known source use
	        0x8: No known source use
	        0x10: Hang Time Turbo
	        0x20: Power Slide
	        0x40: Turbo Boost Hint
	        0x80: Brake Slide (Square button)
	        0x100: Must have 2 boss keys
	        0x200: No known source use
	        0x400: No known source use
	        0x800: No known source use
	        0x1000: Wumpa Fruit Hint
	        0x2000: TNT Hint
	        0x4000: Map Information
	        0x8000: Collect Every Crystal in the Arena
	        0x10000: CTR Token Challenge
	        0x20000: Gem Cups Challenge
	        0x40000: Must get 10 relics
	        0x80000: Relic Challenge
	        0x100000: No known source use
	        0x200000: No known source use
	        0x400000: No known source use
	        0x800000: No known source use
	        0x1000000: No known source use
	        0x2000000: No known source use
	        0x4000000: No known source use
	        0x8000000: No known source use
	        0x10000000: No known source use
	        0x20000000: No known source use
	        0x40000000: No known source use
	        0x80000000: No known source use
	*/

	// 8008FBBC
	char name[18]; // 0x12

	// 8FBCE
	s16 characterID;

	// 8FBD0
	s16 unk;

	// 8FBD2
	s16 HubLevYouSavedOn;

	// 8FBD4
	// Count up to 10 times player lost
	// Including Crystal Challenge
	char timesLostRacePerLev[0x12];

	// 8fbe6
	// Count up to 10 times player lost
	char timesLostCupRace[5];

	// 8FBEB
	// Count up to 10 times player lost
	char timesLostBossRace[5];

	// 8FBF0
	// definitely saves to profile
	int unk_8FBF0;

	// 0x50 -- size of struct
};

// 0x34 bytes large
struct GhostProfile
{
	// ghostProfile_current
	// 8009ABCC - 8009AC00

	// 0x0
	char profile_name[0x15];

	// 0x15
	char SubmitName_name[0x11];

	// 0x26
	s16 alwaysOne;

	// 0x28
	s16 trackID;

	// 0x2A
	s16 characterID;

	// 0x2C
	int memcardProfileIndex;

	// 0x30
	int trackTime;

	// 0x34 -- size of struct
};

// 8008FB7C
struct GameOptions
{
	// 8008fb7c
	s16 volFx;

	// 8008fb7e
	s16 volMusic;

	// 8008fb80
	s16 volVoice;

	// 8008fb82  24 bytes total (0x18)
	struct RacingWheelData rwd[4];

	// 8008fb9A -- 2-byte padding


	// one of these two ints are
	// in 94426 and beyond, not in sep3
	// idk which


	// 8008fb9c
	u32 gameMode1_vibrationFlags;

#if BUILD >= UsaRetail
	// 8008fba0
	// audio mode (mono/stereo)
	int audioMode;
#endif
};

enum GameOptionsVolume
{
	GAME_OPTIONS_VOLUME_FX,
	GAME_OPTIONS_VOLUME_MUSIC,
	GAME_OPTIONS_VOLUME_VOICE,
	GAME_OPTIONS_VOLUME_COUNT,
};

struct MemcardProfile
{
	// 0x0
	s16 header[2];

	// 0x4
	struct AdvProgress advProgress[MEMCARD_ADV_PROFILE_COUNT];

	// 0x144
	struct GameProgress gameProgress;

	// 0x15D8
	struct GameOptions gameOptions;

	// 0x1600 - size of profile
};

CTR_STATIC_ASSERT(sizeof(struct HighScoreEntry) == 0x18);
CTR_STATIC_ASSERT(sizeof(struct HighScoreTrack) == 0x124);
CTR_STATIC_ASSERT((u16)MEMCARD_PROFILE_VERSION == 0xffee);
CTR_STATIC_ASSERT(OFFSETOF(struct GameProgress, unlockFlags) == 0x4);
CTR_STATIC_ASSERT(OFFSETOF(struct GameProgress, extendedUnlockFlags) == 0x8);
CTR_STATIC_ASSERT(OFFSETOF(struct GameProgress, highScoreTracks) == 0xc);
CTR_STATIC_ASSERT(sizeof(struct GameProgress) == 0x1494);
CTR_STATIC_ASSERT(OFFSETOF(struct AdvProgress, platinumCtrTokenFlags) == 0x8);
CTR_STATIC_ASSERT(OFFSETOF(struct AdvProgress, storyFlags) == 0xc);
CTR_STATIC_ASSERT(OFFSETOF(struct AdvProgress, hintFlags) == 0x10);
CTR_STATIC_ASSERT(OFFSETOF(struct AdvProgress, reservedRewardFlags) == 0x14);
CTR_STATIC_ASSERT(OFFSETOF(struct AdvProgress, name) == 0x18);
CTR_STATIC_ASSERT(sizeof(struct AdvProgress) == 0x50);
CTR_STATIC_ASSERT(sizeof(struct GhostProfile) == 0x34);
CTR_STATIC_ASSERT(OFFSETOF(struct GameOptions, volMusic) == OFFSETOF(struct GameOptions, volFx) + sizeof(s16));
CTR_STATIC_ASSERT(OFFSETOF(struct GameOptions, volVoice) == OFFSETOF(struct GameOptions, volFx) + sizeof(s16) * 2);
#if BUILD >= UsaRetail
CTR_STATIC_ASSERT(sizeof(struct GameOptions) == 0x28);
#else
CTR_STATIC_ASSERT(sizeof(struct GameOptions) == 0x24);
#endif

#endif
