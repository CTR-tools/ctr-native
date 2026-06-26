#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b1ef8-0x800b1f78.
void AH_Pause_Destroy(void)
{
	int i;

	// global -> register
	struct PauseObject *ptrPauseObject = D232.ptrPauseObject;

	// check register
	if (ptrPauseObject == 0)
	{
		return;
	}

	// loop through 14 instances, destroy them
	for (i = 0; i < 0xe; i++)
	{
		INSTANCE_Death(ptrPauseObject->PauseMember[i].inst);
	}

	// kill thread
	D232.ptrPauseObject = 0;
	ptrPauseObject->t->flags |= THREAD_FLAG_DEAD;
}

void AH_Pause_Draw(int pageID, int posX)
{
	RECT r;
	int levelID = D232.advPausePages[pageID].hubID;
	int lngIndex = D232.advPausePages[pageID].titleLng;

	int bitIndex;
	struct AdvProgress *adv;
	adv = &sdata->advProgress;

	if (lngIndex < 0)
	{
		lngIndex = data.metaDataLEV[levelID].name_LNG;
	}

	char *str = sdata->lngStrings[lngIndex];

	DecalFont_DrawLine(str, posX + 0x100, 0xf, FONT_BIG, 0xffff8000);

	int len = DecalFont_GetLineWidth(str, FONT_BIG);

	int half = len >> 1;

	// orange/red
	int colorIndex = 0;
	if ((sdata->frameCounter & 4) == 0)
	{
		colorIndex = 3;
	}

	int *ptrColor = data.ptrColor[colorIndex];

	struct GameTracker *gGT = sdata->gGT;
	struct PrimMem *primMem = &gGT->backBuffer->primMem;

	struct Icon **iconPtrArray = ICONGROUP_GETICONS(gGT->iconGroup[4]);

	// Draw arrow pointing Left
	DecalHUD_Arrow2D(iconPtrArray[0x38], (posX - half) + 0xec, 0x16,

	                 primMem, gGT->pushBuffer_UI.ptrOT,

	                 ptrColor[0], ptrColor[1], ptrColor[2], ptrColor[3],

	                 0, 0x1000, 0x800);

	// Draw arrow pointing Right
	DecalHUD_Arrow2D(iconPtrArray[0x38], (posX + half) + 0x112, 0x16,

	                 primMem, gGT->pushBuffer_UI.ptrOT,

	                 ptrColor[0], ptrColor[1], ptrColor[2], ptrColor[3],

	                 0, 0x1000, 0);

	struct PauseObject *ptrPauseObject = D232.ptrPauseObject;

	// loop through 14 instances
	for (int i = 0; i < 0xe; i++)
	{
		// assume no awards won
		ptrPauseObject->PauseMember[i].unlockFlag &= ~(1);

		// dont draw instance
		ptrPauseObject->PauseMember[i].indexAdvPauseInst = -1;
	}

	int type = D232.advPausePages[pageID].type;

	if (type == 0)
	{
		int hubID = levelID - GEM_STONE_VALLEY;
		int rowIndex = 0;
		int pauseIndex = 0;
		int crystalID = -1;
		int textX = 0x50;
		int iconX = 0x15e;
		int rowBase = 0;

		if (hubID == 0)
		{
			textX = 0x6e;
			iconX = 0x16d;
			rowBase = 4;
		}

		for (int trackID = 0; trackID < 0x41; trackID++)
		{
			struct MetaDataLEV *mdLev = &data.metaDataLEV[trackID];

			if (mdLev->hubID != hubID)
			{
				continue;
			}

			if (trackID >= 0x12)
			{
				crystalID = trackID;
				continue;
			}

			int rowY = rowBase + rowIndex * 0x10;
			rowIndex++;

			DecalFont_DrawLine(sdata->lngStrings[mdLev->name_LNG], posX + textX, rowY + 0x26, FONT_BIG, 0);

			if (hubID != 0)
			{
				for (int j = 0; j < 3; j++)
				{
					struct Instance *inst = ptrPauseObject->PauseMember[pauseIndex + j].inst;

					// Remove SelectProfile with regular UI variant
					inst->matrix.t[0] = UI_ConvertX_2(posX + iconX + j * 0x1e, 0x100);

					inst->matrix.t[1] = UI_ConvertY_2(rowY + 0x2f, 0x100);
				}

				ptrPauseObject->PauseMember[pauseIndex].indexAdvPauseInst = 14;
				ptrPauseObject->PauseMember[pauseIndex].unlockFlag |= CHECK_ADV_BIT(adv->rewards, trackID + ADV_REWARD_FIRST_TROPHY);
				pauseIndex++;
			}
			else
			{
				struct Instance *inst = ptrPauseObject->PauseMember[pauseIndex].inst;

				// Remove SelectProfile with regular UI variant
				inst->matrix.t[0] = UI_ConvertX_2(posX + iconX + 1 * 0x1e, 0x100);

				inst->matrix.t[1] = UI_ConvertY_2(rowY + 0x2f, 0x100);
			}

			if (CHECK_ADV_BIT(adv->rewards, trackID + ADV_REWARD_FIRST_PLATINUM_RELIC) != 0)
			{
				ptrPauseObject->PauseMember[pauseIndex].unlockFlag |= 1;
				ptrPauseObject->PauseMember[pauseIndex].indexAdvPauseInst = 8;
			}
			else if (CHECK_ADV_BIT(adv->rewards, trackID + ADV_REWARD_FIRST_GOLD_RELIC) != 0)
			{
				ptrPauseObject->PauseMember[pauseIndex].unlockFlag |= 1;
				ptrPauseObject->PauseMember[pauseIndex].indexAdvPauseInst = 7;
			}
			else
			{
				ptrPauseObject->PauseMember[pauseIndex].indexAdvPauseInst = 6;
				ptrPauseObject->PauseMember[pauseIndex].unlockFlag |= CHECK_ADV_BIT(adv->rewards, trackID + ADV_REWARD_FIRST_SAPPHIRE_RELIC);
			}

			pauseIndex++;

			if (hubID != 0)
			{
				ptrPauseObject->PauseMember[pauseIndex].indexAdvPauseInst = 9 + mdLev->ctrTokenGroupID;
				ptrPauseObject->PauseMember[pauseIndex].unlockFlag |= CHECK_ADV_BIT(adv->rewards, trackID + ADV_REWARD_FIRST_CTR_TOKEN);
				pauseIndex++;
			}
		}

		int bossRowY = rowBase + rowIndex * 0x10;
		int bossID = D232.advPausePages[pageID].characterID_Boss;

		DecalFont_DrawLine(sdata->lngStrings[data.MetaDataCharacters[bossID].name_LNG_long], posX + textX, bossRowY + 0x26, FONT_BIG, 4);

		if (hubID == 0)
		{
			// === Draw Star ===

			// black
			int color = 0x15;

			// set to grey (if beaten oxide at least once)
			if (CHECK_ADV_BIT(adv->rewards, data.BeatBossPrize[0]) != 0)
			{
				color = 1;
			}

			u32 *starColor;
			starColor = data.ptrColor[color];

			struct Icon **iconPtrArray = ICONGROUP_GETICONS(gGT->iconGroup[5]);

			DecalHUD_DrawPolyGT4(iconPtrArray[0x37],

			                     posX + iconX + 0x18, bossRowY + 0x2a,

			                     &gGT->backBuffer->primMem, gGT->pushBuffer_UI.ptrOT,

			                     starColor[0], starColor[1], starColor[2], starColor[3],

			                     0, 0x1000);

			pauseIndex = rowIndex;

			for (int i = 0; i < 5; i++)
			{
				struct Instance *inst = ptrPauseObject->PauseMember[pauseIndex + i].inst;

				// Remove SelectProfile with regular UI variant
				inst->matrix.t[0] = UI_ConvertX_2(posX + 0x100 + (i - 2) * 60, 0x100);

				inst->matrix.t[1] = UI_ConvertY_2(((i & 1) << 4) | 0x6a, 0x100);

				// gem color
				ptrPauseObject->PauseMember[pauseIndex + i].indexAdvPauseInst = i;

				// unlock gem
				ptrPauseObject->PauseMember[pauseIndex + i].unlockFlag |= CHECK_ADV_BIT(adv->rewards, i + ADV_REWARD_FIRST_GEM);
			}
		}
		else
		{
			struct Instance *inst = ptrPauseObject->PauseMember[pauseIndex].inst;

			// Remove SelectProfile with regular UI variant
			inst->matrix.t[0] = UI_ConvertX_2(posX + iconX + 1 * 0x1e, 0x100);

			inst->matrix.t[1] = UI_ConvertY_2(bossRowY + 0x2f, 0x100);

			ptrPauseObject->PauseMember[pauseIndex].indexAdvPauseInst = 5;
			ptrPauseObject->PauseMember[pauseIndex].unlockFlag |= CHECK_ADV_BIT(adv->rewards, data.BeatBossPrize[hubID]);
			pauseIndex++;

			if (crystalID >= 0)
			{
				struct MetaDataLEV *mdLev = &data.metaDataLEV[crystalID];
				int crystalRowY = bossRowY + 0x10;

				DecalFont_DrawLine(sdata->lngStrings[mdLev->name_LNG], posX + textX, crystalRowY + 0x26, FONT_BIG, 1);

				inst = ptrPauseObject->PauseMember[pauseIndex].inst;

				// Remove SelectProfile with regular UI variant
				inst->matrix.t[0] = UI_ConvertX_2(posX + iconX + 1 * 0x1e, 0x100);

				inst->matrix.t[1] = UI_ConvertY_2(crystalRowY + 0x2f, 0x100);

				ptrPauseObject->PauseMember[pauseIndex].indexAdvPauseInst = 9 + mdLev->ctrTokenGroupID;
				ptrPauseObject->PauseMember[pauseIndex].unlockFlag |= CHECK_ADV_BIT(adv->rewards, hubID + ADV_REWARD_PURPLE_TOKEN_HUB_ID_BASE);
			}
		}
	}

	else if (type == 1)
	{
		s16 tokenCount[5] = {0, 0, 0, 0, 0};

		for (int i = 0; i < 0x10; i++)
		{
			if (CHECK_ADV_BIT(adv->rewards, i + ADV_REWARD_FIRST_CTR_TOKEN) != 0)
			{
				tokenCount[data.metaDataLEV[i].ctrTokenGroupID]++;
			}
		}

		// NOTE(aalhendi): Purple tokens are stored in a separate reward bit range not in ctrTokenGroupID.
		for (int i = 0; i < 4; i++)
		{
			if (CHECK_ADV_BIT(adv->rewards, i + ADV_REWARD_FIRST_PURPLE_TOKEN) != 0)
			{
				tokenCount[4]++;
			}
		}

		for (int i = 0; i < 5; i++)
		{
			s16 instPosX = posX + 0xf0 + ((i - 2) * 60);
			s16 instPosY = (i & 1) * 0x28;

			ptrPauseObject->PauseMember[i].indexAdvPauseInst = i + 9;
			ptrPauseObject->PauseMember[i].unlockFlag |= 1;

			struct Instance *inst = ptrPauseObject->PauseMember[i].inst;

			// Remove SelectProfile with regular UI variant
			inst->matrix.t[0] = UI_ConvertX_2(instPosX, 0x100);

			inst->matrix.t[1] = UI_ConvertY_2(instPosY + 0x41, 0x100);

			SelectProfile_PrintInteger(tokenCount[i], instPosX + 0x36, instPosY + 0x3a, 0, 0);

			int strX = 'X'; //"X\0\0" + nullterm
			DecalFont_DrawLine((char *)&strX, instPosX + 0x24, instPosY + 0x3e, FONT_SMALL, 0);
		}
	}

	else if (type == 2)
	{
		char totalString[32];
		s16 count[3];
		count[0] = 0;
		count[1] = 0;
		count[2] = 0;

		for (int i = 0; i < 0x12; i++)
		{
			// platinum
			if (CHECK_ADV_BIT(adv->rewards, i + ADV_REWARD_FIRST_PLATINUM_RELIC) != 0)
			{
				count[2]++;
			}
			// gold
			else if (CHECK_ADV_BIT(adv->rewards, i + ADV_REWARD_FIRST_GOLD_RELIC) != 0)
			{
				count[1]++;
			}
			// sapphire
			else if (CHECK_ADV_BIT(adv->rewards, i + ADV_REWARD_FIRST_SAPPHIRE_RELIC) != 0)
			{
				count[0]++;
			}
		}

		for (int i = 0; i < 3; i++)
		{
			s16 instPosX = posX + 0xf6 + ((i - 1) * 90);

			ptrPauseObject->PauseMember[i].indexAdvPauseInst = i + 6;
			ptrPauseObject->PauseMember[i].unlockFlag |= 1;

			struct Instance *inst = ptrPauseObject->PauseMember[i].inst;

			// Remove SelectProfile with regular UI variant
			inst->matrix.t[0] = UI_ConvertX_2(instPosX, 0x100);

			inst->matrix.t[1] = UI_ConvertY_2(0x49, 0x100);

			SelectProfile_PrintInteger(count[i], instPosX + 0x19, 0x49, 0, 0);

			int strX = 'X'; //"X\0\0" + nullterm
			DecalFont_DrawLine((char *)&strX, instPosX + 10, 0x4e, FONT_SMALL, 0);
		}

		// variable reuse
		bitIndex = count[0] + count[1] + count[2];

		sprintf(totalString, "%s %d", sdata->lngStrings[LNG_TOTAL], bitIndex);

		DecalFont_DrawLine(totalString, posX + 0x100, 0x6e, FONT_BIG, 0xffff8000);
	}

	int iVar7 = DecalFont_GetLineWidth(str, FONT_BIG);

	int iVar11 = iVar7 + 0x14;
	if ((s16)iVar7 < 0x20b)
	{
		iVar11 = 0x21e;
	}

	half = iVar11 >> 1;

	r.x = 0x10a - half;
	r.y = 0x20;
	r.w = (s16)iVar11 + -0x14;
	r.h = 2;

	Color color;
	color.self = sdata->battleSetup_Color_UI_1;
	uint32_t *ot = gGT->backBuffer->otMem.uiOT;
	RECTMENU_DrawOuterRect_Edge(&r, color, 0x20, ot);

	r.x = 0x100 - half;
	r.y = 10;
	r.w = (s16)iVar11;
	r.h = 0x82;

	// Draw 2D Menu rectangle background
	RECTMENU_DrawInnerRect(&r, 4, &ot[3]);

	for (int i = 0; i < 0xe; i++)
	{
		int index = ptrPauseObject->PauseMember[i].indexAdvPauseInst;

		struct Instance *inst = ptrPauseObject->PauseMember[i].inst;
		SVec3 *rot = &ptrPauseObject->PauseMember[i].rot;

		if (index < 0)
		{
			// make invisible
			inst->flags |= HIDE_MODEL;
		}
		else
		{
			inst->flags &= ~(HIDE_MODEL | DRAW_TRANSPARENT | USE_SPECULAR_LIGHT | DRAW_BILLBOARD);
			inst->flags |= D232.advPauseInst[index].instFlags;

			if (ptrPauseObject->PauseMember[i].unlockFlag == 0)
			{
				inst->flags &= ~(HIDE_MODEL | DRAW_TRANSPARENT | USE_SPECULAR_LIGHT | DRAW_BILLBOARD);
				inst->colorRGBA = 0;
				inst->alphaScale = 0x1000;
			}

			else
			{
				u8 *ptrColor = (u8 *)&D232.advPauseInst[index].color;

				inst->alphaScale = 0;
				inst->colorRGBA = (ptrColor[0] << 0x14) | (ptrColor[1] << 0xc) | (ptrColor[2] << 0x4);
			}

			int scale = D232.advPauseInst[index].scale;

			if (type == 1)
			{
				scale = 0x1000;
			}
			else if (type != 0)
			{
				scale = scale << 2;
			}

			inst->scale.x = scale;
			inst->scale.y = scale;
			inst->scale.z = scale;

			int modelID = D232.advPauseInst[index].modelID;

			inst->model = gGT->modelPtr[modelID];

			ConvertRotToMatrix(&inst->matrix, rot);

			if ((inst->flags & 0x70000) == USE_SPECULAR_LIGHT)
			{
				SVec3 *specArr = &D232.advPauseInst[index].lightDir;

				Vector_SpecLightSpin2D(inst, rot, specArr);
			}

			else
			{
				inst->colorRGBA = 0;
			}
		}

		rot->y = inst->matrix.t[0] * 0x10 + inst->matrix.t[1] * 0x20 + sdata->frameCounter * 0x40;

		rot->y &= 0xfff;
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b3144-0x800b344c.
void AH_Pause_Update()
{
	struct GameTracker *gGT;
	gGT = sdata->gGT;

	if (D232.ptrPauseObject == 0)
	{
		struct PauseObject *ptrPauseObject;
		ptrPauseObject = &D232.pauseObject;

		D232.pausePageTimer = 0;
		D232.pausePageCurr = gGT->levelID - GEM_STONE_VALLEY;
		gGT->advPausePage = D232.pausePageCurr;

		// 0 = size
		// 0 = no relation to param4
		// 0x300 = SmallStackPool
		// 0xd = "other" thread bucket
		struct Thread *t = PROC_BirthWithObject(SIZE_RELATIVE_POOL_BUCKET(0, NONE, SMALL, OTHER), 0, R232.s_PAUSE, 0);

		D232.ptrPauseObject = ptrPauseObject;
		ptrPauseObject->t = t;

		for (int i = 0; i < 0xe; i++)
		{
			struct Instance *inst = INSTANCE_Birth3D(gGT->modelPtr[STATIC_GEM], R232.s_pause, t);

			ptrPauseObject->PauseMember[i].indexAdvPauseInst = -1;
			ptrPauseObject->PauseMember[i].inst = inst;
			ptrPauseObject->PauseMember[i].rot.x = 0;
			ptrPauseObject->PauseMember[i].rot.y = 0;
			ptrPauseObject->PauseMember[i].rot.z = 0;

			inst->flags |= (USE_SPECULAR_LIGHT | SCREENSPACE_INSTANCE | HIDE_MODEL);

			struct InstDrawPerPlayer *idpp = INST_GETIDPP(inst);

			idpp[0].pushBuffer = &gGT->pushBuffer_UI;
			for (int j = 1; j < gGT->numPlyrCurrGame; j++)
			{
				idpp[j].pushBuffer = 0;
			}

			*(int *)&inst->matrix.m[0][0] = 0x1000;
			*(int *)&inst->matrix.m[0][2] = 0;
			*(int *)&inst->matrix.m[1][1] = 0x1000;
			*(int *)&inst->matrix.m[2][0] = 0;
			inst->matrix.m[2][2] = 0x1000;
			inst->matrix.t[2] = 0x100;
		}
	}

	int tap = sdata->buttonTapPerPlayer[0];

	if ((tap & (BTN_RIGHT | BTN_LEFT)) != 0)
	{
		if ((tap & BTN_LEFT) != 0)
		{
			D232.pausePageDir = -1;
			gGT->advPausePage += -1;

			if (gGT->advPausePage < 0)
			{
				gGT->advPausePage = 6;
			}
		}

		// assume BTN_RIGHT
		else
		{
			D232.pausePageDir = 1;
			gGT->advPausePage += 1;

			if (gGT->advPausePage > 6)
			{
				gGT->advPausePage = 0;
			}
		}

		// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b3340-0x800b3350 for adventure pause page-turn SFX.
		OtherFX_Play(0, 1);
	}

	// page is flipping
	if (D232.pausePageTimer > 0)
	{
		D232.pausePageTimer--;
	}
	// page is not flipping, flip desired
	else if (gGT->advPausePage != D232.pausePageCurr)
	{
		D232.pausePagePrev = D232.pausePageCurr;
		D232.pausePageDir_dup = D232.pausePageDir;

		D232.pausePageTimer = 8;

		D232.pausePageCurr = gGT->advPausePage;
	}

	int pageID;
	int posX;

	// second half
	if (D232.pausePageTimer < 5)
	{
		pageID = D232.pausePageCurr;
		posX = D232.pausePageTimer * D232.pausePageDir * -0x80;
	}

	// first half
	else
	{
		pageID = D232.pausePagePrev;
		posX = (8 - D232.pausePageTimer) * D232.pausePageDir * 0x80;
	}

	AH_Pause_Draw(pageID, posX);
}
