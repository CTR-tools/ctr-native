#include <shared/ctr_test.h>
#include <shared/ctr_test_scratch.h>
#include <common.h>
#include <platform/native_custom_cup_storage.h>
#include <platform/native_assets.h>

static char s_base[512];
const char *NativeAssets_GetBaseDir(void) { return s_base; }
struct sData sdata_static;

#include <platform/native_track_registry.c>
#include <game/CustomCup.c>
#include <platform/native_custom_cup_storage.c>

struct ScanResult { int valid; int invalid; int errors; };

static void CountCup(void *context, const char *id, enum NativeCustomCupStorageResult result, const struct CustomCup *cup)
{
	struct ScanResult *scan = context;
	(void)id;
	if (result == NATIVE_CUSTOM_CUP_OK && cup != NULL) { scan->valid++; }
	else if (result == NATIVE_CUSTOM_CUP_INVALID && cup == NULL) { scan->invalid++; }
	else { scan->errors++; }
}

int main(void)
{
	CtrTestScratch_Open(s_base, sizeof(s_base), "custom-cup-storage", NULL);
	struct CustomCup cup = {0}, loaded = {0};
	strcpy(cup.name, "Cup % | \xf0\x9f\x8f\x81");
	CTR_TEST_CHECK(CustomCup_InsertTrack(&cup, 0, "custom:track%|x", 3) &&
	               CustomCup_InsertTrack(&cup, 1, "custom:track%|x", 7), "duplicate races can be prepared");
	char id[NATIVE_CUSTOM_CUP_ID_SIZE] = {0};
	CTR_TEST_CHECK(NativeCustomCup_Save(id, &cup) == NATIVE_CUSTOM_CUP_OK && strlen(id) == 32,
	               "a new cup receives an independent file identifier");
	char secondID[NATIVE_CUSTOM_CUP_ID_SIZE] = {0};
	CTR_TEST_CHECK(NativeCustomCup_Save(secondID, &cup) == NATIVE_CUSTOM_CUP_DUPLICATE && secondID[0] == '\0',
	               "duplicate names are rejected without losing the new cup or assigning an ID");
	strcpy(cup.name, "Other");
	CTR_TEST_CHECK(NativeCustomCup_Save(secondID, &cup) == NATIVE_CUSTOM_CUP_OK && strcmp(secondID, id) != 0,
	               "a different name can be saved as a separate file");
	strcpy(cup.name, "Cup % | \xf0\x9f\x8f\x81");
	CTR_TEST_CHECK(NativeCustomCup_Load(id, &loaded) == NATIVE_CUSTOM_CUP_OK &&
	               strcmp(loaded.name, cup.name) == 0 && loaded.trackCount == 2 &&
	               strcmp(loaded.tracks[0], cup.tracks[0]) == 0 && strcmp(loaded.tracks[1], cup.tracks[1]) == 0 &&
	               loaded.laps[0] == 3 && loaded.laps[1] == 7, "round-trip preserves special characters, duplicates and per-position laps");
	strcpy(cup.name, "Renamed");
	CTR_TEST_CHECK(NativeCustomCup_Save(id, &cup) == NATIVE_CUSTOM_CUP_OK &&
	               NativeCustomCup_Load(id, &loaded) == NATIVE_CUSTOM_CUP_OK && strcmp(loaded.name, "Renamed") == 0,
	               "renaming a cup updates the same file");
	strcpy(cup.name, "Other");
	CTR_TEST_CHECK(NativeCustomCup_Save(id, &cup) == NATIVE_CUSTOM_CUP_DUPLICATE &&
	               NativeCustomCup_Load(id, &loaded) == NATIVE_CUSTOM_CUP_OK && strcmp(loaded.name, "Renamed") == 0,
	               "renaming to an existing name leaves the saved cup unchanged");
	strcpy(cup.name, "Renamed");
	char path[1100], directory[1024];
	snprintf(directory, sizeof(directory), "%scustom-cups", s_base);
	snprintf(path, sizeof(path), "%s/%s.ini", directory, id);
	FILE *file = fopen(path, "wb");
	if (file == NULL) { CtrTest_Bail("cannot write cup fixture"); }
	fputs("[cup]\nname=Legacy\ntrack=retail:crash-cove\ntrack=retail:crash-cove|5\n", file);
	fclose(file);
	CTR_TEST_CHECK(NativeCustomCup_Load(id, &loaded) == NATIVE_CUSTOM_CUP_OK && loaded.laps[0] == 3 && loaded.laps[1] == 5,
	               "a missing lap count defaults to three without merging duplicate tracks");
	file = fopen(path, "wb");
	if (file == NULL) { CtrTest_Bail("cannot write invalid cup fixture"); }
	fputs("[cup]\nname=Broken\ntrack=custom:bad%XX|3\n", file);
	fclose(file);
	CTR_TEST_CHECK(NativeCustomCup_Load(id, &loaded) == NATIVE_CUSTOM_CUP_INVALID && strcmp(loaded.name, "Legacy") == 0,
	               "malformed data leaves the previously loaded cup intact");
	CTR_TEST_CHECK(NativeCustomCup_Save(id, &cup) == NATIVE_CUSTOM_CUP_INVALID,
	               "saving refuses to overwrite a malformed file");
	CTR_TEST_CHECK(NativeCustomCup_Load(secondID, &loaded) == NATIVE_CUSTOM_CUP_OK && strcmp(loaded.name, "Other") == 0,
	               "a corrupted cup does not prevent loading another file");
	struct ScanResult scan = {0};
	CTR_TEST_CHECK(NativeCustomCup_Scan(CountCup, &scan) == NATIVE_CUSTOM_CUP_OK &&
	               scan.valid == 1 && scan.invalid == 1 && scan.errors == 0,
	               "scanning reports a corrupt file independently from the valid cup");
	strcpy(cup.name, "Another");
	char thirdID[NATIVE_CUSTOM_CUP_ID_SIZE] = {0};
	CTR_TEST_CHECK(NativeCustomCup_Save(thirdID, &cup) == NATIVE_CUSTOM_CUP_OK,
	               "a corrupt file does not prevent saving an unrelated cup");
	CTR_TEST_CHECK(NativeCustomCup_Load("../invalid", &loaded) == NATIVE_CUSTOM_CUP_INVALID,
	               "file identifiers cannot escape the cup directory");
	CustomCup_Clear(&cup);
	CustomCup_Clear(&loaded);
	SDL_RemovePath(path);
	snprintf(path, sizeof(path), "%s/%s.ini", directory, secondID);
	SDL_RemovePath(path);
	snprintf(path, sizeof(path), "%s/%s.ini", directory, thirdID);
	SDL_RemovePath(path);
	SDL_RemovePath(directory);
	CtrTestScratch_Remove("custom-cup-storage", NULL);
	return CtrTest_Summary();
}
