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
enum TrackImportState { TRACK_IMPORT_CLOSED, TRACK_IMPORT_CHOOSING, TRACK_IMPORT_PATCHING, TRACK_IMPORT_REVIEW, TRACK_IMPORT_INSTALLING, TRACK_IMPORT_RESULT };
static enum TrackImportState s_trackImportState;
static const char *s_trackImportMessage;
static bool s_trackImportIsPatch;
static struct NativeTrackRegistry *s_trackReviewRegistry;
static int s_trackReviewIndex, s_trackReviewRow, s_trackReviewEditing;

static const struct NativeTrackMetadata *MM_TrackImport_ReviewTrack(void)
{
	return NativeTrackRegistry_Get(s_trackReviewRegistry, NATIVE_TRACK_CUSTOM_ID_BASE + s_trackReviewIndex);
}

static void MM_TrackImport_ReviewClose(void)
{
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
	else if (s_trackImportState == TRACK_IMPORT_PATCHING || s_trackImportState == TRACK_IMPORT_INSTALLING)
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
	*view = (struct TrackImportReviewView){
	    .title = track->title, .author = track->author,
	    .editingValue = s_trackReviewEditing ? NativeTextEdit_Value() : NULL,
	    .message = s_trackImportMessage, .index = s_trackReviewIndex,
	    .count = (int)NativeTrackRegistry_Count(s_trackReviewRegistry), .row = s_trackReviewRow,
	    .editing = s_trackReviewEditing,
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
				char identity[256];
				snprintf(identity, sizeof(identity), "%s", track->identity);
				struct NativeTrackRegistry *updated = NativeTrackLibrary_Scan(NativeTrackPatch_ReviewDirectory(), NULL);
				int level;
				if (updated != NULL && NativeTrackRegistry_Find(updated, identity, &level))
				{
					NativeTrackRegistry_Destroy(s_trackReviewRegistry);
					s_trackReviewRegistry = updated;
					s_trackReviewIndex = level - NATIVE_TRACK_CUSTOM_ID_BASE;
				}
				else { NativeTrackRegistry_Destroy(updated); }
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
	if (s_trackReviewRow == TRACK_REVIEW_TRACK && count > 0)
	{
		if (buttons & BTN_LEFT) { s_trackReviewIndex = (s_trackReviewIndex + count - 1) % count; OtherFX_Play(0, 1); }
		if (buttons & BTN_RIGHT) { s_trackReviewIndex = (s_trackReviewIndex + 1) % count; OtherFX_Play(0, 1); }
		if (confirm) { s_trackReviewIndex = (s_trackReviewIndex + 1) % count; }
	}
	if (back || (confirm && s_trackReviewRow == TRACK_REVIEW_CANCEL))
	{
		MM_TrackImport_ReviewClose();
		s_trackImportMessage = NativeTrackPatch_ReviewDiscard() ? "Import cancelled" : "Cannot discard import";
		s_trackImportState = TRACK_IMPORT_RESULT;
	}
	else if (confirm && (s_trackReviewRow == TRACK_REVIEW_TITLE || s_trackReviewRow == TRACK_REVIEW_AUTHOR))
	{
		const char *value = s_trackReviewRow == TRACK_REVIEW_TITLE ? track->title : track->author;
		s_trackReviewEditing = NativeTextEdit_BeginForGame(value != NULL ? value : "", 256);
	}
	else if (confirm && s_trackReviewRow == TRACK_REVIEW_CONFIRM)
	{
		if (NativeTrackPatch_ReviewCommitStart())
		{
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
	else if ((buttons & (BTN_CROSS | BTN_CIRCLE | BTN_TRIANGLE | BTN_START)) != 0)
	{
		OtherFX_Play((buttons & (BTN_TRIANGLE | BTN_START)) != 0 ? 2 : 1, 1);
		s_trackImportState = TRACK_IMPORT_CLOSED;
	}
	return 1;
}

const char *MM_TrackImport_Message(void)
{
	return s_trackImportState == TRACK_IMPORT_CHOOSING ? (s_trackImportIsPatch ? "Choose an xdelta patch" : "Choose a track folder") :
	       (s_trackImportState == TRACK_IMPORT_PATCHING || s_trackImportState == TRACK_IMPORT_INSTALLING) ? NativeTrackPatch_Message() :
	       s_trackImportState == TRACK_IMPORT_RESULT ? s_trackImportMessage : "";
}
