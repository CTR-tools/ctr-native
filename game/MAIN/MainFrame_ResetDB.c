#include <common.h>

#ifndef MAIN_FRAME_BIGFILE1
#define MAIN_FRAME_BIGFILE1 (sdata->ptrBigfile1)
#endif

#if defined(CTR_NATIVE)
static void MainFrame_RegisterGpuLinkRanges(struct GameTracker *gGT)
{
	static const char *const primLabels[2] = {"db0 prim", "db1 prim"};
	static const char *const otLabels[2] = {"db0 OT", "db1 OT"};
	static const char *const swapchainLabels[2] = {"swapchain OT0", "swapchain OT1"};

	NativeGpuLinks_Reset();

	// NOTE(aalhendi): Retail links PS1 RAM addresses directly in 24-bit GPU
	// tags. Native keeps the same packet shape, but maps the double-buffered
	// host draw arenas to stable 24-bit tokens before any OT/tag writer runs.
	for (s32 i = 0; i < 2; i++)
	{
		struct DB *db = &gGT->db[i];
		NativeGpuLinks_RegisterRangeChecked(primLabels[i], db->primMem.start, db->primMem.capacityBytes);
		NativeGpuLinks_RegisterRangeChecked(otLabels[i], db->otMem.start, db->otMem.capacityBytes);
	}

	u32 swapchainOTBytes = ((u32)gGT->numPlyrCurrGame << 12) | 0x18u;
	for (s32 i = 0; i < 2; i++)
	{
		NativeGpuLinks_RegisterRangeChecked(swapchainLabels[i], gGT->otSwapchainDB[i], swapchainOTBytes);
	}
}
#endif

void MainFrame_ResetDB(struct GameTracker *gGT)
{
	s32 i;
	register struct GameTracker *frameTracker CTR_PSX_REGISTER("$6");

	LOAD_Hub_Main(MAIN_FRAME_BIGFILE1);
	gGT->swapchainIndex = 1 - gGT->swapchainIndex;
	gGT->backBuffer = &gGT->db[gGT->swapchainIndex];
	gGT->frameTimer_MainFrame_ResetDB++;
	gGT->backBuffer->blurCameraMask = 0;
	gGT->backBuffer->primMem.cursor = gGT->backBuffer->primMem.start;
	gGT->backBuffer->primMem.primitiveCount = 0;
	gGT->backBuffer->otMem.cursor = gGT->backBuffer->otMem.start;
#ifdef CTR_NATIVE
	MainFrame_RegisterGpuLinkRanges(gGT);
#endif
	CTR_EmptyFunc_MainFrame_ResetDB();
	DecalGlobal_EmptyFunc_MainFrame_ResetDB(gGT);
	ClearOTagR(gGT->otSwapchainDB[gGT->swapchainIndex], GAME_TRACKER->numPlyrCurrGame << 10 | 6);
	// NOTE(aalhendi): Keep retail's tracker register and per-iteration player-count load.
	frameTracker = GAME_TRACKER;
	for (i = 0; i < frameTracker->numPlyrCurrGame; i++)
	{
		CTR_PSX_RELOAD(frameTracker->numPlyrCurrGame);
		gGT->pushBuffer[i].ptrOT = (u32 *)((u8 *)gGT->otSwapchainDB[gGT->swapchainIndex] + ((frameTracker->numPlyrCurrGame - i - 1) * 0x1000 + 0x18));
	}
	for (; i < 4; i++)
	{
		gGT->pushBuffer[i].ptrOT = (u32 *)((u8 *)gGT->otSwapchainDB[gGT->swapchainIndex] + 0x3018);
	}
	gGT->pushBuffer_UI.ptrOT = (u32 *)((u8 *)gGT->otSwapchainDB[gGT->swapchainIndex] + 4);
	gGT->backBuffer->otMem.uiOT = gGT->pushBuffer_UI.ptrOT;
#ifdef CTR_NATIVE
	if (sdata->ptrPushBufferUI != 0)
	{
		struct PushBuffer *wumpaPushBuffer = (struct PushBuffer *)(u32)sdata->ptrPushBufferUI;

		// NOTE(aalhendi): Native's fake UI pushbuffer stores host OT pointers;
		// reset its range metadata before RenderBucket publishes this frame.
		wumpaPushBuffer->ptrOT = gGT->pushBuffer_UI.ptrOT;
		wumpaPushBuffer->renderBucketOTRangeEnd = NULL;
		wumpaPushBuffer->renderBucketOTByteOffset = 0;
	}
#endif
}

#undef MAIN_FRAME_BIGFILE1
