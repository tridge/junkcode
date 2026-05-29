---
description: Save a handover note about current state and tasks so a new Claude session (e.g. after restarting to switch models) can continue this work
argument-hint: <optional focus/emphasis note for the next session>
allowed-tools: Read, Write, Edit, Bash(git:*), Bash(gh:*), Bash(ls:*), Bash(pwd:*)
---

Save a handover so the next Claude session can pick up this work exactly where it left off — most commonly after the user quits to upgrade to a new model, or switches machines.

## Where to save it

Write the handover as a memory file `current_handover.md` in this project's memory directory (the absolute path is in the persistent-memory section of the system prompt — use that path; do not invent one). Use these frontmatter fields:

```markdown
---
name: current-handover
description: ACTIVE handover — read first if continuing this work; <one-line summary of what's in flight>
metadata:
  type: project
---
```

Overwrite any prior `current_handover.md` — there is one live handover at a time per project, not a stack.

## What to capture

Only what a fresh session genuinely needs to continue without re-asking. Skip session chatter and anything the system prompt or `git status` would re-provide on its own.

- **Where we are**: current branch, recent commits ahead of `origin/master` (short SHAs + subjects), uncommitted state, files in flight. Run `git status`/`git log` to ground this — do not rely on the snapshot in the system prompt.
- **What's in flight**: the active task in one or two sentences, and the immediate next concrete step.
- **Recent decisions & findings**: empirical results, design choices, things discovered that wouldn't be obvious from the code or git history alone.
- **Pending**: any decision awaiting the user, any background tasks still running, artifacts with deadlines (open PR numbers, CI runs being watched, scheduled wakeups).
- **Active context**: open PR numbers/branches under iteration, CI status if it matters, pointers to relevant dev-notes/files.
- **User emphasis** (if `$ARGUMENTS` is non-empty): include the user's note verbatim under a "User focus" subheading — they typed it for a reason.

## Update MEMORY.md so the next session sees it

Edit MEMORY.md (in the same memory directory) so the very FIRST line is the handover pointer, formatted to be unmissable. Replace any prior `current_handover` line — don't accumulate. Example:

```
- [CURRENT HANDOVER — read first if continuing this work](current_handover.md) — <one-line summary>
```

If a previous handover line already exists, replace it in place; otherwise insert at the top of the index.

## Confirm

After saving, show the user:
1. The absolute path of the handover file.
2. The one-line summary that went into MEMORY.md.
3. The immediate next step that was captured.

Tell them: in the new session, MEMORY.md loads automatically, so they can either say "read the handover" or just describe what they want — the index entry makes the handover hard to miss.
