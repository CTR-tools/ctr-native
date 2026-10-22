#include <common.h>

#ifndef BOT_NAV_HEADERS
#define BOT_NAV_HEADERS         (sdata->NavPath_ptrHeader)
#define BOT_NAV_FRAMES          (sdata->NavPath_ptrNavFrameArray)
#define BOT_NAV_FALLBACK_HEADER (sdata->blank_NavHeader)
#define BOT_NAV_NUM_POINTS      (sdata->nav_NumPointsOnPath)
#define BOT_NAV_LAST_PATH       (sdata->lastPathIndex)
#endif


#ifndef BOTS_ADV_PROGRESS
#define BOTS_ADV_PROGRESS (sdata->advProgress)
#define BOTS_KART_ORDER   (sdata->kartSpawnOrderArray)
#define BOTS_DRIVER_PATHS (sdata->driver_pathIndexIDs)
#define BOTS_ACCEL_ORDER  (sdata->accelerateOrder)
#define BOTS_NAV_LIST     (sdata->navBotList)
#define BOTS_ADV_RNG      (sdata->advRng)
#endif

#ifndef BOTS_ROBOT_DEBUG_NAME
// NOTE(aalhendi): Native omits debug names; retail supplies the resident robot string.
#define BOTS_ROBOT_DEBUG_NAME NULL
#endif

enum
{
	BOTS_ADV_MAX_LOSS_DIFFICULTY_INDEX = 10,
	BOTS_DIFFICULTY_BLEND_DENOMINATOR = 0xf0,
	BOTS_DIFFICULTY_SUPERHARD = 0x140,
	BOTS_DIFFICULTY_CUP_OFFSET = 0x50,
	BOTS_ADV_NORMAL_SCALE = 5,
	BOTS_ADV_CHEAT_SCALE = 7,
	BOTS_ADV_HIGH_TIER_LOSS_BASE = 0xe1,
	BOTS_ADV_HIGH_TIER_CHEAT_LOSS_BASE = 0x141,
	BOTS_ADV_CUP_LOSS_BASE = 0xcd,
	BOTS_ADV_CUP_CHEAT_LOSS_BASE = 300,
	BOTS_ADV_RACE_LOSS_BASE = 0x3c,
	BOTS_ADV_RACE_TROPHY_SCALE = 0x23,
	BOTS_ADV_RACE_TROPHY_BIAS = 3,
	BOTS_ADV_RACE_TROPHY_SHIFT = 2,
	BOTS_ADV_RACE_CHEAT_LOSS_BASE = 100,
	BOTS_ADV_RACE_CHEAT_TROPHY_SCALE = 0xc,
	BOTS_NAV_PATH_COUNT = 3,
	BOTS_MAX_KARTS = 8,
	BOTS_BATTLE_DRIVER_COUNT = 4,
	BOTS_GRID_ROW_KART_COUNT = 4,
	BOTS_GRID_REAR_ROW_OFFSET = 4,
	BOTS_RNG_BYTE_SHIFT = 8,
	BOTS_RNG_BIT_MASK = 1,
	BOTS_RNG_ACCEL_MASK = 3,
	BOTS_RATAN_ARG_SHIFT = 12,
	BOTS_ROT_BYTE_SHIFT = 4,
	BOTS_NAV_REVERSE_YAW_OFFSET = 0x800,
	BOTS_SPAWN_YAW_OFFSET = 0x400,
	BOTS_BATTLE_PATH_RANDOM_MASK = 0xfff,
	BOTS_BATTLE_PATH_RANDOM_DIVISOR = 0x555,
	BOTS_PATH_CHANGE_PATH_SHIFT = 10,
	BOTS_PATH_CHANGE_FRAME_MASK = 0x3ff,
	BOTS_LEVEL_INST_COLL_RADIUS = 0x19,
	BOTS_MASK_MIDPOINT_DIVISOR = 2,
	BOTS_MASK_DROP_HEIGHT = 0x10000,
	BOTS_AI_COLLISION_SPEED_PENALTY = 3000,
	BOTS_RACE_START_AI_COLLISION_DELAY_FRAMES = CTR_SECONDS_TO_FRAMES(15),

	BOTS_DAMAGE_STATE_SPIN = 1,
	BOTS_DAMAGE_STATE_BLAST = 2,
	BOTS_DAMAGE_STATE_SQUISH = 3,
	BOTS_DAMAGE_STATE_MASK_GRAB = 5,

	BOTS_NAV_FLAG_TURBO = 0x1,
	BOTS_NAV_FLAG_BACK_SKID = 0x2,
	BOTS_NAV_FLAG_FRONT_SKID = 0x4,
	BOTS_NAV_FLAG_SUPER_TURBO = 0x100,
	BOTS_NAV_FLAG_KILLPLANE = 0x200,
	BOTS_NAV_FLAG_JUMP = 0x400,
	BOTS_NAV_FLAG_DRIFT_LEFT = 0x800,
	BOTS_NAV_FLAG_DRIFT_RIGHT = 0x1000,
	BOTS_NAV_FLAG_ENGINE_ECHO = 0x2000,
	BOTS_NAV_FLAG_SPLIT_LINE = 0x8000,
	BOTS_NAV_FLAG_DRIFT_MASK = BOTS_NAV_FLAG_DRIFT_LEFT | BOTS_NAV_FLAG_DRIFT_RIGHT,

	BOTS_NAV_SPECIAL_RAMP_PHYS = 0x10,
	BOTS_NAV_SPECIAL_REFLECTIVE = 0x20,
	BOTS_NAV_SPECIAL_LEVEL_INST_COLL = 0x40,
	BOTS_NAV_SPECIAL_MOON_GRAVITY = 0x80,
	BOTS_NAV_SPECIAL_INDEX_MASK = 0xf,
};

static s16 BOTS_PathChangePathID(s16 changeOpcode)
{
	return (s16)CTR_MipsSra(changeOpcode, BOTS_PATH_CHANGE_PATH_SHIFT);
}

static s16 BOTS_PathChangeFrameIndex(s16 changeOpcode)
{
	return changeOpcode & BOTS_PATH_CHANGE_FRAME_MASK;
}

static s16 BOTS_PathChangeCap(void)
{
	return (s16)CTR_MipsSll(BOTS_NAV_PATH_COUNT, BOTS_PATH_CHANGE_PATH_SHIFT);
}

int BOTS_Adv_NumTimesLostEvent(int numLost)
{
	s16 *difficulty;
	// if you lost more than 10 times
	// the difficulty will not get lower.
	if ((u16)numLost > BOTS_ADV_MAX_LOSS_DIFFICULTY_INDEX)
	{
		// the array apparently has 12, not sure why it stopped at 11.
		numLost = BOTS_ADV_MAX_LOSS_DIFFICULTY_INDEX;
	}

	difficulty = data.advDifficulty;
	CTR_PSX_OBSERVE_VALUE(difficulty);
	return difficulty[(s16)numLost];
}

void BOTS_SetGlobalNavData(u16 index)
{
	register u8 *headers CTR_PSX_REGISTER("$3");
	register s32 indexOffset CTR_PSX_REGISTER("$5");
	register u8 *frames CTR_PSX_REGISTER("$2");
	register struct NavHeader *header CTR_PSX_REGISTER("$3");
	register struct NavFrame **frameSlot CTR_PSX_REGISTER("$5");
	register u16 numPoints CTR_PSX_REGISTER("$2");
	register struct NavFrame *firstPoint CTR_PSX_REGISTER("$3");

	headers = (u8 *)&BOT_NAV_HEADERS;
	indexOffset = (s16)index * (s32)sizeof(struct NavFrame *);
	CTR_PSX_OBSERVE_MEMORY(*(struct NavHeader **)(headers + indexOffset));
	frames = (u8 *)&BOT_NAV_FRAMES;
	header = *(struct NavHeader **)(headers + indexOffset);
	frameSlot = (struct NavFrame **)(frames + indexOffset);
	numPoints = (u16)header->numPoints;
	firstPoint = *frameSlot;
	BOT_NAV_LAST_PATH = index;
	BOT_NAV_NUM_POINTS = (s16)numPoints;
	sdata->nav_ptrFirstPoint = firstPoint;
	sdata->nav_ptrLastPoint = &(*frameSlot)[(s16)numPoints];
	CTR_PSX_OBSERVE_MEMORY(sdata->nav_ptrLastPoint);
	return;
}

void BOTS_InitNavPath(struct GameTracker *gGT, s16 index)
{
	register s32 incomingIndex CTR_PSX_REGISTER("$5");
	s32 pathIndex;
	struct NavHeader *nh;
	struct NavHeader *pathHeader;
	struct NavFrame *lastPoint;
	struct NavFrame *firstPoint;
	struct NavHeader **LevNavTable;
	struct NavHeader **navSlot;
	(void)gGT;
	LevNavTable = GAME_TRACKER->level1->LevNavTable;
	CTR_PSX_CAPTURE_REGISTER(incomingIndex, index);
	CTR_PSX_OBSERVE_VALUE(incomingIndex);
	pathIndex = incomingIndex;

	if (LevNavTable != 0)
	{
		// nullptr on Nitro Court
		CTR_PSX_ADD_POINTER_OFFSET(navSlot, LevNavTable, pathIndex * (s32)sizeof(*LevNavTable));
		nh = *navSlot;
	}
	else
	{
		nh = 0;
	}

	// if path exists
	if (nh != 0)
	{
		// grab the data
		BOT_NAV_HEADERS[pathIndex] = nh;

		BOT_NAV_FRAMES[pathIndex] = NAVHEADER_GETFRAME(BOT_NAV_HEADERS[pathIndex]);

		// if data is outdated
		if ((u16)BOT_NAV_HEADERS[pathIndex]->magicNumber != (u16)-0x1303)
		{
			// never mind then
			BOT_NAV_HEADERS[pathIndex]->numPoints = 0;
		}
	}

	// if no path data is found
	else
	{
		// use a template, which cancels AIs
		BOT_NAV_HEADERS[pathIndex] = &BOT_NAV_FALLBACK_HEADER;

		BOT_NAV_FRAMES[pathIndex] = NAVHEADER_GETFRAME(&BOT_NAV_FALLBACK_HEADER);

		BOT_NAV_HEADERS[pathIndex]->numPoints = 0;
	}

	// save number of points
	BOT_NAV_NUM_POINTS = BOT_NAV_HEADERS[pathIndex]->numPoints;
	pathHeader = BOT_NAV_HEADERS[pathIndex];

	// global last point
	lastPoint = &BOT_NAV_FRAMES[pathIndex][(s16)BOT_NAV_NUM_POINTS];

	// header last point
	pathHeader->last = lastPoint;

	// global first point
	firstPoint = BOT_NAV_FRAMES[pathIndex];
	sdata->nav_ptrLastPoint = lastPoint;
	sdata->nav_ptrFirstPoint = firstPoint;
	CTR_PSX_OBSERVE_MEMORY(sdata->nav_ptrFirstPoint);

	return;
}

void BOTS_EmptyFunc(void)
{
}

// NOTE(aalhendi): Difficulty banks share the full Data object. Read their
// halfwords without aliasing; retail subtracts signed values but adds an unsigned low.
static inline s16 BOTS_ReadDifficultyHalf(const s16 *src)
{
	s16 value;
	memcpy(&value, src, sizeof(value));
	return value;
}

static inline u16 BOTS_ReadDifficultyUnsignedHalf(const s16 *src)
{
	u16 value;
	memcpy(&value, src, sizeof(value));
	return value;
}

// NOTE(aalhendi): Arcade and cup loops keep distinct register lifetimes to
// preserve retail scheduling. Both blend the eight per-kart difficulty entries.
static inline void BOTS_Adv_LerpDifficulty(s32 slotOffset, s16 factor, s16 initialIndex)
{
	register s16 *lo;
	register s16 *hi CTR_PSX_REGISTER("$9");
	register s16 *loopDst CTR_PSX_REGISTER("$11");
	register s32 shiftedFactor CTR_PSX_REGISTER("$2") = CTR_MipsSll(factor, 16);
	register s32 loopFactor CTR_PSX_REGISTER("$13");
	s16 next;
	register s16 i CTR_PSX_REGISTER("$17") = initialIndex;
	CTR_PSX_OBSERVE_VALUE(i);
	CTR_PSX_OBSERVE_VALUE(shiftedFactor);
	loopFactor = CTR_MipsSra(shiftedFactor, 16);
	CTR_PSX_OBSERVE_VALUE(loopFactor);
	CTR_PSX_MEMORY_BARRIER();
	loopDst = *(s16 **)((u8 *)sdata + slotOffset);
	lo = sdata->difficultyParams[1];
	hi = sdata->difficultyParams[0];
	do
	{
		register s32 offset CTR_PSX_REGISTER("$6") = i * sizeof(s16);
		register s16 *output CTR_PSX_REGISTER("$6");
		register const u8 *hiPointer CTR_PSX_REGISTER("$2");
		register const u8 *loPointer CTR_PSX_REGISTER("$7");
		register s32 hiValue CTR_PSX_REGISTER("$4");
		register s32 loValue CTR_PSX_REGISTER("$2");
		register s32 blend CTR_PSX_REGISTER("$4");
		register s32 quotient;
		CTR_PSX_ADD_POINTER_OFFSET(hiPointer, (const u8 *)hi, offset);
		CTR_PSX_ADD_POINTER_OFFSET(loPointer, (const u8 *)lo, offset);
		hiValue = BOTS_ReadDifficultyHalf((const s16 *)hiPointer);
		loValue = BOTS_ReadDifficultyHalf((const s16 *)loPointer);
		hiValue = hiValue - loValue;
		CTR_PSX_OBSERVE_VALUE(hiValue);
		blend = loopFactor * hiValue;
		CTR_PSX_DEPEND_MEMORY((u8 *)loPointer, blend);
		quotient = blend / BOTS_DIFFICULTY_BLEND_DENOMINATOR;
		next = i + 1;
		CTR_PSX_COPY_VALUE(i, next);
		CTR_PSX_DEPEND_VALUE(offset, i);
		CTR_PSX_ADD_POINTER_OFFSET(output, loopDst, offset);
		*output = BOTS_ReadDifficultyUnsignedHalf((const s16 *)loPointer) + quotient;
	} while (next < BOTS_MAX_KARTS);
}

static inline void BOTS_Adv_LerpCupDifficulty(s16 factor, s16 *sourceHi, s16 *sourceLo)
{
	register s16 *boundSourceLo CTR_PSX_REGISTER("$8") = sourceLo;
	register s16 *lo CTR_PSX_REGISTER("$11");
	register s16 *hi CTR_PSX_REGISTER("$9");
	register s16 *loopDst CTR_PSX_REGISTER("$12");
	register s32 shiftedFactor CTR_PSX_REGISTER("$2");
	register s32 loopFactor;
	s16 next;
	register s16 i CTR_PSX_REGISTER("$17") = 0;
	CTR_PSX_OBSERVE_VALUE(i);
	CTR_PSX_DEPEND_MEMORY(&sdata->cup_difficultyParams, i);
	CTR_PSX_LOAD_WORD_AFTER(loopDst, sdata->cup_difficultyParams, i);
	CTR_PSX_DEPEND_VALUE(boundSourceLo, loopDst);
	CTR_PSX_COPY_VALUE(lo, boundSourceLo);
	hi = sourceHi;
	CTR_PSX_CLOBBER("$2");
	shiftedFactor = CTR_MipsSll(factor, 16);
	CTR_PSX_OBSERVE_VALUE(shiftedFactor);
	loopFactor = CTR_MipsSra(shiftedFactor, 16);
	CTR_PSX_OBSERVE_VALUE(loopFactor);
	CTR_PSX_OBSERVE_VALUE(boundSourceLo);

	do
	{
		register s32 offset CTR_PSX_REGISTER("$6") = i * sizeof(s16);
		register s16 *output CTR_PSX_REGISTER("$6");
		register const u8 *hiPointer CTR_PSX_REGISTER("$2");
		register const u8 *loPointer CTR_PSX_REGISTER("$7");
		register s32 hiValue CTR_PSX_REGISTER("$4");
		register s32 loValue CTR_PSX_REGISTER("$2");
		register s32 blend CTR_PSX_REGISTER("$4");
		register s32 quotient;
		CTR_PSX_ADD_POINTER_OFFSET(hiPointer, (const u8 *)hi, offset);
		CTR_PSX_ADD_POINTER_OFFSET(loPointer, (const u8 *)lo, offset);
		hiValue = BOTS_ReadDifficultyHalf((const s16 *)hiPointer);
		loValue = BOTS_ReadDifficultyHalf((const s16 *)loPointer);
		hiValue = hiValue - loValue;
		CTR_PSX_OBSERVE_VALUE(hiValue);
		blend = loopFactor * hiValue;
		CTR_PSX_DEPEND_MEMORY((u8 *)loPointer, blend);
		quotient = blend / BOTS_DIFFICULTY_BLEND_DENOMINATOR;
		next = i + 1;
		CTR_PSX_COPY_VALUE(i, next);
		CTR_PSX_DEPEND_VALUE(offset, i);
		CTR_PSX_ADD_POINTER_OFFSET(output, loopDst, offset);
		*output = BOTS_ReadDifficultyUnsignedHalf((const s16 *)loPointer) + quotient;
	} while (next < BOTS_MAX_KARTS);
}

static inline struct GameTracker *BOTS_Adv_LerpRemainingDifficulty(s16 *dst, s32 shiftedFactor, s16 *hi, s16 *lo, b32 captureTracker)
{
	register s32 factor CTR_PSX_REGISTER("$5");
	register struct GameTracker *nextTracker CTR_PSX_REGISTER("$4") = 0;

	{
		register s32 hiValue;
		register s32 loValue CTR_PSX_REGISTER("$2");
		register s32 blend;
		hiValue = BOTS_ReadDifficultyHalf((const s16 *)((const u8 *)hi + 11 * sizeof(s16)));
		CTR_PSX_OBSERVE_VALUE(hiValue);
		loValue = BOTS_ReadDifficultyHalf((const s16 *)((const u8 *)lo + 11 * sizeof(s16)));
		CTR_PSX_DEPEND_VALUE(shiftedFactor, loValue);
		factor = CTR_MipsSra(shiftedFactor, 16);
		hiValue -= loValue;
		CTR_PSX_OBSERVE_VALUE(hiValue);
		blend = factor * hiValue;
		CTR_PSX_OBSERVE_VALUE(blend);
		CTR_PSX_DEPEND_MEMORY((u8 *)lo + 11 * sizeof(s16), blend);
		dst[11] = BOTS_ReadDifficultyUnsignedHalf((const s16 *)((const u8 *)lo + 11 * sizeof(s16))) + blend / BOTS_DIFFICULTY_BLEND_DENOMINATOR;
	}
	{
		s16 hiValue = BOTS_ReadDifficultyHalf((const s16 *)((const u8 *)hi + 12 * sizeof(s16)));
		s16 loValue = BOTS_ReadDifficultyHalf((const s16 *)((const u8 *)lo + 12 * sizeof(s16)));
		register s32 blend CTR_PSX_REGISTER("$2") = factor * (hiValue - loValue);
		CTR_PSX_DEPEND_MEMORY((u8 *)lo + 12 * sizeof(s16), blend);
		dst[12] = BOTS_ReadDifficultyUnsignedHalf((const s16 *)((const u8 *)lo + 12 * sizeof(s16))) + blend / BOTS_DIFFICULTY_BLEND_DENOMINATOR;
	}
	{
		s16 hiValue = BOTS_ReadDifficultyHalf((const s16 *)((const u8 *)hi + 13 * sizeof(s16)));
		s16 loValue = BOTS_ReadDifficultyHalf((const s16 *)((const u8 *)lo + 13 * sizeof(s16)));
		register s32 blend CTR_PSX_REGISTER("$2") = factor * (hiValue - loValue);
		CTR_PSX_DEPEND_MEMORY((u8 *)lo + 13 * sizeof(s16), blend);
		dst[13] = BOTS_ReadDifficultyUnsignedHalf((const s16 *)((const u8 *)lo + 13 * sizeof(s16))) + blend / BOTS_DIFFICULTY_BLEND_DENOMINATOR;
	}
	{
		s16 hiValue = BOTS_ReadDifficultyHalf((const s16 *)((const u8 *)hi + 9 * sizeof(s16)));
		s16 loValue = BOTS_ReadDifficultyHalf((const s16 *)((const u8 *)lo + 9 * sizeof(s16)));
		register s32 blend CTR_PSX_REGISTER("$2") = factor * (hiValue - loValue);
		CTR_PSX_DEPEND_MEMORY((u8 *)lo + 9 * sizeof(s16), blend);
		dst[9] = BOTS_ReadDifficultyUnsignedHalf((const s16 *)((const u8 *)lo + 9 * sizeof(s16))) + blend / BOTS_DIFFICULTY_BLEND_DENOMINATOR;
	}
	{
		s16 hiValue = BOTS_ReadDifficultyHalf((const s16 *)((const u8 *)hi + 8 * sizeof(s16)));
		s16 loValue = BOTS_ReadDifficultyHalf((const s16 *)((const u8 *)lo + 8 * sizeof(s16)));
		register s32 blend CTR_PSX_REGISTER("$2") = factor * (hiValue - loValue);
		CTR_PSX_DEPEND_MEMORY((u8 *)lo + 8 * sizeof(s16), blend);
		dst[8] = BOTS_ReadDifficultyUnsignedHalf((const s16 *)((const u8 *)lo + 8 * sizeof(s16))) + blend / BOTS_DIFFICULTY_BLEND_DENOMINATOR;
	}
	{
		s16 hiValue = BOTS_ReadDifficultyHalf((const s16 *)((const u8 *)hi + 10 * sizeof(s16)));
		s16 loValue = BOTS_ReadDifficultyHalf((const s16 *)((const u8 *)lo + 10 * sizeof(s16)));
		register s32 blend CTR_PSX_REGISTER("$5") = factor * (hiValue - loValue);
		s32 quotient;
		CTR_PSX_DEPEND_MEMORY((u8 *)lo + 10 * sizeof(s16), blend);
		quotient = blend / BOTS_DIFFICULTY_BLEND_DENOMINATOR;
		if (captureTracker)
		{
			nextTracker = GAME_TRACKER;
		}
		dst[10] = BOTS_ReadDifficultyUnsignedHalf((const s16 *)((const u8 *)lo + 10 * sizeof(s16))) + quotient;
	}
	return nextTracker;
}

static inline void BOTS_Adv_CopySpawnOrder(const u8 *source)
{
	memcpy(BOTS_KART_ORDER, source, sizeof(sdata->kartSpawnOrderArray));
}

internal s32 BOTS_GetTrackDistanceToFinish(struct GameTracker *gGT)
{
#if defined(CTR_NATIVE)
	// NOTE(aalhendi): Menu-storage/wrong-warp can leave stale bot threads in
	// levels without restart points. Retail blind-loads from low PSX memory;
	// native uses zero so only stale AI spacing/rubberband math is affected.
	if ((gGT->level1 == NULL) || (gGT->level1->ptr_restart_points == NULL))
	{
		return 0;
	}
#endif

	return CTR_MipsSll(gGT->level1->ptr_restart_points->distToFinish, 3);
}

void BOTS_Adv_AdjustDifficulty(void)
{
	register s32 currDifficulty CTR_PSX_REGISTER("$8");
	s16 cupDifficulty = 0;

	{
		register struct GameTracker *tracker CTR_PSX_REGISTER("$4") = GAME_TRACKER;
		register u8 *rowBase;
		s32 firstIndex;
		register s32 secondIndex CTR_PSX_REGISTER("$4");
		register s32 firstOffset CTR_PSX_REGISTER("$2");
		u32 initialMode;
		// NOTE(aalhendi): Battle level IDs extend past the arcade bank into
		// adjacent retail data. Address the enclosing object, not ArcadeDifficulty[18].
		initialMode = tracker->gameMode1;
		if ((initialMode & ADVENTURE_BOSS) != 0)
		{
			CTR_PSX_LOAD_WORD_VOLATILE(firstIndex, tracker->bossID);
			CTR_PSX_LOAD_WORD_VOLATILE(secondIndex, tracker->bossID);
			CTR_PSX_OBSERVE_VALUE(secondIndex);
			firstOffset = firstIndex * sizeof(struct Difficulty);
			CTR_PSX_OBSERVE_VALUE(firstOffset);
			rowBase = (u8 *)&data + offsetof(struct Data, BossDifficulty);
		}
		else
		{
			CTR_PSX_LOAD_WORD_VOLATILE(firstIndex, tracker->levelID);
			CTR_PSX_LOAD_WORD_VOLATILE(secondIndex, tracker->levelID);
			CTR_PSX_OBSERVE_VALUE(secondIndex);
			firstOffset = firstIndex * sizeof(struct Difficulty);
			CTR_PSX_OBSERVE_VALUE(firstOffset);
			rowBase = (u8 *)&data + offsetof(struct Data, ArcadeDifficulty);
		}
		{
			register u8 *firstPointer CTR_PSX_REGISTER("$2");
			register u8 *secondPointer CTR_PSX_REGISTER("$2");
			CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(firstPointer, rowBase, firstOffset);
			CTR_PSX_OBSERVE_VALUE(firstPointer);
			sdata->difficultyParams[1] = (s16 *)firstPointer;
			CTR_PSX_OBSERVE_MEMORY(sdata->difficultyParams[1]);
			firstOffset = secondIndex * sizeof(struct Difficulty);
			CTR_PSX_DEPEND_VALUE(rowBase, firstOffset);
			CTR_PSX_ADD_POINTER_IMMEDIATE(rowBase, rowBase, 28, rowBase + offsetof(struct Difficulty, params2));
			CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(secondPointer, rowBase, firstOffset);
			CTR_PSX_OBSERVE_VALUE(secondPointer);
			sdata->difficultyParams[0] = (s16 *)secondPointer;
			CTR_PSX_OBSERVE_MEMORY(sdata->difficultyParams[0]);
		}
	}

	{
		register struct GameTracker *modeTracker CTR_PSX_REGISTER("$5") = GAME_TRACKER;
		register s32 finalLost CTR_PSX_REGISTER("$2");
		s32 finalBase;
		CTR_PSX_OBSERVE_VALUE(modeTracker);
		if ((modeTracker->gameMode1 & ARCADE_MODE) != 0)
		{
			register u32 cheatMask = CHEAT_SUPERHARD;
			u32 cheats;
			CTR_PSX_OBSERVE_VALUE(cheatMask);
			cheats = modeTracker->gameMode2;
			CTR_PSX_LOAD_UNSIGNED_HALF_AFTER(currDifficulty, modeTracker, 7868, modeTracker->arcadeDifficulty, cheats);
			if ((cheats & cheatMask) != 0)
				currDifficulty = BOTS_DIFFICULTY_SUPERHARD;
			cupDifficulty = (s16)((u16)modeTracker->arcadeDifficulty + BOTS_DIFFICULTY_CUP_OFFSET);
			goto difficultyReady;
		}
		else if ((modeTracker->gameMode1 & ADVENTURE_CUP) != 0)
		{
			register s32 lostModifier CTR_PSX_REGISTER("$2");
			register s32 cupCheatTrack CTR_PSX_REGISTER("$4");
			register s32 cupCheatBase;
			if (modeTracker->cup.cupID == 4)
			{
				lostModifier = BOTS_Adv_NumTimesLostEvent((s8)BOTS_ADV_PROGRESS.timesLostCupRace[modeTracker->cup.trackIndex]);
				{
					register struct GameTracker *phaseTracker CTR_PSX_REGISTER("$4") = GAME_TRACKER;
					register s32 track CTR_PSX_REGISTER("$5");
					register s32 difficultyBase;
					CTR_PSX_OBSERVE_VALUE(phaseTracker);
					track = phaseTracker->cup.trackIndex;
					CTR_PSX_DEPEND_VALUE(lostModifier, track);
					lostModifier -= BOTS_ADV_HIGH_TIER_LOSS_BASE;
					difficultyBase = track * BOTS_ADV_NORMAL_SCALE;
					CTR_PSX_DEPEND_VALUE(difficultyBase, lostModifier);
					CTR_PSX_BIND_VALUE_CLOBBER(difficultyBase, "$8");
					currDifficulty = difficultyBase - lostModifier;
					if ((phaseTracker->gameMode2 & CHEAT_ADV) != 0)
					{
						lostModifier = BOTS_Adv_NumTimesLostEvent((s8)BOTS_ADV_PROGRESS.timesLostCupRace[phaseTracker->cup.trackIndex]);
						{
							register struct GameTracker *cheatTracker = GAME_TRACKER;
							CTR_PSX_OBSERVE_VALUE(cheatTracker);
							cupCheatTrack = cheatTracker->cup.trackIndex;
							CTR_PSX_DEPEND_VALUE(lostModifier, cupCheatTrack);
							lostModifier -= BOTS_ADV_HIGH_TIER_CHEAT_LOSS_BASE;
							goto cupCheatDifficulty;
						}
					}
				}
			}
			else
			{
				lostModifier = BOTS_Adv_NumTimesLostEvent((s8)BOTS_ADV_PROGRESS.timesLostCupRace[modeTracker->cup.trackIndex]);
				{
					register struct GameTracker *phaseTracker CTR_PSX_REGISTER("$4") = GAME_TRACKER;
					register s32 track CTR_PSX_REGISTER("$5");
					register s32 difficultyBase;
					CTR_PSX_OBSERVE_VALUE(phaseTracker);
					track = phaseTracker->cup.trackIndex;
					CTR_PSX_DEPEND_VALUE(lostModifier, track);
					lostModifier -= BOTS_ADV_CUP_LOSS_BASE;
					difficultyBase = track * BOTS_ADV_NORMAL_SCALE;
					CTR_PSX_DEPEND_VALUE(difficultyBase, lostModifier);
					CTR_PSX_BIND_VALUE_CLOBBER(difficultyBase, "$8");
					currDifficulty = difficultyBase - lostModifier;
					if ((phaseTracker->gameMode2 & CHEAT_ADV) != 0)
					{
						lostModifier = BOTS_Adv_NumTimesLostEvent((s8)BOTS_ADV_PROGRESS.timesLostCupRace[phaseTracker->cup.trackIndex]);
						{
							register struct GameTracker *cheatTracker = GAME_TRACKER;
							CTR_PSX_OBSERVE_VALUE(cheatTracker);
							cupCheatTrack = cheatTracker->cup.trackIndex;
							CTR_PSX_DEPEND_VALUE(lostModifier, cupCheatTrack);
							lostModifier -= BOTS_ADV_CUP_CHEAT_LOSS_BASE;
							goto cupCheatDifficulty;
						}
					}
				}
			}
			goto cupDifficultyReady;
		cupCheatDifficulty:
			cupCheatBase = cupCheatTrack * BOTS_ADV_CHEAT_SCALE;
			CTR_PSX_DEPEND_VALUE(cupCheatBase, lostModifier);
			CTR_PSX_BIND_VALUE_CLOBBER(cupCheatBase, "$8");
			currDifficulty = cupCheatBase - lostModifier;
		cupDifficultyReady:
			cupDifficulty = (s16)(currDifficulty + BOTS_DIFFICULTY_CUP_OFFSET);
			goto difficultyReady;
		}
		else if ((modeTracker->gameMode1 & ADVENTURE_BOSS) != 0)
		{
			register s32 lostModifier CTR_PSX_REGISTER("$2");
			lostModifier = BOTS_Adv_NumTimesLostEvent((s8)BOTS_ADV_PROGRESS.timesLostBossRace[modeTracker->bossID]);
			{
				register struct GameTracker *phaseTracker CTR_PSX_REGISTER("$4") = GAME_TRACKER;
				register s32 track CTR_PSX_REGISTER("$5");
				register s32 difficultyBase;
				CTR_PSX_OBSERVE_VALUE(phaseTracker);
				track = phaseTracker->bossID;
				CTR_PSX_DEPEND_VALUE(lostModifier, track);
				lostModifier -= BOTS_ADV_HIGH_TIER_LOSS_BASE;
				difficultyBase = track * BOTS_ADV_NORMAL_SCALE;
				CTR_PSX_DEPEND_VALUE(difficultyBase, lostModifier);
				CTR_PSX_BIND_VALUE_CLOBBER(difficultyBase, "$8");
				currDifficulty = difficultyBase - lostModifier;
				if ((phaseTracker->gameMode2 & CHEAT_ADV) != 0)
				{
					lostModifier = BOTS_Adv_NumTimesLostEvent((s8)BOTS_ADV_PROGRESS.timesLostBossRace[phaseTracker->bossID]);
					{
						register struct GameTracker *cheatTracker = GAME_TRACKER;
						register s32 cheatTrack CTR_PSX_REGISTER("$4");
						register s32 cheatDifficultyBase;

						CTR_PSX_OBSERVE_VALUE(cheatTracker);
						cheatTrack = cheatTracker->bossID;
						CTR_PSX_DEPEND_VALUE(lostModifier, cheatTrack);
						lostModifier -= BOTS_ADV_HIGH_TIER_CHEAT_LOSS_BASE;
						cheatDifficultyBase = cheatTrack * BOTS_ADV_CHEAT_SCALE;
						finalBase = cheatDifficultyBase;
						finalLost = lostModifier;
						goto finalCheatDifficulty;
					}
				}
			}

			goto difficultyReady;
		}
		else
		{
			u8 *lossRecordBase = (u8 *)&BOTS_ADV_PROGRESS;
			register s32 numTrophies;
			register s32 lostModifier CTR_PSX_REGISTER("$2");
			register s32 maxDifficulty;
			register s32 lossCount CTR_PSX_REGISTER("$4");
			register u8 *lossAddress CTR_PSX_REGISTER("$2");
			struct GameTracker *phaseTracker;
			lossAddress = lossRecordBase + modeTracker->levelID;
			CTR_PSX_LOAD_SIGNED_BYTE(lossCount, lossAddress, 48, *(s8 *)(lossAddress + offsetof(struct AdvProgress, timesLostRacePerLev)));
			CTR_PSX_LOAD_UNSIGNED_HALF_AFTER(numTrophies, modeTracker, 7728, modeTracker->currAdvProfile.numTrophies, lossCount);
			numTrophies = (s32)((u32)numTrophies << 16);
			lostModifier = BOTS_Adv_NumTimesLostEvent(lossCount);
			CTR_PSX_DEPEND_VALUE(numTrophies, lostModifier);
			numTrophies >>= 16;
			// NOTE(aalhendi): Retail adds in a word; native keeps the signed-halfword wrap.
			CTR_PSX_ADD_POINTER_IMMEDIATE(numTrophies, numTrophies, 1, (s16)(numTrophies + 1));
			CTR_PSX_OBSERVE_VALUE(numTrophies);
			maxDifficulty = numTrophies * BOTS_ADV_RACE_TROPHY_SCALE;
			if (maxDifficulty < 0)
				maxDifficulty += BOTS_ADV_RACE_TROPHY_BIAS;
			maxDifficulty >>= BOTS_ADV_RACE_TROPHY_SHIFT;
			CTR_PSX_DEPEND_VALUE(lostModifier, maxDifficulty);
			lostModifier -= BOTS_ADV_RACE_LOSS_BASE;
			phaseTracker = GAME_TRACKER;
			currDifficulty = maxDifficulty - lostModifier;
			if ((phaseTracker->gameMode2 & CHEAT_ADV) != 0)
			{
				lossAddress = lossRecordBase + phaseTracker->levelID;
				CTR_PSX_LOAD_SIGNED_BYTE(lossCount, lossAddress, 48, *(s8 *)(lossAddress + offsetof(struct AdvProgress, timesLostRacePerLev)));
				lostModifier = BOTS_Adv_NumTimesLostEvent(lossCount);
				maxDifficulty = numTrophies * BOTS_ADV_RACE_CHEAT_TROPHY_SCALE;
				CTR_PSX_DEPEND_VALUE(lostModifier, maxDifficulty);
				lostModifier -= BOTS_ADV_RACE_CHEAT_LOSS_BASE;
				finalBase = maxDifficulty;
				finalLost = lostModifier;
			}
			else
				goto difficultyReady;
		}
	finalCheatDifficulty:
	{
		register s32 sharedBase;
		register s32 sharedLost CTR_PSX_REGISTER("$2");
		sharedBase = finalBase;
		sharedLost = finalLost;
		CTR_PSX_DEPEND_VALUE(sharedBase, sharedLost);
		CTR_PSX_BIND_VALUE_CLOBBER(sharedBase, "$8");
		currDifficulty = sharedBase - sharedLost;
	}
	difficultyReady:;
	}
	if ((s16)currDifficulty < 0)
	{
		currDifficulty = 0;
	}

	{
		register s16 initialIndex CTR_PSX_REGISTER("$17") = 0;
		CTR_PSX_OBSERVE_VALUE(initialIndex);
		CTR_PSX_FORGET_VALUE(currDifficulty);
		BOTS_Adv_LerpDifficulty(offsetof(struct sData, arcade_difficultyParams), (s16)currDifficulty, initialIndex);
	}
	CTR_PSX_RELOAD(sdata->arcade_difficultyParams);
	CTR_PSX_RELOAD(sdata->difficultyParams);
	{
		register s16 *tailDst CTR_PSX_REGISTER("$7");
		register s16 *tailHi CTR_PSX_REGISTER("$9");
		register s32 savedFactor CTR_PSX_REGISTER("$5");
		register s16 *tailLo CTR_PSX_REGISTER("$8");
		struct GameTracker *cupTracker;
		CTR_PSX_LOAD_WORD(tailDst, sdata->arcade_difficultyParams);
		CTR_PSX_LOAD_WORD(tailHi, sdata->difficultyParams[0]);
		savedFactor = CTR_MipsSll(currDifficulty, 16);
		CTR_PSX_OBSERVE_VALUE(savedFactor);
		tailLo = sdata->difficultyParams[1];
		CTR_PSX_OBSERVE_VALUE(tailLo);
		cupTracker = BOTS_Adv_LerpRemainingDifficulty(tailDst, savedFactor, tailHi, tailLo, 1);

		if (((cupTracker->gameMode1 & ADVENTURE_CUP) != 0) || ((cupTracker->gameMode2 & CUP_ANY_KIND) != 0))
		{
			BOTS_Adv_LerpCupDifficulty(cupDifficulty, tailHi, tailLo);
			CTR_PSX_RELOAD(sdata->cup_difficultyParams);
			CTR_PSX_RELOAD(sdata->difficultyParams);
			{
				register s16 *tailDst CTR_PSX_REGISTER("$9");
				register s16 *tailHi CTR_PSX_REGISTER("$7");
				register s32 savedFactor CTR_PSX_REGISTER("$5");
				register s16 *tailLo CTR_PSX_REGISTER("$8");
				CTR_PSX_LOAD_WORD(tailDst, sdata->cup_difficultyParams);
				CTR_PSX_LOAD_WORD(tailHi, sdata->difficultyParams[0]);
				tailLo = sdata->difficultyParams[1];
				CTR_PSX_OBSERVE_VALUE(tailLo);
				savedFactor = CTR_MipsSll(cupDifficulty, 16);
				CTR_PSX_OBSERVE_VALUE(savedFactor);
				{
					BOTS_Adv_LerpRemainingDifficulty(tailDst, savedFactor, tailHi, tailLo, 0);
				}
			}
		}
	}

	{
		register u32 initialState CTR_PSX_REGISTER("$2") = sdata->advRng.state0;
		sdata->aiCollisionDelayFrameCount = 0;
		CTR_PSX_OBSERVE_VALUE(initialState);

		if ((initialState == 0) && (sdata->advRng.state1 == 0))
		{
			sdata->advRng.state0 = 0x30215400;
			sdata->advRng.state1 = 0x493583fe;
		}
	}

	{
		register s32 i CTR_PSX_REGISTER("$17");
		s32 next;
		i = 0;
		do
		{
			LIST_Clear(&BOTS_NAV_LIST[(s16)i]);
			if ((GAME_TRACKER->gameMode1 & (GAME_CUTSCENE | MAIN_MENU)) == 0)
			{
				BOTS_InitNavPath(GAME_TRACKER, (s16)i);
			}
			next = i + 1;
			CTR_PSX_COPY_VALUE(i, next);
		} while ((s16)next < BOTS_NAV_PATH_COUNT);
	}

	BOTS_SetGlobalNavData(0);

	GAME_TRACKER->numBotsNextGame = 0;

	if (((GAME_TRACKER->gameMode2 & CUP_ANY_KIND) == 0) || (GAME_TRACKER->cup.trackIndex == 0))
	{
		if (GAME_TRACKER->numPlyrCurrGame == 2)
		{
			BOTS_Adv_CopySpawnOrder((const u8 *)&data + offsetof(struct Data, kartSpawnOrder.VS_2P_1));
		}
		else if (GAME_TRACKER->numPlyrCurrGame > 2)
		{
			BOTS_Adv_CopySpawnOrder((const u8 *)&data + offsetof(struct Data, kartSpawnOrder.VS_3P_4P_1));
		}

		if ((GAME_TRACKER->gameMode1 & (RELIC_RACE | TIME_TRIAL)) != 0)
		{
			BOTS_Adv_CopySpawnOrder((const u8 *)&data + offsetof(struct Data, kartSpawnOrder.time_trial_1));
		}
		else if ((GAME_TRACKER->gameMode1 & CRYSTAL_CHALLENGE) != 0)
		{
			BOTS_Adv_CopySpawnOrder((const u8 *)&data + offsetof(struct Data, kartSpawnOrder.crystal_challenge_1));
		}
		else if ((GAME_TRACKER->gameMode1 & ADVENTURE_BOSS) != 0)
		{
			BOTS_Adv_CopySpawnOrder((const u8 *)&data + offsetof(struct Data, kartSpawnOrder.boss_challenge_1));
		}
		else if (((GAME_TRACKER->gameMode1 & ADVENTURE_CUP) != 0) && (GAME_TRACKER->cup.cupID == 4))
		{
			BOTS_Adv_CopySpawnOrder((const u8 *)&data + offsetof(struct Data, kartSpawnOrder.purple_cup_1));
		}
		else if ((GAME_TRACKER->gameMode1 & ADVENTURE_MODE) == 0)
		{
			BOTS_Adv_CopySpawnOrder((const u8 *)&data + offsetof(struct Data, kartSpawnOrder.arcade_1));
		}
	}


	{
		u8 pathOrder[8];
		u8 firstPath;
		u8 secondPath;
		pathOrder[0] = 0;
		pathOrder[4] = 0;
		pathOrder[3] = 2;
		pathOrder[7] = 2;

		firstPath = ((s32)RngDeadCoed(&BOTS_ADV_RNG) >> BOTS_RNG_BYTE_SHIFT) & BOTS_RNG_BIT_MASK;
		CTR_PSX_OBSERVE_VALUE(firstPath);
		pathOrder[1] = firstPath;
		pathOrder[5] = firstPath ^ 1;

		secondPath = ((s32)RngDeadCoed(&BOTS_ADV_RNG) >> BOTS_RNG_BYTE_SHIFT) & BOTS_RNG_BIT_MASK;
		CTR_PSX_OBSERVE_VALUE(secondPath);
		pathOrder[2] = secondPath + 1;
		pathOrder[6] = (secondPath ^ 1) + 1;

		{
			register s32 i CTR_PSX_REGISTER("$17") = 0;
			register char *driverPaths CTR_PSX_REGISTER("$7") = BOTS_DRIVER_PATHS;
			register char *kartOrder CTR_PSX_REGISTER("$6") = BOTS_KART_ORDER;
			register u8 *pathBase CTR_PSX_REGISTER("$5") = pathOrder;
			s32 next;
			do
			{
				register s32 offset CTR_PSX_REGISTER("$4") = (s16)i;
				register char *kartSlot CTR_PSX_REGISTER("$2");
				register char *driverSlot CTR_PSX_REGISTER("$4");
				register u32 path CTR_PSX_REGISTER("$2");
				CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(kartSlot, kartOrder, offset);
				CTR_PSX_DEPEND_VALUE(i, offset);
				next = i + 1;
				CTR_PSX_COPY_VALUE(i, next);
				CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(driverSlot, driverPaths, offset);
				path = (u8)*kartSlot;
				path = pathBase[path];
				*driverSlot = path;
			} while ((s16)next < BOTS_MAX_KARTS);
		}
	}

	{
		register struct GameTracker *pathTracker = GAME_TRACKER;
		CTR_PSX_OBSERVE_VALUE(pathTracker);
		if ((pathTracker->gameMode1 & ADVENTURE_BOSS) != 0)
		{
			sdata->driver_pathIndexIDs[0] = 0;
			sdata->driver_pathIndexIDs[1] = 1;
		}

		if ((pathTracker->gameMode1 & BATTLE_MODE) != 0)
		{
			{
				register s32 i CTR_PSX_REGISTER("$17") = 0;
				s32 next;
				do
				{
					BOTS_DRIVER_PATHS[(s16)i] = ((s32)RngDeadCoed(&BOTS_ADV_RNG) & BOTS_BATTLE_PATH_RANDOM_MASK) / BOTS_BATTLE_PATH_RANDOM_DIVISOR;
					next = i + 1;
					CTR_PSX_COPY_VALUE(i, next);
				} while ((s16)next < BOTS_BATTLE_DRIVER_COUNT);
			}
		}
	}
	if ((((GAME_TRACKER->gameMode1 & ADVENTURE_CUP) == 0) && ((GAME_TRACKER->gameMode2 & CUP_ANY_KIND) == 0)) || (GAME_TRACKER->cup.trackIndex == 0))
	{
		u8 accelOrder[8];
		register s32 gridIndex CTR_PSX_REGISTER("$17") = 0;
		u32 accel = ((s32)RngDeadCoed(&BOTS_ADV_RNG) >> BOTS_RNG_BYTE_SHIFT) & BOTS_RNG_ACCEL_MASK;
		u32 rearAccel = ((s32)RngDeadCoed(&BOTS_ADV_RNG) >> BOTS_RNG_BYTE_SHIFT) & BOTS_RNG_ACCEL_MASK;

		{
			register u8 *orderBase CTR_PSX_REGISTER("$6") = accelOrder;
			s32 next;
			do
			{
				register s32 offset = (s16)gridIndex;
				register u8 *frontSlot CTR_PSX_REGISTER("$2");
				register u8 *rearSlot;
				register u32 rearValue CTR_PSX_REGISTER("$4");
				CTR_PSX_ADD_POINTER_OFFSET(frontSlot, orderBase, offset);
				*frontSlot = accel;
				CTR_PSX_FORGET_VALUE(rearAccel);
				accel++;
				CTR_PSX_FORGET_VALUE(accel);
				rearValue = rearAccel + BOTS_GRID_REAR_ROW_OFFSET;
				rearAccel--;
				CTR_PSX_FORGET_VALUE(rearAccel);
				accel &= BOTS_RNG_ACCEL_MASK;
				rearAccel &= BOTS_RNG_ACCEL_MASK;
				CTR_PSX_DEPEND_VALUE(gridIndex, offset);
				next = gridIndex + 1;
				CTR_PSX_COPY_VALUE(gridIndex, next);
				CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(rearSlot, orderBase, offset);
				rearSlot[BOTS_GRID_REAR_ROW_OFFSET] = rearValue;
			} while ((s16)next < BOTS_GRID_ROW_KART_COUNT);
		}


		{
			register s32 i CTR_PSX_REGISTER("$17") = 0;
			register u8 *accelDest CTR_PSX_REGISTER("$7") = BOTS_ACCEL_ORDER;
			register char *kartOrder CTR_PSX_REGISTER("$6") = BOTS_KART_ORDER;
			register u8 *accelBase CTR_PSX_REGISTER("$5") = accelOrder;
			s32 next;
			do
			{
				register s32 offset CTR_PSX_REGISTER("$4") = (s16)i;
				register char *kartSlot CTR_PSX_REGISTER("$2");
				register char *accelSlot CTR_PSX_REGISTER("$4");
				register u32 accelValue CTR_PSX_REGISTER("$2");
				CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(kartSlot, kartOrder, offset);
				CTR_PSX_DEPEND_VALUE(i, offset);
				next = i + 1;
				CTR_PSX_COPY_VALUE(i, next);
				CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(accelSlot, accelDest, offset);
				accelValue = (u8)*kartSlot;
				accelValue = accelBase[accelValue];
				*accelSlot = accelValue;
			} while ((s16)next < BOTS_MAX_KARTS);
		}
	}

	{
		register struct GameTracker *cupModeTracker CTR_PSX_REGISTER("$4") = GAME_TRACKER;
		CTR_PSX_OBSERVE_VALUE(cupModeTracker);
		if ((((cupModeTracker->gameMode1 & ADVENTURE_CUP) != 0) || ((cupModeTracker->gameMode2 & CUP_ANY_KIND) != 0)) && (cupModeTracker->cup.trackIndex > 0))
		{
			register s16 bestPoints CTR_PSX_REGISTER("$5");
			register s16 bestDriverIndex CTR_PSX_REGISTER("$7");
			register s16 topAccelIndex CTR_PSX_REGISTER("$8");
			register s32 i CTR_PSX_REGISTER("$17");
			bestPoints = -1;
			CTR_PSX_OBSERVE_VALUE(bestPoints);
			bestDriverIndex = 0;
			CTR_PSX_OBSERVE_VALUE(bestDriverIndex);
			CTR_PSX_COPY_VALUE(topAccelIndex, bestDriverIndex);
			CTR_PSX_COPY_VALUE(i, bestDriverIndex);
			{
				register struct GameTracker *cupTracker CTR_PSX_REGISTER("$6") = cupModeTracker;
				register s32 numPlayers CTR_PSX_REGISTER("$9");
				u8 *orderBase;
				s32 next;
				CTR_PSX_OBSERVE_VALUE(cupTracker);
				numPlayers = cupTracker->numPlyrCurrGame;
				orderBase = BOTS_ACCEL_ORDER;
				do
				{
					if (numPlayers <= (s16)i)
					{
						register s32 pointsOffset CTR_PSX_REGISTER("$2") = (s16)i * (s32)sizeof(s32);
						register const u8 *pointsBase CTR_PSX_REGISTER("$4");
						register s32 currentPoints;
						register s32 shiftedBest CTR_PSX_REGISTER("$2");
						CTR_PSX_ADD_POINTER_OFFSET(pointsBase, (const u8 *)cupTracker, pointsOffset);
						CTR_PSX_DEPEND_VALUE(bestPoints, pointsBase);
						shiftedBest = (s32)((u32)(s32)bestPoints << 16);
						CTR_PSX_LOAD_WORD_AFTER(currentPoints, *(const s32 *)(pointsBase + offsetof(struct GameTracker, cup.points)), shiftedBest);
						if ((shiftedBest >> 16) < currentPoints)
						{
							bestPoints = (s16)BOTS_ReadDifficultyUnsignedHalf((const s16 *)(pointsBase + offsetof(struct GameTracker, cup.points)));
							bestDriverIndex = (s16)i;
						}
					}
					{
						register s32 orderOffset CTR_PSX_REGISTER("$2") = (s16)i;
						register const u8 *orderSlot CTR_PSX_REGISTER("$2");
						CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(orderSlot, orderBase, orderOffset);
						if (*orderSlot == 0)
							topAccelIndex = (s16)i;
					}
					next = i + 1;
					CTR_PSX_COPY_VALUE(i, next);
				} while ((s16)next < BOTS_MAX_KARTS);
			}
			{
				register s32 bestOffset = bestDriverIndex;
				register s32 topOffset CTR_PSX_REGISTER("$2");
				register u8 *orderBase CTR_PSX_REGISTER("$4") = BOTS_ACCEL_ORDER;
				register u8 *bestSlot;
				register u8 *topSlot CTR_PSX_REGISTER("$2");
				register u32 topValue CTR_PSX_REGISTER("$4");
				register u32 bestValue CTR_PSX_REGISTER("$5");
				CTR_PSX_DEPEND_VALUE(bestOffset, bestDriverIndex);
				CTR_PSX_BIND_VALUE_CLOBBER(bestOffset, "$2");
				CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(bestSlot, orderBase, bestOffset);
				CTR_PSX_DEPEND_VALUE(topAccelIndex, bestSlot);
				topOffset = topAccelIndex;
				CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(topSlot, orderBase, topOffset);
				topValue = *topSlot;
				bestValue = *bestSlot;
				*bestSlot = topValue;
				*topSlot = bestValue;
			}
		}
	}
}

void BOTS_UpdateGlobals(void)
{
	register struct Driver *worstRobotDriver CTR_PSX_REGISTER("$5");
	register s32 i CTR_PSX_REGISTER("$4");
	register s32 nextIndex CTR_PSX_REGISTER("$2");
	register struct GameTracker *gGT CTR_PSX_REGISTER("$6");
	register u32 botFlag CTR_PSX_REGISTER("$7");

	if (GAME_TRACKER->numBotsNextGame != 0)
	{
		EngineSound_NearestAIs();
	}

	worstRobotDriver = NULL;
	i = 7;
	gGT = GAME_TRACKER;
	botFlag = ACTION_BOT;
	sdata->bestHumanRank = NULL;
	sdata->bestRobotRank = NULL;
	sdata->aiCollisionDelayFrameCount++;

	do
	{
		register struct Driver *d CTR_PSX_REGISTER("$3") = gGT->driversInRaceOrder[(s16)i];
		if (d != NULL)
		{
			if (d->actionsFlagSet & botFlag)
			{
				sdata->bestRobotRank = d;
				CTR_PSX_OBSERVE_MEMORY(sdata->bestRobotRank);
				if (worstRobotDriver == NULL)
				{
					worstRobotDriver = d;
				}
			}
			else
			{
				sdata->bestHumanRank = d;
			}
		}
		nextIndex = i - 1;
		CTR_PSX_COPY_VALUE(i, nextIndex);
	} while ((s16)nextIndex >= 0);

	if (sdata->bestHumanRank == NULL)
	{
		sdata->bestHumanRank = worstRobotDriver;
	}
}

void BOTS_SetRotation(struct Driver *bot, int useSpawnYaw)
{
	register int deltaNavX CTR_PSX_REGISTER("$23");
	register int deltaNavY CTR_PSX_REGISTER("$16");
	register int deltaNavZ CTR_PSX_REGISTER("$21");
	int xzDist;
	int xyzDist;
	register s32 xSquared CTR_PSX_REGISTER("$17");
	register s32 zSquared CTR_PSX_REGISTER("$18");
	register s32 ySquared CTR_PSX_REGISTER("$3");
	int rot;
	register int incomingSpawnYaw CTR_PSX_REGISTER("$5") = useSpawnYaw;
	register int spawnYaw CTR_PSX_REGISTER("$19");
	register int navY CTR_PSX_REGISTER("$16");
	register int estimateY CTR_PSX_REGISTER("$2");
	s16 v;
	register s32 posX CTR_PSX_REGISTER("$2") = bot->posCurr.x;
	register s32 posY CTR_PSX_REGISTER("$3") = bot->posCurr.y;
	register struct NavFrame *nf CTR_PSX_REGISTER("$22") = bot->botData.botNavFrame;

	CTR_PSX_ORDER_VALUES(posX, posY);
	bot->botData.aiPhysics.velocity.z = 0;
	bot->botData.aiPhysics.velocity.y = 0;
	bot->botData.aiPhysics.velocity.x = 0;
	CTR_PSX_OBSERVE_MEMORY(bot->botData.aiPhysics.velocity);

	// ======== Get Driver Position =============

	bot->botData.estimateNavFrame.pos.x = (s16)CTR_MipsSra(posX, FRACTIONAL_BITS_8);
	bot->botData.estimateNavFrame.pos.y = (s16)CTR_MipsSra(posY, FRACTIONAL_BITS_8);
	bot->botData.estimateNavFrame.pos.z = (s16)CTR_MipsSra(bot->posCurr.z, FRACTIONAL_BITS_8);
	CTR_PSX_RELOAD(bot->botData.estimateNavFrame.pos);

	// ======== Compare to Nav Position =============

	deltaNavX = CTR_MipsSubLo(nf->pos.x, bot->botData.estimateNavFrame.pos.x);
	xSquared = CTR_MipsMulLo(deltaNavX, deltaNavX);
	deltaNavZ = CTR_MipsSubLo(nf->pos.z, bot->botData.estimateNavFrame.pos.z);
	zSquared = CTR_MipsMulLo(deltaNavZ, deltaNavZ);
	navY = nf->pos.y;
	estimateY = bot->botData.estimateNavFrame.pos.y;
	CTR_PSX_DEPEND_VALUE(incomingSpawnYaw, estimateY);
	CTR_PSX_COPY_VALUE(spawnYaw, incomingSpawnYaw);
	deltaNavY = CTR_MipsSubLo(navY, estimateY);

	// ======== Calculate Distance =============

	// xz dist from driver to nav
	xzDist = SquareRoot0_stub(CTR_MipsAddLo(xSquared, zSquared));
	bot->botData.estimateNavFrame.distToNextNavXZ = xzDist;
	// xyz distance from driver to nav
	ySquared = CTR_MipsMulLo(deltaNavY, deltaNavY);
	CTR_PSX_OBSERVE_VALUE(ySquared);
	xyzDist = SquareRoot0_stub(CTR_MipsAddLo(CTR_MipsAddLo(xSquared, ySquared), zSquared));
	bot->botData.estimateNavFrame.distToNextNavXYZ = xyzDist;

	// ======== Calculate Rotation =============

	rot = ratan2(CTR_MipsSll(deltaNavY, BOTS_RATAN_ARG_SHIFT), CTR_MipsSll(bot->botData.estimateNavFrame.distToNextNavXZ, BOTS_RATAN_ARG_SHIFT));
	rot = CTR_MipsSra(rot, BOTS_ROT_BYTE_SHIFT);
	CTR_PSX_OBSERVE_VALUE(rot);
	// NOTE(aalhendi): Retail tests the low halfword; native retains the full API int.
	CTR_PSX_SHIFT_LEFT_IN_PLACE(spawnYaw, 16);
	CTR_PSX_OBSERVE_VALUE(spawnYaw);
	CTR_PSX_CAPTURE_REGISTER(spawnYaw, useSpawnYaw);
	bot->botData.estimateNavFrame.rot[3] = (u8)rot;
	bot->botData.navProgressRemainder = 0;

	if (spawnYaw)
	{
		bot->botData.estimateNavFrame.rot[1] =
		    (u8)CTR_MipsSra(CTR_MipsAddLo(GAME_TRACKER->level1->DriverSpawn[0].rot.y, BOTS_SPAWN_YAW_OFFSET), BOTS_ROT_BYTE_SHIFT);
	}
	else
	{
		bot->botData.estimateNavFrame.rot[0] = nf->rot[0];
		rot = ratan2(CTR_MipsNegLo(deltaNavX), CTR_MipsNegLo(deltaNavZ));
		bot->botData.estimateNavFrame.rot[1] = (u8)CTR_MipsSra(CTR_MipsAddLo(rot, BOTS_NAV_REVERSE_YAW_OFFSET), BOTS_ROT_BYTE_SHIFT);
		bot->botData.estimateNavFrame.rot[2] = nf->rot[2];
	}

	v = (s16)CTR_MipsSll(bot->botData.estimateNavFrame.rot[1], BOTS_ROT_BYTE_SHIFT);

	// Keep every AI-facing yaw cache in sync with the nav estimate.
	bot->botData.ai_rotY_608 = v;
	bot->angle = v;
	bot->rotCurr.y = v;
	bot->rotPrev.y = v;
	bot->botData.aiRot.y = v;

	bot->botData.botFlags |= BOT_FLAG_ESTIMATE_NAV;
}

void BOTS_LevInstColl(struct Thread *botThread)
{
	SVec3 currPos;
	SVec3 prevPos;
	struct InstDef *instDef;
	struct Instance *inst;
	struct MetaDataMODEL *mdm;
	register int modelID CTR_PSX_REGISTER("$4");
	struct Driver *driver = (struct Driver *)botThread->object;
	struct ScratchpadStruct *sps = CTR_SCRATCHPAD_PTR(struct ScratchpadStruct, 0x108);

	sps->ptr_mesh_info = GAME_TRACKER->level1->ptr_mesh_info;
	sps->Union.QuadBlockColl.searchFlags = COLL_SEARCH_TEST_INSTANCES;
	sps->Input1.modelID = DYNAMIC_ROBOT_CAR;
	sps->Union.QuadBlockColl.quadFlagsWanted = 0;
	sps->Union.QuadBlockColl.quadFlagsIgnored = 0;
	sps->Input1.hitRadius = BOTS_LEVEL_INST_COLL_RADIUS;

	currPos.x = (s16)CTR_MipsSra(driver->posCurr.x, FRACTIONAL_BITS_8);
	currPos.y = (s16)CTR_MipsAddLo(CTR_MipsSra(driver->posCurr.y, FRACTIONAL_BITS_8), BOTS_LEVEL_INST_COLL_RADIUS);
	currPos.z = (s16)CTR_MipsSra(driver->posCurr.z, FRACTIONAL_BITS_8);
	prevPos.x = (s16)CTR_MipsSra(driver->posPrev.x, FRACTIONAL_BITS_8);
	prevPos.y = (s16)CTR_MipsAddLo(CTR_MipsSra(driver->posPrev.y, FRACTIONAL_BITS_8), BOTS_LEVEL_INST_COLL_RADIUS);
	prevPos.z = (s16)CTR_MipsSra(driver->posPrev.z, FRACTIONAL_BITS_8);

	COLL_FIXED_BotsSearch(&currPos, &prevPos, sps);

	if (sps->boolDidTouchHitbox == 0)
	{
		return;
	}

	sps->Union.QuadBlockColl.searchFlags &= ~COLL_SEARCH_REUSE_NORMALS;

	if (((u8)sps->bspHitbox->flag & BSP_HITBOX_COLLIDABLE) == 0)
	{
		return;
	}

	instDef = sps->bspHitbox->data.hitbox.instDef;
	inst = instDef->ptrInstance;
	if (inst == NULL)
	{
		return;
	}

	// NOTE(aalhendi): Retail passes the signed low halfword; native keeps the full model ID.
	modelID = (s16)instDef->modelID;
	CTR_PSX_OBSERVE_VALUE(modelID);
	CTR_PSX_CAPTURE_REGISTER(modelID, instDef->modelID);
	mdm = COLL_LevModelMeta(modelID);
	if ((mdm != NULL) && (mdm->LInC != NULL))
	{
		mdm->LInC(inst, botThread, sps);
	}
}

void BOTS_ThTick_RevEngine(struct Thread *botThread)
{
	struct Driver *botDriver = (struct Driver *)botThread->object;
	register s32 currentY CTR_PSX_REGISTER("$3") = botDriver->posCurr.y;
	struct MaskHeadWeapon *mask = botDriver->botData.maskObj;

	if (botDriver->botData.positionBackup.y >= currentY)
	{
		if (mask != NULL)
		{
			mask->scale = MASK_HEAD_SCALE_NORMAL;
			mask->duration = 0;
			mask->rot.z &= ~MASK_HEAD_ROT_WORLD_SPACE;
		}

		botDriver->botData.maskObj = NULL;
		botDriver->kartState = KS_ENGINE_REVVING;
		botDriver->clockReceive = 0;
		botDriver->squishTimer = 0;

		ThTick_SetAndExec(botThread, BOTS_ThTick_Drive);
	}
	else
	{
		register s32 updatedY CTR_PSX_REGISTER("$2") = CTR_MipsSubLo(currentY, CTR_MipsSra(CTR_MipsSll(GAME_TRACKER->elapsedTimeMS, 9), 5));
		CTR_PSX_OBSERVE_VALUE(updatedY);
		botDriver->posCurr.y = updatedY;

		if (mask != NULL)
		{
			mask->pos.x = (s16)CTR_MipsSra(botDriver->posCurr.x, FRACTIONAL_BITS_8);
			mask->pos.y = (s16)CTR_MipsSra(botDriver->posCurr.y, FRACTIONAL_BITS_8);
			mask->pos.z = (s16)CTR_MipsSra(botDriver->posCurr.z, FRACTIONAL_BITS_8);
		}

		VehPhysForce_TranslateMatrix(botThread, botDriver);
		VehFrameProc_Driving(botThread, botDriver);
		VehEmitter_DriverMain(botThread, botDriver);
	}
}

void BOTS_MaskGrab(struct Thread *botThread)
{
	int midpoint;
	u32 groundActions;
	register struct Driver *maskDriver CTR_PSX_REGISTER("$4");
	register int spawnCoord CTR_PSX_REGISTER("$2");
	register int maskEnabled CTR_PSX_REGISTER("$5");
	register int spawnY CTR_PSX_REGISTER("$3");
	register int spawnZ CTR_PSX_REGISTER("$6");
	register int currentZ CTR_PSX_REGISTER("$5");
	register int nextZ CTR_PSX_REGISTER("$4");
	struct MaskHeadWeapon *mask;
	struct Driver *bot = botThread->object;
	register struct NavFrame *frame CTR_PSX_REGISTER("$6") = bot->botData.botNavFrame;
	register struct NavFrame *nextFrame CTR_PSX_REGISTER("$7") = frame + 1;

	// if the next nav point is a farther address than last point
	if (BOT_NAV_HEADERS[bot->botData.botPath]->last <= nextFrame)
	{
		// set next nav point to first nav point
		nextFrame = BOT_NAV_FRAMES[bot->botData.botPath];
	}

	bot->kartState = KS_MASK_GRABBED;

	bot->botData.navProgressRemainder = CTR_MipsSll((frame->distToNextNavXZ / BOTS_MASK_MIDPOINT_DIVISOR), FRACTIONAL_BITS_8);

	// midpointX between nav frames
	midpoint = CTR_MipsSll(CTR_MipsAddLo(frame->pos.x, (CTR_MipsSubLo(nextFrame->pos.x, frame->pos.x) / BOTS_MASK_MIDPOINT_DIVISOR)), FRACTIONAL_BITS_8);
	bot->botData.positionBackup.x = midpoint;
	bot->posPrev.x = midpoint;

	// midpointY between nav frames
	midpoint = CTR_MipsSll(CTR_MipsAddLo(frame->pos.y, (CTR_MipsSubLo(nextFrame->pos.y, frame->pos.y) / BOTS_MASK_MIDPOINT_DIVISOR)), FRACTIONAL_BITS_8);
	bot->botData.positionBackup.y = midpoint;
	bot->posPrev.y = midpoint;
	bot->quadBlockHeight = midpoint;

	// Read both Z endpoints before resetting independent physics fields.
	currentZ = frame->pos.z;
	groundActions = bot->actionsFlagSet;
	nextZ = nextFrame->pos.z;
	bot->botData.aiPhysics.speedLinear = 0;
	bot->botData.aiPhysics.speedY = 0;
	bot->botData.aiPhysics.reserved_0x5cc = 0;
	bot->botData.aiPhysics.velocity.z = 0;
	bot->botData.aiPhysics.velocity.y = 0;
	bot->botData.aiPhysics.velocity.x = 0;

	bot->actionsFlagSet = groundActions | ACTION_TOUCH_GROUND;
	bot->botData.botFlags &=
	    ~(BOT_FLAG_ESTIMATE_NAV | BOT_FLAG_DAMAGE_ACTIVE | BOT_FLAG_DAMAGE_SUPPRESS_EMITTER | BOT_FLAG_FREE_PHYSICS | BOT_FLAG_BOSS_PATH_REQUESTED);

	currentZ = CTR_MipsSll(CTR_MipsAddLo(currentZ, CTR_MipsSubLo(nextZ, currentZ) / BOTS_MASK_MIDPOINT_DIVISOR), FRACTIONAL_BITS_8);
	bot->botData.positionBackup.z = currentZ;
	bot->posPrev.z = currentZ;

	bot->rotCurr.x = (s16)CTR_MipsSll(frame->rot[0], BOTS_ROT_BYTE_SHIFT);
	bot->rotCurr.y = (s16)CTR_MipsSll(frame->rot[1], BOTS_ROT_BYTE_SHIFT);
	bot->rotCurr.z = (s16)CTR_MipsSll(frame->rot[2], BOTS_ROT_BYTE_SHIFT);

	bot->botData.aiPhysics.squishCooldown = 0;
	bot->botData.aiPhysics.mulDrift = 0;

	bot->turbo_MeterRoomLeft = 0;
	bot->reserves = 0;
	bot->clockReceive = 0;
	bot->squishTimer = 0;
	bot->turbo_outsideTimer = 0;
	bot->matrixArray = BAKED_GTE_MATRIX_NONE;
	bot->matrixIndex = 0;

	bot->actionsFlagSet &= ~(ACTION_AIRBORNE | ACTION_HIGH_JUMP);

	// if driver is not ghost
	if (botThread->modelIndex != DYNAMIC_GHOST)
	{
		// enable collision for this thread
		botThread->flags &= ~THREAD_FLAG_DISABLE_COLLISION;
	}

	// Preserve the midpoint stores before adding the drop height.
	maskDriver = bot;
	CTR_PSX_OBSERVE_VALUE(maskDriver);
	spawnCoord = bot->botData.positionBackup.x;
	spawnY = bot->botData.positionBackup.y;
	CTR_PSX_DEPEND_MEMORY(&bot->botData.positionBackup.z, spawnY);
	spawnZ = bot->botData.positionBackup.z;
	CTR_PSX_BIND_VALUE_CLOBBER(spawnZ, "$5");
	maskEnabled = true;
	CTR_PSX_OBSERVE_VALUE(maskEnabled);
	bot->posCurr.x = spawnCoord;
	CTR_PSX_OBSERVE_MEMORY(bot->posCurr.x);
	bot->posCurr.y = spawnY;
	CTR_PSX_OBSERVE_MEMORY(bot->posCurr.y);
	spawnCoord = spawnY;
	CTR_PSX_FORGET_VALUE(spawnCoord);
	spawnCoord = (s32)((u32)spawnCoord + BOTS_MASK_DROP_HEIGHT);
	bot->posCurr.z = spawnZ;
	bot->posCurr.y = spawnCoord;

	mask = VehPickupItem_MaskUseWeapon(maskDriver, maskEnabled);
	bot->botData.maskObj = mask;

	if (mask != 0)
	{
		mask->duration = MASK_HEAD_DURATION_NORMAL;
		mask->rot.z |= MASK_HEAD_ROT_WORLD_SPACE;
	}

	// execute, then assign per-frame to BOTS_ThTick_RevEngine
	ThTick_SetAndExec(botThread, BOTS_ThTick_RevEngine);

	return;
}

void BOTS_Killplane(struct Thread *botThread)
{
	register struct Thread *savedThread CTR_PSX_REGISTER("$18") = botThread;
	register struct Driver *bot CTR_PSX_REGISTER("$16");
	register b32 usingTinyArenaOverride CTR_PSX_REGISTER("$17");
	register b32 overrideActive CTR_PSX_REGISTER("$2");
	register struct GameTracker *tracker CTR_PSX_REGISTER("$4");
	register u32 arenaPage CTR_PSX_REGISTER("$5");
	register const char *arenaName CTR_PSX_REGISTER("$5");
	CTR_PSX_BIND_VALUE_CLOBBER(savedThread, "$17");
	usingTinyArenaOverride = false;
	CTR_PSX_LOAD_SYMBOL_PAGE_AFTER(arenaPage, "rdata", usingTinyArenaOverride);
	tracker = GAME_TRACKER;
	CTR_PSX_DEPEND_VALUE(arenaPage, tracker);
	CTR_PSX_ADD_SYMBOL_LOW(arenaName, arenaPage, "rdata", rdata.s_asphalt2_thisAppearsTwice);
	CTR_PSX_DEPEND_VALUE(savedThread, arenaName);
	bot = savedThread->object;

	// check for Tiny Arena
	if (strcmp(tracker->levelName, arenaName) == 0)
	{
		// edge-case override?
		register s32 override CTR_PSX_REGISTER("$6");
		register s32 selectedOverride CTR_PSX_REGISTER("$3");
		register s32 checkpoint CTR_PSX_REGISTER("$3") = bot->checkpoint.currentIndex;
		CTR_PSX_BIND_VALUE_CLOBBER(checkpoint, "$6");
		override = 0xff;
		if (checkpoint == 0x94)
		{
			override = 0x84;
		}
		else if (checkpoint == 0xa0)
		{
			override = 0x80;
		}

		selectedOverride = override;
		CTR_PSX_OBSERVE_VALUE(selectedOverride);
		if (selectedOverride != 0xff)
		{
			// pointer to nav point
			register struct NavFrame *frame CTR_PSX_REGISTER("$5") = bot->botData.botNavFrame;

			// goBackCount
			register s32 backCount CTR_PSX_REGISTER("$4") = frame->goBackCount;

			while (backCount < (selectedOverride - 1) || selectedOverride + 1 < backCount)
			{
				// nav path index
				s16 i = bot->botData.botPath;

				// go back to previous point
				frame -= 1;

				// if this is less than address of first nav point
				if (frame < BOT_NAV_FRAMES[i])
				{
					// go to last nav point
					register struct NavFrame *last CTR_PSX_REGISTER("$2") = BOT_NAV_HEADERS[i]->last;
					CTR_PSX_OBSERVE_VALUE(last);
					frame = &last[-1];
				}

				backCount = frame->goBackCount;
				selectedOverride = override;
				CTR_PSX_OBSERVE_VALUE(selectedOverride);
			}
			CTR_PSX_OBSERVE_VALUE(selectedOverride);
			bot->botData.botNavFrame = frame;
			CTR_PSX_OBSERVE_MEMORY(bot->botData.botNavFrame);
			usingTinyArenaOverride = true;
		}
	}

	// if not Tiny Arena, or goBackCount didn't happen
	overrideActive = usingTinyArenaOverride;
	CTR_PSX_OBSERVE_VALUE(overrideActive);
	if (!overrideActive)
	{
		// pointer to navFrame
		register struct NavFrame *frame CTR_PSX_REGISTER("$4") = bot->botData.botNavFrame;

		// current nav point (player turned AI)
		register u32 currNav CTR_PSX_REGISTER("$2") = bot->checkpoint.currentIndex;

		// goBackCount
		register s32 backCount CTR_PSX_REGISTER("$3") = frame->goBackCount;

		while ((backCount == (s32)currNav || ((frame->flags & 0x4000) != 0)))
		{
			// nav path index
			s16 i = bot->botData.botPath;

			// go back one navFrame
			frame -= 1;

			// if you go back to far
			if (frame < BOT_NAV_FRAMES[i])
			{
				// loop back to last navFrame
				register struct NavFrame *last CTR_PSX_REGISTER("$2") = BOT_NAV_HEADERS[i]->last;
				CTR_PSX_OBSERVE_VALUE(last);
				frame = &last[-1];
			}
			backCount = frame->goBackCount;
			currNav = bot->checkpoint.currentIndex;
		}
		// save ptr to nav frame
		bot->botData.botNavFrame = frame;
	}

	BOTS_MaskGrab(savedThread);
	return;
}


void BOTS_ThTick_Drive(struct Thread *botThread)
{
	int elapsedMilliseconds;
	int speedApprox;
	u32 local_38;
	s16 tireColorStep;
	struct NavFrame *navFrameCurr;
	struct NavFrame *navFrameNext; // psVar21
	struct DriverCollisionSearch driverSearch;
	struct Thread *hitThread;
	int deltaPosThisFrame; // iVar4
	int navFrameFlags;
	int navFrameSpecialBits;
	int gravity;
	s16 navDist; // sVar7
	int local_3c;
	b32 useGroundedNavDistance;
	int iVar15;
	int iVar3;
	int navActionFlags;
	struct Terrain *terrain;
	int percentage; // iVar13
	int iVar4_lifetime_3;
	s16 iVar3_lifetime_2;
	u8 cmp1;
	u8 cmp2;
	int badnessRecieveTimer;
	int iVar4_lifetime_2;
	int uVar11;
	s16 camRot;
	struct ScratchpadStruct *sps = CTR_SCRATCHPAD_PTR(struct ScratchpadStruct, 0x108);
	struct Driver *botDriver = (struct Driver *)botThread->object;     // iVar17
	struct Instance *botInstance = (struct Instance *)botThread->inst; // iVar22
	struct GameTracker *gGT = sdata->gGT;

	// local_34 = gGT->elapsedTimeMS;

#if 0
	if(botDriver->driverID != 0)
		return;
#endif

	botDriver->turbo_MeterRoomLeft = 0;
	botDriver->forwardDir = 0;

	botInstance->flags &= ~(SPLIT_LINE | REFLECTIVE);

	if (botDriver->botData.weaponCooldown != 0)
	{
		botDriver->botData.weaponCooldown = (s16)CTR_MipsSubLo((u16)botDriver->botData.weaponCooldown, 1);
	}

	if (botDriver->pendingDamageType == 0)
	{
		if (((botDriver->actionsFlagSet & ACTION_RACE_FINISHED) == 0) && (botDriver->botData.weaponCooldown != 0))
		{
			botDriver->botData.weaponCooldown = (s16)CTR_MipsSubLo((u16)botDriver->botData.weaponCooldown, 1);
		}
	}
	else
	{
		BOTS_ChangeState(botDriver, botDriver->pendingDamageType, botDriver->pendingDamageAttacker, botDriver->pendingDamageReasonByte);
	}

	elapsedMilliseconds = gGT->elapsedTimeMS; // local_34

	botDriver->reserves = (s16)CTR_MipsSubLo((u16)botDriver->reserves, elapsedMilliseconds);
	if (botDriver->reserves < 0)
	{
		botDriver->reserves = 0;
	}

	botDriver->turbo_outsideTimer = (s16)CTR_MipsSubLo((u16)botDriver->turbo_outsideTimer, elapsedMilliseconds);
	if (botDriver->turbo_outsideTimer < 0)
	{
		botDriver->turbo_outsideTimer = 0;
	}

	botDriver->squishTimer = (s16)CTR_MipsSubLo((u16)botDriver->squishTimer, elapsedMilliseconds);
	if (botDriver->squishTimer < 0)
	{
		botDriver->squishTimer = 0;
	}

	botDriver->burnTimer = (s16)CTR_MipsSubLo((u16)botDriver->burnTimer, elapsedMilliseconds);
	if (botDriver->burnTimer < 0)
	{
		botDriver->burnTimer = 0;
	}

	botDriver->clockReceive = (s16)CTR_MipsSubLo((u16)botDriver->clockReceive, elapsedMilliseconds);
	if (botDriver->clockReceive < 0)
	{
		botDriver->clockReceive = 0;
	}

	botDriver->botData.aiPhysics.driftTarget = 0;
	botDriver->kartState = KS_NORMAL;

	botDriver->actionsFlagSetPrevFrame = botDriver->actionsFlagSet;
	botDriver->actionsFlagSet &= ~(ACTION_DROPPING_MINE | ACTION_JUMP_STARTED | ACTION_STARTED_TOUCH_GROUND);

	speedApprox = botDriver->speedApprox; // iVar4

	if (speedApprox < 0)
	{
		speedApprox = CTR_MipsNegLo(speedApprox);
	}

	if ((botDriver->actionsFlagSet & ACTION_TOUCH_GROUND) == 0)
	{
		speedApprox = CTR_MipsAddLo(speedApprox, DRIVER_TIRE_COLOR_SPEED_AIRBORNE_BONUS);
	}
	else
	{
		speedApprox = CTR_MipsSra(CTR_MipsSll(speedApprox, 1), 1);
	}

	// NOTE(aalhendi): Retail initializes this boost scale before catch-up can overwrite it.
	local_38 = 1;
	tireColorStep = (s16)CTR_MipsSra(CTR_MipsSll(CTR_MipsAddLo(CTR_MipsMulLo(speedApprox, DRIVER_TIRE_COLOR_SPEED_WEIGHT),
	                                                           CTR_MipsMulLo(botDriver->tireColorCycleStep, DRIVER_TIRE_COLOR_STEP_WEIGHT)),
	                                             3),
	                                 0xC);
	botDriver->tireColorCycleStep = tireColorStep;

	if ((botDriver->actionsFlagSetPrevFrame & ACTION_ACCEL_PREVENTION) == 0)
	{
		int baseSpeed = botDriver->baseSpeed;
		if (baseSpeed < 0)
		{
			baseSpeed = CTR_MipsNegLo(baseSpeed);
		}

		if (baseSpeed <= DRIVER_TIRE_COLOR_LOW_SPEED_THRESHOLD)
		{
			baseSpeed = botDriver->speedApprox;
			if (baseSpeed < 0)
			{
				baseSpeed = CTR_MipsNegLo(baseSpeed);
			}

			if (baseSpeed <= DRIVER_TIRE_COLOR_LOW_SPEED_THRESHOLD)
			{
				goto UpdateTireColorTimer;
			}
		}

		botDriver->tireColorCycleTimer = (s16)CTR_MipsSubLo((u16)botDriver->tireColorCycleTimer, tireColorStep);
	}
UpdateTireColorTimer:

	if ((botDriver->tireColorCycleTimer < 1) && ((botDriver->tireColor & 1) == 0))
	{
		botDriver->tireColorCycleTimer = DRIVER_TIRE_COLOR_TIMER_RESET;
		botDriver->tireColor = DRIVER_TIRE_COLOR_DARK;
	}
	else
	{
		botDriver->tireColor = DRIVER_TIRE_COLOR_DEFAULT;
	}

	navFrameCurr = &botDriver->botData.estimateNavFrame; // psVar19


	if ((botDriver->botData.botFlags & BOT_FLAG_ESTIMATE_NAV) == 0)
	{
		int pathID;
		navFrameCurr = botDriver->botData.botNavFrame;
		navFrameNext = navFrameCurr + 1;

		pathID = botDriver->botData.botPath;

		if (navFrameNext >= sdata->NavPath_ptrHeader[pathID]->last)
		{
			navFrameNext = sdata->NavPath_ptrNavFrameArray[pathID];
		}
	}
	else
	{
		navFrameNext = botDriver->botData.botNavFrame;
	}


	CTR_SET_VEC3(CTR_VECTOR_DATA(&(driverSearch.bucket.pos)), (s16)CTR_MipsSra(botDriver->posCurr.x, FRACTIONAL_BITS_8),
	             (s16)CTR_MipsSra(botDriver->posCurr.y, FRACTIONAL_BITS_8), (s16)CTR_MipsSra(botDriver->posCurr.z, FRACTIONAL_BITS_8));
	driverSearch.bucket.th = NULL;
	driverSearch.bucket.bestDistSq = 0x7fffffff;

	if ((botThread->flags & (THREAD_FLAG_DEAD | THREAD_FLAG_DISABLE_COLLISION)) == 0)
	{
		if (botThread->modelIndex == DYNAMIC_PLAYER)
		{
			PROC_CollidePointWithBucket(botThread->siblingThread, &driverSearch.bucket);
			PROC_CollidePointWithBucket(gGT->threadBuckets[ROBOT].thread, &driverSearch.bucket);
		}
		else if (botThread->modelIndex == DYNAMIC_ROBOT_CAR)
		{
			PROC_CollidePointWithBucket(botThread->siblingThread, &driverSearch.bucket);
		}
	}

	hitThread = driverSearch.bucket.th;
	if (hitThread != NULL)
	{
		int combinedRadius = CTR_MipsAddLo(botThread->driverHitRadius, hitThread->driverHitRadius);
		if (driverSearch.bucket.bestDistSq < CTR_MipsMulLo(combinedRadius, combinedRadius))
		{
			Vec3 selfVelocity;
			selfVelocity.x = CTR_MipsAddLo(botDriver->xSpeed, botDriver->botData.aiPhysics.accel.x);
			selfVelocity.y = CTR_MipsAddLo(botDriver->ySpeed, botDriver->botData.aiPhysics.accel.y);
			selfVelocity.z = CTR_MipsAddLo(botDriver->zSpeed, botDriver->botData.aiPhysics.accel.z);
			VehPhysCrash_AnyTwoCars(botThread, &driverSearch, &selfVelocity);
		}
	}


	if (botDriver->botData.ai_progress_cooldown == 0)
	{
		u32 botFlags;
		int trafficLightsTimer = gGT->trafficLightsTimer;

		if (data.characterIDs[botDriver->driverID] == NITROS_OXIDE) // check is oxide
		{
			// Pretend there is less time (oxide is a cheater)
			trafficLightsTimer = CTR_MipsSubLo(trafficLightsTimer, 0x1e0);
		}

		if (0 < trafficLightsTimer)
		{
			goto HoldBotBeforeRaceOrCooldown; // if race not started, then skip
		}

		if ((gGT->boolDemoMode != 0) && ((botDriver->botData.botFlags & BOT_FLAG_DEMO_CAMERA_STARTED) == 0) && (botThread->modelIndex == DYNAMIC_PLAYER))
		{
			botDriver->botData.botFlags |= BOT_FLAG_DEMO_CAMERA_STARTED;
			CAM_EndOfRace(&gGT->cameraDC[botDriver->driverID], botDriver);
		}

		if ((botDriver->botData.botFlags & BOT_FLAG_STARTLINE_INIT_DONE) == 0)
		{
			// first frame of race
			botDriver->botData.botFlags |= BOT_FLAG_STARTLINE_INIT_DONE;

			if (data.characterIDs[botDriver->driverID] == NITROS_OXIDE)
			{ // if oxide, then talk
				Voiceline_RequestPlay(0, 0xf, 0x10);
			}

			if (( // if in front row & 25% chance
			        (sdata->kartSpawnOrderArray[botDriver->driverID] < 3) && ((RngDeadCoed(&sdata->advRng) & 0xFF) < 0x40)) ||
			    (data.characterIDs[botDriver->driverID] == NITROS_OXIDE))
			{ // start the race with a boost
				VehFire_Increment(botDriver, 0x2d0, 1, 0x180);

				VehFire_Audio(botDriver, 0x180);
			}
		}

		// between these start and end tags, there's confusing variable lifetime that needs to be looked at closer.

		botFlags = botDriver->botData.botFlags; // uVar8

		// if path change is requested, but not started (BOSS ONLY)
		if ((botFlags & (BOT_FLAG_BOSS_PATH_REQUESTED | BOT_FLAG_BOSS_PATH_ACTIVE)) == BOT_FLAG_BOSS_PATH_REQUESTED)
		{
			s16 changeOpcode = navFrameNext->pathChangeOpcode;
			s16 newPathID = BOTS_PathChangePathID(changeOpcode);
			s16 newFrameIndex = BOTS_PathChangeFrameIndex(changeOpcode);
			s16 cap = BOTS_PathChangeCap(); // 0xC00, cant have path[3]

			if ((changeOpcode < cap) && (newPathID == botDriver->botData.desiredPath_BossOnly))
			{
				s16 oldPathID;
				struct NavFrame *firstNavFrameOnPath;
				// record that change is requested (boss only)
				botDriver->botData.botFlags = botFlags | BOT_FLAG_BOSS_PATH_ACTIVE;

				oldPathID = botDriver->botData.botPath;
				botDriver->botData.botPath = newPathID;

				LIST_RemoveMember(&sdata->navBotList[oldPathID], &botDriver->botData.item);
				LIST_AddFront(&sdata->navBotList[newPathID], &botDriver->botData.item);

				firstNavFrameOnPath = sdata->NavPath_ptrNavFrameArray[newPathID];
				botDriver->botData.botNavFrame = &firstNavFrameOnPath[newFrameIndex];

				BOTS_SetRotation(botDriver, 0);

				navFrameNext = botDriver->botData.botNavFrame;
				navFrameCurr = &botDriver->botData.estimateNavFrame;
			}
		}
		else
		{
			if (BOTS_RACE_START_AI_COLLISION_DELAY_FRAMES < sdata->aiCollisionDelayFrameCount)
			{
				struct Driver *otherDriver = NULL; // iVar4
				if ((botFlags & BOT_FLAG_ESTIMATE_NAV) == 0)
				{
					int iVar3 = 1000;
					s16 sVar7 = 1000;
					struct BotData *botData;

					for (botData = (struct BotData *)LIST_GetFirstItem(&sdata->navBotList[botDriver->botData.botPath]); botData != NULL;
					     botData = (struct BotData *)LIST_GetNextItem((struct Item *)botData), sVar7 = (s16)iVar3)
					{
						int iVar13;
						struct Driver *driverFromBotData = (struct Driver *)((char *)botData - offsetof(struct Driver, botData));

						if (driverFromBotData == botDriver)
						{
							continue;
						}


						// Find the signed index difference between nav frames on this path.
						iVar13 = (int)(botData->botNavFrame - botDriver->botData.botNavFrame);

						// if "other" botData driver is behind "this" botDriver driver,
						if (iVar13 < 0)
						{
							// assume number of points "away" is large (add track length)
							iVar13 = CTR_MipsAddLo(iVar13, sdata->NavPath_ptrHeader[botDriver->botData.botPath]->numPoints);
						}

						// find closest "other" botData driver
						// that is AHEAD of "this" botDriver driver
						if (iVar13 < iVar3)
						{
							// save number of points ahead,
							// and save pointer to other driver
							iVar3 = iVar13;
							otherDriver = driverFromBotData;
						}
					}

					// If two drivers are within 3 navframe points of each other
					if ((otherDriver != NULL) && (sVar7 < 3))
					{
						int diff = CTR_MipsSubLo(botDriver->distanceToFinish_curr, otherDriver->distanceToFinish_curr);

						// if "this" bot is closer to finish than "other" bot, do nothing,
						// because "diff" will be incremented largely and fail the <0x200 check
						if (diff < 0)
						{
							diff = CTR_MipsAddLo(diff, BOTS_GetTrackDistanceToFinish(gGT));
						}

						// if "this" bot driver is behind "other" driver, and very close to them,
						// while also being on the same nav path, then change the nav path
						if (diff < 0x200)
						{
							s16 changeOpcode = navFrameNext->pathChangeOpcode;
							s16 newPathID = BOTS_PathChangePathID(changeOpcode);
							s16 newFrameIndex = BOTS_PathChangeFrameIndex(changeOpcode);
							s16 cap = BOTS_PathChangeCap(); // 0xC00, cant have path[3]

							if (changeOpcode < cap)
							{
								struct NavFrame *firstNavFrameOnPath;
								s16 oldPathID = botDriver->botData.botPath;
								botDriver->botData.botPath = newPathID;

								LIST_RemoveMember(&sdata->navBotList[oldPathID], &botDriver->botData.item);
								LIST_AddFront(&sdata->navBotList[newPathID], &botDriver->botData.item);

								firstNavFrameOnPath = sdata->NavPath_ptrNavFrameArray[newPathID];
								botDriver->botData.botNavFrame = &firstNavFrameOnPath[newFrameIndex & BOTS_PATH_CHANGE_FRAME_MASK];

								BOTS_SetRotation(botDriver, 0);

								navFrameNext = botDriver->botData.botNavFrame;
								navFrameCurr = &botDriver->botData.estimateNavFrame;
							}
						}
					}
				}
			}
		}

		// puVar5 = gGT but different?

		if ((botDriver->actionsFlagSet & ACTION_TOUCH_GROUND) == 0)
		{
			int ZYsqr = CTR_MipsAddLo(CTR_MipsMulLo(botDriver->botData.aiPhysics.speedLinear, botDriver->botData.aiPhysics.speedLinear),
			                          CTR_MipsMulLo(botDriver->botData.aiPhysics.speedY, botDriver->botData.aiPhysics.speedY));

			if (0x2b110000 < ZYsqr) // sqrZY
			{
				int ZY = SquareRoot0_stub(ZYsqr);

				int zVel = CTR_MipsMulLo(botDriver->botData.aiPhysics.speedLinear, 0x6900); // iVar3

#if 0 // in the OG game
				if (ZY == 0)
				{
					//trap(0x1c00);
				}
				if ((ZY == -1) && (zVel == -0x80000000))
				{
					//trap(0x1800);
				}
#endif
				int yVel = CTR_MipsMulLo(botDriver->botData.aiPhysics.speedY, 0x6900); // iVar15
#if 0                                                                                  // in the OG game
				if (ZY == 0)
				{
					//trap(0x1c00);
				}
				if ((ZY == -1) && (yVel == -0x80000000))
				{
					//trap(0x1800);
				}
#endif
				botDriver->botData.aiPhysics.speedLinear = CTR_MipsDiv(zVel, ZY);
				botDriver->botData.aiPhysics.speedY = CTR_MipsDiv(yVel, ZY);
			}
		}
		else
		{
			struct Driver *bestDriverRank;
			int botVelocity; // iVar3
			struct Terrain *botTerrain;
			int velocityAccountingForTerrain;
			int levelID;
			botDriver->actionsFlagSet &= ~ACTION_AIRBORNE;

			bestDriverRank = sdata->bestHumanRank; // iVar4

			if ((gGT->gameMode1 & RELIC_RACE) != 0)
			{
				bestDriverRank = sdata->bestRobotRank;
			}


			if ((bestDriverRank == NULL) || (bestDriverRank == botDriver))
			{
				botVelocity = gGT->constVal_9000;
			}
			else
			{
				s16 difficultyStat; // sVar7
				int distToFinish;
				int lapIndex;
				int botLapIndex;
				int complexDifficultyStat;
				int otherDifficultyStat; // iVar13
				int iVar9;
				int bestDriverWumpaCount;
				int turboMult;
				int netSpeedStat;
				int additionalVelocity;
				int driverRank = botDriver->driverRank; // uVar8
				b32 isInAdvArcadeOrVSCup = false;       // bVar1

				if (((gGT->gameMode1 & ADVENTURE_CUP) != 0) || ((gGT->gameMode2 & CUP_ANY_KIND) != 0))
				{
					isInAdvArcadeOrVSCup = true;
				}

				if (gGT->drivers[0]->driverRank < botDriver->driverRank)
				{
					driverRank--;
				}

				if (1 < gGT->numPlyrCurrGame && gGT->drivers[1]->driverRank <= driverRank)
				{
					driverRank--;
				}

				if (isInAdvArcadeOrVSCup)
				{
					driverRank = sdata->accelerateOrder[botDriver->driverID];
					if (sdata->accelerateOrder[0] < 2)
					{
						driverRank--;
					}
					if (sdata->accelerateOrder[1] < 2)
					{
						driverRank--;
					}
				}


				if (bestDriverRank->lapIndex == 0)
				{
					difficultyStat = sdata->arcade_difficultyParams[0xB];
				}
				else
				{
					if (bestDriverRank->lapIndex == gGT->numLaps - 1)
					{
						difficultyStat = sdata->arcade_difficultyParams[0xD];
					}
					else
					{
						difficultyStat = sdata->arcade_difficultyParams[0xC];
					}
				}

				distToFinish = BOTS_GetTrackDistanceToFinish(gGT); // iVar3

				lapIndex = bestDriverRank->lapIndex; // uVar20

				if ((bestDriverRank->actionsFlagSet & ACTION_BEHIND_START_LINE) != 0)
				{
					lapIndex--;
				}

				botLapIndex = botDriver->lapIndex; // uVar18

				if ((botDriver->actionsFlagSet & ACTION_BEHIND_START_LINE) != 0)
				{
					botLapIndex--;
				}

				// not super meaningful, probably just a difficulty statistic
				complexDifficultyStat = CTR_MipsSubLo(
				    CTR_MipsSubLo(CTR_MipsAddLo(CTR_MipsSubLo(distToFinish, bestDriverRank->distanceToFinish_curr), CTR_MipsMulLo(lapIndex, distToFinish)),
				                  CTR_MipsAddLo(CTR_MipsSubLo(distToFinish, botDriver->distanceToFinish_curr), CTR_MipsMulLo(botLapIndex, distToFinish))),
				    CTR_MipsAddLo(sdata->arcade_difficultyParams[driverRank], difficultyStat));


				if (isInAdvArcadeOrVSCup && ((driverRank & 0xffff) == 0))
				{
					if (complexDifficultyStat < 1)
					{
						otherDifficultyStat = sdata->arcade_difficultyParams[0x9];
					}
					else
					{
						if (botDriver->lapIndex < gGT->numLaps - 1)
						{
							otherDifficultyStat = sdata->cup_difficultyParams[0x8];
						}
						else
						{
							otherDifficultyStat = sdata->cup_difficultyParams[0x8] + sdata->cup_difficultyParams[0xA];
						}
					}
				}
				else
				{
					if (complexDifficultyStat < 1)
					{
						otherDifficultyStat = sdata->arcade_difficultyParams[0x9];
					}
					else
					{
						if (gGT->numLaps - 1 <= botDriver->lapIndex)
						{
							otherDifficultyStat = sdata->arcade_difficultyParams[0x8] + sdata->arcade_difficultyParams[0xA];
						}
						else
						{
							otherDifficultyStat = sdata->arcade_difficultyParams[0x8];
						}
					}
				}
				botVelocity = complexDifficultyStat; // iVar3
				if (complexDifficultyStat < 0)
				{
					botVelocity = CTR_MipsNegLo(complexDifficultyStat);
				}
				iVar9 = otherDifficultyStat;
				if (otherDifficultyStat < 0)
				{
					iVar9 = CTR_MipsNegLo(otherDifficultyStat);
				}

				botVelocity = CTR_MipsSra(CTR_MipsMulLo(iVar9, CTR_MipsDiv(CTR_MipsSll(CTR_MipsAddLo(botVelocity, 0x80), 0xc), 0xa00)), 0xc);
				local_38 = 0 < complexDifficultyStat;
				botVelocity = CTR_MipsAddLo(botVelocity, CTR_MipsMulLo(CTR_MipsDiv(CTR_MipsSll(botVelocity, 3), 100), CTR_MipsSubLo(7, driverRank)));

				if (iVar9 < botVelocity)
				{
					botVelocity = iVar9;
				}
				if (otherDifficultyStat < 0)
				{
					botVelocity = CTR_MipsNegLo(iVar9);
				}

				bestDriverWumpaCount = bestDriverRank->numWumpas; // iVar15, assume max of 9?
				if (9 < bestDriverWumpaCount)
				{
					bestDriverWumpaCount = 9;
				}

				turboMult = bestDriverRank->turboConst; // iVar13, max of 5
				if (5 < turboMult)
				{
					turboMult = 5;
				}

				netSpeedStat = CTR_MipsSubLo(
				    CTR_MipsDiv(CTR_MipsSll(CTR_MipsSubLo(bestDriverRank->const_AccelSpeed_ClassStat, bestDriverRank->const_Speed_ClassStat), 0xc), 5), 1);

				additionalVelocity = CTR_MipsAddLo(
				    CTR_MipsDiv(CTR_MipsSll(bestDriverRank->const_Speed_ClassStat, 3), 10),
				    CTR_MipsSra(CTR_MipsAddLo(CTR_MipsDiv(CTR_MipsMulLo(bestDriverWumpaCount, netSpeedStat), 10), CTR_MipsMulLo(turboMult, netSpeedStat)),
				                0xc));

				if (additionalVelocity > 0x6900)
				{
					additionalVelocity = 0x6900;
				}

				botVelocity = CTR_MipsAddLo(botVelocity, additionalVelocity);

				if (botVelocity < 0x5dc1)
				{
					if (botVelocity < 0x1c20)
					{
						botVelocity = 0x1c20;
					}
				}
				else
				{
					botVelocity = 24000;
				}
			}

			if (botDriver->reserves == 0)
			{
				botDriver->botData.botFlags &= ~BOT_FLAG_NAV_BOOST_ACTIVE;
			}
			else
			{
				if ((botDriver->botData.botFlags & BOT_FLAG_NAV_BOOST_ACTIVE) == 0)
				{
					botVelocity = CTR_MipsAddLo(botVelocity, botDriver->fireSpeedCap);
				}
				else
				{
					botVelocity = CTR_MipsAddLo(botDriver->fireSpeedCap, 10000);
				}
			}

			if (botDriver->clockReceive != 0 || botDriver->squishTimer != 0)
			{
				botVelocity = CTR_MipsSra(CTR_MipsMulLo(botVelocity, 0xc00), 0xc);
			}

			if ((botDriver->botData.botFlags & BOT_FLAG_DAMAGE_ACTIVE) == 0)
			{
				if (botDriver->instTntRecv != NULL || botDriver->thCloud != NULL)
				{
					int damagedVelocityPenalty = CTR_MipsSra(botDriver->const_DamagedSpeed, 1); // iVar4

					botVelocity = CTR_MipsSubLo(botVelocity, damagedVelocityPenalty);
				}
			}
			else
			{
				botDriver->botData.aiPhysics.turboMeter = 0;

				if (data.characterIDs[botDriver->driverID] == NITROS_OXIDE)
				{
					int damagedVelocityPenalty = CTR_MipsSra(botDriver->const_DamagedSpeed, 2); // iVar4

					botVelocity = CTR_MipsSubLo(botVelocity, damagedVelocityPenalty);
				}
				else
				{
					botVelocity = CTR_MipsSubLo(botVelocity, botDriver->const_DamagedSpeed);
				}
			}
			if (botVelocity < 0)
			{
				botVelocity = 0;
			}

			botTerrain = botDriver->terrainMeta1; // iVar15

			botDriver->botData.aiPhysics.speedLinear =
			    CTR_MipsSubLo(botDriver->botData.aiPhysics.speedLinear,
			                  CTR_MipsSra(CTR_MipsMulLo(botDriver->const_PedalFriction_Forward, botTerrain->botFrictionScale), 8)); // iVar4

			if (botDriver->botData.aiPhysics.speedLinear < 0)
			{
				botDriver->botData.aiPhysics.speedLinear = 0;
			}

			if (0x6900 < botVelocity)
			{
				botVelocity = 0x6900;
			}

			velocityAccountingForTerrain = CTR_MipsSra(CTR_MipsMulLo(botVelocity, botTerrain->bot.fields.targetSpeedScale), 8); // iVar4

			if ((botTerrain->bot.fields.speedFlags & TERRAIN_BOT_FLAG_DECEL_TO_TARGET_SPEED) == 0)
			{
			CheckAccelerationTowardTerrainTarget:
				if (botDriver->botData.aiPhysics.speedLinear < velocityAccountingForTerrain)
				{
					s16 accel; // sVar7
				ApplyTerrainAcceleration:;

					if (botDriver->reserves < 1)
					{
						accel = botDriver->const_Accel_ClassStat;
					}
					else
					{
						accel = botDriver->const_Accel_Reserves;
					}
					botVelocity = CTR_MipsSra(CTR_MipsMulLo(accel, botTerrain->bot.fields.accelerationScale), 8);

					if (botDriver->botData.botAccel != 0)
					{
						botDriver->botData.botAccel--;
						botVelocity = CTR_MipsSra(CTR_MipsMulLo(botVelocity, CTR_MipsSubLo(0x100, CTR_MipsMulLo(sdata->AI_AccelFrameSteps,
						                                                                                        sdata->accelerateOrder[botDriver->driverID]))),
						                          8);
					}

					botDriver->botData.aiPhysics.speedLinear = CTR_MipsAddLo(botDriver->botData.aiPhysics.speedLinear, botVelocity);
				}
			}
			else
			{
				botVelocity = CTR_MipsSra(botDriver->botData.aiPhysics.speedLinear, 1);

				if (botDriver->botData.aiPhysics.speedLinear < velocityAccountingForTerrain)
				{
					goto ApplyTerrainAcceleration;
				}

				botDriver->botData.aiPhysics.speedLinear = botVelocity;
				if (botVelocity < velocityAccountingForTerrain)
				{
					botDriver->botData.aiPhysics.speedLinear = velocityAccountingForTerrain;
					goto CheckAccelerationTowardTerrainTarget;
				}
			}

			levelID = gGT->levelID; // iVar3

			if ((levelID == HOT_AIR_SKYWAY || levelID == PAPU_PYRAMID) || levelID == POLAR_PASS)
			{
				int navFrameIndexOnPath;
				int botPathIndex = botDriver->botData.botPath; // iVar3

				if (levelID == POLAR_PASS)
				{
					botPathIndex = CTR_MipsAddLo(botPathIndex, 3);
				}
				else
				{
					if (levelID == PAPU_PYRAMID)
					{
						botPathIndex = CTR_MipsAddLo(botPathIndex, 6);
					}
					else if (levelID == SLIDE_COLISEUM)
					{
						botPathIndex = CTR_MipsAddLo(botPathIndex, 9);
					}
				}

				navFrameIndexOnPath = (int)(navFrameCurr - sdata->NavPath_ptrNavFrameArray[botDriver->botData.botPath]);

				if ((data.botsThrottle[botPathIndex] <= navFrameIndexOnPath) && (navFrameIndexOnPath < CTR_MipsAddLo(data.botsThrottle[botPathIndex], 0xb)) &&
				    (9000 < botDriver->botData.aiPhysics.speedLinear))
				{
					botDriver->botData.aiPhysics.turboMeter = 0;

					botDriver->botData.aiPhysics.speedLinear =
					    CTR_MipsSubLo(botDriver->botData.aiPhysics.speedLinear, CTR_MipsAddLo(100, botDriver->const_Accel_ClassStat));
				}
			}

			if ((u8)0x80u < navFrameCurr->rot[3])
			{
				velocityAccountingForTerrain = CTR_MipsAddLo(velocityAccountingForTerrain, botDriver->const_SlopeForwardSpeedBonus);

				if (botDriver->botData.aiPhysics.speedLinear < velocityAccountingForTerrain)
				{
					u32 var = (u32)navFrameCurr->rot[3];
					int sinOfAngle = MATH_Sin(CTR_MipsSll(var, 4));

					botDriver->botData.aiPhysics.speedLinear =
					    CTR_MipsSubLo(botDriver->botData.aiPhysics.speedLinear,
					                  CTR_MipsSra(CTR_MipsMulLo(botDriver->const_Gravity, sinOfAngle), 0xc)); // force on a slope due to gravity
				}

				botDriver->fireSpeed = velocityAccountingForTerrain;
			}
		}

		if (0x6400 < botDriver->botData.aiPhysics.speedLinear)
		{
			botDriver->botData.aiPhysics.speedLinear = 0x6400;
		}

		deltaPosThisFrame = CTR_MipsSra(CTR_MipsMulLo(botDriver->botData.aiPhysics.speedLinear, elapsedMilliseconds), 5); // iVar4
		if (deltaPosThisFrame < 0)
		{
			deltaPosThisFrame = 0;
		}
		deltaPosThisFrame = CTR_MipsAddLo(deltaPosThisFrame, botDriver->botData.navProgressRemainder);
	}
	else
	{
	HoldBotBeforeRaceOrCooldown:
		botInstance->scale.x = 0xccc;
		botInstance->scale.y = 0xccc;
		botInstance->scale.z = 0xccc;

		botDriver->botData.aiPhysics.speedLinear = 0;

		if (botDriver->botData.ai_progress_cooldown != 0)
		{
			botDriver->botData.ai_progress_cooldown--;
		}

		deltaPosThisFrame = botDriver->botData.navProgressRemainder;
	}

	navFrameFlags = navFrameCurr->flags;
	navFrameSpecialBits = navFrameCurr->specialBits;
	// local_3c == 0
	if ((navFrameSpecialBits & BOTS_NAV_SPECIAL_MOON_GRAVITY) != 0)
	{
		botDriver->botData.botFlags |= BOT_FLAG_MOON_GRAVITY;
	}


	if ((botDriver->botData.botFlags & BOT_FLAG_MOON_GRAVITY) == 0)
	{
		gravity = botDriver->const_Gravity;
	}
	else
	{
		gravity = CTR_MipsDiv(CTR_MipsMulLo(botDriver->const_Gravity, 41), 100);
	}

	botDriver->botData.aiPhysics.speedY =
	    CTR_MipsSubLo(botDriver->botData.aiPhysics.speedY, CTR_MipsSra(CTR_MipsMulLo(gravity, elapsedMilliseconds), 5)); // iVar3

	if (botDriver->botData.aiPhysics.speedY < -0x5000)
	{
		botDriver->botData.aiPhysics.speedY = -0x5000;
	}

	botDriver->botData.positionBackup.y =
	    CTR_MipsAddLo(botDriver->botData.positionBackup.y, CTR_MipsSra(CTR_MipsMulLo(botDriver->botData.aiPhysics.speedY, elapsedMilliseconds), 5));


	if ((botDriver->actionsFlagSet & ACTION_TOUCH_GROUND) == 0)
	{
		navDist = navFrameCurr->distToNextNavXZ;
	}
	else
	{
		navDist = navFrameCurr->distToNextNavXYZ;
	}

	local_3c = 0;
	useGroundedNavDistance = false;
	if ((navFrameCurr->specialBits & BOTS_NAV_SPECIAL_RAMP_PHYS) != 0)
	{
		local_3c = navFrameCurr->specialBits;
	}
	iVar15 = CTR_MipsSra(deltaPosThisFrame, 8);


#if defined(CTR_NATIVE)
	// MultiTrack: custom tracks' nav paths can have frames 0 apart all the way
	// round: the loop below would never use up the distance. One lap of the
	// path in one tick at most
	int navSteps = 0;
#endif
	for (iVar3 = navDist; iVar3 <= iVar15; /* decrement inside loop */)
	{
		int index;
		int uVar8;
		s16 nextNavDist; // sVar7
		u8 compare;
#if defined(CTR_NATIVE)
		if (++navSteps > sdata->NavPath_ptrHeader[botDriver->botData.botPath]->numPoints + 1)
		{
			break;
		}
#endif
		navFrameCurr = navFrameNext;

		deltaPosThisFrame = CTR_MipsSubLo(deltaPosThisFrame, CTR_MipsSll(iVar3, 8));

		index = botDriver->botData.botPath; // index = iVar13

		navFrameNext = NAVFRAME_GETNEXTFRAME(navFrameCurr);

		iVar15 = CTR_MipsSubLo(iVar15, iVar3);

		if (navFrameNext >= sdata->NavPath_ptrHeader[index]->last)
		{
			navFrameNext = sdata->NavPath_ptrNavFrameArray[index];
		}

		if ((CTR_MipsSra(botDriver->botData.positionBackup.y, 8) < navFrameNext->pos.y) && ((navFrameCurr->flags & BOTS_NAV_FLAG_KILLPLANE) != 0))
		{
			BOTS_Killplane(botThread);
		}

		navFrameFlags |= navFrameCurr->flags;
		navFrameSpecialBits |= navFrameCurr->specialBits;


		if ((botDriver->botData.botFlags & BOT_FLAG_BOSS_PATH_ACTIVE) == 0)
		{
			uVar8 = ~BOT_FLAG_ESTIMATE_NAV;
		}
		else
		{
			uVar8 = ~(BOT_FLAG_ESTIMATE_NAV | BOT_FLAG_BOSS_PATH_REQUESTED | BOT_FLAG_BOSS_PATH_ACTIVE);
		}

		botDriver->botData.botFlags &= uVar8;


		// MUST be unsigned,
		// this makes a range-value check, and makes negatives positive,
		// so its really checking value between 0x31 and 0x31+0x9e
		compare = navFrameCurr->rot[3] - 0x31;
		if (!useGroundedNavDistance && (0x9e < compare))
		{
			if ((botDriver->actionsFlagSet & ACTION_TOUCH_GROUND) == 0)
			{
				nextNavDist = navFrameCurr->distToNextNavXZ;
			}
			else
			{
				nextNavDist = navFrameCurr->distToNextNavXYZ;
			}
		}
		else
		{
			useGroundedNavDistance = true;

			nextNavDist = navFrameCurr->distToNextNavXZ;
		}

		iVar3 = nextNavDist;

		if ((navFrameCurr->specialBits & BOTS_NAV_SPECIAL_RAMP_PHYS) != 0)
		{
			local_3c = navFrameCurr->specialBits;
		}
	}

	botDriver->botData.navProgressRemainder = deltaPosThisFrame;

	navActionFlags = ((navFrameFlags & BOTS_NAV_FLAG_BACK_SKID) != 0) ? ACTION_BACK_SKID : 0;

	if ((navFrameFlags & BOTS_NAV_FLAG_ENGINE_ECHO) != 0)
	{
		navActionFlags |= ACTION_ENGINE_ECHO;
	}

	if ((navFrameFlags & BOTS_NAV_FLAG_FRONT_SKID) != 0)
	{
		navActionFlags |= ACTION_FRONT_SKID;
	}

	botDriver->actionsFlagSet &= ~(ACTION_ENGINE_ECHO | ACTION_BACK_SKID | ACTION_FRONT_SKID);
	botDriver->actionsFlagSet |= navActionFlags;

	terrain = VehAfterColl_GetTerrain(((u8)navFrameCurr->flags) >> 3);

	botDriver->terrainMeta1 = terrain;

	if ((navFrameCurr->specialBits & BOTS_NAV_SPECIAL_REFLECTIVE) != 0)
	{
		s16 vertSplit;
		if ((navFrameCurr->specialBits & BOTS_NAV_SPECIAL_INDEX_MASK) == 0)
		{
			vertSplit = gGT->level1->splitLines[0];
		}
		else
		{
			vertSplit = gGT->level1->splitLines[1];
		}

		botInstance->vertSplit = vertSplit;

		botInstance->flags |= REFLECTIVE;
	}

	if (((navFrameCurr->specialBits & (BOTS_NAV_SPECIAL_RAMP_PHYS | BOTS_NAV_SPECIAL_REFLECTIVE)) == 0) && (botThread->modelIndex != DYNAMIC_GHOST))
	{
		int transparency = CTR_MipsMulLo(navFrameCurr->specialBits & BOTS_NAV_SPECIAL_INDEX_MASK, 0x9c00);

		botDriver->alphaScaleBackup = (s16)CTR_MipsSra(CTR_MipsAddLo(CTR_MipsMulLo((u16)botDriver->alphaScaleBackup, 100), transparency), 8);

		botInstance->alphaScale = (s16)CTR_MipsSra(CTR_MipsAddLo(CTR_MipsMulLo((u16)botInstance->alphaScale, 100), transparency), 8);
	}

	if (((botDriver->actionsFlagSet & ACTION_FRONT_SKID) == 0) || ((navFrameFlags & BOTS_NAV_FLAG_DRIFT_MASK) == 0))
	{
		botDriver->turbo_MeterRoomLeft = 0;
		botDriver->botData.aiPhysics.turboMeter = 0;
		botDriver->botData.aiPhysics.fireLevel = 0;
	}
	else
	{
		int uVar6;
		botDriver->actionsFlagSet |= ACTION_TURBO_INPUT_LATCH;
		botDriver->botData.aiPhysics.turboMeter = (s16)CTR_MipsAddLo((u16)botDriver->botData.aiPhysics.turboMeter, elapsedMilliseconds);

		// this ugly tree of ifs may have been a switch perhaps?
		if ((botDriver->botData.aiPhysics.turboMeter < 0xb41) || (5 < botDriver->botData.aiPhysics.fireLevel))
		{
			if ((botDriver->botData.aiPhysics.turboMeter < 0x961) || (uVar6 = 5, 4 < botDriver->botData.aiPhysics.fireLevel))
			{
				if ((botDriver->botData.aiPhysics.turboMeter < 0x781) || (3 < botDriver->botData.aiPhysics.fireLevel))
				{
					if ((0x5a0 < botDriver->botData.aiPhysics.turboMeter) && (uVar6 = 3, botDriver->botData.aiPhysics.fireLevel < 3))
					{
						goto SetTurboChargeThreshold;
					}
					if ((botDriver->botData.aiPhysics.turboMeter < 0x3c1) || (1 < botDriver->botData.aiPhysics.fireLevel))
					{
						if ((0x1e0 < botDriver->botData.aiPhysics.turboMeter) && (uVar6 = 1, botDriver->botData.aiPhysics.fireLevel < 1))
						{
							goto SetTurboChargeThreshold;
						}
					}
					else
					{
						// trigger a turbo boost?
						botDriver->botData.aiPhysics.fireLevel = 2;
						botDriver->turbo_MeterRoomLeft = 0;
						VehFire_Increment(botDriver, 0xf0, 2, CTR_MipsSll(local_38, 7));
					}
				}
				else
				{
					// trigger a turbo boost?
					botDriver->botData.aiPhysics.fireLevel = 4;
					botDriver->turbo_MeterRoomLeft = 0;
					VehFire_Increment(botDriver, 0x1e0, 2, CTR_MipsSll(local_38, 8));
				}
			}
			else
			{
			SetTurboChargeThreshold:
				// trigger a turbo boost?
				botDriver->botData.aiPhysics.fireLevel = uVar6;
				botDriver->turbo_MeterRoomLeft = 0xa0;
			}
		}
		else
		{
			// trigger a turbo boost?
			botDriver->botData.aiPhysics.fireLevel = 6;
			botDriver->turbo_MeterRoomLeft = 0;
			VehFire_Increment(botDriver, 0x2d0, 2, CTR_MipsMulLo(local_38, 0x180));
		}
	}

	if ((navFrameFlags & BOTS_NAV_FLAG_SUPER_TURBO) != 0)
	{
		VehFire_Increment(botDriver, 0x78, 1, 0x900);

		botDriver->botData.botFlags |= BOT_FLAG_NAV_BOOST_ACTIVE;
	}

	if ((navFrameFlags & BOTS_NAV_FLAG_TURBO) != 0)
	{
		VehFire_Increment(botDriver, 0x2d0, 1, 0x180);

		botDriver->botData.botFlags |= BOT_FLAG_NAV_BOOST_ACTIVE;
	}

	if ((botDriver->botData.botFlags & BOT_FLAG_ESTIMATE_NAV) == 0)
	{
		s16 botPath;
		botDriver->botData.botNavFrame = navFrameCurr;

		botPath = botDriver->botData.botPath;

		navFrameNext = NAVFRAME_GETNEXTFRAME(navFrameCurr);

		if (navFrameNext >= sdata->NavPath_ptrHeader[botPath]->last)
		{
			navFrameNext = sdata->NavPath_ptrNavFrameArray[botPath];
		}
	}


	if (iVar3 == 0)
	{
		percentage = 0;
	}
	else
	{
		percentage = CTR_MipsDiv(CTR_MipsSll(iVar15, 0xc), iVar3);
	}

	botDriver->posPrev.x = botDriver->posCurr.x;
	botDriver->posPrev.y = botDriver->posCurr.y;
	botDriver->posPrev.z = botDriver->posCurr.z;

	botDriver->rotPrev.x = botDriver->rotCurr.x;
	botDriver->rotPrev.y = botDriver->rotCurr.y;
	botDriver->rotPrev.z = botDriver->rotCurr.z;

	botDriver->botData.positionBackup.x = CTR_MipsSll(
	    CTR_MipsAddLo(navFrameCurr->pos.x, CTR_MipsSra(CTR_MipsMulLo(CTR_MipsSubLo(navFrameNext->pos.x, navFrameCurr->pos.x), percentage), 0xc)), 8);
	botDriver->quadBlockHeight = CTR_MipsSll(
	    CTR_MipsAddLo(navFrameCurr->pos.y, CTR_MipsSra(CTR_MipsMulLo(CTR_MipsSubLo(navFrameNext->pos.y, navFrameCurr->pos.y), percentage), 0xc)), 8);
	botDriver->botData.positionBackup.z = CTR_MipsSll(
	    CTR_MipsAddLo(navFrameCurr->pos.z, CTR_MipsSra(CTR_MipsMulLo(CTR_MipsSubLo(navFrameNext->pos.z, navFrameCurr->pos.z), percentage), 0xc)), 8);

	if ((botDriver->botData.botFlags & BOT_FLAG_FREE_PHYSICS) != 0)
	{
		int preAccelX;
		int preAccelZ;
		int preX;
		int preZ;
		botDriver->botData.aiPhysics.accel.y = 0;
		botDriver->botData.aiPhysics.velocity.x = CTR_MipsAddLo(botDriver->botData.aiPhysics.velocity.x, botDriver->botData.aiPhysics.accel.x);
		botDriver->botData.aiPhysics.velocity.y = CTR_MipsAddLo(botDriver->botData.aiPhysics.velocity.y, botDriver->botData.aiPhysics.accel.y);
		botDriver->botData.aiPhysics.velocity.z = CTR_MipsAddLo(botDriver->botData.aiPhysics.velocity.z, botDriver->botData.aiPhysics.accel.z);
		preAccelX = botDriver->botData.aiPhysics.accel.x; // iVar3
		preAccelZ = botDriver->botData.aiPhysics.accel.z; // iVar15
		botDriver->botData.aiPhysics.accel.x = CTR_MipsSra(botDriver->botData.aiPhysics.accel.x, 1);
		botDriver->botData.aiPhysics.accel.z = CTR_MipsSra(botDriver->botData.aiPhysics.accel.z, 1);
		botDriver->botData.aiPhysics.velocity.y = CTR_MipsAddLo(botDriver->botData.aiPhysics.velocity.y, botDriver->botData.aiPhysics.accel.y);

		botDriver->botData.aiPhysics.velocity.x = CTR_MipsAddLo(botDriver->botData.aiPhysics.velocity.x, preAccelX);
		botDriver->botData.aiPhysics.velocity.z = CTR_MipsAddLo(botDriver->botData.aiPhysics.velocity.z, preAccelZ);

		preX = botDriver->botData.aiPhysics.velocity.x; // iVar3
		if (preX != 0)
		{
			if (preX < 1)
			{
				botDriver->botData.aiPhysics.velocity.x = CTR_MipsAddLo(preX, 0x444);
				if (0 < CTR_MipsAddLo(preX, 0x444))
				{
					botDriver->botData.aiPhysics.velocity.x = 0;
				}
				else
				{
					if (botDriver->botData.aiPhysics.accel.x == 0)
					{
						botDriver->botData.aiPhysics.accel.x = 0x444;
					}
				}
			}
			else
			{
				botDriver->botData.aiPhysics.velocity.x = CTR_MipsSubLo(preX, 0x444);
				if (CTR_MipsSubLo(preX, 0x444) < 0)
				{
					botDriver->botData.aiPhysics.velocity.x = 0;
				}
				else
				{
					if (botDriver->botData.aiPhysics.accel.x == 0)
					{
						botDriver->botData.aiPhysics.accel.x = -0x444;
					}
				}
			}
		}
		preZ = botDriver->botData.aiPhysics.velocity.z; // iVar3
		if (preZ != 0)
		{
			if (preZ < 1)
			{
				botDriver->botData.aiPhysics.velocity.z = CTR_MipsAddLo(preZ, 0x444);
				if (0 < CTR_MipsAddLo(preZ, 0x444))
				{
					botDriver->botData.aiPhysics.velocity.z = 0;
				}
				else
				{
					if (botDriver->botData.aiPhysics.accel.z == 0)
					{
						botDriver->botData.aiPhysics.accel.z = 0x444;
					}
				}
			}
			else
			{
				botDriver->botData.aiPhysics.velocity.z = CTR_MipsSubLo(preZ, 0x444);
				if (CTR_MipsSubLo(preZ, 0x444) < 0)
				{
					botDriver->botData.aiPhysics.velocity.z = 0;
				}
				else
				{
					if (botDriver->botData.aiPhysics.accel.z == 0)
					{
						botDriver->botData.aiPhysics.accel.z = -0x444;
					}
				}
			}
		}
		if ((botDriver->botData.aiPhysics.velocity.x == 0) && (botDriver->botData.aiPhysics.velocity.z == 0))
		{
			botDriver->botData.botFlags &= ~BOT_FLAG_FREE_PHYSICS;
		}
	}

	if ((botDriver->botData.botFlags & (BOT_FLAG_ESTIMATE_NAV | BOT_FLAG_FREE_PHYSICS)) == 0)
	{
		botDriver->botData.ai_quadblock_checkpointIndex = navFrameCurr->goBackCount;
	}
	else
	{
		s16 probeY;
		s16 probeX;
		s16 probeZ;
		SVec3 probeTop;
		SVec3 probeBottom;
		int deltaRotY = CTR_MipsSubLo(CTR_MipsSll(navFrameNext->rot[1], 4), CTR_MipsSll(navFrameCurr->rot[1], 4)) & 0xfff;
		if (0x7ff < deltaRotY)
		{
			deltaRotY = CTR_MipsSubLo(deltaRotY, 0x1000);
		}

		botDriver->botData.aiRot.y = CTR_MipsAddLo(CTR_MipsSll(navFrameCurr->rot[1], 4), CTR_MipsSra(CTR_MipsMulLo(deltaRotY, percentage), 0xc)) & 0xfff;

		probeY = (s16)CTR_MipsSra(CTR_MipsAddLo(botDriver->botData.positionBackup.y, botDriver->botData.aiPhysics.velocity.y), 8);
		probeX = (s16)CTR_MipsSra(CTR_MipsAddLo(botDriver->botData.positionBackup.x, botDriver->botData.aiPhysics.velocity.x), 8);
		probeZ = (s16)CTR_MipsSra(CTR_MipsAddLo(botDriver->botData.positionBackup.z, botDriver->botData.aiPhysics.velocity.z), 8);
		probeTop.x = probeX;
		probeTop.y = (s16)CTR_MipsSubLo(probeY, 0x100);
		probeTop.z = probeZ;
		probeBottom.x = probeX;
		probeBottom.y = (s16)CTR_MipsAddLo(probeY, 0x80);
		probeBottom.z = probeZ;

		sps->ptr_mesh_info = gGT->level1->ptr_mesh_info;
		sps->Union.QuadBlockColl.quadFlagsWanted = QUADBLOCK_FLAG_GROUND;
		sps->Union.QuadBlockColl.quadFlagsIgnored = QUADBLOCK_FLAG_NO_COLLISION_RESPONSE;
		sps->Union.QuadBlockColl.searchFlags = COLL_SEARCH_HIGH_LOD;

		COLL_SearchBSP_CallbackQUADBLK(&probeTop, &probeBottom, sps, 0);

		if (sps->boolDidTouchQuadblock != 0)
		{
			botDriver->quadBlockHeight = CTR_MipsSll(sps->Union.QuadBlockColl.hitPos.y, 8);

			botDriver->botData.ai_quadblock_checkpointIndex = sps->hit.ptrQuadblock->checkpointIndex;

			VehPhysForce_RotAxisAngle(&botInstance->matrix, CTR_VECTOR_DATA(&(sps->hit.plane.normal)), botDriver->botData.aiRot.y);

			botDriver->AxisAngle3_normalVec = sps->hit.plane.normal;

			botInstance->compressedNormalAndDriverIndex =
			    INST_CompressNormalVectorAndDriverIndex(sps->hit.plane.normal.x, sps->hit.plane.normal.y, sps->hit.plane.normal.z, botDriver->driverID);

			if ((sps->hit.ptrQuadblock->quadFlags & QUADBLOCK_FLAG_KILL_PLANE) != 0)
			{
				BOTS_Killplane(botThread);
			}
		}
	}
	deltaPosThisFrame = CTR_MipsSra(deltaPosThisFrame, 8);
	if (botDriver->botData.positionBackup.y < botDriver->quadBlockHeight)
	{
		u32 oldBotFlags = botDriver->botData.botFlags; // uVar8
		botDriver->botData.botFlags &= ~BOT_FLAG_MOON_GRAVITY;

		if ((navFrameCurr->flags & BOTS_NAV_FLAG_KILLPLANE) == 0)
		{
			if ((oldBotFlags & BOT_FLAG_DAMAGE_ACTIVE) == 0)
			{
				if ((botDriver->actionsFlagSet & ACTION_TOUCH_GROUND) == 0)
				{
					int iVar3;
					s16 sVar7;
					if (botDriver->instSelf->thread->modelIndex == DYNAMIC_PLAYER)
					{
						u32 echo;
						int flags;
						int mapped = VehCalc_MapToRange(CTR_MipsSubLo(botDriver->jumpHeightCurr, botDriver->jumpHeightPrev), 0x300, 0x1400, 0x4b, 200); // uVar8

						int volume; // uVar20
						if (mapped < 0)
						{
							volume = 0;
						}
						else
						{
							volume = mapped & 0xff;

							if (0xff < mapped)
							{
								volume = 0xff;
							}
						}
						echo = ((botDriver->actionsFlagSet & ACTION_ENGINE_ECHO) != 0);
						flags = (int)HowlSfx_Pack(HOWL_SFX_LR_CENTER, HOWL_SFX_DISTORTION_NONE, volume, echo);

						OtherFX_Play_LowLevel(7, 1, flags);
					}
					iVar3 = navFrameCurr->distToNextNavXZ;
					if (iVar3 != 0)
					{
#if 0
						if (iVar3 == 0)
						{
							trap(0x1c00);
						}
						if ((iVar3 == -1) && (deltaPosThisFrame * navFrameCurr->distToNextNavXYZ == -0x80000000))
						{
							trap(0x1800);
						}
#endif
						botDriver->botData.navProgressRemainder =
						    CTR_MipsSll(CTR_MipsDiv(CTR_MipsMulLo(deltaPosThisFrame, navFrameCurr->distToNextNavXYZ), iVar3), 8);
					}
					sVar7 = botDriver->jump_LandingBoost;

					if (sVar7 < 0x5a1)
					{
						if (0x3c0 < sVar7)
						{
							deltaPosThisFrame = CTR_MipsMulLo(local_38, 0x60);
							goto doFireIncrement;
						}
						else if (0x280 < sVar7)
						{
							deltaPosThisFrame = 0;
							goto doFireIncrement;
						}
					}
					else
					{
						deltaPosThisFrame = CTR_MipsMulLo(local_38, 0xc0);
					doFireIncrement:
						VehFire_Increment(botDriver, 0x2d0, 2, deltaPosThisFrame);
					}

					botDriver->actionsFlagSet |= ACTION_STARTED_TOUCH_GROUND;
				}
				deltaPosThisFrame = CTR_MipsSubLo(botDriver->quadBlockHeight, botDriver->posPrev.y);

				botDriver->botData.positionBackup.y = botDriver->quadBlockHeight;

				botDriver->botData.aiPhysics.speedY = deltaPosThisFrame;

				if ((navFrameFlags & BOTS_NAV_FLAG_JUMP) != 0 || (botDriver->instTntRecv != NULL))
				{
					int oldActionsFlags = botDriver->actionsFlagSet;

					botDriver->actionsFlagSet |= ACTION_JUMP_STARTED;

					botDriver->botData.aiPhysics.speedY = CTR_MipsAddLo(deltaPosThisFrame, 0x1400);

					if (botThread->modelIndex == DYNAMIC_PLAYER)
					{
						OtherFX_Play_Echo(8, 1, (oldActionsFlags & ACTION_ENGINE_ECHO) != 0);
					}
				}
				if (16000 < botDriver->botData.aiPhysics.speedY)
				{
					botDriver->botData.aiPhysics.speedY = 16000;
				}

				if ((navFrameFlags & BOTS_NAV_FLAG_DRIFT_MASK) == 0)
				{
					botDriver->botData.aiPhysics.driftTarget = 0;

					botDriver->kartState = KS_NORMAL;
				}
				else
				{
					botDriver->kartState = KS_DRIFTING;

					if ((navFrameFlags & BOTS_NAV_FLAG_DRIFT_LEFT) == 0)
					{
						botDriver->botData.aiPhysics.driftTarget = 0x2aa;
					}
					else
					{
						botDriver->botData.aiPhysics.driftTarget = -0x2aa;
					}
				}
			}
			else
			{
				u8 bVar10;
				if (((botDriver->actionsFlagSet & ACTION_TOUCH_GROUND) == 0))
				{
					int iVar3 = navFrameCurr->distToNextNavXZ;
					if (iVar3 != 0)
					{
#if 0
						if (iVar3 == 0)
						{
							trap(0x1c00);
						}
						if ((iVar3 == -1) && (deltaPosThisFrame * navFrameCurr->distToNextNavXYZ == -0x80000000))
						{
							trap(0x1800);
						}
#endif
						botDriver->botData.navProgressRemainder =
						    CTR_MipsSll(CTR_MipsDiv(CTR_MipsMulLo(deltaPosThisFrame, navFrameCurr->distToNextNavXYZ), iVar3), 8);
					}
				}
				deltaPosThisFrame = CTR_MipsSra(CTR_MipsNegLo(botDriver->botData.aiPhysics.speedY), 1);

				botDriver->botData.aiPhysics.speedY = deltaPosThisFrame;

				if (deltaPosThisFrame < 0)
				{
					botDriver->botData.aiPhysics.speedY = CTR_MipsNegLo(deltaPosThisFrame);
				}

				bVar10 = (u8)CTR_MipsAddLo(botDriver->botData.blastBounceCount, 1);
				botDriver->botData.blastBounceCount = bVar10;

				if (botDriver->botData.aiDamageState == BOTS_DAMAGE_STATE_SPIN)
				{
					botDriver->actionsFlagSet |= ACTION_BACK_SKID | ACTION_FRONT_SKID;
				}
				else
				{
					if (botDriver->botData.aiDamageState == BOTS_DAMAGE_STATE_BLAST && (2 < bVar10))
					{
						botDriver->botData.ai_progress_cooldown = 10;
						botDriver->botData.aiPhysics.speedLinear = 0;
						botDriver->botData.botFlags &= ~(BOT_FLAG_DAMAGE_ACTIVE | BOT_FLAG_DAMAGE_SUPPRESS_EMITTER);
					}
				}

				botDriver->botData.positionBackup.y = botDriver->quadBlockHeight;
			}
		}
		else
		{
			BOTS_Killplane(botThread);
		}

		botDriver->jump_LandingBoost = 0;
		botDriver->actionsFlagSet |= ACTION_TOUCH_GROUND;
	}
	else
	{
		if (!useGroundedNavDistance && ((botDriver->actionsFlagSet & ACTION_TOUCH_GROUND) != 0))
		{
			int iVar3 = navFrameCurr->distToNextNavXYZ;
			if (iVar3 != 0)
			{
#if 0
				if (iVar3 == 0)
				{
					trap(0x1c00);
				}
				if ((iVar3 == -1) && (deltaPosThisFrame * navFrameCurr->distToNextNavXZ == -0x80000000))
				{
					trap(0x1800);
				}
#endif
				botDriver->botData.navProgressRemainder = CTR_MipsSll(CTR_MipsDiv(CTR_MipsMulLo(deltaPosThisFrame, navFrameCurr->distToNextNavXZ), iVar3), 8);
			}
		}

		botDriver->actionsFlagSet &= ~ACTION_TOUCH_GROUND;
		botDriver->actionsFlagSet |= ACTION_AIRBORNE;

		botDriver->jump_LandingBoost = (s16)CTR_MipsAddLo((u16)botDriver->jump_LandingBoost, elapsedMilliseconds);
	}

	iVar4_lifetime_3 = 0x18;

	if (botDriver->botData.aiPhysics.driftTarget != 0)
	{
		iVar4_lifetime_3 = 0x60;
	}

	iVar3_lifetime_2 = (s16)CTR_MipsSubLo((u16)botDriver->botData.aiPhysics.mulDrift, iVar4_lifetime_3);
	if (botDriver->botData.aiPhysics.driftTarget < botDriver->botData.aiPhysics.mulDrift)
	{
		botDriver->botData.aiPhysics.mulDrift = iVar3_lifetime_2;

		if (iVar3_lifetime_2 < botDriver->botData.aiPhysics.driftTarget)
		{
			botDriver->botData.aiPhysics.mulDrift = botDriver->botData.aiPhysics.driftTarget;
		}
	}
	else
	{
		iVar4_lifetime_3 = (s16)CTR_MipsAddLo((u16)botDriver->botData.aiPhysics.mulDrift, iVar4_lifetime_3);

		botDriver->botData.aiPhysics.mulDrift = iVar4_lifetime_3;

		if (botDriver->botData.aiPhysics.driftTarget < iVar4_lifetime_3)
		{
			botDriver->botData.aiPhysics.mulDrift = botDriver->botData.aiPhysics.driftTarget;
		}
	}
	botDriver->multDrift = botDriver->botData.aiPhysics.mulDrift;

	if ((botDriver->botData.botFlags & BOT_FLAG_DAMAGE_ACTIVE) != 0)
	{
		u8 newKartState; // uVar2
		s16 sVar7 = botDriver->botData.aiDamageState;
		if (sVar7 == BOTS_DAMAGE_STATE_BLAST)
		{
			botDriver->botData.aiPhysics.rotXZ = (s16)CTR_MipsSubLo((u16)botDriver->botData.aiPhysics.rotXZ, 1);
		}
		else
		{
			if (sVar7 < BOTS_DAMAGE_STATE_SQUISH)
			{
				if (sVar7 == BOTS_DAMAGE_STATE_SPIN)
				{
					u16 squishCooldownMaybe = (u16)botDriver->botData.aiPhysics.squishCooldown;
					int alsoSquishCooldownMaybe = CTR_MipsSubLo(botDriver->botData.aiPhysics.squishCooldown, 0xc);
					botDriver->botData.aiPhysics.squishCooldown = alsoSquishCooldownMaybe;
					botDriver->botData.aiPhysics.mulDrift = (s16)CTR_MipsAddLo((u16)botDriver->botData.aiPhysics.mulDrift, squishCooldownMaybe);
					if (alsoSquishCooldownMaybe < 0x100)
					{
						botDriver->botData.aiPhysics.squishCooldown = 0;
						botDriver->botData.aiPhysics.mulDrift = 0;
						botDriver->botData.aiPhysics.speedLinear = 0;
						botDriver->botData.botFlags &= ~(BOT_FLAG_DAMAGE_ACTIVE | BOT_FLAG_DAMAGE_SUPPRESS_EMITTER);
					}
					newKartState = 3;
					botDriver->kartState = newKartState;
				}
			}
			else
			{
				if (sVar7 == BOTS_DAMAGE_STATE_SQUISH)
				{
					u16 sVar7 = (u16)botDriver->botData.aiPhysics.squishCooldown;
					int iVar4 = CTR_MipsSubLo(botDriver->botData.aiPhysics.squishCooldown, 0xc);
					botDriver->botData.aiPhysics.squishCooldown = iVar4;
					botDriver->botData.aiPhysics.mulDrift = (s16)CTR_MipsAddLo((u16)botDriver->botData.aiPhysics.mulDrift, sVar7);
					if (iVar4 < 0x200)
					{
						botDriver->botData.aiPhysics.squishCooldown = 0;
						botDriver->botData.aiPhysics.mulDrift = 0;
					}
					iVar4 = (s16)CTR_MipsSubLo((u16)botDriver->botData.aiPhysics.rotXZ, elapsedMilliseconds);
					botDriver->botData.aiPhysics.rotXZ = iVar4;
					if (iVar4 <= 0)
					{
						botDriver->botData.aiPhysics.speedY = 0x1400;
						botDriver->botData.botFlags &= ~(BOT_FLAG_DAMAGE_ACTIVE | BOT_FLAG_DAMAGE_SUPPRESS_EMITTER);
						botDriver->actionsFlagSet |= ACTION_JUMP_STARTED;
					}
				}
				else
				{
					if (sVar7 == BOTS_DAMAGE_STATE_MASK_GRAB)
					{
						int iVar4;
						struct Thread *plant = botDriver->plantEatingMe;

						struct Instance *plantInst = plant->inst;

						if (botDriver->botData.aiPhysics.rotXZ < 0xb40)
						{
							VECTOR v2;
							s32 l3;
							int camDriverXDelta;
							int camY;
							int driverY;
							int camDriverZDelta;
							int rotY;
							int rotX;
							SVECTOR v;
							v.vx = 0xfa;
							if ((plant->object != NULL) && (((struct Plant *)plant->object)->side != 0))
							{
								v.vx = -0xfa;
							}
							v.vy = 0;
							v.vz = 0x2ee;

							SetRotMatrix(&plantInst->matrix);
							SetTransMatrix(&plantInst->matrix);


							RotTrans(&v, &v2, &l3);

							gGT->pushBuffer[botDriver->driverID].pos.x = v2.vx;
							gGT->pushBuffer[botDriver->driverID].pos.y = CTR_MipsAddLo(plantInst->matrix.t[1], 0xc0);
							gGT->pushBuffer[botDriver->driverID].pos.z = v2.vz;

							camDriverXDelta = CTR_MipsSubLo(v2.vx, plantInst->matrix.t[0]);
							camY = gGT->pushBuffer[botDriver->driverID].pos.y;
							driverY = plantInst->matrix.t[1];
							camDriverZDelta = CTR_MipsSubLo(v2.vz, plantInst->matrix.t[2]);

							rotY = ratan2(camDriverXDelta, camDriverZDelta);
							gGT->pushBuffer[botDriver->driverID].rot.y = rotY;

							rotX = SquareRoot0_stub(
							    CTR_MipsAddLo(CTR_MipsMulLo(camDriverXDelta, camDriverXDelta), CTR_MipsMulLo(camDriverZDelta, camDriverZDelta)));
							rotX = ratan2(CTR_MipsSubLo(camY, driverY), rotX);

							gGT->pushBuffer[botDriver->driverID].rot.x = CTR_MipsSubLo(0x800, rotX);
							gGT->pushBuffer[botDriver->driverID].rot.z = 0;
						}

						botDriver->botData.aiPhysics.speedLinear = 0;
						iVar4 = (s16)CTR_MipsSubLo((u16)botDriver->botData.aiPhysics.rotXZ, elapsedMilliseconds);
						botDriver->botData.aiPhysics.rotXZ = iVar4;
						newKartState = 5;
						if (iVar4 < 1)
						{
							botDriver->botData.botFlags &= ~(BOT_FLAG_DAMAGE_ACTIVE | BOT_FLAG_DAMAGE_SUPPRESS_EMITTER);
							botInstance->flags &= ~HIDE_MODEL;
							botDriver->botData.ai_progress_cooldown = 1;
							BOTS_MaskGrab(botThread);
							newKartState = 5;
						}
						botDriver->kartState = newKartState;
					}
				}
			}
		}
	}

	if (botDriver->forcedJumpType != FORCED_JUMP_NONE)
	{
		int turtleJumpForce = CTR_MipsAddLo(CTR_MipsSll(botDriver->const_JumpForce, 1), botDriver->const_JumpForce); // iVar4

		if (botDriver->forcedJumpType != FORCED_JUMP_HIGH)
		{
			turtleJumpForce = CTR_MipsDiv(turtleJumpForce, 2);
		}

		if (botDriver->botData.aiPhysics.speedY < turtleJumpForce)
		{
			botDriver->botData.aiPhysics.speedY = turtleJumpForce;
		}

		botDriver->forcedJumpType = FORCED_JUMP_NONE;
	}

	if (((navFrameSpecialBits & BOTS_NAV_SPECIAL_RAMP_PHYS) != 0) &&
	    ((0x1c1f < botDriver->botData.aiPhysics.speedLinear) || (data.characterIDs[botDriver->driverID] == NITROS_OXIDE)))
	{
		int iVar4 = (local_3c & BOTS_NAV_SPECIAL_INDEX_MASK);
		botDriver->botData.aiPhysics.speedY = sdata->NavPath_ptrHeader[botDriver->botData.botPath]->rampPhys2[iVar4];
		botDriver->botData.aiPhysics.speedLinear = sdata->NavPath_ptrHeader[botDriver->botData.botPath]->rampPhys1[iVar4];

		if (data.characterIDs[botDriver->driverID] == NITROS_OXIDE)
		{
			botDriver->botData.aiPhysics.squishCooldown = 0;
			botDriver->botData.aiPhysics.mulDrift = 0;
			botDriver->botData.botFlags &= ~(BOT_FLAG_DAMAGE_ACTIVE | BOT_FLAG_DAMAGE_SUPPRESS_EMITTER);
		}
	}

	// MUST be unsigned, same reason as the other -0x31
	cmp1 = navFrameCurr->rot[3] - 0x31;
	cmp2 = navFrameNext->rot[3] - 0x31;

	if ((0x9e < cmp1) && (0x9e < cmp2))
	{
		int other_uVar8;
		b32 bVar1;
		s16 sVar7;
		b32 other_bVar1;
		if (((botDriver->botData.botFlags & BOT_FLAG_ESTIMATE_NAV) == 0) && ((botDriver->actionsFlagSet & ACTION_TOUCH_GROUND) != 0))
		{
			// Lerp nav-frame rotation through the shortest signed 12-bit delta.
			int uVar8 = CTR_MipsSubLo(CTR_MipsSll(navFrameNext->rot[0], 4), CTR_MipsSll(navFrameCurr->rot[0], 4)) & 0xfff;
			if (0x7ff < uVar8)
			{
				uVar8 = CTR_MipsSubLo(uVar8, 0x1000);
			}

			botDriver->botData.aiRot.x = CTR_MipsAddLo(CTR_MipsSll(navFrameCurr->rot[0], 4), CTR_MipsSra(CTR_MipsMulLo(uVar8, percentage), 0xc)) & 0xfff;

			uVar8 = CTR_MipsSubLo(CTR_MipsSll(navFrameNext->rot[2], 4), CTR_MipsSll(navFrameCurr->rot[2], 4)) & 0xfff;
			if (0x7ff < uVar8)
			{
				uVar8 = CTR_MipsSubLo(uVar8, 0x1000);
			}

			botDriver->botData.aiRot.z = CTR_MipsAddLo(CTR_MipsSll(navFrameCurr->rot[2], 4), CTR_MipsSra(CTR_MipsMulLo(uVar8, percentage), 0xc)) & 0xfff;
		}

		other_uVar8 = CTR_MipsSubLo(CTR_MipsSll(navFrameNext->rot[1], 4), CTR_MipsSll(navFrameCurr->rot[1], 4)) & 0xfff;
		if (0x7ff < other_uVar8)
		{
			other_uVar8 = CTR_MipsSubLo(other_uVar8, 0x1000);
		}

		botDriver->botData.aiRot.y = CTR_MipsAddLo(CTR_MipsSll(navFrameCurr->rot[1], 4), CTR_MipsSra(CTR_MipsMulLo(other_uVar8, percentage), 0xc)) & 0xfff;

		if ((botDriver->botData.botFlags & BOT_FLAG_ESTIMATE_NAV) != 0)
		{
			other_uVar8 = 0;
		}

		other_uVar8 = CTR_MipsSubLo(CTR_MipsSll(other_uVar8, 1), botDriver->turnAngleCurr) & 0xfff;

		bVar1 = other_uVar8 < 0x21;
		if (0x7ff < other_uVar8)
		{
			other_uVar8 = CTR_MipsSubLo(other_uVar8, 0x1000);
			bVar1 = other_uVar8 < 0x21;
		}
		if (bVar1)
		{
			if (other_uVar8 < -0x20)
			{
				other_uVar8 = -0x20;
			}
		}
		else
		{
			other_uVar8 = 0x20;
		}
		other_uVar8 = (s16)CTR_MipsAddLo((u16)botDriver->turnAngleCurr, other_uVar8);
		botDriver->turnAngleCurr = other_uVar8;


		if (botDriver->botData.aiPhysics.driftTarget == 0)
		{
			other_uVar8 &= 0xfff;
			sVar7 = other_uVar8;
			botDriver->botData.aiPhysics.simpTurnState = sVar7;
			if (0x7ff < other_uVar8)
			{
				botDriver->botData.aiPhysics.simpTurnState = (s16)CTR_MipsSubLo(sVar7, 0x1000);
			}
			sVar7 = (s16)CTR_MipsSra(botDriver->botData.aiPhysics.simpTurnState, 2);
		}
		else
		{
			int uVar11 = CTR_MipsNegLo((u16)botDriver->botData.aiPhysics.mulDrift) & 0xfff;

			botDriver->botData.aiPhysics.simpTurnState = uVar11;
			if (0x7ff < uVar11)
			{
				botDriver->botData.aiPhysics.simpTurnState = (s16)CTR_MipsSubLo(uVar11, 0x1000);
			}

			sVar7 = (s16)CTR_MipsSra(botDriver->botData.aiPhysics.simpTurnState, 3);
		}
		botDriver->botData.aiPhysics.simpTurnState = sVar7;

		other_uVar8 = CTR_MipsSubLo(botDriver->botData.aiPhysics.simpTurnState, botDriver->wheelRotation) & 0xfff;
		other_bVar1 = other_uVar8 < 0x21;

		if (0x7ff < other_uVar8)
		{
			other_uVar8 = CTR_MipsSubLo(other_uVar8, 0x1000);
			other_bVar1 = other_uVar8 < 0x21;
		}
		sVar7 = other_uVar8;
		if (other_bVar1)
		{
			if (other_uVar8 < -0x20)
			{
				sVar7 = -0x20;
			}
		}
		else
		{
			sVar7 = 0x20;
		}

		botDriver->wheelRotation = (s16)CTR_MipsAddLo((u16)botDriver->wheelRotation, sVar7);
		botDriver->simpTurnState = (s8)(u8)botDriver->botData.aiPhysics.simpTurnState;
	}

	botDriver->rotCurr.x = botDriver->botData.aiRot.x;
	botDriver->rotCurr.y = botDriver->botData.aiRot.y;
	botDriver->rotCurr.z = botDriver->botData.aiRot.z;

	badnessRecieveTimer = botDriver->clockReceive; // iVar4

	if (badnessRecieveTimer == 0)
	{
		badnessRecieveTimer = botDriver->squishTimer;
		if (badnessRecieveTimer == 0)
		{
			if (botDriver->thCloud != NULL)
			{
				badnessRecieveTimer = CTR_MipsSll(gGT->timer, 2);

				goto badEffectKartWiggle;
			}
		}
	}
	else
	{
		int wiggleX;
		int wiggleZ;
		SVECTOR wiggle;
	badEffectKartWiggle:;
		wiggleX = CTR_MipsSra(CTR_MipsMulLo(MATH_Sin(CTR_MipsMulLo(badnessRecieveTimer, 0xc)), 50), 10);
		wiggleZ = CTR_MipsSra(CTR_MipsMulLo(MATH_Cos(CTR_MipsMulLo(badnessRecieveTimer, 0xc)), 50), 10);
		wiggle.vx = (s16)wiggleX;
		wiggle.vy = 0;
		wiggle.vz = (s16)wiggleZ;
		wiggle.pad = 0;

		CTR_GteLoadSV0(&wiggle);
		gte_mvmva(1, 0, 0, 3, 0);

		botDriver->botData.aiPhysics.turboMeter = 0;

		botDriver->rotCurr.x = (s16)CTR_MipsAddLo((u16)botDriver->rotCurr.x, (s16)MFC2(25));
		botDriver->rotCurr.y = (s16)CTR_MipsAddLo((u16)botDriver->rotCurr.y, (s16)MFC2(26));
		botDriver->rotCurr.z = (s16)CTR_MipsAddLo((u16)botDriver->rotCurr.z, (s16)MFC2(27));
	}

	if ((botDriver->botData.botFlags & (BOT_FLAG_ESTIMATE_NAV | BOT_FLAG_FREE_PHYSICS)) == 0)
	{
		MATRIX m;
		SVec3 rot;
		rot.x = (s16)CTR_MipsSll(navFrameCurr->rot[0], 4);
		rot.y = (s16)CTR_MipsSll(navFrameCurr->rot[1], 4);
		rot.z = (s16)CTR_MipsSll(navFrameCurr->rot[2], 4);


		ConvertRotToMatrix(&m, &rot);

		botDriver->AxisAngle3_normalVec.x = m.m[0][1];
		botDriver->AxisAngle3_normalVec.y = m.m[1][1];
		botDriver->AxisAngle3_normalVec.z = m.m[2][1];

		botInstance->compressedNormalAndDriverIndex = INST_CompressNormalVectorAndDriverIndex(m.m[0][1], m.m[1][1], m.m[2][1], botDriver->driverID);
	}

	ConvertRotToMatrix(&botInstance->matrix, SVec3Slot_AsVec3(&botDriver->rotCurr));

	// c is row-major (i.e., ticking the rightmost indeces has smaller memory address delta vs ticking the leftmost indeces)
	botDriver->AxisAngle2_normalVec.x = botInstance->matrix.m[0][1];
	botDriver->AxisAngle2_normalVec.y = botInstance->matrix.m[1][1];
	botDriver->AxisAngle2_normalVec.z = botInstance->matrix.m[2][1];

	botDriver->angle = botDriver->rotCurr.y;

	botDriver->speedApprox = botDriver->botData.aiPhysics.speedLinear;

	botDriver->speed = botDriver->botData.aiPhysics.speedLinear;
	botDriver->jumpHeightPrev = botDriver->jumpHeightCurr;
	botDriver->axisRotationX = botDriver->botData.aiRot.y & 0xfff;

	iVar4_lifetime_2 = MATH_Cos(navFrameCurr->rot[3]);

	botDriver->jumpHeightCurr = (s16)CTR_MipsSra(CTR_MipsMulLo(botDriver->botData.aiPhysics.speedY, iVar4_lifetime_2), 0xc);

	iVar4_lifetime_2 = MATH_Cos(botDriver->axisRotationX);

	botDriver->zSpeed = CTR_MipsSra(CTR_MipsMulLo(botDriver->botData.aiPhysics.speedLinear, iVar4_lifetime_2), 0xc);

	iVar4_lifetime_2 = MATH_Sin(botDriver->axisRotationX);

	uVar11 = botDriver->rotCurr.z & 0xfff;

	botDriver->ySpeed = botDriver->botData.aiPhysics.speedY;
	botDriver->rotCurr.z = uVar11;

	botDriver->xSpeed = CTR_MipsSra(CTR_MipsMulLo(botDriver->botData.aiPhysics.speedLinear, iVar4_lifetime_2), 0xc);
	if (0x7ff < uVar11)
	{
		botDriver->rotCurr.z = (s16)CTR_MipsSubLo(uVar11, 0x1000);
	}
	botDriver->rotCurr.y =
	    (s16)CTR_MipsAddLo((u16)botDriver->rotCurr.y, CTR_MipsAddLo((u16)botDriver->botData.aiPhysics.mulDrift, (u16)botDriver->turnAngleCurr));

	botDriver->posCurr.x = CTR_MipsAddLo(botDriver->botData.aiPhysics.velocity.x, botDriver->botData.positionBackup.x);
	botDriver->posCurr.y = CTR_MipsAddLo(botDriver->botData.aiPhysics.velocity.y, botDriver->botData.positionBackup.y);
	botDriver->posCurr.z = CTR_MipsAddLo(botDriver->botData.aiPhysics.velocity.z, botDriver->botData.positionBackup.z);

	botInstance->matrix.t[0] = CTR_MipsSra(botDriver->posCurr.x, FRACTIONAL_BITS_8);
	botInstance->matrix.t[1] = CTR_MipsAddLo(CTR_MipsSra(botDriver->posCurr.y, FRACTIONAL_BITS_8), botDriver->Screen_OffsetY);
	botInstance->matrix.t[2] = CTR_MipsSra(botDriver->posCurr.z, FRACTIONAL_BITS_8);

	badnessRecieveTimer = botDriver->clockReceive;

	if (badnessRecieveTimer == 0)
	{
		badnessRecieveTimer = botDriver->squishTimer;
		if (badnessRecieveTimer == 0)
		{
			u16 uVar11;
			if (0x100 < botDriver->speedApprox)
			{
				botDriver->hazardTimer = (s16)CTR_MipsSubLo((u16)botDriver->hazardTimer, elapsedMilliseconds);
			}

			uVar11 = (u16)botDriver->hazardTimer & 0xfffe;
			botDriver->hazardTimer = (s16)uVar11;

			if ((s16)uVar11 >= 0)
			{
				botDriver->hazardTimer = -2;
			}
			goto FinishHazardTimerUpdate;
		}
	}

	if ((botDriver->actionsFlagSet & ACTION_TOUCH_GROUND) == 0)
	{
		int iVar4 = botDriver->speedApprox;
		if (iVar4 < 0)
		{
			iVar4 = CTR_MipsNegLo(iVar4);
		}

		if (iVar4 < 0x101)
		{
			goto CheckExistingHazardTimer;
		}

		if (badnessRecieveTimer < 0)
		{
			badnessRecieveTimer = CTR_MipsNegLo(badnessRecieveTimer);
		}
	StoreNegativeHazardTimer:
		botDriver->hazardTimer = (s16)CTR_MipsNegLo(badnessRecieveTimer);
	}
	else
	{
		int iVar4 = botDriver->speedApprox;

		if (iVar4 < 0)
		{
			iVar4 = CTR_MipsNegLo(iVar4);
		}

		if (iVar4 < 0x101)
		{
		CheckExistingHazardTimer:
			badnessRecieveTimer = botDriver->hazardTimer;
			if (0 < badnessRecieveTimer)
			{
				goto StoreNegativeHazardTimer;
			}
		}
		else
		{
			if (badnessRecieveTimer < 0)
			{
				badnessRecieveTimer = CTR_MipsNegLo(badnessRecieveTimer);
			}
			botDriver->hazardTimer = (s16)badnessRecieveTimer;
		}
	}

	botDriver->hazardTimer = (s16)((u16)botDriver->hazardTimer | 1); // increment it by 1 if even.

FinishHazardTimerUpdate:

	if (((navFrameSpecialBits & BOTS_NAV_SPECIAL_LEVEL_INST_COLL) != 0) ||
	    ((botDriver->botData.botFlags & (BOT_FLAG_ESTIMATE_NAV | BOT_FLAG_FREE_PHYSICS)) != 0))
	{
		BOTS_LevInstColl(botThread);
	}

	if ((navFrameFlags & BOTS_NAV_FLAG_SPLIT_LINE) != 0)
	{
		botInstance->flags |= SPLIT_LINE;
	}

	VehPhysForce_TranslateMatrix(botThread, botDriver);

	VehPhysForce_RotAxisAngle(&botDriver->matrixMovingDir, CTR_VECTOR_DATA(&(botDriver->AxisAngle2_normalVec)), botDriver->angle);

	VehFrameProc_Driving(botThread, botDriver);

	if (((botDriver->botData.botFlags & BOT_FLAG_DAMAGE_ACTIVE) != 0) && (botDriver->botData.aiDamageState == BOTS_DAMAGE_STATE_BLAST))
	{
		SVec3 rot;
		rot.x = (s16)CTR_MipsSll(botDriver->botData.aiPhysics.rotXZ, 8);
		rot.z = 0;
		rot.y = (s16)CTR_MipsMulLo(botDriver->botData.aiPhysics.rotXZ, 0xe0);

		ConvertRotToMatrix(&sdata->rotXZ, &rot);

		MATH_MatrixMul(&sdata->rotXYZ, &botInstance->matrix, &sdata->rotXZ);

		MATH_MatrixMul(&botInstance->matrix, &sdata->rotXYZ, &data.identity);

		botInstance->matrix.t[1] = CTR_MipsAddLo(botInstance->matrix.t[1], 0x20);
	}

	if ((botDriver->botData.botFlags & BOT_FLAG_DAMAGE_SUPPRESS_EMITTER) == 0)
	{
		VehEmitter_DriverMain(botThread, botDriver);
	}

	if (botInstance->thread->modelIndex == DYNAMIC_PLAYER)
	{
		EngineSound_Player(botDriver);
	}

	camRot = CTR_MipsSubLo(botDriver->angle, botDriver->botData.ai_rotY_608) & 0xfff;

	botDriver->rotCurr.w = (s16)CTR_MipsNegLo(camRot);

	if (0x7ff < camRot)
	{
		camRot |= 0xf000;
	}

	botDriver->botData.ai_rotY_608 = (s16)CTR_MipsAddLo((u16)botDriver->botData.ai_rotY_608, CTR_MipsSra(camRot, 3));
	botDriver->botData.ai_rotY_608 &= 0xfff;

	if (botThread->modelIndex == DYNAMIC_PLAYER)
	{
		SVec3 probeTop;
		SVec3 probeBottom;
		s16 probeX = (s16)CTR_MipsSra(CTR_MipsAddLo(botDriver->botData.positionBackup.x, botDriver->botData.aiPhysics.velocity.x), 8);
		s16 probeY = (s16)CTR_MipsSra(CTR_MipsAddLo(botDriver->botData.positionBackup.y, botDriver->botData.aiPhysics.velocity.y), 8);
		s16 probeZ = (s16)CTR_MipsSra(CTR_MipsAddLo(botDriver->botData.positionBackup.z, botDriver->botData.aiPhysics.velocity.z), 8);
		probeTop.x = probeX;
		probeTop.y = (s16)CTR_MipsSubLo(probeY, 0x100);
		probeTop.z = probeZ;
		probeBottom.x = probeX;
		probeBottom.y = (s16)CTR_MipsAddLo(probeY, 0x40);
		probeBottom.z = probeZ;

		sps->ptr_mesh_info = gGT->level1->ptr_mesh_info;
		sps->Union.QuadBlockColl.quadFlagsWanted = QUADBLOCK_FLAG_GROUND;
		sps->Union.QuadBlockColl.quadFlagsIgnored = 0;
		sps->Union.QuadBlockColl.searchFlags = COLL_SEARCH_HIGH_LOD;

		COLL_SearchBSP_CallbackQUADBLK(&probeTop, &probeBottom, sps, 0);

		if (sps->boolDidTouchQuadblock != 0)
		{
			botDriver->underDriver = sps->hit.ptrQuadblock;
		}
	}
}

u32 BOTS_ChangeState(struct Driver *driverVictim, int damageType, struct Driver *driverAttacker, int reason)
{
	driverVictim->pendingDamageType = 0;

	if (driverVictim->kartState == KS_MASK_GRABBED)
	{
		return 0;
	}

	driverVictim->kartState = KS_NORMAL;

	switch (damageType)
	{
	case 1:
	case 4:
	{
		s16 *characters;
		u32 victimID;
		u32 characterPage;
		s32 spinState;
		if ((driverVictim->botData.botFlags & BOT_FLAG_DAMAGE_ACTIVE) == 0)
		{
			// Keep the canonical character array address whole during the first lookup.
			CTR_PSX_LOAD_IMMEDIATE(spinState, BOTS_DAMAGE_STATE_SPIN);
			CTR_PSX_LOAD_SYMBOL_PAGE_AFTER(characterPage, "data+25828", spinState);
			CTR_PSX_STORE_HALF(driverVictim->botData.aiDamageState, spinState);
			CTR_PSX_BIND_VALUE_CLOBBER(characterPage, "$2");
			victimID = driverVictim->driverID;
			CTR_PSX_DEPEND_VALUE(characterPage, victimID);
			CTR_PSX_ADD_SYMBOL_LOW(characters, characterPage, "data+25828", data.characterIDs);
			CTR_PSX_FORGET_VALUE(characters);
			driverVictim->botData.aiPhysics.turboMeter = 0;
			CTR_PSX_DEPEND_MEMORY(&driverVictim->botData.aiPhysics.turboMeter, characters);
			CTR_PSX_FORGET_VALUE(victimID);

			if ((characters[victimID] != NITROS_OXIDE) || ((driverVictim->actionsFlagSet & ACTION_TOUCH_GROUND) != 0))
			{
				int newSpeed;
				driverVictim->reserves = 0;
				driverVictim->turbo_outsideTimer = 0;
				driverVictim->botData.aiPhysics.reserved_0x5cc = 0;

				if (characters[driverVictim->driverID] == NITROS_OXIDE)
				{
					newSpeed = CTR_MipsSra(driverVictim->botData.aiPhysics.speedLinear, 1);
				}
				else
				{
					newSpeed = CTR_MipsSra(driverVictim->botData.aiPhysics.speedLinear, 2);
				}

				driverVictim->botData.aiPhysics.speedLinear = newSpeed;
			}

			driverVictim->botData.aiPhysics.squishCooldown = 0x300;
			driverVictim->botData.ai_progress_cooldown = 0;

			driverVictim->botData.botFlags |= BOT_FLAG_DAMAGE_ACTIVE;
		}

		if (damageType == 4)
		{
			if (driverVictim->instSelf->thread->modelIndex == DYNAMIC_PLAYER && driverVictim->burnTimer == 0)
			{
				OtherFX_Play(0x69, 1);
			}

			driverVictim->burnTimer = 0xf00;
		}
		break;
	}
	case 2:
	{
		s32 speedY;
		u32 damageFlags;
		driverVictim->botData.blastBounceCount = 0;
		CTR_PSX_MEMORY_BARRIER();
		speedY = sdata->AI_VelY_WhenBlasted_0x3000;
		CTR_PSX_LOAD_WORD_AFTER(damageFlags, driverVictim->botData.botFlags, speedY);
		CTR_PSX_OBSERVE_VALUE(speedY);
		CTR_PSX_BIND_VALUE_CLOBBER(damageFlags, "$2");
		driverVictim->botData.aiPhysics.turboMeter = 0;
		driverVictim->botData.aiDamageState = BOTS_DAMAGE_STATE_BLAST;
		driverVictim->reserves = 0;
		driverVictim->turbo_outsideTimer = 0;
		driverVictim->botData.aiPhysics.reserved_0x5cc = 0;
		CTR_PSX_MEMORY_BARRIER();
		CTR_PSX_FORGET_VALUE(damageFlags);
		driverVictim->botData.aiPhysics.speedY = speedY;

		if ((damageFlags & BOT_FLAG_DAMAGE_ACTIVE) == 0)
		{
			driverVictim->botData.aiPhysics.speedLinear = CTR_MipsSra(driverVictim->botData.aiPhysics.speedLinear, 3);
			driverVictim->botData.positionBackup.y = CTR_MipsAddLo(driverVictim->botData.positionBackup.y, 0x4000);
		}

		driverVictim->botData.ai_progress_cooldown = 0;
		driverVictim->matrixArray = BAKED_GTE_MATRIX_NONE;
		driverVictim->botData.botFlags |= BOT_FLAG_DAMAGE_ACTIVE;

		if (driverAttacker == NULL)
		{
			goto ChangeStateSuccess;
		}

		if ((driverAttacker->actionsFlagSet & ACTION_BOT) == 0)
		{
			int voiceKind = 1;
			u32 characterPage = 0x80080000;
			u32 victimID;
			s16 *characters;
			s16 *characterEntry;
			u32 victimOffset;
			CTR_PSX_OBSERVE_VALUE(voiceKind);
			CTR_PSX_FORGET_VALUE(voiceKind);
			CTR_PSX_BIND_VALUE_CLOBBER(characterPage, "$2");
			victimID = driverVictim->driverID;
			CTR_PSX_DEPEND_VALUE(characterPage, victimID);
			CTR_PSX_ADD_PAGE_OFFSET(characters, characterPage, 0x6e84, data.characterIDs);
			CTR_PSX_FORGET_VALUE(characters);
			victimOffset = (u32)CTR_MipsSll(victimID, voiceKind);
			CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(characterEntry, characters, victimOffset);
			Voiceline_RequestPlay(voiceKind, *characterEntry, 0x10);
		}
		break;
	}
	case 3:
	{
		// This role receives the complete speedLinear word before the shift.
		register s32 speed CTR_PSX_REGISTER("$2");
		u32 damageFlags;
		driverVictim->botData.aiPhysics.turboMeter = 0;

		if (driverVictim->instSelf->thread->modelIndex == DYNAMIC_PLAYER && driverVictim->botData.aiPhysics.squishCooldown == 0)
		{
			OtherFX_Play(0x5a, 1);
		}

		driverVictim->botData.aiDamageState = BOTS_DAMAGE_STATE_SQUISH;
		driverVictim->botData.aiPhysics.squishCooldown = 0x300;
		CTR_PSX_MEMORY_BARRIER();
		CTR_PSX_LOAD_WORD_VOLATILE(speed, driverVictim->botData.aiPhysics.speedLinear);
		driverVictim->botData.aiPhysics.rotXZ = 0xf00;
		driverVictim->squishTimer = 0xf00;
		CTR_PSX_MEMORY_BARRIER();
		CTR_PSX_LOAD_WORD_VOLATILE(damageFlags, driverVictim->botData.botFlags);
		driverVictim->reserves = 0;
		driverVictim->turbo_outsideTimer = 0;
		driverVictim->botData.aiPhysics.reserved_0x5cc = 0;
		driverVictim->botData.aiPhysics.speedY = 0;
		driverVictim->botData.ai_progress_cooldown = 0;
		CTR_PSX_MEMORY_BARRIER();
		CTR_PSX_FORGET_VALUE(speed);
		CTR_PSX_FORGET_VALUE(damageFlags);
		CTR_PSX_SHIFT_RIGHT_ARITHMETIC_IN_PLACE(speed, 1);
		CTR_PSX_DEPEND_VALUE(damageFlags, speed);
		damageFlags |= BOT_FLAG_DAMAGE_ACTIVE | BOT_FLAG_DAMAGE_SUPPRESS_EMITTER;
		driverVictim->botData.aiPhysics.speedLinear = speed;
		driverVictim->botData.botFlags = damageFlags;
		break;
	}
	case 5:
	{
		u32 damageFlags;
		driverVictim->botData.aiPhysics.turboMeter = 0;
		driverVictim->botData.aiPhysics.speedLinear = 0;
		driverVictim->botData.aiPhysics.speedY = 0;
		driverVictim->botData.aiPhysics.reserved_0x5cc = 0;
		driverVictim->instSelf->flags |= HIDE_MODEL;
		driverVictim->botData.aiPhysics.rotXZ = 0xd20;
		CTR_PSX_LOAD_WORD_VOLATILE(damageFlags, driverVictim->botData.botFlags);
		CTR_PSX_BIND_VALUE_CLOBBER(damageFlags, "$3");
		driverVictim->botData.aiDamageState = BOTS_DAMAGE_STATE_MASK_GRAB;
		driverVictim->kartState = KS_MASK_GRABBED;
		driverVictim->botData.botFlags = damageFlags | BOT_FLAG_DAMAGE_ACTIVE | BOT_FLAG_DAMAGE_SUPPRESS_EMITTER;
		driverVictim->instSelf->thread->flags |= THREAD_FLAG_DISABLE_COLLISION;
		break;
	}
	case 0:
		if ((driverVictim->botData.botFlags & BOT_FLAG_DAMAGE_ACTIVE) != 0)
		{
			return 0;
		}
		break;
	default:
		driverVictim->botData.ai_progress_cooldown = 0x3c;
	}

	if (driverAttacker != NULL && damageType != 0)
	{
		driverAttacker->numTimesAttacking++;
		switch (reason)
		{
		case 1:
			driverAttacker->numTimesBombsHitSomeone++;
			break;
		case 3:
			driverAttacker->numTimesMissileHitSomeone++;
			break;
		case 4:
			driverAttacker->numTimesMovingPotionHitSomeone++;
			break;
		}
	}
ChangeStateSuccess:
	return 1;
}

void BOTS_CollideWithOtherAI(struct Driver *robot_1, struct Driver *robot_2)
{
	register SVec3 *navSegmentStartPos CTR_PSX_REGISTER("$19");
	register SVec3 *navSegmentEndPos CTR_PSX_REGISTER("$16");
	SVec3 pos;
	int res1;
	int res2;
	// first determine which driver bumps forward and which bumps backwards
	if (robot_1->driverRank < robot_2->driverRank)
	{
		struct Driver *temp = robot_1;
		robot_1 = robot_2;
		robot_2 = temp;
	}

	if ((robot_1->botData.botFlags & BOT_FLAG_ESTIMATE_NAV) != 0)
	{
		// pointer to nav frame
		struct NavFrame *navFrameNext = robot_1->botData.botNavFrame;

		navSegmentStartPos = &robot_1->botData.estimateNavFrame.pos;
		navSegmentEndPos = &navFrameNext->pos;
	}
	else
	{
		// nav path index
		s16 botPathIndex = robot_1->botData.botPath;

		// NavFrame.pos is the first member, so these roles also hold its position pointer.
		register struct NavFrame *navFrameCurr CTR_PSX_REGISTER("$19") = robot_1->botData.botNavFrame;
		register struct NavFrame *navFrameNext CTR_PSX_REGISTER("$16") = navFrameCurr + 1;

		navSegmentStartPos = &navFrameCurr->pos;

		// if you go out of bounds
		if (BOT_NAV_HEADERS[botPathIndex]->last <= navFrameNext)
		{
			// loop back to first navFrame
			navFrameNext = BOT_NAV_FRAMES[botPathIndex];
		}
		navSegmentEndPos = &navFrameNext->pos;
	}
	pos.x = (s16)CTR_MipsSra(robot_1->posCurr.x, FRACTIONAL_BITS_8);
	pos.y = (s16)CTR_MipsSra(robot_1->posCurr.y, FRACTIONAL_BITS_8);
	pos.z = (s16)CTR_MipsSra(robot_1->posCurr.z, FRACTIONAL_BITS_8);

	res1 = CAM_MapRange_PosPoints(navSegmentEndPos, navSegmentStartPos, &pos);

	pos.x = (s16)CTR_MipsSra(robot_2->posCurr.x, FRACTIONAL_BITS_8);
	pos.y = (s16)CTR_MipsSra(robot_2->posCurr.y, FRACTIONAL_BITS_8);
	pos.z = (s16)CTR_MipsSra(robot_2->posCurr.z, FRACTIONAL_BITS_8);

	res2 = CAM_MapRange_PosPoints(navSegmentEndPos, navSegmentStartPos, &pos);

	// reduce speed of one AI,
	// the AI that is closer to the previous nav point,
	// who therefore is the driver in the back of the collision

	if (res1 < res2)
	{
		int speed = CTR_MipsSubLo(robot_2->botData.aiPhysics.speedLinear, BOTS_AI_COLLISION_SPEED_PENALTY);
		robot_1->botData.aiPhysics.speedLinear = speed;
		if (speed < 0)
		{
			robot_1->botData.aiPhysics.speedLinear = 0;
		}
	}
	else
	{
		int speed = CTR_MipsSubLo(robot_1->botData.aiPhysics.speedLinear, BOTS_AI_COLLISION_SPEED_PENALTY);
		robot_2->botData.aiPhysics.speedLinear = speed;
		if (speed < 0)
		{
			robot_2->botData.aiPhysics.speedLinear = 0;
		}
	}
}

void BOTS_GotoStartingLine(struct Driver *d)
{
	register s32 teleportKind CTR_PSX_REGISTER("$5") = VEH_BIRTH_SPAWN_RACE_START;
	register struct Driver *rotationDriver CTR_PSX_REGISTER("$4");
	struct NavFrame *navFrame;
	u32 spawnPos;
	s32 accelDuration;
	u32 accel;
	s16 rotY;
	int rng;
	s32 posX;
	s32 posY;
	register s32 posZ CTR_PSX_REGISTER("$5");
	CTR_PSX_OBSERVE_VALUE(teleportKind);
	sdata->aiCollisionDelayFrameCount = 0;
	CTR_PSX_OBSERVE_MEMORY(sdata->aiCollisionDelayFrameCount);
	VehBirth_TeleportSelf(d, teleportKind, 0);
	spawnPos = (u8)BOTS_KART_ORDER[d->driverID];
	posZ = d->posCurr.z;
	posX = d->posCurr.x;
	posY = d->posCurr.y;
	CTR_PSX_CLOBBER("$4");
	rotationDriver = d;
	CTR_PSX_OBSERVE_VALUE(rotationDriver);
	d->botData.positionBackup.z = posZ;
	CTR_PSX_MEMORY_BARRIER();
	d->botData.aiPhysics.velocity.z = 0;
	d->botData.aiPhysics.velocity.y = 0;
	d->botData.aiPhysics.velocity.x = 0;
	d->botData.aiPhysics.accel.z = 0;
	d->botData.aiPhysics.accel.y = 0;
	d->botData.aiPhysics.accel.x = 0;
	d->botData.aiPhysics.speedLinear = 0;
	d->botData.aiPhysics.speedY = 0;
	d->botData.aiPhysics.reserved_0x5cc = 0;
	d->botData.positionBackup.x = posX;
	d->botData.positionBackup.y = posY;
	navFrame = BOT_NAV_FRAMES[d->botData.botPath];
	CTR_PSX_DEPEND_MEMORY(&d->botData.navProgressRemainder, navFrame);
	d->botData.navProgressRemainder = 0;
	d->botData.botNavFrame = navFrame;
	BOTS_SetRotation(rotationDriver, 1);
	accel = BOTS_ACCEL_ORDER[spawnPos];
	accelDuration = sdata->AI_AccelFrameCount;
	d->rotCurr.z = 0;
	d->rotPrev.z = 0;
	d->botData.aiRot.z = 0;
	d->rotCurr.x = 0;
	d->rotPrev.x = 0;
	d->botData.aiRot.x = 0;
	d->turnAngleCurr = 0;
	d->actionsFlagSet |= ACTION_BOT;
	rotY = (s16)CTR_MipsSll((u8)d->botData.estimateNavFrame.rot[1], BOTS_ROT_BYTE_SHIFT);
	d->botData.ai_rotY_608 = rotY;
	d->angle = rotY;
	d->rotCurr.y = rotY;
	d->rotPrev.y = rotY;
	d->botData.aiRot.y = rotY;
	d->botData.botAccel = CTR_MipsMulLo(accelDuration, accel);
	rng = RngDeadCoed(&BOTS_ADV_RNG);
	d->botData.weaponCooldown = (s16)CTR_MipsAddLo(CTR_MipsSra(rng, 8) & 0xff, 300);
}

struct Driver *BOTS_Driver_Init(int driverID)
{
	register struct Thread *t CTR_PSX_REGISTER("$16");
	register struct Driver *d CTR_PSX_REGISTER("$18");
	register s32 navPathIndex CTR_PSX_REGISTER("$17");
	register s16 initialNavPathIndex CTR_PSX_REGISTER("$6");
	register int pathFound CTR_PSX_REGISTER("$5");
	register int points CTR_PSX_REGISTER("$2");
	register u32 rawPath CTR_PSX_REGISTER("$2");
	register char *pathSlot CTR_PSX_REGISTER("$2");
	register struct NavHeader *header CTR_PSX_REGISTER("$2");
	register struct NavHeader **headerSlot CTR_PSX_REGISTER("$2");
	register struct NavHeader **headers CTR_PSX_REGISTER("$3");

	register int id CTR_PSX_REGISTER("$19") = driverID;
	CTR_PSX_LOAD_SYMBOL_PAGE_AFTER(pathSlot, "sdata_static+12", id);
	CTR_PSX_ADD_SYMBOL_LOW_IN_PLACE(pathSlot, "sdata_static+12", BOTS_DRIVER_PATHS);
	CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(pathSlot, pathSlot, id);
	CTR_PSX_ZERO_VALUE_AFTER(d, id, pathSlot);
	CTR_PSX_LOAD_SYMBOL_PAGE_AFTER(headers, "sdata_static+2944", d);
	CTR_PSX_LOAD_UNSIGNED_BYTE(rawPath, pathSlot, 0, (u8)*pathSlot);
	CTR_PSX_ADD_SYMBOL_LOW_IN_PLACE(headers, "sdata_static+2944", BOT_NAV_HEADERS);
	CTR_PSX_OBSERVE_VALUE(rawPath);
	navPathIndex = (s8)rawPath;
	CTR_PSX_KEEP_VALUE(navPathIndex);
	{
		register s32 headerOffset CTR_PSX_REGISTER("$2") = CTR_MipsSll((s16)navPathIndex, 2);
		CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(headerSlot, headers, headerOffset);
	}
	CTR_PSX_LOAD_WORD_FROM_PAGE(header, headerSlot, "0", BOT_NAV_HEADERS[(s16)navPathIndex]);
	CTR_PSX_DEPEND_VALUE(d, header);
	CTR_PSX_COPY_VALUE(pathFound, (u32)d);
	CTR_PSX_LOAD_SIGNED_HALF(points, header, 2, header->numPoints);
	initialNavPathIndex = navPathIndex;
	goto CheckInitNavCount;
	while (1)
	{
		{
			register int next CTR_PSX_REGISTER("$2") = navPathIndex - 1;
			CTR_PSX_COPY_VALUE(navPathIndex, next);
			if ((s16)next < 0)
				navPathIndex = 2;
		}
		if ((s16)navPathIndex == initialNavPathIndex)
			break;
		CTR_PSX_LOAD_WORD(header, BOT_NAV_HEADERS[(s16)navPathIndex]);
		CTR_PSX_LOAD_SIGNED_HALF(points, header, 2, header->numPoints);
	CheckInitNavCount:
		if (1 < points)
		{
			pathFound = 1;
			break;
		}
	}
	CTR_PSX_COPY_VALUE(points, pathFound);
	if (points)
	{
		t = PROC_BirthWithObject(SIZE_RELATIVE_POOL_BUCKET(DRIVER_NTSC_RETAIL_SIZE, NONE, LARGE, ROBOT), BOTS_ThTick_Drive, BOTS_ROBOT_DEBUG_NAME, 0);
		d = t->object;
		memset(d, 0, DRIVER_NTSC_RETAIL_SIZE);
		VehBirth_NonGhost(t, id);
		{
			register struct LinkedList *list CTR_PSX_REGISTER("$4") = &BOTS_NAV_LIST[(s16)navPathIndex];
			register int driverOffset CTR_PSX_REGISTER("$6");
			register int frameOffset CTR_PSX_REGISTER("$5");
			register struct GameTracker *tracker CTR_PSX_REGISTER("$2");
			register struct Driver *object CTR_PSX_REGISTER("$3");
			CTR_PSX_OBSERVE_VALUE(list);
			driverOffset = CTR_MipsSll(id, 2);
			CTR_PSX_OBSERVE_VALUE(driverOffset);
			frameOffset = CTR_MipsSll((s16)navPathIndex, 2);
			CTR_PSX_LOAD_SYMBOL_PAGE_AFTER(tracker, "sdata_static+832", frameOffset);
			CTR_PSX_LOAD_WORD_FROM_PAGE(tracker, tracker, "sdata_static+832", GAME_TRACKER);
			object = t->object;
			CTR_PSX_DEPEND_VALUE(tracker, object);
			tracker->drivers[id] = object;
			t->modelIndex = DYNAMIC_ROBOT_CAR;
			d->botData.botPath = (s16)navPathIndex;
			d->actionsFlagSet |= ACTION_BOT;
			d->botData.botNavFrame = *(struct NavFrame **)((u8 *)BOT_NAV_FRAMES + frameOffset);
			LIST_AddFront(list, &d->botData.item);
		}
		GAME_TRACKER->numBotsNextGame++;
		BOTS_GotoStartingLine(d);
	}
	return d;
}

void BOTS_Driver_Convert(struct Driver *driver)
{
	register struct Driver *original CTR_PSX_REGISTER("$18") = driver;
	register struct Driver *d CTR_PSX_REGISTER("$17");
	register s32 navPathIndex CTR_PSX_REGISTER("$16");
	register s16 initialNavPathIndex CTR_PSX_REGISTER("$6");
	register int pathFound CTR_PSX_REGISTER("$5");
	register int points CTR_PSX_REGISTER("$2");
	register u32 rawPath CTR_PSX_REGISTER("$2");
	register char *pathSlot CTR_PSX_REGISTER("$2");
	register char *paths CTR_PSX_REGISTER("$3");
	register struct NavHeader *header CTR_PSX_REGISTER("$2");
	register struct NavHeader **headerSlot CTR_PSX_REGISTER("$2");
	register struct NavHeader **headers CTR_PSX_REGISTER("$3");
	register s32 speedApprox CTR_PSX_REGISTER("$2");
	register struct NavFrame *firstNavFrame CTR_PSX_REGISTER("$2");
	register u32 oldActionFlagsSet CTR_PSX_REGISTER("$2");
	int damageType;
	if ((original->actionsFlagSet & ACTION_BOT) != 0)
		return;
	d = original;
	CTR_PSX_KEEP_VALUE_RELAXED(d);
	CTR_PSX_CLOBBER("$4");
	UI_RaceEnd_GetDriverClock(d);
	rawPath = d->driverID;
	CTR_PSX_LOAD_SYMBOL_PAGE_AFTER(paths, "sdata_static+12", rawPath);
	CTR_PSX_ADD_SYMBOL_LOW_IN_PLACE(paths, "sdata_static+12", BOTS_DRIVER_PATHS);
	CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(pathSlot, paths, rawPath);
	CTR_PSX_LOAD_SYMBOL_PAGE(headers, "sdata_static+2944");
	CTR_PSX_LOAD_UNSIGNED_BYTE(rawPath, pathSlot, 0, (u8)*pathSlot);
	CTR_PSX_ADD_SYMBOL_LOW_IN_PLACE(headers, "sdata_static+2944", BOT_NAV_HEADERS);
	CTR_PSX_OBSERVE_VALUE(rawPath);
	navPathIndex = (s8)rawPath;
	CTR_PSX_KEEP_VALUE(navPathIndex);
	{
		register s32 headerOffset CTR_PSX_REGISTER("$2") = CTR_MipsSll((s16)navPathIndex, 2);
		CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(headerSlot, headers, headerOffset);
	}
	CTR_PSX_LOAD_WORD_FROM_PAGE(header, headerSlot, "0", BOT_NAV_HEADERS[(s16)navPathIndex]);
	CTR_PSX_ZERO_VALUE_AFTER(pathFound, header, navPathIndex);
	CTR_PSX_LOAD_SIGNED_HALF(points, header, 2, header->numPoints);
	initialNavPathIndex = navPathIndex;
	goto CheckConvertNavCount;
	while (1)
	{
		{
			register int next CTR_PSX_REGISTER("$2") = navPathIndex - 1;
			CTR_PSX_COPY_VALUE(navPathIndex, next);
			if ((s16)next < 0)
				navPathIndex = 2;
		}
		if ((s16)navPathIndex == initialNavPathIndex)
			break;
		CTR_PSX_LOAD_WORD(header, BOT_NAV_HEADERS[(s16)navPathIndex]);
		CTR_PSX_LOAD_SIGNED_HALF(points, header, 2, header->numPoints);
	CheckConvertNavCount:
		if (1 < points)
		{
			pathFound = 1;
			break;
		}
	}
	CTR_PSX_COPY_VALUE(points, pathFound);
	if (!points)
		return;

	memset(&original->botData, 0, sizeof(struct BotData));

	{
		register s32 signedPath CTR_PSX_REGISTER("$5");
		register s32 pathHigh CTR_PSX_REGISTER("$3") = CTR_MipsSll(navPathIndex, 16);
		register s32 ySpeed CTR_PSX_REGISTER("$2") = original->ySpeed;
		register int frameOffset CTR_PSX_REGISTER("$6");
		register struct GameTracker *tracker CTR_PSX_REGISTER("$4");
		CTR_PSX_LOAD_SYMBOL_PAGE_AFTER(tracker, "sdata_static+832", ySpeed);
		CTR_PSX_LOAD_WORD_FROM_PAGE(tracker, tracker, "sdata_static+832", GAME_TRACKER);
		CTR_PSX_SHIFT_RIGHT_ARITHMETIC(signedPath, pathHigh, 16);
		d->botData.aiPhysics.speedY = ySpeed;
		speedApprox = original->speedApprox;
		CTR_PSX_DEPEND_VALUE(signedPath, speedApprox);
		frameOffset = CTR_MipsSll(signedPath, 2);
		CTR_PSX_STORE_HALF(d->botData.botPath, navPathIndex);
		if (speedApprox < 0)
			CTR_PSX_NEGATE_IN_PLACE(speedApprox);
		d->botData.aiPhysics.speedLinear = speedApprox;
		{
			register struct NavFrame **frameSlot CTR_PSX_REGISTER("$2") = BOT_NAV_FRAMES;
			CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(frameSlot, frameSlot, frameOffset);
			CTR_PSX_LOAD_WORD_FROM_PAGE(firstNavFrame, frameSlot, "0", BOT_NAV_FRAMES[(s16)navPathIndex]);
		}

		d->botData.navProgressRemainder = 0;
		d->turnAngleCurr = 0;
		d->multDrift = 0;
		d->ampTurnState = 0;
		d->wallRubTimer = 0;

		CTR_PSX_DEPEND_MEMORY(&d->botData.botNavFrame, firstNavFrame);
		d->botData.botNavFrame = firstNavFrame;

		original->instSelf->thread->funcThTick = BOTS_ThTick_Drive;

		if ((tracker->gameMode1 & BATTLE_MODE) != 0)
		{ // you are in battle mode
			register struct NavHeader **slot CTR_PSX_REGISTER("$3") = BOT_NAV_HEADERS;
			register struct NavHeader *h CTR_PSX_REGISTER("$2");
			register int coordinate CTR_PSX_REGISTER("$2");
			CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(slot, slot, frameOffset);
			CTR_PSX_LOAD_WORD(h, *slot);
			CTR_PSX_LOAD_SIGNED_HALF(coordinate, h, 0x4c, NAVHEADER_GETFRAME(h)->pos.x);
			original->posCurr.x = CTR_MipsSll(coordinate, 8);
			CTR_PSX_LOAD_WORD(h, *slot);
			CTR_PSX_LOAD_SIGNED_HALF(coordinate, h, 0x4e, NAVHEADER_GETFRAME(h)->pos.y);
			original->posCurr.y = CTR_MipsSll(coordinate, 8);
			CTR_PSX_LOAD_WORD(h, *slot);
			CTR_PSX_LOAD_SIGNED_HALF(coordinate, h, 0x50, NAVHEADER_GETFRAME(h)->pos.z);
			original->posCurr.z = CTR_MipsSll(coordinate, 8);
		}

		CTR_PSX_KEEP_VALUE(signedPath);
		LIST_AddFront(&BOTS_NAV_LIST[signedPath], &d->botData.item);
	}
	BOTS_SetRotation(d, 0);

	GAMEPAD_JogCon2(original, 0, 0);

	oldActionFlagsSet = original->actionsFlagSet;

	oldActionFlagsSet |= ACTION_BOT;
	CTR_PSX_OBSERVE_VALUE(oldActionFlagsSet);
	oldActionFlagsSet &= ~(ACTION_JUMP_BUTTON_HELD | ACTION_ACCEL_PREVENTION);
	original->actionsFlagSet = oldActionFlagsSet;

	if ((oldActionFlagsSet & ACTION_RACE_FINISHED) != 0)
	{
		CAM_EndOfRace(&GAME_TRACKER->cameraDC[original->driverID], original);
	}


	{
		register struct Driver *victim CTR_PSX_REGISTER("$4");
		switch (original->kartState)
		{
		case 0:
		case 1:
		case 2:
		case 4:
		case 5:
		default:
			return;
		case KS_SPINNING:
			victim = d;
			CTR_PSX_KEEP_VALUE_RELAXED(victim);
			damageType = 1;
			break;
		case KS_BLASTED:
			victim = d;
			CTR_PSX_KEEP_VALUE_RELAXED(victim);
			damageType = 2;
			break;
		}
		BOTS_ChangeState(victim, damageType, NULL, 0);
	}
}
