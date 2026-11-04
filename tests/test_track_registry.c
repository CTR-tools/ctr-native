#include "shared/ctr_test.h"
#include <stdint.h>
#include <platform/native_track_registry.c>

int main(void)
{
	CTR_TEST_SECTION("custom track metadata and identity");
	struct NativeTrackRegistry *registry = NativeTrackRegistry_Create();
	CTR_TEST_CHECK(registry != NULL, "registry can be created");
	if (registry == NULL)
	{
		return CtrTest_Summary();
	}
	char title[] = "First track";
	char author[] = "Author";
	struct NativeTrackMetadata first = {"track-a", title, "a.lev", "a.vrm", NULL, author, NULL, 0, 0};
	struct NativeTrackMetadata second = {"track-b", "Second track", "b.lev", "b.vrm", "b.sca", NULL, NULL, 0, 0};
	int firstID = 0, secondID = 0, found = 0;
	CTR_TEST_CHECK(NativeTrackRegistry_Add(registry, &first, &firstID), "first track is registered");
	const struct NativeTrackMetadata *stored = NativeTrackRegistry_Get(registry, firstID);
	title[0] = 'X';
	author[0] = 'X';
	CTR_TEST_CHECK(stored != NULL && strcmp(stored->title, "First track") == 0, "metadata is owned by the registry");
	CTR_TEST_CHECK(stored != NULL && strcmp(stored->author, "Author") == 0, "author is owned by the registry");
	CTR_TEST_CHECK(NativeTrackRegistry_Add(registry, &second, &secondID), "second track is registered");
	CTR_TEST_CHECK(firstID != secondID && firstID >= NATIVE_TRACK_CUSTOM_ID_BASE, "tracks have distinct extended IDs");
	CTR_TEST_CHECK((int)(int16_t)firstID == firstID && NATIVE_TRACK_CUSTOM_ID_MAX <= INT16_MAX,
	               "custom IDs fit the signed level-load interface");
	CTR_TEST_CHECK(stored == NativeTrackRegistry_Get(registry, firstID), "adding a track preserves existing metadata pointers");
	CTR_TEST_CHECK(NativeTrackRegistry_Find(registry, "track-b", &found) && found == secondID, "stable identity resolves to its runtime ID");
	CTR_TEST_CHECK(!NativeTrackRegistry_Add(registry, &first, NULL), "duplicate identities are rejected");
	CTR_TEST_CHECK(NativeTrackRegistry_Count(registry) == 2, "a failed addition leaves the registry intact");
	CTR_TEST_CHECK(NativeTrackRegistry_Get(registry, 6) == NULL, "retail IDs do not resolve to custom metadata");
	CTR_TEST_CHECK(NativeTrackRegistry_Get(registry, secondID + 1) == NULL, "unknown IDs do not resolve");
	CTR_TEST_CHECK(NativeTrackRegistry_Get(registry, secondID)->scaPath != NULL, "optional music path is retained");
	second.vrmPath = "";
	CTR_TEST_CHECK(!NativeTrackRegistry_Add(registry, &second, NULL), "a track without VRM is rejected");
	NativeTrackRegistry_Destroy(registry);
	return CtrTest_Summary();
}
