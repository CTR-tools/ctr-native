#ifndef NATIVE_DISC_IMAGE_H
#define NATIVE_DISC_IMAGE_H

#include <macros.h>

#include <stddef.h>

struct NativeDiscImageFile
{
	u32 lba;
	u32 size;
};

enum NativeDiscImageId
{
	NATIVE_DISC_IMAGE_BASE,
	NATIVE_DISC_IMAGE_EUROPE,
	NATIVE_DISC_IMAGE_COUNT,
};

int NativeDiscImage_Init(const char *assetsDir);
u32 NativeDiscImage_Generation(void);
int NativeDiscImage_FindFile(const char *path, struct NativeDiscImageFile *fileOut);
int NativeDiscImage_ReadDataSectors(const struct NativeDiscImageFile *file, u32 sector, u32 sectorCount, void *dst);
int NativeDiscImage_ReadRawSectors(const struct NativeDiscImageFile *file, u32 sector, u32 sectorCount, void *dst);
int NativeDiscImage_ReadFileBytes(const char *path, int rawSectors, u8 **dataOut, int *sizeOut);

int NativeDiscImage_FindFileOn(enum NativeDiscImageId image, const char *path, struct NativeDiscImageFile *fileOut);
int NativeDiscImage_ReadDataSectorsOn(enum NativeDiscImageId image, const struct NativeDiscImageFile *file, u32 sector, u32 sectorCount, void *dst);
int NativeDiscImage_ReadRawSectorsOn(enum NativeDiscImageId image, const struct NativeDiscImageFile *file, u32 sector, u32 sectorCount, void *dst);
int NativeDiscImage_ReadFileBytesOn(enum NativeDiscImageId image, const char *path, int rawSectors, u8 **dataOut, int *sizeOut);

#endif
