#!/usr/bin/env python3
"""Render claude -p --output-format stream-json as a readable, line-buffered log.

One line per event so `tail -f` is useful while a run is in flight. Every line is
flushed immediately; nothing is held in a buffer waiting for the run to end.
"""
import json, sys, time

def ts():
    return time.strftime('%H:%M:%S')

def short(s, n=140):
    s = ' '.join(str(s).split())
    return s if len(s) <= n else s[:n-1] + '…'

def emit(line):
    sys.stdout.write(line + '\n')
    sys.stdout.flush()

TOOL_ARG = {
    'Bash': 'command', 'Read': 'file_path', 'Edit': 'file_path', 'Write': 'file_path',
    'Grep': 'pattern', 'Glob': 'pattern', 'Task': 'description', 'Agent': 'description',
    'WebFetch': 'url', 'Skill': 'skill',
}

for raw in sys.stdin:
    raw = raw.strip()
    if not raw:
        continue
    try:
        ev = json.loads(raw)
    except Exception:
        emit('%s | raw | %s' % (ts(), short(raw, 200)))
        continue

    t = ev.get('type')

    if t == 'system' and ev.get('subtype') == 'init':
        emit('%s | init | model=%s cwd=%s tools=%d' % (
            ts(), ev.get('model'), ev.get('cwd'), len(ev.get('tools') or [])))

    elif t == 'assistant':
        for c in (ev.get('message', {}).get('content') or []):
            if c.get('type') == 'text' and c.get('text', '').strip():
                for ln in c['text'].rstrip().split('\n'):
                    if ln.strip():
                        emit('%s | say  | %s' % (ts(), ln.rstrip()))
            elif c.get('type') == 'tool_use':
                name = c.get('name', '?')
                inp = c.get('input') or {}
                key = TOOL_ARG.get(name)
                arg = inp.get(key) if key else None
                if arg is None:
                    arg = next((v for v in inp.values() if isinstance(v, str)), '')
                emit('%s | TOOL | %-10s %s' % (ts(), name, short(arg)))

    elif t == 'user':
        for c in (ev.get('message', {}).get('content') or []):
            if c.get('type') == 'tool_result':
                body = c.get('content')
                if isinstance(body, list):
                    body = ' '.join(b.get('text', '') for b in body if isinstance(b, dict))
                body = short(body or '', 120)
                flag = 'ERR ' if c.get('is_error') else 'res '
                emit('%s | %s | %s' % (ts(), flag, body))

    elif t == 'result':
        # On a fixed monthly plan the dollar figure is an API-list-price
        # equivalent, not a charge. Token counts are the number that actually
        # relates to plan usage, so log those first and keep the $ as a rough
        # size proxy only.
        u = ev.get('usage') or {}
        def g(*names):
            for n in names:
                if u.get(n) is not None:
                    return u[n]
            return 0
        tin = g('input_tokens')
        tout = g('output_tokens')
        cr = g('cache_read_input_tokens', 'cache_read_tokens')
        cw = g('cache_creation_input_tokens', 'cache_creation_tokens')
        emit('%s | ---- | %s  duration=%ss  turns=%s  tokens: in=%s out=%s cache_read=%s cache_write=%s  (api-equiv $%s)' % (
            ts(), ev.get('subtype'),
            round((ev.get('duration_ms') or 0) / 1000),
            ev.get('num_turns'), tin, tout, cr, cw,
            round(ev.get('total_cost_usd') or 0, 2)))
        res = ev.get('result')
        if res:
            for ln in str(res).rstrip().split('\n'):
                emit('%s | FINAL| %s' % (ts(), ln.rstrip()))
