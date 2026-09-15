# /reviewprs — moved

This was the ArduPilot AI pull-request review system: a Claude Code slash command
that reviews the open PRs carrying a review label, cross-checks each one with a
second independent AI reviewer, publishes an HTML report and posts the findings
back to the PR.

It now lives in its own ArduPilot repository, so the dev team can read and
improve it:

**https://github.com/ArduPilot/APReview**

That repo holds the command itself (`commands/reviewprs.md`), the scripts that run
it unattended on a dedicated box (`runner/`), and the operational notes
(`docs/review-box.md`). Install the command with:

```sh
git clone https://github.com/ArduPilot/APReview.git
cp APReview/commands/reviewprs.md ~/.claude/commands/
```

Nothing here is maintained any more — the copy that used to be in this directory
was moved on 2026-09-16 and has since diverged.
