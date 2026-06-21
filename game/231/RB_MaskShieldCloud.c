#include <common.h>

// NOTE(aalhendi): ASM-verified against NTSC-U 926 overlay 231 0x800afb70-0x800afdbc.
void RB_MaskWeapon_FadeAway(struct Thread *t)
{
	s16 totalTime;
	MATRIX *m;
	struct Instance *inst;
	struct Instance *driverInst;
	struct Instance *maskBeamInst;
	struct MaskHeadWeapon *mask;

	inst = t->inst;
	mask = inst->thread->object;
	driverInst = t->parentThread->inst;
	maskBeamInst = mask->maskBeamInst;

	struct MaskHeadScratch *mhs = CTR_SCRATCHPAD_PTR(struct MaskHeadScratch, 0x108);

	// Set up First pass (maskInst)

	int durationAdjusted = ((int)(mask->duration >> 5) * -4 + 0x40);
	mhs->posOffset.x = ((durationAdjusted * MATH_Sin(mask->rot.y)) >> 0xc);
	mhs->posOffset.z = ((durationAdjusted * MATH_Cos(mask->rot.y)) >> 0xc);
	mhs->posOffset.y = 0x40;

	mask->rot.y += -0x100;

	struct Instance *instCurr;
	instCurr = inst;

	// First time is MaskInst,
	// Second time is BeamInst
	for (int i = 0; i < 2; i++)
	{
		LHMatrix_Parent(instCurr, driverInst, (SVECTOR *)&mhs->posOffset);

		instCurr->scale.x += -0x100;
		instCurr->scale.y += -0x100;
		instCurr->scale.z += -0x100;

		// position offset
		mhs->posOffset.x = 0;
		mhs->posOffset.z = 0;

		instCurr = maskBeamInst;
	}

	mhs->rot.x = 0;
	mhs->rot.y = mask->rot.y;
	mhs->rot.z = 0;
	ConvertRotToMatrix(&mhs->m, &mhs->rot);

	m = &maskBeamInst->matrix;
	MatrixRotate(m, m, &mhs->m);

	if (maskBeamInst->alphaScale < 0x1000)
		maskBeamInst->alphaScale += 0x200;

	totalTime = mask->duration;

	if (totalTime < 0x200)
	{
		totalTime += sdata->gGT->elapsedTimeMS;

		if (totalTime > 0x200)
			totalTime = 0x200;

		mask->duration = totalTime;
		return;
	}

	// mask is now dead
	INSTANCE_Death(maskBeamInst);
	t->flags |= THREAD_FLAG_DEAD;
	return;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800afdbc-0x800b0278.
void RB_MaskWeapon_ThTick(struct Thread *maskTh)
{
	char i;
	char numPlyr;
	s16 sVar1;
	struct GameTracker *gGT;
	struct PushBuffer *pb;
	int rot;
	struct MaskHeadWeapon *mask;
	struct Instance *maskInst;
	struct Instance *maskBeamInst;
	struct Instance *driverInst;
	struct Driver *d;

	struct Instance *instCurr;

	gGT = sdata->gGT;
	numPlyr = gGT->numPlyrCurrGame;

	mask = maskTh->object;
	maskInst = maskTh->inst;
	maskBeamInst = mask->maskBeamInst;

	d = maskTh->parentThread->object;
	driverInst = maskTh->parentThread->inst;

	struct InstDrawPerPlayer *maskIdpp = INST_GETIDPP(maskInst);
	struct InstDrawPerPlayer *beamIdpp = INST_GETIDPP(maskBeamInst);

	if (d->invisibleTimer == 0)
	{
		for (i = 0; i < numPlyr; i++)
		{
			pb = &gGT->pushBuffer[i];
			maskIdpp[i].pushBuffer = pb;
			beamIdpp[i].pushBuffer = pb;
		}
	}

	else
	{
		for (i = 0; i < numPlyr; i++)
		{
			if (i == d->driverID)
				continue;

			maskIdpp[i].pushBuffer = NULL;
			beamIdpp[i].pushBuffer = NULL;
		}
	}

	if ((driverInst->flags & REFLECTIVE) == 0)
	{
		maskInst->flags &= ~REFLECTIVE;
	}
	else
	{
		maskInst->flags |= REFLECTIVE;

		maskInst->vertSplit = driverInst->vertSplit;
		maskBeamInst->flags |= REFLECTIVE;
		maskBeamInst->vertSplit = driverInst->vertSplit;
	}

	maskInst->depthBiasNormal = driverInst->depthBiasNormal;
	maskInst->depthBiasSecondary = driverInst->depthBiasSecondary;

	struct MaskHeadScratch *mhs = CTR_SCRATCHPAD_PTR(struct MaskHeadScratch, 0x108);

	// Set up the First pass (MaskInst)

	rot = mask->rot.y;

	mhs->posOffset.x = (((MATH_Sin(rot) << 6) >> 0xc) * mask->scale) >> 0xc;
	mhs->posOffset.z = (((MATH_Cos(rot) << 6) >> 0xc) * mask->scale) >> 0xc;

	mhs->posOffset.y = R231.maskPosArr[(int)maskBeamInst->animFrame >> 0] + 0x40;

	mhs->rot.x = 0;
	mhs->rot.y = rot;
	mhs->rot.z = 0;

	instCurr = maskInst;

	// First time is MaskInst,
	// Second time is BeamInst
	for (int i = 0; i < 2; i++)
	{
		if ((mask->rot.z & 1) == 0)
		{
			LHMatrix_Parent(instCurr, driverInst, (SVECTOR *)&mhs->posOffset);
			ConvertRotToMatrix(&mhs->m, &mhs->rot);
			MatrixRotate(&instCurr->matrix, &instCurr->matrix, &mhs->m);
		}
		else
		{
			instCurr->matrix.t[0] = (int)mask->pos.x + mhs->posOffset.x;
			instCurr->matrix.t[1] = (int)mask->pos.y + mhs->posOffset.y;
			instCurr->matrix.t[2] = (int)mask->pos.z + mhs->posOffset.z;
			ConvertRotToMatrix(&instCurr->matrix, &mhs->rot);
		}

		// Set up the Second pass (BeamInst)

		mhs->posOffset.x = 0;
		mhs->posOffset.y = 0x40;
		mhs->posOffset.z = 0;

		instCurr = maskBeamInst;
	}

	// === Animation ===

	// get animFrame
	sVar1 = INSTANCE_GetNumAnimFrames(maskBeamInst, 0);

	// if animation is not finished
	if ((int)maskBeamInst->animFrame < sVar1 - 1)
	{
		// increment animation frame
		maskBeamInst->animFrame += 1;
	}
	// if animation is finished
	else
	{
		// restart animation
		maskBeamInst->animFrame = 0;
	}

	// adjust rotation
	mask->rot.y += -0x100;

	// If duration is over
	if (mask->duration == 0)
	{
		ThTick_SetAndExec(maskTh, RB_MaskWeapon_FadeAway);
	}
	else
	{
		// if duration is not over
		mask->duration -= gGT->elapsedTimeMS;

		if (mask->duration < 0)
			mask->duration = 0;
	}

	// first pass
	instCurr = maskBeamInst;
	instCurr->alphaScale = 0;

	for (int i = 0; i < 2; i++)
	{
		instCurr->flags &= ~HIDE_MODEL;

		instCurr->scale.x = mask->scale;
		instCurr->scale.y = mask->scale;
		instCurr->scale.z = mask->scale;

		// second pass
		instCurr = maskInst;
	}
}

static const s16 s_shieldPopScale[11][2] = {
    {2150, 1612}, {2419, 1433}, {2508, 1344}, {2329, 1523}, {1792, 1792}, {1254, 2150}, {896, 2419}, {716, 2508}, {537, 2150}, {358, 1254}, {179, 537},
};

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b0278-0x800b0454.
// NOTE(aalhendi): Native uses the extracted shield-pop scale table from RDATA 0x800b2d14.
void RB_ShieldDark_ThTick_Pop(struct Thread *t)
{
	struct Shield *sh;
	struct Instance *instDark;
	struct Instance *instColor;
	struct Driver *driverOwner;
	SVec3 rot;

	sh = t->object;
	instDark = t->inst;
	instColor = sh->instColor;
	driverOwner = t->parentThread->object;

	rot.x = 0;
	rot.y = 0;
	rot.z = 0;
	LHMatrix_Parent(instDark, driverOwner->instSelf, (SVECTOR *)rot.v);
	LHMatrix_Parent(instColor, driverOwner->instSelf, (SVECTOR *)rot.v);

	// set rotation
	*(int *)&instDark->matrix.m[0][0] = 0x1000;
	*(int *)&instDark->matrix.m[0][2] = 0;
	*(int *)&instDark->matrix.m[1][1] = 0x1000;
	*(int *)&instDark->matrix.m[2][0] = 0;
	instDark->matrix.m[2][2] = 0x1000;

	// set rotation
	*(int *)&instColor->matrix.m[0][0] = 0x1000;
	*(int *)&instColor->matrix.m[0][2] = 0;
	*(int *)&instColor->matrix.m[1][1] = 0x1000;
	*(int *)&instColor->matrix.m[2][0] = 0;
	instColor->matrix.m[2][2] = 0x1000;

	int animFrame = sh->animFrame;

	if (animFrame < 0xb)
	{
		// set scale
		instDark->scale.x = s_shieldPopScale[animFrame][0];
		instDark->scale.y = s_shieldPopScale[animFrame][1];
		instDark->scale.z = s_shieldPopScale[animFrame][0];

		// set scale
		instColor->scale.x = s_shieldPopScale[animFrame][0];
		instColor->scale.y = s_shieldPopScale[animFrame][1];
		instColor->scale.z = s_shieldPopScale[animFrame][0];

		// next frame
		sh->animFrame += 1;

		return;
	}

	// === Animation Done ===

	// play 3D sound for "shield pop"
	PlaySound3D(0x58, instDark);

	INSTANCE_Death(instColor);
	INSTANCE_Death(sh->instHighlight);

	// this thread is now dead
	t->flags |= THREAD_FLAG_DEAD;

	return;
}

static const s16 s_shieldGrowScale[8][2] = {
    {977, 1835}, {1792, 2936}, {2205, 2095}, {2335, 1254}, {1884, 1612}, {1433, 1971}, {1612, 1881}, {1792, 1792},
};

static const s16 s_shieldPulseScale[6][2] = {
    {1845, 1756}, {1899, 1720}, {1971, 1684}, {1899, 1720}, {1845, 1756}, {1792, 1792},
};

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b0454-0x800b0dbc.
// NOTE(aalhendi): Native uses extracted shield scale tables from RDATA 0x800b2cf4 and 0x800b2d40.
void RB_ShieldDark_ThTick_Grow(struct Thread *th)
{
	u16 shieldFlags;
	s16 sVar4;
	int i;
	int rotY;
	struct TrackerWeapon *tw;
	struct PushBuffer *pb;

	struct GameTracker *gGT = sdata->gGT;
	struct Instance *shieldInst = th->inst;
	struct Shield *shield = th->object;
	struct Instance *colorInst = shield->instColor;
	struct Instance *highlightInst = shield->instHighlight;

	struct Thread *playerTh = th->parentThread;
	struct Driver *player = playerTh->object;
	struct Instance *driverInst = playerTh->inst;

	// if highlight cooldown is gone
	if (shield->highlightTimer == 0)
	{
		shield->highlightRot.y += 0x100;

		highlightInst->flags &= ~HIDE_MODEL;

		rotY = shield->highlightRot.y;

		int iVar8 = rotY;
		if (rotY < 0)
			iVar8 = rotY + 0xfff;

		// if highlight is finished
		if ((rotY + (iVar8 >> 12) * -0x1000) == 0x400)
		{
			// cooldown is 30 frames (one second)
			shield->highlightTimer = 30;

			shield->highlightRot.y = 0xc00;

			highlightInst->flags |= HIDE_MODEL;
		}
	}

	// if highlight cooldown is not done
	else
	{
		// decrease counter, make invisible when this is zero
		shield->highlightTimer--;

		highlightInst->flags |= HIDE_MODEL;

		// if timer runs out (last frame)
		if (shield->highlightTimer == 0)
		{
			highlightInst->flags &= ~HIDE_MODEL;
		}
	}

	struct InstDrawPerPlayer *idpp = INST_GETIDPP(shieldInst);
	struct InstDrawPerPlayer *colorIdpp = INST_GETIDPP(colorInst);
	struct InstDrawPerPlayer *highlightIdpp = INST_GETIDPP(highlightInst);

	// If the driver that used this weapon is visible
	if (player->invisibleTimer == 0)
	{
		for (i = 0; i < gGT->numPlyrCurrGame; i++)
		{
			pb = &gGT->pushBuffer[i];
			idpp[i].pushBuffer = pb;
			colorIdpp[i].pushBuffer = pb;
			highlightIdpp[i].pushBuffer = pb;
		}
	}

	// if driver is not invisible
	else
	{
		for (i = 0; i < gGT->numPlyrCurrGame; i++)
		{
			if (i == player->driverID)
				continue;

			idpp[i].pushBuffer = 0;
			colorIdpp[i].pushBuffer = 0;
			highlightIdpp[i].pushBuffer = 0;
		}
	}

	SVec3 pos;
	pos.x = 0;
	pos.y = 0;
	pos.z = 0;

	// Copy matrix
	// To: shield instance, highlight instance, etc
	// From: thread (shield) -> parentthread (player) -> object (driver) -> instance
	LHMatrix_Parent(shieldInst, driverInst, (SVECTOR *)pos.v);
	LHMatrix_Parent(colorInst, driverInst, (SVECTOR *)pos.v);
	LHMatrix_Parent(highlightInst, driverInst, (SVECTOR *)pos.v);

	// set rotation variables
	*(int *)&shieldInst->matrix.m[0][0] = 0x1000;
	*(int *)&shieldInst->matrix.m[0][2] = 0;
	*(int *)&shieldInst->matrix.m[1][1] = 0x1000;
	*(int *)&shieldInst->matrix.m[2][0] = 0;
	shieldInst->matrix.m[2][2] = 0x1000;

	// set rotation variables
	*(int *)&colorInst->matrix.m[0][0] = 0x1000;
	*(int *)&colorInst->matrix.m[0][2] = 0;
	*(int *)&colorInst->matrix.m[1][1] = 0x1000;
	*(int *)&colorInst->matrix.m[2][0] = 0;
	colorInst->matrix.m[2][2] = 0x1000;

	// convert 3 rotation shorts into rotation matrix
	ConvertRotToMatrix(&highlightInst->matrix, &shield->highlightRot);

	s16 scaleXZ;
	s16 scaleY;

	// if animation is not done
	if (shield->animFrame < 8)
	{
		scaleXZ = s_shieldGrowScale[shield->animFrame][0];
		scaleY = s_shieldGrowScale[shield->animFrame][1];

		// set scale
		shieldInst->scale.x = scaleXZ;
		shieldInst->scale.y = scaleY;
		shieldInst->scale.z = scaleXZ;

		// set scale
		colorInst->scale.x = scaleXZ;
		colorInst->scale.y = scaleY;
		colorInst->scale.z = scaleXZ;

		// next frame
		shield->animFrame++;
	}

	// if animation is done
	else
	{
		s16 timerIndex = ((gGT->timer >> 0) % 6);

		scaleXZ = s_shieldPulseScale[timerIndex][0];
		scaleY = s_shieldPulseScale[timerIndex][1];

		// set scale
		shieldInst->scale.x = scaleXZ;
		shieldInst->scale.y = scaleY;
		shieldInst->scale.z = scaleXZ;

		// set scale
		colorInst->scale.x = scaleXZ;
		colorInst->scale.y = scaleY;
		colorInst->scale.z = scaleXZ;

		// set scale
		highlightInst->scale.x = scaleXZ;
		highlightInst->scale.y = scaleY;
		highlightInst->scale.z = scaleXZ;
	}

	// if this is not a blue shield,
	// meaning it must fade eventually
	if ((shield->flags & 4) == 0)
	{
		// duration
		s16 duration = shield->duration;

		// if out of time
		if (duration == 0)
		{
			// erase bubble instance from driver
			player->instBubbleHold = NULL;

			goto LAB_800b0d6c;
		}

		// subtract 32ms by hand
		duration -= 32;
		shield->duration = duration;

		// 2.0 seconds
		if (duration < 1920)
		{
			sVar4 = (s16)(((60 - (duration >> 5)) * 3072) / 60) + 0x400;

			// transparency
			shieldInst->alphaScale = sVar4;
			colorInst->alphaScale = sVar4;
			highlightInst->alphaScale = sVar4;
		}
	}

	shieldFlags = shield->flags;

	if (((shieldFlags & 1) != 0) || ((shieldFlags & 8) != 0) ||

	    // if race ended for this driver
	    ((player->actionsFlagSet & ACTION_RACE_FINISHED) != 0) ||

	    // if driver is being mask grabbed
	    (player->kartState == KS_MASK_GRABBED))
	{
		if ((shieldFlags & 8) != 0)
		{
			pb = &gGT->pushBuffer[player->driverID];

			pb->fadeFromBlack_currentValue = 0x1fff;
			pb->fadeFromBlack_desiredResult = 0x1000;
			pb->fade_step = -(0x88);
		}

		shield->animFrame = 0;
		player->instBubbleHold = NULL;

		// execute, then assign per-frame funcPtr to thread
		ThTick_SetAndExec(th, RB_ShieldDark_ThTick_Pop);
		return;
	}

	if ((shieldFlags & 2) == 0)
		return;

	player->instBubbleHold = NULL;
	player->numTimesMissileLaunched++;

	GAMEPAD_ShockFreq(player, 8, 0);
	GAMEPAD_ShockForce1(player, 8, 0x7f);

	// green shield
	u8 model = 0x5e;

	if ((shieldFlags & 4) != 0)
	{
		// blue shield
		model = 0x56;
	}

	// create a thread, get an instance
	struct Instance *bombInst = INSTANCE_BirthWithThread(model, 0, MEDIUM, OTHER, RB_MovingExplosive_ThTick, sizeof(struct TrackerWeapon), playerTh);

	struct Thread *bombTh = bombInst->thread;
	bombTh->funcThDestroy = PROC_DestroyInstance;

	// if driver is not an AI (human)
	if ((player->actionsFlagSet & ACTION_BOT) == 0)
	{
		// make driver talk
		Voiceline_RequestPlay(13, data.characterIDs[player->driverID], 0x10);
	}

	// copy position and rotation from one instance to another
	*(int *)&bombInst->matrix.m[0][0] = *(int *)&shieldInst->matrix.m[0][0];
	*(int *)&bombInst->matrix.m[0][2] = *(int *)&shieldInst->matrix.m[0][2];
	*(int *)&bombInst->matrix.m[1][1] = *(int *)&shieldInst->matrix.m[1][1];
	*(int *)&bombInst->matrix.m[2][0] = *(int *)&shieldInst->matrix.m[2][0];
	bombInst->matrix.m[2][2] = shieldInst->matrix.m[2][2];
	bombInst->matrix.t[0] = shieldInst->matrix.t[0];
	bombInst->matrix.t[1] = shieldInst->matrix.t[1];
	bombInst->matrix.t[2] = shieldInst->matrix.t[2];

	// set scale (x, y, z) and transparency
	bombInst->scale.x = 0x400;
	bombInst->scale.y = 0x400;
	bombInst->scale.z = 0x400;
	bombInst->alphaScale = 0x400;

	// get object from thread
	tw = bombTh->object;

	tw->flags = 0;
	tw->driverTarget = 0;
	tw->timeAlive = 0;
	tw->audioPtr = 0;
	tw->frameCount_Blind = 0;

	tw->driverParent = player;
	tw->instParent = driverInst;

	// do NOT patch for 60fps,
	// velocity uses elapsedTime
	tw->vel.y = 0;
	tw->vel.x = (driverInst->matrix.m[0][2] * 3) >> 7;
	tw->vel.z = (driverInst->matrix.m[2][2] * 3) >> 7;

	tw->rotY = player->angle;
	tw->frameCount_DontHurtParent = 10;

LAB_800b0d6c:

	// green shield fade away sound
	PlaySound3D(0x58, shieldInst);

	// shield and highlight
	INSTANCE_Death(colorInst);
	INSTANCE_Death(highlightInst);

	// This thread is now dead
	th->flags |= THREAD_FLAG_DEAD;
}

// NOTE(aalhendi): ASM-verified against NTSC-U 926 overlay 231 0x800b0f1c-0x800b1000.
void RB_RainCloud_FadeAway(struct Thread *t)
{
	struct Instance *inst;
	struct Instance *parentInst;
	struct RainCloud *rcloud;

	inst = t->inst;
	rcloud = t->object;
	parentInst = t->parentThread->inst;

	// offset upward before averaging
	inst->matrix.t[1] += 0x80;

	// X, Y, Z
	for (int i = 0; i < 3; i++)
	{
		// get average between instance and driver
		inst->matrix.t[i] += parentInst->matrix.t[i];
		inst->matrix.t[i] = inst->matrix.t[i] >> 1;
	}

	struct RainLocal *rainLocal = rcloud->rainLocal;
	rainLocal->frameCount -= 2;

	inst->scale.z += -0x100;
	inst->scale.y += -0x100;
	inst->scale.x += -0x100;

	if (inst->scale.x < 0)
	{
		JitPool_Remove(&sdata->gGT->JitPools.rain, (struct Item *)rainLocal);

		// This thread is now dead
		t->flags |= THREAD_FLAG_DEAD;
	}
	return;
}

// NOTE(aalhendi): ASM-verified against NTSC-U 926 overlay 231 0x800b1000-0x800b1220.
void RB_RainCloud_ThTick(struct Thread *t)
{
	s16 animFrame;
	int numFrames;
	int reduce;
	int rng;
	struct Instance *inst;
	struct Driver *d;
	struct RainCloud *rcloud;
	struct Instance *dInst;

	struct GameTracker *gGT = sdata->gGT;

	inst = t->inst;
	rcloud = t->object;

	// get player who put the potion
	struct Thread *driverTh = t->parentThread;

	d = driverTh->object;
	dInst = driverTh->inst;

	animFrame = inst->animFrame;
	numFrames = INSTANCE_GetNumAnimFrames(inst, 0);

	// if you have not reached the end of the animation
	if ((int)animFrame < numFrames - 1)
	{
		// increment animation frame
		inst->animFrame++;
	}

	// if animation is done
	else
	{
		// restart animation
		inst->animFrame = 0;
	}

	// X, Y, Z
	for (int i = 0; i < 3; i++)
	{
		// get average between instance and driver
		inst->scale.v[i] += dInst->scale.v[i];
		inst->scale.v[i] = inst->scale.v[i] >> 1;
	}

	// offset upward before averaging
	inst->matrix.t[1] += (inst->scale.y * 5 >> 7);

	// X, Y, Z
	for (int i = 0; i < 3; i++)
	{
		// get average between instance and driver
		inst->matrix.t[i] += dInst->matrix.t[i];
		inst->matrix.t[i] = inst->matrix.t[i] >> 1;
	}

	// if driver is not using mask weapon
	if ((d->actionsFlagSet & ACTION_MASK_WEAPON) == 0)
	{
		// if RainCloud alive
		if (rcloud->timeMS != 0)
		{
			rcloud->timeMS -= gGT->elapsedTimeMS;
			if (rcloud->timeMS < 0)
				rcloud->timeMS = 0;

			if (rcloud->effect != RAIN_CLOUD_EFFECT_ITEM_ROLL)
				return;

			if (d->heldItemID == 0xf)
				return;

			if (d->noItemTimer != 0)
				return;

			// set weapon to "weapon roulette" to make it spin
			d->heldItemID = 0x10;

			// you are always 5 frames away from new weapon,
			// so you get weapon 5 frames after cloud dies
			d->itemRollTimer = 5;

			// you hold zero of this item
			d->numHeldItems = 0;
			return;
		}

		// === RainCloud timeMS is over ===

		if ((rcloud->effect == RAIN_CLOUD_EFFECT_ITEM_ROLL) &&

		    // If your weapon is not "no weapon"
		    (d->heldItemID != 0xf))
		{
			d->itemRollTimer = 0;

			// pick random weapon for driver
			VehPhysGeneral_SetHeldItem(d);
		}
	}

	// using mask weapon,
	// or timeMS is over
	rcloud->timeMS = 0;
	d->thCloud = NULL;

	ThTick_SetAndExec(t, RB_RainCloud_FadeAway);
	return;
}

static char s_cloud1[] = "cloud1";

// NOTE(aalhendi): ASM-verified against NTSC-U 926 overlay 231 0x800b1220-0x800b1458.
void RB_RainCloud_Init(struct Driver *d)
{
	struct Instance *cloudInst;
	struct RainCloud *rcloud;
	struct RainLocal *rlocal;

	// if driver -> cloudTh is invalid
	if (d->thCloud == NULL)
	{
		cloudInst = INSTANCE_BirthWithThread(0x42, s_cloud1, SMALL, OTHER, RB_RainCloud_ThTick, sizeof(struct RainCloud), d->instSelf->thread);

		cloudInst->thread->funcThDestroy = PROC_DestroyInstance;

		*(int *)&cloudInst->matrix.m[0][0] = 0x1000;
		*(int *)&cloudInst->matrix.m[0][2] = 0;
		*(int *)&cloudInst->matrix.m[1][1] = 0x1000;
		*(int *)&cloudInst->matrix.m[2][0] = 0;
		cloudInst->matrix.m[2][2] = 0x1000;

		// cloud->posX = driver->posX
		cloudInst->matrix.t[0] = d->instSelf->matrix.t[0];
		cloudInst->matrix.t[1] = d->instSelf->matrix.t[1] + 0x80;
		cloudInst->matrix.t[2] = d->instSelf->matrix.t[2];

		cloudInst->alphaScale = 0x800;

		cloudInst->depthBiasNormal = d->instSelf->depthBiasNormal;
		cloudInst->depthBiasSecondary = d->instSelf->depthBiasSecondary;

		// add rain to pool
		rlocal = (struct RainLocal *)JitPool_Add(&sdata->gGT->JitPools.rain);

		if (rlocal != NULL)
		{
			rlocal->frameCount = 0x1e;

			rlocal->unk1[0] = 0;
			rlocal->unk1[1] = 0;
			rlocal->unk1[2] = 0;

			rlocal->vel.x = 0;
			rlocal->vel.y = -0x28;
			rlocal->vel.z = 0;

			rlocal->pos.x = d->instSelf->matrix.t[0];
			rlocal->pos.y = d->instSelf->matrix.t[1] + 0x80;
			rlocal->pos.z = d->instSelf->matrix.t[2];

			rlocal->cloudInst = cloudInst;
		}

		rcloud = cloudInst->thread->object;
		rcloud->timeMS = 0x1e00; // 7.68s
		rcloud->rainLocal = rlocal;
		rcloud->effect = RAIN_CLOUD_EFFECT_ITEM_ROLL;

		if (
		    // if driver has no weapon
		    (d->heldItemID == 0xf) ||

		    (d->noItemTimer != 0))
		{
			rcloud->effect = RAIN_CLOUD_EFFECT_SLOW;
		}

		d->thCloud = cloudInst->thread;
	}

	// if cloud already exists, and
	// driver hits another red potion
	else
	{
		rcloud = d->thCloud->object;

		// set duration to 8 seconds
		rcloud->timeMS = 0x1e00;

		// random number
		int rng = MixRNG_Scramble();

		rcloud->effect = (RainCloudEffect)((rng % 400) / 100);
	}
	return;
}
