# AI usage panel widget (xfce4)

An xfce4 panel widget showing Claude Code and Codex CLI quota usage at a
glance - real account meters for both, including Claude's per-model weekly cap
- so you can see whether you are near a limit without opening a terminal. Click it for the full `claude-usage` / `codex-usage`
reports.

By default it draws a small **icon**: one vertical gauge per window, about 51px
wide in total. The numbers live in the tooltip.

```
 F  W  5   X
 └──┴──┘   └── Codex, its window closest to the limit
    │
    └── Claude:  F = weekly cap for that model (Fable here)
                 W = weekly cap, all models
                 5 = the rolling 5-hour window
```

Claude's three windows run out independently - you can be at 21% of the 5h
window and 78% of the week while being completely out of Fable - so collapsing
them to a single worst-case gauge hides which one you are actually up against.
The wider gap separates the providers.

**The gauges drain like a battery.** A full bar means plenty of quota left; it
empties as you use it up:

```
 ┌─┐   ┌─┐   ┌─┐   ┌─┐   ┏━┓
 │▓│   │ │   │ │   │ │   ┃ ┃
 │▓│   │▓│   │ │   │▒│   ┃ ┃
 └─┘   └─┘   └─┘   └─┘   ┗━┛
 0%    50%   70%   ~99%   100% used
 green green amber amber  empty, red outline
```

Filling them with the *used* fraction instead reads exactly backwards: a
nearly-empty bar then means "barely touched", which looks like trouble at a
glance. The fill is headroom remaining; the colour still comes from how much is
used (green `<60%`, amber `>=60%`, red `>=85%`, grey = stale).

A sliver stays visible while any headroom remains, so "almost out" is still
distinguishable from "completely out". A gauge with *nothing* left has no
coloured pixels of its own, which would make being out of quota look exactly
like having no data - so it gets a thick outline in its own colour instead. A
provider with no data reads as empty with the usual thin grey outline, never as
full.

### Text mode

`AI_USAGE_TEXT=1` swaps the icon for the original wide textual form:

```
C ▰▰▰▱▱ 58%wk:Fable   X ▰▱▱▱▱ 4%wk
│ │     │  │            └── window it applies to (5h / wk / wk:<model>)
│ │     │  └─────────────── percent of that limit used
│ │     └────────────────── "~" = estimate, not a real meter (Claude only)
│ └────────────────────────── battery-style gauge, 5 cells
└──────────────────────────── C = Claude, X = Codex
```

It shows only each provider's worst window - one line per provider, not per
window - and is far wider even so: a couple of hundred pixels against the
icon's ~51. Hence it is no longer the default.

The colour defaults are saturated mid-tones rather than pastels, because the
original gruvbox palette (`#8ec07c` green, `#fabd2f` amber) was close to
unreadable on a light panel background. All of them are overridable - see
Tuning.

## Install

```sh
sudo apt install xfce4-genmon-plugin      # once
./install.sh                              # add to the panel
./install.sh --remove                     # take it out again
```

`install.sh` backs up `xfce4-panel.xml` before touching anything, and is safe
to re-run (it will not add a second copy).

## Files

| file | role |
|---|---|
| `ai_usage.py` | collects both providers into one dict |
| `claude_quota.py` | Claude's real meters from the OAuth usage endpoint |
| `ai-usage-refresh` | the slow scan (~8s); writes the cache |
| `ai-usage-genmon` | what the panel runs (~0.05s); reads the cache |
| `ai-usage-detail` | click action; opens a terminal |
| `ai-usage-show` | what that terminal runs |
| `install.sh` | add/remove the panel plugin |

Cache lives in `~/.cache/ai-usage/status.json`.

## How it works

`ai_usage.py` imports `~/bin/claude-usage` and `~/bin/codex-usage` and calls
their `scan()` directly rather than screen-scraping their output. Both guard
`main()` with `__name__ == "__main__"`, so they import cleanly; they have no
`.py` extension, so the import needs an explicit `SourceFileLoader`.

A full scan takes about **6 seconds** (codex ~6s, plus one HTTP request for
Claude - only if that request fails does the ~2s local claude scan run) - far too slow to
run inside a panel poll, which blocks the panel while it runs. So the widget is
split: `ai-usage-genmon` only ever reads the cache (~0.05s) and, when the cache
is older than 90s, spawns `ai-usage-refresh` detached. The panel shows the last
known values meanwhile, greyed out once they pass 10 minutes old.

## Where the numbers come from

Both providers now report real account meters; the widget shows whichever
window of a provider is closest to its limit.

**Codex** publishes its meter in the session files (`rate_limits.primary` /
`.secondary`, each with `used_percent`, `window_minutes`, `resets_at`), so the
percentage and reset time are read straight off disk.

**Claude** stores nothing locally, but the endpoint behind `/usage` has it:
`claude_quota.py` does a `GET https://api.anthropic.com/api/oauth/usage` with
the OAuth token Claude Code already keeps in `~/.claude/.credentials.json`. It
never refreshes that token - the refresh token belongs to Claude Code, and
racing it could invalidate the session you are sitting in.

The useful part of the response is `limits`, one entry per window:

| kind | shown as | what it is |
|---|---|---|
| `session` | `5h` | the rolling 5-hour window |
| `weekly_all` | `weekly` | the weekly cap across all models |
| `weekly_scoped` | `weekly:Fable` | the weekly cap for one model |

The scoped window is the one that matters most: you can be at 21% of the 5h
window and 78% of the week while being **completely out of Fable**. The flat
`five_hour`/`seven_day` keys in the same response cannot express that, so they
are only a fallback.

### The estimate fallback

If the call fails (no token, expired token -> `HTTP 401`, offline), the widget
falls back to `claude-usage`'s local estimate: token usage over the trailing 5h
as a share of a calibrated or guessed ceiling. The tooltip then says
`meter unavailable (...)` above a line marked `5h ESTIMATE, this machine only`,
and the panel figure carries a `~`.

The estimate is a rough floor, not a reading: it sees only this machine, not
other devices or claude.ai, and has no notion of the weekly windows at all.
Calibrate it with the 5-hour figure from `/usage`:

```sh
claude-usage --calibrate <that number>
```

## Two traps worth recording

**Do not label a Codex window by its `primary`/`secondary` position.** The API
puts the *weekly* window in `primary` at least some of the time - on this
machine `primary` is `window_minutes: 10080` (7 days). `~/bin/codex-usage`
hardcodes `primary` as "5-hour" in its headline, which is why it can print
`5-hour : 3.0% used ... resets in 6d4h` - a 5-hour window cannot reset in six
days. This widget derives the label from `window_minutes` instead.

**genmon's click tags are per element, and `<img>` needs `<click>`.**
`<txtclick>` only fires on `<txt>`, and `<iconclick>` pairs with `<icon>`, not
`<img>` - so an image-only widget with either of those is simply not
clickable. `<click>` is the generic handler and works for both. Emitting two
click tags at once is worse than useless: genmon then renders **nothing at
all**, so the widget silently disappears from the panel.

**gdk-pixbuf rasterises an SVG at its declared intrinsic size** and genmon does
not scale it up. A `width="16" height="24"` icon renders 16px tall and is
unreadable. `ai_usage_icon.render()` therefore emits width/height in pixels
while keeping the viewBox in drawing units.

**xfce4-panel writes its in-memory plugin config back over xfconf when it
exits.** Setting `/plugins/plugin-N/command` while the panel is running does
not stick - you end up with `command=""` and a widget reading `(genmon)XXX`.
`install.sh` therefore quits the panel, writes the config, and starts it again.
genmon's xfconf keys are lowercase (`command`, `use-label`, `update-period`);
the capitalised names inside `libgenmon.so` are UI strings, not config keys,
and the `genmon-N.rc` file is not what 4.3 reads.

## Tuning

Environment variables read by `ai-usage-genmon`:

| var | default | meaning |
|---|---|---|
| `AI_USAGE_WARN` | 60 | amber threshold (%) |
| `AI_USAGE_CRIT` | 85 | red threshold (%) |
| `AI_USAGE_STALE_AFTER` | 90 | seconds before a background refresh is triggered |
| `AI_USAGE_TOO_OLD` | 600 | seconds before the reading is shown as stale |
| `AI_USAGE_TEXT` | unset | set to `1` for the wide text form instead of the icon |
| `AI_USAGE_ICON_PX` | 40 | rasterised icon height in px (width follows) |
| `AI_USAGE_SEGMENTS` | 5 | cells in the gauge (text mode only) |
| `AI_USAGE_COL_OK` | `#1b8a3a` | colour below the warn threshold |
| `AI_USAGE_COL_WARN` | `#c25e00` | colour at/above warn |
| `AI_USAGE_COL_CRIT` | `#cc0000` | colour at/above crit |
| `AI_USAGE_COL_EMPTY` | `#b9bcc0` | unfilled gauge cells |
| `AI_USAGE_COL_STALE` | `#6b7280` | colour when the reading is stale |

Set these in the plugin's command, e.g.
`env AI_USAGE_SEGMENTS=8 /path/to/ai-usage-genmon`.

Poll interval is `update-period` in the plugin config (milliseconds), set to
30000 by `install.sh`.
