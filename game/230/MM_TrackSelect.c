#include <common.h>
#include <platform/native_track_library.h>
#include <platform/native_track_preview.h>
#include <platform/native_track_map.h>
#include <platform/native_renderer.h>
#include <track_text.h>
#include <level_info.h>
#include <stdlib.h>

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
	MM_TRACK_SELECT_COMBINED_BATTLE_TRACK_COUNT = MM_TRACK_SELECT_BATTLE_TRACK_COUNT + MM_TRACK_SELECT_ARCADE_TRACK_COUNT,
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
	MM_TRACK_SELECT_CUSTOM_TITLE_BYTES = 256,
	MM_TRACK_SELECT_CUSTOM_TITLE_MARGIN = 16,
	MM_TRACK_SELECT_CUSTOM_SCROLL_PAUSE_FRAMES = 45,
	MM_TRACK_SELECT_CUSTOM_SCROLL_STEP_FRAMES = 8,
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

static struct MainMenu_LevelRow s_combinedBattleTracks[MM_TRACK_SELECT_COMBINED_BATTLE_TRACK_COUNT];
static struct MainMenu_LevelRow *s_raceTracks;
static s16 s_raceTrackCount;
static s16 s_titleScrollSelectedRow = -1;
static u32 s_titleScrollStartFrame;
static int s_previewTrackID = -1;
static int s_previewLoaded;
static int s_previewAspectFactor;
static struct NativeTrackMap s_customMap;
static struct Icon s_customMapIcons[2];
static int s_customMapTrackID = -1;
static int s_customMapLoaded;
static u32 s_customMapTexture;

static int MM_TrackSelect_CustomMap(int levelID)
{
	if (NativeTrackRegistry_Get(NativeTrackLibrary_Current(), levelID) == NULL) { return 0; }
	if (s_customMapTrackID != levelID)
	{
		s_customMapTrackID = levelID;
		s_customMapLoaded = NativeTrackMap_Load(&s_customMap, levelID);
		if (s_customMapLoaded)
		{
			s_customMapTexture = NativeRenderer_UploadTrackMap(s_customMap.pixels);
			s_customMapLoaded = s_customMapTexture != 0;
			memset(s_customMapIcons, 0, sizeof(s_customMapIcons));
			for (int i = 0; i < 2; i++)
			{
				struct TextureLayout *layout = &s_customMapIcons[i].texLayout;
				layout->u1 = layout->u3 = (u8)s_customMap.width;
				layout->v0 = layout->v1 = i == 0 ? 0 : (u8)s_customMap.topHeight;
				layout->v2 = layout->v3 = (u8)(i == 0 ? s_customMap.topHeight : s_customMap.height);
				layout->tpage = 0x100;
			}
		}
	}
	return s_customMapLoaded;
}

static void MM_TrackSelect_DrawMapLayers(struct Icon *top, struct Icon *bottom, int x, int y)
{
	struct GameTracker *gGT = GAME_TRACKER;
	for (int i = 0; i < MM_TRACK_SELECT_MAP_LAYER_COUNT; i++)
	{
		struct TrackSelectMapOffset *offset = &MM_TRACK_DRAW_MAP_OFFSET[i];
		UI_Map_DrawMap(top, bottom, x + offset->offsetX, y + offset->offsetY,
		               &gGT->backBuffer->primMem, gGT->pushBuffer_UI.ptrOT, offset->type);
	}
}

static void MM_TrackSelect_DrawCustomMap(int centerX, int centerY)
{
	struct GameTracker *gGT = GAME_TRACKER;
	struct PrimMem *memory = &gGT->backBuffer->primMem;
	size_t needed = 2 * sizeof(DR_PSYX_TEX) + MM_TRACK_SELECT_MAP_LAYER_COUNT * 2 * sizeof(POLY_FT4);
	if ((uintptr_t)memory->cursor > (uintptr_t)memory->guardEnd ||
	    (uintptr_t)memory->guardEnd - (uintptr_t)memory->cursor < needed) { return; }
	/* OT insertion prepends: the reset is emitted after the map is drawn. */
	DR_PSYX_TEX *reset = memory->cursor;
	SetPsyXTexture(reset, 0, 0, 0);
	AddPrim(gGT->pushBuffer_UI.ptrOT, reset);
	memory->cursor = reset + 1;
	MM_TrackSelect_DrawMapLayers(&s_customMapIcons[0], &s_customMapIcons[1],
	                             centerX + s_customMap.width / 2, centerY + s_customMap.height / 2);
	DR_PSYX_TEX *bind = memory->cursor;
	SetPsyXTexture(bind, s_customMapTexture, NATIVE_TRACK_MAP_TEXTURE_SIZE, NATIVE_TRACK_MAP_TEXTURE_SIZE);
	AddPrim(gGT->pushBuffer_UI.ptrOT, bind);
	memory->cursor = bind + 1;
}

static void MM_TrackSelect_BuildRaceTracks(void)
{
	const struct NativeTrackRegistry *registry = NativeTrackLibrary_Current();
	size_t count = NativeTrackRegistry_Count(registry);
	const size_t maxCustom = 0x7fff - MM_TRACK_SELECT_ARCADE_TRACK_COUNT;
	if (count > maxCustom)
	{
		count = maxCustom;
	}
	free(s_raceTracks);
	s_raceTracks = NULL;
	s_raceTrackCount = MM_TRACK_SELECT_ARCADE_TRACK_COUNT;
	if (count == 0)
	{
		return;
	}
	s_raceTracks = malloc((MM_TRACK_SELECT_ARCADE_TRACK_COUNT + count) * sizeof(*s_raceTracks));
	if (s_raceTracks == NULL)
	{
		return;
	}
	memcpy(s_raceTracks, MM_ARCADE_TRACKS, sizeof(MM_ARCADE_TRACKS));
	size_t enabled = 0;
	for (size_t i = 0; i < count; i++)
	{
		int levelID = NATIVE_TRACK_CUSTOM_ID_BASE + (int)i;
		const struct NativeTrackMetadata *track = NativeTrackRegistry_Get(registry, levelID);
		unsigned int mode = (GAME_TRACKER->gameMode1 & ARCADE_MODE) != 0 ? NATIVE_TRACK_DETECTED_ARCADE : NATIVE_TRACK_DETECTED_VS;
		b32 allowed = (GAME_TRACKER->gameMode1 & TIME_TRIAL) != 0 ? NativeTrackLibrary_AllowsTimeTrial(track) :
		              NativeTrackLibrary_AllowsDetectedMode(track, mode);
		if (!NativeTrackLibrary_IsEnabled(track) || !allowed)
		{
			continue;
		}
		struct MainMenu_LevelRow *row = &s_raceTracks[MM_TRACK_SELECT_ARCADE_TRACK_COUNT + enabled++];
		*row = MM_ARCADE_TRACKS[0];
		row->levID = (s16)levelID;
		row->mapTextureID = -1;
		row->unlock = MM_TRACK_UNLOCK_ALWAYS;
	}
	s_raceTrackCount += (s16)enabled;
}

static void MM_TrackSelect_ScrollTitle(char *title, int available, u32 frame)
{
	if (DecalFont_GetLineWidth(title, FONT_BIG) <= available)
	{
		return;
	}

	size_t length = strlen(title);
	size_t lastStart = 0;
	while (lastStart < length && DecalFont_GetLineWidth(title + lastStart, FONT_BIG) > available)
	{
		lastStart++;
	}

	u32 travelFrames = (u32)lastStart * MM_TRACK_SELECT_CUSTOM_SCROLL_STEP_FRAMES;
	u32 cycle = travelFrames + 2 * MM_TRACK_SELECT_CUSTOM_SCROLL_PAUSE_FRAMES;
	u32 position = frame % cycle;
	size_t start = 0;
	if (position >= MM_TRACK_SELECT_CUSTOM_SCROLL_PAUSE_FRAMES)
	{
		start = (position - MM_TRACK_SELECT_CUSTOM_SCROLL_PAUSE_FRAMES) / MM_TRACK_SELECT_CUSTOM_SCROLL_STEP_FRAMES;
		if (start > lastStart)
		{
			start = lastStart;
		}
	}
	memmove(title, title + start, length - start + 1);
	length -= start;
	while (length > 0 && DecalFont_GetLineWidth(title, FONT_BIG) > available)
	{
		title[--length] = '\0';
	}
}

static b32 MM_TrackSelect_UseRaceTracks(void)
{
	return g_config.allowRaceTracksInBattle;
}

static void MM_TrackSelect_GetBattleTracks(struct MainMenu_LevelRow **tracks, s16 *trackCount)
{
	if (!MM_TrackSelect_UseRaceTracks())
	{
		*tracks = MM_BATTLE_TRACKS;
		*trackCount = MM_TRACK_SELECT_BATTLE_TRACK_COUNT;
		return;
	}

	memcpy(s_combinedBattleTracks, MM_BATTLE_TRACKS, sizeof(MM_BATTLE_TRACKS));
	memcpy(&s_combinedBattleTracks[MM_TRACK_SELECT_BATTLE_TRACK_COUNT], MM_ARCADE_TRACKS, sizeof(MM_ARCADE_TRACKS));
	*tracks = s_combinedBattleTracks;
	*trackCount = MM_TRACK_SELECT_COMBINED_BATTLE_TRACK_COUNT;
}

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

#include <platform/native_str.h>

static void MM_TrackSelect_Video_DrawNativePreview(RECT *r, s32 srcX, s32 srcY, int opaque)
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
	if (opaque)
	{
		for (POLY_FT4 *poly = (POLY_FT4 *)prim; poly < (POLY_FT4 *)nextPrim; poly++)
		{
			poly->code &= (u8)~2;
		}
	}
	((POLY_FT4 *)nextPrim - 1)->tag = CtrGpu_PackOTTag(oldTag, 0x09000000);
	gGT->backBuffer->primMem.cursor = nextPrim;
}

void MM_TrackSelect_Video_Draw(RECT *r, struct MainMenu_LevelRow *selectMenu, s16 trackIndex, s32 stopVideo, u16 rectFlags)
{
	struct BigHeader *bigfileHeader;
	struct BigEntry *bigfileEntries;
	struct MainMenu_LevelRow *selectedMenu;
	s32 videoFileIndex;

	if (NativeTrackRegistry_Get(NativeTrackLibrary_Current(), selectMenu[trackIndex].levID) != NULL)
	{
		if (MM_TRACK_VIDEO_MEM_ALLOCATED != 0)
		{
			NativeSTR_Stop();
			MM_TRACK_VIDEO_MEM_ALLOCATED = 0;
		}
		MM_TRACK_VIDEO_STATE_CURR = MM_TRACK_VIDEO_ICON;
		MM_TRACK_VIDEO_STATE_PREV = MM_TRACK_VIDEO_ICON;
		int drawn = 0;
		struct Icon *icon = GAME_TRACKER->ptrIcons[MM_TRACK_VIDEO_ICON_INDEX];
		if (icon != NULL)
		{
			u16 tpage = icon->texLayout.tpage;
			int srcX = icon->texLayout.u0 + (tpage & 0xf) * 0x40 + MM_TRACK_VIDEO_FRAME_SRC_OFFSET_X;
			int srcY = icon->texLayout.v0 + (tpage & 0x10) * 0x10 + ((tpage & 0x800) >> 2) + MM_TRACK_VIDEO_FRAME_SRC_OFFSET_Y;
			int aspectFactor = Widescreen_GetFactor();
			if (s_previewTrackID != selectMenu[trackIndex].levID || s_previewAspectFactor != aspectFactor)
			{
				s_previewTrackID = selectMenu[trackIndex].levID;
				s_previewAspectFactor = aspectFactor;
				s_previewLoaded = NativeTrackPreview_Load(s_previewTrackID, srcX, srcY, MM_TRACK_VIDEO_FRAME_WIDTH, MM_TRACK_VIDEO_FRAME_HEIGHT, aspectFactor);
			}
			if (s_previewLoaded && r->x >= 0 && r->y >= 0 && r->x + r->w <= MM_TRACK_VIDEO_SCREEN_W && r->y + r->h <= MM_TRACK_VIDEO_SCREEN_H)
			{
				MM_TrackSelect_Video_DrawNativePreview(r, srcX, srcY, 1);
 				drawn = 1;
			}
		}
		if (!drawn)
		{
			DecalFont_DrawLine("NO PREVIEW", r->x + r->w / 2, r->y + r->h / 2, FONT_SMALL, JUSTIFY_CENTER | WHITE);
		}
		RECTMENU_DrawInnerRect(r, (s16)(rectFlags | 1), GAME_TRACKER->backBuffer->otMem.uiOT);
		return;
	}
	s_previewTrackID = -1;
	s_customMapTrackID = -1;
	bigfileHeader = MM_BIGFILE_HEADER;
	bigfileEntries = BIG_GETENTRY(bigfileHeader);
	selectedMenu = (struct MainMenu_LevelRow *)((u32)(trackIndex * sizeof(*selectedMenu)) + (u32)selectMenu);
	videoFileIndex = selectedMenu->previewVideoFileIndex;

	if ((bigfileEntries[videoFileIndex].size != 0) && (r->x >= 0) && (r->y >= 0) && ((r->x + r->w) < (MM_TRACK_VIDEO_SCREEN_W + 1)) &&
	    ((r->y + r->h) < (MM_TRACK_VIDEO_SCREEN_H + 1)))
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
				MM_TrackSelect_Video_DrawNativePreview(r, srcX + MM_TRACK_VIDEO_FRAME_SRC_OFFSET_X, srcY + MM_TRACK_VIDEO_FRAME_SRC_OFFSET_Y, 0);
			}
		}
	}

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

  if ((flag == MM_TRACK_UNLOCK_ALWAYS) ||
      ((flag == MM_TRACK_UNLOCK_1P_ONLY) &&
      ((GAME_TRACKER->numPlyrNextGame == 1) ||
      g_config.allowOxideStationMultiplayer)) ||
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

	s_titleScrollSelectedRow = -1;
	s_previewTrackID = -1;
	s_customMapTrackID = -1;
	MM_TrackSelect_BuildRaceTracks();
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
		MM_TrackSelect_GetBattleTracks(&selectMenu, &numTracks);
	}
	else
	{
		selectMenu = s_raceTracks != NULL ? s_raceTracks : MM_ARCADE_TRACKS;
		numTracks = s_raceTrackCount;
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

// GCC 2.8.1 is sensitive to the lifetimes in this menu. This
// body states the same operations without relying on PSX pages or MIPS
// register contents.
void MM_TrackSelect_MenuProc(struct RectMenu *menu)
{
	struct RectMenu *menuLocal;
	struct MainMenu_LevelRow *selectMenu;
	s16 currTrack;
	u16 numTracks;
	u32 trackCountWork;
	register s32 trackChangeFrames CTR_PSX_REGISTER("$2");
	register struct TransitionMeta *levelTitleTransition CTR_PSX_REGISTER("$16");
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
		enteringFramePage = 0;
		enteringFrame = MM_TRACK_TRANSITION_FRAME;
		slideFrames = MM_TRACK_SELECT_SLIDE_FRAMES;
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
					if (NativeTrackRegistry_Get(NativeTrackLibrary_Current(), GAME_TRACKER->currLEV) != NULL)
					{
						MM_GHOST_TAPE_PLAYING = MM_ALLOC_HIGH_MEM(MM_TRACK_SELECT_GHOST_TAPE_ALLOC_SIZE, MM_LOADED_GHOST_DATA);
						if (MM_GHOST_TAPE_PLAYING != NULL)
						{
							memset(MM_GHOST_TAPE_PLAYING, 0, MM_TRACK_SELECT_GHOST_TAPE_ALLOC_SIZE);
							MM_REPLAY_HUMAN_GHOST = GAMEPROG_CustomLoadGhost(GAME_TRACKER->currLEV, MM_GHOST_TAPE_PLAYING);
							if (MM_REPLAY_HUMAN_GHOST)
							{
								data.characterIDs[1] = MM_GHOST_TAPE_PLAYING->characterID;
							}
						}
						else
						{
							MM_REPLAY_HUMAN_GHOST = 0;
						}
						MM_DESIRED_MENU = &MM_MENU_QUEUE_LOAD_TRACK;
						return;
					}
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

	// Battle uses arena tracks and can include race tracks when enabled.
	if ((GAME_TRACKER->gameMode1 & BATTLE_MODE) != 0)
	{
		register u32 trackTablePage CTR_PSX_REGISTER("$2");
		register s16 trackCountValue CTR_PSX_REGISTER("$12");
		trackTablePage = MM_OVERLAY_PAGE_VALUE;
		{
			s16 battleTrackCount;

			MM_TrackSelect_GetBattleTracks(&selectMenu, &battleTrackCount);
			trackCountValue = battleTrackCount;
		}
		(void)trackTablePage;
		CTR_PSX_KEEP_VALUE_RELAXED(trackCountValue);
		numTracks = trackCountValue;
	}
	else
	{
		register u32 trackTablePage CTR_PSX_REGISTER("$2");
		register s16 trackCountValue CTR_PSX_REGISTER("$13");
		trackTablePage = 0;
		selectMenu = s_raceTracks != NULL ? s_raceTracks : MM_ARCADE_TRACKS;
		(void)trackTablePage;
		currTrack = s_raceTrackCount;
		trackCountValue = currTrack;
		CTR_PSX_KEEP_VALUE_RELAXED(trackCountValue);
		numTracks = trackCountValue;
	}
	{
		register u32 trackStatePage CTR_PSX_REGISTER("$2");
		register struct RectMenu *menuView CTR_PSX_REGISTER("$14");
		register s32 trackChangeValue CTR_PSX_REGISTER("$3");
		trackStatePage = 0;
		trackChangeValue = MM_TRACK_CHANGE_FRAMES;
		menuView = menuLocal;
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
		lapReloadPage = 0;
		if (CTR_PSX_PAGE_LVALUE(s16, lapReloadPage, MM_TRACK_LAP_BOX_OPEN_PAGE_OFFSET, MM_TRACK_LAP_BOX_OPEN) != 0)
		{
			s16 lapSelTransitionState;
			{
				register u32 lapMenuPage CTR_PSX_REGISTER("$3");
				lapMenuPage = MM_OVERLAY_PAGE_VALUE;
				CTR_PSX_KEEP_VALUE(lapMenuPage);
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
		CTR_PSX_KEEP_VALUE(trackChangePage);
		trackChangeFrames = CTR_PSX_PAGE_LVALUE(s32, trackChangePage, MM_TRACK_CHANGE_FRAMES_PAGE_OFFSET, MM_TRACK_CHANGE_FRAMES);
		if (0 < trackChangeFrames)
		{
			trackChangeFrames--;
			CTR_PSX_PAGE_LVALUE(s32, trackChangePage, MM_TRACK_CHANGE_FRAMES_PAGE_OFFSET, MM_TRACK_CHANGE_FRAMES) = trackChangeFrames;
			if (trackChangeFrames == 0)
			{
				register u32 currentTrack CTR_PSX_REGISTER("$2");
				register struct RectMenu *menuUpdate CTR_PSX_REGISTER("$13");
				currentTrack = MM_TRACK_CURRENT_TRACK;
				menuUpdate = menuLocal;
				menuUpdate->rowSelected = currentTrack;
			}
		}
	}

	if (MM_TRACK_CHANGE_FRAMES != 0 || s_titleScrollSelectedRow != menuLocal->rowSelected)
	{
		s_titleScrollSelectedRow = menuLocal->rowSelected;
		s_titleScrollStartFrame = (u32)MM_FRAME_COUNTER;
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
		scanTracker = GAME_TRACKER;
		trackCount = numTracks;
		scanMenu = menuLocal;
		selectedTrack = (s16)scanMenu->rowSelected;
		lastTrack = trackCount - 1;
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
		gameTrackerPage = 0;
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
			{
				s32 angleStep;
				s32 angleOffset;
				angleStep = ((MM_TRACK_SELECT_TRACK_CHANGE_FRAMES - changeFrames) * MM_TRACK_SELECT_ROW_ANGLE_STEP) / MM_TRACK_SELECT_TRACK_CHANGE_FRAMES;
				angleOffset = angleStep * MM_TRACK_CHANGE_DIRECTION;
				rowAngle = rowBaseAngle + angleOffset;
			}
		trackRowAngleReady:
			trigIndex = (rowAngle & (((0x1000 / 2) / 2) - 1)) * (sizeof(*trigTable));

			trigTable = MM_TRIG_APPROX;
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
			rowTransition = MM_TRACK_TRANSITIONS;
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
			if ((rowTracker->gameMode1 & TIME_TRIAL) != 0 &&
			    LevelInfo_Lookup(selectMenu[currTrack].levID).kind == LEVEL_INFO_RETAIL)
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
					beatenFlagBit = MM_TIME_TRIAL_STAR_FLAGS[starIndex];
					timeTrialFlags = GAME_PROGRESS.highScoreTracks[rowTracker->levelID].timeTrialFlags;
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
						{
							register u16 *starColors CTR_PSX_REGISTER("$2");
							starColors = MM_TIME_TRIAL_STAR_COLORS;
							starColorIndex = starColors[starIndex];
						}
						starX = (s16)rowX;
						starX += MM_TRACK_SELECT_ROW_W + MM_TRACK_SELECT_STAR_X_OFFSET;
						starY = starIndex * MM_TRACK_SELECT_STAR_Y_STEP;
						starY += MM_TRACK_SELECT_STAR_Y_OFFSET;
						starTracker = CTR_PSX_PAGE_LVALUE(struct GameTracker *, gameTrackerPage, MM_GAME_TRACKER_PAGE_OFFSET, GAME_TRACKER);
						starOt = starTracker->pushBuffer_UI.ptrOT;
						starIconGroup = starTracker->iconGroup[MM_TRACK_SELECT_TT_STAR_ICON_GROUP];
						starBackBuffer = starTracker->backBuffer;
						iconPtrArray = (struct Icon **)(((u32)starIconGroup) + (sizeof(struct IconGroup)));
						colorPtr = &MM_COLOR_POINTERS[starColorIndex];
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
				struct MainMenu_LevelRow *trackEntry;
				register s32 trackLevelID CTR_PSX_REGISTER("$3");
				register s32 rowNameX CTR_PSX_REGISTER("$5");
				register s32 rowNameY CTR_PSX_REGISTER("$6");
				uiPage = 0;
				rowNameX = (s16)(rowX + MM_TRACK_SELECT_ROW_NAME_X_OFFSET);
				trackEntry = &selectMenu[currTrack];
				rowNameY = (s16)(rowY + MM_TRACK_SELECT_ROW_NAME_Y_OFFSET - MM_TRACK_SELECT_ROW_Y_OFFSET);
				trackLevelID = trackEntry->levID;
				struct LevelInfo levelInfo = LevelInfo_Lookup(trackLevelID);
				const struct NativeTrackMetadata *customTrack = levelInfo.custom;
				const char *titleText = LevelInfo_Title(&levelInfo, CTR_PSX_PAGE_LVALUE(char **, uiPage, MM_LANGUAGE_STRINGS_PAGE_OFFSET, GAME_LANGUAGE_STRINGS));
				if (titleText == NULL) { titleText = ""; }
				if (customTrack != NULL)
				{
					char title[MM_TRACK_SELECT_CUSTOM_TITLE_BYTES];
					TrackText_Copy(title, sizeof(title), titleText);
					u32 scrollFrame = 0;
					if (MM_TRACK_CHANGE_FRAMES == 0 && (s16)rowIndex == MM_TRACK_SELECT_CENTER_ROW)
					{
						scrollFrame = (u32)MM_FRAME_COUNTER - s_titleScrollStartFrame;
					}
					MM_TrackSelect_ScrollTitle(title, MM_TRACK_SELECT_ROW_W - MM_TRACK_SELECT_CUSTOM_TITLE_MARGIN, scrollFrame);
					MM_DRAW_LINE_WIDE_FLAGS(title, rowNameX, rowNameY, FONT_BIG, ORANGE);
				}
				else
				{
					MM_DRAW_LINE_WIDE_FLAGS((char *)titleText, rowNameX, rowNameY, FONT_BIG, ORANGE);
				}
				rowDrawRectArg = &rowRect;
				if ((MM_TRACK_CHANGE_FRAMES == 0) && (((s16)rowIndex) == MM_TRACK_SELECT_CENTER_ROW))
				{
					RECT16 highlightRect;
					register RECT16 *clearRectArg CTR_PSX_REGISTER("$4");
					register u32 highlightColorPage CTR_PSX_REGISTER("$5");
					register struct GameTracker *highlightTracker CTR_PSX_REGISTER("$2");
					if (customTrack != NULL && customTrack->author != NULL)
					{
						char author[MM_TRACK_SELECT_CUSTOM_TITLE_BYTES];
						memcpy(author, "by ", sizeof("by ") - 1);
						TrackText_Copy(author + sizeof("by ") - 1, sizeof(author) - (sizeof("by ") - 1), customTrack->author);
						TrackText_Trim(author, MM_TRACK_SELECT_ROW_W - MM_TRACK_SELECT_CUSTOM_TITLE_MARGIN, FONT_SMALL);
						MM_DRAW_LINE_WIDE_FLAGS(author, (s16)(rowX + MM_TRACK_SELECT_ROW_NAME_X_OFFSET + MM_TRACK_SELECT_GHOST_TEXT_FROM_NAME_X),
						                        (s16)(rowRect.y - data.font_charPixHeight[FONT_SMALL] + rowRect.h -
						                              (MM_TRACK_SELECT_GHOST_TEXT_Y_OFFSET - MM_TRACK_SELECT_ROW_Y_OFFSET)),
						                        FONT_SMALL, JUSTIFY_CENTER | PERIWINKLE);
					}
					if ((CTR_PSX_PAGE_LVALUE(struct GameTracker *, gameTrackerPage, MM_GAME_TRACKER_PAGE_OFFSET, GAME_TRACKER)->gameMode1 & TIME_TRIAL) != 0 &&
					    levelInfo.kind == LEVEL_INFO_RETAIL)
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
								ghostTextYCalc = rowY + MM_TRACK_SELECT_GHOST_TEXT_Y_OFFSET - MM_TRACK_SELECT_ROW_Y_OFFSET;
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
				int customMap = MM_TrackSelect_CustomMap(selectMenu[menuLocal->rowSelected].levID);
				previewRect.w = MM_TRACK_VIDEO_WIDTH;
				previewRect.h = MM_TRACK_VIDEO_HEIGHT;
				previewX = MM_TRACK_TRANSITIONS[1].currX;
				previewY = MM_TRACK_TRANSITIONS[1].currY;
				previewRect.x = previewX + MM_TRACK_SELECT_PREVIEW_X;
				previewRect.y = previewY + MM_TRACK_SELECT_PREVIEW_Y_NO_MAP;
				CTR_PSX_MEMORY_BARRIER();
				{
					register struct RectMenu *previewMenu CTR_PSX_REGISTER("$12");
					previewMenu = menuLocal;
					CTR_PSX_KEEP_VALUE(previewMenu);
					if (customMap || (-1) < selectMenu[previewMenu->rowSelected].mapTextureID)
					{
						previewRect.y = MM_TRACK_PREVIEW_Y + MM_TRACK_SELECT_PREVIEW_Y_WITH_MAP;
					}
				}
				{
					register u32 previewLapPage CTR_PSX_REGISTER("$19");
					register s32 titleFont CTR_PSX_REGISTER("$7");
					previewLapPage = 0;
					titleFont = FONT_BIG;
					(void)titleFont;
					if (CTR_PSX_PAGE_LVALUE(s16, previewLapPage, MM_TRACK_LAP_BOX_OPEN_PAGE_OFFSET, MM_TRACK_LAP_BOX_OPEN) == 0)
					{
						levelTitleTransition = MM_TRACK_TRANSITIONS;
						DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_SELECT_LEVEL_SELECT], levelTitleTransition[3].currX + MM_TRACK_SELECT_TITLE_X,
						                   levelTitleTransition[3].currY + (u16)previewRect.y, FONT_BIG, JUSTIFY_CENTER | ORANGE);
						DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_LEVEL], levelTitleTransition[3].currX + MM_TRACK_SELECT_TITLE_X,
						                   levelTitleTransition[3].currY + (u16)previewRect.y + MM_TRACK_SELECT_LEVEL_TEXT_Y_STEP, FONT_BIG,
						                   JUSTIFY_CENTER | ORANGE);
					}
					previewRect.y += MM_TRACK_SELECT_TITLE_TO_MAP_Y;
					if (customMap && CTR_PSX_PAGE_LVALUE(s16, previewLapPage, MM_TRACK_LAP_BOX_OPEN_PAGE_OFFSET, MM_TRACK_LAP_BOX_OPEN) == 0)
					{
						int mapX = previewRect.x + MM_TRACK_TRANSITIONS[2].currX - MM_TRACK_TRANSITIONS[1].currX;
						int mapY = previewRect.y + MM_TRACK_TRANSITIONS[2].currY - MM_TRACK_TRANSITIONS[1].currY;
						MM_TrackSelect_DrawCustomMap(mapX + previewRect.w / 2,
						                             mapY + MM_TRACK_SELECT_MAP_CENTER_Y_OFFSET + MM_TRACK_SELECT_MAP_BOX_H / 2);
					}
					{
						register struct RectMenu *mapCheckMenu CTR_PSX_REGISTER("$13");
						mapCheckMenu = menuLocal;
						CTR_PSX_KEEP_VALUE(mapCheckMenu);
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
							s16 mapHeight;
							register s32 mapBoxHeight CTR_PSX_REGISTER("$2");
							struct TextureLayout *mapLayout;
							RECT16 mapRect;
							mapBoxHeight = MM_TRACK_SELECT_MAP_BOX_H;
							mapTransitions = MM_TRACK_TRANSITIONS;
							mapRect = previewRect;
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
									mapTrackerPage = 0;
									mapRectY = (u16)mapRect.y;
									mapToY -= mapFromY;
									mapRectY += mapToY;
									mapRect.y = mapRectY;
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
							MM_TrackSelect_DrawMapLayers(iconMap0, iconMap1,
							                             (s16)mapRect.x + (s16)mapRect.w / 2 + mapWidth / 2,
							                             (s16)mapRect.y + (s16)mapRect.h / 2 + mapHeight / 2);
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
