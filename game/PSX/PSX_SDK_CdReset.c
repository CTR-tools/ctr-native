#if !defined(CTR_NATIVE)
#include <psx/libcd.h>

int CD_ResetController(int mode);

// NOTE(aalhendi): Separate input so retail's empty call/return delay slots remain.
int CdReset(int mode)
{
	return CD_ResetController(mode);
}
#endif
