#include <platform/native_track_preview.h>
#include <platform/native_track_library.h>
#include <psx/libgpu.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "native_track_jpeg_decoder.h"

enum
{
	NATIVE_TRACK_PREVIEW_PATH_BYTES = 1024,
	NATIVE_TRACK_PREVIEW_FILE_LIMIT = 4 * 1024 * 1024,
	/* The 512x216 menu is presented at 4:3, so its pixels are not square. */
	NATIVE_TRACK_PREVIEW_PIXEL_ASPECT_NUM = 9,
	NATIVE_TRACK_PREVIEW_PIXEL_ASPECT_DEN = 16,
	NATIVE_TRACK_PREVIEW_FACTOR_SCALE = 1000,
};

int NativeTrackPreview_Load(int levelID, int x, int y, int width, int height, int widescreenFactor)
{
	const struct NativeTrackMetadata *track = NativeTrackRegistry_Get(NativeTrackLibrary_Current(), levelID);
	if (track == NULL || width <= 0 || height <= 0 || widescreenFactor <= 0 || x < 0 || y < 0 || x + width > 1024 || y + height > 512)
	{
		return 0;
	}
	const char *slash = strrchr(track->vrmPath, '/');
	const char *backslash = strrchr(track->vrmPath, '\\');
	if (backslash != NULL && (slash == NULL || backslash > slash))
	{
		slash = backslash;
	}
	if (slash == NULL)
	{
		return 0;
	}
	char path[NATIVE_TRACK_PREVIEW_PATH_BYTES];
	size_t directoryLength = (size_t)(slash - track->vrmPath + 1);
	if (directoryLength + sizeof("preview.jpg") > sizeof(path))
	{
		return 0;
	}
	memcpy(path, track->vrmPath, directoryLength);
	memcpy(path + directoryLength, "preview.jpg", sizeof("preview.jpg"));
	FILE *file = fopen(path, "rb");
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
	if (size <= 0 || size > NATIVE_TRACK_PREVIEW_FILE_LIMIT || fseek(file, 0, SEEK_SET) != 0)
	{
		fclose(file);
		return 0;
	}
	unsigned char *bytes = malloc((size_t)size);
	if (bytes == NULL)
	{
		fclose(file);
		return 0;
	}
	int readOk = fread(bytes, 1, (size_t)size, file) == (size_t)size;
	fclose(file);
	int imageWidth, imageHeight, components;
	if (!readOk)
	{
		free(bytes);
		return 0;
	}
	unsigned char *pixels = stbi_load_from_memory(bytes, (int)size, &imageWidth, &imageHeight, &components, 3);
	free(bytes);
	if (pixels == NULL)
	{
		return 0;
	}
	u16 *vram = malloc((size_t)width * height * sizeof(*vram));
	if (vram == NULL)
	{
		stbi_image_free(pixels);
		return 0;
	}
	/* Fit the whole image, accounting for presentation and non-square pixels. */
	long long pixelWidth = NATIVE_TRACK_PREVIEW_PIXEL_ASPECT_NUM * NATIVE_TRACK_PREVIEW_FACTOR_SCALE;
	long long pixelHeight = (long long)NATIVE_TRACK_PREVIEW_PIXEL_ASPECT_DEN * widescreenFactor;
	int drawWidth = width;
	int drawHeight = height;
	if ((long long)imageWidth * height * pixelHeight > (long long)imageHeight * width * pixelWidth)
	{
		drawHeight = (int)((long long)width * pixelWidth * imageHeight / (pixelHeight * imageWidth));
	}
	else
	{
		drawWidth = (int)((long long)height * pixelHeight * imageWidth / (pixelWidth * imageHeight));
	}
	if (drawWidth < 1)
	{
		drawWidth = 1;
	}
	if (drawHeight < 1)
	{
		drawHeight = 1;
	}
	int offsetX = (width - drawWidth) / 2;
	int offsetY = (height - drawHeight) / 2;
	for (int row = 0; row < height; row++)
	{
		for (int column = 0; column < width; column++)
		{
			if (column < offsetX || column >= offsetX + drawWidth || row < offsetY || row >= offsetY + drawHeight)
			{
				vram[row * width + column] = 0x8000;
				continue;
			}
			int sourceX = (column - offsetX) * imageWidth / drawWidth;
			int sourceY = (row - offsetY) * imageHeight / drawHeight;
			const unsigned char *pixel = pixels + (sourceY * imageWidth + sourceX) * 3;
			vram[row * width + column] = (u16)((pixel[0] >> 3) | ((pixel[1] >> 3) << 5) | ((pixel[2] >> 3) << 10) | 0x8000);
		}
	}
	RECT16 rect = {(s16)x, (s16)y, (s16)width, (s16)height};
	LoadImage(&rect, vram);
	free(vram);
	stbi_image_free(pixels);
	return 1;
}
