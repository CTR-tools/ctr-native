#include <common.h>
#include <platform/native_text_catalog.h>
#include <platform/native_text_runtime.h>

static const char *s_languageResolved[LNG_EXPANDED_STRING_BASE + LNG_EXPANDED_STRING_COUNT];

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
	NativeTextRuntime_Reload(slot);
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
