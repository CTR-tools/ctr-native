#ifndef NATIVE_CONFIG_H
#define NATIVE_CONFIG_H

#include <stdbool.h>

typedef struct {
    const char *name;
    int value;
} ConfigEnumValue;

typedef struct {
    bool skipIntro;             // false = off (default), true = skip all intros
    bool showReservesMeter;     // true = show the in-game reserves meter (default = false)
    bool allowOxideStationMultiplayer; // false = retail 1P-only restriction, true = allow multiplayer
    bool allowRaceTracksInBattle; // false = battle arenas only, true = select race tracks in battle mode
    bool extendedMultiplayer; // false = retail rosters, true = allow 3P/4P Arcade races and fill 2P/3P/4P Arcade and Adventure races with 8 racers
    int botSelectionMode;         // BOT_SELECTION_PREDETERMINED (default), RANDOM_UNLOCKED, or RANDOM_ALL
    bool skipHints;             // false = off (default), true = skip all mask hints in adventure mode
    bool extendedAdventureCharacterSelect; // false = garage, true = extended selector for new Adventure
    bool multiplayerAdventure;  // false = off (default), true = enable multiplayer Adventure setup
    int speedMultiplier;        // percent, 10%..200%, 100 = 1.0x (default)
    int gravityMultiplier;      // percent, 10%..300%, 100 = 1.0x (default)
    int turnMultiplier;         // percent, 10%..400%, 100 = 1.0x (default)
    int jumpMultiplier;         // percent, 10%..300%, 100 = 1.0x (default)
    int reserveMultiplier;      // percent, 0%..400%,  100 = 1.0x (default)
    bool unlockAllCharacters;   // false = off (normal unlock logic), true = all characters already unlocked
    bool unlockNitrosOxide;     // false = off, true = Nitros Oxide is selectable
    bool unlockAllGates;        // false = off, true = all adventure mode wood doors open without keys
    bool unlockAllPortals;      // false = off, true = all warp pads and boss garages unlocked
    bool increaseDrawDistance;  // false = off (default), true = render NATIVE_DRAW_DISTANCE_SCALE x farther (platform/native_draw_distance.h)
    bool disableSplitScreenLod; // false = off (default), true = use full-detail split-screen assets and effects
    bool fullscreen;            // false = windowed, true = borderless fullscreen
    int aspectRatio;            // 0 = 4:3 (default), 1 = 16:9, 2 = 16:10, 3 = 21:10
    bool dithering;             // true = dithering enabled (default), false = disabled
    int renderScale;            // 1 = original, 2/3/4 = fixed multiples, 0 = native window
    bool smoothScaling;         // true = linear filtering for direct presentation
    bool fxaa;                  // false = off (default), true = FXAA edge antialiasing on the presented frame
    bool textureFiltering;      // true = bilinear PSX texture sampling
    int pgxpMode;               // PGXP_MODE_OFF (default), GEOMETRY, or PERSPECTIVE
    bool saveAnywhere;          // false = off (default), true = save anywhere in adventure mode by pressing select or from the pause menu
    int missileSpeedMultiplier; // percent, 20%..500%, 100 = 1.0x (default)
    int bombSpeedMultiplier;    // percent, 20%..300%, 100 = 1.0x (default)
    int warpballSpeedMultiplier; // percent, 20%..300%, 100 = 1.0x (default)
    int bombExplosionRadiusMultiplier; // percent, 50%..600%, 100 = 1.0x (default)
    int tntExplosionRadiusMultiplier;  // percent, 50%..600%, 100 = 1.0x (default)
    int maskMode;               // MASK_MODE_NORMAL=0(default), RANDOM=1, INVERTED=2, ALL_UKA=3, ALL_AKU=4
    bool maskProtectsFromDamage; // true (default) = immune to damage while mask is active, false = can take damage through mask
    bool maskDamagesOthers;      // true (default) = mask damages drivers on contact, false = no damage
    bool maskPersistsAfterOOB;   // false (default) = mask is consumed on OOB, true = mask continues after OOB
    int maskDurationMultiplier;  // percent, 20%..250%, 100 = 1.0x (default)
    int maskExtraSpeedMultiplier; // percent, 0%..300%, 100 = 1.0x (default)
    int clockDurationMultiplier;  // percent, 20%..250%, 100 = 1.0x (default)
    bool allowWeaponsDuringClock; // false (default) = clock blocks weapon usage, true = weapons work during clock
    bool autoEndRaceWhenOthersFinish; // false = off (vanilla behavior), true = end race when all other racers have finished
    bool developerHacksEnabled; // false = off (default), true = enable developer hacks menu/options
    bool omniController; // false = off (default), true = mirror port 1 input to all controller slots
    bool oneLapRaces; // false = off (default), true = force every race to a single lap, whatever the mode or menu allows
    bool towPlayer2; // false = off (default), true = hold the Adventure co-op partner kart behind the lead racer; select toggles the tow
    bool quickRace; // false = off (default), true = R3 in the main menu starts a 1P arcade race in Crash Cove as Crash
    bool skipStartCountdown; // false = off (default), true = skip the traffic light countdown and the start-line fly-in
    bool endRaceWithL3; // false = off (default), true = L3 in-race ends the race on the placings the field is standing on
} NativeConfig;

enum
{
    MASK_MODE_NORMAL = 0,
    MASK_MODE_RANDOM = 1,
    MASK_MODE_INVERTED = 2,
    MASK_MODE_ALL_UKA = 3,
    MASK_MODE_ALL_AKU = 4,
};

enum
{
    BOT_SELECTION_PREDETERMINED = 0,
    BOT_SELECTION_RANDOM_UNLOCKED = 1,
    BOT_SELECTION_RANDOM_ALL = 2,
};

// Sub-pixel vertex precision, implemented in platform/native_pgxp.c. The PS1
// GTE truncates every RTPS/RTPT projection to an integer SXY before the CPU
// stores it into a GPU packet; keeping the fractional position and view depth
// next to that integer removes the polygon wobble as the camera moves.
enum
{
    PGXP_MODE_OFF = 0,
    // Sub-pixel vertex positions, retail affine texture mapping.
    PGXP_MODE_GEOMETRY = 1,
    // Sub-pixel vertex positions and perspective-correct texture mapping.
    PGXP_MODE_PERSPECTIVE = 2,
};

typedef enum { CFG_BOOL, CFG_INT, CFG_ENUM, CFG_ACTION } ConfigType;

#define CONFIG_SECTION_DEVELOPER_HACKS "Developer Hacks"
#define CONFIG_SECTION_CONTROLS      "Controls"

typedef struct {
    const char *section;
    const char *key;
    const char *label;
    ConfigType type;
    void *valuePtr;             // points into g_config (e.g. &g_config.skipIntro)
    int min, max, step;         // slider bounds, ignored for CFG_BOOL/CFG_ENUM
    const ConfigEnumValue *enumValues;  // value-name pairs for CFG_ENUM
    int numEnumValues;                  // number of entries in enumValues
} ConfigEntry;

// CFG_ACTION has no g_config backing and no file representation: it exists only to
// give a section a row in the options menu, and the section's own screen draws
// and edits everything under it. NativeConfig_Load and NativeConfig_Save skip it,
// so it never reaches config.ini.

extern NativeConfig g_config;
extern const ConfigEntry g_configEntries[];
extern const int g_numConfigEntries;

void NativeConfig_Load(void);
void NativeConfig_Save(void);
void NativeConfig_ApplyDependencies(const ConfigEntry *changedEntry);

#endif
