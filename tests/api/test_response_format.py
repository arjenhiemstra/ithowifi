"""
Response format validation tests. Verifies all API responses follow
the JSend specification consistently.

Usage:
    pytest tests/api/test_response_format.py -v
"""
import os
import requests

DEVICE_IP = os.environ.get("ITHO_DEVICE", "")
DEVICE_URL = f"http://{DEVICE_IP}"
REST_URL = f"{DEVICE_URL}/api/v2"


class TestJSendFormat:
    """Verify API responses follow JSend specification."""

    def test_success_has_status_and_data(self):
        r = requests.get(f"{REST_URL}/speed", timeout=10)
        data = r.json()
        assert "status" in data, "Missing 'status' field"
        assert data["status"] == "success"
        assert "data" in data, "Success response must have 'data' field"

    def test_success_has_timestamp(self):
        r = requests.get(f"{REST_URL}/speed", timeout=10)
        data = r.json()
        assert "timestamp" in data.get("data", {}), "Missing timestamp"
        assert isinstance(data["data"]["timestamp"], int)

    def test_fail_has_status_and_data(self):
        """Fail responses should have status=fail and a data object."""
        r = requests.post(f"{REST_URL}/rfco2", json={"co2": 800, "index": 99}, timeout=10)
        data = r.json()
        assert "status" in data
        assert data["status"] == "fail"
        assert "data" in data

    def test_fail_has_failreason(self):
        r = requests.post(f"{REST_URL}/rfco2", json={"co2": 800, "index": 99}, timeout=10)
        data = r.json()
        assert "failreason" in data.get("data", {}), "Fail response should have failreason"

    def test_content_type_is_json(self):
        r = requests.get(f"{REST_URL}/speed", timeout=10)
        assert "application/json" in r.headers.get("Content-Type", "")

    def test_get_device_structure(self):
        r = requests.get(f"{REST_URL}/device", timeout=10)
        data = r.json()
        assert data["status"] == "success"
        info = data["data"]["deviceinfo"]
        assert "itho_devtype" in info
        assert "add-on_fwversion" in info
        assert "add-on_hwid" in info

    def test_get_remotes_structure(self):
        r = requests.get(f"{REST_URL}/remotes", timeout=10)
        data = r.json()
        assert data["status"] in ("success", "fail")

    def test_get_speed_structure(self):
        r = requests.get(f"{REST_URL}/speed", timeout=10)
        data = r.json()
        assert data["status"] == "success"
        assert "currentspeed" in data["data"]

    def test_rfremote_success_structure(self):
        r = requests.post(f"{REST_URL}/rfremote", json={"command": "low", "index": 0}, timeout=10)
        data = r.json()
        assert "status" in data
        if data["status"] == "success":
            assert "result" in data["data"]

    def test_rfremote_fail_structure(self):
        """Invalid index should return structured fail."""
        r = requests.post(f"{REST_URL}/rfremote", json={"command": "low", "index": 99}, timeout=10)
        data = r.json()
        assert data["status"] == "fail"
        assert "failreason" in data["data"]


class TestConfigEndpoints:
    """Test direct config file access."""

    def test_config_json_readable(self):
        r = requests.get(f"http://{DEVICE_IP}/config.json", timeout=10)
        assert r.status_code == 200
        data = r.json()
        assert "itho_rf_support" in data
        assert "mqtt_active" in data
        assert "api_version" in data

    def test_config_passwords_masked(self):
        """Passwords in config.json should be masked."""
        r = requests.get(f"http://{DEVICE_IP}/config.json", timeout=10)
        data = r.json()
        assert data.get("sys_password") == "********"
        assert data.get("mqtt_password") == "********"

    def test_remotes_json_readable(self):
        r = requests.get(f"http://{DEVICE_IP}/remotes.json", timeout=10)
        assert r.status_code == 200
        data = r.json()
        # Key is "remotes." (with dot) due to filename-based serialization
        remotes_key = next((k for k in data.keys() if k.startswith("remotes")), None)
        assert remotes_key is not None

    def test_remotes_json_has_destid(self):
        """New destid field should be present for remotes with binding."""
        r = requests.get(f"http://{DEVICE_IP}/remotes.json", timeout=10)
        data = r.json()
        remotes_key = next(k for k in data.keys() if k.startswith("remotes"))
        for remote in data[remotes_key]:
            # destid is optional — only present if binding was done
            if "destid" in remote:
                assert isinstance(remote["destid"], list)
                assert len(remote["destid"]) == 3

    def test_remotes_json_has_bidirectional(self):
        r = requests.get(f"http://{DEVICE_IP}/remotes.json", timeout=10)
        data = r.json()
        remotes_key = next(k for k in data.keys() if k.startswith("remotes"))
        for remote in data[remotes_key]:
            assert "bidirectional" in remote
            assert isinstance(remote["bidirectional"], bool)

    def test_wifi_json_readable(self):
        r = requests.get(f"http://{DEVICE_IP}/wifi.json", timeout=10)
        assert r.status_code == 200

    def test_file_list(self):
        r = requests.get(f"http://{DEVICE_IP}/list", params={"dir": "/"}, timeout=10)
        assert r.status_code == 200
        files = r.json()
        filenames = [f["name"] for f in files]
        assert "config.json" in filenames
        assert "remotes.json" in filenames

    def test_openapi_json(self):
        r = requests.get(f"http://{DEVICE_IP}/api/openapi.json", timeout=10)
        assert r.status_code == 200
        spec = r.json()
        assert spec["openapi"] == "3.0.3"


class TestVirtualRemoteCommands:
    """Test vremote REST endpoint."""

    def test_vremote_low(self):
        r = requests.post(f"{REST_URL}/vremote", json={"command": "low"}, timeout=10)
        assert r.status_code < 500

    def test_vremote_medium(self):
        r = requests.post(f"{REST_URL}/vremote", json={"command": "medium"}, timeout=10)
        assert r.status_code < 500

    def test_vremote_with_index(self):
        r = requests.post(f"{REST_URL}/vremote", json={"command": "low", "index": 0}, timeout=10)
        assert r.status_code < 500

    def test_vremote_invalid(self):
        r = requests.post(f"{REST_URL}/vremote", json={"command": "nonexistent"}, timeout=10)
        assert r.status_code < 500

    def test_vremote_invalid_index(self):
        r = requests.post(f"{REST_URL}/vremote", json={"command": "low", "index": 99}, timeout=10)
        assert r.status_code < 500

    def test_restore_low(self):
        requests.post(f"{REST_URL}/command", json={"command": "low"}, timeout=10)
