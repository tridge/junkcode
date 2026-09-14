import importlib.util
import json
import os
import tempfile
import time
import unittest
import urllib.error
from importlib.machinery import SourceFileLoader

import claude_quota
import codex_quota


HERE = os.path.dirname(os.path.abspath(__file__))


def load_script(name):
    path = os.path.join(HERE, name)
    module_name = name.replace("-", "_")
    spec = importlib.util.spec_from_loader(
        module_name, SourceFileLoader(module_name, path))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class CodexQuotaTests(unittest.TestCase):
    def test_parse_all_current_account_limits_and_credits(self):
        result = {
            "accountId": "current-account",
            "rateLimits": {
                "limitId": "codex", "planType": "pro",
                "primary": {"usedPercent": 17, "windowDurationMins": 10080,
                            "resetsAt": 200},
                "credits": {"hasCredits": False, "balance": "0"},
            },
            "rateLimitsByLimitId": {
                "codex": {
                    "limitId": "codex", "planType": "pro",
                    "primary": {"usedPercent": 17,
                                "windowDurationMins": 10080,
                                "resetsAt": 200},
                },
                "codex_bengalfox": {
                    "limitId": "codex_bengalfox",
                    "limitName": "GPT-5.3-Codex-Spark",
                    "primary": {"usedPercent": 4, "windowDurationMins": 300,
                                "resetsAt": 100},
                },
            },
            "rateLimitResetCredits": {"availableCount": 3},
        }
        got = codex_quota._parse(result)
        self.assertTrue(got["ok"])
        self.assertEqual(got["account_id"], "current-account")
        self.assertEqual(got["plan"], "pro")
        self.assertEqual(got["reset_credits"], 3)
        self.assertEqual(
            [(w["label"], w["pct"]) for w in got["windows"]],
            [("weekly", 17.0), ("5h:5.3-Spark", 4.0)])

    def test_general_snapshot_works_without_by_id_map(self):
        got = codex_quota._parse({
            "rateLimits": {
                "limitId": "codex", "planType": "plus",
                "primary": {"usedPercent": 25, "windowDurationMins": 300},
            }
        })
        self.assertEqual(got["windows"][0]["label"], "5h")
        self.assertEqual(got["plan"], "plus")


class GenmonTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.genmon = load_script("ai-usage-genmon")

    def test_estimate_is_visually_stale(self):
        data = {"generated_at": 1000}
        self.assertTrue(self.genmon.provider_stale(
            data, {"source": "estimate", "updated_at": 1000}))

    def test_fresh_api_meter_is_not_stale(self):
        now = self.genmon.time.time()
        data = {"generated_at": now}
        self.assertFalse(self.genmon.provider_stale(
            data, {"source": "api", "updated_at": now}))


class RefreshTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.refresh = load_script("ai-usage-refresh")

    def test_transient_failure_keeps_same_account_live_meter(self):
        previous = {
            "generated_at": 100,
            "codex": {"ok": True, "source": "api", "account_id": "a",
                      "windows": [{"pct": 20}]},
        }
        data = {
            "generated_at": 200,
            "codex": {"ok": False, "source": "api", "account_id": "a",
                      "windows": [], "error": "offline"},
            "claude": {"ok": False, "source": "estimate", "windows": []},
        }
        got = self.refresh.merge_previous(data, previous)
        self.assertEqual(got["codex"]["windows"], [{"pct": 20}])
        self.assertEqual(got["codex"]["updated_at"], 100)
        self.assertIn("offline", got["codex"]["error"])

    def test_account_switch_never_keeps_old_account_meter(self):
        previous = {
            "generated_at": 100,
            "codex": {"ok": True, "source": "api", "account_id": "old",
                      "windows": [{"pct": 99}]},
        }
        data = {
            "generated_at": 200,
            "codex": {"ok": False, "source": "api", "account_id": "new",
                      "windows": [], "error": "offline"},
            "claude": {"ok": False, "source": "estimate", "windows": []},
        }
        got = self.refresh.merge_previous(data, previous)
        self.assertEqual(got["codex"]["windows"], [])


class ThrottleTests(unittest.TestCase):
    """429 backoff: one refusal must not turn into a retry storm."""

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.old = os.environ.get("XDG_CACHE_HOME")
        os.environ["XDG_CACHE_HOME"] = self.tmp.name
        self.addCleanup(self._restore_env)

    def _restore_env(self):
        if self.old is None:
            os.environ.pop("XDG_CACHE_HOME", None)
        else:
            os.environ["XDG_CACHE_HOME"] = self.old

    def _http_error(self, code, headers=None):
        return urllib.error.HTTPError(
            claude_quota.URL, code, "err", headers or {}, None)

    def test_delay_doubles_from_two_minutes_and_stops_at_the_ceiling(self):
        got = [claude_quota._backoff_delay(n) for n in (1, 2, 3, 4, 9)]
        self.assertEqual(got, [120, 240, 480, 600, 600])

    def test_server_retry_after_wins_but_is_not_trusted_past_an_hour(self):
        self.assertEqual(claude_quota._backoff_delay(1, 45), 45)
        self.assertEqual(claude_quota._backoff_delay(4, 45), 45)
        self.assertEqual(claude_quota._backoff_delay(1, 99999),
                         claude_quota.RETRY_AFTER_MAX)

    def test_retry_after_read_as_seconds_or_as_an_http_date(self):
        self.assertEqual(
            claude_quota._retry_after(self._http_error(429, {"Retry-After": "30"})),
            30)
        when = time.strftime("%a, %d %b %Y %H:%M:%S GMT",
                             time.gmtime(time.time() + 120))
        secs = claude_quota._retry_after(
            self._http_error(429, {"Retry-After": when}))
        self.assertTrue(110 <= secs <= 121, secs)
        self.assertIsNone(claude_quota._retry_after(self._http_error(429)))

    def test_a_429_arms_the_backoff_and_the_next_call_asks_nothing(self):
        calls = []

        def refuse(req, timeout=None):
            calls.append(req)
            raise self._http_error(429, {"Retry-After": "60"})

        claude_quota._token = lambda: "tok"
        with _patched(claude_quota.urllib.request, "urlopen", refuse):
            first = claude_quota.fetch()
        self.assertTrue(first["throttled"])
        self.assertEqual(first["retry_in"], 60)
        self.assertEqual(len(calls), 1)

        # fetch() swallows exceptions, so count the calls rather than raise:
        # a second request here is exactly the retry storm being fixed
        with _patched(claude_quota.urllib.request, "urlopen", refuse):
            second = claude_quota.fetch()
        self.assertEqual(len(calls), 1, "asked the endpoint while throttled")
        self.assertTrue(second["throttled"])
        self.assertEqual(second["error"], "HTTP 429")
        self.assertGreater(second["retry_in"], 0)

    def test_backoff_expires_and_a_good_reading_clears_it(self):
        claude_quota._write_state(3, time.time() - 1)      # deadline passed
        self.assertEqual(claude_quota.throttled_for(), 0)

        body = {"limits": [{"kind": "session", "percent": 4,
                            "resets_at": "2026-09-13T10:10:00Z"}]}

        class Resp:
            def read(self):
                return json.dumps(body).encode()

            def __enter__(self):
                return self

            def __exit__(self, *a):
                return False

        claude_quota._token = lambda: "tok"
        with _patched(claude_quota.urllib.request, "urlopen",
                      lambda req, timeout=None: Resp()):
            got = claude_quota.fetch()
        self.assertTrue(got["ok"])
        self.assertFalse(os.path.exists(claude_quota._state_path()))

    def test_a_throttled_refresh_is_not_worded_as_a_failure(self):
        refresh = load_script("ai-usage-refresh")
        previous = {"generated_at": 100,
                    "claude": {"ok": True, "source": "api",
                               "windows": [{"pct": 3}]}}
        data = {"generated_at": 800,
                "claude": {"ok": False, "source": "estimate", "windows": [],
                           "throttled": True, "retry_in": 240,
                           "error": "meter throttled (HTTP 429), retrying in 4m"},
                "codex": {"ok": True, "source": "api", "windows": []}}
        got = refresh.merge_previous(data, previous)["claude"]
        self.assertEqual(got["windows"], [{"pct": 3}])   # reading survives
        self.assertTrue(got["throttled"])
        self.assertEqual(got["retry_in"], 240)
        self.assertNotIn("failed", got["error"])
        self.assertIn("throttled", got["error"])


class _patched:
    """Minimal attribute patcher, so the tests need no third-party mock."""

    def __init__(self, obj, name, value):
        self.obj, self.name, self.value = obj, name, value

    def __enter__(self):
        self.old = getattr(self.obj, self.name)
        setattr(self.obj, self.name, self.value)
        return self.value

    def __exit__(self, *a):
        setattr(self.obj, self.name, self.old)
        return False


if __name__ == "__main__":
    unittest.main()
