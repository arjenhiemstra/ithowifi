import os
import pytest

DEVICE_IP = os.environ.get("ITHO_DEVICE", "")
DEVICE_URL = f"http://{DEVICE_IP}"
SPEC_URL = f"{DEVICE_URL}/api/openapi.json"


def pytest_addoption(parser):
    parser.addoption(
        "--device",
        action="store",
        default=DEVICE_IP,
        help="IP or hostname of the ithowifi device",
    )


@pytest.fixture(scope="session")
def device_url(request):
    ip = request.config.getoption("--device")
    return f"http://{ip}"
