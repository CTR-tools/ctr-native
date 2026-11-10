#include <common.h>

#include <platform/native_controls.h>
#include <platform/native_audio.h>
#include <platform/native_language_disc.h>
#include <platform/native_text_runtime.h>
#include <platform/native_text_export.h>

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
#include "MM_ChoicePicker.c"

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
	{.section = "", .key = "System", .lngKey = LNG_EXPANDED_CONFIG_SYSTEM, .type = CFG_SUBSECTION},
	{.section = "", .key = "Game", .lngKey = LNG_EXPANDED_CONFIG_GAME, .type = CFG_SUBSECTION},
	{.section = "", .key = "Modifiers", .lngKey = LNG_EXPANDED_CONFIG_MODIFIERS, .type = CFG_SUBSECTION},
	{.section = "", .key = CONFIG_SECTION_CUSTOM_TRACKS, .lngKey = LNG_EXPANDED_CONFIG_CUSTOM_TRACKS, .type = CFG_ACTION},
	{.section = "", .key = CONFIG_SECTION_DEVELOPER_HACKS, .lngKey = LNG_EXPANDED_CONFIG_DEVELOPER_HACKS, .type = CFG_ACTION},
	{.section = "System", .key = CONFIG_SECTION_AUDIO, .lngKey = LNG_EXPANDED_CONFIG_AUDIO, .type = CFG_ACTION},
	{.section = "System", .key = CONFIG_SECTION_LANGUAGE, .lngKey = LNG_EXPANDED_CONFIG_LANGUAGE, .type = CFG_ACTION},
	{.section = "System", .key = CONFIG_SECTION_CONTROLS, .lngKey = LNG_EXPANDED_CONFIG_CONTROLS, .type = CFG_ACTION},
	{.section = "System", .key = "Graphics", .lngKey = LNG_EXPANDED_CONFIG_GRAPHICS, .type = CFG_ACTION},
	{.section = "Game", .key = "Adventure", .lngKey = LNG_EXPANDED_CONFIG_ADVENTURE, .type = CFG_ACTION},
	{.section = "Game", .key = "General", .lngKey = LNG_EXPANDED_CONFIG_GENERAL, .type = CFG_ACTION},
	{.section = "Game", .key = "Unlocks", .lngKey = LNG_EXPANDED_CONFIG_UNLOCKS, .type = CFG_ACTION},
	{.section = "Modifiers", .key = "Vehicle", .lngKey = LNG_EXPANDED_CONFIG_VEHICLE, .type = CFG_ACTION},
	{.section = "Modifiers", .key = "Weapons", .lngKey = LNG_EXPANDED_CONFIG_WEAPONS, .type = CFG_ACTION},
};
static const ConfigEntry *s_configGroup;
static s16 s_configRootRow;
static s16 s_configGroupRow;
static bool s_languagePickerOpen;
static int s_languageChoices[NATIVE_TEXT_MAX_LANGUAGES];
static int s_languageChoiceCount;
static int s_languageChoice;
static int s_textExportStatus;

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
	s_textExportStatus = 0;
	s_languagePickerOpen = false;
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
	if (entry->valuePtr == &g_config.retailTextPriority && !NativeLanguageDisc_HasAnyText())
	{
		return false;
	}
	return s_configSession.context == CONFIG_MENU_MAIN || entry->availableDuringPause;
}

static const char *Config_LanguageLabel(int slot, const char *fallback)
{
	const char *name = NativeTextRuntime_LanguageSelfName(slot);
	if (name == NULL || name[0] == '\0')
	{
		name = LOAD_LanguageSelfName(slot);
	}
	return name != NULL && name[0] != '\0' ? name : fallback;
}

static void Config_DrawValue(const ConfigEntry *e, int y, uint32_t *ot)
{
	const int color = Config_CanEdit(e) ? WHITE : GRAY;
	if (e->type == CFG_ACTION)
	{
		if (s_textExportStatus != 0)
		{
			FullScreenMenu_DrawValueTextColored(LANG_GetString(s_textExportStatus), y, color, ot);
		}
	}
	else if (e->type == CFG_BOOL)
	{
		FullScreenMenu_DrawOnOffColored(*(bool *)e->valuePtr, y, color, ot);
	}
	else if (e->type == CFG_ENUM)
	{
		int val = *(int *)e->valuePtr;
		const char *text = "?";
		if (e->valuePtr == &g_config.textLanguage && val >= NATIVE_TEXT_CUSTOM_FIRST)
		{
			const char *id = NativeTextRuntime_LanguageId(val);
			text = Config_LanguageLabel(val, id != NULL ? id : "?");
		}
		for (int j = 0; j < e->numEnumValues; j++)
		{
			if (e->enumValues[j].value == val)
			{
				text = e->enumValues[j].lngKey >= LNG_EXPANDED_STRING_BASE
				           ? LANG_GetString(e->enumValues[j].lngKey) : e->enumValues[j].name;
				if (e->valuePtr == &g_config.textLanguage || e->valuePtr == &g_config.voiceLanguage)
				{
					text = Config_LanguageLabel(val, text);
				}
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

static const char *Config_EntryLabel(const ConfigEntry *entry)
{
	return LANG_GetString(entry->lngKey);
}

static const char *Config_SectionLabel(const char *section)
{
	for (size_t i = 0; i < len(s_configNavigation); i++)
	{
		if (strcmp(section, s_configNavigation[i].key) == 0)
		{
			return Config_EntryLabel(&s_configNavigation[i]);
		}
	}
	return section;
}

static void Config_DrawRow(const void *context, int row, int y, bool selected, uint32_t *ot)
{
	const ConfigEntry *entries = context;
	(void)selected;
	if (entries[row].valuePtr == &g_config.retailTextPriority && !Config_CanEdit(&entries[row]))
	{
		DecalFont_DrawLineOT((char *)Config_EntryLabel(&entries[row]), FULLSCREEN_MENU_LABEL_X, y, FONT_SMALL, GRAY, ot);
	}
	else
	{
		FullScreenMenu_DrawRowLabel(Config_EntryLabel(&entries[row]), y, ot);
	}
	Config_DrawValue(&entries[row], y, ot);
}

static void Config_OpenLanguagePicker(const ConfigEntry *entry)
{
	s_languageChoiceCount = 0;
	s_languageChoice = 0;
	bool voice = entry->valuePtr == &g_config.voiceLanguage;
	int count = voice ? entry->numEnumValues : NativeTextRuntime_LanguageCount();
	for (int i = 0; i < count; i++)
	{
		int slot = voice ? entry->enumValues[i].value : NativeTextRuntime_LanguageAt(i);
		if (slot != 1 && !(voice ? NativeLanguageDisc_HasVoice(slot) :
		                    (NativeLanguageDisc_HasText(slot) || NativeTextRuntime_HasText(slot))))
		{
			continue;
		}
		if (slot == *(int *)entry->valuePtr)
		{
			s_languageChoice = s_languageChoiceCount;
		}
		s_languageChoices[s_languageChoiceCount++] = slot;
	}
	s_languagePickerOpen = true;
}

static const char *Config_LanguageChoiceLabel(const void *context, int index)
{
	const ConfigEntry *entry = context;
	int slot = s_languageChoices[index];
	const char *fallback = NativeTextRuntime_LanguageId(slot);
	for (int i = 0; i < entry->numEnumValues; i++)
	{
		if (entry->enumValues[i].value == slot) { fallback = entry->enumValues[i].name; break; }
	}
	return Config_LanguageLabel(slot, fallback != NULL ? fallback : "?");
}

static bool Config_ApplyLanguageChoice(const void *context, int selected)
{
	const ConfigEntry *entry = context;
	int slot = s_languageChoices[selected];
	bool applied = true;
	if (entry->valuePtr == &g_config.textLanguage)
	{
		applied = LOAD_ApplyLanguage(slot) != 0;
		if (applied)
		{
			g_config.textLanguage = slot;
			LANG_Refresh(slot);
		}
	}
	else
	{
		g_config.voiceLanguage = slot;
		NativeAudio_RefreshVoiceLanguage();
	}
	NativeConfig_Save();
	return applied;
}

static void Config_LanguagePicker(const ConfigEntry *entry, struct GameTracker *gGT, u32 *ot, u32 tapped)
{
	enum ChoicePickerResult result = ChoicePicker_UpdateAndApply(&s_languagePickerOpen, &s_languageChoice,
	                                                            s_languageChoiceCount, tapped, Config_ApplyLanguageChoice, entry);
	if (result != CHOICE_PICKER_NONE)
	{
		Config_ClearInput();
		return;
	}
	ChoicePicker_Draw(LANG_GetString(LNG_EXPANDED_SELECT_LANGUAGE), s_languageChoiceCount, s_languageChoice, Config_LanguageChoiceLabel, entry, gGT, ot);
}

static void MM_MenuProc_Config(struct RectMenu *menu)
{
	if (g_config.retailTextPriority != RETAIL_TEXT_PRIORITY_EXPANDED && !NativeLanguageDisc_HasAnyText())
	{
		g_config.retailTextPriority = RETAIL_TEXT_PRIORITY_EXPANDED;
		LANG_Refresh(g_config.textLanguage);
	}
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
	if (!MM_Controls_IsPickerOpen() && !s_languagePickerOpen && FullScreenMenu_Back(backTapped))
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
		if (Config_IsSection(sec, CONFIG_SECTION_LANGUAGE) && s_languagePickerOpen)
		{
			Config_LanguagePicker(&g_configEntries[firstEntry + menu->rowSelected], gGT, ot, (u32)pad->buttonsTapped);
			FullScreenMenu_DrawScreen(Config_SectionLabel(g_configEntries[firstEntry].section), 0, &s_configScroll, numRows, menu->rowSelected,
			                          Config_DrawRow, &g_configEntries[firstEntry], gGT, ot);
			return;
		}

		FullScreenMenu_NavScroll(pad, &s_configScroll, &menu->rowSelected, numRows);

		const ConfigEntry *e = &g_configEntries[firstEntry + menu->rowSelected];
		if (Config_CanEdit(e))
		{
			if (FullScreenMenu_Confirm((u32)pad->buttonsTapped))
			{
				if (e->type == CFG_ACTION && strcmp(e->key, "export_active_language") == 0)
				{
					enum NativeTextExportResult result = NativeTextExport_Active(g_config.textLanguage, LANG_GetString);
					s_textExportStatus = result == NATIVE_TEXT_EXPORT_OK ? LNG_EXPANDED_EXPORT_DONE :
					                     result == NATIVE_TEXT_EXPORT_EXISTS ? LNG_EXPANDED_EXPORT_EXISTS : LNG_EXPANDED_EXPORT_FAILED;
				}
				else if (e->type == CFG_BOOL)
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
					if (e->valuePtr == &g_config.textLanguage || e->valuePtr == &g_config.voiceLanguage)
					{
						Config_OpenLanguagePicker(e);
						Config_ClearInput();
					}
					else
					{
						int *val = (int *)e->valuePtr;
						*val = (*val + 1) % e->numEnumValues;
						if (e->valuePtr == &g_config.retailTextPriority)
						{
							LANG_Refresh(g_config.textLanguage);
						}
					}
				}
			}

			if (e->type == CFG_INT)
			{
				Config_UpdateSlider(pad, (int *)e->valuePtr, e->min, e->max, e->step);
			}
		}

		FullScreenMenu_DrawScreen(Config_SectionLabel(g_configEntries[firstEntry].section), 0, &s_configScroll,
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
			sections[section] = Config_EntryLabel(entries[section]);
		}
		const int columnX = s_configSession.context == CONFIG_MENU_MAIN ? FULLSCREEN_MENU_MAIN_X : FULLSCREEN_MENU_TITLE_X;
		FullScreenMenu_DrawSectionScreen(s_configGroup != NULL ? Config_EntryLabel(s_configGroup) : sdata->lngStrings[LNG_OPTIONS],
		                                 sections, &s_configScroll, count, menu->rowSelected, columnX, gGT, ot);
	}
}
