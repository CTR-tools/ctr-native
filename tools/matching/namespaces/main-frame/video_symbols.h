#ifndef CTR_MAIN_FRAME_VIDEO_SYMBOLS_H
#define CTR_MAIN_FRAME_VIDEO_SYMBOLS_H

#include <common.h>

// NOTE(aalhendi): Retail publishes the low halves of these existing resident
// slots. Native keeps the complete int fields and signed destination inputs.
extern s16 ctr_mainFrameVideoDstX;
extern s16 ctr_mainFrameVideoDstY;
#define MAIN_FRAME_VIDEO_DST_X ctr_mainFrameVideoDstX
#define MAIN_FRAME_VIDEO_DST_Y ctr_mainFrameVideoDstY

#endif
