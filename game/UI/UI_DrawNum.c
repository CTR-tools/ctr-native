#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80050c20-0x80050e6c.
void UI_DrawNumWumpa(s16 posX, s16 posY, struct Driver *d)
{
	s8 currWumpa;
	int currWumpa10s;
	int i;
	int iconID;
	struct GameTracker *gGT;

	char message[16];

	gGT = sdata->gGT;

	// if numPlyrCurrGame is less than 3
	if (gGT->numPlyrCurrGame < 3)
	{
		// Draw 'x' before drawing number of wumpa
		DecalFont_DrawLine(&sdata->s_x[0], (int)posX, posY + 4, FONT_SMALL, ORANGE);

		// Draw number after the 'x'
		sprintf(message, &sdata->s_int[0], d->numWumpas);
		DecalFont_DrawLine(message, posX + 0xd, (int)posY, FONT_BIG, ORANGE);
	}

	// if numPlyrCurrGame is 3 or 4
	else
	{
		// amount of wumpa
		currWumpa = d->numWumpas;

		// amount of wumpa / 10
		currWumpa10s = (currWumpa / 10) * 0x1000000 >> 0x18;

		// draw 2 digits
		for (i = 0; i < 2; i++)
		{
			iconID = currWumpa10s;
			if (i > 0)
			{
				iconID = currWumpa + currWumpa10s * -10;
			}

			struct Icon **iconPtrArray = ICONGROUP_GETICONS(gGT->iconGroup[5]);

			DecalHUD_DrawPolyGT4(

			    iconPtrArray[iconID],

			    posX + 0xc * i, posY,

			    // pointer to PrimMem struct
			    &gGT->backBuffer->primMem,

			    // pointer to OT memory
			    gGT->pushBuffer_UI.ptrOT,

			    // color data
			    data.ptrColor[ORANGE][0], data.ptrColor[ORANGE][1], data.ptrColor[ORANGE][2], data.ptrColor[ORANGE][3],

			    0, FP(1.0));
		}
	}
	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80050e6c-0x80050f18.
void UI_DrawNumTimebox(s16 posX, s16 posY, struct Driver *d)
{
	int numCratesOwned;
	int numCratesTotal;
	struct GameTracker *gGT;
	char string[8];

	DecalFont_DrawLine(&sdata->s_x[0], posX + 0x14, posY - 10, FONT_SMALL, ORANGE);

	gGT = sdata->gGT;
	numCratesOwned = d->numTimeCrates;
	numCratesTotal = gGT->timeCratesInLEV;

	sprintf(&string[0], "%2.02d/%ld", numCratesOwned, CTR_PRINTF_PSX_LONG(numCratesTotal));

	DecalFont_DrawLine(&string[0], posX + 0x21, posY - 0xe, FONT_BIG, ORANGE);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80050f18-0x80050fc4.
void UI_DrawNumRelic(s16 posX, s16 posY)
{
	int num;
	struct GameTracker *gGT;
	char string[8];

	DecalFont_DrawLine(&sdata->s_x[0], posX, posY + 4, FONT_SMALL, ORANGE);

	gGT = sdata->gGT;
	num = gGT->currAdvProfile.numRelics;
	if ((gGT->gameMode2 & INC_RELIC) != 0)
	{
		num--;
	}

	sprintf(&string[0], &sdata->s_longInt[0], num);
	DecalFont_DrawLine(&string[0], posX + 0xD, posY, FONT_BIG, ORANGE);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80050fc4-0x80051070.
void UI_DrawNumKey(s16 posX, s16 posY)
{
	int num;
	struct GameTracker *gGT;
	char string[8];

	DecalFont_DrawLine(&sdata->s_x[0], posX, posY + 4, FONT_SMALL, ORANGE);

	gGT = sdata->gGT;
	num = gGT->currAdvProfile.numKeys;
	if ((gGT->gameMode2 & INC_KEY) != 0)
	{
		num--;
	}

	sprintf(&string[0], &sdata->s_longInt[0], num);
	DecalFont_DrawLine(&string[0], posX + 0xD, posY, FONT_BIG, ORANGE);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80051070-0x8005111c.
void UI_DrawNumTrophy(s16 posX, s16 posY)
{
	int num;
	struct GameTracker *gGT;
	char string[8];

	DecalFont_DrawLine(&sdata->s_x[0], posX, posY + 4, FONT_SMALL, ORANGE);

	gGT = sdata->gGT;
	num = gGT->currAdvProfile.numTrophies;
	if ((gGT->gameMode2 & INC_TROPHY) != 0)
	{
		num--;
	}

	sprintf(&string[0], &sdata->s_longInt[0], num);
	DecalFont_DrawLine(&string[0], posX + 0xD, posY, FONT_BIG, ORANGE);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8005111c-0x800511c0.
void UI_DrawNumCrystal(s16 posX, s16 posY, struct Driver *d)
{
	int numCrystalsOwned;
	int numCrystalsTotal;
	struct GameTracker *gGT;
	char string[8];

	DecalFont_DrawLine(&sdata->s_x[0], posX, posY + 4, FONT_SMALL, ORANGE);

	gGT = sdata->gGT;
	numCrystalsOwned = d->numCrystals;
	numCrystalsTotal = gGT->numCrystalsInLEV;

	sprintf(&string[0], "%2.02d/%ld", numCrystalsOwned, CTR_PRINTF_PSX_LONG(numCrystalsTotal));

	DecalFont_DrawLine(&string[0], posX + 0xD, posY, FONT_BIG, ORANGE);
}
