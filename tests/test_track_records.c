#include <shared/ctr_test_scratch.h>
static const char *s_testMemcardRoot;
const char *NativeMemcard_GetRoot(void) { return s_testMemcardRoot; }
#include <platform/native_track_records.c>

static void WriteTrackFile(const char *path, const char *content)
{
	FILE *file = fopen(path, "wb");
	if (file == NULL || fwrite(content, 1, strlen(content), file) != strlen(content) || fclose(file) != 0)
	{
		CtrTest_Bail("cannot create track fixture");
	}
}

int main(void)
{
	char root[800], lev[900], vrm[900], records[900], memcards[900], isolated[900];
	CtrTestScratch_Open(root, sizeof(root), "test-track-records", NULL);
	snprintf(lev, sizeof(lev), "%s/track.lev", root);
	snprintf(vrm, sizeof(vrm), "%s/track.vrm", root);
	snprintf(records, sizeof(records), "%s/records", root);
	snprintf(memcards, sizeof(memcards), "%s/memcards", root);
	snprintf(isolated, sizeof(isolated), "%s/replay", root);
	WriteTrackFile(lev, "first revision");
	WriteTrackFile(vrm, "textures");
	struct NativeTrackMetadata track = {"local:one", "Track", lev, vrm, NULL, NULL, 0, 0};
	struct NativeTrackRecordKey first, changed;
	CTR_TEST_SECTION("persistent track revision records");
	CTR_TEST_CHECK(NativeTrackRecords_Key(&track, &first), "a complete track has a stable revision key");
	unsigned int score[] = {42, 99}, loaded[] = {0, 0};
	CTR_TEST_CHECK(NativeTrackRecords_Save(records, &track, &first, score, sizeof(score)), "records are written");
	CTR_TEST_CHECK(NativeTrackRecords_Load(records, &track, &first, loaded, sizeof(loaded)) && memcmp(score, loaded, sizeof(score)) == 0,
	               "saved records survive a fresh read");
	score[0] = 17;
	CTR_TEST_CHECK(NativeTrackRecords_Save(records, &track, &first, score, sizeof(score)), "an existing record can be replaced");
	CTR_TEST_CHECK(NativeTrackRecords_Load(records, &track, &first, loaded, sizeof(loaded)) && loaded[0] == 17,
	               "replacement returns the latest result");
	CTR_TEST_SECTION("active memcard storage");
	s_testMemcardRoot = memcards;
	CTR_TEST_CHECK(NativeTrackRecords_SaveCurrent(NATIVE_TRACK_RECORD_SCORES, &track, &first, score, sizeof(score)),
	               "scores create their directory under the active memcard root");
	CTR_TEST_CHECK(NativeTrackRecords_SaveCurrent(NATIVE_TRACK_RECORD_GHOST, &track, &first, score, sizeof(score)),
	               "ghosts create their own directory under the active memcard root");
	char scoresDir[TRACK_RECORD_PATH_MAX], ghostsDir[TRACK_RECORD_PATH_MAX], path[TRACK_RECORD_PATH_MAX + 64];
	CTR_TEST_CHECK(TrackRecords_CurrentDirectory(scoresDir, sizeof(scoresDir), NATIVE_TRACK_RECORD_SCORES) &&
	               TrackRecords_CurrentDirectory(ghostsDir, sizeof(ghostsDir), NATIVE_TRACK_RECORD_GHOST),
	               "both storage directories resolve");
	CTR_TEST_CHECK(NativeTrackRecords_LoadCurrent(NATIVE_TRACK_RECORD_GHOST, &track, &first, loaded, sizeof(loaded)) && loaded[0] == 17,
	               "ghosts load from the active memcard root");
	s_testMemcardRoot = isolated;
	CTR_TEST_CHECK(!NativeTrackRecords_LoadCurrent(NATIVE_TRACK_RECORD_SCORES, &track, &first, loaded, sizeof(loaded)),
	               "an isolated memcard root does not read the normal records");
	CTR_TEST_CHECK(NativeTrackRecords_SaveCurrent(NATIVE_TRACK_RECORD_SCORES, &track, &first, score, sizeof(score)),
	               "an isolated root writes separate records");
	char isolatedDir[TRACK_RECORD_PATH_MAX];
	CTR_TEST_CHECK(TrackRecords_CurrentDirectory(isolatedDir, sizeof(isolatedDir), NATIVE_TRACK_RECORD_SCORES),
	               "isolated records resolve under the active root");
	CTR_TEST_CHECK(!NativeTrackRecords_SaveCurrent((enum NativeTrackRecordKind)99, &track, &first, score, sizeof(score)),
	               "unknown record kinds are rejected");
	WriteTrackFile(lev, "second revision");
	CTR_TEST_CHECK(NativeTrackRecords_Key(&track, &changed) && changed.lev != first.lev, "changed level data selects a different revision");
	CTR_TEST_CHECK(!NativeTrackRecords_Load(records, &track, &changed, loaded, sizeof(loaded)), "a new revision cannot inherit old records");
	track.identity = "local:other";
	CTR_TEST_CHECK(!NativeTrackRecords_Load(records, &track, &first, loaded, sizeof(loaded)), "a different identity cannot use the same record file");
	TrackRecords_Path(path, sizeof(path), scoresDir, &first);
	SDL_RemovePath(path);
	TrackRecords_Path(path, sizeof(path), ghostsDir, &first);
	SDL_RemovePath(path);
	TrackRecords_Path(path, sizeof(path), isolatedDir, &first);
	SDL_RemovePath(path);
	SDL_RemovePath(ghostsDir);
	SDL_RemovePath(scoresDir);
	SDL_RemovePath(memcards);
	SDL_RemovePath(isolatedDir);
	SDL_RemovePath(isolated);
	TrackRecords_Path(path, sizeof(path), records, &first);
	SDL_RemovePath(path);
	SDL_RemovePath(records);
	SDL_RemovePath(lev);
	SDL_RemovePath(vrm);
	CtrTestScratch_Remove("test-track-records", NULL);
	return CtrTest_Summary();
}
