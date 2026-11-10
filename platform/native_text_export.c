#include <SDL3/SDL.h>
#include <platform/native_text_export.h>
#include <platform/native_text_catalog.h>
#include <platform/native_text_runtime.h>
#include <platform/native_assets.h>
#include <errno.h>
#include <fcntl.h>
#if defined(_WIN32)
#include <io.h>
#include <sys/stat.h>
#else
#include <unistd.h>
#endif

int NativeTextExport_Write(FILE *file, int first, int end, NativeTextExport_GetString getString)
{
	if (file == NULL || getString == NULL || first < 0 || end > LNG_EXPANDED_STRING_END || first > end) { return 0; }
	if (fputs("# Uncomment and edit entries to override the active translation.\n"
	          "# Keep \\uE0XX escapes: they represent original game font bytes.\n", file) < 0) { return 0; }
	for (int key = first; key < end; key++)
	{
		const char *value = getString(key);
		if (value == NULL || !NativeTextCatalog_WriteTemplateEntry(file, key, value)) { return 0; }
	}
	return !ferror(file);
}

static enum NativeTextExportResult NativeTextExport_Copy(FILE *source, const char *path)
{
	// The Windows C runtime does not support C11's fopen "x" mode.
#if defined(_WIN32)
	int descriptor = _open(path, _O_WRONLY | _O_CREAT | _O_EXCL | _O_BINARY, _S_IREAD | _S_IWRITE);
	FILE *destination = descriptor < 0 ? NULL : _fdopen(descriptor, "wb");
	if (descriptor >= 0 && destination == NULL) { _close(descriptor); }
#else
	int descriptor = open(path, O_WRONLY | O_CREAT | O_EXCL, 0666);
	FILE *destination = descriptor < 0 ? NULL : fdopen(descriptor, "wb");
	if (descriptor >= 0 && destination == NULL) { close(descriptor); }
#endif
	if (destination == NULL)
	{
		if (errno != EEXIST) { fprintf(stderr, "[Language] Cannot create export %s (errno %d)\n", path, errno); }
		return errno == EEXIST ? NATIVE_TEXT_EXPORT_EXISTS : NATIVE_TEXT_EXPORT_ERROR;
	}
	char buffer[4096];
	size_t count;
	int ok = fseek(source, 0, SEEK_SET) == 0;
	while (ok && (count = fread(buffer, 1, sizeof(buffer), source)) != 0)
	{
		ok = fwrite(buffer, 1, count, destination) == count;
	}
	if (ferror(source)) { ok = 0; }
	if (fclose(destination) != 0) { ok = 0; }
	return ok ? NATIVE_TEXT_EXPORT_OK : NATIVE_TEXT_EXPORT_ERROR;
}

enum NativeTextExportResult NativeTextExport_Active(int languageSlot, NativeTextExport_GetString getString)
{
	const char *id = NativeTextRuntime_LanguageId(languageSlot);
	if (id == NULL || getString == NULL) { return NATIVE_TEXT_EXPORT_ERROR; }
	FILE *retail = tmpfile();
	FILE *expanded = tmpfile();
	if (retail == NULL || expanded == NULL)
	{
		if (retail != NULL) { fclose(retail); }
		if (expanded != NULL) { fclose(expanded); }
		return NATIVE_TEXT_EXPORT_ERROR;
	}
	enum NativeTextExportResult result = NATIVE_TEXT_EXPORT_ERROR;
	char directory[1200], retailPath[1200], expandedPath[1200];
	if (NativeTextExport_Write(retail, 0, LNG_RETAIL_STRING_COUNT, getString) &&
	    NativeTextExport_Write(expanded, LNG_EXPANDED_STRING_BASE, LNG_EXPANDED_STRING_END, getString) &&
	    SDL_snprintf(directory, sizeof(directory), "%s/text/%s", NativeAssets_GetAssetDir(), id) < (int)sizeof(directory) &&
	    SDL_snprintf(retailPath, sizeof(retailPath), "%s/retail.toml", directory) < (int)sizeof(retailPath) &&
	    SDL_snprintf(expandedPath, sizeof(expandedPath), "%s/expanded.toml", directory) < (int)sizeof(expandedPath) &&
	    SDL_CreateDirectory(directory))
	{
		enum NativeTextExportResult first = NativeTextExport_Copy(retail, retailPath);
		enum NativeTextExportResult second = NativeTextExport_Copy(expanded, expandedPath);
		result = first == NATIVE_TEXT_EXPORT_ERROR || second == NATIVE_TEXT_EXPORT_ERROR ? NATIVE_TEXT_EXPORT_ERROR :
		         first == NATIVE_TEXT_EXPORT_EXISTS || second == NATIVE_TEXT_EXPORT_EXISTS ? NATIVE_TEXT_EXPORT_EXISTS : NATIVE_TEXT_EXPORT_OK;
	}
	fclose(retail);
	fclose(expanded);
	return result;
}
