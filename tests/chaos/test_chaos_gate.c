// Pins the race gate in include/chaos/chaos_gate.h: which levels Chaos may run on,
// and what it takes to widen that later.

#include <shared/ctr_test.h>
#include <common.h>
#include <chaos/chaos_gate.h>

// regionsEXE.h defines sdata as a pointer into this, so the linker wants the
// storage even though nothing here reads it.
struct sData sdata_static;

// An Adventure race, and the shape every accepted case starts from.
#define CHAOS_GATE_ADVENTURE_RACE (ADVENTURE_MODE)
#define CHAOS_GATE_GAMEPLAY_GROUP  OVERLAY_INDEX_RACING_OR_BATTLE

static b32 IsEligible(int gameMode1)
{
	return ChaosGate_IsRaceEligible(gameMode1, CHAOS_GATE_GAMEPLAY_GROUP, 1, CHAOS_GATE_ADVENTURE_ONLY);
}

// === The levels Chaos must never touch ===

static void TestHubIsExcluded(void)
{
	CTR_TEST_SECTION("the Adventure hub is not a race");

	// ADVENTURE_ARENA is what distinguishes a hub from a race, and LOAD clears
	// it on level change while leaving ADVENTURE_MODE alone. That split is the
	// whole reason the hub needs its own reject.
	CTR_TEST_CHECK(!IsEligible(ADVENTURE_MODE | ADVENTURE_ARENA), "the hub is skipped");
	CTR_TEST_CHECK(!IsEligible(ADVENTURE_MODE | ADVENTURE_ARENA | GAME_CUTSCENE), "a hub cutscene is skipped");
}

static void TestNonRaceStatesAreExcluded(void)
{
	CTR_TEST_SECTION("menus, cutscenes, pauses, loads and results are not races");

	CTR_TEST_CHECK(!IsEligible(ADVENTURE_MODE | MAIN_MENU), "the main menu is skipped");
	CTR_TEST_CHECK(!IsEligible(ADVENTURE_MODE | GAME_CUTSCENE), "a cutscene is skipped");
	CTR_TEST_CHECK(!IsEligible(ADVENTURE_MODE | LOADING), "a load is skipped");
	CTR_TEST_CHECK(!IsEligible(ADVENTURE_MODE | END_OF_RACE), "the results screen is skipped");

	// PAUSE_ALL is four bits, so test the set rather than one.
	CTR_TEST_CHECK(!IsEligible(ADVENTURE_MODE | PAUSE_ALL), "any player's pause is skipped");
	CTR_TEST_CHECK(!IsEligible(ADVENTURE_MODE | PAUSE_1), "a single pause is skipped");
	CTR_TEST_CHECK(!IsEligible(ADVENTURE_MODE | PAUSE_2), "a second pause is skipped");
}

static void TestOverlayGroupMustBeGameplay(void)
{
	CTR_TEST_SECTION("only the gameplay thread group is eligible");

	CTR_TEST_CHECK(!ChaosGate_IsRaceEligible(ADVENTURE_MODE, OVERLAY_INDEX_ADV_HUB, 1, CHAOS_GATE_ADVENTURE_ONLY),
				   "the adventure hub group is rejected");
	CTR_TEST_CHECK(!ChaosGate_IsRaceEligible(ADVENTURE_MODE, OVERLAY_INDEX_MAIN_MENU, 1, CHAOS_GATE_ADVENTURE_ONLY),
				   "the main menu group is rejected");
	CTR_TEST_CHECK(!ChaosGate_IsRaceEligible(ADVENTURE_MODE, OVERLAY_INDEX_PODIUMS, 1, CHAOS_GATE_ADVENTURE_ONLY),
				   "the podiums group is rejected");
	CTR_TEST_CHECK(!ChaosGate_IsRaceEligible(ADVENTURE_MODE, OVERLAY_INDEX_NONE, 1, CHAOS_GATE_ADVENTURE_ONLY),
				   "no group at all is rejected");
}

static void TestWaitsForGo(void)
{
	CTR_TEST_SECTION("the countdown is not a live race");

	CTR_TEST_CHECK(!ChaosGate_IsRaceEligible(ADVENTURE_MODE, CHAOS_GATE_GAMEPLAY_GROUP, 0, CHAOS_GATE_ADVENTURE_ONLY),
				   "effects wait for the lights");
}

// === The levels Chaos should touch ===

static void TestAdventureRacesAreEligible(void)
{
	CTR_TEST_SECTION("every Adventure race is eligible");

	CTR_TEST_CHECK(IsEligible(ADVENTURE_MODE), "a plain Adventure race runs chaos");
	CTR_TEST_CHECK(IsEligible(ADVENTURE_MODE | ADVENTURE_BOSS), "a boss race runs chaos");
	CTR_TEST_CHECK(IsEligible(ADVENTURE_MODE | RELIC_RACE), "a relic race runs chaos");
	CTR_TEST_CHECK(IsEligible(ADVENTURE_MODE | CRYSTAL_CHALLENGE), "a crystal challenge runs chaos");
	CTR_TEST_CHECK(IsEligible(ADVENTURE_MODE | ADVENTURE_CUP), "a gem cup runs chaos");
	CTR_TEST_CHECK(IsEligible(ADVENTURE_MODE | RELIC_RACE | ADVENTURE_CUP), "combined modifiers still run chaos");
}

// === Scope ===

static void TestNonAdventureModesAreExcluded(void)
{
	CTR_TEST_SECTION("modes reached from the main menu are out of scope");

	CTR_TEST_CHECK(!IsEligible(0), "no mode bits at all is not an Adventure race");
	CTR_TEST_CHECK(!IsEligible(ARCADE_MODE), "arcade is skipped");
	CTR_TEST_CHECK(!IsEligible(TIME_TRIAL), "time trial is skipped");
	CTR_TEST_CHECK(!IsEligible(BATTLE_MODE), "battle is skipped");
	CTR_TEST_CHECK(!IsEligible(BATTLE_MODE | TIME_LIMIT | LIFE_LIMIT), "a battle with its limits set is skipped");
	CTR_TEST_CHECK(!IsEligible(ADVENTURE_CUP), "a gem cup without ADVENTURE_MODE is skipped");
}

static void TestWideningTheScopeIsAnArgument(void)
{
	CTR_TEST_SECTION("the Adventure restriction is one argument");

	CTR_TEST_CHECK(ChaosGate_IsRaceEligible(ARCADE_MODE, CHAOS_GATE_GAMEPLAY_GROUP, 1, CHAOS_GATE_ANY_RACE),
				   "any-race scope admits an arcade race");
	CTR_TEST_CHECK(ChaosGate_IsRaceEligible(BATTLE_MODE, CHAOS_GATE_GAMEPLAY_GROUP, 1, CHAOS_GATE_ANY_RACE),
				   "any-race scope admits a battle");

	// The rejects above are not scope-dependent: widening must not let the hub
	// or the results screen through, because they share the racing overlay.
	CTR_TEST_CHECK(!ChaosGate_IsRaceEligible(ADVENTURE_ARENA, CHAOS_GATE_GAMEPLAY_GROUP, 1, CHAOS_GATE_ANY_RACE),
				   "widening still rejects the hub");
	CTR_TEST_CHECK(!ChaosGate_IsRaceEligible(END_OF_RACE, CHAOS_GATE_GAMEPLAY_GROUP, 1, CHAOS_GATE_ANY_RACE),
				   "widening still rejects the results screen");
	CTR_TEST_CHECK(!ChaosGate_IsRaceEligible(MAIN_MENU, CHAOS_GATE_GAMEPLAY_GROUP, 1, CHAOS_GATE_ANY_RACE),
				   "widening still rejects the main menu");
	CTR_TEST_CHECK(!ChaosGate_IsRaceEligible(ADVENTURE_MODE, CHAOS_GATE_GAMEPLAY_GROUP, 0, CHAOS_GATE_ANY_RACE),
				   "widening still waits for the lights");
}

static void TestPodiumIsExcludedByTheOverlayAlone(void)
{
	CTR_TEST_SECTION("a podium is excluded by the thread group alone");

	// The one case where the mode bits cannot help. LOAD_TenStages.c:129 clears
	// the four bits that mark a non-race level and :132-168 sets one back only
	// for the seven level-name prefixes it knows. A podium has none of those
	// prefixes, so it reaches the gate with every bit in the mask clear and
	// ADVENTURE_MODE still set. Only the thread group catches it.
	const int podiumMode = ADVENTURE_MODE;

	CTR_TEST_CHECK(IsEligible(podiumMode), "without the thread group check the mode bits alone would admit it");
	CTR_TEST_CHECK(!ChaosGate_IsRaceEligible(podiumMode, OVERLAY_INDEX_PODIUMS, 1, CHAOS_GATE_ADVENTURE_ONLY),
				   "the podiums group is rejected");
	CTR_TEST_CHECK(!ChaosGate_IsRaceEligible(podiumMode, OVERLAY_INDEX_PODIUMS, 1, CHAOS_GATE_ANY_RACE),
				   "and widening the scope does not reopen it");
	CTR_TEST_CHECK(ChaosGate_IsRaceEligible(podiumMode, CHAOS_GATE_GAMEPLAY_GROUP, 1, CHAOS_GATE_ADVENTURE_ONLY),
				   "while the gameplay group still is admitted");
}

int main(void)
{
	CTR_TEST_SECTION("chaos_gate.h: race eligibility");

	TestHubIsExcluded();
	TestNonRaceStatesAreExcluded();
	TestOverlayGroupMustBeGameplay();
	TestWaitsForGo();
	TestAdventureRacesAreEligible();
	TestNonAdventureModesAreExcluded();
	TestWideningTheScopeIsAnArgument();
	TestPodiumIsExcludedByTheOverlayAlone();

	return CtrTest_Summary();
}
