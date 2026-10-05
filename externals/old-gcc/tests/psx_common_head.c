#include "psx_at44.h"
#ifndef CASE
#define CASE 1
#endif
#ifdef CTR_NATIVE
static u32 headRemaining;
#else
/* The first incoming argument really owns A0, including its exit value. */
register u32 headRemaining __asm__("$4");
#endif
#if defined(CTR_NATIVE) || defined(ORDINARY)
#define HEAD_OWNER
#define HEAD_DECLARE
#define HEAD_SAVE() ((void)0)
#define HEAD_RESTORE() ((void)0)
#else
#define HEAD_OWNER FRAME_OWNER
#define HEAD_DECLARE FRAME_DECLARE()
#define HEAD_SAVE() FRAME_SAVE()
#define HEAD_RESTORE() FRAME_RESTORE()
#endif

void HEAD_OWNER CommonHeads(u32 count, u32 *out, u32 value, void **escape)
{
    HEAD_DECLARE;
#ifdef CTR_NATIVE
    headRemaining = count;
#endif
    HEAD_SAVE();
#if CASE == 6
    *escape = &&done;
#elif CASE == 7
    if (escape)
        goto done;
#endif
    goto test;
work:
    *out++ = value++;
test:
#if CASE == 8 || CASE == 9
#if CASE == 9
    if (headRemaining == 0)
#else
    if (headRemaining != 0)
#endif
        goto enter;
    headRemaining--;
    goto restore;
enter:
    headRemaining--;
    goto work;
#else
    if (headRemaining == 0)
        goto done;
#if CASE == 3
    *out = value;
#elif CASE == 4
    __asm__ volatile("" : : "r"(value));
#endif
#if CASE == 5
    headRemaining += 65536u;
#else
    headRemaining--;
#endif
    goto work;
done:
#if CASE == 3
    *out = value;
#elif CASE == 4
    __asm__ volatile("" : : "r"(value));
#endif
#if CASE == 2
    headRemaining -= 2u;
#elif CASE == 5
    headRemaining += 65536u;
#else
    headRemaining--;
#endif
#endif
restore:
    *out = headRemaining;
    HEAD_RESTORE();
}

#undef HEAD_OWNER
#undef HEAD_DECLARE
#undef HEAD_SAVE
#undef HEAD_RESTORE
