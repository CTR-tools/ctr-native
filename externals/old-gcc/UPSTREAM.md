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
SXY2/VXY0 registers and validates transfer delay slots. Three additional policies
are disabled unless selected for a compilation:

- `-mpsx-normal-ram-lhu-delay=on`: permit a certified adjacent volatile LHU in
  a branch delay slot. Requires normal aligned mapped RAM, not MMIO; precise
  exception context and interrupt observation are outside this contract.
- `-mpsx-local-bgez=on`: emit BGEZ-zero for ABI32 local jumps. This is a short
  branch policy, not a general long-jump implementation. The function-section
  compiler/assembler/linker route must reject out-of-range targets.
- `-mpsx-ctc2-return-delay=on`: permit a typed CTC2 immediately preceding a
  null-epilogue return in that return's delay slot. Requires enabled,
  nonfaulting COP2; precise BD/EPC and asynchronous/interrupt observation are
  outside this contract. Calls, conditional branches and non-null returns do
  not gain this eligibility.

All three options accept only `on` or `off`, default to `off`, and use the last
supplied value. The first two are selected only for `game/DrawSky.c`; the CTC2
policy is scoped to the isolated rotation-composition/loader translation unit.
No function names or retail addresses select behavior inside the compiler.
The patch SHA-256 is
`bd7e26fc281eb104a2c40e5965aa7401de8515f364ca741cb87166d797091936`.

The local patch also adds `-fcrossjumping` / `-fno-crossjumping`. The default
preserves GCC 2.8.1's original final-pass tail merging; disabling it leaves
other jump cleanup and earlier optimization passes enabled.

`__builtin_mips_neg_si(int)` explicitly selects signed32 negation on
little-endian MIPS-I with 32-bit int, registers and ABI. Its value semantics are
ordinary C unary minus: evaluated `INT_MIN` is undefined, with no guaranteed
trap or dead-result effect. Argument evaluation has function-call sequencing.
The nonvolatile operation is classified as potentially trapping to prevent
execution on an additional unchosen path. Ordinary unary minus and ABS lowering
are unchanged; the builtin does not require a compilation-wide semantic policy.
Unsupported targets are diagnosed when the builtin is used.

`__builtin_mips_ctc2(unsigned int value, int control)` is a void hardware
control write on the same little-endian MIPS-I/32-bit target. The control index
must be a compile-time integer from 0 through 31. It stabilizes the full source
word once and completes queued argument effects before the volatile transfer.
Hardware owns control-register normalization; the operation promises no C-valued
control-register storage. It has no CPU GPR output or implicit call. The
return-slot option changes only its narrowly certified scheduling, not ordinary
ASM or arbitrary volatile effects.

## Frame policies

`psx-frame-policies-2.8.1.patch` follows the GTE patch in the same recipe.
Its SHA-256 is
`011f7dbfbc3b1c29ecf2ddfafe4fce027dc2e169c08402ed6a003cf9693b870e`.
All frame options are per compilation; no function names or addresses select them.

- `-mstack-align=4|8` defaults to `8`. Selecting `4` changes the calling
  convention: it is not standard o32 alignment, and every participating callee
  must tolerate word-aligned SP. The caller must provide a word-aligned SP and
  the normal argument homes. This mode requires little-endian MIPS-I/ABI32,
  32-bit int/registers and soft-float, without PIC or ABI calls. Varargs,
  dynamic allocation, setjmp, structure returns, profiling and actual local or
  spill slots aligned above four bytes are rejected. Reload's zero-size frame
  bookkeeping does not impose eight-byte alignment on an otherwise valid
  word-aligned frame; genuine stronger-alignment requests remain errors.
- `-mprologue-arg-alias=on|off` defaults to `off`. For a bounded fixed-frame
  o32 entry prefix after reload, certify only an ordinary word read directly
  based on an unchanged typed incoming pointer in A0–A3 as disjoint from a
  compiler-generated GPR save at its actual new-frame home. The compiler also
  orders work after the real frame allocation. Loaded/copied/integer-derived
  pointers, source ASM, volatile effects, calls, control flow, exposed frames
  and unsupported function contexts do not gain the exemption. In excluded
  contexts, enabling it may still add conservative allocation ordering; it
  does not promise unchanged output with `on`. Save ordering, unrelated alias
  edges and unwind metadata are not rewritten.
- `-mframed-return-boundary=on|off` defaults to `off`. A compiler blockage
  after nonzero frame teardown prevents filling the return delay slot from
  before that boundary. Frameless returns retain ordinary delay-slot filling.
  This is not an exception/interrupt, unwind or alternate-assembler guarantee.
- `-mordered-gp-prologue=on|off` defaults to `off`. For RTL-only fixed frames
  of 1 through 32767 bytes, emit selected GPR saves in ascending order at their
  existing homes and complete the save group before body work. Frame size,
  save mask, per-save frame notes and restore emission are unchanged. Requires
  little-endian MIPS-I/ABI32, 32-bit int/GPR/pointer modes, soft-float and no
  PIC/ABICALLS. Frameless, large or dynamic frames, frame pointers, FPR saves,
  aggregate arguments, varargs, EH, profiling, nonlocal/nested contexts and
  textual prologue emission retain the original path. It changes the legacy
  RA-first debugger heuristic when enabled; compilation with debug records
  does not establish asynchronous unwind, interrupt or fault-order equivalence.
  This option alone does not change the stack calling convention or grant an
  alias exemption. Constrained scheduling can change other enabled bodies;
  no general size/performance parity is promised.

Only the last supplied value for each option is interpreted; invalid final
values are diagnosed. Thus an earlier invalid value followed by a valid value
is accepted. Default/explicit-off controls preserve the existing behavior.
`tests/check_psx_frames.sh` checks emitted frame/save/load/return instructions,
dense/sparse save homes and body boundaries, conservative pointer/ASM cases
and genuine excluded frame/FPU/PIC/profile contexts, not game-specific bytes.
The debug control checks emitted records and frame/save/restore layout only,
not debugger execution or unwind correctness.

## Tested compiler release

- Release:
  <https://github.com/decompals/old-gcc/releases/download/0.17/gcc-2.8.1-psx.tar.gz>
- Release archive SHA-256:
  `f6f6e883942d4d3289d048236c672e71ed410e546aaae8ff655952f1567e1be0`
- Platform: statically linked Linux i386
- Local directory: `prebuilt/linux-i386/2.8.1`

The C compiler pass (`cc1`) below includes both local patches. Two clean builds
produced this hash in the fixed Ubuntu 20.04/GCC 9.4.0 environment: one from the
recorded 1,221-file source input set, and one freshly reconstructed from the GNU
archive and the public patch recipe. The latter also passed the maintained
base, GTE and frame smoke controls.
The other four files remain unchanged from the release archive. The Dockerfile
builds only the replacement `cc1` and exports it alongside those stock files.
Its pinned base image and source archive do not freeze future apt repositories;
the executable hashes, not a fresh package-manager run, identify the tested tuple.

```text
94873e8608def44bfac4d24408e6853afbb48ae979afc4e5d81be2b46778b735  cc1
93e7b36546532deec58cbc221df4d2c2894d444b5685b31a439c46cc597a9572  cc1plus
8d7f4e90f995089a6c2d62214b607fc77acf43b61f60a871d130d713d9ac96bb  cpp
a2cf0a5f0a21fcd4836c6f7ac66f346ba5594c1e4d4ab9d88c48c73c77f876fd  g++
451e6521d860fc6ff49000567d94d983125263bb0ae2149f89d95eaed34b3a02  gcc
```
