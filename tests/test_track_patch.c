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
			unsigned char lev[24] = {8, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 4};
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
	int count = 0;
	const char *error = TrackPatch_Collect(&job, root, &count);
	if (error != NULL) { printf("Discovery error: %s\n", error); }
	CTR_TEST_CHECK(error == NULL && count == 2, "geometry and texture changes are detected without importing the 16 untouched tracks");
	char prepared[PATCH_PATH_BYTES];
	TrackPatch_Path(prepared, root, "prepared");
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
	CTR_TEST_CHECK(TrackPatch_Collect(&job, root, &count) != NULL, "cancellation stops circuit discovery");
	SDL_SetAtomicInt(&job.cancel, 0);
	TrackPatch_RemoveTree(prepared);
	for (int i = 0; i < 2; i++)
	{
		char original[PATCH_PATH_BYTES], patched[PATCH_PATH_BYTES], relative[180];
		SDL_snprintf(relative, sizeof(relative), "original/levels/tracks/%s/1P/data.%s", s_patchTrackSlots[i], i == 0 ? "lev" : "vrm");
		TrackPatch_Path(original, root, relative);
		SDL_snprintf(relative, sizeof(relative), "patched/levels/tracks/%s/1P/data.%s", s_patchTrackSlots[i], i == 0 ? "lev" : "vrm");
		TrackPatch_Path(patched, root, relative);
		SDL_CopyFile(original, patched);
	}
	CTR_TEST_CHECK(TrackPatch_Collect(&job, root, &count) != NULL && count == 0,
	               "an unchanged ROM never installs retail circuits as custom content");
	char missing[PATCH_PATH_BYTES];
	TrackPatch_Path(missing, root, "patched/levels/tracks/proto8/1P/data.vrm");
	SDL_RemovePath(missing);
	CTR_TEST_CHECK(TrackPatch_Collect(&job, root, &count) != NULL, "an incomplete extracted pair is an error, not a guessed match");
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
	if (argc == 3 && strcmp(argv[1], "--retail-live") == 0)
	{
		PatchTest_RetailLive(root, argv[2]);
	}
	else
	{
		PatchTest_Disc(root);
		PatchTest_Collect(root);
		PatchTest_Worker(root);
		PatchTest_Process();
	}
	TrackPatch_RemoveTree(root);
	CtrTestScratch_Remove("test-track-patch", NULL);
	return CtrTest_Summary();
}
