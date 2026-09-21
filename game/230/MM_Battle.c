#include <common.h>

void MM_Battle_DrawIcon_Character(struct Icon *icon, s32 posX, s32 posY, struct PrimMem *primMem, u32 *ot, u8 transparency, s16 scale)
{
	if (icon == 0)
	{
		return;
	}
	DecalHUD_DrawPolyFT4(icon, posX, posY, primMem, ot, transparency, scale);
}

void MM_Battle_CloseSubMenu(struct RectMenu *menu)
{
	menu->state |= ONLY_DRAW_TITLE;
}

enum
{
	BATTLE_TRANSITION_FRAME_COUNT = 0xc,
	BATTLE_ANIMATED_TRANSITION_META_COUNT = 8,
	BATTLE_SETTINGS_COUNT = 5,
	BATTLE_TEAM_COUNT = 4,
	BATTLE_VALID_TEAM_MIN = 0,
	BATTLE_VALID_TEAM_MAX = BATTLE_TEAM_COUNT - 1,
	BATTLE_ROW_TYPE = 0,
	BATTLE_ROW_LENGTH = 1,
	BATTLE_ROW_TEAMS = 2,
	BATTLE_ROW_WEAPON_TOP = 3,
	BATTLE_ROW_WEAPON_BOTTOM = 4,
	BATTLE_ROW_START = 5,
	BATTLE_ROW_LIFE_COUNT = 10,
	BATTLE_WEAPON_ROW_COUNT = 2,
	BATTLE_WEAPONS_PER_ROW = 6,
	BATTLE_WEAPON_ITEM_COUNT = 11,
	BATTLE_WEAPON_FLAG_COUNT = 0xe,
	BATTLE_REQUIRED_WEAPON_FLAGS = 0xcde,
	BATTLE_CONFIRM_INPUT = BTN_CROSS_one | BTN_CIRCLE,
	BATTLE_BACK_INPUT = BTN_SQUARE_one | BTN_TRIANGLE,
	BATTLE_MENU_INPUT = BTN_UP | BTN_DOWN | BTN_LEFT | BTN_RIGHT | BATTLE_CONFIRM_INPUT | BATTLE_BACK_INPUT,
	BATTLE_TIME_LIMIT_TYPE_ROW = 1,
	BATTLE_LIFE_LIMIT_TYPE_ROW = 2,
	BATTLE_MINUTES_TO_EVENT_TIME = 0xe100,
	BATTLE_INACTIVE_TEAM_POINTS = -500,
	BATTLE_ICON_SCALE = 0x1000,
	BATTLE_WEAPON_ICON_ROTATE_RIGHT = 1,
	BATTLE_WEAPON_DISABLED_TEXT_COLOR = 0x15,
	BATTLE_WEAPON_ENABLED_TEXT_COLOR = 4,
	BATTLE_TITLE_META_INDEX = 9,
	BATTLE_TITLE_X_OFFSET = 0x100,
	BATTLE_TITLE_Y_OFFSET = 10,
	BATTLE_LABEL_X_OFFSET = 0x8c,
	BATTLE_MENU_X_OFFSET = 0x9c,
	BATTLE_ROW_TYPE_MENU_META_INDEX = 0,
	BATTLE_ROW_TYPE_LABEL_META_INDEX = 1,
	BATTLE_ROW_LENGTH_MENU_META_INDEX = 2,
	BATTLE_ROW_LENGTH_LABEL_META_INDEX = 3,
	BATTLE_ROW_TEAM_META_INDEX = 4,
	BATTLE_ROW_TEAM_LABEL_META_INDEX = 5,
	BATTLE_ROW_WEAPON_PANEL_META_INDEX = 6,
	BATTLE_ROW_WEAPON_LABEL_META_INDEX = 7,
	BATTLE_ROW_START_META_INDEX = 8,
	BATTLE_TYPE_ROW_Y_OFFSET = 0x24,
	BATTLE_MENU_DEFAULT_HEIGHT = 0xd,
	BATTLE_LENGTH_ROW_Y_OFFSET = 0x20,
	BATTLE_LENGTH_LABEL_Y_OFFSET = 4,
	BATTLE_LENGTH_DUAL_MENU_X_OFFSET = 0x142,
	BATTLE_LENGTH_DUAL_MENU_WIDTH = 0x8e,
	BATTLE_WIDE_MENU_WIDTH = 0x134,
	BATTLE_TEAM_PANEL_START_X = 0x9f,
	BATTLE_TEAM_LABEL_Y_OFFSET = 10,
	BATTLE_TEAM_SEGMENT_BASE_WIDTH = 4,
	BATTLE_TEAM_PLAYER_WIDTH = 0x2a,
	BATTLE_TEAM_PANEL_TOTAL_WIDTH = 0x12e,
	BATTLE_TEAM_PLAYER_CENTER_OFFSET = -0x15,
	BATTLE_TEAM_PLAYER_ICON_Y_OFFSET = 6,
	BATTLE_TEAM_COLOR_H = 0x1a,
	BATTLE_TEAM_COLOR_Y_OFFSET = 5,
	BATTLE_TEAM_HIGHLIGHT_W = 0x134,
	BATTLE_TEAM_HIGHLIGHT_H = 0x1e,
	BATTLE_TEAM_HIGHLIGHT_Y_OFFSET = 3,
	BATTLE_TEAM_PANEL_W = 0x140,
	BATTLE_TEAM_PANEL_H = 0x24,
	BATTLE_TEAM_PANEL_X_OFFSET = 0x96,
	BATTLE_WEAPON_LABEL_Y_OFFSET = 0x44,
	BATTLE_ERROR_COLOR_FRAME_BIT = 1,
	BATTLE_ERROR_COLOR_A = -0x7fff,
	BATTLE_ERROR_COLOR_B = -0x7ffd,
	BATTLE_ERROR_TEXT_X = 0x100,
	BATTLE_ERROR_TEXT_LINE_1_Y_OFFSET = 0x6a,
	BATTLE_ERROR_TEXT_LINE_2_Y_OFFSET = 0x7a,
	BATTLE_START_MENU_Y_OFFSET = 0x78,
	BATTLE_WEAPON_PANEL_W = 0x140,
	BATTLE_WEAPON_PANEL_H = 0x44,
	BATTLE_WEAPON_PANEL_X_OFFSET = 0x96,
	BATTLE_WEAPON_PANEL_Y_OFFSET = 0x2a,
	BATTLE_WEAPON_GRID_X_OFFSET = 6,
	BATTLE_WEAPON_GRID_Y_OFFSET = 2,
	BATTLE_WEAPON_GRID_X_STEP = 0x34,
	BATTLE_WEAPON_GRID_ROW_STAGGER_X = 0x1a,
	BATTLE_WEAPON_GRID_Y_STEP = 0x20,
	BATTLE_WEAPON_AMMO_TEXT_FIRST = 7,
	BATTLE_WEAPON_AMMO_TEXT_COUNT = 2,
	BATTLE_WEAPON_HIGHLIGHT_LEFT_X_OFFSET = 4,
	BATTLE_WEAPON_HIGHLIGHT_RIGHT_X_OFFSET = 0x1e,
	BATTLE_WEAPON_HIGHLIGHT_W = 0x34,
	BATTLE_WEAPON_HIGHLIGHT_H = 0x20,
	BATTLE_WEAPON_HIGHLIGHT_Y_OFFSET = 2,
	BATTLE_WEAPON_PANEL_INSET_X = 3,
	BATTLE_WEAPON_PANEL_INSET_Y = 2,
	BATTLE_WEAPON_PANEL_INSET_W_SHRINK = 6,
	BATTLE_WEAPON_PANEL_INSET_H_SHRINK = 4,
	BATTLE_LABEL_TEXT_FLAGS = 0x4000,
	BATTLE_TITLE_TEXT_FLAGS = JUSTIFY_CENTER | ORANGE,
};

enum
{
	BATTLE_COLOR_RGB_MASK = 0xffffffu,
	BATTLE_GPU_CODE_POLY_FT4 = 0x2c000000u,
	BATTLE_GPU_CODE_POLY_FT4_SEMI_TRANS = 0x2e000000u,
	BATTLE_GPU_TAG_LENGTH_POLY_FT4 = 0x09000000u,
	BATTLE_TPAGE_TRANSPARENCY_MASK = 0xff9fffffu,
	BATTLE_TPAGE_TRANSPARENCY_SHIFT = 0x15,
};

void MM_Battle_DrawIcon_Weapon(struct Icon *icon, u32 posX, s32 posY, struct PrimMem *primMem, u32 *ot, s8 transparency, s32 scale, u16 rotation,
                               const u32 *color)
{
	register struct PrimMem *packetMem CTR_PSX_REGISTER("$16");
	register u32 *orderingTable CTR_PSX_REGISTER("$7");
	register struct Icon *drawIcon CTR_PSX_REGISTER("$12");
	register u32 drawX CTR_PSX_REGISTER("$14");
	register const u32 *iconColor CTR_PSX_REGISTER("$25");
	register u16 iconRotation CTR_PSX_REGISTER("$11");
	register s32 iconScale CTR_PSX_REGISTER("$4");
	register u8 iconTransparency CTR_PSX_REGISTER("$8");
	register u32 uv1 CTR_PSX_REGISTER("$9");
	register u32 uv2 CTR_PSX_REGISTER("$13");
	register u32 packedY CTR_PSX_REGISTER("$6");
	register s32 scaledWidth CTR_PSX_REGISTER("$24");
	register s32 scaledHeight CTR_PSX_REGISTER("$15");
	CtrPackedU32 *packet;
	u32 uv0;
	register u32 topLeft CTR_PSX_REGISTER("$4");
	register u32 topRight CTR_PSX_REGISTER("$6");
	register u32 bottomLeft CTR_PSX_REGISTER("$5");
	register u32 bottomRight CTR_PSX_REGISTER("$9");
	u32 farX;
	u32 farY;

	packetMem = primMem;
	orderingTable = ot;
	iconScale = scale;
	drawIcon = icon;
	iconColor = color;
	iconTransparency = (u8)transparency;
	iconRotation = rotation;
	drawX = posX;

	if (!drawIcon)
	{
		goto cleanup;
	}
	CTR_PSX_KEEP_VALUE(iconTransparency);
	CTR_PSX_KEEP_VALUE(orderingTable);
	CTR_PSX_KEEP_VALUE(iconColor);

	uv1 = CTR_ReadU32AlignedLE(&drawIcon->texLayout.u1);
	uv0 = CTR_ReadU32AlignedLE(&drawIcon->texLayout.u0);
	{
		register s32 widthDifference CTR_PSX_REGISTER("$3");
		register s32 widthBase CTR_PSX_REGISTER("$2");
		register s32 widthProduct CTR_PSX_REGISTER("$5");

		widthDifference = (s32)(uv1 & 0xff);
		widthBase = (s32)(uv0 & 0xff);
		widthDifference -= widthBase;
		widthProduct = widthDifference * (s16)iconScale;
		scaledWidth = widthProduct >> 0xc;
	}
	{
		register s32 heightDifference CTR_PSX_REGISTER("$3");
		register s32 heightValue CTR_PSX_REGISTER("$2");

		heightValue = ((s32)uv0 >> 8) & 0xff;
		heightDifference = (s32)(u8)drawIcon->texLayout.v2 - heightValue;
		heightValue = heightDifference * (s16)iconScale;
		scaledHeight = heightValue >> 0xc;
	}
	packedY = (u32)posY << 0x10;
	uv2 = CTR_ReadU32AlignedLE(&drawIcon->texLayout.u2);
	packet = (CtrPackedU32 *)packetMem->cursor;

	{
		register u32 code CTR_PSX_REGISTER("$2");
		register u32 adjustedUv CTR_PSX_REGISTER("$4");
		u32 colorCode;

		code = iconTransparency;
		if (code != 0)
		{
			colorCode = *iconColor & BATTLE_COLOR_RGB_MASK;
			adjustedUv = BATTLE_TPAGE_TRANSPARENCY_MASK;
			adjustedUv = (adjustedUv & uv1) | ((code - 1) << BATTLE_TPAGE_TRANSPARENCY_SHIFT);
			packet[5] = adjustedUv;
			code = BATTLE_GPU_CODE_POLY_FT4_SEMI_TRANS;
		}
		else
		{
			code = BATTLE_COLOR_RGB_MASK;
			colorCode = *iconColor & code;
			packet[5] = uv1;
			code = BATTLE_GPU_CODE_POLY_FT4;
		}

		colorCode |= code;
		packet[1] = colorCode;
	}
	packet[3] = uv0;
	((CtrPackedU16 *)packet)[0xe] = (u16)uv2;
	((CtrPackedU16 *)packet)[0x12] = CTR_ReadU16AlignedLE(&drawIcon->texLayout.u3);

	farX = drawX + scaledHeight;
	if ((iconRotation & 1) != 0)
	{
		farY = packedY + ((u32)scaledWidth << 0x10);
		CTR_PSX_KEEP_VALUE(scaledWidth);
		topLeft = drawX | packedY;
		topRight = farX | packedY;
		bottomLeft = drawX | farY;
		bottomRight = farX | farY;
		CTR_PSX_KEEP_VALUE(farX);
		CTR_PSX_KEEP_VALUE(farY);

		if ((s16)iconRotation == BATTLE_WEAPON_ICON_ROTATE_RIGHT)
		{
			packet[4] = topLeft;
			packet[8] = topRight;
			packet[2] = bottomLeft;
			packet[6] = bottomRight;
		}
		else
		{
			packet[6] = topLeft;
			packet[2] = topRight;
			packet[8] = bottomLeft;
			packet[4] = bottomRight;
		}
	}
	else
	{
		farX = drawX + scaledWidth;
		farY = packedY + ((u32)scaledHeight << 0x10);
		CTR_PSX_KEEP_VALUE(scaledHeight);
		topLeft = drawX | packedY;
		bottomLeft = drawX | farY;
		bottomRight = farX | farY;
		CTR_PSX_KEEP_VALUE(farX);
		CTR_PSX_KEEP_VALUE(farY);
		topRight = farX | packedY;

		if (((u32)iconRotation << 0x10) == 0)
		{
			packet[2] = topLeft;
			packet[4] = topRight;
			packet[6] = bottomLeft;
			packet[8] = bottomRight;
		}
		else
		{
			packet[8] = topLeft;
			packet[6] = topRight;
			packet[4] = bottomLeft;
			packet[2] = bottomRight;
		}
	}
	CTR_PSX_KEEP_VALUE(iconRotation);
	packet[0] = *orderingTable | BATTLE_GPU_TAG_LENGTH_POLY_FT4;
	*orderingTable = CtrGpu_PrimToOTLink24(packet);
	packetMem->cursor = packet + 10;

cleanup:
	// NOTE(aalhendi): Retail preserves s1 without otherwise using it. This
	// zero-byte clobber retains that prologue and epilogue shape.
	CTR_PSX_CLOBBER("$17");
}

void MM_Battle_Init(void)
{
	MM_BATTLE_TRANSITION_FRAME = BATTLE_TRANSITION_FRAME_COUNT;
	MM_BATTLE_TRANSITION_STATE = ENTERING_MENU;
}

// NOTE(aalhendi): Retail carries several menu values in fixed MIPS registers.
// Matching constraints preserve those lifetimes; CTR_NATIVE paths assign the
// equivalent C values directly.
void MM_Battle_MenuProc(struct RectMenu *unused)
{
	s16 settingIndex;
	struct TransitionMeta *transitionMeta;
	s16 menuHeight = 0;
#ifdef CTR_NATIVE
	s32 defaultMenuHeight = BATTLE_MENU_DEFAULT_HEIGHT;
#else
	s32 defaultMenuHeight;
#endif
	s16 lifeCountMenuHeight;
	(void)unused;
	// Restore the selected row for each battle setting menu.
	for (settingIndex = 0; settingIndex < BATTLE_SETTINGS_COUNT; settingIndex++)
	{
		MM_BATTLE_MENU_ARRAY[settingIndex]->rowSelected = MM_BATTLE_SETTINGS[settingIndex];
	}

	if (MM_BATTLE_TRANSITION_STATE == IN_MENU)
	{
		goto LAB_Battle_DrawMenu;
	}
	if (MM_BATTLE_TRANSITION_STATE < EXITING_MENU)
	{
		if (MM_BATTLE_TRANSITION_STATE == ENTERING_MENU)
		{
			goto LAB_Battle_EnterMenu;
		}
		goto LAB_Battle_DrawMenu;
	}
	if (MM_BATTLE_TRANSITION_STATE == EXITING_MENU)
	{
		goto LAB_Battle_ExitMenu;
	}
	goto LAB_Battle_DrawMenu;
LAB_Battle_EnterMenu:
	MM_TransitionInOut(MM_BATTLE_TRANSITIONS, (int)MM_BATTLE_TRANSITION_FRAME, BATTLE_ANIMATED_TRANSITION_META_COUNT);

	if (MM_BATTLE_TRANSITION_FRAME != 0)
	{
		MM_BATTLE_TRANSITION_FRAME--;
	}
	else
	{
		MM_BATTLE_TRANSITION_STATE = IN_MENU;
	}
	goto LAB_Battle_DrawMenu;
LAB_Battle_ExitMenu:
	MM_TransitionInOut(MM_BATTLE_TRANSITIONS, (int)MM_BATTLE_TRANSITION_FRAME, BATTLE_ANIMATED_TRANSITION_META_COUNT);

	MM_BATTLE_TRANSITION_FRAME++;
	if (BATTLE_TRANSITION_FRAME_COUNT < MM_BATTLE_TRANSITION_FRAME)
	{
		if (MM_BATTLE_TRANSITION_START_AFTER_EXIT != 0)
		{
			MM_DESIRED_MENU = &MM_MENU_QUEUE_LOAD_TRACK;
			return;
		}
		MM_TrackSelect_Init();
		MM_DESIRED_MENU = &MM_MENU_TRACK_SELECT;
		return;
	}
LAB_Battle_DrawMenu:
	// Validate the setup before accepting input or offering Start Battle.
	GAME_TRACKER->battleSetup.teamFlags = 0;

	GAME_TRACKER->battleSetup.numTeams = 0;
	{
		s16 index;
		for (index = 0; index < GAME_TRACKER->numPlyrNextGame; index++)
		{
			s16 teamFlag = 1 << GAME_TRACKER->battleSetup.teamOfEachPlayer[index];
			if ((GAME_TRACKER->battleSetup.teamFlags & teamFlag) == 0)
			{
				GAME_TRACKER->battleSetup.teamFlags |= teamFlag;
				GAME_TRACKER->battleSetup.numTeams++;
			}
		}

		for (index = 0; index < BATTLE_TEAM_COUNT; index++)
		{
			if ((GAME_TRACKER->battleSetup.teamFlags & (1 << index)) != 0)
			{
				GAME_TRACKER->battleSetup.pointsPerTeam[index] = 0;
			}
			else
			{
				GAME_TRACKER->battleSetup.pointsPerTeam[index] = BATTLE_INACTIVE_TEAM_POINTS;
			}
		}
	}
	if (GAME_TRACKER->battleSetup.numTeams >= 2)
	{
		if ((GAME_TRACKER->battleSetup.enabledWeapons & BATTLE_REQUIRED_WEAPON_FLAGS) != 0)
		{
			goto LAB_Battle_ValidSetup;
		}
	}
	if (MM_BATTLE_ROW_HIGHLIGHTED == BATTLE_ROW_START)
	{
		MM_BATTLE_ROW_HIGHLIGHTED = BATTLE_ROW_WEAPON_BOTTOM;
	}
LAB_Battle_ValidSetup:
{
	s16 playerIndex;
	for (playerIndex = 0; playerIndex < GAME_TRACKER->numPlyrNextGame; playerIndex++)
	{
		if (MM_BATTLE_ROW_HIGHLIGHTED == BATTLE_ROW_TEAMS)
		{
			if ((MM_GAME_BUTTON_TAPS[playerIndex] & BTN_LEFT) != 0)
			{
				if (BATTLE_VALID_TEAM_MIN < GAME_TRACKER->battleSetup.teamOfEachPlayer[playerIndex])
				{
					OtherFX_Play(0, 1);
					GAME_TRACKER->battleSetup.teamOfEachPlayer[playerIndex]--;
				}
				MM_GAME_BUTTON_TAPS[playerIndex] = 0;
			}
			if ((MM_GAME_BUTTON_TAPS[playerIndex] & BTN_RIGHT) != 0)
			{
				if (GAME_TRACKER->battleSetup.teamOfEachPlayer[playerIndex] < BATTLE_VALID_TEAM_MAX)
				{
					OtherFX_Play(0, 1);
					GAME_TRACKER->battleSetup.teamOfEachPlayer[playerIndex]++;
				}
				MM_GAME_BUTTON_TAPS[playerIndex] = 0;
			}
		}
	}
}

	CTR_PSX_CLOBBER("$18");
	// Process navigation only while the menu is fully in focus.
	if ((MM_BATTLE_TRANSITION_STATE == IN_MENU) && ((MM_GAME_BUTTON_TAPS[0] & BATTLE_MENU_INPUT) != 0))
	{
		if (((s16)MM_BATTLE_EXPAND_MENU) < 0)
		{
			int buttonTapP1 = MM_GAME_BUTTON_TAPS[0];
			register u16 previousHighlightedRow CTR_PSX_REGISTER("$20") = MM_BATTLE_ROW_HIGHLIGHTED;
			if ((buttonTapP1 & BTN_UP) != 0)
			{
				if ((MM_MENU_BATTLE_TYPE.rowSelected == BATTLE_LIFE_LIMIT_TYPE_ROW) && (((s16)previousHighlightedRow) == BATTLE_ROW_LIFE_COUNT))
				{
					goto LAB_800b1d7c;
				}
				else
				{
					u16 highlightedRow = (u16)MM_BATTLE_ROW_HIGHLIGHTED_UPDATE;
					highlightedRow--;
					MM_BATTLE_ROW_HIGHLIGHTED_UPDATE = highlightedRow;
					if (((s16)highlightedRow) < 0)
					{
						MM_BATTLE_ROW_HIGHLIGHTED_UPDATE = BATTLE_ROW_TYPE;
					}
				}
			}
			else if ((buttonTapP1 & BTN_DOWN) != 0)
			{
				register s32 battleTypeRow CTR_PSX_REGISTER("$4") = (s16)MM_MENU_BATTLE_TYPE.rowSelected;
				if ((battleTypeRow == BATTLE_LIFE_LIMIT_TYPE_ROW) && (((s16)previousHighlightedRow) == BATTLE_ROW_LIFE_COUNT))
				{
					MM_BATTLE_ROW_HIGHLIGHTED = battleTypeRow;
				}
				else
				{
					MM_BATTLE_ROW_HIGHLIGHTED++;
					if (BATTLE_ROW_START < MM_BATTLE_ROW_HIGHLIGHTED)
					{
						MM_BATTLE_ROW_HIGHLIGHTED = BATTLE_ROW_START;
					}
				}
			}
			else if ((buttonTapP1 & BTN_LEFT) != 0)
			{
				if (((u16)(previousHighlightedRow - BATTLE_ROW_WEAPON_TOP)) < BATTLE_WEAPON_ROW_COUNT)
				{
					MM_BATTLE_WEAPON_HIGHLIGHTED--;
				}
				else if ((MM_MENU_BATTLE_TYPE.rowSelected == BATTLE_LIFE_LIMIT_TYPE_ROW) && (((s16)previousHighlightedRow) == BATTLE_ROW_LIFE_COUNT))
				{
				LAB_800b1d7c:
				{
					register s32 transitionState CTR_PSX_REGISTER("$6");
					register u32 rowStatePage CTR_PSX_REGISTER("$16");
					CTR_PSX_CAPTURE_REGISTER(transitionState, MM_BATTLE_TRANSITION_STATE);
					CTR_PSX_CAPTURE_REGISTER(rowStatePage, 0);
					CTR_PSX_PAGE_LVALUE(u16, rowStatePage, MM_BATTLE_ROW_HIGHLIGHTED_PAGE_OFFSET, MM_BATTLE_ROW_HIGHLIGHTED) = transitionState;
				}
				}
			}
			else if ((buttonTapP1 & BTN_RIGHT) != 0)
			{
				if (((u16)(previousHighlightedRow - BATTLE_ROW_WEAPON_TOP)) < BATTLE_WEAPON_ROW_COUNT)
				{
					MM_BATTLE_WEAPON_HIGHLIGHTED++;
				}
				else if ((MM_MENU_BATTLE_TYPE.rowSelected == BATTLE_LIFE_LIMIT_TYPE_ROW) && (((s16)previousHighlightedRow) == MM_BATTLE_TRANSITION_STATE))
				{
					MM_BATTLE_ROW_HIGHLIGHTED = BATTLE_ROW_LIFE_COUNT;
				}
			}
			else if ((buttonTapP1 & BATTLE_CONFIRM_INPUT) != 0)
			{
				OtherFX_Play(1, 1);
				switch (MM_BATTLE_ROW_HIGHLIGHTED)
				{
					s32 originalEventTime;
					s32 weaponFlagIndex;
					s16 playerIndex;
					u32 battleModeFlags;
				case BATTLE_ROW_TYPE:

				case BATTLE_ROW_LENGTH:

				case BATTLE_ROW_LIFE_COUNT:
					MM_BATTLE_EXPAND_MENU = MM_BATTLE_ROW_HIGHLIGHTED;
					break;

				case BATTLE_ROW_WEAPON_TOP:

				case BATTLE_ROW_WEAPON_BOTTOM:
				{
					register u32 gameTrackerPage CTR_PSX_REGISTER("$6");
					register u32 weaponItemsAddress CTR_PSX_REGISTER("$4");
					register u32 rowPage CTR_PSX_REGISTER("$5");
					register s32 itemIndex CTR_PSX_REGISTER("$2");
					register s32 rowOffset CTR_PSX_REGISTER("$3");
					register struct GameTracker *gameTracker CTR_PSX_REGISTER("$5");
					register u32 enabledWeapons CTR_PSX_REGISTER("$3");
					CTR_PSX_LOAD_SYMBOL_PAGE(itemIndex, RETAIL_GAME_TRACKER_ASM_NAME);
					CTR_PSX_LOAD_SYMBOL_PAGE(gameTrackerPage, RETAIL_GAME_TRACKER_ASM_NAME);
					CTR_PSX_LOAD_SYMBOL_PAGE(weaponItemsAddress, MM_BATTLE_WEAPON_ITEMS_ASM_NAME);
					CTR_PSX_ADD_SYMBOL_LOW_IN_PLACE(weaponItemsAddress, MM_BATTLE_WEAPON_ITEMS_ASM_NAME, (u32)MM_BATTLE_WEAPON_ITEMS);
					itemIndex = CTR_PSX_PAGE_LVALUE(u16, itemIndex, MM_BATTLE_ROW_HIGHLIGHTED_PAGE_OFFSET, MM_BATTLE_ROW_HIGHLIGHTED);
					CTR_PSX_LOAD_SYMBOL_PAGE(rowPage, RETAIL_GAME_TRACKER_ASM_NAME);
					itemIndex = (s16)(itemIndex - BATTLE_ROW_WEAPON_TOP);
					rowOffset = itemIndex * BATTLE_WEAPONS_PER_ROW;
					itemIndex = CTR_PSX_PAGE_LVALUE(s16, rowPage, MM_BATTLE_WEAPON_HIGHLIGHTED_PAGE_OFFSET, MM_BATTLE_WEAPON_HIGHLIGHTED);
					gameTracker = CTR_PSX_PAGE_LVALUE(struct GameTracker *, gameTrackerPage, MM_GAME_TRACKER_PAGE_OFFSET, GAME_TRACKER);
					itemIndex += rowOffset;
					enabledWeapons = gameTracker->battleSetup.enabledWeapons;
					enabledWeapons ^= ((struct BattleWeaponMenuItem *)weaponItemsAddress)[itemIndex].enabledWeaponFlag;
					gameTracker->battleSetup.enabledWeapons = enabledWeapons;
				}
				break;

				case BATTLE_ROW_START:
					GAME_TRACKER->gameMode1 &= ~((POINT_LIMIT | LIFE_LIMIT) | TIME_LIMIT);
					battleModeFlags = MM_BATTLE_TYPE_MODE_FLAGS[MM_MENU_BATTLE_TYPE.rowSelected];
					GAME_TRACKER->gameMode1 |= battleModeFlags;
					if ((GAME_TRACKER->gameMode1 & TIME_LIMIT) != 0)
					{
						GAME_TRACKER->gameMode1 |= POINT_LIMIT;
					}
					if (((GAME_TRACKER->gameMode1 & LIFE_LIMIT) != 0) && (0 < MM_BATTLE_LIFE_TIME_LIMIT_MINUTES[MM_MENU_BATTLE_LENGTH_LIFE_TIME.rowSelected]))
					{
						GAME_TRACKER->gameMode1 |= TIME_LIMIT;
					}
					{
						struct GameTracker *battleTracker = GAME_TRACKER;
						register s32 eventTimeMinutes CTR_PSX_REGISTER("$2");
						register u32 gameTrackerPage CTR_PSX_REGISTER("$6");
						register struct GameTracker *timeTracker CTR_PSX_REGISTER("$5");
						register struct GameTracker *setupTracker CTR_PSX_REGISTER("$4");
						register struct GameTracker *setupTrackerWork CTR_PSX_REGISTER("$2");
						register u32 weaponFlagBase CTR_PSX_REGISTER("$7");
						register const u8 *lifeLimits CTR_PSX_REGISTER("$2");
						register const u8 *lifeLimitAddress CTR_PSX_REGISTER("$3");
						u32 lifeLimit;
						battleTracker->battleSetup.killLimit = MM_BATTLE_POINT_LIMIT_VALUES[MM_MENU_BATTLE_LENGTH_POINTS.rowSelected];
						if (MM_MENU_BATTLE_TYPE.rowSelected == BATTLE_TIME_LIMIT_TYPE_ROW)
						{
							eventTimeMinutes = MM_BATTLE_TIME_LIMIT_MINUTES[MM_MENU_BATTLE_LENGTH_TIME_TIME.rowSelected];
						}
						else
						{
							eventTimeMinutes = MM_BATTLE_LIFE_TIME_LIMIT_MINUTES[MM_MENU_BATTLE_LENGTH_LIFE_TIME.rowSelected];
						}
						gameTrackerPage = MM_GAME_TRACKER_PAGE_VALUE;
						timeTracker = CTR_PSX_PAGE_LVALUE(struct GameTracker *, gameTrackerPage, MM_GAME_TRACKER_PAGE_OFFSET, GAME_TRACKER);
						battleTracker->originalEventTime = eventTimeMinutes;
						CTR_PSX_MEMORY_BARRIER();
						originalEventTime = timeTracker->originalEventTime;
						if (0 < originalEventTime)
						{
							timeTracker->originalEventTime = originalEventTime * BATTLE_MINUTES_TO_EVENT_TIME;
						}
						weaponFlagIndex = 0;
						weaponFlagBase = 1;
						lifeLimits = MM_BATTLE_LIFE_LIMIT_VALUES;
						lifeLimitAddress = &lifeLimits[MM_MENU_BATTLE_LENGTH_LIFE_LIFE.rowSelected];
						CTR_PSX_LOAD_WORD_FROM_PAGE_AFTER(setupTrackerWork, gameTrackerPage, RETAIL_GAME_TRACKER_ASM_NAME, GAME_TRACKER, lifeLimitAddress);
						CTR_PSX_KEEP_VALUE(setupTrackerWork);
						lifeLimit = *lifeLimitAddress;
						setupTracker = setupTrackerWork;
						setupTracker->battleSetup.numWeapons = 0;
						setupTracker->battleSetup.lifeLimit = lifeLimit;
						do
						{
							register u32 enabledWeapons CTR_PSX_REGISTER("$2");
							register u32 weaponFlag CTR_PSX_REGISTER("$3");
							enabledWeapons = setupTracker->battleSetup.enabledWeapons;
							weaponFlag = weaponFlagBase << weaponFlagIndex;
							CTR_PSX_KEEP_VALUE(weaponFlag);
							if ((enabledWeapons & weaponFlag) != 0)
							{
								setupTracker->battleSetup.RNG_itemSetCustom[setupTracker->battleSetup.numWeapons] = weaponFlagIndex;
								setupTracker->battleSetup.numWeapons++;
							}
							weaponFlagIndex++;
						} while (weaponFlagIndex < BATTLE_WEAPON_FLAG_COUNT);
						MM_BATTLE_TRANSITION_START_AFTER_EXIT = 1;
						MM_BATTLE_TRANSITION_STATE = EXITING_MENU;
						for (playerIndex = 0; playerIndex < GAME_TRACKER->numPlyrNextGame; playerIndex++)
						{
							if (MM_BATTLE_TEAM_OF_EACH_PLAYER[playerIndex] != GAME_TRACKER->battleSetup.teamOfEachPlayer[playerIndex])
							{
								MainStats_ClearBattleVS();
							}
							MM_BATTLE_TEAM_OF_EACH_PLAYER[playerIndex] = GAME_TRACKER->battleSetup.teamOfEachPlayer[playerIndex];
						}

						MM_GAME_BUTTON_TAPS[1] = 0;
						MM_GAME_BUTTON_TAPS[2] = 0;
						MM_GAME_BUTTON_TAPS[3] = 0;
					}
				}
			}
			else if ((buttonTapP1 & BATTLE_BACK_INPUT) != 0)
			{
				OtherFX_Play(2, 1);
				MM_BATTLE_TRANSITION_START_AFTER_EXIT = 0;
				MM_BATTLE_TRANSITION_STATE = EXITING_MENU;
			}
			{
				register u32 highlightedRow CTR_PSX_REGISTER("$3") = (u16)MM_BATTLE_ROW_HIGHLIGHTED;
				if (((u32)(highlightedRow - BATTLE_ROW_WEAPON_TOP)) < BATTLE_WEAPON_ROW_COUNT)
				{
					register s32 highlightedWeaponRowOffset CTR_PSX_REGISTER("$6");
					register s32 highlightedWeapon CTR_PSX_REGISTER("$3");
					highlightedWeaponRowOffset = highlightedRow - (BATTLE_ROW_WEAPON_TOP - 1);
					if (MM_BATTLE_WEAPON_HIGHLIGHTED < 0)
					{
						MM_BATTLE_WEAPON_HIGHLIGHTED = 0;
					}
					highlightedWeapon = (s16)MM_BATTLE_WEAPON_HIGHLIGHTED;
					if (highlightedWeapon > (BATTLE_WEAPONS_PER_ROW - ((s16)highlightedWeaponRowOffset)))
					{
						register s32 clampedWeapon CTR_PSX_REGISTER("$2") = BATTLE_WEAPONS_PER_ROW - highlightedWeaponRowOffset;
						MM_BATTLE_WEAPON_HIGHLIGHTED = clampedWeapon;
					}
				}
			}
			{
				register u32 highlightedRowPage CTR_PSX_REGISTER("$2");
				register s32 currentHighlightedRow CTR_PSX_REGISTER("$3");
				register s32 previousHighlightedRowSigned CTR_PSX_REGISTER("$2");
				highlightedRowPage = MM_GAME_TRACKER_PAGE_VALUE;
				CTR_PSX_KEEP_VALUE(highlightedRowPage);
				currentHighlightedRow = CTR_PSX_PAGE_LVALUE(s16, highlightedRowPage, MM_BATTLE_ROW_HIGHLIGHTED_PAGE_OFFSET, MM_BATTLE_ROW_HIGHLIGHTED);
				previousHighlightedRowSigned = (s16)previousHighlightedRow;
				if (currentHighlightedRow != previousHighlightedRowSigned)
				{
					OtherFX_Play(0, 1);
				}
			}
		}
		else
		{
			struct RectMenu *dropdownMenu = (void *)0;
			if (MM_BATTLE_EXPAND_MENU == BATTLE_ROW_LENGTH)
			{
				goto LAB_Battle_DropdownLength;
			}
			if (MM_BATTLE_EXPAND_MENU < BATTLE_LIFE_LIMIT_TYPE_ROW)
			{
				if (MM_BATTLE_EXPAND_MENU == BATTLE_ROW_TYPE)
				{
					goto LAB_Battle_DropdownType;
				}
				goto LAB_Battle_DropdownSelected;
			}
			if (MM_BATTLE_EXPAND_MENU == BATTLE_ROW_LIFE_COUNT)
			{
				goto LAB_Battle_DropdownLifeCount;
			}
			goto LAB_Battle_DropdownSelected;
		LAB_Battle_DropdownType:
			dropdownMenu = &MM_MENU_BATTLE_TYPE;

			goto LAB_Battle_DropdownSelected;
		LAB_Battle_DropdownLength:
			if (MM_MENU_BATTLE_TYPE.rowSelected == BATTLE_TIME_LIMIT_TYPE_ROW)
			{
				goto LAB_Battle_DropdownTimeTime;
			}

			if (MM_MENU_BATTLE_TYPE.rowSelected < BATTLE_LIFE_LIMIT_TYPE_ROW)
			{
				if (MM_MENU_BATTLE_TYPE.rowSelected == BATTLE_ROW_TYPE)
				{
					goto LAB_Battle_DropdownPoints;
				}
				goto LAB_Battle_DropdownSelected;
			}
			if (MM_MENU_BATTLE_TYPE.rowSelected == BATTLE_LIFE_LIMIT_TYPE_ROW)
			{
				goto LAB_Battle_DropdownLifeTime;
			}
			goto LAB_Battle_DropdownSelected;
		LAB_Battle_DropdownPoints:
			dropdownMenu = &MM_MENU_BATTLE_LENGTH_POINTS;

			goto LAB_Battle_DropdownSelected;
		LAB_Battle_DropdownTimeTime:
			dropdownMenu = &MM_MENU_BATTLE_LENGTH_TIME_TIME;

			goto LAB_Battle_DropdownSelected;
		LAB_Battle_DropdownLifeTime:
			dropdownMenu = &MM_MENU_BATTLE_LENGTH_LIFE_TIME;

			goto LAB_Battle_DropdownSelected;
		LAB_Battle_DropdownLifeCount:
			dropdownMenu = &MM_MENU_BATTLE_LENGTH_LIFE_LIFE;

		LAB_Battle_DropdownSelected:
			if (dropdownMenu != ((void *)0))
			{
				RECTMENU_ProcessInput(dropdownMenu);
				if ((dropdownMenu->state & ONLY_DRAW_TITLE) != 0)
				{
					dropdownMenu->state &= ~ONLY_DRAW_TITLE;
					MM_BATTLE_EXPAND_MENU = -1;
				}
			}
		}
		RECTMENU_ClearInput();
	}
	{
		struct TransitionMeta *battleTransitions = MM_BATTLE_TRANSITIONS;
		s16 *menuHeightPtr;
		struct RectMenu *lengthMenu;
		s16 afterLengthY;
		s32 lengthRowY;
		union
		{
			u16 teamSegmentWidths[BATTLE_TEAM_COUNT];
			RECT16 weaponHighlightRect;
		} teamWidthScratch;
		union
		{
			s16 teamPlayerCounts[BATTLE_TEAM_COUNT];
			RECT16 weaponPanelInsetRect;
		} teamCountScratch;
		RECT16 teamHighlightRect;
		RECT16 teamColorRect;
		RECT16 menuPanelRect;
		register s16 weaponPanelY CTR_PSX_REGISTER("$16");
		register s32 menuStateMask CTR_PSX_REGISTER("$23");
		struct RectMenu *battleTypeMenu;
		// Draw the variable-height type, length, team, and weapon panels.
		DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_SETUP_BATTLE], battleTransitions[BATTLE_TITLE_META_INDEX].currX + BATTLE_TITLE_X_OFFSET,
		                   battleTransitions[BATTLE_TITLE_META_INDEX].currY + BATTLE_TITLE_Y_OFFSET, FONT_BIG, (s16)BATTLE_TITLE_TEXT_FLAGS);
		DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_TYPE], battleTransitions[BATTLE_ROW_TYPE_LABEL_META_INDEX].currX + BATTLE_LABEL_X_OFFSET,
		                   battleTransitions[BATTLE_ROW_TYPE_LABEL_META_INDEX].currY + BATTLE_TYPE_ROW_Y_OFFSET, FONT_BIG, BATTLE_LABEL_TEXT_FLAGS);
		battleTypeMenu = &MM_MENU_BATTLE_TYPE;
#ifdef CTR_NATIVE
		menuStateMask = ~(HIDE_ROW_HIGHLIGHT | SHOW_ONLY_HIGHLIT_ROW);
#else
		asm("li %0,-321" : "=r"(menuStateMask) : "r"(battleTypeMenu));
#endif
		transitionMeta = battleTransitions;
		CTR_PSX_OBSERVE_VALUE(transitionMeta);
		battleTypeMenu->state &= menuStateMask;
		if (MM_BATTLE_EXPAND_MENU != BATTLE_ROW_TYPE)
		{
			battleTypeMenu->state |= SHOW_ONLY_HIGHLIT_ROW;
		}
		if (MM_BATTLE_ROW_HIGHLIGHTED != BATTLE_ROW_TYPE)
		{
			battleTypeMenu->state |= HIDE_ROW_HIGHLIGHT;
		}
		RECTMENU_DrawSelf(battleTypeMenu, (s16)(((u16)battleTransitions[BATTLE_ROW_TYPE_MENU_META_INDEX].currX) + BATTLE_MENU_X_OFFSET),
		                  battleTransitions[BATTLE_ROW_TYPE_MENU_META_INDEX].currY + BATTLE_TYPE_ROW_Y_OFFSET, BATTLE_WIDE_MENU_WIDTH);
		menuHeight = BATTLE_MENU_DEFAULT_HEIGHT;
		menuHeightPtr = &menuHeight;
		RECTMENU_GetHeight(battleTypeMenu, menuHeightPtr, 0);
		lengthRowY = ((u16)menuHeight) + BATTLE_LENGTH_ROW_Y_OFFSET;
		afterLengthY = lengthRowY;
		DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_LENGTH], battleTransitions[BATTLE_ROW_LENGTH_LABEL_META_INDEX].currX + BATTLE_LABEL_X_OFFSET,
		                   (battleTransitions[BATTLE_ROW_LENGTH_LABEL_META_INDEX].currY + lengthRowY) + BATTLE_LENGTH_LABEL_Y_OFFSET, FONT_BIG,
		                   BATTLE_LABEL_TEXT_FLAGS);
		if (battleTypeMenu->rowSelected == BATTLE_TIME_LIMIT_TYPE_ROW)
		{
			goto LAB_Battle_TimeLength;
		}
		if (((s16)battleTypeMenu->rowSelected) < BATTLE_LIFE_LIMIT_TYPE_ROW)
		{
			if (battleTypeMenu->rowSelected == BATTLE_ROW_TYPE)
			{
				goto LAB_Battle_PointLength;
			}
			goto LAB_800b25f0;
		}
		if (battleTypeMenu->rowSelected == BATTLE_LIFE_LIMIT_TYPE_ROW)
		{
			goto LAB_Battle_LifeLength;
		}
		goto LAB_800b25f0;
	LAB_Battle_PointLength:
		lengthMenu = &MM_MENU_BATTLE_LENGTH_POINTS;

		goto LAB_Battle_DrawLength;
	LAB_Battle_TimeLength:
		lengthMenu = &MM_MENU_BATTLE_LENGTH_TIME_TIME;

	LAB_Battle_DrawLength:
	{
		register s32 selectedLengthRow CTR_PSX_REGISTER("$5");
		register s32 visibleState CTR_PSX_REGISTER("$2");
		register s32 expandedRow CTR_PSX_REGISTER("$3");
		register s32 highlightedRow CTR_PSX_REGISTER("$2");
#ifdef CTR_NATIVE
		selectedLengthRow = BATTLE_ROW_LENGTH;
		visibleState = lengthMenu->state & menuStateMask;
		expandedRow = MM_BATTLE_EXPAND_MENU;
#else
		asm("" : "=r"(selectedLengthRow));
		asm("lw %0,8(%2)\n\tlh %1,%%lo(" MM_BATTLE_EXPAND_MENU_ASM_NAME ")($21)\n\tand %0,%0,$23" : "=&r"(visibleState), "=&r"(expandedRow) : "r"(lengthMenu));
#endif
		lengthMenu->state = visibleState;
		if (expandedRow != selectedLengthRow)
		{
			lengthMenu->state |= SHOW_ONLY_HIGHLIT_ROW;
		}
#ifdef CTR_NATIVE
		highlightedRow = MM_BATTLE_ROW_HIGHLIGHTED;
#else
		asm("lh %0,%%lo(" MM_BATTLE_ROW_HIGHLIGHTED_ASM_NAME ")($30)" : "=r"(highlightedRow));
#endif
		if (highlightedRow != selectedLengthRow)
		{
			lengthMenu->state |= HIDE_ROW_HIGHLIGHT;
		}
		defaultMenuHeight = BATTLE_MENU_DEFAULT_HEIGHT;
		RECTMENU_DrawSelf(lengthMenu, (s16)(((u16)battleTransitions[BATTLE_ROW_LENGTH_MENU_META_INDEX].currX) + BATTLE_MENU_X_OFFSET),
		                  (battleTransitions[BATTLE_ROW_LENGTH_MENU_META_INDEX].currY + lengthRowY) + BATTLE_LENGTH_LABEL_Y_OFFSET, BATTLE_WIDE_MENU_WIDTH);
		menuHeight = defaultMenuHeight;
		RECTMENU_GetHeight(lengthMenu, menuHeightPtr, 0);
		afterLengthY = menuHeight + lengthRowY;
		goto LAB_Battle_AfterLength;
	}

	LAB_Battle_LifeLength:
		MM_MENU_BATTLE_LENGTH_LIFE_TIME.state &= menuStateMask;

		if (MM_BATTLE_EXPAND_MENU != BATTLE_ROW_LENGTH)
		{
			MM_MENU_BATTLE_LENGTH_LIFE_TIME.state |= SHOW_ONLY_HIGHLIT_ROW;
		}
		{
			register s32 highlightedRow CTR_PSX_REGISTER("$2");
#ifdef CTR_NATIVE
			highlightedRow = MM_BATTLE_ROW_HIGHLIGHTED;
#else
			asm("lh %0,%%lo(" MM_BATTLE_ROW_HIGHLIGHTED_ASM_NAME ")($30)" : "=r"(highlightedRow));
#endif
			if (highlightedRow != BATTLE_ROW_LENGTH)
			{
				MM_MENU_BATTLE_LENGTH_LIFE_TIME.state |= HIDE_ROW_HIGHLIGHT;
			}
		}
		RECTMENU_DrawSelf(&MM_MENU_BATTLE_LENGTH_LIFE_TIME, (s16)(((u16)battleTransitions[BATTLE_ROW_LENGTH_MENU_META_INDEX].currX) + BATTLE_MENU_X_OFFSET),
		                  (battleTransitions[BATTLE_ROW_LENGTH_MENU_META_INDEX].currY + lengthRowY) + BATTLE_LENGTH_LABEL_Y_OFFSET,
		                  BATTLE_LENGTH_DUAL_MENU_WIDTH);
		MM_MENU_BATTLE_LENGTH_LIFE_LIFE.state &= menuStateMask;
		if (MM_BATTLE_EXPAND_MENU != BATTLE_ROW_LIFE_COUNT)
		{
			MM_MENU_BATTLE_LENGTH_LIFE_LIFE.state |= SHOW_ONLY_HIGHLIT_ROW;
		}
		if (MM_BATTLE_ROW_HIGHLIGHTED != BATTLE_ROW_LIFE_COUNT)
		{
			MM_MENU_BATTLE_LENGTH_LIFE_LIFE.state |= HIDE_ROW_HIGHLIGHT;
		}
		RECTMENU_DrawSelf(
		    &MM_MENU_BATTLE_LENGTH_LIFE_LIFE, (s16)(((u16)battleTransitions[BATTLE_ROW_LENGTH_MENU_META_INDEX].currX) + BATTLE_LENGTH_DUAL_MENU_X_OFFSET),
		    (battleTransitions[BATTLE_ROW_LENGTH_MENU_META_INDEX].currY + lengthRowY) + BATTLE_LENGTH_LABEL_Y_OFFSET, BATTLE_LENGTH_DUAL_MENU_WIDTH);
		menuHeight = BATTLE_MENU_DEFAULT_HEIGHT;
		RECTMENU_GetHeight(&MM_MENU_BATTLE_LENGTH_LIFE_TIME, menuHeightPtr, 0);
		lifeCountMenuHeight = defaultMenuHeight;
		RECTMENU_GetHeight(&MM_MENU_BATTLE_LENGTH_LIFE_LIFE, &lifeCountMenuHeight, 0);
		{
			s16 selectedHeight = lifeCountMenuHeight;
			if (lifeCountMenuHeight < menuHeight)
			{
				selectedHeight = menuHeight;
			}
			afterLengthY = ((u16)selectedHeight) + lengthRowY;
		}
	LAB_Battle_AfterLength:
	{
		s32 teamPanelX;
		s32 accumulatedTeamWidth;
		s32 remainingTeamWidth;
		register s32 remainingWidthResult CTR_PSX_REGISTER("$2");
		s16 teamIndex;
		u32 sharedTeamPadding;
		register s32 teamFont CTR_PSX_REGISTER("$7");
	LAB_800b25f0:
		teamFont = FONT_BIG;

		teamPanelX = BATTLE_TEAM_PANEL_START_X;
		CTR_PSX_OBSERVE_VALUE(teamFont);
		DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_TEAMS], MM_BATTLE_TRANSITIONS[BATTLE_ROW_TEAM_LABEL_META_INDEX].currX + BATTLE_LABEL_X_OFFSET,
		                   (MM_BATTLE_TRANSITIONS[BATTLE_ROW_TEAM_LABEL_META_INDEX].currY + afterLengthY) + BATTLE_TEAM_LABEL_Y_OFFSET, teamFont,
		                   BATTLE_LABEL_TEXT_FLAGS);
		teamIndex = 0;
		CTR_PSX_COPY_VALUE(accumulatedTeamWidth, teamIndex);
		// Divide the team panel width among the teams currently in use.
		do
		{
			s16 playerIndex;
			accumulatedTeamWidth += BATTLE_TEAM_SEGMENT_BASE_WIDTH;
			teamCountScratch.teamPlayerCounts[teamIndex] = 0;
			teamWidthScratch.teamSegmentWidths[teamIndex] = BATTLE_TEAM_SEGMENT_BASE_WIDTH;
			for (playerIndex = 0; playerIndex < GAME_TRACKER->numPlyrNextGame; playerIndex++)
			{
				s32 playerTeam = GAME_TRACKER->battleSetup.teamOfEachPlayer[playerIndex];
				if (playerTeam == teamIndex)
				{
					teamCountScratch.teamPlayerCounts[playerTeam]++;
					teamWidthScratch.teamSegmentWidths[playerTeam] += BATTLE_TEAM_PLAYER_WIDTH;
					accumulatedTeamWidth += BATTLE_TEAM_PLAYER_WIDTH;
				}
			}

			teamIndex++;
		} while (teamIndex < BATTLE_TEAM_COUNT);
		remainingWidthResult = BATTLE_TEAM_PANEL_TOTAL_WIDTH - accumulatedTeamWidth;
		CTR_PSX_KEEP_VALUE(remainingWidthResult);
		remainingTeamWidth = remainingWidthResult;
		{
			register s16 truncatedRemainingTeamWidth CTR_PSX_REGISTER("$2");
			s32 paddedRemainingTeamWidth;
#ifdef CTR_NATIVE
			truncatedRemainingTeamWidth = (s16)remainingWidthResult;
#else
			asm("" : "=r"(truncatedRemainingTeamWidth) : "0"(remainingWidthResult));
#endif
			paddedRemainingTeamWidth = truncatedRemainingTeamWidth + 3;
			if (paddedRemainingTeamWidth < 0)
			{
				paddedRemainingTeamWidth = truncatedRemainingTeamWidth + 6;
			}
			sharedTeamPadding = ((u32)paddedRemainingTeamWidth) >> 2;
		}
		for (teamIndex = 0; teamIndex < BATTLE_TEAM_COUNT; teamIndex++)
		{
			s32 nextRemainingTeamWidth;
			teamWidthScratch.teamSegmentWidths[teamIndex] += sharedTeamPadding;
			nextRemainingTeamWidth = remainingTeamWidth - sharedTeamPadding;
			remainingTeamWidth = nextRemainingTeamWidth;
			if (((s32)(sharedTeamPadding << 16)) > ((s32)(((u32)nextRemainingTeamWidth) << 16)))
			{
				sharedTeamPadding = nextRemainingTeamWidth;
			}
		}

		for (teamIndex = 0; teamIndex < BATTLE_TEAM_COUNT; teamIndex++)
		{
			s16 playerIndex;
			register s32 teamSegmentWidth CTR_PSX_REGISTER("$19");
			int playerIconX;
			teamSegmentWidth = teamWidthScratch.teamSegmentWidths[teamIndex];
			playerIconX =
			    (teamPanelX + (((s16)teamSegmentWidth) / 2)) + (((int)teamCountScratch.teamPlayerCounts[teamIndex]) * BATTLE_TEAM_PLAYER_CENTER_OFFSET);
			for (playerIndex = 0; playerIndex < GAME_TRACKER->numPlyrNextGame; playerIndex++)
			{
				if (GAME_TRACKER->battleSetup.teamOfEachPlayer[playerIndex] == teamIndex)
				{
					s32 shiftedIconX = ((u32)playerIconX) << 16;
					playerIconX = playerIconX + BATTLE_TEAM_PLAYER_WIDTH;
					MM_Battle_DrawIcon_Character(GAME_TRACKER->ptrIcons[GAME_CHARACTER_METADATA[GAME_CHARACTER_IDS[playerIndex]].iconID],
					                             ((int)MM_BATTLE_TRANSITIONS[BATTLE_ROW_TEAM_META_INDEX].currX) + (shiftedIconX >> 16),
					                             (((int)MM_BATTLE_TRANSITIONS[BATTLE_ROW_TEAM_META_INDEX].currY) + ((int)afterLengthY)) +
					                                 BATTLE_TEAM_PLAYER_ICON_Y_OFFSET,
					                             &GAME_TRACKER->backBuffer->primMem, GAME_TRACKER->pushBuffer_UI.ptrOT, 1, BATTLE_ICON_SCALE);
				}
			}

			teamColorRect.w = teamSegmentWidth;
			teamColorRect.h = BATTLE_TEAM_COLOR_H;
			teamColorRect.x = MM_BATTLE_TRANSITIONS[BATTLE_ROW_TEAM_META_INDEX].currX + ((s16)teamPanelX);
			teamColorRect.y = (MM_BATTLE_TRANSITIONS[BATTLE_ROW_TEAM_META_INDEX].currY + afterLengthY) + BATTLE_TEAM_COLOR_Y_OFFSET;
			teamPanelX = teamPanelX + ((u32)teamSegmentWidth);
			MM_DRAW_SOLID_BOX_WITH_PRIM_MEM(&teamColorRect, (const Color *)MM_COLOR_POINTERS[PLAYER_BLUE + teamIndex], GAME_TRACKER->backBuffer->otMem.uiOT,
			                                &GAME_TRACKER->backBuffer->primMem);
		}

		if (MM_BATTLE_ROW_HIGHLIGHTED == BATTLE_ROW_TEAMS)
		{
			teamHighlightRect.w = BATTLE_TEAM_HIGHLIGHT_W;
			teamHighlightRect.h = BATTLE_TEAM_HIGHLIGHT_H;
			teamHighlightRect.x = MM_BATTLE_TRANSITIONS[BATTLE_ROW_TEAM_META_INDEX].currX + BATTLE_MENU_X_OFFSET;
			teamHighlightRect.y = (MM_BATTLE_TRANSITIONS[BATTLE_ROW_TEAM_META_INDEX].currY + afterLengthY) + BATTLE_TEAM_HIGHLIGHT_Y_OFFSET;
			MM_DRAW_CLEAR_BOX(&teamHighlightRect, &GAME_MENU_HIGHLIGHT, TRANS_50_DECAL, GAME_TRACKER->backBuffer->otMem.uiOT,
			                  &GAME_TRACKER->backBuffer->primMem);
		}
		menuPanelRect.w = BATTLE_TEAM_PANEL_W;
		menuPanelRect.h = BATTLE_TEAM_PANEL_H;
		menuPanelRect.x = MM_BATTLE_TRANSITIONS[BATTLE_ROW_TEAM_META_INDEX].currX + BATTLE_TEAM_PANEL_X_OFFSET;
		menuPanelRect.y = MM_BATTLE_TRANSITIONS[BATTLE_ROW_TEAM_META_INDEX].currY + afterLengthY;
		teamIndex = afterLengthY + BATTLE_WEAPON_PANEL_Y_OFFSET;
#ifndef CTR_NATIVE
		asm("" : "+r"(afterLengthY) : "m"(menuPanelRect.y));
#endif
		weaponPanelY = teamIndex;
		afterLengthY = weaponPanelY;
		afterLengthY += 0x4a;
		RECTMENU_DrawInnerRect(&menuPanelRect, 0, GAME_TRACKER->backBuffer->otMem.uiOT);
	}

		{
			s16 flashingErrorColor;
			register s32 errorLine1 CTR_PSX_REGISTER("$3");
			s32 errorLine2;
			s32 weaponPanelOffset;
			s32 errorState;
			register struct GameTracker *battleTracker CTR_PSX_REGISTER("$4");
			register s32 weaponIndex CTR_PSX_REGISTER("$19");
			DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_WEAPONS], MM_BATTLE_TRANSITIONS[BATTLE_ROW_WEAPON_LABEL_META_INDEX].currX + BATTLE_LABEL_X_OFFSET,
			                   (MM_BATTLE_TRANSITIONS[BATTLE_ROW_WEAPON_LABEL_META_INDEX].currY + weaponPanelY) + 0x1a, FONT_BIG, BATTLE_LABEL_TEXT_FLAGS);
			flashingErrorColor = BATTLE_ERROR_COLOR_A;
			if ((MM_FRAME_COUNTER & BATTLE_ERROR_COLOR_FRAME_BIT) != 0)
			{
				flashingErrorColor = BATTLE_ERROR_COLOR_B;
			}
			errorLine1 = 0;
			errorLine2 = 0;
			battleTracker = GAME_TRACKER;
			if ((battleTracker->battleSetup.enabledWeapons & BATTLE_REQUIRED_WEAPON_FLAGS) == 0)
			{
				errorLine1 = LNG_WEAPONS_ERROR_LINE1;
				errorLine2 = LNG_AT_LEAST_ONE_WEAPON;
			}
			else if (battleTracker->battleSetup.numTeams < 2)
			{
				errorLine1 = LNG_TEAMS_ERROR_LINE1;
				errorLine2 = LNG_TWO_OR_MORE_TEAMS;
			}
			errorState = errorLine1;
			if (errorState != 0)
			{
				DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[errorState], BATTLE_ERROR_TEXT_X, afterLengthY - 0xa, FONT_BIG, (int)flashingErrorColor);
				DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[errorLine2], BATTLE_ERROR_TEXT_X, afterLengthY + 6, FONT_BIG, (int)flashingErrorColor);
			}
			else
			{
				MM_MENU_BATTLE_START_GAME.state &= ~(HIDE_ROW_HIGHLIGHT | SHOW_ONLY_HIGHLIT_ROW);
				if (MM_BATTLE_EXPAND_MENU != BATTLE_ROW_START)
				{
					MM_MENU_BATTLE_START_GAME.state |= SHOW_ONLY_HIGHLIT_ROW;
				}
				if (MM_BATTLE_ROW_HIGHLIGHTED != BATTLE_ROW_START)
				{
					MM_MENU_BATTLE_START_GAME.state |= HIDE_ROW_HIGHLIGHT;
				}
				RECTMENU_DrawSelf(&MM_MENU_BATTLE_START_GAME, (s16)(((u16)MM_BATTLE_TRANSITIONS[BATTLE_ROW_START_META_INDEX].currX) + BATTLE_MENU_X_OFFSET),
				                  (MM_BATTLE_TRANSITIONS[BATTLE_ROW_START_META_INDEX].currY + afterLengthY) + 4, BATTLE_WIDE_MENU_WIDTH);
				menuHeight = BATTLE_MENU_DEFAULT_HEIGHT;
				RECTMENU_GetHeight(&MM_MENU_BATTLE_START_GAME, &menuHeight, 0);
			}
			weaponIndex = 0;
			{
				register u32 weaponTrackerPage CTR_PSX_REGISTER("$21");
				weaponTrackerPage = MM_GAME_TRACKER_PAGE_VALUE;
				menuPanelRect.w = BATTLE_WEAPON_PANEL_W;
				menuPanelRect.h = BATTLE_WEAPON_PANEL_H;
				menuPanelRect.x = MM_BATTLE_TRANSITIONS[BATTLE_ROW_WEAPON_PANEL_META_INDEX].currX + BATTLE_WEAPON_PANEL_X_OFFSET;
				weaponPanelOffset = 0x4a;
				menuPanelRect.y = MM_BATTLE_TRANSITIONS[BATTLE_ROW_WEAPON_PANEL_META_INDEX].currY + (afterLengthY - weaponPanelOffset);
				// Draw both rows of weapon toggles.
				for (;;)
				{
					const Color *weaponColor;
					u32 enabledWeapons;
					u32 weaponFlag;
					u32 weaponTextColor;
					register s32 weaponGridY CTR_PSX_REGISTER("$4");
					s32 weaponGridX;
					s16 weaponPosX;
					s16 weaponPosY;
					s16 weaponRow;
					const struct BattleWeaponMenuItem *weaponItem;
					weaponRow = ((s16)weaponIndex) / BATTLE_WEAPONS_PER_ROW;
					weaponGridX = ((((s16)weaponIndex) % BATTLE_WEAPONS_PER_ROW) * BATTLE_WEAPON_GRID_X_STEP) + (weaponRow * BATTLE_WEAPON_GRID_ROW_STAGGER_X);
					weaponItem = &MM_BATTLE_WEAPON_ITEMS[(s16)weaponIndex];
					weaponColor = &MM_BATTLE_WEAPON_ENABLED_COLOR;
					weaponTextColor = BATTLE_WEAPON_ENABLED_TEXT_COLOR;
					weaponFlag = weaponItem->enabledWeaponFlag;
					enabledWeapons =
					    CTR_PSX_PAGE_LVALUE(struct GameTracker *, weaponTrackerPage, MM_GAME_TRACKER_PAGE_OFFSET, GAME_TRACKER)->battleSetup.enabledWeapons;
					CTR_PSX_KEEP_VALUE(enabledWeapons);
					weaponGridY = weaponRow * BATTLE_WEAPON_GRID_Y_STEP;
					if ((enabledWeapons & weaponFlag) == 0)
					{
						weaponColor = &MM_BATTLE_WEAPON_DISABLED_COLOR;
						weaponTextColor = BATTLE_WEAPON_DISABLED_TEXT_COLOR;
					}
					weaponPosX = ((u32)menuPanelRect.x) + (weaponGridX + BATTLE_WEAPON_GRID_X_OFFSET);
					weaponPosY = ((u32)menuPanelRect.y) + (weaponGridY + BATTLE_WEAPON_GRID_Y_OFFSET);
					CTR_PSX_OBSERVE_VALUE(weaponGridY);
					if (((weaponIndex - 7U) & 0xffffU) < 2U)
					{
						DecalFont_DrawLine(&R230.s_3[0], weaponPosX, weaponPosY, 2, weaponTextColor);
					}
					MM_Battle_DrawIcon_Weapon(
					    CTR_PSX_PAGE_LVALUE(struct GameTracker *, weaponTrackerPage, MM_GAME_TRACKER_PAGE_OFFSET, GAME_TRACKER)->ptrIcons[weaponItem->iconID],
					    weaponPosX, weaponPosY,
					    &CTR_PSX_PAGE_LVALUE(struct GameTracker *, weaponTrackerPage, MM_GAME_TRACKER_PAGE_OFFSET, GAME_TRACKER)->backBuffer->primMem,
					    (u32 *)CTR_PSX_PAGE_LVALUE(struct GameTracker *, weaponTrackerPage, MM_GAME_TRACKER_PAGE_OFFSET, GAME_TRACKER)->pushBuffer_UI.ptrOT, 1,
					    BATTLE_ICON_SCALE, BATTLE_WEAPON_ICON_ROTATE_RIGHT, (const u32 *)weaponColor);
					{
						register s32 nextWeaponIndex CTR_PSX_REGISTER("$2");
						nextWeaponIndex = weaponIndex + 1;
						CTR_PSX_KEEP_VALUE(nextWeaponIndex);
						weaponIndex = nextWeaponIndex;
						CTR_PSX_KEEP_VALUE(nextWeaponIndex);
						if (((s16)nextWeaponIndex) >= BATTLE_WEAPON_ITEM_COUNT)
						{
							break;
						}
					}
				}
			}
			{
				u16 highlightedRow = MM_BATTLE_ROW_HIGHLIGHTED_UNSIGNED;
				if ((((u32)highlightedRow) - BATTLE_ROW_WEAPON_TOP) < BATTLE_WEAPON_ROW_COUNT)
				{
					s32 highlightedWeaponX;
					s32 signedHighlightedRow;
					register s32 weaponHighlightRectX CTR_PSX_REGISTER("$5");
					signedHighlightedRow = (s16)highlightedRow;
					highlightedWeaponX = menuPanelRect.x + (MM_BATTLE_WEAPON_HIGHLIGHTED * BATTLE_WEAPON_GRID_X_STEP);
					weaponHighlightRectX = highlightedWeaponX + BATTLE_WEAPON_HIGHLIGHT_LEFT_X_OFFSET;
					if (signedHighlightedRow == BATTLE_ROW_WEAPON_BOTTOM)
					{
						weaponHighlightRectX = highlightedWeaponX + BATTLE_WEAPON_HIGHLIGHT_RIGHT_X_OFFSET;
					}
					teamWidthScratch.weaponHighlightRect.w = BATTLE_WEAPON_HIGHLIGHT_W;
					teamWidthScratch.weaponHighlightRect.h = BATTLE_WEAPON_HIGHLIGHT_H;
					teamWidthScratch.weaponHighlightRect.x = weaponHighlightRectX;
					teamWidthScratch.weaponHighlightRect.y =
					    (menuPanelRect.y + ((signedHighlightedRow - BATTLE_ROW_WEAPON_TOP) * BATTLE_WEAPON_GRID_Y_STEP)) + BATTLE_WEAPON_HIGHLIGHT_Y_OFFSET;
					MM_DRAW_CLEAR_BOX(&teamWidthScratch.weaponHighlightRect, &GAME_MENU_HIGHLIGHT, TRANS_50_DECAL, GAME_TRACKER->backBuffer->otMem.uiOT,
					                  &GAME_TRACKER->backBuffer->primMem);
				}
			}
			teamCountScratch.weaponPanelInsetRect.x = menuPanelRect.x + BATTLE_WEAPON_PANEL_INSET_X;
			teamCountScratch.weaponPanelInsetRect.y = menuPanelRect.y + BATTLE_WEAPON_PANEL_INSET_Y;
			teamCountScratch.weaponPanelInsetRect.w = menuPanelRect.w - BATTLE_WEAPON_PANEL_INSET_W_SHRINK;
			teamCountScratch.weaponPanelInsetRect.h = menuPanelRect.h - BATTLE_WEAPON_PANEL_INSET_H_SHRINK;
			MM_DRAW_CLEAR_BOX(&teamCountScratch.weaponPanelInsetRect, &MM_BATTLE_WEAPON_PANEL_COLOR, TRANS_50_DECAL, GAME_TRACKER->backBuffer->otMem.uiOT,
			                  &GAME_TRACKER->backBuffer->primMem);
			RECTMENU_DrawInnerRect(&menuPanelRect, 0, GAME_TRACKER->backBuffer->otMem.uiOT);
		}
	}
	// Preserve the selected rows for the next frame.
	for (settingIndex = 0; settingIndex < BATTLE_SETTINGS_COUNT; settingIndex++)
	{
		MM_BATTLE_SETTINGS[settingIndex] = MM_BATTLE_MENU_ARRAY[settingIndex]->rowSelected;
	}

	return;
}
