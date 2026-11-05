#ifndef PLATFORM_NATIVE_MEMCARD_H
#define PLATFORM_NATIVE_MEMCARD_H

enum NativeMemcardResult
{
	NATIVE_MEMCARD_OK = 0,
	NATIVE_MEMCARD_NOT_FOUND,
	NATIVE_MEMCARD_OPEN_FAILED,
	NATIVE_MEMCARD_IO_ERROR,
};

enum NativeMemcardResult NativeMemcard_SetRoot(const char *root_path);
const char *NativeMemcard_GetRoot(void);
void NativeMemcard_ClearRoot(void);
enum NativeMemcardResult NativeMemcard_CloneRoot(const char *src_root, const char *dst_root);
enum NativeMemcardResult NativeMemcard_CloneCurrentRoot(const char *dst_root);
enum NativeMemcardResult NativeMemcard_RemoveRoot(const char *root_path);
int NativeMemcard_FileExists(const char *save_name);
int NativeMemcard_FindFirstFile(const char *pattern, char *dst_name, int dst_size);
int NativeMemcard_FindNextFile(char *dst_name, int dst_size);
enum NativeMemcardResult NativeMemcard_RemoveFile(const char *save_name);
enum NativeMemcardResult NativeMemcard_ReadSaveData(const char *save_name, unsigned char *dst, int byte_count, int data_offset);
enum NativeMemcardResult NativeMemcard_WriteSaveData(const char *save_name, const void *icon, int icon_byte_count, const unsigned char *src, int byte_count);

#endif
