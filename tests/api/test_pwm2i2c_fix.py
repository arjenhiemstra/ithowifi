"""
Tests for the pwm2i2c device type check fix (commit 28c3a74).
The bug was || (OR) instead of && (AND), rejecting ALL device types.

Test device: CVE Silent (type 0x1B) — a supported pwm2i2c device.

Usage:
    pytest tests/api/test_pwm2i2c_fix.py -v
"""
import os
import requests

DEVICE_IP = os.environ.get("ITHO_DEVICE", "")
REST_URL = f"http://{DEVICE_IP}/api/v2"


class TestPwm2i2cDeviceTypeCheck:
    """Verify supported device types can use pwm2i2c commands."""

    def test_command_low_accepted(self):
        """command=low should succeed on CVE Silent (0x1B).
        Before the fix, this returned 'device does not support pwm2i2c'."""
        r = requests.post(f"{REST_URL}/command", json={"command": "low"}, timeout=10)
        data = r.json()
        assert data.get("status") == "success", (
            f"Expected success but got {data}. "
            "If failreason contains 'does not support pwm2i2c', the || vs && bug has regressed."
        )

    def test_command_medium_accepted(self):
        r = requests.post(f"{REST_URL}/command", json={"command": "medium"}, timeout=10)
        data = r.json()
        assert data.get("status") == "success"
        failreason = data.get("data", {}).get("failreason", "")
        assert "does not support pwm2i2c" not in failreason

    def test_command_high_accepted(self):
        r = requests.post(f"{REST_URL}/command", json={"command": "high"}, timeout=10)
        assert r.json().get("status") == "success"

    def test_speed_direct_accepted(self):
        """Direct speed also goes through the pwm2i2c path."""
        r = requests.post(f"{REST_URL}/command", json={"speed": 100}, timeout=10)
        assert r.json().get("status") == "success"

    def test_speed_with_timer_accepted(self):
        r = requests.post(f"{REST_URL}/command", json={"speed": 100, "timer": 1}, timeout=10)
        assert r.json().get("status") == "success"

    def test_all_timer_commands_accepted(self):
        for cmd in ["timer1", "timer2", "timer3"]:
            r = requests.post(f"{REST_URL}/command", json={"command": cmd}, timeout=10)
            assert r.json().get("status") == "success", f"command={cmd} failed"

    def test_restore_low(self):
        requests.post(f"{REST_URL}/command", json={"command": "low"}, timeout=10)
