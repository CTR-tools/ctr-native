#include <shared/ctr_test.h>
#include <common.h>
#include <chaos/chaos_save.h>
#include <adventure_difficulty_save.h>

struct sData sdata_static;

static void TestAdventureDifficultyEncoding(void)
{
	CTR_TEST_SECTION("Adventure difficulty card encoding");
	CTR_TEST_CHECK_EQ(DIFFICULTY_NORMAL, 0, "Normal keeps its saved value");
	CTR_TEST_CHECK_EQ(DIFFICULTY_EASY, 1, "Easy keeps its saved value");
	CTR_TEST_CHECK_EQ(DIFFICULTY_HARD, 2, "Hard keeps its saved value");
	CTR_TEST_CHECK_EQ(DIFFICULTY_SUPER_HARD, 3, "Super Hard keeps its saved value");
	CTR_TEST_CHECK_EQ(DIFFICULTY_ULTRA_HARD, 4, "Ultra Hard keeps its saved value");
	u8 saved = 0;
	CTR_TEST_CHECK_EQ(AdvDifficultySave_Decode(saved), DIFFICULTY_NORMAL, "old zeroed slots load as Normal");
	for (int difficulty = 0; difficulty < DIFFICULTY_COUNT; difficulty++)
	{
		saved = AdvDifficultySave_Encode((enum DifficultyLevel)difficulty);
		CTR_TEST_CHECK_EQ((int)AdvDifficultySave_Decode(saved), difficulty, "each difficulty round-trips");
	}
	saved = DIFFICULTY_COUNT;
	CTR_TEST_CHECK_EQ(AdvDifficultySave_Decode(saved), DIFFICULTY_NORMAL, "unknown difficulty loads as Normal");
	saved = AdvDifficultySave_Encode((enum DifficultyLevel)-1);
	CTR_TEST_CHECK_EQ(AdvDifficultySave_Decode(saved), DIFFICULTY_NORMAL, "invalid live values save as Normal");
}

static void TestBitsetWireFormat(void)
{
	CTR_TEST_SECTION("a fixed V1 byte fixture round-trips with stable effect IDs");
	const u8 bytes[24] = {
		35, 0, 65, 0, 2, 1, 0x89, 0x81,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0x43, 1,
	};
	struct ChaosSettingsSaved saved;
	memcpy(&saved, bytes, sizeof(saved));
	struct ChaosSettings live;
	memset(&live, 0xff, sizeof(live));
	ChaosSave_Decode(&saved, &live);
	CTR_TEST_CHECK(ChaosSave_IsArmed(&saved) && live.armed, "the V1 armed flag is independent of the bitset");
	CTR_TEST_CHECK_EQ(live.durationSeconds, 35, "duration decodes from its original halfword");
	CTR_TEST_CHECK_EQ(live.triggerIntervalSeconds, 65, "interval decodes from its original halfword");
	CTR_TEST_CHECK_EQ(live.trigger, CHAOS_TRIGGER_INTERVAL, "trigger IDs retain their saved meaning");
	CTR_TEST_CHECK_EQ(live.distribution, CHAOS_DISTRIBUTION_SHARED, "legacy Adventure files default to shared distribution");
	for (int player = 0; player < CHAOS_MAX_HUMAN_PLAYERS; player++)
	{
		CTR_TEST_CHECK(!live.disabledPlayers[player], "legacy Adventure files enable every player");
	}
	CTR_TEST_CHECK_EQ(live.enabledCount, 5, "the enabled count is reconstructed rather than stored");
	for (int id = 0; id < CHAOS_EFFECT_COUNT; id++)
	{
		const bool expected = id == CHAOS_EFFECT_MIRROR_MIRROR || id == CHAOS_EFFECT_FLY_ME_TO_THE_MOON ||
		                      id == CHAOS_EFFECT_HOLE_IN_THE_TANK || id == CHAOS_EFFECT_FAIR_MODE ||
		                              id == CHAOS_EFFECT_BIG_KART_ENERGY;
		CTR_TEST_CHECK(live.effects[id] == expected, "each saved bit maps to its stable named effect, including across byte boundaries");
	}
	live.enabledCount = 99;
	memset(&saved, 0xff, sizeof(saved));
	ChaosSave_Encode(&live, &saved);
	CTR_TEST_CHECK(memcmp(&saved, bytes, sizeof(saved)) == 0, "default player options preserve the V1 byte fixture without persisting the cached count");
}

static void TestPlayerOptionsEncoding(void)
{
	CTR_TEST_SECTION("V2 persists distribution and boolean player options without moving save slots");
	const bool selections[][CHAOS_MAX_HUMAN_PLAYERS] = {
		{false, false, false, false}, {true, false, false, false},
		{false, true, false, false}, {false, false, true, false},
		{false, false, false, true}, {true, false, true, false},
		{true, true, true, true},
	};
	for (int distribution = 0; distribution < CHAOS_DISTRIBUTION_COUNT; distribution++)
	{
		for (int selection = 0; selection < (int)len(selections); selection++)
		{
			struct ChaosSettings original = {
				.armed = true, .durationSeconds = 35, .triggerIntervalSeconds = 65,
				.trigger = CHAOS_TRIGGER_INTERVAL, .distribution = distribution,
			};
			memcpy(original.disabledPlayers, selections[selection], sizeof(original.disabledPlayers));
			for (int effect = 0; effect < CHAOS_EFFECT_COUNT; effect++)
			{
				original.effects[effect] = effect % 2 == 0;
			}
			original.enabledCount = ChaosEffect_CountEnabled(original.effects);
			struct ChaosSettingsSaved saved;
			memset(&saved, 0xff, sizeof(saved));
			ChaosSave_Encode(&original, &saved);
			const bool legacy = distribution == CHAOS_DISTRIBUTION_SHARED && selection == 0;
			CTR_TEST_CHECK_EQ(saved.format, legacy ? CHAOS_SAVE_FORMAT_V1 : CHAOS_SAVE_FORMAT_V2, "only nondefault player options require V2");
			CTR_TEST_CHECK(ChaosSave_IsArmed(&saved), "profile-slot detection recognizes armed V1 and V2 entries");
			struct ChaosSettings restored;
			memset(&restored, 0xff, sizeof(restored));
			ChaosSave_Decode(&saved, &restored);
			CTR_TEST_CHECK_EQ(restored.distribution, distribution, "the selected distribution survives saving and loading");
			CTR_TEST_CHECK(memcmp(restored.disabledPlayers, original.disabledPlayers, sizeof(original.disabledPlayers)) == 0, "all four player toggles survive saving and loading");
			CTR_TEST_CHECK(memcmp(restored.effects, original.effects, sizeof(original.effects)) == 0, "player metadata does not overwrite effect bits");
			CTR_TEST_CHECK_EQ(restored.enabledCount, original.enabledCount, "saved player metadata never becomes enabled effects");
			CTR_TEST_CHECK(restored.durationSeconds == 35 && restored.triggerIntervalSeconds == 65 && restored.trigger == CHAOS_TRIGGER_INTERVAL, "V2 preserves the original timing fields");
			for (int bit = CHAOS_EFFECT_COUNT; bit < CHAOS_SAVE_V2_EFFECT_CAPACITY; bit++)
			{
				CTR_TEST_CHECK((saved.v2.effectBits[bit / 8] & (1u << (bit % 8))) == 0, "unused V2 effect bits remain clear");
			}
		}
	}
	const u8 bytes[24] = {
		35, 0, 65, 0, 2, 1, 1, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 1, 1, 0, 1, 0, 0x43, 2,
	};
	struct ChaosSettingsSaved saved;
	memcpy(&saved, bytes, sizeof(saved));
	struct ChaosSettings restored;
	ChaosSave_Decode(&saved, &restored);
	CTR_TEST_CHECK(restored.armed && restored.distribution == CHAOS_DISTRIBUTION_PER_PLAYER &&
	               restored.disabledPlayers[0] && !restored.disabledPlayers[1] && restored.disabledPlayers[2] &&
	               !restored.disabledPlayers[3], "the fixed V2 fixture has stable byte-sized player fields");
	ChaosSave_Encode(&restored, &saved);
	CTR_TEST_CHECK(memcmp(&saved, bytes, sizeof(saved)) == 0, "V2 round-trips the exact 24-byte wire fixture");
	saved.v2.distribution = 0xff;
	saved.v2.disabledPlayers[0] = 0xff;
	ChaosSave_Decode(&saved, &restored);
	CTR_TEST_CHECK(restored.distribution == CHAOS_DISTRIBUTION_SHARED && restored.disabledPlayers[0], "invalid distributions fall back to Shared and stored toggles normalize to booleans");
	saved.format = CHAOS_SAVE_FORMAT_V1;
	ChaosSave_Decode(&saved, &restored);
	CTR_TEST_CHECK(restored.distribution == CHAOS_DISTRIBUTION_SHARED && !restored.disabledPlayers[0], "V1 ignores bytes that only V2 assigns to player options");
	ChaosSave_Encode(&restored, &saved);
	CTR_TEST_CHECK(saved.format == CHAOS_SAVE_FORMAT_V1 && saved.v2.distribution == 0 && saved.v2.disabledPlayers[0] == 0, "writing default options clears metadata from a previous V2 slot");
}

static void TestUnusedBitsAndZeroEffects(void)
{
	CTR_TEST_SECTION("unused bits are zero even when overwriting a dirty slot");
	struct ChaosSettings live = {.armed = true};
	for (int id = 0; id < CHAOS_EFFECT_COUNT; id++)
	{
		live.effects[id] = true;
	}
	struct ChaosSettingsSaved saved;
	memset(&saved, 0xff, sizeof(saved));
	ChaosSave_Encode(&live, &saved);
	for (int bit = 0; bit < CHAOS_SAVE_EFFECT_CAPACITY; bit++)
	{
		const bool enabled = (saved.effectBits[bit / 8] & (1u << (bit % 8))) != 0;
		CTR_TEST_CHECK(enabled == (bit < CHAOS_EFFECT_COUNT), "only implemented effects can have their saved bits set");
	}

	// Model a slot with unknown bits set: they cannot leak into today's live
	// selection, and the next encode must restore the zero-unused-bits contract.
	memset(saved.effectBits, 0xff, sizeof(saved.effectBits));
	ChaosSave_Decode(&saved, &live);
	CTR_TEST_CHECK_EQ(live.enabledCount, CHAOS_EFFECT_COUNT, "unknown bits do not inflate the live effect count");
	memset(live.effects, 0, sizeof(live.effects));
	ChaosSave_Encode(&live, &saved);
	for (int i = 0; i < CHAOS_SAVE_EFFECT_BYTES; i++)
	{
		CTR_TEST_CHECK_EQ(saved.effectBits[i], 0, "a Chaos file can store no selected effects");
	}
	ChaosSave_Decode(&saved, &live);
	CTR_TEST_CHECK(ChaosSave_IsArmed(&saved) && live.armed, "Chaos with zero effects is still Chaos");
	CTR_TEST_CHECK_EQ(live.enabledCount, 0, "the decoded empty bitset has no enabled effects");
}

static void TestEmptyAndUnsupportedEntries(void)
{
	CTR_TEST_SECTION("empty or unsupported entries cannot inherit or arm Chaos settings");
	struct ChaosSettingsSaved saved = {0};
	struct ChaosSettings live;
	memset(&live, 0xff, sizeof(live));
	ChaosSave_Decode(&saved, &live);
	CTR_TEST_CHECK(!ChaosSave_IsArmed(&saved) && !live.armed, "a zeroed retail extension is Regular");
	CTR_TEST_CHECK_EQ(live.enabledCount, 0, "a zeroed extension cannot enable any effects");
	memset(&saved, 0xff, sizeof(saved));
	saved.format = 0x0343;
	ChaosSave_Decode(&saved, &live);
	CTR_TEST_CHECK(!ChaosSave_IsArmed(&saved) && !live.armed, "an unsupported format cannot arm a file");
	CTR_TEST_CHECK_EQ(live.durationSeconds, 0, "an unsupported format discards its duration");
	CTR_TEST_CHECK_EQ(live.triggerIntervalSeconds, 0, "an unsupported format discards its interval");
	CTR_TEST_CHECK_EQ(live.enabledCount, 0, "an unsupported format has no selected effects");
	for (int id = 0; id < CHAOS_EFFECT_COUNT; id++)
	{
		CTR_TEST_CHECK(!live.effects[id], "an unsupported entry clears every live flag");
	}
	saved.format = CHAOS_SAVE_FORMAT_V1;
	saved.trigger = 0xff;
	ChaosSave_Decode(&saved, &live);
	CTR_TEST_CHECK_EQ(live.trigger, CHAOS_TRIGGER_RANDOM, "an invalid trigger retains the existing Random fallback");
}

static void TestSlotIsolation(void)
{
	CTR_TEST_SECTION("saving one slot leaves the retail profile, co-op entries and other slots intact");
	static struct MemcardData card;
	static struct MemcardData before;
	memset(&card, 0xa5, sizeof(card));
	for (int slot = 0; slot < MEMCARD_ADV_PROFILE_COUNT; slot++)
	{
		struct ChaosSettings live = {.armed = true, .durationSeconds = (u16)(10 + slot), .distribution = slot % CHAOS_DISTRIBUTION_COUNT};
		live.disabledPlayers[slot] = true;
		live.effects[CHAOS_EFFECT_MIRROR_MIRROR] = true;
		ChaosSave_Encode(&live, &card.extension.chaos[slot]);
	}
	for (int slot = 0; slot < MEMCARD_ADV_PROFILE_COUNT; slot++)
	{
		memcpy(&before, &card, sizeof(card));
		struct ChaosSettings live;
		ChaosSave_Decode(&card.extension.chaos[slot], &live);
		live.durationSeconds += 5;
		ChaosSave_Encode(&live, &card.extension.chaos[slot]);
		CTR_TEST_CHECK(memcmp(&card.profile, &before.profile, sizeof(card.profile)) == 0, "the retail profile stays byte-identical");
		CTR_TEST_CHECK(memcmp(card.extension.adventure, before.extension.adventure, sizeof(card.extension.adventure)) == 0,
		               "co-op settings stay byte-identical");
		for (int other = 0; other < MEMCARD_ADV_PROFILE_COUNT; other++)
		{
			if (other != slot)
			{
				CTR_TEST_CHECK(memcmp(&card.extension.chaos[other], &before.extension.chaos[other], sizeof(struct ChaosSettingsSaved)) == 0,
				               "every other slot retains its original bytes");
			}
			CTR_TEST_CHECK(ChaosSave_IsArmed(&card.extension.chaos[other]), "every slot retains its Chaos marker");
		}
	}
}

int main(void)
{
	TestAdventureDifficultyEncoding();
	TestBitsetWireFormat();
	TestPlayerOptionsEncoding();
	TestUnusedBitsAndZeroEffects();
	TestEmptyAndUnsupportedEntries();
	TestSlotIsolation();
	return CtrTest_Summary();
}
