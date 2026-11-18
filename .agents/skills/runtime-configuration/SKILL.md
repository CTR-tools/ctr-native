---
name: runtime-configuration
description: Add or modify user-facing options in the table-driven INI configuration and in-game Options Config menu.
---

# Runtime configuration

Use this skill for any adjustable gameplay, display, or quality-of-life option.

1. Add a field to `NativeConfig` in `include/platform/config/native_config.h`; initialize it in `platform/config/native_config_defaults.inc` and add a matching `ConfigEntry` in the appropriate contiguous section of `g_configEntries` in `platform/config/native_config.c`.
2. Pick `CFG_BOOL`, `CFG_INT` (with min/max/step), or `CFG_ENUM` (with a `ConfigEnumValue` table). `game/230/config/MM_ConfigMenu.c` builds a section map from adjacent entries and renders/edits the entries. Check menu assumptions when adding a section or an enum with non-contiguous values.
3. Apply the option in the relevant game/platform code by reading `g_config`; keep the default vanilla unless the request explicitly specifies otherwise. Preserve established defaults for existing options (for example, the current `renderScale` default is Native).
4. Add the setting to the matching table in `README.md` and decide whether each shipped preset should set it. Presets are partial, symmetric where switching profiles should restore a setting, and maintained in `resources/presets/`; do not assume Expanded enables every option. Check runtime effects before including options that initialize early.
5. `main.c` loads configuration before platform initialization: built-in defaults, selected preset, explicit `config.ini` values, then dependencies. `platform/config/native_profile.c` manages `profile.ini` and preset selection; `game/230/config/MM_ProfileMenu.c` owns the profile UI. Config and profile files live at the asset base, which may be a parent of the executable; shipped presets are under `resources/presets/` beside the executable.
6. Verify persistence, preset layering and application, in-game editing, and runtime effects where applicable. Preserve the user's local `config.ini` and `profile.ini`.
