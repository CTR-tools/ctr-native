#include <common.h>

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
void DECOMP_CS_Credits_Init(void)
{
	int i;
	int bitIndex;
	struct Instance *inst;

	int boolAllGold;
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
	boolAllGold = 1;

	for (i = 0; i < 0x12; i++)
	{
		if (creditsBSS.boolAllBlue != 0)
		{
			bitIndex = i + 0x16;
			creditsBSS.boolAllBlue = CHECK_ADV_BIT(advProg->rewards, bitIndex);
		}

		if (boolAllGold != 0)
		{
			bitIndex = i + 0x28;
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
	creditThread = DECOMP_PROC_BirthWithObject(0x30d, CS_Credits_ThTick, cs_creditsThreadName, NULL);
	creditThread->funcThDestroy = CS_Credits_ThDestroy_NoOp;
	creditsBSS.CreditThread = creditThread;

	memset(creditsObj, 0, sizeof(struct CreditsObj));
	creditsObj->countdown = 360;

	// === 5 instances ===
	for (i = 0; i < 5; i++)
	{
		// STATIC_AKUAKU for some reason?
		inst = DECOMP_INSTANCE_Birth3D(gGT->modelPtr[STATIC_AKUAKU], cs_creditGhostName, creditThread);

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

		inst->flags |= 0x400;

		struct InstDrawPerPlayer *idpp = INST_GETIDPP(inst);
		idpp[0].pushBuffer = &gGT->pushBuffer_UI;

		for (int j = 1; j < gGT->numPlyrCurrGame; j++)
		{
			idpp[j].pushBuffer = NULL;
		}
	}

	creditsBSS.dancerInst_invisible = NULL;

	creditsDst = DECOMP_MEMPACK_AllocHighMem(CLH->size /* "credits strings" */);

	memcpy(creditsDst, CLH, CLH->size);

	creditsBSS.numStrings = creditsDst->numStrings;

	char **ptrStrings = (char **)CREDITSHEADER_GETSTRINGS(creditsDst);
	creditsBSS.ptrStrings = ptrStrings;

	for (i = 0; i < creditsBSS.numStrings; i++)
	{
		ptrStrings[i] = (char *)((unsigned int)ptrStrings[i] + (unsigned int)creditsDst);
	}

	creditsObj->credits_posY = 340;
	creditsObj->credits_topString = ptrStrings[0x14];
}
