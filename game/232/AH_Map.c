#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b0b98-0x800b0ce0.
void AH_Map_LoadSave_Prim(s16 *vertPos, char *vertCol, void *ot, struct PrimMem *primMem)
{
	POLY_G4 *p = primMem->cursor;

	if (primMem->end < (void *)p)
	{
		return;
	}

	primMem->cursor = p + 1;

	setPolyG4(p);

	p->r0 = vertCol[0];
	p->g0 = vertCol[1];
	p->b0 = vertCol[2];

	p->r1 = vertCol[4];
	p->g1 = vertCol[5];
	p->b1 = vertCol[6];

	p->r2 = vertCol[8];
	p->g2 = vertCol[9];
	p->b2 = vertCol[10];

	p->r3 = vertCol[12];
	p->g3 = vertCol[13];
	p->b3 = vertCol[14];

	p->x0 = vertPos[0x0];
	p->y0 = vertPos[0x1];

	p->x1 = vertPos[0x2];
	p->y1 = vertPos[0x3];

	p->x2 = vertPos[0x4];
	p->y2 = vertPos[0x5];

	p->x3 = vertPos[0x6];
	p->y3 = vertPos[0x7];

	AddPrim(ot, p);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b0ce0-0x800b0f18.
void AH_Map_LoadSave_Full(int posX, int posY, s16 *vertPos, char *vertCol, int unk800, int angle)
{
	s16 local_30[8];
	s16 local_20[8];

	struct GameTracker *gGT = sdata->gGT;

	int sin = MATH_Sin(angle);
	int cos = MATH_Cos(angle);

	for (int i = 0; i < 4; i++)
	{
		local_30[i * 2 + 0] = posX + 6 +
		                      (s16)(((((vertPos[2 * i + 0] * cos) >> 0xc) + ((vertPos[2 * i + 1] * sin) >> 0xc)) * ((unk800 * 8) / 5)

		                                 ) >>
		                            0xc);

		local_30[i * 2 + 1] = posY + 4 +
		                      (s16)(((((vertPos[2 * i + 1] * cos) >> 0xc) - ((vertPos[2 * i + 0] * sin) >> 0xc)) * unk800

		                             ) >>
		                            0xc);
	}

	s16 *offset = &D232.primOffsetXY_LoadSave[0];

	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			local_20[j * 2 + 0] = local_30[j * 2 + 0] + offset[i * 2 + 0];

			local_20[j * 2 + 1] = local_30[j * 2 + 1] + offset[i * 2 + 1];
		}

		AH_Map_LoadSave_Prim(&local_20[0], vertCol, gGT->pushBuffer_UI.ptrOT, &gGT->backBuffer->primMem);

		vertCol = (char *)&D232.colorQuad[0];
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b0f18-0x800b1150.
void AH_Map_HubArrow(int posX, int posY, s16 *vertPos, char *vertCol, int unk800, int angle)
{
	s16 local_30[6];
	s16 local_20[6];

	struct GameTracker *gGT = sdata->gGT;

	int sin = MATH_Sin(angle);
	int cos = MATH_Cos(angle);

	for (int i = 0; i < 3; i++)
	{
		local_30[i * 2 + 0] = posX + 6 +
		                      (s16)(((((vertPos[2 * i + 0] * cos) >> 0xc) + ((vertPos[2 * i + 1] * sin) >> 0xc)) * ((unk800 * 8) / 5)

		                                 ) >>
		                            0xc);

		local_30[i * 2 + 1] = posY + 4 +
		                      (s16)(((((vertPos[2 * i + 1] * cos) >> 0xc) - ((vertPos[2 * i + 0] * sin) >> 0xc)) * unk800

		                             ) >>
		                            0xc);
	}

	s16 *offset = &D232.primOffsetXY_HubArrow[0];

	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			local_20[j * 2 + 0] = local_30[j * 2 + 0] + offset[i * 2 + 0];

			local_20[j * 2 + 1] = local_30[j * 2 + 1] + offset[i * 2 + 1];
		}

		RECTMENU_DrawRwdTriangle(&local_20[0], vertCol, gGT->pushBuffer_UI.ptrOT, &gGT->backBuffer->primMem);

		vertCol = (char *)&D232.colorTri[0];
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b1150-0x800b14f4.
void AH_Map_HubArrowOutter(void *hubPtrs, int arrowIndex, int posX, int posY, int inputAngle, int type)
{
	struct GameTracker *gGT;
	gGT = sdata->gGT;

	arrowIndex = (s16)arrowIndex;
	type = (s16)type;

	posX += D232.hubArrowXY_Inner[2 * type + 0];
	posY += D232.hubArrowXY_Inner[2 * type + 1];

	int timer = gGT->timer >> 0;

	int var14;
	int var15;
	int var8;

	var15 = 0x40;
	if ((timer & 1) != 0)
	{
		var15 = 0xe0;
	}

	if (type == 0)
	{
		var14 = var15;
		var8 = 0x200;
	}

	else if (type == 1)
	{
		var14 = 0xff;
		var8 = 0x555;

		posX += D232.hubArrowXY_Outter[2 * (((inputAngle >> 0x8) & 0xc) >> 2) + 0];
		posY += D232.hubArrowXY_Outter[2 * (((inputAngle >> 0x8) & 0xc) >> 2) + 1];
	}

	else
	{
		var14 = var15;
		var8 = 0x199;
		inputAngle ^= 0x800;
	}

	inputAngle = (s16)inputAngle;

	for (int iVar10 = 0; iVar10 < 3; iVar10++)
	{
		u32 var5 = (~(timer + (int)arrowIndex * 0xc) & 0x3f) + (2 - (int)(s16)iVar10) * -6;

		if (var5 >= 0xc)
		{
			continue;
		}

		int iVar16 = ((var5 * 0x2aa + 0x1000) * 0x10000) >> 0x1a;

		int bVar1 = 1;

		int shiftToggle = 1;

		int iVar6 = 0;
		int iVar9 = 0;

		for (int iVar13 = 0; iVar13 < var8 + 0xfff; iVar13 += var8)
		{
			if (type != 2)
			{
				shiftToggle = 0;
			}

			int angle = iVar13 + inputAngle;

			int sin = MATH_Sin(angle);
			int cos = MATH_Cos(angle);

			int iVar4 = (shiftToggle & 1) + 0xc;

			sin = posX + ((((iVar16 << 3) / 5) * sin) >> iVar4);
			cos = posY - ((((iVar16)) * cos) >> iVar4);

			if (!bVar1)
			{
				CTR_Box_DrawWirePrims((Point){iVar9, iVar6}, (Point){sin, cos}, MakeColor(var14, var15, 0xff), (void *)gGT->pushBuffer_UI.ptrOT);
			}

			bVar1 = 0;
			iVar9 = sin;
			iVar6 = cos;
			shiftToggle++;
		}
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b14f4-0x800b1a18.
void AH_Map_HubItems(void *hubPtrs, s16 *param_2)
{
	struct GameTracker *gGT;
	struct AdvProgress *adv;
	s16 levelID;
	s16 hubID;
	s16 sVar1;
	s16 *trophies;
	b32 open;
	int iVar3;
	u32 bit;
	int iVar5;
	int uVar6;
	s16 sVar7;
	s16 sVar8;
	s16 *psVar9;
	s16 *psVar10;
	int pos3D[3];
	int local_40;
	int local_3c;
	int local_38;
	int local_34;
	int local_30;
	int local_2c;

	gGT = sdata->gGT;
	adv = &sdata->advProgress;
	levelID = gGT->levelID;

	psVar10 = D232.hubItemsXY_ptrArray[levelID - GEM_STONE_VALLEY];
	if (*psVar10 != -1)
	{
		psVar9 = psVar10 + 1;
		do
		{
			sVar8 = -1;
			sVar8 = -1;
			sVar7 = (s16)0xffffffff; //???
			sVar7 = -1;
			sVar7 = -1;

			// iconType
			sVar1 = psVar9[2];

			open = true;

			// Arrow beach->gemstone
			if (sVar1 == -1)
			{
				sVar7 = 0;

				if (levelID == N_SANITY_BEACH)
				{
					// locked if key < 1
					sVar7 = (gGT->currAdvProfile.numKeys < 1);
				}

			LAB_800b17e8:
				iVar5 = sVar7 << 0x10;
				sVar8 = sVar8;
				sVar7 = (s16)sVar7;
			LAB_800b17ec:
				iVar5 = iVar5 >> 0x10;
			}
			else
			{
				if (-1 < sVar1)
				{
					sVar7 = sVar7;

					// gemstone valley
					if (sVar1 == 4)
					{
						iVar3 = 0;
						iVar5 = 0;

						// check 4 boss keys
						for (iVar3 = 0; iVar3 < 4; iVar3++)
						{
							bit = iVar3 + ADV_REWARD_FIRST_BOSS_KEY;

							if (CHECK_ADV_BIT(adv->rewards, bit) == 0)
							{
								open = false;
								break;
							}
						}
						if (!open)
						{
						LAB_800b17e4:
							sVar8 = 0;
							goto LAB_800b17e8;
						}
						sVar7 = sdata->advProgress.storyFlags & ADV_REWARD_BEAT_OXIDE_FIRST_BOSS_MASK;
					}

					// not gemstone valley
					else
					{
						iVar5 = 0;

						if (3 < sVar1)
						{
							iVar5 = -0x10000;
							sVar8 = sVar8;

							// saveLoad screen (0x64)
							if (sVar1 == 100)
							{
								local_40 = (int)*psVar10 + -0x200;
								local_3c = (int)*psVar9 + -0x100;

								UI_Map_GetIconPos(hubPtrs, &local_40, &local_3c);

								AH_Map_LoadSave_Full(local_40, local_3c, &D232.loadSave_pos[0], (char *)&D232.loadSave_col[0], 0x800, (int)psVar9[1]);

								iVar5 = -0x10000;
							}
							goto LAB_800b17ec;
						}

						// did not use GOTO,
						// must be == 3, for Boss Garage

						int base = levelID - N_SANITY_BEACH;

						for (iVar3 = 0; iVar3 < 4; iVar3++)
						{
							trophies = &data.advHubTrackIDs[base * 4];

							if (CHECK_ADV_BIT(adv->rewards, trophies[iVar3] + ADV_REWARD_FIRST_TROPHY) == 0)
							{
								open = false;
								break;
							}
						}
						if (!open)
						{
							goto LAB_800b17e4;
						}

						// check if key is unlocked
						sVar7 = CHECK_ADV_BIT(adv->rewards, base + ADV_REWARD_FIRST_BOSS_KEY);
					}

					// open, not beaten
					sVar8 = 1;

					iVar5 = -0x10000;

					// boss is beaten
					if (sVar7 != 0)
					{
						sVar8 = 2;
					}
					goto LAB_800b17ec;
				}

				// Arrow beach->glacier
				if (sVar1 == -4)
				{
					// locked if keys < 2
					sVar7 = ((gGT->currAdvProfile.numKeys) < 2);
					goto LAB_800b17e8;
				}
				if (sVar1 < -3)
				{
					// Arrow glacier->citadel
					if (sVar1 == -5)
					{
						// locked if keys < 3
						sVar7 = ((gGT->currAdvProfile.numKeys) < 3);
						goto LAB_800b17e8;
					}
					iVar5 = -1;
				}

				else
				{
					// either arrow on Gemstone hub,
					// pointing to beach or to ruins
					if ((sVar1 == -3) || (sVar1 == -2))
					{
						// never locked
						sVar7 = 0;

						goto LAB_800b17e8;
					}
					iVar5 = -1;
				}
			}

			if (-1 < iVar5)
			{
				local_38 = (int)*psVar10 + -0x200;
				local_34 = (int)*psVar9 + -0x100;
				UI_Map_GetIconPos(hubPtrs, &local_38, &local_34);
				if ((iVar5 == 0) && (D232.unkModeHubItems == 0))
				{
					AH_Map_HubArrowOutter(hubPtrs, (int)*param_2, local_38, local_34, (0x1000 - (u16)psVar9[1]), 1);
					*param_2 = *param_2 + 1;
				}

				// if even frame
				if ((gGT->timer & 2) == 0)
				{
					iVar5 = (int)sVar7 * 6;
				}
				else
				{
					iVar5 = ((int)sVar7 * 2 + 1) * 3;
				}

				AH_Map_HubArrow(local_38, local_34, &D232.hubArrow_pos[0], (char *)&D232.hubArrow_col1[iVar5], 0x800, (int)psVar9[1]);
			}

			if (-1 < sVar8)
			{
				pos3D[0] = (int)*psVar10;
				pos3D[1] = 0;
				pos3D[2] = (int)*psVar9;

				// if beat boss race
				if (sVar8 == 2)
				{
					// red
					uVar6 = 3;
				}
				else
				{
					// locked boss race
					// sVar6 == 0

					// grey
					uVar6 = 0x17;

					// open, not beaten
					if (sVar8 == 1)
					{
						// blue and white
						// depending on frames
						uVar6 = 5;
						if ((gGT->timer & 2) != 0)
						{
							uVar6 = 4;
						}
					}
				}

				// open, not beaten
				if (sVar8 == 1)
				{
					D232.unkModeHubItems = sVar8;
					local_30 = pos3D[0];
					local_2c = pos3D[2];

					UI_Map_GetIconPos(hubPtrs, &local_30, &local_2c);

					AH_Map_HubArrowOutter(hubPtrs, (int)*param_2, local_30, local_2c, 0, 2);

					*param_2 = *param_2 + 1;
				}

				// draw star icon for boss
				UI_Map_DrawRawIcon((int)hubPtrs, &pos3D[0], 0x37, uVar6, 0, 0x1000);
			}
			psVar10 = psVar10 + 4;
			psVar9 = psVar9 + 4;
		} while (*psVar10 != -1);
	}
	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b1a18-0x800b1c90.
void AH_Map_Warppads(s16 *ptrMap, struct Thread *warppadThread, s16 *param_3)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Instance *warppadInst;
	struct Instance *closestWarppadInst;
	int distX;
	int distY;
	int distZ;
	int color;
	int currDistance;
	int minDistance;
	int posX;
	int posY;

	// find minDistance, set to max
	minDistance = 0x7fffffff;
	closestWarppadInst = NULL;

	MATRIX *dMat = &gGT->drivers[0]->instSelf->matrix;

	for (
	    /**/; warppadThread != NULL; warppadThread = warppadThread->siblingThread)
	{
		int index = warppadThread->modelIndex;
		int isTrophy = 0;
		int skipDistance = 0;

		warppadInst = warppadThread->inst;

		switch ((u32)index)
		{
		case 0:
			color = 0x17;
			skipDistance = 1;
			break;
		case 1:
			color = 5;
			if ((gGT->timer & 2) != 0)
			{
				color = 4;
			}
			isTrophy = 1;
			break;
		case 2:
			color = 3;
			break;
		case 3:
			color = 0xe;
			break;
		case 4:
			// Each Slide Coliseum/Turbo Track color lasts two frames.
			color = ((gGT->timer >> 1) & 7) + 5;
			break;
		default:
			color = 0x15;
			skipDistance = 1;
			break;
		}

		if (isTrophy)
		{
			// get posZ in 3D, turns into posY in 2D
			posX = warppadInst->matrix.t[0];
			posY = warppadInst->matrix.t[2];

			D232.unkModeHubItems = 1;

			// Get Icon Dimensions
			UI_Map_GetIconPos(ptrMap, &posX, &posY);

			AH_Map_HubArrowOutter(ptrMap, (int)*param_3, posX, posY, 0, 0);

			*param_3 = *param_3 + 1;
		}

		UI_Map_DrawRawIcon((int)ptrMap, (int *)&warppadInst->matrix.t[0], 0x31, color, 0, 0x1000);

		if (skipDistance)
		{
			// skip distance check
			continue;
		}

		distX = warppadInst->matrix.t[0] - dMat->t[0];
		distY = warppadInst->matrix.t[1] - dMat->t[1];
		distZ = warppadInst->matrix.t[2] - dMat->t[2];

		currDistance = SquareRoot0_stub(distX * distX + distY * distY + distZ * distZ);

		if (minDistance > currDistance)
		{
			minDistance = currDistance;
			closestWarppadInst = warppadInst;
		}
	}

	// play sound from closest unlocked warppad
	if (closestWarppadInst != NULL)
	{
		PlayWarppadSound(minDistance << 1);
	}

	return;
}

#if defined(CTR_NATIVE)
force_inline void AH_MaskHint_DrawRepeatPrompt(void);
#endif

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b1c90-0x800b1ef8.
void AH_Map_Main(void)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Driver *advDriver;
	struct UiElement2D *ptrHudData;
	int iVar1;
	int hubPtrs; // int*?
	s16 local_20;
	s16 local_1e[3];

	// force disable speedometer
	sdata->HudAndDebugFlags &= 0xfffffff7;

	local_20 = 0;
	advDriver = gGT->drivers[0];
	ptrHudData = data.hudStructPtr[gGT->numPlyrCurrGame - 1];
	hubPtrs = 0;
	iVar1 = RaceFlag_GetCanDraw();
	if (iVar1 == 0)
	{
		RaceFlag_SetCanDraw(1);
	}

	if (
	    // if Aku Hint is not unlocked
	    (CHECK_ADV_BIT(sdata->advProgress.rewards, ADV_REWARD_HINT_WELCOME_TO_ARENA) == 0) &&

	    (iVar1 = RaceFlag_IsFullyOffScreen(), iVar1 != 0))
	{
		// Trigger Aku Hint:
		// Welcome to Adventure Arena
		MainFrame_RequestMaskHint(ADV_MASK_HINT_ID_WELCOME_TO_ARENA, 0);
	}


	// NOTE(aalhendi): Retail keeps this AI-only Adventure Hub speedometer fallback.
	if ((gGT->numPlyrCurrGame == 0) && ((advDriver->actionsFlagSet & ACTION_BOT) != 0))
	{
		sdata->HudAndDebugFlags = 8;
	}

	if (gGT->level1->ptrSpawnType1->count != 0)
	{
		void **pointers = ST1_GETPOINTERS(gGT->level1->ptrSpawnType1);
		hubPtrs = (int)pointers[ST1_MAP]; // cast as int*?
	}

	// if game is not paused
	if ((gGT->gameMode1 & PAUSE_ALL) == 0)
	{
		// Jump meter and landing boost
		UI_JumpMeter_Update(advDriver);
	}

	// Check a HUD flag
	if ((gGT->hudFlags & 0x10) == 0)
	{
		local_1e[0] = 0;

		D232.unkModeHubItems = 0;

		UI_Map_DrawDrivers(hubPtrs, gGT->threadBuckets[0].thread, &local_20);

		AH_Map_Warppads((s16 *)hubPtrs, gGT->threadBuckets[5].thread,
		                (s16 *)&local_1e[0]); // local_1e index 1 and 2 are never assigned to, so garbage data?

		AH_Map_HubItems((void *)hubPtrs, &local_1e[0]);

		UI_Map_DrawMap(gGT->ptrIcons[3], gGT->ptrIcons[4],

		               500, 195,

		               &gGT->backBuffer->primMem, gGT->pushBuffer_UI.ptrOT, 1);

		UI_DrawSlideMeter(ptrHudData[8].x, ptrHudData[8].y, advDriver);
	}

	UI_DrawNumRelic(ptrHudData[0xE].x + 0x10, ptrHudData[0xE].y - 10);
	UI_DrawNumKey(ptrHudData[0xF].x + 0x10, ptrHudData[0xF].y - 10);
	UI_DrawNumTrophy(ptrHudData[0x10].x + 0x10, ptrHudData[0x10].y - 10);

#if defined(CTR_NATIVE)
	// NOTE(aalhendi): Retail appends this prompt after DrawOTag starts; the PS1
	// GPU can still consume that late OT write. Native DrawOTag parses
	// synchronously, so emit only this static prompt during the hub UI pass and
	// leave AH_MaskHint_Update to run the real state/audio timing later.
	if (sdata->AkuAkuHintState == 5)
	{
		AH_MaskHint_DrawRepeatPrompt();
	}
#endif

	return;
}
