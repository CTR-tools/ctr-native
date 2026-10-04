
#include <common.h>

static inline struct InstDrawPerPlayer *DecalMP_GetIdpp(struct Instance *inst, s32 cameraID)
{
	return (struct InstDrawPerPlayer *)((char *)INST_GETIDPP(inst) + (cameraID * sizeof(struct InstDrawPerPlayer)));
}


void DecalMP_01(struct GameTracker *unusedGameTracker)
{
	struct DecalMPEntry *entry = GAME_TRACKER->DecalMP;
	struct PushBuffer *pb = GAME_TRACKER->pushBuffer;
	s32 cameraID;
	(void)unusedGameTracker;
	for (cameraID = 0; cameraID < GAME_TRACKER->numPlyrCurrGame; cameraID++, pb++)
	{
		s32 driverID;
		// NOTE(aalhendi): Retail counts driver visits per viewport, not populated
		// entries. Keep the independent visit check; the entry cursor spans viewports.
		s32 visits;
		for (driverID = 0, visits = driverID; driverID < 8; driverID++, visits++)
		{
			struct Driver *driver;
			struct Instance *inst;
			struct InstDrawPerPlayer *idpp;
			if (visits >= 12)
			{
				break;
			}
			driver = GAME_TRACKER->drivers[driverID];
			if (driver == NULL)
			{
				continue;
			}
			inst = driver->instSelf;
			inst->flags |= PUSHBUFFER_EXISTS | PIXEL_LOD;
			if (driverID == cameraID)
			{
				continue;
			}
			if (driver->kartState == KS_BLASTED)
			{
				entry->kartState = KS_BLASTED;
			}
			else if (entry->kartState == KS_BLASTED)
			{
				entry->kartState = 0;
				entry->timer = 1000;
			}
			entry->pb.matrix_ViewProj = pb->matrix_ViewProj;
			entry->pb.pos.x = pb->pos.x;
			entry->pb.pos.y = pb->pos.y;
			entry->pb.pos.z = pb->pos.z;
			entry->pb.rect.x = pb->rect.x;
			entry->pb.rect.y = pb->rect.y;
			entry->pb.rect.w = pb->rect.w;
			entry->pb.rect.h = pb->rect.h;
			entry->pb.ptrOT = pb->ptrOT;
			entry->pb.distanceToScreen_PREV = pb->distanceToScreen_PREV;
			entry->pb.cameraID = pb->cameraID;
			// NOTE(aalhendi): Keep the camera stride separate from the Instance header offset.
			idpp = INST_GETIDPP((u8 *)inst + cameraID * sizeof(struct InstDrawPerPlayer));
			idpp->pushBuffer = &entry->pb;
			entry->inst = inst;
			entry++;
		}
	}
}


void DecalMP_02(struct GameTracker *gGT)
{
	struct DecalMPEntry *entry = gGT->DecalMP;
	s32 index;
	for (index = 0; index < 12; index++, entry++)
	{
		s32 cameraID;
		struct InstDrawPerPlayer *idpp;
		s32 timer;
		if (entry->inst == NULL)
		{
			return;
		}
		cameraID = entry->pb.cameraID;
		idpp = DecalMP_GetIdpp(entry->inst, cameraID);
		if ((idpp->instFlags & 0x140) == 0x140)
		{
			s32 minFrames;
			timer = entry->timer;
			minFrames = entry->pb.renderBucketOTByteOffset >> 3;
			if (minFrames < 2)
			{
				minFrames = 2;
			}
			if (timer >= 1000 || ((timer > minFrames || entry->lodIndex != idpp->lodIndex) && ((gGT->timer ^ index) & 1) != 0))
			{
				entry->boolUpdatedThisFrame = 1;
				if (entry->pb.ptrOT != NULL && entry->pb.renderBucketOTRangeEnd != NULL)
				{
					*entry->pb.ptrOT = gGT->pushBuffer[cameraID].ptrOT[0x3ff];
					gGT->pushBuffer[cameraID].ptrOT[0x3ff] = CtrGpu_PrimToOTLink24(entry->pb.renderBucketOTRangeEnd);
				}
			}
			else
			{
				idpp->instFlags |= 0x80;
				entry->boolUpdatedThisFrame = 0;
			}
			if ((gGT->gameMode1 & PAUSE_ALL) == 0)
			{
				entry->timer = timer + 1;
			}
		}
		else
		{
			entry->timer = 1000;
		}
	}
}

void DecalMP_03(struct GameTracker *gGT)
{
	struct DecalMPEntry *entry = gGT->DecalMP;
	RECT viewport;
	s32 texY, maxTexX;
	s32 texX = 0x1a0;
	s32 index;
	viewport.w = 0x60;
	viewport.h = 0x40;
	// NOTE(aalhendi): Retail uses the singleton for atlas Y and allocation,
	// but the supplied tracker for viewport OTs and final cursor publication.
	texY = (GAME_TRACKER->numPlyrCurrGame == 2) ? 0x180 : 0x100;
	maxTexX = (gGT->numPlyrCurrGame > 2) ? 0x2c0 : 0x380;
	for (index = 0; index < 12; index++, entry++)
	{
		s32 cameraID;
		struct InstDrawPerPlayer *idpp;
		POLY_FT4 *poly;
		s32 x, xEnd;
		u32 topY, bottomY;
		u32 *ot;
		texX += 0x60;
		if (maxTexX < texX)
		{
			texX = 0x200;
			texY += 0x40;
		}
		if (entry->inst == NULL)
		{
			return;
		}
		cameraID = entry->pb.cameraID;
		idpp = DecalMP_GetIdpp(entry->inst, cameraID);
		if ((idpp->instFlags & 0x140) != 0x140)
		{
			continue;
		}
		if (entry->boolUpdatedThisFrame != 0)
		{
			entry->timer = 0;
			x = texX - entry->pb.renderBucketScreenPos.pos.x;
			topY = texY - entry->pb.renderBucketScreenPos.pos.y;
			entry->renderW = entry->pb.renderBucketScreenSize.size.w;
			entry->renderH = entry->pb.renderBucketScreenSize.size.h;
			entry->lodIndex = idpp->lodIndex;
			viewport.x = texX;
			viewport.y = texY;
			PushBuffer_SetDrawEnv_DecalMP(entry->pb.renderBucketOTRangeEnd, GAME_TRACKER->backBuffer, &viewport, (s16)x, (s16)topY, 0, 0, 0, 0, 1);
		}
		poly = GAME_TRACKER->backBuffer->primMem.cursor;
		poly->code = 0x2d;

		// NOTE(aalhendi): Reuse the refresh carriers for the quad and texture bounds.
		// Shift unsigned rows, but preserve the retail unmasked signed X bits.
		x = entry->pb.renderBucketScreenPos.pos.x;
		topY = (u32)entry->pb.renderBucketScreenPos.pos.y << 16;
		xEnd = x + entry->pb.renderBucketScreenSize.size.w;
		bottomY = topY + ((u32)entry->pb.renderBucketScreenSize.size.h << 16);
		CtrGpu_WritePackedXY(&poly->x0, (u32)x | topY);
		CtrGpu_WritePackedXY(&poly->x1, (u32)xEnd | topY);
		CtrGpu_WritePackedXY(&poly->x2, (u32)x | bottomY);
		CtrGpu_WritePackedXY(&poly->x3, (u32)xEnd | bottomY);
		x = texX & 0x3f;
		topY = texY & 0xff;
		xEnd = x + entry->renderW;
		bottomY = topY + entry->renderH;
		if ((s32)bottomY >= 0x100)
		{
			bottomY = 0xff;
		}
		topY <<= 8;
		bottomY <<= 8;
		CtrGpu_WritePackedUV(&poly->u0, (u16)(x | topY));
		CtrGpu_WritePackedUV(&poly->u1, (u16)(xEnd | topY));
		CtrGpu_WritePackedUV(&poly->u2, (u16)(x | bottomY));
		CtrGpu_WritePackedUV(&poly->u3, (u16)(xEnd | bottomY));
		poly->tpage = getTPage(TEXPAGE_COLOR_15BIT, TRANS_50, texX, texY);
		ot = gGT->pushBuffer[cameraID].ptrOT + (entry->pb.renderBucketOTByteOffset >> 2);
		// NOTE(aalhendi): Retail preserves the incoming GPU link word without masking.
		poly->tag = *ot | 0x09000000;
		CtrGpu_LinkPrimToOT(ot, poly);
		gGT->backBuffer->primMem.cursor = poly + 1;
	}
}
