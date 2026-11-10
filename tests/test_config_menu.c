#include <shared/ctr_test.h>
#include <common.h>
#include <platform/native_track_registry.h>
#include <platform/native_text_edit.h>

struct sData sdata_static;
struct Data data;
struct OverlayDATA_232 D232;
static int s_promptDraws, s_cursorDraws;
static int s_drawOrder, s_promptOrder, s_promptBoxOrder, s_frameOrder, s_promptY;
static RECT s_promptRect;
s32 DecalFont_DrawMultiLine(char *text, s16 x, s16 y, s16 width, s16 font, s16 flags)
{
	(void)text; (void)x; (void)width; (void)font; (void)flags;
	s_promptY = y;
	s_promptOrder = ++s_drawOrder;
	s_promptDraws++;
	return 8;
}
#include <platform/native_config.c>

static struct GameTracker s_tracker;
static struct DB s_buffer;
static int s_configSaves;
static int s_controlsSaves;
static int s_languageApplied;
static int s_textCatalogReloaded;
static int s_voiceRefreshed;
static int s_textExportCalls;
static int s_europeTextAvailable = 1;
static bool s_recordLanguageDraw;
static int s_languageDrawSequence;
static int s_languagePopupDraw;
static int s_languageUnderlyingDraw;
static bool s_capturing;
static bool s_pickerOpen;
static NativeConfig s_initialConfig;
static struct RectMenu s_caller;
static const char *s_observedLabel;
static bool s_observingRow;
static bool s_valueDrawn;
static int s_valueFlags;
static char s_valueText[64];
static bool s_sliderDrawn;
static RECT s_sliderMarker;
static s16 s_sliderTriangle[6];
static int s_sliderBoxCount;
static s16 s_centerX;
static int s_frameDrawStyle;
static int s_volumes[GAME_OPTIONS_VOLUME_COUNT];
static int s_stereo;
static int s_previewRow;
static bool s_previewPlaying;
static char *s_strings[LNG_STEREO + 1];
static bool s_hubLoaded;
static int s_hubDestroys;
static int s_hubUpdates;
static int s_unpauses;
static int s_backgroundDeactivations;
static int s_trackImportOpened;
static int s_trackRefreshCalls;
static int s_trackRefreshSuccess = 1;

const char *LANG_GetString(s32 index)
{
	switch (index)
	{
	case LNG_EXPANDED_TRACKS_REFRESH_SUMMARY: return "TRACKS: {tracks}  REJECTED: {rejected}";
	case LNG_EXPANDED_TRACKS_IMPORT: return "Import";
	case LNG_EXPANDED_TRACKS_AUTHOR: return "Author";
	case LNG_EXPANDED_TRACKS_MODES: return "Modes";
	case LNG_EXPANDED_TRACKS_UNKNOWN: return "Unknown";
	case LNG_EXPANDED_TRACKS_NONE_IMPORTED: return "No tracks imported";
	case LNG_EXPANDED_TRACKS_REFRESH_FAILED: return "Refresh failed";
	default: break;
	}
	static char values[LNG_EXPANDED_STRING_COUNT][48];
	if (index < LNG_EXPANDED_STRING_BASE || index >= LNG_EXPANDED_STRING_END)
	{
		return "";
	}
	char *value = values[index - LNG_EXPANDED_STRING_BASE];
	snprintf(value, sizeof(values[0]), "TEXT ID %d", (int)index);
	return value;
}
static int s_lastMenuSound = -1;
static int s_sectionFont = -1;
static int s_trackFixtureCount;
static int s_trackFixtureEnabled = 1;
static int s_trackDeleteSuccess;
static struct NativeTrackMetadata s_trackFixture = {.identity = "test:track", .title = "Test Track", .author = "Test Author", .detectedModes = NATIVE_TRACK_DETECTED_TIME_TRIAL, .detectionKnown = 1};
static enum NativeTextEditResult s_editResult;
static const char *s_editValue;
static char s_savedTitle[256], s_savedAuthor[256];
static int s_savedMusic = -1, s_musicPreviewRequests, s_musicPreviewRestores;
static int s_previewRetail;
static const struct NativeTrackMetadata *s_previewTrack;
static size_t s_editCharacterLimit;
static int s_trackNumberDraws;
static int s_trackDetailRows[2];
static int s_allModesY;
static int s_unknownModesY;
static int s_emptyTracksY;
static int s_importRowColor;
static struct RectMenu *s_chaosCaller;

// Count menu persistence calls without touching any real configuration file.
static void ConfigMenuTest_Save(void);
#define NativeConfig_Save ConfigMenuTest_Save
#include <game/230/MM_CustomTracks.c>
#include <game/230/MM_ConfigMenu.c>
#undef NativeConfig_Save
#include <game/230/MM_AudioConfig.c>
#include <game/UI/UI_VolumeSlider.c>
#include <game/RaceConfig.c>
#include <game/MAIN/MainFreeze.c>

// The menu has no public page/scroll accessor; inspect those internals only to
// distinguish staying at the section list from accidentally reopening a page.

static void ConfigMenuTest_Save(void) { s_configSaves++; }
void LANG_Refresh(int slot) { s_textCatalogReloaded = slot; }
const char *NativeAssets_GetBaseDir(void)
{
	CtrTest_Bail("menu tests must not access configuration files");
	return NULL;
}
const char *NativeAssets_GetAssetDir(void) { return "fixture-assets"; }
int NativeTrackLibrary_Music(const struct NativeTrackMetadata *track) { (void)track; return s_savedMusic; }
int NativeTrackLibrary_SetDetails(const struct NativeTrackMetadata *track, const char *title, const char *author)
{ (void)track; snprintf(s_savedTitle, sizeof(s_savedTitle), "%s", title); snprintf(s_savedAuthor, sizeof(s_savedAuthor), "%s", author); return 1; }
int NativeTrackLibrary_SetMusic(const struct NativeTrackMetadata *track, int retail, int custom)
{ (void)track; (void)custom; s_savedMusic = retail; return 1; }
int NativeTrackRegistry_Find(const struct NativeTrackRegistry *registry, const char *identity, int *level)
{ (void)registry; (void)identity; if (level != NULL) { *level = NATIVE_TRACK_CUSTOM_ID_BASE; } return 1; }
int NativeTextEdit_BeginForGame(const char *initial, size_t maxBytes) { (void)initial; (void)maxBytes; s_editResult = NATIVE_TEXT_EDIT_ACTIVE; return 1; }
void NativeTextEdit_LimitCharacters(size_t maxCharacters) { s_editCharacterLimit = maxCharacters; }
enum NativeTextEditResult NativeTextEdit_Result(void) { return s_editResult; }
const char *NativeTextEdit_Value(void) { return s_editValue; }
void NativeTextEdit_Close(void) {}
void MusicPreview_Request(int retail, const struct NativeTrackMetadata *track) { s_previewRetail = retail; s_previewTrack = track; s_musicPreviewRequests++; }
void MusicPreview_Restore(void) { s_musicPreviewRestores++; }
int NativeTrackLibrary_Delete(const struct NativeTrackMetadata *track, const char *directory)
{
	(void)track; (void)directory;
	return s_trackDeleteSuccess;
}
int NativeTrackLibrary_Refresh(const char *directory, struct NativeTrackScanResult *result)
{
	if (strcmp(directory, "fixture-assets/tracks") != 0) { CtrTest_Bail("unexpected track refresh path"); }
	s_trackRefreshCalls++;
	if (s_trackRefreshSuccess && result != NULL)
	{
		result->installed = 2;
		result->rejected = 1;
		s_trackFixtureCount = 2;
	}
	return s_trackRefreshSuccess;
}
void NativeControls_Save(void) { s_controlsSaves++; }
int NativeLanguageDisc_HasText(int slot) { return slot == 5; }
int NativeLanguageDisc_HasAnyText(void) { return s_europeTextAvailable; }
enum NativeTextExportResult NativeTextExport_Active(int slot, NativeTextExport_GetString getString)
{ (void)slot; (void)getString; s_textExportCalls++; return NATIVE_TEXT_EXPORT_OK; }
int NativeTextRuntime_HasRetail(int slot) { (void)slot; return 0; }
int NativeLanguageDisc_HasVoice(int slot) { return slot == 4; }
const char *LOAD_LanguageSelfName(int slot)
{
	switch (slot)
	{
	case NATIVE_LANGUAGE_ENGLISH: return "English";
	case NATIVE_LANGUAGE_GERMAN: return "DISC GERMAN";
	case NATIVE_LANGUAGE_ITALIAN: return "DISC ITALIAN";
	default: return NULL;
	}
}
int LOAD_ApplyLanguage(int slot) { s_languageApplied = slot; return 1; }
void NativeAudio_RefreshVoiceLanguage(void) { s_voiceRefreshed++; }
void GhostTape_Destroy(void) {}
void MainRaceTrack_RequestLoad(s16 level) { (void)level; }
b32 RaceFlag_IsFullyOnScreen(void) { return false; }
b32 RaceFlag_IsFullyOffScreen(void) { return true; }
void RaceFlag_BeginTransition(s32 direction) { (void)direction; }
void ElimBG_Activate(struct GameTracker *tracker) { (void)tracker; }
void ElimBG_Deactivate(struct GameTracker *tracker) { (void)tracker; s_backgroundDeactivations++; }
void MainFrame_TogglePauseAudio(b32 paused) { s_unpauses += !paused; }
void RECTMENU_Show(struct RectMenu *menu) { sdata->ptrDesiredMenu = menu; }
void RECTMENU_Hide(struct RectMenu *menu) { (void)menu; }
b32 LOAD_IsOpen_AdvHub(void) { return s_hubLoaded; }
void AH_Pause_Destroy(void) { s_hubDestroys++; }
void AH_Pause_Update(void) { s_hubUpdates++; }
b32 VehPickupItem_MaskBoolGoodGuy(struct Driver *driver) { (void)driver; return true; }
void MM_ChaosConfig_OpenReadOnly(struct RectMenu *menu) { s_chaosCaller = menu; }
void NativeControls_CancelCapture(void) { s_capturing = false; }
s32 howl_VolumeGet(s32 type) { return s_volumes[type]; }
void howl_VolumeSet(s32 type, u8 volume) { s_volumes[type] = volume; }
s32 howl_ModeGet(void) { return s_stereo; }
void howl_ModeSet(s32 mode) { s_stereo = mode; }
void OptionsMenu_TestSound(s32 row, b32 playing)
{
	s_previewRow = row;
	s_previewPlaying = playing;
}
void MM_MenuProc_Controls(struct RectMenu *menu) { (void)menu; }
void MM_TrackImport_Open(void) { s_trackImportOpened++; }
void MM_TrackImport_OpenPatch(void) { s_trackImportOpened += 10; }
void MM_TrackImport_Reset(void) {}
int MM_TrackImport_GetReview(struct TrackImportReviewView *view) { (void)view; return 0; }
int MM_TrackImport_Update(u32 buttons) { (void)buttons; return 0; }
const char *MM_TrackImport_Message(void) { return ""; }
const struct NativeTrackRegistry *NativeTrackLibrary_Current(void) { return NULL; }
size_t NativeTrackRegistry_Count(const struct NativeTrackRegistry *registry) { (void)registry; return (size_t)s_trackFixtureCount; }
const struct NativeTrackMetadata *NativeTrackRegistry_Get(const struct NativeTrackRegistry *registry, int levelID)
{
	(void)registry; return s_trackFixtureCount && levelID == NATIVE_TRACK_CUSTOM_ID_BASE ? &s_trackFixture : NULL;
}
int NativeTrackLibrary_IsEnabled(const struct NativeTrackMetadata *track) { (void)track; return s_trackFixtureEnabled; }
int NativeTrackLibrary_SetEnabled(const struct NativeTrackMetadata *track, int enabled) { (void)track; s_trackFixtureEnabled = enabled; return 1; }
b32 MM_Controls_IsPickerOpen(void) { return s_pickerOpen; }
u32 MM_Controls_MenuTapped(void) { return (u32)sdata->gGamepads->gamepad[0].buttonsTapped; }
void MM_Controls_ResetNavigation(void)
{
	s_pickerOpen = false;
	NativeControls_CancelCapture();
}

void RECTMENU_ClearInput(void)
{
	sdata->AnyPlayerTap = 0;
	sdata->AnyPlayerHold = 0;
	memset(sdata->buttonTapPerPlayer, 0, sizeof(sdata->buttonTapPerPlayer));
	memset(sdata->buttonHeldPerPlayer, 0, sizeof(sdata->buttonHeldPerPlayer));
}

int OtherFX_Play(u32 id, int flags) { s_lastMenuSound = (int)id; (void)flags; return 0; }
s32 DecalFont_GetLineWidth(char *text, s16 font) { (void)font; return (s32)strlen(text) * 8; }
void DecalFont_DrawLineOT(char *text, s16 x, s16 y, s16 font, s16 flags, u32 *ot)
{
	(void)y; (void)font; (void)ot;
	if (strcmp(text, "_") == 0) { s_cursorDraws++; }
	if (strcmp(text, "1 / 1") == 0) { s_trackNumberDraws++; }
	if (s_recordLanguageDraw && strcmp(text, "English") == 0 && s_languagePopupDraw == 0)
	{
		s_languagePopupDraw = ++s_languageDrawSequence;
	}
	if (s_recordLanguageDraw && strcmp(text, LANG_GetString(LNG_EXPANDED_CONFIG_AUDIO)) == 0 && s_languageUnderlyingDraw == 0)
	{
		s_languageUnderlyingDraw = ++s_languageDrawSequence;
	}
	if (strcmp(text, LANG_GetString(LNG_EXPANDED_CONFIG_SYSTEM)) == 0)
	{
		s_sectionFont = font;
	}
	if (strcmp(text, "Import") == 0 && x == FULLSCREEN_MENU_LABEL_X)
	{
		s_importRowColor = flags;
	}
	if (strcmp(text, "Modes: Unknown") == 0 && x == FULLSCREEN_MENU_LABEL_X + 12)
	{
		s_unknownModesY = y;
	}
	if (strcmp(text, "No tracks imported") == 0 && x == FULLSCREEN_MENU_LABEL_X)
	{
		s_emptyTracksY = y;
	}
	if (strcmp(text, "Modes: TT, ARC, VS, CTR, REL, CRY") == 0 && x == FULLSCREEN_MENU_LABEL_X + 12)
	{
		s_allModesY = y;
	}
	const char *details[] = {"Author: Test Author", "Modes: TT"};
	for (int i = 0; i < 2; i++)
	{
		if (strcmp(text, details[i]) == 0 && x == FULLSCREEN_MENU_LABEL_X + 12)
		{
			s_trackDetailRows[i] = y;
		}
	}
	if ((flags & JUSTIFY_CENTER) != 0)
	{
		s_centerX = x;
	}
	if (x == FULLSCREEN_MENU_LABEL_X)
	{
		s_observingRow = s_observedLabel != NULL && strcmp(text, s_observedLabel) == 0;
		s_sliderBoxCount = 0;
	}
	else if (x == FULLSCREEN_MENU_VALUE_X && s_observingRow)
	{
		s_valueDrawn = true;
		s_valueFlags = flags;
		snprintf(s_valueText, sizeof(s_valueText), "%s", text);
	}
}
void RECTMENU_DrawInnerRect(RECT *rect, int inset, u32 *ot)
{
	s_promptRect = *rect;
	s_promptBoxOrder = ++s_drawOrder;
	(void)inset; (void)ot;
}
void RECTMENU_DrawOuterRect_Edge(RECT *rect, const Color *color, u32 flags, u32 *ot)
{
	(void)rect; (void)color; (void)flags; (void)ot;
}
void CTR_Box_DrawClearBox(const RECT *rect, const Color *color, s32 transparency, u32 *ot, struct PrimMem *prims)
{
	(void)rect; (void)color; (void)transparency; (void)ot; (void)prims;
}
void RECTMENU_DrawFullRect(struct RectMenu *menu, RECT *rect)
{
	s_frameOrder = ++s_drawOrder;
	s_frameDrawStyle = menu->drawStyle;
	(void)rect;
}

void CTR_Box_DrawSolidBox(RECT *rect, const Color *color, u32 *ot, struct PrimMem *prims)
{
	(void)color; (void)ot; (void)prims;
	if (s_observingRow && s_sliderBoxCount++ == 0)
	{
		s_sliderMarker = *rect;
	}
}

void RECTMENU_DrawRwdTriangle(s16 *position, char *color, u32 *ot, struct PrimMem *prims)
{
	(void)color; (void)ot; (void)prims;
	if (s_observingRow)
	{
		s_sliderDrawn = true;
		memcpy(s_sliderTriangle, position, sizeof(s_sliderTriangle));
	}
}

static void SetInput(int tapped, int held)
{
	sdata->gGamepads->gamepad[0].buttonsTapped = tapped;
	sdata->gGamepads->gamepad[0].buttonsHeldCurrFrame = held;
}

static void UpdateMenu(void)
{
	struct RectMenu *menu = sdata->ptrActiveMenu;
	menu->funcPtr(menu);
}

static int NavigationRow(const char *name)
{
	const ConfigEntry *entries[CONFIG_MAX_SECTIONS];
	int count = Config_NavigationEntries(entries);
	for (int i = 0; i < count; i++)
	{
		if (strcmp(entries[i]->key, name) == 0) { return i; }
	}
	return -1;
}

static void SelectNavigation(const char *name)
{
	int row = NavigationRow(name);
	if (row < 0) { CtrTest_Bail("navigation entry not found"); }
	sdata->ptrActiveMenu->rowSelected = (s16)row;
	SetInput(BTN_CROSS, 0);
	UpdateMenu();
}

static void TestNavigation(void)
{
	CTR_TEST_SECTION("section hierarchy");
	g_config.developerHacksEnabled = false;
	MM_ConfigMenu_Open(&s_caller, CONFIG_MENU_MAIN);
	sdata->ptrActiveMenu = sdata->ptrDesiredMenu;
	const ConfigEntry *entries[CONFIG_MAX_SECTIONS];
	const char *roots[] = {"System", "Game", "Modifiers", CONFIG_SECTION_CUSTOM_TRACKS};
	int count = Config_NavigationEntries(entries);
	CTR_TEST_CHECK(count == (int)len(roots), "the root shows four sections when developer hacks are disabled");
	for (int i = 0; i < count; i++)
	{
		CTR_TEST_CHECK(strcmp(entries[i]->key, roots[i]) == 0, "root sections follow the requested order");
	}
	CTR_TEST_CHECK(Config_EntryLabel(entries[0]) == LANG_GetString(LNG_EXPANDED_CONFIG_SYSTEM),
	               "the System navigation label resolves through the catalog");
	CTR_TEST_CHECK(Config_SectionLabel(CONFIG_SECTION_AUDIO) == LANG_GetString(LNG_EXPANDED_CONFIG_AUDIO),
	               "the Audio section title uses the same catalog entry as its navigation row");
	CTR_TEST_CHECK(strcmp(Config_SectionLabel("Unmigrated"), "Unmigrated") == 0,
	               "an unmigrated section retains its existing label");
	const char *groups[][5] = {
		{"System", "Audio", "Language", "Controls", "Graphics"},
		{"Game", "Adventure", "General", "Unlocks"},
		{"Modifiers", "Vehicle", "Weapons", NULL},
	};
	for (size_t group = 0; group < len(groups); group++)
	{
		SelectNavigation(groups[group][0]);
		count = Config_NavigationEntries(entries);
		int expected = group == 0 ? 4 : (groups[group][3] != NULL ? 3 : 2);
		CTR_TEST_CHECK(count == expected, "each group exposes its expected subsections");
		for (int i = 0; i < count; i++)
		{
			CTR_TEST_CHECK(strcmp(entries[i]->key, groups[group][i + 1]) == 0, "subsections follow the requested order");
		}
		SetInput(BTN_TRIANGLE, 0);
		UpdateMenu();
	}
	g_config.developerHacksEnabled = true;
	count = Config_NavigationEntries(entries);
	CTR_TEST_CHECK(count == 5 && strcmp(entries[4]->key, CONFIG_SECTION_DEVELOPER_HACKS) == 0,
	               "developer hacks appear at the end only when enabled");
	g_config = s_initialConfig;
}

static void TestTrackImportMenu(void)
{
	CTR_TEST_SECTION("track import entry");
	MM_ConfigMenu_Open(&s_caller, CONFIG_MENU_MAIN);
	sdata->ptrActiveMenu = sdata->ptrDesiredMenu;
	int customTracksSection = NavigationRow(CONFIG_SECTION_CUSTOM_TRACKS);
	CTR_TEST_CHECK(customTracksSection >= 0, "Custom Tracks is registered in the section table");
	sdata->ptrActiveMenu->rowSelected = (s16)customTracksSection;
	s_trackImportOpened = 0;
	SetInput(BTN_CROSS, 0);
	UpdateMenu();
	CTR_TEST_CHECK(s_customTracksPage == CUSTOM_TRACKS_ROOT && s_trackImportOpened == 0 && s_currentSection == -1,
	               "the Options entry opens the custom tracks manager");
	s_trackFixtureCount = 1;
	s_lastMenuSound = -1;
	SetInput(BTN_CROSS, 0);
	UpdateMenu();
	CTR_TEST_CHECK(s_customTracksPage == CUSTOM_TRACKS_AVAILABLE, "Available Tracks opens the installed track list");
	CTR_TEST_CHECK(s_lastMenuSound == 1, "opening the list plays the confirm sound");
	memset(s_trackDetailRows, 0, sizeof(s_trackDetailRows));
	SetInput(0, 0);
	UpdateMenu();
	CTR_TEST_CHECK(s_trackDetailRows[0] > 0 && s_trackDetailRows[1] > s_trackDetailRows[0] && s_trackDetailRows[1] < SCREEN_HEIGHT,
	               "the mode summary appears on one row below the author");
	s_trackFixture.detectionKnown = 0;
	UpdateMenu();
	CTR_TEST_CHECK(s_unknownModesY == s_trackDetailRows[1], "unrecognized track modes are shown as Unknown on the same row");
	s_trackFixture.detectionKnown = 1;
	s_trackFixture.detectedModes = NATIVE_TRACK_DETECTED_TIME_TRIAL | NATIVE_TRACK_DETECTED_VS | NATIVE_TRACK_DETECTED_ARCADE |
	                               NATIVE_TRACK_DETECTED_RELIC | NATIVE_TRACK_DETECTED_CTR | NATIVE_TRACK_DETECTED_CRYSTAL;
	s_allModesY = 0;
	UpdateMenu();
	CTR_TEST_CHECK(s_allModesY == s_trackDetailRows[1], "all detected modes fit on the same visible row");
	s_trackFixture.detectedModes = NATIVE_TRACK_DETECTED_TIME_TRIAL;
	s_trackFixture.scaPath = "fixture.sca";
	s_lastMenuSound = -1;
	SetInput(BTN_CROSS, 0);
	UpdateMenu();
	CTR_TEST_CHECK(s_customTracksPage == CUSTOM_TRACKS_DETAIL && s_trackFixtureEnabled, "confirm opens details without changing the track");
	CTR_TEST_CHECK(s_lastMenuSound == 1, "opening track details uses the shared confirmation sound");
	CTR_TEST_CHECK(s_musicPreviewRequests == 1 && s_previewRetail == -1 && s_previewTrack == &s_trackFixture,
	               "opening track details previews the selected custom music");
	s_trackFixture.scaPath = NULL;
	s_trackNumberDraws = 0;
	SetInput(BTN_CROSS, 0);
	UpdateMenu();
	CTR_TEST_CHECK(!s_trackFixtureEnabled, "confirm disables the selected track");
	CTR_TEST_CHECK(s_trackNumberDraws == 0, "installed track details omit the track counter");
	SetInput(BTN_CIRCLE, 0);
	UpdateMenu();
	CTR_TEST_CHECK(s_trackFixtureEnabled, "confirm re-enables the selected track");
	sdata->ptrActiveMenu->rowSelected = TRACK_REVIEW_TITLE;
	SetInput(BTN_CROSS, 0);
	UpdateMenu();
	CTR_TEST_CHECK(s_customTracksEditing, "confirm opens the track name editor");
	CTR_TEST_CHECK(s_editCharacterLimit == TRACK_REVIEW_TEXT_CHARACTERS, "installed track names are limited to 30 characters");
	s_editValue = "Renamed";
	s_editResult = NATIVE_TEXT_EDIT_ACCEPTED;
	SetInput(0, 0);
	int previousFrame = s_frameOrder;
	UpdateMenu();
	CTR_TEST_CHECK(!s_customTracksEditing && strcmp(s_savedTitle, "Renamed") == 0 && strcmp(s_savedAuthor, "Test Author") == 0,
	               "accepting the name saves it without changing the author");
	CTR_TEST_CHECK(s_frameOrder > previousFrame, "the details menu is drawn when editing finishes");
	sdata->ptrActiveMenu->rowSelected = TRACK_REVIEW_AUTHOR;
	SetInput(BTN_CROSS, 0);
	UpdateMenu();
	s_editValue = "New Author";
	s_editResult = NATIVE_TEXT_EDIT_ACCEPTED;
	SetInput(0, 0);
	UpdateMenu();
	CTR_TEST_CHECK(strcmp(s_savedTitle, "Test Track") == 0 && strcmp(s_savedAuthor, "New Author") == 0,
	               "accepting the author saves it without changing the name");
	sdata->ptrActiveMenu->rowSelected = TRACK_REVIEW_MUSIC_TRACK;
	SetInput(BTN_RIGHT, 0);
	previousFrame = s_frameOrder;
	UpdateMenu();
	CTR_TEST_CHECK(s_savedMusic >= 0 && s_musicPreviewRequests == 2 && s_previewRetail == s_savedMusic && s_previewTrack == NULL,
	               "changing retail music saves and previews the choice");
	CTR_TEST_CHECK(s_frameOrder > previousFrame, "the details menu is drawn when music changes");
	sdata->ptrActiveMenu->rowSelected = TRACK_REVIEW_CONFIRM;
	SetInput(BTN_CROSS, 0);
	UpdateMenu();
	CTR_TEST_CHECK(s_customTracksPage == CUSTOM_TRACKS_DELETE && s_musicPreviewRestores == 0,
	               "opening delete confirmation keeps the music preview playing");
	SetInput(BTN_TRIANGLE, 0);
	UpdateMenu();
	CTR_TEST_CHECK(s_customTracksPage == CUSTOM_TRACKS_DETAIL && s_musicPreviewRestores == 0,
	               "cancelling deletion leaves the preview playing");
	s_trackRefreshCalls = 0;
	SetInput(BTN_TRIANGLE, 0);
	UpdateMenu();
	CTR_TEST_CHECK(s_customTracksPage == CUSTOM_TRACKS_AVAILABLE, "back returns to the track list");
	CTR_TEST_CHECK(s_musicPreviewRestores == 1, "leaving track details restores menu music");
	SetInput(BTN_CROSS, 0);
	UpdateMenu();
	CTR_TEST_CHECK(s_musicPreviewRequests == 3 && s_previewRetail == s_savedMusic && s_previewTrack == NULL,
	               "reopening track details previews the saved retail music");
	sdata->ptrActiveMenu->rowSelected = TRACK_REVIEW_CONFIRM;
	SetInput(BTN_CROSS, 0);
	UpdateMenu();
	s_trackDeleteSuccess = 1;
	SetInput(BTN_CROSS, 0);
	UpdateMenu();
	CTR_TEST_CHECK(s_customTracksPage == CUSTOM_TRACKS_AVAILABLE && s_musicPreviewRestores == 2,
	               "confirming deletion restores menu music");
	s_trackDeleteSuccess = 0;
	s_lastMenuSound = -1;
	SetInput(BTN_TRIANGLE, 0);
	UpdateMenu();
	CTR_TEST_CHECK(s_customTracksPage == CUSTOM_TRACKS_ROOT, "back returns from the track list to the manager");
	CTR_TEST_CHECK(s_lastMenuSound == 2, "leaving the list plays the back sound");
	sdata->ptrActiveMenu->rowSelected = 2;
	SetInput(BTN_CROSS, 0);
	UpdateMenu();
	CTR_TEST_CHECK(s_trackRefreshCalls == 1 && s_customTracksPage == CUSTOM_TRACKS_REFRESH_RESULT && s_trackFixtureCount == 2 &&
	               strcmp(s_customTracksRefreshMessage, "TRACKS: 2  REJECTED: 1") == 0,
	               "refresh scans the track directory and displays its result");
	SetInput(BTN_CROSS, 0);
	UpdateMenu();
	CTR_TEST_CHECK(s_customTracksPage == CUSTOM_TRACKS_ROOT && sdata->ptrActiveMenu->rowSelected == 2,
	               "closing the refresh result returns to the refresh row");
	s_trackRefreshSuccess = 0;
	SetInput(BTN_CROSS, 0);
	UpdateMenu();
	CTR_TEST_CHECK(s_trackRefreshCalls == 2 && strcmp(s_customTracksRefreshMessage, "Refresh failed") == 0,
	               "a failed refresh reports the failure");
	SetInput(BTN_CROSS, 0);
	UpdateMenu();
	s_trackRefreshSuccess = 1;
	s_customTracksImportEnabled = false;
	SetInput(BTN_CROSS, 0);
	UpdateMenu();
	CTR_TEST_CHECK(s_trackRefreshCalls == 2 && s_customTracksPage == CUSTOM_TRACKS_ROOT,
	               "refresh is unavailable during a paused game");
	s_customTracksImportEnabled = true;
	sdata->ptrActiveMenu->rowSelected = 0;
	s_trackFixtureCount = 0;
	SetInput(BTN_CROSS, 0);
	UpdateMenu();
	SetInput(0, 0);
	UpdateMenu();
	CTR_TEST_CHECK(s_emptyTracksY == FullScreenMenu_RowY(0, 1), "an empty library uses a normal content row");
	SetInput(BTN_TRIANGLE, 0);
	UpdateMenu();
	sdata->ptrActiveMenu->rowSelected = 1;
	s_lastMenuSound = -1;
	SetInput(BTN_CROSS, 0);
	previousFrame = s_frameOrder;
	UpdateMenu();
	CTR_TEST_CHECK(s_customTracksPage == CUSTOM_TRACKS_IMPORT && s_trackImportOpened == 0,
	               "Import opens the local source selection");
	CTR_TEST_CHECK(s_frameOrder > previousFrame, "opening import keeps a menu frame visible");
	CTR_TEST_CHECK(s_lastMenuSound == 1, "opening import plays the confirm sound");
	SetInput(BTN_CROSS, 0);
	previousFrame = s_frameOrder;
	UpdateMenu();
	CTR_TEST_CHECK(s_trackImportOpened == 1, "Folder opens the directory importer");
	CTR_TEST_CHECK(s_frameOrder > previousFrame, "opening the folder dialog keeps a menu frame visible");
	sdata->ptrActiveMenu->rowSelected = 1;
	SetInput(BTN_CROSS, 0);
	previousFrame = s_frameOrder;
	UpdateMenu();
	CTR_TEST_CHECK(s_trackImportOpened == 11, "Xdelta patch opens the patch importer");
	CTR_TEST_CHECK(s_frameOrder > previousFrame, "opening the patch dialog keeps a menu frame visible");
	SetInput(BTN_TRIANGLE, 0);
	UpdateMenu();
	CTR_TEST_CHECK(s_customTracksPage == CUSTOM_TRACKS_ROOT && sdata->ptrActiveMenu->rowSelected == 1,
	               "back restores the manager's Import row");
	s_lastMenuSound = -1;
	SetInput(BTN_TRIANGLE, 0);
	UpdateMenu();
	CTR_TEST_CHECK(s_customTracksPage == CUSTOM_TRACKS_CLOSED && s_lastMenuSound == 2, "leaving the manager plays the back sound");
	CTR_TEST_CHECK(sdata->ptrActiveMenu->rowSelected == customTracksSection, "leaving the manager restores the Options entry");
	MM_ConfigMenu_Open(&s_caller, CONFIG_MENU_PAUSE);
	CTR_TEST_CHECK(s_customTracksPage == CUSTOM_TRACKS_CLOSED, "opening Options resets custom track navigation");
	sdata->ptrActiveMenu = sdata->ptrDesiredMenu;
	sdata->ptrActiveMenu->rowSelected = (s16)customTracksSection;
	SetInput(BTN_CROSS, 0);
	UpdateMenu();
	CTR_TEST_CHECK(s_customTracksPage == CUSTOM_TRACKS_ROOT, "custom tracks can be opened from pause options");
	sdata->ptrActiveMenu->rowSelected = 1;
	SetInput(0, 0);
	UpdateMenu();
	CTR_TEST_CHECK(s_importRowColor == GRAY, "Import remains visible and gray during a race");
	s_lastMenuSound = -1;
	SetInput(BTN_CROSS, 0);
	UpdateMenu();
	SetInput(BTN_CIRCLE, 0);
	UpdateMenu();
	CTR_TEST_CHECK(s_customTracksPage == CUSTOM_TRACKS_ROOT && s_trackImportOpened == 11 && s_lastMenuSound == -1,
	               "neither confirm button opens import or plays a confirmation sound during a race");
	sdata->ptrActiveMenu->rowSelected = 0;
	SetInput(BTN_CROSS, 0);
	UpdateMenu();
	CTR_TEST_CHECK(s_customTracksPage == CUSTOM_TRACKS_AVAILABLE, "installed tracks remain accessible from pause options");
}

static void TestSession(enum ConfigMenuContext context, int mode)
{
	struct RectMenu caller = {.rowSelected = 5};
	s_tracker.gameMode1 = mode;
	s_capturing = true;
	s_pickerOpen = true;
	SetInput(BTN_CROSS, BTN_RIGHT);
	MM_ConfigMenu_Open(&caller, context);
	sdata->ptrActiveMenu = sdata->ptrDesiredMenu;
	sdata->ptrDesiredMenu = NULL;
	CTR_TEST_CHECK(!s_capturing && !s_pickerOpen, "opening clears stale capture and picker state");
	CTR_TEST_CHECK(sdata->ptrActiveMenu->rowSelected == 0, "opening selects the first section");
	UpdateMenu();
	CTR_TEST_CHECK(s_currentSection == -1, "opening confirm does not also enter a section");
	CTR_TEST_CHECK(s_sectionFont == FONT_BIG, "section entries use the large font");
	CTR_TEST_CHECK(s_tracker.gameMode1 == mode, "opening preserves the caller's game and pause flags");
	CTR_TEST_CHECK(s_centerX == (context == CONFIG_MENU_MAIN ? FULLSCREEN_MENU_MAIN_X : FULLSCREEN_MENU_TITLE_X),
	               "the section list follows the caller's menu position");
	const int expectedStyle = (context == CONFIG_MENU_MAIN) ? FULLSCREEN_MENU_DRAW_STYLE_SHADED
	                                                       : FULLSCREEN_MENU_DRAW_STYLE_FLAT;
	CTR_TEST_CHECK(s_frameDrawStyle == expectedStyle, "the box matches the calling menu's shading");

	SelectNavigation("Game");
	CTR_TEST_CHECK(s_configGroup != NULL && strcmp(s_configGroup->key, "Game") == 0, "Game opens its subsection list");
	SetInput(0, 0);
	UpdateMenu();
	CTR_TEST_CHECK(s_centerX == (context == CONFIG_MENU_MAIN ? FULLSCREEN_MENU_MAIN_X : FULLSCREEN_MENU_TITLE_X),
	               "the subsection list follows the caller's menu position");
	int generalSection = -1;
	for (int section = 0; section < s_numSections; section++)
	{
		if (strcmp(g_configEntries[s_sectionToEntry[section]].section, "General") == 0)
		{
			generalSection = section;
		}
	}
	CTR_TEST_CHECK(generalSection >= 0, "the General section is present");
	SelectNavigation("General");
	CTR_TEST_CHECK(s_currentSection == generalSection, "confirm enters the selected section");
	// Back and confirm together must not reopen the section or edit an option.
	SetInput(BTN_TRIANGLE | BTN_CROSS, BTN_RIGHT);
	UpdateMenu();
	CTR_TEST_CHECK(s_currentSection == -1, "back consumes simultaneous confirm when leaving a section");
	CTR_TEST_CHECK(!g_config.skipIntro, "leaving a section does not edit its option");
	CTR_TEST_CHECK(s_configSaves == 0 && s_controlsSaves == 0, "leaving a section does not close the session");

	SetInput(BTN_TRIANGLE, 0);
	UpdateMenu();
	CTR_TEST_CHECK(s_configGroup == NULL && sdata->ptrActiveMenu->rowSelected == 1, "back restores the parent section row");
	s_capturing = true;
	SetInput(BTN_START | BTN_CROSS, BTN_DOWN);
	UpdateMenu();
	CTR_TEST_CHECK(sdata->ptrDesiredMenu == &caller, "closing returns to the actual caller");
	CTR_TEST_CHECK(caller.rowSelected == 5, "closing preserves the caller's selected row");
	CTR_TEST_CHECK(s_tracker.gameMode1 == mode, "closing preserves pause until the caller resumes");
	CTR_TEST_CHECK(s_configSaves == 1 && s_controlsSaves == 1, "closing saves config and controls once");
	CTR_TEST_CHECK(!s_capturing, "closing cancels capture");
	CTR_TEST_CHECK(sdata->gGamepads->gamepad[0].buttonsTapped == 0 &&
	               sdata->AnyPlayerTap == 0 && sdata->AnyPlayerHold == 0,
	               "closing consumes menu input before returning");
	CTR_TEST_CHECK(sdata->gGamepads->gamepad[0].buttonsHeldCurrFrame == BTN_DOWN,
	               "closing preserves physical held state to avoid a synthetic tap next frame");
	s_configSaves = 0;
	s_controlsSaves = 0;
}

static int EntryValue(const ConfigEntry *entry)
{
	return entry->type == CFG_BOOL ? *(bool *)entry->valuePtr : *(int *)entry->valuePtr;
}

static void OpenEntry(int index, enum ConfigMenuContext context)
{
	MM_ConfigMenu_Open(&s_caller, context);
	sdata->ptrActiveMenu = sdata->ptrDesiredMenu;
	sdata->ptrDesiredMenu = NULL;
	const char *target = g_configEntries[index].section;
	for (size_t i = 0; i < len(s_configNavigation); i++)
	{
		if (strcmp(s_configNavigation[i].key, target) == 0 && s_configNavigation[i].section[0] != '\0')
		{
			SelectNavigation(s_configNavigation[i].section);
			break;
		}
	}
	for (int section = 0; section < s_numSections; section++)
	{
		const int first = s_sectionToEntry[section];
		if (index >= first && index < first + s_sectionCount[section])
		{
			SelectNavigation(target);
			sdata->ptrActiveMenu->rowSelected = (s16)(index - first);
			return;
		}
	}
	CtrTest_Bail("every configured option must remain reachable");
}

static void TestAvailability(void)
{
	const char *const allowedKeys[] = {
		"audio", "text", "retail_priority", "export_active_language", "controls", "custom_tracks", "skip_intro", "skip_podium_ceremonies", "show_reserves_meter",
		"skip_hints", "boss_cutscenes", "increase_draw_distance", "dithering",
		"fullscreen", "vsync", "aspect_ratio", "render_scale", "smooth_scaling",
		"fxaa", "texture_filtering", "pgxp",
		"allow_oxide_station_multiplayer", "allow_race_tracks_in_battle",
		"extended_multiplayer", "bot_selection_mode", "mask_mode",
		"expanded_options", "save_anywhere", "unlock_all_characters", "unlock_nitros_oxide",
		"disable_split_screen_lod",
	};
	for (int i = 0; i < g_numConfigEntries; i++)
	{
		const ConfigEntry *entry = &g_configEntries[i];
		CTR_TEST_CHECK(entry->lngKey >= LNG_EXPANDED_STRING_BASE && entry->lngKey < LNG_EXPANDED_STRING_END,
		               "every configuration entry has a catalog label");
		bool allowed = strcmp(entry->section, CONFIG_SECTION_DEVELOPER_HACKS) == 0;
		for (int k = 0; k < (int)len(allowedKeys); k++)
		{
			allowed |= strcmp(entry->key, allowedKeys[k]) == 0;
		}
		CTR_TEST_CHECK(entry->availableDuringPause == allowed, entry->key);
	}
}

static void TestEditing(enum ConfigMenuContext context)
{
	for (int i = 0; i < g_numConfigEntries; i++)
	{
		const ConfigEntry *entry = &g_configEntries[i];
		if (entry->type == CFG_ACTION || entry->valuePtr == &g_config.developerHacksEnabled || strcmp(entry->section, CONFIG_SECTION_LANGUAGE) == 0)
		{
			continue;
		}
		g_config = s_initialConfig;
		g_config.developerHacksEnabled = true;
		// Non-default values prove disabled rows display the current value, not a placeholder.
		char expectedBuffer[64];
		const char *expectedText;
		if (entry->type == CFG_BOOL)
		{
			*(bool *)entry->valuePtr = true;
			expectedText = LANG_GetString(LNG_EXPANDED_ON);
		}
		else if (entry->type == CFG_ENUM)
		{
			*(int *)entry->valuePtr = entry->enumValues[1].value;
			const ConfigEnumValue *choice = &entry->enumValues[1];
			expectedText = choice->lngKey >= LNG_EXPANDED_STRING_BASE ? LANG_GetString(choice->lngKey) : choice->name;
		}
		else
		{
			*(int *)entry->valuePtr = entry->min + entry->step;
			snprintf(expectedBuffer, sizeof(expectedBuffer), "%d%%", EntryValue(entry));
			expectedText = expectedBuffer;
		}
		const int before = EntryValue(entry);
		const bool editable = context == CONFIG_MENU_MAIN || entry->availableDuringPause;
		OpenEntry(i, context);
		s_observedLabel = entry->lngKey >= LNG_EXPANDED_STRING_BASE ? LANG_GetString(entry->lngKey) : entry->label;
		s_valueDrawn = false;
		SetInput(0, 0);
		UpdateMenu();
		CTR_TEST_CHECK(s_valueDrawn && strcmp(s_valueText, expectedText) == 0, entry->key);
		CTR_TEST_CHECK(s_valueFlags == (JUSTIFY_RIGHT | (editable ? WHITE : GRAY)),
		               "the value's color matches its editability");
		const int buttons[] = {BTN_CROSS, BTN_CIRCLE};
		for (int b = 0; b < (int)len(buttons); b++)
		{
			const int previous = EntryValue(entry);
			SetInput(buttons[b], 0);
			UpdateMenu();
			const bool shouldChange = editable && entry->type != CFG_INT;
			CTR_TEST_CHECK((EntryValue(entry) != previous) == shouldChange,
			               "confirm edits only enabled bool and enum values");
		}
		// Frame zero makes held-arrow repetition due for every numeric row.
		sdata->frameCounter = 0;
		SetInput(0, BTN_RIGHT);
		UpdateMenu();
		if (entry->type == CFG_INT)
		{
			CTR_TEST_CHECK(EntryValue(entry) == before + (editable ? entry->step : 0),
			               "right edits only enabled sliders");
			SetInput(0, BTN_LEFT);
			UpdateMenu();
			CTR_TEST_CHECK(EntryValue(entry) == before, "left edits only enabled sliders");
		}
		else if (!editable)
		{
			CTR_TEST_CHECK(EntryValue(entry) == before, "disabled values ignore all editing input");
		}
	}
	s_observedLabel = NULL;
}

static void TestLanguagePicker(void)
{
	CTR_TEST_SECTION("independent language pickers");
	CTR_TEST_CHECK(strcmp(Config_LanguageLabel(NATIVE_LANGUAGE_FRENCH, "fallback"), "fallback") == 0,
	               "an unavailable disc name falls back to the configured label");
	for (int row = 0; row < 2; row++)
	{
		int index = -1;
		for (int i = 0; i < g_numConfigEntries; i++)
		{
			if (g_configEntries[i].valuePtr == (row == 0 ? (void *)&g_config.voiceLanguage : (void *)&g_config.textLanguage))
			{
				index = i;
				break;
			}
		}
		if (index < 0) { CtrTest_Bail("language row missing"); }
		g_config = s_initialConfig;
		s_languageApplied = 0;
		s_textCatalogReloaded = 0;
		s_voiceRefreshed = 0;
		OpenEntry(index, CONFIG_MENU_MAIN);
		SetInput(BTN_CROSS, 0);
		UpdateMenu();
		CTR_TEST_CHECK(s_languagePickerOpen && s_languageChoiceCount == 2, "picker includes only English and available resources");
		CTR_TEST_CHECK(strcmp(Config_LanguageChoiceLabel(&g_configEntries[index], 1), row == 0 ? "DISC GERMAN" : "DISC ITALIAN") == 0,
		               "picker displays the name supplied by the language resource");
		s_recordLanguageDraw = true;
		s_languageDrawSequence = s_languagePopupDraw = s_languageUnderlyingDraw = 0;
		SetInput(0, 0);
		UpdateMenu();
		s_recordLanguageDraw = false;
		CTR_TEST_CHECK(s_languagePopupDraw != 0 && s_languageUnderlyingDraw > s_languagePopupDraw,
		               "popup primitives are submitted before the underlying menu so they appear above it");
		SetInput(BTN_DOWN, 0);
		UpdateMenu();
		SetInput(BTN_CROSS, 0);
		s_lastMenuSound = -1;
		UpdateMenu();
		CTR_TEST_CHECK(!s_languagePickerOpen, "confirm closes the picker");
		CTR_TEST_CHECK(s_lastMenuSound == 1, "confirming a language plays the confirmation sound");
		if (row == 0)
		{
			CTR_TEST_CHECK(g_config.voiceLanguage == 4 && g_config.textLanguage == 1 && s_voiceRefreshed == 1,
			               "voice selection applies independently of text");
			CTR_TEST_CHECK(s_textCatalogReloaded == 0, "voice changes do not reload text catalogs");
		}
		else
		{
			CTR_TEST_CHECK(g_config.textLanguage == 5 && g_config.voiceLanguage == 1 && s_languageApplied == 5,
			               "text selection applies independently of voices");
			CTR_TEST_CHECK(s_textCatalogReloaded == 5, "text confirmation reloads the selected catalog");
		}
	}
	g_config = s_initialConfig;
}

static void TestRetailDiscPreference(void)
{
	CTR_TEST_SECTION("retail disc preference availability");
	int index = -1;
	for (int i = 0; i < g_numConfigEntries; i++)
	{
		if (g_configEntries[i].valuePtr == &g_config.retailTextPriority) { index = i; break; }
	}
	if (index < 0) { CtrTest_Bail("retail disc preference must exist"); }
	g_config = s_initialConfig;
	s_europeTextAvailable = 0;
	OpenEntry(index, CONFIG_MENU_MAIN);
	g_config.retailTextPriority = RETAIL_TEXT_PRIORITY_ISO_EU;
	s_textCatalogReloaded = 0;
	SetInput(BTN_CROSS, 0);
	UpdateMenu();
	CTR_TEST_CHECK(g_config.retailTextPriority == RETAIL_TEXT_PRIORITY_EXPANDED && !Config_CanEdit(&g_configEntries[index]),
	               "without European text the preference is forced off and cannot be edited");
	CTR_TEST_CHECK(s_textCatalogReloaded == g_config.textLanguage, "forcing off refreshes the active text");
	s_europeTextAvailable = 1;
	SetInput(BTN_CROSS, 0);
	UpdateMenu();
	CTR_TEST_CHECK(g_config.retailTextPriority == RETAIL_TEXT_PRIORITY_ISO_EU && Config_CanEdit(&g_configEntries[index]),
	               "the preference becomes editable when European text is available");
	g_config = s_initialConfig;
}

static void TestExportActiveLanguage(void)
{
	CTR_TEST_SECTION("export active language action");
	for (int i = 0; i < g_numConfigEntries; i++)
	{
		if (strcmp(g_configEntries[i].key, "export_active_language") != 0) { continue; }
		OpenEntry(i, CONFIG_MENU_MAIN);
		s_textExportCalls = 0;
		SetInput(BTN_CROSS, 0);
		UpdateMenu();
		CTR_TEST_CHECK(s_textExportCalls == 1 && s_textExportStatus == LNG_EXPANDED_EXPORT_DONE,
		               "confirm exports the active language and displays the result");
		return;
	}
	CtrTest_Bail("export action must be present");
}

static void TestDeveloperGate(void)
{
	g_config = s_initialConfig;
	g_config.developerHacksEnabled = true;
	g_config.oneLapRaces = true;
	for (int i = 0; i < g_numConfigEntries; i++)
	{
		if (g_configEntries[i].valuePtr == &g_config.developerHacksEnabled)
		{
			OpenEntry(i, CONFIG_MENU_PAUSE);
			SetInput(BTN_CROSS, 0);
			UpdateMenu();
			CTR_TEST_CHECK(!g_config.developerHacksEnabled && !g_config.oneLapRaces,
			               "the developer gate can reset its options while paused");
			CTR_TEST_CHECK(s_currentSection == -1 && g_configMenu.rowSelected < s_numSections,
			               "hiding Developer Hacks returns to a valid section selection");
			SetInput(0, 0);
			UpdateMenu();
			for (int section = 0; section < s_numSections; section++)
			{
				CTR_TEST_CHECK(strcmp(g_configEntries[s_sectionToEntry[section]].section,
				                      CONFIG_SECTION_DEVELOPER_HACKS) != 0,
				               "the disabled developer section is hidden");
			}
			return;
		}
	}
	CtrTest_Bail("the developer gate must exist");
}

static void TestAudio(enum ConfigMenuContext context, int gameMode)
{
	s_tracker.gameMode1 = gameMode;
	s_volumes[HOWL_VOLUME_TYPE_FX] = 128;
	s_volumes[HOWL_VOLUME_TYPE_MUSIC] = 64;
	s_volumes[HOWL_VOLUME_TYPE_VOICE] = 192;
	s_stereo = true;
	int audioEntry = -1;
	for (int i = 0; i < g_numConfigEntries; i++)
	{
		if (strcmp(g_configEntries[i].section, CONFIG_SECTION_AUDIO) == 0)
		{
			audioEntry = i;
		}
	}
	CTR_TEST_CHECK(audioEntry >= 0, "Audio is available in Options");
	OpenEntry(audioEntry, context);
	const char *const labels[] = {"FX", "MUSIC", "VOICE"};
	for (int row = 0; row < GAME_OPTIONS_VOLUME_COUNT; row++)
	{
		s_observedLabel = labels[row];
		s_sliderDrawn = false;
		SetInput(row == 0 ? 0 : BTN_DOWN, 0);
		UpdateMenu();
		CTR_TEST_CHECK(s_sliderDrawn, "each colon-free audio label is paired with an original-style slider");
		CTR_TEST_CHECK(s_sliderTriangle[0] == AUDIO_CONFIG_SLIDER_X &&
		               s_sliderTriangle[2] == FULLSCREEN_MENU_VALUE_X,
		               "the original slider shape fits inside Audio's value column");
		const int markerTravel = FULLSCREEN_MENU_VALUE_X - AUDIO_CONFIG_SLIDER_X - 5;
		CTR_TEST_CHECK(s_sliderMarker.x == AUDIO_CONFIG_SLIDER_X + s_volumes[row] * markerTravel / 256 + 1,
		               "the original slider marker displays the live channel volume");
		if (row == HOWL_VOLUME_TYPE_MUSIC && context == CONFIG_MENU_MAIN)
		{
			CTR_TEST_CHECK(!s_previewPlaying, "the main menu's own music supplies feedback without an extra preview");
		}
		else
		{
			CTR_TEST_CHECK(s_previewPlaying && s_previewRow == row, "other volume rows select their original preview");
		}
		int before[GAME_OPTIONS_VOLUME_COUNT];
		memcpy(before, s_volumes, sizeof(before));
		SetInput(0, BTN_RIGHT);
		UpdateMenu();
		for (int channel = 0; channel < GAME_OPTIONS_VOLUME_COUNT; channel++)
		{
			CTR_TEST_CHECK(s_volumes[channel] == before[channel] + (channel == row ? 4 : 0),
			               "a held direction edits only the selected audio channel");
		}
		SetInput(BTN_CROSS | BTN_CIRCLE, 0);
		UpdateMenu();
		CTR_TEST_CHECK(s_volumes[row] == before[row] + 4, "confirm does not edit volume sliders");
	}
	s_volumes[HOWL_VOLUME_TYPE_VOICE] = 254;
	SetInput(0, BTN_RIGHT);
	UpdateMenu();
	CTR_TEST_CHECK(s_volumes[HOWL_VOLUME_TYPE_VOICE] == 255, "volume clamps at full scale");
	s_volumes[HOWL_VOLUME_TYPE_VOICE] = 1;
	SetInput(0, BTN_LEFT | BTN_RIGHT);
	UpdateMenu();
	CTR_TEST_CHECK(s_volumes[HOWL_VOLUME_TYPE_VOICE] == 0, "Left takes priority and volume clamps at silence");

	s_observedLabel = "MODE";
	s_valueDrawn = false;
	SetInput(BTN_DOWN, 0);
	UpdateMenu();
	CTR_TEST_CHECK(!s_previewPlaying, "the mode row stops volume preview");
	CTR_TEST_CHECK(s_valueDrawn && strcmp(s_valueText, s_strings[LNG_STEREO]) == 0,
	               "the colon-free mode row displays the live stereo mode");
	SetInput(BTN_CROSS, 0);
	UpdateMenu();
	CTR_TEST_CHECK(!s_stereo && strcmp(s_valueText, s_strings[LNG_MONO]) == 0, "Cross selects and displays mono");
	SetInput(BTN_CIRCLE, 0);
	UpdateMenu();
	CTR_TEST_CHECK(s_stereo, "Circle selects stereo again");

	SetInput(BTN_UP, 0);
	UpdateMenu();
	CTR_TEST_CHECK(s_previewPlaying, "returning to a volume row restarts its preview");
	SetInput(BTN_TRIANGLE | BTN_CROSS, BTN_RIGHT);
	UpdateMenu();
	CTR_TEST_CHECK(!s_previewPlaying && s_currentSection == -1, "leaving Audio stops preview and returns to sections");
	CTR_TEST_CHECK(s_tracker.gameMode1 == gameMode, "Audio preserves the session's game and pause flags");
	for (int channel = 0; channel < GAME_OPTIONS_VOLUME_COUNT; channel++)
	{
		CTR_TEST_CHECK(GAME_SAVE.options.volumes[channel] == s_volumes[channel],
		               "leaving Audio captures volume in the existing game options");
	}
	CTR_TEST_CHECK(GAME_SAVE.options.audioMode == s_stereo, "leaving Audio captures stereo in the existing game options");
	memset(s_volumes, 0, sizeof(s_volumes));
	s_stereo = false;
	sdata->boolHasLoadedOptions = false;
	RaceConfig_LoadGameOptions();
	CTR_TEST_CHECK(s_volumes[HOWL_VOLUME_TYPE_FX] == 132 && s_volumes[HOWL_VOLUME_TYPE_MUSIC] == 68 &&
	               s_volumes[HOWL_VOLUME_TYPE_VOICE] == 0 && s_stereo,
	               "the existing game-options loader restores Audio's saved values");

	OpenEntry(audioEntry, context);
	SetInput(0, 0);
	UpdateMenu();
	MM_ConfigMenu_Open(&s_caller, context);
	CTR_TEST_CHECK(!s_previewPlaying, "reopening Options from Audio also stops its preview");
	CTR_TEST_CHECK(strcmp(s_strings[LNG_FX], "FX:") == 0 && strcmp(s_strings[LNG_MODE], "MODE:") == 0,
	               "Audio leaves the shared localized strings intact");
	s_observedLabel = NULL;
}

static void SetupPauseRows(struct RectMenu *menu, struct MenuRow *rows, int count)
{
	for (int row = 0; row < count - 1; row++)
	{
		rows[row] = (struct MenuRow){LNG_RESUME, 0, 0, 0, 0};
	}
	rows[2].stringIndex = LNG_OPTIONS;
	rows[count - 1].stringIndex = RECTMENU_STRING_NONE;
	menu->rows = rows;
}

static void TestPauseEntryPoints(void)
{
	SetupPauseRows(&data.menuAdvHub, data.rowsAdvHub, len(data.rowsAdvHub));
	SetupPauseRows(&data.menuAdvRace, data.rowsAdvRace, len(data.rowsAdvRace));
	SetupPauseRows(&data.menuAdvCup, data.rowsAdvCup, len(data.rowsAdvCup));
	SetupPauseRows(&data.menuBattle, data.rowsBattle, len(data.rowsBattle));
	SetupPauseRows(&data.menuArcadeCup, data.rowsArcadeCup, len(data.rowsArcadeCup));
	SetupPauseRows(&data.menuArcadeRace, data.rowsArcadeRace, len(data.rowsArcadeRace));
	const int modes[] = {ARCADE_MODE, TIME_TRIAL, BATTLE_MODE, ADVENTURE_MODE,
	                     ADVENTURE_MODE | ADVENTURE_ARENA, ADVENTURE_MODE | ADVENTURE_CUP,
	                     ARCADE_MODE};
	for (int index = 0; index < (int)len(modes); index++)
	{
		for (int players = 1; players <= PLATFORM_INPUT_PAD_COUNT; players++)
		{
			for (int chaos = 0; chaos <= 1; chaos++)
			{
				s_tracker.gameMode1 = modes[index] | PAUSE_1;
				s_tracker.gameMode2 = index == (int)len(modes) - 1 ? CUP_ANY_KIND : 0;
				s_tracker.numPlyrCurrGame = players;
				s_tracker.cooldownfromPauseUntilUnpause = 0;
				sdata->chaosSettings.armed = chaos;
				s_hubLoaded = true;
				s_hubDestroys = 0;
				s_hubUpdates = 0;
				s_unpauses = 0;
				s_backgroundDeactivations = 0;
				struct RectMenu *caller = MainFreeze_GetMenuPtr();
				caller->rowSelected = 2;
				caller->funcState = RECTMENU_FUNC_STATE_INPUT;
				sdata->ptrActiveMenu = caller;
				const int mode = s_tracker.gameMode1;
				MainFreeze_MenuPtrDefault(caller);
				CTR_TEST_CHECK(sdata->ptrDesiredMenu == &g_configMenu && s_configSession.context == CONFIG_MENU_PAUSE,
				               "every pause menu opens shared Options in pause context");
				CTR_TEST_CHECK(s_hubDestroys == ((mode & ADVENTURE_ARENA) != 0),
				               "opening Options cleans up Adventure hub presentation only in the hub");
				sdata->ptrActiveMenu = sdata->ptrDesiredMenu;
				CTR_TEST_CHECK(MM_ConfigMenu_IsActive(), "the global Start handler can recognize shared Options");
				sdata->AnyPlayerTap = BTN_START;
				SetInput(BTN_START, BTN_START);
				UpdateMenu();
				CTR_TEST_CHECK(sdata->ptrDesiredMenu == caller && caller->rowSelected == 2,
				               "Start returns to the exact pause menu and Options row");
				CTR_TEST_CHECK(s_tracker.gameMode1 == mode && s_unpauses == 0 && s_backgroundDeactivations == 0,
				               "Options exit keeps the game, audio and captured background paused");
				CTR_TEST_CHECK(sdata->AnyPlayerTap == 0, "Start is consumed before returning to pause");
				sdata->ptrActiveMenu = caller;
				CTR_TEST_CHECK(!MM_ConfigMenu_IsActive(), "Start can resume again after returning to the pause menu");
				caller->funcState = RECTMENU_FUNC_STATE_UPDATE;
				MainFreeze_MenuPtrDefault(caller);
				CTR_TEST_CHECK(s_hubUpdates == ((mode & ADVENTURE_ARENA) != 0),
				               "returning to Adventure hub pause restores its presentation update");
				caller->funcState = RECTMENU_FUNC_STATE_INPUT;
				caller->rowSelected = 0;
				MainFreeze_MenuPtrDefault(caller);
				CTR_TEST_CHECK((s_tracker.gameMode1 & PAUSE_ALL) == 0 && s_unpauses == 1 && s_backgroundDeactivations == 1,
				               "Resume still unpauses the game and audio and releases the captured background");
			}
		}
	}

	s_tracker.gameMode1 = ADVENTURE_MODE | ADVENTURE_ARENA | PAUSE_1;
	s_hubLoaded = false;
	s_hubDestroys = 0;
	data.menuAdvHub.rowSelected = 2;
	MainFreeze_MenuPtrDefault(&data.menuAdvHub);
	CTR_TEST_CHECK(s_hubDestroys == 0, "Options does not call unloaded Adventure hub code");
	s_hubLoaded = true;
	sdata->chaosSettings.armed = true;
	struct RectMenu *caller = MainFreeze_GetMenuPtr();
	caller->rowSelected = len(data.rowsAdvHub) - 1;
	MainFreeze_MenuPtrDefault(caller);
	CTR_TEST_CHECK(s_chaosCaller == caller, "Chaos Rules keeps its independent pause-menu entry point");
}

int main(void)
{
	// Other unity-build menus use these shared helpers; this suite tests Config only.
	(void)FullScreenMenu_RowActivate;
	(void)FullScreenMenu_RowStep;
	(void)FullScreenMenu_DrawRowValue;
	sdata->gGT = &s_tracker;
	s_initialConfig = g_config;
	sdata->gGamepads = &sdata->gamepadSystem;
	sdata->lngStrings = s_strings;
	s_strings[LNG_OPTIONS] = "OPTIONS";
	s_strings[LNG_FX] = "FX:";
	s_strings[LNG_MUSIC] = "MUSIC:";
	s_strings[LNG_VOICE] = "VOICE:";
	s_strings[LNG_MODE] = "MODE:";
	s_strings[LNG_MONO] = "MONO";
	s_strings[LNG_STEREO] = "STEREO";
	s_tracker.backBuffer = &s_buffer;
	CTR_TEST_SECTION("import text editing");
	struct TrackImportReviewView review = {.title = "Circuit", .author = "Author", .row = TRACK_REVIEW_TITLE};
	s_promptDraws = 0;
	CustomTracks_DrawImportReview(&review, &s_tracker, s_buffer.otMem.uiOT);
	CTR_TEST_CHECK(s_promptDraws == 0, "the editing hint is hidden while not typing");
	review.editing = 1;
	review.editingValue = "Circuit";
	data.font_charPixHeight[FONT_SMALL] = 8;
	sdata->frameCounter = 0;
	s_cursorDraws = 0;
	CustomTracks_DrawImportReview(&review, &s_tracker, s_buffer.otMem.uiOT);
	CTR_TEST_CHECK(s_promptDraws == 1 && s_cursorDraws == 1, "editing shows the hint and cursor");
	CTR_TEST_CHECK(s_promptY == SCREEN_HEIGHT - 12 && s_promptRect.y + s_promptRect.h == SCREEN_HEIGHT,
	               "the hint box is positioned at the bottom of the screen");
	CTR_TEST_CHECK(s_promptOrder < s_promptBoxOrder && s_promptBoxOrder < s_frameOrder,
	               "the reversed ordering table draws the hint text and box above the menu");
	sdata->frameCounter = 15;
	s_cursorDraws = 0;
	CustomTracks_DrawImportReview(&review, &s_tracker, s_buffer.otMem.uiOT);
	CTR_TEST_CHECK(s_cursorDraws == 0, "the editing cursor blinks off");
	data.font_charPixHeight[FONT_SMALL] = 0;
	TestNavigation();
	TestTrackImportMenu();
	CTR_TEST_SECTION("Main-menu session");
	TestSession(CONFIG_MENU_MAIN, MAIN_MENU);
	CTR_TEST_SECTION("Paused race session");
	TestSession(CONFIG_MENU_PAUSE, ARCADE_MODE | PAUSE_1);
	CTR_TEST_SECTION("Reopening resets section navigation");
	struct RectMenu caller = {0};
	MM_ConfigMenu_Open(&caller, CONFIG_MENU_MAIN);
	sdata->ptrActiveMenu = sdata->ptrDesiredMenu;
	SetInput(BTN_CROSS, 0);
	UpdateMenu();
	MM_ConfigMenu_Open(&caller, CONFIG_MENU_PAUSE);
	CTR_TEST_CHECK(s_currentSection == -1 && s_configScroll.offset == 0,
	               "a new session starts at the section list with reset scrolling");
	CTR_TEST_SECTION("Pause availability policy");
	TestAvailability();
	TestLanguagePicker();
	TestRetailDiscPreference();
	TestExportActiveLanguage();
	CTR_TEST_SECTION("Main-menu editing and values");
	TestEditing(CONFIG_MENU_MAIN);
	CTR_TEST_SECTION("Paused editing, disabled values and colors");
	TestEditing(CONFIG_MENU_PAUSE);
	CTR_TEST_SECTION("Disabling Developer Hacks while paused");
	TestDeveloperGate();
	CTR_TEST_SECTION("Audio from the main menu");
	TestAudio(CONFIG_MENU_MAIN, MAIN_MENU);
	CTR_TEST_SECTION("Audio while paused");
	TestAudio(CONFIG_MENU_PAUSE, ARCADE_MODE | PAUSE_1);
	CTR_TEST_SECTION("Pause entry points, hub cleanup, return and Resume");
	TestPauseEntryPoints();
	return CtrTest_Summary();
}
