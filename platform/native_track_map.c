#include <platform/native_track_map.h>
#include <platform/native_track_file.h>
#include <platform/native_track_library.h>
#include <platform/native_track_validate.h>
#include <stdlib.h>
#include <string.h>

enum { TRACK_MAP_VRAM_W = 1024, TRACK_MAP_VRAM_H = 512, TRACK_MAP_ICON_BYTES = 32,
       TRACK_MAP_MAX_FILE = 16 * 1024 * 1024, TRACK_MAP_ICON_TOP = 3, TRACK_MAP_ICON_BOTTOM = 4 };

static unsigned int TrackMap_U16(const unsigned char *p)
{
	return p[0] | (unsigned int)p[1] << 8;
}

static uint32_t TrackMap_U32(const unsigned char *p)
{
	return TrackMap_U16(p) | (uint32_t)TrackMap_U16(p + 2) << 16;
}

static int TrackMap_Range(size_t size, size_t at, size_t count)
{
	return at <= size && count <= size - at;
}

static int TrackMap_Icons(const unsigned char *lev, size_t size, const unsigned char **top, const unsigned char **bottom)
{
	if (!NativeTrackValidate_Lev(lev, size) || size < 4 + 0x40) { return 0; }
	size_t payload = TrackMap_U32(lev);
	if (payload > size - 4 || payload < 0x40) { return 0; }
	const unsigned char *base = lev + 4;
	size_t lookup = TrackMap_U32(base + 0x3c);
	if (lookup == 0 || !TrackMap_Range(payload, lookup, 16)) { return 0; }
	uint32_t count = TrackMap_U32(base + lookup), icons = TrackMap_U32(base + lookup + 4);
	if (count > 4096 || !TrackMap_Range(payload, icons, (size_t)count * TRACK_MAP_ICON_BYTES)) { return 0; }
	*top = NULL;
	*bottom = NULL;
	for (uint32_t i = 0; i < count; i++)
	{
		const unsigned char *icon = base + icons + i * TRACK_MAP_ICON_BYTES;
		uint32_t id = TrackMap_U32(icon + 16);
		const unsigned char **target = id == TRACK_MAP_ICON_TOP ? top : id == TRACK_MAP_ICON_BOTTOM ? bottom : NULL;
		if (target != NULL)
		{
			if (*target != NULL) { return 0; }
			*target = icon + 20;
		}
	}
	return *top != NULL && *bottom != NULL;
}

static void TrackMap_VramImage(uint16_t *vram, unsigned char *present, const unsigned char *image)
{
	unsigned int x = TrackMap_U16(image + 12), y = TrackMap_U16(image + 14);
	unsigned int width = TrackMap_U16(image + 16), height = TrackMap_U16(image + 18);
	for (unsigned int row = 0; row < height; row++)
	{
		for (unsigned int col = 0; col < width; col++)
		{
			size_t target = (size_t)(y + row) * TRACK_MAP_VRAM_W + x + col;
			vram[target] = (uint16_t)TrackMap_U16(image + 20 + ((size_t)row * width + col) * 2);
			present[target] = 1;
		}
	}
}

static int TrackMap_Pixel(const uint16_t *vram, const unsigned char *present, const unsigned char *layout,
                          unsigned int u, unsigned int v, uint16_t *color)
{
	unsigned int clut = TrackMap_U16(layout + 2), page = TrackMap_U16(layout + 6);
	unsigned int depth = page >> 7 & 3;
	if (depth > 2 || u > 255 || v > 255) { return 0; }
	unsigned int x = (page & 15) * 64 + (depth == 0 ? u / 4 : depth == 1 ? u / 2 : u);
	unsigned int y = ((page & 16) << 4) + ((page & 0x800) >> 2) + v;
	if (x >= TRACK_MAP_VRAM_W || y >= TRACK_MAP_VRAM_H) { return 0; }
	size_t pos = (size_t)y * TRACK_MAP_VRAM_W + x;
	if (!present[pos]) { return 0; }
	unsigned int pixel = vram[pos];
	if (depth != 2)
	{
		unsigned int index = depth == 0 ? (pixel >> ((u & 3) * 4)) & 15 : (pixel >> ((u & 1) * 8)) & 255;
		unsigned int cx = (clut & 63) * 16 + index, cy = clut >> 6 & 511;
		if (cx >= TRACK_MAP_VRAM_W) { return 0; }
		pos = (size_t)cy * TRACK_MAP_VRAM_W + cx;
		if (!present[pos]) { return 0; }
		pixel = vram[pos];
	}
	*color = (uint16_t)pixel;
	return 1;
}

int NativeTrackMap_Decode(struct NativeTrackMap *out, const void *levData, size_t levSize, const void *vrmData, size_t vrmSize)
{
	if (out == NULL || levData == NULL || vrmData == NULL) { return 0; }
	memset(out, 0, sizeof(*out));
	const unsigned char *top, *bottom, *vrm = vrmData;
	if (!TrackMap_Icons(levData, levSize, &top, &bottom) || !NativeTrackValidate_Vrm(vrm, vrmSize)) { return 0; }
	int width = (int)top[4] - top[0], widthBottom = (int)bottom[4] - bottom[0];
	int topHeight = (int)top[9] - top[1], bottomHeight = (int)bottom[9] - bottom[1];
	if (width <= 0 || width != widthBottom || topHeight <= 0 || bottomHeight <= 0) { return 0; }
	int height = topHeight + bottomHeight;
	int drawWidth = width, drawHeight = height;
	if (drawWidth > NATIVE_TRACK_MAP_MAX_WIDTH)
	{
		drawHeight = drawHeight * NATIVE_TRACK_MAP_MAX_WIDTH / drawWidth;
		drawWidth = NATIVE_TRACK_MAP_MAX_WIDTH;
	}
	if (drawHeight > NATIVE_TRACK_MAP_MAX_HEIGHT)
	{
		drawWidth = drawWidth * NATIVE_TRACK_MAP_MAX_HEIGHT / drawHeight;
		drawHeight = NATIVE_TRACK_MAP_MAX_HEIGHT;
	}
	if (drawWidth <= 0 || drawHeight <= 0) { return 0; }
	size_t pixels = TRACK_MAP_VRAM_W * TRACK_MAP_VRAM_H;
	uint16_t *vram = calloc(pixels, sizeof(*vram));
	unsigned char *present = calloc(pixels, 1);
	if (vram == NULL || present == NULL) { free(vram); free(present); return 0; }
	if (TrackMap_U32(vrm) == 0x20)
	{
		for (size_t at = 4; TrackMap_U32(vrm + at) != 0;)
		{
			size_t length = TrackMap_U32(vrm + at) & ~UINT32_C(3);
			at += 4;
			TrackMap_VramImage(vram, present, vrm + at);
			at += length;
		}
	}
	else { TrackMap_VramImage(vram, present, vrm); }
	int ok = 1, any = 0;
	for (int y = 0; ok && y < drawHeight; y++)
	{
		int sy = y * height / drawHeight;
		const unsigned char *layout = sy < topHeight ? top : bottom;
		int localY = sy < topHeight ? sy : sy - topHeight;
		for (int x = 0; x < drawWidth; x++)
		{
			uint16_t color;
			if (!TrackMap_Pixel(vram, present, layout, layout[0] + x * width / drawWidth, layout[1] + localY, &color))
			{
				ok = 0; break;
			}
			out->pixels[y * NATIVE_TRACK_MAP_TEXTURE_SIZE + x] = color;
			any |= color != 0;
		}
	}
	free(vram);
	free(present);
	if (!ok || !any) { memset(out, 0, sizeof(*out)); return 0; }
	out->width = drawWidth;
	out->height = drawHeight;
	out->topHeight = (topHeight * drawHeight + height - 1) / height;
	if (out->topHeight <= 0 || out->topHeight >= drawHeight) { memset(out, 0, sizeof(*out)); return 0; }
	return 1;
}

static unsigned char *TrackMap_ReadFile(int levelID, enum NativeTrackFileRole role, size_t *size)
{
	struct NativeTrackFile *file = NativeTrackFile_Open(NativeTrackLibrary_Current(), levelID, role);
	if (file == NULL) { return NULL; }
	*size = NativeTrackFile_Size(file);
	size_t padded = NativeTrackFile_SectorBytes(file);
	unsigned char *bytes = *size <= TRACK_MAP_MAX_FILE ? malloc(padded) : NULL;
	if (bytes != NULL && !NativeTrackFile_Read(file, bytes, padded)) { free(bytes); bytes = NULL; }
	NativeTrackFile_Close(file);
	return bytes;
}

int NativeTrackMap_Load(struct NativeTrackMap *out, int levelID)
{
	size_t levSize = 0, vrmSize = 0;
	unsigned char *lev = TrackMap_ReadFile(levelID, NATIVE_TRACK_FILE_LEV, &levSize);
	unsigned char *vrm = lev != NULL ? TrackMap_ReadFile(levelID, NATIVE_TRACK_FILE_VRM, &vrmSize) : NULL;
	int ok = lev != NULL && vrm != NULL && NativeTrackMap_Decode(out, lev, levSize, vrm, vrmSize);
	free(lev);
	free(vrm);
	return ok;
}
