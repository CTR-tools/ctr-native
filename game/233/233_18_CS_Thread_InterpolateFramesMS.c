#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800ae318-0x800ae54c
void CS_Thread_InterpolateFramesMS(struct Thread *t)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Instance *inst = t->inst;
	struct PrimMem *primMem;
	u32 *prim;
	u32 *end;
	u16 curr[3];
	u16 next[3];
	int depth;

	CS_Instance_GetFrameData(inst, inst->animIndex, inst->animFrame, curr, NULL, 0);
	CS_Instance_GetFrameData(inst, inst->animIndex, inst->animFrame, next, NULL, 1);

	curr[0] = (u16)(curr[0] + (u16)inst->matrix.t[0]);
	curr[1] = (u16)(curr[1] + (u16)inst->matrix.t[1]);
	curr[2] = (u16)(curr[2] + (u16)inst->matrix.t[2]);

	next[0] = (u16)(next[0] + (u16)inst->matrix.t[0]);
	next[1] = (u16)(next[1] + (u16)inst->matrix.t[1]);
	next[2] = (u16)(next[2] + (u16)inst->matrix.t[2]);

	primMem = &gGT->backBuffer->primMem;
	prim = (u32 *)primMem->curr;
	end = (u32 *)primMem->endMin100;

	if ((uintptr_t)(prim + 6) >= (uintptr_t)end)
		return;

	gte_SetRotMatrix(&gGT->pushBuffer[0].matrix_ViewProj);
	gte_SetTransMatrix(&gGT->pushBuffer[0].matrix_ViewProj);

	MTC2((u32)curr[0] | ((u32)curr[1] << 16), 0);
	MTC2((u32)curr[2], 1);
	MTC2((u32)next[0] | ((u32)next[1] << 16), 2);
	MTC2((u32)next[2], 3);
	gte_rtpt();

	prim[4] = MFC2(12);
	prim[5] = MFC2(13);

	depth = MFC2(17);
	if ((u32)(depth - 1) < 0x11ff)
	{
		u32 color = 0x3f;
		int otIndex;
		u32 *ot;

		prim[1] = 0xe1000a20;
		prim[2] = 0;

		if (depth > 0xa00)
		{
			int fade = (0x1200 - depth) * 0x3f;

			color = fade >> 11;
			if (fade < 0)
				color = (fade + 0x7ff) >> 11;
		}

		prim[3] = color | (color << 8) | (color << 16) | 0x42000000;

		otIndex = depth >> 6;
		if (otIndex > 0x3ff)
			otIndex = 0x3ff;

		ot = (u32 *)&gGT->pushBuffer[0].ptrOT[otIndex];
		prim[0] = (*ot & 0xffffff) | 0x05000000;
		*ot = CtrGpu_PrimToOTLink24(prim);
		prim += 6;
	}

	primMem->curr = prim;
}
