#ifndef CTR_NATIVE_NAMESPACE_RECTMENU_H
#define CTR_NATIVE_NAMESPACE_RECTMENU_H

enum ConfigMenuContext
{
	CONFIG_MENU_MAIN,
	CONFIG_MENU_PAUSE,
};

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

	// transient close/reset marker cleared by RECTMENU_DrawSelf
	RECTMENU_CLOSE_TRANSIENT = 8,

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
	// Retail only checks this pair together; standalone meanings are not proven.
	RECTMENU_DRAW_CALLBACK_FLAGS = 0x60000,
	// 0x80000
	MENU_CANT_GO_BACK = 0x100000,
	// 0x200000
	// 0x400000
	MUTE_SOUND_OF_MOVING_CURSOR = 0x800000
};

enum RectMenuState
{
	RECTMENU_STATE_CENTERED = RECTMENU_UNKNOWN_0x800 | CENTER_ON_COORDS,
	RECTMENU_STATE_SMALL_CENTERED = RECTMENU_STATE_CENTERED | USE_SMALL_FONT,
	RECTMENU_STATE_EXEC_CENTERED = RECTMENU_STATE_CENTERED | EXECUTE_FUNCPTR,
	RECTMENU_STATE_SMALL_EXEC_CENTERED = RECTMENU_STATE_EXEC_CENTERED | USE_SMALL_FONT,
	RECTMENU_STATE_CALLBACK = RECTMENU_UNKNOWN_0x800 | DISABLE_INPUT_ALLOW_FUNCPTRS,
	RECTMENU_STATE_CALLBACK_CENTERED = RECTMENU_STATE_CALLBACK | CENTER_ON_COORDS,
	RECTMENU_STATE_SMALL_CALLBACK_CENTERED = RECTMENU_STATE_CALLBACK_CENTERED | USE_SMALL_FONT,
	RECTMENU_STATE_SMALL_CALLBACK_CENTER_X = RECTMENU_STATE_CALLBACK | USE_SMALL_FONT | CENTER_ON_X,
	RECTMENU_STATE_SMALL_EXEC_CENTER_X = RECTMENU_UNKNOWN_0x800 | EXECUTE_FUNCPTR | USE_SMALL_FONT | CENTER_ON_X,
	RECTMENU_STATE_INVISIBLE_CALLBACK = RECTMENU_STATE_CALLBACK | INVISIBLE,
};

enum MenuSfx
{
	MENU_SFX_MOVE = 0,
	MENU_SFX_BACK = 1,
	MENU_SFX_CONFIRM = 2,
	MENU_SFX_LOCKED = 5,
};

enum MenuRowFlags
{
	RECTMENU_STRING_NONE = -1,
	MENU_ROW_LNG_MASK = 0x7fff,
	MENU_ROW_LOCKED = 0x8000,
};

enum RectMenuDrawStyle
{
	RECTMENU_DRAW_STYLE_3P4P_LAYOUT = 0x100,
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

struct FullScreenMenu_Scroll
{
	int offset; // first visible row
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

	// End of 0x2c-byte struct
};

enum NewLoadStep
{
	NEWLOAD_STEP_NONE,
	NEWLOAD_STEP_DIFFICULTY,
	NEWLOAD_STEP_CHAOS,
	NEWLOAD_STEP_PLAYERS,
	NEWLOAD_STEP_COUNT,
};

enum ChaosSetting
{
	CHAOS_SETTING_DURATION,
	CHAOS_SETTING_TRIGGER,
	CHAOS_SETTING_INTERVAL,
	CHAOS_SETTING_DISTRIBUTION,
	CHAOS_SETTING_EFFECTS,
	CHAOS_SETTING_CONFIRM,
	CHAOS_SETTING_COUNT,
};

enum ChaosPage
{
	CHAOS_PAGE_SETTINGS,
	CHAOS_PAGE_EFFECTS,
	CHAOS_PAGE_COUNT,
};

// Resident creation and inspection state survives overlays and checkpoints.
// Titles and row specifications are immutable and stay in the menu implementation.
struct NewLoadState
{
	enum NewLoadStep step; // zero is inactive, including at startup
	bool chaosSetupOffered;
	struct RectMenu *previousMenu;
	struct RectMenu *parentOf[NEWLOAD_STEP_COUNT];
};

struct ChaosMenuState
{
	struct RectMenu menu;
	// Non-NULL selects the read-only pause view and its return destination.
	struct RectMenu *returnMenu;
	bool sessionSetup;
	enum ChaosPage page;
	struct FullScreenMenu_Scroll scroll[CHAOS_PAGE_COUNT];
	enum ChaosSetting visibleSettings[CHAOS_SETTING_COUNT];
	int numSettings;
	int effectsRow;
};

enum
{
	CHAOS_PAUSE_ROW_CAPACITY = 9, // seven retail Battle actions, Chaos Rules and a terminator
};

CTR_STATIC_ASSERT(sizeof(struct MenuRow) == 6);
CTR_STATIC_ASSERT(sizeof(struct RectMenu) == 0x2C);

#endif
