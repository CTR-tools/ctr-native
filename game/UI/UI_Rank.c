#include <common.h>
#include <ctr_gte_transfer.h>

enum UIRankConstants
{
	UI_RANK_DRIVER_COUNT = 8,
	UI_RANK_VISIBLE_ARCADE_COUNT = 4,
	UI_RANK_VISIBLE_BOSS_COUNT = 2,
	UI_RANK_DRIVER_COUNT_PLUS_ONE = 9,
	UI_RANK_TEXT_X = 0x34,
	UI_RANK_TEXT_START_Y = 0x38,
	UI_RANK_TEXT_SLOT_HEIGHT = 0x1b,
	UI_RANK_ICON_X = 0x14,
	UI_RANK_ICON_BASE_Y = 0x39,
	UI_RANK_ICON_SCALE = FP(1),
	UI_RANK_TRANSITION_FRAMES = 5,
	UI_RANK_DAMAGE_TIMER_FRAMES = 30,
	UI_RANK_DAMAGE_COLOR_STEP = 4,
	UI_RANK_DAMAGE_COLOR_NEUTRAL = 0x808080,
	UI_RANK_TRACK_DISTANCE_SCALE = 8,
	UI_RANK_TRACK_SCREEN_DIVISOR = 0x1d1,
	UI_RANK_TRACK_SMOOTH_DIVISOR = 0xe,
	UI_RANK_TRACK_SNAP_THRESHOLD = 400,
	UI_RANK_TRACK_ICON_POS_X_OFFSET = 5,
	UI_RANK_TRACK_ICON_POS_Y = 0x66,
	UI_RANK_TRACK_ICON_SCALE = 0x9d8,
	UI_RANK_TRACK_WARPBALL_ICON = 0xe,
	UI_RANK_TRACK_WARPBALL_SCALE = 0x8aa,
	UI_RANK_TRACK_WARPBALL_PROGRESS_SHIFT = 0xc,
	UI_RANK_TRACK_CHECKPOINT_COUNT_LIMIT = 0xff,
};


// NOTE(aalhendi): Both paths write each damage channel separately; packed
// arithmetic would carry between channels for timers outside the normal range.

#define UI_RANK_DAMAGE_COLOR_BODY(driverSlot, color, channels, initialTimer, positiveTimerFrames)                             \
	{                                                                                                                         \
		if ((initialTimer) > 0)                                                                                               \
		{                                                                                                                     \
			s32 redStrength = ((positiveTimerFrames) - (initialTimer)) * UI_RANK_DAMAGE_COLOR_STEP;                           \
			s32 greenTimer = (*(driverSlot))->damageColorTimer;                                                               \
			(color)->r = ~redStrength;                                                                                        \
			(channels)[1] = ((positiveTimerFrames) - greenTimer) * UI_RANK_DAMAGE_COLOR_STEP;                                 \
			(channels)[2] = ((positiveTimerFrames) - (*(driverSlot))->damageColorTimer) * UI_RANK_DAMAGE_COLOR_STEP;          \
			(*(driverSlot))->damageColorTimer--;                                                                              \
		}                                                                                                                     \
		else if ((initialTimer) < 0)                                                                                          \
		{                                                                                                                     \
			(color)->r = ((initialTimer) + UI_RANK_DAMAGE_TIMER_FRAMES) * UI_RANK_DAMAGE_COLOR_STEP;                          \
			(channels)[1] = ~(((*(driverSlot))->damageColorTimer + UI_RANK_DAMAGE_TIMER_FRAMES) * UI_RANK_DAMAGE_COLOR_STEP); \
			(channels)[2] = ((*(driverSlot))->damageColorTimer + UI_RANK_DAMAGE_TIMER_FRAMES) * UI_RANK_DAMAGE_COLOR_STEP;    \
			(*(driverSlot))->damageColorTimer++;                                                                              \
		}                                                                                                                     \
		else                                                                                                                  \
		{                                                                                                                     \
			ColorCode_SetPacked(color, UI_RANK_DAMAGE_COLOR_NEUTRAL);                                                         \
		}                                                                                                                     \
	}

static inline void UI_Rank_UpdateDamageColor1P(struct Driver **driverSlot, Color *color, u8 *channelInput, s32 initialTimer, s32 positiveTimerFrames)
{
	register u8 *channels CTR_PSX_REGISTER("$23") = channelInput;
	UI_RANK_DAMAGE_COLOR_BODY(driverSlot, color, channels, initialTimer, positiveTimerFrames);
}

static inline void UI_Rank_UpdateDamageColorMP(struct Driver **driverSlot, Color *color, u8 *channelInput, s32 initialTimer, s32 positiveTimerFrames)
{
	register u8 *channels CTR_PSX_REGISTER("$19") = channelInput;
	UI_RANK_DAMAGE_COLOR_BODY(driverSlot, color, channels, initialTimer, positiveTimerFrames);
}

void UI_DrawRankedDrivers(void)
{
	struct GameTracker *initialTracker = GAME_TRACKER;
	union
	{
		SVec2 iconPos;
		Color color;
	} draw;
	SVec3 warpDelta;
	SVec3 warpPosition;
	SVec3 trackDir;
	union
	{
		Color color;
		u32 packed;
	} damageColors[2];


	if (initialTracker->numPlyrCurrGame == 1)
	{
		char *rankText = GAME_SDATA_NAME(s_spacebar);
		register u8 *damageChannels CTR_PSX_REGISTER("$23");
		s32 driverIndex;
		s16 visibleRankCount;
		s32 rankTextY;
		u8 *driverCursor;
		s16 *initialDesiredCursor;
		s16 *initialTimerCursor;
		register s16 *rankCurrCursor CTR_PSX_REGISTER("$22");
		register s16 *rankDesiredCursor CTR_PSX_REGISTER("$21");
		s32 rankByteOffset;
		s32 positiveTimerFrames;
		u32 finishFlagMask;
		// Number of racers that have finished race
		s16 numRacersFinished = 0;

		// incremented when looping through player structures

		driverIndex = 0;
		finishFlagMask = ACTION_RACE_FINISHED;
		driverCursor = (u8 *)initialTracker;
		initialDesiredCursor = GAME_DATA_NAME(rankIconsDesired);
		initialTimerCursor = GAME_DATA_NAME(rankIconsTransitionTimer);
		for (; driverIndex < UI_RANK_DRIVER_COUNT; driverCursor += sizeof(struct Driver *), initialDesiredCursor++, driverIndex++, initialTimerCursor++)
		{
			struct Driver **driverSlot = (struct Driver **)(driverCursor + offsetof(struct GameTracker, drivers));
			if (*initialTimerCursor == 0)
			{
#ifdef CTR_NATIVE
				// NOTE(aalhendi): Retail can read an empty slot through low RAM; native
				// must skip it before dereferencing the missing boss-race driver.
				if (*driverSlot != NULL)
				{
					*initialDesiredCursor = (*driverSlot)->driverRank;
				}
#else
				*initialDesiredCursor = (*driverSlot)->driverRank;
#endif
			}
			if (*driverSlot != NULL && ((*driverSlot)->actionsFlagSet & finishFlagMask) != 0)
			{
				numRacersFinished++;
			}
		}

		// Default for Arcade: Show 4 racers
		visibleRankCount = UI_RANK_VISIBLE_ARCADE_COUNT;

		if (IS_BOSS_RACE(GAME_TRACKER->gameMode1))
		{
			// Show 2 racers
			visibleRankCount = UI_RANK_VISIBLE_BOSS_COUNT;
		}

		driverIndex = 0;
		if (visibleRankCount > 0)
		{
			s32 finishedCount = numRacersFinished;
			s32 rankCount = visibleRankCount;
			rankTextY = UI_RANK_TEXT_START_Y << 16;
			do
			{
				// make the text white by default
				s16 txtColor = WHITE;

				// if racer has finished the race
				if (driverIndex < finishedCount)
				{
					// make the text red
					txtColor = RED;
				}

				// draw rank number: '1', '2', '3', '4'
				rankText[0] = (char)driverIndex + '1';
				DecalFont_DrawLine(rankText, UI_RANK_TEXT_X, rankTextY >> 0x10, FONT_SMALL, txtColor);

				// Advance to the next rank line.
				rankTextY = rankTextY + (UI_RANK_TEXT_SLOT_HEIGHT << 16);
				driverIndex++;
			} while (driverIndex < rankCount);
		}

		driverIndex = 0;
		positiveTimerFrames = UI_RANK_DAMAGE_TIMER_FRAMES;
		damageChannels = (u8 *)&damageColors[0];
		rankCurrCursor = GAME_DATA_NAME(rankIconsCurr);
		rankDesiredCursor = GAME_DATA_NAME(rankIconsDesired);
		rankByteOffset = driverIndex;
		for (; driverIndex < UI_RANK_DRIVER_COUNT; rankCurrCursor++, rankDesiredCursor++, driverIndex++, rankByteOffset += sizeof(s16))
		{
			register struct Driver *driver = GAME_TRACKER->drivers[driverIndex];
			ColorCode_SetPacked(&damageColors[0].color, 0);
			if (driver != 0 && *rankDesiredCursor + 1 < UI_RANK_DRIVER_COUNT_PLUS_ONE)
			{
				s16 *transitionTimer;
				s32 desiredRank;
				b32 isTransitioning;
				UI_Rank_UpdateDamageColor1P(&GAME_TRACKER->drivers[driverIndex], &damageColors[0].color, damageChannels, driver->damageColorTimer,
				                            positiveTimerFrames);
				transitionTimer = (s16 *)((u8 *)GAME_DATA_NAME(rankIconsTransitionTimer) + rankByteOffset);
				isTransitioning = *transitionTimer != 0;
				if (!isTransitioning)
				{
					desiredRank = *rankDesiredCursor;
					CTR_PSX_KEEP_VALUE_RELAXED(desiredRank);
					isTransitioning = desiredRank != *rankCurrCursor;
				}
				if (isTransitioning)
				{
					// NOTE(aalhendi): Prefer these work registers locally, without
					// excluding multiplayer packet-copy scratch registers globally.
					s16 *currBase CTR_PSX_LOCAL_REGISTER_HINT("10") = GAME_DATA_NAME(rankIconsCurr);
					s16 *desBase CTR_PSX_LOCAL_REGISTER_HINT("11") = GAME_DATA_NAME(rankIconsDesired);
					register s16 *curr CTR_PSX_REGISTER("$20") = (s16 *)((u8 *)currBase + rankByteOffset);
					register s16 *des CTR_PSX_REGISTER("$19") = (s16 *)((u8 *)desBase + rankByteOffset);
					CTR_PSX_ORDER_VALUES(currBase, curr);
					CTR_PSX_ORDER_VALUES(desBase, des);
					// NOTE(aalhendi): Lerp writes both coordinates before either is read.
					UI_Lerp2D_Angular(&draw.iconPos, *curr, *des, *transitionTimer);
					{
						s16 *characterSlot = (s16 *)((u8 *)GAME_CHARACTER_IDS + rankByteOffset);
						register struct MetaDataCHAR *metadata CTR_PSX_REGISTER("$14");
						metadata = GAME_CHARACTER_METADATA;
						UI_DrawDriverIcon(GAME_TRACKER->ptrIcons[metadata[*characterSlot].iconID], draw.iconPos.x, draw.iconPos.y,
						                  &GAME_TRACKER->backBuffer->primMem, GAME_TRACKER->pushBuffer_UI.ptrOT, TRANS_50_DECAL, UI_RANK_ICON_SCALE,
						                  damageColors[0].packed);
					}
					CTR_PSX_KEEP_VALUE_RELAXED(curr);
					CTR_PSX_KEEP_VALUE_RELAXED(des);
					(*transitionTimer)++;
					if (*transitionTimer >= UI_RANK_TRANSITION_FRAMES)
					{
						*transitionTimer = 0;
						*curr = *des;
					}
				}
				else if (desiredRank + 1 < UI_RANK_VISIBLE_ARCADE_COUNT + 1)
				{
					s16 *characterSlot = (s16 *)((u8 *)GAME_CHARACTER_IDS + rankByteOffset);
					register struct MetaDataCHAR *metadata CTR_PSX_REGISTER("$15");
					s32 iconX = UI_RANK_ICON_X;
					CTR_PSX_KEEP_VALUE_RELAXED(iconX);
					CTR_PSX_ORDER_VALUES(characterSlot, iconX);
					metadata = GAME_CHARACTER_METADATA;
					UI_DrawDriverIcon(GAME_TRACKER->ptrIcons[metadata[*characterSlot].iconID], iconX,
					                  desiredRank * UI_RANK_TEXT_SLOT_HEIGHT + UI_RANK_ICON_BASE_Y, &GAME_TRACKER->backBuffer->primMem,
					                  GAME_TRACKER->pushBuffer_UI.ptrOT, TRANS_50_DECAL, UI_RANK_ICON_SCALE, damageColors[0].packed);
					*rankCurrCursor = *rankDesiredCursor;
				}
			}
		}
	}

	// if this is multiplayer
	else
	{
		register s32 driverIndex CTR_PSX_REGISTER("$20");
		s16 *trackIconX;
		struct Thread *warpballThread;
		register u8 *damageChannels CTR_PSX_REGISTER("$19");
		s32 characterByteOffset;
		s32 driverByteOffset;
		s32 positiveTimerFrames;
		struct MetaDataCHAR *characterMetadata;
		SVec3 *trackDirection;
		struct GameTracker *nextTracker;

		driverIndex = 0;
		if (initialTracker->numPlyrCurrGame + initialTracker->numBotsNextGame != 0)
		{
			positiveTimerFrames = UI_RANK_DAMAGE_TIMER_FRAMES;
			damageChannels = (u8 *)&damageColors[1];
			characterMetadata = GAME_CHARACTER_METADATA;
			trackIconX = &GAME_DATA_NAME(rankIconsTransitionTimer)[0];
			characterByteOffset = 0;
			driverByteOffset = 0;
			do
			{
				u16 currentTrackX;
				register s32 targetTrackX;
				s32 delta;
				register s32 step CTR_PSX_REGISTER("$3");
				s32 stepQuotient;
				b32 passedTarget;
				s32 initialTimer;
				s32 characterID;
				s32 iconX;
				s32 iconY;
				{
					register struct Driver *driver = (*(struct Driver **)((u8 *)GAME_TRACKER + driverByteOffset + offsetof(struct GameTracker, drivers)));
					initialTimer = driver->damageColorTimer;
				}
				ColorCode_SetPacked(&damageColors[1].color, 0);
				UI_Rank_UpdateDamageColorMP((struct Driver **)((u8 *)GAME_TRACKER + driverByteOffset + offsetof(struct GameTracker, drivers)),
				                            &damageColors[1].color, damageChannels, initialTimer, positiveTimerFrames);
				draw.color = damageColors[1].color;
				currentTrackX = *trackIconX;
				if ((*(struct Driver **)((u8 *)GAME_TRACKER + driverByteOffset + offsetof(struct GameTracker, drivers)))->lapIndex == 0 &&
				    ((*(struct Driver **)((u8 *)GAME_TRACKER + driverByteOffset + offsetof(struct GameTracker, drivers)))->actionsFlagSet &
				     ACTION_BEHIND_START_LINE) != 0)
				{
					targetTrackX = 0;
				}
				else
				{
					s32 trackLength = GAME_TRACKER->level1->ptr_restart_points[0].distToFinish * UI_RANK_TRACK_DISTANCE_SCALE;
					register s32 trackProjection =
					    (s32)(trackLength -
					          (*(struct Driver **)((u8 *)GAME_TRACKER + driverByteOffset + offsetof(struct GameTracker, drivers)))->distanceToFinish_curr);
					trackProjection /= trackLength / UI_RANK_TRACK_SCREEN_DIVISOR;
					targetTrackX = trackProjection;
					CTR_PSX_DEPEND_VALUE(targetTrackX, trackProjection);
					if ((s16)trackProjection < 0)
					{
						targetTrackX = 0;
					}
				}
				stepQuotient = abs((s16)targetTrackX - (s16)currentTrackX) / UI_RANK_TRACK_SMOOTH_DIVISOR;
				step = stepQuotient;
				CTR_PSX_KEEP_VALUE_RELAXED(stepQuotient);
				if ((s16)stepQuotient == 0)
				{
					step = 1;
				}
				if ((s16)currentTrackX < (s16)targetTrackX)
				{
					currentTrackX += step;
					passedTarget = (s16)targetTrackX < (s16)currentTrackX;
				}
				else
				{
					currentTrackX -= step;
					passedTarget = (s16)currentTrackX < (s16)targetTrackX;
				}
				CTR_PSX_ORDER_VALUES(currentTrackX, step);
				if (passedTarget)
				{
					currentTrackX = targetTrackX;
				}
				delta = abs((s16)targetTrackX - (s16)currentTrackX);
				if (UI_RANK_TRACK_SNAP_THRESHOLD < delta)
				{
					currentTrackX = targetTrackX;
				}
				iconX = (s16)currentTrackX + UI_RANK_TRACK_ICON_POS_X_OFFSET;
				iconY = UI_RANK_TRACK_ICON_POS_Y;
				CTR_PSX_KEEP_VALUE_RELAXED(iconY);
				CTR_PSX_ORDER_VALUES(iconY, iconX);
				characterID = *(s16 *)((u8 *)GAME_CHARACTER_IDS + characterByteOffset);
				characterByteOffset += sizeof(s16);
				CTR_PSX_OBSERVE_MEMORY(draw.color);
				{
					struct GameTracker *drawTracker;
					drawTracker = GAME_TRACKER_SYMBOLIC;
					driverByteOffset += sizeof(struct Driver *);
					DecalHUD_DrawPolyGT4(drawTracker->ptrIcons[characterMetadata[characterID].iconID], iconX, iconY, &drawTracker->backBuffer->primMem,
					                     drawTracker->pushBuffer_UI.ptrOT, draw.color, draw.color, draw.color, draw.color, TRANS_50_DECAL,
					                     UI_RANK_TRACK_ICON_SCALE);
				}
				nextTracker = GAME_TRACKER;
				driverIndex++;
				*trackIconX = currentTrackX;
				trackIconX++;
			} while (driverIndex < nextTracker->numPlyrCurrGame + nextTracker->numBotsNextGame);
		}

		trackDirection = &trackDir;
		for (warpballThread = GAME_TRACKER->threadBuckets[TRACKING].thread; warpballThread != 0; warpballThread = warpballThread->siblingThread)
		{
			struct CheckpointNode *cn;
			struct CheckpointNode *currentNode;
			register struct TrackerWeapon *tw;
			s16 checkpointIndex;
			struct CheckpointNode *cn0;
			struct CheckpointNode *cn1;
			struct CheckpointNode *cn2;
			register s32 projectedDistance;
			s32 trackLength;
			s32 wrappedProgress;
			s32 remainingProgress;
			s32 trackScreenUnit;
			s32 posX;
			s32 posY;
			register struct GameTracker *trackTracker;
			struct GameTracker *postTracker;
			struct Thread *instanceThread;
			// Get Instance from Thread
			struct Instance *warpballInst = warpballThread->inst;

			// if not warpball, skip
			if (warpballInst->model->id != DYNAMIC_WARPBALL)
			{
				continue;
			}

			instanceThread = warpballInst->thread;

			// pointer to path data
			trackTracker = GAME_TRACKER;
			tw = instanceThread->object;
			currentNode = tw->ptrNodeCurr;
			cn = trackTracker->level1->ptr_restart_points;

			checkpointIndex = currentNode - cn;

			wrappedProgress = 0;
			if (((u32)(trackTracker->level1->cnt_restart_points - 1) < UI_RANK_TRACK_CHECKPOINT_COUNT_LIMIT) && checkpointIndex >= 0)
			{
				s32 projectionTrackLength;
				cn0 = &cn[checkpointIndex];
				cn1 = &cn[cn0->nextIndex_forward];
				cn2 = &cn[cn1->nextIndex_forward];
				warpPosition.x = warpballInst->matrix.t[0];
				warpPosition.y = warpballInst->matrix.t[1];
				warpPosition.z = warpballInst->matrix.t[2];
				trackDir.x = cn1->pos.x - cn2->pos.x;
				trackDir.y = cn1->pos.y - cn2->pos.y;
				trackDir.z = cn1->pos.z - cn2->pos.z;
				MATH_VectorNormalize(trackDirection);
				warpDelta.x = warpPosition.x - cn1->pos.x;
				warpDelta.y = warpPosition.y - cn1->pos.y;
				warpDelta.z = warpPosition.z - cn1->pos.z;
				CTR_GteLoadDotProduct(trackDirection, &warpDelta);
				CTR_GteLoadDelay();
				gte_mvmva(0, 0, 0, 3, 0);
				projectedDistance = CTR_GteReadMAC1();
				postTracker = GAME_TRACKER_SYMBOLIC;
				projectedDistance >>= UI_RANK_TRACK_WARPBALL_PROGRESS_SHIFT;
				wrappedProgress = cn1->distToFinish * UI_RANK_TRACK_DISTANCE_SCALE;
				wrappedProgress += projectedDistance;
				projectionTrackLength = postTracker->level1->ptr_restart_points[0].distToFinish * UI_RANK_TRACK_DISTANCE_SCALE;
				wrappedProgress %= projectionTrackLength;
			}
			if (wrappedProgress == 0)
			{
				continue;
			}
			trackLength = GAME_TRACKER->level1->ptr_restart_points[0].distToFinish * UI_RANK_TRACK_DISTANCE_SCALE;
			remainingProgress = trackLength - wrappedProgress;
			trackScreenUnit = trackLength / UI_RANK_TRACK_SCREEN_DIVISOR;
			posX = (s16)(remainingProgress / trackScreenUnit) + UI_RANK_TRACK_ICON_POS_X_OFFSET;
			posY = UI_RANK_TRACK_ICON_POS_Y;

			DecalHUD_DrawWeapon(
			    // warpball icon
			    GAME_TRACKER->ptrIcons[UI_RANK_TRACK_WARPBALL_ICON], posX, posY,

			    // pointer to PrimMem struct
			    &GAME_TRACKER->backBuffer->primMem,

			    // pointer to OT memory
			    GAME_TRACKER->pushBuffer_UI.ptrOT,

			    TRANS_50_DECAL, UI_RANK_TRACK_WARPBALL_SCALE, 1);
		}
	}
}
