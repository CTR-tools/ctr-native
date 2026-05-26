// included from common.h, do not include common.h

// BOTS

void BOTS_SetGlobalNavData(u16 index);
void BOTS_InitNavPath(struct GameTracker *gGT, s16 index);
void BOTS_EmptyFunc();
int BOTS_Adv_NumTimesLostEvent(int index);
void BOTS_Adv_AdjustDifficulty();
void BOTS_UpdateGlobals();
void BOTS_SetRotation(struct Driver *driver_perhaps, int param_2);
void BOTS_LevInstColl(int param_1);
void BOTS_ThTick_RevEngine(struct Thread *thread, u32 param_2, u8 *param_3, u32 param_4);
void BOTS_MaskGrab(int param_1);
void BOTS_Killplane(int param_1);
void BOTS_ThTick_Drive(struct Thread *thread);
u32 BOTS_ChangeState(
    struct Driver *d1, int param_2, struct Driver *d2,
    int param_4); // either this needs to return int, or VehPickState_NewState needs to return u32, if I were to guess, this one needs to return int.
void BOTS_CollideWithOtherAI(struct Driver *robot_1, struct Driver *robot_2);
void BOTS_GotoStartingLine(struct Driver *driver);
struct Driver *BOTS_Driver_Init(int driverID);
void BOTS_Driver_Convert(struct Driver *driver);

// CAM

void CAM_SkyboxGlow(s16 *param_1, struct PushBuffer *pb, struct PrimMem *primMem, u_long *ptrOT);
void CAM_ClearScreen(struct GameTracker *gGT);
void CAM_Init(struct CameraDC *cDC, int cameraID, struct Driver *d, struct PushBuffer *pb);
void CAM_FindClosestQuadblock(s16 *scratchpad, struct CameraDC *cDC, struct Driver *d, s16 *param_4);
int CAM_Path_GetNumPoints(void);
u8 CAM_Path_Move(int frameIndex, s16 *position, s16 *rotation, s16 *getPath);
void CAM_StartOfRace(struct CameraDC *cDC);
void CAM_EndOfRace_Battle(struct CameraDC *cDC, struct Driver *d);
void CAM_EndOfRace(struct CameraDC *cDC, struct Driver *d);
void CAM_StartLine_FlyIn_FixY(s16 *posRot);
void CAM_ProcessTransition(s16 *currPos, s16 *currRot, s16 *startPos, s16 *startRot, s16 *endPos, s16 *endRot, int frame);
void CAM_FollowDriver_AngleAxis(struct CameraDC *cDC, struct Driver *d, int scratchpad, s16 *pushBufferPos, s16 *pushBufferRot);
void CAM_StartLine_FlyIn(struct FlyInData *flyInData, s16 maxFrames, int frame, s16 *desiredPos, s16 *desiredRot);
u32 CAM_FollowDriver_TrackPath(struct CameraDC *cDC, s16 *pos, int speed, int update);
void CAM_LookAtPosition(int scratchpad, int *positions, s16 *desiredPos, s16 *desiredRot);
void CAM_FollowDriver_Spin360(struct CameraDC *cDC, int param_2, struct Driver *d, s16 *desiredPos, s16 *desiredRot);
void CAM_SetDesiredPosRot(struct CameraDC *cDC, s16 *pos, s16 *rot);
void CAM_FollowDriver_Normal(struct CameraDC *cDC, struct Driver *d, s16 *pushBuffer, int scratchpad, struct ZoomData *zoom);
int CAM_MapRange_PosPoints(s16 *pos1, s16 *pos2, s16 *currPos);
void CAM_ThTick(struct Thread *t);

// CDSYS

u32 CDSYS_Init(int useDisc);
u32 CDSYS_GetFilePosInt(char *fileString, int *filePos);
void CDSYS_SetMode_StreamData(void);
void CDSYS_SetMode_StreamAudio(void);
u32 CDSYS_SetXAToLang(int lngIndex);
void CDSYS_XaCallbackCdSync(char result);
void CDSYS_XaCallbackCdReady(char result);
void CDSYS_SpuCallbackIRQ(void);
void CDSYS_SpuCallbackTransfer(void);
void CDSYS_SpuEnableIRQ(void);
void CDSYS_SpuDisableIRQ(void);
void CDSYS_SpuGetMaxSample(void);
u32 CDSYS_XAGetNumTracks(int categoryID);
u32 CDSYS_XASeek(int isCdControl, int categoryID, int audioTrackID);
int CDSYS_XAGetTrackLength(int categoryID, int audioTrackID);
int CDSYS_XAPlay(int categoryID, int index);
void CDSYS_XAPauseRequest(void);
void CDSYS_XAPauseForce(void);
void CDSYS_XAPauseAtEnd(void);

// COLL

struct MetaDataMODEL *COLL_LevModelMeta(u32 index);
u32 COLL_FIXED_INSTANC_TestPoint(struct ScratchpadStruct *param_1, struct BSP *node);
void COLL_FIXED_BSPLEAF_TestInstance(struct BSP *node, struct ScratchpadStruct *sps);
void COLL_FIXED_BotsSearch(s16 *posCurr, s16 *posPrev, s16 *param_3);
void COLL_FIXED_PlayerSearch(struct Thread *t, struct Driver *d);
void COLL_SearchBSP_CallbackQUADBLK(u32 *posTop, u32 *posBottom, struct ScratchpadStruct *sps,
                                    int param_4); // posTop/posButtom may be backwards, also may have 6 params not 4???
void COLL_SearchBSP_CallbackPARAM(struct BSP *param_1, struct BoundingBox *bbox, void (*callback)(struct BSP *, struct ScratchpadStruct *),
                                  struct ScratchpadStruct *param_4);
u32 FUN_8001ede4(u16 *param_1, s16 *param_2, s16 *param_3, s16 *param_4);
void FUN_8001ef1c();
void FUN_8001ef50(int param_1, s16 *param_2, s16 *param_3, s16 *param_4);
void FUN_8001f2dc(int param_1, s16 *param_2, s16 *param_3, s16 *param_4);
void COLL_FIXED_QUADBLK_TestTriangles(struct QuadBlock *qb, struct ScratchpadStruct *sps);
void COLL_FIXED_BSPLEAF_TestQuadblocks(struct BSP *node, struct ScratchpadStruct *sps);
void FUN_8001f67c(int param_1, int param_2);
void FUN_8001f6f0(int param_1, int param_2);
void FUN_8001f7f0(int param_1);
u32 FUN_8001f928(u32 *param_1, u32 *param_2, u32 *param_3, u32 *param_4);
void FUN_8001fc40(u32 *param_1, int param_2, u32 param_3, u32 param_4);
s32 COLL_MOVED_TRIANGL_ReorderNormals(void *set1, void *v1, void *v2, void *v3);
void COLL_MOVED_TRIANGL_TestPoint(struct ScratchpadStruct *sps, struct BspSearchVertex *v1, struct BspSearchVertex *v2, struct BspSearchVertex *v3);
void COLL_MOVED_QUADBLK_TestTriangles(struct QuadBlock *qb, struct ScratchpadStruct *sps);
void COLL_MOVED_BSPLEAF_TestQuadblocks(struct BSP *node, struct ScratchpadStruct *sps);
void COLL_MOVED_FindScrub(struct QuadBlock *qb, int triangleID, struct ScratchpadStruct *sps);
void COLL_MOVED_PlayerSearch(struct Thread *t, struct Driver *d);
u32 COLL_MOVED_ScrubImpact(struct Driver *d, struct Thread *t, struct ScratchpadStruct *sps, struct Scrub *scrub, int *velocity);

// CTR

void CTR_Box_DrawWirePrims(Point p1, Point p2, Color color, void *ot);
void CTR_Box_DrawWireBox(RECT *r, Color color, void *ot);
void CTR_Box_DrawClearBox(RECT *r, Color *color, int transparency, u_long *ot);
void CTR_Box_DrawSolidBox(RECT *r, Color color, u_long *ot);
void CTR_CycleTex_LEV(struct AnimTex *animtex, int timer);
void CTR_CycleTex_Model(struct AnimTex *pAnimTexArray, int timer);
void CTR_CycleTex_AllModels(u32 numModels, struct Model **pModelArray, int timer);
// CTR_CycleTex_2p3p4pWumpaHUD();
void CTR_ClearRenderLists_1P2P(struct GameTracker *gGT, int numPlyrCurrGame);
void CTR_ClearRenderLists_3P4P(struct GameTracker *gGT, int numPlyrCurrGame);
void CTR_EmptyFunc_MainFrame_ResetDB();
void CTR_ErrorScreen(char r, char g, char b);
void CTR_MatrixToRot(SVECTOR *rot, MATRIX *matrix, u32 flags);
// CTR_ScrambleGhostString();

// DebugFont

void DebugFont_Init(struct GameTracker *gGT);
void DebugFont_DrawNumbers(int index, int screenPosX, int screenPosY);

// DecalFont

#if BUILD == JpnRetail
u32 DecalFont_boolRacingWheel();
#endif
int DecalFont_GetLineWidthStrlen(char *str, int len, int fontType);
int DecalFont_GetLineWidth(char *str, s16 fontType);
void DecalFont_DrawLineStrlen(u8 *str, s16 len, int posX, int posY, s16 fontType, int flags);
void DecalFont_DrawLine(char *str, int posX, int posY, s16 fontType, int flags);
void DecalFont_DrawLineOT(char *str, int posX, int posY, s16 fontType, int flags, u_long *ot);
int DecalFont_DrawMultiLineStrlen(char *str, s16 len, s16 posX, s16 posY, s16 maxPixLen, s16 fontType, s16 flags);
int DecalFont_DrawMultiLine(char *str, int posX, int posY, int maxPixLen, s16 fontType, int flags);

// DecalGlobal

void DecalGlobal_EmptyFunc_MainFrame_ResetDB();
void DecalGlobal_Clear(struct GameTracker *gGT);
void DecalGlobal_Store(struct GameTracker *gGT, struct LevTexLookup *LTL);
int *DecalGlobal_FindInLEV(struct Level *level, char *str);
int *DecalGlobal_FindInMPK(u32 *param_1, char *str);

// DecalHUD

void DecalHUD_DrawPolyFT4(struct Icon *icon, s16 posX, s16 posY, struct PrimMem *primMem, u_long *ot, char transparency, s16 scale);
void DecalHUD_DrawWeapon(struct Icon *icon, s16 posX, s16 posY, struct PrimMem *primMem, u_long *ot, char transparency, s16 scale, char rot);
void DecalHUD_DrawPolyGT4(struct Icon *icon, s16 posX, s16 posY, struct PrimMem *primMem, u_long *ot, u32 color0, u32 color1, u32 color2, u32 color3,
                          char transparency, s16 scale);
void DecalHUD_Arrow2D(struct Icon *icon, s16 posX, s16 posY, struct PrimMem *primMem, u_long *otMemPtr, u32 color1, u32 color2, u32 color3, u32 color4,
                      char transparency, s16 scale, u16 rot);

// DecalMP

void DecalMP_01(struct GameTracker *gGT);
void DecalMP_02(struct GameTracker *gGT);
void DecalMP_03(struct GameTracker *gGT);

// DISPLAY

// DISPLAY_Blur_SubFunc()
void DISPLAY_Blur_Main(struct PushBuffer *pb, int strength);
// DISPLAY_SWap()

// DotLights

// DotLights_Video()
void DotLights_AudioAndVideo(struct GameTracker *gGT);

// DropRain

void DropRain_MakeSound(struct GameTracker *gGT);
void DropRain_Reset(struct GameTracker *gGT);

// ElimBG

void ElimBG_SaveScreenshot_Chunk(u16 *img1, u16 *img2, int param_3);
void ElimBG_SaveScreenshot_Full(struct GameTracker *gGT);
void ElimBG_Activate(struct GameTracker *gGT);
void ElimBG_ToggleInstance(struct Instance *inst, char boolGameIsPaused);
void ElimBG_ToggleAllInstances(struct GameTracker *gGT, int boolGameIsPaused);
void ElimBG_HandleState(struct GameTracker *gGT);
void ElimBG_Deactivate(struct GameTracker *gGT);

// FLARE

// FLARE_ThTick()
// FLARE_Init()

// GAMEPAD

void GAMEPAD_Init(struct GamepadSystem *gGamepads);
void GAMEPAD_SetMainMode(void);
void GAMEPAD_ProcessState(struct GamepadBuffer *pad, int padState, s16 id);
void GAMEPAD_PollVsync(struct GamepadSystem *gGamepads);
int GAMEPAD_GetNumConnected(struct GamepadSystem *gGamepads);
int GAMEPAD_ProcessHold(struct GamepadSystem *gGamepads);
void GAMEPAD_ProcessSticks(struct GamepadSystem *gGamepads);
int GAMEPAD_ProcessTapRelease(struct GamepadSystem *gGamepads);
void GAMEPAD_ProcessMotors(struct GamepadSystem *gGamepads);
int GAMEPAD_ProcessAnyoneVars(struct GamepadSystem *gGamepads);
void GAMEPAD_JogCon1(struct Driver *driver, char val, u16 timeMS);
void GAMEPAD_JogCon2(struct Driver *driver, char val, s16 timeMS);
void GAMEPAD_ShockFreq(struct Driver *driver, int frame, int val);
void GAMEPAD_ShockForce1(struct Driver *driver, int frame, int val);
void GAMEPAD_ShockForce2(struct Driver *driver, int frame, int val);

// GAMEPROG

void GAMEPROG_AdvPercent(struct AdvProgress *adv);
void GAMEPROG_ResetHighScores(struct GameProgress *gameProg);
int GAMEPROG_CheckGhostsBeaten(int ghostID);
void GAMEPROG_NewProfile_OutsideAdv(struct GameProgress *gameProg);
void GAMEPROG_InitFullMemcard(struct MemcardProfile *mcp);
void GAMEPROG_NewProfile_InsideAdv(struct AdvProgress *adv);
void GAMEPROG_SaveCupProgress(void);
void GAMEPROG_SyncGameAndCard(struct GameProgress *memcardProg, struct GameProgress *currentProg);
void GAMEPROG_NewGame_OnBoot(void);
void GAMEPROG_GetPtrHighScoreTrack(void);

// GhostBuffer

void GhostReplay_ThTick(struct Thread *t);
void GhostReplay_Init1(void);
void GhostReplay_Init2(void);
void GhostTape_Start(void);
void GhostTape_End(void);
void GhostTape_WriteMoves(s16 param_1);
void GhostTape_WriteBoosts(int reserves, u8 type, int fireLevel);
void GhostTape_Destroy(void);

// OtherFX

s16 CountSounds();
int OtherFX_Play(u32 soundID, int flags);
void OtherFX_Play_Echo(u32 soundID, int flags, int echoFlag);
int OtherFX_Play_LowLevel(u32 soundID, u8 flags1, u32 flags2);
// OtherFX_Modify()
void OtherFX_Stop1(int soundID_count);
void OtherFX_Stop2(int soundID_count);

// EngineAudio

char EngineAudio_InitOnce(u32 soundID, u32 flags);
s16 EngineAudio_Recalculate(u32 soundID, u32 sfx);
void EngineAudio_Stop(u32 soundID);

// More OtherFX?

// SetReverbMode()

// CseqMusic

int CseqMusic_Start(u16 songID, int p2, struct SongSet *p3, int p4, int p5);
void CseqMusic_Pause(void);
void CseqMusic_Resume(void);
void CseqMusic_ChangeVolume(u16 songID, int p2, int p3);
void CseqMusic_Restart(u16 songID, int p2);
void CseqMusic_ChangeTempo(u16 songID, int p2);
void CseqMusic_AdvHubSwap(u16 songId, struct SongSet *songSet, int songSetActiveBits);
void CseqMusic_Stop(u16 songID);
void CseqMusic_StopAll(void);

// Bank

void Bank_ResetAllocator(void);
int Bank_Alloc(int bankID, struct Bank *ptrBank);
int Bank_AssignSpuAddrs(void);
void Bank_Destroy(struct Bank *ptrLastBank);
void Bank_ClearInRange(u16 min, u16 max);
int Bank_Load(int bankID, struct Bank *ptrBank);
int Bank_DestroyLast(void);
void Bank_DestroyUntilIndex(int index);
void Bank_DestroyAll(void);

// from this point onward it's an absolute mess

// howl


// howl_LoadHeader()
// howl_SetSong()
// howl_LoadSong()
// howl_ErasePtrCseqHeader()
// howl_GetNextNote()

// cseq opcode

void cseq_opcode00_empty(struct SongSeq *seq);
void cseq_opcode01_noteoff(struct SongSeq *seq);
void cseq_opcode02_empty(struct SongSeq *seq);
void cseq_opcode03(struct SongSeq *seq);
void cseq_opcode04_empty(struct SongSeq *seq);

// more howl

// howl_InitChannelAttr_Music()

// more cseq opcode

void cseq_opcode_from06and07(struct SongSeq *seq);
void cseq_opcode05_noteon(struct SongSeq *seq);
void cseq_opcode06(struct SongSeq *seq);
void cseq_opcode07(struct SongSeq *seq);
void cseq_opcode08(struct SongSeq *seq);
void cseq_opcode09(struct SongSeq *seq);
void cseq_opcode0a(struct SongSeq *seq);

// SongPool

struct SongSeq *SongPool_FindFreeChannel(void);
u32 SongPool_CalculateTempo(s16 const60, s16 tpqn, s16 bpm);
void SongPool_ChangeTempo(struct Song *song, s16 p2);
void SongPool_Start(struct Song *song, u16 songID, s16 deltaBPM, int boolLoopAtEnd, struct SongSet *songSet, int songSetActiveBits);
void SongPool_Volume(struct Song *song, int newVol, int newStep, int boolImm);
void SongPool_AdvHub1(struct Song *song, int seqID, int vol, int boolImm);
void SongPool_AdvHub2(struct Song *song, struct SongSet *songSet, int songSetActiveBits);
void SongPool_StopCseq(struct SongSeq *seq);
void SongPool_StopAllCseq(struct Song *song);

// even more howl

int howl_Disable(void);

// UpdateChannelVol

// UpdateChannelVol_EngineFX()
// UpdateChannelVol_OtherFX()
// UpdateChannelVol_Music()
// UpdateChannelVol_EngineFX_All()
// UpdateChannelVol_Music_All()
// UpdateChannelVol_OtherFX_All()

// not even the last of howl

int howl_ModeGet(void);
void howl_ModeSet(int newMode);

// OptionsMenu...

void OptionsMenu_TestSound(int param_1, int param_2);

// Smart (?)

void Smart_EnterCriticalSection(void);
void Smart_ExitCriticalSection(void);

// Channel

void Channel_SetVolume(struct ChannelAttr *attr, int volume, int LR);
int Channel_FindSound(int soundID);
struct ChannelStats *Channel_AllocSlot_AntiSpam(s16 soundID, char boolUseAntiSpam, int flags, struct ChannelAttr *attr);
struct ChannelStats *Channel_AllocSlot(int flags, struct ChannelAttr *attr);
struct ChannelStats *Channel_SearchFX_EditAttr(int type, int soundID, int updateFlags, struct ChannelAttr *attr);
struct ChannelStats *Channel_SearchFX_Destroy(int type, int soundID, int flags);
void Channel_DestroyAll_LowLevel(int opt1, int boolKeepMusic, char type);
void Channel_ParseSongToChannels(void);
void Channel_UpdateChannels(void);

// Cutscene (?)

void Cutscene_VolumeBackup(void);
void Cutscene_VolumeRestore(void);

// this is the last of ho- got em

// Voiceline

void Voiceline_PoolInit(void);
void Voiceline_ClearTimeStamp(void);
void Voiceline_PoolClear(void);
void Voiceline_StopAll(void);
void Voiceline_ToggleEnable(int toggle);
void Voiceline_RequestPlay(u32 voiceID, u32 characterID, u32 characterID2);
void Voiceline_StartPlay(struct Item *voiceLine);
void Voiceline_Update(void);
void Voiceline_EmptyFunc(void);
void Voiceline_SetDefaults(void);

// Audio

void Audio_SetState(u32 state);
void Audio_SetState_Safe(int state);
void Audio_AdvHub_SwapSong(int levelID);
void Audio_SetMaskSong(u32 tempo);
void Audio_Update1(void);
void Audio_SetDefaults(void);
void Audio_SetReverbMode(int levelID, u32 isBossRace, int bossID);

// Music

void Music_SetIntro(void);
void Music_LoadBanks(void);
u32 Music_AsyncParseBanks(void);
void Music_SetDefaults(void);
void Music_Adjust(u32 songID, int newTempo, struct SongSet *set, u32 songSetActiveBits);
void Music_LowerVolume(void);
void Music_RaiseVolume(void);
void Music_Restart(void);
void Music_Stop(void);
void Music_Start(u32 songID);
void Music_End(void);
u32 Music_GetHighestSongPlayIndex(void);

// GTE (?)

void GTE_AudioLR_Inst(MATRIX *m, s32 *param_2);
void GTE_AudioLR_Driver(MATRIX *m, struct Driver *d, s32 *param_3);
int GTE_GetSquaredLength(s32 *param_1);

// Even more OtherFX...

// void OtherFX_RecycleNew(u32* param_1, u32 param_2, u32 param_3);
void OtherFX_RecycleMute(int *param_1);
void OtherFX_DriverCrashing(u32 boolEcho, u32 volume);

// uh...

int GTE_GetSquaredDistance(s16 *param_1, s16 *param_2);
void CalculateVolumeFromDistance(u32 *param_1, u32 param_2, int param_3);
void PlayWarppadSound(u32 param_1);

// Level (?)

void Level_SoundLoopSet(int *param_1, u32 param_2, u32 param_3);
void Level_SoundLoopFade(int *param_1, u32 param_2, int param_3, int param_4);
void Level_RandomFX(int *param_1, u32 param_2, int param_3, u32 param_4, int param_5);
void Level_AmbientSound(void);

// PlaySound3D

void PlaySound3D(u32 soundID, struct Instance *inst);
void PlaySound3D_Flags(u32 *flags, u32 soundID, struct Instance *inst);

// EngineSound

void EngineSound_Player(struct Driver *driver);
int EngineSound_VolumeAdjust(int desired, int current, int step);
void EngineSound_AI(struct Driver *ai, struct Driver *cameraDriver, int slotIndex, int distance, int distanceDelta, u32 lr);
void EngineSound_NearestAIs(void);

// Garage

void Garage_Init(void);
void Garage_Enter(char characterID);
void Garage_PlayFX(u32 soundID, char characterID);
void Garage_LerpFX(void);
void Garage_MoveLR(int desiredID);
void Garage_Leave(void);

// okay finally we're done with that

// INSTANCE

void INSTANCE_Birth(struct Instance *inst, struct Model *model, char *name, struct Thread *th, int flags);
struct Instance *INSTANCE_Birth3D(struct Model *m, char *name, struct Thread *t);
struct Instance *INSTANCE_Birth2D(struct Model *model, char *name, struct Thread *th);
struct Instance *INSTANCE_BirthWithThread(int modelID, char *name, int poolType, int bucket, void *funcThTick, int objSize, struct Thread *parent);
struct Instance *INSTANCE_BirthWithThread_Stack(int *spArr);
void INSTANCE_Death(struct Instance *inst);
void INSTANCE_LevInitAll(struct InstDef *instDef, int num);
void INSTANCE_LevDelayedLInBs(struct Instance *instDefs, u32 numInstances);

// not really part of "INSTANCE" namespace
u32 VehFrameInst_GetNumAnimFrames(struct Instance *inst, int animIndex);
int VehFrameInst_GetStartFrame(int animIndex, int numFrames);

// JitPool

void JitPool_Clear(struct JitPool *AP);
void JitPool_Init(struct JitPool *AP, int maxItems, int itemSize, char *name);
int JitPool_Add(struct JitPool *AP);
void JitPool_Remove(struct JitPool *AP, struct Item *item);

// LevInstDef

void LevInstDef_UnPack(struct mesh_info *mInfo);
void LevInstDef_RePack(struct mesh_info *mInfo, int param_2);

// LHMatrix?

void LHMatrix_Parent(struct Instance *inst, struct Instance *driverInst, SVECTOR *param_3);

// LibraryOfModels

void LibraryOfModels_Store(struct GameTracker *gGT, u32 numModels, struct Model **ptrModelArray);
void LibraryOfModels_Clear(struct GameTracker *gGT);

// LinkedCollide

struct Instance *LinkedCollide_Radius(struct Instance *objInst, struct Thread *_objTh, struct Thread *thBucket, u32 hitRadius);
struct Instance *LinkedCollide_Hitbox_Desc(struct HitboxDesc *objBoxDesc);
struct Instance *LinkedCollide_Hitbox(struct Instance *objInst, struct Thread *_objTh, struct Thread *thBucket, struct BoundingBox bbox);

// LIST

void LIST_Clear(struct LinkedList *L);
void LIST_AddFront(struct LinkedList *L, struct Item *I);
void LIST_AddBack(struct LinkedList *L, struct Item *I);
void *LIST_GetNextItem(struct Item *I);
void *LIST_GetFirstItem(struct LinkedList *L);
struct Item *LIST_RemoveMember(struct LinkedList *L, struct Item *I);
struct Item *LIST_RemoveFront(struct LinkedList *L);
struct Item *LIST_RemoveBack(struct LinkedList *L);
void LIST_Init(struct LinkedList *L, struct Item *item, int itemSize, int numItems);

// LOAD

void LOAD_Callback_Overlay_Generic(struct LoadQueueSlot *lqs);
void LOAD_Callback_Overlay_230(void);
void LOAD_Callback_Overlay_231(void);
void LOAD_Callback_Overlay_232(void);
void LOAD_Callback_Overlay_233(void);
void LOAD_Callback_MaskHints3D(struct LoadQueueSlot *lqs);
void LOAD_Callback_Podiums(struct LoadQueueSlot *lqs);
void LOAD_Callback_LEV(struct LoadQueueSlot *lqs);
void LOAD_Callback_PatchMem(struct LoadQueueSlot *lqs);
void LOAD_Callback_DriverModels(struct LoadQueueSlot *lqs);
void LOAD_HubCallback(struct LoadQueueSlot *lqs);
void LOAD_GlobalModelPtrs_MPK(void);
void LOAD_HubSwapPtrs(struct GameTracker *gGT);
void LOAD_StringToUpper(char *path);
void LOAD_InitCD(void);
void *LOAD_ReadDirectory(char *filepath);
void LOAD_DramFileCallback(struct LoadQueueSlot *lqs);
// LOAD_DramFile()
void LOAD_VramFileCallback(struct LoadQueueSlot *lqs);
void *LOAD_VramFile(void *bigfilePtr, int subfileIndex, void *ptrDestination, int *sizePtr, int callbackOrFlags);
void LOAD_ReadFileASyncCallback(CdlIntrResult result, u8 *unk);
void *LOAD_ReadFile_ex(u32 loadType, int subfileIndex, void *ptrDst);
void *LOAD_XnfFile(char *file, void *addr, int *size);
int LOAD_FindFile(char *filename, CdlFILE *cdlFile);

// these are the last howl functions ever

int LOAD_HowlHeaderSectors(CdlFILE *cdlFileHWL, void *ptrDestination, int firstSector, int numSector);
void LOAD_HowlCallback(CdlIntrResult result, u8 *unk);
int LOAD_HowlSectorChainStart(CdlFILE *cdlFileHWL, void *ptrDestination, int firstSector, int numSector);
int LOAD_HowlSectorChainEnd(void);

// more LOAD

void LOAD_RunPtrMap(char *file, int *map, int size);
int LOAD_Robots2P(int p1, int p2);
void LOAD_Robots1P(int characterID);
void LOAD_DriverMPK(u32 bigfile, int levelLOD);
void LOAD_LangFile(int bigfilePtr, int lang);
int LOAD_GetBigfileIndex(u32 levelID, int lod);
void LOAD_AppendQueue(struct BigHeader *bigfile, int type, int fileIndex, void *destinationPtr, void (*callback)(struct LoadQueueSlot *));
void LOAD_CDRequestCallback(struct LoadQueueSlot *lqs);
void LOAD_NextQueuedFile(void);
void LOAD_Hub_ReadFile(struct BigHeader *bigfilePtr, int levID, int packID);
void LOAD_Hub_SwapNow();
void LOAD_Hub_Main(struct BigHeader *bigfilePtr);
void LOAD_OvrLOD(u32 param_1);
void LOAD_OvrEndRace(u32 param_1);
void LOAD_OvrThreads(u32 param_1);
int LOAD_GetAdvPackIndex(void);
int LOAD_TenStages(struct GameTracker *gGT, int loadingStage, struct BigHeader *bigfile);
void LOAD_TalkingMask(int, int);
void LOAD_LevelFile(int levelID);
int LOAD_IsOpen_RacingOrBattle(void);
int LOAD_IsOpen_MainMenu(void);
int LOAD_IsOpen_AdvHub(void);
int LOAD_IsOpen_Podiums(void);

// MainDB

u32 MainDB_GetClipSize(u32 levelID, int numPlyrCurrGame);
void MainDB_PrimMem(struct PrimMem *primMem, u32 size);
void MainDB_OTMem(struct OTMem *otMem, u32 size);

// MainDrawCb

void MainDrawCb_DrawSync(void);
void MainDrawCb_Vsync(void);

// MainFrame

void MainFrame_TogglePauseAudio(int bool_pause);
void MainFrame_ResetDB(struct GameTracker *gGT);
void MainFrame_GameLogic(struct GameTracker *gGT, struct GamepadSystem *gGamepads);
void MainFrame_VisMemFullFrame(struct GameTracker *gGT, struct Level *level);
void MainFrame_InitVideoSTR(u32 boolPlayVideoStr, RECT *r, s16 posX, s16 posY);
int MainFrame_HaveAllPads(s16 numPlyrNextGame);
void MainFrame_RequestMaskHint(s16 hintId, char interruptWarpPad);
void MainFrame_RenderFrame(struct GameTracker *gGT, struct GamepadSystem *gGamepads);

// MainFreeze

void MainFreeze_ConfigDrawNPC105(s16 startX, s16 startY, s16 param_3, int param_4, s16 param_5, char *color, u_long *otMem, struct PrimMem *primMem);
void MainFreeze_ConfigDrawArrows(s16 posX, s16 posY, char *str);
void MainFreeze_ConfigSetupEntry();
void MainFreeze_MenuPtrOptions(struct RectMenu *menu);
void MainFreeze_MenuPtrQuit(struct RectMenu *menu);
void MainFreeze_SafeAdvDestroy(void);
void MainFreeze_MenuPtrDefault(struct RectMenu *menu);
struct RectMenu *MainFreeze_GetMenuPtr();
void MainFreeze_IfPressStart(void);

// MainGameEnd

void MainGameEnd_SoloRaceGetReward(int subtractTimeCrateBonus);
void MainGameEnd_SoloRaceSaveHighScore(void);
// void MainGameEnd_Initialize(s16);

// MainGameStart (?)

// MainGameStart_Initialize()

// MainInit

void MainInit_VisMem(struct GameTracker *gGT);
// MainInit_RainBuffer()
void MainInit_PrimMem(struct GameTracker *gGT);
void MainInit_JitPoolsReset(struct GameTracker *gGT);
void MainInit_OTMem(struct GameTracker *gGT);
void MainInit_JitPoolsNew(struct GameTracker *gGT);
void MainInit_Drivers(struct GameTracker *gGT);
void MainInit_FinalizeInit(struct GameTracker *gGT);
int MainInit_StringToLevID(char *str);
void MainInit_VRAMClear(void);
void MainInit_VRAMDisplay(void);

// MainKillGame

// MainKillGame_StopCTR()
void MainKillGame_LaunchSpyro2(void);

// MainLoadVLC

// MainLoadVLC_Callback()
void MainLoadVLC(void);

// main

#ifdef CTR_NATIVE
u32 CTR_Main(void);
#else
u32 main(void);
#endif

// MainRaceTrack

void MainRaceTrack_StartLoad(s16 levelID);
void MainRaceTrack_RequestLoad(s16 levelID);

// MainStats

void MainStats_ClearBattleVS();
void MainStats_RestartRaceCountLoss();

// MATH

int MATH_Sin(u32 angle);
int MATH_Cos(u32 angle);
int MATH_FastSqrt(int dist, int unk);
MATRIX *MATH_HitboxMatrix(MATRIX *output, MATRIX *input);
s32 MATH_VectorLength(SVec3 *input);
s32 MATH_VectorNormalize(SVec3 *input);
void MATH_MatrixMul(MATRIX *output, MATRIX *input, VECTOR *rotate);

// MEMCARD

void MEMCARD_SetIcon(int iconID);
u32 MEMCARD_CRC16(u32 crc, int nextByte);
int MEMCARD_ChecksumSave(u8 *saveBytes, int len);
int MEMCARD_ChecksumLoad(u8 *saveBytes, int len);
char *MEMCARD_StringInit(int slotIndex, char *dstString);
void MEMCARD_StringSet(char *dstString, int slotIdx, char *srcString);
void MEMCARD_InitCard(void);
void MEMCARD_CloseCard(void);
u8 MEMCARD_GetNextSwEvent(void);
u8 MEMCARD_GetNextHwEvent(void);
u8 MEMCARD_WaitForHwEvent(void);
void MEMCARD_SkipEvents(void);
int MEMCARD_NewTask(int slotIdx, char *name, u8 *ptrMemcard, int memcardFileSize, int flags);
void MEMCARD_CloseFile(void);
int MEMCARD_ReadFile(int start_offset, int end_offset);
u8 MEMCARD_WriteFile(int start_offset, const u8 *data, int end_offset);
void MEMCARD_GetFreeBytes(int slotIdx);
u8 MEMCARD_GetInfo(int slotIdx);
// MEMCARD_Load()
// MEMCARD_Save()
u8 MEMCARD_Format(int slotIdx);
int MEMCARD_IsFile(int slotIdx, char *save_name);
char *MEMCARD_FindFirstGhost(int slotIdx, char *srcString);
char *MEMCARD_FindNextGhost(void);
u8 MEMCARD_EraseFile(int slotIdx, char *srcString);

// MEMPACK

void MEMPACK_Init(int ramSize);
void MEMPACK_SwapPacks(int index);
void MEMPACK_NewPack(void *start, int size);
int MEMPACK_GetFreeBytes(void);
void *MEMPACK_AllocMem(int allocSize);
void *MEMPACK_AllocHighMem(int allocSize);
void MEMPACK_ClearHighMem(void);
void *MEMPACK_ReallocMem(int allocSize);
int MEMPACK_PushState(void);
void MEMPACK_ClearLowMem(void);
void MEMPACK_PopState(void);
void MEMPACK_PopToState(int id);

int MixRNG_Scramble(void);
int MixRNG_Particles(int numParticle);
u32 MixRNG_GetValue(int val);

// Particle

// Particle_FuncPtr_PotionShatter()
void Particle_FuncPtr_SpitTire();
// Particle_FuncPtr_ExhaustUnderwater()
// Particle_OnDestroy()
// Particle_UpdateList()
void Particle_UpdateAllParticles();
// Particle_BitwiseClampByte()
// Particle_SetColors()
void Particle_RenderList(struct PushBuffer *pb, void *particleList);
struct Particle *Particle_Init(u32 param_1, struct IconGroup *ig, struct ParticleEmitter *pe);

// PickupBots (?)

// PickupBots_Init()
void PickupBots_Update();

// PlayLevel (?)

void PlayLevel_UpdateLapStats(void);
void RR_EndEvent_UnlockAward(void);

// Tawna (?)

void Podium_InitModels(struct GameTracker *gGT);

// THREAD

void PROC_DestroyTracker(struct Thread *t);
void PROC_DestroyInstance(struct Thread *t);
void PROC_DestroyObject(void *object, int threadFlags);
void PROC_DestroySelf(struct Thread *t);
void PROC_DestroyBloodline(struct Thread *t);
void PROC_CheckBloodlineForDead(struct Thread **replaceSelf, struct Thread *th);
void PROC_CheckAllForDead(void);
struct Thread *PROC_BirthWithObject(int creationFlags, void *behaviorFuncPtr, char *debugName,
                                    struct Thread *threadRelative); // 2nd param function ptr, maybe (void (*)(int))?
void PROC_CollidePointWithSelf(struct Thread *th, struct BucketSearchParams *buf);
void PROC_CollidePointWithBucket(struct Thread *th, s16 *vec3_pos);
struct Thread *PROC_SearchForModel(struct Thread *th, s16 modelID);
void PROC_PerBspLeaf_CheckInstances(struct BSP *bspLeaf, struct ScratchpadStruct *sps);
void PROC_StartSearch_Self(struct ScratchpadStruct *sps);
void PROC_CollideHitboxWithBucket(struct Thread *collThread, struct ScratchpadStruct *sps, struct Thread *ignoredThread);

// PushBuffer

void PushBuffer_Init(struct PushBuffer *pb, int id, int total);
void PushBuffer_SetPsyqGeom(struct PushBuffer *pb);
void PushBuffer_SetDrawEnv_DecalMP(void *ot, struct DB *backBuffer, RECT *viewport, s16 offsetX, s16 offsetY, u8 dtd, u8 dfe, u8 isbg, u8 tpageUpper,
                                   u8 tpageLower);
void PushBuffer_SetDrawEnv_Normal(void *ot, struct PushBuffer *pb, struct DB *backBuffer, s16 *param_4, int isbg);
void PushBuffer_SetMatrixVP(struct PushBuffer *pb);
int PushBuffer_SetFrustumPlane(s16 *frustumData, struct FrustumCornerOUT *fc1, s16 *camPos, struct FrustumCornerOUT *fc2);
void PushBuffer_UpdateFrustum(struct PushBuffer *pb);
void PushBuffer_FadeOneWindow(struct PushBuffer *pb);
void PushBuffer_FadeAllWindows(void);

// QueueLoadTrack

// QueueLoadTrack_MenuProc()
// QueueLoadTrack_GetMenuPtr()

// RaceConfig

void RaceConfig_LoadGameOptions(void);
void RaceConfig_SaveGameOptions(void);

// TitleFlag

int RaceFlag_MoveModels(int frameIndex, int numFrames);
int RaceFlag_IsFullyOnScreen(void);
int RaceFlag_IsFullyOffScreen(void);
int RaceFlag_IsTransitioning(void);
void RaceFlag_SetDrawOrder(int drawOrder);
void RaceFlag_BeginTransition(int direction);
void RaceFlag_SetFullyOnScreen(void);
void RaceFlag_SetFullyOffScreen(void);
void RaceFlag_SetCanDraw(s16 enable);
int RaceFlag_GetCanDraw(void);
u32 *RaceFlag_GetOT(void);
void RaceFlag_ResetTextAnim(void);
void RaceFlag_DrawLoadingString(void);
void RaceFlag_DrawSelf(void);

// RECTMENU
void RECTMENU_DrawOuterRect_Edge(RECT *r, Color color, u32 param_3, u_long *otMem);
u8 *RECTMENU_DrawTime(int milliseconds);
void RECTMENU_DrawRwdBlueRect_Subset(s16 *pos, int *color, u_long *ot, struct PrimMem *primMem);
void RECTMENU_DrawRwdBlueRect(RECT *rect, char *metas, u_long *ot, struct PrimMem *primMem);
void RECTMENU_DrawRwdTriangle(s16 *position, char *color, u_long *otMem, struct PrimMem *primMem);
void RECTMENU_DrawOuterRect_LowLevel(RECT *r, s16 x, u16 y, Color color, s16 param_5, u_long *otMem);
void RECTMENU_DrawOuterRect_HighLevel(RECT *r, Color color, s16 param_3, u_long *otMem);
void RECTMENU_DrawQuip(char *comment, s16 startX, int startY, u32 sizeX, s16 fontType, int textFlag, s16 boxFlag);
void RECTMENU_DrawInnerRect(RECT *r, int flag, u_long *ot);
void RECTMENU_DrawFullRect(struct RectMenu *menu, RECT *r);
void RECTMENU_GetHeight(struct RectMenu *m, s16 *height, int boolCheckSubmenu);
void RECTMENU_GetWidth(struct RectMenu *m, s16 *width, int boolCheckSubmenu);
void RECTMENU_DrawSelf(struct RectMenu *m, int posX, s16 posY, s16 menuWidth);
void RECTMENU_ClearInput(void);
void RECTMENU_CollectInput(void);
int RECTMENU_ProcessInput(struct RectMenu *m);
void RECTMENU_ProcessState(void);
void RECTMENU_Show(struct RectMenu *m);
void RECTMENU_Hide(struct RectMenu *m);
int RECTMENU_BoolHidden(struct RectMenu *m);

// GhostData (?)

int RefreshCard_BoolGhostForLEV(u16 trackID);
int RefreshCard_GetResult(int result);
void RefreshCard_NextMemcardAction(int slot, int action, char *fileName, char *fileIconHeader, struct GhostHeader *ptrGhostHeader, int fileSize);
u32 RefreshCard_GhostEncodeByte(int currByte);
int RefreshCard_GhostDecodeByte(int value);
void RefreshCard_GhostEncodeProfile(u32 slotIndex, u16 characterID, u16 levelID, int time, char *name);
void RefreshCard_GhostDecodeProfile(struct GhostProfile *profile, char *fileName);
void RefreshCard_StartMemcardAction(int action);
void RefreshCard_StopMemcardAction(void);
void RefreshCard_SetScreenText(int screenText);
void RefreshCard_Unknown2(void);
void RefreshCard_GetNumGhostsTotal(void);
void RefreshCard_GameProgressAndOptions(void);

// Load...

void RefreshCard_Entry(void);
void SelectProfile_QueueLoadHub_MenuProc(struct RectMenu *menu);
void SelectProfile_ThTick(struct Thread *t);
void SelectProfile_PrintInteger(int value, int posX, int posY, int usePaddedFormat, int color);
int SelectProfile_UI_ConvertX(int param_1, int param_2);
int SelectProfile_UI_ConvertY(int param_1, int param_2);
// SelectProfile_DrawAdvProfile()
void SelectProfile_GetTrackID(void);
void SelectProfile_Init(u16 flags);
void SelectProfile_Destroy(void);
void SelectProfile_AdvPickMode_MenuProc(struct RectMenu *menu);
// SelectProfile_DrawGhostProfile()
void SelectProfile_MuteCursors(void);
void SelectProfile_UnMuteCursors(void);
void SelectProfile_ToggleMode(u32 param_1);
void SelectProfile_InitAndDestroy(void);
u32 SelectProfile_InputLogic(struct RectMenu *menu, s16 param_2, u32 confirm);
// SelectProfile_AllProfiles_MenuProc()

// SubmitName

void SubmitName_RestoreName(s16 param_1);
s16 SubmitName_DrawMenu(u16 stringID);
void SubmitName_MenuProc(struct RectMenu *menu);

// PrixSaveBox (?)

void TakeCupProgress_Activate(s16 stringIndex);
void TakeCupProgress_MenuProc(struct RectMenu *menu);

// RCNT

void Timer_Init(void);
// Timer_Destroy()
int Timer_GetTime_Total();
int Timer_GetTime_Elapsed(int time, int *param_2);

// Torch

void Torch_Main(void *particleList_heatWarp, struct PushBuffer *pb, struct PrimMem *primMem, char numPlyr, int swapchainIndex);

// UI

void UI_SaveLapTime(int lapID, int time, s16 driverID);
void UI_ThTick_CountPickup(struct Thread *);
void UI_ThTick_Reward(struct Thread *);
void UI_ThTick_CtrLetters(struct Thread *);
void UI_ThTick_big1(struct Thread *);
int UI_ConvertX_2(int oldPosX, int newPosX);
int UI_ConvertY_2(int oldPosY, int newPosY);
// UI_INSTANCE_BirthWithThread()
void UI_INSTANCE_InitAll(void);
void UI_Map_DrawMap(struct Icon *mapTop, struct Icon *mapBottom, s16 posX, s16 posY, struct PrimMem *primMem, u_long *otMem, u32 colorID);
void UI_Map_GetIconPos(s16 *ptrMap, int *posX, int *posY);
void UI_Map_DrawAdvPlayer(int ptrMap, int *matrix, int unused1, int unused2, s16 param_5, s16 param_6);
void UI_Map_DrawRawIcon(int ptrMap, int *param_2, int iconID, int colorID, int unused, s16 scale);
void UI_Map_DrawDrivers(int ptrMap, struct Thread *bucket, s16 *param_3);
void UI_Map_DrawGhosts(int ptrMap, struct Thread *bucket);
void UI_Map_DrawTracking(int ptrMap, struct Thread *bucket);
void UI_WeaponBG_AnimateShine();
void UI_WeaponBG_DrawShine(struct Icon *icon, s16 posX, s16 posY, struct PrimMem *primMem, u_long *ot, char transparency, s16 angleX, s16 angleY,
                           int unusedColor);
void UI_TrackerBG(struct Icon *targetIcon, s16 centerX, s16 centerY, struct PrimMem *primMem, u_long *ot, char transparency, s16 angleX, s16 angleY, int color);
void UI_DrawDriverIcon(struct Icon *icon, s16 posX, s16 posY, struct PrimMem *primMem, u_long *ot, char transparency, s16 scale, u32 color);
void UI_Lerp2D_Angular(s16 *ptrPos, s16 drawnPosition, s16 absolutePosition, s16 frameCounter);
void UI_Lerp2D_HUD(s16 *ptrPos, s16 startX, s16 startY, s16 endX, s16 endY, int curFrame, s16 endFrame);
void UI_Lerp2D_Linear(s16 *ptrPos, s16 startX, s16 startY, s16 endX, s16 endY, int curFrame, s16 endFrame);
void UI_DrawRaceClock(u16 paramX, u16 paramY, u32 flags, struct Driver *driver);
void UI_DrawLimitClock(s16 posX, s16 posY, s16 fontType);
void UI_BattleDrawHeadArrows(struct Driver *);
void UI_TrackerSelf(struct Driver *);
void UI_DrawPosSuffix(s16 posX, s16 posY, struct Driver *d, s16 flags);
void UI_DrawLapCount(s16 posX, int posY, int param_3, struct Driver *d);
void UI_DrawBattleScores(int, int, struct Driver *);
void UI_Weapon_DrawSelf(s16, s16, s16, struct Driver *);
void UI_Weapon_DrawBG(s16 param_1, s16 param_2, s16 param_3, struct Driver *d);
void UI_DrawNumWumpa(s16 posX, s16 posY, struct Driver *d);
void UI_DrawNumTimebox(int posX, int posY, struct Driver *d);
void UI_DrawNumRelic(int posX, int posY);
void UI_DrawNumKey(int posX, int posY);
void UI_DrawNumTrophy(int posX, int posY);
void UI_DrawNumCrystal(int posX, int posY, struct Driver *d);
void UI_DrawSpeedNeedle(s16 posX, s16 posY, struct Driver *driver);
void UI_DrawSpeedBG(void);
void UI_JumpMeter_Update(struct Driver *d);
void UI_JumpMeter_Draw(s16 posX, s16 posY, struct Driver *driver);
void UI_DrawSlideMeter(s16 posX, s16 posY, struct Driver *driver);
void UI_DrawRankedDrivers(void);
void UI_RenderFrame_Racing(void);
void UI_RenderFrame_AdvHub(void);
void UI_RenderFrame_CrystChall(void);
void UI_RenderFrame_Wumpa3D_2P3P4P(struct GameTracker *gGT);
u32 UI_VsQuipReadDriver(struct Driver *driver, int offset, int size);
void UI_VsQuipAssign(struct Driver *driver, struct QuipMeta *meta, struct Driver *bestDriver, int characterID);
void UI_VsQuipAssignAll(void);
void UI_VsQuipDrawAll();
void UI_VsWaitForPressX();
void UI_RaceEnd_GetDriverClock(struct Driver *driver);
void UI_RaceStart_IntroText1P();
void UI_RaceEnd_MenuProc();
void UI_CupStandings_FinalizeCupRanks(void);
void UI_CupStandings_UpdateCupRanks(void);
void UI_CupStandings_InputAndDraw(void);

// Vector

// Vector_SpecLightSpin2D()
// Vector_SpecLightSpin3D()
// Vector_SpecLightNoSpin3D()
void Vector_BakeMatrixTable();

// from this point onwards it's all vehicle stuff

struct Scrub *VehAfterColl_GetSurface(u32 scrubId);
// VehAfterColl_GetTerrain()

// VehInit

// VehBirth_TeleportSelf()
void VehBirth_TeleportAll(struct GameTracker *gGT, u32 spawnFlags);
struct Model *VehBirth_GetModelByName(char *searchName);
void VehBirth_SetConsts(struct Driver *driver);
// VehBirth_EngineAudio_AllPlayers()
// VehBirth_TireSprites()
void VehBirth_NonGhost(struct Thread *t, int index);
struct Driver *VehBirth_Player(int index);

// uncategorized

int VehCalc_InterpBySpeed(int currentRot, int rotSpeed, int destinedRot);
int VehCalc_MapToRange(int param_1, int param_2, int param_3, int param_4, int param_5);
int VehCalc_SteerAccel(int param_1, int param_2, int param_3, int param_4, int param_5, int param_6);
// VehCalc_FastSqrt()

// VehParticle

struct Particle *VehEmitter_Exhaust(struct Driver *driver, VECTOR *pos, VECTOR *vel);
void VehEmitter_Sparks_Ground(struct Driver *driver, struct ParticleEmitter *emSet);
void VehEmitter_Terrain_Ground(struct Driver *driver, struct ParticleEmitter *emSet);
void VehEmitter_Sparks_Wall(struct Driver *driver, struct ParticleEmitter *emSet);
void VehEmitter_DriverMain(struct Thread *thread, struct Driver *driver);

// more uncategorized vehicle stuff

void VehFire_Audio(struct Driver *driver, int speed_cap);
void VehFire_Increment(struct Driver *driver, int reserves, u32 type, int fireLevel);
// VehFrameInst_GetStartFrame()
// VehFrameInst_GetNumAnimFrames()
void VehFrameProc_Driving(struct Thread *t, struct Driver *d);
void VehFrameProc_Spinning(struct Thread *t, struct Driver *d);
void VehFrameProc_LastSpin(struct Thread *t, struct Driver *d);
bool VehGroundShadow_Subset1(struct TextureLayout *pDst, int iconIndex);
void VehGroundShadow_Main(void);
// VehGroundSkids_Subset1()
// VehGroundSkids_Subset2()
void VehGroundSkids_Main(struct Thread *thread, struct PushBuffer *pb);
// VehLap_UpdateProgress()
void VehPhysCrash_ConvertVecToSpeed(struct Driver *d, Vec3 *v);
int VehPhysCrash_BounceSelf(s16 *normal, Vec3 *origin, Vec3 *vel, int boolOtherDriver);
void VehPhysCrash_AI(struct Driver *bot, Vec3 *vel);
int VehPhysCrash_Attack(struct Driver *driver1, struct Driver *driver2, int canPlayFeedback, int boolPlayBubblePop);
void VehPhysCrash_AnyTwoCars(struct Thread *thread, u16 *searchWords, Vec3 *selfVel);
void VehPhysForce_ConvertSpeedToVecOut(struct Driver *d, Vec3 *vel);
// VehPhysForce_OnGravity()
void VehPhysForce_OnApplyForces(struct Thread *t, struct Driver *d);
void VehPhysForce_CollideDrivers(struct Thread *t, struct Driver *d);
void VehPhysForce_TranslateMatrix(struct Thread *thread, struct Driver *driver);
void VehPhysForce_RotAxisAngle(MATRIX *m, s16 *normVec, s16 angle);
void VehPhysForce_AccelTerrainSlope(struct Driver *d);

// "VehPtr"
void VehPhysGeneral_PhysAngular(struct Thread *thread, struct Driver *driver);
// VehPhysGeneral_LerpQuarterStrength()
int VehPhysGeneral_LerpToForwards(struct Driver *driver, int param_2, int param_3, int param_4);
int VehPhysGeneral_JumpGetVelY(s16 *normalVec, Vec3 *speedXYZ);
void VehPhysGeneral_JumpAndFriction(struct Thread *thread, struct Driver *driver);
void VehPhysGeneral_SetHeldItem(struct Driver *driver);
int VehPhysGeneral_GetBaseSpeed(struct Driver *driver);
int VehPhysJoystick_ReturnToRest(int param_1, u32 param_2, struct RacingWheelData *param_3);
// void VehPhysJoystick_GetStrength()
int VehPhysJoystick_GetStrengthAbsolute(int param_1, u32 param_2, struct RacingWheelData *param_3);
void VehPhysProc_Driving_PhysLinear(struct Thread *t, struct Driver *d);
void VehPhysProc_Driving_Audio(struct Thread *t, struct Driver *d);
void VehPhysProc_Driving_Update(struct Thread *t, struct Driver *d);
void VehPhysProc_Driving_Init(struct Thread *t, struct Driver *d);
void VehPhysProc_FreezeEndEvent_PhysLinear(struct Thread *t, struct Driver *d);
void VehPhysProc_FreezeEndEvent_Init(struct Thread *, struct Driver *);
void VehPhysProc_FreezeVShift_Update(struct Thread *t, struct Driver *d);
void VehPhysProc_FreezeVShift_ReverseOneFrame(struct Thread *t, struct Driver *d);
void VehPhysProc_FreezeVShift_Init(struct Thread *t, struct Driver *d);
void VehPhysProc_PowerSlide_PhysAngular(struct Thread *th, struct Driver *driver);
void VehPhysProc_PowerSlide_Finalize(struct Driver *d);
void VehPhysProc_PowerSlide_Update(struct Thread *t, struct Driver *d);
void VehPhysProc_PowerSlide_PhysLinear(struct Thread *thread, struct Driver *driver);
void VehPhysProc_PowerSlide_InitSetUpdate(struct Thread *t, struct Driver *d);
void VehPhysProc_PowerSlide_Init(struct Thread *t, struct Driver *d);
void VehPhysProc_SlamWall_PhysAngular(struct Thread *t, struct Driver *d);
void VehPhysProc_SlamWall_Update(struct Thread *t, struct Driver *d);
void VehPhysProc_SlamWall_PhysLinear(struct Thread *t, struct Driver *d);
void VehPhysProc_SlamWall_Animate(struct Thread *t, struct Driver *d);
void VehPhysProc_SlamWall_Init(struct Thread *t, struct Driver *d);
void VehPhysProc_SpinFirst_Update(struct Thread *t, struct Driver *d);
void VehPhysProc_SpinFirst_PhysLinear(struct Thread *t, struct Driver *d);
void VehPhysProc_SpinFirst_PhysAngular(struct Thread *t, struct Driver *d);
void VehPhysProc_SpinFirst_InitSetUpdate(struct Thread *t, struct Driver *d);
void VehPhysProc_SpinFirst_Init(struct Thread *t, struct Driver *d);
void VehPhysProc_SpinLast_Update(struct Thread *t, struct Driver *d);
void VehPhysProc_SpinLast_PhysLinear(struct Thread *t, struct Driver *d);
void VehPhysProc_SpinLast_PhysAngular(struct Thread *t, struct Driver *d);
void VehPhysProc_SpinLast_Init(struct Thread *t, struct Driver *d);
void VehPhysProc_SpinStop_Update(struct Thread *t, struct Driver *d);
void VehPhysProc_SpinStop_PhysLinear(struct Thread *t, struct Driver *d);
void VehPhysProc_SpinStop_PhysAngular(struct Thread *t, struct Driver *d);
void VehPhysProc_SpinStop_Animate(struct Thread *t, struct Driver *d);
void VehPhysProc_SpinStop_Init(struct Thread *t, struct Driver *d);
// VehPickState_NewState()

// Weapon (?)

int VehPickupItem_MaskBoolGoodGuy(struct Driver *d);
struct MaskHeadWeapon *VehPickupItem_MaskUseWeapon(struct Driver *driver, int boolPlaySound);
struct Driver *VehPickupItem_MissileGetTargetDriver(struct Driver *driver);
u32 VehPickupItem_PotionThrow(struct MineWeapon *mine, struct Instance *inst, u32 flags);
void VehPickupItem_ShootNow(struct Driver *d, int weaponID, int flags);
void VehPickupItem_ShootOnCirclePress(struct Driver *d);

// More VehPtr

void VehStuckProc_MaskGrab_FindDestPos(struct Driver *d, struct QuadBlock *quad);
void VehStuckProc_MaskGrab_Particles(struct Driver *d);
void VehStuckProc_MaskGrab_Update(struct Thread *t, struct Driver *d);
void VehStuckProc_MaskGrab_PhysLinear(struct Thread *t, struct Driver *d);
void VehStuckProc_MaskGrab_Animate(struct Thread *t, struct Driver *d);
void VehStuckProc_MaskGrab_Init(struct Thread *t, struct Driver *d);
void VehStuckProc_PlantEaten_Update(struct Thread *t, struct Driver *d);
void VehStuckProc_PlantEaten_PhysLinear(struct Thread *t, struct Driver *d);
void VehStuckProc_PlantEaten_Animate(struct Thread *t, struct Driver *d);
void VehStuckProc_PlantEaten_Init(struct Thread *t, struct Driver *d);
void VehStuckProc_RIP_Init(struct Thread *t, struct Driver *d);
void VehStuckProc_RevEngine_Update(struct Thread *t, struct Driver *d);
void VehStuckProc_RevEngine_PhysLinear(struct Thread *t, struct Driver *d);
void VehStuckProc_RevEngine_Animate(struct Thread *t, struct Driver *d);
void VehStuckProc_RevEngine_Init(struct Thread *t, struct Driver *d);
void VehStuckProc_Tumble_Update(struct Thread *t, struct Driver *d);
void VehStuckProc_Tumble_PhysLinear(struct Thread *t, struct Driver *d);
void VehStuckProc_Tumble_PhysAngular(struct Thread *t, struct Driver *d);
void VehStuckProc_Tumble_Animate(struct Thread *t, struct Driver *d);
void VehStuckProc_Tumble_Init(struct Thread *t, struct Driver *d);
void VehStuckProc_Warp_MoveDustPuff(struct Thread *t, struct Driver *d);
void VehStuckProc_Warp_AddDustPuff1(struct ScratchpadStruct *sps);
void VehStuckProc_Warp_AddDustPuff2(struct Thread *t, struct Driver *d);
void VehStuckProc_Warp_PhysAngular(struct Thread *t, struct Driver *d);
void VehStuckProc_Warp_Init(struct Thread *t, struct Driver *d);

// TalkingMask (?)

// VehTalkMask_ThTick()
// VehTalkMask_Init()
// VehTalkMask_PlayXA()
// VehTalkMask_boolNoXA()
// VehTalkMask_End()

// More Turbo...

void VehTurbo_ProcessBucket(struct Thread *turboThread);
void VehTurbo_ThDestroy(struct Thread *t);
void VehTurbo_ThTick(struct Thread *t);

// DrawSky (?)

void DrawSky_Full(void *skybox, struct PushBuffer *pb, struct PrimMem *primMem);
// DrawSky_Piece()

// uncategorized

void AnimateQuad(int timer, int numSCVert, void *ptrSCVert, int *visSCVertList);
void AnimateQuadVertex(int timer, struct SCVert *scVert, u32 *visBits);
// TRIG_AngleSinCos_r19r17r18()
void DrawConfetti(struct PushBuffer *pb, struct PrimMem *primMem, void *confetti, int frameTimer, int gameMode1);
// TRIG_AngleSinCos_r15r16r17()

// RenderBucket (?)

// RenderBucket_DrawFunc_Normal()
// RenderBucket_UncompressAnimationFrame()
void RenderBucket_Execute(void *param_1, struct PrimMem *param_2);
// RenderBucket_DrawInstPrim_Normal()
// RenderBucket_InitDepthGTE()
// RenderBucket_DrawInstPrim_KeyRelicToken()
// RenderBucket_DrawFunc_Split()

// more uncategorized stuff

// Draw_Uncompress2()
// Draw_KartFlames()
// Draw_KartInWaterMud()
void ConvertRotToMatrix_InverseTranspose_NoRotY(MATRIX *m, s16 *rot);
void ConvertRotToMatrix_InverseTranspose(MATRIX *m, s16 *rot);
void ConvertRotToMatrix(MATRIX *m, s16 *rot);
void ConvertRotToMatrix_Transpose(MATRIX *m, s16 *rot);
void MatrixRotate(MATRIX *dst, MATRIX *src, MATRIX *rot);
// TRIG_AngleSinCos_r16r17r18_duplicate()
int SquareRoot0_stub(int param_1);
// RngDeadCoed()
VECTOR *ApplyMatrixLV_stub(VECTOR *input, VECTOR *output);
// Draw_KartGhost()
// Draw_KartBodyReflection()
// RenderBucket_DrawInstPrim_Ghost()
void AnimateWater1P(int timer, int numWaterVertices, struct WaterVert *waterVert, void *waterEnvMap, int *param_5);
void AnimateWater2P(int timer, int numWaterVertices, struct WaterVert *waterVert, void *waterEnvMap, int *param_5, int *param_6);
void AnimateWater3P(int timer, int numWaterVertices, struct WaterVert *waterVert, void *waterEnvMap, int *param_5, int *param_6, int *param_7);
void AnimateWater4P(int timer, int numWaterVertices, struct WaterVert *waterVert, void *waterEnvMap, int *param_5, int *param_6, int *param_7, int *param_8);
void AnimateWaterVertex(struct WaterVert *waterVert, u16 colorOffset, int firstOffset, int secondOffset, u32 *visBits);
void RedBeaker_RenderRain(struct PushBuffer *pb, struct PrimMem *primMem, struct JitPool *rain, char numPlyr, int gameMode1);
void RenderStars(struct PushBuffer *pb, struct PrimMem *primMem, void *stars, char numPlyr);
void DrawTires_Solid(struct Thread *thread, struct PrimMem *primMem, char numPlyr);
// TRIG_AngleSinCos_r9r8r10()
void DrawTires_Reflection(struct Thread *thread, struct PrimMem *primMem, char numPlyr);
void RenderWeather(struct PushBuffer *pb, struct PrimMem *primMem, struct RainBuffer *rainBuffer, char numPlyr, int gameMode1);
// TRIG_AngleSinCos_r16r17r18()
int RenderLists_Init1P2P(struct BSP *bspRoot, int *visLeafList, struct PushBuffer *pb, u32 LevRenderList, void *bspList, char numPlyr);
void RenderLists_PreInit();
int RenderLists_Init3P4P(struct BSP *bspRoot, int *visLeafList, struct PushBuffer *pb, u32 LevRenderList, void *bspList);
void *RenderBucket_QueueLevInstances(struct CameraDC *cDC, u_long *otMem, void *rbi, char *lod, char numPlyr, int gameMode1);
void *RenderBucket_QueueNonLevInstances(struct Item *item, u_long *otMem, void *rbi, char *lod, char numPlyr, int gameMode1);
// RenderBucket_QueueDraw()
void ThTick_RunBucket(struct Thread *thread);
void ThTick_FastRET(struct Thread *thread);
// ThTick_SetAndExec()
// ThTick_Set()

// PSY-Q

void *BreakDraw();

// ALL EXE FUNCTIONS ARE DECLARED ABOVE
// OVERLAY FUNCTIONS ARE BELOW
// (currently unorganized)

void RB_Bubbles_RoosTubes(void);
void RB_Burst_DrawAll(struct GameTracker *gGT);
void VB_EndEvent_DrawMenu();
void RR_EndEvent_DrawMenu();
void AA_EndEvent_DrawMenu();
void TT_EndEvent_DisplayTime(int paramX, s16 paramY, u32 UI_DrawRaceClockFlags);
void TT_EndEvent_DrawHighScore(s16 startX, int startY, s16 scoreMode);
void TT_EndEvent_DrawMenu(void);
void CC_EndEvent_DrawMenu();
void AH_Map_Main(void);
void AH_MaskHint_Start(s16 requestedHint, s16 bool_interruptWarppad);
void CS_BoxScene_InstanceSplitLines(void);

void RB_Player_ToggleInvisible();
void RB_Player_ToggleFlicker();

void RB_RainCloud_FadeAway(struct Thread *t);
void RB_RainCloud_ThTick(struct Thread *t);

void RB_Burst_ProcessBucket(struct Thread *thread);
void RB_Blowup_ProcessBucket(struct Thread *thread);
void RB_Spider_DrawWebs(struct Thread *thread, struct PushBuffer *pb);
void RB_Follower_ProcessBucket(struct Thread *thread);
void RB_StartText_ProcessBucket(struct Thread *thread);
void AH_WarpPad_AllWarppadNum(void);
void MM_Title_SetTrophyDPP(void);

void DrawLevelOvr1P(void *LevRenderList, struct PushBuffer *pb, struct BSP *bspList, struct PrimMem *primMem, void *VisMem10, void *waterEnvMap);
void DrawLevelOvr2P(void *LevRenderList, struct PushBuffer *pb, struct BSP *bspList, struct PrimMem *primMem, void *VisMem10, void *VisMem14,
                    void *waterEnvMap);
void DrawLevelOvr3P(void *LevRenderList, struct PushBuffer *pb, struct BSP *bspList, struct PrimMem *primMem, void *VisMem10, void *VisMem14, void *VisMem18,
                    void *waterEnvMap);
void DrawLevelOvr4P(void *LevRenderList, struct PushBuffer *pb, struct BSP *bspList, struct PrimMem *primMem, void *VisMem10, void *VisMem14, void *VisMem18,
                    void *VisMem1C, void *waterEnvMap);

u32 MM_Video_CheckIfFinished(int param_1);
void AH_Pause_Update(void);

//=====================================================================================================================
// this section is forward decls to fix warnings by TheUbMunster.
// any commented out entries were hoisted from files, then commented because they're already present in this file.
// these decls should probably be moved into the upper portion of this file & sorted at some point.
//=====================================================================================================================

// void RECTMENU_ClearInput();
// void RECTMENU_Show(struct RectMenu*);
void RECTMENU_DrawPolyGT4(struct Icon *icon, s16 posX, s16 posY, struct PrimMem *primMem, u_long *ot, u32 color0, u32 color1, u32 color2, u32 color3,
                          char transparency, s16 scale);
int ratan2(int x, int y);
void RotTrans(SVECTOR *, VECTOR *, long *);
void SetRotMatrix(MATRIX *);
void SetTransMatrix(MATRIX *);
void VehBirth_TeleportSelf(struct Driver *d, u8 spawnFlag, int spawnPosY); // this is present (but commented out) further up the file. Idk why.
void StCdInterrupt();                                                      // 80078d34
void DecDCTout(u_long *, int);                                             // 80079940 guessed the signature
void DecDCTin(u_long *, int);
void StSetStream(int, int, int, int, int); // guessed about a lot of these signatures (TheUbMunster)
void StSetRing(u_long *, int);
void StClearRing();
int CdDiskReady(int);
void StSetMask(int, int, int);
void CdDataCallback(int);
void DecDCTReset(int);
u_long DecDCTvlcSize2(u_long);
void DecDCToutCallback(void *);
#ifndef REBUILD_PS1 // this forward decl hoisted from 231_006_RB_Hazard_HurtDriver.c
int VehPickState_NewState(
    struct Driver *victim, int damageType, struct Driver *attacker,
    int reason); // this is present (but commented out) further up the file. Idk why, also, return value was void when should have been int
#endif
#ifndef REBUILD_PS1 // this forward decl hoisted from 231_011_RB_Hazard_ThCollide_Generic.c
void RB_Explosion_InitGeneric(struct Instance *inst);
#endif
void RB_GenericMine_ThTick(struct Thread *);
void RB_MakeInstanceReflective(struct ScratchpadStruct *, struct Instance *);
// void COLL_MOVED_PlayerSearch(struct Thread* thread, struct Driver* driver);
// void VehPhysForce_CollideDrivers(struct Thread* thread, struct Driver* driver);
// void COLL_FIXED_PlayerSearch(struct Thread* thread, struct Driver* driver);
void FLARE_Init(s16 *); // this is present (but commented out) further up the file. Idk why.
void RB_ShieldDark_ThTick_Grow(struct Thread *t);
void RB_Warpball_ThTick(struct Thread *t);
struct CheckpointNode *RB_Warpball_NewPathNode(struct CheckpointNode *ptrNodeCurr, struct Driver *victim);
void RB_Warpball_Start(struct TrackerWeapon *tw);
void RB_Warpball_SetTargetDriver(struct TrackerWeapon *tw);
struct Driver *RB_Warpball_GetDriverTarget(struct TrackerWeapon *tw, struct Instance *inst);
void RB_Warpball_SeekDriver(struct TrackerWeapon *tw, u32 param_2, struct Driver *d);
int RB_Hazard_HurtDriver(struct Driver *driverVictim, int damageType, struct Driver *driverAttacker, int reason);
struct Driver *VehPickupItem_MissileGetTargetDriver(struct Driver *); // this is present (but commented out) further up the file. Idk why.
u32 VehPickupItem_PotionThrow(struct MineWeapon *potion, struct Instance *inst,
                              u32 flags); // this is present (but commented out) further up the file. Idk why.
u16 RB_Hazard_CollLevInst(struct ScratchpadStruct *, struct Thread *);
void RB_GenericMine_ThDestroy(struct Thread *t, struct Instance *inst, struct MineWeapon *mw);
void RB_Player_KillPlayer(struct Driver *player_1, struct Driver *player_2);
void RB_Fruit_GetScreenCoords(struct PushBuffer *pb, struct Instance *inst, s16 *output);
void VehPhysForce_OnGravity(struct Driver *driver, Vec3 *velocity);
int RngDeadCoed(u32 *);
struct Terrain *VehAfterColl_GetTerrain(u8 terrainType);
void ThTick_SetAndExec(struct Thread *, void (*callbackMaybe)(struct Thread *)); // guessed about the signature
void RB_Default_LInB(struct Instance *);
u16 INSTANCE_GetNumAnimFrames(struct Instance *pInstance, int animIndex);
void RB_RainCloud_Init(struct Driver *d);
void RB_Explosion_InitPotion(struct Instance *inst);
void RB_Blowup_Init(struct Instance *weaponInst);
void RB_TNT_ThTick_ThrowOffHead();
void RB_Explosion_InitGeneric(struct Instance *inst);
void MatrixRotate(MATRIX *, MATRIX *, MATRIX *);
void RB_Warpball_Death(struct Thread *t);
void RB_Potion_OnShatter_TeethSearch(struct Instance *);
s16 rand(); // guessed about the signature. Also, where is the location of this function????
void Vector_SpecLightSpin3D(struct Instance *, s16 *, s16 *);
void Vector_SpecLightNoSpin3D(struct Instance *, s16 *, s16 *);
void CS_Podium_Prize_Spin(struct Instance *inst, s16 *prize);
int howl_VolumeGet(int type);
void howl_VolumeSet(int type, u8 vol);
void Vector_SpecLightSpin2D(struct Instance *, s16 *, s16 *);
char *CS_Credits_GetNextString(char *);
void CS_Credits_DestroyCreditGhost(void);
void CdSetDebug(int);
void SpuSetIRQ(int);
void SpuReadDecodedData(s16 *, int);
void SpuSetIRQCallback(void (*)(void));
void SpuSetTransferCallback(void (*)(void));
void MainInit_RainBuffer(struct GameTracker *);
void CS_Podium_FullScene_Init(void);
void CS_Cutscene_Start(void);
void howl_StopAudio(int boolErasePauseBackup, int boolEraseMusic, int boolDestroyAllFX);
u32 OtherFX_Modify(u32 soundId, u32 flags);
void RB_TNT_ThTick_ThrowOnHead(struct Thread *);
void RB_TNT_ThTick_ThrowOffHead(struct Thread *);
void RB_Potion_ThTick_InAir(struct Thread *);
void PadInitMtap(struct MultitapPacket *, struct MultitapPacket *);
void PadStartCom();
int PadSetMainMode(int, int, int);
int PadInfoAct(s16, int, int);
int PadSetAct(s16, char *, u32);
int PadSetActAlign(s16, char *);
int PadGetState(u32);
void SpuSetReverbModeDepth(s16, s16);
void SpuSetReverb(s16);
void SpuSetReverbModeParam(SpuReverbAttr *);
void SpuSetCommonCDMix(int);
void SpuSetVoiceADSRAttr(int, int, int, int, int, int, int, int, int);
int SpuSetReverbVoice(int, int);
void Level_AmbientSound();
void OtherFX_RecycleNew(u32 *soundID_Count, u32 newSoundID, u32 modifyFlags);
void LOAD_Hub_ReadFile(struct BigHeader *bigfilePtr, int levID, int packID);
void PickupBots_Init();
void SetDrawMove(DR_MOVE *, RECT *, int x, int y);
void PadStopCom();
void __main();
void SetGraphDebug(int);
void SetGeomOffset(int, int);
void SetGeomScreen(int);
void RenderBucket_InitDepthGTE();
int format(char *);
void Particle_UpdateList(int *, int);
void VehLap_UpdateProgress(struct Driver *);
void MainGameEnd_Initialize(void);
void SetDrawEnv(void *, DRAWENV *);
u32 VehCalc_FastSqrt(u32, u32);
void VehBirth_NullThread(struct Thread *t);
void SelectProfile_DrawAdvProfile(struct AdvProgress *adv, int posX, int posY, u32 isHighlighted, s16 slotIndex, u16 menuFlag);
void Seal_CheckColl(struct Instance *sealInst, struct Thread *sealTh, int damage, int radius, int sound);
void DotLights_AudioAndVideo(struct GameTracker *gGT);
