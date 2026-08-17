# Claude Code customisations

Personal tools and slash commands for [Claude Code](https://claude.com/claude-code).
Each lives in its own subdir with a README and install notes.

| Dir | What | Install as |
|-----|------|------------|
| [`cost/`](cost/) | `claude-usage` script + `/cost` command: per-session view of the Max-plan 5-hour rolling quota (which open session is burning the most). | `~/bin/claude-usage`, `/cost` |
| [`codex/`](codex/) | `/codex` command: run the OpenAI Codex CLI on a task and review its output as a second opinion. | `/codex` |
| [`handover/`](handover/) | `/handover` command: save a handover note so a fresh session (e.g. after a model switch) can continue the work. | `/handover` |
| [`reviewprs/`](reviewprs/) | `/reviewprs <label>` command: review every open GitHub PR with a label (main + wiki + submodule repos) into one sortable HTML report with verdicts. Every PR gets both a Claude review and an independent Codex cross-check, fanned out one agent per PR; findings are posted back to the PRs as marked comments and the report published to the web (head-hash skip makes re-runs cheap). | `/reviewprs` |

## Slash commands

Drop a `*.md` command file into `~/.claude/commands/` (user-global) or a
project's `.claude/commands/`; it then appears as `/<name>` in Claude Code. Each
subdir's README gives the exact copy command.

Most of these stay local — `claude-usage` only reads local transcripts under
`~/.claude/projects`, and `/codex`/`/handover` run local CLIs or write local
files. The exception is `/reviewprs`, which queries GitHub (read-only via `gh`), runs
the `codex` CLI, posts review comments back to the PRs it reviews, and publishes
its HTML report to the public ArduPilot web host
(`uav.tridgell.net/DevCallReviews/`).
