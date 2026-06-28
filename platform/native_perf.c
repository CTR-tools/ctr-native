#include "platform/native_perf.h"

#if defined(CTR_INTERNAL)
#include <macros.h>

#include "platform/native_audio.h"
#include "platform/native_log.h"
#include "platform/native_path.h"

#include <SDL3/SDL.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
#include <direct.h>
#include <sys/stat.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

#define NATIVE_PERF_PATH_MAX         1024
#define NATIVE_PERF_DEFAULT_DIR      "debug/perf/perf-latest"
#define NATIVE_PERF_FRAME_CSV_NAME   "frame_times.csv"
#define NATIVE_PERF_SUMMARY_NAME     "summary.txt"
#define NATIVE_PERF_TOP_FRAME_COUNT  20
#define NATIVE_PERF_FLUSH_INTERVAL   120
#define NATIVE_PERF_FRAME_BUDGET_MS  (1000.0 / 30.0)
#define NATIVE_PERF_FRAME_WARN_40_MS 40.0
#define NATIVE_PERF_FRAME_WARN_50_MS 50.0

struct NativePerfBucketInfo
{
	const char *name;
	s32 dominantCandidate;
};

struct NativePerfWorstFrame
{
	u32 frameIndex;
	f64 totalMs;
	f64 workMs;
	f64 overBudgetMs;
	f64 workOverBudgetMs;
	enum NativePerfBucket dominantBucket;
	f64 dominantMs;
	struct NativePerfFrameInfo info;
	s32 audioUnderrunDelta;
	s32 audioOverflowDelta;
	s32 audioQueuedFrames;
};

global_variable const struct NativePerfBucketInfo s_bucketInfo[NATIVE_PERF_BUCKET_COUNT] = {
    {"game_logic_ms", 1},
    {"render_frame_ms", 0},
    {"mainframe_setup_ms", 1},
    {"mainframe_effects_ms", 1},
    {"mainframe_hud_ms", 1},
    {"mainframe_queue_instances_ms", 1},
    {"mainframe_execute_instances_ms", 1},
    {"mainframe_level_geometry_ms", 1},
    {"mainframe_post_level_ms", 1},
    {"mainframe_refreshcard_ms", 1},
    {"mainframe_clear_screen_ms", 1},
    {"mainframe_ui_ms", 1},
    {"mainframe_render_vsync_ms", 0},
    {"platform_end_frame_ms", 0},
    {"platform_end_scene_ms", 0},
    {"render_submit_ms", 1},
    {"platform_begin_scene_ms", 1},
    {"draw_otag_ms", 0},
    {"draw_otag_parse_ms", 1},
    {"draw_all_splits_ms", 1},
    {"renderer_begin_scene_ms", 1},
    {"renderer_update_vram_ms", 1},
    {"renderer_vertex_upload_ms", 1},
    {"renderer_draw_triangles_ms", 1},
    {"framebuffer_store_ms", 0},
    {"framebuffer_resize_ms", 1},
    {"framebuffer_blit_ms", 1},
    {"framebuffer_flush_ms", 1},
    {"framebuffer_alloc_ms", 1},
    {"framebuffer_readback_ms", 1},
    {"framebuffer_pack_ms", 1},
    {"framebuffer_vram_upload_ms", 1},
    {"swap_window_ms", 1},
    {"vsync_wait_ms", 0},
    {"audio_vblank_ms", 1},
};

global_variable FILE *s_csvFile;
global_variable s32 s_enabled;
global_variable s32 s_frameOpen;
global_variable s32 s_rowsSinceFlush;
global_variable u32 s_frameIndex;
global_variable u32 s_framesOverBudget;
global_variable u32 s_workFramesOverBudget;
global_variable u32 s_framesOver40;
global_variable u32 s_framesOver50;
global_variable f64 s_totalFrameMs;
global_variable f64 s_totalWorkMs;
global_variable f64 s_maxFrameMs;
global_variable f64 s_maxWorkMs;
global_variable f64 s_bucketTotals[NATIVE_PERF_BUCKET_COUNT];
global_variable f64 s_frameBucketMs[NATIVE_PERF_BUCKET_COUNT];
global_variable u64 s_frameStartCounter;
global_variable u64 s_scopeStartCounter[NATIVE_PERF_BUCKET_COUNT];
global_variable s32 s_scopeDepth[NATIVE_PERF_BUCKET_COUNT];
global_variable struct NativePerfFrameInfo s_frameBeginInfo;
global_variable s32 s_beginUnderrunFrames;
global_variable s32 s_beginOverflowFrames;
global_variable s32 s_beginQueuedFrames;
global_variable char s_outputDir[NATIVE_PERF_PATH_MAX];
global_variable char s_csvPath[NATIVE_PERF_PATH_MAX];
global_variable char s_summaryPath[NATIVE_PERF_PATH_MAX];
global_variable struct NativePerfWorstFrame s_worstFrames[NATIVE_PERF_TOP_FRAME_COUNT];
global_variable s32 s_worstFrameCount;

internal f64 NativePerf_CounterToMs(u64 counterDelta)
{
	const u64 freq = SDL_GetPerformanceFrequency();

	if (freq == 0)
	{
		return 0.0;
	}

	return ((f64)counterDelta * 1000.0) / (f64)freq;
}

internal const char *NativePerf_BucketName(enum NativePerfBucket bucket)
{
	if ((bucket < 0) || (bucket >= NATIVE_PERF_BUCKET_COUNT))
	{
		return "unknown_ms";
	}

	return s_bucketInfo[bucket].name;
}

internal s32 NativePerf_PathExists(const char *path)
{
	struct stat st;

	if (path == NULL)
	{
		return 0;
	}

	return stat(path, &st) == 0;
}

internal s32 NativePerf_MakeDir(const char *path)
{
	if ((path == NULL) || (path[0] == '\0'))
	{
		return 1;
	}

#if defined(_WIN32)
	if (_mkdir(path) == 0)
		return 1;
#else
	if (mkdir(path, 0777) == 0)
	{
		return 1;
	}
#endif

	return (errno == EEXIST) && NativePerf_PathExists(path);
}

internal s32 NativePerf_CreateDirs(const char *path)
{
	char copy[NATIVE_PERF_PATH_MAX];
	char *cursor;
	s32 ok = 1;

	if ((path == NULL) || (path[0] == '\0'))
	{
		return 0;
	}

	if (!NativePath_NormalizeSlashes(copy, sizeof(copy), NativeStr8_FromCString(path)))
	{
		return 0;
	}

	cursor = copy;
	if (NativePath_IsSeparator((u8)cursor[0]))
	{
		cursor++;
	}
#if defined(_WIN32)
	if ((cursor[0] != '\0') && (cursor[1] == ':') && NativePath_IsSeparator((u8)cursor[2]))
		cursor += 3;
#endif

	while (*cursor != '\0')
	{
		if (NativePath_IsSeparator((u8)*cursor))
		{
			char saved = *cursor;

			*cursor = '\0';
			if ((copy[0] != '\0') && !NativePerf_MakeDir(copy))
			{
				ok = 0;
				break;
			}
			*cursor = saved;
		}
		cursor++;
	}

	if ((ok != 0) && !NativePerf_MakeDir(copy))
	{
		ok = 0;
	}

	return ok;
}

internal s32 NativePerf_SetOutputDir(const char *path)
{
	if (!NativePath_NormalizeSlashes(s_outputDir, sizeof(s_outputDir), NativeStr8_FromCString(path)))
	{
		return 0;
	}

	if (!NativePath_Join(s_csvPath, sizeof(s_csvPath), NativeStr8_FromCString(s_outputDir), NATIVE_STR8_LIT(NATIVE_PERF_FRAME_CSV_NAME)))
	{
		return 0;
	}

	if (!NativePath_Join(s_summaryPath, sizeof(s_summaryPath), NativeStr8_FromCString(s_outputDir), NATIVE_STR8_LIT(NATIVE_PERF_SUMMARY_NAME)))
	{
		return 0;
	}

	return 1;
}

internal void NativePerf_WriteCsvHeader(FILE *file)
{
	fprintf(file, "frame_index,total_ms,work_ms,over_budget_ms,work_over_budget_ms,dominant_bucket,dominant_ms,frameCounter,timer,level_id,game_mode1,"
	              "loading_stage,bool_demo_mode,num_plyr,elapsed_time_ms,vsync_till_flip,vSync_between_drawSync,frameTimer_VsyncCallback,"
	              "audio_underrun_delta,audio_overflow_delta,audio_queued_frames");

	for (s32 bucket = 0; bucket < NATIVE_PERF_BUCKET_COUNT; bucket++)
	{
		fprintf(file, ",%s", NativePerf_BucketName((enum NativePerfBucket)bucket));
	}

	fprintf(file, "\n");
}

internal enum NativePerfBucket NativePerf_FindDominantBucket(f64 *dominantMsOut)
{
	enum NativePerfBucket dominantBucket = NATIVE_PERF_BUCKET_GAME_LOGIC;
	f64 dominantMs = 0.0;

	for (s32 bucket = 0; bucket < NATIVE_PERF_BUCKET_COUNT; bucket++)
	{
		if (s_bucketInfo[bucket].dominantCandidate == 0)
		{
			continue;
		}

		if (s_frameBucketMs[bucket] > dominantMs)
		{
			dominantBucket = (enum NativePerfBucket)bucket;
			dominantMs = s_frameBucketMs[bucket];
		}
	}

	*dominantMsOut = dominantMs;
	return dominantBucket;
}

internal void NativePerf_InsertWorstFrame(const struct NativePerfWorstFrame *frame)
{
	s32 insertAt = s_worstFrameCount;

	if (s_worstFrameCount == NATIVE_PERF_TOP_FRAME_COUNT && frame->workMs <= s_worstFrames[s_worstFrameCount - 1].workMs)
	{
		return;
	}

	while ((insertAt > 0) && (frame->workMs > s_worstFrames[insertAt - 1].workMs))
	{
		if (insertAt < NATIVE_PERF_TOP_FRAME_COUNT)
		{
			s_worstFrames[insertAt] = s_worstFrames[insertAt - 1];
		}
		insertAt--;
	}

	if (insertAt < NATIVE_PERF_TOP_FRAME_COUNT)
	{
		s_worstFrames[insertAt] = *frame;
	}

	if (s_worstFrameCount < NATIVE_PERF_TOP_FRAME_COUNT)
	{
		s_worstFrameCount++;
	}
}

internal void NativePerf_WriteSummary(FILE *file)
{
	f64 averageMs = (s_frameIndex == 0) ? 0.0 : s_totalFrameMs / (f64)s_frameIndex;
	f64 averageWorkMs = (s_frameIndex == 0) ? 0.0 : s_totalWorkMs / (f64)s_frameIndex;

	fprintf(file, "frames=%u\n", s_frameIndex);
	fprintf(file, "average_ms=%.3f\n", averageMs);
	fprintf(file, "average_work_ms=%.3f\n", averageWorkMs);
	fprintf(file, "max_ms=%.3f\n", s_maxFrameMs);
	fprintf(file, "max_work_ms=%.3f\n", s_maxWorkMs);
	fprintf(file, "frames_over_33_33ms=%u\n", s_framesOverBudget);
	fprintf(file, "work_frames_over_33_33ms=%u\n", s_workFramesOverBudget);
	fprintf(file, "frames_over_40ms=%u\n", s_framesOver40);
	fprintf(file, "frames_over_50ms=%u\n", s_framesOver50);
	fprintf(file, "csv_path=%s\n", s_csvPath);

	fprintf(file, "\nbucket_totals_ms:\n");
	for (s32 bucket = 0; bucket < NATIVE_PERF_BUCKET_COUNT; bucket++)
	{
		fprintf(file, "  %s: %.3f\n", NativePerf_BucketName((enum NativePerfBucket)bucket), s_bucketTotals[bucket]);
	}

	fprintf(file, "\ntop_worst_work_frames:\n");
	fprintf(file, "frame,total_ms,work_ms,over_budget_ms,work_over_budget_ms,dominant_bucket,dominant_ms,frameCounter,timer,level_id,game_mode1,loading_stage,"
	              "bool_demo_mode,num_plyr,elapsed_time_ms,vsync_till_flip,vSync_between_drawSync,frameTimer_VsyncCallback,audio_underrun_delta,"
	              "audio_overflow_delta,audio_queued_frames\n");

	for (s32 i = 0; i < s_worstFrameCount; i++)
	{
		const struct NativePerfWorstFrame *frame = &s_worstFrames[i];

		fprintf(file, "%u,%.3f,%.3f,%.3f,%.3f,%s,%.3f,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", frame->frameIndex, frame->totalMs, frame->workMs,
		        frame->overBudgetMs, frame->workOverBudgetMs, NativePerf_BucketName(frame->dominantBucket), frame->dominantMs, frame->info.frameCounter,
		        frame->info.timer, frame->info.levelID, frame->info.gameMode1, frame->info.loadingStage, frame->info.boolDemoMode, frame->info.numPlyrCurrGame,
		        frame->info.elapsedTimeMS, frame->info.vsyncTillFlip, frame->info.vSync_between_drawSync, frame->info.frameTimer_VsyncCallback,
		        frame->audioUnderrunDelta, frame->audioOverflowDelta, frame->audioQueuedFrames);
	}
}

internal const char *NativePerf_ArgValue(int argc, char **argv, const char *name)
{
	for (s32 i = 1; i < argc - 1; i++)
	{
		if (strcmp(argv[i], name) == 0)
		{
			return argv[i + 1];
		}
	}

	return NULL;
}

internal s32 NativePerf_ArgPresent(int argc, char **argv, const char *name)
{
	for (s32 i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], name) == 0)
		{
			return 1;
		}
	}

	return 0;
}

int NativePerf_ConfigureFromArgs(int argc, char **argv)
{
	const char *outputDir = NativePerf_ArgValue(argc, argv, "--perf-dir");
	const s32 perfEnabled = NativePerf_ArgPresent(argc, argv, "--perf") || (outputDir != NULL);

	if (!perfEnabled)
	{
		return 0;
	}

	if (outputDir == NULL)
	{
		outputDir = NATIVE_PERF_DEFAULT_DIR;
	}

	if (!NativePerf_SetOutputDir(outputDir))
	{
		fprintf(stderr, "[CTR Perf] invalid perf output path: %s\n", outputDir);
		return 1;
	}

	if (!NativePerf_CreateDirs(s_outputDir))
	{
		fprintf(stderr, "[CTR Perf] failed to create perf output directory: %s\n", s_outputDir);
		return 1;
	}

	s_csvFile = fopen(s_csvPath, "wb");
	if (s_csvFile == NULL)
	{
		fprintf(stderr, "[CTR Perf] failed to open frame CSV: %s\n", s_csvPath);
		return 1;
	}

	NativePerf_WriteCsvHeader(s_csvFile);
	fflush(s_csvFile);
	s_enabled = 1;

	Platform_Log("[CTR Perf] capturing frame times: %s\n", s_csvPath);
	return 0;
}

void NativePerf_Shutdown(void)
{
	FILE *summaryFile;

	if (!s_enabled)
	{
		return;
	}

	if (s_frameOpen)
	{
		NativePerf_EndFrame(&s_frameBeginInfo);
	}

	if (s_csvFile != NULL)
	{
		fflush(s_csvFile);
		fclose(s_csvFile);
		s_csvFile = NULL;
	}

	summaryFile = fopen(s_summaryPath, "wb");
	if (summaryFile != NULL)
	{
		NativePerf_WriteSummary(summaryFile);
		fclose(summaryFile);
	}
	else
	{
		Platform_Log("[CTR Perf] failed to write summary: %s\n", s_summaryPath);
	}

	Platform_Log("[CTR Perf] frames=%u avg=%.3fms avgWork=%.3fms max=%.3fms maxWork=%.3fms over33.33=%u workOver33.33=%u over40=%u over50=%u\n", s_frameIndex,
	             (s_frameIndex == 0) ? 0.0 : s_totalFrameMs / (f64)s_frameIndex, (s_frameIndex == 0) ? 0.0 : s_totalWorkMs / (f64)s_frameIndex, s_maxFrameMs,
	             s_maxWorkMs, s_framesOverBudget, s_workFramesOverBudget, s_framesOver40, s_framesOver50);
	Platform_Log("[CTR Perf] summary: %s\n", s_summaryPath);

	s_enabled = 0;
}

void NativePerf_BeginFrame(const struct NativePerfFrameInfo *info)
{
	if (!s_enabled || (info == NULL))
	{
		return;
	}

	if (s_frameOpen)
	{
		NativePerf_EndFrame(info);
	}

	memset(s_frameBucketMs, 0, sizeof(s_frameBucketMs));
	memset(s_scopeDepth, 0, sizeof(s_scopeDepth));
	memset(s_scopeStartCounter, 0, sizeof(s_scopeStartCounter));

	s_frameBeginInfo = *info;
	NativeAudio_GetOutputStats(&s_beginUnderrunFrames, &s_beginOverflowFrames, &s_beginQueuedFrames);
	s_frameStartCounter = SDL_GetPerformanceCounter();
	s_frameOpen = 1;
}

void NativePerf_EndFrame(const struct NativePerfFrameInfo *info)
{
	f64 totalMs;
	f64 workMs;
	f64 overBudgetMs;
	f64 workOverBudgetMs;
	f64 dominantMs;
	enum NativePerfBucket dominantBucket;
	s32 endUnderrunFrames = 0;
	s32 endOverflowFrames = 0;
	s32 endQueuedFrames = 0;
	struct NativePerfWorstFrame worstFrame;
	const struct NativePerfFrameInfo *rowInfo;

	if (!s_enabled || !s_frameOpen)
	{
		return;
	}

	totalMs = NativePerf_CounterToMs(SDL_GetPerformanceCounter() - s_frameStartCounter);
	workMs = totalMs - s_frameBucketMs[NATIVE_PERF_BUCKET_VSYNC_WAIT];
	if (workMs < 0.0)
	{
		workMs = 0.0;
	}

	overBudgetMs = (totalMs > NATIVE_PERF_FRAME_BUDGET_MS) ? (totalMs - NATIVE_PERF_FRAME_BUDGET_MS) : 0.0;
	workOverBudgetMs = (workMs > NATIVE_PERF_FRAME_BUDGET_MS) ? (workMs - NATIVE_PERF_FRAME_BUDGET_MS) : 0.0;
	dominantBucket = NativePerf_FindDominantBucket(&dominantMs);

	NativeAudio_GetOutputStats(&endUnderrunFrames, &endOverflowFrames, &endQueuedFrames);

	rowInfo = (info != NULL) ? info : &s_frameBeginInfo;

	fprintf(s_csvFile, "%u,%.3f,%.3f,%.3f,%.3f,%s,%.3f,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d", s_frameIndex, totalMs, workMs, overBudgetMs,
	        workOverBudgetMs, NativePerf_BucketName(dominantBucket), dominantMs, rowInfo->frameCounter, rowInfo->timer, rowInfo->levelID, rowInfo->gameMode1,
	        rowInfo->loadingStage, rowInfo->boolDemoMode, rowInfo->numPlyrCurrGame, rowInfo->elapsedTimeMS, rowInfo->vsyncTillFlip,
	        rowInfo->vSync_between_drawSync, rowInfo->frameTimer_VsyncCallback, endUnderrunFrames - s_beginUnderrunFrames,
	        endOverflowFrames - s_beginOverflowFrames, endQueuedFrames);

	for (s32 bucket = 0; bucket < NATIVE_PERF_BUCKET_COUNT; bucket++)
	{
		fprintf(s_csvFile, ",%.3f", s_frameBucketMs[bucket]);
		s_bucketTotals[bucket] += s_frameBucketMs[bucket];
	}
	fprintf(s_csvFile, "\n");

	s_rowsSinceFlush++;
	if (s_rowsSinceFlush >= NATIVE_PERF_FLUSH_INTERVAL)
	{
		fflush(s_csvFile);
		s_rowsSinceFlush = 0;
	}

	s_totalFrameMs += totalMs;
	s_totalWorkMs += workMs;
	if (totalMs > s_maxFrameMs)
	{
		s_maxFrameMs = totalMs;
	}
	if (workMs > s_maxWorkMs)
	{
		s_maxWorkMs = workMs;
	}
	if (totalMs > NATIVE_PERF_FRAME_BUDGET_MS)
	{
		s_framesOverBudget++;
	}
	if (workMs > NATIVE_PERF_FRAME_BUDGET_MS)
	{
		s_workFramesOverBudget++;
	}
	if (totalMs > NATIVE_PERF_FRAME_WARN_40_MS)
	{
		s_framesOver40++;
	}
	if (totalMs > NATIVE_PERF_FRAME_WARN_50_MS)
	{
		s_framesOver50++;
	}

	worstFrame.frameIndex = s_frameIndex;
	worstFrame.totalMs = totalMs;
	worstFrame.workMs = workMs;
	worstFrame.overBudgetMs = overBudgetMs;
	worstFrame.workOverBudgetMs = workOverBudgetMs;
	worstFrame.dominantBucket = dominantBucket;
	worstFrame.dominantMs = dominantMs;
	worstFrame.info = *rowInfo;
	worstFrame.audioUnderrunDelta = endUnderrunFrames - s_beginUnderrunFrames;
	worstFrame.audioOverflowDelta = endOverflowFrames - s_beginOverflowFrames;
	worstFrame.audioQueuedFrames = endQueuedFrames;
	NativePerf_InsertWorstFrame(&worstFrame);

	s_frameIndex++;
	s_frameOpen = 0;
}

void NativePerf_BeginScope(enum NativePerfBucket bucket)
{
	if (!s_enabled || !s_frameOpen || (bucket < 0) || (bucket >= NATIVE_PERF_BUCKET_COUNT))
	{
		return;
	}

	if (s_scopeDepth[bucket]++ == 0)
	{
		s_scopeStartCounter[bucket] = SDL_GetPerformanceCounter();
	}
}

void NativePerf_EndScope(enum NativePerfBucket bucket)
{
	if (!s_enabled || !s_frameOpen || (bucket < 0) || (bucket >= NATIVE_PERF_BUCKET_COUNT))
	{
		return;
	}

	if (s_scopeDepth[bucket] <= 0)
	{
		return;
	}

	s_scopeDepth[bucket]--;
	if (s_scopeDepth[bucket] == 0)
	{
		s_frameBucketMs[bucket] += NativePerf_CounterToMs(SDL_GetPerformanceCounter() - s_scopeStartCounter[bucket]);
	}
}
#endif
