#include <common.h>

#if defined(CTR_NATIVE)
#include <platform/native_audio.h>
#endif

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
			return 0;

		sdata->XA_State = 3;
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
		return 0;

	if (categoryID >= CDSYS_XA_NUM_TYPES)
		return 0;

	if (xaID >= CDSYS_XAGetNumTracks(categoryID))
		return 0;

	if (sdata->load_inProgress != 0)
	{
		OtherFX_Play(5, 1);
		return 0;
	}

	if (sdata->discMode != DM_AUDIO)
		CDSYS_SetMode_StreamAudio();

	sdata->XA_State = 2;

	int vol = sdata->vol_Voice;
	if (categoryID == CDSYS_XA_TYPE_MUSIC)
		vol = sdata->vol_Music;

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

	sdata->XA_State = 0;
	return 0;
}
