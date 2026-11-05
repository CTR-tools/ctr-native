#include <psx/libetc.h>
#include <ctr_compiler.h>

extern s32 PSX_videoMode;

#ifdef CTR_NATIVE
#define VIDEO_MODE_STORE(page) (&PSX_videoMode)
#else
#define VIDEO_MODE_STORE(page) ((s32 *)((page) - 16204u))
#endif

s32 SetVideoMode(s32 mode)
{
	register u32 statePage CTR_PSX_REGISTER("$1");
	s32 old = PSX_videoMode;
	// NOTE(aalhendi): Keep the old value and independent signed-low store page live through the exchange.
	CTR_PSX_OBSERVE_VALUE(old);
	statePage = 0x80090000u;
	CTR_PSX_KEEP_VALUE(statePage);
	*VIDEO_MODE_STORE(statePage) = mode;
	return old;
}

s32 GetVideoMode(void)
{
	return PSX_videoMode;
}

#undef VIDEO_MODE_STORE
