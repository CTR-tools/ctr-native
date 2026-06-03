#define _CRT_SECURE_NO_WARNINGS
#define CTR_NATIVE
#define USE_EXTENDED_PRIM_POINTERS 0
#define SDL_MAIN_HANDLED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if __GNUC__
#include <SDL2/SDL.h>
#if !defined(_WIN32)
#include <errno.h>
#include <sys/mman.h>
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif
#endif
#define _EnterCriticalSection(x)
#define EnterCriticalSection(x)
#define ExitCriticalSection()
#endif

#if defined(_WIN32)
#define CTR_MEM_COMMIT     0x00001000
#define CTR_MEM_RESERVE    0x00002000
#define CTR_PAGE_READWRITE 0x04
__declspec(dllimport) void *__stdcall VirtualAlloc(void *lpAddress, size_t dwSize, unsigned long flAllocationType, unsigned long flProtect);
__declspec(dllimport) unsigned long __stdcall GetLastError(void);
#endif

#include "psx/types.h"
#include "psx/libetc.h"
#include "psx/libgte.h"
#include "psx/libgpu.h"
#include "psx/libspu.h"
#include "psx/libcd.h"
#include "psx/libapi.h"
#include "psx/strings.h"
#include "psx/inline_c.h"
#include "PsyX/PsyX_public.h"
#include "PsyX/PsyX_globals.h"
#include "PsyX/PsyX_render.h"
#include "ctr_scratchpad.h"

#define CTR_NATIVE_RCNT1_HZ 15720u

static Uint64 s_rootCounterBase = 0;

static int Native_GetRCnt(int spec)
{
	const Uint64 freq = SDL_GetPerformanceFrequency();
	const Uint64 now = SDL_GetPerformanceCounter();
	Uint64 elapsed;
	Uint64 counts;

	(void)spec;

	if (s_rootCounterBase == 0)
		s_rootCounterBase = now;

	elapsed = now - s_rootCounterBase;
	counts = ((elapsed / freq) * CTR_NATIVE_RCNT1_HZ) + (((elapsed % freq) * CTR_NATIVE_RCNT1_HZ) / freq);
	if (counts > 0x7fffffff)
		return 0x7fffffff;

	return (int)counts;
}

static int Native_ResetRCnt(int spec)
{
	(void)spec;

	s_rootCounterBase = SDL_GetPerformanceCounter();
	return 0;
}

#define ResetRCnt(x) Native_ResetRCnt(x)
#define GetRCnt(x)   Native_GetRCnt(x)

#define BUILD        926
#define u_long       u32

#ifndef __GNUC__
#define _Static_assert(x)
#endif
#define __attribute__(x)

#define RECT RECT16
typedef enum
{
	PAD_ID_MOUSE = 0x1,
	PAD_ID_NEGCON = 0x2,
	PAD_ID_IRQ10_GUN = 0x3,
	PAD_ID_DIGITAL = 0x4,
	PAD_ID_ANALOG_STICK = 0x5,
	PAD_ID_GUNCON = 0x6,
	PAD_ID_ANALOG = 0x7,
	PAD_ID_MULTITAP = 0x8,
	PAD_ID_JOGCON = 0xe,
	PAD_ID_CONFIG_MODE = 0xf,
	PAD_ID_NONE = 0xf
} PadTypeID;

#include "ctr_native.h"
#include "platform.h"
#include "platform/native_audio.h"

#ifndef CTR_NATIVE_MEMPACK_RETAIL_PRESSURE
#define CTR_NATIVE_MEMPACK_RETAIL_PRESSURE 0
#endif

// TODO(aalhendi): Make retail-pressure mode runnable by moving native-only
// loaded-file ownership/header state out of MEMPACK.
// TODO(aalhendi): Shrink native Adventure hub swap packs back to retail
// 0x68800 sizes after host-side load state no longer lives in those packs.
// TODO(aalhendi): Re-audit LOAD_ReadFile_ex, LOAD_DramFileCallback, LEV/PTR
// callbacks, and hub swapping under CTR_NATIVE_MEMPACK_RETAIL_PRESSURE=1.
// TODO(aalhendi): Keep a pressure-mode repro for Time Trial -> quit ->
// Adventure save load so this does not regress silently.
#if CTR_NATIVE_MEMPACK_RETAIL_PRESSURE
// NOTE(aalhendi): Retail pressure mode exposes the NTSC-U 926 mempack window inside a 2 MiB backing store.
#define CTR_NATIVE_MEMPACK_BUFFER_SIZE  0x200000u
#define CTR_NATIVE_MEMPACK_START_OFFSET 0xba9f0u
#define CTR_NATIVE_MEMPACK_SIZE         0x144e10u
#else
#define CTR_NATIVE_MEMPACK_BUFFER_SIZE  (8u * 1024u * 1024u)
#define CTR_NATIVE_MEMPACK_START_OFFSET 0u
#define CTR_NATIVE_MEMPACK_SIZE         CTR_NATIVE_MEMPACK_BUFFER_SIZE
#endif

static char s_mempackMemory[CTR_NATIVE_MEMPACK_BUFFER_SIZE];
static struct PlatformMempackArena s_mempackArena;

#include "game_includes.h"

#include "game/zGlobal_RDATA.c"
#include "game/zGlobal_DATA.c"
#include "game/zGlobal_SDATA.c"

static int frameGap = 2000;
static int frameCount = 0;
static int oldTicks = 0;

void Platform_InitScratchpad(void)
{
#if defined(CTR_NATIVE)
	void *scratchpad = (void *)CTR_SCRATCHPAD_ADDR;
	size_t scratchpadSize = CTR_SCRATCHPAD_MAP_SIZE;

#if defined(_WIN32)
	void *mapped = VirtualAlloc(scratchpad, scratchpadSize, CTR_MEM_RESERVE | CTR_MEM_COMMIT, CTR_PAGE_READWRITE);
	if (mapped == NULL)
	{
		fprintf(stderr, "[CTR Native] Failed to map PS1 scratchpad at %p: GetLastError=%lu\n", scratchpad, GetLastError());
		abort();
	}
#elif defined(__GNUC__)
#ifdef MAP_FIXED_NOREPLACE
	int fixedFlag = MAP_FIXED_NOREPLACE;
#else
	int fixedFlag = MAP_FIXED;
#endif

	void *mapped = mmap(scratchpad, scratchpadSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | fixedFlag, -1, 0);
	if (mapped == MAP_FAILED)
	{
		fprintf(stderr, "[CTR Native] Failed to map PS1 scratchpad at %p: %s\n", scratchpad, strerror(errno));
		abort();
	}
#else
#error "Platform_InitScratchpad needs a fixed-address virtual-memory mapper for this platform"
#endif

	if (mapped != scratchpad)
	{
		fprintf(stderr, "[CTR Native] PS1 scratchpad mapped at %p, expected %p\n", mapped, scratchpad);
		abort();
	}

	memset(mapped, 0, scratchpadSize);
#endif
}

const struct PlatformMempackArena *Platform_InitMempackArena(void)
{
	memset(s_mempackMemory, 0, sizeof(s_mempackMemory));

	s_mempackArena.base = &s_mempackMemory[0];
	s_mempackArena.start = &s_mempackMemory[CTR_NATIVE_MEMPACK_START_OFFSET];
	s_mempackArena.endOfMemory = &s_mempackMemory[CTR_NATIVE_MEMPACK_BUFFER_SIZE];
	s_mempackArena.size = CTR_NATIVE_MEMPACK_SIZE;
	s_mempackArena.backingSize = CTR_NATIVE_MEMPACK_BUFFER_SIZE;

	// NOTE(aalhendi): Native still uses PS1-shaped OT links for many render
	// paths. MEMPACK must stay below 0x01000000 so CtrGpu_PrimToOTLink24 can
	// pack linked primitive pointers without losing address bits.
	s_mempackArena.lowAddressValid =
	    ((u32)s_mempackArena.base < 0x01000000) && ((u32)s_mempackArena.start < 0x01000000) && ((u32)s_mempackArena.endOfMemory <= 0x01000000);

	return &s_mempackArena;
}

static void CalcFPS(void)
{
	if (frameCount++ != frameGap)
		return;

	frameCount = 0;
	int newTicks = SDL_GetTicks();
	int delta = newTicks - oldTicks;
	oldTicks = newTicks;

	printf("FPS: %d\n", (1000 * frameGap) / delta);
}

void PsyXKeyboardHandler(int key, char down)
{
	if (down == 0)
		key = 0;

	SubmitName_UseKeyboard(key);
}

#ifndef CC
#if __GNUC__
#if _WIN32
#ifndef __clang__
#define CC "MINGW-GCC"
#else
#define CC "MINGW-CLANG"
#endif
#else
#ifndef __clang__
#define CC "GCC"
#else
#define CC "CLANG"
#endif
#endif
#elif defined(_MSC_VER)
#define CC "MSVC"
#else
#define CC "Unknown"
#endif
#endif

int main(int argc, char *argv[])
{
	printf("[CTR Native] Starting...\n");
	fflush(stdout);

	char *sdlBasePath = SDL_GetBasePath();
	printf("[CTR Native] SDL base path: %s\n", sdlBasePath ? sdlBasePath : "(null)");
	fflush(stdout);

	char baseDir[1024];

	if (sdlBasePath)
	{
		strncpy(baseDir, sdlBasePath, sizeof(baseDir));
		SDL_free(sdlBasePath);
		char *sep = strrchr(baseDir, '\\');
		if (!sep)
			sep = strrchr(baseDir, '/');
		if (sep)
			*sep = '\0';
		sep = strrchr(baseDir, '\\');
		if (!sep)
			sep = strrchr(baseDir, '/');
		if (sep)
			*sep = '\0';
	}
	else
	{
		getcwd(baseDir, sizeof(baseDir));
	}

	printf("[CTR Native] Built with: " CC "\n");
	printf("[CTR Native] Base: %s\n", baseDir);
	fflush(stdout);

	chdir(baseDir);

#ifdef USE_16BY9
	printf("[CTR Native] Widescreen\n");
	Platform_Init("Crash Team Racing", 1280, 720);
#else
	printf("[CTR Native] 4:3\n");
	Platform_Init("Crash Team Racing", 800, 600);
#endif

	Platform_InitScratchpad();

	// NOTE(aalhendi): CTR already throttles through the retail VSync/draw-sync
	// path. Do not add a second SDL swap wait; some GL drivers charge that wait
	// to the next frame's first clear instead of SDL_GL_SwapWindow.
	PsyX_SetSwapInterval(0);
	PsyX_EnableSwapInterval(0);

	g_cfg_controllerToSlotMapping[0] = 0;

	g_dbg_gameDebugKeys = PsyXKeyboardHandler;

	int result = CTR_Main();

	Platform_Shutdown();
	return result;
}

void Platform_Init(const char *title, int width, int height)
{
	g_cfg_swapInterval = 1;
	PsyX_Initialise(title, width, height, 0);
}

void Platform_Shutdown(void)
{
}

void Platform_BeginFrame(void)
{
	// NOTE(aalhendi): Normal rendering begins from DrawOTag after the current
	// draw env is installed. Starting PsyCross here clears the previous env and
	// can force the host GL driver to block before the retail render-submit path.
}

// NOTE(aalhendi): Frame timing is handled by VSync() in the platform layer,
// matching PS1 hardware behavior. Platform_EndFrame only does buffer swap + FPS.
void Platform_EndFrame(void)
{
	PsyX_EndScene();
	CalcFPS();
}

void Platform_PresentVRAMDisplay(void)
{
	PsyX_BeginScene();
	GR_PresentVRAMDisplay();
	Platform_EndFrame();
}

int Platform_PollInput(void)
{
	return 1;
}

int NikoGetEnterKey(void)
{
	const u8 *kb = SDL_GetKeyboardState(NULL);
	return (kb && kb[SDL_SCANCODE_RETURN]) ? 1 : 0;
}

// NOTE(aalhendi): Native owns the CTR VBlank clock instead of PsyCross's
// autonomous interrupt thread. CTR only registers MainDrawCb_Vsync (or clears
// it on shutdown), so native mirrors the SDK VSyncCallback contract directly
// and emits the registered callback from VSync().
static Uint64 s_nextVBlankCounter = 0;
static Uint64 s_vblankRemainder = 0;
static int s_nativeVBlankCount = 0;
static void (*s_nativeVSyncCallback)(void) = NULL;

int VSyncCallback(void (*func)(void))
{
	int old = (int)s_nativeVSyncCallback;

	s_nativeVSyncCallback = func;
	return old;
}

static void Native_WaitNextVBlank(void)
{
	const Uint64 freq = SDL_GetPerformanceFrequency();
	const Uint64 hz = VBLANK_FREQUENCY_NTSC; // NOTE(aalhendi): ctr-native targets NTSC-U 926.

	if (s_nextVBlankCounter == 0)
		s_nextVBlankCounter = SDL_GetPerformanceCounter();

	s_nextVBlankCounter += freq / hz;
	s_vblankRemainder += freq % hz;
	if (s_vblankRemainder >= hz)
	{
		s_nextVBlankCounter++;
		s_vblankRemainder -= hz;
	}

	while (1)
	{
		const Uint64 now = SDL_GetPerformanceCounter();

		if ((Sint64)(s_nextVBlankCounter - now) <= 0)
			return;

		const Uint64 remaining = s_nextVBlankCounter - now;
		const Uint64 remainingMs = (remaining * 1000) / freq;

		if (remainingMs > 1)
			SDL_Delay((Uint32)(remainingMs - 1));
		else
			SDL_Delay(0);
	}
}

int VSync(int mode)
{
	int vblankCount;

	if (mode < 0)
		return s_nativeVBlankCount;

	vblankCount = (mode == 0) ? 1 : mode;

	for (int i = 0; i < vblankCount; i++)
	{
		Native_WaitNextVBlank();
		if (s_nativeVSyncCallback != NULL)
			s_nativeVSyncCallback();

		NativeAudio_StepVBlank();
		s_nativeVBlankCount++;
	}

	return s_nativeVBlankCount;
}
