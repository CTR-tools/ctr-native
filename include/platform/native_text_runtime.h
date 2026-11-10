#ifndef NATIVE_TEXT_RUNTIME_H
#define NATIVE_TEXT_RUNTIME_H

// Refreshes fixed-address buffers. Previously returned pointers remain valid.
void NativeTextRuntime_Reload(int languageSlot);
// True when a shipped retail catalog contains at least one usable entry.
int NativeTextRuntime_HasRetail(int languageSlot);
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
