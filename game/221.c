#include <common.h>

#include <platform/native_log.h>

enum CrystalChallengeEndMenuConstants
{
	CC_FLY_IN_FRAMES = 0x14,
	CC_SCREEN_DEPTH = 0x200,
	CC_TOKEN_GROW_LIMIT = 0x2001,
	CC_TOKEN_GROW_STEP = 0x200,
	CC_CONFIRM_BUTTON_MASK = BTN_CROSS_one | BTN_CIRCLE,
	CC_COOP_RESULTS_Y_OFFSET = -15,
	// NOTE(aalhendi): Native sentinel for retail stack residue that has not
	// been modeled yet.
	CC_UNMODELED_REWARD_OFFSET = -0x400000,
};

#define CC_BATTLE_TRACK_PURPLE_TOKEN_OFFSETS 3, 1, -1, 0, -1, 2, -1

global_variable const s16 s_battleTrackPurpleTokenOffset[LAB_BASEMENT - NITRO_COURT + 1] = {
    CC_BATTLE_TRACK_PURPLE_TOKEN_OFFSETS,
};

extern struct RectMenu menuArcadeRaceEnd;

// NOTE(aalhendi): The overlay-private matching ABI overrides these defaults
// before compiling the retail artifact.
#define CC_HUD_CRYSTAL             (sdata->ptrHudCrystal)
#define CC_MENU_CRYSTAL            (sdata->ptrMenuCrystal)

static void CC_EndEvent_LogUnmodeledRewardOffset(const s32 levelID)
{
	static u64 s_loggedLevelMask[2];
	static b32 s_loggedOutOfRangeLevel;

	if (levelID >= 0 && levelID < 128)
	{
		const u64 levelBit = (u64)1 << (levelID & 0x3f);
		u64 *levelMask = &s_loggedLevelMask[levelID >> 6];

		if ((*levelMask & levelBit) != 0)
		{
			return;
		}

		*levelMask |= levelBit;
	}
	else
	{
		if (s_loggedOutOfRangeLevel)
		{
			return;
		}

		s_loggedOutOfRangeLevel = true;
	}

	Platform_LogWarn("[CTR 221] unmodeled CC reward residue: levelID=%d\n", levelID);
}

static s32 CC_EndEvent_GetRewardOffset(struct GameTracker *gGT)
{
	s32 levelID = gGT->levelID;
	s32 frustumSavedCameraZ = PushBuffer_GetFrustumSavedCameraZ();

	if (levelID == DINGO_CANYON)
	{
		// NOTE(aalhendi): Retail does not read pushBuffer[0].pos.z directly
		// here. This underreads the battle-token stack table and aliases the
		// earlier PushBuffer_SetFrustumPlane stack save of
		// PushBuffer_UpdateFrustum's s5 camera-Z value.
		return (s16)frustumSavedCameraZ;
	}

	if (levelID == DRAGON_MINES)
	{
		// Same saved s5 word as Dingo, read from its high halfword.
		return (s16)CTR_MipsSra(frustumSavedCameraZ, 16);
	}

	if (levelID == BLIZZARD_BLUFF)
	{
		// Low halfword of saved s6 from the same producer; retail s6 is the
		// scratchpad base 0x1f800000, so this reads as zero.
		return 0;
	}

	if (levelID < NITRO_COURT || levelID > LAB_BASEMENT)
	{
		// NOTE(aalhendi): Retail would keep reading the adjacent stack halfwords.
		// Native only models audited residue producers. We just log unmodeled UB
		CC_EndEvent_LogUnmodeledRewardOffset(levelID);
		return CC_UNMODELED_REWARD_OFFSET;
	}

	return s_battleTrackPurpleTokenOffset[levelID - NITRO_COURT];
}

static u32 CC_EndEvent_GetRewardBitMask(s32 rewardBit)
{
	return 1u << ((u32)rewardBit & 0x1f);
}

CTR_STATIC_ASSERT(OFFSETOF(struct sData, gameSave.options) + sizeof(struct GameOptions) == OFFSETOF(struct sData, advProgress));

static u8 *CC_EndEvent_GetNativeRewardWordBytes(s32 rewardBit)
{
	s32 wordIndex = CTR_MipsSra(rewardBit, 5);
	s64 rewardByteOffset = (s64)OFFSETOF(struct sData, advProgress.rewards) + (s64)wordIndex * (s64)sizeof(u32);
	s64 windowStart = (s64)OFFSETOF(struct sData, gameSave.options);
	s64 windowEnd = (s64)OFFSETOF(struct sData, advProgress) + (s64)sizeof(struct AdvProgress);

	// NOTE(aalhendi): Retail applies the unchecked residue index to
	// advProgress.rewards, so Dingo Bingo can touch adjacent saved options words.
	// Native bounds that retail window without doing host out-of-bounds access.
	if ((rewardByteOffset < windowStart) || (rewardByteOffset > windowEnd - (s32)sizeof(u32)))
	{
		return NULL;
	}

	return (u8 *)sdata + (s32)rewardByteOffset;
}

static b32 CC_EndEvent_HasRewardBit(struct AdvProgress *adv, s32 rewardBit)
{
	(void)adv;
	u8 *wordBytes = CC_EndEvent_GetNativeRewardWordBytes(rewardBit);
	if (wordBytes == NULL)
	{
		return true;
	}

	u32 rewardWord;
	memcpy(&rewardWord, wordBytes, sizeof(rewardWord));
	return (rewardWord & CC_EndEvent_GetRewardBitMask(rewardBit)) != 0;
}

static void CC_EndEvent_UnlockRewardBit(struct AdvProgress *adv, s32 rewardBit)
{
	(void)adv;
	u8 *wordBytes = CC_EndEvent_GetNativeRewardWordBytes(rewardBit);
	if (wordBytes == NULL)
	{
		return;
	}

	u32 rewardWord;
	memcpy(&rewardWord, wordBytes, sizeof(rewardWord));
	rewardWord |= CC_EndEvent_GetRewardBitMask(rewardBit);
	memcpy(wordBytes, &rewardWord, sizeof(rewardWord));
}

void CC_EndEvent_DrawMenu()
{
	struct GameTracker *comparisonGT;
	s16 pos[2];
	s32 resultStringIndex;
	b32 didWin;
	register b32 canAward;
	register s32 currentFrames;
	s32 tokenRewardBit;
	SVec2 promptPos;
	s16 resultsYOffset;

	comparisonGT = GAME_TRACKER_RELOAD();
	UI_RaceEnd_SetMenuLayout(&data.menuRetryExit, &promptPos);
	resultsYOffset = MainGameEnd_IsCoopRace(comparisonGT) ? CC_COOP_RESULTS_Y_OFFSET : 0;

		if (UI_Crystal_CountCollected(comparisonGT) >= comparisonGT->numCrystalsInLEV)
		{
			didWin = true;
			resultStringIndex = LNG_YOU_WIN;
		}
		else
		{
			didWin = false;
			resultStringIndex = LNG_TRY_AGAIN;
		}

		if (GAME_FRAMES_SINCE_RACE_ENDED < CTR_SECONDS_TO_FRAMES(30))
		{
			GAME_FRAMES_SINCE_RACE_ENDED++;
		}

		// NOTE(aalhendi): Menu-storage can enter this flow in tracks without
		// crystal HUD instances; keep reward/menu logic and skip missing models.
		if (CC_HUD_CRYSTAL != NULL)

		{
			CC_HUD_CRYSTAL->flags = CC_HUD_CRYSTAL->flags | HIDE_MODEL;
		}

		currentFrames = GAME_FRAMES_SINCE_RACE_ENDED;

		// fly in from left
		UI_Lerp2D_Linear(&pos[0], -0x64, 0x18, 0x100, 0x18, currentFrames, CC_FLY_IN_FRAMES);
		pos[1] += resultsYOffset;
		DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_TIME_REMAINING], pos[0], pos[1], FONT_BIG, (JUSTIFY_CENTER | ORANGE));
		UI_DrawLimitClock(pos[0] - 0x33, pos[1] + 0x11, FONT_BIG);

		// fly in from right
		UI_Lerp2D_Linear(&pos[0], 0x264, 0x56, 0xcd, 0x56, GAME_FRAMES_SINCE_RACE_ENDED, CC_FLY_IN_FRAMES);
		pos[1] += resultsYOffset;

		// Crystal count
		if (CC_MENU_CRYSTAL != NULL)

		{
			CC_MENU_CRYSTAL->matrix.t[0] = UI_ConvertX_2(pos[0], CC_SCREEN_DEPTH);
			CC_MENU_CRYSTAL->matrix.t[1] = UI_ConvertY_2(pos[1], CC_SCREEN_DEPTH);
		}
		UI_DrawNumCrystal(pos[0] + 0xf, pos[1] - 0x10);

		// YOU WIN, or TRY AGAIN
		DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[resultStringIndex], pos[0] + 0x33, pos[1] + 8, FONT_BIG, (JUSTIFY_CENTER | ORANGE));

		canAward = didWin && (comparisonGT->gameMode1 & ADVENTURE_MODE) != 0;
		if (!canAward)
		{
			goto DrawAlreadyAwardedMenu;
		}

		tokenRewardBit = CC_EndEvent_GetRewardOffset(GAME_TRACKER) + ADV_REWARD_FIRST_PURPLE_TOKEN;
		if (CC_EndEvent_HasRewardBit(&GAME_ADV_PROGRESS, tokenRewardBit))
		{
			goto DrawAlreadyAwardedMenu;
		}

		{
			UI_Lerp2D_Linear(&pos[0], -0x64, 0xA2, 0x100, 0xA2, GAME_FRAMES_SINCE_RACE_ENDED, CC_FLY_IN_FRAMES);

			DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_CTR_TOKEN_AWARDED], pos[0], pos[1], FONT_BIG,
			                   (GAME_TRACKER->timer != 0) ? (s16)(JUSTIFY_CENTER | ORANGE) : (s16)(JUSTIFY_CENTER | WHITE));
			if (GAME_TOKEN != NULL)
			{
				GAME_TOKEN->flags &= ~(HIDE_MODEL);
				GAME_TOKEN->matrix.t[0] = UI_ConvertX_2(pos[0], CC_SCREEN_DEPTH);
				GAME_TOKEN->matrix.t[1] = UI_ConvertY_2(pos[1] - 0x18, CC_SCREEN_DEPTH);
			}

			if (GAME_FRAMES_SINCE_RACE_ENDED == CTR_SECONDS_TO_FRAMES(1))
			{
				OtherFX_Play(0x67, 1);
			}

			if (GAME_FRAMES_SINCE_RACE_ENDED > CTR_SECONDS_TO_FRAMES(1))
			{
				struct Instance *token = GAME_TOKEN;

				if (token != NULL)
				{
					if (token->scale.x < CC_TOKEN_GROW_LIMIT)
					{
						token->scale.x += CC_TOKEN_GROW_STEP;
						token->scale.y += CC_TOKEN_GROW_STEP;
						token->scale.z += CC_TOKEN_GROW_STEP;
					}
				}
			}

			DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_PRESS_TO_CONTINUE], promptPos.x, promptPos.y, FONT_BIG, (JUSTIFY_CENTER | ORANGE));

			// if still waiting to press X/O, quit function
			if ((GAME_ANY_PLAYER_TAP & CC_CONFIRM_BUTTON_MASK) == 0)
			{
				return;
			}

			// if pressed X/O,
			// unlock token and leave level

			RECTMENU_ClearInput();
			GAME_FRAMES_SINCE_RACE_ENDED = 0;

			{
				struct GameTracker *returnGT;
				u32 *addConfig;

				GAME_REMOVE_CONFIG_0 |= CRYSTAL_CHALLENGE;
				returnGT = GAME_TRACKER;
				addConfig = &GAME_ADD_CONFIG_0;
				CC_EndEvent_UnlockRewardBit(&GAME_ADV_PROGRESS, tokenRewardBit);
				*addConfig |= ADVENTURE_ARENA;
				MainRaceTrack_RequestLoad(returnGT->prevLEV); // Adventure hub.
			}

			return;
		}

	DrawAlreadyAwardedMenu:
	{
		if ((GAME_MENU_READY & 1) != 0)
		{
			return;
		}

		DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_PRESS_TO_CONTINUE], promptPos.x, promptPos.y, FONT_BIG, (JUSTIFY_CENTER | ORANGE));

		if ((GAME_ANY_PLAYER_TAP & CC_CONFIRM_BUTTON_MASK) == 0)
		{
			return;
		}

		RECTMENU_ClearInput();
		RECTMENU_Show((comparisonGT->gameMode1 & ARCADE_MODE) != 0 ? &menuArcadeRaceEnd : &data.menuRetryExit);
		GAME_MENU_READY |= 1;
	}
}

