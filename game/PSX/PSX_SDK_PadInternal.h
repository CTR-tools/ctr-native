#ifndef CTR_PSX_SDK_PAD_INTERNAL_H
#define CTR_PSX_SDK_PAD_INTERNAL_H

#include <ctr_compiler.h>
#include <macros.h>
#include <psx/libapi.h>
#include <psx/libetc.h>
#include <psx/libpad.h>

// NOTE(aalhendi): Retail libpad keeps controller slot descriptors and driver
// globals across 0x8008c770..0x8008cb3c; these declarations name existing
// storage only.
struct PsxPadActInfo
{
	u8 func;
	u8 sub;
	u8 size;
	u8 curr;
	u8 sign;
};

struct PsxPadCombEntry
{
	u8 len;
	u8 pad[3];
	u8 *data;
};

struct PsxPadSlot
{
	u16 *modeTable;
	struct PsxPadActInfo *actInfo;
	struct PsxPadCombEntry *combTable;
	struct PsxPadSlot *subSlots;
	struct PsxPadSlot *parent;
	void (*sendCb)(struct PsxPadSlot *);
	s32 (*recvCb)(struct PsxPadSlot *);
	u32 unk1c;
	u8 *alignPtr;
	u8 cmdArg[4];
	u8 *actBuf;
	u8 *cmdData;
	u8 *recvBuf;
	u8 actLen;
	u8 recvLen;
	u8 cmdLen;
	u8 flag55;
	u8 flag56;
	u8 flag57;
	u8 unk3a;
	u8 unk3b;
	u8 *volatile rxBuf;
	u8 *txBuf;
	u8 recvBufLen;
	u8 txIdx;
	u8 cmdStage;
	u8 cmdSubIdx;
	u8 combRem;
	u8 state;
	u8 failCount;
	u8 unk4b;
	u32 failTicks;
	u8 unk50;
	u8 modeOffs;
	u8 modeLock;
	u8 modeSame;
	u8 unk54[3];
	u8 actEnable[6];
	u8 actAlign[6];
	u8 infoBuf[128];
	u8 numModes;
	u8 curMode;
	u8 unkE5;
	u16 modeId;
	u8 mtapType;
	u8 numAct;
	u8 numComb;
	u8 checkFlag;
	u16 extraSize;
	u16 verifyVal;
};

extern void (*psxPadEndComCb)(s32 status);
extern void (*psxPadResetSlot)(struct PsxPadSlot *slot);
extern s32 (*psxPadTxByteCb)(struct PsxPadSlot *slot, s32 mtapActive);
extern void (*psxPadHeaderCb)(struct PsxPadSlot *slot);
extern struct PsxPadSlot *(*psxPadResolvePort)(int port);
extern s32 (*psxPadPreComCb)(struct PsxPadSlot *slot);
extern s32 (*psxPadCanExecCmd)(struct PsxPadSlot *slot);
extern void (*psxPadRecvAutoCb)(struct PsxPadSlot *slot);
extern void (*psxPadShiftFlagsCb)(struct PsxPadSlot *slot);
extern void (*psxPadPollCb1)(void);
extern void (*psxPadPollCb2)(void);
extern struct PsxPadSlot *psxPadRootSlots;
extern s32 psxPadPollPending;
extern s32 psxPadPollCur;
extern s32 psxPadPortBusy;
extern s32 psxPadMtapMode;
extern s32 psxPadPollTarget;
extern s32 psxPadPortMax;
extern s32 psxPadPortSubCount[2];
extern u8 psxPadIntrNode[];
extern s32 psxPadRecvBytes;
extern s32 psxPadComState;
extern s32 psxPadPortStatus[2];
extern volatile u16 *psxPadSioRegs;
extern s32 psxInitPadFlag;
extern u8 *psxPadCombCursor;
extern s32 psxPadWaitStart;
extern s32 psxPadWaitLimit;
extern struct PsxPadSlot psxPadSlots[2];
extern struct PsxPadSlot psxPadSubSlots[8];
extern u8 psxPadRxBufs[2][35];
extern u8 psxPadTxBufs[2][35];

extern void PadSetCmd(struct PsxPadSlot *slot, s32 cmd, u8 *buf, s32 len);
extern void _padSendActAlign(struct PsxPadSlot *slot);
extern s32 _padRecvActAlign(struct PsxPadSlot *slot);
extern void _padSendMainMode(struct PsxPadSlot *slot);
extern s32 _padRecvMainMode(struct PsxPadSlot *slot);
extern void _padCmdParaMode(struct PsxPadSlot *slot, u8 mode);
extern void _padSendCmd45(struct PsxPadSlot *slot);
extern void _padSendCmd4c(struct PsxPadSlot *slot, u8 curMode);
extern void _padSendCmd46(struct PsxPadSlot *slot, u8 subIdx);
extern void _padSendCmd47(struct PsxPadSlot *slot, u8 subIdx);
extern void _padSendCmd4b(struct PsxPadSlot *slot);
extern s32 _padCheckWait(void);
extern s32 _padComInit(struct PsxPadSlot *slot);
extern s32 _padComSendCmd(struct PsxPadSlot *slot);
extern s32 _padComRecvId(struct PsxPadSlot *slot);
extern s32 psxPadStartPort(struct PsxPadSlot *slot);
extern s32 psxPadTransStart(struct PsxPadSlot *slot, s32 val);
extern s32 psxPadTransByte(struct PsxPadSlot *slot, s32 byte);
extern s32 _padGetActSize(struct PsxPadSlot *slot);
extern s32 _padLoadActInfo(struct PsxPadSlot *slot, u8 *buf);
extern void _padSendLoadActInfo(struct PsxPadSlot *slot);
extern s32 _padRecvLoadActInfo(struct PsxPadSlot *slot);
extern void _mtapInit(void);
extern void _mtapResetSlot(struct PsxPadSlot *slot);
extern void _mtapEndCom(s32 status);
extern void _mtapShiftFlags(struct PsxPadSlot *slot);
extern s32 _mtapTxByte(struct PsxPadSlot *slot, s32 mtapActive);
extern void _mtapHeaderCb(struct PsxPadSlot *slot);
extern struct PsxPadSlot *_mtapResolvePort(int port);
extern s32 _mtapPreCom(struct PsxPadSlot *slot);
extern void _mtapRecvAuto(struct PsxPadSlot *slot);
extern void _mtapFailAuto(struct PsxPadSlot *slot, s32 unused);
extern s32 _mtapCanExecCmd(struct PsxPadSlot *slot);
extern void bzero(void *dst, u32 len);
extern void SysDeqIntRP(s32 prio, void *node);
extern void ChangeClearRCnt(s32 spec, s32 flag);

#endif
