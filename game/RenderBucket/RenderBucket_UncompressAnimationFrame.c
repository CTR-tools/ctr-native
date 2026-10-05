#include <common.h>

#ifndef CTR_NATIVE
// NOTE(aalhendi): Renderer register ABI, not an ordinary C call: at points to
// scratch; t3 carries the command, s7 the vertex stream, s8 the temporal cursor,
// s1/s3 the MSB-first bit bank, gp the palette, and t4/t5 coordinate masks.
// Cache slots start at scratch 0x140; X/Z/Y accumulators occupy 0x58/0x5a/0x59.
// Returns packed XY/Z in t0/v1. The color load into t1 is in the return delay
// slot, so its value becomes available after one caller instruction.
#include "RenderBucket_DecoderInstructions.inc"
// clang-format off
__asm__(".section .RenderBucket_UncompressAnimationFrame,\"ax\",@progbits\n"
        ".align 2\n"
        ".ent\tRenderBucket_UncompressAnimationFrame\n"
        ".globl RenderBucket_UncompressAnimationFrame\n"
        ".set\tnoreorder\n"
        ".set\tnoat\n"
        "RenderBucket_UncompressAnimationFrame:\n"
        CTR_RB_DECODER_SINGLE_FRAME(".Lrb_decoder_base_")
        CTR_RB_DECODER_CACHE_PACKED_VERTEX
        CTR_RB_DECODER_RETURN_COMMAND_COLOR(".Lrb_decoder_base_")
        ".end\tRenderBucket_UncompressAnimationFrame\n"
        ".size RenderBucket_UncompressAnimationFrame,.-RenderBucket_UncompressAnimationFrame\n"
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
