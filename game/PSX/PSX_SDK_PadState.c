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
