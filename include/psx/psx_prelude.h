#ifndef PSX_PRELUDE_H
#define PSX_PRELUDE_H

#if defined(CTR_NATIVE)
#include <stdio.h>
#include <string.h>
#else
#include <psx/psn00b_prelude.h>
#endif

// PSX SDK-shaped headers used by game code.
#include <psx/libapi.h>
#include <psx/libcd.h>
#include <psx/libetc.h>
#include <psx/libgte.h>
#include <psx/libgpu.h>
#include <psx/libpad.h>
#include <psx/libspu.h>
#include <psx/strings.h>
#include <psx/inline_c.h>

// Supplemental SDK definitions used by game-owned pad state.
#include <psn00bsdk/include/psxpad.h>
#include <psn00bsdk/include/sys/fcntl.h>

#ifndef RECT
#define RECT RECT16
#endif

#endif
