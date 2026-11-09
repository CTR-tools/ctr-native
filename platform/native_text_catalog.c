#include <platform/native_text_catalog.h>
#include <tomlc17.h>

#include <stdlib.h>
#include <string.h>

enum { NATIVE_TEXT_CATALOG_MAX_VALUE_BYTES = 4096 };

// Same order as the Expanded IDs in namespace_Lng.h.
static const char *const s_nativeExpandedKeys[] = {
	"MODE",
	"CHAOS",
	"REGULAR",
	"CHAOS_RULES",
	"SUPER_HARD",
	"ULTRA_HARD",
};
_Static_assert(sizeof(s_nativeExpandedKeys) / sizeof(s_nativeExpandedKeys[0]) == LNG_EXPANDED_STRING_COUNT,
               "Expanded key count must match the string IDs");

void NativeTextCatalog_Clear(struct NativeTextCatalog *catalog)
{
	if (catalog == NULL)
	{
		return;
	}
	for (int i = 0; i < LNG_EXPANDED_STRING_COUNT; i++)
	{
		free(catalog->values[i]);
		catalog->values[i] = NULL;
	}
}

int NativeTextCatalog_Parse(struct NativeTextCatalog *catalog, const char *text, int length, int *invalidEntries)
{
	if (catalog == NULL || text == NULL || length < 0)
	{
		return 0;
	}
	toml_option_t options = toml_default_option();
	options.check_utf8 = true;
	toml_set_option(options);
	toml_result_t result = toml_parse(text, length);
	if (!result.ok)
	{
		toml_free(result);
		return 0;
	}

	struct NativeTextCatalog next = {0};
	int invalid = 0;
	for (int i = 0; i < result.toptab.u.tab.size; i++)
	{
		const char *name = result.toptab.u.tab.key[i];
		int nameLength = result.toptab.u.tab.len[i];
		toml_datum_t value = result.toptab.u.tab.value[i];
		int key = 0;
		while (key < LNG_EXPANDED_STRING_COUNT &&
		       ((int)strlen(s_nativeExpandedKeys[key]) != nameLength || memcmp(name, s_nativeExpandedKeys[key], (size_t)nameLength) != 0))
		{
			key++;
		}
		if (key == LNG_EXPANDED_STRING_COUNT || value.type != TOML_STRING || value.u.str.len > NATIVE_TEXT_CATALOG_MAX_VALUE_BYTES ||
		    memchr(value.u.str.ptr, '\0', (size_t)value.u.str.len) != NULL)
		{
			invalid++;
			continue;
		}
		next.values[key] = malloc((size_t)value.u.str.len + 1);
		if (next.values[key] == NULL)
		{
			NativeTextCatalog_Clear(&next);
			toml_free(result);
			return 0;
		}
		memcpy(next.values[key], value.u.str.ptr, (size_t)value.u.str.len);
		next.values[key][value.u.str.len] = '\0';
	}
	toml_free(result);
	NativeTextCatalog_Clear(catalog);
	*catalog = next;
	if (invalidEntries != NULL)
	{
		*invalidEntries = invalid;
	}
	return 1;
}

const char *NativeTextCatalog_Get(const struct NativeTextCatalog *catalog, int key)
{
	if (catalog == NULL || key < LNG_EXPANDED_STRING_BASE || key >= LNG_EXPANDED_STRING_BASE + LNG_EXPANDED_STRING_COUNT)
	{
		return NULL;
	}
	return catalog->values[key - LNG_EXPANDED_STRING_BASE];
}

const char *NativeTextCatalog_KeyName(int key)
{
	return key >= LNG_EXPANDED_STRING_BASE && key < LNG_EXPANDED_STRING_BASE + LNG_EXPANDED_STRING_COUNT
	           ? s_nativeExpandedKeys[key - LNG_EXPANDED_STRING_BASE] : "";
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
