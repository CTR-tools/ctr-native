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
//
// MEMPACK_AllocMem never reports OOM (retail shows an error screen and halts
// forever, see MEMPACK.c) -- a decode bug that turns a stray count into
// garbage previously manifested as that permanent halt with no diagnostic,
// burning the whole pool one call at a time. Catch absurd requests here
// instead and fail loudly; this is native-only relocation-walker plumbing,
// not shipped retail behavior.
static void *Reloc64_Alloc(int size)
{
	enum
	{
		RELOC64_ALLOC_SANITY_MAX = 16 * 1024 * 1024
	};
	if (size < 0 || size > RELOC64_ALLOC_SANITY_MAX)
	{
		fprintf(stderr, "[Reloc64] refusing to allocate %d bytes (sanity cap %d) -- a relocation walker likely misread a count/offset\n", size,
		        (int)RELOC64_ALLOC_SANITY_MAX);
		abort();
	}
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

// AnimTex forms a chain: each entry is [header][IconGroup4* x numFrames],
// immediately followed by the next entry (see CTR_CycleTex_LEV/Model, which
// walk it via `curAnimTex = &ptrArray[curAnimTex->numFrames]`). The chain
// self-terminates when an entry's ptrActiveTex slot holds the chain head's
// own (pre-relocation) file offset -- there is no stored count. `off` is
// assumed to be the chain head (true for every caller: ModelHeader.animtex
// and Level.ptr_anim_tex both store the head's offset directly). The whole
// chain is rebuilt into one contiguous native allocation so the retail
// pointer-arithmetic walk still lands on each next entry at native width.
static struct AnimTex *Reloc64_AnimTex(struct Reloc64Ctx *ctx, uint32_t firstOff)
{
	if (firstOff == 0)
		return NULL;
	void *cached = Reloc64_VisitedGet(ctx, firstOff);
	if (cached != NULL)
		return cached;

	enum
	{
		MAX_ANIMTEX_CHAIN = 256
	};
	uint32_t offs[MAX_ANIMTEX_CHAIN];
	int frameCounts[MAX_ANIMTEX_CHAIN];
	int n = 0;
	uint32_t off = firstOff;
	for (;;)
	{
		char *s = ctx->base + off;
		uint32_t rawActive = *(uint32_t *)(s + DISC_ANIMTEX_ACTIVE);
		int numFrames = *(s16 *)(s + DISC_ANIMTEX_NUMFRAME);
		if (numFrames < 0)
			numFrames = 0;

		offs[n] = off;
		frameCounts[n] = numFrames;
		n++;

		if (rawActive == firstOff || n >= MAX_ANIMTEX_CHAIN)
			break;

		off = off + DISC_ANIMTEX_SIZE + (uint32_t)numFrames * 4;
	}

	int sizes[MAX_ANIMTEX_CHAIN];
	int totalSize = 0;
	for (int i = 0; i < n; i++)
	{
		sizes[i] = (int)sizeof(struct AnimTex) + frameCounts[i] * (int)sizeof(void *);
		totalSize += sizes[i];
	}

	char *buf = Reloc64_Alloc(totalSize);
	char *cursor = buf;
	for (int i = 0; i < n; i++)
	{
		struct AnimTex *dst = (struct AnimTex *)cursor;
		Reloc64_VisitedPut(ctx, offs[i], dst);

		char *s = ctx->base + offs[i];
		dst->numFrames = (s16)frameCounts[i];
		dst->frameOffset = *(s16 *)(s + 0x6);
		dst->frameSkip = *(s16 *)(s + 0x8);
		dst->frameCurr = *(s16 *)(s + 0xa);

		void **dstArr = (void **)(cursor + sizeof(struct AnimTex));
		const uint32_t *srcArr = (const uint32_t *)(s + DISC_ANIMTEX_SIZE);
		for (int j = 0; j < frameCounts[i]; j++)
			dstArr[j] = Reloc64_Resolve(ctx, srcArr[j]);

		// Terminator: must literally be the native chain-head pointer, since
		// CTR_CycleTex_LEV/Model compares *(int*)curAnimTex against
		// (int)animtex (the head pointer passed in by the caller).
		dst->ptrActiveTex =
		    (i == n - 1) ? (int *)(void *)buf : (int *)Reloc64_Resolve(ctx, *(uint32_t *)(s + DISC_ANIMTEX_ACTIVE));

		cursor += sizes[i];
	}

	return (struct AnimTex *)buf;
}

// Lookup-only accessor for a tagged QuadBlock.ptr_texture_mid reference into
// an already-walked AnimTex chain (see Reloc64_TexMidPtr below). Falls back
// to rebuilding starting at `off` if the chain wasn't visited yet -- shouldn't
// happen as long as Reloc64_Level walks ptr_anim_tex before mesh_info/quads.
static struct AnimTex *Reloc64_AnimTexNode(struct Reloc64Ctx *ctx, uint32_t off)
{
	void *cached = Reloc64_VisitedGet(ctx, off);
	if (cached != NULL)
		return cached;
	return Reloc64_AnimTex(ctx, off);
}

// QuadBlock.ptr_texture_mid[i]: low bit tags "pointer to an AnimTex's
// ptrActiveTex field" (the retail indirection, see
// DrawLevelOvr1P_ResolveTexturePointer in 226_00_DrawLevelOvr1P.c); untagged
// is a plain IconGroup4 leaf. ptrActiveTex is AnimTex's first field, so the
// masked offset is the AnimTex's own file offset.
static void *Reloc64_TexMidPtr(struct Reloc64Ctx *ctx, uint32_t raw)
{
	if (raw == 0)
		return NULL;
	if ((raw & 1) != 0)
	{
		struct AnimTex *at = Reloc64_AnimTexNode(ctx, raw & ~1u);
		return (void *)(((uintptr_t)&at->ptrActiveTex) | 1);
	}
	return ctx->base + raw; // leaf IconGroup4
}

static struct ModelAnim *Reloc64_ModelAnim(struct Reloc64Ctx *ctx, uint32_t off)
{
	if (off == 0)
		return NULL;
	void *cached = Reloc64_VisitedGet(ctx, off);
	if (cached != NULL)
		return cached;

	char *src = ctx->base + off;
	// Top bit of numFrames is an interpolation flag, not part of the count
	// (see INSTANCE.c:407 "remember it's masked due to interp flag", and the
	// same `& 0x7fff` at CS_Instance.c:155 / VehFrame.c:44 /
	// RenderBucket_QueueExecute.c:1876). Masking here only sizes the frame
	// blob; dst->numFrames below keeps the raw value so retail's own masking
	// at each read site still works unchanged.
	int numFrames = *(u16 *)(src + DISC_ANIM_NUMFRAMES) & 0x7fff;
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

// ---------------------------------------------------------------------------
// Level format (Phase 2)
// ---------------------------------------------------------------------------

// struct QuadBlock (on-disc size 0x5c)
#define DISC_QB_SIZE          0x5c
#define DISC_QB_INDEX         0x00
#define DISC_QB_QUADFLAGS     0x12
#define DISC_QB_DRAWORDERLOW  0x14
#define DISC_QB_DRAWORDERHIGH 0x18
#define DISC_QB_PTRTEXMID     0x1c
#define DISC_QB_BBOX          0x2c
#define DISC_QB_TERRAINTYPE   0x38
#define DISC_QB_BLOCKID       0x3c
#define DISC_QB_CHECKPOINTIDX 0x3e
#define DISC_QB_PTRTEXLOW     0x40
#define DISC_QB_PVS           0x44
#define DISC_QB_TRINORMALDIV  0x48

// struct PVS (on-disc size 0x10, all 4 fields pointers)
#define DISC_PVS_SIZE        0x10
#define DISC_PVS_VISLEAFSRC  0x00
#define DISC_PVS_VISFACESRC  0x04
#define DISC_PVS_VISINSTSRC  0x08
#define DISC_PVS_VISEXTRASRC 0x0c

// struct BSP (on-disc size 0x20)
#define DISC_BSP_SIZE         0x20
#define DISC_BSP_FLAG         0x00
#define DISC_BSP_ID           0x02
#define DISC_BSP_BOX          0x04
#define DISC_BSP_BRANCH_AXIS    0x10
#define DISC_BSP_BRANCH_CHILDID 0x18
#define DISC_BSP_LEAF_UNK1      0x10
#define DISC_BSP_LEAF_HITBOXARR 0x14
#define DISC_BSP_LEAF_NUMQUADS  0x18
#define DISC_BSP_LEAF_QBARRAY   0x1c
#define DISC_BSP_HITBOX_CENTER  0x10
#define DISC_BSP_HITBOX_RADIUS  0x16
#define DISC_BSP_HITBOX_UNK18   0x18
#define DISC_BSP_HITBOX_UNK1A   0x1a
#define DISC_BSP_HITBOX_INSTDEF 0x1c

// struct mesh_info (on-disc size 0x20)
#define DISC_MESH_NUMQUADBLOCK 0x00
#define DISC_MESH_NUMVERTEX    0x04
#define DISC_MESH_UNK1         0x08
#define DISC_MESH_QBARRAY      0x0c
#define DISC_MESH_VERTARRAY    0x10
#define DISC_MESH_UNK2         0x14
#define DISC_MESH_BSPROOT      0x18
#define DISC_MESH_NUMBSPNODES  0x1c

// struct InstDef (on-disc size 0x40)
#define DISC_INSTDEF_SIZE        0x40
#define DISC_INSTDEF_NAME        0x00
#define DISC_INSTDEF_MODEL       0x10
#define DISC_INSTDEF_SCALE       0x14
#define DISC_INSTDEF_COLORRGBA   0x1c
#define DISC_INSTDEF_FLAGS       0x20
#define DISC_INSTDEF_UNK24       0x24
#define DISC_INSTDEF_UNK28       0x28
#define DISC_INSTDEF_PTRINSTANCE 0x2c
#define DISC_INSTDEF_POS         0x30
#define DISC_INSTDEF_ROT         0x36
#define DISC_INSTDEF_MODELID     0x3c

// struct Skybox (on-disc size 0x38)
#define DISC_SKY_NUMVERTEX 0x00
#define DISC_SKY_PTRVERTEX 0x04
#define DISC_SKY_NUMFACES  0x08
#define DISC_SKY_PTRFACES  0x18

// struct WaterVert (on-disc size 0x8)
#define DISC_WV_SIZE 0x8
#define DISC_WV_V    0x0
#define DISC_WV_W    0x4

// struct SpawnType1 header (on-disc size 0x4, then trailing void* array)
#define DISC_ST1_SIZE  0x4
#define DISC_ST1_COUNT 0x0

// struct SpawnType2 (on-disc size 0x8)
#define DISC_ST2_SIZE      0x8
#define DISC_ST2_NUMCOORDS 0x0
#define DISC_ST2_PTR       0x4

// struct NavHeader (on-disc size 0x4c), trailing NavFrame[numPoints]
#define DISC_NAV_SIZE        0x4c
#define DISC_NAV_MAGICNUMBER 0x00
#define DISC_NAV_NUMPOINTS   0x02
#define DISC_NAV_POSY        0x04
#define DISC_NAV_LAST        0x08
#define DISC_NAV_RAMPPHYS1   0x0c
#define DISC_NAV_RAMPPHYS2   0x2c

// struct VisMem (on-disc size 0x90, 4 players x 9 pointer fields)
#define DISC_VISMEM_VISLEAFLIST   0x00
#define DISC_VISMEM_VISFACELIST   0x10
#define DISC_VISMEM_VISOVERTLIST  0x20
#define DISC_VISMEM_VISSCVERTLIST 0x30
#define DISC_VISMEM_VISLEAFSRC    0x40
#define DISC_VISMEM_VISFACESRC    0x50
#define DISC_VISMEM_VISOVERTSRC   0x60
#define DISC_VISMEM_VISSCVERTSRC  0x70
#define DISC_VISMEM_BSPLIST       0x80

// struct Level (on-disc size 0x1f4, footer not consumed by game code)
#define DISC_LEV_MESHINFO         0x00
#define DISC_LEV_SKYBOX           0x04
#define DISC_LEV_ANIMTEX          0x08
#define DISC_LEV_NUMINSTANCES     0x0c
#define DISC_LEV_INSTDEFS         0x10
#define DISC_LEV_NUMMODELS        0x14
#define DISC_LEV_MODELSPTRARRAY   0x18
#define DISC_LEV_UNK3             0x1c
#define DISC_LEV_UNK4             0x20
#define DISC_LEV_INSTDEFPTRARRAY  0x24
#define DISC_LEV_UNK5             0x28
#define DISC_LEV_NULL1            0x2c
#define DISC_LEV_NULL2            0x30
#define DISC_LEV_NUMWATERVERTICES 0x34
#define DISC_LEV_PTRWATER         0x38
#define DISC_LEV_LEVTEXLOOKUP     0x3c
#define DISC_LEV_PTRNAMEDTEXARR   0x40
#define DISC_LEV_PTRTEXWATERENV   0x44
#define DISC_LEV_GLOWGRADIENT     0x48
#define DISC_LEV_DRIVERSPAWN      0x6c
#define DISC_LEV_UNK_CC           0xcc
#define DISC_LEV_UNK_D0           0xd0
#define DISC_LEV_PTRLOWTEXARRAY   0xd4
#define DISC_LEV_CLEARCOLORRGBA   0xd8
#define DISC_LEV_CONFIGFLAGS      0xdc
#define DISC_LEV_BUILDSTART       0xe0
#define DISC_LEV_BUILDEND         0xe4
#define DISC_LEV_BUILDTYPE        0xe8
#define DISC_LEV_UNK_EC           0xec
#define DISC_LEV_RAINBUFFER       0x104
#define DISC_LEV_PTRSPAWNTYPE1    0x134
#define DISC_LEV_NUMSPAWNTYPE2    0x138
#define DISC_LEV_PTRSPAWNTYPE2    0x13c
#define DISC_LEV_NUMSPAWNTYPE2PR  0x140
#define DISC_LEV_PTRSPAWNTYPE2PR  0x144
#define DISC_LEV_CNTRESTARTPTS    0x148
#define DISC_LEV_PTRRESTARTPTS    0x14c
#define DISC_LEV_UNK_150          0x150
#define DISC_LEV_CLEARCOLOR       0x160
#define DISC_LEV_UNK_16C          0x16c
#define DISC_LEV_UNK_170          0x170
#define DISC_LEV_NUMSCVERT        0x174
#define DISC_LEV_PTRSCVERT        0x178
#define DISC_LEV_STARS            0x17c
#define DISC_LEV_SPLITLINES       0x184
#define DISC_LEV_LEVNAVTABLE      0x188
#define DISC_LEV_UNK_18C          0x18c
#define DISC_LEV_VISMEM           0x190

static struct PVS *Reloc64_PVS(struct Reloc64Ctx *ctx, uint32_t off)
{
	if (off == 0)
		return NULL;
	void *cached = Reloc64_VisitedGet(ctx, off);
	if (cached != NULL)
		return cached;

	char *src = ctx->base + off;
	struct PVS *dst = Reloc64_Alloc((int)sizeof(struct PVS));
	Reloc64_VisitedPut(ctx, off, dst);

	dst->visLeafSrc = Reloc64_Resolve(ctx, *(uint32_t *)(src + DISC_PVS_VISLEAFSRC));   // leaf bitmask
	dst->visFaceSrc = Reloc64_Resolve(ctx, *(uint32_t *)(src + DISC_PVS_VISFACESRC));   // leaf bitmask
	dst->visExtraSrc = Reloc64_Resolve(ctx, *(uint32_t *)(src + DISC_PVS_VISEXTRASRC)); // leaf bitmask

	// visInstSrc is a NULL-terminated array of InstDef* (field is typed
	// struct Instance** in retail but holds InstDef* -- see the explicit
	// `(struct InstDef **)` cast at use sites). Shares identity with the main
	// InstDef array via the visited cache, so this must run after
	// Level.ptrInstDefs is walked.
	uint32_t instSrcOff = *(uint32_t *)(src + DISC_PVS_VISINSTSRC);
	if (instSrcOff != 0)
	{
		const uint32_t *srcArr = (const uint32_t *)(ctx->base + instSrcOff);
		int n = 0;
		while (srcArr[n] != 0)
			n++;
		struct Instance **arr = Reloc64_Alloc((n + 1) * (int)sizeof(void *));
		for (int i = 0; i < n; i++)
			arr[i] = Reloc64_VisitedGet(ctx, srcArr[i]);
		arr[n] = NULL;
		dst->visInstSrc = arr;
	}
	else
	{
		dst->visInstSrc = NULL;
	}

	return dst;
}

static struct QuadBlock *Reloc64_QuadBlockArray(struct Reloc64Ctx *ctx, uint32_t off, int count)
{
	if (off == 0 || count <= 0)
		return NULL;

	struct QuadBlock *arr = Reloc64_Alloc(count * (int)sizeof(struct QuadBlock));

	// Pre-register every entry's offset before filling any fields:
	// BSP.data.leaf.ptrQuadBlockArray points into a sub-range of this same
	// contiguous array and is resolved purely through this cache.
	for (int i = 0; i < count; i++)
		Reloc64_VisitedPut(ctx, off + (uint32_t)(i * DISC_QB_SIZE), &arr[i]);

	for (int i = 0; i < count; i++)
	{
		char *src = ctx->base + off + (uint32_t)(i * DISC_QB_SIZE);
		struct QuadBlock *dst = &arr[i];

		memcpy(dst->index, src + DISC_QB_INDEX, sizeof(dst->index));
		dst->quadFlags = (QuadBlockFlags)(*(u16 *)(src + DISC_QB_QUADFLAGS));
		dst->draw_order_low = *(u32 *)(src + DISC_QB_DRAWORDERLOW);
		dst->draw_order_high = *(u32 *)(src + DISC_QB_DRAWORDERHIGH);

		for (int j = 0; j < 4; j++)
			dst->ptr_texture_mid[j] = Reloc64_TexMidPtr(ctx, *(uint32_t *)(src + DISC_QB_PTRTEXMID + j * 4));

		memcpy(&dst->bbox, src + DISC_QB_BBOX, sizeof(dst->bbox));

		dst->terrain_type = *(u8 *)(src + DISC_QB_TERRAINTYPE);
		dst->weather_intensity = *(char *)(src + DISC_QB_TERRAINTYPE + 1);
		dst->weather_vanishRate = *(char *)(src + DISC_QB_TERRAINTYPE + 2);
		dst->mulNormVecY = *(s8 *)(src + DISC_QB_TERRAINTYPE + 3);

		dst->blockID = *(s16 *)(src + DISC_QB_BLOCKID);
		dst->checkpointIndex = *(u8 *)(src + DISC_QB_CHECKPOINTIDX);
		dst->triNormalVecBitShift = *(char *)(src + DISC_QB_CHECKPOINTIDX + 1);

		dst->ptr_texture_low = Reloc64_TexMidPtr(ctx, *(uint32_t *)(src + DISC_QB_PTRTEXLOW));
		dst->pvs = Reloc64_PVS(ctx, *(uint32_t *)(src + DISC_QB_PVS));

		memcpy(dst->triNormalVecDividend, src + DISC_QB_TRINORMALDIV, sizeof(dst->triNormalVecDividend));
	}

	return arr;
}

// Secondary hitbox-variant BSP array (BSP.data.leaf.bspHitboxArray). Despite
// the header comment ("loops until a 4-byte 0x00000000"), the only retail
// consumer (COLL_FIXED_BSPLEAF_TestInstance, `for (; bspArray->flag != 0;
// bspArray++)`) checks the u16 `flag` field alone -- match that exactly.
static struct BSP *Reloc64_BSPHitboxArray(struct Reloc64Ctx *ctx, uint32_t off)
{
	if (off == 0)
		return NULL;
	void *cached = Reloc64_VisitedGet(ctx, off);
	if (cached != NULL)
		return cached;

	int n = 0;
	while (*(u16 *)(ctx->base + off + (uint32_t)(n * DISC_BSP_SIZE) + DISC_BSP_FLAG) != 0)
		n++;

	struct BSP *arr = Reloc64_Alloc((n + 1) * (int)sizeof(struct BSP));
	Reloc64_VisitedPut(ctx, off, arr);

	for (int i = 0; i < n; i++)
	{
		char *s = ctx->base + off + (uint32_t)(i * DISC_BSP_SIZE);
		struct BSP *dst = &arr[i];

		dst->flag = *(u16 *)(s + DISC_BSP_FLAG);
		dst->id = *(s16 *)(s + DISC_BSP_ID);
		memcpy(&dst->box, s + DISC_BSP_BOX, sizeof(dst->box));

		memcpy(&dst->data.hitbox.center, s + DISC_BSP_HITBOX_CENTER, sizeof(dst->data.hitbox.center));
		dst->data.hitbox.radius = *(s16 *)(s + DISC_BSP_HITBOX_RADIUS);
		dst->data.hitbox.unk18 = *(s16 *)(s + DISC_BSP_HITBOX_UNK18);
		dst->data.hitbox.unk1A = *(s16 *)(s + DISC_BSP_HITBOX_UNK1A);

		uint32_t instDefOff = *(uint32_t *)(s + DISC_BSP_HITBOX_INSTDEF);
		dst->data.hitbox.instDef = (instDefOff != 0) ? Reloc64_VisitedGet(ctx, instDefOff) : NULL;
	}
	memset(&arr[n], 0, sizeof(struct BSP)); // terminator entry, flag == 0

	return arr;
}

// Main BSP pool (mesh_info.bspRoot). Rebuilt positionally (src index i -> dst
// index i) so branch.childID, which encodes indices into this same array
// (not byte pointers, see BSP_CHILD_ID_INDEX_MASK), stays valid unmodified.
static struct BSP *Reloc64_BSPArray(struct Reloc64Ctx *ctx, uint32_t off, int numNodes)
{
	if (off == 0 || numNodes <= 0)
		return NULL;

	struct BSP *arr = Reloc64_Alloc(numNodes * (int)sizeof(struct BSP));
	Reloc64_VisitedPut(ctx, off, arr);

	for (int i = 0; i < numNodes; i++)
	{
		char *s = ctx->base + off + (uint32_t)(i * DISC_BSP_SIZE);
		struct BSP *dst = &arr[i];

		dst->flag = *(u16 *)(s + DISC_BSP_FLAG);
		dst->id = *(s16 *)(s + DISC_BSP_ID);
		memcpy(&dst->box, s + DISC_BSP_BOX, sizeof(dst->box));

		if ((dst->flag & BSP_NODE_FLAG_LEAF) != 0)
		{
			dst->data.leaf.unk1 = *(int *)(s + DISC_BSP_LEAF_UNK1);
			dst->data.leaf.bspHitboxArray = Reloc64_BSPHitboxArray(ctx, *(uint32_t *)(s + DISC_BSP_LEAF_HITBOXARR));
			dst->data.leaf.numQuads = *(int *)(s + DISC_BSP_LEAF_NUMQUADS);

			// Sub-range pointer into the main QuadBlock array, walked (and
			// per-entry cached) by Reloc64_MeshInfo before the BSP tree.
			uint32_t qbOff = *(uint32_t *)(s + DISC_BSP_LEAF_QBARRAY);
			dst->data.leaf.ptrQuadBlockArray = (qbOff != 0) ? Reloc64_VisitedGet(ctx, qbOff) : NULL;
		}
		else
		{
			memcpy(dst->data.branch.axis, s + DISC_BSP_BRANCH_AXIS, sizeof(dst->data.branch.axis));
			memcpy(dst->data.branch.childID, s + DISC_BSP_BRANCH_CHILDID, sizeof(dst->data.branch.childID));
		}
	}

	return arr;
}

static struct mesh_info *Reloc64_MeshInfo(struct Reloc64Ctx *ctx, uint32_t off)
{
	if (off == 0)
		return NULL;

	char *src = ctx->base + off;
	struct mesh_info *dst = Reloc64_Alloc((int)sizeof(struct mesh_info));

	dst->numQuadBlock = *(int *)(src + DISC_MESH_NUMQUADBLOCK);
	dst->numVertex = *(int *)(src + DISC_MESH_NUMVERTEX);
	dst->unk1 = *(int *)(src + DISC_MESH_UNK1);
	dst->unk2 = *(int *)(src + DISC_MESH_UNK2);
	dst->numBspNodes = *(int *)(src + DISC_MESH_NUMBSPNODES);

	// QuadBlocks first (and individually cached by file offset), then the BSP
	// tree, whose leaves reference sub-ranges of this same array purely
	// through the visited cache.
	dst->ptrQuadBlockArray = Reloc64_QuadBlockArray(ctx, *(uint32_t *)(src + DISC_MESH_QBARRAY), dst->numQuadBlock);
	dst->ptrVertexArray = Reloc64_Resolve(ctx, *(uint32_t *)(src + DISC_MESH_VERTARRAY)); // leaf
	dst->bspRoot = Reloc64_BSPArray(ctx, *(uint32_t *)(src + DISC_MESH_BSPROOT), dst->numBspNodes);

	return dst;
}

static struct InstDef *Reloc64_InstDefArray(struct Reloc64Ctx *ctx, uint32_t off, int count)
{
	if (off == 0 || count <= 0)
		return NULL;

	struct InstDef *arr = Reloc64_Alloc(count * (int)sizeof(struct InstDef));

	// Pre-register offsets so BSP hitboxes and PVS.visInstSrc (walked later)
	// resolve to these exact same native InstDef objects.
	for (int i = 0; i < count; i++)
		Reloc64_VisitedPut(ctx, off + (uint32_t)(i * DISC_INSTDEF_SIZE), &arr[i]);

	for (int i = 0; i < count; i++)
	{
		char *src = ctx->base + off + (uint32_t)(i * DISC_INSTDEF_SIZE);
		struct InstDef *dst = &arr[i];

		memcpy(dst->name, src + DISC_INSTDEF_NAME, sizeof(dst->name));
		dst->model = Reloc64_Model(ctx, *(uint32_t *)(src + DISC_INSTDEF_MODEL));
		memcpy(&dst->scale, src + DISC_INSTDEF_SCALE, sizeof(dst->scale));
		dst->colorRGBA = *(u32 *)(src + DISC_INSTDEF_COLORRGBA);
		dst->flags = *(u32 *)(src + DISC_INSTDEF_FLAGS);
		dst->unk24 = *(int *)(src + DISC_INSTDEF_UNK24);
		dst->unk28 = *(int *)(src + DISC_INSTDEF_UNK28);
		dst->ptrInstance = Reloc64_Resolve(ctx, *(uint32_t *)(src + DISC_INSTDEF_PTRINSTANCE)); // runtime-set, leaf
		memcpy(&dst->pos, src + DISC_INSTDEF_POS, sizeof(dst->pos));
		memcpy(&dst->rot, src + DISC_INSTDEF_ROT, sizeof(dst->rot));
		dst->modelID = *(int *)(src + DISC_INSTDEF_MODELID);
	}

	return arr;
}

static struct InstDef **Reloc64_InstDefPtrArray(struct Reloc64Ctx *ctx, uint32_t off, int count)
{
	if (off == 0 || count <= 0)
		return NULL;

	// Shares identity with ptrInstDefs via the visited cache -- must run
	// after Reloc64_InstDefArray.
	//
	// Consumers (LevInstDef_UnPack/RePack) walk this as a NULL-terminated
	// list (`visInstSrc[0] != 0`), not bounded by numInstances -- allocate
	// one extra slot and terminate it explicitly, same as
	// dst->ptrModelsPtrArray below. Without it, the walk reads past the
	// array into whatever heap data follows until it happens to hit a zero
	// (or, as found via lldb, a non-zero "pointer" that then gets
	// dereferenced and crashes).
	struct InstDef **arr = Reloc64_Alloc((count + 1) * (int)sizeof(void *));
	const uint32_t *srcArr = (const uint32_t *)(ctx->base + off);
	for (int i = 0; i < count; i++)
		arr[i] = Reloc64_VisitedGet(ctx, srcArr[i]);
	arr[count] = NULL;

	return arr;
}

static struct Skybox *Reloc64_Skybox(struct Reloc64Ctx *ctx, uint32_t off)
{
	if (off == 0)
		return NULL;

	char *src = ctx->base + off;
	struct Skybox *dst = Reloc64_Alloc((int)sizeof(struct Skybox));

	dst->numVertex = *(int *)(src + DISC_SKY_NUMVERTEX);
	dst->ptrVertex = Reloc64_Resolve(ctx, *(uint32_t *)(src + DISC_SKY_PTRVERTEX)); // leaf

	memcpy(dst->numFaces, src + DISC_SKY_NUMFACES, sizeof(dst->numFaces));
	for (int i = 0; i < NUM_SKYBOX_SEGMENTS; i++)
		dst->ptrFaces[i] = Reloc64_Resolve(ctx, *(uint32_t *)(src + DISC_SKY_PTRFACES + i * 4)); // leaf

	return dst;
}

static struct WaterVert *Reloc64_WaterVertArray(struct Reloc64Ctx *ctx, uint32_t off, int count)
{
	if (off == 0 || count <= 0)
		return NULL;

	struct WaterVert *arr = Reloc64_Alloc(count * (int)sizeof(struct WaterVert));
	for (int i = 0; i < count; i++)
	{
		char *src = ctx->base + off + (uint32_t)(i * DISC_WV_SIZE);
		arr[i].v = Reloc64_Resolve(ctx, *(uint32_t *)(src + DISC_WV_V)); // leaf
		arr[i].w = Reloc64_Resolve(ctx, *(uint32_t *)(src + DISC_WV_W)); // leaf
	}
	return arr;
}

static struct SpawnType1 *Reloc64_SpawnType1(struct Reloc64Ctx *ctx, uint32_t off)
{
	if (off == 0)
		return NULL;

	char *src = ctx->base + off;
	int count = *(int *)(src + DISC_ST1_COUNT);
	if (count < 0)
		count = 0;

	struct SpawnType1 *dst = Reloc64_Alloc((int)sizeof(struct SpawnType1) + count * (int)sizeof(void *));
	dst->count = count;

	void **dstArr = ST1_GETPOINTERS(dst);
	const uint32_t *srcArr = (const uint32_t *)(src + DISC_ST1_SIZE);
	for (int i = 0; i < count; i++)
		dstArr[i] = Reloc64_Resolve(ctx, srcArr[i]); // leaf targets

	return dst;
}

static struct SpawnType2 *Reloc64_SpawnType2(struct Reloc64Ctx *ctx, uint32_t off)
{
	if (off == 0)
		return NULL;

	char *src = ctx->base + off;
	struct SpawnType2 *dst = Reloc64_Alloc((int)sizeof(struct SpawnType2));

	dst->numCoords = *(int *)(src + DISC_ST2_NUMCOORDS);
	dst->positions = Reloc64_Resolve(ctx, *(uint32_t *)(src + DISC_ST2_PTR)); // leaf (s16[]/SVec3[]/SpawnPosRot[])

	return dst;
}

static struct NavHeader *Reloc64_NavHeader(struct Reloc64Ctx *ctx, uint32_t off)
{
	if (off == 0)
		return NULL;

	char *src = ctx->base + off;
	int numPoints = *(s16 *)(src + DISC_NAV_NUMPOINTS);
	if (numPoints < 0)
		numPoints = 0;
	int framesBytes = numPoints * (int)sizeof(struct NavFrame);

	// Native: [widened header][inline NavFrame blob] (NavFrame has no pointer
	// fields, so the trailing data is copied verbatim at the native offset).
	struct NavHeader *dst = Reloc64_Alloc((int)sizeof(struct NavHeader) + framesBytes);

	dst->magicNumber = *(s16 *)(src + DISC_NAV_MAGICNUMBER);
	dst->numPoints = (s16)numPoints;
	dst->posY_firstNode = *(int *)(src + DISC_NAV_POSY);
	dst->last = Reloc64_Resolve(ctx, *(uint32_t *)(src + DISC_NAV_LAST)); // runtime cache, leaf
	memcpy(dst->rampPhys1, src + DISC_NAV_RAMPPHYS1, sizeof(dst->rampPhys1));
	memcpy(dst->rampPhys2, src + DISC_NAV_RAMPPHYS2, sizeof(dst->rampPhys2));
	memcpy((char *)dst + sizeof(struct NavHeader), src + DISC_NAV_SIZE, (size_t)framesBytes);

	return dst;
}

static struct NavHeader **Reloc64_LevNavTable(struct Reloc64Ctx *ctx, uint32_t off)
{
	if (off == 0)
		return NULL;

	// Matches sdata->NavPath_ptrHeader[3] (BOTS_InitNavPath only ever indexes
	// 0..2); the on-disc table has no separate stored count.
	enum
	{
		NAV_TABLE_COUNT = 3
	};

	struct NavHeader **arr = Reloc64_Alloc(NAV_TABLE_COUNT * (int)sizeof(void *));
	const uint32_t *srcArr = (const uint32_t *)(ctx->base + off);
	for (int i = 0; i < NAV_TABLE_COUNT; i++)
		arr[i] = Reloc64_NavHeader(ctx, srcArr[i]);

	return arr;
}

static struct VisMem *Reloc64_VisMem(struct Reloc64Ctx *ctx, uint32_t off, int numBspNodes)
{
	if (off == 0)
		return NULL;

	char *src = ctx->base + off;
	struct VisMem *dst = Reloc64_Alloc((int)sizeof(struct VisMem));

	for (int i = 0; i < 4; i++)
	{
		dst->visLeafList[i] = Reloc64_Resolve(ctx, *(uint32_t *)(src + DISC_VISMEM_VISLEAFLIST + i * 4));
		dst->visFaceList[i] = Reloc64_Resolve(ctx, *(uint32_t *)(src + DISC_VISMEM_VISFACELIST + i * 4));
		dst->visOVertList[i] = Reloc64_Resolve(ctx, *(uint32_t *)(src + DISC_VISMEM_VISOVERTLIST + i * 4));
		dst->visSCVertList[i] = Reloc64_Resolve(ctx, *(uint32_t *)(src + DISC_VISMEM_VISSCVERTLIST + i * 4));
		dst->visLeafSrc[i] = Reloc64_Resolve(ctx, *(uint32_t *)(src + DISC_VISMEM_VISLEAFSRC + i * 4));
		dst->visFaceSrc[i] = Reloc64_Resolve(ctx, *(uint32_t *)(src + DISC_VISMEM_VISFACESRC + i * 4));
		dst->visOVertSrc[i] = Reloc64_Resolve(ctx, *(uint32_t *)(src + DISC_VISMEM_VISOVERTSRC + i * 4));
		dst->visSCVertSrc[i] = Reloc64_Resolve(ctx, *(uint32_t *)(src + DISC_VISMEM_VISSCVERTSRC + i * 4));

		// bspList content is fully overwritten by MainInit_InitVisMemBspListNodes
		// right after load (every entry's next/bsp reset); only the allocation
		// needs to exist at native width, sized for numBspNodes.
		uint32_t bspListOff = *(uint32_t *)(src + DISC_VISMEM_BSPLIST + i * 4);
		dst->bspList[i] = (bspListOff != 0 && numBspNodes > 0)
		                      ? Reloc64_Alloc(numBspNodes * (int)sizeof(struct VisMemBspListNode))
		                      : NULL;
	}

	return dst;
}

// Rebuild a just-loaded level file into a native structure and return the
// pointer to store as sdata->ptrLevelFile. `levelBase` is the loaded file
// body (what retail would relocate in place via LOAD_RunPtrMap);
// `ptrMapOffsets`/`numPtrs` is the embedded DRAM pointer map -- unused here
// since every level array has an explicit stored count or fixed size (unlike
// the MPK's ptrTexLayout), kept for signature symmetry with Reloc64_ModelPack.
void *Reloc64_Level(void *levelBase, const int *ptrMapOffsets, int numPtrs)
{
	(void)ptrMapOffsets;
	(void)numPtrs;

	struct Reloc64Ctx ctx;
	ctx.base = (char *)levelBase;
	ctx.ptrSet = NULL;
	ctx.ptrSetCount = 0;
	ctx.visited = NULL;
	ctx.visitedCount = 0;
	ctx.visitedCap = 0;

	char *src = ctx.base;
	struct Level *dst = Reloc64_Alloc((int)sizeof(struct Level));

	// Order matters: producers run before consumers that share identity
	// through the visited map (InstDefs before InstDefPtrArray/BSP
	// hitboxes/PVS; QuadBlocks before the BSP tree).
	dst->ptr_anim_tex = Reloc64_AnimTex(&ctx, *(uint32_t *)(src + DISC_LEV_ANIMTEX));

	dst->numInstances = *(u32 *)(src + DISC_LEV_NUMINSTANCES);
	dst->ptrInstDefs = Reloc64_InstDefArray(&ctx, *(uint32_t *)(src + DISC_LEV_INSTDEFS), (int)dst->numInstances);

	dst->numModels = *(u32 *)(src + DISC_LEV_NUMMODELS);
	{
		uint32_t modelsOff = *(uint32_t *)(src + DISC_LEV_MODELSPTRARRAY);
		int n = (int)dst->numModels;
		if (modelsOff != 0 && n > 0)
		{
			struct Model **arr = Reloc64_Alloc((n + 1) * (int)sizeof(void *));
			const uint32_t *srcArr = (const uint32_t *)(ctx.base + modelsOff);
			for (int i = 0; i < n; i++)
				arr[i] = Reloc64_Model(&ctx, srcArr[i]);
			arr[n] = NULL; // some consumers (CTR_CycleTex_AllModels) NULL-check too
			dst->ptrModelsPtrArray = arr;
		}
		else
		{
			dst->ptrModelsPtrArray = NULL;
		}
	}

	dst->ptr_mesh_info = Reloc64_MeshInfo(&ctx, *(uint32_t *)(src + DISC_LEV_MESHINFO));
	dst->ptrInstDefPtrArray =
	    Reloc64_InstDefPtrArray(&ctx, *(uint32_t *)(src + DISC_LEV_INSTDEFPTRARRAY), (int)dst->numInstances);
	dst->ptr_skybox = Reloc64_Skybox(&ctx, *(uint32_t *)(src + DISC_LEV_SKYBOX));

	// Unknown "extra bsp region" fields: kept as raw resolved leaves (no
	// internal-pointer rebuild). level->unk5 is confirmed a plain packed
	// visibility bitmask (MainFrame.c memcpy source); unk3/unk4/null1/null2
	// are presumed the same until a crash says otherwise.
	dst->unk3 = Reloc64_Resolve(&ctx, *(uint32_t *)(src + DISC_LEV_UNK3));
	dst->unk4 = Reloc64_Resolve(&ctx, *(uint32_t *)(src + DISC_LEV_UNK4));
	dst->unk5 = Reloc64_Resolve(&ctx, *(uint32_t *)(src + DISC_LEV_UNK5));
	dst->null1 = Reloc64_Resolve(&ctx, *(uint32_t *)(src + DISC_LEV_NULL1));
	dst->null2 = Reloc64_Resolve(&ctx, *(uint32_t *)(src + DISC_LEV_NULL2));

	dst->numWaterVertices = *(int *)(src + DISC_LEV_NUMWATERVERTICES);
	dst->ptr_water = Reloc64_WaterVertArray(&ctx, *(uint32_t *)(src + DISC_LEV_PTRWATER), dst->numWaterVertices);

	dst->levTexLookup = Reloc64_LevTexLookup(&ctx, *(uint32_t *)(src + DISC_LEV_LEVTEXLOOKUP));
	dst->ptr_named_tex_array = Reloc64_Resolve(&ctx, *(uint32_t *)(src + DISC_LEV_PTRNAMEDTEXARR)); // Icon[] leaf
	dst->ptr_tex_waterEnvMap = Reloc64_Resolve(&ctx, *(uint32_t *)(src + DISC_LEV_PTRTEXWATERENV)); // leaf

	memcpy(dst->glowGradient, src + DISC_LEV_GLOWGRADIENT, sizeof(dst->glowGradient));
	memcpy(dst->DriverSpawn, src + DISC_LEV_DRIVERSPAWN, sizeof(dst->DriverSpawn));

	dst->unk_Lev_CC = Reloc64_Resolve(&ctx, *(uint32_t *)(src + DISC_LEV_UNK_CC));
	dst->unk_Lev_D0 = Reloc64_Resolve(&ctx, *(uint32_t *)(src + DISC_LEV_UNK_D0));
	dst->ptrLowTexArray = Reloc64_Resolve(&ctx, *(uint32_t *)(src + DISC_LEV_PTRLOWTEXARRAY));

	dst->clearColorRGBA = *(u32 *)(src + DISC_LEV_CLEARCOLORRGBA);
	dst->configFlags = *(u32 *)(src + DISC_LEV_CONFIGFLAGS);

	dst->build_start = Reloc64_Resolve(&ctx, *(uint32_t *)(src + DISC_LEV_BUILDSTART));
	dst->build_end = Reloc64_Resolve(&ctx, *(uint32_t *)(src + DISC_LEV_BUILDEND));
	dst->build_type = Reloc64_Resolve(&ctx, *(uint32_t *)(src + DISC_LEV_BUILDTYPE));

	memcpy(dst->unk_EC, src + DISC_LEV_UNK_EC, sizeof(dst->unk_EC));
	memcpy(&dst->rainBuffer, src + DISC_LEV_RAINBUFFER, sizeof(dst->rainBuffer));

	dst->ptrSpawnType1 = Reloc64_SpawnType1(&ctx, *(uint32_t *)(src + DISC_LEV_PTRSPAWNTYPE1));

	dst->numSpawnType2 = *(int *)(src + DISC_LEV_NUMSPAWNTYPE2);
	dst->ptrSpawnType2 = Reloc64_SpawnType2(&ctx, *(uint32_t *)(src + DISC_LEV_PTRSPAWNTYPE2));

	dst->numSpawnType2_PosRot = *(int *)(src + DISC_LEV_NUMSPAWNTYPE2PR);
	dst->ptrSpawnType2_PosRot = Reloc64_SpawnType2(&ctx, *(uint32_t *)(src + DISC_LEV_PTRSPAWNTYPE2PR));

	dst->cnt_restart_points = *(int *)(src + DISC_LEV_CNTRESTARTPTS);
	dst->ptr_restart_points = Reloc64_Resolve(&ctx, *(uint32_t *)(src + DISC_LEV_PTRRESTARTPTS)); // leaf

	memcpy(dst->unk_150, src + DISC_LEV_UNK_150, sizeof(dst->unk_150));
	memcpy(dst->clearColor, src + DISC_LEV_CLEARCOLOR, sizeof(dst->clearColor));

	dst->unk_16C = *(int *)(src + DISC_LEV_UNK_16C);
	dst->unk_170 = Reloc64_Resolve(&ctx, *(uint32_t *)(src + DISC_LEV_UNK_170));

	dst->numSCVert = *(int *)(src + DISC_LEV_NUMSCVERT);
	dst->ptrSCVert = Reloc64_Resolve(&ctx, *(uint32_t *)(src + DISC_LEV_PTRSCVERT)); // leaf

	memcpy(&dst->stars, src + DISC_LEV_STARS, sizeof(dst->stars));
	memcpy(dst->splitLines, src + DISC_LEV_SPLITLINES, sizeof(dst->splitLines));

	dst->LevNavTable = Reloc64_LevNavTable(&ctx, *(uint32_t *)(src + DISC_LEV_LEVNAVTABLE));
	dst->unk_18C = *(int *)(src + DISC_LEV_UNK_18C);

	dst->visMem = Reloc64_VisMem(&ctx, *(uint32_t *)(src + DISC_LEV_VISMEM),
	                              dst->ptr_mesh_info != NULL ? dst->ptr_mesh_info->numBspNodes : 0);

	memset(dst->footer, 0, sizeof(dst->footer)); // unused by game code

	free(ctx.visited);

	return dst;
}

#endif // CTR_RELOC64
