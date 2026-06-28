#include <platform/native_memcard.h>

#include <macros.h>

#include "platform/native_path.h"

#include <errno.h>
#if defined(_WIN32)
#include "platform/native_win32.h"
#else
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#include <stdio.h>
#include <string.h>

#define NATIVE_MEMCARD_MAX_FOUND_FILES 64
#define NATIVE_MEMCARD_MAX_NAME        64
#define NATIVE_MEMCARD_MAX_PATH        512
#define NATIVE_MEMCARD_COPY_BUFFER     8192
#define NATIVE_MEMCARD_DEFAULT_ROOT    "memcards"
#define NATIVE_MEMCARD_SLOT_COUNT      2

struct NativeMemcardFindState
{
	char names[NATIVE_MEMCARD_MAX_FOUND_FILES][NATIVE_MEMCARD_MAX_NAME];
	int count;
	int index;
};

struct NativeMemcardDeviceName
{
	const char *name;
	int slot;
};

global_variable struct NativeMemcardFindState s_memcardFind;
global_variable char s_memcardRoot[NATIVE_MEMCARD_MAX_PATH];
global_variable char s_memcardResolvedPath[NATIVE_MEMCARD_MAX_PATH];

internal struct NativeMemcardDeviceName NativeMemcard_ParseDeviceName(const char *save_name)
{
	struct NativeMemcardDeviceName result = {save_name, 0};

	if (save_name == NULL)
	{
		result.name = "";
		return result;
	}

	if ((save_name[0] == 'b') && (save_name[1] == 'u') && (save_name[2] >= '0') && (save_name[2] <= '9') && (save_name[3] >= '0') && (save_name[3] <= '9') &&
	    (save_name[4] == ':'))
	{
		result.name = save_name + 5;
		result.slot = (save_name[2] == '0') ? save_name[3] - '0' : save_name[2] - '0';
		result.slot %= NATIVE_MEMCARD_SLOT_COUNT;
	}

	return result;
}

internal const char *NativeMemcard_ActiveRoot(void)
{
	if (s_memcardRoot[0] == '\0')
	{
		return NATIVE_MEMCARD_DEFAULT_ROOT;
	}

	return s_memcardRoot;
}

internal int NativeMemcard_CopyString(char *dst, int dst_size, const char *src)
{
	int i;

	if (dst_size <= 0)
	{
		return 0;
	}

	for (i = 0; (i < dst_size - 1) && (src[i] != '\0'); i++)
	{
		dst[i] = src[i];
	}

	dst[i] = '\0';
	return src[i] == '\0';
}

internal int NativeMemcard_IsCtrSaveName(const char *name)
{
	return (strcmp(name, "BASCUS-94426-SLOTS") == 0) || (strncmp(name, "BASCUS-94426G", 13) == 0);
}

internal int NativeMemcard_JoinPath(char *dst, int dst_size, const char *left, const char *right)
{
	return NativePath_Join(dst, (size_t)dst_size, NativeStr8_FromCString(left), NativeStr8_FromCString(right));
}

internal int NativeMemcard_SlotDir(char *dst, int dst_size, int slot)
{
	int written;

	if (slot < 0)
	{
		slot = 0;
	}

	written = snprintf(dst, (size_t)dst_size, "slot%d", slot);
	return (written >= 0) && (written < dst_size);
}

internal int NativeMemcard_BuildSlotRoot(char *dst, int dst_size, const char *root, int slot)
{
	char slotDir[16];

	if (!NativeMemcard_SlotDir(slotDir, sizeof(slotDir), slot))
	{
		return 0;
	}

	return NativeMemcard_JoinPath(dst, dst_size, root, slotDir);
}

internal int NativeMemcard_PathExists(const char *path)
{
#if defined(_WIN32)
	DWORD attributes;

	if ((path == NULL) || (path[0] == '\0'))
		return 0;

	attributes = GetFileAttributesA(path);
	return attributes != INVALID_FILE_ATTRIBUTES;
#else
	struct stat st;

	if ((path == NULL) || (path[0] == '\0'))
	{
		return 0;
	}

	return stat(path, &st) == 0;
#endif
}

internal int NativeMemcard_PathIsDirectory(const char *path)
{
#if defined(_WIN32)
	DWORD attributes;

	if ((path == NULL) || (path[0] == '\0'))
		return 0;

	attributes = GetFileAttributesA(path);
	return (attributes != INVALID_FILE_ATTRIBUTES) && ((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
#else
	struct stat st;

	if ((path == NULL) || (path[0] == '\0'))
	{
		return 0;
	}
	if (stat(path, &st) != 0)
	{
		return 0;
	}

	return S_ISDIR(st.st_mode);
#endif
}

internal int NativeMemcard_MakeDir(const char *path)
{
	if ((path == NULL) || (path[0] == '\0'))
	{
		return 0;
	}

#if defined(_WIN32)
	if (CreateDirectoryA(path, NULL) != 0)
		return 1;

	return GetLastError() == ERROR_ALREADY_EXISTS;
#else
	if (mkdir(path, 0777) == 0)
	{
		return 1;
	}

	return errno == EEXIST;
#endif
}

internal int NativeMemcard_EnsureSlotRoot(const char *root, int slot)
{
	char slotRoot[NATIVE_MEMCARD_MAX_PATH];

	if (!NativeMemcard_MakeDir(root))
	{
		return 0;
	}
	if (!NativeMemcard_BuildSlotRoot(slotRoot, sizeof(slotRoot), root, slot))
	{
		return 0;
	}

	return NativeMemcard_MakeDir(slotRoot);
}

internal int NativeMemcard_BuildPathFromDeviceName(char *dst, int dst_size, const char *save_name, int ensureSlotRoot)
{
	struct NativeMemcardDeviceName device = NativeMemcard_ParseDeviceName(save_name);
	const char *root = NativeMemcard_ActiveRoot();
	char slotRoot[NATIVE_MEMCARD_MAX_PATH];

	if (ensureSlotRoot != 0)
	{
		if (!NativeMemcard_EnsureSlotRoot(root, device.slot))
		{
			return 0;
		}
	}
	if (!NativeMemcard_BuildSlotRoot(slotRoot, sizeof(slotRoot), root, device.slot))
	{
		return 0;
	}

	return NativeMemcard_JoinPath(dst, dst_size, slotRoot, device.name);
}

internal const char *NativeMemcard_PathFromDeviceName(const char *save_name, int ensureSlotRoot)
{
	if (!NativeMemcard_BuildPathFromDeviceName(s_memcardResolvedPath, sizeof(s_memcardResolvedPath), save_name, ensureSlotRoot))
	{
		s_memcardResolvedPath[0] = '\0';
	}
	return s_memcardResolvedPath;
}

internal enum NativeMemcardResult NativeMemcard_CopyFile(const char *src_path, const char *dst_path)
{
	unsigned char buffer[NATIVE_MEMCARD_COPY_BUFFER];
	FILE *src;
	FILE *dst;
	size_t read_bytes;
	enum NativeMemcardResult result = NATIVE_MEMCARD_OK;

	src = fopen(src_path, "rb");
	if (src == NULL)
	{
		return NATIVE_MEMCARD_NOT_FOUND;
	}

	dst = fopen(dst_path, "wb");
	if (dst == NULL)
	{
		fclose(src);
		return NATIVE_MEMCARD_OPEN_FAILED;
	}

	while ((read_bytes = fread(buffer, 1, sizeof(buffer), src)) != 0)
	{
		if (fwrite(buffer, 1, read_bytes, dst) != read_bytes)
		{
			result = NATIVE_MEMCARD_IO_ERROR;
			break;
		}
	}

	if (ferror(src) != 0)
	{
		result = NATIVE_MEMCARD_IO_ERROR;
	}

	fclose(dst);
	fclose(src);
	return result;
}

internal enum NativeMemcardResult NativeMemcard_CopyOneSave(const char *src_root, const char *dst_root, const char *name, int overwrite)
{
	char src_path[NATIVE_MEMCARD_MAX_PATH];
	char dst_path[NATIVE_MEMCARD_MAX_PATH];

	if (!NativeMemcard_JoinPath(src_path, sizeof(src_path), src_root, name))
	{
		return NATIVE_MEMCARD_IO_ERROR;
	}
	if (!NativeMemcard_JoinPath(dst_path, sizeof(dst_path), dst_root, name))
	{
		return NATIVE_MEMCARD_IO_ERROR;
	}
	if (NativeMemcard_PathIsDirectory(src_path))
	{
		return NATIVE_MEMCARD_OK;
	}
	if ((overwrite == 0) && NativeMemcard_PathExists(dst_path))
	{
		return NATIVE_MEMCARD_OK;
	}

	return NativeMemcard_CopyFile(src_path, dst_path);
}

internal enum NativeMemcardResult NativeMemcard_CopyCtrSavesFromDir(const char *src_root, const char *dst_root, int overwrite)
{
	enum NativeMemcardResult result;

	if (!NativeMemcard_PathExists(src_root))
	{
		return NATIVE_MEMCARD_OK;
	}
	if (!NativeMemcard_MakeDir(dst_root))
	{
		return NATIVE_MEMCARD_OPEN_FAILED;
	}

#if defined(_WIN32)
	{
		char search_path[NATIVE_MEMCARD_MAX_PATH];
		WIN32_FIND_DATAA find_data;
		HANDLE find_handle;

		if (!NativeMemcard_JoinPath(search_path, sizeof(search_path), src_root, "BASCUS-94426*"))
			return NATIVE_MEMCARD_IO_ERROR;

		find_handle = FindFirstFileA(search_path, &find_data);
		if (find_handle == INVALID_HANDLE_VALUE)
			return NATIVE_MEMCARD_OK;

		do
		{
			if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
				continue;
			if (!NativeMemcard_IsCtrSaveName(find_data.cFileName))
				continue;

			result = NativeMemcard_CopyOneSave(src_root, dst_root, find_data.cFileName, overwrite);
			if (result != NATIVE_MEMCARD_OK)
			{
				FindClose(find_handle);
				return result;
			}
		} while (FindNextFileA(find_handle, &find_data) != 0);

		FindClose(find_handle);
	}
#else
	{
		DIR *dir_handle = opendir(src_root);
		struct dirent *entry;

		if (dir_handle == NULL)
		{
			return NATIVE_MEMCARD_OK;
		}

		while ((entry = readdir(dir_handle)) != NULL)
		{
			if (!NativeMemcard_IsCtrSaveName(entry->d_name))
			{
				continue;
			}

			result = NativeMemcard_CopyOneSave(src_root, dst_root, entry->d_name, overwrite);
			if (result != NATIVE_MEMCARD_OK)
			{
				closedir(dir_handle);
				return result;
			}
		}

		closedir(dir_handle);
	}
#endif

	return NATIVE_MEMCARD_OK;
}

internal int NativeMemcard_MatchesPattern(const char *name, const char *pattern)
{
	while (*pattern != '\0')
	{
		if (*pattern == '*')
		{
			pattern++;
			if (*pattern == '\0')
			{
				return 1;
			}

			while (*name != '\0')
			{
				if (NativeMemcard_MatchesPattern(name, pattern) != 0)
				{
					return 1;
				}
				name++;
			}

			return 0;
		}

		if ((*pattern != '?') && (*pattern != *name))
		{
			return 0;
		}

		if (*name == '\0')
		{
			return 0;
		}

		name++;
		pattern++;
	}

	return *name == '\0';
}

internal void NativeMemcard_SplitPattern(const char *path, char *dir, int dir_size, char *pattern, int pattern_size)
{
	const char *slash = strrchr(path, '/');
#if defined(_WIN32)
	const char *backslash = strrchr(path, '\\');
	if ((slash == NULL) || ((backslash != NULL) && (backslash > slash)))
		slash = backslash;
#endif

	if (slash == NULL)
	{
		NativeMemcard_CopyString(dir, dir_size, ".");
		NativeMemcard_CopyString(pattern, pattern_size, path);
		return;
	}

	{
		int dir_len = (int)(slash - path);

		if (dir_len <= 0)
		{
			NativeMemcard_CopyString(dir, dir_size, ".");
		}
		else
		{
			if (dir_len >= dir_size)
			{
				dir_len = dir_size - 1;
			}

			memcpy(dir, path, dir_len);
			dir[dir_len] = '\0';
		}

		NativeMemcard_CopyString(pattern, pattern_size, slash + 1);
	}
}

internal int NativeMemcard_CompareNames(const char *left, const char *right)
{
	return strcmp(left, right) > 0;
}

internal void NativeMemcard_AddFoundName(const char *name)
{
	int i;
	int insert;

	if (s_memcardFind.count >= NATIVE_MEMCARD_MAX_FOUND_FILES)
	{
		return;
	}

	insert = s_memcardFind.count;
	for (i = 0; i < s_memcardFind.count; i++)
	{
		if (NativeMemcard_CompareNames(s_memcardFind.names[i], name) != 0)
		{
			insert = i;
			break;
		}
	}

	for (i = s_memcardFind.count; i > insert; i--)
	{
		memcpy(s_memcardFind.names[i], s_memcardFind.names[i - 1], sizeof(s_memcardFind.names[i]));
	}

	NativeMemcard_CopyString(s_memcardFind.names[insert], sizeof(s_memcardFind.names[insert]), name);
	s_memcardFind.count++;
}

internal void NativeMemcard_BuildFindList(const char *pattern)
{
	const char *path = NativeMemcard_PathFromDeviceName(pattern, 0);
	char dir[NATIVE_MEMCARD_MAX_PATH];
	char file_pattern[NATIVE_MEMCARD_MAX_NAME];

	memset(&s_memcardFind, 0, sizeof(s_memcardFind));
	NativeMemcard_SplitPattern(path, dir, sizeof(dir), file_pattern, sizeof(file_pattern));

#if defined(_WIN32)
	{
		char search_path[NATIVE_MEMCARD_MAX_PATH];
		WIN32_FIND_DATAA find_data;
		HANDLE find_handle;

		if (strcmp(dir, ".") == 0)
			NativeMemcard_CopyString(search_path, sizeof(search_path), file_pattern);
		else
			snprintf(search_path, sizeof(search_path), "%s\\%s", dir, file_pattern);

		find_handle = FindFirstFileA(search_path, &find_data);
		if (find_handle == INVALID_HANDLE_VALUE)
			return;

		do
		{
			if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
				NativeMemcard_AddFoundName(find_data.cFileName);
		} while (FindNextFileA(find_handle, &find_data) != 0);

		FindClose(find_handle);
	}
#else
	{
		DIR *dir_handle = opendir(dir);
		struct dirent *entry;

		if (dir_handle == NULL)
		{
			return;
		}

		while ((entry = readdir(dir_handle)) != NULL)
		{
			if (NativeMemcard_MatchesPattern(entry->d_name, file_pattern) != 0)
			{
				NativeMemcard_AddFoundName(entry->d_name);
			}
		}

		closedir(dir_handle);
	}
#endif
}

enum NativeMemcardResult NativeMemcard_SetRoot(const char *root_path)
{
	if ((root_path == NULL) || (root_path[0] == '\0') || (strcmp(root_path, ".") == 0))
	{
		NativeMemcard_ClearRoot();
		return NATIVE_MEMCARD_OK;
	}

	if (!NativePath_NormalizeSlashes(s_memcardRoot, sizeof(s_memcardRoot), NativeStr8_FromCString(root_path)))
	{
		return NATIVE_MEMCARD_IO_ERROR;
	}

	memset(&s_memcardFind, 0, sizeof(s_memcardFind));
	return NATIVE_MEMCARD_OK;
}

void NativeMemcard_ClearRoot(void)
{
	s_memcardRoot[0] = '\0';
	s_memcardResolvedPath[0] = '\0';
	memset(&s_memcardFind, 0, sizeof(s_memcardFind));
}

enum NativeMemcardResult NativeMemcard_RemoveRoot(const char *root_path)
{
	if ((root_path == NULL) || (root_path[0] == '\0') || (strcmp(root_path, ".") == 0))
	{
		return NATIVE_MEMCARD_IO_ERROR;
	}
	if (strcmp(root_path, NATIVE_MEMCARD_DEFAULT_ROOT) == 0)
	{
		return NATIVE_MEMCARD_IO_ERROR;
	}

	if (!NativeMemcard_PathExists(root_path))
	{
		return NATIVE_MEMCARD_OK;
	}

#if defined(_WIN32)
	{
		char search_path[NATIVE_MEMCARD_MAX_PATH];
		WIN32_FIND_DATAA find_data;
		HANDLE find_handle;

		if (!NativeMemcard_JoinPath(search_path, sizeof(search_path), root_path, "*"))
			return NATIVE_MEMCARD_IO_ERROR;

		find_handle = FindFirstFileA(search_path, &find_data);
		if (find_handle != INVALID_HANDLE_VALUE)
		{
			do
			{
				char file_path[NATIVE_MEMCARD_MAX_PATH];

				if ((strcmp(find_data.cFileName, ".") == 0) || (strcmp(find_data.cFileName, "..") == 0))
					continue;
				if (!NativeMemcard_JoinPath(file_path, sizeof(file_path), root_path, find_data.cFileName))
				{
					FindClose(find_handle);
					return NATIVE_MEMCARD_IO_ERROR;
				}
				if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
				{
					if (NativeMemcard_RemoveRoot(file_path) != NATIVE_MEMCARD_OK)
					{
						FindClose(find_handle);
						return NATIVE_MEMCARD_IO_ERROR;
					}
				}
				else if (DeleteFileA(file_path) == 0)
				{
					FindClose(find_handle);
					return NATIVE_MEMCARD_IO_ERROR;
				}
			} while (FindNextFileA(find_handle, &find_data) != 0);

			FindClose(find_handle);
		}

		if (RemoveDirectoryA(root_path) == 0)
			return NATIVE_MEMCARD_IO_ERROR;
	}
#else
	{
		DIR *dir_handle = opendir(root_path);
		struct dirent *entry;

		if (dir_handle != NULL)
		{
			while ((entry = readdir(dir_handle)) != NULL)
			{
				char file_path[NATIVE_MEMCARD_MAX_PATH];

				if ((strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0))
				{
					continue;
				}
				if (!NativeMemcard_JoinPath(file_path, sizeof(file_path), root_path, entry->d_name))
				{
					closedir(dir_handle);
					return NATIVE_MEMCARD_IO_ERROR;
				}
				if (NativeMemcard_PathIsDirectory(file_path))
				{
					if (NativeMemcard_RemoveRoot(file_path) != NATIVE_MEMCARD_OK)
					{
						closedir(dir_handle);
						return NATIVE_MEMCARD_IO_ERROR;
					}
				}
				else if (unlink(file_path) != 0)
				{
					closedir(dir_handle);
					return NATIVE_MEMCARD_IO_ERROR;
				}
			}

			closedir(dir_handle);
		}

		if (rmdir(root_path) != 0)
		{
			return NATIVE_MEMCARD_IO_ERROR;
		}
	}
#endif

	return NATIVE_MEMCARD_OK;
}

enum NativeMemcardResult NativeMemcard_CloneRoot(const char *src_root, const char *dst_root)
{
	enum NativeMemcardResult result;
	int slot;

	if ((dst_root == NULL) || (dst_root[0] == '\0') || (strcmp(dst_root, ".") == 0))
	{
		return NATIVE_MEMCARD_IO_ERROR;
	}

	if ((src_root == NULL) || (src_root[0] == '\0'))
	{
		src_root = NATIVE_MEMCARD_DEFAULT_ROOT;
	}
	if (strcmp(src_root, dst_root) == 0)
	{
		return NATIVE_MEMCARD_IO_ERROR;
	}

	result = NativeMemcard_RemoveRoot(dst_root);
	if (result != NATIVE_MEMCARD_OK)
	{
		return result;
	}

	if (!NativeMemcard_MakeDir(dst_root))
	{
		return NATIVE_MEMCARD_OPEN_FAILED;
	}

	if (!NativeMemcard_PathExists(src_root))
	{
		return NATIVE_MEMCARD_OK;
	}

	for (slot = 0; slot < NATIVE_MEMCARD_SLOT_COUNT; slot++)
	{
		char srcSlotRoot[NATIVE_MEMCARD_MAX_PATH];
		char dstSlotRoot[NATIVE_MEMCARD_MAX_PATH];

		if (!NativeMemcard_BuildSlotRoot(srcSlotRoot, sizeof(srcSlotRoot), src_root, slot))
		{
			return NATIVE_MEMCARD_IO_ERROR;
		}
		if (!NativeMemcard_BuildSlotRoot(dstSlotRoot, sizeof(dstSlotRoot), dst_root, slot))
		{
			return NATIVE_MEMCARD_IO_ERROR;
		}
		if (!NativeMemcard_PathExists(srcSlotRoot))
		{
			continue;
		}

		result = NativeMemcard_CopyCtrSavesFromDir(srcSlotRoot, dstSlotRoot, 1);
		if (result != NATIVE_MEMCARD_OK)
		{
			return result;
		}
	}

	return NATIVE_MEMCARD_OK;
}

enum NativeMemcardResult NativeMemcard_CloneCurrentRoot(const char *dst_root)
{
	return NativeMemcard_CloneRoot(NativeMemcard_ActiveRoot(), dst_root);
}

int NativeMemcard_FindFirstFile(const char *pattern, char *dst_name, int dst_size)
{
	NativeMemcard_BuildFindList(pattern);
	s_memcardFind.index = 0;
	return NativeMemcard_FindNextFile(dst_name, dst_size);
}

int NativeMemcard_FindNextFile(char *dst_name, int dst_size)
{
	if (s_memcardFind.index >= s_memcardFind.count)
	{
		return 0;
	}

	NativeMemcard_CopyString(dst_name, dst_size, s_memcardFind.names[s_memcardFind.index]);
	s_memcardFind.index++;
	return 1;
}

int NativeMemcard_FileExists(const char *save_name)
{
	const char *path = NativeMemcard_PathFromDeviceName(save_name, 0);
	FILE *file = fopen(path, "rb");

	if (file == NULL)
	{
		return 0;
	}

	fclose(file);
	return 1;
}

enum NativeMemcardResult NativeMemcard_RemoveFile(const char *save_name)
{
	const char *path = NativeMemcard_PathFromDeviceName(save_name, 0);

	return remove(path) == 0 ? NATIVE_MEMCARD_OK : NATIVE_MEMCARD_NOT_FOUND;
}

enum NativeMemcardResult NativeMemcard_ReadSaveData(const char *save_name, unsigned char *dst, int byte_count, int data_offset)
{
	const char *path = NativeMemcard_PathFromDeviceName(save_name, 0);
	FILE *file;
	size_t read_bytes;

	file = fopen(path, "rb");
	if (file == NULL)
	{
		return NATIVE_MEMCARD_NOT_FOUND;
	}

	if (fseek(file, data_offset, SEEK_SET) != 0)
	{
		fclose(file);
		return NATIVE_MEMCARD_IO_ERROR;
	}

	read_bytes = fread(dst, 1, byte_count, file);
	fclose(file);

	return read_bytes == (size_t)byte_count ? NATIVE_MEMCARD_OK : NATIVE_MEMCARD_IO_ERROR;
}

enum NativeMemcardResult NativeMemcard_WriteSaveData(const char *save_name, const void *icon, int icon_byte_count, const unsigned char *src, int byte_count)
{
	const char *path = NativeMemcard_PathFromDeviceName(save_name, 1);
	FILE *file;
	size_t wrote_icon;
	size_t wrote_data;

	file = fopen(path, "wb");
	if (file == NULL)
	{
		return NATIVE_MEMCARD_OPEN_FAILED;
	}

	wrote_icon = fwrite(icon, 1, icon_byte_count, file);
	wrote_data = fwrite(src, 1, byte_count, file);
	fclose(file);

	if ((wrote_icon != (size_t)icon_byte_count) || (wrote_data != (size_t)byte_count))
	{
		return NATIVE_MEMCARD_IO_ERROR;
	}

	return NATIVE_MEMCARD_OK;
}
