#ifndef PSX_AT44_FIXTURE_H
#define PSX_AT44_FIXTURE_H
typedef unsigned int u32;
typedef unsigned short u16;
#ifdef CTR_NATIVE
#define FRAME_OWNER
#define FRAME_DECLARE()
#define FRAME_SAVE() ((void)0)
#define FRAME_RESTORE() ((void)0)
#else
// NOTE(aalhendi): Capture genuine incoming S0-S7/RA into caller-owned scratch.
// Save 0x00-0x1c/0x28, leaving 0x20/0x24 unused; source must balance restores.
// The S-bank provider must preserve AT while this scratch image is live.
#define FRAME_OWNER __attribute__((mips_unsafe_manual_at44))
#define FRAME_DECLARE() \
    register u32 *frame asm("$1"); \
    register u32 saved0 asm("$16"), saved1 asm("$17"), saved2 asm("$18"), saved3 asm("$19"); \
    register u32 saved4 asm("$20"), saved5 asm("$21"), saved6 asm("$22"), saved7 asm("$23"), savedRa asm("$31")
#define FRAME_SAVE() do { \
    __asm__ volatile("" : "=r"(saved0), "=r"(saved1), "=r"(saved2), "=r"(saved3), \
        "=r"(saved4), "=r"(saved5), "=r"(saved6), "=r"(saved7), "=r"(savedRa)); \
    frame = (u32 *)0x1f800000u; \
    frame[0]=saved0; frame[1]=saved1; frame[2]=saved2; frame[3]=saved3; \
    frame[4]=saved4; frame[5]=saved5; frame[6]=saved6; frame[7]=saved7; frame[10]=savedRa; \
} while (0)
#define FRAME_RESTORE() do { \
    savedRa=frame[10]; saved7=frame[7]; saved6=frame[6]; saved5=frame[5]; \
    saved4=frame[4]; saved3=frame[3]; saved2=frame[2]; saved1=frame[1]; saved0=frame[0]; \
    __asm__ volatile("" : : "r"(saved0), "r"(saved1), "r"(saved2), "r"(saved3), \
        "r"(saved4), "r"(saved5), "r"(saved6), "r"(saved7), "r"(savedRa)); \
} while (0)
#endif
#endif
