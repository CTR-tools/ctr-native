#if defined(CTR_NATIVE)
typedef enum _CdlIntrResult
{
	_CdlNoIntr = 0,
	_CdlDataReady = 1,
	_CdlComplete = 2,
	_CdlAcknowledge = 3,
	_CdlDataEnd = 4,
	_CdlDiskError = 5
} CdlIntrResult;
#endif
// typedef void (*CdlCB)(CdlIntrResult, u8*);

// Retail overlay address labels; declared as functions to preserve existing address-taking call sites.
void OVR_Region1(void);
void OVR_Region2(void);
void OVR_Region3(void);
void CS_EndOfFile(void);
void RB_EndOfFile(void);
void AH_EndOfFile(void);
void MM_EndOfFile(void);

void CAM_ClearScreen(struct GameTracker *gGT);
void CAM_Init(struct CameraDC *cDC, s32 cameraID, struct Driver *d, struct PushBuffer *pb);
int CAM_Path_GetNumPoints(void);
u8 CAM_Path_Move(int frameIndex, s16 *position, s16 *rotation, s16 *getPath);
void CAM_SetDesiredPosRot(struct CameraDC *cDC, const SVec3 *pos, const SVec3 *rot);

void BOTS_Adv_AdjustDifficulty(void);

int CDSYS_Init(b32 boolUseDisc);
u32 CDSYS_GetFilePosInt(char *fileString, int *filePos);
void CDSYS_SetMode_StreamData(void);
void CDSYS_SetMode_StreamAudio(void);
int CDSYS_SetXAToLang(int lang);
void CDSYS_XaCallbackCdSync(CdlIntrResult result, u8 *unk);  //+unk to adhere to *CdlCB
void CDSYS_XaCallbackCdReady(CdlIntrResult result, u8 *unk); //+unk to adhere to *CdlCB
void CDSYS_SpuCallbackIRQ(void);
void CDSYS_SpuCallbackTransfer(void);
void CDSYS_SpuEnableIRQ(void);
void CDSYS_SpuDisableIRQ(void);
void CDSYS_SpuGetMaxSample(void);
#if defined(CTR_NATIVE)
void CDSYS_SpuGetMaxSampleAtOffset(int xaCurrOffset);
#endif
int CDSYS_XAGetNumTracks(int categoryID);
int CDSYS_XAGetTrackLength(int categoryID, int xaID);
int CDSYS_XAPlay(int categoryID, int xaID);
void CDSYS_XAPauseRequest(void);
void CDSYS_XAPauseForce(void);
void CDSYS_XAPauseAtEnd(void);

struct MetaDataMODEL *COLL_LevModelMeta(u32 id);
void COLL_SearchBSP_CallbackQUADBLK(const SVec3 *top, const SVec3 *bottom, struct ScratchpadStruct *sps, s32 hitRadius);
void COLL_SearchBSP_CallbackPARAM(struct BSP *root, struct BoundingBox *bbox, CollBspLeafCallback callback, struct ScratchpadStruct *sps);

void CTR_CycleTex_AllModels(u32 numModels, struct Model **pModelArray, int timer);
void CTR_CycleTex_LEV(struct AnimTex *animtex, int timer);
void CTR_ErrorScreen(char r, char g, char b);
void CTR_CycleTex_Model(struct AnimTex *animtex, int timer);
void CTR_CycleTex_2p3p4pWumpaHUD(u32 *ptrActiveTex, u32 *ptrArray, int numFrames);
void CTR_ClearRenderLists_1P2P(struct GameTracker *gGT, int numPlyrCurrGame);
void CTR_ClearRenderLists_3P4P(struct GameTracker *gGT, int numPlyrCurrGame);
void CTR_EmptyFunc_MainFrame_ResetDB(void);
void CTR_MatrixToRot(SVECTOR *rot, MATRIX *matrix, u32 flags);
void CTR_ScrambleGhostString(char *dst, const char *src);
void CTR_unknownMaybeThunk1(void *dst, void *src);
void CTR_unknownMaybeThunk2(void *dst, void *src);
void CTR_unknownMaybeThunk3(void *dst, void *src, int byteCount);

void CTR_Box_DrawWirePrims(Point p1, Point p2, Color color, void *ot);
void CTR_Box_DrawWireBox(RECT *r, const Color *color, void *ot, struct PrimMem *primMem);
void CTR_Box_DrawClearBox(const RECT *r, const Color *color, int transparency, uint32_t *ot);
void CTR_Box_DrawSolidBox(RECT *r, Color color, uint32_t *ot);

// decal
u32 DecalFont_boolRacingWheel(void);
void DecalFont_DrawLine(char *str, int posX, int posY, s16 fontType, int flags);
void DecalFont_DrawLineStrlen(u8 *str, s16 len, int posX, s16 posY, s16 fontType, int flags);
int DecalFont_DrawMultiLine(char *str, int posX, int posY, int maxPixLen, s16 fontType, int flags);
void DecalGlobal_Clear(struct GameTracker *gGT);
void DecalHUD_DrawPolyFT4(struct Icon *icon, s16 posX, s16 posY, struct PrimMem *primMem, uint32_t *ot, char transparency, s16 scale);
void DecalHUD_DrawPolyGT4(struct Icon *icon, s16 posX, s16 posY, struct PrimMem *primMem, uint32_t *ot, u32 color0, u32 color1, u32 color2, u32 color3,
                          char transparency, s16 scale);
void DecalMP_01(struct GameTracker *gGT);
void DecalMP_02(struct GameTracker *gGT);
void DecalMP_03(struct GameTracker *gGT);

struct DisplayBlurTile;
u32 *DISPLAY_Blur_SubFunc(u32 *prim, struct DisplayBlurTile *tile);
void DISPLAY_Blur_Main(struct PushBuffer *pb, int strength);
void DISPLAY_Swap(void);

void DotLights_Video(struct GameTracker *gGT, int red1, int red2, int red3, int green, int posY);

void DropRain_MakeSound(struct GameTracker *gGT);
void DropRain_Reset(struct GameTracker *gGT);

int Ovr228_800a0cbc_Entry(void *LevRenderList, struct PushBuffer *pb, struct BSP *bspList, struct PrimMem *primMem, void *VisMem10, void *VisMem14,
                          void *VisMem18, void *waterEnvMap);
int Ovr229_800a0cbc_Entry(void *LevRenderList, struct PushBuffer *pb, struct BSP *bspList, struct PrimMem *primMem, void *VisMem10, void *VisMem14,
                          void *VisMem18, void *VisMem1C, void *waterEnvMap);

void EngineSound_Player(struct Driver *driver);
int EngineSound_VolumeAdjust(int desired, int current, int step);
void EngineSound_AI(struct Driver *ai, struct Driver *cameraDriver, int slotIndex, int distance, int distanceDelta, u32 lr);
void EngineSound_NearestAIs(void);

// ElimBG
void ElimBG_Activate(struct GameTracker *gGT);
void ElimBG_Deactivate(struct GameTracker *gGT);
void ElimBG_HandleState(struct GameTracker *gGT);
void ElimBG_SaveScreenshot_Full(struct GameTracker *gGT);

// gamepad
void GAMEPAD_Init(struct GamepadSystem *gGamepads);
void GAMEPAD_PollVsync(struct GamepadSystem *gGamepads);
int GAMEPAD_GetNumConnected(struct GamepadSystem *gGamepads);
int GAMEPAD_ProcessHold(struct GamepadSystem *gGamepads);
void GAMEPAD_ProcessSticks(struct GamepadSystem *gGamepads);
int GAMEPAD_ProcessTapRelease(struct GamepadSystem *gGamepads);
void GAMEPAD_ProcessMotors(struct GamepadSystem *gGamepads);
int GAMEPAD_ProcessAnyoneVars(struct GamepadSystem *gGamepads);
void GAMEPAD_ProcessState(struct GamepadBuffer *pad, int padState, s16 id);

int GAMEPROG_CheckGhostsBeaten(int ghostID);
void GAMEPROG_NewGame_OnBoot(void);
void GAMEPROG_GetPtrHighScoreTrack(void);
void GAMEPROG_InitFullMemcard(struct MemcardProfile *mcp);
void GAMEPROG_SyncGameAndCard(struct GameProgress *memcardProg, struct GameProgress *currentProg);

// ghost
void GhostReplay_Init1(void);
void GhostReplay_Init2(void);
void GhostReplay_ThTick(struct Thread *t);
void GhostTape_Destroy(void);
void GhostTape_End(void);
void GhostTape_Start(void);
void GhostTape_WriteBoosts(int addReserve, u8 type, int speedCap);
void GhostTape_WriteMoves(s16 raceFinished);

// howl
int CountSounds(void);
int OtherFX_Play(u32 soundID, int flags);
void OtherFX_Play_Echo(u32 soundID, int flags, int echoFlag);
int OtherFX_Play_LowLevel(u32 soundID, u8 boolAntiSpam, u32 flags);
u32 OtherFX_Modify(u32 soundId, u32 flags);
void OtherFX_Stop1(int soundID_count);
void OtherFX_Stop2(int soundID_count);
void OtherFX_RecycleNew(u32 *soundID_Count, u32 newSoundID, u32 modifyFlags);
void OtherFX_RecycleMute(int *soundID_Count);
char EngineAudio_InitOnce(u32 soundID, u32 flags);
s16 EngineAudio_Recalculate(u32 soundID, u32 sfx);
void EngineAudio_Stop(u32 soundID);
void SetReverbMode(u16 newReverbMode);
int CseqMusic_Start(u16 songID, int p2, struct SongSet *p3, int p4, int p5);
void CseqMusic_Pause(void);
void CseqMusic_Resume(void);
void CseqMusic_ChangeVolume(u16 songID, int p2, int p3);
void CseqMusic_Restart(u16 songID, int p2);
void CseqMusic_ChangeTempo(u16 songID, int p2);
void CseqMusic_AdvHubSwap(u16 songId, struct SongSet *songSet, int songSetActiveBits);
void CseqMusic_Stop(u16 songID);
void CseqMusic_StopAll(void);
void Bank_ResetAllocator(void);
int Bank_Alloc(int bankID, struct Bank *ptrBank);
int Bank_AssignSpuAddrs(void);
void Bank_Destroy(struct Bank *ptrLastBank);
void Bank_ClearInRange(u16 min, u16 max);
int Bank_Load(int bankID, struct Bank *ptrBank);
int Bank_DestroyLast(void);
void Bank_DestroyUntilIndex(int index);
void Bank_DestroyAll(void);
u32 howl_InstrumentPitch(int basePitch, int pitchIndex, u32 distort);
int howl_InitGlobals(char *filename);
void howl_ParseHeader(struct HowlHeader *hh);
void howl_ParseCseqHeader(struct CseqHeader *ch);
int howl_LoadHeader(char *filename);
int howl_SetSong(int songID);
int howl_LoadSong(void);
void howl_ErasePtrCseqHeader(void);
u8 *howl_GetNextNote(u8 *currNote, int *noteLen);
void cseq_opcode00_empty(struct SongSeq *seq);
void cseq_opcode01_noteoff(struct SongSeq *seq);
void cseq_opcode02_empty(struct SongSeq *seq);
void cseq_opcode03(struct SongSeq *seq);
void cseq_opcode04_empty(struct SongSeq *seq);
void howl_InitChannelAttr_Music(struct SongSeq *seq, struct ChannelAttr *attr, int index, int channelVol);
void cseq_opcode_from06and07(struct SongSeq *seq);
void cseq_opcode05_noteon(struct SongSeq *seq);
void cseq_opcode06(struct SongSeq *seq);
void cseq_opcode07(struct SongSeq *seq);
void cseq_opcode08(struct SongSeq *seq);
void cseq_opcode09(struct SongSeq *seq);
void cseq_opcode0a(struct SongSeq *seq);
struct SongSeq *SongPool_FindFreeChannel(void);
u32 SongPool_CalculateTempo(s16 const60, s16 tpqn, s16 bpm);
void SongPool_ChangeTempo(struct Song *song, s16 deltaBPM);
void SongPool_Start(struct Song *song, u16 songID, s16 deltaBPM, b32 boolLoopAtEnd, struct SongSet *songSet, int songSetActiveBits);
void SongPool_Volume(struct Song *song, int newVol, int newStep, b32 boolImm);
void SongPool_AdvHub1(struct Song *song, int seqID, int vol, b32 boolImm);
void SongPool_AdvHub2(struct Song *song, struct SongSet *songSet, int songSetActiveBits);
void SongPool_StopCseq(struct SongSeq *seq);
void SongPool_StopAllCseq(struct Song *song);
int howl_Disable(void);
void UpdateChannelVol_EngineFX(struct EngineFX *engineFX, struct ChannelAttr *attr, int vol, int LR);
void UpdateChannelVol_OtherFX(struct OtherFX *otherFX, struct ChannelAttr *attr, int vol, int LR);
void UpdateChannelVol_Music(struct SongSeq *songSeq, struct ChannelAttr *attr, int index, int vol);
void UpdateChannelVol_EngineFX_All(void);
void UpdateChannelVol_Music_All(void);
void UpdateChannelVol_OtherFX_All(void);
int howl_VolumeGet(int type);
void howl_VolumeSet(int type, u8 vol);
int howl_ModeGet(void);
void howl_ModeSet(int newMode);
void OptionsMenu_TestSound(int newRow, int newBoolPlay);
void Smart_EnterCriticalSection(void);
void Smart_ExitCriticalSection(void);
void Channel_SetVolume(struct ChannelAttr *attr, int volume, int LR);
int Channel_FindSound(int soundID);
struct ChannelStats *Channel_AllocSlot_AntiSpam(s16 soundID, char boolUseAntiSpam, int flags, struct ChannelAttr *attr);
struct ChannelStats *Channel_AllocSlot(int flags, struct ChannelAttr *attr);
struct ChannelStats *Channel_SearchFX_EditAttr(int type, int soundID, int updateFlags, struct ChannelAttr *attr);
struct ChannelStats *Channel_SearchFX_Destroy(int type, int soundID, int flags);
void Channel_DestroyAll_LowLevel(int opt1, b32 boolKeepMusic, char type);
void Channel_ParseSongToChannels(void);
void Channel_UpdateChannels(void);
void Cutscene_VolumeBackup(void);
void Cutscene_VolumeRestore(void);
void howl_PlayAudio_Update(void);
void howl_InitChannelAttr_EngineFX(struct EngineFX *engineFX, struct ChannelAttr *attr, int vol, int LR, int distort);
void howl_InitChannelAttr_OtherFX(struct OtherFX *otherFX, struct ChannelAttr *attr, int vol, int LR, int distort);
void howl_PauseAudio(void);
void howl_UnPauseChannel(struct ChannelStats *stats);
void howl_UnPauseAudio(void);
void howl_StopAudio(b32 boolErasePauseBackup, b32 boolEraseMusic, b32 boolDestroyAllFX);
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
void Audio_SetState(u32 state);
void Audio_SetState_Safe(int state);
void Audio_AdvHub_SwapSong(int levelID);
void Audio_SetMaskSong(u32 tempo);
void Audio_Update1(void);
void Audio_SetDefaults(void);
void Audio_SetReverbMode(int levelID, u32 isBossRace, int bossID);
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
void GTE_AudioLR_Inst(MATRIX *matrix, s32 *vec);
void GTE_AudioLR_Driver(MATRIX *matrix, struct Driver *driver, s32 *out);
int GTE_GetSquaredLength(s32 *vec);
int GTE_GetSquaredDistance(s16 *pos1, s16 *pos2);
void CalculateVolumeFromDistance(u32 *soundIDCount, u32 soundID, int distance);
void PlayWarppadSound(u32 distance);
void Level_SoundLoopSet(int *soundIDCount, u32 soundID, u32 volume);
void Level_SoundLoopFade(int *fade, u32 soundID, int desiredVolume, int fadeStep);
void Level_RandomFX(int *cooldown, u32 soundID, int baseCooldown, u32 randomRange, int volumeScale);
void Level_AmbientSound(void);
void PlaySound3D(u32 soundID, struct Instance *inst);
void PlaySound3D_Flags(u32 *flags, u32 soundID, struct Instance *inst);
void Garage_Init(void);
void Garage_Enter(char charId);
void Garage_PlayFX(u32 soundId, char charId);
void Garage_LerpFX(void);
void Garage_MoveLR(int desiredId);
void Garage_Leave(void);

// INSTANCE
void INSTANCE_Birth(struct Instance *inst, struct Model *model, const char *name, struct Thread *th, int flags);
struct Instance *INSTANCE_Birth2D(struct Model *model, const char *name, struct Thread *th);
struct Instance *INSTANCE_Birth3D(struct Model *model, const char *name, struct Thread *th);
struct Instance *INSTANCE_BirthWithThread(int modelID, const char *name, int poolType, int bucket, void *funcThTick, int objSize, struct Thread *parent);
struct Instance *INSTANCE_BirthWithThread_Stack(int *spArr);
void INSTANCE_Death(struct Instance *inst);
u16 INSTANCE_GetNumAnimFrames(struct Instance *pInstance, int animIndex);
void INSTANCE_LevInitAll(struct InstDef *levInstDef, int numInst);

// JitPool
int JitPool_Add(struct JitPool *AP);
void JitPool_Clear(struct JitPool *AP);
void JitPool_Init(struct JitPool *AP, int maxItems, int itemSize, char *name);
void JitPool_Remove(struct JitPool *AP, struct Item *item);

// LevInstDef
void LevInstDef_UnPack(struct mesh_info *ptr_mesh_info);
void LevInstDef_RePack(struct mesh_info *ptr_mesh_info, b32 boolAdvHub);

struct Instance *LinkedCollide_Hitbox_Desc(struct HitboxDesc *objBoxDesc);
struct Instance *LinkedCollide_Hitbox(struct Instance *objInst, struct Thread *_objTh, struct Thread *thBucket, struct BoundingBox bbox);

// LIST
void LIST_AddBack(struct LinkedList *L, struct Item *I);
void LIST_AddFront(struct LinkedList *L, struct Item *I);
void LIST_Clear(struct LinkedList *L);
void *LIST_GetFirstItem(struct LinkedList *L);
void *LIST_GetNextItem(struct Item *I);

void LOAD_AppendQueue(struct BigHeader *bigfile, int type, int fileIndex, void *destinationPtr, void (*callback)(struct LoadQueueSlot *));
void LIST_Init(struct LinkedList *L, struct Item *item, int itemSize, int numItems);
struct Item *LIST_RemoveBack(struct LinkedList *L);
struct Item *LIST_RemoveFront(struct LinkedList *L);
struct Item *LIST_RemoveMember(struct LinkedList *L, struct Item *I);

void LOAD_Callback_Overlay_Generic(struct LoadQueueSlot *);
void LOAD_Callback_Overlay_230(void);
void LOAD_Callback_Overlay_231(void);
void LOAD_Callback_Overlay_232(void);
void LOAD_Callback_Overlay_233(void);
#ifdef CTR_NATIVE
void OVR230_ResetRuntimeState(void);
void OVR230_InitData(void);
void OVR231_ResetRuntimeState(void);
void OVR231_InitData(void);
void OVR232_ResetRuntimeState(void);
void OVR232_InitData(void);
void OVR233_RebuildInitMatrixTable(void);
void OVR233_ResetRuntimeState(void);
void OVR233_InitData(void);
#endif
void LOAD_ReadFileASyncCallback(CdlIntrResult result, u8 *unk);

// same hack as AppendQueue, see notes there
#define LOAD_ReadFile(a, b, c, d) LOAD_ReadFile_ex(a, b, c, d, &data.currSlot.size_UNUSED, NULL)
void *LOAD_ReadFile_ex(struct BigHeader *bigfile, u32 loadType, int subfileIndex, void *ptrDst, int *sizePtr, void (*callback)(struct LoadQueueSlot *));
// void* LOAD_ReadFile(struct BigHeader* bigfile, /*u32 loadType,*/ int subfileIndex, void* destination, /*int *size,*/ void * callback);


void *LOAD_VramFile(void *bigfilePtr, int subfileIndex, void *ptrDestination, int *sizePtr, int callbackOrFlags);
void *LOAD_DramFile(void *bigfilePtr, int subfileIndex, void *ptrDestination, int *sizePtr, int callbackOrFlags);
void *LOAD_ReadDirectory(char *filename);
void *LOAD_XnfFile(char *filename, void *ptrDestination, int *size);
int LOAD_TenStages(struct GameTracker *gGT, int loadingStage, struct BigHeader *bigfile);
void LOAD_LevelFile(int levelID);

void LOAD_HowlCallback(CdlIntrResult result, u8 *unk);
int LOAD_HowlSectorChainStart(CdlFILE *cdlFileHWL, void *ptrDestination, int firstSector,
                              int numSector); // 2nd param might be `struct SampleBlockHeader*`
int LOAD_HowlSectorChainEnd(void);
void MainLoadVLC_Callback(struct LoadQueueSlot *param_1);

void LOAD_InitCD(void);
int LOAD_InitCDvol(void);
void LOAD_RunPtrMap(char *origin, int *patchArr, int numPtrs); // 1st param might be `struct Level*`, 2nd param might be `char*`
void LOAD_LangFile(struct BigHeader *bigfile, int lang);

void LOAD_NextQueuedFile(void);

void MainDB_OTMem(struct OTMem *otMem, u32 size);
void MainDB_PrimMem(struct PrimMem *primMem, u32 size);

void MainDrawCb_Vsync(void);
void MainDrawCb_DrawSync(void);

void MainFrame_GameLogic(struct GameTracker *gGT, struct GamepadSystem *gGamepads);
void MainFrame_RenderFrame(struct GameTracker *gGT, struct GamepadSystem *gGamepads);
void DrawUnpluggedMsg(struct GameTracker *gGT, struct GamepadSystem *gGamepads);
void DrawFinalLap(struct GameTracker *gGT);
void RainLogic(struct GameTracker *gGT);
void MenuHighlight(void);
void RenderAllWeather(struct GameTracker *gGT);
void RenderAllConfetti(struct GameTracker *gGT);
void RenderAllHUD(struct GameTracker *gGT);
void RenderAllBeakerRain(struct GameTracker *gGT);
void RenderAllBoxSceneSplitLines(struct GameTracker *gGT);
void RenderBucket_QueueAllInstances(struct GameTracker *gGT);
void RenderAllNormalParticles(struct GameTracker *gGT);
void RenderDispEnv_World(struct GameTracker *gGT);
void RenderAllFlag0x40(struct GameTracker *gGT);
void RenderAllTitleDPP(struct GameTracker *gGT);
void RenderBucket_ExecuteAllInstances(struct GameTracker *gGT);
void RenderAllTires(struct GameTracker *gGT);
void RenderAllShadows(struct GameTracker *gGT);
void RenderAllHeatParticles(struct GameTracker *gGT);
void RenderAllLevelGeometry(struct GameTracker *gGT, struct Level *level1, struct mesh_info *ptr_mesh_info);
void WindowBoxLines(struct GameTracker *gGT);
void WindowDivsionLines(struct GameTracker *gGT);
void RenderDispEnv_UI(struct GameTracker *gGT);
void RenderVSYNC(struct GameTracker *gGT);
void RenderFMV(void);
void RenderSubmit(struct GameTracker *gGT);
void MainFrame_ResetDB(struct GameTracker *gGT);
void MainFrame_InitVideoSTR(u32 boolPlayVideoStr, RECT *r, s16 posX, s16 posY);
void MainFrame_VisMemFullFrame(struct GameTracker *gGT, struct Level *level);
void MainFrame_RequestMaskHint(s16 hintId, char interruptWarpPad);
void MainFrame_TogglePauseAudio(b32 bool_pause);

void StateZero(void);
void startSP(void);

void MainFreeze_ConfigDrawNPC105(s16 startX, s16 startY, s16 radius, int angleStep, s16 angle, char *color, uint32_t *otMem, struct PrimMem *primMem);
void MainFreeze_ConfigSetupEntry(void);
void MainFreeze_SafeAdvDestroy(void);
void MainFreeze_MenuPtrOptions(struct RectMenu *menu);
void MainFreeze_MenuPtrQuit(struct RectMenu *menu);
void MainFreeze_MenuPtrDefault(struct RectMenu *menu);
void MainFreeze_IfPressStart(void);

void MainGameStart_Initialize(struct GameTracker *gGT, char boolStopAudio);

void MainInit_VisMem(struct GameTracker *gGT);
void MainInit_RainBuffer(struct GameTracker *gGT);
void MainInit_Drivers(struct GameTracker *gGT);
void MainInit_JitPoolsNew(struct GameTracker *gGT);
void MainInit_JitPoolsReset(struct GameTracker *gGT);

void MainInit_PrimMem(struct GameTracker *gGT);

void MainInit_OTMem(struct GameTracker *gGT);
void MainInit_FinalizeInit(struct GameTracker *gGT);
int MainInit_StringToLevID(char *str);
void MainInit_VRAMClear(void);
void MainInit_VRAMDisplay(void);

void MainRaceTrack_StartLoad(s16 levelID);
void MainRaceTrack_RequestLoad(s16 levelID);

int MATH_Sin(u32 angle);
int MATH_Cos(u32 angle);
s32 MATH_FastSqrt(u32 value, s32 shift);
void TRIG_AngleSinCos_r19r17r18(u32 angle, s32 *sine, s32 *cosine);
void TRIG_AngleSinCos_r15r16r17(u32 angle, s32 *sine, s32 *cosine);
void TRIG_AngleSinCos_r16r17r18_duplicate(u32 angle, u32 *sine, u32 *cosine);
void TRIG_AngleSinCos_r9r8r10(u32 angle, s32 *sine, s32 *cosine);
void TRIG_AngleSinCos_r16r17r18(u32 angle, s32 *sine, s32 *cosine);
MATRIX *MATH_HitboxMatrix(MATRIX *output, MATRIX *input);
void ConvertRotToMatrix_InverseTranspose_NoRotY(MATRIX *m, const SVec3 *rot);
void ConvertRotToMatrix_InverseTranspose(MATRIX *m, const SVec3 *rot);
void ConvertRotToMatrix(MATRIX *m, const SVec3 *rot);
void ConvertRotToMatrix_Transpose(MATRIX *m, const SVec3 *rot);
void MatrixRotate(MATRIX *dst, MATRIX *src, MATRIX *rot);
void Unknown_8006c49c(u32 *r0, u32 *r1, u32 *r2, u32 *r3, u32 *r4);
void MATRIX_SET_r11r12r13r14r15(u32 r0, u32 r1, u32 r2, u32 r3, u32 r4);
void Unknown_8006c558(u32 *r0, u32 *r1, u32 *r2, u32 *r3, u32 *r4);
void Unknown_8006c600(u32 r0, u32 r1, u32 r2, u32 r3, u32 r4);
int SquareRoot0_stub(int param_1);
VECTOR *Unknown_8006c6c8(VECTOR *input, VECTOR *output, MATRIX *matrix);
VECTOR *ApplyMatrixLV_stub(VECTOR *input, VECTOR *output);
void MATH_MatrixMul(MATRIX *output, MATRIX *input, MATRIX *transform);
s32 Unknown_8006ef98(s32 radicand);

void MEMCARD_InitCard(void);
void MEMCARD_SetIcon(int iconID);
u32 MEMCARD_CRC16(u32 crc, int nextByte);
void MEMCARD_ChecksumSave(u8 *saveBytes, int len);
int MEMCARD_ChecksumLoad(u8 *saveBytes, int len);
char *MEMCARD_StringInit(int slotIndex, char *dstString);
void MEMCARD_StringSet(char *dstString, int slotIdx, char *srcString);
void MEMCARD_CloseCard(void);
u8 MEMCARD_GetNextSwEvent(void);
u8 MEMCARD_GetNextHwEvent(void);
void MEMCARD_SkipEvents(void);
int MEMCARD_NewTask(int slotIdx, char *name, u8 *ptrMemcard, int memcardFileSize, int flags);
void MEMCARD_CloseFile(void);
int MEMCARD_ReadFile(int start_offset, int size);
u8 MEMCARD_WriteFile(int start_offset, const u8 *data, int size);
void MEMCARD_GetFreeBytes(int slotIdx);
u8 MEMCARD_GetInfo(int slotIdx);
u8 MEMCARD_Format(int slotIdx);
int MEMCARD_IsFile(int slotIdx, char *save_name);
char *MEMCARD_FindFirstGhost(int slotIdx, char *srcString);
char *MEMCARD_FindNextGhost(void);
u8 MEMCARD_EraseFile(int slotIdx, char *srcString);
int MEMCARD_HandleEvent(void);
u8 MEMCARD_Load(int slotIdx, char *name, u8 *ptrMemcard, int memcardFileSize, u32 loadFlags);
u8 MEMCARD_Save(int slotIdx, char *name, char *icon, u8 *ptrMemcard, int memcardFileSize, u32 saveFlags);

void RaceConfig_LoadGameOptions(void);
void RaceConfig_SaveGameOptions(void);

void RefreshCard_Entry(void);
int RefreshCard_BoolGhostForLEV(u16 trackID);
void RefreshCard_GhostEncodeProfile(u32 slotIndex, u16 characterID, u16 levelID, int time, char *name);
int RefreshCard_GhostDecodeByte(int value);
void RefreshCard_GhostDecodeProfile(struct GhostProfile *profile, char *fileName);
int RefreshCard_GetResult(int result);
void RefreshCard_NextMemcardAction(int slot, int action, char *fileName, char *fileIconHeader, struct GhostHeader *ptrGhostHeader, int fileSize);
u32 RefreshCard_GhostEncodeByte(int currByte);
void RefreshCard_StartMemcardAction(int action);
void RefreshCard_StopMemcardAction(void);
void RefreshCard_SetScreenText(int screenText);
void RefreshCard_Unknown2(void);
void RefreshCard_GetNumGhostsTotal(void);
void RefreshCard_GameProgressAndOptions(void);
void RefreshCard_Unknown3(void);
void RefreshCard_Unknown4(void);

void SelectProfile_MuteCursors(void);
void SelectProfile_UnMuteCursors(void);
void SelectProfile_ThTick(struct Thread *t);
void SelectProfile_PrintInteger(int value, int posX, int posY, b32 usePaddedFormat, int color);
int SelectProfile_UI_ConvertX(int param_1, int param_2);
int SelectProfile_UI_ConvertY(int param_1, int param_2);
void SelectProfile_DrawAdvProfile(struct AdvProgress *adv, int posX, int posY, s16 isHighlighted, s16 slotIndex, u16 menuFlag);
void SelectProfile_GetTrackID(void);
void SelectProfile_Init(u16 flags);
void SelectProfile_Destroy(void);
void SelectProfile_InitAndDestroy(void);
u32 SelectProfile_InputLogic(struct RectMenu *menu, s16 numRows, u32 confirmFlags);
void SelectProfile_AllProfiles_MenuProc(struct RectMenu *menu);
void SelectProfile_ToggleMode(u32 mode);

void MEMPACK_Init(int ramSize);
void MEMPACK_SwapPacks(int index);
void MEMPACK_NewPack(void *start, int size);
int MEMPACK_GetFreeBytes(void);
void *MEMPACK_AllocMem(int size);
void *MEMPACK_AllocHighMem(int allocSize);
void MEMPACK_ClearHighMem(void);
void *MEMPACK_ReallocMem(int size);
int MEMPACK_PushState(void);
void MEMPACK_ClearLowMem(void);
void MEMPACK_PopState(void);
void MEMPACK_PopToState(int id);

void RECTMENU_DrawQuip(char *comment, s16 startX, int startY, u32 sizeX, s16 fontType, int textFlag, s16 boxFlag);
void RECTMENU_DrawInnerRect(RECT *r, int x, uint32_t *ot);
void RECTMENU_DrawSelf(struct RectMenu *menu, int param_2, s16 param_3, s16 width);
void RECTMENU_DrawPolyGT4(struct Icon *icon, s16 posX, s16 posY, struct PrimMem *primMem, uint32_t *ot, u32 color0, u32 color1, u32 color2, u32 color3,
                          char transparency, s16 scale);
int RECTMENU_BoolHidden(struct RectMenu *m);
void RECTMENU_ClearInput(void);
void RECTMENU_CollectInput(void);
void RECTMENU_ProcessState(void);
int RECTMENU_ProcessInput(struct RectMenu *m);
void RECTMENU_DrawOuterRect_Edge(RECT *r, Color color, u32 param_3, uint32_t *otMem);
void RECTMENU_DrawOuterRect_HighLevel(RECT *r, Color color, s16 param_3, uint32_t *otMem);
void RECTMENU_DrawOuterRect_LowLevel(RECT *p, s16 xOffset, u16 yOffset, Color color, s16 param_5, uint32_t *otMem);
u8 *RECTMENU_DrawTime(int milliseconds);
void RECTMENU_DrawRwdBlueRect_Subset(s16 *pos, int *color, uint32_t *ot, struct PrimMem *primMem);
void RECTMENU_DrawRwdBlueRect(RECT *rect, char *metas, uint32_t *ot, struct PrimMem *primMem);
void RECTMENU_DrawRwdTriangle(s16 *position, char *color, uint32_t *otMem, struct PrimMem *primMem);
void RECTMENU_GetHeight(struct RectMenu *m, s16 *height, b32 boolCheckSubmenu);
void RECTMENU_GetWidth(struct RectMenu *m, s16 *width, b32 boolCheckSubmenu);
void RECTMENU_Hide(struct RectMenu *m);
void RECTMENU_Show(struct RectMenu *m);

int MixRNG_Scramble(void);
int RngDeadCoed(u32 *state);

void MainStats_ClearBattleVS(void);
void MainStats_RestartRaceCountLoss(void);

void Particle_UpdateAllParticles(void);
void Particle_RenderList(struct PushBuffer *pb, void *particleList);

void PickupBots_Init(void);
void PickupBots_Update(void);

struct Thread *PROC_BirthWithObject(int flags, void *funcThTick, const char *name, struct Thread *relativeTh);
void PROC_CheckAllForDead(void);
void PROC_CheckBloodlineForDead(struct Thread **replaceSelf, struct Thread *th);
void PROC_CollidePointWithBucket(struct Thread *th, struct BucketSearchParams *buf);
void PROC_CollidePointWithSelf(struct Thread *th, struct BucketSearchParams *buf);
void PROC_CollideHitboxWithBucket(struct Thread *collThread, struct ScratchpadStruct *sps, struct Thread *ignoredThread);
void PROC_DestroyInstance(struct Thread *t);
void PROC_DestroyObject(void *object, int threadFlags);
void PROC_DestroySelf(struct Thread *t);
void PROC_DestroyTracker(struct Thread *t);
void PROC_PerBspLeaf_CheckInstances(struct BSP *bspLeaf, struct ScratchpadStruct *sps);
struct Thread *PROC_SearchForModel(struct Thread *th, s16 modelID);
void PROC_StartSearch_Self(struct ScratchpadStruct *sps);

void ThTick_SetAndExec(struct Thread *thread, void (*funcThTick)(struct Thread *));

void PushBuffer_Init(struct PushBuffer *pb, int id, int total);
void PushBuffer_UpdateFrustum(struct PushBuffer *pb);
#if defined(CTR_NATIVE)
s32 PushBuffer_GetFrustumSavedCameraZ(void);
#endif
void PushBuffer_SetPsyqGeom(struct PushBuffer *pb);
void PushBuffer_SetMatrixVP(struct PushBuffer *pb);

void PushBuffer_SetDrawEnv_DecalMP(void *ot, struct DB *backBuffer, RECT *viewport, s16 offsetX, s16 offsetY, u8 dtd, u8 dfe, u8 isbg, u8 tpageUpper,
                                   u8 tpageLower);
void PushBuffer_SetDrawEnv_Normal(void *ot, struct PushBuffer *pb, struct DB *backBuffer, s16 *copyDrawEnvNULL, int isbg);

void PushBuffer_FadeOneWindow(struct PushBuffer *pb);
void PushBuffer_FadeAllWindows(void);

void QueueLoadTrack_MenuProc(struct RectMenu *menu);
struct RectMenu *QueueLoadTrack_GetMenuPtr(void);

void RaceFlag_SetCanDraw(s16 param_1);
void RaceFlag_BeginTransition(int direction);
void RaceFlag_SetFullyOnScreen(void);
void RaceFlag_SetFullyOffScreen(void);
void RaceFlag_ResetTextAnim(void);
void RaceFlag_DrawSelf(void);

s16 SubmitName_DrawMenu(u16 string);
void SubmitName_MenuProc(struct RectMenu *menu);
void SubmitName_RestoreName(s16 param_1);
#if defined(CTR_NATIVE)
void SubmitName_UseKeyboard(int key);
#endif

void Timer_Init(void);
void Timer_Destroy(void);
int Timer_GetTime_Total();
int Timer_GetTime_Elapsed(int, int *);

// UI
void UI_ThTick_CountPickup(struct Thread *bucket);
void UI_ThTick_Reward(struct Thread *bucket);
void UI_ThTick_CtrLetters(struct Thread *bucket);
void UI_ThTick_big1(struct Thread *bucket);

int UI_ConvertX_2(int oldPosX, int newPosX);
int UI_ConvertY_2(int oldPosY, int newPosY);

void UI_INSTANCE_InitAll(void);
struct Instance *UI_INSTANCE_BirthWithThread(int param_1, int param_2, int param_3, int param_4, int param_5, int param_6);

void UI_DrawBattleScores(int posX, int posY, struct Driver *d);
void UI_Weapon_DrawSelf(s16 posX, s16 posY, s16 scale, struct Driver *d);
void UI_Weapon_DrawBG(s16 param_1, s16 param_2, s16 param_3, struct Driver *d);
void UI_DrawNumWumpa(s16 param_1, s16 param_2, struct Driver *d);
void UI_DrawNumKey(s16 posX, s16 posY);
void UI_DrawNumRelic(s16 posX, s16 posY);
void UI_DrawNumTrophy(s16 posX, s16 posY);
void UI_DrawNumCrystal(s16 posX, s16 posY, struct Driver *d);
void UI_DrawNumTimebox(s16 posX, s16 posY, struct Driver *d);
void UI_DrawSpeedBG(void);
void UI_DrawSpeedNeedle(s16 posX, s16 posY, struct Driver *driver);
void UI_JumpMeter_Draw(s16 posX, s16 posY, struct Driver *driver);
void UI_JumpMeter_Update(struct Driver *d);
void UI_DrawSlideMeter(s16 posX, s16 posY, struct Driver *driver);
u32 UI_VsQuipReadDriver(struct Driver *driver, int offset, int size);
void UI_VsQuipAssign(struct Driver *driver, struct QuipMeta *meta, struct Driver *bestDriver, int characterID);
void UI_VsQuipAssignAll(void);
void UI_VsQuipDrawAll(void);
void UI_VsWaitForPressX(void);
void UI_RaceStart_IntroText1P(void);
void UI_DrawRankedDrivers(void);
void UI_DrawDriverIcon(struct Icon *icon, s16 posX, s16 posY, struct PrimMem *primMem, uint32_t *ot, char transparency, s16 scale, u32 color);
void UI_RenderFrame_AdvHub(void);
void UI_RenderFrame_Racing(void);
void UI_RenderFrame_Wumpa3D_2P3P4P(struct GameTracker *gGT);
void UI_CupStandings_FinalizeCupRanks(void);
void UI_CupStandings_UpdateCupRanks(void);
void UI_CupStandings_InputAndDraw(void);

void UI_SaveLapTime(int numLaps, int lapTime, s16 driverID);

void UI_Map_GetIconPos(s16 *m, int *posX, int *posY);
void UI_Map_DrawMap(struct Icon *mapTop, struct Icon *mapBottom, s16 posX, s16 posY, struct PrimMem *primMem, uint32_t *otMem, u32 colorID);

void UI_Lerp2D_Linear(s16 *ptrPos, s16 startX, s16 startY, s16 endX, s16 endY, int curFrame, s16 endFrame);

void UI_RaceEnd_MenuProc(struct RectMenu *);

// VEH
void VehBirth_TeleportSelf(struct Driver *d, u8 spawnFlag, int spawnPosY);
void VehBirth_TeleportAll(struct GameTracker *gGT, u32 spawnFlags);
struct Model *VehBirth_GetModelByName(char *searchName);
void VehBirth_SetConsts(struct Driver *driver);
void VehBirth_EngineAudio_AllPlayers(void);
void VehBirth_TireSprites(struct Thread *t);
void VehBirth_NonGhost(struct Thread *t, int index);
struct Driver *VehBirth_Player(int index);

struct Terrain *VehAfterColl_GetTerrain(u8 terrainType);
u32 VehCalc_FastSqrt(u32 n, u32 shift);
struct Particle *VehEmitter_Exhaust(struct Driver *driver, VECTOR *pos, VECTOR *vel);
void VehEmitter_Sparks_Ground(struct Driver *driver, struct ParticleEmitter *emSet);
void VehEmitter_Terrain_Ground(struct Driver *driver, struct ParticleEmitter *emSet);
void VehEmitter_Sparks_Wall(struct Driver *driver, struct ParticleEmitter *emSet);
void VehEmitter_DriverMain(struct Thread *thread, struct Driver *driver);
int VehFrameInst_GetStartFrame(int animIndex, int numFrames);
u32 VehFrameInst_GetNumAnimFrames(struct Instance *inst, int animIndex);
struct MaskHeadWeapon *VehPickupItem_MaskUseWeapon(struct Driver *driver, b32 boolPlaySound);

void VehPhysGeneral_PhysAngular(struct Thread *t, struct Driver *d);

void VehPhysProc_Driving_PhysLinear(struct Thread *t, struct Driver *d);
void VehPhysProc_Driving_Audio(struct Thread *t, struct Driver *d);
void VehPhysProc_Driving_Update(struct Thread *t, struct Driver *d);
void VehPhysProc_Driving_Init(struct Thread *t, struct Driver *d);

void VehPhysProc_FreezeEndEvent_PhysLinear(struct Thread *t, struct Driver *d);
void VehPhysProc_FreezeEndEvent_Init(struct Thread *t, struct Driver *d);
void VehStuckProc_PlantEaten_Init(struct Thread *t, struct Driver *d);
void VehStuckProc_RIP_Init(struct Thread *t, struct Driver *d);

void VehPhysProc_FreezeVShift_ReverseOneFrame(struct Thread *t, struct Driver *d);
void VehPhysProc_FreezeVShift_Update(struct Thread *t, struct Driver *d);
void VehPhysProc_FreezeVShift_Init(struct Thread *t, struct Driver *d);

void VehStuckProc_MaskGrab_FindDestPos(struct Driver *d, struct QuadBlock *quad);
void VehStuckProc_MaskGrab_Update(struct Thread *t, struct Driver *d);
void VehStuckProc_MaskGrab_PhysLinear(struct Thread *t, struct Driver *d);
void VehStuckProc_MaskGrab_Animate(struct Thread *t, struct Driver *d);
void VehStuckProc_MaskGrab_Init(struct Thread *t, struct Driver *d);

void VehPhysProc_PowerSlide_PhysAngular(struct Thread *t, struct Driver *d);
void VehPhysProc_PowerSlide_Finalize(struct Driver *d);
void VehPhysProc_PowerSlide_Update(struct Thread *t, struct Driver *d);
void VehPhysProc_PowerSlide_PhysLinear(struct Thread *t, struct Driver *d);
void VehPhysProc_PowerSlide_InitSetUpdate(struct Thread *t, struct Driver *d);
void VehPhysProc_PowerSlide_Init(struct Thread *t, struct Driver *d);

void VehPhysProc_SlamWall_Animate(struct Thread *t, struct Driver *d);
void VehPhysProc_SlamWall_PhysAngular(struct Thread *t, struct Driver *d);
void VehPhysProc_SlamWall_Update(struct Thread *t, struct Driver *d);
void VehPhysProc_SlamWall_PhysLinear(struct Thread *t, struct Driver *d);
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

void VehPhysProc_SpinStop_Animate(struct Thread *t, struct Driver *d);
void VehPhysProc_SpinStop_Update(struct Thread *t, struct Driver *d);
void VehPhysProc_SpinStop_PhysLinear(struct Thread *t, struct Driver *d);
void VehPhysProc_SpinStop_PhysAngular(struct Thread *t, struct Driver *d);
void VehPhysProc_SpinStop_Init(struct Thread *t, struct Driver *d);

void PlayLevel_UpdateLapStats(void);
void MainGameEnd_SoloRaceGetReward(int subtractTimeCrateBonus);
void MainGameEnd_SoloRaceSaveHighScore(void);
void MainGameEnd_Initialize(void);
void Podium_InitModels(struct GameTracker *gGT);
void VehLap_UpdateProgress(struct Driver *driver);

void VehStuckProc_RevEngine_Update(struct Thread *t, struct Driver *d);
void VehStuckProc_RevEngine_PhysLinear(struct Thread *t, struct Driver *d);
void VehStuckProc_RevEngine_Animate(struct Thread *t, struct Driver *d);
void VehStuckProc_RevEngine_Init(struct Thread *t, struct Driver *d);

void VehStuckProc_Warp_AddDustPuff2(struct Driver *d, int *warp);
void VehStuckProc_Warp_PhysAngular(struct Thread *t, struct Driver *d);
void VehStuckProc_Warp_Init(struct Thread *t, struct Driver *d);

void VehPhysForce_ConvertSpeedToVec(struct Driver *driver);
void VehPhysForce_AccelTerrainSlope(struct Driver *driver);
void VehPhysForce_RotAxisAngle(MATRIX *m, s16 *normVec, s16 angle);
void VehPhysForce_OnApplyForces(struct Thread *t, struct Driver *d);
void VehPhysGeneral_SetHeldItem(struct Driver *driver);
void VehPickupItem_ShootOnCirclePress(struct Driver *d);
void VehFire_Audio(struct Driver *driver, int speed_cap);
void VehFire_Increment(struct Driver *driver, int reserves, u32 type, int fireLevel);
void VehTurbo_ProcessBucket(struct Thread *turboThread);
void VehTurbo_ThTick(struct Thread *t);
void VehTurbo_ThDestroy(struct Thread *t);

void VehGroundShadow_Main(void);
void VehGroundSkids_Main(struct Thread *thread, struct PushBuffer *pb);

void Vector_SpecLightSpin2D(struct Instance *inst, const SVec3 *rot, const SVec3 *lightDir);
void Vector_BakeMatrixTable(void);

void VehStuckProc_Tumble_Update(struct Thread *thread, struct Driver *driver);
void VehStuckProc_Tumble_PhysLinear(struct Thread *thread, struct Driver *driver);
void VehStuckProc_Tumble_PhysAngular(struct Thread *thread, struct Driver *driver);
void VehStuckProc_Tumble_Animate(struct Thread *thread, struct Driver *driver);
void VehStuckProc_Tumble_Init(struct Thread *thread, struct Driver *driver);

// 230
void MM_Battle_DrawIcon_Character(struct Icon *icon, int posX, int posY, struct PrimMem *primMem, uint32_t *ot, char transparency, s16 scale);
u8 MM_TransitionInOut(struct TransitionMeta *meta, int framesPassed, int numFrames);
void MM_Title_MenuUpdate(void);
void MM_Title_SetTrophyDPP(void);
void MM_Title_CameraMove(struct Title *title, int frameIndex);
void MM_Title_ThTick(struct Thread *title);
void MM_Title_Init(void);
void MM_Title_CameraReset(void);
void MM_Title_KillThread(void);
void MM_ParseCheatCodes(void);
void MM_MenuProc_Main(struct RectMenu *mainMenu);
void MM_ToggleRows_PlayerCount(void);
void MM_MenuProc_1p2p(struct RectMenu *menu);
void MM_MenuProc_2p3p4p(struct RectMenu *menu);
void MM_ToggleRows_Difficulty(void);
void MM_MenuProc_Difficulty(struct RectMenu *menu);
void MM_MenuProc_SingleCup(struct RectMenu *menu);
void MM_MenuProc_NewLoad(struct RectMenu *menu);
struct RectMenu *MM_AdvNewLoad_GetMenuPtr(void);
void MM_Characters_AnimateColors(u8 *colorData, s16 playerID, s16 flag);
int MM_Characters_GetNextDriver(s16 dpad, char characterID);
u32 MM_Characters_boolIsInvalid(s16 *globalIconPerPlayer, s16 characterID, s16 player);
struct Model *MM_Characters_GetModelByName(int *name);
void MM_Characters_DrawWindows(b32 boolShowDrivers);
void MM_Characters_SetMenuLayout(void);
void MM_Characters_BackupIDs(void);
void MM_Characters_PreventOverlap(void);
void MM_Characters_RestoreIDs(void);
void MM_Characters_HideDrivers(void);
void MM_Characters_MenuProc(struct RectMenu *unused);
void MM_TrackSelect_Video_SetDefaults(void);
void MM_TrackSelect_Video_State(int state);
void MM_TrackSelect_Video_Draw(RECT *r, struct MainMenu_LevelRow *selectMenu, int trackIndex, int param_4, u16 param_5);
char MM_TrackSelect_boolTrackOpen(struct MainMenu_LevelRow *menuSelect);
void MM_TrackSelect_Init(void);
void MM_TrackSelect_MenuProc(struct RectMenu *menu);
struct RectMenu *MM_TrackSelect_GetMenuPtr(void);
void MM_CupSelect_Init(void);
void MM_CupSelect_MenuProc(struct RectMenu *menu);
void MM_Battle_CloseSubMenu(struct RectMenu *menu);
void MM_Battle_DrawIcon_Weapon(struct Icon *icon, u32 posX, int posY, struct PrimMem *primMem, u32 *ot, char transparency, s16 param_7, u16 param_8,
                               u32 *color);
void MM_Battle_Init(void);
void MM_Battle_MenuProc(struct RectMenu *unused);
void MM_HighScore_Text3D(char *string, int posX, int posY, s16 font, u32 flags);
void MM_HighScore_Draw(u16 trackIndex, u32 rowIndex, u32 posX, u32 posY);
void MM_HighScore_Init(void);
void MM_HighScore_MenuProc(struct RectMenu *unused);
void MM_Scrapbook_Init(void);
void MM_Scrapbook_PlayMovie(struct RectMenu *menu);
void MM_ResetAllMenus(void);
void MM_JumpTo_Title_Returning(void);
void MM_JumpTo_Title_FirstTime(void);
void MM_JumpTo_BattleSetup(void);
void MM_JumpTo_TrackSelect(void);
void MM_JumpTo_Characters(void);
void MM_JumpTo_Scrapbook(void);

// soon to have DECOMP prefix
void MM_Video_DecDCToutCallbackFunc(void);
void MM_Video_KickCD(CdlLOC *location);
void MM_Video_VLC_Decode(void);
void MM_Video_StartStream(int param_1, int numFrames);
void MM_Video_StopStream(void);
void MM_Video_AllocMem(u32 width, u16 height, u32 flags, int size, int param_5);
void MM_Video_ClearMem(void);
u32 MM_Video_DecodeFrame(s16 offsetX, s16 offsetY);
u32 MM_Video_CheckIfFinished(int param_1);

// 231 (undone)
void RB_Player_ModifyWumpa(struct Driver *driver, int wumpaDelta);
void RB_MakeInstanceReflective(struct ScratchpadStruct *sps, struct Instance *inst);
void RB_Player_KillPlayer(struct Driver *attacker, struct Driver *victim);
void RB_MinePool_Init(void);
void RB_MinePool_Remove(struct MineWeapon *mw);
void RB_MinePool_Add(struct MineWeapon *mw);

void RB_MaskWeapon_FadeAway(struct Thread *t);
void RB_MaskWeapon_ThTick(struct Thread *maskTh);

struct Instance *RB_Hazard_CollideWithDrivers(struct Instance *weaponInst, char boolCanSkipParent, int hitRadius, struct Instance *mineDriverInst);

struct Instance *RB_Hazard_CollideWithBucket(struct Instance *weaponInst, struct Thread *weaponTh, struct Thread *bucket, char boolCanSkipParent, int hitRadius,
                                             struct Instance *mineDriverInst);

void RB_Hazard_ThCollide_Missile(struct Thread *thread);
void RB_Hazard_ThCollide_Generic(struct Thread *thread);
void RB_Hazard_ThCollide_Generic_Alt(struct Thread **param_1);
u16 RB_Hazard_CollLevInst(struct ScratchpadStruct *sps, struct Thread *th);

int RB_Hazard_InterpolateValue(s16 currRot, s16 desiredRot, s16 rotSpeed);

void RB_MovingExplosive_ThTick(struct Thread *t);

void RB_MovingExplosive_Explode(struct Thread *t, struct Instance *inst, struct TrackerWeapon *tw);

void RB_RainCloud_FadeAway(struct Thread *t);
void RB_RainCloud_ThTick(struct Thread *t);
void RB_RainCloud_Init(struct Driver *d);
void RB_Explosion_ThTick(struct Thread *t);
void RB_Explosion_InitPotion(struct Instance *inst);
void RB_Explosion_InitGeneric(struct Instance *inst);
void RB_Blowup_ThTick(struct Thread *t);
void RB_Blowup_Init(struct Instance *weaponInst);
void RB_Burst_CollThBucket(struct ScratchpadStruct *sps, void *hitObject);
void RB_Burst_CollLevInst(struct ScratchpadStruct *sps, void *hitObject);
void RB_Burst_ThTick(struct Thread *t);
void RB_Burst_DrawAll(struct GameTracker *gGT);
void RB_Fruit_GetScreenCoords(struct PushBuffer *pb, struct Instance *inst, s16 *output);
void RB_Default_LInB(struct Instance *inst);
void RB_Fruit_LInB(struct Instance *inst);
int RB_Fruit_LInC(struct Instance *fruitInst, struct Thread *driverTh, struct ScratchpadStruct *sps);

struct Thread *RB_GetThread_ClosestTracker(struct Driver *d);
void RB_Baron_LInB(struct Instance *inst);

void RB_Blade_ThTick(struct Thread *t);
void RB_Blade_LInB(struct Instance *inst);

void RB_Armadillo_ThTick_TurnAround(struct Thread *t);
void RB_Armadillo_ThTick_Rolling(struct Thread *t);
void RB_Armadillo_LInB(struct Instance *inst);

void RB_Fireball_ThTick(struct Thread *t);
void RB_Fireball_LInB(struct Instance *inst);

void RB_GenericMine_ThTick(struct Thread *t);
void RB_GenericMine_ThDestroy(struct Thread *t, struct Instance *inst, struct MineWeapon *mw);
void RB_GenericMine_LInB(struct Instance *inst);
void RB_ShieldDark_ThTick_Grow(struct Thread *th);
void RB_ShieldDark_ThTick_Pop(struct Thread *t);
void RB_TNT_ThTick_ThrowOffHead(struct Thread *t);
void RB_TNT_ThTick_SitOnHead(struct Thread *t);
void RB_TNT_ThTick_ThrowOnHead(struct Thread *t);

void RB_Minecart_ThTick(struct Thread *t);
void RB_Minecart_CheckColl(struct Instance *minecartInst, struct Thread *minecartTh);
void RB_Minecart_LInB(struct Instance *inst);

void RB_Potion_OnShatter_TeethCallback(struct ScratchpadStruct *sps, void *hitObject);

int RB_CrateWeapon_LInC(struct Instance *crateInst, struct Thread *collidingTh, struct ScratchpadStruct *sps);
int RB_CrateFruit_LInC(struct Instance *crateInst, struct Thread *collidingTh, struct ScratchpadStruct *sps);
int RB_CrateTime_LInC(struct Instance *crateInst, struct Thread *driverTh, struct ScratchpadStruct *sps);

void RB_Banner_LInB(struct Instance *inst);
void RB_CtrLetter_LInB(struct Instance *inst);
int RB_CtrLetter_LInC(struct Instance *letterInst, struct Thread *driverTh, struct ScratchpadStruct *sps);

void RB_Crystal_LInB(struct Instance *inst);
int RB_Crystal_LInC(struct Instance *crystalInst, struct Thread *driverTh, struct ScratchpadStruct *sps);

void RB_FlameJet_LInB(struct Instance *inst);

void RB_Orca_LInB(struct Instance *inst);

void RB_Plant_LInB(struct Instance *inst);
void RB_Plant_ThTick_Rest(struct Thread *t);

void Seal_CheckColl(struct Instance *sealInst, struct Thread *sealTh, int damage, int radius, int sound);
void RB_Seal_ThTick_Move(struct Thread *t);
void RB_Seal_ThTick_TurnAround(struct Thread *t);
void RB_Seal_LInB(struct Instance *inst);

void RB_Snowball_ThTick(struct Thread *t);
void RB_Snowball_LInB(struct Instance *inst);

void RB_Spider_LInB(struct Instance *inst);

void RB_StartText_ProcessBucket(struct Thread *thread);
void RB_StartText_LInB(struct Instance *inst);

void RB_Teeth_LInB(struct Instance *inst);
int RB_Teeth_LInC(struct Instance *teethInst, struct Thread *t, struct ScratchpadStruct *sps);

void RB_Turtle_ThTick(struct Thread *t);
int RB_Turtle_LInC(struct Instance *inst, struct Thread *driverTh, struct ScratchpadStruct *sps);
void RB_Turtle_LInB(struct Instance *inst);

void RB_Warpball_FadeAway(struct Thread *t);
struct CheckpointNode *RB_Warpball_NewPathNode(struct CheckpointNode *cn, struct Driver *d);
void RB_Warpball_Start(struct TrackerWeapon *tw);
struct Driver *RB_Warpball_GetDriverTarget(struct TrackerWeapon *tw, struct Instance *inst);
void RB_Warpball_SetTargetDriver(struct TrackerWeapon *tw);
void RB_Warpball_SeekDriver(struct TrackerWeapon *tw, u32 param_2, struct Driver *d);
void RB_Warpball_ThTick(struct Thread *t);

void RB_Player_ToggleInvisible(void);
void RB_Player_ToggleFlicker(void);
void RB_Burst_ProcessBucket(struct Thread *thread);
void RB_Blowup_ProcessBucket(struct Thread *thread);
void RB_Follower_ProcessBucket(struct Thread *thread);

// 232
s16 *AH_WarpPad_GetSpawnPosRot(s16 *posData);
void AH_WarpPad_AllWarppadNum(void);
void AH_WarpPad_SetNumModelData(struct Instance *inst, struct ModelHeader *mh);
void AH_WarpPad_MenuProc(struct RectMenu *menu);

void AH_WarpPad_SpinRewards(struct Instance *prizeInst, struct WarpPad *warppadObj, int index, int x, int y, int z);

void AH_WarpPad_ThTick(struct Thread *t);
void AH_WarpPad_ThDestroy(struct Thread *t);
void AH_WarpPad_LInB(struct Instance *inst);
void AH_Garage_ThDestroy(struct Thread *t);
void AH_Garage_Open(struct ScratchpadStruct *sps, void *hitObject);
void AH_Garage_ThTick(struct Thread *t);
void AH_Garage_LInB(struct Instance *inst);
void AH_SaveObj_ThDestroy(struct Thread *t);
void AH_SaveObj_ThTick(struct Thread *t);
void AH_SaveObj_LInB(struct Instance *savInst);
void AH_Door_ThDestroy(struct Thread *t);
void AH_Door_ThTick(struct Thread *t);
void AH_Door_LInB(struct Instance *inst);
void AH_Sign_LInB(struct Instance *inst);

void AH_Map_LoadSave_Prim(s16 *vertPos, char *vertCol, void *ot, struct PrimMem *primMem);

void AH_Map_LoadSave_Full(int posX, int posY, s16 *vertPos, char *vertCol, int unk800, int angle);

void AH_Map_HubArrow(int posX, int posY, s16 *vertPos, char *vertCol, int unk800, int angle);

void AH_Map_HubArrowOutter(void *hubPtrs, int arrowIndex, int posX, int posY, int inputAngle, int type);

void AH_Map_HubItems(void *hubPtrs, s16 *param_2);
void AH_Map_Warppads(s16 *ptrMap, struct Thread *warppadThread, s16 *param_3);
void AH_Map_Main(void);
void AH_Pause_Destroy(void);
void AH_Pause_Draw(int pageID, int posX);
void AH_Pause_Update(void);
void AH_HintMenu_FiveArrows(int param_1, s16 rotation);
void AH_HintMenu_MaskPosRot(void);
void AH_HintMenu_MenuProc(struct RectMenu *menu);
void AH_MaskHint_Start(s16 hintId, u16 bool_interruptWarppad);
int AH_MaskHint_boolCanSpawn(void);
void AH_MaskHint_SetAnim(int scale);

void AH_MaskHint_SpawnParticles(s16 numParticles, struct ParticleEmitter *emSet, int maskAnim);

void AH_MaskHint_LerpVol(int param_1);
void AH_MaskHint_Update(void);

struct Particle *Particle_Init(u32 param_1, struct IconGroup *ig, struct ParticleEmitter *emSet);
void Particle_FuncPtr_PotionShatter(struct Particle *p);
void Particle_FuncPtr_ExhaustUnderwater(struct Particle *p);
void Vector_SpecLightSpin3D(struct Instance *inst, s16 *rot, const SVec3 *lightDir);

// 233
void CS_Garage_ZoomOut(char zoomState);
void CS_Garage_MenuProc(struct RectMenu *param_1);
void CS_Garage_Init(void);
struct RectMenu *CS_Garage_GetMenuPtr(void);
void CS_BoxScene_InstanceSplitLines(void);
void ThTick_RunBucket(struct Thread *thread);
void ThTick_FastRET(struct Thread *thread);
void CS_Thread_LInB(struct Instance *inst);
void CS_Cutscene_Start(void);
void CS_LoadBossCallback(struct LoadQueueSlot *lqs);
void CS_Camera_ThTick_Boss(struct Thread *t);
u8 CS_Camera_BoolGotoBoss(void);
void CS_Camera_ThTick_Podium(struct Thread *th);
void CS_OVR233_InitData(void);
char *CS_OVR233_TranslateRetailOpcodePointer(char *opCodeAt);
int CS_Thread_UseOpcode(struct Instance *instance, struct CutsceneObj *cs);
void CS_Thread_AnimateScale(struct Thread *t);
void CS_Thread_MoveOnPath(struct Thread *t);
void CS_Thread_Particles(struct Thread *t);
void CS_Thread_InterpolateFramesMS(struct Thread *t);
void CS_Thread_ThTick(struct Thread *t);
struct Thread *CS_Thread_Init(s16 modelID, const char *name, s16 *param_3, s16 param_4, struct Thread *parent);
void CS_Podium_Prize_ThDestroy(struct Thread *t);
void CS_Podium_Prize_Spin(struct Instance *inst, s16 *prize);
void CS_Podium_Prize_ThTick1(struct Thread *th);
void CS_Podium_Prize_ThTick2(struct Thread *th);
void CS_Podium_Prize_ThTick3(struct Thread *th);
void CS_Podium_Prize_Init(u32 prizeModel, const char *prizeName, s16 *posOnScreen);
void CS_Podium_Stand_ThTick(struct Thread *t);
void CS_Podium_Stand_Init(s16 *podiumData);
void CS_Podium_FullScene_Init(void);
void CS_Credits_Init(void);
char *CS_Credits_GetNextString(char *str);
void CS_Credits_DestroyCreditGhost(void);
void CS_Credits_AnimateCreditGhost(struct Instance *dst, struct Instance *src, int index);
void CS_Credits_ThTick(void);
int CS_Credits_IsTextValid(void);
void CS_Credits_NewDancer(struct Thread *dancerTh, int dancerModelID);
int CS_Credits_NewCreditGhosts(void);
void CS_Credits_End(void);
void CS_Credits_DrawNames(struct CreditsObj *co);
void CS_Credits_DrawEpilogue(struct CreditsObj *co);

void CC_EndEvent_DrawMenu(void);
void AA_EndEvent_DrawMenu(void);
void RR_EndEvent_DrawMenu(void);
void TT_EndEvent_DrawMenu(void);
void TT_EndEvent_DisplayTime(int paramX, s16 paramY, u32 UI_DrawRaceClockFlags);
void TT_EndEvent_DrawHighScore(s16 startX, int startY, s16 scoreMode);
void VB_EndEvent_DrawMenu(void);

void MM_Cheat_MaxWumpa(void);
void MM_Cheat_UnlockRoo(void);
void MM_Cheat_UnlockPapu(void);
void MM_Cheat_UnlockJoe(void);
void MM_Cheat_UnlockPinstripe(void);
void MM_Cheat_UnlockFakeCrash(void);
void MM_Cheat_UnlockPenta(void);
void MM_Cheat_UnlockTropy(void);
void MM_Cheat_UnlockScrapbook(void);
void MM_Cheat_UnlockTracks(void);
void MM_Cheat_InfiniteMasks(void);
void MM_Cheat_MaxTurbos(void);
void MM_Cheat_MaxInvisibility(void);
void MM_Cheat_MaxEngine(void);
void MM_Cheat_MaxBombs(void);
void MM_Cheat_AdvDifficulty(void);
void MM_Cheat_SuperHard(void);
void MM_Cheat_IcyTracks(void);
void MM_Cheat_SuperTurboPads(void);
void MM_Cheat_OneLap(void);
void MM_Cheat_TurboCounter(void);

void UI_Map_DrawMap_ExtraFunc(struct Icon *icon, POLY_FT4 *p, s16 posX, s16 empty, struct PrimMem *primMem, uint32_t *otMem, u32 transparency);

void VehTalkMask_ThTick(struct Thread *t);
void PhysLerpRot(struct Driver *driver, int targetRotW);
void PhysTerrainSlope(struct Driver *driver);

void Channel_DestroySelf(struct ChannelStats *stats);

//=====================================================================================================================
// this section is forward decls to fix warnings by TheUbMunster.
// any commented out entries were hoisted from files, then commented because they're already present in this file.
// these decls should probably be moved into the upper portion of this file & sorted at some point.
//=====================================================================================================================

// int UI_ConvertX_2(int x, int const_0x200);
// int UI_ConvertY_2(int y, int const_0x200);
// void UI_DrawNumCrystal(s16 x, s16 y, struct Driver* d);
void UI_DrawLimitClock(s16 posX, s16 posY, s16 fontType);
void AA_EndEvent_DisplayTime(s16 driverId, s16 timeOffsetFrames);
void UI_DrawPosSuffix(s16 posX, s16 posY, struct Driver *d, s16 flags);
void UI_DrawRaceClock(u16 paramX, u16 paramY, u32 flags, struct Driver *driver);
int DecalFont_GetLineWidth(char *str, s16 fontType);
void RR_EndEvent_UnlockAward(void);
void RR_EndEvent_DrawHighScore(s16 startX, int startY, s16 scoreMode);
int LOAD_IsOpen_RacingOrBattle(void);
void GAMEPROG_NewProfile_InsideAdv(struct AdvProgress *adv);
int RaceFlag_MoveModels(int frameIndex, int numFrames);
void MainKillGame_LaunchSpyro2(void);
void DecalFont_DrawLineOT(char *str, int posX, int posY, s16 fontType, int flags, uint32_t *ot);
void DecalHUD_Arrow2D(struct Icon *icon, s16 posX, s16 posY, struct PrimMem *primMem, uint32_t *otMemPtr, u32 color1, u32 color2, u32 color3, u32 color4,
                      char transparency, int scale, u16 rot);
void RaceFlag_SetDrawOrder(int drawOrder);
int RaceFlag_IsFullyOnScreen(void);
int RaceFlag_IsFullyOffScreen(void);
void RB_Teeth_OpenDoor(struct Instance *teethInst);
int VehCalc_InterpBySpeed(int val, int speed, int desired);
int VehCalc_MapToRange(int val, int oldMin, int oldMax, int newMin, int newMax);
// void VehStuckProc_Tumble_Animate(struct Thread* thread, struct Driver* driver);
int VehPickupItem_MaskBoolGoodGuy(struct Driver *d);
int RB_Hazard_HurtDriver(struct Driver *driverVictim, int damageType, struct Driver *driverAttacker, int reason);
void VehPickupItem_ShootNow(struct Driver *d, int weaponID, int flags);
int VehPickState_NewState(struct Driver *victimDriver, int damageType, struct Driver *attackDriver, int reason);
void RB_Follower_Init(struct Driver *d, struct Thread *mineTh);
void VehPhysForce_CounterSteer(struct Driver *driver);
int LOAD_IsOpen_AdvHub(void);
int VehPhysJoystick_ReturnToRest(int stickVal, int half, struct RacingWheelData *rwd);
int VehPhysJoystick_GetStrengthAbsolute(int stickVal, int maxSteer, struct RacingWheelData *rwd);
int VehPhysJoystick_GetStrength(int val, int max, struct RacingWheelData *rwd);
int VehPhysGeneral_LerpQuarterStrength(int current, int desired);
int VehPhysGeneral_LerpToForwards(struct Driver *d, int currentAngle, int currentVelocity, int targetAngle);
int VehCalc_SteerAccel(int steeringFrameCount, int stage2FirstFrame, int stage2FrameLength, int stage4FirstFrame, int stage1MinSteer, int stage1MaxSteer);
void VehFrameProc_Driving(struct Thread *t, struct Driver *d);
void VehFrameProc_Spinning(struct Thread *t, struct Driver *d);
void VehFrameProc_LastSpin(struct Thread *t, struct Driver *d);
void VehGroundSkids_Subset1(u32 *currXY, u32 *prevXY, int depth, struct VehGroundSkidsScratch *scratch);
void VehGroundSkids_Subset2(struct VehGroundSkidsScratch *scratch, const SVECTOR *v1, const SVECTOR *v2, const SVECTOR *v3);
void GAMEPAD_ShockForce1(struct Driver *d, int frame, int val);
u32 *RaceFlag_GetOT(void);
void RaceFlag_DrawLoadingString(void);
int DecalFont_GetLineWidthStrlen(char *character, int len, int fontType);
void RB_Burst_Init(struct Instance *weaponInst);
void GAMEPAD_ShockFreq(struct Driver *d, int frame, int val);
int RaceFlag_IsTransitioning(void);
void LOAD_Robots1P(int characterID);
void UI_Map_DrawRawIcon( // 1st param is probably a ptr type of some sort (maybe s16*)?, could maybe do `void*` for now
    int ptrMap, int *param_2, int iconID, int colorID, int unused, s16 scale);
int RaceFlag_GetCanDraw(void);
void UI_Map_DrawDrivers(int ptrMap, struct Thread *bucket, s16 *param_3);
int VehTalkMask_boolNoXA(void);
void VehTalkMask_End(void);
struct Instance *VehTalkMask_Init(void);
void VehTalkMask_PlayXA(struct Instance *i, int id);
struct RectMenu *MainFreeze_GetMenuPtr(void);
void LOAD_TalkingMask(int packID, int maskID);
int LOAD_GetAdvPackIndex(void);
void CAM_ProcessTransition(SVec3 *currPos, SVec3 *currRot, SVec3 *startPos, SVec3 *startRot, SVec3 *endPos, SVec3 *endRot, s32 frame);
void CAM_LookAtPosition(struct CameraScratchWork *scratchWork, Vec3 *positions, SVec3 *desiredPos, SVec3 *desiredRot);
void CAM_FollowDriver_Spin360(struct CameraDC *cDC, struct CameraScratchWork *scratchWork, struct Driver *d, SVec3 *desiredPos, SVec3 *desiredRot);
void CAM_FollowDriver_AngleAxis(struct CameraDC *cDC, struct Driver *d, struct CameraScratchWork *scratchWork, SVec3 *pushBufferPos, SVec3 *pushBufferRot);
int MainDB_GetClipSize(u32 levelID, int numPlyrCurrGame);
void MainFreeze_ConfigDrawArrows(s16 offsetX, s16 offsetY, char *str);
void CAM_StartOfRace(struct CameraDC *cDC);
int DecalFont_DrawMultiLineStrlen(char *str, s16 len, s16 posX, s16 posY, s16 maxPixLen, s16 fontType, s16 flags);
void GAMEPROG_ResetHighScores(struct GameProgress *gameProg);
void GAMEPROG_NewProfile_OutsideAdv(struct GameProgress *gameProg);
int LOAD_FindFile(char *filename, CdlFILE *cdlFile);
int LOAD_HowlHeaderSectors(CdlFILE *cdlFileHWL, void *ptrDestination, int firstSector, int numSector);
int CDSYS_XASeek(b32 boolCdControl, int categoryID, int xaID);
void LibraryOfModels_Store(struct GameTracker *gGT, u32 numModels, struct Model **ptrModelArray);
void LOAD_DramFileCallback(struct LoadQueueSlot *lqs);
int LOAD_GetBigfileIndex(u32 levelID, int lod, int fileIndexInGroup);
void LOAD_HubSwapPtrs(struct GameTracker *gGT);
void LOAD_GlobalModelPtrs_MPK(void);
void LOAD_OvrEndRace(u32 param_1);
void LOAD_OvrLOD(u32 param_1);
void LOAD_OvrThreads(u32 param_1);
void LibraryOfModels_Clear(struct GameTracker *gGT);
void DecalGlobal_Store(struct GameTracker *gGT, struct LevTexLookup *LTL);
void DebugFont_Init(struct GameTracker *gGT);
void RB_Bubbles_RoosTubes(void);
int LOAD_IsOpen_Podiums(void);
void RB_Spider_DrawWebs(struct Thread *t, struct PushBuffer *pb);
int LOAD_IsOpen_MainMenu(void);
int Particle_BitwiseClampByte(int *value);
void PROC_DestroyBloodline(struct Thread *t);
void RECTMENU_DrawFullRect(struct RectMenu *menu, RECT *inner);
void UI_Map_DrawAdvPlayer(int ptrMap, int *matrix, int unused1, int unused2, s16 param_5, s16 param_6);
void DecalHUD_DrawWeapon(struct Icon *icon, s16 posX, s16 posY, struct PrimMem *primMem, uint32_t *ot, char transparency, s16 scale, char rot);
void DebugFont_DrawNumbers(int index, int screenPosX, int screenPosY);
void UI_RenderFrame_CrystChall(void);
void UI_Map_DrawGhosts(int ptrMap, struct Thread *bucket);
void UI_Map_DrawTracking(int ptrMap, struct Thread *bucket);
void LOAD_Callback_Podiums(struct LoadQueueSlot *lqs);
void LOAD_Callback_LEV(struct LoadQueueSlot *lqs);
void LOAD_Callback_PatchMem(struct LoadQueueSlot *lqs);
void LOAD_Callback_DriverModels(struct LoadQueueSlot *lqs);
void LOAD_VramFileCallback(struct LoadQueueSlot *lqs);
void VehBirth_NullThread(struct Thread *t);
void ElimBG_SaveScreenshot_Chunk(u16 *param_1, u16 *param_2, int param_3);
void ElimBG_ToggleInstance(struct Instance *inst, char boolGameIsPaused);
void ElimBG_ToggleAllInstances(struct GameTracker *gGT, b32 boolGameIsPaused);
void INSTANCE_LevDelayedLInBs(struct InstDef *instDef, int numInstances);
void CAM_ThTick(struct Thread *t);
void FLARE_ThTick(struct Thread *th);
void FLARE_Init(s16 *pos);
void DotLights_AudioAndVideo(struct GameTracker *gGT);
void LOAD_HubCallback(struct LoadQueueSlot *lqs);
void LOAD_Callback_MaskHints3D(struct LoadQueueSlot *lqs);
void LOAD_CDRequestCallback(struct LoadQueueSlot *lqs);
void LOAD_StringToUpper(char *path);
// void LHMatrix_Parent(struct Instance* inst, struct Instance* driverInst, SVECTOR* param_3);

// TODO:
// CTR_Box_DrawWirePrims change void* ot to uint32_t* ot

void MainLoadVLC(void);
void MainKillGame_StopCTR(void);
void VehStuckProc_MaskGrab_Particles(struct Driver *d);
int MainFrame_HaveAllPads(s16 numPlyrNextGame);
void RB_CtrLetter_ThTick(struct Thread *t);
void BOTS_ThTick_Drive(struct Thread *botThread);
void BOTS_ThTick_RevEngine(struct Thread *botThread);
void BOTS_SetRotation(struct Driver *driver, int useSpawnYaw);
u32 BOTS_ChangeState(struct Driver *driverVictim, int damageType, struct Driver *driverAttacker, int reason);
void BOTS_LevInstColl(struct Thread *botThread);
void BOTS_Killplane(struct Thread *botThread);
void BOTS_MaskGrab(struct Thread *botThread);
struct Driver *BOTS_Driver_Init(int driverID);
void BOTS_Driver_Convert(struct Driver *driver);
void UI_RaceEnd_GetDriverClock(struct Driver *d);
void GAMEPAD_JogCon2(struct Driver *d, char val, s16 timeMS);
void GAMEPAD_JogCon1(struct Driver *d, char val, u16 timeMS);
void CAM_EndOfRace(struct CameraDC *cDC, struct Driver *d);
u32 COLL_FIXED_INSTANC_TestPoint(struct ScratchpadStruct *sps, struct BSP *node);
void COLL_FIXED_BSPLEAF_TestInstance(struct BSP *node, struct ScratchpadStruct *sps);
void COLL_FIXED_BotsSearch(const SVec3 *posCurr, const SVec3 *posPrev, struct ScratchpadStruct *sps);
void COLL_FIXED_TRIANGL_Barycentrics(SVec3 *out, const SVec3 *v1, const SVec3 *v2, const SVec3 *point);
void COLL_FIXED_TRIANGL_UNUSED(struct ScratchpadStruct *sps, struct BspSearchVertex *v1, struct BspSearchVertex *v2, struct BspSearchVertex *v3);
void COLL_FIXED_TRIANGL_TestPoint(struct ScratchpadStruct *sps, struct BspSearchVertex *v1, struct BspSearchVertex *v2, struct BspSearchVertex *v3);
void COLL_FIXED_TRIANGL_GetNormVec(struct ScratchpadStruct *sps, struct BspSearchVertex *v1, struct BspSearchVertex *v2, struct BspSearchVertex *v3);
void COLL_FIXED_QUADBLK_LoadScratchpadVerts(struct ScratchpadStruct *sps);
void COLL_FIXED_QUADBLK_GetNormVecs_LoLOD(struct ScratchpadStruct *sps, struct QuadBlock *quad);
void COLL_FIXED_QUADBLK_GetNormVecs_HiLOD(struct ScratchpadStruct *sps, struct QuadBlock *quad);
void COLL_FIXED_QUADBLK_TestTriangles(struct QuadBlock *qb, struct ScratchpadStruct *sps);
void COLL_FIXED_BSPLEAF_TestQuadblocks(struct BSP *node, struct ScratchpadStruct *sps);
void COLL_FIXED_PlayerSearch(struct Thread *t, struct Driver *d);
s32 COLL_MOVED_TRIANGL_ReorderNormals(struct BspSearchResult *candidate, struct BspSearchVertex *v1, struct BspSearchVertex *v2, struct BspSearchVertex *v3);
void COLL_MOVED_TRIANGL_TestPoint(struct ScratchpadStruct *sps, struct BspSearchVertex *v1, struct BspSearchVertex *v2, struct BspSearchVertex *v3);
void COLL_MOVED_QUADBLK_TestTriangles(struct QuadBlock *quad, struct ScratchpadStruct *sps);
void COLL_MOVED_BSPLEAF_TestQuadblocks(struct BSP *node, struct ScratchpadStruct *sps);
void COLL_MOVED_FindScrub(struct QuadBlock *qb, s32 triangleID, struct ScratchpadStruct *sps);
void COLL_MOVED_PlayerSearch(struct Thread *t, struct Driver *d);
u32 COLL_MOVED_ScrubImpact(struct Driver *d, struct Thread *t, struct ScratchpadStruct *sps, struct Scrub *scrub, Vec3 *velocity);
struct Scrub *VehAfterColl_GetSurface(u32 scrubId);
void BOTS_CollideWithOtherAI(struct Driver *robot_1, struct Driver *robot_2);
void OtherFX_DriverCrashing(u32 boolEcho, u32 volume);
void VehPhysCrash_ConvertVecToSpeed(struct Driver *d, Vec3 *v);
int VehPhysCrash_BounceSelf(const SVec3 *normal, const Vec3 *origin, Vec3 *vel, b32 boolOtherDriver);
void VehPhysCrash_AI(struct Driver *bot, Vec3 *vel);
int VehPhysCrash_Attack(struct Driver *driver1, struct Driver *driver2, b32 canPlayFeedback, b32 boolPlayBubblePop);
void VehPhysCrash_AnyTwoCars(struct Thread *thread, struct DriverCollisionSearch *search, Vec3 *selfVel);
void VehPhysForce_ConvertSpeedToVecOut(struct Driver *d, Vec3 *vel);
void VehPhysForce_CollideDrivers(struct Thread *thread, struct Driver *driver);
void VehPhysForce_TranslateMatrix(struct Thread *thread, struct Driver *driver);
int VehPhysGeneral_JumpGetVelY(s16 *normalVec, Vec3 *speedXYZ);
void VehPhysGeneral_JumpAndFriction(struct Thread *thread, struct Driver *driver);
void CS_LoadBoss(const struct BossCutsceneData *bcd);
