#include <common.h>
#include <ctr_gte_matrix.h>
#include <ctr_gte_transfer.h>

enum
{
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
};

static inline void *UITracker_Reserve(u32 size)
{
	struct DB *db = GAME_TRACKER->backBuffer;
	void *packet = NULL;
	if ((u32)db->primMem.cursor <= (u32)db->primMem.guardEnd)
	{
		packet = db->primMem.cursor;
		db->primMem.cursor = (void *)((u8 *)packet + size);
	}
	return packet;
}

static inline s16 *UITracker_TimerSlot(u32 index)
{
	s16 *base = GAME_DATA_NAME(trackerTimer);
	return &base[index];
}

// NOTE(aalhendi): Animation frames can cross a bank into live rank fields.
// Root byte arithmetic at the complete Data object; a narrow array is not its owner.
static inline const u8 *UITracker_AnimationPair(const s16 *animation, s32 frame)
{
	return (const u8 *)&data + ((u32)animation + (u32)frame * (2 * sizeof(s16)) - (u32)&data);
}

// Draw the tracked weapon's distance chevrons and animated lock-on icon.
void UI_TrackerSelf(struct Driver *driver)
{
	// NOTE(aalhendi): These are internal allocation constraints, not a caller ABI.
	// Keep the persistent driver/ID and short-lived packet/publication lanes separate.
	register struct Driver *d CTR_PSX_REGISTER("$17") = driver;
	s16 y, x, orientedX;
	s32 sideHalfHeight, sideOuterX;
	s32 sideTopTip, sideBottomTip;
	s16 *typeSlot;
	s16 *screen;
	const u8 *animationPair;
	register u32 driverID CTR_PSX_REGISTER("$19");
	struct
	{
		SVec4 position;
		s16 screen[2];
	} projection;
	u32 color0, color2;
	u32 color1;
	s32 warpballDist, missileDist;
	u32 beepRate;
	u32 *borderOT;
	u32 *fillOT;
	register POLY_G3 *poly CTR_PSX_REGISTER("$5");
	register u32 borderLink CTR_PSX_REGISTER("$4");
	register u32 fillLink CTR_PSX_REGISTER("$4");
	s16 orientation;
	u32 bgColor;
	s16 resetTimer;
	s32 trackerDistance;
	struct Thread *trackerTh;
	s16 *timerSlot;
	MATRIX *viewProj;
	struct TrackerWeapon *trackerObject;
	s32 side;
	u32 packetViewport;
	u32 projectionID;
	u32 linkMask;
	trackerTh = d->thTrackingMe;
	driverID = d->driverID;
	// NOTE(aalhendi): Retain this initialized ID copy across GCC's copy forwarding.
	CTR_PSX_KEEP_VALUE(driverID);
	if ((trackerTh != 0) || (GAME_DATA_NAME(trackerTimer)[driverID] != 0))
	{
		warpballDist = 0;
		if (trackerTh != 0)
		{
			s16 *trackerAnim = GAME_DATA_NAME(trackerAnim1);
			animationPair = UITracker_AnimationPair(trackerAnim, GAME_DATA_NAME(trackerTimer)[driverID]);
			x = (s16)CTR_ReadU16AlignedLE(animationPair);
			y = (s16)CTR_ReadU16AlignedLE(animationPair + sizeof(s16));
		}
		else
		{
			s16 *trackerAnim = GAME_DATA_NAME(trackerAnim2);
			animationPair = UITracker_AnimationPair(trackerAnim, GAME_DATA_NAME(trackerTimer)[driverID]);
			x = (s16)CTR_ReadU16AlignedLE(animationPair);
			y = (s16)CTR_ReadU16AlignedLE(animationPair + sizeof(s16));
		}
		trackerTh = RB_GetThread_ClosestTracker(d);
		if ((trackerTh == NULL) && ((timerSlot = UITracker_TimerSlot(driverID)), *timerSlot == 0))
		{
			// A mask-grabbed target can leave its warpball repathing, not finished.
			trackerObject = d->thTrackingMe->object;
			if ((trackerObject->flags & TRACKER_FLAG_WARPBALL_MASK_REPATH) != 0)
				goto UpdateTrackerState;
			resetTimer = UI_TRACKER_TIMER_HIT_TARGET_FRAMES;
		}
		else
		{
			if ((((d->actionsFlagSet & ACTION_TRACKER_TARGETED) != 0) && (trackerTh == d->thTrackingMe)) ||
			    ((timerSlot = UITracker_TimerSlot(driverID)), *timerSlot != 0))
				goto UpdateTrackerState;
			d->actionsFlagSet |= ACTION_TRACKER_TARGETED;
			resetTimer = UI_TRACKER_TIMER_NEW_TARGET_FRAMES;
		}
		*timerSlot = resetTimer;
	UpdateTrackerState:
		d->thTrackingMe = trackerTh;
		if (GAME_DATA_NAME(trackerTimer)[driverID] != 0)
			GAME_DATA_NAME(trackerTimer)[driverID]--;
		projection.position.x = (s16)d->instSelf->matrix.t[0];
		projection.position.y = (s16)d->instSelf->matrix.t[1];
		projection.position.z = (s16)d->instSelf->matrix.t[2];
		// Publish the viewport snapshot separately from the persistent driver ID.
		projectionID = driverID;
		viewProj = &GAME_TRACKER->pushBuffer[projectionID].matrix_ViewProj;
		CTR_GteLoadRotMatrix(viewProj);
		CTR_GteLoadTransMatrix(viewProj);
		CTR_GteLoadPositionV0(&projection.position);
		gte_rtps();
		CTR_GteStorePositionXY(&projection.screen[0]);
		bgColor = UI_TRACKER_BG_RED;
		if (d->thTrackingMe != NULL)
		{
			trackerObject = d->thTrackingMe->object;
			missileDist = VehCalc_FastSqrt(trackerObject->distanceToTarget, 0) / UI_TRACKER_DISTANCE_DISPLAY_DIVISOR;
			trackerDistance = missileDist;
			GAME_DATA_NAME(trackerDist)[driverID] = trackerDistance;
			if (missileDist > UI_TRACKER_BEEP_DIST_MID_MAX)
				beepRate = UI_TRACKER_BEEP_RATE_FAR;
			else if (missileDist > UI_TRACKER_BEEP_DIST_NEAR_MAX)
				beepRate = UI_TRACKER_BEEP_RATE_MID;
			else
				beepRate = UI_TRACKER_BEEP_RATE_NEAR;
			bgColor = UI_TRACKER_BG_RED;
			// NOTE(aalhendi): Keep native's signed timer remainder; retail uses DIVU.
#ifdef CTR_NATIVE
			if ((GAME_TRACKER->timer % (s32)beepRate) == 0)
#else
			if ((GAME_TRACKER->timer % beepRate) == 0)
#endif
			{
				if ((GAME_TRACKER->gameMode1 & PAUSE_ALL) == 0)
					OtherFX_Play(UI_TRACKER_BEEP_SFX, 1);
				bgColor = UI_TRACKER_BG_WHITE;
			}
			if (d->thTrackingMe->inst->model->id == DYNAMIC_WARPBALL)
			{
				struct CheckpointNode *firstNode;
				s32 lapLength;
				trackerObject = d->thTrackingMe->object;
				firstNode = GAME_TRACKER->level1->ptr_restart_points;
				warpballDist =
				    (trackerObject->ptrNodeCurr->distToFinish - firstNode[d->checkpoint.currentIndex].distToFinish) * UI_TRACKER_WARPBALL_DISTANCE_SCALE;
				lapLength = firstNode[0].distToFinish * UI_TRACKER_WARPBALL_DISTANCE_SCALE;
				// Wrap a warpball behind the driver through the remaining lap distance.
				if (warpballDist < 0)
					warpballDist += lapLength;
				GAME_DATA_NAME(trackerType)[driverID] = UI_TRACKER_TYPE_WARPBALL;
			}
			else
				GAME_DATA_NAME(trackerType)[driverID] = UI_TRACKER_TYPE_MISSILE;
		}
		else
			trackerDistance = GAME_DATA_NAME(trackerDist)[driverID];
		if (warpballDist < UI_TRACKER_WARPBALL_DRAW_DIST_MAX)
		{
			s16 *typeBase = GAME_DATA_NAME(trackerType);
			side = 0;
			packetViewport = driverID;
			typeSlot = &typeBase[packetViewport];
			sideOuterX = trackerDistance + (x >> 8);
			screen = projection.screen;
			sideHalfHeight = (s16)((y * UI_TRACKER_SIDE_HEIGHT_SCALE) >> UI_TRACKER_SIDE_HEIGHT_SHIFT);
			sideTopTip = sideHalfHeight + UI_TRACKER_SIDE_TIP_OFFSET;
			sideBottomTip = sideHalfHeight - UI_TRACKER_SIDE_TIP_OFFSET;
			for (; side < UI_TRACKER_SIDE_COUNT;)
			{
				orientation = UI_TRACKER_RIGHT_ORIENTATION;
				if (side == UI_TRACKER_LEFT_SIDE)
					orientation = UI_TRACKER_LEFT_ORIENTATION;
				poly = UITracker_Reserve(sizeof(*poly));
				if (poly == NULL)
					return;
				if (*typeSlot == UI_TRACKER_TYPE_WARPBALL)
				{
					color1 = UI_TRACKER_BORDER_CENTER_COLOR & 0xffff0000u;
					color1 |= 0xffff;
					color0 = UI_TRACKER_WARPBALL_BORDER_COLOR0;
					color2 = UI_TRACKER_WARPBALL_BORDER_COLOR2;
					CtrGpu_WriteColorCode(&poly->r1, color1);
					CtrGpu_WriteColorCode(&poly->r0, color0);
					CtrGpu_WriteColorCode(&poly->r2, color2);
				}
				else
				{
					color1 = UI_TRACKER_BORDER_CENTER_COLOR & 0xffff0000u;
					color1 |= 0xffff;
					color0 = UI_TRACKER_MISSILE_BORDER_COLOR0;
					color2 = UI_TRACKER_MISSILE_BORDER_COLOR2;
					CtrGpu_WriteColorCode(&poly->r1, color1);
					CtrGpu_WriteColorCode(&poly->r0, color0);
					CtrGpu_WriteColorCode(&poly->r2, color2);
				}
				orientedX = orientation * sideOuterX;
				poly->x2 = projection.screen[0] + orientedX;
				orientedX = orientation * trackerDistance;
				poly->y2 = screen[1] - UI_TRACKER_SIDE_TIP_OFFSET;
				poly->x1 = projection.screen[0] + orientedX;
				poly->y1 = screen[1] - UI_TRACKER_SIDE_TIP_OFFSET;
				orientedX = orientation * sideOuterX;
				poly->x0 = projection.screen[0] + orientedX;
				poly->y0 = screen[1] - sideTopTip;
				// Border and fill retain independent link/OT lifetimes.
				borderOT = GAME_TRACKER->pushBuffer[packetViewport].ptrOT;
				borderLink = CtrGpu_PrimToOTLink24(poly);
				// NOTE(aalhendi): Native translates host pointers and clears OT upper bytes;
				// retail OT words already contain only 24-bit links.
#ifdef CTR_NATIVE
				poly->tag = CtrGpu_PackOTTag(*borderOT, UI_TRACKER_POLY_G3_OT_TAG);
#else
				poly->tag = *borderOT | UI_TRACKER_POLY_G3_OT_TAG;
#endif
				*borderOT = borderLink;
				poly = UITracker_Reserve(sizeof(*poly));
				if (poly == NULL)
					return;
				if (*typeSlot == UI_TRACKER_TYPE_WARPBALL)
				{
					color1 = UI_TRACKER_WARPBALL_FILL_COLOR1;
					color0 = UI_TRACKER_WARPBALL_FILL_COLOR0;
					color2 = UI_TRACKER_WARPBALL_FILL_COLOR2;
					CtrGpu_WriteColorCode(&poly->r1, color1);
					CtrGpu_WriteColorCode(&poly->r0, color0);
					CtrGpu_WriteColorCode(&poly->r2, color2);
				}
				else
				{
					color1 = UI_TRACKER_MISSILE_FILL_COLOR1;
					color0 = UI_TRACKER_MISSILE_FILL_COLOR0;
					color2 = UI_TRACKER_MISSILE_FILL_COLOR2;
					CtrGpu_WriteColorCode(&poly->r1, color1);
					CtrGpu_WriteColorCode(&poly->r0, color0);
					CtrGpu_WriteColorCode(&poly->r2, color2);
				}
				orientedX = orientation * sideOuterX;
				poly->x2 = projection.screen[0] + orientedX;
				poly->y2 = screen[1] + sideBottomTip;
				orientedX = orientation * trackerDistance;
				poly->x1 = projection.screen[0] + orientedX;
				poly->y1 = screen[1] - UI_TRACKER_SIDE_TIP_OFFSET;
				orientedX = orientation * sideOuterX;
				poly->x0 = projection.screen[0] + orientedX;
				poly->y0 = screen[1] - UI_TRACKER_SIDE_TIP_OFFSET;
				fillOT = GAME_TRACKER->pushBuffer[packetViewport].ptrOT;
				// NOTE(aalhendi): Separate mask preparation from publication so the side
				// increment retains retail's position among the packet writes.
				linkMask = 0xff0000;
				linkMask |= 0xffff;
				side++;
#ifdef CTR_NATIVE
				fillLink = CtrGpu_PrimToOTLink24(poly) & linkMask;
				poly->tag = CtrGpu_PackOTTag(*fillOT, UI_TRACKER_POLY_G3_OT_TAG);
#else
				fillLink = (u32)poly & linkMask;
				poly->tag = *fillOT | UI_TRACKER_POLY_G3_OT_TAG;
#endif
				*fillOT = fillLink;
			}
		}
		{
			s32 bgOffsetX;
			// NOTE(aalhendi): Preserve this one SRA after short promotion; GCC otherwise
			// folds it into shift23 separately from the background scale argument.
			CTR_PSX_SHIFT_RIGHT_ARITHMETIC(bgOffsetX, (s32)x, UI_TRACKER_BG_X_SHIFT);
			UI_TrackerBG(GAME_TRACKER->ptrIcons[UI_TRACKER_LOCK_ICON], projection.screen[0] - bgOffsetX,
			             projection.screen[1] - ((y * UI_TRACKER_BG_Y_SCALE) >> UI_TRACKER_BG_Y_SHIFT), &GAME_TRACKER->backBuffer->primMem,
			             GAME_TRACKER->pushBuffer[driverID].ptrOT, UI_TRACKER_BG_TRANSPARENCY, x, y, bgColor);
		}
	}
	else
		GAME_DATA_NAME(trackerType)[driverID] = UI_TRACKER_TYPE_MISSILE;
}
