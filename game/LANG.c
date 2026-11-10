#include <common.h>
#include <platform/native_text_catalog.h>
#include <platform/native_text_runtime.h>

static const char *s_languageResolved[LNG_EXPANDED_STRING_BASE + LNG_EXPANDED_STRING_COUNT];
static char *s_languageRetailOriginal[LNG_RETAIL_STRING_COUNT];

// Compare complete printf directives in order. Unsupported directives are
// rejected rather than guessing their argument types or positional semantics.
static int LANG_NextFormat(const char **text, const char **start, size_t *length)
{
	const char *p = strchr(*text, '%');
	if (p == NULL) { return 0; }
	*start = p++;
	if (*p == '%')
	{
		p++;
	}
	else
	{
		while (*p != '\0' && strchr("-+ #0", *p) != NULL) { p++; }
		while (*p >= '0' && *p <= '9') { p++; }
		if (*p == '.')
		{
			p++;
			while (*p >= '0' && *p <= '9') { p++; }
		}
		if (*p != '\0' && strchr("hljztL", *p) != NULL)
		{
			char modifier = *p++;
			if ((modifier == 'h' || modifier == 'l') && *p == modifier) { p++; }
		}
		if (*p == '\0' || strchr("diouxXfFeEgGaAcsp", *p) == NULL) { return -1; }
		p++;
	}
	*length = (size_t)(p - *start);
	*text = p;
	return 1;
}

static bool LANG_FormatsMatch(const char *original, const char *replacement)
{
	for (;;)
	{
		const char *a, *b;
		size_t aLength, bLength;
		int aResult = LANG_NextFormat(&original, &a, &aLength);
		int bResult = LANG_NextFormat(&replacement, &b, &bLength);
		if (aResult < 0 || bResult < 0 || aResult != bResult) { return false; }
		if (aResult == 0) { return true; }
		if (aLength != bLength || memcmp(a, b, aLength) != 0) { return false; }
	}
}

static void LANG_RestoreRetail(void)
{
	if (sdata->lngStrings == NULL) { return; }
	for (int i = 0; i < LNG_RETAIL_STRING_COUNT && i < sdata->numLngStrings; i++)
	{
		const char *previous = NativeTextRuntime_Get(NATIVE_TEXT_SELECTED_SHIPPED, i);
		if (previous != NULL && sdata->lngStrings[i] == previous)
		{
			sdata->lngStrings[i] = s_languageRetailOriginal[i];
		}
	}
}

static void LANG_ApplyRetail(int slot)
{
	if (sdata->lngStrings == NULL) { return; }
	bool preferDisc = g_config.retailTextPriority == RETAIL_TEXT_PRIORITY_ISO_EU && LOAD_RetailDiscActive();
	(void)slot;
	for (int i = 0; i < LNG_RETAIL_STRING_COUNT && i < sdata->numLngStrings; i++)
	{
		char *original = sdata->lngStrings[i];
		const char *replacement = NativeTextRuntime_Get(NATIVE_TEXT_SELECTED_SHIPPED, i);
		s_languageRetailOriginal[i] = original;
		if (!preferDisc && original != NULL && replacement != NULL && LANG_FormatsMatch(original, replacement))
		{
			sdata->lngStrings[i] = (char *)replacement;
		}
	}
}

static void LANG_Rebuild(void)
{
	for (int i = 0; i < LNG_RETAIL_STRING_COUNT; i++)
	{
		s_languageResolved[i] = sdata->lngStrings != NULL && i < sdata->numLngStrings ? sdata->lngStrings[i] : NULL;
	}
	for (int i = 0; i < LNG_EXPANDED_STRING_COUNT; i++)
	{
		int key = LNG_EXPANDED_STRING_BASE + i;
		const char *value = NativeTextRuntime_Get(NATIVE_TEXT_SELECTED_OVERRIDE, key);
		if (value == NULL)
		{
			value = NativeTextRuntime_Get(NATIVE_TEXT_SELECTED_SHIPPED, key);
		}
		if (value == NULL)
		{
			value = NativeTextRuntime_Get(NATIVE_TEXT_ENGLISH_OVERRIDE, key);
		}
		if (value == NULL)
		{
			value = NativeTextRuntime_Get(NATIVE_TEXT_ENGLISH_SHIPPED, key);
		}
		s_languageResolved[key] = value != NULL ? value : NativeTextCatalog_KeyName(key);
	}
}

void LANG_Refresh(int slot)
{
	LANG_RestoreRetail();
	NativeTextRuntime_Reload(slot);
	LANG_ApplyRetail(slot);
	LANG_Rebuild();
}

const char *LANG_GetString(s32 index)
{
	if (index < 0 || index >= (s32)len(s_languageResolved) || s_languageResolved[index] == NULL)
	{
		return "";
	}
	return s_languageResolved[index];
}
