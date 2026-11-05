#include <common.h>
#include <platform/native_track_library.h>
#include <platform/native_track_records.h>
#include <platform/native_log.h>
#include <stdlib.h>

struct CustomHighScoreNode
{
	struct CustomHighScoreNode *next;
	struct HighScoreTrack track;
	struct NativeTrackRecordKey key;
	int hasKey;
	char identity[];
};

static struct CustomHighScoreNode *s_customHighScores;
static struct NativeTrackRecordKey s_customRecordKey;
static int s_customRecordKeyValid;
static int s_customRecordLevel = -1;

void GAMEPROG_CustomBeginLevel(int levelID)
{
	const struct NativeTrackMetadata *track = NativeTrackRegistry_Get(NativeTrackLibrary_Current(), levelID);
	s_customRecordLevel = levelID;
	s_customRecordKeyValid = track != NULL && !g_config.oneLapRaces && NativeTrackRecords_Key(track, &s_customRecordKey);
}

struct HighScoreTrack *GAMEPROG_CustomHighScoreTrack(int levelID)
{
	const struct NativeTrackRegistry *registry = NativeTrackLibrary_Current();
	const struct NativeTrackMetadata *metadata = NativeTrackRegistry_Get(registry, levelID);
	if (metadata == NULL)
	{
		return NULL;
	}
	int hasKey = s_customRecordLevel == levelID && s_customRecordKeyValid;
	for (struct CustomHighScoreNode *node = s_customHighScores; node != NULL; node = node->next)
	{
		if (strcmp(node->identity, metadata->identity) == 0 && node->hasKey == hasKey &&
		    (!node->hasKey || memcmp(&node->key, &s_customRecordKey, sizeof(node->key)) == 0))
		{
			return &node->track;
		}
	}
	size_t identitySize = strlen(metadata->identity) + 1;
	struct CustomHighScoreNode *node = calloc(1, sizeof(*node) + identitySize);
	if (node == NULL)
	{
		abort();
	}
	memcpy(node->identity, metadata->identity, identitySize);
	node->hasKey = hasKey;
	if (node->hasKey)
	{
		node->key = s_customRecordKey;
	}
	for (int mode = 0; mode < MEMCARD_HIGH_SCORE_MODE_COUNT; mode++)
	{
		for (int entry = 0; entry < MEMCARD_HIGH_SCORE_ENTRIES_PER_MODE; entry++)
		{
			node->track.scoreEntry[mode][entry].time = MEMCARD_HIGH_SCORE_DEFAULT_TIME;
		}
	}
	if (node->hasKey)
	{
		struct HighScoreEntry saved[MEMCARD_HIGH_SCORE_ENTRIES_PER_MODE];
		if (NativeTrackRecords_LoadCurrent(NATIVE_TRACK_RECORD_SCORES, metadata, &node->key, saved, sizeof(saved)))
		{
			int valid = 1;
			for (int i = 0; i < MEMCARD_HIGH_SCORE_ENTRIES_PER_MODE; i++)
			{
				if (saved[i].characterID >= GAME_CHARACTER_COUNT || memchr(saved[i].name, '\0', sizeof(saved[i].name)) == NULL)
				{
					valid = 0;
					break;
				}
			}
			if (valid)
			{
				memcpy(node->track.scoreEntry[0], saved, sizeof(saved));
			}
		}
	}
	node->next = s_customHighScores;
	s_customHighScores = node;
	return &node->track;
}

void GAMEPROG_CustomSaveHighScore(int levelID)
{
	const struct NativeTrackMetadata *metadata = NativeTrackRegistry_Get(NativeTrackLibrary_Current(), levelID);
	if (metadata == NULL || s_customRecordLevel != levelID || !s_customRecordKeyValid)
	{
		return;
	}
	struct HighScoreTrack *track = GAMEPROG_CustomHighScoreTrack(levelID);
	if (!NativeTrackRecords_SaveCurrent(NATIVE_TRACK_RECORD_SCORES, metadata, &s_customRecordKey, track->scoreEntry[0], sizeof(track->scoreEntry[0])))
	{
		Platform_LogWarn("[CUSTOM TRACK] Could not save time trial records for %s\n", metadata->identity);
	}
}

/* Ghost data is untrusted on disk: bound packet reads and the 33-entry cache. */
static int GAMEPROG_CustomGhostValid(const struct GhostHeader *ghost)
{
	enum { CUSTOM_GHOST_MAX_BYTES = 0x3dd4, CUSTOM_GHOST_PACKET_CAPACITY = 33 };
	if (ghost == NULL || ghost->version != GHOST_TAPE_VERSION_RETAIL || ghost->size <= 0 ||
	    ghost->size > CUSTOM_GHOST_MAX_BYTES ||
	    ghost->characterID < 0 || ghost->characterID >= GAME_CHARACTER_COUNT || ghost->timeElapsedInRace <= 0)
	{
		return 0;
	}
	const unsigned char *bytes = (const unsigned char *)(ghost + 1);
	int pos = 0, entries = 0, positions = 0;
	while (pos < ghost->size)
	{
		int length;
		u8 op = bytes[pos];
		if (op == GHOST_OP_POSITION)
		{
			if (positions != 0 && entries >= CUSTOM_GHOST_PACKET_CAPACITY) { return 0; }
			length = GHOST_SIZE_POSITION;
			positions++;
			entries = positions > 1 ? 2 : 1;
		}
		else if (op == GHOST_OP_ANIMATION) { length = GHOST_SIZE_ANIMATION; }
		else if (op == GHOST_OP_BOOST) { length = GHOST_SIZE_BOOST; }
		else if (op == GHOST_OP_INSTANCE) { length = GHOST_SIZE_INSTANCE; }
		else if (op == GHOST_OP_IDLE) { length = GHOST_SIZE_IDLE; entries++; }
		else { length = GHOST_SIZE_VELOCITY; entries++; }
		if (positions == 0 && (op == GHOST_OP_IDLE || !GHOST_IS_OPCODE(op))) { return 0; }
		if (length > ghost->size - pos || entries > CUSTOM_GHOST_PACKET_CAPACITY) { return 0; }
		pos += length;
	}
	return positions >= 2;
}

int GAMEPROG_CustomLoadGhost(int levelID, struct GhostHeader *ghost)
{
	const struct NativeTrackMetadata *track = NativeTrackRegistry_Get(NativeTrackLibrary_Current(), levelID);
	struct NativeTrackRecordKey key;
	if (ghost == NULL || g_config.oneLapRaces || track == NULL || !NativeTrackRecords_Key(track, &key))
	{
		return 0;
	}
	if (!NativeTrackRecords_LoadCurrent(NATIVE_TRACK_RECORD_GHOST, track, &key, ghost, GHOST_RECORD_BUFFER_SIZE) ||
	    !GAMEPROG_CustomGhostValid(ghost))
	{
		memset(ghost, 0, GHOST_RECORD_BUFFER_SIZE);
		return 0;
	}
	/* The on-disk runtime ID may belong to a different track this session. */
	ghost->levelID = levelID;
	return 1;
}

int GAMEPROG_CustomSaveGhost(int levelID, const struct GhostHeader *ghost)
{
	const struct NativeTrackMetadata *track = NativeTrackRegistry_Get(NativeTrackLibrary_Current(), levelID);
	if (track == NULL || s_customRecordLevel != levelID || !s_customRecordKeyValid || !GAMEPROG_CustomGhostValid(ghost))
	{
		return 0;
	}
	unsigned char buffer[GHOST_RECORD_BUFFER_SIZE] = {0};
	memcpy(buffer, ghost, sizeof(*ghost) + ghost->size);
	if (!NativeTrackRecords_SaveCurrent(NATIVE_TRACK_RECORD_GHOST, track, &s_customRecordKey, buffer, sizeof(buffer)))
	{
		Platform_LogWarn("[CUSTOM TRACK] Could not save ghost for %s\n", track->identity);
	}
	return 1;
}

void GAMEPROG_AdvPercent(struct AdvProgress *adv)
{
	s32 goldBonus = 0;
	s16 numGems = 0;
	s16 numPurple = 0;
	s32 oxidePercent = 0;
	b16 allGoldOrPlatinumRelics = true;
	struct GameTracker *gGT;
	struct MetaDataLEV *mdLev;
	s32 i;

	// NOTE(aalhendi): Separate tracker lifetimes preserve retail's initialization,
	// reward-counting, and final-sum register allocation.
	{
		struct GameTracker *initialGT = GAME_TRACKER;
		initialGT->currAdvProfile.numRelics = 0;
		initialGT->currAdvProfile.numTrophies = 0;
		initialGT->currAdvProfile.numKeys = 0;
		initialGT->currAdvProfile.numCtrTokens.total = 0;
		for (i = 0; i < 5; i++)
		{
			initialGT->currAdvProfile.numCtrTokens.color[i] = 0;
		}
	}

	i = 0;
	gGT = GAME_TRACKER;
	mdLev = &data.metaDataLEV[0];
	// Count rewards in retail order, including the separate purple-token bonus.
	for (; i < ADV_REWARD_RELIC_TRACK_COUNT; i++)
	{
		if (CHECK_ADV_BIT(adv->rewards, ADV_REWARD_FIRST_SAPPHIRE_RELIC + i))
		{
			gGT->currAdvProfile.numRelics++;
		}

		if (i < ADV_REWARD_RACE_TRACK_COUNT)
		{
			if (CHECK_ADV_BIT(adv->rewards, ADV_REWARD_FIRST_TROPHY + i))
			{
				gGT->currAdvProfile.numTrophies++;
			}

			if (CHECK_ADV_BIT(adv->rewards, ADV_REWARD_FIRST_CTR_TOKEN + i))
			{
				// increment number of tokens, based on
				// the tokenID of this level (red, green, blue, etc)
				gGT->currAdvProfile.numCtrTokens.color[mdLev[i].ctrTokenGroupID]++;

				// increment number of total tokens
				gGT->currAdvProfile.numCtrTokens.total++;
			}
		}

		// Boss keys
		if (i < ADV_REWARD_HUB_COUNT)
		{
			if (CHECK_ADV_BIT(adv->rewards, ADV_REWARD_FIRST_BOSS_KEY + i))
			{
				gGT->currAdvProfile.numKeys++;
			}
		}

		// Gem cups
		if (i < ADV_REWARD_GEM_COUNT)
		{
			if (CHECK_ADV_BIT(adv->rewards, ADV_REWARD_FIRST_GEM + i))
			{
				numGems++;
			}
		}

		if (i < ADV_REWARD_HUB_COUNT)
		{
			if (CHECK_ADV_BIT(adv->rewards, ADV_REWARD_FIRST_PURPLE_TOKEN + i))
			{
				numPurple++;
				// Purple tokens also occupy the fifth color group.
				gGT->currAdvProfile.numCtrTokens.color[4]++;
			}
		}

		// The second Oxide win upgrades the bonus from 2% to 3%.
		if (i < ADV_REWARD_OXIDE_BEAT_COUNT)
		{
			if (CHECK_ADV_BIT(adv->rewards, ADV_REWARD_BEAT_OXIDE_FIRST + i))
			{
				if (i == 0)
				{
					oxidePercent = 2;
				}
				else if (i == 1)
				{
					oxidePercent = 3;
				}
			}
		}
	}

	// The final percent requires every track's gold-or-platinum reward bit.
	for (i = 0; i < ADV_REWARD_RELIC_TRACK_COUNT; i++)
	{
		allGoldOrPlatinumRelics = allGoldOrPlatinumRelics && CHECK_ADV_BIT(adv->rewards, ADV_REWARD_FIRST_GOLD_RELIC + i);
	}

	if (allGoldOrPlatinumRelics)
	{
		goldBonus = 1;
	}
	{
		struct GameTracker *completedGT = GAME_TRACKER;
		completedGT->currAdvProfile.completionPercent =
		    goldBonus + (oxidePercent + ((completedGT->currAdvProfile.numRelics + completedGT->currAdvProfile.numTrophies) * 2 +
		                                 completedGT->currAdvProfile.numKeys + completedGT->currAdvProfile.numCtrTokens.total + numPurple + numGems));
	}
}


void GAMEPROG_ResetHighScores(struct GameProgress *gameProg)
{
	struct GameProgress *progress = gameProg;
	s16 i;
	u32 modeIndex;
	s16 j;
	u8 *entryBase;
	s16 k;
	u32 lapOffset;
	struct HighScoreEntry *lap;

	// Initialize time-trial and relic tables: best lap, then five race times.
	for (i = 0; i < MEMCARD_HIGH_SCORE_TRACK_COUNT; i++)
	{
		s32 trackID = i;
		s32 trackOffset = trackID * (s32)sizeof(struct HighScoreTrack);
		struct HighScoreTrack *track = &progress->highScoreTracks[trackID];

		for (j = 0; j < MEMCARD_HIGH_SCORE_MODE_COUNT; j++)
		{
			// NOTE(aalhendi): Keep the table base in this scope so GCC hoists it
			// before considering the division constants for either score loop.
			struct MetaDataCHAR *characters = GAME_CHARACTER_METADATA;
			s32 mode = j;
			s32 seed;
			s32 characterID;
			register s32 nameSeed CTR_PSX_REGISTER("$6") = trackID + mode;

			// Keep the lap calculation separate from the seed carried across strcpy.
			characterID = nameSeed;
			characterID %= PENTA_PENGUIN;
			seed = nameSeed;
			strcpy(track->scoreEntry[mode][0].name, GAME_LANGUAGE_STRINGS[characters[characterID].name_LNG_short]);

			modeIndex = mode;
			lapOffset = modeIndex * MEMCARD_HIGH_SCORE_ENTRIES_PER_MODE * (s32)sizeof(struct HighScoreEntry) + trackOffset;
			lap = (struct HighScoreEntry *)((u8 *)progress + lapOffset + OFFSETOF(struct GameProgress, highScoreTracks));
			lap->characterID = characterID;
			lap->time = MEMCARD_HIGH_SCORE_DEFAULT_TIME;

			for (k = 0; k < MEMCARD_HIGH_SCORE_ENTRIES_PER_MODE - 1; k++)
			{
				s32 characterID = seed + k + 1;
				characterID %= PENTA_PENGUIN;
				CTR_PSX_DEPEND_VALUE(modeIndex, k);

				strcpy(track->scoreEntry[modeIndex][k + 1].name, GAME_LANGUAGE_STRINGS[characters[characterID].name_LNG_short]);
				entryBase = (u8 *)progress + (k * (s32)sizeof(struct HighScoreEntry) +
				                              modeIndex * MEMCARD_HIGH_SCORE_ENTRIES_PER_MODE * (s32)sizeof(struct HighScoreEntry) + trackOffset);
				// NOTE(aalhendi): Leave the profile-header bias in the field addresses
				// so GCC folds it into the stores instead of advancing entryBase.
				((struct HighScoreEntry *)(entryBase + OFFSETOF(struct GameProgress, highScoreTracks)))[1].characterID = characterID;
				((struct HighScoreEntry *)(entryBase + OFFSETOF(struct GameProgress, highScoreTracks)))[1].time = MEMCARD_HIGH_SCORE_DEFAULT_TIME;
			}
			// NOTE(aalhendi): Retain track values through both modes; the name seed,
			// not either track value, is the temporary spilled across name copies.
			CTR_PSX_ORDER_VALUES(trackOffset, trackID);
		}
	}
}


b32 GAMEPROG_CheckGhostsBeaten(s32 ghostID)
{
	register s32 ghostBit CTR_PSX_REGISTER("$19") = ghostID;
	b16 result = true;
	s16 i = 0;
	struct GameProgress *progress = &GAME_PROGRESS;
	s32 wordOffset;
	s16 levelID;

	// NOTE(aalhendi): Retail keeps the original bit index separate from the
	// signed-halfword word index, and reuses v0 for the per-track address.
	CTR_PSX_KEEP_VALUE_RELAXED(ghostBit);
	wordOffset = ((s16)ghostID >> 5) * (s32)sizeof(u32);
	levelID = GAME_TRACKER->levelID;

	for (; i < MEMCARD_HIGH_SCORE_TRACK_COUNT; i++)
	{
		register s32 trackOffset CTR_PSX_REGISTER("$2");
		b16 trackBeaten;

		GAME_TRACKER->levelID = i;
		GAMEPROG_GetPtrHighScoreTrack();

		trackBeaten = false;
		if (result)
		{
			trackOffset = GAME_TRACKER->levelID * (s32)sizeof(struct HighScoreTrack);
			trackOffset = wordOffset + trackOffset;
			trackBeaten = (CTR_ReadU32AlignedLE((u8 *)(trackOffset + (u32)progress) + OFFSETOF(struct GameProgress, highScoreTracks[0].timeTrialFlags)) >>
			               (ghostBit & 0x1f)) &
			              1;
		}
		result = trackBeaten;
	}

	GAME_TRACKER->levelID = levelID;
	GAMEPROG_GetPtrHighScoreTrack();

	return result;
}


void GAMEPROG_NewProfile_OutsideAdv(struct GameSave *save)
{
	memset(save, 0, sizeof(*save));
	GAMEPROG_ResetHighScores(&save->progress);
}


void GAMEPROG_InitFullMemcard(struct MemcardData *mcd)
{
	struct MemcardProfile *mcp = &mcd->profile;
	struct GameSave *save = &mcp->gameSave;
	s16 i;
	// clear, this port's extension included
	memset(mcd, 0, sizeof(struct MemcardData));
	GAMEPROG_NewProfile_OutsideAdv(save);
	i = 0;

	// header
	mcp->header[0] = MEMCARD_PROFILE_VERSION; // version (-18)
	mcp->header[1] = sizeof(struct MemcardProfile);

	// 4 profiles
	for (; i < MEMCARD_ADV_PROFILE_COUNT; i++)
	{
		// no character selected
		mcp->advProgress[i].characterID = -1;

		// N Sane Beach
		mcp->advProgress[i].HubLevYouSavedOn = N_SANITY_BEACH;
	}
}


void GAMEPROG_NewProfile_InsideAdv(struct AdvProgress *adv)
{
	// clear
	memset(adv, 0x0, sizeof(struct AdvProgress));

	// no character selected
	adv->characterID = -1;

	// N Sane Beach
	adv->HubLevYouSavedOn = N_SANITY_BEACH;
}


void GAMEPROG_SaveCupProgress(void)
{
	s16 i = 0;
	struct GameProgress *progress = &GAME_PROGRESS;

	// 4 cups, 3 difficulties
	for (; i < GAME_PROGRESS_CUP_WIN_COUNT; i++)
	{
		// if cup is "currently" beaten
		s32 bitIndex1 = i + GAME_PROGRESS_CUP_CURRENT_WIN_FIRST_BIT;
		// NOTE(aalhendi): Index-first addition preserves retail's address operands.
		if ((*(MEMCARD_BIT_WORD(bitIndex1) + progress->unlocks) >> (bitIndex1 & 0x1f)) & 1)
		{
			// set if cup was "previously" beaten
			s32 bitIndex2 = bitIndex1 + GAME_PROGRESS_CUP_PREVIOUS_WIN_OFFSET;
			*(MEMCARD_BIT_WORD(bitIndex2) + progress->unlocks) |= MEMCARD_BIT_MASK(bitIndex2);
		}
	}
}


void GAMEPROG_SyncGameAndCard(struct GameProgress *memcardProg, struct GameProgress *currentProg)
{
	u32 *memcardFlags = memcardProg->unlocks;
	u32 *currentFlags = currentProg->unlocks;
	s16 i, j;
	// combine progress of cups,
	// characters, track, scrapbook
	for (i = 0; i < GAME_PROGRESS_UNLOCK_WORD_COUNT; i++)
	{
		u32 joined = *currentFlags | *memcardFlags;
		*currentFlags++ = joined;
		*memcardFlags++ = joined;
	}

	// combine progress of beaten ghosts
	for (j = 0; j < MEMCARD_HIGH_SCORE_TRACK_COUNT; j++)
	{
		memcardFlags = &memcardProg->highScoreTracks[j].timeTrialFlags;
		currentFlags = &currentProg->highScoreTracks[j].timeTrialFlags;
		// NOTE(aalhendi): Retail retains a one-word loop for each track's flags.
		for (i = 0; i < 1; i++)
		{
			u32 joined = *currentFlags | *memcardFlags;
			*currentFlags++ = joined;
			*memcardFlags++ = joined;
		}
	}

	// Merging the reward bits does not re-evaluate dependent unlocks, such as
	// N Tropy when the two saves have complementary sets of beaten ghosts.
}


void GAMEPROG_NewGame_OnBoot(void)
{
	GAMEPROG_NewProfile_OutsideAdv(&GAME_SAVE);
	GAMEPROG_NewProfile_InsideAdv(&GAME_ADV_PROGRESS);
	GAMEPROG_ResetAdvMultiplayer(&sdata->advMultiplayer);
	sdata->advDifficulty = ADV_DIFFICULTY_NORMAL;
	GAMEPROG_ResetChaosSettings(&sdata->chaosSettings);
	Chaos_PublishEffects();
	GAMEPROG_GetPtrHighScoreTrack();
}


void GAMEPROG_ResetAdvMultiplayer(struct AdvMultiplayerSettings *settings)
{
	settings->numPlayers = ADV_MULTIPLAYER_DEFAULT_PLAYERS;
	settings->characterID2 = ADV_MULTIPLAYER_NO_CHARACTER;
}


// All effects are selected by default; the creation menu arms Chaos separately.
void GAMEPROG_ResetChaosSettings(struct ChaosSettings *settings)
{
	settings->durationSeconds = CHAOS_DURATION_DEFAULT_SECONDS;
	settings->triggerIntervalSeconds = CHAOS_TRIGGER_INTERVAL_DEFAULT_SECONDS;
	settings->trigger = CHAOS_TRIGGER_RANDOM;
	settings->armed = false;
	settings->enabledCount = CHAOS_EFFECT_COUNT;

	for (int i = 0; i < CHAOS_EFFECT_COUNT; i++)
	{
		settings->effects[i] = true;
	}
}


enum
{
	// Sapphire, gold and platinum, one target time each per track.
	GAMEPROG_RELIC_TIER_COUNT = 3,
};

// Adventure co-op shares one tier time between two racers on a single clock
// and a single crate pool, so the multiplayer penalty takes 30% off every target.
static const float GAMEPROG_RELIC_2P_TIME_FACTOR = 0.7f;

int GAMEPROG_AdventureLossIndex(int losses)
{
	enum { EASY_LOSS_BONUS = 2 };
	if ((GAME_TRACKER->gameMode1 & ADVENTURE_MODE) != 0 &&
	    sdata->advDifficulty == ADV_DIFFICULTY_EASY && losses > 0)
	{
		return losses + EASY_LOSS_BONUS;
	}
	return losses;
}

int GAMEPROG_AdventureBotDifficulty(int difficulty)
{
	enum
	{
		EASY_SCALE_NUMERATOR = 3,
		EASY_SCALE_DENOMINATOR = 4,
		EASY_MAX = 120,
		HARD_VALUE = 240,
		SUPER_HARD_VALUE = 320,
		ULTRA_HARD_VALUE = 640,
		FIXED_BOSS_BONUS = 20,
	};
	u32 mode = GAME_TRACKER->gameMode1;
	if ((mode & ADVENTURE_MODE) == 0 || (mode & ARCADE_MODE) != 0)
	{
		return difficulty;
	}
	int result = difficulty;
	switch (sdata->advDifficulty)
	{
	case ADV_DIFFICULTY_EASY:
		result = difficulty * EASY_SCALE_NUMERATOR / EASY_SCALE_DENOMINATOR;
		if (result > EASY_MAX)
		{
			result = EASY_MAX;
		}
		break;
	case ADV_DIFFICULTY_HARD: result = HARD_VALUE; break;
	case ADV_DIFFICULTY_SUPER_HARD: result = SUPER_HARD_VALUE; break;
	case ADV_DIFFICULTY_ULTRA_HARD: result = ULTRA_HARD_VALUE; break;
	case ADV_DIFFICULTY_NORMAL:
	case ADV_DIFFICULTY_COUNT:
	default: return difficulty;
	}
	if ((mode & ADVENTURE_BOSS) != 0 && sdata->advDifficulty >= ADV_DIFFICULTY_HARD)
	{
		result += FIXED_BOSS_BONUS;
	}
	return result;
}

int GAMEPROG_AdventureBossLosses(int losses)
{
	if ((GAME_TRACKER->gameMode1 & ADVENTURE_MODE) != 0 &&
	    sdata->advDifficulty >= ADV_DIFFICULTY_HARD && sdata->advDifficulty < ADV_DIFFICULTY_COUNT)
	{
		return 0;
	}
	return losses;
}

int GAMEPROG_AdventureTimeLimit(int time)
{
	if ((GAME_TRACKER->gameMode1 & ADVENTURE_MODE) == 0)
	{
		return time;
	}

	int percent;
	switch (sdata->advDifficulty)
	{
	case ADV_DIFFICULTY_EASY: percent = 120; break;
	case ADV_DIFFICULTY_HARD: percent = 90; break;
	case ADV_DIFFICULTY_SUPER_HARD: percent = 80; break;
	case ADV_DIFFICULTY_ULTRA_HARD: percent = 70; break;
	case ADV_DIFFICULTY_NORMAL:
	case ADV_DIFFICULTY_COUNT:
	default: return time;
	}
	return (int)((s64)time * percent / 100);
}

// The target time the pair has to beat. A lone racer keeps the retail target;
// this is the single place both the HUD goal and the award check read, so the two
// can never disagree about what the round demands.
int GAMEPROG_RelicTargetTime(int levelID, int tier)
{
	int soloTime = data.RelicTime[levelID * GAMEPROG_RELIC_TIER_COUNT + tier];

	if (MainGameEnd_IsCoopRace(GAME_TRACKER))
	{
		soloTime = (int)(soloTime * GAMEPROG_RELIC_2P_TIME_FACTOR);
	}
	return GAMEPROG_AdventureTimeLimit(soloTime);
}


void GAMEPROG_GetPtrHighScoreTrack(void)
{
	struct GameTracker *gGT = GAME_TRACKER;
	struct HighScoreTrack *customTrack = GAMEPROG_CustomHighScoreTrack(gGT->levelID);
	if (customTrack != NULL)
	{
		sdata->ptrActiveHighScoreEntry = customTrack->scoreEntry[(gGT->gameMode1 & RELIC_RACE) != 0];
		return;
	}
	s32 relicBits = gGT->gameMode1 & RELIC_RACE;
	s32 trackOffset = gGT->levelID * (s32)sizeof(struct HighScoreTrack);
	b32 relicMode = relicBits != 0;
	u8 *scores = (u8 *)&GAME_PROGRESS.highScoreTracks + relicMode * MEMCARD_HIGH_SCORE_ENTRIES_PER_MODE * (s32)sizeof(struct HighScoreEntry);
	// NOTE(aalhendi): Both targets use 32-bit addresses. Keeping the track offset
	// separate from the mode's base address preserves retail's addition order.
	u32 scoreAddress = trackOffset + (u32)scores;

	sdata->ptrActiveHighScoreEntry = (struct HighScoreEntry *)scoreAddress;
	// NOTE(aalhendi): Keep the store before the return, outside its delay slot.
	CTR_PSX_OBSERVE_MEMORY(sdata->ptrActiveHighScoreEntry);
}
