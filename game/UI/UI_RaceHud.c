#include <common.h>

enum
{
	UI_BATTLE_HEAD_ARROW_Y_OFFSET_1P2P = 3,
	UI_BATTLE_HEAD_ARROW_Y_OFFSET_3P4P = 5,
	UI_BATTLE_HEAD_ARROW_SPLIT_PLAYER_COUNT = 3,
	UI_BATTLE_HEAD_ARROW_MIN_DIST_SQ = 0x90000,
	UI_BATTLE_HEAD_ARROW_GTE_CLIP_FLAG = 0x40000,
	UI_BATTLE_HEAD_ARROW_POLY_G3_CODE = 0x32,
	UI_BATTLE_HEAD_ARROW_DISTANCE_DIVISOR = 6,
	UI_BATTLE_HEAD_ARROW_DISTANCE_SHIFT = 0xd,
	UI_BATTLE_HEAD_ARROW_DISTANCE_BASE = 0x1000,
	UI_BATTLE_HEAD_ARROW_HALF_WIDTH_SCALE = 3,
	UI_BATTLE_HEAD_ARROW_HALF_WIDTH_SHIFT = 10,
	UI_BATTLE_HEAD_ARROW_HEIGHT_SCALE = 7,
	UI_BATTLE_HEAD_ARROW_HEIGHT_SHIFT = 12,
	UI_BATTLE_HEAD_ARROW_TIP_Y_OFFSET = 12,
	UI_BATTLE_HEAD_ARROW_COLOR_MASK = 0xffffff,
	UI_BATTLE_HEAD_ARROW_SEMITRANS_MASK = 0x30000000,
	UI_BATTLE_HEAD_ARROW_OT_TAG = 0x08000000,

	UI_TRACKER_TIMER_HIT_TARGET_FRAMES = 12,
	UI_TRACKER_TIMER_NEW_TARGET_FRAMES = 8,
	UI_TRACKER_DISTANCE_DISPLAY_DIVISOR = 0x32,
	UI_TRACKER_BEEP_RATE_NEAR = 5,
	UI_TRACKER_BEEP_RATE_MID = 10,
	UI_TRACKER_BEEP_RATE_FAR = 30,
	UI_TRACKER_BEEP_DIST_NEAR_MAX = 100,
	UI_TRACKER_BEEP_DIST_MID_MAX = 200,
	UI_TRACKER_BEEP_SFX = 0x56,
	UI_TRACKER_BG_RED = 0xff,
	UI_TRACKER_BG_WHITE = 0xffffff,
	UI_TRACKER_WARPBALL_DISTANCE_SCALE = 8,
	UI_TRACKER_TYPE_MISSILE = 0,
	UI_TRACKER_TYPE_WARPBALL = 1,
	UI_TRACKER_WARPBALL_DRAW_DIST_MAX = 16000,
	UI_TRACKER_SIDE_COUNT = 2,
	UI_TRACKER_LEFT_SIDE = 0,
	UI_TRACKER_LEFT_ORIENTATION = -1,
	UI_TRACKER_RIGHT_ORIENTATION = 1,
	UI_TRACKER_SIDE_HEIGHT_SCALE = 7,
	UI_TRACKER_SIDE_HEIGHT_SHIFT = 12,
	UI_TRACKER_SIDE_TIP_OFFSET = 12,
	UI_TRACKER_BG_X_SHIFT = 7,
	UI_TRACKER_BG_Y_SCALE = 0xf,
	UI_TRACKER_BG_Y_SHIFT = 0xb,
	UI_TRACKER_LOCK_ICON = 0x2d,
	UI_TRACKER_BG_TRANSPARENCY = 1,
	UI_TRACKER_POLY_G3_OT_TAG = 0x06000000,
	UI_TRACKER_BORDER_CENTER_COLOR = 0x30ffffff,
	UI_TRACKER_WARPBALL_BORDER_COLOR0 = 0x309c6900,
	UI_TRACKER_WARPBALL_BORDER_COLOR2 = 0x30ffff00,
	UI_TRACKER_MISSILE_BORDER_COLOR0 = 0x3000699c,
	UI_TRACKER_MISSILE_BORDER_COLOR2 = 0x3000ffff,
	UI_TRACKER_WARPBALL_FILL_COLOR0 = 0x30322b01,
	UI_TRACKER_WARPBALL_FILL_COLOR1 = 0x305b5b00,
	UI_TRACKER_WARPBALL_FILL_COLOR2 = 0x30ffbb00,
	UI_TRACKER_MISSILE_FILL_COLOR0 = 0x30012b32,
	UI_TRACKER_MISSILE_FILL_COLOR1 = 0x30005b5b,
	UI_TRACKER_MISSILE_FILL_COLOR2 = 0x3000bbff,

	UI_POS_SUFFIX_BIG_NUM_Z_BASE = 0x100,
	UI_LAP_COUNT_SPLIT_PLAYER_COUNT = 3,
	UI_LAP_COUNT_TEXT_Y_OFFSET = 8,
	UI_LAP_COUNT_MESSAGE_BUFFER_SIZE = 24,
	UI_BATTLE_SCORE_STRING_BUFFER_SIZE = 32,
	UI_BATTLE_SCORE_LIFE_ICON = 0x84,
	UI_BATTLE_SCORE_POINTS_ICON = 0x85,
	UI_BATTLE_SCORE_TEXT_X_OFFSET = 37,
	UI_BATTLE_SCORE_TEXT_Y_OFFSET = 4,
	UI_BATTLE_SCORE_ICON_SCALE = 0x1000,
};

static const u32 UI_BATTLE_HEAD_ARROW_DRAW_MODE = 0xe1000a20u;

CTR_STATIC_ASSERT(UI_BATTLE_HEAD_ARROW_MIN_DIST_SQ == 0x90000);
CTR_STATIC_ASSERT(UI_BATTLE_HEAD_ARROW_GTE_CLIP_FLAG == 0x40000);
CTR_STATIC_ASSERT(UI_BATTLE_HEAD_ARROW_POLY_G3_CODE == 0x32);
CTR_STATIC_ASSERT(UI_BATTLE_HEAD_ARROW_SEMITRANS_MASK == 0x30000000);
CTR_STATIC_ASSERT(UI_BATTLE_HEAD_ARROW_OT_TAG == 0x08000000);
CTR_STATIC_ASSERT(UI_TRACKER_POLY_G3_OT_TAG == 0x06000000);
CTR_STATIC_ASSERT(UI_TRACKER_LOCK_ICON == 0x2d);
CTR_STATIC_ASSERT(UI_TRACKER_BEEP_SFX == 0x56);
CTR_STATIC_ASSERT(UI_BATTLE_SCORE_LIFE_ICON == 0x84);
CTR_STATIC_ASSERT(UI_BATTLE_SCORE_POINTS_ICON == 0x85);

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004f9d8-0x8004fd34.
// Draw arrows over the heads of players
void UI_BattleDrawHeadArrows(struct Driver *player)
{
	typedef struct
	{
		u32 tag;
		u32 tpage;
		POLY_G3 g3;
	} G3_SEMITRANS;

	CTR_STATIC_ASSERT(sizeof(G3_SEMITRANS) == 0x24);

	u16 currTeam;
	s16 arrowBaseYOffset;
	s16 halfArrowWidth;
	s16 halfArrowHeight;
	s16 arrowCenterX;
	s16 arrowTipY;
	s16 distanceScale;
	SVec2 screenPos;
	u32 gteFlags;
	u32 *gradient;
	MATRIX *viewProj;
	G3_SEMITRANS *arrow;
	SVECTOR pos;

	struct GameTracker *gGT = sdata->gGT;

	u8 playerID = player->driverID;

	// pushBuffer ViewProj
	viewProj = &gGT->pushBuffer[playerID].matrix_ViewProj;
	gte_SetRotMatrix(viewProj);
	gte_SetTransMatrix(viewProj);

	u8 numPlyr = gGT->numPlyrCurrGame;

	for (u8 i = 0; i < numPlyr; i++)
	{
		// something related to player structure address
		struct Driver *currDriver = gGT->drivers[i];

		if (
		    // skip yourself, skip invisible, skip finished players
		    (i == playerID) || (currDriver->invisibleTimer != 0) || ((currDriver->actionsFlagSet & ACTION_RACE_FINISHED) != 0))
		{
			continue;
		}

		// If 3 or 4 Players
		arrowBaseYOffset = UI_BATTLE_HEAD_ARROW_Y_OFFSET_3P4P;

		// If numPlyrCurrGame is less than 3
		if (numPlyr < UI_BATTLE_HEAD_ARROW_SPLIT_PLAYER_COUNT)
		{
			arrowBaseYOffset = UI_BATTLE_HEAD_ARROW_Y_OFFSET_1P2P;
		}

		struct Instance *currInst = currDriver->instSelf;
		struct Instance *playerInst = player->instSelf;

		// Get X distance and Z distance between two players
		int xDistance = playerInst->matrix.t[0] - currInst->matrix.t[0];
		int zDistance = playerInst->matrix.t[2] - currInst->matrix.t[2];
		int playerDistance = ((xDistance * xDistance) + (zDistance * zDistance));

		// sqrt(0x90000) is 768

		// If currentDriver is less than 768 units away from this player,
		// don't draw that driver's arrow
		if (UI_BATTLE_HEAD_ARROW_MIN_DIST_SQ >= playerDistance)
		{
			continue;
		}

		// load input vector
		pos.vx = currInst->matrix.t[0];
		pos.vy = currInst->matrix.t[1];
		pos.vz = currInst->matrix.t[2];

		CTR_GteLoadSV0(&pos);

		// perspective projection
		gte_rtps();

		// get output
		CTR_GteStoreSXY(screenPos.v);
		gte_stflg(&gteFlags);

		if ((gteFlags & UI_BATTLE_HEAD_ARROW_GTE_CLIP_FLAG) != 0)
		{
			continue;
		}

		struct PrimMem *primMem = &gGT->backBuffer->primMem;

		arrow = primMem->cursor;
		if ((int)arrow > (int)primMem->guardEnd)
		{
			return;
		}

		primMem->cursor = arrow + 1;

		arrow->tpage = UI_BATTLE_HEAD_ARROW_DRAW_MODE;
		arrow->g3.tag = 0;

		arrow->g3.code = UI_BATTLE_HEAD_ARROW_POLY_G3_CODE;

		arrowCenterX = screenPos.x;
		arrowTipY = screenPos.y + arrowBaseYOffset;
		distanceScale = (UI_BATTLE_HEAD_ARROW_DISTANCE_BASE -
		                 (((playerDistance / UI_BATTLE_HEAD_ARROW_DISTANCE_DIVISOR + (playerDistance >> 0x1f)) >> UI_BATTLE_HEAD_ARROW_DISTANCE_SHIFT) -
		                  (playerDistance >> 0x1f)));
		halfArrowWidth = (s16)(distanceScale * UI_BATTLE_HEAD_ARROW_HALF_WIDTH_SCALE >> UI_BATTLE_HEAD_ARROW_HALF_WIDTH_SHIFT);
		halfArrowHeight = (s16)(distanceScale * UI_BATTLE_HEAD_ARROW_HEIGHT_SCALE >> UI_BATTLE_HEAD_ARROW_HEIGHT_SHIFT) + UI_BATTLE_HEAD_ARROW_TIP_Y_OFFSET;

		arrow->g3.x0 = arrowCenterX - halfArrowWidth;
		arrow->g3.y0 = arrowTipY - halfArrowHeight;
		arrow->g3.x1 = arrowCenterX;
		arrow->g3.y1 = arrowTipY - UI_BATTLE_HEAD_ARROW_TIP_Y_OFFSET;
		arrow->g3.x2 = arrowCenterX + halfArrowWidth;
		arrow->g3.y2 = arrowTipY - halfArrowHeight;

		// Battle Team of this driver
		currTeam = currDriver->BattleHUD.teamID;

		gradient = data.ptrColor[PLAYER_BLUE + currTeam];
		CtrGpu_WriteColorCode(&arrow->g3.r0, (gradient[0] & UI_BATTLE_HEAD_ARROW_COLOR_MASK) | UI_BATTLE_HEAD_ARROW_SEMITRANS_MASK);
		CtrGpu_WriteColorCode(&arrow->g3.r1, gradient[1] | UI_BATTLE_HEAD_ARROW_SEMITRANS_MASK);
		CtrGpu_WriteColorCode(&arrow->g3.r2, gradient[2] | UI_BATTLE_HEAD_ARROW_SEMITRANS_MASK);

		uint32_t *ot = gGT->pushBuffer[playerID].ptrOT;

		arrow->tag = CtrGpu_PackOTTag(*ot, UI_BATTLE_HEAD_ARROW_OT_TAG);
		*ot = CtrGpu_PrimToOTLink24(arrow);
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004fd34-0x8005045c.
void UI_TrackerSelf(struct Driver *d)
{
	s16 y;
	s16 x;
	s16 timer;
	s16 orientedX;
	s16 sideHalfHeight;
	s16 sideOuterX;
	s16 *trackerAnim;
	u16 driverID;
	SVec2 screenPos;
	u32 color0, color1, color2;
	int warpballDist;
	int missileDist;
	int beepRate;
	uint32_t *ot;
	POLY_G3 *poly;
	SVECTOR pos;
	struct PrimMem *primMem;
	s16 orientation;
	u32 bgColor;
	s16 resetTimer;
	s16 trackerDistance;
	int screenPosX;
	s16 screenPosY;
	struct GameTracker *gGT = sdata->gGT;

	// get index of driver in driver array
	driverID = d->driverID;

	timer = data.trackerTimer[driverID];

	if (
	    // timer loop ended
	    (timer == 0) &&

	    // no more missile chasing player
	    (d->thTrackingMe == 0))
	{
		// clear type of object tracking the player
		data.trackerType[driverID] = UI_TRACKER_TYPE_MISSILE;
		return;
	}

	warpballDist = 0;

	// If no missile or warpball is chasing this driver
	if (d->thTrackingMe == 0)
	{
		trackerAnim = &data.trackerAnim2[0];
	}
	// if a missile or warpball is chasing this driver
	else
	{
		trackerAnim = &data.trackerAnim1[0];
	}

	x = trackerAnim[timer * 2 + 0];
	y = trackerAnim[timer * 2 + 1];

	struct Thread *trackerTh = RB_GetThread_ClosestTracker(d);

	if (
	    // timer loop ended
	    (timer == 0) &&

	    // no missiles chasing player
	    (trackerTh == NULL))
	{
		// warpball is repathing after target was mask-grabbed
		if ((((struct TrackerWeapon *)d->thTrackingMe->object)->flags & TRACKER_FLAG_WARPBALL_MASK_REPATH) != 0)
		{
			goto UpdateTrackerState;
		}

		// reset timer loop
		resetTimer = UI_TRACKER_TIMER_HIT_TARGET_FRAMES;
	}

	// missile chasing player,
	// or dead missile + timer loop active
	else
	{
		if ((
		        // this driver is already marked as tracker-targeted
		        ((d->actionsFlagSet & ACTION_TRACKER_TARGETED) != 0) &&

		        // tracker chasing driver
		        (trackerTh == d->thTrackingMe)) ||

		    // timer loop active
		    (timer != 0))
		{
			goto UpdateTrackerState;
		}

		// mark this driver as tracker-targeted
		d->actionsFlagSet |= ACTION_TRACKER_TARGETED;

		// reset timer loop
		resetTimer = UI_TRACKER_TIMER_NEW_TARGET_FRAMES;
	}

	// reset timer loop
	data.trackerTimer[driverID] = resetTimer;

UpdateTrackerState:

	// set pointer of the missile or warpball chasing the player
	d->thTrackingMe = trackerTh;

	if (data.trackerTimer[driverID] != 0)
	{
		data.trackerTimer[driverID]--;
	}

	MATRIX *viewProj = &gGT->pushBuffer[driverID].matrix_ViewProj;
	gte_SetRotMatrix(viewProj);
	gte_SetTransMatrix(viewProj);

	pos.vx = (s16)d->instSelf->matrix.t[0];
	pos.vy = (s16)d->instSelf->matrix.t[1];
	pos.vz = (s16)d->instSelf->matrix.t[2];

	CTR_GteLoadSV0(&pos);
	gte_rtps();
	CTR_GteStoreSXY(screenPos.v);

	// red?
	bgColor = UI_TRACKER_BG_RED;

	// if no missile or warpball is chasing this player
	if (d->thTrackingMe == NULL)
	{
		trackerDistance = data.trackerDist[driverID];
	}

	// if a missile or warpball is chasing this player
	else
	{
		struct TrackerWeapon *tw = d->thTrackingMe->object;

		// NOTE(aalhendi): Retail always computes tracker distance here.
		missileDist = VehCalc_FastSqrt(tw->distanceToTarget, 0);

		missileDist = missileDist / UI_TRACKER_DISTANCE_DISPLAY_DIVISOR;
		trackerDistance = (s16)missileDist;
		data.trackerDist[driverID] = trackerDistance;

		beepRate = UI_TRACKER_BEEP_RATE_NEAR;
		if (missileDist > UI_TRACKER_BEEP_DIST_NEAR_MAX)
		{
			beepRate = UI_TRACKER_BEEP_RATE_MID;
		}
		if (missileDist > UI_TRACKER_BEEP_DIST_MID_MAX)
		{
			beepRate = UI_TRACKER_BEEP_RATE_FAR;
		}

		// red
		bgColor = UI_TRACKER_BG_RED;

		if ((gGT->timer % beepRate) == 0)
		{
			if ((gGT->gameMode1 & PAUSE_ALL) == 0)
			{
				// "homing in" sound
				// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80050014-0x8005001c for tracker beep SFX.
				OtherFX_Play(UI_TRACKER_BEEP_SFX, 1);
			}

			// white
			bgColor = UI_TRACKER_BG_WHITE;
		}

		if (d->thTrackingMe->inst->model->id == DYNAMIC_WARPBALL)
		{
			struct CheckpointNode *firstNode = &gGT->level1->ptr_restart_points[0];

			warpballDist = ((tw->ptrNodeCurr->distToFinish - firstNode[d->checkpoint.currentIndex].distToFinish) * UI_TRACKER_WARPBALL_DISTANCE_SCALE);

			// if warpball is further in the lap than the driver,
			// then add a full lap of distance until warpball hits driver
			if (warpballDist < 0)
			{
				// add length of track
				warpballDist += (firstNode[0].distToFinish * UI_TRACKER_WARPBALL_DISTANCE_SCALE);
			}

			// type of object following driver is a warpball
			data.trackerType[driverID] = UI_TRACKER_TYPE_WARPBALL;
		}

		// if this is not a warpball
		else
		{
			// type of object is missile, or nothing?
			data.trackerType[driverID] = UI_TRACKER_TYPE_MISSILE;
		}
	}

	// driver screenspace x and y
	screenPosX = screenPos.x;
	screenPosY = screenPos.y;

	// check distance
	if (warpballDist < UI_TRACKER_WARPBALL_DRAW_DIST_MAX)
	{
		primMem = &gGT->backBuffer->primMem;

		sideOuterX = trackerDistance + (x >> 8);
		sideHalfHeight = (s16)((y * UI_TRACKER_SIDE_HEIGHT_SCALE) >> UI_TRACKER_SIDE_HEIGHT_SHIFT);

		for (int side = 0; side < UI_TRACKER_SIDE_COUNT; side++)
		{
			// if left side or right side
			orientation = UI_TRACKER_RIGHT_ORIENTATION;
			if (side == UI_TRACKER_LEFT_SIDE)
			{
				orientation = UI_TRACKER_LEFT_ORIENTATION;
			}

			// if tracking object is warpball
			if (data.trackerType[driverID] == UI_TRACKER_TYPE_WARPBALL)
			{
				// tracker border colors (blue)
				color0 = UI_TRACKER_WARPBALL_BORDER_COLOR0;

				color2 = UI_TRACKER_WARPBALL_BORDER_COLOR2;
			}

			// if tracking object is missile
			else
			{
				// tracker border colors (orange)
				color0 = UI_TRACKER_MISSILE_BORDER_COLOR0;

				color2 = UI_TRACKER_MISSILE_BORDER_COLOR2;
			}

			poly = primMem->cursor;
			if (poly > (POLY_G3 *)primMem->guardEnd)
			{
				return;
			}
			primMem->cursor = poly + 1;

			CtrGpu_WriteColorCode(&poly->r0, color0);
			CtrGpu_WriteColorCode(&poly->r1, UI_TRACKER_BORDER_CENTER_COLOR);
			CtrGpu_WriteColorCode(&poly->r2, color2);

			orientedX = orientation * sideOuterX;
			poly->x0 = screenPosX + orientedX;
			poly->x2 = screenPosX + orientedX;

			orientedX = orientation * trackerDistance;
			poly->x1 = screenPosX + orientedX;

			poly->y0 = screenPosY - (sideHalfHeight + UI_TRACKER_SIDE_TIP_OFFSET);
			poly->y1 = screenPosY - UI_TRACKER_SIDE_TIP_OFFSET;
			poly->y2 = screenPosY - UI_TRACKER_SIDE_TIP_OFFSET;

			ot = gGT->pushBuffer[driverID].ptrOT;

			poly->tag = CtrGpu_PackOTTag(*ot, UI_TRACKER_POLY_G3_OT_TAG);
			*ot = CtrGpu_PrimToOTLink24(poly);

			// next Prim
			POLY_G3 *borderPoly = poly;
			poly = primMem->cursor;
			if (poly > (POLY_G3 *)primMem->guardEnd)
			{
				return;
			}
			primMem->cursor = poly + 1;

			// if tracking object is warpball
			if (data.trackerType[driverID] == UI_TRACKER_TYPE_WARPBALL)
			{
				color1 = UI_TRACKER_WARPBALL_FILL_COLOR1;

				color0 = UI_TRACKER_WARPBALL_FILL_COLOR0;

				color2 = UI_TRACKER_WARPBALL_FILL_COLOR2;
			}

			// if tracking object is missile
			else
			{
				color1 = UI_TRACKER_MISSILE_FILL_COLOR1;

				color0 = UI_TRACKER_MISSILE_FILL_COLOR0;

				color2 = UI_TRACKER_MISSILE_FILL_COLOR2;
			}

			CtrGpu_WriteColorCode(&poly->r0, color0);
			CtrGpu_WriteColorCode(&poly->r1, color1);
			CtrGpu_WriteColorCode(&poly->r2, color2);

			poly->x0 = borderPoly->x0;
			poly->y0 = screenPosY - UI_TRACKER_SIDE_TIP_OFFSET;
			CtrGpu_WritePackedXY(&poly->x1, CTR_ReadU32LE(&borderPoly->x1));
			poly->x2 = borderPoly->x2;

			poly->y2 = screenPosY + sideHalfHeight - UI_TRACKER_SIDE_TIP_OFFSET;

			poly->tag = CtrGpu_PackOTTag(*ot, UI_TRACKER_POLY_G3_OT_TAG);
			*ot = CtrGpu_PrimToOTLink24(poly);
		}
	}

	UI_TrackerBG(

	    // missile lock-on icon
	    gGT->ptrIcons[UI_TRACKER_LOCK_ICON],

	    screenPosX - (x >> UI_TRACKER_BG_X_SHIFT), screenPosY - ((y * UI_TRACKER_BG_Y_SCALE) >> UI_TRACKER_BG_Y_SHIFT),

	    &gGT->backBuffer->primMem, gGT->pushBuffer[driverID].ptrOT, UI_TRACKER_BG_TRANSPARENCY, x, y, bgColor);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8005045c-0x80050528.
void UI_DrawPosSuffix(s16 posX, s16 posY, struct Driver *d, s16 flags)
{
	int currRank;
	struct GameTracker *gGT = sdata->gGT;

	// If you're not in Battle Mode
	if ((gGT->gameMode1 & BATTLE_MODE) == 0)
	{
		// Get the rank you're in (1st, 2nd, 3rd, etc)
		currRank = d->driverRank;
	}
	else
	{
		// get the rank that the battle team is in
		currRank = gGT->battleSetup.finishedRankOfEachTeam[d->BattleHUD.teamID];
	}

	// Draw the suffix of your current position
	DecalFont_DrawLine(sdata->lngStrings[data.stringIndexSuffix[currRank]], posX, posY, FONT_BIG, flags);

	// setting posZ changes which number draws
	if (d->instBigNum != 0)
	{
		d->instBigNum->matrix.t[2] = (d->driverRank + UI_POS_SUFFIX_BIG_NUM_Z_BASE);
	}

	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80050528-0x80050654
void UI_DrawLapCount(s16 posX, int posY, int unusedScale, struct Driver *d)
{
	(void)unusedScale;
	s16 type;
	s16 currLap;
	int numLaps;
	int flags;
	char message[UI_LAP_COUNT_MESSAGE_BUFFER_SIZE];
	char *str;

	struct GameTracker *gGT;
	int numPlyrCurrGame;

	gGT = sdata->gGT;
	numLaps = gGT->numLaps;
	numPlyrCurrGame = gGT->numPlyrCurrGame;

	currLap = d->lapIndex + 1;

	if (currLap > numLaps)
	{
		currLap = numLaps;
	}

	// 3P or 4P
	type = FONT_SMALL;
	flags = PERIWINKLE;

	// 1P or 2P
	if (numPlyrCurrGame < UI_LAP_COUNT_SPLIT_PLAYER_COUNT)
	{
		DecalFont_DrawLine(sdata->lngStrings[LNG_LAP], posX, posY, FONT_SMALL, (JUSTIFY_RIGHT | PERIWINKLE));

		sprintf(&message[0], &sdata->s_intDividing[0], currLap, numLaps);
		str = &message[0];
		type = FONT_BIG;
		flags = (JUSTIFY_RIGHT | PERIWINKLE);
	}
	else
	{
		str = &sdata->s_printDividing[0];
		str[0] = currLap + '0';
		str[2] = (u8)numLaps + '0';

		type = FONT_SMALL;
		flags = PERIWINKLE;
	}

	// draw string
	DecalFont_DrawLine(str, posX, (posY + UI_LAP_COUNT_TEXT_Y_OFFSET), type, flags);
}

// Draw how many points or lifes the player has in battle
// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80050654-0x800507e0.
void UI_DrawBattleScores(int posX, int posY, struct Driver *d)
{
	struct Icon *icon;
	int value;
	char string[UI_BATTLE_SCORE_STRING_BUFFER_SIZE];
	struct GameTracker *gGT = sdata->gGT;

	if ((gGT->gameMode1 & POINT_LIMIT) == 0)
	{
		if ((gGT->gameMode1 & LIFE_LIMIT) == 0)
		{
			return;
		}

		// == Life Limit

		value = d->BattleHUD.numLives;
		icon = gGT->ptrIcons[UI_BATTLE_SCORE_LIFE_ICON];
	}

	else
	{
		// == Point Limit ==

		value = gGT->battleSetup.pointsPerTeam[d->BattleHUD.teamID];
		icon = gGT->ptrIcons[UI_BATTLE_SCORE_POINTS_ICON];
	}

	// add value to string
	sprintf(string, (char *)&sdata->s_longInt, value);

	DecalFont_DrawLine(string, (s16)(posX + UI_BATTLE_SCORE_TEXT_X_OFFSET), (s16)(posY + UI_BATTLE_SCORE_TEXT_Y_OFFSET), FONT_SMALL,
	                   data.battleScoreColor[gGT->numPlyrCurrGame - 1][d->driverID]);

	DecalHUD_DrawPolyFT4(icon, posX, posY, &gGT->backBuffer->primMem, gGT->pushBuffer_UI.ptrOT, 1, UI_BATTLE_SCORE_ICON_SCALE);
}
