#ifndef NATIVE_OVERLAY_H
#define NATIVE_OVERLAY_H

// MultiTrack: F12 debug overlay. Lists what every F key does and its current
// setting, the ray tracer's settings (keys 1-8 while it is open) and timings.

#include <macros.h>

int Overlay_IsOpen(void);
void Overlay_Toggle(void);
int Overlay_HandleKey(int scancode, int down, int shift); // 1: consumed (keys 1-8 while open)
// text of this frame's overlay rasterised: RGBA rows top-down, NULL when closed
const u8 *Overlay_Rasterize(int *w, int *h);

#endif
