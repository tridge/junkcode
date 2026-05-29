---
description: Run OpenAI Codex CLI on the given task and review its output
argument-hint: <task for codex, e.g. "check foo.doc for accuracy">
allowed-tools: Bash(codex:*), Bash(codex), Read, Glob, Grep
---

Run the OpenAI Codex CLI non-interactively with this task:

> $ARGUMENTS

Use the Bash tool to execute the command exactly as written below. Capture full stdout/stderr — Codex's output is the whole point of this command.

```bash
codex exec --skip-git-repo-check "$ARGUMENTS" </dev/null
```

(`--skip-git-repo-check` avoids the trusted-directory refusal when not inside a git
working tree; `</dev/null` keeps codex from blocking on stdin when its output is
captured by Bash.)

When it finishes:

1. **Summarize Codex's findings** in a few bullets — what it inspected, what it concluded, and any concrete issues it raised.
2. **Surface anything actionable**: file:line citations for problems it flagged, suggested edits, or commands to run.
3. If Codex made claims that contradict what I (Claude) had previously asserted in this session, **flag the disagreement explicitly** rather than silently agreeing — that's a key reason for invoking Codex as a second opinion.
4. Wait for the user to decide whether to apply suggestions; don't auto-apply changes.

Notes:
- Codex runs agentically and may take a while on bigger tasks. That's expected.
- If Codex returns an authentication error, the user needs to run `codex login` once in their own terminal.
