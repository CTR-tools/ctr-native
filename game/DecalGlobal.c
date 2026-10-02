#include <common.h>

// Asset names and lookup keys are fixed 16-byte fields, word-aligned on PSX.
static inline u32 DecalGlobal_ReadNameWord(const char *name, s32 wordIndex)
{
	return CTR_ReadU32AlignedLE(&name[wordIndex * (s32)sizeof(u32)]);
}

// NOTE(aalhendi): Keep comparisons in the caller's branch chain. GCC 2.8.1
// materializes an extra Boolean when this expression returns through an inline.
// Arguments are stable name pointers, not expressions with side effects.
#define DECAL_NAME_EQUALS(lhs, rhs)                                                                                                                      \
	((DecalGlobal_ReadNameWord(lhs, 0) == DecalGlobal_ReadNameWord(rhs, 0)) && (DecalGlobal_ReadNameWord(lhs, 1) == DecalGlobal_ReadNameWord(rhs, 1)) && \
	 (DecalGlobal_ReadNameWord(lhs, 2) == DecalGlobal_ReadNameWord(rhs, 2)) && (DecalGlobal_ReadNameWord(lhs, 3) == DecalGlobal_ReadNameWord(rhs, 3)))

void DecalGlobal_EmptyFunc_MainFrame_ResetDB(struct GameTracker *tracker)
{
	(void)tracker;
}


void DecalGlobal_Clear(struct GameTracker *gGT)
{
	memset(&gGT->ptrIcons, 0, sizeof(gGT->ptrIcons));
	memset(&gGT->iconGroup, 0, sizeof(gGT->iconGroup));
}


void DecalGlobal_Store(struct GameTracker *gGT, struct LevTexLookup *LTL)
{
	struct Icon *currIcon;
	struct Icon *endIcon;
	struct IconGroup **currGroup;
	struct IconGroup **endGroup;

	if (LTL == 0)
	{
		return;
	}

	currIcon = LTL->firstIcon;
	endIcon = &LTL->firstIcon[LTL->numIcon];
	for (; currIcon < endIcon; currIcon++)
	{
		// Unsigned comparison rejects negative IDs as well as oversized ones.
		if ((u32)currIcon->global_IconArray_Index < len(gGT->ptrIcons))
		{
			gGT->ptrIcons[currIcon->global_IconArray_Index] = currIcon;
		}
	}

	currGroup = LTL->firstIconGroupPtr;
	endGroup = &LTL->firstIconGroupPtr[LTL->numIconGroup];
	for (; currGroup < endGroup; currGroup++)
	{
		struct IconGroup *group = *currGroup;
		if ((u32)group->groupID < len(gGT->iconGroup))
		{
			gGT->iconGroup[group->groupID] = group;
		}
	}
}


struct IconGroup *DecalGlobal_FindInLEV(struct Level *level, const char *name)
{
	struct LevTexLookup *ltl = level->levTexLookup;
	struct IconGroup **curr;
	struct IconGroup **end;

	if (ltl != NULL)
	{
		curr = ltl->firstIconGroupPtr;
		end = &ltl->firstIconGroupPtr[ltl->numIconGroup];

		for (; curr < end; curr++)
		{
			struct IconGroup *group = *curr;

			if (DECAL_NAME_EQUALS(group->name, name))
			{
				return group;
			}
		}
	}

	return NULL;
}


struct Icon *DecalGlobal_FindInMPK(struct Icon *icons, const char *name)
{
	struct Icon *icon = icons;

	for (; icon->name[0] != '\0'; icon++)
	{
		if (DECAL_NAME_EQUALS(icon->name, name))
		{
			return icon;
		}
	}

	return NULL;
}

#undef DECAL_NAME_EQUALS
