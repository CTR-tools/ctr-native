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
	MM_TRACK_SELECT_ROW_W = 0x100,
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
	MM_TRACK_SELECT_ROW_NAME_X_OFFSET = 8,
	MM_TRACK_SELECT_CUSTOM_TITLE_BYTES = 256,
	MM_TRACK_SELECT_CUSTOM_TITLE_MARGIN = 16,
	MM_TRACK_SELECT_STAR_X_OFFSET = 4,
	MM_TRACK_SELECT_STAR_Y_OFFSET = 4,
	MM_TRACK_SELECT_STAR_Y_STEP = 8,
	MM_TRACK_SELECT_GHOST_FLASH_FRAME_BIT = 4,
	MM_TRACK_SELECT_GHOST_TEXT_FROM_NAME_X = 0x78,
	MM_TRACK_SELECT_GHOST_TEXT_Y_OFFSET = 0x76,
	MM_TRACK_SELECT_TITLE_X = 0x18c,
	MM_TRACK_SELECT_LEVEL_TEXT_Y_STEP = 0x10,
	MM_TRACK_SELECT_TITLE_TO_MAP_Y = 0x22,
	MM_TRACK_SELECT_MAP_CENTER_Y_OFFSET = 0x49,
	MM_TRACK_SELECT_INPUT_BACK = BTN_TRIANGLE | BTN_SQUARE_one,
};

#define MM_TRACK_GAME_TRACKER(page) CTR_PSX_PAGE_LVALUE(struct GameTracker *, page, MM_GAME_TRACKER_PAGE_OFFSET, GAME_TRACKER)

static struct MainMenu_LevelRow s_combinedBattleTracks[MM_TRACK_SELECT_COMBINED_BATTLE_TRACK_COUNT];
static struct MainMenu_LevelRow *s_raceTracks;
static s16 s_raceTrackCount;
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
		unsigned int mode = (GAME_TRACKER->gameMode1 & TIME_TRIAL) != 0 ? NATIVE_TRACK_DETECTED_TIME_TRIAL :
		                    (GAME_TRACKER->gameMode1 & ARCADE_MODE) != 0 ? NATIVE_TRACK_DETECTED_ARCADE : NATIVE_TRACK_DETECTED_VS;
		b32 allowed = NativeTrackLibrary_AllowsDetectedMode(track, mode);
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
			DecalFont_DrawLine((char *)LANG_GetString(LNG_EXPANDED_NO_PREVIEW), r->x + r->w / 2, r->y + r->h / 2, FONT_SMALL, JUSTIFY_CENTER | WHITE);
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

struct TrackCarouselContext
{
	struct MainMenu_LevelRow *tracks;
	struct RectMenu *menu;
};

static struct MM_Carousel s_trackCarousel;
static struct
{
	bool active;
	bool done;
	bool confirmed;
	int result;
	int previousLevel;
	int previousSelection;
	int previousMenuRow;
	int previousErrorPosition;
} s_trackPicker;

static bool MM_TrackCarousel_Enabled(void *context, int index)
{
	struct TrackCarouselContext *track = context;
	return MM_TrackSelect_boolTrackOpen(&track->tracks[index]);
}

static const char *MM_TrackCarousel_Label(void *context, int index, char *buffer, size_t capacity)
{
	(void)buffer;
	(void)capacity;
	struct TrackCarouselContext *track = context;
	struct LevelInfo info = LevelInfo_Lookup(track->tracks[index].levID);
	return LevelInfo_Title(&info, GAME_LANGUAGE_STRINGS);
}

static bool MM_TrackCarousel_IsRetail(void *context, int index)
{
	struct TrackCarouselContext *track = context;
	return LevelInfo_Lookup(track->tracks[index].levID).custom == NULL;
}

static void MM_TrackCarousel_Selected(void *context, int index)
{
	struct TrackCarouselContext *track = context;
	track->menu->rowSelected = index;
	GAME_TRACKER->currLEV = track->tracks[index].levID;
	MM_TRACK_SEL_BACKUP = index;
}

static void MM_TrackCarousel_Confirm(void *context, int index)
{
	(void)context;
	(void)index;
	if (s_trackPicker.active)
	{
		MM_TRACK_TRANSITION_START_AFTER_EXIT = 1;
		MM_TRACK_TRANSITION_STATE = EXITING_MENU;
		return;
	}
	if ((GAME_TRACKER->gameMode1 & (BATTLE_MODE | TIME_TRIAL)) != 0)
	{
		MM_TRACK_TRANSITION_START_AFTER_EXIT = 1;
		MM_TRACK_TRANSITION_STATE = EXITING_MENU;
	}
	else
	{
		MM_TRACK_LAP_BOX_OPEN = 1;
	}
}

static void MM_TrackCarousel_Decorate(void *context, int index, const RECT16 *row, bool selected, u32 scrollFrame)
{
	struct TrackCarouselContext *track = context;
	struct GameTracker *gGT = GAME_TRACKER;
	int levelID = track->tracks[index].levID;
	struct LevelInfo info = LevelInfo_Lookup(levelID);
	if ((gGT->gameMode1 & TIME_TRIAL) != 0 && info.kind == LEVEL_INFO_RETAIL)
	{
		int previous = gGT->levelID;
		gGT->levelID = levelID;
		GAMEPROG_GetPtrHighScoreTrack();
		for (int i = 0; i < MM_TRACK_SELECT_TT_STAR_COUNT; i++)
		{
			if ((GAME_PROGRESS.highScoreTracks[gGT->levelID].timeTrialFlags & (1u << MM_TIME_TRIAL_STAR_FLAGS[i])) != 0)
			{
				Color *colors = (Color *)MM_COLOR_POINTERS[MM_TIME_TRIAL_STAR_COLORS[i]];
				struct Icon **icons = (struct Icon **)((u32)gGT->iconGroup[MM_TRACK_SELECT_TT_STAR_ICON_GROUP] + sizeof(struct IconGroup));
				MM_DECALHUD_DRAW_POLY_GT4(icons[MM_TRACK_SELECT_TT_STAR_ICON],
				                          row->x + MM_TRACK_SELECT_ROW_W + MM_TRACK_SELECT_STAR_X_OFFSET,
				                          row->y + MM_TRACK_SELECT_STAR_Y_OFFSET + i * MM_TRACK_SELECT_STAR_Y_STEP,
				                          &gGT->backBuffer->primMem, gGT->pushBuffer_UI.ptrOT,
				                          colors[0], colors[1], colors[2], colors[3], 0, FP(1.0));
			}
		}
		gGT->levelID = previous;
		GAMEPROG_GetPtrHighScoreTrack();
	}
	if (!selected) { return; }
	if (info.custom != NULL && info.custom->author != NULL)
	{
		char author[MM_TRACK_SELECT_CUSTOM_TITLE_BYTES];
		memcpy(author, "by ", 3);
		TrackText_Copy(author + 3, sizeof(author) - 3, info.custom->author);
		TrackText_Scroll(author, MM_TRACK_SELECT_ROW_W - MM_TRACK_SELECT_CUSTOM_TITLE_MARGIN, FONT_SMALL, scrollFrame);
		MM_DRAW_LINE_WIDE_FLAGS(author, row->x + MM_TRACK_SELECT_ROW_NAME_X_OFFSET + MM_TRACK_SELECT_GHOST_TEXT_FROM_NAME_X,
		                        row->y - data.font_charPixHeight[FONT_SMALL] + row->h -
		                            (MM_TRACK_SELECT_GHOST_TEXT_Y_OFFSET - MM_TRACK_SELECT_ROW_Y_OFFSET),
		                        FONT_SMALL, JUSTIFY_CENTER | PERIWINKLE);
	}
	if ((gGT->gameMode1 & TIME_TRIAL) != 0 && info.kind == LEVEL_INFO_RETAIL && RefreshCard_CountGhostProfilesForLEV(levelID) != 0)
	{
		int flags = (MM_FRAME_COUNTER & MM_TRACK_SELECT_GHOST_FLASH_FRAME_BIT) != 0 ? -0x7fff : -0x7ffc;
		MM_DRAW_LINE_WIDE_FLAGS(GAME_LANGUAGE_STRINGS[LNG_GHOST_DATA_EXISTS],
		                        row->x + MM_TRACK_SELECT_ROW_NAME_X_OFFSET + MM_TRACK_SELECT_GHOST_TEXT_FROM_NAME_X,
		                        row->y + MM_TRACK_SELECT_GHOST_TEXT_Y_OFFSET - MM_TRACK_SELECT_ROW_Y_OFFSET, FONT_SMALL, flags);
	}
}

static struct MM_CarouselSource MM_TrackCarousel_Source(struct TrackCarouselContext *context, int count)
{
	struct MM_CarouselSource source = {
		.context = context, .count = count, .enabled = MM_TrackCarousel_Enabled,
		.label = MM_TrackCarousel_Label, .labelIsFontEncoded = MM_TrackCarousel_IsRetail,
		.decorate = MM_TrackCarousel_Decorate, .selected = MM_TrackCarousel_Selected, .confirm = MM_TrackCarousel_Confirm,
	};
	return source;
}

void MM_TrackSelect_Init(void)
{
	struct MainMenu_LevelRow *selectMenu;
	s16 numTracks;

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

	struct TrackCarouselContext carouselContext = {selectMenu, &MM_MENU_TRACK_SELECT};
	struct MM_CarouselSource carouselSource = MM_TrackCarousel_Source(&carouselContext, numTracks);
	MM_Carousel_Init(&s_trackCarousel, &carouselSource, MM_MENU_TRACK_SELECT.rowSelected, (u32)MM_FRAME_COUNTER);

	MM_TrackSelect_Video_SetDefaults();
}

static void MM_TrackSelect_OpenPicker(int initialLevel)
{
	s_trackPicker.previousLevel = GAME_TRACKER->currLEV;
	s_trackPicker.previousSelection = MM_TRACK_SEL_BACKUP;
	s_trackPicker.previousMenuRow = MM_MENU_TRACK_SELECT.rowSelected;
	s_trackPicker.previousErrorPosition = MM_ERROR_MESSAGE_POS_INDEX;
	s_trackPicker.active = true;
	s_trackPicker.done = false;
	MM_TRACK_SEL_BACKUP = 0;
	MM_TrackSelect_Init();
	struct MainMenu_LevelRow *tracks = s_raceTracks != NULL ? s_raceTracks : MM_ARCADE_TRACKS;
	int selected = 0;
	for (int i = 0; i < s_raceTrackCount; i++)
	{
		if (tracks[i].levID == initialLevel) { selected = i; break; }
	}
	struct TrackCarouselContext context = {tracks, &MM_MENU_TRACK_SELECT};
	struct MM_CarouselSource source = MM_TrackCarousel_Source(&context, s_raceTrackCount);
	MM_Carousel_Init(&s_trackCarousel, &source, selected, (u32)MM_FRAME_COUNTER);
	RECTMENU_ClearInput();
}

static void MM_TrackSelect_FinishPicker(bool confirmed)
{
	s_trackPicker.result = GAME_TRACKER->currLEV;
	s_trackPicker.confirmed = confirmed;
	s_trackPicker.done = true;
	s_trackPicker.active = false;
	if (MM_TRACK_VIDEO_MEM_ALLOCATED != 0) { NativeSTR_Stop(); }
	MM_TrackSelect_Video_SetDefaults();
	GAME_TRACKER->currLEV = s_trackPicker.previousLevel;
	MM_TRACK_SEL_BACKUP = s_trackPicker.previousSelection;
	MM_MENU_TRACK_SELECT.rowSelected = s_trackPicker.previousMenuRow;
	MM_ERROR_MESSAGE_POS_INDEX = s_trackPicker.previousErrorPosition;
	RECTMENU_ClearInput();
}

static void MM_TrackSelect_DrawPreview(struct MainMenu_LevelRow *selectMenu, struct RectMenu *menuLocal,
                                       struct TransitionMeta *transitions, int next, bool lapOpen, bool exiting);

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
	register b32 resetPreviewVideo CTR_PSX_REGISTER("$4");
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
			if (s_trackPicker.active)
			{
				MM_TrackSelect_FinishPicker(MM_TRACK_TRANSITION_START_AFTER_EXIT != 0);
				return;
			}
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
	struct TrackCarouselContext carouselContext = {selectMenu, menuLocal};
	struct MM_CarouselSource carouselSource = MM_TrackCarousel_Source(&carouselContext, numTracks);
	bool carouselHandled = false;
	if (MM_TRACK_LAP_BOX_OPEN == 0 && MM_TRACK_TRANSITION_STATE == IN_MENU)
	{
		u32 buttons = MM_GAME_BUTTON_TAPS[0];
		if (s_trackCarousel.changeFrames == 0 && (buttons & MM_TRACK_SELECT_INPUT_BACK) != 0)
		{
			OtherFX_Play(2, 1);
			MM_TRACK_TRANSITION_START_AFTER_EXIT = 0;
			MM_TRACK_TRANSITION_STATE = EXITING_MENU;
			RECTMENU_ClearInput();
		}
		else if (MM_Carousel_Update(&s_trackCarousel, &carouselSource, buttons, (u32)MM_FRAME_COUNTER))
		{
			carouselHandled = true;
			RECTMENU_ClearInput();
		}
	}
	else if (s_trackCarousel.changeFrames > 0)
	{
		MM_Carousel_Update(&s_trackCarousel, &carouselSource, 0, (u32)MM_FRAME_COUNTER);
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
			lapSelTransitionState = 0;
			MM_MENU_LAP_SELECT.rowSelected = MM_USELESS_LAP_ROW_COPY;
			if (MM_TRACK_TRANSITION_STATE == IN_MENU && !carouselHandled)
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
	resetPreviewVideo = s_trackCarousel.changeFrames != 0 || MM_TRACK_TRANSITION_STATE == EXITING_MENU;
	MM_TrackSelect_Video_State(resetPreviewVideo);
	MM_Carousel_Draw(&s_trackCarousel, &carouselSource, GAME_TRACKER, MM_TRACK_TRANSITIONS[0].currX,
	                 MM_TRACK_TRANSITIONS[0].currY, (u32)MM_FRAME_COUNTER);
	MM_TrackSelect_DrawPreview(selectMenu, menuLocal, MM_TRACK_TRANSITIONS, s_trackCarousel.next,
	                           MM_TRACK_LAP_BOX_OPEN != 0, MM_TRACK_TRANSITION_STATE == EXITING_MENU);
}

static void MM_TrackSelect_DrawPreview(struct MainMenu_LevelRow *selectMenu, struct RectMenu *menuLocal,
                                       struct TransitionMeta *transitions, int next, bool lapOpen, bool exiting)
{
	register struct TransitionMeta *levelTitleTransition CTR_PSX_REGISTER("$16");
	{
				// The preview, title, and minimap are drawn after the row list.
				register s32 previewX CTR_PSX_REGISTER("$3");
				register s32 previewY CTR_PSX_REGISTER("$2");
				RECT16 previewRect;
				int customMap = MM_TrackSelect_CustomMap(selectMenu[menuLocal->rowSelected].levID);
				previewRect.w = MM_TRACK_VIDEO_WIDTH;
				previewRect.h = MM_TRACK_VIDEO_HEIGHT;
				previewX = transitions[1].currX;
				previewY = transitions[1].currY;
				previewRect.x = previewX + MM_TRACK_SELECT_PREVIEW_X;
				previewRect.y = previewY + MM_TRACK_SELECT_PREVIEW_Y_NO_MAP;
				CTR_PSX_MEMORY_BARRIER();
				{
					register struct RectMenu *previewMenu CTR_PSX_REGISTER("$12");
					previewMenu = menuLocal;
					CTR_PSX_KEEP_VALUE(previewMenu);
					if (customMap || (-1) < selectMenu[previewMenu->rowSelected].mapTextureID)
					{
						previewRect.y = transitions[1].currY + MM_TRACK_SELECT_PREVIEW_Y_WITH_MAP;
					}
				}
				{
					register s32 titleFont CTR_PSX_REGISTER("$7");
					titleFont = FONT_BIG;
					(void)titleFont;
					if (!lapOpen)
					{
						levelTitleTransition = transitions;
						DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_SELECT_LEVEL_SELECT], levelTitleTransition[3].currX + MM_TRACK_SELECT_TITLE_X,
						                   levelTitleTransition[3].currY + (u16)previewRect.y, FONT_BIG, JUSTIFY_CENTER | ORANGE);
						DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_LEVEL], levelTitleTransition[3].currX + MM_TRACK_SELECT_TITLE_X,
						                   levelTitleTransition[3].currY + (u16)previewRect.y + MM_TRACK_SELECT_LEVEL_TEXT_Y_STEP, FONT_BIG,
						                   JUSTIFY_CENTER | ORANGE);
					}
					previewRect.y += MM_TRACK_SELECT_TITLE_TO_MAP_Y;
					if (customMap && !lapOpen)
					{
						int mapX = previewRect.x + transitions[2].currX - transitions[1].currX;
						int mapY = previewRect.y + transitions[2].currY - transitions[1].currY;
						MM_TrackSelect_DrawCustomMap(mapX + previewRect.w / 2,
						                             mapY + MM_TRACK_SELECT_MAP_CENTER_Y_OFFSET + MM_TRACK_SELECT_MAP_BOX_H / 2);
					}
					{
						register struct RectMenu *mapCheckMenu CTR_PSX_REGISTER("$13");
						mapCheckMenu = menuLocal;
						CTR_PSX_KEEP_VALUE(mapCheckMenu);
						if (((-1) < selectMenu[mapCheckMenu->rowSelected].mapTextureID) &&
						    !lapOpen)
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
							mapTransitions = transitions;
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
				MM_TrackSelect_Video_Draw(&previewRect, selectMenu, (s16)next, exiting, 0);
				return;
	}

}

struct RectMenu *MM_TrackSelect_GetMenuPtr(void)
{
	return &MM_MENU_TRACK_SELECT;
}
