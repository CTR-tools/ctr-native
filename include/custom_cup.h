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

/* A running championship owns a snapshot, independent of the editable cup. */
struct CustomCupSession
{
	struct CustomCup cup;
	int trackIndex;
	bool active;
};

enum CustomCupAdvanceResult
{
	CUSTOM_CUP_ADVANCE_INVALID,
	CUSTOM_CUP_ADVANCE_NEXT,
	CUSTOM_CUP_ADVANCE_FINISHED,
};

struct NativeTrackRegistry;

/* Initialize cups to zero. Do not copy them by assignment; use Copy and Clear. */
void CustomCup_Clear(struct CustomCup *cup);
/* Only valid cups can be copied; the destination is unchanged on failure. */
bool CustomCup_Copy(struct CustomCup *destination, const struct CustomCup *source);
bool CustomCup_ReplaceTrack(struct CustomCup *cup, int index, const char *identity, int laps);
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

/* Initialize sessions to zero and clear them when leaving the championship.
   Starting checks Arcade availability and leaves the old session intact on failure. */
bool CustomCupSession_Start(struct CustomCupSession *session, const struct CustomCup *cup,
                            const struct NativeTrackRegistry *registry);
void CustomCupSession_Clear(struct CustomCupSession *session);
/* Resolve again against the current registry, which may have been rebuilt. */
bool CustomCupSession_Current(const struct CustomCupSession *session, const struct NativeTrackRegistry *registry,
                              int *levelID, int *laps);
/* On the final race, retain its position until the session is explicitly cleared. */
enum CustomCupAdvanceResult CustomCupSession_Advance(struct CustomCupSession *session);

#endif
