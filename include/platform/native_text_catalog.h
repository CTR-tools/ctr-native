#ifndef NATIVE_TEXT_CATALOG_H
#define NATIVE_TEXT_CATALOG_H

#include <namespace_Lng.h>

// Catalog values are UTF-8, not game-font bytes. A caller must convert them
// before passing them to the renderer.

struct NativeTextCatalog
{
	char *values[LNG_EXPANDED_STRING_END];
};

enum NativeTextCatalogKind
{
	NATIVE_TEXT_CATALOG_RETAIL,
	NATIVE_TEXT_CATALOG_EXPANDED,
};

// The destination must initially be zeroed. On failure it is unchanged. A
// successful parse replaces only the selected kind, preserving the other.
// Unknown keys and invalid individual values are counted and ignored.
// This parser configures tomlc17's process-global options for UTF-8 checking.
int NativeTextCatalog_Parse(struct NativeTextCatalog *catalog, enum NativeTextCatalogKind kind, const char *text, int length, int *invalidEntries);
void NativeTextCatalog_Clear(struct NativeTextCatalog *catalog);
// Keys are retail or Expanded IDs. Source fallback belongs to the resolver.
const char *NativeTextCatalog_Get(const struct NativeTextCatalog *catalog, int key);
const char *NativeTextCatalog_KeyName(int key);

// Convert UTF-8 to the subset of the game's byte encoding that is verified.
// Returns zero for unsupported characters, embedded NULs or insufficient space.
int NativeTextCatalog_Encode(const char *text, char *output, unsigned int capacity);

#endif
