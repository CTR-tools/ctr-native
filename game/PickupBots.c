#include <common.h>

void PickupBots_Init(void)
{
	// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80040850-0x800408b8.
	int hub;
	int lev = sdata->gGT->levelID;

	// get hubID of level
	hub = data.metaDataLEV[lev].hubID;

	// If Level ID is Oxide Station
	if (lev == OXIDE_STATION)
	{
		hub = 0;
	}

	if (hub > -1)
	{
		// set pointer to boss weapon meta
		sdata->bossWeaponMeta = data.bossWeaponMetaPtr[hub];
	}
	return;
}

enum
{
	PICKUPBOTS_ITEM_NONE = HELD_ITEM_NONE,
	PICKUPBOTS_ITEM_INVALID = -1,
	PICKUPBOTS_ITEM_BOMB = HELD_ITEM_BOMB_1X,
	PICKUPBOTS_ITEM_MISSILE = HELD_ITEM_MISSILE_1X,
	PICKUPBOTS_ITEM_TNT = HELD_ITEM_TNT,
	PICKUPBOTS_ITEM_POTION = HELD_ITEM_POTION,
	PICKUPBOTS_SHOOT_ID_BOMB_MISSILE = 2,
	PICKUPBOTS_SHOOT_FLAG_RANDOM = 0x1,
	PICKUPBOTS_SHOOT_FLAG_BACKWARD = 0x2,
	PICKUPBOTS_VOICELINE_BOMB = 10,
	PICKUPBOTS_VOICELINE_MISSILE = 11,
	PICKUPBOTS_VOICELINE_MINE_DROP = 0xf,
	PICKUPBOTS_CLOSE_DIST_SQ_BIAS = 0x90001,
	PICKUPBOTS_CLOSE_DIST_SQ_RANGE = 0x13affff,
	PICKUPBOTS_COOLDOWN_RANDOM_MASK = 0xff,
	PICKUPBOTS_COOLDOWN_BASE_FRAMES = 0xf0,
	PICKUPBOTS_LEADING_ATTACK_ROLL_MOD = 200,
	PICKUPBOTS_LEADING_ATTACK_POWERED_ROLL_MOD = 100,
	PICKUPBOTS_LEADING_ATTACK_POWERED_ROLL_THRESHOLD = 0x32,
	PICKUPBOTS_TRAILING_ATTACK_ROLL_MOD = 800,
	PICKUPBOTS_TRAILING_ATTACK_MISSILE_THRESHOLD = 2,
	PICKUPBOTS_TRAILING_ATTACK_BOMB_THRESHOLD = 4,
	PICKUPBOTS_TRAILING_ATTACK_DISTANCE_TO_FINISH_MIN = 16000,
	PICKUPBOTS_BOSS_PATH_REQUEST_FRAMES = 0x1e,
	PICKUPBOTS_BOSS_JUICE_COUNTER_MAX = 5,
	PICKUPBOTS_BOSS_SPEED_MIN = 0x1f41,
	PICKUPBOTS_BOSS_COOLDOWN_RANDOM_MASK = 0x10,
	PICKUPBOTS_BOSS_COOLDOWN_BASE_FRAMES = 0xc,
	PICKUPBOTS_BOSS_LOSS_COOLDOWN_STEP = 4,
	PICKUPBOTS_BOSS_CHECKPOINT_DISTANCE_SHIFT = 3,
	PICKUPBOTS_UPDATE_START_DELAY = 0x4b00,
	PICKUPBOTS_UPDATE_NEGATIVE_MODE_START_DELAY = 0x12c0,
};

CTR_STATIC_ASSERT(PICKUPBOTS_ITEM_NONE == 0xf);
CTR_STATIC_ASSERT(PICKUPBOTS_ITEM_INVALID == -1);
CTR_STATIC_ASSERT(PICKUPBOTS_ITEM_BOMB == 1);
CTR_STATIC_ASSERT(PICKUPBOTS_ITEM_MISSILE == 2);
CTR_STATIC_ASSERT(PICKUPBOTS_ITEM_TNT == 3);
CTR_STATIC_ASSERT(PICKUPBOTS_ITEM_POTION == 4);
CTR_STATIC_ASSERT(PICKUPBOTS_SHOOT_ID_BOMB_MISSILE == 2);
CTR_STATIC_ASSERT(PICKUPBOTS_SHOOT_FLAG_RANDOM == 0x1);
CTR_STATIC_ASSERT(PICKUPBOTS_SHOOT_FLAG_BACKWARD == 0x2);
CTR_STATIC_ASSERT(PICKUPBOTS_VOICELINE_BOMB == 10);
CTR_STATIC_ASSERT(PICKUPBOTS_VOICELINE_MISSILE == 11);
CTR_STATIC_ASSERT(PICKUPBOTS_VOICELINE_MINE_DROP == 0xf);
CTR_STATIC_ASSERT(PICKUPBOTS_CLOSE_DIST_SQ_BIAS == 0x90001);
CTR_STATIC_ASSERT(PICKUPBOTS_CLOSE_DIST_SQ_RANGE == 0x13affff);
CTR_STATIC_ASSERT(PICKUPBOTS_COOLDOWN_RANDOM_MASK == 0xff);
CTR_STATIC_ASSERT(PICKUPBOTS_COOLDOWN_BASE_FRAMES == 0xf0);
CTR_STATIC_ASSERT(PICKUPBOTS_LEADING_ATTACK_ROLL_MOD == 200);
CTR_STATIC_ASSERT(PICKUPBOTS_LEADING_ATTACK_POWERED_ROLL_MOD == 100);
CTR_STATIC_ASSERT(PICKUPBOTS_LEADING_ATTACK_POWERED_ROLL_THRESHOLD == 0x32);
CTR_STATIC_ASSERT(PICKUPBOTS_TRAILING_ATTACK_ROLL_MOD == 800);
CTR_STATIC_ASSERT(PICKUPBOTS_TRAILING_ATTACK_MISSILE_THRESHOLD == 2);
CTR_STATIC_ASSERT(PICKUPBOTS_TRAILING_ATTACK_BOMB_THRESHOLD == 4);
CTR_STATIC_ASSERT(PICKUPBOTS_TRAILING_ATTACK_DISTANCE_TO_FINISH_MIN == 16000);
CTR_STATIC_ASSERT(PICKUPBOTS_BOSS_PATH_REQUEST_FRAMES == 0x1e);
CTR_STATIC_ASSERT(PICKUPBOTS_BOSS_JUICE_COUNTER_MAX == 5);
CTR_STATIC_ASSERT(PICKUPBOTS_BOSS_SPEED_MIN == 0x1f41);
CTR_STATIC_ASSERT(PICKUPBOTS_BOSS_COOLDOWN_RANDOM_MASK == 0x10);
CTR_STATIC_ASSERT(PICKUPBOTS_BOSS_COOLDOWN_BASE_FRAMES == 0xc);
CTR_STATIC_ASSERT(PICKUPBOTS_BOSS_LOSS_COOLDOWN_STEP == 4);
CTR_STATIC_ASSERT(PICKUPBOTS_BOSS_CHECKPOINT_DISTANCE_SHIFT == 3);
CTR_STATIC_ASSERT(PICKUPBOTS_UPDATE_START_DELAY == 0x4b00);
CTR_STATIC_ASSERT(PICKUPBOTS_UPDATE_NEGATIVE_MODE_START_DELAY == 0x12c0);

static int PickupBots_IsBotWeaponReady(struct Driver *driver)
{
#if defined(CTR_NATIVE)
	// NOTE(aalhendi): Retail can read PS1 low memory when a boss-race
	// end-of-race rank slot is empty. Native treats that slot as no bot.
	if (driver == NULL)
	{
		return 0;
	}
#endif

	return ((driver->actionsFlagSet & ACTION_BOT) != 0) && ((driver->botData.botFlags & BOT_FLAG_DAMAGE_ACTIVE) == 0) &&
	       ((driver->actionsFlagSet & ACTION_RACE_FINISHED) == 0) && (driver->botData.weaponCooldown == 0) && (driver->instTntRecv == NULL) &&
	       (driver->clockReceive == 0);
}

static int PickupBots_IsCloseToPlayer(struct Driver *player, struct Driver *bot)
{
	int x = player->instSelf->matrix.t[0] - bot->instSelf->matrix.t[0];
	int z = player->instSelf->matrix.t[2] - bot->instSelf->matrix.t[2];

	return (u32)((x * x + z * z) - PICKUPBOTS_CLOSE_DIST_SQ_BIAS) < PICKUPBOTS_CLOSE_DIST_SQ_RANGE;
}

static void PickupBots_SetCooldown(struct Driver *bot)
{
	bot->botData.weaponCooldown = (MixRNG_Scramble() & PICKUPBOTS_COOLDOWN_RANDOM_MASK) + PICKUPBOTS_COOLDOWN_BASE_FRAMES;
}

static void PickupBots_PlayVoice(u32 voiceID, struct Driver *attacker, struct Driver *victim)
{
	Voiceline_RequestPlay(voiceID, data.characterIDs[attacker->driverID], data.characterIDs[victim->driverID]);
}

static void PickupBots_UpdateArcade(void)
{
	struct GameTracker *gGT = sdata->gGT;

	for (int i = 0; i < gGT->numPlyrCurrGame; i++)
	{
		struct Driver *player = gGT->drivers[i];

		if (player->driverRank != 0)
		{
			struct Driver *bot = gGT->driversInRaceOrder[player->driverRank - 1];

			if (PickupBots_IsBotWeaponReady(bot) && PickupBots_IsCloseToPlayer(player, bot))
			{
				int rng = MixRNG_Scramble() % PICKUPBOTS_LEADING_ATTACK_ROLL_MOD;

				if (rng == 0)
				{
					int weaponID;

					if ((bot->lapIndex != 0) &&
					    (MixRNG_Scramble() % PICKUPBOTS_LEADING_ATTACK_POWERED_ROLL_MOD < PICKUPBOTS_LEADING_ATTACK_POWERED_ROLL_THRESHOLD))
					{
						bot->numWumpas = DRIVER_WUMPA_JUICED_COUNT;
					}

					if ((gGT->elapsedEventTime & 1) != 0)
					{
						bot->heldItemID = PICKUPBOTS_ITEM_TNT;

						if ((player->actionsFlagSet & ACTION_BOT) == 0)
						{
							PickupBots_PlayVoice(PICKUPBOTS_VOICELINE_MINE_DROP, bot, player);
						}

						weaponID = PICKUPBOTS_ITEM_TNT;
					}
					else
					{
						bot->heldItemID = PICKUPBOTS_ITEM_POTION;

						if ((player->actionsFlagSet & ACTION_BOT) == 0)
						{
							PickupBots_PlayVoice(PICKUPBOTS_VOICELINE_MINE_DROP, bot, player);
						}

						weaponID = PICKUPBOTS_ITEM_POTION;
					}

					VehPickupItem_ShootNow(bot, weaponID, 0);
					bot->numWumpas = 0;
					PickupBots_SetCooldown(bot);
				}
				else if (rng == 1)
				{
					bot->heldItemID = PICKUPBOTS_ITEM_BOMB;

					if ((player->actionsFlagSet & ACTION_BOT) == 0)
					{
						PickupBots_PlayVoice(PICKUPBOTS_VOICELINE_BOMB, bot, player);
					}

					VehPickupItem_ShootNow(bot, PICKUPBOTS_SHOOT_ID_BOMB_MISSILE, 0);
					PickupBots_SetCooldown(bot);
				}
				else if (rng == 2)
				{
					bot->heldItemID = PICKUPBOTS_ITEM_MISSILE;

					if ((player->actionsFlagSet & ACTION_BOT) == 0)
					{
						PickupBots_PlayVoice(PICKUPBOTS_VOICELINE_MISSILE, bot, player);
					}

					VehPickupItem_ShootNow(bot, PICKUPBOTS_SHOOT_ID_BOMB_MISSILE, 0);
					PickupBots_SetCooldown(bot);
				}

				bot->heldItemID = PICKUPBOTS_ITEM_NONE;
			}
		}

		if (player->driverRank < 3)
		{
			struct Driver *bot = gGT->driversInRaceOrder[player->driverRank + 1];

			if (PickupBots_IsBotWeaponReady(bot) &&
			    (((int)player->lapIndex < (int)gGT->numLaps) || (player->distanceToFinish_curr > PICKUPBOTS_TRAILING_ATTACK_DISTANCE_TO_FINISH_MIN)) &&
			    PickupBots_IsCloseToPlayer(player, bot))
			{
				int rng = MixRNG_Scramble() % PICKUPBOTS_TRAILING_ATTACK_ROLL_MOD;
				int weaponID = PICKUPBOTS_ITEM_NONE;

				if ((rng < PICKUPBOTS_TRAILING_ATTACK_MISSILE_THRESHOLD) && (bot->lapIndex != (u8)(gGT->numLaps - 1)))
				{
					weaponID = PICKUPBOTS_ITEM_MISSILE;
				}
				else if (rng < PICKUPBOTS_TRAILING_ATTACK_BOMB_THRESHOLD)
				{
					weaponID = PICKUPBOTS_ITEM_BOMB;
				}

				if (weaponID != PICKUPBOTS_ITEM_NONE)
				{
					bot->heldItemID = weaponID;

					if ((player->actionsFlagSet & ACTION_BOT) == 0)
					{
						PickupBots_PlayVoice(PICKUPBOTS_VOICELINE_MISSILE, bot, player);
					}

					VehPickupItem_ShootNow(bot, PICKUPBOTS_SHOOT_ID_BOMB_MISSILE, 0);
					PickupBots_SetCooldown(bot);
				}

				bot->heldItemID = PICKUPBOTS_ITEM_NONE;
			}
		}
	}
}

static void PickupBots_SetBossCooldown(struct MetaDataBOSS *bossMeta)
{
	struct GameTracker *gGT = sdata->gGT;

	sdata->bossWeaponCooldown = (RngDeadCoed(&sdata->advRng) & PICKUPBOTS_BOSS_COOLDOWN_RANDOM_MASK) + bossMeta->weaponCooldown +
	                            PICKUPBOTS_BOSS_COOLDOWN_BASE_FRAMES +
	                            ((s8)sdata->advProgress.timesLostBossRace[gGT->bossID] * PICKUPBOTS_BOSS_LOSS_COOLDOWN_STEP);
}

static struct MetaDataBOSS *PickupBots_GetInitialBossMeta(void)
{
	struct GameTracker *gGT = sdata->gGT;

	if (gGT->levelID == OXIDE_STATION)
	{
		return data.bossWeaponMetaPtr[0];
	}

	return data.bossWeaponMetaPtr[data.metaDataLEV[gGT->levelID].hubID];
}

static void PickupBots_AdvanceBossMeta(struct Driver *boss)
{
	struct GameTracker *gGT = sdata->gGT;
	struct MetaDataBOSS *bossMeta = sdata->bossWeaponMeta;
	struct MetaDataBOSS *nextMeta = &bossMeta[1];

	if (nextMeta->throwFlag == 0)
	{
		int threshold = gGT->level1->ptr_restart_points[bossMeta->trackCheckpoint].distToFinish << PICKUPBOTS_BOSS_CHECKPOINT_DISTANCE_SHIFT;

		if (threshold < (int)boss->distanceToFinish_curr)
		{
			int preservedThrow = -1;

			if (((bossMeta->weaponType == BOSS_WEAPON_ENCODED_POTION) || (bossMeta->weaponType == BOSS_WEAPON_ENCODED_TNT)) &&
			    (sdata->bossJuiceCounter == PICKUPBOTS_BOSS_JUICE_COUNTER_MAX))
			{
				preservedThrow = bossMeta->throwFlag;
			}

			bossMeta = PickupBots_GetInitialBossMeta();

			if (preservedThrow != -1)
			{
				bossMeta->throwFlag = preservedThrow;
			}
		}
	}
	else
	{
		int threshold = gGT->level1->ptr_restart_points[nextMeta->trackCheckpoint].distToFinish << PICKUPBOTS_BOSS_CHECKPOINT_DISTANCE_SHIFT;

		if ((int)boss->distanceToFinish_curr < threshold)
		{
			int preservedThrow = -1;

			if (((bossMeta->weaponType == BOSS_WEAPON_ENCODED_POTION) || (bossMeta->weaponType == BOSS_WEAPON_ENCODED_TNT)) &&
			    (sdata->bossJuiceCounter == PICKUPBOTS_BOSS_JUICE_COUNTER_MAX))
			{
				preservedThrow = bossMeta->throwFlag;
			}

			bossMeta = nextMeta;

			if (preservedThrow != -1)
			{
				bossMeta->throwFlag = preservedThrow;
			}
		}
	}

	sdata->bossWeaponMeta = bossMeta;
}

static void PickupBots_UpdateBossPathRequest(struct Driver *boss)
{
	if (sdata->bossWeaponMeta->pathChangeDisabled != 0)
	{
		return;
	}

	if (sdata->bossPathRequestTimer == PICKUPBOTS_BOSS_PATH_REQUEST_FRAMES)
	{
		if ((boss->botData.botFlags & BOT_FLAG_BOSS_PATH_ACTIVE) != 0)
		{
			return;
		}

		if (sdata->bossPathRequestPhase == 0)
		{
			if (boss->botData.botPath == 2)
			{
				boss->botData.desiredPath_BossOnly = 1;
				sdata->bossPathRequestTimer = 0;
				boss->botData.botFlags |= BOT_FLAG_BOSS_PATH_REQUESTED;
			}
			else if (boss->botData.botPath == 1)
			{
				boss->botData.desiredPath_BossOnly = 0;
				sdata->bossPathRequestTimer = 0;
				sdata->bossPathRequestPhase = boss->botData.botPath;
				boss->botData.botFlags |= BOT_FLAG_BOSS_PATH_REQUESTED;
			}
		}
		else
		{
			if (boss->botData.botPath == 0)
			{
				boss->botData.desiredPath_BossOnly = 1;
				sdata->bossPathRequestTimer = 0;
				boss->botData.botFlags |= BOT_FLAG_BOSS_PATH_REQUESTED;
			}
			else if (boss->botData.botPath == 1)
			{
				boss->botData.desiredPath_BossOnly = 2;
				sdata->bossPathRequestTimer = 0;
				sdata->bossPathRequestPhase = 0;
				boss->botData.botFlags |= BOT_FLAG_BOSS_PATH_REQUESTED;
			}
		}
	}
	else if ((boss->botData.botFlags & BOT_FLAG_BOSS_PATH_REQUESTED) == 0)
	{
		sdata->bossPathRequestTimer++;
	}
}

static int PickupBots_GetBossWeaponID(struct MetaDataBOSS *bossMeta)
{
	int weaponID = bossMeta->weaponType;

	if (weaponID == BOSS_WEAPON_ENCODED_TNT)
	{
		weaponID = PICKUPBOTS_ITEM_TNT;
	}
	else if (weaponID == BOSS_WEAPON_ENCODED_BOMB)
	{
		weaponID = PICKUPBOTS_ITEM_BOMB;
	}
	else if (weaponID == BOSS_WEAPON_ENCODED_POTION)
	{
		weaponID = PICKUPBOTS_ITEM_POTION;
	}
	else if (weaponID == BOSS_WEAPON_NONE)
	{
		weaponID = PICKUPBOTS_ITEM_INVALID;
	}

	return weaponID;
}

static int PickupBots_UpdateBossJuice(struct MetaDataBOSS *bossMeta, int weaponID)
{
	u16 juiceFlag = bossMeta->juiceFlag;

	if ((juiceFlag & BOSS_WEAPON_RANDOM_JUICE) == 0)
	{
		sdata->bossJuiceCounter = 0;
		return weaponID;
	}

	if (sdata->bossJuiceCounter < PICKUPBOTS_BOSS_JUICE_COUNTER_MAX)
	{
		sdata->bossJuiceCounter++;
		return weaponID;
	}

	if (bossMeta->weaponType == BOSS_WEAPON_ENCODED_TNT)
	{
		weaponID = PICKUPBOTS_ITEM_TNT;

		if (bossMeta->throwFlag != BOSS_WEAPON_NORMAL)
		{
			bossMeta->throwFlag = BOSS_WEAPON_NORMAL;
			sdata->bossJuiceCounter = PICKUPBOTS_BOSS_JUICE_COUNTER_MAX;
			bossMeta->juiceFlag = juiceFlag | BOSS_WEAPON_JUICED;
			return weaponID;
		}
	}
	else if (bossMeta->weaponType == BOSS_WEAPON_ENCODED_BOMB)
	{
		weaponID = PICKUPBOTS_ITEM_BOMB;

		if ((juiceFlag & BOSS_WEAPON_JUICED) == 0)
		{
			bossMeta->juiceFlag = juiceFlag | BOSS_WEAPON_JUICED;
			sdata->bossJuiceCounter = PICKUPBOTS_BOSS_JUICE_COUNTER_MAX;
			return PICKUPBOTS_ITEM_TNT;
		}

		bossMeta->juiceFlag = juiceFlag & ~BOSS_WEAPON_JUICED;
		sdata->bossJuiceCounter = 0;
		return weaponID;
	}
	else if (bossMeta->weaponType == BOSS_WEAPON_ENCODED_POTION)
	{
		weaponID = PICKUPBOTS_ITEM_POTION;

		if (bossMeta->throwFlag != BOSS_WEAPON_NORMAL)
		{
			bossMeta->throwFlag = BOSS_WEAPON_NORMAL;
			sdata->bossJuiceCounter = PICKUPBOTS_BOSS_JUICE_COUNTER_MAX;
			bossMeta->juiceFlag |= BOSS_WEAPON_JUICED;
			return weaponID;
		}
	}
	else
	{
		return weaponID;
	}

	bossMeta->throwFlag = BOSS_WEAPON_THROW;
	sdata->bossJuiceCounter = 0;
	bossMeta->juiceFlag &= ~BOSS_WEAPON_JUICED;
	return weaponID;
}

static void PickupBots_UpdateBoss(void)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Driver *boss = gGT->drivers[1];
	struct Driver *player = gGT->drivers[0];
	struct MetaDataBOSS *bossMeta = sdata->bossWeaponMeta;

	if (((boss->botData.botFlags & BOT_FLAG_DAMAGE_ACTIVE) != 0) || ((boss->actionsFlagSet & ACTION_RACE_FINISHED) != 0) || (boss->instTntRecv != NULL) ||
	    (boss->clockReceive != 0) || (boss->botData.aiPhysics.speedLinear < PICKUPBOTS_BOSS_SPEED_MIN))
	{
		PickupBots_SetBossCooldown(bossMeta);
		return;
	}

	PickupBots_AdvanceBossMeta(boss);
	bossMeta = sdata->bossWeaponMeta;

	PickupBots_UpdateBossPathRequest(boss);

	if (sdata->bossWeaponCooldown > 0)
	{
		sdata->bossWeaponCooldown--;
		return;
	}

	PickupBots_SetBossCooldown(bossMeta);

	int weaponID = PickupBots_UpdateBossJuice(bossMeta, PickupBots_GetBossWeaponID(bossMeta));
	int throwFlag = bossMeta->throwFlag;
	int weaponFlags = (throwFlag == BOSS_WEAPON_THROW) ? PICKUPBOTS_SHOOT_FLAG_RANDOM : 0;

	if (weaponID >= 0)
	{
		u8 oldWumpa = boss->numWumpas;
		boss->numWumpas = ((bossMeta->juiceFlag & BOSS_WEAPON_JUICED) != 0) ? DRIVER_WUMPA_JUICED_COUNT : 0;
		boss->heldItemID = weaponID;

		if ((u16)(weaponID - PICKUPBOTS_ITEM_TNT) < 2)
		{
			PickupBots_PlayVoice(PICKUPBOTS_VOICELINE_MINE_DROP, boss, player);
		}
		else
		{
			weaponFlags |= PICKUPBOTS_SHOOT_FLAG_BACKWARD;
			PickupBots_PlayVoice(PICKUPBOTS_VOICELINE_BOMB, boss, player);
		}

		if (boss->heldItemID == PICKUPBOTS_ITEM_BOMB)
		{
			VehPickupItem_ShootNow(boss, PICKUPBOTS_SHOOT_ID_BOMB_MISSILE, (s16)weaponFlags);
		}
		else if ((boss->heldItemID == PICKUPBOTS_ITEM_POTION) && (weaponFlags == PICKUPBOTS_SHOOT_FLAG_RANDOM) && (gGT->levelID == OXIDE_STATION))
		{
			VehPickupItem_ShootNow(boss, weaponID, PICKUPBOTS_SHOOT_FLAG_RANDOM);
			VehPickupItem_ShootNow(boss, weaponID, PICKUPBOTS_SHOOT_FLAG_RANDOM);
		}
		else
		{
			VehPickupItem_ShootNow(boss, weaponID, (s16)weaponFlags);

			if ((boss->heldItemID == PICKUPBOTS_ITEM_TNT) && (bossMeta->throwFlag == BOSS_WEAPON_NORMAL) &&
			    (sdata->bossJuiceCounter != PICKUPBOTS_BOSS_JUICE_COUNTER_MAX))
			{
				sdata->bossJuiceCounter = PICKUPBOTS_BOSS_JUICE_COUNTER_MAX;
			}
		}

		boss->heldItemID = PICKUPBOTS_ITEM_NONE;
		boss->numWumpas = oldWumpa;
	}
}

void PickupBots_Update(void)
{
	// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800408b8-0x800414f4.
	struct GameTracker *gGT = sdata->gGT;

	if ((gGT->numBotsNextGame == 0) || (gGT->elapsedEventTime < PICKUPBOTS_UPDATE_START_DELAY))
	{
		if (gGT->gameMode1 >= 0)
		{
			return;
		}

		if (gGT->elapsedEventTime < PICKUPBOTS_UPDATE_NEGATIVE_MODE_START_DELAY)
		{
			return;
		}
	}

	if ((gGT->gameMode1 & (ADVENTURE_BOSS | END_OF_RACE)) != ADVENTURE_BOSS)
	{
		if (gGT->numPlyrCurrGame == 0)
		{
			return;
		}

		PickupBots_UpdateArcade();
		return;
	}

	PickupBots_UpdateBoss();
}
