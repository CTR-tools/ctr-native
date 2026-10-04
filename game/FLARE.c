#include <common.h>
#include <ctr_gte_transfer.h>

struct FlareObject
{
	s32 frameCount;
	// NOTE(aalhendi): Init copies all eight caller bytes; Tick uses XYZ, not W.
	SVec4 position;
};

CTR_STATIC_ASSERT(sizeof(struct FlareObject) == 0xc);

static inline u32 FLARE_PackXY(s16 x, s16 y)
{
	return (u16)x | ((u32)(u16)y << 16);
}

// Project three rows of a 3x3 grid; their shared Z words are initialized once.
static inline void FLARE_LoadGridRow(s16 y)
{
	MTC2(FLARE_PackXY(-409, y), 0);
	MTC2(FLARE_PackXY(0, y), 2);
	MTC2(FLARE_PackXY(409, y), 4);
}

#define FLARE_WriteTexture(poly, word0, word1, uv2, uv3)                        \
	do                                                                          \
	{                                                                           \
		POLY_GT4 *flare_texture_poly = (poly);                                  \
		u32 flare_texture_word0 = (word0);                                      \
		u32 flare_texture_word1 = (word1);                                      \
		u16 flare_texture_uv2 = (uv2);                                          \
		u16 flare_texture_uv3 = (uv3);                                          \
		CtrGpu_WritePackedUVWord(&flare_texture_poly->u0, flare_texture_word0); \
		CtrGpu_WritePackedUVWord(&flare_texture_poly->u1, flare_texture_word1); \
		CtrGpu_WritePackedUV(&flare_texture_poly->u2, flare_texture_uv2);       \
		CtrGpu_WritePackedUV(&flare_texture_poly->u3, flare_texture_uv3);       \
	} while (0)

#define FLARE_WriteColors(poly, word0, word3)                            \
	do                                                                   \
	{                                                                    \
		POLY_GT4 *flare_color_poly = (poly);                             \
		u32 flare_color_word0 = (word0);                                 \
		u32 flare_color_word3 = (word3);                                 \
		CtrGpu_WriteColorCode(&flare_color_poly->r0, flare_color_word0); \
		CtrGpu_WriteColorCode(&flare_color_poly->r1, 0);                 \
		CtrGpu_WriteColorCode(&flare_color_poly->r2, 0);                 \
		CtrGpu_WriteColorCode(&flare_color_poly->r3, flare_color_word3); \
	} while (0)

void FLARE_ThTick(struct Thread *th)
{
	struct GameTracker *gGT;
	struct PushBuffer *pb;
	struct FlareObject *flare = th->object;
	s32 timer = flare->frameCount;
	POLY_GT4 *prim;
	s32 relX;
	s32 relY;
	s32 relZ;
	s32 scale;
	s32 sin;
	s32 cos;
	s32 scaledCos;
	s32 scaledSin;
	struct Icon *icon;
	u32 texWord0;
	u32 texWord1;
	s32 texUV2;
	s32 texUV3;
	u32 colorWord0;
	u32 colorWord3;
	void *rowFirstXY;
	POLY_GT4 *p0;
	POLY_GT4 *p1;
	POLY_GT4 *p2;
	POLY_GT4 *p3;
	s32 depth;
	u32 *ot;
	u32 firstOTToken;

	flare->frameCount = timer + 1;

	if (timer >= 20)
	{
		th->flags |= THREAD_FLAG_DEAD;
		return;
	}

	gGT = GAME_TRACKER;
	prim = gGT->backBuffer->primMem.cursor;
	if ((char *)(prim + 4) >= (char *)gGT->backBuffer->primMem.guardEnd)
	{
		return;
	}

	pb = &gGT->pushBuffer[0];
	PushBuffer_SetPsyqGeom(pb);
	SetLightMatrix(&pb->matrix_ViewProj);

	relX = CTR_MipsSll((s32)flare->position.x - pb->matrix_Camera.t[0], 2);
	relY = CTR_MipsSll((s32)flare->position.y - pb->matrix_Camera.t[1], 2);

	MTC2(((u32)(u16)relX) | ((u32)relY << 16), 0);
	relZ = CTR_MipsSll((s32)flare->position.z - pb->matrix_Camera.t[2], 2);
	MTC2(relZ, 1);
	CTR_PSX_GTE_PIPELINE_DELAY();
	gte_llv0();

	CTR_GteMoveMACToTranslation();

	if (timer < 2)
	{
		scale = VehCalc_MapToRange(timer, 0, 2, 0x400, 0x2000);
	}
	else if (timer < 4)
	{
		scale = VehCalc_MapToRange(timer, 2, 4, 0x2000, 0xc00);
	}
	else if (timer < 8)
	{
		scale = VehCalc_MapToRange(timer, 4, 8, 0xc00, 0x266);
	}
	else
	{
		scale = VehCalc_MapToRange(timer, 8, 20, 0x266, 0);
	}

	sin = (MATH_Sin((u32)(CTR_MipsSll(flare->frameCount, 12) / 20)) * scale) >> 12;
	cos = (MATH_Cos((u32)(CTR_MipsSll(flare->frameCount, 12) / 20)) * scale) >> 12;
	p0 = prim;
	p1 = prim + 1;
	p2 = prim + 2;
	p3 = prim + 3;

	scaledCos = CTR_MipsSll(cos, 9) / 0xf0;
	scaledSin = CTR_MipsSll(sin, 9) / 0xf0;

	CTC2(((u16)scaledCos) | ((u32)(u16)-scaledSin << 16), 0);
	CTC2((u32)sin << 16, 1);
	CTC2((u32)cos & 0xffff, 2);
	CTC2(0, 3);
	CTC2(scale, 4);

	icon = GAME_TRACKER->ptrIcons[0x87];
	if (icon == NULL)
	{
		return;
	}

	texWord0 = CTR_ReadU32LE(&icon->texLayout.u0);
	texWord1 = (CTR_ReadU32LE(&icon->texLayout.u1) & 0xff9fffff) | 0x00200000;
	// NOTE(aalhendi): Signed UV snapshots retain retail LH; packet writers keep the low 16 bits.
	texUV2 = CTR_ReadS16LE(&icon->texLayout.u2);
	texUV3 = CTR_ReadS16LE(&icon->texLayout.u3);

	MTC2(0, 1);
	MTC2(0, 3);
	MTC2(0, 5);
	FLARE_LoadGridRow(-409);
	CTR_PSX_GTE_PIPELINE_DELAY();
	gte_rtpt();
	colorWord0 = 0x3e000000;
	colorWord3 = 0x007f7f7f;
	{
		// NOTE(aalhendi): This destination belongs to one row; consume SXY before the next RTPT.
		void *firstRowXY = &p0->x0;
		FLARE_WriteTexture(p0, texWord0, texWord1, texUV2, texUV3);
		FLARE_WriteTexture(p1, texWord0, texWord1, texUV2, texUV3);
		CTR_PSX_STORE_COP2_WORD(firstRowXY, 12);
		CTR_PSX_STORE_COP2_WORD(&p0->x1, 13);
		CTR_PSX_STORE_COP2_WORD(&p1->x1, 13);
		CTR_PSX_STORE_COP2_WORD(&p1->x0, 14);
	}

	FLARE_LoadGridRow(0);
	CTR_PSX_GTE_PIPELINE_DELAY();
	gte_rtpt();
	rowFirstXY = &p0->x2;
	FLARE_WriteColors(p0, colorWord0, colorWord3);
	FLARE_WriteColors(p1, colorWord0, colorWord3);
	FLARE_WriteTexture(p2, texWord0, texWord1, texUV2, texUV3);
	CTR_PSX_STORE_COP2_WORD(rowFirstXY, 12);
	CTR_PSX_STORE_COP2_WORD(&p0->x3, 13);
	CTR_PSX_STORE_COP2_WORD(&p1->x3, 13);
	CTR_PSX_STORE_COP2_WORD(&p1->x2, 14);
	CTR_PSX_STORE_COP2_WORD(&p2->x2, 12);
	CTR_PSX_STORE_COP2_WORD(&p2->x3, 13);
	CTR_PSX_STORE_COP2_WORD(&p3->x3, 13);
	CTR_PSX_STORE_COP2_WORD(&p3->x2, 14);
	depth = MFC2(18);

	FLARE_LoadGridRow(409);
	CTR_PSX_GTE_PIPELINE_DELAY();
	gte_rtpt();
	rowFirstXY = &p2->x0;
	FLARE_WriteColors(p2, colorWord0, colorWord3);
	FLARE_WriteColors(p3, colorWord0, colorWord3);
	FLARE_WriteTexture(p3, texWord0, texWord1, texUV2, texUV3);
	CTR_PSX_STORE_COP2_WORD(rowFirstXY, 12);
	CTR_PSX_STORE_COP2_WORD(&p2->x1, 13);
	CTR_PSX_STORE_COP2_WORD(&p3->x1, 13);
	CTR_PSX_STORE_COP2_WORD(&p3->x0, 14);

	firstOTToken = CtrGpu_PrimToOTLink24(p1);
	depth >>= 8;
	depth -= 2;
	// The bridge returns a 24-bit link; each GT4 packet has twelve payload words.
	firstOTToken |= 0x0c000000;
	p0->tag = firstOTToken;
	p1->tag = CtrGpu_PackOTTag(CtrGpu_PrimToOTLink24(p2), 0x0c000000);
	p2->tag = CtrGpu_PackOTTag(CtrGpu_PrimToOTLink24(p3), 0x0c000000);
	if (depth < 0)
	{
		depth = 0;
	}
	if (depth > 0x3ff)
	{
		depth = 0x3ff;
	}

	ot = &pb->ptrOT[depth];
	p3->tag = *ot | 0x0c000000;
	*ot = CtrGpu_PrimToOTLink24(p0);

	GAME_TRACKER->backBuffer->primMem.cursor = prim + 4;
}


void FLARE_Init(s16 *pos)
{
	// Twelve-byte object in the small-stack pool, attached to the OTHER thread bucket.
	struct Thread *th = PROC_BirthWithObject(0xc030d, FLARE_ThTick, rdata.s_lensflare, NULL);
	if (th != NULL)
	{
		struct FlareObject *flare = th->object;
		flare->frameCount = 0;
		memcpy(&flare->position, pos, sizeof(flare->position));
	}
}
