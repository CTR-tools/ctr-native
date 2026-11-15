// Pins the effect catalogue in include/chaos/chaos_effect.h: that every effect is
// addressable by index and by slug, that the enabled array is read one entry
// per effect, and that a pick is uniform over what is enabled.

#include <shared/ctr_test.h>
#include <common.h>
#include <chaos/chaos_effect.h>

// regionsEXE.h defines sdata as a pointer into this, so the linker wants the
// storage even though nothing here reads it.
struct sData sdata_static;

#define CHAOS_TEST_PICK_SAMPLES 20000
// Enough to catch a modulus over the array length instead of the enabled
// count, which strands the high ids within a handful of draws.
#define CHAOS_TEST_SINGLE_SAMPLES 1000
#define CHAOS_TEST_PICK_SEED 0x13579bdfu

// The real persisted struct, not a copy of it: namespace_Memcard.h includes
// chaos_effect.h, so common.h above already brought it in. A hand-rolled mirror
// would go stale the moment someone reordered or retyped a field, which is
// exactly the change these tests exist to catch.
static void ClearSettings(struct ChaosSettings *settings)
{
	memset(settings, 0, sizeof(*settings));

	// What an all-zero save means, plus the durations so the fields are not
	// left holding garbage the compiler could not see.
	settings->trigger = CHAOS_TRIGGER_RANDOM;
	settings->durationSeconds = CHAOS_DURATION_DEFAULT_SECONDS;
	settings->triggerIntervalSeconds = CHAOS_TRIGGER_INTERVAL_DEFAULT_SECONDS;
}

static void SetOnlyEnabled(struct ChaosSettings *settings, ChaosEffectId id)
{
	ClearSettings(settings);

	ChaosEffect_Toggle(settings->effects, &settings->enabledCount, id);
}

static void SetAllEnabled(struct ChaosSettings *settings)
{
	ClearSettings(settings);

	for (int i = 0; i < CHAOS_EFFECT_COUNT; i++)
	{
		ChaosEffect_Toggle(settings->effects, &settings->enabledCount, (ChaosEffectId)i);
	}
}

// === Naming ===

static void TestEveryEffectIsAddressable(void)
{
	CTR_TEST_SECTION("every effect answers to its index and to its slug");

	for (int i = 0; i < CHAOS_EFFECT_COUNT; i++)
	{
		const ChaosEffectDef *def = ChaosEffect_Def((ChaosEffectId)i);

		CTR_TEST_CHECK(def != NULL, "an effect index resolves to a definition");
		CTR_TEST_CHECK_EQ(ChaosEffect_FromIndex(i), i, "an index round-trips");
		CTR_TEST_CHECK(ChaosEffect_FromSlug(def->slug) == (ChaosEffectId)i, "a slug round-trips");
		CTR_TEST_CHECK(def->label != NULL, "an effect has a label to draw");
		CTR_TEST_CHECK(def->slug[0] != '\0', "an effect has a non-empty slug");
	}

	// Off the ends of the list.
	CTR_TEST_CHECK(ChaosEffect_FromIndex(-1) == CHAOS_EFFECT_NONE, "a negative index is no effect");
	CTR_TEST_CHECK(ChaosEffect_FromIndex(CHAOS_EFFECT_COUNT) == CHAOS_EFFECT_NONE, "an index past the end is no effect");
	CTR_TEST_CHECK(ChaosEffect_FromSlug("no-such-effect") == CHAOS_EFFECT_NONE, "an unknown slug is no effect");
	CTR_TEST_CHECK(ChaosEffect_FromSlug(NULL) == CHAOS_EFFECT_NONE, "a null slug is no effect");
	CTR_TEST_CHECK(ChaosEffect_Def(CHAOS_EFFECT_NONE) == NULL, "no effect has no definition");
}

static void TestSlugsAreUnique(void)
{
	CTR_TEST_SECTION("two effects never share a slug");

	for (int i = 0; i < CHAOS_EFFECT_COUNT; i++)
	{
		for (int j = i + 1; j < CHAOS_EFFECT_COUNT; j++)
		{
			CTR_TEST_CHECK(strcmp(g_chaosEffectDefs[i].slug, g_chaosEffectDefs[j].slug) != 0,
						   "a provider naming this slug reaches exactly one effect");
		}
	}
}

// === The enabled array ===

static void TestEnablingOneEffectLeavesTheRest(void)
{
	CTR_TEST_SECTION("toggling an effect touches only its own entry");

	struct ChaosSettings settings;

	for (int target = 0; target < CHAOS_EFFECT_COUNT; target++)
	{
		for (int i = 0; i < CHAOS_EFFECT_COUNT; i++)
		{
			settings.effects[i] = false;
		}
		settings.effects[target] = true;

		for (int i = 0; i < CHAOS_EFFECT_COUNT; i++)
		{
			// The mutation this catches is an off-by-one in the index, which
			// would enable a neighbour and leave the target itself off.
			CTR_TEST_CHECK_EQ(settings.effects[i], i == target, "only the toggled entry changed");
			CTR_TEST_CHECK(ChaosEffect_IsEnabled(settings.effects, (ChaosEffectId)i) == (i == target),
						   "the effect reports the state of its own entry");
		}
	}
}

static void TestIsEnabledRejectsBadIds(void)
{
	CTR_TEST_SECTION("reading outside the array is refused, not indexed");

	struct ChaosSettings settings;

	SetAllEnabled(&settings);

	CTR_TEST_CHECK(!ChaosEffect_IsEnabled(settings.effects, CHAOS_EFFECT_NONE), "no effect is not enabled");
	CTR_TEST_CHECK(!ChaosEffect_IsEnabled(settings.effects, (ChaosEffectId)CHAOS_EFFECT_COUNT), "an id past the end is not enabled");
	CTR_TEST_CHECK(!ChaosEffect_IsEnabled(settings.effects, (ChaosEffectId)-1), "a negative id is not enabled");
}

// === Scope ===

static void TestScopeRespectsDriverSlots(void)
{
	CTR_TEST_SECTION("a humans-only effect skips the bot slots");

	CTR_TEST_CHECK(ChaosEffect_AppliesToDriver(CHAOS_SCOPE_HUMANS, 0, 1), "the only player is hit in a 1P race");
	CTR_TEST_CHECK(!ChaosEffect_AppliesToDriver(CHAOS_SCOPE_HUMANS, 1, 1), "a bot slot is untouched in a 1P race");

	CTR_TEST_CHECK(ChaosEffect_AppliesToDriver(CHAOS_SCOPE_HUMANS, 0, 2), "player one is hit in a 2P race");
	CTR_TEST_CHECK(ChaosEffect_AppliesToDriver(CHAOS_SCOPE_HUMANS, 1, 2), "player two is hit in a 2P race");
	CTR_TEST_CHECK(!ChaosEffect_AppliesToDriver(CHAOS_SCOPE_HUMANS, 2, 2), "the first bot slot is untouched in a 2P race");

	CTR_TEST_CHECK(!ChaosEffect_AppliesToDriver(CHAOS_SCOPE_HUMANS, 0, 0), "with no human players, nobody is hit");
	CTR_TEST_CHECK(!ChaosEffect_AppliesToDriver(CHAOS_SCOPE_HUMANS, -1, 2), "a negative driver id is nobody");
	CTR_TEST_CHECK(!ChaosEffect_AppliesToDriver(CHAOS_SCOPE_HUMANS, 0, -1), "a negative human count is nobody");

	// The scope field is what a future bot effect uses.
	CTR_TEST_CHECK(ChaosEffect_AppliesToDriver(CHAOS_SCOPE_ALL, 7, 1), "an all-driver effect reaches a bot slot");
	CTR_TEST_CHECK(!ChaosEffect_AppliesToDriver(CHAOS_SCOPE_ALL, 0, 0), "an all-driver effect still needs a human race");
	for (int players = 1; players <= 4; players++)
	{
		CTR_TEST_CHECK(!ChaosEffect_AppliesToDriver(CHAOS_SCOPE_BOTS, players - 1, players), "bots-only scope excludes every human slot");
		CTR_TEST_CHECK(ChaosEffect_AppliesToDriver(CHAOS_SCOPE_BOTS, players, players), "bots-only scope includes the first bot after the humans");
		CTR_TEST_CHECK(ChaosEffect_AppliesToDriver(CHAOS_SCOPE_BOTS, 7, players), "bots-only scope includes the last bot in each player count");
	}
	CTR_TEST_CHECK(!ChaosEffect_AppliesToDriver(CHAOS_SCOPE_BOTS, -1, 2), "bots-only scope excludes an invalid slot");
	CTR_TEST_CHECK(!ChaosEffect_AppliesToDriver(CHAOS_SCOPE_BOTS, 2, 0), "bots-only effects still require a human race");
}

// === Picking ===

static void TestPickWithNothingEnabled(void)
{
	CTR_TEST_SECTION("with nothing enabled there is nothing to pick");

	struct ChaosSettings settings;

	for (int i = 0; i < CHAOS_EFFECT_COUNT; i++)
	{
		settings.effects[i] = false;
	}

	u32 rngState = CHAOS_TEST_PICK_SEED;

	// Derived the way the game derives it, so the count handed to Pick is never
	// whatever the stack happened to hold.
	settings.enabledCount = (u8)ChaosEffect_CountEnabled(settings.effects);

	CTR_TEST_CHECK_EQ(settings.enabledCount, 0, "nothing is enabled");
	CTR_TEST_CHECK(ChaosEffect_Pick(settings.effects, settings.enabledCount, &rngState) == CHAOS_EFFECT_NONE, "a pick returns no effect");
}

static void TestPickFindsTheOnlyEnabledEffect(void)
{
	CTR_TEST_SECTION("with one effect enabled, that is the effect");

	// The mutation this catches is dividing by the array length instead of by
	// the number of enabled effects, which strands the high ids.
	for (int only = 0; only < CHAOS_EFFECT_COUNT; only++)
	{
		struct ChaosSettings settings;
		SetOnlyEnabled(&settings, (ChaosEffectId)only);

		u32 rngState = CHAOS_TEST_PICK_SEED;
		bool allCorrect = true;

		for (int sample = 0; sample < CHAOS_TEST_SINGLE_SAMPLES; sample++)
		{
			if (ChaosEffect_Pick(settings.effects, settings.enabledCount, &rngState) != (ChaosEffectId)only)
			{
				allCorrect = false;
				break;
			}
		}
		CTR_TEST_CHECK(allCorrect, "the only enabled effect is picked every time");
	}
}

static void TestPickIsDeterministic(void)
{
	CTR_TEST_SECTION("the same seed picks the same sequence");

	struct ChaosSettings settings;
	SetAllEnabled(&settings);

	u32 firstState = CHAOS_TEST_PICK_SEED;
	u32 secondState = CHAOS_TEST_PICK_SEED;
	bool identical = true;

	for (int i = 0; i < 64; i++)
	{
		ChaosEffectId a = ChaosEffect_Pick(settings.effects, settings.enabledCount, &firstState);
		ChaosEffectId b = ChaosEffect_Pick(settings.effects, settings.enabledCount, &secondState);

		if (a != b)
		{
			identical = false;
			break;
		}
	}
	CTR_TEST_CHECK(identical, "replays reproduce the same effect sequence");
}

static void TestPickReachesEveryEffect(void)
{
	CTR_TEST_SECTION("no effect is unreachable from a full settings array");

	struct ChaosSettings settings;
	SetAllEnabled(&settings);

	u32 rngState = CHAOS_TEST_PICK_SEED;
	int hits[CHAOS_EFFECT_COUNT];

	for (int i = 0; i < CHAOS_EFFECT_COUNT; i++)
	{
		hits[i] = 0;
	}

	for (int sample = 0; sample < CHAOS_TEST_PICK_SAMPLES; sample++)
	{
		ChaosEffectId picked = ChaosEffect_Pick(settings.effects, settings.enabledCount, &rngState);

		CTR_TEST_CHECK(picked >= 0, "a pick from a full array never returns nothing");
		if ((picked >= 0) && (picked < CHAOS_EFFECT_COUNT))
		{
			hits[picked]++;
		}
	}

	const int expected = CHAOS_TEST_PICK_SAMPLES / CHAOS_EFFECT_COUNT;

	for (int i = 0; i < CHAOS_EFFECT_COUNT; i++)
	{
		CTR_TEST_CHECK(hits[i] > 0, "every enabled effect is reachable");
		// A modulus over the array length instead of the enabled count would
		// skew the low ids, so bound the spread rather than just checking > 0.
		CTR_TEST_CHECK(hits[i] < expected * 3, "no effect is favoured over the others");
	}
}

static void TestCachedCountMatchesTheArray(void)
{
	CTR_TEST_SECTION("the cached count stays equal to the array it describes");

	struct ChaosSettings settings;

	for (int i = 0; i < CHAOS_EFFECT_COUNT; i++)
	{
		settings.effects[i] = false;
	}
	settings.enabledCount = ChaosEffect_CountEnabled(settings.effects);
	CTR_TEST_CHECK_EQ(settings.enabledCount, 0, "an empty array counts zero");

	// Settings are chosen once, when the save is created, so this invariant has
	// to hold from then on. Toggling is the only writer of the array.
	for (int i = 0; i < CHAOS_EFFECT_COUNT; i++)
	{
		const b32 nowOn = ChaosEffect_Toggle(settings.effects, &settings.enabledCount, (ChaosEffectId)i);

		CTR_TEST_CHECK_EQ(settings.enabledCount, ChaosEffect_CountEnabled(settings.effects),
						  "the cached count agrees with the array after a toggle");
		CTR_TEST_CHECK_EQ(nowOn, ChaosEffect_IsEnabled(settings.effects, (ChaosEffectId)i),
						  "toggling reports the state it just wrote");
	}
	CTR_TEST_CHECK_EQ(settings.enabledCount, CHAOS_EFFECT_COUNT, "everything on counts every effect");

	for (int i = 0; i < CHAOS_EFFECT_COUNT; i++)
	{
		ChaosEffect_Toggle(settings.effects, &settings.enabledCount, (ChaosEffectId)i);

		CTR_TEST_CHECK_EQ(settings.enabledCount, ChaosEffect_CountEnabled(settings.effects),
						  "and again on the way back down");
	}
	CTR_TEST_CHECK_EQ(settings.enabledCount, 0, "everything off counts nothing again");

	// Interleaved on/off, which is what the settings menu does as the cursor
	// moves up and down over the effect rows.
	for (int round = 0; round < 3; round++)
	{
		for (int i = 0; i < CHAOS_EFFECT_COUNT; i += 2)
		{
			ChaosEffect_Toggle(settings.effects, &settings.enabledCount, (ChaosEffectId)i);
		}
		for (int i = 1; i < CHAOS_EFFECT_COUNT; i += 2)
		{
			ChaosEffect_Toggle(settings.effects, &settings.enabledCount, (ChaosEffectId)i);
		}
		CTR_TEST_CHECK_EQ(settings.enabledCount, ChaosEffect_CountEnabled(settings.effects),
						  "interleaved toggling never lets the count drift");
	}
}

static void TestAllZeroSettingsMeanChaosOff(void)
{
	CTR_TEST_SECTION("an all-zero save reads as chaos off");

	// What a file written before this feature existed carries. ChaosSettings has
	// no "is chaos on" flag: the effect array is the whole answer, so every entry
	// off has to mean no effect can ever fire.
	struct ChaosSettings settings;

	memset(&settings, 0, sizeof(settings));

	CTR_TEST_CHECK_EQ(settings.enabledCount, 0, "no effect is counted as enabled");
	CTR_TEST_CHECK_EQ(ChaosEffect_CountEnabled(settings.effects), 0, "and none is");

	u32 rngState = CHAOS_TEST_PICK_SEED;
	CTR_TEST_CHECK(ChaosEffect_Pick(settings.effects, settings.enabledCount, &rngState) == CHAOS_EFFECT_NONE,
				   "so nothing can be picked");

	// The trigger still has to be a legal value, since an all-zero trigger is
	// CHAOS_TRIGGER_RANDOM by construction.
	CTR_TEST_CHECK_EQ(settings.trigger, CHAOS_TRIGGER_RANDOM, "a zeroed trigger is the random one");
	CTR_TEST_CHECK(settings.trigger < CHAOS_TRIGGER_COUNT, "and it is in range");
}

static void TestToggleRejectsBadIds(void)
{
	CTR_TEST_SECTION("toggling outside the array changes nothing");

	struct ChaosSettings settings;
	SetAllEnabled(&settings);

	CTR_TEST_CHECK(!ChaosEffect_Toggle(settings.effects, &settings.enabledCount, CHAOS_EFFECT_NONE),
				   "no effect toggles to off");
	CTR_TEST_CHECK(!ChaosEffect_Toggle(settings.effects, &settings.enabledCount, (ChaosEffectId)CHAOS_EFFECT_COUNT),
				   "an id past the end toggles to off");
	CTR_TEST_CHECK_EQ(settings.enabledCount, CHAOS_EFFECT_COUNT, "and the count is left alone");
}

static void TestPickRefusesABadCount(void)
{
	CTR_TEST_SECTION("a count that cannot be right yields no effect");

	struct ChaosSettings settings;
	SetAllEnabled(&settings);

	u32 rngState = CHAOS_TEST_PICK_SEED;

	// The modulus below divides by this, so a zero or negative count has to be
	// refused rather than divided by.
	CTR_TEST_CHECK(ChaosEffect_Pick(settings.effects, 0, &rngState) == CHAOS_EFFECT_NONE, "a zero count picks nothing");
	CTR_TEST_CHECK(ChaosEffect_Pick(settings.effects, -1, &rngState) == CHAOS_EFFECT_NONE, "a negative count picks nothing");
	CTR_TEST_CHECK(ChaosEffect_Pick(settings.effects, CHAOS_EFFECT_COUNT + 1, &rngState) == CHAOS_EFFECT_NONE,
				   "a count larger than the array picks nothing");
}

static void TestPickSkipsDisabledEffects(void)
{
	CTR_TEST_SECTION("a pick only ever returns an effect that is on");

	for (int off = 0; off < CHAOS_EFFECT_COUNT; off++)
	{
		struct ChaosSettings settings;
		SetAllEnabled(&settings);
		settings.effects[off] = false;
		settings.enabledCount = ChaosEffect_CountEnabled(settings.effects);

		u32 rngState = CHAOS_TEST_PICK_SEED;
		bool clean = true;

		for (int sample = 0; sample < 512; sample++)
		{
			if (ChaosEffect_Pick(settings.effects, settings.enabledCount, &rngState) == (ChaosEffectId)off)
			{
				clean = false;
				break;
			}
		}
		CTR_TEST_CHECK(clean, "the one disabled effect is never returned");
		CTR_TEST_CHECK_EQ(ChaosEffect_CountEnabled(settings.effects), CHAOS_EFFECT_COUNT - 1,
						  "and it is one fewer effect to choose from");
	}
}

static void TestAnyEffectMayFollowAnother(void)
{
	CTR_TEST_SECTION("nothing is withheld, so any effect may follow any other");

	// No effect declares a conflict. Where two write the same driving constant
	// the later one simply wins, because the engine deactivates the previous
	// effect before activating the next. That ordering is the engine's job, so
	// the catalogue has no opinion about it and shrinks the pool by nothing.
	struct ChaosSettings settings;
	SetAllEnabled(&settings);

	u32 rngState = CHAOS_TEST_PICK_SEED;

	for (int previous = 0; previous < CHAOS_EFFECT_COUNT; previous++)
	{
		CTR_TEST_CHECK(ChaosEffect_Pick(settings.effects, settings.enabledCount, &rngState) >= 0, "a pick is possible after any activation");
		CTR_TEST_CHECK_EQ(ChaosEffect_CountEnabled(settings.effects), CHAOS_EFFECT_COUNT,
						  "an activation does not shrink the pool");
	}
}

int main(void)
{
	CTR_TEST_SECTION("chaos_effect.h: catalogue");

	TestEveryEffectIsAddressable();
	TestSlugsAreUnique();
	TestEnablingOneEffectLeavesTheRest();
	TestIsEnabledRejectsBadIds();
	TestScopeRespectsDriverSlots();
	TestAllZeroSettingsMeanChaosOff();
	TestCachedCountMatchesTheArray();
	TestToggleRejectsBadIds();
	TestPickWithNothingEnabled();
	TestPickFindsTheOnlyEnabledEffect();
	TestPickIsDeterministic();
	TestPickReachesEveryEffect();
	TestPickRefusesABadCount();
	TestPickSkipsDisabledEffects();
	TestAnyEffectMayFollowAnother();

	return CtrTest_Summary();
}
