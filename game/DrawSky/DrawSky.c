#include <common.h>

struct DrawSkyContext
{
	const struct ShortVertex *verts;
	uint32_t *ot;
	u32 screenBounds;
};

struct DrawSkyScratch
{
	u32 baseFaceOffset;
	u32 baseCountOffset;
};

CTR_STATIC_ASSERT(sizeof(struct DrawSkyScratch) == 0x8);
CTR_STATIC_ASSERT(offsetof(struct DrawSkyScratch, baseFaceOffset) == 0x0);
CTR_STATIC_ASSERT(offsetof(struct DrawSkyScratch, baseCountOffset) == 0x4);

static u32 DrawSky_ReadWord(const void *base, int offset)
{
	return *(const u32 *)(const void *)((const char *)base + offset);
}

static int DrawSky_IsVisible(u32 gteFlag, u32 sxy0, u32 sxy1, u32 sxy2, u32 screenBounds)
{
	u32 overlap;
	u32 bounds;

	if (((gteFlag << 13) >> 29) != 0)
	{
		return 0;
	}

	overlap = sxy0 & sxy1 & sxy2;
	bounds = ~((sxy0 - screenBounds) | (sxy1 - screenBounds) | (sxy2 - screenBounds)) | overlap;
	if ((s32)bounds < 0)
	{
		return 0;
	}

	return (s32)(bounds << 16) >= 0;
}

static void DrawSky_LoadFaceVertices(struct DrawSkyContext *ctx, const struct SkyboxFace *face)
{
	const char *verts = (const char *)ctx->verts;
	const struct ShortVertex *a = (const struct ShortVertex *)(const void *)(verts + face->A);
	const struct ShortVertex *b = (const struct ShortVertex *)(const void *)(verts + face->B);
	const struct ShortVertex *c = (const struct ShortVertex *)(const void *)(verts + face->C);

	MTC2(DrawSky_ReadWord(&a->Position, 0x0), 0);
	MTC2(DrawSky_ReadWord(&a->Position, 0x4), 1);
	MTC2(DrawSky_ReadWord(&a->Color, 0x0), 20);

	MTC2(DrawSky_ReadWord(&b->Position, 0x0), 2);
	MTC2(DrawSky_ReadWord(&b->Position, 0x4), 3);
	MTC2(DrawSky_ReadWord(&b->Color, 0x0), 21);

	MTC2(DrawSky_ReadWord(&c->Position, 0x0), 4);
	MTC2(DrawSky_ReadWord(&c->Position, 0x4), 5);
	MTC2(DrawSky_ReadWord(&c->Color, 0x0), 22);
}

static void DrawSky_EmitPrimitive(u32 **primCursor, uint32_t *ot)
{
	POLY_G3 *poly = (POLY_G3 *)*primCursor;

	CtrGpu_WriteColorCode(&poly->r0, MFC2(20));
	CtrGpu_WritePackedXY(&poly->x0, MFC2(12));
	CtrGpu_WriteColorCode(&poly->r1, MFC2(21));
	CtrGpu_WritePackedXY(&poly->x1, MFC2(13));
	CtrGpu_WriteColorCode(&poly->r2, MFC2(22));
	CtrGpu_WritePackedXY(&poly->x2, MFC2(14));
	CtrGpu_LinkPacket24(ot, &poly->tag, poly, 0x06000000);

	*primCursor = (u32 *)(poly + 1);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80069cc4-0x80069e70
static u32 *DrawSky_Piece(struct Skybox *skybox, struct DrawSkyContext *ctx, int faceIndex, int countIndex, u32 *prim)
{
	u32 numFaces = (u16)skybox->numFaces[countIndex];
	const struct SkyboxFace *face = skybox->ptrFaces[faceIndex];

	if (numFaces == 0)
	{
		return prim;
	}

	for (u32 i = 0; i < numFaces; i++, face++)
	{
		uint32_t *ot = (uint32_t *)(void *)((char *)ctx->ot + (s16)face->D);
		u32 sxy0;
		u32 sxy1;
		u32 sxy2;
		u32 gteFlag;

		DrawSky_LoadFaceVertices(ctx, face);
		gte_rtpt_b();

		sxy0 = MFC2(12);
		gteFlag = CFC2(31);
		sxy1 = MFC2(13);
		sxy2 = MFC2(14);

		if (DrawSky_IsVisible(gteFlag, sxy0, sxy1, sxy2, ctx->screenBounds))
		{
			DrawSky_EmitPrimitive(&prim, ot);
		}
	}

	return prim;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80069bb0-0x80069cc4
void DrawSky_Full(void *skybox, struct PushBuffer *pb, struct PrimMem *primMem)
{
	struct Skybox *sky = skybox;
	u32 *prim = (u32 *)primMem->cursor;

	// NOTE(aalhendi): PSX-backfeed blocker: retail saves/restores ra and s0-s2 in scratchpad 0x00-0x0c.
	// Native C relies on the host ABI because the only retail data temporaries live in 0x10 and 0x14 and are explicit below.
	if (sky != NULL)
	{
		struct DrawSkyScratch *scratch = CTR_SCRATCHPAD_PTR(struct DrawSkyScratch, 0x10);
		struct DrawSkyContext ctx;
		u32 baseFaceOffset;
		u32 baseCountOffset;
		int faceIndex;
		int countIndex;

		CTC2(DrawSky_ReadWord(&pb->matrix_ViewProj, 0x00), 0);
		CTC2(DrawSky_ReadWord(&pb->matrix_ViewProj, 0x04), 1);
		CTC2(DrawSky_ReadWord(&pb->matrix_ViewProj, 0x08), 2);
		CTC2(DrawSky_ReadWord(&pb->matrix_ViewProj, 0x0c), 3);
		CTC2(DrawSky_ReadWord(&pb->matrix_ViewProj, 0x10), 4);
		CTC2(0, 5);
		CTC2(0, 6);
		CTC2(0, 7);

		baseFaceOffset = ((DrawSky_ReadWord(pb, 0x08) + 0x500) >> 7) & 0x1c;
		baseCountOffset = (baseFaceOffset >> 1) & 0xe;

		scratch->baseFaceOffset = baseFaceOffset;
		scratch->baseCountOffset = baseCountOffset;

		ctx.verts = sky->ptrVertex;
		ctx.ot = &pb->ptrOT[0x3ff];
		ctx.screenBounds = DrawSky_ReadWord(pb, 0x20);

		faceIndex = (int)(baseFaceOffset >> 2);
		countIndex = (int)(baseCountOffset >> 1);
		prim = DrawSky_Piece(sky, &ctx, faceIndex, countIndex, prim);

		baseFaceOffset = (baseFaceOffset + 4) & 0x1c;
		baseCountOffset = (baseCountOffset + 2) & 0xe;
		faceIndex = (int)(baseFaceOffset >> 2);
		countIndex = (int)(baseCountOffset >> 1);
		prim = DrawSky_Piece(sky, &ctx, faceIndex, countIndex, prim);

		baseFaceOffset = (scratch->baseFaceOffset - 4) & 0x1c;
		baseCountOffset = (scratch->baseCountOffset - 2) & 0xe;
		faceIndex = (int)(baseFaceOffset >> 2);
		countIndex = (int)(baseCountOffset >> 1);
		prim = DrawSky_Piece(sky, &ctx, faceIndex, countIndex, prim);

		baseFaceOffset = (scratch->baseFaceOffset - 8) & 0x1c;
		baseCountOffset = (scratch->baseCountOffset - 4) & 0xe;
		faceIndex = (int)(baseFaceOffset >> 2);
		countIndex = (int)(baseCountOffset >> 1);
		prim = DrawSky_Piece(sky, &ctx, faceIndex, countIndex, prim);
	}

	primMem->cursor = prim;
}
