#include "platform/native_replay_scheduler.h"

#if defined(CTR_INTERNAL)
#include "platform/native_audio.h"
#include "platform/native_checkpoint.h"
#include "platform/native_checkpoint_file.h"
#include "platform/native_input.h"
#include "platform/native_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// NOTE(aalhendi): Little-endian tags `CTRR`/`RFRM` = CTR native Replay.
#define NATIVE_REPLAY_FILE_MAGIC                 0x52525443u
#define NATIVE_REPLAY_FRAME_MAGIC                0x4d524652u
#define NATIVE_REPLAY_FILE_VERSION               1u
#define NATIVE_REPLAY_FNV_OFFSET                 2166136261u
#define NATIVE_REPLAY_FNV_PRIME                  16777619u
#define NATIVE_REPLAY_CHECKPOINT_INTERVAL_FRAMES 300u
#define NATIVE_REPLAY_MAX_VSYNC_PACKETS          64u

enum NativeReplaySchedulerMode
{
	NATIVE_REPLAY_MODE_NONE = 0,
	NATIVE_REPLAY_MODE_RECORD,
	NATIVE_REPLAY_MODE_PLAYBACK
};

struct NativeReplayFileHeader
{
	u32 magic;
	u32 version;
	u32 headerSize;
	u32 frameRecordSize;
	u32 frameCount;
	u32 reserved[3];
};

struct NativeReplayFrameRecord
{
	u32 magic;
	u32 replayFrame;
	struct NativeReplaySchedulerFrameInfo beginInfo;
	struct NativeReplaySchedulerFrameInfo endInfo;
	struct PlatformInputPadSnapshot pads[PLATFORM_INPUT_PAD_COUNT];
	u32 vblankTotal;
	u32 vblankPacketCount;
	u16 vblankPackets[NATIVE_REPLAY_MAX_VSYNC_PACKETS];
	u32 padChecksum;
	u32 recordChecksum;
};

static enum NativeReplaySchedulerMode s_mode;
static u32 s_replayFrame;
static u32 s_frameLimit;
static s32 s_beginOpen;
static s32 s_divergenceLogged;
static FILE *s_file;
static struct NativeReplayFileHeader s_header;
static struct NativeReplayFrameRecord s_pendingRecord;
static struct NativeCheckpointFileWriter s_checkpointWriter;
static char *s_checkpointPath;
static u8 *s_checkpointPayload;
static int s_checkpointPayloadSize;
static u32 s_nextCheckpointFrame;
static u32 s_checkpointIndex;
static s32 s_checkpointWriterOpen;
static s32 s_restoreBootstrapCheckpoint;
static s32 s_frameTimingConsumed;
static u32 s_frameVBlankTotal;
static u32 s_frameVBlankPacketCount;
static s32 s_vblankPacketOverflow;
static s32 s_vblankPlaybackMismatch;

static void NativeReplayScheduler_ResetVSyncPackets(void)
{
	s_frameVBlankTotal = 0;
	s_frameVBlankPacketCount = 0;
	s_vblankPacketOverflow = 0;
	s_vblankPlaybackMismatch = 0;
}

static u32 NativeReplayScheduler_Fnv1a(const void *data, u32 size)
{
	const u8 *bytes = (const u8 *)data;
	u32 hash = NATIVE_REPLAY_FNV_OFFSET;
	u32 i;

	for (i = 0; i < size; i++)
	{
		hash ^= bytes[i];
		hash *= NATIVE_REPLAY_FNV_PRIME;
	}

	return hash;
}

static u32 NativeReplayScheduler_PadChecksum(const struct PlatformInputPadSnapshot *pads)
{
	return NativeReplayScheduler_Fnv1a(pads, sizeof(struct PlatformInputPadSnapshot) * PLATFORM_INPUT_PAD_COUNT);
}

static u32 NativeReplayScheduler_RecordChecksum(const struct NativeReplayFrameRecord *record)
{
	struct NativeReplayFrameRecord checksumRecord = *record;

	checksumRecord.recordChecksum = 0;
	return NativeReplayScheduler_Fnv1a(&checksumRecord, sizeof(checksumRecord));
}

static void NativeReplayScheduler_InitHeader(struct NativeReplayFileHeader *header)
{
	memset(header, 0, sizeof(*header));
	header->magic = NATIVE_REPLAY_FILE_MAGIC;
	header->version = NATIVE_REPLAY_FILE_VERSION;
	header->headerSize = sizeof(struct NativeReplayFileHeader);
	header->frameRecordSize = sizeof(struct NativeReplayFrameRecord);
}

static s32 NativeReplayScheduler_HeaderValid(const struct NativeReplayFileHeader *header)
{
	return (header->magic == NATIVE_REPLAY_FILE_MAGIC) && (header->version == NATIVE_REPLAY_FILE_VERSION) &&
	       (header->headerSize == sizeof(struct NativeReplayFileHeader)) && (header->frameRecordSize == sizeof(struct NativeReplayFrameRecord));
}

static const char *NativeReplayScheduler_ArgValue(int argc, char **argv, const char *arg)
{
	int i;

	for (i = 1; i < argc - 1; i++)
	{
		if (strcmp(argv[i], arg) == 0)
			return argv[i + 1];
	}

	return NULL;
}

static s32 NativeReplayScheduler_ArgMissingValue(int argc, char **argv, const char *arg)
{
	int i;

	for (i = 1; i < argc; i++)
	{
		if ((strcmp(argv[i], arg) == 0) && (i + 1 >= argc))
			return 1;
	}

	return 0;
}

static s32 NativeReplayScheduler_ParseU32(const char *text, u32 *out)
{
	char *end;
	unsigned long value;

	if ((text == NULL) || (text[0] == '\0') || (out == NULL))
		return 0;

	value = strtoul(text, &end, 10);
	if ((end == text) || (*end != '\0') || (value > 0xffffffffUL))
		return 0;

	*out = (u32)value;
	return 1;
}

static char *NativeReplayScheduler_MakeSidecarPath(const char *path, const char *extension)
{
	const char *lastSlash;
	const char *lastBackslash;
	const char *lastSeparator;
	const char *lastDot;
	size_t pathLen;
	size_t stemLen;
	size_t extensionLen;
	char *sidecarPath;

	if ((path == NULL) || (extension == NULL))
		return NULL;

	pathLen = strlen(path);
	extensionLen = strlen(extension);
	lastSlash = strrchr(path, '/');
	lastBackslash = strrchr(path, '\\');
	lastSeparator = lastSlash;
	if ((lastSeparator == NULL) || ((lastBackslash != NULL) && (lastBackslash > lastSeparator)))
		lastSeparator = lastBackslash;

	lastDot = strrchr(path, '.');
	if ((lastDot != NULL) && ((lastSeparator == NULL) || (lastDot > lastSeparator)))
		stemLen = (size_t)(lastDot - path);
	else
		stemLen = pathLen;

	sidecarPath = (char *)malloc(stemLen + extensionLen + 1);
	if (sidecarPath == NULL)
		return NULL;

	memcpy(sidecarPath, path, stemLen);
	memcpy(&sidecarPath[stemLen], extension, extensionLen + 1);
	return sidecarPath;
}

static s32 NativeReplayScheduler_FileExists(const char *path)
{
	FILE *file;

	if (path == NULL)
		return 0;

	file = fopen(path, "rb");
	if (file == NULL)
		return 0;

	fclose(file);
	return 1;
}

static s32 NativeReplayScheduler_WriteHeader(void)
{
	long oldPos;

	if (s_file == NULL)
		return 0;

	oldPos = ftell(s_file);
	if (oldPos < 0)
		return 0;

	if (fseek(s_file, 0, SEEK_SET) != 0)
		return 0;

	if (fwrite(&s_header, sizeof(s_header), 1, s_file) != 1)
		return 0;

	if (fseek(s_file, oldPos, SEEK_SET) != 0)
		return 0;

	return 1;
}

static void NativeReplayScheduler_CloseCheckpointFile(void)
{
	if (s_checkpointWriterOpen != 0)
	{
		if (!NativeCheckpointFile_EndWrite(&s_checkpointWriter))
			Platform_Log("[CTR State] failed to finalize rolling checkpoints\n");
		s_checkpointWriterOpen = 0;
	}

	free(s_checkpointPayload);
	s_checkpointPayload = NULL;
	s_checkpointPayloadSize = 0;

	free(s_checkpointPath);
	s_checkpointPath = NULL;

	s_checkpointIndex = 0;
	s_nextCheckpointFrame = 0;
	s_restoreBootstrapCheckpoint = 0;
	s_frameTimingConsumed = 0;
	NativeReplayScheduler_ResetVSyncPackets();
}

static void NativeReplayScheduler_CloseFiles(void)
{
	if (s_file == NULL)
	{
		NativeReplayScheduler_CloseCheckpointFile();
		NativeAudio_SetDeterministicRenderMode(0);
		s_mode = NATIVE_REPLAY_MODE_NONE;
		return;
	}

	if (s_mode == NATIVE_REPLAY_MODE_RECORD)
	{
		if (!NativeReplayScheduler_WriteHeader())
			Platform_Log("[CTR Replay] failed to finalize replay header\n");
	}

	fclose(s_file);
	s_file = NULL;
	NativeReplayScheduler_CloseCheckpointFile();
	NativeAudio_SetDeterministicRenderMode(0);
	s_mode = NATIVE_REPLAY_MODE_NONE;
}

static s32 NativeReplayScheduler_OpenCheckpointRecord(const char *replayPath)
{
	s_checkpointPath = NativeReplayScheduler_MakeSidecarPath(replayPath, ".ctrstates");
	if (s_checkpointPath == NULL)
	{
		Platform_Log("[CTR State] failed to build checkpoint path\n");
		return 0;
	}

	s_checkpointPayloadSize = NativeCheckpoint_GetSize();
	if (s_checkpointPayloadSize <= 0)
	{
		Platform_Log("[CTR State] invalid checkpoint size: %d\n", s_checkpointPayloadSize);
		return 0;
	}

	s_checkpointPayload = (u8 *)malloc((size_t)s_checkpointPayloadSize);
	if (s_checkpointPayload == NULL)
	{
		Platform_Log("[CTR State] failed to allocate checkpoint buffer: %d bytes\n", s_checkpointPayloadSize);
		return 0;
	}

	if (!NativeCheckpointFile_BeginWrite(&s_checkpointWriter, s_checkpointPath))
	{
		Platform_Log("[CTR State] failed to open rolling checkpoints: %s\n", s_checkpointPath);
		return 0;
	}

	s_checkpointWriterOpen = 1;
	s_checkpointIndex = 0;
	s_nextCheckpointFrame = 0;
	Platform_Log("[CTR State] recording rolling checkpoints: %s interval=%u frames\n", s_checkpointPath, NATIVE_REPLAY_CHECKPOINT_INTERVAL_FRAMES);
	return 1;
}

static s32 NativeReplayScheduler_WriteCheckpointIfDue(void)
{
	struct NativeCheckpointFileRecordInfo info;

	if (s_checkpointWriterOpen == 0)
		return 1;
	if ((s_replayFrame != 0) && (s_replayFrame < s_nextCheckpointFrame))
		return 1;

	if (!NativeCheckpoint_Capture(s_checkpointPayload, s_checkpointPayloadSize))
	{
		Platform_Log("[CTR State] failed to capture checkpoint #%u at replay frame %u\n", s_checkpointIndex, s_replayFrame);
		return 0;
	}

	if (!NativeCheckpointFile_AppendRecord(&s_checkpointWriter, s_checkpointPayload, s_checkpointPayloadSize, s_checkpointIndex, s_replayFrame, &info))
	{
		Platform_Log("[CTR State] failed to write checkpoint #%u at replay frame %u\n", s_checkpointIndex, s_replayFrame);
		return 0;
	}

	Platform_Log("[CTR State] checkpoint #%u replayFrame=%u checksum=0x%08x\n", info.checkpointIndex, info.replayFrame, info.checksum);
	s_checkpointIndex++;
	s_nextCheckpointFrame = s_replayFrame + NATIVE_REPLAY_CHECKPOINT_INTERVAL_FRAMES;
	return 1;
}

static s32 NativeReplayScheduler_PrepareBootstrapCheckpoint(const char *replayPath)
{
	char *checkpointPath = NativeReplayScheduler_MakeSidecarPath(replayPath, ".ctrstates");
	int recordCount = 0;

	if (checkpointPath == NULL)
	{
		Platform_Log("[CTR State] failed to build checkpoint path\n");
		return 0;
	}

	if (!NativeReplayScheduler_FileExists(checkpointPath))
	{
		free(checkpointPath);
		return 1;
	}

	if (!NativeCheckpointFile_Validate(checkpointPath, NULL, 0, &recordCount))
	{
		Platform_Log("[CTR State] invalid rolling checkpoints: %s\n", checkpointPath);
		free(checkpointPath);
		return 0;
	}

	Platform_Log("[CTR State] validated rolling checkpoints: %s records=%d\n", checkpointPath, recordCount);
	if (recordCount <= 0)
	{
		Platform_Log("[CTR State] rolling checkpoints are empty: %s\n", checkpointPath);
		free(checkpointPath);
		return 0;
	}

	s_checkpointPath = checkpointPath;
	s_restoreBootstrapCheckpoint = 1;
	return 1;
}

static s32 NativeReplayScheduler_RestoreBootstrapCheckpoint(void)
{
	struct NativeCheckpointFileRecordInfo info;
	u8 *payload;
	int payloadSize;
	s32 ok = 0;

	if (s_restoreBootstrapCheckpoint == 0)
		return 1;

	payloadSize = NativeCheckpoint_GetSize();
	if (payloadSize <= 0)
	{
		Platform_Log("[CTR State] invalid checkpoint size: %d\n", payloadSize);
		return 0;
	}

	payload = (u8 *)malloc((size_t)payloadSize);
	if (payload == NULL)
	{
		Platform_Log("[CTR State] failed to allocate checkpoint restore buffer: %d bytes\n", payloadSize);
		return 0;
	}

	if (!NativeCheckpointFile_ReadRecord(s_checkpointPath, 0, payload, payloadSize, &info))
	{
		Platform_Log("[CTR State] failed to read bootstrap checkpoint: %s\n", s_checkpointPath);
		goto cleanup;
	}
	if (info.replayFrame != 0)
	{
		Platform_Log("[CTR State] bootstrap checkpoint maps to replay frame %u\n", info.replayFrame);
		goto cleanup;
	}
	if (!NativeCheckpoint_Restore(payload, payloadSize))
	{
		Platform_Log("[CTR State] failed to restore bootstrap checkpoint\n");
		goto cleanup;
	}

	Platform_Log("[CTR State] restored bootstrap checkpoint #%u replayFrame=%u checksum=0x%08x\n", info.checkpointIndex, info.replayFrame, info.checksum);
	s_restoreBootstrapCheckpoint = 0;
	ok = 1;

cleanup:
	free(payload);
	return ok;
}

static s32 NativeReplayScheduler_OpenRecord(const char *path)
{
	NativeReplayScheduler_InitHeader(&s_header);
	s_file = fopen(path, "wb+");
	if (s_file == NULL)
	{
		Platform_Log("[CTR Replay] failed to open replay for record: %s\n", path);
		return 0;
	}

	if (fwrite(&s_header, sizeof(s_header), 1, s_file) != 1)
	{
		Platform_Log("[CTR Replay] failed to write replay header: %s\n", path);
		NativeReplayScheduler_CloseFiles();
		return 0;
	}

	s_mode = NATIVE_REPLAY_MODE_RECORD;
	NativeAudio_SetDeterministicRenderMode(1);
	if (!NativeReplayScheduler_OpenCheckpointRecord(path))
	{
		NativeReplayScheduler_CloseFiles();
		remove(path);
		return 0;
	}

	Platform_Log("[CTR Replay] recording input replay: %s\n", path);
	return 1;
}

static s32 NativeReplayScheduler_OpenPlayback(const char *path)
{
	s_file = fopen(path, "rb");
	if (s_file == NULL)
	{
		Platform_Log("[CTR Replay] failed to open replay for playback: %s\n", path);
		return 0;
	}

	if (fread(&s_header, sizeof(s_header), 1, s_file) != 1)
	{
		Platform_Log("[CTR Replay] failed to read replay header: %s\n", path);
		NativeReplayScheduler_CloseFiles();
		return 0;
	}

	if (!NativeReplayScheduler_HeaderValid(&s_header))
	{
		Platform_Log("[CTR Replay] invalid replay header: %s\n", path);
		NativeReplayScheduler_CloseFiles();
		return 0;
	}

	if (!NativeReplayScheduler_PrepareBootstrapCheckpoint(path))
	{
		NativeReplayScheduler_CloseFiles();
		return 0;
	}

	s_mode = NATIVE_REPLAY_MODE_PLAYBACK;
	NativeAudio_SetDeterministicRenderMode(1);
	Platform_Log("[CTR Replay] playing input replay: %s frames=%u\n", path, s_header.frameCount);
	return 1;
}

static void NativeReplayScheduler_LogFrameInfo(const char *prefix, const struct NativeReplaySchedulerFrameInfo *info)
{
	Platform_Log("[CTR Replay] %s vsync=%d frameCounter=%d timer=%d levFrames=%d elapsedMS=%d msLEV=%d eventMS=%d state=%d loading=%d level=%d "
	             "rng=(mix=0x%08x audio=0x%08x dead=0x%08x,0x%08x adv=0x%08x,0x%08x)\n",
	             prefix, info->frameTimer, info->frameCounter, info->timer, info->framesInThisLEV, info->elapsedTimeMS, info->msInThisLEV,
	             info->elapsedEventTime, info->mainGameState, info->loadingStage, info->levelID, info->mixRandomNumber, info->audioRNG, info->deadcoed0,
	             info->deadcoed1, info->advRng0, info->advRng1);
}

static s32 NativeReplayScheduler_FrameInfoMatches(const struct NativeReplaySchedulerFrameInfo *expected, const struct NativeReplaySchedulerFrameInfo *live)
{
	return (expected->frameTimer == live->frameTimer) && (expected->frameCounter == live->frameCounter) && (expected->timer == live->timer) &&
	       (expected->framesInThisLEV == live->framesInThisLEV) && (expected->elapsedTimeMS == live->elapsedTimeMS) &&
	       (expected->msInThisLEV == live->msInThisLEV) && (expected->elapsedEventTime == live->elapsedEventTime) &&
	       (expected->mainGameState == live->mainGameState) && (expected->loadingStage == live->loadingStage) && (expected->levelID == live->levelID) &&
	       (expected->mixRandomNumber == live->mixRandomNumber) && (expected->audioRNG == live->audioRNG) && (expected->deadcoed0 == live->deadcoed0) &&
	       (expected->deadcoed1 == live->deadcoed1) && (expected->advRng0 == live->advRng0) && (expected->advRng1 == live->advRng1);
}

static s32 NativeReplayScheduler_VSyncInfoMatches(const struct NativeReplayFrameRecord *expected)
{
	return (s_vblankPlaybackMismatch == 0) && (expected->vblankTotal == s_frameVBlankTotal) && (expected->vblankPacketCount == s_frameVBlankPacketCount);
}

static void NativeReplayScheduler_ReportDivergence(const struct NativeReplayFrameRecord *expected, const struct NativeReplaySchedulerFrameInfo *live,
                                                   u32 livePadChecksum)
{
	if (s_divergenceLogged != 0)
		return;

	s_divergenceLogged = 1;
	Platform_Log("[CTR Replay] divergence at replay frame %u\n", expected->replayFrame);
	NativeReplayScheduler_LogFrameInfo("expected", &expected->endInfo);
	NativeReplayScheduler_LogFrameInfo("live    ", live);
	Platform_Log("[CTR Replay] expected padChecksum=0x%08x live padChecksum=0x%08x\n", expected->padChecksum, livePadChecksum);
	Platform_Log("[CTR Replay] expected vblankPackets=%u vblankSteps=%u live vblankPackets=%u vblankSteps=%u\n", expected->vblankPacketCount,
	             expected->vblankTotal, s_frameVBlankPacketCount, s_frameVBlankTotal);
}

int NativeReplayScheduler_ConfigureFromArgs(int argc, char **argv)
{
	const char *recordPath = NativeReplayScheduler_ArgValue(argc, argv, "--record-replay");
	const char *playbackPath = NativeReplayScheduler_ArgValue(argc, argv, "--replay");
	const char *frameLimitText = NativeReplayScheduler_ArgValue(argc, argv, "--replay-frame-limit");

	if (NativeReplayScheduler_ArgMissingValue(argc, argv, "--record-replay") || NativeReplayScheduler_ArgMissingValue(argc, argv, "--replay") ||
	    NativeReplayScheduler_ArgMissingValue(argc, argv, "--replay-frame-limit"))
	{
		Platform_Log("[CTR Replay] missing replay command value\n");
		return 1;
	}

	if ((recordPath != NULL) && (playbackPath != NULL))
	{
		Platform_Log("[CTR Replay] choose either --record-replay or --replay, not both\n");
		return 1;
	}

	if ((frameLimitText != NULL) && !NativeReplayScheduler_ParseU32(frameLimitText, &s_frameLimit))
	{
		Platform_Log("[CTR Replay] invalid --replay-frame-limit value: %s\n", frameLimitText);
		return 1;
	}

	s_replayFrame = 0;
	s_beginOpen = 0;
	s_divergenceLogged = 0;
	s_frameTimingConsumed = 0;
	NativeReplayScheduler_ResetVSyncPackets();

	if (recordPath != NULL)
		return NativeReplayScheduler_OpenRecord(recordPath) ? 0 : 1;

	if (playbackPath != NULL)
		return NativeReplayScheduler_OpenPlayback(playbackPath) ? 0 : 1;

	return 0;
}

void NativeReplayScheduler_Shutdown(void)
{
	NativeReplayScheduler_CloseFiles();
	Platform_InputClearInstalledPadSnapshots();
	s_mode = NATIVE_REPLAY_MODE_NONE;
}

int NativeReplayScheduler_BeginFrame(const struct NativeReplaySchedulerFrameInfo *info)
{
	if ((s_mode == NATIVE_REPLAY_MODE_NONE) || (info == NULL))
		return 0;

	if ((s_frameLimit != 0) && (s_replayFrame >= s_frameLimit))
	{
		if (s_mode == NATIVE_REPLAY_MODE_PLAYBACK)
			Platform_Log("[CTR Replay] playback frame limit reached: frames=%u\n", s_replayFrame);
		return 1;
	}

	if (s_mode == NATIVE_REPLAY_MODE_RECORD)
	{
		if (!NativeReplayScheduler_WriteCheckpointIfDue())
			return 1;

		memset(&s_pendingRecord, 0, sizeof(s_pendingRecord));
		s_pendingRecord.magic = NATIVE_REPLAY_FRAME_MAGIC;
		s_pendingRecord.replayFrame = s_replayFrame;
		s_pendingRecord.beginInfo = *info;
		s_frameTimingConsumed = 0;
		NativeReplayScheduler_ResetVSyncPackets();
		if (Platform_InputCapturePadSnapshots(s_pendingRecord.pads, PLATFORM_INPUT_PAD_COUNT) == 0)
		{
			Platform_Log("[CTR Replay] failed to capture input snapshots\n");
			return 1;
		}
		s_pendingRecord.padChecksum = NativeReplayScheduler_PadChecksum(s_pendingRecord.pads);
		s_beginOpen = 1;
		return 0;
	}

	if (s_mode == NATIVE_REPLAY_MODE_PLAYBACK)
	{
		u32 checksum;

		if (!NativeReplayScheduler_RestoreBootstrapCheckpoint())
			return 1;

		if (s_replayFrame >= s_header.frameCount)
		{
			Platform_Log("[CTR Replay] replay finished after %u frames\n", s_replayFrame);
			return 1;
		}

		if (fread(&s_pendingRecord, sizeof(s_pendingRecord), 1, s_file) != 1)
		{
			Platform_Log("[CTR Replay] failed to read replay frame %u\n", s_replayFrame);
			return 1;
		}

		checksum = NativeReplayScheduler_RecordChecksum(&s_pendingRecord);
		if ((s_pendingRecord.magic != NATIVE_REPLAY_FRAME_MAGIC) || (s_pendingRecord.replayFrame != s_replayFrame) ||
		    (checksum != s_pendingRecord.recordChecksum) || (NativeReplayScheduler_PadChecksum(s_pendingRecord.pads) != s_pendingRecord.padChecksum))
		{
			Platform_Log("[CTR Replay] corrupt replay frame %u\n", s_replayFrame);
			return 1;
		}

		if (Platform_InputInstallPadSnapshots(s_pendingRecord.pads, PLATFORM_INPUT_PAD_COUNT) == 0)
		{
			Platform_Log("[CTR Replay] failed to install replay input frame %u\n", s_replayFrame);
			return 1;
		}

		s_frameTimingConsumed = 0;
		NativeReplayScheduler_ResetVSyncPackets();
		s_beginOpen = 1;
	}

	return 0;
}

int NativeReplayScheduler_ConsumeVSyncPacket(int requestedVBlanks, int *emittedVBlanks)
{
	u32 packet;

	if ((s_mode != NATIVE_REPLAY_MODE_PLAYBACK) || (s_beginOpen == 0) || (emittedVBlanks == NULL))
		return 0;

	if (requestedVBlanks < 1)
		requestedVBlanks = 1;

	if (s_frameVBlankPacketCount >= s_pendingRecord.vblankPacketCount)
	{
		s_vblankPlaybackMismatch = 1;
		*emittedVBlanks = requestedVBlanks;
		return 1;
	}

	packet = s_pendingRecord.vblankPackets[s_frameVBlankPacketCount];
	if (packet == 0)
	{
		s_vblankPlaybackMismatch = 1;
		packet = (u32)requestedVBlanks;
	}

	s_frameVBlankPacketCount++;
	s_frameVBlankTotal += packet;
	*emittedVBlanks = (int)packet;
	return 1;
}

int NativeReplayScheduler_ConsumeFrameElapsedTimeMS(int *elapsedTimeMS)
{
	if ((s_mode != NATIVE_REPLAY_MODE_PLAYBACK) || (s_beginOpen == 0) || (elapsedTimeMS == NULL) || (s_frameTimingConsumed != 0))
		return 0;

	*elapsedTimeMS = s_pendingRecord.endInfo.elapsedTimeMS;
	s_frameTimingConsumed = 1;
	return 1;
}

void NativeReplayScheduler_RecordVSyncPacket(int emittedVBlanks)
{
	if ((s_mode != NATIVE_REPLAY_MODE_RECORD) || (s_beginOpen == 0) || (emittedVBlanks <= 0))
		return;

	s_frameVBlankTotal += (u32)emittedVBlanks;
	if (s_frameVBlankPacketCount >= NATIVE_REPLAY_MAX_VSYNC_PACKETS)
	{
		s_vblankPacketOverflow = 1;
		return;
	}
	if (emittedVBlanks > 0xffff)
	{
		s_vblankPacketOverflow = 1;
		return;
	}

	s_pendingRecord.vblankPackets[s_frameVBlankPacketCount] = (u16)emittedVBlanks;
	s_frameVBlankPacketCount++;
}

int NativeReplayScheduler_EndFrame(const struct NativeReplaySchedulerFrameInfo *info)
{
	struct PlatformInputPadSnapshot livePads[PLATFORM_INPUT_PAD_COUNT];
	u32 livePadChecksum;

	if ((s_mode == NATIVE_REPLAY_MODE_NONE) || (info == NULL))
		return 0;

	if (s_beginOpen == 0)
		return 0;

	if (Platform_InputCapturePadSnapshots(livePads, PLATFORM_INPUT_PAD_COUNT) == 0)
		livePadChecksum = 0;
	else
		livePadChecksum = NativeReplayScheduler_PadChecksum(livePads);

	if (s_mode == NATIVE_REPLAY_MODE_RECORD)
	{
		if (s_vblankPacketOverflow != 0)
		{
			Platform_Log("[CTR Replay] too many VSync packets in replay frame %u\n", s_replayFrame);
			return 1;
		}

		s_pendingRecord.vblankTotal = s_frameVBlankTotal;
		s_pendingRecord.vblankPacketCount = s_frameVBlankPacketCount;
		s_pendingRecord.endInfo = *info;
		s_pendingRecord.recordChecksum = NativeReplayScheduler_RecordChecksum(&s_pendingRecord);

		if (fwrite(&s_pendingRecord, sizeof(s_pendingRecord), 1, s_file) != 1)
		{
			Platform_Log("[CTR Replay] failed to write replay frame %u\n", s_replayFrame);
			return 1;
		}

		s_header.frameCount++;
		s_replayFrame++;
		s_beginOpen = 0;
		s_frameTimingConsumed = 0;
		NativeReplayScheduler_ResetVSyncPackets();

		if ((s_frameLimit != 0) && (s_replayFrame >= s_frameLimit))
		{
			Platform_Log("[CTR Replay] recorded input replay frames=%u\n", s_replayFrame);
			NativeReplayScheduler_CloseFiles();
			return 1;
		}

		return 0;
	}

	if (s_mode == NATIVE_REPLAY_MODE_PLAYBACK)
	{
		if (!NativeReplayScheduler_FrameInfoMatches(&s_pendingRecord.endInfo, info) || !NativeReplayScheduler_VSyncInfoMatches(&s_pendingRecord) ||
		    (s_pendingRecord.padChecksum != livePadChecksum))
			NativeReplayScheduler_ReportDivergence(&s_pendingRecord, info, livePadChecksum);

		s_replayFrame++;
		s_beginOpen = 0;
		s_frameTimingConsumed = 0;
		NativeReplayScheduler_ResetVSyncPackets();
	}

	return 0;
}
#endif
