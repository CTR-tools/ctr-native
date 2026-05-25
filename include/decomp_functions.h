#if defined(REBUILD_PC) /* && !defined (REBUILD_PS1)*/
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

void DECOMP_CAM_ClearScreen(struct GameTracker *gGT);
void DECOMP_CAM_Init(struct CameraDC *cDC, int cameraID, struct Driver *d, struct PushBuffer *pb);
int DECOMP_CAM_Path_GetNumPoints(void);
u8 DECOMP_CAM_Path_Move(int frameIndex, s16 *position, s16 *rotation, s16 *getPath);
void DECOMP_CAM_SetDesiredPosRot(struct CameraDC *cDC, s16 *pos, s16 *rot);

int DECOMP_CDSYS_Init(int boolUseDisc);
u32 DECOMP_CDSYS_GetFilePosInt(char *fileString, int *filePos);
void DECOMP_CDSYS_SetMode_StreamData(void);
void DECOMP_CDSYS_SetMode_StreamAudio(void);
int DECOMP_CDSYS_SetXAToLang(int lang);
void DECOMP_CDSYS_XaCallbackCdSync(CdlIntrResult result, u8 *unk);  //+unk to adhere to *CdlCB
void DECOMP_CDSYS_XaCallbackCdReady(CdlIntrResult result, u8 *unk); //+unk to adhere to *CdlCB
void DECOMP_CDSYS_SpuCallbackIRQ(void);
void DECOMP_CDSYS_SpuCallbackTransfer(void);
void DECOMP_CDSYS_SpuEnableIRQ(void);
void DECOMP_CDSYS_SpuDisableIRQ(void);
void DECOMP_CDSYS_SpuGetMaxSample(void);
int DECOMP_CDSYS_XAGetNumTracks(int categoryID);
int DECOMP_CDSYS_XAGetTrackLength(int categoryID, int xaID);
int DECOMP_CDSYS_XAPlay(int categoryID, int xaID);
void DECOMP_CDSYS_XAPauseRequest(void);
void DECOMP_CDSYS_XAPauseForce(void);
void DECOMP_CDSYS_XAPauseAtEnd(void);

struct MetaDataMODEL *DECOMP_COLL_LevModelMeta(u32 id);

void DECOMP_CTR_CycleTex_AllModels(u32 numModels, struct Model **pModelArray, int timer);
void DECOMP_CTR_CycleTex_LEV(struct AnimTex *animtex, int timer);
void DECOMP_CTR_ErrorScreen(char r, char g, char b);
void DECOMP_CTR_CycleTex_Model(struct AnimTex *animtex, int timer);

void DECOMP_CTR_Box_DrawWireBox(RECT *r, Color color, void *ot);
void DECOMP_CTR_Box_DrawClearBox(RECT *r, Color *color, int transparency, u_long *ot);
void DECOMP_CTR_Box_DrawSolidBox(RECT *r, Color color, u_long *ot);

// decal
void DECOMP_DecalFont_DrawLine(char *str, int posX, int posY, s16 fontType, int flags);
void DECOMP_DecalFont_DrawLineStrlen(u8 *str, s16 len, int posX, s16 posY, s16 fontType, int flags);
int DECOMP_DecalFont_DrawMultiLine(char *str, int posX, int posY, int maxPixLen, s16 fontType, int flags);
void DECOMP_DecalGlobal_Clear(struct GameTracker *gGT);
void DECOMP_DecalHUD_DrawPolyFT4(struct Icon *icon, s16 posX, s16 posY, struct PrimMem *primMem, u_long *ot, char transparency, s16 scale);
void DECOMP_DecalHUD_DrawPolyGT4(struct Icon *icon, s16 posX, s16 posY, struct PrimMem *primMem, u_long *ot, u32 color0, u32 color1, u32 color2, u32 color3,
                                 char transparency, s16 scale);

void DECOMP_DISPLAY_Swap(void);

void DECOMP_DotLights_Video(struct GameTracker *gGT, int red1, int red2, int red3, int green, int posY);

void DECOMP_DropRain_MakeSound(struct GameTracker *gGT);
void DECOMP_DropRain_Reset(struct GameTracker *gGT);

void EngineSound_Player(struct Driver *driver);
int DECOMP_EngineSound_VolumeAdjust(int desired, int current, int step);
void DECOMP_EngineSound_AI(struct Driver *ai, struct Driver *cameraDriver, int slotIndex, int distance, int distanceDelta, u32 lr);
void DECOMP_EngineSound_NearestAIs(void);

// ElimBG
void DECOMP_ElimBG_Activate(struct GameTracker *gGT);
void DECOMP_ElimBG_Deactivate(struct GameTracker *gGT);
void DECOMP_ElimBG_HandleState(struct GameTracker *gGT);
void DECOMP_ElimBG_SaveScreenshot_Full(struct GameTracker *gGT);

// gamepad
void DECOMP_GAMEPAD_Init(struct GamepadSystem *gGamepads);
void DECOMP_GAMEPAD_PollVsync(struct GamepadSystem *gGamepads);
int DECOMP_GAMEPAD_GetNumConnected(struct GamepadSystem *gGamepads);
void DECOMP_GAMEPAD_ProcessHold(struct GamepadSystem *gGamepads);
void DECOMP_GAMEPAD_ProcessSticks(struct GamepadSystem *gGamepads);
void DECOMP_GAMEPAD_ProcessTapRelease(struct GamepadSystem *gGamepads);
void DECOMP_GAMEPAD_ProcessMotors(struct GamepadSystem *gGamepads);
void DECOMP_GAMEPAD_ProcessAnyoneVars(struct GamepadSystem *gGamepads);
void DECOMP_GAMEPAD_ProcessState(struct GamepadBuffer *pad, int padState, s16 id);

int DECOMP_GAMEPROG_CheckGhostsBeaten(int ghostID);
void DECOMP_GAMEPROG_NewGame_OnBoot(void);
void DECOMP_GAMEPROG_GetPtrHighScoreTrack(void);

// ghost
void DECOMP_GhostReplay_Init1(void);
void DECOMP_GhostReplay_Init2(void);
void DECOMP_GhostReplay_ThTick(struct Thread *t);
void DECOMP_GhostTape_Destroy(void);
void GhostTape_End(void);
void DECOMP_GhostTape_End(void);
void DECOMP_GhostTape_Start(void);
void DECOMP_GhostTape_WriteBoosts(int addReserve, u8 type, int speedCap);
void DECOMP_GhostTape_WriteMoves(s16 raceFinished);

// howl
int DECOMP_CountSounds(void);
int OtherFX_Play(u32 soundID, int flags);
int DECOMP_OtherFX_Play(u32 soundID, int flags);
void OtherFX_Play_Echo(u32 soundID, int flags, int echoFlag);
void DECOMP_OtherFX_Play_Echo(u32 soundID, int flags, int echoFlag);
int DECOMP_OtherFX_Play_LowLevel(u32 soundID, char boolAntiSpam, u32 flags);
int OtherFX_Play_LowLevel(u32 soundID, u8 boolAntiSpam, u32 flags);
u32 DECOMP_OtherFX_Modify(u32 soundId, u32 flags);
void DECOMP_OtherFX_Stop1(int soundID_count);
void OtherFX_Stop1(int soundID_count);
void OtherFX_Stop2(int soundID_count);
void DECOMP_OtherFX_Stop2(int soundID_count);
void DECOMP_OtherFX_RecycleNew(u32 *soundID_Count, u32 newSoundID, u32 modifyFlags);
void OtherFX_RecycleNew(u32 *soundID_Count, u32 newSoundID, u32 modifyFlags);
void DECOMP_OtherFX_RecycleMute(int *soundID_Count);
char DECOMP_EngineAudio_InitOnce(u32 soundID, u32 flags);
s16 DECOMP_EngineAudio_Recalculate(u32 soundID, u32 sfx);
void DECOMP_EngineAudio_Stop(u32 soundID);
void DECOMP_SetReverbMode(u16 newReverbMode);
int DECOMP_CseqMusic_Start(u16 songID, int p2, struct SongSet *p3, int p4, int p5);
void DECOMP_CseqMusic_Pause(void);
void DECOMP_CseqMusic_Resume(void);
void DECOMP_CseqMusic_ChangeVolume(u16 songID, int p2, int p3);
void DECOMP_CseqMusic_Restart(u16 songID, int p2);
void DECOMP_CseqMusic_ChangeTempo(u16 songID, int p2);
void DECOMP_CseqMusic_AdvHubSwap(u16 songId, struct SongSet *songSet, int songSetActiveBits);
void DECOMP_CseqMusic_Stop(u16 songID);
void DECOMP_CseqMusic_StopAll(void);
void DECOMP_Bank_ResetAllocator(void);
int DECOMP_Bank_Alloc(int bankID, struct Bank *ptrBank);
int DECOMP_Bank_AssignSpuAddrs(void);
void DECOMP_Bank_Destroy(struct Bank *ptrLastBank);
void DECOMP_Bank_ClearInRange(u16 min, u16 max);
int DECOMP_Bank_Load(int bankID, struct Bank *ptrBank);
int DECOMP_Bank_DestroyLast(void);
void DECOMP_Bank_DestroyUntilIndex(int index);
void DECOMP_Bank_DestroyAll(void);
u32 DECOMP_howl_InstrumentPitch(int basePitch, int pitchIndex, u32 distort);
int DECOMP_howl_InitGlobals(char *filename);
void DECOMP_howl_ParseHeader(struct HowlHeader *hh);
void DECOMP_howl_ParseCseqHeader(struct CseqHeader *ch);
int DECOMP_howl_LoadHeader(char *filename);
int DECOMP_howl_SetSong(int songID);
int DECOMP_howl_LoadSong(void);
void DECOMP_howl_ErasePtrCseqHeader(void);
char *DECOMP_howl_GetNextNote(char *currNote, int *noteLen);
void DECOMP_cseq_opcode00_empty(struct SongSeq *seq);
void DECOMP_cseq_opcode01_noteoff(struct SongSeq *seq);
void DECOMP_cseq_opcode02_empty(struct SongSeq *seq);
void DECOMP_cseq_opcode03(struct SongSeq *seq);
void DECOMP_cseq_opcode04_empty(struct SongSeq *seq);
void DECOMP_howl_InitChannelAttr_Music(struct SongSeq *seq, struct ChannelAttr *attr, int index, int channelVol);
void DECOMP_cseq_opcode_from06and07(struct SongSeq *seq);
void DECOMP_cseq_opcode05_noteon(struct SongSeq *seq);
void DECOMP_cseq_opcode06(struct SongSeq *seq);
void DECOMP_cseq_opcode07(struct SongSeq *seq);
void DECOMP_cseq_opcode08(struct SongSeq *seq);
void DECOMP_cseq_opcode09(struct SongSeq *seq);
void DECOMP_cseq_opcode0a(struct SongSeq *seq);
struct SongSeq *DECOMP_SongPool_FindFreeChannel(void);
u32 DECOMP_SongPool_CalculateTempo(s16 const60, s16 tpqn, s16 bpm);
void DECOMP_SongPool_ChangeTempo(struct Song *song, s16 deltaBPM);
void DECOMP_SongPool_Start(struct Song *song, u16 songID, s16 deltaBPM, int boolLoopAtEnd, struct SongSet *songSet, int songSetActiveBits);
void DECOMP_SongPool_Volume(struct Song *song, int newVol, int newStep, int boolImm);
void DECOMP_SongPool_AdvHub1(struct Song *song, int seqID, int vol, int boolImm);
void DECOMP_SongPool_AdvHub2(struct Song *song, struct SongSet *songSet, int songSetActiveBits);
void DECOMP_SongPool_StopCseq(struct SongSeq *seq);
void DECOMP_SongPool_StopAllCseq(struct Song *song);
int DECOMP_howl_Disable(void);
void DECOMP_UpdateChannelVol_EngineFX(struct EngineFX *engineFX, struct ChannelAttr *attr, int vol, int LR);
void DECOMP_UpdateChannelVol_OtherFX(struct OtherFX *otherFX, struct ChannelAttr *attr, int vol, int LR);
void DECOMP_UpdateChannelVol_Music(struct SongSeq *songSeq, struct ChannelAttr *attr, int index, int vol);
void DECOMP_UpdateChannelVol_EngineFX_All(void);
void DECOMP_UpdateChannelVol_Music_All(void);
void DECOMP_UpdateChannelVol_OtherFX_All(void);
int DECOMP_howl_VolumeGet(int type);
void DECOMP_howl_VolumeSet(int type, u8 vol);
int DECOMP_howl_ModeGet(void);
void DECOMP_howl_ModeSet(int newMode);
void DECOMP_OptionsMenu_TestSound(int newRow, int newBoolPlay);
void DECOMP_Smart_EnterCriticalSection(void);
void DECOMP_Smart_ExitCriticalSection(void);
void DECOMP_Channel_SetVolume(struct ChannelAttr *attr, int volume, int LR);
int DECOMP_Channel_FindSound(int soundID);
struct ChannelStats *DECOMP_Channel_AllocSlot_AntiSpam(s16 soundID, char boolUseAntiSpam, int flags, struct ChannelAttr *attr);
struct ChannelStats *DECOMP_Channel_AllocSlot(int flags, struct ChannelAttr *attr);
struct ChannelStats *DECOMP_Channel_SearchFX_EditAttr(int type, int soundID, int updateFlags, struct ChannelAttr *attr);
struct ChannelStats *DECOMP_Channel_SearchFX_Destroy(int type, int soundID, int flags);
void DECOMP_Channel_DestroyAll_LowLevel(int opt1, int boolKeepMusic, char type);
void DECOMP_Channel_ParseSongToChannels(void);
void DECOMP_Channel_UpdateChannels(void);
void DECOMP_Cutscene_VolumeBackup(void);
void DECOMP_Cutscene_VolumeRestore(void);
void DECOMP_howl_PlayAudio_Update(void);
void DECOMP_howl_InitChannelAttr_EngineFX(struct EngineFX *engineFX, struct ChannelAttr *attr, int vol, int LR, int distort);
void DECOMP_howl_InitChannelAttr_OtherFX(struct OtherFX *otherFX, struct ChannelAttr *attr, int vol, int LR, int distort);
void DECOMP_howl_PauseAudio(void);
void DECOMP_howl_UnPauseChannel(struct ChannelStats *stats);
void DECOMP_howl_UnPauseAudio(void);
void DECOMP_howl_StopAudio(int boolErasePauseBackup, int boolEraseMusic, int boolDestroyAllFX);
void DECOMP_Voiceline_PoolInit(void);
void DECOMP_Voiceline_ClearTimeStamp(void);
void DECOMP_Voiceline_StopAll(void);
void DECOMP_Voiceline_ToggleEnable(int toggle);
void DECOMP_Voiceline_StartPlay(struct Item *voiceLine);
void DECOMP_Voiceline_Update(void);
void DECOMP_Voiceline_EmptyFunc(void);
void DECOMP_Voiceline_SetDefaults(void);
void DECOMP_Audio_SetState(u32 state);
void DECOMP_Audio_SetState_Safe(int state);
void DECOMP_Audio_AdvHub_SwapSong(int levelID);
void DECOMP_Audio_SetMaskSong(u32 tempo);
void DECOMP_Audio_Update1(void);
void DECOMP_Audio_SetDefaults(void);
void DECOMP_Audio_SetReverbMode(int levelID, u32 isBossRace, int bossID);
void DECOMP_Music_SetIntro(void);
void DECOMP_Music_LoadBanks(void);
u32 DECOMP_Music_AsyncParseBanks(void);
void DECOMP_Music_SetDefaults(void);
void DECOMP_Music_Adjust(u32 songID, int newTempo, struct SongSet *set, u32 songSetActiveBits);
void DECOMP_Music_LowerVolume(void);
void DECOMP_Music_RaiseVolume(void);
void DECOMP_Music_Restart(void);
void DECOMP_Music_Stop(void);
void DECOMP_Music_Start(u32 songID);
void DECOMP_Music_End(void);
u32 DECOMP_Music_GetHighestSongPlayIndex(void);
void DECOMP_GTE_AudioLR_Inst(MATRIX *matrix, s32 *vec);
void DECOMP_GTE_AudioLR_Driver(MATRIX *matrix, struct Driver *driver, s32 *out);
int DECOMP_GTE_GetSquaredLength(s32 *vec);
int DECOMP_GTE_GetSquaredDistance(s16 *pos1, s16 *pos2);
void DECOMP_CalculateVolumeFromDistance(u32 *soundIDCount, u32 soundID, int distance);
void DECOMP_PlayWarppadSound(u32 distance);
void DECOMP_Level_SoundLoopSet(int *soundIDCount, u32 soundID, u32 volume);
void DECOMP_Level_SoundLoopFade(int *fade, u32 soundID, int desiredVolume, int fadeStep);
void DECOMP_Level_RandomFX(int *cooldown, u32 soundID, int baseCooldown, u32 randomRange, int volumeScale);
void DECOMP_Level_AmbientSound(void);
void DECOMP_PlaySound3D(u32 soundID, struct Instance *inst);
void DECOMP_PlaySound3D_Flags(u32 *flags, u32 soundID, struct Instance *inst);
void DECOMP_Garage_Init(void);
void DECOMP_Garage_Enter(char charId);
void DECOMP_Garage_PlayFX(u32 soundId, char charId);
void DECOMP_Garage_LerpFX(void);
void DECOMP_Garage_MoveLR(int desiredId);
void DECOMP_Garage_Leave(void);

// INSTANCE
void DECOMP_INSTANCE_Birth(struct Instance *inst, struct Model *model, char *name, struct Thread *th, int flags);
struct Instance *DECOMP_INSTANCE_Birth2D(struct Model *model, char *name, struct Thread *th);
struct Instance *DECOMP_INSTANCE_Birth3D(struct Model *model, char *name, struct Thread *th);
struct Instance *DECOMP_INSTANCE_BirthWithThread(int modelID, char *name, int poolType, int bucket, void *funcThTick, int objSize, struct Thread *parent);
struct Instance *DECOMP_INSTANCE_BirthWithThread_Stack(int *spArr);
void DECOMP_INSTANCE_Death(struct Instance *inst);
u16 DECOMP_INSTANCE_GetNumAnimFrames(struct Instance *pInstance, int animIndex);
void DECOMP_INSTANCE_LevInitAll(struct InstDef *levInstDef, int numInst);

// JitPool
int DECOMP_JitPool_Add(struct JitPool *AP);
void DECOMP_JitPool_Clear(struct JitPool *AP);
void DECOMP_JitPool_Init(struct JitPool *AP, int maxItems, int itemSize, char *name);
void DECOMP_JitPool_Remove(struct JitPool *AP, struct Item *item);

// LevInstDef
void DECOMP_LevInstDef_UnPack(struct mesh_info *ptr_mesh_info);
void DECOMP_LevInstDef_RePack(struct mesh_info *ptr_mesh_info, int boolAdvHub);

struct Instance *DECOMP_LinkedCollide_Hitbox(struct HitboxDesc *objBoxDesc);

// LIST
void LIST_AddBack(struct LinkedList *L, struct Item *I);
void LIST_AddFront(struct LinkedList *L, struct Item *I);
void LIST_Clear(struct LinkedList *L);
void *LIST_GetFirstItem(struct LinkedList *L);
void *LIST_GetNextItem(struct Item *I);
void LIST_Init(struct LinkedList *L, struct Item *item, int itemSize, int numItems);
struct Item *LIST_RemoveBack(struct LinkedList *L);
struct Item *LIST_RemoveFront(struct LinkedList *L);
struct Item *LIST_RemoveMember(struct LinkedList *L, struct Item *I);

void DECOMP_LOAD_Callback_Overlay_Generic(struct LoadQueueSlot *);
void DECOMP_LOAD_Callback_Overlay_230(void);
void DECOMP_LOAD_Callback_Overlay_231(void);
void DECOMP_LOAD_Callback_Overlay_232(void);
void DECOMP_LOAD_Callback_Overlay_233(void);
void DECOMP_LOAD_ReadFileASyncCallback(CdlIntrResult result, u8 *unk);

// same hack as AppendQueue, see notes there
#define DECOMP_LOAD_ReadFile(a, b, c, d) DECOMP_LOAD_ReadFile_ex(b, c, d)
void *DECOMP_LOAD_ReadFile_ex(/*struct BigHeader* bigfile,*/ u32 loadType, int subfileIndex, void *ptrDst);
// void* DECOMP_LOAD_ReadFile(struct BigHeader* bigfile, /*u32 loadType,*/ int subfileIndex, void* destination, /*int *size,*/ void * callback);


void *DECOMP_LOAD_VramFile(void *bigfilePtr, int subfileIndex, void *ptrDestination, int *sizePtr, int callbackOrFlags);
// void* DECOMP_LOAD_DramFile(void* bigfilePtr, int subfileIndex, int* ptrDestination, /*int* size,*/ int callbackOrFlags);
void *DECOMP_LOAD_ReadDirectory(char *filename);
void *DECOMP_LOAD_XnfFile(char *filename, void *ptrDestination, int *size);
int DECOMP_LOAD_TenStages(struct GameTracker *gGT, int loadingStage, struct BigHeader *bigfile);
void DECOMP_LOAD_LevelFile(int levelID);

void DECOMP_LOAD_HowlCallback(CdlIntrResult result, u8 *unk);
int DECOMP_LOAD_HowlSectorChainStart(CdlFILE *cdlFileHWL, void *ptrDestination, int firstSector,
                                     int numSector); // 2nd param might be `struct SampleBlockHeader*`
int DECOMP_LOAD_HowlSectorChainEnd(void);

void DECOMP_LOAD_InitCD(void);
void DECOMP_LOAD_RunPtrMap(char *origin, int *patchArr, int numPtrs); // 1st param might be `struct Level*`, 2nd param might be `char*`
void DECOMP_LOAD_LangFile(int bigfilePtr, int lang);


// This is a wonderful hack that removes an unused parameter,
// which saves bytes everywhere, without needing to alter the game code,
// We need the 'bigfile' parameter to stay in the C code, just to keep
// the front-end looking similar to ghidra, for easy comparison purposes
#define DECOMP_LOAD_AppendQueue(a, b, c, d, e) DECOMP_LOAD_AppendQueue_ex(b, c, d, e)
void DECOMP_LOAD_AppendQueue_ex(/*int bigfile,*/ int type, int fileIndex, void *destinationPtr, void (*callback)(struct LoadQueueSlot *));
// void DECOMP_LOAD_AppendQueue(int bigfile, int type, int fileIndex, void* destinationPtr, void (*callback)(struct LoadQueueSlot*));


void DECOMP_LOAD_NextQueuedFile(void);

void DECOMP_MainDB_OTMem(struct OTMem *otMem, u32 size);
void DECOMP_MainDB_PrimMem(struct PrimMem *primMem, u32 size);

void DECOMP_MainDrawCb_Vsync(void);
void DECOMP_MainDrawCb_DrawSync(void);

void DECOMP_MainFrame_GameLogic(struct GameTracker *gGT, struct GamepadSystem *gGamepads);
void DECOMP_MainFrame_RenderFrame(struct GameTracker *gGT, struct GamepadSystem *gGamepads);
void DECOMP_MainFrame_ResetDB(struct GameTracker *gGT);
void DECOMP_MainFrame_RequestMaskHint(s16 hintId, char interruptWarpPad);
void DECOMP_MainFrame_TogglePauseAudio(int bool_pause);

void DECOMP_MainFreeze_SafeAdvDestroy(void);
void DECOMP_MainFreeze_MenuPtrQuit(struct RectMenu *menu);
void DECOMP_MainFreeze_MenuPtrDefault(struct RectMenu *menu);
void DECOMP_MainFreeze_IfPressStart(void);

void DECOMP_MainGameStart_Initialize(struct GameTracker *gGT, char boolStopAudio);

void DECOMP_MainInit_Drivers(struct GameTracker *gGT);
void DECOMP_MainInit_JitPoolsNew(struct GameTracker *gGT);
void DECOMP_MainInit_JitPoolsReset(struct GameTracker *gGT);

void DECOMP_MainInit_PrimMem(struct GameTracker *gGT

#ifdef USE_MOREPRIM
                             ,
                             int force
#endif
);

void DECOMP_MainInit_OTMem(struct GameTracker *gGT);
void DECOMP_MainInit_FinalizeInit(struct GameTracker *gGT);
void DECOMP_MainInit_VRAMClear(void);
void DECOMP_MainInit_VRAMDisplay(void);

void DECOMP_MainRaceTrack_StartLoad(s16 levelID);
void DECOMP_MainRaceTrack_RequestLoad(s16 levelID);

int MATH_Sin(u32 angle);
int MATH_Cos(u32 angle);
void ConvertRotToMatrix_InverseTranspose_NoRotY(MATRIX *m, s16 *rot);
void ConvertRotToMatrix_InverseTranspose(MATRIX *m, s16 *rot);
void ConvertRotToMatrix(MATRIX *m, s16 *rot);
void ConvertRotToMatrix_Transpose(MATRIX *m, s16 *rot);
void MatrixRotate(MATRIX *dst, MATRIX *src, MATRIX *rot);
int SquareRoot0_stub(int param_1);
VECTOR *ApplyMatrixLV_stub(VECTOR *input, VECTOR *output);
void MATH_MatrixMul(MATRIX *output, MATRIX *input, VECTOR *rotate);

void DECOMP_MEMCARD_InitCard(void);
void DECOMP_MEMCARD_SetIcon(int iconID);
u32 DECOMP_MEMCARD_CRC16(u32 crc, int nextByte);
int DECOMP_MEMCARD_ChecksumSave(u8 *saveBytes, int len);
int DECOMP_MEMCARD_ChecksumLoad(u8 *saveBytes, int len);
char *DECOMP_MEMCARD_StringInit(int slotIndex, char *dstString);
void DECOMP_MEMCARD_StringSet(char *dstString, int slotIdx, char *srcString);
void DECOMP_MEMCARD_CloseCard(void);
u8 DECOMP_MEMCARD_GetNextSwEvent(void);
u8 DECOMP_MEMCARD_GetNextHwEvent(void);
void DECOMP_MEMCARD_SkipEvents(void);
int DECOMP_MEMCARD_NewTask(int slotIdx, char *name, u8 *ptrMemcard, int memcardFileSize, int flags);
void DECOMP_MEMCARD_CloseFile(void);
int DECOMP_MEMCARD_ReadFile(int start_offset, int size);
u8 DECOMP_MEMCARD_WriteFile(int start_offset, const u8 *data, int size);
void DECOMP_MEMCARD_GetFreeBytes(int slotIdx);
u8 DECOMP_MEMCARD_GetInfo(int slotIdx);
u8 DECOMP_MEMCARD_Format(int slotIdx);
int DECOMP_MEMCARD_IsFile(int slotIdx, char *save_name);
char *DECOMP_MEMCARD_FindFirstGhost(int slotIdx, char *srcString);
char *DECOMP_MEMCARD_FindNextGhost(void);
u8 DECOMP_MEMCARD_EraseFile(int slotIdx, char *srcString);
int DECOMP_MEMCARD_HandleEvent(void);
u8 DECOMP_MEMCARD_Save(int slotIdx, char *name, char *icon, u8 *ptrMemcard, int memcardFileSize, u32 flags);

void DECOMP_RefreshCard_Entry(void);
void DECOMP_RefreshCard_GhostEncodeProfile(u32 slotIndex, u16 characterID, u16 levelID, int time, char *name);
u32 DECOMP_RefreshCard_GhostEncodeByte(int currByte);
void DECOMP_RefreshCard_StartMemcardAction(int action);

void DECOMP_SelectProfile_MuteCursors(void);
void DECOMP_SelectProfile_UnMuteCursors(void);
u32 DECOMP_SelectProfile_InputLogic(struct RectMenu *menu, s16 numRows, u32 confirmFlags);
void DECOMP_SelectProfile_AllProfiles_MenuProc(struct RectMenu *menu);
void DECOMP_SelectProfile_ToggleMode(u32 mode);

void DECOMP_MEMPACK_Init(int ramSize);
void DECOMP_MEMPACK_SwapPacks(int index);
void DECOMP_MEMPACK_NewPack(void *start, int size);
int DECOMP_MEMPACK_GetFreeBytes(void);
void *DECOMP_MEMPACK_AllocMem(int size);
void *DECOMP_MEMPACK_AllocHighMem(int allocSize);
void DECOMP_MEMPACK_ClearHighMem(void);
void *DECOMP_MEMPACK_ReallocMem(int size);
int DECOMP_MEMPACK_PushState(void);
void DECOMP_MEMPACK_ClearLowMem(void);
void DECOMP_MEMPACK_PopState(void);
void DECOMP_MEMPACK_PopToState(int id);

void DECOMP_RECTMENU_DrawQuip(char *comment, s16 startX, int startY, u32 sizeX, s16 fontType, int textFlag, s16 boxFlag);
void DECOMP_RECTMENU_DrawInnerRect(RECT *r, int x, void *ot);
void DECOMP_RECTMENU_DrawSelf(struct RectMenu *menu, int param_2, s16 param_3, s16 width);
void DECOMP_RECTMENU_DrawPolyGT4(struct Icon *icon, s16 posX, s16 posY, struct PrimMem *primMem, u_long *ot, u32 color0, u32 color1, u32 color2, u32 color3,
                                 char transparency, s16 scale);
int DECOMP_RECTMENU_BoolHidden(struct RectMenu *m);
void DECOMP_RECTMENU_ClearInput(void);
void DECOMP_RECTMENU_CollectInput(void);
void DECOMP_RECTMENU_ProcessState(void);
int DECOMP_RECTMENU_ProcessInput(struct RectMenu *m);
void DECOMP_RECTMENU_DrawOuterRect_Edge(RECT *r, Color color, u32 param_3, u_long *otMem);
void DECOMP_RECTMENU_DrawOuterRect_HighLevel(RECT *r, Color color, s16 param_3, u_long *otMem);
void DECOMP_RECTMENU_DrawOuterRect_LowLevel(RECT *p, s16 xOffset, u16 yOffset, Color color, s16 param_5, u_long *otMem);
u8 *DECOMP_RECTMENU_DrawTime(int milliseconds);
void DECOMP_RECTMENU_DrawRwdBlueRect_Subset(s16 *pos, int *color, u_long *ot, struct PrimMem *primMem);
void DECOMP_RECTMENU_DrawRwdBlueRect(RECT *rect, char *metas, u_long *ot, struct PrimMem *primMem);
void DECOMP_RECTMENU_DrawRwdTriangle(s16 *position, char *color, u_long *otMem, struct PrimMem *primMem);
void DECOMP_RECTMENU_GetHeight(struct RectMenu *m, s16 *height, int boolCheckSubmenu);
void DECOMP_RECTMENU_GetWidth(struct RectMenu *m, s16 *width, int boolCheckSubmenu);
void DECOMP_RECTMENU_Hide(struct RectMenu *m);
void DECOMP_RECTMENU_Show(struct RectMenu *m);

int DECOMP_MixRNG_Scramble(void);

struct Thread *DECOMP_PROC_BirthWithObject(int flags, void *funcThTick, char *name, struct Thread *relativeTh);
void DECOMP_PROC_CheckAllForDead(void);
void DECOMP_PROC_CheckBloodlineForDead(struct Thread **replaceSelf, struct Thread *th);
void DECOMP_PROC_CollidePointWithBucket(struct Thread *th, s16 *vec3_pos);
void DECOMP_PROC_CollidePointWithSelf(struct Thread *th, struct BucketSearchParams *buf);
void DECOMP_PROC_CollideHitboxWithBucket(struct Thread *collThread, struct ScratchpadStruct *sps, struct Thread *ignoredThread);
void DECOMP_PROC_DestroyInstance(struct Thread *t);
void DECOMP_PROC_DestroyObject(void *object, int threadFlags);
void DECOMP_PROC_DestroySelf(struct Thread *t);
void DECOMP_PROC_DestroyTracker(struct Thread *t);
void DECOMP_PROC_PerBspLeaf_CheckInstances(struct BSP *bspLeaf, struct ScratchpadStruct *sps);
struct Thread *DECOMP_PROC_SearchForModel(struct Thread *th, s16 modelID);
void DECOMP_PROC_StartSearch_Self(struct ScratchpadStruct *sps);

void DECOMP_PushBuffer_Init(struct PushBuffer *pb, int id, int total);
void DECOMP_PushBuffer_SetPsyqGeom(struct PushBuffer *pb);
void DECOMP_PushBuffer_SetMatrixVP(struct PushBuffer *pb);

void DECOMP_PushBuffer_SetDrawEnv_DecalMP(void *ot, struct DB *backBuffer, RECT *viewport, s16 offsetX, s16 offsetY, u8 dtd, u8 dfe, u8 isbg, u8 tpageUpper,
                                          u8 tpageLower);
void DECOMP_PushBuffer_SetDrawEnv_Normal(void *ot, struct PushBuffer *pb, struct DB *backBuffer, DRAWENV *copyDrawEnvNULL, int isbg);

void DECOMP_PushBuffer_FadeOneWindow(struct PushBuffer *pb);
void DECOMP_PushBuffer_FadeAllWindows(void);

void DECOMP_QueueLoadTrack_MenuProc(struct RectMenu *menu);
struct RectMenu *DECOMP_QueueLoadTrack_GetMenuPtr(void);

void DECOMP_RaceFlag_SetCanDraw(s16 param_1);
void DECOMP_RaceFlag_BeginTransition(int direction);
void DECOMP_RaceFlag_SetFullyOnScreen(void);
void DECOMP_RaceFlag_SetFullyOffScreen(void);
void DECOMP_RaceFlag_ResetTextAnim(void);
void DECOMP_RaceFlag_DrawSelf(void);

s16 DECOMP_SubmitName_DrawMenu(u16 string);
void DECOMP_SubmitName_MenuProc(struct RectMenu *menu);
void SubmitName_RestoreName(s16 param_1);
void DECOMP_SubmitName_RestoreName(s16 param_1);

void DECOMP_Timer_Init(void);
void DECOMP_Timer_Destroy(void);
int DECOMP_Timer_GetTime_Total();
int DECOMP_Timer_GetTime_Elapsed(int, int *);

// UI
void DECOMP_UI_ThTick_CountPickup(struct Thread *bucket);
void DECOMP_UI_ThTick_Reward(struct Thread *bucket);
void DECOMP_UI_ThTick_CtrLetters(struct Thread *bucket);
void DECOMP_UI_ThTick_big1(struct Thread *bucket);

int DECOMP_UI_ConvertX_2(int oldPosX, int newPosX);
int DECOMP_UI_ConvertY_2(int oldPosY, int newPosY);

void DECOMP_UI_INSTANCE_InitAll(void);
struct Instance *DECOMP_UI_INSTANCE_BirthWithThread(int param_1, int param_2, int param_3, int param_4, int param_5, int param_6);

void DECOMP_UI_DrawBattleScores(int posX, int posY, struct Driver *d);
void DECOMP_UI_Weapon_DrawSelf(s16 posX, s16 posY, s16 scale, struct Driver *d);
void DECOMP_UI_Weapon_DrawBG(s16 param_1, s16 param_2, s16 param_3, struct Driver *d);
void DECOMP_UI_DrawNumWumpa(s16 param_1, s16 param_2, struct Driver *d);
void DECOMP_UI_DrawNumKey(s16 posX, s16 posY);
void DECOMP_UI_DrawNumRelic(s16 posX, s16 posY);
void DECOMP_UI_DrawNumTrophy(s16 posX, s16 posY);
void DECOMP_UI_DrawNumCrystal(s16 posX, s16 posY, struct Driver *d);
void DECOMP_UI_DrawNumTimebox(s16 posX, s16 posY, struct Driver *d);
void DECOMP_UI_DrawSpeedBG(void);
void DECOMP_UI_DrawSpeedNeedle(s16 posX, s16 posY, struct Driver *driver);
void DECOMP_UI_JumpMeter_Draw(s16 posX, s16 posY, struct Driver *driver);
void DECOMP_UI_JumpMeter_Update(struct Driver *d);
void DECOMP_UI_DrawSlideMeter(s16 posX, s16 posY, struct Driver *driver);
u32 DECOMP_UI_VsQuipReadDriver(struct Driver *driver, int offset, int size);
void DECOMP_UI_VsQuipAssign(struct Driver *driver, struct QuipMeta *meta, struct Driver *bestDriver, int characterID);
void DECOMP_UI_VsQuipAssignAll(void);
void DECOMP_UI_DrawRankedDrivers(void);
void DECOMP_UI_DrawDriverIcon(struct Icon *icon, Point point, u_long *ot, u32 transparency, int scale, Color color);
void DECOMP_UI_RenderFrame_AdvHub(void);
void DECOMP_UI_RenderFrame_Racing(void);

void UI_SaveLapTime(int numLaps, int lapTime, s16 driverID);
void DECOMP_UI_SaveLapTime(int numLaps, int lapTime, s16 driverID);

void DECOMP_UI_Map_GetIconPos(s16 *m, int *posX, int *posY);
void DECOMP_UI_Map_DrawMap(struct Icon *mapTop, struct Icon *mapBottom, s16 posX, s16 posY, struct PrimMem *primMem, u_long *otMem, u32 colorID);

void DECOMP_UI_Lerp2D_Linear(s16 *ptrPos, s16 startX, s16 startY, s16 endX, s16 endY, int curFrame, s16 endFrame);

void DECOMP_UI_RaceEnd_MenuProc(struct RectMenu *);

// VEH
void DECOMP_VehBirth_TeleportSelf(struct Driver *d, u8 spawnFlag, int spawnPosY);
void DECOMP_VehBirth_TeleportAll(struct GameTracker *gGT, u32 spawnFlags);
void DECOMP_VehBirth_SetConsts(struct Driver *driver);
void DECOMP_VehBirth_EngineAudio_AllPlayers(void);
void DECOMP_VehBirth_TireSprites(struct Thread *t);
void DECOMP_VehBirth_NonGhost(struct Thread *t, int index);
struct Driver *DECOMP_VehBirth_Player(int index);

struct Terrain *DECOMP_VehAfterColl_GetTerrain(u8 terrainType);
u32 VehCalc_FastSqrt(u32 n, u32 shift);
struct Particle *VehEmitter_Exhaust(struct Driver *driver, VECTOR *pos, VECTOR *vel);
void VehEmitter_Sparks_Ground(struct Driver *driver, struct ParticleEmitter *emSet);
void VehEmitter_Terrain_Ground(struct Driver *driver, struct ParticleEmitter *emSet);
void VehEmitter_Sparks_Wall(struct Driver *driver, struct ParticleEmitter *emSet);
void VehEmitter_DriverMain(struct Thread *thread, struct Driver *driver);
struct Particle *DECOMP_VehEmitter_Exhaust(struct Driver *driver, VECTOR *pos, VECTOR *vel);
void DECOMP_VehEmitter_Sparks_Ground(struct Driver *driver, struct ParticleEmitter *emSet);
void DECOMP_VehEmitter_Terrain_Ground(struct Driver *driver, struct ParticleEmitter *emSet);
void DECOMP_VehEmitter_Sparks_Wall(struct Driver *driver, struct ParticleEmitter *emSet);
void DECOMP_VehEmitter_DriverMain(struct Thread *thread, struct Driver *driver);
int DECOMP_VehFrameInst_GetStartFrame(int animIndex, int numFrames);
u32 DECOMP_VehFrameInst_GetNumAnimFrames(struct Instance *inst, int animIndex);
struct MaskHeadWeapon *DECOMP_VehPickupItem_MaskUseWeapon(struct Driver *driver, int boolPlaySound);

void DECOMP_VehPhysGeneral_PhysAngular(struct Thread *t, struct Driver *d);

void DECOMP_VehPhysProc_Driving_PhysLinear(struct Thread *t, struct Driver *d);
void DECOMP_VehPhysProc_Driving_Audio(struct Thread *t, struct Driver *d);
void DECOMP_VehPhysProc_Driving_Update(struct Thread *t, struct Driver *d);
void DECOMP_VehPhysProc_Driving_Init(struct Thread *t, struct Driver *d);

void DECOMP_VehPhysProc_FreezeEndEvent_PhysLinear(struct Thread *t, struct Driver *d);
void DECOMP_VehPhysProc_FreezeEndEvent_Init(struct Thread *t, struct Driver *d);
void VehStuckProc_PlantEaten_Init(struct Thread *t, struct Driver *d);
void DECOMP_VehStuckProc_RIP_Init(struct Thread *t, struct Driver *d);
void VehStuckProc_RIP_Init(struct Thread *t, struct Driver *d);

void VehPhysProc_FreezeVShift_ReverseOneFrame(struct Thread *t, struct Driver *d);
void DECOMP_VehPhysProc_FreezeVShift_Update(struct Thread *t, struct Driver *d);
void DECOMP_VehPhysProc_FreezeVShift_ReverseOneFrame(struct Thread *t, struct Driver *d);
void DECOMP_VehPhysProc_FreezeVShift_Init(struct Thread *t, struct Driver *d);

void VehStuckProc_MaskGrab_FindDestPos(struct Driver *d, struct QuadBlock *quad);
void DECOMP_VehStuckProc_MaskGrab_FindDestPos(struct Driver *d, struct QuadBlock *quad);
void DECOMP_VehStuckProc_MaskGrab_Update(struct Thread *t, struct Driver *d);
void DECOMP_VehStuckProc_MaskGrab_PhysLinear(struct Thread *t, struct Driver *d);
void DECOMP_VehStuckProc_MaskGrab_Animate(struct Thread *t, struct Driver *d);
void DECOMP_VehStuckProc_MaskGrab_Init(struct Thread *t, struct Driver *d);

void DECOMP_VehPhysProc_PowerSlide_PhysAngular(struct Thread *t, struct Driver *d);
void DECOMP_VehPhysProc_PowerSlide_Finalize(struct Thread *t, struct Driver *d);
void DECOMP_VehPhysProc_PowerSlide_Update(struct Thread *t, struct Driver *d);
void DECOMP_VehPhysProc_PowerSlide_PhysLinear(struct Thread *t, struct Driver *d);
void DECOMP_VehPhysProc_PowerSlide_InitSetUpdate(struct Thread *t, struct Driver *d);
void DECOMP_VehPhysProc_PowerSlide_Init(struct Thread *t, struct Driver *d);

void VehPhysProc_SlamWall_Animate(struct Thread *t, struct Driver *d);
void DECOMP_VehPhysProc_SlamWall_PhysAngular(struct Thread *t, struct Driver *d);
void DECOMP_VehPhysProc_SlamWall_Update(struct Thread *t, struct Driver *d);
void DECOMP_VehPhysProc_SlamWall_PhysLinear(struct Thread *t, struct Driver *d);
void DECOMP_VehPhysProc_SlamWall_Animate(struct Thread *t, struct Driver *d);
void DECOMP_VehPhysProc_SlamWall_Init(struct Thread *t, struct Driver *d);

void DECOMP_VehPhysProc_SpinFirst_Update(struct Thread *t, struct Driver *d);
void DECOMP_VehPhysProc_SpinFirst_PhysLinear(struct Thread *t, struct Driver *d);
void DECOMP_VehPhysProc_SpinFirst_PhysAngular(struct Thread *t, struct Driver *d);
void DECOMP_VehPhysProc_SpinFirst_InitSetUpdate(struct Thread *t, struct Driver *d);
void DECOMP_VehPhysProc_SpinFirst_Init(struct Thread *t, struct Driver *d);

void DECOMP_VehPhysProc_SpinLast_Update(struct Thread *t, struct Driver *d);
void DECOMP_VehPhysProc_SpinLast_PhysLinear(struct Thread *t, struct Driver *d);
void DECOMP_VehPhysProc_SpinLast_PhysAngular(struct Thread *t, struct Driver *d);
void DECOMP_VehPhysProc_SpinLast_Init(struct Thread *t, struct Driver *d);

void VehPhysProc_SpinStop_Animate(struct Thread *t, struct Driver *d);
void DECOMP_VehPhysProc_SpinStop_Update(struct Thread *t, struct Driver *d);
void DECOMP_VehPhysProc_SpinStop_PhysLinear(struct Thread *t, struct Driver *d);
void DECOMP_VehPhysProc_SpinStop_PhysAngular(struct Thread *t, struct Driver *d);
void DECOMP_VehPhysProc_SpinStop_Animate(struct Thread *t, struct Driver *d);
void DECOMP_VehPhysProc_SpinStop_Init(struct Thread *t, struct Driver *d);

void PlayLevel_UpdateLapStats(void);
void DECOMP_PlayLevel_UpdateLapStats(void);
void MainGameEnd_SoloRaceGetReward(int subtractTimeCrateBonus);
void DECOMP_MainGameEnd_SoloRaceGetReward(int subtractTimeCrateBonus);
void MainGameEnd_SoloRaceSaveHighScore(void);
void DECOMP_MainGameEnd_SoloRaceSaveHighScore(void);
void MainGameEnd_Initialize(void);
void DECOMP_MainGameEnd_Initialize(void);
void DECOMP_Podium_InitModels(struct GameTracker *gGT);
void VehLap_UpdateProgress(struct Driver *driver);

void DECOMP_VehStuckProc_RevEngine_Update(struct Thread *t, struct Driver *d);
void DECOMP_VehStuckProc_RevEngine_PhysLinear(struct Thread *t, struct Driver *d);
void DECOMP_VehStuckProc_RevEngine_Animate(struct Thread *t, struct Driver *d);
void DECOMP_VehStuckProc_RevEngine_Init(struct Thread *t, struct Driver *d);
void VehStuckProc_RevEngine_Init(struct Thread *t, struct Driver *d);

void DECOMP_VehStuckProc_Warp_Init(struct Thread *t, struct Driver *d);

void DECOMP_VehPhysForce_ConvertSpeedToVec(struct Driver *driver);
void DECOMP_VehPhysForce_AccelTerrainSlope(struct Driver *driver);
void DECOMP_VehPhysForce_OnApplyForces(struct Thread *t, struct Driver *d);
void DECOMP_VehPhysGeneral_SetHeldItem(struct Driver *driver);
void DECOMP_VehPickupItem_ShootOnCirclePress(struct Driver *d);
void VehFire_Audio(struct Driver *driver, int speed_cap);
void DECOMP_VehFire_Audio(struct Driver *driver, int speed_cap);
void DECOMP_VehFire_Increment(struct Driver *driver, int reserves, u32 type, int fireLevel);
void VehTurbo_ProcessBucket(struct Thread *turboThread);
void DECOMP_VehTurbo_ProcessBucket(struct Thread *turboThread);
void VehTurbo_ThTick(struct Thread *t);
void DECOMP_VehTurbo_ThTick(struct Thread *t);
void VehTurbo_ThDestroy(struct Thread *t);
void DECOMP_VehTurbo_ThDestroy(struct Thread *t);

void VehStuckProc_Tumble_Update(struct Thread *thread, struct Driver *driver);
void VehStuckProc_Tumble_PhysLinear(struct Thread *thread, struct Driver *driver);
void VehStuckProc_Tumble_PhysAngular(struct Thread *thread, struct Driver *driver);
void VehStuckProc_Tumble_Animate(struct Thread *thread, struct Driver *driver);
void VehStuckProc_Tumble_Init(struct Thread *thread, struct Driver *driver);
void DECOMP_VehStuckProc_Tumble_Update(struct Thread *thread, struct Driver *driver);
void DECOMP_VehStuckProc_Tumble_PhysLinear(struct Thread *thread, struct Driver *driver);
void DECOMP_VehStuckProc_Tumble_PhysAngular(struct Thread *thread, struct Driver *driver);
void DECOMP_VehStuckProc_Tumble_Animate(struct Thread *thread, struct Driver *driver);
void DECOMP_VehStuckProc_Tumble_Init(struct Thread *thread, struct Driver *driver);

// 230
void DECOMP_MM_Battle_DrawIcon_Character(struct Icon *icon, int posX, int posY, struct PrimMem *primMem, u_long *ot, char transparency, s16 scale);
u8 DECOMP_MM_TransitionInOut(struct TransitionMeta *meta, int framesPassed, int numFrames);
void DECOMP_MM_Title_MenuUpdate(void);
void DECOMP_MM_Title_SetTrophyDPP(void);
void DECOMP_MM_Title_CameraMove(struct Title *title, int frameIndex);
void DECOMP_MM_Title_ThTick(struct Thread *title);
void DECOMP_MM_Title_Init(void);
void DECOMP_MM_Title_CameraReset(void);
void DECOMP_MM_Title_KillThread(void);
void DECOMP_MM_ParseCheatCodes(void);
void DECOMP_MM_MenuProc_Main(struct RectMenu *mainMenu);
void DECOMP_MM_ToggleRows_PlayerCount(void);
void DECOMP_MM_MenuProc_1p2p(struct RectMenu *menu);
void DECOMP_MM_MenuProc_2p3p4p(struct RectMenu *menu);
void DECOMP_MM_ToggleRows_Difficulty(void);
void DECOMP_MM_MenuProc_Difficulty(struct RectMenu *menu);
void DECOMP_MM_MenuProc_SingleCup(struct RectMenu *menu);
void DECOMP_MM_MenuProc_NewLoad(struct RectMenu *menu);
struct RectMenu *DECOMP_MM_AdvNewLoad_GetMenuPtr(void);
void DECOMP_MM_Characters_AnimateColors(u8 *colorData, s16 playerID, s16 flag);
int DECOMP_MM_Characters_GetNextDriver(s16 dpad, char characterID);
u32 DECOMP_MM_Characters_boolIsInvalid(s16 *globalIconPerPlayer, s16 characterID, s16 player);
struct Model *DECOMP_MM_Characters_GetModelByName(int *name);
void DECOMP_MM_Characters_DrawWindows(int boolShowDrivers);
void DECOMP_MM_Characters_SetMenuLayout(void);
void DECOMP_MM_Characters_BackupIDs(void);
void DECOMP_MM_Characters_PreventOverlap(void);
void DECOMP_MM_Characters_RestoreIDs(void);
void DECOMP_MM_Characters_HideDrivers(void);
void DECOMP_MM_Characters_MenuProc(struct RectMenu *unused);
void DECOMP_MM_TrackSelect_Video_SetDefaults(void);
void DECOMP_MM_TrackSelect_Video_State(int state);
void DECOMP_MM_TrackSelect_Video_Draw(RECT *r, struct MainMenu_LevelRow *selectMenu, int trackIndex, int param_4, u16 param_5);
char DECOMP_MM_TrackSelect_boolTrackOpen(struct MainMenu_LevelRow *menuSelect);
void DECOMP_MM_TrackSelect_Init(void);
void DECOMP_MM_TrackSelect_MenuProc(struct RectMenu *menu);
void *DECOMP_MM_TrackSelect_GetMenuPtr(void);
void DECOMP_MM_CupSelect_Init(void);
void DECOMP_MM_CupSelect_MenuProc(struct RectMenu *menu);
void DECOMP_MM_Battle_CloseSubMenu(struct RectMenu *menu);
void DECOMP_MM_Battle_DrawIcon_Weapon(struct Icon *icon, u32 posX, int posY, struct PrimMem *primMem, u32 *ot, char transparency, s16 param_7, u16 param_8,
                                      u32 *color);
void DECOMP_MM_Battle_Init(void);
void DECOMP_MM_Battle_MenuProc(struct RectMenu *unused);
void DECOMP_MM_HighScore_Text3D(char *string, int posX, int posY, s16 font, u32 flags);
void DECOMP_MM_HighScore_Draw(u16 trackIndex, u32 rowIndex, u32 posX, u32 posY);
void DECOMP_MM_HighScore_Init(void);
void DECOMP_MM_HighScore_MenuProc(struct RectMenu *unused);
void DECOMP_MM_Scrapbook_Init(void);
void DECOMP_MM_Scrapbook_PlayMovie(struct RectMenu *menu);
void DECOMP_MM_ResetAllMenus(void);
void DECOMP_MM_JumpTo_Title_Returning(void);
void DECOMP_MM_JumpTo_Title_FirstTime(void);
void DECOMP_MM_JumpTo_BattleSetup(void);
void DECOMP_MM_JumpTo_TrackSelect(void);
void DECOMP_MM_JumpTo_Characters(void);
void DECOMP_MM_JumpTo_Scrapbook(void);

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
void DECOMP_RB_Player_ModifyWumpa(struct Driver *driver, int wumpaDelta);
void DECOMP_RB_MinePool_Init(void);
void DECOMP_RB_MinePool_Remove(struct MineWeapon *mw);
void DECOMP_RB_MinePool_Add(struct MineWeapon *mw);

void DECOMP_RB_MaskWeapon_FadeAway(struct Thread *t);
void DECOMP_RB_MaskWeapon_ThTick(struct Thread *maskTh);

struct Instance *DECOMP_RB_Hazard_CollideWithDrivers(struct Instance *weaponInst, char boolCanSkipParent, int hitRadius, struct Instance *mineDriverInst);

struct Instance *DECOMP_RB_Hazard_CollideWithBucket(struct Instance *weaponInst, struct Thread *weaponTh, struct Thread *bucket, char boolCanSkipParent,
                                                    int hitRadius, struct Instance *mineDriverInst);

void DECOMP_RB_Hazard_ThCollide_Missile(struct Thread *thread);
void DECOMP_RB_Hazard_ThCollide_Generic(struct Thread *thread);

int DECOMP_RB_Hazard_InterpolateValue(s16 currRot, s16 desiredRot, s16 rotSpeed);

void DECOMP_RB_MovingExplosive_ThTick(struct Thread *t);

void DECOMP_RB_MovingExplosive_Explode(struct Thread *t, struct Instance *inst, struct TrackerWeapon *tw);

void DECOMP_RB_RainCloud_FadeAway(struct Thread *t);
void RB_RainCloud_FadeAway(struct Thread *t);
void DECOMP_RB_RainCloud_ThTick(struct Thread *t);
void DECOMP_RB_RainCloud_Init(struct Driver *d);

struct Thread *DECOMP_RB_GetThread_ClosestTracker(struct Driver *d);
void DECOMP_RB_Baron_LInB(struct Instance *inst);

void DECOMP_RB_Blade_ThTick(struct Thread *t);
void DECOMP_RB_Blade_LInB(struct Instance *inst);

void DECOMP_RB_Armadillo_ThTick_TurnAround(struct Thread *t);
void DECOMP_RB_Armadillo_ThTick_Rolling(struct Thread *t);
void DECOMP_RB_Armadillo_LInB(struct Instance *inst);

void DECOMP_RB_Fireball_ThTick(struct Thread *t);
void DECOMP_RB_Fireball_LInB(struct Instance *inst);

void DECOMP_RB_Minecart_ThTick(struct Thread *t);
void DECOMP_RB_Minecart_LInB(struct Instance *inst);

void DECOMP_RB_Plant_LInB(struct Instance *inst);

void DECOMP_RB_Seal_ThTick_Move(struct Thread *t);
void DECOMP_RB_Seal_ThTick_TurnAround(struct Thread *t);
void DECOMP_RB_Seal_LInB(struct Instance *inst);

void DECOMP_RB_Snowball_ThTick(struct Thread *t);
void DECOMP_RB_Snowball_LInB(struct Instance *inst);

void DECOMP_RB_Spider_LInB(struct Instance *inst);

void DECOMP_RB_Turtle_ThTick(struct Thread *t);
int DECOMP_RB_Turtle_LInC(struct Instance *inst, struct Thread *driverTh, struct ScratchpadStruct *sps);
void DECOMP_RB_Turtle_LInB(struct Instance *inst);

// 232
s16 *DECOMP_AH_WarpPad_GetSpawnPosRot(s16 *posData);
void DECOMP_AH_WarpPad_AllWarppadNum(void);
void DECOMP_AH_WarpPad_MenuProc(struct RectMenu *menu);

void DECOMP_AH_WarpPad_SpinRewards(struct Instance *prizeInst, struct WarpPad *warppadObj, int index, int x, int y, int z);

void DECOMP_AH_WarpPad_ThTick(struct Thread *t);
void DECOMP_AH_WarpPad_ThDestroy(struct Thread *t);
void DECOMP_AH_WarpPad_LInB(struct Instance *inst);
void DECOMP_AH_Garage_ThDestroy(struct Thread *t);
void DECOMP_AH_Garage_Open(struct ScratchpadStruct *sps, struct Thread *otherTh);
void DECOMP_AH_Garage_ThTick(struct Thread *t);
void DECOMP_AH_Garage_LInB(struct Instance *inst);
void DECOMP_AH_SaveObj_ThDestroy(struct Thread *t);
void DECOMP_AH_SaveObj_ThTick(struct Thread *t);
void DECOMP_AH_SaveObj_LInB(struct Instance *savInst);
void DECOMP_AH_Door_ThDestroy(struct Thread *t);
void DECOMP_AH_Door_ThTick(struct Thread *t);
void DECOMP_AH_Door_LInB(struct Instance *inst);

void DECOMP_AH_Map_LoadSave_Prim(s16 *vertPos, char *vertCol, void *ot, struct PrimMem *primMem);

void DECOMP_AH_Map_LoadSave_Full(int posX, int posY, s16 *vertPos, char *vertCol, int unk800, int angle);

void DECOMP_AH_Map_HubArrow(int posX, int posY, s16 *vertPos, char *vertCol, int unk800, int angle);

void DECOMP_AH_Map_HubArrowOutter(void *hubPtrs, int arrowIndex, int posX, int posY, int inputAngle, int type);

void DECOMP_AH_Map_HubItems(void *hubPtrs, s16 *param_2);
void DECOMP_AH_Map_Warppads(s16 *ptrMap, struct Thread *warppadThread, s16 *param_3);
void DECOMP_AH_Map_Main(void);
void DECOMP_AH_Pause_Destroy(void);
void DECOMP_AH_Pause_Draw(int pageID, int posX);
void DECOMP_AH_Pause_Update(void);
void DECOMP_AH_HintMenu_FiveArrows(int param_1, s16 rotation);
void DECOMP_AH_HintMenu_MaskPosRot(void);
void DECOMP_AH_HintMenu_MenuProc(struct RectMenu *menu);
void DECOMP_AH_MaskHint_Start(s16 hintId, u16 bool_interruptWarppad);
int DECOMP_AH_MaskHint_boolCanSpawn(void);
void DECOMP_AH_MaskHint_SetAnim(int scale);

void DECOMP_AH_MaskHint_SpawnParticles(s16 numParticles, struct ParticleEmitter *emSet, int maskAnim);

void DECOMP_AH_MaskHint_LerpVol(int param_1);
void DECOMP_AH_MaskHint_Update(void);

struct Particle *Particle_Init(u32 param_1, struct IconGroup *ig, struct ParticleEmitter *emSet);
void Vector_SpecLightSpin3D(struct Instance *inst, s16 *rot, s16 *lightDir);

// 233
void DECOMP_CS_Garage_ZoomOut(char zoomState);
void DECOMP_CS_Garage_MenuProc(struct RectMenu *param_1);
void DECOMP_CS_Garage_Init(void);
struct RectMenu *DECOMP_CS_Garage_GetMenuPtr(void);
void DECOMP_CS_BoxScene_InstanceSplitLines(void);
void DECOMP_CS_Thread_LInB(struct Instance *inst);
void DECOMP_CS_Cutscene_Start(void);
void DECOMP_CS_LoadBossCallback(struct LoadQueueSlot *lqs);
void CS_Camera_ThTick_Boss(struct Thread *t);
u8 DECOMP_CS_Camera_BoolGotoBoss(void);
void CS_Camera_ThTick_Podium(struct Thread *th);
int DECOMP_CS_Thread_UseOpcode(struct Instance *instance, struct CutsceneObj *cs);
void DECOMP_CS_Thread_AnimateScale(struct Thread *t);
void DECOMP_CS_Thread_MoveOnPath(struct Thread *t);
void DECOMP_CS_Thread_Particles(struct Thread *t);
void DECOMP_CS_Thread_InterpolateFramesMS(struct Thread *t);
void DECOMP_CS_Thread_ThTick(struct Thread *t);
struct Thread *DECOMP_CS_Thread_Init(s16 modelID, char *name, s16 *param_3, s16 param_4, struct Thread *parent);
void DECOMP_CS_Podium_Prize_ThDestroy(struct Thread *t);
void CS_Podium_Prize_Spin(struct Instance *inst, s16 *prize);
void CS_Podium_Prize_ThTick1(struct Thread *th);
void CS_Podium_Prize_ThTick2(struct Thread *th);
void CS_Podium_Prize_ThTick3(struct Thread *th);
void CS_Podium_Prize_Init(u32 prizeModel, char *prizeName, s16 *posOnScreen);
void DECOMP_CS_Podium_Stand_ThTick(struct Thread *t);
void DECOMP_CS_Podium_Stand_Init(s16 *podiumData);
void DECOMP_CS_Podium_FullScene_Init(void);
void DECOMP_CS_Credits_Init(void);
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

//=====================================================================================================================
// this section is forward decls to fix warnings by TheUbMunster.
// any commented out entries were hoisted from files, then commented because they're already present in this file.
// these decls should probably be moved into the upper portion of this file & sorted at some point.
//=====================================================================================================================

// int DECOMP_UI_ConvertX_2(int x, int const_0x200);
// int DECOMP_UI_ConvertY_2(int y, int const_0x200);
// void DECOMP_UI_DrawNumCrystal(s16 x, s16 y, struct Driver* d);
void DECOMP_UI_DrawLimitClock(s16 posX, s16 posY, s16 fontType);
void DECOMP_AA_EndEvent_DisplayTime(s16 driverId, s16 param_2);
void DECOMP_UI_DrawPosSuffix(s16 posX, s16 posY, struct Driver *d, s16 flags);
void DECOMP_UI_DrawRaceClock(u16 paramX, u16 paramY, u32 flags, struct Driver *driver);
int DECOMP_DecalFont_GetLineWidth(char *str, s16 fontType);
void RR_EndEvent_UnlockAward(void);
void DECOMP_RR_EndEvent_UnlockAward(void);
void DECOMP_RR_EndEvent_DrawHighScore(s16 startX, int startY);
int DECOMP_LOAD_IsOpen_RacingOrBattle(void);
void DECOMP_GAMEPROG_NewProfile_InsideAdv(struct AdvProgress *adv);
int DECOMP_RaceFlag_MoveModels(int frameIndex, int numFrames);
void DECOMP_MainKillGame_LaunchSpyro2(void);
void DECOMP_DecalFont_DrawLineOT(char *str, int posX, int posY, s16 fontType, int flags, u_long *ot);
void DECOMP_DecalHUD_Arrow2D(struct Icon *icon, s16 posX, s16 posY, struct PrimMem *primMem, u_long *otMemPtr, u32 color1, u32 color2, u32 color3, u32 color4,
                             char transparency, int scale, u16 rot);
void DECOMP_RaceFlag_SetDrawOrder(int drawOrder);
int DECOMP_RaceFlag_IsFullyOnScreen(void);
int DECOMP_RaceFlag_IsFullyOffScreen(void);
void DECOMP_RB_Teeth_OpenDoor(struct Instance *teethInst);
int DECOMP_VehCalc_InterpBySpeed(int val, int speed, int desired);
int DECOMP_VehCalc_MapToRange(int val, int oldMin, int oldMax, int newMin, int newMax);
// void DECOMP_VehStuckProc_Tumble_Animate(struct Thread* thread, struct Driver* driver);
int DECOMP_VehPickupItem_MaskBoolGoodGuy(struct Driver *d);
u32 DECOMP_VehFrameInst_GetNumAnimFrames(struct Instance *inst, int animIndex);
int DECOMP_RB_Hazard_HurtDriver(struct Driver *driverVictim, int damageType, struct Driver *driverAttacker, int reason);
void DECOMP_VehPickupItem_ShootNow(struct Driver *d, int weaponID, int flags);
int DECOMP_VehPickState_NewState(struct Driver *victimDriver, int damageType, struct Driver *attackDriver, int reason);
void DECOMP_RB_Follower_Init(struct Driver *d, struct Thread *mineTh);
void DECOMP_VehPhysForce_CounterSteer(struct Driver *driver);
int DECOMP_LOAD_IsOpen_AdvHub(void);
int DECOMP_VehPhysJoystick_ReturnToRest(int stickVal, int half, struct RacingWheelData *rwd);
int DECOMP_VehPhysJoystick_GetStrengthAbsolute(int stickVal, int maxSteer, struct RacingWheelData *rwd);
int DECOMP_VehPhysJoystick_GetStrength(int val, int max, struct RacingWheelData *rwd);
int DECOMP_VehPhysGeneral_LerpQuarterStrength(int current, int desired);
int DECOMP_VehPhysGeneral_LerpToForwards(struct Driver *d, int param_2, int param_3, int param_4);
int DECOMP_VehCalc_SteerAccel(int param_1, int param_2, int param_3, int param_4, int param_5, int param_6);
void DECOMP_VehFrameProc_Driving(struct Thread *t, struct Driver *d);
void DECOMP_VehFrameProc_Spinning(struct Thread *t, struct Driver *d);
void DECOMP_VehFrameProc_LastSpin(struct Thread *t, struct Driver *d);
void DECOMP_GAMEPAD_ShockForce1(struct Driver *d, int frame, int val);
u32 *DECOMP_RaceFlag_GetOT(void);
void DECOMP_RaceFlag_DrawLoadingString(void);
int DECOMP_DecalFont_GetLineWidthStrlen(char *character, int len, int fontType);
void DECOMP_RB_Burst_Init(struct Instance *weaponInst);
void DECOMP_GAMEPAD_ShockFreq(struct Driver *d, int frame, int val);
int DECOMP_RaceFlag_IsTransitioning(void);
void DECOMP_LOAD_Robots1P(int characterID);
void DECOMP_CTR_Box_DrawWirePrims(Point p1, Point p2, Color color, void *ot);
void DECOMP_UI_Map_DrawRawIcon( // 1st param is probably a ptr type of some sort (maybe s16*)?, could maybe do `void*` for now
    int ptrMap, int *param_2, int iconID, int colorID, int unused, s16 scale);
int DECOMP_RaceFlag_GetCanDraw(void);
void DECOMP_UI_Map_DrawDrivers(int ptrMap, struct Thread *bucket, s16 *param_3);
int DECOMP_VehTalkMask_boolNoXA(void);
void DECOMP_VehTalkMask_End(void);
struct Instance *DECOMP_VehTalkMask_Init(void);
void DECOMP_VehTalkMask_PlayXA(struct Instance *i, int id);
struct RectMenu *DECOMP_MainFreeze_GetMenuPtr(void);
void DECOMP_LOAD_TalkingMask(int packID, int maskID);
int DECOMP_LOAD_GetAdvPackIndex(void);
void DECOMP_CAM_ProcessTransition(s16 *currPos, s16 *currRot, s16 *startPos, s16 *startRot, s16 *endPos, s16 *endRot, int frame);
void DECOMP_CAM_LookAtPosition(int scratchpad, int *positions, s16 *desiredPos, s16 *desiredRot);
void DECOMP_CAM_FollowDriver_Spin360(struct CameraDC *cDC, int param_2, struct Driver *d, s16 *desiredPos, s16 *desiredRot);
int DECOMP_MainDB_GetClipSize(u32 levelID, int numPlyrCurrGame);
void DECOMP_MainFreeze_ConfigDrawArrows(s16 offsetX, s16 offsetY, char *str);
void DECOMP_CAM_StartOfRace(struct CameraDC *cDC);
int DECOMP_DecalFont_DrawMultiLineStrlen(char *str, s16 len, s16 posX, s16 posY, s16 maxPixLen, s16 fontType, s16 flags);
void DECOMP_GAMEPROG_ResetHighScores(struct GameProgress *gameProg);
void DECOMP_GAMEPROG_NewProfile_OutsideAdv(struct GameProgress *gameProg);
int DECOMP_LOAD_FindFile(char *filename, CdlFILE *cdlFile);
int DECOMP_LOAD_HowlHeaderSectors(CdlFILE *cdlFileHWL, void *ptrDestination, int firstSector, int numSector);
int DECOMP_CDSYS_XASeek(int boolCdControl, int categoryID, int xaID);
void DECOMP_LibraryOfModels_Store(struct GameTracker *gGT, u32 numModels, struct Model **ptrModelArray);
void DECOMP_LOAD_DramFileCallback(struct LoadQueueSlot *lqs);
int DECOMP_LOAD_GetBigfileIndex(u32 levelID, int lod);
void DECOMP_LOAD_HubSwapPtrs(struct GameTracker *gGT);
void DECOMP_LOAD_GlobalModelPtrs_MPK(void);
void DECOMP_LOAD_OvrEndRace(u32 param_1);
void DECOMP_LOAD_OvrLOD(u32 param_1);
void DECOMP_LOAD_OvrThreads(u32 param_1);
void DECOMP_LOAD_DriverMPK(u32 param_1, int levelLOD);
void DECOMP_LibraryOfModels_Clear(struct GameTracker *gGT);
void DECOMP_DecalGlobal_Store(struct GameTracker *gGT, struct LevTexLookup *LTL);
void DECOMP_DebugFont_Init(struct GameTracker *gGT);
void DECOMP_RB_Bubbles_RoosTubes(void);
int DECOMP_LOAD_IsOpen_Podiums(void);
void DECOMP_RB_Spider_DrawWebs(struct Thread *t, struct PushBuffer *pb);
int DECOMP_LOAD_IsOpen_MainMenu(void);
int DECOMP_Particle_BitwiseClampByte(int *value);
void DECOMP_PROC_DestroyBloodline(struct Thread *t);
void DECOMP_RECTMENU_DrawFullRect(struct RectMenu *menu, RECT *inner);
void DECOMP_UI_Map_DrawAdvPlayer(int ptrMap, int *matrix, int unused1, int unused2, s16 param_5, s16 param_6);
void DECOMP_DecalHUD_DrawWeapon(struct Icon *icon, s16 posX, s16 posY, struct PrimMem *primMem, u_long *ot, char transparency, s16 scale, char rot);
void DECOMP_DebugFont_DrawNumbers(int index, int screenPosX, int screenPosY);
void DECOMP_UI_RenderFrame_CrystChall(void);
void DECOMP_UI_Map_DrawGhosts(int ptrMap, struct Thread *bucket);
void DECOMP_UI_Map_DrawTracking(int ptrMap, struct Thread *bucket);
void DECOMP_LOAD_Callback_Podiums(struct LoadQueueSlot *lqs);
void DECOMP_LOAD_Callback_LEV(struct LoadQueueSlot *lqs);
void DECOMP_LOAD_Callback_PatchMem();
void DECOMP_LOAD_Callback_DriverModels(struct LoadQueueSlot *lqs);
void DECOMP_LOAD_VramFileCallback(struct LoadQueueSlot *lqs);
void DECOMP_VehBirth_NullThread(struct Thread *t);
int DECOMP_LOAD_Robots2P(int p1, int p2);
void DECOMP_ElimBG_SaveScreenshot_Chunk(u16 *param_1, u16 *param_2, int param_3);
void DECOMP_ElimBG_ToggleInstance(struct Instance *inst, char boolGameIsPaused);
void DECOMP_ElimBG_ToggleAllInstances(struct GameTracker *gGT, int boolGameIsPaused);
void DECOMP_INSTANCE_LevDelayedLInBs(struct Instance *instDefs, u32 numInstances);
void DECOMP_CAM_ThTick(struct Thread *t);
void DECOMP_FLARE_Init(s16 *pos);
void DECOMP_DotLights_AudioAndVideo(struct GameTracker *gGT);
void DECOMP_LOAD_HubCallback(struct LoadQueueSlot *lqs);
void DECOMP_LOAD_Callback_MaskHints3D(struct LoadQueueSlot *lqs);
void DECOMP_LOAD_CDRequestCallback(struct LoadQueueSlot *lqs);
void DECOMP_LOAD_StringToUpper(char *path);
// void LHMatrix_Parent(struct Instance* inst, struct Instance* driverInst, SVECTOR* param_3);

// TODO:
// DECOMP_RECTMENU_DrawInnerRect change void* ot to u_long* ot
// DECOMP_CTR_Box_DrawWirePrims change void* ot to u_long* ot

void DECOMP_MainLoadVLC(void);
void DECOMP_MainKillGame_StopCTR(void);
void DECOMP_VehStuckProc_MaskGrab_Particles(struct Driver *d);
int DECOMP_MainFrame_HaveAllPads(s16 numPlyrNextGame);
void DECOMP_RB_CtrLetter_ThTick(struct Thread *t);
void BOTS_ThTick_Drive(struct Thread *botThread);
void BOTS_ThTick_RevEngine(struct Thread *botThread);
void BOTS_SetRotation(struct Driver *driver, int param_2);
u32 BOTS_ChangeState(struct Driver *driverVictim, int damageType, struct Driver *driverAttacker, int reason);
void BOTS_LevInstColl(struct Thread *param_1);
void BOTS_Killplane(struct Thread *botThread);
void BOTS_MaskGrab(struct Thread *botThread);
void BOTS_Driver_Convert(struct Driver *driver);
void UI_RaceEnd_GetDriverClock(struct Driver *d);
void DECOMP_UI_RaceEnd_GetDriverClock(struct Driver *d);
void DECOMP_GAMEPAD_JogCon2(struct Driver *d, char val, s16 timeMS);
void DECOMP_GAMEPAD_JogCon1(struct Driver *d, int val, int timeMS);
void DECOMP_CAM_EndOfRace(struct CameraDC *cDC, struct Driver *d);
void DECOMP_COLL_SearchBSP_CallbackPARAM(struct BSP *root, struct BoundingBox *bbox, void (*callback)(struct BSP *, struct ScratchpadStruct *),
                                         struct ScratchpadStruct *param);
u32 COLL_FIXED_INSTANC_TestPoint(struct ScratchpadStruct *sps, struct BSP *node);
void COLL_FIXED_BSPLEAF_TestInstance(struct BSP *node, struct ScratchpadStruct *sps);
void COLL_FIXED_BotsSearch(s16 *posCurr, s16 *posPrev, struct ScratchpadStruct *sps);
void COLL_FIXED_TRIANGL_Barycentrics(s16 *out, s16 *v1, s16 *v2, s16 *point);
void COLL_FIXED_TRIANGL_UNUSED(void *sps, void *v1, void *v2, void *v3);
void COLL_FIXED_TRIANGL_TestPoint(void *sps, void *v1, void *v2, void *v3);
void COLL_FIXED_TRIANGL_GetNormVec(void *sps, void *v1, void *v2, void *v3);
void COLL_FIXED_QUADBLK_LoadScratchpadVerts(struct ScratchpadStruct *sps);
void COLL_FIXED_QUADBLK_GetNormVecs_LoLOD(struct ScratchpadStruct *sps, struct QuadBlock *quad);
void COLL_FIXED_QUADBLK_GetNormVecs_HiLOD(struct ScratchpadStruct *sps, struct QuadBlock *quad);
void COLL_FIXED_QUADBLK_TestTriangles(struct QuadBlock *qb, struct ScratchpadStruct *sps);
void COLL_FIXED_BSPLEAF_TestQuadblocks(struct BSP *node, struct ScratchpadStruct *sps);
void COLL_FIXED_PlayerSearch(struct Thread *t, struct Driver *d);
s32 COLL_MOVED_TRIANGL_ReorderNormals(void *set1, void *v1, void *v2, void *v3);
void COLL_MOVED_TRIANGL_TestPoint(struct ScratchpadStruct *sps, struct BspSearchVertex *v1, struct BspSearchVertex *v2, struct BspSearchVertex *v3);
void COLL_MOVED_QUADBLK_TestTriangles(struct QuadBlock *quad, struct ScratchpadStruct *sps);
void COLL_MOVED_BSPLEAF_TestQuadblocks(struct BSP *node, struct ScratchpadStruct *sps);
void COLL_MOVED_FindScrub(struct QuadBlock *qb, int triangleID, struct ScratchpadStruct *sps);
void COLL_MOVED_PlayerSearch(struct Thread *t, struct Driver *d);
u32 COLL_MOVED_ScrubImpact(struct Driver *d, struct Thread *t, struct ScratchpadStruct *sps, struct Scrub *scrub, int *velocity);
struct Scrub *VehAfterColl_GetSurface(u32 scrubId);
void BOTS_CollideWithOtherAI(struct Driver *robot_1, struct Driver *robot_2);
void OtherFX_DriverCrashing(u32 boolEcho, u32 volume);
void VehPhysCrash_ConvertVecToSpeed(struct Driver *d, Vec3 *v);
int VehPhysCrash_BounceSelf(s16 *normal, Vec3 *origin, Vec3 *vel, int boolOtherDriver);
void VehPhysCrash_AI(struct Driver *bot, Vec3 *vel);
int VehPhysCrash_Attack(struct Driver *driver1, struct Driver *driver2, int canPlayFeedback, int boolPlayBubblePop);
void VehPhysCrash_AnyTwoCars(struct Thread *thread, u16 *searchWords, Vec3 *selfVel);
void VehPhysForce_ConvertSpeedToVec(struct Driver *d, Vec3 *vel);
void VehPhysForce_CollideDrivers(struct Thread *thread, struct Driver *driver);
void VehPhysForce_TranslateMatrix(struct Thread *thread, struct Driver *driver);
void VehFrameProc_Driving(struct Thread *thread, struct Driver *driver);
void VehFrameProc_Spinning(struct Thread *thread, struct Driver *driver);
void VehFrameProc_LastSpin(struct Thread *thread, struct Driver *driver);
int VehPhysGeneral_JumpGetVelY(s16 *normalVec, Vec3 *speedXYZ);
void VehPhysGeneral_JumpAndFriction(struct Thread *thread, struct Driver *driver);
int DECOMP_VehPhysGeneral_JumpGetVelY(s16 *normalVec, Vec3 *speedXYZ);
void DECOMP_VehPhysGeneral_JumpAndFriction(struct Thread *thread, struct Driver *driver);
void DECOMP_VehPhysForce_TranslateMatrix(struct Thread *thread, struct Driver *driver);
void DECOMP_CS_LoadBoss(struct BossCutsceneData *bcd);
