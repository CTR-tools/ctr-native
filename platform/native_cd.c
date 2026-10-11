#include <macros.h>
#include <platform/native_assets.h>
#include <platform/native_cd.h>
#include <platform/native_disc_image.h>
#include <platform/native_path.h>
#include <psx/libcd.h>

#include <stdio.h>
#include <string.h>

// NOTE(aalhendi): Native exports the retail Cd* API. Extracted host files stay
// as dev/modding overrides; missing files can fall back to assets/ctr-u.bin.

#define NATIVE_CD_SECTOR_WORDS   512
#define NATIVE_CD_MAX_OPEN_FILES 8
#define NATIVE_CD_SECTOR_SIZE    0x800

enum NativeCDFileSource
{
	NATIVE_CD_FILE_NONE,
	NATIVE_CD_FILE_HOST,
	NATIVE_CD_FILE_DISC,
};

struct NativeCDOpenFile
{
	int source;
	FILE *hostFile;
	struct NativeDiscImageFile discFile;
};

global_variable s32 CD_debug;
global_variable s32 s_cdLastCom;
global_variable CdlCB CD_cbready;
global_variable CdlCB CD_cbsync;
global_variable u32 s_cdSectorData[NATIVE_CD_SECTOR_WORDS];
global_variable CdlCB s_nativeCdReadCallback;
global_variable struct NativeCDOpenFile s_nativeCdFiles[NATIVE_CD_MAX_OPEN_FILES];
global_variable s32 s_nativeCdFileCount;
global_variable s32 s_nativeCdCurrentFile;
global_variable s32 s_nativeCdCurrentSector;

int boolDecodeXaDuringVsyncCallback;

internal s32 NativeCD_NormalizeFilename(char *dst, s32 dstCount, const char *src)
{
	NativeStr8 filename = NativeStr8_FromCString(src);

	if ((dstCount <= 0) || (src == NULL))
	{
		return 0;
	}

	for (size_t i = 0; i < filename.len; i++)
	{
		if (filename.ptr[i] == ';')
		{
			filename.len = i;
			break;
		}
	}

	return NativePath_NormalizeSlashes(dst, (size_t)dstCount, filename);
}

internal NativeStr8 NativeCD_PathAfterRoot(NativeStr8 filename)
{
	if ((filename.len != 0) && NativePath_IsSeparator(filename.ptr[0]))
	{
		return NativeStr8_Skip(filename, 1);
	}

	return filename;
}

internal s32 NativeCD_OpenFile(const char *filename, s32 *outSize)
{
	char normalized[256];
	char rootlessPath[256];
	struct NativeDiscImageFile discFile;
	s32 fileIndex;

	if (s_nativeCdFileCount >= NATIVE_CD_MAX_OPEN_FILES)
	{
		return -1;
	}

	if (NativeCD_NormalizeFilename(normalized, sizeof(normalized), filename) == 0)
	{
		return -1;
	}

	NativeStr8 rootless = NativeCD_PathAfterRoot(NativeStr8_FromCString(normalized));

	FILE *file = NativeAssets_OpenHostStr8(rootless, "rb");
	if ((file == NULL) && NativeStr8_CopyToCString(rootlessPath, sizeof(rootlessPath), rootless))
	{
		file = fopen(rootlessPath, "rb");
	}

	if (file == NULL)
	{
		if (!NativeStr8_CopyToCString(rootlessPath, sizeof(rootlessPath), rootless) || !NativeDiscImage_FindFile(rootlessPath, &discFile))
		{
			return -1;
		}

		fileIndex = s_nativeCdFileCount++;
		memset(&s_nativeCdFiles[fileIndex], 0, sizeof(s_nativeCdFiles[fileIndex]));
		s_nativeCdFiles[fileIndex].source = NATIVE_CD_FILE_DISC;
		s_nativeCdFiles[fileIndex].discFile = discFile;
		*outSize = (s32)discFile.size;
		return fileIndex;
	}

	if (fseek(file, 0, SEEK_END) != 0)
	{
		fclose(file);
		return -1;
	}

	long fileSize = ftell(file);
	if (fileSize < 0)
	{
		fclose(file);
		return -1;
	}

	if (fseek(file, 0, SEEK_SET) != 0)
	{
		fclose(file);
		return -1;
	}

	fileIndex = s_nativeCdFileCount++;
	memset(&s_nativeCdFiles[fileIndex], 0, sizeof(s_nativeCdFiles[fileIndex]));
	s_nativeCdFiles[fileIndex].source = NATIVE_CD_FILE_HOST;
	s_nativeCdFiles[fileIndex].hostFile = file;
	*outSize = (s32)fileSize;
	return fileIndex;
}

internal s32 NativeCD_SearchFile(CdlFILE *loc, const char *filename)
{
	char normalized[256];
	char rootlessPath[256];
	s32 fileSize;
	s32 encodedPos;

	if ((loc == NULL) || (filename == NULL))
	{
		return 0;
	}

	s32 fileIndex = NativeCD_OpenFile(filename, &fileSize);
	if (fileIndex < 0)
	{
		return 0;
	}

	if (NativeCD_NormalizeFilename(normalized, sizeof(normalized), filename) == 0)
	{
		return 0;
	}

	NativeStr8 rootless = NativeCD_PathAfterRoot(NativeStr8_FromCString(normalized));
	if (!NativeStr8_CopyToCString(rootlessPath, sizeof(rootlessPath), rootless))
	{
		return 0;
	}

	encodedPos = fileIndex << 24;

	memcpy(&loc->pos, &encodedPos, sizeof(encodedPos));
	loc->size = fileSize;
	memset(loc->name, 0, sizeof(loc->name));
	size_t nameLen = rootless.len;
	if (nameLen >= sizeof(loc->name))
	{
		nameLen = sizeof(loc->name) - 1;
	}
	memcpy(loc->name, rootless.ptr, nameLen);

	return 1;
}

internal s32 NativeCD_PosToInt(const CdlLOC *pos)
{
	s32 value;

	memcpy(&value, pos, sizeof(value));
	return value;
}

internal void NativeCD_IntToPos(s32 value, CdlLOC *pos)
{
	memcpy(pos, &value, sizeof(value));
}

internal s32 NativeCD_SetLoc(const CdlLOC *pos)
{
	s32 encodedPos = NativeCD_PosToInt(pos);
	s32 fileIndex = (encodedPos >> 24) & 0xff;
	s32 sector = encodedPos & 0xffffff;

	if ((fileIndex < 0) || (fileIndex >= s_nativeCdFileCount) || (s_nativeCdFiles[fileIndex].source == NATIVE_CD_FILE_NONE))
	{
		return 0;
	}

	s_nativeCdCurrentFile = fileIndex;
	s_nativeCdCurrentSector = sector;

	if (s_nativeCdFiles[s_nativeCdCurrentFile].source == NATIVE_CD_FILE_DISC)
	{
		return 1;
	}

	return fseek(s_nativeCdFiles[s_nativeCdCurrentFile].hostFile, sector * NATIVE_CD_SECTOR_SIZE, SEEK_SET) == 0;
}

internal s32 NativeCD_ReadSectors(s32 sectors, void *dst)
{
	if ((s_nativeCdCurrentFile < 0) || (s_nativeCdCurrentFile >= s_nativeCdFileCount) || (s_nativeCdFiles[s_nativeCdCurrentFile].source == NATIVE_CD_FILE_NONE))
	{
		return 0;
	}

	size_t byteCount = (size_t)sectors * NATIVE_CD_SECTOR_SIZE;

	if (s_nativeCdFiles[s_nativeCdCurrentFile].source == NATIVE_CD_FILE_DISC)
	{
		if (!NativeDiscImage_ReadDataSectors(&s_nativeCdFiles[s_nativeCdCurrentFile].discFile, (u32)s_nativeCdCurrentSector, (u32)sectors, dst))
		{
			return 0;
		}

		s_nativeCdCurrentSector += sectors;
		return 1;
	}

	return fread(dst, 1, byteCount, s_nativeCdFiles[s_nativeCdCurrentFile].hostFile) == byteCount;
}

internal void NativeCD_SetLastCom(int com)
{
	s_cdLastCom = com;
}

int NativeCD_Init(void)
{
	for (s32 i = 0; i < s_nativeCdFileCount; i++)
	{
		if (s_nativeCdFiles[i].hostFile != NULL)
		{
			fclose(s_nativeCdFiles[i].hostFile);
		}
	}

	CD_debug = 0;
	s_cdLastCom = 0;
	CD_cbready = NULL;
	CD_cbsync = NULL;
	s_nativeCdReadCallback = NULL;
	memset(s_nativeCdFiles, 0, sizeof(s_nativeCdFiles));
	s_nativeCdFileCount = 0;
	s_nativeCdCurrentFile = -1;
	s_nativeCdCurrentSector = 0;
	memset(s_cdSectorData, 0, sizeof(s_cdSectorData));
	return 0;
}

int CdInit(void)
{
	s_cdLastCom = 0;
	s_nativeCdCurrentFile = -1;
	s_nativeCdCurrentSector = 0;
	memset(s_cdSectorData, 0, sizeof(s_cdSectorData));
	return 1;
}

#include "../game/PSX/PSX_SDK_CDState.c"

int CdLastCom(void)
{
	return s_cdLastCom;
}

int CdGetSector(void *madr, int size)
{
	if ((madr == NULL) || (size <= 0))
	{
		return 1;
	}

	u32 byteCount = (u32)size * sizeof(u32);
	if (byteCount > sizeof(s_cdSectorData))
	{
		byteCount = sizeof(s_cdSectorData);
	}

	memcpy(madr, s_cdSectorData, byteCount);
	return 1;
}

CdlCB CdReadCallback(CdlCB func)
{
	CdlCB old = s_nativeCdReadCallback;
	s_nativeCdReadCallback = func;
	return old;
}

int CdPosToInt(CdlLOC *p)
{
	return NativeCD_PosToInt(p);
}

CdlLOC *CdIntToPos(int val, CdlLOC *p)
{
	NativeCD_IntToPos(val, p);
	return p;
}

CdlFILE *CdSearchFile(CdlFILE *loc, char *filename)
{
	if (NativeCD_SearchFile(loc, filename) == 0)
	{
		return NULL;
	}

	return loc;
}

int CdControl(uint8_t com, uint8_t *param, uint8_t *result)
{
	(void)result;

	NativeCD_SetLastCom(com);

	if ((com == CdlSetloc) && (param != NULL))
	{
		NativeCD_SetLoc((const CdlLOC *)param);
	}

	if ((com == CdlSetmode) && (param != NULL))
	{
		if (param[0] == 0xE8)
		{
			boolDecodeXaDuringVsyncCallback = 1;
		}

		if (param[0] == CdlModeSpeed)
		{
			boolDecodeXaDuringVsyncCallback = 0;
		}
	}

	return 1;
}

int CdRead(int sectors, uint32_t *buf, int mode)
{
	(void)mode;

	NativeCD_ReadSectors(sectors, buf);

	if (s_nativeCdReadCallback != 0)
	{
		s_nativeCdReadCallback(CdlComplete, 0);
	}

	return 1;
}

int CdReadSync(int mode, uint8_t *result)
{
	(void)mode;
	(void)result;

	return 0;
}
