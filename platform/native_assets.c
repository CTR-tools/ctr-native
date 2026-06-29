#include "platform/native_assets.h"

#include <macros.h>

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

#define NATIVE_ASSETS_XA_TYPE_COUNT              3
#define NATIVE_ASSETS_XA_HEADER_SIZE             0x44
#define NATIVE_ASSETS_XA_NUM_XAS_TOTAL_OFFSET    0x0c
#define NATIVE_ASSETS_XA_NUM_TRACKS_TOTAL_OFFSET 0x10
#define NATIVE_ASSETS_XA_NUM_SONGS_OFFSET        0x2c
#define NATIVE_ASSETS_XA_FIRST_SONG_INDEX_OFFSET 0x38
#define NATIVE_ASSETS_XA_ENTRY_BYTES             4
#define NATIVE_ASSETS_XA_MAX_FILE_NUMBER         256

struct NativeAssetsByteBuffer
{
	u8 *data;
	int size;
};

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

internal u8 NativeAssets_ToLowerAscii(u8 byte)
{
	if ((byte >= 'A') && (byte <= 'Z'))
	{
		byte = (u8)(byte + ('a' - 'A'));
	}

	return byte;
}

internal int NativeAssets_Str8EqualsIgnoreCase(NativeStr8 left, NativeStr8 right)
{
	size_t i;

	if (left.len != right.len)
	{
		return 0;
	}

	for (i = 0; i < left.len; i++)
	{
		if (NativeAssets_ToLowerAscii(left.ptr[i]) != NativeAssets_ToLowerAscii(right.ptr[i]))
		{
			return 0;
		}
	}

	return 1;
}

internal int NativeAssets_FileExistsHost(const char *path)
{
	FILE *file;

	if (path == NULL)
	{
		return 0;
	}

	file = fopen(path, "rb");
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

			if ((match[0] == '\0') && NativeAssets_Str8EqualsIgnoreCase(entryName, child))
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
	DIR *dir;
	struct dirent *entry;

	dir = opendir(parentPath);
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

		if ((match[0] == '\0') && NativeAssets_Str8EqualsIgnoreCase(entryName, child))
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
	size_t size;
	char *copy;

	if (src == NULL)
	{
		return NULL;
	}

	size = strlen(src) + 1u;
	copy = (char *)malloc(size);
	if (copy == NULL)
	{
		return NULL;
	}

	memcpy(copy, src, size);
	return copy;
}

internal void NativeAssets_ClearIndex(void)
{
	int i;

	for (i = 0; i < s_nativeAssetIndexCount; i++)
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
	struct NativeAssetIndexEntry *entries;
	int newCapacity;

	if (needed <= s_nativeAssetIndexCapacity)
	{
		return 1;
	}

	newCapacity = (s_nativeAssetIndexCapacity == 0) ? 256 : s_nativeAssetIndexCapacity * 2;
	while (newCapacity < needed)
	{
		newCapacity *= 2;
	}

	entries = (struct NativeAssetIndexEntry *)realloc(s_nativeAssetIndex, (size_t)newCapacity * sizeof(*s_nativeAssetIndex));
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
	size_t i;

	relativePath = NativePath_SkipLeadingSeparators(relativePath);
	if ((dst == NULL) || (dstSize == 0) || (relativePath.len >= dstSize))
	{
		return 0;
	}

	for (i = 0; i < relativePath.len; i++)
	{
		u8 byte = relativePath.ptr[i];
		dst[i] = (char)(NativePath_IsSeparator(byte) ? '/' : NativeAssets_ToLowerAscii(byte));
	}

	dst[relativePath.len] = '\0';
	return 1;
}

internal const char *NativeAssets_FindIndexedPath(NativeStr8 relativePath)
{
	char key[NATIVE_ASSETS_PATH_MAX];
	int i;

	if (!NativeAssets_NormalizeRelativeKey(relativePath, key, sizeof(key)))
	{
		return NULL;
	}

	for (i = 0; i < s_nativeAssetIndexCount; i++)
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
	char *keyCopy;
	char *pathCopy;

	if (!NativeAssets_NormalizeRelativeKey(relativePath, key, sizeof(key)))
	{
		return 0;
	}

	if (!NativeAssets_IndexReserve(s_nativeAssetIndexCount + 1))
	{
		return 0;
	}

	keyCopy = NativeAssets_CopyCString(key);
	pathCopy = NativeAssets_CopyCString(hostPath);
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
	DIR *dir;
	struct dirent *entry;

	dir = opendir(dirPath);
	if (dir == NULL)
	{
		return;
	}

	while ((entry = readdir(dir)) != NULL)
	{
		NativeStr8 entryName = NativeStr8_FromCString(entry->d_name);
		char childPath[NATIVE_ASSETS_PATH_MAX];
		char relativePath[NATIVE_ASSETS_PATH_MAX];
		DIR *childDir;

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

		childDir = opendir(childPath);
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

	NativeAssets_ClearIndex();
	s_nativeAssetsInitialized = 1;
	return 1;
}

int NativeAssets_Init(const char *executableBasePath)
{
	char parentDir[NATIVE_ASSETS_PATH_MAX];
	NativeStr8 exeDir;

	if ((executableBasePath == NULL) || (executableBasePath[0] == '\0'))
	{
		executableBasePath = ".";
	}

	exeDir = NativePath_TrimTrailingSeparators(NativeStr8_FromCString(executableBasePath));

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
	const char *indexedPath;

	if (!NativeAssets_BuildPathStr8(relativePath, path, sizeof(path)))
	{
		return 0;
	}

	if (NativeAssets_FileExistsHost(path))
	{
		return NativePath_NormalizeSlashes(dst, dstSize, NativeStr8_FromCString(path));
	}

	NativeAssets_BuildIndex();
	indexedPath = NativeAssets_FindIndexedPath(relativePath);
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

FILE *NativeAssets_OpenStr8(NativeStr8 relativePath, const char *mode)
{
	char path[NATIVE_ASSETS_PATH_MAX];
	FILE *file;

	if ((mode == NULL) || !NativeAssets_BuildPathStr8(relativePath, path, sizeof(path)))
	{
		return NULL;
	}

	file = fopen(path, mode);
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

FILE *NativeAssets_Open(const char *relativePath, const char *mode)
{
	return NativeAssets_OpenStr8(NativeStr8_FromCString(relativePath), mode);
}

FILE *NativeAssets_OpenBigfile(const char *mode)
{
	return NativeAssets_OpenStr8(NATIVE_STR8_LIT(NATIVE_ASSETS_BIGFILE_PATH), mode);
}

internal int NativeAssets_ReadExact(FILE *file, void *dst, size_t size)
{
	return fread(dst, 1, size, file) == size;
}

internal u32 NativeAssets_ReadLE32(const u8 *data)
{
	return ((u32)data[0]) | ((u32)data[1] << 8) | ((u32)data[2] << 16) | ((u32)data[3] << 24);
}

internal int NativeAssets_ReadFileBytes(const char *path, struct NativeAssetsByteBuffer *bytes)
{
	FILE *file;
	long size;

	bytes->data = NULL;
	bytes->size = 0;

	file = NativeAssets_Open(path, "rb");
	if (file == NULL)
	{
		return 0;
	}

	if (fseek(file, 0, SEEK_END) != 0)
	{
		fclose(file);
		return 0;
	}

	size = ftell(file);
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

internal void NativeAssets_FreeByteBuffer(struct NativeAssetsByteBuffer *bytes)
{
	free(bytes->data);
	bytes->data = NULL;
	bytes->size = 0;
}

internal void NativeAssets_PrintHeader(void)
{
	fprintf(stderr, "[CTR Native] Missing or incomplete assets.\n");
	fprintf(stderr, "[CTR Native] Expected NTSC-U retail assets under: %s\n", NativeAssets_GetAssetDir());
}

internal void NativeAssets_PrintFooter(void)
{
	fprintf(stderr, "[CTR Native] Required files: %s, %s, %s, %s, plus XA files referenced by %s\n", NATIVE_ASSETS_BIGFILE_PATH, NATIVE_ASSETS_KART_HWL_PATH,
	        NATIVE_ASSETS_TEST_STR_PATH, NATIVE_ASSETS_XNF_PATH, NATIVE_ASSETS_XNF_PATH);
}

internal int NativeAssets_CheckRequiredFile(const char *path)
{
	char assetPath[NATIVE_ASSETS_PATH_MAX];

	if (!NativeAssets_BuildPath(path, assetPath, sizeof(assetPath)))
	{
		fprintf(stderr, "[CTR Native] asset path too long: %s\n", path);
		return 0;
	}

	if (NativeAssets_ResolvePath(path, assetPath, sizeof(assetPath)))
	{
		return 1;
	}

	fprintf(stderr, "[CTR Native] missing asset: %s\n", assetPath);
	return 0;
}

internal int NativeAssets_ValidateXA(void)
{
	local_persist const char *xaDirs[NATIVE_ASSETS_XA_TYPE_COUNT] = {
	    "XA/MUSIC",
	    "XA/ENG/EXTRA",
	    "XA/ENG/GAME",
	};
	struct NativeAssetsByteBuffer xnf;
	u8 required[NATIVE_ASSETS_XA_TYPE_COUNT][NATIVE_ASSETS_XA_MAX_FILE_NUMBER];
	u32 numXasTotal;
	u32 numTracksTotal;
	u32 entryOffset;
	u32 entryEnd;
	u32 missing = 0;
	u32 categoryID;

	memset(required, 0, sizeof(required));

	if (!NativeAssets_ReadFileBytes(NATIVE_ASSETS_XNF_PATH, &xnf))
	{
		return 0;
	}

	if ((xnf.size < NATIVE_ASSETS_XA_HEADER_SIZE) || (NativeAssets_ReadLE32(&xnf.data[0]) != 0x464e4958) || (NativeAssets_ReadLE32(&xnf.data[4]) != 102) ||
	    (NativeAssets_ReadLE32(&xnf.data[8]) != NATIVE_ASSETS_XA_TYPE_COUNT))
	{
		NativeAssets_FreeByteBuffer(&xnf);
		fprintf(stderr, "[CTR Native] invalid XA manifest: %s\n", NATIVE_ASSETS_XNF_PATH);
		return 0;
	}

	numXasTotal = NativeAssets_ReadLE32(&xnf.data[NATIVE_ASSETS_XA_NUM_XAS_TOTAL_OFFSET]);
	numTracksTotal = NativeAssets_ReadLE32(&xnf.data[NATIVE_ASSETS_XA_NUM_TRACKS_TOTAL_OFFSET]);
	entryOffset = NATIVE_ASSETS_XA_HEADER_SIZE + numXasTotal * 4u;
	entryEnd = entryOffset + numTracksTotal * NATIVE_ASSETS_XA_ENTRY_BYTES;

	if ((entryEnd < entryOffset) || (entryEnd > (u32)xnf.size))
	{
		NativeAssets_FreeByteBuffer(&xnf);
		fprintf(stderr, "[CTR Native] invalid XA entry table: %s\n", NATIVE_ASSETS_XNF_PATH);
		return 0;
	}

	for (categoryID = 0; categoryID < NATIVE_ASSETS_XA_TYPE_COUNT; categoryID++)
	{
		u32 numSongs = NativeAssets_ReadLE32(&xnf.data[NATIVE_ASSETS_XA_NUM_SONGS_OFFSET + categoryID * 4u]);
		u32 firstSongIndex = NativeAssets_ReadLE32(&xnf.data[NATIVE_ASSETS_XA_FIRST_SONG_INDEX_OFFSET + categoryID * 4u]);
		u32 xaID;

		if ((firstSongIndex > numTracksTotal) || (numSongs > numTracksTotal) || (firstSongIndex + numSongs > numTracksTotal))
		{
			NativeAssets_FreeByteBuffer(&xnf);
			fprintf(stderr, "[CTR Native] invalid XA song range in %s\n", NATIVE_ASSETS_XNF_PATH);
			return 0;
		}

		for (xaID = 0; xaID < numSongs; xaID++)
		{
			const u8 *entry = &xnf.data[entryOffset + (firstSongIndex + xaID) * NATIVE_ASSETS_XA_ENTRY_BYTES];
			required[categoryID][entry[1]] = 1;
		}
	}

	NativeAssets_FreeByteBuffer(&xnf);

	for (categoryID = 0; categoryID < NATIVE_ASSETS_XA_TYPE_COUNT; categoryID++)
	{
		u32 fileNumber;

		for (fileNumber = 0; fileNumber < NATIVE_ASSETS_XA_MAX_FILE_NUMBER; fileNumber++)
		{
			char relativePath[256];
			char path[NATIVE_ASSETS_PATH_MAX];
			int written;

			if (!required[categoryID][fileNumber])
			{
				continue;
			}

			written = snprintf(relativePath, sizeof(relativePath), "%s/S%02u.XA", xaDirs[categoryID], (unsigned int)fileNumber);
			if ((written <= 0) || ((size_t)written >= sizeof(relativePath)) || !NativeAssets_BuildPath(relativePath, path, sizeof(path)))
			{
				fprintf(stderr, "[CTR Native] XA asset path too long: %s/S%02u.XA\n", xaDirs[categoryID], (unsigned int)fileNumber);
				missing++;
				continue;
			}

			if (!NativeAssets_ResolvePath(relativePath, path, sizeof(path)))
			{
				fprintf(stderr, "[CTR Native] missing XA asset: %s\n", path);
				missing++;
			}
		}
	}

	return missing == 0;
}

int NativeAssets_Validate(void)
{
	int ok = 1;

	ok &= NativeAssets_CheckRequiredFile(NATIVE_ASSETS_BIGFILE_PATH);
	ok &= NativeAssets_CheckRequiredFile(NATIVE_ASSETS_KART_HWL_PATH);
	ok &= NativeAssets_CheckRequiredFile(NATIVE_ASSETS_TEST_STR_PATH);
	ok &= NativeAssets_CheckRequiredFile(NATIVE_ASSETS_XNF_PATH);

	if (ok)
	{
		ok &= NativeAssets_ValidateXA();
	}

	if (!ok)
	{
		NativeAssets_PrintHeader();
		NativeAssets_PrintFooter();
	}

	return ok;
}
