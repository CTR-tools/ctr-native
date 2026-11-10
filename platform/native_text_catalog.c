#include <platform/native_text_catalog.h>
#include <tomlc17.h>

#include <stdlib.h>
#include <string.h>

enum { NATIVE_TEXT_CATALOG_MAX_VALUE_BYTES = 4096 };

// Same order as the Expanded IDs in namespace_Lng.h.
static const char *const s_nativeExpandedKeys[] = {
	"MODE",
	"CHAOS",
	"REGULAR",
	"CHAOS_RULES",
	"SUPER_HARD",
	"ULTRA_HARD",
	"PRESS_START_TO_SKIP",
	"NO_PREVIEW",
	"CHAOS_SETUP_TITLE",
	"CHAOS_EFFECTS_TITLE",
	"DURATION",
	"TRIGGER",
	"INTERVAL",
	"EFFECTS",
	"CONFIRM",
	"ACTIVE_EFFECTS",
	"RANDOM",
	"PICKUP",
	"EXTERNAL",
	"AUTOMATIC",
	"DISABLED",
	"SELECT_DEVICE",
	"DEVICE",
	"VIBRATION",
	"DEADZONE",
	"RESTORE_DEFAULTS",
	"NONE",
	"CHAOS_EFFECT_MIRROR_MIRROR",
	"CHAOS_EFFECT_GROUNDED",
	"CHAOS_EFFECT_TOTALLY_SOBER",
	"CHAOS_EFFECT_FLY_ME_TO_THE_MOON",
	"CHAOS_EFFECT_HEAVY_METAL",
	"CHAOS_EFFECT_SUNDAY_DRIVER",
	"CHAOS_EFFECT_GOTTA_GO_FAST",
	"CHAOS_EFFECT_HOLE_IN_THE_TANK",
	"CHAOS_EFFECT_FAIR_MODE",
	"CHAOS_EFFECT_UNBOXING_GONE_WRONG",
	"CHAOS_EFFECT_WUMPA_TAX",
	"CHAOS_EFFECT_CLOCKBLOCKED",
	"CHAOS_EFFECT_BROWN_FIRE",
	"CHAOS_EFFECT_ANXIOUS_MASK",
	"CHAOS_EFFECT_UNBREAKABLE",
	"CHAOS_EFFECT_BIG_KART_ENERGY",
	"CHAOS_EFFECT_USE_THIS",
	"CHAOS_EFFECT_JUICED_UP",
	"CHAOS_EFFECT_IT_IS_I",
	"CHAOS_EFFECT_PROVIDENCE",
	"CHAOS_EFFECT_RESERVE_INJECTION",
	"CHAOS_EFFECT_TOO_BOOSTED_TO_FAIL",
	"CHAOS_EFFECT_SABOTAGE",
	"CHAOS_EFFECT_THIS_IS_FIRE",
	"CHAOS_EFFECT_HYDRAULICS",
	"CHAOS_EFFECT_MY_PLANET_NEEDS_ME",
	"CHAOS_EFFECT_WHO_BOUND_THIS",
	"CHAOS_EFFECT_TRY_SOMETHING_NEW",
	"CHAOS_EFFECT_SMALL_KART_ENERGY",
	"CHAOS_EFFECT_CLASSIC_GTA",
	"CHAOS_EFFECT_ARENA_CHAMPIONS",
	"CHAOS_EFFECT_YOUVE_GOT_MAIL",
	"CHAOS_EFFECT_FRESH_START",
	"CHAOS_EFFECT_SENILE_MASK",
	"CHAOS_EFFECT_IDENTITY_CRISIS",
	"CHAOS_EFFECT_BODY_SWAP",
	"CHAOS_EFFECT_YARDRAT_TRAINING",
	"CHAOS_EFFECT_DEFECTIVE_TELEPORT",
	"CHAOS_EFFECT_ZA_WARUDO",
	"CHAOS_EFFECT_KING_CRIMSON",
	"CONFIG_SYSTEM",
	"CONFIG_GAME",
	"CONFIG_MODIFIERS",
	"CONFIG_AUDIO",
	"CONFIG_TEXT",
	"CONFIG_CONTROLS",
	"CONFIG_CUSTOM_TRACKS",
	"CONFIG_SKIP_INTROS",
	"CONFIG_SKIP_PODIUM",
	"CONFIG_SHOW_RESERVES",
	"CONFIG_BOT_SELECTION",
	"CONFIG_MASK_MODE",
	"CONFIG_SKIP_HINTS",
	"CONFIG_BOSS_CUTSCENES",
	"CONFIG_SAVE_ANYWHERE",
	"CONFIG_GRAPHICS",
	"CONFIG_GENERAL",
	"CONFIG_UNLOCKS",
	"CONFIG_VEHICLE",
	"CONFIG_WEAPONS",
	"CONFIG_DEVELOPER_HACKS",
	"CONFIG_OXIDE_MULTIPLAYER",
	"CONFIG_RACE_TRACKS_BATTLE",
	"CONFIG_EXTENDED_MULTIPLAYER",
	"CONFIG_AUTO_END_RACES",
	"CONFIG_EXTRA_DIFFICULTY",
	"CONFIG_EXPANDED_OPTIONS",
	"CONFIG_OPEN_GATES",
	"CONFIG_OPEN_PORTALS",
	"CONFIG_DITHERING",
	"CONFIG_FULLSCREEN",
	"CONFIG_VSYNC",
	"CONFIG_ASPECT_RATIO",
	"CONFIG_RENDER_SCALE",
	"CONFIG_SMOOTH_SCALING",
	"CONFIG_FXAA",
	"CONFIG_TEXTURE_FILTERING",
	"CONFIG_SUBPIXEL",
	"CONFIG_SPEED_MULTIPLIER",
	"CONFIG_GRAVITY_MULTIPLIER",
	"CONFIG_TURN_MULTIPLIER",
	"CONFIG_JUMP_MULTIPLIER",
	"CONFIG_RESERVE_MULTIPLIER",
	"CONFIG_MISSILE_SPEED",
	"CONFIG_BOMB_SPEED",
	"CONFIG_WARPBALL_SPEED",
	"CONFIG_TNT_RADIUS",
	"CONFIG_BOMB_RADIUS",
	"CONFIG_MASK_PROTECTION",
	"CONFIG_MASK_DAMAGE",
	"CONFIG_MASK_PERSISTENCE",
	"CONFIG_MASK_DURATION",
	"CONFIG_MASK_SPEED",
	"CONFIG_CLOCK_DURATION",
	"CONFIG_CLOCK_WEAPONS",
	"CONFIG_UNLOCK_CHARACTERS",
	"CONFIG_UNLOCK_OXIDE",
	"CONFIG_DRAW_DISTANCE",
	"CONFIG_MULTIPLAYER_QUALITY",
	"CONFIG_ENABLE_DEVELOPER",
	"CONFIG_OMNI_CONTROLLER",
	"CONFIG_ONE_LAP",
	"CONFIG_TOW_PLAYER",
	"CONFIG_QUICK_RACE",
	"CONFIG_CHAOS_TEST",
	"CONFIG_SKIP_COUNTDOWN",
	"CONFIG_END_RACE",
	"CONFIG_NORMAL",
	"CONFIG_INVERTED",
	"CONFIG_ALL_UKA",
	"CONFIG_ALL_AKU",
	"CONFIG_VANILLA",
	"CONFIG_RANDOM_UNLOCKED",
	"CONFIG_RANDOM_ALL",
	"CONFIG_UNSKIPPABLE",
	"CONFIG_SKIPPABLE",
	"CONFIG_SKIP",
	"CONFIG_ORIGINAL",
	"CONFIG_NATIVE",
	"OFF",
	"CONFIG_GEOMETRY",
	"CONFIG_PERSPECTIVE",
	"ON",
	"SELECT_LANGUAGE",
	"TRACKS_AVAILABLE",
	"TRACKS_NONE_IMPORTED",
	"TRACKS_AUTHOR",
	"TRACKS_MODES",
	"TRACKS_UNKNOWN",
	"TRACKS_IMPORT",
	"TRACKS_REFRESH",
	"TRACKS_FOLDER",
	"TRACKS_XDELTA_PATCH",
	"TRACKS_TRACK",
	"TRACKS_ACTIVE",
	"TRACKS_NAME",
	"TRACKS_MUSIC_SOURCE",
	"TRACKS_MUSIC_TRACK",
	"TRACKS_CONFIRM_IMPORT",
	"TRACKS_CANCEL_IMPORT",
	"TRACKS_DELETE",
	"TRACKS_BACK",
	"TRACKS_YES",
	"TRACKS_NO",
	"TRACKS_EDIT_PROMPT",
	"TRACKS_DETAILS",
	"TRACKS_REVIEW_IMPORT",
	"TRACKS_CUSTOM",
	"TRACKS_RETAIL",
	"TRACKS_AUDIO",
	"TRACKS_IMPORT_TITLE",
	"TRACKS_DELETE_PROMPT",
	"TRACKS_SAVE_DETAILS_FAILED",
	"TRACKS_REFRESH_DETAILS_FAILED",
	"TRACKS_STATUS_FAILED",
	"TRACKS_EDITOR_FAILED",
	"TRACKS_MUSIC_FAILED",
	"TRACKS_DELETE_FAILED",
	"TRACKS_REFRESH_FAILED",
	"TRACKS_REVIEW_FAILED",
	"TRACKS_FOLDER_DIALOG_FAILED",
	"TRACKS_PATCH_DIALOG_FAILED",
	"TRACKS_REFRESH_REVIEW_FAILED",
	"TRACKS_SELECTION_FAILED",
	"TRACKS_DISCARD_FAILED",
	"TRACKS_SELECT_ONE",
	"TRACKS_START_FAILED",
	"TRACKS_START_PATCH_FAILED",
	"TRACKS_DIALOG_FAILED",
	"TRACKS_CLEANUP_FAILED",
	"TRACKS_CHOOSE_PATCH",
	"TRACKS_CHOOSE_FOLDER",
	"CONFIG_LANGUAGE",
	"CONFIG_ADVENTURE",
	"TRACKS_COPY_ROM",
	"TRACKS_APPLY_PATCH",
	"TRACKS_EXTRACT_ROM",
	"TRACKS_FIND_CIRCUITS",
	"TRACKS_INSTALL_CIRCUITS",
	"TRACKS_REMOVE_TEMP",
	"TRACKS_CANCELLING",
	"IMPORT_INVALID_DESTINATION",
	"IMPORT_PATH_TOO_LONG",
	"IMPORT_CREATE_LIBRARY_FAILED",
	"IMPORT_DESTINATION_EXISTS",
	"IMPORT_CREATE_TEMP_FAILED",
	"IMPORT_MISSING_FILES",
	"IMPORT_COPY_FAILED",
	"IMPORT_INVALID_METADATA_FILES",
	"IMPORT_INVALID_FILES",
	"IMPORT_SCAN_FAILED",
	"IMPORT_DUPLICATE",
	"IMPORT_PUBLISH_FAILED",
	"IMPORT_INVALID_SOURCE",
	"IMPORT_AMBIGUOUS_FILES",
	"IMPORT_READ_FOLDER_FAILED",
	"PATCH_CANCELLED",
	"PATCH_READ_METADATA_FAILED",
	"PATCH_WRITE_METADATA_FAILED",
	"PATCH_MISSING_CIRCUITS",
	"PATCH_COMPARE_CIRCUITS_FAILED",
	"PATCH_SAVE_RETAIL_MUSIC_FAILED",
	"PATCH_NO_MODIFIED_CIRCUITS",
	"PATCH_PREPARE_EXTRA_FAILED",
	"PATCH_EXTRACT_EXTRA_FAILED",
	"PATCH_COMPARE_EXTRA_FAILED",
	"PATCH_READ_PREPARED_FAILED",
	"PATCH_READ_IMPORTED_FAILED",
	"PATCH_COPY_MUSIC_FAILED",
	"PATCH_SAVE_MUSIC_FAILED",
	"PATCH_NONE_SELECTED",
	"PATCH_ROLLBACK_FAILED",
	"PATCH_MISSING_BIGTOOL",
	"PATCH_MISSING_XDELTA",
	"PATCH_COPY_ROM_FAILED",
	"PATCH_APPLY_FAILED",
	"PATCH_EXTRACT_BIGFILE_FAILED",
	"PATCH_BIGTOOL_FAILED",
	"PATCH_TEMP_CLEANUP_FAILED",
	"PATCH_ALREADY_ACTIVE",
	"PATCH_INVALID_PATH",
	"PATCH_NO_REVIEW",
	"PATCH_INVALID_METADATA",
	"PATCH_TRACK_NOT_FOUND",
	"PATCH_IMPORTED_CLEANUP_FAILED",
	"PATCH_IMPORTED_RESTART",
	"PATCH_NO_CONFIRM",
	"PATCH_IMPORTED",
	"PATCH_READY_COUNT",
	"PATCH_IMPORTED_MUSIC_COUNT",
	"CONTROLS_CONFIRM_BINDING",
	"CONTROLS_L1",
	"CONTROLS_L2",
	"CONTROLS_L3",
	"CONTROLS_R1",
	"CONTROLS_R2",
	"CONTROLS_R3",
	"CONTROLS_UP",
	"CONTROLS_DOWN",
	"CONTROLS_LEFT",
	"CONTROLS_RIGHT",
	"CONTROLS_START",
	"CONTROLS_SELECT",
	"CONTROLS_LEFT_STICK_X",
	"CONTROLS_LEFT_STICK_Y",
	"CONTROLS_RIGHT_STICK_X",
	"CONTROLS_RIGHT_STICK_Y",
	"CONTROLS_KEYBOARD",
	"PATCH_IMPORTED_SKIPPED",
	"TRACKS_REFRESH_SUMMARY",
	"CONTROLS_PLAYER_TITLE",
};
_Static_assert(sizeof(s_nativeExpandedKeys) / sizeof(s_nativeExpandedKeys[0]) == LNG_EXPANDED_STRING_COUNT,
               "Expanded key count must match the string IDs");

static int NativeTextCatalog_ValidatePlaceholders(int key, const char *value)
{
	const char *first = NULL, *second = NULL;
	switch (key)
	{
	case LNG_EXPANDED_PATCH_READY_COUNT:
	case LNG_EXPANDED_PATCH_IMPORTED_MUSIC_COUNT: first = "tracks"; break;
	case LNG_EXPANDED_PATCH_IMPORTED_SKIPPED: first = "tracks"; second = "skipped_music"; break;
	case LNG_EXPANDED_TRACKS_REFRESH_SUMMARY: first = "tracks"; second = "rejected"; break;
	case LNG_EXPANDED_CONTROLS_PLAYER_TITLE: first = "player"; break;
	default: return 1;
	}
	int seenFirst = 0, seenSecond = second == NULL;
	for (const char *p = value; *p != '\0'; p++)
	{
		if (*p == '}') { return 0; }
		if (*p != '{') { continue; }
		const char *end = strchr(p + 1, '}');
		if (end == NULL) { return 0; }
		size_t size = (size_t)(end - p - 1);
		if (strlen(first) == size && memcmp(p + 1, first, size) == 0) { seenFirst = 1; }
		else if (second != NULL && strlen(second) == size && memcmp(p + 1, second, size) == 0) { seenSecond = 1; }
		else { return 0; }
		p = end;
	}
	return seenFirst && seenSecond;
}

void NativeTextCatalog_Clear(struct NativeTextCatalog *catalog)
{
	if (catalog == NULL)
	{
		return;
	}
	for (int i = 0; i < LNG_EXPANDED_STRING_COUNT; i++)
	{
		free(catalog->values[i]);
		catalog->values[i] = NULL;
	}
}

int NativeTextCatalog_Parse(struct NativeTextCatalog *catalog, const char *text, int length, int *invalidEntries)
{
	if (catalog == NULL || text == NULL || length < 0)
	{
		return 0;
	}
	toml_option_t options = toml_default_option();
	options.check_utf8 = true;
	toml_set_option(options);
	toml_result_t result = toml_parse(text, length);
	if (!result.ok)
	{
		toml_free(result);
		return 0;
	}

	struct NativeTextCatalog next = {0};
	int invalid = 0;
	for (int i = 0; i < result.toptab.u.tab.size; i++)
	{
		const char *name = result.toptab.u.tab.key[i];
		int nameLength = result.toptab.u.tab.len[i];
		toml_datum_t value = result.toptab.u.tab.value[i];
		int key = 0;
		while (key < LNG_EXPANDED_STRING_COUNT &&
		       ((int)strlen(s_nativeExpandedKeys[key]) != nameLength || memcmp(name, s_nativeExpandedKeys[key], (size_t)nameLength) != 0))
		{
			key++;
		}
		if (key == LNG_EXPANDED_STRING_COUNT || value.type != TOML_STRING || value.u.str.len > NATIVE_TEXT_CATALOG_MAX_VALUE_BYTES ||
		    memchr(value.u.str.ptr, '\0', (size_t)value.u.str.len) != NULL)
		{
			invalid++;
			continue;
		}
		next.values[key] = malloc((size_t)value.u.str.len + 1);
		if (next.values[key] == NULL)
		{
			NativeTextCatalog_Clear(&next);
			toml_free(result);
			return 0;
		}
		memcpy(next.values[key], value.u.str.ptr, (size_t)value.u.str.len);
		next.values[key][value.u.str.len] = '\0';
		if (!NativeTextCatalog_ValidatePlaceholders(LNG_EXPANDED_STRING_BASE + key, next.values[key]))
		{
			free(next.values[key]);
			next.values[key] = NULL;
			invalid++;
		}
	}
	toml_free(result);
	NativeTextCatalog_Clear(catalog);
	*catalog = next;
	if (invalidEntries != NULL)
	{
		*invalidEntries = invalid;
	}
	return 1;
}

const char *NativeTextCatalog_Get(const struct NativeTextCatalog *catalog, int key)
{
	if (catalog == NULL || key < LNG_EXPANDED_STRING_BASE || key >= LNG_EXPANDED_STRING_BASE + LNG_EXPANDED_STRING_COUNT)
	{
		return NULL;
	}
	return catalog->values[key - LNG_EXPANDED_STRING_BASE];
}

const char *NativeTextCatalog_KeyName(int key)
{
	return key >= LNG_EXPANDED_STRING_BASE && key < LNG_EXPANDED_STRING_BASE + LNG_EXPANDED_STRING_COUNT
	           ? s_nativeExpandedKeys[key - LNG_EXPANDED_STRING_BASE] : "";
}

int NativeTextCatalog_Encode(const char *text, char *output, unsigned int capacity)
{
	if (text == NULL || output == NULL || capacity == 0)
	{
		return 0;
	}
	unsigned int used = 0;
	const unsigned char *p = (const unsigned char *)text;
	while (*p != 0)
	{
		unsigned char bytes[2];
		unsigned int count = 1;
		if (*p >= 0x20 && *p <= 0x7e)
		{
			bytes[0] = *p++;
		}
		else if (*p == '\r')
		{
			bytes[0] = *p++;
		}
		else if (p[0] == 0xc3 && (p[1] == 0x91 || p[1] == 0xb1))
		{
			bytes[0] = 3;
			bytes[1] = p[1] == 0x91 ? 'N' : 'n';
			count = 2;
			p += 2;
		}
		else if (p[0] == 0xc2 && (p[1] == 0xa1 || p[1] == 0xbf))
		{
			bytes[0] = p[1] == 0xa1 ? '#' : '$';
			p += 2;
		}
		else
		{
			return 0;
		}
		if (used + count >= capacity)
		{
			return 0;
		}
		memcpy(output + used, bytes, count);
		used += count;
	}
	output[used] = '\0';
	return 1;
}
