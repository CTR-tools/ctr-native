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

	WriteFixture(track, "track.ini", "id=test:track\ntitle=Test Track\nauthor=Track Author\n");
	registry = NativeTrackLibrary_Scan(root, &result);
	metadata = NativeTrackRegistry_Get(registry, NATIVE_TRACK_CUSTOM_ID_BASE);
	CTR_TEST_SECTION("music selection persistence");
	CTR_TEST_CHECK(NativeTrackLibrary_Music(metadata) == -1,
	               "missing selection retains the original audio fallback");
	CTR_TEST_CHECK(NativeTrackLibrary_SetMusic(metadata, 7, 0), "retail selection is written");
	struct NativeTrackRegistry *withMusic = NativeTrackLibrary_Scan(root, NULL);
	const struct NativeTrackMetadata *saved = NativeTrackRegistry_Get(withMusic, NATIVE_TRACK_CUSTOM_ID_BASE);
	CTR_TEST_CHECK(NativeTrackLibrary_Music(saved) == 7, "retail selection is read from the refreshed registry");
	CTR_TEST_CHECK(NativeTrackLibrary_SetMusic(saved, -1, 1), "custom selection replaces retail");
	NativeTrackRegistry_Destroy(withMusic);
	withMusic = NativeTrackLibrary_Scan(root, NULL);
	saved = NativeTrackRegistry_Get(withMusic, NATIVE_TRACK_CUSTOM_ID_BASE);
	CTR_TEST_CHECK(saved != NULL && strcmp(saved->title, "Test Track") == 0 && saved->author != NULL &&
	               strcmp(saved->author, "Track Author") == 0 &&
	               saved->music != NULL && strcmp(saved->music, "custom") == 0,
	               "music is stored in a valid manifest without losing the track metadata");
	NativeTrackRegistry_Destroy(withMusic);
	CTR_TEST_CHECK(!NativeTrackLibrary_SetMusic(metadata, 18, 0), "invalid retail selection is rejected");
	CTR_TEST_CHECK(metadata != NULL && strcmp(metadata->identity, "test:track") == 0 &&
	               strcmp(metadata->title, "Test Track") == 0 && strcmp(metadata->author, "Track Author") == 0,
	               "cached catalog metadata overrides local defaults");
	CTR_TEST_CHECK(NativeTrackLibrary_IsEnabled(metadata), "tracks are enabled by default");
	CTR_TEST_CHECK(NativeTrackLibrary_AllowsDetectedMode(metadata, NATIVE_TRACK_DETECTED_TIME_TRIAL),
	               "time trial is available even when detection is unknown");
	struct NativeTrackMetadata arcadeOnly = *metadata;
	arcadeOnly.detectionKnown = 1;
	arcadeOnly.detectedModes = NATIVE_TRACK_DETECTED_VS;
	CTR_TEST_CHECK(NativeTrackLibrary_AllowsDetectedMode(&arcadeOnly, NATIVE_TRACK_DETECTED_TIME_TRIAL),
	               "time trial is available even if detection does not report it");
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
	CTR_TEST_SECTION("unrecognized music fallback");
	const char *invalidMusic[] = {"unknown", "retail:18", "retail:abc", ""};
	for (size_t i = 0; i < SDL_arraysize(invalidMusic); i++)
	{
		char manifest[128];
		snprintf(manifest, sizeof(manifest), "title=Test Track\nmusic=%s\n", invalidMusic[i]);
		WriteFixture(track, "track.ini", manifest);
		registry = NativeTrackLibrary_Scan(root, &result);
		metadata = NativeTrackRegistry_Get(registry, NATIVE_TRACK_CUSTOM_ID_BASE);
		CTR_TEST_CHECK(metadata != NULL && NativeTrackLibrary_Music(metadata) == -1 && metadata->scaPath == NULL,
		               "an unrecognized music value keeps the track and falls back without custom audio");
		NativeTrackRegistry_Destroy(registry);
	}
	WriteFixture(track, "track.ini", "title=Test Track\nmusic=unknown\n");
	WriteFixture(track, "track.sca", "not SCA");
	registry = NativeTrackLibrary_Scan(root, &result);
	metadata = NativeTrackRegistry_Get(registry, NATIVE_TRACK_CUSTOM_ID_BASE);
	CTR_TEST_CHECK(metadata != NULL && NativeTrackLibrary_Music(metadata) == -1 && metadata->scaPath != NULL,
	               "an unrecognized music value retains the local custom audio fallback");
	NativeTrackRegistry_Destroy(registry);
	snprintf(path, sizeof(path), "%s/track.sca", track);
	SDL_RemovePath(path);
	WriteFixture(track, "track.ini", "id=test:track\nmodes=arcade\n");
	registry = NativeTrackLibrary_Scan(root, &result);
	metadata = NativeTrackRegistry_Get(registry, NATIVE_TRACK_CUSTOM_ID_BASE);
	CTR_TEST_CHECK(metadata == NULL && result.rejected == 1, "the manifest cannot declare track modes");
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
	CTR_TEST_SECTION("installed track editing and deletion");
	SDL_CreateDirectory(newTrack);
	WriteFixture(newTrack, "track.lev", "not LEV");
	WriteFixture(newTrack, "track.vrm", "not VRM");
	WriteFixture(track, "track.ini", "title=Updated Track\nmusic=custom:local:aaa-new-track\n");
	WriteFixture(newTrack, "track.ini", "title=Other\nmusic=custom:local:track\n");
	CTR_TEST_CHECK(NativeTrackLibrary_Refresh(root, NULL), "invalid music selections are repaired during refresh");
	int trackID = 0, sourceID = 0;
	NativeTrackRegistry_Find(NativeTrackLibrary_Current(), "local:track", &trackID);
	NativeTrackRegistry_Find(NativeTrackLibrary_Current(), "local:aaa-new-track", &sourceID);
	metadata = NativeTrackRegistry_Get(NativeTrackLibrary_Current(), trackID);
	const struct NativeTrackMetadata *source = NativeTrackRegistry_Get(NativeTrackLibrary_Current(), sourceID);
	CTR_TEST_CHECK(strcmp(metadata->music, "custom") == 0 && NativeTrackLibrary_Music(source) == NATIVE_TRACK_DEFAULT_RETAIL_MUSIC,
	               "invalid references fall back to local SCA or Crash Cove");
	struct NativeTrackRegistry *reloaded = NativeTrackLibrary_Scan(root, NULL);
	int reloadedTrack, reloadedSource;
	NativeTrackRegistry_Find(reloaded, "local:track", &reloadedTrack);
	NativeTrackRegistry_Find(reloaded, "local:aaa-new-track", &reloadedSource);
	CTR_TEST_CHECK(strcmp(NativeTrackRegistry_Get(reloaded, reloadedTrack)->music, "custom") == 0 &&
	               NativeTrackLibrary_Music(NativeTrackRegistry_Get(reloaded, reloadedSource)) == NATIVE_TRACK_DEFAULT_RETAIL_MUSIC,
	               "both fallback choices are persisted to the manifests");
	NativeTrackRegistry_Destroy(reloaded);
	CTR_TEST_CHECK(NativeTrackLibrary_SetDetails(metadata, "New Name", "New Author"), "name and author can be saved");
	CTR_TEST_CHECK(!NativeTrackLibrary_SetDetails(metadata, " ", "Author"), "an empty name is rejected");
	CTR_TEST_CHECK(NativeTrackLibrary_Refresh(root, NULL), "edited details can be reloaded");
	NativeTrackRegistry_Find(NativeTrackLibrary_Current(), "local:track", &trackID);
	metadata = NativeTrackRegistry_Get(NativeTrackLibrary_Current(), trackID);
	CTR_TEST_CHECK(strcmp(metadata->title, "New Name") == 0 && strcmp(metadata->author, "New Author") == 0 &&
	               strcmp(metadata->music, "custom") == 0,
	               "editing details preserves the custom music choice");
	/* SDL's enumeration callback supplies a trailing separator; old scans could
	   consequently have doubled or mixed separators in the stored LEV path. */
	char oldPath[1200];
	snprintf(oldPath, sizeof(oldPath), "%s\\/track/track.lev", root);
	struct NativeTrackMetadata oldScan = *metadata;
	oldScan.levPath = oldPath;
	CTR_TEST_CHECK(NativeTrackLibrary_Delete(&oldScan, root), "a track from an older mixed-separator scan can be deleted");
	SDL_PathInfo deletedInfo;
	CTR_TEST_CHECK(!SDL_GetPathInfo(track, &deletedInfo) && NativeTrackRegistry_Count(NativeTrackLibrary_Current()) == 1,
	               "deletion removes the directory and updates the published library");
	const char *sourceFiles[] = {"track.lev", "track.vrm", "track.ini"};
	for (size_t i = 0; i < SDL_arraysize(sourceFiles); i++)
	{
		snprintf(path, sizeof(path), "%s/%s", newTrack, sourceFiles[i]);
		SDL_RemovePath(path);
	}
	SDL_RemovePath(newTrack);
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
