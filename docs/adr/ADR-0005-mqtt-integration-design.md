# ADR-0005: MQTT Integration Design

Status: Accepted

## Context

Home automation integration is a core project goal (README: "Easily integrate in any home automation system through MQTT or REST API", HA MQTT Discovery support). Evidence: `main/tasks/task_mqtt.cpp` runs the MQTT client task, publishing to a base-topic-prefixed set of topics (`<base>/cmd`, `<base>/cmd/response`, `<base>/lwt`, `<base>/state`, `<base>/ithostatus`, `<base>/rfstatus/<srcName>/31DA|31D9`, `<base>/remotesinfo`, `<base>/lastcmd`, `<base>/deviceinfo`), plus a separate Home Assistant discovery topic (`<ha_topic>/device/<hostName>/config`) and a Domoticz bridge (`mqtt_domoticzin_topic`/`mqtt_domoticzout_topic`). Inbound commands are parsed in `main/api/MqttAPI.cpp:24` (`mqttCallback`) and dispatched to the same `processXxx` logic functions used by REST v2.

## Decision

Structure MQTT topics under a single configurable base topic (`systemConfig.mqtt_base_topic`), route inbound commands through the shared `processXxx` API logic layer (not a separate MQTT-only command parser), and support HA MQTT Discovery and a Domoticz bridge as first-class, separately-configured integrations.

## Consequences

**Positive**: MQTT and REST share validation/business logic, reducing duplication and drift; base-topic configurability lets multiple devices coexist on one broker; HA Discovery removes manual entity setup for the most common integration target.

**Negative**: topic naming changes are a breaking change for existing home-automation configs (HA, Homey, Domoticz integrations depend on the exact topic strings) — treat any topic rename/removal as requiring a spec + this ADR's update, not an incidental change.

## Alternatives Considered

- **Separate MQTT-specific command validation**: rejected in favor of reusing `processXxx` — evidenced by `MqttAPI.cpp` calling into the same logic layer as `WebAPIv2Rest.cpp`.
- **No Domoticz bridge**: rejected — dedicated `mqtt_domoticzin_topic`/`mqtt_domoticzout_topic` config fields exist specifically for this.

## Related Components

`main/tasks/task_mqtt.cpp`, `main/api/MqttAPI.cpp`, `main/config/SystemConfig.h`, `main/HADiscovery.cpp`

## Related Issues

None tracked.

## AI Notes

New MQTT commands/topics must reuse an existing `processXxx` function or add a new one following the same pattern — don't hand-parse MQTT payloads separately from the REST validation path. Any topic string change is a breaking change for downstream HA/Homey/Domoticz integrations; flag it explicitly in the spec's "Risks" section (see [specs/templates/api-spec.md](../../specs/templates/api-spec.md)).
