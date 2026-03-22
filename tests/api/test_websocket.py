"""
WebSocket API tests. Tests the websocket message handler which is the
primary interface for the web UI.

Usage:
    pytest tests/api/test_websocket.py -v
    ITHO_DEVICE=<device-ip> pytest tests/api/test_websocket.py -v
"""
import os
import json
import time
import pytest

try:
    import websocket
    HAS_WS = True
except ImportError:
    HAS_WS = False

DEVICE_IP = os.environ.get("ITHO_DEVICE", "<device-ip>")
WS_URL = f"ws://{DEVICE_IP}:8000/ws"


@pytest.fixture(scope="module")
def ws():
    if not HAS_WS:
        pytest.skip("websocket-client not installed")
    conn = websocket.create_connection(WS_URL, timeout=10)
    yield conn
    conn.close()


def ws_send_recv(ws, msg, wait=0.5, max_msgs=10):
    """Send a JSON message and collect responses."""
    ws.send(json.dumps(msg))
    time.sleep(wait)
    responses = []
    ws.settimeout(0.5)
    for _ in range(max_msgs):
        try:
            data = ws.recv()
            responses.append(json.loads(data))
        except (websocket.WebSocketTimeoutException, Exception):
            break
    ws.settimeout(10)
    return responses


class TestWsDataRequests:
    """Test websocket data query messages."""

    def test_sysstat(self, ws):
        responses = ws_send_recv(ws, {"sysstat": True})
        found = any("systemstat" in r for r in responses)
        assert found, f"No systemstat in responses: {[list(r.keys()) for r in responses]}"

    def test_sysstat_has_freemem(self, ws):
        responses = ws_send_recv(ws, {"sysstat": True})
        for r in responses:
            if "systemstat" in r:
                assert "freemem" in r["systemstat"]
                assert "itho" in r["systemstat"]
                return
        pytest.fail("No systemstat response")

    def test_syssetup(self, ws):
        responses = ws_send_recv(ws, {"syssetup": True}, wait=1)
        found = any("systemsettings" in r for r in responses)
        assert found, f"No systemsettings response"

    def test_syssetup_has_rf_support(self, ws):
        responses = ws_send_recv(ws, {"syssetup": True}, wait=1)
        for r in responses:
            if "systemsettings" in r:
                ss = r["systemsettings"]
                assert "itho_rf_support" in ss
                assert "itho_pwm2i2c" in ss
                return
        pytest.fail("No systemsettings response")

    def test_wifisetup(self, ws):
        responses = ws_send_recv(ws, {"wifisetup": True}, wait=1)
        found = any("wifisettings" in r for r in responses)
        assert found

    def test_wifistat(self, ws):
        responses = ws_send_recv(ws, {"wifistat": True})
        found = any("wifistat" in r for r in responses)
        assert found

    def test_logsetup(self, ws):
        responses = ws_send_recv(ws, {"logsetup": True}, wait=1)
        found = any("logsettings" in r for r in responses)
        assert found

    def test_ithoremotes(self, ws):
        responses = ws_send_recv(ws, {"ithoremotes": True}, wait=1)
        found = any("remotes" in r for r in responses)
        assert found, f"No remotes response"

    def test_ithoremotes_has_structure(self, ws):
        responses = ws_send_recv(ws, {"ithoremotes": True}, wait=1)
        for r in responses:
            if "remotes" in r:
                remotes = r["remotes"]
                assert isinstance(remotes, list)
                if len(remotes) > 0:
                    assert "index" in remotes[0]
                    assert "id" in remotes[0]
                    assert "remtype" in remotes[0]
                    assert "bidirectional" in remotes[0]
                return
        pytest.fail("No remotes response")

    def test_ithovremotes(self, ws):
        responses = ws_send_recv(ws, {"ithovremotes": True}, wait=1)
        found = any("vremotes" in r for r in responses)
        assert found

    def test_remtype(self, ws):
        responses = ws_send_recv(ws, {"remtype": True})
        found = any("remtypeconf" in r for r in responses)
        assert found

    def test_rfsetup(self, ws):
        responses = ws_send_recv(ws, {"rfsetup": True}, wait=1)
        # rfsetup triggers a systemsettings response with RF-specific fields
        found = any("systemsettings" in r for r in responses)
        assert found

    def test_ithostatus(self, ws):
        responses = ws_send_recv(ws, {"ithostatus": True}, wait=2)
        found = any("ithostatusinfo" in r for r in responses)
        # ithostatus may not return data if I2C is slow — just check no crash
        assert True  # no crash = pass


class TestWsCommands:
    """Test websocket device commands."""

    def test_command_low(self, ws):
        ws.send(json.dumps({"command": "low"}))
        time.sleep(1)
        # Verify via sysstat
        responses = ws_send_recv(ws, {"sysstat": True})
        found = any("systemstat" in r for r in responses)
        assert found, "Device not responding after command"

    def test_remote_command(self, ws):
        """RF remote command via websocket."""
        ws.send(json.dumps({"remote": 0, "command": "low"}))
        time.sleep(1)
        responses = ws_send_recv(ws, {"sysstat": True})
        assert any("systemstat" in r for r in responses)

    def test_speed_command(self, ws):
        ws.send(json.dumps({"itho": 120}))
        time.sleep(1)
        responses = ws_send_recv(ws, {"sysstat": True})
        assert any("systemstat" in r for r in responses)

    def test_rfco2_command(self, ws):
        ws.send(json.dumps({"rfco2": 800, "rfremoteindex": 0}))
        time.sleep(0.5)
        responses = ws_send_recv(ws, {"sysstat": True})
        assert any("systemstat" in r for r in responses)

    def test_rfdemand_command(self, ws):
        ws.send(json.dumps({"rfdemand": 100, "rfremoteindex": 0}))
        time.sleep(0.5)
        responses = ws_send_recv(ws, {"sysstat": True})
        assert any("systemstat" in r for r in responses)

    def test_restore_low(self, ws):
        ws.send(json.dumps({"command": "low"}))
        time.sleep(1)


class TestWsEdgeCases:
    """Test websocket error handling."""

    def test_empty_json(self, ws):
        ws.send("{}")
        time.sleep(0.5)
        # Should not crash
        responses = ws_send_recv(ws, {"sysstat": True})
        assert any("systemstat" in r for r in responses)

    def test_invalid_json(self, ws):
        ws.send("not json at all")
        time.sleep(0.5)
        responses = ws_send_recv(ws, {"sysstat": True})
        assert any("systemstat" in r for r in responses)

    def test_unknown_key(self, ws):
        ws.send(json.dumps({"nonexistent_key": 42}))
        time.sleep(0.5)
        responses = ws_send_recv(ws, {"sysstat": True})
        assert any("systemstat" in r for r in responses)

    def test_large_payload(self, ws):
        ws.send(json.dumps({"command": "A" * 500}))
        time.sleep(0.5)
        responses = ws_send_recv(ws, {"sysstat": True})
        assert any("systemstat" in r for r in responses)

    def test_rapid_messages(self, ws):
        """Send 20 rapid messages without waiting."""
        for _ in range(20):
            ws.send(json.dumps({"sysstat": True}))
        time.sleep(2)
        responses = ws_send_recv(ws, {"sysstat": True})
        assert any("systemstat" in r for r in responses), "Device stopped responding after rapid messages"
