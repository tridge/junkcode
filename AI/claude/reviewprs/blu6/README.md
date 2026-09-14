# reviewprs on blu6

Dedicated PR-review box. WSL2 Ubuntu 26.04 ("resolute") on a Windows 11 host,
32 cores, 30G RAM, 886G free. systemd is enabled with `cpu memory pids`
delegated to the user manager, so the workflow's Codex agent pool gets its own
cgroup scope exactly as it did on blu4.

## Layout

| path | what |
|---|---|
| `~/review/data/` | all scratch: checkouts, clones, build trees. `$REVIEW_DATA` |
| `~/review/repositories/` | maintained base clones of every reviewed repo. `$REVIEW_REPOS` |
| `~/review/work/` | working dir for a run; reports land here, base checkouts stay clean |
| `~/review/logs/` | run logs, 30-day retention. `$REVIEW_LOGS` |
| `~/review/bin/` | the scripts below |
| `~/review/etc/` | `gitconfig`, `rsync.password`, `crontab.reviewprs`, the run lock |
| `~/venv-ardupilot/` | ArduPilot python venv (from install-prereqs-ubuntu.sh) |

`/tmp` is a **16G tmpfs** — `review-env.sh` points `TMPDIR` into `~/review/data/tmp`
so a bare `mktemp -d` cannot land there.

## Scripts

- `bin/netns-run.sh` - run a command (an autotest, or a whole agent) with its own
  network namespace, so parallel SITL work keeps the default TCP ports instead of
  needing `--uds`, which changes SITL UART timing and produces false findings.
  `--session <dir>` gives one agent a namespace shared by all its commands. Pair it
  with a two-level agent root (`<agent>/wt` for the checkout) so autotest.py's
  `../buildlogs` lock stays private too. `autotest-netns.sh` is a symlink to it.
- `bin/review-env.sh` — source this first. Sets `$REVIEW_*`, `TMPDIR`, `CCACHE_DIR`,
  the ARM/ccache/autotest PATHs, `GIT_CONFIG_GLOBAL`, and the publish variables.
- `bin/run-reviewprs.sh <mode>` — cron entry point. `followup` | `all` | `rsync` | any label.
- `bin/refresh-repos.sh` — nightly refresh of the base clones.
- `bin/base-build.sh` — SITL + CubeOrange build; proves the toolchain, warms ccache.
- `bin/clone-ardupilot.sh`, `bin/clone-repos.sh` — initial/base clones.

All review runs and the repo refresh share **one flock** (`etc/reviewprs.lock`).
The three label sub-runs share `devcall_pr_reviews.html` and followup reads what
they publish, so overlap would corrupt output. A blocked slot logs `SKIPPED`
rather than queueing.

## Two things that differ from blu4

**Publishing goes over the rsync daemon, not ssh.** blu6 has no ssh key for fjall.
`$REVIEW_PUBLISH=rsync://reviews@fjall`, `$RSYNC_AUTH=--password-file=~/review/etc/rsync.password`.

**`uav.tridgell.net` is in `/etc/hosts`** pointing at fjall's LAN address 192.168.2.10.
fjall's public IP (203.217.61.45) is firewalled from inside the network, so the
workflow's https reads of published reports would otherwise time out. `wsl.conf`
sets `generateHosts=False`, so the entry persists.

**git has an `insteadOf` rewrite** forcing `https://github.com/` to `git@github.com:`,
and there is no ssh key, so plain HTTPS clones fail. `review-env.sh` sets
`GIT_CONFIG_GLOBAL=~/review/etc/gitconfig`, which has no rewrite. The user's own
`~/.gitconfig` is untouched.

## Cron

Staged at `~/review/etc/crontab.reviewprs`, **not installed**. Install with:

    crontab ~/review/etc/crontab.reviewprs

Remove with `crontab -r`, inspect with `crontab -l`.

## Settings (carried over from blu4, 2026-09-06)

**Claude** — `~/.claude/settings.json`: `defaultMode: auto`, `skipAutoPermissionPrompt: true`,
model `opus[1m]`, the three official plugins, and the **`git push` deny rules**. The `allow` list
was emptied: blu4's entries were ssh hosts this box cannot reach plus two `git push tridge pr-SITL`
allowances, and this machine must never push. `autoMode.environment` was re-pointed at blu6
(16G tmpfs, `$REVIEW_DATA`, review-box role). Plus `~/.claude/CLAUDE.md`, the commands, the three
skills, and the accumulated reviewprs memories under
`~/.claude/projects/-home-tridge-review-work/memory/`.

**Codex** — `~/.codex/config.toml`: same globals as blu4 (`gpt-6-astra`, pragmatic, high reasoning,
`approval_policy=on-request`, `sandbox_mode=workspace-write`, `approvals_reviewer=auto_review`), the
**`auto_review` policy byte-identical**, plugins, `agents.max_depth`, and `features.multi_agent_v2`.
Changed for this box: `sandbox_workspace_write.writable_roots` now lists the `~/review` tree and the
venv instead of blu4's `/data/*`, and the 88 blu4 `[projects.*]` trust entries were replaced by five
covering `~/review`. Plus `~/.codex/AGENTS.md` (which carries the never-push rule) and
`~/.codex/rules/default.rules`, both verbatim.

**The cron wrapper does NOT use `--permission-mode bypassPermissions`** — that flag bypasses deny
rules too, which would re-enable `git push`. It uses `auto`, matching blu4, and pre-flights the
settings file before each run.

## Review toolchain (audited from run logs, 2026-09-08)

Runs were scanned for `ModuleNotFoundError`, `command not found` and "please install"
to find what the environment was actually short of. Installed as a result:

| what | where | why a run wanted it |
|---|---|---|
| `pytest`, `pytest-mock` | venv + apt | most repos' test suites; was missing everywhere |
| `scipy` | apt (system) | was venv-only, so system-python checks failed |
| `pylint`, `ruff`, `flake8`, `mypy` | venv / apt | reproducing repos' lint CI jobs |
| `pre-commit` | apt | ArduPilot and wiki CI both run it |
| `shellcheck` | apt | ArduPilot has a shellcheck CI job |
| `uv` | venv | several workflows use `uv pip install` |
| `jsdom` | npm -g | the wiki offline-JS harness; a run failed outright for want of it |
| `tdb` | (fixed separately) | |

`review-env.sh` now also exports `NODE_PATH` — a global npm module is not found by a
bare `require()` without it, so jsdom was installed but unusable until that was set.

**Known gap, not fixed:** `python3.8` is unavailable on Ubuntu 26.04. A run wanted it to
check MethodicConfigurator's minimum supported Python. Testing that needs pyenv or a
container; the reviews currently note it rather than verify it.
