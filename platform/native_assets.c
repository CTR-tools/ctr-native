#include "platform/native_assets.h"

#include <macros.h>

#include <platform/native_disc_image.h>
#include <platform/native_path.h>

#if defined(_WIN32)
#include <platform/native_win32.h>
#else
#include <dirent.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NATIVE_ASSETS_PATH_MAX                   1024
#define NATIVE_ASSETS_DIR_NAME                   "assets"
#define NATIVE_ASSETS_BIGFILE_PATH               "BIGFILE.BIG"
#define NATIVE_ASSETS_KART_HWL_PATH              "SOUNDS/KART.HWL"
#define NATIVE_ASSETS_TEST_STR_PATH              "TEST.STR"
#define NATIVE_ASSETS_XNF_PATH                   "XA/ENG.XNF"
#define NATIVE_ASSETS_DISC_PATH                  "ctr-u.bin"

#define NATIVE_ASSETS_XA_TYPE_COUNT              3
#define NATIVE_ASSETS_XA_HEADER_SIZE             0x44
#define NATIVE_ASSETS_XA_NUM_XAS_TOTAL_OFFSET    0x0c
#define NATIVE_ASSETS_XA_NUM_TRACKS_TOTAL_OFFSET 0x10
#define NATIVE_ASSETS_XA_NUM_SONGS_OFFSET        0x2c
#define NATIVE_ASSETS_XA_FIRST_SONG_INDEX_OFFSET 0x38
#define NATIVE_ASSETS_XA_ENTRY_BYTES             4
#define NATIVE_ASSETS_XA_MAX_FILE_NUMBER         256

struct NativeAssetIndexEntry
{
	char *key;
	char *path;
};

global_variable char s_nativeAssetsBaseDir[NATIVE_ASSETS_PATH_MAX] = ".";
global_variable char s_nativeAssetsDir[NATIVE_ASSETS_PATH_MAX] = NATIVE_ASSETS_DIR_NAME;
global_variable int s_nativeAssetsInitialized;
global_variable struct NativeAssetIndexEntry *s_nativeAssetIndex;
global_variable int s_nativeAssetIndexCount;
global_variable int s_nativeAssetIndexCapacity;
global_variable int s_nativeAssetIndexBuilt;

internal int NativeAssets_FileExistsHost(const char *path)
{
	if (path == NULL)
	{
		return 0;
	}

	FILE *file = fopen(path, "rb");
	if (file == NULL)
	{
		return 0;
	}

	fclose(file);
	return 1;
}

internal int NativeAssets_DirectoryExistsHost(const char *path)
{
#if defined(_WIN32)
	DWORD attributes;

	if (path == NULL)
		return 0;

	attributes = GetFileAttributesA(path);
	return (attributes != INVALID_FILE_ATTRIBUTES) && ((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
#else
	DIR *dir = opendir(path);

	if (dir == NULL)
	{
		return 0;
	}

	closedir(dir);
	return 1;
#endif
}

internal int NativeAssets_FindHostChildCaseInsensitive(NativeStr8 parent, NativeStr8 child, char *dst, size_t dstSize)
{
	char parentPath[NATIVE_ASSETS_PATH_MAX];
	char match[NATIVE_ASSETS_PATH_MAX];

	if (!NativePath_NormalizeSlashes(parentPath, sizeof(parentPath), parent))
	{
		return 0;
	}

#if defined(_WIN32)
	{
		char searchPath[NATIVE_ASSETS_PATH_MAX];
		WIN32_FIND_DATAA findData;
		HANDLE findHandle;

		if (!NativePath_Join(searchPath, sizeof(searchPath), NativeStr8_FromCString(parentPath), NATIVE_STR8_LIT("*")))
			return 0;

		findHandle = FindFirstFileA(searchPath, &findData);
		if (findHandle == INVALID_HANDLE_VALUE)
			return 0;

		match[0] = '\0';
		do
		{
			NativeStr8 entryName = NativeStr8_FromCString(findData.cFileName);

			if (NativeStr8_Equals(entryName, child))
			{
				if (!NativeStr8_CopyToCString(match, sizeof(match), entryName))
				{
					FindClose(findHandle);
					return 0;
				}

				break;
			}

			if ((match[0] == '\0') && NativeStr8_EqualsIgnoreCaseAscii(entryName, child))
			{
				if (!NativeStr8_CopyToCString(match, sizeof(match), entryName))
				{
					FindClose(findHandle);
					return 0;
				}
			}
		} while (FindNextFileA(findHandle, &findData) != 0);

		FindClose(findHandle);
	}
#else
	struct dirent *entry;

	DIR *dir = opendir(parentPath);
	if (dir == NULL)
	{
		return 0;
	}

	match[0] = '\0';
	while ((entry = readdir(dir)) != NULL)
	{
		NativeStr8 entryName = NativeStr8_FromCString(entry->d_name);

		if (NativeStr8_Equals(entryName, child))
		{
			if (!NativeStr8_CopyToCString(match, sizeof(match), entryName))
			{
				closedir(dir);
				return 0;
			}

			break;
		}

		if ((match[0] == '\0') && NativeStr8_EqualsIgnoreCaseAscii(entryName, child))
		{
			if (!NativeStr8_CopyToCString(match, sizeof(match), entryName))
			{
				closedir(dir);
				return 0;
			}
		}
	}

	closedir(dir);
#endif

	if (match[0] == '\0')
	{
		return 0;
	}

	return NativePath_Join(dst, dstSize, NativeStr8_FromCString(parentPath), NativeStr8_FromCString(match));
}

internal int NativeAssets_FindAssetsDir(NativeStr8 baseDir, char *dst, size_t dstSize)
{
	if (!NativePath_Join(dst, dstSize, baseDir, NATIVE_STR8_LIT(NATIVE_ASSETS_DIR_NAME)))
	{
		return 0;
	}

	if (NativeAssets_DirectoryExistsHost(dst))
	{
		return 1;
	}

	return NativeAssets_FindHostChildCaseInsensitive(baseDir, NATIVE_STR8_LIT(NATIVE_ASSETS_DIR_NAME), dst, dstSize);
}

internal char *NativeAssets_CopyCString(const char *src)
{
	if (src == NULL)
	{
		return NULL;
	}

	size_t size = strlen(src) + 1u;
	char *copy = (char *)malloc(size);
	if (copy == NULL)
	{
		return NULL;
	}

	memcpy(copy, src, size);
	return copy;
}

internal void NativeAssets_ClearIndex(void)
{
	for (int i = 0; i < s_nativeAssetIndexCount; i++)
	{
		free(s_nativeAssetIndex[i].key);
		free(s_nativeAssetIndex[i].path);
	}

	free(s_nativeAssetIndex);
	s_nativeAssetIndex = NULL;
	s_nativeAssetIndexCount = 0;
	s_nativeAssetIndexCapacity = 0;
	s_nativeAssetIndexBuilt = 0;
}

internal int NativeAssets_IndexReserve(int needed)
{
	if (needed <= s_nativeAssetIndexCapacity)
	{
		return 1;
	}

	int newCapacity = (s_nativeAssetIndexCapacity == 0) ? 256 : s_nativeAssetIndexCapacity * 2;
	while (newCapacity < needed)
	{
		newCapacity *= 2;
	}

	struct NativeAssetIndexEntry *entries = (struct NativeAssetIndexEntry *)realloc(s_nativeAssetIndex, (size_t)newCapacity * sizeof(*s_nativeAssetIndex));
	if (entries == NULL)
	{
		return 0;
	}

	s_nativeAssetIndex = entries;
	s_nativeAssetIndexCapacity = newCapacity;
	return 1;
}

internal int NativeAssets_NormalizeRelativeKey(NativeStr8 relativePath, char *dst, size_t dstSize)
{
	relativePath = NativePath_SkipLeadingSeparators(relativePath);
	if ((dst == NULL) || (dstSize == 0) || (relativePath.len >= dstSize))
	{
		return 0;
	}

	for (size_t i = 0; i < relativePath.len; i++)
	{
		u8 byte = relativePath.ptr[i];
		dst[i] = (char)(NativePath_IsSeparator(byte) ? '/' : NativeStr8_ToLowerAscii(byte));
	}

	dst[relativePath.len] = '\0';
	return 1;
}

internal const char *NativeAssets_FindIndexedPath(NativeStr8 relativePath)
{
	char key[NATIVE_ASSETS_PATH_MAX];

	if (!NativeAssets_NormalizeRelativeKey(relativePath, key, sizeof(key)))
	{
		return NULL;
	}

	for (int i = 0; i < s_nativeAssetIndexCount; i++)
	{
		if (strcmp(s_nativeAssetIndex[i].key, key) == 0)
		{
			return s_nativeAssetIndex[i].path;
		}
	}

	return NULL;
}

internal int NativeAssets_IndexAdd(NativeStr8 relativePath, const char *hostPath)
{
	char key[NATIVE_ASSETS_PATH_MAX];

	if (!NativeAssets_NormalizeRelativeKey(relativePath, key, sizeof(key)))
	{
		return 0;
	}

	if (!NativeAssets_IndexReserve(s_nativeAssetIndexCount + 1))
	{
		return 0;
	}

	char *keyCopy = NativeAssets_CopyCString(key);
	char *pathCopy = NativeAssets_CopyCString(hostPath);
	if ((keyCopy == NULL) || (pathCopy == NULL))
	{
		free(keyCopy);
		free(pathCopy);
		return 0;
	}

	s_nativeAssetIndex[s_nativeAssetIndexCount].key = keyCopy;
	s_nativeAssetIndex[s_nativeAssetIndexCount].path = pathCopy;
	s_nativeAssetIndexCount++;
	return 1;
}

internal void NativeAssets_IndexScanDir(const char *dirPath, NativeStr8 relativeDir, int depth)
{
	if (depth > 16)
	{
		return;
	}

#if defined(_WIN32)
	{
		char searchPath[NATIVE_ASSETS_PATH_MAX];
		WIN32_FIND_DATAA findData;
		HANDLE findHandle;

		if (!NativePath_Join(searchPath, sizeof(searchPath), NativeStr8_FromCString(dirPath), NATIVE_STR8_LIT("*")))
			return;

		findHandle = FindFirstFileA(searchPath, &findData);
		if (findHandle == INVALID_HANDLE_VALUE)
			return;

		do
		{
			NativeStr8 entryName = NativeStr8_FromCString(findData.cFileName);
			char childPath[NATIVE_ASSETS_PATH_MAX];
			char relativePath[NATIVE_ASSETS_PATH_MAX];

			if (NativeStr8_Equals(entryName, NATIVE_STR8_LIT(".")) || NativeStr8_Equals(entryName, NATIVE_STR8_LIT("..")))
				continue;

			if (!NativePath_Join(childPath, sizeof(childPath), NativeStr8_FromCString(dirPath), entryName))
				continue;

			if (relativeDir.len == 0)
			{
				if (!NativePath_NormalizeSlashes(relativePath, sizeof(relativePath), entryName))
					continue;
			}
			else
			{
				if (!NativePath_Join(relativePath, sizeof(relativePath), relativeDir, entryName))
					continue;
			}

			if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
			{
				NativeAssets_IndexScanDir(childPath, NativeStr8_FromCString(relativePath), depth + 1);
				continue;
			}

			if (NativeAssets_FileExistsHost(childPath))
				NativeAssets_IndexAdd(NativeStr8_FromCString(relativePath), childPath);
		} while (FindNextFileA(findHandle, &findData) != 0);

		FindClose(findHandle);
	}
#else
	struct dirent *entry;

	DIR *dir = opendir(dirPath);
	if (dir == NULL)
	{
		return;
	}

	while ((entry = readdir(dir)) != NULL)
	{
		NativeStr8 entryName = NativeStr8_FromCString(entry->d_name);
		char childPath[NATIVE_ASSETS_PATH_MAX];
		char relativePath[NATIVE_ASSETS_PATH_MAX];

		if (NativeStr8_Equals(entryName, NATIVE_STR8_LIT(".")) || NativeStr8_Equals(entryName, NATIVE_STR8_LIT("..")))
		{
			continue;
		}

		if (!NativePath_Join(childPath, sizeof(childPath), NativeStr8_FromCString(dirPath), entryName))
		{
			continue;
		}

		if (relativeDir.len == 0)
		{
			if (!NativePath_NormalizeSlashes(relativePath, sizeof(relativePath), entryName))
			{
				continue;
			}
		}
		else
		{
			if (!NativePath_Join(relativePath, sizeof(relativePath), relativeDir, entryName))
			{
				continue;
			}
		}

		DIR *childDir = opendir(childPath);
		if (childDir != NULL)
		{
			closedir(childDir);
			NativeAssets_IndexScanDir(childPath, NativeStr8_FromCString(relativePath), depth + 1);
			continue;
		}

		if (NativeAssets_FileExistsHost(childPath))
		{
			NativeAssets_IndexAdd(NativeStr8_FromCString(relativePath), childPath);
		}
	}

	closedir(dir);
#endif
}

internal void NativeAssets_BuildIndex(void)
{
	if (s_nativeAssetIndexBuilt)
	{
		return;
	}

	s_nativeAssetIndexBuilt = 1;

	NativeAssets_IndexScanDir(s_nativeAssetsDir, (NativeStr8){0}, 0);
}

internal int NativeAssets_BaseHasRequiredFile(NativeStr8 baseDir)
{
	char assetsDir[NATIVE_ASSETS_PATH_MAX];
	char path[NATIVE_ASSETS_PATH_MAX];

	if (!NativeAssets_FindAssetsDir(baseDir, assetsDir, sizeof(assetsDir)))
	{
		return 0;
	}

	if (NativeAssets_FindHostChildCaseInsensitive(NativeStr8_FromCString(assetsDir), NATIVE_STR8_LIT(NATIVE_ASSETS_BIGFILE_PATH), path, sizeof(path)))
	{
		return NativeAssets_FileExistsHost(path);
	}

	if (!NativePath_Join(path, sizeof(path), NativeStr8_FromCString(assetsDir), NATIVE_STR8_LIT(NATIVE_ASSETS_BIGFILE_PATH)))
	{
		return 0;
	}

	if (NativeAssets_FileExistsHost(path))
	{
		return 1;
	}

	if (NativeAssets_FindHostChildCaseInsensitive(NativeStr8_FromCString(assetsDir), NATIVE_STR8_LIT(NATIVE_ASSETS_DISC_PATH), path, sizeof(path)))
	{
		return NativeAssets_FileExistsHost(path);
	}

	if (!NativePath_Join(path, sizeof(path), NativeStr8_FromCString(assetsDir), NATIVE_STR8_LIT(NATIVE_ASSETS_DISC_PATH)))
	{
		return 0;
	}

	return NativeAssets_FileExistsHost(path);
}

internal int NativeAssets_SetBaseDir(NativeStr8 baseDir)
{
	char assetsDir[NATIVE_ASSETS_PATH_MAX];

	baseDir = NativePath_TrimTrailingSeparators(baseDir);
	if (!NativePath_NormalizeSlashes(s_nativeAssetsBaseDir, sizeof(s_nativeAssetsBaseDir), baseDir))
	{
		return 0;
	}

	if (!NativeAssets_FindAssetsDir(NativeStr8_FromCString(s_nativeAssetsBaseDir), assetsDir, sizeof(assetsDir)))
	{
		if (!NativePath_Join(assetsDir, sizeof(assetsDir), NativeStr8_FromCString(s_nativeAssetsBaseDir), NATIVE_STR8_LIT(NATIVE_ASSETS_DIR_NAME)))
		{
			return 0;
		}
	}

	if (!NativePath_NormalizeSlashes(s_nativeAssetsDir, sizeof(s_nativeAssetsDir), NativeStr8_FromCString(assetsDir)))
	{
		return 0;
	}

	NativeDiscImage_Init(s_nativeAssetsDir);
	NativeAssets_ClearIndex();
	s_nativeAssetsInitialized = 1;
	return 1;
}

int NativeAssets_Init(const char *executableBasePath)
{
	char parentDir[NATIVE_ASSETS_PATH_MAX];
	char grandparentDir[NATIVE_ASSETS_PATH_MAX];

	if ((executableBasePath == NULL) || (executableBasePath[0] == '\0'))
	{
		executableBasePath = ".";
	}

	NativeStr8 exeDir = NativePath_TrimTrailingSeparators(NativeStr8_FromCString(executableBasePath));

	if (NativeAssets_BaseHasRequiredFile(exeDir))
	{
		return NativeAssets_SetBaseDir(exeDir);
	}

	if (NativePath_Parent(parentDir, sizeof(parentDir), exeDir))
	{
		NativeStr8 parent = NativeStr8_FromCString(parentDir);

		if (NativeAssets_BaseHasRequiredFile(parent))
		{
			return NativeAssets_SetBaseDir(parent);
		}

		if (NativePath_Parent(grandparentDir, sizeof(grandparentDir), parent))
		{
			NativeStr8 grandparent = NativeStr8_FromCString(grandparentDir);

			if (NativeAssets_BaseHasRequiredFile(grandparent))
			{
				return NativeAssets_SetBaseDir(grandparent);
			}
		}
	}

	return NativeAssets_SetBaseDir(exeDir);
}

const char *NativeAssets_GetBaseDir(void)
{
	return s_nativeAssetsBaseDir;
}

const char *NativeAssets_GetAssetDir(void)
{
	return s_nativeAssetsDir;
}

int NativeAssets_BuildPathStr8(NativeStr8 relativePath, char *dst, size_t dstSize)
{
	if (!s_nativeAssetsInitialized && !NativeAssets_Init("."))
	{
		return 0;
	}

	return NativePath_Join(dst, dstSize, NativeStr8_FromCString(s_nativeAssetsDir), relativePath);
}

int NativeAssets_BuildPath(const char *relativePath, char *dst, size_t dstSize)
{
	return NativeAssets_BuildPathStr8(NativeStr8_FromCString(relativePath), dst, dstSize);
}

int NativeAssets_ResolvePathStr8(NativeStr8 relativePath, char *dst, size_t dstSize)
{
	char path[NATIVE_ASSETS_PATH_MAX];

	if (!NativeAssets_BuildPathStr8(relativePath, path, sizeof(path)))
	{
		return 0;
	}

	if (NativeAssets_FileExistsHost(path))
	{
		return NativePath_NormalizeSlashes(dst, dstSize, NativeStr8_FromCString(path));
	}

	NativeAssets_BuildIndex();
	const char *indexedPath = NativeAssets_FindIndexedPath(relativePath);
	if (indexedPath == NULL)
	{
		return 0;
	}

	return NativePath_NormalizeSlashes(dst, dstSize, NativeStr8_FromCString(indexedPath));
}

int NativeAssets_ResolvePath(const char *relativePath, char *dst, size_t dstSize)
{
	return NativeAssets_ResolvePathStr8(NativeStr8_FromCString(relativePath), dst, dstSize);
}

FILE *NativeAssets_OpenHostStr8(NativeStr8 relativePath, const char *mode)
{
	char path[NATIVE_ASSETS_PATH_MAX];

	if ((mode == NULL) || !NativeAssets_BuildPathStr8(relativePath, path, sizeof(path)))
	{
		return NULL;
	}

	FILE *file = fopen(path, mode);
	if (file != NULL)
	{
		return file;
	}

	if (mode[0] != 'r')
	{
		return NULL;
	}

	if (!NativeAssets_ResolvePathStr8(relativePath, path, sizeof(path)))
	{
		return NULL;
	}

	return fopen(path, mode);
}

FILE *NativeAssets_OpenHost(const char *relativePath, const char *mode)
{
	return NativeAssets_OpenHostStr8(NativeStr8_FromCString(relativePath), mode);
}

FILE *NativeAssets_OpenHostBigfile(const char *mode)
{
	return NativeAssets_OpenHostStr8(NATIVE_STR8_LIT(NATIVE_ASSETS_BIGFILE_PATH), mode);
}

internal int NativeAssets_ReadExact(FILE *file, void *dst, size_t size)
{
	return fread(dst, 1, size, file) == size;
}

internal u32 NativeAssets_ReadLE32(const u8 *data)
{
	return ((u32)data[0]) | ((u32)data[1] << 8) | ((u32)data[2] << 16) | ((u32)data[3] << 24);
}

internal int NativeAssets_ReadHostBytes(const char *path, struct NativeAssetsByteBuffer *bytes)
{
	bytes->data = NULL;
	bytes->size = 0;

	FILE *file = NativeAssets_OpenHost(path, "rb");
	if (file == NULL)
	{
		return 0;
	}

	if (fseek(file, 0, SEEK_END) != 0)
	{
		fclose(file);
		return 0;
	}

	long size = ftell(file);
	if ((size <= 0) || (size > 0x7fffffff))
	{
		fclose(file);
		return 0;
	}

	if (fseek(file, 0, SEEK_SET) != 0)
	{
		fclose(file);
		return 0;
	}

	bytes->data = (u8 *)malloc((size_t)size);
	if (bytes->data == NULL)
	{
		fclose(file);
		return 0;
	}

	if (!NativeAssets_ReadExact(file, bytes->data, (size_t)size))
	{
		free(bytes->data);
		bytes->data = NULL;
		fclose(file);
		return 0;
	}

	fclose(file);
	bytes->size = (int)size;
	return 1;
}

int NativeAssets_ReadBytes(const char *path, int readMode, struct NativeAssetsByteBuffer *bytes)
{
	bytes->data = NULL;
	bytes->size = 0;

	if (NativeAssets_ReadHostBytes(path, bytes))
	{
		return 1;
	}

	return NativeDiscImage_ReadFileBytes(path, readMode == NATIVE_ASSET_READ_RAW_CD_SECTORS, &bytes->data, &bytes->size);
}

void NativeAssets_FreeBytes(struct NativeAssetsByteBuffer *bytes)
{
	free(bytes->data);
	bytes->data = NULL;
	bytes->size = 0;
}

internal void NativeAssets_PrintHeader(void)
{
	fprintf(stderr, "[CTR Native Expanded] Missing or invalid assets.\n");
	fprintf(stderr, "[CTR Native Expanded] Expected NTSC-U retail assets under: %s\n", NativeAssets_GetAssetDir());
}

internal void NativeAssets_PrintFooter(void)
{
	fprintf(stderr, "[CTR Native Expanded] Provide either raw NTSC-U disc image %s, or extracted files:\n", NATIVE_ASSETS_DISC_PATH);
	fprintf(stderr, "[CTR Native Expanded]   Required: %s, %s\n", NATIVE_ASSETS_BIGFILE_PATH, NATIVE_ASSETS_KART_HWL_PATH);
	fprintf(stderr, "[CTR Native Expanded]   For complete audio/video: %s, %s, plus XA files referenced by %s\n", NATIVE_ASSETS_TEST_STR_PATH, NATIVE_ASSETS_XNF_PATH,
	        NATIVE_ASSETS_XNF_PATH);
}

internal int NativeAssets_HasFile(const char *path)
{
	char assetPath[NATIVE_ASSETS_PATH_MAX];
	struct NativeDiscImageFile discFile;

	return NativeAssets_ResolvePath(path, assetPath, sizeof(assetPath)) || NativeDiscImage_FindFile(path, &discFile);
}

internal int NativeAssets_CheckRequiredFile(const char *path)
{
	char assetPath[NATIVE_ASSETS_PATH_MAX];

	if (!NativeAssets_BuildPath(path, assetPath, sizeof(assetPath)))
	{
		fprintf(stderr, "[CTR Native Expanded] asset path too long: %s\n", path);
		return 0;
	}

	if (NativeAssets_HasFile(path))
	{
		return 1;
	}

	fprintf(stderr, "[CTR Native Expanded] missing asset: %s\n", assetPath);
	return 0;
}

internal int NativeAssets_ValidateXA(u32 *missing)
{
	local_persist const char *xaDirs[NATIVE_ASSETS_XA_TYPE_COUNT] = {
	    "XA/MUSIC",
	    "XA/ENG/EXTRA",
	    "XA/ENG/GAME",
	};
	struct NativeAssetsByteBuffer xnf;
	char manifestPath[NATIVE_ASSETS_PATH_MAX];
	u8 required[NATIVE_ASSETS_XA_TYPE_COUNT][NATIVE_ASSETS_XA_MAX_FILE_NUMBER];
	u32 categoryID;

	memset(required, 0, sizeof(required));

	// Do not hide an empty or unreadable extracted override behind disc data.
	int loaded = NativeAssets_ResolvePath(NATIVE_ASSETS_XNF_PATH, manifestPath, sizeof(manifestPath))
	                 ? NativeAssets_ReadHostBytes(NATIVE_ASSETS_XNF_PATH, &xnf)
	                 : NativeAssets_ReadBytes(NATIVE_ASSETS_XNF_PATH, NATIVE_ASSET_READ_DATA_FILE, &xnf);
	if (!loaded)
	{
		fprintf(stderr, "[CTR Native Expanded] unable to read XA manifest: %s\n", NATIVE_ASSETS_XNF_PATH);
		return 0;
	}

	if ((xnf.size < NATIVE_ASSETS_XA_HEADER_SIZE) || (NativeAssets_ReadLE32(&xnf.data[0]) != 0x464e4958) || (NativeAssets_ReadLE32(&xnf.data[4]) != 102) ||
	    (NativeAssets_ReadLE32(&xnf.data[8]) != NATIVE_ASSETS_XA_TYPE_COUNT))
	{
		NativeAssets_FreeBytes(&xnf);
		fprintf(stderr, "[CTR Native Expanded] invalid XA manifest: %s\n", NATIVE_ASSETS_XNF_PATH);
		return 0;
	}

	u32 numXasTotal = NativeAssets_ReadLE32(&xnf.data[NATIVE_ASSETS_XA_NUM_XAS_TOTAL_OFFSET]);
	u32 numTracksTotal = NativeAssets_ReadLE32(&xnf.data[NATIVE_ASSETS_XA_NUM_TRACKS_TOTAL_OFFSET]);
	// Bound both tables before multiplying untrusted counts into byte offsets.
	if (numXasTotal > ((u32)xnf.size - NATIVE_ASSETS_XA_HEADER_SIZE) / 4u)
	{
		NativeAssets_FreeBytes(&xnf);
		fprintf(stderr, "[CTR Native Expanded] invalid XA position table: %s\n", NATIVE_ASSETS_XNF_PATH);
		return 0;
	}
	u32 entryOffset = NATIVE_ASSETS_XA_HEADER_SIZE + numXasTotal * 4u;

	if (numTracksTotal > ((u32)xnf.size - entryOffset) / NATIVE_ASSETS_XA_ENTRY_BYTES)
	{
		NativeAssets_FreeBytes(&xnf);
		fprintf(stderr, "[CTR Native Expanded] invalid XA entry table: %s\n", NATIVE_ASSETS_XNF_PATH);
		return 0;
	}

	for (categoryID = 0; categoryID < NATIVE_ASSETS_XA_TYPE_COUNT; categoryID++)
	{
		u32 numSongs = NativeAssets_ReadLE32(&xnf.data[NATIVE_ASSETS_XA_NUM_SONGS_OFFSET + categoryID * 4u]);
		u32 firstSongIndex = NativeAssets_ReadLE32(&xnf.data[NATIVE_ASSETS_XA_FIRST_SONG_INDEX_OFFSET + categoryID * 4u]);

		if ((firstSongIndex > numTracksTotal) || (numSongs > numTracksTotal) || (firstSongIndex + numSongs > numTracksTotal))
		{
			NativeAssets_FreeBytes(&xnf);
			fprintf(stderr, "[CTR Native Expanded] invalid XA song range in %s\n", NATIVE_ASSETS_XNF_PATH);
			return 0;
		}

		for (u32 xaID = 0; xaID < numSongs; xaID++)
		{
			const u8 *entry = &xnf.data[entryOffset + (firstSongIndex + xaID) * NATIVE_ASSETS_XA_ENTRY_BYTES];
			required[categoryID][entry[1]] = 1;
		}
	}

	NativeAssets_FreeBytes(&xnf);

	for (categoryID = 0; categoryID < NATIVE_ASSETS_XA_TYPE_COUNT; categoryID++)
	{
		for (u32 fileNumber = 0; fileNumber < NATIVE_ASSETS_XA_MAX_FILE_NUMBER; fileNumber++)
		{
			char relativePath[256];

			if (!required[categoryID][fileNumber])
			{
				continue;
			}

			int written = snprintf(relativePath, sizeof(relativePath), "%s/S%02u.XA", xaDirs[categoryID], (unsigned int)fileNumber);
			if ((written <= 0) || ((size_t)written >= sizeof(relativePath)))
			{
				fprintf(stderr, "[CTR Native Expanded] XA asset path too long: %s/S%02u.XA\n", xaDirs[categoryID], (unsigned int)fileNumber);
				return 0;
			}

			if (!NativeAssets_HasFile(relativePath))
			{
				(*missing)++;
			}
		}
	}

	return 1;
}

int NativeAssets_Validate(void)
{
	int ok = 1;
	u32 missingXA = 0;

	ok &= NativeAssets_CheckRequiredFile(NATIVE_ASSETS_BIGFILE_PATH);
	ok &= NativeAssets_CheckRequiredFile(NATIVE_ASSETS_KART_HWL_PATH);

	if (ok)
	{
		int missingMovie = !NativeAssets_HasFile(NATIVE_ASSETS_TEST_STR_PATH);
		int missingManifest = !NativeAssets_HasFile(NATIVE_ASSETS_XNF_PATH);

		// NOTE(aalhendi): Missing streams use the existing playback failure paths.
		// A present but invalid manifest is still an error, not optional content.
		if (!missingManifest)
		{
			ok = NativeAssets_ValidateXA(&missingXA);
		}
		if (ok && (missingMovie || missingManifest || missingXA))
		{
			fprintf(stderr, "[CTR Native Expanded] Warning: missing optional media (%s%s%s). Unavailable audio/video will be skipped.\n",
			        missingMovie ? NATIVE_ASSETS_TEST_STR_PATH : "", missingMovie && (missingManifest || missingXA) ? ", " : "",
			        missingManifest ? NATIVE_ASSETS_XNF_PATH
			        : missingXA     ? "XA streams"
			                        : "");
		}
	}

	if (!ok)
	{
		NativeAssets_PrintHeader();
		NativeAssets_PrintFooter();
	}

	return ok;
}
