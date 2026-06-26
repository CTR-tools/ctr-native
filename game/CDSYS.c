#include <common.h>

#if defined(CTR_NATIVE)
#include <platform/native_audio.h>
#endif

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001c360-0x8001c420.
int CDSYS_Init(b32 boolUseDisc)
{
	sdata->boolUseDisc = boolUseDisc;

	*(s16 *)&sdata->unused400[0] = 0;

	// if using parallel port (Naughty Dog Devs only)
	if (boolUseDisc != 0)
	{
		// if Cd does not initialize
		if (CdInit() == 0)
		{
			// use parallel port (Naughty Dog Devs only)
			sdata->boolUseDisc = 0;
			return 0;
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

	// unused_8008d700

	sdata->XA_CurrPos = 0;
	sdata->XA_PauseFrame = 0;

	CDSYS_SetMode_StreamData();

	// 1 - English
	CDSYS_SetXAToLang(1);

	Voiceline_PoolClear();

	return 1;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001c420-0x8001c470.
u32 CDSYS_GetFilePosInt(char *fileString, int *filePos)
{
	CdlFILE cdlFile;

	if (CdSearchFile(&cdlFile, fileString) != 0)
	{
		*filePos = CdPosToInt(&cdlFile.pos);
		return 1;
	}

	return 0;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001c470-0x8001c4f4.
void CDSYS_SetMode_StreamData()
{
	char buf[8];

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
	if (sdata->boolUseDisc == 0)
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

	sdata->discMode = DM_DATA;
	sdata->XA_State = XA_IDLE;

	if (sdata->bool_XnfLoaded != 0)
	{
		CdSyncCallback(0);
		CdReadyCallback(0);
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001c4f4-0x8001c56c.
void CDSYS_SetMode_StreamAudio()
{
	char buf[8];

	if (sdata->boolUseDisc == 0)
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
	buf[0] = 0xE8;
	CdControl(CdlSetmode, buf, 0);

	sdata->discMode = DM_AUDIO;
	sdata->XA_State = XA_IDLE;

	CdSyncCallback(CDSYS_XaCallbackCdSync);
	CdReadyCallback(CDSYS_XaCallbackCdReady);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001c56c-0x8001c7a4.
int CDSYS_SetXAToLang(int lang)
{
	char *xaLang;
	int fileSize;
	struct XNF *xnf;

	if (sdata->boolUseDisc == 0)
	{
		return 1;
	}
	if (lang >= 8)
	{
		return 0;
	}

	sdata->bool_XnfLoaded = 0;
	CDSYS_SetMode_StreamData();

	xaLang = data.xaLanguagePtrs[lang];
	strncpy(&data.s_XA_ENG_XNF[4], xaLang, 3);
	strncpy(&data.s_XA_ENG_EXTRA[4], xaLang, 3);
	strncpy(&data.s_XA_ENG_GAME[4], xaLang, 3);

	// store on heap
	void *ptrDst = 0;

	xnf = LOAD_XnfFile(data.s_XA_ENG_XNF, ptrDst, &fileSize);

	// read error
	if (xnf == 0)
	{
		return 0;
	}

	// header error
	if (xnf->magic != *(int *)&sdata->s_XINF[0])
	{
		return 0;
	}

	// Aug5=100, Sep3=101, Retail=102
	if (xnf->version != 102)
	{
		return 0;
	}

	sdata->xa_numTypes = xnf->numTypes;
	if (sdata->xa_numTypes != CDSYS_XA_NUM_TYPES)
	{
		return 0;
	}

	sdata->ptrArray_NumXAs = &xnf->numXA[0];
	sdata->ptrArray_firstXaIndex = &xnf->firstXaIndex[0];
	sdata->ptrArray_numSongs = &xnf->numSongs[0];
	sdata->ptrArray_firstSongIndex = &xnf->firstSongIndex[0];
	sdata->ptrArray_XaCdPos = XNF_GETXACDPOS(xnf);
	sdata->ptrArray_XaSize = (struct XaSize *)&sdata->ptrArray_XaCdPos[xnf->numXAs_total];

	for (int categoryID = 0; categoryID < sdata->xa_numTypes; categoryID++)
	{
		struct AudioMeta *am = &data.audioMeta[categoryID];

		for (int xaID = 0; xaID < sdata->ptrArray_NumXAs[categoryID]; xaID++)
		{
			am->name[am->stringIndex_char1] = '0' + (xaID / 10);
			am->name[am->stringIndex_char2] = '0' + (xaID % 10);

			int firstXaIndex = sdata->ptrArray_firstXaIndex[categoryID];
			int *returnPtr_xaCdPos = &sdata->ptrArray_XaCdPos[firstXaIndex + xaID];

			// quit on error to find XA file
			if (CDSYS_GetFilePosInt(am->name, returnPtr_xaCdPos) == 0)
			{
				return 0;
			}
		}
	}

	sdata->bool_XnfLoaded = 1;
	return 1;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001c7a4-0x8001c7fc.
void CDSYS_XaCallbackCdSync(u8 result, u8 *unk) //+unk to adhere to *CdlCB
{
	u8 com;

	if (result == CdlComplete)
	{
		// determine CdlSeekL or CdlSeekP
		com = CdLastCom() - 0x15;

		if (com < 2)
		{
			sdata->XA_State = XA_IDLE;
		}

		return;
	}

	sdata->countFail_CdSyncCallback++;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001c7fc-0x8001c8e4.
void CDSYS_XaCallbackCdReady(u8 result, u8 *unk) //+unk to adhere to *CdlCB
{
	if (result == CdlDataReady)
	{
		CdGetSector(&sdata->cdlFile_CdReady[0], 3);
		sdata->XA_CurrPos = CdPosToInt(&sdata->cdlFile_CdReady[0]);

		if (
		    // queued to start (CD seeking?)
		    (sdata->XA_State == XA_STARTING) && (sdata->XA_StartPos <= sdata->XA_CurrPos))
		{
			// XA playing
			sdata->XA_State = XA_PLAYING;

			sdata->XA_CurrOffset = (sdata->XA_CurrPos - sdata->XA_StartPos) * 4;
		}

		if (
		    // XA is playing
		    (sdata->XA_State == XA_PLAYING) && (sdata->XA_EndPos < sdata->XA_CurrPos))
		{
			// XA should stop
			sdata->XA_State = XA_FADING;

			// disable music
			sdata->XA_VolumeDeduct = 0x400;
		}

		sdata->countPass_CdReadyCallback++;
		return;
	}

	sdata->countFail_CdReadyCallback++;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001c8e4-0x8001c984.
void CDSYS_SpuCallbackIRQ()
{
	// disable IRQ, now that it's finished
	SpuSetIRQ(0);

/*

From LibRef
When you call SpuReadDecodedData(), the SPU copies data from its buffers to the SpuDecodeData struct
in main memory that you specify. It copies the data one-half buffer (0x200 bytes) at a time, and returns a
code specifying which half of the buffer is currently being written to, so you can use the data from the other
half.
*/

// from TOMB5
// https://github.com/TOMB5/TOMB5/blob/master/EMULATOR/LIBSPU.H
#define SPU_CDONLY 5
	SpuReadDecodedData((SpuDecodedData *)sdata->SpuDecodedBuf, SPU_CDONLY);

	if ((sdata->XA_boolFinished == 0) &&

	    // if XA is requested to stop,
	    // either by CdReadyCallback,
	    // or CDSYS_XAPauseRequest
	    (sdata->XA_State == XA_FADING))
	{
		// fade volume to zero, over multiple frames

		sdata->XA_VolumeBitshift -= sdata->XA_VolumeDeduct;

		if (sdata->XA_VolumeBitshift < 0)
		{
			sdata->XA_VolumeBitshift = 0;
			sdata->XA_boolFinished = 1;
		}

		SpuSetCommonCDVolume((s16)sdata->XA_VolumeBitshift, (s16)sdata->XA_VolumeBitshift);
	}

	// really? ND bug? or variable reuse?
	sdata->XA_CurrOffset++;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001c984-0x8001c9e4.
void CDSYS_SpuCallbackTransfer()
{
	if (sdata->irqAddr == 0)
	{
		sdata->irqAddr = 0x200;
	}
	else
	{
		sdata->irqAddr = 0;
	}

	SpuSetIRQAddr(sdata->irqAddr);
	SpuSetIRQ(1);

	sdata->countPass_CdTransferCallback++;

	CDSYS_SpuGetMaxSample();
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001c9e4-0x8001ca64.
void CDSYS_SpuEnableIRQ()
{
	for (int i = 0; i < 0x200; i++)
	{
		sdata->SpuDecodedBuf[i] = 0;
	}

	sdata->XA_MaxSampleVal = 0;
	sdata->XA_MaxSampleValInArr = 0;

	SpuSetTransferMode(SPU_TRANSFER_BY_DMA);
	SpuSetTransferCallback(CDSYS_SpuCallbackTransfer);
	SpuSetIRQCallback(CDSYS_SpuCallbackIRQ);

	sdata->irqAddr = 0x200;
	sdata->unused_8008d700 = 0;

	SpuSetIRQAddr(0x200);
	SpuSetIRQ(1);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001ca64-0x8001ca98.
void CDSYS_SpuDisableIRQ()
{
	SpuSetIRQ(0);
	SpuSetIRQCallback(0);
	SpuSetTransferCallback(0);

	sdata->XA_MaxSampleValInArr = 0;
	sdata->XA_MaxSampleVal = 0;
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
	if (sdata->XA_MaxSampleIndex >= 3)
	{
		sdata->XA_MaxSampleIndex = 0;
	}

	if (sdata->XA_MaxSampleNumSaved < 3)
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
			index = 2;
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

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001ca98-0x8001cbe0.
void CDSYS_SpuGetMaxSample(void)
{
	s16 sample;
	s16 max;
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

	int start = 0x100;
	int end = 0x200;
	if (sdata->irqAddr == 0)
	{
		start = 0;
		end = 0x100;
	}

	s16 *ptrSpuBuf = (s16 *)&sdata->SpuDecodedBuf[start];

	// absolute value, find max in block
	for (int i = start; i < end; i++)
	{
		sample = *ptrSpuBuf++;
		if (sample < 0)
		{
			sample = -sample;
		}
		if (max < sample)
		{
			max = sample;
		}
	}

#if defined(CTR_NATIVE)
	CDSYS_SaveMaxSample(max);
#else
	// save max for this block
	sdata->XA_MaxSampleVal = max;
	sdata->XA_MaxSampleValArr[sdata->XA_MaxSampleIndex] = max;

	// Cycle through ring buffer
	sdata->XA_MaxSampleIndex++;
	if (sdata->XA_MaxSampleIndex >= 3)
		sdata->XA_MaxSampleIndex = 0;

	if (sdata->XA_MaxSampleNumSaved < 3)
		sdata->XA_MaxSampleNumSaved++;

	// Find max of last 3 block maxes,
	// as long as 3 blocks have already passed
	sdata->XA_MaxSampleValInArr = 0;
	int index = sdata->XA_MaxSampleIndex;
	for (int i = sdata->XA_MaxSampleNumSaved; i > 0; i--)
	{
		index--;
		if (index < 0)
			index = 2;

		if (sdata->XA_MaxSampleValInArr < sdata->XA_MaxSampleValArr[index])
			sdata->XA_MaxSampleValInArr = sdata->XA_MaxSampleValArr[index];
	}
#endif
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001cbe0-0x8001cc18.
int CDSYS_XAGetNumTracks(int categoryID)
{
	if (sdata->boolUseDisc == 0)
	{
		return 0;
	}
	if (categoryID >= CDSYS_XA_NUM_TYPES)
	{
		return 0;
	}

	return sdata->ptrArray_numSongs[categoryID];
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001cc18-0x8001cd20.
int CDSYS_XASeek(b32 boolCdControl, int categoryID, int xaID)
{
	CdlLOC loc;
	int com;

	if (sdata->boolUseDisc == 0)
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

	struct XaSize *xas = &sdata->ptrArray_XaSize[sdata->ptrArray_firstSongIndex[categoryID] + xaID];
	int sum = sdata->ptrArray_XaCdPos[sdata->ptrArray_firstXaIndex[categoryID] + xas->XaPrefix];

	CdIntToPos(sum, &loc);

	sdata->XA_State = XA_SEEKING;

	com = CdlSeekP;
	if (boolCdControl != 0)
	{
		com = CdlSeekL;
	}

	CdControl(com, (u8 *)&loc, 0);

	return 1;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001cd20-0x8001cdb4
int CDSYS_XAGetTrackLength(int categoryID, int xaID)
{
	if (sdata->boolUseDisc == 0)
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

	int sizeIndex = sdata->ptrArray_firstSongIndex[categoryID] + xaID;

	return sdata->ptrArray_XaSize[sizeIndex].XaBytes;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001cdb4-0x8001cf98
int CDSYS_XAPlay(int categoryID, int xaID)
{
	char buf1[8];
	char buf2[8];

	if (sdata->boolUseDisc == 0)
	{
#if defined(CTR_NATIVE)
		int nativeVol = (categoryID == CDSYS_XA_TYPE_MUSIC) ? sdata->vol_Music : sdata->vol_Voice;

		// NOTE(aalhendi): Native CD has no CD-XA IRQ stream. Feed extracted
		// XA assets to the native audio backend and synthesize the minimal
		// retail XA state gates.
		if (NativeAudio_PlayXATrack(categoryID, xaID, nativeVol << 7, nativeVol << 7) == 0)
		{
			return 0;
		}

		sdata->XA_State = XA_PLAYING;
		sdata->XA_Playing_Index = xaID;
		sdata->XA_Playing_Category = categoryID;
		sdata->XA_VolumeBitshift = nativeVol << 7;
		sdata->XA_boolFinished = 0;
		sdata->XA_CurrOffset = 0;
		sdata->XA_MaxSampleIndex = 0;
		sdata->XA_MaxSampleNumSaved = 0;
		sdata->XA_MaxSampleValArr[0] = 0;
		sdata->XA_MaxSampleValArr[1] = 0;
		sdata->XA_MaxSampleValArr[2] = 0;
		sdata->XA_MaxSampleVal = 0;
		sdata->XA_MaxSampleValInArr = 0;
		return 1;
#endif
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

	if (sdata->load_inProgress != 0)
	{
		OtherFX_Play(5, 1);
		return 0;
	}

	if (sdata->discMode != DM_AUDIO)
	{
		CDSYS_SetMode_StreamAudio();
	}

	sdata->XA_State = XA_STARTING;

	int vol = sdata->vol_Voice;
	if (categoryID == CDSYS_XA_TYPE_MUSIC)
	{
		vol = sdata->vol_Music;
	}

	sdata->XA_VolumeBitshift = vol << 7;
	SpuSetCommonCDVolume((s16)sdata->XA_VolumeBitshift, (s16)sdata->XA_VolumeBitshift);

	sdata->XA_Playing_Index = xaID;
	sdata->XA_Playing_Category = categoryID;

	struct XaSize *xas = &sdata->ptrArray_XaSize[sdata->ptrArray_firstSongIndex[categoryID] + xaID];
	int sum = sdata->ptrArray_XaCdPos[sdata->ptrArray_firstXaIndex[categoryID] + xas->XaPrefix];

	buf1[0] = 1;
	buf1[1] = xas->XaIndex;
	CdControl(CdlSetfilter, &buf1[0], 0);

	CdIntToPos(sum, (CdlLOC *)&buf2[0]);

	sdata->XA_StartPos = sum;
	sdata->XA_EndPos = sum + xas->XaBytes;
	sdata->XA_MaxSampleVal = 0;
	sdata->XA_MaxSampleValInArr = 0;
	sdata->unused_8008d700 = 0;

	sdata->countPass_CdReadyCallback = 0;
	sdata->countFail_CdReadyCallback = 0;
	sdata->XA_CurrOffset = 0; // ND bug? Variable resuse?
	sdata->countPass_CdTransferCallback = 0;

	if (CdControl(CdlReadS, &buf2[0], 0) == 1)
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
	return 0;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001cf98-0x8001cfec.
void CDSYS_XAPauseRequest()
{
	if (sdata->boolUseDisc == 0)
	{
#if defined(CTR_NATIVE)
		if ((sdata->XA_State >= XA_STARTING) && (sdata->XA_State <= XA_PLAYING))
		{
			sdata->XA_State = XA_FADING;
			sdata->XA_VolumeDeduct = 0x400;
		}
#endif
		return;
	}
	if (sdata->bool_XnfLoaded == 0)
	{
		return;
	}
	if (sdata->XA_State < XA_STARTING)
	{
		return;
	}
	if (sdata->XA_State > XA_PLAYING)
	{
		return;
	}

	sdata->XA_State = XA_FADING;
	sdata->XA_VolumeDeduct = 0x400;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001cfec-0x8001d06c.
void CDSYS_XAPauseForce()
{
	if (sdata->boolUseDisc == 0)
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

	SpuSetIRQ(0);
	CDSYS_SpuDisableIRQ();

	CdControl(CdlPause, 0, 0);

	sdata->XA_PauseFrame = sdata->gGT->frameTimer_MainFrame_ResetDB;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001d06c-0x8001d094.
void CDSYS_XAPauseAtEnd()
{
	if (sdata->boolUseDisc == 0)
	{
#if defined(CTR_NATIVE)
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
#endif
		return;
	}

	// wait until IRQ says XA is finished
	if (sdata->XA_boolFinished == 0)
	{
		return;
	}

	CDSYS_XAPauseForce();
}
