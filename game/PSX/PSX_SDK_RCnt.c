#include <ctr_compiler.h>
#include <macros.h>

struct PsxRCntReg
{
	u16 count;
	u16 pad0;
	volatile u16 mode;
	u16 pad1;
	volatile u16 target;
	u16 pad2;
	u32 pad3;
};

extern u32 *psxIntReg;
extern struct PsxRCntReg *psxRCntRegs;
extern u32 psxRCntIrqMasks[];

s32 SetRCnt(u32 spec, u16 target, s32 mode)
{
	// NOTE(aalhendi): Keep idx in $t0 and the return value in $v0 across the
	// shared exit so GCC emits retail's jump to the single jr $ra epilogue.
	s32 idx CTR_PSX_GLOBAL_REGISTER_HINT("$8") = spec & 0xffffu;
	u32 regMode = 0x48;
	register s32 ok CTR_PSX_REGISTER("$2");

	if (idx >= 3)
	{
		ok = 0;
	}
	else
	{
		psxRCntRegs[idx].mode = 0;
		psxRCntRegs[idx].target = target;
		if ((u32)idx < 2u)
		{
			if ((mode & 0x10) != 0)
			{
				regMode = 0x49;
			}
			if ((mode & 1) == 0)
			{
				regMode |= 0x100;
			}
		}
		else if (idx == 2)
		{
			if ((mode & 1) == 0)
			{
				regMode = 0x248;
			}
		}
		if ((mode & 0x1000) != 0)
		{
			regMode |= 0x10;
		}
		psxRCntRegs[idx].mode = (u16)regMode;
		ok = 1;
	}
	CTR_PSX_OBSERVE_VALUE(ok);
	return ok;
}

u32 GetRCnt(u32 spec)
{
	s32 idx = spec & 0xffffu;
	// NOTE(aalhendi): Keep the shared jr $ra epilogue via $v0 observation.
	register u32 val CTR_PSX_REGISTER("$2");

	if (idx < 3)
	{
		val = *(volatile u16 *)&psxRCntRegs[idx].count;
	}
	else
	{
		val = 0;
	}
	CTR_PSX_OBSERVE_VALUE(val);
	return val;
}

s32 StartRCnt(u32 spec)
{
	s32 idx = spec & 0xffffu;

	psxIntReg[1] |= psxRCntIrqMasks[idx];
	return idx < 3;
}

s32 StopRCnt(u32 spec)
{
	// NOTE(aalhendi): Match retail's indexed IRQ-mask load through $v0
	// after loading psxIntReg into $a1.
	u32 offset = (spec & 0xffffu) << 2;
	u32 *intReg;
	u32 page;
	u32 mask;

	CTR_PSX_KEEP_VALUE(offset);
	intReg = psxIntReg;
	CTR_PSX_ORDER_VALUES(intReg, offset);
	page = 0x80090000u;
	CTR_PSX_ORDER_VALUES(page, intReg);
	CTR_PSX_KEEP_VALUE(page);
	mask = *(u32 *)((page + offset) - 16228u);
	intReg[1] &= ~mask;
	return 1;
}

s32 ResetRCnt(u32 spec)
{
	// NOTE(aalhendi): Keep idx in $v1 and ok in $v0 across the shared exit.
	s32 idx CTR_PSX_GLOBAL_REGISTER_HINT("$3") = spec & 0xffffu;
	register s32 ok CTR_PSX_REGISTER("$2");

	if (idx < 3)
	{
		psxRCntRegs[idx].count = 0;
		ok = 1;
	}
	else
	{
		ok = 0;
	}
	CTR_PSX_OBSERVE_VALUE(ok);
	return ok;
}
