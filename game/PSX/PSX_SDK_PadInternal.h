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
struct PsxPadSlot
{
	u32 unk00;
	void *actInfo;
	u32 unk08;
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
	u8 unk35;
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
	u8 unk48;
	u8 state;
	u8 unk4a[25];
	u8 infoBuf[128];
	u8 numModes;
	u8 curMode;
	u8 unkE5;
	u16 modeId;
	u8 unkE8;
	u8 numAct;
	u8 numComb;
	u8 checkFlag;
	u16 extraSize;
	u16 verifyVal;
};

extern void (*psxPadResetSlot)(struct PsxPadSlot *slot);
extern struct PsxPadSlot *(*psxPadResolvePort)(int port);
extern u8 psxPadIntrNode[];
extern s32 psxInitPadFlag;

extern void psxPadSendCmd45(struct PsxPadSlot *slot);
extern void psxPadSendCmd4c(struct PsxPadSlot *slot, u8 curMode);
extern void psxPadSendCmd4cExtra(struct PsxPadSlot *slot, u8 subIdx);
extern s32 _padGetActSize(struct PsxPadSlot *slot);
extern void _padLoadActInfo(struct PsxPadSlot *slot, u8 *buf);
extern void SysDeqIntRP(s32 prio, void *node);
extern void ChangeClearRCnt(s32 spec, s32 flag);

#endif
