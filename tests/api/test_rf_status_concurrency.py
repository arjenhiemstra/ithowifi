"""
RF status concurrency / data-race regression tests.

Targets the fix for the rfStatusSource.measurements31DA/31D9 data race:
those per-source vectors are cleared and repopulated by the CC1101 task
(parseRF31DA / parseRF31D9) whenever an RF frame arrives, while readers
(getRFStatusJSON via /api/v2/rfstatus, the MQTT publisher, and the websocket
handler) iterate them. Before the rfStatusMutex fix these ran with no
synchronization, so a clear()+realloc during a reader's iteration was a
use-after-free -> LoadProhibited crash / reboot.

This device must have RF onboard and be receiving 31DA/31D9 frames from a real
unit (that is the writer side). We drive the reader side hard and assert the
device never returns a 5xx, never returns malformed JSON, never crashes
(syslog), and stays responsive. The assertions hold whether or not a frame
happens to land inside a read window, so the test is not flaky; it only *fails*
when the race actually corrupts a response or crashes the device.

Usage:
    ITHO_DEVICE=<device-ip> pytest tests/api/test_rf_status_concurrency.py -v
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
REST_URL = f"http://{DEVICE_IP}/api/v2"
WS_URL = f"ws://{DEVICE_IP}:8000/ws"

# Tuning: enough sustained concurrency and wall-clock to overlap several
# incoming RF frames. Kept modest so the run stays well under a minute.
WORKERS = 12
DURATION_S = 20


def device_alive():
    try:
        r = requests.get(f"{REST_URL}/deviceinfo", timeout=10)
        return r.status_code == 200
    except Exception:
        return False


def get_rfstatus():
    """One /rfstatus read; returns parsed JSON or raises on any anomaly."""
    r = requests.get(f"{REST_URL}/rfstatus", timeout=15)
    assert r.status_code == 200, f"status {r.status_code}"
    data = r.json()  # raises on truncated / garbage body
    assert data.get("status") == "success", f"payload status {data.get('status')}"
    return data


@pytest.fixture(scope="module")
def _require_rf():
    """Skip the whole module if this device has no RF sources at all — the
    race only exists when measurement vectors get populated from RF frames."""
    try:
        data = get_rfstatus()
    except Exception as e:
        pytest.skip(f"/rfstatus not usable on this device: {e}")
    sources = data["data"]["rfstatus"].get("sources", [])
    if not sources:
        pytest.skip("device is not tracking any RF sources (no writer side to race)")
    return sources


class TestRFStatusBaseline:
    def test_rfstatus_returns_success(self, _require_rf):
        data = get_rfstatus()
        assert "rfstatus" in data["data"]
        assert isinstance(data["data"]["rfstatus"]["sources"], list)

    def test_rfstatus_sources_wellformed(self, _require_rf):
        for s in _require_rf:
            assert "id" in s and "index" in s
            assert "data" in s and isinstance(s["data"], dict)


class TestRFStatusRaceSafety:
    """Sustained concurrent reads of the per-source measurement vectors."""

    def test_sustained_concurrent_rfstatus(self, _require_rf):
        errors = []
        counts = [0]

        def worker(n):
            deadline = time.time() + DURATION_S
            local = 0
            while time.time() < deadline:
                try:
                    get_rfstatus()
                    local += 1
                except Exception as e:
                    errors.append((n, repr(e)))
                    return
            counts[0] += local

        with ThreadPoolExecutor(max_workers=WORKERS) as pool:
            list(as_completed([pool.submit(worker, i) for i in range(WORKERS)]))

        assert not errors, f"{len(errors)} concurrent /rfstatus failures; first few: {errors[:5]}"
        assert device_alive(), "device unresponsive after sustained rfstatus load"

    def test_rfstatus_mixed_with_tracked_name(self, _require_rf):
        """Also exercise the per-source (name-filtered) read path, which
        iterates a single source's vectors — same clear()/realloc hazard."""
        names = [s.get("name") for s in _require_rf if s.get("name")]
        urls = [f"{REST_URL}/rfstatus"]
        urls += [f"{REST_URL}/rfstatus?name={n}" for n in names[:3]]

        errors = []

        def worker(_):
            deadline = time.time() + 10
            while time.time() < deadline:
                for u in urls:
                    try:
                        r = requests.get(u, timeout=15)
                        if r.status_code >= 500:
                            errors.append((u, r.status_code))
                            return
                        r.json()  # must always parse
                    except Exception as e:
                        errors.append((u, repr(e)))
                        return

        with ThreadPoolExecutor(max_workers=WORKERS) as pool:
            list(as_completed([pool.submit(worker, i) for i in range(WORKERS)]))

        assert not errors, f"errors on mixed rfstatus reads: {errors[:5]}"
        assert device_alive()

    @pytest.mark.skipif(not HAS_WS, reason="websocket-client not installed")
    def test_rfstatus_http_and_ws_concurrent(self, _require_rf):
        """HTTP readers + websocket rfstatus readers hitting the same vectors
        at once — maximises overlap with the CC1101 writer."""
        stop = time.time() + 12
        errors = []

        def http_worker(_):
            while time.time() < stop:
                try:
                    get_rfstatus()
                except Exception as e:
                    errors.append(("http", repr(e)))
                    return

        def ws_worker(_):
            try:
                ws = websocket.create_connection(WS_URL, timeout=10)
            except Exception as e:
                errors.append(("ws-connect", repr(e)))
                return
            try:
                while time.time() < stop:
                    ws.send(json.dumps({"rfstatus": True}))
                    time.sleep(0.05)
            except Exception as e:
                errors.append(("ws", repr(e)))
            finally:
                try:
                    ws.close()
                except Exception:
                    pass

        with ThreadPoolExecutor(max_workers=WORKERS) as pool:
            futs = [pool.submit(http_worker, i) for i in range(WORKERS - 2)]
            futs += [pool.submit(ws_worker, i) for i in range(2)]
            list(as_completed(futs))

        # ws send errors alone don't prove a device fault; a crash shows up as
        # http failures or the liveness/syslog checks below.
        http_errors = [e for e in errors if e[0] == "http"]
        assert not http_errors, f"http failures during http+ws load: {http_errors[:5]}"
        assert device_alive()


class TestNoCrashAfterRFStress:
    def test_syslog_clean(self, _require_rf):
        r = requests.get(f"http://{DEVICE_IP}/curlog", timeout=10)
        assert r.status_code == 200
        log = r.text.lower()
        for marker in (
            "panic",
            "guru meditation",
            "loadprohibited",
            "storeprohibited",
            "abort()",
            "stack overflow",
            "use-after",
        ):
            assert marker not in log, f"crash indicator '{marker}' in syslog after RF stress"

    def test_device_healthy(self, _require_rf):
        assert device_alive(), "device not healthy after RF concurrency tests"
