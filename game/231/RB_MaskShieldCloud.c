#include <common.h>

#define RB_SHIELD_POP_SCALE R231.shieldPopScale

#define RB_MASK_POSITION R231.maskPosArr

#define RB_SHIELD_GROW_SCALE R231.shieldGrowScale

#define RB_SHIELD_PULSE_SCALE R231.shieldPulseScale

void RB_MaskWeapon_FadeAway(struct Thread *t)
{
	const struct Driver *driver = t->parentThread->object;
	struct MaskHeadScratch *mhs;
	struct Instance *inst;
	struct MaskHeadWeapon *mask;
	s32 height;
	s32 trig;
	MATRIX *m;

	inst = t->inst;
	mask = inst->thread->object;
	// Contract the orbit while keeping the head above its owner.
	height = 0x40;
	trig = MATH_Sin(mask->rot.y);
	mhs = CTR_SCRATCHPAD_PTR(struct MaskHeadScratch, 0x108);
	mhs->posOffset.x = CTR_MipsMulLo(height - ((mask->duration >> 5) * 4), trig) >> 12;
	mhs->posOffset.y = height;
	trig = MATH_Cos(mask->rot.y);
	mhs->posOffset.z = CTR_MipsMulLo(height - ((mask->duration >> 5) * 4), trig) >> 12;
	mhs->posOffset.x = (s16)VehPhysForce_ScaleValue(driver, mhs->posOffset.x);
	mhs->posOffset.y = (s16)VehPhysForce_ScaleValue(driver, mhs->posOffset.y);
	mhs->posOffset.z = (s16)VehPhysForce_ScaleValue(driver, mhs->posOffset.z);

	// Move and shrink the head before aligning the beam.
	LHMatrix_Parent(inst, ((struct Driver *)t->parentThread->object)->instSelf, &CTR_SCRATCHPAD_PTR(struct MaskHeadScratch, 0x108)->posOffset);
	VehPhysForce_ScaleMatrix(driver, &inst->matrix);
	mask->rot.y -= 0x100;
	inst->scale.x -= 0x100;
	inst->scale.y -= 0x100;
	inst->scale.z -= 0x100;

	mhs->posOffset.x = 0;
	mhs->posOffset.y = (s16)VehPhysForce_ScaleValue(driver, height);
	mhs->posOffset.z = 0;
	LHMatrix_Parent(mask->maskBeamInst, ((struct Driver *)t->parentThread->object)->instSelf, &CTR_SCRATCHPAD_PTR(struct MaskHeadScratch, 0x108)->posOffset);

	mhs->rot.x = 0;
	mhs->rot.y = mask->rot.y;
	mhs->rot.z = 0;
	ConvertRotToMatrix(&CTR_SCRATCHPAD_PTR(struct MaskHeadScratch, 0x108)->m, &CTR_SCRATCHPAD_PTR(struct MaskHeadScratch, 0x108)->rot);

	m = &mask->maskBeamInst->matrix;
	MatrixRotate(m, m, &CTR_SCRATCHPAD_PTR(struct MaskHeadScratch, 0x108)->m);
	VehPhysForce_ScaleMatrix(driver, m);
	// The beam shrinks after its rotation has been composed.
	mask->maskBeamInst->scale.x -= 0x100;
	mask->maskBeamInst->scale.y -= 0x100;
	mask->maskBeamInst->scale.z -= 0x100;

	if ((u16)mask->maskBeamInst->alphaScale < 0x1000)
	{
		mask->maskBeamInst->alphaScale += 0x200;
	}

	if (mask->duration < 0x200)
	{
		mask->duration = (u16)mask->duration + (u16)GAME_TRACKER->elapsedTimeMS;
		if (mask->duration > 0x200)
		{
			mask->duration = 0x200;
		}
	}
	else
	{
		INSTANCE_Death(mask->maskBeamInst);
		t->flags |= THREAD_FLAG_DEAD;
	}
}


void RB_MaskWeapon_ThTick(struct Thread *maskTh)
{
	const struct Driver *driver = maskTh->parentThread->object;
	struct MaskHeadScratch *mhs;
	struct MaskHeadWeapon *mask;
	struct Instance *maskInst;
	struct Instance *driverInst;
	s32 i;

	mhs = CTR_SCRATCHPAD_PTR(struct MaskHeadScratch, 0x108);
	maskInst = maskTh->inst;
	mask = maskInst->thread->object;
	driverInst = maskTh->parentThread->inst;

	if (((struct Driver *)maskTh->parentThread->object)->invisibleTimer == 0)
	{
		for (i = 0; i < GAME_TRACKER->numPlyrCurrGame; i++)
		{
			struct InstDrawPerPlayer *beamDraw;
			INST_GETIDPP(maskInst)[i].pushBuffer = &GAME_TRACKER->pushBuffer[i];
			beamDraw = INST_GETIDPP(mask->maskBeamInst);
			beamDraw[i].pushBuffer = &GAME_TRACKER->pushBuffer[i];
		}
	}
	else
	{
		for (i = 0; i < GAME_TRACKER->numPlyrCurrGame; i++)
		{
			if (i != ((struct Driver *)maskTh->parentThread->object)->driverID)
			{
				struct InstDrawPerPlayer *beamDraw;
				INST_GETIDPP(maskInst)[i].pushBuffer = NULL;
				beamDraw = INST_GETIDPP(mask->maskBeamInst);
				beamDraw[i].pushBuffer = NULL;
			}
		}
	}

	if (driverInst->flags & REFLECTIVE)
	{
		maskInst->flags |= REFLECTIVE;
		maskInst->vertSplit = driverInst->vertSplit;
		mask->maskBeamInst->flags |= REFLECTIVE;
		mask->maskBeamInst->vertSplit = driverInst->vertSplit;
	}
	else
	{
		// NOTE(aalhendi): Retail clears reflection on the head only; the beam retains its state.
		maskInst->flags &= ~REFLECTIVE;
	}
	maskInst->depthBiasNormal = driverInst->depthBiasNormal;
	maskInst->depthBiasSecondary = driverInst->depthBiasSecondary;

	mhs->posOffset.x = CTR_MipsMulLo(CTR_MipsSll(MATH_Sin(mask->rot.y), 6) >> 12, mask->scale) >> 12;
	mhs->posOffset.y = RB_MASK_POSITION[mask->maskBeamInst->animFrame] + 0x40;
	mhs->posOffset.z = CTR_MipsMulLo(CTR_MipsSll(MATH_Cos(mask->rot.y), 6) >> 12, mask->scale) >> 12;
	mhs->posOffset.x = (s16)VehPhysForce_ScaleValue(driver, mhs->posOffset.x);
	mhs->posOffset.y = (s16)VehPhysForce_ScaleValue(driver, mhs->posOffset.y);
	mhs->posOffset.z = (s16)VehPhysForce_ScaleValue(driver, mhs->posOffset.z);
	mhs->rot.x = 0;
	mhs->rot.y = mask->rot.y;
	mhs->rot.z = 0;

	if (mask->rot.z & MASK_HEAD_ROT_WORLD_SPACE)
	{
		maskInst->matrix.t[0] = mask->pos.x + mhs->posOffset.x;
		maskInst->matrix.t[1] = mask->pos.y + mhs->posOffset.y;
		maskInst->matrix.t[2] = mask->pos.z + mhs->posOffset.z;
		ConvertRotToMatrix(&maskInst->matrix, &mhs->rot);
	}
	else
	{
		LHMatrix_Parent(maskInst, driverInst, &mhs->posOffset);
		ConvertRotToMatrix(&mhs->m, &mhs->rot);
		MatrixRotate(&maskInst->matrix, &maskInst->matrix, &mhs->m);
	}

	// NOTE(aalhendi): Resample beam state after the head transforms; it has no horizontal orbit offset.
	VehPhysForce_ScaleMatrix(driver, &maskInst->matrix);
	mhs->posOffset.x = 0;
	mhs->posOffset.y = (s16)VehPhysForce_ScaleValue(driver, 0x40);
	mhs->posOffset.z = 0;
	mhs->rot.x = 0;
	mhs->rot.y = mask->rot.y;
	mhs->rot.z = 0;
	if (mask->rot.z & MASK_HEAD_ROT_WORLD_SPACE)
	{
		mask->maskBeamInst->matrix.t[0] = mask->pos.x + mhs->posOffset.x;
		mask->maskBeamInst->matrix.t[1] = mask->pos.y + mhs->posOffset.y;
		mask->maskBeamInst->matrix.t[2] = mask->pos.z + mhs->posOffset.z;
		ConvertRotToMatrix(&mask->maskBeamInst->matrix, &mhs->rot);
	}
	else
	{
		MATRIX *m;
		LHMatrix_Parent(mask->maskBeamInst, driverInst, &mhs->posOffset);
		ConvertRotToMatrix(&mhs->m, &mhs->rot);
		m = &mask->maskBeamInst->matrix;
		MatrixRotate(m, m, &mhs->m);
	}

	VehPhysForce_ScaleMatrix(driver, &mask->maskBeamInst->matrix);
	if ((s32)mask->maskBeamInst->animFrame + 1 < INSTANCE_GetNumAnimFrames(mask->maskBeamInst, 0))
	{
		mask->maskBeamInst->animFrame++;
	}
	else
	{
		mask->maskBeamInst->animFrame = 0;
	}
	mask->rot.y -= 0x100;

	if (mask->duration != 0)
	{
		if (!Chaos_IsTimeStopped())
		{
			mask->duration = (u16)mask->duration - (u16)GAME_TRACKER->elapsedTimeMS;
			if (mask->duration < 0)
			{
				mask->duration = 0;
			}
		}
	}
	else
	{
		mask->duration = 0;
		ThTick_SetAndExec(maskTh, RB_MaskWeapon_FadeAway);
	}

	mask->maskBeamInst->flags &= ~HIDE_MODEL;
	mask->maskBeamInst->scale.x = mask->scale;
	mask->maskBeamInst->scale.y = mask->scale;
	mask->maskBeamInst->scale.z = mask->scale;
	mask->maskBeamInst->alphaScale = 0;
	maskInst->flags &= ~HIDE_MODEL;
	maskInst->scale.x = mask->scale;
	maskInst->scale.y = mask->scale;
	maskInst->scale.z = mask->scale;
}

void RB_ShieldDark_ThTick_Pop(struct Thread *t)
{
	struct Shield *sh;
	struct Instance *instDark;
	SVec3 rot;

	instDark = t->inst;
	sh = instDark->thread->object;

	rot.x = 0;
	rot.y = 0;
	rot.z = 0;
	LHMatrix_Parent(instDark, ((struct Driver *)t->parentThread->object)->instSelf, &rot);
	LHMatrix_Parent(sh->instColor, ((struct Driver *)t->parentThread->object)->instSelf, &rot);

	// Both layers face forward during the pop.
	CTR_MatrixSetRotIdentity(&instDark->matrix);

	// NOTE(aalhendi): Keep the owning shield's color-layer access at each packed store.
	CTR_WriteU32AlignedLE(&sh->instColor->matrix.m[0][0], 0x1000);
	CTR_WriteU32AlignedLE(&sh->instColor->matrix.m[0][2], 0);
	CTR_WriteU32AlignedLE(&sh->instColor->matrix.m[1][1], 0x1000);
	CTR_WriteU32AlignedLE(&sh->instColor->matrix.m[2][0], 0);
	sh->instColor->matrix.m[2][2] = 0x1000;
	const struct Driver *driver = t->parentThread->object;
	VehPhysForce_ScaleMatrix(driver, &instDark->matrix);
	VehPhysForce_ScaleMatrix(driver, &sh->instColor->matrix);

	if (sh->animFrame < 0xb)
	{
		// set scale
		instDark->scale.x = RB_SHIELD_POP_SCALE[sh->animFrame][0];
		instDark->scale.y = RB_SHIELD_POP_SCALE[sh->animFrame][1];
		instDark->scale.z = RB_SHIELD_POP_SCALE[sh->animFrame][0];

		// set scale
		sh->instColor->scale.x = RB_SHIELD_POP_SCALE[sh->animFrame][0];
		sh->instColor->scale.y = RB_SHIELD_POP_SCALE[sh->animFrame][1];
		sh->instColor->scale.z = RB_SHIELD_POP_SCALE[sh->animFrame][0];

		// next frame
		sh->animFrame += 1;

		return;
	}

	// === Animation Done ===

	// play 3D sound for "shield pop"
	PlaySound3D(0x58, instDark);

	INSTANCE_Death(sh->instColor);
	INSTANCE_Death(sh->instHighlight);

	// this thread is now dead
	t->flags |= THREAD_FLAG_DEAD;

	return;
}

void RB_ShieldDark_ThTick_Grow(struct Thread *th)
{
	SVec3 pos;
	struct Instance *shieldInst;
	struct Shield *shield;
	struct Driver *player;
	struct Instance *bombInst;
	struct TrackerWeapon *tw;
	ShieldFlags shieldFlags;
	s32 i;

	shieldInst = th->inst;
	shield = shieldInst->thread->object;
	pos.x = 0;
	pos.y = 0;
	pos.z = 0;

	if (shield->highlightTimer != 0)
	{
		shield->highlightTimer--;
		shield->instHighlight->flags |= HIDE_MODEL;
		if (shield->highlightTimer == 0)
		{
			shield->instHighlight->flags &= ~HIDE_MODEL;
		}
	}
	else
	{
		shield->highlightRot.y += 0x100;
		shield->instHighlight->flags &= ~HIDE_MODEL;
		if ((s16)(shield->highlightRot.y % 0x1000) == 0x400)
		{
			shield->highlightTimer = 30;
			shield->highlightRot.y = 0xc00;
			shield->instHighlight->flags |= HIDE_MODEL;
		}
	}

	if (((struct Driver *)th->parentThread->object)->invisibleTimer == 0)
	{
		for (i = 0; i < GAME_TRACKER->numPlyrCurrGame; i++)
		{
			struct InstDrawPerPlayer *draw;
			INST_GETIDPP(shieldInst)[i].pushBuffer = &GAME_TRACKER->pushBuffer[i];
			draw = INST_GETIDPP(shield->instColor);
			draw[i].pushBuffer = &GAME_TRACKER->pushBuffer[i];
			draw = INST_GETIDPP(shield->instHighlight);
			draw[i].pushBuffer = &GAME_TRACKER->pushBuffer[i];
		}
	}
	else
	{
		for (i = 0; i < GAME_TRACKER->numPlyrCurrGame; i++)
		{
			if (i != ((struct Driver *)th->parentThread->object)->driverID)
			{
				struct InstDrawPerPlayer *draw;
				INST_GETIDPP(shieldInst)[i].pushBuffer = NULL;
				draw = INST_GETIDPP(shield->instColor);
				draw[i].pushBuffer = NULL;
				draw = INST_GETIDPP(shield->instHighlight);
				draw[i].pushBuffer = NULL;
			}
		}
	}

	// NOTE(aalhendi): Reacquire each layer and its owner after the preceding attachment call.
	LHMatrix_Parent(shieldInst, ((struct Driver *)th->parentThread->object)->instSelf, &pos);
	LHMatrix_Parent(shield->instColor, ((struct Driver *)th->parentThread->object)->instSelf, &pos);
	LHMatrix_Parent(shield->instHighlight, ((struct Driver *)th->parentThread->object)->instSelf, &pos);
	CTR_MatrixSetRotIdentity(&shieldInst->matrix);
	CTR_WriteU32AlignedLE(&shield->instColor->matrix.m[0][0], 0x1000);
	CTR_WriteU32AlignedLE(&shield->instColor->matrix.m[0][2], 0);
	CTR_WriteU32AlignedLE(&shield->instColor->matrix.m[1][1], 0x1000);
	CTR_WriteU32AlignedLE(&shield->instColor->matrix.m[2][0], 0);
	shield->instColor->matrix.m[2][2] = 0x1000;
	ConvertRotToMatrix(&shield->instHighlight->matrix, &shield->highlightRot);
	const struct Driver *driver = th->parentThread->object;
	VehPhysForce_ScaleMatrix(driver, &shieldInst->matrix);
	VehPhysForce_ScaleMatrix(driver, &shield->instColor->matrix);
	VehPhysForce_ScaleMatrix(driver, &shield->instHighlight->matrix);

	// The highlight keeps its own scale during growth, then joins the shared pulse.
	if (shield->animFrame < 8)
	{
		shieldInst->scale.x = RB_SHIELD_GROW_SCALE[shield->animFrame][0];
		shieldInst->scale.y = RB_SHIELD_GROW_SCALE[shield->animFrame][1];
		shieldInst->scale.z = RB_SHIELD_GROW_SCALE[shield->animFrame][0];
		shield->instColor->scale.x = RB_SHIELD_GROW_SCALE[shield->animFrame][0];
		shield->instColor->scale.y = RB_SHIELD_GROW_SCALE[shield->animFrame][1];
		shield->instColor->scale.z = RB_SHIELD_GROW_SCALE[shield->animFrame][0];
		shield->animFrame++;
	}
	else
	{
		shieldInst->scale.x = RB_SHIELD_PULSE_SCALE[(u32)GAME_TRACKER->timer % 6][0];
		shieldInst->scale.y = RB_SHIELD_PULSE_SCALE[(u32)GAME_TRACKER->timer % 6][1];
		shieldInst->scale.z = RB_SHIELD_PULSE_SCALE[(u32)GAME_TRACKER->timer % 6][0];
		shield->instColor->scale.x = RB_SHIELD_PULSE_SCALE[(u32)GAME_TRACKER->timer % 6][0];
		shield->instColor->scale.y = RB_SHIELD_PULSE_SCALE[(u32)GAME_TRACKER->timer % 6][1];
		shield->instColor->scale.z = RB_SHIELD_PULSE_SCALE[(u32)GAME_TRACKER->timer % 6][0];
		shield->instHighlight->scale.x = RB_SHIELD_PULSE_SCALE[(u32)GAME_TRACKER->timer % 6][0];
		shield->instHighlight->scale.y = RB_SHIELD_PULSE_SCALE[(u32)GAME_TRACKER->timer % 6][1];
		shield->instHighlight->scale.z = RB_SHIELD_PULSE_SCALE[(u32)GAME_TRACKER->timer % 6][0];
	}

	if (!(shield->flags & SHIELD_FLAG_BLUE))
	{
		if (shield->duration == 0)
		{
			((struct Driver *)th->parentThread->object)->instBubbleHold = NULL;
			PlaySound3D(0x58, shieldInst);
			goto destroyShield;
		}
		if (!Chaos_IsTimeStopped())
		{
			shield->duration -= 32;
		}
		if (shield->duration < 1920)
		{
			s16 alpha = ((60 - (shield->duration >> 5)) * 3072) / 60 + 0x400;
			shieldInst->alphaScale = alpha;
			shield->instColor->alphaScale = alpha;
			shield->instHighlight->alphaScale = alpha;
		}
	}

	shieldFlags = shield->flags;
	if ((shieldFlags & SHIELD_FLAG_POP_ON_DAMAGE) || (shieldFlags & SHIELD_FLAG_CRASH_ATTACK) ||
	    (((struct Driver *)th->parentThread->object)->actionsFlagSet & ACTION_RACE_FINISHED) ||
	    ((struct Driver *)th->parentThread->object)->kartState == KS_MASK_GRABBED)
	{
		struct Driver *owner = th->parentThread->object;
		if (shield->flags & SHIELD_FLAG_CRASH_ATTACK)
		{
			GAME_TRACKER->pushBuffer[((struct Driver *)th->parentThread->object)->driverID].fadeFromBlack_currentValue = 0x1fff;
			GAME_TRACKER->pushBuffer[((struct Driver *)th->parentThread->object)->driverID].fadeFromBlack_desiredResult = 0x1000;
			GAME_TRACKER->pushBuffer[((struct Driver *)th->parentThread->object)->driverID].fade_step = -0x88;
		}
		shield->animFrame = 0;
		owner->instBubbleHold = NULL;
		ThTick_SetAndExec(th, RB_ShieldDark_ThTick_Pop);
		return;
	}
	if (!(shieldFlags & SHIELD_FLAG_SHOOT))
	{
		return;
	}

	player = th->parentThread->object;
	player->instBubbleHold = NULL;
	player->numTimesMissileLaunched++;
	GAMEPAD_ShockFreq(player, 8, 0);
	GAMEPAD_ShockForce1(player, 8, 0x7f);

	// NOTE(aalhendi): Select from the current shield state after both haptic callbacks.
	// Keep each birth with its model so the compiler preserves retail argument setup.
	if (shield->flags & SHIELD_FLAG_BLUE)
	{
		bombInst = INSTANCE_BirthWithThread(DYNAMIC_SHIELD, rb_nameShieldBomb, MEDIUM, OTHER, RB_MovingExplosive_ThTick, sizeof(struct TrackerWeapon),
		                                    player->instSelf->thread);
	}
	else
	{
		bombInst = INSTANCE_BirthWithThread(DYNAMIC_SHIELD_GREEN, rb_nameShieldBomb, MEDIUM, OTHER, RB_MovingExplosive_ThTick, sizeof(struct TrackerWeapon),
		                                    player->instSelf->thread);
	}
	if (!(player->actionsFlagSet & ACTION_BOT))
	{
		Voiceline_RequestPlayOwned(13, GAME_CHARACTER_IDS[player->driverID], 0x10, player);
	}

	// Inherit the complete world transform before initializing the projectile's motion.
	bombInst->matrix = shieldInst->matrix;
	bombInst->thread->funcThDestroy = PROC_DestroyInstance;
	bombInst->scale.x = 0x400;
	bombInst->scale.y = 0x400;
	bombInst->scale.z = 0x400;
	bombInst->alphaScale = 0x400;
	tw = bombInst->thread->object;

	tw->flags = 0;
	tw->driverParent = player;
	tw->driverTarget = NULL;
	tw->timeAlive = 0;
	tw->soundIDCount = 0;
	tw->rotY = player->angle;
	tw->vel.y = 0;
	tw->vel.x = (player->instSelf->matrix.m[0][2] * 3) >> 7;
	tw->vel.z = (player->instSelf->matrix.m[2][2] * 3) >> 7;
	tw->parentSafetyFrames = 10;
	tw->blindFrames = 0;
	tw->instParent = player->instSelf;

	PlaySound3D(0x58, shieldInst);
destroyShield:
	INSTANCE_Death(shield->instColor);
	INSTANCE_Death(shield->instHighlight);
	th->flags |= THREAD_FLAG_DEAD;
}

void RB_RainCloud_FadeAway(struct Thread *t)
{
	const struct Driver *driver = t->parentThread->object;
	struct Instance *inst;
	struct Instance *parentInst;
	struct RainCloud *rcloud;

	parentInst = t->parentThread->inst;
	inst = t->inst;
	rcloud = inst->thread->object;

	// Follow the driver halfway, retaining the cloud's upward offset.
	inst->matrix.t[0] = (s32)((u32)inst->matrix.t[0] + parentInst->matrix.t[0]) >> 1;
	inst->matrix.t[1] = (s32)((u32)inst->matrix.t[1] + parentInst->matrix.t[1] + VehPhysForce_ScaleValue(driver, 0x80)) >> 1;
	inst->matrix.t[2] = (s32)((u32)inst->matrix.t[2] + parentInst->matrix.t[2]) >> 1;

	inst->scale.x -= VehPhysForce_ScaleValue(driver, 0x100);
	inst->scale.y -= VehPhysForce_ScaleValue(driver, 0x100);
	inst->scale.z -= VehPhysForce_ScaleValue(driver, 0x100);
	rcloud->rainLocal->frameCount = CTR_MipsSubLo(rcloud->rainLocal->frameCount, 2);

	if (inst->scale.x < 0)
	{
		JitPool_Remove(&GAME_TRACKER->JitPools.rain, (struct Item *)rcloud->rainLocal);

		// This thread is now dead
		t->flags |= THREAD_FLAG_DEAD;
	}
	return;
}

void RB_RainCloud_ThTick(struct Thread *t)
{
	struct Thread *driverTh;
	struct Instance *inst;
	struct Driver *d;
	struct RainCloud *rcloud;
	struct Instance *dInst;

	inst = t->inst;
	rcloud = t->object;
	driverTh = t->parentThread;
	d = driverTh->object;
	dInst = driverTh->inst;

	if ((s32)inst->animFrame + 1 < INSTANCE_GetNumAnimFrames(inst, 0))
	{
		inst->animFrame++;
	}
	else
	{
		inst->animFrame = 0;
	}

	// Average signed scales before narrowing the result to a halfword.
	inst->scale.x = (inst->scale.x + dInst->scale.x) >> 1;
	inst->scale.y = (inst->scale.y + dInst->scale.y) >> 1;
	inst->scale.z = (inst->scale.z + dInst->scale.z) >> 1;
	inst->matrix.t[0] = (s32)((u32)inst->matrix.t[0] + dInst->matrix.t[0]) >> 1;
	inst->matrix.t[1] = (s32)((u32)inst->matrix.t[1] + dInst->matrix.t[1] + (inst->scale.y * 5 >> 7)) >> 1;
	inst->matrix.t[2] = (s32)((u32)inst->matrix.t[2] + dInst->matrix.t[2]) >> 1;

	if (d->actionsFlagSet & ACTION_MASK_WEAPON)
	{
		rcloud->timeMS = 0;
		d->thCloud = NULL;
		ThTick_SetAndExec(t, RB_RainCloud_FadeAway);
	}
	else
	{
		if (rcloud->timeMS != 0)
		{
			if (!Chaos_IsTimeStopped())
			{
				rcloud->timeMS = (u16)rcloud->timeMS - (u16)GAME_TRACKER->elapsedTimeMS;
			}
			if (rcloud->timeMS < 0)
			{
				rcloud->timeMS = 0;
			}

			if (rcloud->effect != RAIN_CLOUD_EFFECT_ITEM_ROLL || d->heldItemID == HELD_ITEM_NONE || d->noItemTimer != 0)
			{
				return;
			}

			d->heldItemID = HELD_ITEM_ROULETTE;
			// Keep the roll five frames from completion until the cloud expires.
			d->itemRollTimer = 5;
			d->numHeldItems = 0;
			return;
		}

		// NOTE(aalhendi): Detach the cloud before the item-selection callback.
		rcloud->timeMS = 0;
		d->thCloud = NULL;
		if (rcloud->effect == RAIN_CLOUD_EFFECT_ITEM_ROLL && d->heldItemID != HELD_ITEM_NONE)
		{
			d->itemRollTimer = 0;
			VehPhysGeneral_SetHeldItem(d);
		}
		ThTick_SetAndExec(t, RB_RainCloud_FadeAway);
	}
}


void RB_RainCloud_Init(struct Driver *d)
{
	struct Instance *cloudInst;
	struct RainCloud *rcloud;
	struct RainLocal *rlocal;

	// Create the cloud and its rain emitter on the first hit.
	if (d->thCloud == NULL)
	{
		cloudInst = INSTANCE_BirthWithThread(STATIC_CLOUD, rb_nameCloud, SMALL, OTHER, RB_RainCloud_ThTick, sizeof(struct RainCloud), d->instSelf->thread);

		cloudInst->thread->funcThDestroy = PROC_DestroyInstance;

		CTR_MatrixSetRotIdentity(&cloudInst->matrix);

		// Follow the owner's position, with the cloud above the kart.
		cloudInst->matrix.t[0] = d->instSelf->matrix.t[0];
		cloudInst->matrix.t[1] = CTR_MipsAddLo(d->instSelf->matrix.t[1], VehPhysForce_ScaleValue(d, 0x80));
		cloudInst->matrix.t[2] = d->instSelf->matrix.t[2];

		cloudInst->alphaScale = 0x800;

		cloudInst->depthBiasNormal = d->instSelf->depthBiasNormal;
		cloudInst->depthBiasSecondary = d->instSelf->depthBiasSecondary;

		// add rain to pool
		rlocal = (struct RainLocal *)JitPool_Add(&GAME_TRACKER->JitPools.rain);

		if (rlocal != NULL)
		{
			rlocal->frameCount = 0x1e;
			rlocal->cloudInst = cloudInst;

			rlocal->scroll.x = 0;
			rlocal->scroll.y = 0;
			rlocal->scroll.z = 0;

			rlocal->vel.x = 0;
			rlocal->vel.y = -0x28;
			rlocal->vel.z = 0;

			rlocal->pos.x = d->instSelf->matrix.t[0];
			rlocal->pos.y = (u16)d->instSelf->matrix.t[1] + VehPhysForce_ScaleValue(d, 0x80);
			rlocal->pos.z = d->instSelf->matrix.t[2];
		}

		rcloud = cloudInst->thread->object;
		rcloud->timeMS = 0x1e00; // 7.68s
		rcloud->rainLocal = rlocal;
		rcloud->effect = RAIN_CLOUD_EFFECT_ITEM_ROLL;

		if (d->heldItemID == HELD_ITEM_NONE || d->noItemTimer != 0)
		{
			rcloud->effect = RAIN_CLOUD_EFFECT_SLOW;
		}

		d->thCloud = cloudInst->thread;
	}

	// Another red potion refreshes the cloud and rerolls its effect.
	else
	{
		s32 rng;

		// Restart the 7.68-second effect.
		((struct RainCloud *)d->thCloud->object)->timeMS = 0x1e00;

		// random number
		rng = MixRNG_Scramble();

		((struct RainCloud *)d->thCloud->object)->effect = (RainCloudEffect)((rng % 400) / 100);
	}
	return;
}
