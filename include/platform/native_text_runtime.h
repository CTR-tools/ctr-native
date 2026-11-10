#ifndef NATIVE_TEXT_RUNTIME_H
#define NATIVE_TEXT_RUNTIME_H

enum { NATIVE_TEXT_MAX_LANGUAGES = 64, NATIVE_TEXT_LANGUAGE_ID_SIZE = 32, NATIVE_TEXT_CUSTOM_FIRST = 8 };

// Scan shipped and personal language directories once after assets initialization.
void NativeTextRuntime_DiscoverLanguages(void);
int NativeTextRuntime_LanguageCount(void);
int NativeTextRuntime_LanguageAt(int index);
int NativeTextRuntime_LanguageSlot(const char *id);

// Refreshes fixed-address buffers. Previously returned pointers remain valid.
void NativeTextRuntime_Reload(int languageSlot);
// Stable catalog directory name for a supported language slot, or NULL.
const char *NativeTextRuntime_LanguageId(int languageSlot);
// True when a shipped or personal retail catalog can be opened.
int NativeTextRuntime_HasRetail(int languageSlot);
int NativeTextRuntime_HasText(int languageSlot);
// Cached, game-font-encoded self name from language.toml, or NULL.
const char *NativeTextRuntime_LanguageSelfName(int languageSlot);
enum NativeTextSource
{
	NATIVE_TEXT_SELECTED_OVERRIDE,
	NATIVE_TEXT_SELECTED_SHIPPED,
	NATIVE_TEXT_ENGLISH_OVERRIDE,
	NATIVE_TEXT_ENGLISH_SHIPPED,
	NATIVE_TEXT_SOURCE_COUNT,
};
// Retail IDs are available from selected shipped and override sources;
// Expanded IDs can be read from any source. This API does not install them.
const char *NativeTextRuntime_Get(enum NativeTextSource source, int key);

#endif
