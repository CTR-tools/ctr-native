# CTR Data Section Architecture

CTR's main executable keeps several resident data regions in fixed PS1 RAM
addresses.

## Main EXE Regions

The NTSC-U retail executable is represented in `include/regionsEXE.h` as typed
structs over fixed address ranges.

| Section | Address Range | Symbol | Purpose |
|---------|---------------|--------|---------|
| `.rdata` | `0x80010000`-`0x800123df` | `rdata` | Read-only compiler data: strings, jump tables, format strings, lookup constants |
| `.text` | `0x800123e0`-`0x8008099f` | function symbols | Resident executable code |
| `.data` | `0x800809a0`-`0x8008cf6b` | `data` | Initialized global objects and metadata |
| `.sdata` | `0x8008cf6c`-`0x8008d667` | `sdata_static` | Small data globals addressed through `$gp` |
| `.bss` | `0x8008d668`-`0x8009f6fc` | `bss` | Zero-initialized runtime storage |

The named structs are layout maps. Field offsets matter because code often
depends on retail addresses, not just on C names.

## `.rdata`

`.rdata` contains constant data emitted by the original compiler. It includes
model lookup names like `"crash"` and `"oxide"`, file path strings, UI format
strings, switch jump tables, and other constants referenced by resident code.


## `.data`

`.data` contains initialized resident globals, exposed as `struct Data data`.
This is where many metadata tables live, including character metadata, level
metadata, menus, rendering tables, and pointers into other resident regions.

Pointers in `.data` may legally point into `.rdata`. For example,
`MetaDataCharacters[].name_Debug` points at character model-name strings in
`rdata`.


## `.sdata`

`.sdata` contains small globals normally reached through the MIPS `$gp`
register.

```c
register struct sData *sdata asm("$gp");
```

We do something similar to maintain the host-owned mirror:

```c
struct sData *sdata = &sdata_static;
```
