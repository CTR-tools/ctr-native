#ifndef CTR_NATIVE_NAMESPACE_RECTMENU_H
#define CTR_NATIVE_NAMESPACE_RECTMENU_H

enum MenuFlags
{
	// menu's X position will be used to center it horizontally
	CENTER_ON_X = 1,

	// menu's Y position will be used to center it vertically
	CENTER_ON_Y = 2,

	// menu will be centered on these coordinates
	CENTER_ON_COORDS = 3,

	// only draws the menu's title
	ONLY_DRAW_TITLE = 4,
	// 8

	// allows drawing another menu box while a menu box is being drawn
	// position is automatically derived from originating menu, so cascading windows are easier
	DRAW_NEXT_MENU_IN_HIERARCHY = 0x10,
	DISABLE_INPUT_ALLOW_FUNCPTRS = 0x20,

	// menu will display a single row that changes as you press a direction
	// kind of like scrolling
	SHOW_ONLY_HIGHLIT_ROW = 0x40,

	// instead of the big font, menu will use the small font
	USE_SMALL_FONT = 0x80,

	// do not draw the selected-row highlight box
	HIDE_ROW_HIGHLIGHT = 0x100,

	// center title and row text within the menu width
	CENTER_MENU_TEXT = 0x200,
	EXECUTE_FUNCPTR = 0x400,
	RECTMENU_UNKNOWN_0x800 = 0x800,

	// needs a better name, apparently it's for when it's closed/closing
	NEEDS_TO_CLOSE = 0x1000,
	INVISIBLE = 0x2000,

	// title will use big text
	// to be used in conjunction with "USE_SMALL_FONT_IN_ROWS"
	BIG_TEXT_IN_TITLE = 0x4000,

	ALL_PLAYERS_USE_MENU = 0x8000,
	KEEP_INPUTS_IN_SUBMENU = 0x10000,
	// 0x20000
	// 0x40000
	// 0x80000
	MENU_CANT_GO_BACK = 0x100000,
	// 0x200000
	// 0x400000
	MUTE_SOUND_OF_MOVING_CURSOR = 0x800000
};

enum RectMenuState
{
	RECTMENU_STATE_CALLBACK_CENTERED = RECTMENU_UNKNOWN_0x800 | DISABLE_INPUT_ALLOW_FUNCPTRS | CENTER_ON_COORDS,
	RECTMENU_STATE_SMALL_CALLBACK_CENTERED = RECTMENU_STATE_CALLBACK_CENTERED | USE_SMALL_FONT,
};

enum MenuRowFlags
{
	RECTMENU_STRING_NONE = -1,
	MENU_ROW_LNG_MASK = 0x7fff,
	MENU_ROW_LOCKED = 0x8000,
};

enum RectMenuFuncState
{
	RECTMENU_FUNC_STATE_INPUT = 0,
	RECTMENU_FUNC_STATE_UPDATE = 1,
	RECTMENU_FUNC_STATE_DRAW = 2,
};

enum RectMenuInputMask
{
	RECTMENU_INPUT_MENU = BTN_UP | BTN_DOWN | BTN_LEFT | BTN_RIGHT | BTN_CROSS_one | BTN_SQUARE_one | BTN_CIRCLE | BTN_TRIANGLE,
};

CTR_STATIC_ASSERT(RECTMENU_UNKNOWN_0x800 == 0x800);
CTR_STATIC_ASSERT(HIDE_ROW_HIGHLIGHT == 0x100);
CTR_STATIC_ASSERT(CENTER_MENU_TEXT == 0x200);
CTR_STATIC_ASSERT(RECTMENU_STATE_CALLBACK_CENTERED == 0x823);
CTR_STATIC_ASSERT(RECTMENU_STATE_SMALL_CALLBACK_CENTERED == 0x8A3);
CTR_STATIC_ASSERT(RECTMENU_STRING_NONE == -1);
CTR_STATIC_ASSERT(RECTMENU_FUNC_STATE_INPUT == 0);
CTR_STATIC_ASSERT(RECTMENU_FUNC_STATE_UPDATE == 1);
CTR_STATIC_ASSERT(RECTMENU_FUNC_STATE_DRAW == 2);
CTR_STATIC_ASSERT(RECTMENU_INPUT_MENU == 0x4007f);

struct MenuRow
{
	// can have values above 0xFF,
	// such as 0x155 for "Controller 1C",
	// sometimes the top bit 0x8000 is used,
	// like VS 2P,3P,4P in main menu, to
	// determine if the row is "locked"

	// 0x0
	s16 stringIndex;

	// 0x2
	char rowOnPressUp;

	// 0x3
	char rowOnPressDown;

	// 0x4
	char rowOnPressLeft;

	// 0x5
	char rowOnPressRight;
};

struct RectMenu
{
	// 0x0
	s16 stringIndexTitle; // string index of title (null, with no row)

	// position for current frame
	u16 posX_curr; // X position
	u16 posY_curr; // Y position

	// 0x6
	u16 unk1;

	// 0x8
	// This is an int, see FUN_800469dc
	// & 1, centers posY
	// & 2, centers posX
	// & 4, draw only title bar
	// & 0x10, draw ptrNextBox_InHierarchy
	// & 0x20, disable menu input, allow menu funcptr
	// & 0x40, show only highlighted row
	// & 0x28, main menu character select (better meaning)?
	// & 0x80, tiny text in rows
	// & 0x100, hide row highlight
	// & 0x200, center title and row text
	// & 0xFF, row height (state>>7)
	// & 0x400, execute menu funcptr
	// & 0x800, ??? used in end-event menus
	// & 0x1000, needs to close
	// & 0x2000, invisible
	// & 0x4000, big text in title
	// & 0x8000, anyone can use menu
	// & 0x100000, top of menu hierarchy
	// & 0x800000, mute sound of moving cursor
	u32 state;

	// 0xC
	struct MenuRow *rows;

	// 0x10
	void (*funcPtr)(struct RectMenu *m);

	// 0x14
	// text color, box color, etc
	// one-byte variable with
	// two-byte alignment
	u16 drawStyle;

	// 0x16
	// position for previous frame
	s16 posX_prev;
	s16 posY_prev;

	// 0x1a
	s16 rowSelected;

	// 0x1c
	s16 unk1c;

	// 0x1e
	// tells funcPtr why RectMenu called it
	s16 funcState;

	// 0x20
	s16 width;
	s16 height;

	// 0x24
	struct RectMenu *ptrNextBox_InHierarchy;

	// 0x28
	struct RectMenu *ptrPrevBox_InHierarchy;

	// End of struct
	// in UsaRetail, full struct
	// size is 0x2C

#if BUILD == SepReview
	u8 theRest[8]; // extra size?
#endif
};

CTR_STATIC_ASSERT(sizeof(struct MenuRow) == 6);
CTR_STATIC_ASSERT(MENU_ROW_LNG_MASK == 0x7fff);
CTR_STATIC_ASSERT(MENU_ROW_LOCKED == 0x8000);
#if BUILD != SepReview
CTR_STATIC_ASSERT(sizeof(struct RectMenu) == 0x2C);
#else
CTR_STATIC_ASSERT(sizeof(struct RectMenu) == 0x34);
#endif

#endif
