---
name: runtime-configuration
description: Add or modify user-facing options in the table-driven INI configuration and in-game Options Config menu.
---

# Runtime configuration

Use this skill for any adjustable gameplay, display, or quality-of-life option.

1. Add a field to `NativeConfig` in `include/platform/native_config.h`; initialize it in `g_config` and add a matching `ConfigEntry` in the appropriate contiguous section of `g_configEntries` in `platform/native_config.c`.
2. Pick `CFG_BOOL`, `CFG_INT` (with min/max/step), or `CFG_ENUM` (with a `ConfigEnumValue` table). `game/230/MM_ConfigMenu.c` builds a section map from adjacent entries and renders/edits the entries. Check menu assumptions when adding a section or an enum with non-contiguous values.
3. Apply the option in the relevant game/platform code by reading `g_config`; keep the default vanilla unless the request explicitly specifies otherwise. Preserve established defaults for existing options (for example, the current `renderScale` default is Native).
4. Add the setting to the corresponding section of `resources/config/default_config.ini` and the matching table in `README.md`. `main.c` loads config before platform initialization; `NativeConfig_Load`/`Save` use `config.ini` at the executable base directory. CMake copies the template into a build directory only if no config file exists, so an existing `build/config.ini` will not automatically pick up template changes.
5. Verify persistence, in-game editing, and effect at runtime for options whose behavior cannot be checked by inspection alone. Preserve the user's local `config.ini`.
