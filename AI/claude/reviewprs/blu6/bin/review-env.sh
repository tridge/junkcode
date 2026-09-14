# Common environment for all review work on blu6.
# Source this from cron wrappers and interactive shells alike.
export REVIEW_ROOT="$HOME/review"
export REVIEW_DATA="$REVIEW_ROOT/data"
export REVIEW_LOGS="$REVIEW_ROOT/logs"
export REVIEW_REPOS="$REVIEW_ROOT/repositories"

# Isolated git config: no https->ssh rewrite, so HTTPS clones work without a key.
export GIT_CONFIG_GLOBAL="$REVIEW_ROOT/etc/gitconfig"

# Never let anything default into /tmp - it is a 16G tmpfs on this box and
# filling it takes the machine down.
export TMPDIR="$REVIEW_DATA/tmp"
mkdir -p "$TMPDIR" "$REVIEW_DATA"

export CCACHE_DIR="$REVIEW_ROOT/ccache"
# ccache first, then the ARM toolchain and ArduPilot's autotest dir. cron does
# not source ~/.profile, where install-prereqs-ubuntu.sh put these, so they are
# repeated here or every ChibiOS build fails with "arm-none-eabi-gcc not found".
export PATH="/usr/lib/ccache:$PATH"
export PATH="/opt/gcc-arm-none-eabi-10-2020-q4-major/bin:$PATH"
export PATH="$REVIEW_REPOS/ardupilot/Tools/autotest:$PATH"
export PATH="$REVIEW_ROOT/bin:$HOME/.local/bin:$HOME/.npm-global/bin:$PATH"

# --- per-mode Claude account ------------------------------------------------
# rsync reviews are tridge's own project, so they run on a separate Claude
# subscription and leave the main account's quota for ArduPilot work.
# CLAUDE_CONFIG_DIR relocates the whole config dir, credentials included; the
# shared parts (commands, skills, CLAUDE.md, plugins) are symlinks back into
# ~/.claude so the skill can never drift between the two accounts.
# The address itself lives in etc/account.conf, which is deliberately NOT in the
# junkcode copy of these scripts: that repo is public and this is a personal
# address. See etc/account.conf.example. An env value wins, for testing.
export REVIEW_RSYNC_CLAUDE_DIR="$REVIEW_ROOT/etc/claude-rsync"
[ -z "${REVIEW_RSYNC_CLAUDE_ACCOUNT:-}" ] && [ -f "$REVIEW_ROOT/etc/account.conf" ] && \
    . "$REVIEW_ROOT/etc/account.conf"
export REVIEW_RSYNC_CLAUDE_ACCOUNT="${REVIEW_RSYNC_CLAUDE_ACCOUNT:-}"

# A Claude Code OAuth refresh that dies mid-flight leaves .oauth_refresh.lock
# behind in the config dir, and every later invocation then refuses to refresh
# with "another Claude Code process is refreshing it or exited mid-refresh".
# Nothing clears it on its own: the 05:25 rsync run on 2026-09-15 failed that
# way, 15h after its token expired, and would have failed every day since.
# Clear it only when no live claude process is actually using that dir.
clear_stale_oauth_lock() {
    local dir="${1:-${CLAUDE_CONFIG_DIR:-$HOME/.claude}}"
    local lock="$dir/.oauth_refresh.lock" p env inuse=0
    [ -e "$lock" ] || return 0
    for p in $(pgrep -x claude 2>/dev/null); do
        env=$(tr "\0" "\n" < "/proc/$p/environ" 2>/dev/null) || continue
        if printf %s "$env" | grep -qx "CLAUDE_CONFIG_DIR=$dir"; then
            inuse=1
        elif [ "$dir" = "$HOME/.claude" ] && \
             ! printf %s "$env" | grep -q "^CLAUDE_CONFIG_DIR="; then
            inuse=1                      # no override means the default dir
        fi
    done
    if [ "$inuse" -eq 0 ]; then
        rm -rf "$lock" && echo "cleared a stale OAuth refresh lock in $dir"
    else
        echo "NOTE: OAuth refresh lock held in $dir by a running claude"
    fi
}

# --- publishing -------------------------------------------------------------
# blu6 has no ssh key for fjall, so reports go over the rsync daemon with a
# password instead. The path *after* the base is identical to the old ssh form,
# so only the base and the auth option differ from blu4.
#   blu4 was:  rsync -Pavz --mkpath "$REPORT" tridgell.net:UAV-web/DevCallReviews/...
#   blu6 is:   rsync -Pavz --mkpath $RSYNC_AUTH "$REPORT" $REVIEW_PUBLISH/DevCallReviews/...
export REVIEW_PUBLISH="rsync://reviews@fjall"
export RSYNC_AUTH="--password-file=$REVIEW_ROOT/etc/rsync.password"

# Globally-installed npm modules (jsdom, used by the wiki JS test harnesses) are
# not found by a bare require() from an arbitrary cwd without this.
export NODE_PATH="$(npm root -g 2>/dev/null)"
