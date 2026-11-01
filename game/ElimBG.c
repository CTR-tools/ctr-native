#include <common.h>

enum ElimBGPauseState
{
	ELIM_BG_PAUSE_STATE_NONE = 0,
	ELIM_BG_PAUSE_STATE_CAPTURE = 1,
	ELIM_BG_PAUSE_STATE_DRAW = 2,
	ELIM_BG_PAUSE_STATE_RESTORE = 3,
};

enum ElimBGVramSlot
{
	ELIM_BG_SLOT_TEXTURE_DB0 = 0,
	ELIM_BG_SLOT_TEXTURE_DB1 = 1,
	ELIM_BG_SLOT_RAW_STRIP_DB0 = 2,
	ELIM_BG_SLOT_RAW_STRIP_DB1 = 3,
	ELIM_BG_SLOT_PACKED_STRIP_DB0 = 4,
	ELIM_BG_SLOT_PACKED_STRIP_DB1 = 5,
};

enum ElimBGConstants
{
	ELIM_BG_TEXTURE_LEFT_X = 0x200,
	ELIM_BG_TEXTURE_RIGHT_X = 0x240,
	ELIM_BG_TEXTURE_BACKUP_W = 0x40,
	ELIM_BG_TEXTURE_BACKUP_H = 0x100,
	ELIM_BG_TEXTURE_BACKUP_BYTES = ELIM_BG_TEXTURE_BACKUP_W * ELIM_BG_TEXTURE_BACKUP_H * sizeof(u16),
	ELIM_BG_PRIMMEM_PAUSE_BYTES = 0xc800,
	ELIM_BG_RAW_STRIP_OFFSET = 0x800,
	ELIM_BG_TEXTURE_BACKUP_OFFSET = 0x4800,
	ELIM_BG_SCREEN_W = 0x200,
	ELIM_BG_SCREEN_H = 0xd8,
	ELIM_BG_SWAPCHAIN_Y_STRIDE = 0x128,
	ELIM_BG_STRIP_H = 8,
	ELIM_BG_CAPTURE_VRAM_X = 0x200,
	ELIM_BG_CAPTURE_VRAM_W = 0x80,
	ELIM_BG_FINAL_STRIP_Y = 0xff,
	ELIM_BG_FINAL_STRIP_W = 0x10,
	ELIM_BG_CHUNK_SOURCE_PIXELS = 0x1000,
	ELIM_BG_TILE_W = 0x80,
	ELIM_BG_TILE_H = 0x10,
	ELIM_BG_TILE_COLOR = 0x80,
	ELIM_BG_TILE_CLUT = 0x3fe0,
	ELIM_BG_U_LIMIT = 0x100,
	ELIM_BG_VRAM_U_WRAP = 0x80,
};


void ElimBG_SaveScreenshot_Chunk(u16 *packedStrip, u16 *rawStrip, int rawPixelCount)
{
	register u16 *rawCursor CTR_PSX_REGISTER("$7") = rawStrip;
	u16 *rawGroupLast;

	if (rawPixelCount == 0)
	{
		return;
	}

	rawGroupLast = rawStrip + 3;

	// NOTE(aalhendi): The count must be a nonnegative multiple of four. A destination may alias later source pixels,
	// so each partial store precedes its next source read.
	do
	{
		*packedStrip = (u16)((rawCursor[0] & 0x3e0) >> 6);
		*packedStrip |= rawGroupLast[-2] >> 2 & 0xf0;
		*packedStrip |= (u16)(((u32)rawGroupLast[-1] << 2) & 0xf00);
		*packedStrip |= (u16)(((u32)*rawGroupLast << 6) & 0xf000);
		rawCursor += 4;
		rawPixelCount -= 4;
		rawGroupLast += 4;
		packedStrip++;
	} while (rawPixelCount != 0);
}


void ElimBG_SaveScreenshot_Full(struct GameTracker *gGT)
{
	s32 screenY;
	register char **packedBuffers CTR_PSX_REGISTER("$20");
	char *texture0;
	char *texture1;
	char *packed0;
	RECT pageRects[2] = {{ELIM_BG_TEXTURE_LEFT_X, 0, ELIM_BG_TEXTURE_BACKUP_W, ELIM_BG_TEXTURE_BACKUP_H},
	                     {ELIM_BG_TEXTURE_RIGHT_X, 0, ELIM_BG_TEXTURE_BACKUP_W, ELIM_BG_TEXTURE_BACKUP_H}};
	RECT rSrc;
	RECT rDst;
	// NOTE(aalhendi): This real A0 argument dies at the first StoreImage; later calls use their own pointers.
	register RECT *backupPage CTR_PSX_REGISTER("$4") = pageRects;
	s32 bufferIndex = 0;
	s32 stripY = bufferIndex;

	CTR_PSX_OBSERVE_VALUE(backupPage);

	// Back up the overwritten VRAM pages above the alternating raw/packed strips.
	texture0 = (char *)gGT->db[0].primMem.end - ELIM_BG_TEXTURE_BACKUP_BYTES;
	sdata->PausePtrsVRAM[ELIM_BG_SLOT_TEXTURE_DB0] = texture0;
	texture1 = (char *)gGT->db[1].primMem.end - ELIM_BG_TEXTURE_BACKUP_BYTES;
	sdata->PausePtrsVRAM[ELIM_BG_SLOT_RAW_STRIP_DB0] = texture0 - (ELIM_BG_TEXTURE_BACKUP_OFFSET - ELIM_BG_RAW_STRIP_OFFSET);
	packed0 = texture0 - ELIM_BG_TEXTURE_BACKUP_OFFSET;
	sdata->PausePtrsVRAM[ELIM_BG_SLOT_PACKED_STRIP_DB0] = packed0;
	sdata->PausePtrsVRAM[ELIM_BG_SLOT_TEXTURE_DB1] = texture1;
	sdata->PausePtrsVRAM[ELIM_BG_SLOT_RAW_STRIP_DB1] = texture1 - (ELIM_BG_TEXTURE_BACKUP_OFFSET - ELIM_BG_RAW_STRIP_OFFSET);
	sdata->PausePtrsVRAM[ELIM_BG_SLOT_PACKED_STRIP_DB1] = texture1 - ELIM_BG_TEXTURE_BACKUP_OFFSET;
	gGT->db[0].primMem.end = packed0;
	gGT->db[1].primMem.end = sdata->PausePtrsVRAM[ELIM_BG_SLOT_PACKED_STRIP_DB1];
	{
		register char **rawBuffers CTR_PSX_REGISTER("$23");
		u32 *backupTexture = (u32 *)sdata->PausePtrsVRAM[ELIM_BG_SLOT_TEXTURE_DB0];
		// NOTE(aalhendi): Materialize the capture base after publishing the arenas and reading the first texture argument.
		CTR_PSX_BIND_VALUE_CLOBBER(backupTexture, "$23");
		rawBuffers = &GAME_SDATA_NAME(PausePtrsVRAM)[ELIM_BG_SLOT_RAW_STRIP_DB0];
		CTR_PSX_OBSERVE_VALUE(rawBuffers);
		StoreImage(backupPage, backupTexture);
		StoreImage(&pageRects[1], (u32 *)sdata->PausePtrsVRAM[ELIM_BG_SLOT_TEXTURE_DB1]);
		{
			RECT *captureRect = &rSrc;
			u32 *capturePixels = (u32 *)sdata->PausePtrsVRAM[ELIM_BG_SLOT_RAW_STRIP_DB0];
			screenY = gGT->swapchainIndex * ELIM_BG_SWAPCHAIN_Y_STRIDE;
			rSrc.x = 0;
			rSrc.y = screenY;
			rSrc.w = ELIM_BG_SCREEN_W;
			rSrc.h = ELIM_BG_STRIP_H;
			StoreImage(captureRect, capturePixels);
		}

		do
		{
			u32 previousByteOffset;
			char **previousPacked;
			bufferIndex = 1 - bufferIndex;
			rSrc.x = 0;
			rSrc.y = screenY + stripY + ELIM_BG_STRIP_H;
			rSrc.w = ELIM_BG_SCREEN_W;
			rSrc.h = ELIM_BG_STRIP_H;
			DrawSync(0);
			StoreImage(&rSrc, (u32 *)rawBuffers[bufferIndex]);
			// NOTE(aalhendi): Retain the byte offset before creating the packed base; all supported targets use 32-bit addresses.
			previousByteOffset = (1 - bufferIndex) * sizeof(*packedBuffers);
			CTR_PSX_BIND_VALUE_CLOBBER(previousByteOffset, "$20");
			packedBuffers = &GAME_SDATA_NAME(PausePtrsVRAM)[ELIM_BG_SLOT_PACKED_STRIP_DB0];
			previousPacked = (char **)((u8 *)packedBuffers + previousByteOffset);
			ElimBG_SaveScreenshot_Chunk((u16 *)*previousPacked, (u16 *)*(char **)(previousByteOffset + (u32)rawBuffers), ELIM_BG_CHUNK_SOURCE_PIXELS);
			rDst.x = ELIM_BG_CAPTURE_VRAM_X;
			rDst.y = stripY;
			rDst.w = ELIM_BG_CAPTURE_VRAM_W;
			rDst.h = ELIM_BG_STRIP_H;
			LoadImage(&rDst, (u32 *)*previousPacked);
			stripY += ELIM_BG_STRIP_H;
		} while (stripY + ELIM_BG_STRIP_H < ELIM_BG_SCREEN_H);
	}
	DrawSync(0);
	// NOTE(aalhendi): End the raw-base equivalence here, but retain the packed base through the final image upload.
	CTR_PSX_KEEP_VALUE(packedBuffers);
	ElimBG_SaveScreenshot_Chunk((u16 *)packedBuffers[bufferIndex], (u16 *)(&GAME_SDATA_NAME(PausePtrsVRAM)[ELIM_BG_SLOT_RAW_STRIP_DB0])[bufferIndex],
	                            ELIM_BG_CHUNK_SOURCE_PIXELS);
	rDst.x = ELIM_BG_CAPTURE_VRAM_X;
	rDst.y = stripY;
	rDst.w = ELIM_BG_CAPTURE_VRAM_W;
	rDst.h = ELIM_BG_STRIP_H;
	LoadImage(&rDst, (u32 *)packedBuffers[bufferIndex]);
	rDst.x = ELIM_BG_CAPTURE_VRAM_X;
	rDst.y = ELIM_BG_FINAL_STRIP_Y;
	rDst.w = ELIM_BG_FINAL_STRIP_W;
	rDst.h = 1;
	LoadImage(&rDst, &data.pauseScreenStrip[0]);
	CTR_PSX_OBSERVE_VALUE(packedBuffers);
}


void ElimBG_Activate(struct GameTracker *gGT)
{
	u32 renderFlags = gGT->renderFlags;

	sdata->pause_state = ELIM_BG_PAUSE_STATE_CAPTURE;
	sdata->pause_backup_renderFlags = renderFlags;
	sdata->pause_backup_hudFlags = gGT->hudFlags;
	// NOTE(aalhendi): Retail completes the HUD backup store before returning.
	CTR_PSX_MEMORY_BARRIER();
}


void ElimBG_ToggleInstance(struct Instance *inst, b32 boolGameIsPaused)
{
	u32 flags;
	u32 updatedFlags;
	register u32 pauseMask CTR_PSX_REGISTER("$3");
	register u32 clearedFlags CTR_PSX_REGISTER("$2");

	// if game is being paused
	if (boolGameIsPaused)
	{
		flags = inst->flags;

		if (flags & HIDE_MODEL)
		{
			updatedFlags = flags | INVISIBLE_BEFORE_PAUSE;
		}
		else
		{
			updatedFlags = flags & ~INVISIBLE_BEFORE_PAUSE;
		}

		inst->flags = updatedFlags;
		// NOTE(aalhendi): Retail stores and reloads flags between the two pause stages.
		CTR_PSX_MEMORY_BARRIER();
		inst->flags |= (INVISIBLE_DURING_PAUSE | HIDE_MODEL);

		return;
	}

	if ((inst->flags & (INVISIBLE_BEFORE_PAUSE | INVISIBLE_DURING_PAUSE)) == INVISIBLE_DURING_PAUSE)
	{
		// NOTE(aalhendi): Keep the two clears distinct and in retail's v1/v0 mask order.
		pauseMask = ~INVISIBLE_DURING_PAUSE;
		CTR_PSX_KEEP_VALUE(pauseMask);
		clearedFlags = inst->flags & ~HIDE_MODEL;
		CTR_PSX_KEEP_VALUE(clearedFlags);
		inst->flags = clearedFlags & pauseMask;
	}
}


void ElimBG_ToggleAllInstances(struct GameTracker *gGT, b32 boolGameIsPaused)
{
	struct Level *lev;
	struct Instance *inst;
	struct InstDef *ptrInstDefs;
	struct InstDef *endInstDef;
	int numInstances;

	lev = gGT->level1;
	numInstances = lev->numInstances;
	ptrInstDefs = lev->ptrInstDefs;
	endInstDef = ptrInstDefs + numInstances;

	// Loop through all instances in level
	for (; ptrInstDefs < endInstDef; ptrInstDefs++)
	{
		struct Instance *levelInst = ptrInstDefs->ptrInstance;

		if (levelInst != 0)
		{
			ElimBG_ToggleInstance(levelInst, boolGameIsPaused);
		}
	}

	// Loop through all instances in Instance Pool
	for (inst = (struct Instance *)LIST_GetFirstItem(&gGT->JitPools.instance.taken); inst != 0; inst = (struct Instance *)LIST_GetNextItem((struct Item *)inst))
	{
		ElimBG_ToggleInstance(inst, boolGameIsPaused);
	}
}


void ElimBG_HandleState(struct GameTracker *gGT)
{
	s16 screenY;
	s16 state;
	int screenXForTpage;
	int textureU;
	s32 lowerTextureU;
	POLY_FT4 *p;
	s32 textureX;
	s32 tpage;
	s32 pageOrigin;
	s32 textureY;
	int tileX;

	// if this is last frame of pause
	state = GAME_SDATA_NAME(pause_state);
	if (state == ELIM_BG_PAUSE_STATE_RESTORE)
	{
		u32 *texture0 = (u32 *)sdata->PausePtrsVRAM[ELIM_BG_SLOT_TEXTURE_DB0];
		RECT pageRects[2] = {{ELIM_BG_TEXTURE_LEFT_X, 0, ELIM_BG_TEXTURE_BACKUP_W, ELIM_BG_TEXTURE_BACKUP_H},
		                     {ELIM_BG_TEXTURE_RIGHT_X, 0, ELIM_BG_TEXTURE_BACKUP_W, ELIM_BG_TEXTURE_BACKUP_H}};

		// load from RAM, back to VRAM
		LoadImage(&pageRects[0], texture0);
		LoadImage(&pageRects[1], (u32 *)sdata->PausePtrsVRAM[ELIM_BG_SLOT_TEXTURE_DB1]);

		DrawSync(0);

		gGT->db[0].primMem.end = sdata->PausePtrsVRAM[ELIM_BG_SLOT_TEXTURE_DB0] + ELIM_BG_TEXTURE_BACKUP_BYTES;
		gGT->db[1].primMem.end = sdata->PausePtrsVRAM[ELIM_BG_SLOT_TEXTURE_DB1] + ELIM_BG_TEXTURE_BACKUP_BYTES;

		// Enable all instances
		ElimBG_ToggleAllInstances(gGT, 0);

		// game is not paused anymore
		sdata->pause_state = ELIM_BG_PAUSE_STATE_NONE;
		// NOTE(aalhendi): Publish the idle state before the exit jump, rather than in its delay slot.
		CTR_PSX_MEMORY_BARRIER();
	}

	// if game is paused, but not on the restore frame
	else if (state != ELIM_BG_PAUSE_STATE_NONE)
	{
		// if this is the first frame of pause
		if (state == ELIM_BG_PAUSE_STATE_CAPTURE)
		{
			gGT->renderFlags = (gGT->renderFlags & RENDER_FLAG_CHECKERED_FLAG) | RENDER_FLAG_RENDER_BUCKET;

			GAME_TRACKER->hudFlags &= HUD_FLAG_PAUSE_SCREENSHOT_MASK;

			ElimBG_SaveScreenshot_Full(gGT);

			// Disable all instances
			// (prevent PrimMem from overwriting VRAM backup)
			ElimBG_ToggleAllInstances(gGT, 1);

			// you are now ready to draw the screenshot
			sdata->pause_state = ELIM_BG_PAUSE_STATE_DRAW;
		}
		// rest of the function is for drawing screenshot
		tileX = 0;
		do
		{
			textureY = 0;
			do
			{
				// backBuffer->primMem.cursor
				p = (POLY_FT4 *)gGT->backBuffer->primMem.cursor;

				// increment primMem by size of primitive
				gGT->backBuffer->primMem.cursor = p + 1;

				setPolyFT4(p);

				screenY = (s16)textureY;

				// RGB
				p->b0 = ELIM_BG_TILE_COLOR;
				p->g0 = ELIM_BG_TILE_COLOR;
				p->r0 = ELIM_BG_TILE_COLOR;

				// four (x,y) positions
				setXY4(p, tileX, screenY, tileX + ELIM_BG_TILE_W, screenY, tileX, screenY + ELIM_BG_TILE_H, tileX + ELIM_BG_TILE_W, screenY + ELIM_BG_TILE_H);

				screenXForTpage = tileX;
				if (tileX < 0)
				{
					screenXForTpage = tileX + 3;
				}
				textureX = (screenXForTpage >> 2) + ELIM_BG_TEXTURE_LEFT_X;
				p->tpage = getTPage(TEXPAGE_COLOR_4BIT, TRANS_50, textureX, textureY);
				tpage = p->tpage;

				// clut
				p->clut = ELIM_BG_TILE_CLUT;

				pageOrigin = (tpage << 6) & 0x3c0;
				textureU = (textureX - pageOrigin) * 4;

				p->v0 = textureY;

				// u0
				p->u0 = textureU;

				if (textureU + ELIM_BG_VRAM_U_WRAP < ELIM_BG_U_LIMIT)
				{
					// u1
					p->u1 = textureU - ELIM_BG_VRAM_U_WRAP;
				}
				else
				{
					// u1
					*(s8 *)&p->u1 = -1;
				}

				// u2
				lowerTextureU = (textureX - pageOrigin) * 4;
				// NOTE(aalhendi): Keep this lower-U calculation before the independent Y stores in retail's packet-write order.
				CTR_PSX_OBSERVE_VALUE(lowerTextureU);
				p->v2 = textureY + ELIM_BG_TILE_H;
				p->v1 = textureY;
				p->u2 = lowerTextureU;

				if (lowerTextureU + ELIM_BG_VRAM_U_WRAP < ELIM_BG_U_LIMIT)
				{
					// u3
					p->u3 = lowerTextureU - ELIM_BG_VRAM_U_WRAP;
				}
				else
				{
					// u3
					*(s8 *)&p->u3 = -1;
				}

				// v3 = v0 + 0x10
				textureY += ELIM_BG_TILE_H;
				p->v3 = (char)textureY;

				// pointer to OT mem, and pointer to primitive
				AddPrim(&GAME_TRACKER->pushBuffer_UI.ptrOT[4], p);

				// while v0 (tex coord Y) < screensize
			} while ((int)textureY < ELIM_BG_SCREEN_H);

			// increment u0
			tileX = tileX + ELIM_BG_TILE_W;

			// while u0 (tex coord X) < screensize
		} while (tileX < ELIM_BG_SCREEN_W);
	}
}


void ElimBG_Deactivate(struct GameTracker *gGT)
{
	u32 renderFlags;
	u32 backupRenderFlags;
	u8 backupHudFlags;

	// if game is paused
	if (sdata->pause_state != ELIM_BG_PAUSE_STATE_NONE)
	{
		renderFlags = gGT->renderFlags & RENDER_FLAG_CHECKERED_FLAG;

		// request the one-frame VRAM restore path
		sdata->pause_state = ELIM_BG_PAUSE_STATE_RESTORE;
		backupRenderFlags = sdata->pause_backup_renderFlags & RENDER_FLAG_ALL_EXCEPT_CHECKERED_FLAG_MASK;
		backupHudFlags = (u8)sdata->pause_backup_hudFlags;

		gGT->renderFlags = renderFlags | backupRenderFlags;

		gGT->hudFlags = backupHudFlags;
	}
}
