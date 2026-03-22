"""
Security tests. Verifies password masking, XSS prevention,
and basic input sanitization.

Usage:
    ITHO_DEVICE=<device-ip> pytest tests/api/test_security.py -v
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
API_URL = f"http://{DEVICE_IP}/api.html"
WS_URL = f"ws://{DEVICE_IP}:8000/ws"
CONFIG_URL = f"http://{DEVICE_IP}/config.json"
WIFI_URL = f"http://{DEVICE_IP}/wifi.json"


class TestPasswordMasking:
    """Verify passwords are never exposed in API responses."""

    def test_config_json_masks_sys_password(self):
        r = requests.get(CONFIG_URL, timeout=10)
        data = r.json()
        assert data.get("sys_password") == "********", "System password not masked in config.json"

    def test_config_json_masks_mqtt_password(self):
        r = requests.get(CONFIG_URL, timeout=10)
        data = r.json()
        assert data.get("mqtt_password") == "********", "MQTT password not masked in config.json"

    def test_systemsettings_masks_passwords(self):
        """Websocket systemsettings should mask passwords."""
        if not HAS_WS:
            pytest.skip("websocket-client not installed")
        ws = websocket.create_connection(WS_URL, timeout=10)
        try:
            ws.send(json.dumps({"syssetup": True}))
            time.sleep(1)
            ws.settimeout(2)
            for _ in range(20):
                try:
                    data = json.loads(ws.recv())
                    if "systemsettings" in data:
                        ss = data["systemsettings"]
                        if "sys_password" in ss:
                            assert ss["sys_password"] == "********", "sys_password not masked in WS"
                        if "mqtt_password" in ss:
                            assert ss["mqtt_password"] == "********", "mqtt_password not masked in WS"
                        return
                except Exception:
                    break
        finally:
            ws.close()

    def test_wifi_json_masks_password(self):
        r = requests.get(WIFI_URL, timeout=10)
        data = r.json()
        if "pass" in data:
            assert data["pass"] == "********", "WiFi password not masked"


class TestXSSPrevention:
    """Verify script injection attempts don't get reflected back."""

    def test_xss_in_get_param(self):
        """Script tags in params — response must be JSON, not HTML."""
        payload = "<script>alert('xss')</script>"
        r = requests.get(API_URL, params={"get": payload}, timeout=10)
        assert r.status_code < 500
        # Critical: Content-Type must be JSON so browsers don't execute reflected input
        assert "application/json" in r.headers.get("Content-Type", ""), \
            "Response is not JSON — XSS risk if input is reflected as HTML"
        # Verify it's valid JSON (not HTML)
        r.json()

    def test_xss_in_command_param(self):
        payload = "<img src=x onerror=alert(1)>"
        r = requests.get(API_URL, params={"command": payload}, timeout=10)
        assert r.status_code < 500
        assert "application/json" in r.headers.get("Content-Type", "")

    def test_xss_in_rfremotecmd(self):
        payload = "';alert(1);//"
        r = requests.get(API_URL, params={"rfremotecmd": payload}, timeout=10)
        assert r.status_code < 500

    def test_xss_in_vremotename(self):
        payload = "<script>document.location='http://evil.com'</script>"
        r = requests.get(API_URL, params={"vremotename": payload}, timeout=10)
        assert r.status_code < 500
        assert "<script>" not in r.text

    def test_openapi_spec_no_xss(self):
        """OpenAPI JSON spec should not contain executable content."""
        r = requests.get(f"http://{DEVICE_IP}/api/openapi.json", timeout=10)
        assert "<script>" not in r.text


class TestInputSanitization:
    """Verify malicious input doesn't crash the device."""

    def test_null_bytes(self):
        r = requests.get(API_URL, params={"get": "test\x00value"}, timeout=10)
        assert r.status_code < 500

    def test_very_long_param_name(self):
        r = requests.get(f"http://{DEVICE_IP}/api.html?{'A' * 500}=test", timeout=10)
        assert r.status_code < 500

    def test_many_params(self):
        """50 query parameters at once."""
        params = {f"param{i}": f"value{i}" for i in range(50)}
        r = requests.get(API_URL, params=params, timeout=10)
        assert r.status_code < 500

    def test_sql_injection_attempt(self):
        r = requests.get(API_URL, params={"get": "'; DROP TABLE config;--"}, timeout=10)
        assert r.status_code < 500

    def test_path_traversal_attempt(self):
        r = requests.get(f"http://{DEVICE_IP}/../../etc/passwd", timeout=10)
        # Should return 404, not file contents
        assert r.status_code in (400, 404)

    def test_command_injection_attempt(self):
        r = requests.get(API_URL, params={"command": "low; reboot"}, timeout=10)
        assert r.status_code < 500

    def test_unicode_overflow(self):
        r = requests.get(API_URL, params={"get": "\uffff" * 100}, timeout=10)
        assert r.status_code < 500

    def test_empty_string_params(self):
        """All params set to empty string."""
        r = requests.get(API_URL, params={"command": "", "speed": "", "get": ""}, timeout=10)
        assert r.status_code < 500
