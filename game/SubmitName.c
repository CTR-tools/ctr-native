#include <common.h>

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
	SUBMIT_NAME_KEYBOARD_START_X = 116,
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

CTR_STATIC_ASSERT(SUBMIT_NAME_MODE_ADVENTURE == 0);
CTR_STATIC_ASSERT(SUBMIT_NAME_MODE_TIME_TRIAL == 1);
CTR_STATIC_ASSERT(SUBMIT_NAME_BUFFER_SIZE == 0x11);
CTR_STATIC_ASSERT(SUBMIT_NAME_MAX_VISIBLE_CHARS == 8);
CTR_STATIC_ASSERT(SUBMIT_NAME_KEYBOARD_COLS == 13);
CTR_STATIC_ASSERT(SUBMIT_NAME_KEYBOARD_ROWS == 3);
CTR_STATIC_ASSERT(SUBMIT_NAME_CURSOR_BACKSPACE == 38);
CTR_STATIC_ASSERT(SUBMIT_NAME_CURSOR_CANCEL == 1000);
CTR_STATIC_ASSERT(SUBMIT_NAME_CURSOR_SAVE == 1001);
CTR_STATIC_ASSERT(SUBMIT_NAME_CURSOR_MIDDLE_THRESHOLD == 500);
CTR_STATIC_ASSERT(SUBMIT_NAME_ASCII_BYTE_MARKER == 0x1000);

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004aa08-0x8004aa60
void SubmitName_RestoreName(s16 submitNameMode)
{
	struct GameTracker *gGT = sdata->gGT;

	// Time Trial or Adventure
	sdata->data10_bbb[0xd] = submitNameMode;

	// copy the last string you typed the last time you were in
	// the OSK menu, back into the menu, avoid typing a second time
	memmove(gGT->currNameEntered, gGT->prevNameEntered, SUBMIT_NAME_BUFFER_SIZE);

	// "A" or "SAVE"
	s16 cursor = 0;
	if (gGT->currNameEntered[0] != 0)
	{
		cursor = SUBMIT_NAME_CURSOR_SAVE;
	}

	gGT->typeCursorPosition = cursor;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004aa60-0x8004b144 for the
// retail path. CTR_NATIVE adds host keyboard shortcuts before retail input.

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
	s16 currNameLength;
	int currNameWidth;
	u32 cursorMiddleDelta;
	u32 cursorCharacter;
	int cursorPosition;
	u32 keyboardCharacter;
	char *currNameEntered;
	u16 strColorBlink;
	u16 blinkWhite;
	int j;
	int i;
	u32 soundID;
	s16 currNameLengthIncrement;
	RECT r;
	u16 stringCopy;
	s16 nameLength;
	s16 selectionResult;
	int letterID;
	int strlenCurrNameEnteredInt;
	u32 keyboardCharacterTopByte;

	struct GameTracker *gGT = sdata->gGT;

	soundID = 0;
	selectionResult = 0;
	nameLength = 0;
	stringCopy = string;
	strlenCurrNameEnteredInt = strlen(gGT->currNameEntered);
	currNameLength = strlenCurrNameEnteredInt;
	currNameEntered = gGT->currNameEntered;
	blinkWhite = ((sdata->typeTimer >> 0) & 1) << 2;

	while (currNameEntered[0] != 0)
	{
		if (currNameEntered[0] > 2)
		{
			nameLength++;
		}

		currNameEntered++;
	}

	cursorPosition = gGT->typeCursorPosition;
	if ((cursorPosition > SUBMIT_NAME_CURSOR_BACKSPACE) && (cursorPosition < SUBMIT_NAME_CURSOR_CANCEL))
	{
		cursorPosition = SUBMIT_NAME_CURSOR_BACKSPACE;
	}

	sdata->typeTimer++;
	letterID = 0;

	// grid of letters, 13x3
	for (i = 0; i < SUBMIT_NAME_KEYBOARD_ROWS; i++)
	{
		for (j = 0; j < SUBMIT_NAME_KEYBOARD_COLS; j++)
		{
			char keyboardString[3];
			keyboardCharacter = data.unicodeAscii[letterID];
			keyboardCharacterTopByte = keyboardCharacter & 0xff00;
			if (keyboardCharacterTopByte == SUBMIT_NAME_ASCII_BYTE_MARKER)
			{
				keyboardCharacter &= 0xff;
				keyboardCharacterTopByte = 0;
			}

			if (keyboardCharacterTopByte == 0)
			{
				keyboardString[0] = keyboardCharacter;
				keyboardString[1] = 0;
			}
			else
			{
				keyboardString[0] = (keyboardCharacter << 16) >> 24;
				keyboardString[1] = keyboardCharacter;
				keyboardString[2] = 0;
			}

			// LETTER button blink
			strColorBlink = 0;
			if (cursorPosition == letterID)
			{
				strColorBlink = blinkWhite;
			}

			// LETTER button draw
			DecalFont_DrawLine(keyboardString, j * SUBMIT_NAME_KEY_SPACING_X + SUBMIT_NAME_KEYBOARD_START_X,
			                   i * SUBMIT_NAME_KEY_SPACING_Y + SUBMIT_NAME_KEYBOARD_START_Y, FONT_BIG, strColorBlink);

			letterID++;
		}
	}

	DecalFont_DrawLine(sdata->lngStrings[LNG_PLEASE_ENTER_YOUR_NAME], SUBMIT_NAME_TITLE_X, SUBMIT_NAME_TITLE_Y, FONT_BIG, (JUSTIFY_CENTER | ORANGE));

	// player name
	DecalFont_DrawLine(gGT->currNameEntered, SUBMIT_NAME_TYPED_NAME_X, SUBMIT_NAME_TYPED_NAME_Y, FONT_BIG, WHITE);

	if (((sdata->typeTimer & 2) != 0) && (currNameLength < SUBMIT_NAME_BUFFER_SIZE - 1))
	{
		currNameWidth = DecalFont_GetLineWidth(gGT->currNameEntered, FONT_BIG);

		DecalFont_DrawLine(sdata->str_underscore, currNameWidth + SUBMIT_NAME_TYPED_NAME_X,

		                   SUBMIT_NAME_TYPED_NAME_Y, FONT_BIG, ORANGE);
	}

	// SAVE button blink
	strColorBlink = 0;
	if (cursorPosition == SUBMIT_NAME_CURSOR_SAVE)
	{
		strColorBlink = blinkWhite;
	}

	// SAVE button draw
	DecalFont_DrawLine(sdata->lngStrings[stringCopy], SUBMIT_NAME_ACTION_X, SUBMIT_NAME_ACTION_Y, FONT_BIG, (JUSTIFY_RIGHT | strColorBlink));

	// CANCEL button blink
	strColorBlink = 0;
	if (cursorPosition == SUBMIT_NAME_CURSOR_CANCEL)
	{
		strColorBlink = blinkWhite;
	}

	DecalFont_DrawLine(sdata->lngStrings[LNG_CANCEL], SUBMIT_NAME_CANCEL_X, SUBMIT_NAME_ACTION_Y, 1, strColorBlink);

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
	Color color;
	color.self = sdata->battleSetup_Color_UI_1;
	RECTMENU_DrawOuterRect_Edge(&r, color, 0x20, gGT->backBuffer->otMem.uiOT);

	r.y = SUBMIT_NAME_PANEL_Y;
	r.h = SUBMIT_NAME_PANEL_H;
	RECTMENU_DrawInnerRect(&r, 0, gGT->backBuffer->otMem.uiOT);

	int tap = sdata->buttonTapPerPlayer[0];

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

	if ((tap & (BTN_UP | BTN_DOWN | BTN_LEFT | BTN_RIGHT)) == 0)
	{
		if ((tap & BTN_START) == 0)
		{
			if ((tap & (BTN_TRIANGLE | BTN_SQUARE_one)) == 0)
			{
				if (tap & (BTN_CIRCLE | BTN_CROSS_one))
				{
					// Go Back button
					if (cursorPosition == SUBMIT_NAME_CURSOR_BACKSPACE)
					{
						soundID = 2;
						if (currNameLength != 0)
						{
							gGT->currNameEntered[currNameLength - 1] = 0;
						}
					}

					// not Go Back button
					else if (cursorPosition < SUBMIT_NAME_CURSOR_BACKSPACE)
					{
						// Save or Cancel
						cursorCharacter = data.unicodeAscii[cursorPosition];
						if ((data.unicodeAscii[cursorPosition] & 0xff00) == SUBMIT_NAME_ASCII_BYTE_MARKER)
						{
							cursorCharacter &= 0xff;
						}

						// too many letters
						soundID = 5;

						if (nameLength < SUBMIT_NAME_MAX_VISIBLE_CHARS)
						{
							soundID = 1;

							currNameLengthIncrement = currNameLength;
							if (cursorCharacter & 0xff00)
							{
								currNameLengthIncrement = currNameLength + 1;
								gGT->currNameEntered[currNameLength] = (cursorCharacter << 16) >> 24;
							}

							gGT->currNameEntered[currNameLengthIncrement] = cursorCharacter;
						}
					}

					else
					{
						// SAVE button
						if (cursorPosition == SUBMIT_NAME_CURSOR_SAVE)
						{
							soundID = 2;
							selectionResult = 1;
							memmove(gGT->prevNameEntered, gGT->currNameEntered, SUBMIT_NAME_BUFFER_SIZE);
						}

						// CANCEL button
						else
						{
							soundID = 0;
							if (cursorPosition != SUBMIT_NAME_CURSOR_CANCEL)
							{
								goto FinishInput;
							}
							soundID = 3;
							selectionResult = -1;
						}

						RECTMENU_ClearInput();
					}
				}
			}

			// Backspace
			else
			{
				// no sound
				soundID = 0;
				if (currNameLength != 0)
				{
					// go back one character
					soundID = 4;
					gGT->currNameEntered[currNameLength - 1] = 0;

					if (gGT->currNameEntered[currNameLength - 2] < 3)
					{
						gGT->currNameEntered[currNameLength - 2] = 0;
					}
				}
			}
		}
		else
		{
			soundID = 3;
			if (cursorPosition == SUBMIT_NAME_CURSOR_CANCEL)
			{
				selectionResult = -1;
			}
			else
			{
				cursorPosition = SUBMIT_NAME_CURSOR_CANCEL;
				soundID = 1;
			}
		}
	}
	else
	{
		int tap = sdata->buttonTapPerPlayer[0];

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

		if (cursorPosition < 0)
		{
			cursorPosition = SUBMIT_NAME_CURSOR_SAVE;
		}

		cursorMiddleDelta = cursorPosition - SUBMIT_NAME_CURSOR_MIDDLE_THRESHOLD;
		if ((SUBMIT_NAME_CURSOR_BACKSPACE < cursorPosition) && (cursorPosition < SUBMIT_NAME_CURSOR_MIDDLE_THRESHOLD))
		{
			cursorPosition = SUBMIT_NAME_CURSOR_SAVE;
			cursorMiddleDelta = SUBMIT_NAME_CURSOR_MIDDLE_THRESHOLD + 1;
		}
		if ((cursorMiddleDelta & 0xffff) < SUBMIT_NAME_CURSOR_MIDDLE_THRESHOLD)
		{
			cursorPosition = SUBMIT_NAME_CURSOR_BACKSPACE;
		}
		soundID = 1;

		// == Naughty Dog Bug ==
		// This used to be 1002, which would allow you to
		// press Right on SAVE, and cursor would go off-screen
		if (cursorPosition > SUBMIT_NAME_CURSOR_SAVE)
		{
			cursorPosition = 0;
		}
	}

FinishInput:

	if (soundID != 0)
	{
		// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004b0dc-0x8004b0f8 for keyboard SFX lookup/play.
		OtherFX_Play(data.soundIndexArray[soundID], 1);
	}
	gGT->typeCursorPosition = cursorPosition;
	return selectionResult;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004b144-0x8004b230.
void SubmitName_MenuProc(struct RectMenu *menu)
{
	struct GameTracker *gGT = sdata->gGT;

	s16 selection = SubmitName_DrawMenu(0x13f);
	menu->rowSelected = selection;

	// not finished yet
	if (selection == 0)
	{
		return;
	}

	// if name entered for Time Trial
	if (sdata->data10_bbb[0xd] == SUBMIT_NAME_MODE_TIME_TRIAL)
	{
		// if hit CANCEL
		if (selection < 0)
		{
			// end of race menu with "Save Ghost" option
			extern struct RectMenu menu224;
			sdata->ptrDesiredMenu = &menu224;
		}

		// if hit SAVE
		else
		{
			// GhostMode
			SelectProfile_ToggleMode(SELECT_PROFILE_MODE_GHOST_SAVE);
			sdata->ptrDesiredMenu = &data.menuGhostSelection;
		}
	}

	// if name entered for Adventure
	else if (sdata->data10_bbb[0xd] == SUBMIT_NAME_MODE_ADVENTURE)
	{
		// if hit CANCEL
		if (selection < 0)
		{
			// Change active Menu back to Adv char select
			sdata->ptrDesiredMenu = CS_Garage_GetMenuPtr();
			CS_Garage_ZoomOut(1);
		}
		else
		{
			// make backup of name entered
			memmove(sdata->advProgress.name, gGT->prevNameEntered, sizeof(gGT->prevNameEntered));

			// AdventureMode
			SelectProfile_ToggleMode(SELECT_PROFILE_MODE_ADV_SAVE);
			sdata->ptrDesiredMenu = &data.menuFourAdvProfiles;
		}
	}
}
