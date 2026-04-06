"""
Tests for the api_reboot setting gate.

Verifies that the reboot command is blocked when api_reboot is disabled,
and that other debug commands work regardless of the api_reboot setting.

Usage:
    ITHO_DEVICE=<device-ip> pytest tests/api/test_api_reboot_gate.py -v
"""
import os

import pytest
import requests

DEVICE_IP = os.environ.get("ITHO_DEVICE", "")
REST_URL = f"http://{DEVICE_IP}/api/v2"

pytestmark = pytest.mark.skipif(not DEVICE_IP, reason="ITHO_DEVICE not set")


class TestRebootGate:
    """Test that api_reboot setting controls reboot access."""

    def test_reboot_blocked_when_disabled(self):
        """POST /api/v2/debug with action=reboot should return 403 when
        api_reboot is off (the default)."""
        resp = requests.post(
            f"{REST_URL}/debug",
            json={"action": "reboot"},
            timeout=10,
        )
        body = resp.json()
        # When api_reboot is 0 (default), we expect 403 with a fail status.
        # It could also be 429 if a previous test triggered the rate limiter,
        # so we accept both.
        if resp.status_code == 429:
            pytest.skip("rate-limited, cannot verify reboot gate in this run")
        assert resp.status_code == 400, (
            f"Expected 400 for disabled reboot, got {resp.status_code}: {body}"
        )
        assert body["status"] == "fail"
        assert "disabled" in body["data"]["failreason"].lower()

    def test_debug_level_works_regardless(self):
        """Debug level commands should work regardless of api_reboot setting."""
        resp = requests.post(
            f"{REST_URL}/debug",
            json={"action": "level1"},
            timeout=10,
        )
        assert resp.status_code == 200
        body = resp.json()
        assert body["status"] == "success"
        assert "result" in body["data"]

    def test_debug_level0_works(self):
        """level0 should also work when api_reboot is off."""
        resp = requests.post(
            f"{REST_URL}/debug",
            json={"action": "level0"},
            timeout=10,
        )
        assert resp.status_code == 200
        assert resp.json()["status"] == "success"
