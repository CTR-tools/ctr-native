#include <common.h>

struct CSThreadParentFrameScratch
{
	SVec3Slot parentPos;
	u8 pad_110[0x08];
	SVec3Slot parentRot;
};

CTR_STATIC_ASSERT_LAYOUT(offsetof(struct CSThreadParentFrameScratch, parentPos) == 0x00);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct CSThreadParentFrameScratch, parentRot) == 0x10);

static void CS_SaveDecodedOpcode(const struct CutsceneObj *cs, int out[5])
{
	const int *src = (const int *)&cs->decodedOpcode;
	out[0] = src[0];
	out[1] = src[1];
	out[2] = src[2];
	out[3] = src[3];
	out[4] = src[4];
}

static void CS_RestoreDecodedOpcode(struct CutsceneObj *cs, const int in[5])
{
	int *dst = (int *)&cs->decodedOpcode;
	dst[0] = in[0];
	dst[1] = in[1];
	dst[2] = in[2];
	dst[3] = in[3];
	dst[4] = in[4];
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800ac840-0x800ade8c
int CS_Thread_UseOpcode(struct Instance *instance, struct CutsceneObj *cs)
{
	u8 numPlayers;
	int frameBoundaryHit;
	s16 numCamPathPoints;
	s16 gameModeTarget;
	u16 clockEffectFlags;
	u16 cutsceneFlags;
	u32 conditionMet;
	int iVar8;
	char *const *cutsceneOpcodes;
	int iVar10;
	s16 levelToLoad;
	int distanceToScreen;
	struct Thread *dancerThread;
	char *opcodeAt;
	int iVar12;
	struct CsOpcodeMeta *opcodeMeta;
	s16 *opcodeMetaShorts;
	s16 *frameData;
	int rotInterpNumerator;
	int rotInterpStartFrame;
	int rotInterpFrameRange;
	int nextFrameTime;
	int lodIndex;
	struct ModelHeader *modelHeader;
	int metadataBackup[5];
	SVec3 camRot;
	SVec3 camPos;
	u16 camPathFlags[2];
	int animIndex;
	int opcodeDuration;
	int opcodeChanged;
	int elapsedTimeRemaining;

	struct GameTracker *gGT = sdata->gGT;
	CS_SaveDecodedOpcode(cs, metadataBackup);

	if (instance != 0)
	{
		if ((instance->flags & SPLIT_LINE) != 0)
			instance->vertSplit = D233.VertSplitLine;

		if ((int)instance->model->id == (u32)(u8)gGT->podium_modelIndex_Second)
		{
			if (D233.PodiumInitUnk2 - 0x65U < 0x87)
			{
				instance->flags |= HIDE_MODEL;
			}
			else
			{
				if ((instance->flags & HIDE_MODEL) == 0)
					goto afterPodiumSecondModelCheck;
				instance->depthBiasNormal -= 2;
				instance->depthBiasSecondary -= 2;
				instance->flags &= ~HIDE_MODEL;
			}
		}
	afterPodiumSecondModelCheck:

		if ((int)instance->model->id == (u32)(u8)gGT->podium_modelIndex_First)
		{
			if (D233.PodiumInitUnk2 - 0x83U < 0x69)
			{
				instance->flags |= HIDE_MODEL;
			}
			else
			{
				if ((instance->flags & HIDE_MODEL) == 0)
					goto afterPodiumFirstModelCheck;
				instance->depthBiasNormal -= 6;
				instance->depthBiasSecondary -= 6;
				instance->flags &= ~HIDE_MODEL;
			}
		}
	afterPodiumFirstModelCheck:

		if ((cs->flags & 0x80) != 0)
		{
			if (((int)instance->model->id + -0xce == (int)gGarage.unusedArr_garageChars[sdata->advCharSelectIndex_curr]) && (gGarage.boolSelected == 1))
			{
				if ((cs->flags & 0x100) == 0)
				{
					gGT->pushBuffer[0].fadeFromBlack_currentValue = 0x1fff;
					gGT->pushBuffer[0].fadeFromBlack_desiredResult = 0x1000;
					gGT->pushBuffer[0].fade_step = 0xfd56;
					cs->flags |= 0x100;
					CS_ScriptCmd_OpcodeAt(cs, R233.advCharSelectSelectOpcodes[(int)instance->model->id - STATIC_CRASHSELECT]);
					CS_SaveDecodedOpcode(cs, metadataBackup);
				reloadAdvCharSelectOpcodeState:
					cs->unk18 = ((int *)&cs->decodedOpcode)[2];
					iVar8 = MixRNG_Scramble();
					opcodeMeta = (struct CsOpcodeMeta *)cs->metadata;
					opcodeMetaShorts = (s16 *)opcodeMeta;
					cs->opcodeDuration =
					    opcodeMeta->frameStart + (s16)((int)((iVar8 >> 2 & 0xfff) * (((int)opcodeMeta->frameEnd - (int)opcodeMeta->frameStart) + 1)) >> 0xc);
				}
			}
			else
			{
				if ((cs->flags & 0x100) != 0)
				{
					cs->flags &= 0xfeff;
					CS_ScriptCmd_OpcodeAt(cs, R233.advCharSelectDeselectOpcodes[(int)instance->model->id - STATIC_CRASHSELECT]);
					CS_SaveDecodedOpcode(cs, metadataBackup);
					goto reloadAdvCharSelectOpcodeState;
				}
			}
		}
	}

	opcodeDuration = (int)cs->opcodeDuration;
	iVar12 = cs->unk18;
	iVar8 = (int)cs->unk1e;
	elapsedTimeRemaining = gGT->elapsedTimeMS;
	opcodeMeta = (struct CsOpcodeMeta *)cs->metadata;
	opcodeMetaShorts = (s16 *)opcodeMeta;
	animIndex = (int)opcodeMeta->animIndex;

	if (instance == 0)
	{
		numCamPathPoints = CAM_Path_GetNumPoints();
		if ((int)numCamPathPoints != 0)
		{
			iVar10 = ((s32)((u32)gGT->msInThisLEV << 11)) >> 16;
			if (iVar10 < (int)numCamPathPoints + -1)
			{
				CAM_Path_Move(iVar10, camPos.v, camRot.v, camPathFlags);
				gGT->pushBuffer[0].pos = camPos;
				gGT->pushBuffer[0].rot = camRot;
			}
			else
			{
				if (opcodeMeta->opcode == 0x14)
					CS_ScriptCmd_OpcodeNext(cs);
				CAM_Path_Move((int)(s16)(numCamPathPoints + -1), gGT->pushBuffer[0].pos.v, gGT->pushBuffer[0].rot.v, camPathFlags);
			}

			clockEffectFlags = gGT->clockEffectEnabled;
			gGT->clockEffectEnabled = clockEffectFlags & 0xfffe;
			if ((camPathFlags[0] & 1) != 0)
				gGT->clockEffectEnabled = clockEffectFlags & 0xfffe | 1;

			if ((cs->flags & 0x20) == 0)
			{
				gGT->pushBuffer[0].distanceToScreen_PREV = 0x100;
				if ((camPathFlags[0] & 2) != 0)
					gGT->pushBuffer[0].distanceToScreen_PREV = 0x50;
				if ((camPathFlags[0] & 4) != 0)
					gGT->pushBuffer[0].distanceToScreen_PREV = 0x278;
				if ((camPathFlags[0] & 0x20) != 0)
					gGT->pushBuffer[0].distanceToScreen_PREV = 0x1eb;
				if ((camPathFlags[0] & 0x40) != 0)
					gGT->pushBuffer[0].distanceToScreen_PREV = 0x14d;
			}

			if (((camPathFlags[0] & 0x10) != 0) && ((MixRNG_Scramble() & 0xf) == 0))
			{
				CTR_Box_DrawClearBox(&R233.introClearBoxRect, &R233.introClearBoxColor, 1, gGT->backBuffer->otMem.uiOT);
			}

			if (gGT->levelID == 0x29)
				gGT->pushBuffer[0].distanceToScreen_PREV = 0x140;

			gGT->pushBuffer[0].distanceToScreen_CURR = gGT->pushBuffer[0].distanceToScreen_PREV;
		}

		if ((sdata->gGamepads->gamepad[0].buttonsTapped & BTN_START) != 0)
		{
			gGT->clockEffectEnabled &= 0xfffe;
			if (0x13 < gGT->levelID - 0x2cU)
			{
				if (gGT->levelID == 0x29)
				{
					if ((u32)gGT->msInThisLEV >> 5 < 0xb5)
						goto afterCameraAndSkipChecks;
					RaceFlag_SetCanDraw(1);
					iVar8 = RaceFlag_IsTransitioning();
					if ((iVar8 == 0) && (iVar8 = RaceFlag_IsFullyOnScreen(), iVar8 == 0))
						RaceFlag_SetFullyOffScreen();
				}
				else
				{
					RaceFlag_SetCanDraw(1);
					iVar8 = RaceFlag_IsTransitioning();
					if ((iVar8 == 0) && (iVar8 = RaceFlag_IsFullyOnScreen(), iVar8 == 0))
						RaceFlag_SetFullyOffScreen();
					levelToLoad = CREDITS_CRASH;
					if (gGT->levelID - 0x2aU < 2)
						goto requestSkipLevelLoad;
				}
				CseqMusic_StopAll();
				CDSYS_XAPauseRequest();
				RaceFlag_SetDrawOrder(0);
				levelToLoad = MAIN_MENU_LEVEL;
			requestSkipLevelLoad:
				MainRaceTrack_RequestLoad(levelToLoad);
				D233.isCutsceneOver = 1;
				gGT->gameMode2 &= ~VEH_FREEZE_PODIUM;
				CS_RestoreDecodedOpcode(cs, metadataBackup);
				return 1;
			}
			CS_Credits_End();
		}
	}

afterCameraAndSkipChecks:
	opcodeChanged = 0;
	if (elapsedTimeRemaining == 0)
	{
	updateInstanceAndReturn:
		cs->unk18 = iVar12;
		cs->animIndex = (char)animIndex;
		cs->unk1e = (s16)iVar8;
		cs->opcodeDuration = (s16)opcodeDuration;
		iVar10 = (int)opcodeMeta->rotStart;
		iVar12 = iVar12 >> 5;
		if (iVar10 != (int)opcodeMeta->rotEnd)
		{
			rotInterpStartFrame = opcodeMeta->arg0.i;
			if (opcodeMeta->arg1.i != rotInterpStartFrame)
			{
				rotInterpNumerator = ((((int)opcodeMeta->rotEnd - iVar10) + 0x800U & 0xfff) - 0x800) * (iVar12 - rotInterpStartFrame);
				rotInterpFrameRange = opcodeMeta->arg1.i - rotInterpStartFrame;
				if (rotInterpFrameRange < 0)
					rotInterpFrameRange = -rotInterpFrameRange;
				iVar10 = iVar10 + rotInterpNumerator / rotInterpFrameRange;
			}
		}
		iVar10 = iVar10 + cs->unk1c;
		if ((iVar10 != (int)cs->rot.y) && (cs->rot.y = (s16)iVar10, instance != 0))
		{
			ConvertRotToMatrix(&instance->matrix, &cs->rot);
		}
		iVar8 = CS_Instance_SafeCheckAnimFrame(instance, animIndex, iVar8, iVar12);
		if (iVar12 != iVar8)
		{
			animIndex &= ~0xff;
			iVar12 = 0;
		}
		if (instance != 0)
		{
			instance->animFrame = (s16)iVar12;
			instance->animIndex = (char)animIndex;
		}
		if (cs->frameOverrideRoot != 0)
		{
			frameData = (s16 *)((uintptr_t)(*cs->frameOverrideRoot) + iVar12 * 0x20);
			*(int *)((u8 *)&instance->matrix + 0x00) = *(int *)(frameData + 4);
			*(int *)((u8 *)&instance->matrix + 0x04) = *(int *)(frameData + 6);
			*(int *)((u8 *)&instance->matrix + 0x08) = *(int *)(frameData + 8);
			*(int *)((u8 *)&instance->matrix + 0x0c) = *(int *)(frameData + 10);
			*(int *)((u8 *)&instance->matrix + 0x10) = *(int *)(frameData + 0xc);
			instance->matrix.t[0] = (int)*frameData;
			instance->matrix.t[1] = (int)frameData[1];
			instance->matrix.t[2] = (int)frameData[2];
		}
		return 0;
	}

processOpcode:
	switch (opcodeMeta->opcode)
	{
	case 0:
	case 0x2a:
	case 0x2b:
		if (instance != 0)
		{
			cutsceneFlags = cs->flags;
			if ((cutsceneFlags & 0x200) != 0)
			{
				if (((cutsceneFlags & 0x400) == 0) && (sdata->XA_State == 3))
					cs->flags = cutsceneFlags | 0x400;
				if (sdata->XA_State != 0)
				{
					if ((cs->flags & 0x400) == 0)
						iVar12 = 0;
					else
					{
						iVar12 = CS_Instance_SafeCheckAnimFrame(instance, animIndex, iVar8, (sdata->XA_CurrOffset * 0x1e00) / 0xac44);
						iVar12 = iVar12 << 5;
					}
					if (opcodeMeta->arg1.i << 5 < iVar12)
						break;
					goto updateInstanceAndReturn;
				}
				break;
			}
		}
		if (opcodeChanged != 0)
		{
			animIndex = (int)opcodeMeta->animIndex;
			iVar12 = CS_Instance_SafeCheckAnimFrame(instance, animIndex, iVar8, opcodeMeta->arg0.i);
			iVar12 = iVar12 << 5;
			iVar10 = MixRNG_Scramble();
			opcodeChanged = 0;
			opcodeDuration =
			    ((int)((iVar10 >> 2 & 0xfff) * (((int)opcodeMeta->frameEnd - (int)opcodeMeta->frameStart) + 1)) >> 0xc) + (int)opcodeMeta->frameStart;
		}
		frameBoundaryHit = 0;
		if (opcodeMeta->arg1.i < opcodeMeta->arg0.i)
		{
			iVar10 = opcodeMeta->arg1.i * 0x20;
			iVar12 = iVar12 - elapsedTimeRemaining;
			if (iVar12 < iVar10)
			{
				elapsedTimeRemaining = iVar10 - iVar12;
			markAnimationBoundary:
				frameBoundaryHit = 1;
			}
		}
		else
		{
			iVar10 = CS_Instance_SafeCheckAnimFrame(instance, animIndex, iVar8, opcodeMeta->arg1.i);
			nextFrameTime = (iVar10 + 1) * 0x20;
			iVar12 = iVar12 + elapsedTimeRemaining;
			if (nextFrameTime <= iVar12)
			{
				frameBoundaryHit = 1;
				elapsedTimeRemaining = 0;
				if (nextFrameTime != 0)
				{
					elapsedTimeRemaining = iVar12 + (iVar10 + 1) * -0x20;
					goto markAnimationBoundary;
				}
			}
		}
		if ((frameBoundaryHit) || (opcodeDuration < 1))
		{
			opcodeDuration = opcodeDuration + -1;
			if (opcodeDuration < 1)
			{
				CS_ScriptCmd_OpcodeNext(cs);
				opcodeChanged = 1;
			}
			else
			{
				iVar12 = CS_Instance_SafeCheckAnimFrame(instance, animIndex, iVar8, opcodeMeta->arg0.i);
				iVar12 = iVar12 << 5;
			}
		}
		else
		{
			elapsedTimeRemaining = 0;
		}
		goto finishOpcodeStep;

	case 1:
		opcodeChanged = 1;
		CS_ScriptCmd_OpcodeAt(cs, opcodeMeta->arg1.ptr);
		goto finishOpcodeStep;

	case 2:
		if (instance != 0)
			instance->flags |= HIDE_MODEL;
		CS_RestoreDecodedOpcode(cs, metadataBackup);
		return 1;

	case 3:
		if (instance != 0)
		{
			// Retail builds this opcode 3 init data at scratchpad 0x1f800108.
			struct CsThreadInitData *initData = CTR_SCRATCHPAD_PTR(struct CsThreadInitData, 0x108);
			int spawnModelID = opcodeMeta->arg1.i;

			CS_Instance_GetFrameData(instance, (int)opcodeMeta->animIndex, opcodeMeta->arg0.i, (u16 *)initData->podiumPos.v, (u16 *)initData->rot.v, 0);

			initData->podiumPos.x += (s16)instance->matrix.t[0];
			initData->podiumPos.y += (s16)instance->matrix.t[1];
			initData->podiumPos.z += (s16)instance->matrix.t[2];
			initData->characterPos.x = 0;
			initData->characterPos.y = 0;
			initData->characterPos.z = 0;

			if (spawnModelID == NDI_BOX_PARTICLES_01)
			{
				initData->rot.x = 0;
				initData->rot.y = 0;
				initData->rot.z = 0;
			}

			CS_Thread_Init(spawnModelID, R233.s_spawn, (s16 *)initData, 0, instance->thread);
		}
		break;

	case 4:
		iVar10 = MixRNG_Scramble();
		if (opcodeMeta->arg0.i < (int)(iVar10 >> 2 & 0xff))
			CS_ScriptCmd_OpcodeNext(cs);
		else
			CS_ScriptCmd_OpcodeAt(cs, opcodeMeta->arg1.ptr);
		opcodeChanged = 1;
		goto finishOpcodeStep;

	case 5:
		if (gGT->levelID == 0x28)
		{
			if (instance != 0)
				Garage_PlayFX(opcodeMeta->arg1.u, (int)instance->model->id + -0xce);
		}
		else
		{
			iVar10 = CS_Instance_BoolPlaySound(cs, instance);
			if (iVar10 != 0)
				OtherFX_Play((u32)(u16)opcodeMetaShorts[6], 1);
		}
		break;

	case 6:
		OtherFX_Stop2((u32)(u16)opcodeMetaShorts[6]);
		break;

	case 7:
		CseqMusic_Start((u32)(u16)opcodeMetaShorts[6], 0, 0, 0, opcodeMeta->arg0.i);
		break;

	case 8:
		CseqMusic_Restart((u32)(u16)opcodeMetaShorts[6], 1);
		break;

	case 9:
		if (instance != 0)
		{
			iVar10 = (int)instance->model->numHeaders;
			if ((iVar10 != 0) && (modelHeader = instance->model->headers, modelHeader != 0))
			{
				lodIndex = opcodeMeta->arg1.i;
				iVar8 = lodIndex;
				if (iVar10 <= lodIndex)
				{
					lodIndex = iVar10 + -1;
					iVar8 = lodIndex;
				}
				while (lodIndex != 0)
				{
					modelHeader->maxDistanceLOD = 0;
					lodIndex = lodIndex + -1;
					modelHeader++;
				}
				modelHeader->maxDistanceLOD = 20000;
			}
		}
		break;

	case 10:
		if (opcodeMeta->arg1.i == -1)
			cutsceneFlags = cs->flags | CS_FLAG_PATH_MOTION_DISABLED;
		else
		{
			cs->pathProgress32 = 0;
			cutsceneFlags = cs->flags & ~CS_FLAG_PATH_MOTION_DISABLED;
		}
		cs->flags = cutsceneFlags;
		break;

	case 0xb:
		cs->desiredScale = opcodeMetaShorts[4];
		cs->scaleSpeed = opcodeMetaShorts[6];
		CS_ScriptCmd_OpcodeNext(cs);
		goto finishOpcodeStep;

	case 0xc:
		gGT->pushBuffer[0].fadeFromBlack_currentValue = 0x1fff;
		gGT->pushBuffer[0].fadeFromBlack_desiredResult = 0x1000;
		gGT->pushBuffer[0].fade_step = 0xfd56;
		CS_ScriptCmd_OpcodeNext(cs);
		goto finishOpcodeStep;

	case 0xd:
		cutsceneFlags = cs->flags | opcodeMetaShorts[6];
		goto setFlagsAndAdvanceOpcode;

	case 0xe:
		cs->flags &= ~opcodeMetaShorts[6];
		CS_ScriptCmd_OpcodeNext(cs);
		goto finishOpcodeStep;

	case 0xf:
		gGT->bool_AdvHub_NeedToSwapLEV = 1;
		if ((gGT->gameMode2 & CREDITS) == 0)
		{
			cutsceneOpcodes = R233.introCutsceneOpcodes;
			iVar10 = gGT->levelID + -0x1e;
		}
		else
		{
			cutsceneOpcodes = R233.creditsCutsceneOpcodes;
			iVar10 = gGT->levelID + -0x2c;
		}
		CS_ScriptCmd_OpcodeAt(cs, cutsceneOpcodes[iVar10]);
		goto updateInstanceAndReturn;

	case 0x10:
		iVar10 = opcodeMeta->arg1.i;
		gGT->levelID = iVar10;
		if (iVar10 == 0x1e)
		{
			RaceFlag_SetCanDraw(0);
		requestDirectLevelLoad:
			gGT->gameMode2 &= ~VEH_FREEZE_PODIUM;
			MainRaceTrack_RequestLoad((int)(s16)iVar10);
		}
		else
		{
			if (iVar10 < 0x1f)
			{
				if (iVar10 == 0x19)
				{
					levelToLoad = GEM_STONE_VALLEY;
				requestMappedLevelLoad:
					gGT->gameMode2 &= ~VEH_FREEZE_PODIUM;
					MainRaceTrack_RequestLoad(levelToLoad);
					break;
				}
			}
			else
			{
				if (iVar10 == 0x27)
				{
					RaceFlag_SetDrawOrder(0);
					levelToLoad = MAIN_MENU_LEVEL;
					goto requestMappedLevelLoad;
				}
				if (iVar10 == 0x2c)
					goto requestDirectLevelLoad;
			}
			D233.boolLoadNextSwap = 1;
			LOAD_Hub_ReadFile(sdata->ptrBigfileCdPos_2, iVar10, 3 - (int)gGT->activeMempackIndex);
		}
		break;

	case 0x11:
		if ((D233.boolLoadNextSwap == 0) || (sdata->queueReady == 0) || (sdata->queueLength != 0))
			goto updateInstanceAndReturn;
		break;

	case 0x12:
		CDSYS_XAPlay(opcodeMeta->arg0.i, opcodeMeta->arg1.i);
		if (sdata->XA_State != 0)
			cs->flags = cs->flags & 0xfbff | 0x200;
		break;

	case 0x13:
		if (sdata->XA_State == 0)
		{
			cutsceneFlags = cs->flags & 0xfdff;
			goto setFlagsAndAdvanceOpcode;
		}

	case 0x14:
		goto updateInstanceAndReturn;

	case 0x15:
		numPlayers = gGT->numPlyrCurrGame;
		gGT->stars.numStars = (s16)((int)gGT->level1->stars.numStars / (int)(u32)numPlayers);
		gGT->stars.spread = gGT->level1->stars.spread;
		gGT->stars.seed = gGT->level1->stars.seed;
		gGT->stars.distance = gGT->level1->stars.distance;
		D233.boolLoadNextSwap = 0;
		CS_ScriptCmd_OpcodeNext(cs);
		goto finishOpcodeStep;

	case 0x16:
		iVar10 = RaceFlag_IsFullyOffScreen();
		if (iVar10 == 1)
		{
			RaceFlag_SetCanDraw(1);
			RaceFlag_BeginTransition(1);
		}
		break;

	case 0x17:
		conditionMet = RaceFlag_IsFullyOnScreen();
		goto advanceIfConditionMet;

	case 0x18:
		iVar10 = RaceFlag_IsFullyOnScreen();
		if (iVar10 == 1)
			RaceFlag_BeginTransition(2);
		break;

	case 0x19:
		cs->particleID = opcodeMetaShorts[6];
		CS_ScriptCmd_OpcodeNext(cs);
		goto finishOpcodeStep;

	case 0x1a:
		if (instance != 0)
			instance->flags |= HIDE_MODEL;
		break;

	case 0x1b:
		if (instance != 0)
			instance->flags &= ~HIDE_MODEL;
		break;

	case 0x1c:
		if (instance != 0)
		{
			instance->depthBiasNormal += (char)opcodeMeta->arg1.i;
			instance->depthBiasSecondary += (char)opcodeMeta->arg1.i;
		}
		break;

	case 0x1d:
		if (instance != 0)
			instance->flags |= opcodeMeta->arg1.u;
		break;

	case 0x1e:
		if (instance != 0)
			instance->flags &= ~opcodeMeta->arg1.u;
		break;

	case 0x1f:
		cs->unk4 = 0x1333;
		break;

	case 0x20:
		D233.isCutsceneOver = 1;
		CS_DestroyPodium_StartDriving();
		D233.bossCutsceneIndex = -1;
		gGT->overlayTransition = 3;
		gGT->gameMode2 &= ~VEH_FREEZE_PODIUM;
		CS_ScriptCmd_OpcodeNext(cs);
		goto finishOpcodeStep;

	case 0x21:
		D233.bossCutsceneIndex = opcodeMeta->arg1.i;
		if ((D233.bossCutsceneIndex == 0) && (0x11 < gGT->currAdvProfile.numRelics))
			D233.bossCutsceneIndex = 9;
		D233.cutsceneState = CS_WAIT_INPUT;
		break;

	case 0x22:
		distanceToScreen = opcodeMeta->arg1.i;
		gGT->pushBuffer[0].distanceToScreen_PREV = distanceToScreen;
		gGT->pushBuffer[0].distanceToScreen_CURR = distanceToScreen;
		cutsceneFlags = cs->flags | 0x20;
	setFlagsAndAdvanceOpcode:
		cs->flags = cutsceneFlags;
		CS_ScriptCmd_OpcodeNext(cs);
		goto finishOpcodeStep;

	case 0x23:
		conditionMet = CS_Credits_IsTextValid();
		goto advanceIfConditionMet;

	case 0x24:
	{
		// Retail builds this credits dancer init data at scratchpad 0x1f800108.
		struct CsThreadInitData *initData = CTR_SCRATCHPAD_PTR(struct CsThreadInitData, 0x108);
		int dancerModelID = opcodeMeta->arg1.i;

		initData->podiumPos.x = 0;
		initData->podiumPos.y = 0;
		initData->podiumPos.z = 0;
		initData->rot.x = 0;
		initData->rot.y = 0;
		initData->rot.z = 0;
		initData->characterPos.x = 0;
		initData->characterPos.y = 0;
		initData->characterPos.z = 0;

		gGT->podium_modelIndex_First = (u8)dancerModelID;
		gGT->podium_modelIndex_Second = 0;
		gGT->podium_modelIndex_Third = 0;

		if (dancerModelID == STATIC_OXIDEDANCE)
		{
			gGT->podium_modelIndex_First = 0;
			gGT->podium_modelIndex_Second = STATIC_OXIDEDANCE;
		}
		if (dancerModelID == STATIC_CRASHDANCE)
			initData->rot.y += 0x800;

		initData->rot.x += R233.creditsDancerRotOffset.x;
		initData->rot.y += R233.creditsDancerRotOffset.y;
		initData->rot.z += R233.creditsDancerRotOffset.z;

		dancerThread = (struct Thread *)CS_Thread_Init(dancerModelID, R233.s_g_dancer, (s16 *)initData, 0, 0);
		CS_Credits_NewDancer(dancerThread, (int)opcodeMetaShorts[6]);
	}
	break;

	case 0x25:
		conditionMet = CS_Credits_NewCreditGhosts();
	advanceIfConditionMet:
		conditionMet &= 0xffff;
		if (conditionMet == 0)
			goto updateInstanceAndReturn;
		break;

	case 0x26:
		if (opcodeMeta->frameEnd == 0)
		{
			if ((opcodeMeta->arg0.i != (int)gGarage.unusedArr_garageChars[sdata->advCharSelectIndex_curr]) || (gGarage.boolSelected == 0))
			{
				opcodeAt = opcodeMeta->arg1.ptr;
			branchToGarageOpcode:
				opcodeChanged = 1;
				CS_ScriptCmd_OpcodeAt(cs, opcodeAt);
			}
		}
		else
		{
			if ((opcodeMeta->arg0.i == (int)gGarage.unusedArr_garageChars[sdata->advCharSelectIndex_curr]) && (gGarage.boolSelected == 1))
			{
				opcodeAt = opcodeMeta->arg1.ptr;
				goto branchToGarageOpcode;
			}
		}
		break;

	case 0x27:
		if ((u32)gGT->msInThisLEV >> 5 < opcodeMeta->arg1.u)
			goto updateInstanceAndReturn;
		break;

	case 0x28:
		iVar12 = CS_Instance_SafeCheckAnimFrame(instance, animIndex, iVar8, iVar12 >> 5);
		iVar12 = iVar12 << 5;
		goto updateInstanceAndReturn;

	case 0x29:
		CS_Credits_End();
		CS_RestoreDecodedOpcode(cs, metadataBackup);
		return 1;

	case 0x2c:
		gameModeTarget = opcodeMeta->animIndex;
		if (gameModeTarget == 1)
			gGT->gameMode2 |= opcodeMeta->arg1.u;
		else
		{
			if (gameModeTarget < 2)
			{
				if (gameModeTarget == 0)
					gGT->gameMode1 |= opcodeMeta->arg1.u;
			}
			else
			{
				if (gameModeTarget == 2)
					gGT->renderFlags |= opcodeMeta->arg1.u;
				else
				{
					if (gameModeTarget == 3)
						gGT->renderFlags &= ~opcodeMeta->arg1.u;
				}
			}
		}
		break;

	case 0x2d:
		cs->Subtitles.textPos.x = opcodeMeta->animIndex;
		cs->Subtitles.textPos.y = opcodeMeta->frameStart;
		cs->Subtitles.lngIndex = opcodeMeta->frameEnd;
		cs->Subtitles.font = opcodeMeta->rotStart;
		cs->Subtitles.colors = opcodeMeta->rotEnd;
		CS_ScriptCmd_OpcodeNext(cs);
		goto finishOpcodeStep;

	case 0x2e:
		gGT->pushBuffer_UI.fadeFromBlack_desiredResult = 0;
		gGT->pushBuffer_UI.fade_step = 0xfd56;
		CS_ScriptCmd_OpcodeNext(cs);
		goto finishOpcodeStep;

	case 0x2f:
		if (0 < gGT->pushBuffer_UI.fadeFromBlack_currentValue)
			goto updateInstanceAndReturn;
		break;

	case 0x30:
		D233.CutsceneManipulatesAudio = 1;
		howl_VolumeSet(0, (u32) * ((u8 *)opcodeMeta + 2));
		howl_VolumeSet(1, (u32) * ((u8 *)opcodeMeta + 4));
		howl_VolumeSet(2, (u32) * ((u8 *)opcodeMeta + 6));
		break;

	default:
		CS_RestoreDecodedOpcode(cs, metadataBackup);
		return 0;
	}

	CS_ScriptCmd_OpcodeNext(cs);

finishOpcodeStep:
	if ((elapsedTimeRemaining != 0) || (opcodeChanged != 0))
		goto processOpcode;
	goto updateInstanceAndReturn;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800ae2b8-0x800ae318
void CS_Thread_AnimateScale(struct Thread *t)
{
	struct Instance *inst = t->inst;
	struct CutsceneObj *cs = t->object;

	if (!inst)
		return;

	if (cs->scaleSpeed == 0)
		return;

	int newScale = (int)inst->scale.x + (int)cs->scaleSpeed;
	int desiredScale = (int)cs->desiredScale;

	if (cs->scaleSpeed > 0)
	{
		if (newScale >= desiredScale)
		{
			newScale = desiredScale;
			cs->scaleSpeed = 0;
		}
	}
	else
	{
		if (newScale <= desiredScale)
		{
			newScale = desiredScale;
			cs->scaleSpeed = 0;
		}
	}

	inst->scale.x = (s16)newScale;
	inst->scale.y = (s16)newScale;
	inst->scale.z = (s16)newScale;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800ade8c-0x800ae2b8
void CS_Thread_MoveOnPath(struct Thread *t)
{
	struct CutsceneObj *cs = t->object;
	struct Instance *inst = t->inst;
	struct Level *level;
	struct GameTracker *gGT;
	s16 modelID;
	int switchVal;
	int digit;
	struct SpawnType2 *spawnEntry;
	SVec3 *positions;
	SVec3 *curr;
	SVec3 *next;
	struct SpawnPosRot *posRot;
	u16 progress;
	int idx;
	u16 frac;
	SVec3 rot;

	if ((cs->flags & CS_FLAG_PATH_MOTION_DISABLED) != 0)
		return;

	if (inst == 0)
		return;

	modelID = inst->model->id;
	switchVal = (s16)(modelID - 0xA1);

	if ((u32)switchVal >= 63)
		return;

	gGT = sdata->gGT;
	level = gGT->level1;

	switch (switchVal)
	{
	case 0x00:
	case 0x3E:

		digit = (u8)inst->name[strlen(inst->name) - 1] - '0';

		if (level->numSpawnType2 <= digit)
			return;

		spawnEntry = (struct SpawnType2 *)((char *)level->ptrSpawnType2 + digit * 8);
		positions = spawnEntry->positions;

		if (positions == 0)
			return;

		progress = cs->pathProgress32;
		idx = (s16)progress >> 5;
		cs->pathProgress32 = (u16)(progress + (u16)gGT->elapsedTimeMS);
		frac = progress & 0x1f;

		if (idx >= spawnEntry->numCoords - 1)
		{
			idx = 0;

			if (modelID == 0xDF)
			{
				idx = spawnEntry->numCoords - 2;
				cs->pathProgress32 = idx << 5;
			}
			else
			{
				cs->pathProgress32 = 0;
			}
		}

		curr = &positions[idx];
		next = &curr[1];

		inst->matrix.t[0] = curr->x + ((frac * (next->x - curr->x)) >> 5);
		inst->matrix.t[1] = curr->y + ((frac * (next->y - curr->y)) >> 5);
		inst->matrix.t[2] = curr->z + ((frac * (next->z - curr->z)) >> 5);

		if (idx >= spawnEntry->numCoords - 1)
			return;

		if (modelID == 0xDF)
			return;

		rot.x = cs->rot.x;
		rot.y = cs->rot.y + ratan2(next->x - curr->x, next->z - curr->z);
		rot.z = cs->rot.z;

		ConvertRotToMatrix(&inst->matrix, &rot);
		return;

	case 0x01:
	case 0x02:
	case 0x39:
	case 0x3A:

		digit = (u8)inst->name[strlen(inst->name) - 1] - '0';

		if (level->numSpawnType2_PosRot <= digit)
			return;

		spawnEntry = (struct SpawnType2 *)((char *)level->ptrSpawnType2_PosRot + digit * 8);
		posRot = spawnEntry->posRot;

		if (posRot == 0)
			return;

		progress = cs->pathProgress32;
		cs->pathProgress32 = (u16)(progress + (u16)gGT->elapsedTimeMS);
		idx = (s16)progress >> 5;

		if (idx >= spawnEntry->numCoords - 1)
		{
			idx = 0;
			cs->pathProgress32 = 0;
		}

		{
			struct SpawnPosRot *frame = &posRot[idx];

			inst->matrix.t[0] = frame->pos.x;
			inst->matrix.t[1] = frame->pos.y;
			inst->matrix.t[2] = frame->pos.z;

			rot = frame->rot;
		}

		break;

	case 0x30:

		if (level->numSpawnType2 <= 0)
			return;

		spawnEntry = level->ptrSpawnType2;
		positions = spawnEntry->positions;

		if (positions == 0)
			return;

		{
			int prog = 0;

			if (cs->animIndex == 3)
				prog = cs->unk18;

			frac = prog & 0x1f;
			int numCoords = spawnEntry->numCoords;
			idx = prog >> 5;

			if (idx < numCoords - 1)
			{
				if (idx >= 0)
				{
					curr = &positions[idx];
					next = &curr[1];
				}
				else
				{
					curr = &positions[0];
					next = curr;
				}
			}
			else
			{
				curr = &positions[numCoords - 1];
				next = curr;
			}

			inst->matrix.t[0] = curr->x + ((frac * (next->x - curr->x)) >> 5);
			inst->matrix.t[1] = curr->y + ((frac * (next->y - curr->y)) >> 5);
			inst->matrix.t[2] = curr->z + ((frac * (next->z - curr->z)) >> 5);
		}

		return;

	default:
		return;
	}

	ConvertRotToMatrix(&inst->matrix, &rot);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800abdd4-0x800abf70
void CS_Thread_Particles(struct Thread *t)
{
	struct CutsceneObj *cs = t->object;
	struct Instance *inst = t->inst;
	const struct CsParticleConfig *entry;
	s8 particleID;

	if (inst == NULL)
		return;

	if ((inst->flags & HIDE_MODEL) != 0)
		return;

	particleID = cs->particleID;
	if ((u8)particleID >= 9)
		return;

	entry = &R233.particleConfigs[(int)particleID];

	while (1)
	{
		int iconGroupIndex = entry->meta.iconGroupIndex;
		int frameOffset = entry->meta.frameOffset;
		int count = entry->meta.count;
		int flags = entry->meta.flags;
		s8 modelDelta = entry->spawn.modelDelta;

		for (int i = 0; i < count; i++)
		{
			struct Particle *p = Particle_Init(0, sdata->gGT->iconGroup[iconGroupIndex], entry->emitter);

			if (p != NULL)
			{
				SVec3 pos;

				CS_Instance_GetFrameData(inst, inst->animIndex, inst->animFrame, (u16 *)pos.v, NULL, frameOffset);

				p->axis[0].startVal += (pos.x + inst->matrix.t[0]) << 8;
				p->axis[1].startVal += (pos.y + inst->matrix.t[1]) << 8;
				p->axis[2].startVal += (pos.z + inst->matrix.t[2]) << 8;
				p->otIndexOffset = inst->depthBiasNormal + modelDelta;
			}
		}

		if ((flags & 1) == 0)
			break;

		entry++;
	}
}

struct CSInterpolateLinePacket
{
	u32 tag;
	u32 drawMode;
	u32 pad;
	u32 colorAndCode;
	u32 xy0;
	u32 xy1;
};

CTR_STATIC_ASSERT_LAYOUT(sizeof(struct CSInterpolateLinePacket) == 0x18);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct CSInterpolateLinePacket, tag) == 0x00);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct CSInterpolateLinePacket, drawMode) == 0x04);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct CSInterpolateLinePacket, pad) == 0x08);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct CSInterpolateLinePacket, colorAndCode) == 0x0C);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct CSInterpolateLinePacket, xy0) == 0x10);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct CSInterpolateLinePacket, xy1) == 0x14);

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800ae318-0x800ae54c
void CS_Thread_InterpolateFramesMS(struct Thread *t)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Instance *inst = t->inst;
	struct PrimMem *primMem;
	struct CSInterpolateLinePacket *packet;
	void *end;
	u16 curr[3];
	u16 next[3];
	int depth;

	CS_Instance_GetFrameData(inst, inst->animIndex, inst->animFrame, curr, NULL, 0);
	CS_Instance_GetFrameData(inst, inst->animIndex, inst->animFrame, next, NULL, 1);

	curr[0] = (u16)(curr[0] + (u16)inst->matrix.t[0]);
	curr[1] = (u16)(curr[1] + (u16)inst->matrix.t[1]);
	curr[2] = (u16)(curr[2] + (u16)inst->matrix.t[2]);

	next[0] = (u16)(next[0] + (u16)inst->matrix.t[0]);
	next[1] = (u16)(next[1] + (u16)inst->matrix.t[1]);
	next[2] = (u16)(next[2] + (u16)inst->matrix.t[2]);

	primMem = &gGT->backBuffer->primMem;
	packet = primMem->cursor;
	end = primMem->guardEnd;

	if ((uintptr_t)(packet + 1) >= (uintptr_t)end)
		return;

	gte_SetRotMatrix(&gGT->pushBuffer[0].matrix_ViewProj);
	gte_SetTransMatrix(&gGT->pushBuffer[0].matrix_ViewProj);

	MTC2((u32)curr[0] | ((u32)curr[1] << 16), 0);
	MTC2((u32)curr[2], 1);
	MTC2((u32)next[0] | ((u32)next[1] << 16), 2);
	MTC2((u32)next[2], 3);
	gte_rtpt();

	packet->xy0 = MFC2(12);
	packet->xy1 = MFC2(13);

	depth = MFC2(17);
	if ((u32)(depth - 1) < 0x11ff)
	{
		u32 color = 0x3f;
		int otIndex;
		u32 *ot;

		packet->drawMode = 0xe1000a20;
		packet->pad = 0;

		if (depth > 0xa00)
		{
			int fade = (0x1200 - depth) * 0x3f;

			color = fade >> 11;
			if (fade < 0)
				color = (fade + 0x7ff) >> 11;
		}

		packet->colorAndCode = color | (color << 8) | (color << 16) | 0x42000000;

		otIndex = depth >> 6;
		if (otIndex > 0x3ff)
			otIndex = 0x3ff;

		ot = (u32 *)&gGT->pushBuffer[0].ptrOT[otIndex];
		packet->tag = CtrGpu_PackOTTag(*ot, 0x05000000);
		*ot = CtrGpu_PrimToOTLink24(packet);
		packet++;
	}

	primMem->cursor = packet;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b06ac-0x800b087c
void CS_Thread_LInB(struct Instance *inst)
{
	struct Thread *t;
	struct CutsceneObj *cs;
	s16 modelID;
	char *scriptPtr;

	D233.isCutsceneOver = 0;

	if (inst->thread != 0)
		goto check_polar;

	t = PROC_BirthWithObject(SIZE_RELATIVE_POOL_BUCKET(0x60, NONE, MEDIUM, STATIC), CS_Thread_ThTick, R233.s_introguy, 0);

	inst->thread = t;

	if (t == 0)
		return;

	cs = t->object;

	t->inst = inst;

	cs->metadata = (int *)&cs->decodedOpcode;
	cs->prevOpcode = (char *)-1;
	cs->Subtitles.lngIndex = -1;

	modelID = inst->model->id;

	if (modelID < NDI_BOX_BOX_01)
	{
		if ((u16)(modelID - STATIC_CRASHINTRO) < 0x10)
		{
			scriptPtr = R233.introModelScripts[modelID - STATIC_CRASHINTRO];
		}
		else
		{
			scriptPtr = (char *)R233.script_default;
		}
	}
	else
	{
		scriptPtr = R233.boxModelScripts[modelID - NDI_BOX_BOX_01];
	}

	CS_ScriptCmd_OpcodeAt(cs, scriptPtr);

	cs->unk18 = *(int *)(cs->metadata + 2);

	{
		int rng = MixRNG_Scramble();
		s16 *meta = (s16 *)cs->metadata;
		s16 frameStart = meta[2];
		s16 frameEnd = meta[3];

		cs->unk1c = 0;
		cs->rot.x = 0;
		cs->rot.y = 0;
		cs->rot.z = 0;
		cs->pathProgress32 = 0;
		cs->unk1e = 0;
		cs->flags = 0;
		cs->scaleSpeed = 0;
		cs->frameOverrideRoot = 0;
		cs->desiredScale = 0x1000;
		cs->particleID = 0xff;

		cs->opcodeDuration = frameStart + (s16)(((rng >> 2 & 0xfff) * ((frameEnd - frameStart) + 1)) >> 0xc);

		struct GameTracker *gGT = sdata->gGT;

		cs->unk4 = 0;
		cs->unk6 = 0;
		cs->unk8 = 0x2e808080;
		cs->unk_C = 0;
		cs->unk_E = 0;

		cs->ptrIcons = (struct IconGroup *)((char *)gGT->iconGroup[0] + sizeof(struct IconGroup));
	}

check_polar:
	if (sdata->gGT->levelID == 0x21)
	{
		inst->vertSplit = 0;
		inst->flags |= REFLECTIVE;
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800ae54c-0x800ae81c
void CS_Thread_ThTick(struct Thread *t)
{
	// Retail uses scratchpad 0x1f800108/0x1f800118 for parent frame-data temporaries.
	struct CSThreadParentFrameScratch *parentFrame = CTR_SCRATCHPAD_PTR(struct CSThreadParentFrameScratch, 0x108);
	SVec3 bonePos;
	struct CutsceneObj *cs = t->object;
	struct Instance *inst = t->inst;
	struct Instance *parentInst;
	struct Thread *parentThread;

	if (CS_Thread_UseOpcode(inst, cs))
	{
		t->flags |= THREAD_FLAG_DEAD;

		if ((sdata->gGT->gameMode2 & 0x80) != 0)
			return;
	}

	CS_Thread_MoveOnPath(t);
	CS_Thread_AnimateScale(t);
	CS_Thread_Particles(t);

	if ((cs->flags & 0x40) != 0)
		CS_Thread_InterpolateFramesMS(t);

	// ASM: 0x800ae5dc - parent-thread frameOverrideRoot processing
	if (inst != 0)
	{
		parentThread = t->parentThread;

		if (parentThread != 0)
		{
			if ((cs->flags & 0x4) == 0)
			{
				parentInst = parentThread->inst;

				CS_Instance_GetFrameData(parentInst, parentInst->animIndex, parentInst->animFrame, (u16 *)parentFrame->parentPos.vec.v,
				                         (u16 *)parentFrame->parentRot.vec.v, 0);

				inst->matrix.t[0] = parentInst->matrix.t[0] + parentFrame->parentPos.x;
				inst->matrix.t[1] = parentInst->matrix.t[1] + parentFrame->parentPos.y;
				inst->matrix.t[2] = parentInst->matrix.t[2] + parentFrame->parentPos.z;

				if ((cs->flags & 0x10) == 0)
				{
					ConvertRotToMatrix(&inst->matrix, &parentFrame->parentRot.vec);
				}
			}
		}

		inst = t->inst;
		if (inst == 0)
			goto thTick_subtitles;

		// ASM: 0x800ae6b4 - flag 0x8 writes bone Y to overlay-233 mutable state.
		if ((cs->flags & 0x8) != 0)
		{
			CS_Instance_GetFrameData(inst, inst->animIndex, inst->animFrame, (u16 *)bonePos.v, 0, 0);

			D233.VertSplitLine = bonePos.y;

			inst = t->inst;
			if (inst == 0)
				goto thTick_subtitles;
		}

		// ASM: 0x800ae6fc - flag 0x2: random alphaScale for fade effect
		if ((cs->flags & 0x2) != 0)
		{
			inst->alphaScale = 0;

			if ((sdata->gGT->timer & 0x1) != 0)
			{
				inst->alphaScale = (MixRNG_Scramble() & 0x7ff) + 1024;
			}
		}
	}

	// ASM: 0x800ae744 - subtitle rendering
thTick_subtitles:
	if (cs->Subtitles.lngIndex > 0)
	{
		struct GameTracker *gGT = sdata->gGT;
		int textWidth;
		u16 textRect[4];

		textWidth = DecalFont_DrawMultiLine(sdata->lngStrings[cs->Subtitles.lngIndex], cs->Subtitles.textPos.x, cs->Subtitles.textPos.y, 460,
		                                    cs->Subtitles.font, cs->Subtitles.colors);

		textRect[0] = (u16)((u16)cs->Subtitles.textPos.x - 236);
		textRect[1] = (u16)((u16)cs->Subtitles.textPos.y - 4);
		textRect[2] = 472;
		textRect[3] = (u16)((s16)textWidth + 8);

		RECTMENU_DrawInnerRect((RECT *)textRect, 4, gGT->backBuffer->otMem.uiOT);
	}

thTick_epilogue:
	// ASM: 0x800ae7dc - check isCutsceneOver, re-apply death flag
	if (D233.isCutsceneOver != 0)
	{
		t->flags |= THREAD_FLAG_DEAD;
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800af328-0x800af7c0
struct Thread *CS_Thread_Init(s16 modelID, const char *name, s16 *param_3, s16 param_4, struct Thread *parent)
{
	struct GameTracker *gGT = sdata->gGT;
	struct CutsceneObj *cs;
	struct Instance *inst;
	struct Thread *t;
	char *scriptPtr;
	u32 bucket;
	s16 *meta;

	if (modelID == NOFUNC)
	{
		inst = NULL;

		t = PROC_BirthWithObject(SIZE_RELATIVE_POOL_BUCKET(0x60, NONE, MEDIUM, CAMERA), CS_Thread_ThTick, name, parent);

		if (t == NULL)
			return NULL;
	}
	else
	{
		bucket = OTHER;

		if ((u32)(modelID - NDI_KART6) < 2)
			bucket = AKUAKU;

		if ((u32)(modelID - NDI_KART0) < 4)
			bucket = GHOST;

		inst = INSTANCE_BirthWithThread(modelID, name, MEDIUM, bucket, CS_Thread_ThTick, 0x60, parent);

		if (inst == NULL)
			return NULL;

		t = inst->thread;
		t->funcThDestroy = PROC_DestroyInstance;
	}

	cs = t->object;

	cs->metadata = (int *)&cs->decodedOpcode;
	cs->frameOverrideRoot = NULL;
	cs->prevOpcode = (char *)-1;
	cs->Subtitles.lngIndex = -1;

	if (modelID == NOFUNC)
	{
		int level = gGT->levelID;

		if (level == NAUGHTY_DOG_CRATE)
		{
			scriptPtr = (char *)&R233.creditsOpcodeData[0x18];
		}
		else if (level == OXIDE_ENDING)
		{
			scriptPtr = (char *)&R233.introEndingOpcodeData[0];
		}
		else if (level == OXIDE_TRUE_ENDING)
		{
			scriptPtr = (char *)&R233.introEndingOpcodeData[0x30];
		}
		else if ((gGT->gameMode2 & CREDITS) == 0)
		{
			scriptPtr = R233.introCutsceneOpcodes[level - INTRO_RACE_TODAY];
		}
		else
		{
			scriptPtr = R233.creditsCutsceneOpcodes[level - CREDITS_CRASH];
		}
	}
	else
	{
		if (modelID >= NDI_BOX_BOX_01)
		{
			if ((u32)(modelID - NDI_BOX_BOX_01) < 0x2b)
			{
				scriptPtr = R233.boxModelScripts[modelID - NDI_BOX_BOX_01];
			}
			else
			{
				scriptPtr = (char *)R233.script_default;
			}

			CS_ScriptCmd_OpcodeAt(cs, scriptPtr);

			if ((u32)(modelID - NDI_KART0) < 4)
			{
				// NOTE(native): point at the entry's `.data` field directly
				// rather than the whole table-entry struct -- retail relied
				// on `.data` being the struct's leading 4-byte field so a
				// raw int* read off the struct's address worked; on native
				// `.data` is an 8-byte pointer, so frameOverrideRoot must
				// alias the field itself (see ovr_233.h).
				cs->frameOverrideRoot = &D233.cs_initMatrixTable[modelID - NDI_KART0].data;
			}

			goto after_opcode;
		}

		if ((u32)(modelID - STATIC_PINHEAD) < 5)
		{
			scriptPtr = (char *)R233.script_default;
		}
		else if (modelID == STATIC_DINGOFIRE)
		{
			scriptPtr = (char *)R233.script_dingofire;
		}
		else if ((u32)(modelID - STATIC_TAWNA1) < 4)
		{
			if (gGT->gameMode2 & CREDITS)
				scriptPtr = (char *)R233.script_tawnaCredits;
			else
				scriptPtr = (char *)R233.script_tawnaNormal;
		}
		else if ((u32)(modelID - STATIC_CRASHDANCE) < 0x10)
		{
			char *const *base;
			int off = (modelID - STATIC_CRASHDANCE);

			if (modelID == gGT->podium_modelIndex_First)
				base = R233.danceFirstScripts;
			else
				base = R233.danceOtherScripts;

			scriptPtr = base[off];
		}
		else
		{
			scriptPtr = (char *)R233.script_default;
		}
	}

	CS_ScriptCmd_OpcodeAt(cs, scriptPtr);

after_opcode:

	cs->unk18 = cs->metadata[2];

	meta = (s16 *)cs->metadata;
	cs->opcodeDuration = meta[2] + (s16)(((MixRNG_Scramble() >> 2 & 0xfff) * ((meta[3] - meta[2]) + 1)) >> 0xc);

	if (inst != NULL)
	{
		MTC2(*(int *)(param_3 + 4), 0);
		MTC2(*(int *)(param_3 + 6), 1);
		gte_llv0();

		int rx = MFC2(25);
		int ry = MFC2(26);
		int rz = MFC2(27);

		inst->matrix.t[0] = rx + param_3[0];
		inst->matrix.t[1] = ry + param_3[1];
		inst->matrix.t[2] = rz + param_3[2];

		if (gGT->levelID != NAUGHTY_DOG_CRATE)
		{
			inst->scale.x = 0x2800;
			inst->scale.y = 0x2800;
			inst->scale.z = 0x2800;
		}

		if ((u32)(gGT->levelID - GEM_STONE_VALLEY) < 5)
		{
			inst->depthBiasNormal -= 4;
			inst->depthBiasSecondary -= 4;
		}

		param_3[0xc] = param_3[8];
		param_3[0xe] = param_3[10];
		param_3[0xd] = param_3[9] + param_4;

		ConvertRotToMatrix(&inst->matrix, param_3 + 0xc);

		cs->unk1c = param_3[0xd] & 0xfff;
		cs->rot.x = param_3[0xc] & 0xfff;
		cs->rot.y = param_3[0xd] & 0xfff;
		cs->rot.z = param_3[0xe] & 0xfff;
	}

	cs->particleID = 0xff;
	cs->pathProgress32 = 0;
	cs->unk1e = 0;
	cs->flags = 0;
	cs->scaleSpeed = 0;
	cs->desiredScale = 0x2800;

	cs->unk4 = 0;
	cs->unk6 = 0;
	cs->unk8 = 0x2e808080;
	cs->unk_C = 0;
	cs->unk_E = 0;

	cs->ptrIcons = (struct IconGroup *)((char *)gGT->iconGroup[0] + sizeof(struct IconGroup));

	return t;
}
