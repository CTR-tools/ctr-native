#include <common.h>
// NOTE(aalhendi): Each mask excludes only dead caller registers at its binding site.
#define SUBMIT_NAME_SAVE_SCRATCHES     "$2", "$3", "$4", "$8"
#define SUBMIT_NAME_Y_SCRATCHES        "$2", "$3", "$7"
#define SUBMIT_NAME_ACTION_SCRATCHES   "$2", "$3", "$4", "$5", "$6", "$7"
#define SUBMIT_NAME_KEYCOUNT_SCRATCHES "$2", "$3", "$4", "$5", "$6", "$7", "$8"
#define SUBMIT_NAME_WIDTH_SCRATCHES    "$2", "$3", "$6", "$7", "$8"

#ifndef SUBMIT_NAME_TYPE_TIMER
#define SUBMIT_NAME_TYPE_TIMER     sdata->typeTimer
#define SUBMIT_NAME_MODE           sdata->selectProfileState.submitNameMode
#define SUBMIT_NAME_UNICODE        data.unicodeAscii
#define SUBMIT_NAME_SOUND_INDEX    data.soundIndexArray
#define SUBMIT_NAME_TAPS           sdata->buttonTapPerPlayer
#define SUBMIT_NAME_EDGE_COLOR     sdata->battleSetup_Color_UI_1
#define SUBMIT_NAME_UNDERSCORE     sdata->str_underscore
#define SUBMIT_NAME_DESIRED_MENU   sdata->ptrDesiredMenu
#define SUBMIT_NAME_MENU_GHOST     data.menuGhostSelection
#define SUBMIT_NAME_MENU_ADVENTURE data.menuFourAdvProfiles
#endif

#if defined(CTR_NATIVE)
// NOTE(aalhendi): Native keeps the tracker pointer and blink sampled at frame entry.
#define SUBMIT_NAME_FRAME_TRACKER(cachedTracker)   (cachedTracker)
#define SUBMIT_NAME_FRAME_BLINK(cachedBlink)       (cachedBlink)
#define SUBMIT_NAME_CURSOR_VIEW(value)             (value)
#define SUBMIT_NAME_ACTION_CURSOR(value, rendered) (value)
#else
#define SUBMIT_NAME_FRAME_TRACKER(cachedTracker)   GAME_TRACKER
#define SUBMIT_NAME_FRAME_BLINK(cachedBlink)       ((SUBMIT_NAME_TYPE_TIMER & 1) << 2)
#define SUBMIT_NAME_CURSOR_VIEW(value)             ((s16)(value))
#define SUBMIT_NAME_ACTION_CURSOR(value, rendered) (rendered)
#endif

enum
{
	SUBMIT_NAME_MODE_ADVENTURE = 0,
	SUBMIT_NAME_MODE_TIME_TRIAL = 1,

	SUBMIT_NAME_BUFFER_SIZE = 0x11,
	SUBMIT_NAME_MAX_VISIBLE_CHARS = 8,
	SUBMIT_NAME_KEYBOARD_COLS = 13,
	SUBMIT_NAME_KEYBOARD_ROWS = 3,
	SUBMIT_NAME_KEY_SPACING_X = 22,
	SUBMIT_NAME_KEY_SPACING_Y = 18,
	SUBMIT_NAME_KEYBOARD_WIDTH = 280,
	SUBMIT_NAME_KEYBOARD_START_Y = 88,

	SUBMIT_NAME_CURSOR_BACKSPACE = 38,
	SUBMIT_NAME_CURSOR_CANCEL = 1000,
	SUBMIT_NAME_CURSOR_SAVE = 1001,
	SUBMIT_NAME_CURSOR_MIDDLE_THRESHOLD = 500,
	SUBMIT_NAME_ASCII_BYTE_MARKER = 0x1000,

	SUBMIT_NAME_TITLE_X = 256,
	SUBMIT_NAME_TITLE_Y = 44,
	SUBMIT_NAME_TYPED_NAME_X = 192,
	SUBMIT_NAME_TYPED_NAME_Y = 68,
	SUBMIT_NAME_CANCEL_X = 40,
	SUBMIT_NAME_ACTION_X = 472,
	SUBMIT_NAME_ACTION_Y = 150,
	SUBMIT_NAME_PANEL_X = 32,
	SUBMIT_NAME_PANEL_W = 448,
	SUBMIT_NAME_NAME_UNDERLINE_Y = 62,
	SUBMIT_NAME_PANEL_Y = 39,
	SUBMIT_NAME_PANEL_H = 130,
};

// NOTE(aalhendi): Native indexes with the full u16 value; retail projects it to signed 16 bits.
// Returning the entry address keeps the character-pointer load at the DrawLine call.
static __inline__ char **SubmitName_SaveEntry(char **strings, u32 index)
{
#if defined(CTR_NATIVE)
	return &strings[index];
#else
	s32 byteOffset = (s16)index * (s32)sizeof(strings[0]);
	register char **entry CTR_PSX_REGISTER("$2");
	CTR_PSX_DEPEND_VALUE(byteOffset, index);
	CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(entry, strings, byteOffset);
	return entry;
#endif
}

void SubmitName_RestoreName(s16 submitNameMode)
{
	struct GameTracker *nameTracker = GAME_TRACKER;
	s16 cursor;

	// Time Trial or Adventure
	SUBMIT_NAME_MODE = submitNameMode;

	// copy the last string you typed the last time you were in
	// the OSK menu, back into the menu, avoid typing a second time
	memmove(nameTracker->currNameEntered, nameTracker->prevNameEntered, SUBMIT_NAME_BUFFER_SIZE);

	{
#if defined(CTR_NATIVE)
		// NOTE(aalhendi): Native cursor updates retain the tracker snapshot used for the copy.
		struct GameTracker *gGT = nameTracker;
#else
		struct GameTracker *gGT = GAME_TRACKER;
#endif
		// "A" or "SAVE"
		cursor = 0;
		if (gGT->currNameEntered[0] != 0)
		{
			cursor = SUBMIT_NAME_CURSOR_SAVE;
		}

		gGT->typeCursorPosition = cursor;
	}
}

// CTR_NATIVE adds host keyboard shortcuts before the retail input path.

#ifdef CTR_NATIVE
int kbCurr = 0;
int kbPrev = 0;
void SubmitName_UseKeyboard(int key)
{
	kbCurr = key;
}
#endif

s16 SubmitName_DrawMenu(u16 string)
{
	char keyboardString[3];
	RECT r;
	u16 stringCopy = string;
	s16 nameLength;
	s16 selectionResult;
#if defined(CTR_NATIVE)
	struct GameTracker *gGT = GAME_TRACKER;
#endif
	register u32 soundID CTR_PSX_REGISTER("$22");
	register u32 sound CTR_PSX_REGISTER("$2");
	s32 currNameLength;
	register char *currNameEntered CTR_PSX_REGISTER("$4");
	register s32 cursorPosition CTR_PSX_REGISTER("$19");
	u16 blinkWhite;
	u32 strColorBlink;
	register s32 renderCursor CTR_PSX_REGISTER("$17");
	register s32 saveCursor CTR_PSX_REGISTER("$20");
	register s32 cancelCursor CTR_PSX_REGISTER("$18");

#if defined(CTR_NATIVE)
	Color color;
#endif
	int tap;
	register s32 rowGuard CTR_PSX_REGISTER("$8");
	register b32 withinCancel CTR_PSX_REGISTER("$2");

	CTR_PSX_RELOAD(stringCopy);
	selectionResult = 0;
	nameLength = 0;
	{
		register s32 initialNameLength CTR_PSX_REGISTER("$2");
		register struct GameTracker *initialTracker CTR_PSX_REGISTER("$4") = SUBMIT_NAME_FRAME_TRACKER(gGT);
		CTR_PSX_ZERO_VALUE_AFTER(soundID, initialTracker, initialTracker);
		initialNameLength = strlen(initialTracker->currNameEntered);
		{
			register struct GameTracker *scanTracker CTR_PSX_REGISTER("$3") = SUBMIT_NAME_FRAME_TRACKER(gGT);
			CTR_PSX_OBSERVE_VALUE(scanTracker);
			CTR_PSX_COPY_VALUE(currNameLength, initialNameLength);
			CTR_PSX_DEPEND_VALUE(scanTracker, currNameLength);
			currNameEntered = scanTracker->currNameEntered;
		}
	}

	if (currNameEntered[0] != 0)
	{
		CTR_PSX_OBSERVE_VALUE(currNameEntered);
		do
		{
			if ((u8)currNameEntered[0] > 2)
			{
				register u32 visibleCount CTR_PSX_REGISTER("$8") = (u16)nameLength;
				CTR_PSX_KEEP_VALUE_RELAXED(visibleCount);
				nameLength = visibleCount + 1;
			}

			currNameEntered++;
		} while (currNameEntered[0] != 0);
	}

	{
		register s32 loadedCursor CTR_PSX_REGISTER("$3") = SUBMIT_NAME_FRAME_TRACKER(gGT)->typeCursorPosition;
		cursorPosition = loadedCursor;
		if (loadedCursor > SUBMIT_NAME_CURSOR_BACKSPACE)
		{
			CTR_PSX_KEEP_VALUE_RELAXED(loadedCursor);
			withinCancel = loadedCursor < SUBMIT_NAME_CURSOR_CANCEL;
			CTR_PSX_OBSERVE_VALUE(withinCancel);
			if (withinCancel)
			{
				register s32 keyCount = SUBMIT_NAME_KEYBOARD_ROWS * SUBMIT_NAME_KEYBOARD_COLS;
				CTR_PSX_BIND_VALUE_CLOBBER(keyCount, SUBMIT_NAME_KEYCOUNT_SCRATCHES);
				cursorPosition = keyCount - 1;
				CTR_PSX_OBSERVE_VALUE(keyCount);
			}
		}
	}

	{
		u32 timer = SUBMIT_NAME_TYPE_TIMER;
		CTR_PSX_KEEP_VALUE_RELAXED(timer);
		CTR_PSX_LOAD_IMMEDIATE(rowGuard, SUBMIT_NAME_KEYBOARD_ROWS);
		SUBMIT_NAME_TYPE_TIMER = timer + 1;
	}
	blinkWhite = (SUBMIT_NAME_TYPE_TIMER & 1) << 2;

	// grid of letters, 13x3
	{
		register u32 row CTR_PSX_REGISTER("$18") = 0;
		register u32 nextRow CTR_PSX_REGISTER("$2");
		if (rowGuard != 0)
		{
			register s32 columns CTR_PSX_REGISTER("$20") = SUBMIT_NAME_KEYBOARD_COLS;
			b32 hasColumns;
			register s32 highlightedCursor CTR_PSX_REGISTER("$23");
			CTR_PSX_KEEP_VALUE_RELAXED(columns);
			{
				register s32 zero CTR_PSX_REGISTER("$0");
				register b32 columnCondition;
				CTR_PSX_CAPTURE_REGISTER(zero, 0);
				columnCondition = columns > zero;

				hasColumns = columnCondition;
			}
			{
				register u32 shiftedCursor CTR_PSX_REGISTER("$2") = (u32)cursorPosition << 16;
				CTR_PSX_KEEP_VALUE_RELAXED(shiftedCursor);
				CTR_PSX_SHIFT_RIGHT_ARITHMETIC(highlightedCursor, shiftedCursor, 16);
			}
			do
			{
				{
					register b32 rowHasColumns CTR_PSX_REGISTER("$8") = hasColumns;
					register u32 column CTR_PSX_REGISTER("$16");
					CTR_PSX_OBSERVE_VALUE(rowHasColumns);
					column = 0;
					if (rowHasColumns)
					{
						register s32 signedRow CTR_PSX_REGISTER("$17");
						register s32 characterRow CTR_PSX_REGISTER("$21");
						register u32 nextColumn CTR_PSX_REGISTER("$2");
						{
							register u32 shiftedRow CTR_PSX_REGISTER("$2") = row << 16;
							CTR_PSX_KEEP_VALUE_RELAXED(shiftedRow);
							CTR_PSX_SHIFT_RIGHT_ARITHMETIC(signedRow, shiftedRow, 16);
						}
						characterRow = signedRow * columns;
						CTR_PSX_OBSERVE_VALUE(characterRow);
						do
						{
							register u32 keyboardCharacter CTR_PSX_REGISTER("$4");
							register s16 keyBlink CTR_PSX_REGISTER("$7");
							{
								register s32 keyColumns CTR_PSX_REGISTER("$8") = SUBMIT_NAME_KEYBOARD_COLS;
								register s32 keyRow;
								register s32 selectedIndex CTR_PSX_REGISTER("$2");
								// NOTE(aalhendi): The retail inner loop re-enters this integer multiply pipeline delay.
								CTR_PSX_GTE_READ_DELAY();
								CTR_PSX_KEEP_VALUE_RELAXED(keyColumns);
								keyRow = (s32)row * keyColumns;
								selectedIndex = (s16)(column + keyRow);
								CTR_PSX_OBSERVE_VALUE(selectedIndex);
								keyBlink = 0;
								if (highlightedCursor == selectedIndex)
								{
#if defined(CTR_NATIVE)
									keyBlink = blinkWhite;
#else
									register u32 timerBits CTR_PSX_REGISTER("$2") = SUBMIT_NAME_TYPE_TIMER;
									timerBits &= 1;
									CTR_PSX_KEEP_VALUE_RELAXED(timerBits);
									keyBlink = timerBits << 2;
#endif
								}
							}
							CTR_PSX_OBSERVE_VALUE(keyBlink);
							{
								register u16 *unicodeTable CTR_PSX_REGISTER("$8");
								register s32 loadedCharacter CTR_PSX_REGISTER("$3");
								register u32 marker CTR_PSX_REGISTER("$2");
								register s32 characterOffset CTR_PSX_REGISTER("$2") = (characterRow + (s16)column) * (s32)sizeof(SUBMIT_NAME_UNICODE[0]);
								register u16 *entry CTR_PSX_REGISTER("$2");
								CTR_PSX_LOAD_SYMBOL_PAGE_AFTER(unicodeTable, "data+21492", characterOffset);
								CTR_PSX_ADD_SYMBOL_LOW_IN_PLACE(unicodeTable, "data+21492", SUBMIT_NAME_UNICODE);
								CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(entry, unicodeTable, characterOffset);
								CTR_PSX_LOAD_SIGNED_HALF(loadedCharacter, entry, 0, *entry);
								CTR_PSX_LOAD_IMMEDIATE(marker, SUBMIT_NAME_ASCII_BYTE_MARKER);
								CTR_PSX_COPY_VALUE(keyboardCharacter, loadedCharacter);
								if ((loadedCharacter & 0xff00) == marker)
									keyboardCharacter &= 0xff;
							}
							if ((keyboardCharacter & 0xff00) != 0)
							{
								register u32 shiftedCharacter CTR_PSX_REGISTER("$3") = keyboardCharacter << 16;
								register s32 highByte CTR_PSX_REGISTER("$2");
								CTR_PSX_KEEP_VALUE_RELAXED(shiftedCharacter);
								highByte = (s32)shiftedCharacter >> 24;
								CTR_PSX_KEEP_VALUE_RELAXED(highByte);
								keyboardString[0] = highByte;
								keyboardString[1] = keyboardCharacter;
								keyboardString[2] = 0;
							}
							else
							{
								keyboardString[0] = keyboardCharacter;
								keyboardString[1] = 0;
							}
							// NOTE(aalhendi): Snapshot flags before reusing dead temporaries for the two coordinate trees.
							// Separate loaded/value carriers retain the single definitions scheduled by GCC 2.8.1.
							{
								s16 drawFlags = (s16)keyBlink;
								register s32 normalizedFlags CTR_PSX_REGISTER("$2") = (s16)drawFlags;
								CTR_PSX_OBSERVE_VALUE(normalizedFlags);
								DecalFont_DrawLine(keyboardString, (s16)column * SUBMIT_NAME_KEY_SPACING_X + SUBMIT_NAME_TITLE_X - ({
									                                   u32 loadedWidth;
									                                   u32 width;
									                                   u32 loadedHalf;
									                                   u32 halfWidth;
									                                   CTR_PSX_LOAD_IMMEDIATE(loadedWidth, SUBMIT_NAME_KEYBOARD_WIDTH);
									                                   width = loadedWidth;
									                                   CTR_PSX_BIND_VALUE_CLOBBER(width, SUBMIT_NAME_WIDTH_SCRATCHES);
									                                   loadedHalf = width >> 1;
									                                   halfWidth = loadedHalf;
									                                   CTR_PSX_DEPEND_VALUE(halfWidth, width);
									                                   halfWidth;
								                                   }),
								                   ({
									                   s32 yOffset = signedRow * SUBMIT_NAME_KEY_SPACING_Y;
									                   s32 loadedY;
									                   s32 initialY;
									                   s32 drawY;
									                   CTR_PSX_LOAD_IMMEDIATE(loadedY, SUBMIT_NAME_KEYBOARD_START_Y);
									                   initialY = loadedY;
									                   CTR_PSX_BIND_VALUE_CLOBBER(initialY, SUBMIT_NAME_Y_SCRATCHES);
									                   CTR_PSX_ADD_U32(drawY, initialY, yOffset);
									                   drawY;
								                   }),
								                   FONT_BIG, drawFlags);
							}
							nextColumn = column + 1;
							CTR_PSX_KEEP_VALUE_RELAXED(nextColumn);
							CTR_PSX_COPY_VALUE(column, nextColumn);
						} while ((s16)nextColumn < columns);
					}
				}
				nextRow = row + 1;
				CTR_PSX_COPY_VALUE(row, nextRow);
			} while ((s16)nextRow < SUBMIT_NAME_KEYBOARD_ROWS);
			CTR_PSX_OBSERVE_VALUE(row);
		}
	}

	DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_PLEASE_ENTER_YOUR_NAME], SUBMIT_NAME_TITLE_X, SUBMIT_NAME_TITLE_Y, FONT_BIG, (JUSTIFY_CENTER | ORANGE));

	// player name
	DecalFont_DrawLine(SUBMIT_NAME_FRAME_TRACKER(gGT)->currNameEntered, SUBMIT_NAME_TYPED_NAME_X, SUBMIT_NAME_TYPED_NAME_Y, FONT_BIG, WHITE);

	if (((SUBMIT_NAME_TYPE_TIMER & 2) != 0) && ((s16)currNameLength < SUBMIT_NAME_BUFFER_SIZE - 1))
	{
		int currNameWidth = DecalFont_GetLineWidth(SUBMIT_NAME_FRAME_TRACKER(gGT)->currNameEntered, FONT_BIG);

		DecalFont_DrawLine(SUBMIT_NAME_UNDERSCORE, currNameWidth + SUBMIT_NAME_TYPED_NAME_X, SUBMIT_NAME_TYPED_NAME_Y, FONT_BIG, ORANGE);
	}

	// SAVE button blink
	{
		register u32 shiftedCursor CTR_PSX_REGISTER("$2") = (u32)cursorPosition << 16;
		CTR_PSX_KEEP_VALUE_RELAXED(shiftedCursor);
		CTR_PSX_SHIFT_RIGHT_ARITHMETIC(renderCursor, shiftedCursor, 16);
	}
	saveCursor = SUBMIT_NAME_CURSOR_SAVE;
	CTR_PSX_KEEP_VALUE_RELAXED(saveCursor);
	strColorBlink = 0;
	if (renderCursor == saveCursor)
	{
		strColorBlink = SUBMIT_NAME_FRAME_BLINK(blinkWhite);
	}

	// SAVE button draw
	{
		register s32 renderBlink CTR_PSX_REGISTER("$7") = strColorBlink;
		CTR_PSX_OBSERVE_VALUE(renderBlink);
		{
			register s32 drawX CTR_PSX_REGISTER("$5") = SUBMIT_NAME_ACTION_X;
			register s32 drawY CTR_PSX_REGISTER("$6") = SUBMIT_NAME_ACTION_Y;
			register u32 languageIndex = stringCopy;
			register char **strings CTR_PSX_REGISTER("$3");
			register s32 saveFlags CTR_PSX_REGISTER("$2");
			// NOTE(aalhendi): X, Y and blink remain live; strings and flags are initialized after this binding.
			CTR_PSX_BIND_VALUE_CLOBBER(languageIndex, SUBMIT_NAME_SAVE_SCRATCHES);
			strings = GAME_LANGUAGE_STRINGS;
			CTR_PSX_OBSERVE_VALUE(drawX);
			CTR_PSX_OBSERVE_VALUE(drawY);
			CTR_PSX_OBSERVE_VALUE(strings);
			saveFlags = JUSTIFY_RIGHT | strColorBlink;
			CTR_PSX_OBSERVE_VALUE(saveFlags);
			DecalFont_DrawLine(*SubmitName_SaveEntry(strings, languageIndex), drawX, drawY, FONT_BIG, (JUSTIFY_RIGHT | strColorBlink));
		}
	}

	// CANCEL button blink
	cancelCursor = SUBMIT_NAME_CURSOR_CANCEL;
	CTR_PSX_KEEP_VALUE_RELAXED(cancelCursor);
	strColorBlink = 0;
	if (renderCursor == cancelCursor)
	{
		strColorBlink = SUBMIT_NAME_FRAME_BLINK(blinkWhite);
	}

	{
		register s32 renderBlink CTR_PSX_REGISTER("$7") = strColorBlink;
		CTR_PSX_OBSERVE_VALUE(renderBlink);
		{
			register s32 drawX CTR_PSX_REGISTER("$5") = SUBMIT_NAME_CANCEL_X;
			register s32 drawY CTR_PSX_REGISTER("$6") = SUBMIT_NAME_ACTION_Y;
			register u32 shiftedFlags CTR_PSX_REGISTER("$2") = strColorBlink << 16;
			register char **strings CTR_PSX_REGISTER("$3");
			register s32 cancelFlags CTR_PSX_REGISTER("$2");
			CTR_PSX_OBSERVE_VALUE(drawX);
			CTR_PSX_OBSERVE_VALUE(drawY);
			// NOTE(aalhendi): Retail keeps the flag carrier across the language pointer reload.
			CTR_PSX_KEEP_VALUE_RELAXED(shiftedFlags);
			strings = GAME_LANGUAGE_STRINGS;
			CTR_PSX_OBSERVE_VALUE(strings);
			cancelFlags = (s32)shiftedFlags >> 16;
			CTR_PSX_OBSERVE_VALUE(cancelFlags);
			DecalFont_DrawLine(strings[LNG_CANCEL], drawX, drawY, FONT_BIG, cancelFlags);
		}
	}

	// leftX = 32 (256-224)
	// rightX = 480 (256+224)
	// r.w = 32
	// r.w = 448

	// in 16x9,
	// subtract 1/8 from r.x
	// subtract 1/4 from r.w
	r.x = SUBMIT_NAME_PANEL_X;
	r.w = SUBMIT_NAME_PANEL_W;

	r.y = SUBMIT_NAME_NAME_UNDERLINE_Y;
	r.h = 2;
#if defined(CTR_NATIVE)
	// NOTE(aalhendi): Native passes a real Color sampled from the packed field.
	ColorCode_SetPacked(&color, SUBMIT_NAME_EDGE_COLOR);
	RECTMENU_DrawOuterRect_Edge(&r, &color, 0x20, SUBMIT_NAME_FRAME_TRACKER(gGT)->backBuffer->otMem.uiOT);
#else
	RECTMENU_DrawOuterRect_Edge(&r, (Color *)&SUBMIT_NAME_EDGE_COLOR, 0x20, SUBMIT_NAME_FRAME_TRACKER(gGT)->backBuffer->otMem.uiOT);
#endif

	r.y = SUBMIT_NAME_PANEL_Y;
	r.h = SUBMIT_NAME_PANEL_H;
	RECTMENU_DrawInnerRect(&r, 0, SUBMIT_NAME_FRAME_TRACKER(gGT)->backBuffer->otMem.uiOT);

	tap = SUBMIT_NAME_TAPS[0];

#ifdef CTR_NATIVE
	// NOTE(aalhendi): native keyboard shortcut; retail input remains gamepad-driven.

	if (kbCurr != kbPrev)
	{
		// Between A and Z
		if ((kbCurr >= 4) && (kbCurr <= 29))
		{
			// SDL_KEY_A = 4
			kbCurr -= 4;

			// cursor position of 'A'
			kbCurr += 0;

			cursorPosition = kbCurr;
			soundID = 1;
			tap = BTN_CIRCLE;
		}

		// '0' key
		if (kbCurr == 39)
		{
			cursorPosition = 2 * SUBMIT_NAME_KEYBOARD_COLS;
			tap = BTN_CIRCLE;
		}

		// Between 1 and 9
		if ((kbCurr >= 30) && (kbCurr <= 38))
		{
			// SDL_SCANCODE_1 = 30
			kbCurr -= 30;

			// cursor position of '1'
			kbCurr += 2 * SUBMIT_NAME_KEYBOARD_COLS + 1;

			cursorPosition = kbCurr;
			tap = BTN_CIRCLE;
		}

		// Escape
		if (kbCurr == 41)
		{
			cursorPosition = SUBMIT_NAME_CURSOR_CANCEL;
			tap = BTN_CIRCLE;
		}

		// Backspace
		if (kbCurr == 42)
		{
			cursorPosition = kbCurr - 4;
			tap = BTN_CIRCLE;
		}

		// Space
		if (kbCurr == 44)
		{
			cursorPosition = 37;
			tap = BTN_CIRCLE;
		}
	}

	kbPrev = kbCurr;

	// Press Enter
	if (NikoGetEnterKey())
	{
		cursorPosition = SUBMIT_NAME_CURSOR_SAVE;
		tap = BTN_CIRCLE;
	}

#endif

	if (tap & (BTN_UP | BTN_DOWN | BTN_LEFT | BTN_RIGHT))
	{
		u32 cursorMiddleDelta;

		if (tap & BTN_UP)
		{
			cursorPosition -= SUBMIT_NAME_KEYBOARD_COLS;
		}
		if (tap & BTN_DOWN)
		{
			cursorPosition += SUBMIT_NAME_KEYBOARD_COLS;
		}
		if (tap & BTN_LEFT)
		{
			cursorPosition--;
		}
		if (tap & BTN_RIGHT)
		{
			cursorPosition++;
		}

		if (SUBMIT_NAME_CURSOR_VIEW(cursorPosition) < 0)
		{
			cursorPosition = SUBMIT_NAME_CURSOR_SAVE;
		}

		{
			s32 cursorCheck = SUBMIT_NAME_CURSOR_VIEW(cursorPosition);
			if (cursorCheck > SUBMIT_NAME_CURSOR_BACKSPACE)
			{
				CTR_PSX_KEEP_VALUE_RELAXED(cursorCheck);
				if (cursorCheck < SUBMIT_NAME_CURSOR_MIDDLE_THRESHOLD)
				{
					cursorPosition = SUBMIT_NAME_CURSOR_SAVE;
				}
			}
		}
		cursorMiddleDelta = cursorPosition - SUBMIT_NAME_CURSOR_MIDDLE_THRESHOLD;
		if ((cursorMiddleDelta & 0xffff) < SUBMIT_NAME_CURSOR_MIDDLE_THRESHOLD)
		{
			register s32 keyCount CTR_PSX_REGISTER("$8") = SUBMIT_NAME_KEYBOARD_ROWS * SUBMIT_NAME_KEYBOARD_COLS;
			CTR_PSX_KEEP_VALUE_RELAXED(keyCount);
			cursorPosition = keyCount - 1;
		}
		soundID = 1;

		// == Naughty Dog Bug ==
		// This used to be 1002, which would allow you to
		// press Right on SAVE, and cursor would go off-screen
#if defined(CTR_NATIVE)
		// NOTE(aalhendi): Native keeps the corrected wrap when moving right from SAVE.
		if (cursorPosition > SUBMIT_NAME_CURSOR_SAVE)
#else
		if (SUBMIT_NAME_CURSOR_VIEW(cursorPosition) > SUBMIT_NAME_CURSOR_SAVE + 1)
#endif
		{
			cursorPosition = 0;
		}
	}
	else if (tap & BTN_START)
	{
		soundID = 3;
		if (SUBMIT_NAME_ACTION_CURSOR(cursorPosition, renderCursor) != cancelCursor)
		{
			cursorPosition = SUBMIT_NAME_CURSOR_CANCEL;
			soundID = 1;
		}
		else
		{
			selectionResult = -1;
		}
	}
	else if (tap & (BTN_TRIANGLE | BTN_SQUARE_one))
	{
#if defined(CTR_NATIVE)
		// NOTE(aalhendi): Native empty backspace clears any keyboard shortcut cue.
		soundID = 0;
#endif
		if ((s16)currNameLength != 0)
		{
			soundID = 4;
			SUBMIT_NAME_FRAME_TRACKER(gGT)->currNameEntered[(s16)currNameLength - 1] = 0;
			if ((u8)SUBMIT_NAME_FRAME_TRACKER(gGT)->currNameEntered[(s16)currNameLength - 2] < 3)
			{
				SUBMIT_NAME_FRAME_TRACKER(gGT)->currNameEntered[(s16)currNameLength - 2] = 0;
			}
		}
		else
		{
			sound = soundID;
			goto PlaySound;
		}
	}
	else if (tap & (BTN_CIRCLE | BTN_CROSS_one))
	{
		register s32 rows CTR_PSX_REGISTER("$8") = SUBMIT_NAME_KEYBOARD_ROWS;
		register s32 columns = SUBMIT_NAME_KEYBOARD_COLS;
		register s32 keyCount CTR_PSX_REGISTER("$8");
		register s32 lastKey CTR_PSX_REGISTER("$2");
		// NOTE(aalhendi): Both action tests consume the same live keyboard bound.
		CTR_PSX_KEEP_VALUE_RELAXED(rows);
		CTR_PSX_BIND_VALUE_CLOBBER(columns, SUBMIT_NAME_ACTION_SCRATCHES);
		keyCount = rows * columns;
		lastKey = keyCount - 1;
		if (SUBMIT_NAME_ACTION_CURSOR(cursorPosition, renderCursor) == lastKey)
		{
			soundID = 2;
			if ((s16)currNameLength != 0)
			{
				SUBMIT_NAME_FRAME_TRACKER(gGT)->currNameEntered[(s16)currNameLength - 1] = 0;
			}
			else
			{
				CTR_PSX_KEEP_VALUE_RELAXED(soundID);
				sound = soundID;
				goto PlaySound;
			}
		}
		else if (SUBMIT_NAME_ACTION_CURSOR(cursorPosition, renderCursor) < lastKey)
		{
			register u32 normalizedCharacter CTR_PSX_REGISTER("$4");
			{
				register s32 marker CTR_PSX_REGISTER("$3");
				register u32 rawCharacter CTR_PSX_REGISTER("$2");
				register s32 asciiMarker CTR_PSX_REGISTER("$4");
				register s32 unicodeByteOffset CTR_PSX_REGISTER("$2") =
				    SUBMIT_NAME_ACTION_CURSOR(cursorPosition, renderCursor) * (s32)sizeof(SUBMIT_NAME_UNICODE[0]);
				register u16 *table;
				register u16 *entry CTR_PSX_REGISTER("$2");
				CTR_PSX_KEEP_VALUE_RELAXED(unicodeByteOffset);
				table = SUBMIT_NAME_UNICODE;
				CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(entry, table, unicodeByteOffset);
				CTR_PSX_LOAD_IMMEDIATE(asciiMarker, SUBMIT_NAME_ASCII_BYTE_MARKER);
				// NOTE(aalhendi): The marker and character consume signed and unsigned views of the canonical entry.
				CTR_PSX_LOAD_SIGNED_HALF(marker, entry, 0, *entry);
				CTR_PSX_LOAD_UNSIGNED_HALF(rawCharacter, entry, 0, *entry);
				if ((marker & 0xff00) != asciiMarker)
				{
					normalizedCharacter = rawCharacter;
				}
				else
				{
					normalizedCharacter = rawCharacter & 0xff;
				}
			}
			{
				register u32 cursorCharacter CTR_PSX_REGISTER("$4") = normalizedCharacter;
				register u32 appendVisible CTR_PSX_REGISTER("$8") = (u16)nameLength;
				CTR_PSX_KEEP_VALUE_RELAXED(appendVisible);
				{
					register s32 signedVisible CTR_PSX_REGISTER("$2") = (s16)appendVisible;

					if (signedVisible < SUBMIT_NAME_MAX_VISIBLE_CHARS)
					{
						if (cursorCharacter & 0xff00)
						{
							register s32 highByte CTR_PSX_REGISTER("$5") = (s32)(cursorCharacter << 16);
							s32 previousNameLength;
							CTR_PSX_KEEP_VALUE_RELAXED(highByte);
							previousNameLength = (s16)currNameLength;
							currNameLength++;
							cursorCharacter &= 0xff;
							SUBMIT_NAME_FRAME_TRACKER(gGT)->currNameEntered[previousNameLength] = highByte >> 24;
							// NOTE(aalhendi): Keep the shifted carrier live until the high byte has been stored.
							CTR_PSX_OBSERVE_VALUE(highByte);
						}
						soundID = 1;
						SUBMIT_NAME_FRAME_TRACKER(gGT)->currNameEntered[(s16)currNameLength] = cursorCharacter;
					}
					else
					{
						soundID = 5;
					}
				}
			}
		}
		else
		{
			if (SUBMIT_NAME_ACTION_CURSOR(cursorPosition, renderCursor) == saveCursor)
			{
				register u32 copyLength CTR_PSX_REGISTER("$6") = SUBMIT_NAME_BUFFER_SIZE;
				register struct GameTracker *nameTracker CTR_PSX_REGISTER("$5");
				CTR_PSX_KEEP_VALUE_RELAXED(copyLength);
				soundID = 2;
				// NOTE(aalhendi): Keep the tracker reload before committing the SAVE result.
				CTR_PSX_LOAD_WORD(nameTracker, SUBMIT_NAME_FRAME_TRACKER(gGT));
				selectionResult = 1;
				memmove(nameTracker->prevNameEntered, nameTracker->currNameEntered, copyLength);
			}
			else
			{
#if defined(CTR_NATIVE)
				// NOTE(aalhendi): Native unhandled confirmation clears any keyboard shortcut cue.
				soundID = 0;
#endif
				if (SUBMIT_NAME_ACTION_CURSOR(cursorPosition, renderCursor) != cancelCursor)
				{
					sound = soundID;
					goto PlaySound;
				}
				{
					register s32 cancelResult CTR_PSX_REGISTER("$8");
					soundID = 3;
					cancelResult = -1;
					CTR_PSX_KEEP_VALUE_RELAXED(cancelResult);
					selectionResult = cancelResult;
				}
			}
			RECTMENU_ClearInput();
		}
	}


	{
		CTR_PSX_COPY_VALUE(sound, soundID);
	PlaySound:
		// NOTE(aalhendi): Keep the cue and its audio-check copy live across empty actions.
		CTR_PSX_OBSERVE_VALUE(soundID);
		CTR_PSX_OBSERVE_VALUE(sound);
		if (sound != 0)
		{
			s32 soundIndex;
#ifdef CTR_NATIVE
			// NOTE(aalhendi): Native audio consumes the complete canonical sound table entry.
			soundIndex = SUBMIT_NAME_SOUND_INDEX[soundID];
#else
			register s32 *soundTable CTR_PSX_REGISTER("$3") = SUBMIT_NAME_SOUND_INDEX;

			{
				register s32 *soundEntry CTR_PSX_REGISTER("$2") = &soundTable[sound];
				CTR_PSX_LOAD_UNSIGNED_HALF(soundIndex, soundEntry, 0, *soundEntry);
			}
#endif
			OtherFX_Play(soundIndex, 1);
		}
	}
	// NOTE(aalhendi): These caller temporaries are dead; exclude them from the late halfword return reload.
	CTR_PSX_CLOBBER("$10");
	CTR_PSX_CLOBBER("$11");
	CTR_PSX_CLOBBER("$12");
	CTR_PSX_CLOBBER("$13");
	CTR_PSX_CLOBBER("$14");
	CTR_PSX_CLOBBER("$15");
	CTR_PSX_CLOBBER("$24");
	CTR_PSX_CLOBBER("$25");
	SUBMIT_NAME_FRAME_TRACKER(gGT)->typeCursorPosition = cursorPosition;
	return selectionResult;
}

void SubmitName_MenuProc(struct RectMenu *menu)
{
#if defined(CTR_NATIVE)
	// NOTE(aalhendi): Native commits the name through the tracker snapshot from before DrawMenu callbacks.
	struct GameTracker *gGT = GAME_TRACKER;
#endif
	s16 selection = SubmitName_DrawMenu(0x13f);
	s32 mode;
	menu->rowSelected = selection;
	if (selection == 0)
	{
		return;
	}
	// NOTE(aalhendi): Retail dispatch checks the upper mode bound before the Adventure zero case.
	mode = SUBMIT_NAME_MODE;
	if (mode != SUBMIT_NAME_MODE_TIME_TRIAL)
	{
		if (mode > SUBMIT_NAME_MODE_TIME_TRIAL)
		{
			return;
		}
		if (mode != SUBMIT_NAME_MODE_ADVENTURE)
		{
			return;
		}
		if (selection < 0)
		{
			SUBMIT_NAME_DESIRED_MENU = CS_Garage_GetMenuPtr();
			CS_Garage_ZoomOut(1);
		}
		else
		{
#if !defined(CTR_NATIVE)
			struct GameTracker *gGT = GAME_TRACKER;
#endif
			memcpy(GAME_ADV_PROGRESS.name, gGT->prevNameEntered, sizeof(gGT->prevNameEntered));
			SelectProfile_ToggleMode(SELECT_PROFILE_MODE_ADV_SAVE);
			SUBMIT_NAME_DESIRED_MENU = &SUBMIT_NAME_MENU_ADVENTURE;
		}
	}
	else
	{
		if (selection < 0)
		{
			extern struct RectMenu menu224;
			SUBMIT_NAME_DESIRED_MENU = &menu224;
		}
		else
		{
			SelectProfile_ToggleMode(SELECT_PROFILE_MODE_GHOST_SAVE);
			SUBMIT_NAME_DESIRED_MENU = &SUBMIT_NAME_MENU_GHOST;
		}
	}
}
