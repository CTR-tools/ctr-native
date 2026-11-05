#include <shared/ctr_test.h>
#include <platform/native_track_file.h>
#include <platform/native_track_library.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const struct NativeTrackRegistry *NativeTrackLibrary_Current(void) { return NULL; }
struct NativeTrackFile *NativeTrackFile_Open(const struct NativeTrackRegistry *r, int id, enum NativeTrackFileRole role)
{ (void)r; (void)id; (void)role; return NULL; }
size_t NativeTrackFile_Size(const struct NativeTrackFile *f) { (void)f; return 0; }
size_t NativeTrackFile_SectorBytes(const struct NativeTrackFile *f) { (void)f; return 0; }
int NativeTrackFile_Read(struct NativeTrackFile *f, void *p, size_t n) { (void)f; (void)p; (void)n; return 0; }
void NativeTrackFile_Close(struct NativeTrackFile *f) { (void)f; }

#include <platform/native_track_validate.c>
#include <platform/native_track_map.c>

static void Write32(unsigned char *p, uint32_t n)
{
	for (int i = 0; i < 4; i++) { p[i] = (unsigned char)(n >> (i * 8)); }
}

static unsigned char *ReadFile(const char *path, size_t *size)
{
	FILE *f = fopen(path, "rb");
	if (f == NULL || fseek(f, 0, SEEK_END) != 0) { if (f != NULL) { fclose(f); } return NULL; }
	long n = ftell(f);
	if (n <= 0 || n > TRACK_MAP_MAX_FILE || fseek(f, 0, SEEK_SET) != 0) { fclose(f); return NULL; }
	unsigned char *p = malloc((size_t)n);
	if (p == NULL || fread(p, 1, (size_t)n, f) != (size_t)n) { free(p); fclose(f); return NULL; }
	fclose(f);
	*size = (size_t)n;
	return p;
}

int main(int argc, char **argv)
{
	struct NativeTrackMap map;
	if (argc == 3)
	{
		size_t ls = 0, vs = 0;
		unsigned char *lev = ReadFile(argv[1], &ls), *vrm = ReadFile(argv[2], &vs);
		int ok = NativeTrackMap_Decode(&map, lev, ls, vrm, vs);
		printf("map=%d size=%dx%d\n", ok, map.width, map.height);
		if (ok)
		{
			int visible = 0, runs = 0;
			for (int y = 0; y < map.height; y++)
			{
				int last = 0;
				for (int x = 0; x < map.width; x++)
				{
					int pixel = map.pixels[y * NATIVE_TRACK_MAP_TEXTURE_SIZE + x] != 0;
					visible += pixel;
					if (pixel && !last) { runs++; }
					last = pixel;
				}
			}
			printf("visible pixels=%d silhouette runs=%d\n", visible, runs);
		}
		free(lev); free(vrm);
		return ok ? 0 : 1;
	}
	CTR_TEST_SECTION("custom minimap from bounded LEV and VRM");
	unsigned char lev[4 + NATIVE_TRACK_LEV_HEADER_MIN_BYTES + 4] = {0}, vrm[128] = {0};
	Write32(lev, NATIVE_TRACK_LEV_HEADER_MIN_BYTES);
	Write32(lev + 4 + 0x3c, 64);
	Write32(lev + 4 + 64, 2);
	Write32(lev + 4 + 68, 96);
	unsigned char *top = lev + 4 + 96, *bottom = top + 32;
	Write32(top + 16, 3); Write32(bottom + 16, 4);
	/* Two 4x1 icons in 4-bit mode, with palette row y=2. */
	top[20 + 2] = bottom[20 + 2] = 128;
	top[20 + 4] = bottom[20 + 4] = 4;
	top[20 + 9] = 1;
	bottom[20 + 1] = 1; bottom[20 + 9] = 2;
	Write32(vrm, 0x20); Write32(vrm + 4, 28);
	vrm[8 + 16] = 1; vrm[8 + 18] = 2;
	vrm[8 + 20] = 0x10; vrm[8 + 21] = 0x10;
	vrm[8 + 22] = 0x01; vrm[8 + 23] = 0x01;
	Write32(vrm + 36, 52);
	vrm[40 + 14] = 2; vrm[40 + 16] = 16; vrm[40 + 18] = 1;
	vrm[40 + 22] = 0xff; vrm[40 + 23] = 0x7f;
	CTR_TEST_CHECK(NativeTrackMap_Decode(&map, lev, sizeof(lev), vrm, sizeof(vrm)) && map.width == 4 && map.height == 2,
	               "both icon halves form one map");
	CTR_TEST_CHECK(!map.pixels[0] && map.pixels[1] == 0x7fff && map.pixels[3] == 0x7fff &&
	               map.pixels[NATIVE_TRACK_MAP_TEXTURE_SIZE] == 0x7fff && !map.pixels[NATIVE_TRACK_MAP_TEXTURE_SIZE + 1],
	               "4-bit texture indices and the palette determine transparency");
	vrm[40 + 21] = 0x80;
	CTR_TEST_CHECK(NativeTrackMap_Decode(&map, lev, sizeof(lev), vrm, sizeof(vrm)) && !map.pixels[0] && map.pixels[1] == 0x7fff,
	               "black STP background is transparent in the layered menu map");
	vrm[40 + 21] = 0;
	Write32(bottom + 16, 3);
	CTR_TEST_CHECK(!NativeTrackMap_Decode(&map, lev, sizeof(lev), vrm, sizeof(vrm)), "ambiguous icon IDs are rejected");
	Write32(bottom + 16, 4);
	Write32(lev + 4 + 68, UINT32_MAX);
	CTR_TEST_CHECK(!NativeTrackMap_Decode(&map, lev, sizeof(lev), vrm, sizeof(vrm)), "out-of-range icon pointers are rejected");
	Write32(lev + 4 + 68, 96);
	vrm[40 + 22] = 0; vrm[40 + 23] = 0;
	CTR_TEST_CHECK(!NativeTrackMap_Decode(&map, lev, sizeof(lev), vrm, sizeof(vrm)), "an entirely transparent map is not shown");
	vrm[40 + 22] = 0xff; vrm[40 + 23] = 0x7f;
	Write32(vrm + 36, UINT32_MAX);
	CTR_TEST_CHECK(!NativeTrackMap_Decode(&map, lev, sizeof(lev), vrm, sizeof(vrm)), "a truncated VRM image is rejected before decoding");
	uint16_t *vram = calloc(TRACK_MAP_VRAM_W * TRACK_MAP_VRAM_H, sizeof(*vram));
	unsigned char *present = calloc(TRACK_MAP_VRAM_W * TRACK_MAP_VRAM_H, 1);
	if (vram == NULL || present == NULL) { CtrTest_Bail("cannot allocate VRAM fixture"); }
	unsigned char layout[12] = {0};
	layout[6] = 0x80; /* 8-bit texture */
	vram[0] = 0x0201; present[0] = 1;
	vram[1] = 0x7fff; present[1] = 1;
	vram[2] = 0; present[2] = 1;
	uint16_t color = 0;
	CTR_TEST_CHECK(TrackMap_Pixel(vram, present, layout, 0, 0, &color) && color == 0x7fff,
	               "8-bit textures read their palette index");
	CTR_TEST_CHECK(TrackMap_Pixel(vram, present, layout, 1, 0, &color) && color == 0,
	               "8-bit textures preserve transparent palette entries");
	layout[7] = 1; layout[6] = 0; /* 16-bit texture */
	CTR_TEST_CHECK(TrackMap_Pixel(vram, present, layout, 0, 0, &color) && color == 0x0201,
	               "16-bit textures use their direct pixel value");
	present[0] = 0;
	CTR_TEST_CHECK(!TrackMap_Pixel(vram, present, layout, 0, 0, &color), "missing VRAM data is not mistaken for transparent pixels");
	free(vram); free(present);
	return CtrTest_Summary();
}
