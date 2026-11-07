#include "shared/ctr_test_scratch.h"
#include <platform/native_disc_image.c>

static void TestDisc_WriteLE32(u8 *dst, u32 value)
{
	dst[0] = (u8)value;
	dst[1] = (u8)(value >> 8);
	dst[2] = (u8)(value >> 16);
	dst[3] = (u8)(value >> 24);
}

static void TestDisc_Sector(FILE *file, u32 lba, const u8 *data)
{
	u8 raw[2352] = {0};
	memset(&raw[1], 0xff, 10);
	raw[15] = 2;
	memcpy(&raw[24], data, 2048);
	if (fseek(file, (long)(lba * sizeof(raw)), SEEK_SET) != 0 || fwrite(raw, 1, sizeof(raw), file) != sizeof(raw))
	{
		CtrTest_Bail("could not write test image");
	}
}

static void TestDisc_Create(const char *dir, const char *name, u32 fileLba, u8 marker)
{
	char path[1024];
	u8 data[2048] = {0};
	snprintf(path, sizeof(path), "%s%s", dir, name);
	FILE *file = fopen(path, "wb");
	if (file == NULL)
	{
		CtrTest_Bail("could not create test image");
	}
	data[0] = 1;
	memcpy(&data[1], "CD001", 5);
	data[6] = 1;
	data[156] = 34;
	TestDisc_WriteLE32(&data[158], 17);
	TestDisc_WriteLE32(&data[166], 2048);
	data[181] = 2;
	data[188] = 1;
	TestDisc_Sector(file, 16, data);
	memset(data, 0, sizeof(data));
	data[0] = 43;
	TestDisc_WriteLE32(&data[2], fileLba);
	TestDisc_WriteLE32(&data[10], 2048);
	data[32] = 10;
	memcpy(&data[33], "FILE.BIN;1", 10);
	TestDisc_Sector(file, 17, data);
	memset(data, marker, sizeof(data));
	TestDisc_Sector(file, fileLba, data);
	fclose(file);
}

int main(void)
{
	char dir[768];
	char path[1024];
	struct NativeDiscImageFile base = {0}, europe = {0};
	u8 data[2048];
	CtrTestScratch_Open(dir, sizeof(dir), "test-disc-images", NULL);
	TestDisc_Create(dir, "ctr-u.bin", 18, 'U');
	TestDisc_Create(dir, "ctr-e.bin", 19, 'E');
	CTR_TEST_SECTION("independent disc sources");
	CTR_TEST_CHECK(NativeDiscImage_Init(dir), "base image initializes");
	CTR_TEST_CHECK(NativeDiscImage_FindFile("FILE.BIN", &base), "legacy lookup uses the base image");
	CTR_TEST_CHECK(NativeDiscImage_FindFileOn(NATIVE_DISC_IMAGE_EUROPE, "FILE.BIN", &europe), "European image has its own directory");
	CTR_TEST_CHECK(base.lba == 18 && europe.lba == 19, "each image resolves its own file location");
	CTR_TEST_CHECK(NativeDiscImage_ReadDataSectorsOn(NATIVE_DISC_IMAGE_EUROPE, &europe, 0, 1, data) && data[0] == 'E', "European reads use European sectors");
	CTR_TEST_CHECK(NativeDiscImage_ReadDataSectors(&base, 0, 1, data) && data[0] == 'U', "base reads remain independent after European reads");
	CTR_TEST_CHECK(!NativeDiscImage_FindFileOn(NATIVE_DISC_IMAGE_COUNT, "FILE.BIN", &base), "invalid image IDs are rejected");
	NativeDiscImage_Init(NULL);
	snprintf(path, sizeof(path), "%sctr-e.bin", dir);
	remove(path);
	CTR_TEST_CHECK(NativeDiscImage_Init(dir), "base image initializes without the optional European image");
	CTR_TEST_CHECK(!NativeDiscImage_FindFileOn(NATIVE_DISC_IMAGE_EUROPE, "FILE.BIN", &europe), "missing European image is unavailable");
	NativeDiscImage_Init(NULL);
	CtrTestScratch_Remove("test-disc-images", "ctr-u.bin");
	return CtrTest_Summary();
}
