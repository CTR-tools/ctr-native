#include <common.h>
#include <platform/native_assets.h>
#include <platform/native_folder_dialog.h>
#include <platform/native_track_import.h>
#include <platform/native_track_library.h>
#include <platform/native_track_patch.h>
#include <stdio.h>

enum { TRACK_IMPORT_PATH_BYTES = 1024 };
enum TrackImportState { TRACK_IMPORT_CLOSED, TRACK_IMPORT_CHOOSING, TRACK_IMPORT_PATCHING, TRACK_IMPORT_RESULT };
static enum TrackImportState s_trackImportState;
static const char *s_trackImportMessage;
static bool s_trackImportIsPatch;

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

static void MM_TrackImport_Install(const char *path)
{
	char source[TRACK_IMPORT_PATH_BYTES], library[TRACK_IMPORT_PATH_BYTES];
	int sourceLength = snprintf(source, sizeof(source), "%s", path);
	if (sourceLength < 0 || (size_t)sourceLength >= sizeof(source))
	{
		s_trackImportMessage = "Path is too long";
		s_trackImportState = TRACK_IMPORT_RESULT;
		return;
	}
	size_t length = strlen(source);
	while (length > 0 && (source[length - 1] == '/' || source[length - 1] == '\\'))
	{
		source[--length] = '\0';
	}
	const char *name = source + length;
	while (name > source && name[-1] != '/' && name[-1] != '\\')
	{
		name--;
	}
	int n = snprintf(library, sizeof(library), "%s/tracks", NativeAssets_GetAssetDir());
	const char *error = n < 0 || (size_t)n >= sizeof(library) ? "Path is too long" :
	                    NativeTrackImport_Directory(library, source, name);
	if (error == NULL && NativeTrackLibrary_Refresh(library, NULL))
	{
		s_trackImportMessage = "Track imported";
	}
	else
	{
		s_trackImportMessage = error != NULL ? error : "Imported - restart game";
	}
	s_trackImportState = TRACK_IMPORT_RESULT;
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
				if (NativeTrackPatch_Start(NativeAssets_GetAssetDir(), NativeFolderDialog_Path()))
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
				MM_TrackImport_Install(NativeFolderDialog_Path());
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
				char library[TRACK_IMPORT_PATH_BYTES];
				int n = snprintf(library, sizeof(library), "%s/tracks", NativeAssets_GetAssetDir());
				if (n < 0 || (size_t)n >= sizeof(library) || !NativeTrackLibrary_Refresh(library, NULL))
				{
					s_trackImportMessage = "Imported - restart game";
				}
			}
			s_trackImportState = TRACK_IMPORT_RESULT;
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
	       s_trackImportState == TRACK_IMPORT_PATCHING ? NativeTrackPatch_Message() :
	       s_trackImportState == TRACK_IMPORT_RESULT ? s_trackImportMessage : "";
}
