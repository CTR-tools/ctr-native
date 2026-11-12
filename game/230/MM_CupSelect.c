#include <common.h>
#include "MM_CustomCupSelect.c"

// NOTE(aalhendi): Keeping this shared lookup visible at the call site lets
// GCC reuse $v1 after storing the ordering-table argument.
#define MM_CUP_SELECT_STAR_COLOR(colorTableBase, colorIndex) (*(Color **)(((s32)(colorIndex) * (s32)sizeof(*MM_COLOR_POINTERS)) + (colorTableBase)))

static inline s32 MM_CupSelect_GetStarY(s16 starIndex, s32 startY)
{
	register s32 starY CTR_PSX_REGISTER("$6");
	starY = (s32)starIndex * MM_CUP_SELECT_STAR_Y_STEP;
	starY += MM_CUP_SELECT_STAR_Y_OFFSET;
	return (s16)startY + starY;
}

void MM_CupSelect_Init(void)
{
	MM_CustomCupSelect_Init();
	// reset transition data
	MM_CUP_TRANSITION_FRAME = MM_CUP_SELECT_INITIAL_TRANSITION_FRAMES;
	MM_CUP_TRANSITION_STATE = ENTERING_MENU;

	// disable menu callback execution while the cup menu transitions in
	MM_MENU_CUP_SELECT.state &= ~(EXECUTE_FUNCPTR);
	// allow the callback to keep drawing while input stays blocked
	MM_MENU_CUP_SELECT.state |= DISABLE_INPUT_ALLOW_FUNCPTRS;
}

void MM_CupSelect_MenuProc(struct RectMenu *menu)
{
	if (MM_CustomCupSelect_Update(menu)) { return; }
	// NOTE(aalhendi): Register-shaped intermediates preserve GCC 2.8.1's
	// retail schedule while CTR_PSX_* keeps the native path semantic.
	register struct RectMenu *menuState CTR_PSX_REGISTER("$23") = menu;
	register s16 cupIndex CTR_PSX_REGISTER("$18");
	s16 driverIndex;
	RECT cupBox;

	if (menuState->funcState == RECTMENU_FUNC_STATE_INPUT)
	{
		if (menuState->rowSelected == -1)
		{
			MM_CUP_TRANSITION_STATE = EXITING_MENU;
			MM_CUP_TRANSITION_START_AFTER_EXIT = false;
		}
		else
		{
			MM_CUP_TRANSITION_STATE = EXITING_MENU;
			MM_CUP_TRANSITION_START_AFTER_EXIT = true;
		}

		MM_MENU_CUP_SELECT.state &= ~(EXECUTE_FUNCPTR);
		MM_MENU_CUP_SELECT.state |= DISABLE_INPUT_ALLOW_FUNCPTRS;
		return;
	}

	switch (MM_CUP_TRANSITION_STATE)
	{
	case ENTERING_MENU:
	{
		MM_TransitionInOut(MM_CUP_TRANSITIONS, MM_CUP_TRANSITION_FRAME, MM_CUP_SELECT_LERP_FRAMES);

		if (MM_CUP_TRANSITION_FRAME != 0)
		{
			MM_CUP_TRANSITION_FRAME--;
			break;
		}

		// menu is now in focus
		MM_CUP_TRANSITION_STATE = IN_MENU;
		MM_MENU_CUP_SELECT.state &= ~(DISABLE_INPUT_ALLOW_FUNCPTRS);
		MM_MENU_CUP_SELECT.state |= EXECUTE_FUNCPTR;
		break;
	}

	case IN_MENU:
		break;

	case EXITING_MENU:
	{
		MM_TransitionInOut(MM_CUP_TRANSITIONS, MM_CUP_TRANSITION_FRAME, MM_CUP_SELECT_LERP_FRAMES);

		// increase frame count
		MM_CUP_TRANSITION_FRAME++;

		// if more than 12 frames pass
		if (MM_CUP_TRANSITION_FRAME > MM_CUP_SELECT_TRANSITION_OUT_DONE_FRAME)
		{
			// if cup selected
			if (MM_CUP_TRANSITION_START_AFTER_EXIT != 0)
			{
				CustomCupRace_Clear(&sdata->customCupRace);
				// set cupID to the cup selected
				GAME_TRACKER->cup.cupID = menuState->rowSelected;

				// set track index to zero, to go to first track
				GAME_TRACKER->cup.trackIndex = 0;

				// loop through 8 drivers
				for (driverIndex = 0; driverIndex < MM_CUP_SELECT_DRIVER_SLOT_COUNT; driverIndex++)
				{
					// set all points for all 8 drivers to zero
					GAME_TRACKER->cup.points[driverIndex] = 0;
				}

				// set current level
				GAME_TRACKER->currLEV = MM_ARCADE_CUPS[GAME_TRACKER->cup.cupID].CupTrack[GAME_TRACKER->cup.trackIndex].trackID;

				// passthrough Menu for the function
				MM_DESIRED_MENU = &MM_MENU_QUEUE_LOAD_TRACK;
				return;
			}

			if (g_config.customCups && (GAME_TRACKER->gameMode1 & ARCADE_MODE) != 0)
			{
				s_cupSelectPage = CUSTOM_CUP_SELECT_KIND;
				s_cupKindRow = 0;
				s_cupKindScroll.offset = 0;
				RECTMENU_ClearInput();
				return;
			}

			// return to character selection
			MM_DESIRED_MENU = &MM_MENU_CHARACTER_SELECT;

			MM_Characters_RestoreIDs();
			return;
		}
		break;
	}
	}

	{
		struct TransitionMeta *cupTransitions;
		register u32 arcadeCupsAddress CTR_PSX_REGISTER("$17");

		{
			register s32 titleFont CTR_PSX_REGISTER("$7");
			register struct TransitionMeta *transitionBase CTR_PSX_REGISTER("$2");
			titleFont = FONT_BIG;
			cupIndex = 0;
			transitionBase = MM_CUP_TRANSITIONS;
			// NOTE(aalhendi): Preserve the symbol's pointer provenance while keeping
			// retail's temporary address materialization in $v0.
			CTR_PSX_OBSERVE_VALUE(transitionBase);
			cupTransitions = transitionBase;
			DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_SELECT_CUP_RACE],
			                   (u16)cupTransitions[MM_CUP_SELECT_TITLE_META_INDEX].currX + MM_CUP_SELECT_TITLE_X_OFFSET,
			                   (u16)cupTransitions[MM_CUP_SELECT_TITLE_META_INDEX].currY + MM_CUP_SELECT_TITLE_Y_OFFSET, titleFont, MM_CUP_SELECT_TEXT_COLOR);
		}
		{
			register u32 arcadeCupsPage CTR_PSX_REGISTER("$2");
			CTR_PSX_LOAD_SYMBOL_PAGE(arcadeCupsPage, MM_ARCADE_CUPS_ASM_NAME);
			CTR_PSX_ADD_SYMBOL_LOW(arcadeCupsAddress, arcadeCupsPage, MM_ARCADE_CUPS_ASM_NAME, (u32)MM_ARCADE_CUPS);
		}

		// Draw the four cup names before the cup contents.
		{
			register s16 nextCupIndex CTR_PSX_REGISTER("$2");
			do
			{
				register s32 transitionAddressOrNameX CTR_PSX_REGISTER("$5");
				register s32 nameY CTR_PSX_REGISTER("$6");
				register s32 cupOrNameYWork CTR_PSX_REGISTER("$4");
				register u32 languageStringAddress CTR_PSX_REGISTER("$3");
				register s32 signedCupIndex CTR_PSX_REGISTER("$7");

				{
					register s32 coordinateOffset CTR_PSX_REGISTER("$2");
					cupOrNameYWork = (u32)(u16)cupIndex << 16;
					signedCupIndex = cupOrNameYWork >> 16;
					CTR_PSX_KEEP_VALUE(signedCupIndex);
					transitionAddressOrNameX = signedCupIndex * 10 + (u32)cupTransitions;
					coordinateOffset = (cupIndex & 1) * MM_CUP_SELECT_COLUMN_WIDTH;
					CTR_PSX_OBSERVE_VALUE(coordinateOffset);
					cupOrNameYWork = signedCupIndex + ((u32)cupOrNameYWork >> 31);
					cupOrNameYWork >>= 1;
					CTR_PSX_LOAD_SYMBOL_PAGE(languageStringAddress, RETAIL_LANGUAGE_STRINGS_ASM_NAME);
					nameY = (u16)((struct TransitionMeta *)transitionAddressOrNameX)->currX;
					CTR_PSX_LOAD_WORD_FROM_PAGE_AFTER(languageStringAddress, languageStringAddress, RETAIL_LANGUAGE_STRINGS_ASM_NAME,
					                                  (u32)GAME_LANGUAGE_STRINGS, nameY);
					nameY += coordinateOffset;
					CTR_PSX_OBSERVE_VALUE(nameY);
					coordinateOffset = cupOrNameYWork << 2;
					CTR_PSX_KEEP_VALUE(coordinateOffset);
					coordinateOffset += cupOrNameYWork;
					CTR_PSX_KEEP_VALUE(coordinateOffset);
					coordinateOffset <<= 2;
					CTR_PSX_KEEP_VALUE(coordinateOffset);
					coordinateOffset += cupOrNameYWork;
					CTR_PSX_KEEP_VALUE(coordinateOffset);
					coordinateOffset <<= 2;
					CTR_PSX_KEEP_VALUE(coordinateOffset);
					nameY += MM_CUP_SELECT_NAME_X_OFFSET;
					nameY <<= 16;
					CTR_PSX_DEPEND_VALUE(transitionAddressOrNameX, coordinateOffset);
					cupOrNameYWork = (u16)((struct TransitionMeta *)transitionAddressOrNameX)->currY;
					transitionAddressOrNameX = nameY >> 16;
					cupOrNameYWork += coordinateOffset;
				}
				{
					register s32 workV0 CTR_PSX_REGISTER("$2");

					workV0 = signedCupIndex << 3;
					CTR_PSX_KEEP_VALUE(workV0);
					workV0 += signedCupIndex;
					CTR_PSX_KEEP_VALUE(workV0);
					workV0 <<= 1;
					CTR_PSX_KEEP_VALUE(workV0);
					workV0 += arcadeCupsAddress;
					CTR_PSX_KEEP_VALUE(workV0);
					cupOrNameYWork += MM_CUP_SELECT_NAME_Y_OFFSET;
					workV0 = *(s16 *)workV0;
					cupOrNameYWork <<= 16;
					workV0 <<= 2;
					languageStringAddress = workV0 + languageStringAddress;

					// Make the selected cup name flash.
					workV0 = menuState->rowSelected;
					nameY = cupOrNameYWork >> 16;
					if (signedCupIndex == workV0)
					{
						workV0 = MM_FRAME_COUNTER;
						workV0 &= MM_CUP_SELECT_FLASH_FRAME_BIT;
						if (workV0 == 0)
						{
							workV0 = (s16)(MM_CUP_SELECT_TEXT_COLOR | MM_CUP_SELECT_FLASH_COLOR_BIT);
						}
						else
						{
							workV0 = (s16)MM_CUP_SELECT_TEXT_COLOR;
						}
					}
					else
					{
						workV0 = (s16)MM_CUP_SELECT_TEXT_COLOR;
					}

					// draw the name of the cup
					MM_DRAW_LINE_WIDE_FLAGS(*(char **)languageStringAddress, transitionAddressOrNameX, nameY, FONT_CREDITS, workV0);
				}
				nextCupIndex = cupIndex + 1;
				cupIndex = nextCupIndex;
			} while (nextCupIndex < GAME_PROGRESS_CUP_COUNT);
		}
	}

	// Draw each cup's stars, tracks, selection box, and background.
	{
		register s32 signedCupIndex CTR_PSX_REGISTER("$16");
		register s32 startX CTR_PSX_REGISTER("$21");
		register s32 startY CTR_PSX_REGISTER("$19");
		register s32 signedStartX CTR_PSX_REGISTER("$20");
		register s16 starIndex CTR_PSX_REGISTER("$17");
		u32 gameUnlocksBase;
		register u32 starWinBitsBase CTR_PSX_REGISTER("$22");
		register u32 gameTrackerPage CTR_PSX_REGISTER("$16");

		cupIndex = 0;
		{
			register u32 symbolPage CTR_PSX_REGISTER("$2");
			CTR_PSX_LOAD_SYMBOL_PAGE(symbolPage, RETAIL_GAME_SAVE_ASM_NAME);
			CTR_PSX_ADD_SYMBOL_LOW(gameUnlocksBase, symbolPage, RETAIL_GAME_SAVE_ASM_NAME, (u32)MM_GAME_UNLOCKS - sizeof(u32));
			CTR_PSX_KEEP_VALUE(gameUnlocksBase);
		}
		CTR_PSX_ZERO_VALUE(starIndex);

		{
			register s16 nextCupIndex CTR_PSX_REGISTER("$2");
			do
			{
				{
					register u32 symbolPage CTR_PSX_REGISTER("$2");
					CTR_PSX_LOAD_SYMBOL_PAGE_AFTER(symbolPage, MM_CUP_STAR_WIN_BITS_ASM_NAME, cupIndex);
					CTR_PSX_ADD_SYMBOL_LOW(starWinBitsBase, symbolPage, MM_CUP_STAR_WIN_BITS_ASM_NAME, (u32)MM_CUP_STAR_WIN_BITS);
				}
				{
					register s32 cupShift CTR_PSX_REGISTER("$4");
					register u32 transitionAddress CTR_PSX_REGISTER("$5");
					register s32 xWork CTR_PSX_REGISTER("$6");
					register s32 yWork CTR_PSX_REGISTER("$3");
					register s32 coordinateOffset CTR_PSX_REGISTER("$2");
					register s32 signedCupWork CTR_PSX_REGISTER("$2");
					register u32 transitionPage CTR_PSX_REGISTER("$8");

					cupShift = (u32)(u16)cupIndex << 16;
					signedCupWork = cupShift >> 16;
					CTR_PSX_KEEP_VALUE(signedCupWork);
					signedCupIndex = signedCupWork;
					transitionAddress = signedCupIndex * 10;
					CTR_PSX_LOAD_SYMBOL_PAGE(transitionPage, MM_CUP_TRANSITIONS_ASM_NAME);
					CTR_PSX_ADD_SYMBOL_LOW_IN_PLACE(transitionPage, MM_CUP_TRANSITIONS_ASM_NAME, (u32)MM_CUP_TRANSITIONS);
					transitionAddress += transitionPage;
					coordinateOffset = (cupIndex & 1) * MM_CUP_SELECT_COLUMN_WIDTH;
					CTR_PSX_KEEP_VALUE(coordinateOffset);
					coordinateOffset += MM_CUP_SELECT_CONTENT_X_OFFSET;
					cupShift = signedCupIndex + ((u32)cupShift >> 31);
					cupShift >>= 1;
					xWork = (u16)((struct TransitionMeta *)transitionAddress)->currX;
					yWork = (u16)((struct TransitionMeta *)transitionAddress)->currY;
					xWork += coordinateOffset;
					CTR_PSX_KEEP_VALUE(xWork);
					startX = xWork;
					CTR_PSX_OBSERVE_VALUE(startX);
					coordinateOffset = cupShift << 2;
					CTR_PSX_KEEP_VALUE(coordinateOffset);
					coordinateOffset += cupShift;
					CTR_PSX_KEEP_VALUE(coordinateOffset);
					coordinateOffset <<= 2;
					CTR_PSX_KEEP_VALUE(coordinateOffset);
					coordinateOffset += cupShift;
					CTR_PSX_KEEP_VALUE(coordinateOffset);
					coordinateOffset <<= 2;
					coordinateOffset += MM_CUP_SELECT_CONTENT_Y_OFFSET;
					startY = yWork + coordinateOffset;
					CTR_PSX_KEEP_VALUE(xWork);
					signedStartX = (s16)xWork;
				}

				// loop through 3 stars to draw
				{
					register s16 nextStarIndex CTR_PSX_REGISTER("$2");
					do
					{
						register s32 starOffset CTR_PSX_REGISTER("$4");
						register s32 cupWinBitIndex CTR_PSX_REGISTER("$3");
						register s32 workV0 CTR_PSX_REGISTER("$2");

						workV0 = (u32)(u16)starIndex << 16;
						starOffset = workV0 >> 15;
						workV0 = starOffset;
						workV0 = workV0 + starWinBitsBase;
						CTR_PSX_KEEP_VALUE(workV0);
						cupWinBitIndex = *(s16 *)workV0;
						CTR_PSX_KEEP_VALUE(cupWinBitIndex);
						cupWinBitIndex += signedCupIndex;
						workV0 = cupWinBitIndex >> 5;
						workV0 <<= 2;
						workV0 += gameUnlocksBase;
						workV0 = *(u32 *)(workV0 + sizeof(u32));
						cupWinBitIndex &= 0x1f;
						workV0 = (u32)workV0 >> cupWinBitIndex;
						workV0 &= 1;
						if (workV0 != 0)
						{
							s16 starColorIndex;
							register s32 starX CTR_PSX_REGISTER("$5");
							struct GameTracker *gameTracker;
							register u32 *orderingTable CTR_PSX_REGISTER("$3");
							register struct IconGroup *iconGroup CTR_PSX_REGISTER("$4");
							register struct DB *backBuffer CTR_PSX_REGISTER("$7");
							register u32 workV0 CTR_PSX_REGISTER("$2");

							CTR_PSX_LOAD_SYMBOL_PAGE(workV0, MM_CUP_STAR_COLORS_ASM_NAME);
							CTR_PSX_ADD_SYMBOL_LOW_IN_PLACE(workV0, MM_CUP_STAR_COLORS_ASM_NAME, (u32)MM_CUP_STAR_COLORS);
							workV0 = starOffset + workV0;
							starColorIndex = *(u16 *)workV0;
							CTR_PSX_KEEP_VALUE(starColorIndex);

							starX = 0;
							if ((signedCupIndex != 0) && (signedCupIndex != 2))
							{
								starX = MM_CUP_SELECT_STAR_COLUMN_BIAS;
							}

							workV0 = MM_GAME_TRACKER_PAGE_VALUE;
							CTR_PSX_DEPEND_VALUE(starX, workV0);
							starX += MM_CUP_SELECT_STAR_X_OFFSET;
							gameTracker = CTR_PSX_PAGE_LVALUE(struct GameTracker *, workV0, MM_GAME_TRACKER_PAGE_OFFSET, GAME_TRACKER);
							CTR_PSX_DEPEND_VALUE(starX, gameTracker);
							starX = signedStartX + starX;
							CTR_PSX_OBSERVE_VALUE(starX);
							orderingTable = gameTracker->pushBuffer_UI.ptrOT;
							iconGroup = gameTracker->iconGroup[MM_CUP_SELECT_STAR_ICON_GROUP];
							CTR_PSX_OBSERVE_VALUE(iconGroup);
							backBuffer = gameTracker->backBuffer;
							CTR_PSX_OBSERVE_VALUE(backBuffer);
							CTR_PSX_LOAD_SYMBOL_PAGE(workV0, MM_COLOR_POINTERS_ASM_NAME);
							CTR_PSX_ADD_SYMBOL_LOW_IN_PLACE(workV0, MM_COLOR_POINTERS_ASM_NAME, (u32)MM_COLOR_POINTERS);
							{
								MM_DECALHUD_DRAW_POLY_GT4(
								    (ICONGROUP_GETICONS(iconGroup))[MM_CUP_SELECT_STAR_ICON_ID], starX, MM_CupSelect_GetStarY(starIndex, startY),
								    &backBuffer->primMem, orderingTable, MM_CUP_SELECT_STAR_COLOR(workV0, starColorIndex)[0],
								    MM_CUP_SELECT_STAR_COLOR(workV0, starColorIndex)[1], MM_CUP_SELECT_STAR_COLOR(workV0, starColorIndex)[2],
								    MM_CUP_SELECT_STAR_COLOR(workV0, starColorIndex)[3], 0, FP(1.0));
							}
						}
						nextStarIndex = starIndex + 1;
						starIndex = nextStarIndex;
					} while (nextStarIndex < GAME_PROGRESS_CUP_DIFFICULTY_COUNT);
				}

				// loop through all four track icons in one cup
				{
					register s16 trackIndex CTR_PSX_REGISTER("$17");
					register u32 arcadeCupsBase CTR_PSX_REGISTER("$22");
					register s32 cupTrackOffset CTR_PSX_REGISTER("$20");
					register s16 nextTrackIndex CTR_PSX_REGISTER("$2");

					trackIndex = 0;
					{
						register u32 symbolPage CTR_PSX_REGISTER("$2");
						CTR_PSX_LOAD_SYMBOL_PAGE(symbolPage, MM_ARCADE_CUPS_ASM_NAME);
						CTR_PSX_ADD_SYMBOL_LOW(arcadeCupsBase, symbolPage, MM_ARCADE_CUPS_ASM_NAME, (u32)MM_ARCADE_CUPS);
					}
					{
						register s32 signedCupWork CTR_PSX_REGISTER("$3");
						register s32 offsetWork CTR_PSX_REGISTER("$2");
						signedCupWork = (s16)cupIndex;
						offsetWork = signedCupWork << 3;
						offsetWork += signedCupWork;
						cupTrackOffset = offsetWork << 1;
					}

					do
					{
						register s32 posX CTR_PSX_REGISTER("$5");
						register s32 posY CTR_PSX_REGISTER("$6");
						register s32 signedTrackIndex CTR_PSX_REGISTER("$3");
						register u32 trackAddress CTR_PSX_REGISTER("$3");
						register s32 iconID CTR_PSX_REGISTER("$3");
						register struct GameTracker *gameTracker CTR_PSX_REGISTER("$4");
						register struct DB *backBuffer CTR_PSX_REGISTER("$7");
						register u32 *orderingTable CTR_PSX_REGISTER("$2");

						posX = startX + (trackIndex & 1) * MM_CUP_SELECT_TRACK_X_STEP;
						posX = (s16)posX;
						CTR_PSX_KEEP_VALUE(posX);
						posY = startY + (trackIndex / 2) * MM_CUP_SELECT_TRACK_Y_STEP;
						CTR_PSX_LOAD_SYMBOL_PAGE(gameTrackerPage, RETAIL_GAME_TRACKER_ASM_NAME);
						signedTrackIndex = (s16)trackIndex;
						trackAddress = signedTrackIndex << 2;
						trackAddress += cupTrackOffset;
						trackAddress += arcadeCupsBase;
						posY = (s16)posY;
						CTR_PSX_KEEP_VALUE(posY);
						iconID = *(s16 *)(trackAddress + 4);
						gameTracker = CTR_PSX_PAGE_LVALUE(struct GameTracker *, gameTrackerPage, MM_GAME_TRACKER_PAGE_OFFSET, GAME_TRACKER);
						iconID <<= 2;
						backBuffer = gameTracker->backBuffer;
						orderingTable = gameTracker->pushBuffer_UI.ptrOT;
						gameTracker = (struct GameTracker *)((u32)gameTracker + iconID);

						// Draw Icon of each track
						MM_CUP_SELECT_DRAW_TRACK(gameTracker->ptrIcons[0], posX, posY, &backBuffer->primMem, orderingTable, MM_CUP_COLOR);

						nextTrackIndex = trackIndex + 1;
						trackIndex = nextTrackIndex;
					} while (nextTrackIndex < MM_CUP_TRACK_COUNT);
				}

				if (cupIndex == menuState->rowSelected)
				{
					// highlight box
					cupBox.x = startX + MM_CUP_SELECT_HIGHLIGHT_X_OFFSET;
					cupBox.y = startY + MM_CUP_SELECT_HIGHLIGHT_Y_OFFSET;
					cupBox.w = MM_CUP_SELECT_HIGHLIGHT_WIDTH;
					cupBox.h = MM_CUP_SELECT_HIGHLIGHT_HEIGHT;

					MM_DRAW_CLEAR_BOX(
					    &cupBox, &GAME_MENU_HIGHLIGHT, TRANS_50_DECAL,
					    CTR_PSX_PAGE_LVALUE(struct GameTracker *, gameTrackerPage, MM_GAME_TRACKER_PAGE_OFFSET, GAME_TRACKER)->backBuffer->otMem.uiOT,
					    &CTR_PSX_PAGE_LVALUE(struct GameTracker *, gameTrackerPage, MM_GAME_TRACKER_PAGE_OFFSET, GAME_TRACKER)->backBuffer->primMem);
				}

				// background box
				{
					register struct GameTracker *backgroundGameTracker CTR_PSX_REGISTER("$3");
					backgroundGameTracker = CTR_PSX_PAGE_LVALUE(struct GameTracker *, gameTrackerPage, MM_GAME_TRACKER_PAGE_OFFSET, GAME_TRACKER);
					cupBox.x = startX + MM_CUP_SELECT_BACKGROUND_X_OFFSET;
					cupBox.y = startY + MM_CUP_SELECT_BACKGROUND_Y_OFFSET;
					cupBox.w = MM_CUP_SELECT_BACKGROUND_WIDTH;
					cupBox.h = MM_CUP_SELECT_BACKGROUND_HEIGHT;

					RECTMENU_DrawInnerRect(&cupBox, 0, backgroundGameTracker->backBuffer->otMem.uiOT);
				}
				CTR_PSX_OBSERVE_VALUE(startX);
				CTR_PSX_OBSERVE_VALUE(startY);

				nextCupIndex = cupIndex + 1;
				cupIndex = nextCupIndex;
				starIndex = 0;
			} while (nextCupIndex < GAME_PROGRESS_CUP_COUNT);
		}
	}
}
