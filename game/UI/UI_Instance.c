#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004cae8-0x8004cec4.
// NOTE(native): param_2 (funcThTick), param_5 (pushBuffer), and param_6 (name)
// are real pointers carried through retail's int-sized parameters -- every
// call site truncated them with an `(int)` cast. Widened to their actual
// pointee types; PROC_BirthWithObject already required this for funcThTick.
struct Instance *UI_INSTANCE_BirthWithThread(int param_1, void *param_2, int param_3, int param_4, struct PushBuffer *param_5, const char *param_6)

{
	s16 modelID;
	s16 sVar2;
	s16 sVar3;
	s16 uVar5;
	struct Thread *hudThread;
	s32 lVar7;
	struct UiElement2D *hudStruct;
	int color;
	struct UiElement3D *ui3D;
	struct Instance *inst;
	struct Driver *driver;
	struct Thread *driverThread;
	struct Model *model;
	SVec3 rot;

	struct GameTracker *gGT;
	gGT = sdata->gGT;

	// get model pointer
	model = gGT->modelPtr[param_1];
	if (model == 0)
		return NULL;

	hudStruct = data.hudStructPtr[gGT->numPlyrCurrGame - 1];

	inst = 0;

	driverThread = gGT->threadBuckets[0].thread;

	while (driverThread != 0)
	{
		driver = driverThread->object;

		// Create a new thread for this HUD element
		// 0x38 = size
		// 0 = no relation to param4
		// 0x300 = SmallStackPool
		// 0x10 = hud thread bucket
		hudThread = PROC_BirthWithObject(0x380310, param_2, param_6, NULL);

		// Get the object attached to the thread
		ui3D = hudThread->object;

		// Big Number HUD element
		inst = INSTANCE_Birth2D(model, 0, hudThread);

		// give the Instance to the thread
		hudThread->inst = inst;

		modelID = model->id;

		// bigNum
		if (modelID == STATIC_BIG1)
		{
			driver->instBigNum = inst;
		}

		// fruitDisp
		else if (modelID == STATIC_FRUITDISP)
		{
			driver->instFruitDisp = inst;
		}

		// if this is a gem
		else if (modelID == STATIC_GEM)
		{
			color = 0x6c08080;
		LAB_8004cc4c:
			ui3D->lightDir.x = 0xf368;
			ui3D->lightDir.y = 0x99f;
			ui3D->lightDir.z = 0x232;
		LAB_8004cc58:
			inst->colorRGBA = color;

			inst->flags |= USE_SPECULAR_LIGHT;
		}

		// crystal
		else if (modelID == STATIC_CRYSTAL)
		{
			ui3D->lightDir.x = 0xf4a0;
			ui3D->lightDir.y = 0xb60;
			ui3D->lightDir.z = 0xfd28;
			color = 0xd22fff0;
			goto LAB_8004cc58;
		}

		// relic
		else if (modelID == STATIC_RELIC)
		{
			color = 0x60a5ff0;
			goto LAB_8004cc4c;
		}

		// key
		else if (modelID == STATIC_KEY)
		{
			color = 0xdca6000;
			goto LAB_8004cc4c;
		}

		// if C-T-R letters
		if ((u32)(modelID - 0x93U) < 3)
		{
			// -4 for C
			// +0 for T
			// +4 for R
			ui3D->vel[0] = (modelID - STATIC_T) * 4;
			ui3D->vel[1] = 0xc;

			// Set color
			inst->colorRGBA = 0xffc8000;

			goto lightDir_spec0x30000;
		}

		// token
		else if (modelID == STATIC_TOKEN)
		{
			// get AdvCup ID from level metadata
			int advCupID = data.metaDataLEV[gGT->levelID].ctrTokenGroupID;

			s16 *cupColor = &data.AdvCups[advCupID].color[0];

			inst->colorRGBA = (cupColor[0] << 0x14) | (cupColor[1] << 0xc) | (cupColor[2] << 4);

		lightDir_spec0x30000:

			ui3D->lightDir.x = 0xf368;
			ui3D->lightDir.y = 0x99f;
			ui3D->lightDir.z = 0x232;

			inst->flags |= (DRAW_TRANSPARENT | USE_SPECULAR_LIGHT);
		}

		// if pushBuffer is not supplied,
		// which means this draws in Player pushBuffer
		if (param_5 == 0)
		{
			struct UiElement2D *currUI2D;
			currUI2D = &hudStruct[param_3];

			inst->matrix.t[0] = UI_ConvertX_2(currUI2D->x, currUI2D->z);
			inst->matrix.t[1] = UI_ConvertY_2(currUI2D->y, currUI2D->z);
			inst->matrix.t[2] = currUI2D->z;
		}

		// if pushBuffer is supplied,
		// for decalMP and fruitDisp
		else
		{
			struct InstDrawPerPlayer *idpp = INST_GETIDPP(inst);
			idpp[0].pushBuffer = param_5;

			inst->flags |= PUSHBUFFER_EXISTS;

			inst->matrix.t[0] = 0;
			inst->matrix.t[1] = 0;
			inst->matrix.t[2] = 0x200;
		}

		uVar5 = hudStruct[param_3].scale;
		inst->scale.x = uVar5;
		inst->scale.y = uVar5;
		inst->scale.z = uVar5;

		inst->depthBiasNormal = 0x80;
		inst->depthBiasSecondary = 0x80;
		if (param_4 == 0)
		{
			rot.x = 0;
		}
		else
		{
			lVar7 = ratan2(inst->matrix.t[1], inst->matrix.t[2]);
			rot.x = -(s16)lVar7;
		}
		rot.y = 0;
		rot.z = 0;

		// converted to TEST in rebuildPS1
		ConvertRotToMatrix(&ui3D->m, &rot);

		ui3D->rot.x = 0;
		ui3D->rot.y = 0;
		ui3D->rot.z = 0;
		ui3D->scale = 0x1000;

		// next thread
		driverThread = driverThread->siblingThread;

		// TODO: use enum for hud elements
		hudStruct += 0x14;
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
	sdata->menuReadyToPass &= 0xfffffffe;
	gGT->renderFlags |= 0x8000;

	gameMode1 = gGT->gameMode1;

	// If you're in Crystal Challenge
	if ((gameMode1 & CRYSTAL_CHALLENGE) != 0)
	{
		sdata->ptrMenuCrystal = UI_INSTANCE_BirthWithThread(0x60, (void *)UI_ThTick_Reward, 0x11, 0, 0, rdata.s_crystal1);
		sdata->ptrHudCrystal = UI_INSTANCE_BirthWithThread(0x60, (void *)UI_ThTick_Reward, 0x11, 0, 0, rdata.s_crystal1);

		// Make a token
		sdata->ptrToken = UI_INSTANCE_BirthWithThread(0x7d, (void *)UI_ThTick_Reward, 0x12, 0, 0, sdata->s_token);

		// make Crystal invisible
#if defined(CTR_NATIVE)
		// NOTE(aalhendi): Menu-storage can carry CRYSTAL_CHALLENGE into tracks
		// that did not publish crystal HUD models.
		if (sdata->ptrHudCrystal != NULL)
#endif
			sdata->ptrHudCrystal->flags |= 0x80;

		// make copy of Token pointer
		token = sdata->ptrToken;

#if defined(CTR_NATIVE)
		if (token == NULL)
			return;
#endif

		// set Token scale (x, y, z) to zero
		token->scale.x = 0;
		token->scale.y = 0;
		token->scale.z = 0;

		// make Token invisible
		token->flags |= 0x80;
		return;
	}

	// If you're in Adventure Arena
	if ((gameMode1 & ADVENTURE_ARENA) != 0)
	{
		// is ignoring the return value of these calls intentional?
		UI_INSTANCE_BirthWithThread(0x61, (void *)UI_ThTick_Reward, 0xe, 1, 0, sdata->s_relic1);
		UI_INSTANCE_BirthWithThread(99, (void *)UI_ThTick_Reward, 0xf, 1, 0, sdata->s_key1);
		UI_INSTANCE_BirthWithThread(0x62, (void *)UI_ThTick_Reward, 0x10, 0, 0, sdata->s_trophy1);

		GAMEPROG_AdvPercent(&sdata->advProgress);

		return;
	}

	if ((gameMode1 & (RELIC_RACE | ADVENTURE_ARENA | TIME_TRIAL)) != 0)
	{
		for (i = 0; i < 8; i++)
		{
#if defined(CTR_NATIVE)
			// NOTE(aalhendi): PSX low-memory reads are non-fatal for unused driver slots.
			if (gGT->drivers[i] == NULL)
			{
				data.rankIconsCurr[i] = 0;
			}
			else
#endif
				data.rankIconsCurr[i] = gGT->drivers[i]->driverRank;

			// if more than 1 screen
			if (1 < gGT->numPlyrCurrGame)
			{
				data.rankIconsTransitionTimer[i] = 5;
			}
		}

		// If you're not in a Relic Race
		if ((gameMode1 & RELIC_RACE) == 0)
		{
			return;
		}

		// The rest of this block only happens in Relic Mode
		sdata->ptrRelic = UI_INSTANCE_BirthWithThread(0x61, (void *)UI_ThTick_Reward, 0xe, 1, 0, sdata->s_relic1);
		sdata->ptrTimebox1 = UI_INSTANCE_BirthWithThread(0x5c, (void *)UI_ThTick_CountPickup, 0x13, 1, 0, rdata.s_timebox1);

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
		    (CHECK_ADV_BIT(sdata->advProgress.rewards, gGT->levelID + ADV_REWARD_FIRST_PLATINUM_RELIC) == 0) &&
		    (CHECK_ADV_BIT(sdata->advProgress.rewards, gGT->levelID + ADV_REWARD_FIRST_GOLD_RELIC) == 0))
		{
			// 0 if sapphire not unlocked, (show sapphire)
			// 1 if sapphire is unlocked (show gold)
			relicType = CHECK_ADV_BIT(sdata->advProgress.rewards, gGT->levelID + ADV_REWARD_FIRST_SAPPHIRE_RELIC);
		}

		// if unlocked gold or unlocked platinum
		else
		{
			// put platinum time on screen
			relicType = 2;
		}

		// get relic time on this track, for this relic type (sapphire, gold, platinum)
		relicTime = data.RelicTime[gGT->levelID * 3 + relicType];

		// store globally for HUD to access later
		sdata->relicTime_1min = relicTime / 0xe100;
		sdata->relicTime_10sec = (relicTime / 0x2580) % 6;
		sdata->relicTime_1sec = (relicTime / 0x3c0) % 10;
		sdata->relicTime_10ms = ((relicTime * 100) / 0x3c0) % 10;
		sdata->relicTime_1ms = ((relicTime * 1000) / 0x3c0) % 10;

		return;
	}

	// used for multiplayer wumpa
	sdata->ptrPushBufferUI = 0;
	if (gGT->numPlyrCurrGame >= 2)
	{
		sdata->ptrPushBufferUI = (uintptr_t)&sdata->pushBuffer_DecalMP;
	}

	sdata->pushBuffer_DecalMP.matrix_ViewProj = gGT->pushBuffer_UI.matrix_ViewProj;
	sdata->pushBuffer_DecalMP.pos = gGT->pushBuffer_UI.pos;
	sdata->pushBuffer_DecalMP.rect = gGT->pushBuffer_UI.rect;
	sdata->pushBuffer_DecalMP.ptrOT = gGT->pushBuffer_UI.ptrOT;
	sdata->pushBuffer_DecalMP.distanceToScreen_PREV = gGT->pushBuffer_UI.distanceToScreen_PREV;

	sdata->ptrFruitDisp =
	    (uintptr_t)UI_INSTANCE_BirthWithThread(0x37, (void *)UI_ThTick_CountPickup, 3, 1, (struct PushBuffer *)sdata->ptrPushBufferUI, rdata.s_fruitdisp);

	if ((gGT->numPlyrCurrGame < 3) &&

	    // If you're not in Battle Mode
	    ((gameMode1 & BATTLE_MODE) == 0))
	{
		UI_INSTANCE_BirthWithThread(0x38, (void *)UI_ThTick_big1, 2, 0, 0, sdata->s_big1);
	}

	// If you're not in Adventure Mode
	if ((gameMode1 & ADVENTURE_MODE) == 0)
	{
		return;
	}

	sdata->ptrHudC = UI_INSTANCE_BirthWithThread(0x93, (void *)UI_ThTick_CtrLetters, 0x12, 0, 0, sdata->s_hudc);
	sdata->ptrHudT = UI_INSTANCE_BirthWithThread(0x94, (void *)UI_ThTick_CtrLetters, 0x12, 0, 0, sdata->s_hudt);
	sdata->ptrHudR = UI_INSTANCE_BirthWithThread(0x95, (void *)UI_ThTick_CtrLetters, 0x12, 0, 0, sdata->s_hudr);

	// Make a token
	sdata->ptrToken = UI_INSTANCE_BirthWithThread(0x7d, (void *)UI_ThTick_Reward, 0x12, 0, 0, sdata->s_token);

#if defined(CTR_NATIVE)
	// NOTE(aalhendi): PSX writes the hidden C/T/R flags through null HUD pointers in Garage; native cannot.
	if (sdata->ptrHudC != NULL)
#endif
		sdata->ptrHudC->flags |= 0x80;
#if defined(CTR_NATIVE)
	if (sdata->ptrHudT != NULL)
#endif
		sdata->ptrHudT->flags |= 0x80;
#if defined(CTR_NATIVE)
	if (sdata->ptrHudR != NULL)
#endif
		sdata->ptrHudR->flags |= 0x80;

	// make copy of Token pointer
	token = sdata->ptrToken;

	// set Token scale (x, y, z) to zero
	token->scale.x = 0;
	token->scale.y = 0;
	token->scale.z = 0;

	// make Token invisible
	token->flags |= 0x80;
	return;
}
