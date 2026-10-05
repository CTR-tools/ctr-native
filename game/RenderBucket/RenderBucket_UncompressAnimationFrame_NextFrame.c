#include <common.h>

#ifdef CTR_NATIVE
// NOTE(aalhendi): Native carries the renderer register ABI in the draw
// context. Both streams share the temporal cursor; their bit banks and
// signed-byte accumulators advance independently.
static struct RenderBucketUncompressResult RenderBucket_UncompressAnimationFrame_NextFrame(struct RenderBucketDrawContext *ctx, u32 command, u16 stackIndex)
{
	struct RenderBucketUncompressResult result;
	u32 *deltaArray = (u32 *)ctx->idpp->ptrDeltaArray;
	u8 flags = (command >> 24) & 0xff;
	RenderBucketVertex nextVertex;

	// Native carries retail register state in the explicit draw context.
	if ((ctx->idpp->ptrNextFrame == 0) || (ctx->nextVertData == 0))
	{
		return RenderBucket_UncompressAnimationFrame(ctx, command, stackIndex);
	}

	if ((flags & 4) != 0)
	{
		struct RenderBucketPackedVertex *scratchVertex = RenderBucket_PackedVertexScratch(stackIndex);

		// NOTE(aalhendi): Retail next-frame entry shares the cached-vertex
		// command bit path with 0x8006a8e0 before returning color.
		result.packed = *scratchVertex;
		result.color = RenderBucket_GetCommandColor(ctx, command);
		ctx->packedStack[stackIndex] = result.packed;
		return result;
	}

	if (deltaArray != 0)
	{
		u32 temporal = deltaArray[ctx->vertexIndex];
		u8 xBits = (temporal >> 6) & 7;
		u8 zBits = (temporal >> 3) & 7;
		u8 yBits = temporal & 7;
		int bx = RenderBucket_SignExtendBits(temporal >> 25, 7) * 2;
		int bz = RenderBucket_SignExtendBits(temporal >> 17, 8);
		int by = RenderBucket_SignExtendBits(temporal >> 9, 8);

		// NOTE(aalhendi): Retail saves the virtual slot base, before +0x140 payload.
		*CTR_SCRATCHPAD_PTR(u32, 0x20) = CTR_SCRATCHPAD_ADDR + ((command >> 13) & 0x7f8u);

		// Current and next compressed streams use the same temporal word.
		if (ctx->nextBitIndex == 0)
		{
			int width = xBits + 1;
			u32 fieldMask = (1u << width) - 1u;
			int nextField;

			// NOTE(aalhendi): Execute clears current bytes only. Preserve the
			// actual cold next X/Z/Y scratch bytes before their first use.
			ctx->x_next_alu = RenderBucket_SignedByte(*CTR_SCRATCHPAD_PTR(u8, 0x5c));
			ctx->y_next_alu = RenderBucket_SignedByte(*CTR_SCRATCHPAD_PTR(u8, 0x5e));
			ctx->z_next_alu = RenderBucket_SignedByte(*CTR_SCRATCHPAD_PTR(u8, 0x5d));

			RenderBucket_ReadDeltaComponent(ctx, xBits, bx, &ctx->x_alu);
			nextField = RenderBucket_GetSignedBits((const u8 *)ctx->nextVertData, &ctx->nextBitIndex, width);
			// Retail combines the old TRZ prefix with the fresh field before
			// bias/accumulation; future bank bits remain solely the fresh stream.
			nextField = RenderBucket_SignExtendBits(((u32)nextField & fieldMask) | ((u32)ctx->idpp->mvp.t[2] >> (32 - width)), width);
			ctx->x_next_alu = RenderBucket_SignedByte(xBits == 7 ? nextField : ctx->x_next_alu + nextField + bx);
		}
		else
		{
			RenderBucket_ReadNextFrameDeltaComponent(ctx, xBits, bx, &ctx->x_alu, &ctx->x_next_alu);
		}
		*CTR_SCRATCHPAD_PTR(u8, 0x58) = (u8)ctx->x_alu;
		*CTR_SCRATCHPAD_PTR(u8, 0x5c) = (u8)ctx->x_next_alu;
		RenderBucket_ReadNextFrameDeltaComponent(ctx, zBits, bz, &ctx->y_alu, &ctx->y_next_alu);
		*CTR_SCRATCHPAD_PTR(u8, 0x5a) = (u8)ctx->y_alu;
		*CTR_SCRATCHPAD_PTR(u8, 0x5e) = (u8)ctx->y_next_alu;
		RenderBucket_ReadNextFrameDeltaComponent(ctx, yBits, by, &ctx->z_alu, &ctx->z_next_alu);
		*CTR_SCRATCHPAD_PTR(u8, 0x59) = (u8)ctx->z_alu;
		*CTR_SCRATCHPAD_PTR(u8, 0x5d) = (u8)ctx->z_next_alu;

		ctx->stack[stackIndex].x = ctx->x_alu;
		ctx->stack[stackIndex].y = ctx->z_alu;
		ctx->stack[stackIndex].z = ctx->y_alu;
		nextVertex.x = ctx->x_next_alu;
		nextVertex.y = ctx->z_next_alu;
		nextVertex.z = ctx->y_next_alu;
	}
	else
	{
		RenderBucketCompVertex *currVerts = (RenderBucketCompVertex *)ctx->vertData;
		RenderBucketCompVertex *nextVerts = (RenderBucketCompVertex *)ctx->nextVertData;

		// Both frame vertex streams advance together before the interpolated pack.
		ctx->stack[stackIndex].x = currVerts[ctx->vertexIndex].x;
		ctx->stack[stackIndex].y = currVerts[ctx->vertexIndex].y;
		ctx->stack[stackIndex].z = currVerts[ctx->vertexIndex].z;
		nextVertex.x = nextVerts[ctx->vertexIndex].x;
		nextVertex.y = nextVerts[ctx->vertexIndex].y;
		nextVertex.z = nextVerts[ctx->vertexIndex].z;
	}

	result.packed = RenderBucket_CacheInterpolatedPackedVertex(ctx, stackIndex, &ctx->stack[stackIndex], &nextVertex);
	result.color = RenderBucket_GetCommandColor(ctx, command);
	return result;
}
#else
// NOTE(aalhendi): Renderer register ABI: at=scratch1f800000; t3=command;
// t4/t5=packed masks; s7/t8=current/next stream; s8=shared temporal cursor;
// s1/s2=independent MSB banks; s3=one shared signed bit count; gp=palette.
// a1 slot pointer is saved at scratch20 only for delta. Byte accumulators
// are current58/5a/59 and next5c/5e/5d. t0/v1 return packed coordinates;
// t1 color is pending at return and retires after the actual caller instruction.
// The split decoder is a separate entry, not part of this register contract.
#include "RenderBucket_DecoderInstructions.inc"
// clang-format off
__asm__(".section .RenderBucket_UncompressAnimationFrame_NextFrame,\"ax\",@progbits\n"
        ".align 2\n"
        ".ent\tRenderBucket_UncompressAnimationFrame_NextFrame\n"
        ".globl RenderBucket_UncompressAnimationFrame_NextFrame\n"
        ".set\tnoreorder\n"
        ".set\tnoat\n"
        "RenderBucket_UncompressAnimationFrame_NextFrame:\n"
        CTR_RB_DECODER_PAIRED_FRAMES(".Lrb_decoder_next_")
        CTR_RB_DECODER_CACHE_PACKED_VERTEX
        CTR_RB_DECODER_RETURN_COMMAND_COLOR(".Lrb_decoder_next_")
        ".end\tRenderBucket_UncompressAnimationFrame_NextFrame\n"
        ".size RenderBucket_UncompressAnimationFrame_NextFrame,.-RenderBucket_UncompressAnimationFrame_NextFrame\n"
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
