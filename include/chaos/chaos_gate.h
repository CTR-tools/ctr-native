#ifndef CHAOS_GATE_H
#define CHAOS_GATE_H
#include "namespace_Main.h"


#include <stdbool.h>

// Pure race eligibility check. The engine must separately check the loaded
// file's chaosSettings.armed flag and any active menu; the creation-prompt
// option does not control a loaded file.
//
// The inputs are gGT->gameMode1 and gGT->overlayIndex_Threads. The latter is not
// one of the thirteen overlay binaries: it is the thread group, of which there
// are four, and the LOD renderers 226-229 are picked separately by
// LOAD_OvrLOD on the player count. So the value below is 1 for every race,
// whatever the split-screen count.

// Mode bits that mark a level as not-a-race, whichever way it is not one: the
// hub, the menus, a cutscene, a pause, a load, or the results screen.
#define CHAOS_GATE_NON_RACE_MODES (ADVENTURE_ARENA | MAIN_MENU | GAME_CUTSCENE | PAUSE_ALL | LOADING | END_OF_RACE)

// Retail writes this mask as the literal below. The assertion catches a mode
// bit being renumbered without the mask being reconsidered.
CTR_STATIC_ASSERT(CHAOS_GATE_NON_RACE_MODES == 0x6030200F);

enum ChaosGateScope
{
	CHAOS_GATE_ADVENTURE_ONLY = 0,
	CHAOS_GATE_ANY_RACE,
};

// Each condition is named for what has to be true, rather than for what it
// rules out, so the gate reads as a list of requirements.
static inline bool ChaosGate_IsRaceEligible(int gameMode1, int threadsIndex, int lightsGone, enum ChaosGateScope scope)
{
	// None of the non-race markers is set. The other four thread groups are
	// each marked by one of them, so this alone leaves the podium.
	const bool isRaceLevel = (gameMode1 & CHAOS_GATE_NON_RACE_MODES) == 0;

	if (!isRaceLevel)
	{
		return false;
	}

	// Overlay 231, "Racing gameplay" in docs/OVERLAYS.md: the group holding the
	// weapons, hazards, items, bots and pickups.
	const bool isGameplayOverlay = threadsIndex == OVERLAY_INDEX_RACING_OR_BATTLE;
	if (!isGameplayOverlay)
	{
		return false;
	}

	const bool inScope = (scope == CHAOS_GATE_ANY_RACE) || ((gameMode1 & ADVENTURE_MODE) != 0);

	// Wait for GO.
	return lightsGone && inScope;
}

#endif
