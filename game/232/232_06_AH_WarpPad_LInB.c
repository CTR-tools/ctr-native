#include <common.h>

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
	inst->flags |= 0x80;

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
		if (CHECK_ADV_BIT(sdata->advProgress.rewards, (levelID + 6)) != 0)
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
			if (CHECK_ADV_BIT(sdata->advProgress.rewards, (i + 0x6a)) != 0)
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
			*(int *)((int)&newInst->matrix + 0x0) = *(int *)((int)&inst->matrix + 0x0);
			*(int *)((int)&newInst->matrix + 0x4) = *(int *)((int)&inst->matrix + 0x4);
			*(int *)((int)&newInst->matrix + 0x8) = *(int *)((int)&inst->matrix + 0x8);
			*(int *)((int)&newInst->matrix + 0xC) = *(int *)((int)&inst->matrix + 0xC);
			*(s16 *)((int)&newInst->matrix + 0x10) = *(s16 *)((int)&inst->matrix + 0x10);
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
				*(int *)((int)&newInst->matrix + 0x0) = *(int *)((int)&inst->matrix + 0x0);
				*(int *)((int)&newInst->matrix + 0x4) = *(int *)((int)&inst->matrix + 0x4);
				*(int *)((int)&newInst->matrix + 0x8) = *(int *)((int)&inst->matrix + 0x8);
				*(int *)((int)&newInst->matrix + 0xC) = *(int *)((int)&inst->matrix + 0xC);
				*(s16 *)((int)&newInst->matrix + 0x10) = *(s16 *)((int)&inst->matrix + 0x10);
				newInst->matrix.t[0] = inst->matrix.t[0];
				newInst->matrix.t[1] = inst->matrix.t[1] + i * 0x400;
				newInst->matrix.t[2] = inst->matrix.t[2];

				newInst->alphaScale = 0x400;

				warppadObj->inst[WPIS_OPEN_RING1 + i] = newInst;
			}
		}

		for (i = 0; i < 3; i++)
			warppadObj->thirds[i] = 0x555 * i;

		warppadObj->spinRot_Prize[0] = 0;
		warppadObj->spinRot_Prize[1] = 0;
		warppadObj->spinRot_Prize[2] = 0;

		warppadObj->spinRot_Beam[0] = 0;
		warppadObj->spinRot_Beam[1] = 0;
		warppadObj->spinRot_Beam[2] = 0;

		for (i = 0; i < 2; i++)
		{
			warppadObj->spinRot_Wisp[i][0] = 0;
			warppadObj->spinRot_Wisp[i][1] = 0;
			warppadObj->spinRot_Wisp[i][2] = 0;
		}

		if (levelID < AH_WP_SLIDE_COLISEUM)
		{
			// unlocked all
			t->modelIndex = 2;

			// if trophy not owned
			if (CHECK_ADV_BIT(sdata->advProgress.rewards, (levelID + 6)) == 0)
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
					*(int *)((int)&newInst->matrix + 0x0) = *(int *)((int)&inst->matrix + 0x0);
					*(int *)((int)&newInst->matrix + 0x4) = *(int *)((int)&inst->matrix + 0x4);
					*(int *)((int)&newInst->matrix + 0x8) = *(int *)((int)&inst->matrix + 0x8);
					*(int *)((int)&newInst->matrix + 0xC) = *(int *)((int)&inst->matrix + 0xC);
					*(s16 *)((int)&newInst->matrix + 0x10) = *(s16 *)((int)&inst->matrix + 0x10);
					newInst->matrix.t[0] = inst->matrix.t[0] + ((MATH_Sin(rewardAngle) * 0xc0) >> 0xc);
					newInst->matrix.t[1] = inst->matrix.t[1] + 0x100;
					newInst->matrix.t[2] = inst->matrix.t[2] + ((MATH_Cos(rewardAngle) * 0xc0) >> 0xc);

					if (rewardModelID == STATIC_RELIC)
					{
						newInst->colorRGBA = 0x20a5ff0;
						newInst->flags |= 0x20000;
						newInst->scale[0] = 0x1800;
						newInst->scale[1] = 0x1800;
						newInst->scale[2] = 0x1800;
					}

					else if (rewardModelID == STATIC_TOKEN)
					{
						tokenGroupID = data.metaDataLEV[levelID].ctrTokenGroupID;

						// token color
						newInst->colorRGBA = ((u32)data.AdvCups[tokenGroupID].color[0] << 0x14) | ((u32)data.AdvCups[tokenGroupID].color[1] << 0xc) |
						                     ((u32)data.AdvCups[tokenGroupID].color[2] << 0x4);

						// specular lighting
						newInst->flags |= 0x30000;

						warppadObj->specLightToken[0] = D232.specLightToken[tokenGroupID * 3 + 0];
						warppadObj->specLightToken[1] = D232.specLightToken[tokenGroupID * 3 + 1];
						warppadObj->specLightToken[2] = D232.specLightToken[tokenGroupID * 3 + 2];

						newInst->scale[0] = 0x2000;
						newInst->scale[1] = 0x2000;
						newInst->scale[2] = 0x2000;
					}

					else
					{
						newInst->scale[0] = 0x2800;
						newInst->scale[1] = 0x2800;
						newInst->scale[2] = 0x2800;
					}

					rewardAngle += 0x555;
				}

				return;
			}

			// if relic not owned
			if (CHECK_ADV_BIT(sdata->advProgress.rewards, (levelID + 0x16)) == 0)
			{
				// open for relic/token
				t->modelIndex = 3;

				newInst = INSTANCE_Birth3D(gGT->modelPtr[STATIC_RELIC], "prize2", t);

				// relic blue
				newInst->colorRGBA = 0x20a5ff0;

				// specular lighting
				newInst->flags |= 0x20000;

				warppadObj->specLightRelic[0] = D232.specLightRelic[0];
				warppadObj->specLightRelic[1] = D232.specLightRelic[1];
				warppadObj->specLightRelic[2] = D232.specLightRelic[2];

				// copy matrix
				*(int *)((int)&newInst->matrix + 0x0) = *(int *)((int)&inst->matrix + 0x0);
				*(int *)((int)&newInst->matrix + 0x4) = *(int *)((int)&inst->matrix + 0x4);
				*(int *)((int)&newInst->matrix + 0x8) = *(int *)((int)&inst->matrix + 0x8);
				*(int *)((int)&newInst->matrix + 0xC) = *(int *)((int)&inst->matrix + 0xC);
				*(s16 *)((int)&newInst->matrix + 0x10) = *(s16 *)((int)&inst->matrix + 0x10);
				newInst->matrix.t[0] = inst->matrix.t[0];
				newInst->matrix.t[1] = inst->matrix.t[1] + 0x100;
				newInst->matrix.t[2] = inst->matrix.t[2];

				newInst->scale[0] = 0x1800;
				newInst->scale[1] = 0x1800;
				newInst->scale[2] = 0x1800;

				warppadObj->inst[WPIS_OPEN_PRIZE1] = newInst;
			}

			// if token owned
			if (CHECK_ADV_BIT(sdata->advProgress.rewards, (levelID + 0x4c)) != 0)
				return;

			tokenGroupID = data.metaDataLEV[levelID].ctrTokenGroupID;

			// open for relic/token
			t->modelIndex = 3;

			newInst = INSTANCE_Birth3D(gGT->modelPtr[STATIC_TOKEN], "prize2", t);

			// token color
			newInst->colorRGBA = ((u32)data.AdvCups[tokenGroupID].color[0] << 0x14) | ((u32)data.AdvCups[tokenGroupID].color[1] << 0xc) |
			                     ((u32)data.AdvCups[tokenGroupID].color[2] << 0x4);

			// specular lighting
			newInst->flags |= 0x30000;

			warppadObj->specLightToken[0] = D232.specLightToken[tokenGroupID * 3 + 0];
			warppadObj->specLightToken[1] = D232.specLightToken[tokenGroupID * 3 + 1];
			warppadObj->specLightToken[2] = D232.specLightToken[tokenGroupID * 3 + 2];

			// copy matrix
			*(int *)((int)&newInst->matrix + 0x0) = *(int *)((int)&inst->matrix + 0x0);
			*(int *)((int)&newInst->matrix + 0x4) = *(int *)((int)&inst->matrix + 0x4);
			*(int *)((int)&newInst->matrix + 0x8) = *(int *)((int)&inst->matrix + 0x8);
			*(int *)((int)&newInst->matrix + 0xC) = *(int *)((int)&inst->matrix + 0xC);
			*(s16 *)((int)&newInst->matrix + 0x10) = *(s16 *)((int)&inst->matrix + 0x10);
			newInst->matrix.t[0] = inst->matrix.t[0];
			newInst->matrix.t[1] = inst->matrix.t[1] + 0x100;
			newInst->matrix.t[2] = inst->matrix.t[2];

			newInst->scale[0] = 0x2000;
			newInst->scale[1] = 0x2000;
			newInst->scale[2] = 0x2000;

			warppadObj->inst[WPIS_OPEN_PRIZE2] = newInst;

			return;

		SlideColTurboTrack:

			// if relic not owned
			if (levelID < AH_WP_NITRO_COURT)
				if (CHECK_ADV_BIT(sdata->advProgress.rewards, (levelID + 0x16)) == 0)
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

					// specular lighting
					newInst->flags |= 0x20000;

					warppadObj->specLightRelic[0] = D232.specLightRelic[0];
					warppadObj->specLightRelic[1] = D232.specLightRelic[1];
					warppadObj->specLightRelic[2] = D232.specLightRelic[2];

					newInst->scale[0] = 0x1800;
					newInst->scale[1] = 0x1800;
					newInst->scale[2] = 0x1800;

					warppadObj->inst[WPIS_OPEN_PRIZE1] = newInst;
				}

			for (i = 0; i < 3; i++)
			{
				newInst = warppadObj->inst[WPIS_OPEN_PRIZE1 + i];

				if (newInst == 0)
					continue;

				// copy matrix
				*(int *)((int)&newInst->matrix + 0x0) = *(int *)((int)&inst->matrix + 0x0);
				*(int *)((int)&newInst->matrix + 0x4) = *(int *)((int)&inst->matrix + 0x4);
				*(int *)((int)&newInst->matrix + 0x8) = *(int *)((int)&inst->matrix + 0x8);
				*(int *)((int)&newInst->matrix + 0xC) = *(int *)((int)&inst->matrix + 0xC);
				*(s16 *)((int)&newInst->matrix + 0x10) = *(s16 *)((int)&inst->matrix + 0x10);
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
			i = R232.battleTrackArr[levelID - AH_WP_NITRO_COURT] + 0x6f;

			// already unlocked
			t->modelIndex = 2;

			if (CHECK_ADV_BIT(sdata->advProgress.rewards, i) == 0)
			{
				// rainbow
				t->modelIndex = 4;

				newInst = INSTANCE_Birth3D(gGT->modelPtr[STATIC_TOKEN], "prize2", t);

				// specular lighting
				newInst->flags |= 0x20000;

				tokenGroupID = 4;

				// token color
				newInst->colorRGBA = ((u32)data.AdvCups[tokenGroupID].color[0] << 0x14) | ((u32)data.AdvCups[tokenGroupID].color[1] << 0xc) |
				                     ((u32)data.AdvCups[tokenGroupID].color[2] << 0x4);

				warppadObj->specLightToken[0] = D232.specLightToken[tokenGroupID * 3 + 0];
				warppadObj->specLightToken[1] = D232.specLightToken[tokenGroupID * 3 + 1];
				warppadObj->specLightToken[2] = D232.specLightToken[tokenGroupID * 3 + 2];

				newInst->scale[0] = 0x2000;
				newInst->scale[1] = 0x2000;
				newInst->scale[2] = 0x2000;

				warppadObj->inst[WPIS_OPEN_PRIZE1] = newInst;

				// for matrix copy
				goto SlideColTurboTrack;
			}
		}

		// gem cups
		else if (((u16)(levelID - AH_WP_ADV_CUP)) < 5)
		{
			// bit index of gem
			i = (levelID - AH_WP_ADV_CUP) + 0x6a;

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

			// specular lighting
			newInst->flags |= 0x20000;

			i = levelID - AH_WP_ADV_CUP;

			// token color
			newInst->colorRGBA = ((u32)data.AdvCups[i].color[0] << 0x14) | ((u32)data.AdvCups[i].color[1] << 0xc) | ((u32)data.AdvCups[i].color[2] << 0x4);

			warppadObj->inst[WPIS_OPEN_PRIZE1] = newInst;

			// store in Gem array
			warppadObj->specLightGem[0] = D232.specLightGem[i * 3 + 0];
			warppadObj->specLightGem[1] = D232.specLightGem[i * 3 + 1];
			warppadObj->specLightGem[2] = D232.specLightGem[i * 3 + 2];

			newInst->scale[0] = 0x2000;
			newInst->scale[1] = 0x2000;
			newInst->scale[2] = 0x2000;

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
	*(int *)((int)&newInst->matrix + 0x0) = *(int *)((int)&inst->matrix + 0x0);
	*(int *)((int)&newInst->matrix + 0x4) = *(int *)((int)&inst->matrix + 0x4);
	*(int *)((int)&newInst->matrix + 0x8) = *(int *)((int)&inst->matrix + 0x8);
	*(int *)((int)&newInst->matrix + 0xC) = *(int *)((int)&inst->matrix + 0xC);
	*(s16 *)((int)&newInst->matrix + 0x10) = *(s16 *)((int)&inst->matrix + 0x10);
	newInst->matrix.t[0] = inst->matrix.t[0];
	newInst->matrix.t[1] = inst->matrix.t[1] + 0x100;
	newInst->matrix.t[2] = inst->matrix.t[2];

	newInst->scale[0] = 0x2000;
	newInst->scale[1] = 0x2000;
	newInst->scale[2] = 0x2000;

	// no specular for trophy
	if (unlockItem_modelID != STATIC_TROPHY)
	{
		// specular lighting
		newInst->flags |= 0x20000;

		// relic
		if (unlockItem_modelID == STATIC_RELIC)
		{
			// Relic blue color
			newInst->colorRGBA = 0x20a5ff0;

			warppadObj->specLightRelic[0] = D232.specLightRelic[0];
			warppadObj->specLightRelic[1] = D232.specLightRelic[1];
			warppadObj->specLightRelic[2] = D232.specLightRelic[2];
		}

		// Key
		else if (unlockItem_modelID == STATIC_KEY)
		{
			// Key color
			newInst->colorRGBA = 0xdca6000;

			// store in Gem array (intended by ND, not a bug)
			warppadObj->specLightGem[0] = D232.specLightGem[0];
			warppadObj->specLightGem[1] = D232.specLightGem[1];
			warppadObj->specLightGem[2] = D232.specLightGem[2];
		}

		// Gem
		else if (unlockItem_modelID == STATIC_GEM)
		{
			newInst->colorRGBA = ((u32)data.AdvCups[0].color[0] << 0x14) | ((u32)data.AdvCups[0].color[1] << 0xc) | ((u32)data.AdvCups[0].color[2] << 0x4);

			// store in Gem array
			warppadObj->specLightGem[0] = D232.specLightGem[0];
			warppadObj->specLightGem[1] = D232.specLightGem[1];
			warppadObj->specLightGem[2] = D232.specLightGem[2];
		}

		// assume token
		else
		{
			i = levelID - AH_WP_ADV_CUP;

			// token color
			newInst->colorRGBA = ((u32)data.AdvCups[i].color[0] << 0x14) | ((u32)data.AdvCups[i].color[1] << 0xc) | ((u32)data.AdvCups[i].color[2] << 0x4);

			warppadObj->specLightToken[0] = D232.specLightToken[i * 3 + 0];
			warppadObj->specLightToken[1] = D232.specLightToken[i * 3 + 1];
			warppadObj->specLightToken[2] = D232.specLightToken[i * 3 + 2];
		}
	}

	warppadObj->inst[WPIS_CLOSED_ITEM] = newInst;

	// ====== "X" ========

	// WPIS_CLOSED_X
	newInst = INSTANCE_Birth3D(gGT->modelPtr[STATIC_BIGX], "x", t);

	// copy matrix
	*(int *)((int)&newInst->matrix + 0x0) = 0x1000;
	*(int *)((int)&newInst->matrix + 0x4) = 0;
	*(int *)((int)&newInst->matrix + 0x8) = 0x1000;
	*(int *)((int)&newInst->matrix + 0xC) = 0;
	*(s16 *)((int)&newInst->matrix + 0x10) = 0x1000;
	newInst->matrix.t[0] = inst->matrix.t[0];
	newInst->matrix.t[1] = inst->matrix.t[1] + 0x100;
	newInst->matrix.t[2] = inst->matrix.t[2];

	newInst->scale[0] = 0x2000;
	newInst->scale[1] = 0x2000;
	newInst->scale[2] = 0x2000;

	// always face camera
	newInst->model->headers[0].flags |= 1;

	warppadObj->inst[WPIS_CLOSED_X] = newInst;

	// ====== "10s" ========

	if (warppadObj->digit10s != 0)
	{
		// WPIS_CLOSED_10S
		newInst = INSTANCE_Birth3D(gGT->modelPtr[STATIC_BIG1], "warpnum", t);

		// copy matrix
		*(int *)((int)&newInst->matrix + 0x0) = 0x1000;
		*(int *)((int)&newInst->matrix + 0x4) = 0;
		*(int *)((int)&newInst->matrix + 0x8) = 0x1000;
		*(int *)((int)&newInst->matrix + 0xC) = 0;
		*(s16 *)((int)&newInst->matrix + 0x10) = 0x1000;
		newInst->matrix.t[0] = inst->matrix.t[0];
		newInst->matrix.t[1] = inst->matrix.t[1] + 0x100;
		newInst->matrix.t[2] = inst->matrix.t[2];

		newInst->scale[0] = 0x2000;
		newInst->scale[1] = 0x2000;
		newInst->scale[2] = 0x2000;

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
	*(int *)((int)&newInst->matrix + 0x0) = 0x1000;
	*(int *)((int)&newInst->matrix + 0x4) = 0;
	*(int *)((int)&newInst->matrix + 0x8) = 0x1000;
	*(int *)((int)&newInst->matrix + 0xC) = 0;
	*(s16 *)((int)&newInst->matrix + 0x10) = 0x1000;
	newInst->matrix.t[0] = inst->matrix.t[0];
	newInst->matrix.t[1] = inst->matrix.t[1] + 0x100;
	newInst->matrix.t[2] = inst->matrix.t[2];

	newInst->scale[0] = 0x2000;
	newInst->scale[1] = 0x2000;
	newInst->scale[2] = 0x2000;

	// always face camera
	for (i = 0; i < newInst->model->numHeaders; i++)
		newInst->model->headers[i].flags |= 1;

	warppadObj->inst[WPIS_CLOSED_1S] = newInst;
}
