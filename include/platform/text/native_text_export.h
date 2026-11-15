#ifndef NATIVE_TEXT_EXPORT_H
#define NATIVE_TEXT_EXPORT_H

#include <stdio.h>

typedef const char *(*NativeTextExport_GetString)(int key);

// Writes a commented catalog to an already-open stream, without closing it.
int NativeTextExport_Write(FILE *file, int first, int end, NativeTextExport_GetString getString);

// Exports both catalogs for the active language. Existing files are never replaced.
enum NativeTextExportResult
{
	NATIVE_TEXT_EXPORT_OK,
	NATIVE_TEXT_EXPORT_EXISTS,
	NATIVE_TEXT_EXPORT_ERROR,
};
enum NativeTextExportResult NativeTextExport_Active(int languageSlot, NativeTextExport_GetString getString);

#endif
