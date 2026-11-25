#include "PSX_SDK_PadInternal.h"

void PadSetAct(int port, unsigned char *table, int len)
{
	struct PsxPadSlot *slot = psxPadResolvePort(port);

	slot->actBuf = table;
	slot->actLen = (u8)len;
}

s32 _padRecvAtLoadInfo(struct PsxPadSlot *slot)
{
	switch (slot->cmdStage)
	{
	case 2:
	{
		u8 *rx = slot->rxBuf;

		if (rx[7] != 0)
		{
			return 0;
		}
		if ((slot->numModes == rx[3]) && (slot->curMode == rx[4]) && (slot->numAct == rx[5]) && (slot->numComb == rx[6]))
		{
			slot->verifyVal = 0;
		}
		else
		{
			slot->verifyVal = 0xffff;
		}
		slot->numModes = slot->rxBuf[3];
		slot->curMode = slot->rxBuf[4];
		slot->modeId = 0;
		slot->numAct = slot->rxBuf[5];
		slot->numComb = slot->rxBuf[6];
		slot->extraSize = 0;
		if (slot->verifyVal != 0)
		{
			return 0;
		}
		slot->checkFlag = 0;
		break;
	}
	case 3:
	{
		u8 *rx = slot->rxBuf;
		u16 modeId;

		if ((rx[2] != 0) || (rx[3] != 0))
		{
			return 0;
		}
		modeId = (u16)(((u32)rx[4] << 8) + rx[5]);
		slot->modeId = modeId;
		if (slot->verifyVal != modeId)
		{
			slot->verifyVal = modeId;
			return 0;
		}
		slot->verifyVal = 0xffff;
		slot->checkFlag = 0;
		slot->cmdSubIdx = 0;
		break;
	}
	case 4:
	{
		u8 *rx = slot->rxBuf;

		if ((rx[2] != 0) || (rx[3] != 0))
		{
			return 0;
		}
		{
			u32 extra = slot->extraSize + 8u;

			slot->extraSize = (u16)(extra + ((rx[4] + 3) & 0x1fc));
		}
		slot->cmdSubIdx++;
		if (slot->cmdSubIdx < slot->numComb)
		{
			return 0;
		}
		if (_padGetActSize(slot) > 128)
		{
			psxPadResetSlot(slot);
			slot->cmdStage = 0xfe;
			slot->state = 2;
		}
		else if (slot->verifyVal != slot->extraSize)
		{
			slot->verifyVal = slot->extraSize;
			slot->cmdSubIdx = 0;
			slot->extraSize = 0;
			return 0;
		}
		else
		{
			slot->verifyVal = 0;
			slot->checkFlag = 0;
			slot->cmdStage = 0xff;
			_padLoadActInfo(slot, slot->infoBuf);
			slot->cmdStage = 2;
		}
		return 0;
	}
	}
	return 1;
}

s32 _padGetActSize(struct PsxPadSlot *slot)
{
	s32 modeBytes = (((s32)slot->numModes + 1) >> 1) << 2;
	s32 actBytes = (((s32)slot->numAct * 5 + 3) & 0xffc) + 4;
	u32 extra = slot->extraSize;

	// NOTE(aalhendi): Keep the +4 constant associated with actBytes ($v1)
	// rather than folding into modeBytes ($v0).
	CTR_PSX_OBSERVE_VALUE(actBytes);
	return modeBytes + actBytes + (s32)extra;
}

s32 ReadInitPadFlag(void)
{
	return psxInitPadFlag;
}
