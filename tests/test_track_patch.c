#include <shared/ctr_test_scratch.h>
#include <platform/native_track_registry.c>
#include <platform/native_track_library.c>
#include <platform/native_track_validate.c>
#include <platform/native_track_file.c>
#include <platform/native_track_import.c>
#include <platform/native_track_patch.c>

static void PatchTest_Write(const char *path, const void *data, size_t size)
{
	FILE *file = fopen(path, "wb");
	if (file == NULL || fwrite(data, 1, size, file) != size || fclose(file) != 0)
	{
		CtrTest_Bail("cannot write patch fixture");
	}
}

static void PatchTest_U32(unsigned char *out, uint32_t value)
{
	for (int i = 0; i < 4; i++)
	{
		out[i] = (unsigned char)(value >> (8 * i));
	}
}

static void PatchTest_Disc(const char *root)
{
	CTR_TEST_SECTION("isolated ROM extraction");
	unsigned char image[24 * PATCH_DISC_RAW_BYTES] = {0};
	for (int i = 0; i < 24; i++)
	{
		memset(image + i * PATCH_DISC_RAW_BYTES + 1, 255, 10);
		image[i * PATCH_DISC_RAW_BYTES + 15] = 2;
	}
	unsigned char *pvd = image + 16 * PATCH_DISC_RAW_BYTES + PATCH_DISC_DATA_OFFSET;
	pvd[0] = 1;
	pvd[6] = 1;
	memcpy(pvd + 1, "CD001", 5);
	pvd[156] = 34;
	pvd[181] = 2;
	PatchTest_U32(pvd + 158, 20);
	PatchTest_U32(pvd + 166, PATCH_DISC_DATA_BYTES);
	unsigned char *record = image + 20 * PATCH_DISC_RAW_BYTES + PATCH_DISC_DATA_OFFSET;
	record[0] = 46;
	record[32] = 13;
	memcpy(record + 33, "BIGFILE.BIG;1", 13);
	PatchTest_U32(record + 2, 21);
	PatchTest_U32(record + 10, 3000);
	memset(image + 21 * PATCH_DISC_RAW_BYTES + PATCH_DISC_DATA_OFFSET, 42, PATCH_DISC_DATA_BYTES);
	memset(image + 22 * PATCH_DISC_RAW_BYTES + PATCH_DISC_DATA_OFFSET, 43, PATCH_DISC_DATA_BYTES);
	char disc[PATCH_PATH_BYTES], output[PATCH_PATH_BYTES];
	TrackPatch_Path(disc, root, "fixture.bin");
	TrackPatch_Path(output, root, "extracted.big");
	PatchTest_Write(disc, image, sizeof(image));
	SDL_AtomicInt cancel = {0};
	CTR_TEST_CHECK(PatchDisc_ExtractBig(disc, output, &cancel), "a raw MODE2 image yields its BIGFILE without mounting the game CD");
	unsigned char extracted[3001] = {0};
	FILE *file = fopen(output, "rb");
	size_t read = file != NULL ? fread(extracted, 1, sizeof(extracted), file) : 0;
	if (file != NULL) { fclose(file); }
	CTR_TEST_CHECK(read == 3000 && extracted[0] == 42 && extracted[2047] == 42 && extracted[2048] == 43 &&
	               extracted[2999] == 43, "extraction strips sector headers and respects the exact ISO file length");
	SDL_SetAtomicInt(&cancel, 1);
	CTR_TEST_CHECK(!PatchDisc_ExtractBig(disc, output, &cancel), "ROM extraction observes cancellation");
	SDL_SetAtomicInt(&cancel, 0);
	record[32] = 200;
	PatchTest_Write(disc, image, sizeof(image));
	CTR_TEST_CHECK(!PatchDisc_ExtractBig(disc, output, &cancel), "an out-of-bounds ISO name is refused");
	unsigned char big[PATCH_DISC_DATA_BYTES * 2] = {0};
	PatchTest_U32(big + 4, 1);
	PatchTest_U32(big + 8, 1);
	PatchTest_U32(big + 12, 3);
	memcpy(big + PATCH_DISC_DATA_BYTES, "LOD", 3);
	PatchTest_Write(disc, big, sizeof(big));
	CTR_TEST_CHECK(TrackPatch_BigEntry(disc, 0, output), "a referenced BIGFILE entry can be extracted independently of its LOD name");
	size_t size = 0;
	char *data = SDL_LoadFile(output, &size);
	CTR_TEST_CHECK(data != NULL && size == 3 && memcmp(data, "LOD", 3) == 0,
	               "BIGFILE entry extraction respects its exact length");
	SDL_free(data);
	CTR_TEST_CHECK(!TrackPatch_BigEntry(disc, 1, output), "a BIGFILE index outside the table is rejected");
	PatchTest_U32(big + 12, PATCH_DISC_DATA_BYTES + 1);
	PatchTest_Write(disc, big, sizeof(big));
	CTR_TEST_CHECK(!TrackPatch_BigEntry(disc, 0, output), "a BIGFILE entry outside the archive is rejected");
}

static void PatchTest_Review(const char *root, const char *source)
{
	CTR_TEST_SECTION("prepared patch review transaction");
	char assets[PATCH_PATH_BYTES], work[PATCH_PATH_BYTES], prepared[PATCH_PATH_BYTES], library[PATCH_PATH_BYTES];
	TrackPatch_Path(assets, root, "review-assets");
	TrackPatch_Path(work, assets, ".import-review");
	TrackPatch_Path(prepared, work, "prepared");
	TrackPatch_Path(library, assets, "tracks");
	SDL_CreateDirectory(prepared);
	CTR_TEST_CHECK(NativeTrackImport_Directory(prepared, source, "candidate") == NULL,
	               "a validated candidate can be prepared outside the live library");
	memset(&s_trackPatchJob, 0, sizeof(s_trackPatchJob));
	s_trackPatchJob.reviewReady = 1;
	SDL_strlcpy(s_trackPatchJob.assets, assets, sizeof(s_trackPatchJob.assets));
	SDL_strlcpy(s_trackPatchJob.work, work, sizeof(s_trackPatchJob.work));
	SDL_PathInfo info;
	CTR_TEST_CHECK(NativeTrackPatch_ReviewDirectory() != NULL && !SDL_GetPathInfo(library, &info),
	               "prepared candidates are visible to review but not installed");
	CTR_TEST_CHECK(NativeTrackPatch_ReviewDiscard() && !SDL_GetPathInfo(work, &info) && !SDL_GetPathInfo(library, &info),
	               "discard removes only the temporary import and does not create the library");
	SDL_CreateDirectory(prepared);
	CTR_TEST_CHECK(NativeTrackImport_Directory(prepared, source, "candidate") == NULL,
	               "a second candidate can be prepared after discard");
	CTR_TEST_CHECK(NativeTrackImport_Directory(prepared, source, "other") == NULL,
	               "a second candidate is available for music exchange");
	CTR_TEST_CHECK(NativeTrackImport_Directory(prepared, source, "discard") == NULL,
	               "a third candidate can be excluded while retaining its music");
	char sca[PATCH_PATH_BYTES];
	TrackPatch_Path(sca, prepared, "candidate/track.sca");
	PatchTest_Write(sca, "first", 5);
	TrackPatch_Path(sca, prepared, "other/track.sca");
	PatchTest_Write(sca, "second", 6);
	TrackPatch_Path(sca, prepared, "discard/track.sca");
	PatchTest_Write(sca, "excluded music", 14);
	s_trackPatchJob.reviewReady = 1;
	SDL_strlcpy(s_trackPatchJob.work, work, sizeof(s_trackPatchJob.work));
	CTR_TEST_CHECK(NativeTrackPatch_ReviewSetMusic("local:candidate", "local:discard") &&
	               NativeTrackPatch_ReviewSetMusic("local:other", "local:candidate"),
	               "review can select music from another candidate in both directions");
	CTR_TEST_CHECK(NativeTrackPatch_ReviewSetIncluded("local:discard", 0) &&
	               !NativeTrackPatch_ReviewIncluded("local:discard"), "a candidate can be excluded without deleting its files");
	CTR_TEST_CHECK(NativeTrackPatch_ReviewSetIncluded("local:candidate", 0) && NativeTrackPatch_ReviewSetIncluded("local:other", 0) &&
	               NativeTrackPatch_ReviewCommit() != NULL && NativeTrackPatch_ReviewDirectory() != NULL,
	               "publishing no tracks fails while retaining the review");
	CTR_TEST_CHECK(NativeTrackPatch_ReviewSetIncluded("local:candidate", 1) && NativeTrackPatch_ReviewSetIncluded("local:other", 1),
	               "excluded candidates can be reselected");
	CTR_TEST_CHECK(NativeTrackPatch_ReviewCommit() == NULL && SDL_GetPathInfo(library, &info) && !SDL_GetPathInfo(work, &info),
	               "confirm publishes the candidate and removes the temporary import");
	TrackPatch_Path(sca, library, "candidate/track.sca");
	size_t musicSize = 0;
	char *musicBytes = SDL_LoadFile(sca, &musicSize);
	CTR_TEST_CHECK(musicBytes != NULL && musicSize == 14 && memcmp(musicBytes, "excluded music", 14) == 0,
	               "music from an excluded track is copied into the selected candidate directory");
	SDL_free(musicBytes);
	TrackPatch_Path(sca, library, "discard");
	CTR_TEST_CHECK(!SDL_GetPathInfo(sca, &info), "the excluded candidate is not published");
	TrackPatch_Path(sca, library, "other/track.sca");
	musicBytes = SDL_LoadFile(sca, &musicSize);
	CTR_TEST_CHECK(musicBytes != NULL && musicSize == 5 && memcmp(musicBytes, "first", 5) == 0,
	               "crossed selections do not overwrite their staged sources");
	SDL_free(musicBytes);
	struct NativeTrackRegistry *published = NativeTrackLibrary_Scan(library, NULL);
	int candidateLevel;
	const struct NativeTrackMetadata *candidate = NativeTrackRegistry_Find(published, "local:candidate", &candidateLevel) ?
	    NativeTrackRegistry_Get(published, candidateLevel) : NULL;
	CTR_TEST_CHECK(candidate != NULL && candidate->music != NULL && strcmp(candidate->music, "custom") == 0,
	               "the installed manifest stores only custom, not the source identity");
	NativeTrackRegistry_Destroy(published);
	CTR_TEST_CHECK(NativeTrackPatch_ReviewDirectory() == NULL, "a committed review cannot be committed twice");
	memset(&s_trackPatchJob, 0, sizeof(s_trackPatchJob));
	TrackPatch_Path(assets, root, "folder-review-assets");
	SDL_CreateDirectory(assets);
	TrackPatch_Path(library, assets, "tracks");
	CTR_TEST_CHECK(NativeTrackPatch_PrepareFolder(assets, source) == NULL && NativeTrackPatch_ReviewDirectory() != NULL &&
	               !SDL_GetPathInfo(library, &info), "folder imports also prepare without publishing");
	CTR_TEST_CHECK(NativeTrackPatch_ReviewDiscard() && SDL_GetPathInfo(source, &info) && !SDL_GetPathInfo(library, &info),
	               "discarding a folder import preserves the user's source");
	CTR_TEST_CHECK(NativeTrackPatch_PrepareFolder(assets, source) == NULL, "a discarded folder can be prepared again");
	struct NativeTrackRegistry *registry = NativeTrackLibrary_Scan(NativeTrackPatch_ReviewDirectory(), NULL);
	const struct NativeTrackMetadata *track = NativeTrackRegistry_Get(registry, NATIVE_TRACK_CUSTOM_ID_BASE);
	CTR_TEST_CHECK(NativeTrackLibrary_SetMusic(track, -1, 1), "review saves music in the candidate manifest");
	char identity[256];
	SDL_strlcpy(identity, track != NULL ? track->identity : "", sizeof(identity));
	NativeTrackRegistry_Destroy(registry);
	CTR_TEST_CHECK(NativeTrackPatch_ReviewUpdate(identity, "Edited Circuit", "Example Author") == NULL,
	               "review can edit candidate title and attribution");
	registry = NativeTrackLibrary_Scan(NativeTrackPatch_ReviewDirectory(), NULL);
	track = NativeTrackRegistry_Get(registry, NATIVE_TRACK_CUSTOM_ID_BASE);
	CTR_TEST_CHECK(track != NULL && track->music != NULL && strcmp(track->music, "custom") == 0,
	               "editing metadata preserves the selected music");
	CTR_TEST_CHECK(track != NULL && strcmp(track->identity, identity) == 0 && strcmp(track->title, "Edited Circuit") == 0 &&
	               track->author != NULL && strcmp(track->author, "Example Author") == 0,
	               "editing preserves identity and persists both values in the staged metadata");
	NativeTrackRegistry_Destroy(registry);
	CTR_TEST_CHECK(NativeTrackPatch_ReviewUpdate(identity, "Renamed Circuit", "") == NULL,
	               "editing again replaces the staged metadata and may clear the author");
	registry = NativeTrackLibrary_Scan(NativeTrackPatch_ReviewDirectory(), NULL);
	track = NativeTrackRegistry_Get(registry, NATIVE_TRACK_CUSTOM_ID_BASE);
	CTR_TEST_CHECK(track != NULL && strcmp(track->title, "Renamed Circuit") == 0 && track->author == NULL,
	               "replacement keeps the manifest readable without an author");
	NativeTrackRegistry_Destroy(registry);
	CTR_TEST_CHECK(NativeTrackPatch_ReviewUpdate(identity, "Invalid\nTitle", "") != NULL,
	               "metadata cannot inject extra manifest lines");
	CTR_TEST_CHECK(NativeTrackPatch_ReviewUpdate(identity, "Title", "   ") != NULL,
	               "whitespace-only attribution cannot produce an invalid manifest");
	CTR_TEST_CHECK(NativeTrackPatch_ReviewCommitStart(), "confirmation starts a background installation");
	int result;
	while ((result = NativeTrackPatch_Poll()) == 0) { SDL_Delay(1); }
	struct NativeTrackRegistry *installed = NativeTrackPatch_TakeRegistry();
	CTR_TEST_CHECK(result == 1 && installed != NULL && NativeTrackRegistry_Count(installed) == 1 &&
	               SDL_GetPathInfo(library, &info), "background confirmation publishes and scans the folder import");
	NativeTrackRegistry_Destroy(installed);
	CTR_TEST_CHECK(NativeTrackPatch_PrepareFolder(assets, source) == NULL,
	               "another folder import can be reviewed after installation");
	const char *reviewDirectory = NativeTrackPatch_ReviewDirectory();
	registry = NativeTrackLibrary_Scan(reviewDirectory, NULL);
	track = NativeTrackRegistry_Get(registry, NATIVE_TRACK_CUSTOM_ID_BASE);
	CTR_TEST_CHECK(track != NULL && NativeTrackRegistry_Count(registry) == 1 &&
	               strncmp(track->levPath, reviewDirectory, strlen(reviewDirectory)) == 0 &&
	               strcmp(track->title, "Renamed Circuit") != 0,
	               "a later review scans the new temporary import, not the previously installed library");
	NativeTrackRegistry_Destroy(registry);
	CTR_TEST_CHECK(NativeTrackPatch_ReviewDiscard(), "the later review can be discarded independently");
	memset(&s_trackPatchJob, 0, sizeof(s_trackPatchJob));
}

static void PatchTest_Collect(const char *root)
{
	CTR_TEST_SECTION("detect only modified single-player circuit pairs");
	struct TrackPatchJob job = {0};
	SDL_strlcpy(job.name, "Example", sizeof(job.name));
	for (int side = 0; side < 2; side++)
	{
		for (int i = 0; i < PATCH_TRACK_COUNT; i++)
		{
			char path[PATCH_PATH_BYTES], relative[180], directory[PATCH_PATH_BYTES];
			SDL_snprintf(relative, sizeof(relative), "%s/levels/tracks/%s/1P", side ? "patched" : "original", s_patchTrackSlots[i]);
			TrackPatch_Path(directory, root, relative);
			SDL_CreateDirectory(directory);
			unsigned char lev[4 + NATIVE_TRACK_LEV_HEADER_MIN_BYTES + 8] = {0};
			lev[0] = (unsigned char)NATIVE_TRACK_LEV_HEADER_MIN_BYTES;
			lev[1] = (unsigned char)(NATIVE_TRACK_LEV_HEADER_MIN_BYTES >> 8);
			lev[4] = 4;
			lev[4 + NATIVE_TRACK_LEV_HEADER_MIN_BYTES] = 4;
			unsigned char vrm[36] = {0x20, 0, 0, 0, 24};
			vrm[24] = 2;
			vrm[26] = 1;
			if (side && i == 0) { lev[8] = 17; }
			if (side && i == 1) { vrm[28] = 77; }
			TrackPatch_Path(path, directory, "data.lev");
			PatchTest_Write(path, lev, sizeof(lev));
			TrackPatch_Path(path, directory, "data.vrm");
			PatchTest_Write(path, vrm, sizeof(vrm));
		}
	}
	char cloneSource[PATCH_PATH_BYTES], cloneTarget[PATCH_PATH_BYTES];
	TrackPatch_Path(cloneSource, root, "patched/levels/tracks/proto8/1P/data.lev");
	TrackPatch_Path(cloneTarget, root, "patched/levels/tracks/desert2/1P/data.lev");
	CTR_TEST_CHECK(SDL_CopyFile(cloneSource, cloneTarget), "duplicate fixture copies the modified LEV");
	TrackPatch_Path(cloneSource, root, "patched/levels/tracks/proto8/1P/data.vrm");
	TrackPatch_Path(cloneTarget, root, "patched/levels/tracks/desert2/1P/data.vrm");
	CTR_TEST_CHECK(SDL_CopyFile(cloneSource, cloneTarget), "duplicate fixture copies the matching VRM");
	int accepted[] = {0};
	CTR_TEST_CHECK(TrackPatch_DuplicatePair("", 2, accepted, 1) < 0, "unreadable comparisons are reported as errors");
	char patchedRoot[PATCH_PATH_BYTES];
	TrackPatch_Path(patchedRoot, root, "patched/levels/tracks");
	CTR_TEST_CHECK(TrackPatch_DuplicatePair(patchedRoot, 2, accepted, 1) == 1, "matching LEV and VRM are detected as duplicates");
	TrackPatch_Path(cloneTarget, root, "patched/levels/tracks/desert2/1P/data.vrm");
	unsigned char differentVrm[] = {1, 2, 3};
	PatchTest_Write(cloneTarget, differentVrm, sizeof(differentVrm));
	CTR_TEST_CHECK(TrackPatch_DuplicatePair(patchedRoot, 2, accepted, 1) == 0, "matching geometry with different textures remains distinct");
	CTR_TEST_CHECK(SDL_CopyFile(cloneSource, cloneTarget), "duplicate fixture restores the matching VRM");
	int count = 0;
	unsigned char credits[] = "created by Example Author";
	struct PatchMusic music = {.menu = {credits, sizeof(credits)}};
	const char *error = TrackPatch_Collect(&job, root, &count, &music);
	if (error != NULL) { printf("Discovery error: %s\n", error); }
	CTR_TEST_CHECK(error == NULL && count == 2, "identical LEV/VRM pairs are imported once while distinct pairs remain separate");
	char prepared[PATCH_PATH_BYTES];
	TrackPatch_Path(prepared, root, "prepared");
	char reviewSource[PATCH_PATH_BYTES];
	TrackPatch_Path(reviewSource, prepared, "Example-proto8");
	PatchTest_Review(root, reviewSource);
	char manifest[PATCH_PATH_BYTES];
	TrackPatch_Path(manifest, prepared, "Example-proto8/track.ini");
	SDL_PathInfo manifestInfo;
	CTR_TEST_CHECK(!SDL_GetPathInfo(manifest, &manifestInfo), "global credits are not attributed to multiple imported circuits");
	struct NativeTrackScanResult scan;
	struct NativeTrackRegistry *registry = NativeTrackLibrary_Scan(prepared, &scan);
	CTR_TEST_CHECK(registry != NULL && scan.installed == 2 && scan.rejected == 0,
	               "detected pairs pass through the existing validated local importer");
	NativeTrackRegistry_Destroy(registry);
	char library[PATCH_PATH_BYTES], source[PATCH_PATH_BYTES];
	TrackPatch_Path(library, root, "installed");
	TrackPatch_Path(source, prepared, "Example-proto9");
	CTR_TEST_CHECK(NativeTrackImport_Directory(library, source, "Example-proto9") == NULL,
	               "the publication conflict fixture is installed");
	CTR_TEST_CHECK(TrackPatch_InstallPrepared(&job, prepared, library) != NULL,
	               "an import cannot overwrite an installed circuit");
	registry = NativeTrackLibrary_Scan(library, &scan);
	CTR_TEST_CHECK(registry != NULL && scan.installed == 1 &&
	               NativeTrackRegistry_Find(registry, "local:Example-proto9", NULL),
	               "a multi-circuit conflict preserves existing content and rolls back newly published tracks");
	NativeTrackRegistry_Destroy(registry);
	SDL_SetAtomicInt(&job.cancel, 1);
	CTR_TEST_CHECK(TrackPatch_Collect(&job, root, &count, NULL) != NULL, "cancellation stops circuit discovery");
	SDL_SetAtomicInt(&job.cancel, 0);
	TrackPatch_RemoveTree(prepared);
	for (int i = 0; i < 3; i++)
	{
		char original[PATCH_PATH_BYTES], patched[PATCH_PATH_BYTES], relative[180];
		SDL_snprintf(relative, sizeof(relative), "original/levels/tracks/%s/1P/data.%s", s_patchTrackSlots[i], i == 1 ? "vrm" : "lev");
		TrackPatch_Path(original, root, relative);
		SDL_snprintf(relative, sizeof(relative), "patched/levels/tracks/%s/1P/data.%s", s_patchTrackSlots[i], i == 1 ? "vrm" : "lev");
		TrackPatch_Path(patched, root, relative);
		SDL_CopyFile(original, patched);
	}
	CTR_TEST_CHECK(TrackPatch_Collect(&job, root, &count, NULL) != NULL && count == 0,
	               "an unchanged ROM never installs retail circuits as custom content");
	char missing[PATCH_PATH_BYTES];
	TrackPatch_Path(missing, root, "patched/levels/tracks/proto8/1P/data.vrm");
	SDL_RemovePath(missing);
	CTR_TEST_CHECK(TrackPatch_Collect(&job, root, &count, NULL) != NULL, "an incomplete extracted pair is an error, not a guessed match");
}

static void PatchTest_Author(const char *root)
{
	CTR_TEST_SECTION("optional author from patch credits");
	char author[PATCH_AUTHOR_BYTES] = {0};
	const unsigned char absent[] = "unrelated text";
	CTR_TEST_CHECK(TrackPatch_FindAuthor(absent, sizeof(absent), author) && author[0] == 0,
	               "missing credits leave the author unset");
	const unsigned char embedded[] = "not created by Someone";
	CTR_TEST_CHECK(TrackPatch_FindAuthor(embedded, sizeof(embedded), author) && author[0] == 0,
	               "a substring inside another string is not treated as a credit");
	const unsigned char credit[] = "CREATED BY:  Example Author  ";
	CTR_TEST_CHECK(TrackPatch_FindAuthor(credit, sizeof(credit), author) && strcmp(author, "Example Author") == 0,
	               "a complete credit provides the author without surrounding spaces");
	const unsigned char same[] = "created by example author";
	CTR_TEST_CHECK(TrackPatch_FindAuthor(same, sizeof(same), author), "matching duplicate credits are unambiguous");
	const unsigned char different[] = "created by Someone Else";
	CTR_TEST_CHECK(!TrackPatch_FindAuthor(different, sizeof(different), author), "conflicting credits are not attributed");
	char directory[PATCH_PATH_BYTES], manifest[PATCH_PATH_BYTES];
	TrackPatch_Path(directory, root, "author-track");
	SDL_CreateDirectory(directory);
	char levPath[PATCH_PATH_BYTES];
	TrackPatch_Path(levPath, directory, "track.lev");
	PatchTest_Write(levPath, "LEV", 3);
	TrackPatch_Path(manifest, directory, "track.ini");
	struct PatchMusic noCredit = {.menu = {(unsigned char *)absent, sizeof(absent)}};
	SDL_PathInfo info;
	CTR_TEST_CHECK(TrackPatch_WriteAuthor(root, "author-track", &noCredit) == NULL && !SDL_GetPathInfo(manifest, &info),
	               "an absent credit does not create a manifest or set an author");
	struct PatchMusic music = {.menu = {(unsigned char *)credit, sizeof(credit)}};
	CTR_TEST_CHECK(TrackPatch_WriteAuthor(root, "author-track", &music) == NULL, "a valid credit can be saved in track metadata");
	size_t size = 0;
	char *content = SDL_LoadFile(manifest, &size);
	CTR_TEST_CHECK(content != NULL && strcmp(content, "author = Example Author\n") == 0,
	               "the generated manifest contains only the detected author");
	SDL_free(content);
	PatchTest_Write(manifest, "author = Existing\n", strlen("author = Existing\n"));
	CTR_TEST_CHECK(TrackPatch_WriteAuthor(root, "author-track", &music) == NULL, "existing metadata is preserved");
	content = SDL_LoadFile(manifest, &size);
	CTR_TEST_CHECK(content != NULL && strcmp(content, "author = Existing\n") == 0,
	               "a pre-existing author is not overwritten");
	SDL_free(content);
}

static SDL_EnumerationResult SDLCALL PatchTest_NoTemporary(void *userdata, const char *directory, const char *name)
{
	(void)directory;
	if (SDL_strncmp(name, ".import-", 8) == 0) { *(int *)userdata = 0; }
	return SDL_ENUM_CONTINUE;
}

static void PatchTest_Worker(const char *root)
{
	CTR_TEST_SECTION("background failure and cleanup");
	char patch[PATCH_PATH_BYTES];
	TrackPatch_Path(patch, root, "bad.xdelta");
	PatchTest_Write(patch, "bad", 3);
	CTR_TEST_CHECK(NativeTrackPatch_Start(root, patch), "a selected local patch starts a background job");
	CTR_TEST_CHECK(!NativeTrackPatch_Start(root, patch), "a second job cannot overwrite an active job");
	int result;
	do { result = NativeTrackPatch_Poll(); if (result == 0) { SDL_Delay(1); } } while (result == 0);
	CTR_TEST_CHECK(result < 0 && strstr(NativeTrackPatch_Message(), "Missing") != NULL,
	               "missing local tool dependencies are reported");
	int cleaned = 1;
	SDL_EnumerateDirectory(root, PatchTest_NoTemporary, &cleaned);
	CTR_TEST_CHECK(cleaned, "a failed worker removes its entire temporary directory");
	CTR_TEST_CHECK(!NativeTrackPatch_Start(root, "wrong.zip"), "only an actual xdelta selection is accepted");
	NativeTrackPatch_Shutdown();
}

static void PatchTest_Process(void)
{
	CTR_TEST_SECTION("tool process arguments and cancellation");
	struct TrackPatchJob job = {0};
	char executable[PATCH_PATH_BYTES];
#if defined(_WIN32)
	const char *name = "test_track_patch.exe";
#else
	const char *name = "test_track_patch";
#endif
	TrackPatch_Path(executable, SDL_GetBasePath(), name);
	const char *args[] = {executable, "--child", "path with spaces & symbols", NULL};
	CTR_TEST_CHECK(TrackPatch_Process(&job, args), "external tools receive a literal argv vector, including spaces and shell metacharacters");
	const char *wait[] = {executable, "--wait-child", NULL};
	SDL_SetAtomicInt(&job.cancel, 1);
	Uint64 start = SDL_GetTicks();
	CTR_TEST_CHECK(!TrackPatch_Process(&job, wait) && SDL_GetTicks() - start < 3000,
	               "cancelling kills and reaps a running tool before temporary cleanup");
}

static void PatchTest_Music(const char *root)
{
	CTR_TEST_SECTION("bounded patch music parsing");
	unsigned char bytes[64] = {0};
	struct PatchHowl howl = {.bytes = {bytes, sizeof(bytes)}};
	CTR_TEST_CHECK(!PatchMusic_Howl(&howl), "unrecognized HWL headers are rejected");
	memcpy(bytes, "HOWL", 4);
	PatchTest_U32(bytes + 16, UINT32_MAX);
	CTR_TEST_CHECK(!PatchMusic_Howl(&howl), "oversized HWL tables are rejected without overflow");
	unsigned char code[256] = {0};
	struct PatchMusicBytes menu = {code, sizeof(code)};
	/* lui s2,0x8001; addiu v0,zero,3; sb v0,0xab4(s2). */
	PatchTest_U32(code, UINT32_C(0x3c128001));
	PatchTest_U32(code + 4, UINT32_C(0x24020003));
	PatchTest_U32(code + 8, UINT32_C(0xa2420ab4));
	CTR_TEST_CHECK(PatchMusic_ConfigLevel(&menu, UINT32_C(0x80010ab4)) == 3,
	               "the menu's explicit configuration write supplies the runtime level");
	PatchTest_U32(code + 12, UINT32_C(0x24020002));
	PatchTest_U32(code + 16, UINT32_C(0xa2420ab4));
	CTR_TEST_CHECK(PatchMusic_ConfigLevel(&menu, UINT32_C(0x80010ab4)) < 0,
	               "conflicting runtime level writes are ambiguous rather than guessed");

	size_t exeSize = PATCH_MUSIC_SONG_TABLE + 32;
	unsigned char *retail = calloc(1, exeSize), *modified = calloc(1, exeSize);
	if (retail == NULL || modified == NULL) { CtrTest_Bail("cannot allocate executable fixture"); }
	memcpy(retail, "PS-X EXE", 8);
	PatchTest_U32(retail + 24, UINT32_C(0x80010000));
	PatchTest_U32(retail + 28, (uint32_t)exeSize - PATCH_MUSIC_EXE_HEADER);
	memcpy(modified, retail, exeSize);
	struct PatchMusicBytes originalExe = {retail, exeSize}, modifiedExe = {modified, exeSize};
	CTR_TEST_CHECK(PatchMusic_Level(&originalExe, &modifiedExe, &menu, 3) == 3,
	               "an unchanged retail loader uses the physical circuit level");
	size_t branch = PATCH_MUSIC_LOADER_START + 64;
	PatchTest_U32(modified + branch - 8, UINT32_C(0x8e041a10)); /* lw a0,levelID(s0) */
	PatchTest_U32(modified + branch - 4, UINT32_C(0x2403000e)); /* addiu v1,zero,14 */
	PatchTest_U32(modified + branch, UINT32_C(0x14830010)); /* bne a0,v1,after */
	PatchTest_U32(modified + branch + 8, UINT32_C(0x24060018));
	PatchTest_U32(modified + branch + 12, UINT32_C(0x24050003));
	PatchTest_U32(modified + branch + 16, UINT32_C(0x0c00cb4c));
	PatchTest_U32(modified + branch + 24, UINT32_C(0x24060019));
	PatchTest_U32(modified + branch + 28, UINT32_C(0x24050002));
	PatchTest_U32(modified + branch + 32, UINT32_C(0x0c00cb4c));
	CTR_TEST_CHECK(PatchMusic_Level(&originalExe, &modifiedExe, &menu, 3) == 14,
	               "an explicit TrackROM pair uses the compared logical level, not its storage slot");
	modified[0x1deec + 2048] = 1;
	CTR_TEST_CHECK(PatchMusic_Level(&originalExe, &modifiedExe, &menu, 3) < 0,
	               "a modified audio lookup routine is not interpreted as the retail table");
	struct PatchMusic selection = {.originalExe = originalExe, .exe = modifiedExe, .menu = menu, .available = 1};
	CTR_TEST_CHECK(PatchMusic_SelectLevel(&selection, 3) == 3,
	               "an unrecognized mapping falls back to the physical circuit slot");
	CTR_TEST_CHECK(PatchMusic_SelectLevel(&selection, 18) < 0, "out-of-range slots are not used as audio indices");
	unsigned char allBytes[0x1224 + 18] = {0};
	selection.all = (struct PatchMusicBytes){allBytes, sizeof(allBytes)};
	/* lbu index; lui/addiu table; addu address; lbu mapped level. */
	static const uint32_t mapping[] = {0x9222f320, 0x3c038080, 0x2463f224, 0x00431021, 0x90440000};
	for (size_t i = 0; i < SDL_arraysize(mapping); i++) { PatchTest_U32(modified + 0x3436c + i * 4, mapping[i]); }
	allBytes[0x1224 + 3] = 1;
	CTR_TEST_CHECK(PatchMusic_SelectLevel(&selection, 3) == 1, "recognized ALL.BIN mapping overrides the physical slot");
	allBytes[0x1224 + 3] = 18;
	CTR_TEST_CHECK(PatchMusic_SelectLevel(&selection, 3) == 3, "an invalid ALL.BIN level falls back to slot order");
	allBytes[0x1224 + 3] = 1;
	modified[0x3436f] ^= 0x80;
	CTR_TEST_CHECK(PatchMusic_SelectLevel(&selection, 3) == 3, "an altered loader signature does not trust the ALL.BIN table");

	CTR_TEST_SECTION("multi-track loader signatures");
	unsigned char *all = calloc(1, 0x2000);
	if (all == NULL) { CtrTest_Bail("cannot allocate multi-track fixture"); }
	static const uint32_t musicCode[] = {0x2c820013, 0x10400016, 0x3c02807f, 0x24421000,
	                                      0x00a22821, 0x90a30000, 0, 0x2c620012};
	static const uint32_t titleCode[] = {0x90830003, 0x3c02807f, 0x2c640012, 0x14800004,
	                                      0x24421100, 0x8c420048, 0x03e00008, 0};
	for (size_t i = 0; i < SDL_arraysize(musicCode); i++) { PatchTest_U32(all + 0x100 + i * 4, musicCode[i]); }
	for (size_t i = 0; i < SDL_arraysize(titleCode); i++) { PatchTest_U32(all + 0x200 + i * 4, titleCode[i]); }
	PatchTest_U32(all + 0x300, UINT32_C(0x0c1fc040)); /* jal 807f0100 */
	all[0x1000 + 3] = 14;
	PatchTest_U32(all + 0x1100 + 3 * 4, UINT32_C(0x807f1200));
	memcpy(all + 0x1200, "Custom Circuit", 15);
	struct PatchMusicBytes allLoader = {all, 0x2000};
	char title[64];
	CTR_TEST_CHECK(PatchMusic_AllBinTrack(&modifiedExe, &allLoader, 3, title, sizeof(title)) == 14 &&
	               strcmp(title, "Custom Circuit") == 0, "a recognized loader yields its own title and music mapping");
	all[0x1206] = '\n';
	CTR_TEST_CHECK(PatchMusic_AllBinTrack(&modifiedExe, &allLoader, 3, title, sizeof(title)) == 14 && title[0] == 0,
	               "invalid title bytes do not become metadata");
	all[0x100] ^= 1;
	CTR_TEST_CHECK(PatchMusic_AllBinTrack(&modifiedExe, &allLoader, 3, title, sizeof(title)) < 0,
	               "a changed music lookup is not interpreted as the recognized loader");
	free(all);

	memset(modified + PATCH_MUSIC_LOADER_START, 0, PATCH_MUSIC_LOADER_END - PATCH_MUSIC_LOADER_START);
	size_t init = 0x23e40, arrays = 0x24000;
	PatchTest_U32(modified + init, UINT32_C(0x24a53800));
	PatchTest_U32(modified + init + 16, UINT32_C(0x3c058003));
	PatchTest_U32(modified + init + 20, UINT32_C(0x2406001c));
	PatchTest_U32(modified + init + 24, UINT32_C(0x24a53820));
	PatchTest_U32(modified + init + 28, UINT32_C(0x0c01df2e));
	PatchTest_U32(modified + init + 32, UINT32_C(0x27a40018));
	PatchTest_U32(modified + PATCH_MUSIC_LOADER_START + 8, UINT32_C(0x9046001c));
	PatchTest_U32(modified + PATCH_MUSIC_LOADER_START + 16, UINT32_C(0x0c00cb4c));
	for (int i = 0; i < 27; i++) { modified[arrays + i] = i * 2; modified[arrays + 32 + i] = i * 2 + 1; }
	unsigned char vrmIDs[27], levIDs[27];
	CTR_TEST_CHECK(PatchMusic_AdventureFiles(&modifiedExe, vrmIDs, levIDs) && vrmIDs[3] == 6 && levIDs[3] == 7,
	               "a recognized adventure loader exposes its actual BIGFILE pairs");
	static const uint32_t titleLoader[] = {0x3c02807f, 0x3c05807f, 0x27a40020, 0x24421200,
	                                       0x00001825, 0x24a51300, 0x2408001b};
	for (size_t i = 0; i < SDL_arraysize(titleLoader); i++) { PatchTest_U32(modified + 0x1000 + i * 4, titleLoader[i]); }
	unsigned char adventure[0x400] = {0};
	for (int i = 0; i < 27; i++) { PatchTest_U32(adventure + 0x200 + i * 4, UINT32_C(0x807f1300)); }
	memcpy(adventure + 0x300, "Imperial Grand Prix", sizeof("Imperial Grand Prix"));
	struct PatchMusicBytes advBytes = {adventure, sizeof(adventure)};
	CTR_TEST_CHECK(PatchMusic_AdventureTitle(&modifiedExe, &advBytes, 3, title, sizeof(title)) &&
	               strcmp(title, "Imperial Grand Prix") == 0, "a matched adventure title follows the selected pair index");
	PatchTest_U32(adventure + 0x200 + 4, UINT32_C(0x807f0000));
	CTR_TEST_CHECK(!PatchMusic_AdventureTitle(&modifiedExe, &advBytes, 3, title, sizeof(title)),
	               "an out-of-range title pointer rejects the table");
	modified[arrays + 32 + 3] = 8;
	CTR_TEST_CHECK(!PatchMusic_AdventureFiles(&modifiedExe, vrmIDs, levIDs),
	               "mismatched VRM and LEV indices reject the entire inferred mapping");
	free(retail);
	free(modified);

	unsigned char original[8192] = {0}, patched[8192] = {0}, exe[PATCH_MUSIC_SONG_TABLE + 18] = {0};
	memcpy(original, "HOWL", 4);
	PatchTest_U32(original + 16, 1);
	PatchTest_U32(original + 28, 1);
	PatchTest_U32(original + 32, 1);
	original[42] = 2; /* sample size, in eight-byte units */
	original[44] = 1; /* bank sector */
	original[46] = 3; /* song sector */
	original[2048] = 1; /* one sample, ID zero */
	PatchTest_U32(original + 6144, 32);
	original[6148] = 1; /* one melodic instrument */
	original[6150] = 1; /* one song */
	memcpy(patched, original, sizeof(original));
	struct PatchMusic music = {.original.bytes = {original, sizeof(original)},
	                           .patched.bytes = {patched, sizeof(patched)}, .exe = {exe, sizeof(exe)}};
	char output[PATCH_PATH_BYTES];
	TrackPatch_Path(output, root, "synthetic.sca");
	CTR_TEST_CHECK(PatchMusic_Howl(&music.original) && PatchMusic_Howl(&music.patched),
	               "bounded HWL tables can be parsed");
	CTR_TEST_CHECK(PatchMusic_WriteSca(&music, 0, output), "a compatible bank and CSEQ produce an SCA");
	FILE *file = fopen(output, "rb");
	unsigned char sca[2200] = {0};
	size_t n = file != NULL ? fread(sca, 1, sizeof(sca), file) : 0;
	if (file != NULL) { fclose(file); }
	CTR_TEST_CHECK(n == 2126 && memcmp(sca, "SCA\1BANK", 8) == 0 &&
	               memcmp(sca + 2076, "CSEQ", 4) == 0 && memcmp(sca + 2116, "SIZE", 4) == 0,
	               "SCA chunks contain the selected bank, sequence and sample sizes");
	patched[6144 + 8 + 6] = 1;
	CTR_TEST_CHECK(!PatchMusic_WriteSca(&music, 0, output), "an unavailable instrument sample is rejected");
	patched[6144 + 8 + 6] = 0;
	patched[2048 + PATCH_SCA_SECTOR] ^= 1;
	CTR_TEST_CHECK(!PatchMusic_WriteSca(&music, 0, output), "changed shared samples are not silently replaced by retail audio");
}

/* Optional diagnostics: caller supplies images and an output in scratch space. */
static int PatchTest_AudioLive(const char *base, const char *patched, const char *output, int slot)
{
	SDL_AtomicInt cancel = {0};
	struct PatchMusic music;
	PatchMusic_Open(&music, base, patched, &cancel);
	int level = PatchMusic_SelectLevel(&music, slot);
	int ok = level >= 0 && PatchMusic_WriteSca(&music, level, output);
	printf("available=%d slot=%d level=%d generated=%d\n", music.available, slot, level, ok);
	PatchMusic_Close(&music);
	return ok ? 0 : 1;
}

static void PatchTest_ImportAudioLive(const char *root, const char *assets, const char *patch, const char *expected, const char *expectedAuthor,
                                      const char *slot)
{
	CTR_TEST_SECTION("complete xdelta import with music");
	char from[PATCH_PATH_BYTES], to[PATCH_PATH_BYTES], tools[PATCH_PATH_BYTES];
	TrackPatch_Path(from, assets, "ctr-u.bin");
	TrackPatch_Path(to, root, "ctr-u.bin");
	if (!SDL_CopyFile(from, to)) { CtrTest_Bail("cannot copy test ROM"); }
	TrackPatch_Path(tools, root, "tools");
	SDL_CreateDirectory(tools);
#if defined(_WIN32)
	const char *names[] = {"xdelta3.exe", "bigtool.exe", "CTRFramework.dll", "NAudio.dll", "Newtonsoft.Json.dll"};
#else
	const char *names[] = {"xdelta3", "bigtool"};
#endif
	for (size_t i = 0; i < SDL_arraysize(names); i++)
	{
		char relative[128];
		SDL_snprintf(relative, sizeof(relative), "tools/%s", names[i]);
		TrackPatch_Path(from, assets, relative);
		TrackPatch_Path(to, root, relative);
		if (!SDL_CopyFile(from, to)) { CtrTest_Bail("cannot copy test tools"); }
	}
	CTR_TEST_CHECK(NativeTrackPatch_Start(root, patch), "the local patch starts");
	int result;
	do { result = NativeTrackPatch_Poll(); if (result == 0) { SDL_Delay(20); } } while (result == 0);
	printf("%s\n", NativeTrackPatch_Message());
	CTR_TEST_CHECK(result > 0 && s_trackPatchJob.musicGenerated == 1 && s_trackPatchJob.musicSkipped == 0,
	               "the complete import generates a custom music file");
	char relative[PATCH_NAME_BYTES + 40];
	SDL_snprintf(relative, sizeof(relative), "tracks/%s-%s/track.sca", s_trackPatchJob.name, slot);
	TrackPatch_Path(to, root, relative);
	CTR_TEST_CHECK(TrackPatch_Different(expected, to) == 0, "installed audio matches the verified SCA");
	if (expectedAuthor != NULL)
	{
		TrackPatch_Path(to, root, "tracks");
		struct NativeTrackRegistry *registry = NativeTrackLibrary_Scan(to, NULL);
		const struct NativeTrackMetadata *track = NativeTrackRegistry_Get(registry, NATIVE_TRACK_CUSTOM_ID_BASE);
		int hasAuthor = track != NULL && track->author != NULL && track->author[0] != '\0';
		CTR_TEST_CHECK(strcmp(expectedAuthor, "-") == 0 ? !hasAuthor : hasAuthor && strcmp(track->author, expectedAuthor) == 0,
		               "installed metadata contains exactly the expected optional author");
		NativeTrackRegistry_Destroy(registry);
	}
	int clean = 1;
	SDL_EnumerateDirectory(root, PatchTest_NoTemporary, &clean);
	CTR_TEST_CHECK(clean, "temporary disc images and extracted files are removed");
	NativeTrackPatch_Shutdown();
}

/* Swap the two retail 1P pairs by exchanging their BIGFILE extents. Only the
 * scratch image's archive index changes; the track bytes come from the ROM.
 * This image is for extraction tests, not for booting on a console. */
static void PatchTest_SwapRetailPairs(const char *image)
{
	FILE *file = fopen(image, "r+b");
	uint32_t lba, bytes;
	unsigned char index[PATCH_DISC_DATA_BYTES], entry[8];
	if (file == NULL || !PatchDisc_FindBig(file, &lba, &bytes) || !PatchDisc_Sector(file, lba, index) ||
	    PatchDisc_Read32(index) != 0 || PatchDisc_Read32(index + 4) <= 9)
	{
		CtrTest_Bail("cannot read the retail BIGFILE index");
	}
	/* NTSC-U: proto8 VRM/LEV are entries 0/1; proto9 VRM/LEV are 8/9. */
	for (int role = 0; role < 2; role++)
	{
		unsigned char *first = index + 8 + role * 8;
		unsigned char *second = index + 8 + (8 + role) * 8;
		memcpy(entry, first, sizeof(entry));
		memcpy(first, second, sizeof(entry));
		memcpy(second, entry, sizeof(entry));
	}
	if (fseek(file, (long)((uint64_t)lba * PATCH_DISC_RAW_BYTES + PATCH_DISC_DATA_OFFSET), SEEK_SET) != 0 ||
	    fwrite(index, 1, sizeof(index), file) != sizeof(index) || fclose(file) != 0)
	{
		CtrTest_Bail("cannot write the scratch retail BIGFILE index");
	}
}

/* Opt-in only: generates its own patch from the supplied retail ROM. */
static void PatchTest_RetailLive(const char *root, const char *assets)
{
	CTR_TEST_SECTION("retail ROM xdelta integration");
	char from[PATCH_PATH_BYTES], to[PATCH_PATH_BYTES], tools[PATCH_PATH_BYTES];
	TrackPatch_Path(from, assets, "ctr-u.bin");
	TrackPatch_Path(to, root, "ctr-u.bin");
	if (!SDL_CopyFile(from, to)) { CtrTest_Bail("cannot copy the test base ROM"); }
	TrackPatch_Path(tools, root, "tools");
	SDL_CreateDirectory(tools);
#if defined(_WIN32)
	const char *names[] = {"xdelta3.exe", "bigtool.exe", "CTRFramework.dll", "NAudio.dll", "Newtonsoft.Json.dll"};
#else
	const char *names[] = {"xdelta3", "bigtool"};
#endif
	for (size_t i = 0; i < SDL_arraysize(names); i++)
	{
		char relative[128];
		SDL_snprintf(relative, sizeof(relative), "tools/%s", names[i]);
		TrackPatch_Path(from, assets, relative);
		TrackPatch_Path(to, root, relative);
		if (!SDL_CopyFile(from, to)) { CtrTest_Bail("cannot copy the test tools"); }
	}
	char original[PATCH_PATH_BYTES], target[PATCH_PATH_BYTES], patch[PATCH_PATH_BYTES];
	char bigfile[PATCH_PATH_BYTES], bigtool[PATCH_PATH_BYTES], xdelta[PATCH_PATH_BYTES];
	TrackPatch_Path(original, root, "ctr-u.bin");
	TrackPatch_Path(target, root, "retail-swap.bin");
	TrackPatch_Path(patch, root, "RetailSwap.xdelta");
	TrackPatch_Path(bigfile, root, "retail.big");
	TrackPatch_Path(xdelta, tools, names[0]);
	TrackPatch_Path(bigtool, tools, names[1]);
	if (!SDL_CopyFile(original, target)) { CtrTest_Bail("cannot copy the scratch target image"); }
	PatchTest_SwapRetailPairs(target);
	struct TrackPatchJob job = {0};
	if (!PatchDisc_ExtractBig(original, bigfile, &job.cancel)) { CtrTest_Bail("cannot extract the retail BIGFILE"); }
	const char *extract[] = {bigtool, bigfile, NULL};
	if (!TrackPatch_Process(&job, extract)) { CtrTest_Bail("cannot extract the retail track fixtures with CTR-tools"); }
	const char *encode[] = {xdelta, "-e", "-s", original, target, patch, NULL};
	if (!TrackPatch_Process(&job, encode)) { CtrTest_Bail("cannot generate the retail-only xdelta fixture"); }
	CTR_TEST_CHECK(NativeTrackPatch_Start(root, patch), "a patch generated entirely from retail tracks starts");
	int result;
	do { result = NativeTrackPatch_Poll(); if (result == 0) { SDL_Delay(20); } } while (result == 0);
	printf("%s\n", NativeTrackPatch_Message());
	CTR_TEST_CHECK(result > 0, "the generated retail patch is applied, extracted and installed");
	TrackPatch_Path(to, root, "tracks");
	struct NativeTrackScanResult scan;
	struct NativeTrackRegistry *registry = NativeTrackLibrary_Scan(to, &scan);
	CTR_TEST_CHECK(registry != NULL && scan.installed == 2 && scan.rejected == 0,
	               "only the two swapped retail circuits appear in the scratch library");
	for (int slot = 0; slot < 2; slot++)
	{
		for (int role = 0; role < 2; role++)
		{
			char expected[PATCH_PATH_BYTES], installed[PATCH_PATH_BYTES], relative[180];
			const char *extension = role == 0 ? "lev" : "vrm";
			SDL_snprintf(relative, sizeof(relative), "retail/levels/tracks/%s/1P/data.%s", s_patchTrackSlots[1 - slot], extension);
			TrackPatch_Path(expected, root, relative);
			SDL_snprintf(relative, sizeof(relative), "tracks/RetailSwap-%s/track.%s", s_patchTrackSlots[slot], extension);
			TrackPatch_Path(installed, root, relative);
			CTR_TEST_CHECK(TrackPatch_Different(expected, installed) == 0,
			               "the imported file matches the opposite retail slot extracted from the original ROM");
		}
	}
	NativeTrackRegistry_Destroy(registry);
	TrackPatch_Path(from, assets, "ctr-u.bin");
	TrackPatch_Path(to, root, "ctr-u.bin");
	CTR_TEST_CHECK(TrackPatch_Different(from, to) == 0, "the base ROM is unchanged after the complete import");
	int cleaned = 1;
	SDL_EnumerateDirectory(root, PatchTest_NoTemporary, &cleaned);
	CTR_TEST_CHECK(cleaned, "the complete real import removes all temporary images and extraction output");
	NativeTrackPatch_Shutdown();
}

int main(int argc, char **argv)
{
	if (argc == 4 && strcmp(argv[1], "--extract-big") == 0)
	{
		SDL_AtomicInt cancel = {0};
		return PatchDisc_ExtractBig(argv[2], argv[3], &cancel) ? 0 : 1;
	}
	if (argc == 4 && strcmp(argv[1], "--inspect-loader") == 0)
	{
		SDL_AtomicInt cancel = {0};
		struct PatchMusic music;
		PatchMusic_Open(&music, argv[2], argv[3], &cancel);
		unsigned char vrm[27], lev[27];
		int adventure = PatchMusic_AdventureFiles(&music.exe, vrm, lev);
		char title[256];
		int level = PatchMusic_AllBinTrack(&music.exe, &music.all, 10, title, sizeof(title));
		printf("adventure=%d all-level=%d title=%s\n", adventure, level, level >= 0 ? title : "-");
		if (adventure)
		{
			int found = PatchMusic_AdventureTitle(&music.exe, &music.adventure, 0, title, sizeof(title));
			printf("first pair: %u/%u title=%s\n", vrm[0], lev[0], found ? title : "-");
		}
		PatchMusic_Close(&music);
		return 0;
	}
	if (argc == 6 && strcmp(argv[1], "--audio-live") == 0)
	{
		return PatchTest_AudioLive(argv[2], argv[3], argv[4], atoi(argv[5]));
	}
	if (argc == 3 && strcmp(argv[1], "--child") == 0)
	{
		return strcmp(argv[2], "path with spaces & symbols") == 0 ? 0 : 1;
	}
	if (argc == 2 && strcmp(argv[1], "--wait-child") == 0)
	{
		SDL_Delay(10000);
		return 0;
	}
	char root[PATCH_PATH_BYTES];
	CtrTestScratch_Open(root, sizeof(root), "test-track-patch", NULL);
	TrackPatch_RemoveTree(root);
	SDL_CreateDirectory(root);
	if ((argc == 4 || argc == 6) && strcmp(argv[1], "--import-patch-live") == 0)
	{
		char source[PATCH_PATH_BYTES], target[PATCH_PATH_BYTES], tools[PATCH_PATH_BYTES], work[PATCH_PATH_BYTES];
		TrackPatch_Path(source, argv[2], "ctr-u.bin");
		TrackPatch_Path(target, root, "ctr-u.bin");
		if (!SDL_CopyFile(source, target)) { CtrTest_Bail("cannot copy the test ROM"); }
		TrackPatch_Path(tools, root, "tools");
		SDL_CreateDirectory(tools);
#if defined(_WIN32)
		const char *names[] = {"xdelta3.exe", "bigtool.exe", "bigtool.exe.config", "CTRFramework.dll", "NAudio.dll",
		                       "Newtonsoft.Json.dll", "Crc32.NET.dll", "System.Numerics.Vectors.dll"};
#else
		const char *names[] = {"xdelta3", "bigtool"};
#endif
		for (size_t i = 0; i < SDL_arraysize(names); i++)
		{
			char relative[128];
			SDL_snprintf(relative, sizeof(relative), "tools/%s", names[i]);
			TrackPatch_Path(source, argv[2], relative);
			TrackPatch_Path(target, root, relative);
			if (!SDL_CopyFile(source, target)) { CtrTest_Bail("cannot copy test tools"); }
		}
		TrackPatch_Path(work, root, "work");
		SDL_CreateDirectory(work);
		struct TrackPatchJob job = {0};
		SDL_strlcpy(job.assets, root, sizeof(job.assets));
		SDL_strlcpy(job.patch, argv[3], sizeof(job.patch));
		SDL_strlcpy(job.name, "live", sizeof(job.name));
		int count = 0;
		const char *error = TrackPatch_Run(&job, work, &count);
		printf("imported=%d music=%d skipped=%d error=%s\n", count, job.musicGenerated, job.musicSkipped, error ? error : "none");
		CTR_TEST_CHECK(error == NULL && count > 0, "a supplied patch imports into a disposable library");
		if (argc == 6)
		{
			char relative[PATCH_PATH_BYTES];
			SDL_snprintf(relative, sizeof(relative), "tracks/%s/track.ini", argv[4]);
			TrackPatch_Path(target, root, relative);
			size_t bytes = 0;
			char *manifest = SDL_LoadFile(target, &bytes);
			CTR_TEST_CHECK(manifest != NULL && strstr(manifest, argv[5]) != NULL,
			               "the expected title is attached to the selected BIGFILE pair");
			SDL_free(manifest);
		}
	}
	else if (argc >= 5 && argc <= 7 && strcmp(argv[1], "--import-audio-live") == 0)
	{
		PatchTest_ImportAudioLive(root, argv[2], argv[3], argv[4], argc >= 6 ? argv[5] : NULL, argc == 7 ? argv[6] : "island1");
	}
	else if (argc == 3 && strcmp(argv[1], "--retail-live") == 0)
	{
		PatchTest_RetailLive(root, argv[2]);
	}
	else
	{
		PatchTest_Disc(root);
		PatchTest_Collect(root);
		PatchTest_Author(root);
		PatchTest_Worker(root);
		PatchTest_Process();
		PatchTest_Music(root);
	}
	TrackPatch_RemoveTree(root);
	CtrTestScratch_Remove("test-track-patch", NULL);
	return CtrTest_Summary();
}
