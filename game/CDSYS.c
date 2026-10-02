#include <common.h>

// NOTE(aalhendi): CD/SPU callbacks communicate through full-width shared slots.
#define CDSYS_CALLBACK_STATE ((volatile struct sData *)sdata)

#ifndef CDSYS_DISC_ENABLED
#define CDSYS_DISC_ENABLED (sdata->boolUseDisc)
#endif
#ifndef CDSYS_FINISHED
#define CDSYS_FINISHED (sdata->XA_boolFinished)
#endif
#ifndef CDSYS_CLEAR_INITIAL_STATUS
#define CDSYS_CLEAR_INITIAL_STATUS() CTR_WriteU16LE(&sdata->unused400[0], 0)
#endif
#ifndef CDSYS_MUSIC_VOLUME
#define CDSYS_MUSIC_VOLUME (sdata->vol_Music)
#endif
#ifndef CDSYS_VOICE_VOLUME
#define CDSYS_VOICE_VOLUME (sdata->vol_Voice)
#endif
#ifndef CDSYS_XNF_NAME
#define CDSYS_XNF_NAME   (data.s_XA_ENG_XNF)
#define CDSYS_EXTRA_NAME (data.s_XA_ENG_EXTRA)
#define CDSYS_GAME_NAME  (data.s_XA_ENG_GAME)
#define CDSYS_LANGUAGES  (data.xaLanguagePtrs)
#define CDSYS_AUDIO_META (data.audioMeta)
#define CDSYS_XNF_MAGIC  (sdata->s_XINF)
#endif
#ifndef CDSYS_READ_XNF_MAGIC
#define CDSYS_READ_XNF_MAGIC(headerMagic, expectedMagic, header) \
	do                                                           \
	{                                                            \
		(headerMagic) = CTR_ReadU32AlignedLE(header);            \
		(expectedMagic) = CTR_ReadU32AlignedLE(CDSYS_XNF_MAGIC); \
	} while (0)
#endif

#if defined(CTR_NATIVE)
#include <platform/native_audio.h>
#endif

b32 CDSYS_Init(b32 boolUseDisc)
{
	register b32 result CTR_PSX_REGISTER("$2");
	sdata->boolUseDisc = boolUseDisc;

	CDSYS_CLEAR_INITIAL_STATUS();

	// if using parallel port (Naughty Dog Devs only)
	if (boolUseDisc != 0)
	{
		// if Cd does not initialize
		if (CdInit() == 0)
		{
			result = 0;
			CTR_PSX_OBSERVE_VALUE(result);
			// use parallel port (Naughty Dog Devs only)
			sdata->boolUseDisc = 0;
			CTR_PSX_MEMORY_BARRIER();
			return result;
		}

		CdSetDebug(1);
	}

	sdata->discMode = -1;
	sdata->bool_XnfLoaded = 0;

	sdata->XA_State = XA_IDLE;
	sdata->XA_boolFinished = 0;

	sdata->countFail_CdSyncCallback = 0;
	sdata->countPass_CdReadyCallback = 0;
	sdata->countFail_CdReadyCallback = 0;
	sdata->XA_CurrOffset = 0;
	sdata->countPass_CdTransferCallback = 0;

	sdata->XA_Playing_Category = 0;
	sdata->XA_Playing_Index = 0;

	sdata->XA_StartPos = 0;
	sdata->XA_EndPos = 0;

	sdata->XA_VolumeBitshift = 0;
	sdata->XA_VolumeDeduct = 0;

	sdata->XA_MaxSampleVal = 0;
	sdata->XA_MaxSampleValInArr = 0;

	sdata->irqAddr = 0;

	sdata->unused_8008d700 = 0;

	sdata->XA_CurrPos = 0;
	sdata->XA_PauseFrame = 0;
	CTR_PSX_MEMORY_BARRIER();

	CDSYS_SetMode_StreamData();

	CDSYS_SetXAToLang(CDSYS_LANGUAGE_ENGLISH);

	Voiceline_PoolClear();

	return true;
}


u32 CDSYS_GetFilePosInt(char *fileString, int *filePos)
{
	CdlFILE cdlFile;
	u32 result;

	if (CdSearchFile(&cdlFile, fileString) != 0)
	{
		*filePos = CdPosToInt(&cdlFile.pos);
		result = 1;
	}
	else
	{
		CTR_PSX_ZERO_VALUE(result);
	}
	return result;
}


void CDSYS_SetMode_StreamData()
{
	u8 buf[8];
	s32 xnfLoaded;

#if defined(CTR_NATIVE)
	// NOTE(aalhendi): Native has no disc-mode switch, but retail force-stops
	// XA on every data-mode entry via CDSYS_XAPauseForce when XNF is loaded.
	// Mirror that safety net so any caller expecting XA to be stopped before
	// data reads begin keeps working. Guarded by XA_State so early-boot
	// callers (CDSYS_Init, bigfile load, HOWL load) that run before gGT or
	// any XA playback exists are no-ops.
	if (sdata->XA_State != XA_IDLE)
	{
		CDSYS_XAPauseForce();
	}
	return;
#endif

	// quit if using parallel
	if (CDSYS_DISC_ENABLED == 0)
	{
		return;
	}

	if (sdata->discMode == DM_DATA)
	{
		return;
	}

	// if XAs "might" be in play, cause XNF loaded
	if (sdata->bool_XnfLoaded != 0)
	{
		// force stop, and cancel callbacks
		CDSYS_XAPauseForce();
	}

	// https://www.cybdyn-systems.com.au/forum/viewtopic.php?t=1956
	// CdControl('\x0e',local_10,(u8 *)0x0);
	// param_1: 0xe = CdlSetmode
	// param_2: 0x80 = Speed mode (separate modes exist)
	// param_3: 0 = normal speed, 1 = double speed

	// Set Mode to Data
	buf[0] = CdlModeSpeed;
	CdControl(CdlSetmode, buf, 0);

	xnfLoaded = sdata->bool_XnfLoaded;
	CDSYS_CALLBACK_STATE->XA_State = XA_IDLE;
	CDSYS_CALLBACK_STATE->discMode = DM_DATA;

	if (xnfLoaded != 0)
	{
		CdSyncCallback(0);
		CdReadyCallback(0);
	}
}


void CDSYS_SetMode_StreamAudio()
{
	u8 buf[8];

	if (CDSYS_DISC_ENABLED == 0)
	{
		return;
	}

	if (sdata->bool_XnfLoaded == 0)
	{
		return;
	}

	if (sdata->discMode == DM_AUDIO)
	{
		return;
	}

	// https://www.cybdyn-systems.com.au/forum/viewtopic.php?t=1956
	// CdControl('\x0e',local_10,(u8 *)0x0);
	// param_1: 0xe = CdlSetmode
	// param_2: 0xE8 = set speed, play ADPCM, set sector
	// param_3: 0 = normal speed, 1 = double speed

	// Set Mode to Audio
	buf[0] = CDSYS_CD_MODE_XA_AUDIO;
	CdControl(CdlSetmode, buf, 0);

	sdata->discMode = DM_AUDIO;
	CDSYS_CALLBACK_STATE->XA_State = XA_IDLE;

	CdSyncCallback(CDSYS_XaCallbackCdSync);
	CdReadyCallback(CDSYS_XaCallbackCdReady);
}


int CDSYS_SetXAToLang(int lang)
{
	char **xaLang;
	register char **languageTable CTR_PSX_REGISTER("$2");
	char *languageCode;
	register char *destination CTR_PSX_REGISTER("$4");
	s32 fileSize;
	u8 *cursor;
	s32 value;
	register s32 numXAs CTR_PSX_REGISTER("$3");
	register s32 expectedTypes CTR_PSX_REGISTER("$2");
	s32 expectedMagic;
	s32 numTypes;
	s32 categoryID;
	register s32 categoryOffset CTR_PSX_REGISTER("$18");
	struct AudioMeta *am;

	if (CDSYS_DISC_ENABLED == 0)
	{
		return 1;
	}
	if (lang >= CDSYS_LANGUAGE_COUNT)
	{
		goto failed;
	}

	sdata->bool_XnfLoaded = 0;
	CTR_PSX_MEMORY_BARRIER();
	CDSYS_SetMode_StreamData();

	languageCode = &CDSYS_XNF_NAME[CDSYS_LANGUAGE_CODE_OFFSET];
	destination = languageCode;
	CTR_PSX_OBSERVE_VALUE(destination);
	languageTable = CDSYS_LANGUAGES;
	CTR_PSX_DEPEND_VALUE(lang, languageTable);
	xaLang = &languageTable[lang];
	strncpy(destination, *xaLang, CDSYS_LANGUAGE_CODE_LENGTH);
	strncpy(&CDSYS_EXTRA_NAME[CDSYS_LANGUAGE_CODE_OFFSET], *xaLang, CDSYS_LANGUAGE_CODE_LENGTH);
	strncpy(&CDSYS_GAME_NAME[CDSYS_LANGUAGE_CODE_OFFSET], *xaLang, CDSYS_LANGUAGE_CODE_LENGTH);

	cursor = LOAD_XnfFile(languageCode - CDSYS_LANGUAGE_CODE_OFFSET, 0, &fileSize);

	// read error
	if (cursor == 0)
	{
		return 0;
	}

	// NOTE(aalhendi): XNF is serialized data. Advance a byte cursor by the
	// canonical member sizes instead of indexing across distinct scalar fields.
	CDSYS_READ_XNF_MAGIC(value, expectedMagic, cursor);
	cursor += sizeof(((struct XNF *)0)->magic);
	if (value != expectedMagic)
	{
		goto failed;
	}
	CTR_PSX_KEEP_VALUE_RELAXED(cursor);

	value = CTR_ReadU32AlignedLE(cursor);
	cursor += sizeof(((struct XNF *)0)->version);
	if (value != CDSYS_XNF_VERSION_RETAIL)
	{
		goto failed;
	}
	CTR_PSX_KEEP_VALUE_RELAXED(cursor);

	value = CTR_ReadU32AlignedLE(cursor);
	// NOTE(aalhendi): Keep the comparison temporary independent of the loaded header word.
	CTR_PSX_BIND_VALUE_CLOBBER(value, "$2");
	expectedTypes = CDSYS_XA_NUM_TYPES;
	CTR_PSX_OBSERVE_VALUE(expectedTypes);
	sdata->xa_numTypes = value;
	CTR_PSX_OBSERVE_MEMORY(sdata->xa_numTypes);
	cursor += sizeof(((struct XNF *)0)->numTypes);
	if (value != expectedTypes)
	{
	failed:
		CTR_PSX_MEMORY_BARRIER();
		return 0;
	}
	CTR_PSX_KEEP_VALUE_RELAXED(cursor);

	numXAs = CTR_ReadU32AlignedLE(cursor);
	cursor += sizeof(((struct XNF *)0)->numXAs_total) + sizeof(((struct XNF *)0)->numAudioTracks_total);
	CDSYS_CALLBACK_STATE->ptrArray_NumXAs = (s32 *)cursor;
	cursor += sizeof(((struct XNF *)0)->numXA);
	CDSYS_CALLBACK_STATE->ptrArray_firstXaIndex = (s32 *)cursor;
	cursor += sizeof(((struct XNF *)0)->firstXaIndex);
	CDSYS_CALLBACK_STATE->ptrArray_numSongs = (s32 *)cursor;
	cursor += sizeof(((struct XNF *)0)->numSongs);
	CDSYS_CALLBACK_STATE->ptrArray_firstSongIndex = (s32 *)cursor;
	cursor += sizeof(((struct XNF *)0)->firstSongIndex);
	categoryID = 0;
	// The logical category and its byte offset advance independently through
	// the per-category word tables and the AudioMeta array.
	CTR_PSX_COPY_VALUE(categoryOffset, categoryID);
	am = CDSYS_AUDIO_META;
	CDSYS_CALLBACK_STATE->ptrArray_XaCdPos = (s32 *)cursor;
	cursor += numXAs * sizeof(s32);
	CTR_PSX_KEEP_VALUE_RELAXED(cursor);
	CDSYS_CALLBACK_STATE->ptrArray_XaSize = (struct XaSize *)cursor;

	do
	{
		s32 xaID;
		s32 count;
		register s32 *categoryEntry CTR_PSX_REGISTER("$2");
		CTR_PSX_KEEP_VALUE_RELAXED(am);
		categoryEntry = sdata->ptrArray_NumXAs;
		CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(categoryEntry, categoryEntry, categoryOffset);
		count = *categoryEntry;

		for (xaID = 0; xaID < count;)
		{
			s32 firstXaIndex;
			s32 found;
			register s32 *returnPtr_xaCdPos CTR_PSX_REGISTER("$3");
			am->name[(u8)am->stringIndex_char1] = '0' + (xaID / 10);
			am->name[(u8)am->stringIndex_char2] = '0' + (xaID % 10);

			categoryEntry = sdata->ptrArray_firstXaIndex;
			destination = am->name;
			CTR_PSX_DEPEND_VALUE(categoryEntry, destination);
			CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(categoryEntry, categoryEntry, categoryOffset);
			firstXaIndex = *categoryEntry;
			CTR_PSX_LOAD_WORD_AFTER(returnPtr_xaCdPos, sdata->ptrArray_XaCdPos, firstXaIndex);
			returnPtr_xaCdPos += firstXaIndex;
			CTR_PSX_KEEP_VALUE_RELAXED(returnPtr_xaCdPos);
			returnPtr_xaCdPos += xaID;

			// quit on error to find XA file
			found = CDSYS_GetFilePosInt(destination, returnPtr_xaCdPos);
			CTR_PSX_OBSERVE_VALUE(xaID);
			xaID++;
			if (found == 0)
			{
				goto failed;
			}
			categoryEntry = sdata->ptrArray_NumXAs;
			CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(categoryEntry, categoryEntry, categoryOffset);
			count = *categoryEntry;
		}
		categoryOffset += sizeof(s32);
		CTR_PSX_LOAD_WORD_AFTER(numTypes, sdata->xa_numTypes, categoryOffset);
		CTR_PSX_OBSERVE_VALUE(numTypes);
		categoryID++;
		am++;
	} while (categoryID < numTypes);

	sdata->bool_XnfLoaded = 1;
	return 1;
}


// The unused argument is required by CdlCB.
void CDSYS_XaCallbackCdSync(u8 result, u8 *unk)
{
	u32 com;
	(void)unk;

	if (result == CdlComplete)
	{
		// determine CdlSeekL or CdlSeekP
		com = CdLastCom() - 0x15;

		if (com < 2)
		{
			sdata->XA_State = XA_IDLE;
			// NOTE(aalhendi): Complete the callback store before its return branch.
			CTR_PSX_MEMORY_BARRIER();
		}

		return;
	}

	sdata->countFail_CdSyncCallback++;
}


// The unused argument is required by CdlCB.
void CDSYS_XaCallbackCdReady(u8 result, u8 *unk)
{
	(void)unk;
	if (result == CdlDataReady)
	{
		CdGetSector(&GAME_CD_READY_POSITION[0], 3);
		CDSYS_CALLBACK_STATE->XA_CurrPos = CdPosToInt(&GAME_CD_READY_POSITION[0]);

		if (
		    // queued to start (CD seeking?)
		    (CDSYS_CALLBACK_STATE->XA_State == XA_STARTING) && (CDSYS_CALLBACK_STATE->XA_CurrPos >= CDSYS_CALLBACK_STATE->XA_StartPos))
		{
			// XA playing
			CDSYS_CALLBACK_STATE->XA_State = XA_PLAYING;

			CDSYS_CALLBACK_STATE->XA_CurrOffset = (CDSYS_CALLBACK_STATE->XA_CurrPos - CDSYS_CALLBACK_STATE->XA_StartPos) * 4;
		}

		if (
		    // XA is playing
		    (CDSYS_CALLBACK_STATE->XA_State == XA_PLAYING) && (CDSYS_CALLBACK_STATE->XA_CurrPos > CDSYS_CALLBACK_STATE->XA_EndPos))
		{
			// XA should stop
			CDSYS_CALLBACK_STATE->XA_State = XA_FADING;

			// disable music
			CDSYS_CALLBACK_STATE->XA_VolumeDeduct = CDSYS_XA_FADE_VOLUME_STEP;
		}

		CDSYS_CALLBACK_STATE->countPass_CdReadyCallback++;
		CTR_PSX_MEMORY_BARRIER();
		return;
	}

	CDSYS_CALLBACK_STATE->countFail_CdReadyCallback++;
}


void CDSYS_SpuCallbackIRQ(void)
{
	// disable IRQ, now that it's finished
	SpuSetIRQ(0);

	// SPU_CDONLY writes the two-channel CD storage, not the full four-channel
	// SDK object. Each channel has two 256-sample halves; the transfer callback
	// selects a half for amplitude tracking.
	SpuReadDecodedData(GAME_SPU_DECODED_BUFFER, SPU_CDONLY);

	if ((CDSYS_CALLBACK_STATE->XA_boolFinished == 0) &&

	    // if XA is requested to stop,
	    // either by CdReadyCallback,
	    // or CDSYS_XAPauseRequest
	    (CDSYS_CALLBACK_STATE->XA_State == XA_FADING))
	{
		// fade volume to zero, over multiple frames

		CDSYS_CALLBACK_STATE->XA_VolumeBitshift -= CDSYS_CALLBACK_STATE->XA_VolumeDeduct;

		if (CDSYS_CALLBACK_STATE->XA_VolumeBitshift < 0)
		{
			CDSYS_CALLBACK_STATE->XA_VolumeBitshift = 0;
			CDSYS_CALLBACK_STATE->XA_boolFinished = 1;
		}

		SpuSetCommonCDVolume((s16)CDSYS_CALLBACK_STATE->XA_VolumeBitshift, (s16)CDSYS_CALLBACK_STATE->XA_VolumeBitshift);
	}

	// really? ND bug? or variable reuse?
	GAME_XA_CURRENT_OFFSET++;
}


void CDSYS_SpuCallbackTransfer(void)
{
	if (GAME_XA_IRQ_ADDRESS == 0)
	{
		CDSYS_CALLBACK_STATE->irqAddr = CDSYS_SPU_DECODED_IRQ_ADDR;
		CTR_PSX_MEMORY_BARRIER();
	}
	else
	{
		CDSYS_CALLBACK_STATE->irqAddr = 0;
	}

	SpuSetIRQAddr(CDSYS_CALLBACK_STATE->irqAddr);
	SpuSetIRQ(1);

	CDSYS_CALLBACK_STATE->countPass_CdTransferCallback++;
	CTR_PSX_MEMORY_BARRIER();

	CDSYS_SpuGetMaxSample();
}


void CDSYS_SpuEnableIRQ(void)
{
	int i;
	GAME_XA_MAX_SAMPLE = 0;
	GAME_XA_HISTORY_MAX = 0;
	for (i = 0; i < CDSYS_SPU_DECODED_BUFFER_SAMPLES; i++)
	{
		GAME_SPU_DECODED_BUFFER[i] = 0;
	}

	SpuSetTransferMode(SPU_TRANSFER_BY_DMA);
	SpuSetTransferCallback(CDSYS_SpuCallbackTransfer);
	SpuSetIRQCallback(CDSYS_SpuCallbackIRQ);

	CDSYS_CALLBACK_STATE->irqAddr = CDSYS_SPU_DECODED_IRQ_ADDR;
	CDSYS_CALLBACK_STATE->unused_8008d700 = 0;

	SpuSetIRQAddr(CDSYS_CALLBACK_STATE->irqAddr);
	SpuSetIRQ(1);
}


void CDSYS_SpuDisableIRQ(void)
{
	SpuSetIRQ(0);
	SpuSetIRQCallback(0);
	SpuSetTransferCallback(0);

	GAME_XA_MAX_SAMPLE = 0;
	GAME_XA_HISTORY_MAX = 0;
}


#if defined(CTR_NATIVE)
internal s32 CDSYS_NativeGetXAFadeAmount(s32 fadeSteps)
{
	if (fadeSteps <= 0)
	{
		fadeSteps = 1;
	}

	if (sdata->XA_VolumeDeduct <= 0)
	{
		return sdata->XA_VolumeBitshift;
	}

	if (fadeSteps >= (sdata->XA_VolumeBitshift + sdata->XA_VolumeDeduct - 1) / sdata->XA_VolumeDeduct)
	{
		return sdata->XA_VolumeBitshift;
	}

	return sdata->XA_VolumeDeduct * fadeSteps;
}

static void CDSYS_SaveMaxSample(int max)
{
	// save max for this block
	sdata->XA_MaxSampleVal = max;
	sdata->XA_MaxSampleValArr[sdata->XA_MaxSampleIndex] = max;

	// Cycle through ring buffer
	sdata->XA_MaxSampleIndex++;
	if (sdata->XA_MaxSampleIndex >= CDSYS_XA_MAX_SAMPLE_WINDOW)
	{
		sdata->XA_MaxSampleIndex = 0;
	}

	if (sdata->XA_MaxSampleNumSaved < CDSYS_XA_MAX_SAMPLE_WINDOW)
	{
		sdata->XA_MaxSampleNumSaved++;
	}

	// Find max of last 3 block maxes,
	// as long as 3 blocks have already passed
	sdata->XA_MaxSampleValInArr = 0;
	int index = sdata->XA_MaxSampleIndex;
	for (int i = sdata->XA_MaxSampleNumSaved; i > 0; i--)
	{
		index--;
		if (index < 0)
		{
			index = CDSYS_XA_MAX_SAMPLE_WINDOW - 1;
		}

		if (sdata->XA_MaxSampleValInArr < sdata->XA_MaxSampleValArr[index])
		{
			sdata->XA_MaxSampleValInArr = sdata->XA_MaxSampleValArr[index];
		}
	}
}

void CDSYS_SpuGetMaxSampleAtOffset(int xaCurrOffset)
{
	// NOTE(aalhendi): Retail reads decoded CD-XA samples from SPU IRQ
	// buffers. Native XA is decoded by the native audio backend, so feed
	// the same amplitude globals from the matching native PCM block.
	if (sdata->boolUseDisc == 0)
	{
		CDSYS_SaveMaxSample(NativeAudio_GetXAMaxSampleAtOffset(xaCurrOffset));
		return;
	}

	CDSYS_SpuGetMaxSample();
}
#endif

void CDSYS_SpuGetMaxSample(void)
{
	s32 rawSample;
	s16 sample;
	s16 max;
	int start;
	int end;
	int i;
	int index;
	max = 0;

#if defined(CTR_NATIVE)
	if (sdata->boolUseDisc == 0)
	{
		CDSYS_SaveMaxSample(NativeAudio_GetXAMaxSample());
		return;
	}
#endif

	if (sdata->boolUseDisc == 0)
	{
		return;
	}

	if (sdata->irqAddr == 0)
	{
		start = 0;
		end = CDSYS_SPU_DECODED_HALF_SAMPLES;
	}
	else
	{
		start = CDSYS_SPU_DECODED_HALF_SAMPLES;
		end = CDSYS_SPU_DECODED_BUFFER_SAMPLES;
	}

	// absolute value, find max in block
	for (i = start; i < end; i++)
	{
		rawSample = GAME_SPU_DECODED_BUFFER[i];
		sample = rawSample;
		if (rawSample < 0)
		{
			// NOTE(aalhendi): Retail negates in place and compares the signed
			// low halfword; -32768 therefore remains negative rather than clipping.
			CTR_PSX_NEGATE_IN_PLACE(sample);
		}
		if (max < sample)
		{
			max = sample;
		}
	}

	// save max for this block
	CDSYS_CALLBACK_STATE->XA_MaxSampleVal = max;
	GAME_XA_MAX_SAMPLE_HISTORY[GAME_XA_MAX_SAMPLE_INDEX] = CDSYS_CALLBACK_STATE->XA_MaxSampleVal;

	// Cycle through ring buffer
	GAME_XA_MAX_SAMPLE_INDEX++;
	if (GAME_XA_MAX_SAMPLE_INDEX >= CDSYS_XA_MAX_SAMPLE_WINDOW)
	{
		GAME_XA_MAX_SAMPLE_INDEX = 0;
	}

	if (GAME_XA_SAMPLE_COUNT < CDSYS_XA_MAX_SAMPLE_WINDOW)
	{
		GAME_XA_SAMPLE_COUNT++;
	}

	// Find the maximum among the available blocks in the three-block history.
	GAME_XA_HISTORY_MAX = 0;
	i = GAME_XA_MAX_SAMPLE_INDEX;
	index = GAME_XA_SAMPLE_COUNT;
	if (index != 0)
	{
		for (index--; index != -1; index--)
		{
			i--;
			if (i < 0)
			{
				i = CDSYS_XA_MAX_SAMPLE_WINDOW - 1;
			}

			if (GAME_XA_HISTORY_MAX < GAME_XA_MAX_SAMPLE_HISTORY[i])
			{
				GAME_XA_HISTORY_MAX = GAME_XA_MAX_SAMPLE_HISTORY[i];
			}
		}
	}
}


int CDSYS_XAGetNumTracks(int categoryID)
{
	if (sdata->boolUseDisc == 0)
	{
	no_tracks:
		return 0;
	}
	else if (categoryID >= CDSYS_XA_NUM_TYPES)
	{
		goto no_tracks;
	}

	return sdata->ptrArray_numSongs[categoryID];
}


int CDSYS_XASeek(b32 boolCdControl, int categoryID, int xaID)
{
	CdlLOC loc;
	s32 com;
	register struct XaSize *xas CTR_PSX_REGISTER("$3");
	s32 sum;
	register s32 *cdPositions CTR_PSX_REGISTER("$2");
	register s32 prefix CTR_PSX_REGISTER("$3");
	s32 *firstXa;
	s32 firstSong;
	s32 firstXaIndex;
	s32 *positionsBase;

	if (CDSYS_DISC_ENABLED == 0)
	{
		return 1;
	}

	if (sdata->bool_XnfLoaded == 0)
	{
		return 0;
	}

	if (categoryID >= CDSYS_XA_NUM_TYPES)
	{
		return 0;
	}

	if (xaID >= CDSYS_XAGetNumTracks(categoryID))
	{
		return 0;
	}

	if (sdata->discMode != DM_AUDIO)
	{
		CDSYS_SetMode_StreamAudio();
	}

	firstSong = sdata->ptrArray_firstSongIndex[categoryID];
	// Resolve the category's base before adding the track and XA-file indices.
	CTR_PSX_LOAD_WORD_AFTER(xas, sdata->ptrArray_XaSize, firstSong);
	xas += firstSong;
	CTR_PSX_KEEP_VALUE_RELAXED(xas);
	xas += xaID;
	firstXa = CDSYS_CALLBACK_STATE->ptrArray_firstXaIndex;
	CTR_PSX_DEPEND_VALUE(xas, firstXa);
	prefix = (u8)xas->XaPrefix;
	CTR_PSX_LOAD_WORD_AFTER(firstXaIndex, firstXa[categoryID], prefix * sizeof(s32));
	CTR_PSX_LOAD_WORD_AFTER(positionsBase, sdata->ptrArray_XaCdPos, firstXaIndex);
	CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(cdPositions, positionsBase, firstXaIndex * sizeof(s32));
	sum = cdPositions[prefix];

	CdIntToPos(sum, &loc);

	CDSYS_CALLBACK_STATE->XA_State = XA_SEEKING;
	CTR_PSX_MEMORY_BARRIER();

	com = CdlSeekP;
	if (boolCdControl != 0)
	{
		com = CdlSeekL;
	}

	CdControl(com, (u8 *)&loc, 0);

	return 1;
}


int CDSYS_XAGetTrackLength(int categoryID, int xaID)
{
	register struct XaSize *xas CTR_PSX_REGISTER("$3");
	s32 firstSong;
	if (CDSYS_DISC_ENABLED == 0)
	{
#if defined(CTR_NATIVE)
		return NativeAudio_GetXATrackLength(categoryID, xaID);
#endif
		return 0;
	}

	if (sdata->bool_XnfLoaded == 0)
	{
		return 0;
	}

	if (categoryID >= CDSYS_XA_NUM_TYPES)
	{
		return 0;
	}

	if (xaID >= CDSYS_XAGetNumTracks(categoryID))
	{
		return 0;
	}

	firstSong = sdata->ptrArray_firstSongIndex[categoryID];
	CTR_PSX_LOAD_WORD_AFTER(xas, sdata->ptrArray_XaSize, firstSong);
	xas += firstSong;
	CTR_PSX_KEEP_VALUE_RELAXED(xas);
	xas += xaID;
	return xas->XaBytes;
}


int CDSYS_XAPlay(int categoryID, int xaID)
{
	u8 buf1[8];
	u8 buf2[8];
	s32 vol;
	struct XaSize *xas;
	s32 sum;
	register s32 *cdPositions CTR_PSX_REGISTER("$2");
	register CdlLOC *loc CTR_PSX_REGISTER("$18");
	register s32 prefix CTR_PSX_REGISTER("$3");
	s32 *firstXa;
	register s32 *positionsBase CTR_PSX_REGISTER("$4");
	s32 firstXaIndex;
	s32 endPos;
	register s32 command CTR_PSX_REGISTER("$4");
	register u8 *argument CTR_PSX_REGISTER("$5");
	register u8 *result CTR_PSX_REGISTER("$6");

	if (CDSYS_DISC_ENABLED == 0)
	{
#if defined(CTR_NATIVE)
		int nativeVol = (categoryID == CDSYS_XA_TYPE_MUSIC) ? sdata->vol_Music : sdata->vol_Voice;

		// NOTE(aalhendi): Native CD has no CD-XA IRQ stream. Feed extracted
		// XA assets to the native audio backend and synthesize the minimal
		// retail XA state gates.
		if (NativeAudio_PlayXATrack(categoryID, xaID, nativeVol << CDSYS_XA_VOLUME_SHIFT, nativeVol << CDSYS_XA_VOLUME_SHIFT) == 0)
		{
			return 0;
		}

		sdata->XA_State = XA_PLAYING;
		sdata->XA_Playing_Index = xaID;
		sdata->XA_Playing_Category = categoryID;
		sdata->XA_VolumeBitshift = nativeVol << CDSYS_XA_VOLUME_SHIFT;
		sdata->XA_boolFinished = 0;
		sdata->XA_CurrOffset = 0;
		sdata->XA_MaxSampleIndex = 0;
		sdata->XA_MaxSampleNumSaved = 0;
		for (int i = 0; i < CDSYS_XA_MAX_SAMPLE_WINDOW; i++)
		{
			sdata->XA_MaxSampleValArr[i] = 0;
		}
		sdata->XA_MaxSampleVal = 0;
		sdata->XA_MaxSampleValInArr = 0;
		return 1;
#endif
		return 1;
	}

	if (sdata->bool_XnfLoaded == 0)
	{
		goto failed;
	}

	if (categoryID >= CDSYS_XA_NUM_TYPES)
	{
		return 0;
	}

	if (xaID >= CDSYS_XAGetNumTracks(categoryID))
	{
		return 0;
	}

	CTR_PSX_CLOBBER("$2");
	if (GAME_LOAD_IN_PROGRESS != 0)
	{
		OtherFX_Play(5, 1);
		return 0;
	}

	if (sdata->discMode != DM_AUDIO)
	{
		CDSYS_SetMode_StreamAudio();
	}

	sdata->XA_State = XA_STARTING;
	sdata->XA_Playing_Category = categoryID;
	sdata->XA_Playing_Index = xaID;
	CTR_PSX_MEMORY_BARRIER();

	// NOTE(aalhendi): Each volume branch opens its own absolute byte slot;
	// release the branch temporary before that branch-local address lifetime.
	if (categoryID == CDSYS_XA_TYPE_MUSIC)
	{
		CTR_PSX_CLOBBER("$2");
		vol = CDSYS_MUSIC_VOLUME << CDSYS_XA_VOLUME_SHIFT;
	}
	else
	{
		CTR_PSX_CLOBBER("$2");
		vol = CDSYS_VOICE_VOLUME << CDSYS_XA_VOLUME_SHIFT;
	}

	CDSYS_CALLBACK_STATE->XA_VolumeBitshift = vol;
	SpuSetCommonCDVolume((s16)CDSYS_CALLBACK_STATE->XA_VolumeBitshift, (s16)CDSYS_CALLBACK_STATE->XA_VolumeBitshift);

	xas = &sdata->ptrArray_XaSize[sdata->ptrArray_firstSongIndex[categoryID]];
	CTR_PSX_KEEP_VALUE_RELAXED(xas);
	xas += xaID;

	buf1[0] = 1;
	buf1[1] = xas->XaIndex;
	CdControl(CdlSetfilter, &buf1[0], 0);

	loc = (CdlLOC *)&buf2[0];
	CTR_PSX_KEEP_VALUE_RELAXED(loc);
	firstXa = sdata->ptrArray_firstXaIndex;
	CTR_PSX_DEPEND_VALUE(xas, firstXa);
	prefix = (u8)xas->XaPrefix;
	CTR_PSX_LOAD_WORD_AFTER(positionsBase, sdata->ptrArray_XaCdPos, prefix);
	firstXaIndex = firstXa[categoryID];
	CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(cdPositions, positionsBase, firstXaIndex * sizeof(s32));
	sum = cdPositions[prefix];
	endPos = sum + xas->XaBytes;
	argument = (u8 *)loc;
	CTR_PSX_OBSERVE_VALUE(argument);
	sdata->XA_StartPos = sum;
	sdata->XA_EndPos = endPos;
	CTR_PSX_MEMORY_BARRIER();
	CdIntToPos(sum, (CdlLOC *)argument);

	// Prepare the asynchronous command, then publish all reset counters before
	// the SDK can expose the first read callback.
	command = CdlReadS;
	argument = (u8 *)loc;
	result = 0;
	CTR_PSX_OBSERVE_VALUE(command);
	CTR_PSX_OBSERVE_VALUE(argument);
	CTR_PSX_OBSERVE_VALUE(result);

	sdata->XA_MaxSampleVal = 0;
	sdata->XA_MaxSampleValInArr = 0;
	sdata->unused_8008d700 = 0;

	sdata->countPass_CdReadyCallback = 0;
	sdata->countFail_CdReadyCallback = 0;
	sdata->XA_CurrOffset = 0; // ND bug? Variable resuse?
	sdata->countPass_CdTransferCallback = 0;
	CTR_PSX_MEMORY_BARRIER();

	if (CdControl(command, argument, result) == 1)
	{
		// As of now, XA plays indefinitely, until CdReadyCallback
		// determines the current CD position is past the end position,
		// and then when IRQ determines the SPU is done playing the last
		// of the XA, CDSYS_XAPauseForce is called to stop playing XA.

		// Emulators with no IRQ support will keep playing random
		// XA audio on the disc infinitely, and never reach ND Box

		CDSYS_SpuEnableIRQ();
		return 1;
	}

	sdata->XA_State = XA_IDLE;
	CTR_PSX_MEMORY_BARRIER();
failed:
	return 0;
}


void CDSYS_XAPauseRequest()
{
	if (sdata->boolUseDisc == 0)
	{
#if defined(CTR_NATIVE)
		if ((sdata->XA_State >= XA_STARTING) && (sdata->XA_State <= XA_PLAYING))
		{
			sdata->XA_State = XA_FADING;
			sdata->XA_VolumeDeduct = CDSYS_XA_FADE_VOLUME_STEP;
		}
#endif
		return;
	}
	if (sdata->bool_XnfLoaded == 0)
	{
		return;
	}
	if (CDSYS_CALLBACK_STATE->XA_State != XA_PLAYING)
	{
		CTR_PSX_MEMORY_BARRIER();
		if (CDSYS_CALLBACK_STATE->XA_State != XA_STARTING)
		{
			return;
		}
	}

	sdata->XA_State = XA_FADING;
	sdata->XA_VolumeDeduct = CDSYS_XA_FADE_VOLUME_STEP;
}


void CDSYS_XAPauseForce()
{
	if (CDSYS_DISC_ENABLED == 0)
	{
#if defined(CTR_NATIVE)
		NativeAudio_StopXA();
		sdata->XA_boolFinished = 0;
		sdata->XA_State = XA_IDLE;
		sdata->XA_PauseFrame = sdata->gGT->frameTimer_MainFrame_ResetDB;
#endif
		return;
	}
	if (sdata->bool_XnfLoaded == 0)
	{
		return;
	}
	if (sdata->XA_State == XA_IDLE)
	{
		return;
	}

	sdata->XA_boolFinished = 0;
	sdata->XA_State = XA_IDLE;
	CTR_PSX_MEMORY_BARRIER();

	SpuSetIRQ(0);
	CDSYS_SpuDisableIRQ();

	CdControl(CdlPause, 0, 0);

	sdata->XA_PauseFrame = GAME_TRACKER->frameTimer_MainFrame_ResetDB;
}


void CDSYS_XAPauseAtEnd()
{
// NOTE(aalhendi): Native services no-disc playback here; retail checks the
// shared finished flag directly, including when disc use is disabled.
#if defined(CTR_NATIVE)
	if (sdata->boolUseDisc == 0)
	{
		int xaPlaying = NativeAudio_IsXAPlaying();
		int fadeSteps = 0;

		if (xaPlaying != 0)
		{
			int prevOffset = sdata->XA_CurrOffset;
			int currOffset = NativeAudio_GetXACurrOffset();
			int firstOffset = -1;

			if (currOffset < prevOffset)
			{
				prevOffset = currOffset;
			}

			if (currOffset > prevOffset)
			{
				fadeSteps = currOffset - prevOffset;
			}

			if (sdata->XA_MaxSampleNumSaved == 0)
			{
				firstOffset = currOffset;
			}
			else if (fadeSteps > 0)
			{
				firstOffset = currOffset - 2;
				if (firstOffset < prevOffset + 1)
				{
					firstOffset = prevOffset + 1;
				}
				if (firstOffset < 0)
				{
					firstOffset = 0;
				}
			}

			for (int offset = firstOffset; offset >= 0 && offset <= currOffset; offset++)
			{
				CDSYS_SpuGetMaxSampleAtOffset(offset);
			}

			sdata->XA_CurrOffset = currOffset;
		}

		if (sdata->XA_State == XA_FADING)
		{
			sdata->XA_VolumeBitshift -= CDSYS_NativeGetXAFadeAmount(fadeSteps);
			if (sdata->XA_VolumeBitshift <= 0)
			{
				sdata->XA_VolumeBitshift = 0;
				NativeAudio_StopXA();
				sdata->XA_boolFinished = 0;
				sdata->XA_State = XA_IDLE;
				sdata->XA_MaxSampleVal = 0;
				sdata->XA_MaxSampleValInArr = 0;
				sdata->XA_PauseFrame = sdata->gGT->frameTimer_MainFrame_ResetDB;
			}
			else
			{
				NativeAudio_SetXAVolume((s16)sdata->XA_VolumeBitshift, (s16)sdata->XA_VolumeBitshift);
			}
		}
		else if (xaPlaying == 0)
		{
			if (sdata->XA_State != XA_IDLE)
			{
				sdata->XA_PauseFrame = sdata->gGT->frameTimer_MainFrame_ResetDB;
			}
			sdata->XA_State = XA_IDLE;
			sdata->XA_MaxSampleVal = 0;
			sdata->XA_MaxSampleValInArr = 0;
		}
		return;
	}
#endif

	// wait until IRQ says XA is finished
	if (CDSYS_FINISHED == 0)
	{
		return;
	}

	CDSYS_XAPauseForce();
}
