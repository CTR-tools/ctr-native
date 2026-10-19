#include <common.h>

enum TrackSelectVideoState
{
	MM_TRACK_VIDEO_ICON = 1,
	MM_TRACK_VIDEO_START_STREAM = 2,
	MM_TRACK_VIDEO_PLAYING = 3,
};

enum
{
	MM_TRACK_VIDEO_PREVIEW_WAIT_FRAMES = 21,
	MM_TRACK_VIDEO_WIDTH = 0xb0,
	MM_TRACK_VIDEO_HEIGHT = 0x4b,
	MM_TRACK_VIDEO_FRAME_SRC_OFFSET_X = 3,
	MM_TRACK_VIDEO_FRAME_SRC_OFFSET_Y = 2,
	MM_TRACK_VIDEO_FRAME_WIDTH = 0xaa,
	MM_TRACK_VIDEO_FRAME_HEIGHT = 0x47,
	MM_TRACK_VIDEO_FLAGS = MM_VIDEO_FLAG_LOOP,
	MM_TRACK_VIDEO_ICON_INDEX = 0x3f,
	MM_TRACK_VIDEO_SCREEN_W = 0x200,
	MM_TRACK_VIDEO_SCREEN_H = 0xd8,
	MM_TRACK_SELECT_BATTLE_TRACK_COUNT = 7,
	MM_TRACK_SELECT_ARCADE_TRACK_COUNT = 18,
	MM_TRACK_SELECT_TRANSITION_FRAMES = 12,
	MM_TRACK_SELECT_SLIDE_FRAMES = 8,
	MM_TRACK_SELECT_TRACK_CHANGE_FRAMES = 3,
	MM_TRACK_SELECT_CENTER_ROW = 4,
	MM_TRACK_SELECT_VISIBLE_ROWS = 9,
	MM_TRACK_SELECT_ROW_ANGLE_STEP = 0x73,
	MM_TRACK_SELECT_ROW_W = 0x100,
	MM_TRACK_SELECT_ROW_H = 0x19,
	MM_TRACK_SELECT_ROW_X_RADIUS = 0x19,
	MM_TRACK_SELECT_ROW_X_SHIFT = 9,
	MM_TRACK_SELECT_ROW_X_OFFSET = -0xb4,
	MM_TRACK_SELECT_ROW_Y_RADIUS = 200,
	MM_TRACK_SELECT_ROW_Y_SHIFT = 0xc,
	MM_TRACK_SELECT_ROW_Y_OFFSET = 0x60,
	MM_TRACK_SELECT_TT_STAR_COUNT = 2,
	MM_TRACK_SELECT_TT_STAR_ICON_GROUP = 5,
	MM_TRACK_SELECT_TT_STAR_ICON = 0x37,
	MM_TRACK_SELECT_MAP_LAYER_COUNT = 6,
	MM_TRACK_SELECT_MAP_BOX_H = 100,
	MM_TRACK_SELECT_PREVIEW_X = 0x134,
	MM_TRACK_SELECT_PREVIEW_Y_WITH_MAP = 5,
	MM_TRACK_SELECT_PREVIEW_Y_NO_MAP = 0x3a,
	MM_TRACK_SELECT_LAP_MENU_WIDTH = 0xa4,
	MM_TRACK_SELECT_GHOST_TAPE_ALLOC_SIZE = 0x3e00,
	MM_TRACK_SELECT_GHOST_TAPE_CLEAR_SIZE = 0x28,
	MM_TRACK_SELECT_TIME_TRIAL_FLAGS_OFFSET = 0x12c,
	MM_TRACK_SELECT_ROW_PHASE_UNIT = 0x10000,
	MM_TRACK_SELECT_ROW_NAME_X_OFFSET = 8,
	MM_TRACK_SELECT_ROW_NAME_Y_OFFSET = 0x65,
	MM_TRACK_SELECT_STAR_X_OFFSET = 4,
	MM_TRACK_SELECT_STAR_Y_OFFSET = 4,
	MM_TRACK_SELECT_STAR_Y_STEP = 8,
	MM_TRACK_SELECT_GHOST_FLASH_FRAME_BIT = 4,
	MM_TRACK_SELECT_GHOST_TEXT_FROM_NAME_X = 0x78,
	MM_TRACK_SELECT_GHOST_TEXT_Y_OFFSET = 0x76,
	MM_TRACK_SELECT_HIGHLIGHT_INSET_X = 6,
	MM_TRACK_SELECT_HIGHLIGHT_INSET_Y = 4,
	MM_TRACK_SELECT_HIGHLIGHT_W_SHRINK = 12,
	MM_TRACK_SELECT_HIGHLIGHT_H_SHRINK = 8,
	MM_TRACK_SELECT_TITLE_X = 0x18c,
	MM_TRACK_SELECT_LEVEL_TEXT_Y_STEP = 0x10,
	MM_TRACK_SELECT_TITLE_TO_MAP_Y = 0x22,
	MM_TRACK_SELECT_MAP_CENTER_Y_OFFSET = 0x49,
	MM_TRACK_SELECT_INPUT = BTN_UP | BTN_DOWN | BTN_LEFT | BTN_RIGHT | BTN_TRIANGLE | BTN_SQUARE_one | BTN_CROSS_one | BTN_CIRCLE,
	MM_TRACK_SELECT_INPUT_CONFIRM = BTN_CROSS_one | BTN_CIRCLE,
	MM_TRACK_SELECT_INPUT_BACK = BTN_TRIANGLE | BTN_SQUARE_one,
};

#define MM_TRACK_GAME_TRACKER(page) CTR_PSX_PAGE_LVALUE(struct GameTracker *, page, MM_GAME_TRACKER_PAGE_OFFSET, GAME_TRACKER)

void MM_TrackSelect_Video_SetDefaults(void)
{
	u32 videoRectPage;
	s16 *videoRect;

	// track icon has been viewed for zero frames
	MM_TRACK_VIDEO_PREVIEW_FRAMES = 0;

	// Data is not allocated for TrackSel videos
	MM_TRACK_VIDEO_MEM_ALLOCATED = 0;

	MM_TRACK_VIDEO_STATE_CURR = MM_TRACK_VIDEO_ICON;
	MM_TRACK_VIDEO_STATE_PREV = MM_TRACK_VIDEO_ICON;

	// VRAM destination
	MM_VIDEO_STR_DST_X = 0;
	MM_VIDEO_STR_DST_Y = 0;

	// clear RECT
	CTR_PSX_LOAD_SYMBOL_PAGE(videoRectPage, MM_VIDEO_STR_SRC_RECT_ASM_NAME);
	CTR_PSX_PAGE_LVALUE(s16, videoRectPage, MM_VIDEO_STR_SRC_RECT_PAGE_OFFSET, MM_VIDEO_STR_SRC_X) = 0;
	CTR_PSX_ADD_SYMBOL_LOW(videoRect, videoRectPage, MM_VIDEO_STR_SRC_RECT_ASM_NAME, &sdata_static.videoSTR_src_vramRect.x);
	videoRect[1] = 0;
	videoRect[2] = 0;
	videoRect[3] = 0;
}

void MM_TrackSelect_Video_State(b32 resetPreview)
{
	// if viewing new icon this frame
	if (resetPreview == 1)
	{
		// player sees a track icon (not video)
		MM_TRACK_VIDEO_STATE_CURR = MM_TRACK_VIDEO_ICON;

		// icon has been viewed for zero frames
		MM_TRACK_VIDEO_PREVIEW_FRAMES = 0;

		return;
	}

	// if player sees a track icon
	if (MM_TRACK_VIDEO_STATE_CURR == MM_TRACK_VIDEO_ICON)
	{
		// wait before starting the preview video
		if (MM_TRACK_VIDEO_PREVIEW_FRAMES >= MM_TRACK_VIDEO_PREVIEW_WAIT_FRAMES)
		{
			// allocate video memory, prepare to play video
			MM_TRACK_VIDEO_STATE_CURR = MM_TRACK_VIDEO_START_STREAM;
		}
		else
		{
			MM_TRACK_VIDEO_PREVIEW_FRAMES++;
		}
	}
}

#ifdef CTR_NATIVE
#include <platform/native_str.h>

static void MM_TrackSelect_Video_DrawNativePreview(RECT *r, s32 srcX, s32 srcY)
{
	u32 *prim;
	u32 *ot;
	u32 oldTag;
	u32 *nextPrim;
	struct GameTracker *gGT = sdata_static.gGT;
	prim = (u32 *)gGT->backBuffer->primMem.cursor;
	ot = gGT->pushBuffer_UI.ptrOT;
	oldTag = (u32)*ot;
	struct DisplayBlurTile tile[2] = {
	    {
	        .srcX = (s16)srcX,
	        .srcY = (s16)srcY,
	        .srcW = MM_TRACK_VIDEO_FRAME_WIDTH,
	        .srcH = MM_TRACK_VIDEO_FRAME_HEIGHT,
	        .dstX = (s16)(r->x + MM_TRACK_VIDEO_FRAME_SRC_OFFSET_X),
	        .dstY = (s16)(r->y + MM_TRACK_VIDEO_FRAME_SRC_OFFSET_Y),
	        .dstW = MM_TRACK_VIDEO_FRAME_WIDTH,
	        .dstH = MM_TRACK_VIDEO_FRAME_HEIGHT,
	    },
	};

	// NOTE(aalhendi): Retail copies decoded MDEC output with MoveImage. Native
	// presents menus from queued primitives, so draw the same VRAM rectangle as
	// a 16-bit textured quad instead of relying on a CPU-side VRAM copy.
	*ot = (u32)CtrGpu_PrimToOTLink24(prim);
	nextPrim = DISPLAY_Blur_SubFunc(prim, &tile[0]);
	((POLY_FT4 *)nextPrim - 1)->tag = CtrGpu_PackOTTag(oldTag, 0x09000000);
	gGT->backBuffer->primMem.cursor = nextPrim;
}
#endif

void MM_TrackSelect_Video_Draw(RECT *r, struct MainMenu_LevelRow *selectMenu, s16 trackIndex, s32 stopVideo, u16 rectFlags)
{
	struct BigHeader *bigfileHeader;
	struct BigEntry *bigfileEntries;
	struct MainMenu_LevelRow *selectedMenu;
	s32 videoFileIndex;

	bigfileHeader = MM_BIGFILE_HEADER;
	bigfileEntries = BIG_GETENTRY(bigfileHeader);
	selectedMenu = (struct MainMenu_LevelRow *)((u32)(trackIndex * sizeof(*selectedMenu)) + (u32)selectMenu);
	videoFileIndex = selectedMenu->previewVideoFileIndex;

	if ((bigfileEntries[videoFileIndex].size != 0) && (r->x >= 0) && (r->y >= 0) && ((r->x + r->w) < (MM_TRACK_VIDEO_SCREEN_W + 1)) &&
	    ((r->y + r->h) < (MM_TRACK_VIDEO_SCREEN_H + 1)))
#ifdef CTR_NATIVE
	{
		if (MM_TRACK_VIDEO_STATE_CURR == MM_TRACK_VIDEO_START_STREAM)
		{
			int previousVideoState = MM_TRACK_VIDEO_STATE_PREV;

			if (previousVideoState == MM_TRACK_VIDEO_ICON)
			{
				if (NativeSTR_StartTrackPreviewFromBigfileSector((BIG_GETENTRY(MM_BIGFILE_HEADER))[selectedMenu->previewVideoFileIndex].offset,
				                                                 selectedMenu->previewVideoFrameCount) != 0)
				{
					MM_TRACK_VIDEO_MEM_ALLOCATED = previousVideoState;
				}
			}
		}

		if (((MM_TRACK_VIDEO_STATE_PREV == MM_TRACK_VIDEO_PLAYING) || (MM_TRACK_VIDEO_STATE_CURR == MM_TRACK_VIDEO_PLAYING)) ||
		    (MM_TRACK_VIDEO_STATE_CURR == MM_TRACK_VIDEO_START_STREAM))
		{
			u8 u0;
			u8 v0;
			int srcX;
			int srcY;
			int uploaded;
			u16 tpage = GAME_TRACKER->ptrIcons[MM_TRACK_VIDEO_ICON_INDEX]->texLayout.tpage;
			u0 = GAME_TRACKER->ptrIcons[MM_TRACK_VIDEO_ICON_INDEX]->texLayout.u0;
			v0 = GAME_TRACKER->ptrIcons[MM_TRACK_VIDEO_ICON_INDEX]->texLayout.v0;
			srcX = u0 + (tpage & 0xf) * 0x40;
			srcY = v0 + ((tpage & 0x10) * 0x10 + ((tpage & 0x800) >> 2));
			uploaded = NativeSTR_UploadNextFrame(srcX, srcY);

			if ((uploaded == 1) && (MM_TRACK_VIDEO_STATE_CURR == MM_TRACK_VIDEO_START_STREAM))
			{
				MM_TRACK_VIDEO_STATE_CURR = MM_TRACK_VIDEO_PLAYING;
			}

			if (MM_TRACK_VIDEO_STATE_CURR == MM_TRACK_VIDEO_PLAYING)
			{
				MM_TrackSelect_Video_DrawNativePreview(r, srcX + MM_TRACK_VIDEO_FRAME_SRC_OFFSET_X, srcY + MM_TRACK_VIDEO_FRAME_SRC_OFFSET_Y);
			}
		}
	}
#else
	{
		if (MM_TRACK_VIDEO_STATE_CURR == MM_TRACK_VIDEO_START_STREAM)
		{
			int previousVideoState = MM_TRACK_VIDEO_STATE_PREV;

			if (previousVideoState == MM_TRACK_VIDEO_ICON)
			{
				// If you have not allocated memory for video yet
				if (MM_TRACK_VIDEO_MEM_ALLOCATED == 0)
				{
					// Allocate memory for video in Track Selection
					MM_Video_AllocMem(MM_TRACK_VIDEO_WIDTH, MM_TRACK_VIDEO_HEIGHT, MM_TRACK_VIDEO_FLAGS, 0, 0);

					// You have now allocated the memory
					MM_TRACK_VIDEO_MEM_ALLOCATED = previousVideoState;
				}

				// CD position of video, and numFrames
				{
					struct BigHeader *streamHeader;
					struct BigEntry *streamEntries;
					s32 streamFileIndex;

					streamHeader = MM_BIGFILE_HEADER;
					streamEntries = BIG_GETENTRY(streamHeader);
					streamFileIndex = selectedMenu->previewVideoFileIndex;
					MM_Video_StartStream(streamHeader->cdpos + streamEntries[streamFileIndex].offset, selectedMenu->previewVideoFrameCount);
				}
			}
		}

		if (((MM_TRACK_VIDEO_STATE_PREV == MM_TRACK_VIDEO_PLAYING) || (MM_TRACK_VIDEO_STATE_CURR == MM_TRACK_VIDEO_PLAYING)) ||
		    (MM_TRACK_VIDEO_STATE_CURR == MM_TRACK_VIDEO_START_STREAM))
		{
			u8 u0;
			u8 v0;
			s32 decodeX;
			s32 decodeY;
			register s32 srcX CTR_PSX_REGISTER("$16");
			register s32 srcY CTR_PSX_REGISTER("$17");
			RECT *videoRect;
			s16 displayX;
			s16 displayY;
			s16 dstX;
			s16 dstY;
			b32 decodedFrame;
			u32 tpage = GAME_TRACKER->ptrIcons[MM_TRACK_VIDEO_ICON_INDEX]->texLayout.tpage;
			u0 = GAME_TRACKER->ptrIcons[MM_TRACK_VIDEO_ICON_INDEX]->texLayout.u0;
			v0 = GAME_TRACKER->ptrIcons[MM_TRACK_VIDEO_ICON_INDEX]->texLayout.v0;
			// Decode into the icon's VRAM page; the copied rectangle starts inside it.
			decodeX = u0 + (tpage & 0xf) * 0x40;
			decodeY = v0 + (((tpage & 0x10) * 0x10) + ((tpage & 0x800) >> 2));
			srcX = decodeX;
			srcY = decodeY;
			decodedFrame = MM_Video_DecodeFrame(decodeX, decodeY);

			if ((decodedFrame == 1) && (MM_TRACK_VIDEO_STATE_CURR == MM_TRACK_VIDEO_START_STREAM))
			{
				MM_TRACK_VIDEO_STATE_CURR = MM_TRACK_VIDEO_PLAYING;
			}

			if (MM_TRACK_VIDEO_STATE_PREV == MM_TRACK_VIDEO_PLAYING)
			{
				// RECT position (x,y)
				videoRect = &sdata_static.videoSTR_src_vramRect;
				videoRect->x = srcX + MM_TRACK_VIDEO_FRAME_SRC_OFFSET_X;
				CTR_PSX_OBSERVE_VALUE(srcX);
				videoRect->y = srcY + MM_TRACK_VIDEO_FRAME_SRC_OFFSET_Y;
				CTR_PSX_OBSERVE_VALUE(srcY);

				// RECT size (w,h)
				videoRect->w = MM_TRACK_VIDEO_FRAME_WIDTH;
				videoRect->h = MM_TRACK_VIDEO_FRAME_HEIGHT;

				// VRAM destination (x,y) on swapchain image
				displayX = ((volatile struct GameTracker *)GAME_TRACKER)->db[GAME_TRACKER->swapchainIndex].dispEnv.disp.x;
				dstX = (displayX + ((volatile u16 *)r)[0]) + MM_TRACK_VIDEO_FRAME_SRC_OFFSET_X;
				MM_VIDEO_STR_DST_X = dstX;
				displayY = ((volatile struct GameTracker *)GAME_TRACKER)->db[GAME_TRACKER->swapchainIndex].dispEnv.disp.y;
				dstY = (displayY + ((volatile u16 *)r)[1]) + MM_TRACK_VIDEO_FRAME_SRC_OFFSET_Y;
				MM_VIDEO_STR_DST_Y = dstY;

				// enable video copy, give src and dst
				MainFrame_InitVideoSTR(1, videoRect, dstX, dstY);
			}
		}
	}
#endif

	else
	{
		// draw icon
		MM_TRACK_VIDEO_STATE_CURR = MM_TRACK_VIDEO_ICON;
	}

	// if not playing video, draw icon
	if (MM_TRACK_VIDEO_STATE_CURR != MM_TRACK_VIDEO_PLAYING)
	{
		// Draw Video icon
		MM_RECTMENU_DRAW_POLY_GT4(GAME_TRACKER->ptrIcons[selectMenu[trackIndex].videoThumbnail], (r->x + MM_TRACK_VIDEO_FRAME_SRC_OFFSET_X),
		                          (r->y + MM_TRACK_VIDEO_FRAME_SRC_OFFSET_Y), &GAME_TRACKER->backBuffer->primMem, GAME_TRACKER->pushBuffer_UI.ptrOT,
		                          MM_VIDEO_COLOR, MM_VIDEO_COLOR, MM_VIDEO_COLOR, MM_VIDEO_COLOR, 0, FP(1.0));
	}

#ifndef CTR_NATIVE
	if (MM_TRACK_VIDEO_STATE_PREV == MM_TRACK_VIDEO_ICON)
	{
		// disable video copy
		MainFrame_InitVideoSTR(0, 0, 0, 0);
	}

	// First frame to start video
	if ((stopVideo == 1) && (MM_TRACK_VIDEO_MEM_ALLOCATED == 1))
	{
		MM_TRACK_VIDEO_STATE_CURR = MM_TRACK_VIDEO_ICON;
	}

	// First frame to stop video
	if ((MM_TRACK_VIDEO_STATE_CURR == MM_TRACK_VIDEO_ICON) && (MM_TRACK_VIDEO_STATE_PREV != MM_TRACK_VIDEO_ICON))
	{
		MM_Video_StopStream();
	}

	// First frame to start video,
	// but this time after stopping video safely
	if ((stopVideo == 1) && (MM_TRACK_VIDEO_MEM_ALLOCATED == 1))
	{
		MM_Video_ClearMem();

		MM_TRACK_VIDEO_MEM_ALLOCATED = 0;
	}
#else
	if (MM_TRACK_VIDEO_STATE_PREV == MM_TRACK_VIDEO_ICON)
	{
		MainFrame_InitVideoSTR(0, 0, 0, 0);
	}

	if ((stopVideo == 1) && (MM_TRACK_VIDEO_MEM_ALLOCATED == 1))
	{
		MM_TRACK_VIDEO_STATE_CURR = MM_TRACK_VIDEO_ICON;
	}

	if ((MM_TRACK_VIDEO_STATE_CURR == MM_TRACK_VIDEO_ICON) && (MM_TRACK_VIDEO_STATE_PREV != MM_TRACK_VIDEO_ICON))
	{
		NativeSTR_Stop();
	}

	if ((stopVideo == 1) && (MM_TRACK_VIDEO_MEM_ALLOCATED == 1))
	{
		NativeSTR_Stop();
		MM_TRACK_VIDEO_MEM_ALLOCATED = 0;
	}
#endif

	MM_TRACK_VIDEO_STATE_PREV = MM_TRACK_VIDEO_STATE_CURR;

	// Draw 2D Menu rectangle background
	RECTMENU_DrawInnerRect(r, (s16)(rectFlags | 1), GAME_TRACKER->backBuffer->otMem.uiOT);
}

b16 MM_TrackSelect_boolTrackOpen(struct MainMenu_LevelRow *menuSelect)
{
	s16 flag;
	b16 unlocked;

	flag = menuSelect->unlock;
	unlocked = false;

	if ((flag == MM_TRACK_UNLOCK_ALWAYS) || ((flag == MM_TRACK_UNLOCK_1P_ONLY) && (GAME_TRACKER->numPlyrNextGame == 1)) ||
	    ((flag >= 0) && CHECK_ADV_BIT(GAME_PROGRESS.unlocks, flag)))
	{
		unlocked = true;
	}

	return unlocked;
}

void MM_TrackSelect_Init(void)
{
	struct MainMenu_LevelRow *selectMenu;
	s16 numTracks;

	// lap selection menu is closed by default
	MM_TRACK_LAP_BOX_OPEN = false;
	MM_TRACK_TRANSITION_STATE = ENTERING_MENU;

	// set track index to the index selected in track selection menu, starts at 0 for both Arcade and Battle
	MM_MENU_TRACK_SELECT.rowSelected = MM_TRACK_SEL_BACKUP;

	// 12 frames when moving between selection
	MM_TRACK_TRANSITION_FRAME = MM_TRACK_SELECT_TRANSITION_FRAMES;

	// Set menu and num of tracks based on game mode
	if ((sdata_static.gGT->gameMode1 & BATTLE_MODE) != 0)
	{
		selectMenu = MM_BATTLE_TRACKS;
		numTracks = MM_TRACK_SELECT_BATTLE_TRACK_COUNT;
	}
	else
	{
		selectMenu = MM_ARCADE_TRACKS;
		numTracks = MM_TRACK_SELECT_ARCADE_TRACK_COUNT;
	}

	// If you scroll past the max number of tracks, go back to the first track
	if (numTracks <= MM_MENU_TRACK_SELECT.rowSelected)
	{
		MM_MENU_TRACK_SELECT.rowSelected = 0;
	}

	// Loop through all tracks until an unlocked track is found
	while (!MM_TrackSelect_boolTrackOpen(&selectMenu[MM_MENU_TRACK_SELECT.rowSelected]))
	{
		MM_MENU_TRACK_SELECT.rowSelected++;

		// If track index goes too high, reset to zero
		if (numTracks <= MM_MENU_TRACK_SELECT.rowSelected)
		{
			MM_MENU_TRACK_SELECT.rowSelected = 0;
		}
	}

	MM_TRACK_CURRENT_TRACK = MM_MENU_TRACK_SELECT.rowSelected;

	MM_TrackSelect_Video_SetDefaults();
}

// NOTE(aalhendi): GCC 2.8.1 is sensitive to the lifetimes in this menu. The
// CTR_NATIVE branches state the same operations without relying on PSX pages
// or MIPS register contents.
// MultiTrack: retail menu, replaced by game/MultiTrack/MT_TrackSelect.c
void MM_TrackSelect_MenuProc_Retail(struct RectMenu *menu)
{
	struct RectMenu *menuLocal;
	struct MainMenu_LevelRow *selectMenu;
	s16 currTrack;
	u16 numTracks;
	u32 trackCountWork;
	register s32 trackChangeFrames CTR_PSX_REGISTER("$2");
	register struct TransitionMeta *levelTitleTransition CTR_PSX_REGISTER("$16");
	struct PushBuffer *uiPushBuffer;
	register b32 resetPreviewVideo CTR_PSX_REGISTER("$4");
	s16 rowIndex;
	(void)trackCountWork;
	menuLocal = menu;
	switch (MM_TRACK_TRANSITION_STATE)
	{
	case ENTERING_MENU:
	{
		s32 errorMessagePosIndex = 1;
		register struct TransitionMeta *enteringTransitions CTR_PSX_REGISTER("$4");
		register u32 enteringFramePage CTR_PSX_REGISTER("$16");
		register s32 enteringFrame CTR_PSX_REGISTER("$5");
		register s32 slideFrames CTR_PSX_REGISTER("$6");
		if ((GAME_TRACKER->gameMode1 & BATTLE_MODE) != 0)
		{
			errorMessagePosIndex = 2;
		}
		MM_ERROR_MESSAGE_POS_INDEX = errorMessagePosIndex;
		enteringTransitions = MM_TRACK_TRANSITIONS;
#ifdef CTR_NATIVE
		enteringFramePage = 0;
		enteringFrame = MM_TRACK_TRANSITION_FRAME;
#else
		asm("lui %0,%%hi(" MM_TRACK_TRANSITION_FRAME_ASM_NAME ")" : "=r"(enteringFramePage) : "r"(enteringTransitions));
#endif
		slideFrames = MM_TRACK_SELECT_SLIDE_FRAMES;
#ifndef CTR_NATIVE
		asm("lh %0,%%lo(" MM_TRACK_TRANSITION_FRAME_ASM_NAME ")(%1)" : "=r"(enteringFrame) : "r"(enteringFramePage), "r"(slideFrames));
#endif
		MM_TransitionInOut(enteringTransitions, enteringFrame, slideFrames);
		if (CTR_PSX_PAGE_LVALUE(s16, enteringFramePage, MM_TRACK_TRANSITION_FRAME_PAGE_OFFSET, MM_TRACK_TRANSITION_FRAME) != 0)
		{
			CTR_PSX_PAGE_LVALUE(s16, enteringFramePage, MM_TRACK_TRANSITION_FRAME_PAGE_OFFSET, MM_TRACK_TRANSITION_FRAME)--;
		}
		else
		{
			MM_TRACK_TRANSITION_STATE = IN_MENU;
		}
		break;
	}

	case IN_MENU:
		break;

	case EXITING_MENU:
	{
		MM_TransitionInOut(MM_TRACK_TRANSITIONS, MM_TRACK_TRANSITION_FRAME, MM_TRACK_SELECT_SLIDE_FRAMES);
		MM_TRACK_TRANSITION_FRAME++;
		if (MM_TRACK_TRANSITION_FRAME > MM_TRACK_SELECT_TRANSITION_FRAMES)
		{
			MM_ERROR_MESSAGE_POS_INDEX = 0;
			if (MM_TRACK_TRANSITION_START_AFTER_EXIT != 0)
			{
				if ((GAME_TRACKER->gameMode1 & BATTLE_MODE) != 0)
				{
					MM_DESIRED_MENU = &MM_MENU_BATTLE_WEAPONS;
					MM_Battle_Init();
					return;
				}
				if ((GAME_TRACKER->gameMode1 & TIME_TRIAL) != 0)
				{
					MM_GHOST_TAPE_PLAYING = MM_ALLOC_HIGH_MEM(MM_TRACK_SELECT_GHOST_TAPE_ALLOC_SIZE, MM_LOADED_GHOST_DATA);
					memset(MM_GHOST_TAPE_PLAYING, 0, MM_TRACK_SELECT_GHOST_TAPE_CLEAR_SIZE);
					MM_REPLAY_HUMAN_GHOST = 0;
					SelectProfile_ToggleMode(SELECT_PROFILE_SCREEN_GHOST);
					MM_DESIRED_MENU = &MM_MENU_GHOST_SELECTION;
					return;
				}
				MM_DESIRED_MENU = &MM_MENU_QUEUE_LOAD_TRACK;
				return;
			}
			MM_DESIRED_MENU = &MM_MENU_CHARACTER_SELECT;
			MM_Characters_RestoreIDs();
			return;
		}
		break;
	}

	default:
		break;
	}

#ifndef CTR_NATIVE
	asm("" : "+m"(menuLocal));
	asm("" : "=m"(trackCountWork));
#endif

	// Battle and race modes use different track tables and row counts.
	if ((GAME_TRACKER->gameMode1 & BATTLE_MODE) != 0)
	{
		register u32 trackTablePage CTR_PSX_REGISTER("$2");
		register s16 trackCountValue CTR_PSX_REGISTER("$12");
		trackTablePage = MM_OVERLAY_PAGE_VALUE;
#ifdef CTR_NATIVE
		selectMenu = MM_BATTLE_TRACKS;
		(void)trackTablePage;
#else
		CTR_PSX_KEEP_VALUE(trackTablePage);
		selectMenu = (struct MainMenu_LevelRow *)(trackTablePage + MM_BATTLE_TRACKS_PAGE_OFFSET);
#endif
		trackCountValue = MM_TRACK_SELECT_BATTLE_TRACK_COUNT;
		CTR_PSX_KEEP_VALUE_RELAXED(trackCountValue);
		numTracks = trackCountValue;
	}
	else
	{
		register u32 trackTablePage CTR_PSX_REGISTER("$2");
		register s16 trackCountValue CTR_PSX_REGISTER("$13");
#ifdef CTR_NATIVE
		trackTablePage = 0;
		selectMenu = MM_ARCADE_TRACKS;
		(void)trackTablePage;
#else
		asm("lui %0," MM_OVERLAY_PAGE_UPPER_ASM : "=r"(trackTablePage));
		selectMenu = (struct MainMenu_LevelRow *)(trackTablePage + MM_ARCADE_TRACKS_PAGE_OFFSET);
#endif
		currTrack = MM_TRACK_SELECT_ARCADE_TRACK_COUNT;
		trackCountValue = currTrack;
		CTR_PSX_KEEP_VALUE_RELAXED(trackCountValue);
		numTracks = trackCountValue;
	}
	{
		register u32 trackStatePage CTR_PSX_REGISTER("$2");
		register struct RectMenu *menuView CTR_PSX_REGISTER("$14");
		register s32 trackChangeValue CTR_PSX_REGISTER("$3");
#ifdef CTR_NATIVE
		trackStatePage = 0;
		trackChangeValue = MM_TRACK_CHANGE_FRAMES;
#else
		asm("lui %0,%%hi(" MM_TRACK_CHANGE_FRAMES_ASM_NAME ")" : "=r"(trackStatePage));
#endif
		menuView = menuLocal;
#ifndef CTR_NATIVE
		asm("" : "+r"(menuView) : "r"(trackStatePage));
		asm volatile("lw %0,%%lo(" MM_TRACK_CHANGE_FRAMES_ASM_NAME ")(%1)" : "=r"(trackChangeValue) : "r"(trackStatePage), "r"(menuView));
#endif
		currTrack = menuView->rowSelected;
		MM_TRACK_SEL_BACKUP = currTrack;
		// Ignore navigation while the selected row is still moving.
		if (trackChangeValue == 0)
		{
			register u32 lapStatePage CTR_PSX_REGISTER("$19");
			lapStatePage = MM_OVERLAY_PAGE_VALUE;
			CTR_PSX_KEEP_VALUE(lapStatePage);
			if (CTR_PSX_PAGE_LVALUE(s16, lapStatePage, MM_TRACK_LAP_BOX_OPEN_PAGE_OFFSET, MM_TRACK_LAP_BOX_OPEN) == 0)
			{
				register u32 transitionStatePage CTR_PSX_REGISTER("$18");
				register s32 transitionState CTR_PSX_REGISTER("$16");
				transitionStatePage = MM_OVERLAY_PAGE_VALUE;
				CTR_PSX_KEEP_VALUE(transitionStatePage);
				transitionState = CTR_PSX_PAGE_LVALUE(s16, transitionStatePage, MM_TRACK_TRANSITION_STATE_PAGE_OFFSET, MM_TRACK_TRANSITION_STATE);
				if (transitionState == IN_MENU)
				{
					register s32 buttonTap CTR_PSX_REGISTER("$4");
					register u32 inputMask CTR_PSX_REGISTER("$3");
					inputMask = MM_OVERLAY_PAGE_VALUE;
					CTR_PSX_KEEP_VALUE(inputMask);
					inputMask = MM_TRACK_SELECT_INPUT;
					buttonTap = MM_GAME_BUTTON_TAPS[0];
					{
						u32 downPressed = buttonTap & BTN_DOWN;
						if ((buttonTap & inputMask) != 0)
						{
							CTR_PSX_KEEP_VALUE(transitionState);
							do
							{
								if (downPressed != 0)
								{
									register u32 wrapTrackCountLoad CTR_PSX_REGISTER("$15");
									register s32 wrapTrackCount CTR_PSX_REGISTER("$16");
									OtherFX_Play(0, 1);
									wrapTrackCountLoad = numTracks;
									CTR_PSX_KEEP_VALUE_RELAXED(wrapTrackCountLoad);
									wrapTrackCount = wrapTrackCountLoad;
									CTR_PSX_KEEP_VALUE_RELAXED(wrapTrackCount);
									do
									{
										currTrack++;
										if (currTrack >= wrapTrackCount)
										{
											currTrack = 0;
										}
									} while (!MM_TrackSelect_boolTrackOpen(&selectMenu[currTrack]));
									MM_TRACK_CHANGE_FRAMES = MM_TRACK_SELECT_TRACK_CHANGE_FRAMES;
									MM_TRACK_CHANGE_DIRECTION = -1;
									MM_TRACK_CURRENT_TRACK = currTrack;
									break;
								}
								else if ((buttonTap & BTN_UP) != 0)
								{
									register u32 wrapTrackCount CTR_PSX_REGISTER("$12");
									register s32 wrapLastTrack CTR_PSX_REGISTER("$16");
									OtherFX_Play(0, 1);
									wrapTrackCount = numTracks;
									CTR_PSX_KEEP_VALUE_RELAXED(wrapTrackCount);
									wrapLastTrack = wrapTrackCount - 1;
									CTR_PSX_KEEP_VALUE_RELAXED(wrapLastTrack);
									do
									{
										currTrack--;
										if (currTrack < 0)
										{
											currTrack = wrapLastTrack;
										}
									} while (!MM_TrackSelect_boolTrackOpen(&selectMenu[currTrack]));
									MM_TRACK_CHANGE_FRAMES = MM_TRACK_SELECT_TRACK_CHANGE_FRAMES;
									MM_TRACK_CHANGE_DIRECTION = 1;
									MM_TRACK_CURRENT_TRACK = currTrack;
									break;
								}
								else if ((buttonTap & MM_TRACK_SELECT_INPUT_CONFIRM) != 0)
								{
									OtherFX_Play(1, 1);
									{
										u32 gameMode = GAME_TRACKER->gameMode1;
										if ((gameMode & BATTLE_MODE) != 0)
										{
											register u32 startAfterPage CTR_PSX_REGISTER("$2");
											startAfterPage = MM_OVERLAY_PAGE_VALUE;
											CTR_PSX_KEEP_VALUE(startAfterPage);
											CTR_PSX_PAGE_LVALUE(s16, startAfterPage, MM_TRACK_TRANSITION_START_AFTER_EXIT_PAGE_OFFSET,
											                    MM_TRACK_TRANSITION_START_AFTER_EXIT) = transitionState;
										}
										else if ((gameMode & TIME_TRIAL) != 0)
										{
											MM_TRACK_TRANSITION_START_AFTER_EXIT = transitionState;
										}
										else
										{
											CTR_PSX_PAGE_LVALUE(s16, lapStatePage, MM_TRACK_LAP_BOX_OPEN_PAGE_OFFSET, MM_TRACK_LAP_BOX_OPEN) = transitionState;
											break;
										}
									}
								}
								else
								{
									register u32 backPressed CTR_PSX_REGISTER("$2");
									backPressed = buttonTap & MM_TRACK_SELECT_INPUT_BACK;
									CTR_PSX_KEEP_VALUE_RELAXED(backPressed);
									if (backPressed == 0)
									{
										goto trackClearInput;
									}
									OtherFX_Play(2, 1);
									MM_TRACK_TRANSITION_START_AFTER_EXIT = 0;
								}
								CTR_PSX_PAGE_LVALUE(s16, transitionStatePage, MM_TRACK_TRANSITION_STATE_PAGE_OFFSET, MM_TRACK_TRANSITION_STATE) = EXITING_MENU;
							} while (0);
						trackClearInput:
							RECTMENU_ClearInput();
							goto trackSelectionMenuDone;
						}
						else
						{
							goto trackSelectionMenuDone;
						}
					}
				}
				else
				{
					goto trackSelectionMenuDone;
				}
			}
			else
			{
				goto trackLapMenu;
			}
		}
	}
	// The lap menu is drawn on top of Track Select when it is open.
	{
		register u32 lapReloadPage CTR_PSX_REGISTER("$2");
#ifdef CTR_NATIVE
		lapReloadPage = 0;
#else
		asm(".set\tnoreorder\n\t"
		    ".set\tnomacro\n\t"
		    "lui %0," MM_OVERLAY_PAGE_UPPER_ASM "\n\t"
		    "lh %0," MM_TRACK_LAP_BOX_OPEN_PAGE_OFFSET_ASM "(%0)\n\t"
		    "nop\n\t"
		    "beq %0,$0,$LPSXTrackSelectionDone\n\t"
		    "lui $3," MM_OVERLAY_PAGE_UPPER_ASM "\n\t"
		    ".set\tmacro\n\t"
		    ".set\treorder\n\t"
		    ".if 0"
		    : "=r"(lapReloadPage));
#endif
		if (CTR_PSX_PAGE_LVALUE(s16, lapReloadPage, MM_TRACK_LAP_BOX_OPEN_PAGE_OFFSET, MM_TRACK_LAP_BOX_OPEN) != 0)
		{
			s16 lapSelTransitionState;
			{
				register u32 lapMenuPage CTR_PSX_REGISTER("$3");
				lapMenuPage = MM_OVERLAY_PAGE_VALUE;
#ifdef CTR_NATIVE
				CTR_PSX_KEEP_VALUE(lapMenuPage);
#else
				asm volatile(".endif" : "+r"(lapMenuPage));
#endif
			}
		trackLapMenu:
			lapSelTransitionState = 0;
			MM_MENU_LAP_SELECT.rowSelected = MM_USELESS_LAP_ROW_COPY;
			if (MM_TRACK_TRANSITION_STATE == IN_MENU)
			{
				lapSelTransitionState = RECTMENU_ProcessInput(&MM_MENU_LAP_SELECT);
			}
			RECTMENU_DrawSelf(&MM_MENU_LAP_SELECT, MM_TRACK_TRANSITIONS[2].currX, MM_TRACK_TRANSITIONS[2].currY, MM_TRACK_SELECT_LAP_MENU_WIDTH);
			MM_USELESS_LAP_ROW_COPY = MM_MENU_LAP_SELECT.rowSelected;
			GAME_TRACKER->numLaps = MM_LAP_COUNT_BY_ROW[MM_MENU_LAP_SELECT.rowSelected].lapCount;
			{
				s32 lapResult;
				lapResult = (s16)lapSelTransitionState;
				if (lapResult == 1)
				{
					MM_TRACK_TRANSITION_START_AFTER_EXIT = lapResult;
					MM_TRACK_TRANSITION_STATE = EXITING_MENU;
				}
				else if (lapResult == (-1))
				{
					MM_TRACK_LAP_BOX_OPEN = 0;
				}
			}
			if ((GAME_TRACKER->gameMode2 & CHEAT_ONELAP) != 0)
			{
				GAME_TRACKER->numLaps = 1;
			}
		}
	}
trackSelectionMenuDone:
	// Finish a row transition by publishing the selected track to the menu.
	{
		register u32 trackChangePage CTR_PSX_REGISTER("$3");
		trackChangePage = MM_OVERLAY_PAGE_VALUE;
#ifdef CTR_NATIVE
		CTR_PSX_KEEP_VALUE(trackChangePage);
#else
		asm volatile("$LPSXTrackSelectionDone:" : "+r"(trackChangePage));
#endif
		trackChangeFrames = CTR_PSX_PAGE_LVALUE(s32, trackChangePage, MM_TRACK_CHANGE_FRAMES_PAGE_OFFSET, MM_TRACK_CHANGE_FRAMES);
		if (0 < trackChangeFrames)
		{
			trackChangeFrames--;
			CTR_PSX_PAGE_LVALUE(s32, trackChangePage, MM_TRACK_CHANGE_FRAMES_PAGE_OFFSET, MM_TRACK_CHANGE_FRAMES) = trackChangeFrames;
			if (trackChangeFrames == 0)
			{
				register u32 currentTrack CTR_PSX_REGISTER("$2");
				register struct RectMenu *menuUpdate CTR_PSX_REGISTER("$13");
#ifdef CTR_NATIVE
				currentTrack = MM_TRACK_CURRENT_TRACK;
#else
				asm("lui %0,%%hi(" MM_TRACK_CURRENT_TRACK_ASM_NAME ")" : "=r"(currentTrack));
				asm volatile("lhu %0,%%lo(" MM_TRACK_CURRENT_TRACK_ASM_NAME ")(%0)" : "+r"(currentTrack));
#endif
				menuUpdate = menuLocal;
#ifndef CTR_NATIVE
				asm volatile("" : "+r"(menuUpdate) : "r"(currentTrack));
#endif
				menuUpdate->rowSelected = currentTrack;
			}
		}
	}

	resetPreviewVideo = 0;
	if ((MM_TRACK_CHANGE_FRAMES != 0) || (MM_TRACK_TRANSITION_STATE == EXITING_MENU))
	{
		resetPreviewVideo = 1;
	}
	rowIndex = 0;
	MM_TrackSelect_Video_State(resetPreviewVideo);
	// Find the first visible row before drawing the nine-row carousel.
	{
		register s16 lastTrack CTR_PSX_REGISTER("$16");
		register s16 nextRowIndex CTR_PSX_REGISTER("$2");
		register s16 trackCount CTR_PSX_REGISTER("$14");
		register struct RectMenu *scanMenu CTR_PSX_REGISTER("$15");
		register struct GameTracker *scanTracker CTR_PSX_REGISTER("$3");
		register s32 selectedTrack CTR_PSX_REGISTER("$4");
#ifdef CTR_NATIVE
		scanTracker = GAME_TRACKER;
#else
		asm("lui %0,%%hi(" RETAIL_GAME_TRACKER_ASM_NAME ")" : "=r"(scanTracker));
#endif
		trackCount = numTracks;
#ifndef CTR_NATIVE
		asm("" : "+r"(trackCount) : "r"(scanTracker));
#endif
		scanMenu = menuLocal;
#ifndef CTR_NATIVE
		asm("" : "+r"(scanMenu) : "r"(trackCount));
		asm volatile("lw %0,%%lo(" RETAIL_GAME_TRACKER_ASM_NAME ")(%0)" : "+r"(scanTracker) : "r"(scanMenu));
#endif
		selectedTrack = (s16)scanMenu->rowSelected;
#ifdef CTR_NATIVE
		lastTrack = trackCount - 1;
#else
		asm("addiu %0,%1,-1" : "=r"(lastTrack) : "r"(trackCount), "r"(selectedTrack));
#endif
		scanTracker->currLEV = selectMenu[selectedTrack].levID;
		currTrack = selectedTrack;
		do
		{
			b32 trackOpen;
			do
			{
				currTrack--;
				if (currTrack < 0)
				{
					currTrack = lastTrack;
				}
				trackOpen = MM_TrackSelect_boolTrackOpen(&selectMenu[currTrack]);
			} while (!trackOpen);
			nextRowIndex = rowIndex + 1;
			rowIndex = nextRowIndex;
		} while (nextRowIndex < MM_TRACK_SELECT_CENTER_ROW);
	}
	rowIndex = 0;
	{
		register u32 gameTrackerPage CTR_PSX_REGISTER("$23");
#ifdef CTR_NATIVE
		gameTrackerPage = 0;
#else
		asm("lui %0,%%hi(" RETAIL_GAME_TRACKER_ASM_NAME ")" : "=r"(gameTrackerPage));
#endif
		// Draw each visible row, wrapping past locked and final tracks.
		do
		{
			register s32 rowBaseY CTR_PSX_REGISTER("$3");
			register s16 rowY CTR_PSX_REGISTER("$20");
			b32 trackOpen;
			register u32 rowAngle CTR_PSX_REGISTER("$4");
			register u32 packedTrig CTR_PSX_REGISTER("$16");
			register struct TrigTable *trigTable CTR_PSX_REGISTER("$12");
			register u32 trigIndex CTR_PSX_REGISTER("$2");
			s32 rowCos;
			s32 rowSin;
			RECT16 rowRect;
			RECT16 *rowDrawRectArg;
			register s32 rowX CTR_PSX_REGISTER("$19");
			register s32 rowBaseAngle CTR_PSX_REGISTER("$5");
			register s32 rowBaseIndex CTR_PSX_REGISTER("$2");
			register s32 rowPhaseTemp CTR_PSX_REGISTER("$3");
			register s32 changeFrames CTR_PSX_REGISTER("$2");
			register s32 rowCalc CTR_PSX_REGISTER("$2");
			register s32 rowBaseX CTR_PSX_REGISTER("$4");
			register struct TransitionMeta *rowTransition CTR_PSX_REGISTER("$13");
			register struct GameTracker *rowTracker CTR_PSX_REGISTER("$4");
			register struct GameTracker *rowDrawTracker CTR_PSX_REGISTER("$2");
			register s32 rowDrawZero CTR_PSX_REGISTER("$5");
			register u32 *rowDrawOt CTR_PSX_REGISTER("$6");
			register u16 rowDrawTrackCount CTR_PSX_REGISTER("$15");
			register s32 rowLoopTrackCount CTR_PSX_REGISTER("$16");
			rowBaseIndex = ((s16)rowIndex) - MM_TRACK_SELECT_CENTER_ROW;
			rowPhaseTemp = rowBaseIndex * (MM_TRACK_SELECT_ROW_ANGLE_STEP + 1);
			CTR_PSX_KEEP_VALUE_RELAXED(rowPhaseTemp);
			rowBaseAngle = rowPhaseTemp - rowBaseIndex;
			CTR_PSX_KEEP_VALUE_RELAXED(rowBaseAngle);
			changeFrames = MM_TRACK_CHANGE_FRAMES;
			rowAngle = rowBaseAngle;
			if (changeFrames <= 0)
			{
				goto trackRowAngleReady;
			}
#ifdef CTR_NATIVE
			{
				s32 angleStep;
				s32 angleOffset;
				angleStep = ((MM_TRACK_SELECT_TRACK_CHANGE_FRAMES - changeFrames) * MM_TRACK_SELECT_ROW_ANGLE_STEP) / MM_TRACK_SELECT_TRACK_CHANGE_FRAMES;
				angleOffset = angleStep * MM_TRACK_CHANGE_DIRECTION;
				rowAngle = rowBaseAngle + angleOffset;
			}
#else
			// NOTE(aalhendi): Keep GCC 2.8.1's retail constant-division sequence
			// local; reserving $v1 for the whole row loop perturbs unrelated code.
			asm("li %0,0x55550000\n\t"
			    "ori %0,%0,0x5556\n\t"
			    "li $3,3\n\t"
			    "subu $3,$3,%1\n\t"
			    "sll %1,$3,3\n\t"
			    "subu %1,%1,$3\n\t"
			    "sll %1,%1,2\n\t"
			    "addu %1,%1,$3\n\t"
			    "sll %1,%1,2\n\t"
			    "subu %1,%1,$3\n\t"
			    "mult %1,%0\n\t"
			    "sra %1,%1,31\n\t"
			    "lui $3,%%hi(" MM_TRACK_CHANGE_DIRECTION_ASM_NAME ")\n\t"
			    "mfhi $8\n\t"
			    "lh $3,%%lo(" MM_TRACK_CHANGE_DIRECTION_ASM_NAME ")($3)\n\t"
			    "subu %1,$8,%1\n\t"
			    "mult %1,$3\n\t"
			    "mflo $3\n\t"
			    "addu %0,%2,$3"
			    : "=r"(rowAngle), "+r"(changeFrames)
			    : "r"(rowBaseAngle)
			    : "$3", "$8", "hi", "lo");
#endif
		trackRowAngleReady:
			trigIndex = (rowAngle & (((0x1000 / 2) / 2) - 1)) * (sizeof(*trigTable));

			trigTable = MM_TRIG_APPROX;
#ifndef CTR_NATIVE
			asm volatile(".set $5,$3" : "+r"(trigTable), "+r"(trigIndex));
#endif
			packedTrig = CTR_ReadU32AlignedLE((u32 *)(trigIndex + ((u32)trigTable)));
			if ((rowAngle & ((0x1000 / 2) / 2)) != 0)
			{
				register u32 signTest CTR_PSX_REGISTER("$2");
				rowCos = (s16)packedTrig;
				rowSin = (s16)(packedTrig >> 16);
				signTest = rowAngle & (0x1000 / 2);
				if (signTest != 0)
				{
					rowSin = -rowSin;
				}
				else
				{
					rowCos = -rowCos;
				}
			}
			else
			{
				register u32 signTest CTR_PSX_REGISTER("$2");
				rowCos = (s16)(packedTrig >> 16);
				rowSin = (s16)packedTrig;
				signTest = rowAngle & (0x1000 / 2);
				if (signTest != 0)
				{
					rowCos = -rowCos;
					rowSin = -rowSin;
				}
			}
			rowRect.w = MM_TRACK_SELECT_ROW_W;
			rowRect.h = MM_TRACK_SELECT_ROW_H;
			CTR_PSX_MEMORY_BARRIER();
			rowCalc = (rowCos * MM_TRACK_SELECT_ROW_X_RADIUS) >> MM_TRACK_SELECT_ROW_X_SHIFT;
#ifdef CTR_NATIVE
			rowTransition = MM_TRACK_TRANSITIONS;
#else
			asm(".set $5,$a1\n\t"
			    "lui %0,%%hi(" MM_TRACK_TRANSITIONS_ASM_NAME ")\n\t"
			    "addiu %0,%0,%%lo(" MM_TRACK_TRANSITIONS_ASM_NAME ")"
			    : "=r"(rowTransition));
#endif
			rowBaseX = (u16)rowTransition->currX;
			rowCalc += MM_TRACK_SELECT_ROW_X_OFFSET;
			rowBaseX += rowCalc;
			rowCalc = (rowSin * (MM_TRACK_SELECT_ROW_Y_RADIUS >> 3)) << 3;
			CTR_PSX_KEEP_VALUE(rowCalc);
			rowCalc >>= MM_TRACK_SELECT_ROW_Y_SHIFT;
			CTR_PSX_COPY_VALUE(rowX, rowBaseX);
			rowRect.x = (s16)rowBaseX;
			CTR_PSX_MEMORY_BARRIER();
			rowBaseY = (u16)rowTransition->currY;
			rowTracker = CTR_PSX_PAGE_LVALUE(struct GameTracker *, gameTrackerPage, MM_GAME_TRACKER_PAGE_OFFSET, GAME_TRACKER);
			CTR_PSX_KEEP_VALUE(rowTracker);
			rowBaseY += rowCalc;
			rowBaseY += MM_TRACK_SELECT_ROW_Y_OFFSET;
			CTR_PSX_COPY_VALUE(rowY, rowBaseY);
			rowRect.y = (s16)rowBaseY;
			if ((rowTracker->gameMode1 & TIME_TRIAL) != 0)
			{
				// Draw the N. Tropy and Oxide stars earned for this track.
				register s32 timeTrialTrackOffset CTR_PSX_REGISTER("$2");
				register struct MainMenu_LevelRow *timeTrialTrack CTR_PSX_REGISTER("$21");
				s16 previousLevelID;
				s32 starIndex;
				starIndex = 0;
				previousLevelID = rowTracker->levelID;
				timeTrialTrackOffset = ((s16)currTrack) << 4;
				timeTrialTrack = (struct MainMenu_LevelRow *)(timeTrialTrackOffset + ((u32)selectMenu));
				CTR_PSX_KEEP_VALUE_RELAXED(timeTrialTrack);
				for (; starIndex < MM_TRACK_SELECT_TT_STAR_COUNT; starIndex++)
				{
					s16 beatenFlagBit;
					u32 timeTrialFlags;
					CTR_PSX_PAGE_LVALUE(struct GameTracker *, gameTrackerPage, MM_GAME_TRACKER_PAGE_OFFSET, GAME_TRACKER)->levelID = timeTrialTrack->levID;
					GAMEPROG_GetPtrHighScoreTrack();
#ifdef CTR_NATIVE
					beatenFlagBit = MM_TIME_TRIAL_STAR_FLAGS[starIndex];
					timeTrialFlags = GAME_PROGRESS.highScoreTracks[rowTracker->levelID].timeTrialFlags;
#else
					{
						register u32 starFlagsAddress CTR_PSX_REGISTER("$2");
						register u32 gameProgressBase CTR_PSX_REGISTER("$14");
						asm("lui %0,%%hi(" MM_TIME_TRIAL_STAR_FLAGS_ASM_NAME ")" : "=r"(starFlagsAddress));
						asm("addiu %0,%0,%%lo(" MM_TIME_TRIAL_STAR_FLAGS_ASM_NAME ")" : "+r"(starFlagsAddress));
						starFlagsAddress = (starIndex << 1) + starFlagsAddress;
						asm("lui %0,%%hi(" RETAIL_GAME_SAVE_ASM_NAME ")" : "=r"(gameProgressBase) : "r"(starFlagsAddress));
						asm("addiu %0,%0,%%lo(" RETAIL_GAME_SAVE_ASM_NAME ")" : "+r"(gameProgressBase));
						beatenFlagBit = *((u16 *)starFlagsAddress);
						timeTrialFlags = ((u32 *)(gameProgressBase + MM_TRACK_SELECT_TIME_TRIAL_FLAGS_OFFSET))
						    [((*((struct GameTracker **)(((u32)gameTrackerPage) + MM_GAME_TRACKER_PAGE_OFFSET)))->levelID *
						      ((sizeof(struct HighScoreTrack)) / (sizeof(u32)))) +
						     (beatenFlagBit >> 5)];
					}
#endif
					if (((timeTrialFlags >> (beatenFlagBit & 0x1f)) & 1) != 0)
					{
						register s32 starX CTR_PSX_REGISTER("$5");
						register s32 starY CTR_PSX_REGISTER("$6");
						struct Icon **iconPtrArray;
						struct GameTracker *starTracker;
						u32 *starOt;
						struct IconGroup *starIconGroup;
						struct DB *starBackBuffer;
						u32 **colorPtr;
						register s16 starColorIndex CTR_PSX_REGISTER("$3");
#ifndef CTR_NATIVE
						register u32 colorPointerPage CTR_PSX_REGISTER("$2");
#endif
						{
							register u16 *starColors CTR_PSX_REGISTER("$2");
							starColors = MM_TIME_TRIAL_STAR_COLORS;
							starColorIndex = starColors[starIndex];
						}
#ifndef CTR_NATIVE
						asm("lui %0,%%hi(" MM_COLOR_POINTERS_ASM_NAME ")" : "=r"(colorPointerPage) : "r"(starColorIndex));
						asm("addiu %0,%0,%%lo(" MM_COLOR_POINTERS_ASM_NAME ")\n\t.set $10,$15" : "+r"(colorPointerPage));
#endif
						starX = (s16)rowX;
						starX += MM_TRACK_SELECT_ROW_W + MM_TRACK_SELECT_STAR_X_OFFSET;
						starY = starIndex * MM_TRACK_SELECT_STAR_Y_STEP;
#ifdef CTR_NATIVE
						starY += MM_TRACK_SELECT_STAR_Y_OFFSET;
#else
						asm("addiu %0,%0,4" : "+r"(starY));
#endif
						starTracker = CTR_PSX_PAGE_LVALUE(struct GameTracker *, gameTrackerPage, MM_GAME_TRACKER_PAGE_OFFSET, GAME_TRACKER);
						starOt = starTracker->pushBuffer_UI.ptrOT;
						starIconGroup = starTracker->iconGroup[MM_TRACK_SELECT_TT_STAR_ICON_GROUP];
						starBackBuffer = starTracker->backBuffer;
						iconPtrArray = (struct Icon **)(((u32)starIconGroup) + (sizeof(struct IconGroup)));
#ifdef CTR_NATIVE
						colorPtr = &MM_COLOR_POINTERS[starColorIndex];
#else
						colorPtr = (u32 **)((starColorIndex * (sizeof(u32 *))) + colorPointerPage);
#endif
						MM_DECALHUD_DRAW_POLY_GT4(iconPtrArray[MM_TRACK_SELECT_TT_STAR_ICON], starX, ((s16)rowY) + starY, &starBackBuffer->primMem, starOt,
						                          ((Color *)(*colorPtr))[0], ((Color *)(*colorPtr))[1], ((Color *)(*colorPtr))[2], ((Color *)(*colorPtr))[3], 0,
						                          (int)(((float)1.0) * (1 << 12)));
					}
				}

				CTR_PSX_PAGE_LVALUE(struct GameTracker *, gameTrackerPage, MM_GAME_TRACKER_PAGE_OFFSET, GAME_TRACKER)->levelID = previousLevelID;
				GAMEPROG_GetPtrHighScoreTrack();
			}
			{
				register u32 uiPage CTR_PSX_REGISTER("$21");
				register struct MetaDataLEV *levelMeta CTR_PSX_REGISTER("$4");
				struct MainMenu_LevelRow *trackEntry;
				register s32 trackLevelID CTR_PSX_REGISTER("$3");
				register s32 rowNameX CTR_PSX_REGISTER("$5");
				register s32 rowNameY CTR_PSX_REGISTER("$6");
				s16 trackNameIndex;
#ifdef CTR_NATIVE
				uiPage = 0;
				rowNameX = (s16)(rowX + MM_TRACK_SELECT_ROW_NAME_X_OFFSET);
				levelMeta = MM_LEVEL_METADATA;
				trackEntry = &selectMenu[currTrack];
				rowNameY = (s16)(rowY + MM_TRACK_SELECT_ROW_NAME_Y_OFFSET - MM_TRACK_SELECT_ROW_Y_OFFSET);
				trackLevelID = trackEntry->levID;
#else
				// NOTE(aalhendi): Grouping these loads reproduces the retail
				// row-text call schedule; native computes the values above.
				asm volatile(".set $10,$t2\n\taddiu $5,$19,8\n\tsll $5,$5,16\n\tsra $5,$5,16\n\tlui $4," MM_LEVEL_METADATA_PAGE_UPPER_ASM
				             "\n\tsll $2,$17,16\n\tsra $2,$2,12\n\taddu "
				             "%2,$2,$22\n\taddiu "
				             "$4,$4," MM_LEVEL_METADATA_PAGE_OFFSET_ASM
				             "\n\taddiu $6,$20,5\n\tsll $6,$6,16\n\tlh $3,0(%2)\n\tlui $21," MM_GAME_TRACKER_PAGE_UPPER_ASM
				             : "=r"(rowNameX), "=r"(levelMeta), "=r"(trackEntry), "=r"(rowNameY), "=r"(trackLevelID), "=r"(uiPage)
				             : "r"(rowX), "r"(currTrack), "r"(selectMenu), "r"(rowY));
#endif
				trackNameIndex = levelMeta[trackLevelID].name_LNG;
#ifndef CTR_NATIVE
				asm("sra %0,%0,16" : "+r"(rowNameY));
#endif
				MM_DRAW_LINE_WIDE_FLAGS(CTR_PSX_PAGE_LVALUE(char **, uiPage, MM_LANGUAGE_STRINGS_PAGE_OFFSET, GAME_LANGUAGE_STRINGS)[trackNameIndex], rowNameX,
				                        rowNameY, FONT_BIG, ORANGE);
				rowDrawRectArg = &rowRect;
				if ((MM_TRACK_CHANGE_FRAMES == 0) && (((s16)rowIndex) == MM_TRACK_SELECT_CENTER_ROW))
				{
					RECT16 highlightRect;
					register RECT16 *clearRectArg CTR_PSX_REGISTER("$4");
					register u32 highlightColorPage CTR_PSX_REGISTER("$5");
					register struct GameTracker *highlightTracker CTR_PSX_REGISTER("$2");
					if ((CTR_PSX_PAGE_LVALUE(struct GameTracker *, gameTrackerPage, MM_GAME_TRACKER_PAGE_OFFSET, GAME_TRACKER)->gameMode1 & TIME_TRIAL) != 0)
					{
						s16 ghostProfileCount = RefreshCard_CountGhostProfilesForLEV(trackEntry->levID);
						if (ghostProfileCount != 0)
						{
							register s32 ghostTextX CTR_PSX_REGISTER("$5");
							register s32 ghostTextY CTR_PSX_REGISTER("$6");
							register s32 ghostTextFlags CTR_PSX_REGISTER("$2");
							register char **ghostStrings CTR_PSX_REGISTER("$4");
							{
								register s32 ghostTextXCalc CTR_PSX_REGISTER("$2");
								ghostTextXCalc = (rowX + MM_TRACK_SELECT_ROW_NAME_X_OFFSET) + MM_TRACK_SELECT_GHOST_TEXT_FROM_NAME_X;
								CTR_PSX_KEEP_VALUE_RELAXED(ghostTextXCalc);
								ghostTextX = (s16)ghostTextXCalc;
							}
							{
								register s32 ghostTextYCalc CTR_PSX_REGISTER("$3");
#ifdef CTR_NATIVE
								ghostTextYCalc = rowY + MM_TRACK_SELECT_GHOST_TEXT_Y_OFFSET - MM_TRACK_SELECT_ROW_Y_OFFSET;
#else
								asm("addiu %0,%1,%2" : "=r"(ghostTextYCalc) : "r"(rowY), "I"(22));
#endif
								CTR_PSX_KEEP_VALUE_RELAXED(ghostTextYCalc);
								ghostTextY = (s16)ghostTextYCalc;
							}
							ghostTextFlags = MM_FRAME_COUNTER;
							ghostStrings = CTR_PSX_PAGE_LVALUE(char **, uiPage, MM_LANGUAGE_STRINGS_PAGE_OFFSET, GAME_LANGUAGE_STRINGS);
							ghostTextFlags &= MM_TRACK_SELECT_GHOST_FLASH_FRAME_BIT;
							if (ghostTextFlags != 0)
							{
								ghostTextFlags = -0x7fff;
							}
							else
							{
								ghostTextFlags = -0x7ffc;
							}
							MM_DRAW_LINE_WIDE_FLAGS(ghostStrings[LNG_GHOST_DATA_EXISTS], ghostTextX, ghostTextY, FONT_SMALL, ghostTextFlags);
						}
					}
					clearRectArg = &highlightRect;
					CTR_PSX_KEEP_VALUE(clearRectArg);
					highlightColorPage = MM_GAME_TRACKER_PAGE_VALUE;
					CTR_PSX_KEEP_VALUE(highlightColorPage);
					highlightRect.x = rowRect.x + MM_TRACK_SELECT_HIGHLIGHT_INSET_X;
					highlightRect.w = rowRect.w - MM_TRACK_SELECT_HIGHLIGHT_W_SHRINK;
					highlightRect.y = rowRect.y + MM_TRACK_SELECT_HIGHLIGHT_INSET_Y;
					highlightTracker = CTR_PSX_PAGE_LVALUE(struct GameTracker *, gameTrackerPage, MM_GAME_TRACKER_PAGE_OFFSET, GAME_TRACKER);
					highlightRect.h = rowRect.h - MM_TRACK_SELECT_HIGHLIGHT_H_SHRINK;
					MM_DRAW_CLEAR_BOX(clearRectArg,
					                  &CTR_PSX_PAGE_LVALUE(Color, highlightColorPage, MM_MENU_ROW_HIGHLIGHT_NORMAL_PAGE_OFFSET, GAME_MENU_HIGHLIGHT),
					                  TRANS_50_DECAL, highlightTracker->backBuffer->otMem.uiOT, &highlightTracker->backBuffer->primMem);
					rowDrawRectArg = &rowRect;
				}
				rowDrawTracker = CTR_PSX_PAGE_LVALUE(struct GameTracker *, gameTrackerPage, MM_GAME_TRACKER_PAGE_OFFSET, GAME_TRACKER);
				rowDrawZero = 0;
				rowDrawTrackCount = *((volatile u16 *)&numTracks);
				rowDrawOt = rowDrawTracker->backBuffer->otMem.uiOT;
				rowLoopTrackCount = rowDrawTrackCount;
				RECTMENU_DrawInnerRect(rowDrawRectArg, rowDrawZero, rowDrawOt);
			}
			do
			{
				currTrack++;
				if (rowLoopTrackCount <= currTrack)
				{
					currTrack = 0;
				}
				trackOpen = MM_TrackSelect_boolTrackOpen(&selectMenu[currTrack]);
			} while (!trackOpen);
			{
				register s16 nextRowIndex CTR_PSX_REGISTER("$2");
				nextRowIndex = rowIndex + 1;
				rowIndex = nextRowIndex;
			}
			if (MM_TRACK_SELECT_VISIBLE_ROWS <= rowIndex)
			{
				// The preview, title, and minimap are drawn after the row list.
				register s32 previewX CTR_PSX_REGISTER("$3");
				register s32 previewY CTR_PSX_REGISTER("$2");
				RECT16 previewRect;
				previewRect.w = MM_TRACK_VIDEO_WIDTH;
				previewRect.h = MM_TRACK_VIDEO_HEIGHT;
#ifdef CTR_NATIVE
				previewX = MM_TRACK_TRANSITIONS[1].currX;
				previewY = MM_TRACK_TRANSITIONS[1].currY;
#else
				asm volatile("lui %1," MM_OVERLAY_PAGE_UPPER_ASM "\n\taddiu %1,%1," MM_TRACK_TRANSITIONS_PAGE_OFFSET_ASM
				             "\n\tlhu %0,0x10(%1)\n\tlhu %1,0x12(%1)"
				             : "=r"(previewX), "=r"(previewY)
				             :
				             : "memory");
#endif
				previewRect.x = previewX + MM_TRACK_SELECT_PREVIEW_X;
				previewRect.y = previewY + MM_TRACK_SELECT_PREVIEW_Y_NO_MAP;
				CTR_PSX_MEMORY_BARRIER();
				{
					register struct RectMenu *previewMenu CTR_PSX_REGISTER("$12");
					previewMenu = menuLocal;
					CTR_PSX_KEEP_VALUE(previewMenu);
					if ((-1) < selectMenu[previewMenu->rowSelected].mapTextureID)
					{
						previewRect.y = MM_TRACK_PREVIEW_Y + MM_TRACK_SELECT_PREVIEW_Y_WITH_MAP;
					}
				}
				{
					register u32 previewLapPage CTR_PSX_REGISTER("$19");
					register s32 titleFont CTR_PSX_REGISTER("$7");
#ifdef CTR_NATIVE
					previewLapPage = 0;
#else
					asm("lui %0," MM_OVERLAY_PAGE_UPPER_ASM "\n\t.set $18,$s1\n\t.set $17,$s2" : "=r"(previewLapPage));
#endif
					titleFont = FONT_BIG;
					(void)titleFont;
					if (CTR_PSX_PAGE_LVALUE(s16, previewLapPage, MM_TRACK_LAP_BOX_OPEN_PAGE_OFFSET, MM_TRACK_LAP_BOX_OPEN) == 0)
					{
#ifdef CTR_NATIVE
						levelTitleTransition = MM_TRACK_TRANSITIONS;
						DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_SELECT_LEVEL_SELECT], levelTitleTransition[3].currX + MM_TRACK_SELECT_TITLE_X,
						                   levelTitleTransition[3].currY + (u16)previewRect.y, FONT_BIG, JUSTIFY_CENTER | ORANGE);
						DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_LEVEL], levelTitleTransition[3].currX + MM_TRACK_SELECT_TITLE_X,
						                   levelTitleTransition[3].currY + (u16)previewRect.y + MM_TRACK_SELECT_LEVEL_TEXT_Y_STEP, FONT_BIG,
						                   JUSTIFY_CENTER | ORANGE);
#else
						// NOTE(aalhendi): Retail schedules both title draws as one block;
						// native keeps the equivalent calls visible above.
						asm volatile(".set\tnoreorder\n\t"
						             ".set\tnomacro\n\t"
						             "lui $16,%%hi(" MM_TRACK_TRANSITIONS_ASM_NAME ")\n\t"
						             "addiu $16,$16,%%lo(" MM_TRACK_TRANSITIONS_ASM_NAME ")\n\t"
						             "lui $18,%%hi(" RETAIL_LANGUAGE_STRINGS_ASM_NAME ")\n\t"
						             "lhu $5,36($16)\n\t"
						             "lhu $3,%0\n\t"
						             "lw $2,%%lo(" RETAIL_LANGUAGE_STRINGS_ASM_NAME ")($18)\n\t"
						             "li $17,-32768\n\t"
						             "sw $17,16($sp)\n\t"
						             "lhu $6,38($16)\n\t"
						             "addiu $5,$5,396\n\t"
						             "sll $5,$5,16\n\t"
						             "sra $5,$5,16\n\t"
						             "addu $6,$6,$3\n\t"
						             "sll $6,$6,16\n\t"
						             "lw $4,420($2)\n\t"
						             "jal DecalFont_DrawLine\n\t"
						             "sra $6,$6,16\n\t"
						             "lhu $5,36($16)\n\t"
						             "lhu $3,%0\n\t"
						             "lw $2,%%lo(" RETAIL_LANGUAGE_STRINGS_ASM_NAME ")($18)\n\t"
						             "li $7,1\n\t"
						             "sw $17,16($sp)\n\t"
						             "lhu $6,38($16)\n\t"
						             "addiu $5,$5,396\n\t"
						             "sll $5,$5,16\n\t"
						             "sra $5,$5,16\n\t"
						             "addu $6,$6,$3\n\t"
						             "addiu $6,$6,16\n\t"
						             "sll $6,$6,16\n\t"
						             "lw $4,424($2)\n\t"
						             "jal DecalFont_DrawLine\n\t"
						             "sra $6,$6,16\n\t"
						             ".set\tmacro\n\t"
						             ".set\treorder"
						             :
						             : "m"(previewRect.y), "r"(titleFont)
						             : "memory");
#endif
					}
					previewRect.y += MM_TRACK_SELECT_TITLE_TO_MAP_Y;
					{
						register struct RectMenu *mapCheckMenu CTR_PSX_REGISTER("$13");
						mapCheckMenu = menuLocal;
#ifdef CTR_NATIVE
						CTR_PSX_KEEP_VALUE(mapCheckMenu);
#else
						asm volatile(".set $18,$s2\n\t.set $17,$s1" : "+r"(mapCheckMenu));
#endif
						if (((-1) < selectMenu[mapCheckMenu->rowSelected].mapTextureID) &&
						    (CTR_PSX_PAGE_LVALUE(s16, previewLapPage, MM_TRACK_LAP_BOX_OPEN_PAGE_OFFSET, MM_TRACK_LAP_BOX_OPEN) == 0))
						{
							register struct TransitionMeta *mapTransitions CTR_PSX_REGISTER("$4");
							s32 mapID;
							struct Icon *iconMap0;
							struct Icon *iconMap1;
							int mapTopV2;
							u8 mapTopV0;
							u8 mapBottomV2;
							u8 mapBottomV0;
							s16 mapWidth;
							register s16 mapLayer CTR_PSX_REGISTER("$18");
							register struct TrackSelectMapOffset *mapOffsetBase CTR_PSX_REGISTER("$19");
							s16 mapHeight;
							s32 mapHalfWidth;
							register s32 mapBoxHeight CTR_PSX_REGISTER("$2");
							struct TextureLayout *mapLayout;
							RECT16 mapRect;
							mapLayer = 0;
							mapBoxHeight = MM_TRACK_SELECT_MAP_BOX_H;
#ifdef CTR_NATIVE
							mapTransitions = MM_TRACK_TRANSITIONS;
							mapRect = previewRect;
#else
							asm("lui %0," MM_OVERLAY_PAGE_UPPER_ASM "\n\taddiu %0,%0," MM_TRACK_TRANSITIONS_PAGE_OFFSET_ASM : "=r"(mapTransitions));
							{
								register u32 mapRectWord0 CTR_PSX_REGISTER("$14");
								register u32 mapRectWord1 CTR_PSX_REGISTER("$15");
								asm volatile("lwl %0,%6\n\tlwr %0,%7\n\tlwl %1,%8\n\tlwr %1,%9\n\tswl %0,%2\n\tswr %0,%3\n\tswl %1,%4\n\tswr %1,%5"
								             : "=&r"(mapRectWord0), "=&r"(mapRectWord1), "=m"(*(((u8 *)(&mapRect)) + 3)), "=m"(*(((u8 *)(&mapRect)) + 0)),
								               "=m"(*(((u8 *)(&mapRect)) + 7)), "=m"(*(((u8 *)(&mapRect)) + 4))
								             : "m"(*(((u8 *)(&previewRect)) + 3)), "m"(*(((u8 *)(&previewRect)) + 0)), "m"(*(((u8 *)(&previewRect)) + 7)),
								               "m"(*(((u8 *)(&previewRect)) + 4)));
							}
#endif
							mapRect.h = mapBoxHeight;
							{
								register struct RectMenu *mapMenu CTR_PSX_REGISTER("$14");
								struct GameTracker *mapTracker;
								register u32 mapTrackerPage CTR_PSX_REGISTER("$4");
								{
									register s32 mapFromX CTR_PSX_REGISTER("$5");
									register s32 mapToX CTR_PSX_REGISTER("$2");
									register s32 mapRectX CTR_PSX_REGISTER("$3");
									mapFromX = (u16)mapTransitions[1].currX;
									mapToX = (u16)mapTransitions[2].currX;
									mapRectX = (u16)mapRect.x;
									mapMenu = menuLocal;
									CTR_PSX_KEEP_VALUE(mapMenu);
									mapToX -= mapFromX;
									mapRectX += mapToX;
									mapRect.x = mapRectX;
								}
								{
									register s32 mapFromY CTR_PSX_REGISTER("$5");
									register s32 mapToY CTR_PSX_REGISTER("$3");
									register s32 mapRectY CTR_PSX_REGISTER("$2");
									mapFromY = (u16)mapTransitions[1].currY;
									mapToY = (u16)mapTransitions[2].currY;
#ifdef CTR_NATIVE
									mapTrackerPage = 0;
#else
									asm("lui %0," MM_GAME_TRACKER_PAGE_UPPER_ASM : "=r"(mapTrackerPage));
#endif
									mapRectY = (u16)mapRect.y;
									mapToY -= mapFromY;
									mapRectY += mapToY;
									mapRect.y = mapRectY;
#ifndef CTR_NATIVE
									asm volatile("" : : "m"(mapRect.y));
#endif
									mapRectY += MM_TRACK_SELECT_MAP_CENTER_Y_OFFSET;
									mapRect.y = mapRectY;
								}
								mapID = selectMenu[mapMenu->rowSelected].mapTextureID;
								mapTracker = CTR_PSX_PAGE_LVALUE(struct GameTracker *, mapTrackerPage, MM_GAME_TRACKER_PAGE_OFFSET, GAME_TRACKER);
								iconMap0 = mapTracker->ptrIcons[mapID + 0];
								iconMap1 = mapTracker->ptrIcons[mapID + 1];
							}
							mapLayout = &iconMap0->texLayout;
							mapWidth = (s32)mapLayout->u1;
							mapTopV2 = mapLayout->v2;
							mapTopV0 = mapLayout->v0;
							mapLayout = &iconMap1->texLayout;
							mapBottomV2 = mapLayout->v2;
							mapBottomV0 = mapLayout->v0;
							mapWidth -= (s32)iconMap0->texLayout.u0;
							mapHeight = (s32)mapTopV2;
							mapHeight = mapHeight - ((s32)mapTopV0);
							mapHeight += (s32)mapBottomV2;
							mapHeight -= (s32)mapBottomV0;
#ifdef CTR_NATIVE
							mapOffsetBase = MM_TRACK_DRAW_MAP_OFFSET;
#else
							{
								register u32 mapOffsetPage CTR_PSX_REGISTER("$4");
								asm("lui %0,%%hi(" MM_TRACK_DRAW_MAP_OFFSET_ASM_NAME ")" : "=r"(mapOffsetPage));
								asm("addiu %0,%1,%%lo(" MM_TRACK_DRAW_MAP_OFFSET_ASM_NAME ")" : "=r"(mapOffsetBase) : "r"(mapOffsetPage));
							}
#endif
#ifndef CTR_NATIVE
							// NOTE(aalhendi): GCC 2.8.1 leaves 15 words different in this C
							// loop. PSX substitutes the retail register and delay-slot schedule;
							// CTR_NATIVE executes the shared C behavior below.
							asm volatile(".if 0");
#endif
							do
							{
								register struct TrackSelectMapOffset *mapOffset CTR_PSX_REGISTER("$4");
								register struct RectMenu *mapDrawMenu CTR_PSX_REGISTER("$15");
								register struct GameTracker *mapDrawTracker CTR_PSX_REGISTER("$8");
								register s32 mapLayerIndex CTR_PSX_REGISTER("$2");
								mapDrawTracker = GAME_TRACKER;
								mapHalfWidth = ((s32)((s16)mapRect.w)) / 2;
								mapLayerIndex = (s16)mapLayer;
								mapOffset = &mapOffsetBase[mapLayerIndex];
								mapLayer++;
								mapDrawMenu = menuLocal;
								uiPushBuffer = &mapDrawTracker->pushBuffer_UI;
								rowCos = mapDrawMenu->rowSelected;
								UI_Map_DrawMap(mapDrawTracker->ptrIcons[selectMenu[rowCos].mapTextureID],
								               mapDrawTracker->ptrIcons[selectMenu[rowCos].mapTextureID + 1],
								               ((((s32)((s16)mapOffset->offsetX)) + ((s32)((s16)mapRect.x))) + mapHalfWidth) + (((s32)((s16)mapWidth)) / 2),
								               ((((s32)((s16)mapOffset->offsetY)) + ((s32)((s16)mapRect.y))) + (((s32)((s16)mapRect.h)) / 2)) +
								                   (((s32)((s16)mapHeight)) / 2),
								               &mapDrawTracker->backBuffer->primMem, uiPushBuffer->ptrOT, mapOffset->type);
							} while (((u16)mapLayer) < MM_TRACK_SELECT_MAP_LAYER_COUNT);
#ifndef CTR_NATIVE
							asm volatile(".endif" : : : "memory");
							asm volatile(".set\tnoreorder\n\t"
							             ".set\tnomacro\n\t"
							             "sll $3,$3,16\n\t"
							             "sra $2,$3,16\n\t"
							             "srl $3,$3,31\n\t"
							             "addu $2,$2,$3\n\t"
							             "sra $16,$2,1\n\t"
							             "lui $5,%%hi(" RETAIL_GAME_TRACKER_ASM_NAME ")\n\t"
							             "1:\n\t"
							             "sll $2,$18,16\n\t"
							             "sra $2,$2,16\n\t"
							             "sll $4,$2,1\n\t"
							             "addu $4,$4,$2\n\t"
							             "sll $4,$4,1\n\t"
							             "addu $4,$4,$19\n\t"
							             "addiu $18,$18,1\n\t"
							             "lw $15,80($sp)\n\t"
							             "lw $8,%%lo(" RETAIL_GAME_TRACKER_ASM_NAME ")($5)\n\t"
							             "lh $6,0($4)\n\t"
							             "lh $10,72($sp)\n\t"
							             "lh $9,2($4)\n\t"
							             "lh $11,74($sp)\n\t"
							             "lhu $7,78($sp)\n\t"
							             "addu $6,$6,$10\n\t"
							             "addu $9,$9,$11\n\t"
							             "sll $7,$7,16\n\t"
							             "lh $3,26($15)\n\t"
							             "lw $2,16($8)\n\t"
							             "sll $3,$3,4\n\t"
							             "addu $3,$3,$22\n\t"
							             "lh $5,4($3)\n\t"
							             "lhu $3,76($sp)\n\t"
							             "addiu $2,$2,116\n\t"
							             "sw $2,16($sp)\n\t"
							             "lw $2,5244($8)\n\t"
							             "sll $3,$3,16\n\t"
							             "sw $2,20($sp)\n\t"
							             "sra $2,$3,16\n\t"
							             "srl $3,$3,31\n\t"
							             "addu $2,$2,$3\n\t"
							             "sra $2,$2,1\n\t"
							             "addu $6,$6,$2\n\t"
							             "lbu $2,4($4)\n\t"
							             "addu $6,$6,$16\n\t"
							             "sw $2,24($sp)\n\t"
							             "sll $2,$5,2\n\t"
							             "addu $2,$8,$2\n\t"
							             "addiu $5,$5,1\n\t"
							             "sll $5,$5,2\n\t"
							             "addu $8,$8,$5\n\t"
							             "lw $4,7916($2)\n\t"
							             "sra $2,$7,16\n\t"
							             "srl $7,$7,31\n\t"
							             "addu $2,$2,$7\n\t"
							             "sra $2,$2,1\n\t"
							             "addu $9,$9,$2\n\t"
							             "sll $2,$17,16\n\t"
							             "sra $7,$2,16\n\t"
							             "srl $2,$2,31\n\t"
							             "addu $7,$7,$2\n\t"
							             "sra $7,$7,1\n\t"
							             "lw $5,7916($8)\n\t"
							             "jal UI_Map_DrawMap\n\t"
							             "addu $7,$9,$7\n\t"
							             "andi $2,$18,0xffff\n\t"
							             "sltiu $2,$2,6\n\t"
							             "bne $2,$0,1b\n\t"
							             "lui $5,%%hi(" RETAIL_GAME_TRACKER_ASM_NAME ")\n\t"
							             ".set\tmacro\n\t"
							             ".set\treorder"
							             :
							             :
							             : "memory");
#endif
						}
					}
				}
				MM_TrackSelect_Video_Draw(&previewRect, selectMenu, (int)((s16)MM_TRACK_CURRENT_TRACK), (u32)(MM_TRACK_TRANSITION_STATE == EXITING_MENU), 0);
				return;
			}
		} while (1);
	}
}

struct RectMenu *MM_TrackSelect_GetMenuPtr(void)
{
	return &MM_MENU_TRACK_SELECT;
}
