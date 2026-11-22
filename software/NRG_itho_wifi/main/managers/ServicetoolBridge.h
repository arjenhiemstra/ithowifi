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

// --- shared flags (polled, see ADR-0004) -----------------------------------

// True while a servicetool client is connected. Set by TaskServicetool; read
// by the I2C driver guard, TaskSysControl and the APIs.
extern volatile bool servicetoolSessionActive;
// Session state changed: the MQTT task publishes <base>/servicetool and
// clears it.
extern volatile bool servicetoolMqttUpdate;
// A session ended: TaskSysControl schedules an immediate status refresh and
// clears it.
extern volatile bool servicetoolResumePolling;
// Set by the web UI "End session" button; TaskServicetool closes the session.
extern volatile bool servicetoolEndRequested;

// --- functions ---------------------------------------------------------------

// Create TaskServicetool (called once from boot()).
void startTaskServicetoolBridge();

// Called by the I2C driver guard for every call it refuses during a session
// (counts them and logs the first few).
void servicetoolNoteBlocked(i2c_cmdref_t origin);

// Session state for MQTT / REST / web UI:
//   {"state":"active"|"inactive","enabled":bool, ...}
// While active also client, since, last_activity, timeout_min, and with
// `full` the frame / blocked-command counters.
void servicetoolGetStatusJSON(JsonObject obj, bool full = true);

// MQTT topic for the session state: <mqtt_base_topic>/servicetool
void servicetoolMqttTopic(char *buf, size_t len);
