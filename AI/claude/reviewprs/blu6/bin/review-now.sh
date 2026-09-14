#!/bin/bash
#
# On-demand review run. Waits for the run lock instead of skipping, because a
# run you asked for should happen even if cron is busy.
#
#   review-now.sh                                                # four-label sweep
#   review-now.sh followup
#   review-now.sh rsync
#   review-now.sh DevCallTopic                                   # any label
#   review-now.sh https://github.com/ArduPilot/ardupilot/pull/34206
#   review-now.sh ArduPilot/ardupilot_wiki#8018
#   review-now.sh ardupilot_wiki#8018
#   review-now.sh 34206                                          # main repo
#
# A single PR is reviewed at full depth even if its head has not moved since the
# last comment - that is the point of asking for it by hand.
#
# Waits up to 4 hours for the lock. Ctrl-C only abandons the wait; it does not
# kill whatever run currently holds it.

set -u
export REVIEWPRS_LOCK_WAIT="${REVIEWPRS_LOCK_WAIT:-14400}"
ARG="${1:-all}"

resolve_pr() {
    local a="$1" owner repo num
    # strip anything after the PR number: /files, #issuecomment-..., ?query
    case "$a" in
        https://github.com/*/pull/*|http://github.com/*/pull/*)
            a="${a#*github.com/}"
            owner="${a%%/*}"; a="${a#*/}"
            repo="${a%%/*}";  a="${a#*/pull/}"
            num="${a%%[/#?]*}"
            ;;
        */*\#[0-9]*)                       # owner/repo#N
            owner="${a%%/*}"; a="${a#*/}"
            repo="${a%%#*}";  num="${a#*#}"
            ;;
        *\#[0-9]*)                         # reponame#N
            repo="${a%%#*}";  num="${a#*#}"; owner="ArduPilot"
            [ "$repo" = "upstream-mavlink" ] && { owner="mavlink"; repo="mavlink"; }
            [ "$repo" = "rsync" ]            && owner="RsyncProject"
            ;;
        [0-9]*)                            # bare number -> main repo
            owner="ArduPilot"; repo="ardupilot"; num="$a"
            ;;
        *) return 1 ;;
    esac
    case "$num" in ''|*[!0-9]*) return 1 ;; esac
    printf '%s/%s#%s\n' "$owner" "$repo" "$num"
}

if PR=$(resolve_pr "$ARG"); then
    OWNER_REPO="${PR%#*}"; NUM="${PR#*#}"
    echo "Resolved to PR $OWNER_REPO#$NUM - verifying it exists..."
    if ! INFO=$(gh pr view "$NUM" --repo "$OWNER_REPO" \
                  --json number,title,state,isDraft,headRefOid 2>&1); then
        echo "ERROR: no such PR $OWNER_REPO#$NUM" >&2
        echo "$INFO" | head -3 >&2
        exit 1
    fi
    python3 -c '
import json,sys
d=json.loads(sys.argv[1])
print("  #%s  %s" % (d["number"], d["title"][:70]))
print("  state=%s draft=%s head=%s" % (d["state"], d["isDraft"], d["headRefOid"][:10]))
' "$INFO"
    MODE="$PR"
else
    MODE="$ARG"
fi

exec "$HOME/review/bin/run-reviewprs.sh" "$MODE"
