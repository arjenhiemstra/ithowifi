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
API_URL = f"{DEVICE_URL}/api.html"
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

    def test_spec_has_parameters(self):
        r = requests.get(SPEC_URL, timeout=5)
        spec = r.json()
        params = spec["paths"]["/api.html"]["get"]["parameters"]
        names = [p["name"] for p in params]
        for expected in ["command", "speed", "get", "rfremotecmd", "rfco2", "rfdemand"]:
            assert expected in names, f"Missing parameter: {expected}"

    def test_spec_cors_header(self):
        r = requests.get(SPEC_URL, timeout=5)
        assert r.headers.get("Access-Control-Allow-Origin") == "*"


class TestGetEndpoints:
    """Test read-only GET parameter values."""

    def test_get_ithostatus(self):
        r = requests.get(API_URL, params={"get": "ithostatus"}, timeout=10)
        assert r.status_code == 200
        assert r.json()["status"] == "success"

    def test_get_deviceinfo(self):
        r = requests.get(API_URL, params={"get": "deviceinfo"}, timeout=10)
        assert r.status_code == 200
        data = r.json()
        assert data["status"] == "success"
        assert "deviceinfo" in data.get("data", {})

    def test_get_remotesinfo(self):
        r = requests.get(API_URL, params={"get": "remotesinfo"}, timeout=10)
        assert r.status_code == 200

    def test_get_vremotesinfo(self):
        r = requests.get(API_URL, params={"get": "vremotesinfo"}, timeout=10)
        assert r.status_code == 200

    def test_get_currentspeed(self):
        r = requests.get(API_URL, params={"get": "currentspeed"}, timeout=10)
        assert r.status_code == 200
        assert r.json()["status"] == "success"

    def test_get_rfstatus(self):
        r = requests.get(API_URL, params={"get": "rfstatus"}, timeout=10)
        assert r.status_code == 200

    def test_get_rfstatus_with_name(self):
        r = requests.get(API_URL, params={"get": "rfstatus", "name": "test"}, timeout=10)
        assert r.status_code == 200

    def test_get_unknown_value(self):
        """Unknown get value should return error, not 5xx."""
        r = requests.get(API_URL, params={"get": "nonexistent"}, timeout=10)
        assert r.status_code < 500, f"Server error on unknown get value: {r.text}"

    def test_getsetting_zero(self):
        r = requests.get(API_URL, params={"getsetting": "0"}, timeout=10)
        assert r.status_code < 500

    def test_getsetting_max(self):
        r = requests.get(API_URL, params={"getsetting": "255"}, timeout=10)
        assert r.status_code < 500

    def test_getsetting_out_of_range(self):
        """Out-of-range should return client error, not server error."""
        r = requests.get(API_URL, params={"getsetting": "999"}, timeout=10)
        assert r.status_code < 500


class TestEdgeCases:
    """Test boundary conditions and malformed input."""

    def test_empty_request(self):
        """No parameters should return fail, not server error."""
        r = requests.get(API_URL, timeout=10)
        assert r.status_code < 500, f"Server error on empty request: {r.status_code} {r.text}"
        data = r.json()
        assert data.get("status") == "fail"

    def test_long_string_param(self):
        """Very long string should not crash the ESP32."""
        r = requests.get(API_URL, params={"get": "A" * 200}, timeout=10)
        assert r.status_code < 500

    def test_special_characters(self):
        """Special chars should not crash."""
        r = requests.get(API_URL, params={"get": "<script>alert(1)</script>"}, timeout=10)
        assert r.status_code < 500

    def test_unicode_param(self):
        r = requests.get(API_URL, params={"get": "\u00e9\u00e8\u00ea"}, timeout=10)
        assert r.status_code < 500

    def test_multiple_get_params(self):
        """Multiple params in one request."""
        r = requests.get(API_URL, params={"get": "currentspeed", "getsetting": "0"}, timeout=10)
        assert r.status_code < 500

    def test_response_is_json(self):
        """Successful responses should be valid JSON."""
        for param in ["ithostatus", "deviceinfo", "currentspeed"]:
            r = requests.get(API_URL, params={"get": param}, timeout=10)
            r.json()  # raises if not valid JSON

    def test_response_has_status(self):
        """Successful responses should have a status field."""
        r = requests.get(API_URL, params={"get": "deviceinfo"}, timeout=10)
        data = r.json()
        assert "status" in data

    def test_no_server_errors(self):
        """Rapid-fire requests should not crash the device."""
        for _ in range(10):
            r = requests.get(API_URL, params={"get": "currentspeed"}, timeout=10)
            assert r.status_code < 500
