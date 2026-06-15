#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80068f90-0x80069178.
void VehTalkMask_ThTick(struct Thread *t)
{
	struct GameTracker *gGT = sdata->gGT;

	struct MaskHint *mhObj = t->object;
	struct Instance *mhInst = t->inst;

	int scale = 0x2000;

	if (sdata->modelMaskHints3D != 0)
	{
		mhInst->model = sdata->modelMaskHints3D;
	}
	else
	{
		scale = 0x1000;

		if (gGT->drivers[0] != 0)
		{
			int boolGoodGuy = VehPickupItem_MaskBoolGoodGuy(gGT->drivers[0]);

			// 0x3A for Uka, 0x39 for Aku
			int modelID = STATIC_UKAUKA - boolGoodGuy;
			mhInst->model = gGT->modelPtr[modelID];
		}
	}

	scale = (mhObj->scale * scale) >> 0xc;
	mhInst->scale[0] = scale;
	mhInst->scale[1] = scale;
	mhInst->scale[2] = scale;

	u32 lastFrame = VehFrameInst_GetNumAnimFrames(mhInst, 0) - 1;

	sdata->talkMaskXASamplePeak = sdata->XA_MaxSampleValInArr;

	int targetMouthFrame = sdata->talkMaskXASamplePeak * 7;

	if (targetMouthFrame < 0)
	{
		targetMouthFrame = targetMouthFrame + 0x3fff;
	}

	targetMouthFrame = targetMouthFrame >> 0xe;

	if (sdata->talkMaskMaxMouthFrame < targetMouthFrame)
		sdata->talkMaskMaxMouthFrame = targetMouthFrame;

	int desiredMouthFrame = targetMouthFrame;
	if (targetMouthFrame < 2)
	{
		desiredMouthFrame = 0;
	}

	int currentMouthFrame = mhInst->animFrame;

	if (targetMouthFrame > 3)
	{
		int mouthFrameDelta = currentMouthFrame - desiredMouthFrame;

		if (mouthFrameDelta < 0)
		{
			mouthFrameDelta = -mouthFrameDelta;
		}

		if (mouthFrameDelta > 3)
		{
			mhInst->animFrame = (s16)desiredMouthFrame;

			goto SkipLerp;
		}
	}

	mhInst->animFrame = EngineSound_VolumeAdjust(desiredMouthFrame, currentMouthFrame, 1);

SkipLerp:

	currentMouthFrame = mhInst->animFrame;

	// animFrame
	int mouthFrameDelta = currentMouthFrame - desiredMouthFrame;

	if (mouthFrameDelta < 0)
	{
		mouthFrameDelta = -mouthFrameDelta;
	}

	if (mouthFrameDelta < 6)
	{
		mhInst->animFrame = EngineSound_VolumeAdjust(desiredMouthFrame, currentMouthFrame, 1);
	}
	else
	{
		mhInst->animFrame = (s16)desiredMouthFrame;
	}

	// animation frame goes back and forth
	// 0x00: mouth close
	// 0x0C: mouth open

	if (mhInst->animFrame < 0)
		mhInst->animFrame = 0;

	else if (lastFrame < (u32)mhInst->animFrame)
		mhInst->animFrame = (s16)lastFrame;

	if (sdata->talkMask_boolDead != 0)
	{
		sdata->talkMask_boolDead = 0;

		// dead thread
		t->flags |= THREAD_FLAG_DEAD;
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80069178-0x800691e4.
struct Instance *VehTalkMask_Init()
{
	sdata->boolIsMaskThreadAlive = 1;
	sdata->talkMask_boolDead = 0;

	struct Instance *mhInst = INSTANCE_BirthWithThread(0x39, sdata->s_head, SMALL, AKUAKU, VehTalkMask_ThTick, 6, 0);

	struct Thread *mhTh = mhInst->thread;
	mhTh->funcThDestroy = PROC_DestroyInstance;

	((struct MaskHint *)mhTh->object)->scale = 0;

	return mhInst;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800691e4-0x8006924c.
void VehTalkMask_PlayXA(struct Instance *i, int id)
{
	struct Driver *d = sdata->gGT->drivers[0];

	if (d != 0)
	{
		int boolGoodGuy = VehPickupItem_MaskBoolGoodGuy(d);

		if (boolGoodGuy == 0)
			id += 0x1f;
	}

	CDSYS_XAPlay(CDSYS_XA_TYPE_EXTRA, id);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8006924c-0x8006925c.
int VehTalkMask_boolNoXA()
{
	return sdata->XA_State == 0;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8006925c-0x80069284.
void VehTalkMask_End()
{
	CDSYS_XAPauseRequest();

	sdata->boolIsMaskThreadAlive = 0;
	sdata->talkMask_boolDead = 1;
}
