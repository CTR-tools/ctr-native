/*
 * Native forwarding derived from REDRIVER2/PsyCross MIT source:
 * externals/PsyCross/src/psx/LIBSPU.C
 * See THIRD_PARTY_NOTICES.md for copyright and license details.
 */

#include <macros.h>
#include <psx/libspu.h>
#include <psx/hwregs.h>

#ifdef CTR_NATIVE
#include <platform/native_audio.h>
#endif

void SpuSetVoicePitch(s32 vNum, u16 pitch)
{
#ifdef CTR_NATIVE
	SpuVoiceAttr attr;
	attr.voice = SPU_VOICECH(vNum);
	attr.mask = SPU_VOICE_PITCH;
	attr.pitch = pitch;
	NativeAudio_SpuSetVoiceAttr(&attr);
#else
	volatile s32 iteration;
	volatile s32 delay;
	volatile u16 *registers = psxSpuSdkRegs;
	registers[vNum * 8 + 2] = pitch;
	// NOTE(aalhendi): The SPU SDK waits two iterations after each voice-register update.
	delay = 1;
	for (iteration = 0; iteration < 2; ++iteration)
		delay *= 13;
#endif
}

void SpuSetVoiceVolume(s32 vNum, s16 volL, s16 volR)
{
#ifdef CTR_NATIVE
	SpuVoiceAttr attr;
	attr.mask = SPU_VOICE_VOLL | SPU_VOICE_VOLR;
	attr.voice = SPU_VOICECH(vNum);
	attr.volume.left = volL;
	attr.volume.right = volR;
	NativeAudio_SpuSetVoiceAttr(&attr);
#else
	volatile s32 iteration;
	volatile s32 delay;
	volatile u16 *registers;
	volL &= 0x7fff;
	registers = psxSpuSdkRegs;
	volR &= 0x7fff;
	registers[vNum * 8] = volL;
	registers[vNum * 8 + 1] = volR;
	// NOTE(aalhendi): Retain the actual SDK hardware-settling protocol, not a host delay.
	delay = 1;
	for (iteration = 0; iteration < 2; ++iteration)
		delay *= 13;
#endif
}
