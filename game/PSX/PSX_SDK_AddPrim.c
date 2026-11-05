#include <common.h>

void AddPrim(void *ot, void *p)
{
	// NOTE(aalhendi): Shared tag operations preserve lengths, aliasing and native link tokens.
	addPrim(ot, p);
}
