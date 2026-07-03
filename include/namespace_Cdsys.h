#ifndef CTR_NATIVE_NAMESPACE_CDSYS_H
#define CTR_NATIVE_NAMESPACE_CDSYS_H

enum XA_TYPE
{
	CDSYS_XA_TYPE_MUSIC,
	CDSYS_XA_TYPE_EXTRA,
	CDSYS_XA_TYPE_GAME,

#if BUILD <= SepReview
	CDSYS_XA_TYPE_WARP,
#endif

	CDSYS_XA_NUM_TYPES
};

enum DiscMode
{
	DM_DATA,
	DM_AUDIO
};

enum
{
	XA_IDLE = 0,
	XA_SEEKING = 1,
	XA_STARTING = 2,
	XA_PLAYING = 3,
	XA_FADING = 4,
};
typedef s32 XAState;

enum CdsysConstants
{
	CDSYS_LANGUAGE_ENGLISH = 1,
	CDSYS_LANGUAGE_COUNT = 8,
	CDSYS_LANGUAGE_CODE_OFFSET = 4,
	CDSYS_LANGUAGE_CODE_LENGTH = 3,
	CDSYS_XNF_VERSION_RETAIL = 102,

	CDSYS_CD_MODE_XA_AUDIO = CdlModeSpeed | CdlModeRT | CdlModeSize1 | CdlModeSF,
	CDSYS_XA_VOLUME_SHIFT = 7,
	CDSYS_XA_FADE_VOLUME_STEP = 0x400,

	CDSYS_SPU_DECODED_HALF_SAMPLES = 0x100,
	CDSYS_SPU_DECODED_BUFFER_SAMPLES = 0x200,
	CDSYS_SPU_DECODED_IRQ_ADDR = 0x200,
	CDSYS_XA_MAX_SAMPLE_WINDOW = 3,
};

CTR_STATIC_ASSERT(sizeof(XAState) == 0x4);
CTR_STATIC_ASSERT(XA_IDLE == 0);
CTR_STATIC_ASSERT(XA_FADING == 4);
CTR_STATIC_ASSERT(CDSYS_LANGUAGE_COUNT == 8);
CTR_STATIC_ASSERT(CDSYS_XNF_VERSION_RETAIL == 102);
CTR_STATIC_ASSERT(CDSYS_CD_MODE_XA_AUDIO == 0xe8);
CTR_STATIC_ASSERT(CDSYS_XA_FADE_VOLUME_STEP == 0x400);
CTR_STATIC_ASSERT(CDSYS_SPU_DECODED_IRQ_ADDR == 0x200);
CTR_STATIC_ASSERT(CDSYS_XA_MAX_SAMPLE_WINDOW == 3);

struct XNF
{
	// 0x0
	int magic;

	// 0x4
	int version;

	// 0x8
	int numTypes;

	// 0xC
	int numXAs_total;

	// 0x10
	int numAudioTracks_total;

	// 0x14
	int numXA[CDSYS_XA_NUM_TYPES];

	// 0x24
	int firstXaIndex[CDSYS_XA_NUM_TYPES];

	// 0x34
	int numSongs[CDSYS_XA_NUM_TYPES];

	// 0x44
	int firstSongIndex[CDSYS_XA_NUM_TYPES];

	// 0x54
	// size = numXAs_total
	// int XaCdPos[0];
};
#define XNF_GETXACDPOS(x) (int *)((u32)x + sizeof(struct XNF))

struct XaSize
{
	char XaIndex;
	char XaPrefix;
	s16 XaBytes;
};

struct AudioMeta
{
	// for inserting "Sxx.XA" string numbers
	char stringIndex_char1;
	char stringIndex_char2;

	// filler
	char junk1;
	char junk2;

	// \XA\MUSIC\S01.XA;1
	// \XA\ENG\EXTRA\S05.XA;1
	// \XA\ENG\GAME\S20.XA;1
	char *name;
};

CTR_STATIC_ASSERT(sizeof(struct AudioMeta) == 8);

#endif
