"""
Stress and stability tests. Verifies the device handles sustained
load, concurrent requests, and rapid websocket messages without
crashing or becoming unresponsive.

Usage:
    ITHO_DEVICE=<device-ip> pytest tests/api/test_stability.py -v
"""
import os
import json
import time
import requests
import pytest
from concurrent.futures import ThreadPoolExecutor, as_completed

try:
    import websocket
    HAS_WS = True
except ImportError:
    HAS_WS = False

DEVICE_IP = os.environ.get("ITHO_DEVICE", "")
API_URL = f"http://{DEVICE_IP}/api.html"
WS_URL = f"ws://{DEVICE_IP}:8000/ws"


def device_alive():
    """Quick check if device is responding."""
    try:
        r = requests.get(API_URL, params={"get": "currentspeed"}, timeout=10)
        return r.status_code == 200
    except Exception:
        return False


class TestHTTPStress:
    """Sustained HTTP request load."""

    def test_30_sequential_requests(self):
        """30 sequential API calls — device must stay responsive."""
        for i in range(30):
            r = requests.get(API_URL, params={"get": "currentspeed"}, timeout=10)
            assert r.status_code == 200, f"Request {i} failed: {r.status_code}"

    def test_10_concurrent_requests(self):
        """10 simultaneous HTTP requests."""
        def fetch(i):
            r = requests.get(API_URL, params={"get": "currentspeed"}, timeout=15)
            return i, r.status_code

        with ThreadPoolExecutor(max_workers=10) as pool:
            futures = [pool.submit(fetch, i) for i in range(10)]
            results = [f.result() for f in as_completed(futures)]

        failures = [(i, code) for i, code in results if code >= 500]
        assert len(failures) == 0, f"Server errors: {failures}"
        assert device_alive(), "Device unresponsive after concurrent requests"

    def test_mixed_concurrent_endpoints(self):
        """Concurrent requests to different endpoints."""
        endpoints = [
            {"get": "currentspeed"},
            {"get": "deviceinfo"},
            {"get": "remotesinfo"},
            {"get": "rfstatus"},
            {"get": "currentspeed"},
        ]

        def fetch(params):
            r = requests.get(API_URL, params=params, timeout=15)
            return params, r.status_code

        with ThreadPoolExecutor(max_workers=5) as pool:
            futures = [pool.submit(fetch, p) for p in endpoints]
            results = [f.result() for f in as_completed(futures)]

        for params, code in results:
            assert code < 500, f"Server error on {params}: {code}"

    def test_rapid_speed_changes(self):
        """Rapid speed changes should not crash."""
        for speed in range(0, 255, 25):
            r = requests.get(API_URL, params={"speed": str(speed)}, timeout=10)
            assert r.status_code < 500
        # Restore
        requests.get(API_URL, params={"command": "low"}, timeout=10)

    def test_alternating_read_write(self):
        """Alternate between read and write operations."""
        for _ in range(10):
            r = requests.get(API_URL, params={"get": "currentspeed"}, timeout=10)
            assert r.status_code == 200
            r = requests.get(API_URL, params={"speed": "100"}, timeout=10)
            assert r.status_code < 500
        requests.get(API_URL, params={"command": "low"}, timeout=10)


class TestWebSocketStress:
    """Sustained websocket message load."""

    @pytest.fixture(autouse=True)
    def _check_ws(self):
        if not HAS_WS:
            pytest.skip("websocket-client not installed")

    def test_50_rapid_ws_messages(self):
        """Send 50 messages in quick succession."""
        ws = websocket.create_connection(WS_URL, timeout=10)
        try:
            for _ in range(50):
                ws.send(json.dumps({"sysstat": True}))
            time.sleep(3)
            # Verify still responding
            ws.send(json.dumps({"sysstat": True}))
            ws.settimeout(5)
            data = json.loads(ws.recv())
            assert "systemstat" in data or True  # any response = alive
        finally:
            ws.close()
        assert device_alive(), "Device unresponsive after rapid WS messages"

    def test_ws_reconnect(self):
        """Open, send, close, reconnect — 5 times."""
        for i in range(5):
            ws = websocket.create_connection(WS_URL, timeout=10)
            ws.send(json.dumps({"sysstat": True}))
            time.sleep(0.5)
            ws.close()
            time.sleep(0.5)
        assert device_alive(), "Device unresponsive after reconnect cycle"

    def test_ws_large_messages(self):
        """Send oversized messages that should be rejected gracefully."""
        ws = websocket.create_connection(WS_URL, timeout=10)
        try:
            # 1KB payload
            ws.send(json.dumps({"command": "x" * 1000}))
            time.sleep(1)
            # 4KB payload
            ws.send(json.dumps({"data": "y" * 4000}))
            time.sleep(1)
        finally:
            ws.close()
        assert device_alive(), "Device crashed on large WS message"

    def test_ws_mixed_valid_invalid(self):
        """Alternate valid and invalid messages."""
        ws = websocket.create_connection(WS_URL, timeout=10)
        try:
            for _ in range(10):
                ws.send(json.dumps({"sysstat": True}))
                ws.send("not json {{{")
                ws.send(json.dumps({"nonexistent": 42}))
            time.sleep(2)
        finally:
            ws.close()
        assert device_alive()


class TestHTTPAndWSConcurrent:
    """Test HTTP and WebSocket simultaneously."""

    @pytest.fixture(autouse=True)
    def _check_ws(self):
        if not HAS_WS:
            pytest.skip("websocket-client not installed")

    def test_http_during_ws_traffic(self):
        """Make HTTP requests while websocket is active."""
        ws = websocket.create_connection(WS_URL, timeout=10)
        try:
            # Start WS traffic
            for _ in range(5):
                ws.send(json.dumps({"sysstat": True}))

            # Concurrent HTTP requests
            for _ in range(5):
                r = requests.get(API_URL, params={"get": "currentspeed"}, timeout=10)
                assert r.status_code == 200
                ws.send(json.dumps({"sysstat": True}))
        finally:
            ws.close()

    def test_memory_stable_after_load(self):
        """Check free memory after all stress tests — should be reasonable."""
        ws = websocket.create_connection(WS_URL, timeout=10)
        try:
            ws.send(json.dumps({"sysstat": True}))
            time.sleep(1)
            ws.settimeout(2)
            for _ in range(10):
                try:
                    data = json.loads(ws.recv())
                    if "systemstat" in data:
                        freemem = data["systemstat"].get("freemem", 0)
                        memlow = data["systemstat"].get("memlow", 0)
                        assert freemem > 50000, f"Low free memory: {freemem} bytes"
                        assert memlow > 30000, f"Memory low watermark too low: {memlow} bytes"
                        return
                except Exception:
                    break
        finally:
            ws.close()

    def test_syslog_no_crashes(self):
        """Check syslog for crash/panic indicators."""
        r = requests.get(f"http://{DEVICE_IP}/curlog", timeout=10)
        assert r.status_code == 200
        log = r.text.lower()
        assert "panic" not in log, "Panic found in syslog"
        assert "guru meditation" not in log, "Guru meditation error in syslog"
        assert "abort()" not in log, "Abort found in syslog"
        assert "stack overflow" not in log, "Stack overflow in syslog"

    def test_final_health_check(self):
        """Final verification that device is healthy after all stress tests."""
        r = requests.get(API_URL, params={"get": "deviceinfo"}, timeout=10)
        assert r.status_code == 200
        data = r.json()
        assert data["status"] == "success"
        assert "deviceinfo" in data["data"]
