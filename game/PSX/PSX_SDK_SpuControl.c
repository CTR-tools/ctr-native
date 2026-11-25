#include "PSX_SDK_SpuInternal.h"

SpuIRQCallbackProc SpuSetIRQCallback(SpuIRQCallbackProc func)
{
	SpuIRQCallbackProc old = psxSpuIrqCallback;

	if (func != old)
	{
		// NOTE(aalhendi): Keep the SDK's $at-based store in _SpuCallback's delay slot.
		register u32 statePage CTR_PSX_REGISTER("$1");
		statePage = 0x80090000u;
		CTR_PSX_KEEP_VALUE(statePage);
		*(SpuIRQCallbackProc *)(statePage - 15376u) = func;
		_SpuCallback(func);
	}
	return old;
}

s32 SpuReadDecodedData(void *d_data, s32 flag)
{
	u32 status;

	switch (flag)
	{
	case SPU_CDONLY:
		_spu_Fr_(d_data, 0, 32);
		break;
	case SPU_VOICEONLY:
		_spu_Fr_((u8 *)d_data + 0x800, 0x100, 32);
		break;
	default:
		_spu_Fr_(d_data, 0, 64);
		break;
	}
	status = psxSpuSdkRegs[0x1ae / sizeof(u16)] & 0x800u;
	// NOTE(aalhendi): Keep retail's andi/sltu instead of combine folding to srl/andi.
	CTR_PSX_FORGET_VALUE(status);
	return status != 0;
}

s32 SpuSetReverb(s32 on_off)
{
	switch (on_off)
	{
	case SPU_OFF:
		psxSpuRevFlag = 0;
		psxSpuSdkRegs[0x1aa / sizeof(u16)] &= ~0x80u;
		break;

	case SPU_ON:
		if ((psxSpuRevReserveWa != 1) && (_SpuIsInAllocateArea_(psxSpuRevOffsetAddr) != 0))
		{
			psxSpuRevFlag = 0;
			psxSpuSdkRegs[0x1aa / sizeof(u16)] &= ~0x80u;
		}
		else
		{
			psxSpuRevFlag = on_off;
			psxSpuSdkRegs[0x1aa / sizeof(u16)] |= 0x80u;
		}
		break;
	}
	return psxSpuRevFlag;
}

u32 SpuWrite(u8 *addr, u32 size)
{
	if (size > 0x7eff0u)
	{
		size = 0x7eff0u;
	}
	_spu_Fr(addr, size);
	if (psxSpuTransferCallback == NULL)
	{
		psxSpuInTransfer = 0;
	}
	return size;
}

s32 SpuIsTransferCompleted(s32 flag)
{
	s32 event;

	if ((psxSpuTransferModeRaw == 1) || (psxSpuInTransfer == 1))
	{
		return 1;
	}
	event = TestEvent(psxSpuEvDma);
	if (flag == SPU_TRANSFER_WAIT)
	{
		while (event == 0)
		{
			event = TestEvent(psxSpuEvDma);
		}
		psxSpuInTransfer = 1;
		return 1;
	}
	if (event == 1)
	{
		psxSpuInTransfer = 1;
	}
	return event;
}

void _SpuInit(s32 mode)
{
	ResetCallback();
	_spu_init(mode);
	if (mode == 0)
	{
		// NOTE(aalhendi): Keep retail's $v0/$v1 pointer-and-counter assignment
		// and place the 0xc000 constant load into the branch delay slot.
		u16 centerNote = 0xc000;
		u16 *note CTR_PSX_GLOBAL_REGISTER_HINT("$2");
		s32 i CTR_PSX_GLOBAL_REGISTER_HINT("$3");

		CTR_PSX_KEEP_VALUE(centerNote);
		i = 23;
		CTR_PSX_ORDER_VALUES(i, centerNote);
		note = &psxSpuVoiceCenterNote[23];
		do
		{
			*note-- = centerNote;
		} while (--i >= 0);
	}
	SpuStart();
	psxSpuRevFlag = 0;
	psxSpuRevReserveWa = 0;
	psxSpuRevAttr.mode = 0;
	psxSpuRevAttr.depth.left = 0;
	psxSpuRevAttr.depth.right = 0;
	psxSpuRevAttr.delay = 0;
	psxSpuRevAttr.feedback = 0;
	psxSpuRevOffsetAddr = psxSpuRevStartAddr[0];
	_spu_FsetRXX(209, psxSpuRevOffsetAddr, 0);
	psxSpuMemListSize = 0;
	psxSpuMemListCount = 0;
	psxSpuMemList = NULL;
	psxSpuTransferModeRaw = 0;
	psxSpuTransferMode = 0;
	psxSpuKeyStat = 0;
	psxSpuRQMask = 0;
	psxSpuRQVoice = 0;
	psxSpuEnv = 0;
	// NOTE(aalhendi): Keep epilogue restores after the trailing state clears.
	CTR_PSX_MEMORY_BARRIER();
}

s32 SpuClearReverbWorkArea(s32 mode)
{
	volatile SpuTransferCallbackProc savedCb = NULL;
	s32 restoreMode = 0;
	// NOTE(aalhendi): Keep startPtr and rem in $s1 (with mode/chunk in $s0)
	// and materialize the start-address table base in $v1 before indexing.
	u32 *startPtr CTR_PSX_GLOBAL_REGISTER_HINT("$17");
	u32 rem CTR_PSX_GLOBAL_REGISTER_HINT("$17");
	u32 addr;
	s32 savedMode;
	s32 more;

	if ((u32)mode >= SPU_REV_MODE_MAX)
	{
		return -1;
	}
	{
		u32 *table CTR_PSX_LOCAL_REGISTER_HINT("$3") = psxSpuRevStartAddr;
		CTR_PSX_KEEP_VALUE(table);
		startPtr = (u32 *)((u32)(mode * 4) + (u32)table);
	}
	if (_SpuIsInAllocateArea_(*startPtr) != 0)
	{
		return -1;
	}
	if (mode == SPU_REV_MODE_OFF)
	{
		rem = 0x10u << psxSpuMemModePlus;
		addr = 0xfff0u << psxSpuMemModePlus;
	}
	else
	{
		u32 start = *startPtr;
		rem = (0x10000u - start) << psxSpuMemModePlus;
		addr = start << psxSpuMemModePlus;
	}

	savedMode = psxSpuTransferMode;
	if (savedMode == SPU_TRANSFER_BY_IO)
	{
		psxSpuTransferMode = SPU_TRANSFER_BY_DMA;
		// NOTE(aalhendi): Prevent CSE from copying the comparison constant from $v0.
		CTR_PSX_CLOBBER("$2");
		restoreMode = 1;
	}
	more = 1;
	if (psxSpuTransferCallback != NULL)
	{
		savedCb = psxSpuTransferCallback;
		psxSpuTransferCallback = NULL;
	}

	do
	{
		u32 chunk = 1024u;
		if (rem <= 1024u)
		{
			chunk = rem;
			more = 0;
		}
		_spu_t(2, addr);
		_spu_t(1);
		_spu_t(3, psxSpuZeroBuf, chunk);
		WaitEvent(psxSpuEvDma);
		rem -= 1024u;
		addr += 1024u;
	} while (more != 0);

	if (restoreMode != 0)
	{
		psxSpuTransferMode = savedMode;
	}
	if (savedCb != NULL)
	{
		psxSpuTransferCallback = savedCb;
	}
	return 0;
}
