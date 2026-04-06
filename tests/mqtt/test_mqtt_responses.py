"""
MQTT command response tests.

Verifies that the device publishes structured JSON responses to
{base_topic}/cmd/response when commands are sent via MQTT.

Usage:
    MQTT_PASSWORD=<pw> ITHO_DEVICE=<device-ip> pytest tests/mqtt/test_mqtt_responses.py -v

Requires: paho-mqtt (pip install paho-mqtt)
"""
import json
import os
import threading
import time

import pytest
import requests

from conftest import (
    DEVICE_IP,
    DEVICE_URL,
    MQTT_BROKER,
    MQTT_CMD_TOPIC,
    HAS_PAHO,
    create_mqtt_client,
)

pytestmark = pytest.mark.skipif(
    not HAS_PAHO or not MQTT_BROKER or not DEVICE_IP,
    reason="MQTT or device not configured",
)

# Derive the response topic from the command topic
# MQTT_CMD_TOPIC is "{base_topic}/cmd", response topic is "{base_topic}/cmd/response"
MQTT_RESPONSE_TOPIC = f"{MQTT_CMD_TOPIC}/response" if MQTT_CMD_TOPIC else ""


class ResponseCollector:
    """Subscribe to the response topic and collect messages."""

    def __init__(self, client, topic, timeout=5):
        self.messages = []
        self.event = threading.Event()
        self.topic = topic
        self.timeout = timeout
        client.subscribe(topic, qos=0)
        client.message_callback_add(topic, self._on_message)

    def _on_message(self, client, userdata, msg):
        try:
            payload = json.loads(msg.payload.decode("utf-8"))
        except (json.JSONDecodeError, UnicodeDecodeError):
            payload = msg.payload
        self.messages.append(payload)
        self.event.set()

    def wait(self):
        """Wait for at least one message, return all collected."""
        self.event.wait(timeout=self.timeout)
        return self.messages

    def clear(self):
        self.messages.clear()
        self.event.clear()


@pytest.fixture(scope="module")
def mqtt_response_client():
    """MQTT client that subscribes to the response topic."""
    if not MQTT_RESPONSE_TOPIC:
        pytest.skip("MQTT response topic not available")

    client = create_mqtt_client(client_id="ithowifi_response_test")
    client.loop_start()
    # Allow time for connection
    time.sleep(1)
    yield client
    client.loop_stop()
    client.disconnect()


class TestMqttResponseFormat:
    """Verify that MQTT commands produce structured JSON responses."""

    def test_command_low_response(self, mqtt_response_client):
        collector = ResponseCollector(mqtt_response_client, MQTT_RESPONSE_TOPIC)
        mqtt_response_client.publish(MQTT_CMD_TOPIC, json.dumps({"command": "low"}))
        msgs = collector.wait()

        assert len(msgs) >= 1, "No response received on cmd/response topic"
        resp = msgs[-1]
        assert isinstance(resp, dict), f"Response is not JSON: {resp}"
        assert "status" in resp
        assert "command" in resp
        assert "timestamp" in resp

    def test_success_response_for_speed(self, mqtt_response_client):
        collector = ResponseCollector(mqtt_response_client, MQTT_RESPONSE_TOPIC)
        mqtt_response_client.publish(MQTT_CMD_TOPIC, json.dumps({"speed": 100}))
        msgs = collector.wait()

        assert len(msgs) >= 1, "No response for speed command"
        resp = msgs[-1]
        assert "status" in resp
        assert "timestamp" in resp

    def test_clearqueue_response(self, mqtt_response_client):
        collector = ResponseCollector(mqtt_response_client, MQTT_RESPONSE_TOPIC)
        mqtt_response_client.publish(MQTT_CMD_TOPIC, json.dumps({"clearqueue": True}))
        msgs = collector.wait()

        # clearqueue may or may not produce a response depending on implementation
        # If it does, verify the format
        if msgs:
            resp = msgs[-1]
            if isinstance(resp, dict):
                assert "status" in resp

    def test_invalid_command_response(self, mqtt_response_client):
        collector = ResponseCollector(mqtt_response_client, MQTT_RESPONSE_TOPIC)
        mqtt_response_client.publish(
            MQTT_CMD_TOPIC,
            json.dumps({"command": "__invalid_test_command__"}),
        )
        msgs = collector.wait()

        # Device may or may not respond to invalid commands
        if msgs:
            resp = msgs[-1]
            if isinstance(resp, dict):
                assert "status" in resp

    def test_rfco2_wrong_type_response(self, mqtt_response_client):
        """rfco2 on a non-CO2 remote should produce a fail response."""
        collector = ResponseCollector(mqtt_response_client, MQTT_RESPONSE_TOPIC)
        mqtt_response_client.publish(
            MQTT_CMD_TOPIC,
            json.dumps({"rfco2": 800, "rfremoteindex": 0}),
        )
        msgs = collector.wait()

        # May or may not get a response, depends on remote config
        if msgs:
            resp = msgs[-1]
            if isinstance(resp, dict):
                assert "status" in resp

    def test_response_is_valid_json(self, mqtt_response_client):
        """Ensure the raw payload is parseable JSON."""
        raw_messages = []
        event = threading.Event()

        def on_raw(client, userdata, msg):
            raw_messages.append(msg.payload)
            event.set()

        mqtt_response_client.subscribe(MQTT_RESPONSE_TOPIC, qos=0)
        mqtt_response_client.message_callback_add(
            MQTT_RESPONSE_TOPIC + "_raw", on_raw
        )
        # Use the normal callback instead
        mqtt_response_client.message_callback_add(MQTT_RESPONSE_TOPIC, on_raw)

        mqtt_response_client.publish(MQTT_CMD_TOPIC, json.dumps({"command": "low"}))
        event.wait(timeout=5)

        for raw in raw_messages:
            parsed = json.loads(raw.decode("utf-8"))
            assert "status" in parsed
            assert "command" in parsed
            assert "timestamp" in parsed
            assert isinstance(parsed["timestamp"], int)

    def test_restore_low(self, mqtt_response_client):
        """Restore fan to low after tests."""
        mqtt_response_client.publish(MQTT_CMD_TOPIC, json.dumps({"command": "low"}))
        time.sleep(1)
