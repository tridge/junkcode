"""Collect Claude Code and Codex CLI usage into one small dict.

Reuses ~/bin/claude-usage for Claude's local estimate. Codex's live meter is
read through the installed CLI's app-server, which follows the currently
authenticated account and includes additional model limits and reset credits.

The two providers expose very different things:

  codex  - REAL account meters fetched by ``account/rateLimits/read`` from the
           installed Codex app-server. Unlike session files, this identifies
           the current account and includes all separately metered limits.
  claude - a REAL account meter too, but only over the network: nothing is
           stored locally, so claude_quota asks the same endpoint /usage uses.
           When that fails we fall back to an ESTIMATE from local token usage
           over the trailing 5h, as a share of a ceiling that is either
           calibrated (claude-usage --calibrate) or guessed from the largest
           historical burst. The estimate sees only this machine and knows
           nothing about the weekly windows, so treat it as a rough floor.

Do not label a window from its primary/secondary position. Always derive the
label from its duration.
"""

import importlib.util
import os
import sys
from importlib.machinery import SourceFileLoader

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import claude_quota  # noqa: E402
import codex_quota  # noqa: E402

HOME = os.path.expanduser("~")
CLAUDE_USAGE = os.path.join(HOME, "bin", "claude-usage")


def _load(name, path):
    spec = importlib.util.spec_from_loader(name, SourceFileLoader(name, path))
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def _window_label(minutes):
    """Name a rate-limit window from its length."""
    if minutes is None:
        return "?"
    if minutes >= 10080:
        return "weekly"
    if minutes >= 1440:
        return "%dd" % round(minutes / 1440)
    return "%dh" % round(minutes / 60)


def claude_status():
    """{'ok', 'source', 'windows': [...], 'calibrated': bool, 'error'}

    Real account meters when the endpoint answers, local estimate otherwise.
    """
    q = claude_quota.fetch()
    if q["ok"]:
        return {"ok": True, "source": "api", "windows": q["windows"],
                "calibrated": True, "extra_pct": q["extra_pct"], "error": None}
    out = _claude_estimate()
    # keep why the real meter was unavailable; the estimate is much weaker
    out["error"] = "meter unavailable (%s)%s" % (
        q["error"], "; " + out["error"] if out["error"] else "")
    if q.get("throttled"):
        # a throttle says nothing about the account: the last reading is still
        # good, only the refresh is blocked. Name it as such.
        out["throttled"] = True
        out["retry_in"] = q.get("retry_in")
        out["error"] = "meter throttled (HTTP 429), retrying in %s" % (
            human_secs(q.get("retry_in")))
    return out


def _claude_estimate():
    """Local-token fallback: 5h only, this machine only."""
    out = {"ok": False, "source": "estimate", "windows": [],
           "calibrated": False, "extra_pct": None, "error": None}
    try:
        cu = _load("_claude_usage", CLAUDE_USAGE)
        sessions, events, now = cu.scan()
        nowt = now.timestamp()
        cur = sum(cu.win_sum(r, nowt - cu.FIVE_H) for r in sessions.values())
        cfg = cu.load_config()

        ceiling = cfg.get("five_hour_units")
        calibrated = bool(ceiling)
        if not ceiling:
            # same fallback claude-usage uses when uncalibrated
            ceiling = cu.sliding_peak(events)
        # an uncalibrated ceiling can be exceeded by a later, larger burst
        peak = cu.sliding_peak(events)
        if calibrated and peak > ceiling:
            ceiling = peak
            calibrated = False

        pct = (100.0 * cur / ceiling) if ceiling else 0.0
        out["ok"] = True
        out["calibrated"] = calibrated
        out["windows"] = [{
            "label": "5h",
            "pct": pct,
            "resets_at": None,       # claude stores no reset time locally
            "estimated": True,       # always an estimate, calibrated or not
        }]
    except Exception as e:  # never let the panel die on a parse change
        out["error"] = "%s: %s" % (type(e).__name__, e)
    return out


def codex_status():
    """Live quota for the account currently selected by Codex."""
    return codex_quota.fetch()


def collect():
    import time
    return {"generated_at": time.time(),
            "claude": claude_status(),
            "codex": codex_status()}


def worst(provider):
    """The window closest to its limit, or None."""
    ws = [w for w in provider.get("windows", []) if w.get("pct") is not None]
    return max(ws, key=lambda w: w["pct"]) if ws else None


def human_secs(s):
    """A short duration: 45s, 2m, 8m30s."""
    s = int(s or 0)
    if s < 60:
        return "%ds" % s
    m, s = divmod(s, 60)
    return "%dm%02ds" % (m, s) if s else "%dm" % m


def human_until(epoch, now=None):
    import time
    if not epoch:
        return "?"
    now = now if now is not None else time.time()
    s = int(epoch - now)
    if s <= 0:
        return "now"
    d, s = divmod(s, 86400)
    h, s = divmod(s, 3600)
    m = s // 60
    if d:
        return "%dd%dh" % (d, h)
    if h:
        return "%dh%dm" % (h, m)
    return "%dm" % m


def _report(data=None):
    """Plain-text dump of both providers, for the click-through terminal.

    Worth printing there because `claude-usage` alone shows only the local
    estimate, which now disagrees with what the panel is showing.
    """
    import time
    d = data if data is not None else collect()
    now = time.time()
    for name, key in (("Claude", "claude"), ("Codex", "codex")):
        prov = d[key]
        print("%s:" % name)
        if prov.get("error"):
            print("  %s" % prov["error"])
        for w in sorted(prov["windows"], key=lambda x: -x["pct"]):
            line = "  %-14s %5.1f%% used,  %5.1f%% left" % (
                w["label"], w["pct"], max(0.0, 100.0 - w["pct"]))
            if w["resets_at"]:
                line += "   resets in %s" % human_until(w["resets_at"], now)
            print(line)
        if not prov["windows"]:
            print("  no data")
        if key == "codex":
            credits = prov.get("credits") or {}
            if credits.get("unlimited"):
                print("  purchased credits: unlimited")
            elif credits:
                print("  purchased credits: %s" % credits.get("balance", "?"))
            resets = prov.get("reset_credits")
            if resets is not None:
                print("  full reset credits: %s" % resets)


if __name__ == "__main__":
    if "--cache" in sys.argv:
        cache = os.path.join(
            os.environ.get("XDG_CACHE_HOME", os.path.expanduser("~/.cache")),
            "ai-usage", "status.json")
        try:
            with open(cache) as cache_file:
                _report(__import__("json").load(cache_file))
        except Exception as exc:
            print("unable to read %s: %s" % (cache, exc), file=sys.stderr)
            sys.exit(1)
    else:
        _report()
