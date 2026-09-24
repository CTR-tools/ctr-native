#include <common.h>

void PickupBots_Init(void)
{
	u16 hub;
	int lev = GAME_TRACKER->levelID;

	// get hubID of level
	hub = GAME_LEVEL_METADATA[lev].hubID;

	// If Level ID is Oxide Station
	if (lev == OXIDE_STATION)
	{
		hub = 0;
	}

	if ((s16)hub > -1)
	{
		// set pointer to boss weapon meta
		sdata->bossWeaponMeta = GAME_BOSS_WEAPON_METADATA[(s16)hub];
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


static inline void PickupBots_SetCooldown(struct Driver *bot)
{
	bot->botData.weaponCooldown = (MixRNG_Scramble() & PICKUPBOTS_COOLDOWN_RANDOM_MASK) + PICKUPBOTS_COOLDOWN_BASE_FRAMES;
}

static inline void PickupBots_PlayVoice(u32 voiceID, struct Driver *attacker, struct Driver *victim)
{
	Voiceline_RequestPlay(voiceID, GAME_CHARACTER_IDS[attacker->driverID], GAME_CHARACTER_IDS[victim->driverID]);
}

static inline void PickupBots_UpdateArcade(struct GameTracker *initialGT)
{
	register int i CTR_PSX_REGISTER("s4") = 0;

	if (initialGT->numPlyrCurrGame == 0)
	{
		return;
	}

	do
	{
		struct Driver *player = GAME_TRACKER->drivers[i];

		if (player->driverRank != 0)
		{
			struct Driver *bot = GAME_TRACKER->driversInRaceOrder[player->driverRank - 1];

#if defined(CTR_NATIVE)
			// NOTE(aalhendi): Retail can read PS1 low memory when a rank slot is empty.
			if (bot != NULL)
			{
#endif
				if (((bot->actionsFlagSet & ACTION_BOT) != 0) && ((bot->botData.botFlags & BOT_FLAG_DAMAGE_ACTIVE) == 0) &&
				    ((bot->actionsFlagSet & ACTION_RACE_FINISHED) == 0) && (bot->botData.weaponCooldown == 0) && (bot->instTntRecv == NULL) &&
				    (bot->clockReceive == 0))
				{
					register int playerX CTR_PSX_REGISTER("v1") = player->instSelf->matrix.t[0];
					register int botX CTR_PSX_REGISTER("v0") = bot->instSelf->matrix.t[0];
					register int x CTR_PSX_REGISTER("v1") = playerX - botX;
					int xSquared = x * x;
					register int playerZ CTR_PSX_REGISTER("v1") = player->instSelf->matrix.t[2];
					register int botZ CTR_PSX_REGISTER("v0") = bot->instSelf->matrix.t[2];
					register int z CTR_PSX_REGISTER("v1") = playerZ - botZ;
					int zSquared = z * z;
					register u32 range CTR_PSX_REGISTER("v0") = PICKUPBOTS_CLOSE_DIST_SQ_RANGE - 1;
					register u32 negativeBias CTR_PSX_REGISTER("v1") = (u32)-PICKUPBOTS_CLOSE_DIST_SQ_BIAS;
					register u32 distance CTR_PSX_REGISTER("a0") = xSquared + zSquared;

					if (distance + negativeBias <= range)
					{
						int rng = MixRNG_Scramble() % PICKUPBOTS_LEADING_ATTACK_ROLL_MOD;
						register struct Driver *shootBot CTR_PSX_REGISTER("a0") = bot;

						if (rng == 0)
						{
							int weaponID;

							if ((bot->lapIndex != 0) &&
							    (MixRNG_Scramble() % PICKUPBOTS_LEADING_ATTACK_POWERED_ROLL_MOD < PICKUPBOTS_LEADING_ATTACK_POWERED_ROLL_THRESHOLD))
							{
								bot->numWumpas = DRIVER_WUMPA_JUICED_COUNT;
							}

							if ((GAME_TRACKER->elapsedEventTime & 1) != 0)
							{
								register struct Driver *voiceBot CTR_PSX_REGISTER("a0") = bot;
								bot->heldItemID = PICKUPBOTS_ITEM_TNT;

								if ((player->actionsFlagSet & ACTION_BOT) == 0)
								{
									CTR_PSX_KEEP_VALUE(voiceBot);
									PickupBots_PlayVoice(PICKUPBOTS_VOICELINE_MINE_DROP, bot, player);
									{
										register struct Driver *afterVoiceBot CTR_PSX_REGISTER("a0") = bot;
										CTR_PSX_KEEP_VALUE(afterVoiceBot);
									}
								}

								weaponID = PICKUPBOTS_ITEM_TNT;
							}
							else
							{
								register struct Driver *voiceBot CTR_PSX_REGISTER("a0") = bot;
								bot->heldItemID = PICKUPBOTS_ITEM_POTION;

								if ((player->actionsFlagSet & ACTION_BOT) == 0)
								{
									CTR_PSX_KEEP_VALUE(voiceBot);
									PickupBots_PlayVoice(PICKUPBOTS_VOICELINE_MINE_DROP, bot, player);
									{
										register struct Driver *afterVoiceBot CTR_PSX_REGISTER("a0") = bot;
										CTR_PSX_KEEP_VALUE(afterVoiceBot);
									}
								}
								weaponID = PICKUPBOTS_ITEM_POTION;
							}

							VehPickupItem_ShootNow(shootBot, weaponID, 0);
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
#if defined(CTR_NATIVE)
			}
#endif
		}

		if (player->driverRank < 3)
		{
			struct Driver *bot = GAME_TRACKER->driversInRaceOrder[player->driverRank + 1];

#if defined(CTR_NATIVE)
			if (bot != NULL)
			{
#endif
				if (((bot->actionsFlagSet & ACTION_BOT) != 0) && ((bot->botData.botFlags & BOT_FLAG_DAMAGE_ACTIVE) == 0) &&
				    ((bot->actionsFlagSet & ACTION_RACE_FINISHED) == 0) && (bot->botData.weaponCooldown == 0) && (bot->instTntRecv == NULL) &&
				    (bot->clockReceive == 0) &&
				    (((int)player->lapIndex < (int)GAME_TRACKER->numLaps) ||
				     ((s32)player->distanceToFinish_curr > PICKUPBOTS_TRAILING_ATTACK_DISTANCE_TO_FINISH_MIN)))
				{
					register int playerX CTR_PSX_REGISTER("v1") = player->instSelf->matrix.t[0];
					register int botX CTR_PSX_REGISTER("v0") = bot->instSelf->matrix.t[0];
					register int x CTR_PSX_REGISTER("v1") = playerX - botX;
					int xSquared = x * x;
					register int playerZ CTR_PSX_REGISTER("v1") = player->instSelf->matrix.t[2];
					register int botZ CTR_PSX_REGISTER("v0") = bot->instSelf->matrix.t[2];
					register int z CTR_PSX_REGISTER("v1") = playerZ - botZ;
					int zSquared = z * z;
					register u32 range CTR_PSX_REGISTER("v0") = PICKUPBOTS_CLOSE_DIST_SQ_RANGE - 1;
					register u32 negativeBias CTR_PSX_REGISTER("v1") = (u32)-PICKUPBOTS_CLOSE_DIST_SQ_BIAS;
					register u32 distance CTR_PSX_REGISTER("a0") = xSquared + zSquared;

					if (distance + negativeBias <= range)
					{
						int rng = MixRNG_Scramble() % PICKUPBOTS_TRAILING_ATTACK_ROLL_MOD;
						register int weaponID CTR_PSX_REGISTER("v0");

						if (rng < PICKUPBOTS_TRAILING_ATTACK_MISSILE_THRESHOLD)
						{
							if (bot->lapIndex != (GAME_TRACKER->numLaps - 1))
							{
								weaponID = PICKUPBOTS_ITEM_MISSILE;
								goto TrailingShoot;
							}
						}
						if (rng >= PICKUPBOTS_TRAILING_ATTACK_BOMB_THRESHOLD)
						{
							goto TrailingNoShot;
						}
						weaponID = PICKUPBOTS_ITEM_BOMB;
					TrailingShoot:
						bot->heldItemID = weaponID;

						if ((player->actionsFlagSet & ACTION_BOT) == 0)
						{
							PickupBots_PlayVoice(PICKUPBOTS_VOICELINE_MISSILE, bot, player);
						}

						VehPickupItem_ShootNow(bot, PICKUPBOTS_SHOOT_ID_BOMB_MISSILE, 0);
						PickupBots_SetCooldown(bot);

					TrailingNoShot:
						bot->heldItemID = PICKUPBOTS_ITEM_NONE;
					}
				}
#if defined(CTR_NATIVE)
			}
#endif
		}
		i++;
	} while (i < GAME_TRACKER_RELOAD()->numPlyrCurrGame);
}

static inline void PickupBots_SetBossCooldown(struct MetaDataBOSS *bossMeta)
{
	*(volatile s16 *)&sdata->bossWeaponCooldown = (RngDeadCoed(&GAME_ADV_RNG) & PICKUPBOTS_BOSS_COOLDOWN_RANDOM_MASK) +
	                                              (bossMeta->weaponCooldown + PICKUPBOTS_BOSS_COOLDOWN_BASE_FRAMES) +
	                                              ((s8)GAME_ADV_PROGRESS.timesLostBossRace[GAME_TRACKER->bossID] * PICKUPBOTS_BOSS_LOSS_COOLDOWN_STEP);
}

static inline struct MetaDataBOSS *PickupBots_GetInitialBossMeta(void)
{
	struct GameTracker *gGT = GAME_TRACKER;

	if (gGT->levelID == OXIDE_STATION)
	{
		return GAME_BOSS_WEAPON_METADATA[0];
	}

	return GAME_BOSS_WEAPON_METADATA[GAME_LEVEL_METADATA[gGT->levelID].hubID];
}

static inline void PickupBots_UpdateBoss(struct GameTracker *gGT)
{
	struct Driver *boss = gGT->drivers[1];
	struct Driver *player = gGT->drivers[0];
	register struct MetaDataBOSS *bossMeta CTR_PSX_REGISTER("s1") = sdata->bossWeaponMeta;
	register int weaponID CTR_PSX_REGISTER("s0");
	register int encodedWeaponType CTR_PSX_REGISTER("v1");
	int newWumpa;
	int throwFlag;
	int weaponFlags;

	if (((boss->botData.botFlags & BOT_FLAG_DAMAGE_ACTIVE) != 0) || ((boss->actionsFlagSet & ACTION_RACE_FINISHED) != 0) || (boss->instTntRecv != NULL) ||
	    (boss->clockReceive != 0) || (boss->botData.aiPhysics.speedLinear < PICKUPBOTS_BOSS_SPEED_MIN))
	{
		goto BossCooldown;
	}

	{
		struct MetaDataBOSS *nextMeta = &bossMeta[1];

		if (nextMeta->throwFlag == 0)
		{
			int threshold = gGT->level1->ptr_restart_points[bossMeta->trackCheckpoint].distToFinish << PICKUPBOTS_BOSS_CHECKPOINT_DISTANCE_SHIFT;

			if (threshold < (int)boss->distanceToFinish_curr)
			{
				s16 preservedThrow = -1;

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
				s16 preservedThrow = -1;

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

	if (bossMeta->pathChangeDisabled == 0)
	{
		s16 pathTimer = sdata->bossPathRequestTimer;
		u16 pathTimerUnsigned = sdata->bossPathRequestTimer;

		if (pathTimer == PICKUPBOTS_BOSS_PATH_REQUEST_FRAMES)
		{
			if ((*(volatile u32 *)&boss->botData.botFlags & BOT_FLAG_BOSS_PATH_ACTIVE) != 0)
			{
				goto PathRequestDone;
			}

			if (sdata->bossPathRequestPhase != 0)
			{
				if (boss->botData.botPath == 0)
				{
					goto RequestPathOne;
				}
				if (boss->botData.botPath == 1)
				{
					u32 flags = boss->botData.botFlags;
					boss->botData.desiredPath_BossOnly = 2;
					sdata->bossPathRequestTimer = 0;
					// NOTE(aalhendi): Keep this reset before the shared flag-store tail.
					*(volatile s16 *)&sdata->bossPathRequestPhase = 0;
					boss->botData.botFlags = flags | BOT_FLAG_BOSS_PATH_REQUESTED;
				}
				goto PathRequestDone;
			}

			if (boss->botData.botPath != 2)
			{
				goto RequestPathZero;
			}

		RequestPathOne:
		{
			u32 flags = *(volatile u32 *)&boss->botData.botFlags;
			boss->botData.desiredPath_BossOnly = 1;
			sdata->bossPathRequestTimer = 0;
			boss->botData.botFlags = flags | BOT_FLAG_BOSS_PATH_REQUESTED;
		}
			goto PathRequestDone;

		RequestPathZero:
		{
			register s32 path CTR_PSX_REGISTER("v1") = boss->botData.botPath;
			if (path != 1)
			{
				goto PathRequestDone;
			}
			{
				u32 flags = *(volatile u32 *)&boss->botData.botFlags;
				boss->botData.desiredPath_BossOnly = 0;
				sdata->bossPathRequestTimer = 0;
				sdata->bossPathRequestPhase = path;
				boss->botData.botFlags = flags | BOT_FLAG_BOSS_PATH_REQUESTED;
			}
		}
		}
		else if ((boss->botData.botFlags & BOT_FLAG_BOSS_PATH_REQUESTED) == 0)
		{
			sdata->bossPathRequestTimer = pathTimerUnsigned + 1;
		}

	PathRequestDone:;
	}

	{
		s16 cooldown = sdata->bossWeaponCooldown;
		u16 cooldownUnsigned = sdata->bossWeaponCooldown;

		if (cooldown > 0)
		{
			*(volatile s16 *)&sdata->bossWeaponCooldown = cooldownUnsigned - 1;
			return;
		}
	}

	weaponFlags = 0;
	sdata->bossWeaponCooldown = (RngDeadCoed(&GAME_ADV_RNG) & PICKUPBOTS_BOSS_COOLDOWN_RANDOM_MASK) +
	                            (bossMeta->weaponCooldown + PICKUPBOTS_BOSS_COOLDOWN_BASE_FRAMES) +
	                            ((s8)GAME_ADV_PROGRESS.timesLostBossRace[GAME_TRACKER->bossID] * PICKUPBOTS_BOSS_LOSS_COOLDOWN_STEP);

	weaponID = bossMeta->weaponType;
	encodedWeaponType = weaponID;
	if (encodedWeaponType == BOSS_WEAPON_ENCODED_TNT)
	{
		weaponID = PICKUPBOTS_ITEM_TNT;
	}
	else if (encodedWeaponType == BOSS_WEAPON_ENCODED_BOMB)
	{
		weaponID = PICKUPBOTS_ITEM_BOMB;
	}
	else if (encodedWeaponType == BOSS_WEAPON_ENCODED_POTION)
	{
		weaponID = PICKUPBOTS_ITEM_POTION;
	}
	else if (encodedWeaponType == BOSS_WEAPON_NONE)
	{
		weaponID = PICKUPBOTS_ITEM_INVALID;
	}
	{
		u16 juiceFlag = bossMeta->juiceFlag;
		s16 juiceCounter;
		u16 juiceCounterUnsigned;

		if ((juiceFlag & BOSS_WEAPON_RANDOM_JUICE) == 0)
		{
			goto BossJuiceNoRandom;
		}

		juiceCounter = sdata->bossJuiceCounter;
		juiceCounterUnsigned = sdata->bossJuiceCounter;
		if (juiceCounter < PICKUPBOTS_BOSS_JUICE_COUNTER_MAX)
		{
			goto BossJuiceIncrement;
		}

		if (bossMeta->weaponType == BOSS_WEAPON_ENCODED_TNT)
		{
#if defined(CTR_NATIVE)
			if (bossMeta->throwFlag != BOSS_WEAPON_NORMAL)
			{
				weaponID = PICKUPBOTS_ITEM_TNT;
#else
			// NOTE(aalhendi): Retail sets the TNT ID in this branch's delay slot.
			// GCC 2.8.1 places the juice copy there instead, so this one PSX
			// branch targets the reset block while the native path stays in C.
			__asm__ volatile(
				"lbu $3,%1\n\t"
				"li $2,%2\n\t"
				".set\tnoreorder\n\t"
				"beq $3,$2,1f\n\t"
				"addiu %0,$0,%3\n\t"
				".set\treorder"
				: "=r"(weaponID)
				: "m"(bossMeta->throwFlag), "I"(BOSS_WEAPON_NORMAL), "I"(PICKUPBOTS_ITEM_TNT)
				: "v0", "v1", "memory");
			{
#endif
				register u16 newJuice CTR_PSX_REGISTER("v0") = juiceFlag;
				register s16 counter CTR_PSX_REGISTER("v1");
				CTR_PSX_KEEP_VALUE_RELAXED(newJuice);
				counter = PICKUPBOTS_BOSS_JUICE_COUNTER_MAX;
				newJuice |= BOSS_WEAPON_JUICED;
				bossMeta->throwFlag = weaponID;
				sdata->bossJuiceCounter = counter;
				bossMeta->juiceFlag = newJuice;
				goto BossJuiceDone;
			}
		}
		else if (bossMeta->weaponType == BOSS_WEAPON_ENCODED_BOMB)
		{
			if ((juiceFlag & BOSS_WEAPON_JUICED) == 0)
			{
				bossMeta->juiceFlag = juiceFlag | BOSS_WEAPON_JUICED;
				*(volatile s16 *)&sdata->bossJuiceCounter = PICKUPBOTS_BOSS_JUICE_COUNTER_MAX;
				weaponID = PICKUPBOTS_ITEM_TNT;
				goto BossJuiceDone;
			}

			weaponID = PICKUPBOTS_ITEM_BOMB;
			bossMeta->juiceFlag = juiceFlag & ~BOSS_WEAPON_JUICED;
			sdata->bossJuiceCounter = 0;
			goto BossJuiceDone;
		}
		else if (bossMeta->weaponType == BOSS_WEAPON_ENCODED_POTION)
		{
			weaponID = PICKUPBOTS_ITEM_POTION;

			if (bossMeta->throwFlag != BOSS_WEAPON_NORMAL)
			{
				u16 currentJuice = *(volatile u16 *)&bossMeta->juiceFlag;
				bossMeta->throwFlag = BOSS_WEAPON_NORMAL;
				sdata->bossJuiceCounter = PICKUPBOTS_BOSS_JUICE_COUNTER_MAX;
				bossMeta->juiceFlag = currentJuice | BOSS_WEAPON_JUICED;
				goto BossJuiceDone;
			}
		}
		else
		{
			goto BossJuiceDone;
		}

		{
			u16 resetJuice;
#if !defined(CTR_NATIVE)
			// NOTE(aalhendi): Local label 1 is the TNT branch target and stays
			// valid if the enclosing inline function is ever emitted twice.
			__asm__ volatile("1:");
#endif
			// NOTE(aalhendi): Reload here because the PSX branch above also
			// reaches this block outside the compiler's C control-flow graph.
			resetJuice = *(volatile u16 *)&bossMeta->juiceFlag;
			bossMeta->throwFlag = BOSS_WEAPON_THROW;
			sdata->bossJuiceCounter = 0;
			bossMeta->juiceFlag = resetJuice & ~BOSS_WEAPON_JUICED;
		}
		goto BossJuiceDone;

BossJuiceIncrement:
		*(volatile s16 *)&sdata->bossJuiceCounter = juiceCounterUnsigned + 1;
		goto BossJuiceDone;

	BossJuiceNoRandom:
		sdata->bossJuiceCounter = 0;
	}

BossJuiceDone:
	throwFlag = bossMeta->throwFlag;
	if (throwFlag == BOSS_WEAPON_THROW)
	{
		weaponFlags = PICKUPBOTS_SHOOT_FLAG_RANDOM;
	}

	newWumpa = 0;
	if ((s16)weaponID >= 0)
	{
		register u8 rawWumpa CTR_PSX_REGISTER("v1") = *(volatile u8 *)&boss->numWumpas;
		register int oldWumpa CTR_PSX_REGISTER("s4") = (s8)rawWumpa;
		if ((bossMeta->juiceFlag & BOSS_WEAPON_JUICED) != 0)
		{
			newWumpa = DRIVER_WUMPA_JUICED_COUNT;
		}
		boss->numWumpas = newWumpa;
		boss->heldItemID = weaponID;

		if ((u16)(weaponID - PICKUPBOTS_ITEM_TNT) < 2)
		{
			PickupBots_PlayVoice(PICKUPBOTS_VOICELINE_MINE_DROP, boss, player);
		}
		else
		{
			register u32 bombVoice CTR_PSX_REGISTER("a0") = PICKUPBOTS_VOICELINE_BOMB;
			register const s16 *characterIDs CTR_PSX_REGISTER("a2") = GAME_CHARACTER_IDS;
			register u32 bossID CTR_PSX_REGISTER("v1") = boss->driverID;
			register u32 playerID CTR_PSX_REGISTER("v0") = player->driverID;
			CTR_PSX_KEEP_VALUE(bombVoice);
			CTR_PSX_KEEP_VALUE(bossID);
			CTR_PSX_KEEP_VALUE(playerID);
			weaponFlags |= PICKUPBOTS_SHOOT_FLAG_BACKWARD;
			Voiceline_RequestPlay(bombVoice, characterIDs[bossID], characterIDs[playerID]);
		}

		if (boss->heldItemID == PICKUPBOTS_ITEM_BOMB)
		{
			VehPickupItem_ShootNow(boss, PICKUPBOTS_SHOOT_ID_BOMB_MISSILE, (s16)weaponFlags);
		}
		else if ((boss->heldItemID == PICKUPBOTS_ITEM_POTION) && ((s16)weaponFlags == PICKUPBOTS_SHOOT_FLAG_RANDOM) &&
		         (GAME_TRACKER_RELOAD()->levelID == OXIDE_STATION))
		{
			register s16 repeatWeaponID CTR_PSX_REGISTER("s0") = weaponID;
			VehPickupItem_ShootNow(boss, repeatWeaponID, PICKUPBOTS_SHOOT_FLAG_RANDOM);
			VehPickupItem_ShootNow(boss, repeatWeaponID, PICKUPBOTS_SHOOT_FLAG_RANDOM);
		}
		else
		{
			VehPickupItem_ShootNow(boss, (s16)weaponID, (s16)weaponFlags);

			if ((boss->heldItemID == PICKUPBOTS_ITEM_TNT) && (bossMeta->throwFlag == BOSS_WEAPON_NORMAL) &&
			    (sdata->bossJuiceCounter != PICKUPBOTS_BOSS_JUICE_COUNTER_MAX))
			{
				sdata->bossJuiceCounter = PICKUPBOTS_BOSS_JUICE_COUNTER_MAX;
			}
		}

		boss->heldItemID = PICKUPBOTS_ITEM_NONE;
		boss->numWumpas = oldWumpa;
	}
	return;

BossCooldown:
	PickupBots_SetBossCooldown(bossMeta);
}

void PickupBots_Update(void)
{
	register struct GameTracker *initialGT CTR_PSX_REGISTER("v1") = GAME_TRACKER;
	struct GameTracker *gGT;

	if ((initialGT->numBotsNextGame == 0) || (initialGT->elapsedEventTime < PICKUPBOTS_UPDATE_START_DELAY))
	{
		if (initialGT->gameMode1 >= 0)
		{
			return;
		}

		if (initialGT->elapsedEventTime < PICKUPBOTS_UPDATE_NEGATIVE_MODE_START_DELAY)
		{
			return;
		}
	}

	gGT = GAME_TRACKER_RELOAD();
	if ((gGT->gameMode1 & (ADVENTURE_BOSS | END_OF_RACE)) == ADVENTURE_BOSS)
	{
		PickupBots_UpdateBoss(gGT);
	}
	else
	{
		PickupBots_UpdateArcade(gGT);
	}
}
