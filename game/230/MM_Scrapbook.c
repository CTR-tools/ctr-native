#include <common.h>

void MM_Scrapbook_Init(void)
{
	D230.scrapbookState = SCRAP_INIT;

	// change checkered flag
	RaceFlag_SetDrawOrder(1);

	// clear gamepad input (for menus)
	RECTMENU_ClearInput();
}

#include <platform.h>
#include <platform/native_audio.h>
#include <platform/native_renderer.h>
#include <platform/native_str.h>

#define SCRAPBOOK_NATIVE_XA_PATH       "TEST.STR"
#define SCRAPBOOK_NATIVE_XA_CHANNEL    1
#define SCRAPBOOK_NATIVE_FRAME_Y_PAD   4
#define SCRAPBOOK_NATIVE_DISPLAY_WIDTH SCREEN_WIDTH

global_variable s32 s_scrapbookNativeNextVBlank;

static void MM_Scrapbook_GetNativeSource(s16 *srcX, s16 *srcY, s16 *displayY)
{
	DRAWENV *drawEnv;
	struct GameTracker *gGT = sdata_static.gGT;
	drawEnv = &gGT->db[1 - gGT->swapchainIndex].drawEnv;

	// NOTE(aalhendi): Retail decodes Scrapbook into the inactive draw page.
	// Native then presents that VRAM display page directly instead of drawing
	// the movie back through a texture primitive.
	*srcX = drawEnv->ofs[0];
	*displayY = drawEnv->ofs[1];
	*srcY = *displayY + SCRAPBOOK_NATIVE_FRAME_Y_PAD;
}

void MM_Scrapbook_PlayMovie(struct RectMenu *menu)
{
	// NOTE(aalhendi): Native playback does not perform retail CD file lookup.
	// book state (0,1,2,3,4)
	switch (D230.scrapbookState)
	{
	// Init State,
	// alter checkered flag
	case SCRAP_INIT:
	{
		s32 stateValue;

		stateValue = SCRAP_LOAD;
		if (RaceFlag_IsFullyOnScreen() == stateValue)
		{
			// checkered flag, begin transition off-screen
			RaceFlag_BeginTransition(2);
		}

		// go to Load State
		D230.scrapbookState = stateValue;
		menu->state &= ~NEEDS_TO_CLOSE;
		Audio_SetState_Safe(1);
		break;
	}

	// find the TEST.STR file
	case SCRAP_LOAD:
	{
		s32 fullyOffscreen;

		// if not fully off screen
		fullyOffscreen = RaceFlag_IsFullyOffScreen();
		if (fullyOffscreen != 1)
		{
			// quit, dont start video yet
			return;
		}

		CDSYS_SetMode_StreamData();

		if (NativeSTR_StartScrapbook() != 0)
		{
			// NOTE(aalhendi): Native video decoding skips interleaved XA records;
			// play the Scrapbook CD-XA channel from the same raw STR file.
			if (NativeAudio_PlayXAFile(SCRAPBOOK_NATIVE_XA_PATH, SCRAPBOOK_NATIVE_XA_CHANNEL, sdata_static.vol_Music << 7, sdata_static.vol_Music << 7) == 0)
			{
				NativeSTR_Stop();
				goto GO_BACK;
			}
			s_scrapbookNativeNextVBlank = Platform_GetVBlankCount() + SCRAPBOOK_FRAME_VBLANKS;
			D230.scrapbookState = SCRAP_PLAY;
			return;
		}

		goto GO_BACK;
	}

	// Actually play the movie
	case SCRAP_PLAY:
	{
		s32 stateValue;
		struct GameTracker *gameTracker;
		u32 gameTrackerPage;
		s32 nativeUploaded;
		s16 nativeSrcX;
		s16 nativeSrcY;
		s16 nativeDisplayY;
		int getButtonPress;

		(void)stateValue;
		(void)gameTracker;
		(void)gameTrackerPage;
		getButtonPress = (MM_GAME_BUTTON_TAPS[0] & SCRAPBOOK_SKIP_INPUT);
		nativeUploaded = 0;


		MM_Scrapbook_GetNativeSource(&nativeSrcX, &nativeSrcY, &nativeDisplayY);
		if (getButtonPress == 0)
		{
			NativeRenderer_ClearVRAM(nativeSrcX, nativeDisplayY, SCRAPBOOK_NATIVE_DISPLAY_WIDTH, SCREEN_HEIGHT, 0, 0, 0);
			nativeUploaded = NativeSTR_UploadNextFrame(nativeSrcX, nativeSrcY);
		}

		if ((getButtonPress != 0) || (nativeUploaded == 0))
		{
			if ((MM_GAME_BUTTON_TAPS[0] & SCRAPBOOK_SKIP_INPUT) != 0)
			{
				RaceFlag_SetFullyOnScreen();
			}

			D230.scrapbookState = SCRAP_STOP;
		}
		else
		{
			Platform_PinVRAMDisplayRect4x3(nativeSrcX, nativeDisplayY, SCRAPBOOK_NATIVE_DISPLAY_WIDTH, SCREEN_HEIGHT, 1);
		}

		if ((getButtonPress == 0) && (nativeUploaded != 0))
		{
			// NOTE(aalhendi): Native decodes this frame on the CPU. Count any
			// elapsed decode vblanks toward the retail 15fps cadence instead of
			// adding them on top of a fresh VSync(4).
			Platform_WaitUntilVBlank(s_scrapbookNativeNextVBlank);
			s_scrapbookNativeNextVBlank += SCRAPBOOK_FRAME_VBLANKS;
		}
		else
		{
			VSync(SCRAPBOOK_FRAME_VBLANKS);
		}
		break;
	}

	// return disc to normal,
	// return checkered flag to normal
	case SCRAP_STOP:
		NativeAudio_StopXA();
		NativeSTR_Stop();

		if (RaceFlag_IsFullyOffScreen() == 1)
		{
			RaceFlag_BeginTransition(1);
		}
	GO_BACK:

		D230.scrapbookState = SCRAP_EXIT;
		break;

	case SCRAP_EXIT:
		if (RaceFlag_IsFullyOnScreen() == 1)
		{
			register s32 lev CTR_PSX_REGISTER("$4");

			// change checkered flag back
			RaceFlag_SetDrawOrder(0);

			if ((GAME_TRACKER->gameMode1 & ADVENTURE_MODE) != 0)
			{
				lev = GEM_STONE_VALLEY;
			}
			else
			{
				MM_JumpTo_Title_Returning();

				// return to main menu (adv, tt, arcade, vs, battle)
				sdata_static.mainMenuState = MAIN_MENU_TITLE;

				lev = MAIN_MENU_LEVEL;
			}

			MM_REQUEST_LEVEL(lev);

			RECTMENU_Hide(menu);
		}
		break;
	default:
		return;
	}
}
