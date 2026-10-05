#ifndef RENDER_BUCKET_EXECUTE_SCRATCH_H
#define RENDER_BUCKET_EXECUTE_SCRATCH_H

#include <ctr_math.h>

// NOTE(aalhendi): Execute and its primitive writers share this scratchpad view.
// The dispatch and split views reuse the same bytes; this declares no storage.
struct RenderBucketFrameOriginScratch
{
	SVec2 xy;
	s32 z;
};

union RenderBucketScratchColor
{
	struct
	{
		u8 r;
		u8 g;
		u8 b;
		u8 code;
	} colorCode;
	u32 word;
};

struct RenderBucketSplitScratch
{
	u8 pad_000[0x16];
	s16 splitLineSecondary;
	u8 pad_018[0x04];
	u32 splitCounterPrimary;
	u8 pad_020[0x0e];
	s16 splitLineTertiary;
	u8 pad_030[0x04];
	u32 splitCounterSecondary;
	u8 pad_038[0x14];
	u32 depthFadeColors[3];
	u32 splitFixedLine;
	s16 alphaScale;
	u8 pad_05e[0x02];
	union RenderBucketScratchColor fadeColor;
};

struct RenderBucketExecuteScratch
{
	u32 savedSp;
	u32 nextEntryPtr32;
	u32 pushBufferPtr32;
	u32 primMemPtr32;
	u32 instPtr32;
	u8 pad_014[0x08];
	s16 geomW;
	s16 geomH;
	u32 pad_020;
	u32 instFlags;
	u8 pad_028[0x04];
	u32 depthMac0;
	struct RenderBucketFrameOriginScratch frameOrigin;
	u32 activeOTPtr32;
	u32 primaryOTPtr32;
	u32 secondaryOTPtr32;
	s16 splitLinePrimary;
	u8 pad_046[0x02];
	u8 splitInstanceSpecLightX;
	u8 pad_049[0x03];
	u32 splitReflectionRGBA;
	u32 texturePtr32;
	u8 pad_054[0x04];
	u32 setupDrawClear;
	u8 pad_05c[0x38];
	u32 setupDispatch[8];
	SVec3Slot rawView;
	u8 pad_0bc[0x08];
	union
	{
		u32 primDispatch[8];
		struct RenderBucketSplitScratch split;
	} primitiveState;
	u8 pad_128[0x0c];
	s16 minDepthByteOffset;
	s16 maxDepthByteOffset;
};

CTR_STATIC_ASSERT(sizeof(struct RenderBucketFrameOriginScratch) == 0x8);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketFrameOriginScratch, xy) == 0x00);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketFrameOriginScratch, z) == 0x04);
CTR_STATIC_ASSERT(sizeof(union RenderBucketScratchColor) == 0x4);
CTR_STATIC_ASSERT(sizeof(struct RenderBucketSplitScratch) == 0x64);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketExecuteScratch, savedSp) == 0x00);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketExecuteScratch, nextEntryPtr32) == 0x04);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketExecuteScratch, pushBufferPtr32) == 0x08);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketExecuteScratch, primMemPtr32) == 0x0c);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketExecuteScratch, instPtr32) == 0x10);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketExecuteScratch, geomW) == 0x1c);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketExecuteScratch, geomH) == 0x1e);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketExecuteScratch, instFlags) == 0x24);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketExecuteScratch, depthMac0) == 0x2c);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketExecuteScratch, frameOrigin) == 0x30);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketExecuteScratch, activeOTPtr32) == 0x38);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketExecuteScratch, primaryOTPtr32) == 0x3c);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketExecuteScratch, secondaryOTPtr32) == 0x40);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketExecuteScratch, splitLinePrimary) == 0x44);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketExecuteScratch, splitInstanceSpecLightX) == 0x48);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketExecuteScratch, splitReflectionRGBA) == 0x4c);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketExecuteScratch, texturePtr32) == 0x50);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketExecuteScratch, setupDrawClear) == 0x58);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketExecuteScratch, setupDispatch) == 0x94);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketExecuteScratch, rawView) == 0xb4);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketExecuteScratch, primitiveState.primDispatch) == 0xc4);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketExecuteScratch, primitiveState.split) == 0xc4);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketSplitScratch, splitLineSecondary) == 0x16);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketSplitScratch, splitCounterPrimary) == 0x1c);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketSplitScratch, splitLineTertiary) == 0x2e);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketSplitScratch, splitCounterSecondary) == 0x34);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketSplitScratch, splitFixedLine) == 0x58);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketSplitScratch, alphaScale) == 0x5c);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketSplitScratch, fadeColor) == 0x60);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketExecuteScratch, primitiveState.split.splitLineSecondary) == 0xda);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketExecuteScratch, primitiveState.split.splitCounterPrimary) == 0xe0);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketExecuteScratch, primitiveState.split.splitLineTertiary) == 0xf2);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketExecuteScratch, primitiveState.split.splitCounterSecondary) == 0xf8);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketExecuteScratch, primitiveState.split.splitFixedLine) == 0x11c);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketExecuteScratch, primitiveState.split.alphaScale) == 0x120);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketExecuteScratch, primitiveState.split.fadeColor) == 0x124);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketExecuteScratch, primitiveState.split.depthFadeColors) == 0x110);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketExecuteScratch, minDepthByteOffset) == 0x134);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketExecuteScratch, maxDepthByteOffset) == 0x136);
CTR_STATIC_ASSERT(sizeof(struct RenderBucketExecuteScratch) == 0x138);

#endif
