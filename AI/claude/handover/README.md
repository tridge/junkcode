# /handover

A Claude Code slash command that saves a **handover note** so a fresh session can
pick up the current work exactly where it left off — most useful when quitting to
upgrade to a new model, or switching machines.

## Install

```sh
cp handover.md ~/.claude/commands/handover.md   # gives you /handover in Claude Code
```

This command relies on Claude Code's **persistent memory** feature: it writes a
`current_handover.md` memory file in the project's memory directory and adds a
pointer as the first line of that project's `MEMORY.md` index, so the next
session sees it automatically.

## Usage

```
/handover
/handover focus on the failing rsync xattr test, ignore the docs cleanup
```

An optional argument is recorded verbatim as a "User focus" note for the next
session. The handover captures: current branch / recent commits / uncommitted
state, the in-flight task and immediate next step, recent decisions and empirical
findings, anything pending (open PRs, CI runs, scheduled wakeups), and active
context — grounded by actually running `git status`/`git log`, not the system
prompt snapshot.

There is one live handover per project (it overwrites the previous one, not a
stack). In the new session, just say "read the handover" or describe what you
want — the `MEMORY.md` pointer makes it hard to miss.
