#include <ctr_types.h>

#ifndef CTR_NATIVE
struct BSP;
struct BoundingBox;
struct ScratchpadStruct;

// NOTE(aalhendi): True incoming arguments must precede common.h's inline bodies.
register struct BSP *const collBspRootOwner CTR_PSX_REGISTER("$4");
register struct BoundingBox *const collBspBoundsOwner CTR_PSX_REGISTER("$5");
register void (*const collBspCallbackOwner)(struct BSP *, struct ScratchpadStruct *) CTR_PSX_REGISTER("$6");
register struct ScratchpadStruct *const collBspQueryOwner CTR_PSX_REGISTER("$7");

#endif

#include <common.h>

// NOTE(aalhendi): Child IDs and boxes use their owned BSP fields, not a second
// packed layout. The cursor starts at scratch+0; pending halfwords begin at +0x70.
#define COLL_PARAM_PUSH_CHILD(childID, childIsNone)                                          \
	do                                                                                       \
	{                                                                                        \
		if (childIsNone)                                                                     \
		{                                                                                    \
			break;                                                                           \
		}                                                                                    \
		node = &root[(childID) & BSP_CHILD_ID_INDEX_MASK];                                   \
		planeWord = CTR_ReadU32AlignedLE(&node->box.min.x);                                  \
		minZMaxX = CTR_ReadU32AlignedLE(&node->box.min.z);                                   \
		if (CTR_MipsSubLo((s16)(planeWord >> 16), maxY) > 0)                                 \
		{                                                                                    \
			break;                                                                           \
		}                                                                                    \
		planeWord = (u32)(s32)(s16)planeWord;                                                \
		if (CTR_MipsSubLo((s32)planeWord, maxX) > 0)                                         \
		{                                                                                    \
			break;                                                                           \
		}                                                                                    \
		planeWord = CTR_ReadU32AlignedLE(&node->box.max.y);                                  \
		if (CTR_MipsSubLo(minX, (s16)(minZMaxX >> 16)) > 0)                                  \
		{                                                                                    \
			break;                                                                           \
		}                                                                                    \
		minZMaxX = (u32)(s32)(s16)minZMaxX;                                                  \
		if (CTR_MipsSubLo((s32)minZMaxX, maxZ) > 0)                                          \
		{                                                                                    \
			break;                                                                           \
		}                                                                                    \
		highZDifference = (s16)(planeWord >> 16);                                            \
		highZDifference = CTR_MipsSubLo(minZ, highZDifference);                              \
		if (highZDifference > 0)                                                             \
		{                                                                                    \
			break;                                                                           \
		}                                                                                    \
		/* NOTE(aalhendi): Final-Y rejection writes this ID without advancing the cursor. */ \
		*(CtrPackedU16 *)(cursor + 0x70) = (childID);                                        \
		planeWord = (u32)(s32)(s16)planeWord;                                                \
		if (CTR_MipsSubLo(minY, (s32)planeWord) > 0)                                         \
		{                                                                                    \
			break;                                                                           \
		}                                                                                    \
		cursor += sizeof(BspChildId);                                                        \
	} while (0)

#ifdef CTR_NATIVE
// NOTE(aalhendi): Native retains its logical values through the host ABI. It
// publishes a live native cursor word, but has no synthetic physical GPR image.
#define COLL_PARAM_RESTORE_CALL_STATE() ((void)0)
#else
// NOTE(aalhendi): Callbacks may clobber AT and the three caller-saved values.
// This is their retail spill adapter, not a second traversal implementation.
#define COLL_PARAM_RESTORE_CALL_STATE()     \
	do                                      \
	{                                       \
		frame = CTR_SCRATCHPAD_PTR(u32, 0); \
		cursor = (u8 *)frame[27];           \
		maxY = (s32)frame[25];              \
		maxZ = (s32)frame[26];              \
	} while (0)
#endif

void CTR_PSX_UNSAFE_MANUAL_AT_FULL_FRAME COLL_SearchBSP_CallbackPARAM(struct BSP *rootArgument, struct BoundingBox *bboxArgument,
                                                                      CollBspLeafCallback callbackArgument, struct ScratchpadStruct *queryArgument)
{
	// NOTE(aalhendi): These bindings guide MIPS allocation, not the calling convention.
	struct BSP *root;
	register struct BoundingBox *bbox CTR_PSX_REGISTER("$5");
	register CollBspLeafCallback callback CTR_PSX_REGISTER("$16");
	register struct ScratchpadStruct *sps CTR_PSX_REGISTER("$19");
	register struct BSP *node CTR_PSX_REGISTER("$18");
	register u32 nodeByteOffset CTR_PSX_REGISTER("$18");
	register s32 minX CTR_PSX_REGISTER("$20");
	register s32 minY CTR_PSX_REGISTER("$21");
	register s32 minZ CTR_PSX_REGISTER("$22");
	s32 maxX;
	register s32 maxY CTR_PSX_REGISTER("$24");
	register s32 maxZ CTR_PSX_REGISTER("$25");
	register u32 children CTR_PSX_REGISTER("$12");
	register u32 minZMaxX CTR_PSX_REGISTER("$8");
	register u32 planeWord CTR_PSX_REGISTER("$3");
	register s32 highChildID CTR_PSX_REGISTER("$13");
	register s32 poppedID CTR_PSX_REGISTER("$11");
	register s32 highZDifference CTR_PSX_REGISTER("$9");
	u32 leafFlag;
	register u8 *cursor CTR_PSX_REGISTER("$2");
#ifdef CTR_NATIVE
	u32 *frame;

	if (rootArgument == NULL)
	{
		return;
	}
	frame = CTR_SCRATCHPAD_PTR(u32, 0);
	root = rootArgument;
	bbox = bboxArgument;
	callback = callbackArgument;
	sps = queryArgument;
#else
	register u32 *frame __asm__("$1");
	register u32 savedS0 __asm__("$16");
	register u32 savedS1 __asm__("$17");
	register u32 savedS2 __asm__("$18");
	register u32 savedS3 __asm__("$19");
	register u32 savedS4 __asm__("$20");
	register u32 savedS5 __asm__("$21");
	register u32 savedS6 __asm__("$22");
	register u32 savedS7 __asm__("$23");
	register u32 savedGp __asm__("$28");
	register u32 savedSp __asm__("$29");
	register u32 savedS8 __asm__("$30");
	register u32 savedRa __asm__("$31");

	// NOTE(aalhendi): These empty captures observe real incoming hardware state.
	__asm__ volatile("" : "=r"(savedS0), "=r"(savedS1), "=r"(savedS2), "=r"(savedS3), "=r"(savedS4), "=r"(savedS5));
	__asm__ volatile("" : "=r"(savedS6), "=r"(savedS7), "=r"(savedGp), "=r"(savedS8), "=r"(savedRa));
	frame = CTR_SCRATCHPAD_PTR(u32, 0);
	frame[12] = savedS0;
	if (collBspRootOwner == NULL)
	{
		return;
	}
	frame[13] = savedS1;
	frame[14] = savedS2;
	frame[15] = savedS3;
	frame[16] = savedS4;
	frame[17] = savedS5;
	frame[18] = savedS6;
	frame[19] = savedS7;
	frame[20] = savedGp;
	frame[21] = (u32)__builtin_sp();
	frame[22] = savedS8;
	frame[23] = savedRa;
	// NOTE(aalhendi): Keep late incoming values live through the final image stores.
	__asm__ volatile("" : : "r"(savedS6), "r"(savedS7), "r"(savedGp), "r"((u32)__builtin_sp()), "r"(savedS8), "r"(savedRa));
	cursor = (u8 *)frame;
	root = collBspRootOwner;
	bbox = collBspBoundsOwner;
#endif
	// NOTE(aalhendi): Snapshot all six bounds before callbacks can mutate the box.
	minX = bbox->min.x;
	minY = bbox->min.y;
	minZ = bbox->min.z;
	maxX = bbox->max.x;
	maxY = bbox->max.y;
	maxZ = bbox->max.z;
#ifdef CTR_NATIVE
	cursor = (u8 *)frame;
#else
	// NOTE(aalhendi): Keep six bound loads ahead of the entry-state copies.
	__asm__ volatile("" : : "r"(minX), "r"(minY), "r"(minZ), "r"(maxX), "r"(maxY), "r"(maxZ));
	callback = collBspCallbackOwner;
	sps = collBspQueryOwner;
#endif
	node = root;
	goto children;

pop:
	// NOTE(aalhendi): Push order makes the LIFO traversal visit child one before child zero.
	poppedID = (s16)CTR_ReadU16AlignedLE(cursor + 0x6e);
	cursor -= sizeof(BspChildId);
	leafFlag = (u32)poppedID & BSP_CHILD_ID_LEAF_FLAG;
	poppedID &= BSP_CHILD_ID_INDEX_MASK;
	// NOTE(aalhendi): The scaled offset dies when the node pointer replaces it in S2.
	nodeByteOffset = (u32)poppedID * sizeof(*root);
	node = (struct BSP *)((u8 *)root + nodeByteOffset);
	if (leafFlag == 0)
	{
		goto children;
	}

	CTR_WriteU32AlignedLE((u8 *)frame + 0x64, (u32)maxY);
	CTR_WriteU32AlignedLE((u8 *)frame + 0x68, (u32)maxZ);
	CTR_WriteU32AlignedLE((u8 *)frame + 0x6c, (u32)cursor);
	callback(node, sps);
	COLL_PARAM_RESTORE_CALL_STATE();
	goto next;

children:
	children = CTR_ReadU32AlignedLE(node->data.branch.childID);
	highChildID = (s32)children >> 16;
	COLL_PARAM_PUSH_CHILD(children, (u16)(children + 1u) == 0);
	COLL_PARAM_PUSH_CHILD(highChildID, ((u32)highChildID + 1u) == 0);

next:
	if (cursor != (u8 *)frame)
	{
#ifndef CTR_NATIVE
		// NOTE(aalhendi): Reestablish the physical scratch owner on this loop edge.
		frame = CTR_SCRATCHPAD_PTR(u32, 0);
#endif
		goto pop;
	}

#ifndef CTR_NATIVE
	frame = CTR_SCRATCHPAD_PTR(u32, 0);
	savedRa = frame[23];
	savedS8 = frame[22];
	savedSp = frame[21];
	savedGp = frame[20];
	savedS7 = frame[19];
	savedS6 = frame[18];
	savedS5 = frame[17];
	savedS4 = frame[16];
	savedS3 = frame[15];
	savedS2 = frame[14];
	savedS1 = frame[13];
	savedS0 = frame[12];
	__asm__ volatile("" : : "r"(savedRa), "r"(savedS8), "r"(savedSp), "r"(savedGp), "r"(savedS7), "r"(savedS6));
	__asm__ volatile("" : : "r"(savedS5), "r"(savedS4), "r"(savedS3), "r"(savedS2), "r"(savedS1), "r"(savedS0));
#endif
}

#undef COLL_PARAM_RESTORE_CALL_STATE
#undef COLL_PARAM_PUSH_CHILD
