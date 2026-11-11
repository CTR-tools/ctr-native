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
};

struct NativeTrackRegistry;

/* Initialize cups to zero. Do not copy them by assignment; use Copy and Clear. */
void CustomCup_Clear(struct CustomCup *cup);
bool CustomCup_Copy(struct CustomCup *destination, const struct CustomCup *source);
bool CustomCup_SetTrack(struct CustomCup *cup, int index, const char *identity);
/* Checks structure and identity syntax, not current track availability. */
bool CustomCup_IsValid(const struct CustomCup *cup);
/* Resolves a single identity against the current registry. Availability and
   mode restrictions are checked separately by the caller. */
bool CustomCup_ResolveTrack(const char *identity, const struct NativeTrackRegistry *registry, int *levelID);
/* Returns an owned prefixed identity for a runtime ID, or NULL on failure. */
char *CustomCup_TrackIdentity(int levelID, const struct NativeTrackRegistry *registry);

#endif
