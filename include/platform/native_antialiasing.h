#ifndef NATIVE_ANTIALIASING_H
#define NATIVE_ANTIALIASING_H

// Presentation-time antialiasing for the presented frame, implemented in
// platform/native_antialiasing.c.
//
// These passes run after the frame is finished: they resolve the edges of the
// source the renderer was about to present, then blit the result into the
// letterboxed viewport with the same filter the plain path would have used.
// Keeping them out of the PSX raster is what leaves the render-scale ladder,
// the VRAM roundtrip that feeds feedback effects, and the dithering and mask
// passes exactly where they were.

// True when the player has antialiasing enabled (g_config.fxaa).
int NativeAntialiasing_Enabled(void);

void NativeAntialiasing_Init(void);
void NativeAntialiasing_Shutdown(void);

// Forces the intermediate target to be reallocated on the next present.
void NativeAntialiasing_InvalidateTarget(void);

// Resolve one VRAM display rect. Returns 0 when antialiasing is off or the
// rect is unusable, leaving the caller to draw VRAM itself.
int NativeAntialiasing_PresentVRAMRect(int x, int y, int w, int h);

// Resolve the scaled main render target. Returns 0 under the same conditions,
// leaving the caller to blit the target directly.
int NativeAntialiasing_PresentMainTarget(void);

#endif
