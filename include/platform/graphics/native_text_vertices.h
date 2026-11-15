#ifndef NATIVE_TEXT_VERTICES_H
#define NATIVE_TEXT_VERTICES_H

#include <macros.h>

b32 NativeTextVertices_Set(const void *primitive, float left, float right);
b32 NativeTextVertices_Get(const void *primitive, float *left, float *right);
void NativeTextVertices_EndFrame(void);

#endif
