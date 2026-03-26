"""
OpenAPI spec validation tests. Verifies the spec documents all actual
API endpoints and that every documented enum value works against the
live device.

Usage:
    pytest tests/api/test_openapi_spec.py -v
"""
import os
import requests
import pytest

DEVICE_IP = os.environ.get("ITHO_DEVICE", "")
DEVICE_URL = f"http://{DEVICE_IP}"
REST_URL = f"{DEVICE_URL}/api/v2"
SPEC_URL = f"{DEVICE_URL}/api/openapi.json"


@pytest.fixture(scope="module")
def spec():
    r = requests.get(SPEC_URL, timeout=5)
    return r.json()


class TestSpecStructure:
    """Verify OpenAPI spec has valid structure."""

    def test_openapi_version(self, spec):
        assert spec["openapi"] == "3.0.3"

    def test_info_present(self, spec):
        assert "title" in spec["info"]
        assert "version" in spec["info"]

    def test_v2_paths_present(self, spec):
        paths = spec["paths"]
        for expected in ["/api/v2/speed", "/api/v2/command", "/api/v2/status",
                         "/api/v2/device", "/api/v2/remotes", "/api/v2/settings"]:
            assert expected in paths, f"Missing path: {expected}"
        assert "/api/openapi.json" in paths

    def test_components_present(self, spec):
        assert "ApiResponse" in spec["components"]["schemas"]
        assert "MqttCommand" in spec["components"]["schemas"]

    def test_api_response_schema(self, spec):
        schema = spec["components"]["schemas"]["ApiResponse"]
        props = schema["properties"]
        assert "status" in props
        assert "data" in props
        assert "timestamp" in props

    def test_mqtt_command_has_new_params(self, spec):
        """Verify rfco2/rfdemand/rfzone are in MqttCommand schema."""
        mqtt = spec["components"]["schemas"]["MqttCommand"]
        for key in ["rfco2", "rfdemand", "rfzone"]:
            assert key in mqtt["properties"], f"Missing {key} in MqttCommand"


class TestSpecEndpointCompleteness:
    """Verify all expected REST v2 endpoints are documented."""

    def test_get_endpoints(self, spec):
        paths = spec["paths"]
        for endpoint in ["/api/v2/speed", "/api/v2/status", "/api/v2/device",
                         "/api/v2/remotes", "/api/v2/vremotes", "/api/v2/rfstatus",
                         "/api/v2/queue", "/api/v2/lastcmd", "/api/v2/settings"]:
            assert endpoint in paths, f"Missing GET endpoint: {endpoint}"

    def test_post_endpoints(self, spec):
        paths = spec["paths"]
        for endpoint in ["/api/v2/command", "/api/v2/vremote", "/api/v2/rfremote",
                         "/api/v2/rfco2", "/api/v2/rfdemand", "/api/v2/debug",
                         "/api/v2/outside_temp"]:
            assert endpoint in paths, f"Missing POST endpoint: {endpoint}"

    def test_put_endpoints(self, spec):
        paths = spec["paths"]
        assert "/api/v2/settings" in paths, "Missing PUT endpoint: /api/v2/settings"


class TestSpecEnumValuesWork:
    """Verify every enum value documented in the spec works against the device."""

    def test_all_command_enum_values(self, spec):
        """Test all command enum values from the spec's command endpoint."""
        cmd_path = spec["paths"].get("/api/v2/command", {})
        # Find the command enum in the request body schema
        post_schema = cmd_path.get("post", {}).get("requestBody", {}).get(
            "content", {}).get("application/json", {}).get("schema", {})
        cmd_prop = post_schema.get("properties", {}).get("command", {})
        if "enum" not in cmd_prop:
            pytest.skip("No command enum found in spec")
        for cmd in cmd_prop["enum"]:
            r = requests.post(f"{REST_URL}/command", json={"command": cmd}, timeout=10)
            assert r.status_code == 200, f"command={cmd} returned {r.status_code}: {r.text}"

    def test_all_rfremote_command_enum_values(self, spec):
        """Test all rfremote command enum values from the spec."""
        rf_path = spec["paths"].get("/api/v2/rfremote", {})
        post_schema = rf_path.get("post", {}).get("requestBody", {}).get(
            "content", {}).get("application/json", {}).get("schema", {})
        cmd_prop = post_schema.get("properties", {}).get("command", {})
        if "enum" not in cmd_prop:
            pytest.skip("No rfremote command enum found in spec")
        for cmd in cmd_prop["enum"]:
            r = requests.post(f"{REST_URL}/rfremote", json={"command": cmd, "index": 0}, timeout=10)
            assert r.status_code < 500, f"rfremote command={cmd} returned {r.status_code}"

    def test_all_vremote_command_enum_values(self, spec):
        """Test all vremote command enum values from the spec."""
        vr_path = spec["paths"].get("/api/v2/vremote", {})
        post_schema = vr_path.get("post", {}).get("requestBody", {}).get(
            "content", {}).get("application/json", {}).get("schema", {})
        cmd_prop = post_schema.get("properties", {}).get("command", {})
        if "enum" not in cmd_prop:
            pytest.skip("No vremote command enum found in spec")
        for cmd in cmd_prop["enum"]:
            r = requests.post(f"{REST_URL}/vremote", json={"command": cmd}, timeout=10)
            assert r.status_code < 500, f"vremote command={cmd} returned {r.status_code}"

    def test_speed_boundaries_match_spec(self, spec):
        cmd_path = spec["paths"].get("/api/v2/command", {})
        post_schema = cmd_path.get("post", {}).get("requestBody", {}).get(
            "content", {}).get("application/json", {}).get("schema", {})
        speed_prop = post_schema.get("properties", {}).get("speed", {})
        if "minimum" not in speed_prop:
            pytest.skip("No speed boundaries found in spec")
        min_val = speed_prop["minimum"]
        max_val = speed_prop["maximum"]
        for val in [min_val, max_val]:
            r = requests.post(f"{REST_URL}/command", json={"speed": val}, timeout=10)
            assert r.json().get("status") == "success", f"speed={val} failed"

    def test_restore_low(self):
        requests.post(f"{REST_URL}/command", json={"command": "low"}, timeout=10)
