#include <platform/native_audio.h>

#include <SDL3/SDL.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef s32 b32;

#define NATIVE_AUDIO_SAMPLE_RATE        44100
#define NATIVE_AUDIO_CHANNELS           2
#define NATIVE_AUDIO_SPU_VOICE_COUNT    24
#define NATIVE_AUDIO_SPU_MEMSIZE        (512 * 1024)
#define NATIVE_AUDIO_FP_SHIFT           16
#define NATIVE_AUDIO_FP_ONE             (1 << NATIVE_AUDIO_FP_SHIFT)
#define NATIVE_AUDIO_GAUSS_INDEX_SHIFT  8
#define NATIVE_AUDIO_DIRECT_VOL_MAX     0x4000
#define NATIVE_AUDIO_VBLANK_FRAMES         (NATIVE_AUDIO_SAMPLE_RATE / 60)
#define NATIVE_AUDIO_SCHEDULED_QUEUE_FRAMES (NATIVE_AUDIO_VBLANK_FRAMES * 16)
#define NATIVE_AUDIO_XA_ZIGZAG_TAPS        29
#define NATIVE_AUDIO_XA_ZIGZAG_PHASES      7
#define NATIVE_AUDIO_XA_ZIGZAG_INPUTS      6
// NOTE(aalhendi): Little-endian tag `CTRA` = CTR native Audio snapshot.
#define NATIVE_AUDIO_STATE_MAGIC           0x41525443
#define NATIVE_AUDIO_STATE_VERSION         1
#define NATIVE_AUDIO_ARENA_ALIGN           16
#define NATIVE_AUDIO_ADSR_MIN              (-0x8000)
#define NATIVE_AUDIO_ADSR_MAX              0x7fff
#define NATIVE_AUDIO_ADSR_STEP_BIT         0x8000u
#define NATIVE_AUDIO_REVERB_MAX_BYTES      0x18040
#define NATIVE_AUDIO_REVERB_MAX_SAMPLES    (NATIVE_AUDIO_REVERB_MAX_BYTES / (int)sizeof(s16))
#define NATIVE_AUDIO_REVERB_FIR_TAPS       39

#define XA_NUM_TYPES                    3
#define XA_HEADER_SIZE                  0x44
#define XA_NUM_XAS_TOTAL_OFFSET         0x0c
#define XA_NUM_TRACKS_TOTAL_OFFSET      0x10
#define XA_NUM_SONGS_OFFSET             0x2c
#define XA_FIRST_SONG_INDEX_OFFSET      0x38
#define XA_SIZE_ENTRY_BYTES             4
#define XA_FORM2_SECTOR_SIZE            2336
#define XA_FULL_SECTOR_SIZE             2352
#define XA_FRAMES_PER_SECTOR            18
#define XA_FRAME_SIZE                   128
#define XA_SUBHEADER_SIZE               8
#define XA_SAMPLES_PER_SOUND_UNIT       28
#define XA_BLOCKS_PER_FRAME             4
#define XA_SUBFRAMES_PER_FRAME          8
#define XA_SAMPLE_RATE_37800            37800
#define XA_SAMPLE_RATE_18900            18900

enum
{
	ADPCM_LOOP_END = 1 << 0,
	ADPCM_REPEAT = 1 << 1,
	ADPCM_LOOP_START = 1 << 2
};

enum NativeAudioAdsrPhase
{
	NATIVE_AUDIO_ADSR_OFF,
	NATIVE_AUDIO_ADSR_ATTACK,
	NATIVE_AUDIO_ADSR_DECAY,
	NATIVE_AUDIO_ADSR_SUSTAIN,
	NATIVE_AUDIO_ADSR_RELEASE
};

enum NativeAudioReverbRegister
{
	NATIVE_AUDIO_REV_DAPF1,
	NATIVE_AUDIO_REV_DAPF2,
	NATIVE_AUDIO_REV_VIIR,
	NATIVE_AUDIO_REV_VCOMB1,
	NATIVE_AUDIO_REV_VCOMB2,
	NATIVE_AUDIO_REV_VCOMB3,
	NATIVE_AUDIO_REV_VCOMB4,
	NATIVE_AUDIO_REV_VWALL,
	NATIVE_AUDIO_REV_VAPF1,
	NATIVE_AUDIO_REV_VAPF2,
	NATIVE_AUDIO_REV_MLSAME,
	NATIVE_AUDIO_REV_MRSAME,
	NATIVE_AUDIO_REV_MLCOMB1,
	NATIVE_AUDIO_REV_MRCOMB1,
	NATIVE_AUDIO_REV_MLCOMB2,
	NATIVE_AUDIO_REV_MRCOMB2,
	NATIVE_AUDIO_REV_DLSAME,
	NATIVE_AUDIO_REV_DRSAME,
	NATIVE_AUDIO_REV_MLDIFF,
	NATIVE_AUDIO_REV_MRDIFF,
	NATIVE_AUDIO_REV_MLCOMB3,
	NATIVE_AUDIO_REV_MRCOMB3,
	NATIVE_AUDIO_REV_MLCOMB4,
	NATIVE_AUDIO_REV_MRCOMB4,
	NATIVE_AUDIO_REV_DLDIFF,
	NATIVE_AUDIO_REV_DRDIFF,
	NATIVE_AUDIO_REV_MLAPF1,
	NATIVE_AUDIO_REV_MRAPF1,
	NATIVE_AUDIO_REV_MLAPF2,
	NATIVE_AUDIO_REV_MRAPF2,
	NATIVE_AUDIO_REV_VLIN,
	NATIVE_AUDIO_REV_VRIN,
	NATIVE_AUDIO_REV_REG_COUNT
};

struct NativeAudioReverbPreset
{
	s32 mode;
	s32 sizeBytes;
	s16 reg[NATIVE_AUDIO_REV_REG_COUNT];
};

struct NativeAudioReverbState
{
	s16 buffer[NATIVE_AUDIO_REVERB_MAX_SAMPLES];
	s16 inputHistoryLeft[NATIVE_AUDIO_REVERB_FIR_TAPS];
	s16 inputHistoryRight[NATIVE_AUDIO_REVERB_FIR_TAPS];
	s16 outputHistoryLeft[NATIVE_AUDIO_REVERB_FIR_TAPS];
	s16 outputHistoryRight[NATIVE_AUDIO_REVERB_FIR_TAPS];
	s32 mode;
	s32 sizeSamples;
	s32 cursor;
	s32 inputHistoryCursor;
	s32 outputHistoryCursor;
	s32 samplePhase;
	s16 lastOutLeft;
	s16 lastOutRight;
};

struct NativeAudioSpuArena
{
	// NOTE(aalhendi): Emulated PS1 SPU RAM; this is snapshot state, not host PCM.
	u8 memory[NATIVE_AUDIO_SPU_MEMSIZE];
	int allocCursor;
	int transferOffset;
};

struct NativeAudioOutput
{
	SDL_AudioDeviceID device;
	SDL_AudioStream *stream;
	b32 deterministicRenderMode;
	s16 scheduledPcm[NATIVE_AUDIO_SCHEDULED_QUEUE_FRAMES * NATIVE_AUDIO_CHANNELS];
	int scheduledReadFrame;
	int scheduledFrameCount;
#ifdef CTR_INTERNAL
	int underrunFrames;
	int overflowFrames;
	int reportVBlankCountdown;
	int lastReportedUnderrunFrames;
	int lastReportedOverflowFrames;
	int callbackMaxRequestFrames;
	int lastReportedCallbackMaxRequestFrames;
#endif
};

struct NativeAudioDecodeArena
{
	// NOTE(aalhendi): Native rebuildable cache memory. Do not snapshot this;
	// save SPU RAM, XA identity, and playback cursors, then rebuild decoded PCM on restore.
	u8 *memory;
	int capacity;
	int used;
};

struct NativeAudioVoice
{
	SpuVoiceAttr attr;
	// NOTE(aalhendi): Points into voicePcmArena; the source of truth is compressed SPU RAM at attr.addr.
	s16 *pcm;
	int sampleCount;
	int loopStart;
	int loopEnd;
	b32 loopEnabled;
	b32 active;
	// SPU-backed long samples can exceed the 65,535-frame range of u32 16.16.
	u64 positionFp;
	b32 looped;
	u16 reverb;
	b32 sampleDirty;
	s32 adsrLevel;
	u32 adsrCounter;
	u8 adsrPhase;
};

struct NativeAudioXA
{
	// NOTE(aalhendi): Points into xaPcmArena; rebuild from the XA file identity during restore.
	s16 *pcm;
	int frameCount;
	int sampleRate;
	int categoryID;
	int xaID;
	b32 hasTrackIdentity;
	b32 active;
	// XA streams can exceed the 65,535-frame range of a u32 16.16 cursor.
	u64 positionFp;
	u64 outputFrame;
	u32 stepFp;
	s16 volumeLeft;
	s16 volumeRight;
};

struct NativeAudioState
{
	b32 init;
	b32 muted;
	b32 reverbEnabled;
	b32 cdMixEnabled;
	b32 cdReverbEnabled;
	b32 reverbWorkAreaReserved;
	s16 masterVolumeLeft;
	s16 masterVolumeRight;
	u32 reverbVoiceBits;
	SpuReverbAttr reverbAttr;
	struct NativeAudioReverbState reverb;
	SpuCommonAttr commonAttr;
	struct NativeAudioSpuArena spu;
	struct NativeAudioDecodeArena voicePcmArena;
	struct NativeAudioVoice voices[NATIVE_AUDIO_SPU_VOICE_COUNT];
	struct NativeAudioXA xa;
	struct NativeAudioDecodeArena xaPcmArena;
	struct NativeAudioDecodeArena xaPendingPcmArena;
	struct NativeAudioOutput output;
};

struct NativeAudioByteBuffer
{
	u8 *data;
	int size;
};

struct NativeAudioPcmBuffer
{
	s16 *samples;
	int count;
	int capacity;
};

struct NativeAudioXaTrackInfo
{
	int fileNumber;
	int channelFilter;
	int numSectors;
};

struct NativeAudioXaDecodeState
{
	int old[2];
	int older[2];
};

struct NativeAudioVoiceState
{
	SpuVoiceAttr attr;
	int sampleCount;
	int loopStart;
	int loopEnd;
	b32 loopEnabled;
	b32 active;
	u64 positionFp;
	b32 looped;
	u16 reverb;
	b32 sampleDirty;
	s32 adsrLevel;
	u32 adsrCounter;
	u8 adsrPhase;
};

struct NativeAudioXAState
{
	int frameCount;
	int sampleRate;
	int categoryID;
	int xaID;
	b32 hasTrackIdentity;
	b32 active;
	u64 positionFp;
	u64 outputFrame;
	u32 stepFp;
	s16 volumeLeft;
	s16 volumeRight;
};

struct NativeAudioSnapshot
{
	u32 magic;
	u32 version;
	u32 size;
	b32 init;
	b32 muted;
	int spuAllocCursor;
	b32 reverbEnabled;
	b32 cdMixEnabled;
	b32 cdReverbEnabled;
	b32 reverbWorkAreaReserved;
	int spuTransferOffset;
	s16 masterVolumeLeft;
	s16 masterVolumeRight;
	u32 reverbVoiceBits;
	SpuReverbAttr reverbAttr;
	struct NativeAudioReverbState reverb;
	SpuCommonAttr commonAttr;
	struct NativeAudioVoiceState voices[NATIVE_AUDIO_SPU_VOICE_COUNT];
	struct NativeAudioXAState xa;
	u8 spuSampleMem[NATIVE_AUDIO_SPU_MEMSIZE];
};

static struct NativeAudioState s_audio;

static b32 NativeAudio_OutputOpen(void)
{
	return s_audio.output.stream != NULL;
}

static void NativeAudio_LockOutput(void)
{
	if (s_audio.output.stream != NULL)
		SDL_LockAudioStream(s_audio.output.stream);
}

static void NativeAudio_UnlockOutput(void)
{
	if (s_audio.output.stream != NULL)
		SDL_UnlockAudioStream(s_audio.output.stream);
}

static const int s_posTable[5] = {0, 60, 115, 98, 122};
static const int s_negTable[5] = {0, 0, -52, -55, -60};
// NOTE(aalhendi): PS1 SPU Gaussian interpolation coefficient table.
// Source reference: https://psx-spx.consoledev.net/soundprocessingunitspu/
static const s16 s_gaussTable[512] = {
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    0,     0,     0,     0,     0,     0,
    0,     1,     1,     1,     1,     2,     2,     2,     3,     3,     3,     4,     4,     5,     5,     6,     7,     7,     8,     9,     9,     10,
    11,    12,    13,    14,    15,    16,    17,    18,    19,    21,    22,    24,    25,    27,    28,    30,    32,    33,    35,    37,    39,    41,
    44,    46,    48,    51,    53,    56,    58,    61,    64,    67,    70,    73,    77,    80,    84,    87,    91,    95,    99,    103,   107,   111,
    116,   120,   125,   130,   135,   140,   145,   150,   156,   161,   167,   173,   179,   186,   192,   199,   205,   212,   219,   227,   234,   242,
    250,   257,   266,   274,   283,   291,   300,   309,   319,   328,   338,   348,   358,   369,   379,   390,   401,   412,   424,   436,   448,   460,
    473,   485,   498,   512,   525,   539,   553,   567,   582,   597,   612,   627,   643,   659,   675,   692,   708,   726,   743,   761,   779,   797,
    816,   835,   854,   874,   894,   914,   935,   956,   977,   999,   1020,  1043,  1066,  1089,  1112,  1136,  1160,  1184,  1209,  1234,  1260,  1286,
    1312,  1339,  1366,  1394,  1422,  1450,  1479,  1508,  1537,  1567,  1598,  1628,  1660,  1691,  1723,  1756,  1789,  1822,  1856,  1890,  1924,  1959,
    1995,  2031,  2067,  2104,  2141,  2179,  2217,  2256,  2295,  2334,  2374,  2415,  2456,  2497,  2539,  2582,  2624,  2668,  2712,  2756,  2801,  2846,
    2892,  2938,  2985,  3032,  3079,  3128,  3176,  3225,  3275,  3325,  3376,  3427,  3479,  3531,  3584,  3637,  3691,  3745,  3799,  3855,  3910,  3967,
    4023,  4081,  4138,  4197,  4255,  4315,  4374,  4435,  4495,  4557,  4619,  4681,  4744,  4807,  4871,  4935,  5000,  5065,  5131,  5197,  5264,  5332,
    5399,  5468,  5536,  5606,  5676,  5746,  5817,  5888,  5959,  6032,  6104,  6177,  6251,  6325,  6400,  6475,  6550,  6626,  6702,  6779,  6856,  6934,
    7012,  7091,  7170,  7249,  7329,  7409,  7490,  7571,  7653,  7735,  7817,  7900,  7983,  8066,  8150,  8234,  8319,  8404,  8489,  8575,  8661,  8748,
    8834,  8922,  9009,  9097,  9185,  9273,  9362,  9451,  9541,  9630,  9720,  9811,  9901,  9992,  10083, 10174, 10266, 10358, 10450, 10542, 10635, 10727,
    10820, 10913, 11007, 11100, 11194, 11288, 11382, 11476, 11571, 11665, 11760, 11855, 11950, 12045, 12140, 12236, 12331, 12427, 12522, 12618, 12714, 12809,
    12905, 13001, 13097, 13193, 13289, 13385, 13481, 13577, 13673, 13769, 13865, 13961, 14056, 14152, 14248, 14343, 14439, 14534, 14630, 14725, 14820, 14915,
    15010, 15104, 15199, 15293, 15387, 15481, 15575, 15669, 15762, 15855, 15948, 16041, 16133, 16226, 16317, 16409, 16500, 16592, 16682, 16773, 16863, 16953,
    17042, 17131, 17220, 17308, 17396, 17484, 17571, 17658, 17744, 17830, 17916, 18001, 18086, 18170, 18254, 18337, 18420, 18502, 18584, 18665, 18746, 18826,
    18905, 18985, 19063, 19141, 19219, 19295, 19372, 19447, 19522, 19597, 19671, 19744, 19816, 19888, 19959, 20030, 20100, 20169, 20238, 20306, 20373, 20439,
    20505, 20570, 20634, 20698, 20760, 20822, 20884, 20944, 21004, 21063, 21121, 21178, 21235, 21290, 21345, 21399, 21452, 21505, 21556, 21607, 21657, 21706,
    21754, 21801, 21848, 21893, 21938, 21982, 22025, 22066, 22107, 22148, 22187, 22225, 22262, 22299, 22334, 22369, 22402, 22435, 22467, 22498, 22527, 22556,
    22584, 22611, 22637, 22662, 22686, 22709, 22731, 22752, 22772, 22791, 22809, 22826, 22842, 22857, 22872, 22885, 22897, 22908, 22918, 22927, 22935, 22942,
    22948, 22953, 22957, 22960, 22962, 22963,
};

// NOTE(aalhendi): PS1 CD-XA 37800Hz->44100Hz zig-zag interpolation table.
// Source reference: PSX-SPX, "CDROM XA Audio ADPCM Compression".
static const s16 s_xaZigZagTable[NATIVE_AUDIO_XA_ZIGZAG_TAPS][NATIVE_AUDIO_XA_ZIGZAG_PHASES] = {
    {0,       0,       0,       0,      -0x0001,  0x0002, -0x0005},
    {0,       0,       0,      -0x0001,  0x0003, -0x0008,  0x0011},
    {0,       0,      -0x0001,  0x0003, -0x0008,  0x0010, -0x0023},
    {0,      -0x0002,  0x0003, -0x0008,  0x0011, -0x0023,  0x0046},
    {0,       0,      -0x0002,  0x0006, -0x0010,  0x002B, -0x0017},
    {-0x0002, 0x0003, -0x0005,  0x0005,  0x000A,  0x001A, -0x0044},
    {0x000A, -0x0013,  0x001F, -0x001B,  0x006B, -0x00EB,  0x015B},
    {-0x0022, 0x003C, -0x004A,  0x00A6, -0x016D,  0x027B, -0x0347},
    {0x0041, -0x004B,  0x00B3, -0x01A8,  0x0350, -0x0548,  0x080E},
    {-0x0054, 0x00A2, -0x0192,  0x0372, -0x0623,  0x0AFA, -0x1249},
    {0x0034, -0x00E3,  0x02B1, -0x05BF,  0x0BCD, -0x16FA,  0x3C07},
    {0x0009,  0x0132, -0x039E,  0x09B8, -0x1780,  0x53E0,  0x53E0},
    {-0x010A, -0x0043, 0x04F8, -0x11B4,  0x6794,  0x3C07, -0x16FA},
    {0x0400, -0x0267, -0x05A6,  0x74BB,  0x234C, -0x1249,  0x0AFA},
    {-0x0A78, 0x0C9D,  0x7939,  0x0C9D, -0x0A78,  0x080E, -0x0548},
    {0x234C,  0x74BB, -0x05A6, -0x0267,  0x0400, -0x0347,  0x027B},
    {0x6794, -0x11B4,  0x04F8, -0x0043, -0x010A,  0x015B, -0x00EB},
    {-0x1780, 0x09B8, -0x039E,  0x0132,  0x0009, -0x0044,  0x001A},
    {0x0BCD, -0x05BF,  0x02B1, -0x00E3,  0x0034, -0x0017,  0x002B},
    {-0x0623, 0x0372, -0x0192,  0x00A2, -0x0054,  0x0046, -0x0023},
    {0x0350, -0x01A8,  0x00B3, -0x004B,  0x0041, -0x0023,  0x0010},
    {-0x016D, 0x00A6, -0x004A,  0x003C, -0x0022,  0x0011, -0x0008},
    {0x006B, -0x001B,  0x001F, -0x0013,  0x000A, -0x0005,  0x0002},
    {0x000A,  0x0005, -0x0005,  0x0003, -0x0001,  0,       0},
    {-0x0010, 0x0006, -0x0002,  0,       0,       0,       0},
    {0x0011, -0x0008,  0x0003, -0x0002,  0x0001,  0,       0},
    {-0x0008, 0x0003, -0x0001,  0,       0,       0,       0},
    {0x0003, -0x0001,  0,       0,       0,       0,       0},
    {-0x0001, 0,       0,       0,       0,       0,       0},
};

// NOTE(aalhendi): PS1 SPU reverb 44.1kHz<->22.05kHz FIR resampler.
// Source reference: https://psx-spx.consoledev.net/soundprocessingunitspu/
static const s16 s_reverbFirCoeffs[NATIVE_AUDIO_REVERB_FIR_TAPS] = {
    -0x0001, 0x0000, 0x0002, 0x0000,  -0x000A, 0x0000, 0x0023, 0x0000, -0x0067, 0x0000,  0x010A, 0x0000, -0x0268,
    0x0000,  0x0534, 0x0000, -0x0B90, 0x0000,  0x2806, 0x4000, 0x2806, 0x0000,  -0x0B90, 0x0000, 0x0534, 0x0000,
    -0x0268, 0x0000, 0x010A, 0x0000,  -0x0067, 0x0000, 0x0023, 0x0000, -0x000A, 0x0000,  0x0002, 0x0000, -0x0001,
};

#define NATIVE_AUDIO_REV(value) ((s16)(value))

// NOTE(aalhendi): PS1 SPU reverb preset registers in libspu mode order.
// Source reference: https://psx-spx.consoledev.net/soundprocessingunitspu/
static const struct NativeAudioReverbPreset s_reverbPresets[] = {
    {SPU_REV_MODE_OFF, 0x10, {NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0x0000),
                              NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0x0000),
                              NATIVE_AUDIO_REV(0x0001), NATIVE_AUDIO_REV(0x0001), NATIVE_AUDIO_REV(0x0001), NATIVE_AUDIO_REV(0x0001), NATIVE_AUDIO_REV(0x0001),
                              NATIVE_AUDIO_REV(0x0001), NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0x0001), NATIVE_AUDIO_REV(0x0001),
                              NATIVE_AUDIO_REV(0x0001), NATIVE_AUDIO_REV(0x0001), NATIVE_AUDIO_REV(0x0001), NATIVE_AUDIO_REV(0x0001), NATIVE_AUDIO_REV(0x0000),
                              NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0x0001), NATIVE_AUDIO_REV(0x0001), NATIVE_AUDIO_REV(0x0001), NATIVE_AUDIO_REV(0x0001),
                              NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0x0000)}},
    {SPU_REV_MODE_ROOM, 0x26C0, {NATIVE_AUDIO_REV(0x007D), NATIVE_AUDIO_REV(0x005B), NATIVE_AUDIO_REV(0x6D80), NATIVE_AUDIO_REV(0x54B8),
                                 NATIVE_AUDIO_REV(0xBED0), NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0xBA80),
                                 NATIVE_AUDIO_REV(0x5800), NATIVE_AUDIO_REV(0x5300), NATIVE_AUDIO_REV(0x04D6), NATIVE_AUDIO_REV(0x0333),
                                 NATIVE_AUDIO_REV(0x03F0), NATIVE_AUDIO_REV(0x0227), NATIVE_AUDIO_REV(0x0374), NATIVE_AUDIO_REV(0x01EF),
                                 NATIVE_AUDIO_REV(0x0334), NATIVE_AUDIO_REV(0x01B5), NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0x0000),
                                 NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0x0000),
                                 NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0x01B4), NATIVE_AUDIO_REV(0x0136),
                                 NATIVE_AUDIO_REV(0x00B8), NATIVE_AUDIO_REV(0x005C), NATIVE_AUDIO_REV(0x8000), NATIVE_AUDIO_REV(0x8000)}},
    {SPU_REV_MODE_STUDIO_A, 0x1F40, {NATIVE_AUDIO_REV(0x0033), NATIVE_AUDIO_REV(0x0025), NATIVE_AUDIO_REV(0x70F0), NATIVE_AUDIO_REV(0x4FA8),
                                     NATIVE_AUDIO_REV(0xBCE0), NATIVE_AUDIO_REV(0x4410), NATIVE_AUDIO_REV(0xC0F0), NATIVE_AUDIO_REV(0x9C00),
                                     NATIVE_AUDIO_REV(0x5280), NATIVE_AUDIO_REV(0x4EC0), NATIVE_AUDIO_REV(0x03E4), NATIVE_AUDIO_REV(0x031B),
                                     NATIVE_AUDIO_REV(0x03A4), NATIVE_AUDIO_REV(0x02AF), NATIVE_AUDIO_REV(0x0372), NATIVE_AUDIO_REV(0x0266),
                                     NATIVE_AUDIO_REV(0x031C), NATIVE_AUDIO_REV(0x025D), NATIVE_AUDIO_REV(0x025C), NATIVE_AUDIO_REV(0x018E),
                                     NATIVE_AUDIO_REV(0x022F), NATIVE_AUDIO_REV(0x0135), NATIVE_AUDIO_REV(0x01D2), NATIVE_AUDIO_REV(0x00B7),
                                     NATIVE_AUDIO_REV(0x018F), NATIVE_AUDIO_REV(0x00B5), NATIVE_AUDIO_REV(0x00B4), NATIVE_AUDIO_REV(0x0080),
                                     NATIVE_AUDIO_REV(0x004C), NATIVE_AUDIO_REV(0x0026), NATIVE_AUDIO_REV(0x8000), NATIVE_AUDIO_REV(0x8000)}},
    {SPU_REV_MODE_STUDIO_B, 0x4840, {NATIVE_AUDIO_REV(0x00B1), NATIVE_AUDIO_REV(0x007F), NATIVE_AUDIO_REV(0x70F0), NATIVE_AUDIO_REV(0x4FA8),
                                     NATIVE_AUDIO_REV(0xBCE0), NATIVE_AUDIO_REV(0x4510), NATIVE_AUDIO_REV(0xBEF0), NATIVE_AUDIO_REV(0xB4C0),
                                     NATIVE_AUDIO_REV(0x5280), NATIVE_AUDIO_REV(0x4EC0), NATIVE_AUDIO_REV(0x0904), NATIVE_AUDIO_REV(0x076B),
                                     NATIVE_AUDIO_REV(0x0824), NATIVE_AUDIO_REV(0x065F), NATIVE_AUDIO_REV(0x07A2), NATIVE_AUDIO_REV(0x0616),
                                     NATIVE_AUDIO_REV(0x076C), NATIVE_AUDIO_REV(0x05ED), NATIVE_AUDIO_REV(0x05EC), NATIVE_AUDIO_REV(0x042E),
                                     NATIVE_AUDIO_REV(0x050F), NATIVE_AUDIO_REV(0x0305), NATIVE_AUDIO_REV(0x0462), NATIVE_AUDIO_REV(0x02B7),
                                     NATIVE_AUDIO_REV(0x042F), NATIVE_AUDIO_REV(0x0265), NATIVE_AUDIO_REV(0x0264), NATIVE_AUDIO_REV(0x01B2),
                                     NATIVE_AUDIO_REV(0x0100), NATIVE_AUDIO_REV(0x0080), NATIVE_AUDIO_REV(0x8000), NATIVE_AUDIO_REV(0x8000)}},
    {SPU_REV_MODE_STUDIO_C, 0x6FE0, {NATIVE_AUDIO_REV(0x00E3), NATIVE_AUDIO_REV(0x00A9), NATIVE_AUDIO_REV(0x6F60), NATIVE_AUDIO_REV(0x4FA8),
                                     NATIVE_AUDIO_REV(0xBCE0), NATIVE_AUDIO_REV(0x4510), NATIVE_AUDIO_REV(0xBEF0), NATIVE_AUDIO_REV(0xA680),
                                     NATIVE_AUDIO_REV(0x5680), NATIVE_AUDIO_REV(0x52C0), NATIVE_AUDIO_REV(0x0DFB), NATIVE_AUDIO_REV(0x0B58),
                                     NATIVE_AUDIO_REV(0x0D09), NATIVE_AUDIO_REV(0x0A3C), NATIVE_AUDIO_REV(0x0BD9), NATIVE_AUDIO_REV(0x0973),
                                     NATIVE_AUDIO_REV(0x0B59), NATIVE_AUDIO_REV(0x08DA), NATIVE_AUDIO_REV(0x08D9), NATIVE_AUDIO_REV(0x05E9),
                                     NATIVE_AUDIO_REV(0x07EC), NATIVE_AUDIO_REV(0x04B0), NATIVE_AUDIO_REV(0x06EF), NATIVE_AUDIO_REV(0x03D2),
                                     NATIVE_AUDIO_REV(0x05EA), NATIVE_AUDIO_REV(0x031D), NATIVE_AUDIO_REV(0x031C), NATIVE_AUDIO_REV(0x0238),
                                     NATIVE_AUDIO_REV(0x0154), NATIVE_AUDIO_REV(0x00AA), NATIVE_AUDIO_REV(0x8000), NATIVE_AUDIO_REV(0x8000)}},
    {SPU_REV_MODE_HALL, 0xADE0, {NATIVE_AUDIO_REV(0x01A5), NATIVE_AUDIO_REV(0x0139), NATIVE_AUDIO_REV(0x6000), NATIVE_AUDIO_REV(0x5000),
                                 NATIVE_AUDIO_REV(0x4C00), NATIVE_AUDIO_REV(0xB800), NATIVE_AUDIO_REV(0xBC00), NATIVE_AUDIO_REV(0xC000),
                                 NATIVE_AUDIO_REV(0x6000), NATIVE_AUDIO_REV(0x5C00), NATIVE_AUDIO_REV(0x15BA), NATIVE_AUDIO_REV(0x11BB),
                                 NATIVE_AUDIO_REV(0x14C2), NATIVE_AUDIO_REV(0x10BD), NATIVE_AUDIO_REV(0x11BC), NATIVE_AUDIO_REV(0x0DC1),
                                 NATIVE_AUDIO_REV(0x11C0), NATIVE_AUDIO_REV(0x0DC3), NATIVE_AUDIO_REV(0x0DC0), NATIVE_AUDIO_REV(0x09C1),
                                 NATIVE_AUDIO_REV(0x0BC4), NATIVE_AUDIO_REV(0x07C1), NATIVE_AUDIO_REV(0x0A00), NATIVE_AUDIO_REV(0x06CD),
                                 NATIVE_AUDIO_REV(0x09C2), NATIVE_AUDIO_REV(0x05C1), NATIVE_AUDIO_REV(0x05C0), NATIVE_AUDIO_REV(0x041A),
                                 NATIVE_AUDIO_REV(0x0274), NATIVE_AUDIO_REV(0x013A), NATIVE_AUDIO_REV(0x8000), NATIVE_AUDIO_REV(0x8000)}},
    {SPU_REV_MODE_SPACE, 0xF6C0, {NATIVE_AUDIO_REV(0x033D), NATIVE_AUDIO_REV(0x0231), NATIVE_AUDIO_REV(0x7E00), NATIVE_AUDIO_REV(0x5000),
                                  NATIVE_AUDIO_REV(0xB400), NATIVE_AUDIO_REV(0xB000), NATIVE_AUDIO_REV(0x4C00), NATIVE_AUDIO_REV(0xB000),
                                  NATIVE_AUDIO_REV(0x6000), NATIVE_AUDIO_REV(0x5400), NATIVE_AUDIO_REV(0x1ED6), NATIVE_AUDIO_REV(0x1A31),
                                  NATIVE_AUDIO_REV(0x1D14), NATIVE_AUDIO_REV(0x183B), NATIVE_AUDIO_REV(0x1BC2), NATIVE_AUDIO_REV(0x16B2),
                                  NATIVE_AUDIO_REV(0x1A32), NATIVE_AUDIO_REV(0x15EF), NATIVE_AUDIO_REV(0x15EE), NATIVE_AUDIO_REV(0x1055),
                                  NATIVE_AUDIO_REV(0x1334), NATIVE_AUDIO_REV(0x0F2D), NATIVE_AUDIO_REV(0x11F6), NATIVE_AUDIO_REV(0x0C5D),
                                  NATIVE_AUDIO_REV(0x1056), NATIVE_AUDIO_REV(0x0AE1), NATIVE_AUDIO_REV(0x0AE0), NATIVE_AUDIO_REV(0x07A2),
                                  NATIVE_AUDIO_REV(0x0464), NATIVE_AUDIO_REV(0x0232), NATIVE_AUDIO_REV(0x8000), NATIVE_AUDIO_REV(0x8000)}},
    {SPU_REV_MODE_ECHO, 0x18040, {NATIVE_AUDIO_REV(0x0001), NATIVE_AUDIO_REV(0x0001), NATIVE_AUDIO_REV(0x7FFF), NATIVE_AUDIO_REV(0x7FFF),
                                  NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0x8100),
                                  NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0x1FFF), NATIVE_AUDIO_REV(0x0FFF),
                                  NATIVE_AUDIO_REV(0x1005), NATIVE_AUDIO_REV(0x0005), NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0x0000),
                                  NATIVE_AUDIO_REV(0x1005), NATIVE_AUDIO_REV(0x0005), NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0x0000),
                                  NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0x0000),
                                  NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0x1004), NATIVE_AUDIO_REV(0x1002),
                                  NATIVE_AUDIO_REV(0x0004), NATIVE_AUDIO_REV(0x0002), NATIVE_AUDIO_REV(0x8000), NATIVE_AUDIO_REV(0x8000)}},
    {SPU_REV_MODE_DELAY, 0x18040, {NATIVE_AUDIO_REV(0x0001), NATIVE_AUDIO_REV(0x0001), NATIVE_AUDIO_REV(0x7FFF), NATIVE_AUDIO_REV(0x7FFF),
                                   NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0x0000),
                                   NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0x1FFF), NATIVE_AUDIO_REV(0x0FFF),
                                   NATIVE_AUDIO_REV(0x1005), NATIVE_AUDIO_REV(0x0005), NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0x0000),
                                   NATIVE_AUDIO_REV(0x1005), NATIVE_AUDIO_REV(0x0005), NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0x0000),
                                   NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0x0000),
                                   NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0x0000), NATIVE_AUDIO_REV(0x1004), NATIVE_AUDIO_REV(0x1002),
                                   NATIVE_AUDIO_REV(0x0004), NATIVE_AUDIO_REV(0x0002), NATIVE_AUDIO_REV(0x8000), NATIVE_AUDIO_REV(0x8000)}},
    {SPU_REV_MODE_PIPE, 0x3C00, {NATIVE_AUDIO_REV(0x0017), NATIVE_AUDIO_REV(0x0013), NATIVE_AUDIO_REV(0x70F0), NATIVE_AUDIO_REV(0x4FA8),
                                 NATIVE_AUDIO_REV(0xBCE0), NATIVE_AUDIO_REV(0x4510), NATIVE_AUDIO_REV(0xBEF0), NATIVE_AUDIO_REV(0x8500),
                                 NATIVE_AUDIO_REV(0x5F80), NATIVE_AUDIO_REV(0x54C0), NATIVE_AUDIO_REV(0x0371), NATIVE_AUDIO_REV(0x02AF),
                                 NATIVE_AUDIO_REV(0x02E5), NATIVE_AUDIO_REV(0x01DF), NATIVE_AUDIO_REV(0x02B0), NATIVE_AUDIO_REV(0x01D7),
                                 NATIVE_AUDIO_REV(0x0358), NATIVE_AUDIO_REV(0x026A), NATIVE_AUDIO_REV(0x01D6), NATIVE_AUDIO_REV(0x011E),
                                 NATIVE_AUDIO_REV(0x012D), NATIVE_AUDIO_REV(0x00B1), NATIVE_AUDIO_REV(0x011F), NATIVE_AUDIO_REV(0x0059),
                                 NATIVE_AUDIO_REV(0x01A0), NATIVE_AUDIO_REV(0x00E3), NATIVE_AUDIO_REV(0x0058), NATIVE_AUDIO_REV(0x0040),
                                 NATIVE_AUDIO_REV(0x0028), NATIVE_AUDIO_REV(0x0014), NATIVE_AUDIO_REV(0x8000), NATIVE_AUDIO_REV(0x8000)}},
};

#undef NATIVE_AUDIO_REV

static int NativeAudio_Clamp16(int value)
{
	if (value > 32767)
		return 32767;
	if (value < -32768)
		return -32768;
	return value;
}

static int NativeAudio_VolumeScale(s16 volume)
{
	if (volume < 0)
		return 0;
	if (volume > NATIVE_AUDIO_DIRECT_VOL_MAX)
		return NATIVE_AUDIO_DIRECT_VOL_MAX;
	return volume;
}

static int NativeAudio_ApplyVolume(int sample, s16 volume, s16 masterVolume)
{
	int scaledVolume = NativeAudio_VolumeScale(volume);
	int scaledMasterVolume = NativeAudio_VolumeScale(masterVolume);
	return (int)(((s64)sample * scaledVolume * scaledMasterVolume) / (NATIVE_AUDIO_DIRECT_VOL_MAX * NATIVE_AUDIO_DIRECT_VOL_MAX));
}

static int NativeAudio_ApplyMasterVolume(int sample, s16 masterVolume)
{
	int scaledMasterVolume = NativeAudio_VolumeScale(masterVolume);

	return (int)(((s64)sample * scaledMasterVolume) / NATIVE_AUDIO_DIRECT_VOL_MAX);
}

static const struct NativeAudioReverbPreset *NativeAudio_FindReverbPreset(int mode)
{
	int i;

	for (i = 0; i < (int)(sizeof(s_reverbPresets) / sizeof(s_reverbPresets[0])); i++)
	{
		if (s_reverbPresets[i].mode == mode)
			return &s_reverbPresets[i];
	}

	return &s_reverbPresets[0];
}

static int NativeAudio_ReverbModeFromRaw(int mode)
{
	mode &= 0xff;
	if ((mode < SPU_REV_MODE_OFF) || (mode >= SPU_REV_MODE_MAX))
		mode = SPU_REV_MODE_OFF;
	return mode;
}

static int NativeAudio_ReverbOffsetSamples(s16 reg)
{
	return (int)((u16)reg) * 4;
}

static int NativeAudio_ReverbWrapIndex(int index, int sizeSamples)
{
	if (sizeSamples <= 0)
		return 0;

	index %= sizeSamples;
	if (index < 0)
		index += sizeSamples;
	return index;
}

static int NativeAudio_ReverbRead(const struct NativeAudioReverbPreset *preset, int reg, int deltaSamples)
{
	int index;

	if (s_audio.reverb.sizeSamples <= 0)
		return 0;

	index = s_audio.reverb.cursor + NativeAudio_ReverbOffsetSamples(preset->reg[reg]) + deltaSamples;
	index = NativeAudio_ReverbWrapIndex(index, s_audio.reverb.sizeSamples);
	return s_audio.reverb.buffer[index];
}

static int NativeAudio_ReverbReadAtOffset(int offsetSamples)
{
	int index;

	if (s_audio.reverb.sizeSamples <= 0)
		return 0;

	index = NativeAudio_ReverbWrapIndex(s_audio.reverb.cursor + offsetSamples, s_audio.reverb.sizeSamples);
	return s_audio.reverb.buffer[index];
}

static void NativeAudio_ReverbWrite(const struct NativeAudioReverbPreset *preset, int reg, int value)
{
	int index;

	if (s_audio.reverb.sizeSamples <= 0)
		return;

	index = s_audio.reverb.cursor + NativeAudio_ReverbOffsetSamples(preset->reg[reg]);
	index = NativeAudio_ReverbWrapIndex(index, s_audio.reverb.sizeSamples);
	s_audio.reverb.buffer[index] = (s16)NativeAudio_Clamp16(value);
}

static int NativeAudio_ReverbMul(int sample, s16 volume)
{
	return (int)(((s64)sample * (int)volume) / 0x8000);
}

static int NativeAudio_ReverbFirApply(const s16 *history, s32 cursor)
{
	s64 sum = 0;
	int i;

	for (i = 0; i < NATIVE_AUDIO_REVERB_FIR_TAPS; i++)
	{
		int index = (cursor + i) % NATIVE_AUDIO_REVERB_FIR_TAPS;

		sum += (s64)s_reverbFirCoeffs[i] * history[index];
	}

	return NativeAudio_Clamp16((int)(sum / 0x8000));
}

static int NativeAudio_ReverbFirApplyUpsampled(const s16 *history, s32 cursor)
{
	return NativeAudio_Clamp16(NativeAudio_ReverbFirApply(history, cursor) * 2);
}

static void NativeAudio_ReverbPushInputSampleNoLock(int left, int right)
{
	s_audio.reverb.inputHistoryLeft[s_audio.reverb.inputHistoryCursor] = (s16)NativeAudio_Clamp16(left);
	s_audio.reverb.inputHistoryRight[s_audio.reverb.inputHistoryCursor] = (s16)NativeAudio_Clamp16(right);
	s_audio.reverb.inputHistoryCursor = (s_audio.reverb.inputHistoryCursor + 1) % NATIVE_AUDIO_REVERB_FIR_TAPS;
}

static void NativeAudio_ReverbPushOutputSampleNoLock(int left, int right)
{
	s_audio.reverb.outputHistoryLeft[s_audio.reverb.outputHistoryCursor] = (s16)NativeAudio_Clamp16(left);
	s_audio.reverb.outputHistoryRight[s_audio.reverb.outputHistoryCursor] = (s16)NativeAudio_Clamp16(right);
	s_audio.reverb.outputHistoryCursor = (s_audio.reverb.outputHistoryCursor + 1) % NATIVE_AUDIO_REVERB_FIR_TAPS;
}

static void NativeAudio_ReverbClearBufferNoLock(void)
{
	memset(s_audio.reverb.buffer, 0, sizeof(s_audio.reverb.buffer));
	memset(s_audio.reverb.inputHistoryLeft, 0, sizeof(s_audio.reverb.inputHistoryLeft));
	memset(s_audio.reverb.inputHistoryRight, 0, sizeof(s_audio.reverb.inputHistoryRight));
	memset(s_audio.reverb.outputHistoryLeft, 0, sizeof(s_audio.reverb.outputHistoryLeft));
	memset(s_audio.reverb.outputHistoryRight, 0, sizeof(s_audio.reverb.outputHistoryRight));
	s_audio.reverb.cursor = 0;
	s_audio.reverb.inputHistoryCursor = 0;
	s_audio.reverb.outputHistoryCursor = 0;
	s_audio.reverb.samplePhase = 0;
	s_audio.reverb.lastOutLeft = 0;
	s_audio.reverb.lastOutRight = 0;
}

static void NativeAudio_ReverbConfigureModeNoLock(int rawMode)
{
	const struct NativeAudioReverbPreset *preset;
	int mode = NativeAudio_ReverbModeFromRaw(rawMode);
	int clearWorkArea = (rawMode & SPU_REV_MODE_CLEAR_WA) != 0;
	int oldMode = s_audio.reverb.mode;
	int oldSizeSamples = s_audio.reverb.sizeSamples;

	preset = NativeAudio_FindReverbPreset(mode);
	s_audio.reverb.mode = preset->mode;
	s_audio.reverb.sizeSamples = preset->sizeBytes / (int)sizeof(s16);

	if ((s_audio.reverb.sizeSamples < 0) || (s_audio.reverb.sizeSamples > NATIVE_AUDIO_REVERB_MAX_SAMPLES))
		s_audio.reverb.sizeSamples = 0;

	if (clearWorkArea || (oldMode != s_audio.reverb.mode) || (oldSizeSamples != s_audio.reverb.sizeSamples))
		NativeAudio_ReverbClearBufferNoLock();
}

static int NativeAudio_ReverbRunReflectionStage(const struct NativeAudioReverbPreset *preset, int input, int feedbackReg, int writeReg)
{
	int previous = NativeAudio_ReverbRead(preset, writeReg, -1);
	int feedback = NativeAudio_ReverbMul(NativeAudio_ReverbRead(preset, feedbackReg, 0), preset->reg[NATIVE_AUDIO_REV_VWALL]);
	int value = NativeAudio_ReverbMul(input + feedback - previous, preset->reg[NATIVE_AUDIO_REV_VIIR]) + previous;

	return value;
}

static int NativeAudio_ReverbRunCombStage(const struct NativeAudioReverbPreset *preset, int baseReg)
{
	int value = 0;

	value += NativeAudio_ReverbMul(NativeAudio_ReverbRead(preset, baseReg, 0), preset->reg[NATIVE_AUDIO_REV_VCOMB1]);
	value += NativeAudio_ReverbMul(NativeAudio_ReverbRead(preset, baseReg + 2, 0), preset->reg[NATIVE_AUDIO_REV_VCOMB2]);
	value += NativeAudio_ReverbMul(NativeAudio_ReverbRead(preset, baseReg + 8, 0), preset->reg[NATIVE_AUDIO_REV_VCOMB3]);
	value += NativeAudio_ReverbMul(NativeAudio_ReverbRead(preset, baseReg + 10, 0), preset->reg[NATIVE_AUDIO_REV_VCOMB4]);
	return value;
}

static int NativeAudio_ReverbRunApfStage(const struct NativeAudioReverbPreset *preset, int input, int apfReg, int deltaReg, int volumeReg)
{
	int delta = NativeAudio_ReverbOffsetSamples(preset->reg[apfReg]) - NativeAudio_ReverbOffsetSamples(preset->reg[deltaReg]);
	int delayed = NativeAudio_ReverbReadAtOffset(delta);
	int stored = input - NativeAudio_ReverbMul(delayed, preset->reg[volumeReg]);

	if (s_audio.reverbEnabled)
		NativeAudio_ReverbWrite(preset, apfReg, stored);
	delayed = NativeAudio_ReverbReadAtOffset(delta);
	return NativeAudio_ReverbMul(stored, preset->reg[volumeReg]) + delayed;
}

static void NativeAudio_ReverbProcessNoLock(int sendLeft, int sendRight, int *wetLeft, int *wetRight)
{
	const struct NativeAudioReverbPreset *preset;
	int processThisFrame;
	int lin;
	int rin;
	int sameLeft;
	int sameRight;
	int diffLeft;
	int diffRight;
	int outLeft;
	int outRight;

	NativeAudio_ReverbPushInputSampleNoLock(sendLeft, sendRight);

	s_audio.reverb.samplePhase ^= 1;
	processThisFrame = s_audio.reverb.samplePhase == 0;

	if (!processThisFrame || s_audio.reverb.sizeSamples <= 0)
	{
		NativeAudio_ReverbPushOutputSampleNoLock(0, 0);
		goto output_fir;
	}

	preset = NativeAudio_FindReverbPreset(s_audio.reverb.mode);
	lin = NativeAudio_ReverbMul(NativeAudio_ReverbFirApply(s_audio.reverb.inputHistoryLeft, s_audio.reverb.inputHistoryCursor),
	                            preset->reg[NATIVE_AUDIO_REV_VLIN]);
	rin = NativeAudio_ReverbMul(NativeAudio_ReverbFirApply(s_audio.reverb.inputHistoryRight, s_audio.reverb.inputHistoryCursor),
	                            preset->reg[NATIVE_AUDIO_REV_VRIN]);

	sameLeft = NativeAudio_ReverbRunReflectionStage(preset, lin, NATIVE_AUDIO_REV_DLSAME, NATIVE_AUDIO_REV_MLSAME);
	sameRight = NativeAudio_ReverbRunReflectionStage(preset, rin, NATIVE_AUDIO_REV_DRSAME, NATIVE_AUDIO_REV_MRSAME);
	diffLeft = NativeAudio_ReverbRunReflectionStage(preset, lin, NATIVE_AUDIO_REV_DRDIFF, NATIVE_AUDIO_REV_MLDIFF);
	diffRight = NativeAudio_ReverbRunReflectionStage(preset, rin, NATIVE_AUDIO_REV_DLDIFF, NATIVE_AUDIO_REV_MRDIFF);

	if (s_audio.reverbEnabled)
	{
		NativeAudio_ReverbWrite(preset, NATIVE_AUDIO_REV_MLSAME, sameLeft);
		NativeAudio_ReverbWrite(preset, NATIVE_AUDIO_REV_MRSAME, sameRight);
		NativeAudio_ReverbWrite(preset, NATIVE_AUDIO_REV_MLDIFF, diffLeft);
		NativeAudio_ReverbWrite(preset, NATIVE_AUDIO_REV_MRDIFF, diffRight);
	}

	outLeft = NativeAudio_ReverbRunCombStage(preset, NATIVE_AUDIO_REV_MLCOMB1);
	outRight = NativeAudio_ReverbRunCombStage(preset, NATIVE_AUDIO_REV_MRCOMB1);
	outLeft = NativeAudio_ReverbRunApfStage(preset, outLeft, NATIVE_AUDIO_REV_MLAPF1, NATIVE_AUDIO_REV_DAPF1, NATIVE_AUDIO_REV_VAPF1);
	outRight = NativeAudio_ReverbRunApfStage(preset, outRight, NATIVE_AUDIO_REV_MRAPF1, NATIVE_AUDIO_REV_DAPF1, NATIVE_AUDIO_REV_VAPF1);
	outLeft = NativeAudio_ReverbRunApfStage(preset, outLeft, NATIVE_AUDIO_REV_MLAPF2, NATIVE_AUDIO_REV_DAPF2, NATIVE_AUDIO_REV_VAPF2);
	outRight = NativeAudio_ReverbRunApfStage(preset, outRight, NATIVE_AUDIO_REV_MRAPF2, NATIVE_AUDIO_REV_DAPF2, NATIVE_AUDIO_REV_VAPF2);
	NativeAudio_ReverbPushOutputSampleNoLock(outLeft, outRight);
	s_audio.reverb.cursor = NativeAudio_ReverbWrapIndex(s_audio.reverb.cursor + 1, s_audio.reverb.sizeSamples);

output_fir:
	outLeft = NativeAudio_ReverbFirApplyUpsampled(s_audio.reverb.outputHistoryLeft, s_audio.reverb.outputHistoryCursor);
	outRight = NativeAudio_ReverbFirApplyUpsampled(s_audio.reverb.outputHistoryRight, s_audio.reverb.outputHistoryCursor);
	outLeft = NativeAudio_ReverbMul(outLeft, s_audio.reverbAttr.depth.left);
	outRight = NativeAudio_ReverbMul(outRight, s_audio.reverbAttr.depth.right);
	s_audio.reverb.lastOutLeft = (s16)NativeAudio_Clamp16(outLeft);
	s_audio.reverb.lastOutRight = (s16)NativeAudio_Clamp16(outRight);
	*wetLeft = s_audio.reverb.lastOutLeft;
	*wetRight = s_audio.reverb.lastOutRight;
}

static s16 NativeAudio_GetVoicePcmSample(const struct NativeAudioVoice *voice, int sampleIndex)
{
	if (voice->looped && voice->loopEnabled && (voice->loopEnd > voice->loopStart))
	{
		int loopLen = voice->loopEnd - voice->loopStart;

		while (sampleIndex < voice->loopStart)
			sampleIndex += loopLen;
		while (sampleIndex >= voice->loopEnd)
			sampleIndex -= loopLen;
	}

	if ((sampleIndex < 0) || (sampleIndex >= voice->sampleCount))
		return 0;

	return voice->pcm[sampleIndex];
}

static int NativeAudio_InterpolateVoiceSample(const struct NativeAudioVoice *voice)
{
	int sampleIndex = (int)(voice->positionFp >> NATIVE_AUDIO_FP_SHIFT);
	int gaussIndex = (int)((voice->positionFp >> NATIVE_AUDIO_GAUSS_INDEX_SHIFT) & 0xff);
	int oldest = NativeAudio_GetVoicePcmSample(voice, sampleIndex - 3);
	int older = NativeAudio_GetVoicePcmSample(voice, sampleIndex - 2);
	int old = NativeAudio_GetVoicePcmSample(voice, sampleIndex - 1);
	int newest = NativeAudio_GetVoicePcmSample(voice, sampleIndex);
	int sample;

	sample = (s_gaussTable[0xff - gaussIndex] * oldest) >> 15;
	sample += (s_gaussTable[0x1ff - gaussIndex] * older) >> 15;
	sample += (s_gaussTable[0x100 + gaussIndex] * old) >> 15;
	sample += (s_gaussTable[gaussIndex] * newest) >> 15;

	return NativeAudio_Clamp16(sample);
}

static u64 NativeAudio_GetXAOutputFrameCount(int frameCount, int sampleRate)
{
	if ((frameCount <= 0) || (sampleRate <= 0))
		return 0;

	if (sampleRate == XA_SAMPLE_RATE_37800)
		return (((u64)frameCount * NATIVE_AUDIO_XA_ZIGZAG_PHASES) + (NATIVE_AUDIO_XA_ZIGZAG_INPUTS - 1)) / NATIVE_AUDIO_XA_ZIGZAG_INPUTS;
	if (sampleRate == XA_SAMPLE_RATE_18900)
		return (((u64)frameCount * NATIVE_AUDIO_XA_ZIGZAG_PHASES) + ((NATIVE_AUDIO_XA_ZIGZAG_INPUTS / 2) - 1)) / (NATIVE_AUDIO_XA_ZIGZAG_INPUTS / 2);

	return (((u64)frameCount * NATIVE_AUDIO_SAMPLE_RATE) + ((u64)sampleRate - 1)) / (u64)sampleRate;
}

static void NativeAudio_UpdateXAPositionFromOutputFrameNoLock(void)
{
	if (s_audio.xa.sampleRate <= 0)
	{
		s_audio.xa.positionFp = 0;
		return;
	}

	s_audio.xa.positionFp = ((s_audio.xa.outputFrame * (u64)s_audio.xa.sampleRate) << NATIVE_AUDIO_FP_SHIFT) / NATIVE_AUDIO_SAMPLE_RATE;
}

static void NativeAudio_AdvanceXAOutputFrameNoLock(void)
{
	u64 outputFrameCount;

	s_audio.xa.outputFrame++;
	outputFrameCount = NativeAudio_GetXAOutputFrameCount(s_audio.xa.frameCount, s_audio.xa.sampleRate);
	if ((outputFrameCount > 0) && (s_audio.xa.outputFrame >= outputFrameCount))
	{
		s_audio.xa.outputFrame = outputFrameCount;
		s_audio.xa.active = 0;
	}
	NativeAudio_UpdateXAPositionFromOutputFrameNoLock();
}

static int NativeAudio_GetXAPcmSampleAtFrameNoLock(int channel, u64 frameIndex)
{
	if ((s_audio.xa.pcm == NULL) || (frameIndex >= (u64)s_audio.xa.frameCount))
		return 0;

	return s_audio.xa.pcm[(size_t)frameIndex * NATIVE_AUDIO_CHANNELS + (size_t)channel];
}

static int NativeAudio_GetXAPseudo37800SampleNoLock(int channel, s64 pseudoFrameIndex)
{
	u64 frameIndex;

	if (pseudoFrameIndex < 0)
		return 0;

	frameIndex = (u64)pseudoFrameIndex;
	// NOTE(aalhendi): 18.9kHz XA feeds the same 37.8kHz->44.1kHz zig-zag path with each decoded source frame held twice.
	if (s_audio.xa.sampleRate == XA_SAMPLE_RATE_18900)
		frameIndex >>= 1;

	return NativeAudio_GetXAPcmSampleAtFrameNoLock(channel, frameIndex);
}

static int NativeAudio_ZigZagInterpolateXASampleNoLock(int channel)
{
	u64 group = s_audio.xa.outputFrame / NATIVE_AUDIO_XA_ZIGZAG_PHASES;
	int phase = (int)(s_audio.xa.outputFrame % NATIVE_AUDIO_XA_ZIGZAG_PHASES);
	u64 p = (group + 1) * NATIVE_AUDIO_XA_ZIGZAG_INPUTS;
	int sum = 0;
	int tap;

	for (tap = 0; tap < NATIVE_AUDIO_XA_ZIGZAG_TAPS; tap++)
	{
		int sample = NativeAudio_GetXAPseudo37800SampleNoLock(channel, (s64)p - (s64)(tap + 1));
		sum += (int)(((s64)sample * s_xaZigZagTable[tap][phase]) >> 15);
	}

	return NativeAudio_Clamp16(sum);
}

static int NativeAudio_InterpolateXALinearSampleNoLock(int channel)
{
	u64 frameIndex = s_audio.xa.positionFp >> NATIVE_AUDIO_FP_SHIFT;
	u32 frac = (u32)(s_audio.xa.positionFp & (NATIVE_AUDIO_FP_ONE - 1));
	int a = NativeAudio_GetXAPcmSampleAtFrameNoLock(channel, frameIndex);
	int b = a;

	if (frameIndex + 1 < (u64)s_audio.xa.frameCount)
		b = NativeAudio_GetXAPcmSampleAtFrameNoLock(channel, frameIndex + 1);

	return a + (int)(((s64)(b - a) * frac) >> NATIVE_AUDIO_FP_SHIFT);
}

static int NativeAudio_GetXAMixSampleNoLock(int channel)
{
	if ((s_audio.xa.sampleRate == XA_SAMPLE_RATE_37800) || (s_audio.xa.sampleRate == XA_SAMPLE_RATE_18900))
		return NativeAudio_ZigZagInterpolateXASampleNoLock(channel);

	return NativeAudio_InterpolateXALinearSampleNoLock(channel);
}

// NOTE(aalhendi): PS1 SPU ADSR envelope behavior follows PSX-SPX.
// Source reference: https://psx-spx.consoledev.net/soundprocessingunitspu/
static int NativeAudio_ClampAdsrLevel(int level)
{
	if (level < NATIVE_AUDIO_ADSR_MIN)
		return NATIVE_AUDIO_ADSR_MIN;
	if (level > NATIVE_AUDIO_ADSR_MAX)
		return NATIVE_AUDIO_ADSR_MAX;
	return level;
}

static b32 NativeAudio_AdsrModeIsExponential(int mode)
{
	return mode == SPU_VOICE_EXPIncN || mode == SPU_VOICE_EXPIncR || mode == SPU_VOICE_EXPDec;
}

static b32 NativeAudio_AdsrModeIsDecreasing(int mode)
{
	return mode == SPU_VOICE_LINEARDecN || mode == SPU_VOICE_LINEARDecR || mode == SPU_VOICE_EXPDec;
}

static b32 NativeAudio_AdsrModeIsPhaseNegative(int mode)
{
	return mode == SPU_VOICE_LINEARIncR || mode == SPU_VOICE_LINEARDecR || mode == SPU_VOICE_EXPIncR;
}

static int NativeAudio_AdsrSustainTarget(const struct NativeAudioVoice *voice)
{
	int target = ((int)voice->attr.sl + 1) * 0x800;

	if (target > NATIVE_AUDIO_ADSR_MAX)
		target = NATIVE_AUDIO_ADSR_MAX;
	return target;
}

static void NativeAudio_AdsrSetPhase(struct NativeAudioVoice *voice, int phase)
{
	voice->adsrPhase = (u8)phase;
	voice->adsrCounter = 0;
}

static void NativeAudio_AdsrForceOff(struct NativeAudioVoice *voice)
{
	voice->adsrLevel = 0;
	voice->adsrCounter = 0;
	voice->adsrPhase = NATIVE_AUDIO_ADSR_OFF;
	voice->attr.envx = 0;
	voice->active = 0;
}

static void NativeAudio_AdsrKeyOn(struct NativeAudioVoice *voice)
{
	voice->adsrLevel = 0;
	voice->adsrCounter = 0;
	voice->adsrPhase = NATIVE_AUDIO_ADSR_ATTACK;
	voice->attr.envx = 0;
}

static void NativeAudio_AdsrKeyOff(struct NativeAudioVoice *voice)
{
	if (voice->adsrPhase != NATIVE_AUDIO_ADSR_OFF)
		NativeAudio_AdsrSetPhase(voice, NATIVE_AUDIO_ADSR_RELEASE);
	else
		NativeAudio_AdsrForceOff(voice);
}

static b32 NativeAudio_AdsrRateIsAllOnes(int shiftValue, int stepValue, int bitCount)
{
	int mask = (1 << bitCount) - 1;

	return (stepValue | (shiftValue << 2)) == mask;
}

static void NativeAudio_AdsrRunEnvelopeStep(struct NativeAudioVoice *voice, int shiftValue, int stepValue, b32 exponential, b32 decreasing, b32 phaseNegative,
                                            b32 rateAllOnes)
{
	int adsrStep;
	u32 counterIncrement;
	u32 counter;

	if (rateAllOnes)
		return;

	adsrStep = 7 - stepValue;
	if (decreasing != phaseNegative)
		adsrStep = ~adsrStep;

	if (shiftValue < 11)
		adsrStep <<= 11 - shiftValue;

	counterIncrement = NATIVE_AUDIO_ADSR_STEP_BIT;
	if (shiftValue > 11)
	{
		int shift = shiftValue - 11;
		counterIncrement = shift < 31 ? (NATIVE_AUDIO_ADSR_STEP_BIT >> shift) : 0;
	}

	if (exponential && !decreasing && voice->adsrLevel > 0x6000)
	{
		if (shiftValue < 10)
		{
			adsrStep >>= 2;
		}
		else if (shiftValue >= 11)
		{
			counterIncrement >>= 2;
		}
		else
		{
			adsrStep >>= 1;
			counterIncrement >>= 1;
		}
	}
	else if (exponential && decreasing)
	{
		adsrStep = (int)(((s64)adsrStep * voice->adsrLevel) / 0x8000);
		if (voice->adsrPhase == NATIVE_AUDIO_ADSR_RELEASE && adsrStep == 0 && voice->adsrLevel > 0)
			adsrStep = -1;
	}

	if (counterIncrement == 0)
		counterIncrement = 1;

	counter = voice->adsrCounter + counterIncrement;
	voice->adsrCounter = counter & (NATIVE_AUDIO_ADSR_STEP_BIT - 1);
	if ((counter & NATIVE_AUDIO_ADSR_STEP_BIT) == 0)
		return;

	voice->adsrLevel += adsrStep;
	if (!decreasing)
	{
		voice->adsrLevel = NativeAudio_ClampAdsrLevel(voice->adsrLevel);
	}
	else if (phaseNegative)
	{
		if (voice->adsrLevel < NATIVE_AUDIO_ADSR_MIN)
			voice->adsrLevel = NATIVE_AUDIO_ADSR_MIN;
		if (voice->adsrLevel > 0)
			voice->adsrLevel = 0;
	}
	else if (voice->adsrLevel < 0)
	{
		voice->adsrLevel = 0;
	}

	voice->attr.envx = (s16)voice->adsrLevel;
}

static void NativeAudio_AdsrAdvance(struct NativeAudioVoice *voice)
{
	switch (voice->adsrPhase)
	{
	case NATIVE_AUDIO_ADSR_ATTACK:
	{
		int shift = (voice->attr.ar >> 2) & 0x1f;
		int step = voice->attr.ar & 3;

		NativeAudio_AdsrRunEnvelopeStep(voice, shift, step, NativeAudio_AdsrModeIsExponential(voice->attr.a_mode), 0,
		                                NativeAudio_AdsrModeIsPhaseNegative(voice->attr.a_mode), NativeAudio_AdsrRateIsAllOnes(shift, step, 7));
		if (voice->adsrLevel >= NATIVE_AUDIO_ADSR_MAX)
		{
			voice->adsrLevel = NATIVE_AUDIO_ADSR_MAX;
			voice->attr.envx = NATIVE_AUDIO_ADSR_MAX;
			NativeAudio_AdsrSetPhase(voice, NATIVE_AUDIO_ADSR_DECAY);
		}
		break;
	}

	case NATIVE_AUDIO_ADSR_DECAY:
	{
		int target = NativeAudio_AdsrSustainTarget(voice);

		if (voice->adsrLevel <= target)
		{
			voice->adsrLevel = target;
			voice->attr.envx = (s16)target;
			NativeAudio_AdsrSetPhase(voice, NATIVE_AUDIO_ADSR_SUSTAIN);
			break;
		}

		NativeAudio_AdsrRunEnvelopeStep(voice, voice->attr.dr & 0xf, 0, 1, 1, 0, 0);
		if (voice->adsrLevel <= target)
		{
			voice->adsrLevel = target;
			voice->attr.envx = (s16)target;
			NativeAudio_AdsrSetPhase(voice, NATIVE_AUDIO_ADSR_SUSTAIN);
		}
		break;
	}

	case NATIVE_AUDIO_ADSR_SUSTAIN:
	{
		int shift = (voice->attr.sr >> 2) & 0x1f;
		int step = voice->attr.sr & 3;

		NativeAudio_AdsrRunEnvelopeStep(voice, shift, step, NativeAudio_AdsrModeIsExponential(voice->attr.s_mode),
		                                NativeAudio_AdsrModeIsDecreasing(voice->attr.s_mode), NativeAudio_AdsrModeIsPhaseNegative(voice->attr.s_mode),
		                                NativeAudio_AdsrRateIsAllOnes(shift, step, 7));
		break;
	}

	case NATIVE_AUDIO_ADSR_RELEASE:
	{
		int shift = voice->attr.rr & 0x1f;

		if (voice->adsrLevel <= 0)
		{
			NativeAudio_AdsrForceOff(voice);
			break;
		}

		NativeAudio_AdsrRunEnvelopeStep(voice, shift, 0, NativeAudio_AdsrModeIsExponential(voice->attr.r_mode), 1,
		                                NativeAudio_AdsrModeIsPhaseNegative(voice->attr.r_mode), shift == 0x1f);
		if (voice->adsrLevel <= 0)
			NativeAudio_AdsrForceOff(voice);
		break;
	}

	default:
		NativeAudio_AdsrForceOff(voice);
		break;
	}
}

static int NativeAudio_ApplyAdsrEnvelope(int sample, int adsrLevel)
{
	int scaleMax;

	if (adsrLevel == 0)
		return 0;
	if (adsrLevel >= NATIVE_AUDIO_ADSR_MAX)
		return sample;

	scaleMax = adsrLevel < 0 ? 0x8000 : NATIVE_AUDIO_ADSR_MAX;
	return NativeAudio_Clamp16((int)(((s64)sample * adsrLevel) / scaleMax));
}

static void NativeAudio_UpdatePackedAdsrFromFields(struct NativeAudioVoice *voice)
{
	voice->attr.adsr1 = (u16)((voice->attr.sl & 0xf) | ((voice->attr.dr & 0xf) << 4) | ((voice->attr.ar & 0x7f) << 8) |
	                          (NativeAudio_AdsrModeIsExponential(voice->attr.a_mode) ? 0x8000 : 0));
	voice->attr.adsr2 =
	    (u16)((voice->attr.rr & 0x1f) | ((voice->attr.sr & 0x7f) << 6) | (NativeAudio_AdsrModeIsDecreasing(voice->attr.s_mode) ? 0x4000 : 0) |
	          (NativeAudio_AdsrModeIsExponential(voice->attr.s_mode) ? 0x8000 : 0) | (NativeAudio_AdsrModeIsExponential(voice->attr.r_mode) ? 0x20 : 0));
}

static void NativeAudio_DecodePackedAdsrToFields(struct NativeAudioVoice *voice, b32 decodeAdsr1, b32 decodeAdsr2)
{
	if (decodeAdsr1)
	{
		voice->attr.sl = voice->attr.adsr1 & 0xf;
		voice->attr.dr = (voice->attr.adsr1 >> 4) & 0xf;
		voice->attr.ar = (voice->attr.adsr1 >> 8) & 0x7f;
		voice->attr.a_mode = (voice->attr.adsr1 & 0x8000) != 0 ? SPU_VOICE_EXPIncN : SPU_VOICE_LINEARIncN;
	}

	if (decodeAdsr2)
	{
		b32 sustainDecreasing = (voice->attr.adsr2 & 0x4000) != 0;
		b32 sustainExponential = (voice->attr.adsr2 & 0x8000) != 0;

		voice->attr.rr = voice->attr.adsr2 & 0x1f;
		voice->attr.r_mode = (voice->attr.adsr2 & 0x20) != 0 ? SPU_VOICE_EXPDec : SPU_VOICE_LINEARDecN;
		voice->attr.sr = (voice->attr.adsr2 >> 6) & 0x7f;
		if (sustainExponential)
			voice->attr.s_mode = sustainDecreasing ? SPU_VOICE_EXPDec : SPU_VOICE_EXPIncN;
		else
			voice->attr.s_mode = sustainDecreasing ? SPU_VOICE_LINEARDecN : SPU_VOICE_LINEARIncN;
	}
}

static int NativeAudio_ReadLE32(const u8 *bytes)
{
	return (int)((u32)bytes[0] | ((u32)bytes[1] << 8) | ((u32)bytes[2] << 16) | ((u32)bytes[3] << 24));
}

static int NativeAudio_ReadLE16Signed(const u8 *bytes)
{
	return (s16)((u16)bytes[0] | ((u16)bytes[1] << 8));
}

static int NativeAudio_AlignUpInt(int value, int align)
{
	int mask = align - 1;

	if ((value < 0) || (align <= 0) || ((align & mask) != 0) || (value > INT_MAX - mask))
		return -1;

	return (value + mask) & ~mask;
}

static void NativeAudio_ArenaFree(struct NativeAudioDecodeArena *arena)
{
	free(arena->memory);
	arena->memory = NULL;
	arena->capacity = 0;
	arena->used = 0;
}

static void NativeAudio_ArenaReset(struct NativeAudioDecodeArena *arena)
{
	arena->used = 0;
}

static int NativeAudio_ArenaEnsureCapacity(struct NativeAudioDecodeArena *arena, int capacity)
{
	u8 *newMemory;
	int newCapacity;

	if (capacity < 0)
		return 0;
	if (capacity <= arena->capacity)
		return 1;

	newCapacity = arena->capacity > 0 ? arena->capacity : (256 * 1024);
	while (newCapacity < capacity)
	{
		if (newCapacity > (INT_MAX / 2))
		{
			newCapacity = capacity;
			break;
		}
		newCapacity *= 2;
	}

	newMemory = (u8 *)malloc((size_t)newCapacity);
	if (newMemory == NULL)
		return 0;

	free(arena->memory);
	arena->memory = newMemory;
	arena->capacity = newCapacity;
	arena->used = 0;
	return 1;
}

static void *NativeAudio_ArenaPush(struct NativeAudioDecodeArena *arena, int size, int align, int *markerOut)
{
	int marker;
	int end;

	if (size < 0)
		return NULL;

	marker = NativeAudio_AlignUpInt(arena->used, align);
	if ((marker < 0) || (size > INT_MAX - marker))
		return NULL;

	end = marker + size;
	if (end > arena->capacity)
		return NULL;

	if (markerOut != NULL)
		*markerOut = marker;
	arena->used = end;
	return &arena->memory[marker];
}

static void NativeAudio_ArenaRewind(struct NativeAudioDecodeArena *arena, int marker)
{
	if ((marker >= 0) && (marker <= arena->used))
		arena->used = marker;
}

static void NativeAudio_ArenaSwap(struct NativeAudioDecodeArena *a, struct NativeAudioDecodeArena *b)
{
	struct NativeAudioDecodeArena tmp = *a;

	*a = *b;
	*b = tmp;
}

static int NativeAudio_VoiceArenaEnsureCapacityNoLock(int capacity)
{
	struct NativeAudioDecodeArena *arena = &s_audio.voicePcmArena;
	u8 *oldMemory;
	u8 *newMemory;
	int newCapacity;
	int i;

	if (capacity < 0)
		return 0;
	if (capacity <= arena->capacity)
		return 1;

	newCapacity = arena->capacity > 0 ? arena->capacity : (256 * 1024);
	while (newCapacity < capacity)
	{
		if (newCapacity > (INT_MAX / 2))
		{
			newCapacity = capacity;
			break;
		}
		newCapacity *= 2;
	}

	oldMemory = arena->memory;
	newMemory = (u8 *)malloc((size_t)newCapacity);
	if (newMemory == NULL)
		return 0;

	if ((oldMemory != NULL) && (arena->used > 0))
		memcpy(newMemory, oldMemory, (size_t)arena->used);

	for (i = 0; i < NATIVE_AUDIO_SPU_VOICE_COUNT; i++)
	{
		if ((s_audio.voices[i].pcm != NULL) && (oldMemory != NULL))
		{
			u8 *pcm = (u8 *)s_audio.voices[i].pcm;
			if ((pcm >= oldMemory) && (pcm < oldMemory + arena->used))
				s_audio.voices[i].pcm = (s16 *)(newMemory + (pcm - oldMemory));
		}
	}

	free(oldMemory);
	arena->memory = newMemory;
	arena->capacity = newCapacity;
	return 1;
}

static int NativeAudio_ReadFileBytes(const char *path, struct NativeAudioByteBuffer *bytes)
{
	FILE *fp;
	long size;
	size_t read;

	bytes->data = NULL;
	bytes->size = 0;

	fp = fopen(path, "rb");
	if (fp == NULL)
		return 0;

	if (fseek(fp, 0, SEEK_END) != 0)
	{
		fclose(fp);
		return 0;
	}

	size = ftell(fp);
	if ((size <= 0) || (size > INT_MAX))
	{
		fclose(fp);
		return 0;
	}

	if (fseek(fp, 0, SEEK_SET) != 0)
	{
		fclose(fp);
		return 0;
	}

	bytes->data = (u8 *)malloc((size_t)size);
	if (bytes->data == NULL)
	{
		fclose(fp);
		return 0;
	}

	read = fread(bytes->data, 1, (size_t)size, fp);
	fclose(fp);

	if (read != (size_t)size)
	{
		free(bytes->data);
		bytes->data = NULL;
		return 0;
	}

	bytes->size = (int)size;
	return 1;
}

static void NativeAudio_FreeByteBuffer(struct NativeAudioByteBuffer *bytes)
{
	free(bytes->data);
	bytes->data = NULL;
	bytes->size = 0;
}

static int NativeAudio_PcmReserve(struct NativeAudioPcmBuffer *pcm, int extra)
{
	int target;

	if ((pcm == NULL) || (extra <= 0) || (pcm->count < 0) || (pcm->capacity < 0) || (extra > INT_MAX - pcm->count))
		return 0;

	target = pcm->count + extra;
	return target <= pcm->capacity;
}

static int NativeAudio_PcmPush(struct NativeAudioPcmBuffer *pcm, s16 sample)
{
	if (!NativeAudio_PcmReserve(pcm, 1))
		return 0;

	pcm->samples[pcm->count++] = sample;
	return 1;
}

static int NativeAudio_DecodeAdpcmNibble(u8 soundParameter, int nibble, int *old, int *older)
{
	int shift = soundParameter & 0xf;
	int weight = (soundParameter >> 4) & 0xf;
	int sample;

	if (weight > 4)
		weight = 4;

	if (nibble > 7)
		nibble -= 16;

	sample = (nibble << 12);
	sample >>= shift;
	sample += (*old * s_posTable[weight]) >> 6;
	sample += (*older * s_negTable[weight]) >> 6;
	sample = NativeAudio_Clamp16(sample);

	*older = *old;
	*old = sample;
	return sample;
}

static int NativeAudio_DecodeSpuSample(u32 addr, u32 loopAddr, struct NativeAudioPcmBuffer *pcm, int *loopStart, int *loopEnd, int *loopEnabled)
{
	int old = 0;
	int older = 0;
	int block;
	int loopStartSample = 0;
	int loopEndSample = 0;
	int hasLoopStart = 0;
	int hasLoopEnd = 0;
	int hasRepeat = 0;

	if (addr >= NATIVE_AUDIO_SPU_MEMSIZE)
		return 0;

	for (block = (int)addr; block + 16 <= NATIVE_AUDIO_SPU_MEMSIZE; block += 16)
	{
		u8 soundParameter = s_audio.spu.memory[block];
		u8 flags = s_audio.spu.memory[block + 1];
		int i;

		if ((loopAddr > addr) && (loopAddr == (u32)block))
		{
			loopStartSample = pcm->count;
			hasLoopStart = 1;
		}
		else if ((flags & ADPCM_LOOP_START) != 0)
		{
			loopStartSample = pcm->count;
			hasLoopStart = 1;
		}

		for (i = 0; i < 14; i++)
		{
			u8 packed = s_audio.spu.memory[block + 2 + i];
			if (!NativeAudio_PcmPush(pcm, (s16)NativeAudio_DecodeAdpcmNibble(soundParameter, packed & 0xf, &old, &older)))
				return 0;
			if (!NativeAudio_PcmPush(pcm, (s16)NativeAudio_DecodeAdpcmNibble(soundParameter, (packed >> 4) & 0xf, &old, &older)))
				return 0;
		}

		if ((flags & ADPCM_LOOP_END) != 0)
		{
			loopEndSample = pcm->count;
			hasLoopEnd = 1;
			hasRepeat = (flags & ADPCM_REPEAT) != 0;
			break;
		}
	}

	*loopStart = hasLoopStart ? loopStartSample : 0;
	*loopEnd = hasLoopEnd ? loopEndSample : pcm->count;
	*loopEnabled = (hasLoopEnd && hasRepeat && (*loopEnd > *loopStart));
	return pcm->count > 0;
}

static void NativeAudio_FreeVoicePcm(struct NativeAudioVoice *voice)
{
	voice->pcm = NULL;
	voice->sampleCount = 0;
	voice->loopStart = 0;
	voice->loopEnd = 0;
	voice->loopEnabled = 0;
	voice->looped = 0;
}

static void NativeAudio_ResetVoicePcmArenaNoLock(int markDirty)
{
	int i;

	NativeAudio_ArenaReset(&s_audio.voicePcmArena);
	for (i = 0; i < NATIVE_AUDIO_SPU_VOICE_COUNT; i++)
	{
		NativeAudio_FreeVoicePcm(&s_audio.voices[i]);
		if (markDirty && s_audio.voices[i].active)
			s_audio.voices[i].sampleDirty = 1;
	}
}

static int NativeAudio_PrepareVoicePcmBuffer(u32 addr, struct NativeAudioPcmBuffer *pcm, int *markerOut)
{
	int maxSamples;
	int maxBytes;
	int marker;

	if (addr >= NATIVE_AUDIO_SPU_MEMSIZE)
		return 0;

	maxSamples = ((NATIVE_AUDIO_SPU_MEMSIZE - (int)addr) / 16) * XA_SAMPLES_PER_SOUND_UNIT;
	if ((maxSamples <= 0) || (maxSamples > (INT_MAX / (int)sizeof(s16))))
		return 0;

	maxBytes = maxSamples * (int)sizeof(s16);
	marker = NativeAudio_AlignUpInt(s_audio.voicePcmArena.used, NATIVE_AUDIO_ARENA_ALIGN);
	if ((marker < 0) || (maxBytes > INT_MAX - marker))
		return 0;

	if (!NativeAudio_VoiceArenaEnsureCapacityNoLock(marker + maxBytes))
		return 0;

	pcm->samples = (s16 *)NativeAudio_ArenaPush(&s_audio.voicePcmArena, maxBytes, NATIVE_AUDIO_ARENA_ALIGN, markerOut);
	if (pcm->samples == NULL)
		return 0;

	pcm->count = 0;
	pcm->capacity = maxSamples;
	return 1;
}

static void NativeAudio_MarkVoiceSamplesDirty(void)
{
	int i;

	for (i = 0; i < NATIVE_AUDIO_SPU_VOICE_COUNT; i++)
		s_audio.voices[i].sampleDirty = 1;
}

static void NativeAudio_WrapVoiceLoop(struct NativeAudioVoice *voice)
{
	u64 loopStartFp = (u64)voice->loopStart << NATIVE_AUDIO_FP_SHIFT;
	u64 loopEndFp = (u64)voice->loopEnd << NATIVE_AUDIO_FP_SHIFT;
	u64 loopLenFp = loopEndFp - loopStartFp;
	u64 overshoot = voice->positionFp - loopEndFp;

	voice->positionFp = loopStartFp + (loopLenFp != 0 ? overshoot % loopLenFp : 0);
	voice->looped = 1;
}

static int NativeAudio_UpdateVoiceSample(struct NativeAudioVoice *voice)
{
	struct NativeAudioPcmBuffer pcm;
	int loopStart;
	int loopEnd;
	b32 loopEnabled;
	int arenaMarker = 0;

	memset(&pcm, 0, sizeof(pcm));
	if (!NativeAudio_PrepareVoicePcmBuffer(voice->attr.addr, &pcm, &arenaMarker))
	{
		NativeAudio_FreeVoicePcm(voice);
		voice->active = 0;
		voice->sampleDirty = 0;
		return 0;
	}

	if (!NativeAudio_DecodeSpuSample(voice->attr.addr, voice->attr.loop_addr, &pcm, &loopStart, &loopEnd, &loopEnabled))
	{
		NativeAudio_ArenaRewind(&s_audio.voicePcmArena, arenaMarker);
		NativeAudio_FreeVoicePcm(voice);
		voice->active = 0;
		voice->sampleDirty = 0;
		return 0;
	}

	NativeAudio_ArenaRewind(&s_audio.voicePcmArena, arenaMarker + pcm.count * (int)sizeof(s16));
	NativeAudio_FreeVoicePcm(voice);
	voice->pcm = pcm.samples;
	voice->sampleCount = pcm.count;
	voice->loopStart = loopStart;
	voice->loopEnd = loopEnd;
	voice->loopEnabled = loopEnabled;
	voice->sampleDirty = 0;
	return 1;
}

static void NativeAudio_FreeXA(void)
{
	memset(&s_audio.xa, 0, sizeof(s_audio.xa));
	NativeAudio_ArenaReset(&s_audio.xaPcmArena);
}

static void NativeAudio_CopyVoiceToState(struct NativeAudioVoiceState *dst, const struct NativeAudioVoice *src)
{
	dst->attr = src->attr;
	dst->sampleCount = src->sampleCount;
	dst->loopStart = src->loopStart;
	dst->loopEnd = src->loopEnd;
	dst->loopEnabled = src->loopEnabled;
	dst->active = src->active;
	dst->positionFp = src->positionFp;
	dst->looped = src->looped;
	dst->reverb = src->reverb;
	dst->sampleDirty = src->sampleDirty;
	dst->adsrLevel = src->adsrLevel;
	dst->adsrCounter = src->adsrCounter;
	dst->adsrPhase = src->adsrPhase;
}

static void NativeAudio_CopyStateToVoice(struct NativeAudioVoice *dst, const struct NativeAudioVoiceState *src)
{
	dst->attr = src->attr;
	dst->sampleCount = 0;
	dst->loopStart = 0;
	dst->loopEnd = 0;
	dst->loopEnabled = 0;
	dst->active = src->active;
	dst->positionFp = src->positionFp;
	dst->looped = src->looped;
	dst->reverb = src->reverb;
	dst->sampleDirty = src->sampleDirty || src->active;
	dst->adsrLevel = src->adsrLevel;
	dst->adsrCounter = src->adsrCounter;
	dst->adsrPhase = src->adsrPhase;
	dst->pcm = NULL;
}

static void NativeAudio_CopyXAToState(struct NativeAudioXAState *dst, const struct NativeAudioXA *src)
{
	dst->frameCount = src->frameCount;
	dst->sampleRate = src->sampleRate;
	dst->categoryID = src->categoryID;
	dst->xaID = src->xaID;
	dst->hasTrackIdentity = src->hasTrackIdentity;
	dst->active = src->active;
	dst->positionFp = src->positionFp;
	dst->outputFrame = src->outputFrame;
	dst->stepFp = src->stepFp;
	dst->volumeLeft = src->volumeLeft;
	dst->volumeRight = src->volumeRight;
}

static int NativeAudio_ValidateVoiceSnapshot(const struct NativeAudioVoiceState *voice)
{
	if (voice->active && (voice->attr.addr >= NATIVE_AUDIO_SPU_MEMSIZE))
		return 0;
	if (voice->attr.loop_addr >= NATIVE_AUDIO_SPU_MEMSIZE)
		return 0;
	if (voice->adsrPhase > NATIVE_AUDIO_ADSR_RELEASE)
		return 0;
	if ((voice->adsrLevel < NATIVE_AUDIO_ADSR_MIN) || (voice->adsrLevel > NATIVE_AUDIO_ADSR_MAX))
		return 0;
	return 1;
}

static int NativeAudio_ValidateXASnapshot(const struct NativeAudioXAState *xa)
{
	u64 outputFrameCount;

	if ((xa->active != 0) && (xa->active != 1))
		return 0;
	if ((xa->hasTrackIdentity != 0) && (xa->hasTrackIdentity != 1))
		return 0;
	if ((xa->frameCount < 0) || (xa->sampleRate < 0))
		return 0;
	if (!xa->active && !xa->hasTrackIdentity)
		return 1;
	if ((xa->categoryID < 0) || (xa->categoryID >= XA_NUM_TYPES))
		return 0;
	if (xa->xaID < 0)
		return 0;
	if (xa->active && ((xa->hasTrackIdentity == 0) || (xa->frameCount <= 0) || (xa->sampleRate <= 0)))
		return 0;
	outputFrameCount = NativeAudio_GetXAOutputFrameCount(xa->frameCount, xa->sampleRate);
	if (xa->active && ((outputFrameCount == 0) || (xa->outputFrame >= outputFrameCount)))
		return 0;
	if (xa->active && ((xa->positionFp >> NATIVE_AUDIO_FP_SHIFT) >= (u64)xa->frameCount))
		return 0;
	return 1;
}

static int NativeAudio_ValidateReverbSnapshot(const struct NativeAudioReverbState *reverb)
{
	if ((reverb->mode < SPU_REV_MODE_OFF) || (reverb->mode >= SPU_REV_MODE_MAX))
		return 0;
	if ((reverb->sizeSamples < 0) || (reverb->sizeSamples > NATIVE_AUDIO_REVERB_MAX_SAMPLES))
		return 0;
	if ((reverb->inputHistoryCursor < 0) || (reverb->inputHistoryCursor >= NATIVE_AUDIO_REVERB_FIR_TAPS))
		return 0;
	if ((reverb->outputHistoryCursor < 0) || (reverb->outputHistoryCursor >= NATIVE_AUDIO_REVERB_FIR_TAPS))
		return 0;
	if ((reverb->samplePhase < 0) || (reverb->samplePhase > 1))
		return 0;
	if (reverb->sizeSamples == 0)
		return reverb->cursor == 0;
	if ((reverb->cursor < 0) || (reverb->cursor >= reverb->sizeSamples))
		return 0;
	return 1;
}

static void NativeAudio_ClearOutputQueueNoLock(void)
{
	s_audio.output.scheduledReadFrame = 0;
	s_audio.output.scheduledFrameCount = 0;

	if (s_audio.output.stream != NULL)
		SDL_ClearAudioStream(s_audio.output.stream);
}

static int NativeAudio_OpenDevice(void);
static void NativeAudio_MixFrame(s16 *outLeft, s16 *outRight);
static int NativeAudio_RenderFramesNoLock(s16 *out, int frameCount);

static void NativeAudio_SelectDriverHint(void)
{
#if defined(__linux__)
	if (SDL_GetHint(SDL_HINT_AUDIO_DRIVER) == NULL)
	{
		// NOTE(aalhendi): Keep native Linux playback on the SDL3 drivers that
		// were stable in live probes. User/env overrides still win, and
		// PipeWire remains available as a fallback.
		SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "pulseaudio,alsa,pipewire");
	}
#endif
	if (SDL_GetHint(SDL_HINT_AUDIO_DEVICE_SAMPLE_FRAMES) == NULL)
		SDL_SetHint(SDL_HINT_AUDIO_DEVICE_SAMPLE_FRAMES, "1024");
}

static int NativeAudio_BuildXAPath(char *path, size_t pathSize, int categoryID, int fileNumber)
{
	const char *dir = NULL;
	int written;

	if (categoryID == 0)
		dir = "assets/XA/MUSIC";
	else if (categoryID == 1)
		dir = "assets/XA/ENG/EXTRA";
	else if (categoryID == 2)
		dir = "assets/XA/ENG/GAME";
	else
		return 0;

	written = snprintf(path, pathSize, "%s/S%02d.XA", dir, fileNumber);
	return (written > 0) && ((size_t)written < pathSize);
}

static int NativeAudio_LookupXATrackInfo(int categoryID, int xaID, struct NativeAudioXaTrackInfo *info)
{
	struct NativeAudioByteBuffer xnf;
	int numXasTotal;
	int numTracksTotal;
	int xaSizeOffset;
	int xaSizeEnd;
	int numSongs;
	int firstSongIndex;
	int entryIndex;
	const u8 *entry;

	if ((categoryID < 0) || (categoryID >= XA_NUM_TYPES) || (xaID < 0))
		return 0;

	if (!NativeAudio_ReadFileBytes("assets/XA/ENG.XNF", &xnf))
		return 0;

	if (xnf.size < XA_HEADER_SIZE)
	{
		NativeAudio_FreeByteBuffer(&xnf);
		return 0;
	}

	if ((NativeAudio_ReadLE32(&xnf.data[0]) != 0x464e4958) || (NativeAudio_ReadLE32(&xnf.data[4]) != 102) ||
	    (NativeAudio_ReadLE32(&xnf.data[8]) != XA_NUM_TYPES))
	{
		NativeAudio_FreeByteBuffer(&xnf);
		return 0;
	}

	numXasTotal = NativeAudio_ReadLE32(&xnf.data[XA_NUM_XAS_TOTAL_OFFSET]);
	numTracksTotal = NativeAudio_ReadLE32(&xnf.data[XA_NUM_TRACKS_TOTAL_OFFSET]);
	if ((numXasTotal < 0) || (numTracksTotal < 0) || (numXasTotal > ((INT_MAX - XA_HEADER_SIZE) / 4)))
	{
		NativeAudio_FreeByteBuffer(&xnf);
		return 0;
	}

	xaSizeOffset = XA_HEADER_SIZE + numXasTotal * 4;
	if (numTracksTotal > ((INT_MAX - xaSizeOffset) / XA_SIZE_ENTRY_BYTES))
	{
		NativeAudio_FreeByteBuffer(&xnf);
		return 0;
	}

	xaSizeEnd = xaSizeOffset + numTracksTotal * XA_SIZE_ENTRY_BYTES;
	if ((xaSizeEnd < xaSizeOffset) || (xaSizeEnd > xnf.size))
	{
		NativeAudio_FreeByteBuffer(&xnf);
		return 0;
	}

	numSongs = NativeAudio_ReadLE32(&xnf.data[XA_NUM_SONGS_OFFSET + categoryID * 4]);
	firstSongIndex = NativeAudio_ReadLE32(&xnf.data[XA_FIRST_SONG_INDEX_OFFSET + categoryID * 4]);
	if (xaID >= numSongs)
	{
		NativeAudio_FreeByteBuffer(&xnf);
		return 0;
	}

	entryIndex = firstSongIndex + xaID;
	if ((entryIndex < 0) || (entryIndex >= numTracksTotal))
	{
		NativeAudio_FreeByteBuffer(&xnf);
		return 0;
	}

	entry = &xnf.data[xaSizeOffset + entryIndex * XA_SIZE_ENTRY_BYTES];
	info->channelFilter = entry[0];
	info->fileNumber = entry[1];
	info->numSectors = NativeAudio_ReadLE16Signed(entry + 2);

	NativeAudio_FreeByteBuffer(&xnf);
	return info->numSectors > 0;
}

static int NativeAudio_DecodeXA28Nibbles(const u8 *sector, int frameOff, int block, int nibble, int channel, struct NativeAudioXaDecodeState *state,
                                         struct NativeAudioPcmBuffer *out)
{
	int param = sector[frameOff + 4 + block * 2 + nibble];
	int shift = param & 0xf;
	int weight = (param >> 4) & 0xf;
	int w0;
	int w1;
	int i;

	if (weight > 4)
		weight = 4;

	w0 = s_posTable[weight];
	w1 = s_negTable[weight];

	for (i = 0; i < XA_SAMPLES_PER_SOUND_UNIT; i++)
	{
		u8 byte = sector[frameOff + 16 + i * 4 + block];
		u8 nib = (nibble == 0) ? (u8)((byte & 0xf) << 4) : (u8)(byte & 0xf0);
		int sample = ((s8)nib) << 8;

		sample >>= shift;
		sample += (state->old[channel] * w0) >> 6;
		sample += (state->older[channel] * w1) >> 6;
		sample = NativeAudio_Clamp16(sample);

		state->older[channel] = state->old[channel];
		state->old[channel] = sample;
		if (!NativeAudio_PcmPush(out, (s16)sample))
			return 0;
	}

	return 1;
}

static int NativeAudio_DecodeXASectorMono(const u8 *sector, int sectorBase, struct NativeAudioXaDecodeState *state, struct NativeAudioPcmBuffer *out)
{
	int frame;

	for (frame = 0; frame < XA_FRAMES_PER_SECTOR; frame++)
	{
		int frameOff = sectorBase + XA_SUBHEADER_SIZE + frame * XA_FRAME_SIZE;
		const u8 *header = &sector[frameOff + 4];
		int su;

		for (su = 0; su < XA_SUBFRAMES_PER_FRAME; su++)
		{
			int paramIndex = (su & 3) | ((su & 4) << 1);
			int param = header[paramIndex];
			int shift = param & 0xf;
			int weight = (param >> 4) & 0xf;
			int w0;
			int w1;
			int i;

			if (weight > 4)
				weight = 4;

			w0 = s_posTable[weight];
			w1 = s_negTable[weight];

			for (i = 0; i < XA_SAMPLES_PER_SOUND_UNIT; i++)
			{
				u8 byte = sector[frameOff + 16 + i * 4 + (su >> 1)];
				u8 nib = ((su & 1) == 0) ? (u8)((byte & 0xf) << 4) : (u8)(byte & 0xf0);
				int sample = ((s8)nib) << 8;

				sample >>= shift;
				sample += (state->old[0] * w0) >> 6;
				sample += (state->older[0] * w1) >> 6;
				sample = NativeAudio_Clamp16(sample);

				state->older[0] = state->old[0];
				state->old[0] = sample;
				if (!NativeAudio_PcmPush(out, (s16)sample))
					return 0;
			}
		}
	}

	return 1;
}

static int NativeAudio_DecodeXASectorStereo(const u8 *sector, int sectorBase, struct NativeAudioXaDecodeState *state, struct NativeAudioPcmBuffer *out)
{
	int frame;

	for (frame = 0; frame < XA_FRAMES_PER_SECTOR; frame++)
	{
		int frameOff = sectorBase + XA_SUBHEADER_SIZE + frame * XA_FRAME_SIZE;
		int block;

		for (block = 0; block < XA_BLOCKS_PER_FRAME; block++)
		{
			s16 left[XA_SAMPLES_PER_SOUND_UNIT];
			s16 right[XA_SAMPLES_PER_SOUND_UNIT];
			s16 tmpSamples[XA_SAMPLES_PER_SOUND_UNIT];
			struct NativeAudioPcmBuffer tmp;
			int i;

			memset(&tmp, 0, sizeof(tmp));
			tmp.samples = tmpSamples;
			tmp.capacity = XA_SAMPLES_PER_SOUND_UNIT;
			if (!NativeAudio_DecodeXA28Nibbles(sector, frameOff, block, 0, 0, state, &tmp) || (tmp.count < XA_SAMPLES_PER_SOUND_UNIT))
				return 0;
			for (i = 0; i < XA_SAMPLES_PER_SOUND_UNIT; i++)
				left[i] = tmp.samples[i];
			tmp.count = 0;

			if (!NativeAudio_DecodeXA28Nibbles(sector, frameOff, block, 1, 1, state, &tmp) || (tmp.count < XA_SAMPLES_PER_SOUND_UNIT))
				return 0;
			for (i = 0; i < XA_SAMPLES_PER_SOUND_UNIT; i++)
				right[i] = tmp.samples[i];

			for (i = 0; i < XA_SAMPLES_PER_SOUND_UNIT; i++)
			{
				if (!NativeAudio_PcmPush(out, left[i]) || !NativeAudio_PcmPush(out, right[i]))
					return 0;
			}
		}
	}

	return 1;
}

static int NativeAudio_DecodeXAFile(const u8 *bytes, int byteCount, int channelFilter, int maxSectors, struct NativeAudioPcmBuffer *pcm, int *sampleRate,
                                    int *numChannels)
{
	int sectorSize;
	int sectorBase;
	int totalSectors;
	int sectorsToScan;
	struct NativeAudioXaDecodeState state;
	int sector;

	if ((byteCount <= 0) || (maxSectors <= 0))
		return 0;

	if ((byteCount % XA_FULL_SECTOR_SIZE) == 0)
		sectorSize = XA_FULL_SECTOR_SIZE;
	else if ((byteCount % XA_FORM2_SECTOR_SIZE) == 0)
		sectorSize = XA_FORM2_SECTOR_SIZE;
	else
		return 0;

	sectorBase = (sectorSize == XA_FULL_SECTOR_SIZE) ? 16 : 0;
	totalSectors = byteCount / sectorSize;
	sectorsToScan = maxSectors < totalSectors ? maxSectors : totalSectors;
	memset(&state, 0, sizeof(state));

	*sampleRate = XA_SAMPLE_RATE_37800;
	*numChannels = 1;

	for (sector = 0; sector < sectorsToScan; sector++)
	{
		const u8 *src = &bytes[sector * sectorSize];
		const u8 *header = &src[sectorBase];
		int subMode = header[2];
		int coding = header[3];
		int isStereo = (coding & 0x03) != 0;
		int srBits = (coding >> 2) & 0x03;
		int bpsBits = (coding >> 4) & 0x03;

		if ((subMode & 0x04) == 0)
			continue;
		if ((header[0] != 1) || (header[1] != channelFilter))
			continue;
		if (bpsBits != 0)
			continue;

		*sampleRate = (srBits == 0) ? XA_SAMPLE_RATE_37800 : XA_SAMPLE_RATE_18900;
		*numChannels = isStereo ? 2 : 1;

		if (isStereo)
		{
			if (!NativeAudio_DecodeXASectorStereo(src, sectorBase, &state, pcm))
				return 0;
		}
		else if (!NativeAudio_DecodeXASectorMono(src, sectorBase, &state, pcm))
		{
			return 0;
		}
	}

	return pcm->count > 0;
}

static int NativeAudio_GetXAMaxStereoSamples(int numSectors, int *sampleCountOut)
{
	int samplesPerSector;

	if (numSectors <= 0)
		return 0;

	samplesPerSector = XA_FRAMES_PER_SECTOR * XA_SUBFRAMES_PER_FRAME * XA_SAMPLES_PER_SOUND_UNIT * NATIVE_AUDIO_CHANNELS;
	if (numSectors > (INT_MAX / samplesPerSector))
		return 0;

	*sampleCountOut = numSectors * samplesPerSector;
	return 1;
}

static int NativeAudio_PrepareXAPcmBuffer(struct NativeAudioDecodeArena *arena, int numSectors, struct NativeAudioPcmBuffer *pcm, int *markerOut)
{
	int maxSamples;
	int maxBytes;

	if (!NativeAudio_GetXAMaxStereoSamples(numSectors, &maxSamples))
		return 0;
	if (maxSamples > (INT_MAX / (int)sizeof(s16)))
		return 0;

	maxBytes = maxSamples * (int)sizeof(s16);
	NativeAudio_ArenaReset(arena);
	if (!NativeAudio_ArenaEnsureCapacity(arena, maxBytes))
		return 0;

	pcm->samples = (s16 *)NativeAudio_ArenaPush(arena, maxBytes, NATIVE_AUDIO_ARENA_ALIGN, markerOut);
	if (pcm->samples == NULL)
		return 0;

	pcm->count = 0;
	pcm->capacity = maxSamples;
	return 1;
}

static int NativeAudio_LoadXATrackPcm(struct NativeAudioDecodeArena *arena, int categoryID, int xaID, s16 **pcmOut, int *frameCountOut, int *sampleRateOut)
{
	struct NativeAudioXaTrackInfo info;
	struct NativeAudioByteBuffer data;
	struct NativeAudioPcmBuffer pcm;
	char path[128];
	int sampleRate;
	int numChannels;
	int frameCount;
	int i;
	int arenaMarker = 0;

	*pcmOut = NULL;
	*frameCountOut = 0;
	*sampleRateOut = 0;

	if (!NativeAudio_LookupXATrackInfo(categoryID, xaID, &info))
		return 0;
	if (!NativeAudio_BuildXAPath(path, sizeof(path), categoryID, info.fileNumber))
		return 0;
	if (!NativeAudio_ReadFileBytes(path, &data))
		return 0;

	memset(&pcm, 0, sizeof(pcm));
	if (!NativeAudio_PrepareXAPcmBuffer(arena, info.numSectors, &pcm, &arenaMarker))
	{
		NativeAudio_FreeByteBuffer(&data);
		return 0;
	}

	if (!NativeAudio_DecodeXAFile(data.data, data.size, info.channelFilter, info.numSectors, &pcm, &sampleRate, &numChannels))
	{
		NativeAudio_FreeByteBuffer(&data);
		NativeAudio_ArenaRewind(arena, arenaMarker);
		return 0;
	}

	NativeAudio_FreeByteBuffer(&data);

	if (numChannels == 1)
	{
		if (pcm.count > (INT_MAX / 2))
		{
			NativeAudio_ArenaRewind(arena, arenaMarker);
			return 0;
		}

		for (i = pcm.count; i-- > 0;)
		{
			pcm.samples[i * 2] = pcm.samples[i];
			pcm.samples[i * 2 + 1] = pcm.samples[i];
		}

		frameCount = pcm.count;
		pcm.count *= 2;
	}
	else
	{
		frameCount = pcm.count / 2;
	}

	NativeAudio_ArenaRewind(arena, arenaMarker + pcm.count * (int)sizeof(s16));
	*pcmOut = pcm.samples;
	*frameCountOut = frameCount;
	*sampleRateOut = sampleRate;
	return 1;
}

static void NativeAudio_MixSample(int *dstLeft, int *dstRight, int sampleLeft, int sampleRight)
{
	*dstLeft += sampleLeft;
	*dstRight += sampleRight;
}

static int NativeAudio_GetQueuedFramesNoLock(void)
{
	const int frameBytes = (int)sizeof(s16) * NATIVE_AUDIO_CHANNELS;
	int queuedBytes;
	int queuedFrames = s_audio.output.scheduledFrameCount;

	if (s_audio.output.stream != NULL)
	{
		queuedBytes = SDL_GetAudioStreamQueued(s_audio.output.stream);
		if (queuedBytes > 0)
			queuedFrames += queuedBytes / frameBytes;
	}

	return queuedFrames;
}

static void NativeAudio_AddUnderrunFramesNoLock(int frameCount)
{
#ifdef CTR_INTERNAL
	if (frameCount <= 0)
		return;

	if (frameCount > INT_MAX - s_audio.output.underrunFrames)
		s_audio.output.underrunFrames = INT_MAX;
	else
		s_audio.output.underrunFrames += frameCount;
#else
	(void)frameCount;
#endif
}

static void NativeAudio_AddOverflowFramesNoLock(int frameCount)
{
#ifdef CTR_INTERNAL
	if (frameCount <= 0)
		return;

	if (frameCount > INT_MAX - s_audio.output.overflowFrames)
		s_audio.output.overflowFrames = INT_MAX;
	else
		s_audio.output.overflowFrames += frameCount;
#else
	(void)frameCount;
#endif
}

static int NativeAudio_QueueRenderedFramesNoLock(const s16 *frames, int frameCount)
{
	const int frameSamples = NATIVE_AUDIO_CHANNELS;
	int framesQueued = 0;

	if ((frames == NULL) || (frameCount <= 0))
		return 0;

	while ((framesQueued < frameCount) && (s_audio.output.scheduledFrameCount < NATIVE_AUDIO_SCHEDULED_QUEUE_FRAMES))
	{
		int writeFrame = (s_audio.output.scheduledReadFrame + s_audio.output.scheduledFrameCount) % NATIVE_AUDIO_SCHEDULED_QUEUE_FRAMES;
		int writable = NATIVE_AUDIO_SCHEDULED_QUEUE_FRAMES - s_audio.output.scheduledFrameCount;
		int contiguous = NATIVE_AUDIO_SCHEDULED_QUEUE_FRAMES - writeFrame;
		int remaining = frameCount - framesQueued;
		int chunkFrames = remaining;

		if (chunkFrames > writable)
			chunkFrames = writable;
		if (chunkFrames > contiguous)
			chunkFrames = contiguous;

		memcpy(&s_audio.output.scheduledPcm[writeFrame * frameSamples], &frames[framesQueued * frameSamples],
		       (size_t)chunkFrames * frameSamples * sizeof(s16));
		s_audio.output.scheduledFrameCount += chunkFrames;
		framesQueued += chunkFrames;
	}

	if (framesQueued < frameCount)
		NativeAudio_AddOverflowFramesNoLock(frameCount - framesQueued);

	return framesQueued;
}

static int NativeAudio_DrainRenderedFramesNoLock(s16 *out, int frameCount)
{
	const int frameSamples = NATIVE_AUDIO_CHANNELS;
	int framesDrained = 0;

	if ((out == NULL) || (frameCount <= 0))
		return 0;

	while ((framesDrained < frameCount) && (s_audio.output.scheduledFrameCount > 0))
	{
		int contiguous = NATIVE_AUDIO_SCHEDULED_QUEUE_FRAMES - s_audio.output.scheduledReadFrame;
		int remaining = frameCount - framesDrained;
		int chunkFrames = remaining;

		if (chunkFrames > s_audio.output.scheduledFrameCount)
			chunkFrames = s_audio.output.scheduledFrameCount;
		if (chunkFrames > contiguous)
			chunkFrames = contiguous;

		memcpy(&out[framesDrained * frameSamples], &s_audio.output.scheduledPcm[s_audio.output.scheduledReadFrame * frameSamples],
		       (size_t)chunkFrames * frameSamples * sizeof(s16));

		s_audio.output.scheduledReadFrame = (s_audio.output.scheduledReadFrame + chunkFrames) % NATIVE_AUDIO_SCHEDULED_QUEUE_FRAMES;
		s_audio.output.scheduledFrameCount -= chunkFrames;
		framesDrained += chunkFrames;
	}

	return framesDrained;
}

static void SDLCALL NativeAudio_StreamCallback(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount)
{
	const int frameBytes = (int)sizeof(s16) * NATIVE_AUDIO_CHANNELS;
	s16 out[NATIVE_AUDIO_VBLANK_FRAMES * NATIVE_AUDIO_CHANNELS];
	int framesNeeded;

	(void)userdata;
	(void)total_amount;

	if ((stream == NULL) || (additional_amount <= 0))
		return;

	framesNeeded = (additional_amount + frameBytes - 1) / frameBytes;
#ifdef CTR_INTERNAL
	if (framesNeeded > s_audio.output.callbackMaxRequestFrames)
		s_audio.output.callbackMaxRequestFrames = framesNeeded;
#endif

	while (framesNeeded > 0)
	{
		int chunkFrames = framesNeeded < NATIVE_AUDIO_VBLANK_FRAMES ? framesNeeded : NATIVE_AUDIO_VBLANK_FRAMES;
		int framesReady;

		if (s_audio.output.deterministicRenderMode)
		{
			// NOTE(aalhendi): Deterministic replay/TAS mode must advance audio
			// from its scheduler. SDL only drains already-rendered PCM here.
			framesReady = NativeAudio_DrainRenderedFramesNoLock(out, chunkFrames);
			if (framesReady < chunkFrames)
			{
				memset(&out[framesReady * NATIVE_AUDIO_CHANNELS], 0, (size_t)(chunkFrames - framesReady) * frameBytes);
				NativeAudio_AddUnderrunFramesNoLock(chunkFrames - framesReady);
			}
		}
		else
		{
			framesReady = NativeAudio_RenderFramesNoLock(out, chunkFrames);
			if (framesReady < chunkFrames)
				memset(&out[framesReady * NATIVE_AUDIO_CHANNELS], 0, (size_t)(chunkFrames - framesReady) * frameBytes);
		}

		if (!SDL_PutAudioStreamData(stream, out, chunkFrames * frameBytes))
		{
			NativeAudio_AddUnderrunFramesNoLock(chunkFrames);
			break;
		}

		framesNeeded -= chunkFrames;
	}
}

void NativeAudio_ClearOutputQueue(void)
{
	NativeAudio_LockOutput();

	NativeAudio_ClearOutputQueueNoLock();

	NativeAudio_UnlockOutput();
}

void NativeAudio_SetDeterministicRenderMode(int enabled)
{
	NativeAudio_LockOutput();

	if (s_audio.output.deterministicRenderMode != (enabled != 0))
	{
		s_audio.output.deterministicRenderMode = enabled != 0;
		NativeAudio_ClearOutputQueueNoLock();
	}

	NativeAudio_UnlockOutput();
}

int NativeAudio_IsDeterministicRenderMode(void)
{
	int enabled;

	NativeAudio_LockOutput();

	enabled = s_audio.output.deterministicRenderMode;

	NativeAudio_UnlockOutput();

	return enabled;
}

int NativeAudio_QueueRenderedFrames(const s16 *frames, int frameCount)
{
	int framesQueued;

	if (frameCount <= 0)
		return 0;

	NativeAudio_LockOutput();

	if (s_audio.output.deterministicRenderMode)
		framesQueued = NativeAudio_QueueRenderedFramesNoLock(frames, frameCount);
	else
		framesQueued = 0;

	NativeAudio_UnlockOutput();

	return framesQueued;
}

#ifdef CTR_INTERNAL
void NativeAudio_GetOutputStats(int *underrunFrames, int *overflowFrames, int *queuedFrames)
{
	NativeAudio_LockOutput();

	if (underrunFrames != NULL)
		*underrunFrames = s_audio.output.underrunFrames;
	if (overflowFrames != NULL)
		*overflowFrames = s_audio.output.overflowFrames;
	if (queuedFrames != NULL)
		*queuedFrames = NativeAudio_GetQueuedFramesNoLock();

	NativeAudio_UnlockOutput();
}

static int NativeAudio_ShouldReportOutputStatsNoLock(int *underrunFrames, int *overflowFrames, int *queuedFrames)
{
	if (s_audio.output.reportVBlankCountdown > 0)
	{
		s_audio.output.reportVBlankCountdown--;
		return 0;
	}

	s_audio.output.reportVBlankCountdown = 60;

	*underrunFrames = s_audio.output.underrunFrames;
	*overflowFrames = s_audio.output.overflowFrames;
	*queuedFrames = NativeAudio_GetQueuedFramesNoLock();

	if ((*underrunFrames == 0) && (*overflowFrames == 0))
		return 0;

	if ((*underrunFrames == s_audio.output.lastReportedUnderrunFrames) && (*overflowFrames == s_audio.output.lastReportedOverflowFrames) &&
	    (s_audio.output.callbackMaxRequestFrames == s_audio.output.lastReportedCallbackMaxRequestFrames))
	{
		return 0;
	}

	s_audio.output.lastReportedUnderrunFrames = *underrunFrames;
	s_audio.output.lastReportedOverflowFrames = *overflowFrames;
	s_audio.output.lastReportedCallbackMaxRequestFrames = s_audio.output.callbackMaxRequestFrames;
	return 1;
}
#endif

int NativeAudio_GetStateSize(void)
{
	return (int)sizeof(struct NativeAudioSnapshot);
}

int NativeAudio_CaptureState(void *dst, int dstSize)
{
	struct NativeAudioSnapshot *snapshot = (struct NativeAudioSnapshot *)dst;
	int i;

	if ((dst == NULL) || (dstSize < (int)sizeof(*snapshot)))
		return 0;

	NativeAudio_LockOutput();

	memset(snapshot, 0, sizeof(*snapshot));
	snapshot->magic = NATIVE_AUDIO_STATE_MAGIC;
	snapshot->version = NATIVE_AUDIO_STATE_VERSION;
	snapshot->size = sizeof(*snapshot);
	snapshot->init = s_audio.init;
	snapshot->muted = s_audio.muted;
	snapshot->spuAllocCursor = s_audio.spu.allocCursor;
	snapshot->reverbEnabled = s_audio.reverbEnabled;
	snapshot->cdMixEnabled = s_audio.cdMixEnabled;
	snapshot->cdReverbEnabled = s_audio.cdReverbEnabled;
	snapshot->reverbWorkAreaReserved = s_audio.reverbWorkAreaReserved;
	snapshot->spuTransferOffset = s_audio.spu.transferOffset;
	snapshot->masterVolumeLeft = s_audio.masterVolumeLeft;
	snapshot->masterVolumeRight = s_audio.masterVolumeRight;
	snapshot->reverbVoiceBits = s_audio.reverbVoiceBits;
	snapshot->reverbAttr = s_audio.reverbAttr;
	snapshot->reverb = s_audio.reverb;
	snapshot->commonAttr = s_audio.commonAttr;
	for (i = 0; i < NATIVE_AUDIO_SPU_VOICE_COUNT; i++)
		NativeAudio_CopyVoiceToState(&snapshot->voices[i], &s_audio.voices[i]);
	NativeAudio_CopyXAToState(&snapshot->xa, &s_audio.xa);
	memcpy(snapshot->spuSampleMem, s_audio.spu.memory, sizeof(snapshot->spuSampleMem));

	NativeAudio_UnlockOutput();

	return 1;
}

int NativeAudio_RestoreState(const void *src, int srcSize)
{
	const struct NativeAudioSnapshot *snapshot = (const struct NativeAudioSnapshot *)src;
	s16 *xaPcm = NULL;
	int xaFrameCount = 0;
	int xaSampleRate = 0;
	int i;
	int restoreInit;

	if ((src == NULL) || (srcSize < (int)sizeof(*snapshot)))
		return 0;
	if ((snapshot->magic != NATIVE_AUDIO_STATE_MAGIC) || (snapshot->version != NATIVE_AUDIO_STATE_VERSION) || (snapshot->size != sizeof(*snapshot)))
		return 0;
	if ((snapshot->spuAllocCursor < 0) || (snapshot->spuAllocCursor > NATIVE_AUDIO_SPU_MEMSIZE))
		return 0;
	if ((snapshot->spuTransferOffset < 0) || (snapshot->spuTransferOffset > NATIVE_AUDIO_SPU_MEMSIZE))
		return 0;
	for (i = 0; i < NATIVE_AUDIO_SPU_VOICE_COUNT; i++)
	{
		if (!NativeAudio_ValidateVoiceSnapshot(&snapshot->voices[i]))
			return 0;
	}
	if (!NativeAudio_ValidateXASnapshot(&snapshot->xa))
		return 0;
	if (!NativeAudio_ValidateReverbSnapshot(&snapshot->reverb))
		return 0;

	restoreInit = snapshot->init != 0;
	if (restoreInit && !NativeAudio_OpenDevice())
		return 0;

	if (snapshot->xa.active && snapshot->xa.hasTrackIdentity)
	{
		if (!NativeAudio_LoadXATrackPcm(&s_audio.xaPendingPcmArena, snapshot->xa.categoryID, snapshot->xa.xaID, &xaPcm, &xaFrameCount, &xaSampleRate))
			return 0;
	}

	NativeAudio_LockOutput();

	NativeAudio_ResetVoicePcmArenaNoLock(0);
	NativeAudio_FreeXA();

	s_audio.init = restoreInit;
	s_audio.muted = snapshot->muted;
	s_audio.spu.allocCursor = snapshot->spuAllocCursor;
	s_audio.reverbEnabled = snapshot->reverbEnabled;
	s_audio.cdMixEnabled = snapshot->cdMixEnabled;
	s_audio.cdReverbEnabled = snapshot->cdReverbEnabled;
	s_audio.reverbWorkAreaReserved = snapshot->reverbWorkAreaReserved;
	s_audio.masterVolumeLeft = snapshot->masterVolumeLeft;
	s_audio.masterVolumeRight = snapshot->masterVolumeRight;
	s_audio.reverbVoiceBits = snapshot->reverbVoiceBits;
	s_audio.reverbAttr = snapshot->reverbAttr;
	s_audio.reverb = snapshot->reverb;
	s_audio.commonAttr = snapshot->commonAttr;
	memcpy(s_audio.spu.memory, snapshot->spuSampleMem, sizeof(s_audio.spu.memory));
	s_audio.spu.transferOffset = snapshot->spuTransferOffset;
	for (i = 0; i < NATIVE_AUDIO_SPU_VOICE_COUNT; i++)
		NativeAudio_CopyStateToVoice(&s_audio.voices[i], &snapshot->voices[i]);

	if (snapshot->xa.active && snapshot->xa.hasTrackIdentity)
	{
		NativeAudio_ArenaSwap(&s_audio.xaPcmArena, &s_audio.xaPendingPcmArena);
		s_audio.xa.pcm = xaPcm;
		s_audio.xa.frameCount = xaFrameCount;
		s_audio.xa.sampleRate = xaSampleRate;
		s_audio.xa.categoryID = snapshot->xa.categoryID;
		s_audio.xa.xaID = snapshot->xa.xaID;
		s_audio.xa.hasTrackIdentity = 1;
		s_audio.xa.active = snapshot->xa.active;
		s_audio.xa.outputFrame = snapshot->xa.outputFrame;
		s_audio.xa.stepFp = (u32)(((u64)xaSampleRate << NATIVE_AUDIO_FP_SHIFT) / NATIVE_AUDIO_SAMPLE_RATE);
		s_audio.xa.volumeLeft = snapshot->xa.volumeLeft;
		s_audio.xa.volumeRight = snapshot->xa.volumeRight;
		NativeAudio_UpdateXAPositionFromOutputFrameNoLock();
	}
	else
	{
		s_audio.xa.frameCount = snapshot->xa.frameCount;
		s_audio.xa.sampleRate = snapshot->xa.sampleRate;
		s_audio.xa.categoryID = snapshot->xa.categoryID;
		s_audio.xa.xaID = snapshot->xa.xaID;
		s_audio.xa.hasTrackIdentity = snapshot->xa.hasTrackIdentity;
		s_audio.xa.active = 0;
		s_audio.xa.positionFp = snapshot->xa.positionFp;
		s_audio.xa.outputFrame = snapshot->xa.outputFrame;
		s_audio.xa.stepFp = snapshot->xa.stepFp;
		s_audio.xa.volumeLeft = snapshot->xa.volumeLeft;
		s_audio.xa.volumeRight = snapshot->xa.volumeRight;
	}

	NativeAudio_ClearOutputQueueNoLock();

	NativeAudio_UnlockOutput();

	return 1;
}

static void NativeAudio_MixFrame(s16 *outLeft, s16 *outRight)
{
	int mixLeft = 0;
	int mixRight = 0;
	int reverbSendLeft = 0;
	int reverbSendRight = 0;
	int reverbWetLeft = 0;
	int reverbWetRight = 0;
	int i;

	if (s_audio.xa.active && s_audio.xa.pcm != NULL)
	{
		u64 outputFrameCount = NativeAudio_GetXAOutputFrameCount(s_audio.xa.frameCount, s_audio.xa.sampleRate);

		if ((outputFrameCount == 0) || (s_audio.xa.outputFrame >= outputFrameCount))
		{
			s_audio.xa.active = 0;
		}
		else if (!s_audio.cdMixEnabled)
		{
			NativeAudio_AdvanceXAOutputFrameNoLock();
		}
		else
		{
			int srcLeft = NativeAudio_GetXAMixSampleNoLock(0);
			int srcRight = NativeAudio_GetXAMixSampleNoLock(1);
			int left = NativeAudio_ApplyVolume(srcLeft, s_audio.xa.volumeLeft, s_audio.masterVolumeLeft);
			int right = NativeAudio_ApplyVolume(srcRight, s_audio.xa.volumeRight, s_audio.masterVolumeRight);

			NativeAudio_MixSample(&mixLeft, &mixRight, left, right);
			if (s_audio.cdReverbEnabled)
			{
				NativeAudio_MixSample(&reverbSendLeft, &reverbSendRight, NativeAudio_ApplyVolume(srcLeft, s_audio.xa.volumeLeft, NATIVE_AUDIO_DIRECT_VOL_MAX),
				                      NativeAudio_ApplyVolume(srcRight, s_audio.xa.volumeRight, NATIVE_AUDIO_DIRECT_VOL_MAX));
			}
			NativeAudio_AdvanceXAOutputFrameNoLock();
		}
	}

	if (!s_audio.muted)
	{
		for (i = 0; i < NATIVE_AUDIO_SPU_VOICE_COUNT; i++)
		{
			struct NativeAudioVoice *voice = &s_audio.voices[i];
			u64 sampleIndex;
			u32 stepFp;
			int sample;
			int left;
			int right;

			if (voice->active && ((voice->pcm == NULL) || voice->sampleDirty))
				NativeAudio_UpdateVoiceSample(voice);

			if (!voice->active || voice->pcm == NULL || voice->sampleCount <= 0 || voice->adsrPhase == NATIVE_AUDIO_ADSR_OFF)
				continue;

			if (voice->attr.pitch == 0)
			{
				NativeAudio_AdsrAdvance(voice);
				continue;
			}

			sampleIndex = voice->positionFp >> NATIVE_AUDIO_FP_SHIFT;
			if (sampleIndex >= (u64)voice->sampleCount)
			{
				if (voice->loopEnabled && voice->loopEnd > voice->loopStart)
				{
					NativeAudio_WrapVoiceLoop(voice);
					sampleIndex = voice->positionFp >> NATIVE_AUDIO_FP_SHIFT;
				}
				else
				{
					NativeAudio_AdsrForceOff(voice);
					continue;
				}
			}

			sample = NativeAudio_InterpolateVoiceSample(voice);
			sample = NativeAudio_ApplyAdsrEnvelope(sample, voice->adsrLevel);
			left = NativeAudio_ApplyVolume(sample, voice->attr.volume.left, s_audio.masterVolumeLeft);
			right = NativeAudio_ApplyVolume(sample, voice->attr.volume.right, s_audio.masterVolumeRight);
			NativeAudio_MixSample(&mixLeft, &mixRight, left, right);
			if (voice->reverb)
			{
				NativeAudio_MixSample(&reverbSendLeft, &reverbSendRight, NativeAudio_ApplyVolume(sample, voice->attr.volume.left, NATIVE_AUDIO_DIRECT_VOL_MAX),
				                      NativeAudio_ApplyVolume(sample, voice->attr.volume.right, NATIVE_AUDIO_DIRECT_VOL_MAX));
			}
			NativeAudio_AdsrAdvance(voice);

			stepFp = ((u32)voice->attr.pitch << NATIVE_AUDIO_FP_SHIFT) / 0x1000u;
			voice->positionFp += stepFp;

			if (voice->loopEnabled && ((voice->positionFp >> NATIVE_AUDIO_FP_SHIFT) >= (u64)voice->loopEnd))
				NativeAudio_WrapVoiceLoop(voice);
		}
	}

	NativeAudio_ReverbProcessNoLock(reverbSendLeft, reverbSendRight, &reverbWetLeft, &reverbWetRight);
	NativeAudio_MixSample(&mixLeft, &mixRight, NativeAudio_ApplyMasterVolume(reverbWetLeft, s_audio.masterVolumeLeft),
	                      NativeAudio_ApplyMasterVolume(reverbWetRight, s_audio.masterVolumeRight));

	*outLeft = (s16)NativeAudio_Clamp16(mixLeft);
	*outRight = (s16)NativeAudio_Clamp16(mixRight);
}

static int NativeAudio_RenderFramesNoLock(s16 *out, int frameCount)
{
	int frame;

	if ((out == NULL) || (frameCount <= 0) || (frameCount > INT_MAX / NATIVE_AUDIO_CHANNELS))
		return 0;

	for (frame = 0; frame < frameCount; frame++)
	{
		if (s_audio.init)
			NativeAudio_MixFrame(&out[frame * NATIVE_AUDIO_CHANNELS], &out[frame * NATIVE_AUDIO_CHANNELS + 1]);
		else
		{
			out[frame * NATIVE_AUDIO_CHANNELS] = 0;
			out[frame * NATIVE_AUDIO_CHANNELS + 1] = 0;
		}
	}

	return frameCount;
}

int NativeAudio_RenderFrames(s16 *out, int frameCount)
{
	int framesRendered;

	if ((out == NULL) || (frameCount <= 0))
		return 0;

	NativeAudio_LockOutput();

	framesRendered = NativeAudio_RenderFramesNoLock(out, frameCount);

	NativeAudio_UnlockOutput();

	return framesRendered;
}

void NativeAudio_StepVBlank(void)
{
#ifdef CTR_INTERNAL
	int shouldReportStats = 0;
	int underrunFrames = 0;
	int overflowFrames = 0;
	int queuedFrames = 0;
#endif

	if (!s_audio.init)
		return;

	NativeAudio_LockOutput();

	if (s_audio.output.deterministicRenderMode)
	{
		s16 renderedFrames[NATIVE_AUDIO_VBLANK_FRAMES * NATIVE_AUDIO_CHANNELS];

		// NOTE(aalhendi): Replay/TAS mode advances SPU/XA from the native PS1
		// VBlank clock. SDL only drains the scheduled PCM in its callback.
		int framesRendered = NativeAudio_RenderFramesNoLock(renderedFrames, NATIVE_AUDIO_VBLANK_FRAMES);
		NativeAudio_QueueRenderedFramesNoLock(renderedFrames, framesRendered);
	}

#ifdef CTR_INTERNAL
	shouldReportStats = NativeAudio_ShouldReportOutputStatsNoLock(&underrunFrames, &overflowFrames, &queuedFrames);
#endif

	NativeAudio_UnlockOutput();

#ifdef CTR_INTERNAL
	if (shouldReportStats)
	{
		printf("[CTR Native] Audio output stats: underrunFrames=%d overflowFrames=%d queuedFrames=%d callbackMaxRequestFrames=%d\n", underrunFrames, overflowFrames,
		       queuedFrames, s_audio.output.callbackMaxRequestFrames);
	}
#endif
}

static int NativeAudio_OpenDevice(void)
{
	SDL_AudioSpec want;
	SDL_AudioSpec srcSpec;
	SDL_AudioSpec dstSpec;
	SDL_AudioSpec deviceSpec;
	int deviceSampleFrames = 0;

	if (NativeAudio_OutputOpen())
		return 1;

	NativeAudio_SelectDriverHint();

	if (SDL_InitSubSystem(SDL_INIT_AUDIO) == 0)
		return 0;

	memset(&want, 0, sizeof(want));
	memset(&srcSpec, 0, sizeof(srcSpec));
	memset(&dstSpec, 0, sizeof(dstSpec));
	memset(&deviceSpec, 0, sizeof(deviceSpec));
	want.freq = NATIVE_AUDIO_SAMPLE_RATE;
	want.format = SDL_AUDIO_S16;
	want.channels = NATIVE_AUDIO_CHANNELS;

	s_audio.output.stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &want, NativeAudio_StreamCallback, NULL);
	if (s_audio.output.stream == NULL)
	{
		fprintf(stderr, "[CTR Native] SDL audio unavailable: %s\n", SDL_GetError());
		return 0;
	}

	s_audio.output.device = SDL_GetAudioStreamDevice(s_audio.output.stream);
	if (!SDL_GetAudioDeviceFormat(s_audio.output.device, &deviceSpec, &deviceSampleFrames))
		deviceSampleFrames = 0;
	if (!SDL_GetAudioStreamFormat(s_audio.output.stream, &srcSpec, &dstSpec))
	{
		fprintf(stderr, "[CTR Native] SDL audio stream format unavailable: %s\n", SDL_GetError());
		SDL_DestroyAudioStream(s_audio.output.stream);
		s_audio.output.stream = NULL;
		s_audio.output.device = 0;
		return 0;
	}

	if ((srcSpec.freq != want.freq) || (srcSpec.format != want.format) || (srcSpec.channels != want.channels))
	{
		fprintf(stderr, "[CTR Native] SDL audio rejected fixed PCM contract: got %d Hz format 0x%x channels %d\n", srcSpec.freq, srcSpec.format, srcSpec.channels);
		SDL_DestroyAudioStream(s_audio.output.stream);
		s_audio.output.stream = NULL;
		s_audio.output.device = 0;
		return 0;
	}

	printf("[CTR Native] SDL audio stream opened: driver=%s src=%d Hz/%d ch dst=%d Hz/%d ch device=%d Hz/%d ch sampleFrames=%d\n", SDL_GetCurrentAudioDriver(), srcSpec.freq,
	       srcSpec.channels, dstSpec.freq, dstSpec.channels, deviceSpec.freq, deviceSpec.channels, deviceSampleFrames);
	NativeAudio_ClearOutputQueueNoLock();
	if (!SDL_ResumeAudioStreamDevice(s_audio.output.stream))
	{
		fprintf(stderr, "[CTR Native] SDL audio stream resume failed: %s\n", SDL_GetError());
		SDL_DestroyAudioStream(s_audio.output.stream);
		s_audio.output.stream = NULL;
		s_audio.output.device = 0;
		return 0;
	}

	return 1;
}

void NativeAudio_Shutdown(void)
{
	if (s_audio.output.stream != NULL)
	{
		SDL_DestroyAudioStream(s_audio.output.stream);
		s_audio.output.stream = NULL;
		s_audio.output.device = 0;
	}

	SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

s32 NativeAudio_SpuInit(void)
{
	if (!s_audio.init)
	{
		memset(&s_audio.spu, 0, sizeof(s_audio.spu));
		memset(&s_audio.voices, 0, sizeof(s_audio.voices));
		memset(&s_audio.reverb, 0, sizeof(s_audio.reverb));
		s_audio.cdMixEnabled = 1;
		s_audio.masterVolumeLeft = 0x3fff;
		s_audio.masterVolumeRight = 0x3fff;
		s_audio.commonAttr.mvol.left = 0x3fff;
		s_audio.commonAttr.mvol.right = 0x3fff;
		s_audio.commonAttr.cd.mix = 1;
		s_audio.reverbAttr.mode = SPU_REV_MODE_OFF;
	}

	if (!NativeAudio_OpenDevice())
		return 0;

	s_audio.init = 1;
	return 1;
}

u32 NativeAudio_SpuSetTransferStartAddr(u32 addr)
{
	u32 result;

	if (addr > NATIVE_AUDIO_SPU_MEMSIZE)
		return 0;

	NativeAudio_LockOutput();

	s_audio.spu.transferOffset = (int)addr;
	result = (addr < 0x1010) ? 0 : 1;

	NativeAudio_UnlockOutput();

	return result;
}

u32 NativeAudio_SpuWrite(const u8 *addr, u32 size)
{
	int wptrOfs;

	NativeAudio_LockOutput();

	wptrOfs = s_audio.spu.transferOffset;
	if ((addr == NULL) || (size == 0) || (size > (u32)NATIVE_AUDIO_SPU_MEMSIZE) || (wptrOfs < 0) || (size > (u32)(NATIVE_AUDIO_SPU_MEMSIZE - wptrOfs)))
	{
		NativeAudio_UnlockOutput();
		return 0;
	}

	memcpy(&s_audio.spu.memory[wptrOfs], addr, size);
	NativeAudio_MarkVoiceSamplesDirty();

	NativeAudio_UnlockOutput();

	return size;
}

void NativeAudio_SpuSetVoiceAttr(SpuVoiceAttr *psxAttrib)
{
	int i;

	if (!s_audio.init || psxAttrib == NULL)
		return;

	NativeAudio_LockOutput();

	for (i = 0; i < NATIVE_AUDIO_SPU_VOICE_COUNT; i++)
	{
		struct NativeAudioVoice *voice;

		if ((psxAttrib->voice & SPU_VOICECH(i)) == 0)
			continue;

		voice = &s_audio.voices[i];

		if (psxAttrib->mask & SPU_VOICE_WDSA)
		{
			if (voice->attr.addr != psxAttrib->addr)
				voice->sampleDirty = 1;
			voice->attr.addr = psxAttrib->addr;
		}

		if (psxAttrib->mask & SPU_VOICE_LSAX)
		{
			if (voice->attr.loop_addr != psxAttrib->loop_addr)
				voice->sampleDirty = 1;
			voice->attr.loop_addr = psxAttrib->loop_addr;
		}

		if (psxAttrib->mask & SPU_VOICE_VOLL)
			voice->attr.volume.left = psxAttrib->volume.left;
		if (psxAttrib->mask & SPU_VOICE_VOLR)
			voice->attr.volume.right = psxAttrib->volume.right;
		if (psxAttrib->mask & SPU_VOICE_PITCH)
			voice->attr.pitch = psxAttrib->pitch;
		if (psxAttrib->mask & SPU_VOICE_ADSR_AR)
			voice->attr.ar = psxAttrib->ar;
		if (psxAttrib->mask & SPU_VOICE_ADSR_DR)
			voice->attr.dr = psxAttrib->dr;
		if (psxAttrib->mask & SPU_VOICE_ADSR_SR)
			voice->attr.sr = psxAttrib->sr;
		if (psxAttrib->mask & SPU_VOICE_ADSR_RR)
			voice->attr.rr = psxAttrib->rr;
		if (psxAttrib->mask & SPU_VOICE_ADSR_SL)
			voice->attr.sl = psxAttrib->sl;
		if (psxAttrib->mask & SPU_VOICE_ADSR_AMODE)
			voice->attr.a_mode = psxAttrib->a_mode;
		if (psxAttrib->mask & SPU_VOICE_ADSR_SMODE)
			voice->attr.s_mode = psxAttrib->s_mode;
		if (psxAttrib->mask & SPU_VOICE_ADSR_RMODE)
			voice->attr.r_mode = psxAttrib->r_mode;
		if (psxAttrib->mask & SPU_VOICE_ADSR_ADSR1)
			voice->attr.adsr1 = psxAttrib->adsr1;
		if (psxAttrib->mask & SPU_VOICE_ADSR_ADSR2)
			voice->attr.adsr2 = psxAttrib->adsr2;
		NativeAudio_DecodePackedAdsrToFields(voice, (psxAttrib->mask & SPU_VOICE_ADSR_ADSR1) != 0, (psxAttrib->mask & SPU_VOICE_ADSR_ADSR2) != 0);
		if (psxAttrib->mask & (SPU_VOICE_ADSR_AR | SPU_VOICE_ADSR_DR | SPU_VOICE_ADSR_SR | SPU_VOICE_ADSR_RR | SPU_VOICE_ADSR_SL | SPU_VOICE_ADSR_AMODE |
		                       SPU_VOICE_ADSR_SMODE | SPU_VOICE_ADSR_RMODE | SPU_VOICE_ADSR_ADSR1 | SPU_VOICE_ADSR_ADSR2))
			NativeAudio_UpdatePackedAdsrFromFields(voice);
	}

	NativeAudio_UnlockOutput();
}

void NativeAudio_SpuSetKey(s32 on_off, u32 voice_bit)
{
	int i;

	if (!s_audio.init)
		return;

	NativeAudio_LockOutput();

	for (i = 0; i < NATIVE_AUDIO_SPU_VOICE_COUNT; i++)
	{
		struct NativeAudioVoice *voice;

		if ((voice_bit & SPU_VOICECH(i)) == 0)
			continue;

		voice = &s_audio.voices[i];
		if (on_off && !s_audio.muted)
		{
			if ((voice->pcm == NULL) || voice->sampleDirty)
				NativeAudio_UpdateVoiceSample(voice);

			voice->positionFp = 0;
			voice->looped = 0;
			voice->active = voice->pcm != NULL;
			if (voice->active)
				NativeAudio_AdsrKeyOn(voice);
			else
				NativeAudio_AdsrForceOff(voice);
		}
		else
		{
			if (on_off)
				NativeAudio_AdsrForceOff(voice);
			else
				NativeAudio_AdsrKeyOff(voice);
		}
	}

	NativeAudio_UnlockOutput();
}

s32 NativeAudio_SpuSetReverb(s32 on_off)
{
	int oldState;

	NativeAudio_LockOutput();

	oldState = s_audio.reverbEnabled;
	s_audio.reverbEnabled = on_off != 0;

	NativeAudio_UnlockOutput();

	return oldState;
}

s32 NativeAudio_SpuSetReverbModeParam(SpuReverbAttr *attr)
{
	if (attr == NULL)
		return SPU_INVALID_ARGS;

	NativeAudio_LockOutput();

	if (attr->mask & SPU_REV_MODE)
	{
		s_audio.reverbAttr.mode = attr->mode;
		if (attr->mode != SPU_REV_MODE_CHECK)
			NativeAudio_ReverbConfigureModeNoLock(attr->mode);
	}
	if (attr->mask & SPU_REV_DEPTHL)
		s_audio.reverbAttr.depth.left = attr->depth.left;
	if (attr->mask & SPU_REV_DEPTHR)
		s_audio.reverbAttr.depth.right = attr->depth.right;
	if (attr->mask & SPU_REV_DELAYTIME)
		s_audio.reverbAttr.delay = attr->delay;
	if (attr->mask & SPU_REV_FEEDBACK)
		s_audio.reverbAttr.feedback = attr->feedback;
	s_audio.reverbAttr.mask |= attr->mask;

	NativeAudio_UnlockOutput();

	return SPU_SUCCESS;
}

void NativeAudio_SpuSetReverbModeDepth(s16 left, s16 right)
{
	NativeAudio_LockOutput();

	s_audio.reverbAttr.depth.left = left;
	s_audio.reverbAttr.depth.right = right;
	s_audio.reverbAttr.mask |= SPU_REV_DEPTHL | SPU_REV_DEPTHR;

	NativeAudio_UnlockOutput();
}

u32 NativeAudio_SpuSetReverbVoice(s32 on_off, u32 voice_bit)
{
	int i;

	NativeAudio_LockOutput();

	if (on_off)
		s_audio.reverbVoiceBits |= voice_bit;
	else
		s_audio.reverbVoiceBits &= ~voice_bit;

	for (i = 0; i < NATIVE_AUDIO_SPU_VOICE_COUNT; i++)
	{
		if ((voice_bit & SPU_VOICECH(i)) != 0)
			s_audio.voices[i].reverb = on_off != 0;
	}

	NativeAudio_UnlockOutput();

	return 0;
}

void NativeAudio_SpuSetCommonMasterVolume(s16 left, s16 right)
{
	NativeAudio_LockOutput();

	s_audio.masterVolumeLeft = left;
	s_audio.masterVolumeRight = right;
	s_audio.commonAttr.mvol.left = left;
	s_audio.commonAttr.mvol.right = right;
	s_audio.commonAttr.mask |= SPU_COMMON_MVOLL | SPU_COMMON_MVOLR;

	NativeAudio_UnlockOutput();
}

void NativeAudio_SpuSetCommonCDMix(s32 enabled)
{
	NativeAudio_LockOutput();

	s_audio.cdMixEnabled = enabled != 0;
	s_audio.commonAttr.cd.mix = enabled;
	s_audio.commonAttr.mask |= SPU_COMMON_CDMIX;

	NativeAudio_UnlockOutput();
}

void NativeAudio_SpuSetCommonCDVolume(s16 left, s16 right)
{
	NativeAudio_LockOutput();

	s_audio.xa.volumeLeft = left;
	s_audio.xa.volumeRight = right;
	s_audio.commonAttr.cd.volume.left = left;
	s_audio.commonAttr.cd.volume.right = right;
	s_audio.commonAttr.mask |= SPU_COMMON_CDVOLL | SPU_COMMON_CDVOLR;

	NativeAudio_UnlockOutput();
}

void NativeAudio_SpuSetCommonCDReverb(s32 enabled)
{
	NativeAudio_LockOutput();

	s_audio.cdReverbEnabled = enabled != 0;
	s_audio.commonAttr.cd.reverb = enabled;
	s_audio.commonAttr.mask |= SPU_COMMON_CDREV;

	NativeAudio_UnlockOutput();
}

int NativeAudio_GetXATrackLength(int categoryID, int xaID)
{
	struct NativeAudioXaTrackInfo info;

	if (!NativeAudio_LookupXATrackInfo(categoryID, xaID, &info))
		return 0;

	return info.numSectors;
}

void NativeAudio_StopXA(void)
{
	NativeAudio_LockOutput();

	NativeAudio_FreeXA();

	NativeAudio_UnlockOutput();
}

void NativeAudio_SetXAVolume(int volumeLeft, int volumeRight)
{
	NativeAudio_LockOutput();

	s_audio.xa.volumeLeft = (s16)volumeLeft;
	s_audio.xa.volumeRight = (s16)volumeRight;
	s_audio.commonAttr.cd.volume.left = (s16)volumeLeft;
	s_audio.commonAttr.cd.volume.right = (s16)volumeRight;
	s_audio.commonAttr.mask |= SPU_COMMON_CDVOLL | SPU_COMMON_CDVOLR;

	NativeAudio_UnlockOutput();
}

int NativeAudio_IsXAPlaying(void)
{
	int playing;

	NativeAudio_LockOutput();

	playing = s_audio.xa.active;

	NativeAudio_UnlockOutput();

	return playing;
}

int NativeAudio_PlayXATrack(int categoryID, int xaID, int volumeLeft, int volumeRight)
{
	s16 *pcm;
	int sampleRate;
	int frameCount;

	if (!NativeAudio_SpuInit())
		return 0;

	if (!NativeAudio_LoadXATrackPcm(&s_audio.xaPendingPcmArena, categoryID, xaID, &pcm, &frameCount, &sampleRate))
		return 0;

	NativeAudio_LockOutput();

	NativeAudio_FreeXA();
	NativeAudio_ArenaSwap(&s_audio.xaPcmArena, &s_audio.xaPendingPcmArena);
	s_audio.xa.pcm = pcm;
	s_audio.xa.frameCount = frameCount;
	s_audio.xa.sampleRate = sampleRate;
	s_audio.xa.categoryID = categoryID;
	s_audio.xa.xaID = xaID;
	s_audio.xa.hasTrackIdentity = 1;
	s_audio.xa.positionFp = 0;
	s_audio.xa.outputFrame = 0;
	s_audio.xa.stepFp = (u32)(((u64)sampleRate << NATIVE_AUDIO_FP_SHIFT) / NATIVE_AUDIO_SAMPLE_RATE);
	s_audio.xa.volumeLeft = (s16)volumeLeft;
	s_audio.xa.volumeRight = (s16)volumeRight;
	s_audio.commonAttr.cd.volume.left = (s16)volumeLeft;
	s_audio.commonAttr.cd.volume.right = (s16)volumeRight;
	s_audio.commonAttr.mask |= SPU_COMMON_CDVOLL | SPU_COMMON_CDVOLR;
	s_audio.xa.active = 1;

	NativeAudio_UnlockOutput();

	return 1;
}

int NativeAudio_GetXACurrOffset(void)
{
	u64 outputFrame;
	u64 outputFrameCount;
	int offset;

	NativeAudio_LockOutput();

	if ((s_audio.xa.hasTrackIdentity == 0) || (s_audio.xa.sampleRate <= 0))
	{
		offset = 0;
	}
	else
	{
		// NOTE(aalhendi): Retail XA_CurrOffset advances in decoded SPU blocks,
		// which cutscene lipsync converts with (offset * 30 * 0x100) / 44100.
		outputFrame = s_audio.xa.outputFrame;
		outputFrameCount = NativeAudio_GetXAOutputFrameCount(s_audio.xa.frameCount, s_audio.xa.sampleRate);
		if ((outputFrameCount > 0) && (outputFrame > outputFrameCount))
			outputFrame = outputFrameCount;
		outputFrame >>= 8;
		offset = outputFrame > (u64)INT_MAX ? INT_MAX : (int)outputFrame;
	}

	NativeAudio_UnlockOutput();

	return offset;
}

static int NativeAudio_GetXAMaxSampleAtSourceFrameNoLock(u64 frameIndex)
{
	int max = 0;
	int frame;

	if (s_audio.xa.active && s_audio.xa.pcm != NULL)
	{
		for (frame = 0; frame < 0x80; frame++)
		{
			int left;
			int right;

			if (frameIndex + (u64)frame >= (u64)s_audio.xa.frameCount)
				break;

			left = s_audio.xa.pcm[((size_t)frameIndex + (size_t)frame) * 2];
			right = s_audio.xa.pcm[((size_t)frameIndex + (size_t)frame) * 2 + 1];

			if (left < 0)
				left = -left;
			if (right < 0)
				right = -right;

			if (max < left)
				max = left;
			if (max < right)
				max = right;
		}
	}

	return max;
}

int NativeAudio_GetXAMaxSample(void)
{
	int max;

	NativeAudio_LockOutput();

	max = NativeAudio_GetXAMaxSampleAtSourceFrameNoLock(s_audio.xa.positionFp >> NATIVE_AUDIO_FP_SHIFT);

	NativeAudio_UnlockOutput();

	return max;
}

int NativeAudio_GetXAMaxSampleAtOffset(int xaCurrOffset)
{
	u64 outputFrame;
	u64 sourceFrame;
	int max = 0;

	if (xaCurrOffset < 0)
		return 0;

	NativeAudio_LockOutput();

	if ((s_audio.xa.active != 0) && (s_audio.xa.pcm != NULL) && (s_audio.xa.sampleRate > 0))
	{
		outputFrame = (u64)xaCurrOffset << 8;
		sourceFrame = (outputFrame * (u64)s_audio.xa.sampleRate) / NATIVE_AUDIO_SAMPLE_RATE;
		max = NativeAudio_GetXAMaxSampleAtSourceFrameNoLock(sourceFrame);
	}

	NativeAudio_UnlockOutput();

	return max;
}
