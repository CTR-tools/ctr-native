#include <platform/native_text_runtime.h>
#include <platform/native_text_catalog.h>
#include <platform/native_assets.h>
#include <platform/native_language_disc.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { NATIVE_TEXT_RUNTIME_MAX_FILE = 256 * 1024, NATIVE_TEXT_RUNTIME_VALUE_SIZE = 4097 };

struct NativeTextRuntimeValues
{
	char values[LNG_EXPANDED_STRING_COUNT][NATIVE_TEXT_RUNTIME_VALUE_SIZE];
	unsigned char present[LNG_EXPANDED_STRING_COUNT];
};

static struct NativeTextRuntimeValues s_nativeTextValues[NATIVE_TEXT_SOURCE_COUNT];

static const char *NativeTextRuntime_LanguageId(int slot)
{
	switch (slot)
	{
	case NATIVE_LANGUAGE_ENGLISH: return "en";
	case NATIVE_LANGUAGE_FRENCH: return "fr";
	case NATIVE_LANGUAGE_GERMAN: return "de";
	case NATIVE_LANGUAGE_ITALIAN: return "it";
	case NATIVE_LANGUAGE_SPANISH: return "es";
	case NATIVE_LANGUAGE_DUTCH: return "nl";
	default: return NULL;
	}
}

static void NativeTextRuntime_Load(const char *id, int shipped, struct NativeTextRuntimeValues *destination)
{
	char path[1200];
	FILE *file;
	if (shipped)
	{
		int length = snprintf(path, sizeof(path), "%s/resources/text/%s/expanded.toml", NativeAssets_GetExecutableDir(), id);
		if (length < 0 || (size_t)length >= sizeof(path))
		{
			return;
		}
		file = fopen(path, "rb");
	}
	else
	{
		snprintf(path, sizeof(path), "text/%s/expanded.toml", id);
		file = NativeAssets_OpenHost(path, "rb");
	}
	if (file == NULL)
	{
		if (shipped && strcmp(id, "en") == 0)
		{
			fprintf(stderr, "[Language] Missing English text catalog: %s\n", path);
		}
		return;
	}
	char *bytes = malloc(NATIVE_TEXT_RUNTIME_MAX_FILE + 1);
	if (bytes == NULL)
	{
		fclose(file);
		return;
	}
	size_t size = fread(bytes, 1, NATIVE_TEXT_RUNTIME_MAX_FILE + 1, file);
	int failed = ferror(file) || size > NATIVE_TEXT_RUNTIME_MAX_FILE;
	fclose(file);
	struct NativeTextCatalog catalog = {0};
	int invalid = 0;
	if (failed || !NativeTextCatalog_Parse(&catalog, bytes, (int)size, &invalid))
	{
		fprintf(stderr, "[Language] Cannot parse %s\n", path);
		free(bytes);
		return;
	}
	free(bytes);
	for (int i = 0; i < LNG_EXPANDED_STRING_COUNT; i++)
	{
		int key = LNG_EXPANDED_STRING_BASE + i;
		const char *value = NativeTextCatalog_Get(&catalog, key);
		if (value != NULL)
		{
			if (NativeTextCatalog_Encode(value, destination->values[i], sizeof(destination->values[i])))
			{
				destination->present[i] = 1;
			}
			else
			{
				fprintf(stderr, "[Language] Unsupported text in %s: %s\n", path, NativeTextCatalog_KeyName(key));
			}
		}
	}
	if (invalid != 0)
	{
		fprintf(stderr, "[Language] Ignored %d invalid entries in %s\n", invalid, path);
	}
	NativeTextCatalog_Clear(&catalog);
}

void NativeTextRuntime_Reload(int languageSlot)
{
	struct NativeTextRuntimeValues next[NATIVE_TEXT_SOURCE_COUNT] = {0};
	const char *id = NativeTextRuntime_LanguageId(languageSlot);
	NativeTextRuntime_Load("en", 0, &next[NATIVE_TEXT_ENGLISH_OVERRIDE]);
	NativeTextRuntime_Load("en", 1, &next[NATIVE_TEXT_ENGLISH_SHIPPED]);
	if (id != NULL && strcmp(id, "en") != 0)
	{
		NativeTextRuntime_Load(id, 0, &next[NATIVE_TEXT_SELECTED_OVERRIDE]);
		NativeTextRuntime_Load(id, 1, &next[NATIVE_TEXT_SELECTED_SHIPPED]);
	}
	memcpy(s_nativeTextValues, next, sizeof(next));
}

const char *NativeTextRuntime_Get(enum NativeTextSource source, int key)
{
	if (source < 0 || source >= NATIVE_TEXT_SOURCE_COUNT || key < LNG_EXPANDED_STRING_BASE || key >= LNG_EXPANDED_STRING_BASE + LNG_EXPANDED_STRING_COUNT)
	{
		return NULL;
	}
	int index = key - LNG_EXPANDED_STRING_BASE;
	return s_nativeTextValues[source].present[index] ? s_nativeTextValues[source].values[index] : NULL;
}
