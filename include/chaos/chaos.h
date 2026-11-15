#ifndef CTR_NATIVE_CHAOS_H
#define CTR_NATIVE_CHAOS_H

void Chaos_Init(u32 seed);
void Chaos_Reset(void);
bool Chaos_IsRunning(const struct GameTracker *gGT);
void Chaos_BeginRace(struct GameTracker *gGT);
void Chaos_ReturnBodySwapsNearFinish(struct GameTracker *gGT, int nearDistance);
bool Chaos_ConsumeTeleportLapCrossing(const struct Driver *driver);
bool Chaos_IsTimeStopped(void);
bool Chaos_UsesTimePalette(void);
bool Chaos_IsHumanThread(const struct Thread *thread);
bool Chaos_IsHumanOwnedThread(const struct Thread *thread);
bool Chaos_IsFrozenWorldThread(const struct Thread *thread);
bool Chaos_DeferImpact(enum ChaosImpactKind kind, struct Thread *source, struct Thread *target, struct BSP *bsp);
bool Chaos_DeferDriverDamage(struct Driver *victim, int damage, struct Driver *attacker, int reason);
void Chaos_RecordFrozenMaskContacts(struct Driver *attacker);
bool Chaos_HasDeferredImpact(const struct Thread *thread);
void Chaos_ResolveDeferredImpacts(void);
void Chaos_CollideFrozenThread(struct Thread *thread);
bool Chaos_IsHumanCharacter(int character);
bool Chaos_ShouldFreezeThread(const struct Thread *thread);
bool Chaos_ShouldFreezeParticle(const struct Particle *particle);
bool Chaos_SetHumanAudio(bool human);
void Chaos_TagAudioChannel(struct ChannelStats *channel);
bool Chaos_ShouldFreezeAudioChannel(const struct ChannelStats *channel);
bool Chaos_IsMenuPreviewSong(const struct Song *song);
void Chaos_UpdateTimeStopAudio(void);
void Chaos_LoadCharacterModels(struct GameTracker *gGT, struct BigHeader *bigfile);
void Chaos_Update(struct GameTracker *gGT);
void Chaos_PublishEffects(void);
bool Chaos_Activate(ChaosEffectId effect, s32 durationMS);
bool Chaos_OnPickup(struct Driver *driver);
void Chaos_OnTurboPadContact(struct Driver *driver, bool touching);
bool Chaos_IsActive(ChaosEffectId effect);
bool Chaos_IsActiveForDriver(ChaosEffectId effect, const struct Driver *driver);
int Chaos_GravityForDriver(const struct Driver *driver);
int Chaos_SpeedForDriver(const struct Driver *driver, int speed);
int Chaos_JumpForceForDriver(const struct Driver *driver, int force);
int Chaos_AccelerationForDriver(const struct Driver *driver, int acceleration);
int Chaos_ReservesForDriver(const struct Driver *driver, int reserves);
int Chaos_FireLevelForDriver(const struct Driver *driver, int fireLevel);
int Chaos_EngineClassForDriver(const struct Driver *driver, int engineClass);
int Chaos_StartTimerForDriver(const struct Driver *driver);
bool Chaos_IsWaitingForStart(const struct Driver *driver);
bool Chaos_ConsumeCameraBlend(struct CameraDC *camera);
void Chaos_ApplyStartPose(struct Driver *driver);
void Chaos_DrawStartLights(struct GameTracker *gGT);
void Chaos_DrawWrongWayWarnings(struct GameTracker *gGT);
bool Chaos_OverridesWrongWayWarning(const struct Driver *driver);
bool Chaos_PlayWrongWayVoice(void);
bool Chaos_ApplyCameraRules(struct CameraDC *camera);
void Chaos_ApplyLevelVisibilityRules(const struct PushBuffer *pushBuffer, struct VisMem *visibility,
                                   const struct mesh_info *mesh, int playerIndex);
bool Chaos_HideCameraQuadBlock(const struct PushBuffer *pushBuffer, const struct QuadBlock *block, const struct LevVertex *vertices);
bool Chaos_HideCameraInstance(const struct PushBuffer *pushBuffer, const struct Instance *instance, int nearestDepth, bool coversKart);
void Chaos_ApplyItemRules(struct Driver *driver);
void Chaos_ApplyCrateRules(struct Driver *driver, struct Instance *crateInst);
bool Chaos_IsHardCrateForDriver(const struct Instance *crateInst, const struct Thread *collidingTh,
                               const struct ScratchpadStruct *sps);
void Chaos_ApplyRescueRules(struct Driver *driver);
void Chaos_ApplyInputRules(const struct Driver *driver, struct GamepadBuffer *pad);
u32 Chaos_ApplyDriver(struct Driver *driver, u32 actions);
void UI_Chaos_DrawActivation(struct GameTracker *gGT);

#endif
