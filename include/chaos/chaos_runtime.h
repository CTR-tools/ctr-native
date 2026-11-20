#ifndef CTR_NATIVE_CHAOS_RUNTIME_H
#define CTR_NATIVE_CHAOS_RUNTIME_H

#include <chaos/chaos_effect.h>

enum ChaosRuntimeConstants
{
	CHAOS_MAX_ACTIVATIONS = 4,
	CHAOS_MAX_INSTANT_NOTICES = 4,
	CHAOS_MILLISECONDS_PER_SECOND = 1000,
	CHAOS_ACTIVATION_NOTICE_MS = 3000,
	CHAOS_ACTIVATION_DOCK_MS = 320,
	CHAOS_WOBBLE_PERIOD_MS = 2000,
	CHAOS_WUMPA_DRAIN_PERIOD_MS = 1000,
	CHAOS_INJECTION_RESERVES = 10 * 960,
	CHAOS_MAX_DEFERRED_IMPACTS = 1024,
	CHAOS_MAX_HUMAN_PLAYERS = 4,
};

enum ChaosImpactKind
{
	CHAOS_IMPACT_MINE_DRIVER,
	CHAOS_IMPACT_MOVING_DRIVER,
	CHAOS_IMPACT_BURST_THREAD,
	CHAOS_IMPACT_BURST_LEVEL,
	CHAOS_IMPACT_HAZARD_THREAD,
	CHAOS_IMPACT_HAZARD_LEVEL,
	CHAOS_IMPACT_DRIVER_DAMAGE,
};

struct ChaosDeferredImpact
{
	enum ChaosImpactKind kind;
	struct Thread *source;
	struct Thread *target;
	struct BSP *bsp;
	SVec3 pos;
	s16 damage;
	s16 reason;
	s16 attackerSlot;
};

struct ChaosActivation
{
	ChaosEffectId effect;
	s32 remainingMS;
	s32 noticeRemainingMS;
	s32 elapsedMS;
};

enum ChaosControlAction
{
	CHAOS_CONTROL_THROTTLE,
	CHAOS_CONTROL_BRAKE,
	CHAOS_CONTROL_ITEM,
	CHAOS_CONTROL_HOP_LEFT,
	CHAOS_CONTROL_HOP_RIGHT,
	CHAOS_CONTROL_COUNT,
};

struct ChaosControlShuffle
{
	u8 destination[CHAOS_CONTROL_COUNT];
	u32 analogHeld;
	u32 analogPrev;
};

struct ChaosRestart
{
	s32 timer;
	s32 lightsPrevious;
	bool pending;
	bool active;
	bool floating;
	bool cameraBlendPending;
	struct MaskHeadWeapon *mask;
};

struct ChaosIdentity
{
	struct Model *originalModel;
	s16 originalCharacter;
	bool changed;
	s16 pendingCharacter;
	s32 transformRemainingMS;
};

struct ChaosPlayerRuntime
{
	struct ChaosActivation active[CHAOS_MAX_ACTIVATIONS];
	int numActive;
	struct ChaosActivation instantNotices[CHAOS_MAX_INSTANT_NOTICES];
	int numInstantNotices;
	s32 intervalRemainingMS;
};

// Race-only state, captured in SDATA. Shared Chaos uses only players[0].
struct ChaosRuntime
{
	struct ChaosPlayerRuntime players[CHAOS_MAX_HUMAN_PLAYERS];
	s32 botCooldownMS;
	s32 botEffectCooldownMS[CHAOS_EFFECT_COUNT];
	bool raceInitialized;
	bool touchingTurboPad[CHAOS_MAX_HUMAN_PLAYERS];
	struct ChaosControlShuffle controls[CHAOS_MAX_HUMAN_PLAYERS];
	u8 engineClass[LOAD_CHARACTER_ID_COUNT];
	bool engineClassChanged[LOAD_CHARACTER_ID_COUNT];
	struct ChaosRestart restart[CHAOS_MAX_HUMAN_PLAYERS];
	struct ChaosIdentity identity[LOAD_CHARACTER_ID_COUNT];
	// Partner slot plus one; zero means this controller owns its original kart.
	u8 bodySwapPartner[LOAD_CHARACTER_ID_COUNT];
	bool cameraBlendPending[CHAOS_MAX_HUMAN_PLAYERS];
	u8 teleportForwardCrossings[LOAD_CHARACTER_ID_COUNT];
	bool humanAudioContext;
	bool humanXA;
	int numDeferredImpacts;
	struct ChaosDeferredImpact deferredImpacts[CHAOS_MAX_DEFERRED_IMPACTS];
};

#endif
