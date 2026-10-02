#ifndef CTR_MATCHING_CDSYS_RETAIL_SYMBOLS_H
#define CTR_MATCHING_CDSYS_RETAIL_SYMBOLS_H

#define GAME_SPU_DECODED_BUFFER      cdsysSpuDecodedBuffer
#define GAME_CD_READY_POSITION       cdsysCdReadyPosition
#define GAME_XA_MAX_SAMPLE_HISTORY   cdsysMaxSampleHistory
#define GAME_XA_CURRENT_OFFSET       cdsysCurrentOffset
#define GAME_XA_IRQ_ADDRESS          cdsysIrqAddress
#define GAME_XA_MAX_SAMPLE           cdsysMaxSample
#define GAME_XA_HISTORY_MAX          cdsysHistoryMax
#define GAME_XA_MAX_SAMPLE_INDEX     cdsysMaxSampleIndex
#define GAME_XA_SAMPLE_COUNT         cdsysSampleCount
#define CDSYS_DISC_ENABLED           cdsysUseDisc
#define CDSYS_FINISHED               cdsysFinished
#define CDSYS_CLEAR_INITIAL_STATUS() (cdsysInitialStatus[0] = 0)
#define CDSYS_MUSIC_VOLUME           (cdsysMusicVolume[0])
#define CDSYS_VOICE_VOLUME           (cdsysVoiceVolume[0])
#define CDSYS_XNF_NAME               cdsysXnfName
#define CDSYS_EXTRA_NAME             cdsysExtraName
#define CDSYS_GAME_NAME              cdsysGameName
#define CDSYS_LANGUAGES              cdsysLanguages
#define CDSYS_AUDIO_META             cdsysAudioMeta
#define CDSYS_XNF_MAGIC              cdsysXnfMagic
#define CDSYS_XNF_MAGIC_ASM_NAME     "sdata_static+84"
// NOTE(aalhendi): This resident access opens the global page before reading
// the loaded header; retain that independently scheduled address lifetime.
#define CDSYS_READ_XNF_MAGIC(headerMagic, expectedMagic, header)                                                                                   \
	do                                                                                                                                             \
	{                                                                                                                                              \
		u32 magicPage;                                                                                                                             \
		CTR_PSX_LOAD_SYMBOL_PAGE(magicPage, CDSYS_XNF_MAGIC_ASM_NAME);                                                                             \
		CTR_PSX_LOAD_WORD_AFTER(headerMagic, ((struct XNF *)(header))->magic, magicPage);                                                          \
		CTR_PSX_LOAD_WORD_FROM_PAGE_AFTER(expectedMagic, magicPage, CDSYS_XNF_MAGIC_ASM_NAME, CTR_ReadU32AlignedLE(CDSYS_XNF_MAGIC), headerMagic); \
	} while (0)

#include "../../retail_bindings.h"

// NOTE(aalhendi): CDSYS opens the absolute slot, unlike the shared GP binding.
#undef GAME_LOAD_IN_PROGRESS
#define GAME_LOAD_IN_PROGRESS (cdsysLoadInProgress[0])

// NOTE(aalhendi): The decoded samples, CD position and history are addressed
// absolutely. Scalar externs below retain the resident small-data addressing
// and are linked to their canonical sData slots by the namespace manifest.
extern s16 cdsysSpuDecodedBuffer[0x400] asm("sdata_static+3516");
extern CdlLOC cdsysCdReadyPosition[4] asm("sdata_static+5964");
extern s32 cdsysMaxSampleHistory[3] asm("sdata_static+5980");
extern s32 cdsysCurrentOffset;
extern s32 cdsysIrqAddress;
extern s32 cdsysMaxSample;
extern s32 cdsysHistoryMax;
extern s32 cdsysMaxSampleIndex;
extern s32 cdsysSampleCount;
extern s32 cdsysUseDisc;
extern s32 cdsysFinished;
// Remaining arrays retain absolute addresses for status, settings and XA data.
extern s16 cdsysInitialStatus[] asm("sdata_static+5564");
extern u8 cdsysMusicVolume[] asm("sdata_static+2124");
extern u8 cdsysVoiceVolume[] asm("sdata_static+2128");
extern s32 cdsysLoadInProgress[] asm("sdata_static+312");
extern char cdsysXnfName[16] asm("data+1628");
extern char cdsysExtraName[24] asm("data+1664");
extern char cdsysGameName[24] asm("data+1688");
extern char *cdsysLanguages[8] asm("data+1712");
extern struct AudioMeta cdsysAudioMeta[3] asm("data+1744");
extern char cdsysXnfMagic[8] asm(CDSYS_XNF_MAGIC_ASM_NAME);

#endif
