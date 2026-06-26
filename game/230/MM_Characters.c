#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 overlay 230 0x800ad98c-0x800ada4c.
void MM_Characters_AnimateColors(u8 *colorData, s16 playerID, s16 flag)
{
	u8 colorAdjustmentValue;
	u32 trigApproximationIndex;
	u32 trigApprox;

	// access int RGBA as a char array,
	// for editing components of color
	u8 *ptrColor;
	ptrColor = (u8 *)data.ptrColor[playerID + PLAYER_BLUE];

	trigApprox = 0;

	// if player has not selected character yet
	// see MM_Characters_MenuProc
	if (flag == 0)
	{
		trigApproximationIndex = sdata->frameCounter * 0x100 + playerID * 0x400;

		// approximate trigonometry
		trigApprox = *(u32 *)&data.trigApprox[trigApproximationIndex & 0x3ff];

		if ((trigApproximationIndex & 0x400) == 0)
		{
			trigApprox = trigApprox << 0x10;
		}
		trigApprox = trigApprox >> 0x10;

		if ((trigApproximationIndex & 0x800) != 0)
		{
			trigApprox = -(int)trigApprox;
		}
	}

	colorAdjustmentValue = 0;
	if (0xc00 < (int)trigApprox)
	{
		colorAdjustmentValue = ((trigApprox << 7) >> 0xc);
	}

	colorData[0] = ptrColor[0] | colorAdjustmentValue;
	colorData[1] = ptrColor[1] | colorAdjustmentValue;
	colorData[2] = ptrColor[2] | colorAdjustmentValue;
	colorData[3] = 0;

	return;
}

// NOTE(aalhendi): ASM-verified against NTSC-U 926 overlay 230 0x800ada4c-0x800adae4.
int MM_Characters_GetNextDriver(s16 dpad, char characterID)
{
	char nextDriver;
	s16 unlocked;
	char newDriver;

	nextDriver = D230.csm_Active[characterID].indexNext[dpad];
	unlocked = D230.csm_Active[nextDriver].unlockFlags;

	// set new driver to the driver
	// you'd get when pressing Up button
	newDriver = nextDriver;

	if (
	    // if desired driver is not unlocked by default
	    ((unlocked != -1) &&

	    (((sdata->gameProgress.unlocks[unlocked >> 5] >> (unlocked & 0x1f)) & 1) == 0)) &&
		!g_config.unlockAllCharacters) 
	{
		// set new driver to the driver you already have
		newDriver = characterID;
	}

	// return new driver
	return newDriver;
}

// used for preventing players highlighting the same character
// also for when you go left of komodo joe's icon
// NOTE(aalhendi): ASM-verified against NTSC-U 926 overlay 230 0x800adae4-0x800adb64.
u32 MM_Characters_boolIsInvalid(s16 *globalIconPerPlayer, s16 characterID, s16 player)
{
	s16 i = 0;

	// if there are players
	if (sdata->gGT->numPlyrNextGame)
	{
		// loop through players
		for (i = 0; i < sdata->gGT->numPlyrNextGame; i++)
		{
			// if driver is taken
			if ((i != player) && (characterID == globalIconPerPlayer[i]))
			{
				return 1;
			}
		}
	}

	// if driver is not taken
	return 0;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 overlay 230 0x800adb64-0x800adc0c.
// Search for character model by string,
// specific to main menu lev, altered in oxide mod
struct Model *MM_Characters_GetModelByName(int *name)
{
	struct Model **models;
	struct Model *model;
	struct Level *level1 = sdata->gGT->level1;

	// if LEV is invalid
	if (level1 == NULL)
	{
		return NULL;
	}

	models = level1->ptrModelsPtrArray;
	if (models == NULL)
	{
		return NULL;
	}

	// loop through all models in array
	// of model pointers, until nullptr
	for (model = models[0]; model != NULL; models++, model = models[0])
	{
		int *modelName = (int *)model;

		if ((modelName[0] == name[0]) && (modelName[1] == name[1]) && (modelName[2] == name[2]) && (modelName[3] == name[3]))
		{
			// found it
			return model;
		}
	}
	return NULL;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 overlay 230 0x800adc0c-0x800ae0bc PSX path.
void MM_Characters_DrawWindows(b32 boolShowDrivers)
{
	struct GameTracker *gGT;
	s16 uVar2;
	s16 uVar3;
	struct Model *uVar4;
	int iVar5;
	u32 iVar6;
	u32 uVar7;
	int iVar8;
	s16 sVar9;
	struct Instance *iVar10;
	s16 *psVar11;
	struct PushBuffer *pb;
	s16 *ptrCurr;
	int iVar14;
	struct TransitionMeta *tMeta;
	SVec3 rot;

	gGT = sdata->gGT;

	if (boolShowDrivers != 0)
	{
		// enable drawing wheels
		gGT->renderFlags |= 0x80;
	}

	for (iVar14 = 0; iVar14 < gGT->numPlyrNextGame; iVar14++)
	{
		psVar11 = &D230.characterSelect_ptrWindowXY[iVar14 * 2];
		tMeta = &D230.ptrTransitionMeta[iVar14];

		pb = &gGT->pushBuffer[iVar14];
		pb->rect.x = psVar11[0] + tMeta[0x10].currX;
		pb->rect.y = psVar11[1] + tMeta[0x10].currY;
		pb->rect.w = D230.characterSelect_sizeX;
		pb->rect.h = D230.characterSelect_sizeY;

		// negative StartX
		if ((s16)pb->rect.x < 0)
		{
			pb->rect.w -= pb->rect.x;
			pb->rect.x = 0;
			if ((s16)pb->rect.w < 0)
			{
				pb->rect.w = 0;
			}
		}

		// negative StartY
		if ((s16)pb->rect.y < 0)
		{
			pb->rect.h -= pb->rect.y;
			pb->rect.y = 0;
			if ((s16)pb->rect.h < 0)
			{
				pb->rect.h = 0;
			}
		}

		// startX + sizeX out of bounds
		if ((0x200 < pb->rect.x + pb->rect.w) && (pb->rect.w = 0x200 - pb->rect.x, pb->rect.w < 0))
		{
			pb->rect.x = 0x200;
			pb->rect.w = 0;

#ifdef CTR_NATIVE
			// NOTE(aalhendi): Native renderer guard; retail leaves w at zero.
			pb->rect.w = 1;
#endif
		}

		// startY + sizeY out of bounds
		if ((0xd8 < pb->rect.y + pb->rect.h) && (pb->rect.h = 0xd8 - pb->rect.y, pb->rect.h < 0))
		{
			pb->rect.y = 0xd8;
			pb->rect.h = 0;

#ifdef CTR_NATIVE
			// NOTE(aalhendi): Native renderer guard; retail leaves h at zero.
			pb->rect.h = 1;
#endif
		}

		// distanceToScreen
		pb->distanceToScreen_CURR = 0x100;
		pb->distanceToScreen_PREV = 0x100;

		// pushBuffer pos and rot to all zero
		pb->pos.x = 0;
		pb->pos.y = 0;
		pb->pos.z = 0;
		pb->rot.x = 0;
		pb->rot.y = 0;
		pb->rot.z = 0;

		// player -> instance
		iVar10 = gGT->drivers[iVar14]->instSelf;

		// Make Visible
		iVar10->flags &= 0xffffff7f;

		// if driver is off-screen
		if ((gGT->numPlyrNextGame <= iVar14) || (boolShowDrivers == 0))
		{
			// invisible
			iVar10->flags |= 0x80;
		}

		iVar6 = iVar14;

		struct InstDrawPerPlayer *idpp = INST_GETIDPP(iVar10);

		// clear pushBuffer in every InstDrawPerPlayer
		idpp[0].pushBuffer = 0;
		idpp[1].pushBuffer = 0;
		idpp[2].pushBuffer = 0;
		idpp[3].pushBuffer = 0;

		// set pushBuffer in InstDrawPerPlayer,
		// so that each camera can only see one driver
		idpp[iVar6].pushBuffer = pb;

		ptrCurr = &D230.characterSelect_charIDs_curr[iVar6];

		iVar10->animFrame = 0;
		iVar10->vertSplit = 0;

		uVar4 = MM_Characters_GetModelByName((int *)data.MetaDataCharacters[(int)*ptrCurr].name_Debug);

		// set modelPtr in Instance
		iVar10->model = uVar4;

		// CameraDC, freecam mode
		gGT->cameraDC[iVar6].cameraMode = 3;

		// Set position of player
		iVar10->matrix.t[0] = D230.csm_instPos.x;
		iVar10->matrix.t[1] = D230.csm_instPos.y;
		iVar10->matrix.t[2] = D230.csm_instPos.z;

		psVar11 = &D230.timerPerPlayer[iVar6];
		sVar9 = *psVar11 + -1;

		// If no transition between players
		if (*psVar11 == 0)
		{
			// compare to character ID
			if (*ptrCurr != data.characterIDs[iVar6])
			{
				*psVar11 = D230.moveModels << 1;
				D230.characterSelect_charIDs_desired[iVar6] = data.characterIDs[iVar6];
			}
		}

		// if transition between players
		else
		{
			// get timer
			*psVar11 = sVar9;

			// if timer is before midpoint
			if ((int)sVar9 < (int)D230.moveModels)
			{
				// make driver fly off screen
				*ptrCurr = D230.characterSelect_charIDs_desired[iVar6];
				iVar5 = RaceFlag_MoveModels((int)sVar9, (int)D230.moveModels);

				// direction moving
				iVar6 = -D230.characterSelect_MoveDir[iVar6];
				iVar8 = iVar5 * D230.unkCharacterWindows >> 0xc;
			}

			// if timer is after midpoint
			else
			{
				// make new driver fly on screen
				iVar5 = RaceFlag_MoveModels((int)sVar9 - (int)D230.moveModels, (int)D230.moveModels);

				// direction moving
				iVar8 = D230.characterSelect_MoveDir[iVar6];
				iVar6 = (0x1000 - iVar5) * (int)D230.unkCharacterWindows >> 0xc;
			}

			iVar10->matrix.t[0] += iVar6 * iVar8;
		}

		// driver rotation
		rot.x = D230.csm_instRot.x;
		rot.y = D230.csm_instRot.y + D230.characterSelect_angle[iVar14];
		rot.z = D230.csm_instRot.z;

		ConvertRotToMatrix(&iVar10->matrix, &rot);
	}
	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 overlay 230 0x800ae0bc-0x800ae274.
void MM_Characters_SetMenuLayout(void)
{
	u16 unlocked;
	char expand;
	int iVar3;
	int i;
	int numPlyrNextGame;

	expand = 0;

	// By default, draw "Select character" in 3P menu
	D230.isRosterExpanded = 0;

	numPlyrNextGame = sdata->gGT->numPlyrNextGame;

	iVar3 = numPlyrNextGame - 1;

	// original game
#define NUM_ICONS 0xF

	// Loop through bottom characters,
	// if any are unlocked, use expanded
	for (i = 0xc; i < NUM_ICONS; i++)
	{
		// OG game code
		unlocked = D230.csm_1P2P[i].unlockFlags;

		if (((sdata->gameProgress.unlocks[unlocked >> 5] >> (unlocked & 0x1f)) & 1) != 0)
		{
			expand = 1;
			break;
		}
	}

	if (
	    // if 1P2P (0 or 1)
	    (iVar3 < 2) &&

	    // if very few characters are unlocked
	    (!expand))
	{
		// layout [4] and [5] for 1P2P without expansion
		iVar3 += 4;
	}

	D230.isRosterExpanded = expand;

	D230.characterSelectIconLayout = iVar3;

	D230.csm_instPos.y = D230.driverPosY[iVar3];
	D230.csm_instPos.z = D230.driverPosZ[iVar3];

	D230.characterSelect_sizeX = D230.windowW[iVar3];
	D230.characterSelect_sizeY = D230.windowH[iVar3];

	D230.characterSelect_ptrWindowXY = D230.ptrSelectWindowPos[iVar3];

	D230.csm_Active = D230.ptrCsmArr[iVar3];

	D230.textPos = D230.textPosArr[iVar3];

	D230.ptrTransitionMeta = D230.ptr_transitionMeta_csm[numPlyrNextGame - 1];

	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800ae274-0x800ae2c0.
void MM_Characters_BackupIDs(void)
{
	char i;

	for (i = 0; i < 8; i++)
	{
		// make a backup when you leave character selection,
		// backup is restored when you go back to selection
		sdata->characterIDs_backup[i] = data.characterIDs[i];
	}
	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 overlay 230 0x800ae2c0-0x800ae464.
void MM_Characters_PreventOverlap(void)
{
	struct GameTracker *gGT = sdata->gGT;
	char cVar1;
	int iVar2;
	char *pcVar3;
	int iVar4;
	int iVar5;
	int iVar6;
	int iVar7;

	char local_8[8];

	// default 0,1,2,3,4,5,6,7
	*(int *)&local_8[0] = R230.characterID_default[0];
	*(int *)&local_8[4] = R230.characterID_default[1];

	iVar2 = 0;

	for (iVar7 = 0; iVar7 < gGT->numPlyrNextGame; iVar7++)
	{
		// get character ID
		iVar2 = data.characterIDs[iVar7];

		// if not a secret character
		if (iVar2 < 8)
		{
			// character is taken
			local_8[iVar2] = 0xff;
		}
	}

	for (iVar7 = 1; iVar7 < gGT->numPlyrNextGame; iVar7++)
	{
		for (iVar6 = 0; iVar6 < iVar7; iVar6++)
		{
			// if two characters are the same
			if (data.characterIDs[iVar7] == data.characterIDs[iVar6])
			{
				// look for a new character
				for (iVar5 = 0; iVar5 < 8; iVar5++)
				{
					// get default character
					pcVar3 = &local_8[iVar5];
					cVar1 = *pcVar3;

					// if character is not taken
					if (-1 < cVar1)
					{
						// assign free character
						data.characterIDs[iVar7] = (s16)cVar1;

						// character is now taken
						*pcVar3 = -1;

						break;
					}
				}
			}
		}
	}
	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 overlay 230 0x800ae464-0x800ae6b0.
void MM_Characters_RestoreIDs(void)
{
	struct GameTracker *gGT = sdata->gGT;
	s16 *currID;
	int iVar3;
	int iVar4;
	char i;
	s16 uVar1;

	// erase select bits
	sdata->characterSelectFlags = 0;
	D230.transitionFrames = 0xc;
	D230.isMenuTransitioning = 0;

	// This uses 80086e84, which controls character IDs
	// loop 8 times
	// shouldn't it only need to loop a maximum of 4 times?
	for (i = 0; i < 8; i++)
	{
		// set character ID to the last ID you entered
		data.characterIDs[i] = sdata->characterIDs_backup[i];
	}

	MM_Characters_SetMenuLayout();

#define NUM_ICONS 0xF

	for (i = 0; i < NUM_ICONS; i++)
	{
		// would not need this if CSM was sorted
		// by order of character ID

		// Basically sets them to 0, 1, 2, 3, 4... up to 0xE,
		// setting Oxide's manually to 0xF is needed to make his icon appear

		D230.characterMenuID[D230.csm_Active[i].characterID] = i;
	}

	for (i = 0; i < gGT->numPlyrNextGame; i++)
	{
		// Determine if this icon is unlocked (and drawing)

		// get character ID
		currID = &data.characterIDs[i];

		// get unlock requirement for this character
		uVar1 = D230.csm_Active[*currID].unlockFlags;

		if (
		    // If Icon has an unlock requirement
		    (uVar1 != -1) &&

		    // If Character is Locked
		    (((sdata->gameProgress.unlocks[uVar1 >> 5] >> (uVar1 & 0x1f)) & 1) == 0))
		{
			// change character to Crash
			*currID = 0;
		}
	}

	MM_Characters_PreventOverlap();

	for (i = 0; i < gGT->numPlyrNextGame; i++)
	{
		// set name string ID to the character ID of each player.
		// The string will only draw if both these variables match
		D230.characterSelect_charIDs_curr[i] = data.characterIDs[i];
		D230.characterSelect_charIDs_desired[i] = data.characterIDs[i];

		// something to do with transitioning between icons
		D230.timerPerPlayer[i] = 0;

		// rotation of each driver, 90 degrees difference
		D230.characterSelect_angle[i] = (i * 0x400) + 400;
	}

	MM_Characters_DrawWindows(0);
	return;
}

// NOTE(aalhendi): ASM-verified against NTSC-U 926 overlay 230 0x800ae6b0-0x800ae74c.
void MM_Characters_HideDrivers(void)
{
	char i;
	struct GameTracker *gGT = sdata->gGT;

	for (i = 0; i < 4; i++)
	{
		PushBuffer_Init(&gGT->pushBuffer[i], 0, 1);

		gGT->drivers[i]->instSelf->flags |= HIDE_MODEL;
	}

	return;
}

void MM_Characters_MenuProc(struct RectMenu *unused)
{
	u8 numPlyrNextGame;
	int bVar2;
	int bVar3;
	u16 characterSelectFlags5bit;
	s16 *psVar5;
	s16 sVar6;
	s16 nextDriver;
	int iVar8;
	u32 button;
	s16 sVar10;
	int globalIconPerPlayerCopy5;
	u8 *puVar12;
	u32 characterSelectType;
	u32 fontType;
	u32 iconColor;
	s16 globalIconPerPlayerCopy3;
	int nextDriverCopy;
	s16 globalIconPerPlayerCopy4;
	s16 globalIconPerPlayerCopy;
	s16 globalIconPerPlayerCopy2;
	u16 *globalIconPerPlayerPtr2;
	int iVar24;
	u32 k;
	u16 *puVar26;
	s16 globalIconPerPlayer[4];
	u8 auStack120[8];
	Color color;

	u8 colorRGBA[4];

	RECT r1;
	RECT *r = &r1;
	RECT r58;

	s16 local_50;
	s16 *globalIconPerPlayerPtr;

	int i;
	int j;
	int posX;
	int posY;
	s16 playerIcon;
	int direction;

	s16 *psVar22;
	struct CharacterSelectMeta *csm_Active;

	struct GameTracker *gGT = sdata->gGT;

	uint32_t *ot = gGT->backBuffer->otMem.uiOT;

	for (i = 0; i < 4; i++)
	{
		globalIconPerPlayer[i] = D230.characterMenuID[data.characterIDs[i]];
	}

	// if menu is not in focus
	if (D230.isMenuTransitioning != 1)
	{
		MM_TransitionInOut(D230.ptrTransitionMeta, (int)D230.transitionFrames, 8);
	}

	MM_Characters_SetMenuLayout();
	MM_Characters_DrawWindows(1);

	// if transitioning in
	if (D230.isMenuTransitioning == 0)
	{
		// if no more frames
		if (D230.transitionFrames == 0)
		{
			// menu is now in focus
			D230.isMenuTransitioning = 1;
		}
		else
		{
			D230.transitionFrames--;
		}
	}

	// if transitioning out
	if (D230.isMenuTransitioning == 2)
	{
		// increase frame
		D230.transitionFrames++;

		// if more than 12 frames
		if (D230.transitionFrames > 12)
		{
			// Make a backup of the characters
			// you selected in character selection screen
			MM_Characters_BackupIDs();

			// if returning to main menu
			if (D230.movingToTrackMenu == 0)
			{
				MM_JumpTo_Title_Returning();
				MM_Characters_HideDrivers();
				return;
			}

			MM_Characters_HideDrivers();

			// if you are in a cup
			if ((gGT->gameMode2 & CUP_ANY_KIND) != 0)
			{
				sdata->ptrDesiredMenu = &D230.menuCupSelect;
				MM_CupSelect_Init();
				return;
			}

			// if going to track selection
			sdata->ptrDesiredMenu = &D230.menuTrackSelect;
			MM_TrackSelect_Init();
			return;
		}
	}

	posX = D230.ptrTransitionMeta[15].currX;
	posY = D230.ptrTransitionMeta[15].currY;

	char *characterSelectString;
	switch (D230.characterSelectIconLayout)
	{
	// 3P character selection
	case 2:

		// If you have a lot of characters unlocked, do not draw SELECT CHARACTER
		if (D230.isRosterExpanded)
		{
			goto dontDrawSelectCharacter;
		}

		DecalFont_DrawLine(sdata->lngStrings[LNG_SELECT_CHARACTER_SELECT], posX + 0x9c, posY + 0x14, FONT_BIG, (JUSTIFY_CENTER | ORANGE));
		characterSelectType = FONT_BIG;

		characterSelectString = sdata->lngStrings[LNG_CHARACTER];

		posX = posX + 0x9c;
		posY = posY + 0x26;
		break;

	// 4P character selection
	case 3:

		// If Fake Crash is unlocked, do not draw "Select Character"
		if (sdata->gameProgress.unlockFlags & UNLOCK_FAKE_CRASH)
		{
			goto dontDrawSelectCharacter;
		}

		DecalFont_DrawLine(sdata->lngStrings[LNG_SELECT_CHARACTER_SELECT], posX + 0xfc, posY + 8, FONT_CREDITS, (JUSTIFY_CENTER | ORANGE));
		characterSelectType = FONT_CREDITS;

		characterSelectString = sdata->lngStrings[LNG_CHARACTER];

		posX = posX + 0xfc;
		posY = posY + 0x18;
		break;

	// If you are in 1P or 2P character selection,
	// when you do NOT have a lot of characters selected
	case 4:
	case 5:
		characterSelectType = FONT_BIG;

		characterSelectString = sdata->lngStrings[LNG_SELECT_CHARACTER];

		posX = posX + 0xfc;
		posY = posY + 10;
		break;

	default:
		goto dontDrawSelectCharacter;
	}

	// Draw String
	DecalFont_DrawLine(characterSelectString, posX, posY, characterSelectType, (JUSTIFY_CENTER | ORANGE));

dontDrawSelectCharacter:

	globalIconPerPlayerPtr = &globalIconPerPlayer[0];

	for (i = 0; i < gGT->numPlyrNextGame; i++)
	{
		characterSelectFlags5bit = (u16)(1 << i);
		globalIconPerPlayerCopy = globalIconPerPlayerPtr[i];
		globalIconPerPlayerCopy2 = globalIconPerPlayerCopy;

		MM_Characters_AnimateColors(auStack120, i, (int)(s16)(sdata->characterSelectFlags & characterSelectFlags5bit));

		puVar26 = (u16 *)&D230.csm_Active[globalIconPerPlayerCopy];

		if (

		    (D230.isMenuTransitioning == 1) &&
		    (
		        // get input from this player
		        button = sdata->buttonTapPerPlayer[i],

		        // If you press the D-Pad, or Cross, Square, Triangle, Circle
		        button & (BTN_TRIANGLE | BTN_CIRCLE | BTN_SQUARE_one | BTN_CROSS_one | BTN_RIGHT | BTN_LEFT | BTN_DOWN | BTN_UP)))
		{
			// if character has not been selected by this player
			if (((int)(s16)sdata->characterSelectFlags >> i & 1U) == 0)
			{
				// If you pressed any of the D-pad buttons
				if ((button & (BTN_RIGHT | BTN_LEFT | BTN_DOWN | BTN_UP)) != 0)
				{
					local_50 = 0;

					// If you do not press Up
					if ((button & BTN_UP) == 0)
					{
						// If you do not press Down
						if ((button & BTN_DOWN) == 0)
						{
							// This must be if you press Left,
							// because the variable will change
							// if it is anything that isn't Left

							// Left
							direction = 2;

							// If you press Left
							if ((button & BTN_LEFT) != 0)
							{
								goto LAB_800aec08;
							}

							// At this point, you must have pressed Right

							// Right
							direction = 3;

							// Move down character selection list
							D230.characterSelect_MoveDir[i] = 1;
						}

						// If you pressed Down
						else
						{
							// Down
							direction = 1;

							// Move down character selection list
							D230.characterSelect_MoveDir[i] = 1;
						}
					}

					// If you pressed Up
					else
					{
						// Up
						direction = 0;
					LAB_800aec08:
						// If you press Up or Left

						// Move up character selection list
						(D230.characterSelect_MoveDir)[i] = 0xffff;
					}

					j = i;
					globalIconPerPlayerPtr2 = &globalIconPerPlayerPtr[j];
					globalIconPerPlayerCopy3 = globalIconPerPlayerCopy2;
					do
					{
						globalIconPerPlayerCopy2 = MM_Characters_GetNextDriver(direction, globalIconPerPlayerCopy3);
						globalIconPerPlayerCopy4 = globalIconPerPlayerCopy2;

						if (globalIconPerPlayerCopy2 == globalIconPerPlayerCopy3)
						{
							local_50 = 1;
							nextDriver = MM_Characters_GetNextDriver(direction, (int)(s16)*globalIconPerPlayerPtr2);
							nextDriverCopy = (int)nextDriver;
							globalIconPerPlayerCopy2 = MM_Characters_GetNextDriver((u32)(u8)D230.getNextDriver1[direction], nextDriverCopy);
							globalIconPerPlayerCopy5 = (int)(s16)globalIconPerPlayerCopy2;

							if ((((globalIconPerPlayerCopy5 == globalIconPerPlayerCopy4) || (nextDriverCopy == globalIconPerPlayerCopy4)) ||
							     (nextDriverCopy == globalIconPerPlayerCopy5)) ||
							    (button = MM_Characters_boolIsInvalid(globalIconPerPlayerPtr, globalIconPerPlayerCopy5, j), (button & 0xffff) != 0))
							{
								nextDriver = MM_Characters_GetNextDriver((u32)(u8)D230.getNextDriver1[direction], (int)(s16)*globalIconPerPlayerPtr2);
								globalIconPerPlayerCopy5 = (int)nextDriver;
								globalIconPerPlayerCopy2 = MM_Characters_GetNextDriver(direction, globalIconPerPlayerCopy5);
								globalIconPerPlayerCopy4 = (int)(s16)globalIconPerPlayerCopy2;

								if (((globalIconPerPlayerCopy4 == globalIconPerPlayerCopy3) || (globalIconPerPlayerCopy5 == globalIconPerPlayerCopy3)) ||
								    ((globalIconPerPlayerCopy5 == globalIconPerPlayerCopy4 ||
								      (button = MM_Characters_boolIsInvalid(globalIconPerPlayerPtr, globalIconPerPlayerCopy4, j), (button & 0xffff) != 0))))
								{
									nextDriver = MM_Characters_GetNextDriver(direction, (int)(s16)*globalIconPerPlayerPtr2);
									globalIconPerPlayerCopy5 = (int)nextDriver;
									globalIconPerPlayerCopy2 = MM_Characters_GetNextDriver((u32)(u8)D230.getNextDriver2[direction], globalIconPerPlayerCopy5);
									globalIconPerPlayerCopy4 = (int)(s16)globalIconPerPlayerCopy2;

									if (((globalIconPerPlayerCopy4 == globalIconPerPlayerCopy3) || (globalIconPerPlayerCopy5 == globalIconPerPlayerCopy3)) ||
									    ((globalIconPerPlayerCopy5 == globalIconPerPlayerCopy4 ||
									      (button = MM_Characters_boolIsInvalid(globalIconPerPlayerPtr, globalIconPerPlayerCopy4, j), (button & 0xffff) != 0))))
									{
										nextDriver = MM_Characters_GetNextDriver((u32)(u8)D230.getNextDriver2[direction], (int)(s16)*globalIconPerPlayerPtr2);
										globalIconPerPlayerCopy5 = (int)nextDriver;
										globalIconPerPlayerCopy2 = MM_Characters_GetNextDriver(direction, globalIconPerPlayerCopy5);
										globalIconPerPlayerCopy4 = (int)(s16)globalIconPerPlayerCopy2;

										if ((((globalIconPerPlayerCopy4 == globalIconPerPlayerCopy3) ||
										      (globalIconPerPlayerCopy5 == globalIconPerPlayerCopy3)) ||
										     (globalIconPerPlayerCopy5 == globalIconPerPlayerCopy4)) ||
										    (button = MM_Characters_boolIsInvalid(globalIconPerPlayerPtr, globalIconPerPlayerCopy4, j), (button & 0xffff) != 0))
										{
											globalIconPerPlayerCopy2 = (u32)*globalIconPerPlayerPtr2;
										}
									}
								}
							}
						}
						bVar2 = false;

						for (k = 0; k < (u32)gGT->numPlyrNextGame; k++)
						{
							if (((int)k != j) && ((s16)globalIconPerPlayerCopy2 == globalIconPerPlayerPtr[k]))
							{
								bVar2 = true;
								break;
							}
						}

						if (globalIconPerPlayerCopy3 << 0x10 != globalIconPerPlayerCopy2 << 0x10)
						{
							// Play sound
							// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800aeeb8-0x800aeecc for character cursor-change SFX.
							OtherFX_Play(0, 1);
						}
						if (local_50 != 0)
						{
							bVar3 = !bVar2;
							bVar2 = false;
							if (bVar3)
							{
								break;
							}
							globalIconPerPlayerCopy2 = (u32)*globalIconPerPlayerPtr2;
						}
						globalIconPerPlayerCopy3 = globalIconPerPlayerCopy2;
					} while (bVar2);
				}
				globalIconPerPlayerCopy = (u16)globalIconPerPlayerCopy2;

				for (j = 0; j < gGT->numPlyrNextGame; j++)
				{
					if ((j != i) && ((s16)globalIconPerPlayerCopy2 == globalIconPerPlayerPtr[j]))
					{
						globalIconPerPlayerCopy2 = (u32)(u16)globalIconPerPlayerPtr[i];
					}
					globalIconPerPlayerCopy = (u16)globalIconPerPlayerCopy2;
				}

				// If this player pressed Cross or Circle
				if (((sdata->buttonTapPerPlayer)[i] & (BTN_CIRCLE | BTN_CROSS_one)) != 0)
				{
					// this player has now selected a character
					sdata->characterSelectFlags = sdata->characterSelectFlags | (u16)(1 << i);

					numPlyrNextGame = gGT->numPlyrNextGame;

					// Play sound
					// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800aefa4-0x800aefe4 for character confirm SFX.
					OtherFX_Play(1, 1);

					// if all players have selected their characters
					if ((int)(s16)sdata->characterSelectFlags == (1 << numPlyrNextGame) - 1)
					{
						// move to track selection
						D230.movingToTrackMenu = 1;
						D230.isMenuTransitioning = 2;
					}
				}

				if (
				    // if this is the first iteration of the loop
				    ((i & 0xffff) == 0) &&

				    // if you press Square or Triangle
				    ((sdata->buttonTapPerPlayer[0] & (BTN_TRIANGLE | BTN_SQUARE_one)) != 0))
				{
					// return to main menu
					D230.movingToTrackMenu = 0;
					D230.isMenuTransitioning = 2;

					// Play sound
					// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800af01c-0x800af054 for character-select back SFX.
					OtherFX_Play(2, 1);
				}
			}
			else
			{
				// if you press Square or Triangle
				if ((button & (BTN_TRIANGLE | BTN_SQUARE_one)) != 0)
				{
					// Play sound
					// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800af060-0x800af074 for character deselect SFX.
					OtherFX_Play(2, 1);

					// this player has de-selected their character
					sdata->characterSelectFlags = sdata->characterSelectFlags & ~characterSelectFlags5bit;
				}
			}

			// clear input
			sdata->buttonTapPerPlayer[i] = 0;
		}

		globalIconPerPlayerPtr[i] = globalIconPerPlayerCopy;

		// transition of each icon
		iVar24 = (int)&D230.ptrTransitionMeta[globalIconPerPlayerCopy];

		// if player has not selected a character
		if (((sdata->characterSelectFlags >> i) & 1U) == 0)
		{
			// draw string
			// "1", "2", "3", "4", above the character icon
			DecalFont_DrawLine(D230.PlayerNumberStrings[i], ((struct TransitionMeta *)iVar24)->currX + (u32)*puVar26 + -6,
			                   ((struct TransitionMeta *)iVar24)->currY + (u32)puVar26[1] + -3, FONT_BIG, WHITE);
			puVar12 = auStack120;
		}
		else
		{
			puVar12 = (u8 *)&D230.characterSelect_Outline;
		}

		r->x = ((struct TransitionMeta *)iVar24)->currX + *puVar26;
		r->y = ((struct TransitionMeta *)iVar24)->currY + puVar26[1];
		r->w = 0x34;
		r->h = 0x21;

		color = *(Color *)puVar12;
		RECTMENU_DrawOuterRect_HighLevel(r, color, 0, ot);
	}

	MM_Characters_PreventOverlap();

	csm_Active = D230.csm_Active;

#define NUM_ICONS 0xF

	// loop through character icons
	for (i = 0; i < NUM_ICONS; i++)
	{
		iVar8 = csm_Active->unlockFlags;
		if (
		    // If Icon is unlocked by default,
		    // dont use iVar8, must be interpeted as "s16"
		    ((s16)csm_Active->unlockFlags == -1) ||

		    // if character is unlocked
		    // from 4-byte variable that handles all rewards
		    // also the variable written by cheats
		    (((sdata->gameProgress.unlocks[iVar8 >> 5] >> (iVar8 & 0x1f)) & 1) != 0) ||
			g_config.unlockAllCharacters) 
		{
			iconColor = D230.characterSelect_NeutralColor;

			for (j = 0; j < gGT->numPlyrNextGame; j++)
			{
				if (((s16)i == globalIconPerPlayer[j]) &&

				    // if player selected a character
				    (((int)(s16)sdata->characterSelectFlags >> (j & 0x1fU) & 1U) != 0))
				{
					iconColor = D230.characterSelect_ChosenColor;
				}
			}

			iVar8 = (int)&D230.ptrTransitionMeta[i];

			RECTMENU_DrawPolyGT4(gGT->ptrIcons[data.MetaDataCharacters[csm_Active->characterID].iconID],
			                     ((struct TransitionMeta *)iVar8)->currX + csm_Active->posX + 6, ((struct TransitionMeta *)iVar8)->currY + csm_Active->posY + 4,

			                     &gGT->backBuffer->primMem, gGT->pushBuffer_UI.ptrOT,

			                     iconColor, iconColor, iconColor, iconColor, TRANS_50_DECAL, FP(1.0));
		}

		csm_Active++;
	}

	// reset
	csm_Active = D230.csm_Active;

	for (i = 0; i < 4; i++)
	{
		data.characterIDs[i] = csm_Active[(int)globalIconPerPlayer[i]].characterID;
	}

	for (i = 0; i < gGT->numPlyrNextGame; i++)
	{
		j = i;
		playerIcon = globalIconPerPlayer[j];
		csm_Active = &D230.csm_Active[playerIcon];

		// if player has not selected a character
		if (((int)(s16)sdata->characterSelectFlags >> j & 1U) == 0)
		{
			MM_Characters_AnimateColors((u8 *)&colorRGBA, j,

			                            // flags of which characters are selected
			                            (int)(s16)(sdata->characterSelectFlags & (u16)(1 << j)));

			colorRGBA[0] = (u8)((int)((u32)colorRGBA[0] << 2) / 5);
			colorRGBA[1] = (u8)((int)((u32)colorRGBA[1] << 2) / 5);
			colorRGBA[2] = (u8)((int)((u32)colorRGBA[2] << 2) / 5);

			iVar8 = (int)&D230.ptrTransitionMeta[playerIcon];

			r->x = ((struct TransitionMeta *)iVar8)->currX + csm_Active->posX + 3;
			r->y = ((struct TransitionMeta *)iVar8)->currY + csm_Active->posY + 2;
			r->w = 0x2e;
			r->h = 0x1d;

			Color color = *(Color *)&colorRGBA;
			// this draws the flashing blue square that appears when you highlight a character in the character select screen
			CTR_Box_DrawSolidBox(r, color, ot);
		}
		if ((D230.timerPerPlayer[j] == 0) && (D230.characterSelect_charIDs_curr[j] == data.characterIDs[j]))
		{
			// get number of players
			numPlyrNextGame = gGT->numPlyrNextGame;

			// if number of players is 1 or 2
			fontType = FONT_CREDITS;

			// if number of players is 3 or 4
			if (numPlyrNextGame >= 3)
			{
				fontType = FONT_SMALL;
			}

			iVar8 = (int)&D230.ptrTransitionMeta[j + 0x10];
			sVar10 = ((struct TransitionMeta *)iVar8)->currY + D230.characterSelect_ptrWindowXY[j * 2 + 1];
			sVar6 = (s16)((((u32)(numPlyrNextGame < 3) ^ 1) << 0x12) >> 0x10);

			if ((numPlyrNextGame == 4) && (j > 1))
			{
				sVar6 = sVar10 + sVar6 - 6;
			}
			else
			{
				sVar6 = sVar10 + D230.textPos + sVar6;
			}

			// draw string
			DecalFont_DrawLine(sdata->lngStrings[data.MetaDataCharacters[csm_Active->characterID].name_LNG_long],
			                   (int)((struct TransitionMeta *)iVar8)->currX + D230.characterSelect_ptrWindowXY[j * 2] +
			                       (int)((u32)D230.characterSelect_sizeX >> 1),
			                   (int)sVar6, fontType, (JUSTIFY_CENTER | ORANGE));
		}

		// spin the character
		D230.characterSelect_angle[i] += 0x40;
	}

	// reset
	csm_Active = D230.csm_Active;

	// loop through all icons
	for (i = 0; i < NUM_ICONS; i++)
	{
		iVar8 = csm_Active[i].unlockFlags;

		if (
		    // If Icon is unlocked (from array of icons)
		    ((s16)csm_Active[i].unlockFlags == -1) ||

		    // if character is unlocked
		    // from 4-byte variable that handles all rewards
		    // also the variable written by cheats
		    ((sdata->gameProgress.unlocks[iVar8 >> 5] >> (iVar8 & 0x1fU) & 1) != 0) ||
			g_config.unlockAllCharacters) 
		{
			iVar8 = (int)&D230.ptrTransitionMeta[i];

			r->x = ((struct TransitionMeta *)iVar8)->currX + csm_Active[i].posX;
			r->y = ((struct TransitionMeta *)iVar8)->currY + csm_Active[i].posY;
			r->w = 0x34;
			r->h = 0x21;

			// Draw 2D Menu rectangle background
			RECTMENU_DrawInnerRect(r, 0, ot);
		}
	}

	psVar22 = D230.characterSelect_ptrWindowXY;

	for (i = 0; i < gGT->numPlyrNextGame; i++)
	{
		j = i;
		iVar8 = (int)&D230.ptrTransitionMeta[j];

		// store window width and height in one 4-byte variable
		r->x = *(s16 *)(iVar8 + 0xa6) + *psVar22;
		r->y = *(s16 *)(iVar8 + 0xa8) + psVar22[1];
		r->w = D230.characterSelect_sizeX;
		r->h = D230.characterSelect_sizeY;

		MM_Characters_AnimateColors((u8 *)&colorRGBA, j,

		                            // flags of which characters are selected
		                            ((int)(s16)sdata->characterSelectFlags >> j ^ 1U) & 1);

		color = *(Color *)&colorRGBA;
		RECTMENU_DrawOuterRect_HighLevel(r, color, 0, ot);

		// if player selected a character
		if (((int)(s16)sdata->characterSelectFlags >> j & 1U) != 0)
		{
			r58.x = r->x;
			r58.y = r->y;
			r58.w = r->w;
			r58.h = r->h;

			for (iVar8 = 0; iVar8 < 2; iVar8++)
			{
				r58.x += 3;
				r58.y += 2;
				r58.w -= 6;
				r58.h -= 4;

				colorRGBA[0] = (u8)((int)((u32)colorRGBA[0] << 2) / 5);
				colorRGBA[1] = (u8)((int)((u32)colorRGBA[1] << 2) / 5);
				colorRGBA[2] = (u8)((int)((u32)colorRGBA[2] << 2) / 5);

				color = *(Color *)&colorRGBA;
				RECTMENU_DrawOuterRect_HighLevel(&r58, color, 0, ot);
			}
		}
		psVar22 = psVar22 + 2;

		// Draw 2D Menu rectangle background
		RECTMENU_DrawInnerRect(r, 9, &ot[3]);

		// not screen-space anymore,
		// this is viewport-space
		r->x = 0;
		r->y = 0;

		RECTMENU_DrawRwdBlueRect(r, &D230.characterSelect_BlueRectColors[0], &gGT->pushBuffer[i].ptrOT[0x3ff], &gGT->backBuffer->primMem);
	}
	return;
}
