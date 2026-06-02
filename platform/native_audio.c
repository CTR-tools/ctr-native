#include <platform/native_audio.h>

#include "psx/libspu.h"
#include "psx/types.h"

#include <SDL2/SDL.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// TODO(aalhendi): unify defs with a base layer!
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int32_t b32;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define NATIVE_AUDIO_SAMPLE_RATE       44100
#define NATIVE_AUDIO_CHANNELS          2
#define NATIVE_AUDIO_SPU_VOICE_COUNT   24
#define NATIVE_AUDIO_SPU_MEMSIZE       (512 * 1024)
#define NATIVE_AUDIO_FP_SHIFT          16
#define NATIVE_AUDIO_FP_ONE            (1 << NATIVE_AUDIO_FP_SHIFT)
#define NATIVE_AUDIO_GAUSS_INDEX_SHIFT 8
#define NATIVE_AUDIO_DIRECT_VOL_MAX    0x4000
#define NATIVE_AUDIO_VBLANK_FRAMES     (NATIVE_AUDIO_SAMPLE_RATE / 60)
#define NATIVE_AUDIO_RING_FRAMES       (NATIVE_AUDIO_VBLANK_FRAMES * 8)
#define NATIVE_AUDIO_STATE_MAGIC       0x41525443
#define NATIVE_AUDIO_STATE_VERSION     1
#define NATIVE_AUDIO_ARENA_ALIGN       16
#define NATIVE_AUDIO_ADSR_MIN          (-0x8000)
#define NATIVE_AUDIO_ADSR_MAX          0x7fff
#define NATIVE_AUDIO_ADSR_STEP_BIT     0x8000u

#define XA_NUM_TYPES                   3
#define XA_HEADER_SIZE                 0x44
#define XA_NUM_XAS_TOTAL_OFFSET        0x0c
#define XA_NUM_TRACKS_TOTAL_OFFSET     0x10
#define XA_NUM_SONGS_OFFSET            0x2c
#define XA_FIRST_SONG_INDEX_OFFSET     0x38
#define XA_SIZE_ENTRY_BYTES            4
#define XA_FORM2_SECTOR_SIZE           2336
#define XA_FULL_SECTOR_SIZE            2352
#define XA_FRAMES_PER_SECTOR           18
#define XA_FRAME_SIZE                  128
#define XA_SUBHEADER_SIZE              8
#define XA_SAMPLES_PER_SOUND_UNIT      28
#define XA_BLOCKS_PER_FRAME            4
#define XA_SUBFRAMES_PER_FRAME         8
#define XA_SAMPLE_RATE_37800           37800
#define XA_SAMPLE_RATE_18900           18900

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
	// NOTE(aalhendi): Host output queue only; clear it on restore instead of snapshotting it.
	s16 ring[NATIVE_AUDIO_RING_FRAMES * NATIVE_AUDIO_CHANNELS];
	int readFrame;
	int writeFrame;
	int frameCount;
#ifdef CTR_INTERNAL
	int underrunFrames;
	int overflowFrames;
	int reportVBlankCountdown;
	int lastReportedUnderrunFrames;
	int lastReportedOverflowFrames;
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

// TODO(aalhendi): Add fuller PS1 SPU reverb/noise behavior when audible
// evidence shows CTR needs it.

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
	SpuCommonAttr commonAttr;
	struct NativeAudioVoiceState voices[NATIVE_AUDIO_SPU_VOICE_COUNT];
	struct NativeAudioXAState xa;
	u8 spuSampleMem[NATIVE_AUDIO_SPU_MEMSIZE];
};

static struct NativeAudioState s_audio;

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
	return (int)(((int64_t)sample * scaledVolume * scaledMasterVolume) / (NATIVE_AUDIO_DIRECT_VOL_MAX * NATIVE_AUDIO_DIRECT_VOL_MAX));
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
		adsrStep = (int)(((int64_t)adsrStep * voice->adsrLevel) / 0x8000);
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
	return NativeAudio_Clamp16((int)(((int64_t)sample * adsrLevel) / scaleMax));
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
	if (xa->active && ((xa->positionFp >> NATIVE_AUDIO_FP_SHIFT) >= (u64)xa->frameCount))
		return 0;
	return 1;
}

static void NativeAudio_ClearOutputQueueNoLock(void)
{
	memset(s_audio.output.ring, 0, sizeof(s_audio.output.ring));
	s_audio.output.readFrame = 0;
	s_audio.output.writeFrame = 0;
	s_audio.output.frameCount = 0;
}

static int NativeAudio_OpenDevice(void);

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

static void NativeAudio_RingPush(s16 left, s16 right)
{
	if (s_audio.output.frameCount == NATIVE_AUDIO_RING_FRAMES)
	{
		s_audio.output.readFrame = (s_audio.output.readFrame + 1) % NATIVE_AUDIO_RING_FRAMES;
		s_audio.output.frameCount--;
#ifdef CTR_INTERNAL
		if (s_audio.output.overflowFrames < INT_MAX)
			s_audio.output.overflowFrames++;
#endif
	}

	s_audio.output.ring[s_audio.output.writeFrame * 2] = left;
	s_audio.output.ring[s_audio.output.writeFrame * 2 + 1] = right;
	s_audio.output.writeFrame = (s_audio.output.writeFrame + 1) % NATIVE_AUDIO_RING_FRAMES;
	s_audio.output.frameCount++;
}

static void NativeAudio_RingPop(s16 *left, s16 *right)
{
	if (s_audio.output.frameCount <= 0)
	{
		*left = 0;
		*right = 0;
#ifdef CTR_INTERNAL
		if (s_audio.output.underrunFrames < INT_MAX)
			s_audio.output.underrunFrames++;
#endif
		return;
	}

	*left = s_audio.output.ring[s_audio.output.readFrame * 2];
	*right = s_audio.output.ring[s_audio.output.readFrame * 2 + 1];
	s_audio.output.readFrame = (s_audio.output.readFrame + 1) % NATIVE_AUDIO_RING_FRAMES;
	s_audio.output.frameCount--;
}

void NativeAudio_ClearOutputQueue(void)
{
	if (s_audio.output.device != 0)
		SDL_LockAudioDevice(s_audio.output.device);

	NativeAudio_ClearOutputQueueNoLock();

	if (s_audio.output.device != 0)
		SDL_UnlockAudioDevice(s_audio.output.device);
}

#ifdef CTR_INTERNAL
void NativeAudio_GetOutputStats(int *underrunFrames, int *overflowFrames, int *queuedFrames)
{
	if (s_audio.output.device != 0)
		SDL_LockAudioDevice(s_audio.output.device);

	if (underrunFrames != NULL)
		*underrunFrames = s_audio.output.underrunFrames;
	if (overflowFrames != NULL)
		*overflowFrames = s_audio.output.overflowFrames;
	if (queuedFrames != NULL)
		*queuedFrames = s_audio.output.frameCount;

	if (s_audio.output.device != 0)
		SDL_UnlockAudioDevice(s_audio.output.device);
}

static int NativeAudio_ShouldReportOutputStatsNoLock(int *underrunFrames, int *overflowFrames, int *queuedFrames)
{
	if (s_audio.output.reportVBlankCountdown > 0)
	{
		s_audio.output.reportVBlankCountdown--;
		return 0;
	}

	s_audio.output.reportVBlankCountdown = 60;

	if ((s_audio.output.underrunFrames == 0) && (s_audio.output.overflowFrames == 0))
		return 0;

	if ((s_audio.output.underrunFrames == s_audio.output.lastReportedUnderrunFrames) &&
	    (s_audio.output.overflowFrames == s_audio.output.lastReportedOverflowFrames))
	{
		return 0;
	}

	s_audio.output.lastReportedUnderrunFrames = s_audio.output.underrunFrames;
	s_audio.output.lastReportedOverflowFrames = s_audio.output.overflowFrames;
	*underrunFrames = s_audio.output.underrunFrames;
	*overflowFrames = s_audio.output.overflowFrames;
	*queuedFrames = s_audio.output.frameCount;
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

	if (s_audio.output.device != 0)
		SDL_LockAudioDevice(s_audio.output.device);

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
	snapshot->commonAttr = s_audio.commonAttr;
	for (i = 0; i < NATIVE_AUDIO_SPU_VOICE_COUNT; i++)
		NativeAudio_CopyVoiceToState(&snapshot->voices[i], &s_audio.voices[i]);
	NativeAudio_CopyXAToState(&snapshot->xa, &s_audio.xa);
	memcpy(snapshot->spuSampleMem, s_audio.spu.memory, sizeof(snapshot->spuSampleMem));

	if (s_audio.output.device != 0)
		SDL_UnlockAudioDevice(s_audio.output.device);

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

	restoreInit = snapshot->init != 0;
	if (restoreInit && !NativeAudio_OpenDevice())
		return 0;

	if (snapshot->xa.active && snapshot->xa.hasTrackIdentity)
	{
		if (!NativeAudio_LoadXATrackPcm(&s_audio.xaPendingPcmArena, snapshot->xa.categoryID, snapshot->xa.xaID, &xaPcm, &xaFrameCount, &xaSampleRate))
			return 0;
	}

	if (s_audio.output.device != 0)
		SDL_LockAudioDevice(s_audio.output.device);

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
		s_audio.xa.positionFp = snapshot->xa.positionFp;
		s_audio.xa.stepFp = (u32)(((u64)xaSampleRate << NATIVE_AUDIO_FP_SHIFT) / NATIVE_AUDIO_SAMPLE_RATE);
		s_audio.xa.volumeLeft = snapshot->xa.volumeLeft;
		s_audio.xa.volumeRight = snapshot->xa.volumeRight;
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
		s_audio.xa.stepFp = snapshot->xa.stepFp;
		s_audio.xa.volumeLeft = snapshot->xa.volumeLeft;
		s_audio.xa.volumeRight = snapshot->xa.volumeRight;
	}

	NativeAudio_ClearOutputQueueNoLock();

	if (s_audio.output.device != 0)
		SDL_UnlockAudioDevice(s_audio.output.device);

	return 1;
}

static void NativeAudio_MixFrame(s16 *outLeft, s16 *outRight)
{
	int mixLeft = 0;
	int mixRight = 0;
	int i;

	if (s_audio.xa.active && s_audio.xa.pcm != NULL)
	{
		u64 frameIndex = s_audio.xa.positionFp >> NATIVE_AUDIO_FP_SHIFT;

		if (frameIndex >= (u64)s_audio.xa.frameCount)
		{
			s_audio.xa.active = 0;
		}
		else if (!s_audio.cdMixEnabled)
		{
			s_audio.xa.positionFp += s_audio.xa.stepFp;
		}
		else
		{
			s16 srcLeft = s_audio.xa.pcm[(size_t)frameIndex * 2];
			s16 srcRight = s_audio.xa.pcm[(size_t)frameIndex * 2 + 1];
			int left = NativeAudio_ApplyVolume(srcLeft, s_audio.xa.volumeLeft, s_audio.masterVolumeLeft);
			int right = NativeAudio_ApplyVolume(srcRight, s_audio.xa.volumeRight, s_audio.masterVolumeRight);

			NativeAudio_MixSample(&mixLeft, &mixRight, left, right);
			s_audio.xa.positionFp += s_audio.xa.stepFp;
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
			NativeAudio_AdsrAdvance(voice);

			stepFp = ((u32)voice->attr.pitch << NATIVE_AUDIO_FP_SHIFT) / 0x1000u;
			voice->positionFp += stepFp;

			if (voice->loopEnabled && ((voice->positionFp >> NATIVE_AUDIO_FP_SHIFT) >= (u64)voice->loopEnd))
				NativeAudio_WrapVoiceLoop(voice);
		}
	}

	*outLeft = (s16)NativeAudio_Clamp16(mixLeft);
	*outRight = (s16)NativeAudio_Clamp16(mixRight);
}

static void NativeAudio_Callback(void *userdata, u8 *stream, int len)
{
	s16 *out = (s16 *)stream;
	int frameCount = len / ((int)sizeof(s16) * NATIVE_AUDIO_CHANNELS);
	int frame;

	(void)userdata;
	memset(stream, 0, (size_t)len);

	if (!s_audio.init)
		return;

	for (frame = 0; frame < frameCount; frame++)
	{
		NativeAudio_RingPop(&out[frame * 2], &out[frame * 2 + 1]);
	}
}

void NativeAudio_StepVBlank(void)
{
	int frame;
#ifdef CTR_INTERNAL
	int shouldReportStats = 0;
	int underrunFrames = 0;
	int overflowFrames = 0;
	int queuedFrames = 0;
#endif

	if (!s_audio.init)
		return;

	if (s_audio.output.device != 0)
		SDL_LockAudioDevice(s_audio.output.device);

	for (frame = 0; frame < NATIVE_AUDIO_VBLANK_FRAMES; frame++)
	{
		s16 left;
		s16 right;

		NativeAudio_MixFrame(&left, &right);
		NativeAudio_RingPush(left, right);
	}

#ifdef CTR_INTERNAL
	shouldReportStats = NativeAudio_ShouldReportOutputStatsNoLock(&underrunFrames, &overflowFrames, &queuedFrames);
#endif

	if (s_audio.output.device != 0)
		SDL_UnlockAudioDevice(s_audio.output.device);

#ifdef CTR_INTERNAL
	if (shouldReportStats)
	{
		printf("[CTR Native] Audio output stats: underrunFrames=%d overflowFrames=%d queuedFrames=%d\n", underrunFrames, overflowFrames, queuedFrames);
	}
#endif
}

static int NativeAudio_OpenDevice(void)
{
	SDL_AudioSpec want;
	SDL_AudioSpec have;

	if (s_audio.output.device != 0)
		return 1;

	if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
		return 0;

	memset(&want, 0, sizeof(want));
	memset(&have, 0, sizeof(have));
	want.freq = NATIVE_AUDIO_SAMPLE_RATE;
	want.format = AUDIO_S16SYS;
	want.channels = NATIVE_AUDIO_CHANNELS;
	want.samples = 1024;
	want.callback = NativeAudio_Callback;

	s_audio.output.device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
	if (s_audio.output.device == 0)
	{
		fprintf(stderr, "[CTR Native] SDL audio unavailable: %s\n", SDL_GetError());
		return 0;
	}

	if ((have.freq != want.freq) || (have.format != want.format) || (have.channels != want.channels))
	{
		fprintf(stderr, "[CTR Native] SDL audio rejected fixed PCM contract: got %d Hz format 0x%x channels %d\n", have.freq, have.format, have.channels);
		SDL_CloseAudioDevice(s_audio.output.device);
		s_audio.output.device = 0;
		return 0;
	}

	printf("[CTR Native] SDL audio device opened: %d Hz, %d channels\n", have.freq, have.channels);
	SDL_PauseAudioDevice(s_audio.output.device, 0);
	return 1;
}

int PsyX_SPUAL_InitSound(void)
{
	if (!s_audio.init)
	{
		memset(&s_audio.spu, 0, sizeof(s_audio.spu));
		memset(&s_audio.voices, 0, sizeof(s_audio.voices));
		s_audio.cdMixEnabled = 1;
		s_audio.masterVolumeLeft = 0x3fff;
		s_audio.masterVolumeRight = 0x3fff;
		s_audio.commonAttr.mvol.left = 0x3fff;
		s_audio.commonAttr.mvol.right = 0x3fff;
		s_audio.commonAttr.cd.mix = 1;
	}

	if (!NativeAudio_OpenDevice())
		return 0;

	s_audio.init = 1;
	return 1;
}

void PsyX_SPUAL_ShutdownSound(void)
{
	int i;

	if (s_audio.output.device != 0)
	{
		SDL_CloseAudioDevice(s_audio.output.device);
		s_audio.output.device = 0;
	}

	for (i = 0; i < NATIVE_AUDIO_SPU_VOICE_COUNT; i++)
		NativeAudio_FreeVoicePcm(&s_audio.voices[i]);

	NativeAudio_FreeXA();
	NativeAudio_ArenaFree(&s_audio.voicePcmArena);
	NativeAudio_ArenaFree(&s_audio.xaPcmArena);
	NativeAudio_ArenaFree(&s_audio.xaPendingPcmArena);
	memset(&s_audio, 0, sizeof(s_audio));
}

int PsyX_SPUAL_Alloc(int size)
{
	int addr;
	int nextAddr;

	if (size <= 0)
		return -1;

	if (s_audio.output.device != 0)
		SDL_LockAudioDevice(s_audio.output.device);

	addr = s_audio.spu.allocCursor;
	if ((addr < 0) || (addr > NATIVE_AUDIO_SPU_MEMSIZE) || (size > NATIVE_AUDIO_SPU_MEMSIZE - addr))
	{
		if (s_audio.output.device != 0)
			SDL_UnlockAudioDevice(s_audio.output.device);
		return -1;
	}

	nextAddr = addr + size;
	s_audio.spu.allocCursor = nextAddr;

	if (s_audio.output.device != 0)
		SDL_UnlockAudioDevice(s_audio.output.device);

	return addr;
}

int PsyX_SPUAL_AllocWithStartAddr(u_int addr, int size)
{
	u_int nextAddr;

	if (size <= 0)
		return -1;

	if (s_audio.output.device != 0)
		SDL_LockAudioDevice(s_audio.output.device);

	if (addr > NATIVE_AUDIO_SPU_MEMSIZE)
	{
		if (s_audio.output.device != 0)
			SDL_UnlockAudioDevice(s_audio.output.device);
		return -1;
	}

	if (((int)addr < s_audio.spu.allocCursor) || ((u_int)size > ((u_int)NATIVE_AUDIO_SPU_MEMSIZE - addr)))
	{
		if (s_audio.output.device != 0)
			SDL_UnlockAudioDevice(s_audio.output.device);
		return -1;
	}

	nextAddr = addr + (u_int)size;
	s_audio.spu.allocCursor = nextAddr;

	if (s_audio.output.device != 0)
		SDL_UnlockAudioDevice(s_audio.output.device);

	return (int)addr;
}

int PsyX_SPUAL_InitAlloc(int num, char *top)
{
	(void)num;
	(void)top;

	if (s_audio.output.device != 0)
		SDL_LockAudioDevice(s_audio.output.device);

	s_audio.spu.allocCursor = 0;

	if (s_audio.output.device != 0)
		SDL_UnlockAudioDevice(s_audio.output.device);

	return 0;
}

void PsyX_SPUAL_Free(u_int addr)
{
	(void)addr;
}

u_int PsyX_SPUAL_SetTransferStartAddr(u_int addr)
{
	u_int result;

	if (addr > NATIVE_AUDIO_SPU_MEMSIZE)
		return 0;

	if (s_audio.output.device != 0)
		SDL_LockAudioDevice(s_audio.output.device);

	s_audio.spu.transferOffset = (int)addr;
	result = (addr < 0x1010) ? 0 : 1;

	if (s_audio.output.device != 0)
		SDL_UnlockAudioDevice(s_audio.output.device);

	return result;
}

u_int PsyX_SPUAL_Write(u_char *addr, u_int size)
{
	int wptrOfs;

	if (s_audio.output.device != 0)
		SDL_LockAudioDevice(s_audio.output.device);

	wptrOfs = s_audio.spu.transferOffset;
	if ((addr == NULL) || (size == 0) || (size > (u_int)NATIVE_AUDIO_SPU_MEMSIZE) || (wptrOfs < 0) || (size > (u_int)(NATIVE_AUDIO_SPU_MEMSIZE - wptrOfs)))
	{
		if (s_audio.output.device != 0)
			SDL_UnlockAudioDevice(s_audio.output.device);
		return 0;
	}

	memcpy(&s_audio.spu.memory[wptrOfs], addr, size);
	NativeAudio_MarkVoiceSamplesDirty();

	if (s_audio.output.device != 0)
		SDL_UnlockAudioDevice(s_audio.output.device);

	return size;
}

u_int PsyX_SPUAL_Read(u_char *addr, u_int size)
{
	int rptrOfs;

	if (s_audio.output.device != 0)
		SDL_LockAudioDevice(s_audio.output.device);

	rptrOfs = s_audio.spu.transferOffset;
	if ((addr == NULL) || (size == 0) || (size > (u_int)NATIVE_AUDIO_SPU_MEMSIZE) || (rptrOfs < 0) || (size > (u_int)(NATIVE_AUDIO_SPU_MEMSIZE - rptrOfs)))
	{
		if (s_audio.output.device != 0)
			SDL_UnlockAudioDevice(s_audio.output.device);
		return 0;
	}

	memcpy(addr, &s_audio.spu.memory[rptrOfs], size);

	if (s_audio.output.device != 0)
		SDL_UnlockAudioDevice(s_audio.output.device);

	return size;
}

void PsyX_SPUAL_GetVoiceVolume(int vNum, short *volL, short *volR)
{
	if ((vNum < 0) || (vNum >= NATIVE_AUDIO_SPU_VOICE_COUNT))
		return;

	if (s_audio.output.device != 0)
		SDL_LockAudioDevice(s_audio.output.device);

	if (volL != NULL)
		*volL = s_audio.voices[vNum].attr.volume.left;
	if (volR != NULL)
		*volR = s_audio.voices[vNum].attr.volume.right;

	if (s_audio.output.device != 0)
		SDL_UnlockAudioDevice(s_audio.output.device);
}

void PsyX_SPUAL_GetVoicePitch(int vNum, u_short *pitch)
{
	if ((vNum < 0) || (vNum >= NATIVE_AUDIO_SPU_VOICE_COUNT) || (pitch == NULL))
		return;

	if (s_audio.output.device != 0)
		SDL_LockAudioDevice(s_audio.output.device);

	*pitch = s_audio.voices[vNum].attr.pitch;

	if (s_audio.output.device != 0)
		SDL_UnlockAudioDevice(s_audio.output.device);
}

void PsyX_SPUAL_SetVoiceAttr(SpuVoiceAttr *psxAttrib)
{
	int i;

	if (!s_audio.init || psxAttrib == NULL)
		return;

	if (s_audio.output.device != 0)
		SDL_LockAudioDevice(s_audio.output.device);

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

	if (s_audio.output.device != 0)
		SDL_UnlockAudioDevice(s_audio.output.device);
}

void PsyX_SPUAL_SetKey(int on_off, u_int voice_bit)
{
	int i;

	if (!s_audio.init)
		return;

	if (s_audio.output.device != 0)
		SDL_LockAudioDevice(s_audio.output.device);

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

	if (s_audio.output.device != 0)
		SDL_UnlockAudioDevice(s_audio.output.device);
}

int PsyX_SPUAL_GetKeyStatus(u_int voice_bit)
{
	int i;
	b32 active = 0;

	if (s_audio.output.device != 0)
		SDL_LockAudioDevice(s_audio.output.device);

	for (i = 0; i < NATIVE_AUDIO_SPU_VOICE_COUNT; i++)
	{
		if (voice_bit == SPU_VOICECH(i))
		{
			active = s_audio.voices[i].active;
			break;
		}
	}

	if (s_audio.output.device != 0)
		SDL_UnlockAudioDevice(s_audio.output.device);

	return active;
}

void PsyX_SPUAL_GetAllKeysStatus(char *status)
{
	int i;

	if (status == NULL)
		return;

	if (s_audio.output.device != 0)
		SDL_LockAudioDevice(s_audio.output.device);

	for (i = 0; i < NATIVE_AUDIO_SPU_VOICE_COUNT; i++)
		status[i] = s_audio.voices[i].active != 0;

	if (s_audio.output.device != 0)
		SDL_UnlockAudioDevice(s_audio.output.device);
}

int PsyX_SPUAL_SetMute(int on_off)
{
	int oldState;

	if (s_audio.output.device != 0)
		SDL_LockAudioDevice(s_audio.output.device);

	oldState = s_audio.muted;
	s_audio.muted = on_off;

	if (s_audio.output.device != 0)
		SDL_UnlockAudioDevice(s_audio.output.device);

	return oldState;
}

int PsyX_SPUAL_SetReverb(int on_off)
{
	int oldState;

	if (s_audio.output.device != 0)
		SDL_LockAudioDevice(s_audio.output.device);

	oldState = s_audio.reverbEnabled;
	s_audio.reverbEnabled = on_off;

	if (s_audio.output.device != 0)
		SDL_UnlockAudioDevice(s_audio.output.device);

	return oldState;
}

int PsyX_SPUAL_GetReverbState(void)
{
	int state;

	if (s_audio.output.device != 0)
		SDL_LockAudioDevice(s_audio.output.device);

	state = s_audio.reverbEnabled;

	if (s_audio.output.device != 0)
		SDL_UnlockAudioDevice(s_audio.output.device);

	return state;
}

int PsyX_SPUAL_SetReverbModeParam(SpuReverbAttr *attr)
{
	if (attr == NULL)
		return SPU_INVALID_ARGS;

	if (s_audio.output.device != 0)
		SDL_LockAudioDevice(s_audio.output.device);

	if (attr->mask & SPU_REV_MODE)
		s_audio.reverbAttr.mode = attr->mode;
	if (attr->mask & SPU_REV_DEPTHL)
		s_audio.reverbAttr.depth.left = attr->depth.left;
	if (attr->mask & SPU_REV_DEPTHR)
		s_audio.reverbAttr.depth.right = attr->depth.right;
	if (attr->mask & SPU_REV_DELAYTIME)
		s_audio.reverbAttr.delay = attr->delay;
	if (attr->mask & SPU_REV_FEEDBACK)
		s_audio.reverbAttr.feedback = attr->feedback;
	s_audio.reverbAttr.mask |= attr->mask;

	if (s_audio.output.device != 0)
		SDL_UnlockAudioDevice(s_audio.output.device);

	return SPU_SUCCESS;
}

void PsyX_SPUAL_GetReverbModeParam(SpuReverbAttr *attr)
{
	if (attr == NULL)
		return;

	if (s_audio.output.device != 0)
		SDL_LockAudioDevice(s_audio.output.device);

	*attr = s_audio.reverbAttr;

	if (s_audio.output.device != 0)
		SDL_UnlockAudioDevice(s_audio.output.device);
}

int PsyX_SPUAL_SetReverbDepth(SpuReverbAttr *attr)
{
	if (attr == NULL)
		return SPU_INVALID_ARGS;

	if (s_audio.output.device != 0)
		SDL_LockAudioDevice(s_audio.output.device);

	if ((attr->mask == 0) || (attr->mask & SPU_REV_DEPTHL))
		s_audio.reverbAttr.depth.left = attr->depth.left;
	if ((attr->mask == 0) || (attr->mask & SPU_REV_DEPTHR))
		s_audio.reverbAttr.depth.right = attr->depth.right;
	s_audio.reverbAttr.mask |= SPU_REV_DEPTHL | SPU_REV_DEPTHR;

	if (s_audio.output.device != 0)
		SDL_UnlockAudioDevice(s_audio.output.device);

	return SPU_SUCCESS;
}

int PsyX_SPUAL_SetReverbModeType(int mode)
{
	int oldMode;

	if (s_audio.output.device != 0)
		SDL_LockAudioDevice(s_audio.output.device);

	oldMode = s_audio.reverbAttr.mode;
	if (mode != SPU_REV_MODE_CHECK)
	{
		s_audio.reverbAttr.mode = mode;
		s_audio.reverbAttr.mask |= SPU_REV_MODE;
	}

	if (s_audio.output.device != 0)
		SDL_UnlockAudioDevice(s_audio.output.device);

	return oldMode;
}

void PsyX_SPUAL_SetReverbModeDepth(short left, short right)
{
	if (s_audio.output.device != 0)
		SDL_LockAudioDevice(s_audio.output.device);

	s_audio.reverbAttr.depth.left = left;
	s_audio.reverbAttr.depth.right = right;
	s_audio.reverbAttr.mask |= SPU_REV_DEPTHL | SPU_REV_DEPTHR;

	if (s_audio.output.device != 0)
		SDL_UnlockAudioDevice(s_audio.output.device);
}

int PsyX_SPUAL_ReserveReverbWorkArea(int on_off)
{
	int oldState;

	if (s_audio.output.device != 0)
		SDL_LockAudioDevice(s_audio.output.device);

	oldState = s_audio.reverbWorkAreaReserved;
	s_audio.reverbWorkAreaReserved = on_off != 0;

	if (s_audio.output.device != 0)
		SDL_UnlockAudioDevice(s_audio.output.device);

	return oldState;
}

int PsyX_SPUAL_IsReverbWorkAreaReserved(void)
{
	int state;

	if (s_audio.output.device != 0)
		SDL_LockAudioDevice(s_audio.output.device);

	state = s_audio.reverbWorkAreaReserved;

	if (s_audio.output.device != 0)
		SDL_UnlockAudioDevice(s_audio.output.device);

	return state;
}

int PsyX_SPUAL_ClearReverbWorkArea(int mode)
{
	(void)mode;
	return SPU_SUCCESS;
}

u_int PsyX_SPUAL_SetReverbVoice(int on_off, u_int voice_bit)
{
	int i;

	if (s_audio.output.device != 0)
		SDL_LockAudioDevice(s_audio.output.device);

	if (on_off)
		s_audio.reverbVoiceBits |= voice_bit;
	else
		s_audio.reverbVoiceBits &= ~voice_bit;

	for (i = 0; i < NATIVE_AUDIO_SPU_VOICE_COUNT; i++)
	{
		if ((voice_bit & SPU_VOICECH(i)) != 0)
			s_audio.voices[i].reverb = on_off != 0;
	}

	if (s_audio.output.device != 0)
		SDL_UnlockAudioDevice(s_audio.output.device);

	return 0;
}

u_int PsyX_SPUAL_GetReverbVoice(void)
{
	u_int voiceBits;

	if (s_audio.output.device != 0)
		SDL_LockAudioDevice(s_audio.output.device);

	voiceBits = s_audio.reverbVoiceBits;

	if (s_audio.output.device != 0)
		SDL_UnlockAudioDevice(s_audio.output.device);

	return voiceBits;
}

void PsyX_SPUAL_SetCommonAttr(SpuCommonAttr *attr)
{
	if (attr == NULL)
		return;

	if (s_audio.output.device != 0)
		SDL_LockAudioDevice(s_audio.output.device);

	if (attr->mask & SPU_COMMON_MVOLL)
	{
		s_audio.masterVolumeLeft = attr->mvol.left;
		s_audio.commonAttr.mvol.left = attr->mvol.left;
	}
	if (attr->mask & SPU_COMMON_MVOLR)
	{
		s_audio.masterVolumeRight = attr->mvol.right;
		s_audio.commonAttr.mvol.right = attr->mvol.right;
	}
	if (attr->mask & SPU_COMMON_MVOLMODEL)
		s_audio.commonAttr.mvolmode.left = attr->mvolmode.left;
	if (attr->mask & SPU_COMMON_MVOLMODER)
		s_audio.commonAttr.mvolmode.right = attr->mvolmode.right;
	if (attr->mask & SPU_COMMON_RVOLL)
		s_audio.commonAttr.mvolx.left = attr->mvolx.left;
	if (attr->mask & SPU_COMMON_RVOLR)
		s_audio.commonAttr.mvolx.right = attr->mvolx.right;
	if (attr->mask & SPU_COMMON_CDVOLL)
	{
		s_audio.xa.volumeLeft = attr->cd.volume.left;
		s_audio.commonAttr.cd.volume.left = attr->cd.volume.left;
	}
	if (attr->mask & SPU_COMMON_CDVOLR)
	{
		s_audio.xa.volumeRight = attr->cd.volume.right;
		s_audio.commonAttr.cd.volume.right = attr->cd.volume.right;
	}
	if (attr->mask & SPU_COMMON_CDREV)
	{
		s_audio.cdReverbEnabled = attr->cd.reverb != 0;
		s_audio.commonAttr.cd.reverb = attr->cd.reverb;
	}
	if (attr->mask & SPU_COMMON_CDMIX)
	{
		s_audio.cdMixEnabled = attr->cd.mix != 0;
		s_audio.commonAttr.cd.mix = attr->cd.mix;
	}
	if (attr->mask & SPU_COMMON_EXTVOLL)
		s_audio.commonAttr.ext.volume.left = attr->ext.volume.left;
	if (attr->mask & SPU_COMMON_EXTVOLR)
		s_audio.commonAttr.ext.volume.right = attr->ext.volume.right;
	if (attr->mask & SPU_COMMON_EXTREV)
		s_audio.commonAttr.ext.reverb = attr->ext.reverb;
	if (attr->mask & SPU_COMMON_EXTMIX)
		s_audio.commonAttr.ext.mix = attr->ext.mix;
	s_audio.commonAttr.mask |= attr->mask;

	if (s_audio.output.device != 0)
		SDL_UnlockAudioDevice(s_audio.output.device);
}

void PsyX_SPUAL_GetCommonAttr(SpuCommonAttr *attr)
{
	if (attr == NULL)
		return;

	if (s_audio.output.device != 0)
		SDL_LockAudioDevice(s_audio.output.device);

	*attr = s_audio.commonAttr;

	if (s_audio.output.device != 0)
		SDL_UnlockAudioDevice(s_audio.output.device);
}

void PsyX_SPUAL_SetCommonMasterVolume(short left, short right)
{
	if (s_audio.output.device != 0)
		SDL_LockAudioDevice(s_audio.output.device);

	s_audio.masterVolumeLeft = left;
	s_audio.masterVolumeRight = right;
	s_audio.commonAttr.mvol.left = left;
	s_audio.commonAttr.mvol.right = right;
	s_audio.commonAttr.mask |= SPU_COMMON_MVOLL | SPU_COMMON_MVOLR;

	if (s_audio.output.device != 0)
		SDL_UnlockAudioDevice(s_audio.output.device);
}

void PsyX_SPUAL_SetCommonCDMix(int enabled)
{
	if (s_audio.output.device != 0)
		SDL_LockAudioDevice(s_audio.output.device);

	s_audio.cdMixEnabled = enabled != 0;
	s_audio.commonAttr.cd.mix = enabled;
	s_audio.commonAttr.mask |= SPU_COMMON_CDMIX;

	if (s_audio.output.device != 0)
		SDL_UnlockAudioDevice(s_audio.output.device);
}

void PsyX_SPUAL_SetCommonCDVolume(short left, short right)
{
	if (s_audio.output.device != 0)
		SDL_LockAudioDevice(s_audio.output.device);

	s_audio.xa.volumeLeft = left;
	s_audio.xa.volumeRight = right;
	s_audio.commonAttr.cd.volume.left = left;
	s_audio.commonAttr.cd.volume.right = right;
	s_audio.commonAttr.mask |= SPU_COMMON_CDVOLL | SPU_COMMON_CDVOLR;

	if (s_audio.output.device != 0)
		SDL_UnlockAudioDevice(s_audio.output.device);
}

void PsyX_SPUAL_SetCommonCDReverb(int enabled)
{
	if (s_audio.output.device != 0)
		SDL_LockAudioDevice(s_audio.output.device);

	s_audio.cdReverbEnabled = enabled != 0;
	s_audio.commonAttr.cd.reverb = enabled;
	s_audio.commonAttr.mask |= SPU_COMMON_CDREV;

	if (s_audio.output.device != 0)
		SDL_UnlockAudioDevice(s_audio.output.device);
}

int PsyX_SPUAL_GetXATrackLength(int categoryID, int xaID)
{
	struct NativeAudioXaTrackInfo info;

	if (!NativeAudio_LookupXATrackInfo(categoryID, xaID, &info))
		return 0;

	return info.numSectors;
}

void PsyX_SPUAL_StopXA(void)
{
	if (s_audio.output.device != 0)
		SDL_LockAudioDevice(s_audio.output.device);

	NativeAudio_FreeXA();

	if (s_audio.output.device != 0)
		SDL_UnlockAudioDevice(s_audio.output.device);
}

void NativeAudio_SetXAVolume(int volumeLeft, int volumeRight)
{
	if (s_audio.output.device != 0)
		SDL_LockAudioDevice(s_audio.output.device);

	s_audio.xa.volumeLeft = (s16)volumeLeft;
	s_audio.xa.volumeRight = (s16)volumeRight;
	s_audio.commonAttr.cd.volume.left = (s16)volumeLeft;
	s_audio.commonAttr.cd.volume.right = (s16)volumeRight;
	s_audio.commonAttr.mask |= SPU_COMMON_CDVOLL | SPU_COMMON_CDVOLR;

	if (s_audio.output.device != 0)
		SDL_UnlockAudioDevice(s_audio.output.device);
}

int PsyX_SPUAL_IsXAPlaying(void)
{
	int playing;

	if (s_audio.output.device != 0)
		SDL_LockAudioDevice(s_audio.output.device);

	playing = s_audio.xa.active;

	if (s_audio.output.device != 0)
		SDL_UnlockAudioDevice(s_audio.output.device);

	return playing;
}

int PsyX_SPUAL_PlayXATrack(int categoryID, int xaID, int volumeLeft, int volumeRight)
{
	s16 *pcm;
	int sampleRate;
	int frameCount;

	if (!PsyX_SPUAL_InitSound())
		return 0;

	if (!NativeAudio_LoadXATrackPcm(&s_audio.xaPendingPcmArena, categoryID, xaID, &pcm, &frameCount, &sampleRate))
		return 0;

	if (s_audio.output.device != 0)
		SDL_LockAudioDevice(s_audio.output.device);

	NativeAudio_FreeXA();
	NativeAudio_ArenaSwap(&s_audio.xaPcmArena, &s_audio.xaPendingPcmArena);
	s_audio.xa.pcm = pcm;
	s_audio.xa.frameCount = frameCount;
	s_audio.xa.sampleRate = sampleRate;
	s_audio.xa.categoryID = categoryID;
	s_audio.xa.xaID = xaID;
	s_audio.xa.hasTrackIdentity = 1;
	s_audio.xa.positionFp = 0;
	s_audio.xa.stepFp = (u32)(((u64)sampleRate << NATIVE_AUDIO_FP_SHIFT) / NATIVE_AUDIO_SAMPLE_RATE);
	s_audio.xa.volumeLeft = (s16)volumeLeft;
	s_audio.xa.volumeRight = (s16)volumeRight;
	s_audio.commonAttr.cd.volume.left = (s16)volumeLeft;
	s_audio.commonAttr.cd.volume.right = (s16)volumeRight;
	s_audio.commonAttr.mask |= SPU_COMMON_CDVOLL | SPU_COMMON_CDVOLR;
	s_audio.xa.active = 1;

	if (s_audio.output.device != 0)
		SDL_UnlockAudioDevice(s_audio.output.device);

	return 1;
}

int NativeAudio_PlayXATrack(int categoryID, int xaID, int volumeLeft, int volumeRight)
{
	return PsyX_SPUAL_PlayXATrack(categoryID, xaID, volumeLeft, volumeRight);
}

int NativeAudio_GetXATrackLength(int categoryID, int xaID)
{
	return PsyX_SPUAL_GetXATrackLength(categoryID, xaID);
}

int NativeAudio_IsXAPlaying(void)
{
	return PsyX_SPUAL_IsXAPlaying();
}

void NativeAudio_StopXA(void)
{
	PsyX_SPUAL_StopXA();
}
