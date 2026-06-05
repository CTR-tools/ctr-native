#include <common.h>

#ifdef CTR_NATIVE
#include <platform/native_str.h>

#define SCRAPBOOK_NATIVE_SRC_X       512
#define SCRAPBOOK_NATIVE_SRC_Y       0
#define SCRAPBOOK_NATIVE_DST_X       0
#define SCRAPBOOK_NATIVE_DST_Y       4
#define SCRAPBOOK_NATIVE_WIDTH       512
#define SCRAPBOOK_NATIVE_HEIGHT      208
#define SCRAPBOOK_NATIVE_STRIP_WIDTH 128

static void MM_Scrapbook_DrawNativeFrame(void)
{
	struct GameTracker *gGT = sdata->gGT;
	u32 *prim = (u32 *)gGT->backBuffer->primMem.curr;
	u32 *firstPrim = prim;
	u_long *ot = gGT->pushBuffer_UI.ptrOT;
	u32 oldTag = (u32)*ot;
	s32 x;

	for (x = 0; x < SCRAPBOOK_NATIVE_WIDTH; x += SCRAPBOOK_NATIVE_STRIP_WIDTH)
	{
		s16 tile[16] = {
		    SCRAPBOOK_NATIVE_SRC_X + (s16)x, SCRAPBOOK_NATIVE_SRC_Y, SCRAPBOOK_NATIVE_STRIP_WIDTH, SCRAPBOOK_NATIVE_HEIGHT,
		    SCRAPBOOK_NATIVE_DST_X + (s16)x, SCRAPBOOK_NATIVE_DST_Y, SCRAPBOOK_NATIVE_STRIP_WIDTH, SCRAPBOOK_NATIVE_HEIGHT,
		};

		prim = DISPLAY_Blur_SubFunc(prim, tile);
	}

	*ot = (u_long)CtrGpu_PrimToOTLink24(firstPrim);
	prim[-10] = oldTag | 0x09000000;
	gGT->backBuffer->primMem.curr = prim;
}
#endif

#ifndef CTR_NATIVE
__attribute__((optimize("O0"))) int ScrapBookPlayMovie_DecodeFrame()
{
	struct GameTracker *gGT = sdata->gGT;
	DRAWENV *ptrDrawEnv = &gGT->db[1 - gGT->swapchainIndex].drawEnv;

	return MM_Video_DecodeFrame(ptrDrawEnv->ofs[0], ptrDrawEnv->ofs[1] + 4) == 0;
}
#endif

// NOTE(aalhendi): ASM-verified NTSC-U 926 overlay 230 0x800b4014-0x800b42b0 PSX path.
void MM_Scrapbook_PlayMovie(struct RectMenu *menu)
{
	s16 lev;
	int cdPos;
	int getButtonPress = 0;
	DRAWENV *ptrDrawEnv;
	CdlFILE cdlFile;
	struct GameTracker *gGT = sdata->gGT;

	// book state (0,1,2,3,4)
	switch (D230.scrapbookState)
	{
	// Init State,
	// alter checkered flag
	case 0:
		if (RaceFlag_IsFullyOnScreen() == 1)
		{
			// checkered flag, begin transition off-screen
			RaceFlag_BeginTransition(2);
		}

		// go to Load State
		D230.scrapbookState = 1;
		menu->state &= ~NEEDS_TO_CLOSE;
		// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b4070-0x800b408c for scrapbook audio state handoff.
		Audio_SetState_Safe(1);
		break;

	// Load State,
	// find the TEST.STR file
	case 1:

		// if not fully off screen
		if (RaceFlag_IsFullyOffScreen() != 1)
		{
			// quit, dont start video yet
			return;
		}

		// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b40a8-0x800b40b4 for scrapbook CD stream mode.
		CDSYS_SetMode_StreamData();

#ifdef CTR_NATIVE
		if (NativeSTR_StartScrapbook() != 0)
		{
			D230.scrapbookState = 2;
			return;
		}
#else
		// \TEST.STR;1
		// if file was found
		if (CdSearchFile(&cdlFile, R230.s_teststr1) != 0)
		{
			SpuSetCommonCDVolume(sdata->vol_Music << 7, sdata->vol_Music << 7);

			// Alloc memory to store Scrapbook
			MM_Video_AllocMem(0x200, 0xd0, 10, 0x40, 1);

			cdPos = CdPosToInt(&cdlFile.pos);

			// scrapbook runs 15fps,
			// see bottom of Duckstation screen while running

			// scrapbook is 4.91 min, 4 mins + 54 sec,
			// (4424 total / 15fps / 60 sec per min) mins,
			// 0x1148 = 4424 = numFrames

			// CD position of video, and numFrames
			MM_Video_StartStream(cdPos, 0x1148);

			// start playing movie
			D230.scrapbookState = 2;

			return;
		}
#endif

		goto GO_BACK;

	// Actually play the movie
	case 2:

#ifndef CTR_NATIVE
		// infinite loop (cause this is scrapbook),
		// keep doing DecodeFrame and VSync until done
		while (ScrapBookPlayMovie_DecodeFrame())
		{
			VSync(0);
		}

		// If you press Start, Cross, Circle, Triangle, or Square
		getButtonPress = (sdata->buttonTapPerPlayer[0] & 0x41070);

		if (
		    // if movie is finished,
		    // means scrapbook ended, no looping
		    (MM_Video_CheckIfFinished(0) == 1) || (getButtonPress != 0))
#else
		getButtonPress = (sdata->buttonTapPerPlayer[0] & 0x41070);

		if ((getButtonPress != 0) || (NativeSTR_UploadNextFrame(SCRAPBOOK_NATIVE_SRC_X, SCRAPBOOK_NATIVE_SRC_Y) == 0))
#endif
		{
			if (getButtonPress != 0)
			{
				RaceFlag_SetFullyOnScreen();
			}

			// stop video
			D230.scrapbookState = 3;
		}
#ifdef CTR_NATIVE
		else
		{
			MM_Scrapbook_DrawNativeFrame();
		}
#endif

		VSync(4);
		break;

	// return disc to normal,
	// return checkered flag to normal
	case 3:
#ifndef CTR_NATIVE
		SpuSetCommonCDVolume(0, 0);

		MM_Video_StopStream();

		MM_Video_ClearMem();
#else
		NativeSTR_Stop();
#endif

		if (RaceFlag_IsFullyOffScreen() == 1)
		{
			// checkered flag, begin transition on-screen
			RaceFlag_BeginTransition(1);
		}
	GO_BACK:

		// return to gameplay
		D230.scrapbookState = 4;
		break;

	// send player back to adv hub,
	// or back to main menu
	case 4:
		if (RaceFlag_IsFullyOnScreen() == 1)
		{
			// change checkered flag back
			RaceFlag_SetDrawOrder(0);

			// if adventure mode
			lev = GEM_STONE_VALLEY;

			// If you're not in Adventure Mode
			if ((gGT->gameMode1 & ADVENTURE_MODE) == 0)
			{
				lev = MAIN_MENU_LEVEL;

				MM_JumpTo_Title_Returning();

				// return to main menu (adv, tt, arcade, vs, battle)
				sdata->mainMenuState = 0;
			}

			MainRaceTrack_RequestLoad(lev);

			RECTMENU_Hide(menu);
		}
		break;
	default:
		return;
	}
}
