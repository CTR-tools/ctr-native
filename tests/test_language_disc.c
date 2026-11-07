#include "shared/ctr_test.h"
#include <platform/native_disc_image.h>
#include <stdlib.h>
#include <string.h>

static u8 s_header[2048];
static u8 s_text[2048];
static int s_europePresent = 1;
static u32 s_generation;
static int s_textReads;
static u8 s_howl[8192];

u32 NativeDiscImage_Generation(void) { return s_generation; }

int NativeDiscImage_FindFileOn(enum NativeDiscImageId id, const char *path, struct NativeDiscImageFile *out)
{
	if (id != NATIVE_DISC_IMAGE_EUROPE || !s_europePresent)
	{
		return 0;
	}
	if (strcmp(path, "SOUNDS/KART.HWL") == 0)
	{
		*out = (struct NativeDiscImageFile){.lba = 200, .size = sizeof(s_howl)};
		return 1;
	}
	if (strcmp(path, "BIGFILE.BIG") != 0) { return 0; }
	*out = (struct NativeDiscImageFile){.lba = 100, .size = 4096};
	return 1;
}

int NativeDiscImage_ReadDataSectorsOn(enum NativeDiscImageId id, const struct NativeDiscImageFile *file, u32 sector, u32 count, void *dst)
{
	if (id == NATIVE_DISC_IMAGE_EUROPE && file->lba == 200 && sector <= 4 && count <= 4 - sector)
	{
		memcpy(dst, s_howl + sector * 2048, count * 2048);
		return 1;
	}
	if (id != NATIVE_DISC_IMAGE_EUROPE || file->lba != 100 || count != 1 || sector > 1)
	{
		return 0;
	}
	memcpy(dst, sector == 0 ? s_header : s_text, 2048);
	s_textReads++;
	return 1;
}

int NativeDiscImage_ReadFileBytesOn(enum NativeDiscImageId id, const char *path, int raw, u8 **bytes, int *size)
{
	(void)id;
	(void)path;
	(void)raw;
	*bytes = NULL;
	*size = 0;
	return 0;
}

#include <platform/native_language_disc.c>

static void TestLanguage_WriteLE32(u8 *p, u32 value)
{
	p[0] = (u8)value;
	p[1] = (u8)(value >> 8);
	p[2] = (u8)(value >> 16);
	p[3] = (u8)(value >> 24);
}

int main(void)
{
	u8 *bytes = NULL;
	u32 size = 0;
	u8 *entry = &s_header[8 + (234 + 5) * 8];
	TestLanguage_WriteLE32(s_header + 4, 242);
	TestLanguage_WriteLE32(entry, 1);
	TestLanguage_WriteLE32(entry + 4, 16);
	TestLanguage_WriteLE32(s_text, 1);
	TestLanguage_WriteLE32(s_text + 4, 12);
	memcpy(s_text + 8, "CIA", 4);
	TestLanguage_WriteLE32(s_text + 12, 8);
	CTR_TEST_SECTION("European text records");
	CTR_TEST_CHECK(NativeLanguageDisc_PalXaIndex(2, 0) == 0 && NativeLanguageDisc_PalXaIndex(2, 19) == 16 &&
	               NativeLanguageDisc_PalXaIndex(2, 200) == 170 && NativeLanguageDisc_PalXaIndex(2, 300) == 255,
	               "character voice IDs use the corresponding PAL character block");
	CTR_TEST_CHECK(NativeLanguageDisc_PalXaIndex(2, 16) == -1 && NativeLanguageDisc_PalXaIndex(2, 303) == -1 &&
	               NativeLanguageDisc_PalXaIndex(2, 189) == -1 && NativeLanguageDisc_PalXaIndex(2, 190) == -1,
	               "voices absent or unused in PAL use the English fallback");
	CTR_TEST_CHECK(NativeLanguageDisc_PalXaIndex(1, 0x3d) == 0x3d && NativeLanguageDisc_PalXaIndex(1, 0x3e) == -1 &&
	               NativeLanguageDisc_PalXaIndex(1, 0x40) == 0x3e && NativeLanguageDisc_PalXaIndex(1, 0x52) == 0x50 &&
	               NativeLanguageDisc_PalXaIndex(1, 0x53) == -1, "EXTRA dialogue maps around PAL omissions");
	CTR_TEST_CHECK(NativeLanguageDisc_PalXaIndex(2, 0x130) == 0x100 && NativeLanguageDisc_PalXaIndex(2, 0x131) == 0x101 &&
	               NativeLanguageDisc_PalXaIndex(2, 0x132) == -1 && NativeLanguageDisc_PalXaIndex(2, 0x134) == 0x102 &&
	               NativeLanguageDisc_PalXaIndex(2, 0x139) == 0x107, "boss cutscene dialogue maps to PAL tracks");
	CTR_TEST_CHECK(NativeLanguageDisc_PalIndex(546) == 546 && NativeLanguageDisc_PalIndex(547) == 548 &&
	               NativeLanguageDisc_PalIndex(548) == 550 && NativeLanguageDisc_PalIndex(553) == 556 &&
	               NativeLanguageDisc_PalIndex(587) == 590, "PAL-only controller strings are skipped at each insertion");
	CTR_TEST_CHECK(NativeLanguageDisc_ReadText(5, &bytes, &size) && size == 16 && strcmp((char *)bytes + 8, "CIA") == 0,
	               "a valid Italian record is read from the European bigfile");
	free(bytes);
	int reads = s_textReads;
	CTR_TEST_CHECK(NativeLanguageDisc_HasText(5) && NativeLanguageDisc_HasText(5) && s_textReads == reads + 2,
	               "repeated availability checks reuse the validated text result");
	s_generation++;
	CTR_TEST_CHECK(NativeLanguageDisc_HasText(5) && s_textReads == reads + 4,
	               "reinitializing the disc invalidates cached text availability");
	s_europePresent = 0;
	s_generation++;
	CTR_TEST_CHECK(!NativeLanguageDisc_HasText(5), "a removed image is not reported as available after reinitialization");
	s_europePresent = 1;
	s_generation++;
	CTR_TEST_CHECK(NativeLanguageDisc_HasText(5), "a restored image is detected after reinitialization");
	CTR_TEST_CHECK(!NativeLanguageDisc_ReadText(1, &bytes, &size), "English is not sourced from the European image");
	CTR_TEST_CHECK(strcmp(NativeLanguageDisc_TextString(s_text, 16, 0), "CIA") == 0 &&
	               NativeLanguageDisc_TextString(s_text, 16, 1) == NULL,
	               "raw language strings are resolved with bounds checks");
	TestLanguage_WriteLE32(entry + 4, 32 + (LNG_ITALIAN + 1) * 4);
	TestLanguage_WriteLE32(s_text, LNG_ITALIAN + 1);
	TestLanguage_WriteLE32(s_text + 4, 32);
	for (int i = 0; i <= LNG_ITALIAN; i++)
	{
		TestLanguage_WriteLE32(s_text + 32 + i * 4, 8);
	}
	s_generation++;
	CTR_TEST_CHECK(strcmp(NativeLanguageDisc_SelfName(5), "CIA") == 0,
	               "the Italian name comes from the Italian disc table");
	s_europePresent = 0;
	s_generation++;
	CTR_TEST_CHECK(NativeLanguageDisc_SelfName(5) == NULL, "disc replacement invalidates cached names");
	s_europePresent = 1;
	TestLanguage_WriteLE32(entry + 4, 16);
	TestLanguage_WriteLE32(s_text, 1);
	TestLanguage_WriteLE32(s_text + 4, 12);
	TestLanguage_WriteLE32(s_text + 12, 16);
	CTR_TEST_CHECK(!NativeLanguageDisc_ReadText(5, &bytes, &size), "out-of-range string offsets are rejected");
	TestLanguage_WriteLE32(s_text + 12, 8);
	TestLanguage_WriteLE32(entry + 4, 0xffffffffu);
	CTR_TEST_CHECK(!NativeLanguageDisc_ReadText(5, &bytes, &size), "oversized entries are rejected before allocation");
	TestLanguage_WriteLE32(entry + 4, 16);
	CTR_TEST_SECTION("European sampled voices");
	TestLanguage_WriteLE32(s_howl, 0x4c574f48);
	TestLanguage_WriteLE32(s_howl + 4, 128);
	TestLanguage_WriteLE32(s_howl + 16, 1);
	TestLanguage_WriteLE32(s_howl + 20, 428);
	TestLanguage_WriteLE32(s_howl + 28, 1);
	TestLanguage_WriteLE32(s_howl + 36, 4096 - 40);
	s_howl[42] = 4; // sample size, in eight-byte units
	u8 *fx = s_howl + 44 + 326 * 8;
	fx[0] = 4; fx[1] = 70; fx[3] = 8; fx[6] = 123;
	u8 *bankOffset = s_howl + 44 + 428 * 8;
	bankOffset[0] = 2;
	s_howl[4096] = 1; // one sample, index zero
	memset(s_howl + 6144, 0x5a, 32);
	struct NativeLanguageVoiceSample sample;
	CTR_TEST_CHECK(NativeLanguageDisc_ReadVoiceSample(5, 28, &sample) && sample.size == 32 && sample.data[0] == 0x5a &&
	               sample.pitch == 2048 && sample.duration == 123, "Italian voice data and metadata come from the European HOWL bank");
	free(sample.data);
	s_howl[0] = 0;
	CTR_TEST_CHECK(!NativeLanguageDisc_ReadVoiceSample(5, 28, &sample) && sample.data == NULL,
	               "a malformed HOWL header is rejected without producing a sample");
	s_howl[0] = 'H';
	CTR_TEST_CHECK(!NativeLanguageDisc_ReadVoiceSample(1, 28, &sample), "English keeps its base-disc sample");
	CTR_TEST_CHECK(!NativeLanguageDisc_ReadVoiceSample(5, 60, &sample), "unrelated effects do not use language samples");
	s_europePresent = 0;
	CTR_TEST_CHECK(!NativeLanguageDisc_ReadText(5, &bytes, &size) && bytes == NULL && size == 0,
	               "a missing European image leaves no result");
	return CtrTest_Summary();
}
