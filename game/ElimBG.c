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

CTR_STATIC_ASSERT(ELIM_BG_PRIMMEM_PAUSE_BYTES == 0xc800);
CTR_STATIC_ASSERT(ELIM_BG_RAW_STRIP_OFFSET == 0x800);
CTR_STATIC_ASSERT(ELIM_BG_TEXTURE_BACKUP_OFFSET == 0x4800);
CTR_STATIC_ASSERT(ELIM_BG_CHUNK_SOURCE_PIXELS == 0x1000);

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80024524-0x8002459c.
void ElimBG_SaveScreenshot_Chunk(u16 *packedStrip, u16 *rawStrip, int rawPixelCount)
{
	u16 packedPixel;
	u16 *rawGroupLast;

	if (rawPixelCount == 0)
	{
		return;
	}

	rawGroupLast = rawStrip + 3;

	for (; rawPixelCount > 0; rawPixelCount -= 4, rawStrip += 4, rawGroupLast += 4, packedStrip++)
	{
		packedPixel = (u16)((rawStrip[0] & 0x3e0) >> 6);
		packedPixel |= rawGroupLast[-2] >> 2 & 0xf0;
		packedPixel |= (u16)((rawGroupLast[-1] & 0x3c0) << 2);
		packedPixel |= (u16)((*rawGroupLast & 0x3c0) << 6);
		*packedStrip = packedPixel;
	};
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002459c-0x8002481c.
void ElimBG_SaveScreenshot_Full(struct GameTracker *gGT)
{
	int bufferIndex;
	RECT rect1;
	RECT rect2;
	RECT rSrc;
	RECT rDst;

	bufferIndex = 0;

	rect1.x = ELIM_BG_TEXTURE_LEFT_X;
	rect1.y = 0;
	rect1.w = ELIM_BG_TEXTURE_BACKUP_W;
	rect1.h = ELIM_BG_TEXTURE_BACKUP_H;
	rect2.x = ELIM_BG_TEXTURE_RIGHT_X;
	rect2.y = 0;
	rect2.w = ELIM_BG_TEXTURE_BACKUP_W;
	rect2.h = ELIM_BG_TEXTURE_BACKUP_H;

	// vram copy, then overwrite vram with pause image

	u32 start1 = (u32)gGT->db[0].primMem.end;
	u32 start2 = (u32)gGT->db[1].primMem.end;
	start1 -= ELIM_BG_PRIMMEM_PAUSE_BYTES;
	start2 -= ELIM_BG_PRIMMEM_PAUSE_BYTES;
	gGT->db[0].primMem.end = (void *)start1;
	gGT->db[1].primMem.end = (void *)start2;

	// double-buffered packed 4bpp pause strips
	sdata->PausePtrsVRAM[ELIM_BG_SLOT_PACKED_STRIP_DB0] = (char *)start1;
	sdata->PausePtrsVRAM[ELIM_BG_SLOT_PACKED_STRIP_DB1] = (char *)start2;

	// double-buffered raw screenshot strips from VRAM
	sdata->PausePtrsVRAM[ELIM_BG_SLOT_RAW_STRIP_DB0] = (char *)(start1 + ELIM_BG_RAW_STRIP_OFFSET);
	sdata->PausePtrsVRAM[ELIM_BG_SLOT_RAW_STRIP_DB1] = (char *)(start2 + ELIM_BG_RAW_STRIP_OFFSET);

	// backups of the two VRAM pages overwritten by the pause image
	sdata->PausePtrsVRAM[ELIM_BG_SLOT_TEXTURE_DB0] = (char *)(start1 + ELIM_BG_TEXTURE_BACKUP_OFFSET);
	sdata->PausePtrsVRAM[ELIM_BG_SLOT_TEXTURE_DB1] = (char *)(start2 + ELIM_BG_TEXTURE_BACKUP_OFFSET);

	// copy texture vram into PrimMem
	StoreImage(&rect1, (u32 *)sdata->PausePtrsVRAM[ELIM_BG_SLOT_TEXTURE_DB0]);
	StoreImage(&rect2, (u32 *)sdata->PausePtrsVRAM[ELIM_BG_SLOT_TEXTURE_DB1]);

	// === copy screen into texture vram ===

	rSrc.x = 0;
	rSrc.y = gGT->swapchainIndex * ELIM_BG_SWAPCHAIN_Y_STRIDE;
	rSrc.w = ELIM_BG_SCREEN_W;
	rSrc.h = ELIM_BG_STRIP_H;

	rDst.x = ELIM_BG_CAPTURE_VRAM_X;
	rDst.w = ELIM_BG_CAPTURE_VRAM_W;
	rDst.h = ELIM_BG_STRIP_H;

	// start the first Store
	StoreImage(&rSrc, (u32 *)sdata->PausePtrsVRAM[ELIM_BG_SLOT_RAW_STRIP_DB0]);

	for (rDst.y = 0; rDst.y < (ELIM_BG_SCREEN_H - ELIM_BG_STRIP_H); rDst.y += ELIM_BG_STRIP_H)
	{
		bufferIndex = 1 - bufferIndex;

		// pause until Store is done
		DrawSync(0);

		// start next Store, while processing previous store
		rSrc.y += ELIM_BG_STRIP_H;
		StoreImage((RECT *)&rSrc, (u32 *)sdata->PausePtrsVRAM[ELIM_BG_SLOT_RAW_STRIP_DB0 + bufferIndex]);

		ElimBG_SaveScreenshot_Chunk((u16 *)sdata->PausePtrsVRAM[ELIM_BG_SLOT_PACKED_STRIP_DB0 + (1 - bufferIndex)],
		                            (u16 *)sdata->PausePtrsVRAM[ELIM_BG_SLOT_RAW_STRIP_DB0 + (1 - bufferIndex)], ELIM_BG_CHUNK_SOURCE_PIXELS);

		LoadImage(&rDst, (u32 *)sdata->PausePtrsVRAM[ELIM_BG_SLOT_PACKED_STRIP_DB0 + (1 - bufferIndex)]);
	}

	// wait for last Store
	DrawSync(0);

	ElimBG_SaveScreenshot_Chunk((u16 *)sdata->PausePtrsVRAM[ELIM_BG_SLOT_PACKED_STRIP_DB0 + bufferIndex],
	                            (u16 *)sdata->PausePtrsVRAM[ELIM_BG_SLOT_RAW_STRIP_DB0 + bufferIndex], ELIM_BG_CHUNK_SOURCE_PIXELS);

	LoadImage(&rDst, (u32 *)sdata->PausePtrsVRAM[ELIM_BG_SLOT_PACKED_STRIP_DB0 + bufferIndex]);

	rDst.y = ELIM_BG_FINAL_STRIP_Y;
	rDst.w = ELIM_BG_FINAL_STRIP_W;
	rDst.h = 1;
	LoadImage(&rDst, &data.pauseScreenStrip[0]);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002481c-0x80024840.
void ElimBG_Activate(struct GameTracker *gGT)
{
	sdata->pause_backup_renderFlags = gGT->renderFlags;
	sdata->pause_backup_hudFlags = gGT->hudFlags;
	sdata->pause_state = ELIM_BG_PAUSE_STATE_CAPTURE;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80024840-0x800248bc.
void ElimBG_ToggleInstance(struct Instance *inst, b32 boolGameIsPaused)
{
	u32 flags;

	// if game is being paused
	if (boolGameIsPaused)
	{
		flags = inst->flags;

		if (!(flags & HIDE_MODEL))
		{
			flags &= ~INVISIBLE_BEFORE_PAUSE;
		}
		else
		{
			flags |= INVISIBLE_BEFORE_PAUSE;
		}

		inst->flags = flags;
		inst->flags |= (INVISIBLE_DURING_PAUSE | HIDE_MODEL);

		return;
	}

	if ((inst->flags & (INVISIBLE_BEFORE_PAUSE | INVISIBLE_DURING_PAUSE)) == INVISIBLE_DURING_PAUSE)
	{
		inst->flags &= ~(INVISIBLE_DURING_PAUSE | HIDE_MODEL);
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800248bc-0x80024974.
void ElimBG_ToggleAllInstances(struct GameTracker *gGT, b32 boolGameIsPaused)
{
	struct Level *lev;
	struct Instance *inst;
	struct InstDef *ptrInstDefs;

	lev = gGT->level1;

	// Loop through all instances in level
	for (ptrInstDefs = &lev->ptrInstDefs[0]; ptrInstDefs < &lev->ptrInstDefs[lev->numInstances]; ptrInstDefs++)
	{
		inst = ptrInstDefs->ptrInstance;

		if (inst != 0)
		{
			ElimBG_ToggleInstance(inst, boolGameIsPaused);
		}
	}

	// Loop through all instances in Instance Pool
	for (inst = (struct Instance *)gGT->JitPools.instance.taken.first; inst != 0; inst = inst->next)
	{
		ElimBG_ToggleInstance(inst, boolGameIsPaused);
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80024974-0x80024c08.
void ElimBG_HandleState(struct GameTracker *gGT)
{
	s16 screenX;
	s16 screenY;
	char textureUByte;
	int screenXForTpage;
	int textureU;
	POLY_FT4 *p;
	u32 textureX;
	u32 tpage;
	u32 textureY;
	int tileX;
	RECT rect1;
	RECT rect2;

	// if this is last frame of pause
	if (sdata->pause_state == ELIM_BG_PAUSE_STATE_RESTORE)
	{
		rect1.x = ELIM_BG_TEXTURE_LEFT_X;
		rect1.y = 0;
		rect1.w = ELIM_BG_TEXTURE_BACKUP_W;
		rect1.h = ELIM_BG_TEXTURE_BACKUP_H;
		rect2.x = ELIM_BG_TEXTURE_RIGHT_X;
		rect2.y = 0;
		rect2.w = ELIM_BG_TEXTURE_BACKUP_W;
		rect2.h = ELIM_BG_TEXTURE_BACKUP_H;

		// load from RAM, back to VRAM
		LoadImage(&rect1, (u32 *)sdata->PausePtrsVRAM[ELIM_BG_SLOT_TEXTURE_DB0]);
		LoadImage(&rect2, (u32 *)sdata->PausePtrsVRAM[ELIM_BG_SLOT_TEXTURE_DB1]);

		DrawSync(0);

		gGT->db[0].primMem.end = (void *)((int)gGT->db[0].primMem.end + ELIM_BG_PRIMMEM_PAUSE_BYTES);
		gGT->db[1].primMem.end = (void *)((int)gGT->db[1].primMem.end + ELIM_BG_PRIMMEM_PAUSE_BYTES);

		// Enable all instances
		ElimBG_ToggleAllInstances(gGT, 0);

		// game is not paused anymore
		sdata->pause_state = ELIM_BG_PAUSE_STATE_NONE;
	}

	// if game is paused, but not on the restore frame
	else if (sdata->pause_state != ELIM_BG_PAUSE_STATE_NONE)
	{
		// if this is the first frame of pause
		if (sdata->pause_state == ELIM_BG_PAUSE_STATE_CAPTURE)
		{
			gGT->renderFlags = (gGT->renderFlags & RENDER_FLAG_CHECKERED_FLAG) | RENDER_FLAG_RENDER_BUCKET;

			gGT->hudFlags &= HUD_FLAG_PAUSE_SCREENSHOT_MASK;

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
			screenX = (s16)tileX;
			do
			{
				// backBuffer->primMem.cursor
				p = (POLY_FT4 *)gGT->backBuffer->primMem.cursor;

				// increment primMem by size of primitive
				gGT->backBuffer->primMem.cursor = p + 1;

				setPolyFT4(p);

				screenY = (s16)textureY;

				// RGB
				setRGB0(p, ELIM_BG_TILE_COLOR, ELIM_BG_TILE_COLOR, ELIM_BG_TILE_COLOR);

				// four (x,y) positions
				setXY4(p, screenX, screenY, screenX + ELIM_BG_TILE_W, screenY, screenX, screenY + ELIM_BG_TILE_H, screenX + ELIM_BG_TILE_W,
				       screenY + ELIM_BG_TILE_H);

				screenXForTpage = tileX;
				if (tileX < 0)
				{
					screenXForTpage = tileX + 3;
				}
				textureX = (screenXForTpage >> 2) + ELIM_BG_TEXTURE_LEFT_X;
				tpage = getTPage(TEXPAGE_COLOR_4BIT, TRANS_50, (u32)textureX, (u32)textureY);

				// tpage
				p->tpage = (u16)tpage;

				// clut
				p->clut = ELIM_BG_TILE_CLUT;

				textureU = (textureX - ((tpage << 6) & 0x3c0)) * 4;

				p->v0 = textureY;
				p->v1 = textureY;

				// u0
				textureUByte = (char)textureU;
				p->u0 = textureUByte;

				if (textureU + ELIM_BG_VRAM_U_WRAP < ELIM_BG_U_LIMIT)
				{
					// u1
					p->u1 = textureUByte + -ELIM_BG_VRAM_U_WRAP;
				}
				else
				{
					// u1
					p->u1 = 0xff;
				}

				// u2
				textureU = (textureX - ((tpage << 6) & 0x3c0)) * 4;

				// u2
				textureUByte = (char)textureU;
				p->u2 = textureUByte;

				if (textureU + ELIM_BG_VRAM_U_WRAP < ELIM_BG_U_LIMIT)
				{
					// u3
					p->u3 = textureUByte + -ELIM_BG_VRAM_U_WRAP;
				}
				else
				{
					// u3
					p->u3 = 0xff;
				}

				// v3 = v0 + 0x10
				textureY += ELIM_BG_TILE_H;
				p->v2 = (char)textureY;
				p->v3 = (char)textureY;

				// pointer to OT mem, and pointer to primitive
				AddPrim(&gGT->pushBuffer_UI.ptrOT[4], p);

				// while v0 (tex coord Y) < screensize
			} while ((int)textureY < ELIM_BG_SCREEN_H);

			// increment u0
			tileX = tileX + ELIM_BG_TILE_W;

			// while u0 (tex coord X) < screensize
		} while (tileX < ELIM_BG_SCREEN_W);
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80024c08-0x80024c4c.
void ElimBG_Deactivate(struct GameTracker *gGT)
{
	// it's written this way for bytebudget reasons.
	u8 backup = (u8)sdata->pause_backup_hudFlags;

	// if game is paused
	if (sdata->pause_state != ELIM_BG_PAUSE_STATE_NONE)
	{
		// request the one-frame VRAM restore path
		sdata->pause_state = ELIM_BG_PAUSE_STATE_RESTORE;

		gGT->renderFlags = (gGT->renderFlags & RENDER_FLAG_CHECKERED_FLAG) | (sdata->pause_backup_renderFlags & RENDER_FLAG_ALL_EXCEPT_CHECKERED_FLAG_MASK);

		gGT->hudFlags = backup;
	}
}
