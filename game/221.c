#include <common.h>

#if defined(CTR_NATIVE)
#include <platform/native_log.h>
#endif

enum CrystalChallengeEndMenuConstants
{
	CC_FLY_IN_FRAMES = 0x14,
	CC_SCREEN_DEPTH = 0x200,
	CC_TOKEN_GROW_LIMIT = 0x2001,
	CC_TOKEN_GROW_STEP = 0x200,
	CC_CONFIRM_BUTTON_MASK = BTN_CROSS_one | BTN_CIRCLE,
	CC_UNMODELED_REWARD_OFFSET = -0x400000, // NOTE(aalhendi): see CC_EndEvent_LogUnmodeledRewardOffset
};

global_variable const s16 s_battleTrackPurpleTokenOffset[LAB_BASEMENT - NITRO_COURT + 1] = {
    [NITRO_COURT - NITRO_COURT] = 3,     // Citadel City
    [RAMPAGE_RUINS - NITRO_COURT] = 1,   // Lost Ruins
    [PARKING_LOT - NITRO_COURT] = -1,    // not used in any hub
    [SKULL_ROCK - NITRO_COURT] = 0,      // N. Sanity Beach
    [THE_NORTH_BOWL - NITRO_COURT] = -1, // not used in any hub
    [ROCKY_ROAD - NITRO_COURT] = 2,      // Glacier Park
    [LAB_BASEMENT - NITRO_COURT] = -1,   // not used in any hub
};

extern struct RectMenu menu221;

#if defined(CTR_NATIVE)
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
#endif

static s32 CC_EndEvent_GetRewardOffset(struct GameTracker *gGT)
{
	s32 levelID = gGT->levelID;
#if defined(CTR_NATIVE)
	s32 frustumSavedCameraZ = PushBuffer_GetFrustumSavedCameraZ();
#else
	s32 frustumSavedCameraZ = gGT->pushBuffer[0].pos.z;
#endif

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

#if defined(CTR_NATIVE)
	if (levelID < NITRO_COURT || levelID > LAB_BASEMENT)
	{
		// NOTE(aalhendi): Retail would keep reading the adjacent stack halfwords.
		// Native only models audited residue producers. We just log unmodeled UB
		CC_EndEvent_LogUnmodeledRewardOffset(levelID);
		return CC_UNMODELED_REWARD_OFFSET;
	}
#endif

	return s_battleTrackPurpleTokenOffset[levelID - NITRO_COURT];
}

static u32 CC_EndEvent_GetRewardBitMask(s32 rewardBit)
{
	return 1u << ((u32)rewardBit & 0x1f);
}

#if defined(CTR_NATIVE)
CTR_STATIC_ASSERT(OFFSETOF(struct sData, gameOptions) + sizeof(struct GameOptions) == OFFSETOF(struct sData, advProgress));

static u8 *CC_EndEvent_GetNativeRewardWordBytes(s32 rewardBit)
{
	s32 wordIndex = CTR_MipsSra(rewardBit, 5);
	s64 rewardByteOffset = (s64)OFFSETOF(struct sData, advProgress.rewards) + (s64)wordIndex * (s64)sizeof(u32);
	s64 windowStart = (s64)OFFSETOF(struct sData, gameOptions);
	s64 windowEnd = (s64)OFFSETOF(struct sData, advProgress) + (s64)sizeof(struct AdvProgress);

	// NOTE(aalhendi): Retail applies the unchecked residue index to
	// advProgress.rewards, so Dingo Bingo can touch adjacent gameOptions words.
	// Native bounds that retail window without doing host out-of-bounds access.
	if ((rewardByteOffset < windowStart) || (rewardByteOffset > windowEnd - (s32)sizeof(u32)))
	{
		return NULL;
	}

	return (u8 *)sdata + (s32)rewardByteOffset;
}
#endif

static b32 CC_EndEvent_HasRewardBit(struct AdvProgress *adv, s32 rewardBit)
{
#if defined(CTR_NATIVE)
	(void)adv;
	u8 *wordBytes = CC_EndEvent_GetNativeRewardWordBytes(rewardBit);
	if (wordBytes == NULL)
	{
		return true;
	}

	u32 rewardWord;
	memcpy(&rewardWord, wordBytes, sizeof(rewardWord));
	return (rewardWord & CC_EndEvent_GetRewardBitMask(rewardBit)) != 0;
#else
	return CHECK_ADV_BIT(adv->rewards, rewardBit) != 0;
#endif
}

static void CC_EndEvent_UnlockRewardBit(struct AdvProgress *adv, s32 rewardBit)
{
#if defined(CTR_NATIVE)
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
#else
	UNLOCK_ADV_BIT(adv->rewards, rewardBit);
#endif
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8009f710-0x8009fbec for the retail path.
void CC_EndEvent_DrawMenu()
{
	struct GameTracker *gGT = sdata->gGT;
	struct Driver *driver = gGT->drivers[0];
	SVec2 pos;
	s32 tokenRewardOffset = CC_EndEvent_GetRewardOffset(gGT);

	s32 tokenRewardBit = tokenRewardOffset + ADV_REWARD_FIRST_PURPLE_TOKEN;
	struct AdvProgress *adv = &sdata->advProgress;
	b32 didLose = driver->numCrystals < gGT->numCrystalsInLEV;
	s32 elapsedFrames = sdata->framesSinceRaceEnded;

	if (elapsedFrames < CTR_SECONDS_TO_FRAMES(30))
	{
		elapsedFrames++;
	}

	sdata->framesSinceRaceEnded = elapsedFrames;
#if defined(CTR_NATIVE)
	// NOTE(aalhendi): Menu-storage can enter this flow in tracks without
	// crystal HUD instances; keep reward/menu logic and skip missing models.
	if (sdata->ptrHudCrystal != NULL)
#endif
	{
		sdata->ptrHudCrystal->flags |= HIDE_MODEL;
	}

	// fly in from left
	UI_Lerp2D_Linear(pos.v, -0x64, 0x18, 0x100, 0x18, elapsedFrames, CC_FLY_IN_FRAMES);
	DecalFont_DrawLine(sdata->lngStrings[LNG_TIME_REMAINING], pos.x, pos.y, FONT_BIG, (JUSTIFY_CENTER | ORANGE));
	UI_DrawLimitClock(pos.x - 0x33, pos.y + 0x11, FONT_BIG);

	// fly in from right
	UI_Lerp2D_Linear(pos.v, 0x264, 0x56, 0xcd, 0x56, elapsedFrames, CC_FLY_IN_FRAMES);

	// Crystal count
#if defined(CTR_NATIVE)
	if (sdata->ptrMenuCrystal != NULL)
#endif
	{
		sdata->ptrMenuCrystal->matrix.t[0] = UI_ConvertX_2(pos.x, CC_SCREEN_DEPTH);
		sdata->ptrMenuCrystal->matrix.t[1] = UI_ConvertY_2(pos.y, CC_SCREEN_DEPTH);
	}
	UI_DrawNumCrystal(pos.x + 0xf, pos.y - 0x10, driver);

	s32 resultStringIndex = LNG_YOU_WIN;
	if (didLose)
	{
		resultStringIndex = LNG_TRY_AGAIN;
	}

	// YOU WIN, or TRY AGAIN
	DecalFont_DrawLine(sdata->lngStrings[resultStringIndex], pos.x + 0x33, pos.y + 8, FONT_BIG, (JUSTIFY_CENTER | ORANGE));

	// if a token is not newly-unlocked
	if (didLose || CC_EndEvent_HasRewardBit(adv, tokenRewardBit))
	{
		// If you pressed X/O to continue, quit function
		if ((sdata->menuReadyToPass & 1) != 0)
		{
			return;
		}

		DecalFont_DrawLine(sdata->lngStrings[LNG_PRESS_TO_CONTINUE], 0x100, 0xbe, FONT_BIG, (JUSTIFY_CENTER | ORANGE));

		if ((sdata->AnyPlayerTap & CC_CONFIRM_BUTTON_MASK) == 0)
		{
			return;
		}

		RECTMENU_ClearInput();
		RECTMENU_Show(&menu221); // Retry / Exit To Map menu
		sdata->menuReadyToPass |= 1;
		return;
	}

	// == if a token is newly-unlocked ==

	struct Instance *token = sdata->ptrToken;
	s32 color = (JUSTIFY_CENTER | ORANGE);
	if (gGT->timer == 0)
	{
		color = (JUSTIFY_CENTER | WHITE);
	}

	UI_Lerp2D_Linear(pos.v, -0x64, 0xA2, 0x100, 0xA2, elapsedFrames, CC_FLY_IN_FRAMES);

	DecalFont_DrawLine(sdata->lngStrings[LNG_CTR_TOKEN_AWARDED], pos.x, pos.y, FONT_BIG, color);
#if defined(CTR_NATIVE)
	if (token != NULL)
#endif
	{
		token->flags &= ~(HIDE_MODEL);
		token->matrix.t[0] = UI_ConvertX_2(pos.x, CC_SCREEN_DEPTH);
		token->matrix.t[1] = UI_ConvertY_2(0xA2 - 0x18, CC_SCREEN_DEPTH);
	}

	if (elapsedFrames > CTR_SECONDS_TO_FRAMES(1))
	{
#if defined(CTR_NATIVE)
		if (token != NULL)
#endif
		{
			if (token->scale.x < CC_TOKEN_GROW_LIMIT)
			{
				token->scale.x += CC_TOKEN_GROW_STEP;
				token->scale.y += CC_TOKEN_GROW_STEP;
				token->scale.z += CC_TOKEN_GROW_STEP;
			}
		}
	}
	else if (elapsedFrames == CTR_SECONDS_TO_FRAMES(1))
	{
		// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8009fa24-0x8009fa2c for crystal token unlock SFX.
		OtherFX_Play(0x67, 1);
	}

	DecalFont_DrawLine(sdata->lngStrings[LNG_PRESS_TO_CONTINUE], 0x100, 0xbe, FONT_BIG, (JUSTIFY_CENTER | ORANGE));

	// if still waiting to press X/O, quit function
	if ((sdata->AnyPlayerTap & CC_CONFIRM_BUTTON_MASK) == 0)
	{
		return;
	}

	// if pressed X/O,
	// unlock token and leave level

	RECTMENU_ClearInput();
	sdata->framesSinceRaceEnded = 0;

	sdata->Loading.OnBegin.AddBitsConfig0 |= ADVENTURE_ARENA;
	sdata->Loading.OnBegin.RemBitsConfig0 |= CRYSTAL_CHALLENGE;
	CC_EndEvent_UnlockRewardBit(adv, tokenRewardBit);
	MainRaceTrack_RequestLoad(gGT->prevLEV); // NOTE(aalhendi): Adv hub.

	return;
}

struct MenuRow rows221[3] = {
    // Retry
    {
        .stringIndex = LNG_RETRY,
        .rowOnPressUp = 0,
        .rowOnPressDown = 1,
        .rowOnPressLeft = 0,
        .rowOnPressRight = 0,
    },

    // Exit to map
    {
        .stringIndex = LNG_EXIT_TO_MAP,
        .rowOnPressUp = 0,
        .rowOnPressDown = 1,
        .rowOnPressLeft = 1,
        .rowOnPressRight = 1,
    },

    // NULL, end of menu
    {
        .stringIndex = 0xFFFF,
        .rowOnPressUp = 0,
        .rowOnPressDown = 0,
        .rowOnPressLeft = 0,
        .rowOnPressRight = 0,
    }};

struct RectMenu menu221 = {
    .stringIndexTitle = 0xFFFF,
    .posX_curr = 0x100,
    .posY_curr = 0xB4,

    .unk1 = 0,

    .state = 0x803,
    .rows = rows221,
    .funcPtr = UI_RaceEnd_MenuProc,
    .drawStyle = 4,

    // rest of variables all default zero
};
