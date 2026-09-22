#include <common.h>

#ifndef UI_MAP_COLORS
#define UI_MAP_COLORS       data.ptrColor
#define UI_MAP_ARROW_POS    data.playerIconAdvMap.pos
#define UI_MAP_ARROW_COLORS data.playerIconAdvMap.colors
#endif

enum UIMapConstants
{
	UI_MAP_NEUTRAL_COLOR = 0x808080,
	UI_MAP_COLOR_MODE_BLACK = 2,
	UI_MAP_COLOR_MODE_BLUE = 3,
	UI_MAP_BLUE_OUTLINE_COLOR = 0x402000,
	UI_MAP_TPAGE_BLEND_MASK = 0xff9f,
	UI_MAP_TPAGE_BLEND_SHIFT = 5,
	UI_MAP_SEMI_TRANS_CODE_BIT = 2,
	UI_MAP_MODE_0_DEGREES = 0,
	UI_MAP_MODE_90_DEGREES = 1,
	UI_MAP_MODE_180_DEGREES = 2,
	UI_MAP_ICON_Y_OFFSET = 0x10,
	UI_MAP_3P_OFFSET_X = 60,
	UI_MAP_3P_OFFSET_Y = 10,
	UI_MAP_PLAYER_ICON_AI = 0x31,
	UI_MAP_PLAYER_ICON_HUMAN = 0x32,
	UI_MAP_WARPBALL_ICON = 0x20,
	UI_MAP_WARPBALL_TARGET_ICON = 0x21,
	UI_MAP_ICON_GROUP = 5,
	UI_MAP_ARROW_ROT_FLIP = 0x800,
	UI_MAP_ARROW_ROT_FLAG = 0x1000,
	UI_MAP_ICON_SCALE = 0x1000,
	UI_MAP_ADV_ARROW_SCALE = 0x800,
};
void UI_Map_DrawMap(struct Icon *mapTop, struct Icon *mapBottom, s32 posX, s32 posY, struct PrimMem *primMem, u32 *otMem, u8 colorID)
{
	s32 mapBottomWidth;
	s32 mapBottomHeight;
	// NOTE(aalhendi): Keep the two packet-local X values and packed color in
	// their retail registers. Native leaves these ordinary C temporaries.
	register s32 leftX CTR_PSX_REGISTER("$4");
	register s32 bottomX CTR_PSX_REGISTER("$3");
	s32 topY;
	register u32 color CTR_PSX_REGISTER("$18");
	struct UIMapSpawnMetadata *mapMetadata;
	POLY_FT4 *p;
	struct GameTracker *gGT;

	mapMetadata = NULL;

	// draw minimap with neutral/none vertex color, minimap's regular color is white
	color = UI_MAP_NEUTRAL_COLOR;

	// draw map black
	// used for the minimap shadow in the track select screen
	if (colorID == UI_MAP_COLOR_MODE_BLACK)
	{
		colorID = 0;
		color = 0;
	}

	// draw minimap blue
	// used for the minimap outline in the track select screen
	else if (colorID == UI_MAP_COLOR_MODE_BLUE)
	{
		colorID = 0;
		color = UI_MAP_BLUE_OUTLINE_COLOR;
	}

	gGT = GAME_TRACKER;
	if (
#ifdef CTR_NATIVE
	    // NOTE(aalhendi): Native menu/loading states may lack level spawn data.
	    gGT->level1 != NULL && gGT->level1->ptrSpawnType1 != NULL &&
#endif
	    gGT->level1->ptrSpawnType1->count != 0)
	{
		void **pointers = ST1_GETPOINTERS(gGT->level1->ptrSpawnType1);
		mapMetadata = pointers[ST1_MAP];
	}

	// The supplied position is the map's bottom-right corner.
	mapBottomWidth = mapBottom->texLayout.u1 - mapBottom->texLayout.u0;
	mapBottomHeight = mapBottom->texLayout.v2 - mapBottom->texLayout.v0;

	// Menus show both texture halves; gameplay follows the level's map metadata.
	if (((mapMetadata != NULL) && (mapMetadata->topHalfMode == 0)) || ((gGT->gameMode1 & MAIN_MENU) != 0))
	{
		p = (POLY_FT4 *)primMem->cursor;
#ifdef CTR_NATIVE
		// NOTE(aalhendi): Retail assumes HUD packet space remains. Native must
		// respect the same guard used by the other primitive allocators.
		if ((u32)p > (u32)primMem->guardEnd)
		{
			return;
		}
#endif
		leftX = posX - (mapTop->texLayout.u1 - mapTop->texLayout.u0);
		topY = posY - (mapTop->texLayout.v2 - mapTop->texLayout.v0 + mapBottomHeight);
		// r0, g0, b0 (vertex color)
		CtrGpu_WriteColorCode(&p->r0, color);
		CtrGpu_WritePackedUVWord(&p->u0, CTR_ReadU32LE(&mapTop->texLayout.u0));
		CtrGpu_WritePackedUVWord(&p->u1, CTR_ReadU32LE(&mapTop->texLayout.u1));
		CtrGpu_WritePackedUVWord(&p->u2, CTR_ReadU32LE(&mapTop->texLayout.u2));
		CtrGpu_WritePackedUV(&p->u3, CTR_ReadU16LE(&mapTop->texLayout.u3));
		p->y2 = posY - mapBottomHeight;
		p->y3 = posY - mapBottomHeight;
		setPolyFT4(p);
		p->x0 = leftX;
		p->y0 = topY;
		p->x1 = posX;
		p->y1 = topY;
		p->x2 = leftX;
		p->x3 = posX;
		if (colorID != 0)
		{
			p->tpage = (p->tpage & UI_MAP_TPAGE_BLEND_MASK) | ((u8)colorID << UI_MAP_TPAGE_BLEND_SHIFT);
		}
		p->code |= UI_MAP_SEMI_TRANS_CODE_BIT;
		AddPrim(otMem, p);
		primMem->cursor = (u8 *)primMem->cursor + sizeof(*p);
	}

	p = (POLY_FT4 *)primMem->cursor;
#ifdef CTR_NATIVE
	if ((u32)p > (u32)primMem->guardEnd)
	{
		return;
	}
#endif
	// r0, g0, b0 (vertex color)
	CtrGpu_WriteColorCode(&p->r0, color);
	CtrGpu_WritePackedUVWord(&p->u0, CTR_ReadU32LE(&mapBottom->texLayout.u0));
	CtrGpu_WritePackedUVWord(&p->u1, CTR_ReadU32LE(&mapBottom->texLayout.u1));
	CtrGpu_WritePackedUVWord(&p->u2, CTR_ReadU32LE(&mapBottom->texLayout.u2));
	CtrGpu_WritePackedUV(&p->u3, CTR_ReadU16LE(&mapBottom->texLayout.u3));
	bottomX = posX - mapBottomWidth;
	p->y0 = posY - mapBottomHeight;
	p->y1 = posY - mapBottomHeight;
	setPolyFT4(p);
	p->x0 = bottomX;
	p->x1 = posX;
	p->x2 = bottomX;
	p->y2 = posY;
	p->x3 = posX;
	p->y3 = posY;
	if (colorID != 0)
	{
		p->tpage = (p->tpage & UI_MAP_TPAGE_BLEND_MASK) | ((u8)colorID << UI_MAP_TPAGE_BLEND_SHIFT);
	}
	p->code |= UI_MAP_SEMI_TRANS_CODE_BIT;
	AddPrim(otMem, p);
	primMem->cursor = (u8 *)primMem->cursor + sizeof(*p);
}

void UI_Map_GetIconPos(struct UIMap *map, s32 *posX, s32 *posY)
{
	s32 screenX, screenY;
	s32 originY = map->iconStartY - UI_MAP_ICON_Y_OFFSET;
	s32 worldRangeX = map->worldEndX - map->worldStartX;
	s32 worldRangeY = map->worldEndY - map->worldStartY;
	s32 mode = map->mode;

	if (mode == UI_MAP_MODE_0_DEGREES)
	{
		// 0 degrees
		screenX = map->iconStartX + (s32)((u32)*posX * map->iconSizeX) / worldRangeX;
		screenY = originY + (s32)((u32)*posY * map->iconSizeY * 2) / worldRangeY;
	}

	else if (mode == UI_MAP_MODE_90_DEGREES)
	{
		// 90 degrees
		screenY = originY + (s32)((u32)*posX * map->iconSizeY * 2) / worldRangeX;
		screenX = map->iconStartX - (s32)((u32)*posY * map->iconSizeX) / worldRangeY;
	}

	else if (mode == UI_MAP_MODE_180_DEGREES)
	{
		// 180 degrees
		screenX = map->iconStartX - (s32)((u32)*posX * map->iconSizeX) / worldRangeX;
		screenY = originY - (s32)((u32)*posY * map->iconSizeY * 2) / worldRangeY;
	}

	else
	{
		// 270 degrees
		screenY = originY - (s32)((u32)*posX * map->iconSizeY * 2) / worldRangeX;
		screenX = map->iconStartX + (s32)((u32)*posY * map->iconSizeX) / worldRangeY;
	}

	if (GAME_TRACKER->numPlyrCurrGame == 3)
	{
		screenX -= UI_MAP_3P_OFFSET_X;
		screenY += UI_MAP_3P_OFFSET_Y;
	}

	*posX = screenX;
	*posY = screenY;
	return;
}

// Draw dot for Player on 2D Adv Map
void UI_Map_DrawAdvPlayer(struct UIMap *map, const s32 worldPos[3], s32 unused1, s32 unused2, s16 rot, s16 scale)
{
	s32 posX;
	s32 posY;
	(void)unused1;
	(void)unused2;

	posX = worldPos[0];
	posY = worldPos[2];

	UI_Map_GetIconPos(map, &posX, &posY);

	AH_Map_HubArrow((s16)posX, (s16)posY, UI_MAP_ARROW_POS, (char *)&UI_MAP_ARROW_COLORS + ((GAME_TRACKER->timer & 2) ? sizeof(UI_MAP_ARROW_COLORS[0]) : 0),
	                (s16)scale, (s16)rot);

	return;
}

// Draw icon on map
void UI_Map_DrawRawIcon(struct UIMap *map, const s32 worldPos[3], s32 iconID, s32 colorID, s32 unused, s16 scale)
{
	s32 posX;
	s32 posY;
	struct Icon *icon;
	struct GameTracker *gGT;
	u32 **colors;

	(void)unused;

	posX = worldPos[0];
	posY = worldPos[2];

	UI_Map_GetIconPos(map, &posX, &posY);

	// NOTE(aalhendi): Narrow the retail indices before advancing the group's
	// appended pointer array; retain the byte offset until the icon load.
	iconID = (s16)iconID * sizeof(struct Icon *);
	colorID = (s16)colorID;
	gGT = GAME_TRACKER;
	colors = &UI_MAP_COLORS[colorID];
	icon = *(struct Icon **)((u8 *)gGT->iconGroup[UI_MAP_ICON_GROUP] + iconID + sizeof(struct IconGroup));
	DecalHUD_DrawPolyGT4(icon, posX, posY, &gGT->backBuffer->primMem, gGT->pushBuffer_UI.ptrOT, ColorCode_Load(&(*colors)[0]), ColorCode_Load(&(*colors)[1]),
	                     ColorCode_Load(&(*colors)[2]), ColorCode_Load(&(*colors)[3]), 0, (s32)scale);

	return;
}

void UI_Map_DrawDrivers(struct UIMap *map, struct Thread *bucket, s16 *driverIconCounter)
{
	s32 kartColor;
	struct Driver *d;
	struct Instance *inst;
	struct GameTracker *gGT;
	s32 drawColor;
	s32 iconID;
	const s32 *worldPos;
	struct UIMap *drawMap;

	for (; bucket != 0; *driverIconCounter = (u16)*driverIconCounter + 1, bucket = bucket->siblingThread)
	{
		gGT = GAME_TRACKER;
		inst = bucket->inst;
		if (gGT->numPlyrCurrGame != 1 && gGT->numPlyrCurrGame != 3)
		{
			continue;
		}

		d = bucket->object;
		kartColor = (u16)GAME_CHARACTER_IDS[d->driverID] + CRASH_BLUE;

		// NOTE(aalhendi): Prepare one shared icon call in each branch. Separate
		// calls hoist the scale into an extra saved register with GCC 2.8.1.
		if (d->actionsFlagSet & ACTION_BOT)
		{
			drawMap = map;
			worldPos = inst->matrix.t;
			iconID = UI_MAP_PLAYER_ICON_AI;
			// Sign-extend the palette index in place, without signed left-shift overflow.
			drawColor = (u32)kartColor << 16;
			drawColor >>= 16;
		}
		else if (gGT->gameMode1 & ADVENTURE_ARENA)
		{
			UI_Map_DrawAdvPlayer(map, inst->matrix.t, UI_MAP_PLAYER_ICON_HUMAN, (gGT->timer & 2) ? (s16)kartColor : WHITE,
			                     (s16)(((u16)d->rotCurr.y + UI_MAP_ARROW_ROT_FLIP) | UI_MAP_ARROW_ROT_FLAG), UI_MAP_ADV_ARROW_SCALE);
			continue;
		}
		else
		{
			worldPos = inst->matrix.t;
			drawColor = (gGT->timer & 2) ? (s16)kartColor : WHITE;
			drawMap = map;
			iconID = UI_MAP_PLAYER_ICON_HUMAN;
		}
		UI_Map_DrawRawIcon(drawMap, worldPos, iconID, drawColor, 0, UI_MAP_ICON_SCALE);
	}
	return;
}

void UI_Map_DrawGhosts(struct UIMap *map, struct Thread *bucket)
{
	s32 color;
	struct Driver *d;
	struct Instance *inst;

	for (/* bucket */; bucket != 0; bucket = bucket->siblingThread)
	{
		d = bucket->object;
		inst = bucket->inst;
#ifdef CTR_NATIVE
		// NOTE(aalhendi): An absent/unallocated ghost has no drawable instance.
		if (d == NULL || inst == NULL)
		{
			continue;
		}
#endif

		// if ghost not initialized
		if (d->ghostBoolInit == 0)
		{
			continue;
		}

		// Staff ghosts: N. Tropy is blue; Oxide flashes white/red.
		if (d->ghostID != 0)
		{
			color = (GAME_SAVE.progress.highScoreTracks[GAME_TRACKER->levelID].timeTrialFlags & TT_NTROPY_BEATEN) ? ((GAME_TRACKER->timer & 1) ? WHITE : RED)
			                                                                                                      : TROPY_LIGHT_BLUE;
		}

		// The player's recorded ghost flashes blue/red.
		else
		{
			color = (GAME_TRACKER->timer & 1) ? CRASH_BLUE : CORTEX_RED;
		}

		UI_Map_DrawRawIcon(map, &inst->matrix.t[0], UI_MAP_PLAYER_ICON_AI, color, 0, UI_MAP_ICON_SCALE);
	}
	return;
}

void UI_Map_DrawTracking(struct UIMap *map, struct Thread *bucket)
{
	struct Instance *inst;
	struct TrackerWeapon *tw;
	struct Driver *d;

	for (/* bucket */; bucket != 0; bucket = bucket->siblingThread)
	{
		// thread -> instance
		inst = bucket->inst;

		// instance -> model -> modelID != warpball
		if (inst->model->id != DYNAMIC_WARPBALL)
		{
			continue;
		}

		// draw warpball
		UI_Map_DrawRawIcon(map, &inst->matrix.t[0], UI_MAP_WARPBALL_ICON, 0, 0, UI_MAP_ICON_SCALE);

		// driver target
		tw = (struct TrackerWeapon *)inst->thread->object;
		d = tw->driverTarget;

		// check if target exists
		if (d == 0)
		{
			continue;
		}
#ifdef CTR_NATIVE
		if (d->instSelf == NULL)
		{
			continue;
		}
#endif

		// The target flashes between the white and red gradients.
		UI_Map_DrawRawIcon(map, &d->instSelf->matrix.t[0], UI_MAP_WARPBALL_TARGET_ICON, (GAME_TRACKER->timer & 1) ? RED : WHITE, 0, UI_MAP_ICON_SCALE);
	}
	return;
}
