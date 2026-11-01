#include <shared/ctr_test_scratch.h>

#include <platform/native_track_registry.c>
#include <platform/native_track_library.c>

static void WriteFixture(const char *directory, const char *name, const char *text)
{
	char path[1200];
	int length = snprintf(path, sizeof(path), "%s/%s", directory, name);
	if (length < 0 || (size_t)length >= sizeof(path))
	{
		CtrTest_Bail("fixture path is too long");
	}
	FILE *file = fopen(path, "wb");
	if (file == NULL || fputs(text, file) == EOF || fclose(file) != 0)
	{
		CtrTest_Bail("could not write fixture");
	}
}

int main(void)
{
	char root[1024], track[1100], path[1200];
	CtrTestScratch_Open(root, sizeof(root), "test-track-library", NULL);
	snprintf(track, sizeof(track), "%s/track", root);
	if (!SDL_CreateDirectory(track))
	{
		CtrTest_Bail("could not create track fixture");
	}
	WriteFixture(track, "track.lev", "not a LEV");
	WriteFixture(track, "track.vrm", "not a VRM");

	CTR_TEST_SECTION("scan reads metadata, not track contents");
	struct NativeTrackScanResult result;
	struct NativeTrackRegistry *registry = NativeTrackLibrary_Scan(root, &result);
	CTR_TEST_CHECK(registry != NULL && result.installed == 1 && result.rejected == 0, "a directory with required files is indexed");
	int id = 0;
	CTR_TEST_CHECK(NativeTrackRegistry_Find(registry, "local:track", &id), "the directory supplies a local identity without a manifest");
	const struct NativeTrackMetadata *metadata = NativeTrackRegistry_Get(registry, id);
	CTR_TEST_CHECK(metadata != NULL && strcmp(metadata->title, "track") == 0 && metadata->scaPath == NULL && metadata->author == NULL,
	               "directory name and music use defaults");
	NativeTrackRegistry_Destroy(registry);

	WriteFixture(track, "track.ini", "id=saphi:1\ntitle=Test Track\nauthor=Track Author\n");
	registry = NativeTrackLibrary_Scan(root, &result);
	metadata = NativeTrackRegistry_Get(registry, NATIVE_TRACK_CUSTOM_ID_BASE);
	CTR_TEST_CHECK(metadata != NULL && strcmp(metadata->identity, "saphi:1") == 0 &&
	               strcmp(metadata->title, "Test Track") == 0 && strcmp(metadata->author, "Track Author") == 0,
	               "cached catalog metadata overrides local defaults");
	NativeTrackRegistry_Destroy(registry);

	WriteFixture(track, "track.ini", "title=First\ntitle=Second\n");
	registry = NativeTrackLibrary_Scan(root, &result);
	CTR_TEST_CHECK(registry != NULL && result.rejected == 1, "duplicate metadata is refused");
	NativeTrackRegistry_Destroy(registry);

	WriteFixture(track, "track.ini", "lev=../race.lev\n");
	registry = NativeTrackLibrary_Scan(root, &result);
	CTR_TEST_CHECK(registry != NULL && result.rejected == 1, "cache cannot redirect track files");
	NativeTrackRegistry_Destroy(registry);
	WriteFixture(track, "track.ini", "title=Test Track\n");
	WriteFixture(track, "track.sca", "not SCA");
	registry = NativeTrackLibrary_Scan(root, &result);
	metadata = NativeTrackRegistry_Get(registry, NATIVE_TRACK_CUSTOM_ID_BASE);
	CTR_TEST_CHECK(metadata != NULL && metadata->scaPath != NULL, "optional audio is indexed");
	NativeTrackRegistry_Destroy(registry);

	CTR_TEST_SECTION("published library lifecycle");
	CTR_TEST_CHECK(NativeTrackLibrary_Refresh(root, &result), "a successful scan publishes the library");
	const struct NativeTrackRegistry *current = NativeTrackLibrary_Current();
	CTR_TEST_CHECK(current != NULL && NativeTrackRegistry_Find(current, "local:track", NULL), "the published library is available to runtime consumers");
	CTR_TEST_CHECK(!NativeTrackLibrary_Refresh("", &result) && NativeTrackLibrary_Current() == current,
	               "a failed refresh preserves the previous library");
	NativeTrackLibrary_Shutdown();
	CTR_TEST_CHECK(NativeTrackLibrary_Current() == NULL, "shutdown releases the published library");

	const char *files[] = {"track.ini", "track.lev", "track.vrm", "track.sca"};
	for (size_t i = 0; i < SDL_arraysize(files); i++)
	{
		snprintf(path, sizeof(path), "%s/%s", track, files[i]);
		SDL_RemovePath(path);
	}
	SDL_RemovePath(track);
	CtrTestScratch_Remove("test-track-library", NULL);
	return CtrTest_Summary();
}
