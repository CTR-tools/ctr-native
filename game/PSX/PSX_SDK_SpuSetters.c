#include <macros.h>
#include <psx/libspu.h>
#include <psx/hwregs.h>

extern SpuVolume psxSpuReverbDepth;

void SpuSetCommonCDVolume(s16 left, s16 right)
{
	volatile u16 *registers = psxSpuSdkRegs;
	registers[0x1b0 / sizeof(u16)] = left;
	registers[0x1b2 / sizeof(u16)] = right;
}

void SpuSetReverbModeDepth(s16 left, s16 right)
{
	volatile u16 *registers = psxSpuSdkRegs;
	registers[0x184 / sizeof(u16)] = left;
	registers[0x186 / sizeof(u16)] = right;
	psxSpuReverbDepth.left = left;
	psxSpuReverbDepth.right = right;
}

void SpuSetCommonCDMix(s32 enabled)
{
	volatile u16 *registers;
	u16 control;
	if (enabled == 0)
	{
		registers = psxSpuSdkRegs;
		control = registers[0x1aa / sizeof(u16)] & ~1u;
	}
	else
	{
		registers = psxSpuSdkRegs;
		control = registers[0x1aa / sizeof(u16)] | 1u;
	}
	registers[0x1aa / sizeof(u16)] = control;
}

void SpuSetCommonMasterVolume(s16 left, s16 right)
{
	volatile u16 *registers;
	left &= 0x7fff;
	registers = psxSpuSdkRegs;
	right &= 0x7fff;
	registers[0x180 / sizeof(u16)] = left;
	registers[0x182 / sizeof(u16)] = right;
}

void SpuSetCommonCDReverb(s32 enabled)
{
	volatile u16 *registers;
	u16 control;
	if (enabled == 0)
	{
		registers = psxSpuSdkRegs;
		control = registers[0x1aa / sizeof(u16)] & ~4u;
	}
	else
	{
		registers = psxSpuSdkRegs;
		control = registers[0x1aa / sizeof(u16)] | 4u;
	}
	registers[0x1aa / sizeof(u16)] = control;
}
