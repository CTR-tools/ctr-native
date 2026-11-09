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

## Loop optimization control

`generic-loop-optimize-2.8.1.patch` adds `-floop-optimize` and
`-fno-loop-optimize` to the matching C compiler. Its SHA-256 is
`ddb7cd2bb75d0d635e8385016fbe171be76ce9d9261478908a024b89b36cb621`.
The default retains the original loop pass; the last supplied value wins.
Disabling it skips all of `loop.c`, including invariant motion, induction
optimization and unrolling, but preserves frontend loop notes and the other
optimization passes. It does not change ordinary C operators or instruction
lowering. The opt-out is scoped to the icon unit in the matching manifest.
`tests/check_loop_optimize.sh` checks actual O0/O2 output and RTL for default/ON
identity, precedence, loop notes and retained volatile/call effects. Its optional
third argument compares against a separately retained baseline compiler.

## Opt-in T9 operand class

`mips-t9-constraint-2.8.1.patch` adds `-mt9-constraint=on|off`, default `off`,
last supplied value wins. SHA-256:
`34aa7a10a85f6aec8a1d9eb1df5dd11ea877cab012db1dfd6856704e3737b22f`.
ON permits the `c` constraint to request the ordinary caller-clobbered T9 GPR
at an operand. It does not permanently bind a C variable or change the call ABI;
the allocator preserves live C values around calls. Default/OFF keep this class
empty and the constraint unsupported. Class tables are recomputed after target
options without repeating fixed/call-used/global-register usage initialization.
The ordinary-GPR spill/copy-cost treatment is explicit, not a new hardware unit.
`tests/check_mips_t9.sh` checks default/OFF and last-value output, impossible
operands, requested register usage, volatile effects and real call preservation.
These are finite compiler-output checks; no GDB or game source is needed.

## Opt-in static-object and private-frame disjointness

`mips-static-frame-alias-2.8.1.patch` adds
`-mprologue-static-alias=on|off`, default `off`, last supplied value wins.
SHA-256:
`abce0a83ad019cca09a51e73800107fd81d2aec093844a156506c01aca2d4390`.
It follows the T9 patch and changes 287 added/1 removed compiler lines across
the MIPS target, reload allocation and sched2 dependency owners. No game name,
address, expected word or hard-register equivalence selects the behavior.

For an eligible fixed o32 frame after reload, the policy can prove a bounded
ordinary byte read from declared static storage disjoint from a real computed
GPR save home or a fresh, unshared compiler reload home. Reload ownership uses
the exact pseudo node converted in place to MEM, reset each function, not a
numeric SP offset. Only complete aligned SI stores inside the computed locals
area qualify; caller/outgoing homes, copied/partial/source-addressable/unknown
stores do not inherit that identity. Existing non-memory dependencies, frame
notes, save/restore semantics and the source memory operation remain intact.

Static extents remain mutable. A bounded latest-definition trace accepts only
checked nonnegative offsets and complete matched HIGH/LO_SUM relocations,
within 96 entry instructions and 12 trace levels. Both recursive interpretations
share one conservative latest-single-GPR-definition lookup. Loaded pointer/index
values do not gain bounds from assumed table contents. Assembler names require an alphabetic
or underscore base, then alphanumeric/underscore/dot characters, optionally
one fully parsed +/- integer. Register, relocation and other expression names
are not static-owner evidence; leading-dot names are conservatively excluded.
Truthful declarations must describe real readable static objects or views.

The physical contract is normal, nonfaulting memory and a fresh disjoint ABI
stack frame, not MMIO or an unsafe/manual stack window. Little-endian MIPS-I,
ABI32 and soft-float are required. PIC, frame pointers, dynamic/large/exposed
frames, profiling, EH/nonlocal/nested/variadic/manual contexts, source ASM and
prefix calls/control transfers do not gain an exemption. Precise fault order,
asynchronous observation and debugger/unwind equivalence are not promised.
ON may still add conservative frame-allocation ordering in excluded contexts;
default/OFF retain the original output, not universal ON output identity.

`tests/check_mips_static_frame_alias.sh` uses standalone C and real scheduling
dependencies to check static extents, private reload stores, source-home edges
after the reload-owning function, legal section-qualified SP/GP name refusal,
default/OFF/precedence and observable call/branch/ASM ordering. These are finite
compiler raw-output/RTL checks, not assembler execution, MIPS execution or a
complete exclusion matrix. Application, actual assembler/link and native gates
remain separate.

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
- `-mordered-gp-prologue=on|off|descending` defaults to `off`. For RTL-only fixed
  frames of 1 through 32767 bytes, emit selected GPR saves in ascending (`on`)
  or descending order at their existing homes and complete the save group before
  body work. Frame size,
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

`mips-ordered-save-direction-2.8.1.patch` follows the static/frame-alias patch.
SHA-256:
`ed2765cc6e4b978649cb5522c275d7adfd2a62b72cade7e900dc6356f579ed77`.
Its 18 added/7 removed target lines reverse only the existing eligible save
traversal/home direction and list all three values in invalid-option diagnostics.
Default, explicit OFF and previous ascending ON retain their prior output.
The frame checker reuses its dense/sparse/debug home, restore and body-boundary
checks for descending order, compares default/OFF/ON with an optional preceding
tool prefix, and checks descending precedence, exclusions and invalid values.
It adds no source fixture or suite and does not certify MIPS execution.

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

`generic-scheduler-2.8.1.patch` follows the ordered-save patch (SHA-256
`a9fb7f24a39142f22fc8b8441c0b6c81ba7134302b415c07c33c8c94cdc8319a`).
Its three default-off options enable dying-input/fresh-output coalescing for
canonical empty ASM, first hazard-safe pre-reload ready ties, and guarded
zero-issue accounting for exact-empty postreload ASM. It retains dependency,
priority and hardware-latency rules; uncertain real input latency and grouped
instructions refuse zero issue. The matching manifest scopes these options to
Matrix setup. The standalone generic-scheduler smoke checks actual copy removal,
ready ties, zero-clock observations, real load/LO-overlap refusals and option
precedence. These finite compiler-output checks are not hardware timing proof.

`generic-local-register-hint-2.8.1.patch` follows the scheduler patch (SHA-256
`5a476128a14964acfcd31f047c2164c37f3bbb7473946d31b4a3a5be08417285`).
The explicit C attribute `__attribute__((local_register_hint("10")))` gives an
automatic single-word integer/pointer an ordinary LOCAL quantity preference,
below copy/ABI preferences; it does not create hard-register RTL or exclude
reload scratch registers function-wide. Unavailable/conflicting hints fall
back, and declarations lost to optimization need not retain their preference.
Invalid scopes/types/registers and hard-asm combinations diagnose. Existing
`asm("reg")` bindings remain hard; GLOBAL allocation and reload policy are
unchanged. The patch adds 118 C lines (115 net), plus a Makefile dependency.
`tests/check_local_register_hint.sh` covers actual homes, inline metadata
lifetime, fallback/coalesced conflict, unannotated identity, unchanged hard-asm,
invalid forms and disjoint same-physical-register BLK scratch reuse.

`mips-nonfaulting-sh-return-2.8.1.patch` follows the local-register-hint patch
(SHA-256 `9a07f18d2fd45363dc3856b755a2a9c702685181dcd1e11056cff3748341f4ea`).
The default-off `-mnonfaulting-sh-return=on` policy certifies one immediately
preceding volatile halfword store for an ordinary frameless leaf return, in
little-endian MIPS-I, ABI32, soft-float, non-PIC code only. The caller asserts
mapped, nonfaulting MMIO; fault BD/EPC and interrupt observation are excluded.
The store remains volatile, its width/address/input must be legal, and all true
memory/register conflicts remain enforced. Only comparison copies lose the
certified volatility bit. Existing protected-slot checks prevent copying or
retargeting; final validation rejects a broken installed contract.
`tests/check_nonfaulting_sh_return.sh` covers positive stores, unchanged
unsupported shapes/profiles, preceding volatile order, normal scheduling,
default-off identity, option precedence and invalid values.

## Earlier complete compiler construction

Two fresh clean constructions produced byte-identical complete five-export
tuples, with all thirteen maintained smoke suites passing on both binaries. The
replacement cc1 is
`01dbda0e42a1029dfe64fc264262da74bfb1f1dba1c0d8d4a4da5905094f2ad4`,
a statically linked Linux i386 executable. The four stock companions retain the
hashes listed in the historical release table below. The complete deterministic
pre-configure source fingerprint is
`1489d3e3c9cd45fadeebed5dfeadd7e228b9f4970313e104a5be2fd1f24a7e59`,
1221 regular files, zero symlinks, with only the six patched files made
owner-writable as in the recipe. Cached offline Ubuntu 20.04/GCC 9.4.0
provisioning is distinct from freshly executing the
Dockerfile's mutable apt steps. Matching and native coverage are verified
separately; these compiler constructions and output controls do not establish
whole-game or MIPS-hardware correctness.

Each construction retains 1111 legacy host build warnings and the existing
configure diagnostic `-D__psx__: not found`. The static/frame-alias smoke emits
no stderr; the extended frame smoke emits expected debug/profile warnings.
These diagnostics are recorded, not a warning-free build claim.
The preceding static/frame construction used cc1
`de93d4970b7596316aed47a1c13f72ee9084dd55a79ea1801e9de90abd183876`
and source fingerprint
`607595a168727cc764ac4ef8e5aa06c612acde55d384ad96fc64b608bf0f114f`.
The earlier T9-option construction used cc1
`9bff81a6308db65bfa81c043bef00aed3f91773700563eea00b44a1dddac9ddb`
and source fingerprint
`4a14b455edd6c3df8575c474eafece4ae353003fa6ff870875c65ab2ad5fe00f`.

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

## Opt-in late in-place updates at actual call setup

`generic-call-setup-tie-2.8.1.patch` follows the nonfaulting-SH-return patch.
SHA-256: `beb2f8d8fcfc0e9ecc10ced3da1ff202969041c5e98914299c9bc835adcc1156`.
The default-off `-fsched2-late-inplace-at-call-setup` policy changes only a
postreload ready tie with equal priority and potential hazard after actual
hazard filtering. It prefers a pure nontrapping complete single-hard-register
integer in-place update which is not itself an actual call argument, when a
genuine call-argument definition competes in that tie. Argument identity needs
a TRUE CALL dependency plus a register USE in CALL_INSN_FUNCTION_USAGE, not a
particular register, callee, literal or machine opcode.

Per-block role marking adds no dependencies, ABI, instruction or RTL form.
Existing priority, hardware hazards, groups, frame rules and call/register/
memory dependencies remain authoritative. Fixed/global/partial/multiword
destinations and unknown/effectful/memory expressions receive no preference.
Default/OFF and final-option precedence are covered by standalone controls;
an optional retained-compiler prefix also compares against its baseline. The new
tests are finite compiler-output/RTL controls, not physical MIPS timing,
runtime execution or universal exclusion coverage. Application, native and
aggregate output checks remain separate gates.

## Opt-in volatile word store in a private-frame local-jump slot

`mips-private-frame-sw-jump-2.8.1.patch` follows the call-setup tie patch.
SHA-256: `7a7589c325964172ff16ae4b667cfd636fff3780f46ab22c6cda0ed6f9ef66ff`.
The default-off `-mprivate-frame-sw-jump=on|off` policy certifies one adjacent
volatile SI word store for a direct nonannulled local jump, in little-endian
MIPS-I/ABI32 soft-float code with 32-bit int/long/pointer/word and ordinary
non-PIC calling conventions. It requires actual fixed private leaf locals,
aligned complete in-local bounds and a dominating straight-entry allocation.
Calls, exposed/dynamic/manual/unsupported frames, other widths/bases/regions,
conditional/computed/return edges and source/SP hazards are excluded.

Existing comparison-only volatility handling preserves actual RTL/register/
memory resources; protected delay sequences and final validation remain.
The caller supplies mapped nonfaulting private stack RAM. Precise fault BD/EPC,
interrupt/asynchronous and debugger observations are outside this opt-in.
The matching manifest enables it only for the two SPU voice-setter C bodies;
native audio delegation is unchanged and needs no host timing loop.

The patch adds151 C/header lines and removes3, without changing MD or reorg
algorithms. Standalone C/POSIX-shell controls check four triggered sequences,
sixteen unchanged negative bodies, option/default/precedence and unsupported
profiles. Optional retained-tool comparison and two clean offline archive
builds supplement the existing compiler suite. Controls inspect generated
instructions/RTL; they do not execute MIPS or prove hardware/IRQ/fault timing.
Only C cc1 is rebuilt; other exported frontend tools remain unchanged.

## Opt-in target-thread delay-slot search order

`generic-delay-slot-target-first-2.8.1.patch` follows the private-frame store patch.
SHA-256: `f34e266d526290cd9bcdc0c4080aa1069ad28e3cbd665bd384efc58aa74364eb`.
The default-off `-fdelay-slot-target-first` preference tries the existing target
thread scan before fallthrough for a direct local conditional with one slot,
no annulment, both threads owned, static prediction zero and no profiling or
probability note. Real prediction and LIKELY, eligibility, resource/liveness,
trapping/volatile, ownership and fallback checks remain unchanged.

This is search order, not fabricated probability or a new instruction lowerer.
The manifest enables it only for the stream read/gate source. The patch adds
16 lines and replaces one; no game names, registers, constants, addresses or
expected bytes occur in its predicates. C/POSIX-shell controls cover actual
target moves, live-value/trapping/volatile refusals, ownership, strong NE eager
selection, precedence, disabled scheduling and supported profile guards.
They use only gcc/cpp/cc1; an optional retained-tool prefix checks old/default
identity. Real profile-note and eager prediction-two cases remain unexercised;
the unrelated old profile-note accessor defect is not changed or exploited.

Two clean offline archive builds reproduce the pinned C cc1; other four tools
are unchanged. Controls inspect finite assembly/RTL output, not executed MIPS,
hardware/IRQ behavior or whole-program equivalence. Native stream backend
selection is unchanged; application and provider checks remain separate.

## Opt-in constants carried through unconditional continuations

`generic-continuation-constants-2.8.1.patch` follows the target-first patch.
SHA-256: `3ebceed3f5cb7fe7951c15bcfde4241d78b743f564975ae9d2fdc6f87eec397d`.
The default-off `-fsched-continuation-constants` preference acts only after
reload; `-fno-sched-continuation-constants` disables it and the last supplied
value wins. It applies to the complete C translation unit, not a function-name
or address selector. Disabling sched2 also disables its final-crossjump guard.

Only a complete recognized one-instruction, ready-cost-one CONST_INT assignment
to a nonfixed single hard register in the target's BASE_REG_CLASS can qualify,
in QI, HI or SI mode. The real block tail must be a simple direct unconditional
jump. A straight source scan must prove the value is not overwritten before
that jump; a target scan must find an actual operand read before an overwrite,
call, control split or unknown instruction. Inert target labels/notes and bare
USE metadata can be skipped but do not establish a real read. Partial writes,
read-modify-write ambiguity, ASM, memory destinations and unsupported shapes
are refused. The proof uses actual lowered RTL, not source-literal cost claims.

The backward scheduler prefers these ready constants early in its reverse
selection, thereby late in forward order. Existing dataflow dependencies,
register/memory hazards, eligibility, clocks and hardware latencies remain
authoritative. Final crossjumping reuses the same proof to avoid merging away
the protected materialization; unrelated tails can still merge. Delay slots
remain governed by the existing resource and legality checks. The per-block
temporary eligibility cache is cleared at the common scheduler exit.

This is a continuation-state scheduling preference, not a claim of lower
register pressure or improved performance. It adds84 C/header lines and one
Makefile dependency replacement. `tests/check_continuation_constants.sh` uses
two standalone C90 fixtures for actual constant slots, retained two-jump tail
stores, ordinary sharing, default/OFF/precedence, disabled-sched2 inactivity
and reached overwrite/call/control refusals. Pre-reload RTL comparison ignores
only CODE_LABEL_NUMBER metadata, not RTL UIDs/operands; independent assembly
comparison uses a bijective local-label rename preserving definitions and
references. Expensive source materialization reaches an overwrite refusal
after LUI/ORI splitting, not a reached cost refusal; the loop control reaches
the direct-tail gate, not a special loop exclusion. These are finite compiler
outputs, not MIPS execution or a complete eligibility/exclusion matrix.

## Declaration-local optional spill homes

`generic-spill-home-2.8.1.patch` follows the continuation-constants patch.
SHA-256: `19826d695779c20b098da1d3b0b82f0a0ff096086bafccdaab1dcfb60abcf70f`.
`__attribute__((spill_home(offset)))` gives an existing automatic single-word
integer/pointer declaration a requested local-area home **if it actually
spills**. It does not force a spill, bind a hard register, make source memory
addressable or change the outward calling convention. Register-resident,
constant and eliminated declarations retain the ordinary path.

The offset is relative to the compiler's locals area, before the ABI outgoing
argument area is added. Actual reload width and rounded slot extent determine
size/alignment; forward unused space can be reserved without inventing a source
variable. The request cannot overlap/backtrack across allocated locals, violate
alignment or exceed the checked small extent. Real declaration/pseudo ownership
is collected before pseudo renumbering; ambiguous/inline associations,
memory/hard-ASM associations, late/shared or wide spill allocation are rejected.
Unsupported targets and dynamic, exposed, debug/profile, inline/nested,
variadic/nonlocal and other nonordinary frame contexts diagnose rather than
silently assuming a home. The implementation is deliberately limited to the
existing little-endian MIPS-I/ABI32 soft-float, non-PIC, eight-byte-stack target
contract. It is not a general stack-layout, unwind or debugger API.

The patch adds183 net C/header lines and one Makefile dependency replacement.
`tests/check_spill_home.sh` checks real SI/HI homes and rounded extents, shifted
outgoing areas, inactive declarations, old/default identity and actual checked
alignment/overlap/origin/unsupported-context refusals. Its optional fourth
argument runs an independent native ASan/UBSan snapshot control; it does not
execute MIPS or establish application semantics. Both new checkers accept an
optional retained old compiler prefix for genuine historical identity checks.

## Current tested compiler tuple

Two fresh offline GNU-archive constructions produce the same complete five
exports. The maintained Dockerfile's preconfigure commands reconstruct the
same complete1221-file source-content vector. Its deterministic path/mode/
content fingerprint is
`9a7b5dac3ce369e1683d5c344dd6f9879151ced89bcda506e8cc424607156fec`.
The two patches above total267 net C/header lines, including the scheduler
cache-exit clear, plus two Makefile dependency replacements. The replacement
statically linked Linux i386 C compiler is:

```text
d847631b432b24673971461bbcc2df9b8f8530f47e67912736beebae2598ecb7  cc1
93e7b36546532deec58cbc221df4d2c2894d444b5685b31a439c46cc597a9572  cc1plus
8d7f4e90f995089a6c2d62214b607fc77acf43b61f60a871d130d713d9ac96bb  cpp
a2cf0a5f0a21fcd4836c6f7ac66f346ba5594c1e4d4ab9d88c48c73c77f876fd  g++
451e6521d860fc6ff49000567d94d983125263bb0ae2149f89d95eaed34b3a02  gcc
```

The four companion exports remain unchanged; the C++ frontend does not gain
these C contracts. Cached offline Ubuntu20.04/GCC9.4.0 builds are distinct from
freshly executing the Dockerfile's mutable apt steps. Legacy host warnings are
retained, not a warning-free-build claim. Compiler controls, actual assembler/
link application ranges, native providers and whole-program coverage remain
separate gates; no hardware timing or universal correctness claim follows from
the compiler tuple.
