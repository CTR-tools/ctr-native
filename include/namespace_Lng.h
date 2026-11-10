#ifndef CTR_NATIVE_NAMESPACE_LNG_H
#define CTR_NATIVE_NAMESPACE_LNG_H

// Expanded IDs follow the fixed NTSC-U retail index range.
// Resolve either kind through LANG_GetString.
enum ExpandedLngString
{
	LNG_TRACK_RESULT_SKIPPED = -1,
	LNG_TRACK_RESULT_OK = 0,
	LNG_RETAIL_STRING_COUNT = 0x24c,
	LNG_EXPANDED_STRING_BASE = LNG_RETAIL_STRING_COUNT,
	LNG_EXPANDED_FIRST = LNG_EXPANDED_STRING_BASE - 1,
#define EXPANDED_TEXT_KEY(name) name,
#include <expanded_text_keys.h>
#undef EXPANDED_TEXT_KEY
	LNG_EXPANDED_STRING_END,
	LNG_EXPANDED_STRING_COUNT = LNG_EXPANDED_STRING_END - LNG_EXPANDED_STRING_BASE,
};

#define LNG_EXPANDED_CHAOS_EFFECT_BASE LNG_EXPANDED_CHAOS_EFFECT_MIRROR_MIRROR

// Values are retail LNG string indices; the shared list also supplies TOML keys.
enum LngStringId
{
#define RETAIL_TEXT_KEY(name, value) name = value,
#include <retail_text_keys.h>
#undef RETAIL_TEXT_KEY
};

#endif
