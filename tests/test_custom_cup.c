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
	CTR_TEST_CHECK(CustomCup_InsertTrack(&cup, 0, "retail:crash-cove", 1), "retail identity and laps can be stored together");
	CTR_TEST_CHECK(CustomCup_InsertTrack(&cup, 1, "retail:crash-cove", 7), "a repeated track can have a different lap count");
	CTR_TEST_CHECK(!CustomCup_ReplaceTrack(&cup, 0, "retail:other", 0) &&
	               strcmp(cup.tracks[0], "retail:crash-cove") == 0 && cup.laps[0] == 1,
	               "an invalid lap count leaves the entire position unchanged");
	CTR_TEST_CHECK(!CustomCup_SetLaps(&cup, 0, 0) && !CustomCup_SetLaps(&cup, 0, 100), "out-of-range lap counts are rejected");
	CTR_TEST_CHECK(!CustomCup_SetLaps(NULL, 0, 3) && !CustomCup_SetLaps(&cup, -1, 3) &&
	               !CustomCup_SetLaps(&cup, CUSTOM_CUP_MAX_TRACKS, 3), "invalid lap count destinations are rejected");
	CTR_TEST_CHECK(CustomCup_SetLaps(&cup, 0, 5) && cup.laps[1] == 7, "changing one position leaves the other unchanged");
	CTR_TEST_CHECK(CustomCup_IsValid(&cup), "two repeated tracks form a valid cup");
	cup.laps[0] = 0;
	CTR_TEST_CHECK(!CustomCup_IsValid(&cup), "validation rejects an unsupported stored lap count");
	cup.laps[0] = 5;
	CTR_TEST_CHECK(!CustomCup_IsValid(NULL), "a missing cup is invalid");
	cup.trackCount = 1;
	CTR_TEST_CHECK(!CustomCup_IsValid(&cup), "a cup needs at least two tracks");
	cup.trackCount = 2;
	for (int i = 2; i < CUSTOM_CUP_MAX_TRACKS; i++) { CustomCup_InsertTrack(&cup, i, "custom:track", CUSTOM_CUP_DEFAULT_LAPS); }
	CTR_TEST_CHECK(CustomCup_IsValid(&cup), "ten custom identities are accepted structurally");
	cup.trackCount++;
	CTR_TEST_CHECK(!CustomCup_IsValid(&cup), "more than ten tracks are rejected before reading the array");
	cup.trackCount = CUSTOM_CUP_MAX_TRACKS;
	while (cup.trackCount > 2) { CustomCup_RemoveTrack(&cup, cup.trackCount - 1); }
	CTR_TEST_CHECK(!CustomCup_ReplaceTrack(&cup, 1, "custom:", 3), "an empty identity is rejected");
	CTR_TEST_CHECK(!CustomCup_ReplaceTrack(&cup, 1, "track", 3) && cup.laps[1] == 7, "an invalid identity leaves the lap count unchanged");
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
	CTR_TEST_CHECK(CustomCup_ReplaceTrack(&cup, 0, identity, cup.laps[0]), "the cup owns a copy of the identity and preserves chosen laps");
	free(identity);
	struct CustomCup copy = {0};
	CTR_TEST_CHECK(!CustomCup_Copy(&copy, &cup), "an invalid cup cannot be copied");
	strcpy(cup.name, "Cup");
	CTR_TEST_CHECK(CustomCup_Copy(&copy, &cup), "a cup can be copied deeply");
	cup.laps[0] = 0;
	CTR_TEST_CHECK(!CustomCup_Copy(&copy, &cup) && copy.laps[0] == 5, "a failed copy leaves the destination unchanged");
	CTR_TEST_CHECK(copy.laps[0] == 5 && copy.laps[1] == 7, "copy preserves the lap count of each position");
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

	CTR_TEST_SECTION("Editing race positions");
	struct CustomCup edit = {0};
	strcpy(edit.name, "Edit");
	CTR_TEST_CHECK(CustomCup_InsertTrack(&edit, 0, "retail:crash-cove", 3) &&
	               CustomCup_InsertTrack(&edit, 1, "retail:roo-tubes", 5), "two races can be assembled in order");
	CTR_TEST_CHECK(!CustomCup_RemoveTrack(&edit, 0), "removing a race cannot go below the minimum");
	CTR_TEST_CHECK(!CustomCup_ReplaceTrack(&edit, 2, "retail:crash-cove", 3) && edit.trackCount == 2,
	               "replacement cannot create a new position");
	CTR_TEST_CHECK(CustomCup_ReplaceTrack(&edit, 0, edit.tracks[1], 7) && edit.trackCount == 2 &&
	               strcmp(edit.tracks[0], "retail:roo-tubes") == 0 && edit.laps[0] == 7 && edit.laps[1] == 5,
	               "replacement changes one existing race without moving the others");
	CTR_TEST_CHECK(CustomCup_ReplaceTrack(&edit, 0, "retail:crash-cove", 3), "the original race can be restored");
	CTR_TEST_CHECK(CustomCup_InsertTrack(&edit, 0, edit.tracks[1], 7) &&
	               strcmp(edit.tracks[0], "retail:roo-tubes") == 0 && edit.laps[0] == 7 &&
	               strcmp(edit.tracks[2], "retail:roo-tubes") == 0 && edit.laps[2] == 5,
	               "inserting an existing identity duplicates it with independent laps");
	CTR_TEST_CHECK(CustomCup_MoveTrack(&edit, 0, 2) && edit.laps[0] == 3 && edit.laps[1] == 5 && edit.laps[2] == 7 &&
	               strcmp(edit.tracks[0], "retail:crash-cove") == 0, "moving forward preserves race identities and laps");
	CTR_TEST_CHECK(CustomCup_MoveTrack(&edit, 2, 0) && edit.laps[0] == 7 && edit.laps[1] == 3 && edit.laps[2] == 5,
	               "moving backward preserves race identities and laps");
	CTR_TEST_CHECK(!CustomCup_InsertTrack(&edit, 1, "retail:crash-cove", 0) &&
	               !CustomCup_MoveTrack(&edit, -1, 0) && !CustomCup_RemoveTrack(&edit, 3) &&
	               edit.trackCount == 3 && edit.laps[0] == 7 && edit.laps[1] == 3 && edit.laps[2] == 5,
	               "invalid edits leave every race unchanged");
	CTR_TEST_CHECK(CustomCup_RemoveTrack(&edit, 1) && edit.trackCount == 2 && edit.laps[0] == 7 && edit.laps[1] == 5 &&
	               edit.tracks[2] == NULL && edit.laps[2] == 0, "removal shifts both fields and clears the vacated slot");
	for (int i = edit.trackCount; i < CUSTOM_CUP_MAX_TRACKS; i++)
	{
		CTR_TEST_CHECK(CustomCup_InsertTrack(&edit, i, "custom:track", 3), "a race can be appended up to the maximum");
	}
	CTR_TEST_CHECK(!CustomCup_InsertTrack(&edit, 0, "retail:crash-cove", 3) && CustomCup_IsValid(&edit),
	               "insertion beyond the maximum leaves a valid cup unchanged");
	CustomCup_Clear(&edit);
	return CtrTest_Summary();
}
