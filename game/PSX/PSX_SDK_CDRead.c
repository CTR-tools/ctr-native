#include <ctr_compiler.h>
#include <macros.h>
#include <psx/libcd.h>
#include <psx/libetc.h>

// NOTE(aalhendi): Retail libcd owns this 52-byte transfer state block at
// 0x8008af00; these declarations name existing storage only.
struct PsxCdReadState
{
	s32 sectors;
	u32 *volatile buf;
	s32 unk8;
	volatile s32 mode;
	s32 sectorWords;
	volatile s32 leftSectors;
	volatile s32 lastTick;
	volatile s32 startTick;
	s32 unk20;
	volatile s32 active;
	CdlCB savedSyncCb;
	volatile CdlCB savedReadyCb;
	void (*savedDataCb)(void);
};

extern struct PsxCdReadState CD_readState;
extern s32 CD_dataMode;
extern s32 CD_read_retry(s32 mode);

int CdRead(int sectors, u32 *buf, int mode)
{
	s32 startTick;
	struct PsxCdReadState *st;
	s32 ok;

	if (CD_readState.active != 0)
	{
		startTick = VSync(-1);
		while (CD_readState.active != 0)
		{
			if ((u32)(VSync(-1) - startTick) > 120u)
			{
				// NOTE(aalhendi): Materialize the struct base address so
				// GCC emits retail's 36(s0) store rather than a negative offset.
				struct PsxCdReadState *timeoutSt;

				CdSyncCallback(CD_readState.savedSyncCb);
				timeoutSt = &CD_readState;
				CTR_PSX_KEEP_VALUE(timeoutSt);
				timeoutSt->active = 0;
				break;
			}
		}
	}

	{
		// NOTE(aalhendi): Keep retail's base pointer in $a0 across the mode
		// switch and keep the default branch's base/constant in $v1/$v0.
		struct PsxCdReadState *modeSt CTR_PSX_GLOBAL_REGISTER_HINT("$4") = &CD_readState;

		CTR_PSX_KEEP_VALUE(modeSt);
		modeSt->mode = mode;
		switch (modeSt->mode & 0x30)
		{
		case 0:
			modeSt->sectorWords = 512;
			break;
		case 0x20:
			modeSt->sectorWords = 585;
			break;
		default:
		{
			s32 words CTR_PSX_LOCAL_REGISTER_HINT("$2") = 582;
			struct PsxCdReadState *defSt CTR_PSX_LOCAL_REGISTER_HINT("$3") = &CD_readState;

			CTR_PSX_KEEP_VALUE(defSt);
			defSt->sectorWords = words;
			break;
		}
		}
	}

	// NOTE(aalhendi): Keep CD_readState's base in $s0, place NULL in $a0
	// during the mode load delay slot, and keep sectors stored after buf.
	st = &CD_readState;
	CTR_PSX_KEEP_VALUE(st);
	{
		s32 nextMode = st->mode | 0x20;
		register CdlCB nullCb CTR_PSX_REGISTER("$4") = NULL;

		st->mode = nextMode;
		st->buf = buf;
		CTR_PSX_DEPEND_MEMORY(&st->sectors, buf);
		st->sectors = sectors;
		st->savedSyncCb = CdSyncCallback(nullCb);
	}
	st->savedReadyCb = CdReadyCallback(NULL);
	if ((CD_dataMode & 1) != 0)
	{
		st->savedDataCb = CdDataCallback(NULL);
	}
	st->startTick = VSync(-1);
	if ((CdStatus() & 0xe0) != 0)
	{
		// NOTE(aalhendi): Keep command 9 loaded into $a0 before zeroing $a1
		// and prevent CSE from aliasing $a2 to $a1 instead of $zero.
		register u32 cmd CTR_PSX_REGISTER("$4") = 9;
		register u8 *nullParam CTR_PSX_REGISTER("$5") = NULL;

		CTR_PSX_ORDER_VALUES(nullParam, cmd);
		CTR_PSX_FORGET_VALUE(nullParam);
		CdControlB(9, nullParam, NULL);
	}
	ok = CD_read_retry(0) > 0;
	// NOTE(aalhendi): Keep slt before epilogue register restores.
	CTR_PSX_MEMORY_BARRIER();
	return ok;
}

int CdReadSync(int mode, u8 *result)
{
	s32 left;
	// NOTE(aalhendi): Keep retail's startTick ($s1) and active ($s2 = $s1 + 8)
	// pointers across the polling loop.
	volatile s32 *startPtr CTR_PSX_GLOBAL_REGISTER_HINT("$17") = &CD_readState.startTick;
	volatile s32 *active CTR_PSX_GLOBAL_REGISTER_HINT("$18") = startPtr + 2;

	do
	{
		if (VSync(-1) > startPtr[0] + 1200)
		{
			left = -1;
		}
		else if ((startPtr[-2] < 0) || (VSync(-1) > startPtr[-1] + 60))
		{
			CD_read_retry(1);
			left = startPtr[-7];
		}
		else
		{
			left = startPtr[-2];
		}
	} while ((mode == 0) && (((*active != 0) && (left == 0)) || (left > 0)));

	CdReady(1, result);
	if ((*active != 0) && (left == 0))
	{
		left = 1;
	}
	return left;
}
