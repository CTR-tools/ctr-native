#include <psx/libgte.h>
#include <psx/libetc.h>
#include <psx/libgpu.h>

DRAWENV *SetDefDrawEnv(DRAWENV *env, s32 x, s32 y, s32 w, s32 h)
{
	s32 mode = GetVideoMode();
	env->clip.x = x;
	env->clip.y = y;
	env->clip.w = w;
	env->tw.x = 0;
	env->tw.y = 0;
	env->tw.w = 0;
	env->tw.h = 0;
	env->r0 = 0;
	env->g0 = 0;
	env->b0 = 0;
	env->dtd = 1;
	env->clip.h = h;
	if (mode)
		env->dfe = h < 289;
	else
		env->dfe = h < 257;
	env->ofs[0] = x;
	env->ofs[1] = y;
	env->tpage = 10;
	env->isbg = 0;
	return env;
}

DISPENV *SetDefDispEnv(DISPENV *env, s32 x, s32 y, s32 w, s32 h)
{
	env->disp.x = x;
	env->disp.y = y;
	env->disp.w = w;
	env->screen.x = 0;
	env->screen.y = 0;
	env->screen.w = 0;
	env->screen.h = 0;
	env->isrgb24 = 0;
	env->isinter = 0;
	env->pad1 = 0;
	env->pad0 = 0;
	env->disp.h = h;
	return env;
}
