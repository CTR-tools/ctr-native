#ifndef NATIVE_PERF_H
#define NATIVE_PERF_H

#include <macros.h>

enum NativePerfBucket
{
	NATIVE_PERF_BUCKET_GAME_LOGIC,
	NATIVE_PERF_BUCKET_RENDER_FRAME,
	NATIVE_PERF_BUCKET_MAINFRAME_SETUP,
	NATIVE_PERF_BUCKET_MAINFRAME_EFFECTS,
	NATIVE_PERF_BUCKET_MAINFRAME_HUD,
	NATIVE_PERF_BUCKET_MAINFRAME_QUEUE_INSTANCES,
	NATIVE_PERF_BUCKET_MAINFRAME_EXECUTE_INSTANCES,
	NATIVE_PERF_BUCKET_MAINFRAME_LEVEL_GEOMETRY,
	NATIVE_PERF_BUCKET_MAINFRAME_POST_LEVEL,
	NATIVE_PERF_BUCKET_MAINFRAME_REFRESHCARD,
	NATIVE_PERF_BUCKET_MAINFRAME_CLEAR_SCREEN,
	NATIVE_PERF_BUCKET_MAINFRAME_UI,
	NATIVE_PERF_BUCKET_MAINFRAME_RENDER_VSYNC,
	NATIVE_PERF_BUCKET_PLATFORM_END_FRAME,
	NATIVE_PERF_BUCKET_PLATFORM_END_SCENE,
	NATIVE_PERF_BUCKET_RENDER_SUBMIT,
	NATIVE_PERF_BUCKET_PLATFORM_BEGIN_SCENE,
	NATIVE_PERF_BUCKET_DRAW_OTAG,
	NATIVE_PERF_BUCKET_DRAW_OTAG_PARSE,
	NATIVE_PERF_BUCKET_DRAW_ALL_SPLITS,
	NATIVE_PERF_BUCKET_RENDERER_BEGIN_SCENE,
	NATIVE_PERF_BUCKET_RENDERER_UPDATE_VRAM,
	NATIVE_PERF_BUCKET_RENDERER_VERTEX_UPLOAD,
	NATIVE_PERF_BUCKET_RENDERER_DRAW_TRIANGLES,
	NATIVE_PERF_BUCKET_FRAMEBUFFER_STORE,
	NATIVE_PERF_BUCKET_FRAMEBUFFER_READBACK,
	NATIVE_PERF_BUCKET_SWAP_WINDOW,
	NATIVE_PERF_BUCKET_VSYNC_WAIT,
	NATIVE_PERF_BUCKET_AUDIO_VBLANK,
	NATIVE_PERF_BUCKET_COUNT
};

struct NativePerfFrameInfo
{
	s32 frameCounter;
	s32 timer;
	s32 levelID;
	s32 gameMode1;
	s32 loadingStage;
	s32 boolDemoMode;
	s32 numPlyrCurrGame;
	s32 elapsedTimeMS;
	s32 vsyncTillFlip;
	s32 vSync_between_drawSync;
	s32 frameTimer_VsyncCallback;
};

#if defined(CTR_INTERNAL)
int NativePerf_ConfigureFromArgs(int argc, char **argv);
int NativePerf_IsEnabled(void);
void NativePerf_Shutdown(void);
void NativePerf_BeginFrame(const struct NativePerfFrameInfo *info);
void NativePerf_EndFrame(const struct NativePerfFrameInfo *info);
void NativePerf_RecordGpuFrame(u32 frameIndex, f64 gpuMs);
void NativePerf_BeginScope(enum NativePerfBucket bucket);
void NativePerf_EndScope(enum NativePerfBucket bucket);
#else
static inline int NativePerf_ConfigureFromArgs(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	return 0;
}

static inline void NativePerf_Shutdown(void)
{
}

static inline int NativePerf_IsEnabled(void)
{
	return 0;
}

static inline void NativePerf_BeginFrame(const struct NativePerfFrameInfo *info)
{
	(void)info;
}

static inline void NativePerf_EndFrame(const struct NativePerfFrameInfo *info)
{
	(void)info;
}

static inline void NativePerf_RecordGpuFrame(u32 frameIndex, f64 gpuMs)
{
	(void)frameIndex;
	(void)gpuMs;
}

static inline void NativePerf_BeginScope(enum NativePerfBucket bucket)
{
	(void)bucket;
}

static inline void NativePerf_EndScope(enum NativePerfBucket bucket)
{
	(void)bucket;
}
#endif

#endif
