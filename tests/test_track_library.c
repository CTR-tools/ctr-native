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

static void DetectWrite32(unsigned char *p, uint32_t value)
{
	for (int i = 0; i < 4; i++)
	{
		p[i] = (unsigned char)(value >> (8 * i));
	}
}

static void TestDetectModes(void)
{
	CTR_TEST_SECTION("LEV mode detection");
	unsigned char bytes[4 + 0x500] = {0};
	unsigned char *p = bytes + 4;
	DetectWrite32(bytes, 0x500);
	DetectWrite32(p + 0x0c, 7);
	DetectWrite32(p + 0x10, 0x180);
	const unsigned int ids[] = {0x08, 0x07, 0x5c, 0x93, 0x94, 0x95, 0x60};
	for (unsigned int i = 0; i < SDL_arraysize(ids); i++)
	{
		unsigned int model = 0x380 + i * 0x18;
		DetectWrite32(p + 0x180 + i * 0x40 + 0x10, model);
		p[model + 0x10] = (unsigned char)ids[i];
	}
	unsigned int modes = 0;
	CTR_TEST_CHECK(TrackDetect_Lev(bytes, sizeof(bytes), &modes) && (modes & NATIVE_TRACK_DETECTED_VS) &&
	               !(modes & NATIVE_TRACK_DETECTED_ARCADE), "a track without bot paths allows VS but not arcade");
	DetectWrite32(p + 0x188, 0x440);
	DetectWrite32(p + 0x440, 0x450);
	p[0x450] = 0xfd;
	p[0x451] = 0xec;
	p[0x452] = 2;
	CTR_TEST_CHECK(TrackDetect_Lev(bytes, sizeof(bytes), &modes) && modes == 63,
	               "a valid nonempty bot path identifies arcade as well as VS");
	p[0x451] = 0;
	CTR_TEST_CHECK(TrackDetect_Lev(bytes, sizeof(bytes), &modes) && !(modes & NATIVE_TRACK_DETECTED_ARCADE),
	               "outdated bot paths do not imply arcade");
	p[0x451] = 0xec;
	p[0x380 + 5 * 0x18 + 0x10] = 0;
	CTR_TEST_CHECK(TrackDetect_Lev(bytes, sizeof(bytes), &modes) && !(modes & NATIVE_TRACK_DETECTED_CTR),
	               "a partial set of letters does not imply a CTR challenge");
	p[0x380 + 1 * 0x18 + 0x10] = 0;
	CTR_TEST_CHECK(TrackDetect_Lev(bytes, sizeof(bytes), &modes) && (modes & NATIVE_TRACK_DETECTED_ARCADE),
	               "a bot path allows arcade without Wumpa pickups");
	DetectWrite32(p + 0x0c, 0);
	CTR_TEST_CHECK(TrackDetect_Lev(bytes, sizeof(bytes), &modes) && (modes & NATIVE_TRACK_DETECTED_VS) &&
	               (modes & NATIVE_TRACK_DETECTED_ARCADE), "a bot path allows arcade and VS without any placed instances");
	DetectWrite32(p + 0x0c, 0xffffffffu);
	CTR_TEST_CHECK(!TrackDetect_Lev(bytes, sizeof(bytes), &modes), "an overflowing instance array is unknown");
	DetectWrite32(p + 0x0c, 1);
	DetectWrite32(p + 0x190, 0x4ff);
	CTR_TEST_CHECK(!TrackDetect_Lev(bytes, sizeof(bytes), &modes), "an out-of-bounds model is unknown");
	DetectWrite32(p + 0x188, 0x4ff);
	CTR_TEST_CHECK(!TrackDetect_Lev(bytes, sizeof(bytes), &modes), "a truncated bot table is unknown");
	CTR_TEST_CHECK(!TrackDetect_Lev(bytes, 10, &modes), "a truncated header is unknown");
}

int main(void)
{
	TestDetectModes();
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

	WriteFixture(track, "track.ini", "id=test:track\ntitle=Test Track\nauthor=Track Author\nmodes=time_trial,arcade\n");
	registry = NativeTrackLibrary_Scan(root, &result);
	metadata = NativeTrackRegistry_Get(registry, NATIVE_TRACK_CUSTOM_ID_BASE);
	CTR_TEST_CHECK(metadata != NULL && strcmp(metadata->identity, "test:track") == 0 &&
	               strcmp(metadata->title, "Test Track") == 0 && strcmp(metadata->author, "Track Author") == 0 &&
	               strcmp(metadata->modes, "time_trial,arcade") == 0,
	               "cached catalog metadata overrides local defaults");
	CTR_TEST_CHECK(NativeTrackLibrary_IsEnabled(metadata), "tracks are enabled by default");
	CTR_TEST_CHECK(NativeTrackLibrary_AllowsTimeTrial(metadata), "source tags containing time_trial allow time trial selection");
	struct NativeTrackMetadata arcadeOnly = *metadata;
	arcadeOnly.modes = "arcade, relic_race";
	CTR_TEST_CHECK(!NativeTrackLibrary_AllowsTimeTrial(&arcadeOnly), "explicit tags without time_trial exclude the track from time trial");
	arcadeOnly.modes = NULL;
	CTR_TEST_CHECK(NativeTrackLibrary_AllowsTimeTrial(&arcadeOnly), "unknown source modes preserve time trial availability");
	arcadeOnly.detectionKnown = 1;
	arcadeOnly.detectedModes = NATIVE_TRACK_DETECTED_VS;
	CTR_TEST_CHECK(NativeTrackLibrary_AllowsDetectedMode(&arcadeOnly, NATIVE_TRACK_DETECTED_VS) &&
	               !NativeTrackLibrary_AllowsDetectedMode(&arcadeOnly, NATIVE_TRACK_DETECTED_ARCADE),
	               "a track without bot paths allows VS but not arcade");
	arcadeOnly.detectedModes |= NATIVE_TRACK_DETECTED_ARCADE;
	CTR_TEST_CHECK(NativeTrackLibrary_AllowsDetectedMode(&arcadeOnly, NATIVE_TRACK_DETECTED_ARCADE),
	               "detected bot paths allow arcade");
	arcadeOnly.detectionKnown = 0;
	CTR_TEST_CHECK(NativeTrackLibrary_AllowsDetectedMode(&arcadeOnly, NATIVE_TRACK_DETECTED_VS) &&
	               !NativeTrackLibrary_AllowsDetectedMode(&arcadeOnly, NATIVE_TRACK_DETECTED_ARCADE),
	               "unknown compatibility permits VS but does not confirm arcade");
	CTR_TEST_CHECK(NativeTrackLibrary_SetEnabled(metadata, 0) && !NativeTrackLibrary_IsEnabled(metadata), "a track can be disabled");
	NativeTrackRegistry_Destroy(registry);
	registry = NativeTrackLibrary_Scan(root, &result);
	metadata = NativeTrackRegistry_Get(registry, NATIVE_TRACK_CUSTOM_ID_BASE);
	CTR_TEST_CHECK(metadata != NULL && !NativeTrackLibrary_IsEnabled(metadata), "disabled state survives a library rescan");
	CTR_TEST_CHECK(NativeTrackLibrary_SetEnabled(metadata, 1) && NativeTrackLibrary_IsEnabled(metadata), "a disabled track can be re-enabled");
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
	char newTrack[1100];
	snprintf(newTrack, sizeof(newTrack), "%s/aaa-new-track", root);
	SDL_CreateDirectory(newTrack);
	WriteFixture(newTrack, "track.lev", "not LEV");
	WriteFixture(newTrack, "track.vrm", "not VRM");
	WriteFixture(track, "track.ini", "title=Updated Track\n");
	CTR_TEST_CHECK(NativeTrackLibrary_Refresh(root, &result) &&
	               NativeTrackRegistry_Count(NativeTrackLibrary_Current()) == 2 &&
	               NativeTrackRegistry_Find(NativeTrackLibrary_Current(), "local:aaa-new-track", NULL),
	               "a refresh includes newly imported tracks");
	int refreshedID = -1;
	NativeTrackRegistry_Find(NativeTrackLibrary_Current(), "local:track", &refreshedID);
	metadata = NativeTrackRegistry_Get(NativeTrackLibrary_Current(), refreshedID);
	CTR_TEST_CHECK(metadata != NULL && strcmp(metadata->title, "Updated Track") == 0,
	               "a refresh replaces stale metadata with the current disk contents");
	const char *newFiles[] = {"track.lev", "track.vrm"};
	for (size_t i = 0; i < SDL_arraysize(newFiles); i++)
	{
		snprintf(path, sizeof(path), "%s/%s", newTrack, newFiles[i]);
		SDL_RemovePath(path);
	}
	SDL_RemovePath(newTrack);
	CTR_TEST_CHECK(NativeTrackLibrary_Refresh(root, &result) && NativeTrackRegistry_Count(NativeTrackLibrary_Current()) == 1,
	               "a refresh removes entries deleted from disk");
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
