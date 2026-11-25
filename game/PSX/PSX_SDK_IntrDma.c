#include <ctr_compiler.h>
#include <macros.h>

// NOTE(aalhendi): Retail libetc stores the DICR pointer (0x8008cb04), DMA
// callback table (0x8008cb08), and DMA0 MADR base pointer (0x8008cb28) in
// resident data; these declarations name existing storage only.
extern volatile u32 *psxDmaDicr;
extern void (*psxDmaCallbacks[8])(void);
extern volatile u32 *psxDmaMadrBase;
extern s32 printf(const char *fmt, ...);

void _trapIntrDMA(void)
{
	u32 flags;
	s32 i;

	flags = (*psxDmaDicr >> 24) & 0x7fu;
	if (flags != 0)
	{
		// NOTE(aalhendi): Keep the hoisted shift base in $s4 so GCC 2.8.1 matches
		// the retail saved-register assignment ($s3=0x00ffffff, $s4=1, $s5=table).
		register u32 one CTR_PSX_REGISTER("$20") = 1;
		u32 lowMask = 0x00ffffffu;
		void (**callbacks)(void) = psxDmaCallbacks;

		CTR_PSX_KEEP_VALUE(one);
		do
		{
			for (i = 0; flags != 0 && i < 7; i++, flags >>= 1)
			{
				if (flags & 1)
				{
					*psxDmaDicr = *psxDmaDicr & ((one << (i + 24)) | lowMask);
					if (callbacks[i] != NULL)
					{
						callbacks[i]();
					}
				}
			}
			flags = (*psxDmaDicr >> 24) & 0x7fu;
		} while (flags != 0);
	}
	if (((*psxDmaDicr & 0xff000000u) == 0x80000000u) || ((*psxDmaDicr & 0x8000u) != 0))
	{
		printf("DMA bus error: code=%08x\n", *psxDmaDicr);
		for (i = 0; i < 7; i++)
		{
			printf("MADR[%d]=%08x\n", i, psxDmaMadrBase[i * 4]);
		}
	}
}

void *_setIntrDMA(s32 ch, void (*func)(void))
{
	s32 chReg = ch;
	// NOTE(aalhendi): Match retail leaf register homes ($v1=slot, $a0=func,
	// $a2=ch, $a3=old) and offset-first pointer add order under GCC 2.8.1.
	register void (**base)(void) CTR_PSX_REGISTER("$3");
	void (**slot)(void);
	void (*old)(void);
	register void (*funcReg)(void) CTR_PSX_REGISTER("$4");
	void (*ret)(void);

	CTR_PSX_KEEP_VALUE(chReg);
	base = psxDmaCallbacks;
	CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(slot, base, (u32)chReg << 2);
	old = *slot;
	funcReg = func;
	if (funcReg != old)
	{
		if (funcReg != NULL)
		{
			volatile u32 *dicr = psxDmaDicr;
			u32 cur;
			u32 mask;

			*slot = funcReg;
			cur = *dicr & 0x00ffffffu;
			mask = (1u << (chReg + 16)) | 0x00800000u;
			// NOTE(aalhendi): Keep chReg live across the DICR update so $a2 stays
			// pinned and does not merge with scratch temporaries.
			CTR_PSX_OBSERVE_VALUE(chReg);
			*dicr = cur | mask;
		}
		else
		{
			volatile u32 *dicr = psxDmaDicr;

			// NOTE(aalhendi): Non-volatile value reset prevents GCC 2.8.1 from
			// reusing $a0 (known zero) for *slot = NULL while still letting the
			// branch delay slot hoist the shared 0x00ff0000 mask upper half.
			CTR_PSX_KEEP_VALUE_RELAXED(funcReg);
			*slot = NULL;
			*dicr = ((*dicr & 0x00ffffffu) | 0x00800000u) & ~(1u << (chReg + 16));
			CTR_PSX_OBSERVE_VALUE(chReg);
		}
	}
	ret = old;
	// NOTE(aalhendi): Keep the return copy in $v0 at each exit edge.
	CTR_PSX_KEEP_VALUE(ret);
	return ret;
}

void _clearIntrDMATable(void **buf, s32 n)
{
	s32 i;

	for (i = n - 1; i != -1; i--)
	{
		*buf++ = NULL;
	}
}
