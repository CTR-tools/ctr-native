#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80054a08-0x80054a78
u32 UI_VsQuipReadDriver(struct Driver *d, int offset, int size)
{
	char *data = (char *)d + offset;

	if (size == 2)
	{
		return *(s16 *)data;
	}

	if (size < 3)
	{
		if (size == 1)
		{
			return *(u8 *)data;
		}

		return 0;
	}

	if (size == 4)
	{
		return *(u32 *)data;
	}

	return 0;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80054a78-0x80054bfc
void UI_VsQuipAssign(struct Driver *driver, struct QuipMeta *meta, struct Driver *bestDriver, int characterID)
{
	if (driver == NULL)
	{
		return;
	}

	if (((meta->flags & 4) != 0) && (driver != bestDriver))
	{
		return;
	}

	struct QuipStr *selected = meta->ptrQuipStrCurr;

	for (struct QuipStr *curr = selected + 1; curr < meta->ptrQuipStrNext; curr++)
	{
		if (selected->priority < curr->priority)
		{
			selected = curr;
			continue;
		}

		if ((curr->priority == selected->priority) && (((MixRNG_Scramble() >> 3) & 0xff) < 0x40))
		{
			selected = curr;
		}
	}

	struct QuipStr *oldQuip = (struct QuipStr *)driver->EndOfRaceComment_ptrQuip;

	if (oldQuip != NULL)
	{
		int oldPriority = oldQuip->priority;

		if (selected->priority <= oldPriority)
		{
			if (selected->priority != oldPriority)
			{
				return;
			}

			if (((MixRNG_Scramble() >> 3) & 0xff) > 0x3f)
			{
				return;
			}
		}

		if (oldQuip->priority < 0)
		{
			oldQuip->priority = -oldQuip->priority;
		}
	}

	if (selected->priority > 0)
	{
		selected->priority = -selected->priority;
	}

	driver->EndOfRaceComment_ptrQuip = (s16 *)selected;
	driver->EndOfRaceComment_characterID = characterID;

	sdata->gGT->timerEndOfRaceVS = ((sdata->gGT->gameMode1 & BATTLE_MODE) != 0) ? 150 : 300;
}

#define UI_QUIP_DATA_BASE_PSX 0x800864dcu
#define UI_QUIP_VS_META_OFF   0x170u
#define UI_QUIP_VS_META_END   0x518u
#define UI_QUIP_BAT_META_OFF  0x730u
#define UI_QUIP_BAT_META_END  0x850u

struct QuipMetaRaw
{
	u32 ptrQuipStrCurr;
	u32 ptrQuipStrNext;
	s16 conditionType;
	s16 flags;
	int threshold;
	int driverOffset;
	int dataSize;
};

CTR_STATIC_ASSERT(sizeof(struct QuipMetaRaw) == 0x18);

static u8 *UI_VsQuipData(void)
{
#if BUILD >= JpnTrial
	return data.data830;
#else
	return data.data850;
#endif
}

static struct QuipStr *UI_VsQuipPtrFromPsx(u32 psxAddr)
{
	return (struct QuipStr *)(void *)(UI_VsQuipData() + (psxAddr - UI_QUIP_DATA_BASE_PSX));
}

static struct QuipMeta UI_VsQuipMetaFromRaw(struct QuipMetaRaw *raw)
{
	struct QuipMeta meta;

	meta.ptrQuipStrCurr = UI_VsQuipPtrFromPsx(raw->ptrQuipStrCurr);
	meta.ptrQuipStrNext = UI_VsQuipPtrFromPsx(raw->ptrQuipStrNext);
	meta.conditionType = raw->conditionType;
	meta.flags = raw->flags;
	meta.threshold = raw->threshold;
	meta.driverOffset = raw->driverOffset;
	meta.dataSize = raw->dataSize;

	return meta;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80054bfc-0x800550f4
void UI_VsQuipAssignAll(void)
{
	struct GameTracker *gGT = sdata->gGT;
	int characterID = 0;

	if (gGT->numPlyrCurrGame < 2)
	{
		return;
	}

	u8 *quipData = UI_VsQuipData();
	u32 metaStartOff = UI_QUIP_VS_META_OFF;
	u32 metaEndOff = UI_QUIP_VS_META_END;

	if ((gGT->gameMode1 & BATTLE_MODE) != 0)
	{
		metaStartOff = UI_QUIP_BAT_META_OFF;
		metaEndOff = UI_QUIP_BAT_META_END;
	}

	struct QuipMetaRaw *metaStart = (struct QuipMetaRaw *)(void *)(quipData + metaStartOff);
	struct QuipMetaRaw *metaEnd = (struct QuipMetaRaw *)(void *)(quipData + metaEndOff);

	for (struct QuipMetaRaw *raw = metaStart; raw < metaEnd; raw++)
	{
		struct QuipStr *curr = UI_VsQuipPtrFromPsx(raw->ptrQuipStrCurr);
		struct QuipStr *next = UI_VsQuipPtrFromPsx(raw->ptrQuipStrNext);

		while (curr < next)
		{
			curr->priority++;
			curr++;
		}
	}

	struct Driver *bestDriver = NULL;
	int bestScore = 0;
	int secondScore = 0;
	int scoreByDriverID[8];

	struct Thread *thread = gGT->threadBuckets[PLAYER].thread;

	while (thread != NULL)
	{
		struct Driver *driver = thread->object;

		driver->EndOfRaceComment_ptrQuip = NULL;

		int score;

		if ((gGT->gameMode1 & POINT_LIMIT) == 0)
		{
			score = driver->BattleHUD.numLives;
		}
		else
		{
			score = gGT->battleSetup.pointsPerTeam[driver->BattleHUD.teamID];
		}

		scoreByDriverID[driver->driverID] = score;

		if (score > bestScore)
		{
			secondScore = bestScore;
			bestDriver = driver;
			bestScore = score;
		}
		else if (score == bestScore)
		{
			secondScore = bestScore;
			bestDriver = NULL;
		}

		thread = thread->siblingThread;
	}

	for (struct QuipMetaRaw *raw = metaStart; raw < metaEnd; raw++)
	{
		struct QuipMeta meta = UI_VsQuipMetaFromRaw(raw);
		struct Driver *selectedDriver = NULL;
		int threshold = meta.threshold;
		u32 bestValue = (meta.conditionType == 5) ? 0x7fffffff : 0;

		if ((meta.flags & 1) != 0)
		{
			int numLaps = (s8)gGT->numLaps;

			if (numLaps < 0)
			{
				numLaps = -numLaps;
			}

			threshold *= numLaps;
		}

		thread = gGT->threadBuckets[PLAYER].thread;

		while (thread != NULL)
		{
			struct Driver *driver = thread->object;
			struct Driver *nextSelectedDriver = selectedDriver;
			int nextThreshold = threshold;
			u32 nextBestValue = bestValue;

			switch (meta.conditionType)
			{
			case 0:
			{
				int value = UI_VsQuipReadDriver(driver, meta.driverOffset, meta.dataSize);

				nextSelectedDriver = driver;
				nextThreshold = value;

				if (value <= threshold)
				{
					nextSelectedDriver = selectedDriver;
					nextThreshold = threshold;

					if (value == threshold)
					{
						nextSelectedDriver = NULL;
					}
				}
				break;
			}

			case 1:
			{
				int value = UI_VsQuipReadDriver(driver, meta.driverOffset, meta.dataSize);

				if (value >= 0)
				{
					nextSelectedDriver = driver;
					nextThreshold = value;

					if (threshold <= value)
					{
						nextSelectedDriver = selectedDriver;
						nextThreshold = threshold;

						if (value == threshold)
						{
							nextSelectedDriver = NULL;
						}
					}
				}
				break;
			}

			case 3:
			{
				for (int i = 0; i < 8; i++)
				{
					u32 value = (u8)driver->numTimesAttackedByPlayer[i];
					int delta = value - bestValue;

					if (threshold < delta)
					{
						characterID = i;
						selectedDriver = driver;
						bestValue = value;
					}
					else if (-threshold < delta)
					{
						selectedDriver = NULL;

						if ((int)bestValue < (int)value)
						{
							bestValue = value;
						}
					}
				}

				nextSelectedDriver = selectedDriver;
				nextBestValue = bestValue;
				break;
			}

			case 4:
			{
				u32 value = UI_VsQuipReadDriver(driver, meta.driverOffset, meta.dataSize);
				int delta = value - bestValue;

				if (threshold < delta)
				{
					nextSelectedDriver = driver;
					nextBestValue = value;
				}
				else
				{
					nextSelectedDriver = selectedDriver;
					nextBestValue = bestValue;

					if (-threshold < delta)
					{
						nextSelectedDriver = NULL;

						if ((int)bestValue < (int)value)
						{
							nextBestValue = value;
						}
					}
				}
				break;
			}

			case 5:
			{
				u32 value = UI_VsQuipReadDriver(driver, meta.driverOffset, meta.dataSize);
				int delta = bestValue - value;

				if ((int)value >= 0)
				{
					if (threshold < delta)
					{
						nextSelectedDriver = driver;
						nextBestValue = value;
					}
					else
					{
						nextSelectedDriver = selectedDriver;
						nextBestValue = bestValue;

						if (-threshold < delta)
						{
							nextSelectedDriver = NULL;

							if ((int)value < (int)bestValue)
							{
								nextBestValue = value;
							}
						}
					}
				}
				break;
			}

			case 6:
			{
				u32 value = UI_VsQuipReadDriver(driver, meta.driverOffset, meta.dataSize);

				if (value == (u8)driver->numTimesAttacking)
				{
					nextSelectedDriver = driver;
				}
				break;
			}

			case 7:
				if (threshold == 0)
				{
					if (driver == bestDriver)
					{
						nextSelectedDriver = bestDriver;
					}
				}
				else if ((threshold == 1) && (secondScore != 0) && (scoreByDriverID[driver->driverID] == secondScore))
				{
					nextSelectedDriver = driver;
				}
				break;

			case 8:
			{
				int value = UI_VsQuipReadDriver(driver, meta.driverOffset, meta.dataSize);

				if (value == threshold)
				{
					nextSelectedDriver = driver;
				}
				break;
			}

			case 9:
				if (driver->EndOfRaceComment_ptrQuip == NULL)
				{
					UI_VsQuipAssign(driver, &meta, bestDriver, 0);
				}
				break;

			default:
				break;
			}

			if ((nextSelectedDriver != NULL) && ((meta.flags & 0xc) != 0))
			{
				UI_VsQuipAssign(nextSelectedDriver, &meta, bestDriver, characterID);
			}

			thread = thread->siblingThread;
			selectedDriver = nextSelectedDriver;
			threshold = nextThreshold;
			bestValue = nextBestValue;
		}

		UI_VsQuipAssign(selectedDriver, &meta, bestDriver, characterID);
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800550f4-0x800552a4.
void UI_VsQuipDrawAll(void)
{
	enum
	{
		UI_VS_QUIP_FORMAT_BUFFER_SIZE = 0x80
	};

	char *print;
	char printBuffer[UI_VS_QUIP_FORMAT_BUFFER_SIZE];
	s16 *printArr;
	struct Driver *d;

	struct GameTracker *gGT = sdata->gGT;
	struct Thread *thread;

	// loop through player threads
	int playerIndex = 0;
	for (thread = gGT->threadBuckets[0].thread; thread != 0; thread = thread->siblingThread, playerIndex++)
	{
		// get player struct from thread
		d = (struct Driver *)thread->object;

		// If driver already pressed X to continue
		if ((sdata->Battle_EndOfRace.boolPressX[playerIndex] & 2) != 0)
		{
			continue;
		}

		// This is secretly a s16[2], to hold a config bit
		printArr = (s16 *)d->EndOfRaceComment_ptrQuip;

		if (printArr == 0)
		{
			continue;
		}

		printArr[2] = 0;
		printArr[3] = 0;

		// if this is only one comment
		if ((printArr[1] & 1) == 0)
		{
			// Print the string as a comment
			print = sdata->lngStrings[printArr[0]];
		}

		// if the comment is conjoined
		else
		{
			// Add two strings together
			sprintf(printBuffer,

			        // Contains '%s' format:
			        // Original end-of-race comment
			        sdata->lngStrings[printArr[0]],

			        // second part of comment,
			        // lngIndex of driver,
			        // for stuff like "hit by Crash Bandicoot" or something
			        sdata->lngStrings[data.MetaDataCharacters[d->EndOfRaceComment_characterID].name_LNG_long]);

			print = printBuffer;
		}

		// get current player's pushBuffer
		RECT *r = &gGT->pushBuffer[playerIndex].rect;

		// Draw the string with a box around it
		RECTMENU_DrawQuip(print,
		                  (r->x + (r->w >> 1)), // 50% width
		                  (r->y + (r->h >> 3)), // 12% height
		                  0, 3, 0xffff8000, 4);
	}
}

// Retail 0x800116ec: Battle end stat positions for 3P/4P.
static const SVec2 s_battleStatsPos3P4P[4] = {
    {0x55, 0x35},
    {0xaa, 0x35},
    {0x55, 0x43},
    {0xaa, 0x43},
};

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800552a4-0x8005572c.
void UI_VsWaitForPressX(void)
{
	char i, j;
	u8 numAttacked;
	s16 sVar4;
	int string;
	int local_78;
	char statText[8];
	Color clearColor;
	RECT clearRect;

	RECT *r;
	struct Driver *currDriver;

	struct GameTracker *gGT = sdata->gGT;
	char numPlyr = gGT->numPlyrCurrGame;

	int tap;
	int ready = 0;
	char *pressedX;

	for (i = 0; i < numPlyr; i++)
	{
		// flags, for which players have pressed X to continue
		pressedX = &sdata->Battle_EndOfRace.boolPressX[i];

		currDriver = gGT->drivers[i];
		r = &gGT->pushBuffer[i].rect;
		tap = sdata->gGamepads->gamepad[i].buttonsTapped;

		// If Player has not pressed X to continue
		// Draw comment, and battle stats
		if ((*pressedX & 2) == 0)
		{
			// If you hit left or right on the D-Pad, or Analog Stick
			if ((tap & (BTN_LEFT | BTN_RIGHT)) != 0)
			{
				// Invert &1 bit
				*pressedX = *pressedX ^ 1;
			}

			if (
			    // ready to continue, after cooldown
			    (gGT->timerEndOfRaceVS < 0x78) && ((tap & (BTN_CROSS_one | BTN_START)) != 0))
			{
				// invert &2 bit
				*pressedX = *pressedX ^ 2;
			}

			// If you're in Battle Mode
			if ((gGT->gameMode1 & 0x20) != 0)
			{
				// ivar9 0x157: YOU HIT
				// ivar9 0x158: HIT YOU
				string = 0x157 + (*pressedX & 1);

				DecalFont_DrawLine(sdata->lngStrings[string],

				                   // Midpoint between pushBuffer Start X and End X
				                   r->x + (r->w >> 1),

				                   (r->y + 0x23),

				                   3, 0xffff8004);

				// If you have 3 screens, you need 9 prints
				// If you have 4 screens, you need 16 prints

				// for (a in players)
				// for (b in players)
				// print a hit b
				// print b hit a

				const SVec2 *battleStatsPos = s_battleStatsPos3P4P;
				if (numPlyr == 2)
				{
					battleStatsPos = sdata->Battle_EndOfRace.textPos2P;
				}

				for (j = 0; j < numPlyr; j++)
				{
					// YOU HIT THEM
					if ((*pressedX & 1) == 0)
					{
						numAttacked = (u8)currDriver->numTimesAttackingPlayer[j];
					}

					// HIT YOU
					else
					{
						numAttacked = (u8)currDriver->numTimesAttackedByPlayer[j];
					}

					sprintf(statText, "p%d:%2.02d",

					        // basically, j + 1
					        // which is (1, 2, 3, 4)
					        (j + 1),

					        // Amount of times this player hit you,
					        // or amount of times you hit them
					        numAttacked);


					// Get font color based on battle team
					sVar4 = (s16)gGT->drivers[j]->BattleHUD.teamID;
					local_78 = ((sVar4 + 0x18U) | 0x8000);


					DecalFont_DrawLine(statText,

					                   // midpoint between Start X and Size X
					                   (r->x + battleStatsPos[j].x),

					                   // midpoint between Start Y and Size Y
					                   (r->y + battleStatsPos[j].y),

					                   2, local_78);
				}
			}
		}

		// If Player has pressed X to continue
		else
		{
			// Stop drawing comment + battle stats

			memset(&clearColor, 0, sizeof(clearColor));
			clearRect = *r;
			CTR_Box_DrawClearBox(&clearRect, &clearColor, 0, gGT->backBuffer->otMem.uiOT);

			// Allow Go-Back option to YouHit/HitYou
			if ((tap & BTN_SQUARE_two) != 0)
			{
				// invert &2 bit
				*pressedX = *pressedX ^ 2;
			}

			// increment counter of players ready to continue
			ready++;
		}
	}

	// If all players press X to continue
	if (ready == numPlyr)
	{
		// Stop drawing 4 screens, draw 1 screen and options
		gGT->timerEndOfRaceVS = 0;
		*(int *)&sdata->Battle_EndOfRace.boolPressX[0] = 0;
	}

	return;
}
