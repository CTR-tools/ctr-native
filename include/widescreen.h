#ifndef WIDESCREEN_H
#define WIDESCREEN_H

int Widescreen_GetFactor(void);
int Widescreen_ScaleWidth(int width);
// Per-edge inset when both edges move toward the center.
int Widescreen_CenteredEdgeShift(int width);
// Total inset when the opposite edge remains fixed.
int Widescreen_FixedEdgeShift(int width);
int Widescreen_ScaleXFromAnchor(int x, int anchor);
void Widescreen_CompressRect(RECT *rect);

void Widescreen_CompressFT4(POLY_FT4 *p);
void Widescreen_CompressGT4(POLY_GT4 *p);
void Widescreen_CompressGT4Scaled(POLY_GT4 *p, int unscaledWidth, int scale);
void Widescreen_CompressNative(PolyFT4 *p);

#endif
