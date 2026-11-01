#include <common.h>

void UI_RenderFrame_Wumpa3D_2P3P4P(struct GameTracker *gGT)
{
	struct GameTracker *initialGT = GAME_TRACKER;
	RECT viewport2P;
	RECT viewport3P4P;
	RECT *viewport;
	struct PushBuffer *wumpaPushBuffer;
	struct UiElement2D *hud;
	s32 playerIndex;
	s32 posX, posY;
	s32 halfOffset;

	viewport = &viewport2P;
	viewport2P = GAME_WUMPA_HUD_VIEWPORT_2P;
	viewport3P4P = GAME_WUMPA_HUD_VIEWPORT_3P4P;
	if (initialGT->numPlyrCurrGame >= 3)
	{
		viewport = &viewport3P4P;
	}

	// NOTE(aalhendi): UI_INSTANCE_InitAll publishes the push buffer here,
	// not in the adjacent ptrFruitDisp instance slot.
	wumpaPushBuffer = (struct PushBuffer *)(u32)GAME_SDATA_NAME(ptrPushBufferUI);
	if (wumpaPushBuffer != NULL)
	{
		struct PushBuffer *texturePushBuffer;
		u32 *textureStart;
		u32 *textureEnd;
		s32 viewportY, halfHeight;
		posX = viewport->x;
		halfOffset = viewport->w >> 1;
		halfOffset -= 0x100;
		posX += halfOffset;
		viewportY = viewport->y;
		halfHeight = (viewport->h >> 1) - 0x6c;
		posY = viewportY + halfHeight;
		PushBuffer_SetDrawEnv_DecalMP(wumpaPushBuffer->renderBucketOTRangeEnd, initialGT->backBuffer, viewport, posX, posY, 0, 0, 0, 0, 1);
		texturePushBuffer = (struct PushBuffer *)(u32)GAME_SDATA_NAME(ptrPushBufferUI);
		textureStart = texturePushBuffer->ptrOT;
		if (textureStart != NULL)
		{
			textureEnd = texturePushBuffer->renderBucketOTRangeEnd;
			if (textureEnd != NULL)
			{
#ifdef CTR_NATIVE
				// NOTE(aalhendi): Compiled-in overlays can retain a stale OT range.
				// Only cycle links belonging to the current native back buffer.
				if (CtrGpu_IsCurrentOTRange(gGT->backBuffer, textureStart, textureEnd))
#endif
				{
					CTR_CycleTex_2p3p4pWumpaHUD(&gGT->pushBuffer[0].ptrOT[0x3ff], textureStart, (int)(textureEnd - textureStart) + 1);
				}
			}
		}
	}

	if (gGT->numPlyrCurrGame < 2)
	{
		return;
	}
	hud = GAME_HUD_LAYOUTS[gGT->numPlyrCurrGame - 1];
	for (playerIndex = 0; playerIndex < gGT->numPlyrCurrGame; playerIndex++)
	{
		struct GameTracker *current = GAME_TRACKER;
		struct UiElement2D *playerHud = &hud[playerIndex * UI_HUD_SLOT_COUNT];
		POLY_FT4 *prim;
		s32 positionSumX, positionSumY;

		if ((current->drivers[playerIndex]->actionsFlagSet & ACTION_RACE_FINISHED) != 0)
		{
			continue;
		}
		if ((current->gameMode1 & END_OF_RACE) != 0)
		{
			continue;
		}

		posX = playerHud[UI_HUD_SLOT_FRUIT_MODEL].x;
		posY = playerHud[UI_HUD_SLOT_FRUIT_MODEL].y;
		// NOTE(aalhendi): Keep the one-use sums separate from the reused
		// coordinates; their lifetimes preserve retail allocation and scheduling.
		positionSumX = posX + ((struct PushBuffer *)(u32)GAME_SDATA_NAME(ptrPushBufferUI))->rect.x;
		posX = positionSumX - (viewport->w >> 1);
		positionSumY = posY + ((struct PushBuffer *)(u32)GAME_SDATA_NAME(ptrPushBufferUI))->rect.y;
		posY = positionSumY - (viewport->h >> 1);
		prim = (POLY_FT4 *)current->backBuffer->primMem.cursor;
		setlen(prim, 9);
		prim->code = 0x2c;
		prim->r0 = prim->g0 = prim->b0 = 0x80;
		prim->x0 = posX;
		prim->y0 = posY;
		prim->x1 = posX + viewport->w;
		prim->y1 = posY;
		prim->x2 = posX;
		prim->y2 = posY + viewport->h;
		prim->x3 = posX + viewport->w;
		prim->y3 = posY + viewport->h;
		prim->u0 = viewport->x & 0x3f;
		prim->v0 = viewport->y;
		prim->u1 = prim->u0 + viewport->w;
		prim->v1 = prim->v0;
		prim->u2 = prim->u0;
		prim->v2 = prim->v0 + viewport->h;
		prim->u3 = prim->u0 + viewport->w;
		prim->v3 = prim->v0 + viewport->h;
		prim->tpage = getTPage(2, 0, viewport->x, viewport->y);
		if (GAME_TRACKER->drivers[playerIndex]->numWumpas >= DRIVER_WUMPA_JUICED_COUNT)
		{
			prim->r0 = prim->g0 = prim->b0 = GAME_SDATA_NAME(wumpaShineResult);
		}
		gGT->backBuffer->primMem.cursor = (POLY_FT4 *)gGT->backBuffer->primMem.cursor + 1;
		AddPrim(gGT->pushBuffer_UI.ptrOT, prim);
	}
}
