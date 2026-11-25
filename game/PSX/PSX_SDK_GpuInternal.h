#ifndef CTR_PSX_SDK_GPU_INTERNAL_H
#define CTR_PSX_SDK_GPU_INTERNAL_H

#include <ctr_compiler.h>
#include <macros.h>
#include <psx/libetc.h>
#include <psx/libgte.h>
#include <psx/libgpu.h>

// NOTE(aalhendi): Retail libgpu dispatches operations through these resident
// tables and environment state at 0x8008ad78..0x8008ae40; these declarations
// name existing storage only.
struct PsxGpuJumpTable
{
	void *fn0;
	void *fn1;
	s32 (*addque)(void *fn, void *arg1, s32 size, void *arg3);
	void *fn3;
	void (*cwc)(u32 cmd);
	void *fn5;
	void *otc;
	void *store;
	void *load;
	void *fn9;
	void *fn10;
	void (*clearOTagR)(u32 *ot, s32 n);
	void *fn12;
	s32 (*reset)(s32 mode);
	void *fn14;
	s32 (*sync)(s32 mode);
};

struct PsxGpuEnv
{
	volatile u8 version;
	u8 isque;
	u8 debug;
	u8 reverse;
	s16 vramW;
	s16 vramH;
	u32 pad8;
	void (*drawSyncCb)(void);
	DRAWENV drawEnv;
	DISPENV dispEnv;
};

struct PsxGpuMovePrim
{
	u32 tag;
	u32 code;
	u32 src;
	u32 dst;
	u32 wh;
};

extern struct PsxGpuJumpTable *psxGpuJtb;
extern s32 (*psxGpuPrintf)(const char *fmt, ...);
extern struct PsxGpuEnv psxGpuEnv;
extern struct PsxGpuMovePrim psxGpuMovePrim;
extern u32 psxGpuOtTail;
extern u32 psxGpuOtHead;

extern void *memcpy(void *dst, const void *src, u32 n);
extern void psxGpuFillBytes(void *dst, s32 val, s32 n);
extern void psxGpuCheckRect(const char *name, RECT16 *rect);
extern void psxGpuBuildDrawEnv(DR_ENV *drEnv, DRAWENV *env);

#endif
