#include <shared/ctr_test.h>
#include <SDL3/SDL.h>
#include <common.h>
#include <platform/native_folder_dialog.h>
#include <platform/native_track_library.h>
#include <platform/native_text_edit.h>
#include <track_import_view.h>
#include <platform/native_track_registry.c>

struct sData sdata_static;
struct Data data;
void RECTMENU_DrawFullRect(struct RectMenu *menu, RECT *rect) { (void)menu; (void)rect; }
void CTR_Box_DrawClearBox(const RECT *rect, const Color *color, s32 transparency, u32 *ot, struct PrimMem *prims)
{
	(void)rect; (void)color; (void)transparency; (void)ot; (void)prims;
}
void DecalFont_DrawLineOT(char *text, s16 x, s16 y, s16 font, s16 flags, u32 *ot)
{
	(void)text; (void)x; (void)y; (void)font; (void)flags; (void)ot;
}
static enum NativeFolderDialogResult s_dialogResult;
static int s_prepared, s_commits, s_discards, s_refresh, s_patchResult, s_cancel;
static int s_sound = -1;
static const char *s_prepareError;
static char s_lastTitle[256], s_lastAuthor[256];
static enum NativeTextEditResult s_editResult;
static size_t s_characterLimit;
static const char *s_editValue;
static int s_previewRetail, s_previewRestores;
static char s_previewIdentity[256];
void MusicPreview_Request(int retail, const struct NativeTrackMetadata *track)
{
	s_previewRetail = retail;
	snprintf(s_previewIdentity, sizeof(s_previewIdentity), "%s", track != NULL ? track->identity : "");
}
void MusicPreview_Restore(void) { s_previewRestores++; }
int MusicPreview_Busy(void) { return 0; }
static int s_musicRetail = -1;
static char s_musicIdentity[256];
static int s_musicAvailable;
static unsigned int s_excluded;
int NativeTrackPatch_ReviewIncluded(const char *identity)
{
	unsigned int bit = strcmp(identity, "local:other") == 0 ? 2u : 1u;
	return (s_excluded & bit) == 0;
}
int NativeTrackPatch_ReviewSetIncluded(const char *identity, int included)
{
	unsigned int bit = strcmp(identity, "local:other") == 0 ? 2u : 1u;
	if (included) { s_excluded &= ~bit; } else { s_excluded |= bit; }
	return 1;
}
static struct NativeTrackRegistry *s_installedRegistry;
const char *NativeTrackPatch_ReviewMusic(const char *identity) { (void)identity; return s_musicIdentity[0] ? s_musicIdentity : NULL; }
int NativeTrackPatch_ReviewSetMusic(const char *identity, const char *source)
{
	(void)identity;
	snprintf(s_musicIdentity, sizeof(s_musicIdentity), "%s", source != NULL ? source : "");
	return 1;
}
const struct NativeTrackRegistry *NativeTrackLibrary_Current(void) { return s_installedRegistry; }
int NativeTrackLibrary_Music(const struct NativeTrackMetadata *track)
{
	(void)track;
	return s_musicRetail;
}
int NativeTrackLibrary_SetMusic(const struct NativeTrackMetadata *track, int retail, int custom)
{
	(void)track; (void)custom; s_musicRetail = retail;
	return 1;
}

int NativeFolderDialog_Open(void) { s_dialogResult = NATIVE_FOLDER_DIALOG_PENDING; return 1; }
int NativeFileDialog_OpenPatch(void) { return NativeFolderDialog_Open(); }
enum NativeFolderDialogResult NativeFolderDialog_Result(void) { return s_dialogResult; }
const char *NativeFolderDialog_Path(void) { return "C:/source"; }
const char *NativeAssets_GetAssetDir(void) { return "C:/assets"; }
const char *NativeTrackPatch_PrepareFolder(const char *assets, const char *source) { (void)assets; (void)source; s_prepared++; return s_prepareError; }
int NativeTrackPatch_StartReview(const char *assets, const char *patch) { (void)assets; (void)patch; s_patchResult = 0; return 1; }
int NativeTrackPatch_Poll(void) { return s_patchResult; }
const char *NativeTrackPatch_Message(void) { return "Patch result"; }
void NativeTrackPatch_Cancel(void) { s_cancel++; }
void NativeTrackPatch_Shutdown(void) { s_cancel++; }
const char *NativeTrackPatch_ReviewDirectory(void) { return "C:/staged"; }
const char *NativeTrackPatch_ReviewCommit(void) { s_commits++; return NULL; }
int NativeTrackPatch_ReviewCommitStart(void) { s_commits++; s_patchResult = 0; return 1; }
struct NativeTrackRegistry *NativeTrackPatch_TakeRegistry(void) { return NativeTrackRegistry_Create(); }
void NativeTrackLibrary_Replace(struct NativeTrackRegistry *registry) { s_refresh++; NativeTrackRegistry_Destroy(registry); }
int NativeTrackPatch_ReviewDiscard(void) { s_discards++; return 1; }
int NativeTrackPatch_ReviewDiscardStart(void) { s_discards++; s_patchResult = 0; return 1; }
const char *NativeTrackPatch_ReviewUpdate(const char *identity, const char *title, const char *author)
{
	(void)identity;
	snprintf(s_lastTitle, sizeof(s_lastTitle), "%s", title);
	snprintf(s_lastAuthor, sizeof(s_lastAuthor), "%s", author);
	return NULL;
}
struct NativeTrackRegistry *NativeTrackLibrary_Scan(const char *directory, struct NativeTrackScanResult *result)
{
	(void)directory; (void)result;
	struct NativeTrackRegistry *registry = NativeTrackRegistry_Create();
	struct NativeTrackMetadata track = {"local:track", "Track", "track.lev", "track.vrm", s_musicAvailable == 1 ? "track.sca" : NULL, "Author", 0, 0, NULL};
	NativeTrackRegistry_Add(registry, &track, NULL);
	if (s_musicAvailable)
	{
		struct NativeTrackMetadata other = {"local:other", "Other", "other.lev", "other.vrm", "other.sca", NULL, 0, 0, NULL};
		NativeTrackRegistry_Add(registry, &other, NULL);
	}
	return registry;
}
int NativeTrackLibrary_Refresh(const char *directory, struct NativeTrackScanResult *result)
{
	(void)directory; (void)result; s_refresh++; return 1;
}
int NativeTextEdit_BeginForGame(const char *value, size_t maxBytes)
{
	(void)maxBytes; s_editValue = value; s_editResult = NATIVE_TEXT_EDIT_ACTIVE; return 1;
}
void NativeTextEdit_LimitCharacters(size_t maxCharacters) { s_characterLimit = maxCharacters; }
enum NativeTextEditResult NativeTextEdit_Result(void) { return s_editResult; }
const char *NativeTextEdit_Value(void) { return s_editValue; }
void NativeTextEdit_Close(void) { s_editResult = NATIVE_TEXT_EDIT_IDLE; }
int OtherFX_Play(u32 id, int flags) { s_sound = (int)id; (void)flags; return 0; }

#include <game/230/MM_TrackImport.c>

int main(void)
{
	static char *language[] = {"localized crash", "localized blizzard"};
	sdata->lngStrings = language;
	data.metaDataLEV[CRASH_COVE].name_LNG = 0;
	data.metaDataLEV[BLIZZARD_BLUFF].name_LNG = 1;
	MM_TrackImport_Reset();
	CTR_TEST_SECTION("folder review and cancellation");
	MM_TrackImport_Open();
	s_dialogResult = NATIVE_FOLDER_DIALOG_SELECTED;
	MM_TrackImport_Update(0);
	CTR_TEST_CHECK(s_prepared == 1 && s_commits == 0 && s_refresh == 0 && s_trackImportState == TRACK_IMPORT_REVIEW,
	               "selecting a folder prepares review without publishing");
	struct TrackImportReviewView view;
	MM_TrackImport_GetReview(&view);
	CTR_TEST_CHECK(strcmp(view.musicSource, "Retail") == 0, "imports without custom audio default to retail");
	CTR_TEST_CHECK(s_previewRetail == CRASH_COVE && s_previewIdentity[0] == '\0', "opening review previews the selected retail music");
	CTR_TEST_CHECK(strcmp(view.musicTrack, "localized crash") == 0, "retail music uses the localized level metadata");
	s_trackReviewRow = TRACK_REVIEW_MUSIC_SOURCE;
	MM_TrackImport_Update(BTN_RIGHT);
	MM_TrackImport_GetReview(&view);
	CTR_TEST_CHECK(strcmp(view.musicSource, "Retail") == 0, "custom source cannot be selected without custom audio");
	s_trackReviewRow = TRACK_REVIEW_MUSIC_TRACK;
	MM_TrackImport_Update(BTN_LEFT);
	CTR_TEST_CHECK(s_musicRetail == CRASH_COVE - 1, "retail music scrolls backwards");
	CTR_TEST_CHECK(s_previewRetail == CRASH_COVE - 1, "changing the retail selection updates the preview");
	MM_TrackImport_GetReview(&view);
	CTR_TEST_CHECK(strcmp(view.musicTrack, "localized blizzard") == 0, "scrolling resolves the selected level's localized name");
	s_trackReviewRow = TRACK_REVIEW_TRACK;
	MM_TrackImport_Update(BTN_DOWN);
	CTR_TEST_CHECK(s_sound == 0, "review navigation uses the shared menu navigation sound");
	MM_TrackImport_Update(BTN_DOWN);
	MM_TrackImport_Update(BTN_CROSS);
	CTR_TEST_CHECK(s_trackReviewEditing && s_editResult == NATIVE_TEXT_EDIT_ACTIVE && s_sound == 1,
	               "name opens the keyboard editor with the shared confirmation sound");
	CTR_TEST_CHECK(s_characterLimit == TRACK_REVIEW_TEXT_CHARACTERS, "import review limits metadata to 30 characters");
	s_editResult = NATIVE_TEXT_EDIT_CANCELLED;
	MM_TrackImport_Update(0);
	CTR_TEST_CHECK(!s_trackReviewEditing && s_trackImportState == TRACK_IMPORT_REVIEW, "escape cancels text editing only");
	MM_TrackImport_Update(BTN_CROSS);
	s_editValue = "New Name";
	s_editResult = NATIVE_TEXT_EDIT_ACCEPTED;
	MM_TrackImport_Update(0);
	CTR_TEST_CHECK(strcmp(s_lastTitle, "New Name") == 0 && strcmp(s_lastAuthor, "Author") == 0,
	               "enter saves the edited name without changing attribution");
	MM_TrackImport_Update(BTN_TRIANGLE);
	CTR_TEST_CHECK(s_discards == 1 && s_commits == 0 && s_refresh == 0 && s_sound == 2 &&
	               s_trackImportState == TRACK_IMPORT_DISCARDING,
	               "back starts asynchronous cleanup with the shared back sound");
	CTR_TEST_CHECK(s_previewRestores > 0, "leaving review restores the menu music");
	MM_TrackImport_Update(0);
	CTR_TEST_CHECK(s_trackImportState == TRACK_IMPORT_DISCARDING, "review cleanup remains responsive while pending");
	s_patchResult = -1;
	MM_TrackImport_Update(0);
	CTR_TEST_CHECK(s_trackImportState == TRACK_IMPORT_DISCARD_FAILED && strcmp(s_trackImportMessage, "Cleanup failed - confirm to retry") == 0,
	               "failed cleanup offers a retry even if staged files were partially removed");
	MM_TrackImport_Update(BTN_CROSS);
	CTR_TEST_CHECK(s_trackImportState == TRACK_IMPORT_DISCARDING && s_discards == 2, "cleanup can be retried after a failure");
	s_patchResult = 1;
	MM_TrackImport_Update(0);
	CTR_TEST_CHECK(s_trackImportState == TRACK_IMPORT_RESULT, "completed cleanup displays its result");
	MM_TrackImport_Update(BTN_CROSS);
	s_prepareError = "Invalid LEV or VRM";
	MM_TrackImport_Open();
	s_dialogResult = NATIVE_FOLDER_DIALOG_SELECTED;
	MM_TrackImport_Update(0);
	CTR_TEST_CHECK(s_trackImportState == TRACK_IMPORT_RESULT && strcmp(MM_TrackImport_Message(), s_prepareError) == 0 &&
	               s_commits == 0 && s_refresh == 0, "an invalid folder reports its error without publishing");
	MM_TrackImport_Update(BTN_CROSS);
	s_prepareError = NULL;
	s_musicAvailable = 1;
	s_musicRetail = -1;
	s_musicIdentity[0] = '\0';
	s_installedRegistry = NativeTrackRegistry_Create();
	struct NativeTrackMetadata installed = {"local:installed", "Installed", "installed.lev", "installed.vrm", "installed.sca", NULL, 0, 0, NULL};
	NativeTrackRegistry_Add(s_installedRegistry, &installed, NULL);
	CTR_TEST_SECTION("patch review and confirmation");
	MM_TrackImport_OpenPatch();
	s_dialogResult = NATIVE_FOLDER_DIALOG_SELECTED;
	MM_TrackImport_Update(0);
	MM_TrackImport_Update(BTN_TRIANGLE);
	CTR_TEST_CHECK(s_cancel == 1 && s_commits == 0, "back requests cancellation of a running patch");
	s_patchResult = -1;
	MM_TrackImport_Update(0);
	MM_TrackImport_Update(BTN_CROSS);
	MM_TrackImport_OpenPatch();
	s_dialogResult = NATIVE_FOLDER_DIALOG_SELECTED;
	MM_TrackImport_Update(0);
	s_patchResult = 1;
	MM_TrackImport_Update(0);
	CTR_TEST_CHECK(s_trackImportState == TRACK_IMPORT_REVIEW && s_commits == 0, "a prepared patch opens review before publication");
	MM_TrackImport_GetReview(&view);
	CTR_TEST_CHECK(strcmp(view.musicSource, "Custom") == 0 && view.musicIndex == 1 && view.musicTrack == NULL,
	               "an imported custom song is selected by default");
 	CTR_TEST_CHECK(view.included && view.includedCount == 2, "all imported tracks are selected initially");
	s_trackReviewRow = TRACK_REVIEW_INCLUDE;
	MM_TrackImport_Update(BTN_CROSS);
	MM_TrackImport_GetReview(&view);
	CTR_TEST_CHECK(!view.included && view.includedCount == 1, "a track can be excluded without removing it from review");
	s_trackReviewIndex = 1;
	MM_TrackImport_Update(BTN_CROSS);
	MM_TrackImport_GetReview(&view);
	CTR_TEST_CHECK(!view.included && view.includedCount == 0, "every candidate can be excluded");
	s_trackReviewRow = TRACK_REVIEW_CONFIRM;
	MM_TrackImport_Update(BTN_CROSS);
	CTR_TEST_CHECK(s_commits == 0 && s_trackImportState == TRACK_IMPORT_REVIEW, "an empty selection cannot be confirmed");
	s_trackReviewRow = TRACK_REVIEW_INCLUDE;
	MM_TrackImport_Update(BTN_CROSS);
	MM_TrackImport_GetReview(&view);
	CTR_TEST_CHECK(view.included && view.includedCount == 1, "an excluded track can be restored");
	s_trackReviewIndex = 0;
	s_trackReviewRow = TRACK_REVIEW_MUSIC_TRACK;
	MM_TrackImport_Update(BTN_RIGHT);
	MM_TrackImport_GetReview(&view);
	CTR_TEST_CHECK(view.musicIndex == 2 && strcmp(s_musicIdentity, "local:other") == 0,
	               "custom music can select another track in the current import");
	CTR_TEST_CHECK(strcmp(s_previewIdentity, "local:other") == 0, "the selected staged music is previewed");
	MM_TrackImport_Update(BTN_RIGHT);
	MM_TrackImport_GetReview(&view);
	CTR_TEST_CHECK(view.musicIndex == 1 && strcmp(s_musicIdentity, "local:track") == 0,
	               "custom choices wrap within this import and exclude installed tracks");
	/* A candidate without its own SCA can still use another candidate's audio. */
	NativeTrackRegistry_Destroy(s_trackReviewRegistry);
	s_musicAvailable = 2;
	s_trackReviewRegistry = NativeTrackLibrary_Scan("C:/staged", NULL);
	s_musicIdentity[0] = '\0';
	s_musicRetail = -1;
	s_trackReviewRow = TRACK_REVIEW_MUSIC_SOURCE;
	MM_TrackImport_Update(BTN_RIGHT);
	MM_TrackImport_GetReview(&view);
	CTR_TEST_CHECK(strcmp(view.musicSource, "Custom") == 0 && strcmp(s_musicIdentity, "local:other") == 0 &&
	               strcmp(s_previewIdentity, "local:other") == 0,
	               "a track without local audio can select and preview another candidate's SCA");
	s_trackReviewRow = TRACK_REVIEW_TRACK;
	MM_TrackImport_Update(BTN_DOWN);
	MM_TrackImport_Update(BTN_DOWN);
	MM_TrackImport_Update(BTN_DOWN);
	MM_TrackImport_Update(BTN_DOWN);
	MM_TrackImport_Update(BTN_DOWN);
	MM_TrackImport_Update(BTN_DOWN);
	MM_TrackImport_Update(BTN_CROSS);
	CTR_TEST_CHECK(s_commits == 1 && s_refresh == 0 && s_trackImportState == TRACK_IMPORT_INSTALLING,
	               "confirm starts installation without blocking for completion");
	s_patchResult = 1;
	MM_TrackImport_Update(0);
	CTR_TEST_CHECK(s_commits == 1 && s_refresh == 1 && s_trackImportState == TRACK_IMPORT_RESULT,
	               "confirm publishes the reviewed patch and refreshes the library");
	NativeTrackRegistry_Destroy(s_installedRegistry);
	return CtrTest_Summary();
}
