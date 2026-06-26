#include "platform/native_checkpoint.h"

#include <macros.h>

#include "../platform.h"
#include "ctr_scratchpad.h"
#include "platform/native_state.h"

#include <string.h>

#define NATIVE_CHECKPOINT_FOURCC(a, b, c, d) ((u32)(a) | ((u32)(b) << 8) | ((u32)(c) << 16) | ((u32)(d) << 24))

// NOTE(aalhendi): Whole-machine checkpoints are included after native memory
// and retail globals are defined, so they can snapshot the same process-local
// regions the game mutates.
#define NATIVE_CHECKPOINT_MAGIC              NATIVE_CHECKPOINT_FOURCC('C', 'T', 'R', 'C')
#define NATIVE_CHECKPOINT_VERSION            2u
#define NATIVE_CHECKPOINT_ADDRESS_RANGE_CAP  16u
#define NATIVE_CHECKPOINT_POINTER_SLOT_CAP   65536u
#define NATIVE_CHECKPOINT_CREDITS_STRING_CAP 4096u
#define NATIVE_CHECKPOINT_LNG_STRING_CAP     4096u

enum NativeCheckpointRegionKind
{
	NATIVE_CHECKPOINT_REGION_RDATA = NATIVE_CHECKPOINT_FOURCC('R', 'D', 'A', 'T'), // resident rdata globals
	NATIVE_CHECKPOINT_REGION_DATA = NATIVE_CHECKPOINT_FOURCC('D', 'A', 'T', 'A'),  // resident data globals
	NATIVE_CHECKPOINT_REGION_SDATA = NATIVE_CHECKPOINT_FOURCC('S', 'D', 'A', 'T'), // resident sdata globals
	NATIVE_CHECKPOINT_REGION_R230 = NATIVE_CHECKPOINT_FOURCC('R', '2', '3', '0'),  // main-menu overlay static data
	NATIVE_CHECKPOINT_REGION_D230 = NATIVE_CHECKPOINT_FOURCC('D', '2', '3', '0'),  // main-menu overlay data
	NATIVE_CHECKPOINT_REGION_V230 = NATIVE_CHECKPOINT_FOURCC('V', '2', '3', '0'),  // main-menu video BSS
	NATIVE_CHECKPOINT_REGION_R231 = NATIVE_CHECKPOINT_FOURCC('R', '2', '3', '1'),  // race/battle overlay static data
	NATIVE_CHECKPOINT_REGION_D231 = NATIVE_CHECKPOINT_FOURCC('D', '2', '3', '1'),  // race/battle overlay data
	NATIVE_CHECKPOINT_REGION_R232 = NATIVE_CHECKPOINT_FOURCC('R', '2', '3', '2'),  // adventure overlay static data
	NATIVE_CHECKPOINT_REGION_D232 = NATIVE_CHECKPOINT_FOURCC('D', '2', '3', '2'),  // adventure overlay data
	NATIVE_CHECKPOINT_REGION_R233 = NATIVE_CHECKPOINT_FOURCC('R', '2', '3', '3'),  // cutscene overlay static data
	NATIVE_CHECKPOINT_REGION_D233 = NATIVE_CHECKPOINT_FOURCC('D', '2', '3', '3'),  // cutscene overlay mutable data
	NATIVE_CHECKPOINT_REGION_GAR3 = NATIVE_CHECKPOINT_FOURCC('G', 'A', 'R', '3'),  // garage runtime state
	NATIVE_CHECKPOINT_REGION_CRD3 = NATIVE_CHECKPOINT_FOURCC('C', 'R', 'D', '3'),  // credits runtime state
	NATIVE_CHECKPOINT_REGION_MPAK = NATIVE_CHECKPOINT_FOURCC('M', 'P', 'A', 'K'),  // mempack backing store
	NATIVE_CHECKPOINT_REGION_SCRP = NATIVE_CHECKPOINT_FOURCC('S', 'C', 'R', 'P'),  // PS1 scratchpad RAM
	NATIVE_CHECKPOINT_REGION_PMAP = NATIVE_CHECKPOINT_FOURCC('P', 'M', 'A', 'P'),  // native pointer-map relocation slots
	NATIVE_CHECKPOINT_REGION_NATS = NATIVE_CHECKPOINT_FOURCC('N', 'A', 'T', 'S'),  // native subsystem state bundle
};

struct NativeCheckpointRegion
{
	u32 kind;
	u32 offset;
	u32 size;
};

struct NativeCheckpointAddressRange
{
	u32 kind;
	u32 start;
	u32 size;
};

struct NativeCheckpointPointerSlotRecord
{
	u32 slotRegion;
	u32 slotOffset;
};

struct NativeCheckpointPointerSlotState
{
	u32 count;
	u32 reserved[3];
	struct NativeCheckpointPointerSlotRecord records[NATIVE_CHECKPOINT_POINTER_SLOT_CAP];
};

enum NativeCheckpointFieldRelocationKind
{
	NATIVE_CHECKPOINT_FIELD_POINTER,
	NATIVE_CHECKPOINT_FIELD_IMAGE_POINTER,
	NATIVE_CHECKPOINT_FIELD_POINTER_OR_IMAGE,
};

struct NativeCheckpointFieldRelocation
{
	u32 offset;
	u32 kind;
};

#define NATIVE_CHECKPOINT_FIELD_PTR(type, field)          {OFFSETOF(type, field), NATIVE_CHECKPOINT_FIELD_POINTER}
#define NATIVE_CHECKPOINT_FIELD_IMAGE(type, field)        {OFFSETOF(type, field), NATIVE_CHECKPOINT_FIELD_IMAGE_POINTER}
#define NATIVE_CHECKPOINT_FIELD_PTR_OR_IMAGE(type, field) {OFFSETOF(type, field), NATIVE_CHECKPOINT_FIELD_POINTER_OR_IMAGE}

struct NativeCheckpointHeader
{
	u32 magic;
	u32 version;
	u32 size;
	u32 regionCount;
	struct PlatformMempackArena mempackArena;
	u32 psxRandSeed;
	s32 activeMempackIndex;
	u32 addressRangeCount;
	u32 codeAnchor;
	struct NativeCheckpointAddressRange addressRanges[NATIVE_CHECKPOINT_ADDRESS_RANGE_CAP];
	struct NativeCheckpointRegion regions[14];
};

global_variable void *s_nativeCheckpointPointerSlots[NATIVE_CHECKPOINT_POINTER_SLOT_CAP];
global_variable u32 s_nativeCheckpointPointerSlotCount;

internal int NativeCheckpoint_InitHeader(struct NativeCheckpointHeader *header);

internal u32 NativeCheckpoint_Align4(u32 value)
{
	return (value + 3u) & ~3u;
}

internal b32 NativeCheckpoint_PtrToU32(const void *ptr, u32 *out)
{
	uintptr_t value = (uintptr_t)ptr;

	if ((ptr == NULL) || (out == NULL) || (value > 0xffffffffu))
	{
		return 0;
	}

	*out = (u32)value;
	return 1;
}

internal b32 NativeCheckpoint_ReadU32Slot(const void *slot, u32 *out)
{
	if ((slot == NULL) || (out == NULL))
	{
		return 0;
	}

	memcpy(out, slot, sizeof(*out));
	return 1;
}

internal void NativeCheckpoint_WriteU32Slot(void *slot, u32 value)
{
	if (slot != NULL)
	{
		memcpy(slot, &value, sizeof(value));
	}
}

void NativeCheckpoint_OnMempackArenaReset(void)
{
	s_nativeCheckpointPointerSlotCount = 0;
}

void NativeCheckpoint_RegisterPointerSlot(void *slot)
{
	if (slot == NULL)
	{
		return;
	}
	if (s_nativeCheckpointPointerSlotCount >= NATIVE_CHECKPOINT_POINTER_SLOT_CAP)
	{
		return;
	}
	for (u32 i = 0; i < s_nativeCheckpointPointerSlotCount; i++)
	{
		if (s_nativeCheckpointPointerSlots[i] == slot)
		{
			return;
		}
	}

	s_nativeCheckpointPointerSlots[s_nativeCheckpointPointerSlotCount++] = slot;
}

internal int NativeCheckpoint_GetActiveMempackIndex(void)
{
	int i;

	for (i = 0; i < 4; i++)
	{
		if (sdata_static.PtrMempack == &sdata_static.mempack[i])
		{
			return i;
		}
	}

	if ((sdata_static.gameTracker.activeMempackIndex >= 0) && (sdata_static.gameTracker.activeMempackIndex < 4))
	{
		return sdata_static.gameTracker.activeMempackIndex;
	}

	return 0;
}

internal int NativeCheckpoint_GetRegionSize(u32 kind)
{
	switch (kind)
	{
	case NATIVE_CHECKPOINT_REGION_RDATA:
		return (int)sizeof(rdata);
	case NATIVE_CHECKPOINT_REGION_DATA:
		return (int)sizeof(data);
	case NATIVE_CHECKPOINT_REGION_SDATA:
		return (int)sizeof(sdata_static);
	case NATIVE_CHECKPOINT_REGION_R230:
		return (int)sizeof(R230);
	case NATIVE_CHECKPOINT_REGION_D230:
		return (int)sizeof(D230);
	case NATIVE_CHECKPOINT_REGION_V230:
		return (int)sizeof(V230);
	case NATIVE_CHECKPOINT_REGION_R231:
		return (int)sizeof(R231);
	case NATIVE_CHECKPOINT_REGION_D231:
		return (int)sizeof(D231);
	case NATIVE_CHECKPOINT_REGION_R232:
		return (int)sizeof(R232);
	case NATIVE_CHECKPOINT_REGION_D232:
		return (int)sizeof(D232);
	case NATIVE_CHECKPOINT_REGION_R233:
		return (int)sizeof(R233);
	case NATIVE_CHECKPOINT_REGION_D233:
		return (int)sizeof(D233);
	case NATIVE_CHECKPOINT_REGION_GAR3:
		return (int)sizeof(gGarage);
	case NATIVE_CHECKPOINT_REGION_CRD3:
		return (int)sizeof(creditsBSS) - OFFSETOF(struct Ovr233_Credits_BSS, CreditThread);
	case NATIVE_CHECKPOINT_REGION_MPAK:
		return (int)sizeof(s_mempackMemory);
	case NATIVE_CHECKPOINT_REGION_SCRP:
		return (int)CTR_SCRATCHPAD_SIZE;
	case NATIVE_CHECKPOINT_REGION_PMAP:
		return (int)sizeof(struct NativeCheckpointPointerSlotState);
	case NATIVE_CHECKPOINT_REGION_NATS:
		return NativeState_GetSize();
	}

	return 0;
}

internal void *NativeCheckpoint_GetRegionPtr(u32 kind)
{
	switch (kind)
	{
	case NATIVE_CHECKPOINT_REGION_RDATA:
		return &rdata;
	case NATIVE_CHECKPOINT_REGION_DATA:
		return &data;
	case NATIVE_CHECKPOINT_REGION_SDATA:
		return &sdata_static;
	case NATIVE_CHECKPOINT_REGION_R230:
		return &R230;
	case NATIVE_CHECKPOINT_REGION_D230:
		return &D230;
	case NATIVE_CHECKPOINT_REGION_V230:
		return &V230;
	case NATIVE_CHECKPOINT_REGION_R231:
		return &R231;
	case NATIVE_CHECKPOINT_REGION_D231:
		return &D231;
	case NATIVE_CHECKPOINT_REGION_R232:
		return &R232;
	case NATIVE_CHECKPOINT_REGION_D232:
		return &D232;
	case NATIVE_CHECKPOINT_REGION_R233:
		return (void *)&R233;
	case NATIVE_CHECKPOINT_REGION_D233:
		return &D233;
	case NATIVE_CHECKPOINT_REGION_GAR3:
		return &gGarage;
	case NATIVE_CHECKPOINT_REGION_CRD3:
		return &creditsBSS.CreditThread;
	case NATIVE_CHECKPOINT_REGION_MPAK:
		return &s_mempackMemory[0];
	case NATIVE_CHECKPOINT_REGION_SCRP:
		return CTR_SCRATCHPAD_BASE;
	}

	return NULL;
}

internal int NativeCheckpoint_CaptureD233(void *dst, int dstSize)
{
	struct OverlayDATA_233 *state = (struct OverlayDATA_233 *)dst;

	if ((dst == NULL) || (dstSize != (int)sizeof(*state)))
	{
		return 0;
	}

	*state = D233;
	memset(state->cs_initMatrixTable, 0, sizeof(state->cs_initMatrixTable));

	return 1;
}

internal int NativeCheckpoint_RestoreD233(const void *src, int srcSize)
{
	const struct OverlayDATA_233 *state = (const struct OverlayDATA_233 *)src;

	if ((src == NULL) || (srcSize != (int)sizeof(*state)))
	{
		return 0;
	}

	D233 = *state;
	OVR233_RebuildInitMatrixTable();

	return 1;
}

internal int NativeCheckpoint_AddAddressRange(struct NativeCheckpointHeader *header, u32 kind)
{
	void *ptr = NativeCheckpoint_GetRegionPtr(kind);
	int size = NativeCheckpoint_GetRegionSize(kind);
	u32 start;
	struct NativeCheckpointAddressRange *range;

	if ((header == NULL) || (ptr == NULL) || (size <= 0))
	{
		return 0;
	}
	if (header->addressRangeCount >= NATIVE_CHECKPOINT_ADDRESS_RANGE_CAP)
	{
		return 0;
	}
	if (!NativeCheckpoint_PtrToU32(ptr, &start))
	{
		return 0;
	}
	if (start + (u32)size < start)
	{
		return 0;
	}

	range = &header->addressRanges[header->addressRangeCount++];
	range->kind = kind;
	range->start = start;
	range->size = (u32)size;
	return 1;
}

internal int NativeCheckpoint_FillAddressRanges(struct NativeCheckpointHeader *header)
{
	local_persist const u32 rangeKinds[] = {
	    NATIVE_CHECKPOINT_REGION_RDATA, NATIVE_CHECKPOINT_REGION_DATA, NATIVE_CHECKPOINT_REGION_SDATA, NATIVE_CHECKPOINT_REGION_R230,
	    NATIVE_CHECKPOINT_REGION_D230,  NATIVE_CHECKPOINT_REGION_V230, NATIVE_CHECKPOINT_REGION_R231,  NATIVE_CHECKPOINT_REGION_D231,
	    NATIVE_CHECKPOINT_REGION_R232,  NATIVE_CHECKPOINT_REGION_D232, NATIVE_CHECKPOINT_REGION_R233,  NATIVE_CHECKPOINT_REGION_D233,
	    NATIVE_CHECKPOINT_REGION_GAR3,  NATIVE_CHECKPOINT_REGION_CRD3, NATIVE_CHECKPOINT_REGION_MPAK,  NATIVE_CHECKPOINT_REGION_SCRP,
	};

	if (header == NULL)
	{
		return 0;
	}

	header->addressRangeCount = 0;
	for (u32 i = 0; i < len(rangeKinds); i++)
	{
		if (!NativeCheckpoint_AddAddressRange(header, rangeKinds[i]))
		{
			return 0;
		}
	}

	return 1;
}

internal const struct NativeCheckpointAddressRange *NativeCheckpoint_FindAddressRange(const struct NativeCheckpointHeader *header, u32 kind)
{
	if (header == NULL)
	{
		return NULL;
	}

	for (u32 i = 0; i < header->addressRangeCount; i++)
	{
		const struct NativeCheckpointAddressRange *range = &header->addressRanges[i];

		if (range->kind == kind)
		{
			return range;
		}
	}

	return NULL;
}

internal b32 NativeCheckpoint_IsAddressRangeValid(const struct NativeCheckpointAddressRange *range)
{
	if ((range == NULL) || (range->size == 0))
	{
		return false;
	}

	return range->start + range->size >= range->start;
}

internal const struct NativeCheckpointAddressRange *NativeCheckpoint_FindAddressOwner(const struct NativeCheckpointHeader *header, u32 address, u32 *offsetOut)
{
	if (header == NULL)
	{
		return NULL;
	}

	for (u32 i = 0; i < header->addressRangeCount; i++)
	{
		const struct NativeCheckpointAddressRange *range = &header->addressRanges[i];
		const u32 end = range->start + range->size;

		if (NativeCheckpoint_IsAddressRangeValid(range) && (address >= range->start) && (address < end))
		{
			if (offsetOut != NULL)
			{
				*offsetOut = address - range->start;
			}
			return range;
		}
	}

	return NULL;
}

internal void *NativeCheckpoint_GetAddressFromRangeOffset(const struct NativeCheckpointHeader *header, u32 kind, u32 offset)
{
	const struct NativeCheckpointAddressRange *range = NativeCheckpoint_FindAddressRange(header, kind);

	if (!NativeCheckpoint_IsAddressRangeValid(range) || (offset >= range->size))
	{
		return NULL;
	}

	return (void *)(uintptr_t)(range->start + offset);
}

internal int NativeCheckpoint_RelocateAddress(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader, u32 oldAddress,
                                              u32 *newAddressOut)
{
	u32 offset;
	const struct NativeCheckpointAddressRange *oldRange;
	const struct NativeCheckpointAddressRange *liveRange;

	if ((oldAddress == 0) || (newAddressOut == NULL))
	{
		return 0;
	}

	oldRange = NativeCheckpoint_FindAddressOwner(oldHeader, oldAddress, &offset);
	if (oldRange == NULL)
	{
		return 0;
	}

	liveRange = NativeCheckpoint_FindAddressRange(liveHeader, oldRange->kind);
	if ((liveRange == NULL) || (offset >= liveRange->size))
	{
		return 0;
	}

	*newAddressOut = liveRange->start + offset;
	return 1;
}

internal int NativeCheckpoint_IsLivePointer(const struct NativeCheckpointHeader *liveHeader, const void *ptr)
{
	u32 address;

	if (!NativeCheckpoint_PtrToU32(ptr, &address))
	{
		return 0;
	}

	return NativeCheckpoint_FindAddressOwner(liveHeader, address, NULL) != NULL;
}

internal void NativeCheckpoint_RelocatePointerSlot(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader, void *slot)
{
	u32 oldAddress;
	u32 newAddress;

	if (!NativeCheckpoint_ReadU32Slot(slot, &oldAddress))
	{
		return;
	}

	if (NativeCheckpoint_RelocateAddress(oldHeader, liveHeader, oldAddress, &newAddress))
	{
		NativeCheckpoint_WriteU32Slot(slot, newAddress);
	}
}

internal void NativeCheckpoint_RelocateImagePointerSlot(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                                        void *slot)
{
	u32 oldAddress;
	u32 newAddress;

	if ((oldHeader == NULL) || (liveHeader == NULL) || (oldHeader->codeAnchor == 0) || (liveHeader->codeAnchor == 0))
	{
		return;
	}
	if (!NativeCheckpoint_ReadU32Slot(slot, &oldAddress) || (oldAddress == 0) || (oldAddress == 0xffffffffu) || (oldAddress == 0xfffffffeu))
	{
		return;
	}

	newAddress = oldAddress + (liveHeader->codeAnchor - oldHeader->codeAnchor);
	NativeCheckpoint_WriteU32Slot(slot, newAddress);
}

internal void NativeCheckpoint_RelocatePointerOrImageSlot(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                                          void *slot)
{
	u32 oldAddress;
	u32 newAddress;

	if (!NativeCheckpoint_ReadU32Slot(slot, &oldAddress))
	{
		return;
	}

	if (NativeCheckpoint_RelocateAddress(oldHeader, liveHeader, oldAddress, &newAddress))
	{
		NativeCheckpoint_WriteU32Slot(slot, newAddress);
	}
	else
	{
		NativeCheckpoint_RelocateImagePointerSlot(oldHeader, liveHeader, slot);
	}
}

internal void NativeCheckpoint_RelocateFields(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader, void *base,
                                              const struct NativeCheckpointFieldRelocation *fields, u32 fieldCount)
{
	u8 *bytes = (u8 *)base;

	if ((base == NULL) || (fields == NULL))
	{
		return;
	}

	for (u32 i = 0; i < fieldCount; i++)
	{
		void *slot = &bytes[fields[i].offset];

		switch (fields[i].kind)
		{
		case NATIVE_CHECKPOINT_FIELD_POINTER:
			NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, slot);
			break;
		case NATIVE_CHECKPOINT_FIELD_IMAGE_POINTER:
			NativeCheckpoint_RelocateImagePointerSlot(oldHeader, liveHeader, slot);
			break;
		case NATIVE_CHECKPOINT_FIELD_POINTER_OR_IMAGE:
			NativeCheckpoint_RelocatePointerOrImageSlot(oldHeader, liveHeader, slot);
			break;
		}
	}
}

internal void NativeCheckpoint_RelocateRectMenu(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                                struct RectMenu *menu)
{
	local_persist const struct NativeCheckpointFieldRelocation fields[] = {
	    NATIVE_CHECKPOINT_FIELD_PTR_OR_IMAGE(struct RectMenu, rows),
	    NATIVE_CHECKPOINT_FIELD_IMAGE(struct RectMenu, funcPtr),
	    NATIVE_CHECKPOINT_FIELD_PTR_OR_IMAGE(struct RectMenu, ptrNextBox_InHierarchy),
	    NATIVE_CHECKPOINT_FIELD_PTR_OR_IMAGE(struct RectMenu, ptrPrevBox_InHierarchy),
	};

	NativeCheckpoint_RelocateFields(oldHeader, liveHeader, menu, fields, len(fields));
}

internal void NativeCheckpoint_RelocateTitle(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                             struct Title *title)
{
	local_persist const struct NativeCheckpointFieldRelocation fields[] = {
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Title, t),
	};

	if (title == NULL)
	{
		return;
	}

	NativeCheckpoint_RelocateFields(oldHeader, liveHeader, title, fields, len(fields));
	for (u32 i = 0; i < len(title->i); i++)
	{
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &title->i[i]);
	}
}

internal void NativeCheckpoint_RelocateAdventurePauseObject(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                                            struct PauseObject *pauseObject)
{
	if (pauseObject == NULL)
	{
		return;
	}

	for (u32 i = 0; i < len(pauseObject->PauseMember); i++)
	{
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &pauseObject->PauseMember[i].inst);
	}
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &pauseObject->t);
}

internal void NativeCheckpoint_RelocateLoadQueueSlot(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                                     struct LoadQueueSlot *slot)
{
	local_persist const struct NativeCheckpointFieldRelocation fields[] = {
	    NATIVE_CHECKPOINT_FIELD_PTR(struct LoadQueueSlot, ptrBigfileCdPos_UNUSED),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct LoadQueueSlot, ptrDestination),
	    NATIVE_CHECKPOINT_FIELD_IMAGE(struct LoadQueueSlot, callbackFuncPtr),
	};

	NativeCheckpoint_RelocateFields(oldHeader, liveHeader, slot, fields, len(fields));
}

internal void NativeCheckpoint_RelocateLinkedList(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                                  struct LinkedList *list)
{
	local_persist const struct NativeCheckpointFieldRelocation fields[] = {
	    NATIVE_CHECKPOINT_FIELD_PTR(struct LinkedList, first),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct LinkedList, last),
	};

	NativeCheckpoint_RelocateFields(oldHeader, liveHeader, list, fields, len(fields));
}

internal void NativeCheckpoint_RelocateItemLinks(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                                 struct Item *item)
{
	local_persist const struct NativeCheckpointFieldRelocation fields[] = {
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Item, next),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Item, prev),
	};

	NativeCheckpoint_RelocateFields(oldHeader, liveHeader, item, fields, len(fields));
}

internal void NativeCheckpoint_RelocateJitPool(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                               struct JitPool *pool)
{
	uintptr_t currSlot;
	u32 itemStep;

	if (pool == NULL)
	{
		return;
	}

	NativeCheckpoint_RelocateLinkedList(oldHeader, liveHeader, &pool->free);
	NativeCheckpoint_RelocateLinkedList(oldHeader, liveHeader, &pool->taken);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &pool->ptrPoolData);

	if ((pool->ptrPoolData == NULL) || (pool->maxItems <= 0) || (pool->itemSize <= 0))
	{
		return;
	}

	itemStep = ((u32)pool->itemSize >> 2) << 2;
	if (itemStep == 0)
	{
		return;
	}

	currSlot = (uintptr_t)pool->ptrPoolData;
	for (int itemIndex = 0; itemIndex < pool->maxItems; itemIndex++)
	{
		NativeCheckpoint_RelocateItemLinks(oldHeader, liveHeader, (struct Item *)currSlot);
		currSlot += itemStep;
	}
}

internal void NativeCheckpoint_RelocatePrimMem(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                               struct PrimMem *primMem)
{
	local_persist const struct NativeCheckpointFieldRelocation fields[] = {
	    NATIVE_CHECKPOINT_FIELD_PTR(struct PrimMem, start),           NATIVE_CHECKPOINT_FIELD_PTR(struct PrimMem, end),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct PrimMem, cursor),          NATIVE_CHECKPOINT_FIELD_PTR(struct PrimMem, guardEnd),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct PrimMem, allocationStart),
	};

	NativeCheckpoint_RelocateFields(oldHeader, liveHeader, primMem, fields, len(fields));
}

internal void NativeCheckpoint_RelocateOTMem(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                             struct OTMem *otMem)
{
	local_persist const struct NativeCheckpointFieldRelocation fields[] = {
	    NATIVE_CHECKPOINT_FIELD_PTR(struct OTMem, start),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct OTMem, end),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct OTMem, cursor),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct OTMem, uiOT),
	};

	NativeCheckpoint_RelocateFields(oldHeader, liveHeader, otMem, fields, len(fields));
}

internal void NativeCheckpoint_RelocateDB(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader, struct DB *db)
{
	if (db == NULL)
	{
		return;
	}

	NativeCheckpoint_RelocatePrimMem(oldHeader, liveHeader, &db->primMem);
	NativeCheckpoint_RelocateOTMem(oldHeader, liveHeader, &db->otMem);
}

internal void NativeCheckpoint_RelocatePushBuffer(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                                  struct PushBuffer *pushBuffer)
{
	local_persist const struct NativeCheckpointFieldRelocation fields[] = {
	    NATIVE_CHECKPOINT_FIELD_PTR(struct PushBuffer, ptrOT),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct PushBuffer, renderBucketOTRangeEnd),
	};

	NativeCheckpoint_RelocateFields(oldHeader, liveHeader, pushBuffer, fields, len(fields));
}

internal void NativeCheckpoint_RelocateCameraDC(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                                struct CameraDC *camera)
{
	local_persist const struct NativeCheckpointFieldRelocation fields[] = {
	    NATIVE_CHECKPOINT_FIELD_PTR(struct CameraDC, visLeafSrc),   NATIVE_CHECKPOINT_FIELD_PTR(struct CameraDC, visFaceSrc),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct CameraDC, visInstSrc),   NATIVE_CHECKPOINT_FIELD_PTR(struct CameraDC, visOVertSrc),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct CameraDC, visSCVertSrc), NATIVE_CHECKPOINT_FIELD_PTR(struct CameraDC, driverToFollow),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct CameraDC, pushBuffer),   NATIVE_CHECKPOINT_FIELD_PTR(struct CameraDC, trackPathNode),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct CameraDC, currEOR),
	};

	NativeCheckpoint_RelocateFields(oldHeader, liveHeader, camera, fields, len(fields));
}

internal void NativeCheckpoint_RelocateInstDrawPerPlayer(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                                         struct InstDrawPerPlayer *idpp)
{
	local_persist const struct NativeCheckpointFieldRelocation fields[] = {
	    NATIVE_CHECKPOINT_FIELD_PTR(struct InstDrawPerPlayer, pushBuffer),    NATIVE_CHECKPOINT_FIELD_PTR(struct InstDrawPerPlayer, ptrCurrFrame),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct InstDrawPerPlayer, ptrNextFrame),  NATIVE_CHECKPOINT_FIELD_PTR(struct InstDrawPerPlayer, ptrCommandList),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct InstDrawPerPlayer, ptrTexLayout),  NATIVE_CHECKPOINT_FIELD_PTR(struct InstDrawPerPlayer, ptrColorLayout),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct InstDrawPerPlayer, ptrDeltaArray), NATIVE_CHECKPOINT_FIELD_PTR(struct InstDrawPerPlayer, mh),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct InstDrawPerPlayer, otRangeNormal), NATIVE_CHECKPOINT_FIELD_PTR(struct InstDrawPerPlayer, otRangeSecondary),
	};

	NativeCheckpoint_RelocateFields(oldHeader, liveHeader, idpp, fields, len(fields));
}

internal void NativeCheckpoint_RelocateInstance(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                                struct Instance *inst, s32 numPlayers)
{
	local_persist const struct NativeCheckpointFieldRelocation fields[] = {
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Instance, next),   NATIVE_CHECKPOINT_FIELD_PTR(struct Instance, prev),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Instance, model),  NATIVE_CHECKPOINT_FIELD_PTR(struct Instance, instDef),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Instance, thread),
	};

	if (inst == NULL)
	{
		return;
	}

	NativeCheckpoint_RelocateFields(oldHeader, liveHeader, inst, fields, len(fields));

	if ((numPlayers < 1) || (numPlayers > 4))
	{
		numPlayers = 4;
	}

	struct InstDrawPerPlayer *idpp = INST_GETIDPP(inst);
	for (s32 playerIndex = 0; playerIndex < numPlayers; playerIndex++)
	{
		NativeCheckpoint_RelocateInstDrawPerPlayer(oldHeader, liveHeader, &idpp[playerIndex]);
	}
}

internal void NativeCheckpoint_RelocateThread(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                              struct Thread *thread)
{
	local_persist const struct NativeCheckpointFieldRelocation fields[] = {
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Thread, next),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Thread, prev),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Thread, name),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Thread, parentThread),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Thread, siblingThread),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Thread, childThread),
	    NATIVE_CHECKPOINT_FIELD_IMAGE(struct Thread, funcThDestroy),
	    NATIVE_CHECKPOINT_FIELD_IMAGE(struct Thread, funcThCollide),
	    NATIVE_CHECKPOINT_FIELD_IMAGE(struct Thread, funcThTick),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Thread, object),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Thread, inst),
	};

	NativeCheckpoint_RelocateFields(oldHeader, liveHeader, thread, fields, len(fields));
}

internal void NativeCheckpoint_RelocateDriver(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                              struct Driver *driver)
{
	local_persist const struct NativeCheckpointFieldRelocation fields[] = {
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Driver, wheelSprites),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Driver, instBombThrow),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Driver, instBubbleHold),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Driver, instTntRecv),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Driver, instSelf),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Driver, instTntSend),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Driver, currBlockTouching),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Driver, underDriver),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Driver, lastValid),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Driver, terrainMeta1),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Driver, terrainMeta2),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Driver, instBigNum),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Driver, instFruitDisp),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Driver, thCloud),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Driver, thTrackingMe),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Driver, plantEatingMe),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Driver, wakeInst),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Driver, pendingDamageAttacker),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Driver, botData.botNavFrame),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Driver, botData.maskObj),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Driver, EndOfRaceComment_ptrQuip),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Driver, ghostTape),
	};

	if (driver == NULL)
	{
		return;
	}

	NativeCheckpoint_RelocateFields(oldHeader, liveHeader, driver, fields, len(fields));

	for (u32 i = 0; i < len(driver->funcPtrs); i++)
	{
		NativeCheckpoint_RelocateImagePointerSlot(oldHeader, liveHeader, &driver->funcPtrs[i]);
	}

	for (u32 i = 0; i < len(driver->driverAudioPtrs); i++)
	{
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &driver->driverAudioPtrs[i]);
	}

	NativeCheckpoint_RelocateItemLinks(oldHeader, liveHeader, &driver->botData.item);

	if (driver->kartState == KS_MASK_GRABBED)
	{
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &driver->KartStates.MaskGrab.maskObj);
	}
	else if (driver->kartState == KS_ENGINE_REVVING)
	{
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &driver->KartStates.RevEngine.maskObj);
	}
}

internal void NativeCheckpoint_RelocateMaskHeadWeapon(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                                      struct MaskHeadWeapon *weapon)
{
	local_persist const struct NativeCheckpointFieldRelocation fields[] = {
	    NATIVE_CHECKPOINT_FIELD_PTR(struct MaskHeadWeapon, maskBeamInst),
	};

	NativeCheckpoint_RelocateFields(oldHeader, liveHeader, weapon, fields, len(fields));
}

internal void NativeCheckpoint_RelocateTrackerWeapon(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                                     struct TrackerWeapon *weapon)
{
	local_persist const struct NativeCheckpointFieldRelocation fields[] = {
	    NATIVE_CHECKPOINT_FIELD_PTR(struct TrackerWeapon, driverTarget), NATIVE_CHECKPOINT_FIELD_PTR(struct TrackerWeapon, driverParent),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct TrackerWeapon, instParent),   NATIVE_CHECKPOINT_FIELD_PTR(struct TrackerWeapon, ptrParticle),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct TrackerWeapon, ptrNodeCurr),  NATIVE_CHECKPOINT_FIELD_PTR(struct TrackerWeapon, ptrNodeNext),
	};

	NativeCheckpoint_RelocateFields(oldHeader, liveHeader, weapon, fields, len(fields));
}

internal void NativeCheckpoint_RelocateRainLocal(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                                 struct RainLocal *rain)
{
	if (rain == NULL)
	{
		return;
	}

	NativeCheckpoint_RelocateItemLinks(oldHeader, liveHeader, (struct Item *)rain);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &rain->cloudInst);
}

internal void NativeCheckpoint_RelocateRainCloud(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                                 struct RainCloud *cloud)
{
	local_persist const struct NativeCheckpointFieldRelocation fields[] = {
	    NATIVE_CHECKPOINT_FIELD_PTR(struct RainCloud, rainLocal),
	};

	NativeCheckpoint_RelocateFields(oldHeader, liveHeader, cloud, fields, len(fields));
}

internal void NativeCheckpoint_RelocateMaskHint(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                                struct MaskHint *maskHint)
{
	local_persist const struct NativeCheckpointFieldRelocation fields[] = {
	    NATIVE_CHECKPOINT_FIELD_PTR(struct MaskHint, self),
	};

	NativeCheckpoint_RelocateFields(oldHeader, liveHeader, maskHint, fields, len(fields));
}

internal void NativeCheckpoint_RelocateShield(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                              struct Shield *shield)
{
	local_persist const struct NativeCheckpointFieldRelocation fields[] = {
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Shield, instColor),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Shield, instHighlight),
	};

	NativeCheckpoint_RelocateFields(oldHeader, liveHeader, shield, fields, len(fields));
}

internal void NativeCheckpoint_RelocateMineWeapon(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                                  struct MineWeapon *weapon)
{
	local_persist const struct NativeCheckpointFieldRelocation fields[] = {
	    NATIVE_CHECKPOINT_FIELD_PTR(struct MineWeapon, driverTarget),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct MineWeapon, instParent),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct MineWeapon, crateInst),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct MineWeapon, weaponSlot231),
	};

	NativeCheckpoint_RelocateFields(oldHeader, liveHeader, weapon, fields, len(fields));
}

internal void NativeCheckpoint_RelocateBaron(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                             struct Baron *baron)
{
	local_persist const struct NativeCheckpointFieldRelocation fields[] = {
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Baron, otherInst),
	};

	NativeCheckpoint_RelocateFields(oldHeader, liveHeader, baron, fields, len(fields));
}

internal void NativeCheckpoint_RelocateFollower(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                                struct Follower *follower)
{
	local_persist const struct NativeCheckpointFieldRelocation fields[] = {
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Follower, driver),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Follower, mineTh),
	};

	NativeCheckpoint_RelocateFields(oldHeader, liveHeader, follower, fields, len(fields));
}

internal void NativeCheckpoint_RelocateFruit(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                             struct Fruit *fruit)
{
	local_persist const struct NativeCheckpointFieldRelocation fields[] = {
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Fruit, driver),
	};

	NativeCheckpoint_RelocateFields(oldHeader, liveHeader, fruit, fields, len(fields));
}

internal void NativeCheckpoint_RelocateSpider(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                              struct Spider *spider)
{
	local_persist const struct NativeCheckpointFieldRelocation fields[] = {
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Spider, shadowInst),
	};

	NativeCheckpoint_RelocateFields(oldHeader, liveHeader, spider, fields, len(fields));
}

internal void NativeCheckpoint_RelocateTurbo(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                             struct Turbo *turbo)
{
	local_persist const struct NativeCheckpointFieldRelocation fields[] = {
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Turbo, inst),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Turbo, driver),
	};

	NativeCheckpoint_RelocateFields(oldHeader, liveHeader, turbo, fields, len(fields));
}

internal void NativeCheckpoint_RelocateBlowupSlots(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader, s32 *slots)
{
	if (slots == NULL)
	{
		return;
	}

	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &slots[0]);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &slots[1]);
}

internal void NativeCheckpoint_RelocateBurstSlots(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader, s32 *slots)
{
	if (slots == NULL)
	{
		return;
	}

	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &slots[0]);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &slots[1]);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &slots[2]);
}

internal void NativeCheckpoint_RelocateBossGarageDoor(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                                      struct BossGarageDoor *door)
{
	local_persist const struct NativeCheckpointFieldRelocation fields[] = {
	    NATIVE_CHECKPOINT_FIELD_PTR(struct BossGarageDoor, garageTopInst),
	};

	NativeCheckpoint_RelocateFields(oldHeader, liveHeader, door, fields, len(fields));
}

internal void NativeCheckpoint_RelocateWoodDoor(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                                struct WoodDoor *door)
{
	if (door == NULL)
	{
		return;
	}

	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &door->otherDoor);
	for (u32 i = 0; i < len(door->keyInst); i++)
	{
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &door->keyInst[i]);
	}
}

internal void NativeCheckpoint_RelocateWarpPad(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                               struct WarpPad *warpPad)
{
	if (warpPad == NULL)
	{
		return;
	}

	for (u32 i = 0; i < len(warpPad->inst); i++)
	{
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &warpPad->inst[i]);
	}
}

internal void NativeCheckpoint_RelocateSaveObj(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                               struct SaveObj *saveObj)
{
	local_persist const struct NativeCheckpointFieldRelocation fields[] = {
	    NATIVE_CHECKPOINT_FIELD_PTR(struct SaveObj, inst),
	};

	NativeCheckpoint_RelocateFields(oldHeader, liveHeader, saveObj, fields, len(fields));
}

internal void NativeCheckpoint_RelocateCutsceneObj(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                                   struct CutsceneObj *cutscene)
{
	if (cutscene == NULL)
	{
		return;
	}

	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &cutscene->ptrIcons);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &cutscene->metadata);
	for (u32 i = 0; i < len(cutscene->currOpcode); i++)
	{
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &cutscene->currOpcode[i]);
	}
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &cutscene->prevOpcode);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &cutscene->frameOverrideRoot);
}

internal void NativeCheckpoint_RelocateSelectProfileLoadSaveObj(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                                                struct SelectProfileLoadSaveObj *obj)
{
	if (obj == NULL)
	{
		return;
	}

	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &obj->thread);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &obj->icons);

	if (NativeCheckpoint_IsLivePointer(liveHeader, obj->icons))
	{
		for (u32 i = 0; i < 12; i++)
		{
			NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &obj->icons[i].inst);
		}
	}
}

internal void NativeCheckpoint_RelocateThreadObject(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                                    struct Thread *thread)
{
	if ((thread == NULL) || (thread->object == NULL))
	{
		return;
	}

	if (thread->funcThTick == GhostReplay_ThTick)
	{
		NativeCheckpoint_RelocateDriver(oldHeader, liveHeader, (struct Driver *)thread->object);
		return;
	}

	if (thread->funcThTick == VehTalkMask_ThTick)
	{
		NativeCheckpoint_RelocateMaskHint(oldHeader, liveHeader, (struct MaskHint *)thread->object);
	}
	else if ((thread->funcThTick == RB_MaskWeapon_ThTick) || (thread->funcThTick == RB_MaskWeapon_FadeAway))
	{
		NativeCheckpoint_RelocateMaskHeadWeapon(oldHeader, liveHeader, (struct MaskHeadWeapon *)thread->object);
	}
	else if ((thread->funcThTick == RB_MovingExplosive_ThTick) || (thread->funcThTick == RB_Warpball_ThTick) || (thread->funcThTick == RB_Warpball_FadeAway) ||
	         (thread->funcThTick == RB_Warpball_TurnAround))
	{
		NativeCheckpoint_RelocateTrackerWeapon(oldHeader, liveHeader, (struct TrackerWeapon *)thread->object);
	}
	else if ((thread->funcThTick == RB_GenericMine_ThTick) || (thread->funcThTick == RB_TNT_ThTick_ThrowOffHead) ||
	         (thread->funcThTick == RB_TNT_ThTick_SitOnHead) || (thread->funcThTick == RB_TNT_ThTick_ThrowOnHead) ||
	         (thread->funcThTick == RB_Potion_ThTick_InAir))
	{
		NativeCheckpoint_RelocateMineWeapon(oldHeader, liveHeader, (struct MineWeapon *)thread->object);
	}
	else if ((thread->funcThTick == RB_RainCloud_ThTick) || (thread->funcThTick == RB_RainCloud_FadeAway))
	{
		NativeCheckpoint_RelocateRainCloud(oldHeader, liveHeader, (struct RainCloud *)thread->object);
	}
	else if ((thread->funcThTick == RB_ShieldDark_ThTick_Grow) || (thread->funcThTick == RB_ShieldDark_ThTick_Pop))
	{
		NativeCheckpoint_RelocateShield(oldHeader, liveHeader, (struct Shield *)thread->object);
	}
	else if (thread->funcThTick == RB_Baron_ThTick)
	{
		NativeCheckpoint_RelocateBaron(oldHeader, liveHeader, (struct Baron *)thread->object);
	}
	else if (thread->funcThTick == RB_Follower_ThTick)
	{
		NativeCheckpoint_RelocateFollower(oldHeader, liveHeader, (struct Follower *)thread->object);
	}
	else if (thread->funcThTick == RB_Fruit_ThTick)
	{
		NativeCheckpoint_RelocateFruit(oldHeader, liveHeader, (struct Fruit *)thread->object);
	}
	else if (thread->funcThTick == RB_Spider_ThTick)
	{
		NativeCheckpoint_RelocateSpider(oldHeader, liveHeader, (struct Spider *)thread->object);
	}
	else if (thread->funcThTick == VehTurbo_ThTick)
	{
		NativeCheckpoint_RelocateTurbo(oldHeader, liveHeader, (struct Turbo *)thread->object);
	}
	else if (thread->funcThTick == RB_Blowup_ThTick)
	{
		NativeCheckpoint_RelocateBlowupSlots(oldHeader, liveHeader, (s32 *)thread->object);
	}
	else if (thread->funcThTick == RB_Burst_ThTick)
	{
		NativeCheckpoint_RelocateBurstSlots(oldHeader, liveHeader, (s32 *)thread->object);
	}
	else if (thread->funcThTick == AH_Garage_ThTick)
	{
		NativeCheckpoint_RelocateBossGarageDoor(oldHeader, liveHeader, (struct BossGarageDoor *)thread->object);
	}
	else if (thread->funcThTick == AH_Door_ThTick)
	{
		NativeCheckpoint_RelocateWoodDoor(oldHeader, liveHeader, (struct WoodDoor *)thread->object);
	}
	else if (thread->funcThTick == AH_WarpPad_ThTick)
	{
		NativeCheckpoint_RelocateWarpPad(oldHeader, liveHeader, (struct WarpPad *)thread->object);
	}
	else if (thread->funcThTick == AH_SaveObj_ThTick)
	{
		NativeCheckpoint_RelocateSaveObj(oldHeader, liveHeader, (struct SaveObj *)thread->object);
	}
	else if (thread->funcThTick == CS_Thread_ThTick)
	{
		NativeCheckpoint_RelocateCutsceneObj(oldHeader, liveHeader, (struct CutsceneObj *)thread->object);
	}
	else if (thread->funcThTick == SelectProfile_ThTick)
	{
		NativeCheckpoint_RelocateSelectProfileLoadSaveObj(oldHeader, liveHeader, (struct SelectProfileLoadSaveObj *)thread->object);
	}
}

internal void NativeCheckpoint_RelocateThreadsInPool(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                                     struct JitPool *pool)
{
	struct Item *item;
	s32 guard = 0;

	if (pool == NULL)
	{
		return;
	}

	item = pool->taken.first;
	while ((item != NULL) && (guard++ < pool->maxItems))
	{
		struct Item *next = item->next;
		NativeCheckpoint_RelocateThread(oldHeader, liveHeader, (struct Thread *)item);
		item = next;
	}
}

internal void NativeCheckpoint_RelocateThreadObjectsInPool(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                                           struct JitPool *pool)
{
	struct Item *item;
	s32 guard = 0;

	if (pool == NULL)
	{
		return;
	}

	item = pool->taken.first;
	while ((item != NULL) && (guard++ < pool->maxItems))
	{
		struct Item *next = item->next;
		NativeCheckpoint_RelocateThreadObject(oldHeader, liveHeader, (struct Thread *)item);
		item = next;
	}
}

internal void NativeCheckpoint_RelocateInstancesInPool(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                                       struct JitPool *pool, s32 numPlayers)
{
	struct Item *item;
	s32 guard = 0;

	if (pool == NULL)
	{
		return;
	}

	item = pool->taken.first;
	while ((item != NULL) && (guard++ < pool->maxItems))
	{
		struct Item *next = item->next;
		NativeCheckpoint_RelocateInstance(oldHeader, liveHeader, (struct Instance *)item, numPlayers);
		item = next;
	}
}

internal void NativeCheckpoint_RelocateRainPool(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                                struct JitPool *pool)
{
	struct Item *item;
	s32 guard = 0;

	if (pool == NULL)
	{
		return;
	}

	item = pool->taken.first;
	while ((item != NULL) && (guard++ < pool->maxItems))
	{
		struct Item *next = item->next;
		NativeCheckpoint_RelocateRainLocal(oldHeader, liveHeader, (struct RainLocal *)item);
		item = next;
	}
}

internal void NativeCheckpoint_RelocateParticle(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                                struct Particle *particle)
{
	local_persist const struct NativeCheckpointFieldRelocation fields[] = {
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Particle, ptrIconArray),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Particle, ptrIconGroup),
	    NATIVE_CHECKPOINT_FIELD_IMAGE(struct Particle, funcPtr),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct Particle, driverInst),
	};

	NativeCheckpoint_RelocateFields(oldHeader, liveHeader, particle, fields, len(fields));
}

internal void NativeCheckpoint_RelocateParticleList(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                                    struct Particle *particle, s32 maxParticles)
{
	s32 guard = 0;

	while ((particle != NULL) && (guard++ < maxParticles))
	{
		struct Particle *next = particle->next;
		NativeCheckpoint_RelocateParticle(oldHeader, liveHeader, particle);
		particle = next;
	}
}

internal void NativeCheckpoint_RelocateRDataPointers(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader)
{
	for (u32 i = 0; i < len(rdata.jumpPointers1); i++)
	{
		NativeCheckpoint_RelocateImagePointerSlot(oldHeader, liveHeader, &rdata.jumpPointers1[i]);
	}
	for (u32 i = 0; i < len(rdata.jumpPointers2); i++)
	{
		NativeCheckpoint_RelocateImagePointerSlot(oldHeader, liveHeader, &rdata.jumpPointers2[i]);
	}
	for (u32 i = 0; i < len(rdata.jumpPointers3); i++)
	{
		NativeCheckpoint_RelocateImagePointerSlot(oldHeader, liveHeader, &rdata.jumpPointers3[i]);
	}
	for (u32 i = 0; i < len(rdata.LOAD_TenStages_jumpPointers4); i++)
	{
		NativeCheckpoint_RelocateImagePointerSlot(oldHeader, liveHeader, &rdata.LOAD_TenStages_jumpPointers4[i]);
	}
	for (u32 i = 0; i < len(rdata.jumpPointers5); i++)
	{
		NativeCheckpoint_RelocateImagePointerSlot(oldHeader, liveHeader, &rdata.jumpPointers5[i]);
	}
	for (u32 i = 0; i < len(rdata.jumpPointers6); i++)
	{
		NativeCheckpoint_RelocateImagePointerSlot(oldHeader, liveHeader, &rdata.jumpPointers6[i]);
	}
}

internal void NativeCheckpoint_RelocateMetaDataModel(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                                     struct MetaDataMODEL *meta)
{
	local_persist const struct NativeCheckpointFieldRelocation fields[] = {
	    NATIVE_CHECKPOINT_FIELD_PTR(struct MetaDataMODEL, name),
	    NATIVE_CHECKPOINT_FIELD_IMAGE(struct MetaDataMODEL, LInB),
	    NATIVE_CHECKPOINT_FIELD_IMAGE(struct MetaDataMODEL, LInC),
	};

	NativeCheckpoint_RelocateFields(oldHeader, liveHeader, meta, fields, len(fields));
}

internal void NativeCheckpoint_RelocateDataPointers(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader)
{
	NativeCheckpoint_RelocateRectMenu(oldHeader, liveHeader, &data.menuRacingWheelConfig);
	NativeCheckpoint_RelocateRectMenu(oldHeader, liveHeader, &data.menuQuit);
	NativeCheckpoint_RelocateRectMenu(oldHeader, liveHeader, &data.menuAdvHub);
	NativeCheckpoint_RelocateRectMenu(oldHeader, liveHeader, &data.menuAdvRace);
	NativeCheckpoint_RelocateRectMenu(oldHeader, liveHeader, &data.menuAdvCup);
	NativeCheckpoint_RelocateRectMenu(oldHeader, liveHeader, &data.menuBattle);
	NativeCheckpoint_RelocateRectMenu(oldHeader, liveHeader, &data.menuArcadeCup);
	NativeCheckpoint_RelocateRectMenu(oldHeader, liveHeader, &data.menuArcadeRace);
	NativeCheckpoint_RelocateRectMenu(oldHeader, liveHeader, &data.menuSaveGame);
	NativeCheckpoint_RelocateRectMenu(oldHeader, liveHeader, &data.menuQueueLoadTrack);
	NativeCheckpoint_RelocateRectMenu(oldHeader, liveHeader, &data.menuGreenLoadSave);
	NativeCheckpoint_RelocateRectMenu(oldHeader, liveHeader, &data.menuFourAdvProfiles);
	NativeCheckpoint_RelocateRectMenu(oldHeader, liveHeader, &data.menuGhostSelection);
	NativeCheckpoint_RelocateRectMenu(oldHeader, liveHeader, &data.menuWarning2);
	NativeCheckpoint_RelocateRectMenu(oldHeader, liveHeader, &data.menuSubmitName);
	NativeCheckpoint_RelocateRectMenu(oldHeader, liveHeader, &data.menuQueueLoadHub);
	NativeCheckpoint_RelocateRectMenu(oldHeader, liveHeader, &data.menuOverwriteAdv);
	NativeCheckpoint_RelocateRectMenu(oldHeader, liveHeader, &data.menuOverwriteGhost);
	NativeCheckpoint_RelocateRectMenu(oldHeader, liveHeader, &data.menuRetryExit);

	for (u32 i = 0; i < len(data.xaLanguagePtrs); i++)
	{
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &data.xaLanguagePtrs[i]);
	}
	for (u32 i = 0; i < len(data.MetaDataModels); i++)
	{
		NativeCheckpoint_RelocateMetaDataModel(oldHeader, liveHeader, &data.MetaDataModels[i]);
	}
	for (u32 i = 0; i < len(data.ptrRenderedQuadblockDestination_forEachPlayer); i++)
	{
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &data.ptrRenderedQuadblockDestination_forEachPlayer[i]);
	}
	for (u32 i = 0; i < len(data.ptrRenderedQuadblockDestination_again); i++)
	{
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &data.ptrRenderedQuadblockDestination_again[i]);
	}
	for (u32 i = 0; i < len(data.ptrColor); i++)
	{
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &data.ptrColor[i]);
	}
	for (u32 i = 0; i < len(data.opcodeFunc); i++)
	{
		NativeCheckpoint_RelocateImagePointerSlot(oldHeader, liveHeader, &data.opcodeFunc[i]);
	}
	for (u32 i = 0; i < len(data.voiceData); i++)
	{
		for (u32 j = 0; j < len(data.voiceData[i].voiceSet); j++)
		{
			NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &data.voiceData[i].voiceSet[j].ptr);
		}
	}
	for (u32 i = 0; i < len(data.voiceSetPtr); i++)
	{
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &data.voiceSetPtr[i]);
	}
	for (u32 i = 0; i < len(data.driverModelExtras); i++)
	{
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &data.driverModelExtras[i].fileBase);
	}

	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &data.podiumModel_firstPlace);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &data.podiumModel_secondPlace);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &data.podiumModel_thirdPlace);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &data.podiumModel_tawna);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &data.podiumModel_unk1);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &data.podiumModel_dingoFire);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &data.podiumModel_unk2);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &data.podiumModel_podiumStands);
	NativeCheckpoint_RelocateLoadQueueSlot(oldHeader, liveHeader, &data.currSlot);

	for (u32 i = 0; i < len(data.overlayCallbackFuncs); i++)
	{
		NativeCheckpoint_RelocateImagePointerSlot(oldHeader, liveHeader, &data.overlayCallbackFuncs[i]);
	}
	for (u32 i = 0; i < len(data.metaDataLEV); i++)
	{
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &data.metaDataLEV[i].name_Debug);
	}
	for (u32 i = 0; i < len(data.PtrClipBuffer); i++)
	{
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &data.PtrClipBuffer[i]);
	}
	for (u32 i = 0; i < len(data.bossWeaponMetaPtr); i++)
	{
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &data.bossWeaponMetaPtr[i]);
	}
	for (u32 i = 0; i < len(data.hudStructPtr); i++)
	{
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &data.hudStructPtr[i]);
	}
}

internal void NativeCheckpoint_RelocateLanguagePointers(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader)
{
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.lngFile);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.lngStrings);

	if ((sdata_static.numLngStrings > 0) && ((u32)sdata_static.numLngStrings <= NATIVE_CHECKPOINT_LNG_STRING_CAP) &&
	    NativeCheckpoint_IsLivePointer(liveHeader, sdata_static.lngStrings))
	{
		for (s32 i = 0; i < sdata_static.numLngStrings; i++)
		{
			NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.lngStrings[i]);
		}
	}
}

internal void NativeCheckpoint_RelocateGhostRecording(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader)
{
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.GhostRecording.ptrGhost);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.GhostRecording.ptrStartOffset);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.GhostRecording.ptrEndOffset);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.GhostRecording.ptrCurrOffset);
}

internal void NativeCheckpoint_RelocateHowlLists(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader)
{
	NativeCheckpoint_RelocateLinkedList(oldHeader, liveHeader, &sdata_static.channelTaken);
	NativeCheckpoint_RelocateLinkedList(oldHeader, liveHeader, &sdata_static.channelFree);
	for (u32 i = 0; i < len(sdata_static.channelStatsPrev); i++)
	{
		NativeCheckpoint_RelocateItemLinks(oldHeader, liveHeader, (struct Item *)&sdata_static.channelStatsPrev[i]);
	}

	NativeCheckpoint_RelocateLinkedList(oldHeader, liveHeader, &sdata_static.Voiceline1);
	NativeCheckpoint_RelocateLinkedList(oldHeader, liveHeader, &sdata_static.Voiceline2);
	for (u32 i = 0; i < len(sdata_static.voicelinePool); i++)
	{
		NativeCheckpoint_RelocateItemLinks(oldHeader, liveHeader, (struct Item *)&sdata_static.voicelinePool[i]);
	}
}

internal void NativeCheckpoint_RelocateSDataPointers(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader)
{
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.arcade_difficultyParams);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.cup_difficultyParams);
	for (u32 i = 0; i < len(sdata_static.PausePtrsVRAM); i++)
	{
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.PausePtrsVRAM[i]);
	}

	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.ptrMPK);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.ptrLevelFile);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.PatchMem_Ptr);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.ptrBigfileCdPos_2);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.modelMaskHints3D);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.gGT);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.gGamepads);
	NativeCheckpoint_RelocateImagePointerSlot(oldHeader, liveHeader, &sdata_static.MainDrawCb_DrawSyncPtr);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.ptrVlcTable);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.memcard_ptrStart);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.PtrMempack);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.bestHumanRank);
	for (u32 i = 0; i < len(sdata_static.difficultyParams); i++)
	{
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.difficultyParams[i]);
	}
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.nav_ptrFirstPoint);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.nav_ptrLastPoint);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.bestRobotRank);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.ptrArray_XaSize);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.ptrArray_NumXAs);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.ptrArray_XaCdPos);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.ptrArray_firstXaIndex);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.ptrArray_numSongs);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.ptrArray_firstSongIndex);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.ptrActiveHighScoreEntry);
	for (u32 i = 0; i < len(sdata_static.ptrGhostTape); i++)
	{
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.ptrGhostTape[i]);
	}
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.ptrGhostTapePlaying);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.ptrLastBank);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.ptrSampleBlock1);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.ptrSampleBlock2);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.ptrCseqHeader);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.ptrCseqSongStartOffset);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.ptrHowlHeader);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.ptrCseqShortSamples);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.ptrCseqSongData);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.howl_metaEngineFX);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.howl_metaOtherFX);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.howl_spuAddrs);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.howl_songOffsets);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.howl_bankOffsets);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.ptrCseqLongSamples);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.howlChainParams[0]);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.howlChainParams[1]);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.ptrHubAlloc);
	NativeCheckpoint_RelocateLanguagePointers(oldHeader, liveHeader);
	NativeCheckpoint_RelocateGhostRecording(oldHeader, liveHeader);
	NativeCheckpoint_RelocateHowlLists(oldHeader, liveHeader);
	NativeCheckpoint_RelocateImagePointerSlot(oldHeader, liveHeader, &sdata_static.callbackCdReadSuccess);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.instMaskHints3D);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.ptrBigfile1);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.PLYROBJECTLIST);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.bossWeaponMeta);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.ptrLoadSaveObj);
	NativeCheckpoint_RelocatePointerOrImageSlot(oldHeader, liveHeader, &sdata_static.ptrActiveMenu);
	NativeCheckpoint_RelocatePointerOrImageSlot(oldHeader, liveHeader, &sdata_static.ptrDesiredMenu);
	NativeCheckpoint_RelocatePointerOrImageSlot(oldHeader, liveHeader, &sdata_static.activeSubMenu);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.ptrPushBufferUI);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.ptrFruitDisp);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.ptrRelic);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.ptrHudCrystal);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.ptrMenuCrystal);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.ptrHudT);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.ptrHudR);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.ptrHudC);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.ptrToken);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.ptrTimebox1);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.blank_NavHeader.last);

	for (u32 i = 0; i < len(sdata_static.NavPath_ptrNavFrameArray); i++)
	{
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.NavPath_ptrNavFrameArray[i]);
	}
	for (u32 i = 0; i < len(sdata_static.NavPath_ptrHeader); i++)
	{
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.NavPath_ptrHeader[i]);
	}
	for (u32 i = 0; i < len(sdata_static.navBotList); i++)
	{
		NativeCheckpoint_RelocateLinkedList(oldHeader, liveHeader, &sdata_static.navBotList[i]);
	}
	for (u32 i = 0; i < len(sdata_static.queueSlots); i++)
	{
		NativeCheckpoint_RelocateLoadQueueSlot(oldHeader, liveHeader, &sdata_static.queueSlots[i]);
	}
	for (u32 i = 0; i < len(sdata_static.quadBlocksRendered); i++)
	{
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.quadBlocksRendered[i]);
	}

	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.ghostProfile_ptrGhostHeader);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.ghostProfile_fileName);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &sdata_static.ghostProfile_fileIconHeader);
	NativeCheckpoint_RelocatePushBuffer(oldHeader, liveHeader, &sdata_static.pushBuffer_DecalMP);
}

internal void NativeCheckpoint_RelocateD230Pointers(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader)
{
	NativeCheckpoint_RelocateRectMenu(oldHeader, liveHeader, &D230.menuMainMenu);
	NativeCheckpoint_RelocateRectMenu(oldHeader, liveHeader, &D230.menuPlayers1P2P);
	NativeCheckpoint_RelocateRectMenu(oldHeader, liveHeader, &D230.menuPlayers2P3P4P);
	NativeCheckpoint_RelocateRectMenu(oldHeader, liveHeader, &D230.menuDifficulty);
	NativeCheckpoint_RelocateRectMenu(oldHeader, liveHeader, &D230.menuRaceType);
	NativeCheckpoint_RelocateRectMenu(oldHeader, liveHeader, &D230.menuAdventure);
	NativeCheckpoint_RelocateRectMenu(oldHeader, liveHeader, &D230.menuCharacterSelect);
	NativeCheckpoint_RelocateRectMenu(oldHeader, liveHeader, &D230.menuTrackSelect);
	NativeCheckpoint_RelocateRectMenu(oldHeader, liveHeader, &D230.menuCupSelect);
	NativeCheckpoint_RelocateRectMenu(oldHeader, liveHeader, &D230.menuBattleWeapons);
	NativeCheckpoint_RelocateRectMenu(oldHeader, liveHeader, &D230.menuHighScores);
	NativeCheckpoint_RelocateRectMenu(oldHeader, liveHeader, &D230.menuScrapbook);
	NativeCheckpoint_RelocateRectMenu(oldHeader, liveHeader, &D230.menuLapSel);
	NativeCheckpoint_RelocateRectMenu(oldHeader, liveHeader, &D230.menuBattleType);
	NativeCheckpoint_RelocateRectMenu(oldHeader, liveHeader, &D230.menuBattleLengthLifeTime);
	NativeCheckpoint_RelocateRectMenu(oldHeader, liveHeader, &D230.menuBattleLengthTimeTime);
	NativeCheckpoint_RelocateRectMenu(oldHeader, liveHeader, &D230.menuBattleLengthPoints);
	NativeCheckpoint_RelocateRectMenu(oldHeader, liveHeader, &D230.menuBattleLengthLifeLife);
	NativeCheckpoint_RelocateRectMenu(oldHeader, liveHeader, &D230.menuBattleStartGame);
	NativeCheckpoint_RelocateRectMenu(oldHeader, liveHeader, &D230.menuHighScore);

	for (u32 i = 0; i < len(D230.arrayMenuPtrs); i++)
	{
		NativeCheckpoint_RelocatePointerOrImageSlot(oldHeader, liveHeader, &D230.arrayMenuPtrs[i]);
	}
	for (u32 i = 0; i < len(D230.battleMenuArray); i++)
	{
		NativeCheckpoint_RelocatePointerOrImageSlot(oldHeader, liveHeader, &D230.battleMenuArray[i]);
	}
	for (u32 i = 0; i < len(D230.cheats); i++)
	{
		NativeCheckpoint_RelocateImagePointerSlot(oldHeader, liveHeader, &D230.cheats[i].funcPtr);
	}
	for (u32 i = 0; i < len(D230.ptrSelectWindowPos); i++)
	{
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &D230.ptrSelectWindowPos[i]);
	}
	for (u32 i = 0; i < len(D230.ptrCsmArr); i++)
	{
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &D230.ptrCsmArr[i]);
	}
	for (u32 i = 0; i < len(D230.ptr_transitionMeta_csm); i++)
	{
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &D230.ptr_transitionMeta_csm[i]);
	}
	for (u32 i = 0; i < len(D230.PlayerNumberStrings); i++)
	{
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &D230.PlayerNumberStrings[i]);
	}

	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &D230.titleObj);
	if (NativeCheckpoint_IsLivePointer(liveHeader, D230.titleObj))
	{
		NativeCheckpoint_RelocateTitle(oldHeader, liveHeader, D230.titleObj);
	}

	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &D230.characterSelect_ptrWindowXY);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &D230.csm_Active);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &D230.ptrIntroCam);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &D230.ptrTransitionMeta);
}

internal void NativeCheckpoint_RelocateV230Pointers(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader)
{
	for (u32 i = 0; i < len(V230.in_Buf); i++)
	{
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &V230.in_Buf[i]);
	}
	for (u32 i = 0; i < len(V230.out_Buf); i++)
	{
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &V230.out_Buf[i]);
	}
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &V230.ptrCdLoc);
}

internal void NativeCheckpoint_RelocateD231Pointers(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader)
{
	for (u32 i = 0; i < len(D231.minePoolItem); i++)
	{
		NativeCheckpoint_RelocateItemLinks(oldHeader, liveHeader, (struct Item *)&D231.minePoolItem[i]);
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &D231.minePoolItem[i].mineWeapon);
	}

	NativeCheckpoint_RelocateLinkedList(oldHeader, liveHeader, &D231.minePoolTaken);
	NativeCheckpoint_RelocateLinkedList(oldHeader, liveHeader, &D231.minePoolFree);
}

internal void NativeCheckpoint_RelocateD232Pointers(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader)
{
	NativeCheckpoint_RelocateRectMenu(oldHeader, liveHeader, &D232.menuTokenRelic);
	NativeCheckpoint_RelocateRectMenu(oldHeader, liveHeader, &D232.menuHintMenu);

	for (u32 i = 0; i < len(D232.hubItemsXY_ptrArray); i++)
	{
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &D232.hubItemsXY_ptrArray[i]);
	}

	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &D232.ptrPauseObject);
	NativeCheckpoint_RelocateAdventurePauseObject(oldHeader, liveHeader, &D232.pauseObject);
	if ((D232.ptrPauseObject != &D232.pauseObject) && NativeCheckpoint_IsLivePointer(liveHeader, D232.ptrPauseObject))
	{
		NativeCheckpoint_RelocateAdventurePauseObject(oldHeader, liveHeader, D232.ptrPauseObject);
	}
}

internal void NativeCheckpoint_RelocateD233Pointers(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader)
{
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &D233.ptrModelBossHead);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &D233.ptrModelBossBody);
}

internal void NativeCheckpoint_RelocateCreditsObjPointers(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                                          struct CreditsObj *creditsObj)
{
	local_persist const struct NativeCheckpointFieldRelocation fields[] = {
	    NATIVE_CHECKPOINT_FIELD_PTR(struct CreditsObj, creditDanceInst),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct CreditsObj, credits_topString),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct CreditsObj, epilogue_topString),
	    NATIVE_CHECKPOINT_FIELD_PTR(struct CreditsObj, epilogue_nextString),
	};

	if (creditsObj == NULL)
	{
		return;
	}

	for (u32 i = 0; i < len(creditsObj->creditGhostModel); i++)
	{
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &creditsObj->creditGhostModel[i]);
	}
	for (u32 i = 0; i < len(creditsObj->creditGhostInst); i++)
	{
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &creditsObj->creditGhostInst[i]);
	}
	NativeCheckpoint_RelocateFields(oldHeader, liveHeader, creditsObj, fields, len(fields));
}

internal void NativeCheckpoint_RelocateCreditsPointers(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader)
{
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &creditsBSS.CreditThread);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &creditsBSS.DancerThread);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &creditsBSS.dancerInst_invisible);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &creditsBSS.ptrStrings);

	if ((creditsBSS.numStrings > 0) && ((u32)creditsBSS.numStrings <= NATIVE_CHECKPOINT_CREDITS_STRING_CAP) &&
	    NativeCheckpoint_IsLivePointer(liveHeader, creditsBSS.ptrStrings))
	{
		for (s32 i = 0; i < creditsBSS.numStrings; i++)
		{
			NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &creditsBSS.ptrStrings[i]);
		}
	}

	NativeCheckpoint_RelocateCreditsObjPointers(oldHeader, liveHeader, &creditsBSS.creditsObj);
}

internal void NativeCheckpoint_RelocateGameTrackerPointers(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader)
{
	struct GameTracker *gGT = &sdata_static.gameTracker;

	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &gGT->backBuffer);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &gGT->frontBuffer);
	for (u32 i = 0; i < len(gGT->db); i++)
	{
		NativeCheckpoint_RelocateDB(oldHeader, liveHeader, &gGT->db[i]);
	}
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &gGT->level1);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &gGT->level2);

	for (u32 i = 0; i < len(gGT->pushBuffer); i++)
	{
		NativeCheckpoint_RelocatePushBuffer(oldHeader, liveHeader, &gGT->pushBuffer[i]);
	}
	for (u32 i = 0; i < len(gGT->DecalMP); i++)
	{
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &gGT->DecalMP[i].inst);
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &gGT->DecalMP[i].ptrOT1);
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &gGT->DecalMP[i].ptrOT2);
	}
	NativeCheckpoint_RelocatePushBuffer(oldHeader, liveHeader, &gGT->pushBuffer_UI);
	for (u32 i = 0; i < len(gGT->cameraDC); i++)
	{
		NativeCheckpoint_RelocateCameraDC(oldHeader, liveHeader, &gGT->cameraDC[i]);
	}

	for (u32 player = 0; player < len(gGT->LevRenderLists); player++)
	{
		for (u32 listIndex = 0; listIndex < len(gGT->LevRenderLists[player].list); listIndex++)
		{
			NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &gGT->LevRenderLists[player].list[listIndex].ptrQuadBlocksRendered);
			NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &gGT->LevRenderLists[player].list[listIndex].bspListStart);
		}
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &gGT->LevRenderLists[player].bspListStart_FullDynamic);
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &gGT->LevRenderLists[player].ptrQuadBlocksRendered_FullDynamic);
	}

	for (u32 i = 0; i < len(gGT->otSwapchainDB); i++)
	{
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &gGT->otSwapchainDB[i]);
	}

	NativeCheckpoint_RelocateJitPool(oldHeader, liveHeader, &gGT->JitPools.thread);
	NativeCheckpoint_RelocateJitPool(oldHeader, liveHeader, &gGT->JitPools.instance);
	NativeCheckpoint_RelocateJitPool(oldHeader, liveHeader, &gGT->JitPools.smallStack);
	NativeCheckpoint_RelocateJitPool(oldHeader, liveHeader, &gGT->JitPools.mediumStack);
	NativeCheckpoint_RelocateJitPool(oldHeader, liveHeader, &gGT->JitPools.largeStack);
	NativeCheckpoint_RelocateJitPool(oldHeader, liveHeader, &gGT->JitPools.particle);
	NativeCheckpoint_RelocateJitPool(oldHeader, liveHeader, &gGT->JitPools.oscillator);
	NativeCheckpoint_RelocateJitPool(oldHeader, liveHeader, &gGT->JitPools.rain);

	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &gGT->visMem1);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &gGT->visMem2);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &gGT->ptrCircle);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &gGT->ptrClod);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &gGT->ptrDustpuff);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &gGT->ptrSmoking);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &gGT->ptrSparkle);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &gGT->mpkIcons);
	for (u32 i = 0; i < len(gGT->iconGroup); i++)
	{
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &gGT->iconGroup[i]);
	}

	for (u32 i = 0; i < len(gGT->threadBuckets); i++)
	{
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &gGT->threadBuckets[i].thread);
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &gGT->threadBuckets[i].s_longName);
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &gGT->threadBuckets[i].s_shortName);
	}

	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &gGT->ptrRenderBucketInstance);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &gGT->particleList_ordinary);
	NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &gGT->particleList_heatWarp);
	NativeCheckpoint_RelocateParticleList(oldHeader, liveHeader, gGT->particleList_ordinary, gGT->JitPools.particle.maxItems);
	NativeCheckpoint_RelocateParticleList(oldHeader, liveHeader, gGT->particleList_heatWarp, gGT->JitPools.particle.maxItems);
	for (u32 i = 0; i < len(gGT->trafficLightIcon); i++)
	{
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &gGT->trafficLightIcon[i]);
	}
	for (u32 i = 0; i < len(gGT->ptrIcons); i++)
	{
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &gGT->ptrIcons[i]);
	}
	for (u32 i = 0; i < len(gGT->modelPtr); i++)
	{
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &gGT->modelPtr[i]);
	}
	for (u32 i = 0; i < len(gGT->drivers); i++)
	{
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &gGT->drivers[i]);
	}
	for (u32 i = 0; i < len(gGT->driversInRaceOrder); i++)
	{
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &gGT->driversInRaceOrder[i]);
	}

	NativeCheckpoint_RelocateThreadsInPool(oldHeader, liveHeader, &gGT->JitPools.thread);
	NativeCheckpoint_RelocateInstancesInPool(oldHeader, liveHeader, &gGT->JitPools.instance, gGT->numPlyrCurrGame);
	NativeCheckpoint_RelocateRainPool(oldHeader, liveHeader, &gGT->JitPools.rain);
	NativeCheckpoint_RelocateThreadObjectsInPool(oldHeader, liveHeader, &gGT->JitPools.thread);
	for (u32 i = 0; i < len(gGT->drivers); i++)
	{
		NativeCheckpoint_RelocateDriver(oldHeader, liveHeader, gGT->drivers[i]);
	}
}

internal void NativeCheckpoint_RelocateRuntimePointers(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader)
{
	NativeCheckpoint_RelocateRDataPointers(oldHeader, liveHeader);
	NativeCheckpoint_RelocateDataPointers(oldHeader, liveHeader);
	NativeCheckpoint_RelocateSDataPointers(oldHeader, liveHeader);
	NativeCheckpoint_RelocateD230Pointers(oldHeader, liveHeader);
	NativeCheckpoint_RelocateV230Pointers(oldHeader, liveHeader);
	NativeCheckpoint_RelocateD231Pointers(oldHeader, liveHeader);
	NativeCheckpoint_RelocateD232Pointers(oldHeader, liveHeader);
	NativeCheckpoint_RelocateD233Pointers(oldHeader, liveHeader);
	NativeCheckpoint_RelocateRectMenu(oldHeader, liveHeader, &gGarage.menuGarage);
	NativeCheckpoint_RelocateCreditsPointers(oldHeader, liveHeader);
	NativeCheckpoint_RelocateGameTrackerPointers(oldHeader, liveHeader);
}

internal int NativeCheckpoint_CapturePointerSlotState(void *dst, int dstSize)
{
	struct NativeCheckpointHeader liveHeader;
	struct NativeCheckpointPointerSlotState *state = (struct NativeCheckpointPointerSlotState *)dst;
	u32 count = 0;

	if ((dst == NULL) || (dstSize != (int)sizeof(*state)))
	{
		return 0;
	}
	if (!NativeCheckpoint_InitHeader(&liveHeader))
	{
		return 0;
	}

	memset(state, 0, sizeof(*state));

	for (u32 i = 0; i < s_nativeCheckpointPointerSlotCount; i++)
	{
		u32 slotAddress;
		u32 slotOffset;
		const struct NativeCheckpointAddressRange *slotRange;

		if (count >= NATIVE_CHECKPOINT_POINTER_SLOT_CAP)
		{
			return 0;
		}
		if (!NativeCheckpoint_PtrToU32(s_nativeCheckpointPointerSlots[i], &slotAddress))
		{
			continue;
		}

		slotRange = NativeCheckpoint_FindAddressOwner(&liveHeader, slotAddress, &slotOffset);
		if (slotRange == NULL)
		{
			continue;
		}

		state->records[count].slotRegion = slotRange->kind;
		state->records[count].slotOffset = slotOffset;
		count++;
	}

	state->count = count;
	return 1;
}

internal int NativeCheckpoint_ApplyPointerSlotState(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader,
                                                    const void *src, int srcSize)
{
	const struct NativeCheckpointPointerSlotState *state = (const struct NativeCheckpointPointerSlotState *)src;

	if ((oldHeader == NULL) || (liveHeader == NULL) || (src == NULL) || (srcSize != (int)sizeof(*state)))
	{
		return 0;
	}
	if (state->count > NATIVE_CHECKPOINT_POINTER_SLOT_CAP)
	{
		return 0;
	}

	s_nativeCheckpointPointerSlotCount = 0;

	for (u32 i = 0; i < state->count; i++)
	{
		const struct NativeCheckpointPointerSlotRecord *record = &state->records[i];
		void *slot = NativeCheckpoint_GetAddressFromRangeOffset(liveHeader, record->slotRegion, record->slotOffset);

		if (slot == NULL)
		{
			return 0;
		}

		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, slot);
		NativeCheckpoint_RegisterPointerSlot(slot);
	}

	return 1;
}

internal void NativeCheckpoint_RelocateMempackPointers(const struct NativeCheckpointHeader *oldHeader, const struct NativeCheckpointHeader *liveHeader)
{
	for (u32 i = 0; i < len(sdata_static.mempack); i++)
	{
		struct Mempack *mempack = &sdata_static.mempack[i];

		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &mempack->start);
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &mempack->lastFreeByte);
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &mempack->endOfAllocator);
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &mempack->endOfMemory);
		NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &mempack->firstFreeByte);

		for (u32 bookmarkIndex = 0; bookmarkIndex < len(mempack->bookmarks); bookmarkIndex++)
		{
			NativeCheckpoint_RelocatePointerSlot(oldHeader, liveHeader, &mempack->bookmarks[bookmarkIndex]);
		}
	}
}

internal int NativeCheckpoint_CaptureRegion(u32 kind, void *dst, int dstSize)
{
	void *src;

	if (kind == NATIVE_CHECKPOINT_REGION_D233)
	{
		return NativeCheckpoint_CaptureD233(dst, dstSize);
	}
	if (kind == NATIVE_CHECKPOINT_REGION_PMAP)
	{
		return NativeCheckpoint_CapturePointerSlotState(dst, dstSize);
	}
	if (kind == NATIVE_CHECKPOINT_REGION_NATS)
	{
		return NativeState_Capture(dst, dstSize);
	}

	src = NativeCheckpoint_GetRegionPtr(kind);
	if (src == NULL)
	{
		return 0;
	}

	memcpy(dst, src, (size_t)dstSize);
	return 1;
}

internal int NativeCheckpoint_RestoreRegion(u32 kind, const void *src, int srcSize)
{
	void *dst;

	if (kind == NATIVE_CHECKPOINT_REGION_D233)
	{
		return NativeCheckpoint_RestoreD233(src, srcSize);
	}

	dst = NativeCheckpoint_GetRegionPtr(kind);
	if (dst == NULL)
	{
		return 0;
	}

	memcpy(dst, src, (size_t)srcSize);
	return 1;
}

internal int NativeCheckpoint_InitHeader(struct NativeCheckpointHeader *header)
{
	u32 offset = NativeCheckpoint_Align4((u32)sizeof(*header));
	u32 i;
	local_persist const u32 regionKinds[] = {
	    NATIVE_CHECKPOINT_REGION_RDATA, NATIVE_CHECKPOINT_REGION_DATA, NATIVE_CHECKPOINT_REGION_SDATA, NATIVE_CHECKPOINT_REGION_D230,
	    NATIVE_CHECKPOINT_REGION_V230,  NATIVE_CHECKPOINT_REGION_D231, NATIVE_CHECKPOINT_REGION_D232,  NATIVE_CHECKPOINT_REGION_D233,
	    NATIVE_CHECKPOINT_REGION_GAR3,  NATIVE_CHECKPOINT_REGION_CRD3, NATIVE_CHECKPOINT_REGION_MPAK,  NATIVE_CHECKPOINT_REGION_SCRP,
	    NATIVE_CHECKPOINT_REGION_PMAP,  NATIVE_CHECKPOINT_REGION_NATS,
	};

	memset(header, 0, sizeof(*header));
	header->magic = NATIVE_CHECKPOINT_MAGIC;
	header->version = NATIVE_CHECKPOINT_VERSION;
	header->regionCount = (u32)len(regionKinds);
	if (!NativeCheckpoint_PtrToU32((const void *)(uintptr_t)&NativeCheckpoint_GetSize, &header->codeAnchor))
	{
		return 0;
	}
	if (!NativeCheckpoint_FillAddressRanges(header))
	{
		return 0;
	}

	if (header->regionCount > len(header->regions))
	{
		return 0;
	}

	for (i = 0; i < header->regionCount; i++)
	{
		const int regionSize = NativeCheckpoint_GetRegionSize(regionKinds[i]);

		if (regionSize <= 0)
		{
			return 0;
		}

		header->regions[i].kind = regionKinds[i];
		header->regions[i].offset = offset;
		header->regions[i].size = (u32)regionSize;
		offset = NativeCheckpoint_Align4(offset + (u32)regionSize);
	}

	header->size = offset;
	return 1;
}

internal int NativeCheckpoint_ValidateHeader(const struct NativeCheckpointHeader *header, int srcSize)
{
	struct NativeCheckpointHeader liveHeader;
	u32 i;

	if ((header == NULL) || (srcSize < (int)sizeof(*header)))
	{
		return 0;
	}
	if ((header->magic != NATIVE_CHECKPOINT_MAGIC) || (header->version != NATIVE_CHECKPOINT_VERSION))
	{
		return 0;
	}
	if ((header->size < sizeof(*header)) || (header->size > (u32)srcSize))
	{
		return 0;
	}
	if (!NativeCheckpoint_InitHeader(&liveHeader))
	{
		return 0;
	}
	if ((header->size != liveHeader.size) || (header->regionCount != liveHeader.regionCount))
	{
		return 0;
	}
	if (header->addressRangeCount != liveHeader.addressRangeCount)
	{
		return 0;
	}

	for (i = 0; i < header->addressRangeCount; i++)
	{
		const struct NativeCheckpointAddressRange *range = &header->addressRanges[i];
		const struct NativeCheckpointAddressRange *liveRange = &liveHeader.addressRanges[i];

		if ((range->kind != liveRange->kind) || (range->size != liveRange->size))
		{
			return 0;
		}
		if (!NativeCheckpoint_IsAddressRangeValid(range) || !NativeCheckpoint_IsAddressRangeValid(liveRange))
		{
			return 0;
		}
	}

	for (i = 0; i < header->regionCount; i++)
	{
		const struct NativeCheckpointRegion *region = &header->regions[i];
		const struct NativeCheckpointRegion *liveRegion = &liveHeader.regions[i];
		const u32 end = region->offset + region->size;

		if ((region->kind != liveRegion->kind) || (region->offset != liveRegion->offset) || (region->size != liveRegion->size))
		{
			return 0;
		}
		if ((region->size == 0) || (region->offset < sizeof(*header)) || (end < region->offset) || (end > header->size))
		{
			return 0;
		}
	}

	return 1;
}

int NativeCheckpoint_GetSize(void)
{
	struct NativeCheckpointHeader header;

	if (!NativeCheckpoint_InitHeader(&header))
	{
		return 0;
	}

	return (int)header.size;
}

int NativeCheckpoint_Capture(void *dst, int dstSize)
{
	struct NativeCheckpointHeader header;
	u8 *bytes = (u8 *)dst;
	u32 i;

	if (!NativeCheckpoint_InitHeader(&header))
	{
		return 0;
	}
	if ((dst == NULL) || (dstSize < (int)header.size))
	{
		return 0;
	}

	header.mempackArena = s_mempackArena;
	header.psxRandSeed = psxRandSeed;
	header.activeMempackIndex = NativeCheckpoint_GetActiveMempackIndex();

	memset(dst, 0, header.size);
	memcpy(dst, &header, sizeof(header));

	for (i = 0; i < header.regionCount; i++)
	{
		struct NativeCheckpointRegion *region = &header.regions[i];

		if (!NativeCheckpoint_CaptureRegion(region->kind, &bytes[region->offset], (int)region->size))
		{
			return 0;
		}
	}

	return 1;
}

int NativeCheckpoint_Restore(const void *src, int srcSize)
{
	const struct NativeCheckpointHeader *header = (const struct NativeCheckpointHeader *)src;
	const u8 *bytes = (const u8 *)src;
	const struct NativeCheckpointRegion *nativeStateRegion = NULL;
	const struct NativeCheckpointRegion *pointerMapRegion = NULL;
	struct NativeCheckpointHeader liveHeader;
	u32 i;

	if (!NativeCheckpoint_ValidateHeader(header, srcSize))
	{
		return 0;
	}
	if (!NativeCheckpoint_InitHeader(&liveHeader))
	{
		return 0;
	}

	// NOTE(aalhendi): 233 checkpoints store only mutable overlay state. Restore
	// the source-owned static image first, then overlay the captured runtime
	// fields below.
	OVR233_ResetRuntimeState();

	for (i = 0; i < header->regionCount; i++)
	{
		const struct NativeCheckpointRegion *region = &header->regions[i];

		if (region->kind == NATIVE_CHECKPOINT_REGION_NATS)
		{
			nativeStateRegion = region;
		}
		else if (region->kind == NATIVE_CHECKPOINT_REGION_PMAP)
		{
			pointerMapRegion = region;
		}
		else
		{
			if (!NativeCheckpoint_RestoreRegion(region->kind, &bytes[region->offset], (int)region->size))
			{
				return 0;
			}
		}
	}

	psxRandSeed = header->psxRandSeed;
	Platform_ConfigureMempackArena();
	NativeCheckpoint_RelocateMempackPointers(header, &liveHeader);
	NativeCheckpoint_RelocateRuntimePointers(header, &liveHeader);
	if (pointerMapRegion == NULL)
	{
		return 0;
	}
	if (!NativeCheckpoint_ApplyPointerSlotState(header, &liveHeader, &bytes[pointerMapRegion->offset], (int)pointerMapRegion->size))
	{
		return 0;
	}
	Platform_RepairResidentPointers(header->activeMempackIndex);

	if (nativeStateRegion == NULL)
	{
		return 0;
	}
	if (!NativeState_Restore(&bytes[nativeStateRegion->offset], (int)nativeStateRegion->size))
	{
		return 0;
	}

	return 1;
}
