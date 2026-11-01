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


struct Instance *UI_INSTANCE_BirthWithThread(s32 modelID, s32 tickFunc, s32 hudSlot, b32 rotateToHud, s32 pushBuffer, s32 threadName)
{
	struct GameTracker *gGT = GAME_TRACKER;
	struct UiElement2D *playerLayouts = GAME_HUD_LAYOUTS[gGT->numPlyrCurrGame - 1];
	struct Model *model = gGT->modelPtr[modelID];
	struct Instance *lastInst = NULL;
	struct Thread *driverThread;
	SVec3 rot;

	if (model == 0)
	{
		return NULL;
	}

	driverThread = gGT->threadBuckets[PLAYER].thread;

	if (driverThread != 0)
	{
		// NOTE(aalhendi): Indexing the player group lets GCC reduce it to the
		// retail cursor without moving its setup ahead of the light constants.
		s32 playerIndex = 0;
		do
		{
			struct UiElement2D *hudStruct = playerLayouts + playerIndex * UI_HUD_SLOT_COUNT;
			struct Driver *driver = driverThread->object;
			struct UiElement3D *ui3D;
			struct Thread *hudThread;
			struct Instance *inst;

			hudThread = PROC_BirthWithObject(UI_INSTANCE_THREAD_FLAGS, (void *)tickFunc, (char *)threadName, NULL);

			// Get the object attached to the thread
			ui3D = hudThread->object;

			// Attach the model instance to the HUD thread.
			hudThread->inst = INSTANCE_Birth2D(model, 0, hudThread);
			inst = hudThread->inst;

			// bigNum
			if (model->id == STATIC_BIG1)
			{
				driver->instBigNum = inst;
			}

			// fruitDisp
			else if (model->id == STATIC_FRUITDISP)
			{
				driver->instFruitDisp = inst;
			}

			// if this is a gem
			if (model->id == STATIC_GEM)
			{
				ui3D->lightDir.x = UI_INSTANCE_DEFAULT_LIGHT_X;
				ui3D->lightDir.y = UI_INSTANCE_DEFAULT_LIGHT_Y;
				ui3D->lightDir.z = UI_INSTANCE_DEFAULT_LIGHT_Z;
				inst->colorRGBA = UI_INSTANCE_GEM_COLOR;
				inst->flags |= USE_SPECULAR_LIGHT;
			}

			// relic
			else if (model->id == STATIC_RELIC)
			{
				ui3D->lightDir.x = UI_INSTANCE_DEFAULT_LIGHT_X;
				ui3D->lightDir.y = UI_INSTANCE_DEFAULT_LIGHT_Y;
				ui3D->lightDir.z = UI_INSTANCE_DEFAULT_LIGHT_Z;
				inst->colorRGBA = UI_INSTANCE_RELIC_COLOR;
				inst->flags |= USE_SPECULAR_LIGHT;
			}

			// crystal
			else if (model->id == STATIC_CRYSTAL)
			{
				ui3D->lightDir.x = UI_INSTANCE_CRYSTAL_LIGHT_X;
				ui3D->lightDir.y = UI_INSTANCE_CRYSTAL_LIGHT_Y;
				ui3D->lightDir.z = UI_INSTANCE_CRYSTAL_LIGHT_Z;
				inst->colorRGBA = UI_INSTANCE_CRYSTAL_COLOR;
				inst->flags |= USE_SPECULAR_LIGHT;
			}

			// key
			else if (model->id == STATIC_KEY)
			{
				ui3D->lightDir.x = UI_INSTANCE_DEFAULT_LIGHT_X;
				ui3D->lightDir.y = UI_INSTANCE_DEFAULT_LIGHT_Y;
				ui3D->lightDir.z = UI_INSTANCE_DEFAULT_LIGHT_Z;
				inst->colorRGBA = UI_INSTANCE_KEY_COLOR;
				inst->flags |= USE_SPECULAR_LIGHT;
			}
			// if C-T-R letters
			else if ((u16)(model->id - STATIC_C) < UI_INSTANCE_CTR_LETTER_COUNT)
			{
				ui3D->lightDir.x = UI_INSTANCE_DEFAULT_LIGHT_X;
				ui3D->lightDir.y = UI_INSTANCE_DEFAULT_LIGHT_Y;
				ui3D->lightDir.z = UI_INSTANCE_DEFAULT_LIGHT_Z;
				ui3D->vel[1] = UI_INSTANCE_CTR_LETTER_VEL_Y;
				if (model->id == STATIC_C)
				{
					ui3D->vel[0] = -UI_INSTANCE_CTR_LETTER_VEL_STEP;
				}
				else if (model->id == STATIC_T)
				{
					ui3D->vel[0] = 0;
				}
				else if (model->id == STATIC_R)
				{
					ui3D->vel[0] = UI_INSTANCE_CTR_LETTER_VEL_STEP;
				}

				// Set color
				inst->colorRGBA = UI_INSTANCE_CTR_COLOR;

				inst->flags |= (DRAW_TRANSPARENT | USE_SPECULAR_LIGHT);
			}

			// token
			else if (model->id == STATIC_TOKEN)
			{
				// get AdvCup ID from level metadata
				s32 levelID = GAME_TRACKER->levelID;
				s16 advCupID = GAME_LEVEL_METADATA[levelID].ctrTokenGroupID;

				ui3D->lightDir.x = UI_INSTANCE_DEFAULT_LIGHT_X;
				ui3D->lightDir.y = UI_INSTANCE_DEFAULT_LIGHT_Y;
				ui3D->lightDir.z = UI_INSTANCE_DEFAULT_LIGHT_Z;

				inst->colorRGBA = ((u32)GAME_DATA_NAME(AdvCups)[advCupID].color[0] << UI_INSTANCE_TOKEN_COLOR_R_SHIFT) |
				                  ((u32)GAME_DATA_NAME(AdvCups)[advCupID].color[1] << UI_INSTANCE_TOKEN_COLOR_G_SHIFT) |
				                  ((u32)GAME_DATA_NAME(AdvCups)[advCupID].color[2] << UI_INSTANCE_TOKEN_COLOR_B_SHIFT);
				inst->flags |= (DRAW_TRANSPARENT | USE_SPECULAR_LIGHT);
			}

			// Explicit push buffers use fixed depth; otherwise use the player's HUD.
			if (pushBuffer != 0)
			{
				inst->idpp[0].pushBuffer = (struct PushBuffer *)pushBuffer;

				inst->flags |= PUSHBUFFER_EXISTS;

				lastInst = inst;
				lastInst->matrix.t[0] = 0;
				lastInst->matrix.t[1] = 0;
				lastInst->matrix.t[2] = UI_INSTANCE_PUSHBUFFER_Z;
			}
			else
			{
				struct UiElement2D *currUI2D = &hudStruct[hudSlot];
				inst->matrix.t[0] = UI_ConvertX_2(currUI2D->x, currUI2D->z);
				inst->matrix.t[1] = UI_ConvertY_2(currUI2D->y, currUI2D->z);
				inst->matrix.t[2] = currUI2D->z;
				lastInst = inst;
			}

			{
				struct UiElement2D *scaleLayout = hudStruct + hudSlot;
				inst->scale.x = scaleLayout->scale;
				inst->scale.y = scaleLayout->scale;
				inst->scale.z = scaleLayout->scale;
			}

			inst->depthBiasNormal = UI_INSTANCE_DEPTH_BIAS;
			inst->depthBiasSecondary = UI_INSTANCE_DEPTH_BIAS;
			if (rotateToHud != UI_INSTANCE_ROTATE_NONE)
			{
				s32 xRot = ratan2(inst->matrix.t[1], inst->matrix.t[2]);
				rot.x = -(s16)xRot;
				// NOTE(aalhendi): Complete each branch's rotation before the call;
				// GCC merges the zero stores without pulling arguments ahead of them.
				rot.y = 0;
				rot.z = 0;
			}
			else
			{
				rot.x = 0;
				rot.y = 0;
				rot.z = 0;
			}

			ConvertRotToMatrix(&ui3D->m, &rot);

			ui3D->rot.x = 0;
			ui3D->rot.y = 0;
			ui3D->rot.z = 0;
			ui3D->scale = UI_INSTANCE_SCALE;

			// next thread
			driverThread = driverThread->siblingThread;

			playerIndex++;
		} while (driverThread != 0);
	}

	return lastInst;
}

void UI_INSTANCE_InitAll(void)
{
	struct GameTracker *gGT;
	struct GameTracker *copyTracker;
	struct Instance *token;
	struct Instance *crystalToken;
	struct Instance *hide;
	struct Instance *hudC;
	struct Instance *hudT;
	struct Instance *hudR;
	struct Instance *relic;
	u32 gameMode1;
	s32 relicType;
	s32 i;

	gGT = GAME_TRACKER;
	GAME_SDATA_NAME(menuReadyToPass) &= ~UI_INSTANCE_MENU_READY_SHOW_MENU;
	gGT->renderFlags |= RENDER_FLAG_SPLIT_SCREEN_LINES;

	gameMode1 = gGT->gameMode1;

	// If you're in Crystal Challenge
	if ((gameMode1 & CRYSTAL_CHALLENGE) != 0)
	{
		GAME_SDATA_NAME(ptrMenuCrystal) =
		    UI_INSTANCE_BirthWithThread(STATIC_CRYSTAL, (s32)UI_ThTick_Reward, UI_HUD_SLOT_CRYSTAL, 0, 0, (s32)GAME_RDATA_NAME(s_crystal1));
		GAME_SDATA_NAME(ptrHudCrystal) =
		    UI_INSTANCE_BirthWithThread(STATIC_CRYSTAL, (s32)UI_ThTick_Reward, UI_HUD_SLOT_CRYSTAL, 0, 0, (s32)GAME_RDATA_NAME(s_crystal1));

		// Make a token
		GAME_SDATA_NAME(ptrToken) =
		    UI_INSTANCE_BirthWithThread(STATIC_TOKEN, (s32)UI_ThTick_Reward, UI_HUD_SLOT_TOKEN_OR_CTR, 0, 0, (s32)GAME_SDATA_NAME(s_token));
		hide = GAME_SDATA_NAME(ptrHudCrystal);

		// make Crystal invisible
#if defined(CTR_NATIVE)
		// NOTE(aalhendi): Menu-storage can carry CRYSTAL_CHALLENGE into tracks
		// that did not publish crystal HUD models.
		if (hide != NULL)
#endif
		{
			hide->flags |= HIDE_MODEL;
		}

		// make copy of Token pointer
		crystalToken = GAME_SDATA_NAME(ptrToken);

#if defined(CTR_NATIVE)
		if (crystalToken == NULL)
		{
			return;
		}
#endif

		// set Token scale (x, y, z) to zero
		crystalToken->scale.x = 0;
		crystalToken->scale.y = 0;
		crystalToken->scale.z = 0;

		// make Token invisible
		crystalToken->flags |= HIDE_MODEL;
		return;
	}

	// If you're in Adventure Arena
	if ((gameMode1 & ADVENTURE_ARENA) != 0)
	{
		// Birth the arena reward HUD elements.
		UI_INSTANCE_BirthWithThread(STATIC_RELIC, (s32)UI_ThTick_Reward, UI_HUD_SLOT_RELIC, 1, 0, (s32)GAME_SDATA_NAME(s_relic1));
		UI_INSTANCE_BirthWithThread(STATIC_KEY, (s32)UI_ThTick_Reward, UI_HUD_SLOT_KEY, 1, 0, (s32)GAME_SDATA_NAME(s_key1));
		UI_INSTANCE_BirthWithThread(STATIC_TROPHY, (s32)UI_ThTick_Reward, UI_HUD_SLOT_TROPHY, 0, 0, (s32)GAME_SDATA_NAME(s_trophy1));

		GAMEPROG_AdvPercent(&GAME_ADV_PROGRESS);

		return;
	}

	if ((gameMode1 & (RELIC_RACE | ADVENTURE_ARENA | TIME_TRIAL)) != 0)
	{
		goto InitRankIcons;
	}

	// used for multiplayer wumpa
	GAME_SDATA_NAME(ptrPushBufferUI) = gGT->numPlyrCurrGame >= 2 ? (s32)&GAME_SDATA_NAME(pushBuffer_DecalMP) : 0;

	GAME_SDATA_NAME(pushBuffer_DecalMP).matrix_ViewProj = GAME_TRACKER->pushBuffer_UI.matrix_ViewProj;
	copyTracker = GAME_TRACKER;
	GAME_SDATA_NAME(pushBuffer_DecalMP).pos.x = copyTracker->pushBuffer_UI.pos.x;
	GAME_SDATA_NAME(pushBuffer_DecalMP).pos.y = copyTracker->pushBuffer_UI.pos.y;
	GAME_SDATA_NAME(pushBuffer_DecalMP).pos.z = copyTracker->pushBuffer_UI.pos.z;
	GAME_SDATA_NAME(pushBuffer_DecalMP).rect.x = copyTracker->pushBuffer_UI.rect.x;
	GAME_SDATA_NAME(pushBuffer_DecalMP).rect.y = copyTracker->pushBuffer_UI.rect.y;
	GAME_SDATA_NAME(pushBuffer_DecalMP).rect.w = copyTracker->pushBuffer_UI.rect.w;
	GAME_SDATA_NAME(pushBuffer_DecalMP).rect.h = copyTracker->pushBuffer_UI.rect.h;
	GAME_SDATA_NAME(pushBuffer_DecalMP).ptrOT = copyTracker->pushBuffer_UI.ptrOT;
	GAME_SDATA_NAME(pushBuffer_DecalMP).distanceToScreen_PREV = copyTracker->pushBuffer_UI.distanceToScreen_PREV;

	GAME_SDATA_NAME(ptrFruitDisp) = (s32)UI_INSTANCE_BirthWithThread(STATIC_FRUITDISP, (s32)UI_ThTick_CountPickup, UI_HUD_SLOT_FRUIT_MODEL, 1,
	                                                                 GAME_SDATA_NAME(ptrPushBufferUI), (s32)GAME_RDATA_NAME(s_fruitdisp));

	if ((GAME_TRACKER->numPlyrCurrGame < 3) &&

	    // If you're not in Battle Mode
	    ((GAME_TRACKER->gameMode1 & BATTLE_MODE) == 0))
	{
		UI_INSTANCE_BirthWithThread(STATIC_BIG1, (s32)UI_ThTick_big1, UI_HUD_SLOT_BIG1, 0, 0, (s32)GAME_SDATA_NAME(s_big1));
	}

	// If you're not in Adventure Mode
	if ((GAME_TRACKER->gameMode1 & ADVENTURE_MODE) == 0)
	{
		return;
	}

	GAME_SDATA_NAME(ptrHudC) = UI_INSTANCE_BirthWithThread(STATIC_C, (s32)UI_ThTick_CtrLetters, UI_HUD_SLOT_TOKEN_OR_CTR, 0, 0, (s32)GAME_SDATA_NAME(s_hudc));
	GAME_SDATA_NAME(ptrHudT) = UI_INSTANCE_BirthWithThread(STATIC_T, (s32)UI_ThTick_CtrLetters, UI_HUD_SLOT_TOKEN_OR_CTR, 0, 0, (s32)GAME_SDATA_NAME(s_hudt));
	GAME_SDATA_NAME(ptrHudR) = UI_INSTANCE_BirthWithThread(STATIC_R, (s32)UI_ThTick_CtrLetters, UI_HUD_SLOT_TOKEN_OR_CTR, 0, 0, (s32)GAME_SDATA_NAME(s_hudr));

	// Make a token
	GAME_SDATA_NAME(ptrToken) = UI_INSTANCE_BirthWithThread(STATIC_TOKEN, (s32)UI_ThTick_Reward, UI_HUD_SLOT_TOKEN_OR_CTR, 0, 0, (s32)GAME_SDATA_NAME(s_token));
	hudC = GAME_SDATA_NAME(ptrHudC);

#if defined(CTR_NATIVE)
	// NOTE(aalhendi): PSX writes the hidden C/T/R flags through null HUD pointers in Garage; native cannot.
	if (hudC != NULL)
#endif
	{
		hudC->flags |= HIDE_MODEL;
	}
	hudT = GAME_SDATA_NAME(ptrHudT);
#if defined(CTR_NATIVE)
	if (hudT != NULL)
#endif
	{
		hudT->flags |= HIDE_MODEL;
	}
	hudR = GAME_SDATA_NAME(ptrHudR);
#if defined(CTR_NATIVE)
	if (hudR != NULL)
#endif
	{
		hudR->flags |= HIDE_MODEL;
	}

	// make copy of Token pointer
	token = GAME_SDATA_NAME(ptrToken);

	// set Token scale (x, y, z) to zero
	token->scale.x = 0;
	token->scale.y = 0;
	token->scale.z = 0;

	// make Token invisible
	token->flags |= HIDE_MODEL;
	return;

InitRankIcons:
	for (i = 0; i < UI_INSTANCE_DRIVER_COUNT; i++)
	{
#if defined(CTR_NATIVE)
		// NOTE(aalhendi): PSX low-memory reads are non-fatal for unused driver slots.
		if (gGT->drivers[i] == NULL)
		{
			GAME_DATA_NAME(rankIconsCurr)[i] = 0;
		}
		else
#endif
		{
			GAME_DATA_NAME(rankIconsCurr)[i] = gGT->drivers[i]->driverRank;
		}
		if (1 < gGT->numPlyrCurrGame)
		{
			GAME_DATA_NAME(rankIconsTransitionTimer)[i] = UI_INSTANCE_RANK_TRANSITION_FRAMES;
		}
	}
	if ((GAME_TRACKER->gameMode1 & RELIC_RACE) == 0)
	{
		return;
	}
	GAME_SDATA_NAME(ptrRelic) = UI_INSTANCE_BirthWithThread(STATIC_RELIC, (s32)UI_ThTick_Reward, UI_HUD_SLOT_RELIC, 1, 0, (s32)GAME_SDATA_NAME(s_relic1));
	GAME_SDATA_NAME(ptrTimebox1) =
	    UI_INSTANCE_BirthWithThread(STATIC_TIME_CRATE_01, (s32)UI_ThTick_CountPickup, UI_HUD_SLOT_TIMEBOX, 1, 0, (s32)GAME_RDATA_NAME(s_timebox1));
	relic = GAME_SDATA_NAME(ptrRelic);
	if (relic != 0)
	{
		relic->scale.z = 0;
		relic->scale.y = 0;
		relic->scale.x = 0;
	}
	if (CHECK_ADV_BIT(GAME_ADV_PROGRESS.rewards, GAME_TRACKER->levelID + ADV_REWARD_FIRST_PLATINUM_RELIC) ||
	    CHECK_ADV_BIT(GAME_ADV_PROGRESS.rewards, GAME_TRACKER->levelID + ADV_REWARD_FIRST_GOLD_RELIC))
	{
		relicType = UI_INSTANCE_RELIC_PLATINUM_TYPE;
	}
	else
	{
		relicType = CHECK_ADV_BIT(GAME_ADV_PROGRESS.rewards, GAME_TRACKER->levelID + ADV_REWARD_FIRST_SAPPHIRE_RELIC);
	}
	// Relic thresholds are signed; select this track's entry for each digit.
	GAME_SDATA_NAME(relicTime_1min) =
	    (GAME_DATA_NAME(RelicTime) + relicType)[GAME_TRACKER->levelID * UI_INSTANCE_RELIC_TYPE_COUNT] / UI_INSTANCE_RELIC_TIME_UNITS_PER_MINUTE;
	GAME_SDATA_NAME(relicTime_10sec) =
	    ((GAME_DATA_NAME(RelicTime) + relicType)[GAME_TRACKER->levelID * UI_INSTANCE_RELIC_TYPE_COUNT] / UI_INSTANCE_RELIC_TIME_UNITS_PER_10_SECONDS) %
	    UI_INSTANCE_SECONDS_TENS_DIGIT_BASE;
	GAME_SDATA_NAME(relicTime_1sec) =
	    ((GAME_DATA_NAME(RelicTime) + relicType)[GAME_TRACKER->levelID * UI_INSTANCE_RELIC_TYPE_COUNT] / UI_INSTANCE_RELIC_TIME_UNITS_PER_SECOND) %
	    UI_INSTANCE_DECIMAL_DIGIT_BASE;
	GAME_SDATA_NAME(relicTime_10ms) =
	    ((GAME_DATA_NAME(RelicTime) + relicType)[GAME_TRACKER->levelID * UI_INSTANCE_RELIC_TYPE_COUNT] / UI_INSTANCE_RELIC_TIME_UNITS_PER_TENTH_SECOND) %
	    UI_INSTANCE_DECIMAL_DIGIT_BASE;
	GAME_SDATA_NAME(relicTime_1ms) =
	    (((GAME_DATA_NAME(RelicTime) + relicType)[GAME_TRACKER->levelID * UI_INSTANCE_RELIC_TYPE_COUNT] * UI_INSTANCE_RELIC_HUNDREDTH_SCALE) /
	     UI_INSTANCE_RELIC_TIME_UNITS_PER_SECOND) %
	    UI_INSTANCE_DECIMAL_DIGIT_BASE;
}
