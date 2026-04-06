"""
OpenAPI spec completeness tests. Verifies the spec documents all
actual API v2 endpoints and MQTT commands.

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
def v2_paths(spec):
    return [p for p in spec["paths"].keys() if p.startswith("/api/v2/")]


@pytest.fixture(scope="module")
def mqtt_schema_props(spec):
    return list(spec["components"]["schemas"]["MqttCommand"]["properties"].keys())


class TestRESTv2EndpointCoverage:
    """Every REST API v2 endpoint should be documented in the spec."""

    def test_get_endpoints(self, v2_paths):
        for expected in ["/api/v2/speed", "/api/v2/ithostatus", "/api/v2/deviceinfo",
                         "/api/v2/queue", "/api/v2/lastcmd", "/api/v2/remotes",
                         "/api/v2/vremotes", "/api/v2/rfstatus", "/api/v2/settings"]:
            assert expected in v2_paths, f"Missing GET endpoint: {expected}"

    def test_post_endpoints(self, v2_paths):
        for expected in ["/api/v2/command", "/api/v2/vremote", "/api/v2/rfremote/command",
                         "/api/v2/rfremote/co2", "/api/v2/rfremote/demand", "/api/v2/debug",
                         "/api/v2/wpu/outside_temp"]:
            assert expected in v2_paths, f"Missing POST endpoint: {expected}"

    def test_put_endpoints(self, v2_paths):
        assert "/api/v2/settings" in v2_paths, "Missing PUT endpoint: /api/v2/settings"

    def test_command_endpoint_has_all_commands(self, spec):
        """Verify the command endpoint documents all known commands."""
        # Follow $ref to the schema in components
        cmd_schema = spec["components"]["schemas"].get("CommandRequest", {})
        cmd_prop = cmd_schema.get("properties", {}).get("command", {})
        enum_vals = cmd_prop.get("enum", [])
        for expected in ["low", "medium", "high", "timer1", "timer2", "timer3", "away",
                         "cook30", "cook60", "autonight", "clearqueue"]:
            assert expected in enum_vals, f"Missing command: {expected}"

    def test_rfremote_endpoint_has_all_commands(self, spec):
        # Follow $ref to the schema in components
        rf_schema = spec["components"]["schemas"].get("RFRemoteRequest", {})
        cmd_prop = rf_schema.get("properties", {}).get("command", {})
        enum_vals = cmd_prop.get("enum", [])
        for expected in ["low", "medium", "high", "join", "leave", "auto", "autonight",
                         "motion_on", "motion_off"]:
            assert expected in enum_vals, f"Missing rfremote command: {expected}"


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
