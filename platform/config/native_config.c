#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <errno.h>
#include <limits.h>
#include <platform/config/native_config.h>
#include <platform/native_assets.h>
#include <platform/native_path.h>
#include <platform/text/native_language_disc.h>
#include <platform/text/native_text_runtime.h>
#include <text/namespace_Lng.h>

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
static const ConfigEnumValue kLoadingScreenValues[] = {
    {"Vanilla", LOADING_SCREEN_VANILLA, LNG_EXPANDED_CONFIG_VANILLA},
    {"No text", LOADING_SCREEN_NO_TEXT, LNG_EXPANDED_CONFIG_NO_TEXT},
    {"Fade", LOADING_SCREEN_FADE, LNG_EXPANDED_CONFIG_FADE},
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
  .loadingScreen = LOADING_SCREEN_VANILLA,
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
  .engineSelection = false,
  .customCups = false,
  .expandedArcade = false,

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
  .unlockCupDifficulties = false,
  .unlockTurboTrack = false,
  .unlockAllBattleArenas = false,
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
        .key = "loading_screen",
        .availableDuringPause = true,
        .lngKey = LNG_EXPANDED_CONFIG_LOADING_SCREEN,
        .type = CFG_ENUM,
        .valuePtr = &g_config.loadingScreen,
        .enumValues = kLoadingScreenValues,
        .numEnumValues = sizeof(kLoadingScreenValues) / sizeof(kLoadingScreenValues[0]),
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
        .section = "General",
        .key = "engine_selection",
        .lngKey = LNG_EXPANDED_CONFIG_ENGINE_SELECTION,
        .type = CFG_BOOL,
        .valuePtr = &g_config.engineSelection
    },
    {
        .section = "General",
        .key = "custom_cups",
        .lngKey = LNG_EXPANDED_CONFIG_CUSTOM_CUPS,
        .type = CFG_BOOL,
        .valuePtr = &g_config.customCups
    },
    {
        .section = "General",
        .key = "expanded_arcade",
        .lngKey = LNG_EXPANDED_CONFIG_EXPANDED_ARCADE,
        .type = CFG_BOOL,
        .valuePtr = &g_config.expandedArcade
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
      .key = "tnt_explosion_radius_multiplier",
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
        .section = "Unlocks",
        .key = "unlock_cup_difficulties",
        .availableDuringPause = true,
        .lngKey = LNG_EXPANDED_CONFIG_UNLOCK_CUP_DIFFICULTIES,
        .type = CFG_BOOL,
        .valuePtr = &g_config.unlockCupDifficulties
    },
    {
        .section = "Unlocks",
        .key = "unlock_turbo_track",
        .availableDuringPause = true,
        .lngKey = LNG_EXPANDED_CONFIG_UNLOCK_TURBO_TRACK,
        .type = CFG_BOOL,
        .valuePtr = &g_config.unlockTurboTrack
    },
    {
        .section = "Unlocks",
        .key = "unlock_all_battle_arenas",
        .availableDuringPause = true,
        .lngKey = LNG_EXPANDED_CONFIG_UNLOCK_BATTLE_ARENAS,
        .type = CFG_BOOL,
        .valuePtr = &g_config.unlockAllBattleArenas
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

typedef enum
{
    CONFIG_PARSE_COMPATIBLE,
    CONFIG_PARSE_STRICT,
} ConfigParseMode;

static bool NativeConfig_ParseBool(const char *value, bool *destination, ConfigParseMode mode)
{
    if (strcmp(value, "true") == 0 || strcmp(value, "1") == 0)
    {
        *destination = true;
        return true;
    }
    if (mode == CONFIG_PARSE_COMPATIBLE || strcmp(value, "false") == 0 || strcmp(value, "0") == 0)
    {
        *destination = false;
        return true;
    }
    return false;
}

static bool NativeConfig_ParseNumber(const char *value, int *destination)
{
    errno = 0;
    char *end;
    long number = strtol(value, &end, 10);
    if (end == value || *end != '\0' || errno == ERANGE || number < INT_MIN || number > INT_MAX)
    {
        return false;
    }
    *destination = (int)number;
    return true;
}

static bool NativeConfig_ParseInt(const ConfigEntry *entry, const char *value, int *destination, ConfigParseMode mode)
{
    if (mode == CONFIG_PARSE_COMPATIBLE)
    {
        *destination = atoi(value);
        return true;
    }
    int number;
    if (!NativeConfig_ParseNumber(value, &number) || number < entry->min || number > entry->max)
    {
        return false;
    }
    *destination = number;
    return true;
}

static bool NativeConfig_ParseEnum(const ConfigEntry *entry, const char *value, int *destination, ConfigParseMode mode)
{
    if (entry->valuePtr == &g_config.textLanguage)
    {
        int slot = NativeTextRuntime_LanguageSlot(value);
        if (slot >= 0)
        {
            *destination = slot;
            return true;
        }
    }
    for (int i = 0; i < entry->numEnumValues; i++)
    {
        if (strcmp(value, entry->enumValues[i].name) == 0)
        {
            *destination = entry->enumValues[i].value;
            return true;
        }
    }

    if (mode == CONFIG_PARSE_COMPATIBLE)
    {
        if (entry->valuePtr == &g_config.textLanguage)
        {
            char *end;
            long slot = strtol(value, &end, 10);
            *destination = end != value && *end == '\0' &&
                           slot >= 0 && slot < NATIVE_TEXT_CUSTOM_FIRST &&
                           NativeTextRuntime_LanguageId((int)slot) != NULL
                               ? (int)slot : NATIVE_LANGUAGE_ENGLISH;
        }
        else
        {
            *destination = atoi(value);
        }
        return true;
    }

    int number;
    if (!NativeConfig_ParseNumber(value, &number))
    {
        return false;
    }
    for (int i = 0; i < entry->numEnumValues; i++)
    {
        if (number == entry->enumValues[i].value)
        {
            *destination = number;
            return true;
        }
    }
    return false;
}

static bool NativeConfig_ParseValue(const ConfigEntry *entry, const char *value, void *destination, ConfigParseMode mode)
{
    switch (entry->type)
    {
        case CFG_BOOL:
            return NativeConfig_ParseBool(value, (bool *)destination, mode);
        case CFG_INT:
            return NativeConfig_ParseInt(entry, value, (int *)destination, mode);
        case CFG_ENUM:
            return NativeConfig_ParseEnum(entry, value, (int *)destination, mode);
        case CFG_ACTION:
        case CFG_SUBSECTION:
        default:
            return false;
    }
}

typedef void *(*ConfigDestination)(const ConfigEntry *entry, void *context);

static void *NativeConfig_ConfigDestination(const ConfigEntry *entry, void *context)
{
    size_t offset = (size_t)((const char *)entry->valuePtr - (const char *)&g_config);
    return (char *)context + offset;
}

static void *NativeConfig_ProfileDestination(const ConfigEntry *entry, void *context)
{
    (void)entry;
    return context;
}

static ConfigLoadResult NativeConfig_ReadFile(const char *path, void *context, ConfigDestination destination,
                                               const ConfigEntry *entries, int entryCount, ConfigParseMode mode)
{
    FILE *file = fopen(path, "r");
    if (file == NULL)
    {
        return CONFIG_LOAD_FILE_ERROR;
    }

    ConfigLoadResult result = CONFIG_LOAD_OK;
    char line[256];
    char section[64] = "";
    while (fgets(line, sizeof(line), file) != NULL)
    {
        if (mode == CONFIG_PARSE_STRICT && strchr(line, '\n') == NULL && !feof(file))
        {
            result = CONFIG_LOAD_INVALID;
            break;
        }
        char *p = trimWhitespace(line);
        if (*p == '\0' || *p == ';' || *p == '#')
        {
            continue;
        }
        if (*p == '[')
        {
            char *end = strchr(p + 1, ']');
            if (mode == CONFIG_PARSE_STRICT && (end == NULL || *trimWhitespace(end + 1) != '\0' || end == p + 1 || (size_t)(end - p - 1) >= sizeof(section)))
            {
                result = CONFIG_LOAD_INVALID;
                break;
            }
            if (end != NULL)
            {
                *end = '\0';
                strncpy(section, p + 1, sizeof(section) - 1);
                section[sizeof(section) - 1] = '\0';
            }
            continue;
        }
        char *eq = strchr(p, '=');
        if (eq == NULL)
        {
            if (mode == CONFIG_PARSE_STRICT)
            {
                result = CONFIG_LOAD_INVALID;
                break;
            }
            continue;
        }
        *eq = '\0';
        char *key = trimWhitespace(p);
        char *value = trimWhitespace(eq + 1);
        const ConfigEntry *entry = NULL;
        for (int i = 0; i < entryCount; i++)
        {
            if (strcmp(section, entries[i].section) == 0 && strcmp(key, entries[i].key) == 0)
            {
                entry = &entries[i];
                break;
            }
        }
        if (entry == NULL || entry->type == CFG_ACTION || entry->type == CFG_SUBSECTION)
        {
            if (mode == CONFIG_PARSE_STRICT)
            {
                result = CONFIG_LOAD_INVALID;
                break;
            }
            continue;
        }
        if (!NativeConfig_ParseValue(entry, value, destination(entry, context), mode))
        {
            result = CONFIG_LOAD_INVALID;
            break;
        }
    }
    if (ferror(file))
    {
        result = CONFIG_LOAD_FILE_ERROR;
    }
    if (fclose(file) != 0)
    {
        result = CONFIG_LOAD_FILE_ERROR;
    }
    return result;
}

ConfigLoadResult NativeConfig_ApplyPresetFile(const char *path)
{
    NativeConfig candidate = g_config;
    ConfigLoadResult result = NativeConfig_ReadFile(path, &candidate, NativeConfig_ConfigDestination,
                                                     g_configEntries, g_numConfigEntries, CONFIG_PARSE_STRICT);
    if (result == CONFIG_LOAD_OK)
    {
        g_config = candidate;
        NativeConfig_ResetDisabledDeveloperHacks();
    }
    return result;
}

static const ConfigEnumValue kProfileValues[] = {
    {"Vanilla", CONFIG_PROFILE_VANILLA, 0},
    {"Expanded", CONFIG_PROFILE_EXPANDED, 0},
};
static const ConfigEntry kProfileEntry = {
    .section = "Profile",
    .key = "active",
    .type = CFG_ENUM,
    .enumValues = kProfileValues,
    .numEnumValues = sizeof(kProfileValues) / sizeof(kProfileValues[0]),
};

ConfigLoadResult NativeConfig_LoadProfile(ConfigProfile *profile)
{
    if (profile == NULL)
    {
        return CONFIG_LOAD_INVALID;
    }
    char path[512];
    NativePath_Join(path, sizeof(path), NativeStr8_FromCString(NativeAssets_GetBaseDir()), NATIVE_STR8_LIT("profile.ini"));
    int value = -1;
    ConfigLoadResult result = NativeConfig_ReadFile(path, &value, NativeConfig_ProfileDestination,
                                                     &kProfileEntry, 1, CONFIG_PARSE_STRICT);
    if (result != CONFIG_LOAD_OK)
    {
        return result;
    }
    if (value != CONFIG_PROFILE_VANILLA && value != CONFIG_PROFILE_EXPANDED)
    {
        return CONFIG_LOAD_INVALID;
    }
    *profile = (ConfigProfile)value;
    return CONFIG_LOAD_OK;
}

static const char *NativeConfig_EnumName(const ConfigEntry *entry, int value)
{
    for (int i = 0; i < entry->numEnumValues; i++)
    {
        if (entry->enumValues[i].value == value)
        {
            return entry->enumValues[i].name;
        }
    }
    if (entry->valuePtr == &g_config.textLanguage && value >= NATIVE_TEXT_CUSTOM_FIRST)
    {
        return NativeTextRuntime_LanguageId(value);
    }
    return NULL;
}

static bool NativeConfig_WriteValue(FILE *file, const ConfigEntry *entry, const void *value)
{
    switch (entry->type)
    {
        case CFG_BOOL:
            return fprintf(file, "%s = %s\n", entry->key, *(const bool *)value ? "true" : "false") >= 0;
        case CFG_INT:
            return fprintf(file, "%s = %d\n", entry->key, *(const int *)value) >= 0;
        case CFG_ENUM:
        {
            const char *name = NativeConfig_EnumName(entry, *(const int *)value);
            return fprintf(file, "%s = %s\n", entry->key, name != NULL ? name : "?") >= 0;
        }
        case CFG_ACTION:
        case CFG_SUBSECTION:
        default:
            return false;
    }
}

bool NativeConfig_SaveProfile(ConfigProfile profile)
{
    int value = (int)profile;
    if (NativeConfig_EnumName(&kProfileEntry, value) == NULL)
    {
        return false;
    }
    char path[512];
    NativePath_Join(path, sizeof(path), NativeStr8_FromCString(NativeAssets_GetBaseDir()), NATIVE_STR8_LIT("profile.ini"));
    FILE *file = fopen(path, "w");
    if (file == NULL)
    {
        return false;
    }
    bool written = fprintf(file, "[%s]\n", kProfileEntry.section) >= 0;
    if (!NativeConfig_WriteValue(file, &kProfileEntry, &value))
    {
        written = false;
    }
    return fclose(file) == 0 && written;
}

void NativeConfig_Load(void)
{
    // Must happen before the parse below (and before the config menu can edit
    // anything) so the snapshot still holds the pristine defaults.
    s_defaults = g_config;

    printf("[Config] Base:       %s\n", NativeAssets_GetBaseDir());
    char path[512];
    NativePath_Join(path, sizeof(path), NativeStr8_FromCString(NativeAssets_GetBaseDir()), NATIVE_STR8_LIT("config.ini"));
    if (NativeConfig_ReadFile(path, &g_config, NativeConfig_ConfigDestination, g_configEntries, g_numConfigEntries,
                              CONFIG_PARSE_COMPATIBLE) != CONFIG_LOAD_OK)
    {
        printf("[Config] Could not read config.ini (%s)\n", path);
        return;
    }

    printf("[Config] config.ini opened OK\n");

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

        NativeConfig_WriteValue(f, e, e->valuePtr);
    }

    fclose(f);
}
