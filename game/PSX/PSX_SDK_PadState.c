#include "PSX_SDK_PadInternal.h"

int PadGetState(int port)
{
	struct PsxPadSlot *slot = psxPadResolvePort(port);

	if ((slot->flag55 != 0) || (slot->flag56 != 0) || ((slot != slot->parent) && (slot->flag57 != 0)) || (slot->recvBuf[0] != 0))
	{
		switch (slot->state)
		{
		case PadStateFindCTP1:
			return PadStateFindPad;
		case PadStateFindCTP2:
			return PadStateFindPad;
		case PadStateStable:
			return PadStateReqInfo;
		}
	}
	return slot->state;
}

void PadStopCom(void)
{
	EnterCriticalSection();
	ChangeClearRCnt(3, 1);
	SysDeqIntRP(2, psxPadIntrNode);
	ExitCriticalSection();
}

void _padSendActAlign(struct PsxPadSlot *slot)
{
	PadSetCmd(slot, 0x4d, slot->alignPtr, 6);
}

void _padSendMainMode(struct PsxPadSlot *slot)
{
	switch (slot->cmdStage)
	{
	case 2:
		PadSetCmd(slot, 0x44, &slot->modeOffs, 2);
		break;
	case 3:
		PadSetCmd(slot, 0x4d, slot->actAlign, 6);
		break;
	}
}

s32 _padRecvMainMode(struct PsxPadSlot *slot)
{
	if (slot->modeSame != 0)
	{
		if (slot->cmdStage == 2)
		{
			return 1;
		}
		slot->cmdStage = 0xfe;
		return 0;
	}
	psxPadResetSlot(slot);
	return 0;
}

void _padSendAtLoadInfo(struct PsxPadSlot *slot)
{
	switch (slot->cmdStage)
	{
	case 2:
		psxPadSendCmd45(slot);
		break;
	case 3:
		psxPadSendCmd4c(slot, slot->curMode);
		break;
	case 4:
		psxPadSendCmd4cExtra(slot, slot->cmdSubIdx);
		break;
	}
}

void _padSendLoadActInfo(struct PsxPadSlot *slot)
{
	switch (slot->cmdStage)
	{
	case 2:
		psxPadSendCmd4c(slot, slot->cmdSubIdx);
		break;
	case 3:
		psxPadSendCmd4cAct(slot, slot->cmdSubIdx);
		break;
	case 4:
		if (slot->combRem == 0)
		{
			psxPadSendCmd4cExtra(slot, slot->cmdSubIdx);
		}
		else
		{
			psxPadSendCmd4cNext(slot);
		}
		break;
	}
}
