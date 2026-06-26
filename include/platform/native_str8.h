#ifndef NATIVE_STR8_H
#define NATIVE_STR8_H

#include <macros.h>

#include <stddef.h>
#include <string.h>

typedef struct NativeStr8
{
	const u8 *ptr;
	size_t len;
} NativeStr8;

#define NATIVE_STR8_LIT(s) ((NativeStr8){(const u8 *)(s), sizeof(s) - 1u})

static inline NativeStr8 NativeStr8_FromCString(const char *text)
{
	NativeStr8 result = {0};

	if (text == NULL)
	{
		return result;
	}

	result.ptr = (const u8 *)text;
	while (text[result.len] != '\0')
	{
		result.len++;
	}

	return result;
}

static inline NativeStr8 NativeStr8_Skip(NativeStr8 text, size_t count)
{
	if (count > text.len)
	{
		count = text.len;
	}

	text.ptr += count;
	text.len -= count;
	return text;
}

static inline s32 NativeStr8_Equals(NativeStr8 left, NativeStr8 right)
{
	if (left.len != right.len)
	{
		return 0;
	}

	if (left.len == 0)
	{
		return 1;
	}

	return memcmp(left.ptr, right.ptr, left.len) == 0;
}

static inline s32 NativeStr8_StartsWith(NativeStr8 text, NativeStr8 prefix)
{
	if (prefix.len > text.len)
	{
		return 0;
	}

	if (prefix.len == 0)
	{
		return 1;
	}

	return memcmp(text.ptr, prefix.ptr, prefix.len) == 0;
}

static inline s32 NativeStr8_LastIndexOfAny(NativeStr8 text, u8 first, u8 second, size_t *indexOut)
{
	size_t index;

	for (index = text.len; index > 0; index--)
	{
		u8 byte = text.ptr[index - 1u];

		if ((byte == first) || (byte == second))
		{
			if (indexOut != NULL)
			{
				*indexOut = index - 1u;
			}
			return 1;
		}
	}

	return 0;
}

static inline s32 NativeStr8_CopyToCString(char *dst, size_t dstSize, NativeStr8 text)
{
	if ((dst == NULL) || (dstSize == 0) || (text.len >= dstSize))
	{
		return 0;
	}

	if (text.len != 0)
	{
		memcpy(dst, text.ptr, text.len);
	}

	dst[text.len] = '\0';
	return 1;
}

#endif
