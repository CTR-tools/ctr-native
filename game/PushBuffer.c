#include <common.h>


void PushBuffer_Init(struct PushBuffer *pb, int id, int total)
{
#define SIZEY_TOP 0x6a
#define SIZEY_1P  0xD8
#define STARTY_2P 0x6e

	*(u8 *)&pb->cameraID = (u8)id;

	pb->fadeFromBlack_currentValue = 0x1000;
	pb->fadeFromBlack_desiredResult = 0x1000;

	pb->fade_step = 0x88;
	pb->matrix_Proj.m[0][0] = 0x1c71;

	pb->matrix_Proj.m[0][1] = 0;
	pb->matrix_Proj.m[0][2] = 0;
	pb->matrix_Proj.m[1][0] = 0;
	pb->matrix_Proj.m[1][1] = 0x1000;
	pb->matrix_Proj.m[1][2] = 0;
	pb->matrix_Proj.m[2][0] = 0;
	pb->matrix_Proj.m[2][1] = 0;
	pb->matrix_Proj.m[2][2] = 0x1000;
	pb->matrix_Proj.t[0] = 0;
	pb->matrix_Proj.t[1] = 0;
	pb->matrix_Proj.t[2] = 0;

	switch (total)
	{
	case 1:
		pb->rect.w = 0x200;
		pb->rect.h = SIZEY_1P;

		pb->distanceToScreen_PREV = 0x100;
		pb->distanceToScreen_CURR = 0x100;

		pb->aspectX = 4;
		pb->rect.x = 0;
		pb->rect.y = 0;
		pb->aspectY = 3;
		return;

	case 2:
		switch (id)
		{
		case 0:
			pb->rect.w = 0x200;
			pb->rect.h = SIZEY_TOP;

			pb->distanceToScreen_PREV = 0x100;
			pb->distanceToScreen_CURR = 0x100;

			pb->aspectX = 8;
			pb->rect.x = 0;
			pb->rect.y = 0;
			pb->aspectY = 3;
			return;

		case 1:
			pb->rect.y = STARTY_2P;
			pb->rect.w = 0x200;
			pb->rect.h = SIZEY_TOP;

			pb->distanceToScreen_PREV = 0x100;
			pb->distanceToScreen_CURR = 0x100;

			pb->aspectX = 8;
			pb->rect.x = 0;
			pb->aspectY = 3;
			return;

		default:
			return;
		}

	case 3:
	case 4:
		switch (id)
		{
		case 0:
			pb->rect.w = 0xfd;
			pb->rect.h = SIZEY_TOP;

			pb->distanceToScreen_PREV = 0x80;
			pb->distanceToScreen_CURR = 0x80;

			pb->aspectX = 4;
			pb->rect.x = 0;
			pb->rect.y = 0;
			pb->aspectY = 3;
			return;

		case 1:
			pb->rect.x = 0x103;
			pb->rect.w = 0xfd;
			pb->rect.h = SIZEY_TOP;

			pb->distanceToScreen_PREV = 0x80;
			pb->distanceToScreen_CURR = 0x80;

			pb->aspectX = 4;
			pb->rect.y = 0;
			pb->aspectY = 3;
			return;

		case 2:
			pb->rect.y = STARTY_2P;
			pb->rect.w = 0xfd;
			pb->rect.h = SIZEY_TOP;

			pb->distanceToScreen_PREV = 0x80;
			pb->distanceToScreen_CURR = 0x80;

			pb->aspectX = 4;
			pb->rect.x = 0;
			pb->aspectY = 3;
			return;

		case 3:
			pb->rect.x = 0x103;
			pb->rect.y = STARTY_2P;
			pb->rect.w = 0xfd;
			pb->rect.h = SIZEY_TOP;

			pb->distanceToScreen_PREV = 0x80;
			pb->distanceToScreen_CURR = 0x80;

			pb->aspectX = 4;
			pb->aspectY = 3;
			return;

		default:
			return;
		}

	default:
		return;
	}
}


void PushBuffer_SetPsyqGeom(struct PushBuffer *pb)
{
	SetGeomOffset(pb->rect.w / 2, pb->rect.h / 2);
	SetGeomScreen(pb->distanceToScreen_PREV);
	return;
}


void PushBuffer_SetDrawEnv_DecalMP(void *ot, struct DB *backBuffer, RECT *viewport, s16 offsetX, s16 offsetY, u8 r0, u8 g0, u8 b0, s32 dtd, s32 isbg)
{
	DRAWENV newDrawEnv;
	struct PrimMem *primMem;
	void *prim;

#ifdef CTR_NATIVE
	// NOTE(aalhendi): Retail receives PS1 RAM OT slots here. Native translates
	// 24-bit OT tokens back to host pointers, so stale DecalMP range metadata
	// must not splice a DR_ENV packet into unrelated current-frame memory.
	if (!CtrGpu_IsCurrentOTRange(backBuffer, ot, ot))
	{
		return;
	}
#endif

	primMem = &backBuffer->primMem;
	newDrawEnv = backBuffer->drawEnv;
	newDrawEnv.clip.x = viewport->x;
	newDrawEnv.clip.y = viewport->y;
	newDrawEnv.clip.w = viewport->w;
	newDrawEnv.clip.h = viewport->h;
	newDrawEnv.ofs[0] = offsetX;
	newDrawEnv.ofs[1] = offsetY;
	newDrawEnv.r0 = r0;
	newDrawEnv.g0 = g0;
	newDrawEnv.b0 = b0;
	newDrawEnv.isbg = isbg;
	newDrawEnv.dtd = dtd;

	prim = NULL;
	if (primMem->cursor <= primMem->guardEnd)
	{
		prim = primMem->cursor;
		primMem->cursor = (u8 *)prim + 0x40;
	}

	if (prim != NULL)
	{
		SetDrawEnv(prim, &newDrawEnv);
		AddPrim(ot, prim);
	}
}


void PushBuffer_SetDrawEnv_Normal(void *ot, struct PushBuffer *pb, struct DB *backBuffer, s16 *copyDrawEnvNULL, int isbg)
{
	DRAWENV newDrawEnv;
	struct PrimMem *primMem;
	void *prim;

	primMem = &backBuffer->primMem;
	newDrawEnv = backBuffer->drawEnv;
	if (copyDrawEnvNULL != NULL)
	{
		newDrawEnv.clip.x = copyDrawEnvNULL[0];
		newDrawEnv.clip.y = copyDrawEnvNULL[1];
		newDrawEnv.clip.w = copyDrawEnvNULL[2];
		newDrawEnv.clip.h = copyDrawEnvNULL[3];
		newDrawEnv.ofs[0] = copyDrawEnvNULL[0];
		newDrawEnv.ofs[1] = copyDrawEnvNULL[1];
	}
	else
	{
		newDrawEnv.clip.x += pb->rect.x;
		newDrawEnv.clip.y += pb->rect.y;
		newDrawEnv.clip.w = pb->rect.w;
		newDrawEnv.clip.h = pb->rect.h;
		newDrawEnv.ofs[0] += pb->rect.x;
		newDrawEnv.ofs[1] += pb->rect.y;
	}
	newDrawEnv.isbg = isbg;

	prim = NULL;
	if (primMem->cursor <= primMem->guardEnd)
	{
		prim = primMem->cursor;
		primMem->cursor = (u8 *)prim + 0x40;
	}

	if (prim != NULL)
	{
		SetDrawEnv(prim, &newDrawEnv);
		AddPrim(ot, prim);
	}
}

#include "PushBuffer/PushBuffer_SetMatrixVP.c"


#include "PushBuffer/PushBuffer_SetFrustumPlane.c"


#include "PushBuffer/PushBuffer_UpdateFrustum.c"


void PushBuffer_FadeOneWindow(struct PushBuffer *pb)
{
	typedef struct
	{
		u32 tag;
		u32 tpage;
		POLY_F4 f4;
	} multiCmdPacket;

	int fadeStrength;
	multiCmdPacket *p = NULL;

	if (pb->fadeFromBlack_currentValue != 0x1000)
	{
		if (pb->fadeFromBlack_currentValue > 0x1000)
		{
			p = (multiCmdPacket *)GAME_TRACKER->backBuffer->primMem.cursor;

			setlen(p, 7);
			p->f4.code = 0x2a;
			p->tpage = 0xe1000a20;
			p->f4.tag = 0;
			fadeStrength = pb->fadeFromBlack_currentValue - 0x1000;
			p->f4.x0 = 0;
			p->f4.y0 = 0;
		}
		else
		{
			p = (multiCmdPacket *)GAME_TRACKER->backBuffer->primMem.cursor;

			setlen(p, 7);
			p->f4.code = 0x2a;
			p->tpage = 0xe1000a40;
			p->f4.tag = 0;
			fadeStrength = 0xfff - pb->fadeFromBlack_currentValue;
			p->f4.x0 = 0;
			p->f4.y0 = 0;
		}

#ifdef CTR_NATIVE
		// NOTE(aalhendi): Native PsyCross needs dfe=1 for this full-window fade.
		p->tpage |= 0x400; // set dfe=1
#endif

		// strength of fade
		fadeStrength = fadeStrength >> 4;

		p->f4.r0 = p->f4.g0 = p->f4.b0 = (u8)fadeStrength;
		p->f4.x1 = pb->rect.w;
		p->f4.y1 = 0;
		p->f4.x2 = 0;
		p->f4.y2 = pb->rect.h;
		p->f4.x3 = pb->rect.w;
		p->f4.y3 = pb->rect.h;
		AddPrim(pb->ptrOT, p);

		// move pointer after writing polygons
		{
			struct DB *backBuffer = GAME_TRACKER->backBuffer;
			backBuffer->primMem.cursor = (u8 *)backBuffer->primMem.cursor + sizeof(multiCmdPacket);
		}
	}

	// NOTE(aalhendi): Narrow the updated fade state before clamping its field.
	pb->fadeFromBlack_currentValue += pb->fade_step;

	if (pb->fade_step > 0)
	{
		if (pb->fadeFromBlack_desiredResult < pb->fadeFromBlack_currentValue)
		{
			pb->fadeFromBlack_currentValue = pb->fadeFromBlack_desiredResult;
		}
	}
	else
	{
		if (pb->fadeFromBlack_currentValue < pb->fadeFromBlack_desiredResult)
		{
			pb->fadeFromBlack_currentValue = pb->fadeFromBlack_desiredResult;
		}
	}
}


void PushBuffer_FadeAllWindows()
{
	int i;

	for (i = 0; i < GAME_TRACKER->numPlyrCurrGame; i++)
	{
		PushBuffer_FadeOneWindow(&GAME_TRACKER->pushBuffer[i]);
	}

	PushBuffer_FadeOneWindow(&GAME_TRACKER->pushBuffer_UI);
}
