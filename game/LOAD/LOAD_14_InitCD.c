#include <common.h>
#ifdef CTR_NATIVE
#include <platform/native_cd.h>
#endif

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8007c118-0x8007c208.
int LOAD_InitCDvol(void)
{
#ifndef CTR_NATIVE
	if ((SPU_CURRENT_VOL_L == 0) && (SPU_CURRENT_VOL_R == 0))
	{
		SPU_MASTER_VOL_L = 0x3fff;
		SPU_MASTER_VOL_R = 0x3fff;
	}

	SPU_CD_VOL_L = 0x3fff;
	SPU_CD_VOL_R = 0x3fff;
	SPU_CTRL = 0xc001;

	CD_REG(0) = 2;
	CD_REG(2) = 0x80;
	CD_REG(3) = 0;
	CD_REG(0) = 3;
	CD_REG(1) = 0x80;
	CD_REG(2) = 0;
	CD_REG(3) = 0x20;
#else
	// NOTE(aalhendi): Native CD/XA mix is owned by the native audio backend, not PSX MMIO.
#endif

	return 0;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80031c58-0x80031c78 for the retail path.
void LOAD_InitCD()
{
#ifdef CTR_NATIVE
	NativeCD_Init();
	CDSYS_Init(0);
	return;
#endif

	CDSYS_Init(1);
}
