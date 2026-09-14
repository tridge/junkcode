#!/usr/bin/env python3
"""Build runs.html - a dashboard of recent reviewprs runs on blu6.

Data sources, and what is real vs derived:
  * ~/review/logs/reviewprs-*.log - run type, start/finish, duration, outcome,
    lock waits, turns and token counts. All directly recorded.
  * ~/.codex/sessions/**/rollout-*.jsonl - Codex's own rate-limit meter, which
    reports a REAL used_percent for the weekly (10080-minute) window.
  * Claude exposes NO quota meter anywhere - not in the CLI, not in the session
    transcripts. Only token counts. So this page shows Claude tokens and says
    plainly that no percentage is available, rather than inventing one.
"""
import json, os, re, glob, html, datetime, sys

HOME = os.path.expanduser('~')
LOGS = os.path.join(HOME, 'review', 'logs')
DAYS = 10
OUT = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HOME, 'review', 'work', 'runs.html')

now = datetime.datetime.now().astimezone()
cutoff = now - datetime.timedelta(days=DAYS)


def parse_iso(s):
    try:
        return datetime.datetime.fromisoformat(s)
    except Exception:
        return None


# ---------------------------------------------------------------- runs
runs = []
for path in sorted(glob.glob(os.path.join(LOGS, 'reviewprs-*.log'))):
    try:
        txt = open(path, errors='replace').read()
    except Exception:
        continue
    m = re.search(r'^reviewprs mode=(\S+)\s+host=\S+\s+start=(\S+)', txt, re.M)
    if not m:
        continue
    mode, start = m.group(1), parse_iso(m.group(2))
    if not start or start < cutoff:
        continue

    r = dict(mode=mode, start=start, finish=None, elapsed=None, status='running',
             rc=None, turns=None, tin=0, tout=0, tcr=0, tcw=0, waited=None, prs=None)

    f = re.search(r'^reviewprs mode=\S+ rc=(\d+) elapsed=(\d+)m finish=(\S+)', txt, re.M)
    if f:
        r['rc'] = int(f.group(1)); r['elapsed'] = int(f.group(2))
        r['finish'] = parse_iso(f.group(3))
        r['status'] = 'ok' if r['rc'] == 0 else 'failed'
    # Every early exit prints "finish=<iso> status=<name>". Parse it for all of
    # them: a run left with finish=None is treated as still running, so its
    # window swallows whatever the next run does.
    ac = re.search(r'claude account: (\S+)', txt)
    if ac and ac.group(1) != 'unknown':
        r['account'] = ac.group(1)
    fi_any = re.search(r'finish=(\S+) status=\S+', txt)
    if fi_any:
        r['finish'] = parse_iso(fi_any.group(1))
    if 'status=skipped-locked' in txt:
        r['status'] = 'skipped'
        r['elapsed'] = 0
        fi = re.search(r'finish=(\S+) status=skipped-locked', txt)
        if fi: r['finish'] = parse_iso(fi.group(1))
    elif 'status=lock-timeout' in txt:
        r['status'] = 'lock-timeout'
    elif 'status=no-gh-auth' in txt:
        r['status'] = 'no-gh-auth'
    elif 'status=wrong-claude-account' in txt:
        # the run refused rather than spend the wrong subscription
        r['status'] = 'wrong-account'
        r['elapsed'] = 0
    elif 'status=quota-exhausted' in txt:
        r['status'] = 'quota'
        r['elapsed'] = 0
    elif 'hit your weekly limit' in txt or 'hit your usage limit' in txt:
        # runs from before the quota pre-flight existed
        r['status'] = 'quota'
        r['elapsed'] = 0

    acq = re.search(r'lock acquired at (\S+)', txt)
    if acq:
        a = parse_iso(acq.group(1))
        if a: r['waited'] = int((a - start).total_seconds() // 60)

    # result line, both old and new formats
    res = re.search(r'\|\s*----\s*\|\s*(\w+).*?turns=(\d+)', txt)
    if res: r['turns'] = int(res.group(2))
    tk = re.search(r'tokens:\s*in=(\d+)\s+out=(\d+)\s+cache_read=(\d+)\s+cache_write=(\d+)', txt)
    if tk:
        r['tin'], r['tout'], r['tcr'], r['tcw'] = (int(tk.group(i)) for i in (1, 2, 3, 4))

    fn = re.search(r'(\d+)\s+re-reviewed', txt)
    if fn: r['prs'] = int(fn.group(1))

    runs.append(r)

runs.sort(key=lambda r: r['start'], reverse=True)

# ------------------------------------------------- codex weekly quota meter
# Each token_count event carries the live meter. window_minutes 10080 == weekly.
quota = []            # (timestamp, used_percent)
reset_epochs = set()
plan = None
for rp in glob.glob(os.path.join(HOME, '.codex', 'sessions', '*', '*', '*', 'rollout-*.jsonl')):
    try:
        mt = datetime.datetime.fromtimestamp(os.path.getmtime(rp)).astimezone()
    except Exception:
        continue
    if mt < cutoff:
        continue
    try:
        for line in open(rp, errors='replace'):
            if 'token_count' not in line or 'rate_limits' not in line:
                continue
            try:
                d = json.loads(line)
            except Exception:
                continue
            p = d.get('payload') or d
            if p.get('type') != 'token_count':
                continue
            rl = (p.get('info') or {}).get('rate_limits') or p.get('rate_limits') or {}
            plan = rl.get('plan_type') or plan
            for slot in ('primary', 'secondary'):
                s = rl.get(slot) or {}
                if s.get('window_minutes') == 10080 and s.get('used_percent') is not None:
                    ts = None
                    raw = d.get('timestamp')
                    if raw:
                        try:
                            ts = datetime.datetime.fromisoformat(
                                raw.replace('Z', '+00:00')).astimezone()
                        except Exception:
                            ts = None
                    quota.append((ts or mt, float(s['used_percent']), s.get('resets_at')))
                    if s.get('resets_at'):
                        reset_epochs.add(s['resets_at'])
    except Exception:
        continue
quota.sort(key=lambda x: x[0])


def quota_at(t):
    """Weekly used_percent as of time t (latest sample at or before t)."""
    if not t:
        return None
    best = None
    for ts, pct, _r in quota:
        if ts <= t:
            best = pct
        else:
            break
    return best


for r in runs:
    end = r['finish'] or now
    a, b = quota_at(r['start']), quota_at(end)
    r['q_end'] = b
    r['q_delta'] = (b - a) if (a is not None and b is not None and b >= a) else None

cur_quota = quota[-1][1] if quota else None

# Burn rate. Group samples by which window they belong to - resets_at is the
# natural key, and is stable to within a few seconds - rather than trying to
# detect resets from the values. Concurrent Codex sessions report slightly
# different snapshots of the same meter, so the series jitters by a point or two
# and any "split on a decrease" heuristic shreds it; within a window the meter
# only really goes up, so take min->max across the window.
burn = None      # percentage points per day
windows = {}
for ts, pc, rst in quota:
    key = round(rst / 300.0) if rst else 0      # bucket to 5 minutes
    windows.setdefault(key, []).append((ts, pc))
best = None
for key, pts in windows.items():
    if len(pts) < 2:
        continue
    pts.sort()
    lo, hi = min(p[1] for p in pts), max(p[1] for p in pts)
    days = (pts[-1][0] - pts[0][0]).total_seconds() / 86400.0
    if days > 0.25 and hi > lo:
        rate = (hi - lo) / days
        if best is None or days > best[0]:
            best = (days, rate, key)
if best:
    burn = best[1]

reset_at = None
if reset_epochs:
    reset_at = datetime.datetime.fromtimestamp(max(reset_epochs)).astimezone()

# Projected usage when the current window closes, at the observed burn rate.
projected = None
if burn is not None and cur_quota is not None and reset_at:
    days_left = (reset_at - now).total_seconds() / 86400.0
    if days_left > 0:
        projected = cur_quota + burn * days_left

# ------------------------------------------------- claude token usage
# Claude exposes no quota meter, so tokens are the only real measure. The result
# line in the log counts the MAIN SESSION ONLY; a review run does most of its work
# in subagents, whose usage lives in separate transcripts. Scanning the transcripts
# picks those up - it is roughly an order of magnitude more than the result line.
#
# The weighted "effective" figure uses Anthropic's published price ratios relative
# to input tokens (output 5x, cache write 1.25x, cache read 0.1x). That is a
# documented proxy for how heavily each token type counts, NOT a quota reading.
W_IN, W_OUT, W_CW, W_CR = 1.0, 5.0, 1.25, 0.1
claude = []      # (timestamp, total_tokens, weighted)
_roots = ['~/.claude', os.environ.get('REVIEW_RSYNC_CLAUDE_DIR',
                                      '~/review/etc/claude-rsync')]
_pats = [os.path.join(r, sub) for r in _roots
         for sub in ('projects/*/*.jsonl', 'projects/*/*/*.jsonl',
                     'projects/*/*/*/*.jsonl')]
_files = set()
for _p in _pats:
    _files.update(glob.glob(os.path.expanduser(_p)))
for f in _files:
    try:
        if datetime.datetime.fromtimestamp(os.path.getmtime(f)).astimezone() < cutoff:
            continue
    except Exception:
        continue
    try:
        fh = open(f, errors='replace')
    except Exception:
        continue
    for line in fh:
        if '"usage"' not in line:
            continue
        try:
            d = json.loads(line)
        except Exception:
            continue
        u = (d.get('message') or {}).get('usage') or {}
        if not u:
            continue
        raw = d.get('timestamp')
        if not raw:
            continue
        try:
            t = datetime.datetime.fromisoformat(raw.replace('Z', '+00:00')).astimezone()
        except Exception:
            continue
        i = u.get('input_tokens', 0) or 0
        o = u.get('output_tokens', 0) or 0
        cr = u.get('cache_read_input_tokens', 0) or 0
        cw = u.get('cache_creation_input_tokens', 0) or 0
        claude.append((t, i + o + cr + cw, i * W_IN + o * W_OUT + cw * W_CW + cr * W_CR))
claude.sort(key=lambda x: x[0])

def claude_between(a, b):
    tot = wt = 0
    for t, n, w in claude:
        if a <= t <= b:
            tot += n; wt += w
    return tot, wt

for r in runs:
    r['ctok'], r['cwt'] = claude_between(r['start'], r['finish'] or now)

week_ago = now - datetime.timedelta(days=7)
cl_week, cl_week_w = claude_between(week_ago, now)
cl_5h, _ = claude_between(now - datetime.timedelta(hours=5), now)
cl_per_day = None
if claude:
    span = (claude[-1][0] - claude[0][0]).total_seconds() / 86400.0
    if span > 0.5:
        cl_per_day = sum(c[1] for c in claude) / span

# Claude's REAL usage meter. `claude -p /usage` works headlessly and reports the
# actual session and weekly percentages, so claude-usage-probe.sh samples it
# before and after every run and hourly in between. No calibration required.
cl_pct = cl_session = cl_resets = cl_share = None
cl_hist = []
_uf = os.path.join(LOGS, 'claude-usage.jsonl')
try:
    for line in open(_uf, errors='replace'):
        try:
            rec = json.loads(line)
            t = datetime.datetime.fromisoformat(rec['at'])
        except Exception:
            continue
        if t >= cutoff:
            cl_hist.append((t, rec))
except Exception:
    pass
cl_hist.sort(key=lambda x: x[0])

# Readings carry the account they were taken from: rsync reviews run on a
# separate subscription with its own windows. Comparing across the two gives
# nonsense (a 1% rsync run read as +51%), so every lookup is per-account, and
# the headline figures use the account most of the work runs on.
def _acct(rec):
    return rec.get('account') or MAIN_ACCOUNT

_counts = {}
for _t, _rec in cl_hist:
    if _rec.get('account'):
        _counts[_rec['account']] = _counts.get(_rec['account'], 0) + 1
MAIN_ACCOUNT = max(_counts, key=_counts.get) if _counts else None

_main = [(t, rec) for t, rec in cl_hist if _acct(rec) == MAIN_ACCOUNT]
if _main:
    last = _main[-1][1]
    cl_pct = last.get('week_pct')
    cl_session = last.get('session_pct')
    cl_resets = last.get('week_resets')
    cl_share = last.get('reviewprs_share_pct')


def cl_week_at(t, account=None):
    """Weekly percentage for one account as of time t, nearest earlier sample."""
    want = account or MAIN_ACCOUNT
    best = None
    for ts, rec in cl_hist:
        if _acct(rec) != want:
            continue
        if ts <= t and rec.get('week_pct') is not None:
            best = rec['week_pct']
        elif ts > t:
            break
    return best


for r in runs:
    acct = r.get('account')
    a = cl_week_at(r['start'], acct)
    b = cl_week_at(r['finish'] or now, acct)
    r['cl_delta'] = (b - a) if (a is not None and b is not None and b >= a) else None

# Claude weekly burn, measured the same way as the Codex one.
cl_burn = None
if len(cl_hist) > 1:
    pts = [(t, rec['week_pct']) for t, rec in cl_hist
           if rec.get('week_pct') is not None and _acct(rec) == MAIN_ACCOUNT]
    segs, cur = [], []
    for t, pc in pts:
        if cur and pc < cur[-1][1] - 5:      # weekly reset
            segs.append(cur); cur = []
        cur.append((t, pc))
    if cur: segs.append(cur)
    bestseg = max(segs, key=len, default=None)
    if bestseg and len(bestseg) > 1:
        days = (bestseg[-1][0] - bestseg[0][0]).total_seconds() / 86400.0
        rise = max(p[1] for p in bestseg) - min(p[1] for p in bestseg)
        if days > 0.2 and rise > 0:
            cl_burn = rise / days

# ---------------------------------------------------------------- summary
by_mode = {}
for r in runs:
    m = by_mode.setdefault(r['mode'], dict(n=0, ok=0, skip=0, fail=0, mins=[], tok=0))
    m['n'] += 1
    if r['status'] == 'ok':
        m['ok'] += 1
        if r['elapsed'] is not None: m['mins'].append(r['elapsed'])
    elif r['status'] == 'skipped': m['skip'] += 1
    elif r['status'] not in ('running',): m['fail'] += 1
    m['tok'] += r['tin'] + r['tout'] + r['tcr'] + r['tcw']


def med(v):
    if not v: return None
    v = sorted(v); n = len(v)
    return v[n // 2] if n % 2 else (v[n // 2 - 1] + v[n // 2]) / 2


def fmt_tok(n):
    if not n: return '&mdash;'
    for u, d in (('B', 1e9), ('M', 1e6), ('k', 1e3)):
        if n >= d: return '%.1f%s' % (n / d, u)
    return str(n)


# ------------------------------------------------- label coverage
# DevCallEU and AIReview never appear as their own run rows: they only ever run
# as sub-runs INSIDE an `all` sweep, which writes one combined log. So "which
# labels are actually being swept, and how recently" cannot be read from the run
# logs. The published reports are the ground truth - their manifest is exactly
# what the next run reads to decide what to skip.
import urllib.request
labels = []
for lab in ('DevCallTopic', 'DevCallEU', 'AIReview'):
    url = 'https://uav.tridgell.net/DevCallReviews/%s/devcall_pr_reviews.html' % lab
    try:
        with urllib.request.urlopen(url, timeout=20) as fh:
            body = fh.read().decode('utf-8', 'replace')
    except Exception:
        labels.append((lab, None, None, None, None, None, None))
        continue
    m = re.search(r'reviewprs-manifest v1[^>]*generated="([^"]*)"', body)
    gen = m.group(1) if m else None
    m = re.search(r'followup="([^"]*)"', body)
    fup = m.group(1) if m else None
    h = re.search(r'heads="([^"]*)"', body)
    npr = len(h.group(1).split()) if h else None
    # exactly one verdict badge per PR section meta line
    a = len(re.findall(r'Verdict: <span class="v-approve"', body))
    c = len(re.findall(r'Verdict: <span class="v-comment"', body))
    rc = len(re.findall(r'Verdict: <span class="v-request"', body))
    labels.append((lab, gen, fup, npr, a, c, rc))

lrows = []
for lab, gen, fup, npr, a, c, rc in labels:
    if gen is None:
        lrows.append('<tr><td>%s</td><td colspan="4">unreachable</td></tr>' % lab)
        continue
    stamp = gen + (' <span class="muted">+ followup %s</span>' % html.escape(fup) if fup else '')
    lrows.append('<tr><td><a href="/DevCallReviews/%s/devcall_pr_reviews.html">%s</a></td>'
                 '<td>%s</td><td>%s</td><td>%s</td>'
                 '<td>%d / %d / %d</td></tr>' % (
                     lab, lab, stamp, npr if npr is not None else '?',
                     'sub-run of <code>all</code>' if lab != 'DevCallTopic' else
                     'sub-run of <code>all</code> + manual',
                     a, c, rc))

BADGE = {'ok': 'b-ok', 'skipped': 'b-skip', 'running': 'b-run', 'quota': 'b-quota',
         'failed': 'b-fail', 'lock-timeout': 'b-fail', 'no-gh-auth': 'b-fail',
         'wrong-account': 'b-fail'}

rows = []
for r in runs:
    tot = r['ctok'] or (r['tin'] + r['tout'] + r['tcr'] + r['tcw'])
    rows.append(
        '<tr>'
        '<td data-sort="%d">%s</td>'
        '<td>%s</td>'
        '<td data-sort="%s">%s</td>'
        '<td data-sort="%s">%s</td>'
        '<td data-sort="%s">%s</td>'
        '<td data-sort="%d">%s</td>'
        '<td data-sort="%s">%s</td>'
        '<td data-sort="%s">%s</td>'
        '<td data-sort="%s">%s</td>'
        '</tr>' % (
            int(r['start'].timestamp()), r['start'].strftime('%a %d %b %H:%M'),
            html.escape(r['mode']),
            r['elapsed'] if r['elapsed'] is not None else (
                int((now - r['start']).total_seconds() // 60) if r['status'] == 'running' else -1),
            ('%dm' % r['elapsed']) if r['elapsed'] is not None else (
                ('%dm so far' % int((now - r['start']).total_seconds() // 60))
                if r['status'] == 'running' else '&mdash;'),
            r['waited'] if r['waited'] is not None else -1,
            ('%dm' % r['waited']) if r['waited'] else '&mdash;',
            {'ok': 0, 'running': 1, 'skipped': 2, 'quota': 3}.get(r['status'], 4), 
            '<span class="badge %s">%s</span>' % (BADGE.get(r['status'], 'b-fail'), r['status']),
            r['turns'] or -1, r['turns'] or '&mdash;',
            tot or -1, fmt_tok(tot),
            ('%.1f' % r['cl_delta']) if r['cl_delta'] is not None else -1,
            ('+%d%%' % r['cl_delta']) if r['cl_delta'] else '&mdash;',
            ('%.1f' % r['q_delta']) if r['q_delta'] is not None else -1,
            ('+%.1f%%' % r['q_delta']) if r['q_delta'] is not None else '&mdash;',
        ))

srows = []
for mode in sorted(by_mode, key=lambda m: -by_mode[m]['n']):
    m = by_mode[mode]
    mm = med(m['mins'])
    srows.append('<tr><td>%s</td><td>%d</td><td>%d</td><td>%d</td><td>%d</td>'
                 '<td>%s</td><td>%s</td><td>%s</td></tr>' % (
        html.escape(mode), m['n'], m['ok'], m['skip'], m['fail'],
        ('%dm' % mm) if mm is not None else '&mdash;',
        ('%dm' % max(m['mins'])) if m['mins'] else '&mdash;',
        fmt_tok(m['tok'])))

qline = ('<strong>%.1f%%</strong> of the weekly window used' % cur_quota) if cur_quota is not None \
        else 'no sample available'

doc = """<!doctype html>
<meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>reviewprs runs &mdash; blu6</title>
<style>
:root{--bg:#f7f8fa;--panel:#fff;--ink:#16191d;--muted:#5b6570;--line:#dfe3e8;
--ok:#0f7b3d;--warn:#a15c00;--bad:#b3261e;--run:#1d4ed8;--code:#f0f2f5}
@media (prefers-color-scheme:dark){:root{--bg:#14171a;--panel:#1c2024;--ink:#e6e9ec;
--muted:#9aa4ad;--line:#2c3238;--ok:#4ec97f;--warn:#e0a53d;--bad:#f2837a;--run:#7aa2f7;--code:#23282d}}
*{box-sizing:border-box}body{margin:0;padding:24px;background:var(--bg);color:var(--ink);
font:14px/1.5 system-ui,-apple-system,Segoe UI,Roboto,sans-serif}
.wrap{max-width:1180px;margin:0 auto}
h1{font-size:20px;margin:0 0 4px}h2{font-size:15px;margin:28px 0 8px;color:var(--muted);
text-transform:uppercase;letter-spacing:.04em}
.sub{color:var(--muted);margin:0 0 18px}
.cards{display:flex;flex-wrap:wrap;gap:12px;margin:16px 0}
.card{background:var(--panel);border:1px solid var(--line);border-radius:8px;padding:12px 16px;min-width:150px}
.card .k{color:var(--muted);font-size:12px}.card .v{font-size:20px;font-weight:600;margin-top:2px}
table{width:100%;border-collapse:collapse;background:var(--panel);border:1px solid var(--line);
border-radius:8px;overflow:hidden}
th,td{text-align:left;padding:7px 10px;border-bottom:1px solid var(--line);white-space:nowrap}
th{background:var(--code);font-weight:600;cursor:pointer;user-select:none}
th::after{content:"\\2195";opacity:.35;margin-left:5px;font-size:11px}
th[aria-sort=ascending]::after{content:"\\25B2";opacity:1}
th[aria-sort=descending]::after{content:"\\25BC";opacity:1}
tr:last-child td{border-bottom:0}
.badge{padding:1px 7px;border-radius:10px;font-size:12px;font-weight:600}
.b-ok{background:rgba(15,123,61,.14);color:var(--ok)}
.b-skip{background:rgba(161,92,0,.14);color:var(--warn)}
.b-run{background:rgba(29,78,216,.14);color:var(--run)}
.b-fail{background:rgba(179,38,30,.14);color:var(--bad)}
.b-quota{background:rgba(161,92,0,.22);color:var(--warn);font-weight:700}
.muted{color:var(--muted);font-weight:400}
.note{background:var(--panel);border:1px solid var(--line);border-left:3px solid var(--warn);
border-radius:6px;padding:10px 14px;margin:14px 0;color:var(--muted)}
.scroll{overflow-x:auto}code{background:var(--code);padding:1px 5px;border-radius:4px}
</style>
<div class="wrap">
<h1>reviewprs runs &mdash; blu6</h1>
<p class="sub">Last __DAYS__ days &middot; generated __GEN__ &middot; regenerated after every run</p>

<div class="cards">
  <div class="card"><div class="k">Runs (__DAYS__d)</div><div class="v">__NRUNS__</div></div>
  <div class="card"><div class="k">Completed</div><div class="v">__NOK__</div></div>
  <div class="card"><div class="k">Skipped (lock)</div><div class="v">__NSKIP__</div></div>
  <div class="card"><div class="k">Failed</div><div class="v">__NFAIL__</div></div>
  <div class="card"><div class="k">Blocked on quota</div><div class="v">__NQUOTA__</div></div>
  <div class="card"><div class="k">Claude week__CLRESET__</div><div class="v">__CLPCT__</div></div>
  <div class="card"><div class="k">Claude session</div><div class="v">__CLSESS__</div></div>
  <div class="card"><div class="k">Claude burn rate</div><div class="v">__CLBURN__</div></div>
  <div class="card"><div class="k">Claude tokens (7d)</div><div class="v">__CLWEEK__</div></div>
  <div class="card"><div class="k">Codex weekly quota__PLAN__</div><div class="v">__QUOTA__</div></div>
  <div class="card"><div class="k">Codex burn rate</div><div class="v">__BURN__</div></div>
  <div class="card"><div class="k">At reset (__RESET__)</div><div class="v">__PROJ__</div></div>
</div>

<h2>Label coverage</h2>
<p class="sub">Only <code>followup</code>, <code>all</code> and <code>rsync</code> appear as run rows.
The three labels are swept <em>inside</em> each <code>all</code> run, which writes one combined log,
so their state is read from the published reports instead.</p>
<div class="scroll"><table class="sortable">
<thead><tr><th>Label</th><th>Report generated</th><th>PRs</th><th>Swept by</th>
<th>APPROVE / COMMENT / REQUEST</th></tr></thead>
<tbody>__LROWS__</tbody></table></div>

<h2>By run type</h2>
<div class="scroll"><table class="sortable">
<thead><tr><th>Mode</th><th>Runs</th><th>OK</th><th>Skipped</th><th>Failed</th>
<th>Median</th><th>Longest</th><th>Tokens</th></tr></thead>
<tbody>__SROWS__</tbody></table></div>

<h2>Runs</h2>
<div class="scroll"><table class="sortable">
<thead><tr><th>Started</th><th>Mode</th><th>Duration</th><th>Lock wait</th><th>Status</th>
<th>Turns</th><th>Claude tokens</th><th>Claude wk &Delta;</th><th>Codex wk &Delta;</th></tr></thead>
<tbody>__ROWS__</tbody></table></div>

<div class="note">
<strong>About the quota columns.</strong> The Codex figure is real: the CLI records its own
rate-limit meter (<code>used_percent</code> for the 10080-minute weekly window) in every session
rollout, so <em>Codex weekly &Delta;</em> is how much of the weekly allowance that run consumed.
<strong>Both figures are now real measurements.</strong> Codex records its own rate-limit meter
(<code>used_percent</code>, 10080-minute weekly window) in every session rollout. Claude's
<code>/usage</code> works headlessly, so <code>claude-usage-probe.sh</code> reads the true session
and weekly percentages before and after every run, and hourly in between &mdash; the
<em>&Delta;</em> columns are the difference the run itself made, not an estimate.
<br><br>
<em>Claude tokens</em> is taken from the session transcripts <strong>including every subagent</strong>.
That matters: the <code>result</code> line in the log counts only the main session, which understates
a review run by roughly an order of magnitude, since most of the work happens in subagents. A run's
<code>api-equiv $</code> figure in the logs is list-price arithmetic &mdash; not a charge, and not
plan usage.</div>
</div>
<script>
document.querySelectorAll('table.sortable').forEach(function(t){
  var tb=t.tBodies[0], coll=new Intl.Collator(undefined,{numeric:true});
  Array.prototype.forEach.call(t.tHead.rows[0].cells,function(th,i){
    th.tabIndex=0; th.setAttribute('role','button');
    function key(tr){var c=tr.cells[i];return c.dataset.sort!==undefined?c.dataset.sort:c.textContent.trim();}
    function go(){
      var asc=th.getAttribute('aria-sort')!=='ascending';
      Array.prototype.forEach.call(t.tHead.rows[0].cells,function(o){o.removeAttribute('aria-sort');});
      th.setAttribute('aria-sort',asc?'ascending':'descending');
      var rows=Array.prototype.slice.call(tb.rows);
      rows.forEach(function(r,n){r._i=n;});
      rows.sort(function(a,b){
        var x=key(a),y=key(b),nx=parseFloat(x),ny=parseFloat(y),d;
        if(!isNaN(nx)&&!isNaN(ny)&&x.trim()!==''&&y.trim()!=='') d=nx-ny; else d=coll.compare(x,y);
        return (d||a._i-b._i)*(asc?1:-1);
      });
      rows.forEach(function(r){tb.appendChild(r);});
    }
    th.addEventListener('click',go);
    th.addEventListener('keydown',function(e){if(e.key==='Enter'||e.key===' '){e.preventDefault();go();}});
  });
});
</script>
"""

nok = sum(1 for r in runs if r['status'] == 'ok')
nskip = sum(1 for r in runs if r['status'] == 'skipped')
nfail = sum(1 for r in runs if r['status'] in ('failed', 'lock-timeout', 'no-gh-auth',
                                               'wrong-account'))
nquota = sum(1 for r in runs if r['status'] == 'quota')

doc = (doc.replace('__DAYS__', str(DAYS))
          .replace('__GEN__', now.strftime('%a %d %b %Y %H:%M %Z'))
          .replace('__NRUNS__', str(len(runs)))
          .replace('__NOK__', str(nok))
          .replace('__NSKIP__', str(nskip))
          .replace('__NFAIL__', str(nfail))
          .replace('__NQUOTA__', str(nquota))
          .replace('__PLAN__', (' (%s)' % html.escape(plan)) if plan else '')
          .replace('__CLPCT__', ('%d%%' % cl_pct) if cl_pct is not None else '&mdash;')
          .replace('__CLSESS__', ('%d%%' % cl_session) if cl_session is not None else '&mdash;')
          .replace('__CLBURN__', ('%.1f%%/day' % cl_burn) if cl_burn is not None else '&mdash;')
          .replace('__CLRESET__', (' (resets %s)' % html.escape(cl_resets)) if cl_resets else '')
          .replace('__CLWEEK__', fmt_tok(cl_week))
          .replace('__QUOTA__', ('%.0f%%' % cur_quota) if cur_quota is not None else '&mdash;')
          .replace('__BURN__', ('%.1f%%/day' % burn) if burn is not None else '&mdash;')
          .replace('__RESET__', reset_at.strftime('%a %d %b') if reset_at else '?')
          .replace('__PROJ__', ('~%.0f%%' % projected) if projected is not None else '&mdash;')
          .replace('__LROWS__', '\n'.join(lrows))
          .replace('__SROWS__', '\n'.join(srows))
          .replace('__ROWS__', '\n'.join(rows)))

os.makedirs(os.path.dirname(OUT), exist_ok=True)
open(OUT, 'w').write(doc)
print("wrote %s (%d bytes, %d runs, codex weekly %s)" % (
    OUT, len(doc), len(runs), ('%.1f%%' % cur_quota) if cur_quota is not None else 'n/a'))
