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
supplied value. Enabled compilations are scoped by
`metadata/retail/ntsc-u-926/matching.json`; the CTC2 policy remains limited to
the isolated rotation-composition/loader translation unit.
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

## Unreachable assertion

`generic-unreachable-2.8.1.patch` adds the target-independent C builtin
`void __builtin_unreachable(void)` in 19 frontend lines. Its SHA-256 is
`0274f15c7160a44b19cef40e8a1dd157d9ea1d362d4387425696697a23cf3d2d`.
It completes queued expression effects and emits an RTL barrier, not a CPU
instruction, call, trap, fence or physical transfer. Reaching it is undefined
execution; it neither restores a frame nor makes an escaping ASM boundary safe.
Unoptimized output may serialize dead continuation after the barrier.
`tests/check_unreachable.sh` checks real O0/O2 barriers, earlier effects,
optimized dead-continuation cleanup, valid framed returns and argument errors.

## Explicit scratch-frame and register-call contracts

`psx-scratch-call-2.8.1.patch` follows the unreachable patch. Its SHA-256 is
`81e9d1f1165849d20bf49247e402db5852d015764a988ff33c4cb98fbb37d965`.
The exact delta is 611 added/15 removed lines across nine compiler owners,
596 net; generic reorg.c is unchanged. It contains four bounded capabilities:

- `mips_regcall_s012(mask)` applies to direct, non-inline C `void(void)`
  declarations on little-endian MIPS-I/ABI32 soft-float, non-PIC targets.
  It promises zero incoming stack homes, hidden reads from the constant
  GPR2–25 bit mask, and real RA/S0–S2 kills. Other ordinary-call kills remain
  conservative. Equivalent folded integral masks are canonicalized, not
  arbitrary casts stripped. Declarations must repeat the contract; function
  addresses and unsupported signatures/contexts are rejected.
- `mips_unsafe_manual_at_frame` marks an implementation body, not its outward
  ordinary call ABI. Its source must balance RA/S0–S2 saves in an AT-based
  24-byte scratch window and owns slots16/20 for data. Calls must be the
  register-call form with the separate `mips_preserves_at24` provider assertion.
  The compiler models AT's hidden read, suppresses automatic stack frames and
  emits function type/size rather than misleading SP-frame metadata. It rejects
  stack/spill/homing/extra-save requirements and unsupported debug, profiling,
  exception, dynamic and inline contexts, including late post-reload checks.
  Ordinary callers keep their normal homes; the body marker cannot be inlined.
  These unsafe assertions do not prove the source's balance, actual provider
  preservation, assembler macro behavior or asynchronous safety.
- `-mpsx-normal-ram-lw-delay=on|off` defaults to off, accepts only those values
  and uses the last supplied value. It permits only a certified immediately
  preceding volatile SI LW to fill an EQ/NE-zero branch slot, retaining the
  access on both paths. Full-register hazards, two real successor gaps and
  installed-slot final validation remain mandatory. The caller asserts aligned,
  mapped, nonfaulting normal RAM, never MMIO; precise BD/EPC, IRQ timing and
  asynchronous mutation are outside the contract. The existing LHU input-role
  check now admits a genuinely fixed read-only condition, without relaxing
  its strict destination guard.
- Typed CTC2 accepts constant zero through the existing CONST0/J/%z convention,
  without pretending the virtual argument-pointer register is physical GPR0.
  Constant zero has no mutable CPU-register dependency; other inputs remain
  full SI registers and argument effects are still completed before transfer.
  One generic CSE correction validates PARALLEL USE substitutions together
  with their owning instruction, preserving recognized MATCH_DUP operands.

`tests/check_psx_scratch.sh` checks real call masks/kills/homes, manual-body and
inline guards, default/off/on load slots and reached hazard/fixed-role cases.
The GTE tests retain zero/ordinary-word CTC2 and side-effect/frame controls at
O0/O2, including actual intermediate operand agreement. These are finite
compiler-output controls, not execution on MIPS or universal ABI/fault proof.
No game names or retail addresses select behavior in the compiler.

## Ordinary mapped-RAM slots and fixed epilogue cloning

`psx-ram-load-2.8.1.patch` follows the scratch/call patch. It is the literal
595-net-line delta (+618/-23 across four compiler files), SHA-256
`a2221b5a05bf81f3797b20725e901422e52579681ba5b9b1e95a60767154cd37`.
Its two independent optimization policies default to off, accept only `on`
or `off`, and interpret the last supplied value. Invalid final values diagnose;
an invalid earlier value followed by a valid final value is accepted.

- `-mpsx-prebranch-lw-delay=on` admits an adjacent backward ordinary SI LW or
  signed LH, or a first-fallthrough SI LW, into a certified unannulled NE/GT-zero
  branch slot on little-endian MIPS-I/ABI32. Forward speculation does not admit
  signed LH. Unlike the existing volatile-load option, the access is ordinary
  nonvolatile RAM. The caller asserts complete, aligned, mapped, stable,
  nonfaulting normal storage on every affected path, including paths on which
  the source did not access it. MMIO, unmapped or faulting storage, precise
  fault/BD/EPC context, IRQ timing and asynchronous mutation/observation are
  outside this contract; a C pointer type does not establish those properties.
  Real register hazards and generic opposite-path old-value liveness remain
  mandatory. One emitting prefix ALU and independent physical instructions on
  both successors protect load retirement. A USE marker is never an instruction
  gap: the sole admitted administrative marker must identify the actual
  preceding SEQUENCE's ALU tail. Installed slots are protected from copying,
  threading, retargeting and relaxation; final output validation rechecks their
  current ownership, carrier and both gaps, failing fatally on invalidation.
- `-mclone-fixed-epilogues=on` clones actual compiler-generated restores,
  blockage, SP teardown and RA return at exactly one direct shared-exit jump.
  It runs after final jump merging and before fresh delay-slot resource tables.
  Its initial scope is optimized C void functions with small fixed SP frames
  saving only ABI S0-S7/RA. Frameless, value/aggregate-return, dynamic/FP/FPU,
  PIC, debug, profiling, EH/nonlocal/varargs contexts, multiple clone candidates,
  extra SP effects and unsupported template notes retain the ordinary path.
  Empty constraints are inspected through real operand vectors, not constraint
  metadata; nonempty ASM and opaque machine effects exclude the transformation.
  Actual frame homes, RA use and the original final epilogue are retained, with
  fresh instruction identities and updated exit bookkeeping. This is neither
  a scratch-frame ABI nor a debug/unwind, fault or interrupt-equivalence promise.

Enabled game compilations are approved only for the FIXED and MOVED collision
quadblock dispatcher translation units selected in the matching manifest.
Neither policy grants neighboring compiler, source or assembly exceptions.
`tests/check_psx_ram_load.sh` checks actual installed SI slots and physical gaps,
an opposite-path old-value refusal, literal default/OFF output and pre-DBR
parity, parser/last-value behavior, real fixed-frame clones/restore homes and
selected conservative fallbacks. Its fixtures are maintained inputs, not
private-path dependencies. Signed-LH admission, mixed volatile policies,
malformed-marker/lifecycle cases, a complete exclusion matrix and physical
MIPS execution are not established by these finite compile-output controls.

## Adjacent continuations and full scratch bodies

`psx-continuations-2.8.1.patch` follows the ordinary-RAM patch, SHA-256
`698ebf5719f8a6ac8a594af894da1dee94400826f04263ee84e53dc2c1d27dcb`.
Its delta is768 added/15 removed lines across calls.c, toplev.c and mips.c/h/md,
753 net; generic flow, allocation and reorg passes remain unchanged.

`mips_adjacent_tail("Target")` is a C body contract for a genuine terminal
direct void four-SI call. Its real nonconstant CALL retains arguments and RA
reads but emits no physical transfer, RA write or delay slot. Supported setup
branches are local forward acyclic edges before that call. Extra calls, paths
skipping it, later work, frames, protected writes and unsupported contexts fail.
Each entry emits a nonallocated eight-byte end/target relocation record;
linkage must preserve it and verify distinct real positive function owners
with exact end-to-entry adjacency, not ABS addresses or interior labels.

`mips_unsafe_manual_at_full_frame` retains an ordinary four-SI void outward
interface and a real two-SI indirect void callback with sixteen incoming homes.
It suppresses automatic frames, not real CALL/RA/memory effects. Its source
owns the original S0-S7/GP/SP/S8/RA image, balance, mapped/disjoint scratch,
AT rebuilding, provider preservation and nonreentry. Late checks reject
storage, unsupported calls, opaque instruction ASM and unmodeled protected
operands/writes; they do not prove the unsafe source obligations. Empty cues
remain constrained register captures/observations, not CPU instructions.

`-mpsx-prebranch-lw-delay=extended` adds bounded SI load-gap classes and a
backward-adjacent local-jump load slot; strict `on`, default and `off` retain
their existing scope. Final reciprocal sequence ownership and actual successor
gaps remain mandatory. A write-only pending-GPR kill or independent EQ/NE
branch is narrowly recognized, never a blanket branch or ASM exemption.
Mapped, stable, nonfaulting RAM and normal execution are required; precise
fault/BD/EPC, IRQ and asynchronous observation remain outside the contract.

Only full-scratch ADDU with three actual GPR operands and destination tied to
source two prints that tied source first. It changes no RTL effects, opcode,
trap behavior or scheduling policy; ordinary/immediate/other-profile output
retains the original template. No game names, addresses or expected words
select any of these behaviors.

The archive and ordered pinned patches determine the source; no per-file public
registry is maintained. Source fingerprints use C-sorted `./path`, NUL, octal
mode, NUL, lowercase content SHA-256 and newline for each regular file before
configure. The current complete tuple is recorded below. The C++ frontend and
four stock exported files do not gain these C contracts. Compile/output checks
do not prove hardware execution, universal provider balance or fault equivalence.

## Sparse S-bank owners and typed ordered operations

`psx-animation-2.8.1.patch` follows the continuation patch. Its SHA-256 is
`f6ad3c811fa75bc93a63755439b2a806b0344013bbe837db46fd6cca1312b1c0`.
The delta is759 added/25 removed lines across nine C/target owners,734 net.
The ordinary flow/allocation passes remain unchanged; one bounded delay-slot
helper is added to reorg.c. No game name, address or expected word selects it.

`mips_unsafe_manual_at44` marks one emitted ordinary C void body per translation
unit with four to six o32 SI arguments. Source owns the original S0-S7/RA
captures, mapped/disjoint AT-based scratch image, restoration and provider
preservation. The used offsets are0..28 and40;32/36 are not compiler-managed
saves. Actual storage, automatic homes, SP/GP/S8 use, unsupported calls/contexts
or unsafe return/slot shapes are refused, including after allocation and DBR.
The marker suppresses automatic frames, not the outward ordinary caller homes.
It is unsafe: acceptance does not prove path balance or hardware preservation.

An optional single bounded C identifier exports a real GLOBAL zero-size CODE
label on the same certified JRRA/NOP as the internal owned return. Consistent
redeclarations inherit it; conflicting names, malformed names, own C/assembler
names and multiple emitted owners are rejected. Other global symbol collisions
remain the real assembler/linker's responsibility. The interface does not
provide an offset, EQU, trampoline, alternate entry frame or cross-owner restore.

`mips_regcall_sbank(mask)` describes direct non-inline C void(void) calls with
hidden reads selected from GPR2..25, an AT read, actual RA/S0-S7 kills and the
remaining conservative ordinary-call effects. It requires the separate
`mips_preserves_at44` provider assertion inside a sparse owner. Function
addresses, incompatible declarations and unsupported contexts are errors.

`__builtin_mips_incoming_ptr(formal)` reads only an owner's actual fifth/sixth
pointer from original SP16/20, once each and before control transfers. It is
not arbitrary stack addressing. Typed `__builtin_mips_mtc2(value,index)` accepts
only literal data indices0..31, completes queued argument effects, records its
CPU input and conservatively invalidates the modeled SXY2/VXY0 aliases. It
promises neither C-valued GTE storage, a command nor a hardware latency barrier.

`__builtin_mips_umod_hi(n,divisor)` has unsigned32 remainder value semantics;
the divisor is a nonzero constant. It uses allocatable HI plus truthful LO/HILO
scratches. The pure validated divisor is materialized before the dividend;
argument effects still complete once. `__builtin_mips_read_hi(value)` transfers
that value to a GPR, using MFHI for HI or a normal move for a materialized GPR.
A value can survive other writers/spills without a spurious MTHI restoring it
over an unread multiply result. Neither operation exposes a global HI token;
ordinary multiplication, reload and quotient/remainder lowering are unchanged.

`-mcommon-head-slot=on|off` defaults to off and uses the last supplied value.
On the supported non-PIC MIPS-I/ABI32 target, its post-reload helper admits
only owned EQ/NE-zero branches whose two immediate successors contain the
identical nontrapping one-word SI register signed16 update. It excludes
protected registers, memory/opaque effects, alternate entries, unsupported
notes and debug/profiling contexts; it updates real resource/block ownership
and installs one unannulled slot using the old branch condition. It is not a
generic opposite-path speculation or register-allocation policy.

`tests/check_psx_scratch.sh` reuses its existing compile/body/refusal helpers
with five shared standalone fixtures. They check actual emitted sparse images,
typed incoming reads, real call uses/kills/homes, HI values across MULT/spills,
GTE aliases, common-head positive/refusal/default output and named-return
metadata/diagnostics. They are compiler-output controls, not MIPS execution,
physical assembler macro/latency proof or automatic unsafe-source certification.
Actual AS/link collision/PC16 and complete application-window ownership remain
separate integration gates; case16 and TEST_BORROWER in the owned return fixture
are available for those real-tool checks without any private include.

## Code-generation policies and signed high values

`psx-codegen-policies-2.8.1.patch` follows animation: 74 added lines, SHA-256
`341a6892bc34d6470c40809f5d748d59aaac7398b5a379026f4c8e633195950e`.
Both policies default off and are selected only for the Clock translation unit.

- `-fschedule-store-load-order` adds conservative order-only ANTI dependencies
  for eligible simple, nongrouped loads in the first scheduler. Existing TRUE
  dependencies remain; self edges, groups and cc0 participation are excluded.
  It neither changes alias analysis nor supplies a CPU/C memory fence.
- `-mpreserve-small-si-constants=on|off` protects necessary one-word, nonzero
  signed16 SI literal assignments from two late reload rewrites on MIPS-I/ABI32.
  Only allocatable GPR2..25 qualify. It does not preserve dead constants or
  arbitrary immediate operands. The final option wins; invalid values diagnose.

`generic-smul-high-2.8.1.patch` follows that patch: 49 added frontend/expander
lines, SHA-256
`d9db3535d1fe3b4641f6369b544d76f433eb581b2c559375f36849da88467dc0`.
`int __builtin_smul_high_si(int,int)` returns the signed upper32 of a signed32
product. Converted arguments evaluate once; raw queued values are captured before
their effects complete. It requires signed32 SI arguments/result and an existing
direct signed-high optab handler, otherwise diagnoses without library fallback.
The operation has ordinary value semantics and truthful HI/LO effects, not a
global hardware-state token, new calling ABI or compile-wide arithmetic policy.

`tests/check_psx_codegen.sh` checks actual added/retained dependencies, literal
default/OFF behavior, boundaries/exclusions, reached literal preservation and
parser refusals. `tests/check_smul_high.sh` checks real O0/O2 signed products,
queued old-value snapshots, live results through later writers/calls and arity
errors with standalone C/provider fixtures. These are finite compiler-output
controls, not MIPS execution or universal scheduling/availability coverage.

## Current complete compiler tuple

Two fresh maintained constructions produced byte-identical complete five-export
tuples, with all seven maintained smoke suites passing on both binaries. The
replacement cc1 is
`8c1112f99e9e47a496a1d98bc9efc468c630a4925361e56b7801da5016fa5207`,
a statically linked Linux i386 executable. The four stock companions retain the
hashes listed in the historical release table below. The complete deterministic
pre-configure source fingerprint is
`b5a88639f676ad13568262c0cdd4685516a143f85a5cfdb58ffd5f3877bbe443`,
1221 regular files, zero symlinks, with original source modes retained. Cached
offline Ubuntu20.04/GCC9.4.0 provisioning is distinct from freshly executing the
Dockerfile's mutable apt steps. Matching and native coverage are verified
separately; these compiler constructions and output controls do not establish
whole-game or MIPS-hardware correctness.

## Previously tested compiler release

- Release:
  <https://github.com/decompals/old-gcc/releases/download/0.17/gcc-2.8.1-psx.tar.gz>
- Release archive SHA-256:
  `f6f6e883942d4d3289d048236c672e71ed410e546aaae8ff655952f1567e1be0`
- Platform: statically linked Linux i386
- Local directory: `prebuilt/linux-i386/2.8.1`

The C compiler pass (`cc1`) below includes the GTE, frame, unreachable,
scratch/call and ordinary-RAM/epilogue patches. Two clean offline builds from the GNU
archive and public patch recipe produced this hash in the fixed Ubuntu
20.04/GCC 9.4.0 environment,
with the same complete
1,221-file source map
`680e1af032aec56163871eddfa96a633e794dd99b429d17d0c859611dac4973e`
and the base, GTE, frame, unreachable, scratch/call/load and ordinary-RAM/epilogue smoke suites passing
on both binaries. The source permissions follow the GNU archive; executable
and source-content identities are independent of owner-write permissions.
The other four files remain unchanged from the
release archive. The Dockerfile builds only the replacement `cc1` and exports
it alongside those stock files; the C++ frontend has not gained these C APIs.
Its pinned base image and source archive do not freeze future apt repositories;
the executable hashes, not a fresh package-manager run, identify the tested tuple.

```text
a22f62beedb71c97ef637600492189327039433b1da7c8679b75492e60247770  cc1
93e7b36546532deec58cbc221df4d2c2894d444b5685b31a439c46cc597a9572  cc1plus
8d7f4e90f995089a6c2d62214b607fc77acf43b61f60a871d130d713d9ac96bb  cpp
a2cf0a5f0a21fcd4836c6f7ac66f346ba5594c1e4d4ab9d88c48c73c77f876fd  g++
451e6521d860fc6ff49000567d94d983125263bb0ae2149f89d95eaed34b3a02  gcc
```
