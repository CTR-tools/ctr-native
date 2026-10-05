#include <common.h>

#ifndef CTR_NATIVE
// NOTE(aalhendi): These entries retain the base/paired renderer register ABI.
// They transform uncached packed coordinates before caching and selecting color;
// cached coordinates bypass the transform. Native keeps its existing C helper.
#include "RenderBucket_DecoderInstructions.inc"
// clang-format off
__asm__(".section .RenderBucket_UncompressAnimationFrame_Split,\"ax\",@progbits\n"
        ".align 2\n"
        ".ent\tRenderBucket_UncompressAnimationFrame_Split\n"
        ".globl RenderBucket_UncompressAnimationFrame_Split\n"
        ".set\tnoreorder\n"
        ".set\tnoat\n"
        "RenderBucket_UncompressAnimationFrame_Split:\n"
        CTR_RB_DECODER_SINGLE_FRAME(".Lrb_decoder_split_")
        CTR_RB_DECODER_TRANSFORM_PACKED_VERTEX
        CTR_RB_DECODER_RETURN_COMMAND_COLOR(".Lrb_decoder_split_")
        ".end\tRenderBucket_UncompressAnimationFrame_Split\n"
        ".size RenderBucket_UncompressAnimationFrame_Split,.-RenderBucket_UncompressAnimationFrame_Split\n"
        ".section .RenderBucket_UncompressAnimationFrame_ReflectNextFrame,\"ax\",@progbits\n"
        ".align 2\n"
        ".ent\tRenderBucket_UncompressAnimationFrame_ReflectNextFrame\n"
        ".globl RenderBucket_UncompressAnimationFrame_ReflectNextFrame\n"
        "RenderBucket_UncompressAnimationFrame_ReflectNextFrame:\n"
        CTR_RB_DECODER_PAIRED_FRAMES(".Lrb_decoder_reflect_")
        CTR_RB_DECODER_TRANSFORM_PACKED_VERTEX
        CTR_RB_DECODER_RETURN_COMMAND_COLOR(".Lrb_decoder_reflect_")
        ".end\tRenderBucket_UncompressAnimationFrame_ReflectNextFrame\n"
        ".size RenderBucket_UncompressAnimationFrame_ReflectNextFrame,.-RenderBucket_UncompressAnimationFrame_ReflectNextFrame\n"
        ".set\tat\n"
        ".set\treorder\n"
        ".text\n");
// clang-format on
#undef CTR_RB_DECODER_SINGLE_FRAME
#undef CTR_RB_DECODER_PAIRED_FRAMES
#undef CTR_RB_DECODER_CACHE_PACKED_VERTEX
#undef CTR_RB_DECODER_TRANSFORM_PACKED_VERTEX
#undef CTR_RB_DECODER_RETURN_COMMAND_COLOR
#endif
