#ifndef NATIVE_TEXT_RUNTIME_H
#define NATIVE_TEXT_RUNTIME_H

// Refreshes fixed-address buffers. Previously returned pointers remain valid.
void NativeTextRuntime_Reload(int languageSlot);
enum NativeTextSource
{
	NATIVE_TEXT_SELECTED_OVERRIDE,
	NATIVE_TEXT_SELECTED_SHIPPED,
	NATIVE_TEXT_ENGLISH_OVERRIDE,
	NATIVE_TEXT_ENGLISH_SHIPPED,
	NATIVE_TEXT_SOURCE_COUNT,
};
// key is an LNG_EXPANDED_* ID.
const char *NativeTextRuntime_Get(enum NativeTextSource source, int key);

#endif
