---
description: Show each Claude Code session as a percentage of your 5-hour rolling quota (Max plan), so you can see which open session is burning the most
argument-hint: "[--window 5h|17h|2d|30m|1w|1d6h|total] [--api-cost] [--tokens] [--calibrate PCT] [--show-quota] [--top N]"
allowed-tools: Bash(/home/tridge/bin/claude-usage:*)
---

Run `~/bin/claude-usage $ARGUMENTS` and show the user its output verbatim in a code block.

The script scans every Claude Code session transcript under `~/.claude/projects`,
folds each subagent's tokens into its parent session, weights tokens by per-model
price ratio into a single "unit", and reports each session as a **percentage of
the user's 5-hour rolling quota** (they are on a Max plan, so absolute dollars
don't matter — percentages of the 5h quota do).

`--window` accepts any duration: `5h`, `17h`, `2d`, `30m`, `1w`, combined like
`1d6h`, or `total`. For a window <= 5h it shows each session's `%QUOTA` (share of
the 5h quota) and `SHARE` (slice of the window). For a window > 5h, "% of a 5h
quota" is NOT additive (the quota refills every 5h), so it instead shows each
session's `PEAK5h` — its busiest trailing-5h as a % of one full quota, which
stays bounded near 100% — plus its `SHARE` of that window's total work.

`--api-cost` (alias `--dollars`) switches to absolute USD: what the window's usage
WOULD cost at Anthropic API pay-as-you-go list prices — useful for gauging the
value of overflow usage that would be billed per token off-Max. `--tokens` shows
raw token counts. Both respect `--window`.

The 5h ceiling is calibrated against Claude's real built-in `/usage` meter: the
user reads the **5-HOUR** percentage `/usage` shows (NOT the weekly one), ideally
when usage is high, then runs `claude-usage --calibrate <that number>`. The tool
warns if a calibration is inconsistent with the largest historical 5h burst.
Until calibrated it falls back to that historical peak (labelled "uncalibrated").

After showing the table:
- Call out which active session is the biggest `%QUOTA` (5h) / `PEAK5h` (longer) consumer.
- If the ceiling is still uncalibrated or flagged suspect, remind them to open
  `/usage`, read the **5-hour** %, and run `/cost --calibrate <that %>` once while
  usage is high.

Do not re-implement the analysis yourself — just run the script.
