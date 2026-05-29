# claude-usage

Per-session view of Claude Code usage, aimed at the **Max plan 5-hour rolling
quota**: when you have many sessions open at once, it shows which one is burning
the most quota.

Claude Code writes a JSONL transcript per session under
`~/.claude/projects/<project>/<session-id>.jsonl`, plus one per subagent under
`<session-id>/subagents/agent-*.jsonl`. Every assistant turn records its `model`
and a `usage` block (input / output / cache-read / cache-write tokens). Subagent
records carry their **parent** `sessionId`, so grouping by `sessionId` folds
subagent usage into the session that spawned it. This script reads those
transcripts (nothing is sent anywhere) and summarises per session.

Tokens are weighted by per-model API price ratio into a single "unit" (a proxy
for how the quota meter weights cheap input / expensive output / cheap cache
reads / different models), then shown as a percentage of your 5h quota.

## Install

```sh
cp claude-usage ~/bin/                       # or anywhere on $PATH
cp cost.md ~/.claude/commands/cost.md        # optional: gives you /cost in Claude Code
```

## Usage

```sh
claude-usage                  # last 5h: each session as % of the 5h quota
claude-usage --window 17h     # any duration: 5h, 17h, 2d, 30m, 1w, 1d6h, total
claude-usage --api-cost       # what the window would cost at API pay-as-you-go rates
claude-usage --tokens         # raw token counts for the window
claude-usage --show-quota     # print the current ceiling + 5h usage
claude-usage --top N          # rows to show (default 20)
```

- **Window <= 5h** → additive `%QUOTA` (share of your 5h quota) + `SHARE` (slice
  of the window total).
- **Window > 5h** → `%QUOTA` is not additive (the quota refills every 5h), so it
  shows each session's `PEAK5h`: its busiest trailing-5h as a % of one full
  quota (stays bounded near 100%) + `SHARE` of the window's total work.

## Calibrating the 5h ceiling

There is no public token number for the Max 5h ceiling, so anchor it to the real
meter: open Claude's built-in `/usage`, read the **5-hour** percentage (NOT the
weekly one), ideally while usage is high, then:

```sh
claude-usage --calibrate 80   # "right now my 5h usage is 80% full"
```

The ceiling is saved to `~/.claude/usage-quota.json` and later runs line up with
the real meter. The tool warns if a calibration is inconsistent with your largest
historical 5h burst (a sign you read the weekly % or calibrated while idle).
Uncalibrated, it falls back to that historical peak.

## Caveats

The token→quota mapping and the price weighting are approximations — Anthropic
doesn't publish the exact quota formula. Treat this as a solid *ranking* of which
session is heavy; calibration anchors the totals, the per-session split is only
as good as the price-ratio weighting. `--api-cost` is a list-price estimate, not
a bill (and on Max you are not charged it).
