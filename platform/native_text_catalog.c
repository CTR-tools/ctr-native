#include <platform/native_text_catalog.h>
#include <tomlc17.h>

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>

enum { NATIVE_TEXT_CATALOG_MAX_VALUE_BYTES = 4096 };

static const char *const s_nativeTextKeys[LNG_EXPANDED_STRING_END] = {
#define RETAIL_TEXT_KEY(name, value) [name] = #name + 4,
#include <retail_text_keys.h>
#undef RETAIL_TEXT_KEY
#define EXPANDED_TEXT_KEY(name) [name] = #name + sizeof("LNG_EXPANDED_") - 1,
#include <expanded_text_keys.h>
#undef EXPANDED_TEXT_KEY
};

static int NativeTextCatalog_ValidatePlaceholders(int key, const char *value)
{
	const char *first = NULL, *second = NULL;
	switch (key)
	{
	case LNG_EXPANDED_PATCH_READY_COUNT:
	case LNG_EXPANDED_PATCH_IMPORTED_MUSIC_COUNT: first = "tracks"; break;
	case LNG_EXPANDED_PATCH_IMPORTED_SKIPPED: first = "tracks"; second = "skipped_music"; break;
	case LNG_EXPANDED_TRACKS_REFRESH_SUMMARY: first = "tracks"; second = "rejected"; break;
	case LNG_EXPANDED_CONTROLS_PLAYER_TITLE: first = "player"; break;
	default: return 1;
	}
	int seenFirst = 0, seenSecond = second == NULL;
	for (const char *p = value; *p != '\0'; p++)
	{
		if (*p == '}') { return 0; }
		if (*p != '{') { continue; }
		const char *end = strchr(p + 1, '}');
		if (end == NULL) { return 0; }
		size_t size = (size_t)(end - p - 1);
		if (strlen(first) == size && memcmp(p + 1, first, size) == 0) { seenFirst = 1; }
		else if (second != NULL && strlen(second) == size && memcmp(p + 1, second, size) == 0) { seenSecond = 1; }
		else { return 0; }
		p = end;
	}
	return seenFirst && seenSecond;
}

static void NativeTextCatalog_ClearValues(char **values, int count)
{
	for (int i = 0; i < count; i++)
	{
		free(values[i]);
		values[i] = NULL;
	}
}

int NativeTextCatalog_Parse(struct NativeTextCatalog *catalog, enum NativeTextCatalogKind kind, const char *text, int length, int *invalidEntries)
{
	if (catalog == NULL || text == NULL || length < 0 ||
	    (kind != NATIVE_TEXT_CATALOG_RETAIL && kind != NATIVE_TEXT_CATALOG_EXPANDED)) { return 0; }
	int start = kind == NATIVE_TEXT_CATALOG_RETAIL ? 0 : LNG_EXPANDED_STRING_BASE;
	int end = kind == NATIVE_TEXT_CATALOG_RETAIL ? LNG_RETAIL_STRING_COUNT : LNG_EXPANDED_STRING_END;
	toml_option_t options = toml_default_option();
	options.check_utf8 = true;
	toml_set_option(options);
	toml_result_t result = toml_parse(text, length);
	if (!result.ok)
	{
		toml_free(result);
		return 0;
	}
	char *next[LNG_EXPANDED_STRING_END] = {0};
	int invalid = 0;
	for (int i = 0; i < result.toptab.u.tab.size; i++)
	{
		const char *name = result.toptab.u.tab.key[i];
		int nameLength = result.toptab.u.tab.len[i];
		toml_datum_t value = result.toptab.u.tab.value[i];
		int key = -1;
		for (int j = start; j < end; j++)
		{
			if (s_nativeTextKeys[j] != NULL && (int)strlen(s_nativeTextKeys[j]) == nameLength && memcmp(name, s_nativeTextKeys[j], (size_t)nameLength) == 0)
			{
				key = j;
				break;
			}
		}
		if (key < 0 || value.type != TOML_STRING ||
		    value.u.str.len > NATIVE_TEXT_CATALOG_MAX_VALUE_BYTES || memchr(value.u.str.ptr, '\0', (size_t)value.u.str.len) != NULL)
		{
			invalid++;
			continue;
		}
		next[key] = malloc((size_t)value.u.str.len + 1);
		if (next[key] == NULL)
		{
			NativeTextCatalog_ClearValues(next, LNG_EXPANDED_STRING_END);
			toml_free(result);
			return 0;
		}
		memcpy(next[key], value.u.str.ptr, (size_t)value.u.str.len);
		next[key][value.u.str.len] = '\0';
		if (!NativeTextCatalog_ValidatePlaceholders(key, next[key]))
		{
			free(next[key]);
			next[key] = NULL;
			invalid++;
		}
	}
	toml_free(result);
	NativeTextCatalog_ClearValues(catalog->values + start, end - start);
	memcpy(catalog->values + start, next + start, (size_t)(end - start) * sizeof(*next));
	if (invalidEntries != NULL) { *invalidEntries = invalid; }
	return 1;
}

void NativeTextCatalog_Clear(struct NativeTextCatalog *catalog)
{
	if (catalog != NULL) { NativeTextCatalog_ClearValues(catalog->values, LNG_EXPANDED_STRING_END); }
}

const char *NativeTextCatalog_Get(const struct NativeTextCatalog *catalog, int key)
{
	if (catalog == NULL || key < 0 || key >= LNG_EXPANDED_STRING_END)
	{
		return NULL;
	}
	return catalog->values[key];
}

const char *NativeTextCatalog_KeyName(int key)
{
	return key >= 0 && key < LNG_EXPANDED_STRING_END && s_nativeTextKeys[key] != NULL ? s_nativeTextKeys[key] : "";
}

int NativeTextCatalog_Encode(const char *text, char *output, unsigned int capacity)
{
	if (text == NULL || output == NULL || capacity == 0)
	{
		return 0;
	}
	unsigned int used = 0;
	const unsigned char *p = (const unsigned char *)text;
	while (*p != 0)
	{
		unsigned char bytes[2];
		unsigned int count = 1;
		if (*p >= 0x20 && *p <= 0x7e)
		{
			bytes[0] = *p++;
		}
		else if (*p == '\r')
		{
			bytes[0] = *p++;
		}
		else if (p[0] == 0xc3 && (p[1] == 0x91 || p[1] == 0xb1))
		{
			bytes[0] = 3;
			bytes[1] = p[1] == 0x91 ? 'N' : 'n';
			count = 2;
			p += 2;
		}
		else if (p[0] == 0xc2 && (p[1] == 0xa1 || p[1] == 0xbf))
		{
			bytes[0] = p[1] == 0xa1 ? '#' : '$';
			p += 2;
		}
		else if (p[0] == 0xee && p[1] >= 0x80 && p[1] <= 0x83 && p[2] >= 0x80 && p[2] <= 0xbf)
		{
			bytes[0] = (unsigned char)(((p[1] - 0x80) << 6) | (p[2] - 0x80));
			if (bytes[0] == 0) { return 0; }
			p += 3;
		}
		else
		{
			return 0;
		}
		if (used + count >= capacity)
		{
			return 0;
		}
		memcpy(output + used, bytes, count);
		used += count;
	}
	output[used] = '\0';
	return 1;
}

int NativeTextCatalog_Decode(const char *text, char *output, unsigned int capacity)
{
	if (text == NULL || output == NULL || capacity == 0) { return 0; }
	const unsigned char *p = (const unsigned char *)text;
	unsigned int used = 0;
	while (*p != 0)
	{
		unsigned char bytes[3];
		unsigned int count = 1;
		if (*p == 3 && (p[1] == 'N' || p[1] == 'n'))
		{
			bytes[0] = 0xc3;
			bytes[1] = p[1] == 'N' ? 0x91 : 0xb1;
			count = 2;
			p += 2;
		}
		else if ((*p >= 0x20 && *p <= 0x7e) || *p == '\r')
		{
			bytes[0] = *p++;
		}
		else
		{
			// Preserve unverified font bytes in the Unicode private-use area;
			// exported TOML spells them as \uE0xx rather than invisible glyphs.
			bytes[0] = 0xee;
			bytes[1] = (unsigned char)(0x80 | (*p >> 6));
			bytes[2] = (unsigned char)(0x80 | (*p & 0x3f));
			count = 3;
			p++;
		}
		if (used + count >= capacity)
		{
			output[0] = '\0';
			return 0;
		}
		memcpy(output + used, bytes, count);
		used += count;
	}
	output[used] = '\0';
	return 1;
}

int NativeTextCatalog_WriteTemplateEntry(FILE *file, int key, const char *text)
{
	if (file == NULL || text == NULL || key < 0 || key >= LNG_EXPANDED_STRING_END) { return 0; }
	const char *name = NativeTextCatalog_KeyName(key);
	if (name[0] == '\0') { return 0; }
	size_t length = strlen(text);
	if (length > (SIZE_MAX - 1) / 3 || length * 3 + 1 > UINT_MAX) { return 0; }
	char *decoded = malloc(length * 3 + 1);
	if (decoded == NULL) { return 0; }
	if (!NativeTextCatalog_Decode(text, decoded, (unsigned int)(length * 3 + 1)))
	{
		free(decoded);
		return 0;
	}
	int ok = fprintf(file, "# %s = \"", name) >= 0;
	for (const unsigned char *p = (const unsigned char *)decoded; ok && *p != 0; p++)
	{
		if (p[0] == 0xee && p[1] >= 0x80 && p[1] <= 0x83 && p[2] >= 0x80 && p[2] <= 0xbf)
		{
			unsigned int byte = (unsigned int)((p[1] - 0x80) << 6) | (unsigned int)(p[2] - 0x80);
			ok = fprintf(file, "\\uE0%02X", byte) >= 0;
			p += 2;
		}
		else if (*p == '"' || *p == '\\') { ok = fputc('\\', file) != EOF && fputc(*p, file) != EOF; }
		else if (*p == '\r') { ok = fputs("\\r", file) >= 0; }
		else { ok = fputc(*p, file) != EOF; }
	}
	if (ok) { ok = fputs("\"\n", file) >= 0; }
	free(decoded);
	return ok;
}
