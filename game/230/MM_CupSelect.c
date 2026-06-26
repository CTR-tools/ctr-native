#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b0eb8-0x800b0eec.
void MM_CupSelect_Init(void)
{
	// reset transition data
	D230.cupSel_transitionFrames = 0xc;
	D230.cupSel_transitionState = 0;

	// disable 0x400 (dont exec funcptr)
	// enable 0x20 (allow exec funcptr, and block input
	D230.menuCupSelect.state &= ~(EXECUTE_FUNCPTR);
	D230.menuCupSelect.state |= DISABLE_INPUT_ALLOW_FUNCPTRS;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 overlay 230 0x800b0eec-0x800b164c.
void MM_CupSelect_MenuProc(struct RectMenu *menu)
{
	char i;
	u8 cupIndex;
	u8 starIndex;
	u8 trackIndex;
	s16 elapsedFrames;
	u32 txtColor;
	u32 *starColor;
	int startX;
	int startY;
	struct GameTracker *gGT = sdata->gGT;
	RECT cupBox;

	if (menu->unk1e == 0)
	{
		D230.cupSel_postTransition_boolStart = (menu->rowSelected != -1);
		D230.cupSel_transitionState = 2;
		D230.menuCupSelect.state &= ~(EXECUTE_FUNCPTR);
		D230.menuCupSelect.state |= DISABLE_INPUT_ALLOW_FUNCPTRS;
		return;
	}

	elapsedFrames = D230.cupSel_transitionFrames;

	// if not stationary
	if (D230.cupSel_transitionState != 1)
	{
		// if transitioning in
		if (D230.cupSel_transitionState == 0)
		{
			MM_TransitionInOut(&D230.transitionMeta_cupSel[0], elapsedFrames, 8);

			// if no more frames
			if (elapsedFrames == 0)
			{
				// menu is now in focus
				D230.cupSel_transitionState = 1;
				D230.menuCupSelect.state &= ~(DISABLE_INPUT_ALLOW_FUNCPTRS);
				D230.menuCupSelect.state |= EXECUTE_FUNCPTR;
			}

			else
			{
				elapsedFrames--;
			}
		}
		// if transitioning out
		else if (D230.cupSel_transitionState == 2)
		{
			MM_TransitionInOut(&D230.transitionMeta_cupSel[0], elapsedFrames, 8);

			// increase frame count
			elapsedFrames++;

			// if more than 12 frames pass
			if (12 < elapsedFrames)
			{
				// if cup selected
				if (D230.cupSel_postTransition_boolStart != 0)
				{
					// set cupID to the cup selected
					gGT->cup.cupID = menu->rowSelected;

					// set track index to zero, to go to first track
					gGT->cup.trackIndex = 0;

					// loop through 8 drivers
					for (i = 0; i < 8; i++)
					{
						// set all points for all 8 drivers to zero
						gGT->cup.points[i] = 0;
					}

					// passthrough Menu for the function
					sdata->ptrDesiredMenu = &data.menuQueueLoadTrack;

					// set current level
					gGT->currLEV = data.ArcadeCups[gGT->cup.cupID].CupTrack[gGT->cup.trackIndex].trackID;
					return;
				}

				// return to character selection
				sdata->ptrDesiredMenu = &D230.menuCharacterSelect;

				MM_Characters_RestoreIDs();
				return;
			}
		}
	}

	D230.cupSel_transitionFrames = elapsedFrames;

	DecalFont_DrawLine(sdata->lngStrings[LNG_SELECT_CUP_RACE], (D230.transitionMeta_cupSel[4].currX + 0x100), (D230.transitionMeta_cupSel[4].currY + 0x10), 1,
	                   0xffff8000);

	// Loop through all four cups
	for (cupIndex = 0; cupIndex < 4; cupIndex++)
	{
		// Use solid color
		txtColor = 0xffff8000;

		// If this cup is the one you selected
		if (cupIndex == menu->rowSelected)
		{
			// Make text flash
			if ((sdata->frameCounter & 2) != 0)
			{
				txtColor |= 4;
			}
		}

		startX = (s16)D230.transitionMeta_cupSel[cupIndex].currX + (cupIndex & 1) * 200;
		startY = (s16)D230.transitionMeta_cupSel[cupIndex].currY + (cupIndex >> 1) * 0x54;

		// draw the name of the cup
		DecalFont_DrawLine(sdata->lngStrings[data.ArcadeCups[cupIndex].lngIndex_CupName], startX + 0xa2, startY + 0x44, 3, txtColor);

		startX = startX + 0x4e;
		startY = startY + 0x29;

		// loop through 3 stars to draw
		for (starIndex = 0; starIndex < 3; starIndex++)
		{
			int starUnlock = D230.cupSel_StarUnlockFlag[starIndex] + cupIndex;
			if (CHECK_ADV_BIT(sdata->gameProgress.unlocks, starUnlock) != 0)
			{
				// array of colorIDs
				// 0x11: driver_C (tropy) (blue)
				// 0x0E: driver_9 (papu) (yellow)
				// 0x16: silver

				starColor = data.ptrColor[D230.cupSel_StarColorIndex[starIndex]];

				struct Icon **iconPtrArray = ICONGROUP_GETICONS(gGT->iconGroup[5]);

				DecalHUD_DrawPolyGT4(iconPtrArray[0x37], (startX + (cupIndex & 1) * 0xCA - 0x16), (startY + ((starIndex * 0x10) + 0x10)),
				                     &gGT->backBuffer->primMem, gGT->pushBuffer_UI.ptrOT, starColor[0], starColor[1], starColor[2], starColor[3], 0, FP(1.0));
			}
		}

		// loop through all four track icons in one cup
		for (trackIndex = 0; trackIndex < 4; trackIndex++)
		{
			int posX = (startX + (trackIndex & 1) * 0x54);
			int posY = (startY + (trackIndex >> 1) * 0x23);

			// Draw Icon of each track
			RECTMENU_DrawPolyGT4(gGT->ptrIcons[data.ArcadeCups[cupIndex].CupTrack[trackIndex].iconID], posX, posY, &gGT->backBuffer->primMem,
			                     gGT->pushBuffer_UI.ptrOT, D230.cupSel_Color, D230.cupSel_Color, D230.cupSel_Color, D230.cupSel_Color, 0, FP(0.5));
		}

		if (cupIndex == menu->rowSelected)
		{
			// highlight box
			cupBox.x = startX - 3;
			cupBox.y = startY - 2;
			cupBox.w = 174;
			cupBox.h = 74;

			CTR_Box_DrawClearBox(&cupBox, &sdata->menuRowHighlight_Normal, TRANS_50_DECAL, gGT->backBuffer->otMem.uiOT);
		}

		// background box
		cupBox.x = startX - 6;
		cupBox.y = startY - 4;
		cupBox.w = 180;
		cupBox.h = 78;

		RECTMENU_DrawInnerRect(&cupBox, 0, gGT->backBuffer->otMem.uiOT);
	}
}
