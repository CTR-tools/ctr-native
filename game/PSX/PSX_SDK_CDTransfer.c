#include <psx/libcd.h>

// NOTE(aalhendi): These hardware-facing providers are separate SDK owners.
extern int CD_ReadSectorController(void *destination, int wordCount);
extern int CD_ReadSectorAsyncController(void *destination, int wordCount);

#ifdef CTR_NATIVE
// NOTE(aalhendi): Standalone CPU controls supply host storage; the native game
// keeps its own CD backend and does not select these hardware wrappers.
extern CdlCB CD_cbread;
#define CD_READ_CALLBACK_PTR(page) (&CD_cbread)
#else
CdlCB CD_cbread CTR_PSX_MATCH_SECTION(".data.CD_cbread") = 0;
__asm__(".type CD_cbread,@object\n.size CD_cbread,4\n");
#define CD_READ_CALLBACK_PTR(page) ((CdlCB *)((page) - 21232u))
#endif

int CdGetSector(void *destination, int wordCount)
{
	return !CD_ReadSectorController(destination, wordCount);
}

int CdGetSector2(void *destination, int wordCount)
{
	return !CD_ReadSectorAsyncController(destination, wordCount);
}

// NOTE(aalhendi): Capture the old callback before forming the signed-low store
// address, as in the other SDK exchanges. The constraints emit no instructions.
CdlCB CdReadCallback(CdlCB callback)
{
	register u32 statePage CTR_PSX_REGISTER("$1");
	CdlCB old = CD_cbread;

	CTR_PSX_OBSERVE_VALUE(old);
	statePage = 0x80090000u;
	CTR_PSX_KEEP_VALUE(statePage);
	*CD_READ_CALLBACK_PTR(statePage) = callback;
	return old;
}

#undef CD_READ_CALLBACK_PTR
