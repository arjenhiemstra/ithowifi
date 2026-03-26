"""
Fan command API tests. Changes fan speed/timer but is reversible.
Only run on a test bench where fan noise is acceptable.

Usage:
    ITHO_DEVICE=<device-ip> pytest tests/api/test_fan_commands.py -v
"""
import os
import requests
import pytest
import time

DEVICE_IP = os.environ.get("ITHO_DEVICE", "")
DEVICE_URL = f"http://{DEVICE_IP}"
REST_URL = f"{DEVICE_URL}/api/v2"
LEGACY_URL = f"{DEVICE_URL}/api.html"


class TestFanCommandsREST:
    """Test fan commands via REST API v2."""

    def test_command_low(self):
        r = requests.post(f"{REST_URL}/command", json={"command": "low"}, timeout=10)
        assert r.status_code == 200
        assert r.json().get("status") == "success"

    def test_command_medium(self):
        r = requests.post(f"{REST_URL}/command", json={"command": "medium"}, timeout=10)
        assert r.status_code == 200
        assert r.json().get("status") == "success"

    def test_command_high(self):
        r = requests.post(f"{REST_URL}/command", json={"command": "high"}, timeout=10)
        assert r.status_code == 200
        assert r.json().get("status") == "success"

    def test_command_timer1(self):
        r = requests.post(f"{REST_URL}/command", json={"command": "timer1"}, timeout=10)
        assert r.status_code == 200
        assert r.json().get("status") == "success"

    def test_command_invalid(self):
        """Invalid command should fail gracefully, not crash."""
        r = requests.post(f"{REST_URL}/command", json={"command": "nonexistent"}, timeout=10)
        assert r.status_code == 400
        assert r.json().get("status") == "fail"

    def test_speed_direct(self):
        r = requests.post(f"{REST_URL}/command", json={"speed": 150}, timeout=10)
        assert r.status_code == 200
        assert r.json().get("status") == "success"

    def test_speed_zero(self):
        r = requests.post(f"{REST_URL}/command", json={"speed": 0}, timeout=10)
        assert r.status_code == 200

    def test_speed_max(self):
        r = requests.post(f"{REST_URL}/command", json={"speed": 255}, timeout=10)
        assert r.status_code == 200

    def test_speed_out_of_range(self):
        r = requests.post(f"{REST_URL}/command", json={"speed": 300}, timeout=10)
        assert r.status_code == 400
        assert "range" in r.json()["data"]["failreason"]

    def test_timer_out_of_range(self):
        r = requests.post(f"{REST_URL}/command", json={"timer": 70000}, timeout=10)
        assert r.status_code == 400
        assert "range" in r.json()["data"]["failreason"]

    def test_speed_with_timer(self):
        r = requests.post(f"{REST_URL}/command", json={"speed": 200, "timer": 1}, timeout=10)
        assert r.status_code == 200
        assert r.json().get("status") == "success"

    def test_speed_then_read(self):
        """Set speed then verify current speed is readable."""
        requests.post(f"{REST_URL}/command", json={"speed": 120}, timeout=10)
        time.sleep(1)
        r = requests.get(f"{REST_URL}/speed", timeout=10)
        assert r.json().get("status") == "success"
        assert "currentspeed" in r.json().get("data", {})

    def test_missing_body(self):
        """POST with no body should fail."""
        r = requests.post(f"{REST_URL}/command", json={}, timeout=10)
        assert r.status_code == 400

    def test_clearqueue(self):
        r = requests.post(f"{REST_URL}/command", json={"command": "clearqueue"}, timeout=10)
        assert r.status_code == 200

    def test_restore_low(self):
        """Restore fan to low after all tests."""
        r = requests.post(f"{REST_URL}/command", json={"command": "low"}, timeout=10)
        assert r.json().get("status") == "success"


class TestFanCommandsLegacy:
    """Test fan commands via legacy v1 /api.html."""

    def test_command_low_v1(self):
        r = requests.get(LEGACY_URL, params={"command": "low"}, timeout=10)
        assert r.status_code == 200
        assert r.text.strip() in ("OK", "NOK")

    def test_speed_v1(self):
        r = requests.get(LEGACY_URL, params={"speed": "150"}, timeout=10)
        assert r.status_code == 200

    def test_restore_low_v1(self):
        r = requests.get(LEGACY_URL, params={"command": "low"}, timeout=10)
        assert r.status_code == 200
