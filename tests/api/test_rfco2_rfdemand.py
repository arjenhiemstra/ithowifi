"""
Tests for rfco2 and rfdemand API parameters (commits 6eaa9c1, b0a9a24).
Tests boundary values and error handling. The device may or may not have
an RFTCO2 remote configured — tests verify no crashes regardless.

Usage:
    pytest tests/api/test_rfco2_rfdemand.py -v
"""
import os
import requests

DEVICE_IP = os.environ.get("ITHO_DEVICE", "")
REST_URL = f"http://{DEVICE_IP}/api/v2"


class TestRFCO2:
    """Test rfco2 endpoint — sends CO2 ppm via RF."""

    def test_rfco2_default_index(self):
        """rfco2 without index should default to index 0."""
        r = requests.post(f"{REST_URL}/rfremote/co2", json={"co2": 800}, timeout=10)
        assert r.status_code < 500

    def test_rfco2_with_index(self):
        r = requests.post(f"{REST_URL}/rfremote/co2", json={"co2": 800, "index": 0}, timeout=10)
        assert r.status_code < 500

    def test_rfco2_zero(self):
        r = requests.post(f"{REST_URL}/rfremote/co2", json={"co2": 0}, timeout=10)
        assert r.status_code < 500

    def test_rfco2_max(self):
        r = requests.post(f"{REST_URL}/rfremote/co2", json={"co2": 10000}, timeout=10)
        assert r.status_code < 500

    def test_rfco2_over_max(self):
        """Above max should not crash."""
        r = requests.post(f"{REST_URL}/rfremote/co2", json={"co2": 65535}, timeout=10)
        assert r.status_code < 500

    def test_rfco2_negative(self):
        r = requests.post(f"{REST_URL}/rfremote/co2", json={"co2": -1}, timeout=10)
        assert r.status_code < 500

    def test_rfco2_non_numeric(self):
        r = requests.post(f"{REST_URL}/rfremote/co2", json={"co2": "abc"}, timeout=10)
        assert r.status_code < 500

    def test_rfco2_invalid_index(self):
        """Out-of-range index should fail gracefully."""
        r = requests.post(f"{REST_URL}/rfremote/co2", json={"co2": 800, "index": 99}, timeout=10)
        assert r.status_code < 500

    def test_rfco2_wrong_remote_type(self):
        """rfco2 on non-RFTCO2 remote should return fail, not crash."""
        # Index 0 might not be RFTCO2 — that's fine, we test error handling
        r = requests.post(f"{REST_URL}/rfremote/co2", json={"co2": 500, "index": 0}, timeout=10)
        assert r.status_code < 500
        data = r.json()
        assert data.get("status") in ("success", "fail")


class TestRFDemand:
    """Test rfdemand endpoint — sends ventilation demand via RF."""

    def test_rfdemand_default_index(self):
        r = requests.post(f"{REST_URL}/rfremote/demand", json={"demand": 100}, timeout=10)
        assert r.status_code < 500

    def test_rfdemand_with_index(self):
        r = requests.post(f"{REST_URL}/rfremote/demand", json={"demand": 100, "index": 0}, timeout=10)
        assert r.status_code < 500

    def test_rfdemand_zero(self):
        r = requests.post(f"{REST_URL}/rfremote/demand", json={"demand": 0}, timeout=10)
        assert r.status_code < 500

    def test_rfdemand_max(self):
        r = requests.post(f"{REST_URL}/rfremote/demand", json={"demand": 200}, timeout=10)
        assert r.status_code < 500

    def test_rfdemand_over_max(self):
        """Above 200 should be clamped or fail, not crash."""
        r = requests.post(f"{REST_URL}/rfremote/demand", json={"demand": 255}, timeout=10)
        assert r.status_code < 500

    def test_rfdemand_with_zone(self):
        r = requests.post(f"{REST_URL}/rfremote/demand", json={"demand": 100, "zone": 1}, timeout=10)
        assert r.status_code < 500

    def test_rfdemand_zone_zero(self):
        r = requests.post(f"{REST_URL}/rfremote/demand", json={"demand": 100, "zone": 0}, timeout=10)
        assert r.status_code < 500

    def test_rfdemand_zone_max(self):
        r = requests.post(f"{REST_URL}/rfremote/demand", json={"demand": 100, "zone": 255}, timeout=10)
        assert r.status_code < 500

    def test_rfdemand_negative(self):
        r = requests.post(f"{REST_URL}/rfremote/demand", json={"demand": -1}, timeout=10)
        assert r.status_code < 500

    def test_rfdemand_non_numeric(self):
        r = requests.post(f"{REST_URL}/rfremote/demand", json={"demand": "xyz"}, timeout=10)
        assert r.status_code < 500

    def test_rfdemand_invalid_index(self):
        r = requests.post(f"{REST_URL}/rfremote/demand", json={"demand": 100, "index": 99}, timeout=10)
        assert r.status_code < 500


class TestRFRemoteCommands:
    """Test rfremote endpoint with various indices."""

    def test_rfremote_index_zero(self):
        r = requests.post(f"{REST_URL}/rfremote/command", json={"command": "low", "index": 0}, timeout=10)
        assert r.status_code < 500

    def test_rfremote_index_max(self):
        r = requests.post(f"{REST_URL}/rfremote/command", json={"command": "low", "index": 11}, timeout=10)
        assert r.status_code < 500

    def test_rfremote_index_out_of_range(self):
        r = requests.post(f"{REST_URL}/rfremote/command", json={"command": "low", "index": 99}, timeout=10)
        assert r.status_code < 500

    def test_rfremote_invalid_command(self):
        r = requests.post(f"{REST_URL}/rfremote/command", json={"command": "nonexistent"}, timeout=10)
        assert r.status_code < 500

    def test_rfremote_join(self):
        """Join command should not crash (may fail if no device to bind to)."""
        r = requests.post(f"{REST_URL}/rfremote/command", json={"command": "join", "index": 0}, timeout=10)
        assert r.status_code < 500

    def test_rfremote_all_commands(self):
        """Every RF command should not cause a server error."""
        commands = ["away", "low", "medium", "high", "timer1", "timer2", "timer3",
                    "auto", "autonight", "cook30", "cook60", "motion_on", "motion_off", "leave"]
        for cmd in commands:
            r = requests.post(f"{REST_URL}/rfremote/command", json={"command": cmd, "index": 0}, timeout=10)
            assert r.status_code < 500, f"rfremote command={cmd} returned {r.status_code}"
