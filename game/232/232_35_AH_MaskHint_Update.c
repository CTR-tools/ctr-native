#include <common.h>

force_inline void AH_MaskHint_DrawRepeatPrompt(void)
{
	int lngIndex = 0;
	int boolFound = 0;

	if (sdata->AkuAkuHintState != 5)
		return;

	s16 *ptrLngID = &D232.hintMenu_lngIndexArr[0];
	struct GameTracker *gGT = sdata->gGT;
	struct Driver *d = gGT->drivers[0];

	for (/**/; *ptrLngID > -1; ptrLngID++)
	{
		if (D232.maskHintID == (ptrLngID[0] - 0x17b) / 2)
		{
			boolFound = 1;
			break;
		}
	}

	if (!boolFound)
		return;

	// Retail finds the hint subtitle entry above, but the shipped path draws a
	// generic "press start to repeat" prompt instead of that hint text.
	if (VehPickupItem_MaskBoolGoodGuy(d))
		lngIndex = 0x177;
	else
		lngIndex = 0x232;

	RECT r;
	r.x = -10;
	r.y = 0xb0;
	r.w = 0x214;
	r.h = 8 + DecalFont_DrawMultiLine(sdata->lngStrings[lngIndex], 0x100, 0xb4, 400, 2, 0xffff8000);

	RECTMENU_DrawInnerRect(&r, 4, gGT->backBuffer->otMem.startPlusFour);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 overlay 232 0x800b4470-0x800b4c80.
void AH_MaskHint_Update()
{
	struct GameTracker *gGT = sdata->gGT;
	struct Driver *d = gGT->drivers[0];
	s16 pos[3];
	s16 rot[3];

	switch (sdata->AkuAkuHintState - 1)
	{
	case 0:
		if (sdata->XA_State != 0)
			return;

		sdata->AkuAkuHintState++;
		break;

	case 1:
	{
		int absSpeedApprox = d->speedApprox;
		if (absSpeedApprox < 0)
			absSpeedApprox = -absSpeedApprox;
		if (absSpeedApprox > 0x31)
			return;

		if ((D232.maskWarppadBoolInterrupt & 1) == 0)
		{
			struct CameraDC *cdc = &gGT->cameraDC[0];

			cdc->driverOffset_CamEyePos[0] = D232.eyePos[0];
			cdc->driverOffset_CamEyePos[1] = D232.eyePos[1];
			cdc->driverOffset_CamEyePos[2] = D232.eyePos[2];

			cdc->driverOffset_CamLookAtPos[0] = D232.lookAtPos[0];
			cdc->driverOffset_CamLookAtPos[1] = D232.lookAtPos[1];
			cdc->driverOffset_CamLookAtPos[2] = D232.lookAtPos[2];

			cdc->flags |= 8;

			// Get pos and rot, then set them as desired
			CAM_FollowDriver_AngleAxis(cdc, d, 0x1f800108, pos, rot);
			CAM_SetDesiredPosRot(cdc, pos, rot);
		}

		D232.maskWarppadDelayFrames = 60;

		sdata->AkuAkuHintState++;
	}
	break;

	case 2:

		if (((D232.maskWarppadBoolInterrupt & 1) == 0) && ((gGT->cameraDC[0].flags & 0x800) == 0) && (D232.maskSpawnFrame != 20))
		{
			return;
		}

		struct Instance *dInst = d->instSelf;
		sdata->instMaskHints3D = VehTalkMask_Init();
		struct Instance *mhInst = sdata->instMaskHints3D;

		CTR_MatrixToRot((SVECTOR *)rot, &dInst->matrix, 0x11);

		// not a typo
		D232.maskCamRotStart[0] = rot[1] & 0xfff;
		D232.maskCamRotStart[2] = rot[2] & 0xfff;
		D232.maskCamRotStart[1] = rot[0] & 0xfff;

		D232.maskCamPosStart[0] = dInst->matrix.t[0];
		D232.maskCamPosStart[1] = dInst->matrix.t[1];
		D232.maskCamPosStart[2] = dInst->matrix.t[2];

		((struct MaskHint *)mhInst->thread->object)->scale = 0;

		AH_MaskHint_SetAnim(0);

		D232.maskFrameCurr = 0;

		sdata->AkuAkuHintState++;
		break;

	case 3:

		// first frame "whoosh" sound
		if (D232.maskFrameCurr == 0)
			// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b46d4-0x800b46e4 for mask spawn-start SFX.
			OtherFX_Play_LowLevel(0x100, 1, 0xff8080);

		// if 3-second spawn, play more sounds
		if (D232.maskSpawnFrame == 0x5a)
		{
			if (D232.maskFrameCurr == 10)
				// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b470c-0x800b4774 for mask spawn pulse 10 SFX.
				OtherFX_Play_LowLevel(0x100, 0, 0xd78a80);

			else if (D232.maskFrameCurr == 20)
				// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b4728-0x800b4774 for mask spawn pulse 20 SFX.
				OtherFX_Play_LowLevel(0x100, 1, 0xaf9480);

			else if (D232.maskFrameCurr == 25)
				// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b4744-0x800b4774 for mask spawn pulse 25 SFX.
				OtherFX_Play_LowLevel(0x100, 0, 0x879e80);

			else if (D232.maskFrameCurr == 30)
				// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b4760-0x800b4774 for mask spawn pulse 30 SFX.
				OtherFX_Play_LowLevel(0x100, 1, 0x5fa880);
		}

		int timer4096 = (D232.maskFrameCurr << 0xc) / D232.maskSpawnFrame;

		AH_MaskHint_SetAnim(timer4096);

		AH_MaskHint_SpawnParticles(3, &D232.emSet_maskSpawn[0], timer4096);

		// if not finished spawning
		if (D232.maskFrameCurr < D232.maskSpawnFrame)
		{
			D232.maskFrameCurr++;

			timer4096 = (D232.maskFrameCurr << 0xc) / D232.maskSpawnFrame;

			AH_MaskHint_LerpVol(timer4096);
			break;
		}

		// NOTE(aalhendi): Retail only waits for the mask model pointer.
		if (sdata->modelMaskHints3D == 0)
		{
			AH_MaskHint_LerpVol(0x1000);
			break;
		}

		if (((D232.maskWarppadBoolInterrupt & 1) != 0) || ((gGT->cameraDC[0].flags & 0x800) != 0))
		{
			AH_MaskHint_LerpVol(0x1000);

			AH_MaskHint_SpawnParticles(0x18, &D232.emSet_maskLeave[0], 0x1000);

			VehTalkMask_PlayXA(sdata->instMaskHints3D, D232.maskHintID);

			if (((gGT->gameMode1 & ADVENTURE_ARENA) != 0) &&

			    // Not "Welcome to Adventure" or "You need a Boss Key"
			    (D232.maskHintID != 0) && (D232.maskHintID != 0x18))
			{
				// hide UI map
				gGT->hudFlags |= 0x10;
			}

			sdata->AkuAkuHintState++;
			break;
		}
		break;

	case 4:
	{
		// NOTE(aalhendi): Native draws only this shared prompt earlier from
		// AH_Map_Main so synchronous DrawOTag sees it; the rest of this state
		// remains retail-timed here.
#if !defined(CTR_NATIVE)
		AH_MaskHint_DrawRepeatPrompt();
#endif

		AH_MaskHint_SetAnim(0x1000);

		int bVar8;
		int uVar3 = D232.maskWarppadDelayFrames - 1;
		if ((((D232.maskWarppadDelayFrames == 0) || (bVar8 = D232.maskWarppadDelayFrames == 1, D232.maskWarppadDelayFrames = uVar3, bVar8)) &&
		     (((VehTalkMask_boolNoXA() != 0) || ((sdata->gGamepads->gamepad[0].buttonsTapped & BTN_TRIANGLE) != 0)))) &&
		    (sdata->AkuAkuHintState++,

		     // If you're in Adventure Arena
		     ((gGT->gameMode1 & ADVENTURE_ARENA) != 0)))
		{
			// show map again
			gGT->hudFlags &= ~(0x10);
		}
	}
	break;

	case 5:

		AH_MaskHint_SpawnParticles(20, &D232.emSet_maskLeave[0], 0x1000);

		// vanish sound
		// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b4b24-0x800b4b2c for mask vanish SFX.
		OtherFX_Play(0x101, 1);

		VehTalkMask_End();

		if ((D232.maskWarppadBoolInterrupt & 1) == 0)
		{
			// transition back to player
			gGT->cameraDC[0].flags |= 0x400;
		}

		sdata->AkuAkuHintState++;
		break;

	case 6:

		AH_MaskHint_LerpVol(0x1000 - gGT->cameraDC[0].unk8C);

		if (((gGT->cameraDC[0].flags & 0x200) == 0) || ((D232.maskWarppadBoolInterrupt & 1) != 0))
		{
			AH_MaskHint_SetAnim(0);
			AH_MaskHint_LerpVol(0);

			D232.maskWarppadDelayFrames = 0;
			if ((D232.maskWarppadBoolInterrupt & 1) != 0)
				D232.maskWarppadDelayFrames = 30;

			sdata->AkuAkuHintState++;
		}
		break;

	case 7:

		AH_MaskHint_LerpVol(0);

		D232.maskWarppadDelayFrames--;

		if (D232.maskWarppadDelayFrames < 1)
		{
			RECTMENU_ClearInput();

			sdata->AkuAkuHintState = 0;
			sdata->boolDraw3D_AdvMask = 0;

			gGT->gameMode2 &= ~(VEH_FREEZE_DOOR);
			d->funcPtrs[0] = VehPhysProc_Driving_Init;
		}

		break;
	}
}
