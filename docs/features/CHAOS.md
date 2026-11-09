# Chaos mode

Chaos adds temporary gameplay effects to Adventure: reversed steering, giant
karts, explosive crates and more. You choose which effects can appear and what
triggers them when creating the Adventure file.

It works in solo Adventure and [2P co-op Adventure](2P_ADVENTURE.md).
The hub is unaffected; Chaos runs in Adventure events,
including trophy, boss, CTR, relic, crystal and gem-cup challenges.

## Starting a Chaos file

1. Enable **Expanded Options** under **Adventure** in the in-game **Options Config**
   menu, or set `expanded_options = true` in the `[Adventure]` section of `config.ini`.
2. Start a **New Adventure** and choose **Chaos** at the mode prompt. Choose
   **Regular** to create an ordinary Adventure instead.
3. Configure Chaos, select **Confirm**, then continue to character selection.

![New Adventure mode selection showing Chaos and Regular](images/chaos-mode-choice.jpg)

The global option only enables this creation prompt. Your choice and Chaos rules
are stored in the Adventure save slot. Loading a Chaos file does not show setup
again, and turning the global option off does not disable Chaos in that file.
Existing Regular files stay Regular.

## Choosing the rules

| Setting | Meaning |
| --- | --- |
| **Duration** | How long timed effects last: 5–60 seconds, in 5-second steps. Default: 30 seconds. Instant effects apply once. |
| **Trigger** | What causes the next activation; see below. Default: Random. |
| **Interval** | Gap used by Random, Interval and External: 5–120 seconds, in 5-second steps. Default: 35 seconds. Hidden for Pickup. |
| **Effects** | Open the list and turn individual effects ON or OFF. All are selected initially. |
| **Confirm** | Accept these rules. At least one effect must be selected. |

![Chaos setup with Duration, Trigger, Interval, Effects and Confirm](images/chaos-setup.jpg)

Use Up/Down to navigate, Left/Right to adjust numeric values, and Cross/Circle to
cycle the trigger, open Effects or toggle an effect. Triangle returns from the
effect list to setup. These refer to the game's mapped controller buttons.

![Chaos effect selection with individual ON and OFF toggles](images/chaos-effect-selection.jpg)

**Rules are fixed once the file is created.** To play with another trigger,
duration, interval or selection, create another Adventure file.

## Triggers

### Interval

After the configured interval, activate one randomly selected enabled effect.
Repeat at that interval while the race is running. The first interval starts
from GO.

### Random

Like Interval, but each gap is randomly chosen between **30% and 100%** of your
Interval setting, including the first gap. With Interval set to 35 seconds, the
next effect arrives after approximately 10.5–35 seconds of active race time.

### Pickup

Activate one randomly selected enabled effect when a human:

- consumes an item, Wumpa or time crate;
- collects loose Wumpa, a crystal or a CTR letter;
- enters a normal or super turbo pad.

### External

An external application chooses the effect by placing its name in a local file.
The game waits one full Interval from GO, then reads the next request. Each
successful activation starts a fresh interval. With no request, no effect starts.

Requests wait through pauses and Za Warudo's time stop, and can carry over to the
next race, which starts a new interval from GO. The selected effect uses your saved
Duration and must be enabled in the file. Setup and a manual submission example are in the
[external provider guide](CHAOS_EXTERNAL_PROVIDER.md).

## Effects

| Effect                  | What happens                                                                                                                                                                      |
|-------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| **Mirror, Mirror** | Left and right steering are reversed. |
| **Grounded** | Manual hopping is disabled. Existing drifts and forced jumps can still happen. |
| **Totally Sober** | Steering sways from side to side, making a straight line harder to hold. |
| **Fly Me to the Moon** | Gravity falls to one quarter of normal, keeping you airborne longer. |
| **Heavy Metal** | Gravity triples, bringing you down sooner after jumps. |
| **Sunday Driver** | Reduce forward target speed to 70%, including boosted speeds, without a class-based speed cap. |
| **Gotta Go Fast** | Doubles forward target speed and acceleration. The game's usual speed ceiling still applies. |
| **Hole in the Tank** | Continually clears stored boost fuel, preventing you from building up reserves. |
| **Fair Mode** | Empties held items and cancels item rolls. Already deployed items, shields and masks remain active. |
| **Unboxing Gone Wrong** | Item, Wumpa and time crates explode when you drive into and break them. The blast can hurt you through mask or shield protection. |
| **Wumpa Tax** | Removes one collected Wumpa per second until you reach zero. |
| **Clockblocked** | Applies an initial spin hit and clock slowdown. The initial hit bypasses mask and shield protection. |
| **Brown Fire** | While you have reserves, the fire-speed bonus becomes a penalty instead. |
| **Anxious Mask**        | Contact with slowing terrain, such as off-road surfaces, requests a normal mask rescue back to a checkpoint. Normal rescue penalties apply.                                       |
| **Unbreakable** | Item, Wumpa and time crates become solid obstacles when you drive into them, instead of breaking and giving rewards. |
| **Big Kart Energy** | Doubles kart size and lateral collision size, adds 30% to forward target speed and jump force, and lets larger karts squash smaller ones. Protected karts can resist squashing. |
| **Use This!**           | Replace each human's item with a random one.                                                                                                                                      |
| **Juiced Up**           | Fill Wumpas to ten.                                                                                                                                                               |
| **It is I**             | Activate or renew a normal mask without consuming inventory, using normal Wumpa and duration rules.                                                                               |
| **Providence**          | Activate or renew a normal shield without consuming inventory, using normal Wumpa, color and duration rules.                                                                      |
| **Reserve Injection**   | Bring reserves to at least ten seconds; give third-drift-boost fire if not already boosting. Preserve an existing boost and any larger reserve balance.                           |
| **Too Boosted TO Fail** | Boosts can add reserves, but consumption, damage, U-turns and rescues cannot reduce them while the effect lasts.                                                                  |
| **Sabotage**            | Disable braking and reverse, including airborne braking and analog brake/reverse input. Forward throttle remains available.                                                       |
| **This Is Fire**        | Upgrade existing turbo and new boosts to USF without free reserves. Earned USF remains after expiry until normal gameplay removes it.                                             |
| **Hydraulics**          | Automatically hop with normal force as soon as normal jump rules allow, including while drifting.                                                                                 |
| **My Planet Needs Me**  | Instantly add an upward impulse up to 6× normal jump force, even in midair. The launch is limited by the kart's total speed capacity to prevent overflow. Gravity effects still apply. |
| **Who Bound This?** | Randomize throttle, brake, item and both hop/drift controls independently per human for the effect's duration. Steering, pause and menus remain unchanged.                        |
| **Try Something New** | Switch to a different standard driving class and its engine sound until race end, keeping the character. Repeats select a class different from the current one.                   |
| **Small Kart Energy** | Halve kart dimensions and collision size; reduce forward speed and jump force to 70%. Larger karts can squash you, subject to normal mask, shield and immunity protections.       |
| **Classic GTA** | Use a vertical top-down camera that turns with your kart. Nearby overhead scenery is cut away to reveal the road, including in tunnels. Each human viewport follows its own kart. |
| **Arena Champions** | Increase bot driving speed and acceleration to 175%, retaining normal rubber-banding. Human karts are unaffected. Available in events with bot racing. |
| **You've Got Mail** | The last-place unfinished bot launches a normal orb at the leading unfinished human. Its target stays fixed, and the bot keeps its inventory. Available in events with bot racing. |
| **Fresh Start** | Stop humans, clear reserves and replay the starting lights with starting boost available. Airborne karts are held aloft in the rescue falling pose by an orbiting mask; other karts return to neutral. An ongoing rescue finishes before the lights start. Bots keep racing. |
| **Senile Mask** | Flash the wrong-way warning only while humans drive in the correct direction; suppress the warning and voice when actually going the wrong way. The mask voice repeats with half its normal cooldown, without overlapping speech. Race progress is unaffected. |
| **Identity Crisis** | Switch each human to a random playable character not currently present in the race, regardless of unlocks, with that character's driving class and engine sound until race end. A brief smoke poof and the invisibility sound accompany the transformation. Preserve race state and inventory. The latest class change from this effect or Try Something New wins. |
| **Body Swap** | Available with at least two human players. Temporarily exchange karts with another human or bot in random disjoint pairs. Take the kart's character, current class, inventory, damage and race progress; human driving modifiers follow the player. Control and camera switch immediately, including during rescues. Return at expiry or on either kart's final finish-line approach. |
| **Yardrat Training** | Instantly move each unfinished human forward by 10% of a lap along the track path, following the selected branch or the main route. Preserve speed, reserves and held items; face the destination route and realign the camera. Finish-line crossings count normally, including completing the race. Unavailable in battle and crystal challenges. |
| **Defective Teleport** | Instantly move each unfinished human to a random point in the space behind their kart, also sideways, above or below. The maximum 3D displacement is 20% of the lap length, with a vertical offset limited to 256 world units. Destinations stay within the level footprint and near track geometry; they do not follow the racing line. Water, off-road positions and normal mask saves are allowed. Preserve speed, reserves and inventory at teleport time; subsequent landings and rescues use normal game rules. Keep the heading, realign the camera and play the invisibility sound. Unavailable in battle and crystal challenges. |
| **Za Warudo** | Stop time for up to ten seconds while humans keep driving. Bots, hazards, world animations/particles and nonhuman weapons freeze. Karts can pass through each other; touched Nitro and potions also allow passage and detonate only when time resumes, affecting karts within their respective blast/contact radius at that moment. Other hazards retain their collisions. Human weapons keep moving, but their impacts on frozen entities take effect when time resumes. Human masks, boost flames and kart smoke keep animating; crates and pickups remain collectible. Race clocks, item-effect durations and other Chaos timers stop. Blue/yellow scene colors leave the HUD unchanged. Clock/time-crate cues and six ticks mark the effect; music and world/bot sounds pause while human pickup, launch and weapon sounds remain active. |
| **King Crimson** | Instantly skip ten seconds: advance unfinished bots along the track according to their current speed and add ten seconds to the race clock. Human positions, weapons, power-ups and other effect timers do not advance through the skipped interval. Bots can complete laps and finish, with normal finish order/results handling. A half-second blue/yellow world palette and clock/time-crate cues mark the skip. Available only in events that support racing bots. Replaces Za Warudo; Za Warudo also cancels its short presentation. |

Effects can overlap: Duration does not delay the next trigger. For example,
Duration 30 seconds with Interval 10 seconds allows several effects at once.

- Up to **four distinct timed effects** can be active. A fifth unrelated timed effect is
  ignored; refreshing or replacing an existing effect is still possible.
- Triggering an active effect starts its duration over and announces it again;
  it does not multiply that effect's strength.
- These pairs replace one another: **Mirror, Mirror / Totally Sober**,
  **Fly Me to the Moon / Heavy Metal**, **Sunday Driver / Gotta Go Fast**,
  **Hole in the Tank / Too Boosted TO Fail**, **This Is Fire / Brown Fire**,
  **Grounded / Hydraulics**, **Big Kart Energy / Small Kart Energy**, and
  **Za Warudo / King Crimson**.
- **Use This!** ends **Fair Mode**; **Reserve Injection** ends **Hole in the Tank**.
- **My Planet Needs Me** ends **Grounded** and **Hydraulics** before launching.
- Instant effects (**Use This!**, **Juiced Up**, **It is I**,
  **Providence**, **Reserve Injection**, **My Planet Needs Me**, **Try Something New**,
  **You've Got Mail**, **Fresh Start**, **Identity Crisis**, **Yardrat Training**,
  **Defective Teleport**, **King Crimson**) take no timed slot.
  Repeating one applies it again.

## During a race

A sound and on-screen banner announce each activation. After a few seconds, its
name moves into the smaller list on the right. Instant effects show only the
initial banner and never dock, even when their gameplay result persists.

![Chaos activation banner and effect list during a race](images/chaos-race-hud.jpg)

Chaos timers stop in pause menus and outside active races. During Za Warudo,
only its own expiry advances; other Chaos timers remain suspended. King Crimson
advances the race clock without consuming other effect durations.

To inspect those rules, pause and choose **Chaos Rules**.

![Read-only Chaos Rules screen opened from the pause menu](images/chaos-pause-rules.jpg)

## Saved games

Normal Adventure saves retain your Chaos rules, not an in-progress set of effect
timers. Quick states also capture the current race and active effects.
