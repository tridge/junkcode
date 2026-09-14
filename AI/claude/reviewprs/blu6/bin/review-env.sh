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
