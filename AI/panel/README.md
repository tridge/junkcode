# AI usage panel widget (xfce4)

An xfce4 panel widget showing Claude Code and Codex CLI quota usage at a
glance, so you can see whether you are near a 5-hour or weekly limit without
opening a terminal. Click it for the full `claude-usage` / `codex-usage`
reports.

By default it draws a small **icon**: two vertical gauges, `C` = Claude on the
left, `X` = Codex on the right. About 27px wide, so it sits alongside the other
panel icons instead of dominating the panel. The numbers live in the tooltip.

**The gauges drain like a battery.** A full bar means plenty of quota left; it
empties as you use it up:

```
 ┌─┐┌─┐  ┌─┐┌─┐  ┌─┐┌─┐  ┌─┐┌─┐
 │▓││▓│  │ ││ │  │ ││ │  │ ││ │
 │▓││▓│  │▓││▓│  │ ││ │  │ ││ │
 └─┘└─┘  └─┘└─┘  └─┘└─┘  └─┘└─┘
  0% used  50%     70%     ~100%
  green    green   amber   red sliver -> empty
```

Filling them with the *used* fraction instead reads exactly backwards: a
nearly-empty bar then means "barely touched", which looks like trouble at a
glance. The fill is headroom remaining; the colour still comes from how much is
used (green `<60%`, amber `>=60%`, red `>=85%`, grey = stale).

A sliver stays visible while any headroom remains, so "almost out" is still
distinguishable from "completely out". A provider with no data reads as empty,
never as full.

### Text mode

`AI_USAGE_TEXT=1` swaps the icon for the original wide textual form:

```
C ▰▰▰▱▱ ~58%5h   X ▰▱▱▱▱ 4%wk
│ │      │  │      └── window the figure applies to (5h / wk)
│ │      │  └───────── percent of that limit used
│ │      └──────────── "~" = estimate, not a real meter (Claude only)
│ └─────────────────── battery-style gauge, 5 cells
└───────────────────── C = Claude, X = Codex
```

It is far wider - a couple of hundred pixels against the icon's ~27 - which is
why it is no longer the default.

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

A full scan takes about **8 seconds** (claude ~2s, codex ~6s) - far too slow to
run inside a panel poll, which blocks the panel while it runs. So the widget is
split: `ai-usage-genmon` only ever reads the cache (~0.05s) and, when the cache
is older than 90s, spawns `ai-usage-refresh` detached. The panel shows the last
known values meanwhile, greyed out once they pass 10 minutes old.

## What the two numbers actually mean

They are not equally trustworthy, which is why Claude's is marked with `~`:

**Codex** publishes a real account meter. It is read straight out of the
session files (`rate_limits.primary` / `.secondary`, each with
`used_percent`, `window_minutes`, `resets_at`), so the percentage and the reset
time are exact.

**Claude** stores no meter anywhere locally. Its figure is `claude-usage`'s
estimate: local token usage over the trailing 5h as a share of a ceiling that
is either calibrated or guessed from your largest historical 5h burst. Improve
it by reading the 5-HOUR figure from Claude's `/usage` and running:

```sh
claude-usage --calibrate <that number>
```

The tooltip says `UNCALIBRATED` until you do. There is **no Claude weekly
figure** - the data to compute one is not available locally.

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
