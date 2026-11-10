# tomlc17

Vendored from https://github.com/cktan/tomlc17, release R261003,
commit `8d3766ddafa6515417f4885ae6878ef69fac401e`. The source and header
are unmodified; see `LICENSE` for the MIT license. Upstream describes this
release as a TOML 1.1 parser.

The parser is compiled as a separate C17 static library, not included into the
game's unity translation unit. A caller must call `toml_free` for each parse
result, including errors; tree values and strings belong to that result and
must be copied before it is freed. UTF-8 checking is disabled by default:
catalog loading must enable `check_utf8` using the global parser options before
parsing. Embedded NULs have explicit lengths in the API and must be rejected
for game strings, which use NUL termination.
