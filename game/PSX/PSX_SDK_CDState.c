#include <psx/libcd.h>

#ifdef CTR_NATIVE
// NOTE(aalhendi): Included after native_cd.c's private state owners. Native
// stores through real host pointers, never through the retail address page.
#define CD_STATE_PTR(type, owner, page, offset) (&(owner))
#else
extern s32 CD_debug;
extern CdlCB CD_cbsync;
extern CdlCB CD_cbready;

// NOTE(aalhendi): The SDK state owners share the signed-low address page
// 0x80090000. Keep the store address in $at: GCC otherwise reuses the load's
// address register. The linker checks the owners at these retail offsets.
#define CD_STATE_PTR(type, owner, page, offset) ((type *)((page) - (offset)))
#endif

// NOTE(aalhendi): Observe the old value before forming the store address;
// keep the page opaque so GCC emits one store in the return delay slot.
// These constraints emit no instructions and are no-ops on native.
int CdSetDebug(int level)
{
	register u32 statePage CTR_PSX_REGISTER("$1");
	s32 old = CD_debug;

	CTR_PSX_OBSERVE_VALUE(old);
	statePage = 0x80090000u;
	CTR_PSX_KEEP_VALUE(statePage);
	*CD_STATE_PTR(s32, CD_debug, statePage, 15324u) = level;
	return old;
}

CdlCB CdSyncCallback(CdlCB func)
{
	register u32 statePage CTR_PSX_REGISTER("$1");
	CdlCB old = CD_cbsync;

	CTR_PSX_OBSERVE_VALUE(old);
	statePage = 0x80090000u;
	CTR_PSX_KEEP_VALUE(statePage);
	*CD_STATE_PTR(CdlCB, CD_cbsync, statePage, 15332u) = func;
	return old;
}

CdlCB CdReadyCallback(CdlCB func)
{
	register u32 statePage CTR_PSX_REGISTER("$1");
	CdlCB old = CD_cbready;

	CTR_PSX_OBSERVE_VALUE(old);
	statePage = 0x80090000u;
	CTR_PSX_KEEP_VALUE(statePage);
	*CD_STATE_PTR(CdlCB, CD_cbready, statePage, 15328u) = func;
	return old;
}

#undef CD_STATE_PTR
