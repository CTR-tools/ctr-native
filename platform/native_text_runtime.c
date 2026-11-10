#include <platform/native_text_runtime.h>
#include <platform/native_text_catalog.h>
#include <platform/native_assets.h>
#include <platform/native_language_disc.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { NATIVE_TEXT_RUNTIME_MAX_FILE = 256 * 1024, NATIVE_TEXT_RUNTIME_VALUE_SIZE = 4097 };
enum { NATIVE_LANGUAGE_NAME_FILE_MAX = 4096, NATIVE_LANGUAGE_NAME_SIZE = 128 };

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
	char path[1200];
	int length = snprintf(path, sizeof(path), "%s/resources/text/%s/retail.toml", NativeAssets_GetExecutableDir(), id);
	FILE *file = NULL;
	if (length >= 0 && (size_t)length < sizeof(path))
	{
		file = fopen(path, "rb");
	}
	if (file == NULL)
	{
		snprintf(path, sizeof(path), "text/%s/retail.toml", id);
		file = NativeAssets_OpenHost(path, "rb");
	}
	if (file == NULL) { return 0; }
	fclose(file);
	return 1;
}

static int NativeTextRuntime_ReadLanguageName(const char *id, int shipped, char *output, size_t capacity)
{
	char path[1200];
	int length;
	FILE *file;
	if (shipped)
	{
		length = snprintf(path, sizeof(path), "%s/resources/text/%s/language.toml", NativeAssets_GetExecutableDir(), id);
		if (length < 0 || (size_t)length >= sizeof(path)) { return 0; }
		file = fopen(path, "rb");
	}
	else
	{
		length = snprintf(path, sizeof(path), "text/%s/language.toml", id);
		if (length < 0 || (size_t)length >= sizeof(path)) { return 0; }
		file = NativeAssets_OpenHost(path, "rb");
	}
	if (file == NULL) { return 0; }
	char bytes[NATIVE_LANGUAGE_NAME_FILE_MAX + 1];
	size_t size = fread(bytes, 1, sizeof(bytes), file);
	int failed = ferror(file) || size > NATIVE_LANGUAGE_NAME_FILE_MAX;
	fclose(file);
	if (failed) { return 0; }
	toml_option_t options = toml_default_option();
	options.check_utf8 = true;
	toml_set_option(options);
	toml_result_t result = toml_parse(bytes, (int)size);
	if (!result.ok)
	{
		toml_free(result);
		return 0;
	}
	toml_datum_t name = toml_get(result.toptab, "self_name");
	int valid = name.type == TOML_STRING && name.u.str.len > 0 &&
	            memchr(name.u.str.ptr, '\0', (size_t)name.u.str.len) == NULL &&
	            memchr(name.u.str.ptr, '\r', (size_t)name.u.str.len) == NULL &&
	            NativeTextCatalog_Encode(name.u.str.ptr, output, (unsigned int)capacity);
	toml_free(result);
	if (!valid) { output[0] = '\0'; }
	return valid;
}

const char *NativeTextRuntime_LanguageSelfName(int languageSlot)
{
	static char names[NATIVE_LANGUAGE_DUTCH + 1][NATIVE_LANGUAGE_NAME_SIZE];
	static unsigned char checked[NATIVE_LANGUAGE_DUTCH + 1];
	const char *id = NativeTextRuntime_LanguageId(languageSlot);
	if (id == NULL) { return NULL; }
	if (!checked[languageSlot])
	{
		checked[languageSlot] = 1;
		if (!NativeTextRuntime_ReadLanguageName(id, 0, names[languageSlot], sizeof(names[languageSlot])))
		{
			NativeTextRuntime_ReadLanguageName(id, 1, names[languageSlot], sizeof(names[languageSlot]));
		}
	}
	return names[languageSlot][0] != '\0' ? names[languageSlot] : NULL;
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
