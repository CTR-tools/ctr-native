// Pins the race-field rules in include/race_field.h: how many racers each kind
// of race fields, and how many of those are bots.

#include <shared/ctr_test.h>
#include <common.h>
#include <race_field.h>

// regionsEXE.h defines sdata as a pointer into this, so the linker wants the
// storage even though nothing here reads it.
struct sData sdata_static;

// A race is Arcade or Adventure; every other mode only modifies one of the two,
// so a modifier bit on its own is not a race. Naming the combination once keeps
// every call site from having to remember that.
#define RACE_ADVENTURE_MODE(modifier) (ADVENTURE_MODE | (modifier))

#define RACE_GEM_CUP RACE_ADVENTURE_MODE(ADVENTURE_CUP)
#define RACE_BOSS    RACE_ADVENTURE_MODE(ADVENTURE_BOSS)
#define RACE_RELIC   RACE_ADVENTURE_MODE(RELIC_RACE)
#define RACE_CRYSTAL RACE_ADVENTURE_MODE(CRYSTAL_CHALLENGE)
#define RACE_CLOCK   RACE_ADVENTURE_MODE(TIME_TRIAL)
#define RACE_HUB     RACE_ADVENTURE_MODE(ADVENTURE_ARENA)

// === A normal race ===

static void TestNormalRaceGrid(void)
{
	CTR_TEST_SECTION("normal races: a lone racer fills eight, a pair shares six");

	// 1P always gets the full grid, option or not: that is retail's own roster.
	CTR_TEST_CHECK_EQ(RaceField_RacerCount(ARCADE_MODE, LOAD_CUP_ID_NONE, 1, 0), RACE_FIELD_FULL_GRID_RACERS,
	           "1P arcade fills all eight slots");
	CTR_TEST_CHECK_EQ(RaceField_RacerCount(ADVENTURE_MODE, LOAD_CUP_ID_NONE, 1, 0), RACE_FIELD_FULL_GRID_RACERS,
	           "1P adventure fills all eight slots");

	// A pair is the interesting one, and the extended option is the whole
	// difference between the short six-racer grid and a full one.
	CTR_TEST_CHECK_EQ(RaceField_RacerCount(ARCADE_MODE, LOAD_CUP_ID_NONE, 2, 0), RACE_FIELD_PAIR_GRID_RACERS,
	           "2P arcade shares six racers without the option");
	CTR_TEST_CHECK_EQ(RaceField_RacerCount(ADVENTURE_MODE, LOAD_CUP_ID_NONE, 2, 0), RACE_FIELD_PAIR_GRID_RACERS,
	           "2P adventure shares six racers without the option");
	CTR_TEST_CHECK_EQ(RaceField_RacerCount(ARCADE_MODE, LOAD_CUP_ID_NONE, 2, 1), RACE_FIELD_FULL_GRID_RACERS,
	           "2P arcade fills eight with the option");
	CTR_TEST_CHECK_EQ(RaceField_RacerCount(ADVENTURE_MODE, LOAD_CUP_ID_NONE, 2, 1), RACE_FIELD_FULL_GRID_RACERS,
	           "2P adventure fills eight with the option");
}

static void TestBotCountFollowsTheGrid(void)
{
	CTR_TEST_SECTION("bots: every grid leaves one bot per racer past the humans");

	for (int extended = 0; extended <= 1; extended++)
	{
		for (int playerCount = 1; playerCount <= 2; playerCount++)
		{
			const int racers = RaceField_RacerCount(ADVENTURE_MODE, LOAD_CUP_ID_NONE, playerCount, extended);
			const int bots = RaceField_BotCount(ADVENTURE_MODE, LOAD_CUP_ID_NONE, playerCount, extended);

			CTR_TEST_CHECK_EQ(bots, racers - playerCount, "bots are the racers past the players");
			CTR_TEST_CHECK(bots >= 0, "a race never fields fewer racers than it has players");
			CTR_TEST_CHECK(racers <= RACE_FIELD_FULL_GRID_RACERS, "a grid never exceeds the eight-racer maximum");
		}
	}
}

// === Player counts beyond a pair ===

static void TestHighPlayerCountsNeedTheOption(void)
{
	CTR_TEST_SECTION("player counts: battle goes to four, everything else needs the option");

	for (int extended = 0; extended <= 1; extended++)
	{
		for (int playerCount = 2; playerCount <= 4; playerCount++)
		{
			CTR_TEST_CHECK_EQ(RaceField_BotCount(BATTLE_MODE, LOAD_CUP_ID_NONE, playerCount, 0), 0,
					   "battle mode spawns no bots");
			CTR_TEST_CHECK_EQ(RaceField_RacerCount(BATTLE_MODE, LOAD_CUP_ID_NONE, playerCount, 0), playerCount,
					   "battle mode seats only its players");
		}
	}

	// So with the option, a third or fourth racer shares the same eight-racer grid
	// a lone racer fills, and the bots are whatever is left over.
	CTR_TEST_CHECK_EQ(RaceField_RacerCount(ARCADE_MODE, LOAD_CUP_ID_NONE, 3, 1), RACE_FIELD_FULL_GRID_RACERS,
	           "3P arcade fills eight with the option on");
	CTR_TEST_CHECK_EQ(RaceField_RacerCount(ARCADE_MODE, LOAD_CUP_ID_NONE, 4, 1), RACE_FIELD_FULL_GRID_RACERS,
	           "4P arcade fills eight with the option on");
	// Eight racers in total, so four players leave four bots rather than the
	// eight a player-count addition would give.
	CTR_TEST_CHECK_EQ(RaceField_BotCount(ARCADE_MODE, LOAD_CUP_ID_NONE, 4, 1), RACE_FIELD_FULL_GRID_RACERS - 4,
	           "4P arcade leaves four bots on the grid");
}

// === Cup races ===

static void TestGemCupRaces(void)
{
	CTR_TEST_SECTION("gem cups: only the Purple cup adds bosses");

	// A normal gem cup is an ordinary race as far as the count goes.
	CTR_TEST_CHECK_EQ(RaceField_RacerCount(RACE_GEM_CUP, 0, 1, 0), RACE_FIELD_FULL_GRID_RACERS, "1P turquoise cup fills eight");
	CTR_TEST_CHECK_EQ(RaceField_RacerCount(RACE_GEM_CUP, 1, 2, 0), RACE_FIELD_PAIR_GRID_RACERS, "2P blue cup shares six");
	CTR_TEST_CHECK_EQ(RaceField_RacerCount(RACE_GEM_CUP, 3, 2, 1), RACE_FIELD_FULL_GRID_RACERS, "2P yellow cup fills eight with the option");

	// The Purple cup always adds its four bosses, whoever is driving, so the
	// extended option does not get to widen it.
	CTR_TEST_CHECK_EQ(RaceField_RacerCount(RACE_GEM_CUP, CUP_ID_PURPLE_GEM, 1, 0), 1 + LOAD_2P_AI_SET_RACER_COUNT,
	           "1P purple cup adds four bosses");
	CTR_TEST_CHECK_EQ(RaceField_RacerCount(RACE_GEM_CUP, CUP_ID_PURPLE_GEM, 1, 1), 1 + LOAD_2P_AI_SET_RACER_COUNT,
	           "the option does not widen the purple cup");
	CTR_TEST_CHECK_EQ(RaceField_RacerCount(RACE_GEM_CUP, CUP_ID_PURPLE_GEM, 2, 0), 2 + LOAD_2P_AI_SET_RACER_COUNT,
	           "2P purple cup adds four bosses");
	CTR_TEST_CHECK_EQ(RaceField_BotCount(RACE_GEM_CUP, CUP_ID_PURPLE_GEM, 2, 0), LOAD_2P_AI_SET_RACER_COUNT,
	           "2P purple cup fields exactly its four bosses");

	CTR_TEST_CHECK(RaceField_UsesExtendedGrid(RACE_GEM_CUP, CUP_ID_PURPLE_GEM, 1) == 0,
	               "the purple cup keeps its roster off the extended grid");
}

// === Boss races ===

static void TestBossRaceFieldsTheBoss(void)
{
	CTR_TEST_SECTION("boss races: the player plus the boss, whatever the option");

	// IS_BOSS_RACE is a signed test on gameMode1's sign bit, so reading the mode
	// as unsigned is the mutation that makes every boss race look like a normal
	// one and silently drops the boss from the field.
	CTR_TEST_CHECK_EQ(RaceField_RacerCount(RACE_BOSS, LOAD_CUP_ID_NONE, 1, 0), 1 + RACE_FIELD_BOSS_RACERS,
	           "1P boss race is the player and the boss");
	CTR_TEST_CHECK_EQ(RaceField_RacerCount(RACE_BOSS, LOAD_CUP_ID_NONE, 1, 1), 1 + RACE_FIELD_BOSS_RACERS,
	           "the option cannot add a bot to a solo boss race");
	CTR_TEST_CHECK_EQ(RaceField_RacerCount(RACE_BOSS, LOAD_CUP_ID_NONE, 2, 0), 2 + RACE_FIELD_BOSS_RACERS,
	           "2P boss race is the two players and the boss");
	CTR_TEST_CHECK_EQ(RaceField_BotCount(RACE_BOSS, LOAD_CUP_ID_NONE, 2, 0), RACE_FIELD_BOSS_RACERS,
	           "a boss race fields exactly one bot");

	// The boss occupies a slot of its own, so the extended option must not add
	// racers behind it however many players there are.
	CTR_TEST_CHECK_EQ(RaceField_RacerCount(RACE_BOSS, LOAD_CUP_ID_NONE, 2, 1), 3, "the option cannot widen a boss race");
}

// === Rounds with no grid ===

static void TestRoundsWithNoGridFieldNoBots(void)
{
	CTR_TEST_SECTION("no-grid rounds: only the players take part");

	// Each of these races without a starting grid: a clock, a set of crystals,
	// the hub, or no race at all. Retail spawns no AI for any of them, and
	// filling a grid for one would queue racers nobody drives.
	static const struct
	{
		const char *name;
		int gameMode1;
	} kNoGridRaces[] = {
		{"relic race", RACE_RELIC},
		{"crystal challenge", RACE_CRYSTAL},
		{"time trial", RACE_CLOCK},
		{"adventure hub", RACE_HUB},
		{"battle mode", BATTLE_MODE},
		{"main menu", MAIN_MENU},
		{"cutscene", GAME_CUTSCENE},
	};

	for (size_t i = 0; i < sizeof(kNoGridRaces) / sizeof(kNoGridRaces[0]); i++)
	{
		for (int playerCount = 1; playerCount <= 4; playerCount++)
		{
			// The option is the axis too: it must not turn a clock run into a
			// grid race by filling a grid the round does not have.
			CTR_TEST_CHECK_EQ(RaceField_BotCount(kNoGridRaces[i].gameMode1, LOAD_CUP_ID_NONE, playerCount, 0), 0,
			           kNoGridRaces[i].name);
			CTR_TEST_CHECK_EQ(RaceField_BotCount(kNoGridRaces[i].gameMode1, LOAD_CUP_ID_NONE, playerCount, 1), 0,
			           kNoGridRaces[i].name);
			CTR_TEST_CHECK_EQ(RaceField_RacerCount(kNoGridRaces[i].gameMode1, LOAD_CUP_ID_NONE, playerCount, 0), playerCount,
			           "a no-grid round has only its players");
		}
	}
}

// === Cross-checks ===

static void TestRetailConstantsAreUnchanged(void)
{
	CTR_TEST_SECTION("constants: the masks still read the way retail wrote them");

	// Retail writes this mask as the literal 0x2c122020 in MainInit_Drivers. If
	// a mode bit moves, races silently start or stop fielding bots. The header
	// asserts it at compile time; this pins it as a reported failure too.
	CTR_TEST_CHECK_EQ(RACE_FIELD_NO_BOT_MODES, 0x2c122020, "the no-bot mask is retail's literal");

	// Every mode the mask claims must actually be a round with no grid.
	CTR_TEST_CHECK((RACE_FIELD_NO_BOT_MODES & RELIC_RACE) != 0, "a relic race is in the mask");
	CTR_TEST_CHECK((RACE_FIELD_NO_BOT_MODES & CRYSTAL_CHALLENGE) != 0, "a crystal challenge is in the mask");
	CTR_TEST_CHECK((RACE_FIELD_NO_BOT_MODES & TIME_TRIAL) != 0, "a clock run is in the mask");
	CTR_TEST_CHECK((RACE_FIELD_NO_BOT_MODES & ADVENTURE_ARENA) != 0, "the hub is in the mask");
	CTR_TEST_CHECK((RACE_FIELD_NO_BOT_MODES & BATTLE_MODE) != 0, "a battle is in the mask");
	CTR_TEST_CHECK((RACE_FIELD_NO_BOT_MODES & MAIN_MENU) != 0, "the main menu is in the mask");
	CTR_TEST_CHECK((RACE_FIELD_NO_BOT_MODES & GAME_CUTSCENE) != 0, "a cutscene is in the mask");

	// And the modes that do race must stay out of it.
	CTR_TEST_CHECK((RACE_FIELD_NO_BOT_MODES & ARCADE_MODE) == 0, "an arcade race is not in the mask");
	CTR_TEST_CHECK((RACE_FIELD_NO_BOT_MODES & ADVENTURE_MODE) == 0, "an adventure race is not in the mask");
	CTR_TEST_CHECK((RACE_FIELD_NO_BOT_MODES & ADVENTURE_CUP) == 0, "a cup race is not in the mask");
	CTR_TEST_CHECK((RACE_FIELD_NO_BOT_MODES & (int)ADVENTURE_BOSS) == 0, "a boss race is not in the mask");

	CTR_TEST_CHECK_EQ(RACE_FIELD_FULL_GRID_RACERS, 8, "the full grid is eight racers");
	CTR_TEST_CHECK_EQ(RACE_FIELD_PAIR_GRID_RACERS, 6, "the pair grid is six racers");
	CTR_TEST_CHECK_EQ(RACE_FIELD_BOSS_RACERS, 1, "a boss race fields one bot");
}

static void TestExtendedOptionScope(void)
{
	CTR_TEST_SECTION("option scope: the extended grid applies to Arcade and Adventure races only");

	for (int extended = 0; extended <= 1; extended++)
	{
		// The option is checked before the managed-roster test, so a mode that
		// never races cannot pick up a grid from it.
		CTR_TEST_CHECK_EQ(RaceField_UsesExtendedGrid(ARCADE_MODE, LOAD_CUP_ID_NONE, extended), extended, "arcade follows the option");
		CTR_TEST_CHECK_EQ(RaceField_UsesExtendedGrid(ADVENTURE_MODE, LOAD_CUP_ID_NONE, extended), extended,
		           "adventure follows the option");
		CTR_TEST_CHECK_EQ(RaceField_UsesExtendedGrid(RACE_GEM_CUP, 0, extended), extended, "a gem cup follows the option");

		// These pick their own roster, so the option must leave their slots alone.
		CTR_TEST_CHECK_EQ(RaceField_UsesExtendedGrid(RACE_GEM_CUP, CUP_ID_PURPLE_GEM, extended), 0,
		           "the purple cup keeps its own roster");
		CTR_TEST_CHECK_EQ(RaceField_UsesExtendedGrid(RACE_BOSS, LOAD_CUP_ID_NONE, extended), 0, "a boss race keeps its own roster");
		CTR_TEST_CHECK_EQ(RaceField_UsesExtendedGrid(RACE_RELIC, LOAD_CUP_ID_NONE, extended), 0, "a relic race has no grid to widen");
		CTR_TEST_CHECK_EQ(RaceField_UsesExtendedGrid(RACE_CRYSTAL, LOAD_CUP_ID_NONE, extended), 0,
		           "a crystal challenge has no grid to widen");
		CTR_TEST_CHECK_EQ(RaceField_UsesExtendedGrid(RACE_CLOCK, LOAD_CUP_ID_NONE, extended), 0, "a clock run has no grid to widen");

		// Not a race at all.
		CTR_TEST_CHECK_EQ(RaceField_UsesExtendedGrid(RACE_HUB, LOAD_CUP_ID_NONE, extended), 0, "the hub is not a race");
		CTR_TEST_CHECK_EQ(RaceField_UsesExtendedGrid(MAIN_MENU, LOAD_CUP_ID_NONE, extended), 0, "the main menu is not a race");
		CTR_TEST_CHECK_EQ(RaceField_UsesExtendedGrid(BATTLE_MODE, LOAD_CUP_ID_NONE, extended), 0, "a battle is not a race");
		CTR_TEST_CHECK_EQ(RaceField_UsesExtendedGrid(GAME_CUTSCENE, LOAD_CUP_ID_NONE, extended), 0, "a cutscene is not a race");
		CTR_TEST_CHECK_EQ(RaceField_UsesExtendedGrid(0, LOAD_CUP_ID_NONE, extended), 0, "no mode at all is not a race");
	}
}

// No mode bit means "a race" by itself: a race is Arcade or Adventure, and the
// rest only modify one. So a caller that forgets to pass the base gets the safe
// answer (no bots) rather than a grid for a race that is not happening.
static void TestAModeBitAloneIsNotARace(void)
{
	CTR_TEST_SECTION("coverage: a mode bit on its own is not a race");

	static const int kAloneBits[] = {
		ADVENTURE_CUP,
		(int)ADVENTURE_BOSS,
		END_OF_RACE,
		RELIC_RACE,
		CRYSTAL_CHALLENGE,
		TIME_TRIAL,
		ADVENTURE_ARENA,
		BATTLE_MODE,
		MAIN_MENU,
		GAME_CUTSCENE,
		LOADING,
	};

	for (size_t i = 0; i < sizeof(kAloneBits) / sizeof(kAloneBits[0]); i++)
	{
		for (int extended = 0; extended <= 1; extended++)
		{
			CTR_TEST_CHECK_EQ(RaceField_BotCount(kAloneBits[i], LOAD_CUP_ID_NONE, 2, extended), 0, "a lone mode bit fields no bots");
		}
	}
}

int main(void)
{
	TestNormalRaceGrid();
	TestBotCountFollowsTheGrid();
	TestHighPlayerCountsNeedTheOption();
	TestGemCupRaces();
	TestBossRaceFieldsTheBoss();
	TestRoundsWithNoGridFieldNoBots();
	TestRetailConstantsAreUnchanged();
	TestExtendedOptionScope();
	TestAModeBitAloneIsNotARace();

	return CtrTest_Summary();
}