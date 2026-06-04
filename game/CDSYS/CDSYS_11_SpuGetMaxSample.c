#include <common.h>

#if defined(CTR_NATIVE)
#include <platform/native_audio.h>
#endif

#if defined(CTR_NATIVE)
static void CDSYS_SaveMaxSample(int max)
{
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
		return;

	int start = 0x100;
	int end = 0x200;
	if (sdata->irqAddr == 0)
	{
		start = 0;
		end = 0x100;
	}

	s16 *ptrSpuBuf = &sdata->SpuDecodedBuf[start];

	// absolute value, find max in block
	for (int i = start; i < end; i++)
	{
		sample = *ptrSpuBuf++;
		if (sample < 0)
			sample = -sample;
		if (max < sample)
			max = sample;
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
