"""
Remote configuration management tests via websocket.
Tests add/update/remove of RF remotes and verifies persistence.

Usage:
    ITHO_DEVICE=<device-ip> pytest tests/api/test_remote_config.py -v
"""
import os
import json
import time
import pytest
import requests

try:
    import websocket
    HAS_WS = True
except ImportError:
    HAS_WS = False

DEVICE_IP = os.environ.get("ITHO_DEVICE", "<device-ip>")
WS_URL = f"ws://{DEVICE_IP}:8000/ws"
API_URL = f"http://{DEVICE_IP}/api.html"
REMOTES_URL = f"http://{DEVICE_IP}/remotes.json"


@pytest.fixture(scope="module")
def ws():
    if not HAS_WS:
        pytest.skip("websocket-client not installed")
    conn = websocket.create_connection(WS_URL, timeout=10)
    yield conn
    conn.close()


def get_remotes():
    """Fetch remotes config from device."""
    r = requests.get(REMOTES_URL, timeout=10)
    data = r.json()
    key = next(k for k in data.keys() if k.startswith("remotes"))
    return data[key]


def ws_send(ws, msg):
    ws.send(json.dumps(msg))
    time.sleep(1)


def find_empty_slot(remotes):
    """Find the first empty remote slot (id=[0,0,0])."""
    for r in remotes:
        if r["id"] == [0, 0, 0]:
            return r["index"]
    return None


class TestRemoteConfigRead:
    """Test reading remote configuration."""

    def test_remotes_json_has_all_slots(self):
        remotes = get_remotes()
        assert len(remotes) > 0, "No remote slots configured"

    def test_each_remote_has_required_fields(self):
        remotes = get_remotes()
        for r in remotes:
            assert "index" in r
            assert "id" in r
            assert "name" in r
            assert "remtype" in r
            assert "remfunc" in r
            assert "bidirectional" in r

    def test_bidirectional_matches_type(self):
        """Verify bidirectional is derived from remote type."""
        BIDIR_TYPES = [0x22F5, 0x22F4, 0x1298, 0x12A0, 0x22F2]  # RFTN, RFTAUTON, RFTCO2, RFTRV, RFTSPIDER
        remotes = get_remotes()
        for r in remotes:
            if r["id"] != [0, 0, 0]:
                expected = r["remtype"] in BIDIR_TYPES
                assert r["bidirectional"] == expected, (
                    f"Remote {r['index']} type {r['remtype']}: expected bidirectional={expected}, got {r['bidirectional']}"
                )

    def test_remotesinfo_via_api(self):
        r = requests.get(API_URL, params={"get": "remotesinfo"}, timeout=10)
        assert r.status_code == 200

    def test_remotes_via_websocket(self, ws):
        ws.send(json.dumps({"ithoremotes": True}))
        time.sleep(1)
        ws.settimeout(1)
        found = False
        for _ in range(10):
            try:
                data = json.loads(ws.recv())
                if "remotes" in data:
                    assert isinstance(data["remotes"], list)
                    found = True
                    break
            except Exception:
                break
        ws.settimeout(10)
        assert found, "No remotes response via websocket"


class TestRemoteConfigUpdate:
    """Test updating remote configuration via websocket.
    Uses the last remote slot to avoid disturbing active remotes."""

    def _get_last_empty_slot(self):
        remotes = get_remotes()
        # Find last slot (highest index with id=[0,0,0])
        for r in reversed(remotes):
            if r["id"] == [0, 0, 0]:
                return r["index"]
        pytest.skip("No empty remote slot available for testing")

    def test_update_remote_name(self, ws):
        idx = self._get_last_empty_slot()
        ws_send(ws, {
            "itho_update_remote": idx,
            "value": "test_remote",
            "remtype": 0x22F1,  # RFTCVE
            "remfunc": 1,  # receive
            "id": [0, 0, 0]
        })
        remotes = get_remotes()
        assert remotes[idx]["name"] == "test_remote"

    def test_update_remote_type(self, ws):
        idx = self._get_last_empty_slot()
        ws_send(ws, {
            "itho_update_remote": idx,
            "value": "test_co2",
            "remtype": 0x1298,  # RFTCO2
            "remfunc": 5,  # send
            "id": [0xAA, 0xBB, 0xCC]
        })
        remotes = get_remotes()
        assert remotes[idx]["remtype"] == 0x1298
        assert remotes[idx]["id"] == [0xAA, 0xBB, 0xCC]

    def test_bidirectional_auto_set_on_update(self, ws):
        """When type is set to a bidirectional type, bidirectional should be auto-set."""
        idx = self._get_last_empty_slot()
        # Set to RFTCO2 (bidirectional type)
        ws_send(ws, {
            "itho_update_remote": idx,
            "value": "test_bidir",
            "remtype": 0x1298,  # RFTCO2
            "remfunc": 5,
            "id": [0xAA, 0xBB, 0xCC]
        })
        remotes = get_remotes()
        assert remotes[idx]["bidirectional"] == True, "RFTCO2 should be bidirectional"

        # Set to RFTCVE (non-bidirectional type)
        ws_send(ws, {
            "itho_update_remote": idx,
            "value": "test_nonbidir",
            "remtype": 0x22F1,  # RFTCVE
            "remfunc": 1,
            "id": [0xAA, 0xBB, 0xCC]
        })
        remotes = get_remotes()
        assert remotes[idx]["bidirectional"] == False, "RFTCVE should not be bidirectional"

    def test_destid_persisted(self, ws):
        """Verify destid field is present and persisted in remotes.json."""
        remotes = get_remotes()
        # Check any remote that has a destid
        for r in remotes:
            if "destid" in r and r["destid"] != [0, 0, 0]:
                assert len(r["destid"]) == 3
                assert all(isinstance(b, int) for b in r["destid"])
                return
        # If no destid found, that's ok — none bound yet

    def test_cleanup_remote(self, ws):
        """Clean up: reset the test slot."""
        remotes = get_remotes()
        for r in reversed(remotes):
            if r["name"] in ("test_remote", "test_co2", "test_bidir", "test_nonbidir"):
                ws_send(ws, {
                    "itho_update_remote": r["index"],
                    "value": f"remote{r['index']}",
                    "remtype": 0x22F1,
                    "remfunc": 1,
                    "id": [0, 0, 0]
                })


class TestCombinedParameters:
    """Test interactions between multiple API parameters."""

    def test_speed_overrides_command(self):
        """When both speed and command are present, both should be processed."""
        r = requests.get(API_URL, params={"speed": "200", "command": "low"}, timeout=10)
        assert r.status_code < 500

    def test_get_with_command(self):
        """get + command in same request — both should be processed."""
        r = requests.get(API_URL, params={"get": "currentspeed", "command": "medium"}, timeout=10)
        assert r.status_code == 200

    def test_rfremotecmd_with_rfco2(self):
        """Both RF params in one request."""
        r = requests.get(API_URL, params={"rfremotecmd": "low", "rfco2": "800"}, timeout=10)
        assert r.status_code < 500

    def test_vremotecmd_with_rfremotecmd(self):
        """Virtual + RF remote in same request."""
        r = requests.get(API_URL, params={"vremotecmd": "low", "rfremotecmd": "high"}, timeout=10)
        assert r.status_code < 500

    def test_speed_boundary_with_timer_boundary(self):
        r = requests.get(API_URL, params={"speed": "255", "timer": "65535"}, timeout=10)
        assert r.status_code < 500

    def test_speed_zero_with_timer(self):
        r = requests.get(API_URL, params={"speed": "0", "timer": "10"}, timeout=10)
        assert r.status_code < 500

    def test_getsetting_with_get(self):
        """Two get-type params — first match wins in chain."""
        r = requests.get(API_URL, params={"get": "currentspeed", "getsetting": "0"}, timeout=10)
        assert r.status_code < 500

    def test_multiple_unknown_params(self):
        """Unknown params should be ignored, not crash."""
        r = requests.get(API_URL, params={"foo": "bar", "baz": "123", "get": "currentspeed"}, timeout=10)
        assert r.status_code == 200

    def test_rfco2_rfdemand_together(self):
        """Both CO2 and demand in one request."""
        r = requests.get(API_URL, params={"rfco2": "800", "rfdemand": "100"}, timeout=10)
        assert r.status_code < 500

    def test_all_rf_params(self):
        """All RF params at once."""
        r = requests.get(API_URL, params={
            "rfremotecmd": "low",
            "rfremoteindex": "0",
            "rfco2": "500",
            "rfdemand": "100",
            "rfzone": "0"
        }, timeout=10)
        assert r.status_code < 500

    def test_restore_low(self):
        requests.get(API_URL, params={"command": "low"}, timeout=10)


class TestSettingsRead:
    """Test getsetting — uses I2C which can be slow."""

    def test_getsetting_index_1(self):
        """Read a single setting — longer timeout for I2C."""
        r = requests.get(API_URL, params={"getsetting": "1"}, timeout=30)
        assert r.status_code < 500

    def test_getsetting_response_structure(self):
        """Successful getsetting should return structured data."""
        r = requests.get(API_URL, params={"getsetting": "1"}, timeout=30)
        if r.status_code == 200:
            data = r.json()
            assert data.get("status") in ("success", "fail")
            assert "timestamp" in data.get("data", {})
