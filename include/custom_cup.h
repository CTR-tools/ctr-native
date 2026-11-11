#ifndef CTR_CUSTOM_CUP_H
#define CTR_CUSTOM_CUP_H

#include <stdbool.h>

enum
{
	CUSTOM_CUP_MIN_TRACKS = 2,
	CUSTOM_CUP_MAX_TRACKS = 10,
	CUSTOM_CUP_NAME_MAX_CHARACTERS = 30,
	CUSTOM_CUP_NAME_BYTES = CUSTOM_CUP_NAME_MAX_CHARACTERS * 4 + 1,
	CUSTOM_CUP_VISIBLE_NAME_CHARACTERS = 14,
	CUSTOM_CUP_DEFAULT_LAPS = 3,
	CUSTOM_CUP_TRACKS_RESOLVED = -1,
	CUSTOM_CUP_INVALID = -2,
};

/* The kind is separate from retail cup indices: custom cups must never be
   used as indices into the standard or Adventure cup tables. */
enum CupKind
{
	CUP_KIND_STANDARD,
	CUP_KIND_ADVENTURE,
	CUP_KIND_CUSTOM,
};

struct CustomCup
{
	char name[CUSTOM_CUP_NAME_BYTES];
	int trackCount;
	/* Owned, prefixed stable identities in race order; never borrowed from the registry. */
	char *tracks[CUSTOM_CUP_MAX_TRACKS];
	/* Lap counts belong to positions, not track identities. */
	int laps[CUSTOM_CUP_MAX_TRACKS];
};

/* Pointer-free race snapshot, suitable for resident checkpoints. Runtime track
   IDs are resolved together when the championship starts. */
struct CustomCupRaceState
{
	char name[CUSTOM_CUP_NAME_BYTES];
	int trackCount;
	int trackIndex;
	int levelIDs[CUSTOM_CUP_MAX_TRACKS];
	int laps[CUSTOM_CUP_MAX_TRACKS];
	bool active;
};

enum CustomCupAdvanceResult
{
	CUSTOM_CUP_ADVANCE_INVALID,
	CUSTOM_CUP_ADVANCE_NEXT,
	CUSTOM_CUP_ADVANCE_FINISHED,
};

struct NativeTrackRegistry;
struct GameTracker;

/* Initialize cups to zero. Do not copy them by assignment; use Copy and Clear. */
void CustomCup_Clear(struct CustomCup *cup);
/* Only valid cups can be copied; the destination is unchanged on failure. */
bool CustomCup_Copy(struct CustomCup *destination, const struct CustomCup *source);
bool CustomCup_ReplaceTrack(struct CustomCup *cup, int index, const char *identity, int laps);
/* Only an existing race position can have its laps changed. */
bool CustomCup_SetLaps(struct CustomCup *cup, int index, int laps);
/* Editing operations move identities and lap counts together; failures leave the cup unchanged. */
bool CustomCup_InsertTrack(struct CustomCup *cup, int index, const char *identity, int laps);
bool CustomCup_RemoveTrack(struct CustomCup *cup, int index);
bool CustomCup_MoveTrack(struct CustomCup *cup, int from, int to);
/* Checks structure and identity syntax, not current track availability. */
bool CustomCup_IsValid(const struct CustomCup *cup);
/* Resolves a single identity against the current registry. Availability and
   mode restrictions are checked separately by the caller. */
bool CustomCup_ResolveTrack(const char *identity, const struct NativeTrackRegistry *registry, int *levelID);
/* Returns the first unresolved position, CUSTOM_CUP_TRACKS_RESOLVED, or
   CUSTOM_CUP_INVALID if the cup is invalid.
   This does not check whether a resolved track is enabled or supports a mode. */
int CustomCup_FirstUnresolvedTrack(const struct CustomCup *cup, const struct NativeTrackRegistry *registry);
/* As above, also rejects disabled custom tracks and tracks without Arcade support.
   Player-count and retail unlock restrictions are checked separately. */
int CustomCup_FirstUnavailableArcadeTrack(const struct CustomCup *cup, const struct NativeTrackRegistry *registry);
/* Returns an owned prefixed identity for a runtime ID, or NULL on failure. */
char *CustomCup_TrackIdentity(int levelID, const struct NativeTrackRegistry *registry);

/* Initialize race state to zero. Starting validates Arcade availability and
   leaves the previous state intact on failure. The final race remains current
   until the state is explicitly cleared. */
bool CustomCupRace_Start(struct CustomCupRaceState *state, const struct CustomCup *cup,
                         const struct NativeTrackRegistry *registry);
bool CustomCupRace_Current(const struct CustomCupRaceState *state, int *levelID, int *laps);
bool CustomCupRace_IsCustomCup(const struct CustomCupRaceState *state, const struct GameTracker *tracker);
enum CustomCupAdvanceResult CustomCupRace_Advance(struct CustomCupRaceState *state);
void CustomCupRace_Clear(struct CustomCupRaceState *state);

#endif
