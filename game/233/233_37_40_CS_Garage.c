#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b7784-0x800b7834
void CS_Garage_ZoomOut(char zoomState)
{
	if (zoomState != 0)
	{
		// number of frames to zoom in, or out,
		// when selecting or cancelling OSK
		gGarage.numFramesCurr_ZoomIn = gGarage.numFramesMax_Zoom;
		gGarage.numFramesCurr_ZoomOut = gGarage.numFramesMax_Zoom;
	}
	else
	{
		gGarage.numFramesCurr_ZoomIn = 0;
		gGarage.numFramesCurr_ZoomOut = 0;
	}

	gGarage.numFramesCurr_GarageMove = 0;
	gGarage.boolSelected = 0;
	gGarage.delayOneSecond = 0;

	sdata->gGT->gameMode2 &= ~(GARAGE_OSK);

	// if just entered garage
	if (zoomState == 0)
	{
		Garage_Init();
		Garage_Enter(sdata->advCharSelectIndex_curr);

		Audio_SetState_Safe(8);
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b7834-0x800b854c
void CS_Garage_MenuProc(struct RectMenu *param_1)
{
	s16 garageFrames;
	s16 *barLen;
	u16 classNamePosX;
	int i;
	u32 statNamePosX;
	u32 statBarPosX;

	u32 currSelectIndex = sdata->advCharSelectIndex_curr;
	struct GameTracker *gGT = sdata->gGT;
	struct PrimMem *primMem = &gGT->backBuffer->primMem;
	s16 currCharacterID = gGarage.unusedArr_garageChars[currSelectIndex];
	struct MetaDataCHAR *MDC = &data.MetaDataCharacters[currCharacterID];
	int nameIndex = MDC->name_LNG_long;
	RECT r;
	Color white = MakeColor(0xFF, 0xFF, 0xFF);
	Color black = MakeColor(0, 0, 0);

	// CameraDC, freecam mode
	gGT->cameraDC[0].cameraMode = 3;


	// subtract transition timer by one frame
	garageFrames = gGarage.numFramesCurr_GarageMove - 1;

	// if mid-transition, skip some code
	if (gGarage.numFramesCurr_GarageMove != 0)
		goto SKIP_CONTROLS;

	// At this point, there must not be a transition
	// between drivers, so start drawing the UI

	// count frames in garage?
	gGarage.unusedFrameCount++;

	// animate growth of all three stat bars
	for (i = 0; i < 3; i++)
	{
		barLen = &gGarage.barLen[i];
		s16 stat = gGarage.barStat[MDC->engineID * 3 + i];


#define BAR_RATE 3

		if (*barLen < stat)
			*barLen = *barLen + BAR_RATE;
		if (stat < *barLen)
			*barLen = stat;
	}

	if (
	    // Tiny Tiger
	    (nameIndex == 46) ||

	    (statNamePosX = 383,

	     // Pura
	     nameIndex == 51))
	{
		statNamePosX = 129;
		classNamePosX = 128;
		statBarPosX = 139;
	}
	else
	{
		classNamePosX = 384;
		statBarPosX = 393;
	}

	// "Speed"
	DecalFont_DrawLine(sdata->lngStrings[0x245], statNamePosX, 30, FONT_BIG, JUSTIFY_RIGHT | ORANGE_RED);

	// "Accel"
	DecalFont_DrawLine(sdata->lngStrings[0x246], statNamePosX, 0x2d, 1, 0x4021);

	// "Turn"
	DecalFont_DrawLine(sdata->lngStrings[0x247], statNamePosX, 60, FONT_BIG, JUSTIFY_RIGHT | BLUE);

	int engineID = MDC->engineID;

	// 0x248 - Beginner
	// EngineID == 3
	i = 0;

	// 0x24A - Advanced
	if (engineID == SPEED)
		i = 2;

	// 0x249 - Intermediate
	if (engineID < SPEED)
		i = 1;

	// 7 pixels tall
	u16 statBarStart_Y = 33;
	u16 statBarEnd_Y = 40;

	u16 statBarShadows_Y = 34;

	// Draw class name
	DecalFont_DrawLine(sdata->lngStrings[gGarage.unusedArr_lngIndex[i]], classNamePosX, 15, FONT_BIG, (JUSTIFY_CENTER | ORANGE));

	// bar length (animated)

	for (i = 0; i < 3; i++)
	{
		barLen = &gGarage.barLen[i];

		// bar outline
		r.x = statBarPosX;
		r.y = statBarStart_Y;
		r.w = *barLen;
		r.h = 7;

		// outline color white at 0x800b7780
		CTR_Box_DrawWireBox(&r, &white, gGT->pushBuffer_UI.ptrOT, primMem);

		// bar shadows
		r.x = statBarPosX + 1;
		r.y = statBarShadows_Y;
		r.w = *barLen - 2;
		r.h = 5;

		// outline color black (shadows)
		CTR_Box_DrawWireBox(&r, &black, gGT->pushBuffer_UI.ptrOT, primMem);

		int segmentLen = 13;
		int segmentStart = 0;
		int segmentEnd = segmentLen;

		for (int segmentIndex = 0; segmentIndex < 6; segmentIndex++)
		{
			// color data of bars (blue green yellow red)
			u32 *barColor = &gGarage.barColors[segmentIndex];
			s16 currSegmentLen = (s16)segmentLen;

			if (*barLen <= segmentEnd)
			{
				currSegmentLen = *barLen - segmentStart;
			}

			if ((int)currSegmentLen << 0x10 < 0)
			{
				currSegmentLen = 0;
			}

			if (segmentStart + currSegmentLen <= *barLen)
			{
				// primMem curr
				POLY_G4 *p = primMem->curr;

				// quit if prim mem runs out
				if (primMem->end < (void *)p)
					return;

				primMem->curr = p + 1;

				// color data
				*(int *)&p->r0 = barColor[0] | 0x38000000;
				*(int *)&p->r1 = barColor[1] | 0x38000000;
				*(int *)&p->r2 = barColor[0] | 0x38000000;
				*(int *)&p->r3 = barColor[1] | 0x38000000;

				s16 segmentX = statBarPosX + segmentStart;

				// top left
				p->x0 = segmentX;
				p->y0 = statBarStart_Y;

				// top right
				p->x1 = segmentX + currSegmentLen;
				p->y1 = statBarStart_Y;

				// bottom left
				p->x2 = segmentX;
				p->y2 = statBarEnd_Y;

				// bottom right
				p->x3 = segmentX + currSegmentLen;
				p->y3 = statBarEnd_Y;

				// pointer to OT memory
				void *ot = gGT->pushBuffer_UI.ptrOT;

				*(int *)p = (*(int *)ot & 0xffffff) | 0x8000000;
				*(int *)ot = (int)CtrGpu_PrimToOTLink24(p);
			}

			segmentStart += segmentLen;
			segmentEnd += segmentLen;
		}

		// 15 pixels lower Y axis
		statBarStart_Y += 15;
		statBarEnd_Y += 15;
		statBarShadows_Y += 15;
	}

	// "Intermediate" (longest class name)
	s16 classMaxLen = DecalFont_GetLineWidth(sdata->lngStrings[0x249], 1);

	// Stats box
	r.x = (classNamePosX - (classMaxLen >> 1)) - 6;
	r.y = 11;
	r.w = classMaxLen + 12;
	r.h = 68;

	// Draw 2D Menu rectangle background
	RECTMENU_DrawInnerRect(&r, 4, gGT->backBuffer->otMem.startPlusFour);

	char *name = sdata->lngStrings[nameIndex];

	// Draw character name
	DecalFont_DrawLine(name, 0x100, 0xb4, 1, 0xffff8000);

	char arrowColor = ORANGE;

	// blink arrows
	if ((sdata->frameCounter & 4) == 0)
	{
		arrowColor = RED;
	}

	// Color data
	int *arrowColors = data.ptrColor[arrowColor];

	int nameLen = DecalFont_GetLineWidth(name, 1) >> 1;

	int arrowPos[2] = {236 - nameLen, nameLen + 274};
	int arrowRot[2] = {0x800, 0};

	struct Icon **iconPtrArray = ICONGROUP_GETICONS(gGT->iconGroup[4]);

	for (i = 0; i < 2; i++)
	{
		DecalHUD_Arrow2D(iconPtrArray[0x38], arrowPos[i], 187,

		                 primMem, gGT->pushBuffer_UI.ptrOT,

		                 arrowColors[0], arrowColors[1], arrowColors[2], arrowColors[3],

		                 0, 0x1000, arrowRot[i]);
	}

	garageFrames = gGarage.numFramesCurr_GarageMove;

	if (((gGT->renderFlags & 0x1000) != 0) ||

	    (
	        // If you dont press Triangle, Cross, Circle, or Square
	        ((sdata->AnyPlayerTap & 0x40070) == 0) &&

	        // If you dont press D-pad
	        ((sdata->AnyPlayerHold & 0xc) == 0)))

		goto SKIP_CONTROLS;

	// If you dont press D-pad
	if ((sdata->AnyPlayerHold & 0xc) == 0)
	{
		// If you do not press Cross or Circle
		if ((sdata->AnyPlayerTap & 0x50) == 0)
		{
			// If you press Triangle or Square
			if ((sdata->AnyPlayerTap & 0x40020) != 0)
			{
				// Play Sound
				OtherFX_Play(2, 1);

				garageFrames = gGarage.numFramesCurr_ZoomIn;
				if (gGarage.boolSelected == 1)
				{
					gGarage.boolSelected = 0;
					gGT->gameMode2 &= ~GARAGE_OSK;

					if (garageFrames < gGarage.numFramesMax_Zoom)
					{
						gGarage.numFramesCurr_ZoomOut = gGarage.numFramesMax_Zoom - garageFrames;
					}
				}
				else
				{
					// return to main menu
					sdata->mainMenuState = 0;

					Garage_Leave();

					// load main menu LEV
					MainRaceTrack_RequestLoad(0x27);
				}
			}
		}

		// If you press Cross or Circle
		else
		{
			// "Have you selected character?"
			// If true, it will show an animation, and then show the
			// OSK (keyboard) screen. If set to 0 after in that screen,
			// the screen does not disappear

			// if false
			if (gGarage.boolSelected == 0)
			{
				// make it true
				gGarage.boolSelected = 1;
			}

			// if true
			else
			{
				// if pressed X twice quickly
				if (gGarage.boolSelected == 1)
				{
					// set desiredMenu to OSK (on-screen keyboard)
					sdata->ptrDesiredMenu = &data.menuSubmitName;

					data.characterIDs[0] = gGarage.unusedArr_garageChars[currSelectIndex];
					sdata->advProgress.characterID = data.characterIDs[0];

					SubmitName_RestoreName(0);
					OtherFX_Play(1, 1);
				}
			}
		}
	}

	// if using D-pad
	else
	{
		// erase animated bars
		for (i = 2; i > -1; i--)
		{
			barLen = &gGarage.barLen[i];
			*barLen = 0;
		}
		// Play Sound
		OtherFX_Play(0, 1);

		// If you dont press Left
		if ((sdata->AnyPlayerHold & 4) == 0)
		{
			// If you dont press Right
			if ((sdata->AnyPlayerHold & 8) != 0)
			{
				currSelectIndex++;
				goto LAB_800b8084;
			}
		}

		// If you press Left
		else
		{
			currSelectIndex--;

		LAB_800b8084:

			// previous equals current
			sdata->advCharSelectIndex_prev = sdata->advCharSelectIndex_curr;

			// clamp 0-7
			currSelectIndex &= 7;
			sdata->advCharSelectIndex_curr = currSelectIndex;

			Garage_MoveLR(currSelectIndex);
		}

		// reset frame counter to max number of frames
		gGarage.numFramesCurr_GarageMove = gGarage.numFramesMax_GarageMove;

		if (gGarage.numFramesCurr_ZoomIn < gGarage.numFramesMax_Zoom)
		{
			gGarage.numFramesCurr_ZoomOut = gGarage.numFramesMax_Zoom - gGarage.numFramesCurr_ZoomIn;
		}

		gGarage.boolSelected = 0;
		gGT->gameMode2 &= ~GARAGE_OSK;
	}

	// clear gamepad input (for menus)
	RECTMENU_ClearInput();

	garageFrames = gGarage.numFramesCurr_GarageMove;
SKIP_CONTROLS:
	gGarage.numFramesCurr_GarageMove = garageFrames;

	// if frames remaing for zoom camera
	if (0 < gGarage.numFramesCurr_ZoomIn)
	{
		// decrease zoom frame timer
		gGarage.numFramesCurr_ZoomIn--;
	}

	// if pressed X once, and waited for countdown clock
	if ((gGarage.boolSelected == 1) && (gGarage.numFramesCurr_ZoomIn == 0))
	{
		if (
		    // frames remaining for animation
		    (59 < gGarage.delayOneSecond) || ((gGT->gameMode2 & GARAGE_OSK) != 0))
		{
			// set desiredMenu to OSK (on-screen keyboard)
			sdata->ptrDesiredMenu = &data.menuSubmitName;

			data.characterIDs[0] = gGarage.unusedArr_garageChars[currSelectIndex];
			sdata->advProgress.characterID = data.characterIDs[0];

			SubmitName_RestoreName(0);
			OtherFX_Play(1, 1);
		}
		else
		{
			gGarage.delayOneSecond++;
		}
	}

#ifdef CTR_NATIVE
	if (sdata->ptrDesiredMenu == &data.menuSubmitName)
	{
		// NOTE(aalhendi): PC-only keyboard shim; retail gamepad flow above stays unchanged.
		// flush async key state buffer. If not, tapping Enter "before" picking a garage character,
		//  then picking character, will immediately warp you to the adv hub, with no time to type the name
		NikoGetEnterKey();
	}
#endif

	if (gGarage.boolSelected == 0)
	{
		gGarage.numFramesCurr_ZoomIn = gGarage.numFramesMax_Zoom;
	}

	if (gGarage.numFramesCurr_ZoomOut != 0)
	{
		gGarage.numFramesCurr_ZoomOut--;
	}

	u32 prevSelectIndex = sdata->advCharSelectIndex_prev;

	// Pura->Crash
	if ((currSelectIndex == 0) && (prevSelectIndex == 7))
		garageFrames = 240 - gGarage.numFramesCurr_GarageMove;

	// Crash->Pura
	else if ((currSelectIndex == 7) && (prevSelectIndex == 0))
		garageFrames = gGarage.numFramesCurr_GarageMove + 210;

	// Move Right
	else if (prevSelectIndex < currSelectIndex)
		garageFrames = currSelectIndex * 30 - gGarage.numFramesCurr_GarageMove;

	// Move Left
	else
		garageFrames = currSelectIndex * 30 + gGarage.numFramesCurr_GarageMove;

	// animation frame index,
	// pointer to position,
	// pointer to rotation

	int getPath;
	s16 camPos[3];
	s16 camRot[3];
	CAM_Path_Move((int)garageFrames, &camPos[0], &camRot[0], &getPath);

	// set position and rotation to pushBuffer
	gGT->pushBuffer[0].pos[0] = camPos[0];
	gGT->pushBuffer[0].pos[1] = camPos[1];
	gGT->pushBuffer[0].pos[2] = camPos[2];

	gGT->pushBuffer[0].rot[0] = camRot[0];
	gGT->pushBuffer[0].rot[1] = camRot[1];
	gGT->pushBuffer[0].rot[2] = camRot[2];

	int zoom = gGarage.numFramesCurr_ZoomOut;
	if (zoom == 0)
	{
		zoom = (gGarage.numFramesMax_Zoom - gGarage.numFramesCurr_ZoomIn) * (gGarage.fovMax - gGarage.fovMin);
	}
	else
	{
		zoom = zoom * (gGarage.fovMax - gGarage.fovMin);
	}

	zoom = gGarage.fovMin + zoom / gGarage.numFramesMax_Zoom;

	gGT->pushBuffer[0].distanceToScreen_CURR = zoom;
	gGT->pushBuffer[0].distanceToScreen_PREV = zoom;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b854c-0x800b8558
struct RectMenu *CS_Garage_GetMenuPtr(void)
{
	return &gGarage.menuGarage;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b8558-0x800b8598
void CS_Garage_Init(void)
{
	// go to 3D character selection
	sdata->ptrActiveMenu = &gGarage.menuGarage;

	gGarage.menuGarage.state &= ~(ONLY_DRAW_TITLE);

	// 0 = just entered garage
	CS_Garage_ZoomOut(0);
}
