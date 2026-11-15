#include <shared/ctr_test.h>
#include <shared/ctr_test_scratch.h>
#include <platform/native_chaos_ipc.h>

static char s_base[1024];
static int s_playback;
static int s_errors;
static bool s_failDelete;

int NativeReplayScheduler_IsPlayback(void)
{
	return s_playback;
}

static bool RemovePathForTest(const char *path)
{
	if (s_failDelete)
	{
		return false;
	}
	return SDL_RemovePath(path);
}

#define SDL_RemovePath RemovePathForTest
#include <platform/native_chaos_ipc.c>
#undef SDL_RemovePath

const char *NativeAssets_GetBaseDir(void)
{
	return s_base;
}

void Platform_LogError(const char *format, ...)
{
	(void)format;
	s_errors++;
}

static void FixturePath(char *path, const char *name)
{
	snprintf(path, CHAOS_IPC_PATH_SIZE, "%s/chaos/%s", s_base, name);
}

static void WriteFile(const char *name, const char *content, size_t length)
{
	char path[CHAOS_IPC_PATH_SIZE];
	FixturePath(path, name);
	FILE *stream = fopen(path, "wb");
	if (stream == NULL)
	{
		CtrTest_Bail("could not publish IPC fixture");
	}
	fwrite(content, 1, length, stream);
	fclose(stream);
}

static bool Exists(const char *name)
{
	char path[CHAOS_IPC_PATH_SIZE];
	SDL_PathInfo info;
	FixturePath(path, name);
	return SDL_GetPathInfo(path, &info);
}

static int ReadAgain(char *command)
{
	SDL_Delay(CHAOS_IPC_POLL_MS + 1);
	return NativeChaosIPC_ReadCommand(command, NATIVE_CHAOS_COMMAND_SIZE);
}

static void CheckCatalogue(const char *expected)
{
	char path[CHAOS_IPC_PATH_SIZE];
	FixturePath(path, "effects");
	FILE *stream = fopen(path, "rb");
	char content[NATIVE_CHAOS_CATALOGUE_SIZE] = {0};
	if (stream == NULL)
	{
		CtrTest_Bail("effects catalogue was not published");
	}
	fread(content, 1, sizeof(content) - 1, stream);
	fclose(stream);
	CTR_TEST_CHECK(strcmp(content, expected) == 0, "the published catalogue exactly matches the active selection");
	CTR_TEST_CHECK(!Exists("effects.tmp"), "catalogue publication leaves only the final file");
}

int main(void)
{
	CtrTestScratch_Open(s_base, sizeof(s_base), "test_chaos_ipc", NULL);
	CTR_TEST_SECTION("catalogues are atomically replaced, including an empty selection");
	NativeChaosIPC_WriteEffectsCatalogue("fly-me-to-the-moon\nbig-kart-energy\n");
	CheckCatalogue("fly-me-to-the-moon\nbig-kart-energy\n");
	NativeChaosIPC_WriteEffectsCatalogue("mirror-mirror\n");
	CheckCatalogue("mirror-mirror\n");
	NativeChaosIPC_WriteEffectsCatalogue("");
	CheckCatalogue("");

	CTR_TEST_SECTION("only the fixed atomically published command is consumed");
	char command[NATIVE_CHAOS_COMMAND_SIZE];
	const char padded[] = " \tfly-me-to-the-moon\r\n";
	WriteFile("command.tmp", padded, strlen(padded));
	CTR_TEST_CHECK_EQ(ReadAgain(command), NATIVE_CHAOS_NO_COMMAND, "temporary files are ignored");
	char temporary[CHAOS_IPC_PATH_SIZE];
	char path[CHAOS_IPC_PATH_SIZE];
	FixturePath(temporary, "command.tmp");
	FixturePath(path, "command");
	CTR_TEST_CHECK(SDL_RenamePath(temporary, path), "the producer can atomically publish a completed command");
	CTR_TEST_CHECK_EQ(ReadAgain(command), NATIVE_CHAOS_COMMAND, "a published command is delivered");
	CTR_TEST_CHECK(strcmp(command, "fly-me-to-the-moon") == 0, "the engine receives a slug without surrounding whitespace");
	CTR_TEST_CHECK(!Exists("command"), "delivery deletes the command to release the slot");
	CTR_TEST_CHECK_EQ(ReadAgain(command), NATIVE_CHAOS_NO_COMMAND, "a consumed command cannot run twice");

	CTR_TEST_SECTION("failed deletion and playback retain commands without delivering them");
	WriteFile("command", "big-kart-energy", strlen("big-kart-energy"));
	s_failDelete = true;
	CTR_TEST_CHECK_EQ(ReadAgain(command), NATIVE_CHAOS_NO_COMMAND, "failed deletion prevents activation");
	CTR_TEST_CHECK(Exists("command"), "failed deletion keeps the pending command");
	CTR_TEST_CHECK_EQ(s_errors, 1, "deletion failures are logged");
	s_failDelete = false;
	s_playback = 1;
	CTR_TEST_CHECK_EQ(ReadAgain(command), NATIVE_CHAOS_NO_COMMAND, "playback never delivers live commands");
	CTR_TEST_CHECK(Exists("command"), "playback preserves the command for later");
	NativeChaosIPC_WriteEffectsCatalogue("fly-me-to-the-moon\n");
	CheckCatalogue("");
	s_playback = 0;
	CTR_TEST_CHECK_EQ(ReadAgain(command), NATIVE_CHAOS_COMMAND, "a retained command is available after playback");

	CTR_TEST_SECTION("invalid byte payloads are consumed and logged without reaching the engine");
	char oversized[NATIVE_CHAOS_COMMAND_SIZE];
	memset(oversized, 'x', sizeof(oversized));
	WriteFile("command", oversized, sizeof(oversized));
	CTR_TEST_CHECK_EQ(ReadAgain(command), NATIVE_CHAOS_INVALID_COMMAND, "oversized input is explicitly rejected");
	CTR_TEST_CHECK(!Exists("command"), "oversized input does not block the slot");
	const char binary[] = "fly-me-to-the-moon\0big-kart-energy";
	WriteFile("command", binary, sizeof(binary) - 1);
	CTR_TEST_CHECK_EQ(ReadAgain(command), NATIVE_CHAOS_INVALID_COMMAND, "embedded NUL cannot silently truncate a command");
	CTR_TEST_CHECK(!Exists("command"), "binary input is consumed");
	CTR_TEST_CHECK_EQ(s_errors, 3, "invalid payloads are logged once each");
	FixturePath(path, "effects");
	SDL_RemovePath(path);
	FixturePath(path, "");
	SDL_RemovePath(path);
	CtrTestScratch_Remove("test_chaos_ipc", NULL);
	return CtrTest_Summary();
}
