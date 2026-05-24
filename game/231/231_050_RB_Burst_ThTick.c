#include <common.h>

static void RB_Burst_UpdateSlot(int *slot)
{
	struct Instance *inst;
	int nextFrame;

	inst = (struct Instance *)*slot;
	if (inst == NULL)
		return;

	nextFrame = inst->animFrame + 1;
	if (nextFrame < DECOMP_INSTANCE_GetNumAnimFrames(inst, 0))
	{
		inst->animFrame++;
		return;
	}

	DECOMP_INSTANCE_Death(inst);
	*slot = 0;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b1d2c-0x800b1e90.
void DECOMP_RB_Burst_ThTick(struct Thread *t)
{
	int *burst;
	burst = t->object;

	RB_Burst_UpdateSlot(&burst[1]);
	RB_Burst_UpdateSlot(&burst[2]);
	RB_Burst_UpdateSlot(&burst[0]);

	if ((burst[1] == 0) && (burst[2] == 0))
		t->flags |= 0x800;
}
