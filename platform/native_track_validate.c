#include <platform/native_track_validate.h>

#include <stdint.h>

enum
{
	NATIVE_TRACK_WORD_BYTES = 4,
	NATIVE_TRACK_VRAM_HEADER_BYTES = 20,
	NATIVE_TRACK_VRAM_MULTI = 0x20,
	NATIVE_TRACK_VRAM_WIDTH = 1024,
	NATIVE_TRACK_VRAM_HEIGHT = 512,
};

static uint32_t NativeTrackValidate_Read32(const unsigned char *bytes)
{
	return (uint32_t)bytes[0] | (uint32_t)bytes[1] << 8 | (uint32_t)bytes[2] << 16 | (uint32_t)bytes[3] << 24;
}

static uint16_t NativeTrackValidate_Read16(const unsigned char *bytes)
{
	return (uint16_t)((uint16_t)bytes[0] | (uint16_t)bytes[1] << 8);
}

int NativeTrackValidate_Lev(const void *data, size_t size)
{
	if (data == NULL || size < 2 * NATIVE_TRACK_WORD_BYTES)
	{
		return 0;
	}
	const unsigned char *bytes = data;
	size_t payload = size - NATIVE_TRACK_WORD_BYTES;
	uint32_t mapOffset = NativeTrackValidate_Read32(bytes);
	if (mapOffset > payload || payload - mapOffset < NATIVE_TRACK_WORD_BYTES)
	{
		return 0;
	}
	const unsigned char *map = bytes + NATIVE_TRACK_WORD_BYTES + mapOffset;
	uint32_t mapSize = NativeTrackValidate_Read32(map);
	if (mapSize % NATIVE_TRACK_WORD_BYTES != 0 || mapSize > payload - mapOffset - NATIVE_TRACK_WORD_BYTES)
	{
		return 0;
	}
	for (size_t i = 0; i < mapSize; i += NATIVE_TRACK_WORD_BYTES)
	{
		uint32_t slot = NativeTrackValidate_Read32(map + NATIVE_TRACK_WORD_BYTES + i) & ~UINT32_C(3);
		if (slot > mapOffset || mapOffset - slot < NATIVE_TRACK_WORD_BYTES)
		{
			return 0;
		}
		uint32_t target = NativeTrackValidate_Read32(bytes + NATIVE_TRACK_WORD_BYTES + slot);
		if (target >= mapOffset)
		{
			return 0;
		}
	}
	return 1;
}

static int NativeTrackValidate_VramImage(const unsigned char *bytes, size_t size)
{
	if (size < NATIVE_TRACK_VRAM_HEADER_BYTES)
	{
		return 0;
	}
	unsigned int x = NativeTrackValidate_Read16(bytes + 12);
	unsigned int y = NativeTrackValidate_Read16(bytes + 14);
	unsigned int width = NativeTrackValidate_Read16(bytes + 16);
	unsigned int height = NativeTrackValidate_Read16(bytes + 18);
	if (width == 0 || height == 0 || x >= NATIVE_TRACK_VRAM_WIDTH || y >= NATIVE_TRACK_VRAM_HEIGHT ||
	    width > NATIVE_TRACK_VRAM_WIDTH - x || height > NATIVE_TRACK_VRAM_HEIGHT - y)
	{
		return 0;
	}
	return (size_t)width * height * 2 <= size - NATIVE_TRACK_VRAM_HEADER_BYTES;
}

int NativeTrackValidate_Vrm(const void *data, size_t size)
{
	if (data == NULL || size < NATIVE_TRACK_WORD_BYTES)
	{
		return 0;
	}
	const unsigned char *bytes = data;
	if (NativeTrackValidate_Read32(bytes) != NATIVE_TRACK_VRAM_MULTI)
	{
		return NativeTrackValidate_VramImage(bytes, size);
	}
	size_t position = NATIVE_TRACK_WORD_BYTES;
	while (position <= size && size - position >= NATIVE_TRACK_WORD_BYTES)
	{
		uint32_t rawSize = NativeTrackValidate_Read32(bytes + position);
		position += NATIVE_TRACK_WORD_BYTES;
		if (rawSize == 0)
		{
			return 1;
		}
		size_t imageSize = rawSize & ~UINT32_C(3);
		if (imageSize > size - position || !NativeTrackValidate_VramImage(bytes + position, imageSize))
		{
			return 0;
		}
		position += imageSize;
	}
	return 0;
}
