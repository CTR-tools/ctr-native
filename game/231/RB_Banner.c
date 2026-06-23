#include <common.h>

union RBBannerScratchVertex
{
	struct
	{
		s16 x;
		s16 z;
		s16 y;
		s16 residue;
	};
	struct
	{
		u32 xz;
		u32 yResidue;
	};
};

enum
{
	RB_BANNER_TEMP_VERTEX_SCRATCH_OFFSET = 0x300,
};

CTR_STATIC_ASSERT_LAYOUT(sizeof(union RBBannerScratchVertex) == 0x8);
CTR_STATIC_ASSERT_LAYOUT(offsetof(union RBBannerScratchVertex, x) == 0x0);
CTR_STATIC_ASSERT_LAYOUT(offsetof(union RBBannerScratchVertex, z) == 0x2);
CTR_STATIC_ASSERT_LAYOUT(offsetof(union RBBannerScratchVertex, y) == 0x4);
CTR_STATIC_ASSERT_LAYOUT(offsetof(union RBBannerScratchVertex, residue) == 0x6);
CTR_STATIC_ASSERT_LAYOUT(offsetof(union RBBannerScratchVertex, xz) == 0x0);
CTR_STATIC_ASSERT_LAYOUT(offsetof(union RBBannerScratchVertex, yResidue) == 0x4);

static inline u8 *RB_Banner_FirstVertex(struct ModelHeader *mh)
{
	return (u8 *)mh->ptrFrameData + mh->ptrFrameData->vertexOffset;
}

static inline union RBBannerScratchVertex *RB_Banner_VertexSlot(u32 scratchOffset)
{
	return CTR_SCRATCHPAD_PTR(union RBBannerScratchVertex, scratchOffset);
}

static inline union RBBannerScratchVertex *RB_Banner_TempVertex(void)
{
	return RB_Banner_VertexSlot(RB_BANNER_TEMP_VERTEX_SCRATCH_OFFSET);
}

static inline void RB_Banner_SaveVertex(u8 stackIndex, const u8 *vertex)
{
	union RBBannerScratchVertex *temp = RB_Banner_TempVertex();
	union RBBannerScratchVertex *slot = RB_Banner_VertexSlot((u32)stackIndex * sizeof(*slot));

	// NOTE(aalhendi): Retail stages X/Z/Y at scratch 0x300, then copies both
	// words into the slot. The upper halfword at 0x306 is scratch residue.
	temp->x = vertex[0];
	temp->z = vertex[2];
	temp->y = vertex[1];
	slot->xz = temp->xz;
	slot->yResidue = temp->yResidue;
}

static inline int RB_Banner_LoadSavedX(u32 command, u8 stackIndex)
{
	union RBBannerScratchVertex *temp = RB_Banner_TempVertex();
	union RBBannerScratchVertex *saved = RB_Banner_VertexSlot((command >> 13) & 0x7f8);
	union RBBannerScratchVertex *slot = RB_Banner_VertexSlot((u32)stackIndex * sizeof(*slot));

	temp->xz = saved->xz;
	temp->yResidue = slot->yResidue;

	return (s32)((u32)(u16)temp->x << 16) >> 18;
}

static inline u32 RB_Banner_RewriteCommandX(u32 command, int xQuarter, int reusedVertex)
{
	u32 mask = reusedVertex ? 0xf7ff01ffU : 0xffff01ffU;

	return (command & mask) | ((u32)xQuarter << 9);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b53e0-0x800b56c4.
int RB_Banner_Animate_Init(struct ModelHeader *mh)
{
	u32 *cmd;
	u8 *vertex;
	int count = 0;

	if ((s16)(*(u16 *)(void *)mh->ptrCommandList) < 0x40)
		return 0;

	vertex = RB_Banner_FirstVertex(mh);
	cmd = (u32 *)((u8 *)mh->ptrCommandList + 4);

	while (*cmd != 0xffffffffU)
	{
		u32 command = *cmd;

		if ((command & 0xffff0000U) == 0)
		{
			cmd++;
			continue;
		}

		if ((s32)command >= 0)
		{
			u8 stackIndex = ((u8 *)cmd)[2];
			int xQuarter;

			if ((command & 0x04000000U) == 0)
			{
				RB_Banner_SaveVertex(stackIndex, vertex);
				xQuarter = vertex[0] >> 2;
				vertex += 3;
				count++;
				*cmd = RB_Banner_RewriteCommandX(command, xQuarter, 0);
			}
			else
			{
				xQuarter = RB_Banner_LoadSavedX(command, stackIndex);
				*cmd = RB_Banner_RewriteCommandX(command, xQuarter, 1);
			}

			cmd++;
			continue;
		}

		for (int i = 0; i < 3; i++, cmd++)
		{
			command = *cmd;
			u8 stackIndex = ((u8 *)cmd)[2];
			int xQuarter;

			if ((command & 0x04000000U) == 0)
			{
				RB_Banner_SaveVertex(stackIndex, vertex);
				xQuarter = vertex[0] >> 2;
				vertex += 3;
				count++;
				*cmd = RB_Banner_RewriteCommandX(command, xQuarter, 0);
			}
			else
			{
				xQuarter = RB_Banner_LoadSavedX(command, stackIndex);
				*cmd = RB_Banner_RewriteCommandX(command, xQuarter, 1);
			}
		}
	}

	if (sdata->gGT->numPlyrCurrGame >= 4)
	{
		vertex = RB_Banner_FirstVertex(mh);
		for (int i = 0; i < count; i++, vertex += 3)
			vertex[1] = 0x80;
	}

	return count;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b56c4-0x800b57b4.
void RB_Banner_Animate_Play(struct ModelHeader *mh, s16 numVertices)
{
	u32 *colors = mh->ptrColors;
	u32 firstColor = colors[0];
	u8 *vertex;

	for (int i = 0; i < 0x3f; i++)
		colors[i] = colors[i + 1];
	colors[0x3f] = firstColor;

	if (numVertices <= 0)
		return;

	vertex = (u8 *)mh->ptrFrameData + mh->ptrFrameData->vertexOffset;
	for (int i = 0; i < numVertices; i++, vertex += 3)
	{
		u8 x = vertex[0];
		u8 color = ((u8 *)mh->ptrColors)[(((x >> 2) + 10) & 0x3f) * 4];
		int wave = (int)color - 0x80;

		if (x < 0x40)
			wave = (wave * ((int)x << 2)) >> 8;
		else if (x > 0xc0)
			wave = (wave * ((0x100 - (int)x) << 2)) >> 8;

		vertex[1] = (u8)(wave + 0x80);
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b57b4-0x800b57f8.
void RB_Banner_ThTick(struct Thread *t)
{
	struct StartBanner *banner = t->object;

	if (banner->numVertices != 0)
		RB_Banner_Animate_Play(t->inst->model->headers, banner->numVertices);
}

static char s_startbanner[] = "startbanner";

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b57f8-0x800b5968.
void RB_Banner_LInB(struct Instance *inst)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Thread *t;
	struct StartBanner *banner;
	struct Model *model;

	if (inst->thread != NULL)
		return;

	t = PROC_BirthWithObject(SIZE_RELATIVE_POOL_BUCKET(sizeof(struct StartBanner), NONE, SMALL, STATIC), RB_Banner_ThTick, s_startbanner, NULL);
	inst->thread = t;
	if (t == NULL)
		return;

	if (gGT->numPlyrCurrGame >= 4)
		t->funcThTick = NULL;

	banner = t->object;
	t->inst = inst;
	banner->unused = 0;
	banner->numVertices = 0;

	model = gGT->modelPtr[STATIC_STARTBANNERWAVE];
	if (model == NULL)
		return;

	inst->model = model;
	banner->numVertices = RB_Banner_Animate_Init(model->headers);
	if (banner->numVertices == 0)
		return;

	for (int i = 0; i < 0x40; i++)
	{
		u8 *color = (u8 *)&model->headers->ptrColors[i];
		int value = (MATH_Sin((u32)i << 7) >> 6) + 0x80;

		if (gGT->numPlyrCurrGame >= 4)
			value = 0x80;

		color[0] = (u8)value;
		color[1] = (u8)value;
		color[2] = (u8)value;
	}
}
