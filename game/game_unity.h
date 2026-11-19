#ifndef GAME_UNITY_H
#define GAME_UNITY_H

#include <common.h>

// Resident rendering/core code shares a unit with the native GTE implementation
// for inlining. Audio, UI, vehicles and overlays are separate CMake sources.

#include "LevelInfo.c"
#include "custom-cups/CustomCup.c"

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
#include "MAIN/MainFrame_Tow.c"
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
#include "MATH/MATH_MatrixRotate.c"
#include "MATH/MATH_6_MatrixMul.c"

#include "MEMCARD/MEMCARD_Icon.c"
#include "MEMCARD/MEMCARD_Checksum.c"
#include "MEMCARD/MEMCARD_String.c"
#include "MEMCARD/MEMCARD_Card.c"
#include "MEMCARD/MEMCARD_Events.c"
#include "MEMCARD/MEMCARD_FileIO.c"

// The game-facing MEMCARD_* API comes from
// platform/native_memcard_adapter.c; retail PSX card/event/file sources are
// not part of this build.

#include "MEMPACK.c"

#include "MixRNG/PSX_BIOS_Rand.c"
#include "MixRNG/RngDeadCoed.c"
#include "MixRNG/MixRNG.c"

#include "Particle.c"

#include "PickupBots.c"

#include "PROC.c"

#include "PushBuffer.c"

#include "QueueLoadTrack.c"

#include "Podium.c"

#include "RaceFlag.c"

#include "LANG.c"
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

#include "Vector.c"

#include "221.c"
#include "222.c"
#include "223.c"
#include "224.c"
#include "225.c"

#include "221_225_EndEvent.c"

#include "PlayLevel.c"

#endif
