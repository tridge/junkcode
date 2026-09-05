import importlib.util
import os
import unittest
from importlib.machinery import SourceFileLoader

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


if __name__ == "__main__":
    unittest.main()
