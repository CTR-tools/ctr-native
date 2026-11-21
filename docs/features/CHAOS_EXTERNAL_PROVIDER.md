# Chaos external file provider

The game's External trigger accepts one effect request at a time by reading an
externally written file.

External supports Shared distribution only. Per player publishes an empty
catalogue and does not consume commands; personal external targeting is not supported.
In Shared sessions, the Players selection filters human-scoped effects, including
provider requests. Bot/world effects remain global. Body Swap is omitted from the
catalogue if the selected players and bot grid cannot form a pair.

## Files and format

| File            | Owner             | Contents                                      |
|-----------------|-------------------|-----------------------------------------------|
| `chaos/effects` | Game              | One enabled, event-applicable effect slug per line |
| `chaos/command` | External producer | One effect slug, e.g. `fly-me-to-the-moon` |

Final files have no extension. Use ASCII or UTF-8 without a BOM. A command may
have surrounding whitespace but no verbs, duration or extra arguments. It must
be shorter than 128 bytes; oversized input and embedded NUL bytes are rejected.

The game creates `chaos/` and atomically publishes `effects` via `effects.tmp`
at game initialization and in `Chaos_BeginRace()`, never in frame updates.
The list reflects the Adventure save's or Arcade/VS/Battle session's selection
and current event for any trigger; it is
empty outside eligible races, when Chaos is unarmed or when no selected effect
applies. It is not a readiness signal or heartbeat. Execution validates
eligibility again, including whether an unfinished target remains.
Publication failures are logged; the next initialization attempts publication
again, without frame retries.

## Producer sequence

1. Read `effects` and choose an available slug.
2. Wait until `command` is absent. Use one producer; never overwrite a pending request.
3. Write `command.tmp`, close it, then atomically rename it to `command` in the
   same directory. The game ignores the temporary file.
4. Wait for `command` to disappear before submitting the next request.

PowerShell example, run from the asset base directory:

```powershell
if (-not (Test-Path -LiteralPath 'chaos/command')) {
    [System.IO.File]::WriteAllText(
        (Join-Path (Get-Location) 'chaos/command.tmp'),
        'fly-me-to-the-moon',
        [System.Text.Encoding]::ASCII)
    Move-Item -LiteralPath 'chaos/command.tmp' -Destination 'chaos/command'
}
```

## Timing and consumption

- Requires armed Shared Chaos rules in an eligible Adventure or Arcade/VS/Battle
  event with trigger **External**.
- Wait one saved interval from GO, then one full interval after each successful
  activation. Only eligible active race time counts; pause, menus, countdown,
  loading, hubs, results and Za Warudo's time stop neither spend the interval nor
  consume commands. A command submitted during time stop waits until time resumes.
- Once ready, check at most every 100 ms on normal frames. An empty slot leaves
  the provider ready; an interval of zero permits a request on the next eligible poll.
- Pending commands survive race changes; each new race waits a fresh interval.
- Effects use the saved duration and ordinary overlap/replacement rules.

Read, close and delete the file before activation. Read/open/delete errors retain
it for retry; errors other than file absence are logged. Invalid, unknown,
disabled or runtime-rejected commands are consumed and logged without restarting
the interval. **Deletion acknowledges consumption, not successful activation.**
There is no result file or outbox.

The external voting timer is independent of gameplay time; a submitted winner
waits through pause or an unfinished interval. Exact overlay synchronization
would require an additional readiness/timing channel.

Playback neither consumes commands nor publishes catalogues. Requests are not
recorded, so provider-driven replay verification is not guaranteed. Files are
outside quick states: restoring a state does not restore a consumed command.
