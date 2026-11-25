#include "PSX_SDK_PadInternal.h"

int PadSetActAlign(int port, unsigned char *table)
{
	struct PsxPadSlot *slot = psxPadResolvePort(port);

	if (psxPadCanExecCmd(slot) != 0)
	{
		return 0;
	}
	{
		s32 ret = 1;

		// NOTE(aalhendi): Keep the return value distinct from the cmdStage
		// literal so GCC emits li $v1,1 instead of move $v1,$v0.
		CTR_PSX_FORGET_VALUE(ret);
		slot->cmdStage = 1;
		slot->sendCb = _padSendActAlign;
		slot->alignPtr = table;
		slot->recvCb = _padRecvActAlign;
		return ret;
	}
}

s32 _padRecvActAlign(struct PsxPadSlot *slot)
{
	s32 act;

	if (slot->numAct != 0)
	{
		s32 actOffs;
		s32 numAct;
		u8 empty;

		act = 0;
		// NOTE(aalhendi): Prevent CSE from initializing actOffs ($t2) from act
		// ($t0) instead of $zero, and keep the 0xff constant and loop-end
		// numAct load in retail order.
		CTR_PSX_FORGET_VALUE(act);
		empty = 0xff;
		CTR_PSX_KEEP_VALUE(empty);
		actOffs = 0;
		do
		{
			s32 count;
			s32 i;
			s32 limit;
			u8 *p;

			p = slot->alignPtr;
			count = 0;
			for (i = 5; i >= 0; i--)
			{
				if (*p++ == act)
				{
					count++;
				}
			}
			limit = ((struct PsxPadActInfo *)((u32)actOffs + (u32)slot->actInfo))->size;
			p = slot->alignPtr;
			if (limit == 0)
			{
				limit = 1;
			}
			for (i = 0; i < 6; i++)
			{
				if (*p++ == act)
				{
					if (count < limit)
					{
						slot->actAlign[i] = empty;
						count--;
					}
					else
					{
						slot->actAlign[i] = (u8)act;
					}
				}
			}
			numAct = slot->numAct;
			CTR_PSX_KEEP_VALUE(numAct);
			act++;
			actOffs += 5;
		} while (act < numAct);
	}
	slot->cmdStage = 0xfe;
	return 0;
}

int PadSetMainMode(int socket, int offs, int lock)
{
	// NOTE(aalhendi): Preserve retail $s1/$s2 assignment (lock in $s1, offs in
	// $s2) and keep the return literal distinct from the cmdStage store.
	register int lockReg CTR_PSX_REGISTER("$17") = lock;
	struct PsxPadSlot *slot = psxPadResolvePort(socket);

	if (psxPadCanExecCmd(slot) != 0)
	{
		return 0;
	}
	{
		s32 ret = 1;

		CTR_PSX_FORGET_VALUE(ret);
		slot->cmdStage = 1;
		slot->sendCb = _padSendMainMode;
		slot->modeOffs = (u8)offs;
		slot->modeLock = (u8)lockReg;
		slot->recvCb = _padRecvMainMode;
		slot->modeSame = (u8)(offs == slot->curMode);
		return ret;
	}
}

void PadSetAct(int port, unsigned char *table, int len)
{
	struct PsxPadSlot *slot = psxPadResolvePort(port);

	slot->actBuf = table;
	slot->actLen = (u8)len;
}

int PadInfoAct(int port, int acno, int term)
{
	struct PsxPadSlot *slot = psxPadResolvePort(port);
	struct PsxPadActInfo *info;

	if (acno < 0)
	{
		return slot->numAct;
	}
	if (acno >= slot->numAct)
	{
		return 0;
	}
	info = &slot->actInfo[acno];
	switch (term)
	{
	case InfoActFunc:
		return info->func;
	case InfoActSub:
		return info->sub;
	case InfoActSize:
		return info->size;
	case InfoActCurr:
		return info->curr;
	case InfoActSign:
		return info->sign;
	default:
		return 0;
	}
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

s32 _padLoadActInfo(struct PsxPadSlot *slot, u8 *buf)
{
	// NOTE(aalhendi): Keep slot in $a0 across the NULL/actInfo checks so the
	// indirect psxPadCanExecCmd call omits a redundant move $a0,$s0 in its
	// delay slot, and keep the return/state literals in retail order.
	register struct PsxPadSlot *slotArg CTR_PSX_REGISTER("$4") = slot;

	if (buf == NULL)
	{
		goto fail;
	}
	if (slot->actInfo != NULL)
	{
		return 0;
	}
	if (psxPadCanExecCmd(slotArg) != 0)
	{
	fail:
		return 0;
	}
	{
		s32 ret = 1;
		u8 *p;

		CTR_PSX_FORGET_VALUE(ret);
		{
			s32 reqState = PadStateReqInfo;

			CTR_PSX_KEEP_VALUE(reqState);
			p = (u8 *)((((s32)buf + 3) >> 2) << 2);
			slot->state = (u8)reqState;
		}
		slot->cmdStage = 1;
		slot->sendCb = _padSendLoadActInfo;
		slot->recvCb = _padRecvLoadActInfo;
		slot->modeTable = (u16 *)p;
		slot->cmdSubIdx = 0;
		p += (((s32)slot->numModes + 1) >> 1) << 2;
		slot->actInfo = (struct PsxPadActInfo *)p;
		p += ((s32)slot->numAct * 5 + 3) & 0xffc;
		slot->combTable = (struct PsxPadCombEntry *)p;
		return ret;
	}
}

s32 _padRecvLoadActInfo(struct PsxPadSlot *slot)
{
	// NOTE(aalhendi): Keep the leaf return converged on $v0 and preserve retail
	// arithmetic right-shift and combination-loop register allocation.
	register s32 ret CTR_PSX_REGISTER("$2");

	switch (slot->cmdStage)
	{
	case 2:
	{
		u8 *rx = slot->rxBuf;
		u16 *modes;

		if ((rx[2] != 0) || (rx[3] != 0))
		{
			ret = 0;
			goto done;
		}
		modes = slot->modeTable;
		modes[slot->cmdSubIdx] = (u16)(((u32)rx[4] << 8) + rx[5]);
		if (slot->verifyVal != slot->modeTable[slot->cmdSubIdx])
		{
			slot->verifyVal = slot->modeTable[slot->cmdSubIdx];
			ret = 0;
			goto done;
		}
		slot->verifyVal = 0;
		slot->checkFlag = 0;
		slot->cmdSubIdx++;
		if (slot->cmdSubIdx < slot->numModes)
		{
			ret = 0;
			goto done;
		}
		slot->cmdSubIdx = 0;
		break;
	}
	case 3:
	{
		u8 *rx = slot->rxBuf;
		struct PsxPadActInfo *info;

		if ((rx[2] != 0) || (rx[3] != 0))
		{
			ret = 0;
			goto done;
		}
		info = &slot->actInfo[slot->cmdSubIdx];
		{
			s32 b5;

			if ((info->func == rx[4]) && (info->sub == (rx[5] & 0x7f)) && (info->size == rx[6]) && (info->curr == rx[7]))
			{
				b5 = rx[5];
				CTR_PSX_FORGET_VALUE(b5);
				if (info->sign == (b5 >> 7))
				{
					slot->verifyVal = 0;
				}
				else
				{
					slot->verifyVal = 0xffff;
				}
			}
			else
			{
				slot->verifyVal = 0xffff;
			}
			info->func = slot->rxBuf[4];
			info->sub = slot->rxBuf[5] & 0x7f;
			info->size = slot->rxBuf[6];
			info->curr = slot->rxBuf[7];
			b5 = slot->rxBuf[5];
			CTR_PSX_FORGET_VALUE(b5);
			info->sign = (u8)(b5 >> 7);
		}
		if (slot->verifyVal != 0)
		{
			ret = 0;
			goto done;
		}
		slot->checkFlag = 0;
		slot->cmdSubIdx++;
		if (slot->cmdSubIdx < slot->numAct)
		{
			ret = 0;
			goto done;
		}
		slot->cmdSubIdx = 0;
		slot->combRem = 0;
		break;
	}
	case 4:
	{
		u8 *rx = slot->rxBuf;
		struct PsxPadCombEntry *comb;
		u8 *src;
		register s32 count CTR_PSX_REGISTER("$7");
		s32 firstStop;

		if (rx[2] != 0)
		{
			slot->combRem = 0;
			ret = 0;
			goto done;
		}
		comb = &slot->combTable[slot->cmdSubIdx];
		if (slot->combRem == 0)
		{
			u8 len = rx[4];
			u8 *data;

			slot->combRem = len;
			comb->len = len;
			count = 3;
			CTR_PSX_FORGET_VALUE(count);
			src = &slot->rxBuf[5];
			if (slot->cmdSubIdx == 0)
			{
				data = (u8 *)slot->combTable + ((u32)slot->numComb << 3);
			}
			else
			{
				data = comb[-1].data + ((comb[-1].len + 3) & 0x1fc);
			}
			comb->data = data;
			psxPadCombCursor = data;
		}
		else
		{
			count = 5;
			src = &rx[3];
		}
		count--;
		firstStop = -1;
		CTR_PSX_FORGET_VALUE(firstStop);
		if (count != firstStop)
		{
			register u8 *endPtr CTR_PSX_REGISTER("$8") = &slot->numModes;
			register u16 mismatch CTR_PSX_REGISTER("$10") = 0xffff;
			u8 **cursorPtr = &psxPadCombCursor;
			register s32 stop CTR_PSX_REGISTER("$9");

			CTR_PSX_KEEP_VALUE(endPtr);
			CTR_PSX_KEEP_VALUE(mismatch);
			CTR_PSX_KEEP_VALUE(cursorPtr);
			stop = -1;
			CTR_PSX_KEEP_VALUE(stop);
			do
			{
				if (slot->combRem == 0)
				{
					goto check_verify;
				}
				if (psxPadCombCursor >= endPtr)
				{
					slot->cmdSubIdx = 0;
					slot->combRem = 0;
					ret = 0;
					goto done;
				}
				if (*psxPadCombCursor != *src)
				{
					slot->verifyVal = mismatch;
				}
				*(*cursorPtr)++ = *src++;
				slot->combRem--;
			} while (--count != stop);
		}
		if (slot->combRem != 0)
		{
			ret = 0;
			goto done;
		}
	check_verify:
		if (slot->verifyVal != 0)
		{
			slot->verifyVal = 0;
			slot->combRem = 0;
			ret = 0;
			goto done;
		}
		slot->cmdSubIdx++;
		if (slot->cmdSubIdx >= slot->numComb)
		{
			slot->state = PadStateStable;
			slot->cmdStage = 0xfe;
			slot->checkFlag = 0;
			ret = 0;
			goto done;
		}
		slot->combRem = 0;
		slot->checkFlag = 0;
		ret = 0;
		goto done;
	}
	}
	ret = 1;
done:
	CTR_PSX_KEEP_VALUE(ret);
	return ret;
}

#define PSX_RCNT2_COUNT  (*(volatile u16 *)0x1f801120u)
#define PSX_RCNT2_MODE   (*(volatile u16 *)0x1f801124u)
#define PSX_RCNT2_TARGET (*(volatile u16 *)0x1f801128u)

void _padCmdParaMode(struct PsxPadSlot *slot, u8 mode)
{
	slot->flag55 = 0x43;
	slot->cmdData = slot->cmdArg;
	slot->cmdArg[0] = mode;
	slot->cmdLen = 1;
}

void _padSendCmd45(struct PsxPadSlot *slot)
{
	slot->flag55 = 0x45;
	slot->cmdData = NULL;
	slot->cmdLen = 0;
}

void _padSendCmd4c(struct PsxPadSlot *slot, u8 arg)
{
	slot->flag55 = 0x4c;
	slot->cmdData = slot->cmdArg;
	slot->cmdArg[0] = arg;
	slot->cmdLen = 1;
}

void _padSendCmd46(struct PsxPadSlot *slot, u8 arg)
{
	slot->flag55 = 0x46;
	slot->cmdData = slot->cmdArg;
	slot->cmdArg[0] = arg;
	slot->cmdLen = 1;
}

void _padSendCmd47(struct PsxPadSlot *slot, u8 arg)
{
	slot->flag55 = 0x47;
	slot->cmdData = slot->cmdArg;
	slot->cmdArg[0] = arg;
	slot->cmdLen = 1;
}

void _padSendCmd4b(struct PsxPadSlot *slot)
{
	slot->flag55 = 0x4b;
	slot->cmdData = NULL;
	slot->cmdLen = 0;
}

s32 _padCheckWait(void)
{
	// NOTE(aalhendi): Prefer $a0 for the counter reading so wraparound leaves $v1
	// free for the target register pointer and the elapsed comparison matches retail.
	s32 cur CTR_PSX_GLOBAL_REGISTER_HINT("$4") = PSX_RCNT2_COUNT;
	s32 elapsed;
	s32 limit;

	if (cur < psxPadWaitStart)
	{
		if (PSX_RCNT2_TARGET != 0)
		{
			cur += PSX_RCNT2_TARGET;
		}
		else
		{
			cur += 0x10000;
		}
	}
	if ((PSX_RCNT2_MODE & 0x200) == 0)
	{
		elapsed = (cur - psxPadWaitStart) >> 3;
		limit = psxPadWaitLimit;
	}
	else
	{
		elapsed = cur - psxPadWaitStart;
		limit = psxPadWaitLimit;
	}
	return elapsed >= limit;
}

s32 _padComInit(struct PsxPadSlot *slot)
{
	psxPadComState = psxPadPreComCb(slot);
	slot->rxBuf[0] = 0;
	return psxPadTransStart(slot, -2);
}

s32 _padComSendCmd(struct PsxPadSlot *slot)
{
	if (psxPadPollCur == psxPadPollTarget && psxPadPollPending != 0)
	{
		psxPadPollCb1();
		psxPadPollCb2();
	}
	if (psxPadComState != 0)
	{
		psxPadPreComCb(&slot->subSlots[0]);
		psxPadPreComCb(&slot->subSlots[1]);
	}
	if (slot->flag55 != 0)
	{
		return psxPadTransByte(slot, slot->flag55);
	}
	return psxPadTransByte(slot, 0x42);
}

s32 _padComRecvId(struct PsxPadSlot *slot)
{
	s32 res;

	if (psxPadComState != 0)
	{
		psxPadPreComCb(&slot->subSlots[2]);
		psxPadPreComCb(&slot->subSlots[3]);
	}
	res = psxPadTransByte(slot, (slot->flag55 != 0) ? 0 : psxPadMtapMode);
	if (res < 0)
	{
		return res;
	}
	psxPadRecvBytes = (res & 0xf) << 1;
	if (psxPadRecvBytes == 0)
	{
		psxPadRecvBytes = 32;
	}
	return 0;
}

void _mtapInit(void)
{
	// NOTE(aalhendi): Match retail saved-register assignment ($s0=slots, $s1=subs)
	// and keep the final callback store ordered before the epilogue restores.
	register struct PsxPadSlot *slots CTR_PSX_REGISTER("$16") = psxPadSlots;
	register struct PsxPadSlot *subs CTR_PSX_REGISTER("$17");
	register u8 *rx CTR_PSX_REGISTER("$2");
	register u8 *tx CTR_PSX_REGISTER("$3");
	void (*recvAuto)(struct PsxPadSlot *);

	bzero(slots, sizeof(psxPadSlots));
	subs = psxPadSubSlots;
	bzero(subs, sizeof(psxPadSubSlots));
	rx = psxPadRxBufs[0];
	tx = psxPadTxBufs[0];
	slots[0].rxBuf = rx;
	rx += 35;
	slots[0].txBuf = tx;
	tx += 35;
	slots[0].subSlots = subs;
	subs += 4;
	slots[1].rxBuf = rx;
	psxPadEndComCb = _mtapEndCom;
	psxPadResetSlot = _mtapResetSlot;
	psxPadTxByteCb = _mtapTxByte;
	psxPadHeaderCb = _mtapHeaderCb;
	psxPadResolvePort = _mtapResolvePort;
	psxPadShiftFlagsCb = _mtapShiftFlags;
	psxPadPreComCb = _mtapPreCom;
	psxPadCanExecCmd = _mtapCanExecCmd;
	recvAuto = _mtapRecvAuto;
	CTR_PSX_KEEP_VALUE(recvAuto);
	psxPadRootSlots = slots;
	slots[1].txBuf = tx;
	slots[1].subSlots = subs;
	CTR_PSX_MEMORY_BARRIER();
	psxPadRecvAutoCb = recvAuto;
	CTR_PSX_MEMORY_BARRIER();
}

void _mtapResetSlot(struct PsxPadSlot *slot)
{
	if (slot->state != 0)
	{
		u8 *align;
		s32 i;
		u8 fill;

		// NOTE(aalhendi): Keep the entry branch delay slot empty as in retail.
		CTR_PSX_OBSERVE_VALUE(slot);
		align = slot->actAlign;
		fill = 0xff;
		i = 5;
		slot->state = 0;
		slot->cmdStage = 0;
		slot->modeId = 0;
		slot->sendCb = NULL;
		slot->recvCb = NULL;
		slot->numModes = 0;
		slot->curMode = 0;
		slot->modeId = 0;
		slot->numAct = 0;
		slot->numComb = 0;
		slot->modeTable = NULL;
		slot->actInfo = NULL;
		slot->combTable = NULL;
		slot->flag55 = 0;
		slot->flag56 = 0;
		slot->flag57 = 0;
		do
		{
			*align++ = fill;
			i--;
		} while (i >= 0);
	}
}

void _mtapEndCom(s32 status)
{
	// NOTE(aalhendi): Hoist loop-invariant table bases and sentinel into retail
	// saved registers ($s0=slots, $s1=subCount, $s2=-9, $s3=statusTbl) with $a1=status.
	register s32 st CTR_PSX_REGISTER("$5") = status;
	register struct PsxPadSlot *slots CTR_PSX_REGISTER("$16") = psxPadSlots;
	register s32 *statusTbl CTR_PSX_REGISTER("$19") = psxPadPortStatus;
	register s32 neg9 CTR_PSX_REGISTER("$18") = -9;
	register s32 *subCountTbl CTR_PSX_REGISTER("$17") = psxPadPortSubCount;
	s32 started;

	CTR_PSX_KEEP_VALUE(slots);
	CTR_PSX_KEEP_VALUE(statusTbl);
	CTR_PSX_KEEP_VALUE(neg9);
	do
	{
		s32 cur = psxPadPollCur;
		struct PsxPadSlot *slot;

		psxPadSioRegs[5] = 0;
		statusTbl[cur] = st;
		slot = (struct PsxPadSlot *)((u32)(cur * (s32)sizeof(struct PsxPadSlot)) + (u32)slots);
		if (st != neg9)
		{
			if (st == 0)
			{
				subCountTbl[cur] = ((slot->rxBuf[0] >> 4) == 8) ? 4 : 0;
			}
			else
			{
				_mtapFailAuto(slot, st);
			}
		}
		psxPadPortBusy = 0;
		psxPadPollCur++;
		if (psxPadPollCur <= psxPadPortMax)
		{
			started = psxPadStartPort((struct PsxPadSlot *)((u32)(psxPadPollCur * (s32)sizeof(struct PsxPadSlot)) + (u32)slots));
		}
		else
		{
			started = 1;
		}
		st = 0xffff;
	} while (started == 0);
}

void _mtapShiftFlags(struct PsxPadSlot *slot)
{
	struct PsxPadSlot *sub;
	s32 i;

	slot->flag56 = slot->flag55;
	slot->flag55 = 0;
	sub = slot->subSlots;
	for (i = 0; i < 4; i++, sub++)
	{
		sub->flag57 = sub->flag56;
		sub->flag56 = sub->flag55;
		sub->flag55 = 0;
	}
}

s32 _mtapTxByte(struct PsxPadSlot *slotArg, s32 mtapActive)
{
	// NOTE(aalhendi): Keep slot in $a0 and converge leaf returns on $v0 so the
	// command-byte switch shares the single epilogue and byte-load tail.
	struct PsxPadSlot *slot CTR_PSX_GLOBAL_REGISTER_HINT("$4") = slotArg;
	register s32 ret CTR_PSX_REGISTER("$2");
	s32 rawIdx = (s32)slot->txIdx - 3;
	s32 idx;
	s32 cmd;

	if (mtapActive != 0)
	{
		s32 subIdx = rawIdx / 8;

		idx = (rawIdx % 8) - 2;
		slot = &slot->subSlots[subIdx];
	}
	else
	{
		idx = rawIdx;
	}
	cmd = slot->flag55;
	if (idx >= 0)
	{
		u8 *p;

		switch (cmd)
		{
		case 0:
			if (idx < 6 && slot->actEnable[idx] == 0)
			{
				ret = 0;
				goto done;
			}
			if (idx < slot->actLen)
			{
				p = slot->actBuf + idx;
			readByte:
				ret = *p;
				goto done;
			}
			ret = 0;
			goto done;
		case 0x4d:
			if (idx < slot->cmdLen)
			{
				p = slot->cmdData + idx;
				goto readByte;
			}
			ret = 0xff;
			goto done;
		default:
			if (idx < slot->cmdLen)
			{
				p = slot->cmdData + idx;
				goto readByte;
			}
			ret = 0;
			goto done;
		}
	}
	CTR_PSX_OBSERVE_VALUE(idx);
	{
		s32 rem CTR_PSX_LOCAL_REGISTER_HINT("$4") = idx + 2;

		if (rem == 0)
		{
			if (cmd == 0)
			{
				ret = 0x42;
			}
			else
			{
				CTR_PSX_FORGET_VALUE(cmd);
				ret = cmd & 0xff;
			}
		}
		else
		{
			ret = 0;
		}
	}
done:
	CTR_PSX_KEEP_VALUE(ret);
	return ret;
}

struct PsxPadSlot *_mtapResolvePort(int port)
{
	struct PsxPadSlot *slot = &psxPadSlots[0];

	if ((port & 0xf0) != 0)
	{
		slot = &psxPadSlots[1];
	}
	if (psxPadMtapMode != 0)
	{
		// NOTE(aalhendi): Preserve the retail branch-delay slot boundaries on
		// the multitap port check and sub-slot index mask.
		CTR_PSX_OBSERVE_VALUE(slot);
		if (slot->mtapType == 8 && (port & 0xf) == 0)
		{
			goto useSub;
		}
		if ((port & 3) != 0)
		{
			CTR_PSX_FORGET_VALUE(port);
		useSub:
			slot = &slot->subSlots[port & 3];
		}
	}
	return slot;
}

void _mtapFailAuto(struct PsxPadSlot *slot, s32 unused)
{
	// NOTE(aalhendi): Keep slot in $s1 so the recursive multitap loop assigns
	// the loop counter to $s2 and idHi stays materialized in $s3 before checks.
	register struct PsxPadSlot *s CTR_PSX_REGISTER("$17") = slot;
	u32 idHi;
	struct PsxPadSlot *sub;
	u8 prev55;

	(void)unused;
	idHi = s->rxBuf[0] >> 4;
	CTR_PSX_KEEP_VALUE(idHi);
	if (s->mtapType != 8)
	{
		if (idHi == 8 && s == s->parent)
		{
			s->mtapType = (u8)idHi;
			s->recvBuf[0] = 0xff;
			s->recvBuf[1] = 0x80;
			s->recvLen = 2;
		}
	}
	if (s->mtapType == 8)
	{
		sub = s->subSlots;
		if (sub != NULL)
		{
			s32 i;

			for (i = 0; i < 4; i++)
			{
				_mtapFailAuto(sub++, -1);
			}
		}
	}
	sub = s->parent->subSlots;
	if (s->subSlots != NULL && s->flag55 == 0 && ((sub++)->flag56 != 0 || (sub++)->flag56 != 0 || (sub++)->flag56 != 0 || sub->flag56 != 0))
	{
		if (s->failCount == 0)
		{
			s->failCount = 1;
		}
		return;
	}
	sub = s->parent->subSlots;
	if (s->subSlots == NULL && s->flag56 == 0 &&
	    (s->parent->flag55 != 0 || (sub++)->flag56 != 0 || (sub++)->flag56 != 0 || (sub++)->flag56 != 0 || sub->flag56 != 0))
	{
		if (s->failCount == 0)
		{
			s->failCount = 1;
		}
		return;
	}
	prev55 = s->flag55;
	s->flag55 = 0;
	s->failTicks++;
	s->flag57 = s->flag56;
	s->flag56 = prev55;
	if (s->cmdStage != 0)
	{
		if (s->cmdStage == 1)
		{
			if (s->failCount < 11 && s->mtapType != 8)
			{
				s->failCount++;
				return;
			}
			if (s->modeId != 0)
			{
				if (s->failCount < 21)
				{
					s->failCount++;
					return;
				}
				psxPadResetSlot(s);
			}
			s->state = 2;
			if (s->mtapType != 8)
			{
				if (s->subSlots != NULL)
				{
					s->flag56 = 0;
					s->flag55 = 0;
				}
				s->cmdStage = 0xff;
			}
			else
			{
				s->cmdStage = 0xfe;
			}
			return;
		}
		if (s->failCount < 11)
		{
			s->failCount++;
			return;
		}
	}
	if (s->state != 0)
	{
		s->recvBuf[0] = 0xff;
		s->recvBuf[1] = 0;
		s->mtapType = 0;
		s->recvLen = 0;
		psxPadResetSlot(s);
	}
	if (idHi != 8)
	{
		if (s->mtapType == 8)
		{
			s->mtapType = 0;
			s->recvBuf[1] = 0;
		}
	}
	else if (s->subSlots != NULL)
	{
		s->mtapType = (u8)idHi;
		s->recvBuf[1] = 0x80;
	}
}

s32 _mtapCanExecCmd(struct PsxPadSlot *slot)
{
	// NOTE(aalhendi): Converge leaf returns on $v0 and place the sub-slot load
	// delay before the loop header so the back-edge targets the first loop load.
	register s32 ret CTR_PSX_REGISTER("$2");

	if (slot->modeId == 0)
	{
		ret = 1;
	}
	else if (slot->subSlots != NULL)
	{
		if (slot->cmdStage != 0xff)
		{
			ret = 1;
		}
		else
		{
			struct PsxPadSlot *sub;
			struct PsxPadSlot *parent;
			s32 i CTR_PSX_GLOBAL_REGISTER_HINT("$5") = 0;
			u32 idleStage;

			parent = slot->parent;
			idleStage = 0xff;
			CTR_PSX_LOAD_WORD_DELAYED(sub, parent->subSlots);
			do
			{
				if (sub->cmdStage != idleStage && sub->cmdStage != 0)
				{
					ret = 1;
					goto done;
				}
				i++;
				sub++;
			} while (i < 4);
			ret = 0;
		}
	}
	else
	{
		s32 res = 0;

		if (slot->parent->cmdStage != 0xff || slot->cmdStage != 0xff)
		{
			res = 1;
		}
		ret = res;
	}
done:
	CTR_PSX_KEEP_VALUE(ret);
	return ret;
}

s32 ReadInitPadFlag(void)
{
	return psxInitPadFlag;
}
