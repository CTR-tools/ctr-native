#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80031734-0x80031744.
void LIST_Clear(struct LinkedList *L)
{
	L->first = 0;
	L->last = 0;
	L->count = 0;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80031744-0x80031788.
void LIST_AddFront(struct LinkedList *L, struct Item *I)
{
	struct Item *oldFirst;

	if (I == 0)
	{
		return;
	}

	I->prev = 0;

	oldFirst = L->first;
	I->next = oldFirst;

	if (oldFirst != 0)
	{
		L->first->prev = I;
	}
	else
	{
		L->last = I;
	}

	L->first = I;
	L->count = L->count + 1;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80031788-0x800317cc.
void LIST_AddBack(struct LinkedList *L, struct Item *I)
{
	struct Item *oldLast;

	if (I == 0)
	{
		return;
	}

	I->next = 0;

	oldLast = L->last;
	I->prev = oldLast;

	if (oldLast != 0)
	{
		L->last->next = I;
	}
	else
	{
		L->first = I;
	}

	L->last = I;
	L->count = L->count + 1;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800317cc-0x800317d8.
void *LIST_GetNextItem(struct Item *I)
{
	return I->next;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800317d8-0x800317e4.
void *LIST_GetFirstItem(struct LinkedList *L)
{
	return L->first;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800317e4-0x8003186c.
struct Item *LIST_RemoveMember(struct LinkedList *L, struct Item *I)
{
	if (I == 0)
	{
		return 0;
	}

	if (L->first != 0)
	{
		if (I->prev != 0)
		{
			I->prev->next = I->next;
		}
		else
		{
			L->first = I->next;
		}

		if (I->next != 0)
		{
			I->next->prev = I->prev;
		}
		else
		{
			L->last = I->prev;
		}

		L->count = L->count - 1;
	}

	I->next = 0;
	I->prev = 0;

	return I;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003186c-0x800318ec.
struct Item *LIST_RemoveFront(struct LinkedList *L)
{
	struct Item *I = L->first;

	if (I == 0)
	{
		return 0;
	}

	if (I->prev != 0)
	{
		I->prev->next = I->next;
	}
	else
	{
		L->first = I->next;
	}

	if (I->next != 0)
	{
		I->next->prev = I->prev;
	}
	else
	{
		L->last = I->prev;
	}

	L->count = L->count - 1;
	I->next = 0;
	I->prev = 0;

	return I;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800318ec-0x8003197c.
struct Item *LIST_RemoveBack(struct LinkedList *L)
{
	struct Item *I = L->last;

	if (I == 0)
	{
		return 0;
	}

	if (L->first != 0)
	{
		if (I->prev != 0)
		{
			I->prev->next = I->next;
		}
		else
		{
			L->first = I->next;
		}

		if (I->next != 0)
		{
			I->next->prev = I->prev;
		}
		else
		{
			L->last = I->prev;
		}

		L->count = L->count - 1;
	}

	I->next = 0;
	I->prev = 0;

	return I;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003197c-0x800319e8.
void LIST_Init(struct LinkedList *L, struct Item *item, int itemSize, int numItems)
{
	while (numItems > 0)
	{
		LIST_AddBack(L, item);

		numItems--;
		item = (struct Item *)((int)item + itemSize);
	}
}
