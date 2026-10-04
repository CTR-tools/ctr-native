#ifndef GAME_UNITY_H
#define GAME_UNITY_H

#include <common.h>

#include "226/R226.c"
#include "227/R227.c"
#include "228/R228.c"
#include "229/R229.c"
#include "226/226_00_DrawLevelOvr1P.c"
#include "227/227_00_DrawLevelOvr2P.c"
#include "228/228_00_DrawLevelOvr3P.c"
#include "229/229_00_DrawLevelOvr4P.c"
#include "DrawConfetti.c"
#include "DrawSky.c"

#include "RenderBucket/RenderBucket_08_InitDepthGTE.c"
#include "RenderBucket/RenderBucket_QueueExecute.c"
#include "RenderLevel/AnimateWater.c"
#include "RenderLevel/RenderLists.c"
#include "DrawTires.c"
#include "RenderStars.c"
#include "Torch.c"
#include "RenderWeather/RedBeaker_RenderRain.c"
#include "RenderWeather/RenderWeather.c"

#include "CAM.c"

#include "BOTS.c"
#include "CDSYS.c"

#include "COLL.c"
#include "COLL/COLL_InstanceQuery.c"

#include "CTR/CTR_Box.c"
#include "CTR/CTR_CycleTex.c"
#include "CTR/CTR_RenderLists.c"
#include "CTR/CTR_Error.c"
#include "CTR/CTR_Visibility.c"
#include "CTR/CTR_Matrix.c"
#include "CTR/CTR_Ghost.c"

#include "DebugFont.c"

#include "DecalFont.c"

#include "DecalGlobal.c"

#include "DecalHUD.c"

#include "DecalMP.c"

#include "Display.c"

#include "DotLights.c"

#include "DropRain.c"

#include "ElimBG.c"

#include "FLARE.c"

#include "GAMEPAD.c"

#include "GAMEPROG.c"

#include "GhostReplay.c"

#include "GhostTape.c"

#include "HOWL/HOWL_OtherFX.c"
#include "HOWL/HOWL_AudioLR.c"
#include "HOWL/HOWL_OtherFXRecycle.c"
#include "HOWL/HOWL_LevelAudio.c"
#include "HOWL/HOWL_Engine.c"
#include "HOWL/HOWL_Reverb.c"
#include "HOWL/HOWL_CseqMusic.c"
#include "HOWL/HOWL_Bank.c"
#include "HOWL/HOWL_Load.c"
#include "HOWL/HOWL_CseqOpcode.c"
#include "HOWL/HOWL_SongPool.c"
#include "HOWL/HOWL_Settings.c"
#include "HOWL/HOWL_Channel.c"
#include "HOWL/HOWL_Playback.c"
#include "HOWL/HOWL_Voiceline.c"
#include "HOWL/HOWL_AudioState.c"
#include "HOWL/HOWL_Music.c"
#include "HOWL/HOWL_Garage.c"

#include "INSTANCE.c"

#include "LHMatrix.c"

#include "JitPool.c"

#include "LevInstDef.c"

#include "LibraryOfModels.c"

#include "LinkedCollide.c"

#include "LIST.c"

#include "LOAD/LOAD_Callbacks.c"
#include "LOAD/LOAD_ModelPtrs.c"
#include "LOAD/LOAD_File.c"
#include "LOAD/LOAD_Howl.c"
#include "LOAD/LOAD_Assets.c"
#include "LOAD/LOAD_Queue.c"
#include "LOAD/LOAD_Hub.c"
#include "LOAD/LOAD_Overlays.c"
#include "LOAD/LOAD_TenStages.c"
#include "LOAD/LOAD_Level.c"
#include "LOAD/LOAD_IsOpen.c"

#include "MAIN/MainInit.c"
#include "MAIN/MainKillGame.c"
#include "MAIN/MainLoadVLC.c"
#include "MAIN/MainMain.c"
#include "MAIN/MainDB.c"
#include "MAIN/MainDrawCb.c"

#include "MAIN/MainFrame.c"
#include "MAIN/MainFrame_RenderFrame.c"

#include "MAIN/MainFreeze.c"

#include "MAIN/MainGameStart.c"
#include "MAIN/MainGameEnd.c"
#include "MAIN/MainStats.c"

#include "MAIN/MainRaceTrack.c"

#include "MATH/MATH_0_Sin.c"
#include "MATH/MATH_1_Cos.c"
#include "MATH/TRIG_AngleSinCos.c"
#include "MATH/MATH_2_FastSqrt.c"
#include "MATH/MATH_3_HitboxMatrix.c"
#include "MATH/MATH_7_MatrixStubs.c"
// NOTE(aalhendi): Retail compiles this register-entry leaf separately.
#ifdef CTR_NATIVE
#include "MATH/MATH_LightMatrix.c"
#endif
#include "MATH/MATH_MatrixRotate.c"
#include "MATH/MATH_6_MatrixMul.c"

#include "MEMCARD/MEMCARD_Icon.c"
#include "MEMCARD/MEMCARD_Checksum.c"
#include "MEMCARD/MEMCARD_RetailChecksumLoad.c"
#include "MEMCARD/MEMCARD_String.c"
#include "MEMCARD/MEMCARD_Card.c"
#include "MEMCARD/MEMCARD_Events.c"
#include "MEMCARD/MEMCARD_FileIO.c"

// NOTE(aalhendi): Retail card operations below use PSX card/event/file APIs.
// Native selects their implementations from platform/native_memcard_adapter.c.
#if !defined(CTR_NATIVE)
#include "MEMCARD/MEMCARD_RetailInitCard.c"
#include "MEMCARD/MEMCARD_RetailCard.c"
#include "MEMCARD/MEMCARD_RetailHandleEvent.c"
#include "MEMCARD/MEMCARD_RetailTransfer.c"
#endif

#include "MEMPACK.c"

#include "MixRNG/PSX_BIOS_Rand.c"
#if !defined(CTR_NATIVE)
#include "PSX/PSX_BIOS_CallGates.c"
#include "PSX/PSX_SDK_Leaves.c"
#include "PSX/PSX_SDK_CDCore.c"
#include "PSX/PSX_SDK_CDState.c"
#endif
#include "MixRNG/RngDeadCoed.c"
#include "MixRNG/MixRNG.c"

#include "Particle.c"

#include "PickupBots.c"

#include "PROC.c"

#include "PushBuffer.c"

#include "QueueLoadTrack.c"

#include "Podium.c"

#include "RaceFlag.c"

#include "RECTMENU.c"

#include "RaceConfig.c"

#include "RefreshCard.c"

#include "SelectProfile.c"

#include "TakeCupProgress.c"

#include "SubmitName.c"

#include "Timer.c"

#include "prim.c"
#include "math.c"
#include "MATH/MATH_4_VectorLength.c"
#include "MATH/MATH_5_VectorNormalize.c"
#include "UI/UI_Reward.c"
#include "UI/UI_Convert.c"
#include "UI/UI_Instance.c"
#include "UI/UI_Map.c"
#include "UI/UI_Icon.c"
#include "UI/UI_Lerp2D.c"
#include "UI/UI_Clock.c"
#include "UI/UI_RaceHud.c"
#include "UI/UI_Weapon.c"
#include "UI/UI_DrawNum.c"
#include "UI/UI_Speedometer.c"
#include "UI/UI_Meter.c"
#include "UI/UI_Rank.c"
#include "UI/UI_RenderFrame.c"
#include "UI/UI_VsQuip.c"
#include "UI/UI_VsWait.c"
#include "UI/UI_RaceFlow.c"
#include "UI/UI_CupStandings.c"

#include "Vehicle/VehBirth.c"
#include "Vehicle/VehCalc.c"
#include "Vehicle/VehAfterColl.c"
#include "Vehicle/VehEmitter.c"
#include "Vehicle/VehFire.c"
#include "Vehicle/VehFrame.c"
#include "Vehicle/VehLap.c"
#include "Vehicle/VehPhysCrash.c"

#include "Vehicle/VehPhysForce.c"
#include "Vehicle/VehGroundShadow.c"
#include "Vehicle/VehPhysGeneral.c"
#include "Vehicle/VehPhysJoystick.c"
#include "Vehicle/VehGroundSkids.c"

#include "Vehicle/VehPhysProc.c"

#include "Vehicle/VehPickupItem.c"
#include "Vehicle/VehPickState.c"

#include "Vehicle/VehStuckProc.c"

#include "Vehicle/VehTurbo.c"
#include "Vehicle/VehTalkMask.c"

#include "Vector.c"

#include "221.c"
#include "222.c"
#include "223.c"
#include "224.c"
#include "225.c"

#include "221_225_EndEvent.c"

#include "PlayLevel.c"

#include "230.c"

#include "231/R231.c"
#include "231/D231.c"
#include "231/RB_Instance.c"
#include "231/RB_Player.c"
#include "231/RB_MinePool.c"
#include "231/RB_Hazard.c"
#include "231/RB_Potion.c"
#include "231/RB_GenericMine.c"
#include "231/RB_TNT.c"
#include "231/RB_Explosion.c"
#include "231/RB_MovingExplosive.c"
#include "231/RB_Warpball.c"
#include "231/RB_MaskShieldCloud.c"
#include "231/RB_Blowup.c"
#include "231/RB_Burst.c"
#include "231/RB_Tracker.c"
#include "231/RB_Baron.c"
#include "231/RB_Blade.c"
#include "231/RB_Bubbles.c"
#include "231/RB_Crate.c"
#include "231/RB_Crystal.c"
#include "231/RB_Default.c"
#include "231/RB_CtrLetter.c"
#include "231/RB_Banner.c"
#include "231/RB_Armadillo.c"
#include "231/RB_Fireball.c"
#include "231/RB_FlameJet.c"
#include "231/RB_Follower.c"
#include "231/RB_Fruit.c"
#include "231/RB_Minecart.c"
#include "231/RB_Orca.c"
#include "231/RB_Plant.c"
#include "231/RB_Seal.c"
#include "231/RB_Snowball.c"
#include "231/RB_Spider.c"
#include "231/RB_StartText.c"
#include "231/RB_Teeth.c"
#include "231/RB_Turtle.c"

#include "232/R232.c"
#include "232/D232.c"

#include "232/AH_WarpPad.c"
#include "232/AH_Garage.c"
#include "232/AH_SaveObj.c"
#include "232/AH_Door.c"
#include "232/AH_Map.c"
#include "232/AH_Pause.c"
#include "232/AH_HintMenu.c"
#include "232/AH_MaskHint.c"
#include "232/AH_Sign.c"

#include "233/CS_Instance.c"
#include "233/CS_ScriptCmd.c"
#include "233/CS_Podium.c"
#include "233/CS_Thread.c"
#include "233/CS_LoadBoss.c"
#include "233/CS_Camera.c"
#include "233/CS_BoxScene.c"
#include "233/CS_Cutscene.c"
#include "233/CS_Credits.c"
#include "233/CS_Garage.c"
#include "233/R233.c"
#include "233/D233.c"

#endif
