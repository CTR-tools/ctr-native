#include <common.h>

// In air, after spamming L1 or R1,
// will explode on impact with ground
// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800ad310-0x800ad44c.
void RB_TNT_ThTick_ThrowOffHead(struct Thread *t)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Instance *inst;
	struct MineWeapon *mw;

	inst = t->inst;

	//  object (tnt)
	mw = t->object;

	// do NOT use parenthesis
	inst->matrix.t[1] += (mw->velocity.y * gGT->elapsedTimeMS) >> 5;

#if defined(CTR_NATIVE)
	// NOTE(aalhendi): Retail reads through driverTarget blindly here. Boss-thrown TNT can have
	// no target, and native cannot mirror PS1 low-memory null reads.
	if ((mw->stopFallAtY == 0x3fff) && (mw->driverTarget != NULL))
	{
#else
	if (mw->stopFallAtY == 0x3fff)
#endif
		mw->stopFallAtY = mw->driverTarget->instSelf->matrix.t[1];
	}

	if (inst->matrix.t[1] <= mw->stopFallAtY)
	{
		// plays tnt explosion sound 3D
		PlaySound3D(0x3d, inst);

		RB_Blowup_Init(inst);

		// Set scale (x, y, z) to zero
		inst->scale.x = 0;
		inst->scale.y = 0;
		inst->scale.z = 0;

		// make invisible
		inst->flags |= HIDE_MODEL;

		// this thread is now dead
		t->flags |= THREAD_FLAG_DEAD;

#if defined(CTR_NATIVE)
		// NOTE(aalhendi) Retail writes through driverTarget blindly; boss-thrown TNT has no
		// driver-owned instTntRecv slot to clear.
		if (mw->driverTarget != NULL)
		{
#endif
			mw->driverTarget->instTntRecv = 0;
		}
	}

	// decrease velocity (artificial gravity)
	mw->velocity.y -= ((gGT->elapsedTimeMS << 2) >> 5);
	if (mw->velocity.y < -0x60)
	{
		mw->velocity.y = -0x60;
	}
}

static const s16 s_tntSitScale[(0x5a + 1) * 2] = {
    2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2355, 2148,
    2539, 2248, 2462, 2293, 2280, 2273, 2061, 2220, 1878, 2148, 1802, 2070, 1815, 1998, 1850, 1945, 1898, 1925, 1951, 1934, 1999, 1957, 2034, 1986, 2048, 2038,
    2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2560, 2214,
    2867, 2381, 2739, 2457, 2434, 2423, 2070, 2335, 1766, 2215, 1638, 2085, 1649, 1965, 1679, 1877, 1728, 1843, 1791, 1857, 1867, 1899, 1954, 1964, 2048, 2048,
    2150, 2150, 2048, 2048, 1945, 1945, 2048, 2048, 2150, 2150, 2048, 2048, 1945, 1945, 2048, 2048, 2150, 2150, 2123, 2123, 2074, 2074, 2048, 2048, 2728, 2270,
    3137, 2491, 2967, 2592, 2562, 2532, 2078, 2381, 1673, 2184, 1503, 1987, 1620, 1836, 1878, 1775, 2136, 2014, 2253, 2253, 2052, 2052, 1851, 1851, 2067, 2067,
    2283, 2283, 1943, 1943, 2331, 2331, 1926, 1926, 2344, 2344, 1899, 1899, 2379, 2379, 1904, 1904, 2327, 2327, 1926, 1926, 2379, 2379, 3072, 3072, 32,   32,
};

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800ad44c-0x800ad710.
// NOTE(aalhendi): Retail frame 89 indexes one pair into the next RDATA table at 0x800b2ac4.
void RB_TNT_ThTick_SitOnHead(struct Thread *t)
{
	struct Instance *inst;
	struct MineWeapon *mw;
	char state;
	s16 numFrames;
	u16 uVar3;
	int rng;

	inst = t->inst;

	// object (tnt)
	mw = t->object;

	// CopyMatrix
	// To: TNT instance
	// From: obj->driverWhoHitMe->instance
	// Delta: TNT -> 0x1c (position relative to driver)
	LHMatrix_Parent(inst, mw->driverTarget->instSelf, (SVECTOR *)&mw->deltaPos.x);

	// Get Kart State
	state = mw->driverTarget->kartState;

	if ((state == KS_CRASHING) || (state == KS_MASK_GRABBED) || (state == KS_SPINNING))
	{
		// Play explosion sound
		PlaySound3D(0x3d, inst);

		RB_Blowup_Init(inst);

	LAB_800ad4ec:

		// reset TNT-related pointers
		inst->scale.x = 0;
		inst->scale.y = 0;
		inst->scale.z = 0;

		// make invisible
		inst->flags |= HIDE_MODEL;

		// this thread is now dead
		t->flags |= THREAD_FLAG_DEAD;

		mw->driverTarget->instTntRecv = 0;

		return;
	}
	else
	{
		// If you are already blasted
		if (state == 6)
		{
			// Play explosion sound
			PlaySound3D(0x3d, inst);

			RB_Explosion_InitGeneric(inst);

			goto LAB_800ad4ec;
		}
	}

	// if this driver is not an AI
	if ((mw->driverTarget->actionsFlagSet & ACTION_BOT) == 0)
	{
		// if player did not start jumping this frame
		if ((mw->driverTarget->actionsFlagSet & ACTION_JUMP_STARTED) == 0)
		{
			goto LAB_800ad5f8;
		}

		if (mw->jumpsRemaining != 0)
		{
			mw->jumpsRemaining += -1;
			goto LAB_800ad5f8;
		}
		mw->jumpsRemaining = 0;
	}
	else
	{
		rng = MixRNG_Scramble();
		if (rng != (rng / 0x10e) * 0x10e)
		{
			goto LAB_800ad5f8;
		}
	}

	// set scale (x, y, z)
	inst->scale.x = 0x800;
	inst->scale.y = 0x800;
	inst->scale.z = 0x800;

	mw->driverTarget->instTntRecv = 0;

	mw->velocity.x = 0;
	mw->velocity.y = 0x30;
	mw->velocity.z = 0;
	mw->deltaPos.x = 0;
	mw->deltaPos.y = 0;
	mw->deltaPos.z = 0;

	// assign RB_TNT_ThTick_ThrowOffHead
	ThTick_SetAndExec(t, RB_TNT_ThTick_ThrowOffHead);
	return;

LAB_800ad5f8:

	// Get how many frames the TNT has
	// been on top of someone's head
	numFrames = mw->numFramesOnHead;

	// If there is time remaining until TNT blows up,
	// which takes 0x5a frames, 3 seconds
	if (numFrames < 0x5a)
	{
		// If frame is any of these 6 numbers
		if ((numFrames == 0x0) || (numFrames == 0x14) || (numFrames == 0x28) || (numFrames == 0x3c) || (numFrames == 0x46) || (numFrames == 0x50))
		{
			// Make a "honk" sound
			PlaySound3D(0x3e, inst);
		}

		// add to the frame counter
		mw->numFramesOnHead += 1;
		numFrames = mw->numFramesOnHead;
	}

	// If time runs out
	else
	{
		// Blow up

		RB_Hazard_HurtDriver(mw->driverTarget, 2, mw->instParent->thread->object, 0);

		// icon damage timer, draw icon as red
		mw->driverTarget->damageColorTimer = 0x1e;

		// play 3D sound for TNT explosion
		PlaySound3D(0x3d, inst);

		RB_Blowup_Init(inst);

		// this thread is now dead
		t->flags |= THREAD_FLAG_DEAD;

		mw->driverTarget->instTntRecv = NULL;
	}

	// set scale of TNT, given frame of animation
	uVar3 = s_tntSitScale[numFrames * 2 + 0];
	inst->scale.x = uVar3;
	inst->scale.z = uVar3;
	inst->scale.y = s_tntSitScale[numFrames * 2 + 1];
}

static const s16 s_tntThrowHeadY[0x10] = {32, 32, 64, 32, 32, 48, 32, 32, 48, 64, 32, 48, 56, 24, 32, 56};

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800ad710-0x800ad92c.
// NOTE(aalhendi): Native uses retail character-height table bytes from 0x800b2ac4.
void RB_TNT_ThTick_ThrowOnHead(struct Thread *t)
{
	struct MineWeapon *mw;
	struct Instance *inst;
	s16 *array;
	struct GameTracker *gGT;

	SVec3 rot;
	s16 distHead;

	// matrix?
	s16 auStack48[32];

	gGT = sdata->gGT;

	inst = t->inst;

	// object
	mw = inst->thread->object;

	// alter height of TNT as it flies onto a driver's head,
	// do NOT use parenthesis
	mw->deltaPos.y += (mw->velocity.y * gGT->elapsedTimeMS) >> 5;

	// if TNT is moving downward
	if (mw->velocity.y < 0)
	{
		array = (s16 *)s_tntThrowHeadY;

		distHead = array[data.characterIDs[mw->driverTarget->driverID]];

		// if TNT landed on head
		if ((mw->deltaPos.y < distHead) && (mw->deltaPos.y = distHead, inst->scale.x == 0x800))
		{
			// Set TNT timer to 0, it blows up at 0x5a
			mw->numFramesOnHead = 0;

			// number of jumps is 7 or 8
			mw->jumpsRemaining = 8 - (MixRNG_Scramble() & 1);

			// play sound that you hit a TNT
			PlaySound3D(0x51, inst);

			inst->depthBiasNormal = mw->driverTarget->instSelf->depthBiasNormal + -1;
			inst->depthBiasSecondary = mw->driverTarget->instSelf->depthBiasSecondary + -1;

			// assign
			ThTick_SetAndExec(t, RB_TNT_ThTick_SitOnHead);
			return;
		}
	}

	// CopyMatrix
	LHMatrix_Parent(inst, mw->driverTarget->instSelf, (SVECTOR *)&mw->deltaPos.x);

	// rotation
	rot.x = 0;
	rot.y = mw->tntSpinY;
	rot.z = 0;

	// convert 3 rotation shorts into rotation matrix
	ConvertRotToMatrix((MATRIX *)auStack48, &rot);

	MatrixRotate(&inst->matrix, &inst->matrix, (MATRIX *)auStack48);

	// reduce time remaining until TNT lands on head
	mw->velocity.y -= ((gGT->elapsedTimeMS << 2) >> 5);

	// set a minimum value (-0x60)
	if (mw->velocity.y < -0x60)
	{
		mw->velocity.y = -0x60;
	}

	// rotation
	mw->tntSpinY += 0x100;

	// if scale is small
	if (inst->scale.x < 0x801)
	{
		// set min scale
		inst->scale.x = 0x800;
		inst->scale.y = 0x800;
		inst->scale.z = 0x800;
	}

	// if scale is large
	else
	{
		// reduce scale
		inst->scale.x -= 0x100;
		inst->scale.y -= 0x100;
		inst->scale.z -= 0x100;
	}
	return;
}
