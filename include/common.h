#ifndef COMMON_H
#define COMMON_H

// Project base types and helpers.
#include <macros.h>
#include <psx/psx_prelude.h>

// Project-owned helpers layered on top of the PSX-shaped SDK headers.
#include <ctr_math.h>
#include <ctr_gte.h>
#include <ctr_scratchpad.h>
#include <prim.h>

#include <game_layouts.h>

// Game declarations and GPU helpers that depend on the layout headers above.
#include <functions.h>
#include <gpu.h>

#if defined(CTR_NATIVE)
static inline void *CTR_PsyqMemmove(void *dest, const void *src, s32 count)
{
	// NOTE(aalhendi): Retail PSYQ memmove at NTSC-U 926 0x80077e38 returns
	// immediately for signed lengths <= 0. Host libc takes size_t, so native
	// must preserve the signed PSYQ contract for ASM-verified game code.
	if (count <= 0)
	{
		return dest;
	}

	return memmove(dest, src, (size_t)count);
}

#define memmove(dest, src, count) CTR_PsyqMemmove((dest), (src), (s32)(count))
#endif

#endif
