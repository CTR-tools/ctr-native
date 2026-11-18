#include "VehCommon.h"
#include "VehPickState.h"

enum
{
	VEH_PICK_REASON_BOMB = 1,
	VEH_PICK_REASON_MOTIONLESS_POTION = 2,
	VEH_PICK_REASON_MISSILE = 3,
	VEH_PICK_REASON_MOVING_POTION = 4,
	VEH_PICK_REASON_TURBO_SQUISH = 5,
	VEH_PICK_REASON_MASK_WEAPON = 6,

	VEH_PICK_VOICELINE_VICTIM_LAUGH = 2,
	VEH_PICK_VOICELINE_COMMON_DAMAGE = 1,
	VEH_PICK_VOICELINE_SQUISH = 4,
	VEH_PICK_VOICELINE_PRIORITY = 0x10,

	VEH_PICK_SHIELD_DAMAGE_INVINCIBLE_TIMER = 0x2a0,
	VEH_PICK_SPIN_NO_INPUT_TIMER = 0x3c0,
	VEH_PICK_BLAST_NO_INPUT_TIMER = 0x960,
	VEH_PICK_SQUISH_NO_INPUT_TIMER = 0xf0,
	VEH_PICK_SQUISH_TIMER = 0xf00,
	VEH_PICK_BURN_NO_INPUT_TIMER = 0x780,
	VEH_PICK_BURN_TIMER = 0xf00,
	VEH_PICK_MASK_GRAB_NO_INPUT_TIMER = 0xd20,

	VEH_PICK_SOUND_SQUISH = 0x5a,
	VEH_PICK_SOUND_BURN = 0x69,

	VEH_PICK_ATTACK_QUIP_BOMB = 0x1,
	VEH_PICK_ATTACK_QUIP_MISSILE = 0x2,
	VEH_PICK_ATTACK_QUIP_MOVING_POTION = 0x4,
	VEH_PICK_ATTACK_QUIP_MASK = 0x8,

	VEH_PICK_RUMBLE_FRAMES = 8,
	VEH_PICK_RUMBLE_FORCE = 0x7f,
	VEH_PICK_BATTLE_HUD_OFFSET_Y = 0x14,
	VEH_PICK_BATTLE_HUD_COOLDOWN = 5,
};


static int VehPickState_ApplyDamage(struct Driver *victimDriver, int damageType, struct Driver *attackDriver, int reason, bool bypassProtection)
{
	int victimState = victimDriver->kartState;
	register int victimID CTR_PSX_REGISTER("$2");
	register int voiceType CTR_PSX_REGISTER("$4");
	register u32 characterPage CTR_PSX_REGISTER("$3");
	register s16 *characterIDs CTR_PSX_REGISTER("$3");
	struct Thread *victimThread;
	int battleResultPass;
	SVec4 worldPosition;
	SVec2 posScreen;
	register s16 *screenPosition CTR_PSX_REGISTER("$6");
	register int one CTR_PSX_REGISTER("$20") = 1;

	CTR_PSX_KEEP_VALUE(one);
	victimDriver->pendingDamageType = 0;

	if (victimState == KS_MASK_GRABBED)
	{
		goto ReturnZero;
	}

	if (!bypassProtection && (
	    // If player is using mask weapon and it protects from damage
	    ((victimDriver->actionsFlagSet & ACTION_MASK_WEAPON) != 0 && g_config.maskProtectsFromDamage) ||

	    (victimDriver->invincibleTimer != 0)))
	{
		voiceType = VEH_PICK_VOICELINE_VICTIM_LAUGH;
		characterIDs = GAME_CHARACTER_IDS;
		victimID = victimDriver->driverID;
		goto VictimLaugh;
	}

	if (!bypassProtection && victimDriver->instBubbleHold != NULL)
	{
		{
			register struct Shield *shield CTR_PSX_REGISTER("$3");
			register u32 shieldFlags CTR_PSX_REGISTER("$2");

			shield = victimDriver->instBubbleHold->thread->object;
			shieldFlags = shield->flags;
			CTR_PSX_OBSERVE_VALUE(shieldFlags);
			voiceType = VEH_PICK_VOICELINE_VICTIM_LAUGH;
			shield->flags = (u16)(shieldFlags | SHIELD_FLAG_POP_ON_DAMAGE);
			CTR_PSX_OBSERVE_MEMORY(shield->flags);
		}

		{
			register int shieldTimer CTR_PSX_REGISTER("$2");

			shieldTimer = VEH_PICK_SHIELD_DAMAGE_INVINCIBLE_TIMER;
			victimDriver->invincibleTimer = shieldTimer;
		}
		characterIDs = GAME_CHARACTER_IDS;
		victimID = victimDriver->driverID;
		victimDriver->instBubbleHold = NULL;
	}
	else
	{
		goto CheckDamage;
	}

VictimLaugh:
	Voiceline_RequestPlay(voiceType, characterIDs[victimID], VEH_PICK_VOICELINE_PRIORITY);
ReturnZero:
	return 0;

CheckDamage:
	if (damageType == VEH_PICK_DAMAGE_NONE)
	{
		return 1;
	}

	victimThread = victimDriver->instSelf->thread;

	if (damageType == VEH_PICK_DAMAGE_SQUISH)
	{
		goto Squish;
	}

	if (damageType < VEH_PICK_DAMAGE_BURN)
	{
		if (damageType == VEH_PICK_DAMAGE_BLAST)
		{
			goto Blast;
		}

		goto DefaultSpin;
	}

	if (damageType == VEH_PICK_DAMAGE_BURN)
	{
		goto Burn;
	}

	if (damageType == VEH_PICK_DAMAGE_MASK_GRAB)
	{
		goto MaskGrab;
	}

	goto DefaultSpin;

Blast:
	if (victimState == KS_BLASTED)
	{
		goto ReturnZero;
	}

	{
		register u32 tumblePage CTR_PSX_REGISTER("$2");
		register DriverFunc currentInit CTR_PSX_REGISTER("$3");
		register DriverFunc tumbleInit CTR_PSX_REGISTER("$5");

		tumblePage = VEH_TUMBLE_INIT_PAGE;
		CTR_PSX_FORGET_VALUE(tumblePage);
		currentInit = victimDriver->funcPtrs[DRIVER_FUNC_INIT];
		tumbleInit = VEH_TUMBLE_INIT_FROM_PAGE(tumblePage);
		if (currentInit == tumbleInit)
		{
			goto ReturnZero;
		}

		voiceType = VEH_PICK_VOICELINE_COMMON_DAMAGE;
		{
			register int noInputTimer CTR_PSX_REGISTER("$2");

			noInputTimer = VEH_PICK_BLAST_NO_INPUT_TIMER;
			victimDriver->NoInputTimer = noInputTimer;
		}
		VEH_LOAD_CHARACTER_IDS_PAGE(characterPage);
		victimID = victimDriver->driverID;
		VEH_ADD_CHARACTER_IDS_LOW(characterIDs, characterPage);
		victimDriver->squishTimer = 0;
		victimDriver->funcPtrs[DRIVER_FUNC_INIT] = tumbleInit;
		goto CommonDamageVoice;
	}

MaskGrab:
	voiceType = VEH_PICK_VOICELINE_COMMON_DAMAGE;
	{
		register int noInputTimer CTR_PSX_REGISTER("$2");

		noInputTimer = VEH_PICK_MASK_GRAB_NO_INPUT_TIMER;
		victimDriver->NoInputTimer = noInputTimer;
		CTR_PSX_OBSERVE_MEMORY(victimDriver->NoInputTimer);
	}
	{
		register DriverFunc plantEatenInit CTR_PSX_REGISTER("$2");

		plantEatenInit = VehStuckProc_PlantEaten_Init;
		VEH_LOAD_CHARACTER_IDS_PAGE(characterPage);
		victimDriver->funcPtrs[DRIVER_FUNC_INIT] = plantEatenInit;
		CTR_PSX_OBSERVE_MEMORY(victimDriver->funcPtrs[DRIVER_FUNC_INIT]);
	}
	victimID = victimDriver->driverID;
	CTR_PSX_OBSERVE_VALUE(victimID);
	VEH_ADD_CHARACTER_IDS_LOW(characterIDs, characterPage);

CommonDamageVoice:
{
	register s16 *characterEntry CTR_PSX_REGISTER("$2");

	CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(characterEntry, characterIDs, CTR_MipsSll((u32)victimID, voiceType));
	Voiceline_RequestPlay(voiceType, *characterEntry, VEH_PICK_VOICELINE_PRIORITY);
}
	goto DamageApplied;

Burn:
	if (victimDriver->burnTimer == 0)
	{
		OtherFX_Play(VEH_PICK_SOUND_BURN, 1);
		Voiceline_RequestPlay(VEH_PICK_VOICELINE_COMMON_DAMAGE, GAME_CHARACTER_IDS[victimDriver->driverID], VEH_PICK_VOICELINE_PRIORITY);
	}

	victimDriver->burnTimer = VEH_PICK_BURN_TIMER;
	victimDriver->NoInputTimer = VEH_PICK_BURN_NO_INPUT_TIMER;
	goto SpinOut;

Squish:
	if (victimState != KS_SPINNING)
	{
		OtherFX_Play_Echo(VEH_PICK_SOUND_SQUISH, 1, (u16)(victimDriver->actionsFlagSet >> 16) & 1);
		Voiceline_RequestPlay(VEH_PICK_VOICELINE_SQUISH, GAME_CHARACTER_IDS[victimDriver->driverID], VEH_PICK_VOICELINE_PRIORITY);
	}

	victimDriver->squishTimer = VEH_PICK_SQUISH_TIMER;
	victimDriver->NoInputTimer = VEH_PICK_SQUISH_NO_INPUT_TIMER;
	goto SpinOut;

DefaultSpin:
	victimDriver->NoInputTimer = VEH_PICK_SPIN_NO_INPUT_TIMER;
	if (victimDriver->kartState == KS_SPINNING)
	{
		goto DamageApplied;
	}

SpinOut:
	victimDriver->funcPtrs[DRIVER_FUNC_INIT] = VehPhysProc_SpinFirst_Init;

DamageApplied:
	switch (reason)
	{
	case VEH_PICK_REASON_BOMB:
		victimDriver->numTimesBombHitYou++;
		if ((attackDriver != NULL) && (attackDriver != victimDriver))
		{
			attackDriver->numTimesBombsHitSomeone++;
			attackDriver->quip4 |= VEH_PICK_ATTACK_QUIP_BOMB;
		}
		break;

	case VEH_PICK_REASON_MISSILE:
		victimDriver->numTimesMissileHitYou++;
		if ((attackDriver != NULL) && (attackDriver != victimDriver))
		{
			attackDriver->numTimesMissileHitSomeone++;
			attackDriver->quip4 |= VEH_PICK_ATTACK_QUIP_MISSILE;
		}
		break;

	case VEH_PICK_REASON_MOVING_POTION:
		if ((attackDriver != NULL) && (attackDriver != victimDriver))
		{
			attackDriver->numTimesMovingPotionHitSomeone++;
			attackDriver->quip4 |= VEH_PICK_ATTACK_QUIP_MOVING_POTION;
		}
		break;

	case VEH_PICK_REASON_MOTIONLESS_POTION:
		victimDriver->numTimesMotionlessPotionHitYou++;
		break;

	case VEH_PICK_REASON_TURBO_SQUISH:
		if ((attackDriver != NULL) && (attackDriver != victimDriver))
		{
			attackDriver->numTimesSquishedSomeone++;
		}
		break;

	case VEH_PICK_REASON_MASK_WEAPON:
		if ((attackDriver != NULL) && (attackDriver != victimDriver))
		{
			attackDriver->quip4 |= VEH_PICK_ATTACK_QUIP_MASK;
		}
		break;

	default:
		break;
	}

	victimDriver->kartState = KS_NORMAL;
	VehFire_SetReserves(victimDriver, 0);
	victimDriver->turbo_outsideTimer = 0;
	victimDriver->matrixArray = BAKED_GTE_MATRIX_NONE;
	victimDriver->matrixIndex = 0;

	GAMEPAD_ShockFreq(victimDriver, VEH_PICK_RUMBLE_FRAMES, 0);
	GAMEPAD_ShockForce1(victimDriver, VEH_PICK_RUMBLE_FRAMES, VEH_PICK_RUMBLE_FORCE);

	if ((attackDriver != NULL) && ((GAME_TRACKER->gameMode1 & END_OF_RACE) == 0))
	{
		worldPosition.x = (s16)attackDriver->instSelf->matrix.t[0];
		worldPosition.y = (s16)attackDriver->instSelf->matrix.t[1];
		worldPosition.z = (s16)attackDriver->instSelf->matrix.t[2];
		VehGteSetRotTransMatrix(&GAME_TRACKER->pushBuffer[attackDriver->driverID].matrix_ViewProj);
		CTR_GteLoadPositionV0(&worldPosition);
		gte_rtps();
		screenPosition = CTR_VECTOR_DATA(&posScreen);
		CTR_GteStorePositionXY(screenPosition);

		// screenPosXY
		attackDriver->BattleHUD.startX = screenPosition[0] + GAME_TRACKER->pushBuffer[attackDriver->driverID].rect.x;
		attackDriver->BattleHUD.startY = screenPosition[1] + GAME_TRACKER->pushBuffer[attackDriver->driverID].rect.y - VEH_PICK_BATTLE_HUD_OFFSET_Y;

		battleResultPass = 0;
		if ((GAME_TRACKER->gameMode1 & LIFE_LIMIT) != 0)
		{
			one = 1;
		}
		while (battleResultPass < one)
		{
			RB_Player_KillPlayer(attackDriver, victimDriver);

			// NOTE(aalhendi): Retail rechecks END_OF_RACE after RB_Player_KillPlayer,
			// which can transition battle finish state inside this one-pass loop.
			if ((GAME_TRACKER->gameMode1 & END_OF_RACE) != 0)
			{
				attackDriver->quip1 = (s16)reason;
				victimDriver->quip3 = (s16)reason;
			}

			battleResultPass++;
		}

		if ((attackDriver == victimDriver) && ((GAME_TRACKER->gameMode1 & POINT_LIMIT) != 0))
		{
			if (victimDriver->BattleHUD.cooldown == VEH_PICK_BATTLE_HUD_COOLDOWN)
			{
				victimDriver->BattleHUD.scoreDelta = CTR_MipsSubLo(victimDriver->BattleHUD.scoreDelta, one);
			}
			else
			{
				victimDriver->BattleHUD.scoreDelta = CTR_MipsNegLo(one);
			}
		}
		else
		{
			if (attackDriver->BattleHUD.cooldown == VEH_PICK_BATTLE_HUD_COOLDOWN)
			{
				attackDriver->BattleHUD.scoreDelta = CTR_MipsAddLo(attackDriver->BattleHUD.scoreDelta, one);
			}
			else
			{
				attackDriver->BattleHUD.scoreDelta = one;
			}
		}

		CTR_PSX_KEEP_VALUE(one);
		attackDriver->BattleHUD.cooldown = VEH_PICK_BATTLE_HUD_COOLDOWN;
		victimDriver->numTimesAttackedByPlayer[attackDriver->driverID]++;
		attackDriver->numTimesAttackingPlayer[victimDriver->driverID]++;

		if (attackDriver != victimDriver)
		{
			attackDriver->numTimesAttacking++;
		}
	}

	victimThread->flags &= ~THREAD_FLAG_DISABLE_COLLISION;
	victimDriver->instSelf->flags &= ~HIDE_MODEL;

	return 1;
}

int VehPickState_TrySquish(struct Driver *driver)
{
	return VehPickState_ApplyDamage(driver, VEH_PICK_DAMAGE_SQUISH, NULL, 0, false);
}

int VehPickState_NewState(struct Driver *victimDriver, int damageType, struct Driver *attackDriver, int reason)
{
	return VehPickState_ApplyDamage(victimDriver, damageType, attackDriver, reason, false);
}

int VehPickState_ForceDamage(struct Driver *driver, int damageType)
{
	return VehPickState_ApplyDamage(driver, damageType, NULL, 0, true);
}
