#include <common.h>

#ifdef CTR_NATIVE
static void MainInit_InitVisMemBspListNodes(struct VisMem *visMem, struct mesh_info *mesh)
{
	if (mesh == NULL || mesh->bspRoot == NULL)
	{
		return;
	}

	for (int playerIndex = 0; playerIndex < 4; playerIndex++)
	{
		struct VisMemBspListNode *bspList = visMem->bspList[playerIndex];

		if (bspList == NULL)
		{
			continue;
		}

		for (int bspIndex = 0; bspIndex < mesh->numBspNodes; bspIndex++)
		{
			// NOTE(aalhendi): Native 226 reads the retained BSP pointer; RenderLists only rewrites the link word.
			bspList[bspIndex].next = NULL;
			bspList[bspIndex].bsp = &mesh->bspRoot[bspIndex];
		}
	}
}
#endif

void MainInit_VisMem(struct GameTracker *gGT)
{
	struct VisMem *visMem = gGT->level1->visMem;
	s32 i;
	gGT->visMem1 = visMem;

	if (visMem == NULL)
	{
		return;
	}

	for (i = 0; i < gGT->numPlyrCurrGame; i++)
	{
		gGT->visMem1->visLeafSrc[i] = NULL;
		gGT->visMem1->visFaceSrc[i] = NULL;
		gGT->visMem1->visOVertSrc[i] = NULL;
		gGT->visMem1->visSCVertSrc[i] = NULL;
	}

#ifdef CTR_NATIVE
	MainInit_InitVisMemBspListNodes(visMem, gGT->level1->ptr_mesh_info);
#endif
}

// NOTE(aalhendi): Packed lists encode their own extent; only raw copies read
// the level's bit count. Reload the destination after selecting the source.
static inline void MainFrame_ReplacePackedVisList(struct GameTracker *tracker, int playerIndex, int listOffset, void *src, const int *bitCount)
{
	u32 srcWord = (u32)src;

	if ((srcWord & 1) != 0)
	{
		CTR_unknownMaybeThunk1(*(int **)((u8 *)tracker->visMem1 + (playerIndex << 2) + listOffset), (void *)(srcWord & ~(u32)3));
		return;
	}

	memcpy(*(int **)((u8 *)tracker->visMem1 + (playerIndex << 2) + listOffset), src, ((*bitCount + 0x1f) >> 5) << 2);
}

#define MainFrame_OrPackedVisList(dst, src, byteCount)                   \
	do                                                                   \
	{                                                                    \
		u32 srcWord = (u32)(src);                                        \
		if ((srcWord & 1) != 0)                                          \
			CTR_unknownMaybeThunk2((dst), (void *)(srcWord & ~(u32)3));  \
		else                                                             \
			CTR_unknownMaybeThunk3((dst), (void *)srcWord, (byteCount)); \
	} while (0)
#define MAIN_FRAME_REPLACE_FIELD(tracker, index, field, src, count) \
	MainFrame_ReplacePackedVisList((tracker), (index), offsetof(struct VisMem, field), (src), &(count))

static inline int MainFrame_VisMemHasQuad(const int *visFaceList, const struct QuadBlock *quad, const struct mesh_info *mesh)
{
	int quadIndex = (int)(quad - mesh->ptrQuadBlockArray);
	int byteOffset;
	const int *wordPtr;
	int word;
	register int bitIndex CTR_PSX_REGISTER("$4");
	register u32 mask CTR_PSX_REGISTER("$3");
	// NOTE(aalhendi): Offset-first 32-bit address arithmetic keeps retail's
	// ADDU operands; finish the address before v1 is reused for the mask.
	bitIndex = quadIndex;
	byteOffset = (quadIndex >> 5) * 4;
	wordPtr = (const int *)((u32)byteOffset + (u32)visFaceList);
	bitIndex &= 0x1f;
	CTR_PSX_BIND_VALUE_CLOBBER(wordPtr, "$3");
	mask = 1u;
	word = *wordPtr;
	mask <<= bitIndex;
	return (word & mask) != 0;
}

#ifdef CTR_NATIVE
static
#endif
    void MainFrame_VisMemAddDriverPVS(struct GameTracker *gGT, int playerIndex)
{
	struct Driver *driver = gGT->drivers[playerIndex];
	register struct mesh_info *mesh CTR_PSX_REGISTER("$19") = gGT->level1->ptr_mesh_info;
	struct QuadBlock *quad = driver->underDriver;
	struct PVS *pvs;

	if (quad == NULL)
	{
		return;
	}

	pvs = quad->pvs;
	if (pvs == NULL)
	{
		return;
	}

	if (pvs->visLeafSrc != NULL)
	{
		MainFrame_OrPackedVisList(gGT->visMem1->visLeafList[playerIndex], pvs->visLeafSrc, ((mesh->numBspNodes + 0x1f) >> 5) << 2);
	}

	// NOTE(aalhendi): Read face visibility after the leaf decoder has finished.
	pvs = driver->underDriver->pvs;

	if (pvs->visFaceSrc != NULL)
	{
		register int faceOffset CTR_PSX_REGISTER("$3") = playerIndex << 2;
		u8 *faceBytes;
		// NOTE(aalhendi): Offset the complete byte owner, then read the real
		// face-list pointer slot without constructing a shifted VisMem object.
		MainFrame_OrPackedVisList(*(int **)((faceBytes = (u8 *)gGT->visMem1 + faceOffset) + offsetof(struct VisMem, visFaceList)), pvs->visFaceSrc,
		                          ((mesh->numQuadBlock + 0x1f) >> 5) << 2);
	}
}

#undef MainFrame_OrPackedVisList

// NOTE(aalhendi): Native accepts visibility from the supplied level; retail
// tests driver quads against the resident level's mesh.
#ifdef CTR_NATIVE
#define MAIN_FRAME_QUAD_MESH(tracker, supplied) (supplied)
#else
#define MAIN_FRAME_QUAD_MESH(tracker, supplied) ((tracker)->level1->ptr_mesh_info)
#endif

void MainFrame_VisMemFullFrame(struct GameTracker *gGT, struct Level *suppliedLevel)
{
	register struct Level *level CTR_PSX_REGISTER("$21") = suppliedLevel;
	struct VisMem *visMem;
	register struct mesh_info *mesh CTR_PSX_REGISTER("$22");
	int playerIndex;

	visMem = gGT->visMem1;
	if (visMem == NULL)
	{
		return;
	}

	if (level == NULL)
	{
		return;
	}

	CTR_PSX_FORGET_VALUE(level);
	mesh = level->ptr_mesh_info;

	if (gGT->numPlyrCurrGame == 0)
	{
		return;
	}

	playerIndex = 0;
	do
	{
		struct CameraDC *camDC = &gGT->cameraDC[playerIndex];
		struct Driver *driver = gGT->drivers[playerIndex];
		void *leafSource;
		void *faceSource;

		camDC->flags &= ~0x4000;

		if (camDC->visLeafSrc == NULL)
		{
			if (((driver->underDriver != NULL) && (driver->underDriver->pvs != NULL)) && (driver->underDriver->pvs->visLeafSrc != NULL))
			{
				gGT->visMem1->visLeafSrc[playerIndex] = driver->underDriver->pvs->visLeafSrc;
				leafSource = driver->underDriver->pvs->visLeafSrc;
				if (((u32)leafSource & 1) == 0)
					goto leafRaw;
				// Keep this packed fallback separate from the changed-camera path.
				CTR_PSX_OBSERVE_VALUE(leafSource);
				goto leafPacked;
			}
		}
		else if (gGT->visMem1->visLeafSrc[playerIndex] != camDC->visLeafSrc)
		{
			gGT->visMem1->visLeafSrc[playerIndex] = camDC->visLeafSrc;
			leafSource = camDC->visLeafSrc;
			if (((u32)leafSource & 1) == 0)
				goto leafRaw;
		leafPacked:
			CTR_unknownMaybeThunk1(gGT->visMem1->visLeafList[playerIndex], (void *)((u32)leafSource & ~(u32)3));
			goto leafDone;
		leafRaw:
			memcpy(gGT->visMem1->visLeafList[playerIndex], leafSource, ((mesh->numBspNodes + 0x1f) >> 5) << 2);
		}

	leafDone:;

		if (camDC->visFaceSrc == NULL)
		{
			if (((driver->underDriver != NULL) && (driver->underDriver->pvs != NULL)) && (driver->underDriver->pvs->visFaceSrc != NULL))
			{
				gGT->visMem1->visFaceSrc[playerIndex] = driver->underDriver->pvs->visFaceSrc;
				faceSource = driver->underDriver->pvs->visFaceSrc;
				if (((u32)faceSource & 1) == 0)
					goto faceFallbackRaw;
				CTR_unknownMaybeThunk1(gGT->visMem1->visFaceList[playerIndex], (void *)((u32)faceSource & ~(u32)3));
				goto faceDone;
			faceFallbackRaw:
				memcpy(gGT->visMem1->visFaceList[playerIndex], faceSource, ((mesh->numQuadBlock + 0x1f) >> 5) << 2);
				goto faceDone;
			}
		}
		else if (gGT->visMem1->visFaceSrc[playerIndex] != camDC->visFaceSrc)
		{
			gGT->visMem1->visFaceSrc[playerIndex] = camDC->visFaceSrc;
			faceSource = camDC->visFaceSrc;
			if (((u32)faceSource & 1) == 0)
				goto faceChangedRaw;
			CTR_unknownMaybeThunk1(gGT->visMem1->visFaceList[playerIndex], (void *)((u32)faceSource & ~(u32)3));
			goto faceChangedDone;
		faceChangedRaw:
			memcpy(gGT->visMem1->visFaceList[playerIndex], faceSource, ((mesh->numQuadBlock + 0x1f) >> 5) << 2);
		faceChangedDone:;

			if (((driver->underDriver != NULL) && (driver->underDriver->pvs != NULL)) && (driver->underDriver->pvs->visLeafSrc != NULL) &&
			    (driver->underDriver->pvs->visFaceSrc != NULL) && (driver->underDriver->pvs->visInstSrc != NULL) &&
			    !MainFrame_VisMemHasQuad(gGT->visMem1->visFaceList[playerIndex], driver->underDriver, MAIN_FRAME_QUAD_MESH(gGT, mesh)))
			{
				camDC->flags |= 0x2000;
			}
			else
			{
				camDC->flags &= ~0x2000;
			}

			if ((camDC->flags & 0x2000) != 0)
			{
				MainFrame_VisMemAddDriverPVS(gGT, playerIndex);
				camDC->flags |= 0x4000;
			}
		}

	faceDone:;

		if ((camDC->flags & 0x5000) == 0x1000)
		{
			MainFrame_VisMemAddDriverPVS(gGT, playerIndex);
		}

		if ((camDC->cameraMode == 0) && ((camDC->flags & 0x2000) != 0) && ((driver->underDriver != NULL) && (driver->underDriver->pvs != NULL)) &&
		    (driver->underDriver->pvs->visInstSrc != NULL))
		{
			camDC->visInstSrc = driver->underDriver->pvs->visInstSrc;
		}

		if ((level->configFlags & 4) == 0)
		{
			if (gGT->visMem1->visOVertSrc[playerIndex] != camDC->visOVertSrc)
			{
				gGT->visMem1->visOVertSrc[playerIndex] = camDC->visOVertSrc;
				MAIN_FRAME_REPLACE_FIELD(gGT, playerIndex, visOVertList, camDC->visOVertSrc, level->numWaterVertices);
			}
			else if (gGT->visMem1->visOVertSrc[playerIndex] == NULL)
			{
				memcpy(gGT->visMem1->visOVertList[playerIndex], level->visOVertSrc, ((level->numWaterVertices + 0x1f) >> 5) << 2);
			}
		}
		else
		{
			if (gGT->visMem1->visSCVertSrc[playerIndex] != camDC->visSCVertSrc)
			{
				gGT->visMem1->visSCVertSrc[playerIndex] = camDC->visSCVertSrc;
				MAIN_FRAME_REPLACE_FIELD(gGT, playerIndex, visSCVertList, camDC->visSCVertSrc, level->numSCVert);
			}
			else if (gGT->visMem1->visSCVertSrc[playerIndex] == NULL)
			{
				memcpy(gGT->visMem1->visSCVertList[playerIndex], level->visSCVertSrc, ((level->numSCVert + 0x1f) >> 5) << 2);
			}
		}
		playerIndex++;
	} while (playerIndex < gGT->numPlyrCurrGame);
}

#undef MAIN_FRAME_REPLACE_FIELD
#undef MAIN_FRAME_QUAD_MESH
