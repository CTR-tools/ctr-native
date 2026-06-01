#include "../PsyX_main.h"

#include "psx/libspu.h"
#include "psx/libetc.h"
#include "psx/libmath.h"
#include "PsyX_SPUAL.h"

#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <vector>
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include <AL/efx.h>

// TODO: implement ADSR

static const char* getALCErrorString(int err)
{
	switch (err)
	{
	case ALC_NO_ERROR:
		return "AL_NO_ERROR";
	case ALC_INVALID_DEVICE:
		return "ALC_INVALID_DEVICE";
	case ALC_INVALID_CONTEXT:
		return "ALC_INVALID_CONTEXT";
	case ALC_INVALID_ENUM:
		return "ALC_INVALID_ENUM";
	case ALC_INVALID_VALUE:
		return "ALC_INVALID_VALUE";
	case ALC_OUT_OF_MEMORY:
		return "ALC_OUT_OF_MEMORY";
	default:
		return "AL_UNKNOWN";
	}
}

static const char* getALErrorString(int err)
{
	switch (err)
	{
	case AL_NO_ERROR:
		return "AL_NO_ERROR";
	case AL_INVALID_NAME:
		return "AL_INVALID_NAME";
	case AL_INVALID_ENUM:
		return "AL_INVALID_ENUM";
	case AL_INVALID_VALUE:
		return "AL_INVALID_VALUE";
	case AL_INVALID_OPERATION:
		return "AL_INVALID_OPERATION";
	case AL_OUT_OF_MEMORY:
		return "AL_OUT_OF_MEMORY";
	default:
		return "AL_UNKNOWN";
	}
}

#define SPU_REALMEMSIZE			(512 * 1024)
#define SPU_MEMSIZE				(2048*1024)		// SPU_REALMEMSIZE

typedef struct
{
	u_char samplemem[SPU_MEMSIZE];
	u_char* writeptr;
} SPUMemory;

static SPUMemory s_SpuMemory;
static SDL_mutex* g_SpuMutex = NULL;
static int g_spuInit = 0;
static int s_spuMallocVal = 0;

typedef struct
{
	SpuVoiceAttr attr;	// .voice is Id of this channel

	ALuint alBuffer;
	ALuint alSource;
	float directLeft;
	float directRight;
	ushort sampledirty;
	ushort reverb;
} SPUALVoice;

const int s_spuVoiceCount = 24;

SPUALVoice	g_SpuVoices[s_spuVoiceCount];
ALCdevice*	g_ALCdevice = NULL;
ALCcontext* g_ALCcontext = NULL;
int			g_SPUMuted = 0;
ALuint		g_ALEffectSlots[2];
int			g_currEffectSlotIdx = 0;
ALuint		g_nAlReverbEffect = 0;
int			g_enableSPUReverb = 0;
int			g_ALEffectsSupported = 0;
static ALuint g_XaSource = 0;
static ALuint g_XaBuffer = 0;

LPALGENEFFECTS alGenEffects = NULL;
LPALDELETEEFFECTS alDeleteEffects = NULL;
LPALEFFECTI alEffecti = NULL;
LPALEFFECTF alEffectf = NULL;
LPALGENAUXILIARYEFFECTSLOTS alGenAuxiliaryEffectSlots = NULL;
LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots = NULL;
LPALAUXILIARYEFFECTSLOTI alAuxiliaryEffectSloti = NULL;

// NOTE(aalhendi): ctr-native local divergence. Upstream PsyCross does not know
// CTR's extracted XNF/XA layout, so native decodes XA assets through OpenAL here
// while retail still uses the PS1 CD-XA path.
static const int XA_NUM_TYPES = 3;
static const int XA_HEADER_SIZE = 0x44;
static const int XA_NUM_XAS_TOTAL_OFFSET = 0x0c;
static const int XA_NUM_TRACKS_TOTAL_OFFSET = 0x10;
static const int XA_NUM_SONGS_OFFSET = 0x2c;
static const int XA_FIRST_SONG_INDEX_OFFSET = 0x38;
static const int XA_SIZE_ENTRY_BYTES = 4;
static const int XA_FORM2_SECTOR_SIZE = 2336;
static const int XA_FULL_SECTOR_SIZE = 2352;
static const int XA_FRAMES_PER_SECTOR = 18;
static const int XA_FRAME_SIZE = 128;
static const int XA_SUBHEADER_SIZE = 8;
static const int XA_SAMPLES_PER_SOUND_UNIT = 28;
static const int XA_BLOCKS_PER_FRAME = 4;
static const int XA_SUBFRAMES_PER_FRAME = 8;
static const int XA_SAMPLE_RATE_37800 = 37800;
static const int XA_SAMPLE_RATE_18900 = 18900;
static const int XA_POS_TABLE[5] = {0, 60, 115, 98, 122};
static const int XA_NEG_TABLE[5] = {0, 0, -52, -55, -60};

struct XaTrackInfo
{
	int fileNumber;
	int channelFilter;
	int numSectors;
};

struct XaDecodeState
{
	int old[2];
	int older[2];
};

static unsigned int ReadLE32(const unsigned char *bytes)
{
	return (unsigned int)bytes[0] | ((unsigned int)bytes[1] << 8) | ((unsigned int)bytes[2] << 16) | ((unsigned int)bytes[3] << 24);
}

static int ReadLE16Signed(const unsigned char *bytes)
{
	return (short)((unsigned short)bytes[0] | ((unsigned short)bytes[1] << 8));
}

static int ReadFileBytes(const char *path, std::vector<unsigned char> &bytes)
{
	FILE *fp = fopen(path, "rb");
	if (!fp)
		return 0;

	if (fseek(fp, 0, SEEK_END) != 0)
	{
		fclose(fp);
		return 0;
	}

	long size = ftell(fp);
	if (size <= 0)
	{
		fclose(fp);
		return 0;
	}

	if (fseek(fp, 0, SEEK_SET) != 0)
	{
		fclose(fp);
		return 0;
	}

	bytes.resize((size_t)size);
	size_t read = fread(bytes.data(), 1, bytes.size(), fp);
	fclose(fp);
	return read == bytes.size();
}

static int BuildXAPath(char *path, size_t pathSize, int categoryID, int fileNumber)
{
	const char *dir = NULL;

	if (categoryID == 0)
		dir = "assets/XA/MUSIC";
	else if (categoryID == 1)
		dir = "assets/XA/ENG/EXTRA";
	else if (categoryID == 2)
		dir = "assets/XA/ENG/GAME";
	else
		return 0;

	return snprintf(path, pathSize, "%s/S%02d.XA", dir, fileNumber) > 0;
}

static int LookupXATrackInfo(int categoryID, int xaID, XaTrackInfo *info)
{
	std::vector<unsigned char> xnf;

	if ((categoryID < 0) || (categoryID >= XA_NUM_TYPES) || (xaID < 0))
		return 0;

	if (!ReadFileBytes("assets/XA/ENG.XNF", xnf))
		return 0;

	if ((int)xnf.size() < XA_HEADER_SIZE)
		return 0;

	if (ReadLE32(&xnf[0]) != 0x464e4958)
		return 0;

	if (ReadLE32(&xnf[4]) != 102)
		return 0;

	if (ReadLE32(&xnf[8]) != XA_NUM_TYPES)
		return 0;

	int numXasTotal = (int)ReadLE32(&xnf[XA_NUM_XAS_TOTAL_OFFSET]);
	int numTracksTotal = (int)ReadLE32(&xnf[XA_NUM_TRACKS_TOTAL_OFFSET]);
	int xaSizeOffset = XA_HEADER_SIZE + numXasTotal * 4;
	int xaSizeEnd = xaSizeOffset + numTracksTotal * XA_SIZE_ENTRY_BYTES;

	if ((numXasTotal < 0) || (numTracksTotal < 0) || (xaSizeEnd < xaSizeOffset) || (xaSizeEnd > (int)xnf.size()))
		return 0;

	int numSongs = (int)ReadLE32(&xnf[XA_NUM_SONGS_OFFSET + categoryID * 4]);
	int firstSongIndex = (int)ReadLE32(&xnf[XA_FIRST_SONG_INDEX_OFFSET + categoryID * 4]);

	if (xaID >= numSongs)
		return 0;

	int entryIndex = firstSongIndex + xaID;
	if ((entryIndex < 0) || (entryIndex >= numTracksTotal))
		return 0;

	const unsigned char *entry = &xnf[xaSizeOffset + entryIndex * XA_SIZE_ENTRY_BYTES];
	info->channelFilter = entry[0];
	info->fileNumber = entry[1];
	info->numSectors = ReadLE16Signed(entry + 2);

	return info->numSectors > 0;
}

static int Clamp16(int value)
{
	if (value > 32767)
		return 32767;
	if (value < -32768)
		return -32768;
	return value;
}

static float ClampGain(float value)
{
	if (value < 0.0f)
		return 0.0f;
	if (value > 1.0f)
		return 1.0f;
	return value;
}

static void GetSpuVoiceDirectMix(SPUALVoice *voice, float *leftScale, float *rightScale, float *sourceGain)
{
	float left = ClampGain((float)voice->attr.volume.left / 16384.0f);
	float right = ClampGain((float)voice->attr.volume.right / 16384.0f);
	float peak = left > right ? left : right;

	if (peak <= 0.0f)
	{
		// NOTE(aalhendi): Keep decoded PCM intact for voices keyed at zero
		// volume, like CTR's engine/turbo loops; later gain updates must be
		// able to fade them in.
		*leftScale = 1.0f;
		*rightScale = 1.0f;
		*sourceGain = 0.0f;
		return;
	}

	*leftScale = left / peak;
	*rightScale = right / peak;
	*sourceGain = peak;
}

static void ConfigureDirectStereoSource(ALuint source)
{
	alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
	alSource3f(source, AL_POSITION, 0.0f, 0.0f, 0.0f);

	if (alIsExtensionPresent("AL_SOFT_source_spatialize"))
	{
		// NOTE(aalhendi): CTR native divergence. Retail SPU/CD audio is a direct
		// left/right mix; OpenAL 3D panning/HRTF makes centered audio
		// headphone-dependent.
		alSourcei(source, AL_SOURCE_SPATIALIZE_SOFT, AL_FALSE);
	}

	if (alIsExtensionPresent("AL_SOFT_direct_channels_remix"))
	{
		alSourcei(source, AL_DIRECT_CHANNELS_SOFT, AL_REMIX_UNMATCHED_SOFT);
	}
	else if (alIsExtensionPresent("AL_SOFT_direct_channels"))
	{
		alSourcei(source, AL_DIRECT_CHANNELS_SOFT, AL_DROP_UNMATCHED_SOFT);
	}
}

static void DecodeXA28Nibbles(const unsigned char *sector, int frameOff, int block, int nibble, int channel, XaDecodeState *state, std::vector<short> &out)
{
	int param = sector[frameOff + 4 + block * 2 + nibble];
	int shift = param & 0xf;
	int weight = (param >> 4) & 0xf;

	if (weight > 4)
		weight = 4;

	int w0 = XA_POS_TABLE[weight];
	int w1 = XA_NEG_TABLE[weight];

	for (int i = 0; i < XA_SAMPLES_PER_SOUND_UNIT; i++)
	{
		unsigned char byte = sector[frameOff + 16 + i * 4 + block];
		unsigned char nib = (nibble == 0) ? (unsigned char)((byte & 0xf) << 4) : (unsigned char)(byte & 0xf0);
		int sample = ((signed char)nib) << 8;

		sample >>= shift;
		sample += (state->old[channel] * w0) >> 6;
		sample += (state->older[channel] * w1) >> 6;
		sample = Clamp16(sample);

		state->older[channel] = state->old[channel];
		state->old[channel] = sample;
		out.push_back((short)sample);
	}
}

static void DecodeXASectorMono(const unsigned char *sector, int sectorBase, XaDecodeState *state, std::vector<short> &out)
{
	for (int frame = 0; frame < XA_FRAMES_PER_SECTOR; frame++)
	{
		int frameOff = sectorBase + XA_SUBHEADER_SIZE + frame * XA_FRAME_SIZE;
		const unsigned char *header = &sector[frameOff + 4];

		for (int su = 0; su < XA_SUBFRAMES_PER_FRAME; su++)
		{
			int paramIndex = (su & 3) | ((su & 4) << 1);
			int param = header[paramIndex];
			int shift = param & 0xf;
			int weight = (param >> 4) & 0xf;

			if (weight > 4)
				weight = 4;

			int w0 = XA_POS_TABLE[weight];
			int w1 = XA_NEG_TABLE[weight];

			for (int i = 0; i < XA_SAMPLES_PER_SOUND_UNIT; i++)
			{
				unsigned char byte = sector[frameOff + 16 + i * 4 + (su >> 1)];
				unsigned char nib = ((su & 1) == 0) ? (unsigned char)((byte & 0xf) << 4) : (unsigned char)(byte & 0xf0);
				int sample = ((signed char)nib) << 8;

				sample >>= shift;
				sample += (state->old[0] * w0) >> 6;
				sample += (state->older[0] * w1) >> 6;
				sample = Clamp16(sample);

				state->older[0] = state->old[0];
				state->old[0] = sample;
				out.push_back((short)sample);
			}
		}
	}
}

static void DecodeXASectorStereo(const unsigned char *sector, int sectorBase, XaDecodeState *state, std::vector<short> &out)
{
	for (int frame = 0; frame < XA_FRAMES_PER_SECTOR; frame++)
	{
		int frameOff = sectorBase + XA_SUBHEADER_SIZE + frame * XA_FRAME_SIZE;

		for (int block = 0; block < XA_BLOCKS_PER_FRAME; block++)
		{
			short left[XA_SAMPLES_PER_SOUND_UNIT];
			short right[XA_SAMPLES_PER_SOUND_UNIT];
			std::vector<short> tmp;

			tmp.reserve(XA_SAMPLES_PER_SOUND_UNIT);
			DecodeXA28Nibbles(sector, frameOff, block, 0, 0, state, tmp);
			for (int i = 0; i < XA_SAMPLES_PER_SOUND_UNIT; i++)
				left[i] = tmp[i];

			tmp.clear();
			DecodeXA28Nibbles(sector, frameOff, block, 1, 1, state, tmp);
			for (int i = 0; i < XA_SAMPLES_PER_SOUND_UNIT; i++)
				right[i] = tmp[i];

			for (int i = 0; i < XA_SAMPLES_PER_SOUND_UNIT; i++)
			{
				out.push_back(left[i]);
				out.push_back(right[i]);
			}
		}
	}
}

static int DecodeXAFile(const unsigned char *bytes, int byteCount, int channelFilter, int maxSectors, std::vector<short> &pcm, int *sampleRate,
                        int *numChannels)
{
	int sectorSize;

	if ((byteCount % XA_FULL_SECTOR_SIZE) == 0)
		sectorSize = XA_FULL_SECTOR_SIZE;
	else if ((byteCount % XA_FORM2_SECTOR_SIZE) == 0)
		sectorSize = XA_FORM2_SECTOR_SIZE;
	else
		return 0;

	int sectorBase = (sectorSize == XA_FULL_SECTOR_SIZE) ? 16 : 0;
	int totalSectors = byteCount / sectorSize;
	int sectorsToScan = maxSectors < totalSectors ? maxSectors : totalSectors;
	XaDecodeState state = {};

	*sampleRate = XA_SAMPLE_RATE_37800;
	*numChannels = 1;

	for (int sector = 0; sector < sectorsToScan; sector++)
	{
		const unsigned char *src = &bytes[sector * sectorSize];
		const unsigned char *header = &src[sectorBase];
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
			DecodeXASectorStereo(src, sectorBase, &state, pcm);
		else
			DecodeXASectorMono(src, sectorBase, &state, pcm);
	}

	return !pcm.empty();
}

static void InitOpenAlEffects()
{
	g_ALEffectsSupported = 0;

	if (!alcIsExtensionPresent(g_ALCdevice, ALC_EXT_EFX_NAME))
	{
		eprintf("PSX SPU effects are NOT supported!\n");
		return;
	}

	alGenEffects = (LPALGENEFFECTS)alGetProcAddress("alGenEffects");
	alDeleteEffects = (LPALDELETEEFFECTS)alGetProcAddress("alDeleteEffects");
	alEffecti = (LPALEFFECTI)alGetProcAddress("alEffecti");
	alEffectf = (LPALEFFECTF)alGetProcAddress("alEffectf");
	alGenAuxiliaryEffectSlots = (LPALGENAUXILIARYEFFECTSLOTS)alGetProcAddress("alGenAuxiliaryEffectSlots");
	alDeleteAuxiliaryEffectSlots = (LPALDELETEAUXILIARYEFFECTSLOTS)alGetProcAddress("alDeleteAuxiliaryEffectSlots");
	alAuxiliaryEffectSloti = (LPALAUXILIARYEFFECTSLOTI)alGetProcAddress("alAuxiliaryEffectSloti");

	int max_sends = 0;
	alcGetIntegerv(g_ALCdevice, ALC_MAX_AUXILIARY_SENDS, 1, &max_sends);

	// make reverb effect slot
	g_currEffectSlotIdx = 0;
	alGenAuxiliaryEffectSlots(1, g_ALEffectSlots);

	// make reverb effect
	alGenEffects(1, &g_nAlReverbEffect);
	alEffecti(g_nAlReverbEffect, AL_EFFECT_TYPE, AL_EFFECT_REVERB);

	// setup defaults of effect
	alEffectf(g_nAlReverbEffect, AL_REVERB_GAIN, 0.45f);
	alEffectf(g_nAlReverbEffect, AL_REVERB_GAINHF, 0.25f);
	alEffectf(g_nAlReverbEffect, AL_REVERB_DECAY_TIME, 2.0f);
	alEffectf(g_nAlReverbEffect, AL_REVERB_DECAY_HFRATIO, 0.9f);
	alEffectf(g_nAlReverbEffect, AL_REVERB_REFLECTIONS_DELAY, 0.08f);
	alEffectf(g_nAlReverbEffect, AL_REVERB_REFLECTIONS_GAIN, 0.2f);
	alEffectf(g_nAlReverbEffect, AL_REVERB_DIFFUSION, 0.9f);
	alEffectf(g_nAlReverbEffect, AL_REVERB_DENSITY, 0.1f);
	alEffectf(g_nAlReverbEffect, AL_REVERB_AIR_ABSORPTION_GAINHF, 0.1f);

	g_ALEffectsSupported = 1;

	eprintf("PSX SPU effects are supported and initialized\n");

	alAuxiliaryEffectSloti(g_ALEffectSlots[g_currEffectSlotIdx], AL_EFFECTSLOT_EFFECT, g_nAlReverbEffect);
}

int PsyX_SPUAL_InitSound()
{
	if (!g_SpuMutex)
		g_SpuMutex = SDL_CreateMutex();

	if (!g_spuInit)
		memset(&s_SpuMemory, 0, sizeof(s_SpuMemory));

	g_spuInit = 1;

	int numDevices, alErr, i;
	const char* devices;
	const char* devStrptr;

	// out_channel_formats snd_outputchannels
	static int al_context_params[] =
	{
		ALC_FREQUENCY, 44100,
		ALC_MAX_AUXILIARY_SENDS, 2,
		0
	};

	if (g_ALCdevice)
		return 1;

	numDevices = 0;

	// Init openAL
	// check devices list

	devStrptr = alcGetString(NULL, ALC_DEVICE_SPECIFIER);
	devices = devStrptr;

	// go through device list (each device terminated with a single NULL, list terminated with double NULL)
	while ((*devStrptr) != '\0')
	{
		eprintinfo("found sound device: %s\n", devStrptr);
		devStrptr += strlen(devStrptr) + 1;
		numDevices++;
	}

	if(numDevices == 0)
		return 0;
	
	g_ALCdevice = alcOpenDevice(NULL);

	alErr = AL_NO_ERROR;

	if (!g_ALCdevice)
	{
		alErr = alcGetError(NULL);
		eprinterr("alcOpenDevice: NULL DEVICE error: %s\n", getALCErrorString(alErr));
		return 0;
	}

#ifndef __EMSCRIPTEN__
	g_ALCcontext = alcCreateContext(g_ALCdevice, al_context_params);
#else
	g_ALCcontext = alcCreateContext(g_ALCdevice, NULL);
#endif

	alErr = alcGetError(g_ALCdevice);
	if (alErr != AL_NO_ERROR)
	{
		eprinterr("alcCreateContext error: %s\n", getALCErrorString(alErr));
		return 0;
	}

	alcMakeContextCurrent(g_ALCcontext);

	alErr = alcGetError(g_ALCdevice);
	if (alErr != AL_NO_ERROR)
	{
		eprinterr("alcMakeContextCurrent error: %s\n", getALCErrorString(alErr));
		return 0;
	}

	// Setup defaults
	alListenerf(AL_GAIN, 1.0f);
	alDistanceModel(AL_NONE);

	// create channels
	for (i = 0; i < s_spuVoiceCount; i++)
	{
		SPUALVoice* voice = &g_SpuVoices[i];
		memset(voice, 0, sizeof(SPUALVoice));

		alGenSources(1, &voice->alSource);
		alGenBuffers(1, &voice->alBuffer);

		alSourcei(voice->alSource, AL_SOURCE_RESAMPLER_SOFT, 2);	// Use cubic resampler
		ConfigureDirectStereoSource(voice->alSource);
	}


	InitOpenAlEffects();

	return 1;
}

void PsyX_SPUAL_ShutdownSound()
{
	g_spuInit = 0;

#ifndef __EMSCRIPTEN__
	if (!g_ALCcontext)
		return;

	PsyX_SPUAL_StopXA();
	if (g_XaSource != 0)
	{
		alDeleteSources(1, &g_XaSource);
		g_XaSource = 0;
	}

	for (int i = 0; i < s_spuVoiceCount; i++)
	{
		SPUALVoice* voice = &g_SpuVoices[i];
		alDeleteSources(1, &voice->alSource);
		alDeleteBuffers(1, &voice->alBuffer);
	}

	if (g_ALEffectsSupported)
	{
		alDeleteEffects(1, &g_nAlReverbEffect);
		g_ALEffectsSupported = AL_NONE;
		alDeleteAuxiliaryEffectSlots(1, g_ALEffectSlots);
	}

	alcDestroyContext(g_ALCcontext);
	alcCloseDevice(g_ALCdevice);

	g_ALCcontext = NULL;
	g_ALCdevice = NULL;
#endif // __EMSCRIPTEN__
}

int PsyX_SPUAL_GetXATrackLength(int categoryID, int xaID)
{
	XaTrackInfo info;

	if (!LookupXATrackInfo(categoryID, xaID, &info))
		return 0;

	return info.numSectors;
}

void PsyX_SPUAL_StopXA(void)
{
	if (g_XaSource != 0)
	{
		alSourceStop(g_XaSource);
		alSourcei(g_XaSource, AL_BUFFER, 0);
	}

	if (g_XaBuffer != 0)
	{
		alDeleteBuffers(1, &g_XaBuffer);
		g_XaBuffer = 0;
	}
}

int PsyX_SPUAL_IsXAPlaying(void)
{
	ALint state = 0;

	if (g_XaSource == 0)
		return 0;

	alGetSourcei(g_XaSource, AL_SOURCE_STATE, &state);
	return state == AL_PLAYING;
}

int PsyX_SPUAL_PlayXATrack(int categoryID, int xaID, int volumeLeft, int volumeRight)
{
	// NOTE(aalhendi): Native host bridge for CTR XA. This intentionally maps
	// retail category/id requests onto extracted assets instead of CD sectors.
	XaTrackInfo info;
	char path[128];
	std::vector<unsigned char> data;
	std::vector<short> pcm;
	int sampleRate;
	int numChannels;

	if (!g_ALCdevice && !PsyX_SPUAL_InitSound())
		return 0;

	if (!LookupXATrackInfo(categoryID, xaID, &info))
		return 0;

	if (!BuildXAPath(path, sizeof(path), categoryID, info.fileNumber))
		return 0;

	if (!ReadFileBytes(path, data))
		return 0;

	if (!DecodeXAFile(data.data(), (int)data.size(), info.channelFilter, info.numSectors, pcm, &sampleRate, &numChannels))
		return 0;

	if (numChannels == 1)
	{
		// NOTE(aalhendi): CTR native divergence. Retail CD-XA is a direct
		// left/right CD mix, not a positional source. Expand mono XA to dual-mono
		// so OpenAL headphone/HRTF output cannot collapse it to one ear.
		std::vector<short> stereoPcm;
		stereoPcm.reserve(pcm.size() * 2);
		for (size_t i = 0; i < pcm.size(); i++)
		{
			stereoPcm.push_back(pcm[i]);
			stereoPcm.push_back(pcm[i]);
		}

		pcm.swap(stereoPcm);
		numChannels = 2;
	}

	SDL_LockMutex(g_SpuMutex);

	if (g_XaSource == 0)
		alGenSources(1, &g_XaSource);

	PsyX_SPUAL_StopXA();
	alGenBuffers(1, &g_XaBuffer);

	ALenum format = (numChannels == 2) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
	alBufferData(g_XaBuffer, format, pcm.data(), (ALsizei)(pcm.size() * sizeof(short)), sampleRate);
	alSourcei(g_XaSource, AL_BUFFER, g_XaBuffer);
	ConfigureDirectStereoSource(g_XaSource);

	int volume = (volumeLeft + volumeRight) / 2;
	float gain = (float)volume / 16384.0f;
	if (gain < 0.0f)
		gain = 0.0f;
	if (gain > 1.0f)
		gain = 1.0f;

	alSourcef(g_XaSource, AL_GAIN, gain);
	alSourcef(g_XaSource, AL_PITCH, 1.0f);
	alSourcePlay(g_XaSource);

	SDL_UnlockMutex(g_SpuMutex);

	return 1;
}

//--------------------------------------------------------------------------------

int PsyX_SPUAL_Alloc(int size)
{
	int addr = s_spuMallocVal;
	s_spuMallocVal += size;

	if (s_spuMallocVal > SPU_MEMSIZE)
		return -1;

	return addr;
}

int PsyX_SPUAL_InitAlloc(int num, char* top)
{
	s_spuMallocVal = 0;
	return 0;
}

void PsyX_SPUAL_Free(u_int addr)
{
	s_spuMallocVal = 0;
}

u_int PsyX_SPUAL_SetTransferStartAddr(u_int addr)
{
	s_SpuMemory.writeptr = s_SpuMemory.samplemem + addr;

	if (addr > SPU_MEMSIZE)
		return 0;

	if (addr < 0x1010)
		return 0;

	return 1;
}

u_int PsyX_SPUAL_Write(u_char* addr, u_int size)
{
	//if (0x7EFF0 < size)
	//	size = 0x7EFF0;

	volatile int wptr_ofs = s_SpuMemory.writeptr - s_SpuMemory.samplemem;

	if (wptr_ofs + size > SPU_REALMEMSIZE)
	{
		eprintf("SPU WARNING: SpuWrite exceeded SPU_REALMEMSIZE (%d > 512k)!\n", wptr_ofs + size);
	}
	assert(size > 0 && wptr_ofs + size < SPU_MEMSIZE);

	// simply copy to the writeptr
	memcpy(s_SpuMemory.writeptr, addr, size);

#if 0 // BANK TEST
	{
		static short waveBuffer[SPU_MEMSIZE];

		ALuint alSource;
		ALuint alBuffer;

		alGenSources(1, &alSource);
		alGenBuffers(1, &alBuffer);

		int loopStart = 0, loopLen = 0;
		int count = decodeSound(addr, size, waveBuffer, &loopStart, &loopLen);

		// update AL buffer
		alBufferData(alBuffer, AL_FORMAT_MONO16, waveBuffer, count * sizeof(short), 11000);

		// set the buffer
		alSourcei(alSource, AL_BUFFER, alBuffer);
		alSourcef(alSource, AL_GAIN, 1.0f);// TODO: panning
		alSourcef(alSource, AL_PITCH, 1);

		alSourcePlay(alSource);
		int status;
		do
		{
			alGetSourcei(alSource, AL_SOURCE_STATE, &status);
		} while (status == AL_PLAYING);

		alSourceStop(alSource);

		alDeleteSources(1, &alSource);
		alDeleteBuffers(1, &alBuffer);
	}
#endif

	return size;
}

u_int PsyX_SPUAL_Read(u_char* addr, u_int size)
{
	volatile int rptr_ofs = s_SpuMemory.writeptr - s_SpuMemory.samplemem;

	if (rptr_ofs + size > SPU_REALMEMSIZE)
	{
		eprintf("SPU WARNING: SpuRead exceeded SPU_REALMEMSIZE (%d > 512k)!\n", rptr_ofs + size);
	}
	assert(size > 0 && rptr_ofs + size < SPU_MEMSIZE);

	// simply copy to the writeptr
	memcpy(addr, s_SpuMemory.writeptr, size);

	return size;
}

// PSX ADPCM coefficients
static const float K0[5] = { 0, 0.9375, 1.796875, 1.53125, 1.90625 };
static const float K1[5] = { 0, 0, -0.8125, -0.859375, -0.9375 };

// PSX ADPCM decoding routine - decodes a single sample
static short vagToPcm(u_char soundParameter, int soundData, float* vagPrev1, float* vagPrev2)
{
	int resultInt = 0;

	float dTmp1 = 0.0;
	float dTmp2 = 0.0;
	float dTmp3 = 0.0;

	if (soundData > 7)
		soundData -= 16;

	dTmp1 = (float)soundData * pow(2, (float)(12 - (soundParameter & 0x0F)));

	dTmp2 = (*vagPrev1) * K0[(soundParameter >> 4) & 0x0F];
	dTmp3 = (*vagPrev2) * K1[(soundParameter >> 4) & 0x0F];

	(*vagPrev2) = (*vagPrev1);
	(*vagPrev1) = dTmp1 + dTmp2 + dTmp3;

	resultInt = (int)round((*vagPrev1));

	if (resultInt > 32767)
		resultInt = 32767;

	if (resultInt < -32768)
		resultInt = -32768;

	return (short)resultInt;
}

typedef enum 
{
	LoopEnd = 1 << 0,		// Jump to repeat address after this block
							// 1 - Copy repeatAddress to currentAddress AFTER this block
							//     set ENDX (TODO: Immediately or after this block?)
							// 0 - Nothing

	Repeat = 1 << 1,		// Takes an effect only with LoopEnd bit set.
							// 1 - Loop normally
							// 0 - Loop and force Release

	LoopStart = 1 << 2,		// Mark current address as the beginning of repeat
							// 1 - Load currentAddress to repeatAddress
							// 0 - Nothing
} ADPCM_FLAGS;


// Main decoding routine - Takes PSX ADPCM formatted audio data and converts it to PCM. It also extracts the looping information if used.
static int decodeSound(u_char* iData, int soundSize, short* oData, int* loopStart, int* loopLength, int breakOnEnd /*= 0*/)
{
	u_char sp;
	u_char flag;
	int sd = 0;
	float vagPrev1 = 0.0;
	float vagPrev2 = 0.0;
	int k = 0;

	int loopStrt = 0, loopEnd = 0;
	int breakOn = -1;

	for (int i = 0; i < soundSize; i++)
	{
		if (i % 16 == 0)
		{
			sp = iData[i];
			flag = iData[i+1];
			i += 2;
		}

		sd = (int)iData[i] & 0xF;
		oData[k++] = vagToPcm(sp, sd, &vagPrev1, &vagPrev2);

		sd = ((int)iData[i] >> 4) & 0xF;
		oData[k++] = vagToPcm(sp, sd, &vagPrev1, &vagPrev2);

		if (breakOnEnd && k == breakOn)
			return k;

		if (breakOn == -1)
		{
			// flags parsed
			if (flag & LoopStart)
			{
				loopStrt = k + 26; // FIXME: is that correct?
			}

			if (flag & LoopEnd)
			{
				loopEnd = k + 26;

				if (flag & Repeat)
				{
					*loopStart = loopStrt;
					*loopLength = loopEnd - loopStrt;
				}

				if (breakOnEnd)
					breakOn = k + 26;
			}
		}
	}

	return soundSize;
}

static void UpdateVoiceSample(SPUALVoice* voice)
{
	static short waveBuffer[SPU_REALMEMSIZE];
	static short stereoBuffer[SPU_REALMEMSIZE * 2];
	int loopStart, loopLen, count;
	ALuint alSource, alBuffer;

	//if (!voice->sampledirty)
	//	return;

	voice->sampledirty = 0;

	alSource = voice->alSource;
	alBuffer = voice->alBuffer;

	if (alSource == AL_NONE)
		return;

	loopStart = 0;
	loopLen = 0;

	count = decodeSound(s_SpuMemory.samplemem + voice->attr.addr, SPU_MEMSIZE - voice->attr.addr, waveBuffer, &loopStart, &loopLen, 1);

	if (count == 0)
		return;

	float leftScale;
	float rightScale;
	float sourceGain;
	GetSpuVoiceDirectMix(voice, &leftScale, &rightScale, &sourceGain);

	for (int i = 0; i < count; i++)
	{
		stereoBuffer[i * 2] = (short)Clamp16((int)roundf((float)waveBuffer[i] * leftScale));
		stereoBuffer[i * 2 + 1] = (short)Clamp16((int)roundf((float)waveBuffer[i] * rightScale));
	}

	voice->directLeft = leftScale;
	voice->directRight = rightScale;

#if 0	// sample test
	{
		ALuint aalSource;
		ALuint aalBuffer;

		alGenSources(1, &aalSource);
		alGenBuffers(1, &aalBuffer);

		// update AL buffer
		alBufferData(aalBuffer, AL_FORMAT_MONO16, waveBuffer, count * sizeof(short), 11000);

		// set the buffer
		alSourcei(aalSource, AL_BUFFER, aalBuffer);
		alSourcef(aalSource, AL_GAIN, 1.0f);// TODO: panning
		alSourcef(aalSource, AL_PITCH, 1);

		alSourcePlay(aalSource);
		int status;
		do
		{
			alGetSourcei(aalSource, AL_SOURCE_STATE, &status);
		} while (status == AL_PLAYING);

		alSourceStop(aalSource);

		alDeleteSources(1, &aalSource);
		alDeleteBuffers(1, &aalBuffer);
	}
#endif

	alSourcei(alSource, AL_BUFFER, 0);
	alBufferData(alBuffer, AL_FORMAT_STEREO16, stereoBuffer, count * 2 * sizeof(short), 44100);
	ConfigureDirectStereoSource(alSource);
	alSourcef(alSource, AL_GAIN, sourceGain);

	if (loopLen > 0)
	{
		loopStart += voice->attr.loop_addr - voice->attr.addr;

		if (loopStart - 54 > 0 && loopStart + loopLen <= count)
		{
			int sampleOffs[] = { loopStart, loopStart + loopLen };
			alBufferiv(alBuffer, AL_LOOP_POINTS_SOFT, sampleOffs);
		}

		alSourcei(alSource, AL_LOOPING, AL_TRUE);
	}
	else
	{
		//int sampleOffs[] = { 0, 0 };
		//alBufferiv(alBuffer, AL_LOOP_POINTS_SOFT, sampleOffs);
		alSourcei(alSource, AL_LOOPING, AL_FALSE);
	}

	// set the buffer
	alSourcei(alSource, AL_BUFFER, alBuffer);
}

int PsyX_SPUAL_SetMute(int on_off)
{
	int old_state = g_SPUMuted;
	g_SPUMuted = on_off;
	return old_state;
}

void PsyX_SPUAL_GetVoiceVolume(int vNum, short* volL, short* volR)
{
	if (volL)
		*volL = g_SpuVoices[vNum].attr.volume.left;

	if (volR)
		*volR = g_SpuVoices[vNum].attr.volume.right;
}

void PsyX_SPUAL_GetVoicePitch(int vNum, u_short* pitch)
{
	*pitch = g_SpuVoices[vNum].attr.pitch;
}

void PsyX_SPUAL_SetVoiceAttr(SpuVoiceAttr* psxAttrib)
{
	if (!g_spuInit)
		return;

	SDL_LockMutex(g_SpuMutex);

	for (int i = 0; i < s_spuVoiceCount; i++)
	{
		if ((psxAttrib->voice & SPU_VOICECH(i)) == 0)
			continue;

		SPUALVoice* voice = &g_SpuVoices[i];
		ALuint alSource = voice->alSource;

		if (alSource == AL_NONE)
			continue;

		// update sample
		if ((psxAttrib->mask & SPU_VOICE_WDSA) || (psxAttrib->mask & SPU_VOICE_LSAX))
		{
			if (psxAttrib->mask & SPU_VOICE_WDSA)
			{
				if (voice->attr.addr != psxAttrib->addr)
					voice->sampledirty++;

				voice->attr.addr = psxAttrib->addr;
			}
			
			if (psxAttrib->mask & SPU_VOICE_LSAX)
			{
				if(voice->attr.loop_addr != psxAttrib->loop_addr)
					voice->sampledirty++;

				voice->attr.loop_addr = psxAttrib->loop_addr;
			}			
		}

		// update volume
		if ((psxAttrib->mask & SPU_VOICE_VOLL) || (psxAttrib->mask & SPU_VOICE_VOLR))
		{
			if (psxAttrib->mask & SPU_VOICE_VOLL)
				voice->attr.volume.left = psxAttrib->volume.left;

			if (psxAttrib->mask & SPU_VOICE_VOLR)
				voice->attr.volume.right = psxAttrib->volume.right;

			float leftScale;
			float rightScale;
			float sourceGain;
			GetSpuVoiceDirectMix(voice, &leftScale, &rightScale, &sourceGain);

			if ((fabsf(leftScale - voice->directLeft) > 0.001f) || (fabsf(rightScale - voice->directRight) > 0.001f))
				voice->sampledirty++;

			alSourcef(alSource, AL_GAIN, sourceGain);
		}

		// update pitch
		if (psxAttrib->mask & SPU_VOICE_PITCH)
		{
			ALint state;
			alGetSourcei(alSource, AL_SOURCE_STATE, &state);

			if (psxAttrib->pitch == 0 && state == AL_PLAYING)
				alSourcePause(alSource);
			else if (voice->attr.pitch == 0 && state == AL_PAUSED)
				alSourcePlay(alSource);

			voice->attr.pitch = psxAttrib->pitch;

			const float pitch = (float)(voice->attr.pitch) / 4096.0f;
			alSourcef(alSource, AL_PITCH, pitch);
		}
		
		// TODO: ADSR and other stuff
	}
	SDL_UnlockMutex(g_SpuMutex);
}

void PsyX_SPUAL_SetKey(int on_off, u_int voice_bit)
{
	if (!g_spuInit)
		return;

	SDL_LockMutex(g_SpuMutex);
	for (int i = 0; i < s_spuVoiceCount; i++)
	{
		if ((voice_bit & SPU_VOICECH(i)) == 0)
			continue;

		SPUALVoice* voice = &g_SpuVoices[i];
		ALuint alSource = voice->alSource;

		if (alSource == AL_NONE)
			continue;

		if (on_off && !g_SPUMuted)
		{
			alSourceStop(alSource);
			UpdateVoiceSample(voice);

			alSourcePlay(alSource);
		}
		else
		{
			alSourceStop(alSource);
		}
	}
	SDL_UnlockMutex(g_SpuMutex);
}

int PsyX_SPUAL_GetKeyStatus(u_int voice_bit)
{
	int state = AL_STOPPED;
	SDL_LockMutex(g_SpuMutex);

	for (int i = 0; i < s_spuVoiceCount; i++)
	{
		if (voice_bit != SPU_VOICECH(i))
			continue;

		SPUALVoice* voice = &g_SpuVoices[i];
		ALuint alSource = voice->alSource;

		if (alSource == AL_NONE)
			break; // SpuOff?

		alGetSourcei(alSource, AL_SOURCE_STATE, &state);
		break;
	}

	SDL_UnlockMutex(g_SpuMutex);

	return (state == AL_PLAYING);	// simple as this?
}

void PsyX_SPUAL_GetAllKeysStatus(char* status)
{
	SDL_LockMutex(g_SpuMutex);
	for (int i = 0; i < s_spuVoiceCount; i++)
	{
		SPUALVoice* voice = &g_SpuVoices[i];
		ALuint alSource = voice->alSource;
		if (alSource == AL_NONE)
		{
			status[i] = 0; // SpuOff?
			continue;
		}

		int state;
		alGetSourcei(alSource, AL_SOURCE_STATE, &state);
		status[i] = (state == AL_PLAYING);
	}
	SDL_UnlockMutex(g_SpuMutex);
}

int PsyX_SPUAL_SetReverb(int on_off)
{
	int old_state = g_enableSPUReverb;
	g_enableSPUReverb = on_off;

	if (!g_spuInit)
		return old_state;

	// switch if needed
	if (g_ALEffectsSupported && old_state != g_enableSPUReverb)
	{
		if (g_enableSPUReverb)
		{
			alAuxiliaryEffectSloti(g_ALEffectSlots[g_currEffectSlotIdx], AL_EFFECTSLOT_EFFECT, g_nAlReverbEffect);
		}
		else
		{
			g_currEffectSlotIdx = 0;
			alAuxiliaryEffectSloti(g_ALEffectSlots[0], AL_EFFECTSLOT_EFFECT, AL_EFFECT_NULL);
			alAuxiliaryEffectSloti(g_ALEffectSlots[1], AL_EFFECTSLOT_EFFECT, AL_EFFECT_NULL);
		}
	}

	return old_state;
}

int PsyX_SPUAL_GetReverbState()
{
	return g_enableSPUReverb;
}

u_int PsyX_SPUAL_SetReverbVoice(int on_off, u_int voice_bit)
{
	if (!g_spuInit)
		return 0;

	if (!g_ALEffectsSupported)
		return 0;

	SDL_LockMutex(g_SpuMutex);

	for (int i = 0; i < s_spuVoiceCount; i++)
	{
		if ((voice_bit & SPU_VOICECH(i)) == 0)
			continue;

		SPUALVoice* voice = &g_SpuVoices[i];
		ALuint alSource = voice->alSource;
		if (alSource == AL_NONE)
			continue;

		voice->reverb = on_off > 0;
		if (on_off)
			alSource3i(alSource, AL_AUXILIARY_SEND_FILTER, g_ALEffectSlots[g_currEffectSlotIdx], 0, AL_FILTER_NULL);
		else
			alSource3i(alSource, AL_AUXILIARY_SEND_FILTER, AL_EFFECTSLOT_NULL, 0, AL_FILTER_NULL);
	}

	SDL_UnlockMutex(g_SpuMutex);

	return 0;
}

u_int PsyX_SPUAL_GetReverbVoice()
{
	u_int bits = 0;
	for (int i = 0; i < s_spuVoiceCount; i++)
	{
		SPUALVoice* voice = &g_SpuVoices[i];
		if (voice->reverb)
			bits |= SPU_KEYCH(i);
	}
	return bits;
}
