#include <common.h>

enum UIInstanceConstants
{
	UI_INSTANCE_MENU_READY_SHOW_MENU = 1,
	UI_INSTANCE_DRIVER_COUNT = 8,
	UI_INSTANCE_THREAD_FLAGS = SIZE_RELATIVE_POOL_BUCKET(sizeof(struct UiElement3D), NONE, SMALL, HUD),
	UI_INSTANCE_DEFAULT_LIGHT_X = -3224,
	UI_INSTANCE_DEFAULT_LIGHT_Y = 2463,
	UI_INSTANCE_DEFAULT_LIGHT_Z = 562,
	UI_INSTANCE_CRYSTAL_LIGHT_X = -2912,
	UI_INSTANCE_CRYSTAL_LIGHT_Y = 2912,
	UI_INSTANCE_CRYSTAL_LIGHT_Z = -728,
	UI_INSTANCE_GEM_COLOR = 0x6c08080,
	UI_INSTANCE_CRYSTAL_COLOR = 0xd22fff0,
	UI_INSTANCE_RELIC_COLOR = 0x60a5ff0,
	UI_INSTANCE_KEY_COLOR = 0xdca6000,
	UI_INSTANCE_CTR_COLOR = 0xffc8000,
	UI_INSTANCE_CTR_LETTER_COUNT = 3,
	UI_INSTANCE_CTR_LETTER_VEL_STEP = 4,
	UI_INSTANCE_CTR_LETTER_VEL_Y = 0xc,
	UI_INSTANCE_TOKEN_COLOR_R_SHIFT = 0x14,
	UI_INSTANCE_TOKEN_COLOR_G_SHIFT = 0xc,
	UI_INSTANCE_TOKEN_COLOR_B_SHIFT = 4,
	UI_INSTANCE_PUSHBUFFER_Z = 0x200,
	UI_INSTANCE_DEPTH_BIAS = 0x80,
	UI_INSTANCE_ROTATE_NONE = 0,
	UI_INSTANCE_SCALE = 0x1000,
	UI_INSTANCE_RELIC_TYPE_COUNT = 3,
	UI_INSTANCE_RELIC_PLATINUM_TYPE = 2,
	UI_INSTANCE_RANK_TRANSITION_FRAMES = 5,
	UI_INSTANCE_RELIC_TIME_UNITS_PER_MINUTE = 0xe100,
	UI_INSTANCE_RELIC_TIME_UNITS_PER_10_SECONDS = 0x2580,
	UI_INSTANCE_RELIC_TIME_UNITS_PER_SECOND = 0x3c0,
	UI_INSTANCE_RELIC_TIME_UNITS_PER_TENTH_SECOND = 0x60,
	UI_INSTANCE_SECONDS_TENS_DIGIT_BASE = 6,
	UI_INSTANCE_DECIMAL_DIGIT_BASE = 10,
	UI_INSTANCE_RELIC_HUNDREDTH_SCALE = 100,
};

CTR_STATIC_ASSERT(UI_INSTANCE_THREAD_FLAGS == 0x380310);
CTR_STATIC_ASSERT(UI_INSTANCE_DEFAULT_LIGHT_X == -3224);
CTR_STATIC_ASSERT(UI_INSTANCE_CRYSTAL_LIGHT_Z == -728);
CTR_STATIC_ASSERT(UI_INSTANCE_RELIC_TIME_UNITS_PER_MINUTE == 0xe100);
CTR_STATIC_ASSERT(UI_INSTANCE_RELIC_TIME_UNITS_PER_10_SECONDS == 0x2580);
CTR_STATIC_ASSERT(UI_INSTANCE_RELIC_TIME_UNITS_PER_SECOND == 0x3c0);
CTR_STATIC_ASSERT(UI_INSTANCE_RELIC_TIME_UNITS_PER_TENTH_SECOND == 0x60);

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004cae8-0x8004cec4.
struct Instance *UI_INSTANCE_BirthWithThread(int modelID, int tickFunc, int hudSlot, int rotateToHud, int pushBuffer, int threadName)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Model *model = gGT->modelPtr[modelID];
	struct UiElement2D *hudStruct;
	struct Instance *inst = NULL;
	struct Thread *driverThread;
	SVec3 rot;

	if (model == 0)
	{
		return NULL;
	}

	hudStruct = data.hudStructPtr[gGT->numPlyrCurrGame - 1];
	driverThread = gGT->threadBuckets[PLAYER].thread;

	while (driverThread != 0)
	{
		struct Driver *driver = driverThread->object;
		struct UiElement3D *ui3D;
		struct Thread *hudThread;
		s16 createdModelID;
		s16 scale;
		int rewardColor;

		hudThread = PROC_BirthWithObject(UI_INSTANCE_THREAD_FLAGS, (void *)tickFunc, (char *)threadName, NULL);

		// Get the object attached to the thread
		ui3D = hudThread->object;

		// Big Number HUD element
		inst = INSTANCE_Birth2D(model, 0, hudThread);

		// give the Instance to the thread
		hudThread->inst = inst;

		createdModelID = model->id;

		// bigNum
		if (createdModelID == STATIC_BIG1)
		{
			driver->instBigNum = inst;
		}

		// fruitDisp
		else if (createdModelID == STATIC_FRUITDISP)
		{
			driver->instFruitDisp = inst;
		}

		// if this is a gem
		else if (createdModelID == STATIC_GEM)
		{
			rewardColor = UI_INSTANCE_GEM_COLOR;
			goto ApplyDefaultRewardLight;
		}

		// crystal
		else if (createdModelID == STATIC_CRYSTAL)
		{
			rewardColor = UI_INSTANCE_CRYSTAL_COLOR;
			ui3D->lightDir.x = UI_INSTANCE_CRYSTAL_LIGHT_X;
			ui3D->lightDir.y = UI_INSTANCE_CRYSTAL_LIGHT_Y;
			ui3D->lightDir.z = UI_INSTANCE_CRYSTAL_LIGHT_Z;
			goto ApplyRewardColor;
		}

		// relic
		else if (createdModelID == STATIC_RELIC)
		{
			rewardColor = UI_INSTANCE_RELIC_COLOR;
			goto ApplyDefaultRewardLight;
		}

		// key
		else if (createdModelID == STATIC_KEY)
		{
			rewardColor = UI_INSTANCE_KEY_COLOR;
			goto ApplyDefaultRewardLight;
		}
		goto AfterRewardSetup;

	ApplyDefaultRewardLight:
		ui3D->lightDir.x = UI_INSTANCE_DEFAULT_LIGHT_X;
		ui3D->lightDir.y = UI_INSTANCE_DEFAULT_LIGHT_Y;
		ui3D->lightDir.z = UI_INSTANCE_DEFAULT_LIGHT_Z;
	ApplyRewardColor:
		inst->colorRGBA = rewardColor;
		inst->flags |= USE_SPECULAR_LIGHT;

	AfterRewardSetup:

		// if C-T-R letters
		if ((u32)(createdModelID - STATIC_C) < UI_INSTANCE_CTR_LETTER_COUNT)
		{
			// -4 for C
			// +0 for T
			// +4 for R
			ui3D->vel[0] = (createdModelID - STATIC_T) * UI_INSTANCE_CTR_LETTER_VEL_STEP;
			ui3D->vel[1] = UI_INSTANCE_CTR_LETTER_VEL_Y;

			// Set color
			inst->colorRGBA = UI_INSTANCE_CTR_COLOR;

			goto ApplyDefaultLightTransparent;
		}

		// token
		else if (createdModelID == STATIC_TOKEN)
		{
			// get AdvCup ID from level metadata
			int advCupID = data.metaDataLEV[gGT->levelID].ctrTokenGroupID;

			s16 *cupColor = &data.AdvCups[advCupID].color[0];

			inst->colorRGBA = (cupColor[0] << UI_INSTANCE_TOKEN_COLOR_R_SHIFT) | (cupColor[1] << UI_INSTANCE_TOKEN_COLOR_G_SHIFT) |
			                  (cupColor[2] << UI_INSTANCE_TOKEN_COLOR_B_SHIFT);

		ApplyDefaultLightTransparent:

			ui3D->lightDir.x = UI_INSTANCE_DEFAULT_LIGHT_X;
			ui3D->lightDir.y = UI_INSTANCE_DEFAULT_LIGHT_Y;
			ui3D->lightDir.z = UI_INSTANCE_DEFAULT_LIGHT_Z;

			inst->flags |= (DRAW_TRANSPARENT | USE_SPECULAR_LIGHT);
		}

		// if pushBuffer is not supplied,
		// which means this draws in Player pushBuffer
		if (pushBuffer == 0)
		{
			struct UiElement2D *currUI2D = &hudStruct[hudSlot];

			inst->matrix.t[0] = UI_ConvertX_2(currUI2D->x, currUI2D->z);
			inst->matrix.t[1] = UI_ConvertY_2(currUI2D->y, currUI2D->z);
			inst->matrix.t[2] = currUI2D->z;
		}

		// if pushBuffer is supplied,
		// for decalMP and fruitDisp
		else
		{
			struct InstDrawPerPlayer *idpp = INST_GETIDPP(inst);
			idpp[0].pushBuffer = (struct PushBuffer *)pushBuffer;

			inst->flags |= PUSHBUFFER_EXISTS;

			inst->matrix.t[0] = 0;
			inst->matrix.t[1] = 0;
			inst->matrix.t[2] = UI_INSTANCE_PUSHBUFFER_Z;
		}

		scale = hudStruct[hudSlot].scale;
		inst->scale.x = scale;
		inst->scale.y = scale;
		inst->scale.z = scale;

		inst->depthBiasNormal = UI_INSTANCE_DEPTH_BIAS;
		inst->depthBiasSecondary = UI_INSTANCE_DEPTH_BIAS;
		if (rotateToHud == UI_INSTANCE_ROTATE_NONE)
		{
			rot.x = 0;
		}
		else
		{
			s32 xRot = ratan2(inst->matrix.t[1], inst->matrix.t[2]);
			rot.x = -(s16)xRot;
		}
		rot.y = 0;
		rot.z = 0;

		// converted to TEST in rebuildPS1
		ConvertRotToMatrix(&ui3D->m, &rot);

		ui3D->rot.x = 0;
		ui3D->rot.y = 0;
		ui3D->rot.z = 0;
		ui3D->scale = UI_INSTANCE_SCALE;

		// next thread
		driverThread = driverThread->siblingThread;

		hudStruct += UI_HUD_SLOT_COUNT;
	}

	return inst;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004cec4-0x8004d614 for the retail path.
void UI_INSTANCE_InitAll(void)
{
	struct GameTracker *gGT;
	struct Instance *token;
	u32 gameMode1;
	u32 relicType;
	u32 relicTime;
	int i;

	gGT = sdata->gGT;
	sdata->menuReadyToPass &= ~UI_INSTANCE_MENU_READY_SHOW_MENU;
	gGT->renderFlags |= RENDER_FLAG_SPLIT_SCREEN_LINES;

	gameMode1 = gGT->gameMode1;

	// If you're in Crystal Challenge
	if ((gameMode1 & CRYSTAL_CHALLENGE) != 0)
	{
		sdata->ptrMenuCrystal = UI_INSTANCE_BirthWithThread(STATIC_CRYSTAL, (int)UI_ThTick_Reward, UI_HUD_SLOT_CRYSTAL, 0, 0, (int)rdata.s_crystal1);
		sdata->ptrHudCrystal = UI_INSTANCE_BirthWithThread(STATIC_CRYSTAL, (int)UI_ThTick_Reward, UI_HUD_SLOT_CRYSTAL, 0, 0, (int)rdata.s_crystal1);

		// Make a token
		sdata->ptrToken = UI_INSTANCE_BirthWithThread(STATIC_TOKEN, (int)UI_ThTick_Reward, UI_HUD_SLOT_TOKEN_OR_CTR, 0, 0, (int)sdata->s_token);

		// make Crystal invisible
#if defined(CTR_NATIVE)
		// NOTE(aalhendi): Menu-storage can carry CRYSTAL_CHALLENGE into tracks
		// that did not publish crystal HUD models.
		if (sdata->ptrHudCrystal != NULL)
#endif
		{
			sdata->ptrHudCrystal->flags |= HIDE_MODEL;
		}

		// make copy of Token pointer
		token = sdata->ptrToken;

#if defined(CTR_NATIVE)
		if (token == NULL)
		{
			return;
		}
#endif

		// set Token scale (x, y, z) to zero
		token->scale.x = 0;
		token->scale.y = 0;
		token->scale.z = 0;

		// make Token invisible
		token->flags |= HIDE_MODEL;
		return;
	}

	// If you're in Adventure Arena
	if ((gameMode1 & ADVENTURE_ARENA) != 0)
	{
		// is ignoring the return value of these calls intentional?
		UI_INSTANCE_BirthWithThread(STATIC_RELIC, (int)UI_ThTick_Reward, UI_HUD_SLOT_RELIC, 1, 0, (int)sdata->s_relic1);
		UI_INSTANCE_BirthWithThread(STATIC_KEY, (int)UI_ThTick_Reward, UI_HUD_SLOT_KEY, 1, 0, (int)sdata->s_key1);
		UI_INSTANCE_BirthWithThread(STATIC_TROPHY, (int)UI_ThTick_Reward, UI_HUD_SLOT_TROPHY, 0, 0, (int)sdata->s_trophy1);

		GAMEPROG_AdvPercent(&sdata->advProgress);

		return;
	}

	if ((gameMode1 & (RELIC_RACE | ADVENTURE_ARENA | TIME_TRIAL)) != 0)
	{
		for (i = 0; i < UI_INSTANCE_DRIVER_COUNT; i++)
		{
#if defined(CTR_NATIVE)
			// NOTE(aalhendi): PSX low-memory reads are non-fatal for unused driver slots.
			if (gGT->drivers[i] == NULL)
			{
				data.rankIconsCurr[i] = 0;
			}
			else
			{
#endif
				data.rankIconsCurr[i] = gGT->drivers[i]->driverRank;
			}

			// if more than 1 screen
			if (1 < gGT->numPlyrCurrGame)
			{
				data.rankIconsTransitionTimer[i] = UI_INSTANCE_RANK_TRANSITION_FRAMES;
			}
		}

		// If you're not in a Relic Race
		if ((gameMode1 & RELIC_RACE) == 0)
		{
			return;
		}

		// The rest of this block only happens in Relic Mode
		sdata->ptrRelic = UI_INSTANCE_BirthWithThread(STATIC_RELIC, (int)UI_ThTick_Reward, UI_HUD_SLOT_RELIC, 1, 0, (int)sdata->s_relic1);
		sdata->ptrTimebox1 = UI_INSTANCE_BirthWithThread(STATIC_TIME_CRATE_01, (int)UI_ThTick_CountPickup, UI_HUD_SLOT_TIMEBOX, 1, 0, (int)rdata.s_timebox1);

		// if instance
		if (sdata->ptrRelic != 0)
		{
			// set scale to zero
			sdata->ptrRelic->scale.z = 0;
			sdata->ptrRelic->scale.y = 0;
			sdata->ptrRelic->scale.x = 0;
		}

		// Get Relic Time to put in HUD
		if (
		    // no platinum and no gold
		    !CHECK_ADV_BIT(sdata->advProgress.rewards, gGT->levelID + ADV_REWARD_FIRST_PLATINUM_RELIC) &&
		    !CHECK_ADV_BIT(sdata->advProgress.rewards, gGT->levelID + ADV_REWARD_FIRST_GOLD_RELIC))
		{
			// 0 if sapphire not unlocked, (show sapphire)
			// 1 if sapphire is unlocked (show gold)
			relicType = CHECK_ADV_BIT(sdata->advProgress.rewards, gGT->levelID + ADV_REWARD_FIRST_SAPPHIRE_RELIC);
		}

		// if unlocked gold or unlocked platinum
		else
		{
			// put platinum time on screen
			relicType = UI_INSTANCE_RELIC_PLATINUM_TYPE;
		}

		// get relic time on this track, for this relic type (sapphire, gold, platinum)
		relicTime = data.RelicTime[gGT->levelID * UI_INSTANCE_RELIC_TYPE_COUNT + relicType];

		// store globally for HUD to access later
		sdata->relicTime_1min = relicTime / UI_INSTANCE_RELIC_TIME_UNITS_PER_MINUTE;
		sdata->relicTime_10sec = (relicTime / UI_INSTANCE_RELIC_TIME_UNITS_PER_10_SECONDS) % UI_INSTANCE_SECONDS_TENS_DIGIT_BASE;
		sdata->relicTime_1sec = (relicTime / UI_INSTANCE_RELIC_TIME_UNITS_PER_SECOND) % UI_INSTANCE_DECIMAL_DIGIT_BASE;
		sdata->relicTime_10ms = (relicTime / UI_INSTANCE_RELIC_TIME_UNITS_PER_TENTH_SECOND) % UI_INSTANCE_DECIMAL_DIGIT_BASE;
		sdata->relicTime_1ms = ((relicTime * UI_INSTANCE_RELIC_HUNDREDTH_SCALE) / UI_INSTANCE_RELIC_TIME_UNITS_PER_SECOND) % UI_INSTANCE_DECIMAL_DIGIT_BASE;

		return;
	}

	// used for multiplayer wumpa
	sdata->ptrPushBufferUI = (int)NULL;
	if (gGT->numPlyrCurrGame >= 2)
	{
		sdata->ptrPushBufferUI = (int)&sdata->pushBuffer_DecalMP;
	}

	sdata->pushBuffer_DecalMP.matrix_ViewProj = gGT->pushBuffer_UI.matrix_ViewProj;
	sdata->pushBuffer_DecalMP.pos = gGT->pushBuffer_UI.pos;
	sdata->pushBuffer_DecalMP.rect = gGT->pushBuffer_UI.rect;
	sdata->pushBuffer_DecalMP.ptrOT = gGT->pushBuffer_UI.ptrOT;
	sdata->pushBuffer_DecalMP.distanceToScreen_PREV = gGT->pushBuffer_UI.distanceToScreen_PREV;

	sdata->ptrFruitDisp = (int)UI_INSTANCE_BirthWithThread(STATIC_FRUITDISP, (int)UI_ThTick_CountPickup, UI_HUD_SLOT_FRUIT_MODEL, 1, sdata->ptrPushBufferUI,
	                                                       (int)rdata.s_fruitdisp);

	if ((gGT->numPlyrCurrGame < 3) &&

	    // If you're not in Battle Mode
	    ((gameMode1 & BATTLE_MODE) == 0))
	{
		UI_INSTANCE_BirthWithThread(STATIC_BIG1, (int)UI_ThTick_big1, UI_HUD_SLOT_BIG1, 0, 0, (int)sdata->s_big1);
	}

	// If you're not in Adventure Mode
	if ((gameMode1 & ADVENTURE_MODE) == 0)
	{
		return;
	}

	sdata->ptrHudC = UI_INSTANCE_BirthWithThread(STATIC_C, (int)UI_ThTick_CtrLetters, UI_HUD_SLOT_TOKEN_OR_CTR, 0, 0, (int)sdata->s_hudc);
	sdata->ptrHudT = UI_INSTANCE_BirthWithThread(STATIC_T, (int)UI_ThTick_CtrLetters, UI_HUD_SLOT_TOKEN_OR_CTR, 0, 0, (int)sdata->s_hudt);
	sdata->ptrHudR = UI_INSTANCE_BirthWithThread(STATIC_R, (int)UI_ThTick_CtrLetters, UI_HUD_SLOT_TOKEN_OR_CTR, 0, 0, (int)sdata->s_hudr);

	// Make a token
	sdata->ptrToken = UI_INSTANCE_BirthWithThread(STATIC_TOKEN, (int)UI_ThTick_Reward, UI_HUD_SLOT_TOKEN_OR_CTR, 0, 0, (int)sdata->s_token);

#if defined(CTR_NATIVE)
	// NOTE(aalhendi): PSX writes the hidden C/T/R flags through null HUD pointers in Garage; native cannot.
	if (sdata->ptrHudC != NULL)
#endif
	{
		sdata->ptrHudC->flags |= HIDE_MODEL;
	}
#if defined(CTR_NATIVE)
	if (sdata->ptrHudT != NULL)
#endif
	{
		sdata->ptrHudT->flags |= HIDE_MODEL;
	}
#if defined(CTR_NATIVE)
	if (sdata->ptrHudR != NULL)
#endif
	{
		sdata->ptrHudR->flags |= HIDE_MODEL;
	}

	// make copy of Token pointer
	token = sdata->ptrToken;

	// set Token scale (x, y, z) to zero
	token->scale.x = 0;
	token->scale.y = 0;
	token->scale.z = 0;

	// make Token invisible
	token->flags |= HIDE_MODEL;
	return;
}
