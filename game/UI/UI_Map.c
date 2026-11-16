#include <common.h>

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
	UI_MAP_RIGHT_EDGE = 500,
};


void UI_Map_DrawMap(struct Icon *mapTop, struct Icon *mapBottom, s32 posX, s32 posY, struct PrimMem *primMem, u32 *otMem, u32 colorID)
{
	s16 mapBottomHeight;
	s16 mapTopHeight;
	struct UIMapSpawnMetadata *mapMetadata;
	POLY_FT4 *p;
	u32 color;
	u32 transparency;
	struct GameTracker *gGT;

	gGT = sdata->gGT;

	mapMetadata = NULL;

	// draw minimap with neutral/none vertex color, minimap's regular color is white
	color = UI_MAP_NEUTRAL_COLOR;
	transparency = colorID;

	// draw map black
	// used for the minimap shadow in the track select screen
	if (colorID == UI_MAP_COLOR_MODE_BLACK)
	{
		color = 0;
		transparency = 0;
	}

	// draw minimap blue
	// used for the minimap outline in the track select screen
	if (colorID == UI_MAP_COLOR_MODE_BLUE)
	{
		color = UI_MAP_BLUE_OUTLINE_COLOR;
		transparency = 0;
	}

	if ((gGT->level1->ptrSpawnType1 != NULL) && (gGT->level1->ptrSpawnType1->count != 0))
	{
		void **pointers = ST1_GETPOINTERS(gGT->level1->ptrSpawnType1);
		mapMetadata = pointers[ST1_MAP];
	}

	// position of the bottom margin of the primitive for the bottom half of the minimap
	mapBottomHeight = mapBottom->texLayout.v2 - mapBottom->texLayout.v0;

	p = (POLY_FT4 *)primMem->cursor;

	// if these conditions are met, then draw the top half of the minimap; otherwise, only draw the bottom half
	// not sure when the game ever draws only the bottom half
	if (((mapMetadata != NULL) && (mapMetadata->topHalfMode == 0)) ||

	    // if in main menu (character selection, track selection, any part of it)
	    ((gGT->gameMode1 & MAIN_MENU) != 0))
	{
		// r0, g0, b0 (vertex color)
		CtrGpu_WriteColorCode(&p->r0, color);

		// position of the top margin of the primitive for the top half of the minimap
		mapTopHeight = posY - (((u16)mapTop->texLayout.v2 - (u16)mapTop->texLayout.v0) + mapBottomHeight);

		p->y0 = mapTopHeight;
		p->y1 = mapTopHeight;
		p->y2 = posY - mapBottomHeight;
		p->y3 = posY - mapBottomHeight;

		UI_Map_DrawMap_ExtraFunc(mapTop, p, posX, 0, primMem, otMem, transparency);

		p = p + 1;
	}

	// r0, g0, b0 (vertex color)
	CtrGpu_WriteColorCode(&p->r0, color);

	p->y0 = posY - mapBottomHeight;
	p->y1 = posY - mapBottomHeight;
	p->y2 = posY;
	p->y3 = posY;

	UI_Map_DrawMap_ExtraFunc(mapBottom, p, posX, 0, primMem, otMem, transparency);

	primMem->cursor = p + 1;
}

void UI_Map_DrawMap_ExtraFunc(struct Icon *icon, POLY_FT4 *p, s16 posX, s16 empty, struct PrimMem *primMem, u32 *otMem, u32 transparency)
{
	(void)empty;
	(void)primMem;
	s16 leftX;
	s16 sizeX;

	sizeX = icon->texLayout.u1 - icon->texLayout.u0;

	// posX is the right side,
	// letftX is the left side
	leftX = Widescreen_ScaleXFromAnchor(posX - sizeX, posX);

	p->x0 = leftX;
	p->x1 = posX;
	p->x2 = leftX;
	p->x3 = posX;

	// set header
	setPolyFT4(p);

	// UVs
	CtrGpu_WritePackedUVWord(&p->u0, CTR_ReadU32LE(&icon->texLayout.u0));
	CtrGpu_WritePackedUVWord(&p->u1, CTR_ReadU32LE(&icon->texLayout.u1));
	CtrGpu_WritePackedUVWord(&p->u2, CTR_ReadU32LE(&icon->texLayout.u2));
	CtrGpu_WritePackedUV(&p->u3, CTR_ReadU16LE(&icon->texLayout.u3));

	if (transparency != 0)
	{
		p->tpage = (p->tpage & UI_MAP_TPAGE_BLEND_MASK) | ((u16)transparency << UI_MAP_TPAGE_BLEND_SHIFT);
	}

	p->code |= UI_MAP_SEMI_TRANS_CODE_BIT;

	AddPrim(otMem, p);
}

void UI_Map_GetIconPos(struct UIMap *map, int *posX, int *posY)
{
	s16 mode;
	int addX;
	int addY;
	int worldRangeX;
	int worldRangeY;
	// rendering mode (forward, sideways, etc)
	mode = map->mode;

	worldRangeX = map->worldEndX - map->worldStartX;
	worldRangeY = map->worldEndY - map->worldStartY;

	if (mode == UI_MAP_MODE_0_DEGREES)
	{
		// 0 degrees
		addX = (*posX * map->iconSizeX) / worldRangeX;
		addY = (*posY * map->iconSizeY * 2) / worldRangeY;
	}

	else if (mode == UI_MAP_MODE_90_DEGREES)
	{
		// 90 degrees
		addX = -(*posY * map->iconSizeX) / worldRangeY;
		addY = (*posX * map->iconSizeY * 2) / worldRangeX;
	}

	else if (mode == UI_MAP_MODE_180_DEGREES)
	{
		// 180 degrees
		addX = -(*posX * map->iconSizeX) / worldRangeX;
		addY = -(*posY * map->iconSizeY * 2) / worldRangeY;
	}

	else
	{
		// 270 degrees
		addX = (*posY * map->iconSizeX) / worldRangeY;
		addY = -(*posX * map->iconSizeY * 2) / worldRangeX;
	}

	if (sdata->gGT->numPlyrCurrGame == 3)
	{
		addX -= UI_MAP_3P_OFFSET_X;
		addY += UI_MAP_3P_OFFSET_Y;
	}

	int rightEdge = UI_MAP_RIGHT_EDGE;
	if (sdata->gGT->numPlyrCurrGame == 3)
	{
		rightEdge -= UI_MAP_3P_OFFSET_X;
	}
	*posX = Widescreen_ScaleXFromAnchor(map->iconStartX + addX, rightEdge);
	*posY = map->iconStartY + addY - UI_MAP_ICON_Y_OFFSET;
	return;
}

// Draw dot for Player on 2D Adv Map
void UI_Map_DrawAdvPlayer(struct UIMap *map, const s32 worldPos[3], int unused1, int unused2, s16 rot, s16 scale)
{
	(void)unused1;
	(void)unused2;
	int *arrowColor;
	int posX;
	int posY;

	posX = worldPos[0];
	posY = worldPos[2];

	UI_Map_GetIconPos(map, &posX, &posY);

	arrowColor = &data.playerIconAdvMap.vertCol1[0];
	if ((sdata->gGT->timer & 2) != 0)
	{
		arrowColor = &data.playerIconAdvMap.vertCol2[0];
	}

	AH_Map_HubArrow(posX, posY, &data.playerIconAdvMap.pos[0], (char *)arrowColor, (int)scale, (int)rot);

	return;
}

// Draw icon on map
void UI_Map_DrawRawIcon(struct UIMap *map, const s32 worldPos[3], int iconID, int colorID, int unused, s16 scale)
{
	int posX;
	int posY;
	u32 *ptrColor;
	struct GameTracker *gGT = sdata->gGT;

	(void)unused;

	posX = worldPos[0];
	posY = worldPos[2];

	UI_Map_GetIconPos(map, &posX, &posY);

	ptrColor = data.ptrColor[colorID];

	struct Icon **iconPtrArray = ICONGROUP_GETICONS(sdata->gGT->iconGroup[UI_MAP_ICON_GROUP]);
	struct Icon *icon = iconPtrArray[iconID];
	if (icon != NULL)
	{
		// DrawPolyGT4 compresses around the icon's center, but posX is its
		// original left edge. Keep that edge in the map's coordinate space.
		int width = ((icon->texLayout.u1 - icon->texLayout.u0) * scale) >> 12;
		posX -= Widescreen_CenteredEdgeShift(width);
	}

	DecalHUD_DrawPolyGT4(icon, posX, posY, &gGT->backBuffer->primMem, gGT->pushBuffer_UI.ptrOT, ColorCode_Load(&ptrColor[0]),
	                     ColorCode_Load(&ptrColor[1]), ColorCode_Load(&ptrColor[2]), ColorCode_Load(&ptrColor[3]), 0, (int)scale);

	return;
}

void UI_Map_DrawDrivers(struct UIMap *map, struct Thread *bucket, s16 *driverIconCounter)
{
	int kartColor;
	int iconID;
	struct Driver *d;
	struct GameTracker *gGT = sdata->gGT;

	for (/* bucket */; bucket != 0; bucket = bucket->siblingThread, *driverIconCounter = *driverIconCounter + 1)
	{
		// if 2P or 4P
		if ((gGT->numPlyrCurrGame & 1) == 0)
		{
			continue;
		}

		// Player structure
		d = bucket->object;

		// characterID + 5
		// corresponds with ptrColors
		kartColor = data.characterIDs[d->driverID] + 5;

		// default (AI)
		iconID = UI_MAP_PLAYER_ICON_AI;

		// TO-DO: Should we just spawn player threads
		// and enable the AI flag anyway? What would it do?
		if ((d->actionsFlagSet & ACTION_BOT) == 0)

		{
			// If this is an even numbered frame
			// ptrColors white value
			if ((gGT->timer & 2) == 0)
			{
				kartColor = WHITE;
			}

			// If you're in Adventure Arena
			if ((gGT->gameMode1 & ADVENTURE_ARENA) != 0)
			{
				// Draw dot for Player on 2D Adv Map
				UI_Map_DrawAdvPlayer(map, &bucket->inst->matrix.t[0], UI_MAP_PLAYER_ICON_HUMAN, kartColor,
				                     (d->rotCurr.y + UI_MAP_ARROW_ROT_FLIP) | UI_MAP_ARROW_ROT_FLAG, UI_MAP_ADV_ARROW_SCALE);

				continue;
			}

			// Player
			iconID = UI_MAP_PLAYER_ICON_HUMAN;
		}

		UI_Map_DrawRawIcon(map, &bucket->inst->matrix.t[0], iconID, (s16)kartColor, 0, UI_MAP_ICON_SCALE);
	}
	return;
}

void UI_Map_DrawGhosts(struct UIMap *map, struct Thread *bucket)
{
	int color;
	struct Driver *d;
	struct GameTracker *gGT = sdata->gGT;

	for (/* bucket */; bucket != 0; bucket = bucket->siblingThread)
	{
		d = bucket->object;

		// if ghost not initialized
		if (d->ghostBoolInit == 0)
		{
			continue;
		}

		// ghost made by player
		if (d->ghostID == 0)
		{
			// flash red and blue

			color = CORTEX_RED;
			if ((gGT->timer & 1) != 0)
			{
				color = CRASH_BLUE;
			}
		}

		// ghost is N Tropy or Oxide
		else
		{
			// N Tropy doesn't flicker
			color = TROPY_LIGHT_BLUE;

			// if timeTrialFlags for this track show [ n tropy beaten, oxide open ]
			if ((sdata->gameSave.progress.highScoreTracks[gGT->levelID].timeTrialFlags & 2) != 0)
			{
				// oxide flickers

				color = RED;
				if ((gGT->timer & 1) != 0)
				{
					color = WHITE;
				}
			}
		}

		UI_Map_DrawRawIcon(map, &bucket->inst->matrix.t[0], UI_MAP_PLAYER_ICON_AI, color, 0, UI_MAP_ICON_SCALE);
	}
	return;
}

void UI_Map_DrawTracking(struct UIMap *map, struct Thread *bucket)
{
	int targetColor;
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

		// == only draw warpball ==

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

		// == only draw target if target exists ==

		// flicker
		targetColor = CRASH_BLUE;
		if ((sdata->gGT->timer & 1) != 0)
		{
			targetColor = CORTEX_RED;
		}

		UI_Map_DrawRawIcon(map, &d->instSelf->matrix.t[0], UI_MAP_WARPBALL_TARGET_ICON, targetColor, 0, UI_MAP_ICON_SCALE);
	}
	return;
}
