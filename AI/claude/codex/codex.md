---
description: Run OpenAI Codex CLI on the given task and review its output (retains context across /codex calls)
argument-hint: <task for codex, e.g. "check foo.doc for accuracy"> | --new <task to start a fresh thread>
allowed-tools: Bash(codex-session:*), Bash(codex:*), Bash(codex), Read, Glob, Grep
---

Run the OpenAI Codex CLI non-interactively with this task:

> $ARGUMENTS

Use the Bash tool to execute the command exactly as written below. Capture full stdout/stderr — Codex's output is the whole point of this command.

```bash
codex-session "$ARGUMENTS" </dev/null
```

`codex-session` (a wrapper at `~/bin/codex-session`) threads context across calls:
it keeps a per-directory Codex session id and `codex exec resume`s it, so each
`/codex` call in the same working directory continues the previous conversation
instead of starting cold. It prints a `>> [codex] resuming session <id>` (or
`starting new session`) line first so you can see which it did. To deliberately
drop prior context and begin a fresh thread, the user passes `--new` as the first
word (e.g. `/codex --new review this file`); `/codex --new` on its own just resets
the thread for the current directory. (The wrapper passes `--skip-git-repo-check`
so it works outside a git tree, and `</dev/null` keeps codex from blocking on stdin.)

When it finishes:

1. **Summarize Codex's findings** in a few bullets — what it inspected, what it concluded, and any concrete issues it raised.
2. **Surface anything actionable**: file:line citations for problems it flagged, suggested edits, or commands to run.
3. If Codex made claims that contradict what I (Claude) had previously asserted in this session, **flag the disagreement explicitly** rather than silently agreeing — that's a key reason for invoking Codex as a second opinion.
4. Wait for the user to decide whether to apply suggestions; don't auto-apply changes.

Notes:
- Codex runs agentically and may take a while on bigger tasks. That's expected.
- If Codex returns an authentication error, the user needs to run `codex login` once in their own terminal.
- If a thread gets long/stale or you want a clean slate, start with `--new`.
