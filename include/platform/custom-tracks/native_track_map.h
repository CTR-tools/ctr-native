#ifndef CTR_NATIVE_TRACK_MAP_H
#define CTR_NATIVE_TRACK_MAP_H

#include <stddef.h>
#include <stdint.h>

enum { NATIVE_TRACK_MAP_MAX_WIDTH = 96, NATIVE_TRACK_MAP_MAX_HEIGHT = 96,
       NATIVE_TRACK_MAP_TEXTURE_SIZE = 128 };

struct NativeTrackMap
{
	int width, height, topHeight;
	uint16_t pixels[NATIVE_TRACK_MAP_TEXTURE_SIZE * NATIVE_TRACK_MAP_TEXTURE_SIZE];
};

/* Decode a track's map into an independent 16-bit image, without modifying VRAM. */
int NativeTrackMap_Decode(struct NativeTrackMap *out, const void *lev, size_t levSize, const void *vrm, size_t vrmSize);
int NativeTrackMap_Load(struct NativeTrackMap *out, int levelID);

#endif
