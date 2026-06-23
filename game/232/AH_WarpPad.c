#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800abafc-0x800abbdc.
s16 *AH_WarpPad_GetSpawnPosRot(s16 *posData)
{
	struct Thread *t;
	struct GameTracker *gGT;
	struct Instance *inst;
	struct InstDef *instDef;

	gGT = sdata->gGT;
	t = gGT->threadBuckets[WARPPAD].thread;

	// check all warppads
	while (1)
	{
		// if can't find a warppad
		if (t == 0)
		{
			// quit
			return 0;
		}

		// if warppad found that matches level exited
		if (((struct WarpPad *)t->object)->levelID == gGT->prevLEV)
		{
			// end loop
			break;
		}

		t = t->siblingThread;
	}

	inst = t->inst;
	instDef = inst->instDef;

	posData[0] = inst->matrix.t[0] + ((MATH_Cos(instDef->rot.y) << 0xA) >> 0xC);

	posData[1] = inst->matrix.t[1];

	posData[2] = inst->matrix.t[2] + ((MATH_Sin(instDef->rot.y) * -0x400) >> 0xC);

	return &instDef->rot.x;
}

CTR_STATIC_ASSERT_LAYOUT(sizeof(struct WarpPad) == 0x78);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct WarpPad, lightDirGem) == 0x50);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct WarpPad, digit10s) == 0x68);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct WarpPad, levelID) == 0x6c);

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800abbdc-0x800abd80.
void AH_WarpPad_AllWarppadNum()
{
	struct WarpPad *wp;
	struct ModelHeader *mh;
	struct Instance *inst;

	struct Thread *t = sdata->gGT->threadBuckets[WARPPAD].thread;

	for (t; t != 0; t = t->siblingThread)
	{
		wp = t->object;

		// DCxDemo says:
		// 1 to 8 is taken from mpk i guess, 0, 9 and X are seprate models

		if ((wp->inst[2] != 0) && (wp->digit1s != 0) && (wp->digit1s != 9))
		{
			inst = wp->inst[2];
			mh = &inst->model->headers[0];
			AH_WarpPad_SetNumModelData(inst, &mh[wp->digit1s - 1]);
		}

		if ((wp->inst[3] != 0) && (wp->digit10s != 0))
		{
			inst = wp->inst[3];
			mh = &inst->model->headers[0];
			AH_WarpPad_SetNumModelData(inst, mh);
		}
	}
}

void AH_WarpPad_SetNumModelData(struct Instance *inst, struct ModelHeader *mh)
{
	struct InstDrawPerPlayer *idpp = INST_GETIDPP(inst);

	idpp[0].ptrCommandList = mh->ptrCommandList;
	idpp[0].ptrColorLayout = (u32)mh->ptrColors;
	idpp[0].ptrTexLayout = mh->ptrTexLayout;
	idpp[0].ptrCurrFrame = mh->ptrFrameData;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800abd80-0x800abdfc.
void AH_WarpPad_MenuProc(struct RectMenu *menu)
{
	struct GameTracker *gGT = sdata->gGT;

	RECTMENU_Hide(menu);

	if (menu->rowSelected == 0)
	{
		gGT->gameMode2 |= TOKEN_RACE;
	}

	else if (menu->rowSelected == 1)
	{
		gGT->gameMode1 |= RELIC_RACE;
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800abdfc-0x800abf48.
void AH_WarpPad_SpinRewards(struct Instance *prizeInst, struct WarpPad *warppadObj, int index, int x, int y, int z)
{
	SVec3 *lightDir;
	u32 modelID;
	u32 trig;
	u32 thirds;

	ConvertRotToMatrix(&prizeInst->matrix, &warppadObj->spinRot_Prize);

	modelID = prizeInst->model->id;

	if (modelID != STATIC_TROPHY) // if not trophy (no lightDir on trophy)
	{
		if (modelID == STATIC_GEM) // gem
			lightDir = &warppadObj->lightDirGem;
		else
		{
			if (modelID == STATIC_RELIC) // relic
				lightDir = &warppadObj->lightDirRelic;
			else
			{
				if (modelID == STATIC_TOKEN) // token
					lightDir = &warppadObj->lightDirToken;
				else
					goto SpinReward;
			}
		}
		Vector_SpecLightSpin3D(prizeInst, &warppadObj->spinRot_Prize.x, lightDir);
	}

SpinReward:

	// initialized as 0x555*index, but not const
	thirds = warppadObj->thirds[index];

	trig = MATH_Sin(thirds);
	prizeInst->matrix.t[1] = y + ((trig << 6) >> 0xc) + 0x100;

	// do not use original "thirds",
	// set new value without "+="
	thirds = 0x555 * index + warppadObj->spinRot_Rewards.y;

	trig = MATH_Sin(thirds);
	prizeInst->matrix.t[0] = x + (trig * 0xA0 >> 0xc);

	trig = MATH_Cos(thirds);
	prizeInst->matrix.t[2] = z + (trig * 0xA0 >> 0xc);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800abf48-0x800ad2c8.
void AH_WarpPad_ThTick(struct Thread *t)
{
	int i;
	int j;
	b32 boolOpen;
	struct GameTracker *gGT;
	struct WarpPad *warppadObj;
	struct Instance *warppadInst;
	struct Instance **visInstSrc;
	struct Instance **instArr;

	struct Driver *driver;
	struct Instance *driverInst;

	int modelID;
	int levelID;
	int x, y, z, dist;
	char *warppadLNG;

	int angleCamToWarppad;
	int angleSin, angleCos;
	MATRIX *warppadMatrix;

	int wispMaxHeight;
	int wispRiseRate;
	int rng1;
	int rng2;

	int rewardScale;
	int rewardScale2;

	int champID;
	int champSlot;

	char randKartSpawn[8];

	// NOTE(aalhendi): WarpPad level IDs come from "warppad#NN" instance names
	// and use retail adventure numbering, not the native LevelID enum.
	enum
	{
		AH_WP_SLIDE_COLISEUM = 16,
		AH_WP_TURBO_TRACK = 17,
		AH_WP_NITRO_COURT = 18,
		AH_WP_GEM_STONE_VALLEY = 25,
		AH_WP_ADV_CUP = 100,
	};

	boolOpen = false;
	gGT = sdata->gGT;
	warppadObj = t->object;
	warppadInst = t->inst;
	visInstSrc = gGT->cameraDC[0].visInstSrc;

#if defined(CTR_NATIVE)
	// NOTE(aalhendi): Retail can read PS1 low RAM when the hub-swap frame
	// leaves this list null; native treats that as an empty visible-instance list.
	if (visInstSrc != NULL)
#endif
	{
		while (visInstSrc[0] != 0)
		{
			if (visInstSrc[0] == warppadInst)
			{
				boolOpen = true;
				break;
			}

			visInstSrc++;
		}
	}

	// array of instances in warppad object
	instArr = &warppadObj->inst[0];
	warppadMatrix = &warppadInst->matrix;

	// make instances visible
	if (boolOpen)
	{
		for (i = 0; i < WPIS_NUM_INSTANCES; i++)
			if (instArr[i] != 0)
				instArr[i]->flags &= ~(0x80);
	}

	// make instances invisible
	else
	{
		for (i = 0; i < WPIS_NUM_INSTANCES; i++)
			if (instArr[i] != 0)
				instArr[i]->flags |= 0x80;
	}

	warppadInst->flags |= HIDE_MODEL;

	driver = gGT->drivers[0];
	driverInst = driver->instSelf;

	x = warppadMatrix->t[0] - driverInst->matrix.t[0];
	y = warppadMatrix->t[1] - driverInst->matrix.t[1];
	z = warppadMatrix->t[2] - driverInst->matrix.t[2];
	dist = x * x + y * y + z * z;

	levelID = warppadObj->levelID;

	// if near a portal
	if (
	    // Trophy tracks (-16)
	    ((levelID < AH_WP_SLIDE_COLISEUM) && (dist < 0x144000)) ||

	    // Slide Col + Turbo Track (-16)
	    ((((u16)(levelID - AH_WP_SLIDE_COLISEUM)) < 2) && (dist < 0x90000)) ||

	    // Battle tracks (-18)
	    ((((u16)(levelID - AH_WP_NITRO_COURT)) < 7) && (dist < 0x144000)) ||

	    // Gem cups
	    ((levelID >= AH_WP_ADV_CUP) && (dist < 0x90000)))
	{
		// if you are near a new warppad, or if you already were
		// determined as near the same warppad in the last frame,
		// then use this warppad as the "closest". Otherwise the
		// game could run this for two warppads right next to each other
		if ((D232.levelID == -1) || (D232.levelID == levelID))
		{
			// saved as nearest warppad
			D232.levelID = levelID;


			// if not giving Aku Hint
			if (sdata->AkuAkuHintState == 0)
			{
				// default
				if (levelID < AH_WP_ADV_CUP)
					warppadLNG = sdata->lngStrings[data.metaDataLEV[levelID].name_LNG];

				// gem cups
				else
					warppadLNG = sdata->lngStrings[data.AdvCups[levelID - AH_WP_ADV_CUP].lngIndex_CupName];

				// midpoing X,
				// 30 pixels above botttom Y
				DecalFont_DrawLine(warppadLNG, gGT->pushBuffer[0].rect.x + gGT->pushBuffer[0].rect.w / 2,
				                   gGT->pushBuffer[0].rect.y + gGT->pushBuffer[0].rect.h - 30, FONT_BIG, (JUSTIFY_CENTER | ORANGE));
			}

			// if track is unlocked, ignore all other ELSE-IFs
			if (instArr[WPIS_CLOSED_1S] == 0)
			{
			}

			else if (

			    // gem cup
			    (levelID >= AH_WP_ADV_CUP) &&

			    // Dont have hint "you must have 4 tokens for a gem"
			    (CHECK_ADV_BIT(sdata->advProgress.rewards, ADV_REWARD_HINT_GEM_CUPS_CHALLENGE) == 0)

			)
			{
				// give hint "you must have 4 tokens for a gem"
				MainFrame_RequestMaskHint(ADV_MASK_HINT_ID_GEM_CUPS_CHALLENGE, 0);
			}

			else if (

			    // Trophy track
			    (levelID < AH_WP_SLIDE_COLISEUM) &&

			    // Dont have hint "you must have more trophies"
			    (CHECK_ADV_BIT(sdata->advProgress.rewards, ADV_REWARD_HINT_NEED_MORE_TROPHIES) == 0) &&

			    // required item is not KEY
			    (instArr[WPIS_CLOSED_ITEM]->model->id != STATIC_KEY))
			{
				// give hint for "need more trophies"
				MainFrame_RequestMaskHint(ADV_MASK_HINT_ID_NEED_MORE_TROPHIES, 0);
			}

			else if (

			    // Slide Col
			    (levelID == AH_WP_SLIDE_COLISEUM) &&

			    // Dont have hint "you must have 10 relics"
			    (CHECK_ADV_BIT(sdata->advProgress.rewards, ADV_REWARD_HINT_MUST_GET_10_RELICS) == 0))
			{
				// give hint for "need more trophies"
				MainFrame_RequestMaskHint(ADV_MASK_HINT_ID_MUST_GET_10_RELICS, 0);
			}
		}
	}

	// not near portal
	else
	{
		D232.levelID = -1;
	}

	// if warppad is locked
	if (instArr[WPIS_CLOSED_1S] != 0)
	{
		angleCamToWarppad = ratan2(warppadMatrix->t[0] - gGT->pushBuffer[0].pos.x, warppadMatrix->t[2] - gGT->pushBuffer[0].pos.z);

		angleCamToWarppad = -angleCamToWarppad;

		angleSin = MATH_Sin(angleCamToWarppad);
		angleCos = MATH_Cos(angleCamToWarppad);

		// no 10s digit
		if (instArr[WPIS_CLOSED_10S] == 0)
		{
			instArr[WPIS_CLOSED_1S]->matrix.t[0] = warppadMatrix->t[0] + (angleCos * -0x80 >> 0xC);
			instArr[WPIS_CLOSED_1S]->matrix.t[2] = warppadMatrix->t[2] + (angleSin * -0x80 >> 0xC);

			instArr[WPIS_CLOSED_ITEM]->matrix.t[0] = warppadMatrix->t[0] + ((angleCos << 7) >> 0xC);
			instArr[WPIS_CLOSED_ITEM]->matrix.t[2] = warppadMatrix->t[2] + ((angleSin << 7) >> 0xC);
		}

		// 10s digit
		else
		{
			instArr[WPIS_CLOSED_ITEM]->matrix.t[0] = warppadMatrix->t[0] + (angleCos * 0xC0 >> 0xC);
			instArr[WPIS_CLOSED_ITEM]->matrix.t[2] = warppadMatrix->t[2] + (angleSin * 0xC0 >> 0xC);

			instArr[WPIS_CLOSED_X]->matrix.t[0] = warppadMatrix->t[0] + ((angleCos << 6) >> 0xC);
			instArr[WPIS_CLOSED_X]->matrix.t[2] = warppadMatrix->t[2] + ((angleSin << 6) >> 0xC);

			instArr[WPIS_CLOSED_10S]->matrix.t[0] = warppadMatrix->t[0] + (angleCos * -0x40 >> 0xC);
			instArr[WPIS_CLOSED_10S]->matrix.t[2] = warppadMatrix->t[2] + (angleSin * -0x40 >> 0xC);

			instArr[WPIS_CLOSED_1S]->matrix.t[0] = warppadMatrix->t[0] + (angleCos * -0xa0 >> 0xC);
			instArr[WPIS_CLOSED_1S]->matrix.t[2] = warppadMatrix->t[2] + (angleSin * -0xa0 >> 0xC);
		}

		warppadObj->spinRot_Prize.x = 0;
		warppadObj->spinRot_Prize.z = 0;

		warppadObj->spinRot_Prize.y += 0x40;

		// reuse variable,
		// end of function anyway
		warppadInst = instArr[WPIS_CLOSED_ITEM];
#define InstArr0 warppadInst

		// converted to TEST in rebuildPS1
		ConvertRotToMatrix(&InstArr0->matrix, &warppadObj->spinRot_Prize);

		modelID = InstArr0->model->id;

		// Trophy has no specular light
		if (modelID == STATIC_TROPHY)
			return;

		// NOTE(aalhendi): Retail passes the per-WarpPad spec-light arrays at
		// offsets 0x50/0x58/0x60.

		// Relic
		if (modelID == STATIC_RELIC)
		{
			Vector_SpecLightSpin3D(InstArr0, &warppadObj->spinRot_Prize.x, &warppadObj->lightDirRelic);
			return;
		}

		// Token
		if (modelID == STATIC_TOKEN)
		{
			Vector_SpecLightSpin3D(InstArr0, &warppadObj->spinRot_Prize.x, &warppadObj->lightDirToken);
			return;
		}

		// If Gem, change colors every 2 seconds
		if (modelID == STATIC_GEM)
		{
			i = (gGT->timer / 0x3C) % 5;

			InstArr0->colorRGBA = ((u32)data.AdvCups[i].color[0] << 0x14) | ((u32)data.AdvCups[i].color[1] << 0xc) | ((u32)data.AdvCups[i].color[2] << 0x4);
		}

		// for Key or Gem
		Vector_SpecLightSpin3D(InstArr0, &warppadObj->spinRot_Prize.x, &warppadObj->lightDirGem);
		return;
	}

	// === Assume Unlocked ===

	if ((dist > 0x8fff) && (warppadObj->boolEnteredWarppad == 0))
		goto WarpPad_AnimateOpen;

	// Retail repeats this setup every close/warping frame before the
	// transition/load gate.
	LOAD_Robots1P(data.characterIDs[0]);

	// variable reuse, get track speed champion
	champID = data.metaDataLEV[levelID].characterID_Champion;

	// default
	champSlot = 0;

	// If Speed Champion is on the track (Crash-Pura)
	// and is not the same characterID as this driver
	if ((champID < 8) && (champID != data.characterIDs[driver->driverID]))
	{
		// set everyone to spawn in order
		for (i = 1; i < 8; i++)
		{
			if (champID == data.characterIDs[i])
			{
				sdata->kartSpawnOrderArray[i] = 0;
				champSlot = i;
			}

			else if (i == 7)
			{
				sdata->kartSpawnOrderArray[7] = champSlot;
			}

			else
			{
				sdata->kartSpawnOrderArray[i] = i;
			}
		}
	}

	// Speed Champion is invalid
	else
	{
		for (i = 1; i < 8; i++)
			randKartSpawn[i] = i;

		for (i = 0; i < 7; i++)
		{
			rng1 = RngDeadCoed(&sdata->const_0x30215400);

			rng2 = 7 - i;

			rng2 = (rng1 & 0xfff) % rng2 + 1;
			rng2 = (s16)rng2;

			sdata->kartSpawnOrderArray[randKartSpawn[rng2]] = (char)i;

			while (rng2 < 7)
			{
				randKartSpawn[rng2] = randKartSpawn[rng2 + 1];
				rng2++;
			}
		}
	}

	// spawn P1 in the back
	sdata->kartSpawnOrderArray[0] = 7;

	// if flag is on-screen, loading has already been finalized
	if (RaceFlag_IsTransitioning() != 0)
		goto WarpPad_AnimateOpen;

	levelID = warppadObj->levelID;

	// gem cups
	if (levelID >= AH_WP_ADV_CUP)
	{
		warppadObj->boolEnteredWarppad = 1;
		warppadObj->framesWarping++;
		gGT->drivers[0]->funcPtrs[DRIVER_FUNC_INIT] = VehStuckProc_Warp_Init;
		if (warppadObj->framesWarping < 61)
			goto WarpPad_AnimateOpen;

		sdata->Loading.OnBegin.AddBitsConfig0 |= ADVENTURE_CUP;

		gGT->cup.cupID = levelID - AH_WP_ADV_CUP;
		gGT->cup.trackIndex = 0;
		for (i = 0; i < 8; i++)
			gGT->cup.points[i] = 0;

		levelID = data.advCupTrackIDs[4 * gGT->cup.cupID];
		goto WarpPad_RequestLoad;
	}

	// Slide Col or Turbo Track
	if (((u16)(levelID - AH_WP_SLIDE_COLISEUM)) < 2)
	{
		warppadObj->boolEnteredWarppad = 1;
		warppadObj->framesWarping++;
		gGT->drivers[0]->funcPtrs[DRIVER_FUNC_INIT] = VehStuckProc_Warp_Init;
		if (warppadObj->framesWarping < 61)
			goto WarpPad_AnimateOpen;

		sdata->Loading.OnBegin.AddBitsConfig0 |= RELIC_RACE;
		goto WarpPad_RequestLoad;
	}

	// Battle Tracks
	if ((((u16)(levelID - AH_WP_NITRO_COURT)) < 2) || (levelID == 21) || (levelID == 23))
	{
		warppadObj->boolEnteredWarppad = 1;
		warppadObj->framesWarping++;
		gGT->drivers[0]->funcPtrs[DRIVER_FUNC_INIT] = VehStuckProc_Warp_Init;
		if (warppadObj->framesWarping < 61)
			goto WarpPad_AnimateOpen;

		sdata->Loading.OnBegin.AddBitsConfig0 |= CRYSTAL_CHALLENGE;

		// Dont have hint "collect every crystal"
		if (CHECK_ADV_BIT(sdata->advProgress.rewards, ADV_REWARD_HINT_COLLECT_EVERY_CRYSTAL) == 0)
			MainFrame_RequestMaskHint(ADV_MASK_HINT_ID_COLLECT_EVERY_CRYSTAL, 1);

		// if can't spawn aku cause he's already here,
		// quit function, wait till he's done to start race
		i = AH_MaskHint_boolCanSpawn();
		if ((i & 0xffff) == 0)
			goto WarpPad_AnimateOpen;

		gGT->originalEventTime = D232.timeCrystalChallenge[levelID - AH_WP_NITRO_COURT];
		goto WarpPad_RequestLoad;
	}

	if (levelID < AH_WP_SLIDE_COLISEUM)
	{
		if (CHECK_ADV_BIT(sdata->advProgress.rewards, levelID + ADV_REWARD_FIRST_TROPHY) != 0)
		{
			if (gGT->currAdvProfile.numTrophies >= data.metaDataLEV[levelID].numTrophiesToOpen)
			{
				if (warppadObj->framesWarping < 61)
					goto WarpPad_TrophyAnimateOnly;

				// if never opened
				if (sdata->boolOpenTokenRelicMenu == 0)
				{
					if ((gGT->gameMode1 & ADVENTURE_ARENA) != 0)
					{
						D232.menuTokenRelic.rowSelected = (CHECK_ADV_BIT(sdata->advProgress.rewards, levelID + ADV_REWARD_FIRST_CTR_TOKEN) != 0);

						RECTMENU_Show(&D232.menuTokenRelic);

						// now opened
						sdata->boolOpenTokenRelicMenu = 1;
					}
				}

				// if opened, but not closed yet
				if ((RECTMENU_BoolHidden(&D232.menuTokenRelic) & 0xffff) == 0)
					goto WarpPad_TrophyAnimateOnly;

				// Relic Hint
				i = ADV_MASK_HINT_ID_RELIC_CHALLENGE;

				// CTR Token Hint
				if ((gGT->gameMode2 & 8) != 0)
					i = ADV_MASK_HINT_ID_CTR_TOKEN_CHALLENGE;

				// if hint is locked
				if (CHECK_ADV_BIT(sdata->advProgress.rewards, ADV_REWARD_FIRST_HINT + i) == 0)
					MainFrame_RequestMaskHint(i, 1);

				// if can't spawn aku cause he's already here,
				// quit function, wait till he's done to start race
				i = AH_MaskHint_boolCanSpawn();
				if ((i & 0xffff) == 0)
					goto WarpPad_TrophyAnimateOnly;

				// reset for future gameplay
				sdata->boolOpenTokenRelicMenu = 0;
				warppadObj->boolEnteredWarppad = 0;

				// Rem Adventure Arena
				sdata->Loading.OnBegin.RemBitsConfig0 |= ADVENTURE_ARENA;

				MainRaceTrack_RequestLoad(levelID);
				goto WarpPad_TrophyAnimateOnly;
			}
		}
	}

	if (CHECK_ADV_BIT(sdata->advProgress.rewards, levelID + ADV_REWARD_FIRST_TROPHY) != 0)
	{
		i = data.metaDataLEV[levelID].hubID + ADV_REWARD_BOSS_KEY_HUB_ID_BASE;

		if (CHECK_ADV_BIT(sdata->advProgress.rewards, i) == 0)
			goto WarpPad_AnimateOpen;
	}

	warppadObj->boolEnteredWarppad = 1;
	warppadObj->framesWarping++;
	gGT->drivers[0]->funcPtrs[DRIVER_FUNC_INIT] = VehStuckProc_Warp_Init;
	if (warppadObj->framesWarping < 61)
		goto WarpPad_AnimateOpen;

WarpPad_RequestLoad:

	// Rem Adventure Arena
	sdata->Loading.OnBegin.RemBitsConfig0 |= ADVENTURE_ARENA;

	MainRaceTrack_RequestLoad(levelID);
	goto WarpPad_AnimateOpen;

WarpPad_TrophyAnimateOnly:

	if (warppadObj->framesWarping < 0x400)
		warppadObj->framesWarping++;

	warppadObj->boolEnteredWarppad = 1;

	gGT->drivers[0]->funcPtrs[DRIVER_FUNC_INIT] = VehStuckProc_Warp_Init;

WarpPad_AnimateOpen:

	if ((instArr[WPIS_OPEN_BEAM] != 0) && ((gGT->timer & 1) != 0))
	{
		warppadObj->spinRot_Beam.x = 0;
		warppadObj->spinRot_Beam.z = 0;

		// what on earth was this RNG?
		// how'd they come up with something so random, that looks so good?
		i = MixRNG_Scramble();
		warppadObj->spinRot_Beam.y += ((s16)(i >> 3) + (s16)((i >> 3) / 6) * -6 + 1) * 0x200;

		// converted to TEST in rebuildPS1
		ConvertRotToMatrix(&instArr[WPIS_OPEN_BEAM]->matrix, &warppadObj->spinRot_Beam);
	}

	wispRiseRate = 0x20;

	wispMaxHeight = 0x600;

	// if close to this warppad
	if (D232.levelID != -1)
		wispMaxHeight = 0x400;

	for (i = 0; i < 2; i++)
	{
		if (instArr[WPIS_OPEN_RING1 + i] != 0)
		{
			warppadObj->spinRot_Wisp[i].x = 0;
			warppadObj->spinRot_Wisp[i].z = 0;

			warppadObj->spinRot_Wisp[i].y += 0x100;

			// converted to TEST in rebuildPS1
			ConvertRotToMatrix(&instArr[WPIS_OPEN_RING1 + i]->matrix, &warppadObj->spinRot_Wisp[i]);

			// if height hasn't reached max height
			if (instArr[WPIS_OPEN_RING1 + i]->matrix.t[1] < (warppadInst->matrix.t[1] + wispMaxHeight))
			{
				instArr[WPIS_OPEN_RING1 + i]->matrix.t[1] += wispRiseRate;

				// if height hasn't reached 4x RiseRate,
				// first 4 frames of rising
				if (instArr[WPIS_OPEN_RING1 + i]->matrix.t[1] < (warppadInst->matrix.t[1] + wispRiseRate * 4))
				{
					// reduce transparency
					instArr[WPIS_OPEN_RING1 + i]->alphaScale -= 0x380;
				}

				// after first 4 frames
				else
				{
					// add transparency as the wisp spirals upward (~0x60  per frame)
					instArr[WPIS_OPEN_RING1 + i]->alphaScale += 0xc00 / (wispMaxHeight / wispRiseRate);
				}
			}

			// eached max height
			else
			{
				// reset height
				instArr[WPIS_OPEN_RING1 + i]->matrix.t[1] = warppadInst->matrix.t[1];

				// full transparency
				instArr[WPIS_OPEN_RING1 + i]->alphaScale = 0x1000;

				rng1 = MixRNG_Scramble() >> 3;

				rng2 = rng1;
				if (rng1 < 0)
					rng2 = rng1 + 0xfff;

				warppadObj->spinRot_Wisp[i].y = (s16)rng1 + (s16)(rng2 >> 0xc) * -0x1000;
			}
		}

		wispRiseRate += 0x10;
	}

	warppadObj->spinRot_Prize.y += 0x80;

	rewardScale = 0x100;

	if (dist > 0x900000 * 2)
	{
		rewardScale = 0;
	}

	else if (dist > 0x900000)
	{
		// range [90, 90*2] to [0%, 100%]
		rewardScale = ((((0x900000 * 2) - dist) * 0x100) / 0x900000);
	}

	for (i = 0; i < 3; i++)
	{
		warppadObj->spinRot_Prize.z = 0x155;

		if (instArr[WPIS_OPEN_PRIZE1 + i] != 0)
		{
			AH_WarpPad_SpinRewards(instArr[WPIS_OPEN_PRIZE1 + i], warppadObj, i, warppadInst->matrix.t[0], warppadInst->matrix.t[1], warppadInst->matrix.t[2]);

			modelID = instArr[WPIS_OPEN_PRIZE1 + i]->model->id;

			if (rewardScale == 0)
			{
				// invisible
				instArr[WPIS_OPEN_PRIZE1 + i]->flags |= 0x80;
			}

			else
			{
				// visible
				instArr[WPIS_OPEN_PRIZE1 + i]->flags &= ~(0x80);

				// token
				rewardScale2 = 0x2000;

				// not token
				if (modelID != STATIC_TOKEN)
				{
					// trophy
					rewardScale2 = 0x2800;

					// relic
					if (modelID == STATIC_RELIC)
					{
						rewardScale2 = 0x1800;
					}
				}

				rewardScale2 = (u32)(rewardScale2 * rewardScale) >> 8;
				instArr[WPIS_OPEN_PRIZE1 + i]->scale.x = (s16)rewardScale2;
				instArr[WPIS_OPEN_PRIZE1 + i]->scale.y = (s16)rewardScale2;
				instArr[WPIS_OPEN_PRIZE1 + i]->scale.z = (s16)rewardScale2;
			}
		}

		warppadObj->thirds[i] += 0x20;
		warppadObj->spinRot_Rewards.y += 0x4;
	}

	if (instArr[WPIS_CLOSED_1S] != 0)
	{
		INSTANCE_Death(instArr[WPIS_CLOSED_1S]);
		INSTANCE_Death(instArr[WPIS_CLOSED_10S]);
		INSTANCE_Death(instArr[WPIS_CLOSED_X]);
		INSTANCE_Death(instArr[WPIS_CLOSED_ITEM]);
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800ad2c8-0x800ad3ec.
void AH_WarpPad_ThDestroy(struct Thread *t)
{
	int i;
	struct Instance **instArr;
	struct WarpPad *warppadObj;

	warppadObj = t->object;

	// array of instances in warppad object
	instArr = &warppadObj->inst[0];

	if (instArr[WPIS_CLOSED_1S] != 0)
	{
		INSTANCE_Death(instArr[WPIS_CLOSED_1S]);
		instArr[WPIS_CLOSED_1S] = 0;
	}

	if (instArr[WPIS_CLOSED_10S] != 0)
	{
		INSTANCE_Death(instArr[WPIS_CLOSED_10S]);
		instArr[WPIS_CLOSED_10S] = 0;
	}

	if (instArr[WPIS_CLOSED_X] != 0)
	{
		INSTANCE_Death(instArr[WPIS_CLOSED_X]);
		instArr[WPIS_CLOSED_X] = 0;
	}

	if (instArr[WPIS_CLOSED_ITEM] != 0)
	{
		INSTANCE_Death(instArr[WPIS_CLOSED_ITEM]);
		instArr[WPIS_CLOSED_ITEM] = 0;
	}

	if (instArr[WPIS_OPEN_BEAM] != 0)
	{
		INSTANCE_Death(instArr[WPIS_OPEN_BEAM]);
		instArr[WPIS_OPEN_BEAM] = 0;
	}

	for (i = WPIS_OPEN_RING1; i < WPIS_OPEN_PRIZE1; i++)
	{
		if (instArr[i] != 0)
		{
			INSTANCE_Death(instArr[i]);
			instArr[i] = 0;
		}
	}

	for (i = WPIS_OPEN_PRIZE1; i < WPIS_NUM_INSTANCES; i++)
	{
		if (instArr[i] != 0)
		{
			INSTANCE_Death(instArr[i]);
			instArr[i] = 0;
		}
	}
}

static const s16 s_warpPadRewardModelIDs[3] = {STATIC_TROPHY, STATIC_RELIC, STATIC_TOKEN};

// NOTE(aalhendi): Source-backed for NTSC-U 926 0x800ad3ec-0x800ae870.
void AH_WarpPad_LInB(struct Instance *inst)
{
	int i;
	int levelID;
	struct Thread *t;
	struct WarpPad *warppadObj;

	struct GameTracker *gGT;

	int unlockItem_numOwned;
	int unlockItem_numNeeded;
	int unlockItem_modelID;
	int rewardModelID;
	int rewardAngle;
	int tokenGroupID;

	int *arrTokenCount;
	struct Instance *newInst;

	// NOTE(aalhendi): WarpPad level IDs come from "warppad#NN" instance names
	// and use retail adventure numbering, not the native LevelID enum.
	enum
	{
		AH_WP_SLIDE_COLISEUM = 16,
		AH_WP_TURBO_TRACK = 17,
		AH_WP_NITRO_COURT = 18,
		AH_WP_ADV_CUP = 100,
	};

	gGT = sdata->gGT;

	if (inst->thread != NULL)
		return;

	t = PROC_BirthWithObject(SIZE_RELATIVE_POOL_BUCKET(sizeof(struct WarpPad), NONE, MEDIUM, WARPPAD),

	                         AH_WarpPad_ThTick, // behavior
	                         "warppad",         // debug name
	                         0                  // thread relative
	);

	if (t == 0)
		return;
	inst->thread = t;
	t->inst = inst;

	t->funcThDestroy = AH_WarpPad_ThDestroy;

	// 0 - locked
	// 1 - open for trophy
	// 2 - unlocked all
	// 3 - open for relic/token
	// 4 - purple token or SlideCol/TurboTrack

	// locked
	t->modelIndex = 0;

	// make invisible
	// this is the red triangle
	// instance from DCxDemo's LEV Viewer
	inst->flags |= HIDE_MODEL;

	warppadObj = t->object;
	warppadObj->levelID = 0; // this is dingo canyon
	warppadObj->boolEnteredWarppad = 0;
	warppadObj->framesWarping = 0;

	for (i = 0; i < WPIS_NUM_INSTANCES; i++)
		warppadObj->inst[i] = 0;

	// each warppad has a name "warppad#xxx"
	// "warppad#0" is dingo canyon, level ID 0
	// "warppad#16" is slide col, level ID 16
	// "warppad#102" is gem cup 2
	// "warppad#104" is gem cup 4
	// etc

	levelID = 0;
	for (i = 8; inst->name[i] != 0; i++)
	{
		levelID = levelID * 10 + inst->name[i] - '0';
	}

	warppadObj->levelID = levelID;

	unlockItem_modelID = 0;
	unlockItem_numOwned = 0;
	unlockItem_numNeeded = -1;

	// Trophy Track
	if (levelID < AH_WP_SLIDE_COLISEUM)
	{
		// optimization idea:
		// instead of data.metaDataLEV[levelID].hubID
		// can we just do gGT->levelID-0x19?

		// if trophy owned
		if (CHECK_ADV_BIT(sdata->advProgress.rewards, levelID + ADV_REWARD_FIRST_TROPHY) != 0)
		{
		GetKeysRequirement:

			// keys needed to unlock track again
			unlockItem_modelID = STATIC_KEY;
			unlockItem_numOwned = gGT->currAdvProfile.numKeys;
			unlockItem_numNeeded = D232.arrKeysNeeded[data.metaDataLEV[levelID].hubID];
		}

		// if trophy not owned
		else
		{
			// number trophies needed to open
			unlockItem_modelID = STATIC_TROPHY;
			unlockItem_numOwned = gGT->currAdvProfile.numTrophies;
			unlockItem_numNeeded = data.metaDataLEV[levelID].numTrophiesToOpen;
		}
	}

	// Slide Col
	else if (levelID == AH_WP_SLIDE_COLISEUM)
	{
		// number relics needed to open
		unlockItem_modelID = STATIC_RELIC;
		unlockItem_numOwned = gGT->currAdvProfile.numRelics;
		unlockItem_numNeeded = 10;
	}

	// Turbo Track
	else if (levelID == AH_WP_TURBO_TRACK)
	{
		// number gems needed to open
		unlockItem_modelID = STATIC_GEM;
		unlockItem_numNeeded = 5;

		// count number of gems owned
		unlockItem_numOwned = 0;
		for (i = 0; i < 5; i++)
			if (CHECK_ADV_BIT(sdata->advProgress.rewards, ADV_REWARD_FIRST_GEM + i) != 0)
				unlockItem_numOwned++;
	}

	// battle maps
	else if ((((u16)(levelID - AH_WP_NITRO_COURT)) < 2) || (levelID == 21) || (levelID == 23))
	{
		goto GetKeysRequirement;
	}

	// gem cups
	else if (((u16)(levelID - AH_WP_ADV_CUP)) < 5)
	{
		// number tokens needed to open
		unlockItem_modelID = STATIC_TOKEN;
		unlockItem_numNeeded = 4;

		arrTokenCount = &gGT->currAdvProfile.numCtrTokens.red;
		unlockItem_numOwned = arrTokenCount[levelID - AH_WP_ADV_CUP];
	}

	// if unlocked
	if (unlockItem_numOwned >= unlockItem_numNeeded)
	{
		warppadObj->digit1s = 0;
		t->modelIndex = 1;

		// if beam model exists
		if (gGT->modelPtr[STATIC_BEAM] != 0)
		{
			newInst = INSTANCE_Birth3D(gGT->modelPtr[STATIC_BEAM], "beam", t);

			// copy matrix
			*(int *)((uintptr_t)&newInst->matrix + 0x0) = *(int *)((uintptr_t)&inst->matrix + 0x0);
			*(int *)((uintptr_t)&newInst->matrix + 0x4) = *(int *)((uintptr_t)&inst->matrix + 0x4);
			*(int *)((uintptr_t)&newInst->matrix + 0x8) = *(int *)((uintptr_t)&inst->matrix + 0x8);
			*(int *)((uintptr_t)&newInst->matrix + 0xC) = *(int *)((uintptr_t)&inst->matrix + 0xC);
			*(s16 *)((uintptr_t)&newInst->matrix + 0x10) = *(s16 *)((uintptr_t)&inst->matrix + 0x10);
			newInst->matrix.t[0] = inst->matrix.t[0];
			newInst->matrix.t[1] = inst->matrix.t[1];
			newInst->matrix.t[2] = inst->matrix.t[2];

			newInst->alphaScale = 0xc00;

			warppadObj->inst[WPIS_OPEN_BEAM] = newInst;
		}

		// if spiral ring exists
		if (gGT->modelPtr[STATIC_BOTTOMRING] != 0)
		{
			for (i = 0; i < 2; i++)
			{
				newInst = INSTANCE_Birth3D(gGT->modelPtr[STATIC_BOTTOMRING], "bottomRing", t);

				// copy matrix
				*(int *)((uintptr_t)&newInst->matrix + 0x0) = *(int *)((uintptr_t)&inst->matrix + 0x0);
				*(int *)((uintptr_t)&newInst->matrix + 0x4) = *(int *)((uintptr_t)&inst->matrix + 0x4);
				*(int *)((uintptr_t)&newInst->matrix + 0x8) = *(int *)((uintptr_t)&inst->matrix + 0x8);
				*(int *)((uintptr_t)&newInst->matrix + 0xC) = *(int *)((uintptr_t)&inst->matrix + 0xC);
				*(s16 *)((uintptr_t)&newInst->matrix + 0x10) = *(s16 *)((uintptr_t)&inst->matrix + 0x10);
				newInst->matrix.t[0] = inst->matrix.t[0];
				newInst->matrix.t[1] = inst->matrix.t[1] + i * 0x400;
				newInst->matrix.t[2] = inst->matrix.t[2];

				newInst->alphaScale = 0x400;

				warppadObj->inst[WPIS_OPEN_RING1 + i] = newInst;
			}
		}

		for (i = 0; i < 3; i++)
			warppadObj->thirds[i] = 0x555 * i;

		warppadObj->spinRot_Prize.x = 0;
		warppadObj->spinRot_Prize.y = 0;
		warppadObj->spinRot_Prize.z = 0;

		warppadObj->spinRot_Beam.x = 0;
		warppadObj->spinRot_Beam.y = 0;
		warppadObj->spinRot_Beam.z = 0;

		for (i = 0; i < 2; i++)
		{
			warppadObj->spinRot_Wisp[i].x = 0;
			warppadObj->spinRot_Wisp[i].y = 0;
			warppadObj->spinRot_Wisp[i].z = 0;
		}

		if (levelID < AH_WP_SLIDE_COLISEUM)
		{
			// unlocked all
			t->modelIndex = 2;

			// if trophy not owned
			if (CHECK_ADV_BIT(sdata->advProgress.rewards, levelID + ADV_REWARD_FIRST_TROPHY) == 0)
			{
				// open for trophy
				t->modelIndex = 1;

				rewardAngle = 0;
				for (i = 0; i < 3; i++)
				{
					rewardModelID = s_warpPadRewardModelIDs[i];
					newInst = INSTANCE_Birth3D(gGT->modelPtr[rewardModelID], "prize1", t);
					warppadObj->inst[WPIS_OPEN_PRIZE1 + i] = newInst;

					// copy matrix
					*(int *)((uintptr_t)&newInst->matrix + 0x0) = *(int *)((uintptr_t)&inst->matrix + 0x0);
					*(int *)((uintptr_t)&newInst->matrix + 0x4) = *(int *)((uintptr_t)&inst->matrix + 0x4);
					*(int *)((uintptr_t)&newInst->matrix + 0x8) = *(int *)((uintptr_t)&inst->matrix + 0x8);
					*(int *)((uintptr_t)&newInst->matrix + 0xC) = *(int *)((uintptr_t)&inst->matrix + 0xC);
					*(s16 *)((uintptr_t)&newInst->matrix + 0x10) = *(s16 *)((uintptr_t)&inst->matrix + 0x10);
					newInst->matrix.t[0] = inst->matrix.t[0] + ((MATH_Sin(rewardAngle) * 0xc0) >> 0xc);
					newInst->matrix.t[1] = inst->matrix.t[1] + 0x100;
					newInst->matrix.t[2] = inst->matrix.t[2] + ((MATH_Cos(rewardAngle) * 0xc0) >> 0xc);

					if (rewardModelID == STATIC_RELIC)
					{
						newInst->colorRGBA = 0x20a5ff0;
						newInst->flags |= USE_SPECULAR_LIGHT;
						newInst->scale.x = 0x1800;
						newInst->scale.y = 0x1800;
						newInst->scale.z = 0x1800;
					}

					else if (rewardModelID == STATIC_TOKEN)
					{
						tokenGroupID = data.metaDataLEV[levelID].ctrTokenGroupID;

						// token color
						newInst->colorRGBA = ((u32)data.AdvCups[tokenGroupID].color[0] << 0x14) | ((u32)data.AdvCups[tokenGroupID].color[1] << 0xc) |
						                     ((u32)data.AdvCups[tokenGroupID].color[2] << 0x4);

						newInst->flags |= (DRAW_TRANSPARENT | USE_SPECULAR_LIGHT);

						warppadObj->lightDirToken = D232.lightDirToken[tokenGroupID];

						newInst->scale.x = 0x2000;
						newInst->scale.y = 0x2000;
						newInst->scale.z = 0x2000;
					}

					else
					{
						newInst->scale.x = 0x2800;
						newInst->scale.y = 0x2800;
						newInst->scale.z = 0x2800;
					}

					rewardAngle += 0x555;
				}

				return;
			}

			// if relic not owned
			if (CHECK_ADV_BIT(sdata->advProgress.rewards, levelID + ADV_REWARD_FIRST_SAPPHIRE_RELIC) == 0)
			{
				// open for relic/token
				t->modelIndex = 3;

				newInst = INSTANCE_Birth3D(gGT->modelPtr[STATIC_RELIC], "prize2", t);

				// relic blue
				newInst->colorRGBA = 0x20a5ff0;

				newInst->flags |= USE_SPECULAR_LIGHT;

				warppadObj->lightDirRelic = D232.lightDirRelic[0];

				// copy matrix
				*(int *)((uintptr_t)&newInst->matrix + 0x0) = *(int *)((uintptr_t)&inst->matrix + 0x0);
				*(int *)((uintptr_t)&newInst->matrix + 0x4) = *(int *)((uintptr_t)&inst->matrix + 0x4);
				*(int *)((uintptr_t)&newInst->matrix + 0x8) = *(int *)((uintptr_t)&inst->matrix + 0x8);
				*(int *)((uintptr_t)&newInst->matrix + 0xC) = *(int *)((uintptr_t)&inst->matrix + 0xC);
				*(s16 *)((uintptr_t)&newInst->matrix + 0x10) = *(s16 *)((uintptr_t)&inst->matrix + 0x10);
				newInst->matrix.t[0] = inst->matrix.t[0];
				newInst->matrix.t[1] = inst->matrix.t[1] + 0x100;
				newInst->matrix.t[2] = inst->matrix.t[2];

				newInst->scale.x = 0x1800;
				newInst->scale.y = 0x1800;
				newInst->scale.z = 0x1800;

				warppadObj->inst[WPIS_OPEN_PRIZE1] = newInst;
			}

			// if token owned
			if (CHECK_ADV_BIT(sdata->advProgress.rewards, levelID + ADV_REWARD_FIRST_CTR_TOKEN) != 0)
				return;

			tokenGroupID = data.metaDataLEV[levelID].ctrTokenGroupID;

			// open for relic/token
			t->modelIndex = 3;

			newInst = INSTANCE_Birth3D(gGT->modelPtr[STATIC_TOKEN], "prize2", t);

			// token color
			newInst->colorRGBA = ((u32)data.AdvCups[tokenGroupID].color[0] << 0x14) | ((u32)data.AdvCups[tokenGroupID].color[1] << 0xc) |
			                     ((u32)data.AdvCups[tokenGroupID].color[2] << 0x4);

			newInst->flags |= (DRAW_TRANSPARENT | USE_SPECULAR_LIGHT);

			warppadObj->lightDirToken = D232.lightDirToken[tokenGroupID];

			// copy matrix
			*(int *)((uintptr_t)&newInst->matrix + 0x0) = *(int *)((uintptr_t)&inst->matrix + 0x0);
			*(int *)((uintptr_t)&newInst->matrix + 0x4) = *(int *)((uintptr_t)&inst->matrix + 0x4);
			*(int *)((uintptr_t)&newInst->matrix + 0x8) = *(int *)((uintptr_t)&inst->matrix + 0x8);
			*(int *)((uintptr_t)&newInst->matrix + 0xC) = *(int *)((uintptr_t)&inst->matrix + 0xC);
			*(s16 *)((uintptr_t)&newInst->matrix + 0x10) = *(s16 *)((uintptr_t)&inst->matrix + 0x10);
			newInst->matrix.t[0] = inst->matrix.t[0];
			newInst->matrix.t[1] = inst->matrix.t[1] + 0x100;
			newInst->matrix.t[2] = inst->matrix.t[2];

			newInst->scale.x = 0x2000;
			newInst->scale.y = 0x2000;
			newInst->scale.z = 0x2000;

			warppadObj->inst[WPIS_OPEN_PRIZE2] = newInst;

			return;

		SlideColTurboTrack:

			// if relic not owned
			if (levelID < AH_WP_NITRO_COURT)
				if (CHECK_ADV_BIT(sdata->advProgress.rewards, levelID + ADV_REWARD_FIRST_SAPPHIRE_RELIC) == 0)
				{
					// SlideCol/TurboTrack
					if (levelID >= AH_WP_SLIDE_COLISEUM)
						t->modelIndex = 4;

					// open for token/relic
					else if (t->modelIndex != 1)
						t->modelIndex = 3;

					newInst = INSTANCE_Birth3D(gGT->modelPtr[STATIC_RELIC], "prize2", t);

					// relic blue
					newInst->colorRGBA = 0x20a5ff0;

					newInst->flags |= USE_SPECULAR_LIGHT;

					warppadObj->lightDirRelic = D232.lightDirRelic[0];

					newInst->scale.x = 0x1800;
					newInst->scale.y = 0x1800;
					newInst->scale.z = 0x1800;

					warppadObj->inst[WPIS_OPEN_PRIZE1] = newInst;
				}

			for (i = 0; i < 3; i++)
			{
				newInst = warppadObj->inst[WPIS_OPEN_PRIZE1 + i];

				if (newInst == 0)
					continue;

				// copy matrix
				*(int *)((uintptr_t)&newInst->matrix + 0x0) = *(int *)((uintptr_t)&inst->matrix + 0x0);
				*(int *)((uintptr_t)&newInst->matrix + 0x4) = *(int *)((uintptr_t)&inst->matrix + 0x4);
				*(int *)((uintptr_t)&newInst->matrix + 0x8) = *(int *)((uintptr_t)&inst->matrix + 0x8);
				*(int *)((uintptr_t)&newInst->matrix + 0xC) = *(int *)((uintptr_t)&inst->matrix + 0xC);
				*(s16 *)((uintptr_t)&newInst->matrix + 0x10) = *(s16 *)((uintptr_t)&inst->matrix + 0x10);
				newInst->matrix.t[0] = inst->matrix.t[0];
				newInst->matrix.t[1] = inst->matrix.t[1] + 0x100;
				newInst->matrix.t[2] = inst->matrix.t[2];
			}
		}

		// slide col, turbo track
		else if (levelID < AH_WP_NITRO_COURT)
		{
			// already unlocked
			t->modelIndex = 2;

			goto SlideColTurboTrack;
		}

		// battle tracks
		else if ((((u16)(levelID - AH_WP_NITRO_COURT)) < 2) || (levelID == 21) || (levelID == 23))
		{
			i = R232.battleTrackArr[levelID - AH_WP_NITRO_COURT] + ADV_REWARD_FIRST_PURPLE_TOKEN;

			// already unlocked
			t->modelIndex = 2;

			if (CHECK_ADV_BIT(sdata->advProgress.rewards, i) == 0)
			{
				// rainbow
				t->modelIndex = 4;

				newInst = INSTANCE_Birth3D(gGT->modelPtr[STATIC_TOKEN], "prize2", t);

				newInst->flags |= USE_SPECULAR_LIGHT;

				tokenGroupID = 4;

				// token color
				newInst->colorRGBA = ((u32)data.AdvCups[tokenGroupID].color[0] << 0x14) | ((u32)data.AdvCups[tokenGroupID].color[1] << 0xc) |
				                     ((u32)data.AdvCups[tokenGroupID].color[2] << 0x4);

				warppadObj->lightDirToken = D232.lightDirToken[tokenGroupID];

				newInst->scale.x = 0x2000;
				newInst->scale.y = 0x2000;
				newInst->scale.z = 0x2000;

				warppadObj->inst[WPIS_OPEN_PRIZE1] = newInst;

				// for matrix copy
				goto SlideColTurboTrack;
			}
		}

		// gem cups
		else if (((u16)(levelID - AH_WP_ADV_CUP)) < 5)
		{
			// bit index of gem
			i = (levelID - AH_WP_ADV_CUP) + ADV_REWARD_FIRST_GEM;

			// if gem is already unlocked, quit
			if (CHECK_ADV_BIT(sdata->advProgress.rewards, i) != 0)
			{
				// beaten
				t->modelIndex = 2;

				return;
			}

			// rainbow color
			t->modelIndex = 4;

			newInst = INSTANCE_Birth3D(gGT->modelPtr[STATIC_GEM], "prize2", t);

			newInst->flags |= USE_SPECULAR_LIGHT;

			i = levelID - AH_WP_ADV_CUP;

			// token color
			newInst->colorRGBA = ((u32)data.AdvCups[i].color[0] << 0x14) | ((u32)data.AdvCups[i].color[1] << 0xc) | ((u32)data.AdvCups[i].color[2] << 0x4);

			warppadObj->inst[WPIS_OPEN_PRIZE1] = newInst;

			// store in Gem array
			warppadObj->lightDirGem = D232.lightDirGem[i];

			newInst->scale.x = 0x2000;
			newInst->scale.y = 0x2000;
			newInst->scale.z = 0x2000;

			// for matrix copy
			goto SlideColTurboTrack;
		}

		return;
	}

	// === if locked ===

	if (unlockItem_numNeeded < 10)
	{
		warppadObj->digit10s = 0;
		warppadObj->digit1s = unlockItem_numNeeded;
	}

	else
	{
		warppadObj->digit10s = 1;
		warppadObj->digit1s = unlockItem_numNeeded - 10;
	}

	// ====== Item ========

	// WPIS_CLOSED_ITEM
	newInst = INSTANCE_Birth3D(gGT->modelPtr[unlockItem_modelID], "reqObj", t);

	// copy matrix
	*(int *)((uintptr_t)&newInst->matrix + 0x0) = *(int *)((uintptr_t)&inst->matrix + 0x0);
	*(int *)((uintptr_t)&newInst->matrix + 0x4) = *(int *)((uintptr_t)&inst->matrix + 0x4);
	*(int *)((uintptr_t)&newInst->matrix + 0x8) = *(int *)((uintptr_t)&inst->matrix + 0x8);
	*(int *)((uintptr_t)&newInst->matrix + 0xC) = *(int *)((uintptr_t)&inst->matrix + 0xC);
	*(s16 *)((uintptr_t)&newInst->matrix + 0x10) = *(s16 *)((uintptr_t)&inst->matrix + 0x10);
	newInst->matrix.t[0] = inst->matrix.t[0];
	newInst->matrix.t[1] = inst->matrix.t[1] + 0x100;
	newInst->matrix.t[2] = inst->matrix.t[2];

	newInst->scale.x = 0x2000;
	newInst->scale.y = 0x2000;
	newInst->scale.z = 0x2000;

	// no specular for trophy
	if (unlockItem_modelID != STATIC_TROPHY)
	{
		newInst->flags |= USE_SPECULAR_LIGHT;

		// relic
		if (unlockItem_modelID == STATIC_RELIC)
		{
			// Relic blue color
			newInst->colorRGBA = 0x20a5ff0;

			warppadObj->lightDirRelic = D232.lightDirRelic[0];
		}

		// Key
		else if (unlockItem_modelID == STATIC_KEY)
		{
			// Key color
			newInst->colorRGBA = 0xdca6000;

			// store in Gem array (intended by ND, not a bug)
			warppadObj->lightDirGem = D232.lightDirGem[0];
		}

		// Gem
		else if (unlockItem_modelID == STATIC_GEM)
		{
			newInst->colorRGBA = ((u32)data.AdvCups[0].color[0] << 0x14) | ((u32)data.AdvCups[0].color[1] << 0xc) | ((u32)data.AdvCups[0].color[2] << 0x4);

			// store in Gem array
			warppadObj->lightDirGem = D232.lightDirGem[0];
		}

		// assume token
		else
		{
			i = levelID - AH_WP_ADV_CUP;

			// token color
			newInst->colorRGBA = ((u32)data.AdvCups[i].color[0] << 0x14) | ((u32)data.AdvCups[i].color[1] << 0xc) | ((u32)data.AdvCups[i].color[2] << 0x4);

			warppadObj->lightDirToken = D232.lightDirToken[i];
		}
	}

	warppadObj->inst[WPIS_CLOSED_ITEM] = newInst;

	// ====== "X" ========

	// WPIS_CLOSED_X
	newInst = INSTANCE_Birth3D(gGT->modelPtr[STATIC_BIGX], "x", t);

	// copy matrix
	*(int *)((uintptr_t)&newInst->matrix + 0x0) = 0x1000;
	*(int *)((uintptr_t)&newInst->matrix + 0x4) = 0;
	*(int *)((uintptr_t)&newInst->matrix + 0x8) = 0x1000;
	*(int *)((uintptr_t)&newInst->matrix + 0xC) = 0;
	*(s16 *)((uintptr_t)&newInst->matrix + 0x10) = 0x1000;
	newInst->matrix.t[0] = inst->matrix.t[0];
	newInst->matrix.t[1] = inst->matrix.t[1] + 0x100;
	newInst->matrix.t[2] = inst->matrix.t[2];

	newInst->scale.x = 0x2000;
	newInst->scale.y = 0x2000;
	newInst->scale.z = 0x2000;

	// always face camera
	newInst->model->headers[0].flags |= 1;

	warppadObj->inst[WPIS_CLOSED_X] = newInst;

	// ====== "10s" ========

	if (warppadObj->digit10s != 0)
	{
		// WPIS_CLOSED_10S
		newInst = INSTANCE_Birth3D(gGT->modelPtr[STATIC_BIG1], "warpnum", t);

		// copy matrix
		*(int *)((uintptr_t)&newInst->matrix + 0x0) = 0x1000;
		*(int *)((uintptr_t)&newInst->matrix + 0x4) = 0;
		*(int *)((uintptr_t)&newInst->matrix + 0x8) = 0x1000;
		*(int *)((uintptr_t)&newInst->matrix + 0xC) = 0;
		*(s16 *)((uintptr_t)&newInst->matrix + 0x10) = 0x1000;
		newInst->matrix.t[0] = inst->matrix.t[0];
		newInst->matrix.t[1] = inst->matrix.t[1] + 0x100;
		newInst->matrix.t[2] = inst->matrix.t[2];

		newInst->scale.x = 0x2000;
		newInst->scale.y = 0x2000;
		newInst->scale.z = 0x2000;

		// always face camera
		for (i = 0; i < newInst->model->numHeaders; i++)
			newInst->model->headers[i].flags |= 1;

		warppadObj->inst[WPIS_CLOSED_10S] = newInst;
	}

	// ====== "1s" ========

	// STATIC_BIG (1-8)
	i = 0x38;
	if (warppadObj->digit1s == 0)
		i = 0x6d; // '0'
	if (warppadObj->digit1s == 9)
		i = 0x6e; // '9'

	// WPIS_CLOSED_1S
	newInst = INSTANCE_Birth3D(gGT->modelPtr[i], "warpnum", t);

	// copy matrix
	*(int *)((uintptr_t)&newInst->matrix + 0x0) = 0x1000;
	*(int *)((uintptr_t)&newInst->matrix + 0x4) = 0;
	*(int *)((uintptr_t)&newInst->matrix + 0x8) = 0x1000;
	*(int *)((uintptr_t)&newInst->matrix + 0xC) = 0;
	*(s16 *)((uintptr_t)&newInst->matrix + 0x10) = 0x1000;
	newInst->matrix.t[0] = inst->matrix.t[0];
	newInst->matrix.t[1] = inst->matrix.t[1] + 0x100;
	newInst->matrix.t[2] = inst->matrix.t[2];

	newInst->scale.x = 0x2000;
	newInst->scale.y = 0x2000;
	newInst->scale.z = 0x2000;

	// always face camera
	for (i = 0; i < newInst->model->numHeaders; i++)
		newInst->model->headers[i].flags |= 1;

	warppadObj->inst[WPIS_CLOSED_1S] = newInst;
}
