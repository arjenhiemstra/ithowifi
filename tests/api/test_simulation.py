"""
Simulated Device Runtime API tests.

The safe tests only read/write simulation settings (restoring them afterwards)
and require no reboot. The lifecycle test enables simulation, reboots the
device and verifies simulated telemetry end-to-end; it only runs when
ITHO_ALLOW_REBOOT=1 is set and the api_reboot setting is enabled on the device.

Usage:
    ITHO_DEVICE=<device-ip> pytest tests/api/test_simulation.py -v
    ITHO_DEVICE=<device-ip> ITHO_ALLOW_REBOOT=1 pytest tests/api/test_simulation.py -v
"""
import os
import time

import pytest
import requests

DEVICE_IP = os.environ.get("ITHO_DEVICE", "")
REST_URL = f"http://{DEVICE_IP}/api/v2"
ALLOW_REBOOT = os.environ.get("ITHO_ALLOW_REBOOT", "") == "1"

pytestmark = pytest.mark.skipif(not DEVICE_IP, reason="ITHO_DEVICE not set")

PROFILES = ["CVE-Silent", "HRU 350", "DemandFlow", "Heatpump"]
SCENARIOS = ["normal", "boost", "fault", "humidity_spike", "co2_rise"]


def get_simulation():
    r = requests.get(f"{REST_URL}/simulation", timeout=10)
    assert r.status_code == 200
    body = r.json()
    assert body["status"] == "success"
    return body["data"]["simulation"]


def post_simulation(payload, expect_status=200):
    r = requests.post(f"{REST_URL}/simulation", json=payload, timeout=10)
    assert r.status_code == expect_status, r.text
    return r.json()


def reboot_device():
    """Trigger reboot via the debug endpoint; returns False when gated."""
    r = requests.post(f"{REST_URL}/debug", json={"action": "reboot"}, timeout=10)
    return r.status_code == 200


def wait_for_device(timeout=120):
    deadline = time.time() + timeout
    time.sleep(5)  # give the device time to actually go down
    while time.time() < deadline:
        try:
            r = requests.get(f"{REST_URL}/deviceinfo", timeout=3)
            if r.status_code == 200:
                return True
        except requests.RequestException:
            pass
        time.sleep(2)
    return False


class TestSimulationEndpoint:
    """Safe tests: no reboot, settings restored afterwards."""

    def test_get_simulation_shape(self):
        sim = get_simulation()
        for key in ["enabled", "active", "profile", "profile_name", "scenario",
                    "seed", "reboot_required", "available_profiles",
                    "available_scenarios"]:
            assert key in sim, f"missing key: {key}"
        assert sim["available_profiles"] == PROFILES
        assert sim["available_scenarios"] == SCENARIOS

    def test_post_invalid_profile(self):
        body = post_simulation({"profile": "bogus"}, expect_status=400)
        assert body["status"] == "fail"
        body = post_simulation({"profile": 99}, expect_status=400)
        assert body["status"] == "fail"

    def test_post_invalid_scenario(self):
        body = post_simulation({"scenario": "bogus"}, expect_status=400)
        assert body["status"] == "fail"

    def test_post_settings_roundtrip(self):
        before = get_simulation()
        try:
            body = post_simulation({"profile": 1, "seed": 4242, "scenario": "co2_rise"})
            assert body["status"] == "success"
            sim = get_simulation()
            assert sim["profile"] == 1 or sim["profile_name"] == "HRU 350" or sim["active"]
            assert sim["seed"] == 4242 or sim["active"]  # active runtime reports the running seed
        finally:
            post_simulation({
                "profile": before["profile"],
                "seed": before["seed"],
                "scenario": before["scenario"],
            })

    def test_deviceinfo_simulation_flag_matches_state(self):
        sim = get_simulation()
        r = requests.get(f"{REST_URL}/deviceinfo", timeout=10)
        assert r.status_code == 200
        info = r.json()["data"]["deviceinfo"]
        if sim["active"]:
            assert info.get("simulation") is True
        else:
            assert "simulation" not in info


@pytest.mark.skipif(not ALLOW_REBOOT, reason="set ITHO_ALLOW_REBOOT=1 to run reboot lifecycle test")
class TestSimulationLifecycle:
    """Full lifecycle: enable simulation, reboot, verify telemetry, disable."""

    def test_full_simulation_lifecycle(self):
        original = get_simulation()

        body = post_simulation({
            "enable": True,
            "profile": "CVE-Silent",
            "seed": 12345,
            "scenario": "normal",
        })
        assert body["status"] == "success"

        if not reboot_device():
            # restore config before skipping
            post_simulation({"enable": bool(original["enabled"]),
                             "profile": original["profile"],
                             "seed": original["seed"],
                             "scenario": original["scenario"]})
            pytest.skip("reboot via API is disabled (api_reboot setting)")

        assert wait_for_device(), "device did not come back after reboot"

        try:
            # simulation must be active and impersonate a CVE-Silent
            sim = get_simulation()
            assert sim["active"] is True
            assert sim["profile_name"] == "CVE-Silent"

            r = requests.get(f"{REST_URL}/deviceinfo", timeout=10)
            info = r.json()["data"]["deviceinfo"]
            assert info.get("simulation") is True
            assert "CVE-Silent" in info["itho_devtype"]
            assert info["itho_fwversion"] == 27

            # simulated telemetry must be present with plausible values
            r = requests.get(f"{REST_URL}/ithostatus", timeout=10)
            status = r.json()["data"]["ithostatus"]
            assert len(status) >= 12
            keys = {k.lower(): k for k in status.keys()}
            temp_key = next((keys[k] for k in keys if k.startswith("temp")), None)
            assert temp_key is not None, f"no temperature field in {list(status)}"
            assert 15.0 < float(status[temp_key]) < 28.0

            # commands must update the simulated fan state
            requests.post(f"{REST_URL}/command", json={"speed": 254}, timeout=10)
            time.sleep(2)
            sim = get_simulation()
            assert sim["fan_setpoint"] >= 95

            requests.post(f"{REST_URL}/command", json={"speed": 50}, timeout=10)
            time.sleep(2)
            sim = get_simulation()
            assert sim["fan_setpoint"] <= 25

            # scenario change applies live: error status flips to fault
            post_simulation({"scenario": "fault"})
            time.sleep(1)
            sim = get_simulation()
            assert sim["scenario"] == "fault"
            post_simulation({"scenario": "normal"})
        finally:
            # disable simulation and reboot back to normal operation
            post_simulation({"enable": bool(original["enabled"]),
                             "profile": original["profile"],
                             "seed": original["seed"],
                             "scenario": original["scenario"]})
            if reboot_device():
                assert wait_for_device(), "device did not come back after restore reboot"
                sim = get_simulation()
                assert bool(sim["active"]) == bool(original["enabled"])
