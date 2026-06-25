#include <common.h>


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80024524-0x8002459c.
void ElimBG_SaveScreenshot_Chunk(u16 *param_1, u16 *param_2, int param_3)
{
	u16 uVar1;
	u16 *puVar2;

	if (param_3 == 0)
	{
		return;
	}

	puVar2 = param_2 + 3;

	for (; param_3 > 0; param_3 -= 4, param_2 += 4, puVar2 += 4, param_1++)
	{
		uVar1 = (u16)((param_2[0] & 0x3e0) >> 6);
		uVar1 |= puVar2[-2] >> 2 & 0xf0;
		uVar1 |= (u16)((puVar2[-1] & 0x3c0) << 2);
		uVar1 |= (u16)((*puVar2 & 0x3c0) << 6);
		*param_1 = uVar1;
	};
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002459c-0x8002481c.
void ElimBG_SaveScreenshot_Full(struct GameTracker *gGT)
{
	int iVar4;
	u32 local_48[2];
	u32 local_40[2];
	RECT rect1;
	RECT rect2;
	RECT rSrc;
	RECT rDst;

	iVar4 = 0;

	// rdataPauseData
	// TODO: modify this code to just properly assign to the rect's members instead of this jank.
	((u32 *)&rect1)[0] = 0x200;
	((u32 *)&rect1)[1] = 0x1000040;
	((u32 *)&rect2)[0] = 0x240;
	((u32 *)&rect2)[1] = 0x1000040;

	// vram copy, then overwrite vram with pause image

	u32 start1 = (u32)gGT->db[0].primMem.end;
	u32 start2 = (u32)gGT->db[1].primMem.end;
	start1 -= 0xc800;
	start2 -= 0xc800;
	gGT->db[0].primMem.end = (void *)start1;
	gGT->db[1].primMem.end = (void *)start2;

	// 0x800 byte hole
	sdata->PausePtrsVRAM[4] = (char *)start1;
	sdata->PausePtrsVRAM[5] = (char *)start2;

	// 0x4000 byte hole
	sdata->PausePtrsVRAM[2] = (char *)(start1 + 0x800);
	sdata->PausePtrsVRAM[3] = (char *)(start2 + 0x800);

	// 0x8000 byte hole
	sdata->PausePtrsVRAM[0] = (char *)(start1 + 0x4800);
	sdata->PausePtrsVRAM[1] = (char *)(start2 + 0x4800);

	// copy texture vram into PrimMem
	StoreImage(&rect1, (u32 *)sdata->PausePtrsVRAM[0]);
	StoreImage(&rect2, (u32 *)sdata->PausePtrsVRAM[1]);

	// === copy screen into texture vram ===

#define STRIP_H 8

	rSrc.x = 0;
	rSrc.y = gGT->swapchainIndex * 0x128;
	rSrc.w = 0x200;
	rSrc.h = STRIP_H;

	rDst.x = 0x200;
	rDst.w = 0x80;
	rDst.h = STRIP_H;

	// start the first Store
	StoreImage(&rSrc, (u32 *)sdata->PausePtrsVRAM[2]);

	for (rDst.y = 0; rDst.y < (0xd8 - STRIP_H); rDst.y += STRIP_H)
	{
		iVar4 = 1 - iVar4;

		// pause until Store is done
		DrawSync(0);

		// start next Store, while processing previous store
		rSrc.y += STRIP_H;
		StoreImage((RECT *)&rSrc, (u32 *)sdata->PausePtrsVRAM[2 + iVar4]);

		ElimBG_SaveScreenshot_Chunk((u16 *)sdata->PausePtrsVRAM[4 + (1 - iVar4)], (u16 *)sdata->PausePtrsVRAM[2 + (1 - iVar4)], 0x1000);

		LoadImage(&rDst, (u32 *)sdata->PausePtrsVRAM[4 + (1 - iVar4)]);
	}

	// wait for last Store
	DrawSync(0);

	ElimBG_SaveScreenshot_Chunk((u16 *)sdata->PausePtrsVRAM[4 + (iVar4)], (u16 *)sdata->PausePtrsVRAM[2 + (iVar4)], 0x1000);

	LoadImage(&rDst, (u32 *)sdata->PausePtrsVRAM[4 + (iVar4)]);

	rDst.y = 0xff;
	rDst.w = 0x10;
	rDst.h = 1;
	LoadImage(&rDst, (u32 *)&data.pauseScreenStrip[0]);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002481c-0x80024840.
void ElimBG_Activate(struct GameTracker *gGT)
{
	sdata->pause_backup_renderFlags = gGT->renderFlags;
	sdata->pause_backup_hudFlags = gGT->hudFlags;
	sdata->pause_state = 1;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80024840-0x800248bc.
void ElimBG_ToggleInstance(struct Instance *inst, char boolGameIsPaused)
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
	s16 sVar1;
	s16 sVar2;
	char cVar4;
	int iVar5;
	int iVar6;
	POLY_FT4 *p;
	u32 uVar7;
	char cVar8;
	u32 tpage;
	u32 uVar9;
	int iVar10;
	RECT rect1;
	RECT rect2;

	// if this is last frame of pause
	if (sdata->pause_state == 3)
	{
		// rdataPauseData
		// TODO: modify this code to just properly assign to the rect's members instead of this jank.
		((u32 *)&rect1)[0] = 0x200;
		((u32 *)&rect1)[1] = 0x1000040;
		((u32 *)&rect2)[0] = 0x240;
		((u32 *)&rect2)[1] = 0x1000040;

		// load from RAM, back to VRAM
		LoadImage(&rect1, (u32 *)sdata->PausePtrsVRAM[0]);
		LoadImage(&rect2, (u32 *)sdata->PausePtrsVRAM[1]);

		DrawSync(0);

		gGT->db[0].primMem.end = (void *)((int)gGT->db[0].primMem.end + 0xc800);
		gGT->db[1].primMem.end = (void *)((int)gGT->db[1].primMem.end + 0xc800);

		// Enable all instances
		ElimBG_ToggleAllInstances(gGT, 0);

		// game is not paused anymore
		sdata->pause_state = 0;
	}

	// if this is not last frame of paus
	// if game is paused at all
	else if (sdata->pause_state != 0)
	{
		// if this is the first frame of pause
		if (sdata->pause_state == 1)
		{
			gGT->renderFlags = (gGT->renderFlags & 0x1000) | 0x20;

			gGT->hudFlags &= 0xf6;

			ElimBG_SaveScreenshot_Full(gGT);

			// Disable all instances
			// (prevent PrimMem from overwriting VRAM backup)
			ElimBG_ToggleAllInstances(gGT, 1);

			// you are now ready to draw the screenshot
			sdata->pause_state = 2;
		}
		// rest of the function is for drawing screenshot
		iVar10 = 0;
		do
		{
			uVar9 = 0;
			sVar1 = (s16)iVar10;
			do
			{
				// backBuffer->primMem.cursor
				p = (POLY_FT4 *)gGT->backBuffer->primMem.cursor;

				// increment primMem by size of primitive
				gGT->backBuffer->primMem.cursor = p + 1;

				setPolyFT4(p);

				sVar2 = (s16)uVar9;

				// RGB
				setRGB0(p, 0x80, 0x80, 0x80);

				// four (x,y) positions
				setXY4(p, sVar1, sVar2, sVar1 + 0x80, sVar2, sVar1, sVar2 + 0x10, sVar1 + 0x80, sVar2 + 0x10);

				iVar5 = iVar10;
				if (iVar10 < 0)
				{
					iVar5 = iVar10 + 3;
				}
				uVar7 = (iVar5 >> 2) + 0x200;
				tpage = ((uVar9 & 0x100) >> 4) | ((uVar7 & 0x3ff) >> 6) | ((uVar9 & 0x200) << 2);

				// tpage
				p->tpage = (u16)tpage;

				// clut
				p->clut = 0x3fe0;

				iVar6 = (uVar7 - ((tpage << 6) & 0x3c0)) * 4;

				p->v0 = uVar9;
				p->v1 = uVar9;

				// u0
				cVar4 = (char)iVar6;
				p->u0 = cVar4;

				if (iVar6 + 0x80 < 0x100)
				{
					// u1
					p->u1 = cVar4 + -0x80;
				}
				else
				{
					// u1
					p->u1 = 0xff;
				}

				// u2
				iVar5 = (uVar7 - ((tpage << 6) & 0x3c0)) * 4;

				// u2
				cVar4 = (char)iVar5;
				p->u2 = cVar4;

				if (iVar5 + 0x80 < 0x100)
				{
					// u3
					p->u3 = cVar4 + -0x80;
				}
				else
				{
					// u3
					p->u3 = 0xff;
				}

				// v3 = v0 + 0x10
				uVar9 += 0x10;
				p->v2 = (char)uVar9;
				p->v3 = (char)uVar9;

				// pointer to OT mem, and pointer to primitive
				AddPrim(&gGT->pushBuffer_UI.ptrOT[4], p);

				// while v0 (tex coord Y) < screensize
			} while ((int)uVar9 < 0xd8);

			// increment u0
			iVar10 = iVar10 + 0x80;

			// while u0 (tex coord X) < screensize
		} while (iVar10 < 0x200);
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80024c08-0x80024c4c.
void ElimBG_Deactivate(struct GameTracker *gGT)
{
	// it's written this way for bytebudget reasons.
	u8 backup = (u8)sdata->pause_backup_hudFlags;

	// if game is paused
	if (sdata->pause_state != 0)
	{
		// if game is not paused
		sdata->pause_state = 3;

		gGT->renderFlags = (gGT->renderFlags & 0x1000) | (sdata->pause_backup_renderFlags & 0xffffefff);

		gGT->hudFlags = backup;
	}
}
