#ifndef NATIVE_PATH_H
#define NATIVE_PATH_H

#include "platform/native_str8.h"

static inline s32 NativePath_IsSeparator(u8 byte)
{
	return (byte == '/') || (byte == '\\');
}

static inline NativeStr8 NativePath_TrimTrailingSeparators(NativeStr8 path)
{
	while ((path.len > 1) && NativePath_IsSeparator(path.ptr[path.len - 1u]))
	{
		if ((path.len == 3) && (path.ptr[1] == ':'))
		{
			break;
		}

		path.len--;
	}

	return path;
}

static inline NativeStr8 NativePath_SkipLeadingSeparators(NativeStr8 path)
{
	while ((path.len != 0) && NativePath_IsSeparator(path.ptr[0]))
	{
		path = NativeStr8_Skip(path, 1);
	}

	return path;
}

static inline s32 NativePath_NormalizeSlashes(char *dst, size_t dstSize, NativeStr8 path)
{
	size_t i;

	if ((dst == NULL) || (dstSize == 0) || (path.len >= dstSize))
	{
		return 0;
	}

	if ((path.ptr == NULL) && (path.len != 0))
	{
		return 0;
	}

	for (i = 0; i < path.len; i++)
	{
		u8 byte = path.ptr[i];
		dst[i] = (char)((byte == '\\') ? '/' : byte);
	}

	dst[path.len] = '\0';
	return 1;
}

static inline s32 NativePath_Join(char *dst, size_t dstSize, NativeStr8 left, NativeStr8 right)
{
	NativeStr8 rightPart = NativePath_SkipLeadingSeparators(right);
	size_t writeAt;
	s32 needsSeparator;

	if ((dst == NULL) || (dstSize == 0))
	{
		return 0;
	}

	if (((left.ptr == NULL) && (left.len != 0)) || ((rightPart.ptr == NULL) && (rightPart.len != 0)))
	{
		return 0;
	}

	if (left.len == 0)
	{
		return NativePath_NormalizeSlashes(dst, dstSize, rightPart);
	}

	needsSeparator = (rightPart.len != 0) && !NativePath_IsSeparator(left.ptr[left.len - 1u]);
	if (left.len + (size_t)needsSeparator + rightPart.len >= dstSize)
	{
		return 0;
	}

	if (!NativePath_NormalizeSlashes(dst, dstSize, left))
	{
		return 0;
	}

	writeAt = left.len;
	if (needsSeparator != 0)
	{
		dst[writeAt++] = '/';
	}

	if (rightPart.len != 0)
	{
		size_t i;

		for (i = 0; i < rightPart.len; i++)
		{
			u8 byte = rightPart.ptr[i];
			dst[writeAt + i] = (char)((byte == '\\') ? '/' : byte);
		}
	}

	dst[writeAt + rightPart.len] = '\0';
	return 1;
}

static inline s32 NativePath_Parent(char *dst, size_t dstSize, NativeStr8 path)
{
	size_t sepIndex;

	path = NativePath_TrimTrailingSeparators(path);
	if (!NativeStr8_LastIndexOfAny(path, '/', '\\', &sepIndex))
	{
		return 0;
	}

	if (sepIndex == 0)
	{
		path.len = 1;
	}
	else if ((sepIndex == 2) && (path.len > 2) && (path.ptr[1] == ':'))
	{
		path.len = 3;
	}
	else
	{
		path.len = sepIndex;
	}

	return NativePath_NormalizeSlashes(dst, dstSize, path);
}

#endif
