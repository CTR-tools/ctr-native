#ifndef CTR_NATIVE_NAMESPACE_HOWL_H
#define CTR_NATIVE_NAMESPACE_HOWL_H

#if 0
// this is a type in libsnd.h
struct SndVolume
{
    u16 left;
    u16 right;
};
#endif

enum
{
	AUDIO_NONE = 0,
	AUDIO_LOADING = 1,
	AUDIO_STOP_ALL = 2,
	AUDIO_ADV_HUB = 5,
	AUDIO_ADV_HUB_WAIT = 6,
	AUDIO_GARAGE_ENTRY = 7,
	AUDIO_GARAGE = 8,
	AUDIO_RACE_INTRO = 9,
	AUDIO_TRAFFIC = 10,
	AUDIO_RACING = 11,
	AUDIO_PRE_LAST_LAP = 12,
	AUDIO_FINAL_LAP = 13,
	AUDIO_POST_LAST_LAP = 14,
	AUDIO_LAST_LAP = 15,
	AUDIO_RACE_END = 16,
};
typedef s16 AudioState;

CTR_STATIC_ASSERT(sizeof(AudioState) == 0x2);
CTR_STATIC_ASSERT(AUDIO_NONE == 0);
CTR_STATIC_ASSERT(AUDIO_RACE_END == 16);

struct VoicelineItem
{
	// 0x0
	struct Item item;

	// 0x8
	s16 voiceID;

	// 0xa
	u8 characterID;

	// 0xb
	u8 secondaryCharacterID;

	// 0xc
	s32 startFrame;
};

enum HowlSfxParam
{
	HOWL_SFX_LR_SHIFT = 0,
	HOWL_SFX_DISTORTION_SHIFT = 8,
	HOWL_SFX_VOLUME_SHIFT = 16,
	HOWL_SFX_ECHO_SHIFT = 24,

	HOWL_SFX_LR_CENTER = 0x80,
	HOWL_SFX_DISTORTION_NONE = 0x80,
	HOWL_SFX_VOLUME_MAX = 0xff,

	HOWL_SFX_CENTER_NO_DISTORTION = (HOWL_SFX_DISTORTION_NONE << HOWL_SFX_DISTORTION_SHIFT) | HOWL_SFX_LR_CENTER,
	HOWL_SFX_DEFAULT_FLAGS = (HOWL_SFX_VOLUME_MAX << HOWL_SFX_VOLUME_SHIFT) | HOWL_SFX_CENTER_NO_DISTORTION,
	HOWL_SFX_ECHO_FLAG = 1 << HOWL_SFX_ECHO_SHIFT,
};

CTR_STATIC_ASSERT(HOWL_SFX_CENTER_NO_DISTORTION == 0x8080);
CTR_STATIC_ASSERT(HOWL_SFX_DEFAULT_FLAGS == 0xff8080);
CTR_STATIC_ASSERT(HOWL_SFX_ECHO_FLAG == 0x1000000);

enum HowlChannelType
{
	HOWL_CHANNEL_TYPE_ENGINE_FX = 0,
	HOWL_CHANNEL_TYPE_OTHER_FX = 1,
	HOWL_CHANNEL_TYPE_MUSIC = 2,
};

enum HowlVolumeType
{
	HOWL_VOLUME_TYPE_FX = 0,
	HOWL_VOLUME_TYPE_MUSIC = 1,
	HOWL_VOLUME_TYPE_VOICE = 2,
};

enum HowlChannelUpdateFlag
{
	HOWL_CHANNEL_UPDATE_OFF = 0x1,
	HOWL_CHANNEL_UPDATE_KEY_ON = 0x2,
	HOWL_CHANNEL_UPDATE_SPU_ADDR = 0x4,
	HOWL_CHANNEL_UPDATE_ADSR = 0x8,
	HOWL_CHANNEL_UPDATE_PITCH = 0x10,
	HOWL_CHANNEL_UPDATE_REVERB = 0x20,
	HOWL_CHANNEL_UPDATE_VOLUME = 0x40,

	HOWL_CHANNEL_UPDATE_ALL_ATTRS =
	    HOWL_CHANNEL_UPDATE_SPU_ADDR | HOWL_CHANNEL_UPDATE_ADSR | HOWL_CHANNEL_UPDATE_PITCH | HOWL_CHANNEL_UPDATE_REVERB | HOWL_CHANNEL_UPDATE_VOLUME,
	HOWL_CHANNEL_UPDATE_DYNAMIC_ATTRS = HOWL_CHANNEL_UPDATE_PITCH | HOWL_CHANNEL_UPDATE_REVERB | HOWL_CHANNEL_UPDATE_VOLUME,
	HOWL_CHANNEL_UPDATE_RESUME = HOWL_CHANNEL_UPDATE_KEY_ON | HOWL_CHANNEL_UPDATE_ALL_ATTRS,
};

CTR_STATIC_ASSERT(HOWL_CHANNEL_TYPE_ENGINE_FX == 0);
CTR_STATIC_ASSERT(HOWL_CHANNEL_TYPE_OTHER_FX == 1);
CTR_STATIC_ASSERT(HOWL_CHANNEL_TYPE_MUSIC == 2);
CTR_STATIC_ASSERT(HOWL_VOLUME_TYPE_FX == 0);
CTR_STATIC_ASSERT(HOWL_VOLUME_TYPE_MUSIC == 1);
CTR_STATIC_ASSERT(HOWL_VOLUME_TYPE_VOICE == 2);
CTR_STATIC_ASSERT(HOWL_CHANNEL_UPDATE_ALL_ATTRS == 0x7c);
CTR_STATIC_ASSERT(HOWL_CHANNEL_UPDATE_DYNAMIC_ATTRS == 0x70);
CTR_STATIC_ASSERT(HOWL_CHANNEL_UPDATE_RESUME == 0x7e);

force_inline u32 HowlSfx_Pack(u32 lr, u32 distortion, u32 volume, u32 echo)
{
	// NOTE(aalhendi): echo is the raw high-byte field. Most callers use 0/1,
	// but 3D quadblock audio passes QUADBLOCK_FLAG_ENGINE_ECHO (0x80).
	return ((lr & 0xff) << HOWL_SFX_LR_SHIFT) | ((distortion & 0xff) << HOWL_SFX_DISTORTION_SHIFT) | ((volume & 0xff) << HOWL_SFX_VOLUME_SHIFT) |
	       ((echo & 0xff) << HOWL_SFX_ECHO_SHIFT);
}

force_inline u32 HowlSfx_LR(u32 flags)
{
	return (flags >> HOWL_SFX_LR_SHIFT) & 0xff;
}

force_inline u32 HowlSfx_Distortion(u32 flags)
{
	return (flags >> HOWL_SFX_DISTORTION_SHIFT) & 0xff;
}

force_inline u32 HowlSfx_Volume(u32 flags)
{
	return (flags >> HOWL_SFX_VOLUME_SHIFT) & 0xff;
}

force_inline u32 HowlSfx_Echo(u32 flags)
{
	return (flags >> HOWL_SFX_ECHO_SHIFT) & 0xff;
}

// Global song indices into howl_songOffsets[].
enum HowlSong
{
	HOWL_SONG_BOSS_RACE = 25, // 0x19, shared by all boss races
	HOWL_SONG_MAIN_MENU = 27,
	HOWL_SONG_ND_CRATE = 28,
	HOWL_SONG_INTRO_RACE = 29,
	HOWL_SONG_OXIDE_END = 30,
	HOWL_SONG_OXIDE_TRUE = 31,
	HOWL_SONG_CREDITS = 32,
};

// Local song indices within a loaded CSEQ song pack. These are relative to
// whatever song pack is currently resident.
enum CseqSong
{
	CSEQ_SONG_LEVEL = 0,
	CSEQ_SONG_AKU = 1,
	CSEQ_SONG_UKA = 2,
};

#ifndef CTR_NATIVE
// from TOMB5, not from psyq
// https://github.com/TOMB5/TOMB5/blob/master/EMULATOR/LIBSPU.H
typedef struct
{
	u32 mask;
	s32 mode;

	// SpuVolume from psn00b headers
	SpuVolume depth; /* reverb depth */

	s32 delay;    /* Delay Time  (ECHO, DELAY only)   */
	s32 feedback; /* Feedback    (ECHO only)          */
} SpuReverbAttr;
#endif

// similar to SndRegisterAttr in psyq libsnd.h
struct ChannelAttr
{
	// 0x0
	void *spuStartAddr;

	// as + dr = ASDR (envelope standard)

	// 0x4
	s16 ad;
	s16 sr;

	// 0x8
	s16 pitch;

	// 0xa
	s16 reverb;

	// 0xc
	s16 audioL;

	// 0xe
	s16 audioR;

	// 0x10 bytes large
};

// similar to SndVoiceStats in psyq libsnd.h
struct ChannelStats
{
	union
	{
		// 0x0
		struct Item item;

		struct
		{
			// 0x0
			struct ChannelStats *next;

			// 0x4
			struct ChannelStats *prev;
		};
	};

	// 0x8
	u8 flags;

	// 0x9
	u8 channelID;

	// 0xa
	// ??? set in "noteon"
	u8 unk1;

	// 0xb
	// Type (0=engineFX,1=otherFX,2=music)
	u8 type;

	// 0xc
	// ??? set in "noteon"
	u8 unk2;

	// 0xd
	// union, either or
	u8 drumIndex_pitchIndex;

	// 0xe
	u8 echo;

	// 0xf
	u8 vol;

	// 0x10
	u8 distort;

	// 0x11
	u8 LR;

	// 0x12
	s16 ad;
	s16 sr;

	// 0x16
	s16 timeLeft;

	// 0x18
	// bitshifted top 2 bytes are "CountSounds"
	int soundID; // from Sound_Play

	// 0x1c
	int startFrame;
};

enum GarageSoundPos
{
	GSP_CENTER = 0,
	GSP_LEFT,
	GSP_RIGHT,
	GSP_GONE,
	GSP_NUM
};

struct GarageFX
{
	// enum GarageSoundPos
	char gsp_curr;
	char gsp_prev;

	// 0x2
	s16 volume;

	// 0x4
	int LR;

	// 0x8
	u32 soundIDCount;

	// 0xC - size of each member
};

struct OtherFX
{
	// 0x0
	u8 flags;
	u8 volume;

	// 0x2
	u16 pitch;

	// 0x4
	u16 spuIndex;

	// 0x6
	u16 duration;

	// 0x8 -- size
};

struct EngineFX
{
	// 0x0
	u8 flags;
	u8 volume;

	// 0x2
	u16 pitch;

	// 0x4
	u16 unk;

	// 0x6
	u16 spuIndex;

	// 0x8 -- size
};

struct HowlHeader
{
	int magic;
	int version;
	int unk1;
	int unk2;

	// 0x10
	int numSpuAddrs;
	int numOtherFX;
	int numEngineFX;
	int numBanks;

	// 0x20
	int numSequences;
	int headerSize;

	// 0x28 -- size
};

// Start of a Cseq Pack,
// contains CseqHeader,
// then SampleInstrument array,
// then ShortSamples array,
// then songs (CseqSongHeader + Seq/Note array)
struct CseqHeader
{
	int songSize;

	// 0x4
	u8 numLongSamples;
	u8 numShortSamples;
	u16 numSongs;
};

struct SampleInstrument
{
	// 0x0
	u8 alwaysOne;
	u8 volume;

	// 0x2
	s16 alwaysZero;

	// 0x4
	// middle C at frequency 60
	s16 basePitch;

	// 0x6
	s16 spuIndex;

	// 0x8
	s16 ad;
	s16 sr;

	// 0xC -- end of struct
};

struct SampleDrums
{
	// 0x0
	u8 alwaysOne;
	u8 volume;

	// 0x2
	s16 pitch;

	// 0x4
	s16 spuIndex;

	// 0x6
	s16 alwaysZero;

	// 0x8 -- end of struct
};

// inside CseqPack (after CseqHeader, Instrument Array, and Drums Array)
struct CseqSongHeader
{
	// 0x0
	u8 unk;
	u8 numSeqs;

	// 0x2
	s16 bpm; // beats per minute

	// 0x4
	s16 tpqn; // ticks per quarter note

	// size of numSeqs
	// each seq is an array of SongNote
	// s16 seqOffsetArr[0];
};
#define SONGHEADER_GETSEQOFFARR(x) ((u32)x + sizeof(struct CseqSongHeader))

// right before first note
struct SongNoteHeader
{
	// instrument or drums
	u8 flags;

	u8 unk;

	// char notes[0];
};
#define NOTEHEADER_GETNOTES(x) ((u32)x + sizeof(struct SongNoteHeader))

struct SongSeq
{
	// pointer in SongPool->CseqSequences
	// stored in global array 800902cc songSeq[NUM_SFX_CHANNELS]

	// 0x0
	// & 1 - playing
	// & 2 - song loops
	// & 4 - instrument or drums
	// & 8 - restart song
	u8 flags;

	// 0x1
	u8 soundID;
	u8 unk;

	// 0x3 (SampleInstrument*)
	u8 instrumentID;

	// 0x4
	u8 reverb;

	// one is curr, one is desired

	// 0x5
	u8 vol_Curr;

	// 0x6
	u8 vol_New;

	// 0x7
	u8 vol_StepRate;

	// one is curr, one is desired

	// 0x8
	u8 distort;

	// 0x9
	u8 LR;

	// 0xA
	u8 unk0A;

	// 0xb
	u8 songPoolIndex;

	// 0xc (time until next note is played)
	int NoteLength;

	// 0x10
	int NoteTimeElapsed;

	// 0x14
	u8 *firstNote;

	// 0x18
	u8 *currNote;

	// 0x1C -- size
};

// 80095D84
// Song in SongPool,
// not a song in HOWL file, need renaming
struct Song
{
	// 0x0
	// & 1 = Playing
	// & 2 = Paused (can be &3 in menus)
	// & 4 = needs to stop
	u8 flags;

	// 0x1
	u8 songPoolIndex;

	// 0x2
	// songID out of all songs in RAM
	u16 id;

	// 0x4
	int songSetActiveBits;

	// 0x8
	// ticks per quarter note
	s16 tpqn;

	// 0xA
	// beats per minute
	s16 bpm;

	// 0xC
	int tempo;

	// 0x10
	int unk10;

	// 0x14
	int timeSpentPlaying;

	// 0x18 = vol_Curr
	u8 vol_Curr;
	u8 vol_New;
	u8 vol_StepRate;

	// 0x1b
	u8 numSequences;

	// 0x1c array of all cseq sequences in song
	struct SongSeq *CseqSequences[0x18];
};

struct SongSet
{
	int numSeqs;
	u8 *ptrSongSetBits;
};

struct SampleBlockHeader
{
	s16 numSamples;

	// s16 spuIndexArr[0];
};
#define SBHEADER_GETARR(x) (s16 *)((u32)x + sizeof(struct SampleBlockHeader))

struct SpuAddrEntry
{
	u16 spuAddr;
	u16 spuSize;
};

struct Bank
{
	// 0x0
	s16 bankID;

	// 0x2
	u16 flags;

	// min and max are ranges used in FUN_80029730,
	// range for what kind of data?

	// could also be offset 0x4 as base index,
	// and offset 0x6 as number of elements

	// 0x4
	u16 min;

	// 0x6
	u16 max;

	// 8 elements of 8-byte struct
};

#if 0
enum VoiceType_SFX
{
	VT_Blasted = 1,
	VT_Spinout = 3,
	VT_Jump = 7,
	// 0xa,
	// 0xb,
	// 0xf,
	VT_Turbo = 0x10,
	VT_Crate = 0x13,
}

// This is XA_Game, move to CDSYS, and the WRONG ORDER
enum VoiceType_XAGAME2
{
	VT_ActiveTaunt1=0,
	VT_ActiveTaunt2=1,
	VT_BigAir1,
	VT_BigAir2,
	VT_Finish1,
	VT_Finish2,
	VT_Finish3,
	VT_LaughTaunt1,
	VT_LaughTaunt2,
	VT_Ouch1,
	VT_Ouch2,
	VT_PassiveTaunt1,
	VT_PassiveTaunt2,
	VT_SneakyTaunt1,
	VT_SneakyTaunt2,
	VT_Spinout1,
	VT_Spinout2,
	VT_Yay1,
	VT_Yay2,
	VT_NUM // 0x13
}
#endif

CTR_STATIC_ASSERT(sizeof(SpuReverbAttr) == 0x14);
CTR_STATIC_ASSERT(sizeof(struct VoicelineItem) == 0x10);
CTR_STATIC_ASSERT(sizeof(struct ChannelAttr) == 0x10);
CTR_STATIC_ASSERT(sizeof(struct ChannelStats) == 0x20);
CTR_STATIC_ASSERT(sizeof(struct SongSeq) == 0x1C);
CTR_STATIC_ASSERT(sizeof(struct Song) == 0x7C);

#endif
