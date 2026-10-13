#include <macros.h>
#include <platform/native_pgxp.h>

#include <stdlib.h>
#include <string.h>

// Recently read or stored GTE results, for CPU code that copies SXY words around
// by value before writing them into a packet.
#define NATIVE_PGXP_RECENT_COUNT 64
// Precise vertices stored outside primitive memory (scratchpad, stack, small
// packet buffers). These are consumed quickly, so a direct-mapped cache is
// enough; a collision only costs the evicted vertex its precision.
#define NATIVE_PGXP_CACHE_BITS  12
#define NATIVE_PGXP_CACHE_COUNT (1u << NATIVE_PGXP_CACHE_BITS)

typedef struct
{
	NativePgxpVertex vertex;
	b32 valid;
} NativePgxpRegister;

typedef struct
{
	NativePgxpVertex vertex;
	u32 generation;
} NativePgxpPrimSlot;

typedef struct
{
	const void *key;
	NativePgxpVertex vertex;
	u32 generation;
} NativePgxpCacheSlot;

typedef struct
{
	const void *start;
	size_t size;
	NativePgxpPrimSlot *slots;
} NativePgxpPrimRegion;

global_variable NativePgxpRegister s_pgxpFifo[3];
global_variable NativePgxpVertex s_pgxpRecent[NATIVE_PGXP_RECENT_COUNT];
global_variable u32 s_pgxpRecentGeneration[NATIVE_PGXP_RECENT_COUNT];
global_variable u32 s_pgxpRecentHead;
global_variable NativePgxpCacheSlot s_pgxpCache[NATIVE_PGXP_CACHE_COUNT];
global_variable NativePgxpPrimRegion s_pgxpRegions[NATIVE_PGXP_PRIM_REGION_COUNT];
// Shadow entries are only trusted for the frame that wrote them; the GPU parses
// every frame's packets before the next frame starts building one.
global_variable u32 s_pgxpGeneration = 1;
global_variable b32 s_pgxpWorldPhase;

void NativePgxp_SetPrimRegion(int index, const void *start, size_t size)
{
	if ((index < 0) || (index >= NATIVE_PGXP_PRIM_REGION_COUNT))
	{
		return;
	}

	NativePgxpPrimRegion *region = &s_pgxpRegions[index];
	const size_t regionSize = (start != NULL) ? (size & ~(size_t)3) : 0;

	if ((region->start == start) && (region->size == regionSize) && ((region->slots != NULL) || (regionSize == 0)))
	{
		return;
	}

	free(region->slots);
	region->start = start;
	region->slots = (regionSize != 0) ? (NativePgxpPrimSlot *)calloc(regionSize / sizeof(u32), sizeof(NativePgxpPrimSlot)) : NULL;
	region->size = (region->slots != NULL) ? regionSize : 0;
}

internal NativePgxpPrimSlot *NativePgxp_PrimSlot(const void *addr)
{
	const uintptr_t value = (uintptr_t)addr;

	for (int i = 0; i < NATIVE_PGXP_PRIM_REGION_COUNT; i++)
	{
		const NativePgxpPrimRegion *region = &s_pgxpRegions[i];

		if ((region->slots != NULL) && (value >= (uintptr_t)region->start) && ((value - (uintptr_t)region->start) < region->size))
		{
			return &region->slots[(value - (uintptr_t)region->start) / sizeof(u32)];
		}
	}

	return NULL;
}

internal NativePgxpCacheSlot *NativePgxp_CacheSlot(const void *addr)
{
	const u32 hash = (u32)((uintptr_t)addr >> 1) * 2654435761u;

	return &s_pgxpCache[hash >> (32 - NATIVE_PGXP_CACHE_BITS)];
}

internal void NativePgxp_SetShadow(const void *dst, NativePgxpVertex vertex)
{
	NativePgxpPrimSlot *primSlot = NativePgxp_PrimSlot(dst);

	if (primSlot != NULL)
	{
		primSlot->vertex = vertex;
		primSlot->generation = s_pgxpGeneration;
		return;
	}

	NativePgxpCacheSlot *cacheSlot = NativePgxp_CacheSlot(dst);
	cacheSlot->key = dst;
	cacheSlot->vertex = vertex;
	cacheSlot->generation = s_pgxpGeneration;
}

internal void NativePgxp_ClearShadow(const void *dst)
{
	NativePgxpPrimSlot *primSlot = NativePgxp_PrimSlot(dst);

	if (primSlot != NULL)
	{
		primSlot->generation = 0;
		return;
	}

	NativePgxpCacheSlot *cacheSlot = NativePgxp_CacheSlot(dst);
	if (cacheSlot->key == dst)
	{
		cacheSlot->generation = 0;
	}
}

internal const NativePgxpVertex *NativePgxp_FindShadow(const void *addr, u32 value)
{
	const NativePgxpPrimSlot *primSlot = NativePgxp_PrimSlot(addr);

	if (primSlot != NULL)
	{
		return ((primSlot->generation == s_pgxpGeneration) && (primSlot->vertex.value == value)) ? &primSlot->vertex : NULL;
	}

	const NativePgxpCacheSlot *cacheSlot = NativePgxp_CacheSlot(addr);
	if ((cacheSlot->key == addr) && (cacheSlot->generation == s_pgxpGeneration) && (cacheSlot->vertex.value == value))
	{
		return &cacheSlot->vertex;
	}

	return NULL;
}

internal const NativePgxpVertex *NativePgxp_FindRecent(u32 value)
{
	for (u32 i = 1; i <= NATIVE_PGXP_RECENT_COUNT; i++)
	{
		const u32 index = (s_pgxpRecentHead - i) & (NATIVE_PGXP_RECENT_COUNT - 1);

		if ((s_pgxpRecentGeneration[index] == s_pgxpGeneration) && (s_pgxpRecent[index].value == value))
		{
			return &s_pgxpRecent[index];
		}
	}

	return NULL;
}

internal void NativePgxp_RememberRecent(const NativePgxpVertex *vertex)
{
	const u32 last = (s_pgxpRecentHead - 1) & (NATIVE_PGXP_RECENT_COUNT - 1);

	// Culling helpers read the same SXY registers right before the packet writer
	// does; do not let those repeats flush the history.
	if ((s_pgxpRecentGeneration[last] == s_pgxpGeneration) && (s_pgxpRecent[last].value == vertex->value) && (s_pgxpRecent[last].x == vertex->x) &&
	    (s_pgxpRecent[last].y == vertex->y) && (s_pgxpRecent[last].w == vertex->w))
	{
		return;
	}

	const u32 index = s_pgxpRecentHead++ & (NATIVE_PGXP_RECENT_COUNT - 1);
	s_pgxpRecent[index] = *vertex;
	s_pgxpRecentGeneration[index] = s_pgxpGeneration;
}

internal NativePgxpRegister *NativePgxp_FifoRegister(int reg)
{
	// SXYP (15) reads back as SXY2.
	return &s_pgxpFifo[(reg >= 14) ? 2 : (reg - 12)];
}

internal const NativePgxpVertex *NativePgxp_FifoMatch(int reg, u32 value)
{
	const NativePgxpRegister *fifoReg = NativePgxp_FifoRegister(reg);

	return (fifoReg->valid && (fifoReg->vertex.value == value)) ? &fifoReg->vertex : NULL;
}

void NativePgxp_GteProject(float x, float y, float w, u32 value)
{
	s_pgxpFifo[0] = s_pgxpFifo[1];
	s_pgxpFifo[1] = s_pgxpFifo[2];
	s_pgxpFifo[2].vertex.x = x;
	s_pgxpFifo[2].vertex.y = y;
	s_pgxpFifo[2].vertex.w = w;
	s_pgxpFifo[2].vertex.value = value;
	s_pgxpFifo[2].valid = 1;
}

void NativePgxp_GteWriteSXY(int reg, u32 value)
{
	NativePgxpRegister written = {0};
	const NativePgxpVertex *source = NULL;

	// Game code saves and restores SXY registers around helper calls, or shuffles
	// them to continue a strip; recover the precise vertex the value came from.
	for (int i = 2; (i >= 0) && (source == NULL); i--)
	{
		if (s_pgxpFifo[i].valid && (s_pgxpFifo[i].vertex.value == value))
		{
			source = &s_pgxpFifo[i].vertex;
		}
	}

	if (source == NULL)
	{
		source = NativePgxp_FindRecent(value);
	}

	if (source != NULL)
	{
		written.vertex = *source;
		written.valid = 1;
	}

	if (reg == 15)
	{
		s_pgxpFifo[0] = s_pgxpFifo[1];
		s_pgxpFifo[1] = s_pgxpFifo[2];
		s_pgxpFifo[2] = written;
	}
	else
	{
		s_pgxpFifo[reg - 12] = written;
	}
}

void NativePgxp_GteReadSXY(int reg, u32 value)
{
	const NativePgxpVertex *vertex = NativePgxp_FifoMatch(reg, value);

	if (vertex != NULL)
	{
		NativePgxp_RememberRecent(vertex);
	}
}

void NativePgxp_GteLoadSXY(int reg, const void *src, u32 value)
{
	const NativePgxpVertex *vertex = NativePgxp_FindShadow(src, value);
	NativePgxpRegister *fifoReg = NativePgxp_FifoRegister(reg);

	if (vertex != NULL)
	{
		fifoReg->vertex = *vertex;
		fifoReg->valid = 1;
	}
}

int NativePgxp_GteNclip(u32 sxy0, u32 sxy1, u32 sxy2, s64 *out)
{
	const NativePgxpVertex *v0 = NativePgxp_FifoMatch(12, sxy0);
	const NativePgxpVertex *v1 = NativePgxp_FifoMatch(13, sxy1);
	const NativePgxpVertex *v2 = NativePgxp_FifoMatch(14, sxy2);

	if ((v0 == NULL) || (v1 == NULL) || (v2 == NULL))
	{
		return 0;
	}

	// Same winding test as NCLIP, on the positions the GPU will draw. A sliver
	// that rounds to zero or negative area on the integer grid can still cover
	// pixels at its precise position, and culling it leaves a hole.
	double nclip = ((double)v0->x * v1->y) + ((double)v1->x * v2->y) + ((double)v2->x * v0->y) - ((double)v0->x * v2->y) - ((double)v1->x * v0->y) -
	               ((double)v2->x * v1->y);

	// Keep small fractional areas from truncating to zero, as DuckStation does.
	const double nclipAbs = (nclip < 0.0) ? -nclip : nclip;
	if ((nclipAbs > 0.1) && (nclipAbs < 1.0))
	{
		nclip += (nclip < 0.0) ? -1.0 : 1.0;
	}

	*out = (s64)nclip;
	return 1;
}

void NativePgxp_StoreGteSXY(const void *dst, int reg, u32 value)
{
	const NativePgxpVertex *vertex = NativePgxp_FifoMatch(reg, value);

	if (vertex != NULL)
	{
		NativePgxp_SetShadow(dst, *vertex);
	}
	else
	{
		NativePgxp_ClearShadow(dst);
	}
}

void NativePgxp_CopyXY(const void *dst, const void *src, u32 value)
{
	const NativePgxpVertex *vertex = NativePgxp_FindShadow(src, value);

	if (vertex != NULL)
	{
		NativePgxp_SetShadow(dst, *vertex);
	}
	else
	{
		NativePgxp_ClearShadow(dst);
	}
}

void NativePgxp_BindWrittenXY(const void *dst, u32 value)
{
	const NativePgxpVertex *vertex = s_pgxpWorldPhase ? NativePgxp_FindRecent(value) : NULL;

	if (vertex != NULL)
	{
		NativePgxp_SetShadow(dst, *vertex);
	}
	else
	{
		NativePgxp_ClearShadow(dst);
	}
}

void NativePgxp_SetWorldPhase(int active)
{
	s_pgxpWorldPhase = active != 0;
}

int NativePgxp_Lookup(const void *addr, u32 value, NativePgxpVertex *out)
{
	const NativePgxpVertex *vertex = NativePgxp_FindShadow(addr, value);

	if (vertex == NULL)
	{
		return 0;
	}

	*out = *vertex;
	return 1;
}

void NativePgxp_EndFrame(void)
{
	s_pgxpGeneration++;
	if (s_pgxpGeneration == 0)
	{
		s_pgxpGeneration = 1;
	}
	s_pgxpWorldPhase = 0;
	memset(s_pgxpFifo, 0, sizeof(s_pgxpFifo));
}