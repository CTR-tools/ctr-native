#ifndef CTR_NATIVE_TRACK_JPEG_DECODER_H
#define CTR_NATIVE_TRACK_JPEG_DECODER_H

/* Keep diagnostics from SDL's bundled decoder separate from application code. */
#if defined(__GNUC__)
#pragma GCC system_header
#endif

#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#define STBI_MAX_DIMENSIONS 4096
#define STB_IMAGE_IMPLEMENTATION
#include "../externals/SDL/src/video/stb_image.h"

#endif
