#include <common.h>
#include <widescreen.h>
#include "AH_UI.h"

enum AHPauseIconGroupID
{
	AH_PAUSE_ICON_GROUP_HUD = 4,
	AH_PAUSE_ICON_GROUP_MISC = 5,
};

enum AHPauseHudIconID
{
	AH_PAUSE_HUD_ICON_BOSS_STAR = 0x37,
	AH_PAUSE_HUD_ICON_PAGE_ARROW = 0x38,
};

enum AHPauseLayout
{
	AH_PAUSE_UI_COORD_SCALE = 0x100,

	AH_PAUSE_TITLE_CENTER_X = 0x100,
	AH_PAUSE_TITLE_Y = 0xf,
	AH_PAUSE_TITLE_FRAME_PAD_W = 0x14,
	AH_PAUSE_TITLE_FRAME_MIN_TEXT_W = 0x20b,
	AH_PAUSE_TITLE_FRAME_W = 0x21e,

	AH_PAUSE_ARROW_Y = 0x16,
	AH_PAUSE_ARROW_LEFT_X_OFFSET = 0xec,
	AH_PAUSE_ARROW_RIGHT_X_OFFSET = 0x112,
	AH_PAUSE_ARROW_SCALE = 0x1000,
	AH_PAUSE_ARROW_LEFT_ROT_Y = 0x800,

	AH_PAUSE_HUB_TEXT_X = 0x50,
	AH_PAUSE_HUB_ICON_X = 0x15e,
	AH_PAUSE_HUB_FIRST_TEXT_X = 0x6e,
	AH_PAUSE_HUB_FIRST_ICON_X = 0x16d,
	AH_PAUSE_HUB_FIRST_ROW_BASE = 4,
	AH_PAUSE_LEVEL_SCAN_COUNT = 0x41,
	AH_PAUSE_FIRST_CRYSTAL_LEVEL = AH_PAUSE_RELIC_TRACK_COUNT,
	AH_PAUSE_HUB_REWARD_ICON_COUNT = 3,
	AH_PAUSE_ROW_STEP_Y = 0x10,
	AH_PAUSE_ROW_TEXT_Y = 0x26,
	AH_PAUSE_ROW_ICON_Y = 0x2f,
	AH_PAUSE_ROW_ICON_STEP_X = 0x1e,

	AH_PAUSE_BOSS_STAR_X_OFFSET = 0x18,
	AH_PAUSE_BOSS_STAR_Y_OFFSET = 0x2a,

	AH_PAUSE_GEM_PAGE_CENTER_INDEX = 2,
	AH_PAUSE_GEM_PAGE_ICON_SPACING_X = 60,
	AH_PAUSE_GEM_PAGE_Y_BASE = 0x6a,
	AH_PAUSE_GEM_PAGE_Y_ALT_SHIFT = 4,

	AH_PAUSE_TOKEN_TOTAL_CENTER_INDEX = 2,
	AH_PAUSE_TOKEN_TOTAL_X_BASE = 0xf0,
	AH_PAUSE_TOKEN_TOTAL_SPACING_X = 60,
	AH_PAUSE_TOKEN_TOTAL_ROW_Y_STEP = 0x28,
	AH_PAUSE_TOKEN_TOTAL_ICON_Y = 0x41,
	AH_PAUSE_TOKEN_TOTAL_NUMBER_X = 0x36,
	AH_PAUSE_TOKEN_TOTAL_NUMBER_Y = 0x3a,
	AH_PAUSE_TOKEN_TOTAL_X_GLYPH_X = 0x24,
	AH_PAUSE_TOKEN_TOTAL_X_GLYPH_Y = 0x3e,

	AH_PAUSE_RELIC_TOTAL_CENTER_INDEX = 1,
	AH_PAUSE_RELIC_TOTAL_X_BASE = 0xf6,
	AH_PAUSE_RELIC_TOTAL_SPACING_X = 90,
	AH_PAUSE_RELIC_TOTAL_Y = 0x49,
	AH_PAUSE_RELIC_TOTAL_NUMBER_X = 0x19,
	AH_PAUSE_RELIC_TOTAL_X_GLYPH_X = 10,
	AH_PAUSE_RELIC_TOTAL_X_GLYPH_Y = 0x4e,

	AH_PAUSE_TOTAL_TEXT_Y = 0x6e,

	AH_PAUSE_OUTER_RECT_CENTER_X = 0x10a,
	AH_PAUSE_OUTER_RECT_Y = 0x20,
	AH_PAUSE_OUTER_RECT_H = 2,
	AH_PAUSE_INNER_RECT_CENTER_X = 0x100,
	AH_PAUSE_INNER_RECT_Y = 10,
	AH_PAUSE_INNER_RECT_H = 0x82,
	AH_PAUSE_INNER_RECT_COLOR = WHITE,
	AH_PAUSE_INNER_RECT_OT_OFFSET = 3,

	AH_PAUSE_LOCKED_ALPHA = 0x1000,
	AH_PAUSE_TOKEN_TOTAL_SCALE = 0x1000,
	AH_PAUSE_NON_HUB_SCALE_SHIFT = 2,

	AH_PAUSE_MODEL_ROT_X_WEIGHT = 0x10,
	AH_PAUSE_MODEL_ROT_Y_WEIGHT = 0x20,
	AH_PAUSE_MODEL_ROT_FRAME_WEIGHT = 0x40,
	AH_PAUSE_MODEL_ROT_MASK = 0xfff,

	AH_PAUSE_INSTANCE_DEPTH = 0x100,
	AH_PAUSE_PAGE_FLIP_FRAMES = 8,
	AH_PAUSE_PAGE_FLIP_SECOND_HALF = 5,
	AH_PAUSE_PAGE_FLIP_X_STEP = 0x80,
};

enum AHPauseColorSlot
{
	AH_PAUSE_ARROW_COLOR_PRIMARY = ORANGE,
	AH_PAUSE_ARROW_COLOR_SECONDARY = RED,
	AH_PAUSE_BOSS_STAR_LOCKED_COLOR = BLACK,
	AH_PAUSE_BOSS_STAR_UNLOCKED_COLOR = PERIWINKLE,
};

enum AHPauseInstanceFlags
{
	AH_PAUSE_INSTANCE_DYNAMIC_FLAGS = HIDE_MODEL | DRAW_TRANSPARENT | USE_SPECULAR_LIGHT | DRAW_BILLBOARD,
};

enum AHPauseMemberFlags
{
	AH_PAUSE_MEMBER_UNLOCKED = 1,
};

void AH_Pause_Destroy(void)
{
	s16 i;
	struct Thread *t;
	u32 memberAddress;

	// global -> register
	struct PauseObject *ptrPauseObject = AH_PAUSE_OBJECT;

	// check register
	if (ptrPauseObject == 0)
	{
		return;
	}

	// NOTE(aalhendi): Both targets use 32-bit addresses. An integer cursor
	// preserves retail's reverse walk without forming a pointer before the array.
	memberAddress = (u32)&ptrPauseObject->members[AH_PAUSE_MEMBER_COUNT - 1];
	i = 0;
	do
	{
		INSTANCE_Death(((struct AHPauseMember *)memberAddress)->inst);
		memberAddress -= sizeof(struct AHPauseMember);
		i++;
	} while (i < AH_PAUSE_MEMBER_COUNT);

	// kill thread
	t = AH_PAUSE_OBJECT->t;
	AH_PAUSE_OBJECT = 0;
	t->flags |= THREAD_FLAG_DEAD;
}

void AH_Pause_Draw(s32 pageID, s32 inputPosX)
{
	// NOTE(aalhendi): Keep the title/arrow working coordinate separate from the
	// saved short used by the sliding page and its reward instances.
	register s32 posX CTR_PSX_REGISTER("$18") = inputPosX;
	s16 i;
	struct AHPauseMember *member;
	s16 savedPosX = posX;
	struct AHPausePage *page = &AH_PAUSE_PAGES[(s16)pageID];
	s16 lngIndex = page->titleLng;

	s32 titleX = posX + AH_PAUSE_TITLE_CENTER_X;
	if (page->titleLng < 0)
		lngIndex = AH_LEVEL_METADATA[page->hubID].name_LNG;
	DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[lngIndex], titleX, AH_PAUSE_TITLE_Y, FONT_BIG, JUSTIFY_CENTER | ORANGE);
	{
		s32 colorIndex = AH_PAUSE_ARROW_COLOR_PRIMARY;
		s16 titleWidth;
		s32 titleHalfWidth;
		if ((AH_FRAME_COUNTER & 4) == 0)
			colorIndex = AH_PAUSE_ARROW_COLOR_SECONDARY;
		titleWidth = DecalFont_GetLineWidth(GAME_LANGUAGE_STRINGS[lngIndex], FONT_BIG);
		titleHalfWidth = titleWidth / 2;
		posX = (s16)posX;
		AH_DECALHUD_ARROW_2D((ICONGROUP_GETICONS(GAME_TRACKER->iconGroup[AH_PAUSE_ICON_GROUP_HUD]))[AH_PAUSE_HUD_ICON_PAGE_ARROW],
		                     posX - titleHalfWidth + AH_PAUSE_ARROW_LEFT_X_OFFSET, AH_PAUSE_ARROW_Y, &GAME_TRACKER->backBuffer->primMem,
		                     GAME_TRACKER->pushBuffer_UI.ptrOT, ((Color *)AH_COLOR_POINTERS[colorIndex])[0], ((Color *)AH_COLOR_POINTERS[colorIndex])[1],
		                     ((Color *)AH_COLOR_POINTERS[colorIndex])[2], ((Color *)AH_COLOR_POINTERS[colorIndex])[3], 0, AH_PAUSE_ARROW_SCALE,
		                     AH_PAUSE_ARROW_LEFT_ROT_Y);
		AH_DECALHUD_ARROW_2D((ICONGROUP_GETICONS(GAME_TRACKER->iconGroup[AH_PAUSE_ICON_GROUP_HUD]))[AH_PAUSE_HUD_ICON_PAGE_ARROW],
		                     titleHalfWidth + posX + AH_PAUSE_ARROW_RIGHT_X_OFFSET, AH_PAUSE_ARROW_Y, &GAME_TRACKER->backBuffer->primMem,
		                     GAME_TRACKER->pushBuffer_UI.ptrOT, ((Color *)AH_COLOR_POINTERS[colorIndex])[0], ((Color *)AH_COLOR_POINTERS[colorIndex])[1],
		                     ((Color *)AH_COLOR_POINTERS[colorIndex])[2], ((Color *)AH_COLOR_POINTERS[colorIndex])[3], 0, AH_PAUSE_ARROW_SCALE, 0);
	}
	member = AH_PAUSE_OBJECT->members;
	for (i = 0; i < AH_PAUSE_MEMBER_COUNT; i++, member++)
	{
		member->iconIndex = AH_PAUSE_ICON_NONE;
		member->unlockFlags &= ~AH_PAUSE_MEMBER_UNLOCKED;
	}
	member = AH_PAUSE_OBJECT->members;
	switch (page->type)
	{
	case AH_PAUSE_PAGE_HUB:
	{
		s16 hubID;
		s16 crystalID;
		s16 iconX;
		s16 textX;
		s16 rowY = 0;
		s16 trackID;
		crystalID = -1;
		hubID = page->hubID - GEM_STONE_VALLEY;
		iconX = AH_PAUSE_HUB_ICON_X;
		textX = AH_PAUSE_HUB_TEXT_X;
		if (hubID == 0)
		{
			iconX = AH_PAUSE_HUB_FIRST_ICON_X;
			rowY = AH_PAUSE_HUB_FIRST_ROW_BASE;
			textX = AH_PAUSE_HUB_FIRST_TEXT_X;
		}
		for (trackID = 0; trackID < AH_PAUSE_LEVEL_SCAN_COUNT; trackID++)
		{
			if (AH_LEVEL_METADATA[trackID].hubID != hubID)
				continue;
			if (trackID >= AH_PAUSE_FIRST_CRYSTAL_LEVEL)
			{
				crystalID = trackID;
				continue;
			}
			// Set the row's rewards before positioning them through the UI helpers.
			if (hubID != 0)
			{
				member->iconIndex = AH_PAUSE_ICON_TROPHY;
				if (CHECK_ADV_BIT(GAME_ADV_PROGRESS.rewards, trackID + ADV_REWARD_FIRST_TROPHY))
					member->unlockFlags |= AH_PAUSE_MEMBER_UNLOCKED;
				member++;
			}
			if (CHECK_ADV_BIT(GAME_ADV_PROGRESS.rewards, trackID + ADV_REWARD_FIRST_PLATINUM_RELIC))
			{
				member->unlockFlags |= AH_PAUSE_MEMBER_UNLOCKED;
				member->iconIndex = AH_PAUSE_ICON_PLATINUM_RELIC;
			}
			else if (CHECK_ADV_BIT(GAME_ADV_PROGRESS.rewards, trackID + ADV_REWARD_FIRST_GOLD_RELIC))
			{
				member->unlockFlags |= AH_PAUSE_MEMBER_UNLOCKED;
				member->iconIndex = AH_PAUSE_ICON_GOLD_RELIC;
			}
			else
			{
				member->iconIndex = AH_PAUSE_ICON_SAPPHIRE_RELIC;
				if (CHECK_ADV_BIT(GAME_ADV_PROGRESS.rewards, trackID + ADV_REWARD_FIRST_SAPPHIRE_RELIC))
					member->unlockFlags |= AH_PAUSE_MEMBER_UNLOCKED;
			}
			member++;
			if (hubID != 0)
			{
				member->iconIndex = AH_PAUSE_ICON_FIRST_TOKEN + AH_LEVEL_METADATA[trackID].ctrTokenGroupID;
				if (CHECK_ADV_BIT(GAME_ADV_PROGRESS.rewards, trackID + ADV_REWARD_FIRST_CTR_TOKEN))
					member->unlockFlags |= AH_PAUSE_MEMBER_UNLOCKED;
				member++;
				for (i = 0; i < AH_PAUSE_HUB_REWARD_ICON_COUNT; i++)
				{
					Vec3 *translation = (Vec3 *)member[i - AH_PAUSE_HUB_REWARD_ICON_COUNT].inst->matrix.t;
					translation->x = SelectProfile_UI_ConvertX(savedPosX + iconX + Widescreen_ScaleWidth(i * AH_PAUSE_ROW_ICON_STEP_X), AH_PAUSE_UI_COORD_SCALE);
					translation->y = SelectProfile_UI_ConvertY(rowY + AH_PAUSE_ROW_ICON_Y, AH_PAUSE_UI_COORD_SCALE);
				}
			}
			else
			{
				Vec3 *translation = (Vec3 *)member[-1].inst->matrix.t;
				translation->x = SelectProfile_UI_ConvertX(savedPosX + iconX + Widescreen_ScaleWidth(AH_PAUSE_ROW_ICON_STEP_X), AH_PAUSE_UI_COORD_SCALE);
				translation->y = SelectProfile_UI_ConvertY(rowY + AH_PAUSE_ROW_ICON_Y, AH_PAUSE_UI_COORD_SCALE);
			}
			DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[AH_LEVEL_METADATA[trackID].name_LNG], savedPosX + textX, rowY + AH_PAUSE_ROW_TEXT_Y, FONT_BIG, 0);
			rowY += AH_PAUSE_ROW_STEP_Y;
		}
		DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[GAME_CHARACTER_METADATA[page->characterID_Boss].name_LNG_long], savedPosX + textX, rowY + AH_PAUSE_ROW_TEXT_Y,
		                   FONT_BIG, WHITE);
		if (hubID != 0)
		{
			Vec3 *translation;
			member->iconIndex = AH_PAUSE_ICON_BOSS_KEY;
			if (CHECK_ADV_BIT(GAME_ADV_PROGRESS.rewards, AH_BOSS_REWARDS[hubID]))
				member->unlockFlags |= AH_PAUSE_MEMBER_UNLOCKED;
			translation = (Vec3 *)member->inst->matrix.t;
			translation->x = SelectProfile_UI_ConvertX(savedPosX + iconX + Widescreen_ScaleWidth(AH_PAUSE_ROW_ICON_STEP_X), AH_PAUSE_UI_COORD_SCALE);
			translation->y = SelectProfile_UI_ConvertY(rowY + AH_PAUSE_ROW_ICON_Y, AH_PAUSE_UI_COORD_SCALE);
			member++;
		}
		else
		{
			s16 colorIndex = AH_PAUSE_BOSS_STAR_LOCKED_COLOR;
			if (CHECK_ADV_BIT(GAME_ADV_PROGRESS.rewards, AH_BOSS_REWARDS[0]))
				colorIndex = AH_PAUSE_BOSS_STAR_UNLOCKED_COLOR;
			AH_DECALHUD_DRAW_POLY_GT4((ICONGROUP_GETICONS(GAME_TRACKER->iconGroup[AH_PAUSE_ICON_GROUP_MISC]))[AH_PAUSE_HUD_ICON_BOSS_STAR],
			                          savedPosX + iconX + AH_PAUSE_BOSS_STAR_X_OFFSET, rowY + AH_PAUSE_BOSS_STAR_Y_OFFSET, &GAME_TRACKER->backBuffer->primMem,
			                          GAME_TRACKER->pushBuffer_UI.ptrOT, ((Color *)AH_COLOR_POINTERS[colorIndex])[0],
			                          ((Color *)AH_COLOR_POINTERS[colorIndex])[1], ((Color *)AH_COLOR_POINTERS[colorIndex])[2],
			                          ((Color *)AH_COLOR_POINTERS[colorIndex])[3], 0, AH_PAUSE_ARROW_SCALE);
		}
		if (crystalID >= 0)
		{
			Vec3 *translation;
			s16 crystalRowY = rowY + AH_PAUSE_ROW_STEP_Y;
			// NOTE(aalhendi): Both draw calls retain this short row coordinate.
			CTR_PSX_KEEP_VALUE_RELAXED(crystalRowY);
			DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[AH_LEVEL_METADATA[crystalID].name_LNG], savedPosX + textX, crystalRowY + AH_PAUSE_ROW_TEXT_Y, FONT_BIG,
			                   PERIWINKLE);
			member->iconIndex = AH_PAUSE_ICON_FIRST_TOKEN + AH_LEVEL_METADATA[crystalID].ctrTokenGroupID;
			if (CHECK_ADV_BIT(GAME_ADV_PROGRESS.rewards, hubID + ADV_REWARD_PURPLE_TOKEN_HUB_ID_BASE))
				member->unlockFlags |= AH_PAUSE_MEMBER_UNLOCKED;
			translation = (Vec3 *)member->inst->matrix.t;
			translation->x = SelectProfile_UI_ConvertX(savedPosX + iconX + Widescreen_ScaleWidth(AH_PAUSE_ROW_ICON_STEP_X), AH_PAUSE_UI_COORD_SCALE);
			translation->y = SelectProfile_UI_ConvertY(crystalRowY + AH_PAUSE_ROW_ICON_Y, AH_PAUSE_UI_COORD_SCALE);
			member++;
		}
		if (hubID == 0)
		{
			for (i = 0; i < AH_PAUSE_GEM_ICON_COUNT; i++, member++)
			{
				Vec3 *translation;
				member->iconIndex = AH_PAUSE_ICON_FIRST_GEM + i;
				if (CHECK_ADV_BIT(GAME_ADV_PROGRESS.rewards, i + ADV_REWARD_FIRST_GEM))
					member->unlockFlags |= AH_PAUSE_MEMBER_UNLOCKED;
				translation = (Vec3 *)member->inst->matrix.t;
				translation->x = SelectProfile_UI_ConvertX(
				    savedPosX + AH_PAUSE_TITLE_CENTER_X + Widescreen_ScaleWidth((i - AH_PAUSE_GEM_PAGE_CENTER_INDEX) * AH_PAUSE_GEM_PAGE_ICON_SPACING_X), AH_PAUSE_UI_COORD_SCALE);
				translation->y = SelectProfile_UI_ConvertY(((i & 1) << AH_PAUSE_GEM_PAGE_Y_ALT_SHIFT) | AH_PAUSE_GEM_PAGE_Y_BASE, AH_PAUSE_UI_COORD_SCALE);
			}
		}
		break;
	}
	case AH_PAUSE_PAGE_TOKEN_TOTALS:
	{
		s16 totals[AH_PAUSE_TOKEN_ICON_COUNT];
		for (i = 0; i < AH_PAUSE_TOKEN_ICON_COUNT; i++)
			totals[i] = 0;
		for (i = 0; i < AH_PAUSE_CTR_TOKEN_TRACK_COUNT; i++)
			if (CHECK_ADV_BIT(GAME_ADV_PROGRESS.rewards, i + ADV_REWARD_FIRST_CTR_TOKEN))
				totals[AH_LEVEL_METADATA[i].ctrTokenGroupID]++;
		for (i = 0; i < AH_PAUSE_PURPLE_TOKEN_COUNT; i++)
			if (CHECK_ADV_BIT(GAME_ADV_PROGRESS.rewards, i + ADV_REWARD_FIRST_PURPLE_TOKEN))
				totals[AH_PAUSE_PURPLE_TOKEN_INDEX]++;
		for (i = 0; i < AH_PAUSE_TOKEN_ICON_COUNT; i++, member++)
		{
			Vec3 *translation;
			s16 instPosX = savedPosX + AH_PAUSE_TOKEN_TOTAL_X_BASE + Widescreen_ScaleWidth((i - AH_PAUSE_TOKEN_TOTAL_CENTER_INDEX) * AH_PAUSE_TOKEN_TOTAL_SPACING_X);
			s32 instPosY = (i & 1) * AH_PAUSE_TOKEN_TOTAL_ROW_Y_STEP;
			member->iconIndex = AH_PAUSE_ICON_FIRST_TOKEN + i;
			member->unlockFlags |= AH_PAUSE_MEMBER_UNLOCKED;
			translation = (Vec3 *)member->inst->matrix.t;
			translation->x = SelectProfile_UI_ConvertX(instPosX, AH_PAUSE_UI_COORD_SCALE);
			translation->y = SelectProfile_UI_ConvertY(instPosY + AH_PAUSE_TOKEN_TOTAL_ICON_Y, AH_PAUSE_UI_COORD_SCALE);
			SelectProfile_PrintInteger(totals[i], (s16)(instPosX + Widescreen_ScaleWidth(AH_PAUSE_TOKEN_TOTAL_NUMBER_X)), instPosY + AH_PAUSE_TOKEN_TOTAL_NUMBER_Y, 0, 0);
			DecalFont_DrawLine(AH_PAUSE_X_GLYPH, instPosX + Widescreen_ScaleWidth(AH_PAUSE_TOKEN_TOTAL_X_GLYPH_X), instPosY + AH_PAUSE_TOKEN_TOTAL_X_GLYPH_Y, FONT_SMALL, 0);
		}
		break;
	}
	case AH_PAUSE_PAGE_RELIC_TOTALS:
	{
		s16 totals[AH_PAUSE_RELIC_ICON_COUNT];
		char totalString[100];
		for (i = 0; i < AH_PAUSE_RELIC_ICON_COUNT; i++)
			totals[i] = 0;
		for (i = 0; i < AH_PAUSE_RELIC_TRACK_COUNT; i++)
		{
			if (CHECK_ADV_BIT(GAME_ADV_PROGRESS.rewards, i + ADV_REWARD_FIRST_PLATINUM_RELIC))
				totals[2]++;
			else if (CHECK_ADV_BIT(GAME_ADV_PROGRESS.rewards, i + ADV_REWARD_FIRST_GOLD_RELIC))
				totals[1]++;
			else if (CHECK_ADV_BIT(GAME_ADV_PROGRESS.rewards, i + ADV_REWARD_FIRST_SAPPHIRE_RELIC))
				totals[0]++;
		}
		for (i = 0; i < AH_PAUSE_RELIC_ICON_COUNT; i++, member++)
		{
			Vec3 *translation;
			s16 instPosX = savedPosX + AH_PAUSE_RELIC_TOTAL_X_BASE + Widescreen_ScaleWidth((i - AH_PAUSE_RELIC_TOTAL_CENTER_INDEX) * AH_PAUSE_RELIC_TOTAL_SPACING_X);
			member->iconIndex = AH_PAUSE_ICON_SAPPHIRE_RELIC + i;
			member->unlockFlags |= AH_PAUSE_MEMBER_UNLOCKED;
			translation = (Vec3 *)member->inst->matrix.t;
			translation->x = SelectProfile_UI_ConvertX(instPosX, AH_PAUSE_UI_COORD_SCALE);
			translation->y = SelectProfile_UI_ConvertY(AH_PAUSE_RELIC_TOTAL_Y, AH_PAUSE_UI_COORD_SCALE);
			SelectProfile_PrintInteger(totals[i], (s16)(instPosX + Widescreen_ScaleWidth(AH_PAUSE_RELIC_TOTAL_NUMBER_X)), AH_PAUSE_RELIC_TOTAL_Y, 0, 0);
			DecalFont_DrawLine(AH_PAUSE_X_GLYPH, instPosX + Widescreen_ScaleWidth(AH_PAUSE_RELIC_TOTAL_X_GLYPH_X), AH_PAUSE_RELIC_TOTAL_X_GLYPH_Y, FONT_SMALL, 0);
		}
		sprintf(totalString, AH_PAUSE_TOTAL_FORMAT, GAME_LANGUAGE_STRINGS[LNG_TOTAL], totals[0] + totals[1] + totals[2]);
		DecalFont_DrawLine(totalString, savedPosX + AH_PAUSE_TITLE_CENTER_X, AH_PAUSE_TOTAL_TEXT_Y, FONT_BIG, JUSTIFY_CENTER | ORANGE);
		break;
	}
	}
	{
		s32 width = DecalFont_GetLineWidth(GAME_LANGUAGE_STRINGS[lngIndex], FONT_BIG);
		s32 halfWidth;
		RECT r;
		if ((s16)width < AH_PAUSE_TITLE_FRAME_MIN_TEXT_W)
			width = AH_PAUSE_TITLE_FRAME_W;
		else
			width += AH_PAUSE_TITLE_FRAME_PAD_W;
		halfWidth = (s16)width / 2;
		r.x = AH_PAUSE_OUTER_RECT_CENTER_X - halfWidth;
		r.y = AH_PAUSE_OUTER_RECT_Y;
		r.w = width - AH_PAUSE_TITLE_FRAME_PAD_W;
		r.h = AH_PAUSE_OUTER_RECT_H;
		AH_DrawMenuEdge(&r, GAME_TRACKER->backBuffer->otMem.uiOT);
		r.x = AH_PAUSE_INNER_RECT_CENTER_X - halfWidth;
		r.y = AH_PAUSE_INNER_RECT_Y;
		r.w = width;
		r.h = AH_PAUSE_INNER_RECT_H;
		RECTMENU_DrawInnerRect(&r, AH_PAUSE_INNER_RECT_COLOR, &GAME_TRACKER->backBuffer->otMem.uiOT[AH_PAUSE_INNER_RECT_OT_OFFSET]);
	}
	member = AH_PAUSE_OBJECT->members;
	for (i = 0; i < AH_PAUSE_MEMBER_COUNT; i++, member++)
	{
		s16 index = member->iconIndex;
		struct Instance *inst = member->inst;
		if (index >= 0)
		{
			struct AHPauseInstance *info = &AH_PAUSE_ICONS[index];
			s16 scale;
			register s32 lockedAlpha CTR_PSX_REGISTER("$2");
			inst->flags &= ~AH_PAUSE_INSTANCE_DYNAMIC_FLAGS;
			inst->flags |= info->instFlags;
			if (member->unlockFlags & AH_PAUSE_MEMBER_UNLOCKED)
			{
				inst->alphaScale = 0;
				inst->colorRGBA = INST_PackColorRGB(((u8 *)&info->color)[0], ((u8 *)&info->color)[1], ((u8 *)&info->color)[2]);
			}
			else
			{
				// NOTE(aalhendi): The locked path rereads the flags and reuses the
				// branch's temporary for alpha; do not fold it into the earlier mask.
				lockedAlpha = AH_PAUSE_LOCKED_ALPHA;
				CTR_PSX_DEPEND_MEMORY(&inst->flags, lockedAlpha);
				inst->flags &= ~AH_PAUSE_INSTANCE_DYNAMIC_FLAGS;
				inst->colorRGBA = 0;
				inst->alphaScale = lockedAlpha;
			}
			if (page->type != AH_PAUSE_PAGE_TOKEN_TOTALS)
			{
				if (page->type == AH_PAUSE_PAGE_HUB)
					scale = info->scale;
				else
					scale = info->scale * (1 << AH_PAUSE_NON_HUB_SCALE_SHIFT);
			}
			else
				scale = AH_PAUSE_TOKEN_TOTAL_SCALE;
			inst->scale.z = scale;
			inst->scale.y = scale;
			inst->scale.x = scale;
			inst->model = GAME_TRACKER->modelPtr[info->modelID];
			ConvertRotToMatrix(&inst->matrix, &member->rot);
			if ((inst->flags & (DRAW_BILLBOARD | DRAW_TRANSPARENT | USE_SPECULAR_LIGHT)) == USE_SPECULAR_LIGHT)
			{
				SVec3 direction;
				// Lighting receives a snapshot, not the mutable icon-table entry.
				direction.x = info->lightDir.x;
				direction.y = info->lightDir.y;
				direction.z = info->lightDir.z;
				Vector_SpecLightSpin2D(inst, &member->rot, &direction);
			}
			else
				inst->colorRGBA = 0;
		}
		else
			inst->flags |= HIDE_MODEL;
		// NOTE(aalhendi): Retail stores the wrapping 16-bit angle, not a 12-bit mask.
		member->rot.y = (u32)member->inst->matrix.t[0] * AH_PAUSE_MODEL_ROT_X_WEIGHT + (u32)member->inst->matrix.t[1] * AH_PAUSE_MODEL_ROT_Y_WEIGHT +
		                (s16)AH_FRAME_COUNTER * AH_PAUSE_MODEL_ROT_FRAME_WEIGHT;
	}
}

void AH_Pause_Update(void)
{
	s32 tap;
	s32 pageID;
	s32 posX;

	if (AH_PAUSE_OBJECT == 0)
	{
		struct Thread *t;
		struct PauseObject *ptrPauseObject;

		AH_PAUSE_TIMER = 0;
		GAME_TRACKER->advPausePage = AH_PAUSE_CURRENT_PAGE = GAME_TRACKER->levelID - GEM_STONE_VALLEY;

		// 0 = size
		// 0 = no relation to param4
		// 0x300 = SmallStackPool
		// 0xd = "other" thread bucket
		t = PROC_BirthWithObject(SIZE_RELATIVE_POOL_BUCKET(0, NONE, SMALL, OTHER), 0, AH_PAUSE_THREAD_NAME, 0);

		ptrPauseObject = &AH_PAUSE_STORAGE;
		AH_PAUSE_OBJECT = ptrPauseObject;
		ptrPauseObject->t = t;

		{
			s16 i;
			struct AHPauseMember *member = ptrPauseObject->members;
			for (i = 0; i < AH_PAUSE_MEMBER_COUNT; i++, member++)
			{
				struct InstDrawPerPlayer *idpp;
				struct Instance *inst = INSTANCE_Birth3D(GAME_TRACKER->modelPtr[STATIC_GEM], AH_PAUSE_INSTANCE_NAME, t);

				member->inst = inst;

				inst->flags |= (USE_SPECULAR_LIGHT | SCREENSPACE_INSTANCE | HIDE_MODEL);

				idpp = INST_GETIDPP(inst);

				idpp[0].pushBuffer = &GAME_TRACKER->pushBuffer_UI;
				{
					s16 j;
					for (j = 1; j < GAME_TRACKER->numPlyrCurrGame; j++)
					{
						u32 offset = j * sizeof(struct InstDrawPerPlayer);
						((struct InstDrawPerPlayer *)((u32)inst + offset + sizeof(struct Instance)))->pushBuffer = 0;
					}
				}

				member->rot.x = member->rot.y = member->rot.z = 0;
				member->iconIndex = AH_PAUSE_ICON_NONE;

				CTR_MatrixSetRotIdentity(&inst->matrix);
				inst->matrix.t[2] = AH_PAUSE_INSTANCE_DEPTH;
			}
		}
	}

	tap = AH_PAUSE_BUTTON_TAP;

	if ((tap & (BTN_RIGHT | BTN_LEFT)) != 0)
	{
		if ((tap & BTN_LEFT) != 0)
		{
			AH_PAUSE_DIRECTION = -1;
			GAME_TRACKER->advPausePage += -1;

			if (GAME_TRACKER->advPausePage < 0)
			{
				GAME_TRACKER->advPausePage = AH_PAUSE_MENU_PAGE_COUNT - 1;
			}
		}

		// Right input
		else
		{
			if (!(tap & BTN_RIGHT))
			{
				goto noInput;
			}
			AH_PAUSE_DIRECTION = 1;
			GAME_TRACKER->advPausePage += 1;

			if ((u16)GAME_TRACKER->advPausePage >= AH_PAUSE_MENU_PAGE_COUNT)
			{
				GAME_TRACKER->advPausePage = 0;
			}
		}

		OtherFX_Play(0, 1);
	}

noInput:
	// page is flipping
	if (AH_PAUSE_TIMER > 0)
	{
		AH_PAUSE_TIMER--;
	}
	// page is not flipping, flip desired
	else if (GAME_TRACKER->advPausePage != AH_PAUSE_CURRENT_PAGE)
	{
		AH_PAUSE_PREVIOUS_PAGE = AH_PAUSE_CURRENT_PAGE;

		AH_PAUSE_TIMER = AH_PAUSE_PAGE_FLIP_FRAMES;

		AH_PAUSE_CURRENT_PAGE = GAME_TRACKER->advPausePage;
		AH_PAUSE_FLIP_DIRECTION = AH_PAUSE_DIRECTION;
	}


	pageID = AH_PAUSE_TIMER;
	if (pageID >= AH_PAUSE_PAGE_FLIP_SECOND_HALF)
	{
		s32 remaining = AH_PAUSE_PAGE_FLIP_FRAMES - pageID;
		posX = remaining * CTR_MipsSll(AH_PAUSE_DIRECTION, 7);
		// NOTE(aalhendi): Keep the page read after the slide product without
		// forcing an early read of the MIPS multiply result.
		CTR_PSX_OBSERVE_MEMORY(AH_PAUSE_PREVIOUS_PAGE);
		pageID = AH_PAUSE_PREVIOUS_PAGE;
	}
	else
	{
		// NOTE(aalhendi): Retain the signed divide and its input lifetime.
		// Combining this into timer * -128 changes the retail instruction sequence.
		s32 shifted = CTR_MipsSll(pageID, 9);
		register s32 offset CTR_PSX_REGISTER("$5") = -shifted;
		CTR_PSX_ORDER_VALUES(offset, shifted);
		pageID = AH_PAUSE_CURRENT_PAGE;
		posX = (offset / 4) * AH_PAUSE_DIRECTION;
	}

	AH_Pause_Draw(pageID, (s16)posX);
}
