"""
OpenAPI spec completeness tests. Verifies the spec documents all
actual API processors and MQTT commands.

Usage:
    ITHO_DEVICE=<device-ip> pytest tests/api/test_openapi_completeness.py -v
"""
import os
import requests
import pytest

DEVICE_IP = os.environ.get("ITHO_DEVICE", "")
SPEC_URL = f"http://{DEVICE_IP}/api/openapi.json"


@pytest.fixture(scope="module")
def spec():
    r = requests.get(SPEC_URL, timeout=5)
    return r.json()


@pytest.fixture(scope="module")
def api_param_names(spec):
    return [p["name"] for p in spec["paths"]["/api.html"]["get"]["parameters"]]


@pytest.fixture(scope="module")
def mqtt_schema_props(spec):
    return list(spec["components"]["schemas"]["MqttCommand"]["properties"].keys())


class TestWebAPIParamCoverage:
    """Every WebAPIv2 processor should have its params in the spec."""

    def test_general_commands(self, api_param_names):
        assert "command" in api_param_names
        assert "speed" in api_param_names
        assert "timer" in api_param_names

    def test_virtual_remote(self, api_param_names):
        assert "vremotecmd" in api_param_names
        assert "vremoteindex" in api_param_names
        assert "vremotename" in api_param_names

    def test_get_commands(self, api_param_names):
        assert "get" in api_param_names
        assert "name" in api_param_names

    def test_settings(self, api_param_names):
        assert "getsetting" in api_param_names
        assert "setsetting" in api_param_names
        assert "value" in api_param_names

    def test_rf_remote(self, api_param_names):
        assert "rfremotecmd" in api_param_names
        assert "rfremoteindex" in api_param_names

    def test_rf_co2_demand(self, api_param_names):
        assert "rfco2" in api_param_names
        assert "rfdemand" in api_param_names
        assert "rfzone" in api_param_names

    def test_wpu(self, api_param_names):
        assert "outside_temp" in api_param_names

    def test_get_enum_includes_all_endpoints(self, spec):
        """Verify the 'get' param enum includes all known endpoints."""
        params = spec["paths"]["/api.html"]["get"]["parameters"]
        get_param = next(p for p in params if p["name"] == "get")
        enum_vals = get_param["schema"]["enum"]
        for expected in ["ithostatus", "remotesinfo", "vremotesinfo", "deviceinfo", "currentspeed", "rfstatus"]:
            assert expected in enum_vals, f"Missing get value: {expected}"

    def test_command_enum_includes_all_commands(self, spec):
        params = spec["paths"]["/api.html"]["get"]["parameters"]
        cmd_param = next(p for p in params if p["name"] == "command")
        enum_vals = cmd_param["schema"]["enum"]
        for expected in ["low", "medium", "high", "timer1", "timer2", "timer3", "away",
                         "cook30", "cook60", "autonight", "clearqueue"]:
            assert expected in enum_vals, f"Missing command: {expected}"

    def test_rfremotecmd_enum_includes_all(self, spec):
        params = spec["paths"]["/api.html"]["get"]["parameters"]
        rf_param = next(p for p in params if p["name"] == "rfremotecmd")
        enum_vals = rf_param["schema"]["enum"]
        for expected in ["low", "medium", "high", "join", "leave", "auto", "autonight",
                         "motion_on", "motion_off"]:
            assert expected in enum_vals, f"Missing rfremotecmd: {expected}"


class TestMQTTSchemaCoverage:
    """MqttCommand schema should document all MQTT JSON keys."""

    def test_basic_commands(self, mqtt_schema_props):
        assert "command" in mqtt_schema_props
        assert "speed" in mqtt_schema_props
        assert "timer" in mqtt_schema_props

    def test_virtual_remote(self, mqtt_schema_props):
        assert "vremotecmd" in mqtt_schema_props
        assert "vremoteindex" in mqtt_schema_props
        assert "vremotename" in mqtt_schema_props

    def test_rf_commands(self, mqtt_schema_props):
        assert "rfremotecmd" in mqtt_schema_props
        assert "rfremoteindex" in mqtt_schema_props

    def test_co2_demand(self, mqtt_schema_props):
        assert "rfco2" in mqtt_schema_props
        assert "rfdemand" in mqtt_schema_props
        assert "rfzone" in mqtt_schema_props

    def test_domoticz(self, mqtt_schema_props):
        assert "dtype" in mqtt_schema_props

    def test_api_response_schema(self, spec):
        """ApiResponse schema should have JSend fields."""
        schema = spec["components"]["schemas"]["ApiResponse"]
        props = schema["properties"]
        assert "status" in props
        assert "data" in props
        assert "message" in props
        assert "timestamp" in props

    def test_api_response_status_enum(self, spec):
        schema = spec["components"]["schemas"]["ApiResponse"]
        status_enum = schema["properties"]["status"]["enum"]
        assert "success" in status_enum
        assert "fail" in status_enum
        assert "error" in status_enum
