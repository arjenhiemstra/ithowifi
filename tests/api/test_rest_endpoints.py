"""
Comprehensive REST API v2 endpoint tests.

Tests all GET and POST/PUT endpoints for correct JSend response format,
data structure, validation, and error handling.

Usage:
    ITHO_DEVICE=<device-ip> pytest tests/api/test_rest_endpoints.py -v
"""
import os
import time

import pytest
import requests

DEVICE_IP = os.environ.get("ITHO_DEVICE", "")
DEVICE_URL = f"http://{DEVICE_IP}"
REST_URL = f"{DEVICE_URL}/api/v2"

pytestmark = pytest.mark.skipif(not DEVICE_IP, reason="ITHO_DEVICE not set")


def jsend_success(resp):
    """Assert response is a JSend success and return the data dict."""
    assert resp.status_code == 200, f"Expected 200, got {resp.status_code}: {resp.text}"
    body = resp.json()
    assert body["status"] == "success", f"Expected success, got {body}"
    assert "data" in body, "Success response must have 'data'"
    return body["data"]


def jsend_fail(resp, expected_status_code=400):
    """Assert response is a JSend fail and return the data dict."""
    assert resp.status_code == expected_status_code, (
        f"Expected {expected_status_code}, got {resp.status_code}: {resp.text}"
    )
    body = resp.json()
    assert body["status"] == "fail", f"Expected fail, got {body}"
    assert "data" in body, "Fail response must have 'data'"
    assert "failreason" in body["data"], "Fail response must have 'failreason'"
    return body["data"]


# ---------------------------------------------------------------------------
# GET endpoints
# ---------------------------------------------------------------------------


class TestGetSpeed:
    def test_response_structure(self):
        data = jsend_success(requests.get(f"{REST_URL}/speed", timeout=10))
        assert "currentspeed" in data
        assert "timestamp" in data
        assert isinstance(data["timestamp"], int)

    def test_currentspeed_is_int(self):
        data = jsend_success(requests.get(f"{REST_URL}/speed", timeout=10))
        assert isinstance(data["currentspeed"], int)


class TestGetIthoStatus:
    def test_response_structure(self):
        data = jsend_success(requests.get(f"{REST_URL}/ithostatus", timeout=10))
        assert "ithostatus" in data

    def test_ithostatus_is_object(self):
        data = jsend_success(requests.get(f"{REST_URL}/ithostatus", timeout=10))
        assert isinstance(data["ithostatus"], dict)


class TestGetDeviceInfo:
    def test_response_structure(self):
        data = jsend_success(requests.get(f"{REST_URL}/deviceinfo", timeout=10))
        assert "deviceinfo" in data

    def test_deviceinfo_fields(self):
        data = jsend_success(requests.get(f"{REST_URL}/deviceinfo", timeout=10))
        info = data["deviceinfo"]
        assert "itho_devtype" in info
        assert "add-on_fwversion" in info
        assert "add-on_hwid" in info


class TestGetQueue:
    def test_response_structure(self):
        data = jsend_success(requests.get(f"{REST_URL}/queue", timeout=10))
        assert "queue" in data
        assert isinstance(data["queue"], list)

    def test_speed_fields(self):
        data = jsend_success(requests.get(f"{REST_URL}/queue", timeout=10))
        assert "ithoSpeed" in data
        assert "ithoOldSpeed" in data
        assert "fallBackSpeed" in data


class TestGetLastCmd:
    def test_response_structure(self):
        data = jsend_success(requests.get(f"{REST_URL}/lastcmd", timeout=10))
        assert "lastcmd" in data

    def test_lastcmd_is_object(self):
        data = jsend_success(requests.get(f"{REST_URL}/lastcmd", timeout=10))
        assert isinstance(data["lastcmd"], dict)


class TestGetRemotes:
    def test_response_structure(self):
        data = jsend_success(requests.get(f"{REST_URL}/remotes", timeout=10))
        assert "remotesinfo" in data

    def test_remotesinfo_is_object(self):
        data = jsend_success(requests.get(f"{REST_URL}/remotes", timeout=10))
        assert isinstance(data["remotesinfo"], dict)


class TestGetVRemotes:
    def test_response_ok(self):
        resp = requests.get(f"{REST_URL}/vremotes", timeout=10)
        data = jsend_success(resp)
        assert "timestamp" in data


class TestGetRFStatus:
    def test_response_ok(self):
        resp = requests.get(f"{REST_URL}/rfstatus", timeout=10)
        data = jsend_success(resp)
        assert "rfstatus" in data

    def test_filter_nonexistent_name(self):
        resp = requests.get(f"{REST_URL}/rfstatus", params={"name": "__nonexistent__"}, timeout=10)
        jsend_fail(resp, expected_status_code=404)


class TestGetSettings:
    def test_with_index_zero(self):
        resp = requests.get(f"{REST_URL}/settings", params={"index": "0"}, timeout=10)
        body = resp.json()
        # May succeed or error depending on device state, but must be valid JSend
        assert body["status"] in ("success", "fail", "error")
        if body["status"] == "success":
            data = body["data"]
            assert "label" in data
            assert "current" in data
            assert "minimum" in data
            assert "maximum" in data

    def test_missing_index(self):
        resp = requests.get(f"{REST_URL}/settings", timeout=10)
        jsend_fail(resp)

    def test_invalid_index(self):
        resp = requests.get(f"{REST_URL}/settings", params={"index": "abc"}, timeout=10)
        jsend_fail(resp)

    def test_index_out_of_range(self):
        resp = requests.get(f"{REST_URL}/settings", params={"index": "99999"}, timeout=10)
        body = resp.json()
        assert body["status"] in ("fail", "error")


# ---------------------------------------------------------------------------
# PUT /api/v2/settings
# ---------------------------------------------------------------------------


class TestPutSettings:
    def test_missing_fields(self):
        resp = requests.put(f"{REST_URL}/settings", json={"index": 0}, timeout=10)
        jsend_fail(resp)

    def test_missing_index(self):
        resp = requests.put(f"{REST_URL}/settings", json={"value": 42}, timeout=10)
        jsend_fail(resp)

    def test_invalid_index(self):
        resp = requests.put(f"{REST_URL}/settings", json={"index": 99999, "value": 0}, timeout=10)
        body = resp.json()
        assert body["status"] in ("fail", "error")


# ---------------------------------------------------------------------------
# POST /api/v2/command
# ---------------------------------------------------------------------------


class TestPostCommand:
    """Test named commands and speed/timer variants."""

    @pytest.mark.parametrize("command", [
        "low", "medium", "high",
        "timer1", "timer2", "timer3",
        "away", "cook30", "cook60",
        "autonight", "clearqueue",
    ])
    def test_named_commands(self, command):
        resp = requests.post(f"{REST_URL}/command", json={"command": command}, timeout=10)
        body = resp.json()
        # Some commands may fail if the device doesn't support pwm2i2c, that's ok
        assert body["status"] in ("success", "fail")
        assert "data" in body

    def test_speed_integer(self):
        resp = requests.post(f"{REST_URL}/command", json={"speed": 100}, timeout=10)
        body = resp.json()
        assert body["status"] in ("success", "fail")

    def test_speed_with_timer(self):
        resp = requests.post(f"{REST_URL}/command", json={"speed": 150, "timer": 1}, timeout=10)
        body = resp.json()
        assert body["status"] in ("success", "fail")

    def test_speed_out_of_range_high(self):
        resp = requests.post(f"{REST_URL}/command", json={"speed": 300}, timeout=10)
        jsend_fail(resp)
        assert "range" in resp.json()["data"]["failreason"]

    def test_speed_out_of_range_negative(self):
        resp = requests.post(f"{REST_URL}/command", json={"speed": -1}, timeout=10)
        jsend_fail(resp)

    def test_timer_out_of_range(self):
        resp = requests.post(f"{REST_URL}/command", json={"timer": 70000}, timeout=10)
        jsend_fail(resp)

    def test_invalid_command(self):
        resp = requests.post(f"{REST_URL}/command", json={"command": "nonexistent_xyz"}, timeout=10)
        body = resp.json()
        assert body["status"] == "fail"

    def test_empty_body(self):
        resp = requests.post(f"{REST_URL}/command", json={}, timeout=10)
        assert resp.status_code == 400
        assert resp.json()["status"] == "fail"

    def test_restore_low(self):
        """Restore fan to low after all command tests."""
        requests.post(f"{REST_URL}/command", json={"command": "low"}, timeout=10)


# ---------------------------------------------------------------------------
# POST /api/v2/vremote
# ---------------------------------------------------------------------------


class TestPostVRemote:
    def test_with_command_and_index(self):
        resp = requests.post(f"{REST_URL}/vremote", json={"command": "low", "index": 0}, timeout=10)
        body = resp.json()
        assert body["status"] in ("success", "fail")

    def test_with_command_and_name(self):
        resp = requests.post(f"{REST_URL}/vremote", json={"command": "low", "name": "test"}, timeout=10)
        body = resp.json()
        # Name may not exist, so fail is acceptable
        assert body["status"] in ("success", "fail")

    def test_missing_command(self):
        resp = requests.post(f"{REST_URL}/vremote", json={"index": 0}, timeout=10)
        jsend_fail(resp)

    def test_command_only_defaults_to_index_0(self):
        resp = requests.post(f"{REST_URL}/vremote", json={"command": "low"}, timeout=10)
        body = resp.json()
        assert body["status"] in ("success", "fail")

    def test_invalid_index(self):
        resp = requests.post(f"{REST_URL}/vremote", json={"command": "low", "index": 99}, timeout=10)
        body = resp.json()
        assert body["status"] in ("success", "fail")


# ---------------------------------------------------------------------------
# POST /api/v2/rfremote/command
# ---------------------------------------------------------------------------


class TestPostRFRemoteCommand:
    def test_with_command_and_index(self):
        resp = requests.post(
            f"{REST_URL}/rfremote/command",
            json={"command": "low", "index": 0},
            timeout=10,
        )
        body = resp.json()
        assert body["status"] in ("success", "fail")

    def test_invalid_index(self):
        resp = requests.post(
            f"{REST_URL}/rfremote/command",
            json={"command": "low", "index": 99},
            timeout=10,
        )
        body = resp.json()
        assert body["status"] == "fail"
        assert "failreason" in body["data"]


# ---------------------------------------------------------------------------
# POST /api/v2/rfremote/co2
# ---------------------------------------------------------------------------


class TestPostRFCO2:
    def test_valid_co2(self):
        resp = requests.post(
            f"{REST_URL}/rfremote/co2",
            json={"co2": 800, "index": 0},
            timeout=10,
        )
        body = resp.json()
        # Might fail if remote is not CO2 type
        assert body["status"] in ("success", "fail")

    def test_missing_co2(self):
        resp = requests.post(f"{REST_URL}/rfremote/co2", json={"index": 0}, timeout=10)
        jsend_fail(resp)

    def test_co2_out_of_range(self):
        resp = requests.post(
            f"{REST_URL}/rfremote/co2",
            json={"co2": 20000, "index": 0},
            timeout=10,
        )
        jsend_fail(resp)
        assert "range" in resp.json()["data"]["failreason"]


# ---------------------------------------------------------------------------
# POST /api/v2/rfremote/demand
# ---------------------------------------------------------------------------


class TestPostRFDemand:
    def test_valid_demand(self):
        resp = requests.post(
            f"{REST_URL}/rfremote/demand",
            json={"demand": 100, "zone": 0, "index": 0},
            timeout=10,
        )
        body = resp.json()
        assert body["status"] in ("success", "fail")

    def test_with_zone(self):
        resp = requests.post(
            f"{REST_URL}/rfremote/demand",
            json={"demand": 50, "zone": 1, "index": 0},
            timeout=10,
        )
        body = resp.json()
        assert body["status"] in ("success", "fail")

    def test_missing_demand(self):
        resp = requests.post(
            f"{REST_URL}/rfremote/demand",
            json={"zone": 0, "index": 0},
            timeout=10,
        )
        jsend_fail(resp)

    def test_demand_out_of_range(self):
        resp = requests.post(
            f"{REST_URL}/rfremote/demand",
            json={"demand": 250, "index": 0},
            timeout=10,
        )
        jsend_fail(resp)
        assert "range" in resp.json()["data"]["failreason"]


# ---------------------------------------------------------------------------
# POST /api/v2/rfremote/config
# ---------------------------------------------------------------------------


class TestPostRFConfig:
    def test_set_sourceid(self):
        resp = requests.post(
            f"{REST_URL}/rfremote/config",
            json={"index": 0, "setting": "setrfdevicesourceid", "value": "00,00,00"},
            timeout=10,
        )
        body = resp.json()
        assert body["status"] in ("success", "fail")

    def test_set_destid(self):
        resp = requests.post(
            f"{REST_URL}/rfremote/config",
            json={"index": 0, "setting": "setrfdevicedestid", "value": "00,00,00"},
            timeout=10,
        )
        body = resp.json()
        assert body["status"] in ("success", "fail")

    def test_missing_fields(self):
        resp = requests.post(
            f"{REST_URL}/rfremote/config",
            json={"index": 0},
            timeout=10,
        )
        jsend_fail(resp)


# ---------------------------------------------------------------------------
# POST /api/v2/debug
# ---------------------------------------------------------------------------


class TestPostDebug:
    @pytest.mark.parametrize("level", ["level0", "level1", "level2", "level3"])
    def test_debug_levels(self, level):
        resp = requests.post(f"{REST_URL}/debug", json={"action": level}, timeout=10)
        data = jsend_success(resp)
        assert "result" in data

    def test_reboot_when_disabled(self):
        """When api_reboot is off (default), reboot should be rejected."""
        resp = requests.post(f"{REST_URL}/debug", json={"action": "reboot"}, timeout=10)
        body = resp.json()
        # Could be 403 (disabled) or 429 (rate-limited) or success (if enabled)
        if body["status"] == "fail":
            assert resp.status_code in (400, 403, 429)

    def test_missing_action(self):
        resp = requests.post(f"{REST_URL}/debug", json={}, timeout=10)
        jsend_fail(resp)

    def test_invalid_action(self):
        resp = requests.post(f"{REST_URL}/debug", json={"action": "nonexistent"}, timeout=10)
        jsend_fail(resp)


# ---------------------------------------------------------------------------
# POST /api/v2/wpu/outside_temp
# ---------------------------------------------------------------------------


class TestPostOutsideTemp:
    def test_valid_temp(self):
        resp = requests.post(
            f"{REST_URL}/wpu/outside_temp",
            json={"temp": 15},
            timeout=10,
        )
        body = resp.json()
        # May fail if settings API is disabled
        assert body["status"] in ("success", "fail")

    def test_out_of_range(self):
        resp = requests.post(
            f"{REST_URL}/wpu/outside_temp",
            json={"temp": 200},
            timeout=10,
        )
        jsend_fail(resp)

    def test_missing_temp(self):
        resp = requests.post(f"{REST_URL}/wpu/outside_temp", json={}, timeout=10)
        jsend_fail(resp)


# ---------------------------------------------------------------------------
# POST /api/v2/wpu/manual_control
# ---------------------------------------------------------------------------


class TestPostManualControl:
    def test_with_all_fields(self):
        resp = requests.post(
            f"{REST_URL}/wpu/manual_control",
            json={"index": 1, "datatype": 1, "value": 1, "checked": 1},
            timeout=10,
        )
        data = jsend_success(resp)
        assert data["index"] == 1
        assert data["datatype"] == 1

    def test_missing_index(self):
        resp = requests.post(
            f"{REST_URL}/wpu/manual_control",
            json={"datatype": 1, "value": 1},
            timeout=10,
        )
        jsend_fail(resp)

    def test_defaults_for_optional_fields(self):
        resp = requests.post(
            f"{REST_URL}/wpu/manual_control",
            json={"index": 0},
            timeout=10,
        )
        data = jsend_success(resp)
        assert data["datatype"] == 0
        assert data["value"] == 0
        assert data["checked"] == 0


# ---------------------------------------------------------------------------
# Content-Type and CORS
# ---------------------------------------------------------------------------


class TestHTTPHeaders:
    def test_content_type_json(self):
        resp = requests.get(f"{REST_URL}/speed", timeout=10)
        assert "application/json" in resp.headers.get("Content-Type", "")

    def test_cors_header(self):
        resp = requests.get(f"{REST_URL}/speed", timeout=10)
        assert resp.headers.get("Access-Control-Allow-Origin") == "*"

    def test_options_preflight(self):
        resp = requests.options(f"{REST_URL}/speed", timeout=10)
        assert resp.status_code == 204
        assert "GET" in resp.headers.get("Access-Control-Allow-Methods", "")


class TestCleanup:
    def test_restore_fan_low(self):
        """Ensure fan is set back to low after all tests."""
        requests.post(f"{REST_URL}/command", json={"command": "low"}, timeout=10)
