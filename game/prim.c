#include <common.h>

void GetPrimitiveMem(void **ppPrim, size_t primSize)
{
	struct DB *backBuffer = sdata->gGT->backBuffer;
	if (backBuffer->primMem.cursor <= backBuffer->primMem.guardEnd)
	{
		*ppPrim = backBuffer->primMem.cursor;

		backBuffer->primMem.cursor = (void *)((size_t)backBuffer->primMem.cursor + primSize);

		((Tag *)*ppPrim)->size = (primSize - sizeof(Tag)) / sizeof(u32);
	}
	else
	{
		*ppPrim = nullptr;
	}
}

void AddPrimitive(void *pPrim, void *pOt)
{
	u32 primTag = CTR_GPU_ReadTagWord(pPrim);
	u32 otTag = CTR_GPU_ReadTagWord(pOt);

	CTR_GPU_WriteTagWord(pPrim, CtrGpu_PackOTTag(otTag, primTag & 0xff000000u));
	CTR_GPU_WriteTagWord(pOt, CtrGpu_PrimToOTLink24(pPrim));
}
