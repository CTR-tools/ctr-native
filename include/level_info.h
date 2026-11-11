#ifndef CTR_LEVEL_INFO_H
#define CTR_LEVEL_INFO_H

#include <stddef.h>

struct MetaDataLEV;
struct NativeTrackMetadata;

enum LevelInfoKind
{
	LEVEL_INFO_UNKNOWN,
	LEVEL_INFO_RETAIL,
	LEVEL_INFO_CUSTOM,
};

// Pointers are borrowed; custom metadata is invalidated by a library refresh.
struct LevelInfo
{
	enum LevelInfoKind kind;
	const struct MetaDataLEV *retail;
	const struct NativeTrackMetadata *custom;
};

struct LevelInfo LevelInfo_Lookup(int levelID);
const char *LevelInfo_Title(const struct LevelInfo *info, char *const *languageStrings);
/* Retail titles are borrowed; custom titles are converted into buffer. */
const char *LevelInfo_DisplayTitle(int levelID, char *const *languageStrings, char *buffer, size_t capacity);

#endif
