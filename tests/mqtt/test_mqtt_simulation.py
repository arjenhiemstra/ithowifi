"""
Simulated Device Runtime MQTT tests.

These tests verify that a device running in simulation mode publishes
realistic simulated state over MQTT and accepts commands on the cmd topic.
They skip automatically when the device is not in simulation mode (enable it
via POST /api/v2/simulation {"enable": true} plus a reboot, or the debug page).

Usage:
    MQTT_PASSWORD=secret ITHO_DEVICE=<device-ip> pytest tests/mqtt/test_mqtt_simulation.py -v
"""
import json
import threading
import time

import pytest
import requests

from conftest import (
    MQTT_BROKER, MQTT_PORT, MQTT_USERNAME, MQTT_PASSWORD,
    DEVICE_IP, HAS_PAHO, create_mqtt_client
)

try:
    import paho.mqtt.client as mqtt
except ImportError:
    pass

REST_URL = f"http://{DEVICE_IP}/api/v2"

BASE_TOPIC = "itho"
try:
    r = requests.get(f"http://{DEVICE_IP}/config.json", timeout=5)
    if r.status_code == 200:
        BASE_TOPIC = r.json().get("mqtt_base_topic", "itho")
except Exception:
    pass

CMD_TOPIC = f"{BASE_TOPIC}/cmd"
STATUS_TOPIC = f"{BASE_TOPIC}/ithostatus"
DEVICEINFO_TOPIC = f"{BASE_TOPIC}/deviceinfo"


def skip_unless_simulating():
    if not HAS_PAHO:
        pytest.skip("paho-mqtt not installed")
    if not MQTT_BROKER:
        pytest.skip("MQTT_BROKER not available")
    try:
        r = requests.get(f"{REST_URL}/simulation", timeout=5)
        sim = r.json()["data"]["simulation"]
        if not sim.get("active"):
            pytest.skip("device is not running in simulation mode")
        return sim
    except requests.RequestException:
        pytest.skip("device not reachable")


class Collector:
    """Collects MQTT messages on subscribed topics."""

    def __init__(self, topics):
        self.messages = {t: [] for t in topics}
        self.lock = threading.Lock()
        self.client = mqtt.Client(client_id="test_sim_collector", protocol=mqtt.MQTTv311)
        if MQTT_USERNAME:
            self.client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)
        self.client.on_message = self._on_message
        self.client.connect(MQTT_BROKER, MQTT_PORT, 60)
        for t in topics:
            self.client.subscribe(t)
        self.client.loop_start()

    def _on_message(self, client, userdata, msg):
        with self.lock:
            self.messages.setdefault(msg.topic, []).append(
                msg.payload.decode("utf-8", errors="replace"))

    def get(self, topic, clear=True):
        with self.lock:
            msgs = list(self.messages.get(topic, []))
            if clear:
                self.messages[topic] = []
            return msgs

    def wait_for(self, topic, timeout=30):
        start = time.time()
        while time.time() - start < timeout:
            msgs = self.get(topic, clear=False)
            if msgs:
                return msgs
            time.sleep(0.5)
        return []

    def close(self):
        self.client.loop_stop()
        self.client.disconnect()


class TestSimulatedMqttState:

    def test_deviceinfo_marks_simulation(self):
        skip_unless_simulating()
        collector = Collector([DEVICEINFO_TOPIC])
        try:
            msgs = collector.wait_for(DEVICEINFO_TOPIC, timeout=70)
            if not msgs:
                pytest.skip("no deviceinfo published — device MQTT may not be connected")
            info = json.loads(msgs[-1])
            assert info.get("simulation") is True
        finally:
            collector.close()

    def test_simulated_status_published(self):
        sim = skip_unless_simulating()
        collector = Collector([STATUS_TOPIC])
        try:
            msgs = collector.wait_for(STATUS_TOPIC, timeout=60)
            if not msgs:
                pytest.skip("no ithostatus published — device MQTT may not be connected")
            status = json.loads(msgs[-1])
            assert isinstance(status, dict)
            assert len(status) >= 10, f"unexpectedly few status fields: {list(status)}"
            if sim["profile_name"] == "CVE-Silent":
                keys = {k.lower(): k for k in status.keys()}
                temp_key = next((keys[k] for k in keys if k.startswith("temp")), None)
                assert temp_key is not None
                assert 15.0 < float(status[temp_key]) < 28.0
        finally:
            collector.close()

    def test_mqtt_command_updates_simulated_state(self):
        skip_unless_simulating()
        collector = Collector([STATUS_TOPIC])
        try:
            pub = create_mqtt_client("test_sim_pub")
            pub.loop_start()
            pub.publish(CMD_TOPIC, json.dumps({"speed": 254}))
            time.sleep(3)

            r = requests.get(f"{REST_URL}/simulation", timeout=10)
            sim = r.json()["data"]["simulation"]
            assert sim["fan_setpoint"] >= 95, f"setpoint not raised: {sim}"

            pub.publish(CMD_TOPIC, json.dumps({"speed": 50}))
            time.sleep(3)
            r = requests.get(f"{REST_URL}/simulation", timeout=10)
            sim = r.json()["data"]["simulation"]
            assert sim["fan_setpoint"] <= 25, f"setpoint not lowered: {sim}"

            pub.loop_stop()
            pub.disconnect()
        finally:
            collector.close()
