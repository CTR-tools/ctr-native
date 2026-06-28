#include <platform/native_gpu_links.h>

#include <macros.h>

#include <stdio.h>
#include <stdlib.h>

#define NATIVE_GPU_LINK_MAX_RANGES          64
#define NATIVE_GPU_LINK_FIRST_DYNAMIC_TOKEN 0x00f00000u

struct NativeGpuLinkRange
{
	uintptr_t hostStart;
	uintptr_t hostEnd;
	uint32_t tokenStart;
	uint32_t tokenEnd;
};

global_variable struct NativeGpuLinkRange s_nativeGpuLinkRanges[NATIVE_GPU_LINK_MAX_RANGES];
global_variable int s_nativeGpuLinkRangeCount;
global_variable uint32_t s_nativeGpuLinkNextToken = NATIVE_GPU_LINK_FIRST_DYNAMIC_TOKEN;

static int NativeGpuLinks_AlignTokenSize(size_t size, uint32_t *tokenSizeOut)
{
	size_t alignedSize;

	if ((size == 0) || (size > NATIVE_GPU_LINK_FIRST_DYNAMIC_TOKEN) || (size > (SIZE_MAX - 3u)))
	{
		return 0;
	}

	alignedSize = (size + 3u) & ~(size_t)3u;
	if (alignedSize > NATIVE_GPU_LINK_FIRST_DYNAMIC_TOKEN)
	{
		return 0;
	}

	*tokenSizeOut = (uint32_t)alignedSize;
	return 1;
}

void NativeGpuLinks_Reset(void)
{
	s_nativeGpuLinkRangeCount = 0;
	s_nativeGpuLinkNextToken = NATIVE_GPU_LINK_FIRST_DYNAMIC_TOKEN;
}

int NativeGpuLinks_IsTerminator(uint32_t token)
{
	return (token & 0x00ffffffu) == NATIVE_GPU_LINK_TERMINATOR;
}

int NativeGpuLinks_RegisterRange(const void *hostStart, size_t size, uint32_t *tokenStartOut)
{
	uint32_t tokenSize;
	uintptr_t start;
	uintptr_t end;

	if ((hostStart == NULL) || (size == 0))
	{
		return 0;
	}

	if (s_nativeGpuLinkRangeCount >= NATIVE_GPU_LINK_MAX_RANGES)
	{
		return 0;
	}

	if (!NativeGpuLinks_AlignTokenSize(size, &tokenSize) || (tokenSize > s_nativeGpuLinkNextToken))
	{
		return 0;
	}

	start = (uintptr_t)hostStart;
	end = start + (uintptr_t)size;
	if (end < start)
	{
		return 0;
	}

	s_nativeGpuLinkNextToken -= tokenSize;

	struct NativeGpuLinkRange *range = &s_nativeGpuLinkRanges[s_nativeGpuLinkRangeCount++];
	range->hostStart = start;
	range->hostEnd = end;
	range->tokenStart = s_nativeGpuLinkNextToken;
	range->tokenEnd = s_nativeGpuLinkNextToken + tokenSize;

	if (tokenStartOut != NULL)
	{
		*tokenStartOut = range->tokenStart;
	}

	return 1;
}

void NativeGpuLinks_RegisterRangeChecked(const char *label, const void *hostStart, size_t size)
{
	if (NativeGpuLinks_RegisterRange(hostStart, size, NULL))
	{
		return;
	}

	fprintf(stderr, "[CTR Native] GPU link bridge failed to register %s range start=%p size=%zu\n", label, hostStart, size);
	abort();
}

static const struct NativeGpuLinkRange *NativeGpuLinks_FindHostRange(uintptr_t hostPtr)
{
	for (int i = 0; i < s_nativeGpuLinkRangeCount; i++)
	{
		const struct NativeGpuLinkRange *range = &s_nativeGpuLinkRanges[i];
		if ((hostPtr >= range->hostStart) && (hostPtr < range->hostEnd))
		{
			return range;
		}
	}

	return NULL;
}

static const struct NativeGpuLinkRange *NativeGpuLinks_FindTokenRange(uint32_t token)
{
	token &= 0x00ffffffu;

	for (int i = 0; i < s_nativeGpuLinkRangeCount; i++)
	{
		const struct NativeGpuLinkRange *range = &s_nativeGpuLinkRanges[i];
		if ((token >= range->tokenStart) && (token < range->tokenEnd))
		{
			return range;
		}
	}

	return NULL;
}

uint32_t NativeGpuLinks_FromHostPointer(const void *hostPtr)
{
	uintptr_t host = (uintptr_t)hostPtr;
	const struct NativeGpuLinkRange *range = NativeGpuLinks_FindHostRange(host);

	if (range != NULL)
	{
		return range->tokenStart + (uint32_t)(host - range->hostStart);
	}

	fprintf(stderr, "[CTR Native] GPU link bridge has no token for host pointer %p\n", hostPtr);
	abort();
}

void *NativeGpuLinks_ToHostPointer(uint32_t token)
{
	const struct NativeGpuLinkRange *range;

	token &= 0x00ffffffu;
	if (NativeGpuLinks_IsTerminator(token))
	{
		return NULL;
	}

	range = NativeGpuLinks_FindTokenRange(token);
	if (range != NULL)
	{
		return (void *)(range->hostStart + (uintptr_t)(token - range->tokenStart));
	}

	return NULL;
}

int NativeGpuLinks_IsRegisteredHostPointer(const void *hostPtr)
{
	return NativeGpuLinks_FindHostRange((uintptr_t)hostPtr) != NULL;
}

int NativeGpuLinks_IsRegisteredHostRange(const void *hostPtr, size_t size)
{
	uintptr_t start;
	uintptr_t end;

	if ((hostPtr == NULL) || (size == 0))
	{
		return 0;
	}

	start = (uintptr_t)hostPtr;
	end = start + (uintptr_t)size;
	if (end < start)
	{
		return 0;
	}

	for (int i = 0; i < s_nativeGpuLinkRangeCount; i++)
	{
		const struct NativeGpuLinkRange *range = &s_nativeGpuLinkRanges[i];
		if ((start >= range->hostStart) && (end <= range->hostEnd))
		{
			return 1;
		}
	}

	return 0;
}
