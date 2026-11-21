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
	CTR_TEST_CHECK_EQ(live.distribution, CHAOS_DISTRIBUTION_SHARED, "Adventure decoding clears any personal session variant");
	for (int player = 0; player < CHAOS_MAX_HUMAN_PLAYERS; player++)
	{
		CTR_TEST_CHECK(!live.disabledPlayers[player], "Adventure decoding clears every session-only player exclusion");
		live.disabledPlayers[player] = true;
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
	live.distribution = CHAOS_DISTRIBUTION_PER_PLAYER;
	memset(&saved, 0xff, sizeof(saved));
	ChaosSave_Encode(&live, &saved);
	CTR_TEST_CHECK(memcmp(&saved, bytes, sizeof(saved)) == 0, "encoding preserves the byte fixture without persisting session distribution, exclusions or the cached count");
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
	saved.format = 0x0243;
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
		struct ChaosSettings live = {.armed = true, .durationSeconds = (u16)(10 + slot)};
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
	TestUnusedBitsAndZeroEffects();
	TestEmptyAndUnsupportedEntries();
	TestSlotIsolation();
	return CtrTest_Summary();
}
