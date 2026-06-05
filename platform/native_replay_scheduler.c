#include "platform/native_replay_scheduler.h"

#if defined(CTR_INTERNAL)
#include "platform/native_audio.h"
#include "platform/native_checkpoint.h"
#include "platform/native_checkpoint_file.h"
#include "platform/native_input.h"
#include "platform/native_log.h"
#include "platform/native_state.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32)
#include <direct.h>
#include <sys/stat.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

// NOTE(aalhendi): Little-endian tags `CTRR`/`RFRM` = CTR native Replay.
#define NATIVE_REPLAY_FILE_MAGIC                 0x52525443u
#define NATIVE_REPLAY_FRAME_MAGIC                0x4d524652u
#define NATIVE_REPLAY_FILE_VERSION               1u
#define NATIVE_REPLAY_FNV_OFFSET                 2166136261u
#define NATIVE_REPLAY_FNV_PRIME                  16777619u
#define NATIVE_REPLAY_CHECKPOINT_INTERVAL_FRAMES 300u
#define NATIVE_REPLAY_MAX_VSYNC_PACKETS          64u
#define NATIVE_REPLAY_BUILD_ID_BYTES             64u
#define NATIVE_REPLAY_PLATFORM_ID_BYTES          32u
#define NATIVE_REPLAY_DEFAULT_REPORT_ROOT        "debug/reports"
#define NATIVE_REPLAY_REPORT_REPLAY_NAME         "recording.ctrreplay"
#define NATIVE_REPLAY_REPORT_METADATA_NAME       "metadata.txt"
#define NATIVE_REPLAY_REPORT_LOG_NAME            "ctr-native.log"

#ifndef CTR_NATIVE_BUILD_ID
#define CTR_NATIVE_BUILD_ID "unknown"
#endif

enum NativeReplaySchedulerMode
{
	NATIVE_REPLAY_MODE_NONE = 0,
	NATIVE_REPLAY_MODE_ARMED,
	NATIVE_REPLAY_MODE_RECORD,
	NATIVE_REPLAY_MODE_PLAYBACK
};

enum NativeReplayCheckpointPolicy
{
	NATIVE_REPLAY_CHECKPOINT_POLICY_ROLLING = 0,
	NATIVE_REPLAY_CHECKPOINT_POLICY_BOOTSTRAP_ONLY
};

struct NativeReplayFileHeader
{
	u32 magic;
	u32 version;
	u32 headerSize;
	u32 frameRecordSize;
	u32 frameCount;
	u32 checkpointCount;
	u32 checkpointSize;
	u32 nativeStateSize;
	u32 identityChecksum;
	u32 checkpointPolicy;
	char buildId[NATIVE_REPLAY_BUILD_ID_BYTES];
	char platformId[NATIVE_REPLAY_PLATFORM_ID_BYTES];
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
static enum NativeReplayCheckpointPolicy s_checkpointPolicy = NATIVE_REPLAY_CHECKPOINT_POLICY_ROLLING;
static s32 s_startRequested;
static s32 s_stopRequested;
static s32 s_reportCompleted;
static s32 s_reportManualStart;
static s32 s_reportEnabled;
static char *s_reportDir;
static char *s_reportReplayPath;
static char *s_reportCheckpointPath;
static char *s_reportMetadataPath;
static char *s_reportLogPath;

static void NativeReplayScheduler_ResetVSyncPackets(void)
{
	s_frameVBlankTotal = 0;
	s_frameVBlankPacketCount = 0;
	s_vblankPacketOverflow = 0;
	s_vblankPlaybackMismatch = 0;
}

static void NativeReplayScheduler_ResetSessionState(void)
{
	s_replayFrame = 0;
	s_beginOpen = 0;
	s_divergenceLogged = 0;
	s_frameTimingConsumed = 0;
	s_stopRequested = 0;
	NativeReplayScheduler_ResetVSyncPackets();
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

static u32 NativeReplayScheduler_Fnv1aStep(u32 hash, const void *data, u32 size)
{
	const u8 *bytes = (const u8 *)data;
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

static const char *NativeReplayScheduler_CheckpointPolicyName(enum NativeReplayCheckpointPolicy policy)
{
	if (policy == NATIVE_REPLAY_CHECKPOINT_POLICY_BOOTSTRAP_ONLY)
		return "bootstrap-only";

	return "rolling";
}

static const char *NativeReplayScheduler_MetadataStatus(s32 finalMetadata)
{
	if (finalMetadata != 0)
		return "finalized";
	if (s_mode == NATIVE_REPLAY_MODE_ARMED)
		return "armed";
	if (s_mode == NATIVE_REPLAY_MODE_RECORD)
		return "recording";
	if (s_reportCompleted != 0)
		return "finalized";

	return "idle";
}

static void NativeReplayScheduler_CopyFixedString(char *dst, u32 dstSize, const char *src)
{
	size_t srcLen;

	if ((dst == NULL) || (dstSize == 0))
		return;

	memset(dst, 0, dstSize);
	if (src == NULL)
		return;

	srcLen = strlen(src);
	if (srcLen >= dstSize)
		srcLen = dstSize - 1u;
	memcpy(dst, src, srcLen);
}

static const char *NativeReplayScheduler_PlatformID(void)
{
#if defined(_WIN32)
	return "win32";
#elif defined(__APPLE__)
	return "macos";
#elif defined(__linux__)
	return "linux";
#else
	return "unknown";
#endif
}

static u32 NativeReplayScheduler_IdentityChecksum(const struct NativeReplayFileHeader *header)
{
	u32 hash = NATIVE_REPLAY_FNV_OFFSET;

	hash = NativeReplayScheduler_Fnv1aStep(hash, &header->magic, sizeof(header->magic));
	hash = NativeReplayScheduler_Fnv1aStep(hash, &header->version, sizeof(header->version));
	hash = NativeReplayScheduler_Fnv1aStep(hash, &header->headerSize, sizeof(header->headerSize));
	hash = NativeReplayScheduler_Fnv1aStep(hash, &header->frameRecordSize, sizeof(header->frameRecordSize));
	hash = NativeReplayScheduler_Fnv1aStep(hash, &header->checkpointSize, sizeof(header->checkpointSize));
	hash = NativeReplayScheduler_Fnv1aStep(hash, &header->nativeStateSize, sizeof(header->nativeStateSize));
	hash = NativeReplayScheduler_Fnv1aStep(hash, header->buildId, sizeof(header->buildId));
	hash = NativeReplayScheduler_Fnv1aStep(hash, header->platformId, sizeof(header->platformId));
	return hash;
}

static void NativeReplayScheduler_InitHeader(struct NativeReplayFileHeader *header)
{
	memset(header, 0, sizeof(*header));
	header->magic = NATIVE_REPLAY_FILE_MAGIC;
	header->version = NATIVE_REPLAY_FILE_VERSION;
	header->headerSize = sizeof(struct NativeReplayFileHeader);
	header->frameRecordSize = sizeof(struct NativeReplayFrameRecord);
	header->checkpointSize = (u32)NativeCheckpoint_GetSize();
	header->nativeStateSize = (u32)NativeState_GetSize();
	header->checkpointPolicy = (u32)s_checkpointPolicy;
	NativeReplayScheduler_CopyFixedString(header->buildId, sizeof(header->buildId), CTR_NATIVE_BUILD_ID);
	NativeReplayScheduler_CopyFixedString(header->platformId, sizeof(header->platformId), NativeReplayScheduler_PlatformID());
	header->identityChecksum = NativeReplayScheduler_IdentityChecksum(header);
}

static s32 NativeReplayScheduler_HeaderValid(const struct NativeReplayFileHeader *header)
{
	struct NativeReplayFileHeader liveHeader;

	if ((header == NULL) || (header->magic != NATIVE_REPLAY_FILE_MAGIC) || (header->version != NATIVE_REPLAY_FILE_VERSION) ||
	    (header->headerSize != sizeof(struct NativeReplayFileHeader)) || (header->frameRecordSize != sizeof(struct NativeReplayFrameRecord)))
		return 0;

	NativeReplayScheduler_InitHeader(&liveHeader);
	return (header->checkpointSize == liveHeader.checkpointSize) && (header->nativeStateSize == liveHeader.nativeStateSize) &&
	       (header->identityChecksum == liveHeader.identityChecksum) && (memcmp(header->buildId, liveHeader.buildId, sizeof(header->buildId)) == 0) &&
	       (memcmp(header->platformId, liveHeader.platformId, sizeof(header->platformId)) == 0);
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

static s32 NativeReplayScheduler_ArgPresent(int argc, char **argv, const char *arg)
{
	int i;

	for (i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], arg) == 0)
			return 1;
	}

	return 0;
}

static s32 NativeReplayScheduler_ArgMissingValue(int argc, char **argv, const char *arg)
{
	int i;

	for (i = 1; i < argc; i++)
	{
		if ((strcmp(argv[i], arg) == 0) && ((i + 1 >= argc) || (strncmp(argv[i + 1], "--", 2) == 0)))
			return 1;
	}

	return 0;
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

static s32 NativeReplayScheduler_PathExists(const char *path)
{
	struct stat st;

	return (path != NULL) && (stat(path, &st) == 0);
}

static char *NativeReplayScheduler_DupString(const char *text)
{
	size_t len;
	char *copy;

	if (text == NULL)
		return NULL;

	len = strlen(text);
	copy = (char *)malloc(len + 1u);
	if (copy == NULL)
		return NULL;

	memcpy(copy, text, len + 1u);
	return copy;
}

static s32 NativeReplayScheduler_IsPathSeparator(char c)
{
	return (c == '/') || (c == '\\');
}

static char *NativeReplayScheduler_JoinPath(const char *left, const char *right)
{
	size_t leftLen;
	size_t rightLen;
	s32 needsSeparator;
	char *path;

	if ((left == NULL) || (right == NULL))
		return NULL;

	leftLen = strlen(left);
	rightLen = strlen(right);
	needsSeparator = (leftLen != 0) && !NativeReplayScheduler_IsPathSeparator(left[leftLen - 1u]);

	path = (char *)malloc(leftLen + (size_t)needsSeparator + rightLen + 1u);
	if (path == NULL)
		return NULL;

	memcpy(path, left, leftLen);
	if (needsSeparator != 0)
		path[leftLen++] = '/';
	memcpy(&path[leftLen], right, rightLen + 1u);
	return path;
}

static s32 NativeReplayScheduler_MakeDir(const char *path)
{
	if ((path == NULL) || (path[0] == '\0'))
		return 1;

#if defined(_WIN32)
	if (_mkdir(path) == 0)
		return 1;
#else
	if (mkdir(path, 0777) == 0)
		return 1;
#endif

	return errno == EEXIST;
}

static s32 NativeReplayScheduler_CreateDirs(const char *path)
{
	char *copy;
	char *cursor;
	s32 ok = 1;

	if ((path == NULL) || (path[0] == '\0'))
		return 0;

	copy = NativeReplayScheduler_DupString(path);
	if (copy == NULL)
		return 0;

	cursor = copy;
	if (NativeReplayScheduler_IsPathSeparator(cursor[0]))
		cursor++;
#if defined(_WIN32)
	if ((cursor[0] != '\0') && (cursor[1] == ':') && NativeReplayScheduler_IsPathSeparator(cursor[2]))
		cursor += 3;
#endif

	while (*cursor != '\0')
	{
		if (NativeReplayScheduler_IsPathSeparator(*cursor))
		{
			char saved = *cursor;

			*cursor = '\0';
			if ((copy[0] != '\0') && !NativeReplayScheduler_MakeDir(copy))
			{
				ok = 0;
				break;
			}
			*cursor = saved;
		}
		cursor++;
	}

	if ((ok != 0) && !NativeReplayScheduler_MakeDir(copy))
		ok = 0;

	free(copy);
	return ok;
}

static void NativeReplayScheduler_FreeReportPaths(void)
{
	free(s_reportDir);
	free(s_reportReplayPath);
	free(s_reportCheckpointPath);
	free(s_reportMetadataPath);
	free(s_reportLogPath);
	s_reportDir = NULL;
	s_reportReplayPath = NULL;
	s_reportCheckpointPath = NULL;
	s_reportMetadataPath = NULL;
	s_reportLogPath = NULL;
	s_reportEnabled = 0;
}

static s32 NativeReplayScheduler_PrepareReportPaths(const char *root)
{
	time_t now;
	struct tm *localTime;
	char dateText[16];
	char runText[32];
	char runCandidate[40];
	char *dateDir = NULL;
	u32 attempt;

	if ((root == NULL) || (root[0] == '\0'))
		return 0;

	now = time(NULL);
	localTime = localtime(&now);
	if (localTime == NULL)
		return 0;

	if ((strftime(dateText, sizeof(dateText), "%Y%m%d", localTime) == 0) || (strftime(runText, sizeof(runText), "ctr-%H%M%S", localTime) == 0))
		return 0;

	NativeReplayScheduler_FreeReportPaths();

	dateDir = NativeReplayScheduler_JoinPath(root, dateText);
	if (dateDir == NULL)
		return 0;
	for (attempt = 0; attempt < 100u; attempt++)
	{
		int written;

		if (attempt == 0)
			written = snprintf(runCandidate, sizeof(runCandidate), "%s", runText);
		else
			written = snprintf(runCandidate, sizeof(runCandidate), "%s-%02u", runText, (unsigned int)attempt);

		if ((written < 0) || ((size_t)written >= sizeof(runCandidate)))
			goto fail;

		s_reportDir = NativeReplayScheduler_JoinPath(dateDir, runCandidate);
		if (s_reportDir == NULL)
			goto fail;
		if (!NativeReplayScheduler_PathExists(s_reportDir))
			break;

		free(s_reportDir);
		s_reportDir = NULL;
	}
	free(dateDir);
	dateDir = NULL;
	if (s_reportDir == NULL)
		goto fail;

	s_reportReplayPath = NativeReplayScheduler_JoinPath(s_reportDir, NATIVE_REPLAY_REPORT_REPLAY_NAME);
	s_reportCheckpointPath = NativeReplayScheduler_MakeSidecarPath(s_reportReplayPath, ".ctrstates");
	s_reportMetadataPath = NativeReplayScheduler_JoinPath(s_reportDir, NATIVE_REPLAY_REPORT_METADATA_NAME);
	s_reportLogPath = NativeReplayScheduler_JoinPath(s_reportDir, NATIVE_REPLAY_REPORT_LOG_NAME);
	if ((s_reportReplayPath == NULL) || (s_reportCheckpointPath == NULL) || (s_reportMetadataPath == NULL) || (s_reportLogPath == NULL))
		goto fail;

	if (!NativeReplayScheduler_CreateDirs(s_reportDir))
		goto fail;

	if (!Platform_LogSetPath(s_reportLogPath))
		goto fail;

	s_reportEnabled = 1;
	return 1;

fail:
	free(dateDir);
	NativeReplayScheduler_FreeReportPaths();
	return 0;
}

int NativeReplayScheduler_PrepareReportFromArgs(int argc, char **argv)
{
	const s32 recordReport = NativeReplayScheduler_ArgPresent(argc, argv, "--record");

	if (recordReport == 0)
		return 0;

	if (NativeReplayScheduler_ArgPresent(argc, argv, "--replay") != 0)
	{
		fprintf(stderr, "[CTR Replay] choose --record or --replay, not both\n");
		return 1;
	}

	if (!NativeReplayScheduler_PrepareReportPaths(NATIVE_REPLAY_DEFAULT_REPORT_ROOT))
	{
		fprintf(stderr, "[CTR Replay] failed to prepare report folder under: %s\n", NATIVE_REPLAY_DEFAULT_REPORT_ROOT);
		return 1;
	}

	return 0;
}

static void NativeReplayScheduler_WriteReportMetadata(s32 finalMetadata)
{
	FILE *file;

	if ((s_reportEnabled == 0) || (s_reportMetadataPath == NULL))
		return;

	file = fopen(s_reportMetadataPath, "wb");
	if (file == NULL)
	{
		Platform_Log("[CTR Replay] failed to write report metadata: %s\n", s_reportMetadataPath);
		return;
	}

	fprintf(file, "ctr_native_report=1\n");
	fprintf(file, "finalized=%d\n", finalMetadata != 0);
	fprintf(file, "recording_status=%s\n", NativeReplayScheduler_MetadataStatus(finalMetadata));
	fprintf(file, "manual_start=%d\n", s_reportManualStart != 0);
	fprintf(file, "start_hotkey=F9\n");
	fprintf(file, "stop_hotkey=F10\n");
	fprintf(file, "build_id=%s\n", s_header.buildId);
	fprintf(file, "platform=%s\n", s_header.platformId);
	fprintf(file, "replay_version=%u\n", (unsigned int)s_header.version);
	fprintf(file, "frame_record_size=%u\n", (unsigned int)s_header.frameRecordSize);
	fprintf(file, "checkpoint_size=%u\n", (unsigned int)s_header.checkpointSize);
	fprintf(file, "native_state_size=%u\n", (unsigned int)s_header.nativeStateSize);
	fprintf(file, "checkpoint_mode=%s\n", NativeReplayScheduler_CheckpointPolicyName((enum NativeReplayCheckpointPolicy)s_header.checkpointPolicy));
	fprintf(file, "identity_checksum=0x%08x\n", (unsigned int)s_header.identityChecksum);
	fprintf(file, "frame_count=%u\n", (unsigned int)s_header.frameCount);
	fprintf(file, "checkpoint_count=%u\n", (unsigned int)s_header.checkpointCount);
	fprintf(file, "checkpoint_interval_frames=%u\n",
	        s_header.checkpointPolicy == NATIVE_REPLAY_CHECKPOINT_POLICY_ROLLING ? (unsigned int)NATIVE_REPLAY_CHECKPOINT_INTERVAL_FRAMES : 0u);
	fprintf(file, "report_dir=%s\n", s_reportDir != NULL ? s_reportDir : "");
	fprintf(file, "replay_path=%s\n", s_reportReplayPath != NULL ? s_reportReplayPath : "");
	fprintf(file, "checkpoint_path=%s\n", s_reportCheckpointPath != NULL ? s_reportCheckpointPath : "");
	fprintf(file, "log_path=%s\n", Platform_LogGetPath());
	fprintf(file, "playback_command=build/ctr_native --replay \"%s\"\n", s_reportReplayPath != NULL ? s_reportReplayPath : "");
	fclose(file);
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

	fflush(s_file);
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
		s_stopRequested = 0;
		return;
	}

	if (s_mode == NATIVE_REPLAY_MODE_RECORD)
	{
		s_header.checkpointCount = s_checkpointIndex;
		if (!NativeReplayScheduler_WriteHeader())
			Platform_Log("[CTR Replay] failed to finalize replay header\n");
		NativeReplayScheduler_WriteReportMetadata(1);
	}

	fclose(s_file);
	s_file = NULL;
	NativeReplayScheduler_CloseCheckpointFile();
	NativeAudio_SetDeterministicRenderMode(0);
	if (s_reportEnabled != 0)
		s_reportCompleted = 1;
	s_stopRequested = 0;
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
	if (s_checkpointPolicy == NATIVE_REPLAY_CHECKPOINT_POLICY_BOOTSTRAP_ONLY)
		Platform_Log("[CTR State] recording bootstrap checkpoint only: %s\n", s_checkpointPath);
	else
		Platform_Log("[CTR State] recording rolling checkpoints: %s interval=%u frames\n", s_checkpointPath, NATIVE_REPLAY_CHECKPOINT_INTERVAL_FRAMES);
	return 1;
}

static s32 NativeReplayScheduler_WriteCheckpointIfDue(void)
{
	struct NativeCheckpointFileRecordInfo info;

	if (s_checkpointWriterOpen == 0)
		return 1;
	if ((s_checkpointPolicy == NATIVE_REPLAY_CHECKPOINT_POLICY_BOOTSTRAP_ONLY) && (s_checkpointIndex != 0))
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
	s_header.checkpointCount = s_checkpointIndex;
	if (!NativeReplayScheduler_WriteHeader())
		Platform_Log("[CTR Replay] failed to update replay header checkpoint count\n");
	s_nextCheckpointFrame = s_replayFrame + NATIVE_REPLAY_CHECKPOINT_INTERVAL_FRAMES;
	NativeReplayScheduler_WriteReportMetadata(0);
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
		Platform_Log("[CTR State] missing rolling checkpoints for replay: %s\n", checkpointPath);
		free(checkpointPath);
		return 0;
	}

	if (!NativeCheckpointFile_Validate(checkpointPath, NULL, 0, &recordCount))
	{
		Platform_Log("[CTR State] invalid rolling checkpoints: %s\n", checkpointPath);
		free(checkpointPath);
		return 0;
	}

	Platform_Log("[CTR State] validated replay checkpoints: %s records=%d\n", checkpointPath, recordCount);
	if (recordCount <= 0)
	{
		Platform_Log("[CTR State] replay checkpoints are empty: %s\n", checkpointPath);
		free(checkpointPath);
		return 0;
	}
	if ((u32)recordCount != s_header.checkpointCount)
	{
		Platform_Log("[CTR State] checkpoint count mismatch: replay=%u state=%d\n", s_header.checkpointCount, recordCount);
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
	fflush(s_file);

	s_mode = NATIVE_REPLAY_MODE_RECORD;
	NativeAudio_SetDeterministicRenderMode(1);
	if (!NativeReplayScheduler_OpenCheckpointRecord(path))
	{
		NativeReplayScheduler_CloseFiles();
		remove(path);
		return 0;
	}

	NativeReplayScheduler_WriteReportMetadata(0);
	Platform_Log("[CTR Replay] recording input replay: %s\n", path);
	return 1;
}

static s32 NativeReplayScheduler_ArmReport(void)
{
	if ((s_reportEnabled == 0) || (s_reportReplayPath == NULL))
		return 0;

	NativeReplayScheduler_ResetSessionState();
	NativeReplayScheduler_InitHeader(&s_header);
	s_mode = NATIVE_REPLAY_MODE_ARMED;
	s_reportManualStart = 1;
	s_reportCompleted = 0;
	NativeReplayScheduler_WriteReportMetadata(0);
	Platform_Log("[CTR Replay] report armed: press F9 to start recording, F10 to stop\n");
	return 1;
}

static s32 NativeReplayScheduler_StartReportRecording(void)
{
	if ((s_reportEnabled == 0) || (s_reportReplayPath == NULL))
		return 0;
	if (s_reportCompleted != 0)
	{
		Platform_Log("[CTR Replay] report already finalized: %s\n", s_reportDir != NULL ? s_reportDir : "");
		return 0;
	}

	NativeReplayScheduler_ResetSessionState();
	if (!NativeReplayScheduler_OpenRecord(s_reportReplayPath))
		return 0;

	Platform_Log("[CTR Replay] report recording started\n");
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
	const char *playbackPath = NativeReplayScheduler_ArgValue(argc, argv, "--replay");
	const s32 recordReport = NativeReplayScheduler_ArgPresent(argc, argv, "--record");
	const s32 playback = NativeReplayScheduler_ArgPresent(argc, argv, "--replay");
	s32 toggle = NativeReplayScheduler_ArgPresent(argc, argv, "--toggle");
	s32 detailed = NativeReplayScheduler_ArgPresent(argc, argv, "--detailed");

	if (NativeReplayScheduler_ArgMissingValue(argc, argv, "--replay"))
	{
		Platform_Log("[CTR Replay] missing replay command value\n");
		return 1;
	}

	if ((recordReport != 0) && (playback != 0))
	{
		Platform_Log("[CTR Replay] choose --record or --replay, not both\n");
		return 1;
	}
	if (((toggle != 0) || (detailed != 0)) && (recordReport == 0))
	{
		Platform_Log("[CTR Replay] --toggle and --detailed only apply to --record\n");
		return 1;
	}

	if ((recordReport != 0) && (s_reportEnabled == 0) && !NativeReplayScheduler_PrepareReportPaths(NATIVE_REPLAY_DEFAULT_REPORT_ROOT))
	{
		Platform_Log("[CTR Replay] failed to prepare report folder under: %s\n", NATIVE_REPLAY_DEFAULT_REPORT_ROOT);
		return 1;
	}

	NativeReplayScheduler_ResetSessionState();
	s_startRequested = 0;
	s_reportCompleted = 0;
	s_reportManualStart = 0;
	s_checkpointPolicy = (recordReport != 0) ? NATIVE_REPLAY_CHECKPOINT_POLICY_BOOTSTRAP_ONLY : NATIVE_REPLAY_CHECKPOINT_POLICY_ROLLING;
	if (detailed != 0)
		s_checkpointPolicy = NATIVE_REPLAY_CHECKPOINT_POLICY_ROLLING;

	if (recordReport != 0)
	{
		if (toggle == 0)
			return NativeReplayScheduler_OpenRecord(s_reportReplayPath) ? 0 : 1;

		return NativeReplayScheduler_ArmReport() ? 0 : 1;
	}

	if (playbackPath != NULL)
		return NativeReplayScheduler_OpenPlayback(playbackPath) ? 0 : 1;

	return 0;
}

void NativeReplayScheduler_Shutdown(void)
{
	NativeReplayScheduler_CloseFiles();
	NativeReplayScheduler_FreeReportPaths();
	Platform_InputClearInstalledPadSnapshots();
	s_mode = NATIVE_REPLAY_MODE_NONE;
}

int NativeReplayScheduler_RequestStart(void)
{
	if (s_mode == NATIVE_REPLAY_MODE_RECORD)
	{
		Platform_Log("[CTR Replay] report recording is already active\n");
		return 1;
	}
	if (s_mode != NATIVE_REPLAY_MODE_ARMED)
		return 0;
	if (s_reportCompleted != 0)
	{
		Platform_Log("[CTR Replay] report is already finalized\n");
		return 1;
	}

	if (s_startRequested == 0)
	{
		s_startRequested = 1;
		Platform_Log("[CTR Replay] report start requested; recording begins next frame\n");
	}
	return 1;
}

int NativeReplayScheduler_RequestStop(void)
{
	if (s_mode == NATIVE_REPLAY_MODE_ARMED)
	{
		Platform_Log("[CTR Replay] report is armed but not recording; press F9 to start\n");
		return 1;
	}
	if (s_mode != NATIVE_REPLAY_MODE_RECORD)
		return 0;

	if (s_stopRequested == 0)
	{
		s_stopRequested = 1;
		Platform_Log("[CTR Replay] report stop requested; finalizing after current frame\n");
	}
	return 1;
}

int NativeReplayScheduler_BeginFrame(const struct NativeReplaySchedulerFrameInfo *info)
{
	if ((s_mode == NATIVE_REPLAY_MODE_NONE) || (info == NULL))
		return 0;

	if (s_mode == NATIVE_REPLAY_MODE_ARMED)
	{
		if (s_startRequested == 0)
			return 0;

		s_startRequested = 0;
		if (!NativeReplayScheduler_StartReportRecording())
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
		if (!NativeReplayScheduler_WriteHeader())
		{
			Platform_Log("[CTR Replay] failed to update replay header frame count\n");
			return 1;
		}
		s_replayFrame++;
		s_beginOpen = 0;
		s_frameTimingConsumed = 0;
		NativeReplayScheduler_ResetVSyncPackets();

		if (s_stopRequested != 0)
		{
			Platform_Log("[CTR Replay] report finalized by hotkey: frames=%u\n", s_replayFrame);
			NativeReplayScheduler_CloseFiles();
			return 0;
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
