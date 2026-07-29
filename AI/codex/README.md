# codex-usage

Per-session usage view for the **OpenAI Codex CLI** — the Codex analogue of
[`../claude/cost/claude-usage`](../claude/cost). When you have many Codex sessions
running, it shows the real account quota meter and which session/project is
burning the most of it.

Codex writes one rollout transcript per session under
`$CODEX_HOME/sessions/YYYY/MM/DD/rollout-<ts>-<UUID>.jsonl`. Every assistant turn
emits a `token_count` event that records BOTH the per-turn token usage
(`info.last_token_usage`) AND the live account rate-limit meter
(`rate_limits.primary` and `rate_limits.secondary`), each with `used_percent`,
`resets_at` and `window_minutes`. So — unlike claude-usage, which has to
calibrate the 5h ceiling against `/usage` — this reads the **real** quota
percentage straight from the data, no calibration.

The primary/secondary slots do **not** have fixed window lengths: the weekly
(10080-minute) window turns up in `primary` too. Each window is therefore
labelled from its own `window_minutes`, never from which slot it arrived in.

## Install

```sh
cp codex-usage ~/bin/ && chmod +x ~/bin/codex-usage
```

Reads `~/.codex` (or `$CODEX_HOME`); nothing is sent anywhere.

## Usage

```sh
codex-usage                 # quota meter + sessions active in the last 24h
codex-usage --window 5h     # attribute tokens over the 5h quota window
codex-usage --window 7d     # the weekly window
codex-usage --all           # every session ever
codex-usage --by-project    # aggregate by project dir (SUBS column -> SESS count)
codex-usage --top N         # rows to show (default 20)
codex-usage --api-cost      # show USD at OpenAI API list prices instead of token counts
```

`--api-cost` reweights the per-session token deltas (uncached input / cached input
/ output) by the model's API list price — for gpt-5.5: **$5 / $0.50 / $30 per 1M**
in/cached/out — to estimate what the usage *would* cost billed pay-as-you-go. On a
Codex subscription you are **not** charged this; it's a value estimate. The model
is read per session from each turn's `turn_context.model` (unknown models fall back
to gpt-5.5 pricing).

Example:

```
Codex usage — 2026-06-04 14:22 AEST
plan: prolite    (meter as of 0s ago)
  5-hour :  88.0% used  [##################..]  resets in 3h37m
  weekly :  14.0% used  [###.................]  resets in 6d22h

 LAST  PROJECT             TURNS SUBS    5h    24h    7d   TOTAL  TITLE
   0s  rsync.secscan        1039   25  116M   116M  116M   116M  $codex-security:deep-security-scan
  61m  rsync-git              52    0  3.1M   6.8M  6.8M   6.8M  carefully review pending changes...
```

## Notes

- The **5h / weekly percentages are Codex's real account meter** (account-global).
- The **token columns are per-session totals** (`total_tokens` deltas) — a proxy
  for which session drove the usage, since the meter itself isn't broken out per
  session. Codex's exact quota weighting of input/cached/output tokens isn't
  published, so treat tokens as a "who's heavy" ranking, not an exact cost.
- `SUBS` = subagent threads rolled into the parent session that spawned them
  (via each rollout's `session_meta.parent_thread_id`).
