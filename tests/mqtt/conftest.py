import os
import pytest

try:
    import paho.mqtt.client as mqtt
    HAS_PAHO = True
except ImportError:
    HAS_PAHO = False

MQTT_BROKER = os.environ.get("MQTT_BROKER", "")
MQTT_PORT = int(os.environ.get("MQTT_PORT", "1883"))
MQTT_CMD_TOPIC = os.environ.get("MQTT_CMD_TOPIC", "itho/cmd")
DEVICE_IP = os.environ.get("ITHO_DEVICE", "<device-ip>")
DEVICE_URL = f"http://{DEVICE_IP}"
API_URL = f"{DEVICE_URL}/api.html"


def pytest_collection_modifyitems(config, items):
    if not HAS_PAHO:
        skip = pytest.mark.skip(reason="paho-mqtt not installed (pip install paho-mqtt)")
        for item in items:
            item.add_marker(skip)
    elif not MQTT_BROKER:
        skip = pytest.mark.skip(reason="MQTT_BROKER env var not set")
        for item in items:
            item.add_marker(skip)


@pytest.fixture(scope="module")
def mqtt_client():
    if not HAS_PAHO or not MQTT_BROKER:
        pytest.skip("MQTT not configured")

    client = mqtt.Client(client_id="ithowifi_test", protocol=mqtt.MQTTv311)
    client.connect(MQTT_BROKER, MQTT_PORT, 60)
    client.loop_start()
    yield client
    client.loop_stop()
    client.disconnect()
