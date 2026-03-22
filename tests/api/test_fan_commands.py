"""
Fan command API tests. Changes fan speed/timer but is reversible.
Only run on a test bench where fan noise is acceptable.

Usage:
    pytest tests/api/test_fan_commands.py -v
    ITHO_DEVICE=<device-ip> pytest tests/api/test_fan_commands.py -v
"""
import os
import requests
import pytest
import time

DEVICE_IP = os.environ.get("ITHO_DEVICE", "")
DEVICE_URL = f"http://{DEVICE_IP}"
API_URL = f"{DEVICE_URL}/api.html"


class TestFanCommands:
    """Test fan speed/timer commands."""

    def test_command_low(self):
        r = requests.get(API_URL, params={"command": "low"}, timeout=10)
        assert r.status_code == 200
        assert r.json().get("status") == "success"

    def test_command_medium(self):
        r = requests.get(API_URL, params={"command": "medium"}, timeout=10)
        assert r.status_code == 200
        assert r.json().get("status") == "success"

    def test_command_high(self):
        r = requests.get(API_URL, params={"command": "high"}, timeout=10)
        assert r.status_code == 200
        assert r.json().get("status") == "success"

    def test_command_timer1(self):
        r = requests.get(API_URL, params={"command": "timer1"}, timeout=10)
        assert r.status_code == 200
        assert r.json().get("status") == "success"

    def test_command_invalid(self):
        """Invalid command should fail gracefully, not crash."""
        r = requests.get(API_URL, params={"command": "nonexistent"}, timeout=10)
        assert r.status_code < 500
        assert r.json().get("status") in ("fail", "error")

    def test_speed_direct(self):
        r = requests.get(API_URL, params={"speed": "150"}, timeout=10)
        assert r.status_code == 200
        assert r.json().get("status") == "success"

    def test_speed_zero(self):
        r = requests.get(API_URL, params={"speed": "0"}, timeout=10)
        assert r.status_code == 200

    def test_speed_max(self):
        r = requests.get(API_URL, params={"speed": "255"}, timeout=10)
        assert r.status_code == 200

    def test_speed_with_timer(self):
        r = requests.get(API_URL, params={"speed": "200", "timer": "1"}, timeout=10)
        assert r.status_code == 200
        assert r.json().get("status") == "success"

    def test_speed_then_read(self):
        """Set speed then verify current speed is readable."""
        requests.get(API_URL, params={"speed": "120"}, timeout=10)
        time.sleep(1)
        r = requests.get(API_URL, params={"get": "currentspeed"}, timeout=10)
        assert r.json().get("status") == "success"

    def test_clearqueue(self):
        r = requests.get(API_URL, params={"command": "clearqueue"}, timeout=10)
        assert r.status_code == 200

    def test_restore_low(self):
        """Restore fan to low after all tests."""
        r = requests.get(API_URL, params={"command": "low"}, timeout=10)
        assert r.json().get("status") == "success"
