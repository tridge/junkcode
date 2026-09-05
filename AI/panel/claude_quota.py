"""Read Claude's REAL account meters from the OAuth usage endpoint.

Claude Code stores no quota figures on disk, so the local-token estimate in
ai_usage was the only number the panel had - and it is badly wrong in both
directions: it knows nothing about usage from other machines or claude.ai, and
it has no notion of the weekly windows at all. A session sitting at 5% by the
local estimate was in fact 78% through the week and completely out of Fable.

The same endpoint /usage uses (GET /api/oauth/usage) returns all of it, so ask
for it directly with the OAuth token Claude Code already keeps in
~/.claude/.credentials.json. Read-only, one small request per refresh.

The interesting part of the response is `limits`: one entry per window with
kind/percent/resets_at, plus an optional `scope` naming the model a window
applies to (the per-model weekly cap). The older flat five_hour/seven_day keys
are still there and are used as a fallback, but they cannot express a scoped
window, which is exactly the one that runs out first.
"""

import datetime
import json
import os
import urllib.error
import urllib.request

CRED = os.path.expanduser("~/.claude/.credentials.json")
URL = "https://api.anthropic.com/api/oauth/usage"
TIMEOUT = 15


def _token():
    """The stored OAuth access token, or None.

    Never refreshes it: the refresh token is Claude Code's to spend, and racing
    it here could invalidate the session the user is sitting in. An expired
    token just means we fall back to the estimate until Claude Code renews it.
    """
    env = os.environ.get("CLAUDE_CODE_OAUTH_TOKEN")
    if env:
        return env
    with open(CRED) as f:
        d = json.load(f)
    for k in ("claudeAiOauth", "oauth", "claudeAi"):
        if isinstance(d.get(k), dict):
            d = d[k]
            break
    return d.get("accessToken") or d.get("access_token")


def _epoch(iso):
    if not iso:
        return None
    try:
        return datetime.datetime.fromisoformat(iso.replace("Z", "+00:00")).timestamp()
    except ValueError:
        return None


def _label(lim):
    """Name a window from its kind, qualified by scope where there is one."""
    kind = lim.get("kind")
    base = {"session": "5h", "weekly_all": "weekly"}.get(kind)
    if base is None:
        base = "weekly" if str(kind).startswith("weekly") else str(kind)
    scope = lim.get("scope") or {}
    model = (scope.get("model") or {}).get("display_name")
    surface = scope.get("surface")
    tag = model or surface
    return "%s:%s" % (base, tag) if tag else base


def _windows(js):
    out = []
    for lim in js.get("limits") or []:
        pct = lim.get("percent")
        if pct is None:
            continue
        out.append({"label": _label(lim),
                    "pct": float(pct),
                    "resets_at": _epoch(lim.get("resets_at")),
                    "estimated": False})
    if out:
        return out
    # older response shape, no scoped windows
    for key, label in (("five_hour", "5h"), ("seven_day", "weekly")):
        w = js.get(key)
        if not isinstance(w, dict) or w.get("utilization") is None:
            continue
        out.append({"label": label,
                    "pct": float(w["utilization"]),
                    "resets_at": _epoch(w.get("resets_at")),
                    "estimated": False})
    return out


def fetch():
    """{'ok', 'windows': [...], 'extra_pct', 'error'}"""
    out = {"ok": False, "windows": [], "extra_pct": None, "error": None}
    try:
        tok = _token()
        if not tok:
            out["error"] = "no OAuth token in %s" % CRED
            return out
        req = urllib.request.Request(URL, headers={
            "Authorization": "Bearer %s" % tok,
            "anthropic-beta": "oauth-2025-04-20",
            "Content-Type": "application/json",
        })
        with urllib.request.urlopen(req, timeout=TIMEOUT) as r:
            js = json.load(r)
        ws = _windows(js)
        if not ws:
            out["error"] = "usage response had no windows"
            return out
        out["ok"] = True
        out["windows"] = ws
        extra = js.get("extra_usage") or {}
        if extra.get("is_enabled") and extra.get("utilization") is not None:
            out["extra_pct"] = float(extra["utilization"])
    except urllib.error.HTTPError as e:
        # 401 is the normal "token has expired, Claude Code will renew it" case
        out["error"] = "HTTP %s" % e.code
    except Exception as e:
        out["error"] = "%s: %s" % (type(e).__name__, e)
    return out
