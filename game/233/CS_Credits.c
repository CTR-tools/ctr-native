#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b8810-0x800b885c
char *CS_Credits_GetNextString(char *str)
{
#if defined(CTR_NATIVE)
	if (str == NULL)
	{
		// NOTE(aalhendi): Retail blindly reads the input pointer. Native
		// returns the same "no next string" result when credits epilogue text
		// reaches the end and the next pointer is PS1 null-space.
		return NULL;
	}
#endif

	char c = *str;
	while (c != '\0')
	{
		if (c == '\r')
		{
			return str + 1;
		}
		str++;
		c = *str;
	}
	if (*str != '\r')
	{
		return 0;
	}
	return str + 1;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b885c-0x800b88c8
void CS_Credits_DestroyCreditGhost(void)
{
	for (int i = 0; i < 5; i++)
	{
		INSTANCE_Death(creditsBSS.creditsObj.creditGhostInst[i]);
	}

	MEMPACK_ClearHighMem();
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b8668-0x800b8810
void CS_Credits_AnimateCreditGhost(struct Instance *dst, struct Instance *src, int index)
{
	struct CreditsObj *co = &creditsBSS.creditsObj;

	dst->animFrame = src->animFrame;
	dst->animIndex = src->animIndex;

	dst->matrix = src->matrix;

	s16 scale = 0x1000 + (index + 1) * 300;

	dst->scale.x = scale;
	dst->scale.y = scale;
	dst->scale.z = scale;

	dst->flags &= ~0x80;
	if ((int)dst->model == 0)
	{
		dst->flags |= 0x80;
	}

	dst->alphaScale = (index + 1) * 630;

	struct Model *localModel = (struct Model *)co->data_0x18_0x5[index].data;
	dst->model = localModel;

	struct Model *srcModel = src->model;
	int *dstModelInts = (int *)localModel;
	int *srcModelInts = (int *)srcModel;
	dstModelInts[0] = srcModelInts[0];
	dstModelInts[1] = srcModelInts[1];
	dstModelInts[2] = srcModelInts[2];
	dstModelInts[3] = srcModelInts[3];
	dstModelInts[4] = srcModelInts[4];
	dstModelInts[5] = srcModelInts[5];

	localModel->headers = (struct ModelHeader *)co->data_0x80_0x5[index].data;

	s16 srcNumHeaders = srcModel->numHeaders;
	if (srcNumHeaders > 0)
	{
		struct ModelHeader *dstHeaders = localModel->headers;
		struct ModelHeader *srcHeaders = srcModel->headers;

		for (int i = 0; i < srcNumHeaders; i++)
		{
			int *d = (int *)&dstHeaders[i];
			int *s = (int *)&srcHeaders[i];
			d[0] = s[0];
			d[1] = s[1];
			d[2] = s[2];
			d[3] = s[3];
			d[4] = s[4];
			d[5] = s[5];
			d[6] = s[6];
			d[7] = s[7];
			d[8] = s[8];
			d[9] = s[9];
			d[10] = s[10];
			d[11] = s[11];
			d[12] = s[12];
			d[13] = s[13];
			d[14] = s[14];
			d[15] = s[15];
		}
	}
}

// NOTE(aalhendi): Native copies of retail credits rdata names at
// 0x800b8644-0x800b8678.
static char cs_creditsRData[] = "credits\0creditghost\0credit strings";
static char *const cs_creditsThreadName = &cs_creditsRData[0];
static char *const cs_creditGhostName = &cs_creditsRData[8];

// NOTE(aalhendi): Retail stores the no-op return stub at 0x800b8f84 as the
// credits thread destroy callback.
static void CS_Credits_ThDestroy_NoOp(struct Thread *self)
{
	(void)self;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b8f8c-0x800b92a0
void CS_Credits_Init(void)
{
	int i;
	int bitIndex;
	struct Instance *inst;

	b32 boolAllGold;
	struct GameTracker *gGT;
	struct AdvProgress *advProg;
	struct CreditsObj *creditsObj;
	struct CreditsLevHeader *CLH;
	struct CreditsLevHeader *creditsDst;
	struct Thread *creditThread;

	gGT = sdata->gGT;
	advProg = &sdata->advProgress;
	creditsObj = &creditsBSS.creditsObj;

	void **pointers = ST1_GETPOINTERS(gGT->level1->ptrSpawnType1);
	CLH = pointers[ST1_CREDITS];

	creditsBSS.DancerThread = 0;

	creditsBSS.boolAllBlue = 1;
	boolAllGold = true;

	for (i = 0; i < 0x12; i++)
	{
		if (creditsBSS.boolAllBlue != 0)
		{
			bitIndex = i + ADV_REWARD_FIRST_SAPPHIRE_RELIC;
			creditsBSS.boolAllBlue = CHECK_ADV_BIT(advProg->rewards, bitIndex);
		}

		if (boolAllGold != 0)
		{
			bitIndex = i + ADV_REWARD_FIRST_GOLD_RELIC;
			boolAllGold = CHECK_ADV_BIT(advProg->rewards, bitIndex);
		}
	}

	if (boolAllGold != 0)
	{
		gGT->numWinners = 1;
		gGT->winnerIndex[0] = 0;
		gGT->confetti.numParticles_max = 250;
		gGT->confetti.unk2 = 250;
		gGT->renderFlags |= 4;
	}

	// 0 = size
	// 0 = no relation to param4
	// 0x300 = SmallStackPool
	// 0xd = "other" thread bucket
	creditThread = PROC_BirthWithObject(0x30d, CS_Credits_ThTick, cs_creditsThreadName, NULL);
	creditThread->funcThDestroy = CS_Credits_ThDestroy_NoOp;
	creditsBSS.CreditThread = creditThread;

	memset(creditsObj, 0, sizeof(struct CreditsObj));
	creditsObj->countdown = 360;

	// === 5 instances ===
	for (i = 0; i < 5; i++)
	{
		// STATIC_AKUAKU for some reason?
		inst = INSTANCE_Birth3D(gGT->modelPtr[STATIC_AKUAKU], cs_creditGhostName, creditThread);

		// save instance
		creditsObj->creditGhostInst[4 - i] = inst;

		inst->matrix.m[0][0] = 0x1000;
		inst->matrix.m[0][1] = 0;
		inst->matrix.m[0][2] = 0;
		inst->matrix.m[1][0] = 0;
		inst->matrix.m[1][1] = 0x1000;
		inst->matrix.m[1][2] = 0;
		inst->matrix.m[2][0] = 0;
		inst->matrix.m[2][1] = 0;
		inst->matrix.m[2][2] = 0x1000;

		inst->flags |= SCREENSPACE_INSTANCE;

		struct InstDrawPerPlayer *idpp = INST_GETIDPP(inst);
		idpp[0].pushBuffer = &gGT->pushBuffer_UI;

		for (int j = 1; j < gGT->numPlyrCurrGame; j++)
		{
			idpp[j].pushBuffer = NULL;
		}
	}

	creditsBSS.dancerInst_invisible = NULL;

	creditsDst = MEMPACK_AllocHighMem(CLH->size /* "credits strings" */);

	memcpy(creditsDst, CLH, CLH->size);

	creditsBSS.numStrings = creditsDst->numStrings;

	char **ptrStrings = (char **)CREDITSHEADER_GETSTRINGS(creditsDst);
	creditsBSS.ptrStrings = ptrStrings;

	for (i = 0; i < creditsBSS.numStrings; i++)
	{
		ptrStrings[i] = (char *)((u32)ptrStrings[i] + (u32)creditsDst);
	}

	creditsObj->credits_posY = 340;
	creditsObj->credits_topString = ptrStrings[0x14];
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b92a0-0x800b92cc
int CS_Credits_IsTextValid(void)
{
	struct CreditsObj *creditsObj = &creditsBSS.creditsObj;

	if (creditsObj->epilogue_topString != 0)
	{
		return 0;
	}

	creditsObj->countdown = 360;
	return 1;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b92cc-0x800b9398
void CS_Credits_NewDancer(struct Thread *dancerTh, int dancerModelID)
{
	struct CreditsObj *creditsObj = &creditsBSS.creditsObj;

	// kill any living thread
	struct Thread *oldDancerThread = creditsBSS.DancerThread;
	if (oldDancerThread != 0)
	{
		creditsBSS.DancerThread = 0;
		oldDancerThread->flags |= 0x800;
	}

	// store globally, make instance invisible
	creditsBSS.DancerThread = dancerTh;
	creditsBSS.dancerInst_invisible = dancerTh->inst;
	creditsBSS.dancerInst_invisible->flags |= HIDE_MODEL;

	creditsObj->countdown = 360;

	char **ptrStrings = creditsBSS.ptrStrings;

	// less than TAWNA1
	if (dancerModelID < STATIC_TAWNA1)
	{
		// subtract CRASHDANCE
		creditsObj->epilogue_topString = ptrStrings[dancerModelID - STATIC_CRASHDANCE];
	}

	// TAWNA
	else
	{
		// subtract an extra cause of GARAGE_TOP
		creditsObj->epilogue_topString = ptrStrings[(dancerModelID - STATIC_CRASHDANCE) - 1];
	}

	creditsObj->epilogueCount200 = 200;

	creditsObj->epilogue_nextString = CS_Credits_GetNextString(creditsObj->epilogue_topString);

	creditsObj->epiloguePosX = 0x200;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b9398-0x800b93f4
int CS_Credits_NewCreditGhosts(void)
{
	struct Model *model = creditsBSS.dancerInst_invisible->model;
	int i;

	for (i = 0; i < 5; i++)
	{
		if (creditsBSS.creditsObj.creditGhostModel[i] != model)
		{
			return 0;
		}
	}

	return 1;
}

#if defined(CTR_NATIVE)
static void CS_Credits_RestorePodiumAudioForNativeHandoff(void)
{
	if ((D233.CutsceneManipulatesAudio == 0) || (howl_VolumeGet(0) != 0) || (D233.FXVolumeBackup == 0))
	{
		return;
	}

	// NOTE(aalhendi): Boss scripts can mute HOWL with opcode 0x30; normal
	// podium exits restore it in CS_DestroyPodium_StartDriving. Credits do not
	// return to driving, and their scripts do not apply this mute. Native can
	// still reach this handoff with a leaked FX mute, so restore the podium
	// backup before loading the hub/Scrapbook. A zero backup preserves
	// user-muted SFX.
	howl_VolumeSet(0, (u8)D233.FXVolumeBackup);
	howl_VolumeSet(1, (u8)D233.MusicVolumeBackup);
	howl_VolumeSet(2, (u8)D233.VoiceVolumeBackup);
}
#endif

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b93f4-0x800b9488
void CS_Credits_End(void)
{
	int levID;
	struct GameTracker *gGT = sdata->gGT;

	// erase 5 instances
	CS_Credits_DestroyCreditGhost();

	// kill thread
	creditsBSS.CreditThread->flags |= 0x800;

#if defined(CTR_NATIVE)
	CS_Credits_RestorePodiumAudioForNativeHandoff();
#endif

	// go to gemstone valley
	if (creditsBSS.boolAllBlue == 0)
	{
		levID = GEM_STONE_VALLEY;

		gGT->gameMode1 |= ADVENTURE_MODE;
	}

	// go to scrapbook
	else
	{
		sdata->mainMenuState = MAIN_MENU_SCRAPBOOK;

		levID = SCRAPBOOK;
	}

	MainRaceTrack_RequestLoad(levID);

	gGT->renderFlags &= 0xfffffffb;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b88c8-0x800b8bd0
void CS_Credits_DrawNames(struct CreditsObj *co)
{
	if (co->credits_topString == 0)
	{
		return;
	}

	co->credits_posY--;

	if (co->credits_posY < -20)
	{
		co->credits_topString = CS_Credits_GetNextString(co->credits_topString);
		co->credits_posY += 20;
	}

	int posY = co->credits_posY;
	char *str = co->credits_topString;
	int charId = 0;

	while (posY < 0x114)
	{
		u16 textFlags = 0;

		if (*str == '~')
		{
			char *p = str + 2;

			do
			{
				int digit1 = (u8)p[-1] - 0x30;
				int digit2 = (u8)p[0];
				p += 3;
				int value = digit2 + digit1 * 10 - 0x30;
				str += 3;

				if (value < 0x32)
				{
					charId = value;
				}
				else if (value == 0x34)
				{
					textFlags |= 0x2000;
				}
				else if (value == 0x35)
				{
					textFlags |= 0x1000;
				}
			} while (*str == '~');
		}

		char *nextStr = CS_Credits_GetNextString(str);
		s16 strLen;

		if (nextStr == 0)
		{
			strLen = strlen(str);
		}
		else
		{
			strLen = (s16)(nextStr - str) - 1;
		}

		int clampedY = posY;
		int fadeAmount = 20;

		if (clampedY < 0x83)
		{
			if (clampedY < 0x14)
			{
				fadeAmount = clampedY;
			}
		}
		else
		{
			fadeAmount = 0x96 - clampedY;
		}

		int colorSlot = charId;

		if ((fadeAmount < 20) && (creditsBSS.boolAllBlue != 0))
		{
			if (fadeAmount < 1)
			{
				colorSlot = -1;
			}
			else
			{
				colorSlot = CREDITS_FADE;

				int fade8 = (fadeAmount << 8) / 20;
				char *src = (char *)data.ptrColor[charId];
				char *dst = (char *)&data.colors[CREDITS_FADE];

				for (int i = 0; i < 4; i++)
				{
					int stride = i * 4;
					dst[stride + 0] = (char)((u8)src[stride + 0] * fade8 >> 8);
					dst[stride + 1] = (char)((u8)src[stride + 1] * fade8 >> 8);
					dst[stride + 2] = (char)((u8)src[stride + 2] * fade8 >> 8);
				}
			}
		}

		if (colorSlot >= 0)
		{
			DecalFont_DrawLineStrlen((u8 *)str, strLen, creditsBSS.creditText_PosX, posY, 3, colorSlot | textFlags);
		}

		posY += 20;

		if (nextStr == 0)
		{
			return;
		}

		str = nextStr;
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b8bd0-0x800b8dc8
void CS_Credits_DrawEpilogue(struct CreditsObj *co)
{
	if (co->epilogue_topString == 0)
	{
		return;
	}

	co->epilogueCount200--;

	if (co->epilogueCount200 <= 0)
	{
		co->epilogueCount200 = 200;
		co->epilogue_topString = co->epilogue_nextString;
		co->epilogue_nextString = CS_Credits_GetNextString(co->epilogue_nextString);
	}

	if (co->epilogue_topString == 0)
	{
		return;
	}

	s16 timeRemaining = co->epilogueCount200;
	s16 fadeAmount = 20;

	if (timeRemaining < 0xb5)
	{
		if (timeRemaining <= 0x13)
		{
			fadeAmount = timeRemaining;
		}
	}
	else
	{
		fadeAmount = 200 - timeRemaining;
	}

	s16 colorSlot = 4;

	if (fadeAmount < 20)
	{
		if (fadeAmount > 0)
		{
			colorSlot = 0x1f;

			int fade8 = (fadeAmount << 8) / 20;
			char *dst = (char *)&data.colors[31];
			char *src = (char *)data.ptrColor[4];

			for (int i = 0; i < 4; i++)
			{
				int stride = i * 4;
				dst[stride + 0] = (char)((u8)src[stride + 0] * fade8 >> 8);
				dst[stride + 1] = (char)((u8)src[stride + 1] * fade8 >> 8);
				dst[stride + 2] = (char)((u8)src[stride + 2] * fade8 >> 8);
			}
		}
		else
		{
			colorSlot = -1;
		}
	}

	if ((colorSlot >= 0) && (creditsBSS.boolAllBlue != 0))
	{
		s16 strLen = -1;

		if (co->epilogue_nextString != 0)
		{
			strLen = (s16)(co->epilogue_nextString - co->epilogue_topString) - 1;
		}

		DecalFont_DrawMultiLineStrlen(co->epilogue_topString, strLen, 0x100, 0xaf, 0x1cc, 2, colorSlot | 0x8000);
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b8dc8-0x800b8f8c
void CS_Credits_ThTick(void)
{
	struct CreditsObj *co = &creditsBSS.creditsObj;
	struct Instance *danceInst = creditsBSS.dancerInst_invisible;

	co->creditDanceInst = danceInst;

	if (danceInst != NULL)
	{
		danceInst->flags |= HIDE_MODEL;

		danceInst->matrix.t[0] = (int)creditsBSS.creditGhost_Pos.x;
		danceInst->matrix.t[1] = (int)creditsBSS.creditGhost_Pos.y;
		danceInst->matrix.t[2] = (int)creditsBSS.creditGhost_Pos.z;

		struct GameTracker *gGT = sdata->gGT;

		if ((gGT->timer & 3) == 0)
		{
			for (int i = 4; i > 0; i--)
			{
				CS_Credits_AnimateCreditGhost(co->creditGhostInst[i], co->creditGhostInst[i - 1], i);
				co->creditGhostModel[i] = co->creditGhostModel[i - 1];
			}

			CS_Credits_AnimateCreditGhost(co->creditGhostInst[0], co->creditDanceInst, 0);
			co->creditGhostModel[0] = co->creditDanceInst->model;
		}
		else
		{
			CS_Credits_AnimateCreditGhost(co->creditGhostInst[0], co->creditDanceInst, 0);

			for (int i = 1; i < 5; i++)
			{
				struct Instance *ghost = co->creditGhostInst[i];
				ghost->scale.x += 0x4b;
				ghost->scale.y += 0x4b;
				ghost->scale.z += 0x4b;
				ghost->alphaScale += 0x9d;
			}
		}
	}

	if (co->countdown > 0)
	{
		co->countdown--;
	}

	CS_Credits_DrawNames(co);
	CS_Credits_DrawEpilogue(co);
}
