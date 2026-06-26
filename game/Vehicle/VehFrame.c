#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8005b0c4-0x8005b0f4.
int VehFrameInst_GetStartFrame(int animIndex, int numFrames)
{
	switch (animIndex)
	{
	// midpoint
	case 0:
		return numFrames >> 1;

	// end
	case 4:
		return (numFrames - 1);

	// start
	default:
		return 0;
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8005b0f4-0x8005b178.
u32 VehFrameInst_GetNumAnimFrames(struct Instance *inst, int animIndex)
{
	if (inst->model == NULL)
	{
		return 0;
	}
	if (inst->model->numHeaders <= 0)
	{
		return 0;
	}
	if (inst->model->headers == NULL)
	{
		return 0;
	}

	struct ModelHeader *mh = inst->model->headers;

	if (animIndex >= mh->numAnimations)
	{
		return 0;
	}
	if (mh->ptrAnimations == NULL)
	{
		return 0;
	}

	struct ModelAnim *anim = mh->ptrAnimations[animIndex];

	if (anim == NULL)
	{
		return 0;
	}

	return anim->numFrames & 0x7fff;
}

static void VehFrameProc_Driving_SpawnBurnSmoke(struct Driver *d)
{
	struct Particle *p = Particle_Init(0, sdata->gGT->iconGroup[1], &data.emSet_BurnSmoke[0]);

	if (p != NULL)
	{
		p->otIndexOffset = d->instSelf->depthBiasNormal;
		p->driverInst = d->instSelf;
		p->driverID = d->driverID;
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8005b178-0x8005b510
void VehFrameProc_Driving(struct Thread *t, struct Driver *d)
{
	struct Instance *inst = t->inst;
	u8 desiredAnim = 0;
	int numFrames;

	if ((d->instTntRecv == NULL) && (d->kartState != KS_WARP_PAD))
	{
		if (d->fireSpeed < 0)
		{
			desiredAnim = d->speedApprox < 1;
		}

		if (((d->jumpHeightCurr > 0x600) || (inst->animIndex == 3)) && (d->posCurr.y - d->quadBlockHeight > 0x8000))
		{
			desiredAnim = 3;
		}
	}

	numFrames = VehFrameInst_GetNumAnimFrames(inst, inst->animIndex);
	if (numFrames <= 0)
	{
		return;
	}

	if (desiredAnim != inst->animIndex)
	{
		u8 currAnim = inst->animIndex;
		int startFrame;

		if (currAnim == 2)
		{
			startFrame = VehFrameInst_GetNumAnimFrames(inst, 2) - 1;
		}
		else
		{
			startFrame = VehFrameInst_GetStartFrame(currAnim, numFrames);
		}

		if (inst->animFrame == startFrame)
		{
			numFrames = VehFrameInst_GetNumAnimFrames(inst, desiredAnim);
			if (numFrames <= 0)
			{
				return;
			}

			inst->animIndex = desiredAnim;
			inst->animFrame = VehFrameInst_GetStartFrame(desiredAnim, numFrames);
			d->matrixArray = 0;
			d->matrixIndex = 0;
		}
		else
		{
			int speed = 2;

			if (currAnim == 0)
			{
				speed = 6;
			}
			else if (currAnim == 2)
			{
				speed = 1;
				d->matrixIndex = inst->animFrame;
			}

			inst->animFrame = VehCalc_InterpBySpeed(inst->animFrame, speed, startFrame);

			if ((u32)(inst->animIndex - 2) < 2)
			{
				d->matrixIndex = (u8)inst->animFrame;
				if (d->matrixIndex == 0)
				{
					d->matrixArray = 0;
					d->matrixIndex = 0;
				}
			}

			return;
		}
	}

	if (desiredAnim == 0)
	{
		int targetFrame = numFrames >> 1;

		if (d->instTntRecv == NULL)
		{
			s16 burnTimer = d->burnTimer;

			if ((burnTimer != 0) && (burnTimer < 0x1e0))
			{
				targetFrame += (((burnTimer >> 5) % 5) << 2) - 8;
				inst->animFrame = targetFrame;
				VehFrameProc_Driving_SpawnBurnSmoke(d);
			}
			else
			{
				int turnMin = -0x40;
				int turnMax = 0x40;
				int turnState = d->ampTurnState;

				if ((d->actionsFlagSet & ACTION_ACCEL_PREVENTION) == 0)
				{
					turnMax = (u8)d->const_TurnRate;
					turnMin = -turnMax;
					turnState = d->simpTurnState;
				}

				targetFrame = VehCalc_MapToRange(-turnState, turnMin, turnMax, 0, numFrames - 1);
			}
		}

		inst->animFrame = VehCalc_InterpBySpeed(inst->animFrame, 1, targetFrame);
		return;
	}

	if (desiredAnim == 3)
	{
		s16 characterID;
		u8 matrixArray;

		inst->animFrame = VehCalc_InterpBySpeed(inst->animFrame, 1, numFrames - 1);

		if (d->kartState == KS_MASK_GRABBED)
		{
			return;
		}

		characterID = data.characterIDs[d->driverID];
		if (characterID == PENTA_PENGUIN)
		{
			characterID = COCO_BANDICOOT;
		}
		if (characterID == FAKE_CRASH)
		{
			characterID = CRASH_BANDICOOT;
		}

		matrixArray = characterID + 7;
		if (characterID == NITROS_OXIDE)
		{
			matrixArray = 7;
		}

		d->matrixArray = matrixArray;
		d->matrixIndex = (u8)inst->animFrame;
		return;
	}

	inst->animFrame = VehCalc_InterpBySpeed(inst->animFrame, 1, numFrames - 1);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8005b510-0x8005b5fc.
void VehFrameProc_Spinning(struct Thread *t, struct Driver *d)
{
	struct Instance *inst = t->inst;
	int numFrames = VehFrameInst_GetNumAnimFrames(inst, inst->animIndex);
	int targetFrame;

	if (numFrames <= 0)
	{
		return;
	}

	if (inst->animIndex != 0)
	{
		targetFrame = VehFrameInst_GetStartFrame(inst->animIndex, numFrames);

		if ((u32)(inst->animIndex - 2) < 2)
		{
			d->matrixArray = 0;
			d->matrixIndex = 0;
		}

		if (inst->animFrame == targetFrame)
		{
			numFrames = VehFrameInst_GetNumAnimFrames(inst, 0);
			if (numFrames <= 0)
			{
				return;
			}

			inst->animIndex = 0;
		}

		if (inst->animIndex != 0)
		{
			inst->animFrame = VehCalc_InterpBySpeed(inst->animFrame, 4, targetFrame);
			return;
		}
	}

	targetFrame = 0;
	if (d->KartStates.Spinning.spinDir >= 0)
	{
		targetFrame = numFrames - 1;
	}

	inst->animFrame = VehCalc_InterpBySpeed(inst->animFrame, 4, targetFrame);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8005b5fc-0x8005b6b8.
void VehFrameProc_LastSpin(struct Thread *t, struct Driver *d)
{
	struct Instance *inst = t->inst;
	int targetFrame;
	int numFrames;

	if (inst->animIndex != 0)
	{
		VehFrameProc_Spinning(t, d);
		return;
	}

	numFrames = VehFrameInst_GetNumAnimFrames(inst, 0);
	if (numFrames <= 0)
	{
		return;
	}

	targetFrame = inst->animFrame;

	if (d->turnAngleCurr > 0)
	{
		if (d->turnAngleLerpVel < 0)
		{
			targetFrame = numFrames - 1;
		}
	}

	if ((d->turnAngleCurr < 0) && (d->turnAngleLerpVel > 0))
	{
		targetFrame = 0;
	}

	inst->animFrame = VehCalc_InterpBySpeed(inst->animFrame, 3, targetFrame);
}
