#include <platform/native_memcard.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <dirent.h>
#endif
#include <stdio.h>
#include <string.h>

#define NATIVE_MEMCARD_MAX_FOUND_FILES 64
#define NATIVE_MEMCARD_MAX_NAME        64
#define NATIVE_MEMCARD_MAX_PATH        512

struct NativeMemcardFindState
{
	char names[NATIVE_MEMCARD_MAX_FOUND_FILES][NATIVE_MEMCARD_MAX_NAME];
	int count;
	int index;
};

static struct NativeMemcardFindState s_memcardFind;

static const char *NativeMemcard_PathFromDeviceName(const char *save_name)
{
	if (strncmp(save_name, "bu00:", 5) == 0)
		return save_name + 5;

	if (strncmp(save_name, "bu01:", 5) == 0)
		return save_name + 5;

	return save_name;
}

static int NativeMemcard_CopyString(char *dst, int dst_size, const char *src)
{
	int i;

	if (dst_size <= 0)
		return 0;

	for (i = 0; (i < dst_size - 1) && (src[i] != '\0'); i++)
		dst[i] = src[i];

	dst[i] = '\0';
	return src[i] == '\0';
}

static int NativeMemcard_MatchesPattern(const char *name, const char *pattern)
{
	while (*pattern != '\0')
	{
		if (*pattern == '*')
		{
			pattern++;
			if (*pattern == '\0')
				return 1;

			while (*name != '\0')
			{
				if (NativeMemcard_MatchesPattern(name, pattern) != 0)
					return 1;
				name++;
			}

			return 0;
		}

		if ((*pattern != '?') && (*pattern != *name))
			return 0;

		if (*name == '\0')
			return 0;

		name++;
		pattern++;
	}

	return *name == '\0';
}

static void NativeMemcard_SplitPattern(const char *path, char *dir, int dir_size, char *pattern, int pattern_size)
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
			NativeMemcard_CopyString(dir, dir_size, ".");
		else
		{
			if (dir_len >= dir_size)
				dir_len = dir_size - 1;

			memcpy(dir, path, dir_len);
			dir[dir_len] = '\0';
		}

		NativeMemcard_CopyString(pattern, pattern_size, slash + 1);
	}
}

static int NativeMemcard_CompareNames(const char *left, const char *right)
{
	return strcmp(left, right) > 0;
}

static void NativeMemcard_AddFoundName(const char *name)
{
	int i;
	int insert;

	if (s_memcardFind.count >= NATIVE_MEMCARD_MAX_FOUND_FILES)
		return;

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
		memcpy(s_memcardFind.names[i], s_memcardFind.names[i - 1], sizeof(s_memcardFind.names[i]));

	NativeMemcard_CopyString(s_memcardFind.names[insert], sizeof(s_memcardFind.names[insert]), name);
	s_memcardFind.count++;
}

static void NativeMemcard_BuildFindList(const char *pattern)
{
	const char *path = NativeMemcard_PathFromDeviceName(pattern);
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
			return;

		while ((entry = readdir(dir_handle)) != NULL)
		{
			if (NativeMemcard_MatchesPattern(entry->d_name, file_pattern) != 0)
				NativeMemcard_AddFoundName(entry->d_name);
		}

		closedir(dir_handle);
	}
#endif
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
		return 0;

	NativeMemcard_CopyString(dst_name, dst_size, s_memcardFind.names[s_memcardFind.index]);
	s_memcardFind.index++;
	return 1;
}

int NativeMemcard_FileExists(const char *save_name)
{
	const char *path = NativeMemcard_PathFromDeviceName(save_name);
	FILE *file = fopen(path, "rb");

	if (file == NULL)
		return 0;

	fclose(file);
	return 1;
}

enum NativeMemcardResult NativeMemcard_RemoveFile(const char *save_name)
{
	const char *path = NativeMemcard_PathFromDeviceName(save_name);

	return remove(path) == 0 ? NATIVE_MEMCARD_OK : NATIVE_MEMCARD_NOT_FOUND;
}

enum NativeMemcardResult NativeMemcard_ReadSaveData(const char *save_name, unsigned char *dst, int byte_count, int data_offset)
{
	const char *path = NativeMemcard_PathFromDeviceName(save_name);
	FILE *file;
	size_t read_bytes;

	file = fopen(path, "rb");
	if (file == NULL)
		return NATIVE_MEMCARD_NOT_FOUND;

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
	const char *path = NativeMemcard_PathFromDeviceName(save_name);
	FILE *file;
	size_t wrote_icon;
	size_t wrote_data;

	file = fopen(path, "wb");
	if (file == NULL)
		return NATIVE_MEMCARD_OPEN_FAILED;

	wrote_icon = fwrite(icon, 1, icon_byte_count, file);
	wrote_data = fwrite(src, 1, byte_count, file);
	fclose(file);

	if ((wrote_icon != (size_t)icon_byte_count) || (wrote_data != (size_t)byte_count))
		return NATIVE_MEMCARD_IO_ERROR;

	return NATIVE_MEMCARD_OK;
}
