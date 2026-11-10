#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <platform/native_config.h>
#include <platform/native_assets.h>
#include <platform/native_path.h>
#include <platform/native_language_disc.h>
#include <namespace_Lng.h>

static const ConfigEnumValue kMaskModeValues[] = {
    {"Normal", MASK_MODE_NORMAL, LNG_EXPANDED_CONFIG_NORMAL},
    {"Random", MASK_MODE_RANDOM, LNG_EXPANDED_RANDOM},
    {"Inverted", MASK_MODE_INVERTED, LNG_EXPANDED_CONFIG_INVERTED},
    {"All Uka", MASK_MODE_ALL_UKA, LNG_EXPANDED_CONFIG_ALL_UKA},
    {"All Aku", MASK_MODE_ALL_AKU, LNG_EXPANDED_CONFIG_ALL_AKU},
};

static const ConfigEnumValue kLanguageValues[] = {
    {"English", NATIVE_LANGUAGE_ENGLISH, 0}, {"French", NATIVE_LANGUAGE_FRENCH, 0}, {"German", NATIVE_LANGUAGE_GERMAN, 0},
    {"Italian", NATIVE_LANGUAGE_ITALIAN, 0}, {"Spanish", NATIVE_LANGUAGE_SPANISH, 0}, {"Dutch", NATIVE_LANGUAGE_DUTCH, 0},
};
static const ConfigEnumValue kRetailTextPriorityValues[] = {
    {"EXPANDED", RETAIL_TEXT_PRIORITY_EXPANDED, 0},
    {"ISO EU", RETAIL_TEXT_PRIORITY_ISO_EU, 0},
};
#define NUM_MASK_MODES (sizeof(kMaskModeValues) / sizeof(kMaskModeValues[0]))

static const ConfigEnumValue kBotSelectionModeValues[] = {
    {"Vanilla", BOT_SELECTION_PREDETERMINED, LNG_EXPANDED_CONFIG_VANILLA},
    {"R. unlocked", BOT_SELECTION_RANDOM_UNLOCKED, LNG_EXPANDED_CONFIG_RANDOM_UNLOCKED},
    {"R. all", BOT_SELECTION_RANDOM_ALL, LNG_EXPANDED_CONFIG_RANDOM_ALL},
};
#define NUM_BOT_SELECTION_MODES (sizeof(kBotSelectionModeValues) / sizeof(kBotSelectionModeValues[0]))

static const ConfigEnumValue kBossCutsceneValues[] = {
    {"Unskippable", BOSS_CUTSCENES_UNSKIPPABLE, LNG_EXPANDED_CONFIG_UNSKIPPABLE},
    {"Skippable", BOSS_CUTSCENES_SKIPPABLE, LNG_EXPANDED_CONFIG_SKIPPABLE},
    {"Skip", BOSS_CUTSCENES_SKIP, LNG_EXPANDED_CONFIG_SKIP},
};
#define NUM_BOSS_CUTSCENE_MODES (sizeof(kBossCutsceneValues) / sizeof(kBossCutsceneValues[0]))

static const ConfigEnumValue kAspectRatioValues[] = {
    {"4:3", 0, 0},
    {"16:9", 1, 0},
    {"16:10", 2, 0},
    {"21:9", 3, 0},
};
#define NUM_ASPECT_RATIOS (sizeof(kAspectRatioValues) / sizeof(kAspectRatioValues[0]))

static const ConfigEnumValue kRenderScaleValues[] = {
    {"Original", 1, LNG_EXPANDED_CONFIG_ORIGINAL},
    {"2X", 2, 0},
    {"3X", 3, 0},
    {"4X", 4, 0},
    {"Native", 0, LNG_EXPANDED_CONFIG_NATIVE},
};
#define NUM_RENDER_SCALES (sizeof(kRenderScaleValues) / sizeof(kRenderScaleValues[0]))

static const ConfigEnumValue kPgxpModeValues[] = {
    {"Off", PGXP_MODE_OFF, LNG_EXPANDED_OFF},
    {"Geometry", PGXP_MODE_GEOMETRY, LNG_EXPANDED_CONFIG_GEOMETRY},
    {"Perspective", PGXP_MODE_PERSPECTIVE, LNG_EXPANDED_CONFIG_PERSPECTIVE},
};
#define NUM_PGXP_MODES (sizeof(kPgxpModeValues) / sizeof(kPgxpModeValues[0]))

NativeConfig g_config = {
  .textLanguage = NATIVE_LANGUAGE_ENGLISH,
  .voiceLanguage = NATIVE_LANGUAGE_ENGLISH,
  .retailTextPriority = RETAIL_TEXT_PRIORITY_EXPANDED,
  .skipIntro = false,
  .skipPodiumCeremonies = false,
  .showReservesMeter = false,
  .allowOxideStationMultiplayer = false,
  .allowRaceTracksInBattle = false,
  .extendedMultiplayer = false,
  .botSelectionMode = BOT_SELECTION_PREDETERMINED,
  .skipHints = false,
  .bossCutscenes = BOSS_CUTSCENES_UNSKIPPABLE,
  .expandedAdventureOptions = false,
  .extraDifficultyLevels = false,

  .speedMultiplier = 100,
  .gravityMultiplier = 100,
  .turnMultiplier = 100,
  .jumpMultiplier = 100,
  .reserveMultiplier = 100,

  .missileSpeedMultiplier =  100,
  .bombSpeedMultiplier = 100,
  .warpballSpeedMultiplier = 100,
  .bombExplosionRadiusMultiplier = 100,
  .tntExplosionRadiusMultiplier = 100,

  .unlockAllCharacters = false,
  .unlockNitrosOxide = false,
  .unlockAllGates = false,
  .unlockAllPortals = false,

  .increaseDrawDistance = false,
  .disableSplitScreenLod = false,

  .fullscreen = false,
  .vsync = false,
  .aspectRatio = 0,
  .dithering = true,
  .renderScale = 0,
  .smoothScaling = true,
  .fxaa = false,
  .textureFiltering = false,
  .pgxpMode = PGXP_MODE_OFF,
  .saveAnywhere = false,
  .maskMode = MASK_MODE_NORMAL,
  .maskProtectsFromDamage = true,
  .maskDamagesOthers = true,
  .maskPersistsAfterOOB = false,
  .maskDurationMultiplier = 100,
  .maskExtraSpeedMultiplier = 100,
  .clockDurationMultiplier = 100,
  .allowWeaponsDuringClock = false,
  .autoEndRaceWhenOthersFinish = false,
  .developerHacksEnabled = false,
  .omniController = false,
  .oneLapRaces = false,
  .towPlayer2 = false,
  .quickRace = false,
  .chaosQuickRace = false,
  .skipStartCountdown = false,
  .endRaceWithL3 = false
};

const ConfigEntry g_configEntries[] = {

    {
        .section = CONFIG_SECTION_LANGUAGE, .key = "audio", .lngKey = LNG_EXPANDED_CONFIG_AUDIO,
        .type = CFG_ENUM, .valuePtr = &g_config.voiceLanguage,
        .enumValues = kLanguageValues, .numEnumValues = sizeof(kLanguageValues) / sizeof(kLanguageValues[0]),
        .availableDuringPause = true,
    },
    {
        .section = CONFIG_SECTION_LANGUAGE, .key = "text", .lngKey = LNG_EXPANDED_CONFIG_TEXT,
        .type = CFG_ENUM, .valuePtr = &g_config.textLanguage,
        .enumValues = kLanguageValues, .numEnumValues = sizeof(kLanguageValues) / sizeof(kLanguageValues[0]),
        .availableDuringPause = true,
    },
    {
        .section = CONFIG_SECTION_LANGUAGE, .key = "retail_priority", .lngKey = LNG_EXPANDED_CONFIG_RETAIL_PRIORITY,
        .type = CFG_ENUM, .valuePtr = &g_config.retailTextPriority,
        .enumValues = kRetailTextPriorityValues, .numEnumValues = sizeof(kRetailTextPriorityValues) / sizeof(kRetailTextPriorityValues[0]),
        .availableDuringPause = true,
    },
    {
        .section = CONFIG_SECTION_LANGUAGE, .key = "export_active_language",
        .lngKey = LNG_EXPANDED_EXPORT_ACTIVE_LANGUAGE, .type = CFG_ACTION, .availableDuringPause = true,
    },

    {
        .section = CONFIG_SECTION_AUDIO,
        .key = "audio",
        .lngKey = LNG_EXPANDED_CONFIG_AUDIO,
        .type = CFG_ACTION,
        .availableDuringPause = true,
    },
    {
        // No value backing: this row only exists so the options menu offers a
        // "Controls" section. MM_MenuProc_Config hands that section to
        // MM_MenuProc_Controls, which owns the screen, and the loader and saver
        // both skip CFG_ACTION entries.
        .section = CONFIG_SECTION_CONTROLS,
        .key = "controls",
        .availableDuringPause = true,
        .lngKey = LNG_EXPANDED_CONFIG_CONTROLS,
        .type = CFG_ACTION,
    },
    {
        .section = CONFIG_SECTION_CUSTOM_TRACKS,
        .key = "custom_tracks",
        .availableDuringPause = true,
        .lngKey = LNG_EXPANDED_CONFIG_CUSTOM_TRACKS,
        .type = CFG_ACTION,
    },
    {
        .section = "General",
        .key = "skip_intro",
        .availableDuringPause = true,
        .lngKey = LNG_EXPANDED_CONFIG_SKIP_INTROS,
        .type = CFG_BOOL,
        .valuePtr = &g_config.skipIntro
    },
    {
        .section = "General",
        .key = "skip_podium_ceremonies",
        .availableDuringPause = true,
        .lngKey = LNG_EXPANDED_CONFIG_SKIP_PODIUM,
        .type = CFG_BOOL,
        .valuePtr = &g_config.skipPodiumCeremonies
    },
    {
        .section = "General",
        .key = "show_reserves_meter",
        .availableDuringPause = true,
        .lngKey = LNG_EXPANDED_CONFIG_SHOW_RESERVES,
        .type = CFG_BOOL,
        .valuePtr = &g_config.showReservesMeter
    },
    {
        .section = "General",
        .key = "allow_oxide_station_multiplayer",
    	.availableDuringPause = true,
        .lngKey = LNG_EXPANDED_CONFIG_OXIDE_MULTIPLAYER,
        .type = CFG_BOOL,
        .valuePtr = &g_config.allowOxideStationMultiplayer
    },
    {
        .section = "General",
        .key = "allow_race_tracks_in_battle",
    	.availableDuringPause = true,
        .lngKey = LNG_EXPANDED_CONFIG_RACE_TRACKS_BATTLE,
        .type = CFG_BOOL,
        .valuePtr = &g_config.allowRaceTracksInBattle
    },
    {
        .section = "General",
        .key = "extended_multiplayer",
    	.availableDuringPause = true,
        .lngKey = LNG_EXPANDED_CONFIG_EXTENDED_MULTIPLAYER,
        .type = CFG_BOOL,
        .valuePtr = &g_config.extendedMultiplayer
    },
    {
        .section = "General",
        .key = "bot_selection_mode",
    	.availableDuringPause = true,
        .lngKey = LNG_EXPANDED_CONFIG_BOT_SELECTION,
        .type = CFG_ENUM,
        .valuePtr = &g_config.botSelectionMode,
        .enumValues = kBotSelectionModeValues,
        .numEnumValues = NUM_BOT_SELECTION_MODES
    },
      {
        .section = "General",
        .key = "mask_mode",
      	.availableDuringPause = true,
        .lngKey = LNG_EXPANDED_CONFIG_MASK_MODE,
        .type = CFG_ENUM,
        .valuePtr = &g_config.maskMode,
        .enumValues = kMaskModeValues,
        .numEnumValues = NUM_MASK_MODES
      },
    {
        .section = "General",
        .key = "auto_end_race_when_others_finish",
        .lngKey = LNG_EXPANDED_CONFIG_AUTO_END_RACES,
        .type = CFG_BOOL,
        .valuePtr = &g_config.autoEndRaceWhenOthersFinish
    },
    {
        .section = "General",
        .key = "extra_difficulty_levels",
        .lngKey = LNG_EXPANDED_CONFIG_EXTRA_DIFFICULTY,
        .type = CFG_BOOL,
        .valuePtr = &g_config.extraDifficultyLevels
    },
    {
        .section = "Adventure",
        .key = "skip_hints",
        .availableDuringPause = true,
        .lngKey = LNG_EXPANDED_CONFIG_SKIP_HINTS,
        .type = CFG_BOOL,
        .valuePtr = &g_config.skipHints
    },
    {
        .section = "Adventure",
        .key = "boss_cutscenes",
        .availableDuringPause = true,
        .lngKey = LNG_EXPANDED_CONFIG_BOSS_CUTSCENES,
        .type = CFG_ENUM,
        .valuePtr = &g_config.bossCutscenes,
        .enumValues = kBossCutsceneValues,
        .numEnumValues = NUM_BOSS_CUTSCENE_MODES
    },
    {
        .section = "Adventure",
        .key = "expanded_options",
    	.availableDuringPause = true,
        .lngKey = LNG_EXPANDED_CONFIG_EXPANDED_OPTIONS,
        .type = CFG_BOOL,
        .valuePtr = &g_config.expandedAdventureOptions
    },
    {
        .section = "Adventure",
        .key = "save_anywhere",
    	.availableDuringPause = true,
        .lngKey = LNG_EXPANDED_CONFIG_SAVE_ANYWHERE,
        .type = CFG_BOOL,
        .valuePtr = &g_config.saveAnywhere
    },
    {
        .section = "Adventure",
        .key = "unlock_all_gates",
        .lngKey = LNG_EXPANDED_CONFIG_OPEN_GATES,
        .type = CFG_BOOL,
        .valuePtr = &g_config.unlockAllGates
    },
    {
        .section = "Adventure",
        .key = "unlock_all_portals",
        .lngKey = LNG_EXPANDED_CONFIG_OPEN_PORTALS,
        .type = CFG_BOOL,
        .valuePtr = &g_config.unlockAllPortals
    },
    {
        .section = "Vehicle",
        .key = "speed_stat_multiplier",
        .lngKey = LNG_EXPANDED_CONFIG_SPEED_MULTIPLIER,
        .type = CFG_INT,
        .valuePtr = &g_config.speedMultiplier,
        .min = 10,
        .max = 200,
        .step = 10
    },
    {
        .section = "Vehicle",
        .key = "gravity_stat_multiplier",
        .lngKey = LNG_EXPANDED_CONFIG_GRAVITY_MULTIPLIER,
        .type = CFG_INT,
        .valuePtr = &g_config.gravityMultiplier,
        .min = 10,
        .max = 300,
        .step = 10
    },
    {
        .section = "Vehicle",
        .key = "turn_stat_multiplier",
        .lngKey = LNG_EXPANDED_CONFIG_TURN_MULTIPLIER,
        .type = CFG_INT,
        .valuePtr = &g_config.turnMultiplier,
        .min = 10,
        .max = 400,
        .step = 10
    },
    {
        .section = "Vehicle",
        .key = "jump_stat_multiplier",
        .lngKey = LNG_EXPANDED_CONFIG_JUMP_MULTIPLIER,
        .type = CFG_INT,
        .valuePtr = &g_config.jumpMultiplier,
        .min = 10,
        .max = 300,
        .step = 10
    },
    {
        .section = "Vehicle",
        .key = "turbo_reserves_multiplier",
        .lngKey = LNG_EXPANDED_CONFIG_RESERVE_MULTIPLIER,
        .type = CFG_INT,
        .valuePtr = &g_config.reserveMultiplier,
        .min = 0,
        .max = 400,
        .step = 20
    },
    {
        .section = "Weapons",
        .key = "missile_speed_multiplier",
        .lngKey = LNG_EXPANDED_CONFIG_MISSILE_SPEED,
        .type = CFG_INT,
        .valuePtr = &g_config.missileSpeedMultiplier,
        .min = 20,
        .max = 500,
        .step = 20
    },
    {
        .section = "Weapons",
        .key = "bomb_speed_multiplier",
        .lngKey = LNG_EXPANDED_CONFIG_BOMB_SPEED,
        .type = CFG_INT,
        .valuePtr = &g_config.bombSpeedMultiplier,
        .min = 20,
        .max = 300,
        .step = 20
    },
    {
        .section = "Weapons",
        .key = "warpball_speed_multiplier",
        .lngKey = LNG_EXPANDED_CONFIG_WARPBALL_SPEED,
        .type = CFG_INT,
        .valuePtr = &g_config.warpballSpeedMultiplier,
        .min = 20,
        .max = 300,
        .step = 20
    },
    {
      .section = "Weapons",
      .key = "tnt_explosion_radius",
      .lngKey = LNG_EXPANDED_CONFIG_TNT_RADIUS,
      .type = CFG_INT,
      .valuePtr = &g_config.tntExplosionRadiusMultiplier,
      .min = 50,
      .max = 600,
      .step = 50
    },
    {
        .section = "Weapons",
        .key = "bomb_explosion_radius_multiplier",
        .lngKey = LNG_EXPANDED_CONFIG_BOMB_RADIUS,
        .type = CFG_INT,
        .valuePtr = &g_config.bombExplosionRadiusMultiplier,
        .min = 50,
        .max = 600,
        .step = 50
    },
      {
        .section = "Weapons",
        .key = "mask_protects_from_damage",
        .lngKey = LNG_EXPANDED_CONFIG_MASK_PROTECTION,
        .type = CFG_BOOL,
        .valuePtr = &g_config.maskProtectsFromDamage
      },
      {
        .section = "Weapons",
        .key = "mask_damages_others",
        .lngKey = LNG_EXPANDED_CONFIG_MASK_DAMAGE,
        .type = CFG_BOOL,
        .valuePtr = &g_config.maskDamagesOthers
      },
    {
        .section = "Weapons",
        .key = "mask_persists_after_oob",
        .lngKey = LNG_EXPANDED_CONFIG_MASK_PERSISTENCE,
        .type = CFG_BOOL,
        .valuePtr = &g_config.maskPersistsAfterOOB
    },
    {
        .section = "Weapons",
        .key = "mask_duration_multiplier",
        .lngKey = LNG_EXPANDED_CONFIG_MASK_DURATION,
        .type = CFG_INT,
        .valuePtr = &g_config.maskDurationMultiplier,
        .min = 20,
        .max = 250,
        .step = 10
    },
    {
        .section = "Weapons",
        .key = "mask_extra_speed_multiplier",
        .lngKey = LNG_EXPANDED_CONFIG_MASK_SPEED,
        .type = CFG_INT,
        .valuePtr = &g_config.maskExtraSpeedMultiplier,
        .min = 0,
        .max = 300,
        .step = 20
    },
    {
        .section = "Weapons",
        .key = "clock_duration_multiplier",
        .lngKey = LNG_EXPANDED_CONFIG_CLOCK_DURATION,
        .type = CFG_INT,
        .valuePtr = &g_config.clockDurationMultiplier,
        .min = 20,
        .max = 250,
        .step = 10
    },
    {
        .section = "Weapons",
        .key = "allow_weapons_during_clock",
        .lngKey = LNG_EXPANDED_CONFIG_CLOCK_WEAPONS,
        .type = CFG_BOOL,
        .valuePtr = &g_config.allowWeaponsDuringClock
    },
    {
        .section = "Unlocks",
        .key = "unlock_all_characters",
    	.availableDuringPause = true,
        .lngKey = LNG_EXPANDED_CONFIG_UNLOCK_CHARACTERS,
        .type = CFG_BOOL,
        .valuePtr = &g_config.unlockAllCharacters
    },
    {
        .section = "Unlocks",
        .key = "unlock_nitros_oxide",
    	.availableDuringPause = true,
        .lngKey = LNG_EXPANDED_CONFIG_UNLOCK_OXIDE,
        .type = CFG_BOOL,
        .valuePtr = &g_config.unlockNitrosOxide
    },
    {
        .section = "Graphics",
        .key = "increase_draw_distance",
        .availableDuringPause = true,
        .lngKey = LNG_EXPANDED_CONFIG_DRAW_DISTANCE,
        .type = CFG_BOOL,
        .valuePtr = &g_config.increaseDrawDistance
    },
    {
        .section = "Graphics",
        .key = "disable_split_screen_lod",
    	.availableDuringPause = true,
        .lngKey = LNG_EXPANDED_CONFIG_MULTIPLAYER_QUALITY,
        .type = CFG_BOOL,
        .valuePtr = &g_config.disableSplitScreenLod
    },
    {
        .section = "Graphics",
        .key = "dithering",
        .availableDuringPause = true,
        .lngKey = LNG_EXPANDED_CONFIG_DITHERING,
        .type = CFG_BOOL,
        .valuePtr = &g_config.dithering
    },
    {
        .section = "Graphics",
        .key = "fullscreen",
        .availableDuringPause = true,
        .lngKey = LNG_EXPANDED_CONFIG_FULLSCREEN,
        .type = CFG_BOOL,
        .valuePtr = &g_config.fullscreen
    },
    {
        .section = "Graphics",
        .key = "vsync",
        .availableDuringPause = true,
        .lngKey = LNG_EXPANDED_CONFIG_VSYNC,
        .type = CFG_BOOL,
        .valuePtr = &g_config.vsync
    },
    {
        .section = "Graphics",
        .key = "aspect_ratio",
        .availableDuringPause = true,
        .lngKey = LNG_EXPANDED_CONFIG_ASPECT_RATIO,
        .type = CFG_ENUM,
        .valuePtr = &g_config.aspectRatio,
        .enumValues = kAspectRatioValues,
        .numEnumValues = NUM_ASPECT_RATIOS
    },
    {
        .section = "Graphics",
        .key = "render_scale",
        .availableDuringPause = true,
        .lngKey = LNG_EXPANDED_CONFIG_RENDER_SCALE,
        .type = CFG_ENUM,
        .valuePtr = &g_config.renderScale,
        .enumValues = kRenderScaleValues,
        .numEnumValues = NUM_RENDER_SCALES
    },
    {
        .section = "Graphics",
        .key = "smooth_scaling",
        .availableDuringPause = true,
        .lngKey = LNG_EXPANDED_CONFIG_SMOOTH_SCALING,
        .type = CFG_BOOL,
        .valuePtr = &g_config.smoothScaling
    },
    {
        .section = "Graphics",
        .key = "fxaa",
        .availableDuringPause = true,
        .lngKey = LNG_EXPANDED_CONFIG_FXAA,
        .type = CFG_BOOL,
        .valuePtr = &g_config.fxaa
    },
    {
        .section = "Graphics",
        .key = "texture_filtering",
        .availableDuringPause = true,
        .lngKey = LNG_EXPANDED_CONFIG_TEXTURE_FILTERING,
        .type = CFG_BOOL,
        .valuePtr = &g_config.textureFiltering
    },
    {
        .section = "Graphics",
        .key = "pgxp",
        .availableDuringPause = true,
        .lngKey = LNG_EXPANDED_CONFIG_SUBPIXEL,
        .type = CFG_ENUM,
        .valuePtr = &g_config.pgxpMode,
        .enumValues = kPgxpModeValues,
        .numEnumValues = NUM_PGXP_MODES
    },
    {
        .section = CONFIG_SECTION_DEVELOPER_HACKS,
        .key = "developer_hacks_enabled",
        .availableDuringPause = true,
        .lngKey = LNG_EXPANDED_CONFIG_ENABLE_DEVELOPER,
        .type = CFG_BOOL,
        .valuePtr = &g_config.developerHacksEnabled,
    },
    {
        .section = CONFIG_SECTION_DEVELOPER_HACKS,
        .key = "omni_controller",
        .availableDuringPause = true,
        .lngKey = LNG_EXPANDED_CONFIG_OMNI_CONTROLLER,
        .type = CFG_BOOL,
        .valuePtr = &g_config.omniController,
    },
    {
        .section = CONFIG_SECTION_DEVELOPER_HACKS,
        .key = "one_lap_races",
        .availableDuringPause = true,
        .lngKey = LNG_EXPANDED_CONFIG_ONE_LAP,
        .type = CFG_BOOL,
        .valuePtr = &g_config.oneLapRaces,
    },
    {
        .section = CONFIG_SECTION_DEVELOPER_HACKS,
        .key = "tow_player_2",
        .availableDuringPause = true,
        .lngKey = LNG_EXPANDED_CONFIG_TOW_PLAYER,
        .type = CFG_BOOL,
        .valuePtr = &g_config.towPlayer2,
    },
    {
        .section = CONFIG_SECTION_DEVELOPER_HACKS,
        .key = "quick_race",
        .availableDuringPause = true,
        .lngKey = LNG_EXPANDED_CONFIG_QUICK_RACE,
        .type = CFG_BOOL,
        .valuePtr = &g_config.quickRace,
    },
    {
        .section = CONFIG_SECTION_DEVELOPER_HACKS,
        .key = "chaos_quick_race",
        .availableDuringPause = true,
        .lngKey = LNG_EXPANDED_CONFIG_CHAOS_TEST,
        .type = CFG_BOOL,
        .valuePtr = &g_config.chaosQuickRace,
    },
    {
        .section = CONFIG_SECTION_DEVELOPER_HACKS,
        .key = "skip_start_countdown",
        .availableDuringPause = true,
        .lngKey = LNG_EXPANDED_CONFIG_SKIP_COUNTDOWN,
        .type = CFG_BOOL,
        .valuePtr = &g_config.skipStartCountdown,
    },
    {
        .section = CONFIG_SECTION_DEVELOPER_HACKS,
        .key = "end_race_with_l3",
        .availableDuringPause = true,
        .lngKey = LNG_EXPANDED_CONFIG_END_RACE,
        .type = CFG_BOOL,
        .valuePtr = &g_config.endRaceWithL3,
    },
};

const int g_numConfigEntries = sizeof(g_configEntries) / sizeof(g_configEntries[0]);

// Snapshot of the pristine g_config, copied at the top of NativeConfig_Load
// before anything mutates it.
static NativeConfig s_defaults;

// Developer hacks are an umbrella switch: while developerHacksEnabled is off,
// every other option in the section is restored to its default so a hidden
// option can never silently alter gameplay.
static void NativeConfig_ResetDisabledDeveloperHacks(void)
{
    if (g_config.developerHacksEnabled)
    {
        return;
    }

    for (int i = 0; i < g_numConfigEntries; i++)
    {
        const ConfigEntry *e = &g_configEntries[i];

        if (strcmp(e->section, CONFIG_SECTION_DEVELOPER_HACKS) != 0)
        {
            continue;
        }

        // The gate itself is never reset.
        if (e->valuePtr == &g_config.developerHacksEnabled)
        {
            continue;
        }

        // Menu markers have no value to restore.
        if (e->type == CFG_ACTION || e->type == CFG_SUBSECTION)
        {
            continue;
        }

        // valuePtr points into g_config, so the same offset selects the
        // matching field of the snapshot.
        const size_t offset = (size_t)((const char *)e->valuePtr - (const char *)&g_config);

        if (e->type == CFG_BOOL)
        {
            *(bool *)e->valuePtr = *(const bool *)((const char *)&s_defaults + offset);
        }
        else
        {
            *(int *)e->valuePtr = *(const int *)((const char *)&s_defaults + offset);
        }
    }
}

void NativeConfig_ApplyDependencies(const ConfigEntry *changedEntry)
{
    if (changedEntry == NULL)
    {
        return;
    }

    if (changedEntry->valuePtr == &g_config.developerHacksEnabled)
    {
        NativeConfig_ResetDisabledDeveloperHacks();
    }
}

static bool ParseBool(const char *s)
{
    return strcmp(s, "true") == 0 || strcmp(s, "1") == 0;
}

static char *trimWhitespace(char *s)
{
    while (isspace((unsigned char)*s))
        s++;
    if (*s == '\0')
        return s;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end))
        end--;
    *(end + 1) = '\0';
    return s;
}

void NativeConfig_Load(void)
{
    // Must happen before the parse below (and before the config menu can edit
    // anything) so the snapshot still holds the pristine defaults.
    s_defaults = g_config;

    printf("[Config] Base:       %s\n", NativeAssets_GetBaseDir());
    char path[512];
    NativePath_Join(path, sizeof(path), NativeStr8_FromCString(NativeAssets_GetBaseDir()), NATIVE_STR8_LIT("config.ini"));
    FILE *f = fopen(path, "r");
    if (!f) {
        printf("[Config] config.ini NOT FOUND (%s)\n", path);
        return;
    }

    printf("[Config] config.ini opened OK\n");

    char line[256];
    char section[64] = "";
    bool hasExtraOptions = false;
    bool legacyExtraOptions = false;

    while (fgets(line, sizeof(line), f))
    {
        char *p = trimWhitespace(line);

        if (*p == '\0' || *p == ';' || *p == '#')
            continue;

        if (*p == '[')
        {
            char *end = strchr(p + 1, ']');
            if (end)
            {
                *end = '\0';
                strncpy(section, p + 1, sizeof(section) - 1);
                section[sizeof(section) - 1] = '\0';
            }
            printf("[Config] Section: [%s]\n", section);
            continue;
        }

        char *eq = strchr(p, '=');
        if (!eq)
            continue;

        *eq = '\0';
        char *key = trimWhitespace(p);
        char *value = trimWhitespace(eq + 1);

        if (strcmp(section, "Adventure") == 0)
        {
            if (strcmp(key, "expanded_options") == 0)
            {
                hasExtraOptions = true;
            }
            else if (strcmp(key, "extra_options") == 0 || strcmp(key, "extended_character_select") == 0 ||
                     strcmp(key, "multiplayer_adventure") == 0 || strcmp(key, "chaos_mode") == 0)
            {
                legacyExtraOptions |= ParseBool(value);
            }
        }

        for (int i = 0; i < g_numConfigEntries; i++)
        {
            const ConfigEntry *e = &g_configEntries[i];
            if (strcmp(section, e->section) == 0 &&
                strcmp(key, e->key) == 0)
            {
                if (e->type == CFG_ACTION || e->type == CFG_SUBSECTION)
                {
                    // Nothing to parse; the section is drawn by its own screen.
                }
                else if (e->type == CFG_BOOL)
                    *(bool *)e->valuePtr = ParseBool(value);
                else if (e->type == CFG_ENUM)
                {
                    int matched = 0;
                    for (int j = 0; j < e->numEnumValues; j++)
                    {
                        if (strcmp(value, e->enumValues[j].name) == 0)
                        {
                            *(int *)e->valuePtr = e->enumValues[j].value;
                            matched = 1;
                            break;
                        }
                    }
                    if (!matched)
                        *(int *)e->valuePtr = atoi(value);
                }
                else
                    *(int *)e->valuePtr = atoi(value);
                printf("[Config] %s/%s = %s\n", e->section, e->key, value);
                break;
            }
        }
    }

    fclose(f);

    if (!hasExtraOptions)
    {
        g_config.expandedAdventureOptions = legacyExtraOptions;
    }

    // Applied after the whole file is parsed so key order in config.ini does
    // not matter: developer hack options only take effect while the section is
    // enabled.
    NativeConfig_ResetDisabledDeveloperHacks();
}

void NativeConfig_Save(void)
{
    char path[512];
    NativePath_Join(path, sizeof(path), NativeStr8_FromCString(NativeAssets_GetBaseDir()), NATIVE_STR8_LIT("config.ini"));
    FILE *f = fopen(path, "w");
    if (!f)
        return;

    const char *lastSection = NULL;

    for (int i = 0; i < g_numConfigEntries; i++)
    {
        const ConfigEntry *e = &g_configEntries[i];

        // Menu-only rows have no config.ini representation.
        if (e->type == CFG_ACTION || e->type == CFG_SUBSECTION)
            continue;

        if (lastSection == NULL || strcmp(e->section, lastSection) != 0)
        {
            if (lastSection != NULL)
                fprintf(f, "\n");
            fprintf(f, "[%s]\n", e->section);
            lastSection = e->section;
        }

        if (e->type == CFG_BOOL)
            fprintf(f, "%s = %s\n", e->key, *(bool *)e->valuePtr ? "true" : "false");
        else if (e->type == CFG_ENUM)
        {
            int val = *(int *)e->valuePtr;
            const char *name = "?";
            for (int j = 0; j < e->numEnumValues; j++)
            {
                if (e->enumValues[j].value == val)
                {
                    name = e->enumValues[j].name;
                    break;
                }
            }
            fprintf(f, "%s = %s\n", e->key, name);
        }
        else
            fprintf(f, "%s = %d\n", e->key, *(int *)e->valuePtr);
    }

    fclose(f);
}
