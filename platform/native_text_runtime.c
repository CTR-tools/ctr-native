#include <platform/native_text_runtime.h>
#include <platform/native_text_catalog.h>
#include <platform/native_assets.h>
#include <platform/native_language_disc.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL3/SDL_filesystem.h>

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

static const struct { int slot; const char *id; } s_discLanguages[] = {
	{NATIVE_LANGUAGE_ENGLISH, "en"}, {NATIVE_LANGUAGE_FRENCH, "fr"},
	{NATIVE_LANGUAGE_GERMAN, "de"}, {NATIVE_LANGUAGE_ITALIAN, "it"},
	{NATIVE_LANGUAGE_SPANISH, "es"}, {NATIVE_LANGUAGE_DUTCH, "nl"},
};
static char s_customLanguageIds[NATIVE_TEXT_MAX_LANGUAGES][NATIVE_TEXT_LANGUAGE_ID_SIZE];
static int s_customLanguageCount;
static int s_languagesDiscovered;
static int NativeTextRuntime_ReadLanguageName(const char *id, int shipped, char *output, size_t capacity);

static FILE *NativeTextRuntime_Open(const char *id, int shipped, const char *fileName, char *path, size_t pathSize)
{
	int length = shipped ? snprintf(path, pathSize, "%s/resources/text/%s/%s", NativeAssets_GetExecutableDir(), id, fileName)
	                     : snprintf(path, pathSize, "text/%s/%s", id, fileName);
	if (length < 0 || (size_t)length >= pathSize)
	{
		return NULL;
	}
	return shipped ? fopen(path, "rb") : NativeAssets_OpenHost(path, "rb");
}

const char *NativeTextRuntime_LanguageId(int slot)
{
	for (size_t i = 0; i < sizeof(s_discLanguages) / sizeof(s_discLanguages[0]); i++)
	{
		if (s_discLanguages[i].slot == slot) { return s_discLanguages[i].id; }
	}
	if (slot >= NATIVE_TEXT_CUSTOM_FIRST && slot - NATIVE_TEXT_CUSTOM_FIRST < s_customLanguageCount)
	{
		return s_customLanguageIds[slot - NATIVE_TEXT_CUSTOM_FIRST];
	}
	return NULL;
}

int NativeTextRuntime_LanguageCount(void)
{
	return (int)(sizeof(s_discLanguages) / sizeof(s_discLanguages[0])) + s_customLanguageCount;
}

int NativeTextRuntime_LanguageAt(int index)
{
	int builtinCount = (int)(sizeof(s_discLanguages) / sizeof(s_discLanguages[0]));
	if (index < 0 || index >= builtinCount + s_customLanguageCount) { return -1; }
	return index < builtinCount ? s_discLanguages[index].slot : NATIVE_TEXT_CUSTOM_FIRST + index - builtinCount;
}

int NativeTextRuntime_LanguageSlot(const char *id)
{
	if (id == NULL) { return -1; }
	for (int i = 0; i < NativeTextRuntime_LanguageCount(); i++)
	{
		int slot = NativeTextRuntime_LanguageAt(i);
		if (strcmp(id, NativeTextRuntime_LanguageId(slot)) == 0) { return slot; }
	}
	return -1;
}

static SDL_EnumerationResult SDLCALL NativeTextRuntime_CollectLanguage(void *context, const char *directory, const char *id)
{
	(void)context;
	(void)directory;
	size_t size = strlen(id);
	int builtInCount = (int)(sizeof(s_discLanguages) / sizeof(s_discLanguages[0]));
	if (size == 0 || size >= NATIVE_TEXT_LANGUAGE_ID_SIZE || s_customLanguageCount >= NATIVE_TEXT_MAX_LANGUAGES - builtInCount)
	{
		return SDL_ENUM_CONTINUE;
	}
	for (size_t i = 0; i < size; i++)
	{
		if (!((id[i] >= 'a' && id[i] <= 'z') || (id[i] >= '0' && id[i] <= '9') || id[i] == '-' || id[i] == '_'))
		{
			return SDL_ENUM_CONTINUE;
		}
	}
	if (NativeTextRuntime_LanguageSlot(id) >= 0) { return SDL_ENUM_CONTINUE; }
	char name[NATIVE_LANGUAGE_NAME_SIZE] = {0};
	if (!NativeTextRuntime_ReadLanguageName(id, 0, name, sizeof(name)) &&
	    !NativeTextRuntime_ReadLanguageName(id, 1, name, sizeof(name))) { return SDL_ENUM_CONTINUE; }
	memcpy(s_customLanguageIds[s_customLanguageCount++], id, size + 1);
	return SDL_ENUM_CONTINUE;
}

static void NativeTextRuntime_ScanLanguages(const char *base)
{
	char path[1200];
	int n = snprintf(path, sizeof(path), "%s/text", base);
	if (n >= 0 && (size_t)n < sizeof(path))
	{
		SDL_EnumerateDirectory(path, NativeTextRuntime_CollectLanguage, NULL);
	}
}

static int NativeTextRuntime_CompareLanguageIds(const void *a, const void *b)
{
	return strcmp((const char *)a, (const char *)b);
}

void NativeTextRuntime_DiscoverLanguages(void)
{
	if (s_languagesDiscovered) { return; }
	s_languagesDiscovered = 1;
	char resources[1200];
	int n = snprintf(resources, sizeof(resources), "%s/resources", NativeAssets_GetExecutableDir());
	if (n >= 0 && (size_t)n < sizeof(resources)) { NativeTextRuntime_ScanLanguages(resources); }
	NativeTextRuntime_ScanLanguages(NativeAssets_GetAssetDir());
	qsort(s_customLanguageIds, (size_t)s_customLanguageCount, sizeof(s_customLanguageIds[0]), NativeTextRuntime_CompareLanguageIds);
}

static void NativeTextRuntime_Load(const char *id, int shipped, enum NativeTextCatalogKind kind,
                                   char values[][NATIVE_TEXT_RUNTIME_VALUE_SIZE], unsigned char *present)
{
	char path[1200];
	const char *fileName = kind == NATIVE_TEXT_CATALOG_RETAIL ? "retail.toml" : "expanded.toml";
	FILE *file = NativeTextRuntime_Open(id, shipped, fileName, path, sizeof(path));
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

static int NativeTextRuntime_HasFile(const char *id, const char *fileName)
{
	char path[1200];
	FILE *file = NativeTextRuntime_Open(id, 1, fileName, path, sizeof(path));
	if (file == NULL)
	{
		file = NativeTextRuntime_Open(id, 0, fileName, path, sizeof(path));
	}
	if (file == NULL) { return 0; }
	fclose(file);
	return 1;
}

int NativeTextRuntime_HasRetail(int languageSlot)
{
	const char *id = NativeTextRuntime_LanguageId(languageSlot);
	return id != NULL && NativeTextRuntime_HasFile(id, "retail.toml");
}

int NativeTextRuntime_HasText(int languageSlot)
{
	const char *id = NativeTextRuntime_LanguageId(languageSlot);
	return id != NULL && (NativeTextRuntime_HasFile(id, "retail.toml") || NativeTextRuntime_HasFile(id, "expanded.toml"));
}

static int NativeTextRuntime_ReadLanguageName(const char *id, int shipped, char *output, size_t capacity)
{
	char path[1200];
	FILE *file = NativeTextRuntime_Open(id, shipped, "language.toml", path, sizeof(path));
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
	static char names[NATIVE_TEXT_MAX_LANGUAGES + 8][NATIVE_LANGUAGE_NAME_SIZE];
	static unsigned char checked[NATIVE_TEXT_MAX_LANGUAGES + 8];
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
