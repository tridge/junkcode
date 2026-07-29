"""Collect Claude Code and Codex CLI usage into one small dict.

Reuses ~/bin/claude-usage and ~/bin/codex-usage rather than reimplementing
their parsing: both guard their main() with __name__ == "__main__", so they
import cleanly and we call their scan() directly. They have no .py extension,
so the import needs an explicit SourceFileLoader.

The two providers expose very different things:

  codex  - a REAL account meter, read straight out of the session files
           (rate_limits.primary / .secondary, each with used_percent,
           window_minutes and resets_at). Trustworthy.
  claude - no meter is stored locally anywhere, so the 5h figure is an
           ESTIMATE: local token usage over the trailing 5h expressed as a
           share of a ceiling that is either calibrated (claude-usage
           --calibrate) or guessed from the largest historical 5h burst.
           There is no weekly figure available at all.

Do not label a codex window from its primary/secondary position - the API
puts the weekly window in `primary` at least some of the time. Always derive
the label from window_minutes.
"""

import importlib.util
import os
from importlib.machinery import SourceFileLoader

HOME = os.path.expanduser("~")
CLAUDE_USAGE = os.path.join(HOME, "bin", "claude-usage")
CODEX_USAGE = os.path.join(HOME, "bin", "codex-usage")


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
    """{'ok', 'windows': [...], 'calibrated': bool, 'error'}"""
    out = {"ok": False, "windows": [], "calibrated": False, "error": None}
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
    """{'ok', 'plan', 'meter_age_s', 'windows': [...], 'error'}"""
    out = {"ok": False, "plan": None, "meter_age_s": None,
           "windows": [], "error": None}
    try:
        xu = _load("_codex_usage", CODEX_USAGE)
        _sessions, latest_rl, now = xu.scan()
        if not latest_rl:
            out["error"] = "no rate-limit data in session files"
            return out
        epoch, rl = latest_rl
        out["ok"] = True
        out["plan"] = rl.get("plan_type")
        out["meter_age_s"] = max(0, now.timestamp() - epoch)
        # label by window_minutes, NOT by primary/secondary position
        for key in ("primary", "secondary"):
            w = rl.get(key)
            if not w:
                continue
            out["windows"].append({
                "label": _window_label(w.get("window_minutes")),
                "pct": float(w.get("used_percent") or 0.0),
                "resets_at": w.get("resets_at"),
                "estimated": False,
            })
    except Exception as e:
        out["error"] = "%s: %s" % (type(e).__name__, e)
    return out


def collect():
    import time
    return {"generated_at": time.time(),
            "claude": claude_status(),
            "codex": codex_status()}


def worst(provider):
    """The window closest to its limit, or None."""
    ws = [w for w in provider.get("windows", []) if w.get("pct") is not None]
    return max(ws, key=lambda w: w["pct"]) if ws else None


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
