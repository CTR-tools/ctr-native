#include <shared/ctr_test_scratch.h>
#include <platform/native_track_registry.c>
#include <platform/native_track_validate.c>
#include <platform/native_track_file.c>

int main(void)
{
	char directory[1024], path[1200];
	CtrTestScratch_Open(directory, sizeof(directory), "test-track-file", "track.lev");
	snprintf(path, sizeof(path), "%strack.lev", directory);
	FILE *fixture = fopen(path, "wb");
	/* Eight-byte payload, followed by a one-entry pointer map. */
	const unsigned char lev[] = {8, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0};
	if (fixture == NULL || fwrite(lev, 1, sizeof(lev), fixture) != sizeof(lev) || fclose(fixture) != 0)
	{
		CtrTest_Bail("could not create track fixture");
	}
	struct NativeTrackRegistry *registry = NativeTrackRegistry_Create();
	struct NativeTrackMetadata metadata = {"local:one", "Track", path, path, NULL, NULL};
	int id = 0;
	CTR_TEST_SECTION("on-demand track file reads");
	CTR_TEST_CHECK(registry != NULL && NativeTrackRegistry_Add(registry, &metadata, &id), "track metadata can be registered");
	struct NativeTrackFile *file = NativeTrackFile_Open(registry, id, NATIVE_TRACK_FILE_LEV);
	CTR_TEST_CHECK(file != NULL && NativeTrackFile_Size(file) == sizeof(lev) && NativeTrackFile_SectorBytes(file) == 2048,
	               "opening exposes the actual and sector-rounded sizes");
	unsigned char bytes[2049];
	memset(bytes, 0xaa, sizeof(bytes));
	CTR_TEST_CHECK(!NativeTrackFile_Read(file, bytes, 2047) && bytes[0] == 0xaa, "an undersized buffer is not written");
	CTR_TEST_CHECK(NativeTrackFile_Read(file, bytes, sizeof(bytes)), "the selected LEV is read");
	int zeroPadding = 1;
	for (size_t i = sizeof(lev); i < 2048; i++)
	{
		if (bytes[i] != 0)
		{
			zeroPadding = 0;
		}
	}
	CTR_TEST_CHECK(memcmp(bytes, lev, sizeof(lev)) == 0 && zeroPadding && bytes[2048] == 0xaa,
	               "sector padding is zeroed without writing past the rounded size");
	struct NativeTrackFile *wrongRole = NativeTrackFile_Open(registry, id, NATIVE_TRACK_FILE_VRM);
	CTR_TEST_CHECK(wrongRole != NULL && !NativeTrackFile_Read(wrongRole, bytes, sizeof(bytes)),
	               "LEV bytes are refused when requested as a VRM");
	NativeTrackFile_Close(wrongRole);
	CTR_TEST_CHECK(NativeTrackFile_Open(registry, id, NATIVE_TRACK_FILE_SCA) == NULL, "missing optional audio cannot be opened");
	CTR_TEST_CHECK(NativeTrackFile_Open(registry, 6, NATIVE_TRACK_FILE_LEV) == NULL, "a retail ID cannot resolve a custom file");
	NativeTrackFile_Close(file);
	NativeTrackRegistry_Destroy(registry);
	CtrTestScratch_Remove("test-track-file", "track.lev");
	return CtrTest_Summary();
}
