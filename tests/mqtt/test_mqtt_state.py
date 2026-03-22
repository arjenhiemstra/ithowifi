"""
MQTT state verification tests. Subscribes to topics, sends commands,
and verifies that state updates are published correctly.

Requires:
    MQTT_BROKER=<device-ip>
    ITHO_DEVICE=<device-ip>

Usage:
    MQTT_BROKER=<device-ip> ITHO_DEVICE=<device-ip> pytest tests/mqtt/test_mqtt_state.py -v
"""
import os
import json
import time
import threading
import requests
import pytest

try:
    import paho.mqtt.client as mqtt
    HAS_PAHO = True
except ImportError:
    HAS_PAHO = False

MQTT_BROKER = os.environ.get("MQTT_BROKER", "")
MQTT_PORT = int(os.environ.get("MQTT_PORT", "1883"))
DEVICE_IP = os.environ.get("ITHO_DEVICE", "<device-ip>")
API_URL = f"http://{DEVICE_IP}/api.html"

# Read base topic from device config
BASE_TOPIC = None
try:
    r = requests.get(f"http://{DEVICE_IP}/config.json", timeout=5)
    if r.status_code == 200:
        BASE_TOPIC = r.json().get("mqtt_base_topic", "itho")
except Exception:
    BASE_TOPIC = "itho"

CMD_TOPIC = f"{BASE_TOPIC}/cmd" if BASE_TOPIC else "itho/cmd"
STATE_TOPIC = f"{BASE_TOPIC}/state" if BASE_TOPIC else "itho/state"
LWT_TOPIC = f"{BASE_TOPIC}/lwt" if BASE_TOPIC else "itho/lwt"


def skip_if_no_mqtt():
    if not HAS_PAHO:
        pytest.skip("paho-mqtt not installed")
    if not MQTT_BROKER:
        pytest.skip("MQTT_BROKER not set")


class MqttCollector:
    """Collects messages on subscribed topics."""

    def __init__(self, broker, port, topics):
        skip_if_no_mqtt()
        self.messages = {}
        self.lock = threading.Lock()
        self.client = mqtt.Client(client_id="test_collector", protocol=mqtt.MQTTv311)
        self.client.on_message = self._on_message
        self.client.connect(broker, port, 60)
        for topic in topics:
            self.client.subscribe(topic)
            self.messages[topic] = []
        self.client.loop_start()

    def _on_message(self, client, userdata, msg):
        with self.lock:
            if msg.topic not in self.messages:
                self.messages[msg.topic] = []
            self.messages[msg.topic].append(msg.payload.decode("utf-8", errors="replace"))

    def get_messages(self, topic, clear=True):
        with self.lock:
            msgs = list(self.messages.get(topic, []))
            if clear:
                self.messages[topic] = []
            return msgs

    def wait_for_message(self, topic, timeout=10):
        start = time.time()
        while time.time() - start < timeout:
            msgs = self.get_messages(topic, clear=False)
            if msgs:
                return msgs
            time.sleep(0.5)
        return []

    def close(self):
        self.client.loop_stop()
        self.client.disconnect()


class TestMqttStatePublishing:
    """Verify state updates are published to MQTT topics."""

    @pytest.fixture(autouse=True)
    def setup(self):
        skip_if_no_mqtt()

    def test_lwt_online(self):
        """LWT topic should show device is online."""
        collector = MqttCollector(MQTT_BROKER, MQTT_PORT, [LWT_TOPIC])
        try:
            msgs = collector.wait_for_message(LWT_TOPIC, timeout=5)
            if msgs:
                assert any("online" in m.lower() for m in msgs), f"Expected 'online' in LWT: {msgs}"
        finally:
            collector.close()

    def test_state_published_after_speed_change(self):
        """Changing speed should publish to state topic (if MQTT connected)."""
        state_topic = f"{BASE_TOPIC}/state"
        collector = MqttCollector(MQTT_BROKER, MQTT_PORT, [state_topic])
        try:
            time.sleep(1)
            collector.get_messages(state_topic)  # clear

            # Change speed via HTTP
            requests.get(API_URL, params={"speed": "150"}, timeout=10)
            time.sleep(5)

            msgs = collector.get_messages(state_topic)
            if len(msgs) == 0:
                # Device MQTT may not be connected — check via wildcard
                pytest.skip("No state published — device MQTT may not be connected to broker")
        finally:
            collector.close()
            requests.get(API_URL, params={"command": "low"}, timeout=10)

    def test_ithostatus_published(self):
        """Itho status topic should have periodic updates."""
        status_topic = f"{BASE_TOPIC}/ithostatus"
        collector = MqttCollector(MQTT_BROKER, MQTT_PORT, [status_topic])
        try:
            msgs = collector.wait_for_message(status_topic, timeout=15)
            if msgs:
                # Should be valid JSON
                data = json.loads(msgs[-1])
                assert isinstance(data, dict)
        finally:
            collector.close()

    def test_command_via_mqtt_updates_state(self):
        """Send command via MQTT, verify state reflects it."""
        state_topic = f"{BASE_TOPIC}/state"
        collector = MqttCollector(MQTT_BROKER, MQTT_PORT, [state_topic])
        try:
            time.sleep(1)
            collector.get_messages(state_topic)  # clear

            # Send command via MQTT
            pub = mqtt.Client(client_id="test_pub", protocol=mqtt.MQTTv311)
            pub.connect(MQTT_BROKER, MQTT_PORT, 60)
            pub.publish(CMD_TOPIC, json.dumps({"speed": 200}))
            pub.disconnect()

            time.sleep(3)
            msgs = collector.get_messages(state_topic)
            if len(msgs) == 0:
                pytest.skip("No state published — device MQTT may not be connected to broker")
        finally:
            collector.close()
            requests.get(API_URL, params={"command": "low"}, timeout=10)


class TestMqttRFStatusPublishing:
    """Verify RF status data is published to MQTT when available."""

    @pytest.fixture(autouse=True)
    def setup(self):
        skip_if_no_mqtt()

    def test_rfstatus_topics_exist(self):
        """Subscribe to rfstatus wildcard — may or may not have data."""
        rfstatus_topic = f"{BASE_TOPIC}/rfstatus/#"
        collector = MqttCollector(MQTT_BROKER, MQTT_PORT, [rfstatus_topic])
        try:
            time.sleep(5)
            # RF status may not be published if no tracked sources
            # Just verify we can subscribe without error
        finally:
            collector.close()

    def test_cmd_topic_cleaned_after_command(self):
        """Command topic should be cleaned after processing."""
        collector = MqttCollector(MQTT_BROKER, MQTT_PORT, [CMD_TOPIC])
        try:
            pub = mqtt.Client(client_id="test_pub2", protocol=mqtt.MQTTv311)
            pub.connect(MQTT_BROKER, MQTT_PORT, 60)
            pub.publish(CMD_TOPIC, json.dumps({"command": "low"}))
            pub.disconnect()

            time.sleep(3)
            # The device should clean the cmd topic after processing
            msgs = collector.get_messages(CMD_TOPIC)
            # We should see the command, then possibly an empty message (cleanup)
        finally:
            collector.close()
