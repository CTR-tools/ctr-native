#include "PSX_SDK_SpuInternal.h"

struct PsxSpuReverbRegs
{
	u32 mask;
	u16 reg[32];
};

s32 _SpuIsInAllocateArea_(u32 addr)
{
	u32 *list;
	s32 i;
	s32 found;

	addr <<= psxSpuMemModePlus;
	list = (u32 *)psxSpuMemList;
	if (list == NULL)
	{
		found = 0;
		goto done;
	}
	for (i = 0;; i += 2)
	{
		u32 head = list[i];

		if ((head & 0x80000000u) == 0)
		{
			u32 start;

			if ((head & 0x40000000u) != 0)
			{
				break;
			}
			start = head & 0x0fffffffu;
			if (start >= addr)
			{
				found = 1;
				goto done;
			}
			if (addr < start + list[i + 1])
			{
				found = 1;
				goto done;
			}
		}
	}
	found = 0;
done:
	// NOTE(aalhendi): Keep all exit paths converging on a single leaf return
	// so GCC hoists the 32-bit flag masks ahead of the loop.
	CTR_PSX_KEEP_VALUE(found);
	return found;
}

void _spu_setReverbAttr(struct PsxSpuReverbRegs *attr)
{
	u32 mask = attr->mask;
	s32 all = (mask == 0);

	if (all || (mask & (1u << 0)))
	{
		psxSpuSdkRegs[224] = attr->reg[0];
	}
	if (all || (mask & (1u << 1)))
	{
		psxSpuSdkRegs[225] = attr->reg[1];
	}
	if (all || (mask & (1u << 2)))
	{
		psxSpuSdkRegs[226] = attr->reg[2];
	}
	if (all || (mask & (1u << 3)))
	{
		psxSpuSdkRegs[227] = attr->reg[3];
	}
	if (all || (mask & (1u << 4)))
	{
		psxSpuSdkRegs[228] = attr->reg[4];
	}
	if (all || (mask & (1u << 5)))
	{
		psxSpuSdkRegs[229] = attr->reg[5];
	}
	if (all || (mask & (1u << 6)))
	{
		psxSpuSdkRegs[230] = attr->reg[6];
	}
	if (all || (mask & (1u << 7)))
	{
		psxSpuSdkRegs[231] = attr->reg[7];
	}
	if (all || (mask & (1u << 8)))
	{
		psxSpuSdkRegs[232] = attr->reg[8];
	}
	if (all || (mask & (1u << 9)))
	{
		psxSpuSdkRegs[233] = attr->reg[9];
	}
	if (all || (mask & (1u << 10)))
	{
		psxSpuSdkRegs[234] = attr->reg[10];
	}
	if (all || (mask & (1u << 11)))
	{
		psxSpuSdkRegs[235] = attr->reg[11];
	}
	if (all || (mask & (1u << 12)))
	{
		psxSpuSdkRegs[236] = attr->reg[12];
	}
	if (all || (mask & (1u << 13)))
	{
		psxSpuSdkRegs[237] = attr->reg[13];
	}
	if (all || (mask & (1u << 14)))
	{
		psxSpuSdkRegs[238] = attr->reg[14];
	}
	if (all || (mask & (1u << 15)))
	{
		psxSpuSdkRegs[239] = attr->reg[15];
	}
	if (all || (mask & (1u << 16)))
	{
		psxSpuSdkRegs[240] = attr->reg[16];
	}
	if (all || (mask & (1u << 17)))
	{
		psxSpuSdkRegs[241] = attr->reg[17];
	}
	if (all || (mask & (1u << 18)))
	{
		psxSpuSdkRegs[242] = attr->reg[18];
	}
	if (all || (mask & (1u << 19)))
	{
		psxSpuSdkRegs[243] = attr->reg[19];
	}
	if (all || (mask & (1u << 20)))
	{
		psxSpuSdkRegs[244] = attr->reg[20];
	}
	if (all || (mask & (1u << 21)))
	{
		psxSpuSdkRegs[245] = attr->reg[21];
	}
	if (all || (mask & (1u << 22)))
	{
		psxSpuSdkRegs[246] = attr->reg[22];
	}
	if (all || (mask & (1u << 23)))
	{
		psxSpuSdkRegs[247] = attr->reg[23];
	}
	if (all || (mask & (1u << 24)))
	{
		psxSpuSdkRegs[248] = attr->reg[24];
	}
	if (all || (mask & (1u << 25)))
	{
		psxSpuSdkRegs[249] = attr->reg[25];
	}
	if (all || (mask & (1u << 26)))
	{
		psxSpuSdkRegs[250] = attr->reg[26];
	}
	if (all || (mask & (1u << 27)))
	{
		psxSpuSdkRegs[251] = attr->reg[27];
	}
	if (all || (mask & (1u << 28)))
	{
		psxSpuSdkRegs[252] = attr->reg[28];
	}
	if (all || (mask & (1u << 29)))
	{
		psxSpuSdkRegs[253] = attr->reg[29];
	}
	if (all || (mask & (1u << 30)))
	{
		psxSpuSdkRegs[254] = attr->reg[30];
	}
	if (all || (mask & (1u << 31)))
	{
		psxSpuSdkRegs[255] = attr->reg[31];
	}
}
