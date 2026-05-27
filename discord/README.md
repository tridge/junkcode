# discord-x-reap

Live workaround for Discord (and any Electron app's) Xorg-client leak per
[electron/electron#2922](https://github.com/electron/electron/issues/2922):
every desktop notification leaks one Xorg client connection (~10 yr unfixed
upstream). On this machine Discord drips ~13 leaks/h from its main browser
process; once Xorg's 256-client table fills, *all* new X clients on the
system start failing with `Maximum number of clients reached`.

`discord-x-reap` reaps the leaked connections **without restarting Discord**
(so voice calls aren't dropped). It identifies the orphaned X11 socket file
descriptors in Discord's main browser process and uses `gdb` to inject
`shutdown(fd, SHUT_RDWR)` on each. Xorg sees EOF on its side and frees the
client slot.

## Usage

```sh
# Safe to run any time. Defaults: keep 10 oldest legit conns, close the rest.
./discord-x-reap

# Show what would be closed, do nothing — read-only, no gdb attach.
./discord-x-reap --dry-run

# Safety test: close only the single newest candidate.
./discord-x-reap --one

# Tune how many oldest connections are treated as "legit Chromium startup".
./discord-x-reap --keep 12
```

**No sudo needed.** `ss -x` (without `-p`) shows all unix sockets — including
the X server's — to any user; we don't need the per-process attribution that
would require root because we read `/proc/$pid/fd` directly (same uid as
Discord). gdb attach to a same-uid process works without sudo because
`kernel.yama.ptrace_scope=0` is set on this box.

Logs to `~/.cache/discord-x-reap.log` (timestamp, pid, mode, before/after
client counts, fds closed).

## How it works

1. Find Discord's main browser pid — the one whose cmdline contains
   `discord/Discord` and has **no** `--type=` arg (which identifies
   zygote/renderer/utility/GPU children).
2. Snapshot the set of X11 client-side socket inodes from `/usr/bin/ss -xp`'s
   view of `/tmp/.X11-unix/X0` (peer inodes in `* N` notation).
3. **Attach gdb** to Discord — this suspends the process. Everything below
   happens while Discord is paused, so the fd→inode mapping can't change
   under us between enumeration and the `shutdown()` call (TOCTOU-safe).
4. Inside the gdb session a Python helper enumerates `/proc/$pid/fd`,
   intersects with the X11-inode set from step 2, sorts ascending by inode,
   and keeps the **N oldest** (default 10 — Hereket measured 7 for legit
   Chromium startup; 10 is a safety margin). The rest are leaked.
5. The helper then calls `(int) shutdown(fd, 2)` (SHUT_RDWR) on each leaked
   fd via `gdb.execute()`, counting how many returned 0 vs an error code.
   Failed calls are reported with the fd and inode for diagnosis.
6. gdb detaches; Discord resumes. The script verifies the X server's client
   count actually dropped and exits non-zero if it didn't.

### Why `shutdown` not `close`

- Triggers Xorg to see EOF and free its client slot — the actually-scarce
  resource (Xorg's 256-client table, *not* Discord's per-process fd limit
  which is 64k+).
- Leaves Discord's fd table entry intact, so the fd number can't be
  reassigned to something unrelated while code that still thinks it's an X
  connection might touch it.
- If Discord ever does touch a reaped fd, it gets a clean `EPIPE` /
  `ECONNRESET` rather than reading garbage from an unrelated socket.

## Why this is safe for *this* bug

The Electron #2922 pattern is the friendly case: leaks happen because the
in-memory `Display*` / `xcb_connection_t*` reference is dropped on the
floor after a transient operation (notification path). The kernel fd is
orphaned — no Discord code holds a pointer to anything that references it.
Closing it externally has nothing to break.

We additionally keep the oldest 10 connections untouched, which are
Chromium's legitimate startup connections (window manager, input,
GLX/rendering, plus the 4 idle ones it always opens — see Hereket's
analysis at <https://hereket.com/posts/monitoring-raw-x11-communication/>).

## First-time validation

```sh
./discord-x-reap --dry-run     # confirm candidate count makes sense (~117 - 10 = ~107)
./discord-x-reap --one         # close just one; verify Discord still responsive
                               # and `ss -x | grep -c X11-unix` dropped by 1
./discord-x-reap               # full reap
```

If anything goes wrong, the fallback is `~/bin/discord-noupdate` which
killalls Discord and relaunches it with the existing `--disable-gpu` flags.

## Caveats

- It only fixes the *current* leak, not the cause. Discord will keep
  leaking from new notifications.  Best run periodically — e.g. via a
  systemd user timer every 30 min (see "Stretch" below).
- Requires `kernel.yama.ptrace_scope=0` (already set on this box at
  `/etc/sysctl.d/10-ptrace.conf`). On a stricter system you'd need `sudo
  gdb` and a different invocation.
- Generalises to other Electron apps suffering the same bug (Slack etc.)
  by tweaking the pid-discovery step.

## Stretch — periodic timer (not yet written)

```ini
# ~/.config/systemd/user/discord-x-reap.service
[Service]
Type=oneshot
ExecStart=%h/junkcode/discord/discord-x-reap

# ~/.config/systemd/user/discord-x-reap.timer
[Timer]
OnBootSec=10min
OnUnitActiveSec=30min
[Install]
WantedBy=timers.target
```

`systemctl --user enable --now discord-x-reap.timer` once. With ~13 leaks/h
and a 30-min cadence, Discord's slot usage stays bounded at ~10 (legit) +
~7 (half-hour drip) ≈ 17, comfortably under 256 indefinitely without
restarting Discord.
