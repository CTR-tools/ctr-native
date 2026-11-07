#ifndef PLATFORM_NATIVE_AUDIO_H
#define PLATFORM_NATIVE_AUDIO_H

#include <macros.h>
#include <psx/libspu.h>

s32 NativeAudio_SpuInit(void);
u32 NativeAudio_SpuSetTransferStartAddr(u32 addr);
u32 NativeAudio_SpuWrite(const u8 *addr, u32 size);
void NativeAudio_SpuSetVoiceAttr(SpuVoiceAttr *psxAttrib);
void NativeAudio_SpuSetKey(s32 on_off, u32 voice_bit);
s32 NativeAudio_SpuSetReverb(s32 on_off);
s32 NativeAudio_SpuSetReverbModeParam(SpuReverbAttr *attr);
void NativeAudio_SpuSetReverbModeDepth(s16 left, s16 right);
u32 NativeAudio_SpuSetReverbVoice(s32 on_off, u32 voice_bit);
void NativeAudio_SpuSetCommonMasterVolume(s16 left, s16 right);
void NativeAudio_SpuSetCommonCDMix(s32 enabled);
void NativeAudio_SpuSetCommonCDVolume(s16 left, s16 right);
void NativeAudio_SpuSetCommonCDReverb(s32 enabled);

int NativeAudio_PlayXATrack(int categoryID, int xaID, int volumeLeft, int volumeRight);
int NativeAudio_PlayLocalizedXATrack(int categoryID, int xaID, int volumeLeft, int volumeRight);
void NativeAudio_RefreshVoiceLanguage(void);
int NativeAudio_PlayXAFile(const char *relativePath, int channelFilter, int volumeLeft, int volumeRight);
int NativeAudio_GetXATrackLength(int categoryID, int xaID);
int NativeAudio_IsXAPlaying(void);
int NativeAudio_GetXACurrOffset(void);
int NativeAudio_GetXAMaxSample(void);
int NativeAudio_GetXAMaxSampleAtOffset(int xaCurrOffset);
void NativeAudio_SetXAVolume(int volumeLeft, int volumeRight);
void NativeAudio_StopXA(void);
void NativeAudio_StepVBlank(void);
int NativeAudio_RenderFrames(s16 *out, int frameCount);
void NativeAudio_SetDeterministicRenderMode(int enabled);
int NativeAudio_IsDeterministicRenderMode(void);
int NativeAudio_QueueRenderedFrames(const s16 *frames, int frameCount);
void NativeAudio_ClearOutputQueue(void);
void NativeAudio_SetPausedVoices(u32 voices, b32 pauseXA);
void NativeAudio_Shutdown(void);
#ifdef CTR_INTERNAL
void NativeAudio_GetOutputStats(int *underrunFrames, int *overflowFrames, int *queuedFrames);
#endif
int NativeAudio_GetStateSize(void);
int NativeAudio_CaptureState(void *dst, int dstSize);
int NativeAudio_RestoreState(const void *src, int srcSize);

#endif
