"""
Read-only API tests. Safe to run against any live device.

Usage:
    pytest tests/api/test_readonly.py -v
    ITHO_DEVICE=<device-ip> pytest tests/api/test_readonly.py -v
"""
import os
import requests
import pytest

DEVICE_IP = os.environ.get("ITHO_DEVICE", "")
DEVICE_URL = f"http://{DEVICE_IP}"
REST_URL = f"{DEVICE_URL}/api/v2"
SPEC_URL = f"{DEVICE_URL}/api/openapi.json"


class TestOpenAPISpec:
    """Verify the OpenAPI spec endpoint."""

    def test_spec_reachable(self):
        r = requests.get(SPEC_URL, timeout=5)
        assert r.status_code == 200

    def test_spec_valid_json(self):
        r = requests.get(SPEC_URL, timeout=5)
        spec = r.json()
        assert spec["openapi"] == "3.0.3"
        assert "paths" in spec
        assert "components" in spec

    def test_spec_has_v2_paths(self):
        r = requests.get(SPEC_URL, timeout=5)
        spec = r.json()
        paths = spec["paths"]
        for expected in ["/api/v2/speed", "/api/v2/command", "/api/v2/ithostatus",
                         "/api/v2/deviceinfo", "/api/v2/remotes", "/api/v2/settings"]:
            assert expected in paths, f"Missing path: {expected}"

    def test_spec_cors_header(self):
        r = requests.get(SPEC_URL, timeout=5)
        assert r.headers.get("Access-Control-Allow-Origin") == "*"


class TestGetEndpoints:
    """Test read-only GET endpoints."""

    def test_get_status(self):
        r = requests.get(f"{REST_URL}/ithostatus", timeout=10)
        assert r.status_code == 200
        assert r.json()["status"] == "success"

    def test_get_device(self):
        r = requests.get(f"{REST_URL}/deviceinfo", timeout=10)
        assert r.status_code == 200
        data = r.json()
        assert data["status"] == "success"
        assert "deviceinfo" in data.get("data", {})

    def test_get_remotes(self):
        r = requests.get(f"{REST_URL}/remotes", timeout=10)
        assert r.status_code == 200

    def test_get_vremotes(self):
        r = requests.get(f"{REST_URL}/vremotes", timeout=10)
        assert r.status_code == 200

    def test_get_speed(self):
        r = requests.get(f"{REST_URL}/speed", timeout=10)
        assert r.status_code == 200
        assert r.json()["status"] == "success"

    def test_get_rfstatus(self):
        r = requests.get(f"{REST_URL}/rfstatus", timeout=10)
        assert r.status_code == 200

    def test_get_rfstatus_with_name(self):
        """Unknown name should return 404 fail, not 5xx."""
        r = requests.get(f"{REST_URL}/rfstatus", params={"name": "nonexistent"}, timeout=10)
        assert r.status_code in (200, 404)
        assert r.status_code < 500

    def test_get_unknown_endpoint(self):
        """Unknown endpoint should return 404, not 5xx."""
        r = requests.get(f"{REST_URL}/nonexistent", timeout=10)
        assert r.status_code < 500, f"Server error on unknown endpoint: {r.text}"

    def test_getsetting_zero(self):
        r = requests.get(f"{REST_URL}/settings", params={"index": "0"}, timeout=15)
        assert r.status_code < 500

    def test_getsetting_max(self):
        r = requests.get(f"{REST_URL}/settings", params={"index": "255"}, timeout=15)
        assert r.status_code < 500

    def test_getsetting_out_of_range(self):
        """Out-of-range should return client error, not server error."""
        r = requests.get(f"{REST_URL}/settings", params={"index": "999"}, timeout=10)
        assert r.status_code < 500


class TestEdgeCases:
    """Test boundary conditions and malformed input."""

    def test_long_string_endpoint(self):
        """Very long path should not crash the ESP32."""
        r = requests.get(f"{REST_URL}/{'A' * 200}", timeout=10)
        assert r.status_code < 500

    def test_special_characters(self):
        """Special chars should not crash."""
        r = requests.get(f"{REST_URL}/ithostatus", params={"name": "<script>alert(1)</script>"}, timeout=10)
        assert r.status_code < 500

    def test_unicode_param(self):
        r = requests.get(f"{REST_URL}/rfstatus", params={"name": "\u00e9\u00e8\u00ea"}, timeout=10)
        assert r.status_code < 500

    def test_response_is_json(self):
        """Successful responses should be valid JSON."""
        for endpoint in ["ithostatus", "deviceinfo", "speed"]:
            r = requests.get(f"{REST_URL}/{endpoint}", timeout=10)
            r.json()  # raises if not valid JSON

    def test_response_has_status(self):
        """Successful responses should have a status field."""
        r = requests.get(f"{REST_URL}/deviceinfo", timeout=10)
        data = r.json()
        assert "status" in data

    def test_no_server_errors(self):
        """Rapid-fire requests should not crash the device."""
        for _ in range(10):
            r = requests.get(f"{REST_URL}/speed", timeout=10)
            assert r.status_code < 500
