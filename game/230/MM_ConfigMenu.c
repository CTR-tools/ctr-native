#include <common.h>

#include <platform/native_controls.h>

// Row arrays with CONFIG entry at bottom, used by MM_MenuProc_Main
struct MenuRow s_rowsMainMenuBasicConfig[] = {
	{0x4C, 7, 1, 0, 0},
	{0x4D, 0, 2, 1, 1},
	{0x4E, 1, 3, 2, 2},
	{0x4F, 2, 4, 3, 3},
	{0x50, 3, 5, 4, 4},
	{0x51, 4, 6, 5, 5},
	{0x0E, 5, 7, 6, 6},
	{0x003, 6, 0, 7, 7},
	{RECTMENU_STRING_NONE, 0, 0, 0, 0},
};

struct MenuRow s_rowsMainMenuWithSBConfig[] = {
	{0x4C, 8, 1, 0, 0},
	{0x4D, 0, 2, 1, 1},
	{0x4E, 1, 3, 2, 2},
	{0x4F, 2, 4, 3, 3},
	{0x50, 3, 5, 4, 4},
	{0x51, 4, 6, 5, 5},
	{0x234, 5, 7, 6, 6},
	{0x0E, 6, 8, 7, 7},
	{0x003, 7, 0, 8, 8},
	{RECTMENU_STRING_NONE, 0, 0, 0, 0},
};

static void MM_MenuProc_Config(struct RectMenu *menu);

// Owns the "Controls" section screen: player selection, device selection, and
// rebinding. Defined in MM_ControlsConfig.c.
void MM_MenuProc_Controls(struct RectMenu *menu);

// True while the device picker is up, so this file leaves the back button to it
// instead of backing out of the whole section.
b32 MM_Controls_IsPickerOpen(void);
void MM_Controls_ResetNavigation(void);

// Union of the button presses this screen accepts: player 1's device and the selected
// player's own. Used for the back button, which this file handles on its behalf.
u32 MM_Controls_MenuTapped(void);

#include "MM_FullScreenMenu.c"

// Section lookup built from g_configEntries at first use.
enum
{
	CONFIG_MAX_SECTIONS = 16,
};

static int s_sectionToEntry[CONFIG_MAX_SECTIONS];
static int s_sectionCount[CONFIG_MAX_SECTIONS];
static int s_numSections = 0;

static void BuildSectionMap(void)
{
	s_numSections = 0;
	const char *curSection = NULL;
	extern NativeConfig g_config;
	for (int i = 0; i < g_numConfigEntries; i++)
	{
		// Only include Developer Hacks section if enabled
		if (strcmp(g_configEntries[i].section, CONFIG_SECTION_DEVELOPER_HACKS) == 0)
		{
			if (!g_config.developerHacksEnabled)
			{
				continue;
			}
		}
		if (curSection == NULL || strcmp(g_configEntries[i].section, curSection) != 0)
		{
			if (s_numSections >= CONFIG_MAX_SECTIONS)
			{
				break;
			}

			curSection = g_configEntries[i].section;
			s_sectionToEntry[s_numSections] = i;
			s_sectionCount[s_numSections] = 0;
			s_numSections++;
		}
		s_sectionCount[s_numSections - 1]++;
	}
}

static int s_currentSection = -1; // -1 = section selector, 0+ = submenu

// Menu-only entries: key names the destination, section names the parent.
static const ConfigEntry s_configNavigation[] = {
	{.section = "", .key = "System", .label = "System", .type = CFG_SUBSECTION},
	{.section = "", .key = "Game", .label = "Game", .type = CFG_SUBSECTION},
	{.section = "", .key = "Modifiers", .label = "Modifiers", .type = CFG_SUBSECTION},
	{.section = "", .key = CONFIG_SECTION_CUSTOM_TRACKS, .label = CONFIG_SECTION_CUSTOM_TRACKS, .type = CFG_ACTION},
	{.section = "", .key = CONFIG_SECTION_DEVELOPER_HACKS, .label = CONFIG_SECTION_DEVELOPER_HACKS, .type = CFG_ACTION},
	{.section = "System", .key = CONFIG_SECTION_AUDIO, .label = CONFIG_SECTION_AUDIO, .type = CFG_ACTION},
	{.section = "System", .key = CONFIG_SECTION_CONTROLS, .label = CONFIG_SECTION_CONTROLS, .type = CFG_ACTION},
	{.section = "System", .key = "Graphics", .label = "Graphics", .type = CFG_ACTION},
	{.section = "Game", .key = "Adventure", .label = "Adventure", .type = CFG_ACTION},
	{.section = "Game", .key = "General", .label = "General", .type = CFG_ACTION},
	{.section = "Game", .key = "Unlocks", .label = "Unlocks", .type = CFG_ACTION},
	{.section = "Modifiers", .key = "Vehicle", .label = "Vehicle", .type = CFG_ACTION},
	{.section = "Modifiers", .key = "Weapons", .label = "Weapons", .type = CFG_ACTION},
};
static const ConfigEntry *s_configGroup;
static s16 s_configRootRow;
static s16 s_configGroupRow;

static int Config_NavigationEntries(const ConfigEntry **entries)
{
	const char *parent = s_configGroup != NULL ? s_configGroup->key : "";
	int count = 0;
	for (size_t i = 0; i < len(s_configNavigation); i++)
	{
		const ConfigEntry *entry = &s_configNavigation[i];
		if (strcmp(entry->section, parent) == 0 &&
		    (strcmp(entry->key, CONFIG_SECTION_DEVELOPER_HACKS) != 0 || g_config.developerHacksEnabled))
		{
			entries[count++] = entry;
		}
	}
	return count;
}

// First visible row in the section list.
static struct FullScreenMenu_Scroll s_configScroll = {0};

static bool Config_IsSection(int section, const char *name)
{
	if ((section < 0) || (section >= s_numSections))
	{
		return false;
	}

	return strcmp(g_configEntries[s_sectionToEntry[section]].section, name) == 0;
}

static struct RectMenu g_configMenu = {
	.stringIndexTitle = -1,
	.state = EXECUTE_FUNCPTR | DISABLE_INPUT_ALLOW_FUNCPTRS,
	.funcPtr = MM_MenuProc_Config,
};

static struct
{
	struct RectMenu *returnMenu;
	enum ConfigMenuContext context;
	s16 returnRow;
} s_configSession;

static void Config_ClearInput(void)
{
	RECTMENU_ClearInput();
	// Config and Controls read taps directly. Keep held state intact: GAMEPAD
	// uses it as the previous frame when deriving the next frame's taps.
	for (int i = 0; i < PLATFORM_INPUT_PAD_COUNT; i++)
	{
		sdata->gGamepads->gamepad[i].buttonsTapped = 0;
	}
}

void MM_ConfigMenu_Open(struct RectMenu *returnMenu, enum ConfigMenuContext context)
{
	if (Config_IsSection(s_currentSection, CONFIG_SECTION_AUDIO))
	{
		MM_AudioConfig_Leave();
	}
	s_configSession.returnMenu = returnMenu;
	s_configSession.context = context;
	s_configSession.returnRow = returnMenu->rowSelected;
	FullScreenMenu_SetShadedFrame(context == CONFIG_MENU_MAIN);
	s_currentSection = -1;
	s_configGroup = NULL;
	s_configRootRow = 0;
	s_configGroupRow = 0;
	MM_CustomTracks_ResetNavigation();
	s_configScroll.offset = 0;
	g_configMenu.rowSelected = 0;
	BuildSectionMap();
	MM_Controls_ResetNavigation();
	Config_ClearInput();
	sdata->ptrDesiredMenu = &g_configMenu;
}

static void Config_Close(void)
{
	MM_Controls_ResetNavigation();
	NativeConfig_Save();
	NativeControls_Save();
	Config_ClearInput();
	s_configSession.returnMenu->rowSelected = s_configSession.returnRow;
	sdata->ptrDesiredMenu = s_configSession.returnMenu;
}

b32 MM_ConfigMenu_IsActive(void)
{
	return sdata->ptrActiveMenu == &g_configMenu;
}

static void Config_UpdateSlider(const struct GamepadBuffer *pad, int *value,
                                const int min, const int max, const int step)
{
	for (int direction = -1; direction <= 1; direction += 2)
	{
		if (FullScreenMenu_OptionStepDue(pad->buttonsHeldCurrFrame, direction))
		{
			FullScreenMenu_OptionStep(value, direction, min, max, step);
		}
	}
}

static bool Config_CanEdit(const ConfigEntry *entry)
{
	return s_configSession.context == CONFIG_MENU_MAIN || entry->availableDuringPause;
}

static void Config_DrawValue(const ConfigEntry *e, int y, uint32_t *ot)
{
	const int color = Config_CanEdit(e) ? WHITE : GRAY;
	if (e->type == CFG_BOOL)
	{
		FullScreenMenu_DrawOnOffColored(*(bool *)e->valuePtr, y, color, ot);
	}
	else if (e->type == CFG_ENUM)
	{
		int val = *(int *)e->valuePtr;
		const char *text = "?";
		for (int j = 0; j < e->numEnumValues; j++)
		{
			if (e->enumValues[j].value == val)
			{
				text = e->enumValues[j].name;
				break;
			}
		}
		FullScreenMenu_DrawValueTextColored(text, y, color, ot);
	}
	else
	{
		FullScreenMenu_DrawPercentColored(*(int *)e->valuePtr, y, color, ot);
	}
}

static void Config_DrawRow(const void *context, int row, int y, bool selected, uint32_t *ot)
{
	const ConfigEntry *entries = context;
	(void)selected;
	FullScreenMenu_DrawRowLabel(entries[row].label, y, ot);
	Config_DrawValue(&entries[row], y, ot);
}

static void MM_MenuProc_Config(struct RectMenu *menu)
{
	struct GameTracker *gGT = sdata->gGT;
	uint32_t *ot = gGT->backBuffer->otMem.uiOT;
	struct GamepadBuffer *pad = &sdata->gGamepads->gamepad[0];
	if (MM_CustomTracks_Update(menu))
	{
		Config_ClearInput();
		return;
	}

	if (s_numSections == 0)
	{
		BuildSectionMap();
	}

	// Inside the Controls section the back button belongs to player 1's device and the
	// selected player's own, so read it through the same union that screen uses.
	const bool inControls = Config_IsSection(s_currentSection, CONFIG_SECTION_CONTROLS);
	const u32 backTapped = inControls ? MM_Controls_MenuTapped() : (u32)pad->buttonsTapped;

	// The device picker consumes back itself, so it must close before this file acts,
	// otherwise one press would close the picker and leave the section in the same
	// frame.
	if (!MM_Controls_IsPickerOpen() && FullScreenMenu_Back(backTapped))
	{

		// A rebind in progress would keep eating input on the next screen.
		NativeControls_CancelCapture();

		if (s_currentSection >= 0)
		{
			if (Config_IsSection(s_currentSection, CONFIG_SECTION_AUDIO))
			{
				MM_AudioConfig_Leave();
			}
			menu->rowSelected = s_configGroup != NULL ? s_configGroupRow : s_configRootRow;
			s_currentSection = -1;
			s_configScroll.offset = 0;
			Config_ClearInput();
		}
		else if (s_configGroup != NULL)
		{
			s_configGroup = NULL;
			menu->rowSelected = s_configRootRow;
			s_configScroll.offset = 0;
			Config_ClearInput();
		}
		else
		{
			Config_Close();
		}
	}

	if (s_currentSection >= 0)
	{
		if (Config_IsSection(s_currentSection, CONFIG_SECTION_CONTROLS))
		{
			// Draws and edits everything itself, including its own input.
			MM_MenuProc_Controls(menu);
			return;
		}
		if (Config_IsSection(s_currentSection, CONFIG_SECTION_AUDIO))
		{
			MM_MenuProc_Audio(menu);
			return;
		}

		const int sec = s_currentSection;
		const int numRows = s_sectionCount[sec];
		const int firstEntry = s_sectionToEntry[sec];

		FullScreenMenu_NavScroll(pad, &s_configScroll, &menu->rowSelected, numRows);

		const ConfigEntry *e = &g_configEntries[firstEntry + menu->rowSelected];
		if (Config_CanEdit(e))
		{
			if (FullScreenMenu_Confirm((u32)pad->buttonsTapped))
			{
				if (e->type == CFG_BOOL)
				{
					*(bool *)e->valuePtr ^= 1;
					NativeConfig_ApplyDependencies(e);
					// Rebuild section map if visibility changed (e.g. Developer Hacks toggle)
					BuildSectionMap();
					// Turning the gate off hides the section we are standing in, so the
					// cached index no longer names a section. Fall back to the section
					// list instead of reading past the entry table.
					if (s_currentSection >= s_numSections)
					{
						s_currentSection = -1;
						menu->rowSelected = s_configRootRow;
						s_configScroll.offset = 0;
						Config_ClearInput();
						return;
					}
				}
				else if (e->type == CFG_ENUM)
				{
					int *val = (int *)e->valuePtr;
					*val = (*val + 1) % e->numEnumValues;
				}
			}

			if (e->type == CFG_INT)
			{
				Config_UpdateSlider(pad, (int *)e->valuePtr, e->min, e->max, e->step);
			}
		}

		FullScreenMenu_DrawScreen(g_configEntries[firstEntry].section, 0, &s_configScroll,
		                          numRows, menu->rowSelected, Config_DrawRow,
		                          &g_configEntries[firstEntry], gGT, ot);
	}
	else
	{
		// Refresh visibility before navigating or drawing the section list.
		if (s_numSections == 0 || (sdata->frameCounter % 30) == 0)
		{
			BuildSectionMap();
		}

		const ConfigEntry *entries[CONFIG_MAX_SECTIONS];
		const char *sections[CONFIG_MAX_SECTIONS];
		int count = Config_NavigationEntries(entries);
		FullScreenMenu_NavScroll(pad, &s_configScroll, &menu->rowSelected, count);

		if (FullScreenMenu_Confirm((u32)pad->buttonsTapped))
		{
			const ConfigEntry *selected = entries[menu->rowSelected];
			if (selected->type == CFG_SUBSECTION)
			{
				s_configRootRow = menu->rowSelected;
				s_configGroup = selected;
				menu->rowSelected = 0;
				s_configScroll.offset = 0;
				Config_ClearInput();
				return;
			}
			if (strcmp(selected->key, CONFIG_SECTION_CUSTOM_TRACKS) == 0)
			{
				MM_CustomTracks_Open(menu, s_configSession.context == CONFIG_MENU_MAIN);
				Config_ClearInput();
				return;
			}
			if (s_configGroup != NULL) { s_configGroupRow = menu->rowSelected; }
			else { s_configRootRow = menu->rowSelected; }
			for (int section = 0; section < s_numSections; section++)
			{
				if (Config_IsSection(section, selected->key)) { s_currentSection = section; break; }
			}
			if (s_currentSection < 0) { return; }
			menu->rowSelected = 0;
			s_configScroll.offset = 0;
		}

		for (int section = 0; section < count; section++)
		{
			sections[section] = entries[section]->label;
		}
		const int columnX = s_configSession.context == CONFIG_MENU_MAIN ? FULLSCREEN_MENU_MAIN_X : FULLSCREEN_MENU_TITLE_X;
		FullScreenMenu_DrawSectionScreen(s_configGroup != NULL ? s_configGroup->label : sdata->lngStrings[LNG_OPTIONS],
		                                 sections, &s_configScroll, count, menu->rowSelected, columnX, gGT, ot);
	}
}
