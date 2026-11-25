#include <ctr_compiler.h>
#include <macros.h>

extern volatile u32 *psxDmaGpuMadr;
extern volatile u32 *psxDmaGpuChcr;
extern volatile u32 *psxGpuStat;

s32 BreakDraw(void)
{
	volatile u32 addr;
	volatile u32 *chcr = psxDmaGpuChcr;

	if ((*chcr & 0x01000000u) == 0)
	{
		return 0;
	}
	if (((*chcr & 0x700u) >> 8) != 4)
	{
		return -1;
	}
	*chcr &= ~0x01000000u;
	addr = *psxDmaGpuChcr;
	addr = *psxDmaGpuMadr;
	if ((addr & 0x00ffffffu) == 0x00ffffffu)
	{
		return 0;
	}
	return (s32)addr;
}

s32 IsIdleGPU(s32 maxCount)
{
	volatile u32 *stat = psxGpuStat;
	s32 count = 0;
	// NOTE(aalhendi): Keep the return value in $v0 across the shared epilogue
	// so GCC preserves retail's jump to the single jr $ra exit.
	register s32 res CTR_PSX_REGISTER("$2");

	if ((*stat & 0x04000000u) == 0)
	{
		// NOTE(aalhendi): Keep the polling pointer in $v1 and mask in $a2
		// inside the wait loop, matching retail's loop preheader layout.
		register volatile u32 *loopStat CTR_PSX_REGISTER("$3");
		register u32 mask CTR_PSX_REGISTER("$6");

		loopStat = stat;
		CTR_PSX_KEEP_VALUE(loopStat);
		mask = 0x04000000u;
		CTR_PSX_KEEP_VALUE(mask);
		do
		{
			if (count++ > maxCount)
			{
				res = -1;
				goto done;
			}
		} while ((*loopStat & mask) == 0);
	}
	res = 0;
done:
	CTR_PSX_OBSERVE_VALUE(res);
	return res;
}
