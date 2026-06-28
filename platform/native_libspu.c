#include <macros.h>
#include <platform/native_audio.h>
#include <psx/libetc.h>
#include <psx/libspu.h>

#include <string.h>

// NOTE(aalhendi): Native libspu preserves behavior from PsyCross's
// MIT-licensed LIBSPU.C while moving host SPU ownership into ctr-native.
// See THIRD_PARTY_NOTICES.md.

global_variable s32 s_inTransfer = 0;
global_variable s32 s_transferMode = SPU_TRANSFER_BY_DMA;
global_variable SpuTransferCallbackProc s_transferCallback = NULL;
global_variable s32 s_spuIRQEnabled = 0;
global_variable u32 s_spuIRQAddr = 0;
global_variable SpuIRQCallbackProc s_spuIRQCallback = NULL;

unsigned int SpuWrite(unsigned char *addr, unsigned int size)
{
	u32 result = NativeAudio_SpuWrite(addr, size);

	if (s_transferCallback)
	{
		s_transferCallback();
	}
	else
	{
		s_inTransfer = 0;
	}

	return result;
}

int SpuSetTransferMode(int mode)
{
	s32 modeFix = mode == 0 ? 0 : 1;

	s_transferMode = modeFix;
	return modeFix;
}

unsigned int SpuSetTransferStartAddr(unsigned int addr)
{
	return NativeAudio_SpuSetTransferStartAddr(addr);
}

int SpuIsTransferCompleted(int flag)
{
	(void)flag;
	(void)s_transferMode;
	(void)s_inTransfer;
	return 1;
}

void SpuInit(void)
{
	ResetCallback();
	NativeAudio_SpuInit();
}

internal void NativeLibSpu_SetVoiceAttr(SpuVoiceAttr *arg)
{
	NativeAudio_SpuSetVoiceAttr(arg);
}

void SpuSetKey(int on_off, unsigned int voice_bit)
{
	NativeAudio_SpuSetKey(on_off, voice_bit);
}

int SpuSetReverb(int on_off)
{
	return NativeAudio_SpuSetReverb(on_off);
}

int SpuSetReverbModeParam(SpuReverbAttr *attr)
{
	return NativeAudio_SpuSetReverbModeParam(attr);
}

unsigned int SpuSetReverbVoice(int on_off, unsigned int voice_bit)
{
	return NativeAudio_SpuSetReverbVoice(on_off, voice_bit);
}

void SpuSetCommonMasterVolume(short mvol_left, short mvol_right)
{
	NativeAudio_SpuSetCommonMasterVolume(mvol_left, mvol_right);
}

void SpuSetReverbModeDepth(short depth_left, short depth_right)
{
	NativeAudio_SpuSetReverbModeDepth(depth_left, depth_right);
}

#define VOICE_ATTRIB_SETTER_SHORTCUT(flag, field, value) \
	SpuVoiceAttr attr;                                   \
	attr.voice = SPU_VOICECH(vNum);                      \
	attr.mask = flag;                                    \
	attr.field = value;                                  \
	NativeLibSpu_SetVoiceAttr(&attr)

void SpuSetVoiceVolume(int vNum, short volL, short volR)
{
	SpuVoiceAttr attr;

	attr.mask = SPU_VOICE_VOLL | SPU_VOICE_VOLR;
	attr.voice = SPU_VOICECH(vNum);
	attr.volume.left = volL;
	attr.volume.right = volR;

	NativeLibSpu_SetVoiceAttr(&attr);
}

void SpuSetVoicePitch(int vNum, unsigned short pitch)
{
	VOICE_ATTRIB_SETTER_SHORTCUT(SPU_VOICE_PITCH, pitch, pitch);
}

void SpuSetVoiceStartAddr(int vNum, unsigned int startAddr)
{
	VOICE_ATTRIB_SETTER_SHORTCUT(SPU_VOICE_WDSA, addr, startAddr);
}

void SpuSetVoiceADSRAttr(int vNum, unsigned short AR, unsigned short DR, unsigned short SR, unsigned short RR, unsigned short SL, int ARmode, int SRmode,
                         int RRmode)
{
	SpuVoiceAttr attr;

	attr.mask = SPU_VOICE_ADSR_AR | SPU_VOICE_ADSR_DR | SPU_VOICE_ADSR_SR | SPU_VOICE_ADSR_RR | SPU_VOICE_ADSR_SL | SPU_VOICE_ADSR_AMODE |
	            SPU_VOICE_ADSR_SMODE | SPU_VOICE_ADSR_RMODE;

	attr.voice = SPU_VOICECH(vNum);
	attr.ar = AR;
	attr.dr = DR;
	attr.sr = SR;
	attr.rr = RR;
	attr.sl = SL;
	attr.a_mode = ARmode;
	attr.s_mode = SRmode;
	attr.r_mode = RRmode;

	NativeLibSpu_SetVoiceAttr(&attr);
}

SpuTransferCallbackProc SpuSetTransferCallback(SpuTransferCallbackProc func)
{
	SpuTransferCallbackProc oldFn = s_transferCallback;
	s_transferCallback = func;
	return oldFn;
}

int SpuReadDecodedData(SpuDecodedData *d_data, int flag)
{
	size_t byteCount = sizeof(*d_data);

	if (flag == SPU_CDONLY)
	{
		byteCount = sizeof(d_data->cd_left) + sizeof(d_data->cd_right);
	}

	if (d_data != NULL)
	{
		memset(d_data, 0, byteCount);
	}

	return SPU_DECODED_FIRSTHALF;
}

int SpuSetIRQ(int on_off)
{
	s32 old = s_spuIRQEnabled;
	s_spuIRQEnabled = on_off != 0;
	return old;
}

unsigned int SpuSetIRQAddr(unsigned int x)
{
	u32 old = s_spuIRQAddr;
	s_spuIRQAddr = x;
	return old;
}

SpuIRQCallbackProc SpuSetIRQCallback(SpuIRQCallbackProc x)
{
	SpuIRQCallbackProc old = s_spuIRQCallback;
	s_spuIRQCallback = x;
	return old;
}

void SpuSetCommonCDMix(int cd_mix)
{
	NativeAudio_SpuSetCommonCDMix(cd_mix);
}

void SpuSetCommonCDVolume(short cd_left, short cd_right)
{
	NativeAudio_SpuSetCommonCDVolume(cd_left, cd_right);
}

void SpuSetCommonCDReverb(int cd_reverb)
{
	NativeAudio_SpuSetCommonCDReverb(cd_reverb);
}
