"""Read the current Codex account's live quota through ``codex app-server``.

The app-server owns Codex authentication and exposes ``account/rateLimits/read``.
Using it matters when accounts are switched: session transcripts contain quota
snapshots but do not identify the account that produced them, so the newest
transcript is not necessarily for the account in ``~/.codex/auth.json``.

This also returns additional model-specific limits, purchased-credit state and
available full-reset credits, none of which can be reconstructed reliably from
the transcript files.
"""

import json
import os
import select
import shutil
import subprocess
import time


TIMEOUT = 20


def _codex_command():
    return shutil.which("codex") or os.path.expanduser("~/.local/bin/codex")


def _stored_account_id():
    """Read only the non-secret account selector used by current Codex auth."""
    path = os.path.join(os.environ.get("CODEX_HOME", os.path.expanduser("~/.codex")),
                        "auth.json")
    try:
        with open(path) as auth_file:
            auth = json.load(auth_file)
        return (auth.get("tokens") or {}).get("account_id")
    except Exception:
        return None


def _send(proc, message):
    proc.stdin.write(json.dumps(message, separators=(",", ":")) + "\n")
    proc.stdin.flush()


def _read_response(proc, request_id, deadline):
    """Read JSONL until the requested response arrives; ignore notifications."""
    while time.monotonic() < deadline:
        remaining = deadline - time.monotonic()
        readable, _, _ = select.select([proc.stdout], [], [], max(0, remaining))
        if not readable:
            break
        line = proc.stdout.readline()
        if not line:
            break
        try:
            message = json.loads(line)
        except json.JSONDecodeError:
            continue
        if message.get("id") == request_id:
            if message.get("error"):
                err = message["error"]
                raise RuntimeError(err.get("message") or str(err))
            return message.get("result") or {}
    raise TimeoutError("timed out waiting for Codex app-server")


def _rpc_rate_limits():
    proc = subprocess.Popen(
        [_codex_command(), "app-server", "--stdio"],
        stdin=subprocess.PIPE, stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL, text=True, bufsize=1)
    deadline = time.monotonic() + TIMEOUT
    try:
        _send(proc, {
            "jsonrpc": "2.0", "id": 1, "method": "initialize",
            "params": {
                "clientInfo": {
                    "name": "ai_usage_panel", "title": "AI usage panel",
                    "version": "1",
                },
                "capabilities": {},
            },
        })
        _read_response(proc, 1, deadline)
        _send(proc, {
            "jsonrpc": "2.0", "method": "initialized", "params": {},
        })
        _send(proc, {
            "jsonrpc": "2.0", "id": 2,
            "method": "account/rateLimits/read", "params": {},
        })
        return _read_response(proc, 2, deadline)
    finally:
        try:
            proc.stdin.close()
        except (BrokenPipeError, OSError):
            pass
        try:
            proc.terminate()
            proc.wait(timeout=2)
        except (OSError, subprocess.TimeoutExpired):
            proc.kill()
            proc.wait()


def _window_label(minutes):
    if minutes is None:
        return "?"
    if minutes >= 10080:
        return "weekly"
    if minutes >= 1440:
        return "%dd" % round(minutes / 1440)
    if minutes >= 60:
        return "%dh" % round(minutes / 60)
    return "%dm" % round(minutes)


def _short_limit_name(name):
    """Keep model-specific labels useful without making the tooltip enormous."""
    if not name:
        return None
    for prefix in ("GPT-", "Codex-"):
        if name.startswith(prefix):
            name = name[len(prefix):]
    return name.replace("Codex-", "")


def _parse(result):
    """Convert the app-server response to the panel's provider schema."""
    by_id = result.get("rateLimitsByLimitId") or {}
    general = result.get("rateLimits") or {}
    if general and (general.get("limitId") or "codex") not in by_id:
        by_id = dict(by_id)
        by_id.setdefault(general.get("limitId") or "codex", general)

    windows = []
    limits = []
    for limit_id, snapshot in by_id.items():
        if not isinstance(snapshot, dict):
            continue
        limit_name = snapshot.get("limitName")
        qualifier = (None if limit_id == "codex" else
                     (_short_limit_name(limit_name) or limit_id))
        for slot in ("primary", "secondary"):
            window = snapshot.get(slot)
            if not isinstance(window, dict) or window.get("usedPercent") is None:
                continue
            label = _window_label(window.get("windowDurationMins"))
            if qualifier:
                label += ":" + qualifier
            windows.append({
                "label": label,
                "pct": float(window["usedPercent"]),
                "resets_at": window.get("resetsAt"),
                "estimated": False,
                "limit_id": limit_id,
            })
        limits.append({
            "id": limit_id,
            "name": limit_name,
            "spend_control_reached": snapshot.get("spendControlReached"),
            "rate_limit_reached_type": snapshot.get("rateLimitReachedType"),
            "individual_limit": snapshot.get("individualLimit"),
        })

    reset = result.get("rateLimitResetCredits") or {}
    return {
        "ok": bool(windows),
        "source": "api",
        "account_id": result.get("accountId"),
        "plan": general.get("planType"),
        "windows": windows,
        "credits": general.get("credits"),
        "reset_credits": reset.get("availableCount"),
        "ordinary_usage_allowed": result.get("ordinaryUsageAllowed"),
        "limits": limits,
        "error": None if windows else "usage response had no windows",
    }


def fetch():
    try:
        return _parse(_rpc_rate_limits())
    except Exception as exc:
        return {
            "ok": False, "source": "api", "account_id": _stored_account_id(),
            "plan": None, "windows": [], "credits": None,
            "reset_credits": None, "ordinary_usage_allowed": None,
            "limits": [], "error": "%s: %s" % (type(exc).__name__, exc),
        }


if __name__ == "__main__":
    # Deliberately omit the account id from ad-hoc output.
    data = fetch()
    data.pop("account_id", None)
    print(json.dumps(data, indent=2))
