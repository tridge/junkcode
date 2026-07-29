#!/bin/bash
# Install the AI usage widget into the xfce4 panel.
#
#   ./install.sh            add the widget (backs up the panel config first)
#   ./install.sh --remove   take it out again
#
# Safe to re-run: it will not add a second copy.
#
# WHY THIS STOPS THE PANEL
# ------------------------
# xfce4-panel writes its in-memory plugin config back over xfconf when it
# exits. So configuring a plugin while the panel is running does not stick:
# the panel either has not read your values yet, or blanks them on the next
# restart. Setting /plugins/plugin-N/command with the panel up and then
# restarting reliably ends with command="" and a widget showing "(genmon)XXX".
#
# The sequence that works is: quit the panel, write the config, start it again.
# genmon's keys in xfconf are lowercase (command, use-label, update-period) -
# the capitalised names in the plugin binary are UI strings, not config keys.

set -eu
HERE="$(cd "$(dirname "$0")" && pwd)"
GENMON_CMD="$HERE/ai-usage-genmon"
PANEL="panel-1"
UPDATE_PERIOD_MS=30000      # genmon wants milliseconds; the poll costs ~0.05s
BACKUP_DIR="$HOME/.config/xfce4/xfconf/xfce-perchannel-xml"
BACKUP="$BACKUP_DIR/xfce4-panel.xml.bak-ai-usage-$(date +%Y%m%d%H%M%S)"
RCDIR="$HOME/.config/xfce4/panel"

if ! command -v xfconf-query >/dev/null 2>&1; then
    echo "xfconf-query not found - is this an Xfce session?" >&2
    exit 1
fi
if ! ls /usr/lib/*/xfce4/panel/plugins/libgenmon.so >/dev/null 2>&1; then
    echo "xfce4-genmon-plugin is not installed. Run:" >&2
    echo "  sudo apt install xfce4-genmon-plugin" >&2
    exit 1
fi

panel_stop() { xfce4-panel --quit >/dev/null 2>&1 || true; sleep 2; }
panel_start() { nohup xfce4-panel >/dev/null 2>&1 & sleep 4; }

plugin_ids() {
    xfconf-query -c xfce4-panel -p "/panels/$PANEL/plugin-ids" \
        | tail -n +3 | grep -v '^$'
}

set_ids() {   # set_ids "1 2 3"
    args=""
    for i in $1; do args="$args -t int -s $i"; done
    # shellcheck disable=SC2086
    xfconf-query -c xfce4-panel -p "/panels/$PANEL/plugin-ids" -n -a $args
}

find_existing() {
    for p in $(xfconf-query -c xfce4-panel -l 2>/dev/null \
               | grep -oE '/plugins/plugin-[0-9]+/command' || true); do
        cmd=$(xfconf-query -c xfce4-panel -p "$p" 2>/dev/null || true)
        case "$cmd" in
            *ai-usage-genmon*) echo "$p" | grep -oE '[0-9]+' | head -1; return;;
        esac
    done
}

# --- remove -------------------------------------------------------------
if [ "${1:-}" = "--remove" ]; then
    id=$(find_existing || true)
    if [ -z "${id:-}" ]; then
        echo "widget not found in the panel; nothing to do"
        exit 0
    fi
    cp -p "$BACKUP_DIR/xfce4-panel.xml" "$BACKUP"
    echo "panel config backed up to $BACKUP"
    keep=$(plugin_ids | grep -vx "$id" | tr '\n' ' ')
    panel_stop
    set_ids "$keep"
    xfconf-query -c xfce4-panel -p "/plugins/plugin-$id" -r -R 2>/dev/null || true
    rm -f "$RCDIR/genmon-$id.rc"
    panel_start
    echo "removed plugin-$id"
    exit 0
fi

# --- add ----------------------------------------------------------------
existing=$(find_existing || true)
cp -p "$BACKUP_DIR/xfce4-panel.xml" "$BACKUP"
echo "panel config backed up to $BACKUP"

if [ -n "${existing:-}" ]; then
    id="$existing"
    ids=$(plugin_ids | tr '\n' ' ')
    echo "widget already present as plugin-$id - refreshing its settings"
else
    last=$(xfconf-query -c xfce4-panel -l 2>/dev/null \
           | grep -oE '/plugins/plugin-[0-9]+' | grep -oE '[0-9]+$' \
           | sort -n | tail -1)
    id=$(( ${last:-0} + 1 ))
    ids="$(plugin_ids | tr '\n' ' ') $id"
    echo "adding as plugin-$id"
fi

# everything below happens with the panel DOWN, or it will not stick
panel_stop
xfconf-query -c xfce4-panel -p "/plugins/plugin-$id" -n -t string -s genmon 2>/dev/null || true
xfconf-query -c xfce4-panel -p "/plugins/plugin-$id/command" \
             -n -t string -s "$GENMON_CMD" 2>/dev/null \
  || xfconf-query -c xfce4-panel -p "/plugins/plugin-$id/command" -s "$GENMON_CMD"
xfconf-query -c xfce4-panel -p "/plugins/plugin-$id/update-period" \
             -n -t int -s "$UPDATE_PERIOD_MS" 2>/dev/null \
  || xfconf-query -c xfce4-panel -p "/plugins/plugin-$id/update-period" -s "$UPDATE_PERIOD_MS"
# our script emits its own <txt>; genmon's built-in label would just duplicate it
xfconf-query -c xfce4-panel -p "/plugins/plugin-$id/use-label" \
             -n -t bool -s false 2>/dev/null \
  || xfconf-query -c xfce4-panel -p "/plugins/plugin-$id/use-label" -s false
set_ids "$ids"

# prime the cache so the widget has real numbers on first poll
"$HERE/ai-usage-refresh" >/dev/null 2>&1 &

panel_start

echo
echo "Installed. Widget shows e.g.   C ~31%5h  X 3%wk"
echo "  C = Claude (~ = estimate, not a real meter), X = Codex"
echo "  green <60%, amber >=60%, bold red >=85%, grey = stale"
echo "  click it for full claude-usage / codex-usage output"
echo
echo "To remove:  $HERE/install.sh --remove"
