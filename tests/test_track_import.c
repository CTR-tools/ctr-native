#include <shared/ctr_test_scratch.h>
#include <platform/native_track_registry.c>
#include <platform/native_track_library.c>
#include <platform/native_track_validate.c>
#include <platform/native_track_file.c>
#include <platform/native_track_import.c>

static void WriteImportFixture(const char *dir, const char *name, const void *data, size_t size)
{
	char path[1024];
	snprintf(path, sizeof(path), "%s/%s", dir, name);
	FILE *file = fopen(path, "wb");
	if (file == NULL || fwrite(data, 1, size, file) != size || fclose(file) != 0)
	{
		CtrTest_Bail("cannot write import fixture");
	}
}

int main(void)
{
	char root[700], source[800], library[800], target[900], path[1000];
	CtrTestScratch_Open(root, sizeof(root), "test-track-import", NULL);
	snprintf(source, sizeof(source), "%s/source", root);
	snprintf(library, sizeof(library), "%s/library", root);
	snprintf(target, sizeof(target), "%s/installed", library);
	SDL_CreateDirectory(source);
	unsigned char lev[24] = {8, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 4};
	unsigned char vrm[36] = {0x20, 0, 0, 0, 24};
	vrm[24] = 2;
	vrm[26] = 1;
	WriteImportFixture(source, "track.lev", lev, sizeof(lev));
	WriteImportFixture(source, "track.vrm", vrm, sizeof(vrm));
	const char manifest[] = "id=example:1\ntitle=Example\n";
	WriteImportFixture(source, "track.ini", manifest, sizeof(manifest) - 1);
	CTR_TEST_SECTION("local track import");
	CTR_TEST_CHECK(NativeTrackImport_Directory(library, source, "../escape") != NULL, "destination traversal is refused");
	const char *error = NativeTrackImport_Directory(library, source, "installed");
	CTR_TEST_CHECK(error == NULL, "a valid directory is installed");
	struct NativeTrackScanResult result;
	struct NativeTrackRegistry *registry = NativeTrackLibrary_Scan(library, &result);
	CTR_TEST_CHECK(registry != NULL && result.installed == 1 && NativeTrackRegistry_Find(registry, "example:1", NULL),
	               "the installed track is discoverable with its identity");
	NativeTrackRegistry_Destroy(registry);
	CTR_TEST_CHECK(NativeTrackImport_Directory(library, source, "installed") != NULL, "existing tracks are not overwritten");
	CTR_TEST_CHECK(NativeTrackImport_Directory(library, source, "duplicate") != NULL, "duplicate identities are refused");
	char otherSource[800], otherTarget[900];
	snprintf(otherSource, sizeof(otherSource), "%s/other-source", root);
	snprintf(otherTarget, sizeof(otherTarget), "%s/named-files", library);
	SDL_CreateDirectory(otherSource);
	WriteImportFixture(otherSource, "custom.LEV", lev, sizeof(lev));
	WriteImportFixture(otherSource, "textures.VRM", vrm, sizeof(vrm));
	CTR_TEST_CHECK(NativeTrackImport_Directory(library, otherSource, "named-files") == NULL,
	               "folder import accepts uniquely named files and case-insensitive extensions");
	WriteImportFixture(otherSource, "second.lev", lev, sizeof(lev));
	CTR_TEST_CHECK(NativeTrackImport_Directory(library, otherSource, "ambiguous") != NULL,
	               "a folder with multiple LEV files is refused rather than chosen arbitrarily");
	lev[0] = 255;
	WriteImportFixture(source, "track.lev", lev, sizeof(lev));
	CTR_TEST_CHECK(NativeTrackImport_Directory(library, source, "invalid") != NULL, "invalid LEV data is refused");
	snprintf(path, sizeof(path), "%s/invalid", library);
	SDL_PathInfo info;
	CTR_TEST_CHECK(!SDL_GetPathInfo(path, &info), "a rejected import does not publish a track");
	snprintf(path, sizeof(path), "%s/track.vrm", source);
	SDL_RemovePath(path);
	CTR_TEST_CHECK(NativeTrackImport_Directory(library, source, "missing") != NULL, "a missing VRM is refused");
	const char *files[] = {"track.lev", "track.vrm", "track.ini"};
	for (size_t i = 0; i < SDL_arraysize(files); i++)
	{
		snprintf(path, sizeof(path), "%s/%s", source, files[i]);
		SDL_RemovePath(path);
		snprintf(path, sizeof(path), "%s/%s", target, files[i]);
		SDL_RemovePath(path);
	}
	SDL_RemovePath(source);
	SDL_RemovePath(target);
	const char *otherFiles[] = {"custom.LEV", "textures.VRM", "second.lev"};
	for (size_t i = 0; i < SDL_arraysize(otherFiles); i++)
	{
		snprintf(path, sizeof(path), "%s/%s", otherSource, otherFiles[i]);
		SDL_RemovePath(path);
	}
	snprintf(path, sizeof(path), "%s/track.lev", otherTarget);
	SDL_RemovePath(path);
	snprintf(path, sizeof(path), "%s/track.vrm", otherTarget);
	SDL_RemovePath(path);
	SDL_RemovePath(otherSource);
	SDL_RemovePath(otherTarget);
	SDL_RemovePath(library);
	CtrTestScratch_Remove("test-track-import", NULL);
	return CtrTest_Summary();
}
