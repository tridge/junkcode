# /codex

A Claude Code slash command that runs the **OpenAI Codex CLI** non-interactively
on a task, then has Claude summarise and review Codex's output — a quick way to
get a second opinion from a different model and flag where the two disagree.
Context is **retained across calls** so successive `/codex` invocations form one
ongoing conversation.

## Install

```sh
cp codex.md ~/.claude/commands/codex.md      # gives you /codex in Claude Code
cp codex-session ~/bin/ && chmod +x ~/bin/codex-session   # the threading wrapper
```

Requires the `codex` CLI on `$PATH`. First use needs a one-time `codex login` in
your own terminal (Codex auth is interactive; the command can't do it for you).

## Usage

```
/codex check foo.py for off-by-one errors in the ring buffer
/codex is this commit message accurate for the diff?
/codex --new start a fresh thread, forgetting prior context
```

The command calls `codex-session "$ARGUMENTS"`, which threads context across calls:

```sh
codex exec --skip-git-repo-check "$ARGUMENTS"            # first call in a dir
codex exec resume <session-id> --skip-git-repo-check "$ARGUMENTS"   # subsequent
```

`codex-session` keeps a **per-directory Codex session id** under
`$CODEX_HOME/.claude-threads/` (keyed by the cwd) and `codex exec resume`s it, so
each `/codex` call in the same working directory continues the previous
conversation instead of starting cold. It prints `>> [codex] resuming session
<id>` (or `starting new session`) first so you can see which it did, and refreshes
the stored id to the newest rollout after each run so the chain moves forward.
Start over with `--new` (e.g. `/codex --new ...`; `/codex --new` alone just resets
the current dir's thread). `--skip-git-repo-check` lets it run outside a git tree;
`</dev/null` stops Codex blocking on stdin when Bash captures its output.

After Codex finishes, Claude summarises what it inspected and concluded, surfaces
anything actionable (file:line citations, suggested edits), explicitly flags any
disagreement with Claude's own earlier assertions, and waits for you to decide —
it does not auto-apply changes. Codex runs agentically, so bigger tasks take a
while.
