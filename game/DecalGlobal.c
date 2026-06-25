#include <common.h>


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80022b94-0x80022b9c.
void DecalGlobal_EmptyFunc_MainFrame_ResetDB(void)
{
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80022b9c-0x80022bdc.
void DecalGlobal_Clear(struct GameTracker *gGT)
{
	memset(&gGT->ptrIcons, 0, sizeof(gGT->ptrIcons));
	memset(&gGT->iconGroup, 0, sizeof(gGT->iconGroup));
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80022bdc-0x80022c88.
void DecalGlobal_Store(struct GameTracker *gGT, struct LevTexLookup *LTL)
{
	struct Icon *currIcon;
	struct IconGroup **currGroup;

	if (LTL == 0)
	{
		return;
	}

	for (
	    // array of Icon
	    currIcon = &LTL->firstIcon[0]; currIcon < &LTL->firstIcon[LTL->numIcon]; currIcon++)
	{
		// uint, in case of negatives
		if ((u32)currIcon->global_IconArray_Index < 0x88)
		{
			gGT->ptrIcons[currIcon->global_IconArray_Index] = currIcon;
		}
	}

	for (
	    // array of POINTER to iconGroup
	    currGroup = &LTL->firstIconGroupPtr[0]; currGroup < &LTL->firstIconGroupPtr[LTL->numIconGroup]; currGroup++)
	{
		// use '[0]' to dereference pointer
		if ((u32)currGroup[0]->groupID < 0x11)
		{
			gGT->iconGroup[currGroup[0]->groupID] = currGroup[0];
		}
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80022c88-0x80022d2c.
int *DecalGlobal_FindInLEV(struct Level *level, char *str)
{
	struct LevTexLookup *ltl = level->levTexLookup;

	if (ltl == NULL)
	{
		return NULL;
	}

	struct IconGroup **curr = ltl->firstIconGroupPtr;
	struct IconGroup **end = &ltl->firstIconGroupPtr[ltl->numIconGroup];

	for (; curr < end; curr++)
	{
		struct IconGroup *group = *curr;

		if (*(int *)&group->name[0x0] == *(int *)&str[0x0] && *(int *)&group->name[0x4] == *(int *)&str[0x4] &&
		    *(int *)&group->name[0x8] == *(int *)&str[0x8] && *(int *)&group->name[0xc] == *(int *)&str[0xc])
		{
			return (int *)group;
		}
	}

	return NULL;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80022d2c-0x80022db0.
int *DecalGlobal_FindInMPK(u32 *icons, char *str)
{
	struct Icon *icon = (struct Icon *)icons;

	for (; icon->name[0] != '\0'; icon++)
	{
		if (*(int *)&icon->name[0x0] == *(int *)&str[0x0] && *(int *)&icon->name[0x4] == *(int *)&str[0x4] && *(int *)&icon->name[0x8] == *(int *)&str[0x8] &&
		    *(int *)&icon->name[0xc] == *(int *)&str[0xc])
		{
			return (int *)icon;
		}
	}

	return NULL;
}
