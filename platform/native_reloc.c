#include <platform/native_reloc.h>

#ifdef CTR_RELOC64

#include <macros.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ---------------------------------------------------------------------------
// On-disc field offsets (retail 32-bit layout). The native C structs grow as
// their pointer fields widen 4->8 bytes, so when reading the raw file we index
// by these fixed offsets and fill the native structs by field name.
// ---------------------------------------------------------------------------

// struct Model (on-disc size 0x18)
#define DISC_MODEL_NAME       0x00
#define DISC_MODEL_ID         0x10
#define DISC_MODEL_NUMHEADERS 0x12
#define DISC_MODEL_HEADERS    0x14

// struct ModelHeader (on-disc size 0x40); pointer slots at 0x20,0x24,0x28,0x2c,0x38,0x3c
#define DISC_MH_SIZE          0x40
#define DISC_MH_NAME          0x00
#define DISC_MH_UNK1          0x10
#define DISC_MH_MAXDIST       0x14
#define DISC_MH_FLAGS         0x16
#define DISC_MH_SCALE         0x18
#define DISC_MH_PADSCALE      0x1e
#define DISC_MH_CMDLIST       0x20
#define DISC_MH_FRAMEDATA     0x24
#define DISC_MH_TEXLAYOUT     0x28
#define DISC_MH_COLORS        0x2c
#define DISC_MH_UNK3          0x30
#define DISC_MH_NUMANIM       0x34
#define DISC_MH_ANIMATIONS    0x38
#define DISC_MH_ANIMTEX       0x3c

// struct ModelAnim (on-disc header size 0x18); pointer slot at 0x14
#define DISC_ANIM_SIZE        0x18
#define DISC_ANIM_NAME        0x00
#define DISC_ANIM_NUMFRAMES   0x10
#define DISC_ANIM_FRAMESIZE   0x12
#define DISC_ANIM_DELTAARRAY  0x14

// struct AnimTex (on-disc header size 0xc); pointer slot at 0x0, then ptr array
#define DISC_ANIMTEX_SIZE     0x0c
#define DISC_ANIMTEX_ACTIVE   0x00
#define DISC_ANIMTEX_NUMFRAME 0x04

// struct LevTexLookup (on-disc size 0x10); pointer slots at 0x4 and 0xc
#define DISC_LTL_NUMICON       0x00
#define DISC_LTL_FIRSTICON     0x04
#define DISC_LTL_NUMICONGROUP  0x08
#define DISC_LTL_GROUPPTR      0x0c

// struct IconGroup (header size 0x14, no pointer fields), then Icon* array
#define DISC_ICONGROUP_SIZE    0x14
#define DISC_ICONGROUP_NUMICON 0x12

// ---------------------------------------------------------------------------
// Relocation context
// ---------------------------------------------------------------------------

struct Reloc64Visited
{
	uint32_t srcOff;  // file-relative offset of the source struct
	void *dstPtr;     // rebuilt native struct
};

struct Reloc64Ctx
{
	char *base; // loaded file body; pointer slots hold offsets relative to this

	// Sorted set of pointer-slot offsets from the embedded DRAM pointer map,
	// used to bound length-less pointer arrays (which entries are pointers).
	uint32_t *ptrSet;
	int ptrSetCount;

	// Visited map (src offset -> rebuilt native pointer) for sharing/cycles.
	struct Reloc64Visited *visited;
	int visitedCount;
	int visitedCap;
};

// base + off, or NULL for a 0 offset (NULL pointer / array terminator).
static void *Reloc64_Resolve(struct Reloc64Ctx *ctx, uint32_t off)
{
	if (off == 0)
		return NULL;
	return ctx->base + off;
}

// Is the 4-byte word at file offset `slotOff` a relocatable pointer slot?
static int Reloc64_IsPtrSlot(struct Reloc64Ctx *ctx, uint32_t slotOff)
{
	int lo = 0;
	int hi = ctx->ptrSetCount - 1;
	while (lo <= hi)
	{
		int mid = (lo + hi) >> 1;
		uint32_t v = ctx->ptrSet[mid];
		if (v == slotOff)
			return 1;
		if (v < slotOff)
			lo = mid + 1;
		else
			hi = mid - 1;
	}
	return 0;
}

static void *Reloc64_VisitedGet(struct Reloc64Ctx *ctx, uint32_t srcOff)
{
	for (int i = 0; i < ctx->visitedCount; i++)
	{
		if (ctx->visited[i].srcOff == srcOff)
			return ctx->visited[i].dstPtr;
	}
	return NULL;
}

static void Reloc64_VisitedPut(struct Reloc64Ctx *ctx, uint32_t srcOff, void *dstPtr)
{
	if (ctx->visitedCount == ctx->visitedCap)
	{
		int newCap = (ctx->visitedCap == 0) ? 64 : (ctx->visitedCap * 2);
		ctx->visited = realloc(ctx->visited, (size_t)newCap * sizeof(ctx->visited[0]));
		ctx->visitedCap = newCap;
	}
	ctx->visited[ctx->visitedCount].srcOff = srcOff;
	ctx->visited[ctx->visitedCount].dstPtr = dstPtr;
	ctx->visitedCount++;
}

// Persistent allocation for the rebuilt "spine"; lives in the same MEMPACK
// region as the source asset so the existing pack swap/clear frees both.
static void *Reloc64_Alloc(int size)
{
	return MEMPACK_AllocMem(size);
}

static int Reloc64_CmpU32(const void *a, const void *b)
{
	uint32_t x = *(const uint32_t *)a;
	uint32_t y = *(const uint32_t *)b;
	return (x > y) - (x < y);
}

// ---------------------------------------------------------------------------
// Per-format walkers
// ---------------------------------------------------------------------------

static struct AnimTex *Reloc64_AnimTex(struct Reloc64Ctx *ctx, uint32_t off)
{
	if (off == 0)
		return NULL;
	void *cached = Reloc64_VisitedGet(ctx, off);
	if (cached != NULL)
		return cached;

	char *src = ctx->base + off;
	int numFrames = *(s16 *)(src + DISC_ANIMTEX_NUMFRAME);
	if (numFrames < 0)
		numFrames = 0;

	// Native: [header][IconGroup4* array of numFrames] at sizeof(AnimTex).
	struct AnimTex *dst = Reloc64_Alloc((int)sizeof(struct AnimTex) + numFrames * (int)sizeof(void *));
	Reloc64_VisitedPut(ctx, off, dst);

	dst->ptrActiveTex = Reloc64_Resolve(ctx, *(uint32_t *)(src + DISC_ANIMTEX_ACTIVE)); // leaf
	dst->numFrames = *(s16 *)(src + 0x4);
	dst->frameOffset = *(s16 *)(src + 0x6);
	dst->frameSkip = *(s16 *)(src + 0x8);
	dst->frameCurr = *(s16 *)(src + 0xa);

	// Trailing array of IconGroup4* (leaf targets), 4-byte on disc -> native.
	void **dstArr = (void **)((char *)dst + sizeof(struct AnimTex));
	const uint32_t *srcArr = (const uint32_t *)(src + DISC_ANIMTEX_SIZE);
	for (int i = 0; i < numFrames; i++)
		dstArr[i] = Reloc64_Resolve(ctx, srcArr[i]);

	return dst;
}

static struct ModelAnim *Reloc64_ModelAnim(struct Reloc64Ctx *ctx, uint32_t off)
{
	if (off == 0)
		return NULL;
	void *cached = Reloc64_VisitedGet(ctx, off);
	if (cached != NULL)
		return cached;

	char *src = ctx->base + off;
	int numFrames = *(u16 *)(src + DISC_ANIM_NUMFRAMES);
	int frameSize = *(s16 *)(src + DISC_ANIM_FRAMESIZE);
	int framesBytes = numFrames * frameSize;
	if (framesBytes < 0)
		framesBytes = 0;

	// Native: [widened header][inline frame blob] so MODELANIM_GETFRAME (which
	// adds sizeof(struct ModelAnim)) still lands on the frames.
	struct ModelAnim *dst = Reloc64_Alloc((int)sizeof(struct ModelAnim) + framesBytes);
	Reloc64_VisitedPut(ctx, off, dst);

	memcpy(dst->name, src + DISC_ANIM_NAME, sizeof(dst->name));
	dst->numFrames = *(u16 *)(src + DISC_ANIM_NUMFRAMES);
	dst->frameSize = *(s16 *)(src + DISC_ANIM_FRAMESIZE);
	dst->ptrDeltaArray = Reloc64_Resolve(ctx, *(uint32_t *)(src + DISC_ANIM_DELTAARRAY)); // leaf
	memcpy((char *)dst + sizeof(struct ModelAnim), src + DISC_ANIM_SIZE, (size_t)framesBytes);

	return dst;
}

// Length of a contiguous run of pointer slots starting at file offset
// `startOff` (used for ptrTexLayout, which has no stored count).
static int Reloc64_PtrRunLen(struct Reloc64Ctx *ctx, uint32_t startOff)
{
	int n = 0;
	while (Reloc64_IsPtrSlot(ctx, startOff + (uint32_t)(n * 4)))
		n++;
	return n;
}

static void Reloc64_ModelHeaderInto(struct Reloc64Ctx *ctx, char *src, struct ModelHeader *dst)
{
	memcpy(dst->name, src + DISC_MH_NAME, sizeof(dst->name));
	dst->unk1 = *(int *)(src + DISC_MH_UNK1);
	dst->maxDistanceLOD = *(s16 *)(src + DISC_MH_MAXDIST);
	dst->flags = *(u16 *)(src + DISC_MH_FLAGS);
	memcpy(&dst->scale, src + DISC_MH_SCALE, sizeof(dst->scale));
	dst->_pad_scale = *(s16 *)(src + DISC_MH_PADSCALE);

	// ptrCommandList holds a real pointer (cast to u32* and dereferenced at
	// render time, e.g. RB_Banner.c); widened to uintptr_t. Command-list bytes
	// are a leaf in the original buffer (GPU links bridged at submit time).
	dst->ptrCommandList = (uintptr_t)Reloc64_Resolve(ctx, *(uint32_t *)(src + DISC_MH_CMDLIST));
	dst->ptrFrameData = Reloc64_Resolve(ctx, *(uint32_t *)(src + DISC_MH_FRAMEDATA));   // leaf
	dst->ptrColors = Reloc64_Resolve(ctx, *(uint32_t *)(src + DISC_MH_COLORS));         // leaf
	dst->unk3 = *(u32 *)(src + DISC_MH_UNK3);
	dst->numAnimations = *(u32 *)(src + DISC_MH_NUMANIM);

	// ptrTexLayout: array of TextureLayout* (leaf targets), length-less on disc
	// -> bounded by the contiguous pointer-slot run in the DRAM pointer map.
	uint32_t texOff = *(uint32_t *)(src + DISC_MH_TEXLAYOUT);
	if (texOff != 0)
	{
		int n = Reloc64_PtrRunLen(ctx, texOff);
		struct TextureLayout **arr = Reloc64_Alloc(n * (int)sizeof(void *));
		const uint32_t *srcArr = (const uint32_t *)(ctx->base + texOff);
		for (int i = 0; i < n; i++)
			arr[i] = Reloc64_Resolve(ctx, srcArr[i]);
		dst->ptrTexLayout = arr;
	}
	else
	{
		dst->ptrTexLayout = NULL;
	}

	// ptrAnimations: array of ModelAnim*, length numAnimations.
	uint32_t animOff = *(uint32_t *)(src + DISC_MH_ANIMATIONS);
	if (animOff != 0 && dst->numAnimations != 0)
	{
		int n = (int)dst->numAnimations;
		struct ModelAnim **arr = Reloc64_Alloc(n * (int)sizeof(void *));
		const uint32_t *srcArr = (const uint32_t *)(ctx->base + animOff);
		for (int i = 0; i < n; i++)
			arr[i] = Reloc64_ModelAnim(ctx, srcArr[i]);
		dst->ptrAnimations = arr;
	}
	else
	{
		dst->ptrAnimations = NULL;
	}

	dst->animtex = Reloc64_AnimTex(ctx, *(uint32_t *)(src + DISC_MH_ANIMTEX));
}

static struct Model *Reloc64_Model(struct Reloc64Ctx *ctx, uint32_t off)
{
	if (off == 0)
		return NULL;
	void *cached = Reloc64_VisitedGet(ctx, off);
	if (cached != NULL)
		return cached;

	char *src = ctx->base + off;
	struct Model *dst = Reloc64_Alloc((int)sizeof(struct Model));
	Reloc64_VisitedPut(ctx, off, dst);

	memcpy(dst->name, src + DISC_MODEL_NAME, sizeof(dst->name));
	dst->id = *(s16 *)(src + DISC_MODEL_ID);
	dst->numHeaders = *(s16 *)(src + DISC_MODEL_NUMHEADERS);

	uint32_t headersOff = *(uint32_t *)(src + DISC_MODEL_HEADERS);
	int n = dst->numHeaders;
	if (headersOff != 0 && n > 0)
	{
		// headers is an array of n contiguous ModelHeader structs (0x40 on disc).
		struct ModelHeader *arr = Reloc64_Alloc(n * (int)sizeof(struct ModelHeader));
		for (int i = 0; i < n; i++)
			Reloc64_ModelHeaderInto(ctx, ctx->base + headersOff + (uint32_t)(i * DISC_MH_SIZE), &arr[i]);
		dst->headers = arr;
	}
	else
	{
		dst->headers = NULL;
	}

	return dst;
}

static struct IconGroup *Reloc64_IconGroup(struct Reloc64Ctx *ctx, uint32_t off)
{
	if (off == 0)
		return NULL;
	void *cached = Reloc64_VisitedGet(ctx, off);
	if (cached != NULL)
		return cached;

	char *src = ctx->base + off;
	int numIcons = *(s16 *)(src + DISC_ICONGROUP_NUMICON);
	if (numIcons < 0)
		numIcons = 0;

	// Native: [header (no pointer fields, 0x14)][Icon* array]; ICONGROUP_GETICONS
	// adds sizeof(struct IconGroup) so the array must follow the header.
	struct IconGroup *dst = Reloc64_Alloc((int)sizeof(struct IconGroup) + numIcons * (int)sizeof(void *));
	Reloc64_VisitedPut(ctx, off, dst);

	memcpy(dst, src, DISC_ICONGROUP_SIZE); // name/groupID/numIcons (no pointers)

	void **dstArr = (void **)((char *)dst + sizeof(struct IconGroup));
	const uint32_t *srcArr = (const uint32_t *)(src + DISC_ICONGROUP_SIZE);
	for (int i = 0; i < numIcons; i++)
		dstArr[i] = Reloc64_Resolve(ctx, srcArr[i]); // Icon leaves

	return dst;
}

static struct LevTexLookup *Reloc64_LevTexLookup(struct Reloc64Ctx *ctx, uint32_t off)
{
	if (off == 0)
		return NULL;
	void *cached = Reloc64_VisitedGet(ctx, off);
	if (cached != NULL)
		return cached;

	char *src = ctx->base + off;
	struct LevTexLookup *dst = Reloc64_Alloc((int)sizeof(struct LevTexLookup));
	Reloc64_VisitedPut(ctx, off, dst);

	dst->numIcon = *(int *)(src + DISC_LTL_NUMICON);
	dst->firstIcon = Reloc64_Resolve(ctx, *(uint32_t *)(src + DISC_LTL_FIRSTICON)); // Icon[] leaf
	dst->numIconGroup = *(int *)(src + DISC_LTL_NUMICONGROUP);

	uint32_t groupOff = *(uint32_t *)(src + DISC_LTL_GROUPPTR);
	if (groupOff != 0 && dst->numIconGroup > 0)
	{
		int n = dst->numIconGroup;
		struct IconGroup **arr = Reloc64_Alloc(n * (int)sizeof(void *));
		const uint32_t *srcArr = (const uint32_t *)(ctx->base + groupOff);
		for (int i = 0; i < n; i++)
			arr[i] = Reloc64_IconGroup(ctx, srcArr[i]);
		dst->firstIconGroupPtr = arr;
	}
	else
	{
		dst->firstIconGroupPtr = NULL;
	}

	return dst;
}

// ---------------------------------------------------------------------------
// Native MPK header (replaces the raw `*ptrMPK` / `ptrMPK+4` reads)
// ---------------------------------------------------------------------------

struct Reloc64MpkHeader
{
	uintptr_t icons; // resolved LevTexLookup* (or raw scalar if slot 0 isn't a ptr)
	int numModels;
	struct Model *models[1]; // [numModels + 1], NULL-terminated
};

uintptr_t Reloc64_MpkIcons(const void *mpkHeader)
{
	return ((const struct Reloc64MpkHeader *)mpkHeader)->icons;
}

struct Model **Reloc64_MpkModels(const void *mpkHeader)
{
	// cast away const: callers treat the table as the live PLYROBJECTLIST.
	return (struct Model **)((struct Reloc64MpkHeader *)mpkHeader)->models;
}

void *Reloc64_ModelPack(void *mpkBase, const int *ptrMapOffsets, int numPtrs)
{
	struct Reloc64Ctx ctx;
	ctx.base = (char *)mpkBase;
	ctx.visited = NULL;
	ctx.visitedCount = 0;
	ctx.visitedCap = 0;

	// Build the sorted pointer-slot set (mask low bits like LOAD_RunPtrMap).
	if (numPtrs < 0)
		numPtrs = 0;
	ctx.ptrSet = malloc((size_t)(numPtrs > 0 ? numPtrs : 1) * sizeof(uint32_t));
	ctx.ptrSetCount = numPtrs;
	for (int i = 0; i < numPtrs; i++)
		ctx.ptrSet[i] = (uint32_t)((ptrMapOffsets[i] >> 2) << 2);
	qsort(ctx.ptrSet, (size_t)numPtrs, sizeof(uint32_t), Reloc64_CmpU32);

	// MPK body: [icons][PLYROBJECTLIST: Model* table at +4, NULL-terminated].
	// Slot 0 is a LevTexLookup* when present (it's relocated by retail and cast
	// to LevTexLookup*); if it's not in the pointer map it's a plain scalar.
	uintptr_t icons;
	if (Reloc64_IsPtrSlot(&ctx, 0))
		icons = (uintptr_t)Reloc64_LevTexLookup(&ctx, *(uint32_t *)ctx.base);
	else
		icons = *(uint32_t *)ctx.base;

	// Count table entries: each real entry's slot is in the pointer map; the
	// terminator (0) is not.
	int count = 0;
	while (Reloc64_IsPtrSlot(&ctx, 4u + (uint32_t)(count * 4)))
		count++;

	struct Reloc64MpkHeader *hdr =
	    Reloc64_Alloc((int)offsetof(struct Reloc64MpkHeader, models) + (count + 1) * (int)sizeof(struct Model *));
	hdr->icons = icons;
	hdr->numModels = count;

	const uint32_t *table = (const uint32_t *)(ctx.base + 4);
	for (int i = 0; i < count; i++)
		hdr->models[i] = Reloc64_Model(&ctx, table[i]);
	hdr->models[count] = NULL;

	free(ctx.ptrSet);
	free(ctx.visited);

	return hdr;
}

#endif // CTR_RELOC64
