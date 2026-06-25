#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800ba2c0-0x800ba548 (ThTick, LInC, LInB).

void RB_Turtle_ThTick(struct Thread *t)
{
	struct Turtle *turtleObj;
	struct Instance *turtleInst;
	int currTimer;

	turtleObj = t->object;
	turtleInst = t->inst;

	// 0 from moment it hits bottom to moment it hits top
	if (turtleObj->direction == 0)
	{
		// use timer variables for milliseconds

		currTimer = turtleObj->timer;

		// if less than 1.0 seconds
		// wait for rise
		if (currTimer < 0x3c0)
		{
			// increment
			currTimer += sdata->gGT->elapsedTimeMS;

			// set
			turtleObj->timer = currTimer;

			if (currTimer > 0x5a0)
			{
				turtleObj->timer = 0x5a0;
			}

			if (turtleObj->timer == 0x5a0)
			{
				PlaySound3D(0x7d, turtleInst);
			}
		}

		// if more than one second has passed
		// time to rise
		else
		{
			// turtle not fully down,
			// impacts jumping
			turtleObj->state = 1;

			// use timer variables for frame counting

			// decrement frame (make turtle rise)
			currTimer = turtleInst->animFrame - 1;

			// end of animation
			if (currTimer < 1)
			{
				// reset direction
				turtleObj->direction = 1;

				// reset timer
				turtleObj->timer = 0;
			}

			// playing animation
			else
			{
				// decrement frame (make turtle rise)
				turtleInst->animFrame = currTimer;
			}
		}
	}

	// 1 from moment it hits top to moment it hits bottom
	else
	{
		// use timer variables for milliseconds

		currTimer = turtleObj->timer;

		// if less than 1.0 seconds
		// wait for fall
		if (currTimer < 0x3c0)
		{
			// increment
			currTimer += sdata->gGT->elapsedTimeMS;

			// set
			turtleObj->timer = currTimer;

			if (currTimer > 0x5a0)
			{
				turtleObj->timer = 0x5a0;
			}
		}

		// if more than one second has passed
		// time to fall
		else
		{
			// use timer variables for frame counting

			// increment frame (make turtle fall)
			currTimer = turtleInst->animFrame + 1;

			// playing animation
			if (currTimer < INSTANCE_GetNumAnimFrames(turtleInst, 0))
			{
				// increment frame (make turtle fall)
				turtleInst->animFrame = currTimer;
			}

			// finished animation
			else
			{
				// reset direction
				turtleObj->direction = 0;

				// reset timer
				turtleObj->timer = 0;

				// turtle is "fully" down
				turtleObj->state = 0;
			}
		}
	}
}

int RB_Turtle_LInC(struct Instance *inst, struct Thread *driverTh, struct ScratchpadStruct *sps) // unused 3rd param?
{
	int speed;
	int jumpType;
	struct Driver *driver;

	driver = driverTh->object;

	// absolute value
	speed = driver->speedApprox;
	if (speed < 0)
	{
		speed = -speed;
	}

	if (speed > 0x1400)
	{
		// small jump
		jumpType = FORCED_JUMP_LOW;

		if (
		    // turtleObj->state != FULLY_DOWN
		    ((struct Turtle *)inst->thread->object)->state != 0)
		{
			// big jump
			jumpType = FORCED_JUMP_HIGH;
		}

		// make the player jump
		driver->forcedJumpType = jumpType;

		return 2;
	}

	return 1;
}

void RB_Turtle_LInB(struct Instance *inst)
{
	int turtleID;
	struct Thread *t;
	struct Turtle *turtleObj;

	inst->flags |= SPLIT_LINE;

	if (inst->thread != 0)
	{
		return;
	}

	t = PROC_BirthWithObject(
	    // creation flags
	    SIZE_RELATIVE_POOL_BUCKET(sizeof(struct Turtle), NONE, SMALL, STATIC),

	    RB_Turtle_ThTick, // behavior
	    "turtle",         // debug name
	    0                 // thread relative
	);

	if (t == 0)
	{
		return;
	}
	inst->thread = t;
	t->inst = inst;

	inst->scale.x = 0x1000;
	inst->scale.y = 0x1000;
	inst->scale.z = 0x1000;

	turtleID = inst->name[strlen(inst->name) - 1] - '0';

	turtleObj = ((struct Turtle *)t->object);
	turtleObj->turtleID = turtleID;
	turtleObj->timer = 0;
	turtleObj->direction = 1;
	inst->animFrame = 0;

	// put turtles on different cycles, based on turtleID
	if ((turtleID & 1) == 0)
	{
		// fully up
		turtleObj->direction = 1;
		turtleObj->state = 1; // "not fully down"
		inst->animFrame = 0;
		return;
	}

	// fully down
	turtleObj->direction = 0;
	turtleObj->state = 0;
	inst->animFrame = INSTANCE_GetNumAnimFrames(inst, 0);
}
