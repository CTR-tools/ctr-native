#include <common.h>

// The warp pad lays out a starting grid before the race has loaded, so it asks
// the same rule the level will count its racers with.
#include <race_field.h>

// NOTE(aalhendi): WarpPad level IDs come from "warppad#NN" instance names
// and use retail adventure numbering, not the native LevelID enum.
enum AdventureWarpPadID
{
	AH_WP_ID_SLIDE_COLISEUM = 16,
	AH_WP_ID_TURBO_TRACK = 17,
	AH_WP_ID_FIRST_BATTLE_TRACK = 18,
	AH_WP_ID_SKULL_ROCK = 21,
	AH_WP_ID_ROCKY_ROAD = 23,
	AH_WP_ID_FIRST_GEM_CUP = ADVENTURE_CUP_SYNTHETIC_LEVEL_ID_BASE,
};

enum
{
	AH_WP_GEM_CUP_COUNT = 5,
	AH_WP_BATTLE_TRACK_ID_SPAN = 7,
	AH_WP_RACER_SLOT_COUNT = RACE_FIELD_FULL_GRID_RACERS,
	AH_WP_SPEED_CHAMPION_GRID_SLOT = 0,
	AH_WP_FIRST_BOT_GRID_SLOT = 1,
	AH_WP_REWARD_INSTANCE_COUNT = 3,
	AH_WP_WISP_COUNT = 2,
	AH_WP_WARP_LOAD_FRAMES = 61,
	AH_WP_TROPHY_PORTAL_HOLD_FRAMES = 0x400,
	AH_WP_LONG_RANGE_NEAR_DIST_SQ = 0x144000,
	AH_WP_SHORT_RANGE_NEAR_DIST_SQ = 0x90000,
	AH_WP_WARP_TRIGGER_DIST_SQ = 0x8fff,
	AH_WP_REWARD_FADE_DIST_SQ = 0x900000,
	AH_WP_REWARD_SPACING_ANGLE = 0x555,
	AH_WP_SPAWN_FORWARD_OFFSET = 0x400,
	AH_WP_FLOATING_OBJECT_HEIGHT = 0x100,
	AH_WP_PRIZE_RING_RADIUS = 0xc0,
	AH_WP_PRIZE_TILT_Z = 0x155,
	AH_WP_RELIC_PRIZE_SCALE = 0x1800,
	AH_WP_STANDARD_ITEM_SCALE = 0x2000,
	AH_WP_TROPHY_PRIZE_SCALE = 0x2800,
	AH_WP_CLOSED_SINGLE_DIGIT_OFFSET = -0x80,
	AH_WP_CLOSED_SINGLE_ITEM_OFFSET = 0x80,
	AH_WP_CLOSED_ITEM_WITH_TENS_OFFSET = 0xc0,
	AH_WP_CLOSED_X_WITH_TENS_OFFSET = 0x40,
	AH_WP_CLOSED_TENS_OFFSET = -0x40,
	AH_WP_CLOSED_ONES_WITH_TENS_OFFSET = -0xa0,
	AH_WP_SPIN_PRIZE_STEP = 0x40,
	AH_WP_SPIN_PRIZE_REWARD_STEP = 0x80,
	AH_WP_SPIN_BEAM_STEP = 0x200,
	AH_WP_SPIN_WISP_STEP = 0x100,
	AH_WP_SPIN_REWARD_RING_STEP = 0x4,
	AH_WP_REWARD_PHASE_STEP = 0x20,
	AH_WP_WISP_RISE_RATE = 0x20,
	AH_WP_WISP_RISE_RATE_STEP = 0x10,
	AH_WP_WISP_FAR_MAX_HEIGHT = 0x600,
	AH_WP_WISP_NEAR_MAX_HEIGHT = 0x400,
	AH_WP_WISP_FIRST_FRAMES = 4,
	AH_WP_WISP_FADE_IN_STEP = 0x380,
	AH_WP_WISP_FADE_OUT_RANGE = 0xc00,
	AH_WP_OPEN_BEAM_ALPHA = 0xc00,
	AH_WP_OPEN_RING_ALPHA = 0x400,
	AH_WP_OPEN_RING_HEIGHT_STEP = 0x400,
	AH_WP_FULL_ALPHA = 0x1000,
	AH_WP_PURPLE_TOKEN_GROUP = 4,
	AH_WP_NAME_LEVEL_ID_OFFSET = 8,
	AH_WP_DIGIT_MODEL_FIRST_1_TO_8 = 0x38,
	AH_WP_DIGIT_MODEL_0 = 0x6d,
	AH_WP_DIGIT_MODEL_9 = 0x6e,
};

s16 *AH_WarpPad_GetSpawnPosRot(s16 *posData)
{
	struct Thread *t;
	struct Instance *inst;

	for (t = GAME_TRACKER->threadBuckets[WARPPAD].thread; t != NULL; t = t->siblingThread)
	{
		inst = t->inst;
		if (((struct WarpPad *)t->object)->levelID == GAME_TRACKER->prevLEV)
		{
			posData[0] = inst->matrix.t[0] + ((MATH_Cos(inst->instDef->rot.y) * AH_WP_SPAWN_FORWARD_OFFSET) >> 12);
			posData[1] = inst->matrix.t[1];
			posData[2] = inst->matrix.t[2] + ((MATH_Sin(inst->instDef->rot.y) * -AH_WP_SPAWN_FORWARD_OFFSET) >> 12);
			return &inst->instDef->rot.x;
		}
	}
	return NULL;
}

CTR_STATIC_ASSERT(sizeof(struct WarpPad) == 0x78);
CTR_STATIC_ASSERT(offsetof(struct WarpPad, lightDirGem) == 0x50);
CTR_STATIC_ASSERT(offsetof(struct WarpPad, digit10s) == 0x68);
CTR_STATIC_ASSERT(offsetof(struct WarpPad, levelID) == 0x6c);

void AH_WarpPad_AllWarppadNum()
{
	struct Thread *t = GAME_TRACKER->threadBuckets[WARPPAD].thread;

	for (; t != 0; t = t->siblingThread)
	{
		struct WarpPad *wp = t->object;

		// DCxDemo says:
		// 1 to 8 is taken from mpk i guess, 0, 9 and X are seprate models

		if ((wp->slots.inst[2] != 0) && (wp->digit1s != 0) && (wp->digit1s != 9))
		{
			INST_GETIDPP(wp->slots.inst[2])->ptrCommandList = wp->slots.inst[2]->model->headers[wp->digit1s - 1].ptrCommandList;
			INST_GETIDPP(wp->slots.inst[2])->ptrColorLayout = (u32)wp->slots.inst[2]->model->headers[wp->digit1s - 1].ptrColors;
			INST_GETIDPP(wp->slots.inst[2])->ptrTexLayout = wp->slots.inst[2]->model->headers[wp->digit1s - 1].ptrTexLayout;
			INST_GETIDPP(wp->slots.inst[2])->ptrCurrFrame = wp->slots.inst[2]->model->headers[wp->digit1s - 1].ptrFrameData;
		}

		if ((wp->slots.inst[3] != 0) && (wp->digit10s != 0))
		{
			INST_GETIDPP(wp->slots.inst[3])->ptrCommandList = wp->slots.inst[3]->model->headers[0].ptrCommandList;
			INST_GETIDPP(wp->slots.inst[3])->ptrColorLayout = (u32)wp->slots.inst[3]->model->headers[0].ptrColors;
			INST_GETIDPP(wp->slots.inst[3])->ptrTexLayout = wp->slots.inst[3]->model->headers[0].ptrTexLayout;
			INST_GETIDPP(wp->slots.inst[3])->ptrCurrFrame = wp->slots.inst[3]->model->headers[0].ptrFrameData;
		}
	}
}

void AH_WarpPad_MenuProc(struct RectMenu *menu)
{
	RECTMENU_Hide(menu);

	switch (menu->rowSelected)
	{
	case 0:
		GAME_TRACKER->gameMode2 |= TOKEN_RACE;
		break;
	case 1:
		GAME_TRACKER->gameMode1 |= RELIC_RACE;
		break;
	}
}

void AH_WarpPad_SpinRewards(struct Instance *prizeInst, struct WarpPad *warppadObj, s16 index, Vec3 position)
{
	s32 modelID;
	s32 thirds;
	s32 trig;

	ConvertRotToMatrix(&prizeInst->matrix, &warppadObj->spinRot_Prize);

	modelID = prizeInst->model->id;

	if (modelID != STATIC_TROPHY) // if not trophy (no lightDir on trophy)
	{
		if (modelID == STATIC_GEM) // gem
		{
			Vector_SpecLightSpin3D(prizeInst, &warppadObj->spinRot_Prize, &warppadObj->lightDirGem);
		}
		else
		{
			if (modelID == STATIC_RELIC) // relic
			{
				Vector_SpecLightSpin3D(prizeInst, &warppadObj->spinRot_Prize, &warppadObj->lightDirRelic);
			}
			else
			{
				if (modelID == STATIC_TOKEN) // token
				{
					Vector_SpecLightSpin3D(prizeInst, &warppadObj->spinRot_Prize, &warppadObj->lightDirToken);
				}
			}
		}
	}

	// initialized as AH_WP_REWARD_SPACING_ANGLE * index, but not const
	thirds = (s16)warppadObj->thirds[index];

	trig = MATH_Sin(thirds);
	trig = ((trig * 64) >> 0xc) + AH_WP_FLOATING_OBJECT_HEIGHT;
	prizeInst->matrix.t[1] = position.y + trig;

	// do not use original "thirds",
	// set new value without "+="
	thirds = AH_WP_REWARD_SPACING_ANGLE * index + warppadObj->spinRot_Rewards.y;

	trig = MATH_Sin(thirds);
	prizeInst->matrix.t[0] = position.x + (trig * 0xA0 >> 0xc);

	thirds = AH_WP_REWARD_SPACING_ANGLE * index + warppadObj->spinRot_Rewards.y;
	trig = MATH_Cos(thirds);
	prizeInst->matrix.t[2] = position.z + (trig * 0xA0 >> 0xc);
}

// NOTE(aalhendi): Synthetic cup IDs probe beyond metaDataLEV. Read through
// the enclosing data image without indexing past the native array.
static inline s16 AH_WarpPad_Champion(s32 levelID)
{
	return (s16)CTR_ReadU16AlignedLE((const u8 *)&data + offsetof(struct Data, metaDataLEV) + levelID * sizeof(struct MetaDataLEV) +
	                                 offsetof(struct MetaDataLEV, characterID_Champion));
}

// Slide Col + Turbo Track, which open a relic race rather than a normal round.
static b32 AH_WarpPad_IsRelicOnlyTrack(s32 destination)
{
	return (u16)(destination - AH_WP_ID_SLIDE_COLISEUM) < 2;
}

// The battle maps, which open a crystal challenge.
static b32 AH_WarpPad_IsBattleTrack(s32 destination)
{
	return (u16)(destination - AH_WP_ID_FIRST_BATTLE_TRACK) < 2 || destination == AH_WP_ID_SKULL_ROCK ||
	       destination == AH_WP_ID_ROCKY_ROAD;
}

void AH_WarpPad_ThTick(struct Thread *t)
{
	struct Driver *driver;
	struct Instance *driverInst;
	s32 i;
	b16 boolOpen;

	struct WarpPad *warppadObj;
	struct Instance *warppadInst;
	struct Instance **visInstSrc;

	s32 modelID;
	s32 x, y, z, dist;
	s32 rangeLevel;
	u32 unsignedLevel;

	s16 angleCamToWarppad;

	s16 wispMaxHeight;
	s16 wispRiseRate;
	s32 rng1;
	s32 resetQuotient;
	s16 rng2;

	s16 rewardScale;
	s32 rewardScale2;

	s32 champID;
	s32 champGridSlot;

	s32 destination;
	s32 destinationCupID;
	int destinationMode;
	s32 racerCount;

	// The human drivers always take the grid slots at the back, so the bots are
	// the ones that share the front of the grid.
	s32 firstBotSlot;
	s32 botCount;

	u8 randKartSpawn[AH_WP_RACER_SLOT_COUNT];
	rewardScale = 0x100;
	warppadObj = t->object;
	visInstSrc = GAME_TRACKER->cameraDC[0].visInstSrc;
	warppadInst = t->inst;
	driver = GAME_TRACKER->drivers[0];
	boolOpen = false;

	// NOTE(aalhendi): Retail can read PS1 low RAM when the hub-swap frame
	// leaves this list null; native treats that as an empty visible-instance list.
	if (visInstSrc != NULL)
	{
		while (visInstSrc[0] != 0)
		{
			if (visInstSrc[0] == warppadInst)
			{
				boolOpen = true;
				break;
			}

			visInstSrc++;
		}
	}

	// make instances visible
	if (boolOpen)
	{
		if (warppadObj->slots.visuals.closed[0])
			warppadObj->slots.visuals.closed[0]->flags &= ~HIDE_MODEL;
		if (warppadObj->slots.visuals.closed[1])
			warppadObj->slots.visuals.closed[1]->flags &= ~HIDE_MODEL;
		if (warppadObj->slots.visuals.closed[2])
			warppadObj->slots.visuals.closed[2]->flags &= ~HIDE_MODEL;
		if (warppadObj->slots.visuals.closed[3])
			warppadObj->slots.visuals.closed[3]->flags &= ~HIDE_MODEL;
		if (warppadObj->slots.visuals.beam)
			warppadObj->slots.visuals.beam->flags &= ~HIDE_MODEL;
		for (i = 0; i < 2; i++)
		{
			if (warppadObj->slots.visuals.rings[i] != 0)
			{
				warppadObj->slots.visuals.rings[i]->flags &= ~HIDE_MODEL;
			}
		}
		for (i = 0; i < 3; i++)
		{
			if (warppadObj->slots.visuals.prizes[i] != 0)
			{
				warppadObj->slots.visuals.prizes[i]->flags &= ~HIDE_MODEL;
			}
		}
	}

	// make instances invisible
	else
	{
		if (warppadObj->slots.visuals.closed[0])
			warppadObj->slots.visuals.closed[0]->flags |= HIDE_MODEL;
		if (warppadObj->slots.visuals.closed[1])
			warppadObj->slots.visuals.closed[1]->flags |= HIDE_MODEL;
		if (warppadObj->slots.visuals.closed[2])
			warppadObj->slots.visuals.closed[2]->flags |= HIDE_MODEL;
		if (warppadObj->slots.visuals.closed[3])
			warppadObj->slots.visuals.closed[3]->flags |= HIDE_MODEL;
		if (warppadObj->slots.visuals.beam)
			warppadObj->slots.visuals.beam->flags |= HIDE_MODEL;
		for (i = 0; i < 2; i++)
		{
			if (warppadObj->slots.visuals.rings[i] != 0)
			{
				warppadObj->slots.visuals.rings[i]->flags |= HIDE_MODEL;
			}
		}
		for (i = 0; i < 3; i++)
		{
			if (warppadObj->slots.visuals.prizes[i] != 0)
			{
				warppadObj->slots.visuals.prizes[i]->flags |= HIDE_MODEL;
			}
		}
	}

	warppadInst->flags |= HIDE_MODEL;

	driverInst = driver->instSelf;

	x = driverInst->matrix.t[0];
	x = (s32)((u32)warppadInst->matrix.t[0] - (u32)x);
	y = CTR_MipsSubLo(warppadInst->matrix.t[1], driverInst->matrix.t[1]);
	// NOTE(aalhendi): Keep the Z input separate from its displacement while
	// the three squared terms overlap in the register schedule.
	{
		s32 portalZ;
		register s32 playerZ CTR_PSX_REGISTER("$2");
		portalZ = warppadInst->matrix.t[2];
		playerZ = driverInst->matrix.t[2];
		z = (s32)((u32)portalZ - (u32)playerZ);
	}
	dist = (s32)((u32)CTR_MipsMulLo(x, x) + (u32)CTR_MipsMulLo(y, y) + (u32)CTR_MipsMulLo(z, z));

	rangeLevel = warppadObj->levelID;
	unsignedLevel = (u16)warppadObj->levelID;
	// if near a portal
	if (
	    // Trophy tracks (-16)
	    ((rangeLevel < AH_WP_ID_SLIDE_COLISEUM) && (dist < AH_WP_LONG_RANGE_NEAR_DIST_SQ)) ||

	    // Slide Col + Turbo Track (-16)
	    (((unsignedLevel - AH_WP_ID_SLIDE_COLISEUM) < 2) && (dist < AH_WP_SHORT_RANGE_NEAR_DIST_SQ)) ||

	    // Battle tracks (-18)
	    (((unsignedLevel - AH_WP_ID_FIRST_BATTLE_TRACK) < AH_WP_BATTLE_TRACK_ID_SPAN) && (dist < AH_WP_LONG_RANGE_NEAR_DIST_SQ)) ||

	    // Gem cups
	    ((rangeLevel >= AH_WP_ID_FIRST_GEM_CUP) && (dist < AH_WP_SHORT_RANGE_NEAR_DIST_SQ)))
	{
		// if you are near a new warppad, or if you already were
		// determined as near the same warppad in the last frame,
		// then use this warppad as the "closest". Otherwise the
		// game could run this for two warppads right next to each other
		if ((AH_WARP_NEAREST_ID == -1) || (AH_WARP_NEAREST_ID == warppadObj->levelID))
		{
			// saved as nearest warppad
			AH_WARP_NEAREST_ID = warppadObj->levelID;
			{
				struct PushBuffer *pushBuffer = &GAME_TRACKER->pushBuffer[0];
				if (warppadObj->levelID >= AH_WP_ID_FIRST_GEM_CUP)
				{
					s16 titleCup = warppadObj->levelID - AH_WP_ID_FIRST_GEM_CUP;
					if (AH_HINT_STATE == 0)
						DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[AH_WARP_CUPS[titleCup].lngIndex_CupName], pushBuffer->rect.x + (pushBuffer->rect.w >> 1),
						                   pushBuffer->rect.y + pushBuffer->rect.h - 30, FONT_BIG, JUSTIFY_CENTER | ORANGE);
					if (!(GAME_ADV_PROGRESS.rewards[ADV_REWARD_HINT_GEM_CUPS_CHALLENGE / 32] & MEMCARD_BIT_MASK(ADV_REWARD_HINT_GEM_CUPS_CHALLENGE)) &&
					    warppadObj->slots.inst[WPIS_CLOSED_1S])
						MainFrame_RequestMaskHint(ADV_MASK_HINT_ID_GEM_CUPS_CHALLENGE, 0);
				}
				else
				{
					if (AH_HINT_STATE == 0)
						DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[AH_LEVEL_METADATA[warppadObj->levelID].name_LNG],
						                   pushBuffer->rect.x + (pushBuffer->rect.w >> 1), pushBuffer->rect.y + pushBuffer->rect.h - 30, FONT_BIG,
						                   JUSTIFY_CENTER | ORANGE);
					if (warppadObj->levelID == AH_WP_ID_SLIDE_COLISEUM)
					{
						if (!(GAME_ADV_PROGRESS.rewards[ADV_REWARD_HINT_MUST_GET_10_RELICS / 32] & MEMCARD_BIT_MASK(ADV_REWARD_HINT_MUST_GET_10_RELICS)) &&
						    warppadObj->slots.inst[WPIS_CLOSED_1S])
							MainFrame_RequestMaskHint(ADV_MASK_HINT_ID_MUST_GET_10_RELICS, 0);
					}
					else if ((u16)(warppadObj->levelID - AH_WP_ID_TURBO_TRACK) >= 8)
					{
						if (!(GAME_ADV_PROGRESS.rewards[ADV_REWARD_HINT_NEED_MORE_TROPHIES / 32] & MEMCARD_BIT_MASK(ADV_REWARD_HINT_NEED_MORE_TROPHIES)) &&
						    warppadObj->slots.inst[WPIS_CLOSED_1S] && warppadObj->slots.inst[WPIS_CLOSED_ITEM]->model->id != STATIC_KEY)
							MainFrame_RequestMaskHint(ADV_MASK_HINT_ID_NEED_MORE_TROPHIES, 0);
					}
				}
			}
		}
	}

	// not near portal
	else
	{
		AH_WARP_NEAREST_ID = -1;
	}

	// if warppad is locked
	if (warppadObj->slots.inst[WPIS_CLOSED_1S] != 0)
	{
		struct Instance *closedItemInst;
		angleCamToWarppad = ratan2(warppadInst->matrix.t[0] - GAME_TRACKER->pushBuffer[0].pos.x, warppadInst->matrix.t[2] - GAME_TRACKER->pushBuffer[0].pos.z);

		angleCamToWarppad = -angleCamToWarppad;


		if (warppadObj->slots.inst[WPIS_CLOSED_10S] != 0)
		{
			warppadObj->slots.inst[WPIS_CLOSED_ITEM]->matrix.t[0] =
			    warppadInst->matrix.t[0] + (MATH_Cos(angleCamToWarppad) * AH_WP_CLOSED_ITEM_WITH_TENS_OFFSET >> 0xC);
			warppadObj->slots.inst[WPIS_CLOSED_ITEM]->matrix.t[2] =
			    warppadInst->matrix.t[2] + (MATH_Sin(angleCamToWarppad) * AH_WP_CLOSED_ITEM_WITH_TENS_OFFSET >> 0xC);

			warppadObj->slots.inst[WPIS_CLOSED_X]->matrix.t[0] =
			    warppadInst->matrix.t[0] + (MATH_Cos(angleCamToWarppad) * AH_WP_CLOSED_X_WITH_TENS_OFFSET >> 0xC);
			warppadObj->slots.inst[WPIS_CLOSED_X]->matrix.t[2] =
			    warppadInst->matrix.t[2] + (MATH_Sin(angleCamToWarppad) * AH_WP_CLOSED_X_WITH_TENS_OFFSET >> 0xC);

			warppadObj->slots.inst[WPIS_CLOSED_10S]->matrix.t[0] = warppadInst->matrix.t[0] + (MATH_Cos(angleCamToWarppad) * AH_WP_CLOSED_TENS_OFFSET >> 0xC);
			warppadObj->slots.inst[WPIS_CLOSED_10S]->matrix.t[2] = warppadInst->matrix.t[2] + (MATH_Sin(angleCamToWarppad) * AH_WP_CLOSED_TENS_OFFSET >> 0xC);

			warppadObj->slots.inst[WPIS_CLOSED_1S]->matrix.t[0] =
			    warppadInst->matrix.t[0] + (MATH_Cos(angleCamToWarppad) * AH_WP_CLOSED_ONES_WITH_TENS_OFFSET >> 0xC);
			warppadObj->slots.inst[WPIS_CLOSED_1S]->matrix.t[2] =
			    warppadInst->matrix.t[2] + (MATH_Sin(angleCamToWarppad) * AH_WP_CLOSED_ONES_WITH_TENS_OFFSET >> 0xC);
		}
		else
		{
			warppadObj->slots.inst[WPIS_CLOSED_1S]->matrix.t[0] =
			    warppadInst->matrix.t[0] + (MATH_Cos(angleCamToWarppad) * AH_WP_CLOSED_SINGLE_DIGIT_OFFSET >> 0xC);
			warppadObj->slots.inst[WPIS_CLOSED_1S]->matrix.t[2] =
			    warppadInst->matrix.t[2] + (MATH_Sin(angleCamToWarppad) * AH_WP_CLOSED_SINGLE_DIGIT_OFFSET >> 0xC);

			warppadObj->slots.inst[WPIS_CLOSED_ITEM]->matrix.t[0] =
			    warppadInst->matrix.t[0] + (MATH_Cos(angleCamToWarppad) * AH_WP_CLOSED_SINGLE_ITEM_OFFSET >> 0xC);
			warppadObj->slots.inst[WPIS_CLOSED_ITEM]->matrix.t[2] =
			    warppadInst->matrix.t[2] + (MATH_Sin(angleCamToWarppad) * AH_WP_CLOSED_SINGLE_ITEM_OFFSET >> 0xC);
		}

		warppadObj->spinRot_Prize.x = 0;
		warppadObj->spinRot_Prize.z = 0;

		warppadObj->spinRot_Prize.y += AH_WP_SPIN_PRIZE_STEP;

		ConvertRotToMatrix(&warppadObj->slots.inst[WPIS_CLOSED_ITEM]->matrix, &warppadObj->spinRot_Prize);
		closedItemInst = warppadObj->slots.inst[WPIS_CLOSED_ITEM];
		if (closedItemInst->model->id == STATIC_GEM)
		{
			s16 colorIndex = ((u32)GAME_TRACKER->timer / 60) % 5;
			closedItemInst->colorRGBA =
			    ((u32)AH_WARP_CUPS[colorIndex].color[0] << 20) | ((u32)AH_WARP_CUPS[colorIndex].color[1] << 12) | ((u32)AH_WARP_CUPS[colorIndex].color[2] << 4);
		}
		modelID = warppadObj->slots.inst[WPIS_CLOSED_ITEM]->model->id;
		if (modelID == STATIC_TROPHY)
			return;
		if (modelID == STATIC_KEY)
			Vector_SpecLightSpin3D(warppadObj->slots.inst[WPIS_CLOSED_ITEM], &warppadObj->spinRot_Prize, &warppadObj->lightDirGem);
		else if (modelID == STATIC_RELIC)
			Vector_SpecLightSpin3D(warppadObj->slots.inst[WPIS_CLOSED_ITEM], &warppadObj->spinRot_Prize, &warppadObj->lightDirRelic);
		else if (modelID == STATIC_TOKEN)
			Vector_SpecLightSpin3D(warppadObj->slots.inst[WPIS_CLOSED_ITEM], &warppadObj->spinRot_Prize, &warppadObj->lightDirToken);
		else if (modelID == STATIC_GEM)
			Vector_SpecLightSpin3D(warppadObj->slots.inst[WPIS_CLOSED_ITEM], &warppadObj->spinRot_Prize, &warppadObj->lightDirGem);
		return;
	}

	// === Assume Unlocked ===

	if ((dist > AH_WP_WARP_TRIGGER_DIST_SQ) && (warppadObj->boolEnteredWarppad == 0))
	{
		goto WarpPad_AnimateOpen;
	}

	// NOTE(aalhendi): This setup repeats every close/warping frame before the
	// transition/load gate.
	LOAD_Robots1P(GAME_CHARACTER_IDS[0]);

	// Track speed champion.
	champID = AH_WarpPad_Champion(warppadObj->levelID);

	// The grid must be as long as the field this pad warps to, or the humans at the
	// back land on slots no racer reaches. The hub runs with Adventure-arena
	// flags, so the round is described by the pad's destination instead.
	destination = warppadObj->levelID;
	firstBotSlot = sdata->advMultiplayer.numPlayers;

	destinationMode = ADVENTURE_MODE;
	destinationCupID = LOAD_CUP_ID_NONE;

	if (destination >= AH_WP_ID_FIRST_GEM_CUP)
	{
		destinationCupID = destination - AH_WP_ID_FIRST_GEM_CUP;
		destinationMode |= ADVENTURE_CUP;
	}
	else if (AH_WarpPad_IsRelicOnlyTrack(destination))
	{
		destinationMode |= RELIC_RACE;
	}
	else if (AH_WarpPad_IsBattleTrack(destination))
	{
		destinationMode |= CRYSTAL_CHALLENGE;
	}

	racerCount = RaceField_RacerCount(destinationMode, destinationCupID, firstBotSlot, g_config.extendedMultiplayer);
	botCount = racerCount - firstBotSlot;

	// If Speed Champion is on the track (Crash-Pura)
	// and is not the same characterID as this driver
	if ((champID < AH_WP_RACER_SLOT_COUNT) && (champID != GAME_CHARACTER_IDS[driver->driverID]))
	{
		champGridSlot = 0;
		// set everyone to spawn in order, starting after the human drivers
		for (i = firstBotSlot; i < racerCount; i++)
		{
			if (AH_WarpPad_Champion(warppadObj->levelID) == GAME_CHARACTER_IDS[i])
			{
				// The last bot takes over the grid slot the champion vacated, which
				// is not the champion's driver slot once the humans sit at the back.
				champGridSlot = AH_WP_FIRST_BOT_GRID_SLOT + (i - firstBotSlot);
				AH_KART_SPAWN_ORDER[i] = AH_WP_SPEED_CHAMPION_GRID_SLOT;
			}

			else if (i == racerCount - 1)
			{
				AH_KART_SPAWN_ORDER[racerCount - 1] = champGridSlot;
			}

			else
			{
				AH_KART_SPAWN_ORDER[i] = AH_WP_FIRST_BOT_GRID_SLOT + (i - firstBotSlot);
			}
		}
	}

	// Speed Champion is invalid
	else
	{
		for (i = firstBotSlot; i < racerCount; i++)
		{
			randKartSpawn[i] = i;
		}

		for (i = 0; i < botCount; i++)
		{
			rng1 = RngDeadCoed(&AH_ADVENTURE_RNG);

			rng2 = (rng1 & 0xfff) % (botCount - i) + firstBotSlot;
			AH_KART_SPAWN_ORDER[(s32)randKartSpawn[rng2]] = (u8)i;

			while (rng2 < racerCount - 1)
			{
				randKartSpawn[rng2] = randKartSpawn[rng2 + 1];
				rng2++;
			}
		}
	}

	// spawn the human drivers in the back, P1 last
	for (i = 0; i < firstBotSlot; i++)
	{
		AH_KART_SPAWN_ORDER[i] = racerCount - 1 - i;
	}

	// if flag is on-screen, loading has already been finalized
	if (RaceFlag_IsTransitioning())
	{
		goto WarpPad_AnimateOpen;
	}


	{
		if (destination >= AH_WP_ID_FIRST_GEM_CUP)
		{
			s16 cupID = warppadObj->levelID - AH_WP_ID_FIRST_GEM_CUP;
			warppadObj->boolEnteredWarppad = 1;
			warppadObj->framesWarping++;
			driver->funcPtrs[DRIVER_FUNC_INIT] = VehStuckProc_Warp_Init;
			if (warppadObj->framesWarping >= AH_WP_WARP_LOAD_FRAMES)
			{
				struct GameTracker *cupGT = GAME_TRACKER;
				GAME_REMOVE_CONFIG_0 |= ADVENTURE_ARENA;
				GAME_ADD_CONFIG_0 |= ADVENTURE_CUP;
				cupGT->cup.trackIndex = 0;
				cupGT->cup.cupID = cupID;
				for (i = 7; i >= 0; i--)
					cupGT->cup.points[i] = 0;
				{
					u32 trackOffset;
					s32 cupOffset;
					register const u8 *cupTracks CTR_PSX_REGISTER("$4");
					trackOffset = GAME_TRACKER->cup.trackIndex;
					cupTracks = (const u8 *)AH_WARP_CUP_TRACKS;

					cupOffset = cupID;
					// NOTE(aalhendi): Each track occupies a word slot; this
					// path consumes its signed low halfword.
					trackOffset *= 4;
					cupOffset *= 16;
					trackOffset += cupOffset;
					trackOffset = (u32)trackOffset + (u32)cupTracks;
					MainRaceTrack_RequestLoad((s16)CTR_ReadU16AlignedLE((const void *)trackOffset));
				}
			}
			goto WarpPad_AnimateOpen;
		}

		// Slide Col or Turbo Track
		if (AH_WarpPad_IsRelicOnlyTrack(warppadObj->levelID))
		{
			warppadObj->boolEnteredWarppad = 1;
			warppadObj->framesWarping++;
			driver->funcPtrs[DRIVER_FUNC_INIT] = VehStuckProc_Warp_Init;
			if (warppadObj->framesWarping < AH_WP_WARP_LOAD_FRAMES)
			{
				goto WarpPad_AnimateOpen;
			}

			GAME_ADD_CONFIG_0 |= RELIC_RACE;
			GAME_REMOVE_CONFIG_0 |= ADVENTURE_ARENA;
			MainRaceTrack_RequestLoad(warppadObj->levelID);
			goto WarpPad_AnimateOpen;
		}

		// Battle Tracks
		if (AH_WarpPad_IsBattleTrack(destination))
		{
			warppadObj->boolEnteredWarppad = 1;
			warppadObj->framesWarping++;
			driver->funcPtrs[DRIVER_FUNC_INIT] = VehStuckProc_Warp_Init;
			if (warppadObj->framesWarping < AH_WP_WARP_LOAD_FRAMES)
			{
				goto WarpPad_AnimateOpen;
			}

			// Dont have hint "collect every crystal"
			if (!(GAME_ADV_PROGRESS.rewards[ADV_REWARD_HINT_COLLECT_EVERY_CRYSTAL / 32] & MEMCARD_BIT_MASK(ADV_REWARD_HINT_COLLECT_EVERY_CRYSTAL)))
			{
				MainFrame_RequestMaskHint(ADV_MASK_HINT_ID_COLLECT_EVERY_CRYSTAL, 1);
			}

			// Keep animating the portal while the crystal hint finishes.
			if (!(s16)AH_MaskHint_boolCanSpawn())
			{
				goto WarpPad_AnimateOpen;
			}

			GAME_REMOVE_CONFIG_0 |= ADVENTURE_ARENA;
			GAME_TRACKER->originalEventTime = AH_WARP_BATTLE_TIMES[warppadObj->levelID - AH_WP_ID_FIRST_BATTLE_TRACK];
			GAME_ADD_CONFIG_0 |= CRYSTAL_CHALLENGE;
			MainRaceTrack_RequestLoad(warppadObj->levelID);
			goto WarpPad_AnimateOpen;
		}

		if (destination >= AH_WP_ID_SLIDE_COLISEUM)
			goto WarpPad_OtherTrack;
		if (!(CHECK_ADV_BIT(GAME_ADV_PROGRESS.rewards, destination + ADV_REWARD_FIRST_TROPHY)))
			goto WarpPad_DefaultEnter;
		// Token/relic selection is unlocked by the hub's key requirement,
		// which the all-portals option waives along with the pad's own lock.
		if (!g_config.unlockAllPortals && (GAME_TRACKER->currAdvProfile.numKeys < AH_HUB_REQUIRED_KEYS[AH_LEVEL_METADATA[destination].hubID]))
			goto WarpPad_OtherTrack;
		if (warppadObj->framesWarping < AH_WP_WARP_LOAD_FRAMES)
		{
			goto WarpPad_TrophyAnimateOnly;
		}

		// if never opened
		if (AH_WARP_MENU_OPENED == 0)
		{
			if ((GAME_TRACKER->gameMode1 & ADVENTURE_ARENA) != 0)
			{
				AH_WARP_MENU.rowSelected = CHECK_ADV_BIT(GAME_ADV_PROGRESS.rewards, destination + ADV_REWARD_FIRST_CTR_TOKEN);

				RECTMENU_Show(&AH_WARP_MENU);

				// now opened
				AH_WARP_MENU_OPENED = 1;
			}
		}

		// if opened, but not closed yet
		if (!(s16)RECTMENU_BoolHidden(&AH_WARP_MENU))
		{
			goto WarpPad_TrophyAnimateOnly;
		}

		{
			s16 hintID;
			// Relic Hint
			hintID = ADV_MASK_HINT_ID_RELIC_CHALLENGE;

			// CTR Token Hint
			if ((GAME_TRACKER->gameMode2 & TOKEN_RACE) != 0)
			{
				hintID = ADV_MASK_HINT_ID_CTR_TOKEN_CHALLENGE;
			}

			// if hint is locked
			if (!CHECK_ADV_BIT(GAME_ADV_PROGRESS.rewards, ADV_REWARD_FIRST_HINT + hintID))
			{
				MainFrame_RequestMaskHint(hintID, 1);
			}
		}

		// Hold the warp animation until the chosen challenge hint finishes.
		if (!(s16)AH_MaskHint_boolCanSpawn())
		{
			goto WarpPad_TrophyAnimateOnly;
		}

		// reset for future gameplay
		AH_WARP_MENU_OPENED = 0;
		warppadObj->boolEnteredWarppad = 0;

		// Rem Adventure Arena
		GAME_REMOVE_CONFIG_0 |= ADVENTURE_ARENA;

		MainRaceTrack_RequestLoad(warppadObj->levelID);
	WarpPad_TrophyAnimateOnly:

		if (warppadObj->framesWarping < AH_WP_TROPHY_PORTAL_HOLD_FRAMES)
		{
			warppadObj->framesWarping++;
		}

		warppadObj->boolEnteredWarppad = 1;

		driver->funcPtrs[DRIVER_FUNC_INIT] = VehStuckProc_Warp_Init;

		goto WarpPad_AnimateOpen;
	WarpPad_OtherTrack:
		if (CHECK_ADV_BIT(GAME_ADV_PROGRESS.rewards, warppadObj->levelID + ADV_REWARD_FIRST_TROPHY))
		{
			s32 bossReward = AH_LEVEL_METADATA[warppadObj->levelID].hubID + ADV_REWARD_BOSS_KEY_HUB_ID_BASE;

			if (!CHECK_ADV_BIT(GAME_ADV_PROGRESS.rewards, bossReward))
			{
				goto WarpPad_AnimateOpen;
			}
		}

	WarpPad_DefaultEnter:
		warppadObj->boolEnteredWarppad = 1;
		warppadObj->framesWarping++;
		driver->funcPtrs[DRIVER_FUNC_INIT] = VehStuckProc_Warp_Init;
		if (warppadObj->framesWarping < AH_WP_WARP_LOAD_FRAMES)
		{
			goto WarpPad_AnimateOpen;
		}

		// Rem Adventure Arena
		GAME_REMOVE_CONFIG_0 |= ADVENTURE_ARENA;

		MainRaceTrack_RequestLoad(warppadObj->levelID);
		goto WarpPad_AnimateOpen;
	}
WarpPad_AnimateOpen:

	if ((warppadObj->slots.inst[WPIS_OPEN_BEAM] != 0) && ((GAME_TRACKER->timer & 1) != 0))
	{
		s32 beamRng;
		warppadObj->spinRot_Beam.x = 0;
		// Randomized beam rotation advances only on odd frames.
		beamRng = MixRNG_Scramble();
		warppadObj->spinRot_Beam.z = 0;
		warppadObj->spinRot_Beam.y += ((beamRng >> 3) % 6 + 1) * AH_WP_SPIN_BEAM_STEP;

		ConvertRotToMatrix(&warppadObj->slots.inst[WPIS_OPEN_BEAM]->matrix, &warppadObj->spinRot_Beam);
	}

	i = 0;


	for (; i < AH_WP_WISP_COUNT; i++)
	{
		wispRiseRate = i * AH_WP_WISP_RISE_RATE_STEP + AH_WP_WISP_RISE_RATE;

		if (warppadObj->slots.visuals.rings[i] != 0)
		{
			wispMaxHeight = AH_WP_WISP_FAR_MAX_HEIGHT;

			// if close to this warppad
			if (AH_WARP_NEAREST_ID != -1)
			{
				wispMaxHeight = AH_WP_WISP_NEAR_MAX_HEIGHT;
			}

			warppadObj->spinRot_Wisp[i].x = 0;
			warppadObj->spinRot_Wisp[i].z = 0;

			warppadObj->spinRot_Wisp[i].y += AH_WP_SPIN_WISP_STEP;

			ConvertRotToMatrix(&warppadObj->slots.visuals.rings[i]->matrix,
			                   (SVec3 *)((u32)warppadObj + offsetof(struct WarpPad, spinRot_Wisp) + i * sizeof(SVec3Slot)));

			// if height hasn't reached max height
			if (warppadObj->slots.visuals.rings[i]->matrix.t[1] < (warppadInst->matrix.t[1] + wispMaxHeight))
			{
				warppadObj->slots.visuals.rings[i]->matrix.t[1] += wispRiseRate;

				// if height hasn't reached 4x RiseRate,
				// first 4 frames of rising
				if (warppadObj->slots.visuals.rings[i]->matrix.t[1] < (warppadInst->matrix.t[1] + wispRiseRate * AH_WP_WISP_FIRST_FRAMES))
				{
					// reduce transparency
					warppadObj->slots.visuals.rings[i]->alphaScale -= AH_WP_WISP_FADE_IN_STEP;
				}

				// after first 4 frames
				else
				{
					// add transparency as the wisp spirals upward (~0x60  per frame)
					warppadObj->slots.visuals.rings[i]->alphaScale += AH_WP_WISP_FADE_OUT_RANGE / (wispMaxHeight / wispRiseRate);
				}
			}

			// eached max height
			else
			{
				// reset height
				warppadObj->slots.visuals.rings[i]->matrix.t[1] = warppadInst->matrix.t[1];

				// full transparency
				warppadObj->slots.visuals.rings[i]->alphaScale = AH_WP_FULL_ALPHA;

				rng1 = MixRNG_Scramble() >> 3;

				resetQuotient = rng1;
				if (rng1 < 0)
				{
					resetQuotient = rng1 + 0xfff;
				}

				warppadObj->spinRot_Wisp[i].y = rng1 + (resetQuotient >> 0xc) * -AH_WP_FULL_ALPHA;
			}
		}
	}

	warppadObj->spinRot_Prize.y += AH_WP_SPIN_PRIZE_REWARD_STEP;


	if (dist > AH_WP_REWARD_FADE_DIST_SQ)
	{
		s32 fadeDistance = dist - AH_WP_REWARD_FADE_DIST_SQ;
		if (fadeDistance > AH_WP_REWARD_FADE_DIST_SQ)
			fadeDistance = AH_WP_REWARD_FADE_DIST_SQ;
		rewardScale = (CTR_MipsMulLo(AH_WP_REWARD_FADE_DIST_SQ - fadeDistance, 256)) / AH_WP_REWARD_FADE_DIST_SQ;
	}

	for (i = 0; i < AH_WP_REWARD_INSTANCE_COUNT; i++)
	{
		warppadObj->spinRot_Prize.z = AH_WP_PRIZE_TILT_Z;

		if (warppadObj->slots.visuals.prizes[i] != 0)
		{
			s32 prizeModel;
			// NOTE(aalhendi): Position is a by-value vector, split between
			// the last argument register and the stack on MIPS.
			AH_WarpPad_SpinRewards(warppadObj->slots.visuals.prizes[i], warppadObj, i, *(const Vec3 *)warppadInst->matrix.t);

			prizeModel = warppadObj->slots.visuals.prizes[i]->model->id;

			rewardScale2 = AH_WP_STANDARD_ITEM_SCALE;
			if (prizeModel != STATIC_TOKEN)
			{
				rewardScale2 = AH_WP_TROPHY_PRIZE_SCALE;
				if (prizeModel == STATIC_RELIC)
					rewardScale2 = AH_WP_RELIC_PRIZE_SCALE;
			}
			rewardScale2 = (rewardScale2 * rewardScale) >> 8;
			warppadObj->slots.visuals.prizes[i]->scale.x = rewardScale2;
			warppadObj->slots.visuals.prizes[i]->scale.y = rewardScale2;
			warppadObj->slots.visuals.prizes[i]->scale.z = rewardScale2;
			if (rewardScale == 0)
				warppadObj->slots.visuals.prizes[i]->flags |= HIDE_MODEL;
			else
				warppadObj->slots.visuals.prizes[i]->flags &= ~HIDE_MODEL;
		}

		warppadObj->thirds[i] += AH_WP_REWARD_PHASE_STEP;
		warppadObj->spinRot_Rewards.y += AH_WP_SPIN_REWARD_RING_STEP;
	}

	if (warppadObj->slots.inst[WPIS_CLOSED_1S] != 0)
	{
		INSTANCE_Death(warppadObj->slots.inst[WPIS_CLOSED_1S]);
		INSTANCE_Death(warppadObj->slots.inst[WPIS_CLOSED_10S]);
		INSTANCE_Death(warppadObj->slots.inst[WPIS_CLOSED_X]);
		INSTANCE_Death(warppadObj->slots.inst[WPIS_CLOSED_ITEM]);
	}
}

void AH_WarpPad_ThDestroy(struct Thread *t)
{
	struct WarpPad *warppadObj = t->object;

	// array of instances in warppad object
	struct Instance **instArr = &warppadObj->slots.inst[0];

	if (instArr[WPIS_CLOSED_1S] != 0)
	{
		INSTANCE_Death(instArr[WPIS_CLOSED_1S]);
		instArr[WPIS_CLOSED_1S] = 0;
	}

	if (instArr[WPIS_CLOSED_10S] != 0)
	{
		INSTANCE_Death(instArr[WPIS_CLOSED_10S]);
		instArr[WPIS_CLOSED_10S] = 0;
	}

	if (instArr[WPIS_CLOSED_X] != 0)
	{
		INSTANCE_Death(instArr[WPIS_CLOSED_X]);
		instArr[WPIS_CLOSED_X] = 0;
	}

	if (instArr[WPIS_CLOSED_ITEM] != 0)
	{
		INSTANCE_Death(instArr[WPIS_CLOSED_ITEM]);
		instArr[WPIS_CLOSED_ITEM] = 0;
	}

	if (instArr[WPIS_OPEN_BEAM] != 0)
	{
		INSTANCE_Death(instArr[WPIS_OPEN_BEAM]);
		instArr[WPIS_OPEN_BEAM] = 0;
	}

	{
		int i;
		for (i = 0; i < WPIS_OPEN_PRIZE1 - WPIS_OPEN_RING1; i++)
		{
			if (instArr[WPIS_OPEN_RING1 + i] != 0)
			{
				INSTANCE_Death(instArr[WPIS_OPEN_RING1 + i]);
				instArr[WPIS_OPEN_RING1 + i] = 0;
			}
		}
	}

	{
		int i;
		for (i = 0; i < WPIS_NUM_INSTANCES - WPIS_OPEN_PRIZE1; i++)
		{
			if (instArr[WPIS_OPEN_PRIZE1 + i] != 0)
			{
				INSTANCE_Death(instArr[WPIS_OPEN_PRIZE1 + i]);
				instArr[WPIS_OPEN_PRIZE1 + i] = 0;
			}
		}
	}
}

void AH_WarpPad_LInB(struct Instance *inst)
{
	s32 i;
	s32 digitModelID;

	u32 *rewards;
	s16 nameIndex;

	struct Thread *t;
	struct WarpPad *warppadObj;


	s32 unlockItem_modelID = 0;
	s32 unlockItem_numNeeded = -1;
	s32 unlockItem_numOwned = 0;
	s16 tokenGroupID;


	t = inst->thread;
	if (t != NULL)
	{
		return;
	}

	inst->thread = PROC_BirthWithObject(SIZE_RELATIVE_POOL_BUCKET(sizeof(struct WarpPad), NONE, MEDIUM, WARPPAD),

	                                    AH_WarpPad_ThTick, // behavior
	                                    "warppad",         // debug name
	                                    0                  // thread relative
	);

	t = inst->thread;
	if (t == 0)
	{
		return;
	}
	t->inst = inst;

	t->funcThDestroy = AH_WarpPad_ThDestroy;

	// locked
	t->modelIndex = AH_WP_VISUAL_LOCKED;

	// make invisible
	// this is the red triangle
	// instance from DCxDemo's LEV Viewer
	warppadObj = t->object;
	inst->flags |= HIDE_MODEL;
	warppadObj->levelID = 0; // this is dingo canyon
	warppadObj->boolEnteredWarppad = 0;
	warppadObj->framesWarping = 0;


	for (nameIndex = AH_WP_NAME_LEVEL_ID_OFFSET; inst->name[nameIndex] != 0; nameIndex++)
	{
		warppadObj->levelID *= 10;
		warppadObj->levelID += (s16)((u8)inst->name[nameIndex] - '0');
	}

	// Trophy Track
	if (warppadObj->levelID < AH_WP_ID_SLIDE_COLISEUM)
	{
		// if trophy owned
		if (CHECK_ADV_BIT(GAME_ADV_PROGRESS.rewards, warppadObj->levelID + ADV_REWARD_FIRST_TROPHY))
		{
			// keys needed to unlock track again
			unlockItem_modelID = STATIC_KEY;
			unlockItem_numOwned = GAME_TRACKER->currAdvProfile.numKeys;
			unlockItem_numNeeded = AH_HUB_REQUIRED_KEYS[AH_LEVEL_METADATA[warppadObj->levelID].hubID];
		}

		// if trophy not owned
		else
		{
			// number trophies needed to open
			unlockItem_modelID = STATIC_TROPHY;
			unlockItem_numOwned = GAME_TRACKER->currAdvProfile.numTrophies;
		}
	}

	// Slide Col
	else if (warppadObj->levelID == AH_WP_ID_SLIDE_COLISEUM)
	{
		// number relics needed to open
		unlockItem_modelID = STATIC_RELIC;
		unlockItem_numOwned = GAME_TRACKER->currAdvProfile.numRelics;
	}

	// Turbo Track
	else if (warppadObj->levelID == AH_WP_ID_TURBO_TRACK)
	{
		// number gems needed to open
		unlockItem_modelID = STATIC_GEM;

		// count number of gems owned
		for (i = 0; i < AH_WP_GEM_CUP_COUNT; i++)
		{
			if (CHECK_ADV_BIT(GAME_ADV_PROGRESS.rewards, ADV_REWARD_FIRST_GEM + i))
			{
				unlockItem_numOwned++;
			}
		}
	}

	// battle maps
	else if (AH_WarpPad_IsBattleTrack(warppadObj->levelID))
	{
		unlockItem_modelID = STATIC_KEY;
		unlockItem_numOwned = GAME_TRACKER->currAdvProfile.numKeys;
	}

	// gem cups
	else if (((u16)(warppadObj->levelID - AH_WP_ID_FIRST_GEM_CUP)) < AH_WP_GEM_CUP_COUNT)
	{
		const u8 *arrTokenCount;
		// number tokens needed to open
		unlockItem_modelID = STATIC_TOKEN;

		// NOTE(aalhendi): The five color counts follow total. Read through the
		// owning struct's bytes instead of indexing past its scalar red member.
		arrTokenCount = (const u8 *)&GAME_TRACKER->currAdvProfile.numCtrTokens;
		unlockItem_numOwned = (s32)CTR_ReadU32AlignedLE(arrTokenCount + sizeof(s32) + (s16)(warppadObj->levelID - AH_WP_ID_FIRST_GEM_CUP) * sizeof(s32));
	}


	if (unlockItem_numNeeded < 0)
	{
		if (unlockItem_modelID == STATIC_TOKEN)
			unlockItem_numNeeded = 4;
		else if (unlockItem_modelID == STATIC_GEM)
			unlockItem_numNeeded = 5;
		else
			unlockItem_numNeeded = AH_LEVEL_METADATA[warppadObj->levelID].numTrophiesToOpen;
	}
	if (!g_config.unlockAllPortals && unlockItem_numOwned < unlockItem_numNeeded)
	{
		// NOTE(aalhendi): Pool objects are not zeroed. Each visual path initializes
		// its unused slots; the destroy callback may later visit all ten.

		if (unlockItem_numNeeded < 10)
		{
			warppadObj->digit10s = 0;
			warppadObj->digit1s = unlockItem_numNeeded;
		}

		else
		{
			warppadObj->digit10s = 1;
			warppadObj->digit1s = unlockItem_numNeeded - 10;
		}

		// ====== Item ========

		// WPIS_CLOSED_ITEM
		warppadObj->slots.inst[WPIS_CLOSED_ITEM] = INSTANCE_Birth3D(GAME_TRACKER->modelPtr[unlockItem_modelID], "reqObj", t);

		warppadObj->slots.inst[WPIS_CLOSED_ITEM]->matrix = inst->matrix;
		warppadObj->slots.inst[WPIS_CLOSED_ITEM]->matrix.t[1] += AH_WP_FLOATING_OBJECT_HEIGHT;

		warppadObj->slots.inst[WPIS_CLOSED_ITEM]->scale.x = AH_WP_STANDARD_ITEM_SCALE;
		warppadObj->slots.inst[WPIS_CLOSED_ITEM]->scale.y = AH_WP_STANDARD_ITEM_SCALE;
		warppadObj->slots.inst[WPIS_CLOSED_ITEM]->scale.z = AH_WP_STANDARD_ITEM_SCALE;

		// no specular for trophy
		if (unlockItem_modelID != STATIC_TROPHY)
		{
			// relic
			if (unlockItem_modelID == STATIC_RELIC)
			{
				// Relic blue color
				warppadObj->slots.inst[WPIS_CLOSED_ITEM]->colorRGBA = INST_COLOR_SAPPHIRE_RELIC;

				warppadObj->lightDirRelic.x = (AH_WARP_LIGHT_RELIC[0]).x;
				warppadObj->lightDirRelic.y = (AH_WARP_LIGHT_RELIC[0]).y;
				warppadObj->lightDirRelic.z = (AH_WARP_LIGHT_RELIC[0]).z;
				warppadObj->slots.inst[WPIS_CLOSED_ITEM]->flags |= USE_SPECULAR_LIGHT;
			}

			// Key
			else if (unlockItem_modelID == STATIC_KEY)
			{
				// Key color
				warppadObj->slots.inst[WPIS_CLOSED_ITEM]->colorRGBA = INST_COLOR_KEY;

				// store in Gem array (intended by ND, not a bug)
				warppadObj->lightDirGem.x = (AH_WARP_LIGHT_GEM[0]).x;
				warppadObj->lightDirGem.y = (AH_WARP_LIGHT_GEM[0]).y;
				warppadObj->lightDirGem.z = (AH_WARP_LIGHT_GEM[0]).z;
				warppadObj->slots.inst[WPIS_CLOSED_ITEM]->flags |= USE_SPECULAR_LIGHT;
			}

			// Gem
			else if (unlockItem_modelID == STATIC_GEM)
			{
				warppadObj->slots.inst[WPIS_CLOSED_ITEM]->colorRGBA =
				    INST_PackColorRGB(AH_WARP_CUPS[0].color[0], AH_WARP_CUPS[0].color[1], AH_WARP_CUPS[0].color[2]);

				// store in Gem array
				warppadObj->lightDirGem.x = (AH_WARP_LIGHT_GEM[0]).x;
				warppadObj->lightDirGem.y = (AH_WARP_LIGHT_GEM[0]).y;
				warppadObj->lightDirGem.z = (AH_WARP_LIGHT_GEM[0]).z;
				warppadObj->slots.inst[WPIS_CLOSED_ITEM]->flags |= USE_SPECULAR_LIGHT;
			}

			// token
			else if (unlockItem_modelID == STATIC_TOKEN)
			{
				tokenGroupID = warppadObj->levelID - AH_WP_ID_FIRST_GEM_CUP;

				// token color
				warppadObj->slots.inst[WPIS_CLOSED_ITEM]->colorRGBA =
				    INST_PackColorRGB(AH_WARP_CUPS[tokenGroupID].color[0], AH_WARP_CUPS[tokenGroupID].color[1], AH_WARP_CUPS[tokenGroupID].color[2]);

				warppadObj->lightDirToken.x = (AH_WARP_LIGHT_TOKEN[tokenGroupID]).x;
				warppadObj->lightDirToken.y = (AH_WARP_LIGHT_TOKEN[tokenGroupID]).y;
				warppadObj->lightDirToken.z = (AH_WARP_LIGHT_TOKEN[tokenGroupID]).z;
				warppadObj->slots.inst[WPIS_CLOSED_ITEM]->flags |= USE_SPECULAR_LIGHT;
			}
		}


		// NOTE(aalhendi): Keep these slot-based stores explicit. Passing a cached
		// MATRIX pointer to the identity helper loses retail's intervening reloads.
		warppadObj->slots.inst[WPIS_CLOSED_X] = INSTANCE_Birth3D(GAME_TRACKER->modelPtr[STATIC_BIGX], "x", t);

		warppadObj->slots.inst[WPIS_CLOSED_X]->matrix.t[0] = inst->matrix.t[0];
		warppadObj->slots.inst[WPIS_CLOSED_X]->matrix.t[1] = inst->matrix.t[1];
		warppadObj->slots.inst[WPIS_CLOSED_X]->matrix.t[2] = inst->matrix.t[2];
		CTR_WriteU32AlignedLE(&warppadObj->slots.inst[WPIS_CLOSED_X]->matrix.m[0][0], 0x1000);
		CTR_WriteU32AlignedLE(&warppadObj->slots.inst[WPIS_CLOSED_X]->matrix.m[0][2], 0);
		CTR_WriteU32AlignedLE(&warppadObj->slots.inst[WPIS_CLOSED_X]->matrix.m[1][1], 0x1000);
		CTR_WriteU32AlignedLE(&warppadObj->slots.inst[WPIS_CLOSED_X]->matrix.m[2][0], 0);
		warppadObj->slots.inst[WPIS_CLOSED_X]->matrix.m[2][2] = 0x1000;
		warppadObj->slots.inst[WPIS_CLOSED_X]->matrix.t[1] += AH_WP_FLOATING_OBJECT_HEIGHT;

		warppadObj->slots.inst[WPIS_CLOSED_X]->scale.x = AH_WP_STANDARD_ITEM_SCALE;
		warppadObj->slots.inst[WPIS_CLOSED_X]->scale.y = AH_WP_STANDARD_ITEM_SCALE;
		warppadObj->slots.inst[WPIS_CLOSED_X]->scale.z = AH_WP_STANDARD_ITEM_SCALE;

		// always face camera
		warppadObj->slots.inst[WPIS_CLOSED_X]->model->headers[0].flags |= 1;


		// ====== "10s" ========

		if (warppadObj->digit10s != 0)
		{
			// WPIS_CLOSED_10S
			warppadObj->slots.inst[WPIS_CLOSED_10S] = INSTANCE_Birth3D(GAME_TRACKER->modelPtr[STATIC_BIG1], "warpnum", t);

			warppadObj->slots.inst[WPIS_CLOSED_10S]->matrix.t[0] = inst->matrix.t[0];
			warppadObj->slots.inst[WPIS_CLOSED_10S]->matrix.t[1] = inst->matrix.t[1];
			warppadObj->slots.inst[WPIS_CLOSED_10S]->matrix.t[2] = inst->matrix.t[2];
			CTR_WriteU32AlignedLE(&warppadObj->slots.inst[WPIS_CLOSED_10S]->matrix.m[0][0], 0x1000);
			CTR_WriteU32AlignedLE(&warppadObj->slots.inst[WPIS_CLOSED_10S]->matrix.m[0][2], 0);
			CTR_WriteU32AlignedLE(&warppadObj->slots.inst[WPIS_CLOSED_10S]->matrix.m[1][1], 0x1000);
			CTR_WriteU32AlignedLE(&warppadObj->slots.inst[WPIS_CLOSED_10S]->matrix.m[2][0], 0);
			warppadObj->slots.inst[WPIS_CLOSED_10S]->matrix.m[2][2] = 0x1000;
			warppadObj->slots.inst[WPIS_CLOSED_10S]->matrix.t[1] += AH_WP_FLOATING_OBJECT_HEIGHT;

			warppadObj->slots.inst[WPIS_CLOSED_10S]->scale.x = AH_WP_STANDARD_ITEM_SCALE;
			warppadObj->slots.inst[WPIS_CLOSED_10S]->scale.y = AH_WP_STANDARD_ITEM_SCALE;
			warppadObj->slots.inst[WPIS_CLOSED_10S]->scale.z = AH_WP_STANDARD_ITEM_SCALE;

			// always face camera
			for (i = 0; i < warppadObj->slots.inst[WPIS_CLOSED_10S]->model->numHeaders; i++)
			{
				warppadObj->slots.inst[WPIS_CLOSED_10S]->model->headers[i].flags |= 1;
			}
		}

		else
		{
			warppadObj->slots.inst[WPIS_CLOSED_10S] = 0;
		}

		// ====== "1s" ========

		// STATIC_BIG (1-8)
		digitModelID = AH_WP_DIGIT_MODEL_FIRST_1_TO_8;
		if (warppadObj->digit1s == 0)
		{
			digitModelID = AH_WP_DIGIT_MODEL_0;
		}
		else if (warppadObj->digit1s == 9)
		{
			digitModelID = AH_WP_DIGIT_MODEL_9;
		}

		// WPIS_CLOSED_1S
		warppadObj->slots.inst[WPIS_CLOSED_1S] = INSTANCE_Birth3D(GAME_TRACKER->modelPtr[digitModelID], "warpnum", t);

		warppadObj->slots.inst[WPIS_CLOSED_1S]->matrix.t[0] = inst->matrix.t[0];
		warppadObj->slots.inst[WPIS_CLOSED_1S]->matrix.t[1] = inst->matrix.t[1];
		warppadObj->slots.inst[WPIS_CLOSED_1S]->matrix.t[2] = inst->matrix.t[2];
		CTR_WriteU32AlignedLE(&warppadObj->slots.inst[WPIS_CLOSED_1S]->matrix.m[0][0], 0x1000);
		CTR_WriteU32AlignedLE(&warppadObj->slots.inst[WPIS_CLOSED_1S]->matrix.m[0][2], 0);
		CTR_WriteU32AlignedLE(&warppadObj->slots.inst[WPIS_CLOSED_1S]->matrix.m[1][1], 0x1000);
		CTR_WriteU32AlignedLE(&warppadObj->slots.inst[WPIS_CLOSED_1S]->matrix.m[2][0], 0);
		warppadObj->slots.inst[WPIS_CLOSED_1S]->matrix.m[2][2] = 0x1000;
		warppadObj->slots.inst[WPIS_CLOSED_1S]->matrix.t[1] += AH_WP_FLOATING_OBJECT_HEIGHT;

		warppadObj->slots.inst[WPIS_CLOSED_1S]->scale.x = AH_WP_STANDARD_ITEM_SCALE;
		warppadObj->slots.inst[WPIS_CLOSED_1S]->scale.y = AH_WP_STANDARD_ITEM_SCALE;
		warppadObj->slots.inst[WPIS_CLOSED_1S]->scale.z = AH_WP_STANDARD_ITEM_SCALE;

		// always face camera
		for (i = 0; i < warppadObj->slots.inst[WPIS_CLOSED_1S]->model->numHeaders; i++)
		{
			warppadObj->slots.inst[WPIS_CLOSED_1S]->model->headers[i].flags |= 1;
		}


		warppadObj->slots.inst[WPIS_OPEN_BEAM] = 0;
		for (i = AH_WP_WISP_COUNT - 1; i >= 0; i--)
			warppadObj->slots.visuals.rings[i] = 0;
		for (i = AH_WP_REWARD_INSTANCE_COUNT - 1; i >= 0; i--)
			warppadObj->slots.visuals.prizes[i] = 0;
		return;
	}
	warppadObj->slots.inst[WPIS_CLOSED_1S] = 0;
	warppadObj->slots.inst[WPIS_CLOSED_10S] = 0;
	warppadObj->slots.inst[WPIS_CLOSED_X] = 0;
	warppadObj->slots.inst[WPIS_CLOSED_ITEM] = 0;

	warppadObj->digit1s = 0;
	t->modelIndex = AH_WP_VISUAL_TROPHY_OPEN;

	// if beam model exists
	if (GAME_TRACKER->modelPtr[STATIC_BEAM] != 0)
	{
		warppadObj->slots.inst[WPIS_OPEN_BEAM] = INSTANCE_Birth3D(GAME_TRACKER->modelPtr[STATIC_BEAM], "beam", t);

		warppadObj->slots.inst[WPIS_OPEN_BEAM]->matrix = inst->matrix;
		warppadObj->slots.inst[WPIS_OPEN_BEAM]->matrix.t[0] = inst->matrix.t[0];
		warppadObj->slots.inst[WPIS_OPEN_BEAM]->matrix.t[1] = inst->matrix.t[1];
		warppadObj->slots.inst[WPIS_OPEN_BEAM]->matrix.t[2] = inst->matrix.t[2];

		warppadObj->slots.inst[WPIS_OPEN_BEAM]->alphaScale = AH_WP_OPEN_BEAM_ALPHA;
	}

	else
		warppadObj->slots.inst[WPIS_OPEN_BEAM] = 0;

	// if spiral ring exists
	for (i = 0; i < AH_WP_WISP_COUNT; i++)
	{
		if (GAME_TRACKER->modelPtr[STATIC_BOTTOMRING] != 0)
		{
			warppadObj->slots.visuals.rings[i] = INSTANCE_Birth3D(GAME_TRACKER->modelPtr[STATIC_BOTTOMRING], "bottomRing", t);

			warppadObj->slots.visuals.rings[i]->matrix = inst->matrix;
			warppadObj->slots.visuals.rings[i]->matrix.t[0] = inst->matrix.t[0];
			warppadObj->slots.visuals.rings[i]->matrix.t[1] = inst->matrix.t[1] + i * AH_WP_OPEN_RING_HEIGHT_STEP;
			warppadObj->slots.visuals.rings[i]->matrix.t[2] = inst->matrix.t[2];

			warppadObj->slots.visuals.rings[i]->alphaScale = AH_WP_OPEN_RING_ALPHA;
		}
		else
			warppadObj->slots.visuals.rings[i] = 0;
	}

	for (i = 0; i < AH_WP_REWARD_INSTANCE_COUNT; i++)
	{
		warppadObj->slots.visuals.prizes[i] = 0;
		warppadObj->thirds[i] = AH_WP_REWARD_SPACING_ANGLE * i;
	}

	warppadObj->spinRot_Prize.x = 0;
	warppadObj->spinRot_Prize.y = 0;
	warppadObj->spinRot_Prize.z = 0;

	warppadObj->spinRot_Beam.x = 0;
	warppadObj->spinRot_Beam.y = 0;
	warppadObj->spinRot_Beam.z = 0;

	for (i = 0; i < AH_WP_WISP_COUNT; i++)
	{
		warppadObj->spinRot_Wisp[i].x = 0;
		warppadObj->spinRot_Wisp[i].y = 0;
		warppadObj->spinRot_Wisp[i].z = 0;
	}

	if (warppadObj->levelID < AH_WP_ID_SLIDE_COLISEUM)
	{
		rewards = GAME_ADV_PROGRESS.rewards;
		// if trophy not owned
		if (!CHECK_ADV_BIT(rewards, warppadObj->levelID + ADV_REWARD_FIRST_TROPHY))
		{
			s32 tokenIndex;
			s16 rewardModels[3] = {STATIC_TROPHY, STATIC_RELIC, STATIC_TOKEN};

			for (i = 0; i < AH_WP_REWARD_INSTANCE_COUNT; i++)
			{
				s32 modelID = rewardModels[i];
				warppadObj->slots.visuals.prizes[i] = INSTANCE_Birth3D(GAME_TRACKER->modelPtr[modelID], "prize1", t);


				warppadObj->slots.visuals.prizes[i]->matrix = inst->matrix;
				warppadObj->slots.visuals.prizes[i]->matrix.t[0] =
				    inst->matrix.t[0] + ((MATH_Sin(i * AH_WP_REWARD_SPACING_ANGLE) * AH_WP_PRIZE_RING_RADIUS) >> 0xc);
				warppadObj->slots.visuals.prizes[i]->matrix.t[1] = inst->matrix.t[1] + AH_WP_FLOATING_OBJECT_HEIGHT;
				warppadObj->slots.visuals.prizes[i]->matrix.t[2] =
				    inst->matrix.t[2] + ((MATH_Cos(i * AH_WP_REWARD_SPACING_ANGLE) * AH_WP_PRIZE_RING_RADIUS) >> 0xc);

				if (rewardModels[i] == STATIC_RELIC)
				{
					warppadObj->slots.visuals.prizes[i]->colorRGBA = INST_COLOR_SAPPHIRE_RELIC;
					warppadObj->slots.visuals.prizes[i]->flags |= USE_SPECULAR_LIGHT;
					warppadObj->slots.visuals.prizes[i]->scale.x = AH_WP_RELIC_PRIZE_SCALE;
					warppadObj->slots.visuals.prizes[i]->scale.y = AH_WP_RELIC_PRIZE_SCALE;
					warppadObj->slots.visuals.prizes[i]->scale.z = AH_WP_RELIC_PRIZE_SCALE;
				}

				else if (rewardModels[i] == STATIC_TOKEN)
				{
					{
						const struct MetaDataLEV *level = AH_LEVEL_METADATA;
						__typeof__(&AH_WARP_CUPS[0]) cups = AH_WARP_CUPS;
						tokenIndex = level[warppadObj->levelID].ctrTokenGroupID;

						// token color
						warppadObj->slots.visuals.prizes[i]->colorRGBA =
						    INST_PackColorRGB(cups[tokenIndex].color[0], cups[tokenIndex].color[1], cups[tokenIndex].color[2]);

						warppadObj->slots.visuals.prizes[i]->flags |= (DRAW_TRANSPARENT | USE_SPECULAR_LIGHT);
					}
					{
						warppadObj->lightDirToken.x = AH_WARP_LIGHT_TOKEN[tokenIndex].x;
						warppadObj->lightDirToken.y = AH_WARP_LIGHT_TOKEN[tokenIndex].y;
						warppadObj->lightDirToken.z = AH_WARP_LIGHT_TOKEN[tokenIndex].z;
					}

					warppadObj->slots.visuals.prizes[i]->scale.x = AH_WP_STANDARD_ITEM_SCALE;
					warppadObj->slots.visuals.prizes[i]->scale.y = AH_WP_STANDARD_ITEM_SCALE;
					warppadObj->slots.visuals.prizes[i]->scale.z = AH_WP_STANDARD_ITEM_SCALE;
				}

				else
				{
					warppadObj->slots.visuals.prizes[i]->scale.x = AH_WP_TROPHY_PRIZE_SCALE;
					warppadObj->slots.visuals.prizes[i]->scale.y = AH_WP_TROPHY_PRIZE_SCALE;
					warppadObj->slots.visuals.prizes[i]->scale.z = AH_WP_TROPHY_PRIZE_SCALE;
				}
			}

			return;
		}

		t->modelIndex = AH_WP_VISUAL_COMPLETE;

		// if relic not owned
		if (!CHECK_ADV_BIT(rewards, warppadObj->levelID + ADV_REWARD_FIRST_SAPPHIRE_RELIC))
		{
			// open for relic/token
			t->modelIndex = AH_WP_VISUAL_RELIC_TOKEN_OPEN;

			warppadObj->slots.inst[WPIS_OPEN_PRIZE1] = INSTANCE_Birth3D(GAME_TRACKER->modelPtr[STATIC_RELIC], "prize2", t);

			// relic blue
			warppadObj->slots.inst[WPIS_OPEN_PRIZE1]->colorRGBA = INST_COLOR_SAPPHIRE_RELIC;

			warppadObj->slots.inst[WPIS_OPEN_PRIZE1]->flags |= USE_SPECULAR_LIGHT;

			warppadObj->lightDirRelic.x = (AH_WARP_LIGHT_RELIC[0]).x;
			warppadObj->lightDirRelic.y = (AH_WARP_LIGHT_RELIC[0]).y;
			warppadObj->lightDirRelic.z = (AH_WARP_LIGHT_RELIC[0]).z;

			warppadObj->slots.inst[WPIS_OPEN_PRIZE1]->matrix = inst->matrix;
			warppadObj->slots.inst[WPIS_OPEN_PRIZE1]->matrix.t[0] = inst->matrix.t[0];
			warppadObj->slots.inst[WPIS_OPEN_PRIZE1]->matrix.t[1] = inst->matrix.t[1] + AH_WP_FLOATING_OBJECT_HEIGHT;
			warppadObj->slots.inst[WPIS_OPEN_PRIZE1]->matrix.t[2] = inst->matrix.t[2];

			warppadObj->slots.inst[WPIS_OPEN_PRIZE1]->scale.x = AH_WP_RELIC_PRIZE_SCALE;
			warppadObj->slots.inst[WPIS_OPEN_PRIZE1]->scale.y = AH_WP_RELIC_PRIZE_SCALE;
			warppadObj->slots.inst[WPIS_OPEN_PRIZE1]->scale.z = AH_WP_RELIC_PRIZE_SCALE;
		}

		// if token owned
		if (CHECK_ADV_BIT(rewards, warppadObj->levelID + ADV_REWARD_FIRST_CTR_TOKEN))
		{
			return;
		}

		tokenGroupID = AH_LEVEL_METADATA[warppadObj->levelID].ctrTokenGroupID;

		// open for relic/token
		t->modelIndex = AH_WP_VISUAL_RELIC_TOKEN_OPEN;

		warppadObj->slots.inst[WPIS_OPEN_PRIZE2] = INSTANCE_Birth3D(GAME_TRACKER->modelPtr[STATIC_TOKEN], "prize2", t);

		// token color
		warppadObj->slots.inst[WPIS_OPEN_PRIZE2]->colorRGBA =
		    INST_PackColorRGB(AH_WARP_CUPS[tokenGroupID].color[0], AH_WARP_CUPS[tokenGroupID].color[1], AH_WARP_CUPS[tokenGroupID].color[2]);

		warppadObj->slots.inst[WPIS_OPEN_PRIZE2]->flags |= (DRAW_TRANSPARENT | USE_SPECULAR_LIGHT);

		warppadObj->lightDirToken.x = (AH_WARP_LIGHT_TOKEN[tokenGroupID]).x;
		warppadObj->lightDirToken.y = (AH_WARP_LIGHT_TOKEN[tokenGroupID]).y;
		warppadObj->lightDirToken.z = (AH_WARP_LIGHT_TOKEN[tokenGroupID]).z;

		warppadObj->slots.inst[WPIS_OPEN_PRIZE2]->matrix = inst->matrix;
		warppadObj->slots.inst[WPIS_OPEN_PRIZE2]->matrix.t[0] = inst->matrix.t[0];
		warppadObj->slots.inst[WPIS_OPEN_PRIZE2]->matrix.t[1] = inst->matrix.t[1] + AH_WP_FLOATING_OBJECT_HEIGHT;
		warppadObj->slots.inst[WPIS_OPEN_PRIZE2]->matrix.t[2] = inst->matrix.t[2];

		warppadObj->slots.inst[WPIS_OPEN_PRIZE2]->scale.x = AH_WP_STANDARD_ITEM_SCALE;
		warppadObj->slots.inst[WPIS_OPEN_PRIZE2]->scale.y = AH_WP_STANDARD_ITEM_SCALE;
		warppadObj->slots.inst[WPIS_OPEN_PRIZE2]->scale.z = AH_WP_STANDARD_ITEM_SCALE;


		return;
	}

	else if ((u16)(warppadObj->levelID - AH_WP_ID_SLIDE_COLISEUM) < 2)
	{
		t->modelIndex = AH_WP_VISUAL_COMPLETE;
		if (!CHECK_ADV_BIT(GAME_ADV_PROGRESS.rewards, warppadObj->levelID + ADV_REWARD_FIRST_SAPPHIRE_RELIC))
		{
			t->modelIndex = AH_WP_VISUAL_COLOR_CYCLE_OPEN;
			warppadObj->slots.inst[WPIS_OPEN_PRIZE1] = INSTANCE_Birth3D(GAME_TRACKER->modelPtr[STATIC_RELIC], "prize2", t);
			warppadObj->slots.inst[WPIS_OPEN_PRIZE1]->colorRGBA = INST_COLOR_SAPPHIRE_RELIC;
			warppadObj->slots.inst[WPIS_OPEN_PRIZE1]->flags |= USE_SPECULAR_LIGHT;
			warppadObj->lightDirRelic.x = AH_WARP_LIGHT_RELIC[0].x;
			warppadObj->lightDirRelic.y = AH_WARP_LIGHT_RELIC[0].y;
			warppadObj->lightDirRelic.z = AH_WARP_LIGHT_RELIC[0].z;
			warppadObj->slots.inst[WPIS_OPEN_PRIZE1]->matrix = inst->matrix;
			warppadObj->slots.inst[WPIS_OPEN_PRIZE1]->matrix.t[0] = inst->matrix.t[0];
			warppadObj->slots.inst[WPIS_OPEN_PRIZE1]->matrix.t[1] = inst->matrix.t[1] + AH_WP_FLOATING_OBJECT_HEIGHT;
			warppadObj->slots.inst[WPIS_OPEN_PRIZE1]->matrix.t[2] = inst->matrix.t[2];
			warppadObj->slots.inst[WPIS_OPEN_PRIZE1]->scale.x = AH_WP_RELIC_PRIZE_SCALE;
			warppadObj->slots.inst[WPIS_OPEN_PRIZE1]->scale.y = AH_WP_RELIC_PRIZE_SCALE;
			warppadObj->slots.inst[WPIS_OPEN_PRIZE1]->scale.z = AH_WP_RELIC_PRIZE_SCALE;
		}
	}
	else if (AH_WarpPad_IsBattleTrack(warppadObj->levelID))
	{
		s16 battleOffsets[7];
		memcpy(battleOffsets, AH_WARP_BATTLE_OFFSETS, sizeof(battleOffsets));
		t->modelIndex = AH_WP_VISUAL_COMPLETE;
		if (!CHECK_ADV_BIT(GAME_ADV_PROGRESS.rewards, battleOffsets[warppadObj->levelID - AH_WP_ID_FIRST_BATTLE_TRACK] + ADV_REWARD_FIRST_PURPLE_TOKEN))
		{
			t->modelIndex = AH_WP_VISUAL_COLOR_CYCLE_OPEN;
			warppadObj->slots.inst[WPIS_OPEN_PRIZE1] = INSTANCE_Birth3D(GAME_TRACKER->modelPtr[STATIC_TOKEN], "prize2", t);
			warppadObj->slots.inst[WPIS_OPEN_PRIZE1]->colorRGBA =
			    INST_PackColorRGB(AH_WARP_CUPS[4].color[0], AH_WARP_CUPS[4].color[1], AH_WARP_CUPS[4].color[2]);
			warppadObj->slots.inst[WPIS_OPEN_PRIZE1]->flags |= USE_SPECULAR_LIGHT;
			warppadObj->lightDirToken.x = AH_WARP_LIGHT_TOKEN[4].x;
			warppadObj->lightDirToken.y = AH_WARP_LIGHT_TOKEN[4].y;
			warppadObj->lightDirToken.z = AH_WARP_LIGHT_TOKEN[4].z;
			warppadObj->slots.inst[WPIS_OPEN_PRIZE1]->matrix = inst->matrix;
			warppadObj->slots.inst[WPIS_OPEN_PRIZE1]->matrix.t[0] = inst->matrix.t[0];
			warppadObj->slots.inst[WPIS_OPEN_PRIZE1]->matrix.t[1] = inst->matrix.t[1] + AH_WP_FLOATING_OBJECT_HEIGHT;
			warppadObj->slots.inst[WPIS_OPEN_PRIZE1]->matrix.t[2] = inst->matrix.t[2];
			warppadObj->slots.inst[WPIS_OPEN_PRIZE1]->scale.x = AH_WP_STANDARD_ITEM_SCALE;
			warppadObj->slots.inst[WPIS_OPEN_PRIZE1]->scale.y = AH_WP_STANDARD_ITEM_SCALE;
			warppadObj->slots.inst[WPIS_OPEN_PRIZE1]->scale.z = AH_WP_STANDARD_ITEM_SCALE;
		}
	}
	else if ((u16)(warppadObj->levelID - AH_WP_ID_FIRST_GEM_CUP) < AH_WP_GEM_CUP_COUNT)
	{
		s16 gemLevel;

		t->modelIndex = AH_WP_VISUAL_COMPLETE;
		gemLevel = warppadObj->levelID;
		if (!CHECK_ADV_BIT(GAME_ADV_PROGRESS.rewards, gemLevel + ADV_REWARD_FIRST_GEM - AH_WP_ID_FIRST_GEM_CUP))
		{
			gemLevel -= AH_WP_ID_FIRST_GEM_CUP;
			t->modelIndex = AH_WP_VISUAL_COLOR_CYCLE_OPEN;

			warppadObj->slots.inst[WPIS_OPEN_PRIZE1] = INSTANCE_Birth3D(GAME_TRACKER->modelPtr[STATIC_GEM], "prize2", t);
			warppadObj->slots.inst[WPIS_OPEN_PRIZE1]->colorRGBA =
			    INST_PackColorRGB(AH_WARP_CUPS[gemLevel].color[0], AH_WARP_CUPS[gemLevel].color[1], AH_WARP_CUPS[gemLevel].color[2]);
			warppadObj->slots.inst[WPIS_OPEN_PRIZE1]->flags |= USE_SPECULAR_LIGHT;
			warppadObj->lightDirGem.x = AH_WARP_LIGHT_GEM[gemLevel].x;
			warppadObj->lightDirGem.y = AH_WARP_LIGHT_GEM[gemLevel].y;
			warppadObj->lightDirGem.z = AH_WARP_LIGHT_GEM[gemLevel].z;
			warppadObj->slots.inst[WPIS_OPEN_PRIZE1]->matrix = inst->matrix;
			warppadObj->slots.inst[WPIS_OPEN_PRIZE1]->matrix.t[0] = inst->matrix.t[0];
			warppadObj->slots.inst[WPIS_OPEN_PRIZE1]->matrix.t[1] = inst->matrix.t[1] + AH_WP_FLOATING_OBJECT_HEIGHT;
			warppadObj->slots.inst[WPIS_OPEN_PRIZE1]->matrix.t[2] = inst->matrix.t[2];
			warppadObj->slots.inst[WPIS_OPEN_PRIZE1]->scale.x = AH_WP_STANDARD_ITEM_SCALE;
			warppadObj->slots.inst[WPIS_OPEN_PRIZE1]->scale.y = AH_WP_STANDARD_ITEM_SCALE;
			warppadObj->slots.inst[WPIS_OPEN_PRIZE1]->scale.z = AH_WP_STANDARD_ITEM_SCALE;
		}
	}
}
