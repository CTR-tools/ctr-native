#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004d614-0x8004d8b4.
void UI_Map_DrawMap(struct Icon *mapTop, struct Icon *mapBottom, s16 posX, s16 posY, struct PrimMem *primMem, uint32_t *otMem, u32 colorID)
{
	s16 mapBottomHeight;
	s16 mapTopHeight;
	int iVar9;
	POLY_FT4 *p;
	u32 color;
	u32 transparency;
	struct GameTracker *gGT;

	gGT = sdata->gGT;

	iVar9 = 0;

	// draw minimap with neutral/none vertex color, minimap's regular color is white
	color = 0x808080;
	transparency = colorID;

	// draw map black
	// used for the minimap shadow in the track select screen
	if (colorID == 2)
	{
		color = 0;
		transparency = 0;
	}

	// draw minimap blue
	// used for the minimap outline in the track select screen
	if (colorID == 3)
	{
		color = 0x402000;
		transparency = 0;
	}

	if (gGT->level1->ptrSpawnType1 != 0)
	{
		void **pointers = ST1_GETPOINTERS(gGT->level1->ptrSpawnType1);
		iVar9 = (int)pointers[ST1_MAP];
	}

	// position of the bottom margin of the primitive for the bottom half of the minimap
	mapBottomHeight = mapBottom->texLayout.v2 - mapBottom->texLayout.v0;

	p = (POLY_FT4 *)primMem->cursor;

	// if these conditions are met, then draw the top half of the minimap; otherwise, only draw the bottom half
	// not sure when the game ever draws only the bottom half
	if (((iVar9 != 0) && (*(s16 *)(iVar9 + 0x12) == 0)) ||

	    // if in main menu (character selection, track selection, any part of it)
	    ((gGT->gameMode1 & MAIN_MENU) != 0))
	{
		// r0, g0, b0 (vertex color)
		*(int *)&p->r0 = color;

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
	*(int *)&p->r0 = color;

	p->y0 = posY - mapBottomHeight;
	p->y1 = posY - mapBottomHeight;
	p->y2 = posY;
	p->y3 = posY;

	UI_Map_DrawMap_ExtraFunc(mapBottom, p, posX, 0, primMem, otMem, transparency);

	primMem->cursor = p + 1;
}

void UI_Map_DrawMap_ExtraFunc(struct Icon *icon, POLY_FT4 *p, s16 posX, s16 empty, struct PrimMem *primMem, uint32_t *otMem, u32 transparency)
{
	s16 leftX;
	s16 sizeX;

	sizeX = icon->texLayout.u1 - icon->texLayout.u0;

	// posX is the right side,
	// letftX is the left side
	leftX = posX - sizeX;

	p->x0 = leftX;
	p->x1 = posX;
	p->x2 = leftX;
	p->x3 = posX;

	// set header
	setPolyFT4(p);

	// UVs
	*(int *)&p->u0 = *(int *)&icon->texLayout.u0;
	*(int *)&p->u1 = *(int *)&icon->texLayout.u1;
	*(int *)&p->u2 = *(int *)&icon->texLayout.u2;
	*(s16 *)&p->u3 = *(s16 *)&icon->texLayout.u3;

	if (transparency != 0)
	{
		p->tpage = (p->tpage & 0xff9f) | ((u16)transparency << 5);
	}

	p->code |= 2;

	AddPrim(otMem, p);
}

// move to headers later
struct Map
{
	s16 worldEndX;
	s16 worldEndY;
	s16 worldStartX;
	s16 worldStartY;

	s16 iconSizeX;
	s16 iconSizeY;
	s16 iconStartX;
	s16 iconStartY;

	s16 mode;
};

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004d8b4-0x8004dbac.
void UI_Map_GetIconPos(s16 *m, int *posX, int *posY)

{
	s16 mode;
	int addX;
	int addY;
	int worldRangeX;
	int worldRangeY;

	struct Map *map = (struct Map *)m;

#if 0
  // trap() functions were removed from original,
  // we assume dividing by zero will never happen
#endif

	// rendering mode (forward, sideways, etc)
	mode = map->mode;

	worldRangeX = map->worldEndX - map->worldStartX;
	worldRangeY = map->worldEndY - map->worldStartY;

	if (mode == 0)
	{
		// 0 degrees
		addX = (*posX * map->iconSizeX) / worldRangeX;
		addY = (*posY * map->iconSizeY * 2) / worldRangeY;
	}

	else if (mode == 1)
	{
		// 90 degrees
		addX = -(*posY * map->iconSizeX) / worldRangeY;
		addY = (*posX * map->iconSizeY * 2) / worldRangeX;
	}

	else if (mode == 2)
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
		addX -= 60;
		addY += 10;
	}

	*posX = map->iconStartX + addX;
	*posY = map->iconStartY + addY - 0x10;
	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004dbac-0x8004dc44.
// Draw dot for Player on 2D Adv Map
void UI_Map_DrawAdvPlayer(int ptrMap, int *matrix, int unused1, int unused2, s16 param_5, s16 param_6)
{
	int *arrowColor;
	int posX;
	int posY;

	posX = *matrix;
	posY = matrix[2];

	UI_Map_GetIconPos((s16 *)ptrMap, &posX, &posY);

	arrowColor = &data.playerIconAdvMap.vertCol1[0];
	if ((sdata->gGT->timer & 2) != 0)
	{
		arrowColor = &data.playerIconAdvMap.vertCol2[0];
	}

	AH_Map_HubArrow(posX, posY, &data.playerIconAdvMap.unk_playerAdvMap[0], (char *)arrowColor, (int)param_6, (int)param_5);

	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004dc44-0x8004dd5c.
// Draw icon on map
void UI_Map_DrawRawIcon(int ptrMap, int *param_2, int iconID, int colorID, int unused, s16 scale)
{
	int posX;
	int posY;
	int *ptrColor;
	struct GameTracker *gGT = sdata->gGT;

	(void)unused;

	posX = param_2[0];
	posY = param_2[2];

	UI_Map_GetIconPos((s16 *)ptrMap, &posX, &posY);

	ptrColor = data.ptrColor[colorID];

	struct Icon **iconPtrArray = ICONGROUP_GETICONS(sdata->gGT->iconGroup[5]);

	DecalHUD_DrawPolyGT4(iconPtrArray[iconID], posX, posY, &gGT->backBuffer->primMem, gGT->pushBuffer_UI.ptrOT, ptrColor[0], ptrColor[1], ptrColor[2],
	                     ptrColor[3], 0, (int)scale);

	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004dd5c-0x8004dee8.
void UI_Map_DrawDrivers(int ptrMap, struct Thread *bucket, s16 *param_3)

{
	int kartColor;
	int iconID;
	struct Driver *d;
	struct GameTracker *gGT = sdata->gGT;

	// if 2P or 4P
	if ((gGT->numPlyrCurrGame & 1) == 0)
	{
		// quit, no map drawn
		return;
	}

	for (/* bucket */; bucket != 0; bucket = bucket->siblingThread, *param_3 = *param_3 + 1)
	{
		// Player structure
		d = bucket->object;

		// characterID + 5
		// corresponds with ptrColors
		kartColor = data.characterIDs[d->driverID] + 5;

		// default (AI)
		iconID = 0x31;

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
				UI_Map_DrawAdvPlayer(ptrMap, (int *)&bucket->inst->matrix.t[0], 0x32, kartColor, (d->rotCurr.y + 0x800U) | 0x1000, 0x800);

				continue;
			}

			// Player
			iconID = 0x32;
		}

		UI_Map_DrawRawIcon(ptrMap, (int *)&bucket->inst->matrix.t[0], iconID, (s16)kartColor, 0, 0x1000);
	}
	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004dee8-0x8004dffc.
void UI_Map_DrawGhosts(int ptrMap, struct Thread *bucket)
{
	int color;
	struct Driver *d;
	struct GameTracker *gGT = sdata->gGT;

	for (/* bucket */; bucket != 0; bucket = bucket->siblingThread)
	{
		d = bucket->object;

		// Need to finish Driver struct

		// if ghost not initialized
		if (*(s16 *)((uintptr_t)d + 0x632) == 0)
			continue;

		// ghost made by player
		if (*(s16 *)((uintptr_t)d + 0x630) == 0)
		{
			// flash red and blue

			color = CORTEX_RED;
			if ((gGT->timer & 1) != 0)
				color = CRASH_BLUE;
		}

		// ghost is N Tropy or Oxide
		else
		{
			// N Tropy doesn't flicker
			color = TROPY_LIGHT_BLUE;

			// if timeTrialFlags for this track show [ n tropy beaten, oxide open ]
			if ((sdata->gameProgress.highScoreTracks[gGT->levelID].timeTrialFlags & 2) != 0)
			{
				// oxide flickers

				color = RED;
				if ((gGT->timer & 1) != 0)
					color = WHITE;
			}
		}

		UI_Map_DrawRawIcon(ptrMap, (int *)&bucket->inst->matrix.t[0], 0x31, color, 0, 0x1000);
	}
	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004dffc-0x8004e0e0.
void UI_Map_DrawTracking(int ptrMap, struct Thread *bucket)

{
	int uVar1;
	struct Instance *inst;
	struct TrackerWeapon *tw;
	struct Driver *d;

	for (/* bucket */; bucket != 0; bucket = bucket->siblingThread)
	{
		// thread -> instance
		inst = bucket->inst;

		// instance -> model -> modelID != warpball
		if (inst->model->id != DYNAMIC_WARPBALL)
			continue;

		// == only draw warpball ==

		// draw warpball
		UI_Map_DrawRawIcon(ptrMap, (int *)&inst->matrix.t[0], 0x20, 0, 0, 0x1000);

		// driver target
		tw = (struct TrackerWeapon *)inst->thread->object;
		d = tw->driverTarget;

		// check if target exists
		if (d == 0)
			continue;

		// == only draw target if target exists ==

		// flicker
		uVar1 = 4;
		if ((sdata->gGT->timer & 1) != 0)
			uVar1 = 3;

		UI_Map_DrawRawIcon(ptrMap, (int *)&d->instSelf->matrix.t[0], 0x21, uVar1, 0, 0x1000);
	}
	return;
}
