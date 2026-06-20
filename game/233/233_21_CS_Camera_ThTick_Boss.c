#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800ae9a8-0x800aed48
void CS_Camera_ThTick_Boss(struct Thread *t)
{
	char i;

	int cutsceneID;
	s16 levID;

	struct Instance *inst;
	struct CutsceneObj *cs;
	struct GameTracker *gGT;

	gGT = sdata->gGT;
	levID = gGT->levelID;

	// for undecided cutscene:
	//	hub*2+0 - intro (after winning trophy)
	//	hub*2+1 - outro (after winning key)
	if (D233.bossCutsceneIndex < 0)
	{
		cutsceneID = (levID - GEM_STONE_VALLEY) * 2;

		if (gGT->podiumRewardID == STATIC_KEY)
			cutsceneID++;
	}

	else
	{
		cutsceneID = D233.bossCutsceneIndex;
	}

	const struct BossCutsceneData *bcd = &R233.bossCS[cutsceneID];

	switch (D233.cutsceneState)
	{
	// Start Fade-to-black
	case CS_CAMERA_PAN:
	case CS_WAIT_INPUT:
		gGT->pushBuffer_UI.fadeFromBlack_desiredResult = 0;
		gGT->pushBuffer_UI.fade_step = -0x400;
		D233.cutsceneState = CS_FADE_OUT;
		break;

	// Wait for fade-to-black
	// Start loading process
	case CS_FADE_OUT:

		// wait for fade
		if (gGT->pushBuffer_UI.fadeFromBlack_currentValue != 0)
			break;

		// kill all podium "other" threads
		t = gGT->threadBuckets[OTHER].thread;
		while (t != 0)
		{
			t->flags |= THREAD_FLAG_DEAD;
			t = t->siblingThread;
		}

		// wait one frame, for the thread recycler to finish
		if (gGT->threadBuckets[OTHER].thread != 0)
			break;

		CS_LoadBoss(bcd);
		D233.cutsceneState = CS_LOADING;
		break;

	// Wait for loading callback,
	// start thread for head+body
	// start fade-to-normal
	case CS_LOADING:

		// NULLPTR checks if load finished,
		// because CS_LoadBossCallback writes this last
		if (D233.ptrModelBossHead == 0)
			break;

		struct Model **mArr = &D233.ptrModelBossHead;

		for (i = 0; i < 2; i++)
		{
			if (mArr[i] != NULL)
			{
				if (i != 0)
					mArr[i] = (struct Model *)((char *)mArr[i] + 4);

				gGT->modelPtr[mArr[i]->id] = mArr[i];
			}
		}

		MEMPACK_SwapPacks(gGT->activeMempackIndex);

		struct CsThreadInitData initData;
		initData.podiumPos.x = bcd->bossPos.x;
		initData.podiumPos.y = bcd->bossPos.y;
		initData.podiumPos.z = bcd->bossPos.z;
		initData.rot.x = bcd->bossRot.x;
		initData.rot.y = bcd->bossRot.y;
		initData.rot.z = bcd->bossRot.z;
		initData.characterPos.x = 0;
		initData.characterPos.y = 0;
		initData.characterPos.z = 0;

		// MUST go backwards,
		// Body first, sibling = 0
		// Head next, sibling = body
		t = 0;
		for (i = 1; i >= 0; i--)
		{
			if (mArr[i] == NULL)
				continue;

			t = CS_Thread_Init(mArr[i]->id, mArr[i], &initData, 0, t);
			if (t == NULL)
				continue;

			inst = t->inst;
			cs = t->object;

			// head
			if (i == 0)
			{
				CS_ScriptCmd_OpcodeAt(cs, bcd->opcode);
				cs->opcodeDuration = 0;
				continue;
			}

			// body
			inst->scale.x = 0x1000;
			inst->scale.y = 0x1000;
			inst->scale.z = 0x1000;
			cs->desiredScale = 0x1000;
		}

		// set camera position and rotation for cutscene
		gGT->pushBuffer[0].pos.x = bcd->camPos.x;
		gGT->pushBuffer[0].pos.y = bcd->camPos.y;
		gGT->pushBuffer[0].pos.z = bcd->camPos.z;

		gGT->pushBuffer[0].rot.x = bcd->camRot.x + 0x800;
		gGT->pushBuffer[0].rot.y = bcd->camRot.y;
		gGT->pushBuffer[0].rot.z = bcd->camRot.z;

		// fade back in
		gGT->pushBuffer_UI.fadeFromBlack_desiredResult = 0x1000;
		gGT->pushBuffer_UI.fade_step = 0x400;
		D233.cutsceneState = CS_FADE_IN;
		break;

	case CS_FADE_IN:

		// wait for fade
		if (gGT->pushBuffer_UI.fadeFromBlack_currentValue != 0x1000)
			break;

		D233.cutsceneState = CS_WAIT_END;
		break;

	case CS_WAIT_END:

		// wait for cutscene to end
		if (D233.isCutsceneOver != 1)
			break;

		gGT->podiumRewardID = NOFUNC; // 0
		t->flags |= THREAD_FLAG_DEAD;
	}
}
