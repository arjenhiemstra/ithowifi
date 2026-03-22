"""
MQTT API integration tests.

Requires:
  - paho-mqtt: pip install paho-mqtt
  - MQTT broker running and device connected
  - Environment variables:
      MQTT_BROKER=<broker-ip> (broker IP)
      MQTT_CMD_TOPIC=itho/cmd (command topic)
      ITHO_DEVICE=<device-ip> (for HTTP verification)

Usage:
    MQTT_BROKER=<broker-ip> pytest tests/mqtt/test_mqtt_commands.py -v
"""
import json
import os
import time
import requests
import pytest

from conftest import MQTT_CMD_TOPIC, API_URL


class TestMqttJsonCommands:
    """Test MQTT JSON command handling."""

    def test_command_low(self, mqtt_client):
        mqtt_client.publish(MQTT_CMD_TOPIC, json.dumps({"command": "low"}))
        time.sleep(2)
        r = requests.get(API_URL, params={"get": "currentspeed"}, timeout=10)
        assert r.status_code == 200

    def test_command_medium(self, mqtt_client):
        mqtt_client.publish(MQTT_CMD_TOPIC, json.dumps({"command": "medium"}))
        time.sleep(2)
        r = requests.get(API_URL, params={"get": "currentspeed"}, timeout=10)
        assert r.status_code == 200

    def test_command_high(self, mqtt_client):
        mqtt_client.publish(MQTT_CMD_TOPIC, json.dumps({"command": "high"}))
        time.sleep(2)

    def test_speed_json(self, mqtt_client):
        mqtt_client.publish(MQTT_CMD_TOPIC, json.dumps({"speed": 150}))
        time.sleep(2)

    def test_speed_with_timer(self, mqtt_client):
        mqtt_client.publish(MQTT_CMD_TOPIC, json.dumps({"speed": 200, "timer": 1}))
        time.sleep(2)

    def test_clearqueue(self, mqtt_client):
        mqtt_client.publish(MQTT_CMD_TOPIC, json.dumps({"command": "clearqueue"}))
        time.sleep(1)


class TestMqttRFCommands:
    """Test RF-specific MQTT commands (rfco2, rfdemand)."""

    def test_rfco2(self, mqtt_client):
        """rfco2 via MQTT should not crash the device."""
        mqtt_client.publish(MQTT_CMD_TOPIC, json.dumps({"rfco2": 800, "rfremoteindex": 0}))
        time.sleep(1)
        r = requests.get(API_URL, params={"get": "currentspeed"}, timeout=10)
        assert r.status_code == 200, "Device not responding after rfco2"

    def test_rfco2_without_index(self, mqtt_client):
        mqtt_client.publish(MQTT_CMD_TOPIC, json.dumps({"rfco2": 600}))
        time.sleep(1)

    def test_rfdemand(self, mqtt_client):
        mqtt_client.publish(MQTT_CMD_TOPIC, json.dumps({"rfdemand": 100, "rfremoteindex": 0}))
        time.sleep(1)

    def test_rfdemand_with_zone(self, mqtt_client):
        mqtt_client.publish(MQTT_CMD_TOPIC, json.dumps({"rfdemand": 50, "rfzone": 1}))
        time.sleep(1)

    def test_rfremotecmd(self, mqtt_client):
        mqtt_client.publish(MQTT_CMD_TOPIC, json.dumps({"rfremotecmd": "low", "rfremoteindex": 0}))
        time.sleep(1)


class TestMqttPlainText:
    """Test plain-text MQTT commands (non-JSON)."""

    def test_plain_text_low(self, mqtt_client):
        mqtt_client.publish(MQTT_CMD_TOPIC, "low")
        time.sleep(2)

    def test_plain_text_speed(self, mqtt_client):
        """Plain numeric value should set speed."""
        mqtt_client.publish(MQTT_CMD_TOPIC, "128")
        time.sleep(2)


class TestMqttEdgeCases:
    """Test error handling for malformed MQTT messages."""

    def test_invalid_json(self, mqtt_client):
        """Malformed JSON should not crash."""
        mqtt_client.publish(MQTT_CMD_TOPIC, "{invalid json!!")
        time.sleep(2)
        r = requests.get(API_URL, params={"get": "currentspeed"}, timeout=10)
        assert r.status_code == 200, "Device crashed after invalid JSON"

    def test_empty_message(self, mqtt_client):
        mqtt_client.publish(MQTT_CMD_TOPIC, "")
        time.sleep(1)
        r = requests.get(API_URL, params={"get": "currentspeed"}, timeout=10)
        assert r.status_code == 200

    def test_very_long_message(self, mqtt_client):
        mqtt_client.publish(MQTT_CMD_TOPIC, json.dumps({"command": "A" * 500}))
        time.sleep(1)
        r = requests.get(API_URL, params={"get": "currentspeed"}, timeout=10)
        assert r.status_code == 200

    def test_unknown_key(self, mqtt_client):
        mqtt_client.publish(MQTT_CMD_TOPIC, json.dumps({"nonexistent": "value"}))
        time.sleep(1)

    def test_restore_low(self, mqtt_client):
        mqtt_client.publish(MQTT_CMD_TOPIC, json.dumps({"command": "low"}))
        time.sleep(2)
