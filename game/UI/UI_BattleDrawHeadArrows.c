#include <common.h>
#include <ctr_gte_matrix.h>
#include <ctr_gte_transfer.h>

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
};

static const u32 UI_BATTLE_HEAD_ARROW_DRAW_MODE = 0xe1000a20u;

static inline void *UI_BattleHeadArrows_ReservePacket(u32 size)
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

// Draw arrows over the heads of the other visible, unfinished players.
void UI_BattleDrawHeadArrows(struct Driver *driver)
{
	typedef struct
	{
		u32 tag;
		u32 tpage;
		POLY_G3 g3;
	} G3_SEMITRANS;
	s16 teamColor;
	s16 yOffset;
	s32 halfWidth;
	s32 halfHeight;
	s16 distanceScale;
	struct
	{
		SVec4 position;
		s16 screen[2];
		u32 flags;
	} projection;
	MATRIX *matrix;
	G3_SEMITRANS *arrow;
	// NOTE(aalhendi): T8/T7 are internal allocation constraints, not a caller ABI.
	// T8 retains the input outside the SDK transfers and packet-color lanes.
	register struct Driver *player CTR_PSX_REGISTER("$24") = driver;
	u32 sourcePlayerID = player->driverID;
	s32 playerID;
	s32 i;
	struct GameTracker *gGT = GAME_TRACKER;
	u32 *const *colorTable;
	u32 colorMask;
	register u32 semitrans CTR_PSX_REGISTER("$15");
	s16 *screenXY;
	CTR_STATIC_ASSERT(sizeof(G3_SEMITRANS) == 0x24);
	matrix = &gGT->pushBuffer[sourcePlayerID].matrix_ViewProj;
	CTR_GteLoadRotMatrix(matrix);
	CTR_GteLoadTransMatrix(matrix);
	i = 0;
	if (gGT->numPlyrCurrGame == 0)
		return;

	// Publish the viewport identity after the SDK matrix transfers.
	playerID = sourcePlayerID;
	screenXY = projection.screen;
	// NOTE(aalhendi): The table binding retains one internal page observation;
	// native still selects the canonical runtime table, with no caller contract.
	GAME_LOAD_UI_COLOR_TABLE(colorTable);
	semitrans = UI_BATTLE_HEAD_ARROW_SEMITRANS_MASK;
	colorMask = UI_BATTLE_HEAD_ARROW_COLOR_MASK;
	do
	{
		struct Driver *other = gGT->drivers[i];
		struct Instance *otherInst;
		struct Instance *playerInst;
		s32 dx, dz, distance;
		u16 team;
		u16 worldX;
		u32 *ot;
		if ((i == playerID) || (other->invisibleTimer != 0) || ((other->actionsFlagSet & ACTION_RACE_FINISHED) != 0))
			goto NextDriver;
		yOffset = UI_BATTLE_HEAD_ARROW_Y_OFFSET_3P4P;
		if (gGT->numPlyrCurrGame < UI_BATTLE_HEAD_ARROW_SPLIT_PLAYER_COUNT)
			yOffset = UI_BATTLE_HEAD_ARROW_Y_OFFSET_1P2P;
		otherInst = other->instSelf;
		playerInst = player->instSelf;
		// NOTE(aalhendi): Distance arithmetic wraps like the MIPS low-word operations.
		dx = CTR_MipsSubLo(playerInst->matrix.t[0], otherInst->matrix.t[0]);
		dz = CTR_MipsSubLo(playerInst->matrix.t[2], otherInst->matrix.t[2]);
		distance = CTR_MipsAddLo(CTR_MipsMulLo(dx, dx), CTR_MipsMulLo(dz, dz));
		// Drivers within 768 units do not need a head arrow.
		if (UI_BATTLE_HEAD_ARROW_MIN_DIST_SQ >= distance)
			goto NextDriver;
		worldX = other->instSelf->matrix.t[0];
		team = other->BattleHUD.teamID;
		projection.position.x = worldX;
		projection.position.y = other->instSelf->matrix.t[1];
		projection.position.z = other->instSelf->matrix.t[2];
		teamColor = PLAYER_BLUE + team;
		CTR_GteLoadPositionV0(&projection.position);
		gte_rtps();
		CTR_GteStorePositionXY(screenXY);
		CTR_GteStoreFlagsWord(&projection.flags);
		if ((projection.flags & UI_BATTLE_HEAD_ARROW_GTE_CLIP_FLAG) != 0)
			goto NextDriver;
		arrow = UI_BattleHeadArrows_ReservePacket(sizeof(*arrow));
		if (arrow == NULL)
			return;
		screenXY[1] += yOffset;
		distanceScale = UI_BATTLE_HEAD_ARROW_DISTANCE_BASE - distance / (UI_BATTLE_HEAD_ARROW_DISTANCE_DIVISOR << UI_BATTLE_HEAD_ARROW_DISTANCE_SHIFT);
		halfWidth = distanceScale * UI_BATTLE_HEAD_ARROW_HALF_WIDTH_SCALE >> UI_BATTLE_HEAD_ARROW_HALF_WIDTH_SHIFT;
		halfHeight = (distanceScale * UI_BATTLE_HEAD_ARROW_HEIGHT_SCALE >> UI_BATTLE_HEAD_ARROW_HEIGHT_SHIFT) + UI_BATTLE_HEAD_ARROW_TIP_Y_OFFSET;
		arrow->g3.x2 = projection.screen[0] + halfWidth;
		arrow->g3.y2 = screenXY[1] - halfHeight;
		arrow->g3.x1 = projection.screen[0];
		arrow->g3.y1 = screenXY[1] - UI_BATTLE_HEAD_ARROW_TIP_Y_OFFSET;
		arrow->g3.x0 = projection.screen[0] - halfWidth;
		arrow->g3.y0 = screenXY[1] - halfHeight;
		setlen(arrow, 8);
		arrow->g3.code = UI_BATTLE_HEAD_ARROW_POLY_G3_CODE;
		arrow->tpage = UI_BATTLE_HEAD_ARROW_DRAW_MODE;
		arrow->g3.tag = 0;
		CtrGpu_WriteColorCode(&arrow->g3.r1, colorTable[teamColor][1] | semitrans);
		CtrGpu_WriteColorCode(&arrow->g3.r0, (colorTable[teamColor][0] & colorMask) | semitrans);
		CtrGpu_WriteColorCode(&arrow->g3.r2, colorTable[teamColor][2] | semitrans);
		ot = GAME_TRACKER->pushBuffer[playerID].ptrOT;
		// NOTE(aalhendi): Native translates host pointers and clears OT upper bytes
		// to preserve packet length. Retail OT words already contain 24-bit links.
#ifdef CTR_NATIVE
		arrow->tag = CtrGpu_PackOTTag(*ot, UI_BATTLE_HEAD_ARROW_OT_TAG);
		*ot = CtrGpu_PrimToOTLink24(arrow);
#else
		arrow->tag = *ot | UI_BATTLE_HEAD_ARROW_OT_TAG;
		*ot = (u32)arrow & colorMask;
#endif
	NextDriver:
		gGT = GAME_TRACKER;
		i++;
	} while (i < gGT->numPlyrCurrGame);
}
