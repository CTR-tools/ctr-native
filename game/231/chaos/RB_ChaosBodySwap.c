#include <common.h>
#include "RB_ChaosAudio.h"
#include "RB_ChaosBodySwap.h"
#include "RB_ChaosEffects.h"

static void Chaos_MoveDriverThread(struct Driver *driver, int from, int to)
{
	struct Thread *thread = driver->instSelf->thread;
	struct Thread **link = &GAME_TRACKER->threadBuckets[from].thread;
	while (*link != thread)
	{
		link = &(*link)->siblingThread;
	}
	*link = thread->siblingThread;
	thread->siblingThread = GAME_TRACKER->threadBuckets[to].thread;
	GAME_TRACKER->threadBuckets[to].thread = thread;
	thread->flags = (thread->flags & ~0xffu) | to;
}

void Chaos_FollowBody(int player, struct Driver *driver)
{
	struct CameraDC *camera = &GAME_TRACKER->cameraDC[player];
	camera->driverToFollow = driver;
	camera->flags &= ~CAMERA_FLAG_MASK_GRAB;
	camera->flags |= CAMERA_FLAG_DIRECTION_CHANGED | CAMERA_FLAG_RESET_RAIN_POS;
	memset(&camera->BlastedLerp, 0, sizeof(camera->BlastedLerp));
	camera->heightSmoothing = (struct CameraHeightSmoothing){0};
	sdata->chaosRuntime.cameraBlendPending[player] = true;
}

static void Chaos_SwapBodies(int first, int second)
{
	struct GameTracker *tracker = GAME_TRACKER;
	struct ChaosRuntime *runtime = &sdata->chaosRuntime;
	struct Driver *a = tracker->drivers[first];
	struct Driver *b = tracker->drivers[second];
	const bool aHuman = first < tracker->numPlyrCurrGame;
	const bool bHuman = second < tracker->numPlyrCurrGame;
	const int aEngine = Chaos_EngineClassForDriver(a, GAME_CHARACTER_METADATA[GAME_CHARACTER_IDS[first]].engineID);
	const int bEngine = Chaos_EngineClassForDriver(b, GAME_CHARACTER_METADATA[GAME_CHARACTER_IDS[second]].engineID);
	const s16 character = GAME_CHARACTER_IDS[first];
	GAME_CHARACTER_IDS[first] = GAME_CHARACTER_IDS[second];
	GAME_CHARACTER_IDS[second] = character;
	tracker->drivers[first] = b;
	tracker->drivers[second] = a;
	a->driverID = second;
	b->driverID = first;
	// The rank number belongs to the viewport, not the physical kart.
	struct Instance *bigNum = a->instBigNum;
	a->instBigNum = b->instBigNum;
	b->instBigNum = bigNum;
	// Rank icon animation, unlike the viewport number, follows the kart.
	s16 rankIcon = data.rankIconsCurr[first];
	data.rankIconsCurr[first] = data.rankIconsCurr[second];
	data.rankIconsCurr[second] = rankIcon;
	rankIcon = data.rankIconsDesired[first];
	data.rankIconsDesired[first] = data.rankIconsDesired[second];
	data.rankIconsDesired[second] = rankIcon;
	rankIcon = data.rankIconsTransitionTimer[first];
	data.rankIconsTransitionTimer[first] = data.rankIconsTransitionTimer[second];
	data.rankIconsTransitionTimer[second] = rankIcon;
	if (aHuman)
	{
		tracker->humanPlayerPositions[first] = b->driverRank;
	}
	if (bHuman)
	{
		tracker->humanPlayerPositions[second] = a->driverRank;
	}

	const struct ChaosIdentity identity = runtime->identity[first];
	runtime->identity[first] = runtime->identity[second];
	runtime->identity[second] = identity;
	const u8 engine = runtime->engineClass[first];
	const bool changed = runtime->engineClassChanged[first];
	runtime->engineClass[first] = runtime->engineClass[second];
	runtime->engineClassChanged[first] = runtime->engineClassChanged[second];
	runtime->engineClass[second] = engine;
	runtime->engineClassChanged[second] = changed;
	const u8 crossings = runtime->teleportForwardCrossings[first];
	runtime->teleportForwardCrossings[first] = runtime->teleportForwardCrossings[second];
	runtime->teleportForwardCrossings[second] = crossings;

	Chaos_ResetKartScale(a);
	Chaos_ResetKartScale(b);
	if (aHuman != bHuman)
	{
		struct Driver *human = aHuman ? b : a;
		struct Driver *bot = aHuman ? a : b;
		Chaos_MoveDriverThread(human, ROBOT, PLAYER);
		Chaos_MoveDriverThread(bot, PLAYER, ROBOT);
		LIST_RemoveMember(&sdata->navBotList[human->botData.botPath], &human->botData.item);
		VehPhysProc_TakeControl(human);
		bot->instSelf->thread->modelIndex = DYNAMIC_ROBOT_CAR;
		BOTS_Driver_Resume(bot);
	}
	if (aHuman)
	{
		Chaos_ChangeEngineAudio(b, aEngine, bEngine);
		Chaos_FollowBody(first, b);
		runtime->touchingTurboPad[first] = false;
	}
	if (bHuman)
	{
		Chaos_ChangeEngineAudio(a, bEngine, aEngine);
		Chaos_FollowBody(second, a);
		runtime->touchingTurboPad[second] = false;
	}
}

void Chaos_ReturnBodySwap(int first)
{
	struct ChaosRuntime *runtime = &sdata->chaosRuntime;
	const int second = runtime->bodySwapPartner[first] - 1;
	runtime->bodySwapPartner[first] = 0;
	runtime->bodySwapPartner[second] = 0;
	Chaos_SwapBodies(first, second);
}

void Chaos_ReturnBodySwaps(void)
{
	struct ChaosRuntime *runtime = &sdata->chaosRuntime;
	for (int first = 0; first < (int)len(runtime->bodySwapPartner); first++)
	{
		const int second = runtime->bodySwapPartner[first] - 1;
		if (second > first)
		{
			Chaos_ReturnBodySwap(first);
		}
	}
}

static bool Chaos_CanSwapBody(int slot)
{
	const struct Driver *driver = GAME_TRACKER->drivers[slot];
	return driver != NULL && driver->instSelf != NULL && driver->instSelf->thread != NULL &&
	       (slot >= GAME_TRACKER->numPlyrCurrGame || Chaos_IsPlayerEnabled(slot)) &&
	       (driver->actionsFlagSet & ACTION_RACE_FINISHED) == 0;
}

int Chaos_CollectBodySwapPartners(int player, int *candidates)
{
	const struct ChaosRuntime *runtime = &sdata->chaosRuntime;
	if (!Chaos_CanSwapBody(player) || runtime->bodySwapPartner[player] != 0)
	{
		return 0;
	}
	int count = 0;
	for (int slot = 0; slot < (int)len(GAME_TRACKER->drivers); slot++)
	{
		if (slot != player && runtime->bodySwapPartner[slot] == 0 && Chaos_CanSwapBody(slot))
		{
			candidates[count++] = slot;
		}
	}
	return count;
}

void Chaos_BeginBodySwapPair(int player, int partner)
{
	struct ChaosRuntime *runtime = &sdata->chaosRuntime;
	runtime->bodySwapPartner[player] = partner + 1;
	runtime->bodySwapPartner[partner] = player + 1;
	Chaos_SwapBodies(player, partner);
}

void Chaos_BeginBodySwaps(void)
{
	struct GameTracker *tracker = GAME_TRACKER;
	Chaos_ReturnBodySwaps();
	for (int player = 0; player < tracker->numPlyrCurrGame; player++)
	{
		int candidates[LOAD_CHARACTER_ID_COUNT];
		const int count = Chaos_CollectBodySwapPartners(player, candidates);
		if (count == 0)
		{
			continue;
		}
		const int partner = candidates[ChaosEffect_NextRandom(&sdata->chaosRng) % count];
		Chaos_BeginBodySwapPair(player, partner);
	}
	Chaos_RefreshBodySwapModifiers();
}

