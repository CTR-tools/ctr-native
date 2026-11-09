#include <common.h>
#include <platform/native_assets.h>
#include <platform/native_folder_dialog.h>
#include <platform/native_track_import.h>
#include <platform/native_track_library.h>
#include <platform/native_track_patch.h>
#include <platform/native_text_edit.h>
#include <track_import_view.h>
#include <stdio.h>

#include "MM_FullScreenMenu.c"

enum { TRACK_IMPORT_PATH_BYTES = 1024 };
enum TrackImportState { TRACK_IMPORT_CLOSED, TRACK_IMPORT_CHOOSING, TRACK_IMPORT_PATCHING, TRACK_IMPORT_REVIEW, TRACK_IMPORT_INSTALLING, TRACK_IMPORT_DISCARDING, TRACK_IMPORT_RESULT };
static enum TrackImportState s_trackImportState;
static const char *s_trackImportMessage;
static bool s_trackImportIsPatch;
static struct NativeTrackRegistry *s_trackReviewRegistry;
static int s_trackReviewIndex, s_trackReviewRow, s_trackReviewEditing;

enum { TRACK_IMPORT_RETAIL_MUSIC_COUNT = TURBO_TRACK + 1 };

static int MM_TrackImport_CustomMusicCount(void)
{
	int count = 0;
	for (int i = 0; i < (int)NativeTrackRegistry_Count(s_trackReviewRegistry); i++)
	{
		const struct NativeTrackMetadata *track = NativeTrackRegistry_Get(s_trackReviewRegistry, NATIVE_TRACK_CUSTOM_ID_BASE + i);
		if (track != NULL && track->scaPath != NULL) { count++; }
	}
	return count;
}

static const struct NativeTrackMetadata *MM_TrackImport_CustomMusic(int index)
{
	for (int i = 0; i < (int)NativeTrackRegistry_Count(s_trackReviewRegistry); i++)
	{
		const struct NativeTrackMetadata *track = NativeTrackRegistry_Get(s_trackReviewRegistry, NATIVE_TRACK_CUSTOM_ID_BASE + i);
		if (track != NULL && track->scaPath != NULL && index-- == 0) { return track; }
	}
	return NULL;
}

static int MM_TrackImport_MusicChoice(const struct NativeTrackMetadata *track, int *custom)
{
	int retail = NativeTrackLibrary_Music(track);
	const char *selected = NativeTrackPatch_ReviewMusic(track->identity);
	int count = MM_TrackImport_CustomMusicCount();
	if (count == 0 || (selected == NULL && (retail >= 0 || track->scaPath == NULL)))
	{ *custom = 0; return retail >= 0 ? retail : CRASH_COVE; }
	*custom = 1;
	const char *wanted = selected != NULL ? selected : track->identity;
	for (int i = 0; i < count; i++)
	{
		if (strcmp(MM_TrackImport_CustomMusic(i)->identity, wanted) == 0) { return i; }
	}
	return 0;
}

static const struct NativeTrackMetadata *MM_TrackImport_ReviewTrack(void)
{
	return NativeTrackRegistry_Get(s_trackReviewRegistry, NATIVE_TRACK_CUSTOM_ID_BASE + s_trackReviewIndex);
}

static int MM_TrackImport_IncludedCount(void)
{
	int count = 0;
	for (int i = 0; i < (int)NativeTrackRegistry_Count(s_trackReviewRegistry); i++)
	{
		const struct NativeTrackMetadata *track = NativeTrackRegistry_Get(s_trackReviewRegistry, NATIVE_TRACK_CUSTOM_ID_BASE + i);
		if (track != NULL && NativeTrackPatch_ReviewIncluded(track->identity)) { count++; }
	}
	return count;
}

 static void MM_TrackImport_PreviewMusic(void)
{
	const struct NativeTrackMetadata *track = MM_TrackImport_ReviewTrack();
	if (track == NULL) { return; }
	int custom;
	int choice = MM_TrackImport_MusicChoice(track, &custom);
	MusicPreview_Request(custom ? -1 : choice, custom ? MM_TrackImport_CustomMusic(choice) : NULL);
}

static int MM_TrackImport_ReviewRescan(const char *identity)
{
	char saved[256];
	snprintf(saved, sizeof(saved), "%s", identity);
	struct NativeTrackRegistry *updated = NativeTrackLibrary_Scan(NativeTrackPatch_ReviewDirectory(), NULL);
	int level;
	if (updated == NULL || !NativeTrackRegistry_Find(updated, saved, &level))
	{
		NativeTrackRegistry_Destroy(updated);
		return 0;
	}
	NativeTrackRegistry_Destroy(s_trackReviewRegistry);
	s_trackReviewRegistry = updated;
	s_trackReviewIndex = level - NATIVE_TRACK_CUSTOM_ID_BASE;
	return 1;
}

static void MM_TrackImport_ReviewClose(void)
{
	MusicPreview_Restore();
	NativeTextEdit_Close();
	NativeTrackRegistry_Destroy(s_trackReviewRegistry);
	s_trackReviewRegistry = NULL;
}

void MM_TrackImport_Reset(void)
{
	if (s_trackImportState == TRACK_IMPORT_REVIEW)
	{
		MM_TrackImport_ReviewClose();
		NativeTrackPatch_ReviewDiscard();
	}
	else if (s_trackImportState == TRACK_IMPORT_PATCHING || s_trackImportState == TRACK_IMPORT_INSTALLING || s_trackImportState == TRACK_IMPORT_DISCARDING)
	{
		NativeTrackPatch_Shutdown();
		MM_TrackImport_ReviewClose();
	}
	s_trackImportState = TRACK_IMPORT_CLOSED;
}

int MM_TrackImport_GetReview(struct TrackImportReviewView *view)
{
	if (view == NULL || s_trackImportState != TRACK_IMPORT_REVIEW) { return 0; }
	const struct NativeTrackMetadata *track = MM_TrackImport_ReviewTrack();
	if (track == NULL) { return 0; }
	int custom;
	int choice = MM_TrackImport_MusicChoice(track, &custom);
	*view = (struct TrackImportReviewView){
	    .title = track->title, .author = track->author,
	    .editingValue = s_trackReviewEditing ? NativeTextEdit_Value() : NULL,
	    .message = s_trackImportMessage, .index = s_trackReviewIndex,
	    .count = (int)NativeTrackRegistry_Count(s_trackReviewRegistry), .row = s_trackReviewRow,
	    .editing = s_trackReviewEditing,
	    .musicSource = custom ? "Custom" : "Retail",
	    .musicTrack = custom ? NULL : sdata->lngStrings[data.metaDataLEV[choice].name_LNG],
	    .musicIndex = custom ? choice + 1 : 0,
	    .included = NativeTrackPatch_ReviewIncluded(track->identity), .includedCount = MM_TrackImport_IncludedCount(),
	};
	return 1;
}

static void MM_TrackImport_ReviewOpen(void)
{
	const char *directory = NativeTrackPatch_ReviewDirectory();
	s_trackReviewRegistry = directory != NULL ? NativeTrackLibrary_Scan(directory, NULL) : NULL;
	if (s_trackReviewRegistry == NULL || NativeTrackRegistry_Count(s_trackReviewRegistry) == 0)
	{
		MM_TrackImport_ReviewClose();
		NativeTrackPatch_ReviewDiscard();
		s_trackImportMessage = "Cannot review imported tracks";
		s_trackImportState = TRACK_IMPORT_RESULT;
		return;
	}
	s_trackReviewIndex = 0;
	s_trackReviewRow = 0;
	s_trackReviewEditing = 0;
	s_trackImportMessage = NULL;
	s_trackImportState = TRACK_IMPORT_REVIEW;
	MM_TrackImport_PreviewMusic();
}

void MM_TrackImport_Open(void)
{
	s_trackImportIsPatch = false;
	if (NativeFolderDialog_Open())
	{
		s_trackImportState = TRACK_IMPORT_CHOOSING;
	}
	else
	{
		s_trackImportState = TRACK_IMPORT_RESULT;
		s_trackImportMessage = "Cannot open folder dialog";
	}
}

void MM_TrackImport_OpenPatch(void)
{
	s_trackImportIsPatch = true;
	if (NativeFileDialog_OpenPatch())
	{
		s_trackImportState = TRACK_IMPORT_CHOOSING;
	}
	else
	{
		s_trackImportState = TRACK_IMPORT_RESULT;
		s_trackImportMessage = "Cannot open patch dialog";
	}
}

static void MM_TrackImport_PrepareFolder(const char *path)
{
	const char *error = NativeTrackPatch_PrepareFolder(NativeAssets_GetAssetDir(), path);
	if (error == NULL) { MM_TrackImport_ReviewOpen(); }
	else { s_trackImportMessage = error; s_trackImportState = TRACK_IMPORT_RESULT; }
}

static void MM_TrackImport_ReviewUpdate(u32 buttons)
{
	const struct NativeTrackMetadata *track = MM_TrackImport_ReviewTrack();
	if (track == NULL) { return; }
	if (s_trackReviewEditing)
	{
		enum NativeTextEditResult result = NativeTextEdit_Result();
		if (result == NATIVE_TEXT_EDIT_ACTIVE) { return; }
		if (result == NATIVE_TEXT_EDIT_ACCEPTED)
		{
			const char *value = NativeTextEdit_Value();
			const char *error = NativeTrackPatch_ReviewUpdate(track->identity,
			    s_trackReviewRow == TRACK_REVIEW_TITLE ? value : track->title,
			    s_trackReviewRow == TRACK_REVIEW_AUTHOR ? value : (track->author != NULL ? track->author : ""));
			if (error == NULL)
			{
				if (!MM_TrackImport_ReviewRescan(track->identity)) { s_trackImportMessage = "Cannot refresh review"; }
			}
			else { s_trackImportMessage = error; OtherFX_Play(2, 1); }
		}
		NativeTextEdit_Close();
		s_trackReviewEditing = 0;
		return;
	}
	short selected = (short)s_trackReviewRow;
	struct FullScreenMenu_Scroll scroll = {0};
	FullScreenMenu_NavTapped(buttons, &scroll, &selected, TRACK_REVIEW_ROWS);
	s_trackReviewRow = selected;
	int back = FullScreenMenu_Back(buttons);
	int confirm = !back && FullScreenMenu_Confirm(buttons);
	int count = (int)NativeTrackRegistry_Count(s_trackReviewRegistry);
	if (!back && s_trackReviewRow == TRACK_REVIEW_INCLUDE && ((buttons & (BTN_LEFT | BTN_RIGHT)) != 0 || confirm))
	{
		if (!NativeTrackPatch_ReviewSetIncluded(track->identity, !NativeTrackPatch_ReviewIncluded(track->identity)))
		{
			s_trackImportMessage = "Cannot change track selection";
			OtherFX_Play(2, 1);
		}
		else
		{
			s_trackImportMessage = NULL;
			if (!confirm) { OtherFX_Play(0, 1); }
		}
	}
	if (!back && (s_trackReviewRow == TRACK_REVIEW_MUSIC_SOURCE || s_trackReviewRow == TRACK_REVIEW_MUSIC_TRACK) &&
	    ((buttons & (BTN_LEFT | BTN_RIGHT)) != 0 || confirm))
	{
		int custom;
		int choice = MM_TrackImport_MusicChoice(track, &custom);
		int customCount = MM_TrackImport_CustomMusicCount();
		int direction = (buttons & BTN_LEFT) != 0 ? -1 : 1;
		if (s_trackReviewRow == TRACK_REVIEW_MUSIC_SOURCE)
		{
			if (customCount > 0) { custom = !custom; choice = custom ? 0 : CRASH_COVE; }
		}
		else
		{
			int choices = custom ? customCount : TRACK_IMPORT_RETAIL_MUSIC_COUNT;
			choice = (choice + choices + direction) % choices;
		}
		const struct NativeTrackMetadata *music = custom ? MM_TrackImport_CustomMusic(choice) : NULL;
		int saved = custom ? NativeTrackPatch_ReviewSetMusic(track->identity, music->identity) :
		    NativeTrackLibrary_SetMusic(track, choice, 0) && NativeTrackPatch_ReviewSetMusic(track->identity, NULL);
		if (!saved)
		{
			s_trackImportMessage = "Cannot save music selection";
			OtherFX_Play(2, 1);
		}
		else if (!custom && !MM_TrackImport_ReviewRescan(track->identity)) { s_trackImportMessage = "Cannot refresh review"; }
		else if (!confirm) { OtherFX_Play(0, 1); }
	}
	if (s_trackReviewRow == TRACK_REVIEW_TRACK && count > 0)
	{
		if (buttons & BTN_LEFT) { s_trackReviewIndex = (s_trackReviewIndex + count - 1) % count; OtherFX_Play(0, 1); }
		if (buttons & BTN_RIGHT) { s_trackReviewIndex = (s_trackReviewIndex + 1) % count; OtherFX_Play(0, 1); }
		if (confirm) { s_trackReviewIndex = (s_trackReviewIndex + 1) % count; }
	}
	if (back || (confirm && s_trackReviewRow == TRACK_REVIEW_CANCEL))
	{
		if (NativeTrackPatch_ReviewDiscardStart())
		{
			MM_TrackImport_ReviewClose();
			s_trackImportState = TRACK_IMPORT_DISCARDING;
		}
		else { s_trackImportMessage = "Cannot discard import"; }
	}
	else if (confirm && (s_trackReviewRow == TRACK_REVIEW_TITLE || s_trackReviewRow == TRACK_REVIEW_AUTHOR))
	{
		const char *value = s_trackReviewRow == TRACK_REVIEW_TITLE ? track->title : track->author;
		s_trackReviewEditing = NativeTextEdit_BeginForGame(value != NULL ? value : "", 256);
	}
	else if (confirm && s_trackReviewRow == TRACK_REVIEW_CONFIRM)
	{
		if (MM_TrackImport_IncludedCount() == 0)
		{
			s_trackImportMessage = "Select at least one track";
			OtherFX_Play(2, 1);
		}
		else if (NativeTrackPatch_ReviewCommitStart())
		{
			MusicPreview_Restore();
			s_trackImportState = TRACK_IMPORT_INSTALLING;
		}
		else { s_trackImportMessage = "Cannot start import"; }
	}
}

int MM_TrackImport_Update(u32 buttons)
{
	if (s_trackImportState == TRACK_IMPORT_CLOSED)
	{
		return 0;
	}
	if (s_trackImportState == TRACK_IMPORT_CHOOSING)
	{
		enum NativeFolderDialogResult result = NativeFolderDialog_Result();
		if (result == NATIVE_FOLDER_DIALOG_SELECTED)
		{
			if (s_trackImportIsPatch)
			{
				if (NativeTrackPatch_StartReview(NativeAssets_GetAssetDir(), NativeFolderDialog_Path()))
				{
					s_trackImportState = TRACK_IMPORT_PATCHING;
				}
				else
				{
					s_trackImportState = TRACK_IMPORT_RESULT;
					s_trackImportMessage = "Cannot start xdelta import";
				}
			}
			else
			{
				MM_TrackImport_PrepareFolder(NativeFolderDialog_Path());
			}
		}
		else if (result == NATIVE_FOLDER_DIALOG_CANCELLED)
		{
			s_trackImportState = TRACK_IMPORT_CLOSED;
			OtherFX_Play(2, 1);
		}
		else if (result == NATIVE_FOLDER_DIALOG_ERROR)
		{
			s_trackImportState = TRACK_IMPORT_RESULT;
			s_trackImportMessage = "Folder dialog failed";
		}
	}
	else if (s_trackImportState == TRACK_IMPORT_PATCHING)
	{
		if ((buttons & (BTN_TRIANGLE | BTN_START)) != 0)
		{
			NativeTrackPatch_Cancel();
		}
		int result = NativeTrackPatch_Poll();
		if (result != 0)
		{
			s_trackImportMessage = NativeTrackPatch_Message();
			if (result > 0)
			{
				MM_TrackImport_ReviewOpen();
				return 1;
			}
			s_trackImportState = TRACK_IMPORT_RESULT;
		}
	}
	else if (s_trackImportState == TRACK_IMPORT_REVIEW)
	{
		MM_TrackImport_ReviewUpdate(buttons);
		if (s_trackImportState == TRACK_IMPORT_REVIEW) { MM_TrackImport_PreviewMusic(); }
	}
	else if (s_trackImportState == TRACK_IMPORT_INSTALLING)
	{
		int result = NativeTrackPatch_Poll();
		if (result != 0)
		{
			s_trackImportMessage = NativeTrackPatch_Message();
			if (result < 0 && NativeTrackPatch_ReviewDirectory() != NULL)
			{
				s_trackImportState = TRACK_IMPORT_REVIEW;
			}
			else
			{
				NativeTrackLibrary_Replace(NativeTrackPatch_TakeRegistry());
				MM_TrackImport_ReviewClose();
				s_trackImportState = TRACK_IMPORT_RESULT;
			}
		}
	}
	else if (s_trackImportState == TRACK_IMPORT_DISCARDING)
	{
		if (NativeTrackPatch_Poll() != 0)
		{
			s_trackImportMessage = NativeTrackPatch_Message();
			s_trackImportState = TRACK_IMPORT_RESULT;
		}
	}
	else if (!MusicPreview_Busy() && (buttons & (BTN_CROSS | BTN_CIRCLE | BTN_TRIANGLE | BTN_START)) != 0)
	{
		OtherFX_Play((buttons & (BTN_TRIANGLE | BTN_START)) != 0 ? 2 : 1, 1);
		s_trackImportState = TRACK_IMPORT_CLOSED;
	}
	return 1;
}

const char *MM_TrackImport_Message(void)
{
	return s_trackImportState == TRACK_IMPORT_CHOOSING ? (s_trackImportIsPatch ? "Choose an xdelta patch" : "Choose a track folder") :
	       (s_trackImportState == TRACK_IMPORT_PATCHING || s_trackImportState == TRACK_IMPORT_INSTALLING || s_trackImportState == TRACK_IMPORT_DISCARDING) ? NativeTrackPatch_Message() :
	       s_trackImportState == TRACK_IMPORT_RESULT ? s_trackImportMessage : "";
}
