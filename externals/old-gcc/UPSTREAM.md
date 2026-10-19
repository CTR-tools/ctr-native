# Upstream provenance

## Build recipes

- Project: <https://github.com/decompals/old-gcc>
- Tag: `0.17`
- Commit: `b74211c9d959e9724802f3177c8229cd67202c87`

The upstream repository snapshot is retained without its `.git` directory.
The matching pipeline uses only GCC 2.8.1.

## GCC source

- Source: <https://ftp.gnu.org/gnu/gcc/gcc-2.8.1.tar.gz>
- SHA-256:
  `3b30fbfdf93e628373d90d174243f3267b0eec9ebe792bb64fd15b8828c2ea4c`
- Local archive: `gcc-2.8.1.tar.gz`
- License: GNU GPL v2 (`GCC-COPYING`)

The `gcc-2.8.1-psx.Dockerfile` and `patches/` directory retain the upstream PSX
configuration and host compatibility changes, then apply the local
`psx-gte-delay-policies-2.8.1.patch`. It models the real, nonallocatable GTE
SXY2/VXY0 registers and validates transfer delay slots. Two additional policies
are disabled unless selected for a compilation:

- `-mpsx-normal-ram-lhu-delay=on`: permit a certified adjacent volatile LHU in
  a branch delay slot. Requires normal aligned mapped RAM, not MMIO; precise
  exception context and interrupt observation are outside this contract.
- `-mpsx-local-bgez=on`: emit BGEZ-zero for ABI32 local jumps. This is a short
  branch policy, not a general long-jump implementation. The function-section
  compiler/assembler/linker route must reject out-of-range targets.

Both options accept only `on` or `off`, default to `off`, and use the last
supplied value. The game selects them only for `game/DrawSky.c` through its
per-source build flags. No function names or retail addresses select behavior
inside the compiler. The patch SHA-256 is
`98a97146ddf2c011883781f617d48d3b2fa92d85455fc515fccd6d73dfa79a2c`.

## Tested compiler release

- Release:
  <https://github.com/decompals/old-gcc/releases/download/0.17/gcc-2.8.1-psx.tar.gz>
- Release archive SHA-256:
  `f6f6e883942d4d3289d048236c672e71ed410e546aaae8ff655952f1567e1be0`
- Platform: statically linked Linux i386
- Local directory: `prebuilt/linux-i386/2.8.1`

The C compiler pass (`cc1`) is rebuilt with the local patch. Two clean builds
in the same fixed Ubuntu 20.04/GCC 9.4.0 environment produced the hash below.
The other four files remain unchanged from the release archive. The Dockerfile
builds only the replacement `cc1` and exports it alongside those stock files.
Its pinned base image and source archive do not freeze future apt repositories;
the executable hashes, not a fresh package-manager run, identify the tested tuple.

```text
2258e9e4c4feefd7478ca2f665e9792dd2e4998ba6b3987a8bfe2690644f2529  cc1
93e7b36546532deec58cbc221df4d2c2894d444b5685b31a439c46cc597a9572  cc1plus
8d7f4e90f995089a6c2d62214b607fc77acf43b61f60a871d130d713d9ac96bb  cpp
a2cf0a5f0a21fcd4836c6f7ac66f346ba5594c1e4d4ab9d88c48c73c77f876fd  g++
451e6521d860fc6ff49000567d94d983125263bb0ae2149f89d95eaed34b3a02  gcc
```
