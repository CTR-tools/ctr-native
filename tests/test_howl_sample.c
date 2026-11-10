#include <shared/ctr_test.h>
#include <common.h>

struct sData sdata_static;
struct Data data;
NativeConfig g_config;
static struct HowlHeader s_header;
static struct OtherFX s_effect;
static struct SpuAddrEntry s_samples[2];
static u16 s_bankSector;
static u8 s_disc[8192];
static u8 s_written[32];
static int s_reads;
static int s_writtenCount;
static int s_failRead;
static u32 s_destination;
static u32 s_expectedDestination;
static struct ChannelStats s_channel;
static int s_mixedVolume;
static struct GameTracker s_tracker;
static bool s_freezeAudio;
static bool s_previewHumanAudio;
static int s_previewSound;
static int s_localizedReads;
static bool s_localizedAvailable;

bool Chaos_SetHumanAudio(bool human)
{
	const bool previous = sdata->chaosRuntime.humanAudioContext;
	sdata->chaosRuntime.humanAudioContext = human;
	return previous;
}

bool Chaos_ShouldFreezeAudioChannel(const struct ChannelStats *channel)
{
	(void)channel;
	return s_freezeAudio;
}

#include <game/HOWL/HOWL_Bank.c>
#include <game/HOWL/HOWL_Sample.c>
#include <game/HOWL/HOWL_OtherFX.c>
#include <game/HOWL/HOWL_Settings.c>
#include <game/HOWL/HOWL_Playback.c>
#include <game/LIST.c>

int NativeLanguageDisc_ReadVoiceSample(int language, int soundID, struct NativeLanguageVoiceSample *sample)
{
	s_localizedReads++;
	if (!s_localizedAvailable || language != 5 || soundID != 28) { return 0; }
	*sample = (struct NativeLanguageVoiceSample){.data = malloc(32), .size = 32, .pitch = 2048, .duration = 123, .flags = 4, .volume = 70};
	if (sample->data == NULL) { CtrTest_Bail("sample allocation failed"); }
	memset(sample->data, 0x5a, sample->size);
	return 1;
}

// Demand loading must not borrow the retail CD chain or MEMPACK allocator.
s32 MEMPACK_PushState(void) { CtrTest_Bail("sample cache must not use MEMPACK"); return 0; }
void MEMPACK_PopState(void) { CtrTest_Bail("sample cache must not use MEMPACK"); }
void *MEMPACK_AllocMem(s32 size, const char *name)
{
	(void)size; (void)name;
	CtrTest_Bail("sample cache must not use MEMPACK");
	return NULL;
}
void *MEMPACK_ReallocMem(s32 size)
{
	(void)size;
	CtrTest_Bail("sample cache must not use MEMPACK");
	return NULL;
}
int LOAD_HowlSectorChainStart(CdlFILE *file, void *destination, int sector, int count)
{
	(void)file; (void)destination; (void)sector; (void)count;
	CtrTest_Bail("sample cache must not use the retail CD chain");
	return 0;
}
int LOAD_HowlSectorChainEnd(void) { CtrTest_Bail("sample cache must not use the retail CD chain"); return 0; }

int NativeCD_ReadFileSectorsAt(const CdlFILE *file, unsigned int sector, unsigned int count, void *destination)
{
	(void)file;
	s_reads++;
	if (s_reads == s_failRead || ((u64)sector + count) * 2048 > sizeof(s_disc))
	{
		return 0;
	}
	memcpy(destination, s_disc + sector * 2048, count * 2048);
	return 1;
}
unsigned int SpuSetTransferStartAddr(unsigned int address)
{
	s_destination = address;
	return address;
}
unsigned int SpuWrite(unsigned char *bytes, unsigned int size)
{
	CTR_TEST_CHECK(s_writtenCount + size <= sizeof(s_written), "only the selected sample is uploaded");
	CTR_TEST_CHECK_EQ(s_destination, s_expectedDestination + (u32)s_writtenCount, "successive sector fragments remain contiguous in SPU memory");
	memcpy(s_written + s_writtenCount, bytes, size);
	s_writtenCount += size;
	return size;
}
int SpuIsTransferCompleted(int flag) { (void)flag; return 1; }

void Smart_EnterCriticalSection(void) {}
void Smart_ExitCriticalSection(void) {}
struct ChannelStats *Channel_AllocSlot_AntiSpam(s16 sound, u8 antiSpam, int flags, struct ChannelAttr *attr)
{
	(void)sound; (void)antiSpam;
	s_previewHumanAudio = sdata->chaosRuntime.humanAudioContext;
	s_previewSound = sound;
	s_channel = (struct ChannelStats){ .flags = 1 };
	LIST_Clear(&sdata->channelTaken);
	LIST_Clear(&sdata->channelFree);
	LIST_AddBack(&sdata->channelTaken, &s_channel.link.item);
	sdata->channelAttrNew[0] = *attr;
	sdata->ChannelUpdateFlags[0] = flags;
	return &s_channel;
}
int Channel_FindSound(int sound) { (void)sound; return 0; }
void Channel_SetVolume(struct ChannelAttr *attr, int volume, int lr)
{
	(void)lr;
	s_mixedVolume = volume;
	attr->audioL = volume;
	attr->audioR = volume;
}
struct ChannelStats *Channel_SearchFX_EditAttr(int type, int sound, int flags, struct ChannelAttr *attr)
{
	(void)type; (void)sound;
	sdata->channelAttrNew[0].pitch = attr->pitch;
	sdata->channelAttrNew[0].audioL = attr->audioL;
	sdata->channelAttrNew[0].audioR = attr->audioR;
	sdata->channelAttrNew[0].reverb = attr->reverb;
	sdata->ChannelUpdateFlags[0] |= flags;
	return &s_channel;
}
struct ChannelStats *Channel_SearchFX_Destroy(int type, int sound, int mask)
{
	(void)type; (void)sound; (void)mask;
	return NULL;
}

void CDSYS_XAPauseRequest(void) {}
void CseqMusic_Pause(void) {}
void CseqMusic_Resume(void) {}
void CseqMusic_StopAll(void) {}
void CseqMusic_Stop(u16 song) { (void)song; }
int CseqMusic_Start(u16 song, int p2, struct SongSet *set, int p4, int p5)
{
	(void)song; (void)p2; (void)set; (void)p4; (void)p5;
	s_previewHumanAudio = sdata->chaosRuntime.humanAudioContext;
	return 0;
}
u32 Music_GetHighestSongPlayIndex(void) { return 0; }
void Channel_ParseSongToChannels(void) {}
void Channel_UpdateChannels(void) {}
void Channel_DestroyAll_LowLevel(int opt, b32 keepMusic, u8 type)
{
	(void)opt; (void)keepMusic; (void)type;
}
void howl_InitChannelAttr_Music(struct SongSeq *seq, struct ChannelAttr *attr, int index, int volume)
{
	(void)seq; (void)attr; (void)index; (void)volume;
	CtrTest_Bail("effect gain tests must not resume music channels");
}

static void Reset(void)
{
	memset(&sdata_static, 0, sizeof(sdata_static));
	s_header = (struct HowlHeader){ .numSpuAddrs = 2, .numOtherFX = 1, .numBanks = 1 };
	s_effect = (struct OtherFX){ .spuIndex = 1, .volume = 76, .pitch = 4096 };
	s_samples[0] = (struct SpuAddrEntry){ .spuSize = 255 };
	s_samples[1] = (struct SpuAddrEntry){ .spuSize = 4 };
	s_bankSector = 1;
	sdata->gGT = &s_tracker;
	sdata->ptrHowlHeader = &s_header;
	sdata->howl_metaOtherFX = &s_effect;
	sdata->howl_spuAddrs = s_samples;
	sdata->howl_bankOffsets = &s_bankSector;
	sdata->boolAudioEnabled = 1;
	sdata->vol_FX = 200;
	sdata->vol_Voice = 100;
	sdata->bankLoadStage = 4;
	sdata->songLoadStage = 3;
	sdata->numAudioBanks = 1;
	sdata->bank[0] = (struct Bank){ .flags = 2, .min = 0x101, .max = 5 };
	sdata->audioAllocPtr = 0x106;
	s_expectedDestination = 0x20c * 8;
	const s16 indices[] = { 2, 0, 1 };
	memcpy(s_disc + 2048, indices, sizeof(indices));
	for (int i = 0; i < 32; i++)
	{
		s_disc[4096 + 2040 + i] = (u8)(i + 1);
	}
	s_reads = 0;
	s_writtenCount = 0;
	s_failRead = 0;
	s_localizedReads = 0;
	s_localizedAvailable = false;
	g_config.voiceLanguage = 1;
	Bank_InvalidateLocalizedVoices();
}

int main(void)
{
	CTR_TEST_SECTION("ordinary playback resolves a missing sample into a level-local cache");
	Reset();
	CTR_TEST_CHECK(OtherFX_Play(0, 1) != 0, "first playback loads and starts a missing sample");
	CTR_TEST_CHECK(memcmp(s_written, s_disc + 6136, 32) == 0, "sample offsets are correct across disc sectors");
	CTR_TEST_CHECK_EQ(s_samples[1].spuAddr, 0x106, "playback can resolve the cached sample");
	CTR_TEST_CHECK_EQ(sdata->audioAllocPtr, 0x108, "subsequent banks cannot overwrite the sample");
	CTR_TEST_CHECK_EQ(sdata->bank[0].max, 5, "the retained base bank does not acquire level-local samples");
	CTR_TEST_CHECK_EQ(sdata->bank[1].max, 2, "the cache bank owns the sample allocation");
	CTR_TEST_CHECK_EQ(sdata->numAudioBanks, 2, "extra samples share a separate bank lifetime");
	OtherFX_Play_Echo(0, 1, 1);
	CTR_TEST_CHECK_EQ(s_reads, 3, "repeated playback and echo reuse the loaded sample without I/O");
	CTR_TEST_CHECK_EQ(s_channel.echo, 1, "echo survives the common playback path");
	Bank_DestroyUntilIndex(0);
	CTR_TEST_CHECK_EQ(s_samples[1].spuAddr, 0, "level teardown invalidates cached sample addresses");
	CTR_TEST_CHECK_EQ(sdata->audioAllocPtr, 0x106, "level teardown returns SPU space to the allocator");
	CTR_TEST_CHECK_EQ(sdata->numAudioBanks, 1, "level teardown preserves the base bank");
	s_writtenCount = 0;
	CTR_TEST_CHECK(Bank_EnsureOtherFX(0), "the next level can reload an invalidated cache sample");

	CTR_TEST_SECTION("failed reads and unavailable resources preserve allocator ownership");
	Reset();
	s_failRead = 3;
	CTR_TEST_CHECK(!Bank_EnsureOtherFX(0), "a failed sample read rejects the sample");
	CTR_TEST_CHECK_EQ(s_samples[1].spuAddr, 0, "partial uploads cannot mark a sample playable");
	CTR_TEST_CHECK_EQ(sdata->audioAllocPtr, 0x106, "failed reads preserve the SPU allocator");
	CTR_TEST_CHECK_EQ(sdata->numAudioBanks, 1, "failed reads create no cache bank");
	sdata->audioAllocPtr = CTR_SPU_UNITS(CTR_SPU_CEILING) - 1;
	CTR_TEST_CHECK(!Bank_EnsureOtherFX(0), "samples cannot overlap reserved SPU memory");
	CTR_TEST_CHECK_EQ(s_reads, 3, "SPU capacity is checked before reading");
	Reset();
	sdata->audioAllocPtr = CTR_SPU_UNITS(0x80000);
	s_expectedDestination = 0x80000;
	CTR_TEST_CHECK(OtherFX_Play(0, 1) != 0, "a sample can be loaded and played above the retail SPU limit");
	CTR_TEST_CHECK_EQ(s_samples[1].spuAddr, CTR_SPU_UNITS(0x80000), "the upper-half address fits the sample table");
	CTR_TEST_CHECK_EQ((u32)sdata->channelAttrNew[0].spuStartAddr, 0x80000, "playback converts the upper-half sample address to bytes");
	Bank_DestroyUntilIndex(0);
	CTR_TEST_CHECK_EQ(s_samples[1].spuAddr, 0, "teardown clears an upper-half sample address");
	Reset();
	sdata->bankLoadStage = 2;
	CTR_TEST_CHECK(!Bank_EnsureOtherFX(0), "an asynchronous bank transfer cannot be interrupted");
	sdata->bankLoadStage = 4;
	sdata->songLoadStage = 1;
	CTR_TEST_CHECK(!Bank_EnsureOtherFX(0), "preloading waits for music loading to finish");
	CTR_TEST_CHECK_EQ(s_reads, 0, "busy audio loading does not touch the disc");
	Reset();
	sdata->numAudioBanks = len(sdata->bank);
	CTR_TEST_CHECK(!Bank_EnsureOtherFX(0), "a full bank table rejects a new cache");

	CTR_TEST_SECTION("per-playback gain doubles the notification without changing shared metadata");
	Reset();
	OtherFX_Play_LowLevel(0, 1, HOWL_SFX_DEFAULT_FLAGS | HOWL_SFX_GAIN_DOUBLE_FLAG | HOWL_SFX_ECHO_FLAG);
	CTR_TEST_CHECK_EQ(s_mixedVolume, 7570, "double gain reaches the real mixer without wrapping the byte-sized volume");
	CTR_TEST_CHECK_EQ(s_channel.vol, 255, "the channel retains its normal volume range");
	CTR_TEST_CHECK_EQ(s_channel.echo, 1, "gain is not passed to the SPU as echo");
	CTR_TEST_CHECK_EQ(s_effect.volume, 76, "gain does not modify the shared sound definition");
	CTR_TEST_CHECK_EQ(sdata->channelAttrNew[0].pitch, 4096, "initial playback uses the sound's base pitch");

	CTR_TEST_SECTION("real pause and slider paths preserve boosted gain and support muting");
	howl_PauseAudio();
	CTR_TEST_CHECK_EQ(sdata->numBackup_ChannelStats, 1, "pause backs up the boosted effect channel");
	sdata->channelAttrNew[0].audioL = 0;
	howl_UnPauseAudio();
	CTR_TEST_CHECK_EQ(sdata->channelAttrNew[0].audioL, 7570, "resume restores boosted gain through the real attribute initializer");
	CTR_TEST_CHECK_EQ(sdata->numBackup_ChannelStats, 0, "resume consumes the channel backup");
	howl_VolumeSet(HOWL_VOLUME_TYPE_FX, 100);
	CTR_TEST_CHECK_EQ(sdata->channelAttrNew[0].audioL, 3785, "the Effects slider updates active channels while retaining double gain");
	howl_VolumeSet(HOWL_VOLUME_TYPE_FX, 0);
	CTR_TEST_CHECK_EQ(sdata->channelAttrNew[0].audioL, 0, "the Effects slider mutes boosted playback");
	howl_VolumeSet(HOWL_VOLUME_TYPE_FX, 200);
	CTR_TEST_CHECK_EQ(sdata->channelAttrNew[0].audioL, 7570, "unmuting restores boosted gain");

	CTR_TEST_SECTION("modification shares volume and pitch preparation with initial playback");
	OtherFX_Modify(s_channel.soundID, HOWL_SFX_DEFAULT_FLAGS | HOWL_SFX_GAIN_DOUBLE_FLAG);
	CTR_TEST_CHECK_EQ(sdata->channelAttrNew[0].audioL, 7570, "modification uses the same boosted mix as initial playback");
	data.distortConst_OtherFX[0] = 0x8000;
	const u32 distortedFlags = HowlSfx_Pack(HOWL_SFX_LR_CENTER, 0, HOWL_SFX_VOLUME_MAX, 0);
	OtherFX_Modify(s_channel.soundID, distortedFlags);
	CTR_TEST_CHECK_EQ(sdata->channelAttrNew[0].pitch, 2048, "modification applies the shared pitch distortion calculation");
	CTR_TEST_CHECK_EQ(sdata->channelAttrNew[0].audioL, 3785, "modification can return boosted playback to normal gain");
	OtherFX_Play_LowLevel(0, 1, distortedFlags);
	CTR_TEST_CHECK_EQ(sdata->channelAttrNew[0].pitch, 2048, "initial playback uses the same distorted pitch as modification");
	s_effect.flags = 4;
	OtherFX_Play_LowLevel(0, 1, HOWL_SFX_DEFAULT_FLAGS | HOWL_SFX_GAIN_DOUBLE_FLAG);
	CTR_TEST_CHECK_EQ(sdata->channelAttrNew[0].audioL, 3785, "voice effects use the Voice slider in the shared initializer");
	howl_VolumeSet(HOWL_VOLUME_TYPE_VOICE, 200);
	CTR_TEST_CHECK_EQ(sdata->channelAttrNew[0].audioL, 7570, "the Voice slider retains gain through its real channel update path");
	s_effect.flags = 0;
	OtherFX_Play(0, 1);
	CTR_TEST_CHECK_EQ(s_mixedVolume, 3785, "ordinary playback retains vanilla gain");
	CTR_TEST_CHECK((s_channel.flags & HOWL_CHANNEL_GAIN_DOUBLE) == 0, "reused channels do not inherit boosted gain");
	CTR_TEST_SECTION("selectively frozen sounds retain their lifetime and resume without catch-up");
	s_channel.flags = 1;
	s_channel.timeLeft = 10;
	s_freezeAudio = true;
	howl_PlayAudio_Update();
	howl_PlayAudio_Update();
	CTR_TEST_CHECK_EQ(s_channel.timeLeft, 10, "frozen world sounds cannot expire while their samples are suspended");
	s_freezeAudio = false;
	howl_PlayAudio_Update();
	CTR_TEST_CHECK_EQ(s_channel.timeLeft, 5, "resume advances only one ordinary audio tick");
	howl_PlayAudio_Update();
	CTR_TEST_CHECK(sdata->channelTaken.first == NULL, "a resumed sound returns its channel when its remaining lifetime expires");
	CTR_TEST_SECTION("options slider playback uses human audio context without leaking it");
	Reset();
	struct OtherFX previewEffects[0x49];
	for (int i = 0; i < (int)len(previewEffects); i++)
	{
		previewEffects[i] = s_effect;
	}
	s_header.numOtherFX = len(previewEffects);
	sdata->howl_metaOtherFX = previewEffects;
	struct Driver previewDriver = {.driverID = 0};
	s_tracker.cameraDC[0].driverToFollow = &previewDriver;
	s_tracker.frameTimer_MainFrame_ResetDB = 0;
	const int rows[] = {HOWL_VOLUME_TYPE_FX, HOWL_VOLUME_TYPE_MUSIC, HOWL_VOLUME_TYPE_VOICE};
	for (int i = 0; i < (int)len(rows); i++)
	{
		s_previewHumanAudio = false;
		OptionsMenu_TestSound(rows[i], true);
		CTR_TEST_CHECK(s_previewHumanAudio, "effect, music and voice preview launch in human audio context");
		CTR_TEST_CHECK(!sdata->chaosRuntime.humanAudioContext, "slider playback restores the previous world audio context");
	}
	Chaos_SetHumanAudio(true);
	OptionsMenu_TestSound(HOWL_VOLUME_TYPE_VOICE, false);
	CTR_TEST_CHECK(sdata->chaosRuntime.humanAudioContext, "stopping preview preserves an enclosing human audio context");
	CTR_TEST_SECTION("voice preview without a followed racer");
	s_tracker.gameMode1 = MAIN_MENU;
	s_tracker.cameraDC[0].driverToFollow = NULL;
	GAME_CHARACTER_IDS[0] = NEO_CORTEX;
	OptionsMenu_TestSound(HOWL_VOLUME_TYPE_VOICE, true);
	CTR_TEST_CHECK(s_previewSound == NEO_CORTEX + 0x1c, "main-menu voice preview uses player 1 without a racer");
	previewDriver.driverID = 1;
	GAME_CHARACTER_IDS[1] = COCO_BANDICOOT;
	s_tracker.cameraDC[0].driverToFollow = &previewDriver;
	OptionsMenu_TestSound(HOWL_VOLUME_TYPE_VOICE, true);
	CTR_TEST_CHECK(s_previewSound == NEO_CORTEX + 0x1c, "main-menu voice preview ignores stale followed-racer state");
	s_tracker.gameMode1 = ARCADE_MODE | PAUSE_1;
	s_tracker.frameTimer_MainFrame_ResetDB = 25;
	OptionsMenu_TestSound(HOWL_VOLUME_TYPE_VOICE, true);
	CTR_TEST_CHECK(s_previewSound == COCO_BANDICOOT + 0x2c, "paused voice preview still follows the camera's racer and alternates clips");
	OptionsMenu_TestSound(HOWL_VOLUME_TYPE_VOICE, false);
	CTR_TEST_SECTION("localized sampled voices");
	Reset();
	struct OtherFX voiceEffects[60];
	for (int i = 0; i < (int)len(voiceEffects); i++) { voiceEffects[i] = s_effect; }
	s_header.numOtherFX = len(voiceEffects);
	sdata->howl_metaOtherFX = voiceEffects;
	s_samples[1].spuAddr = 0x101;
	s_localizedAvailable = true;
	g_config.voiceLanguage = 5;
	CTR_TEST_CHECK(OtherFX_Play(28, 1) != 0, "localized impact voice starts");
	CTR_TEST_CHECK_EQ(s_localizedReads, 1, "localized sample is loaded from the selected language");
	CTR_TEST_CHECK_EQ((u32)sdata->channelAttrNew[0].spuStartAddr, s_expectedDestination, "playback uses the localized SPU address");
	CTR_TEST_CHECK_EQ(sdata->channelAttrNew[0].pitch, 2048, "playback uses the localized pitch");
	CTR_TEST_CHECK_EQ(s_channel.timeLeft, 123, "playback uses the localized duration");
	s_samples[1].spuAddr = 0;
	sdata->bankLoadStage = 2;
	CTR_TEST_CHECK(OtherFX_Play(28, 1) != 0, "a cached localized voice plays even when the English sample cannot load");
	sdata->bankLoadStage = 4;
	s_samples[1].spuAddr = 0x101;
	OtherFX_Play(28, 1);
	CTR_TEST_CHECK_EQ(s_localizedReads, 1, "repeated playback reuses the localized sample");
	s_tracker.gameMode1 = MAIN_MENU;
	s_tracker.frameTimer_MainFrame_ResetDB = 0;
	GAME_CHARACTER_IDS[0] = 0;
	OptionsMenu_TestSound(HOWL_VOLUME_TYPE_VOICE, true);
	CTR_TEST_CHECK_EQ((u32)sdata->channelAttrNew[0].spuStartAddr, s_expectedDestination, "voice-volume preview uses the localized sample");
	OptionsMenu_TestSound(HOWL_VOLUME_TYPE_VOICE, false);
	Bank_DestroyUntilIndex(0);
	CTR_TEST_CHECK_EQ(sdata->audioAllocPtr, 0x106, "teardown releases localized SPU storage");
	s_writtenCount = 0;
	OtherFX_Play(28, 1);
	CTR_TEST_CHECK_EQ(s_localizedReads, 2, "a later level reloads invalidated samples");
	g_config.voiceLanguage = 1;
	OtherFX_Play(28, 1);
	CTR_TEST_CHECK_EQ((u32)sdata->channelAttrNew[0].spuStartAddr, CTR_SPU_BYTES_AT(0x101), "switching to English uses the original sample immediately");
	return CtrTest_Summary();
}
