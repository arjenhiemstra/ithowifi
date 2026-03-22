"""
OpenAPI spec validation tests. Verifies the spec documents all actual
API parameters and that every documented enum value works against the
live device.

Usage:
    pytest tests/api/test_openapi_spec.py -v
"""
import os
import requests
import pytest

DEVICE_IP = os.environ.get("ITHO_DEVICE", "<device-ip>")
DEVICE_URL = f"http://{DEVICE_IP}"
API_URL = f"{DEVICE_URL}/api.html"
SPEC_URL = f"{DEVICE_URL}/api/openapi.json"


@pytest.fixture(scope="module")
def spec():
    r = requests.get(SPEC_URL, timeout=5)
    return r.json()


@pytest.fixture(scope="module")
def api_params(spec):
    return spec["paths"]["/api.html"]["get"]["parameters"]


class TestSpecStructure:
    """Verify OpenAPI spec has valid structure."""

    def test_openapi_version(self, spec):
        assert spec["openapi"] == "3.0.3"

    def test_info_present(self, spec):
        assert "title" in spec["info"]
        assert "version" in spec["info"]

    def test_paths_present(self, spec):
        assert "/api.html" in spec["paths"]
        assert "/api/openapi.json" in spec["paths"]

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


class TestSpecParameterCompleteness:
    """Verify all expected parameters are documented."""

    def test_general_params(self, api_params):
        names = [p["name"] for p in api_params]
        for expected in ["command", "speed", "timer"]:
            assert expected in names, f"Missing param: {expected}"

    def test_vremote_params(self, api_params):
        names = [p["name"] for p in api_params]
        for expected in ["vremotecmd", "vremoteindex", "vremotename"]:
            assert expected in names, f"Missing param: {expected}"

    def test_get_params(self, api_params):
        names = [p["name"] for p in api_params]
        assert "get" in names
        assert "name" in names

    def test_settings_params(self, api_params):
        names = [p["name"] for p in api_params]
        for expected in ["getsetting", "setsetting", "value"]:
            assert expected in names, f"Missing param: {expected}"

    def test_rf_params(self, api_params):
        names = [p["name"] for p in api_params]
        for expected in ["rfremotecmd", "rfremoteindex", "rfco2", "rfdemand", "rfzone"]:
            assert expected in names, f"Missing param: {expected}"

    def test_wpu_params(self, api_params):
        names = [p["name"] for p in api_params]
        assert "outside_temp" in names


class TestSpecEnumValuesWork:
    """Verify every enum value documented in the spec works against the device."""

    def test_all_command_enum_values(self, api_params):
        cmd_param = next(p for p in api_params if p["name"] == "command")
        for cmd in cmd_param["schema"]["enum"]:
            r = requests.get(API_URL, params={"command": cmd}, timeout=10)
            assert r.status_code == 200, f"command={cmd} returned {r.status_code}: {r.text}"

    def test_all_get_enum_values(self, api_params):
        get_param = next(p for p in api_params if p["name"] == "get")
        for val in get_param["schema"]["enum"]:
            r = requests.get(API_URL, params={"get": val}, timeout=10)
            assert r.status_code == 200, f"get={val} returned {r.status_code}: {r.text}"

    def test_all_rfremotecmd_enum_values(self, api_params):
        rf_param = next(p for p in api_params if p["name"] == "rfremotecmd")
        for cmd in rf_param["schema"]["enum"]:
            r = requests.get(API_URL, params={"rfremotecmd": cmd, "rfremoteindex": "0"}, timeout=10)
            assert r.status_code < 500, f"rfremotecmd={cmd} returned {r.status_code}"

    def test_all_vremotecmd_enum_values(self, api_params):
        vr_param = next(p for p in api_params if p["name"] == "vremotecmd")
        for cmd in vr_param["schema"]["enum"]:
            r = requests.get(API_URL, params={"vremotecmd": cmd}, timeout=10)
            assert r.status_code < 500, f"vremotecmd={cmd} returned {r.status_code}"

    def test_speed_boundaries_match_spec(self, api_params):
        speed_param = next(p for p in api_params if p["name"] == "speed")
        min_val = speed_param["schema"]["minimum"]
        max_val = speed_param["schema"]["maximum"]
        for val in [min_val, max_val]:
            r = requests.get(API_URL, params={"speed": str(val)}, timeout=10)
            assert r.json().get("status") == "success", f"speed={val} failed"

    def test_restore_low(self):
        requests.get(API_URL, params={"command": "low"}, timeout=10)
