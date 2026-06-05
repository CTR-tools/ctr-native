#ifndef NATIVE_STR_H
#define NATIVE_STR_H

#include <psx/types.h>

s32 NativeSTR_StartTrackPreview(s32 bigfileIndex, s32 frameCount);
s32 NativeSTR_StartScrapbook(void);
void NativeSTR_Stop(void);
s32 NativeSTR_UploadNextFrame(s32 dstX, s32 dstY);

#endif
