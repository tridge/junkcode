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
import email.utils
import json
import os
import tempfile
import time
import urllib.error
import urllib.request

CRED = os.path.expanduser("~/.claude/.credentials.json")
URL = "https://api.anthropic.com/api/oauth/usage"
TIMEOUT = 15

# The endpoint is metered per account and every Claude client on it polls the
# same one, so it does hand out 429s. Retrying on the panel's ordinary 90s
# cadence through a throttle just keeps the window topped up - on 2026-09-13 it
# stayed refused for 81 minutes. Back off instead, and remember the deadline on
# disk so every process that reads the meter honours the same one.
THROTTLE_STATE = "claude-throttle.json"
BACKOFF_FIRST = 120       # wait after the first 429
BACKOFF_MAX = 600         # ceiling on the doubling
RETRY_AFTER_MAX = 3600    # trust a server Retry-After only this far


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


def _state_path():
    cache = os.environ.get("XDG_CACHE_HOME", os.path.expanduser("~/.cache"))
    return os.path.join(cache, "ai-usage", THROTTLE_STATE)


def _read_state():
    """(consecutive 429s, epoch before which not to ask again)."""
    try:
        with open(_state_path()) as f:
            d = json.load(f)
        return int(d.get("fails", 0)), float(d.get("until", 0))
    except Exception:
        return 0, 0.0


def _write_state(fails, until):
    path = _state_path()
    try:
        os.makedirs(os.path.dirname(path), exist_ok=True)
        fd, tmp = tempfile.mkstemp(dir=os.path.dirname(path), prefix=".throttle-")
        with os.fdopen(fd, "w") as f:
            json.dump({"fails": fails, "until": until}, f)
        os.replace(tmp, path)          # atomic: readers never see a half file
    except Exception:
        pass                           # the meter must not die over its own cache


def _clear_state():
    try:
        os.unlink(_state_path())
    except OSError:
        pass


def _backoff_delay(fails, retry_after=None):
    """Seconds to wait after `fails` consecutive 429s. Server wins if it said."""
    if retry_after:
        return int(min(retry_after, RETRY_AFTER_MAX))
    return int(min(BACKOFF_FIRST * 2 ** max(0, fails - 1), BACKOFF_MAX))


def _retry_after(err):
    """Retry-After as seconds, from either the delta or HTTP-date form."""
    raw = (err.headers.get("Retry-After") if err.headers else None)
    if not raw:
        return None
    raw = raw.strip()
    try:
        return max(0, int(raw))
    except ValueError:
        pass
    try:
        when = email.utils.parsedate_to_datetime(raw)
    except (TypeError, ValueError):
        return None
    if when is None:
        return None
    if when.tzinfo is None:
        when = when.replace(tzinfo=datetime.timezone.utc)
    return max(0, int(when.timestamp() - time.time()))


def throttled_for(now=None):
    """Seconds still to wait before the endpoint may be asked again, or 0."""
    now = time.time() if now is None else now
    _, until = _read_state()
    return max(0, int(until - now))


def fetch():
    """{'ok', 'windows': [...], 'extra_pct', 'error', 'throttled', 'retry_in'}"""
    out = {"ok": False, "windows": [], "extra_pct": None, "error": None,
           "throttled": False, "retry_in": None}

    # still inside a backoff: say so without spending a request on it
    waiting = throttled_for()
    if waiting:
        out.update(throttled=True, retry_in=waiting, error="HTTP 429")
        return out
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
        _clear_state()
        extra = js.get("extra_usage") or {}
        if extra.get("is_enabled") and extra.get("utilization") is not None:
            out["extra_pct"] = float(extra["utilization"])
    except urllib.error.HTTPError as e:
        # 401 is the normal "token has expired, Claude Code will renew it" case
        out["error"] = "HTTP %s" % e.code
        if e.code == 429:
            fails = _read_state()[0] + 1
            delay = _backoff_delay(fails, _retry_after(e))
            _write_state(fails, time.time() + delay)
            out.update(throttled=True, retry_in=delay)
    except Exception as e:
        out["error"] = "%s: %s" % (type(e).__name__, e)
    return out
