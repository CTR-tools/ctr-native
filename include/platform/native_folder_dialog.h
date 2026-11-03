#ifndef CTR_NATIVE_FOLDER_DIALOG_H
#define CTR_NATIVE_FOLDER_DIALOG_H

enum NativeFolderDialogResult
{
	NATIVE_FOLDER_DIALOG_PENDING,
	NATIVE_FOLDER_DIALOG_SELECTED,
	NATIVE_FOLDER_DIALOG_CANCELLED,
	NATIVE_FOLDER_DIALOG_ERROR,
};

int NativeFolderDialog_Open(void);
/* Shares the result/path accessors; only one native import dialog is open. */
int NativeFileDialog_OpenPatch(void);
enum NativeFolderDialogResult NativeFolderDialog_Result(void);
const char *NativeFolderDialog_Path(void);

#endif
