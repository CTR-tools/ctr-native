#include <shared/ctr_test.h>
#include <platform/native_track_validate.c>
#include <string.h>

static void Write32(unsigned char *p, uint32_t value)
{
	p[0] = (unsigned char)value;
	p[1] = (unsigned char)(value >> 8);
	p[2] = (unsigned char)(value >> 16);
	p[3] = (unsigned char)(value >> 24);
}

int main(void)
{
	CTR_TEST_SECTION("LEV pointer map");
	unsigned char lev[4 + NATIVE_TRACK_LEV_HEADER_MIN_BYTES + 8] = {0};
	unsigned char empty[8] = {0};
	CTR_TEST_CHECK(!NativeTrackValidate_Lev(empty, sizeof(empty)), "an empty payload with an empty map is rejected");
	Write32(lev, NATIVE_TRACK_LEV_HEADER_MIN_BYTES - 1);
	CTR_TEST_CHECK(!NativeTrackValidate_Lev(lev, sizeof(lev)), "a map overlapping the last byte of the level header is rejected");
	Write32(lev, NATIVE_TRACK_LEV_HEADER_MIN_BYTES);
	Write32(lev + 4, 4);
	Write32(lev + 4 + NATIVE_TRACK_LEV_HEADER_MIN_BYTES, 4);
	Write32(lev + 8 + NATIVE_TRACK_LEV_HEADER_MIN_BYTES, 0);
	CTR_TEST_CHECK(NativeTrackValidate_Lev(lev, sizeof(lev)), "a complete minimum header and bounded pointer map are accepted");
	Write32(lev + 8 + NATIVE_TRACK_LEV_HEADER_MIN_BYTES, NATIVE_TRACK_LEV_HEADER_MIN_BYTES);
	CTR_TEST_CHECK(!NativeTrackValidate_Lev(lev, sizeof(lev)), "a pointer slot outside the payload is rejected");
	Write32(lev + 8 + NATIVE_TRACK_LEV_HEADER_MIN_BYTES, 0);
	Write32(lev + 4, NATIVE_TRACK_LEV_HEADER_MIN_BYTES);
	CTR_TEST_CHECK(!NativeTrackValidate_Lev(lev, sizeof(lev)), "a relocated target outside the payload is rejected");
	Write32(lev + 4 + NATIVE_TRACK_LEV_HEADER_MIN_BYTES, 100);
	CTR_TEST_CHECK(!NativeTrackValidate_Lev(lev, sizeof(lev)), "a truncated map is rejected");

	CTR_TEST_SECTION("VRM image bounds");
	unsigned char vrm[36] = {0};
	Write32(vrm, 0x20);
	Write32(vrm + 4, 24);
	vrm[8 + 16] = 2;
	vrm[8 + 18] = 1;
	CTR_TEST_CHECK(NativeTrackValidate_Vrm(vrm, sizeof(vrm)), "a bounded multi-image VRM is accepted");
	CTR_TEST_CHECK(!NativeTrackValidate_Vrm(vrm, 33), "a missing terminator is rejected");
	vrm[8 + 16] = 3;
	CTR_TEST_CHECK(!NativeTrackValidate_Vrm(vrm, sizeof(vrm)), "pixel data shorter than the rectangle is rejected");
	vrm[8 + 16] = 2;
	vrm[8 + 12] = 0xff;
	vrm[8 + 13] = 0x03;
	CTR_TEST_CHECK(!NativeTrackValidate_Vrm(vrm, sizeof(vrm)), "a rectangle beyond VRAM is rejected");
	return CtrTest_Summary();
}
