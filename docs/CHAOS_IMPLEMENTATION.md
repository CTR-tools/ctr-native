# Chaos implementation contracts

## Source map

| Responsibility                                | Owner                                                                            |
|-----------------------------------------------|----------------------------------------------------------------------------------|
| Stable effect IDs, slugs, scope and selection | `include/chaos_effect.h`                                                         |
| Save encoding and runtime layouts             | `include/chaos_save.h`, `include/namespace_Memcard.h`, `include/chaos_runtime.h` |
| Eligibility and runtime                       | `include/chaos_gate.h`, `game/231/RB_Chaos.c`                                    |
| Effect implementations                        | `game/231/RB_ChaosEffects.c`                                                      |
| Shared driving-class constant loader          | `game/Vehicle/VehBirth_Consts.c` (included by `VehBirth.c`)                       |
| Creation flow and shared settings screen      | `game/230/MM_NewLoad.c`, `game/230/MM_ChaosConfig.c`                             |
| Pause entry and HUD                           | `game/MAIN/MainFreeze_Chaos.c`, `game/UI/UI_Chaos.c`                             |
| External filesystem interface                 | `platform/native_chaos_ipc.c`                                                    |

Player guide: [Chaos mode](features/CHAOS.md).
External protocol: [Chaos external file provider](features/CHAOS_EXTERNAL_PROVIDER.md).

Asset-free regression suites mirror these responsibilities:
`tests/test_chaos_runtime.c` covers scheduling, activation, triggers, IPC and HUD;
`tests/test_chaos_effects.c` covers gameplay modifiers and instant rewards.
Both use `tests/shared/chaos_fixture.h` for the shared world, real module include
chain and integration doubles. `tests/test_warpball.c` independently covers orb
launch and teardown without loading the Chaos runtime. Visual/audio presentation,
AI movement after handover or time skip, and combined 2P/rescue/retry/quick-state
behavior also need in-game coverage; the asset-free suites use integration doubles
at those boundaries.

`RB_Chaos.c` owns activation, gameplay hooks and per-driver dispatch,
sharing `Chaos_TargetsDriver` for recipient checks. `RB_ChaosEffects.c` owns
effect operations, effect-specific state updates and queries, start-light drawing,
and Senile Mask warnings/voice. Body Swap pair selection and finish-line eligibility
stay in the runtime; returning the selected pairs is an effect operation.
Gameplay hooks check activation and applicability before invoking operations;
effect-specific adapters also query activation where needed. Operations retain their
gameplay conditions (terrain, fuel, pickup queues). The gameplay hooks preserve modifier
ordering and check eligibility before calling them; Hole in the Tank is tested
through `Chaos_ApplyDriver`.

## Time skip

King Crimson (instant ID 39, `king-crimson`) advances the event clock by 10,000 ms
and estimates unfinished bot travel from current AI speed in retail navigation
units, including Arena Champions' motion multiplier. `Chaos_GetTrackLength` shares
checkpoint-data validation with Yardrat Training; `Chaos_FindTrackDestination` shares its
route traversal and interpolation, including selected checkpoint branches.
Track-path lookup needs no mesh; only Yardrat Training's ground-placement operation
requires one. `BOTS_FindClosestNavFrame` is shared with bot handover;
`BOTS_SetRotation` rebuilds the destination navigation estimate. Bot position,
backup, render translation and checkpoint projection are synchronized without
running world physics or consuming damage/item/Chaos durations. Stationary bots
and unavailable destinations are left in place.

AI reattachment targets the frame after the nearest navigation point on the bot's
current path. This is an accepted approximation, not a segment-projected or full
ten-second simulation; it can skip a navigation point near a corner. With valid
track data, the race clock still advances if a bot's destination/navigation is
unavailable.

`PlayLevel_CrossFinishLine` is extracted from ordinary lap statistics and handles
both normal crossings and skipped bot crossings. Skip crossings are dispatched in
time order, including multiple laps, so finished bots retain their actual order
and time within the interval. The next ordinary results update sorts the field
and applies the existing event-completion rules. King Crimson uses an instant
notice for its 500-ms world-only palette window and delayed time-crate break;
the cue helper suppresses Za Warudo's ticks for this effect. The two effects
replace each other; pending time-stop impacts resolve before bot relocation.

## Time stop

Za Warudo (ID 38, `za-warudo`) is timed and capped at 10,000 ms.
`Chaos_Update` runs before the event clock and world updates. While stopped,
only its activation clock advances; other activation/notice/trigger clocks,
Fresh Start and transformation delays retain their values. Wumpa Tax cannot
repeat a suspended pulse. External command polling is also suspended: requests
remain pending until time resumes. Ordinary human driving cooldowns/reserves
still advance.

`Chaos_ShouldFreezeThread` filters the staged driver loop, native thread walker
and direct tick transitions. Human drivers, cameras/HUD, attached masks, boost
flames and human-owned weapons continue. Turbo ownership is read only for TURBO-bucket threads running
`VehTurbo_ThTick`; static scenery can share the flame model ID with another object
layout. Frozen hazards run collision-only operations without advancing motion,
animation or cooldowns. Kart-to-kart collision searches and responses are skipped
during time stop. Nitro/potion contacts are retained regardless of ownership,
allowing passage without immediate damage or detonation. On resumption, Nitro
uses its ordinary area blast; red/green potions recheck their normal 3D contact
radius against current kart positions. An empty-radius potion still shatters,
without damaging its former toucher or applying a red cloud remotely.
Queue traversal and single-impact dispatch are separate; triggered-mine resolution
shares classification with enqueueing. Mask contacts with frozen karts
are recorded once per attacker/target and resolve on resumption without physical
collision response. Pickups remain
collectible through collision callbacks. World frame/texture,
vertex-animation and bot-global updates are skipped. Particle updates retain
human kart emissions by live owner-instance identity, not the camera filter;
new particles clear stale pool ownership and tag human-created weapon effects.
Particle integration is bounded to the eleven real axes: ownership metadata in
the flag word must never index past `Particle.axis` into the next pool record.
Split-screen exhaust emission uses the
live frame counter while the world clock is stopped. Human invisibility,
super-engine, received/sent clock, mask and shield/cloud durations stop separately
while human attachments update. Direct human collection and damage to humans
remain immediate. Human weapon impacts on frozen karts, weapons and level objects
are retained in the resident deferred-impact queue and dispatched before world
updates resume. An impacting mine/projectile waits at contact; explosion callbacks
retain their original source until dispatch. Dead participants cannot be recycled
while referenced. Reset discards pending operations; checkpoints relocate their
source, target and BSP pointers.

Audio context tags human FX in `ChannelStats.flags`, preserving ownership through
retail pause backups and quick states. Menu move/back/confirm/locked effects are
always exempt from time stop, including opening and closing pause. Menu sound
matching uses only the low 16-bit sample ID, excluding the instance counter packed
by `OtherFX_Play_LowLevel`. Positional sounds preserve an enclosing human context,
including Wumpa collection after its instance loses its thread; weapon sounds
also inherit live owner identity. Options slider FX/voice previews explicitly
enter human audio context. The active
music slider's Aku/Uka preview alone may advance its sequencer and music channels;
channels resolve their sequence's song-pool slot, leaving race music suspended.
Other music sequencing and frozen FX lifetimes
stop; native audio suspends selected SPU voices and nonhuman XA without advancing
samples or envelopes. Human engine channels and human XA voices remain active.
Weapon owners are initialized before launch audio, and attribution compares owner
addresses against live human karts without dereferencing a recycled payload pointer.
Audio snapshot version 3 includes selective suspension. At the UI ordering-table
boundary, the GPU bridge flushes the completed world viewports and grades the main
target before drawing HUD/menu primitives. Luminance remaps the scene to a saturated
navy/electric-blue and gold/lemon-yellow palette, blended at 80% strength with
the original image and slightly dimmed for readability.
HUD colors are untouched in Original and direct/scaled presentation. Race completion/reset
releases suspension.

`Chaos_PlayTimeEffectSound` preserves human audio context for both time effects.
The crate-break cue follows the clock cue after 200 ms through
`Chaos_TickTimeEffectSound`. Za Warudo additionally plays the retail time-crate
tick (`0x40`) six times at normal pitch and sample lifetime, spaced 192 ms apart,
then stops.
It uses the normal demand-loading path from `KART.HWL` and is
tagged as effect audio so world-audio suspension does not mute it.

## Adventure saves

SLOTS uses native 32-bit little-endian layout. The retail profile and advertised
size remain `0x1600`; the co-op extension is unchanged.

| Payload offset | Contents                                     |
|----------------|----------------------------------------------|
| `0x1600`       | Four 4-byte `AdvMultiplayerSettings` entries |
| `0x1610`       | Four 24-byte `ChaosSettingsSaved` entries    |
| `0x1670`       | 14 unused bytes                              |
| `0x167e`       | Two-byte checksum                            |

Each V1 Chaos entry contains:

| Offset | Contents                                 |
|--------|------------------------------------------|
| `0x00` | `u16 durationSeconds`                    |
| `0x02` | `u16 triggerIntervalSeconds`             |
| `0x04` | `u8 trigger` (`ChaosTrigger`)            |
| `0x05` | `u8 armed`                               |
| `0x06` | 16-byte enabled-effect bitset            |
| `0x16` | `u16 format = 0x0143` (`'C'`, version 1) |

Effect ID `n` maps to bit `n % 8` of byte `n / 8`, least-significant bit first.
Append IDs; never renumber or reuse them. Slugs are stable external identifiers.
The live settings use `bool effects[CHAOS_EFFECT_COUNT]` and derived
`enabledCount`; encoding zeroes unused bits. Unknown bits are ignored on decode
and cleared on rewrite, so new effects start disabled in existing saves.

`armed` distinguishes Chaos from Regular even with an empty selection. Missing
or unsupported format tags decode to unarmed, empty settings. Normal save/load
preserves the selected slot's settings; deleting a slot zeroes them. Only V1 is
supported. Field/trigger encoding changes or growth beyond 128 effects require a
new format and migration; `namespace_Memcard.h` assertions pin the layout.

## Profile and menu integration

- Every New resets the live settings to Regular with all implemented effects
  selected. `expanded_options` controls the creation prompt; loaded files use
  `chaosSettings.armed`. Settings are immutable once the file is created.
- Creation retains choices when navigating Back, including across the character
  selection overlay. Confirm requires a selected effect; this does not reinterpret
  an existing armed, empty file as Regular.
- Armed Adventure files add **Chaos Rules** to hub/race/gem-cup pause menus.
  The shared setup screen is read-only, without Confirm. **Active Effects** lists
  enabled effects, not currently running activations, without ON/OFF controls.
- Custom menu strings live in `RECTMENU_Text.c`; `RECTMENU_GetString` handles
  retail LNG indices and custom IDs starting at `0x4000` for drawing and width.

## Runtime and checkpoints

`Chaos_BeginRace` runs after vehicle teleport in `MainGameStart_Initialize`.
`Chaos_Update` runs before gameplay threads, only after GO in eligible Adventure
races, with no active menu, debug menu or pending load. Pause freezes timers.
Race end, retry teardown and level-load stage 0 clear runtime before driver reuse;
settings and the dedicated `chaosRng` survive. `Chaos_Init(seed)` initializes the
generator once at process startup; the host supplies a seed from SDL's
automatically seeded RNG, before replay restoration;
Adventure saves do not restore it. Checkpoints restore its exact captured state.
Use that RNG for scheduling and
selection rather than retail RNG. Unbreakable's retail weapon interactions can
still consume retail RNG; do not conceal this by restoring it.

Timed activations are ordered, with at most four unique IDs. Reactivation removes the
old entry and appends a fresh one, resetting duration, phase, notice and one-off
behavior. Conflicts use last-activation-wins, as listed in the
[player guide](features/CHAOS.md#effects). Symmetric conflicts are represented once
as pairs; instant effects' one-way replacements remain explicit.
A fifth unrelated timed effect is
rejected; replacement remains possible at capacity. Expiry precedes scheduling,
so new entries receive their full duration.

Instant effects execute once and use a separate four-entry announcement queue,
without entering `active` or consuming timed capacity. Repeats execute again and
refresh their notice; overflow drops the oldest notice. Zero duration is invalid
for both kinds. Selection draws from enabled effects allowed in the current
event with `chaosRng`. Bots-only effects are excluded from relic, crystal,
time-trial and battle events; this rule also applies to publication and incoming
requests. There is no live recipient-availability filter. Application excludes
finished drivers and respects scope; bots-only scope selects slots above the
human count. Event filtering reuses `RACE_FIELD_NO_BOT_MODES` from race setup.

Trigger behavior is described in the [player guide](features/CHAOS.md#triggers).
Interval/Random retain frame overshoot; rejected scheduled events still consume
their gap and selection RNG. External restarts a full gap only after success.
Triggers normally do not wait for earlier effects to expire; Za Warudo suspends
trigger clocks and external polling until time resumes.

Pickup hooks run after full-size crates, loose Wumpa, crystals or CTR letters are
consumed. Weapon-owned rewards, hidden/regrowing crates, crates made solid by Unbreakable, bots
and finished humans do not count. Normal/super turbo pads dispatch after the
retail boost: one captured latch per human prevents repeats until leaving all
pad terrain. Pause freezes latches; race reset clears them.

SDATA checkpoints capture settings, RNG, runtime, creation flow/menu navigation
and pause rows; D230 captures the ordinary chain menus. The checkpoint relocator
repairs standalone menu, pause-return and chain pointers. Immutable catalogue,
titles and row specifications need no snapshot. Do not re-enter setup after
restore: that resets restored navigation. Changed resident layouts reject old
checkpoints; this is separate from Adventure-save compatibility.

Host command/catalogue files are outside checkpoints. Catalogue publication runs
at game initialization and `Chaos_BeginRace`, not per frame. Playback performs no
provider file I/O; external request timing is not recorded.
Published commands are enabled effects applicable to the current eligible race;
hubs publish an empty list. Requests are checked again at execution time.

## Effect integration

Effects target unfinished humans or bots according to their catalogue scope;
hubs are excluded. Apply local modifiers in
activation order without overwriting shared input or snapshotting another
effect's modified constants. Persistent ownership/baselines, if introduced, must
be captured and released before driver destruction; overlay changes alone do not
guarantee teardown.

| Effect                     | Hook and constraints                                                                                                                                                                    |
|----------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Mirror, Mirror | Mirror the resolved axis around the calibrated center. |
| Grounded | Clear resolved manual hop latch/buffer after input resolution; preserve drift, forced jumps and cloud ownership. |
| Totally Sober | Two-second sine wave at 75% of controller range, clamped; phase uses captured activation elapsed time. |
| Fly Me to the Moon / Heavy Metal | Derive gravity from the live constant at 1/4 or 3× before terrain multipliers; preserve jump impulse and terminal velocity. |
| Sunday Driver / Gotta Go Fast | Scale the resolved forward target to 70% without a class-based cap / double forward target and positive acceleration. Preserve reverse and the retail `0x6400` target limit. |
| Hole in the Tank | Clear both fuel timers in the driver-input hook; later retail grants survive until the next application. |
| Fair Mode | Clear inventory, pending launch and roulette before item processing; deployed effects remain active. |
| Wumpa Tax | Remove one credited fruit per crossed game second, at most once per frame; preserve pending HUD awards and stop at zero. No pulse at activation or expiry. |
| Clockblocked | One-off spin and clock audio bypass protection without consuming it; assign full duration to 32-bit `clockReceive`. Retail countdown owns it thereafter, even after a Chaos-only reset. |
| Brown Fire | Subtract twice the live fire-speed bonus while reserves remain, clamping the forward target to zero; retain ordinary fuel/fire bookkeeping. |
| Anxious Mask               | Physical slowing terrain requests retail rescue before checkpoint progress; retail eligibility, destination and penalties remain authoritative.                                         |
| Unboxing Gone Wrong | Direct human contact with a consumed item/Wumpa/time crate creates the retail burst and protection-bypassing damage; retain rewards and weapon-hit behavior. |
| Unbreakable | Full-size item/Wumpa/time crates return the solid-hitbox response before breaking or granting rewards; retain weapon/bot/regrowth behavior. |
| Big Kart Energy | Target `Driver.kartScale = 2`; double models, tires and lateral collision radii, retain ground clearance, add 30% forward speed/jump force. Restore target 1 on deactivation/reset. |

Scale calculations belong in `VehPhysForce.c`; Chaos sets driver targets. Retail
interpolation owns body growth/squash/stretch. Giant-vs-smaller collisions use
retail squish damage with protection, skip normal bounce after a successful
squash and queue the forced hop; equal scales keep ordinary collision.

`Chaos_GiveRandomItem` owns inventory replacement, uniform selection from an
explicit list of distinct Race2 items and mask choice. It passes the resolved
item ID to `VehPhysProc_AssignHeldItem`, extracted from ordinary roulette
finalization to share orb ownership, missile limits and stack quantities.
`Chaos_ClearItems` cancels roulette, cooldown and pending fire input after calling
`VehPhysProc_RemoveHeldItem`. That shared item operation handles removal and
missile/orb ownership, and is also used when ordinary item use completes.
The reusable operations live in `VehPhysProc_Items.c`, included by `VehPhysProc.c`.
Ordinary roulette selection and finalization remain intact.
Use This! is available in relic/crystal events too.
Relic HUDs display granted inventory alongside their time-crate feedback.
Direct mask/shield grants use normal activation without
consuming inventory; renewal reuses the existing mask model or shield layers.
The held item's mask choice is preserved.
The gift and ordinary roulette share `VehPhysProc_PlayItemAcquireSound`, using
the original `ITEMSET_ACQUIRED_*` sound IDs in `VehPhysGeneral.c`. Direct mask grants use
`VehPickupItem_MaskUseWeapon` with no inventory mask choice, allowing
normal renewal and configured mask selection. That power-up helper retains its
normal RNG; uniform inventory selection uses `chaosRng`.
Juiced Up tops up the ordinary pending Wumpa pickup queue;
HUD crediting animates the count and `RB_Player_ModifyWumpa` plays the full-count
chime. Existing pending fruit is counted toward the reward. Crystal challenges
credit Wumpas directly through that helper, keeping their crystal queue intact.

The warpball branch in `VehPickupItem_ShootNow` prepares its routing inline.
Race tracks use their checkpoint path;
crystal/battle arenas and levels without that path immediately start the normal
orb teardown after spawning, without seeking targets or running movement.
Teardown tolerates missing particles.

Reserve Injection brings reserves to at least 9,600 units (ten game seconds),
writing the reward directly, preserving larger balances and using `VehFire_Increment`
at third-drift-boost fire level if no boost remains. Reserve losses share
`VehFire_SetReserves`, whose single Chaos hook protects depletion, damage,
U-turn and rescue resets. `COLL_MOVED_ScrubImpact` skips clearing reserves and
`turbo_outsideTimer` for drivers protected by Too Boosted To Fail.
`VehFire_Increment` applies the original reserve grants directly, without
checking Chaos protection.

Sabotage (ID 22, `sabotage`) filters driver-local resolved brake input through
`Chaos_ApplyInputRules` before braking can clear reserves or affect target speed.
It disables Square and the positive/reverse halves of both vertical sticks,
preserving forward throttle and shared gamepad input. Recipient/activation checks
remain in `RB_Chaos.c`; `RB_ChaosEffects.c` owns the input operation.

This Is Fire (ID 23, `this-is-fire`) upgrades existing turbo at activation and
sets new boost grants to `VEH_PHYS_FORCE_SUPER_TURBO_PAD_FIRE_LEVEL`, using the existing super turbo pad constant.
`VehFire_SetLevel` shares normal fire speed/size assignment between activation
and `VehFire_Increment`; activation does not add reserves or count a boost.
`Chaos_FireLevelForDriver` filters new grants before their normal bookkeeping.
The effect replaces Brown Fire symmetrically. Expiry does not restore a
previous fire tier; earned USF remains subject to normal boost-loss rules.

Hydraulics (ID 24, `hydraulics`) renews a normal hop request in
`Chaos_ApplyDriver` by filling an empty `jump_TenBuffer` for the current physics
step. It preserves genuine input buffers and never changes shared pad input or
simulates a held shoulder. Normal jump/friction checks own eligibility and force,
so there is no extra delay, airborne impulse or cooldown bypass. Requests also
apply while drifting; damage states and jump-lockout clouds do not receive them.
Hydraulics and Grounded replace each other.

My Planet Needs Me (ID 25, `my-planet-needs-me`) adds one world-up impulse at
six times the effective normal jump force, including while airborne. It uses
the existing speed/vector converters and synchronizes the working velocity,
preserving horizontal momentum, inventory and reserves. It bypasses normal-hop
eligibility without changing cooldowns or queuing a repeated jump. Activation
removes Grounded/Hydraulics and preserves gravity modifiers. Before conversion,
the vertical component is bounded by the remaining length available in signed
16-bit `Driver.speed`, keeping the full vector at or below `INT16_MAX`. Horizontal
momentum is retained unless its reconstruction alone exceeds that limit through
fixed-point rounding or the negative-speed endpoint; only then is it normalized
to fit. Impulse/length calculations use 64-bit intermediates. Overflow regressions
exercise the real speed converter and square root across headings, rising/falling
motion, large jump forces and repeated launches; angle reconstruction uses a
math-backed test double.

Who Bound This? (ID 26, `who-bound-this`) keeps independent per-human permutations
and analog edge history in `ChaosRuntime`. `Chaos_ApplyInputRules` remaps fresh
local pad copies before applying Mirror, Mirror / Totally Sober and Sabotage. Driving
physics, drift entry and drift updates all use the same logical controls;
shared pad/menu input remains unchanged. Analog throttle/reverse become mapped
button actions while the effect is active. Grounded/Hydraulics stay in
`Chaos_ApplyDriver`, after normal jump-input resolution.

Try Something New (ID 27, `try-something-new`) is a one-off class change lasting
until race end. The runtime remembers the selected standard class so repeats
exclude the live class rather than always excluding the character baseline.
`VehBirth_SetConstsForClass` shares the ordinary metaphysics table loader and
configured multipliers; `VehBirth_SetConsts` still selects the character's class
for normal birth. Both live in `VehBirth_Consts.c`, allowing focused tests to use
the real loader without spawning vehicles. Changing class leaves characters,
shared metadata and current race state intact. Race teardown clears the selected
class bookkeeping; normal vehicle birth restores the character's class.
Class changes also stop the previous engine loop and start the selected class's
loop. `Chaos_EngineClassForDriver` keeps engine updates, audio initialization and
warp teardown on that same class, without overriding ghosts that share a player
ID. Reset restores character constants and the human engine loop before clearing
the override, including class changes left on a bot-controlled kart by Body Swap.

Identity Crisis (ID 34, `identity-crisis`) changes human identities once until
race end. Eligible races with the effect enabled reuse resident character models
through `VehBirth_GetModelByName` and preload missing high-detail models at load
stage 6 through the synchronous relocating DRAM loader;
the models live below the race-retry MEMPACK bookmark. SDATA stores their pointers
and each human's first original model/character; checkpoint restoration relocates
these pointers. Activation uses Chaos RNG to select a resident character absent
from live driver slots and pending transformations. Eight growing retail burn-smoke
particles follow each kart in all viewports, scaled through `VehPhysForce_ScaleParticle`.
After 100 ms of eligible race time, the existing instance, animation bounds and
tires switch to the pending character. The retail emitter owns smoke fade/lifetime
(about 0.3 seconds). Pause freezes the delay; finished humans cancel it and reset
discards it. Particle pool exhaustion does not prevent the model change.
`Chaos_SetDrivingClass` shares class/audio updates with Try Something New, so the
latest change wins: class/audio update at activation, while only the model waits
for smoke coverage. Reset restores original identities before resetting engine
audio; ordinary birth restores class constants. No asset reads occur on activation.
Activation plays the retail invisibility sound once, shared by all recipients.

Body Swap (ID 35, `body-swap`) is available only with at least two human players.
It stores symmetric controller-slot partners in
`ChaosRuntime`. Operations exchange `GameTracker.drivers`, driver IDs and character
IDs rather than copying/recreating karts, preserving weapon ownership and physical
race state. Effective class overrides and Identity Crisis baselines move with the
kart; their arrays cover all eight slots. Input shuffles and timed driving modifiers
remain player-owned. Size and existing USF overrides refresh on the newly controlled
humans, and former human karts return to normal size under AI control.

Human/bot swaps move the existing root threads between PLAYER/ROBOT buckets.
`VehPhysProc_TakeControl` installs human physics without reapplying impact penalties;
`BOTS_Driver_Resume` shares the retail conversion core without recording race results
or moving battle racers to their starting point. It reconnects navigation near the
current position and preserves reserves, damage timers and motion. The ordinary
end-of-race converter retains its existing behavior. Navigation-list membership,
thread model IDs and tick callbacks follow the new controller. Camera targets and
engine loops update per human, using the existing one-shot camera recovery blend.

Activation skips finished drivers. Ownership and camera switch immediately regardless
of kart state; expiry/reactivation also returns pairs without waiting for rescues or
start holds. Bot takeover seeds the human anti-cut baseline from live progress,
discards spawn-only surface references and converts world motion to human polar
speed. An existing bot rescue retains its mask/descent callback under human ownership,
then installs human driving on landing; return to AI reuses an existing rescue mask.
Human takeover clears AI-only wrapped yaw lag and its human recovery velocity.
Camera retargeting invalidates the previous body's damage/rescue/height smoothing
and requests direction reinitialization before the one-shot blend; zoom stays player-owned.
Reset returns all pairs before class and identity teardown.
`PlayLevel_UpdateLapStats` returns a pair on either kart's
last-lap near-finish segment, before evaluating lap completion, using the retail
near-line distance. Crystal/battle events have no such segment. Partner slots and
class/identity ownership are checkpoint state. Asset-free tests cover ownership,
pairing, lifecycle and the real human takeover's progress/motion transfer; driving
callbacks and AI conversion are doubled.

Yardrat Training (ID 36, `yardrat-training`) is instant and human-only, excluded from battle and
crystal challenges. It advances one tenth of the lap's retail progress distance,
interpolating checkpoint positions along the selected route. It reuses
`RB_Warpball_NewPathNode`, extracted unchanged into `RB_Warpball_Path.c` so the
asset-free suites also exercise retail branch selection. An unknown branch uses
the main route; traversal is bounded by the checkpoint count.

`Chaos_RepositionDriver` probes destination ground before mutating the driver.
No ground means no movement. A successful placement refreshes surfaces, terrain,
checkpoint/anti-cut state, previous position and orientation, preserving speed,
reserves and held items. Existing rescues/start holds are released; ordinary
damage callbacks remain installed. Motion is realigned to the destination yaw,
and the shared camera handover requests one recovery blend.

`ChaosRuntime.teleportForwardCrossings` queues skipped finish crossings for
`PlayLevel_UpdateLapStats`, preserving retail lap times, start-line debt, final-lap
announcements and results. Crossings are consumed once and follow the physical
kart through Body Swap; a pending final crossing returns its pair before results.
Exact-line destinations use the subsequent physical crossing. Runtime checkpoints
capture pending crossings and the shared `cameraBlendPending` array.
Tests run the real route selection, repositioning and lap/results flow with world
collision, matrix construction and end-screen setup doubled. Focused geometry
checks also run the real progress projection and vector normalization through
the native GTE on a circuit with coherent world/progress distances, covering
placement, subsequent frames, finish wrapping and alternate-branch surfaces.

Defective Teleport (ID 37, `defective-teleport`) is instant, human-only and uses
the same event exclusions. It samples free 3D space rather than
moving backward along checkpoints: the rear hemisphere relative to kart yaw,
with radius one fifth of the retail lap length in world units and a fixed vertical
offset cap of 256 world units. Chaos RNG samples independent local
backward/lateral/vertical offsets. Destinations must stay inside the BSP root's
XZ footprint and inside a nonempty geometry leaf's bounding box expanded by 50
world units horizontally and 256 vertically. This rejects empty space between
distant track areas without filtering water, off-road or kill-plane surfaces. Rejected candidates are
retried up to 64 times, then a vertical-only fallback is checked against the same
bounds; no valid fallback means no movement. No ground snap is applied.

Both teleports use `Chaos_MoveDriver` for hold/rescue release, pose/motion,
camera recovery, turbo-pad latch invalidation and invisibility audio. Ordinary
Yardrat Training then installs its destination surfaces/progress. Defective Teleport
keeps heading and canonical speed, clears current contacts/terrain, marks the
kart airborne and retains `lastValid` for retail mask rescue. Normal physics and
rescue rules can subsequently change speed or reserves.

Progress, checkpoint validation and mask saves use the ordinary retail rules;
Defective Teleport adds no progress suspension or artificial lap displacement.
Regression checks cover 3D range/headings, geometry bounds, unsafe placements,
repeatability, rescue anchors and lifecycle gates.

Small Kart Energy (ID 28, `small-kart-energy`) uses the same size pipeline as
Big Kart Energy. `Driver.kartScale` is a float multiplier (zero defaults to 1×),
resolved by `VehPhysForce_GetKartScale`; finished drivers use 1×. Visual scales,
tires and contact radii convert to integer dimensions through
`VehPhysForce_ScaleValue`. Squash compares the floating-point multipliers without
integer truncation and reuses normal damage protections. Small size is 0.5×;
forward speed targets and jump force are 0.7×. Size effects replace one another,
and expiry/reset restore normal size, including for finished humans.
Ground shadows scale their local footprint through the same helper, preserving
the normal height-based shrink/fade. Skid emission scales all four tire contact
positions, strip widths and lateral offsets before storing world-space vertices;
existing skid history keeps the size at which it was emitted. Both paths follow
`kartScale` directly, rather than checking individual Chaos effects.
Kart particles use `VehPhysForce_ScaleParticle` at emission, before adding world
positions. It scales spatial axes, growth and spatial oscillators together while
preserving angles, color and timing. Exhaust, burn smoke, terrain/wall/ground
sparks, mud and rescue/water particles share this path. `VehPhysForce_ScaleMatrix`
scales freshly rebuilt effect geometry for turbo flames, shields and masks;
attachment offsets are scaled separately from world positions. Water wake size,
revving animation and wall-crash scale resets also follow the kart multiplier.
TNT remains its original size: only its attachment offset follows the kart, so
it rests on the resized character's head without changing its animation/state.

Classic GTA (ID 29, `classic-gta`) overrides the final pose in `CAM_ThTick` after
normal camera processing, preserving the live mode and zoom state. Each human
viewport looks vertically down from `CHAOS_TOP_DOWN_HEIGHT` (2,560 world units), following kart yaw.
The driver's ground/contact block supplies visibility; only missing ground data
requires a driver-position camera probe. `Chaos_TopDownDriver` gates presentation
independently of simulation pause, so pause menus retain the camera/cutaway.
`Chaos_ApplyLevelVisibilityRules` runs after normal PVS processing and enables all
leaf/face mask bits for that viewport, leaving geometry selection to the overhead
frustum instead of the ground-level PVS. It invalidates the decoded-source caches
so expiry reloads retail masks even when the driver stays on the same block.
The four shared level BSP-list workers filter ceiling blocks before emission,
consuming visibility bits normally. Classification uses the high-detail triangle
topology and normal winding from collision, with 64-bit X/Z cross products.
Blocks with downward-facing triangles and no upward-facing triangles are omitted;
any road-facing triangle preserves the whole block. Vertical panels and the
driver's supporting/contact blocks remain. Triangular blocks use only their four
valid high-detail triangles. This avoids tile-boundary and driver-height toggles
and preserves unflagged ramps and mixed floor/ceiling geometry. Static level
models whose projected bounds cover the kart are culled before OT allocation if
they extend above the cutaway plane. Threaded gameplay objects are preserved.
These are per-viewport whole-object cutaways; they do not change collision data
or shared flags, and do not require checkpoint-owned camera state.

Arena Champions (ID 30, `arena-champions`) uses bots-only scope. Normal
rank/distance/leader-stat rubber-banding, boost, terrain and damage rules still
run. `Chaos_SpeedForDriver` applies a 175% multiplier to the final navigation
speed and horizontal velocity. AI dynamics stay in retail
units: scaling the output scales actual acceleration too, keeps existing caps
and airborne square calculations valid, and avoids accumulating multipliers.
The 16-bit speed/reporting mirrors saturate rather than wrapping; navigation and
32-bit horizontal velocity keep the full speed. Vertical jump/gravity and impact
offsets retain normal behavior. There is no saved baseline or teardown mutation.
Event filtering is static and never inspects whether any bot is still racing.

You've Got Mail (ID 31, `youve-got-mail`) is an instant bot-scoped action.
The last unfinished bot launches one normal orb at the highest-placed unfinished
human through the shared item dispatcher, preserving inventory. The fixed-target
tracker flag bypasses rank-based retargeting and the incidental-leader teardown;
source Wumpas do not power it up. A finished target fades the orb. Orb teardown
and inventory removal share ownership refresh across held and deployed orbs.

Fresh Start (ID 32, `fresh-start`) stores per-human countdown, pending/active
flags, light history, suspension, temporary mask and camera-blend request in
`ChaosRestart`. Existing rescues finish, including descent, before local entry
into `VehStuckProc_RevEngine_Init`. Pause freezes the countdown; repeat activation
restarts it and reset releases the hold/mask.

Entry clears motion, jump requests, fire and reserves, bypassing reserve protection
without changing inventory or progress. Neutralization aligns physical heading
with visual yaw and clears drift/wobble/camera-lag offsets. Suspended karts share
`VehStuckProc_MaskGrab_SetFallPose` and use an independent mask that expires at
green/reset without renewing inventory power-ups.

Revving reads `Chaos_StartTimerForDriver`; driving re-entry uses only
`Chaos_IsWaitingForStart`, including after damage. Other drivers retain the global
timer. `DotLights_Sequence` shares retail timing, audio and starting boosts without
mutating that timer; requested Chaos duration does not affect the countdown.
Synchronized players share light audio. Rendering and HUD reservations use the
same animated `DotLights_GetBounds`, alongside Final Lap bounds.

`Chaos_ConsumeCameraBlend` requests the normal follow camera's eight-frame
damage-recovery interpolation for eye and look-at positions once at countdown
entry. Freecam and end-of-race cameras do not consume it.

Senile Mask (ID 33, `senile-mask`) dispatches false warnings only to unfinished
humans without `ACTION_DRIVING_WRONG_WAY`, leaving direction/progression untouched.
`Chaos_OverridesWrongWayWarning` replaces the retail HUD/voice paths while the
effect is active; its own paths warn only in the correct direction.
`UI_DrawWrongWay` shares blink timing, text and
viewport positioning; the Chaos HUD also supplies this path in crystal events.
`Voiceline_PlayWrongWay` shares mask choice and playback with the retail warning.
After normal voice-enable/cooldown/XA gates, Chaos repeats it with half the
track-length-plus-delay cooldown at normal voice volume. In 2P,
the first eligible human supplies the mask voice; the shared XA stream never
overlaps clips. No additional Chaos runtime state or user-volume changes are needed.

## HUD and audio

Each successful activation plays the preloaded double-gain transition swish and
starts a three-second announcement; timed effects then get 320 ms docking.
Instant notices expire without docking. Rejected requests
and redraws do neither. Race audio teardown releases level-local cached samples;
preloading avoids synchronous reads at activation.

The newest banner starts at the standard race-message baseline. Older banners
stack down in 1P and up in 2P, avoiding Final Lap. A separate right-hand table
shows at most four docked effects at 50% small-font scale, newest first. Docked
rows use UI OT slot 1 behind lap statistics; Final Lap is submitted after the HUD.
Pause freezes/hides announcements, menus hide them and all-human finish hides
them before results setup. Reset clears them.
