#include <common.h>
#include <custom_cup.h>
#include <platform/native_track_registry.h>
#include <platform/native_track_library.h>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static const struct
{
	const char *slug;
	int levelID;
} s_customCupRetailTracks[] = {
	{"dingo-canyon", DINGO_CANYON}, {"dragon-mines", DRAGON_MINES}, {"blizzard-bluff", BLIZZARD_BLUFF},
	{"crash-cove", CRASH_COVE}, {"tiger-temple", TIGER_TEMPLE}, {"papu-pyramid", PAPU_PYRAMID},
	{"roo-tubes", ROO_TUBES}, {"hot-air-skyway", HOT_AIR_SKYWAY}, {"sewer-speedway", SEWER_SPEEDWAY},
	{"mystery-caves", MYSTERY_CAVES}, {"cortex-castle", CORTEX_CASTLE}, {"n-gin-labs", N_GIN_LABS},
	{"polar-pass", POLAR_PASS}, {"oxide-station", OXIDE_STATION}, {"coco-park", COCO_PARK},
	{"tiny-arena", TINY_ARENA}, {"slide-coliseum", SLIDE_COLISEUM}, {"turbo-track", TURBO_TRACK},
};

static const char s_customCupRetailPrefix[] = "retail:";
static const char s_customCupCustomPrefix[] = "custom:";

static bool CustomCup_HasPrefix(const char *identity, const char *prefix)
{
	size_t length = strlen(prefix);
	return identity != NULL && strncmp(identity, prefix, length) == 0 && identity[length] != '\0';
}

static char *CustomCup_Join(const char *prefix, const char *value)
{
	size_t prefixLength = strlen(prefix), valueLength = strlen(value);
	if (valueLength > SIZE_MAX - prefixLength - 1)
	{
		return NULL;
	}
	char *result = malloc(prefixLength + valueLength + 1);
	if (result != NULL)
	{
		memcpy(result, prefix, prefixLength);
		memcpy(result + prefixLength, value, valueLength + 1);
	}
	return result;
}

void CustomCup_Clear(struct CustomCup *cup)
{
	if (cup == NULL) { return; }
	for (int i = 0; i < CUSTOM_CUP_MAX_TRACKS; i++) { free(cup->tracks[i]); }
	memset(cup, 0, sizeof(*cup));
}

static bool CustomCup_LapsAreValid(int laps)
{
	return laps > 0 && laps <= 99;
}

static char *CustomCup_CopyTrack(const char *identity, int laps)
{
	if (!CustomCup_LapsAreValid(laps) ||
	    (!CustomCup_HasPrefix(identity, s_customCupRetailPrefix) && !CustomCup_HasPrefix(identity, s_customCupCustomPrefix)))
	{
		return NULL;
	}
	return CustomCup_Join("", identity);
}

bool CustomCup_ReplaceTrack(struct CustomCup *cup, int index, const char *identity, int laps)
{
	if (cup == NULL || cup->trackCount < 0 || cup->trackCount > CUSTOM_CUP_MAX_TRACKS ||
	    index < 0 || index >= cup->trackCount || cup->tracks[index] == NULL)
	{
		return false;
	}
	char *copy = CustomCup_CopyTrack(identity, laps);
	if (copy == NULL) { return false; }
	free(cup->tracks[index]);
	cup->tracks[index] = copy;
	cup->laps[index] = laps;
	return true;
}

bool CustomCup_SetLaps(struct CustomCup *cup, int index, int laps)
{
	if (cup == NULL || index < 0 || index >= CUSTOM_CUP_MAX_TRACKS || !CustomCup_LapsAreValid(laps))
	{
		return false;
	}
	cup->laps[index] = laps;
	return true;
}

bool CustomCup_InsertTrack(struct CustomCup *cup, int index, const char *identity, int laps)
{
	if (cup == NULL || cup->trackCount < 0 || cup->trackCount >= CUSTOM_CUP_MAX_TRACKS ||
	    index < 0 || index > cup->trackCount || cup->tracks[cup->trackCount] != NULL)
	{
		return false;
	}
	/* Copy before shifting: identity may already belong to this cup. */
	char *copy = CustomCup_CopyTrack(identity, laps);
	if (copy == NULL)
	{
		return false;
	}
	for (int i = cup->trackCount; i > index; i--)
	{
		cup->tracks[i] = cup->tracks[i - 1];
		cup->laps[i] = cup->laps[i - 1];
	}
	cup->tracks[index] = copy;
	cup->laps[index] = laps;
	cup->trackCount++;
	return true;
}

bool CustomCup_RemoveTrack(struct CustomCup *cup, int index)
{
	if (cup == NULL || cup->trackCount <= CUSTOM_CUP_MIN_TRACKS ||
	    cup->trackCount > CUSTOM_CUP_MAX_TRACKS || index < 0 || index >= cup->trackCount)
	{
		return false;
	}
	free(cup->tracks[index]);
	for (int i = index; i < cup->trackCount - 1; i++)
	{
		cup->tracks[i] = cup->tracks[i + 1];
		cup->laps[i] = cup->laps[i + 1];
	}
	cup->trackCount--;
	cup->tracks[cup->trackCount] = NULL;
	cup->laps[cup->trackCount] = 0;
	return true;
}

bool CustomCup_MoveTrack(struct CustomCup *cup, int from, int to)
{
	if (cup == NULL || cup->trackCount < 0 || cup->trackCount > CUSTOM_CUP_MAX_TRACKS ||
	    from < 0 || from >= cup->trackCount || to < 0 || to >= cup->trackCount)
	{
		return false;
	}
	char *track = cup->tracks[from];
	int laps = cup->laps[from];
	int step = from < to ? 1 : -1;
	for (int i = from; i != to; i += step)
	{
		cup->tracks[i] = cup->tracks[i + step];
		cup->laps[i] = cup->laps[i + step];
	}
	cup->tracks[to] = track;
	cup->laps[to] = laps;
	return true;
}

bool CustomCup_Copy(struct CustomCup *destination, const struct CustomCup *source)
{
	if (destination == NULL || !CustomCup_IsValid(source)) { return false; }
	if (destination == source) { return true; }
	struct CustomCup copy = {0};
	memcpy(copy.name, source->name, sizeof(copy.name));
	for (int i = 0; i < source->trackCount; i++)
	{
		if (!CustomCup_InsertTrack(&copy, i, source->tracks[i], source->laps[i]))
		{
			CustomCup_Clear(&copy);
			return false;
		}
	}
	CustomCup_Clear(destination);
	*destination = copy;
	return true;
}

bool CustomCup_ResolveTrack(const char *identity, const struct NativeTrackRegistry *registry, int *levelID)
{
	if (CustomCup_HasPrefix(identity, s_customCupRetailPrefix))
	{
		const char *slug = identity + sizeof(s_customCupRetailPrefix) - 1;
		for (size_t i = 0; i < sizeof(s_customCupRetailTracks) / sizeof(s_customCupRetailTracks[0]); i++)
		{
			if (strcmp(slug, s_customCupRetailTracks[i].slug) == 0)
			{
				if (levelID != NULL) { *levelID = s_customCupRetailTracks[i].levelID; }
				return true;
			}
		}
	}
	else if (CustomCup_HasPrefix(identity, s_customCupCustomPrefix))
	{
		return NativeTrackRegistry_Find(registry, identity + sizeof(s_customCupCustomPrefix) - 1, levelID) != 0;
	}
	return false;
}

int CustomCup_FirstUnresolvedTrack(const struct CustomCup *cup, const struct NativeTrackRegistry *registry)
{
	if (!CustomCup_IsValid(cup)) { return CUSTOM_CUP_INVALID; }
	for (int i = 0; i < cup->trackCount; i++)
	{
		if (!CustomCup_ResolveTrack(cup->tracks[i], registry, NULL)) { return i; }
	}
	return CUSTOM_CUP_TRACKS_RESOLVED;
}

int CustomCup_FirstUnavailableArcadeTrack(const struct CustomCup *cup, const struct NativeTrackRegistry *registry)
{
	if (!CustomCup_IsValid(cup)) { return CUSTOM_CUP_INVALID; }
	for (int i = 0; i < cup->trackCount; i++)
	{
		int levelID;
		if (!CustomCup_ResolveTrack(cup->tracks[i], registry, &levelID)) { return i; }
		const struct NativeTrackMetadata *track = NativeTrackRegistry_Get(registry, levelID);
		if (track != NULL && (!NativeTrackLibrary_IsEnabled(track) ||
		                     !NativeTrackLibrary_AllowsDetectedMode(track, NATIVE_TRACK_DETECTED_ARCADE)))
		{
			return i;
		}
	}
	return CUSTOM_CUP_TRACKS_RESOLVED;
}

char *CustomCup_TrackIdentity(int levelID, const struct NativeTrackRegistry *registry)
{
	for (size_t i = 0; i < sizeof(s_customCupRetailTracks) / sizeof(s_customCupRetailTracks[0]); i++)
	{
		if (levelID == s_customCupRetailTracks[i].levelID)
		{
			return CustomCup_Join(s_customCupRetailPrefix, s_customCupRetailTracks[i].slug);
		}
	}
	const struct NativeTrackMetadata *track = NativeTrackRegistry_Get(registry, levelID);
	return track != NULL ? CustomCup_Join(s_customCupCustomPrefix, track->identity) : NULL;
}

static bool CustomCup_NameIsValid(const char *name)
{
	size_t characters = 0;
	size_t i = 0;
	while (i < CUSTOM_CUP_NAME_BYTES && name[i] != '\0')
	{
		unsigned char first = (unsigned char)name[i];
		size_t bytes;
		// Printable ASCII or a valid UTF-8 leading byte (2, 3 or 4 bytes).
		// Reject ASCII controls, continuation bytes and invalid leading bytes.
		if (first >= 0x20 && first <= 0x7e)
		{
			bytes = 1;
		}
		else if (first >= 0xc2 && first <= 0xdf)
		{
			bytes = 2;
		}
		else if (first >= 0xe0 && first <= 0xef)
		{
			bytes = 3;
		}
		else if (first >= 0xf0 && first <= 0xf4)
		{
			bytes = 4;
		}
		else
		{
			return false;
		}
		if (i + bytes >= CUSTOM_CUP_NAME_BYTES || ++characters > CUSTOM_CUP_NAME_MAX_CHARACTERS)
		{
			return false;
		}
		for (size_t j = 1; j < bytes; j++)
		{
			unsigned char next = (unsigned char)name[i + j];
			// Continuation bytes are 0x80..0xbf. The first continuation also
			// excludes overlong encodings, UTF-16 surrogates and codepoints
			// above Unicode's maximum U+10FFFF.
			if (next < 0x80 || next > 0xbf ||
			    (j == 1 && ((first == 0xe0 && next < 0xa0) || (first == 0xed && next >= 0xa0) ||
			                (first == 0xf0 && next < 0x90) || (first == 0xf4 && next >= 0x90))))
			{
				return false;
			}
		}
		i += bytes;
	}
	return characters > 0 && i < CUSTOM_CUP_NAME_BYTES;
}

bool CustomCup_IsValid(const struct CustomCup *cup)
{
	if (cup == NULL || !CustomCup_NameIsValid(cup->name) ||
	    cup->trackCount < CUSTOM_CUP_MIN_TRACKS || cup->trackCount > CUSTOM_CUP_MAX_TRACKS)
	{
		return false;
	}
	for (int i = 0; i < cup->trackCount; i++)
	{
		if ((!CustomCup_HasPrefix(cup->tracks[i], s_customCupRetailPrefix) &&
		     !CustomCup_HasPrefix(cup->tracks[i], s_customCupCustomPrefix)) ||
		    !CustomCup_LapsAreValid(cup->laps[i]))
		{
			return false;
		}
	}
	return true;
}

void CustomCupRace_Clear(struct CustomCupRaceState *state)
{
	if (state != NULL) { memset(state, 0, sizeof(*state)); }
}

bool CustomCupRace_Start(struct CustomCupRaceState *state, const struct CustomCup *cup,
                         const struct NativeTrackRegistry *registry)
{
	if (state == NULL || CustomCup_FirstUnavailableArcadeTrack(cup, registry) != CUSTOM_CUP_TRACKS_RESOLVED)
	{
		return false;
	}
	struct CustomCupRaceState next = {0};
	memcpy(next.name, cup->name, sizeof(next.name));
	next.trackCount = cup->trackCount;
	for (int i = 0; i < cup->trackCount; i++)
	{
		if (!CustomCup_ResolveTrack(cup->tracks[i], registry, &next.levelIDs[i])) { return false; }
		next.laps[i] = cup->laps[i];
	}
	next.active = true;
	*state = next;
	return true;
}

bool CustomCupRace_Current(const struct CustomCupRaceState *state, int *levelID, int *laps)
{
	if (state == NULL || !state->active || state->trackCount < CUSTOM_CUP_MIN_TRACKS ||
	    state->trackCount > CUSTOM_CUP_MAX_TRACKS || state->trackIndex < 0 || state->trackIndex >= state->trackCount)
	{
		return false;
	}
	if (levelID != NULL) { *levelID = state->levelIDs[state->trackIndex]; }
	if (laps != NULL) { *laps = state->laps[state->trackIndex]; }
	return true;
}

bool CustomCupRace_IsCustomCup(const struct CustomCupRaceState *state, const struct GameTracker *tracker)
{
	return tracker != NULL && (tracker->gameMode2 & CUP_ANY_KIND) != 0 && state != NULL && state->active;
}

enum CustomCupAdvanceResult CustomCupRace_Advance(struct CustomCupRaceState *state)
{
	if (!CustomCupRace_Current(state, NULL, NULL)) { return CUSTOM_CUP_ADVANCE_INVALID; }
	if (state->trackIndex + 1 == state->trackCount) { return CUSTOM_CUP_ADVANCE_FINISHED; }
	state->trackIndex++;
	return CUSTOM_CUP_ADVANCE_NEXT;
}
