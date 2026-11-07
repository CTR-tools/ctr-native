#ifndef CTR_TEST_CHAOS_FIXTURE_H
#define CTR_TEST_CHAOS_FIXTURE_H

// Shared asset-free world and integration doubles for the Chaos suites.
// Include the real modules in unity order so both suites exercise the same paths.
#include <shared/ctr_test.h>
#include <common.h>

struct sData sdata_static;
struct Data data;
NativeConfig g_config;
// VehBirth.c precedes Chaos in the game unity build; this fixture doubles birth.
enum { VEH_BIRTH_WHEEL_SIZE = 0xccc };
static struct GameTracker s_tracker;
static struct GamepadSystem s_gamepads;
static struct Driver s_defaultDrivers[2];
static struct QuadBlock s_teleportGround;
static bool s_teleportGroundMissing;
static SVec3 s_teleportProbe;
static int s_teleportProbes;
static bool s_allowRaceEnd;
static int s_teleportRaceEnds;
static u32 s_pausedVoices;
static b32 s_pausedXA;
static int s_timeStopCrateSounds;
static int s_timeStopTickSounds;
static u32 s_timeStopTickFlags;
static struct ChannelStats s_timeStopTickChannel;
static int s_deferredCollisions;
static int s_deferredNitroBlasts;
static int s_deferredPotionShatters;
static struct Thread *s_lastMineDriver;
static int s_frozenObstacleCollisions;

void RB_GenericMine_ThTick(struct Thread *thread) { (void)thread; }
void RB_MovingExplosive_ThTick(struct Thread *thread) { (void)thread; }
void RB_Warpball_ThTick(struct Thread *thread) { (void)thread; }
void RB_Minecart_ThTick(struct Thread *thread) { (void)thread; }
void RB_Snowball_ThTick(struct Thread *thread) { (void)thread; }
void RB_Baron_ThTick(struct Thread *thread) { (void)thread; }
void RB_Spider_ThTick(struct Thread *thread) { (void)thread; }
void RB_Plant_ThTick_Hungry(struct Thread *thread) { (void)thread; }
void RB_FlameJet_ThTick(struct Thread *thread) { (void)thread; }
void RB_Armadillo_ThTick_Rolling(struct Thread *thread) { (void)thread; }
void RB_Armadillo_ThTick_TurnAround(struct Thread *thread) { (void)thread; }
void RB_Seal_ThTick_Move(struct Thread *thread) { (void)thread; }
void RB_Seal_ThTick_TurnAround(struct Thread *thread) { (void)thread; }
void RB_Fireball_ThTick(struct Thread *thread) { (void)thread; }
struct Instance *LinkedCollide_Radius(struct Instance *inst, struct Thread *thread, struct Thread *bucket, u32 radius)
{
	(void)inst; (void)thread; (void)bucket; (void)radius;
	CtrTest_Bail("isolated Chaos checks must not run a hazard's spherical collision query");
	return NULL;
}
void RB_Minecart_CollideDrivers(struct Thread *thread) { (void)thread; s_frozenObstacleCollisions++; }
void RB_Snowball_CollideDrivers(struct Thread *thread) { (void)thread; s_frozenObstacleCollisions++; }
void RB_Baron_CollideDrivers(struct Thread *thread) { (void)thread; s_frozenObstacleCollisions++; }
void RB_Spider_CollideDrivers(struct Thread *thread) { (void)thread; s_frozenObstacleCollisions++; }
void RB_FlameJet_CollideDrivers(struct Thread *thread) { (void)thread; s_frozenObstacleCollisions++; }
void RB_GenericMine_CollideDriver(struct Thread *thread, struct Thread *driver)
{
	s_lastMineDriver = driver;
	if (thread->modelIndex == STATIC_BEAKER_RED || thread->modelIndex == STATIC_BEAKER_GREEN)
	{
		thread->flags |= THREAD_FLAG_DEAD;
	}
	s_deferredCollisions++;
}
void RB_GenericMine_ThDestroy(struct Thread *thread, struct Instance *inst, struct MineWeapon *mine)
{
	(void)inst; (void)mine;
	thread->flags |= THREAD_FLAG_DEAD;
	if (thread->modelIndex == PU_EXPLOSIVE_CRATE)
	{
		s_deferredNitroBlasts++;
	}
	else
	{
		s_deferredPotionShatters++;
	}
	s_deferredCollisions++;
}
void RB_MovingExplosive_CollideDriver(struct Thread *thread, struct Thread *driver) { (void)thread; (void)driver; s_deferredCollisions++; }
void RB_Burst_CollThBucket(struct ScratchpadStruct *sps, void *target) { (void)sps; (void)target; s_deferredCollisions++; }
void RB_Burst_CollLevInst(struct ScratchpadStruct *sps, void *target) { (void)sps; (void)target; s_deferredCollisions++; }
void RB_Teeth_OpenDoor(struct Instance *inst) { (void)inst; }
void RB_MinePool_Remove(struct MineWeapon *mine) { (void)mine; }
void RB_Blowup_Init(struct Instance *inst) { (void)inst; }
void RB_Explosion_InitGeneric(struct Instance *inst) { (void)inst; }
void OtherFX_RecycleMute(u32 *sound) { *sound = 0; }
u32 BOTS_ChangeState(struct Driver *victim, int damage, struct Driver *attacker, int reason)
{
	(void)attacker;
	(void)reason;
	victim->kartState = damage == 2 ? KS_BLASTED : KS_SPINNING;
	return 1;
}
struct MetaDataMODEL *COLL_LevModelMeta(u32 id) { (void)id; return NULL; }

void NativeAudio_SetPausedVoices(u32 voices, b32 pauseXA)
{
	s_pausedVoices = voices;
	s_pausedXA = pauseXA;
}

void COLL_SearchBSP_CallbackQUADBLK(const SVec3 *top, const SVec3 *bottom, struct ScratchpadStruct *sps, s32 radius)
{
	(void)radius;
	s_teleportProbe = (SVec3){top->x, (top->y + bottom->y) / 2, top->z};
	s_teleportProbes++;
	sps->boolDidTouchQuadblock = !s_teleportGroundMissing;
	sps->hit.ptrQuadblock = &s_teleportGround;
	sps->hit.plane.normal = (SVec3){0, FP_ONE, 0};
	sps->Union.QuadBlockColl.hitPos = s_teleportProbe;
}

static void TestTeleportMatrix(MATRIX *matrix, s16 *normal, s32 yaw)
{
	(void)normal;
	*matrix = (MATRIX){0};
	matrix->m[0][0] = MATH_Sin(yaw + ANG_HALF_PI);
	matrix->m[0][2] = MATH_Sin(yaw);
	matrix->m[1][1] = FP_ONE;
	matrix->m[2][0] = -MATH_Sin(yaw);
	matrix->m[2][2] = MATH_Sin(yaw + ANG_HALF_PI);
}

static void TestBotTick(struct Thread *thread)
{
	(void)thread;
}

struct NavFrame *BOTS_FindClosestNavFrame(int path, const Vec3 *position)
{
	(void)position;
	return sdata->NavPath_ptrNavFrameArray[path];
}

void BOTS_SetRotation(struct Driver *driver, int spawnYaw)
{
	(void)spawnYaw;
	driver->botData.navProgressRemainder = 0;
	driver->botData.botFlags |= BOT_FLAG_ESTIMATE_NAV;
}

#ifndef CHAOS_FIXTURE_REAL_LAP_STATS
void PlayLevel_CrossFinishLine(struct Driver *driver, bool leadingHuman, bool coopRace)
{
	(void)driver; (void)leadingHuman; (void)coopRace;
	CtrTest_Bail("this fixture must not dispatch a lap crossing");
}
#endif

bool BOTS_Driver_Resume(struct Driver *driver)
{
	driver->actionsFlagSet |= ACTION_BOT;
	driver->instSelf->thread->funcThTick = TestBotTick;
	memset(&driver->botData, 0, sizeof(driver->botData));
	LIST_AddFront(&sdata->navBotList[0], &driver->botData.item);
	return true;
}

void BOTS_ThTick_RevEngine(struct Thread *thread)
{
	(void)thread;
}

void VehPhysProc_Driving_Init(struct Thread *thread, struct Driver *driver)
{
	(void)thread;
	driver->kartState = KS_NORMAL;
}

void VehPhysProc_SpinFirst_InitSetUpdate(struct Thread *thread, struct Driver *driver)
{
	(void)thread;
	(void)driver;
}

void VehStuckProc_Tumble_SetFunctions(struct Driver *driver)
{
	(void)driver;
}
static bool s_allHumansFinished;
static int s_activationSounds;
static int s_clockSounds;
static int s_identitySounds;
static struct Particle s_transformationSmoke[16];
static int s_transformationSmokeCount;
static bool s_transformationSmokeExhausted;
static int s_squishSounds;
static int s_crateExplosions;
static int s_crateSounds;
static struct Instance *s_lastExplodingCrate;
static u32 s_lastSound;
static u32 s_lastSoundFlags;
static int s_noticeDraws;
static const char *s_noticeText;
static s16 s_noticeX;
static s16 s_noticeY;
static s16 s_noticeFont;
static u16 s_noticeFlags;
static u32 *s_noticeOT;
static const char *s_drawText[CHAOS_EFFECT_COUNT];
static s16 s_drawX[CHAOS_EFFECT_COUNT];
static s16 s_drawY[CHAOS_EFFECT_COUNT];
static s16 s_drawFont[CHAOS_EFFECT_COUNT];
static s32 s_drawScale[CHAOS_EFFECT_COUNT];
static u32 *s_drawOT[CHAOS_EFFECT_COUNT];
static int s_maskGrants;
static int s_shieldGrants;
static int s_itemSounds;
static u32 s_itemSound;
static int s_juicedSounds;
static u32 s_engineStarted[4];
static u32 s_engineStopped[4];
static int s_engineStarts[4];
static u8 s_chaosScratchpad[1024];
u8 *gCTRNativeScratchpadBase = s_chaosScratchpad;
static struct MaskHeadWeapon s_restartMasks[ADV_MULTIPLAYER_NUM_PLAYERS_MAX];
static int s_restartInitializations;
static int s_restartFallPoses;
static int s_startLightDraws;
static int s_startCountdownSounds;
static int s_startGreenSounds;
static u32 s_wrongWayDraws;
static int s_wrongWayVoices;
static int s_wrongWayVoiceID;
static bool s_voicePlayFails;
static int s_testLocalizedVoiceID;
static int s_testFallbackVoiceID;
static bool s_testLocalizedVoiceSucceeds;
static struct Driver *s_voiceDriver;
static struct
{
	u32 header;
	struct Model model;
} s_characterModelFiles[GAME_CHARACTER_COUNT];
static int s_characterModelReads;
static struct Model *s_residentCharacterModels[GAME_CHARACTER_COUNT];

struct Model *VehBirth_GetModelByName(char *name)
{
	for (int character = 0; character < GAME_CHARACTER_COUNT; character++)
	{
		if (name == GAME_CHARACTER_METADATA[character].name_Debug)
		{
			return s_residentCharacterModels[character];
		}
	}
	return NULL;
}

void *LOAD_DramFile(void *bigfile, int fileIndex, void *destination, u32 *size, int callbackOrFlags)
{
	(void)bigfile;
	CTR_TEST_CHECK(destination == NULL && callbackOrFlags == -1, "character models are prepared with the synchronous relocatable loader");
	const int character = fileIndex - BI_RACERMODELHI;
	CTR_TEST_CHECK(character >= 0 && character < GAME_CHARACTER_COUNT, "preloading requests a playable character model");
	s_characterModelReads++;
	*size = sizeof(s_characterModelFiles[character]);
	return &s_characterModelFiles[character];
}

void UI_DrawWrongWay(const struct Driver *driver)
{
	s_wrongWayDraws |= 1u << driver->driverID;
}

b32 VehPickupItem_MaskBoolGoodGuy(struct Driver *driver)
{
	s_voiceDriver = driver;
	return driver->driverID == 0;
}

int CDSYS_XAPlay(int category, int voice)
{
	if (category == CDSYS_XA_TYPE_GAME)
	{
		s_testFallbackVoiceID = voice;
		return 1;
	}
	CTR_TEST_CHECK_EQ(category, CDSYS_XA_TYPE_EXTRA, "wrong-way voices use the normal mask speech bank");
	s_wrongWayVoiceID = voice;
	s_wrongWayVoices++;
	if (s_voicePlayFails)
	{
		return 0;
	}
	sdata->XA_State = XA_PLAYING;
	return 1;
}

int CDSYS_XAPlayLocalized(int category, int voice)
{
	(void)category;
	s_testLocalizedVoiceID = voice;
	return s_testLocalizedVoiceSucceeds;
}

int CDSYS_XAGetTrackLength(int category, int voice)
{
	(void)category; (void)voice;
	return 300;
}

void Bank_ResetAllocator(void) {}
void Audio_SetDefaults(void) {}
void Music_SetDefaults(void) {}
unsigned int SpuSetReverbVoice(int enabled, unsigned int voices)
{
	(void)enabled; (void)voices;
	return 0;
}
void SpuSetVoiceADSRAttr(int voice, unsigned short ar, unsigned short dr, unsigned short sr,
                       unsigned short rr, unsigned short sl, int arMode, int srMode, int rrMode)
{
	(void)voice; (void)ar; (void)dr; (void)sr; (void)rr; (void)sl;
	(void)arMode; (void)srMode; (void)rrMode;
}

void VehStuckProc_RevEngine_Init(struct Thread *thread, struct Driver *driver)
{
	(void)thread;
	driver->kartState = KS_ENGINE_REVVING;
	driver->KartStates.RevEngine.boolMaskGrab = false;
	s_restartInitializations++;
}

struct MaskHeadWeapon *VehPickupItem_MaskCreateRescue(struct Driver *driver)
{
	return &s_restartMasks[driver->driverID];
}

void VehStuckProc_MaskGrab_SetFallPose(struct Driver *driver)
{
	(void)driver;
	s_restartFallPoses++;
}

u32 VehFrameInst_GetNumAnimFrames(struct Instance *instance, int animation)
{
	(void)instance; (void)animation;
	return 41;
}

int VehFrameInst_GetStartFrame(int animation, int frames)
{
	return animation == 0 ? frames / 2 : 0;
}

void DecalHUD_DrawPolyFT4(struct Icon *icon, s32 x, s32 y, struct PrimMem *memory, u32 *ot, u8 color, s16 scale)
{
	(void)icon; (void)x; (void)y; (void)memory; (void)ot; (void)color; (void)scale;
	s_startLightDraws++;
}

b32 EngineAudio_InitOnce(u32 soundID, u32 flags)
{
	CTR_TEST_CHECK_EQ(flags, HOWL_SFX_CENTER_NO_DISTORTION, "class changes use normal engine audio flags");
	s_engineStarted[soundID % 4] = soundID;
	s_engineStarts[soundID % 4]++;
	return true;
}

void EngineAudio_Stop(u32 soundID)
{
	s_engineStopped[soundID % 4] = soundID;
}

void VehStuckProc_RIP_Init(struct Thread *thread, struct Driver *driver)
{
	(void)thread;
	(void)driver;
	CtrTest_Bail("Wumpa crediting must not eliminate a driver");
}

void MainGameEnd_Initialize(void)
{
	if (!s_allowRaceEnd)
	{
		CtrTest_Bail("Wumpa crediting must not end the race");
	}
	s_teleportRaceEnds++;
	s_tracker.gameMode1 |= END_OF_RACE;
}

s32 MixRNG_Scramble(void)
{
	CtrTest_Bail("Chaos item selection must use its dedicated RNG");
	return 0;
}

b32 VehPickupItem_ApplyMaskMode(struct Driver *driver)
{
	(void)driver;
	return true;
}

struct MaskHeadWeapon *VehPickupItem_MaskUseWeapon(struct Driver *driver, b32 playSound)
{
	CTR_TEST_CHECK_EQ(driver->maskIsAku, -1, "direct mask activation ignores the held item's mask choice");
	CTR_TEST_CHECK(playSound, "direct mask activation requests the normal power-up sound");
	s_maskGrants++;
	return NULL;
}

void VehPickupItem_ShootNow(struct Driver *driver, s32 weapon, s32 flags)
{
	(void)driver;
	CTR_TEST_CHECK_EQ(flags, 0, "Chaos power-ups use the normal activation flags");
	if (weapon == HELD_ITEM_MASK)
	{
		s_maskGrants++;
	}
	else if (weapon == HELD_ITEM_SHIELD)
	{
		s_shieldGrants++;
	}
	else
	{
		CtrTest_Bail("Chaos direct grants must dispatch a mask or shield");
	}
}

static struct Driver *s_mailSource;
static struct Driver *s_mailTarget;
static int s_mailLaunches;

void VehPickupItem_ShootWarpball(struct Driver *driver, struct Driver *target)
{
	s_mailSource = driver;
	s_mailTarget = target;
	s_mailLaunches++;
}

// VehPickupItem.c precedes Chaos in the game unity build.
enum { SOUND_CLOCK = 0x44, SOUND_INVISIBILITY = 0x61 };

#include <game/LIST.c>
#include <game/HOWL/HOWL_Voiceline.c>
#include <game/DotLights.c>
#include <game/Vehicle/VehPhysCrash.c>
#include <game/Vehicle/VehBirth_Consts.c>
#include <game/Vehicle/VehCalc.c>
#include <game/Vehicle/VehPhysForce.c>
// Exercise the real bot-to-human handover; only driving callbacks are doubled.
enum { BOTS_DAMAGE_STATE_SPIN = 1, BOTS_DAMAGE_STATE_BLAST = 2, VEH_PHYS_PROC_SPIN_INITIAL_RATE = 300 };
#include <game/Vehicle/VehPhysProc_TakeControl.c>
#include <game/Vehicle/VehPickState.c>
#include <game/Vehicle/VehPhysGeneral.c>
#include <game/Vehicle/VehPhysProc_Items.c>
// Keep the damage-path guard double while exercising real Wumpa crediting.
#define RB_Player_KillPlayer RB_Player_KillPlayer_Unused
#include <game/231/RB_Player.c>
#undef RB_Player_KillPlayer
static int s_launchConversions[8];
static Vec3 s_launchVelocity[8];
static bool s_convertLaunchSpeed;

// Ordinary lifecycle checks capture the boundary; overflow checks also commit it.
static void CaptureLaunchVelocity(struct Driver *driver, Vec3 *velocity)
{
	s_launchConversions[driver->driverID]++;
	s_launchVelocity[driver->driverID] = *velocity;
	if (s_convertLaunchSpeed)
	{
		VehPhysCrash_ConvertVecToSpeed(driver, velocity);
	}
}

#define VehPhysCrash_ConvertVecToSpeed CaptureLaunchVelocity
#define VehPhysForce_RotAxisAngle TestTeleportMatrix
#include <game/231/RB_Warpball_Path.c>
#include <game/231/RB_Hazard.c>
#include <game/231/RB_ChaosEffects.c>
#undef VehPhysForce_RotAxisAngle
#undef VehPhysCrash_ConvertVecToSpeed
#include <game/231/RB_Chaos.c>

static const char *s_externalCommand;
static int s_externalReads;
static int s_externalErrors;
static char s_externalCatalogue[NATIVE_CHAOS_CATALOGUE_SIZE];
static int s_cataloguePublications;

int NativeChaosIPC_ReadCommand(char *command, size_t capacity)
{
	s_externalReads++;
	if (s_externalCommand == NULL)
	{
		return NATIVE_CHAOS_NO_COMMAND;
	}
	snprintf(command, capacity, "%s", s_externalCommand);
	s_externalCommand = NULL;
	return NATIVE_CHAOS_COMMAND;
}

void NativeChaosIPC_WriteEffectsCatalogue(const char *catalogue)
{
	s_cataloguePublications++;
	snprintf(s_externalCatalogue, sizeof(s_externalCatalogue), "%s", catalogue);
}

void Platform_LogError(const char *format, ...)
{
	(void)format;
	s_externalErrors++;
}
#include <game/Vehicle/VehFire.c>
#include <game/Vehicle/VehAfterColl.c>
#include <game/UI/UI_Lerp2D.c>
#include <game/UI/UI_Chaos.c>
#include <game/MATH/MATH_0_Sin.c>

s32 DecalFont_GetLineWidth(char *text, s16 font)
{
	return (s32)strlen(text) * (font == FONT_SMALL ? 4 : 8);
}

void DecalFont_DrawLineOT(char *text, s16 x, s16 y, s16 font, s16 flags, u32 *ot)
{
	if (s_noticeDraws >= CHAOS_EFFECT_COUNT)
	{
		CtrTest_Bail("the Chaos HUD must draw at most one label per catalogue effect");
	}
	s_drawText[s_noticeDraws] = text;
	s_drawX[s_noticeDraws] = x;
	s_drawY[s_noticeDraws] = y;
	s_drawFont[s_noticeDraws] = font;
	s_drawOT[s_noticeDraws] = ot;
	s_drawScale[s_noticeDraws] = FP(1.0);
	s_noticeDraws++;
	s_noticeText = text;
	s_noticeX = x;
	s_noticeY = y;
	s_noticeFont = font;
	s_noticeFlags = (u16)flags;
	s_noticeOT = ot;
}

void DecalFont_DrawLineScaledOT(char *text, s16 x, s16 y, s16 font, s16 flags, s32 scale, u32 *ot)
{
	DecalFont_DrawLineOT(text, x, y, font, flags, ot);
	s_drawScale[s_noticeDraws - 1] = scale;
}

b32 MainGameEnd_AllHumansFinished(struct GameTracker *gGT)
{
	(void)gGT;
	return s_allHumansFinished;
}

void RB_Burst_InitVisual(struct Instance *inst)
{
	s_crateExplosions++;
	s_lastExplodingCrate = inst;
}
void PlaySound3D(u32 soundID, struct Instance *inst)
{
	CTR_TEST_CHECK_EQ(soundID, HOWL_SFX_TNT_EXPLOSION, "explosive crates use explosion audio");
	CTR_TEST_CHECK(inst == s_lastExplodingCrate, "explosion audio originates at the touched crate");
	s_crateSounds++;
}
void VehPhysProc_SpinFirst_Init(struct Thread *thread, struct Driver *driver)
{
	(void)thread;
	(void)driver;
}
void VehStuckProc_PlantEaten_Init(struct Thread *thread, struct Driver *driver)
{
	(void)thread;
	(void)driver;
	CtrTest_Bail("Clock Curse must select spin damage");
}
void VehStuckProc_Tumble_Init(struct Thread *thread, struct Driver *driver)
{
	(void)thread;
	(void)driver;
	CtrTest_Bail("Clock Curse must select spin damage");
}
void GAMEPAD_ShockFreq(struct Driver *driver, int frame, int value)
{
	(void)driver;
	(void)frame;
	(void)value;
}
void OtherFX_Play_Echo(u32 soundID, int flags, int echoFlag)
{
	(void)flags;
	(void)echoFlag;
	CTR_TEST_CHECK_EQ(soundID, VEH_PICK_SOUND_SQUISH, "squish damage uses the retail audio");
	s_squishSounds++;
}
void RB_Player_KillPlayer(struct Driver *attacker, struct Driver *victim)
{
	(void)attacker;
	(void)victim;
	CtrTest_Bail("Clock Curse must not attribute damage to an attacker");
}
void CTC2(u32 value, s32 reg)
{
	(void)value;
	(void)reg;
	CtrTest_Bail("clock damage without an attacker needs no projection");
}
void MTC2(u32 value, s32 reg)
{
	(void)value;
	(void)reg;
	CtrTest_Bail("clock damage without an attacker needs no projection");
}
s32 MFC2_S(s32 reg)
{
	(void)reg;
	CtrTest_Bail("clock damage without an attacker needs no projection");
	return 0;
}
s32 doCOP2(s32 op)
{
	(void)op;
	CtrTest_Bail("clock damage without an attacker needs no projection");
	return 0;
}
int OtherFX_Play(u32 soundID, int flags)
{
	if (soundID == 0x40)
	{
		return OtherFX_Play_LowLevel(soundID, flags, HOWL_SFX_DEFAULT_FLAGS);
	}
	if (soundID == 0x3c)
	{
		CTR_TEST_CHECK(sdata->chaosRuntime.humanAudioContext, "the time-stop cue remains audible while world sounds pause");
		s_timeStopCrateSounds++;
		return 0;
	}
	if (soundID == SOUND_INVISIBILITY)
	{
		CTR_TEST_CHECK_EQ(flags, 1, "Identity Crisis uses the normal invisibility sound flags");
		s_identitySounds++;
		return 0;
	}
	if (soundID == DOT_LIGHT_FX_COUNTDOWN || soundID == DOT_LIGHT_FX_GREEN)
	{
		s_startCountdownSounds += soundID == DOT_LIGHT_FX_COUNTDOWN;
		s_startGreenSounds += soundID == DOT_LIGHT_FX_GREEN;
		return 0;
	}
	if (soundID == 0x5e || soundID == 0x41)
	{
		CTR_TEST_CHECK_EQ(flags, soundID == 0x41, "item acquisition retains the normal Wumpa-dependent sound flags");
		s_itemSounds++;
		s_itemSound = soundID;
		if (soundID == 0x41)
		{
			s_juicedSounds++;
		}
		return 0;
	}
	CTR_TEST_CHECK_EQ(soundID, SOUND_CLOCK, "Clock Curse plays the retail clock sound");
	s_clockSounds++;
	return 0;
}
int OtherFX_Play_LowLevel(u32 soundID, u8 antiSpam, u32 flags)
{
	if (soundID == 0x40)
	{
		CTR_TEST_CHECK(sdata->chaosRuntime.humanAudioContext, "time-stop ticks remain audible while world audio is suspended");
		if (s_timeStopTickSounds == 0)
		{
			LIST_AddBack(&sdata->channelTaken, &s_timeStopTickChannel.link.item);
		}
		s_timeStopTickSounds++;
		s_timeStopTickFlags = flags;
		s_timeStopTickChannel.type = HOWL_CHANNEL_TYPE_OTHER_FX;
		s_timeStopTickChannel.soundID = 0x10040;
		s_timeStopTickChannel.timeLeft = 10;
		Chaos_TagAudioChannel(&s_timeStopTickChannel);
		return s_timeStopTickChannel.soundID;
	}
	(void)antiSpam;
	s_activationSounds++;
	s_lastSound = soundID;
	s_lastSoundFlags = flags;
	return 0;
}

void howl_StopAudio(b32 backup, b32 music, b32 fx)
{
	(void)backup;
	(void)music;
	(void)fx;
}

void GhostTape_WriteBoosts(int addReserve, u32 type, int speedCap)
{
	(void)addReserve;
	(void)type;
	(void)speedCap;
}

void GAMEPAD_ShockForce1(struct Driver *driver, int frame, int value)
{
	(void)driver;
	(void)frame;
	(void)value;
}

struct Instance *INSTANCE_BirthWithThread(s32 modelID, const char *name, s32 poolType,
                                        s32 bucket, void *tick, s32 objSize, struct Thread *parent)
{
	(void)modelID;
	(void)name;
	(void)poolType;
	(void)bucket;
	(void)tick;
	(void)objSize;
	(void)parent;
	return NULL;
}

struct Instance *INSTANCE_Birth3D(struct Model *model, const char *name, struct Thread *thread)
{
	(void)model;
	(void)name;
	(void)thread;
	return NULL;
}

void VehTurbo_ThTick(struct Thread *thread)
{
	(void)thread;
}

void VehTurbo_ThDestroy(struct Thread *thread)
{
	(void)thread;
}

#include <game/MATH/MATH_2_FastSqrt.c>

// These full-physics paths are outside the asset-free scale/contact checks.
int ratan2(int y, int x)
{
	return (int)lround(atan2((double)y, (double)x) * ANG_HALF_PI / atan2(1.0, 0.0));
}
void ConvertRotToMatrix(MATRIX *matrix, const SVec3 *rotation)
{
	(void)matrix; (void)rotation;
	CtrTest_Bail("scale/contact checks must not animate matrices");
}
void MatrixRotate(void *dst, MATRIX *src, MATRIX *rotation)
{
	(void)dst; (void)src; (void)rotation;
	CtrTest_Bail("scale/contact checks must not animate matrices");
}
void OtherFX_DriverCrashing(u32 echo, u32 volume)
{
	(void)echo; (void)volume;
	CtrTest_Bail("squash checks must not use the ordinary crash sound");
}
void BOTS_CollideWithOtherAI(struct Driver *first, struct Driver *second)
{
	(void)first; (void)second;
	CtrTest_Bail("squash checks must not run bot navigation");
}
void GAMEPAD_JogCon1(struct Driver *driver, u8 value, u16 timeMS)
{
	(void)driver; (void)value; (void)timeMS;
	CtrTest_Bail("scale/contact checks must not run ordinary crash rumble");
}
void PROC_CollidePointWithBucket(struct Thread *thread, struct BucketSearchParams *search)
{
	(void)thread; (void)search;
	CtrTest_Bail("scale/contact checks must not search collision buckets");
}
void PROC_CollidePointWithSelf(struct Thread *thread, struct BucketSearchParams *search)
{
	(void)thread; (void)search;
	CtrTest_Bail("isolated scale checks must not search frozen driver buckets");
}
u32 MFC2(int reg)
{
	(void)reg;
	CtrTest_Bail("scale/contact checks must not project geometry");
	return 0;
}
struct Particle *Particle_Init(u32 flags, struct IconGroup *icons, struct ParticleEmitter *emitter)
{
	(void)flags; (void)icons;
	if (emitter == data.emSet_BurnSmoke)
	{
		if (s_transformationSmokeExhausted)
		{
			return NULL;
		}
		struct Particle *particle = &s_transformationSmoke[s_transformationSmokeCount++ % len(s_transformationSmoke)];
		*particle = (struct Particle){
			.flagsSetColor = PARTICLE_SET_COLOR_FLAG_DRIVER_LOCAL,
			.flagsAxis = PARTICLE_AXIS_FLAG_SCALE_X,
			.driverID = -1,
		};
		return particle;
	}
	CtrTest_Bail("scale/contact checks must not spawn particles");
	return NULL;
}

static void SetupRace(int trigger)
{
	s_timeStopCrateSounds = 0;
	s_timeStopTickSounds = 0;
	s_timeStopTickFlags = 0;
	memset(&s_timeStopTickChannel, 0, sizeof(s_timeStopTickChannel));
	s_allowRaceEnd = false;
	memset(data.metaPhys, 0, sizeof(data.metaPhys));
	memset(data.MetaDataCharacters, 0, sizeof(data.MetaDataCharacters));
	memset(data.characterIDs, 0, sizeof(data.characterIDs));
	// Stand-in retail storage: repeated entries must not weight uniform gifts.
	memset(data.RNG_itemSetRace2, HELD_ITEM_TURBO, sizeof(data.RNG_itemSetRace2));
	int poolIndex = 0;
	for (int item = HELD_ITEM_TURBO; item <= HELD_ITEM_MISSILE_3X; item++)
	{
		if (item != HELD_ITEM_SPRING)
		{
			data.RNG_itemSetRace2[poolIndex++] = (u8)item;
		}
	}
	s_allHumansFinished = false;
	s_activationSounds = 0;
	s_clockSounds = 0;
	s_identitySounds = 0;
	s_transformationSmokeCount = 0;
	s_transformationSmokeExhausted = false;
	s_squishSounds = 0;
	s_crateExplosions = 0;
	s_crateSounds = 0;
	s_wrongWayDraws = 0;
	s_wrongWayVoices = 0;
	s_wrongWayVoiceID = 0;
	s_voicePlayFails = false;
	s_voiceDriver = NULL;
	s_characterModelReads = 0;
	memset(s_residentCharacterModels, 0, sizeof(s_residentCharacterModels));
	memset(s_engineStarted, 0, sizeof(s_engineStarted));
	memset(s_engineStopped, 0, sizeof(s_engineStopped));
	memset(s_engineStarts, 0, sizeof(s_engineStarts));
	memset(s_launchConversions, 0, sizeof(s_launchConversions));
	memset(s_launchVelocity, 0, sizeof(s_launchVelocity));
	s_convertLaunchSpeed = false;
	s_lastExplodingCrate = NULL;
	memset(&sdata_static, 0, sizeof(sdata_static));
	memset(&s_tracker, 0, sizeof(s_tracker));
	memset(&s_gamepads, 0, sizeof(s_gamepads));
	memset(s_defaultDrivers, 0, sizeof(s_defaultDrivers));
	memset(&g_config, 0, sizeof(g_config));
	sdata->gGT = &s_tracker;
	sdata->gGamepads = &s_gamepads;
	sdata->Loading.stage = LOAD_IDLE;
	s_tracker.gameMode1 = ADVENTURE_MODE;
	s_tracker.overlayIndex_Threads = OVERLAY_INDEX_RACING_OR_BATTLE;
	s_tracker.numPlyrCurrGame = 2;
	for (int i = 0; i < s_tracker.numPlyrCurrGame; i++)
	{
		s_defaultDrivers[i].driverID = i;
		s_tracker.drivers[i] = &s_defaultDrivers[i];
		s_gamepads.gamepad[i].stickLX = CHAOS_STICK_CENTER;
		s_gamepads.gamepad[i].stickLY = CHAOS_STICK_CENTER;
		s_gamepads.gamepad[i].stickRY = CHAOS_STICK_CENTER;
	}
	sdata->chaosSettings = (struct ChaosSettings){
		.armed = true, .trigger = (u8)trigger, .durationSeconds = 30,
		.triggerIntervalSeconds = 5, .enabledCount = 1,
	};
	sdata->chaosSettings.effects[CHAOS_EFFECT_FLY_ME_TO_THE_MOON] = true;
	Chaos_BeginRace(&s_tracker);
}

static void Tick(int elapsedMS)
{
	s_tracker.elapsedTimeMS = elapsedMS;
	Chaos_Update(&s_tracker);
}

static void EnableInputEffects(void)
{
	SetupRace(CHAOS_TRIGGER_INTERVAL);
	memset(sdata->chaosSettings.effects, 0, sizeof(sdata->chaosSettings.effects));
	sdata->chaosSettings.effects[CHAOS_EFFECT_MIRROR_MIRROR] = true;
	sdata->chaosSettings.effects[CHAOS_EFFECT_GROUNDED] = true;
	sdata->chaosSettings.enabledCount = 2;
}

static void DrawNotice(void)
{
	s_noticeDraws = 0;
	UI_Chaos_DrawActivation(&s_tracker);
}

#endif
