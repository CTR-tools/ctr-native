#ifndef CTR_NATIVE_TRACK_PREVIEW_H
#define CTR_NATIVE_TRACK_PREVIEW_H

/* Decode the selected track's optional preview.jpg into a VRAM rectangle. */
int NativeTrackPreview_Load(int levelID, int x, int y, int width, int height, int widescreenFactor);

#endif
