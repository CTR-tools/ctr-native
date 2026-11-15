# Shipped translation length checks

`test_text_lengths` checks the retail and Expanded TOML catalogs under
`resources/text/`. It does not read personal overrides or text from disc images.
Catalogs may be partial; only entries present in a catalog are checked.

Limits are declared next to IDs in `include/text/retail_text_keys.h` and
`include/text/expanded_text_keys.h`. Unannotated entries default to 30 characters. Use
`RETAIL_TEXT_KEY_LIMIT(name, id, max)` or `EXPANDED_TEXT_KEY_LIMIT(name, max)`
for a specific limit. Reusable categories such as `RETAIL_TEXT_KEY_MENU_ENTRY`
and `EXPANDED_TEXT_KEY_MENU_ENTRY` use the constants in `include/text/text_limits.h`.
The initial menu-entry limit and annotated examples are provisional; choose
additional limits based on the actual screens.
All limits must be positive; zero cannot disable the check.

The test counts Unicode code points in the complete TOML-decoded value,
including line breaks and literal placeholders. It does not measure rendered
width or substitute placeholder values, so passing does not guarantee that
text fits in the game. Run it with:

```text
ctest --test-dir build --output-on-failure -R ^test_text_lengths$
```
