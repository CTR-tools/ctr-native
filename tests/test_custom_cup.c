#include <shared/ctr_test.h>
#include <common.h>
#include <custom_cup.h>
#include <platform/native_track_registry.h>
#include <string.h>

struct sData sdata_static;

#include <platform/native_track_registry.c>
#include <game/CustomCup.c>

int main(void)
{
	CTR_TEST_SECTION("Custom cup rules");
	struct CustomCup cup = {0};
	strcpy(cup.name, "Cup");
	cup.trackCount = 2;
	CTR_TEST_CHECK(CustomCup_SetTrack(&cup, 0, "retail:crash-cove"), "retail identity can be stored");
	CTR_TEST_CHECK(CustomCup_SetTrack(&cup, 1, "retail:crash-cove"), "a track may be repeated");
	CTR_TEST_CHECK(CustomCup_IsValid(&cup), "two repeated tracks form a valid cup");
	CTR_TEST_CHECK(!CustomCup_IsValid(NULL), "a missing cup is invalid");
	cup.trackCount = 1;
	CTR_TEST_CHECK(!CustomCup_IsValid(&cup), "a cup needs at least two tracks");
	cup.trackCount = CUSTOM_CUP_MAX_TRACKS;
	for (int i = 0; i < cup.trackCount; i++) { CustomCup_SetTrack(&cup, i, "custom:track"); }
	CTR_TEST_CHECK(CustomCup_IsValid(&cup), "ten custom identities are accepted structurally");
	cup.trackCount++;
	CTR_TEST_CHECK(!CustomCup_IsValid(&cup), "more than ten tracks are rejected before reading the array");
	cup.trackCount = 2;
	CTR_TEST_CHECK(!CustomCup_SetTrack(&cup, 1, "custom:"), "an empty identity is rejected");
	CTR_TEST_CHECK(!CustomCup_SetTrack(&cup, 1, "track"), "an unprefixed identity is rejected");
	memset(cup.name, 'A', 30);
	cup.name[30] = '\0';
	CTR_TEST_CHECK(CustomCup_IsValid(&cup), "a thirty-character name is accepted");
	cup.name[30] = 'A';
	cup.name[31] = '\0';
	CTR_TEST_CHECK(!CustomCup_IsValid(&cup), "a thirty-one-character name is rejected");
	memset(cup.name, 0, sizeof(cup.name));
	for (int i = 0; i < 30; i++) { memcpy(cup.name + i * 4, "\xf0\x9f\x8f\x81", 4); }
	CTR_TEST_CHECK(CustomCup_IsValid(&cup), "thirty four-byte UTF-8 characters fit in the name");
	cup.name[0] = (char)0xff;
	CTR_TEST_CHECK(!CustomCup_IsValid(&cup), "invalid UTF-8 is rejected");
	memset(cup.name, 'A', sizeof(cup.name));
	CTR_TEST_CHECK(!CustomCup_IsValid(&cup), "a non-terminated name is rejected");
	cup.name[0] = '\0';
	CTR_TEST_CHECK(!CustomCup_IsValid(&cup), "an empty name is rejected");

	CTR_TEST_SECTION("Stable track identities");
	int id = -1;
	CTR_TEST_CHECK(CustomCup_ResolveTrack("retail:crash-cove", NULL, &id) && id == CRASH_COVE, "retail slug resolves without a registry");
	CTR_TEST_CHECK(!CustomCup_ResolveTrack("retail:missing", NULL, &id), "unknown retail slug is rejected");
	struct NativeTrackRegistry *registry = NativeTrackRegistry_Create();
	struct NativeTrackMetadata track = {"crash-cove", "Custom", "track.lev", "track.vrm", NULL, NULL, 0, 0, NULL};
	CTR_TEST_CHECK(registry != NULL && NativeTrackRegistry_Add(registry, &track, &id), "custom track is registered");
	char *identity = CustomCup_TrackIdentity(id, registry);
	CTR_TEST_CHECK(identity != NULL && strcmp(identity, "custom:crash-cove") == 0, "custom identity has its own namespace");
	int resolved = -1;
	CTR_TEST_CHECK(CustomCup_ResolveTrack(identity, registry, &resolved) && resolved == id, "custom identity resolves to its runtime ID");
	CTR_TEST_CHECK(!CustomCup_ResolveTrack("custom:missing", registry, &resolved), "an absent custom track does not resolve");
	for (int retail = DINGO_CANYON; retail <= TURBO_TRACK; retail++)
	{
		char *retailIdentity = CustomCup_TrackIdentity(retail, registry);
		CTR_TEST_CHECK(retailIdentity != NULL && CustomCup_ResolveTrack(retailIdentity, registry, &resolved) && resolved == retail,
		               "every retail race track round-trips through its identity");
		free(retailIdentity);
	}
	CTR_TEST_CHECK(CustomCup_SetTrack(&cup, 0, identity), "the cup owns a copy of the identity");
	free(identity);
	struct CustomCup copy = {0};
	CTR_TEST_CHECK(CustomCup_Copy(&copy, &cup), "a cup can be copied deeply");
	CustomCup_Clear(&cup);
	CTR_TEST_CHECK(strcmp(copy.tracks[0], "custom:crash-cove") == 0, "the copy survives clearing the original");
	NativeTrackRegistry_Destroy(registry);
	CTR_TEST_CHECK(!CustomCup_ResolveTrack(copy.tracks[0], NULL, &resolved), "a removed registry cannot resolve an old identity");
	registry = NativeTrackRegistry_Create();
	struct NativeTrackMetadata other = {"other", "Other", "other.lev", "other.vrm", NULL, NULL, 0, 0, NULL};
	int newID = -1;
	CTR_TEST_CHECK(registry != NULL && NativeTrackRegistry_Add(registry, &other, NULL) && NativeTrackRegistry_Add(registry, &track, &newID) &&
	               newID != id && CustomCup_ResolveTrack(copy.tracks[0], registry, &resolved) && resolved == newID,
	               "the stable identity resolves after registry order and runtime ID change");
	NativeTrackRegistry_Destroy(registry);
	CustomCup_Clear(&copy);
	identity = CustomCup_TrackIdentity(CRASH_COVE, NULL);
	CTR_TEST_CHECK(identity != NULL && strcmp(identity, "retail:crash-cove") == 0, "retail ID converts to a stable slug");
	free(identity);
	CTR_TEST_CHECK(CustomCup_TrackIdentity(NITRO_COURT, NULL) == NULL, "battle arena IDs have no cup race identity");
	return CtrTest_Summary();
}
