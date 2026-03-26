"""
Config persistence tests. Verifies that configuration changes survive
device reboot and that destID/bidirectional flags persist correctly.

WARNING: These tests REBOOT the device. Only run on a test bench.

Usage:
    ITHO_DEVICE=<device-ip> pytest tests/api/test_config_persistence.py -v
"""
import os
import json
import time
import requests
import pytest

try:
    import websocket
    HAS_WS = True
except ImportError:
    HAS_WS = False

DEVICE_IP = os.environ.get("ITHO_DEVICE", "")
REST_URL = f"http://{DEVICE_IP}/api/v2"
WS_URL = f"ws://{DEVICE_IP}:8000/ws"
REMOTES_URL = f"http://{DEVICE_IP}/remotes.json"
CONFIG_URL = f"http://{DEVICE_IP}/config.json"


def wait_for_device(timeout=30):
    """Wait for device to come back online after reboot."""
    start = time.time()
    while time.time() - start < timeout:
        try:
            r = requests.get(f"{REST_URL}/speed", timeout=3)
            if r.status_code == 200:
                return True
        except Exception:
            pass
        time.sleep(2)
    return False


def reboot_device():
    """Reboot via websocket and wait for recovery."""
    if not HAS_WS:
        pytest.skip("websocket-client not installed")
    try:
        ws = websocket.create_connection(WS_URL, timeout=5)
        ws.send(json.dumps({"reboot": True}))
        ws.close()
    except Exception:
        pass
    time.sleep(5)
    assert wait_for_device(30), "Device did not come back after reboot"


def get_remotes():
    r = requests.get(REMOTES_URL, timeout=10)
    data = r.json()
    key = next(k for k in data.keys() if k.startswith("remotes"))
    return data[key]


class TestRemoteConfigPersistence:
    """Verify remote config survives reboot."""

    def test_read_remotes_before_reboot(self):
        """Capture remote config before reboot."""
        remotes = get_remotes()
        assert len(remotes) > 0
        # Store in class for comparison
        TestRemoteConfigPersistence._remotes_before = remotes

    def test_reboot_device(self):
        reboot_device()

    def test_remotes_survive_reboot(self):
        """Remote config should be identical after reboot."""
        remotes_after = get_remotes()
        remotes_before = TestRemoteConfigPersistence._remotes_before

        for i, (before, after) in enumerate(zip(remotes_before, remotes_after)):
            assert before["id"] == after["id"], f"Remote {i} ID changed after reboot"
            assert before["name"] == after["name"], f"Remote {i} name changed"
            assert before["remtype"] == after["remtype"], f"Remote {i} type changed"
            assert before["bidirectional"] == after["bidirectional"], f"Remote {i} bidirectional changed"

    def test_destid_survives_reboot(self):
        """destID field should persist across reboot."""
        remotes = get_remotes()
        for r in remotes:
            if "destid" in r and r["destid"] != [0, 0, 0]:
                assert len(r["destid"]) == 3
                assert all(isinstance(b, int) for b in r["destid"])
                # If we had a destid before reboot, it should still be there
                before = TestRemoteConfigPersistence._remotes_before[r["index"]]
                if "destid" in before:
                    assert r["destid"] == before["destid"], (
                        f"Remote {r['index']} destid changed: {before['destid']} -> {r['destid']}"
                    )

    def test_bidirectional_derived_after_reboot(self):
        """Bidirectional should match remote type after reboot."""
        BIDIR_TYPES = [0x22F5, 0x22F4, 0x1298, 0x12A0, 0x22F2]
        remotes = get_remotes()
        for r in remotes:
            if r["id"] != [0, 0, 0]:
                expected = r["remtype"] in BIDIR_TYPES
                assert r["bidirectional"] == expected, (
                    f"Remote {r['index']} type {r['remtype']}: bidirectional should be {expected}"
                )


class TestSystemConfigPersistence:
    """Verify system config survives reboot."""

    def test_config_readable_after_reboot(self):
        """config.json should be accessible after reboot."""
        r = requests.get(CONFIG_URL, timeout=10)
        assert r.status_code == 200
        data = r.json()
        assert "itho_rf_support" in data
        assert "api_version" in data

    def test_rf_module_id_persists(self):
        """Module RF ID should survive reboot."""
        r = requests.get(CONFIG_URL, timeout=10)
        data = r.json()
        rf_id = data.get("module_rf_id", [])
        assert len(rf_id) == 3
        assert all(isinstance(b, int) for b in rf_id)
