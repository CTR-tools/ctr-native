#include <common.h>

u32 UI_VsQuipReadDriver(struct Driver *d, int offset, int size)
{
	switch (size)
	{
	case 1:
		return *(u8 *)((char *)d + offset);
	case 2:
		return *(s16 *)((char *)d + offset);
	case 4:
		return *(u32 *)((char *)d + offset);
	}

	return 0;
}

void UI_VsQuipAssign(struct Driver *driver, struct QuipMeta *meta, struct Driver *bestDriver, int characterID)
{
	struct QuipStr *selected;
	struct GameTracker *gGT;

	if (driver == NULL)
	{
		return;
	}

	if (((meta->flags & 4) != 0) && (driver != bestDriver))
	{
		return;
	}

	selected = meta->ptrQuipStrCurr;

	{
		struct QuipStr *curr;

		for (curr = selected + 1; curr < meta->ptrQuipStrNext; curr++)
		{
			if (curr->priority > selected->priority)
			{
				selected = curr;
				continue;
			}

			if ((curr->priority == selected->priority) && (((MixRNG_Scramble() >> 3) & 0xff) < 0x40))
			{
				selected = curr;
			}
		}
	}

	{
		struct QuipStr *oldQuip = (struct QuipStr *)driver->EndOfRaceComment_ptrQuip;

		if (oldQuip != NULL)
		{
			int selectedPriority = selected->priority;
			int oldPriority = oldQuip->priority;

			if (selectedPriority <= oldPriority)
			{
				if (selectedPriority != oldPriority)
				{
					return;
				}

				if (((MixRNG_Scramble() >> 3) & 0xff) > 0x3f)
				{
					return;
				}
			}

			{
				// NOTE(aalhendi): Retail rereads the old quip after the tie test.
				struct QuipStr *oldQuip = (struct QuipStr *)driver->EndOfRaceComment_ptrQuip;

				if ((oldQuip != NULL) && (oldQuip->priority < 0))
				{
					oldQuip->priority = -oldQuip->priority;
				}
			}
		}
	}

	if (selected->priority > 0)
	{
		selected->priority = -selected->priority;
	}

	gGT = GAME_TRACKER;

	driver->EndOfRaceComment_ptrQuip = (s16 *)selected;
	driver->EndOfRaceComment_characterID = characterID;

	if ((gGT->gameMode1 & BATTLE_MODE) == 0)
	{
		gGT->timerEndOfRaceVS = 300;
	}
	else
	{
		gGT->timerEndOfRaceVS = 150;
	}
}

void UI_VsQuipAssignAll(void)
{
	struct GameTracker *gGT = GAME_TRACKER;
	struct QuipMeta *metaStart;
	struct QuipMeta *metaEnd;
	// NOTE(aalhendi): The attacker index persists across quip conditions.
	int characterID = 0;
	struct QuipMeta *meta;
	struct Driver *bestDriver;
	struct Driver *driver;
	int bestScore;
	int secondScore;
	int scoreByDriverID[8];
	struct Thread *thread;

	if (gGT->numPlyrCurrGame < 2)
	{
		return;
	}

	if ((gGT->gameMode1 & BATTLE_MODE) != 0)
	{
		metaStart = data.endRaceQuips.battleMetadata;
		metaEnd = metaStart + 12;
	}
	else
	{
		metaStart = data.endRaceQuips.vsMetadata;
		metaEnd = metaStart + 39;
	}

	{
		for (meta = metaStart; meta < metaEnd; meta++)
		{
			struct QuipStr *curr = meta->ptrQuipStrCurr;

			while (curr < meta->ptrQuipStrNext)
			{
				curr->priority++;
				curr++;
			}
		}
	}

	// NOTE(aalhendi): secondScore retains the previous or tied maximum, not a sorted runner-up.
	bestScore = 0;
	secondScore = 0;
	bestDriver = NULL;

	thread = GAME_TRACKER->threadBuckets[PLAYER].thread;

	while (thread != NULL)
	{
		int score;

		driver = thread->object;

		driver->EndOfRaceComment_ptrQuip = NULL;

		if ((GAME_TRACKER->gameMode1 & POINT_LIMIT) != 0)
		{
			score = GAME_TRACKER->battleSetup.pointsPerTeam[driver->BattleHUD.teamID];
		}
		else
		{
			score = driver->BattleHUD.numLives;
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

	{
		for (meta = metaStart; meta < metaEnd; meta++)
		{
			struct Driver *selectedDriver;
			int threshold;
			u32 bestValue;

			bestValue = (meta->conditionType == 5) ? 0x7fffffff : 0;
			threshold = meta->threshold;
			selectedDriver = NULL;

			if ((meta->flags & 1) != 0)
			{
				int numLaps = GAME_TRACKER->numLaps;

				numLaps = abs(numLaps);

				threshold *= numLaps;
			}

			thread = GAME_TRACKER->threadBuckets[PLAYER].thread;

			while (thread != NULL)
			{
				u32 sampledValue;

				driver = thread->object;

				switch (meta->conditionType)
				{
				case 7:
				{
					switch (threshold)
					{
					case 0:
					{
						if (driver == bestDriver)
						{
							selectedDriver = bestDriver;
						}
						break;
					}

					case 1:
					{
						if ((secondScore != 0) && (scoreByDriverID[driver->driverID] == secondScore))
						{
							selectedDriver = driver;
						}
						break;
					}

					default:
					{
						break;
					}
					}
					break;
				}

				case 6:
				{
					u32 value = UI_VsQuipReadDriver(driver, meta->driverOffset, meta->dataSize);

					if (value == driver->numTimesAttacking)
					{
						selectedDriver = driver;
					}
					break;
				}

				case 8:
				{
					int value = UI_VsQuipReadDriver(driver, meta->driverOffset, meta->dataSize);

					if (value == threshold)
					{
						selectedDriver = driver;
					}
					break;
				}

				case 0:
				{
					int value = UI_VsQuipReadDriver(driver, meta->driverOffset, meta->dataSize);

					if (value > threshold)
					{
						selectedDriver = driver;
						threshold = value;
					}
					else if (value == threshold)
					{
						selectedDriver = NULL;
					}
					break;
				}

				case 1:
				{
					int value = UI_VsQuipReadDriver(driver, meta->driverOffset, meta->dataSize);

					if (value >= 0)
					{
						if (value < threshold)
						{
							selectedDriver = driver;
							threshold = value;
						}
						else if (value == threshold)
						{
							selectedDriver = NULL;
						}
					}
					break;
				}

				case 4:
				{
					int delta;

					sampledValue = UI_VsQuipReadDriver(driver, meta->driverOffset, meta->dataSize);
					delta = sampledValue - bestValue;

					if (threshold < delta)
					{
						bestValue = sampledValue;
						selectedDriver = driver;
					}
					else if (-threshold < delta)
					{
						selectedDriver = NULL;

						if ((int)bestValue < (int)sampledValue)
						{
							bestValue = sampledValue;
						}
					}
					break;
				}

				case 5:
				{
					int delta;

					sampledValue = UI_VsQuipReadDriver(driver, meta->driverOffset, meta->dataSize);
					delta = bestValue - sampledValue;

					if ((int)sampledValue >= 0)
					{
						if (threshold < delta)
						{
							bestValue = sampledValue;
							selectedDriver = driver;
						}
						else if (-threshold < delta)
						{
							selectedDriver = NULL;

							if ((int)sampledValue < (int)bestValue)
							{
								bestValue = sampledValue;
							}
						}
					}
					break;
				}

				case 3:
				{
					{
						int i;

						for (i = 0; i < 8; i++)
						{
							// NOTE(aalhendi): Keep the sampled byte in v1 rather than exchanging it with the delta.
							register u32 value CTR_PSX_REGISTER("$3") = driver->numTimesAttackedByPlayer[i];
							int delta = value - bestValue;

							if (threshold < delta)
							{
								bestValue = value;
								selectedDriver = driver;
								characterID = i;
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
					}

					break;
				}

				case 9:
				{
					if (driver->EndOfRaceComment_ptrQuip == NULL)
					{
						UI_VsQuipAssign(driver, meta, bestDriver, 0);
					}
					break;
				}

				default:
				{
					break;
				}
				}

				if ((selectedDriver != NULL) && ((meta->flags & 0xc) != 0))
				{
					UI_VsQuipAssign(selectedDriver, meta, bestDriver, characterID);
				}

				thread = thread->siblingThread;
			}

			UI_VsQuipAssign(selectedDriver, meta, bestDriver, characterID);
		}
	}
}

void UI_VsQuipDrawAll(void)
{
	enum
	{
		UI_VS_QUIP_FORMAT_BUFFER_SIZE = 0x80
	};

	char printBuffer[UI_VS_QUIP_FORMAT_BUFFER_SIZE];
	struct QuipStr *quip;
	struct Driver *d;

	struct GameTracker *gGT = GAME_TRACKER;
	struct Thread *thread;
	struct PushBuffer *pushBuffer;

	// loop through player threads
	int playerIndex = 0;
	int fontType = FONT_CREDITS;
	thread = gGT->threadBuckets[0].thread;
	pushBuffer = gGT->pushBuffer;
	for (; thread != 0; playerIndex++, thread = thread->siblingThread, pushBuffer++)
	{
		RECT *r;
		char *pressX;
		struct MetaDataCHAR *characterMetadata;
		int pressIndex;
		int centerX;
		int centerY;

		// NOTE(aalhendi): Keep the viewport snapshot before skips and formatting.
		r = &pushBuffer->rect;
		centerX = r->x + (r->w >> 1);
		centerY = r->y + (r->h >> 3);

		// get player struct from thread
		d = (struct Driver *)thread->object;

		// NOTE(aalhendi): Snapshot the metadata and resident press array here.
		characterMetadata = GAME_CHARACTER_METADATA;
		pressIndex = (s16)playerIndex;
		pressX = sdata_static.Battle_EndOfRace.boolPressX;
		// Draw only while this player is active and has a selected quip.
		if ((pressX[pressIndex] & 2) == 0)
		{
			quip = (struct QuipStr *)d->EndOfRaceComment_ptrQuip;
			if (quip != 0)
			{
				quip->priority = 0;

				// NOTE(aalhendi): Keep each branch's formatting and draw arguments together.
				if ((quip->flags & 1) != 0)
				{
					sprintf(printBuffer, GAME_LANGUAGE_STRINGS[(u16)quip->lngIndex],
					        GAME_LANGUAGE_STRINGS[characterMetadata[d->EndOfRaceComment_characterID].name_LNG_long]);
					RECTMENU_DrawQuip(printBuffer, centerX, centerY, 0, fontType, JUSTIFY_CENTER | ORANGE, 4);
				}
				else
				{
					RECTMENU_DrawQuip(GAME_LANGUAGE_STRINGS[(u16)quip->lngIndex], centerX, centerY, 0, fontType, JUSTIFY_CENTER | ORANGE, 4);
				}
			}
		}
	}
}
