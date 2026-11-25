#include "PSX_SDK_GpuInternal.h"

u32 *ClearOTagR(u32 *ot, int n)
{
	u32 *head;

	if (psxGpuEnv.debug >= 2)
	{
		psxGpuPrintf("ClearOTagR(%08x,%d)...\n", ot, n);
	}
	psxGpuJtb->clearOTagR(ot, n);
	head = &psxGpuOtHead;
	*head = ((u32)&psxGpuOtTail & 0x00ffffffu) | 0x04000000u;
	ot[0] = (u32)head & 0x00ffffffu;
	return ot;
}

void DrawOTag(void *p)
{
	struct PsxGpuJumpTable *jtb;
	// NOTE(aalhendi): Keep $a1 loaded before psxGpuJtb and prevent CSE from
	// copying $a2 into $a3 instead of zeroing $a3 directly.
	register void *otArg CTR_PSX_REGISTER("$5");
	register s32 sizeArg CTR_PSX_REGISTER("$6");

	if (psxGpuEnv.debug >= 2)
	{
		psxGpuPrintf("DrawOTag(%08x)...\n", p);
	}
	otArg = p;
	CTR_PSX_KEEP_VALUE(otArg);
	jtb = psxGpuJtb;
	CTR_PSX_ORDER_VALUES(jtb, otArg);
	sizeArg = 0;
	CTR_PSX_ORDER_VALUES(sizeArg, jtb);
	CTR_PSX_FORGET_VALUE(sizeArg);
	jtb->addque(jtb->otc, otArg, sizeArg, NULL);
}

DRAWENV *PutDrawEnv(DRAWENV *env)
{
	if (psxGpuEnv.debug >= 2)
	{
		psxGpuPrintf("PutDrawEnv(%08x)...\n", env);
	}
	psxGpuBuildDrawEnv(&env->dr_env, env);
	env->dr_env.tag |= 0x00ffffffu;
	psxGpuJtb->addque(psxGpuJtb->otc, &env->dr_env, 64, NULL);
	memcpy(&psxGpuEnv.drawEnv, env, sizeof(DRAWENV));
	return env;
}
