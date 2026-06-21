#ifndef GAME_INCLUDES_H
#define GAME_INCLUDES_H

#include <common.h>

#include "game/226/R226.c"
#include "game/227/R227.c"
#include "game/228/R228.c"
#include "game/229/R229.c"
#include "game/226/226_00_DrawLevelOvr1P.c"
#include "game/227/227_00_DrawLevelOvr2P.c"
#include "game/228/228_00_DrawLevelOvr3P.c"
#include "game/229/229_00_DrawLevelOvr4P.c"
#include "game/DrawConfetti/DrawConfetti.c"
#include "game/DrawSky/DrawSky.c"

#include "game/RenderBucket/RenderBucket_08_InitDepthGTE.c"
#include "game/RenderBucket/RenderBucket_QueueExecute.c"
#include "game/RenderLevel/AnimateWater.c"
#include "game/RenderLevel/RenderLists.c"
#include "game/RenderTires/DrawTires_Solid.c"
#include "game/RenderTires/DrawTires_Reflection.c"
#include "game/RenderStars/RenderStars.c"
#include "game/Torch/Torch.c"
#include "game/RenderWeather/RedBeaker_RenderRain.c"
#include "game/RenderWeather/RenderWeather.c"

#include "game/CAM.c"

#include "game/BOTS.c"
#include "game/CDSYS.c"

#include "game/COLL.c"

#include "game/CTR/CTR_Box.c"
#include "game/CTR/CTR_CycleTex.c"
#include "game/CTR/CTR_RenderLists.c"
#include "game/CTR/CTR_Error.c"
#include "game/CTR/CTR_Visibility.c"
#include "game/CTR/CTR_Matrix.c"
#include "game/CTR/CTR_Ghost.c"

#include "game/DebugFont.c"

#include "game/DecalFont.c"

#include "game/DecalGlobal.c"

#include "game/DecalHUD.c"

#include "game/DecalMP.c"

#include "game/Display.c"

#include "game/DotLights.c"

#include "game/DropRain.c"

#include "game/ElimBG.c"

#include "game/FLARE.c"

#include "game/GAMEPAD.c"

#include "game/GAMEPROG.c"

#include "game/GhostReplay.c"

#include "game/GhostTape.c"

#include "game/HOWL/HOWL_OtherFX.c"
#include "game/HOWL/HOWL_AudioLR.c"
#include "game/HOWL/HOWL_OtherFXRecycle.c"
#include "game/HOWL/HOWL_LevelAudio.c"
#include "game/HOWL/HOWL_Engine.c"
#include "game/HOWL/HOWL_Reverb.c"
#include "game/HOWL/HOWL_CseqMusic.c"
#include "game/HOWL/HOWL_Bank.c"
#include "game/HOWL/HOWL_Load.c"
#include "game/HOWL/HOWL_CseqOpcode.c"
#include "game/HOWL/HOWL_SongPool.c"
#include "game/HOWL/HOWL_Settings.c"
#include "game/HOWL/HOWL_Channel.c"
#include "game/HOWL/HOWL_Playback.c"
#include "game/HOWL/HOWL_Voiceline.c"
#include "game/HOWL/HOWL_AudioState.c"
#include "game/HOWL/HOWL_Music.c"
#include "game/HOWL/HOWL_Garage.c"

#include "game/INSTANCE.c"

#include "game/LHMatrix.c"

#include "game/JitPool.c"

#include "game/LevInstDef.c"

#include "game/LibraryOfModels/LibraryOfModels_0_Store.c"
#include "game/LibraryOfModels/LibraryOfModels_1_Clear.c"

#include "game/LinkedCollide.c"

#include "game/LIST.c"

#include "game/LOAD/LOAD_Callbacks.c"
#include "game/LOAD/LOAD_ModelPtrs.c"
#include "game/LOAD/LOAD_File.c"
#include "game/LOAD/LOAD_Howl.c"
#include "game/LOAD/LOAD_Assets.c"
#include "game/LOAD/LOAD_Queue.c"
#include "game/LOAD/LOAD_Hub.c"
#include "game/LOAD/LOAD_Overlays.c"
#include "game/LOAD/LOAD_TenStages.c"
#include "game/LOAD/LOAD_Level.c"
#include "game/LOAD/LOAD_IsOpen.c"

#include "game/MAIN/MainInit.c"
#include "game/MAIN/MainKillGame.c"
#include "game/MAIN/MainLoadVLC.c"
#include "game/MAIN/MainMain.c"
#include "game/MAIN/MainDB.c"
#include "game/MAIN/MainDrawCb.c"

#include "game/MAIN/MainFrame.c"
#include "game/MAIN/MainFrame_RenderFrame.c"

#include "game/MAIN/MainFreeze.c"

#include "game/MAIN/MainGameStart.c"
#include "game/MAIN/MainGameEnd.c"
#include "game/MAIN/MainStats.c"

#include "game/MAIN/MainRaceTrack.c"

#include "game/MATH/MATH_0_Sin.c"
#include "game/MATH/MATH_1_Cos.c"
#include "game/MATH/TRIG_AngleSinCos.c"
#include "game/MATH/MATH_2_FastSqrt.c"
#include "game/MATH/MATH_3_HitboxMatrix.c"
#include "game/MATH/MATH_7_MatrixStubs.c"
#include "game/MATH/MATH_MatrixRotate.c"
#include "game/MATH/MATH_6_MatrixMul.c"

#include "game/MEMCARD/MEMCARD_00_SetIcon.c"
#include "game/MEMCARD/MEMCARD_01_CRC16.c"
// NOTE(aalhendi): CTR_NATIVE routes host-backed card operations through MEMCARD_NativeAdapter; non-native builds use the retail card functions below.
#if defined(CTR_NATIVE)
#include "game/MEMCARD/MEMCARD_NativeAdapter.c"
#else
#include "game/MEMCARD/MEMCARD_16_GetFreeBytes.c"
#include "game/MEMCARD/MEMCARD_18_GetInfo.c"
#include "game/MEMCARD/MEMCARD_21_Format.c"
#include "game/MEMCARD/MEMCARD_22_IsFile.c"
#include "game/MEMCARD/MEMCARD_23_FindFirstGhost.c"
#include "game/MEMCARD/MEMCARD_24_FindNextGhost.c"
#include "game/MEMCARD/MEMCARD_25_EraseFile.c"
#endif
#include "game/MEMCARD/MEMCARD_02_ChecksumSave.c"
#include "game/MEMCARD/MEMCARD_03_ChecksumLoad.c"
#include "game/MEMCARD/MEMCARD_04_StringInit.c"
#include "game/MEMCARD/MEMCARD_05_StringSet.c"
#include "game/MEMCARD/MEMCARD_06_InitCard.c"
#include "game/MEMCARD/MEMCARD_07_CloseCard.c"
#include "game/MEMCARD/MEMCARD_08_GetNextSwEvent.c"
#include "game/MEMCARD/MEMCARD_09_GetNextHwEvent.c"
#include "game/MEMCARD/MEMCARD_10_WaitForHwEvent.c"
#include "game/MEMCARD/MEMCARD_11_SkipEvents.c"
#include "game/MEMCARD/MEMCARD_12_NewTask.c"
#include "game/MEMCARD/MEMCARD_13_CloseFile.c"
#include "game/MEMCARD/MEMCARD_14_ReadFile.c"
#include "game/MEMCARD/MEMCARD_15_WriteFile.c"
#if !defined(CTR_NATIVE)
#include "game/MEMCARD/MEMCARD_17_HandleEvent.c"
#endif
#if !defined(CTR_NATIVE)
#include "game/MEMCARD/MEMCARD_19_Load.c"
#include "game/MEMCARD/MEMCARD_20_Save.c"
#endif

#include "game/MEMPACK.c"

#include "game/MixRNG/PSX_BIOS_Rand.c"
#include "game/MixRNG/RngDeadCoed.c"
#include "game/MixRNG/MixRNG_0_Scramble.c"
#include "game/MixRNG/MixRNG_1_Particles.c"
#include "game/MixRNG/MixRNG_2_GetValue.c"

#include "game/Particle.c"

#include "game/PickupBots/PickupBots_0_Init.c"
#include "game/PickupBots/PickupBots_1_Update.c"

#include "game/PROC.c"

#include "game/PushBuffer.c"

#include "game/QueueLoadTrack.c"

#include "game/Podium/Podium_0_InitModels.c"

#include "game/RaceFlag.c"

#include "game/RECTMENU.c"

#include "game/RaceConfig.c"

#include "game/RefreshCard.c"

#include "game/SelectProfile.c"

#include "game/TakeCupProgress.c"

#include "game/SubmitName/SubmitName_0_RestoreName.c"
#include "game/SubmitName/SubmitName_1_DrawMenu.c"
#include "game/SubmitName/SubmitName_2_MenuProc.c"

#include "game/Timer/Timer_0_Init.c"
#include "game/Timer/Timer_1_Destroy.c"
#include "game/Timer/Timer_2_GetTime_Total.c"
#include "game/Timer/Timer_3_GetTime_Elapsed.c"

#include "game/prim.c"
#include "game/math.c"
#include "game/gte.c"
#include "game/MATH/MATH_4_VectorLength.c"
#include "game/MATH/MATH_5_VectorNormalize.c"
#include "game/UI/UI_Reward.c"
#include "game/UI/UI_Convert.c"
#include "game/UI/UI_Instance.c"
#include "game/UI/UI_Map.c"
#include "game/UI/UI_Icon.c"
#include "game/UI/UI_Lerp2D.c"
#include "game/UI/UI_Clock.c"
#include "game/UI/UI_RaceHud.c"
#include "game/UI/UI_Weapon.c"
#include "game/UI/UI_DrawNum.c"
#include "game/UI/UI_Speedometer.c"
#include "game/UI/UI_Meter.c"
#include "game/UI/UI_Rank.c"
#include "game/UI/UI_RenderFrame.c"
#include "game/UI/UI_VsQuip.c"
#include "game/UI/UI_RaceFlow.c"
#include "game/UI/UI_CupStandings.c"

#include "game/Vehicle/VehBirth.c"
#include "game/Vehicle/VehCalc.c"
#include "game/Vehicle/VehAfterColl.c"
#include "game/Vehicle/VehEmitter.c"
#include "game/Vehicle/VehFire.c"
#include "game/Vehicle/VehFrame.c"
#include "game/Vehicle/VehLap.c"
#include "game/Vehicle/VehPhysCrash.c"

#include "game/Vehicle/VehPhysForce.c"
#include "game/Vehicle/VehGroundShadow.c"
#include "game/Vehicle/VehPhysGeneral.c"
#include "game/Vehicle/VehPhysJoystick.c"
#include "game/Vehicle/VehGroundSkids.c"

#include "game/Vehicle/VehPhysProc.c"

#include "game/Vehicle/VehPickupItem.c"
#include "game/Vehicle/VehPickState.c"

#include "game/Vehicle/VehStuckProc.c"

#include "game/Vehicle/VehTurbo.c"
#include "game/Vehicle/VehTalkMask.c"

#include "game/Vector.c"

#include "game/221.c"
#include "game/222.c"
#include "game/223.c"
#include "game/224.c"
#include "game/225.c"

#include "game/221_225_EndEvent.c"

#include "game/PlayLevel/PlayLevel_0_UpdateLapStats.c"

#include "game/230/R230.c"
#include "game/230/D230.c"
#include "game/230/MM_Battle.c"
#include "game/230/MM_MenuFlow.c"
#include "game/230/MM_Title.c"
#include "game/230/MM_CheatCodes.c"
#include "game/230/MM_Characters.c"
#include "game/230/MM_TrackSelect.c"
#include "game/230/MM_CupSelect.c"
#include "game/230/MM_HighScore.c"
#include "game/230/MM_Scrapbook.c"
#include "game/230/MM_Video.c"

#include "game/231/R231.c"
#include "game/231/D231.c"
#include "game/231/RB_Instance.c"
#include "game/231/RB_Player.c"
#include "game/231/RB_MinePool.c"
#include "game/231/RB_Hazard.c"
#include "game/231/RB_Potion.c"
#include "game/231/RB_GenericMine.c"
#include "game/231/RB_TNT.c"
#include "game/231/RB_Explosion.c"
#include "game/231/RB_MovingExplosive.c"
#include "game/231/RB_Warpball.c"
#include "game/231/RB_MaskShieldCloud.c"
#include "game/231/RB_Blowup.c"
#include "game/231/RB_Burst.c"
#include "game/231/RB_Tracker.c"
#include "game/231/RB_Baron.c"
#include "game/231/RB_Blade.c"
#include "game/231/RB_Bubbles.c"
#include "game/231/RB_Crate.c"
#include "game/231/RB_Crystal.c"
#include "game/231/RB_Default.c"
#include "game/231/RB_CtrLetter.c"
#include "game/231/RB_Banner.c"
#include "game/231/RB_Armadillo.c"
#include "game/231/RB_Fireball.c"
#include "game/231/RB_FlameJet.c"
#include "game/231/RB_Follower.c"
#include "game/231/RB_Fruit.c"
#include "game/231/RB_Minecart.c"
#include "game/231/RB_Orca.c"
#include "game/231/RB_Plant.c"
#include "game/231/RB_Seal.c"
#include "game/231/RB_Snowball.c"
#include "game/231/RB_Spider.c"
#include "game/231/RB_StartText.c"
#include "game/231/RB_Teeth.c"
#include "game/231/RB_Turtle.c"

#include "game/232/R232.c"
#include "game/232/D232.c"

#include "game/232/232_Full.c"

#include "game/233/233_Full.c"

#endif
