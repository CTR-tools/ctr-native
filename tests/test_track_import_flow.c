#include <shared/ctr_test.h>
#include <common.h>
#include <platform/native_folder_dialog.h>
#include <platform/native_track_library.h>

struct sData sdata_static;
static enum NativeFolderDialogResult s_dialogResult;
static int s_importCalls, s_refreshCalls, s_sound;
static const char *s_importError;
static char s_source[1024], s_name[1024];
static const char *s_dialogPath = "C:/download/Example Track/";
static int s_patchResult, s_patchCancel;

int NativeFolderDialog_Open(void) { s_dialogResult = NATIVE_FOLDER_DIALOG_PENDING; return 1; }
int NativeFileDialog_OpenPatch(void) { return NativeFolderDialog_Open(); }
int NativeTrackPatch_Start(const char *assets, const char *patch) { (void)assets; (void)patch; s_patchResult = 0; return 1; }
int NativeTrackPatch_Poll(void) { return s_patchResult; }
const char *NativeTrackPatch_Message(void) { return s_patchResult > 0 ? "1 track imported" : "Applying xdelta patch"; }
void NativeTrackPatch_Cancel(void) { s_patchCancel++; }
enum NativeFolderDialogResult NativeFolderDialog_Result(void) { return s_dialogResult; }
const char *NativeFolderDialog_Path(void) { return s_dialogPath; }
const char *NativeAssets_GetAssetDir(void) { return "C:/game/assets"; }
const char *NativeTrackImport_Directory(const char *library, const char *source, const char *name)
{
	(void)library;
	s_importCalls++;
	snprintf(s_source, sizeof(s_source), "%s", source);
	snprintf(s_name, sizeof(s_name), "%s", name);
	return s_importError;
}
int NativeTrackLibrary_Refresh(const char *directory, struct NativeTrackScanResult *result)
{
	(void)directory; (void)result; s_refreshCalls++; return 1;
}
int OtherFX_Play(u32 id, int flags) { (void)flags; s_sound = (int)id; return 0; }

#include <game/230/MM_TrackImport.c>

int main(void)
{
	CTR_TEST_SECTION("folder dialog import flow");
	MM_TrackImport_Open();
	CTR_TEST_CHECK(MM_TrackImport_Update(0) && s_importCalls == 0, "a pending dialog does not import anything");
	s_dialogResult = NATIVE_FOLDER_DIALOG_CANCELLED;
	CTR_TEST_CHECK(MM_TrackImport_Update(0) && s_importCalls == 0 && s_sound == 2, "cancel returns without importing and plays back");
	CTR_TEST_CHECK(!MM_TrackImport_Update(0), "cancel closes the import screen");
	MM_TrackImport_Open();
	s_dialogResult = NATIVE_FOLDER_DIALOG_SELECTED;
	MM_TrackImport_Update(0);
	CTR_TEST_CHECK(s_importCalls == 1 && s_refreshCalls == 1 && strcmp(s_source, "C:/download/Example Track") == 0 &&
	               strcmp(s_name, "Example Track") == 0, "the chosen folder is imported and the library refreshed");
	CTR_TEST_CHECK(strcmp(MM_TrackImport_Message(), "Track imported") == 0, "success is reported");
	MM_TrackImport_Update(BTN_CROSS);
	CTR_TEST_CHECK(s_sound == 1 && !MM_TrackImport_Update(0), "confirm closes the result with the confirm sound");
	s_importError = "Invalid LEV or VRM";
	MM_TrackImport_Open();
	s_dialogResult = NATIVE_FOLDER_DIALOG_SELECTED;
	MM_TrackImport_Update(0);
	CTR_TEST_CHECK(s_refreshCalls == 1 && strcmp(MM_TrackImport_Message(), s_importError) == 0,
	               "an import failure is reported without refreshing the library");
	MM_TrackImport_Update(BTN_TRIANGLE);
	CTR_TEST_CHECK(s_sound == 2 && !MM_TrackImport_Update(0), "back closes the result with the back sound");
	char longPath[1100];
	memset(longPath, 'a', sizeof(longPath) - 1);
	longPath[sizeof(longPath) - 1] = '\0';
	s_dialogPath = longPath;
	MM_TrackImport_Open();
	s_dialogResult = NATIVE_FOLDER_DIALOG_SELECTED;
	MM_TrackImport_Update(0);
	CTR_TEST_CHECK(s_importCalls == 2 && strcmp(MM_TrackImport_Message(), "Path is too long") == 0,
	               "an oversized folder path is rejected instead of importing a truncated path");
	MM_TrackImport_Update(BTN_CROSS);
	CTR_TEST_SECTION("xdelta background import flow");
	s_dialogPath = "C:/download/Example.xdelta";
	MM_TrackImport_OpenPatch();
	s_dialogResult = NATIVE_FOLDER_DIALOG_SELECTED;
	MM_TrackImport_Update(0);
	CTR_TEST_CHECK(s_trackImportState == TRACK_IMPORT_PATCHING && s_importCalls == 2,
	               "selecting a patch starts the worker without using folder import");
	MM_TrackImport_Update(BTN_CROSS);
	CTR_TEST_CHECK(s_trackImportState == TRACK_IMPORT_PATCHING && s_refreshCalls == 1,
	               "confirm cannot close an active import or publish its library early");
	MM_TrackImport_Update(BTN_TRIANGLE);
	CTR_TEST_CHECK(s_patchCancel == 1 && s_trackImportState == TRACK_IMPORT_PATCHING,
	               "back cancels and waits for worker cleanup");
	s_patchResult = -1;
	MM_TrackImport_Update(0);
	CTR_TEST_CHECK(s_refreshCalls == 1 && s_trackImportState == TRACK_IMPORT_RESULT,
	               "a failed patch does not refresh the library");
	MM_TrackImport_Update(BTN_CROSS);
	MM_TrackImport_OpenPatch();
	s_dialogResult = NATIVE_FOLDER_DIALOG_SELECTED;
	MM_TrackImport_Update(0);
	s_patchResult = 1;
	MM_TrackImport_Update(0);
	CTR_TEST_CHECK(s_refreshCalls == 2 && strcmp(MM_TrackImport_Message(), "1 track imported") == 0,
	               "a completed import refreshes the library and reports the result");
	return CtrTest_Summary();
}
