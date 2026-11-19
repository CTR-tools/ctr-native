// Pins the supported events and gameplay states in include/chaos/chaos_gate.h.

#include <shared/ctr_test.h>
#include <common.h>
#include <chaos/chaos_gate.h>

// Resident storage for the suite's game-facing helpers.
struct sData sdata_static;

#define CHAOS_GATE_GAMEPLAY_GROUP  OVERLAY_INDEX_RACING_OR_BATTLE

static b32 IsEligible(int gameMode1)
{
	return ChaosGate_IsRaceEligible(gameMode1, CHAOS_GATE_GAMEPLAY_GROUP, 1);
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

	CTR_TEST_CHECK(!ChaosGate_IsRaceEligible(ADVENTURE_MODE, OVERLAY_INDEX_ADV_HUB, 1),
				   "the adventure hub group is rejected");
	CTR_TEST_CHECK(!ChaosGate_IsRaceEligible(ADVENTURE_MODE, OVERLAY_INDEX_MAIN_MENU, 1),
				   "the main menu group is rejected");
	CTR_TEST_CHECK(!ChaosGate_IsRaceEligible(ADVENTURE_MODE, OVERLAY_INDEX_PODIUMS, 1),
				   "the podiums group is rejected");
	CTR_TEST_CHECK(!ChaosGate_IsRaceEligible(ADVENTURE_MODE, OVERLAY_INDEX_NONE, 1),
				   "no group at all is rejected");
}

static void TestWaitsForGo(void)
{
	CTR_TEST_SECTION("the countdown is not a live race");

	CTR_TEST_CHECK(!ChaosGate_IsRaceEligible(ADVENTURE_MODE, CHAOS_GATE_GAMEPLAY_GROUP, 0),
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

static void TestSessionModesAreEligible(void)
{
	CTR_TEST_SECTION("Arcade, VS and Battle share the gameplay-state requirements");
	const int modes[] = {ARCADE_MODE, 0, BATTLE_MODE, BATTLE_MODE | TIME_LIMIT | LIFE_LIMIT,
	                     ARCADE_MODE | RELIC_RACE, ARCADE_MODE | CRYSTAL_CHALLENGE};
	const int blocked[] = {ADVENTURE_ARENA, END_OF_RACE, MAIN_MENU, GAME_CUTSCENE, PAUSE_ALL, LOADING, TIME_TRIAL};
	for (int i = 0; i < (int)len(modes); i++)
	{
		CTR_TEST_CHECK(IsEligible(modes[i]), "session events are eligible in the gameplay overlay after GO");
		CTR_TEST_CHECK(!ChaosGate_IsRaceEligible(modes[i], CHAOS_GATE_GAMEPLAY_GROUP, 0),
		               "session effects also wait for the lights");
		for (int j = 0; j < (int)len(blocked); j++)
		{
			CTR_TEST_CHECK(!IsEligible(modes[i] | blocked[j]), "non-race states and Time Trial block every session mode");
		}
	}
	CTR_TEST_CHECK(!IsEligible(TIME_TRIAL), "Time Trial is excluded");
	CTR_TEST_CHECK(!IsEligible(ADVENTURE_MODE | TIME_TRIAL), "Adventure cannot override the Time Trial exclusion");
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
	CTR_TEST_CHECK(!ChaosGate_IsRaceEligible(podiumMode, OVERLAY_INDEX_PODIUMS, 1),
				   "the podiums group is rejected");
	CTR_TEST_CHECK(ChaosGate_IsRaceEligible(podiumMode, CHAOS_GATE_GAMEPLAY_GROUP, 1),
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
	TestSessionModesAreEligible();
	TestPodiumIsExcludedByTheOverlayAlone();

	return CtrTest_Summary();
}
