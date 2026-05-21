#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001d094-0x8001d0c4
struct MetaDataMODEL *DECOMP_COLL_LevModelMeta(u32 id)
{
	// use unsigned so -1 is positive
	if (id >= 0xe2)
		id = 0;

	return &data.MetaDataModels[id];
}
