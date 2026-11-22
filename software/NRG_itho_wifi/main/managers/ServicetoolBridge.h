/*
   Servicetool bridge - lets the official Itho Daalderop servicetool talk to
   the Itho over this module, by emulating the serial "interface cable" on a
   raw TCP socket. On Windows, map a virtual COM port (HW VSP3, Tibbo VSP,
   com0com+hub4com) in raw TCP client mode to <module-ip>:<servicetool_port>.

   While a client is connected (a "session"):
     - the module's own periodic I2C polling is paused
     - all other I2C traffic (API/MQTT/web/virtual remote commands) is refused
     - the session state is published on <mqtt_base_topic>/servicetool
   The session ends on disconnect, after servicetool_timeout minutes without
   traffic, when the bridge is disabled, or from the web UI.

   Part of NRG itho wifi firmware
*/

#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

#include "ithodevice/i2c_logger.h"

// True while a servicetool client is connected.
extern volatile bool servicetoolSessionActive;
// Set when the session state changed; consumed by the MQTT task.
extern volatile bool servicetoolMqttUpdate;
// Set when a session ended; consumed by the system control task to
// trigger an immediate status refresh.
extern volatile bool servicetoolResumePolling;
// Set from the web UI / API to force-close the current session.
extern volatile bool servicetoolEndRequested;

void startTaskServicetoolBridge();

// Called by the I2C driver when it refuses a send during a session.
void servicetoolNoteBlocked(i2c_cmdref_t origin);

// Session info for MQTT / web UI. `full` adds counters.
void servicetoolGetStatusJSON(JsonObject obj, bool full = true);

// MQTT topic for the session state: <mqtt_base_topic>/servicetool
void servicetoolMqttTopic(char *buf, size_t len);
