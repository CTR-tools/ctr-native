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
	u32 unk24;
	u8 *actBuf;
	u32 unk2c;
	u8 *recvBuf;
	u8 actLen;
	u8 recvLen;
	u8 unk36;
	u8 flag55;
	u8 flag56;
	u8 flag57;
	u8 unk3a;
	u8 unk3b;
	u8 *volatile rxBuf;
	u8 *txBuf;
	u8 unk44[2];
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
	u8 unk54[9];
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

extern void (*psxPadResetSlot)(struct PsxPadSlot *slot);
extern struct PsxPadSlot *(*psxPadResolvePort)(int port);
extern s32 (*psxPadCanExecCmd)(struct PsxPadSlot *slot);
extern u8 psxPadIntrNode[];
extern s32 psxInitPadFlag;
extern u8 *psxPadCombCursor;

extern void PadSetCmd(struct PsxPadSlot *slot, s32 cmd, u8 *buf, s32 len);
extern void _padSendActAlign(struct PsxPadSlot *slot);
extern s32 _padRecvActAlign(struct PsxPadSlot *slot);
extern void _padSendMainMode(struct PsxPadSlot *slot);
extern s32 _padRecvMainMode(struct PsxPadSlot *slot);
extern void psxPadSendCmd45(struct PsxPadSlot *slot);
extern void psxPadSendCmd4c(struct PsxPadSlot *slot, u8 curMode);
extern void psxPadSendCmd4cAct(struct PsxPadSlot *slot, u8 subIdx);
extern void psxPadSendCmd4cExtra(struct PsxPadSlot *slot, u8 subIdx);
extern void psxPadSendCmd4cNext(struct PsxPadSlot *slot);
extern s32 _padGetActSize(struct PsxPadSlot *slot);
extern s32 _padLoadActInfo(struct PsxPadSlot *slot, u8 *buf);
extern void _padSendLoadActInfo(struct PsxPadSlot *slot);
extern s32 _padRecvLoadActInfo(struct PsxPadSlot *slot);
extern void _mtapFailAuto(struct PsxPadSlot *slot, s32 unused);
extern s32 _mtapCanExecCmd(struct PsxPadSlot *slot);
extern void SysDeqIntRP(s32 prio, void *node);
extern void ChangeClearRCnt(s32 spec, s32 flag);

#endif
