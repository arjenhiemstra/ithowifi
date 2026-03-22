import os
import pytest
import requests

try:
    import paho.mqtt.client as mqtt
    HAS_PAHO = True
except ImportError:
    HAS_PAHO = False

DEVICE_IP = os.environ.get("ITHO_DEVICE", "")
DEVICE_URL = f"http://{DEVICE_IP}"
API_URL = f"{DEVICE_URL}/api.html"

# Read MQTT config from the device itself
MQTT_BROKER = os.environ.get("MQTT_BROKER", "")
MQTT_PORT = int(os.environ.get("MQTT_PORT", "1883"))
MQTT_USERNAME = os.environ.get("MQTT_USERNAME", "")
MQTT_PASSWORD = os.environ.get("MQTT_PASSWORD", "")
MQTT_CMD_TOPIC = os.environ.get("MQTT_CMD_TOPIC", "")

if not MQTT_BROKER:
    try:
        r = requests.get(f"{DEVICE_URL}/config.json", timeout=5)
        if r.status_code == 200:
            cfg = r.json()
            MQTT_BROKER = cfg.get("mqtt_serverName", "")
            MQTT_PORT = int(cfg.get("mqtt_port", 1883))
            MQTT_USERNAME = cfg.get("mqtt_username", "")
            base_topic = cfg.get("mqtt_base_topic", "itho")
            MQTT_CMD_TOPIC = f"{base_topic}/cmd"
    except Exception:
        pass

# Password can't be read from config (masked) — must be provided via env
if not MQTT_PASSWORD:
    MQTT_PASSWORD = os.environ.get("MQTT_PASSWORD", "")


def pytest_collection_modifyitems(config, items):
    if not HAS_PAHO:
        skip = pytest.mark.skip(reason="paho-mqtt not installed (pip install paho-mqtt)")
        for item in items:
            item.add_marker(skip)
    elif not MQTT_BROKER:
        skip = pytest.mark.skip(reason="MQTT_BROKER not available (set env or ensure device config.json is accessible)")
        for item in items:
            item.add_marker(skip)


def create_mqtt_client(client_id="ithowifi_test"):
    """Create an MQTT client with credentials from device config."""
    client = mqtt.Client(client_id=client_id, protocol=mqtt.MQTTv311)
    if MQTT_USERNAME:
        client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)
    client.connect(MQTT_BROKER, MQTT_PORT, 60)
    return client


@pytest.fixture(scope="module")
def mqtt_client():
    if not HAS_PAHO or not MQTT_BROKER:
        pytest.skip("MQTT not configured")

    client = create_mqtt_client()
    client.loop_start()
    yield client
    client.loop_stop()
    client.disconnect()
