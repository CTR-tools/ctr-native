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
static struct NativeRetailTextRuntimeValues
{
	char values[LNG_RETAIL_STRING_COUNT][NATIVE_TEXT_RUNTIME_VALUE_SIZE];
	unsigned char present[LNG_RETAIL_STRING_COUNT];
} s_nativeRetailText[2];

const char *NativeTextRuntime_LanguageId(int slot)
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

static void NativeTextRuntime_Load(const char *id, int shipped, enum NativeTextCatalogKind kind,
                                   char values[][NATIVE_TEXT_RUNTIME_VALUE_SIZE], unsigned char *present)
{
	char path[1200];
	FILE *file;
	const char *fileName = kind == NATIVE_TEXT_CATALOG_RETAIL ? "retail.toml" : "expanded.toml";
	if (shipped)
	{
		int length = snprintf(path, sizeof(path), "%s/resources/text/%s/%s", NativeAssets_GetExecutableDir(), id, fileName);
		if (length < 0 || (size_t)length >= sizeof(path))
		{
			return;
		}
		file = fopen(path, "rb");
	}
	else
	{
		snprintf(path, sizeof(path), "text/%s/%s", id, fileName);
		file = NativeAssets_OpenHost(path, "rb");
	}
	if (file == NULL)
	{
		if (shipped && kind == NATIVE_TEXT_CATALOG_EXPANDED && strcmp(id, "en") == 0)
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
	if (failed || !NativeTextCatalog_Parse(&catalog, kind, bytes, (int)size, &invalid))
	{
		fprintf(stderr, "[Language] Cannot parse %s\n", path);
		free(bytes);
		return;
	}
	free(bytes);
	int count = kind == NATIVE_TEXT_CATALOG_RETAIL ? LNG_RETAIL_STRING_COUNT : LNG_EXPANDED_STRING_COUNT;
	int first = kind == NATIVE_TEXT_CATALOG_RETAIL ? 0 : LNG_EXPANDED_STRING_BASE;
	for (int i = 0; i < count; i++)
	{
		int key = first + i;
		const char *value = NativeTextCatalog_Get(&catalog, key);
		if (value != NULL)
		{
			if (NativeTextCatalog_Encode(value, values[i], NATIVE_TEXT_RUNTIME_VALUE_SIZE))
			{
				present[i] = 1;
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

int NativeTextRuntime_HasRetail(int languageSlot)
{
	const char *id = NativeTextRuntime_LanguageId(languageSlot);
	if (id == NULL) { return 0; }
	struct NativeRetailTextRuntimeValues *probe = calloc(1, sizeof(*probe));
	if (probe == NULL) { return 0; }
	NativeTextRuntime_Load(id, 1, NATIVE_TEXT_CATALOG_RETAIL, probe->values, probe->present);
	NativeTextRuntime_Load(id, 0, NATIVE_TEXT_CATALOG_RETAIL, probe->values, probe->present);
	int found = 0;
	for (int i = 0; i < LNG_RETAIL_STRING_COUNT; i++)
	{
		if (probe->present[i]) { found = 1; break; }
	}
	free(probe);
	return found;
}

void NativeTextRuntime_Reload(int languageSlot)
{
	static struct NativeTextRuntimeValues next[NATIVE_TEXT_SOURCE_COUNT];
	static struct NativeRetailTextRuntimeValues retailNext[2];
	memset(next, 0, sizeof(next));
	memset(retailNext, 0, sizeof(retailNext));
	const char *id = NativeTextRuntime_LanguageId(languageSlot);
	NativeTextRuntime_Load("en", 0, NATIVE_TEXT_CATALOG_EXPANDED, next[NATIVE_TEXT_ENGLISH_OVERRIDE].values, next[NATIVE_TEXT_ENGLISH_OVERRIDE].present);
	NativeTextRuntime_Load("en", 1, NATIVE_TEXT_CATALOG_EXPANDED, next[NATIVE_TEXT_ENGLISH_SHIPPED].values, next[NATIVE_TEXT_ENGLISH_SHIPPED].present);
	if (id != NULL && strcmp(id, "en") != 0)
	{
		NativeTextRuntime_Load(id, 0, NATIVE_TEXT_CATALOG_EXPANDED, next[NATIVE_TEXT_SELECTED_OVERRIDE].values, next[NATIVE_TEXT_SELECTED_OVERRIDE].present);
		NativeTextRuntime_Load(id, 1, NATIVE_TEXT_CATALOG_EXPANDED, next[NATIVE_TEXT_SELECTED_SHIPPED].values, next[NATIVE_TEXT_SELECTED_SHIPPED].present);
	}
	if (id != NULL)
	{
		NativeTextRuntime_Load(id, 1, NATIVE_TEXT_CATALOG_RETAIL, retailNext[1].values, retailNext[1].present);
		NativeTextRuntime_Load(id, 0, NATIVE_TEXT_CATALOG_RETAIL, retailNext[0].values, retailNext[0].present);
	}
	memcpy(s_nativeTextValues, next, sizeof(next));
	memcpy(s_nativeRetailText, retailNext, sizeof(retailNext));
}

const char *NativeTextRuntime_Get(enum NativeTextSource source, int key)
{
	if ((source == NATIVE_TEXT_SELECTED_SHIPPED || source == NATIVE_TEXT_SELECTED_OVERRIDE) && key >= 0 && key < LNG_RETAIL_STRING_COUNT)
	{
		int index = source == NATIVE_TEXT_SELECTED_SHIPPED ? 1 : 0;
		return s_nativeRetailText[index].present[key] ? s_nativeRetailText[index].values[key] : NULL;
	}
	if (source < 0 || source >= NATIVE_TEXT_SOURCE_COUNT || key < LNG_EXPANDED_STRING_BASE || key >= LNG_EXPANDED_STRING_BASE + LNG_EXPANDED_STRING_COUNT)
	{
		return NULL;
	}
	int index = key - LNG_EXPANDED_STRING_BASE;
	return s_nativeTextValues[source].present[index] ? s_nativeTextValues[source].values[index] : NULL;
}
